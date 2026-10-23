#include "droid_campos_game.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace DroidCampos {

using namespace Tyra;

namespace {
constexpr float kPi = 3.1415926535F;
constexpr float kEyeHeight = 1.70F;
constexpr float kMaxPitch = 1.28F;
constexpr float kPlayerRadius = 0.48F;

// Test radio message (Metal Gear Codec style)
constexpr const char* kRadioTestMessage =
    "DR. CAMPOS, DO YOU READ ME?\n"
    "ETIP PERIMETER IS UNDER LOCKDOWN.\n"
    "CHECK THE GUARD BOOTH AND CARD\n"
    "READER TO BREACH THE MAIN GATE.";

struct BoxCollider2D {
  float minX;
  float maxX;
  float minZ;
  float maxZ;
};

constexpr BoxCollider2D kWorldObstacles[] = {
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

DroidCamposGame::DroidCamposGame(Engine* t_engine)
    : engine(t_engine),
      scenarioLoaded(false),
      mistLoaded(false),
      pistolLoaded(false),
      playerPos(0.0F, kEyeHeight, 17.0F, 1.0F),
      cameraPos(0.0F, kEyeHeight, 17.0F, 1.0F),
      cameraLookAt(0.0F, kEyeHeight, 16.0F, 1.0F),
      cameraUp(0.0F, 1.0F, 0.0F, 0.0F),
      cameraYaw(-kPi * 0.5F),
      cameraPitch(0.0F),
      moveSpeed(0.11F),
      lookSensitivity(0.032F),
      walkBobPhase(0.0F),
      walkBobAmount(0.0F),
      weaponSwayX(0.0F),
      weaponSwayY(0.0F),
      radioPopupActive(true),
      radioVisibleChars(0),
      radioTypeTimer(0.0F),
      currentScreen(MenuScreen::MAIN_MENU),
      mainSelectedIndex(0),
      loadSelectedIndex(0),
      settingsSelectedIndex(0),
      bgmVolume(80),
      sfxVolume(90),
      scanlinesEnabled(true),
      nearestFilter(true),
      padCooldown(0),
      animTick(0.0F),
      scanlineY(0.0F),
      bgScrollOffset(0.0F),
      activeSaveSlot(1) {}

DroidCamposGame::~DroidCamposGame() {
  auto& repo = engine->renderer.getTextureRepository();
  if (scenarioMesh != nullptr) {
    repo.freeByMesh(scenarioMesh.get());
  }
  if (mistMesh != nullptr) {
    repo.freeByMesh(mistMesh.get());
  }
  if (pistolMesh != nullptr) {
    repo.freeByMesh(pistolMesh.get());
  }
  repo.freeBySprite(bgSprite);
  repo.freeBySprite(uiRectSprite);
  font.free(repo);
}

void DroidCamposGame::init() {
  engine->renderer.setClearScreenColor(Color(12.0F, 15.0F, 22.0F));
  engine->renderer.core.renderer2D.setTextureMappingType(TyraNearest);

  stapip.setRenderer(&engine->renderer.core);
  loadResources();
  loadScenario3D();
  loadMist3D();
  loadPistol3D();
  resetPlayerToSpawn();

  TYRA_LOG("DroidCampos-PS2 initialized (FPS + Pistol + Radio + Fog Ready)!");
}

void DroidCamposGame::loadResources() {
  auto& repo = engine->renderer.getTextureRepository();

  font.load(repo, &engine->renderer.renderer2D);

  bgSprite.mode = SpriteMode::MODE_STRETCH;
  bgSprite.size = Vec2(512.0F, 448.0F);
  bgSprite.position = Vec2(0.0F, 0.0F);
  bgSprite.color = Color(128.0F, 128.0F, 128.0F, 128.0F);
  auto* bgTex = repo.add(FileUtils::fromCwd("bg_cyber.png"));
  bgTex->addLink(bgSprite.id);

  uiRectSprite.mode = SpriteMode::MODE_STRETCH;
  uiRectSprite.size = Vec2(16.0F, 16.0F);
  uiRectSprite.position = Vec2(0.0F, 0.0F);
  auto* whiteTex = repo.add(FileUtils::fromCwd("ui_white.png"));
  whiteTex->addLink(uiRectSprite.id);
}

void DroidCamposGame::loadScenario3D() {
  ObjLoaderOptions options;
  options.scale = 1.0F;
  options.flipUVs = true;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("etip/etip_cenario.obj"), options);
  scenarioMesh = std::make_unique<StaticMesh>(data.get());

  auto& repo = engine->renderer.getTextureRepository();
  repo.addByMesh(scenarioMesh.get(), FileUtils::fromCwd("etip/"), "png");

  for (auto* mat : scenarioMesh->materials) {
    mat->ambient.set(128.0F, 128.0F, 128.0F, 128.0F);
  }

  scenarioOptions.frustumCulling = PipelineFrustumCulling_Precise;
  scenarioOptions.fullClipChecks = true;
  scenarioOptions.textureMappingType = TyraNearest;
  scenarioOptions.transformationType = TyraMVP;

  scenarioLoaded = true;
  TYRA_LOG("ETIP 3D Scenario loaded successfully!");
}

void DroidCamposGame::loadMist3D() {
  ObjLoaderOptions options;
  options.scale = 1.0F;
  options.flipUVs = true;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("etip/etip_nevoa.obj"), options);
  mistMesh = std::make_unique<StaticMesh>(data.get());

  auto& repo = engine->renderer.getTextureRepository();
  repo.addByMesh(mistMesh.get(), FileUtils::fromCwd("etip/"), "png");

  for (auto* mat : mistMesh->materials) {
    // Soft translucent cool mist color modulated with 32-bit RGBA texture alpha
    mat->ambient.set(118.0F, 126.0F, 134.0F, 88.0F);
  }

  // Render in strict back-to-front order with bilinear filtering & alpha blending
  mistOptions.transformationType = TyraMVP;
  mistOptions.frustumCulling = PipelineFrustumCulling_None;
  mistOptions.fullClipChecks = false;
  mistOptions.textureMappingType = TyraLinear;
  mistOptions.blendingEnabled = true;
  mistOptions.zTestType = PipelineZTest_Standard;

  mistLoaded = true;
  TYRA_LOG("Blender 3D Atmospheric Mist loaded successfully!");
}

void DroidCamposGame::loadPistol3D() {
  ObjLoaderOptions options;
  options.scale = 1.0F;
  options.flipUVs = true;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("pistol/pistol_scifi.obj"), options);
  pistolMesh = std::make_unique<StaticMesh>(data.get());

  auto& repo = engine->renderer.getTextureRepository();
  repo.addByMesh(pistolMesh.get(), FileUtils::fromCwd("pistol/"), "png");

  for (auto* mat : pistolMesh->materials) {
    mat->ambient.set(126.0F, 128.0F, 132.0F, 128.0F);
  }

  // Render pistol in Camera/View space (TyraMP) so it stays locked to FPS hand
  pistolOptions.transformationType = TyraMP;
  pistolOptions.frustumCulling = PipelineFrustumCulling_None;
  pistolOptions.fullClipChecks = false;
  pistolOptions.textureMappingType = TyraNearest;
  pistolOptions.zTestType = PipelineZTest_Standard;

  pistolLoaded = true;
  updatePistolTransform();
  TYRA_LOG("Sci-Fi Low-Poly Pistol loaded successfully!");
}

void DroidCamposGame::resetPlayerToSpawn() {
  playerPos.set(0.0F, kEyeHeight, 17.0F, 1.0F);
  cameraYaw = -kPi * 0.5F;
  cameraPitch = 0.04F;
  walkBobPhase = 0.0F;
  walkBobAmount = 0.0F;
  weaponSwayX = 0.0F;
  weaponSwayY = 0.0F;

  // Trigger incoming Metal Gear-style radio transmission at mission start
  radioPopupActive = true;
  radioVisibleChars = 0;
  radioTypeTimer = 0.0F;

  updateCameraVectors();
  updatePistolTransform();
}

void DroidCamposGame::updateCameraVectors() {
  // Subtle natural head-bob on camera while walking
  const float bobVertical =
      std::sin(walkBobPhase * 2.0F) * 0.042F * walkBobAmount;
  const float bobLateral = std::cos(walkBobPhase) * 0.022F * walkBobAmount;

  const float rightX = -std::sin(cameraYaw);
  const float rightZ = std::cos(cameraYaw);

  cameraPos.set(playerPos.x + rightX * bobLateral, kEyeHeight + bobVertical,
                playerPos.z + rightZ * bobLateral, 1.0F);

  const float cosPitch = std::cos(cameraPitch);
  const float dirX = cosPitch * std::cos(cameraYaw);
  const float dirY = std::sin(cameraPitch);
  const float dirZ = cosPitch * std::sin(cameraYaw);

  cameraLookAt.set(cameraPos.x + dirX, cameraPos.y + dirY, cameraPos.z + dirZ,
                   1.0F);
}

void DroidCamposGame::updatePistolTransform() {
  if (!pistolLoaded || pistolMesh == nullptr) {
    return;
  }

  // Lissajous figure-8 walking bob + idle breathing + camera look inertia sway
  const float idleBreath = std::sin(animTick * 0.75F) * 0.0035F;
  const float bobX = std::cos(walkBobPhase) * 0.015F * walkBobAmount;
  const float bobY =
      -std::fabs(std::sin(walkBobPhase)) * 0.018F * walkBobAmount;
  const float bobZ = std::sin(walkBobPhase * 2.0F) * 0.008F * walkBobAmount;

  pistolMesh->translation.identity();
  pistolMesh->rotation.identity();
  pistolMesh->scale.identity();

  // Position in lower-right FPS hand (View Space: +X right, -Y down, -Z forward)
  pistolMesh->translation.translate(
      Vec4(0.145F + bobX + weaponSwayX,
           -0.135F + bobY + weaponSwayY + idleBreath, -0.320F + bobZ, 1.0F));

  // Slight natural inward angle toward center crosshair + dynamic sway tilt
  pistolMesh->rotation.rotateY(0.11F - weaponSwayX * 1.8F);
  pistolMesh->rotation.rotateX(0.03F + weaponSwayY * 1.6F + bobY * 0.9F);
  pistolMesh->rotation.rotateZ(-bobX * 1.4F - weaponSwayX * 1.2F);
}

void DroidCamposGame::updateRadioTransmission() {
  if (!radioPopupActive) {
    return;
  }

  const int totalLen = static_cast<int>(std::strlen(kRadioTestMessage));
  if (radioVisibleChars < totalLen) {
    radioTypeTimer += 0.65F;
    if (radioTypeTimer >= 1.0F) {
      int step = static_cast<int>(radioTypeTimer);
      radioTypeTimer -= static_cast<float>(step);
      radioVisibleChars += step;
      if (radioVisibleChars > totalLen) {
        radioVisibleChars = totalLen;
      }
    }
  }
}

void DroidCamposGame::resolveWorldCollisions(float& x, float& z) const {
  if (x < -5.25F) x = -5.25F;
  if (x > 5.25F) x = 5.25F;
  if (z > 19.30F) z = 19.30F;
  if (z < 3.20F) z = 3.20F;

  for (const auto& box : kWorldObstacles) {
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

      if (axis == 0)
        x = expMinX;
      else if (axis == 1)
        x = expMaxX;
      else if (axis == 2)
        z = expMinZ;
      else
        z = expMaxZ;
    }
  }
}

void DroidCamposGame::updateFpsMovement() {
  const auto& pad = engine->pad;
  const auto& pressed = pad.getPressed();
  const auto& leftJoy = pad.getLeftJoyPad();
  const auto& rightJoy = pad.getRightJoyPad();

  // --- 1. CAMERA LOOK (Left / Right / Up / Down) ---
  float lookYawDelta = 0.0F;
  float lookPitchDelta = 0.0F;

  const float rx = (static_cast<float>(rightJoy.h) - 128.0F) / 128.0F;
  const float ry = (static_cast<float>(rightJoy.v) - 128.0F) / 128.0F;

  if (std::fabs(rx) > 0.18F) {
    lookYawDelta += rx * lookSensitivity;
  }
  if (std::fabs(ry) > 0.18F) {
    lookPitchDelta -= ry * lookSensitivity;
  }

  // Face Buttons look (note: while radio popup is open, 'X'/Cross closes radio instead of looking down)
  if (pressed.Square) lookYawDelta -= lookSensitivity * 0.85F;
  if (pressed.Circle) lookYawDelta += lookSensitivity * 0.85F;
  if (pressed.Triangle) lookPitchDelta += lookSensitivity * 0.75F;
  if (!radioPopupActive && pressed.Cross) {
    lookPitchDelta -= lookSensitivity * 0.75F;
  }

  cameraYaw += lookYawDelta;
  if (cameraYaw > kPi) cameraYaw -= 2.0F * kPi;
  if (cameraYaw < -kPi) cameraYaw += 2.0F * kPi;

  cameraPitch += lookPitchDelta;
  if (cameraPitch > kMaxPitch) cameraPitch = kMaxPitch;
  if (cameraPitch < -kMaxPitch) cameraPitch = -kMaxPitch;

  // Inertial weapon sway when turning head/camera
  weaponSwayX = weaponSwayX * 0.80F - lookYawDelta * 0.35F;
  weaponSwayY = weaponSwayY * 0.80F - lookPitchDelta * 0.35F;
  if (weaponSwayX > 0.032F) weaponSwayX = 0.032F;
  if (weaponSwayX < -0.032F) weaponSwayX = -0.032F;
  if (weaponSwayY > 0.028F) weaponSwayY = 0.028F;
  if (weaponSwayY < -0.028F) weaponSwayY = -0.028F;

  // --- 2. PLAYER WALKING & STRAFING + HEAD/WEAPON BOB ---
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

  if (pressed.DpadUp) moveForward += 1.0F;
  if (pressed.DpadDown) moveForward -= 1.0F;
  if (pressed.DpadLeft) moveStrafe -= 1.0F;
  if (pressed.DpadRight) moveStrafe += 1.0F;

  float moveMag =
      std::sqrt(moveForward * moveForward + moveStrafe * moveStrafe);
  if (moveMag > 1.0F) {
    const float invMag = 1.0F / moveMag;
    moveForward *= invMag;
    moveStrafe *= invMag;
    moveMag = 1.0F;
  }

  const bool isRunning = (pressed.L1 || pressed.R1);
  const float speedMult = isRunning ? 1.55F : 1.0F;
  const float step = moveSpeed * speedMult;

  if (moveMag > 0.05F) {
    walkBobPhase += (isRunning ? 0.21F : 0.14F) * moveMag;
    if (walkBobPhase > 2.0F * kPi) {
      walkBobPhase -= 2.0F * kPi;
    }
    const float targetBob = moveMag * (isRunning ? 1.25F : 1.0F);
    walkBobAmount = walkBobAmount * 0.82F + targetBob * 0.18F;
  } else {
    walkBobAmount *= 0.80F;
    if (walkBobAmount < 0.005F) {
      walkBobAmount = 0.0F;
    }
  }

  const float fwdX = std::cos(cameraYaw);
  const float fwdZ = std::sin(cameraYaw);
  const float rightX = -fwdZ;
  const float rightZ = fwdX;

  float nextX = playerPos.x + (fwdX * moveForward + rightX * moveStrafe) * step;
  float nextZ = playerPos.z + (fwdZ * moveForward + rightZ * moveStrafe) * step;

  resolveWorldCollisions(nextX, nextZ);

  playerPos.x = nextX;
  playerPos.y = kEyeHeight;
  playerPos.z = nextZ;

  updateCameraVectors();
  updatePistolTransform();
}

void DroidCamposGame::loop() {
  handleInput();
  updateAnimations();

  auto& renderer = engine->renderer;
  renderer.core.renderer2D.setTextureMappingType(nearestFilter ? TyraNearest
                                                               : TyraLinear);

  if (currentScreen == MenuScreen::GAMEPLAY_FPS) {
    renderer.setClearScreenColor(Color(16.0F, 20.0F, 30.0F));
    updateFpsMovement();
    updateRadioTransmission();

    CameraInfo3D camInfo(&cameraPos, &cameraLookAt, &cameraUp);
    renderer.beginFrame(camInfo);
    renderGameplayFps();
    renderFpsHud();
    if (radioPopupActive) {
      renderRadioPopup();
    }
    renderer.endFrame();
    return;
  }

  renderer.setClearScreenColor(Color(12.0F, 15.0F, 22.0F));
  renderer.beginFrame();

  renderBackground();
  renderHeader();

  switch (currentScreen) {
    case MenuScreen::MAIN_MENU:
      renderMainMenu();
      renderFooter("DPAD: NAVIGATE   CROSS/START: SELECT   AXIOM-OS V4.09");
      break;
    case MenuScreen::NEW_GAME_SCREEN:
      renderNewGameScreen();
      renderFooter("CROSS/START: ENTER ETIP 3D FPS   CIRCLE: BACK");
      break;
    case MenuScreen::LOAD_GAME_SCREEN:
      renderLoadGameScreen();
      renderFooter("DPAD: SELECT SLOT   CROSS: LOAD 3D FPS   CIRCLE: BACK");
      break;
    case MenuScreen::SETTINGS_SCREEN:
      renderSettingsScreen();
      renderFooter("DPAD UP/DOWN: OPTION   LEFT/RIGHT: ADJUST   CIRCLE: BACK");
      break;
    case MenuScreen::GAMEPLAY_FPS:
      break;
  }

  renderer.endFrame();
}

void DroidCamposGame::updateAnimations() {
  animTick += 0.06F;
  scanlineY += 1.8F;
  if (scanlineY > 448.0F) {
    scanlineY = 0.0F;
  }

  bgScrollOffset += 0.25F;
  if (bgScrollOffset >= 32.0F) {
    bgScrollOffset = 0.0F;
  }
}

void DroidCamposGame::handleInput() {
  if (padCooldown > 0) {
    padCooldown--;
  }

  const auto& clicked = engine->pad.getClicked();
  const auto& joy = engine->pad.getLeftJoyPad();

  if (currentScreen == MenuScreen::GAMEPLAY_FPS) {
    // Dismiss the Metal Gear-style radio popup when pressing 'X' (Cross)
    if (radioPopupActive && clicked.Cross) {
      radioPopupActive = false;
      return;
    }
    // Allow re-triggering the radio test popup anytime with L2 or R2
    if (!radioPopupActive && (clicked.L2 || clicked.R2)) {
      radioPopupActive = true;
      radioVisibleChars = 0;
      radioTypeTimer = 0.0F;
    }
    if (clicked.Select) {
      currentScreen = MenuScreen::MAIN_MENU;
    } else if (clicked.L3 || clicked.R3) {
      resetPlayerToSpawn();
    }
    return;
  }

  bool upPressed = clicked.DpadUp || (padCooldown == 0 && joy.v <= 70);
  bool downPressed = clicked.DpadDown || (padCooldown == 0 && joy.v >= 185);
  bool leftPressed = clicked.DpadLeft || (padCooldown == 0 && joy.h <= 70);
  bool rightPressed = clicked.DpadRight || (padCooldown == 0 && joy.h >= 185);

  if (upPressed || downPressed || leftPressed || rightPressed) {
    padCooldown = 12;
  }

  if (currentScreen == MenuScreen::MAIN_MENU) {
    if (upPressed) {
      mainSelectedIndex = (mainSelectedIndex + 2) % 3;
    } else if (downPressed) {
      mainSelectedIndex = (mainSelectedIndex + 1) % 3;
    }

    if (clicked.Cross || clicked.Start) {
      if (mainSelectedIndex == 0) {
        currentScreen = MenuScreen::NEW_GAME_SCREEN;
      } else if (mainSelectedIndex == 1) {
        currentScreen = MenuScreen::LOAD_GAME_SCREEN;
      } else if (mainSelectedIndex == 2) {
        currentScreen = MenuScreen::SETTINGS_SCREEN;
      }
    }
  } else if (currentScreen == MenuScreen::NEW_GAME_SCREEN) {
    if (clicked.Cross || clicked.Start) {
      activeSaveSlot = 0;
      resetPlayerToSpawn();
      currentScreen = MenuScreen::GAMEPLAY_FPS;
    } else if (clicked.Circle || clicked.Select) {
      currentScreen = MenuScreen::MAIN_MENU;
    }
  } else if (currentScreen == MenuScreen::LOAD_GAME_SCREEN) {
    if (upPressed) {
      loadSelectedIndex = (loadSelectedIndex + 2) % 3;
    } else if (downPressed) {
      loadSelectedIndex = (loadSelectedIndex + 1) % 3;
    }

    if (clicked.Cross || clicked.Start) {
      if (loadSelectedIndex < 2) {
        activeSaveSlot = loadSelectedIndex + 1;
        resetPlayerToSpawn();
        currentScreen = MenuScreen::GAMEPLAY_FPS;
      }
    } else if (clicked.Circle || clicked.Select) {
      currentScreen = MenuScreen::MAIN_MENU;
    }
  } else if (currentScreen == MenuScreen::SETTINGS_SCREEN) {
    if (upPressed) {
      settingsSelectedIndex = (settingsSelectedIndex + 3) % 4;
    } else if (downPressed) {
      settingsSelectedIndex = (settingsSelectedIndex + 1) % 4;
    }

    if (leftPressed || rightPressed || clicked.Cross) {
      int dir = rightPressed || clicked.Cross ? 1 : -1;
      if (settingsSelectedIndex == 0) {
        bgmVolume += dir * 10;
        if (bgmVolume < 0) bgmVolume = 0;
        if (bgmVolume > 100) bgmVolume = 100;
      } else if (settingsSelectedIndex == 1) {
        sfxVolume += dir * 10;
        if (sfxVolume < 0) sfxVolume = 0;
        if (sfxVolume > 100) sfxVolume = 100;
      } else if (settingsSelectedIndex == 2) {
        scanlinesEnabled = !scanlinesEnabled;
      } else if (settingsSelectedIndex == 3) {
        nearestFilter = !nearestFilter;
      }
    }

    if (clicked.Circle || clicked.Select) {
      currentScreen = MenuScreen::MAIN_MENU;
    }
  }
}

void DroidCamposGame::renderGameplayFps() {
  engine->renderer.renderer3D.usePipeline(stapip);

  if (scenarioLoaded && scenarioMesh != nullptr) {
    stapip.render(scenarioMesh.get(), scenarioOptions);
  }

  if (mistLoaded && mistMesh != nullptr) {
    stapip.render(mistMesh.get(), mistOptions);
  }

  if (pistolLoaded && pistolMesh != nullptr) {
    stapip.render(pistolMesh.get(), pistolOptions);
  }
}

void DroidCamposGame::renderAnonymousCodecPortrait(float x, float y,
                                                   bool speaking) {
  // Metal Gear Solid-style Codec Portrait Box on the right side
  drawPanel(x, y, 98.0F, 114.0F, Color(6.0F, 18.0F, 14.0F, 122.0F),
            Color(40.0F, 210.0F, 145.0F, 125.0F));

  // Inner dark green/cyan CRT screen background
  drawRect(x + 4.0F, y + 4.0F, 90.0F, 90.0F, Color(8.0F, 26.0F, 20.0F, 128.0F));

  const float cx = x + 49.0F;
  const float cy = y + 48.0F;

  // Anonymous Shadowed Silhouette (shoulders, high tactical collar, hooded head)
  // Shoulders & tactical coat collar
  drawRect(cx - 36.0F, cy + 20.0F, 72.0F, 24.0F,
           Color(16.0F, 54.0F, 42.0F, 128.0F));
  drawRect(cx - 26.0F, cy + 10.0F, 52.0F, 14.0F,
           Color(22.0F, 72.0F, 56.0F, 128.0F));

  // Outer hood / shadowy head contour
  drawRect(cx - 22.0F, cy - 28.0F, 44.0F, 40.0F,
           Color(26.0F, 86.0F, 68.0F, 128.0F));
  drawRect(cx - 18.0F, cy - 32.0F, 36.0F, 6.0F,
           Color(32.0F, 102.0F, 80.0F, 128.0F));

  // Deep shadow obscuring the face (anonymous informant aesthetic)
  drawRect(cx - 16.0F, cy - 22.0F, 32.0F, 28.0F,
           Color(6.0F, 18.0F, 14.0F, 128.0F));

  // Subtle glinting encrypted eyes / visor slits in the shadow
  drawRect(cx - 11.0F, cy - 12.0F, 8.0F, 3.0F,
           Color(90.0F, 245.0F, 185.0F, 128.0F));
  drawRect(cx + 3.0F, cy - 12.0F, 8.0F, 3.0F,
           Color(90.0F, 245.0F, 185.0F, 128.0F));

  // Nose bridge & lower jaw shadow highlight
  drawRect(cx - 1.0F, cy - 9.0F, 2.0F, 7.0F,
           Color(22.0F, 75.0F, 58.0F, 128.0F));

  // Animated voice waveform / mouth transmission bar while text is printing
  if (speaking && (static_cast<int>(animTick * 4.0F) % 2 == 0)) {
    drawRect(cx - 7.0F, cy + 1.0F, 14.0F, 3.0F,
             Color(70.0F, 230.0F, 165.0F, 128.0F));
  } else {
    drawRect(cx - 5.0F, cy + 2.0F, 10.0F, 1.0F,
             Color(28.0F, 95.0F, 72.0F, 128.0F));
  }

  // CRT Codec Scanlines across portrait
  for (int sl = 0; sl < 9; sl++) {
    drawRect(x + 4.0F, y + 8.0F + static_cast<float>(sl) * 10.0F, 90.0F, 1.0F,
             Color(0.0F, 10.0F, 8.0F, 55.0F));
  }

  // Rolling interference bar inside portrait
  const float glitchY =
      y + 6.0F + std::fmod(animTick * 9.0F, 84.0F);
  drawRect(x + 4.0F, glitchY, 90.0F, 2.0F,
           Color(90.0F, 255.0F, 190.0F, 36.0F));

  // Bottom label inside portrait frame: "UNKNOWN"
  font.drawText("UNKNOWN", static_cast<int>(x + 16.0F),
                static_cast<int>(y + 96.0F),
                Color(80.0F, 240.0F, 175.0F, 128.0F), 0.9F);
}

void DroidCamposGame::renderRadioPopup() {
  // Main Metal Gear-style Radio / Codec Popup Window
  const float px = 18.0F;
  const float py = 266.0F;
  const float pw = 476.0F;
  const float ph = 136.0F;

  drawPanel(px, py, pw, ph, Color(6.0F, 16.0F, 14.0F, 116.0F),
            Color(45.0F, 215.0F, 150.0F, 122.0F));

  // Header strip (PTT / Frequency 140.85)
  drawRect(px + 8.0F, py + 6.0F, 356.0F, 18.0F,
           Color(12.0F, 38.0F, 30.0F, 120.0F));
  font.drawText("PTT // FREQ 140.85   [ENCRYPTED CODEC]",
                static_cast<int>(px + 14.0F), static_cast<int>(py + 8.0F),
                Color(90.0F, 245.0F, 180.0F, 128.0F), 0.95F);

  // Animated signal strength bars
  for (int b = 0; b < 5; b++) {
    const float bh = 3.0F + static_cast<float>(b) * 2.5F;
    const float bx = px + 328.0F + static_cast<float>(b) * 6.0F;
    const float by = py + 20.0F - bh;
    drawRect(bx, by, 4.0F, bh, Color(80.0F, 245.0F, 175.0F, 128.0F));
  }

  // Typewriter text area on the left (x = 28..370)
  char partialBuf[256];
  int copyLen = radioVisibleChars;
  if (copyLen > static_cast<int>(sizeof(partialBuf)) - 1) {
    copyLen = static_cast<int>(sizeof(partialBuf)) - 1;
  }
  std::memcpy(partialBuf, kRadioTestMessage, static_cast<size_t>(copyLen));
  partialBuf[copyLen] = '\0';

  font.drawText(partialBuf, static_cast<int>(px + 14.0F),
                static_cast<int>(py + 32.0F),
                Color(205.0F, 255.0F, 225.0F, 128.0F), 1.0F);

  const int totalLen = static_cast<int>(std::strlen(kRadioTestMessage));
  const bool stillTyping = (radioVisibleChars < totalLen);

  // Blinking prompt to dismiss radio with 'X'
  const float pulse = (std::sin(animTick * 4.5F) + 1.0F) * 0.5F;
  drawRect(px + 10.0F, py + 110.0F, 354.0F, 18.0F,
           Color(10.0F, 32.0F, 26.0F, 110.0F));
  font.drawText(
      "[ PRESS 'X' TO CLOSE RADIO TRANSMISSION ]", static_cast<int>(px + 22.0F),
      static_cast<int>(py + 112.0F),
      Color(120.0F + pulse * 135.0F, 245.0F, 175.0F, 128.0F), 0.95F);

  // Fixed Anonymous Portrait on the right corner of the radio popup
  renderAnonymousCodecPortrait(px + 368.0F, py + 11.0F, stillTyping);
}

void DroidCamposGame::renderFpsHud() {
  // Center First-Person Reticle (Crosshair)
  const float cx = 256.0F;
  const float cy = 224.0F;
  const Color crosshairCol(0.0F, 235.0F, 255.0F, 110.0F);
  drawRect(cx - 8.0F, cy - 1.0F, 5.0F, 2.0F, crosshairCol);
  drawRect(cx + 3.0F, cy - 1.0F, 5.0F, 2.0F, crosshairCol);
  drawRect(cx - 1.0F, cy - 8.0F, 2.0F, 5.0F, crosshairCol);
  drawRect(cx - 1.0F, cy + 3.0F, 2.0F, 5.0F, crosshairCol);

  // Compact Top Tactical Telemetry Bar
  drawPanel(12.0F, 10.0F, 488.0F, 26.0F, Color(8.0F, 12.0F, 18.0F, 92.0F),
            Color(0.0F, 180.0F, 220.0F, 95.0F));

  const char* zoneLabel = "ZONA B // VIA DE ACESSO";
  if (playerPos.z > 13.0F) {
    zoneLabel = "ZONA A // PONTO INICIAL";
  } else if (playerPos.z < 6.5F) {
    zoneLabel = "ZONA C // PORTAO & CONTROLE ETIP";
  }

  font.drawText(zoneLabel, 22, 16, Color(240.0F, 185.0F, 40.0F, 128.0F), 1.0F);

  char posBuf[48];
  std::snprintf(posBuf, sizeof(posBuf), "POS X:%.1f Z:%.1f", playerPos.x,
                -playerPos.z);
  font.drawText(posBuf, 350, 16, Color(140.0F, 235.0F, 255.0F, 128.0F), 1.0F);

  // Bottom Controls Bar
  drawPanel(12.0F, 412.0F, 488.0F, 24.0F, Color(8.0F, 12.0F, 18.0F, 92.0F),
            Color(80.0F, 100.0F, 120.0F, 85.0F));
  if (radioPopupActive) {
    font.drawText(
        "X: CLOSE RADIO   L-STICK/DPAD: WALK   R-STICK: LOOK   SELECT: MENU",
        20, 417, Color(140.0F, 245.0F, 190.0F, 128.0F), 0.95F);
  } else {
    font.drawText(
        "L-STICK/DPAD: WALK   R-STICK/BTNS: LOOK   L2: RADIO   SELECT: MENU",
        20, 417, Color(190.0F, 215.0F, 230.0F, 128.0F), 0.95F);
  }
}

void DroidCamposGame::drawRect(float x, float y, float w, float h,
                               const Color& color) {
  uiRectSprite.position.set(x, y);
  uiRectSprite.size.set(w, h);
  uiRectSprite.scale = 1.0F;
  uiRectSprite.color = color;
  engine->renderer.renderer2D.render(uiRectSprite);
}

void DroidCamposGame::drawPanel(float x, float y, float w, float h,
                                const Color& bgColor,
                                const Color& borderColor) {
  drawRect(x, y, w, h, bgColor);
  drawRect(x, y, w, 2.0F, borderColor);
  drawRect(x, y + h - 2.0F, w, 2.0F, borderColor);
  drawRect(x, y, 2.0F, h, borderColor);
  drawRect(x + w - 2.0F, y, 2.0F, h, borderColor);
  drawRect(x, y, 8.0F, 4.0F, borderColor);
  drawRect(x + w - 8.0F, y, 8.0F, 4.0F, borderColor);
  drawRect(x, y + h - 4.0F, 8.0F, 4.0F, borderColor);
  drawRect(x + w - 8.0F, y + h - 4.0F, 8.0F, 4.0F, borderColor);
}

void DroidCamposGame::renderBackground() {
  engine->renderer.renderer2D.render(bgSprite);

  for (int i = 0; i < 6; i++) {
    float lineY = 70.0F + i * 60.0F + (bgScrollOffset * 0.5F);
    if (lineY < 410.0F) {
      drawRect(0.0F, lineY, 512.0F, 1.0F, Color(0.0F, 80.0F, 110.0F, 28.0F));
    }
  }

  if (scanlinesEnabled) {
    drawRect(0.0F, scanlineY, 512.0F, 3.0F, Color(0.0F, 200.0F, 255.0F, 24.0F));
    drawRect(0.0F, scanlineY + 3.0F, 512.0F, 1.0F,
             Color(255.0F, 180.0F, 0.0F, 18.0F));
  }
}

void DroidCamposGame::renderHeader() {
  drawPanel(16.0F, 14.0F, 480.0F, 64.0F, Color(10.0F, 16.0F, 26.0F, 112.0F),
            Color(0.0F, 190.0F, 230.0F, 120.0F));

  drawRect(20.0F, 18.0F, 6.0F, 56.0F, Color(230.0F, 140.0F, 10.0F, 128.0F));

  font.drawText("DROID CAMPOS", 36, 22, Color(200.0F, 40.0F, 20.0F, 100.0F),
                2.0F);
  font.drawText("DROID CAMPOS", 34, 20, Color(0.0F, 230.0F, 255.0F, 128.0F),
                2.0F);

  font.drawText("AXIOM-7 HEAVY IND. // PROJECT S.I.G.M.A.", 35, 56,
                Color(230.0F, 165.0F, 30.0F, 128.0F), 1.0F);

  font.drawText("UNIT: DC-01", 380, 22, Color(180.0F, 220.0F, 230.0F, 128.0F),
                1.0F);
  font.drawText("SYNC: 98.4%", 380, 38, Color(60.0F, 220.0F, 120.0F, 128.0F),
                1.0F);
  font.drawText("SECTOR: ETIP-01", 356, 54,
                Color(160.0F, 170.0F, 185.0F, 128.0F), 1.0F);
}

void DroidCamposGame::renderFooter(const char* controlsHint) {
  drawPanel(16.0F, 406.0F, 480.0F, 28.0F, Color(8.0F, 12.0F, 20.0F, 118.0F),
            Color(90.0F, 110.0F, 130.0F, 100.0F));
  font.drawText(controlsHint, 28, 413, Color(190.0F, 215.0F, 230.0F, 128.0F),
                1.0F);
}

void DroidCamposGame::renderCyborgPortrait(float x, float y) {
  drawPanel(x, y, 228.0F, 304.0F, Color(12.0F, 18.0F, 28.0F, 112.0F),
            Color(0.0F, 160.0F, 200.0F, 115.0F));

  font.drawText("BIOMETRIC FEED // DC-01", static_cast<int>(x + 14),
                static_cast<int>(y + 10), Color(0.0F, 210.0F, 240.0F, 128.0F));
  drawRect(x + 10.0F, y + 28.0F, 208.0F, 1.0F,
           Color(0.0F, 160.0F, 200.0F, 90.0F));

  float cx = x + 114.0F;
  float cy = y + 98.0F;
  float pulse = (std::sin(animTick * 2.0F) + 1.0F) * 0.5F;

  drawRect(cx - 64.0F, cy + 28.0F, 128.0F, 46.0F,
           Color(35.0F, 45.0F, 58.0F, 128.0F));
  drawRect(cx - 72.0F, cy + 34.0F, 22.0F, 36.0F,
           Color(90.0F, 65.0F, 20.0F, 128.0F));
  drawRect(cx + 50.0F, cy + 34.0F, 22.0F, 36.0F,
           Color(90.0F, 65.0F, 20.0F, 128.0F));

  drawRect(cx - 18.0F, cy + 40.0F, 36.0F, 24.0F,
           Color(15.0F, 25.0F, 35.0F, 128.0F));
  drawRect(cx - 12.0F, cy + 44.0F, 24.0F, 16.0F,
           Color(0.0F, 160.0F + pulse * 80.0F, 230.0F, 128.0F));

  drawRect(cx - 16.0F, cy + 14.0F, 32.0F, 16.0F,
           Color(25.0F, 32.0F, 42.0F, 128.0F));
  drawRect(cx - 24.0F, cy + 8.0F, 4.0F, 22.0F,
           Color(180.0F, 50.0F, 30.0F, 128.0F));
  drawRect(cx + 20.0F, cy + 8.0F, 4.0F, 22.0F,
           Color(0.0F, 180.0F, 220.0F, 128.0F));

  drawRect(cx - 32.0F, cy - 38.0F, 64.0F, 54.0F,
           Color(50.0F, 62.0F, 78.0F, 128.0F));
  drawRect(cx - 26.0F, cy - 44.0F, 52.0F, 8.0F,
           Color(70.0F, 85.0F, 100.0F, 128.0F));

  drawRect(cx - 28.0F, cy - 20.0F, 56.0F, 14.0F,
           Color(18.0F, 10.0F, 15.0F, 128.0F));
  drawRect(cx - 24.0F, cy - 17.0F, 48.0F, 8.0F,
           Color(220.0F, 40.0F + pulse * 60.0F, 20.0F, 128.0F));
  float eyeScanX = cx - 20.0F + std::sin(animTick * 3.0F) * 14.0F;
  drawRect(eyeScanX, cy - 17.0F, 10.0F, 8.0F,
           Color(255.0F, 230.0F, 120.0F, 128.0F));

  drawRect(cx - 20.0F, cy - 2.0F, 40.0F, 14.0F,
           Color(30.0F, 38.0F, 48.0F, 128.0F));
  drawRect(cx - 14.0F, cy + 2.0F, 6.0F, 8.0F,
           Color(120.0F, 140.0F, 160.0F, 128.0F));
  drawRect(cx - 3.0F, cy + 2.0F, 6.0F, 8.0F,
           Color(120.0F, 140.0F, 160.0F, 128.0F));
  drawRect(cx + 8.0F, cy + 2.0F, 6.0F, 8.0F,
           Color(120.0F, 140.0F, 160.0F, 128.0F));

  drawRect(x + 10.0F, y + 180.0F, 208.0F, 1.0F,
           Color(0.0F, 160.0F, 200.0F, 90.0F));

  font.drawText("SUBJECT: DR. V. CAMPOS", static_cast<int>(x + 14),
                static_cast<int>(y + 188),
                Color(240.0F, 180.0F, 40.0F, 128.0F));
  font.drawText("FORMER CHIEF MATHEMATICIAN", static_cast<int>(x + 14),
                static_cast<int>(y + 206),
                Color(180.0F, 200.0F, 215.0F, 128.0F));
  font.drawText("BETRAYED BY AXIOM BOARD.", static_cast<int>(x + 14),
                static_cast<int>(y + 224),
                Color(180.0F, 200.0F, 215.0F, 128.0F));
  font.drawText("REBUILT IN CHASSIS DC-01", static_cast<int>(x + 14),
                static_cast<int>(y + 242),
                Color(180.0F, 200.0F, 215.0F, 128.0F));
  font.drawText("TARGET: ETIP COMPLEX.", static_cast<int>(x + 14),
                static_cast<int>(y + 260),
                Color(0.0F, 220.0F, 250.0F, 128.0F));
  font.drawText("STATUS: VENGEANCE_ACTIVE", static_cast<int>(x + 14),
                static_cast<int>(y + 280),
                Color(240.0F, 70.0F, 50.0F, 128.0F));
}

void DroidCamposGame::renderMainMenu() {
  drawPanel(16.0F, 90.0F, 236.0F, 304.0F, Color(12.0F, 18.0F, 28.0F, 112.0F),
            Color(0.0F, 180.0F, 220.0F, 115.0F));

  font.drawText("COMMAND TERMINAL", 32, 102,
                Color(230.0F, 165.0F, 30.0F, 128.0F));
  drawRect(26.0F, 122.0F, 216.0F, 1.0F, Color(0.0F, 160.0F, 200.0F, 90.0F));

  const char* labels[3] = {"NEW GAME", "LOAD GAME", "SETTINGS"};
  const char* subLabels[3] = {"DEPLOY AT ETIP GATE (FPS)",
                              "READ MEMORY CARD SLOTS",
                              "AUDIO & OPTICAL CONFIG"};

  for (int i = 0; i < 3; i++) {
    float itemY = 138.0F + i * 66.0F;
    bool selected = (mainSelectedIndex == i);

    if (selected) {
      float pulse = (std::sin(animTick * 3.0F) + 1.0F) * 0.5F;
      drawPanel(28.0F, itemY, 212.0F, 52.0F,
                Color(0.0F, 65.0F + pulse * 25.0F, 95.0F, 118.0F),
                Color(0.0F, 230.0F, 255.0F, 128.0F));
      drawRect(32.0F, itemY + 6.0F, 5.0F, 40.0F,
               Color(255.0F, 190.0F, 20.0F, 128.0F));

      font.drawText(">", 44, static_cast<int>(itemY + 8),
                    Color(255.0F, 200.0F, 40.0F, 128.0F), 1.3F);
      font.drawText(labels[i], 62, static_cast<int>(itemY + 8),
                    Color(255.0F, 255.0F, 255.0F, 128.0F), 1.3F);
      font.drawText(subLabels[i], 46, static_cast<int>(itemY + 32),
                    Color(140.0F, 235.0F, 255.0F, 128.0F), 1.0F);
    } else {
      drawPanel(28.0F, itemY, 212.0F, 52.0F,
                Color(16.0F, 24.0F, 36.0F, 95.0F),
                Color(55.0F, 75.0F, 95.0F, 95.0F));
      font.drawText(labels[i], 48, static_cast<int>(itemY + 8),
                    Color(160.0F, 180.0F, 195.0F, 128.0F), 1.2F);
      font.drawText(subLabels[i], 46, static_cast<int>(itemY + 32),
                    Color(100.0F, 120.0F, 135.0F, 128.0F), 1.0F);
    }
  }

  font.drawText("3D ENGINE: ETIP + PISTOL", 30, 346,
                Color(90.0F, 190.0F, 140.0F, 128.0F));
  font.drawText("BUILD: PS2-TYRA-v0.0.1", 30, 366,
                Color(120.0F, 140.0F, 160.0F, 128.0F));

  renderCyborgPortrait(268.0F, 90.0F);
}

void DroidCamposGame::renderNewGameScreen() {
  drawPanel(16.0F, 90.0F, 480.0F, 304.0F, Color(10.0F, 16.0F, 26.0F, 118.0F),
            Color(240.0F, 160.0F, 20.0F, 125.0F));

  font.drawText("CLASSIFIED DOSSIER // INFILTRATING ETIP COMPLEX", 32, 104,
                Color(255.0F, 190.0F, 30.0F, 128.0F));
  drawRect(28.0F, 124.0F, 456.0F, 2.0F, Color(240.0F, 160.0F, 20.0F, 100.0F));

  font.drawText(
      "YEAR 2147 - NEO-CAMPOS INDUSTRIAL WASTELAND.\n"
      "DR. VICTOR CAMPOS, CHIEF MATHEMATICIAN AT AXIOM-7,\n"
      "ARRIVES AT THE ABANDONED PERIMETER OF THE ETIP\n"
      "INDUSTRIAL INSTALLATION.\n\n"
      "OBJECTIVE:\n"
      "- ADVANCE THROUGH ACCESS ROAD (ZONA A -> ZONA B)\n"
      "- INSPECT SECURITY GUARITA & CARD READER AT GATE\n"
      "- BREACH MAIN INDUSTRIAL GATE INTO ETIP FACILITY.",
      32, 136, Color(200.0F, 225.0F, 235.0F, 128.0F));

  float pulse = (std::sin(animTick * 4.0F) + 1.0F) * 0.5F;
  drawPanel(86.0F, 344.0F, 340.0F, 36.0F,
            Color(0.0F, 70.0F + pulse * 40.0F, 100.0F, 120.0F),
            Color(0.0F, 235.0F, 255.0F, 128.0F));
  font.drawText(">> PRESS CROSS TO START 3D FPS <<", 112, 354,
                Color(255.0F, 240.0F, 150.0F, 128.0F));
}

void DroidCamposGame::renderLoadGameScreen() {
  drawPanel(16.0F, 90.0F, 480.0F, 304.0F, Color(10.0F, 16.0F, 26.0F, 118.0F),
            Color(0.0F, 200.0F, 240.0F, 125.0F));

  font.drawText("LOAD GAME // MEMORY CARD [mc0:/DROIDCAMPOS]", 32, 104,
                Color(0.0F, 225.0F, 255.0F, 128.0F));
  drawRect(28.0F, 124.0F, 456.0F, 2.0F, Color(0.0F, 180.0F, 220.0F, 100.0F));

  const char* slotTitles[3] = {
      "SLOT 01 // ETIP PERIMETER GATE (ZONA A)",
      "SLOT 02 // AXIOM-7 QUANTUM RESEARCH LAB",
      "SLOT 03 // [ EMPTY / UNFORMATTED DATA BLOCK ]"};
  const char* slotDetails[3] = {
      "DR. CAMPOS LVL 01   NEURAL SYNC: 98%   TIME: 00:05:12",
      "DR. CAMPOS LVL 02   NEURAL SYNC: 78%   TIME: 00:54:09",
      "NO TELEMETRY CHECKSUM FOUND IN THIS MEMORY SECTOR"};

  for (int i = 0; i < 3; i++) {
    float slotY = 140.0F + i * 74.0F;
    bool selected = (loadSelectedIndex == i);

    if (selected) {
      drawPanel(32.0F, slotY, 448.0F, 60.0F,
                Color(0.0F, 60.0F, 90.0F, 118.0F),
                Color(255.0F, 195.0F, 30.0F, 128.0F));
      drawRect(36.0F, slotY + 6.0F, 6.0F, 48.0F,
               Color(255.0F, 195.0F, 30.0F, 128.0F));
      font.drawText(slotTitles[i], 52, static_cast<int>(slotY + 12),
                    Color(255.0F, 255.0F, 255.0F, 128.0F), 1.1F);
      font.drawText(slotDetails[i], 52, static_cast<int>(slotY + 36),
                    Color(140.0F, 235.0F, 255.0F, 128.0F), 1.0F);
    } else {
      drawPanel(32.0F, slotY, 448.0F, 60.0F,
                Color(16.0F, 22.0F, 34.0F, 95.0F),
                Color(60.0F, 80.0F, 100.0F, 95.0F));
      font.drawText(slotTitles[i], 52, static_cast<int>(slotY + 12),
                    Color(160.0F, 175.0F, 190.0F, 128.0F), 1.0F);
      font.drawText(slotDetails[i], 52, static_cast<int>(slotY + 36),
                    Color(100.0F, 115.0F, 130.0F, 128.0F), 1.0F);
    }
  }

  if (loadSelectedIndex == 2) {
    font.drawText("STATUS: CANNOT LOAD EMPTY DATA BLOCK", 32, 366,
                  Color(240.0F, 80.0F, 60.0F, 128.0F));
  } else {
    font.drawText("STATUS: CHECKSUM VALID - PRESS CROSS TO LOAD", 32, 366,
                  Color(80.0F, 230.0F, 130.0F, 128.0F));
  }
}

void DroidCamposGame::renderSettingsScreen() {
  drawPanel(16.0F, 90.0F, 480.0F, 304.0F, Color(10.0F, 16.0F, 26.0F, 118.0F),
            Color(0.0F, 200.0F, 240.0F, 125.0F));

  font.drawText("SYSTEM SETTINGS // CYBERNETIC & AUDIO CALIBRATION", 32, 104,
                Color(230.0F, 165.0F, 30.0F, 128.0F));
  drawRect(28.0F, 124.0F, 456.0F, 2.0F, Color(0.0F, 180.0F, 220.0F, 100.0F));

  const char* optionNames[4] = {"BGM SYNTH VOLUME", "INDUSTRIAL SFX VOL",
                                "OPTICAL SCANLINES", "GS TEXTURE FILTER"};

  char valBuf[4][32];
  std::snprintf(valBuf[0], sizeof(valBuf[0]), "%d%%", bgmVolume);
  std::snprintf(valBuf[1], sizeof(valBuf[1]), "%d%%", sfxVolume);
  std::snprintf(valBuf[2], sizeof(valBuf[2]), "%s",
                scanlinesEnabled ? "[ ENABLED ]" : "[ DISABLED ]");
  std::snprintf(valBuf[3], sizeof(valBuf[3]), "%s",
                nearestFilter ? "[ NEAREST / PIXEL ]" : "[ LINEAR / SMOOTH ]");

  for (int i = 0; i < 4; i++) {
    float rowY = 142.0F + i * 56.0F;
    bool selected = (settingsSelectedIndex == i);

    if (selected) {
      drawPanel(32.0F, rowY, 448.0F, 46.0F,
                Color(0.0F, 60.0F, 90.0F, 115.0F),
                Color(0.0F, 230.0F, 255.0F, 128.0F));
      drawRect(36.0F, rowY + 6.0F, 5.0F, 34.0F,
               Color(255.0F, 195.0F, 30.0F, 128.0F));
    } else {
      drawPanel(32.0F, rowY, 448.0F, 46.0F,
                Color(16.0F, 22.0F, 34.0F, 95.0F),
                Color(55.0F, 75.0F, 95.0F, 95.0F));
    }

    Color labelCol = selected ? Color(255.0F, 255.0F, 255.0F, 128.0F)
                              : Color(170.0F, 185.0F, 200.0F, 128.0F);
    font.drawText(optionNames[i], 50, static_cast<int>(rowY + 14), labelCol);

    if (i == 0 || i == 1) {
      int vol = (i == 0) ? bgmVolume : sfxVolume;
      float barX = 245.0F;
      float barY = rowY + 15.0F;
      drawRect(barX, barY, 140.0F, 14.0F, Color(8.0F, 12.0F, 18.0F, 128.0F));
      float fillW = (136.0F * static_cast<float>(vol)) / 100.0F;
      if (fillW > 0.0F) {
        drawRect(barX + 2.0F, barY + 2.0F, fillW, 10.0F,
                 Color(0.0F, 215.0F, 250.0F, 128.0F));
      }
      font.drawText(valBuf[i], 398, static_cast<int>(rowY + 14),
                    Color(255.0F, 200.0F, 40.0F, 128.0F));
    } else {
      font.drawText(valBuf[i], 270, static_cast<int>(rowY + 14),
                    Color(120.0F, 240.0F, 170.0F, 128.0F));
    }
  }
}

}  // namespace DroidCampos
