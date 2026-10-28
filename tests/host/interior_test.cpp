#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
#include "systems/input/input_system.hpp"
#include "systems/player/player_controller_system.hpp"
#include "systems/physics/collision_system.hpp"
#include "core/interior_layout.hpp"
using namespace DroidCampos;
int main(){
  Core::WorldState s{}; s.resetToDefaults(); Tyra::Engine engine;
  Events::EventBus bus; Hardware::Ps2ScratchpadArena spr;
  Systems::Input::InputSystem input;
  auto poll=[&](){bus.flushFrameEvents(); input.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&engine,s,bus,spr);};
  auto resolve=[&](float& x,float& z){spr.resetFrame(); Systems::Physics::CollisionSystem::resolvePlayerCollisions(x,z,spr,s);};
  s.menu.currentScreen=Core::ScreenState::GameplayFps;
  s.player.playerPos.set(2.48F,1.7F,3.8F);
  auto aim=[&](float x,float y,float z){
    s.player.yaw=std::atan2(z-s.player.playerPos.z,x-s.player.playerPos.x);
    s.player.pitch=std::atan2(y-s.player.playerPos.y,std::hypot(x-s.player.playerPos.x,z-s.player.playerPos.z));
    Systems::Player::PlayerControllerSystem::updateCameraVectors(s);
  };
  aim(2.48F,1.3F,3.25F);
  engine.pad.clicked.Square=true;
  poll(); assert(bus.size()==0); // Radio blocks entry.
  s.radio.popupActive=false; poll(); assert(bus.size()==1);
  aim(2.48F,1.7F,10.0F);poll();assert(bus.size()==0 && !s.cardReader.isPlayerNearby);
  aim(2.48F,4.0F,3.25F);poll();assert(bus.size()==0 && !s.cardReader.isPlayerNearby);
  aim(2.48F,1.3F,3.25F);
  engine.pad.clicked={}; engine.pad.pressed.Square=true; poll(); assert(bus.size()==0);
  engine.pad.clicked.Square=true; engine.pad.clicked.L2=true; poll();
  u32 n=0; auto* ev=bus.getFrameEvents(n); assert(n==1 && ev[0].type==Events::EventType::RadioReviewRequested);
  engine.pad.clicked={}; engine.pad.pressed={};
  s.scene.active=Core::SceneId::Interior; s.resetPlayerToSpawn();
  assert(std::fabs(s.player.playerPos.z+1.8F)<.001F && s.radio.popupActive &&
         s.radio.phase==Core::RadioPhase::Waiting);
  s.radio.popupActive=false; // Simulate dismissal before testing door input.
  for(int d=0;d<4;d++){
    const float sign=Core::kInsideDoorX[d]<0?-1:1;
    float x=0,z=Core::kInsideDoorZ[d];
    for(int step=0;step<35;step++){x+=sign*.11F;resolve(x,z);}
    assert(std::fabs(x)<1.1F); // Closed panel blocks crossing.
    s.scene.doors[d].openAmount=1;
    x=0;z=Core::kInsideDoorZ[d];
    for(int step=0;step<30;step++){x+=sign*.11F;resolve(x,z);}
    assert(std::fabs(x)>2.5F); // Open aperture permits room traversal.
    s.player.playerPos.set(sign*1.0F,1.7F,Core::kInsideDoorZ[d]);
    aim(sign*1.0F,1.7F,Core::kInsideDoorZ[d]-5.0F);
    engine.pad.clicked.Square=true;poll();assert(s.scene.nearbyDoor==-1 && !s.scene.doors[d].targetOpen);
    aim(sign*1.6F,4.0F,Core::kInsideDoorZ[d]);poll();assert(s.scene.nearbyDoor==-1);
    aim(sign*1.6F,1.7F,Core::kInsideDoorZ[d]);
    engine.pad.clicked.Square=true;poll();assert(s.scene.nearbyDoor==d && s.scene.doors[d].targetOpen);
    engine.pad.clicked={};poll();assert(s.scene.doors[d].targetOpen); // No toggle on held button.
  }
  // Face buttons never rotate the camera. Jump is edge-triggered and grounded.
  engine.pad.clicked={};engine.pad.pressed={};
  s.resetPlayerToSpawn();s.radio.popupActive=false;
  Systems::Player::PlayerControllerSystem player;
  auto tick=[&](){spr.resetFrame();player.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,s,bus,spr);};
  engine.pad.clicked.Cross=true;engine.pad.pressed.Cross=true;poll();tick();
  assert(!s.player.grounded && s.player.playerPos.y>1.7F);
  assert(s.inputCmd.lookPitchDelta==0 && s.inputCmd.lookYawDelta==0);
  engine.pad.clicked={};
  float peak=s.player.playerPos.y;
  for(int i=0;i<60;i++){poll();tick();peak=std::max(peak,s.player.playerPos.y);}
  assert(peak>2.5F && peak<3.0F && s.player.grounded);
  assert(std::fabs(s.player.cameraPos.y-1.7F)<.001F);
  // Radio overlay leaves jumping available and X never dismisses the message.
  s.radio.popupActive=true;s.radio.phase=Core::RadioPhase::Message;
  engine.pad.clicked.Cross=true;poll();tick();
  assert(!s.player.grounded && s.player.playerPos.y>1.7F && s.radio.popupActive);
  assert(bus.size()==0 && s.inputCmd.jumpRequested);
  engine.pad.clicked={};
  for(int i=0;i<60;i++){poll();tick();}
  assert(s.player.grounded && s.radio.popupActive);
  engine.pad.pressed={};engine.pad.clicked.Circle=true;poll();
  {u32 count=0;const auto* events=bus.getFrameEvents(count);assert(count==1 && events[0].type==Events::EventType::RadioDismissRequested);}
  engine.pad.clicked={};s.radio.popupActive=false;
  engine.pad.pressed={};engine.pad.pressed.R1=true;poll();assert(!s.inputCmd.isRunning);
  engine.pad.pressed.L1=true;poll();assert(s.inputCmd.isRunning);
  engine.pad.pressed={};
  s.player.playerPos.set(-3.0F,1.7F,-28.0F);engine.pad.clicked.Triangle=true;
  poll();assert(s.scene.terminalNearby && s.scene.terminalActive);
  engine.pad.clicked={};poll();assert(s.scene.terminalActive);
  engine.pad.clicked.Triangle=true;poll();assert(!s.scene.terminalActive);
  engine.pad.clicked={};
  // R2 is unassigned; L2 silently reviews the current scene message.
  engine.pad.clicked.R2=true;poll();assert(bus.size()==0 && !s.inputCmd.jumpRequested);
  engine.pad.clicked={};engine.pad.clicked.L2=true;poll();assert(bus.size()==1);
  {u32 count=0;const auto* events=bus.getFrameEvents(count);assert(count==1 && events[0].type==Events::EventType::RadioReviewRequested);}
  engine.pad.clicked={};engine.pad.clicked.Start=true;poll();
  assert(s.menu.currentScreen==Core::ScreenState::Paused);
  const float pausedY=s.player.playerPos.y;
  engine.pad.clicked={};engine.pad.pressed.DpadUp=true;poll();tick();
  assert(s.player.playerPos.y==pausedY && s.inputCmd.moveForward==0);
  engine.pad.pressed={};engine.pad.clicked.Start=true;poll();
  assert(s.menu.currentScreen==Core::ScreenState::GameplayFps);
  engine.pad.clicked={};engine.pad.clicked.Start=true;poll();
  engine.pad.clicked={};engine.pad.clicked.DpadDown=true;poll();
  assert(s.menu.pauseSelectedIndex==1);
  engine.pad.clicked={};engine.pad.clicked.Cross=true;poll();
  assert(s.menu.currentScreen==Core::ScreenState::MainMenu);
  s.menu.currentScreen=Core::ScreenState::GameplayFps;engine.pad.clicked={};
  float x=0,z=-1; for(int i=0;i<50;i++){z+=.1F;resolve(x,z);} assert(z<=-.59F);
  x=0;z=-20;for(int i=0;i<80;i++){z-=.1F;resolve(x,z);} assert(z<-27.5F);
  // Capsule occupies only the right wall; the wide central aisle stays free.
  for(int i=0;i<80;i++){x+=.1F;resolve(x,z);} assert(x>1.6F && x<1.73F);
  for(int i=0;i<120;i++){x-=.1F;resolve(x,z);} assert(x>-3.11F && x<-3.0F);
  x=0;z=-25.5F;for(int i=0;i<80;i++){x+=.1F;resolve(x,z);} assert(x>4.4F && x<4.53F);
  x=0;z=-28;for(int i=0;i<80;i++){z-=.1F;resolve(x,z);} assert(z<-31.4F && z>=-31.53F);
  x=0;z=-12;for(int i=0;i<50;i++){x+=.11F;resolve(x,z);} assert(x<1.1F);
  s.menu.currentScreen=Core::ScreenState::Loading;s.scene.error=nullptr;
  engine.pad.clicked.Square=true;engine.pad.pressed.DpadUp=true;poll();assert(bus.size()==0 && s.inputCmd.moveForward==0);
  s.menu.currentScreen=Core::ScreenState::NewGame;engine.pad.clicked={};engine.pad.clicked.Cross=true;poll();
  assert(s.scene.requested && s.scene.target==Core::SceneId::Exterior);
  std::cout<<"PASS: reader/radio/held input, 4 closed/open doors, room traversal, walls, sealed entrance, terminal, spawn, loading, new game\n";
}
