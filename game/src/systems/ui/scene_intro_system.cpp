#include "systems/ui/scene_intro_system.hpp"
#include "core/scene_intro.hpp"
#include <string>

namespace DroidCampos {
namespace Systems {
namespace Ui {

void SceneIntroSystem::executeStage(Core::HardwareStage stage, Tyra::Engine*,
                                    Core::WorldState& state, Events::EventBus&,
                                    Hardware::Ps2ScratchpadArena&) {
  if (stage == Core::HardwareStage::Stage1_EeVu0Simulation) {
    const u32 elapsed = timer_.getTimeDelta();
    timer_.prime();
    if (!state.intro.active || !state.scene.resident || !state.scene.presented ||
        state.scene.requested || state.menu.currentScreen != Core::ScreenState::GameplayFps) return;
    const u32 duration = Core::introDurationTicks(state.scene.active);
    state.intro.elapsedTicks = std::min(duration, state.intro.elapsedTicks + elapsed);
    if (state.intro.elapsedTicks >= duration) state.intro.active = false;
  } else if (stage == Core::HardwareStage::Stage3_GifPath3Render2D && drawer_ &&
             state.intro.active && state.scene.resident &&
             state.menu.currentScreen == Core::ScreenState::GameplayFps) {
    // Registered last: this overlay covers the complete 3D scene and HUD.
    const float alpha = Core::introBlackAlpha(state.intro.elapsedTicks);
    if (alpha > 0) drawer_->drawRect(0, 0, 512, 448, Tyra::Color(0, 0, 0, alpha));
    if (state.scene.active != Core::SceneId::Exterior) return;
    const int visible = Core::introVisibleCharacters(state.intro.elapsedTicks);
    const float textAlpha = Core::introTextAlpha(state.intro.elapsedTicks);
    if (visible > 0 && textAlpha > 0) {
      const Tyra::Color cyan(80, 120, 116, textAlpha);
      drawer_->drawRect(34, 292, 172, 1, Tyra::Color(27, 56, 58, textAlpha));
      drawer_->getFont().drawText(std::string(Core::kIntroLocationText, visible), 34, 304, cyan);
    }
  }
}

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
