#pragma once
#include <switch.h>
#include <cstdint>
class SwitchInput{public:static void init(){}static void poll(){}static bool exitRequested(){return false;}static int16_t state(unsigned,unsigned,unsigned,unsigned){return 0;}};
