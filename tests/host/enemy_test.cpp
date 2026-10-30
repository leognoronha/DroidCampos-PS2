#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <chrono>
#include "systems/enemy/enemy_render_system.hpp"
using namespace DroidCampos;
using namespace Systems::Enemy;
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
  Tyra::Engine engine;auto s=fixture();
  // Independent mutable geometry, shared textures and bounded pose sampling.
  EnemyRenderSystem renderer;renderer.init(&engine);assert(EnemyRenderSystem::validateAssets());
  for(int cycle=0;cycle<3;++cycle) {
    for(int i=0;i<5;++i)assert(renderer.loadSlot(&engine,i));
    assert(!renderer.loadSlot(&engine,0));assert(engine.renderer.repo.getTexturesCount()==2);
    s=fixture();for(int i=0;i<5;++i){s.enemies[i].active=true;s.enemies[i].variant=i%2;}
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
  std::cout<<"PASS: five independent meshes, shared textures, poses and corrupt assets\n";
}
