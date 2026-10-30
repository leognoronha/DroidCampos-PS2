#pragma once

#include <tyra>
#include <array>
#include "core/combat_config.hpp"

namespace DroidCampos {
namespace Core {

constexpr float kDefaultPi = 3.1415926535F;
constexpr float kDefaultEyeHeight = 1.70F;
constexpr const char* kDefaultCodecMessage =
    "DR. CAMPOS, DO YOU READ ME?\n"
    "ETIP PERIMETER IS UNDER LOCKDOWN.\n"
    "CHECK THE GUARD BOOTH AND CARD\n"
    "READER TO BREACH THE MAIN GATE.";

enum class ScreenState : u8 {
  MainMenu = 0,
  NewGame = 1,
  LoadGame = 2,
  Settings = 3,
  GameplayFps = 4,
  Loading = 5,
  Paused = 6
};

enum class SceneId : u8 { Exterior, Interior };
struct DoorState { float openAmount; bool targetOpen; };
struct SceneState {
  SceneId active = SceneId::Exterior;
  SceneId target = SceneId::Exterior;
  bool resident = false;
  bool presented = false;
  bool requested = false;
  int accessFrames = 0;
  int loadStep = 0;
  int totalSteps = 0;
  const char* error = nullptr;
  int nearbyDoor = -1;
  bool terminalNearby = false;
  bool terminalActive = false;
  int terminalTopic = 0;
  int terminalPage = 0;
  bool terminalReading = false;
  int terminalBootFrames = 0;
  DoorState doors[4] = {};
};

struct alignas(16) PlayerKinematicsState {
  Tyra::Vec4 playerPos;
  Tyra::Vec4 cameraPos;
  Tyra::Vec4 cameraLookAt;
  Tyra::Vec4 cameraUp;
  float yaw;
  float pitch;
  float moveSpeed;
  float lookSensitivity;
  float walkBobPhase;
  float walkBobAmount;
  float weaponSwayX;
  float weaponSwayY;
  float verticalVelocity;
  bool grounded;
};

struct alignas(16) InputCommandState {
  float moveForward;
  float moveStrafe;
  float lookYawDelta;
  float lookPitchDelta;
  bool isRunning;
  bool jumpRequested;
  bool fireRequested;
  bool reloadRequested;

  void clear() {
    moveForward = 0.0F;
    moveStrafe = 0.0F;
    lookYawDelta = 0.0F;
    lookPitchDelta = 0.0F;
    isRunning = false;
    jumpRequested = false;
    fireRequested = false;
    reloadRequested = false;
  }
};

enum class RadioPhase : u8 { Idle, Waiting, Alert, Message };

struct RadioCodecState {
  RadioPhase phase = RadioPhase::Idle;
  bool popupActive;
  int visibleChars;
  float typeTimer;
  const char* currentMessage;
};

struct MenuNavigationState {
  ScreenState currentScreen;
  int mainSelectedIndex;
  int loadSelectedIndex;
  int settingsSelectedIndex;
  int bgmVolume;
  int sfxVolume;
  bool scanlinesEnabled;
  bool nearestFilter;
  int padCooldown;
  int activeSaveSlot;
  int pauseSelectedIndex = 0;
};

struct AnimationFrameState {
  float animTick;
  float scanlineY;
  float bgScrollOffset;
};

struct CardReaderState {
  bool isPlayerNearby;
  bool isUnlocked;
};

struct SceneIntroState {
  bool active = false;
  u32 elapsedTicks = 0;
};

constexpr int kMagazineSize = 8;
constexpr u32 kShotIntervalTicks = 3907; // ceil(0.25 * 15625)
constexpr u32 kReloadTicks = 8032; // ceil(0.514 * 15625)
constexpr u32 kBeamTicks = 1250; // 80ms
constexpr u32 kImpactTicks = 1563; // 100ms, rounded up
struct WeaponState {
  int ammo = kMagazineSize;
  u32 cooldownTicks = 0;
  u32 reloadTicks = 0;
  u32 beamTicks = 0;
  u32 impactTicks = 0;
  bool impact = false;
  Tyra::Vec4 beamStart, beamEnd, impactNormal;
  void cancelTransient() {
    cooldownTicks = reloadTicks = beamTicks = impactTicks = 0;
    impact = false;
  }
};

struct EnemyState {
  bool active = false;
  bool moving = false;
  int variant = 0;
  Tyra::Vec4 position = Tyra::Vec4(0,0,0,1);
  float yaw = 0;
  WeaponState weapon;
  u32 animationTicks = 0, shotAnimationTicks = 0;
};

struct alignas(64) WorldState {
  PlayerKinematicsState player;
  InputCommandState inputCmd;
  RadioCodecState radio;
  MenuNavigationState menu;
  AnimationFrameState anim;
  CardReaderState cardReader;
  SceneState scene;
  SceneIntroState intro;
  WeaponState weapon;
  std::array<EnemyState,kMaxEnemies> enemies{};

  void resetPlayerToSpawn() {
    player.playerPos.set(0.0F, kDefaultEyeHeight, 17.0F, 1.0F);
    player.cameraPos.set(0.0F, kDefaultEyeHeight, 17.0F, 1.0F);
    player.cameraLookAt.set(0.0F, kDefaultEyeHeight, 16.0F, 1.0F);
    player.cameraUp.set(0.0F, 1.0F, 0.0F, 0.0F);
    player.yaw = -kDefaultPi * 0.5F;
    player.pitch = 0.04F;
    if (scene.active == SceneId::Interior) {
      player.playerPos.set(0.0F, kDefaultEyeHeight, -1.8F, 1.0F);
      player.cameraPos = player.playerPos;
      player.cameraLookAt.set(0.0F, kDefaultEyeHeight, -2.8F, 1.0F);
      player.pitch = 0.0F;
    }
    player.verticalVelocity = 0.0F;
    player.grounded = true;
    scene.terminalNearby = false;
    scene.terminalActive = false;
    player.walkBobPhase = 0.0F;
    player.walkBobAmount = 0.0F;
    player.weaponSwayX = 0.0F;
    player.weaponSwayY = 0.0F;

    inputCmd.clear();

    cardReader.isPlayerNearby = false;
    cardReader.isUnlocked = false;

    radio.popupActive = true;
    radio.phase = RadioPhase::Waiting;
    radio.visibleChars = 0;
    radio.typeTimer = 0.0F;
    radio.currentMessage = scene.active == SceneId::Interior
        ? "VOCE ESTA DENTRO DA ETIP.\nA ENTRADA FOI SELADA.\nEXPLORE AS SALAS LATERAIS.\nLOCALIZE O TERMINAL CENTRAL."
        : kDefaultCodecMessage;
  }

  void resetToDefaults() {
    weapon = WeaponState{};
    enemies = {};
    scene = SceneState{};
    intro = SceneIntroState{};
    player.moveSpeed = 0.11F;
    player.lookSensitivity = 0.032F;
    radio.currentMessage = kDefaultCodecMessage;
    resetPlayerToSpawn();
    player.pitch = 0.0F;

    menu.currentScreen = ScreenState::MainMenu;
    menu.mainSelectedIndex = 0;
    menu.loadSelectedIndex = 0;
    menu.settingsSelectedIndex = 0;
    menu.bgmVolume = 80;
    menu.sfxVolume = 90;
    menu.scanlinesEnabled = true;
    menu.nearestFilter = true;
    menu.padCooldown = 0;
    menu.activeSaveSlot = 1;

    anim.animTick = 0.0F;
    anim.scanlineY = 0.0F;
    anim.bgScrollOffset = 0.0F;
  }
};

}  // namespace Core
}  // namespace DroidCampos
