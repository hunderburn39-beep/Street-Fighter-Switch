// Pixel-level test of HdRenderer against a real GL context (run under Xvfb in CI).
#include "../hd_renderer.hpp"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

static int failures=0;
#define CHECK(cond) do{if(!(cond)){std::cerr<<"FAIL line "<<__LINE__<<": "<<#cond<<"\n";++failures;}}while(0)

static void put(std::vector<uint8_t>& v,uint32_t x){for(int i=0;i<4;++i)v.push_back(uint8_t(x>>(8*i)));}
static void writeFile(const std::string& path,const std::vector<uint8_t>& d){
 FILE* f=std::fopen(path.c_str(),"wb");if(!f)std::exit(10);std::fwrite(d.data(),1,d.size(),f);std::fclose(f);
}
static void fill(std::vector<uint8_t>& page,int x,int y,int w,int h,uint8_t r,uint8_t g,uint8_t b,uint8_t a){
 for(int j=0;j<h;++j)for(int i=0;i<w;++i){auto* p=&page[((y+j)*16+x+i)*4];p[0]=r;p[1]=g;p[2]=b;p[3]=a;}
}
static SftmVisualOp blit(uint32_t rom,int x,int y,uint32_t flags=0){
 SftmVisualOp o={};o.kind=SFTM_OP_BLIT;o.rom_address=rom;o.width=o.height=2;o.x=x;o.y=y;o.flags=flags;
 o.src_xstep=o.src_ystep=o.dst_xstep=o.dst_ystep=0x100;return o;
}
struct Px{unsigned char c[4];};
static Px at(HdRenderer& r,int x,int y){Px p;r.readPlanePixel(0,x,y,p.c);return p;}
static bool is(Px p,int R,int G,int B,int A){
 auto close=[](int a,int b){return std::abs(a-b)<=2;};
 return close(p.c[0],R)&&close(p.c[1],G)&&close(p.c[2],B)&&close(p.c[3],A);
}

int main(){
 if(SDL_Init(SDL_INIT_VIDEO))return 1;
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
 auto w=SDL_CreateWindow("hd renderer",0,0,64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
 if(!w)return 2;
 auto ctx=SDL_GL_CreateContext(w);if(!ctx)return 3;
 glewExperimental=GL_TRUE;if(glewInit()!=GLEW_OK)return 4;while(glGetError()!=GL_NO_ERROR){}

 char tmpl[]="/tmp/sfhdglXXXXXX";char* d=mkdtemp(tmpl);if(!d)return 5;std::string dir=d;
 // Page 16x16: A red | S red-left blue-right | B green-left transparent-right (4x4 each, scale 2).
 std::vector<uint8_t> page(16*16*4,0);
 fill(page,1,1,4,4,255,0,0,255);
 fill(page,6,1,2,4,255,0,0,255);fill(page,8,1,2,4,0,0,255,255);
 fill(page,11,1,2,4,0,255,0,255);
 writeFile(dir+"/hd-0.rgba",page);
 std::vector<uint8_t> idx={'S','F','H','D'};
 put(idx,1);put(idx,16);put(idx,1);put(idx,3);put(idx,2);
 const uint32_t regions[3][4]={{1,1,4,4},{6,1,4,4},{11,1,4,4}};
 for(uint32_t i=0;i<3;++i){uint32_t e[11]={i+1,0,0,2,2,0,regions[i][0],regions[i][1],regions[i][2],regions[i][3],2};for(auto v:e)put(idx,v);}
 writeFile(dir+"/hd.idx",idx);

 try{
  HdAssetPack pack(dir);
  HdRenderer r(pack,64,64,64);
  static unsigned unresolvedCalls=0;
  r.setUnresolvedCallback([](const SftmVisualOp&){++unresolvedCalls;});
  VisualQueue::Frame f;f.number=0;
  auto& ops=f.ops;
  ops.push_back(blit(1,10,5));                                   // opaque red 2x2 at (10,5)
  ops.push_back(blit(2,30,5));                                   // red|blue, normal
  ops.push_back(blit(2,20,5,SFTM_FLAG_XFLIP));                   // mirrored: grows leftwards
  ops.push_back(blit(1,40,5));ops.push_back(blit(3,40,5,SFTM_FLAG_TRANSPARENT)); // green over red, right half transparent
  ops.push_back(blit(1,45,5));ops.push_back(blit(3,45,5));       // opaque blit overwrites incl. alpha 0
  {auto o=blit(1,50,30,SFTM_FLAG_CLIP);o.clip_left=0;o.clip_right=51;o.clip_top=0;o.clip_bottom=64;ops.push_back(o);}
  ops.push_back(blit(1,60,10));
  {SftmVisualOp c={};c.kind=SFTM_OP_CLEAR;c.x=0;c.y=10;c.height=4;ops.push_back(c);} // row 10 copied to 11..13
  ops.push_back(blit(2,511,20));                                 // wraps around the right edge
  ops.push_back(blit(99,5,50));                                  // not in the pack
  ops.push_back(blit(99,6,50));                                  // same key: reported once
  {SftmVisualOp s={};s.kind=SFTM_OP_DISPLAY;s.a4=64;s.a5=1;ops.push_back(s);}
  r.apply(f);

  CHECK(is(at(r,10,5),255,0,0,255)&&is(at(r,11,6),255,0,0,255));
  CHECK(is(at(r,12,5),0,0,0,0)&&is(at(r,9,5),0,0,0,0));
  CHECK(is(at(r,30,5),255,0,0,255)&&is(at(r,31,5),0,0,255,255));
  CHECK(is(at(r,20,5),255,0,0,255)&&is(at(r,19,5),0,0,255,255)&&is(at(r,21,5),0,0,0,0));
  CHECK(is(at(r,40,5),0,255,0,255)&&is(at(r,41,5),255,0,0,255));   // transparent blit keeps what is beneath
  CHECK(is(at(r,45,5),0,255,0,255)&&is(at(r,46,5),0,0,0,0));        // opaque blit replaces it
  CHECK(is(at(r,50,30),255,0,0,255)&&is(at(r,51,30),0,0,0,0));      // clip rectangle
  CHECK(is(at(r,60,10),255,0,0,255)&&is(at(r,60,12),255,0,0,255)&&is(at(r,60,13),255,0,0,255)&&is(at(r,60,14),0,0,0,0));
  CHECK(is(at(r,511,20),255,0,0,255)&&is(at(r,0,20),0,0,255,255));  // VRAM wrap
  CHECK(unresolvedCalls==1&&r.stats().unresolvedBlits==2);
  CHECK(r.stats().frames==1&&r.stats().resolvedBlits>=10);

  // Scan-out: window (64x64) shows plane pixel (10,5) red; shifting the origin moves it.
  glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
  r.present(0,0,64,64);
  unsigned char px[4]={};
  glReadPixels(10,63-5,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);
  CHECK(px[0]>250&&px[1]<5&&px[2]<5);
  VisualQueue::Frame g;g.number=1;{SftmVisualOp s={};s.kind=SFTM_OP_DISPLAY;s.a0=10;s.a4=64;s.a5=1;g.ops.push_back(s);}
  r.apply(g);r.present(0,0,64,64);
  glReadPixels(0,63-5,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);
  CHECK(px[0]>250&&px[1]<5&&px[2]<5);
  glReadPixels(10,63-5,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);
  CHECK(px[0]<5);
  CHECK(glGetError()==GL_NO_ERROR);
 }catch(const std::exception& e){std::cerr<<"exception: "<<e.what()<<"\n";failures+=100;}
 std::string cmd="rm -rf "+dir;if(std::system(cmd.c_str())!=0)++failures;
 SDL_GL_DeleteContext(ctx);SDL_DestroyWindow(w);SDL_Quit();
 if(failures)std::cerr<<failures<<" failure(s)\n";
 return failures?20:0;
}
