// Street Fighter: The Movie Remaster (Nintendo Switch).
// Remaster only: MAME2003-plus supplies logic, physics, AI, collisions and audio.
// Every pixel on screen comes from HdRenderer, which replays the core's blitter
// events with the redrawn HD textures. The native framebuffer is discarded.
#include "switch_graphics.hpp"
#include "switch_core.hpp"
#include "switch_input.hpp"
#include "switch_audio.hpp"
#include "runtime_session.hpp"
#include "visual_session.hpp"
#include "hd_asset_pack.hpp"
#include "hd_renderer.hpp"
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

static const char* const kDirectory="sdmc:/switch/sftm";
static const char* const kPackDirectory="sdmc:/switch/sftm/hd";
static const char* const kLog="sdmc:/switch/sftm/sftm-debug.log";
static const char* const kUnresolved="sdmc:/switch/sftm/unresolved.tsv";

static void trace(const char* msg){
 if(FILE* f=std::fopen(kLog,"a")){std::fprintf(f,"%s\n",msg);std::fflush(f);std::fclose(f);}
}
static void coreLog(int level,const char* fmt,...){
 if(level<2)return;
 if(std::strstr(fmt,"set_irq_line") || std::strstr(fmt,"sound_data_") || std::strstr(fmt,"sound output write"))return;
 if(FILE* f=std::fopen(kLog,"a")){std::fprintf(f,"core[%d]: ",level);va_list ap;va_start(ap,fmt);std::vfprintf(f,fmt,ap);va_end(ap);std::fflush(f);std::fclose(f);}
}
// The core still renders its own frame internally; it is intentionally ignored.
static void discardNativeFrame(const void*,unsigned,unsigned,std::size_t){}

// Sprites the core drew that have no HD replacement yet. They are drawn as nothing
// (never as the original graphics) and listed once so the asset team can cover them.
static void reportUnresolved(const SftmVisualOp& op){
 FILE* f=std::fopen(kUnresolved,"a");
 if(!f)return;
 if(std::ftell(f)==0)std::fputs("rom_address\tpalette_bank\tencoding\twidth\theight\n",f);
 std::fprintf(f,"%u\t%u\t%u\t%d\t%d\n",op.rom_address,op.palette_bank,op.encoding,op.width,op.height);
 std::fclose(f);
}

static bool environment(unsigned cmd,void* data){
 struct Variable { const char* key; const char* value; };
 static std::map<std::string,std::string> defaults;
 switch(cmd){
 case 27: if(data){struct LogCallback{void(*log)(int,const char*,...);};static_cast<LogCallback*>(data)->log=coreLog;return true;}return false;
 case 10:return data!=nullptr; // pixel format: accepted, the native frame is never displayed
 case 3:if(data)*static_cast<bool*>(data)=true;return data!=nullptr;
 case 9:case 31:if(data)*static_cast<const char**>(data)=kDirectory;return data!=nullptr;
 case 16: {
  if(!data)return false;
  defaults.clear();
  for(auto* v=static_cast<const Variable*>(data);v->key;++v){
   if(!v->value)continue;
   std::string value=v->value;
   auto pos=value.find(';');
   if(pos==std::string::npos)continue;
   auto start=value.find_first_not_of(" ",pos+1);
   if(start==std::string::npos)continue;
   defaults.emplace(v->key,value.substr(start,value.find('|',start)-start));
  }
  return true;
 }
 case 15: {
  if(!data)return false;
  auto* v=static_cast<Variable*>(data);
  auto it=v->key?defaults.find(v->key):defaults.end();
  v->value=it==defaults.end()?nullptr:it->second.c_str();
  return it!=defaults.end();
 }
 case 52: if(data)*static_cast<unsigned*>(data)=0;return data!=nullptr;
 case 17:if(data)*static_cast<bool*>(data)=false;return data!=nullptr;
 default:return false;
 }
}

int main(){
 if(FILE* f=std::fopen(kLog,"w")){std::fputs("SFTM Remaster GLES3\n",f);std::fclose(f);}
 if(FILE* f=std::fopen(kUnresolved,"w"))std::fclose(f);
 trace("main entered");
 try{
 SwitchGraphics graphics;
 trace("graphics ok");
 SwitchInput::init();
 SwitchAudio audio;
 trace("audio ok; HD pack begin");
 HdAssetPack pack(kPackDirectory); // fails early with a readable message if the pack is absent
 {char msg[128];std::snprintf(msg,sizeof msg,"HD pack: %zu sprites, %u page(s), plane scale %u",pack.info().entries,pack.info().pageCount,pack.info().planeScale);trace(msg);}
 CoreLoader core(switchCoreApi());
 VisualSession visual(sftm_visual_bind); // must be bound before the game starts drawing
 CoreLoader::GameInfo game{"sdmc:/switch/sftm/sftm.zip",nullptr,0,nullptr};
 {RuntimeSession session(core,discardNativeFrame,60.0,environment,SwitchAudio::sample,
  SwitchAudio::batch,SwitchInput::poll,SwitchInput::state,game);
  NativeAvInfo av{};retro_get_system_av_info(&av);
  const int screenW=av.base_width?int(av.base_width):384, screenH=av.base_height?int(av.base_height):240;
  {char msg[96];std::snprintf(msg,sizeof msg,"native display window %dx%d",screenW,screenH);trace(msg);}
  HdRenderer renderer(pack,screenW,screenH);
  renderer.setUnresolvedCallback(reportUnresolved);
  trace("renderer ok; first frame begin");
  std::vector<VisualQueue::Frame> frames;
  bool first=true;
  unsigned perfFrames=0;
  auto perfStart=std::chrono::steady_clock::now();
  double stepMs=0,renderMs=0;
  while(appletMainLoop()){
   SwitchInput::poll();if(SwitchInput::exitRequested())break;
   auto t0=std::chrono::steady_clock::now();
   session.step();
   auto t1=std::chrono::steady_clock::now();
   frames.clear();visual.frames().take(frames);
   for(const auto& frame:frames)renderer.apply(frame); // every frame, in order: planes are persistent
   glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
   renderer.present(160,0,960,720);
   graphics.present();
   auto t2=std::chrono::steady_clock::now();
   if(first){trace("first presentation ok");first=false;}
   stepMs+=std::chrono::duration<double,std::milli>(t1-t0).count();
   renderMs+=std::chrono::duration<double,std::milli>(t2-t1).count();
   if(++perfFrames==300){
    const double elapsed=std::chrono::duration<double>(t2-perfStart).count();
    const auto& s=renderer.stats();
    char msg[240];
    std::snprintf(msg,sizeof msg,"PERF fps=%.2f step_ms=%.2f render_ms=%.2f blits=%llu unresolved=%llu draws=%llu clears=%llu",
     300/elapsed,stepMs/300,renderMs/300,(unsigned long long)s.resolvedBlits,(unsigned long long)s.unresolvedBlits,
     (unsigned long long)s.draws,(unsigned long long)s.clears);
    trace(msg);
    perfFrames=0;stepMs=renderMs=0;perfStart=t2;
   }
  }
 }
 }catch(const std::exception& e){
  trace(e.what());
  consoleInit(nullptr);
  std::printf("SFTM Remaster: %s\n\nExpected files in sdmc:/switch/sftm/ :\n  sftm.zip  (your own ROM set)\n  hd/hd.idx and hd/hd-N.rgba  (HD texture pack)\n\nPress PLUS to exit.\n",e.what());
  SwitchInput::init();while(appletMainLoop()){SwitchInput::poll();if(SwitchInput::state(0,1,0,3))break;consoleUpdate(nullptr);}consoleExit(nullptr);return 1;
 }
 return 0;
}
