#include "systems/enemy/enemy_system.hpp"
#include "systems/enemy/enemy_animation.hpp"
#include "systems/enemy/enemy_muzzles.hpp"
#include <cmath>

namespace DroidCampos { namespace Systems { namespace Enemy {
using namespace Core;
using namespace Physics;
using namespace Weapon;
namespace {
const Tyra::Vec4 kSpawnPoints[] = {
  {-3.8F,0,-8,1},{3.8F,0,-8,1},{-3.8F,0,-16,1},{3.8F,0,-16,1},
  {-4.25F,0,-25.2F,1},{4.25F,0,-25.2F,1},{-4.25F,0,-31.2F,1},
  {4.25F,0,-31.2F,1},{-1.6F,0,-31.2F,1},{1.F,0,-31.2F,1},
  {-3.8F,0,-7,1},{3.8F,0,-7,1},{-3.8F,0,-15,1},{3.8F,0,-15,1}
};
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
  s.enemies={};s.encounter=EncounterState{};s.droid=DroidEncounterState{};s.playerHealth=kPlayerHealth;s.weapon=WeaponState{};
  s.damageFlashTicks=0;
}
void EnemySystem::retry(WorldState& s) {
  auto& c=s.encounter;if(!c.checkpointValid)return;
  s.player=c.checkpointPlayer;
  s.inventory=c.checkpointInventory;
  for(int i=0;i<4;++i)s.scene.doors[i]=c.checkpointDoors[i];
  s.playerHealth=kPlayerHealth;s.damageFlashTicks=0;s.weapon=WeaponState{};
  s.enemies={};s.inputCmd.clear();s.scene.terminalActive=false;
  s.radio.popupActive=false;s.radio.reviewing=false;s.radio.phase=RadioPhase::Idle;
  c.phase=EncounterPhase::Countdown;c.wave=0;c.remaining=0;
  c.countdownTicks=kEncounterDelay;c.attackTicks=0;c.attackCursor=0;
  c.completionNoticeTicks = 0;
  c.newEntryUnlocked=c.objectiveComplete=c.finalRadioPending=false;
  s.menu.currentScreen=ScreenState::GameplayFps;
}
bool EnemySystem::spawnWave(WorldState& s,int wave) {
  if(wave<0 || wave>2)return false;
  std::array<int,kMaxEnemies> chosen{};
  int count=0;
  // Prefer occluded / behind-camera points, then use any valid designated point.
  auto view=unit(sub(s.player.cameraLookAt,s.player.cameraPos));
  for(int pass=0;pass<2;++pass)for(int p=0;p<static_cast<int>(sizeof(kSpawnPoints)/sizeof(*kSpawnPoints));++p) {
    const auto& point=kSpawnPoints[p];
    if(distanceXZ(point,s.player.playerPos)<kSpawnDistance ||
       !interiorFree(s,point.x,point.z,kEnemyRadius))continue;
    bool duplicate=false;
    for(int j=0;j<count;++j)
      if(distanceXZ(point,kSpawnPoints[chosen[j]])<2*kEnemyRadius)duplicate=true;
    if(duplicate)continue;
    auto relative=sub(point,s.player.cameraPos);relative.y=0;
    bool behind=dot(unit(relative),view)<.55F;
    bool occluded=!interiorSegmentFree(s,s.player.playerPos,point,.02F);
    if(pass==0 && !behind && !occluded)continue;
    chosen[count++]=p;
    if(count==kWaveSizes[wave]) {
      s.enemies={};
      for(int i=0;i<count;++i) {
        auto& e=s.enemies[i];e.active=true;e.variant=i%2;
        e.position=kSpawnPoints[chosen[i]];
        e.randomState=static_cast<u32>(1+i*997+wave*7919);
        e.pathTicks=i*1500U;
        e.yaw=std::atan2(s.player.playerPos.x-e.position.x,s.player.playerPos.z-e.position.z);
      }
      s.encounter.wave=wave+1;s.encounter.remaining=count;
      s.encounter.phase=EncounterPhase::Wave;s.encounter.attackTicks=0;
      return true;
    }
  }
  return false; // No partial waves. The authored spawn coverage is audited in host tests.
}
void EnemySystem::executeStage(HardwareStage stage,Tyra::Engine*,WorldState& s,
                               Events::EventBus& bus,Hardware::Ps2ScratchpadArena&) {
  if(stage!=HardwareStage::Stage1_EeVu0Simulation)return;
  u32 elapsed=timer_.getTimeDelta();timer_.prime();
  reduceTicks(s.encounter.completionNoticeTicks, elapsed);
  if(!s.simulationActive() || !wasActive_)elapsed=0;
  wasActive_=s.simulationActive();
  advance(s,bus,std::min(elapsed,3907U));
}
void EnemySystem::advance(WorldState& s,Events::EventBus& bus,u32 elapsed) {
  auto& c=s.encounter;
  u32 count=0;auto events=bus.getFrameEvents(count);
  for(u32 i=0;i<count;++i) {
    switch(events[i].type) {
      case Events::EventType::PlayerSpawnResetRequested:
        reset(s);navigationReady_=false;pathSizes_.fill(0);wasActive_=false;break;
      case Events::EventType::EncounterRetryRequested:
        retry(s);pathSizes_.fill(0);pathCursors_.fill(0);decisionTicks_=0;
        wasActive_=false;elapsed=0;break;
      case Events::EventType::TerminalOpened:
        if(c.phase==EncounterPhase::Dormant)c.phase=EncounterPhase::Armed;
        break;
      case Events::EventType::TerminalClosed:
        if(c.newEntryUnlocked && c.objectiveComplete && s.droid.phase==DroidPhase::Inactive) {
          s.droid.phase=DroidPhase::ButtonRising;s.droid.phaseTicks=0;
        }
        if(c.phase==EncounterPhase::Armed) {
          c.checkpointValid=true;c.checkpointPlayer=s.player;
          c.checkpointInventory=s.inventory;
          for(int d=0;d<4;++d)c.checkpointDoors[d]=s.scene.doors[d];
          c.phase=EncounterPhase::Countdown;c.countdownTicks=kEncounterDelay;
          elapsed=0;
        }break;
      default:break;
    }
  }
  if(s.scene.active!=SceneId::Interior){navigationReady_=false;return;}
  if(!s.simulationActive()){wasActive_=false;return;}
  if(!navigationReady_){navigation_.build(s);navigationReady_=true;}
  reduceTicks(s.damageFlashTicks,elapsed);
  doors(s,static_cast<float>(elapsed)/kCombatTicksPerSecond);
  droidController_.advance(s,bus,elapsed,scene_);
  if(c.finalRadioPending) {
    c.finalRadioPending=false;
    bus.publish(Events::GameEvent::makeSimple(Events::EventType::RadioTransmissionTriggered,
                                            0,0,kEncounterRadioMessage));
    // Stop this frame as soon as the new transmission owns gameplay.
    return;
  }
  if(c.phase==EncounterPhase::Countdown || c.phase==EncounterPhase::Intermission) {
    reduceTicks(c.countdownTicks,elapsed);
    if(!c.countdownTicks) {
      if(spawnWave(s,c.wave)){pathSizes_.fill(0);pathCursors_.fill(0);decisionTicks_=0;}
      else c.countdownTicks=kCombatTicksPerSecond;
    }
    return;
  }
  if(c.phase!=EncounterPhase::Wave)return;
  c.remaining=0;
  for(const auto& e:s.enemies)if(e.active)++c.remaining;
  if(!c.remaining) {
    bus.publish(Events::GameEvent::makeSimple(Events::EventType::WaveCompleted,static_cast<s16>(c.wave)));
    if(c.wave==3) {
      c.phase=EncounterPhase::Complete;c.newEntryUnlocked=true;c.finalRadioPending=true;
      c.completionNoticeTicks = 4 * kCombatTicksPerSecond;
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::EncounterCompleted));
    }else {
      s.playerHealth=std::min(kPlayerHealth,s.playerHealth+kWaveHealing);
      c.phase=EncounterPhase::Intermission;c.countdownTicks=kWaveInterval;
    }
    return;
  }
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
    if(s.droid.mobile() && occupied(s.droid.position,kDroidRadius))hasActor=true;
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
