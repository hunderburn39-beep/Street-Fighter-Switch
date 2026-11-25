#pragma once
// Pure geometry of the ITech32 blitter, mirrored from the core's draw_raw/draw_rle:
// 8.8 fixed point source/destination steps, X/Y flipping (the start pixel is the
// edge the sprite grows away from), skew registers, clip rectangle and VRAM wrap.
#include "replacement_index.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

struct HdVertex { float x,y,u,v; };
struct HdRect { int x0,y0,x1,y1; }; // [x0,x1) x [y0,y1) in VRAM pixels
struct HdBlit {
 std::array<HdVertex,6> vertices;
 float minX,minY,maxX,maxY; // VRAM pixel bounds of the quad
};

inline float hdFixed(int32_t step,bool abs_value=true){
 float v=step/256.0f;
 if(v==0.0f)return 1.0f;
 return abs_value?std::fabs(v):v;
}

// Builds the textured quad for one blit. Positions are VRAM pixels (not wrapped).
inline HdBlit hdBlitGeometry(const SftmVisualOp& op,const ReplacementRegion& r,unsigned atlasW,unsigned atlasH){
 if(op.width<=0||op.height<=0||!atlasW||!atlasH||r.x>atlasW||r.y>atlasH||r.width>atlasW-r.x||r.height>atlasH-r.y)
  throw std::invalid_argument("Invalid blit or atlas region");
 const bool xflip=op.flags&SFTM_FLAG_XFLIP, yflip=op.flags&SFTM_FLAG_YFLIP;
 const float srcX=hdFixed(op.src_xstep), srcY=hdFixed(op.src_ystep);
 const float cols=std::ceil(op.width/srcX), rows=std::ceil(op.height/srcY);
 const float dstX=(op.flags&SFTM_FLAG_DSTXSCALE)?hdFixed(op.dst_xstep):1.0f;
 const float dstY=hdFixed(op.dst_ystep);
 const float dw=cols*dstX, dh=rows*dstY;
 const float originX=float(op.x)+(xflip?1.0f:0.0f), originY=float(op.y)+(yflip?1.0f:0.0f);
 const float sx=xflip?-1.0f:1.0f, sy=yflip?-1.0f:1.0f;
 // Skew accumulates once per destination row/column from the start edge.
 const float xPerRow=((op.flags&SFTM_FLAG_DXDYSIGN)?1.0f:-1.0f)*(op.xstep_per_y/256.0f);
 const float yPerCol=((op.flags&SFTM_FLAG_DYDXSIGN)?-1.0f:1.0f)*(op.ystep_per_x/256.0f);
 const float u0=float(r.x)/atlasW,u1=float(r.x+r.width)/atlasW;
 const float v0=float(r.y)/atlasH,v1=float(r.y+r.height)/atlasH;
 auto corner=[&](float c,float row,float u,float v){
  return HdVertex{originX+sx*c+row*xPerRow,originY+sy*row+c*yPerCol,u,v};
 };
 HdVertex tl=corner(0,0,u0,v0),tr=corner(dw,0,u1,v0),br=corner(dw,dh,u1,v1),bl=corner(0,dh,u0,v1);
 HdBlit b{{{tl,tr,br,tl,br,bl}},tl.x,tl.y,tl.x,tl.y};
 for(const auto& v:{tl,tr,br,bl}){
  b.minX=std::min(b.minX,v.x);b.maxX=std::max(b.maxX,v.x);
  b.minY=std::min(b.minY,v.y);b.maxY=std::max(b.maxY,v.y);
 }
 return b;
}

// Clip registers apply only when the CLIP flag is set; otherwise the whole plane.
inline HdRect hdClipRect(const SftmVisualOp& op,int planeW,int planeH){
 if(!(op.flags&SFTM_FLAG_CLIP))return {0,0,planeW,planeH};
 return {op.clip_left<0?0:op.clip_left,op.clip_top<0?0:op.clip_top,op.clip_right>planeW?planeW:op.clip_right,op.clip_bottom>planeH?planeH:op.clip_bottom};
}

// VRAM addresses wrap in both directions; a quad can touch up to four tiles.
struct HdOffset { float dx,dy; };
inline std::vector<HdOffset> hdWrapOffsets(const HdBlit& b,int planeW,int planeH){
 std::vector<HdOffset> out;
 const float baseX=-std::floor(b.minX/planeW)*planeW, baseY=-std::floor(b.minY/planeH)*planeH;
 for(int ix=0;ix<2;++ix){
  const float dx=baseX-ix*planeW;
  if(b.minX+dx>=planeW||b.maxX+dx<=0)continue;
  for(int iy=0;iy<2;++iy){
   const float dy=baseY-iy*planeH;
   if(b.minY+dy>=planeH||b.maxY+dy<=0)continue;
   out.push_back({dx,dy});
  }
 }
 return out;
}

// Row indices touched by a shift-register clear: row y is copied over `height-1` rows.
inline std::vector<int> hdClearRows(const SftmVisualOp& op,int planeH){
 std::vector<int> rows;
 const int dir=(op.flags&SFTM_FLAG_YFLIP)?-1:1;
 for(int k=1;k<op.height;++k){
  int y=(op.y+dir*k)%planeH; if(y<0)y+=planeH;
  rows.push_back(y);
 }
 return rows;
}
