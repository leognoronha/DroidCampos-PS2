#pragma once

#include "core/isubsystem.hpp"

namespace DroidCampos {
namespace Systems {
namespace Player {

class PlayerControllerSystem final : public Core::ISubsystem {
 public:
  PlayerControllerSystem() = default;
  ~PlayerControllerSystem() override = default;

  const char* getName() const override { return "PlayerControllerSystem"; }

  void init(Tyra::Engine* engine, Core::WorldState& state,
            Events::EventBus& bus) override;

  void executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                    Core::WorldState& state, Events::EventBus& bus,
                    Hardware::Ps2ScratchpadArena& scratchpad) override;

  void shutdown(Tyra::Engine* engine) override;

  static void updateCameraVectors(Core::WorldState& state);

 private:
  void updateGlobalAnimations(Core::WorldState& state);
  void updateFpsKinematics(Core::WorldState& state,
                           Hardware::Ps2ScratchpadArena& scratchpad);
};

}  // namespace Player
}  // namespace Systems
}  // namespace DroidCampos
