#include "systems/physics/collision_system.hpp"
#include <cstring>
#include <cmath>
#include <initializer_list>
#include "core/interior_layout.hpp"

namespace DroidCampos {
namespace Systems {
namespace Physics {

namespace {
constexpr float kPlayerRadius = 0.48F;
constexpr float kPerimeterMinX = -5.25F;
constexpr float kPerimeterMaxX = 5.25F;
constexpr float kPerimeterMinZ = 3.20F;
constexpr float kPerimeterMaxZ = 19.30F;

constexpr u32 kWorldObstacleCount = 11U;

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
    // Rear Physical Blockade (Zona A South: Cargo Container + Concrete/Hazard Barriers)
    {-5.15F, 0.80F, 18.15F, 19.55F},
    {0.65F, 5.68F, 18.25F, 19.45F},
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
    float& x, float& z, Hardware::Ps2ScratchpadArena& scratchpad,
    const Core::WorldState& state) {
  if (state.scene.active == Core::SceneId::Interior) {
    const auto resolve = [&x, &z](float minX, float maxX, float minZ, float maxZ) {
      minX -= kPlayerRadius; maxX += kPlayerRadius;
      minZ -= kPlayerRadius; maxZ += kPlayerRadius;
      if (x <= minX || x >= maxX || z <= minZ || z >= maxZ) return;
      float penetration = x - minX; int axis = 0;
      if (maxX - x < penetration) { penetration = maxX - x; axis = 1; }
      if (z - minZ < penetration) { penetration = z - minZ; axis = 2; }
      if (maxZ - z < penetration) { axis = 3; }
      if (axis == 0) x = minX;
      else if (axis == 1) x = maxX;
      else if (axis == 2) z = minZ;
      else z = maxZ;
    };
    if (x < -5.12F) x = -5.12F;
    if (x > 5.12F) x = 5.12F;
    if (z < -23.40F) z = -23.40F;
    if (z > -0.60F) z = -0.60F;
    for (int side : {-1, 1}) {
      const float wx = side * 1.6F;
      resolve(wx - .1F, wx + .1F, -7.3F, 0);
      resolve(wx - .1F, wx + .1F, -15.3F, -8.7F);
      resolve(wx - .1F, wx + .1F, -24, -16.7F);
      // Empty space beside the corridor outside the two enclosed rooms.
      const float minX = side < 0 ? -5.8F : 1.5F;
      const float maxX = side < 0 ? -1.5F : 5.8F;
      resolve(minX, maxX, -6.1F, 0);
      resolve(minX, maxX, -14.1F, -9.9F);
      resolve(minX, maxX, -24, -17.9F);
      for (float roomZ : {-8.0F, -16.0F}) {
        const float cx = side * 5.25F;
        for (int j = 0; j < 3; ++j) {
          const float cz = roomZ + 1.25F - j * .8F;
          resolve(cx - .25F, cx + .25F, cz - .32F, cz + .32F);
        }
        const float bx = side * 3.7F;
        resolve(bx - .9F, bx + .9F, roomZ - 1.90F, roomZ - 1.20F);
      }
    }
    for (int i = 0; i < 4; ++i) {
      const float dx = Core::kInsideDoorX[i];
      const float dz = Core::kInsideDoorZ[i] - state.scene.doors[i].openAmount * Core::kDoorTravel;
      resolve(dx - .08F, dx + .08F, dz - .7F, dz + .7F);
    }
    resolve(-1.175F, 1.175F, -23.4F, -22.02F); // Terminal and keyboard.
    return;
  }
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
