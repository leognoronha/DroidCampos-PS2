#pragma once

#include <tyra>
#include "core/isubsystem.hpp"
#include "systems/ui/ui_context_drawer.hpp"

namespace DroidCampos {
namespace Systems {
namespace Narrative {

class RadioCodecSystem : public Core::ISubsystem {
 public:
  explicit RadioCodecSystem(Ui::UiContextDrawer* drawer = nullptr);
  explicit RadioCodecSystem(Ui::UiContextDrawer& drawer);
  ~RadioCodecSystem() override = default;

  void setDrawer(Ui::UiContextDrawer* drawer) { drawer_ = drawer; }

  const char* getName() const override { return "RadioCodecSystem"; }

  void init(Tyra::Engine* engine, Core::WorldState& state,
            Events::EventBus& bus) override;
  void executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                    Core::WorldState& state, Events::EventBus& bus,
                    Hardware::Ps2ScratchpadArena& scratchpad) override;
  void shutdown(Tyra::Engine* engine) override;

 private:
  void updateSimulation(Core::WorldState& state, Events::EventBus& bus);
  void renderIncomingIndicator();
  void renderRadioPopup(const Core::WorldState& state);
  void renderSignalMonitor(float x, float y, bool speaking);

  Ui::UiContextDrawer* drawer_;
  Tyra::Timer revealTimer_;
  float currentAnimTick_;
};

}  // namespace Narrative
}  // namespace Systems
}  // namespace DroidCampos
