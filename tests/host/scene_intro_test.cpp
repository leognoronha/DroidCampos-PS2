#include <cassert>
#include <iostream>
#include "core/scene_intro.hpp"
#include "systems/audio/menu_audio_system.hpp"
#include "systems/input/input_system.hpp"
#include "systems/narrative/radio_codec_system.hpp"
#include "systems/ui/scene_intro_system.hpp"
#include "systems/ui/hud_system.hpp"
using namespace DroidCampos;
int main(int argc,char**argv) {
  assert(argc>1);Tyra::FileUtils::root=argv[1];
  Core::WorldState s;s.resetToDefaults();Tyra::Engine e;Events::EventBus bus;Hardware::Ps2ScratchpadArena spr;
  Systems::Ui::UiContextDrawer drawer;Systems::Ui::SceneIntroSystem intro(&drawer);Systems::Ui::HudSystem hud(drawer);
  Systems::Narrative::RadioCodecSystem radio(drawer);Systems::Audio::MenuAudioSystem audio;Systems::Input::InputSystem input;
  radio.init(&e,s,bus);audio.init(&e,s,bus);intro.init(&e,s,bus);
  auto step=[&](u32 delta){Tyra::Timer::ticks+=delta;
    radio.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&e,s,bus,spr);
    audio.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&e,s,bus,spr);
    intro.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&e,s,bus,spr);};
  auto draw=[&](){e.renderer.renderer2D.draws.clear();
    radio.executeStage(Core::HardwareStage::Stage3_GifPath3Render2D,&e,s,bus,spr);
    hud.executeStage(Core::HardwareStage::Stage3_GifPath3Render2D,&e,s,bus,spr);
    intro.executeStage(Core::HardwareStage::Stage3_GifPath3Render2D,&e,s,bus,spr);};
  s.menu.currentScreen=Core::ScreenState::GameplayFps;s.scene.presented=true;
  step(0);assert(HostAudio::played.empty()); // No resident scene yet.
  s.scene.resident=true;s.scene.presented=true; // Previously loaded scene still exists.
  s.menu.currentScreen=Core::ScreenState::NewGame;e.pad.clicked.Cross=true;
  input.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&e,s,bus,spr);
  assert(s.menu.currentScreen==Core::ScreenState::GameplayFps&&s.scene.requested);
  step(0);assert(HostAudio::played.empty()&&s.radio.phase==Core::RadioPhase::Waiting);
  bus.flushFrameEvents();e.pad.clicked={};s.menu.currentScreen=Core::ScreenState::Loading;s.scene.requested=false;
  step(60000);assert(HostAudio::played.empty());
  s.scene.resident=true;s.scene.presented=false;s.menu.currentScreen=Core::ScreenState::GameplayFps;
  step(10000);assert(HostAudio::played.empty()); // Gameplay alone cannot start a call.
  s.intro={true,0};step(10000);assert(s.intro.elapsedTicks==0); // Discard loading time.
  draw();auto& d=e.renderer.renderer2D.draws;
  assert(d.size()==1&&d[0].position.x==0&&d[0].position.y==0);
  assert(d[0].size.x==512&&d[0].size.y==448&&d[0].color.a==128);
  s.scene.presented=true; // GameApp marks this only after endFrame.
  step(Core::kIntroFadeTicks/2);draw();assert(d[0].size.x==512&&d[0].color.a>63&&d[0].color.a<65);
  assert(d.size()>1&&HostAudio::played.empty());const auto early=d.size();
  step(Core::kIntroFadeTicks-Core::kIntroFadeTicks/2);draw();assert(d[0].size.x!=512&&d.size()>early);
  for(auto& sprite:d){assert(sprite.position.x>=34&&sprite.position.x+sprite.size.x*sprite.scale<=500);assert(sprite.position.y>=292&&sprite.position.y+sprite.size.y*sprite.scale<=400);}
  e.pad.clicked.Cross=true;e.pad.clicked.L2=true;e.pad.pressed.DpadUp=true;
  bus.flushFrameEvents();input.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&e,s,bus,spr);
  assert(bus.size()==0&&s.inputCmd.moveForward==0);e.pad.clicked={};e.pad.pressed={};
  s.menu.currentScreen=Core::ScreenState::Settings;const auto before=s.intro.elapsedTicks;step(15000);draw();
  assert(s.intro.elapsedTicks==before&&d.empty()&&HostAudio::played.empty());
  s.menu.currentScreen=Core::ScreenState::GameplayFps;
  step(Core::introTextEndTicks()-s.intro.elapsedTicks);draw();
  assert(Core::introVisibleCharacters(s.intro.elapsedTicks)==static_cast<int>(std::strlen(Core::kIntroLocationText)));
  step(Core::introTextEndTicks()+Core::kIntroTextHoldTicks+Core::kIntroTextFadeTicks-s.intro.elapsedTicks);draw();
  assert(d.empty()&&s.intro.active&&HostAudio::played.empty());
  step(Core::introDurationTicks(Core::SceneId::Exterior)-s.intro.elapsedTicks);
  assert(!s.intro.active&&HostAudio::played.empty());step(1);
  assert(HostAudio::played.size()==1&&s.radio.phase==Core::RadioPhase::Alert);
  s.scene.active=Core::SceneId::Interior;s.intro={true,0};s.radio.phase=Core::RadioPhase::Waiting;
  HostAudio::played.clear();step(Core::kIntroFadeTicks);draw();assert(s.intro.active&&d.empty()&&HostAudio::played.empty());
  step(Core::kIntroRadioGraceTicks);assert(!s.intro.active&&HostAudio::played.empty());
  audio.shutdown(&e);drawer.shutdown(&e);
  std::cout<<"PASS: loading/request/first-frame audio gates, black-to-scene fade, typed location, pause, input/HUD gating, quiet delay and interior fade\n";
}
