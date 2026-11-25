#pragma once
#include <switch.h>
#include <cstdint>
#include <cstddef>
class SwitchAudio{public:SwitchAudio(){}static void setSourceRate(double){}static void sample(int16_t,int16_t){}static std::size_t batch(const int16_t*,std::size_t n){return n;}};
