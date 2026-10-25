#include <cassert>
#include <cmath>
#include <iostream>
#include "systems/render3d/world_render_system.hpp"
#include "core/interior_layout.hpp"
using namespace DroidCampos;
int main(int argc,char** argv){
  assert(argc==2);Tyra::FileUtils::root=argv[1];
  Tyra::Engine engine;Core::WorldState s{};s.resetToDefaults();
  Events::EventBus bus;Hardware::Ps2ScratchpadArena spr;
  Systems::Render3D::WorldRenderSystem render;
  assert(render.validateScene(Core::SceneId::Exterior));assert(render.validateScene(Core::SceneId::Interior));
  auto root=Tyra::FileUtils::root;Tyra::FileUtils::root+="missing/";
  assert(!render.validateScene(Core::SceneId::Interior));Tyra::FileUtils::root=root;
  render.init(&engine,s,bus);auto& repo=engine.renderer.repo;assert(repo.getTexturesCount()==1);
  for(int cycle=0;cycle<3;cycle++){
    render.loadSceneStep(&engine,Core::SceneId::Exterior,0);render.loadSceneStep(&engine,Core::SceneId::Exterior,1);
    assert(repo.getTexturesCount()>1);render.unloadScene(&engine);assert(repo.getTexturesCount()==1);
    for(int step=0;step<9;step++)render.loadSceneStep(&engine,Core::SceneId::Interior,step);
    assert(repo.getTexturesCount()==9); // 8 shared scene PNGs + persistent pistol.
    s.scene.active=Core::SceneId::Interior;s.resetPlayerToSpawn();s.menu.currentScreen=Core::ScreenState::GameplayFps;
    for(int i=0;i<4;i++){
      s.scene.doors[i]={1,false};s.player.playerPos.set(Core::kInsideDoorX[i],1.7F,Core::kInsideDoorZ[i]);
      render.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,s,bus,spr);
      assert(s.scene.doors[i].targetOpen && s.scene.doors[i].openAmount==1);
      s.player.playerPos.set(0,1.7F,-1.8F);s.scene.doors[i].targetOpen=false;
      render.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,s,bus,spr);
      assert(s.scene.doors[i].openAmount<1);
    }
    Tyra::StaticPipeline::draws.clear();render.executeStage(Core::HardwareStage::Stage2_Vu1GsRender3D,&engine,s,bus,spr);
    assert(Tyra::StaticPipeline::draws.size()==13); // 8 static parts, four panels, pistol.
    render.unloadScene(&engine);assert(repo.getTexturesCount()==1);
  }
  render.shutdown(&engine);assert(repo.getTexturesCount()==0);
  std::cout<<"PASS: asset preflight, 3 exterior/interior unload cycles, 8 shared textures, door occupancy reversal, 4 panel draws, persistent pistol\n";
}
