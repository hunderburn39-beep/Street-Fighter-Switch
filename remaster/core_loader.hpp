#pragma once
#include <dlfcn.h>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
class CoreLoader {
 void* handle=nullptr;
 template<class T> T symbol(const char* name){
  auto p=dlsym(handle,name);if(!p)throw std::runtime_error(name);return reinterpret_cast<T>(p);
 }
public:
 struct GameInfo {const char* path;const void* data;std::size_t size;const char* meta;};
 using Environment=bool(*)(unsigned,void*);
 using Audio=void(*)(int16_t,int16_t);
 using AudioBatch=std::size_t(*)(const int16_t*,std::size_t);
 using InputPoll=void(*)();
 using InputState=int16_t(*)(unsigned,unsigned,unsigned,unsigned);
 void (*set_environment)(Environment)=nullptr;
 void (*set_audio)(Audio)=nullptr;
 void (*set_audio_batch)(AudioBatch)=nullptr;
 void (*set_input_poll)(InputPoll)=nullptr;
 void (*set_input_state)(InputState)=nullptr;
 bool (*load_game)(const GameInfo*)=nullptr;
 void (*unload_game)()=nullptr;
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
   set_environment=symbol<decltype(set_environment)>("retro_set_environment");
   set_audio=symbol<decltype(set_audio)>("retro_set_audio_sample");
   set_audio_batch=symbol<decltype(set_audio_batch)>("retro_set_audio_sample_batch");
   set_input_poll=symbol<decltype(set_input_poll)>("retro_set_input_poll");
   set_input_state=symbol<decltype(set_input_state)>("retro_set_input_state");
   load_game=symbol<decltype(load_game)>("retro_load_game");
   unload_game=symbol<decltype(unload_game)>("retro_unload_game");
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
