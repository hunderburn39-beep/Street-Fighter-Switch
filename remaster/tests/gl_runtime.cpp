#include "../libretro_video.hpp"
#include <SDL2/SDL.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
static GLuint shader(GLenum type,const char* path){
 std::ifstream f(path); if(!f) throw std::runtime_error(path);
 std::stringstream ss;ss<<f.rdbuf();auto s=ss.str();auto p=s.c_str();
 GLuint id=glCreateShader(type);glShaderSource(id,1,&p,nullptr);glCompileShader(id);
 GLint ok;glGetShaderiv(id,GL_COMPILE_STATUS,&ok);if(!ok)throw std::runtime_error(path);return id;
}
int main(){
 if(SDL_Init(SDL_INIT_VIDEO))return 1;
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
 SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
 auto w=SDL_CreateWindow("renderer test",0,0,64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
 if(!w)return 2;
 auto ctx=SDL_GL_CreateContext(w);if(!ctx)return 3;
 glewExperimental=GL_TRUE;if(glewInit()!=GLEW_OK)return 4;while(glGetError()!=GL_NO_ERROR){}
 try{
 auto v=shader(GL_VERTEX_SHADER,"remaster/shaders/hd.vert"),f=shader(GL_FRAGMENT_SHADER,"remaster/shaders/hd.frag");
 auto p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);
 GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok)return 5;
 FrameRenderer r;r.init(p);LibretroVideo::attach(r);LibretroVideo::setFormat(2);
 std::array<uint16_t,4> pixels={0xf800,0xf800,0xf800,0xf800};
 LibretroVideo::refresh(pixels.data(),2,2,4);
 for(bool hd:{false,true}){
 r.draw(0,0,64,64,hd);unsigned char rgba[4]={};
 glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
 if(rgba[0]<250||rgba[1]>5||rgba[2]>5||glGetError()!=GL_NO_ERROR)return 6;
 }
 if(LibretroVideo::hasError())return 7;
 LibretroVideo::detach();r.release();glDeleteProgram(p);glDeleteShader(v);glDeleteShader(f);
 }catch(const std::exception& e){std::cerr<<e.what();return 8;}
 SDL_GL_DeleteContext(ctx);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
