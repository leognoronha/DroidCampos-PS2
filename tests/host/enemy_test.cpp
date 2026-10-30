#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <chrono>
#include "systems/enemy/enemy_system.hpp"
#include "systems/enemy/enemy_render_system.hpp"
using namespace DroidCampos;
using namespace Systems::Enemy;
using namespace Systems::Physics;
using namespace Systems::Weapon;
struct TestScene : WeaponScene {
  bool wall=false,blind=false;
  bool traceWeapon(const Core::WorldState& s,const Tyra::Vec4& o,
                   const Tyra::Vec4& d,RayHit& h)const override {
    if(blind) {h.distance=.1F;h.position=add(o,mul(d,.1F));return true;}
    bool found=false;
    for(int i=0;i<4;++i)if(std::fabs(d.x)>1e-6F) {
      float t=(Core::kInsideDoorX[i]-o.x)/d.x;
      float z=o.z+d.z*t, y=o.y+d.y*t;
      float panel=Core::kInsideDoorZ[i]-s.scene.doors[i].openAmount*Core::kDoorTravel;
      if(t>.0001F && t<h.distance && std::fabs(z-panel)<.7F && y>=0 && y<=2.2F) {
        h.distance=t;h.position=add(o,mul(d,t));h.normal=Tyra::Vec4(-1,0,0,0);found=true;
      }
    }
    if(!wall || std::fabs(d.z)<1e-6F)return found;
    float t=(-27-o.z)/d.z;
    if(t<.0001F || t>h.distance)return found;
    h.distance=t;h.position=add(o,mul(d,t));h.normal=Tyra::Vec4(0,0,1,0);return true;
  }
  Tyra::Vec4 weaponMuzzle(const Core::WorldState& s)override{return s.player.cameraPos;}
};
Core::WorldState fixture() {
  Core::WorldState s{};s.resetToDefaults();s.scene.active=Core::SceneId::Interior;
  s.scene.resident=s.scene.presented=true;s.menu.currentScreen=Core::ScreenState::GameplayFps;
  s.radio.popupActive=false;s.radio.phase=Core::RadioPhase::Idle;
  s.player.playerPos=Tyra::Vec4(-2.5F,1.7F,-28,1);
  s.player.cameraPos=s.player.playerPos;s.player.cameraLookAt=Tyra::Vec4(0,1.7F,-28,1);
  return s;
}
int main(int argc,char** argv) {
  assert(argc==2);Tyra::FileUtils::root=argv[1];
  TestScene scene;EnemySystem ai(&scene);Events::EventBus bus;Tyra::Engine engine;
  auto s=fixture();ai.init(&engine,s,bus);
  InteriorNavigation nav;nav.build(s);assert(nav.count()>30 && nav.count()<=64);
  std::array<int,64> path;int size=0;
  const Tyra::Vec4 destinations[]={{-3.8F,0,-8,1},{3.8F,0,-8,1},
    {-3.8F,0,-16,1},{3.8F,0,-16,1},{0,0,-1.8F,1},{-2.5F,0,-28,1},{4.3F,0,-31.2F,1}};
  for(auto a:destinations)for(auto b:destinations)assert(nav.route(s,a,b,path,size));
  // Across the whole playable floor, enough distinct reachable spawn points exist.
  // Independent floor flood fill excludes isolated spaces behind solid furniture.
  constexpr int width=113,depth=321;
  std::vector<unsigned char> reachable(width*depth,0);
  std::vector<int> queue;
  const int origin=302*width+56;reachable[origin]=1;queue.push_back(origin);
  for(size_t q=0;q<queue.size();++q) {
    int id=queue[q],ix=id%width,iz=id/width;
    for(auto offset:std::array<std::array<int,2>,4>{{{{1,0}},{{-1,0}},{{0,1}},{{0,-1}}}}) {
      int nx=ix+offset[0],nz=iz+offset[1];
      if(nx<0 || nx>=width || nz<0 || nz>=depth)continue;
      int next=nz*width+nx;
      if(!reachable[next] && interiorFree(s,-5.6F+nx*.1F,-32+nz*.1F,Core::kPlayerBodyRadius,false)) {
        reachable[next]=1;queue.push_back(next);
      }
    }
  }
  assert(queue.size()>5000);
  int audited=0;
  for(int ix=0;ix<width;ix+=5)for(int iz=0;iz<depth;iz+=5) {
    if(!reachable[iz*width+ix])continue;
    float x=-5.6F+ix*.1F,z=-32+iz*.1F;
    auto trial=s;trial.player.playerPos=Tyra::Vec4(x,1.7F,z,1);
    assert(EnemySystem::spawnWave(trial,2));
    for(int i=0;i<5;++i) {
      assert(trial.enemies[i].active);
      assert(distanceXZ(trial.enemies[i].position,trial.player.playerPos)>=3);
      assert(interiorFree(trial,trial.enemies[i].position.x,trial.enemies[i].position.z,.35F));
      if(!nav.route(trial,trial.enemies[i].position,trial.player.playerPos,path,size)){std::cerr << "route failed spawn " << trial.enemies[i].position.x << ", " << trial.enemies[i].position.z << " target " << x << ", " << z << std::endl;return 2;}
      for(int j=0;j<i;++j)assert(distanceXZ(trial.enemies[i].position,trial.enemies[j].position)>=.7F);
    }
    ++audited;
  }
  assert(audited>150);
  // Swept collision cannot tunnel through the corridor wall or closed doors.
  s=fixture();auto p=moveInterior(s,Tyra::Vec4(0,0,-4,1),5,0,.35F,0);
  assert(p.x<1.2F);
  p=moveInterior(s,Tyra::Vec4(.8F,0,-8,1),3,0,.35F,0);assert(p.x<1.2F);
  s.scene.doors[1].openAmount=1;p=moveInterior(s,Tyra::Vec4(.8F,0,-8,1),3,0,.35F,0);
  assert(p.x>3.5F);
  // Full lifecycle and unique terminal trigger.
  s=fixture();bus.clear();bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalOpened));
  s.scene.terminalActive=true;ai.advance(s,bus,100000);assert(s.encounter.phase==Core::EncounterPhase::Armed);
  bus.clear();bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalClosed));
  s.scene.terminalActive=false;ai.advance(s,bus,0);assert(s.encounter.countdownTicks==46875);
  auto checkpoint=s.player;auto doorSnapshot=s.scene.doors[0];
  bus.clear();ai.advance(s,bus,46874);assert(!s.enemies[0].active);
  ai.advance(s,bus,1);assert(s.encounter.wave==1 && s.encounter.remaining==2);
  for(int wave=1;wave<=3;++wave) {
    assert(s.encounter.wave==wave && s.encounter.remaining==Core::kWaveSizes[wave-1]);
    s.playerHealth=50;
    for(int i=0;i<Core::kWaveSizes[wave-1];++i) {
      for(int shot=0;shot<4;++shot)damageActor(s,bus,i,-1);
      assert(!s.enemies[i].active && !s.enemies[i].health);
    }
    u32 count;const auto* events=bus.getFrameEvents(count);int deaths=0;
    for(u32 j=0;j<count;++j)if(events[j].type==Events::EventType::EnemyDefeated)++deaths;
    assert(deaths==Core::kWaveSizes[wave-1]);
    bus.clear();ai.advance(s,bus,0);
    if(wave<3) {
      assert(s.playerHealth==75 && s.encounter.countdownTicks==78125);
      bus.clear();ai.advance(s,bus,78124);assert(s.encounter.phase==Core::EncounterPhase::Intermission);
      ai.advance(s,bus,1);
    }
  }
  assert(s.encounter.phase==Core::EncounterPhase::Complete && s.encounter.newEntryUnlocked);
  bus.clear();ai.advance(s,bus,1);u32 count=0;auto events=bus.getFrameEvents(count);
  assert(count==1 && events[0].type==Events::EventType::RadioTransmissionTriggered);
  bus.clear();ai.advance(s,bus,100000);assert(bus.size()==0);
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalClosed));
  ai.advance(s,bus,0);assert(s.encounter.phase==Core::EncounterPhase::Complete);
  // Retry restores camera/position/doors without replaying terminal or radio.
  s.player.playerPos.x=0;s.scene.doors[0].openAmount=1;s.playerHealth=10;
  damageActor(s,bus,-1,0);assert(s.menu.currentScreen==Core::ScreenState::Defeated);
  bus.clear();bus.publish(Events::GameEvent::makeSimple(Events::EventType::EncounterRetryRequested));
  ai.advance(s,bus,99999);
  assert(s.playerHealth==100 && s.weapon.ammo==8 && !s.radio.popupActive);
  assert(s.player.playerPos.x==checkpoint.playerPos.x && s.player.cameraLookAt.z==checkpoint.cameraLookAt.z);
  assert(s.scene.doors[0].openAmount==doorSnapshot.openAmount && s.encounter.wave==0);
  assert(!s.encounter.newEntryUnlocked && s.encounter.countdownTicks==46875);
  // All suspension surfaces freeze the encounter, including doors and damage feedback.
  auto frozen=s.encounter.countdownTicks;
  for(int mode=0;mode<5;++mode) {
    auto trial=s;
    if(mode==0)trial.radio.popupActive=true;
    if(mode==1)trial.scene.terminalActive=true;
    if(mode==2)trial.intro.active=true;
    if(mode==3)trial.menu.currentScreen=Core::ScreenState::Paused;
    if(mode==4)trial.scene.presented=false;
    bus.clear();ai.advance(trial,bus,99999);assert(trial.encounter.countdownTicks==frozen);
  }
  // Real scheduler timer drops time spent paused, including the first resumed frame.
  Hardware::Ps2ScratchpadArena arena;
  ai.init(&engine,s,bus);bus.clear();
  auto sim=[&](u32 ticks){Tyra::Timer::ticks+=ticks;ai.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,s,bus,arena);};
  sim(0);sim(100);frozen=s.encounter.countdownTicks;
  s.radio.popupActive=true;sim(90000);s.radio.popupActive=false;sim(90000);
  assert(s.encounter.countdownTicks==frozen);sim(100);assert(s.encounter.countdownTicks==frozen-100);
  // Occlusion gates Combat; rotation never grants more than two firing slots.
  s=fixture();s.player.playerPos=Tyra::Vec4(0,1.7F,-29,1);
  s.encounter.phase=Core::EncounterPhase::Wave;s.encounter.wave=3;
  const Tyra::Vec4 positions[]={{-2,0,-27,1},{-1,0,-26,1},{0,0,-25,1},{1,0,-26,1},{1,0,-30.5F,1}};
  for(int i=0;i<5;++i){auto& e=s.enemies[i];e.active=true;e.position=positions[i];e.alertTicks=1;e.randomState=i+1;}
  ai.init(&engine,s,bus);bus.clear();ai.advance(s,bus,0);
  for(const auto& e:s.enemies)assert(e.behavior==Core::EnemyBehavior::Alert);
  ai.advance(s,bus,Core::kDecisionInterval);int slots=0;
  for(const auto& e:s.enemies)if(e.canShoot)++slots;
  assert(slots==2);
  int oldCursor=s.encounter.attackCursor;s.playerHealth=100;ai.advance(s,bus,Core::kAttackRotation);
  assert(s.encounter.attackCursor!=oldCursor);slots=0;
  for(const auto& e:s.enemies)if(e.canShoot)++slots;
  assert(slots<=2);
  // A pursued side room opens its own door and crosses only after clearance.
  s=fixture();s.player.playerPos=Tyra::Vec4(3.8F,1.7F,-16,1);
  s.encounter.phase=Core::EncounterPhase::Wave;s.encounter.wave=1;
  auto& pursuer=s.enemies[0];pursuer.active=true;pursuer.position=Tyra::Vec4(0,0,-16,1);
  pursuer.alertTicks=0;scene.blind=true;ai.init(&engine,s,bus);
  for(int i=0;i<400;++i){bus.clear();ai.advance(s,bus,260);s.playerHealth=100;}
  assert(s.scene.doors[3].targetOpen && s.scene.doors[3].openAmount==1);
  assert(pursuer.position.x>1.9F);
  // Door safety includes enemies, even when the player is elsewhere.
  pursuer.position=Tyra::Vec4(Core::kInsideDoorX[3],0,Core::kInsideDoorZ[3],1);
  pursuer.alertTicks=9999;s.scene.doors[3]={1,false};bus.clear();ai.advance(s,bus,100);
  assert(s.scene.doors[3].targetOpen && s.scene.doors[3].openAmount==1);
  // Five agents queue through a side-room door without overlapping or escaping geometry.
  s=fixture();s.player.playerPos=Tyra::Vec4(0,1.7F,-25,1);
  s.encounter.phase=Core::EncounterPhase::Wave;s.encounter.wave=3;
  const Tyra::Vec4 queuePositions[]={{-3.8F,0,-16,1},{-3,0,-16,1},
      {-2.2F,0,-16,1},{-3,0,-15.1F,1},{-4.1F,0,-15.1F,1}};
  for(int i=0;i<5;++i){s.enemies[i].active=true;s.enemies[i].alertTicks=0;s.enemies[i].position=queuePositions[i];}
  ai.init(&engine,s,bus);
  for(int frame=0;frame<2400;++frame) {
    bus.clear();ai.advance(s,bus,260);
    for(int i=0;i<5;++i) {
      const auto& position=s.enemies[i].position;
      assert(interiorFree(s,position.x,position.z,Core::kEnemyRadius));
      for(int j=0;j<i;++j)
        assert(distanceXZ(position,s.enemies[j].position)>=2*Core::kEnemyRadius-.0011F);
    }
  }
  for(const auto& enemy:s.enemies)assert(enemy.position.z<-18);
  // Independent mutable geometry, shared textures and bounded pose sampling.
  EnemyRenderSystem renderer;renderer.init(&engine);assert(EnemyRenderSystem::validateAssets());
  for(int cycle=0;cycle<3;++cycle) {
    for(int i=0;i<5;++i)assert(renderer.loadSlot(&engine,i));
    assert(!renderer.loadSlot(&engine,0));assert(engine.renderer.repo.getTexturesCount()==2);
    s=fixture();assert(EnemySystem::spawnWave(s,2));
    s.enemies[0].moving=true;s.enemies[0].animationTicks=5000;
    s.enemies[2].shotAnimationTicks=5000;
    Tyra::StaticPipeline::draws.clear();Tyra::StaticPipeline::geometrySignatures.clear();
    renderer.render(&engine,s);assert(Tyra::StaticPipeline::draws.size()==5);
    assert(Tyra::StaticPipeline::geometrySignatures[0]!=Tyra::StaticPipeline::geometrySignatures[2]);
    s.enemies[0].active=false;Tyra::StaticPipeline::draws.clear();renderer.render(&engine,s);
    assert(Tyra::StaticPipeline::draws.size()==4);
    renderer.unload(&engine);assert(engine.renderer.repo.getTexturesCount()==0);
  }
  namespace fs=std::filesystem;
  auto temp=fs::temp_directory_path()/("droid-ai-corrupt-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(temp/"enemies/leader");
  auto oldRoot=Tyra::FileUtils::root;Tyra::FileUtils::root=temp.generic_string()+"/";
  {std::ofstream file(temp/"enemies/leader/motion.bin",std::ios::binary);file<<"invalid";}
  assert(!renderer.loadSlot(&engine,0));assert(engine.renderer.repo.getTexturesCount()==0);
  Tyra::FileUtils::root=oldRoot;fs::remove_all(temp);
  std::cout<<"PASS: "<<audited<<" spawn positions; A*, swept movement, FSM, doors, 2/3/5 waves, damage, retry, pause, final radio and five independent meshes\n";
}
