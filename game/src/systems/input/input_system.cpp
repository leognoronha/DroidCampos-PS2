#include "pad/analog_axis.hpp"
#include "systems/input/input_system.hpp"
#include "core/terminal_documents.hpp"
#include <cmath>
#include "core/interior_layout.hpp"
#include "core/pickup_layout.hpp"
#include "systems/physics/interior_geometry.hpp"

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
  state.droid.buttonNearby=false;
}

void InputSystem::executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                               Core::WorldState& state, Events::EventBus& bus,
                               Hardware::Ps2ScratchpadArena& scratchpad) {
  (void)scratchpad;
  if (stage != Core::HardwareStage::Stage0_IopInputPoll || engine == nullptr) {
    return;
  }

  state.inputCmd.clear();
  state.droid.buttonNearby=false;
  state.nearbyPickup = Core::Pickup::Count;

  if (state.menu.padCooldown > 0) {
    state.menu.padCooldown--;
  }

  if (state.menu.currentScreen == Core::ScreenState::Loading) {
    if (state.scene.error && (engine->pad.getClicked().Cross || engine->pad.getClicked().Circle)) {
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuBackRequested));
      state.scene.requested = false;
      state.scene.accessTicks = 0;
      state.menu.currentScreen = Core::ScreenState::MainMenu;
    }
    return;
  }

  if (state.menu.currentScreen == Core::ScreenState::GameplayFps) {
    // Terminal owns input before pause, radio, debug shortcuts and gameplay.
    if (state.scene.terminalActive) {
      const auto& clicked = engine->pad.getClicked();
      auto& terminal = state.scene;
      ++terminal.terminalAnimationFrame;
      if (clicked.Triangle ||
          (clicked.Circle && !terminal.terminalReading && !terminal.terminalVideoActive)) {
        terminal.terminalVideoActive = false;
        terminal.terminalSecretUnlocked = false;
        terminal.terminalActive = false;
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalClosed));
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuBackRequested));
      } else if (terminal.terminalVideoActive) {
        if (clicked.Circle) terminal.terminalVideoActive = false;
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
        const int documentCount = Core::kTruthTopic + (state.encounter.newEntryUnlocked ? 1 : 0);
        const int topicCount = documentCount + terminal.terminalSecretUnlocked;
        if (!terminal.terminalReading && clicked.Cross) {
          terminal.terminalCodeProgress = 0;
          if (terminal.terminalTopic == Core::kSecretTopic) {
            terminal.terminalVideoError = false;
            terminal.terminalVideoActive = true;
            return;
          }
          terminal.terminalReading = true;
          terminal.terminalPage = 0;
          if (terminal.terminalTopic == Core::kTruthTopic) state.encounter.objectiveComplete = true;
          bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
        } else if (!terminal.terminalReading) {
          if (!terminal.terminalSecretUnlocked) {
            const auto& held = engine->pad.getPressed();
            const int heldDirections = held.DpadDown + held.DpadLeft + held.DpadRight + held.DpadUp;
            const int direction = clicked.DpadDown + 2 * clicked.DpadLeft +
                                  3 * clicked.DpadRight + 4 * clicked.DpadUp;
            if (heldDirections > 1 ||
                clicked.DpadDown + clicked.DpadLeft + clicked.DpadRight + clicked.DpadUp > 1) {
              terminal.terminalCodeProgress = 0;
            } else if (direction != 0) {
              constexpr int code[] = {1, 1, 2, 3, 4};
              auto& progress = terminal.terminalCodeProgress;
              if (direction == code[progress]) ++progress;
              else if (direction == 1) progress = progress == 2 ? 2 : 1;
              else progress = 0;
              if (progress == 5) {
                terminal.terminalSecretUnlocked = true;
                terminal.terminalTopic = Core::kSecretTopic;
                bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalSecretUnlocked));
                return;
              }
            }
          }
          if (up || down) {
            const int index = terminal.terminalTopic == Core::kSecretTopic
                                  ? documentCount : terminal.terminalTopic;
            const int next = (index + (up ? topicCount - 1 : 1)) % topicCount;
            terminal.terminalTopic = next == documentCount ? Core::kSecretTopic : next;
            state.menu.padCooldown = 12;
            bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuNavigateRequested));
          }
        } else if (
                   ((left && terminal.terminalPage > 0) ||
                    (right && terminal.terminalPage + 1 < Core::kTerminalDocuments[terminal.terminalTopic].pageCount))) {
          terminal.terminalPage += left ? -1 : 1;
          state.menu.padCooldown = 12;
          bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuNavigateRequested));
        }
      }
      return; // Closing must consume this frame's Circle/Cross/R1 too.
    }
    if (state.scene.accessTicks > 0) return;
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

  if (state.radio.blocksGameplay() && clicked.Circle) {
    if (state.radio.phase == Core::RadioPhase::Message)
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::RadioDismissRequested));
    return;
  }

  if (clicked.L2 && !state.radio.blocksGameplay()) {
    bus.publish(Events::GameEvent::makeSimple(
        state.radio.popupActive ? Events::EventType::RadioDismissRequested
                                : Events::EventType::RadioReviewRequested));
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
    return;
  }

  bool pickupInteraction = false;
  if (state.simulationActive()) {
    if (clicked.R2 && state.inventory.useMedkit(state.playerHealth))
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MedkitUsed));
    const auto& player = state.player.playerPos;
    float nearestSq = Core::kPickupRadius * Core::kPickupRadius;
    for (const auto& pickup : Core::kPickups) {
      if (pickup.scene != state.scene.active || state.inventory.has(pickup.item)) continue;
      const float dx = player.x - pickup.x, dz = player.z - pickup.z;
      const float distanceSq = dx * dx + dz * dz;
      if (distanceSq >= nearestSq) continue;
      if (pickup.scene == Core::SceneId::Interior &&
          !Physics::interiorSegmentFree(state, player,
              Tyra::Vec4(pickup.x, 0, pickup.z, 1), .02F)) continue;
      state.nearbyPickup = pickup.item;
      nearestSq = distanceSq;
    }
    pickupInteraction = clicked.Square && state.nearbyPickup != Core::Pickup::Count;
    if (pickupInteraction && state.inventory.collect(state.nearbyPickup)) {
      if (state.nearbyPickup == Core::Pickup::Pistol)
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::WeaponPickedUp));
      else if (state.nearbyPickup != Core::Pickup::AccessCard)
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::MedkitPickedUp));
      state.nearbyPickup = Core::Pickup::Count;
    }
  }

  // Check proximity to Card Reader pedestal (X = 2.48m, Z_tyra = 3.25m)
  constexpr float kCardReaderX = 2.48F;
  constexpr float kCardReaderZ = 3.25F;
  constexpr float kInteractRadiusSq = 2.55F * 2.55F;
  const float dxReader = state.player.playerPos.x - kCardReaderX;
  const float dzReader = state.player.playerPos.z - kCardReaderZ;
  float readerHit = 2.55F;
  state.cardReader.isPlayerNearby = state.scene.active == Core::SceneId::Exterior &&
      !state.radio.blocksGameplay() &&
      ((dxReader * dxReader + dzReader * dzReader) <= kInteractRadiusSq) &&
      reticleHits(state, 2.2665F, 1.0F, 3.0365F, 2.7027F, 1.4766F, 3.5508F, readerHit);

  if (state.cardReader.isPlayerNearby && !state.cardReader.isUnlocked &&
      state.simulationActive() && state.inventory.canEnter() &&
      clicked.Square && !pickupInteraction) {
    bus.publish(Events::GameEvent::makeSimple(
        Events::EventType::CardReaderInteracted));
  }

  state.droid.buttonNearby=false;
  if(state.scene.active==Core::SceneId::Interior && !state.radio.blocksGameplay() &&
     state.droid.phase==Core::DroidPhase::ButtonReady) {
    float hit=1.5F;
    Tyra::Vec4 post(0,0,-26,1);
    if(Systems::Physics::distanceXZ(state.player.playerPos,post)<=1.5F &&
       reticleHits(state,-.1F,.90F,-26.1F,.1F,1.02F,-25.9F,hit) &&
       Systems::Physics::interiorSegmentFree(state,state.player.playerPos,
         Tyra::Vec4(state.player.playerPos.x*.25F/std::max(.001F,Systems::Physics::distanceXZ(state.player.playerPos,post)),0,-26+(state.player.playerPos.z+26)*.25F/std::max(.001F,Systems::Physics::distanceXZ(state.player.playerPos,post)),1),.01F)) {
      state.droid.buttonNearby=true;
      if(clicked.Triangle) {
        state.droid.phase=Core::DroidPhase::Release;state.droid.phaseTicks=0;
        state.weapon.cancelTransient();state.inputCmd.clear();
        bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));return;
      }
    }
  }
  state.scene.nearbyDoor = -1;
  if (state.scene.active == Core::SceneId::Interior && !state.radio.blocksGameplay()) {
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
    if (state.scene.nearbyDoor >= 0 && clicked.Square && !pickupInteraction) {
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
  if (state.scene.terminalNearby && !state.radio.blocksGameplay() && clicked.Triangle) {
    state.scene.terminalActive = true;
    state.scene.terminalCodeProgress = 0;
    state.scene.terminalSecretUnlocked = false;
    state.scene.terminalVideoActive = false;
    state.scene.terminalVideoError = false;
    bus.publish(Events::GameEvent::makeSimple(Events::EventType::TerminalOpened));
    state.scene.terminalAnimationFrame = 0;
    state.scene.terminalTopic = state.encounter.newEntryUnlocked ? Core::kTruthTopic : 0;
    state.scene.terminalPage = 0;
    state.scene.terminalReading = false;
    state.scene.terminalBootFrames = 24;
    state.menu.padCooldown = 12;
    state.weapon.cancelTransient();
    bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
    return;
  }

  const float lookSens = state.player.lookSensitivity;
  const float lookYawDelta = Tyra::AnalogAxis::normalize(rightJoy.h) * lookSens;
  const float lookPitchDelta = -Tyra::AnalogAxis::normalize(rightJoy.v) * lookSens;
  float moveForward = -Tyra::AnalogAxis::normalize(leftJoy.v);
  float moveStrafe = Tyra::AnalogAxis::normalize(leftJoy.h);

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
  state.inputCmd.fireRequested = clicked.R1 && state.inventory.armed();
  state.inputCmd.reloadRequested = clicked.Circle && !state.radio.blocksGameplay() &&
                                  state.inventory.armed();
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
      const int count = state.droid.phase == Core::DroidPhase::GameOver ? 3 : 2;
      state.menu.pauseSelectedIndex =
          (state.menu.pauseSelectedIndex + (upPressed ? count - 1 : 1)) % count;
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuNavigateRequested));
    } else if(clicked.Cross) {
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
      if(state.droid.phase==Core::DroidPhase::GameOver) {
        if (state.menu.pauseSelectedIndex == 0) {
          bus.publish(Events::GameEvent::makeSimple(Events::EventType::BossRetryRequested));
          return;
        }
        state.menu.saveMenuReturnScreen = Core::ScreenState::Defeated;
        state.menu.saveDialog = false;
        state.menu.confirmNewGame = false;
        state.menu.currentScreen = state.menu.pauseSelectedIndex == 1
            ? Core::ScreenState::NewGame : Core::ScreenState::LoadGame;
        bus.publish(Events::GameEvent::makeScreenTransition(static_cast<u8>(state.menu.currentScreen)));
      } else if(state.menu.pauseSelectedIndex==0)
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
      if (state.menu.pauseSelectedIndex == 1) {
        state.menu.settingsReturnScreen = Core::ScreenState::Paused;
        state.menu.currentScreen = Core::ScreenState::Settings;
      } else {
        state.menu.currentScreen = state.menu.pauseSelectedIndex == 0
            ? Core::ScreenState::GameplayFps : Core::ScreenState::MainMenu;
      }
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuConfirmRequested));
    } else if (upPressed || downPressed) {
      state.menu.pauseSelectedIndex = (state.menu.pauseSelectedIndex + (upPressed ? 2 : 1)) % 3;
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
      if (state.menu.mainSelectedIndex < 2) {
        state.menu.saveMenuReturnScreen = Core::ScreenState::MainMenu;
        state.menu.saveDialog = false;
        state.menu.confirmNewGame = false;
      }
      if (state.menu.mainSelectedIndex == 0) {
        state.menu.currentScreen = Core::ScreenState::NewGame;
        bus.publish(Events::GameEvent::makeScreenTransition(
            static_cast<u8>(Core::ScreenState::NewGame)));
      } else if (state.menu.mainSelectedIndex == 1) {
        state.menu.currentScreen = Core::ScreenState::LoadGame;
        bus.publish(Events::GameEvent::makeScreenTransition(
            static_cast<u8>(Core::ScreenState::LoadGame)));
      } else if (state.menu.mainSelectedIndex == 2) {
        state.menu.settingsReturnScreen = Core::ScreenState::MainMenu;
        state.menu.currentScreen = Core::ScreenState::Settings;
        bus.publish(Events::GameEvent::makeScreenTransition(
            static_cast<u8>(Core::ScreenState::Settings)));
      }
    }
  } else if (state.menu.currentScreen == Core::ScreenState::NewGame ||
             state.menu.currentScreen == Core::ScreenState::LoadGame) {
    auto& menu = state.menu;
    const bool newGame = menu.currentScreen == Core::ScreenState::NewGame;
    const auto send = [&](Events::EventType type) {
      bus.publish(Events::GameEvent::makeSimple(type));
    };
    if (clicked.Circle || clicked.Select) {
      send(Events::EventType::MenuBackRequested);
      if (menu.confirmNewGame) {
        menu.confirmNewGame = false;
      } else if (newGame && menu.saveDialog) {
        menu.saveDialog = false;
      } else {
        menu.currentScreen = menu.saveMenuReturnScreen;
        bus.publish(Events::GameEvent::makeScreenTransition(static_cast<u8>(menu.currentScreen)));
      }
    } else if (clicked.Cross || (clicked.Start && !menu.saveDialog)) {
      if (newGame && !menu.saveDialog) {
        send(Events::EventType::MenuConfirmRequested);
        menu.saveDialog = true;
        menu.saveChoice = 0;
        menu.saveStatus = Core::SaveStatus::Checking;
        send(Events::EventType::CheckCardRequested);
      } else if (menu.saveStatus != Core::SaveStatus::Checking) {
        if (!newGame && menu.saveStatus != Core::SaveStatus::Ready) return;
        send(Events::EventType::MenuConfirmRequested);
        if (!newGame) {
          send(Events::EventType::LoadCheckpointRequested);
        } else if (menu.confirmNewGame) {
          send(Events::EventType::NewCampaignRequested);
        } else if (menu.saveChoice == 1) {
          send(Events::EventType::NewCampaignWithoutSaveRequested);
        } else if (!Core::canEnableAutosave(menu.saveStatus)) {
          menu.saveStatus = Core::SaveStatus::Checking;
          send(Events::EventType::CheckCardRequested);
        } else if (menu.saveStatus != Core::SaveStatus::Empty) {
          menu.confirmNewGame = true;
        } else {
          send(Events::EventType::NewCampaignRequested);
        }
      }
    } else if (newGame && menu.saveDialog && !menu.confirmNewGame &&
               menu.saveStatus != Core::SaveStatus::Checking && (upPressed || downPressed)) {
      menu.saveChoice = 1 - menu.saveChoice;
      send(Events::EventType::MenuNavigateRequested);
    }
  } else if (state.menu.currentScreen == Core::ScreenState::Settings) {
    if (clicked.Circle || clicked.Select) {
      bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuBackRequested));
      state.menu.currentScreen = state.menu.settingsReturnScreen;
      bus.publish(Events::GameEvent::makeScreenTransition(
          static_cast<u8>(state.menu.currentScreen)));
      return;
    }
    constexpr int optionCount = Core::MenuNavigationState::kSettingsOptionCount;
    if (upPressed) {
      state.menu.settingsSelectedIndex =
          (state.menu.settingsSelectedIndex + optionCount - 1) % optionCount;
    } else if (downPressed) {
      state.menu.settingsSelectedIndex =
          (state.menu.settingsSelectedIndex + 1) % optionCount;
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
        state.menu.ambientVolume = std::clamp(state.menu.ambientVolume + dir * 10, 0, 100);
        bus.publish(Events::GameEvent::makeSimple(
            Events::EventType::AudioSettingChanged,
            static_cast<s16>(state.menu.ambientVolume)));
      }
    }

  }

  // One click per accepted selection/value change, including analog repeats.
  // Confirm and back keep their own sounds when pressed with a direction.
  const auto& current = state.menu;
  const bool changed = previous.mainSelectedIndex != current.mainSelectedIndex ||
      previous.settingsSelectedIndex != current.settingsSelectedIndex ||
      previous.bgmVolume != current.bgmVolume || previous.sfxVolume != current.sfxVolume ||
      previous.ambientVolume != current.ambientVolume;
  if (changed && previous.currentScreen == current.currentScreen &&
      !clicked.Cross && !clicked.Start && !clicked.Circle && !clicked.Select)
    bus.publish(Events::GameEvent::makeSimple(Events::EventType::MenuNavigateRequested));
}

}  // namespace Input
}  // namespace Systems
}  // namespace DroidCampos
