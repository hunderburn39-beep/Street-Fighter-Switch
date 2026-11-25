#include "../remaster/hd_blit.hpp"
#include <cassert>
#include <cmath>
static bool near(float a,float b){return std::fabs(a-b)<1e-4f;}
static SftmVisualOp blit(){
 SftmVisualOp o={};o.kind=SFTM_OP_BLIT;o.x=10;o.y=20;o.width=16;o.height=32;
 o.src_xstep=o.src_ystep=0x100;o.dst_xstep=o.dst_ystep=0x100;return o;
}
int main(){
 ReplacementRegion r{0,64,128,64,128}; // 4x texture at atlas (64,128)
 {// plain blit: rect [10,26) x [20,52), uv exactly the region
  auto b=hdBlitGeometry(blit(),r,2048,2048);
  assert(near(b.minX,10)&&near(b.maxX,26)&&near(b.minY,20)&&near(b.maxY,52));
  assert(near(b.vertices[0].u,64/2048.f)&&near(b.vertices[0].v,128/2048.f));
  assert(near(b.vertices[2].u,128/2048.f)&&near(b.vertices[2].v,256/2048.f));
 }
 {// X flip: start pixel is the right edge, sprite grows leftwards, uv stays source order
  auto o=blit();o.flags=SFTM_FLAG_XFLIP;auto b=hdBlitGeometry(o,r,2048,2048);
  assert(near(b.minX,-5)&&near(b.maxX,11));
  assert(near(b.vertices[0].x,11)&&near(b.vertices[1].x,-5)&&near(b.vertices[0].u,64/2048.f));
 }
 {// Y flip
  auto o=blit();o.flags=SFTM_FLAG_YFLIP;auto b=hdBlitGeometry(o,r,2048,2048);
  assert(near(b.minY,-11)&&near(b.maxY,21));
 }
 {// destination X scale 0x80 halves the width; source step 0x200 skips every other row
  auto o=blit();o.flags=SFTM_FLAG_DSTXSCALE;o.dst_xstep=0x80;o.src_ystep=0x200;
  auto b=hdBlitGeometry(o,r,2048,2048);
  assert(near(b.maxX-b.minX,8)&&near(b.maxY-b.minY,16));
 }
 {// skew: bottom edge shifted by rows*xstep_per_y
  auto o=blit();o.flags=SFTM_FLAG_DXDYSIGN;o.xstep_per_y=0x100;
  auto b=hdBlitGeometry(o,r,2048,2048);
  assert(near(b.vertices[5].x,10+32)&&near(b.vertices[0].x,10));
 }
 {// clip
  auto o=blit();auto c=hdClipRect(o,512,1024);assert(c.x0==0&&c.x1==512&&c.y1==1024);
  o.flags=SFTM_FLAG_CLIP;o.clip_left=8;o.clip_right=384;o.clip_top=0;o.clip_bottom=240;
  c=hdClipRect(o,512,1024);assert(c.x0==8&&c.x1==384&&c.y0==0&&c.y1==240);
 }
 {// wrap: straddling the right edge needs two tiles; fully inside needs one
  auto o=blit();o.x=500;auto b=hdBlitGeometry(o,r,2048,2048);
  assert(hdWrapOffsets(b,512,1024).size()==2);
  o.x=100;b=hdBlitGeometry(o,r,2048,2048);assert(hdWrapOffsets(b,512,1024).size()==1);
  o.x=600;b=hdBlitGeometry(o,r,2048,2048);auto w=hdWrapOffsets(b,512,1024);
  assert(w.size()==1&&near(w[0].dx,-512));
 }
 {// shift-register clear rows
  SftmVisualOp o={};o.y=1022;o.height=4;auto rows=hdClearRows(o,1024);
  assert(rows.size()==3&&rows[0]==1023&&rows[1]==0&&rows[2]==1);
  o.flags=SFTM_FLAG_YFLIP;o.y=1;rows=hdClearRows(o,1024);assert(rows[0]==0&&rows[1]==1023);
 }
 {// invalid regions are rejected
  bool threw=false;try{hdBlitGeometry(blit(),ReplacementRegion{0,2000,0,100,10},2048,2048);}catch(const std::invalid_argument&){threw=true;}
  assert(threw);
 }
}
