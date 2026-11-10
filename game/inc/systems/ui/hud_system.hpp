#pragma once

#include <tyra>
#include <cstdio>
#if defined(_EE)
#include <memory>
#endif
#include "core/isubsystem.hpp"
#include "systems/ui/ui_context_drawer.hpp"

namespace DroidCampos {
namespace Systems {
namespace Ui {

class HudSystem : public Core::ISubsystem {
 public:
  explicit HudSystem(UiContextDrawer* drawer = nullptr);
  explicit HudSystem(UiContextDrawer& drawer);
  ~HudSystem() override = default;

  void setDrawer(UiContextDrawer* drawer) { drawer_ = drawer; }

  const char* getName() const override { return "HudSystem"; }

  void init(Tyra::Engine* engine, Core::WorldState& state,
            Events::EventBus& bus) override;
  void executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                    Core::WorldState& state, Events::EventBus& bus,
                    Hardware::Ps2ScratchpadArena& scratchpad) override;
  void shutdown(Tyra::Engine* engine) override;

 private:
  void renderFpsHud(const Core::WorldState& state);
  void renderTerminal(const Core::WorldState& state);

#if defined(_EE)
  bool renderTerminalVideo(Tyra::Engine* engine, Core::WorldState& state);
  void closeTerminalVideo(Tyra::Engine* engine);

  std::FILE* videoFile_ = nullptr;
  Tyra::Texture* videoTexture_ = nullptr;
  Tyra::Sprite videoSprite_;
  Tyra::Timer videoTimer_;
  u32 videoTicks_ = 0;
  int videoFrame_ = -1;
  int videoFrameCount_ = 0;
  bool videoAudioLoaded_ = false;
  std::unique_ptr<u16[]> videoPixels_;
#endif

  UiContextDrawer* drawer_;
};

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
