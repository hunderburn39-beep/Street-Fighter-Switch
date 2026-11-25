#pragma once
#include "replacement_commands.hpp"
#include <array>
#include <stdexcept>
struct ReplacementVertex {float x,y,u,v;};
inline std::array<ReplacementVertex,6> replacementQuad(const ReplacementCommand& c,unsigned atlasW,unsigned atlasH){
 const auto& r=c.region;const auto& o=c.operation;
 if(!atlasW||!atlasH||r.x>atlasW||r.y>atlasH||r.width>atlasW-r.x||r.height>atlasH-r.y)throw std::invalid_argument("Invalid atlas region");
 float l=o.x,t=o.y,rr=l+o.width,b=t+o.height;
 float u=float(r.x)/atlasW,v=float(r.y)/atlasH,u1=float(r.x+r.width)/atlasW,v1=float(r.y+r.height)/atlasH;
 return {{{l,t,u,v},{rr,t,u1,v},{rr,b,u1,v1},{l,t,u,v},{rr,b,u1,v1},{l,b,u,v1}}};
}
