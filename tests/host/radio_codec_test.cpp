#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include "systems/audio/menu_audio_system.hpp"
#include "systems/input/input_system.hpp"
#include "systems/narrative/radio_codec_system.hpp"
using namespace DroidCampos;
int main(int argc,char**argv) {
  assert(argc>1);Tyra::FileUtils::root=argv[1];
  Core::WorldState state;state.resetToDefaults();
  state.menu.ambientVolume = 0;
  state.menu.bgmVolume = 0;
  state.menu.currentScreen=Core::ScreenState::GameplayFps;
  state.scene.resident=true;state.scene.presented=true;
  Tyra::Engine engine;Events::EventBus bus;Hardware::Ps2ScratchpadArena scratch;
  Systems::Input::InputSystem input;Systems::Audio::MenuAudioSystem audio;
  Systems::Ui::UiContextDrawer drawer;Systems::Narrative::RadioCodecSystem radio(drawer);
  radio.init(&engine,state,bus);audio.init(&engine,state,bus);
  assert(HostAudio::loaded.size()==9);
  audsrv_adpcm_t* alert=nullptr; audsrv_adpcm_t* confirmation=nullptr;
  for(auto& entry:HostAudio::loaded) {
    if(entry.second==32000)alert=entry.first;
    if(entry.second==17120)confirmation=entry.first;
  }
  assert(alert&&confirmation);
  auto read32=[](const unsigned char* p)->uint32_t{return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);};
  const uint32_t pitch=read32(static_cast<unsigned char*>(alert->buffer)+8);
  const uint32_t samples=static_cast<uint32_t>((alert->size-16)/16)*28;
  const u32 duration=static_cast<u32>((uint64_t(samples)*4096*15625+uint64_t(pitch)*48000-1)/(uint64_t(pitch)*48000));
  auto stage0=[&](){input.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&engine,state,bus,scratch);audio.executeStage(Core::HardwareStage::Stage0_IopInputPoll,&engine,state,bus,scratch);};
  auto step=[&](u32 ticks){Tyra::Timer::ticks+=ticks;radio.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,state,bus,scratch);audio.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&engine,state,bus,scratch);};
  auto render=[&](const char* name,bool icon) {
    auto& draws=engine.renderer.renderer2D.draws;draws.clear();
    radio.executeStage(Core::HardwareStage::Stage3_GifPath3Render2D,&engine,state,bus,scratch);
    assert(!draws.empty());
    for(const auto& s:draws) {
      assert(s.size.x>0&&s.size.y>0);
      assert(s.position.x>=18&&s.position.x+s.size.x*s.scale<=494);
      assert(s.position.y>= (icon?52:226) &&s.position.y+s.size.y*s.scale<=(icon?86:402));
      if(icon)assert(s.position.x+s.size.x*s.scale<=50);
    }
    if(argc>2) {
      std::ofstream out(std::string(argv[2])+"/"+name+".txt");
      for(auto& s:draws){auto* tex=engine.renderer.repo.getByMeshMaterialId(s.id);assert(tex);
        out<<tex->path<<" "<<s.position.x<<" "<<s.position.y<<" "<<s.size.x<<" "<<s.size.y<<" "<<s.offset.x<<" "<<s.offset.y<<" "<<s.scale<<" "<<s.color.r<<" "<<s.color.g<<" "<<s.color.b<<" "<<s.color.a<<"\n";
      }
    }
  };
  stage0();step(0);
  assert(state.radio.phase==Core::RadioPhase::Alert&&state.radio.visibleChars==0);
  assert(HostAudio::played.size()==1&&HostAudio::played[0]==alert);
  render("radio_alert",true);
  bus.flushFrameEvents();step(duration-1);
  assert(state.radio.phase==Core::RadioPhase::Alert&&HostAudio::played.size()==1);
  step(1);
  assert(state.radio.phase==Core::RadioPhase::Message&&state.radio.visibleChars==0);
  assert(HostAudio::played.size()==2&&HostAudio::played[1]==confirmation);
  bus.flushFrameEvents();
  const int firstLine = static_cast<int>(std::strchr(state.radio.currentMessage, '\n') -
                                         state.radio.currentMessage);
  for (int i = 0; i < 200 && state.radio.visibleChars < firstLine; ++i) step(500);
  assert(state.radio.visibleChars == firstLine);
  step(7031);
  assert(state.radio.visibleChars == firstLine);
  step(1);
  assert(state.radio.visibleChars == firstLine + 2);
  for (int i = 0; i < 1000; ++i) step(500);
  assert(state.radio.visibleChars==static_cast<int>(std::strlen(state.radio.currentMessage)));
  assert(HostAudio::played.size()==2);render("radio_message",false);
  engine.pad.clicked.Circle=true;bus.flushFrameEvents();stage0();step(0);
  assert(!state.radio.popupActive&&state.radio.phase==Core::RadioPhase::Idle);
  assert(HostAudio::played.size()==3 && HostAudio::played.back()==confirmation);
  engine.pad.clicked={};engine.pad.clicked.L2=true;bus.flushFrameEvents();stage0();step(0);
  assert(state.radio.popupActive && state.radio.phase==Core::RadioPhase::Message);
  assert(state.radio.visibleChars==static_cast<int>(std::strlen(state.radio.currentMessage)));
  assert(HostAudio::played.size()==3); // Manual review never plays alert/confirmation.
  engine.pad.clicked={};engine.pad.clicked.Circle=true;bus.flushFrameEvents();stage0();step(0);
  assert(!state.radio.popupActive && HostAudio::played.size()==4);
  engine.pad.clicked={};bus.flushFrameEvents();
  engine.renderer.renderer2D.draws.clear();radio.executeStage(Core::HardwareStage::Stage3_GifPath3Render2D,&engine,state,bus,scratch);assert(engine.renderer.renderer2D.draws.empty());
  // New messages restart the sequence and carry their own text.
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::RadioTransmissionTriggered,0,0,"CUSTOM RADIO MESSAGE"));step(0);
  assert(state.radio.phase==Core::RadioPhase::Alert&&std::strcmp(state.radio.currentMessage,"CUSTOM RADIO MESSAGE")==0);
  engine.pad.clicked.Circle=true;bus.flushFrameEvents();stage0();step(0);
  assert(state.radio.phase==Core::RadioPhase::Alert); // O cannot close an unopened message.
  engine.pad.clicked={};bus.flushFrameEvents();state.menu.currentScreen=Core::ScreenState::Settings;
  const auto plays=HostAudio::played.size();step(duration);
  assert(state.radio.phase==Core::RadioPhase::Alert&&HostAudio::played.size()==plays);
  state.menu.currentScreen=Core::ScreenState::GameplayFps;step(0);
  assert(state.radio.phase==Core::RadioPhase::Message&&HostAudio::played.back()==confirmation);
  // Mute bypasses the wait and never plays either radio clip.
  state.menu.sfxVolume=0;bus.flushFrameEvents();stage0();HostAudio::played.clear();
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::RadioTransmissionTriggered));step(0);
  assert(state.radio.phase==Core::RadioPhase::Message&&HostAudio::played.empty());
  state.scene.active=Core::SceneId::Interior;state.resetPlayerToSpawn();
  assert(state.radio.popupActive&&state.radio.phase==Core::RadioPhase::Waiting);
  state.radio.popupActive=false;state.radio.phase=Core::RadioPhase::Idle;
  engine.pad.clicked={};engine.pad.clicked.L2=true;bus.flushFrameEvents();stage0();step(0);
  assert(state.radio.popupActive && state.radio.phase==Core::RadioPhase::Message);
  assert(std::strstr(state.radio.currentMessage,"DENTRO DA ETIP"));
  assert(HostAudio::played.empty()); // Interior review also stays silent, even muted.
  render("radio_interior",false);
  audio.shutdown(&engine);assert(HostAudio::loaded.empty());
  Tyra::FileUtils::root="missing-radio-assets/";audio.init(&engine,state,bus);
  bus.flushFrameEvents();state.radio.phase=Core::RadioPhase::Waiting;state.menu.sfxVolume=90;
  step(0);assert(state.radio.phase==Core::RadioPhase::Message);audio.shutdown(&engine);
  drawer.shutdown(&engine);assert(engine.renderer.repo.getTexturesCount()==0);
  std::cout<<"PASS: alert/icon then confirmation/message, exact timing, no repeats, dismissal, pause, mute, missing assets, interior and UI bounds\n";
}
