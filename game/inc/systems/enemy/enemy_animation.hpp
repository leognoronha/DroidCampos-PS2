#pragma once
#include <cmath>
#include "core/world_state.hpp"

namespace DroidCampos { namespace Systems { namespace Enemy {
constexpr int kEnemyVariantCount=2;
constexpr int kEnemyFrameCount=18;
struct EnemyPose {int from,to;float blend;};
inline EnemyPose enemyPose(const Core::EnemyState& e) {
  int base=0,count=4;float frame=0;
  if(e.weapon.beamTicks || e.shotAnimationTicks) {
    base=12;count=6;
    frame=std::min(5.F,(7813-std::min(7813U,e.shotAnimationTicks))*5.F/7813.F);
  }else if(e.moving) {
    base=4;count=8;frame=std::fmod(e.animationTicks*(8.F/10417.F),8.F);
  }else frame=std::fmod(e.animationTicks*(4.F/31250.F),4.F);
  int index=std::min(count-1,static_cast<int>(frame));
  return {base+index,base+(index+1<count?index+1:base==12?index:0),frame-index};
}
}}}
