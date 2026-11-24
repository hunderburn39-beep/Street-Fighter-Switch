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
 inline static std::vector<uint16_t> converted;
public:
 static void attach(FrameRenderer& r){renderer=&r;failed=false;format=0;frames=0;}
 static void detach(){renderer=nullptr;}
 static bool setFormat(unsigned f){if(f>2)return false;format=f;return true;}
 static bool hasError(){return failed;}
 // Convert all libretro software pixel formats to RGB565.
 static void refresh(const void* data,unsigned width,unsigned height,size_t pitch) noexcept {
  if(!renderer || !data) return; // null means repeat previous frame
  if(data==reinterpret_cast<const void*>(~uintptr_t(0)) ||
     width>unsigned(std::numeric_limits<int>::max()) ||
     height>unsigned(std::numeric_limits<int>::max()) ||
     pitch>size_t(std::numeric_limits<int>::max())){failed=true;return;}
  try {
   const size_t bpp=format==1?4:2;
   if(!width||!height||width>4096||height>4096||pitch<size_t(width)*bpp)throw std::runtime_error("Invalid video dimensions");
   if(format!=2){converted.resize(size_t(width)*height);for(unsigned y=0;y<height;++y){const auto* row=static_cast<const uint8_t*>(data)+size_t(y)*pitch;for(unsigned x=0;x<width;++x){uint32_t p=0;if(format==1){std::memcpy(&p,row+x*4,4);converted[size_t(y)*width+x]=uint16_t(((p>>8)&0xf800)|((p>>5)&0x07e0)|((p>>3)&31));}else{uint16_t q;std::memcpy(&q,row+x*2,2);converted[size_t(y)*width+x]=uint16_t(((q&0x7fe0)<<1)|((q&0x0200)>>4)|(q&31));}}}data=converted.data();pitch=size_t(width)*2;}
#ifdef __SWITCH__
   if(frames<5||frames%300==0){unsigned nonzero=0;for(unsigned y=0;y<height;++y){auto row=static_cast<const uint8_t*>(data)+size_t(y)*pitch;for(unsigned x=0;x<width;++x){uint16_t p;std::memcpy(&p,row+x*2,2);nonzero+=p!=0;}}if(FILE* f=std::fopen("sdmc:/switch/sftm/sftm-debug.log","a")){std::fprintf(f,"VIDEO frame=%u format=%u size=%ux%u nonzero=%u\n",frames,format,width,height,nonzero);std::fclose(f);}}
#endif
   ++frames;renderer->upload(data,int(width),int(height),int(pitch));
  }
  catch(...){failed=true;}
 }
};
