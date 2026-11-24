#pragma once
#ifdef __SWITCH__
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#else
#include <GL/glew.h>
#endif
#include <stdexcept>
class FrameRenderer {
 GLuint tex=0, vao=0, program=0;
 GLint strength=-1;
 int w=0,h=0;
public:
 FrameRenderer()=default;
 FrameRenderer(const FrameRenderer&)=delete;
 FrameRenderer& operator=(const FrameRenderer&)=delete;
 // Call with a current GL context and a successfully linked HD program.
 void init(GLuint linkedProgram){
  if(!linkedProgram || tex) throw std::runtime_error("Invalid renderer initialization");
  program=linkedProgram;
  glGenTextures(1,&tex); glGenVertexArrays(1,&vao);
  glBindTexture(GL_TEXTURE_2D,tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glUseProgram(program);
  glUniform1i(glGetUniformLocation(program,"sourceFrame"),0);
  strength=glGetUniformLocation(program,"strength");
 }
 // RGB565 native-endian input; pitch is measured in bytes.
 void upload(const void* pixels,int width,int height,int pitch){
  if(!tex || !pixels || width<=0 || height<=0 || pitch/2<width || pitch%2) throw std::runtime_error("Invalid frame");
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,pitch/2);
  if(w!=width || h!=height){
   w=width;h=height;
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,pixels);
  }else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,pixels);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_ALIGNMENT,4);
 }
 void draw(int x,int y,int width,int height,bool hd){
  if(!tex || !w || !h) return;
  glViewport(x,y,width,height);
  glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
  glUseProgram(program);glUniform1f(strength,hd?1.0f:0.0f);
  glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,tex);
  glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);
 }
 // Release before destroying the GL context. Program ownership stays with caller.
 void release(){glDeleteTextures(1,&tex);glDeleteVertexArrays(1,&vao);tex=vao=program=0;w=h=0;}
};
