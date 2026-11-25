#pragma once
#include "visual_bridge.h"
#include <vector>
#include <deque>
#include <utility>
// Completed frames are queued until the renderer drains them: planes are
// persistent, so a skipped frame would corrupt every later frame.
class VisualQueue {
public:
 struct Frame { uint64_t number=0; std::vector<SftmVisualOp> ops; };
 SftmVisualSink sink(){return {this,begin,transfer,end};}
 const Frame& latest() const {return done.empty()?last:done.back();}
 bool available() const {return valid;}
 // Moves every completed, not yet consumed frame into out, oldest first.
 void take(std::vector<Frame>& out){
  for(auto& f:done)out.push_back(std::move(f));
  if(!done.empty()){last.number=out.back().number;last.ops=out.back().ops;}
  done.clear();
 }
private:
 Frame pending,last;
 std::deque<Frame> done;
 bool valid=false;
 static void begin(void* p,uint64_t n){
  auto& q=*static_cast<VisualQueue*>(p);
  q.pending.number=n;q.pending.ops.clear();
 }
 static void transfer(void* p,const SftmVisualOp* op){
  static_cast<VisualQueue*>(p)->pending.ops.push_back(*op);
 }
 static void end(void* p,uint64_t n){
  auto& q=*static_cast<VisualQueue*>(p);
  if(q.pending.number!=n)return;
  q.done.push_back(std::move(q.pending));q.pending=Frame();q.valid=true;
 }
};
