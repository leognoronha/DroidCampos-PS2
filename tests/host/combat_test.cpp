#include <cassert>
#include <iostream>
#include "systems/weapon/weapon_system.hpp"
#include "systems/weapon/combat_queries.hpp"
using namespace DroidCampos;
using namespace Systems::Weapon;
struct CombatScene : WeaponScene {
  float wallZ=-100;
  bool muzzleWall=false;
  bool traceWeapon(const Core::WorldState&,const Tyra::Vec4& o,
                   const Tyra::Vec4& d,RayHit& h)const override {
    bool found=false;
    if(std::fabs(d.z)>1e-6F) {
      float t=(wallZ-o.z)/d.z;
      if(t>.0001F && t<h.distance) {
        h.distance=t;h.position=add(o,mul(d,t));h.normal=Tyra::Vec4(0,0,1,0);found=true;
      }
    }
    if(muzzleWall && std::fabs(d.x)>1e-6F) {
      float t=(.1F-o.x)/d.x;
      if(t>.0001F && t<h.distance) {
        h.distance=t;h.position=add(o,mul(d,t));h.normal=Tyra::Vec4(-1,0,0,0);found=true;
      }
    }
    return found;
  }
  Tyra::Vec4 weaponMuzzle(const Core::WorldState&)override{return Tyra::Vec4(.2F,1.3F,-.5F,1);}
};
int main() {
  Core::WorldState s{};s.resetToDefaults();
  s.menu.currentScreen=Core::ScreenState::GameplayFps;s.scene.resident=s.scene.presented=true;
  s.radio.popupActive=false;s.player.playerPos=Tyra::Vec4(0,1.7F,0,1);
  s.player.cameraPos=s.player.playerPos;s.player.cameraLookAt=Tyra::Vec4(0,1.7F,-1,1);
  for(int i=0;i<2;++i){s.enemies[i].active=true;s.enemies[i].position=Tyra::Vec4(0,0,-3-i*3,1);}
  CombatScene scene;RayHit hit;
  assert(traceCombat(scene,s,Tyra::Vec4(0,1.3F,0,1),Tyra::Vec4(0,0,-1,0),-1,hit));
  assert(hit.actor==0 && hit.distance<3 && hit.shooter==-1);
  scene.wallZ=-2;hit=RayHit{};
  assert(traceCombat(scene,s,Tyra::Vec4(0,1.3F,0,1),Tyra::Vec4(0,0,-1,0),-1,hit));
  assert(hit.actor==-2 && hit.distance==2);
  scene.wallZ=-100;
  // Another enemy stops the shot before the player, without friendly damage.
  s.enemies[1].position=Tyra::Vec4(0,0,4,1);s.enemies[0].position=Tyra::Vec4(0,0,2,1);
  hit=RayHit{};
  assert(traceCombat(scene,s,Tyra::Vec4(0,1.3F,4,1),Tyra::Vec4(0,0,-1,0),1,hit));
  assert(hit.actor==0);
  Events::EventBus bus;Core::WeaponState shot;
  finishShot(shot,hit,true,s,bus,1);assert(s.enemies[0].health==30 && s.playerHealth==100);
  s.enemies[0].active=false;hit=RayHit{};
  assert(traceCombat(scene,s,Tyra::Vec4(0,1.3F,4,1),Tyra::Vec4(0,0,-1,0),1,hit));
  assert(hit.actor==-1);
  s.player.playerPos.y=3.0F;hit=RayHit{};
  assert(!traceCombat(scene,s,Tyra::Vec4(0,.9F,4,1),Tyra::Vec4(0,0,-1,0),1,hit));
  s.player.playerPos.y=1.7F;
  // Independent magazines and shared reload boundary.
  auto& a=s.enemies[0].weapon;auto& b=s.enemies[1].weapon;
  assert(consumeShot(a) && a.ammo==7 && b.ammo==8);
  assert(!consumeShot(a));tickWeapon(a,Core::kShotIntervalTicks);assert(consumeShot(a));
  assert(reloadWeapon(a,bus,0));tickWeapon(a,Core::kReloadTicks-1);assert(a.ammo==6);
  tickWeapon(a,1);assert(a.ammo==8 && b.ammo==8);
  // Full player shot, including camera-to-muzzle obstruction and exactly three hits.
  s.enemies[0]=Core::EnemyState{};s.enemies[0].active=true;s.enemies[0].position=Tyra::Vec4(0,0,-3,1);
  s.enemies[1].active=false;
  Tyra::Engine engine;Hardware::Ps2ScratchpadArena arena;WeaponSystem weapon(&scene);
  weapon.init(&engine,s,bus);s.inputCmd.fireRequested=true;scene.muzzleWall=true;
  weapon.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,s,bus,arena);
  assert(s.weapon.ammo==8 && !s.weapon.beamTicks);
  s.inventory.collect(Core::Pickup::Pistol);
  weapon.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,s,bus,arena);
  assert(s.enemies[0].health==30 && s.weapon.impact && s.weapon.beamStart.x==0);
  scene.muzzleWall=false;
  for(int i=0;i<3;++i) {
    bus.clear();Tyra::Timer::ticks+=Core::kShotIntervalTicks;
    weapon.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,s,bus,arena);
  }
  assert(!s.enemies[0].active && s.enemies[0].health==0);
  // Defeat is terminal for damage and wave progression, without negative health.
  s.playerHealth=10;damageActor(s,bus,-1,1);damageActor(s,bus,-1,1);
  assert(s.playerHealth==0 && s.menu.currentScreen==Core::ScreenState::Defeated);
  std::cout<<"PASS: nearest actor/world hit, friendly obstruction, jump capsules, independent reloads, obstructed muzzle, three-hit kill and defeat\n";
}
