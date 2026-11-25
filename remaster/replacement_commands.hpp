#pragma once
#include "replacement_index.hpp"
#include "visual_queue.hpp"
#include <vector>
struct ReplacementCommand { SftmVisualOp operation; ReplacementRegion region; };
struct ReplacementFrame {
 uint64_t number=0;
 std::vector<ReplacementCommand> commands;
 std::vector<SftmVisualOp> unresolved;
};
inline ReplacementFrame resolveFrame(const VisualQueue::Frame& frame,const ReplacementIndex& index){
 ReplacementFrame result;result.number=frame.number;
 result.commands.reserve(frame.ops.size());
 for(const auto& op:frame.ops){
  const auto* region=index.find(op);
  if(region)result.commands.push_back({op,*region});
  else result.unresolved.push_back(op);
 }
 return result;
}
