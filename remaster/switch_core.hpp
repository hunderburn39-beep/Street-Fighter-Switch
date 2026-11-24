#pragma once
#ifdef __SWITCH__
#include "core_loader.hpp"
extern "C" {
void retro_set_environment(CoreLoader::Environment);
void retro_set_audio_sample(CoreLoader::Audio);
void retro_set_audio_sample_batch(CoreLoader::AudioBatch);
void retro_set_input_poll(CoreLoader::InputPoll);
void retro_set_input_state(CoreLoader::InputState);
bool retro_load_game(const CoreLoader::GameInfo*);
void retro_unload_game();
void retro_set_video_refresh(CoreLoader::Video);
void retro_init();
void retro_deinit();
void retro_run();
unsigned retro_api_version();
}
inline CoreLoader::StaticApi switchCoreApi(){
 return {retro_set_environment,retro_set_audio_sample,
 retro_set_audio_sample_batch,retro_set_input_poll,
 retro_set_input_state,retro_load_game,retro_unload_game,
 retro_set_video_refresh,retro_init,retro_deinit,retro_run,retro_api_version};
}
#endif
