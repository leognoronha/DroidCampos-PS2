#pragma once

#include "core/isubsystem.hpp"

namespace DroidCampos {
namespace Systems {
namespace Input {

class InputSystem final : public Core::ISubsystem {
 public:
  InputSystem() = default;
  ~InputSystem() override = default;

  const char* getName() const override { return "InputSystem"; }

  void init(Tyra::Engine* engine, Core::WorldState& state,
            Events::EventBus& bus) override;

  void executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                    Core::WorldState& state, Events::EventBus& bus,
                    Hardware::Ps2ScratchpadArena& scratchpad) override;

  void shutdown(Tyra::Engine* engine) override;

 private:
  void pollGameplayInput(Tyra::Engine* engine, Core::WorldState& state,
                         Events::EventBus& bus);
  void pollMenuInput(Tyra::Engine* engine, Core::WorldState& state,
                     Events::EventBus& bus);
};

}  // namespace Input
}  // namespace Systems
}  // namespace DroidCampos
