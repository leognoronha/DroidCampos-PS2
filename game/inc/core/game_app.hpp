#pragma once

#include <tyra>
#include "core/world_state.hpp"
#include "systems/audio/menu_audio_system.hpp"
#include "events/event_bus.hpp"
#include "hardware/ps2_scratchpad.hpp"
#include "hardware/ps2_task_scheduler.hpp"
#include "systems/input/input_system.hpp"
#include "systems/narrative/radio_codec_system.hpp"
#include "systems/player/player_controller_system.hpp"
#include "systems/render3d/world_render_system.hpp"
#include "systems/ui/hud_system.hpp"
#include "systems/ui/menu_system.hpp"
#include "systems/ui/scene_intro_system.hpp"
#include "systems/ui/ui_context_drawer.hpp"

namespace DroidCampos {
namespace Core {

class GameApp final : public Tyra::Game {
 public:
  explicit GameApp(Tyra::Engine* engine);
  ~GameApp();

  void init() override;
  void loop() override;

 private:
  void updateSceneTransition();
  void advanceLoading();
  void logSceneMemory(const char* phase);
  Tyra::Engine* engine_;
  WorldState worldState_;
  Events::EventBus eventBus_;
  Hardware::Ps2ScratchpadArena scratchpad_;
  Hardware::Ps2TaskScheduler scheduler_;
  Systems::Ui::UiContextDrawer uiDrawer_;
  Systems::Input::InputSystem inputSystem_;
  Systems::Audio::MenuAudioSystem menuAudioSystem_;
  Systems::Player::PlayerControllerSystem playerSystem_;
  Systems::Render3D::WorldRenderSystem render3dSystem_;
  Systems::Narrative::RadioCodecSystem radioCodecSystem_;
  Systems::Ui::HudSystem hudSystem_;
  Systems::Ui::MenuSystem menuSystem_;
  Systems::Ui::SceneIntroSystem sceneIntroSystem_;
};

}  // namespace Core
}  // namespace DroidCampos
