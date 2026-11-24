#include "../frame_metrics.hpp"
#include <cassert>
#include <cmath>
int main(){
 FrameMetrics m;
 assert(m.summary().frames==0);
 for(int i=1;i<=100;++i)m.record(i);
 auto s=m.summary();
 assert(s.frames==100 && s.mean_ms==50.5 && s.p95_ms==95 && s.max_ms==100);
 for(int i=0;i<600;++i)m.record(16);
 s=m.summary();
 assert(s.frames==600 && s.mean_ms==16 && s.p95_ms==16 && s.max_ms==16);
 m.record(-1);
 assert(m.summary().mean_ms==16);
}
