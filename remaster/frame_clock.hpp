#pragma once
#include <chrono>
#include <cmath>
#include <stdexcept>
class FrameClock {
 using Clock=std::chrono::steady_clock;
 using Seconds=std::chrono::duration<double>;
 using Point=std::chrono::time_point<Clock,Seconds>;
 Seconds period;
 Point deadline;
public:
 explicit FrameClock(double fps):period(0),deadline(Clock::now()){
  if(!std::isfinite(fps)||fps<=0||fps>1000)throw std::invalid_argument("Invalid core FPS");
  period=Seconds(1.0/fps);
 }
 void reset(){deadline=Clock::now();}
 void advance(){deadline+=period;}
 auto next()const{return deadline;}
 double lateness()const{return Seconds(Clock::now()-deadline).count();}
};
