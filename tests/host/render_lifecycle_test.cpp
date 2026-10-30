#include <cassert>
#include <algorithm>
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
  auto checkPass=[&](int totalDraws) {
    s.menu.currentScreen=Core::ScreenState::GameplayFps;
    for(int initialMask:{0,1}) {
      Tyra::StaticPipeline::visible=true;engine.renderer.core.gs.zBuffer.mask=initialMask;
      Tyra::RendererCore::renderEvents.clear();Tyra::StaticPipeline::draws.clear();
      render.executeStage(Core::HardwareStage::Stage2_Vu1GsRender3D,&engine,s,bus,spr);
      assert(Tyra::StaticPipeline::draws.size()==unsigned(totalDraws));
      assert(engine.renderer.core.gs.zBuffer.mask==initialMask);
      const auto& events=Tyra::RendererCore::renderEvents;
      auto p=std::find(events.begin(),events.end(),"Zoff");assert(p!=events.end());--p;
      const auto index=p-events.begin();assert(index+7<=int(events.size()));
      const std::vector<std::string> expected={"P1","Zoff","P3","draw","P1",initialMask?"Zoff":"Zon","P3"};
      assert(std::vector<std::string>(p,p+7)==expected);
      assert(std::count(events.begin(),events.end(),"P1")==2);
      assert(std::count(events.begin(),events.end(),"P3")==2);
    }
    Tyra::StaticPipeline::visible=false;Tyra::RendererCore::renderEvents.clear();
    Tyra::StaticPipeline::draws.clear();
    render.executeStage(Core::HardwareStage::Stage2_Vu1GsRender3D,&engine,s,bus,spr);
    assert(Tyra::StaticPipeline::draws.size()==unsigned(totalDraws-1));
    for(const auto& event:Tyra::RendererCore::renderEvents)assert(event=="draw" || event=="P1");
    Tyra::StaticPipeline::visible=true;engine.renderer.core.gs.zBuffer.mask=0;
  };
  for(int cycle=0;cycle<3;cycle++){
    render.loadSceneStep(&engine,Core::SceneId::Exterior,0);render.loadSceneStep(&engine,Core::SceneId::Exterior,1);
    checkPass(3); // Exterior, mist and weapon.
    assert(repo.getTexturesCount()>1);render.unloadScene(&engine);assert(repo.getTexturesCount()==1);
    for(int step=0;step<Core::kInsideLoadStepCount;step++)assert(render.loadSceneStep(&engine,Core::SceneId::Interior,step));
    assert(repo.getTexturesCount()==13); // Scene, two enemies and persistent pistol.
    checkPass(16); // No enemies before the first terminal interaction.
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
    assert(Tyra::StaticPipeline::draws.size()==16);
    assert(engine.renderer.core.gs.zBuffer.mask==0); // Restore depth writes after glass.
    render.unloadScene(&engine);assert(repo.getTexturesCount()==1);
  }
  render.shutdown(&engine);assert(repo.getTexturesCount()==0);
  std::cout<<"PASS: asset preflight, 3 unload cycles, 10 shared textures, mist/glass PATH1/PATH3 ordering, both prior depth masks, invisible pass rejection, doors and persistent pistol\n";
}
