#pragma once
#include "systems/enemy/interior_navigation.hpp"
#include "systems/weapon/combat_queries.hpp"
namespace DroidCampos { namespace Systems { namespace Enemy {
class DroidController {
  InteriorNavigation navigation_;
  std::array<int,InteriorNavigation::kCapacity> path_{};
  int size_=0,cursor_=0;
 public:
  void advance(Core::WorldState& s,Events::EventBus& bus,u32 elapsed,Weapon::WeaponScene* scene) {
    using namespace Core;using namespace Physics;using namespace Weapon;
    auto& b=s.droid;
    if(!s.simulationActive() || s.scene.active!=SceneId::Interior)return;
    if(b.phase==DroidPhase::ButtonRising) {
      b.phaseTicks=std::min(4*kDroidSecond,b.phaseTicks+elapsed);
      if(b.phaseTicks==4*kDroidSecond){b.phase=DroidPhase::ButtonReady;b.phaseTicks=0;}
      return;
    }
    if(b.phase==DroidPhase::Release) {
      b.phaseTicks=std::min(6*kDroidSecond,b.phaseTicks+elapsed);
      if(b.phaseTicks>=3*kDroidSecond) {
        float t=(b.phaseTicks-3*kDroidSecond)/float(kDroidSecond);
        // Clear the raised capsule base before the short landing.
        b.position.set(3.58F-2.68F*std::min(t/2.F,1.F),1.237F*(1-std::max(0.F,t-2)), -29,1);
        b.animationTicks+=elapsed;b.moving=t<2;
      }
      if(b.phaseTicks==6*kDroidSecond) {
        // A player deliberately blocking the exit is displaced, never overlapped.
        if(distanceXZ(b.position,s.player.playerPos)<kDroidRadius+kPlayerBodyRadius) {
          auto p=projectInterior(s,Tyra::Vec4(-.5F,0,-29,1),kPlayerBodyRadius);
          s.player.playerPos.x=p.x;s.player.playerPos.z=p.z;
        }
        b.phase=DroidPhase::Pursuit;b.phaseTicks=0;b.pathTicks=0;b.moving=false;
        navigation_.build(s,kDroidRadius);size_=cursor_=0;
      }
      return;
    }
    if(b.phase!=DroidPhase::Pursuit)return;
    b.phaseTicks+=elapsed;b.moving=false;
    // Crouch before entering low corridors or side-room doorways.
    b.crouched=b.position.z>-25.5F || b.attacking;
    if(b.passageDoor>=0 && std::fabs(b.position.x-kInsideDoorX[b.passageDoor])>1.65F)b.passageDoor=-1;
    if(b.passageDoor<0)for(int d=0;d<4;++d)
      if(std::fabs(b.position.x-kInsideDoorX[d])<1.8F && std::fabs(b.position.z-kInsideDoorZ[d])<.65F &&
        (s.player.playerPos.x-kInsideDoorX[d])*(b.position.x-kInsideDoorX[d])<0) {
        b.passageDoor=d;b.passageSide=s.player.playerPos.x>kInsideDoorX[d]?1:-1;break;
      }
    b.passage=b.passageDoor>=0;
    float range=distanceXZ(b.position,s.player.playerPos);
    if(b.attacking) {
      u32 previous=b.attackTicks;b.attackTicks=std::min(3*kDroidSecond,b.attackTicks+elapsed);
      constexpr u32 contact=45*kDroidSecond/24,contactEnd=49*kDroidSecond/24;
      if(!b.impactDone && b.attackTicks>=contact && previous<=contactEnd) {
        b.impactDone=true;
        auto delta=sub(s.player.playerPos,b.position);
        float facing=range>.001F?(std::sin(b.yaw)*delta.x+std::cos(b.yaw)*delta.z)/range:1;
        float handY=b.position.y+1.25F;
        float feet=s.player.playerPos.y-kDefaultEyeHeight;
        auto origin=b.position;origin.y=handY;
        auto target=s.player.playerPos;target.y=std::max(feet+.2F,std::min(handY,s.player.playerPos.y));
        RayHit wall;wall.distance=std::sqrt(dot(sub(target,origin),sub(target,origin)));
        bool blocked=scene && scene->traceWeapon(s,origin,unit(sub(target,origin)),wall);
        if(range<=1.9F && facing>.65F && handY>=feet && handY<=s.player.playerPos.y+.5F && !blocked) {
          constexpr int damage = kPlayerHealth * 60 / 100;
          s.playerHealth = std::max(0, s.playerHealth - damage);
          s.damageFlashTicks = 3125;
          bus.publish(Events::GameEvent::makeSimple(Events::EventType::DroidPunchLanded));
          if(!s.playerHealth) {
            b.phase=DroidPhase::GameOver;s.menu.currentScreen=ScreenState::Defeated;
            s.menu.pauseSelectedIndex=0;s.inputCmd.clear();s.weapon.cancelTransient();
            bus.publish(Events::GameEvent::makeSimple(Events::EventType::PlayerDefeated));
          }
        }
      }
      if(b.attackTicks==3*kDroidSecond){b.attacking=false;b.attackTicks=0;}
      return;
    }
    b.yaw=std::atan2(s.player.playerPos.x-b.position.x,s.player.playerPos.z-b.position.z);
    if(!b.passage && range<=1.75F && interiorSegmentFree(s,b.position,s.player.playerPos,.01F)) {
      b.attacking=true;b.crouched=true;b.attackTicks=0;b.impactDone=false;return;
    }
    reduceTicks(b.pathTicks,elapsed);
    auto goal=s.player.playerPos;goal.y=0;
    if(!b.pathTicks) {
      navigation_.build(s,kDroidRadius);
      navigation_.route(s,b.position,goal,path_,size_);cursor_=0;b.pathTicks=7813;
    }
    auto target=goal;
    if(!b.passage && !interiorSegmentFree(s,b.position,goal,kDroidRadius,false)) {
      while(cursor_<size_ && distanceXZ(b.position,navigation_.node(path_[cursor_]))<.2F)++cursor_;
      if(cursor_>=size_)return;
      target=navigation_.node(path_[cursor_]);
    }
    for(int d=0;d<4;++d)
      if(std::fabs(b.position.x-kInsideDoorX[d])<1.4F && std::fabs(b.position.z-kInsideDoorZ[d])<.65F &&
        (target.x-kInsideDoorX[d])*(b.position.x-kInsideDoorX[d])<=0)s.scene.doors[d].targetOpen=true;
    if(b.passage) {
      int d=b.passageDoor;s.scene.doors[d].targetOpen=true;
      target=Tyra::Vec4(kInsideDoorX[d]+b.passageSide*1.8F,0,kInsideDoorZ[d],1);
      if(std::fabs(b.position.z-kInsideDoorZ[d])>.02F && std::fabs(b.position.x-kInsideDoorX[d])>1.3F)target.x=b.position.x;
    }
    auto delta=sub(target,b.position);delta.y=0;float length=distanceXZ(target,b.position);
    if(length<.001F)return;
    float step=std::min(length,elapsed/float(kDroidSecond)*2.F);
    if(b.passage && distanceXZ(b.position,s.player.playerPos)<kDroidRadius+kPlayerBodyRadius+.06F) {
      auto pushed=moveInterior(s,s.player.playerPos,delta.x/length*step,delta.z/length*step,kPlayerBodyRadius,-1);
      s.player.playerPos.x=pushed.x;s.player.playerPos.z=pushed.z;
    }
    auto next=moveInterior(s,b.position,delta.x/length*step,delta.z/length*step,kDroidRadius,kMaxEnemies);
    b.moving=distanceXZ(next,b.position)>.0001F;b.position=next;
    if(b.moving){b.animationTicks+=elapsed;b.yaw=b.passage?b.passageSide*1.570796327F:std::atan2(delta.x,delta.z);}
  }
};
}}}
