#include "../remaster/visual_queue.hpp"
#include <cassert>
int main(){
 VisualQueue q; auto s=q.sink(); sftm_visual_bind(&s);
 assert(!q.available());
 SftmVisualOp a={},b={};a.rom_address=42;b.rom_address=43;
 sftm_visual_begin();sftm_visual_transfer(&a);sftm_visual_transfer(&b);sftm_visual_end();
 assert(q.available());assert(q.latest().number==0);assert(q.latest().ops.size()==2);
 assert(q.latest().ops[0].rom_address==42 && q.latest().ops[1].rom_address==43);
 sftm_visual_begin();assert(q.latest().ops.size()==2);sftm_visual_end();
 assert(q.latest().number==1 && q.latest().ops.empty());
 sftm_visual_bind(nullptr);
}
