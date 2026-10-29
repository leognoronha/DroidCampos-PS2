#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <chrono>
#include "systems/enemy/enemy_test_system.hpp"
#include "core/interior_layout.hpp"
using namespace DroidCampos;
using namespace Systems::Enemy;
int main(int argc, char** argv) {
  assert(argc == 2); Tyra::FileUtils::root = argv[1];
  // All samples remain bounded and the two lanes avoid the capsule/terminal.
  for (int variant = 0; variant < 2; ++variant) {
    bool ready=false, run=false, shoot=false;
    for (u32 tick = 0; tick < kEnemyCycleTicks; ++tick) {
      auto p = sampleEnemyTest(tick, variant);
      assert(p.from >= 0 && p.to >= 0 && p.from < 18 && p.to < 18);
      assert(p.blend >= 0 && p.blend <= 1);
      assert(p.z >= -30.801F && p.z <= -28.799F);
      assert(p.x > -3 && p.x < Core::kCapsuleMinX - 1);
      ready |= p.action == TestAction::Ready;
      run |= p.action == TestAction::Run; shoot |= p.action == TestAction::Shoot;
    }
    assert(ready && run && shoot);
    auto a=sampleEnemyTest(0,variant), b=sampleEnemyTest(kEnemyCycleTicks,variant);
    assert(a.from==b.from && a.to==b.to && a.z==b.z && a.blend==b.blend);
  }
  assert(sampleEnemyTest(31250,0).action==TestAction::Run);
  assert(sampleEnemyTest(41667,0).action==TestAction::Ready);
  assert(sampleEnemyTest(57292,0).action==TestAction::Shoot);
  assert(sampleEnemyTest(65105,0).action==TestAction::Run);
  assert(EnemyTestSystem::validateAssets());
  Tyra::Engine engine; Events::EventBus bus; Core::WorldState s{}; s.resetToDefaults();
  EnemyTestSystem enemies; enemies.init(&engine);
  assert(!enemies.loadVariant(&engine,-1));
  for(int cycle=0;cycle<3;++cycle) {
    assert(enemies.loadVariant(&engine,0)); assert(enemies.loadVariant(&engine,1));
    assert(!enemies.loadVariant(&engine,0));
    assert(engine.renderer.repo.getTexturesCount()==2);
    s.scene.active=Core::SceneId::Interior;s.scene.resident=s.scene.presented=true;
    s.menu.currentScreen=Core::ScreenState::GameplayFps;s.radio.popupActive=false;s.intro.active=false;
    bus.clear(); enemies.update(s,bus);
    auto draw=[&](){Tyra::StaticPipeline::draws.clear();Tyra::StaticPipeline::geometrySignatures.clear();
      enemies.render(&engine);assert(Tyra::StaticPipeline::draws.size()==2);
      return Tyra::StaticPipeline::geometrySignatures[0];};
    const auto initial=draw();Tyra::Timer::ticks+=1563;enemies.update(s,bus);
    const auto moved=draw();assert(std::fabs(moved-initial)>1e-4F);
    s.menu.currentScreen=Core::ScreenState::Paused;enemies.update(s,bus);
    Tyra::Timer::ticks+=156250;enemies.update(s,bus);assert(draw()==moved);
    s.menu.currentScreen=Core::ScreenState::GameplayFps;enemies.update(s,bus);assert(draw()==moved);
    s.radio.popupActive=true;enemies.update(s,bus);Tyra::Timer::ticks+=15625;enemies.update(s,bus);assert(draw()==moved);
    s.radio.popupActive=false;
    bus.publish(Events::GameEvent::makeSimple(Events::EventType::PlayerSpawnResetRequested));
    enemies.update(s,bus);assert(draw()==initial);bus.clear();
    enemies.unload(&engine);assert(engine.renderer.repo.getTexturesCount()==0);
    Tyra::StaticPipeline::draws.clear();enemies.render(&engine);assert(Tyra::StaticPipeline::draws.empty());
  }
  // Corrupt data must fail before invoking the OBJ parser or adding a texture.
  namespace fs=std::filesystem;
  auto fixture=fs::temp_directory_path()/("droid-enemy-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(fixture/"enemies/leader");
  auto previous=Tyra::FileUtils::root;Tyra::FileUtils::root=fixture.generic_string()+"/";
  {std::ofstream file(fixture/"enemies/leader/motion.bin",std::ios::binary);file<<"bad";}
  assert(!enemies.loadVariant(&engine,0));
  {std::ofstream file(fixture/"enemies/leader/motion.bin",std::ios::binary);
   const u32 count=5343,frames=18;file.write("ETIPANM1",8);file.write(reinterpret_cast<const char*>(&count),4);file.write(reinterpret_cast<const char*>(&frames),4);
   std::vector<float> data(count*frames*3,0);data[17]=std::numeric_limits<float>::quiet_NaN();
   file.write(reinterpret_cast<const char*>(data.data()),data.size()*sizeof(float));}
  assert(!enemies.loadVariant(&engine,0));assert(engine.renderer.repo.getTexturesCount()==0);
  Tyra::FileUtils::root=previous;fs::remove_all(fixture);
  std::cout<<"PASS: enemy action timing, safe lanes, interpolation, pause/radio/reset, corrupt banks and three resource cycles\n";
}
