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
  bool loadSample(const char* path, audsrv_adpcm_t& sample, u8*& data);
  audsrv_adpcm_t confirm_{};
  audsrv_adpcm_t back_{};
  audsrv_adpcm_t navigate_{};
  u8* confirmData_ = nullptr;
  u8* backData_ = nullptr;
  u8* navigateData_ = nullptr;
  bool confirmLoaded_ = false;
  bool backLoaded_ = false;
  bool navigateLoaded_ = false;
  bool initialized_ = false;
  int volume_ = -1;
};

}  // namespace Audio
}  // namespace Systems
}  // namespace DroidCampos
