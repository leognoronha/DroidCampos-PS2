#include <cassert>
#include <cmath>
#include <iostream>
#include "systems/input/input_system.hpp"
#include "systems/player/player_controller_system.hpp"
#include "systems/physics/interior_geometry.hpp"
using namespace DroidCampos;using namespace Core;
int main(){
 Tyra::Engine engine;Events::EventBus bus;Hardware::Ps2ScratchpadArena spr;Systems::Input::InputSystem input;
 WorldState s{};s.resetToDefaults();s.scene.active=SceneId::Interior;s.scene.resident=s.scene.presented=true;s.menu.currentScreen=ScreenState::GameplayFps;s.radio.popupActive=false;s.intro.active=false;
 s.player.playerPos=Tyra::Vec4(0,1.7F,-24.8F,1);s.player.cameraPos=s.player.playerPos;
 auto aim=[&](float x,float y,float z){float dx=x-s.player.cameraPos.x,dy=y-s.player.cameraPos.y,dz=z-s.player.cameraPos.z;float l=std::sqrt(dx*dx+dy*dy+dz*dz);s.player.cameraLookAt=Tyra::Vec4(s.player.cameraPos.x+dx/l,s.player.cameraPos.y+dy/l,s.player.cameraPos.z+dz/l,1);};
 auto poll=[&](){bus.flushFrameEvents();input.executeStage(HardwareStage::Stage0_IopInputPoll,&engine,s,bus,spr);};
 engine.pad.clicked.Triangle=true;aim(0,.99F,-26);
 s.droid.phase=DroidPhase::ButtonRising;poll();assert(s.droid.phase==DroidPhase::ButtonRising);
 s.droid.phase=DroidPhase::ButtonReady;aim(2,1,-26);poll();assert(!s.droid.buttonNearby && s.droid.phase==DroidPhase::ButtonReady);
 aim(0,.99F,-26);engine.pad.clicked.R1=true;poll();assert(s.droid.phase==DroidPhase::Release && !s.inputCmd.fireRequested);
 s.droid.phase=DroidPhase::ButtonReady;s.player.playerPos.z=s.player.cameraPos.z=-24.4F;aim(0,.99F,-26);poll();assert(s.droid.phase==DroidPhase::ButtonReady);
 s.droid.phase=DroidPhase::GameOver;s.menu.currentScreen=ScreenState::Defeated;engine.pad.clicked={};engine.pad.clicked.Cross=true;s.menu.pauseSelectedIndex=0;poll();assert(s.menu.currentScreen==ScreenState::NewGame);
 s.menu.currentScreen=ScreenState::Defeated;s.menu.pauseSelectedIndex=1;poll();assert(s.menu.currentScreen==ScreenState::LoadGame);
 s=WorldState{};s.resetToDefaults();s.droid.phase=DroidPhase::Release;s.droid.phaseTicks=2*15625;s.player.yaw=.1F;s.player.pitch=.2F;
 Systems::Player::PlayerControllerSystem::updateCameraVectors(s);auto camera=s.player.cameraLookAt;auto physical=s.player.playerPos;
 for(int i=0;i<100;++i)Systems::Player::PlayerControllerSystem::updateCameraVectors(s);
 assert(s.player.cameraLookAt.x==camera.x && s.player.cameraLookAt.y==camera.y && s.player.playerPos.x==physical.x && s.player.yaw==.1F);
 std::cout<<"PASS: post readiness/distance/aim, consumed activation, boss game-over routes and non-accumulating camera shake\n";
}
