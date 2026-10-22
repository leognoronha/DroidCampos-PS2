#pragma once

#include <tyra>
#include <memory>
#include "font_sprite.hpp"

namespace DroidCampos {

enum class MenuScreen {
  MAIN_MENU = 0,
  NEW_GAME_SCREEN = 1,
  LOAD_GAME_SCREEN = 2,
  SETTINGS_SCREEN = 3,
  GAMEPLAY_FPS = 4
};

class DroidCamposGame : public Tyra::Game {
 public:
  explicit DroidCamposGame(Tyra::Engine* engine);
  ~DroidCamposGame();

  void init() override;
  void loop() override;

 private:
  void loadResources();
  void loadScenario3D();
  void resetPlayerToSpawn();

  void handleInput();
  void updateFpsMovement();
  void resolveWorldCollisions(float& x, float& z) const;
  void updateCameraVectors();
  void updateAnimations();

  void renderBackground();
  void renderHeader();
  void renderFooter(const char* controlsHint);
  void renderCyborgPortrait(float x, float y);

  void renderMainMenu();
  void renderNewGameScreen();
  void renderLoadGameScreen();
  void renderSettingsScreen();
  void renderGameplayFps();
  void renderFpsHud();

  void drawRect(float x, float y, float w, float h, const Tyra::Color& color);
  void drawPanel(float x, float y, float w, float h,
                 const Tyra::Color& bgColor, const Tyra::Color& borderColor);

  Tyra::Engine* engine;
  Font font;

  Tyra::Sprite bgSprite;
  Tyra::Sprite uiRectSprite;

  // 3D Scenario & Pipeline (ETIP Industrial Level)
  Tyra::StaticPipeline stapip;
  std::unique_ptr<Tyra::StaticMesh> scenarioMesh;
  Tyra::StaPipOptions scenarioOptions;
  bool scenarioLoaded;

  // First-Person (FPS) Player & Camera State
  Tyra::Vec4 playerPos;
  Tyra::Vec4 cameraPos;
  Tyra::Vec4 cameraLookAt;
  Tyra::Vec4 cameraUp;
  float cameraYaw;    // radians (horizontal look: left/right)
  float cameraPitch;  // radians (vertical look: up/down)
  float moveSpeed;
  float lookSensitivity;

  MenuScreen currentScreen;
  int mainSelectedIndex;
  int loadSelectedIndex;
  int settingsSelectedIndex;

  // Settings values
  int bgmVolume;
  int sfxVolume;
  bool scanlinesEnabled;
  bool nearestFilter;

  // Animation & input timers
  int padCooldown;
  float animTick;
  float scanlineY;
  float bgScrollOffset;
  int activeSaveSlot;
};

}  // namespace DroidCampos
