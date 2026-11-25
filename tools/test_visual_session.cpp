#include "../remaster/visual_session.hpp"
#include <cassert>
int main(){
 bool rejected=false;
 try { VisualSession invalid(nullptr); } catch(const std::invalid_argument&) { rejected=true; }
 assert(rejected);
 {
  VisualSession session(sftm_visual_bind);
  SftmVisualOp op={};op.rom_address=42;
  sftm_visual_begin();sftm_visual_transfer(&op);sftm_visual_end();
  assert(session.frames().available());
  assert(session.frames().latest().ops.at(0).rom_address==42);
 }
 sftm_visual_begin();sftm_visual_end();
}
