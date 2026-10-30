#include "systems/enemy/enemy_system.hpp"
#include <cmath>
namespace DroidCampos { namespace Systems { namespace Enemy {
using namespace Core;
using namespace Physics;
using namespace Weapon;
void EnemySystem::init(Tyra::Engine*,WorldState&,Events::EventBus&) {
  timer_.prime();wasActive_=false;navigationReady_=false;
}
void EnemySystem::reset(WorldState& s) {
  s.enemies={};s.playerHealth=kPlayerHealth;s.weapon=WeaponState{};
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
  u32 count=0;auto events=bus.getFrameEvents(count);
  for(u32 i=0;i<count;++i)if(events[i].type==Events::EventType::PlayerSpawnResetRequested) {
    reset(s);navigationReady_=false;pathSizes_.fill(0);wasActive_=false;
  }
  if(s.scene.active!=SceneId::Interior){navigationReady_=false;return;}
  if(!s.simulationActive()){wasActive_=false;return;}
  if(!navigationReady_){navigation_.build(s);navigationReady_=true;}
  reduceTicks(s.damageFlashTicks,elapsed);
  doors(s,static_cast<float>(elapsed)/kCombatTicksPerSecond);
  for(int i=0;i<kMaxEnemies;++i)if(s.enemies[i].active) {
    reduceTicks(s.enemies[i].pathTicks,elapsed);
    move(s,i,static_cast<float>(elapsed)/kCombatTicksPerSecond);
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
}}}
