#include <cassert>
#include <iostream>
#include "systems/enemy/enemy_system.hpp"
using namespace DroidCampos;using namespace Core;using namespace Systems;
struct EmptyScene:Weapon::WeaponScene {
 bool blocked=false;
 bool traceWeapon(const WorldState&,const Tyra::Vec4&,const Tyra::Vec4&,Weapon::RayHit&)const override{return blocked;}
 Tyra::Vec4 weaponMuzzle(const WorldState& s)override{return s.player.cameraPos;}
};
WorldState fixture(){WorldState s{};s.resetToDefaults();s.scene.active=SceneId::Interior;s.scene.resident=s.scene.presented=true;s.menu.currentScreen=ScreenState::GameplayFps;s.radio.popupActive=false;s.intro.active=false;s.encounter.phase=EncounterPhase::Complete;return s;}
int main(){
 EmptyScene scene;Events::EventBus bus;Enemy::EnemySystem enemies(&scene);Tyra::Engine engine;
 auto s=fixture();enemies.init(&engine,s,bus);
 s.encounter.newEntryUnlocked=true;
 bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalClosed));enemies.advance(s,bus,0);bus.flushFrameEvents();assert(s.droid.phase==DroidPhase::Inactive);
 s.encounter.objectiveComplete=true;bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalClosed));enemies.advance(s,bus,0);bus.flushFrameEvents();assert(s.droid.phase==DroidPhase::ButtonRising);
 enemies.advance(s,bus,4*kDroidSecond-1);assert(s.droid.phase==DroidPhase::ButtonRising);
 enemies.advance(s,bus,1);assert(s.droid.phase==DroidPhase::ButtonReady);
 s.droid.phase=DroidPhase::Release;s.droid.phaseTicks=0;Enemy::DroidController boss;
 boss.advance(s,bus,3*kDroidSecond-1,&scene);assert(!s.droid.mobile());assert(s.playerHealth==100);
 s.menu.currentScreen=ScreenState::Paused;auto frozen=s.droid.phaseTicks;boss.advance(s,bus,9999,&scene);assert(s.droid.phaseTicks==frozen);s.menu.currentScreen=ScreenState::GameplayFps;
 boss.advance(s,bus,3*kDroidSecond+1,&scene);assert(s.droid.phase==DroidPhase::Pursuit && s.droid.position.y==0);
 s.droid.position=Tyra::Vec4(0,0,-29,1);s.droid.yaw=0;s.player.playerPos=Tyra::Vec4(0,1.7F,-27.5F,1);
 boss.advance(s,bus,1,&scene);assert(s.droid.attacking);
 boss.advance(s,bus,45*kDroidSecond/24,&scene);assert(s.playerHealth==80);
 boss.advance(s,bus,100,&scene);assert(s.playerHealth==80);
 s.playerHealth=20;s.droid.attacking=false;boss.advance(s,bus,1,&scene);boss.advance(s,bus,2*kDroidSecond,&scene);
 assert(s.playerHealth==0 && s.droid.phase==DroidPhase::GameOver && s.menu.currentScreen==ScreenState::Defeated);
 s=fixture();s.droid.phase=DroidPhase::Pursuit;s.droid.position=Tyra::Vec4(0,0,-29,1);s.player.playerPos=Tyra::Vec4(0,1.7F,-27.5F,1);scene.blocked=true;
 boss.advance(s,bus,1,&scene);boss.advance(s,bus,2*kDroidSecond,&scene);assert(s.playerHealth==100);
 scene.blocked=false;s.droid.attacking=false;boss.advance(s,bus,1,&scene);s.player.playerPos.z=-24.5F;boss.advance(s,bus,2*kDroidSecond,&scene);assert(s.playerHealth==100);
 Weapon::damageActor(s,bus,kMaxEnemies,-1);assert(s.droid.phase==DroidPhase::Pursuit);
 s=fixture();s.droid.phase=DroidPhase::Pursuit;s.droid.position=Tyra::Vec4(0,0,-29,1);s.player.playerPos=Tyra::Vec4(0,1.7F,-3,1);
 for(int i=0;i<1200;++i)boss.advance(s,bus,kDroidSecond/60,&scene);
 assert(s.droid.position.z>-7 && s.droid.crouched);

 for(int side:{-1,1})for(float doorZ:{-8.F,-16.F}) {
  s=fixture();s.droid.phase=DroidPhase::Pursuit;s.droid.position=Tyra::Vec4(0,0,doorZ,1);
  s.player.playerPos=Tyra::Vec4(side*5.F,1.7F,doorZ+.4F,1);bus.clear();
  for(int i=0;i<1200 && s.playerHealth>0;++i)enemies.advance(s,bus,kDroidSecond/60);
  assert(s.droid.position.x*side>1.7F);
 }
 Enemy::EnemySystem::reset(s);assert(s.droid.phase==DroidPhase::Inactive);
 std::cout<<"PASS: terminal gate, four-second rise, six-second release, pause, melee window, dodge/wall, invulnerability, corridor pursuit and reset\n";
}
