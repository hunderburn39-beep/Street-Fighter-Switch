#pragma once
#include "visual_bridge.h"
#include <vector>
#include <utility>
class VisualQueue {
public:
 struct Frame { uint64_t number=0; std::vector<SftmVisualOp> ops; };
 SftmVisualSink sink(){return {this,begin,transfer,end};}
 const Frame& latest() const {return ready;}
 bool available() const {return valid;}
private:
 Frame pending,ready;
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
  std::swap(q.pending,q.ready);q.valid=true;
 }
};
