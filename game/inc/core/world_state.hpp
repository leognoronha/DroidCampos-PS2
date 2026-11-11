#pragma once

#include <tyra>
#include <array>
#include <algorithm>
#include "core/combat_config.hpp"

namespace DroidCampos {
namespace Core {

constexpr float kDefaultPi = 3.1415926535F;
constexpr float kDefaultEyeHeight = 1.70F;
constexpr const char* kDefaultCodecMessage =
    "PM, ESTA NA ESCUTA?\n"
    "AQUI E O CJE. ESTAMOS NA LINHA.\n"
    "A ENTRADA DA ETIP ESTA BLOQUEADA.\n"
    "PEGUE A ARMA E O CARTAO NA GUARITA.\n"
    "LOCALIZE O LEITOR PERTO DA ENTRADA.\n"
    "E ENTRE NO COMPLEXO. CAMBIO";

enum class ScreenState : u8 {
  MainMenu = 0,
  NewGame = 1,
  LoadGame = 2,
  Settings = 3,
  GameplayFps = 4,
  Loading = 5,
  Paused = 6,
  Defeated = 7
};

enum class SceneId : u8 { Exterior, Interior };
struct DoorState { float openAmount; bool targetOpen; };
struct SceneState {
  SceneId active = SceneId::Exterior;
  SceneId target = SceneId::Exterior;
  bool resident = false;
  bool presented = false;
  bool requested = false;
  u32 accessTicks = 0;
  int loadStep = 0;
  int totalSteps = 0;
  const char* error = nullptr;
  int nearbyDoor = -1;
  bool terminalNearby = false;
  bool terminalActive = false;
  int terminalCodeProgress = 0;
  bool terminalSecretUnlocked = false;
  bool terminalVideoActive = false;
  bool terminalVideoError = false;
  int terminalTopic = 0;
  int terminalPage = 0;
  unsigned int terminalAnimationFrame = 0;
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
  u32 revealTicks = 0;
  const char* currentMessage;
};

struct MenuNavigationState {
  ScreenState currentScreen;
  int mainSelectedIndex;
  int loadSelectedIndex;
  int settingsSelectedIndex;
  ScreenState settingsReturnScreen = ScreenState::MainMenu;
  int bgmVolume;
  int sfxVolume;
  int ambientVolume;
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

enum class Pickup : u8 { Pistol, AccessCard, RearMedkit, SideRoomMedkit, Count };
constexpr int kMedkitCapacity = 2;
constexpr int kMedkitHeal = kPlayerHealth * 40 / 100;

struct InventoryState {
  std::array<bool, static_cast<int>(Pickup::Count)> collected{};
  int medkits = 0;

  bool has(Pickup item) const { return collected[static_cast<int>(item)]; }
  bool armed() const { return has(Pickup::Pistol); }
  bool canEnter() const { return armed() && has(Pickup::AccessCard); }
  bool collect(Pickup item) {
    if (item == Pickup::Count || has(item)) return false;
    if (item == Pickup::RearMedkit || item == Pickup::SideRoomMedkit) {
      if (medkits >= kMedkitCapacity) return false;
      ++medkits;
    }
    collected[static_cast<int>(item)] = true;
    return true;
  }
  bool useMedkit(int& health) {
    if (medkits <= 0 || health <= 0 || health >= kPlayerHealth) return false;
    health = std::min(kPlayerHealth, health + kMedkitHeal);
    --medkits;
    return true;
  }
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
  Tyra::Vec4 beamStart = Tyra::Vec4(0,0,0,1);
  Tyra::Vec4 beamEnd = Tyra::Vec4(0,0,0,1);
  Tyra::Vec4 impactNormal = Tyra::Vec4(0,1,0,0);
  void cancelTransient() {
    cooldownTicks = reloadTicks = beamTicks = impactTicks = 0;
    impact = false;
  }
};

struct EnemyState {
  bool active = false;
  bool moving = false;
  bool canShoot = false;
  int variant = 0;
  int health = kEnemyHealth;
  EnemyBehavior behavior = EnemyBehavior::Alert;
  Tyra::Vec4 position = Tyra::Vec4(0,0,0,1);
  float yaw = 0;
  WeaponState weapon;
  u32 alertTicks = kEnemyReaction;
  u32 fireTicks = 0, pathTicks = 0, stuckTicks = 0;
  u32 animationTicks = 0, shotAnimationTicks = 0, hitTicks = 0;
  u32 randomState = 1;
};
enum class DroidPhase : u8 { Inactive, ButtonRising, ButtonReady, Release, Pursuit, GameOver };
constexpr float kDroidRadius=.60F;
constexpr u32 kDroidSecond=15625;
struct DroidEncounterState {
  DroidPhase phase=DroidPhase::Inactive;
  u32 phaseTicks=0, animationTicks=0, attackTicks=0, pathTicks=0;
  bool buttonNearby=false, attacking=false, impactDone=false, moving=false, crouched=false, passage=false;
  Tyra::Vec4 position=Tyra::Vec4(3.58F,1.237F,-29.F,1);
  int passageDoor=-1, passageSide=1;
  float yaw=-1.570796327F;
  float opening() const {
    return phase==DroidPhase::Release ? std::min(1.F,phaseTicks/(3.F*kDroidSecond)) :
      (phase==DroidPhase::Pursuit || phase==DroidPhase::GameOver ? 1.F : 0.F);
  }
  bool mobile() const {return phase==DroidPhase::Pursuit || phase==DroidPhase::GameOver ||
    (phase==DroidPhase::Release && phaseTicks>=3*kDroidSecond);}
  float shake() const {
    if(phase!=DroidPhase::Release)return 0;
    float t=phaseTicks/float(kDroidSecond);
    return t<3 ? t/3 : std::max(0.F,(6-t)/3);
  }
};
struct EncounterState {
  EncounterPhase phase = EncounterPhase::Dormant;
  int wave = 0, remaining = 0;
  u32 countdownTicks = 0, attackTicks = 0;
  u32 completionNoticeTicks = 0;
  int attackCursor = 0;
  bool checkpointValid = false;
  bool newEntryUnlocked = false, objectiveComplete = false;
  bool finalRadioPending = false;
  PlayerKinematicsState checkpointPlayer{};
  DoorState checkpointDoors[4]{};
  InventoryState checkpointInventory{};
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
  InventoryState inventory;
  Pickup nearbyPickup = Pickup::Count;
  int playerHealth = kPlayerHealth;
  u32 damageFlashTicks = 0;
  std::array<EnemyState,kMaxEnemies> enemies{};
  EncounterState encounter;
  DroidEncounterState droid;

  bool playerMovementActive() const {
    return menu.currentScreen == ScreenState::GameplayFps && scene.resident &&
        scene.presented && !scene.requested && !scene.accessTicks &&
        !intro.active && !scene.terminalActive && playerHealth>0;
  }
  bool simulationActive() const {
    return playerMovementActive() && !radio.popupActive;
  }

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
    nearbyPickup = Pickup::Count;

    radio.popupActive = true;
    radio.phase = RadioPhase::Waiting;
    radio.visibleChars = 0;
    radio.revealTicks = 0;
    radio.currentMessage = scene.active == SceneId::Interior
        ? "VOCE ESTA DENTRO DA ETIP.\nA ENTRADA FOI SELADA.\nEXPLORE AS SALAS LATERAIS.\nLOCALIZE O TERMINAL CENTRAL."
        : kDefaultCodecMessage;
  }

  void resetToDefaults() {
    weapon = WeaponState{};
    inventory = InventoryState{};
    playerHealth = kPlayerHealth;
    damageFlashTicks = 0;
    enemies = {};
    encounter = EncounterState{};
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
    menu.settingsReturnScreen = ScreenState::MainMenu;
    menu.bgmVolume = 80;
    menu.sfxVolume = 90;
    menu.ambientVolume = 70;
    menu.scanlinesEnabled = true;
    menu.nearestFilter = true;
    menu.padCooldown = 0;
    menu.activeSaveSlot = 1;

    droid = DroidEncounterState{};
    anim.animTick = 0.0F;
    anim.scanlineY = 0.0F;
    anim.bgScrollOffset = 0.0F;
  }
};

}  // namespace Core
}  // namespace DroidCampos
