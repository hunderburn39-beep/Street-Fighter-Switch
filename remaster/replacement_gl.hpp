#pragma once
#include <GLES3/gl3.h>
#include "replacement_geometry.hpp"
#include <vector>
class ReplacementGL {
 GLuint vao=0,vbo=0;
public:
 ReplacementGL()=default;
 ReplacementGL(const ReplacementGL&)=delete;
 ReplacementGL& operator=(const ReplacementGL&)=delete;
 void init(){
 glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
 glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(ReplacementVertex),nullptr);
 glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(ReplacementVertex),reinterpret_cast<void*>(2*sizeof(float)));
 glBindVertexArray(0);
 }
 struct Page {GLuint texture;unsigned width,height;};
 // Caller supplies shader mapping logical pixel positions to clip space.
 void draw(const ReplacementFrame& frame,const std::vector<Page>& pages,GLuint program){
 glUseProgram(program);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
 glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glActiveTexture(GL_TEXTURE0);
 for(const auto& cmd:frame.commands){
 const auto& page=pages.at(cmd.region.page);
 auto vertices=replacementQuad(cmd,page.width,page.height);
 glBindTexture(GL_TEXTURE_2D,page.texture);
 glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices.data(),GL_STREAM_DRAW);
 glDrawArrays(GL_TRIANGLES,0,6);
 }
 glBindVertexArray(0);glDisable(GL_BLEND);
 }
 void release(){if(vbo)glDeleteBuffers(1,&vbo);if(vao)glDeleteVertexArrays(1,&vao);vbo=vao=0;}
};
