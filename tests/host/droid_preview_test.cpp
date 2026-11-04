#include <cassert>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "systems/render3d/droid_preview.hpp"
using namespace DroidCampos;
int main(int argc,char** argv) {
 assert(argc==2);Tyra::FileUtils::root=argv[1];Tyra::Engine engine;
 Core::WorldState s{};s.resetToDefaults();
 Systems::Render3D::DroidPreview render;render.init(&engine);assert(render.validateAssets());
 for(int cycle=0;cycle<3;++cycle) {
  assert(render.load(&engine));s.droid=Core::DroidEncounterState{};
  auto draw=[&](){Tyra::StaticPipeline::draws.clear();Tyra::StaticPipeline::geometrySignatures.clear();render.render(&engine,s);};
  draw();assert(Tyra::StaticPipeline::draws.empty());
  s.droid.phase=Core::DroidPhase::ButtonRising;s.droid.phaseTicks=2*15625;draw();assert(Tyra::StaticPipeline::draws.size()==1);assert(Tyra::StaticPipeline::draws[0].y<-.5F);
  s.droid.phase=Core::DroidPhase::ButtonReady;draw();assert(Tyra::StaticPipeline::draws[0].y==0);
  s.droid.phase=Core::DroidPhase::Release;s.droid.phaseTicks=2*15625;draw();assert(Tyra::StaticPipeline::draws.size()==1);
  s.droid.phaseTicks=4*15625;draw();assert(Tyra::StaticPipeline::draws.size()==2);
  s.droid.phase=Core::DroidPhase::Pursuit;s.droid.position=Tyra::Vec4(0,0,-29,1);draw();
  std::ifstream bank(Tyra::FileUtils::root+"droid_preview/motion.bin",std::ios::binary);bank.seekg(16);std::vector<float> reference(6900*3);bank.read(reinterpret_cast<char*>(reference.data()),reference.size()*4);assert(bank);
  float expected=0;unsigned offset=0;
  for(unsigned count:{4068U,2832U}){for(unsigned i=0;i<count;i+=17){unsigned j=(offset+i)*3;expected+=reference[j]+reference[j+1]*2+reference[j+2]*3;}offset+=count;}
  assert(std::fabs(Tyra::StaticPipeline::geometrySignatures.back()-expected)<.001F);
  auto standing=Tyra::StaticPipeline::geometrySignatures.back();s.droid.crouched=true;draw();assert(std::fabs(Tyra::StaticPipeline::geometrySignatures.back()-standing)>.01F);
  auto paused=Tyra::StaticPipeline::geometrySignatures.back();draw();assert(paused==Tyra::StaticPipeline::geometrySignatures.back());
  render.unload(&engine);assert(engine.renderer.repo.getTexturesCount()==0);
 }
 std::cout<<"PASS: no duplicate actor, staged button/release, simulation-owned pose, original scale, crouch and three unloads\n";
}
