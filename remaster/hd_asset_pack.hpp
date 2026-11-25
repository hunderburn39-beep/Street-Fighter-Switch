#pragma once
// Loader for the HD replacement pack written by tools/pack_replacements.py:
//   hd.idx      binary index (see below)
//   hd-N.rgba   raw RGBA8 atlas pages, pageSize*pageSize*4 bytes each
// Pages are streamed one at a time so the CPU never holds the whole pack.
#include "replacement_index.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

struct HdPackInfo {
 unsigned pageSize=0,pageCount=0,planeScale=1;
 std::size_t entries=0;
};

class HdAssetPack {
 std::string dir;
 HdPackInfo meta;
 ReplacementIndex idx;
 static uint32_t u32(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
 static std::vector<uint8_t> slurp(const std::string& path,std::size_t limit){
  FILE* f=std::fopen(path.c_str(),"rb");
  if(!f)throw std::runtime_error("Cannot open "+path);
  std::vector<uint8_t> data;
  uint8_t buffer[65536];
  std::size_t n;
  while((n=std::fread(buffer,1,sizeof buffer,f))>0){
   if(data.size()+n>limit){std::fclose(f);throw std::runtime_error("File too large: "+path);}
   data.insert(data.end(),buffer,buffer+n);
  }
  std::fclose(f);
  return data;
 }
public:
 explicit HdAssetPack(const std::string& directory):dir(directory){
  if(!dir.empty()&&dir.back()!='/')dir.push_back('/');
  auto raw=slurp(dir+"hd.idx",64u<<20);
  if(raw.size()<24||std::memcmp(raw.data(),"SFHD",4)!=0)throw std::runtime_error("Invalid HD index header");
  if(u32(&raw[4])!=1)throw std::runtime_error("Unsupported HD index version");
  meta.pageSize=u32(&raw[8]);meta.pageCount=u32(&raw[12]);
  const uint32_t count=u32(&raw[16]);meta.planeScale=u32(&raw[20]);
  if(meta.pageSize<8||meta.pageSize>8192||!meta.pageCount||meta.pageCount>256||meta.planeScale<1||meta.planeScale>8)
   throw std::runtime_error("Invalid HD index parameters");
  if(raw.size()!=24+std::size_t(count)*44)throw std::runtime_error("Truncated HD index");
  for(uint32_t i=0;i<count;++i){
   const uint8_t* e=&raw[24+std::size_t(i)*44];
   SftmVisualOp op={};
   op.rom_address=u32(e);op.palette_bank=u32(e+4);op.encoding=u32(e+8);
   op.width=int32_t(u32(e+12));op.height=int32_t(u32(e+16));
   ReplacementRegion r{u32(e+20),u32(e+24),u32(e+28),u32(e+32),u32(e+36)};
   if(r.page>=meta.pageCount||r.width>meta.pageSize||r.height>meta.pageSize||
      r.x>meta.pageSize-r.width||r.y>meta.pageSize-r.height)throw std::runtime_error("HD region outside page");
   if(!idx.insert(op,r))throw std::runtime_error("Duplicate or invalid HD sprite entry");
  }
  meta.entries=count;
 }
 const HdPackInfo& info() const{return meta;}
 const ReplacementIndex& index() const{return idx;}
 // Reads page `n` and converts it to premultiplied alpha (the plane blend assumes it).
 std::vector<uint8_t> readPage(unsigned n) const{
  if(n>=meta.pageCount)throw std::out_of_range("HD page");
  const std::size_t bytes=std::size_t(meta.pageSize)*meta.pageSize*4;
  auto data=slurp(dir+"hd-"+std::to_string(n)+".rgba",bytes);
  if(data.size()!=bytes)throw std::runtime_error("HD page has wrong size");
  premultiply(data.data(),data.size()/4);
  return data;
 }
 static void premultiply(uint8_t* px,std::size_t count){
  for(std::size_t i=0;i<count;++i,px+=4){
   const unsigned a=px[3];
   if(a==255)continue;
   px[0]=uint8_t((px[0]*a+127)/255);px[1]=uint8_t((px[1]*a+127)/255);px[2]=uint8_t((px[2]*a+127)/255);
  }
 }
};
