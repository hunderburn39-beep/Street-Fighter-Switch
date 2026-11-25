#include "visual_bridge.h"
#include <string.h>
static SftmVisualSink sink;
static uint64_t frame;
static int active;
void sftm_visual_bind(const SftmVisualSink *s) {
 if(active) return;
 if(s) sink=*s; else memset(&sink,0,sizeof sink);
 frame=0;
}
void sftm_visual_begin(void) {
 if(active) return;
 active=1;
 if(sink.begin_frame) sink.begin_frame(sink.context,frame);
}
void sftm_visual_transfer(const SftmVisualOp *op) {
 if(active && op && sink.transfer) sink.transfer(sink.context,op);
}
void sftm_visual_end(void) {
 if(!active) return;
 if(sink.end_frame) sink.end_frame(sink.context,frame);
 active=0; ++frame;
}
