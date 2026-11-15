#pragma once
#include "systems/weapon/weapon_geometry.hpp"
#include "events/event_bus.hpp"

namespace DroidCampos { namespace Systems { namespace Weapon {
inline void reduceTicks(u32& value,u32 elapsed){value=elapsed>=value?0:value-elapsed;}
inline void tickWeapon(Core::WeaponState& w,u32 elapsed) {
  reduceTicks(w.cooldownTicks,elapsed);reduceTicks(w.beamTicks,elapsed);
  reduceTicks(w.impactTicks,elapsed);
  const bool reloading=w.reloadTicks>0;reduceTicks(w.reloadTicks,elapsed);
  if(reloading && !w.reloadTicks)w.ammo=Core::kMagazineSize;
}
inline bool reloadWeapon(Core::WeaponState& w,Events::EventBus& bus,int shooter) {
  if(w.reloadTicks || w.ammo==Core::kMagazineSize)return false;
  w.reloadTicks=Core::kReloadTicks;
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::WeaponReloadStarted,
                                        static_cast<s16>(shooter)));
  return true;
}
inline bool consumeShot(Core::WeaponState& w) {
  if(w.reloadTicks || w.cooldownTicks || w.ammo<=0)return false;
  --w.ammo;w.cooldownTicks=Core::kShotIntervalTicks;w.beamTicks=Core::kBeamTicks;
  return true;
}
// Vertical capsule ray test, with endpoints at the centers of its hemispheres.
inline bool rayCapsule(const Tyra::Vec4& o,const Tyra::Vec4& d,
                       const Tyra::Vec4& foot,float radius,float height,float& limit) {
  float best=limit;bool found=false;
  auto sphere=[&](float y){
    const Tyra::Vec4 q(o.x-foot.x,o.y-y,o.z-foot.z,0);
    float b=dot(q,d),c=dot(q,q)-radius*radius,disc=b*b-c;
    if(disc<0)return;
    float t=-b-std::sqrt(disc);if(t<0)t=-b+std::sqrt(disc);
    if(t>=.0001F && t<best){best=t;found=true;}
  };
  float a=d.x*d.x+d.z*d.z;
  if(a>1e-8F){
    float x=o.x-foot.x,z=o.z-foot.z,b=x*d.x+z*d.z;
    float disc=b*b-a*(x*x+z*z-radius*radius);
    if(disc>=0){
      for(float t : {(-b-std::sqrt(disc))/a,(-b+std::sqrt(disc))/a}) {
        float y=o.y+d.y*t;
        if(t>=.0001F && t<best && y>=foot.y+radius && y<=foot.y+height-radius)
          {best=t;found=true;}
      }
    }
  }
  sphere(foot.y+radius);sphere(foot.y+height-radius);
  if(found)limit=best;
  return found;
}
inline bool traceCombat(const WeaponScene& scene,const Core::WorldState& s,
                        const Tyra::Vec4& origin,const Tyra::Vec4& direction,
                        int shooter,RayHit& hit) {
  bool found=scene.traceWeapon(s,origin,direction,hit);
  auto actor=[&](int id,Tyra::Vec4 foot,float radius,float height){
    float t=hit.distance;
    if(!rayCapsule(origin,direction,foot,radius,height,t))return;
    hit.distance=t;hit.position=add(origin,mul(direction,t));
    const float cy=std::max(foot.y+radius,std::min(hit.position.y,foot.y+height-radius));
    hit.normal=unit(sub(hit.position,Tyra::Vec4(foot.x,cy,foot.z,1)));
    hit.actor=id;found=true;
  };
  if(shooter!=-1 && s.playerHealth>0){
    auto foot=s.player.playerPos;foot.y-=Core::kDefaultEyeHeight;
    actor(-1,foot,Core::kPlayerBodyRadius,Core::kDefaultEyeHeight+.12F);
  }
  for(int i=0;i<Core::kMaxEnemies;++i)
    if(i!=shooter && s.enemies[i].active)
      actor(i,s.enemies[i].position,Core::kEnemyRadius,Core::kEnemyHeight);
  if(s.droid.mobile() && shooter!=Core::kMaxEnemies)
    actor(Core::kMaxEnemies,s.droid.position,Core::kDroidRadius,s.droid.crouched?2.05F:3.27F);
  hit.shooter=shooter;return found;
}
inline void damageActor(Core::WorldState& s,Events::EventBus& bus,int victim,int shooter) {
  if(victim==-1){
    if(shooter<0 || s.playerHealth<=0)return;
    s.playerHealth=std::max(0,s.playerHealth-Core::kShotDamage);
    s.damageFlashTicks=3125;
    if(!s.playerHealth){
      s.menu.currentScreen=Core::ScreenState::Defeated;s.menu.pauseSelectedIndex=0;
      s.inputCmd.clear();s.weapon.cancelTransient();
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::PlayerDefeated));
    }
  }else if(victim>=0 && victim<Core::kMaxEnemies && shooter==-1){
    auto& e=s.enemies[victim];if(!e.active)return;
    e.health=std::max(0,e.health-Core::kShotDamage);e.hitTicks=1563;
    if(!e.health){
      e.active=false;e.weapon.cancelTransient();
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::EnemyDefeated,
                                            static_cast<s16>(victim)));
    }
  }
}
inline void finishShot(Core::WeaponState& w,const RayHit& hit,bool impact,
                       Core::WorldState& s,Events::EventBus& bus,int shooter) {
  w.beamEnd=hit.position;w.impactNormal=hit.normal;w.impact=impact;
  w.impactTicks=impact?Core::kImpactTicks:0;
  if(impact && hit.actor!=-2)damageActor(s,bus,hit.actor,shooter);
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::WeaponFired,
                                        static_cast<s16>(shooter)));
}
}}}
