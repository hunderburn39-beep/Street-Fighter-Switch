#pragma once
#include "core_loader.hpp"
#include "libretro_video.hpp"
#include "frame_clock.hpp"
#include <thread>
#include <cmath>
#include <cstdio>
#ifdef __SWITCH__
#include "switch_audio.hpp"
struct NativeAvInfo { unsigned base_width,base_height,max_width,max_height; float aspect_ratio; double fps,sample_rate; };
extern "C" void retro_get_system_av_info(NativeAvInfo*);
#endif
class RuntimeSession {
 CoreLoader& core;
 FrameClock clock;
 bool initialized=false, loaded=false;
 inline static bool active=false;
public:
 // Legacy path used by the desktop tests: the native frame is uploaded by LibretroVideo.
 RuntimeSession(CoreLoader& c, FrameRenderer& renderer, double fps,
 CoreLoader::Environment env, CoreLoader::Audio audio,
 CoreLoader::AudioBatch batch, CoreLoader::InputPoll poll,
 CoreLoader::InputState input, const CoreLoader::GameInfo& game)
 :RuntimeSession(c,attachLegacyVideo(renderer),fps,env,audio,batch,poll,input,game){}
 // Remaster path: the caller owns the video callback and must not present the native frame.
 RuntimeSession(CoreLoader& c, CoreLoader::Video video, double fps,
 CoreLoader::Environment env, CoreLoader::Audio audio,
 CoreLoader::AudioBatch batch, CoreLoader::InputPoll poll,
 CoreLoader::InputState input, const CoreLoader::GameInfo& game)
 :core(c),clock(fps){
  if(active || !video || !env || !audio || !batch || !poll || !input)
   throw std::runtime_error("Invalid runtime callbacks");
  active=true;
  try {
   core.set_environment(env);
   core.set_video(video);
   core.set_audio(audio);core.set_audio_batch(batch);
   core.set_input_poll(poll);core.set_input_state(input);
   core.init();initialized=true;
   if(!core.load_game(&game))throw std::runtime_error("Game load failed");
   loaded=true;
#ifdef __SWITCH__
   NativeAvInfo av{};retro_get_system_av_info(&av);
   clock=FrameClock(av.fps);
   const bool validRate=std::isfinite(av.sample_rate)&&av.sample_rate>=1000&&av.sample_rate<=384000;
   const double sourceRate=validRate?av.sample_rate:48000.0;
   if(FILE* log=std::fopen("sdmc:/switch/sftm/sftm-debug.log","a")){
    std::fprintf(log,"AV: fps=%.6f reported audio=%.6f effective audio=%.6f fallback=%d\n",av.fps,av.sample_rate,sourceRate,validRate?0:1);
    std::fclose(log);
   }
   SwitchAudio::setSourceRate(sourceRate);
#endif
   clock.reset();
  } catch(...) { close();throw; }
 }
 RuntimeSession(const RuntimeSession&)=delete;
 RuntimeSession& operator=(const RuntimeSession&)=delete;
 ~RuntimeSession(){close();}
 void step(){
  if(!loaded)throw std::runtime_error("No game loaded");
  std::this_thread::sleep_until(clock.next());
  core.run();clock.advance();
  if(LibretroVideo::hasError())throw std::runtime_error("Video callback failed");
 }
private:
 static CoreLoader::Video attachLegacyVideo(FrameRenderer& renderer){
  LibretroVideo::attach(renderer);return LibretroVideo::refresh;
 }
 void close(){
  if(loaded){core.unload_game();loaded=false;}
  if(initialized){core.deinit();initialized=false;}
  LibretroVideo::detach();active=false;
 }
};
