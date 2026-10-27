#include <cassert>
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
  assert(HostAudio::loaded.size()==3&&HostAudio::resets==0);
  auto* confirm=static_cast<audsrv_adpcm_t*>(nullptr);
  auto* back=confirm;
  auto* navigate=confirm;
  for(auto& entry:HostAudio::loaded) {
    if(entry.second>40000)confirm=entry.first;
    else if(entry.second==10432)back=entry.first;
    else navigate=entry.first;
  }
  assert(confirm&&back&&navigate&&confirm!=back&&navigate!=back&&navigate!=confirm);
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
  state.menu.settingsSelectedIndex=2;
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
  for(int volume:HostAudio::volumes)assert(volume==30);
  HostAudio::playResult=-1;poll(Core::ScreenState::MainMenu,true,false);assert(HostAudio::played.size()==1);
  HostAudio::played.clear();audio.executeStage(Core::HardwareStage::Stage3_GifPath3Render2D,&engine,state,bus,scratch);assert(HostAudio::played.empty());
  audio.shutdown(&engine);audio.shutdown(&engine);assert(HostAudio::loaded.empty()&&HostAudio::resets==1);
  // A complete second lifecycle must not retain samples in SPU2.
  audio.init(&engine,state,bus);assert(HostAudio::loaded.size()==3);
  audio.shutdown(&engine);assert(HostAudio::loaded.empty()&&HostAudio::resets==2);
  // Optional asset failure must leave menus usable, without touching shared banks.
  HostAudio::failLoad=true;audio.init(&engine,state,bus);poll(Core::ScreenState::MainMenu,true,false);
  assert(HostAudio::played.empty()&&HostAudio::loaded.empty());audio.shutdown(&engine);
  HostAudio::failLoad=false;Tyra::FileUtils::root="missing-menu-audio/";
  audio.init(&engine,state,bus);poll(Core::ScreenState::MainMenu,true,false);assert(HostAudio::played.empty());audio.shutdown(&engine);
  assert(HostAudio::resets==2);
  std::cout<<"Menu audio passed: action mapping, settings, mute, busy voices, load failure and lifecycle\n";
}
