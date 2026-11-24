#pragma once
#include <dlfcn.h>
#include <stdexcept>
#include <cstddef>
class CoreLoader {
 void* handle=nullptr;
 template<class T> T symbol(const char* name){
  auto p=dlsym(handle,name);if(!p)throw std::runtime_error(name);return reinterpret_cast<T>(p);
 }
public:
 using Video=void(*)(const void*,unsigned,unsigned,std::size_t);
 void (*set_video)(Video)=nullptr;
 void (*init)()=nullptr;
 void (*deinit)()=nullptr;
 void (*run)()=nullptr;
 unsigned (*api_version)()=nullptr;
 explicit CoreLoader(const char* path){
  handle=dlopen(path,RTLD_NOW|RTLD_LOCAL);
  if(!handle)throw std::runtime_error(dlerror());
  try{
   set_video=symbol<decltype(set_video)>("retro_set_video_refresh");
   init=symbol<decltype(init)>("retro_init");
   deinit=symbol<decltype(deinit)>("retro_deinit");
   run=symbol<decltype(run)>("retro_run");
   api_version=symbol<decltype(api_version)>("retro_api_version");
   if(api_version()!=1)throw std::runtime_error("Unsupported Libretro API");
  }catch(...){dlclose(handle);throw;}
 }
 CoreLoader(const CoreLoader&)=delete;
 CoreLoader& operator=(const CoreLoader&)=delete;
 ~CoreLoader(){if(handle)dlclose(handle);}
 // Caller must unload game and deinitialize core before destruction.
};
