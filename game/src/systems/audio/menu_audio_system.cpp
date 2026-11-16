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
constexpr s8 kDroidAlarmChannel = 4;
constexpr s8 kDroidStepsChannel = 3;
constexpr s8 kTerminalVideoChannel = 19;
constexpr s8 kDroidVoiceChannel = 20;
constexpr s8 kRadioChannel = 22;
constexpr s8 kAmbienceChannel = 23;
// Enemy voices are idle in the exterior, where the reader is used.
constexpr s8 kCardReaderChannel = 6;

u32 playbackTicks(const u8* data, int size) {
  const uint32_t pitch = uint32_t(data[8]) | (uint32_t(data[9]) << 8) |
                         (uint32_t(data[10]) << 16) | (uint32_t(data[11]) << 24);
  if (!pitch) return 0;
  // Include the padded tail and round up at the actual SPU2 pitch.
  const uint64_t samples = (size - 16) / 16 * 28;
  const uint64_t rate = uint64_t(pitch) * 48000;
  return static_cast<u32>((samples * 4096 * 15625 + rate - 1) / rate);
}

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

bool MenuAudioSystem::loadSample(const char* path, audsrv_adpcm_t& sample, u8*& data,
                                 bool loop, u32* duration, long maxBytes) {
  const auto fullPath = Tyra::FileUtils::fromCwd(path);
  FILE* file = std::fopen(fullPath.c_str(), "rb");
  if (!file) {
    std::printf("[UI AUDIO] Missing sample: %s\n", path);
    return false;
  }
  if (std::fseek(file, 0, SEEK_END) != 0) { std::fclose(file); return false; }
  const long size = std::ftell(file);
  const long maxSize = maxBytes > 0 ? maxBytes : (loop ? 384 : 128) * 1024;
  if (size < 32 || size > maxSize || size % 16 != 0) {
    std::fclose(file);
    return false;
  }
  std::rewind(file);
  // AUDSRV transfers the APCM payload to SPU2 via DMA. Pad the payload
  // independently of the 16-byte header to complete the final DMA unit.
  constexpr int headerSize = 16;
  constexpr int dmaUnit = 64;
  const int transferSize = headerSize + ((size - headerSize + dmaUnit - 1) & -dmaUnit);
  data = allocateSampleData(transferSize);
  const bool read = data && std::fread(data, 1, size, file) == static_cast<size_t>(size);
  std::fclose(file);
  const bool valid = read && std::memcmp(data, "APCM", 4) == 0 &&
      data[4] == 1 && data[5] == 1 && data[6] == static_cast<u8>(loop);
  if (valid) {
    // Keep one-shot samples from re-entering the following SPU2 allocation.
    // adpenc appends a silent repeat marker even to non-looping samples.
    if (!loop) {
      for (int block = headerSize; block < size - 16; block += 16) data[block + 1] &= 1;
      data[size - 15] = 1;
    }
    std::memset(data + size, 0, transferSize - size);
#ifdef _EE
    SyncDCache(data, data + transferSize);
#endif
  }
  if (!valid || audsrv_load_adpcm(&sample, data, transferSize) != 0) {
    std::printf("[UI AUDIO] Invalid or failed sample: %s\n", path);
    freeSampleData(data);
    data = nullptr;
    return false;
  }
  if (duration) *duration = playbackTicks(data, static_cast<int>(size));
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
  droidAlarm_.loaded = loadSample("audio/droid_alarm.adpcm", droidAlarm_.sample, droidAlarm_.data, true);
  droidSteps_.loaded = loadSample("audio/droid_steps.adpcm", droidSteps_.sample, droidSteps_.data, true);
  loadSample("audio/droid_ce_sabe.adpcm", droidVoice_.sample, droidVoice_.data);
  secretConfirmationLoaded_ =
      loadSample("audio/terminal_secret.adpcm", secretConfirmation_, secretConfirmationData_);
  loadSample("audio/card_reader.adpcm", cardReader_.sample, cardReader_.data,
             false, &cardReaderTicks_);
  loadSample("audio/pickup_weapon.adpcm", pickupWeapon_.sample, pickupWeapon_.data);
  loadSample("audio/pickup_medkit.adpcm", pickupMedkit_.sample, pickupMedkit_.data);
  loadSample("audio/impact_punch.adpcm", punchImpact_.sample, punchImpact_.data);
  loadSample("audio/heal_up.adpcm", healUp_.sample, healUp_.data);
  loadSample("audio/game_over.adpcm", gameOver_.sample, gameOver_.data);
  loadSample("video/claudio.adpcm", terminalVideo_.sample, terminalVideo_.data,
             false, &terminalVideoTicks_, 320 * 1024);
  if (radioAlertLoaded_) {
    radioAlertTicks_ = playbackTicks(radioAlertData_, radioAlert_.size);
  }
  radioTimer_.prime();
  initialized_ = true;
}

u32 MenuAudioSystem::playTerminalVideoAudio(Tyra::Engine* engine, int volume) {
  if (!terminalVideo_.data) return 0;
  if (volume <= 0) return terminalVideoTicks_;
  if (engine->audio.adpcm.tryPlay(&terminalVideo_.sample, kTerminalVideoChannel) !=
      Tyra::ADPCM_OK) return 0;
  engine->audio.adpcm.setVolume(static_cast<u8>(std::clamp(volume, 0, 100)), kTerminalVideoChannel);
  terminalVideoPlaying_ = true;
  return terminalVideoTicks_;
}

void MenuAudioSystem::stopTerminalVideoAudio(Tyra::Engine* engine, bool interrupted) {
  if (!terminalVideoPlaying_) return;
  engine->audio.adpcm.setVolume(0, kTerminalVideoChannel);
  terminalVideoPlaying_ = false;
  // This AUDSRV has no per-voice stop. Reload the bank after an interruption
  // so the dedicated channel is available on an immediate replay.
  reloadBank_ = interrupted;
}

u32 MenuAudioSystem::playCardReader(Tyra::Engine* engine, int volume) {
  constexpr u32 fallbackTicks = 5209; // Original one-third-second LED cue.
  if (!cardReader_.data) return fallbackTicks;
  if (volume > 0) engine->audio.adpcm.tryPlay(&cardReader_.sample, kCardReaderChannel);
  return std::max(fallbackTicks, cardReaderTicks_);
}

void MenuAudioSystem::playCombatCue(Tyra::Engine* engine, Effect& effect) {
  if (!effect.data) return;
  for (int attempt = 0; attempt < kDroidStepsChannel; ++attempt) {
    const auto channel = nextUiChannel_;
    nextUiChannel_ = (channel + 1) % kDroidStepsChannel;
    if (engine->audio.adpcm.tryPlay(&effect.sample, channel) == Tyra::ADPCM_OK) return;
  }
}

void MenuAudioSystem::executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                                   Core::WorldState& state, Events::EventBus& bus,
                                   Hardware::Ps2ScratchpadArena&) {
  if (!engine || !initialized_) return;
  if (reloadBank_) {
    shutdown(engine);
    init(engine, state, bus);
  }
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
          // Cycle through the weapon voices without waiting for a free voice.
          if(events[i].intParam>=0)
            engine->audio.adpcm.tryPlay(&weaponShot_,6+events[i].intParam%4);
          else {
            engine->audio.adpcm.tryPlay(&weaponShot_, nextWeaponChannel_);
            nextWeaponChannel_ = nextWeaponChannel_ == kTerminalVideoChannel - 1 ? 10 : nextWeaponChannel_ + 1;
          }
        } else if (events[i].type == Events::EventType::WeaponReloadStarted &&
                   weaponReloadLoaded_) {
          engine->audio.adpcm.tryPlay(&weaponReload_,events[i].intParam>=0?6+events[i].intParam%4:21);
        } else if (events[i].type == Events::EventType::DroidPunchLanded) {
          playCombatCue(engine, punchImpact_);
        } else if (events[i].type == Events::EventType::MedkitUsed) {
          playCombatCue(engine, healUp_);
        } else if (events[i].type == Events::EventType::PlayerDefeated) {
          // Defeat can coincide with a punch. Try the reload voice first so
          // both sounds can play even when the UI voices are busy.
          if (gameOver_.data &&
              engine->audio.adpcm.tryPlay(&gameOver_.sample, 21) != Tyra::ADPCM_OK)
            playCombatCue(engine, gameOver_);
        }
      }
    }
    const auto screen = state.menu.currentScreen;
    const auto& scene = state.scene;
    const bool inside = screen == Core::ScreenState::GameplayFps &&
        scene.active == Core::SceneId::Interior && scene.resident && scene.presented &&
        !scene.requested && !scene.terminalVideoActive && scene.accessTicks == 0;
    const bool inMainMenu = screen == Core::ScreenState::MainMenu ||
        screen == Core::ScreenState::NewGame || screen == Core::ScreenState::LoadGame ||
        (screen == Core::ScreenState::Settings &&
         state.menu.settingsReturnScreen == Core::ScreenState::MainMenu);
    const bool encounterActive = scene.active == Core::SceneId::Interior &&
        state.simulationActive() &&
        (state.droid.phase == Core::DroidPhase::Release ||
         state.droid.phase == Core::DroidPhase::Pursuit);
    const bool walking = encounterActive && state.droid.moving;
    updateLoop(engine, droidAlarm_, kDroidAlarmChannel, encounterActive ? state.menu.sfxVolume : 0);
    updateLoop(engine, droidSteps_, kDroidStepsChannel, walking ? state.menu.sfxVolume : 0);
    updateDroidVoice(engine, state, encounterActive);
    updateLoop(engine, ambience_, kAmbienceChannel, inside ? state.menu.ambientVolume : 0);
    updateLoop(engine, menuMusic_, kMenuMusicChannel, inMainMenu ? state.menu.bgmVolume : 0);
    updateRadio(engine, state);
    return;
  }
  if (stage != Core::HardwareStage::Stage0_IopInputPoll) return;
  const int volume = std::max(0, std::min(100, state.menu.sfxVolume));
  if (volume != volume_) {
    for (int channel = 0; channel < kAmbienceChannel; ++channel) {
      if (channel != kMenuMusicChannel && channel != kDroidAlarmChannel &&
          channel != kDroidStepsChannel && channel != kDroidVoiceChannel &&
          channel != kTerminalVideoChannel && channel != kRadioChannel)
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
    else if (events[i].type == Events::EventType::WeaponPickedUp && pickupWeapon_.data)
      sample = &pickupWeapon_.sample;
    else if (events[i].type == Events::EventType::MedkitPickedUp && pickupMedkit_.data)
      sample = &pickupMedkit_.sample;
    else if (events[i].type == Events::EventType::TerminalSecretUnlocked &&
             secretConfirmationLoaded_)
      sample = &secretConfirmation_;
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
        playRadioCue(engine, *sample, radioConfirmationData_, volume);
      else {
        engine->audio.adpcm.tryPlay(sample, nextUiChannel_);
        nextUiChannel_ = nextUiChannel_ == kDroidStepsChannel - 1 ? 0 : nextUiChannel_ + 1;
      }
    }
  }
}

void MenuAudioSystem::updateDroidVoice(Tyra::Engine* engine, const Core::WorldState& state,
                                       bool encounterActive) {
  if (!droidVoice_.data) return;
  const auto& droid = state.droid;
  const bool pursuing = droid.phase == Core::DroidPhase::Pursuit;
  if (!pursuing) nextDroidVoiceTick_ = 0;

  // Keep the phrase below the footsteps and alarm in the SFX mix.
  const int volume = encounterActive && pursuing
      ? std::clamp(state.menu.sfxVolume, 0, 100) * 9 / 10 : 0;
  if (volume != droidVoiceVolume_) {
    engine->audio.adpcm.setVolume(static_cast<u8>(volume), kDroidVoiceChannel);
    droidVoiceVolume_ = volume;
  }

  if (volume > 0 && droid.phaseTicks >= nextDroidVoiceTick_) {
    engine->audio.adpcm.tryPlay(&droidVoice_.sample, kDroidVoiceChannel);
    nextDroidVoiceTick_ = droid.phaseTicks + 2 * Core::kDroidSecond;
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

bool MenuAudioSystem::playRadioCue(Tyra::Engine* engine, audsrv_adpcm_t& sample,
                                   const u8* data, int volume) {
  if (engine->audio.adpcm.tryPlay(&sample, kRadioChannel) != Tyra::ADPCM_OK) return false;
  engine->audio.adpcm.setVolume(static_cast<u8>(std::clamp(volume, 0, 100)), kRadioChannel);
  radioVoiceTicks_ = playbackTicks(data, sample.size);
  return true;
}

void MenuAudioSystem::updateRadio(Tyra::Engine* engine,
                                  Core::WorldState& state) {
  const u32 elapsed = radioTimer_.getTimeDelta();
  radioTimer_.prime();
  if (radioVoiceTicks_ > 0) {
    if (elapsed >= radioVoiceTicks_ || state.menu.sfxVolume <= 0) {
      // Silence a voice even if the physical SPU2 did not signal its end.
      engine->audio.adpcm.setVolume(0, kRadioChannel);
      radioVoiceTicks_ = 0;
    } else {
      radioVoiceTicks_ -= elapsed;
    }
  }
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
      state.scene.requested || state.scene.accessTicks > 0 ||
      state.intro.active)
    return;
  const bool muted = state.menu.sfxVolume <= 0;
  if (radio.phase == Core::RadioPhase::Waiting) {
    if (!muted && radioAlertLoaded_ && radioAlertTicks_ > 0 &&
        playRadioCue(engine, radioAlert_, radioAlertData_, state.menu.sfxVolume)) {
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
    playRadioCue(engine, radioConfirmation_, radioConfirmationData_, state.menu.sfxVolume);
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
                 weaponReloadLoaded_ || secretConfirmationLoaded_ || ambience_.loaded || menuMusic_.loaded || droidAlarm_.loaded ||
                 droidSteps_.loaded || droidVoice_.data || cardReader_.data || pickupWeapon_.data || pickupMedkit_.data ||
                 punchImpact_.data || healUp_.data || gameOver_.data || terminalVideo_.data))
    engine->audio.adpcm.reset();
  for (auto* effect : {&cardReader_, &pickupWeapon_, &pickupMedkit_,
                       &punchImpact_, &healUp_, &gameOver_, &terminalVideo_}) {
    freeSampleData(effect->data);
    *effect = {};
  }
  cardReaderTicks_ = terminalVideoTicks_ = 0;
  terminalVideoPlaying_ = reloadBank_ = false;
  freeSampleData(ambience_.data);
  freeSampleData(menuMusic_.data);
  freeSampleData(droidAlarm_.data);
  freeSampleData(droidSteps_.data);
  freeSampleData(droidVoice_.data);
  ambience_ = {};
  menuMusic_ = {};
  droidAlarm_ = {};
  droidSteps_ = {};
  droidVoice_ = {};
  droidVoiceVolume_ = -1;
  nextDroidVoiceTick_ = 0;
  freeSampleData(secretConfirmationData_);
  secretConfirmationData_ = nullptr;
  secretConfirmationLoaded_ = false;
  secretConfirmation_ = {};
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
  radioTicksRemaining_ = radioAlertTicks_ = radioVoiceTicks_ = 0;
  navigate_ = {};
  back_ = {};
  confirm_ = {};
  volume_ = -1;
}

}  // namespace Audio
}  // namespace Systems
}  // namespace DroidCampos
