#pragma once

#include <tyra>
#include "core/isubsystem.hpp"
#include "systems/ui/ui_context_drawer.hpp"

namespace DroidCampos {
namespace Systems {
namespace Ui {

class MenuSystem : public Core::ISubsystem {
 public:
  explicit MenuSystem(UiContextDrawer* drawer = nullptr);
  explicit MenuSystem(UiContextDrawer& drawer);
  ~MenuSystem() override = default;

  void setDrawer(UiContextDrawer* drawer) { drawer_ = drawer; }

  const char* getName() const override { return "MenuSystem"; }

  void init(Tyra::Engine* engine, Core::WorldState& state,
            Events::EventBus& bus) override;
  void executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                    Core::WorldState& state, Events::EventBus& bus,
                    Hardware::Ps2ScratchpadArena& scratchpad) override;
  void shutdown(Tyra::Engine* engine) override;

 private:
  void renderMainMenu(const Core::WorldState& state);
  void renderNewGameScreen(const Core::WorldState& state);
  void renderLoadGameScreen(const Core::WorldState& state);
  void renderSettingsScreen(const Core::WorldState& state);

  UiContextDrawer* drawer_;
  Tyra::Timer galleryTimer_;
  u32 galleryTicks_ = 0;
  bool galleryActive_ = false;
};

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
