#include "switch_graphics.hpp"
#include "switch_core.hpp"
#include "switch_input.hpp"
#include "switch_audio.hpp"
#include "runtime_session.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cstdarg>
static void trace(const char* msg){
 if(FILE* f=std::fopen("sdmc:/switch/sftm/sftm-debug.log","a")){std::fprintf(f,"%s\n",msg);std::fflush(f);std::fclose(f);}
}
static void coreLog(int level,const char* fmt,...){
 if(FILE* f=std::fopen("sdmc:/switch/sftm/sftm-debug.log","a")){std::fprintf(f,"core[%d]: ",level);va_list ap;va_start(ap,fmt);std::vfprintf(f,fmt,ap);va_end(ap);std::fflush(f);std::fclose(f);}
}
static bool environment(unsigned cmd,void* data){
 static const char* directory="sdmc:/switch/sftm";
 switch(cmd){
 case 27: if(data){struct LogCallback{void(*log)(int,const char*,...);};static_cast<LogCallback*>(data)->log=coreLog;return true;}return false;
 case 10:return data&&*static_cast<unsigned*>(data)==2;
 case 3:if(data)*static_cast<bool*>(data)=true;return data!=nullptr;
 case 9:case 31:if(data)*static_cast<const char**>(data)=directory;return data!=nullptr;
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
 bool first=true;
 while(appletMainLoop()){
 SwitchInput::poll();if(SwitchInput::exitRequested())break;
 session.step();if(first)trace("first core frame ok");glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
 renderer.draw(160,0,960,720,true);graphics.present();if(first){trace("first presentation ok");first=false;}
 }}
 renderer.release();glDeleteProgram(p);glDeleteShader(v);glDeleteShader(f);
 }catch(const std::exception& e){
 trace(e.what());
 consoleInit(nullptr);std::printf("SFTM: %s\nPress PLUS to exit.\n",e.what());
 SwitchInput::init();while(appletMainLoop()){SwitchInput::poll();if(SwitchInput::state(0,1,0,3))break;consoleUpdate(nullptr);}consoleExit(nullptr);return 1;
 }
 return 0;
}
