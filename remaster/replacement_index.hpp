#pragma once
#include "visual_bridge.h"
#include <map>
#include <tuple>
#include <cstdint>
struct ReplacementRegion {
 uint32_t page,x,y,width,height;
};
class ReplacementIndex {
 using Key=std::tuple<uint32_t,uint32_t,uint32_t,int32_t,int32_t>;
 std::map<Key,ReplacementRegion> regions;
 static Key key(const SftmVisualOp& op){
  return Key(op.rom_address,op.palette_bank,op.encoding,op.width,op.height);
 }
public:
 bool insert(const SftmVisualOp& op,const ReplacementRegion& region){
  if(op.width<=0||op.height<=0||!region.width||!region.height)return false;
  return regions.emplace(key(op),region).second;
 }
 const ReplacementRegion* find(const SftmVisualOp& op) const{
  auto it=regions.find(key(op));
  return it==regions.end()?nullptr:&it->second;
 }
};
