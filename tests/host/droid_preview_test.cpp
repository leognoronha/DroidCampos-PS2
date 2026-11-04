#include <cassert>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "systems/render3d/droid_preview.hpp"
using namespace DroidCampos;
int main(int argc,char** argv) {
 assert(argc==2);Tyra::FileUtils::root=argv[1];Tyra::Engine engine;
 Core::WorldState s{};s.resetToDefaults();s.scene.active=Core::SceneId::Interior;
 s.scene.resident=s.scene.presented=true;s.menu.currentScreen=Core::ScreenState::GameplayFps;
 s.radio.popupActive=false;s.intro.active=false;
 Systems::Render3D::DroidPreview preview;preview.init(&engine);
 assert(preview.validateAssets());assert(preview.load(&engine));
 auto draw=[&](){Tyra::Timer::ticks+=1562;Tyra::StaticPipeline::draws.clear();Tyra::StaticPipeline::geometrySignatures.clear();preview.render(&engine,s);assert(Tyra::StaticPipeline::draws.size()==1);};
 draw();float start=Tyra::StaticPipeline::geometrySignatures.back();
 // The capsule uses the unscaled source mesh; the mobile mesh must match it.
 std::ifstream bank(Tyra::FileUtils::root+"droid_preview/motion.bin",std::ios::binary);
 bank.seekg(16);std::vector<float> reference(6900*3);
 bank.read(reinterpret_cast<char*>(reference.data()),reference.size()*4);assert(bank);
 float expected=0;unsigned offset=0;
 for(unsigned count:{4068U,2832U}) {for(unsigned i=0;i<count;i+=17){unsigned j=(offset+i)*3;expected+=reference[j]+reference[j+1]*2+reference[j+2]*3;}offset+=count;}
 assert(std::fabs(start-expected)<.001F);

 for(int i=0;i<20;++i)draw();
 assert(Tyra::StaticPipeline::draws.back().x> -1.1F);
 assert(std::fabs(Tyra::StaticPipeline::geometrySignatures.back()-start)>.01F);
 for(int i=0;i<23;++i)draw();
 auto pos=Tyra::StaticPipeline::draws.back();float punch=Tyra::StaticPipeline::geometrySignatures.back();
 for(int i=0;i<10;++i)draw();
 assert(std::fabs(pos.x-Tyra::StaticPipeline::draws.back().x)<.001F);
 assert(std::fabs(Tyra::StaticPipeline::geometrySignatures.back()-punch)>.01F);
 s.menu.currentScreen=Core::ScreenState::Paused;draw();
 float frozen=Tyra::StaticPipeline::geometrySignatures.back();
 for(int i=0;i<40;++i)draw();
 assert(Tyra::StaticPipeline::geometrySignatures.back()==frozen);
 s.menu.currentScreen=Core::ScreenState::GameplayFps;
 for(int i=0;i<280;++i) {draw();auto p=Tyra::StaticPipeline::draws.back();assert(p.x>=-2.001F&&p.x<=.001F&&p.z>=-30.001F&&p.z<=-25.999F);}
 preview.unload(&engine);assert(engine.renderer.repo.getTexturesCount()==0);
 assert(preview.load(&engine));draw();assert(std::fabs(Tyra::StaticPipeline::draws.back().x+2)<.001F);
 preview.unload(&engine);assert(engine.renderer.repo.getTexturesCount()==0);
 std::cout<<"PASS: DroidCampos walks, punches stationary, pauses, stays in room and reloads without texture leaks\n";
}
