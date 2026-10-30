#include "systems/input/input_system.hpp"
#include <cmath>
#include "core/interior_layout.hpp"

namespace DroidCampos {
namespace Systems {
namespace Input {

namespace {
// Ray from the rendered reticle; slab intersection handles vertical aim too.
bool reticleHits(const Core::WorldState& state, float minX, float minY,
                 float minZ, float maxX, float maxY, float maxZ, float& hit) {
  const auto& origin = state.player.cameraPos;
  const auto& look = state.player.cameraLookAt;
  const float origins[3] = {origin.x, origin.y, origin.z};
  const float directions[3] = {look.x-origin.x, look.y-origin.y, look.z-origin.z};
  const float mins[3] = {minX, minY, minZ};
  const float maxs[3] = {maxX, maxY, maxZ};
  float near = 0.0F, far = hit;
  for (int axis=0; axis<3; ++axis) {
    if (std::fabs(directions[axis]) < 0.00001F) {
      if (origins[axis] < mins[axis] || origins[axis] > maxs[axis]) return false;
    } else {
      float a = (mins[axis]-origins[axis])/directions[axis];
      float b = (maxs[axis]-origins[axis])/directions[axis];
      if (a>b) { const float temp=a; a=b; b=temp; }
      if (a>near) near=a;
      if (b<far) far=b;
      if (near>far) return false;
    }
  }
  hit=near;
  return true;
}
}  // namespace

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
    if (state.scene.error && (engine->pad.getClicked().Cross || engine->pad.getClicked().Circle)) {
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuBackRequested));
      state.scene.requested = false;
      state.scene.accessFrames = 0;
      state.menu.currentScreen = Core::ScreenState::MainMenu;
    }
    return;
  }

  if (state.menu.currentScreen == Core::ScreenState::GameplayFps) {
    // Terminal owns input before pause, radio, debug shortcuts and gameplay.
    if (state.scene.terminalActive) {
      const auto& clicked = engine->pad.getClicked();
      auto& terminal = state.scene;
      if (clicked.Triangle || (clicked.Circle && !terminal.terminalReading)) {
        terminal.terminalActive = false;
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalClosed));
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuBackRequested));
      } else if (clicked.Circle) {
        terminal.terminalReading = false;
        terminal.terminalPage = 0;
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuBackRequested));
      } else if (terminal.terminalBootFrames > 0) {
        --terminal.terminalBootFrames;
      } else {
        const auto& joy = engine->pad.getLeftJoyPad();
        const bool up = clicked.DpadUp || (state.menu.padCooldown == 0 && joy.v <= 70);
        const bool down = clicked.DpadDown || (state.menu.padCooldown == 0 && joy.v >= 185);
        const bool left = clicked.DpadLeft || (state.menu.padCooldown == 0 && joy.h <= 70);
        const bool right = clicked.DpadRight || (state.menu.padCooldown == 0 && joy.h >= 185);
        if (!terminal.terminalReading && clicked.Cross) {
          terminal.terminalReading = true;
          terminal.terminalPage = 0;
          if(terminal.terminalTopic==2 && terminal.terminalEntry==1 &&
             state.encounter.newEntryUnlocked) state.encounter.objectiveComplete=true;
          bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
        } else if (!terminal.terminalReading && (up || down)) {
          terminal.terminalTopic = (terminal.terminalTopic + (up ? 2 : 1)) % 3;
          terminal.terminalEntry=terminal.terminalTopic==2 && state.encounter.newEntryUnlocked?1:0;
          state.menu.padCooldown = 12;
          bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuNavigateRequested));
        } else if (!terminal.terminalReading && terminal.terminalTopic==2 &&
                   state.encounter.newEntryUnlocked && (left || right)) {
          terminal.terminalEntry=1-terminal.terminalEntry;
          state.menu.padCooldown=12;
          bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuNavigateRequested));
        } else if (terminal.terminalReading &&
                   ((left && terminal.terminalPage > 0) ||
                    (right && terminal.terminalPage < 1))) {
          terminal.terminalPage += left ? -1 : 1;
          state.menu.padCooldown = 12;
          bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuNavigateRequested));
        }
      }
      return; // Closing must consume this frame's Circle/Cross/R1 too.
    }
    if (engine->pad.getClicked().Start) {
      state.menu.currentScreen = Core::ScreenState::Paused;
      state.menu.pauseSelectedIndex = 0;
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
      return;
    }
    if (state.intro.active) return;
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

  if (state.radio.popupActive && clicked.Circle) {
    if (state.radio.phase == Core::RadioPhase::Message)
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::RadioDismissRequested));
    return;
  }

  if (!state.radio.popupActive && clicked.L2) {
    bus.publish(Events::GameEvent::makeSimple(
        Events::EventType::RadioReviewRequested));
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
  float readerHit = 2.55F;
  state.cardReader.isPlayerNearby = state.scene.active == Core::SceneId::Exterior &&
      !state.radio.popupActive &&
      ((dxReader * dxReader + dzReader * dzReader) <= kInteractRadiusSq) &&
      reticleHits(state, 2.2665F, 1.0F, 3.0365F, 2.7027F, 1.4766F, 3.5508F, readerHit);

  if (state.cardReader.isPlayerNearby && !state.radio.popupActive &&
      !state.cardReader.isUnlocked && clicked.Square) {
    bus.publish(Events::GameEvent::makeSimple(
        Events::EventType::CardReaderInteracted));
  }

  state.scene.nearbyDoor = -1;
  if (state.scene.active == Core::SceneId::Interior && !state.radio.popupActive) {
    float nearestHit = 1.8F;
    for (int i = 0; i < 4; ++i) {
      const float dx = state.player.playerPos.x - Core::kInsideDoorX[i];
      const float dz = state.player.playerPos.z - Core::kInsideDoorZ[i];
      const float dist = dx * dx + dz * dz;
      // Keep the doorway target available when its sliding panel is open.
      // Its bounds match the 1.4m wide, 2.2m high opening in the model.
      float hit = nearestHit;
      if (dist <= 1.8F * 1.8F && reticleHits(state,
          Core::kInsideDoorX[i] - .08F, 0.0F, Core::kInsideDoorZ[i] - .7F,
          Core::kInsideDoorX[i] + .08F, 2.2F, Core::kInsideDoorZ[i] + .7F, hit)) {
        nearestHit = hit;
        state.scene.nearbyDoor = i;
      }
    }
    if (state.scene.nearbyDoor >= 0 && clicked.Square) {
      auto& door = state.scene.doors[state.scene.nearbyDoor];
      door.targetOpen = !door.targetOpen;
    }
  }

  // Reach the front of the terminal without entering its solid collider.
  const float terminalDx = state.player.playerPos.x - (-3.58F);
  const float terminalDz = state.player.playerPos.z - (-28.0F);
  state.scene.terminalNearby = state.scene.active == Core::SceneId::Interior &&
      terminalDx * terminalDx + terminalDz * terminalDz <= 1.8F * 1.8F;
  if (!state.scene.terminalNearby) state.scene.terminalActive = false;
  if (state.scene.terminalNearby && !state.radio.popupActive && clicked.Triangle) {
    state.scene.terminalActive = true;
    bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalOpened));
    state.scene.terminalEntry=0;
    state.scene.terminalTopic = 0;
    state.scene.terminalPage = 0;
    state.scene.terminalReading = false;
    state.scene.terminalBootFrames = 24;
    state.menu.padCooldown = 12;
    state.weapon.cancelTransient();
    bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
    return;
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
  state.inputCmd.isRunning = pressed.L1;
  state.inputCmd.jumpRequested = clicked.Cross;
  state.inputCmd.fireRequested = clicked.R1;
  state.inputCmd.reloadRequested = clicked.Circle && !state.radio.popupActive;
}

void InputSystem::pollMenuInput(Tyra::Engine* engine, Core::WorldState& state,
                                Events::EventBus& bus) {
  const auto previous = state.menu;
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

  if (state.menu.currentScreen == Core::ScreenState::Defeated) {
    if(upPressed || downPressed) {
      state.menu.pauseSelectedIndex=1-state.menu.pauseSelectedIndex;
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuNavigateRequested));
    } else if(clicked.Cross) {
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
      if(state.menu.pauseSelectedIndex==0)
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::EncounterRetryRequested));
      else state.menu.currentScreen=Core::ScreenState::MainMenu;
    }
    return;
  }

  if (state.menu.currentScreen == Core::ScreenState::Paused) {
    if (clicked.Start || clicked.Circle) {
      state.menu.currentScreen = Core::ScreenState::GameplayFps;
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuBackRequested));
    } else if (clicked.Cross) {
      state.menu.currentScreen = state.menu.pauseSelectedIndex == 0
          ? Core::ScreenState::GameplayFps : Core::ScreenState::MainMenu;
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
    } else if (upPressed || downPressed) {
      state.menu.pauseSelectedIndex = 1 - state.menu.pauseSelectedIndex;
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuNavigateRequested));
    }
    return;
  }

  if (state.menu.currentScreen == Core::ScreenState::MainMenu) {
    if (upPressed) {
      state.menu.mainSelectedIndex = (state.menu.mainSelectedIndex + 2) % 3;
    } else if (downPressed) {
      state.menu.mainSelectedIndex = (state.menu.mainSelectedIndex + 1) % 3;
    }

    if (clicked.Cross || clicked.Start) {
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
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
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
      state.weapon = Core::WeaponState{};
      state.menu.activeSaveSlot = 0;
      state.scene.target = Core::SceneId::Exterior;
      state.scene.requested = true;
      state.menu.currentScreen = Core::ScreenState::GameplayFps;
      bus.publish(Events::GameEvent::makeSimple(
          Events::EventType::PlayerSpawnResetRequested));
      bus.publish(Events::GameEvent::makeScreenTransition(
          static_cast<u8>(Core::ScreenState::GameplayFps)));
    } else if (clicked.Circle || clicked.Select) {
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuBackRequested));
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
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
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
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuBackRequested));
      state.menu.currentScreen = Core::ScreenState::MainMenu;
      bus.publish(Events::GameEvent::makeScreenTransition(
          static_cast<u8>(Core::ScreenState::MainMenu)));
    }
  } else if (state.menu.currentScreen == Core::ScreenState::Settings) {
    if (clicked.Circle || clicked.Select) {
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuBackRequested));
      state.menu.currentScreen = Core::ScreenState::MainMenu;
      bus.publish(Events::GameEvent::makeScreenTransition(
          static_cast<u8>(Core::ScreenState::MainMenu)));
      return;
    }
    if (upPressed) {
      state.menu.settingsSelectedIndex =
          (state.menu.settingsSelectedIndex + 3) % 4;
    } else if (downPressed) {
      state.menu.settingsSelectedIndex =
          (state.menu.settingsSelectedIndex + 1) % 4;
    }

    if (leftPressed || rightPressed || clicked.Cross) {
      if (clicked.Cross)
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
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

  }

  // One click per accepted selection/value change, including analog repeats.
  // Confirm and back keep their own sounds when pressed with a direction.
  const auto& current = state.menu;
  const bool changed = previous.mainSelectedIndex != current.mainSelectedIndex ||
      previous.loadSelectedIndex != current.loadSelectedIndex ||
      previous.settingsSelectedIndex != current.settingsSelectedIndex ||
      previous.bgmVolume != current.bgmVolume || previous.sfxVolume != current.sfxVolume ||
      previous.scanlinesEnabled != current.scanlinesEnabled ||
      previous.nearestFilter != current.nearestFilter;
  if (changed && previous.currentScreen == current.currentScreen &&
      !clicked.Cross && !clicked.Start && !clicked.Circle && !clicked.Select)
    bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuNavigateRequested));
}

}  // namespace Input
}  // namespace Systems
}  // namespace DroidCampos
