#include "switch_graphics.hpp"
#include "switch_core.hpp"
#include "switch_input.hpp"
#include "switch_audio.hpp"
#include "runtime_session.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <map>
#include <string>
static void trace(const char* msg){
 if(FILE* f=std::fopen("sdmc:/switch/sftm/sftm-debug.log","a")){std::fprintf(f,"%s\n",msg);std::fflush(f);std::fclose(f);}
}
static void coreLog(int level,const char* fmt,...){
 if(level<2)return;
 if(std::strstr(fmt,"set_irq_line") || std::strstr(fmt,"sound_data_") || std::strstr(fmt,"sound output write"))return;
 if(FILE* f=std::fopen("sdmc:/switch/sftm/sftm-debug.log","a")){std::fprintf(f,"core[%d]: ",level);va_list ap;va_start(ap,fmt);std::vfprintf(f,fmt,ap);va_end(ap);std::fflush(f);std::fclose(f);}
}
static bool environment(unsigned cmd,void* data){
 struct Variable { const char* key; const char* value; };
 static std::map<std::string,std::string> defaults;
 static const char* directory="sdmc:/switch/sftm";
 switch(cmd){
 case 27: if(data){struct LogCallback{void(*log)(int,const char*,...);};static_cast<LogCallback*>(data)->log=coreLog;return true;}return false;
 case 10:return data&&LibretroVideo::setFormat(*static_cast<unsigned*>(data));
 case 3:if(data)*static_cast<bool*>(data)=true;return data!=nullptr;
 case 9:case 31:if(data)*static_cast<const char**>(data)=directory;return data!=nullptr;
 case 16: {
  if(!data)return false;
  defaults.clear();
  for(auto* v=static_cast<const Variable*>(data);v->key;++v){
   if(!v->value)continue;
   std::string value=v->value;
   auto pos=value.find(';');
   if(pos==std::string::npos)continue;
   auto start=value.find_first_not_of(" ",pos+1);
   if(start==std::string::npos)continue;
   defaults.emplace(v->key,value.substr(start,value.find('|',start)-start));
  }
  return true;
 }
 case 15: {
  if(!data)return false;
  auto* v=static_cast<Variable*>(data);
  auto it=v->key?defaults.find(v->key):defaults.end();
  v->value=it==defaults.end()?nullptr:it->second.c_str();
  return it!=defaults.end();
 }
 case 52: if(data)*static_cast<unsigned*>(data)=0;return data!=nullptr;
 case 17:if(data)*static_cast<bool*>(data)=false;return data!=nullptr;
 default:return false;
 }
}
static GLuint shader(GLenum type,const char* path){
 std::ifstream file(path);if(!file)throw std::runtime_error(path);
 std::stringstream ss;ss<<file.rdbuf();auto source=ss.str();auto p=source.c_str();
 GLuint id=glCreateShader(type);glShaderSource(id,1,&p,nullptr);glCompileShader(id);
 GLint ok=0;glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
 if(!ok){glDeleteShader(id);throw std::runtime_error("Shader compile failed");}
 return id;
}

// Tiny GPU overlay using scissored clears, no texture allocation.
static void modeOverlay(bool hd){
 const char* label=hd?"HD REMASTER":"ORIGINAL";
 const char* alphabet="ADEGHILMNORST";
 const unsigned glyphs[]={0x1F8FE,0x1E63E,0x1F83F,0x1F6FE,0x11F71,0x1F11F,0x1083F,0x1BF71,0x1B771,0x1F631,0x1EFA9,0x1F07F,0x1F084};
 // Explicit 5x7 glyph rows.
 const char* rows[]={"01110""10001""10001""11111""10001""10001""10001","11110""10001""10001""10001""10001""10001""11110","11111""10000""10000""11110""10000""10000""11111","01110""10001""10000""10111""10001""10001""01110","10001""10001""10001""11111""10001""10001""10001","11111""00100""00100""00100""00100""00100""11111","10000""10000""10000""10000""10000""10000""11111","10001""11011""10101""10101""10001""10001""10001","10001""11001""10101""10011""10001""10001""10001","01110""10001""10001""10001""10001""10001""01110","11110""10001""10001""11110""10100""10010""10001","01111""10000""10000""01110""00001""00001""11110","11111""00100""00100""00100""00100""00100""00100"};
 int n=int(std::strlen(label)), x=640-n*12;
 glEnable(GL_SCISSOR_TEST);glScissor(x-12,650,n*24+24,48);glClearColor(0.04f,0.04f,0.04f,1);glClear(GL_COLOR_BUFFER_BIT);
 glClearColor(1,1,1,1);
 for(int k=0;k<n;++k){const char* at=std::strchr(alphabet,label[k]);if(!at)continue;const char* r=rows[at-alphabet];for(int y=0;y<7;++y)for(int c=0;c<5;++c)if(r[y*5+c]=='1'){glScissor(x+k*24+c*4,660+(6-y)*4,4,4);glClear(GL_COLOR_BUFFER_BIT);}}
 glDisable(GL_SCISSOR_TEST);
}

int main(){
 if(FILE* f=std::fopen("sdmc:/switch/sftm/sftm-debug.log","w")){std::fputs("SFTM diagnostic GLES3\n",f);std::fclose(f);}
 trace("main entered");
 try{
 trace("graphics begin");
 SwitchGraphics graphics;
 trace("graphics ok");
 SwitchInput::init();
 trace("input ok; audio begin");
 SwitchAudio audio;
 trace("audio ok; shaders begin");
 auto v=shader(GL_VERTEX_SHADER,"sdmc:/switch/sftm/hd.vert");
 auto f=shader(GL_FRAGMENT_SHADER,"sdmc:/switch/sftm/hd.frag");
 auto p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);
 GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);
 if(!ok)throw std::runtime_error("Shader link failed");
 trace("shaders ok; renderer begin");
 FrameRenderer renderer;renderer.init(p);
 trace("renderer ok; core binding begin");
 CoreLoader core(switchCoreApi());
 trace("core binding ok; session begin");
 CoreLoader::GameInfo game{"sdmc:/switch/sftm/sftm.zip",nullptr,0,nullptr};
 {RuntimeSession session(core,renderer,60.0,environment,SwitchAudio::sample,
 SwitchAudio::batch,SwitchInput::poll,SwitchInput::state,game);
 trace("session ok; first frame begin");
 bool first=true, hd=true, previousChord=false;
 unsigned perfFrames=0, overlayFrames=90;
 auto perfStart=std::chrono::steady_clock::now();
 double stepMs=0,presentMs=0;
 while(appletMainLoop()){
 SwitchInput::poll();if(SwitchInput::exitRequested())break;
 bool chord=SwitchInput::graphicsChord();if(chord&&!previousChord){hd=!hd;overlayFrames=90;trace(hd?"GRAPHICS HD":"GRAPHICS ORIGINAL");}previousChord=chord;
 auto t0=std::chrono::steady_clock::now();session.step();auto t1=std::chrono::steady_clock::now();if(first)trace("first core frame ok");glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
 renderer.draw(160,0,960,720,hd);if(overlayFrames){modeOverlay(hd);--overlayFrames;}graphics.present();auto t2=std::chrono::steady_clock::now();
 stepMs+=std::chrono::duration<double,std::milli>(t1-t0).count();presentMs+=std::chrono::duration<double,std::milli>(t2-t1).count();
 if(++perfFrames==300){double elapsed=std::chrono::duration<double>(t2-perfStart).count();char msg[160];std::snprintf(msg,sizeof(msg),"PERF fps=%.2f step_ms=%.2f render_ms=%.2f",300/elapsed,stepMs/300,presentMs/300);trace(msg);perfFrames=0;stepMs=presentMs=0;perfStart=t2;}
 if(first){trace("first presentation ok");first=false;}
 }}
 renderer.release();glDeleteProgram(p);glDeleteShader(v);glDeleteShader(f);
 }catch(const std::exception& e){
 trace(e.what());
 consoleInit(nullptr);std::printf("SFTM: %s\nPress PLUS to exit.\n",e.what());
 SwitchInput::init();while(appletMainLoop()){SwitchInput::poll();if(SwitchInput::state(0,1,0,3))break;consoleUpdate(nullptr);}consoleExit(nullptr);return 1;
 }
 return 0;
}
