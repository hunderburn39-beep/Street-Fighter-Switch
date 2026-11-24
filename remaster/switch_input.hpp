#pragma once
#ifdef __SWITCH__
#include <switch.h>
#include <cstdint>
class SwitchInput {
 inline static PadState pad{};
 inline static uint64_t held=0;
public:
 static void init(){
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);
 }
 static void poll(){padUpdate(&pad);held=padGetButtons(&pad);}
 static bool graphicsChord(){return (held&HidNpadButton_StickL)&&(held&HidNpadButton_StickR);}
 static bool exitRequested(){return (held&HidNpadButton_Plus)&&(held&HidNpadButton_Minus);}
 static int16_t state(unsigned port,unsigned device,unsigned index,unsigned id){
  if(graphicsChord()&&(id==14||id==15))return 0;
  if(port!=0||device!=1||index!=0)return 0;
  static constexpr uint64_t mapping[]={
   HidNpadButton_B,HidNpadButton_Y,HidNpadButton_Minus,HidNpadButton_Plus,
   HidNpadButton_Up|HidNpadButton_StickLUp,
   HidNpadButton_Down|HidNpadButton_StickLDown,
   HidNpadButton_Left|HidNpadButton_StickLLeft,
   HidNpadButton_Right|HidNpadButton_StickLRight,
   HidNpadButton_A,HidNpadButton_X,HidNpadButton_L,HidNpadButton_R,
   HidNpadButton_ZL,HidNpadButton_ZR,HidNpadButton_StickL,HidNpadButton_StickR
  };
  return id<16&&(held&mapping[id])?1:0;
 }
};
#endif
