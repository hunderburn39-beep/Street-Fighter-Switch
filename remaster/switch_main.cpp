#include "switch_graphics.hpp"
#include "switch_core.hpp"
#include "switch_input.hpp"
#include "switch_audio.hpp"
#include "runtime_session.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
static bool environment(unsigned cmd,void* data){
 static const char* directory="sdmc:/switch/sftm";
 switch(cmd){
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
 try{
 SwitchGraphics graphics;
 SwitchInput::init();
 SwitchAudio audio;
 auto v=shader(GL_VERTEX_SHADER,"sdmc:/switch/sftm/hd.vert");
 auto f=shader(GL_FRAGMENT_SHADER,"sdmc:/switch/sftm/hd.frag");
 auto p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);
 GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);
 if(!ok)throw std::runtime_error("Shader link failed");
 FrameRenderer renderer;renderer.init(p);
 CoreLoader core(switchCoreApi());
 CoreLoader::GameInfo game{"sdmc:/switch/sftm/sftm.zip",nullptr,0,nullptr};
 {RuntimeSession session(core,renderer,60.0,environment,SwitchAudio::sample,
 SwitchAudio::batch,SwitchInput::poll,SwitchInput::state,game);
 while(appletMainLoop()){
 SwitchInput::poll();if(SwitchInput::exitRequested())break;
 session.step();glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
 renderer.draw(160,0,960,720,true);graphics.present();
 }}
 renderer.release();glDeleteProgram(p);glDeleteShader(v);glDeleteShader(f);
 }catch(const std::exception& e){
 consoleInit(nullptr);std::printf("SFTM: %s\nPress PLUS to exit.\n",e.what());
 SwitchInput::init();while(appletMainLoop()){SwitchInput::poll();if(SwitchInput::state(0,1,0,3))break;consoleUpdate(nullptr);}consoleExit(nullptr);return 1;
 }
 return 0;
}
