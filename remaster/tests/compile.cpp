#include "../libretro_video.hpp"
#include <type_traits>
static_assert(!std::is_copy_constructible<FrameRenderer>::value);
int main(){FrameRenderer renderer; LibretroVideo::attach(renderer);LibretroVideo::refresh(nullptr,384,255,768);LibretroVideo::detach();return LibretroVideo::hasError()?1:0;}
