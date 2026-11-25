#pragma once
#include "replacement_gl.hpp"
#include <png.h>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
class ReplacementAtlas {
public:
 ReplacementIndex index;
 std::vector<ReplacementGL::Page> pages;
 ReplacementAtlas()=default;
 ReplacementAtlas(const ReplacementAtlas&)=delete;
 ReplacementAtlas& operator=(const ReplacementAtlas&)=delete;
 ~ReplacementAtlas(){for(auto& p:pages)glDeleteTextures(1,&p.texture);}
 void load(const std::string& dir){
  if(!pages.empty())throw std::runtime_error("Atlas already loaded");
  std::ifstream in(dir+"/atlas.tsv");
  std::string magic;unsigned count=0,size=0;
  if(!(in>>magic>>count>>size)||magic!="SFTM_ATLAS_V1"||!count||count>64||!size||size>2048)
   throw std::runtime_error("Invalid or missing HD atlas");
  std::string line;std::getline(in,line);
  unsigned entries=0;
  while(std::getline(in,line)){
   if(line.empty())continue;
   std::istringstream row(line);SftmVisualOp op{};ReplacementRegion r{};std::string extra;
   if(!(row>>op.rom_address>>op.palette_bank>>op.encoding>>op.width>>op.height>>r.page>>r.x>>r.y>>r.width>>r.height)||(row>>extra)||r.page>=count||r.x>=size||r.y>=size||r.width>size-r.x||r.height>size-r.y||!index.insert(op,r))
    throw std::runtime_error("Invalid HD atlas entry");
   ++entries;
  }
  if(!entries)throw std::runtime_error("Empty HD atlas");
  pages.reserve(count);
  for(unsigned i=0;i<count;++i){
   png_image img{};img.version=PNG_IMAGE_VERSION;
   auto path=dir+"/atlas-"+std::to_string(i)+".png";
   if(!png_image_begin_read_from_file(&img,path.c_str())){png_image_free(&img);throw std::runtime_error("Missing atlas PNG");}
   if(img.width!=size||img.height!=size){png_image_free(&img);throw std::runtime_error("Atlas dimensions mismatch");}
   img.format=PNG_FORMAT_RGBA;
   std::vector<unsigned char> pixels(PNG_IMAGE_SIZE(img));
   if(!png_image_finish_read(&img,nullptr,pixels.data(),0,nullptr)){png_image_free(&img);throw std::runtime_error("Atlas PNG decode failed");}
   png_image_free(&img);
   GLuint tex=0;glGenTextures(1,&tex);pages.push_back({tex,size,size});
   glBindTexture(GL_TEXTURE_2D,tex);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,size,size,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
   if(!tex||glGetError()!=GL_NO_ERROR)throw std::runtime_error("Atlas GPU upload failed");
  }
 }
};
