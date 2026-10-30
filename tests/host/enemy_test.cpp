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
  // Swept collision cannot tunnel through the corridor wall or closed doors.
  s=fixture();auto p=moveInterior(s,Tyra::Vec4(0,0,-4,1),5,0,.35F,0);
  assert(p.x<1.2F);
  p=moveInterior(s,Tyra::Vec4(.8F,0,-8,1),3,0,.35F,0);assert(p.x<1.2F);
  s.scene.doors[1].openAmount=1;p=moveInterior(s,Tyra::Vec4(.8F,0,-8,1),3,0,.35F,0);
  assert(p.x>3.5F);
  // A pursued side room opens its own door and crosses only after clearance.
  s=fixture();s.player.playerPos=Tyra::Vec4(3.8F,1.7F,-16,1);
  auto& pursuer=s.enemies[0];pursuer.active=true;pursuer.position=Tyra::Vec4(0,0,-16,1);
  pursuer.behavior=Core::EnemyBehavior::Pursuit;scene.blind=true;ai.init(&engine,s,bus);
  for(int i=0;i<400;++i){bus.clear();ai.advance(s,bus,260);s.playerHealth=100;}
  assert(s.scene.doors[3].targetOpen && s.scene.doors[3].openAmount==1);
  assert(pursuer.position.x>1.9F);
  // Door safety includes enemies, even when the player is elsewhere.
  pursuer.position=Tyra::Vec4(Core::kInsideDoorX[3],0,Core::kInsideDoorZ[3],1);
  pursuer.behavior=Core::EnemyBehavior::Alert;s.scene.doors[3]={1,false};bus.clear();ai.advance(s,bus,100);
  assert(s.scene.doors[3].targetOpen && s.scene.doors[3].openAmount==1);
  // Five agents queue through a side-room door without overlapping or escaping geometry.
  s=fixture();s.player.playerPos=Tyra::Vec4(0,1.7F,-25,1);
  const Tyra::Vec4 queuePositions[]={{-3.8F,0,-16,1},{-3,0,-16,1},
      {-2.2F,0,-16,1},{-3,0,-15.1F,1},{-4.1F,0,-15.1F,1}};
  for(int i=0;i<5;++i){s.enemies[i].active=true;s.enemies[i].behavior=Core::EnemyBehavior::Pursuit;s.enemies[i].position=queuePositions[i];}
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
  std::cout<<"PASS: A*, swept movement, actor separation, doors and independent meshes\n";
}
