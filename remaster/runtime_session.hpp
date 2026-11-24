#pragma once
#include "core_loader.hpp"
#include "libretro_video.hpp"
#include "frame_clock.hpp"
#include <thread>
class RuntimeSession {
 CoreLoader& core;
 FrameClock clock;
 bool initialized=false, loaded=false;
 inline static bool active=false;
public:
 RuntimeSession(CoreLoader& c, FrameRenderer& renderer, double fps,
 CoreLoader::Environment env, CoreLoader::Audio audio,
 CoreLoader::AudioBatch batch, CoreLoader::InputPoll poll,
 CoreLoader::InputState input, const CoreLoader::GameInfo& game)
 :core(c),clock(fps){
  if(active || !env || !audio || !batch || !poll || !input)
   throw std::runtime_error("Invalid runtime callbacks");
  active=true;
  LibretroVideo::attach(renderer);
  try {
   core.set_environment(env);
   core.set_video(LibretroVideo::refresh);
   core.set_audio(audio);core.set_audio_batch(batch);
   core.set_input_poll(poll);core.set_input_state(input);
   core.init();initialized=true;
   if(!core.load_game(&game))throw std::runtime_error("Game load failed");
   loaded=true;clock.reset();
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
 void close(){
  if(loaded){core.unload_game();loaded=false;}
  if(initialized){core.deinit();initialized=false;}
  LibretroVideo::detach();active=false;
 }
};
