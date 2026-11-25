#include "../remaster/replacement_index.hpp"
#include <cassert>
int main(){
 ReplacementIndex index;
 SftmVisualOp op={};op.rom_address=123;op.width=32;op.height=64;
 ReplacementRegion r={0,0,0,128,256};
 assert(!index.find(op));assert(index.insert(op,r));assert(!index.insert(op,r));
 op.x=40;op.y=90;assert(index.find(op)->height==256);
 op.palette_bank=1;assert(!index.find(op));assert(index.insert(op,r));
 op.width=0;assert(!index.insert(op,r));
}
