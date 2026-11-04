#include "systems/weapon/weapon_system.hpp"
#include "systems/weapon/combat_queries.hpp"
namespace DroidCampos { namespace Systems { namespace Weapon {
void WeaponSystem::init(Tyra::Engine*,Core::WorldState&,Events::EventBus&) {
  timer_.prime();wasActive_=false;
}
void WeaponSystem::executeStage(Core::HardwareStage stage,Tyra::Engine*,
                                Core::WorldState& s,Events::EventBus& bus,
                                Hardware::Ps2ScratchpadArena&) {
  if(stage!=Core::HardwareStage::Stage1_EeVu0Simulation)return;
  u32 elapsed=timer_.getTimeDelta();timer_.prime();
  // Radio pauses combat, but visible shot effects must still finish while walking.
  if(s.playerMovementActive() && s.radio.popupActive) {
    auto expire=[&](Core::WeaponState& w) {
      reduceTicks(w.beamTicks,elapsed);reduceTicks(w.impactTicks,elapsed);
      if(!w.impactTicks)w.impact=false;
    };
    expire(s.weapon);
    for(auto& enemy:s.enemies)expire(enemy.weapon);
  }
  if(!s.simulationActive()){wasActive_=false;return;}
  if(!wasActive_)elapsed=0;
  wasActive_=true;
  auto& w=s.weapon;tickWeapon(w,elapsed);
  if(s.inputCmd.reloadRequested && reloadWeapon(w,bus,-1))return;
  if(!s.inputCmd.fireRequested || !scene_ || !consumeShot(w))return;
  const auto direction=unit(sub(s.player.cameraLookAt,s.player.cameraPos));
  RayHit hit;
  bool aimedHit=traceCombat(*scene_,s,s.player.cameraPos,direction,-1,hit);
  auto target=aimedHit?hit.position:add(s.player.cameraPos,mul(direction,50));
  w.beamStart=scene_->weaponMuzzle(s);
  auto toMuzzle=sub(w.beamStart,s.player.cameraPos);RayHit obstruction;
  obstruction.distance=std::sqrt(dot(toMuzzle,toMuzzle));
  bool impact=false;
  if(traceCombat(*scene_,s,s.player.cameraPos,unit(toMuzzle),-1,obstruction)) {
    w.beamStart=s.player.cameraPos;hit=obstruction;impact=true;
  }else {
    auto segment=sub(target,w.beamStart);RayHit muzzleHit;
    muzzleHit.distance=std::sqrt(dot(segment,segment))+.0001F;
    if(traceCombat(*scene_,s,w.beamStart,unit(segment),-1,muzzleHit)) {
      hit=muzzleHit;impact=true;
    }else {impact=aimedHit;hit.position=target;}
  }
  finishShot(w,hit,impact,s,bus,-1);
}
}}}
