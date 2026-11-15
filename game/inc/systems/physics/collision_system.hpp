#pragma once

#include "core/isubsystem.hpp"
#include "hardware/ps2_scratchpad.hpp"

namespace DroidCampos {
namespace Systems {
namespace Physics {

struct alignas(16) BoxCollider2D {
  float minX;
  float maxX;
  float minZ;
  float maxZ;
};

class CollisionSystem final : public Core::ISubsystem {
 public:
  CollisionSystem() = default;
  ~CollisionSystem() override = default;

  const char* getName() const override { return "CollisionSystem"; }

  void init(Tyra::Engine* engine, Core::WorldState& state,
            Events::EventBus& bus) override;

  void executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                    Core::WorldState& state, Events::EventBus& bus,
                    Hardware::Ps2ScratchpadArena& scratchpad) override;

  void shutdown(Tyra::Engine* engine) override;

  static void resolvePlayerCollisions(float& x, float& z,
                                      Hardware::Ps2ScratchpadArena& scratchpad,
                                      const Core::WorldState& state);
};

}  // namespace Physics
}  // namespace Systems
}  // namespace DroidCampos
