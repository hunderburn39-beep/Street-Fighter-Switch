#include "../remaster/replacement_geometry.hpp"
#include <cassert>
int main(){
 ReplacementCommand c={};c.operation.x=10;c.operation.y=20;c.operation.width=32;c.operation.height=64;c.region={0,128,256,128,256};
 auto q=replacementQuad(c,512,512);
 assert(q[0].x==10&&q[0].y==20&&q[0].u==0.25f&&q[0].v==0.5f);
 assert(q[2].x==42&&q[2].y==84&&q[2].u==0.5f&&q[2].v==1.0f);
 bool rejected=false;try{replacementQuad(c,64,64);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);
}
