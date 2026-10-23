#include "systems/player/player_controller_system.hpp"
#include "systems/physics/collision_system.hpp"
#include <cmath>

namespace DroidCampos {
namespace Systems {
namespace Player {

namespace {
constexpr float kPi = 3.1415926535F;
constexpr float kEyeHeight = 1.70F;
constexpr float kMaxPitch = 1.28F;
}  // namespace

void PlayerControllerSystem::init(Tyra::Engine* engine, Core::WorldState& state,
                                  Events::EventBus& bus) {
  (void)engine;
  (void)bus;
  state.resetPlayerToSpawn();
  updateCameraVectors(state);
}

void PlayerControllerSystem::executeStage(
    Core::HardwareStage stage, Tyra::Engine* engine, Core::WorldState& state,
    Events::EventBus& bus, Hardware::Ps2ScratchpadArena& scratchpad) {
  (void)engine;
  if (stage != Core::HardwareStage::Stage1_EeVu0Simulation) {
    return;
  }

  u32 eventCount = 0;
  const Events::GameEvent* events = bus.getFrameEvents(eventCount);
  for (u32 i = 0; i < eventCount; ++i) {
    if (events[i].type == Events::EventType::PlayerSpawnResetRequested) {
      state.resetPlayerToSpawn();
      updateCameraVectors(state);
    }
  }

  updateGlobalAnimations(state);

  if (state.menu.currentScreen == Core::ScreenState::GameplayFps) {
    updateFpsKinematics(state, scratchpad);
  }
}

void PlayerControllerSystem::shutdown(Tyra::Engine* engine) { (void)engine; }

void PlayerControllerSystem::updateGlobalAnimations(Core::WorldState& state) {
  state.anim.animTick += 0.06F;
  state.anim.scanlineY += 1.8F;
  if (state.anim.scanlineY > 448.0F) {
    state.anim.scanlineY = 0.0F;
  }

  state.anim.bgScrollOffset += 0.25F;
  if (state.anim.bgScrollOffset >= 32.0F) {
    state.anim.bgScrollOffset = 0.0F;
  }
}

void PlayerControllerSystem::updateCameraVectors(Core::WorldState& state) {
  const float walkBobPhase = state.player.walkBobPhase;
  const float walkBobAmount = state.player.walkBobAmount;
  const float yaw = state.player.yaw;
  const float pitch = state.player.pitch;

  const float bobVertical =
      std::sin(walkBobPhase * 2.0F) * 0.042F * walkBobAmount;
  const float bobLateral = std::cos(walkBobPhase) * 0.022F * walkBobAmount;

  const float rightX = -std::sin(yaw);
  const float rightZ = std::cos(yaw);

  state.player.cameraPos.set(state.player.playerPos.x + rightX * bobLateral,
                             kEyeHeight + bobVertical,
                             state.player.playerPos.z + rightZ * bobLateral,
                             1.0F);

  const float cosPitch = std::cos(pitch);
  const float dirX = cosPitch * std::cos(yaw);
  const float dirY = std::sin(pitch);
  const float dirZ = cosPitch * std::sin(yaw);

  state.player.cameraLookAt.set(
      state.player.cameraPos.x + dirX, state.player.cameraPos.y + dirY,
      state.player.cameraPos.z + dirZ, 1.0F);
}

void PlayerControllerSystem::updateFpsKinematics(
    Core::WorldState& state, Hardware::Ps2ScratchpadArena& scratchpad) {
  const float lookYawDelta = state.inputCmd.lookYawDelta;
  const float lookPitchDelta = state.inputCmd.lookPitchDelta;

  state.player.yaw += lookYawDelta;
  if (state.player.yaw > kPi) state.player.yaw -= 2.0F * kPi;
  if (state.player.yaw < -kPi) state.player.yaw += 2.0F * kPi;

  state.player.pitch += lookPitchDelta;
  if (state.player.pitch > kMaxPitch) state.player.pitch = kMaxPitch;
  if (state.player.pitch < -kMaxPitch) state.player.pitch = -kMaxPitch;

  state.player.weaponSwayX =
      state.player.weaponSwayX * 0.80F - lookYawDelta * 0.35F;
  state.player.weaponSwayY =
      state.player.weaponSwayY * 0.80F - lookPitchDelta * 0.35F;
  if (state.player.weaponSwayX > 0.032F) state.player.weaponSwayX = 0.032F;
  if (state.player.weaponSwayX < -0.032F) state.player.weaponSwayX = -0.032F;
  if (state.player.weaponSwayY > 0.028F) state.player.weaponSwayY = 0.028F;
  if (state.player.weaponSwayY < -0.028F) state.player.weaponSwayY = -0.028F;

  float moveForward = state.inputCmd.moveForward;
  float moveStrafe = state.inputCmd.moveStrafe;

  float moveMag =
      std::sqrt(moveForward * moveForward + moveStrafe * moveStrafe);
  if (moveMag > 1.0F) {
    const float invMag = 1.0F / moveMag;
    moveForward *= invMag;
    moveStrafe *= invMag;
    moveMag = 1.0F;
  }

  const bool isRunning = state.inputCmd.isRunning;
  const float speedMult = isRunning ? 1.55F : 1.0F;
  const float step = state.player.moveSpeed * speedMult;

  if (moveMag > 0.05F) {
    state.player.walkBobPhase += (isRunning ? 0.21F : 0.14F) * moveMag;
    if (state.player.walkBobPhase > 2.0F * kPi) {
      state.player.walkBobPhase -= 2.0F * kPi;
    }
    const float targetBob = moveMag * (isRunning ? 1.25F : 1.0F);
    state.player.walkBobAmount =
        state.player.walkBobAmount * 0.82F + targetBob * 0.18F;
  } else {
    state.player.walkBobAmount *= 0.80F;
    if (state.player.walkBobAmount < 0.005F) {
      state.player.walkBobAmount = 0.0F;
    }
  }

  const float fwdX = std::cos(state.player.yaw);
  const float fwdZ = std::sin(state.player.yaw);
  const float rightX = -fwdZ;
  const float rightZ = fwdX;

  float nextX =
      state.player.playerPos.x + (fwdX * moveForward + rightX * moveStrafe) * step;
  float nextZ =
      state.player.playerPos.z + (fwdZ * moveForward + rightZ * moveStrafe) * step;

  Physics::CollisionSystem::resolvePlayerCollisions(nextX, nextZ, scratchpad);

  state.player.playerPos.x = nextX;
  state.player.playerPos.y = kEyeHeight;
  state.player.playerPos.z = nextZ;

  updateCameraVectors(state);
}

}  // namespace Player
}  // namespace Systems
}  // namespace DroidCampos
