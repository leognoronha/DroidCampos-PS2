#include "systems/audio/menu_audio_system.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

namespace DroidCampos {
namespace Systems {
namespace Audio {
namespace {
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

bool MenuAudioSystem::loadSample(const char* path, audsrv_adpcm_t& sample, u8*& data) {
  const auto fullPath = Tyra::FileUtils::fromCwd(path);
  FILE* file = std::fopen(fullPath.c_str(), "rb");
  if (!file) {
    std::printf("[MENU AUDIO] Missing sample: %s\n", path);
    return false;
  }
  if (std::fseek(file, 0, SEEK_END) != 0) { std::fclose(file); return false; }
  const long size = std::ftell(file);
  // Menu effects are small mono one-shots; bound allocations and DMA alignment.
  if (size < 32 || size > 128 * 1024 || size % 16 != 0) {
    std::fclose(file);
    return false;
  }
  std::rewind(file);
  data = allocateSampleData(static_cast<size_t>(size));
  const bool read = data && std::fread(data, 1, size, file) == static_cast<size_t>(size);
  std::fclose(file);
  const bool valid = read && std::memcmp(data, "APCM", 4) == 0 &&
      data[4] == 1 && data[5] == 1 && data[6] == 0;
  if (!valid || audsrv_load_adpcm(&sample, data, static_cast<int>(size)) != 0) {
    std::printf("[MENU AUDIO] Invalid or failed sample: %s\n", path);
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
  initialized_ = true;
}

void MenuAudioSystem::executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                                   Core::WorldState& state, Events::EventBus& bus,
                                   Hardware::Ps2ScratchpadArena&) {
  if (stage != Core::HardwareStage::Stage0_IopInputPoll || !engine || !initialized_) return;
  const int volume = std::max(0, std::min(100, state.menu.sfxVolume));
  if (volume != volume_) {
    for (int channel = 0; channel < 24; ++channel)
      engine->audio.adpcm.setVolume(static_cast<u8>(volume), static_cast<s8>(channel));
    volume_ = volume;
  }
  if (volume == 0) return;
  u32 count = 0;
  const auto* events = bus.getFrameEvents(count);
  for (u32 i = 0; i < count; ++i) {
    audsrv_adpcm_t* sample = nullptr;
    if (events[i].type == Events::EventType::MenuConfirmRequested && confirmLoaded_)
      sample = &confirm_;
    else if (events[i].type == Events::EventType::MenuBackRequested && backLoaded_)
      sample = &back_;
    else if (events[i].type == Events::EventType::MenuNavigateRequested && navigateLoaded_)
      sample = &navigate_;
    // Free-channel playback never blocks input, even during rapid presses.
    if (sample) engine->audio.adpcm.tryPlay(sample);
  }
}

void MenuAudioSystem::shutdown(Tyra::Engine* engine) {
  // This SDK has no per-sample free API. The menu owns all current game ADPCM
  // samples; reset its bank only at game shutdown, before releasing EE buffers.
  // reset() also stops active voices. Never call it on a menu screen transition.
  if (engine && (backLoaded_ || confirmLoaded_ || navigateLoaded_)) engine->audio.adpcm.reset();
  freeSampleData(navigateData_);
  freeSampleData(backData_);
  freeSampleData(confirmData_);
  navigateData_ = backData_ = confirmData_ = nullptr;
  navigateLoaded_ = backLoaded_ = confirmLoaded_ = initialized_ = false;
  navigate_ = {};
  back_ = {};
  confirm_ = {};
  volume_ = -1;
}

}  // namespace Audio
}  // namespace Systems
}  // namespace DroidCampos
