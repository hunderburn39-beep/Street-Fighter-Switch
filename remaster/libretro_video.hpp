#pragma once
#include "frame_renderer.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
// Single frontend instance, callbacks on the GL context thread.
class LibretroVideo {
 inline static FrameRenderer* renderer=nullptr;
 inline static bool failed=false;
public:
 static void attach(FrameRenderer& r){renderer=&r;failed=false;}
 static void detach(){renderer=nullptr;}
 static bool hasError(){return failed;}
 // Frontend must negotiate RETRO_PIXEL_FORMAT_RGB565 before using this callback.
 static void refresh(const void* data,unsigned width,unsigned height,size_t pitch) noexcept {
  if(!renderer || !data) return; // null means repeat previous frame
  if(data==reinterpret_cast<const void*>(~uintptr_t(0)) ||
     width>unsigned(std::numeric_limits<int>::max()) ||
     height>unsigned(std::numeric_limits<int>::max()) ||
     pitch>size_t(std::numeric_limits<int>::max())){failed=true;return;}
  try {renderer->upload(data,int(width),int(height),int(pitch));}
  catch(...){failed=true;}
 }
};
