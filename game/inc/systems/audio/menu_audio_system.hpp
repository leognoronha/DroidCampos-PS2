#pragma once
#include "core/isubsystem.hpp"
#include <audsrv.h>

namespace DroidCampos {
namespace Systems {
namespace Audio {

class MenuAudioSystem final : public Core::ISubsystem {
 public:
  const char* getName() const override { return "MenuAudioSystem"; }
  void init(Tyra::Engine* engine, Core::WorldState& state, Events::EventBus& bus) override;
  void executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                    Core::WorldState& state, Events::EventBus& bus,
                    Hardware::Ps2ScratchpadArena& scratchpad) override;
  void shutdown(Tyra::Engine* engine) override;

 private:
  bool loadSample(const char* path, audsrv_adpcm_t& sample, u8*& data, bool loop = false);
  struct Loop {
    audsrv_adpcm_t sample{};
    u8* data = nullptr;
    bool loaded = false;
    bool playing = false;
    int volume = -1;
  };
  void updateLoop(Tyra::Engine* engine, Loop& loop, s8 channel, int volume);
  Loop ambience_;
  Loop menuMusic_;
  Loop droidAlarm_;
  Loop droidSteps_;
  audsrv_adpcm_t confirm_{};
  audsrv_adpcm_t back_{};
  void updateRadio(Tyra::Engine* engine, Core::WorldState& state);
  audsrv_adpcm_t navigate_{};
  audsrv_adpcm_t radioAlert_{};
  audsrv_adpcm_t radioConfirmation_{};
  audsrv_adpcm_t weaponShot_{}, weaponReload_{};
  u8* weaponShotData_ = nullptr;
  u8* weaponReloadData_ = nullptr;
  bool weaponShotLoaded_ = false, weaponReloadLoaded_ = false;
  s8 nextWeaponChannel_ = 10;
  s8 nextUiChannel_ = 0;
  Tyra::Timer radioTimer_;
  u32 radioTicksRemaining_ = 0;
  u32 radioAlertTicks_ = 0;
  u8* confirmData_ = nullptr;
  u8* backData_ = nullptr;
  u8* navigateData_ = nullptr;
  u8* radioAlertData_ = nullptr;
  u8* radioConfirmationData_ = nullptr;
  bool confirmLoaded_ = false;
  bool backLoaded_ = false;
  bool navigateLoaded_ = false;
  bool radioAlertLoaded_ = false;
  bool radioConfirmationLoaded_ = false;
  bool initialized_ = false;
  int volume_ = -1;
};

}  // namespace Audio
}  // namespace Systems
}  // namespace DroidCampos
