#include "systems/audio/menu_audio_system.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <malloc.h>

namespace DroidCampos {
namespace Systems {
namespace Audio {
namespace {
constexpr s8 kMenuMusicChannel = 5;
constexpr s8 kAmbienceChannel = 23;

u8* allocateSampleData(size_t size) {
#ifdef _WIN32
  return static_cast<u8*>(_aligned_malloc(size, 64));
#else
  return static_cast<u8*>(memalign(64, size));
#endif
}
void freeSampleData(u8* data) {
#ifdef _WIN32
  _aligned_free(data);
#else
  std::free(data);
#endif
}
}  // namespace

bool MenuAudioSystem::loadSample(const char* path, audsrv_adpcm_t& sample, u8*& data, bool loop) {
  const auto fullPath = Tyra::FileUtils::fromCwd(path);
  FILE* file = std::fopen(fullPath.c_str(), "rb");
  if (!file) {
    std::printf("[UI AUDIO] Missing sample: %s\n", path);
    return false;
  }
  if (std::fseek(file, 0, SEEK_END) != 0) { std::fclose(file); return false; }
  const long size = std::ftell(file);
  const long maxSize = (loop ? 384 : 128) * 1024;
  if (size < 32 || size > maxSize || size % 16 != 0) {
    std::fclose(file);
    return false;
  }
  std::rewind(file);
  data = allocateSampleData(static_cast<size_t>(size));
  const bool read = data && std::fread(data, 1, size, file) == static_cast<size_t>(size);
  std::fclose(file);
  const bool valid = read && std::memcmp(data, "APCM", 4) == 0 &&
      data[4] == 1 && data[5] == 1 && data[6] == static_cast<u8>(loop);
  if (!valid || audsrv_load_adpcm(&sample, data, static_cast<int>(size)) != 0) {
    std::printf("[UI AUDIO] Invalid or failed sample: %s\n", path);
    freeSampleData(data);
    data = nullptr;
    return false;
  }
  return true;
}

void MenuAudioSystem::init(Tyra::Engine* engine, Core::WorldState&, Events::EventBus&) {
  if (!engine || initialized_) return;
  // The engine already initializes AUDSRV. Do not reset its shared sample bank.
  confirmLoaded_ = loadSample("audio/menu_confirm.adpcm", confirm_, confirmData_);
  backLoaded_ = loadSample("audio/menu_back.adpcm", back_, backData_);
  navigateLoaded_ = loadSample("audio/menu_navigate.adpcm", navigate_, navigateData_);
  radioAlertLoaded_ = loadSample("audio/radio_alert.adpcm", radioAlert_, radioAlertData_);
  radioConfirmationLoaded_ = loadSample("audio/radio_confirmation.adpcm", radioConfirmation_, radioConfirmationData_);
  weaponShotLoaded_ = loadSample("audio/weapon_shot.adpcm", weaponShot_, weaponShotData_);
  weaponReloadLoaded_ = loadSample("audio/weapon_reload.adpcm", weaponReload_, weaponReloadData_);
  ambience_.loaded = loadSample("audio/etip_ambient.adpcm", ambience_.sample, ambience_.data, true);
  menuMusic_.loaded = loadSample("audio/menu_bgm.adpcm", menuMusic_.sample, menuMusic_.data, true);
  if (radioAlertLoaded_) {
    // APCM stores little-endian source samples and the actual SPU2 playback pitch.
    auto read32 = [](const u8* p) -> uint32_t {
      return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
             (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
    };
    const uint32_t pitch = read32(radioAlertData_ + 8);
    // Include the encoder's padded tail/end blocks as well as source samples.
    const uint32_t samples = static_cast<uint32_t>((radioAlert_.size - 16) / 16) * 28;
    // Tyra Timer uses 15625 ticks/s, independently of the render frame rate.
    if (pitch > 0) radioAlertTicks_ = static_cast<u32>(
        (uint64_t(samples) * 4096 * 15625 + uint64_t(pitch) * 48000 - 1) /
        (uint64_t(pitch) * 48000));
  }
  radioTimer_.prime();
  initialized_ = true;
}

void MenuAudioSystem::executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                                   Core::WorldState& state, Events::EventBus& bus,
                                   Hardware::Ps2ScratchpadArena&) {
  if (!engine || !initialized_) return;
  if (stage == Core::HardwareStage::Stage1_EeVu0Simulation) {
    // Weapon events are emitted in simulation, after input and player movement.
    if ((state.menu.currentScreen == Core::ScreenState::GameplayFps ||
         state.menu.currentScreen == Core::ScreenState::Defeated) &&
        state.menu.sfxVolume > 0) {
      u32 count = 0;
      const auto* events = bus.getFrameEvents(count);
      for (u32 i = 0; i < count; ++i) {
        if (events[i].type == Events::EventType::WeaponFired &&
            weaponShotLoaded_) {
          // 11 voices cover the full 2.598s tail at 4 shots/sec; never
          // wait/retry.
          if(events[i].intParam>=0)
            engine->audio.adpcm.tryPlay(&weaponShot_,6+events[i].intParam%4);
          else {
            engine->audio.adpcm.tryPlay(&weaponShot_, nextWeaponChannel_);
            nextWeaponChannel_ = nextWeaponChannel_ == 20 ? 10 : nextWeaponChannel_ + 1;
          }
        } else if (events[i].type == Events::EventType::WeaponReloadStarted &&
                   weaponReloadLoaded_)
          engine->audio.adpcm.tryPlay(&weaponReload_,events[i].intParam>=0?6+events[i].intParam%4:21);
      }
    }
    const auto screen = state.menu.currentScreen;
    const auto& scene = state.scene;
    const bool inside = screen == Core::ScreenState::GameplayFps &&
        scene.active == Core::SceneId::Interior && scene.resident && scene.presented &&
        !scene.requested && scene.accessFrames == 0;
    const bool inMainMenu = screen == Core::ScreenState::MainMenu ||
        screen == Core::ScreenState::NewGame || screen == Core::ScreenState::LoadGame ||
        (screen == Core::ScreenState::Settings &&
         state.menu.settingsReturnScreen == Core::ScreenState::MainMenu);
    updateLoop(engine, ambience_, kAmbienceChannel, inside ? state.menu.ambientVolume : 0);
    updateLoop(engine, menuMusic_, kMenuMusicChannel, inMainMenu ? state.menu.bgmVolume : 0);
    updateRadio(engine, state);
    return;
  }
  if (stage != Core::HardwareStage::Stage0_IopInputPoll) return;
  const int volume = std::max(0, std::min(100, state.menu.sfxVolume));
  if (volume != volume_) {
    for (int channel = 0; channel < kAmbienceChannel; ++channel) {
      if (channel != kMenuMusicChannel)
        engine->audio.adpcm.setVolume(static_cast<u8>(volume), static_cast<s8>(channel));
    }
    volume_ = volume;
  }
  if (volume == 0) return;
  u32 count = 0;
  const auto* events = bus.getFrameEvents(count);
  for (u32 i = 0; i < count; ++i) {
    audsrv_adpcm_t* sample = nullptr;
    if (events[i].type == Events::EventType::MenuConfirmRequested &&
        confirmLoaded_)
      sample = &confirm_;
    else if (events[i].type == Events::EventType::MenuBackRequested &&
             backLoaded_)
      sample = &back_;
    else if (events[i].type == Events::EventType::MenuNavigateRequested &&
             navigateLoaded_)
      sample = &navigate_;
    else if (events[i].type == Events::EventType::RadioDismissRequested &&
             state.radio.popupActive &&
             state.radio.phase == Core::RadioPhase::Message &&
             radioConfirmationLoaded_)
      sample = &radioConfirmation_;
    // Non-blocking playback stays within the reserved channel pools.
    if (sample) {
      if (events[i].type == Events::EventType::RadioDismissRequested)
        engine->audio.adpcm.tryPlay(sample, 22);
      else {
        engine->audio.adpcm.tryPlay(sample, nextUiChannel_);
        nextUiChannel_ = nextUiChannel_ == kMenuMusicChannel - 1 ? 0 : nextUiChannel_ + 1;
      }
    }
  }
}

void MenuAudioSystem::updateLoop(Tyra::Engine* engine, Loop& loop, s8 channel, int volume) {
  if (!loop.loaded) return;
  volume = std::clamp(volume, 0, 100);
  if (volume != loop.volume) {
    engine->audio.adpcm.setVolume(static_cast<u8>(volume), channel);
    loop.volume = volume;
  }
  // The SDK has no per-voice stop. Silence the reserved loop when inactive.
  if (volume > 0 && !loop.playing)
    loop.playing = engine->audio.adpcm.tryPlay(&loop.sample, channel) == Tyra::ADPCM_OK;
}

void MenuAudioSystem::updateRadio(Tyra::Engine* engine,
                                  Core::WorldState& state) {
  const u32 elapsed = radioTimer_.getTimeDelta();
  radioTimer_.prime();
  auto& radio = state.radio;
  if (!radio.popupActive) {
    radioTicksRemaining_ = 0;
    return;
  }
  if (state.menu.currentScreen != Core::ScreenState::GameplayFps) {
    // The alert can finish while a menu is open, but its confirmation and
    // message wait for gameplay to resume. Do not replay the alert on return.
    if (radio.phase == Core::RadioPhase::Alert)
      radioTicksRemaining_ =
          elapsed < radioTicksRemaining_ ? radioTicksRemaining_ - elapsed : 0;
    return;
  }
  // GameplayFps alone is insufficient: input can request a scene before the
  // loader runs, and its first image may not yet have reached the screen.
  if (!state.scene.resident || !state.scene.presented ||
      state.scene.requested || state.scene.accessFrames > 0 ||
      state.intro.active)
    return;
  const bool muted = state.menu.sfxVolume <= 0;
  if (radio.phase == Core::RadioPhase::Waiting) {
    if (!muted && radioAlertLoaded_ && radioAlertTicks_ > 0 &&
        engine->audio.adpcm.tryPlay(&radioAlert_, 22) == Tyra::ADPCM_OK) {
      radioTicksRemaining_ = radioAlertTicks_;
      radio.phase = Core::RadioPhase::Alert;
      return;
    }
    // Missing/busy audio must not prevent access to a mission message.
  } else if (radio.phase == Core::RadioPhase::Alert) {
    if (!muted && elapsed < radioTicksRemaining_) {
      radioTicksRemaining_ -= elapsed;
      return;
    }
    // A ceil-rounded duration plus the next update keeps the two clips apart.
  } else {
    return;
  }
  if (!muted && radioConfirmationLoaded_)
    engine->audio.adpcm.tryPlay(&radioConfirmation_, 22);
  radio.phase = Core::RadioPhase::Message;
  radio.visibleChars = 0;
  radio.revealTicks = 0;
  radioTicksRemaining_ = 0;
}

void MenuAudioSystem::shutdown(Tyra::Engine* engine) {
  // This SDK has no per-sample free API. The menu owns all current game ADPCM
  // samples; reset its bank only at game shutdown, before releasing EE buffers.
  // reset() also stops active voices. Never call it on a menu screen transition.
  if (engine && (backLoaded_ || confirmLoaded_ || navigateLoaded_ ||
                 radioAlertLoaded_ || radioConfirmationLoaded_ || weaponShotLoaded_ ||
                 weaponReloadLoaded_ || ambience_.loaded || menuMusic_.loaded))
    engine->audio.adpcm.reset();
  freeSampleData(ambience_.data);
  freeSampleData(menuMusic_.data);
  ambience_ = {};
  menuMusic_ = {};
  freeSampleData(weaponShotData_);freeSampleData(weaponReloadData_);
  weaponShotData_ = weaponReloadData_ = nullptr;
  weaponShotLoaded_ = weaponReloadLoaded_ = false;
  weaponShot_ = {};weaponReload_ = {};nextWeaponChannel_ = 10;nextUiChannel_ = 0;
  freeSampleData(radioConfirmationData_);
  freeSampleData(radioAlertData_);
  freeSampleData(navigateData_);
  freeSampleData(backData_);
  freeSampleData(confirmData_);
  radioAlertData_ = radioConfirmationData_ = navigateData_ = backData_ = confirmData_ = nullptr;
  radioAlertLoaded_ = radioConfirmationLoaded_ = navigateLoaded_ = backLoaded_ = confirmLoaded_ = initialized_ = false;
  radioAlert_ = {};
  radioConfirmation_ = {};
  radioTicksRemaining_ = radioAlertTicks_ = 0;
  navigate_ = {};
  back_ = {};
  confirm_ = {};
  volume_ = -1;
}

}  // namespace Audio
}  // namespace Systems
}  // namespace DroidCampos
