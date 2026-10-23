#include "systems/physics/collision_system.hpp"
#include <cstring>

namespace DroidCampos {
namespace Systems {
namespace Physics {

namespace {
constexpr float kPlayerRadius = 0.48F;
constexpr float kPerimeterMinX = -5.25F;
constexpr float kPerimeterMaxX = 5.25F;
constexpr float kPerimeterMinZ = 3.20F;
constexpr float kPerimeterMaxZ = 19.30F;

constexpr u32 kWorldObstacleCount = 9U;

alignas(16) constexpr BoxCollider2D kWorldObstacles[kWorldObstacleCount] = {
    // Guarita de Vigilancia (West of gate)
    {-5.60F, -2.75F, 3.80F, 6.65F},
    // Card Reader Pedestal (East of gate)
    {2.15F, 2.85F, 2.95F, 3.55F},
    // Abandoned Container (East flank)
    {3.35F, 5.80F, 1.00F, 4.00F},
    // Concrete Jersey Barriers (Zona B funnel)
    {-4.15F, -2.05F, 12.10F, 13.10F},
    {2.05F, 4.15F, 11.70F, 12.70F},
    {-3.75F, -1.65F, 7.90F, 8.90F},
    {1.75F, 3.85F, 7.30F, 8.30F},
    // Metallic Traffic Barriers
    {-3.55F, -1.45F, 10.10F, 10.90F},
    {1.45F, 3.55F, 5.40F, 6.20F},
};
}  // namespace

void CollisionSystem::init(Tyra::Engine* engine, Core::WorldState& state,
                           Events::EventBus& bus) {
  (void)engine;
  (void)state;
  (void)bus;
}

void CollisionSystem::executeStage(Core::HardwareStage stage,
                                   Tyra::Engine* engine,
                                   Core::WorldState& state,
                                   Events::EventBus& bus,
                                   Hardware::Ps2ScratchpadArena& scratchpad) {
  (void)stage;
  (void)engine;
  (void)state;
  (void)bus;
  (void)scratchpad;
}

void CollisionSystem::shutdown(Tyra::Engine* engine) { (void)engine; }

void CollisionSystem::resolvePlayerCollisions(
    float& x, float& z, Hardware::Ps2ScratchpadArena& scratchpad) {
  if (x < kPerimeterMinX) x = kPerimeterMinX;
  if (x > kPerimeterMaxX) x = kPerimeterMaxX;
  if (z > kPerimeterMaxZ) z = kPerimeterMaxZ;
  if (z < kPerimeterMinZ) z = kPerimeterMinZ;

  BoxCollider2D* activeColliders =
      scratchpad.allocArray<BoxCollider2D>(kWorldObstacleCount);
  const BoxCollider2D* colliderSource = kWorldObstacles;

  if (activeColliders != nullptr) {
    std::memcpy(activeColliders, kWorldObstacles,
                sizeof(BoxCollider2D) * kWorldObstacleCount);
    colliderSource = activeColliders;
  }

  for (u32 i = 0; i < kWorldObstacleCount; ++i) {
    const BoxCollider2D& box = colliderSource[i];
    const float expMinX = box.minX - kPlayerRadius;
    const float expMaxX = box.maxX + kPlayerRadius;
    const float expMinZ = box.minZ - kPlayerRadius;
    const float expMaxZ = box.maxZ + kPlayerRadius;

    if (x > expMinX && x < expMaxX && z > expMinZ && z < expMaxZ) {
      const float penLeft = x - expMinX;
      const float penRight = expMaxX - x;
      const float penFront = z - expMinZ;
      const float penBack = expMaxZ - z;

      float minPen = penLeft;
      int axis = 0;
      if (penRight < minPen) {
        minPen = penRight;
        axis = 1;
      }
      if (penFront < minPen) {
        minPen = penFront;
        axis = 2;
      }
      if (penBack < minPen) {
        minPen = penBack;
        axis = 3;
      }

      if (axis == 0) {
        x = expMinX;
      } else if (axis == 1) {
        x = expMaxX;
      } else if (axis == 2) {
        z = expMinZ;
      } else {
        z = expMaxZ;
      }
    }
  }
}

}  // namespace Physics
}  // namespace Systems
}  // namespace DroidCampos
