#include "systems/input/input_system.hpp"
#include <cmath>
#include "core/interior_layout.hpp"

namespace DroidCampos {
namespace Systems {
namespace Input {

void InputSystem::init(Tyra::Engine* engine, Core::WorldState& state,
                       Events::EventBus& bus) {
  (void)engine;
  (void)bus;
  state.inputCmd.clear();
}

void InputSystem::executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                               Core::WorldState& state, Events::EventBus& bus,
                               Hardware::Ps2ScratchpadArena& scratchpad) {
  (void)scratchpad;
  if (stage != Core::HardwareStage::Stage0_IopInputPoll || engine == nullptr) {
    return;
  }

  state.inputCmd.clear();

  if (state.menu.padCooldown > 0) {
    state.menu.padCooldown--;
  }

  if (state.menu.currentScreen == Core::ScreenState::Loading) {
    if (state.scene.error && engine->pad.getClicked().Cross) {
      state.scene.requested = false;
      state.scene.accessFrames = 0;
      state.menu.currentScreen = Core::ScreenState::MainMenu;
    }
    return;
  }

  if (state.menu.currentScreen == Core::ScreenState::GameplayFps) {
    pollGameplayInput(engine, state, bus);
  } else {
    pollMenuInput(engine, state, bus);
  }
}

void InputSystem::shutdown(Tyra::Engine* engine) { (void)engine; }

void InputSystem::pollGameplayInput(Tyra::Engine* engine,
                                    Core::WorldState& state,
                                    Events::EventBus& bus) {
  const auto& pad = engine->pad;
  const auto& clicked = pad.getClicked();
  const auto& pressed = pad.getPressed();
  const auto& leftJoy = pad.getLeftJoyPad();
  const auto& rightJoy = pad.getRightJoyPad();

  if (state.scene.accessFrames > 0) return;

  if (state.radio.popupActive && clicked.Cross) {
    bus.publish(
        Events::GameEvent::makeSimple(Events::EventType::RadioDismissRequested));
    return;
  }

  if (!state.radio.popupActive && (clicked.L2 || clicked.R2)) {
    bus.publish(Events::GameEvent::makeSimple(
        Events::EventType::RadioTransmissionTriggered));
    return;
  }

  if (clicked.Select) {
    state.menu.currentScreen = Core::ScreenState::MainMenu;
    bus.publish(Events::GameEvent::makeScreenTransition(
        static_cast<u8>(Core::ScreenState::MainMenu)));
    return;
  }

  if (clicked.L3 || clicked.R3) {
    bus.publish(Events::GameEvent::makeSimple(
        Events::EventType::PlayerSpawnResetRequested));
  }

  // Check proximity to Card Reader pedestal (X = 2.48m, Z_tyra = 3.25m)
  constexpr float kCardReaderX = 2.48F;
  constexpr float kCardReaderZ = 3.25F;
  constexpr float kInteractRadiusSq = 2.55F * 2.55F;
  const float dxReader = state.player.playerPos.x - kCardReaderX;
  const float dzReader = state.player.playerPos.z - kCardReaderZ;
  state.cardReader.isPlayerNearby = state.scene.active == Core::SceneId::Exterior &&
      ((dxReader * dxReader + dzReader * dzReader) <= kInteractRadiusSq);

  if (state.cardReader.isPlayerNearby && !state.radio.popupActive &&
      !state.cardReader.isUnlocked && clicked.Square) {
    bus.publish(Events::GameEvent::makeSimple(
        Events::EventType::CardReaderInteracted));
  }

  state.scene.nearbyDoor = -1;
  if (state.scene.active == Core::SceneId::Interior && !state.radio.popupActive) {
    float nearest = 1.8F * 1.8F;
    for (int i = 0; i < 4; ++i) {
      const float dx = state.player.playerPos.x - Core::kInsideDoorX[i];
      const float dz = state.player.playerPos.z - Core::kInsideDoorZ[i];
      const float dist = dx * dx + dz * dz;
      if (dist < nearest) { nearest = dist; state.scene.nearbyDoor = i; }
    }
    if (state.scene.nearbyDoor >= 0 && clicked.Square) {
      auto& door = state.scene.doors[state.scene.nearbyDoor];
      door.targetOpen = !door.targetOpen;
    }
  }

  float lookYawDelta = 0.0F;
  float lookPitchDelta = 0.0F;

  const float rx = (static_cast<float>(rightJoy.h) - 128.0F) / 128.0F;
  const float ry = (static_cast<float>(rightJoy.v) - 128.0F) / 128.0F;
  const float lookSens = state.player.lookSensitivity;

  if (std::fabs(rx) > 0.18F) {
    lookYawDelta += rx * lookSens;
  }
  if (std::fabs(ry) > 0.18F) {
    lookPitchDelta -= ry * lookSens;
  }

  if (!state.cardReader.isPlayerNearby && state.scene.nearbyDoor < 0 && pressed.Square) {
    lookYawDelta -= lookSens * 0.85F;
  }
  if (pressed.Circle) {
    lookYawDelta += lookSens * 0.85F;
  }
  if (pressed.Triangle) {
    lookPitchDelta += lookSens * 0.75F;
  }
  if (!state.radio.popupActive && pressed.Cross) {
    lookPitchDelta -= lookSens * 0.75F;
  }

  float moveForward = 0.0F;
  float moveStrafe = 0.0F;

  const float lx = (static_cast<float>(leftJoy.h) - 128.0F) / 128.0F;
  const float ly = (static_cast<float>(leftJoy.v) - 128.0F) / 128.0F;

  if (std::fabs(ly) > 0.18F) {
    moveForward -= ly;
  }
  if (std::fabs(lx) > 0.18F) {
    moveStrafe += lx;
  }

  if (pressed.DpadUp) {
    moveForward += 1.0F;
  }
  if (pressed.DpadDown) {
    moveForward -= 1.0F;
  }
  if (pressed.DpadLeft) {
    moveStrafe -= 1.0F;
  }
  if (pressed.DpadRight) {
    moveStrafe += 1.0F;
  }

  state.inputCmd.moveForward = moveForward;
  state.inputCmd.moveStrafe = moveStrafe;
  state.inputCmd.lookYawDelta = lookYawDelta;
  state.inputCmd.lookPitchDelta = lookPitchDelta;
  state.inputCmd.isRunning = (pressed.L1 || pressed.R1);
}

void InputSystem::pollMenuInput(Tyra::Engine* engine, Core::WorldState& state,
                                Events::EventBus& bus) {
  const auto& clicked = engine->pad.getClicked();
  const auto& joy = engine->pad.getLeftJoyPad();

  const bool upPressed =
      clicked.DpadUp || (state.menu.padCooldown == 0 && joy.v <= 70);
  const bool downPressed =
      clicked.DpadDown || (state.menu.padCooldown == 0 && joy.v >= 185);
  const bool leftPressed =
      clicked.DpadLeft || (state.menu.padCooldown == 0 && joy.h <= 70);
  const bool rightPressed =
      clicked.DpadRight || (state.menu.padCooldown == 0 && joy.h >= 185);

  if (upPressed || downPressed || leftPressed || rightPressed) {
    state.menu.padCooldown = 12;
  }

  if (state.menu.currentScreen == Core::ScreenState::MainMenu) {
    if (upPressed) {
      state.menu.mainSelectedIndex = (state.menu.mainSelectedIndex + 2) % 3;
    } else if (downPressed) {
      state.menu.mainSelectedIndex = (state.menu.mainSelectedIndex + 1) % 3;
    }

    if (clicked.Cross || clicked.Start) {
      if (state.menu.mainSelectedIndex == 0) {
        state.menu.currentScreen = Core::ScreenState::NewGame;
        bus.publish(Events::GameEvent::makeScreenTransition(
            static_cast<u8>(Core::ScreenState::NewGame)));
      } else if (state.menu.mainSelectedIndex == 1) {
        state.menu.currentScreen = Core::ScreenState::LoadGame;
        bus.publish(Events::GameEvent::makeScreenTransition(
            static_cast<u8>(Core::ScreenState::LoadGame)));
      } else if (state.menu.mainSelectedIndex == 2) {
        state.menu.currentScreen = Core::ScreenState::Settings;
        bus.publish(Events::GameEvent::makeScreenTransition(
            static_cast<u8>(Core::ScreenState::Settings)));
      }
    }
  } else if (state.menu.currentScreen == Core::ScreenState::NewGame) {
    if (clicked.Cross || clicked.Start) {
      state.menu.activeSaveSlot = 0;
      state.scene.target = Core::SceneId::Exterior;
      state.scene.requested = true;
      state.menu.currentScreen = Core::ScreenState::GameplayFps;
      bus.publish(Events::GameEvent::makeSimple(
          Events::EventType::PlayerSpawnResetRequested));
      bus.publish(Events::GameEvent::makeScreenTransition(
          static_cast<u8>(Core::ScreenState::GameplayFps)));
    } else if (clicked.Circle || clicked.Select) {
      state.menu.currentScreen = Core::ScreenState::MainMenu;
      bus.publish(Events::GameEvent::makeScreenTransition(
          static_cast<u8>(Core::ScreenState::MainMenu)));
    }
  } else if (state.menu.currentScreen == Core::ScreenState::LoadGame) {
    if (upPressed) {
      state.menu.loadSelectedIndex = (state.menu.loadSelectedIndex + 2) % 3;
    } else if (downPressed) {
      state.menu.loadSelectedIndex = (state.menu.loadSelectedIndex + 1) % 3;
    }

    if (clicked.Cross || clicked.Start) {
      if (state.menu.loadSelectedIndex < 2) {
        state.menu.activeSaveSlot = state.menu.loadSelectedIndex + 1;
        state.scene.target = Core::SceneId::Exterior;
        state.scene.requested = true;
        state.menu.currentScreen = Core::ScreenState::GameplayFps;
        bus.publish(Events::GameEvent::makeSimple(
            Events::EventType::PlayerSpawnResetRequested));
        bus.publish(Events::GameEvent::makeScreenTransition(
            static_cast<u8>(Core::ScreenState::GameplayFps)));
      }
    } else if (clicked.Circle || clicked.Select) {
      state.menu.currentScreen = Core::ScreenState::MainMenu;
      bus.publish(Events::GameEvent::makeScreenTransition(
          static_cast<u8>(Core::ScreenState::MainMenu)));
    }
  } else if (state.menu.currentScreen == Core::ScreenState::Settings) {
    if (upPressed) {
      state.menu.settingsSelectedIndex =
          (state.menu.settingsSelectedIndex + 3) % 4;
    } else if (downPressed) {
      state.menu.settingsSelectedIndex =
          (state.menu.settingsSelectedIndex + 1) % 4;
    }

    if (leftPressed || rightPressed || clicked.Cross) {
      const int dir = (rightPressed || clicked.Cross) ? 1 : -1;
      if (state.menu.settingsSelectedIndex == 0) {
        state.menu.bgmVolume += dir * 10;
        if (state.menu.bgmVolume < 0) state.menu.bgmVolume = 0;
        if (state.menu.bgmVolume > 100) state.menu.bgmVolume = 100;
        bus.publish(Events::GameEvent::makeSimple(
            Events::EventType::AudioSettingChanged,
            static_cast<s16>(state.menu.bgmVolume)));
      } else if (state.menu.settingsSelectedIndex == 1) {
        state.menu.sfxVolume += dir * 10;
        if (state.menu.sfxVolume < 0) state.menu.sfxVolume = 0;
        if (state.menu.sfxVolume > 100) state.menu.sfxVolume = 100;
        bus.publish(Events::GameEvent::makeSimple(
            Events::EventType::AudioSettingChanged,
            static_cast<s16>(state.menu.sfxVolume)));
      } else if (state.menu.settingsSelectedIndex == 2) {
        state.menu.scanlinesEnabled = !state.menu.scanlinesEnabled;
        bus.publish(Events::GameEvent::makeSimple(
            Events::EventType::VideoFilterChanged,
            state.menu.scanlinesEnabled ? 1 : 0));
      } else if (state.menu.settingsSelectedIndex == 3) {
        state.menu.nearestFilter = !state.menu.nearestFilter;
        bus.publish(Events::GameEvent::makeSimple(
            Events::EventType::VideoFilterChanged,
            state.menu.nearestFilter ? 1 : 0));
      }
    }

    if (clicked.Circle || clicked.Select) {
      state.menu.currentScreen = Core::ScreenState::MainMenu;
      bus.publish(Events::GameEvent::makeScreenTransition(
          static_cast<u8>(Core::ScreenState::MainMenu)));
    }
  }
}

}  // namespace Input
}  // namespace Systems
}  // namespace DroidCampos
