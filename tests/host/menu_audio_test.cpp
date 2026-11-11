#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include "systems/audio/menu_audio_system.hpp"
#include "systems/input/input_system.hpp"
using namespace DroidCampos;
int main(int argc,char**argv) {
  assert(argc>1);Tyra::FileUtils::root=argv[1];
  Core::WorldState state;state.resetToDefaults();
  Tyra::Engine engine;Events::EventBus bus;Hardware::Ps2ScratchpadArena scratch;
  Systems::Input::InputSystem input;Systems::Audio::MenuAudioSystem audio;
  audio.init(&engine,state,bus);audio.init(&engine,state,bus);
  assert(HostAudio::loaded.size()==18&&HostAudio::resets==0);
  auto* confirm=static_cast<audsrv_adpcm_t*>(nullptr);
  auto* back=confirm;
  auto* navigate=confirm;
  auto* secret=confirm;
  auto* defeat=confirm;
  std::ifstream defeatFile(std::string(argv[1]) + "audio/game_over.adpcm", std::ios::binary);
  const std::vector<char> defeatBytes((std::istreambuf_iterator<char>(defeatFile)), {});
  assert(!defeatBytes.empty());
  for(auto& entry:HostAudio::loaded) {
    if(static_cast<size_t>(entry.second)==defeatBytes.size() &&
       std::memcmp(entry.first->buffer, defeatBytes.data(), defeatBytes.size())==0)
      defeat=entry.first;
    else if(entry.second==50432)confirm=entry.first;
    else if(entry.second==10432)back=entry.first;
    else if(entry.second==2560)navigate=entry.first;
    else if(entry.second==16256)secret=entry.first;
  }
  assert(confirm&&back&&navigate&&confirm!=back&&navigate!=back&&navigate!=confirm);
  assert(secret);
  state.menu.currentScreen=Core::ScreenState::Defeated;
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::DroidPunchLanded));
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::PlayerDefeated));
  audio.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,state,bus,scratch);
  assert(defeat && HostAudio::played.size()==2 && HostAudio::played[1]==defeat);
  const auto* punch=HostAudio::played[0];
  HostAudio::played.clear();
  bus.clear();
  state.menu.currentScreen=Core::ScreenState::GameplayFps;
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::MedkitUsed));
  audio.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,state,bus,scratch);
  assert(HostAudio::played.size()==1 && HostAudio::played[0]!=punch &&
         HostAudio::played[0]!=defeat);
  HostAudio::played.clear();
  bus.clear();
  state.menu.currentScreen=Core::ScreenState::MainMenu;
  const u32 readerTicks = audio.playCardReader(&engine, 80);
  assert(readerTicks >= 56600 && readerTicks <= 56800);
  assert(HostAudio::played.size() == 1 && HostAudio::channels.back() == 6);
  assert(audio.playCardReader(&engine, 0) == readerTicks && HostAudio::played.size() == 1);
  HostAudio::played.clear();
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::WeaponPickedUp));
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::MedkitPickedUp));
  audio.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&engine,state,bus,scratch);
  assert(HostAudio::played.size() == 2 && HostAudio::played[0] != HostAudio::played[1]);
  HostAudio::played.clear();
  bus.clear();
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalSecretUnlocked));
  audio.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&engine,state,bus,scratch);
  assert(HostAudio::played.size()==1&&HostAudio::played[0]==secret);
  auto poll=[&](Core::ScreenState screen,bool x,bool circle) {
    bus.flushFrameEvents();HostAudio::played.clear();engine.pad.clicked={};
    engine.pad.clicked.Cross=x;engine.pad.clicked.Circle=circle;
    state.menu.currentScreen=screen;
    input.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&engine,state,bus,scratch);
    audio.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&engine,state,bus,scratch);
  };
  auto tick=[&]() {
    bus.flushFrameEvents();HostAudio::played.clear();
    input.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&engine,state,bus,scratch);
    audio.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&engine,state,bus,scratch);
  };
  auto click=[&]() { assert(HostAudio::played.size()==1&&HostAudio::played[0]==navigate); };
  engine.pad.clicked.DpadDown=true;state.menu.mainSelectedIndex=2;tick();click();
  assert(state.menu.mainSelectedIndex==0); // Wrap still counts as navigation.
  engine.pad.clicked={};engine.pad.clicked.DpadUp=true;tick();click();
  assert(state.menu.mainSelectedIndex==2);
  state.menu.currentScreen=Core::ScreenState::LoadGame;tick();click();
  state.menu.currentScreen=Core::ScreenState::Settings;tick();click();
  engine.pad.clicked={};engine.pad.clicked.DpadLeft=true;
  state.menu.settingsSelectedIndex=0;state.menu.bgmVolume=0;tick();assert(HostAudio::played.empty());
  state.menu.bgmVolume=50;tick();click();assert(state.menu.bgmVolume==40);
  engine.pad.clicked={};engine.pad.clicked.DpadDown=true;engine.pad.clicked.Cross=true;
  tick();assert(HostAudio::played.size()==1&&HostAudio::played[0]==confirm);
  // Held analog must click only when its actual navigation repeat is accepted.
  engine.pad.clicked={};engine.pad.left.v=255;state.menu.padCooldown=0;
  state.menu.currentScreen=Core::ScreenState::MainMenu;tick();click();
  for(int i=0;i<11;++i){tick();assert(HostAudio::played.empty());}
  tick();click();engine.pad.left.v=128;
  engine.pad.clicked.DpadDown=true;state.menu.currentScreen=Core::ScreenState::NewGame;
  tick();assert(HostAudio::played.empty()); // No selectable rows on this page.
  state.menu.currentScreen=Core::ScreenState::MainMenu;state.menu.sfxVolume=0;
  tick();assert(HostAudio::played.empty());
  state.resetToDefaults();engine.pad.clicked={};
  for(int i=0;i<3;++i) {
    state.menu.mainSelectedIndex=i;poll(Core::ScreenState::MainMenu,true,false);
    assert(HostAudio::played.size()==1&&HostAudio::played[0]==confirm);
  }
  poll(Core::ScreenState::NewGame,true,false);assert(HostAudio::played[0]==confirm);
  for(int i=0;i<3;++i) {
    state.menu.loadSelectedIndex=i;poll(Core::ScreenState::LoadGame,true,false);
    assert(HostAudio::played.size()==(i==2?0U:1U));
  }
  for(auto screen:{Core::ScreenState::NewGame,Core::ScreenState::LoadGame,Core::ScreenState::Settings}) {
    poll(screen,false,true);assert(HostAudio::played.size()==1&&HostAudio::played[0]==back);
    assert(state.menu.currentScreen==Core::ScreenState::MainMenu);
  }
  state.menu.settingsSelectedIndex=3;
  poll(Core::ScreenState::Settings,true,false);assert(HostAudio::played[0]==confirm);
  const bool scanlines=state.menu.scanlinesEnabled;
  poll(Core::ScreenState::Settings,true,true);assert(HostAudio::played.size()==1&&HostAudio::played[0]==back);
  assert(state.menu.scanlinesEnabled==scanlines);
  poll(Core::ScreenState::MainMenu,false,true);assert(HostAudio::played.empty());
  poll(Core::ScreenState::GameplayFps,true,false);assert(HostAudio::played.empty());
  state.scene.error=nullptr;poll(Core::ScreenState::Loading,true,true);assert(HostAudio::played.empty());
  state.scene.error="FAIL";poll(Core::ScreenState::Loading,false,true);assert(HostAudio::played[0]==back);
  state.menu.sfxVolume=0;poll(Core::ScreenState::MainMenu,true,false);assert(HostAudio::played.empty());
  for(int volume:HostAudio::volumes)assert(volume==0);
  state.menu.sfxVolume=30;poll(Core::ScreenState::MainMenu,true,false);assert(HostAudio::played.size()==1);
  for(int i=0;i<23;++i)assert(HostAudio::volumes[i]==((i>=3&&i<=5)?0:30));
  HostAudio::playResult=-1;poll(Core::ScreenState::MainMenu,true,false);assert(HostAudio::played.size()==1);
  HostAudio::played.clear();audio.executeStage(Core::HardwareStage::Stage3_GifPath3Render2D,&engine,state,bus,scratch);assert(HostAudio::played.empty());
  HostAudio::playResult = Tyra::ADPCM_OK;
  state.menu.settingsSelectedIndex = 2;
  engine.pad.clicked = {}; engine.pad.clicked.DpadRight = true;
  state.menu.currentScreen = Core::ScreenState::Settings;
  tick();
  assert(state.menu.ambientVolume == 80 && state.menu.sfxVolume == 30);
  HostAudio::played.clear();
  state.radio.popupActive = false;
  state.scene = {};
  state.scene.resident = state.scene.presented = true;
  state.menu.currentScreen = Core::ScreenState::GameplayFps;
  auto simulate = [&]() {
    audio.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,state,bus,scratch);
  };
  simulate();
  assert(HostAudio::played.empty() && HostAudio::volumes[23] == 0);
  state.scene.active = Core::SceneId::Interior;
  state.scene.presented = false;
  simulate();
  assert(HostAudio::played.empty());
  state.scene.presented = true;
  simulate();
  assert(HostAudio::played.size() == 1 && HostAudio::channels.back() == 23);
  assert(HostAudio::volumes[23] == 80);
  state.menu.ambientVolume = 0;
  simulate();
  assert(HostAudio::played.size() == 1 && HostAudio::volumes[23] == 0);
  state.menu.ambientVolume = 20;
  state.menu.sfxVolume = 0;
  state.scene.active = Core::SceneId::Exterior;
  simulate();
  assert(HostAudio::volumes[23] == 0);
  state.scene.active = Core::SceneId::Interior;
  simulate();
  assert(HostAudio::played.size() == 1 && HostAudio::volumes[23] == 20);
  state.menu.currentScreen = Core::ScreenState::MainMenu;
  state.menu.bgmVolume = 40;
  simulate();
  assert(HostAudio::played.size() == 2 && HostAudio::channels.back() == 5);
  assert(HostAudio::volumes[5] == 40 && HostAudio::volumes[23] == 0);
  state.menu.currentScreen = Core::ScreenState::Settings;
  state.menu.settingsReturnScreen = Core::ScreenState::MainMenu;
  state.menu.bgmVolume = 60;
  simulate();
  assert(HostAudio::played.size() == 2 && HostAudio::volumes[5] == 60);
  state.menu.settingsReturnScreen = Core::ScreenState::Paused;
  simulate();
  assert(HostAudio::volumes[5] == 0);
  state.menu.currentScreen = Core::ScreenState::GameplayFps;
  simulate();
  assert(HostAudio::played.size() == 2 && HostAudio::volumes[5] == 0 &&
         HostAudio::volumes[23] == 20);
  state.menu.sfxVolume = 65;
  state.droid.phase = Core::DroidPhase::ButtonReady;
  simulate();
  assert(HostAudio::played.size() == 2 && HostAudio::volumes[4] == 0);
  state.droid.phase = Core::DroidPhase::Release;
  simulate();
  assert(HostAudio::played.size() == 3 && HostAudio::channels.back() == 4);
  assert(HostAudio::volumes[4] == 65 && HostAudio::volumes[23] == 20);
  state.droid.phase = Core::DroidPhase::Pursuit;
  simulate();
  assert(HostAudio::played.size() == 3 && HostAudio::volumes[4] == 65);
  state.droid.moving = true;
  simulate();
  assert(HostAudio::played.size() == 4 && HostAudio::channels.back() == 3);
  assert(HostAudio::volumes[3] == 65 && HostAudio::volumes[4] == 65);
  state.droid.moving = false;
  simulate();
  assert(HostAudio::played.size() == 4 && HostAudio::volumes[3] == 0);
  state.droid.moving = true;
  simulate();
  assert(HostAudio::played.size() == 4 && HostAudio::volumes[3] == 65);
  state.droid.phase = Core::DroidPhase::GameOver;
  state.menu.currentScreen = Core::ScreenState::Defeated;
  simulate();
  assert(HostAudio::played.size() == 4 && HostAudio::volumes[4] == 0 && HostAudio::volumes[3] == 0);
  audio.shutdown(&engine);audio.shutdown(&engine);assert(HostAudio::loaded.empty()&&HostAudio::resets==1);
  // A complete second lifecycle must not retain samples in SPU2.
  audio.init(&engine,state,bus);assert(HostAudio::loaded.size()==18);
  audio.shutdown(&engine);assert(HostAudio::loaded.empty()&&HostAudio::resets==2);
  // Optional asset failure must leave menus usable, without touching shared banks.
  HostAudio::failLoad=true;audio.init(&engine,state,bus);poll(Core::ScreenState::MainMenu,true,false);
  assert(HostAudio::played.empty()&&HostAudio::loaded.empty());audio.shutdown(&engine);
  HostAudio::failLoad=false;Tyra::FileUtils::root="missing-menu-audio/";
  audio.init(&engine,state,bus);poll(Core::ScreenState::MainMenu,true,false);assert(HostAudio::played.empty());audio.shutdown(&engine);
  assert(HostAudio::resets==2);
  std::cout<<"Menu audio passed: action mapping, settings, mute, busy voices, load failure and lifecycle\n";
}
