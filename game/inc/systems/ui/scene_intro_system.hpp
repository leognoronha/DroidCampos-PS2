#pragma once
#include "core/isubsystem.hpp"
#include "systems/ui/ui_context_drawer.hpp"

namespace DroidCampos {
namespace Systems {
namespace Ui {

class SceneIntroSystem final : public Core::ISubsystem {
 public:
  explicit SceneIntroSystem(UiContextDrawer* drawer) : drawer_(drawer) {}
  const char* getName() const override { return "SceneIntroSystem"; }
  void init(Tyra::Engine*, Core::WorldState&, Events::EventBus&) override { timer_.prime(); }
  void executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                    Core::WorldState& state, Events::EventBus& bus,
                    Hardware::Ps2ScratchpadArena& scratchpad) override;
  void shutdown(Tyra::Engine*) override {}
 private:
  UiContextDrawer* drawer_;
  Tyra::Timer timer_;
};

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
