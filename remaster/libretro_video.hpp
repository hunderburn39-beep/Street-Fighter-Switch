#pragma once
#include "frame_renderer.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
#include <cstring>
#include <cstdio>
// Single frontend instance, callbacks on the GL context thread.
class LibretroVideo {
 inline static FrameRenderer* renderer=nullptr;
 inline static bool failed=false;
 inline static unsigned format=0, frames=0;
 inline static std::vector<uint8_t> converted;
public:
 static void attach(FrameRenderer& r){renderer=&r;failed=false;format=0;frames=0;}
 static void detach(){renderer=nullptr;}
 static bool setFormat(unsigned f){if(f>2)return false;format=f;return true;}
 static bool hasError(){return failed;}
 // Convert all libretro software pixel formats to RGBA8.
 static void refresh(const void* data,unsigned width,unsigned height,size_t pitch) noexcept {
  if(!renderer || !data) return; // null means repeat previous frame
  if(data==reinterpret_cast<const void*>(~uintptr_t(0)) ||
     width>unsigned(std::numeric_limits<int>::max()) ||
     height>unsigned(std::numeric_limits<int>::max()) ||
     pitch>size_t(std::numeric_limits<int>::max())){failed=true;return;}
  try {
   const size_t bpp=format==1?4:2;
   if(!width||!height||width>4096||height>4096||pitch<size_t(width)*bpp)throw std::runtime_error("Invalid video dimensions");
#ifdef __SWITCH__
   if(format==2 && pitch%2==0){
    ++frames;renderer->upload(data,int(width),int(height),int(pitch),false);return;
   }
#endif
   converted.resize(size_t(width)*height*4);
   unsigned nonzero=0;
   for(unsigned y=0;y<height;++y){
    const auto* row=static_cast<const uint8_t*>(data)+size_t(y)*pitch;
    for(unsigned x=0;x<width;++x){
     auto* out=&converted[(size_t(y)*width+x)*4];
     if(format==1){uint32_t p;std::memcpy(&p,row+x*4,4);out[0]=p>>16;out[1]=p>>8;out[2]=p;}
     else{uint16_t p;std::memcpy(&p,row+x*2,2);unsigned red=(p>>(format==2?11:10))&31,green=(p>>5)&(format==2?63:31),blue=p&31;out[0]=(red<<3)|(red>>2);out[1]=format==2?((green<<2)|(green>>4)):((green<<3)|(green>>2));out[2]=(blue<<3)|(blue>>2);}
     out[3]=255;nonzero+=(out[0]|out[1]|out[2])!=0;
    }
   }
#ifdef __SWITCH__
   if(frames<5||frames%300==0){if(FILE* f=std::fopen("sdmc:/switch/sftm/sftm-debug.log","a")){std::fprintf(f,"VIDEO frame=%u format=%u size=%ux%u nonzero=%u\n",frames,format,width,height,nonzero);std::fclose(f);}}
#endif
   ++frames;renderer->upload(converted.data(),int(width),int(height),int(width*4),true);
  }
  catch(...){failed=true;}
 }
};
