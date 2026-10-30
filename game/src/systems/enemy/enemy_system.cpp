#include "systems/enemy/enemy_system.hpp"
#include "systems/enemy/enemy_animation.hpp"
#include "systems/enemy/enemy_muzzles.hpp"
#include <cmath>

namespace DroidCampos { namespace Systems { namespace Enemy {
using namespace Core;
using namespace Physics;
using namespace Weapon;
namespace {
float randomSigned(EnemyState& e) {
  e.randomState=e.randomState*1664525U+1013904223U;
  return static_cast<float>((e.randomState>>8)&0xffff)/32767.5F-1;
}
Tyra::Vec4 muzzlePosition(const EnemyState& e) {
  const auto pose=enemyPose(e);
  const float* a=kEnemyMuzzles[e.variant][pose.from];
  const float* b=kEnemyMuzzles[e.variant][pose.to];
  float x=a[0]+(b[0]-a[0])*pose.blend,y=a[1]+(b[1]-a[1])*pose.blend;
  float z=a[2]+(b[2]-a[2])*pose.blend,c=std::cos(e.yaw),sn=std::sin(e.yaw);
  return Tyra::Vec4(e.position.x+c*x+sn*z,e.position.y+y,
                    e.position.z-sn*x+c*z,1);
}
}
void EnemySystem::init(Tyra::Engine*,WorldState&,Events::EventBus&) {
  timer_.prime();wasActive_=false;navigationReady_=false;decisionTicks_=0;
}
void EnemySystem::reset(WorldState& s) {
  s.enemies={};s.encounter=EncounterState{};s.playerHealth=kPlayerHealth;s.weapon=WeaponState{};
  s.damageFlashTicks=0;
}
void EnemySystem::executeStage(HardwareStage stage,Tyra::Engine*,WorldState& s,
                               Events::EventBus& bus,Hardware::Ps2ScratchpadArena&) {
  if(stage!=HardwareStage::Stage1_EeVu0Simulation)return;
  u32 elapsed=timer_.getTimeDelta();timer_.prime();
  if(!s.simulationActive() || !wasActive_)elapsed=0;
  wasActive_=s.simulationActive();
  advance(s,bus,std::min(elapsed,3907U));
}
void EnemySystem::advance(WorldState& s,Events::EventBus& bus,u32 elapsed) {
  auto& c=s.encounter;
  u32 count=0;auto events=bus.getFrameEvents(count);
  for(u32 i=0;i<count;++i)if(events[i].type==Events::EventType::PlayerSpawnResetRequested) {
    reset(s);navigationReady_=false;pathSizes_.fill(0);wasActive_=false;
  }
  if(s.scene.active!=SceneId::Interior){navigationReady_=false;return;}
  if(!s.simulationActive()){wasActive_=false;return;}
  if(!navigationReady_){navigation_.build(s);navigationReady_=true;}
  reduceTicks(s.damageFlashTicks,elapsed);
  doors(s,static_cast<float>(elapsed)/kCombatTicksPerSecond);
  c.attackTicks+=elapsed;
  if(c.attackTicks>=kAttackRotation){c.attackTicks%=kAttackRotation;c.attackCursor=(c.attackCursor+1)%kMaxEnemies;}
  for(auto& e:s.enemies)if(e.active) {
    tickWeapon(e.weapon,elapsed);reduceTicks(e.alertTicks,elapsed);reduceTicks(e.fireTicks,elapsed);
    reduceTicks(e.pathTicks,elapsed);reduceTicks(e.hitTicks,elapsed);reduceTicks(e.shotAnimationTicks,elapsed);
    e.animationTicks=(e.animationTicks+elapsed)%312500U;
  }
  reduceTicks(decisionTicks_,elapsed);
  if(!decisionTicks_){decide(s);decisionTicks_=kDecisionInterval;}
  for(int i=0;i<kMaxEnemies;++i)if(s.enemies[i].active) {
    move(s,i,static_cast<float>(elapsed)/kCombatTicksPerSecond);
    if(s.enemies[i].behavior==EnemyBehavior::Combat && s.enemies[i].canShoot)
      fire(s,bus,i);
    if(!s.playerHealth)return;
  }
}
void EnemySystem::doors(WorldState& s,float dt) {
  for(int i=0;i<4;++i) {
    auto& d=s.scene.doors[i];
    auto occupied=[&](const Tyra::Vec4& p,float radius) {
      return std::fabs(p.x-kInsideDoorX[i])<radius+.4F &&
             std::fabs(p.z-kInsideDoorZ[i])<radius+.8F;
    };
    bool hasActor=occupied(s.player.playerPos,kPlayerBodyRadius);
    for(const auto& e:s.enemies)if(e.active && occupied(e.position,kEnemyRadius))hasActor=true;
    if(!d.targetOpen && d.openAmount>0 && hasActor)d.targetOpen=true;
    d.openAmount=std::max(0.F,std::min(1.F,d.openAmount+(d.targetOpen?1:-1)*kDoorSpeed*60*dt));
  }
}
void EnemySystem::decide(WorldState& s) {
  int slots=0;
  for(auto& e:s.enemies)e.canShoot=false;
  for(int step=0;step<kMaxEnemies;++step) {
    int i=(s.encounter.attackCursor+step)%kMaxEnemies;auto& e=s.enemies[i];
    if(!e.active)continue;
    e.yaw=std::atan2(s.player.playerPos.x-e.position.x,s.player.playerPos.z-e.position.z);
    if(e.alertTicks){e.behavior=EnemyBehavior::Alert;continue;}
    if(e.behavior==EnemyBehavior::Alert)e.pathTicks=i*kDecisionInterval;
    float range=e.behavior==EnemyBehavior::Combat?kCombatExitRange:kCombatEnterRange;
    bool visible=false;
    if(scene_ && distanceXZ(e.position,s.player.playerPos)<=range) {
      auto origin=e.position;origin.y+=1.3F;
      auto target=s.player.playerPos;target.y-=.55F;
      auto delta=sub(target,origin);RayHit hit;hit.distance=std::sqrt(dot(delta,delta))+.5F;
      visible=traceCombat(*scene_,s,origin,unit(delta),i,hit) && hit.actor==-1;
    }
    if(visible && slots<2) {e.canShoot=true;++slots;e.behavior=EnemyBehavior::Combat;}
    else e.behavior=EnemyBehavior::Pursuit;
  }
}
void EnemySystem::move(WorldState& s,int i,float dt) {
  auto& e=s.enemies[i];e.moving=false;
  // Keep facing the current target between the less frequent decision ticks.
  e.yaw=std::atan2(s.player.playerPos.x-e.position.x,s.player.playerPos.z-e.position.z);
  if(e.behavior!=EnemyBehavior::Pursuit || dt<=0)return;
  Tyra::Vec4 goal=s.player.playerPos;goal.y=0;
  // Separate approach positions avoid a pile-up and keep doorways clear.
  if(distanceXZ(goal,e.position)<7.F) {
    for(int option=0;option<5;++option) {
      float angle=(i+option)*1.256637061F;
      auto candidate=goal;candidate.x+=std::cos(angle)*3.2F;candidate.z+=std::sin(angle)*3.2F;
      if(interiorFree(s,candidate.x,candidate.z,kEnemyRadius) &&
         interiorSegmentFree(s,goal,candidate,kEnemyRadius,false)) {goal=candidate;break;}
    }
  }
  if(distanceXZ(goal,e.position)<.15F)return;
  if(!e.pathTicks) {
    navigation_.route(s,e.position,goal,paths_[i],pathSizes_[i]);
    pathCursors_[i]=0;e.pathTicks=kPathInterval;
  }
  Tyra::Vec4 target=goal;
  if(!interiorSegmentFree(s,e.position,goal,kEnemyRadius,false)) {
    while(pathCursors_[i]<pathSizes_[i] &&
          distanceXZ(e.position,navigation_.node(paths_[i][pathCursors_[i]]))<.18F)++pathCursors_[i];
    if(pathCursors_[i]>=pathSizes_[i])return;
    target=navigation_.node(paths_[i][pathCursors_[i]]);
  }
  // Open only a panel actually blocking the next route segment; never toggle it.
  for(int d=0;d<4;++d) {
    if(std::fabs(e.position.x-kInsideDoorX[d])<1.25F &&
       std::fabs(e.position.z-kInsideDoorZ[d])<.65F &&
       (target.x-kInsideDoorX[d])*(e.position.x-kInsideDoorX[d])<=0)
      s.scene.doors[d].targetOpen=true;
  }
  auto delta=sub(target,e.position);delta.y=0;
  float length=std::sqrt(dot(delta,delta));if(length<.001F)return;
  float step=std::min(length,kEnemySpeed*dt);
  auto next=moveInterior(s,e.position,delta.x/length*step,delta.z/length*step,kEnemyRadius,i);
  float progress=distanceXZ(next,e.position);
  e.moving=progress>.0001F;
  if(progress<step*.1F)e.stuckTicks+=static_cast<u32>(dt*kCombatTicksPerSecond);
  else e.stuckTicks=0;
  if(e.stuckTicks>=kCombatTicksPerSecond) {
    e.pathTicks=0;
    // Deterministic yield: try a short perpendicular step, collision checked.
    float side=(i%2)?1.F:-1.F;
    next=moveInterior(s,e.position,-delta.z/length*step*side,
                      delta.x/length*step*side,kEnemyRadius,i);
    e.stuckTicks=0;
  }
  e.position=next;
}
void EnemySystem::fire(WorldState& s,Events::EventBus& bus,int i) {
  auto& e=s.enemies[i];if(!scene_ || e.fireTicks || e.alertTicks)return;
  if(!e.weapon.ammo){reloadWeapon(e.weapon,bus,i);return;}
  auto body=e.position;body.y+=1.3F;
  auto target=s.player.playerPos;target.y-=.55F;
  auto delta=sub(target,body);RayHit sight;sight.distance=std::sqrt(dot(delta,delta))+.5F;
  if(!traceCombat(*scene_,s,body,unit(delta),i,sight) || sight.actor!=-1)return;
  if(!consumeShot(e.weapon))return;
  e.fireTicks=kEnemyFireInterval;e.shotAnimationTicks=7813;
  e.weapon.beamStart=muzzlePosition(e);
  RayHit hit;bool impact=false;
  auto toMuzzle=sub(e.weapon.beamStart,body);
  hit.distance=std::sqrt(dot(toMuzzle,toMuzzle));
  if(scene_->traceWeapon(s,body,unit(toMuzzle),hit)) {
    e.weapon.beamStart=body;impact=true;
  }else {
    auto direction=unit(sub(target,e.weapon.beamStart));
    auto right=unit(cross(direction,Tyra::Vec4(0,1,0,0)));
    auto up=unit(cross(right,direction));
    float angle=randomSigned(e)*3.14159265F;
    float spread=std::sqrt((randomSigned(e)+1)*.5F)*std::tan(kEnemySpreadRadians);
    direction=unit(add(direction,add(mul(right,std::cos(angle)*spread),mul(up,std::sin(angle)*spread))));
    hit=RayHit{};impact=traceCombat(*scene_,s,e.weapon.beamStart,direction,i,hit);
    if(!impact)hit.position=add(e.weapon.beamStart,mul(direction,50));
  }
  finishShot(e.weapon,hit,impact,s,bus,i);
}
}}}
