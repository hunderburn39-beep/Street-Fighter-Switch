#include "../remaster/visual_bridge.h"
#include <assert.h>
static int phase, count;
static void begin(void *p,uint64_t f){assert(p==&count);assert(f==(uint64_t)count);assert(phase==0);phase=1;}
static void transfer(void *p,const SftmVisualOp *op){assert(p==&count);assert(phase==1);assert(op->rom_address==123);}
static void end(void *p,uint64_t f){assert(p==&count);assert(f==(uint64_t)count);assert(phase==1);phase=0;++count;}
int main(void){
 SftmVisualSink sink={&count,begin,transfer,end};
 SftmVisualOp op={0};op.rom_address=123;
 sftm_visual_bind(&sink);
 sftm_visual_transfer(&op);
 sftm_visual_begin();sftm_visual_begin();sftm_visual_transfer(&op);sftm_visual_end();sftm_visual_end();
 sftm_visual_begin();sftm_visual_end();
 assert(count==2 && phase==0);
 sftm_visual_bind(0);sftm_visual_begin();sftm_visual_end();
 assert(count==2);return 0;
}
