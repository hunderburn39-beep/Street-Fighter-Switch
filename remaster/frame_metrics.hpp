#pragma once
#include <array>
#include <algorithm>
#include <chrono>
#include <cstddef>
class FrameMetrics {
 using Clock=std::chrono::steady_clock;
 std::array<double,600> samples{};
 std::size_t count=0,next=0;
 Clock::time_point start{};
public:
 void begin(){start=Clock::now();}
 void end(){record(std::chrono::duration<double,std::milli>(Clock::now()-start).count());}
 void record(double ms){if(!(ms>=0))return;samples[next]=ms;next=(next+1)%samples.size();count=std::min(count+1,samples.size());}
 struct Summary{std::size_t frames;double mean_ms,p95_ms,max_ms;};
 Summary summary()const{
 if(!count)return {};
 auto sorted=samples;std::sort(sorted.begin(),sorted.begin()+count);
 double sum=0;for(std::size_t i=0;i<count;++i)sum+=sorted[i];
 return {count,sum/count,sorted[(95*count+99)/100-1],sorted[count-1]};
 }
};
