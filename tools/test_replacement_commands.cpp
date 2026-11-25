#include "../remaster/replacement_commands.hpp"
#include <cassert>
int main(){
 ReplacementIndex index; SftmVisualOp a={},b={};a.width=b.width=16;a.height=b.height=32;a.rom_address=1;b.rom_address=2;
 assert(index.insert(a,{0,0,0,64,128}));
 VisualQueue::Frame frame;frame.number=7;frame.ops={a,b,a};frame.ops[2].x=100;
 auto out=resolveFrame(frame,index);
 assert(out.number==7&&out.commands.size()==2&&out.unresolved.size()==1);
 assert(out.commands[1].operation.x==100&&out.unresolved[0].rom_address==2);
}
