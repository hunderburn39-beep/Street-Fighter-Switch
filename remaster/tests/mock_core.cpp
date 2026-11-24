#include <cstdint>
#include <cstddef>
extern "C" {
struct GameInfo {const char* path;const void* data;std::size_t size;const char* meta;};
static bool (*env)(unsigned,void*);
static void (*video)(const void*,unsigned,unsigned,std::size_t);
static void (*audio)(int16_t,int16_t);
static std::size_t (*batch)(const int16_t*,std::size_t);
static void (*poll)();
static int16_t (*input)(unsigned,unsigned,unsigned,unsigned);
static bool loaded=false;
unsigned retro_api_version(){return 1;}
void retro_set_environment(decltype(env) p){env=p;}
void retro_set_video_refresh(decltype(video) p){video=p;}
void retro_set_audio_sample(decltype(audio) p){audio=p;}
void retro_set_audio_sample_batch(decltype(batch) p){batch=p;}
void retro_set_input_poll(decltype(poll) p){poll=p;}
void retro_set_input_state(decltype(input) p){input=p;}
void retro_init(){}
void retro_deinit(){}
bool retro_load_game(const GameInfo* g){unsigned format=2;return loaded=g&&env(10,&format);}
void retro_unload_game(){loaded=false;}
void retro_run(){
 if(!loaded)return;
 poll();input(0,1,0,0);
 const uint16_t pixels[4]={0xf800,0xf800,0xf800,0xf800};
 video(pixels,2,2,4);
 audio(0,0);const int16_t samples[2]={0,0};batch(samples,1);
}
}
