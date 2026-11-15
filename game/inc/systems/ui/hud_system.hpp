#pragma once

#include <tyra>
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

  UiContextDrawer* drawer_;
};

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
