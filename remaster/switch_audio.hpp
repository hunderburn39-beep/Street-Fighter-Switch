#pragma once
#ifdef __SWITCH__
#include <switch.h>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
class SwitchAudio {
 static constexpr size_t frames=1024, bytes=frames*4;
 struct Slot { AudioOutBuffer buffer{}; void* memory=nullptr; bool busy=false; };
 std::array<Slot,4> slots{};
 std::array<int16_t,frames*2> pending{};
 size_t used=0;
 inline static SwitchAudio* instance=nullptr;
 bool initialized=false,started=false;
 void reclaim(){
  AudioOutBuffer* released=nullptr;u32 count=0;
  if(R_FAILED(audoutGetReleasedAudioOutBuffer(&released,&count)))return;
  while(released){for(auto& s:slots)if(&s.buffer==released)s.busy=false;released=released->next;}
 }
 void flush(){
  reclaim();
  for(auto& s:slots)if(!s.busy){
   std::memcpy(s.memory,pending.data(),bytes);
   s.buffer.next=nullptr;s.buffer.data=s.memory;s.buffer.buffer_size=bytes;
   s.buffer.data_size=bytes;s.buffer.data_offset=0;
   if(R_SUCCEEDED(audoutAppendAudioOutBuffer(&s.buffer)))s.busy=true;
   break;
  }
  used=0;
 }
 void push(int16_t l,int16_t r){
  pending[used++]=l;pending[used++]=r;
  if(used==pending.size())flush();
 }
public:
 SwitchAudio(){
  if(instance)throw std::runtime_error("Audio already active");
  try{
   if(R_FAILED(audoutInitialize()))throw std::runtime_error("AudioOut init failed");
   initialized=true;
   for(auto& s:slots){s.memory=std::aligned_alloc(4096,bytes);if(!s.memory)throw std::bad_alloc();}
   if(R_FAILED(audoutStartAudioOut()))throw std::runtime_error("AudioOut start failed");
   started=true;instance=this;
  }catch(...){close();throw;}
 }
 SwitchAudio(const SwitchAudio&)=delete;
 SwitchAudio& operator=(const SwitchAudio&)=delete;
 ~SwitchAudio(){close();}
 static void sample(int16_t l,int16_t r){if(instance)instance->push(l,r);}
 static size_t batch(const int16_t* data,size_t n){
  if(instance&&data)for(size_t i=0;i<n;++i)instance->push(data[2*i],data[2*i+1]);
  return n;
 }
private:
 void close(){
  if(instance==this)instance=nullptr;
  if(started)audoutStopAudioOut();
  if(initialized)audoutExit();
  for(auto& s:slots){std::free(s.memory);s.memory=nullptr;}
 }
};
#endif
