#pragma once
// Independent OpenGL ES 3 renderer. The MAME core only publishes blitter events;
// this class replays them into two persistent HD video planes (the same model as
// the ITech32 VRAM) using redrawn RGBA textures, then scans the planes out.
// The native framebuffer of the core is never read or displayed.
#include "gl_api.hpp"
#include "hd_asset_pack.hpp"
#include "hd_blit.hpp"
#include "replacement_shader.hpp"
#include "visual_queue.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

class HdRenderer {
public:
 struct Stats { uint64_t frames=0,resolvedBlits=0,unresolvedBlits=0,draws=0,clears=0; };
 using UnresolvedFn=void(*)(const SftmVisualOp&);
 static constexpr int PlaneWidth=512;

 HdRenderer(const HdAssetPack& assets,int screenWidth,int screenHeight,int vramHeight=1024)
  :pack(assets),screenW(screenWidth),screenH(screenHeight),vramH(vramHeight),scale(int(assets.info().planeScale)){
  if(screenW<=0||screenH<=0||vramH<=0||vramH>4096)throw std::invalid_argument("Invalid renderer geometry");
  try{
   spriteProgram=link(replacementVertexShader,replacementFragmentShader);
   spriteLogical=glGetUniformLocation(spriteProgram,"logicalSize");
   glUseProgram(spriteProgram);glUniform1i(glGetUniformLocation(spriteProgram,"atlas"),0);
   compositeProgram=link(compositeVertexShader(),compositeFragmentShader());
   glUseProgram(compositeProgram);
   glUniform1i(glGetUniformLocation(compositeProgram,"plane0"),0);
   glUniform1i(glGetUniformLocation(compositeProgram,"plane1"),1);
   uScreen=glGetUniformLocation(compositeProgram,"screenSize");
   uPlane=glGetUniformLocation(compositeProgram,"planeSize");
   uOrigins=glGetUniformLocation(compositeProgram,"origins");
   uCount=glGetUniformLocation(compositeProgram,"planeCount");
   glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenVertexArrays(1,&emptyVao);
   glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
   glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(HdVertex),nullptr);
   glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(HdVertex),reinterpret_cast<void*>(2*sizeof(float)));
   glBindVertexArray(0);
   uploadPages();
   createPlanes();
   check("initialization");
  }catch(...){release();throw;}
 }
 HdRenderer(const HdRenderer&)=delete;
 HdRenderer& operator=(const HdRenderer&)=delete;
 ~HdRenderer(){release();}

 void setUnresolvedCallback(UnresolvedFn fn){onUnresolved=fn;}
 const Stats& stats() const{return counters;}

 // Replays one completed core frame. Frames must be applied in order and never skipped.
 void apply(const VisualQueue::Frame& frame){
  for(const auto& op:frame.ops){
   switch(op.kind){
   case SFTM_OP_BLIT: queueBlit(op);break;
   case SFTM_OP_CLEAR: flush();copyRows(op);break;
   case SFTM_OP_DISPLAY:
    flush();
    if(op.a4>0&&op.a4!=vramH&&op.a4<=4096){vramH=op.a4;createPlanes();}
    display={op.a0,op.a1,op.a2,op.a3,op.a5>1?2:1};
    break;
   default:break;
   }
  }
  flush();++counters.frames;
  check("frame");
 }

 // Scans the planes out into the viewport (x,y,w,h in window pixels).
 void present(int x,int y,int w,int h){
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  glViewport(x,y,w,h);
  glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
  glUseProgram(compositeProgram);
  glUniform2f(uScreen,float(screenW),float(screenH));
  glUniform2f(uPlane,float(PlaneWidth),float(vramH));
  glUniform4f(uOrigins,float(display.x0),float(display.y0),float(display.x1),float(display.y1));
  glUniform1i(uCount,display.planes);
  for(int i=0;i<2;++i){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,plane[i].tex);}
  glBindVertexArray(emptyVao);glDrawArrays(GL_TRIANGLES,0,3);glBindVertexArray(0);
  glActiveTexture(GL_TEXTURE0);
  check("present");
 }

 // Test and diagnostics access: reads back a plane pixel in original VRAM coordinates.
 void readPlanePixel(int planeIndex,int x,int y,unsigned char rgba[4]){
  glBindFramebuffer(GL_READ_FRAMEBUFFER,plane[planeIndex&1].fbo);
  glReadPixels(x*scale+scale/2,(vramH-1-y)*scale+scale/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
  glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
 }

private:
 struct Plane { GLuint tex=0,fbo=0; };
 struct Display { int x0=0,y0=0,x1=0,y1=0,planes=1; };
 struct BatchState {
  GLuint texture=0;bool blend=false;int plane=0;HdRect clip{0,0,0,0};
  bool operator==(const BatchState& o) const{
   return texture==o.texture&&blend==o.blend&&plane==o.plane&&clip.x0==o.clip.x0&&clip.y0==o.clip.y0&&clip.x1==o.clip.x1&&clip.y1==o.clip.y1;
  }
 };
 const HdAssetPack& pack;
 int screenW,screenH,vramH,scale;
 GLuint spriteProgram=0,compositeProgram=0,vao=0,vbo=0,emptyVao=0,scratchTex=0,scratchFbo=0;
 GLint spriteLogical=-1,uScreen=-1,uPlane=-1,uOrigins=-1,uCount=-1;
 Plane plane[2];
 std::vector<GLuint> pages;
 std::vector<HdVertex> batch;
 BatchState state;
 Display display;
 Stats counters;
 UnresolvedFn onUnresolved=nullptr;
 std::set<std::tuple<uint32_t,uint32_t,uint32_t,int32_t,int32_t>> reported;

 static const char* compositeVertexShader(){
  return R"GLSL(#version 300 es
precision highp float;
out vec2 uv;
void main(){
 vec2 p=vec2(float((gl_VertexID << 1) & 2),float(gl_VertexID & 2));
 uv=vec2(p.x,1.0-p.y);
 gl_Position=vec4(p*2.0-1.0,0.0,1.0);
}
)GLSL";
 }
 // uv has its origin at the top-left; plane textures store original row 0 at the top
 // of the framebuffer (the sprite vertex shader flips Y), i.e. at v = 1.
 static const char* compositeFragmentShader(){
  return R"GLSL(#version 300 es
precision highp float;
uniform sampler2D plane0;
uniform sampler2D plane1;
uniform vec2 screenSize;
uniform vec2 planeSize;
uniform vec4 origins;
uniform int planeCount;
in vec2 uv;
out vec4 fragColor;
vec4 fetchPlane(sampler2D t,vec2 origin){
 vec2 p=(uv*screenSize+origin)/planeSize;
 return texture(t,vec2(p.x,1.0-p.y));
}
void main(){
 vec4 c=fetchPlane(plane0,origins.xy);
 if(planeCount>1)c=c+fetchPlane(plane1,origins.zw)*(1.0-c.a);
 fragColor=vec4(c.rgb,1.0);
}
)GLSL";
 }

 static GLuint compile(GLenum type,const char* source){
  GLuint id=glCreateShader(type);glShaderSource(id,1,&source,nullptr);glCompileShader(id);
  GLint ok=0;glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
  if(!ok){char log[512]={};glGetShaderInfoLog(id,sizeof log,nullptr,log);glDeleteShader(id);throw std::runtime_error(std::string("Shader compile failed: ")+log);}
  return id;
 }
 static GLuint link(const char* vs,const char* fs){
  GLuint v=compile(GL_VERTEX_SHADER,vs),f=compile(GL_FRAGMENT_SHADER,fs);
  GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);
  glDeleteShader(v);glDeleteShader(f);
  GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);
  if(!ok){char log[512]={};glGetProgramInfoLog(p,sizeof log,nullptr,log);glDeleteProgram(p);throw std::runtime_error(std::string("Shader link failed: ")+log);}
  return p;
 }
 static void check(const char* where){
  if(glGetError()!=GL_NO_ERROR)throw std::runtime_error(std::string("OpenGL error during ")+where);
 }
 static void setupTexture(GLuint t,GLint filter,GLint wrap){
  glBindTexture(GL_TEXTURE_2D,t);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrap);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrap);
 }
 void uploadPages(){
  const unsigned n=pack.info().pageCount,size=pack.info().pageSize;
  pages.assign(n,0);
  glGenTextures(GLsizei(n),pages.data());
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  for(unsigned i=0;i<n;++i){
   auto data=pack.readPage(i); // one page in memory at a time
   setupTexture(pages[i],GL_LINEAR,GL_CLAMP_TO_EDGE);
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,GLsizei(size),GLsizei(size),0,GL_RGBA,GL_UNSIGNED_BYTE,data.data());
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT,4);
 }
 void destroyPlanes(){
  for(auto& p:plane){if(p.fbo)glDeleteFramebuffers(1,&p.fbo);if(p.tex)glDeleteTextures(1,&p.tex);p=Plane();}
  if(scratchFbo)glDeleteFramebuffers(1,&scratchFbo);
  if(scratchTex)glDeleteTextures(1,&scratchTex);
  scratchFbo=scratchTex=0;
 }
 static void makeTarget(GLuint& tex,GLuint& fbo,GLint wrap,int w,int h){
  glGenTextures(1,&tex);setupTexture(tex,GL_LINEAR,wrap);
  glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,w,h);
  glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("Plane framebuffer incomplete");
  glDisable(GL_SCISSOR_TEST);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
 }
 void createPlanes(){
  destroyPlanes();
  for(auto& p:plane)makeTarget(p.tex,p.fbo,GL_REPEAT,PlaneWidth*scale,vramH*scale);
  makeTarget(scratchTex,scratchFbo,GL_CLAMP_TO_EDGE,PlaneWidth*scale,scale); // one VRAM row
  glBindFramebuffer(GL_FRAMEBUFFER,0);
 }
 void release(){
  if(!pages.empty())glDeleteTextures(GLsizei(pages.size()),pages.data());
  pages.clear();
  destroyPlanes();
  if(vbo)glDeleteBuffers(1,&vbo);
  if(vao)glDeleteVertexArrays(1,&vao);
  if(emptyVao)glDeleteVertexArrays(1,&emptyVao);
  if(spriteProgram)glDeleteProgram(spriteProgram);
  if(compositeProgram)glDeleteProgram(compositeProgram);
  vbo=vao=emptyVao=spriteProgram=compositeProgram=0;
 }

 void queueBlit(const SftmVisualOp& op){
  const auto* region=pack.index().find(op);
  if(!region){
   ++counters.unresolvedBlits;
   if(onUnresolved&&reported.emplace(op.rom_address,op.palette_bank,op.encoding,op.width,op.height).second)onUnresolved(op);
   return;
  }
  ++counters.resolvedBlits;
  BatchState next{pages[region->page],(op.flags&SFTM_FLAG_TRANSPARENT)!=0,op.plane&1,hdClipRect(op,PlaneWidth,vramH)};
  if(!batch.empty()&&!(next==state))flush();
  state=next;
  const unsigned size=pack.info().pageSize;
  const HdBlit b=hdBlitGeometry(op,*region,size,size);
  for(const auto& o:hdWrapOffsets(b,PlaneWidth,vramH))
   for(auto v:b.vertices){v.x+=o.dx;v.y+=o.dy;batch.push_back(v);}
 }

 void flush(){
  if(batch.empty())return;
  const HdRect c=state.clip;
  if(c.x1>c.x0&&c.y1>c.y0){
   glBindFramebuffer(GL_FRAMEBUFFER,plane[state.plane].fbo);
   glViewport(0,0,PlaneWidth*scale,vramH*scale);
   glEnable(GL_SCISSOR_TEST);
   glScissor(c.x0*scale,(vramH-c.y1)*scale,(c.x1-c.x0)*scale,(c.y1-c.y0)*scale);
   glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
   if(state.blend){glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);}else glDisable(GL_BLEND);
   glUseProgram(spriteProgram);
   glUniform2f(spriteLogical,float(PlaneWidth),float(vramH));
   glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,state.texture);
   glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
   glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(batch.size()*sizeof(HdVertex)),batch.data(),GL_STREAM_DRAW);
   glDrawArrays(GL_TRIANGLES,0,GLsizei(batch.size()));
   glBindVertexArray(0);glDisable(GL_BLEND);glDisable(GL_SCISSOR_TEST);
   // Never leave a plane bound: the caller's next glClear must hit the screen, not the plane.
   glBindFramebuffer(GL_FRAMEBUFFER,0);
   ++counters.draws;
  }
  batch.clear();
 }

 // Shift-register copy: row y is replicated over the following rows (original semantics).
 void copyRows(const SftmVisualOp& op){
  const auto rows=hdClearRows(op,vramH);
  if(rows.empty())return;
  ++counters.clears;
  const int w=PlaneWidth*scale;
  const int source=((op.y%vramH)+vramH)%vramH;
  glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);
  auto band=[&](int row){return (vramH-1-row)*scale;}; // framebuffer rows grow upwards
  glBindFramebuffer(GL_READ_FRAMEBUFFER,plane[op.plane&1].fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER,scratchFbo);
  glBlitFramebuffer(0,band(source),w,band(source)+scale,0,0,w,scale,GL_COLOR_BUFFER_BIT,GL_NEAREST);
  glBindFramebuffer(GL_READ_FRAMEBUFFER,scratchFbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER,plane[op.plane&1].fbo);
  for(int row:rows)glBlitFramebuffer(0,0,w,scale,0,band(row),w,band(row)+scale,GL_COLOR_BUFFER_BIT,GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
 }
};
