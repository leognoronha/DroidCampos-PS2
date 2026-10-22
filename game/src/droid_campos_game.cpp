#include "droid_campos_game.hpp"
#include <cmath>
#include <cstdio>

namespace DroidCampos {

using namespace Tyra;

DroidCamposGame::DroidCamposGame(Engine* t_engine)
    : engine(t_engine),
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
  repo.freeBySprite(bgSprite);
  repo.freeBySprite(uiRectSprite);
  font.free(repo);
}

void DroidCamposGame::init() {
  // Cor de fundo escura industrial/cyberpunk
  engine->renderer.setClearScreenColor(Color(8.0F, 12.0F, 20.0F));
  engine->renderer.core.renderer2D.setTextureMappingType(TyraNearest);

  loadResources();
  TYRA_LOG("DroidCampos Menu Template initialized!");
}

void DroidCamposGame::loadResources() {
  auto& repo = engine->renderer.getTextureRepository();

  font.load(repo, &engine->renderer.renderer2D);

  // Textura de fundo cyberpunk industrial (256x256 esticada para 512x448)
  bgSprite.mode = SpriteMode::MODE_STRETCH;
  bgSprite.size = Vec2(512.0F, 448.0F);
  bgSprite.position = Vec2(0.0F, 0.0F);
  bgSprite.color = Color(128.0F, 128.0F, 128.0F, 128.0F);
  auto* bgTex = repo.add(FileUtils::fromCwd("bg_cyber.png"));
  bgTex->addLink(bgSprite.id);

  // Textura 16x16 branca para desenhar painéis, bordas, barras e HUD
  uiRectSprite.mode = SpriteMode::MODE_STRETCH;
  uiRectSprite.size = Vec2(16.0F, 16.0F);
  uiRectSprite.position = Vec2(0.0F, 0.0F);
  auto* whiteTex = repo.add(FileUtils::fromCwd("ui_white.png"));
  whiteTex->addLink(uiRectSprite.id);
}

void DroidCamposGame::loop() {
  handleInput();
  updateAnimations();

  auto& renderer = engine->renderer;
  renderer.core.renderer2D.setTextureMappingType(nearestFilter ? TyraNearest
                                                               : TyraLinear);

  renderer.beginFrame();

  renderBackground();
  renderHeader();

  switch (currentScreen) {
    case MenuScreen::MAIN_MENU:
      renderMainMenu();
      renderFooter(
          "DPAD: NAVIGATE   CROSS/START: SELECT   AXIOM-OS V4.09");
      break;
    case MenuScreen::NEW_GAME_SCREEN:
      renderNewGameScreen();
      renderFooter("CROSS: DEPLOY DROID_CAMPOS   CIRCLE: RETURN TO MENU");
      break;
    case MenuScreen::LOAD_GAME_SCREEN:
      renderLoadGameScreen();
      renderFooter("DPAD: SELECT SLOT   CROSS: LOAD DATA   CIRCLE: BACK");
      break;
    case MenuScreen::SETTINGS_SCREEN:
      renderSettingsScreen();
      renderFooter("DPAD UP/DOWN: OPTION   LEFT/RIGHT: ADJUST   CIRCLE: BACK");
      break;
    case MenuScreen::GAMEPLAY_TEMPLATE:
      renderGameplayTemplate();
      renderFooter("CIRCLE / SELECT: ABORT SIMULATION & RETURN TO MAIN MENU");
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
      currentScreen = MenuScreen::GAMEPLAY_TEMPLATE;
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
      // Slots 0 e 1 possuem dados salvos simulados
      if (loadSelectedIndex < 2) {
        activeSaveSlot = loadSelectedIndex + 1;
        currentScreen = MenuScreen::GAMEPLAY_TEMPLATE;
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
  } else if (currentScreen == MenuScreen::GAMEPLAY_TEMPLATE) {
    if (clicked.Circle || clicked.Select || clicked.Start) {
      currentScreen = MenuScreen::MAIN_MENU;
    }
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
  // Fundo do painel
  drawRect(x, y, w, h, bgColor);
  // Bordas industriais (topo, base, esquerda, direita)
  drawRect(x, y, w, 2.0F, borderColor);
  drawRect(x, y + h - 2.0F, w, 2.0F, borderColor);
  drawRect(x, y, 2.0F, h, borderColor);
  drawRect(x + w - 2.0F, y, 2.0F, h, borderColor);
  // Cantoneiras de reforço estilo HUD militar/industrial
  drawRect(x, y, 8.0F, 4.0F, borderColor);
  drawRect(x + w - 8.0F, y, 8.0F, 4.0F, borderColor);
  drawRect(x, y + h - 4.0F, 8.0F, 4.0F, borderColor);
  drawRect(x + w - 8.0F, y + h - 4.0F, 8.0F, 4.0F, borderColor);
}

void DroidCamposGame::renderBackground() {
  engine->renderer.renderer2D.render(bgSprite);

  // Linhas de grade cibernética animadas sutis
  for (int i = 0; i < 6; i++) {
    float lineY = 70.0F + i * 60.0F + (bgScrollOffset * 0.5F);
    if (lineY < 410.0F) {
      drawRect(0.0F, lineY, 512.0F, 1.0F, Color(0.0F, 80.0F, 110.0F, 28.0F));
    }
  }

  // Scanline de CRT / Visor Óptico em movimento
  if (scanlinesEnabled) {
    drawRect(0.0F, scanlineY, 512.0F, 3.0F, Color(0.0F, 200.0F, 255.0F, 24.0F));
    drawRect(0.0F, scanlineY + 3.0F, 512.0F, 1.0F,
             Color(255.0F, 180.0F, 0.0F, 18.0F));
  }
}

void DroidCamposGame::renderHeader() {
  // Painel superior do cabeçalho
  drawPanel(16.0F, 14.0F, 480.0F, 64.0F, Color(10.0F, 16.0F, 26.0F, 112.0F),
            Color(0.0F, 190.0F, 230.0F, 120.0F));

  // Barra lateral de alerta industrial (amarelo/laranja pós-apocalíptico)
  drawRect(20.0F, 18.0F, 6.0F, 56.0F, Color(230.0F, 140.0F, 10.0F, 128.0F));

  // Sombra e Título principal "DROID CAMPOS"
  font.drawText("DROID CAMPOS", 36, 22, Color(200.0F, 40.0F, 20.0F, 100.0F),
                2.0F);
  font.drawText("DROID CAMPOS", 34, 20, Color(0.0F, 230.0F, 255.0F, 128.0F),
                2.0F);

  // Subtítulo corporativo e equação
  font.drawText("AXIOM-7 HEAVY IND. // PROJECT S.I.G.M.A.", 35, 56,
                Color(230.0F, 165.0F, 30.0F, 128.0F), 1.0F);

  // Telemetria à direita do cabeçalho
  font.drawText("UNIT: DC-01", 380, 22, Color(180.0F, 220.0F, 230.0F, 128.0F),
                1.0F);
  font.drawText("SYNC: 98.4%", 380, 38, Color(60.0F, 220.0F, 120.0F, 128.0F),
                1.0F);
  font.drawText("SECTOR: FORJA-09", 356, 54,
                Color(160.0F, 170.0F, 185.0F, 128.0F), 1.0F);
}

void DroidCamposGame::renderFooter(const char* controlsHint) {
  drawPanel(16.0F, 406.0F, 480.0F, 28.0F, Color(8.0F, 12.0F, 20.0F, 118.0F),
            Color(90.0F, 110.0F, 130.0F, 100.0F));
  font.drawText(controlsHint, 28, 413, Color(190.0F, 215.0F, 230.0F, 128.0F),
                1.0F);
}

void DroidCamposGame::renderCyborgPortrait(float x, float y) {
  // Moldura do terminal biométrico de DroidCampos (Dr. Victor Campos)
  drawPanel(x, y, 228.0F, 304.0F, Color(12.0F, 18.0F, 28.0F, 112.0F),
            Color(0.0F, 160.0F, 200.0F, 115.0F));

  font.drawText("BIOMETRIC FEED // DC-01", static_cast<int>(x + 14),
                static_cast<int>(y + 10), Color(0.0F, 210.0F, 240.0F, 128.0F));
  drawRect(x + 10.0F, y + 28.0F, 208.0F, 1.0F,
           Color(0.0F, 160.0F, 200.0F, 90.0F));

  // Ilustração geométrica / esquemática do Ciborgue DroidCampos
  float cx = x + 114.0F;
  float cy = y + 98.0F;
  float pulse = (std::sin(animTick * 2.0F) + 1.0F) * 0.5F;

  // Ombros e armadura industrial pesada
  drawRect(cx - 64.0F, cy + 28.0F, 128.0F, 46.0F,
           Color(35.0F, 45.0F, 58.0F, 128.0F));
  drawRect(cx - 72.0F, cy + 34.0F, 22.0F, 36.0F,
           Color(90.0F, 65.0F, 20.0F, 128.0F));
  drawRect(cx + 50.0F, cy + 34.0F, 22.0F, 36.0F,
           Color(90.0F, 65.0F, 20.0F, 128.0F));

  // Reator torácico (Equação Campos / Núcleo de Plasma)
  drawRect(cx - 18.0F, cy + 40.0F, 36.0F, 24.0F,
           Color(15.0F, 25.0F, 35.0F, 128.0F));
  drawRect(cx - 12.0F, cy + 44.0F, 24.0F, 16.0F,
           Color(0.0F, 160.0F + pulse * 80.0F, 230.0F, 128.0F));

  // Pescoço mecânico e cabos neurais
  drawRect(cx - 16.0F, cy + 14.0F, 32.0F, 16.0F,
           Color(25.0F, 32.0F, 42.0F, 128.0F));
  drawRect(cx - 24.0F, cy + 8.0F, 4.0F, 22.0F,
           Color(180.0F, 50.0F, 30.0F, 128.0F));
  drawRect(cx + 20.0F, cy + 8.0F, 4.0F, 22.0F,
           Color(0.0F, 180.0F, 220.0F, 128.0F));

  // Capacete / Crânio Cibernético
  drawRect(cx - 32.0F, cy - 38.0F, 64.0F, 54.0F,
           Color(50.0F, 62.0F, 78.0F, 128.0F));
  drawRect(cx - 26.0F, cy - 44.0F, 52.0F, 8.0F,
           Color(70.0F, 85.0F, 100.0F, 128.0F));

  // Visor Óptico Cyberpunk (vermelho/laranja brilhante com varredura)
  drawRect(cx - 28.0F, cy - 20.0F, 56.0F, 14.0F,
           Color(18.0F, 10.0F, 15.0F, 128.0F));
  drawRect(cx - 24.0F, cy - 17.0F, 48.0F, 8.0F,
           Color(220.0F, 40.0F + pulse * 60.0F, 20.0F, 128.0F));
  float eyeScanX = cx - 20.0F + std::sin(animTick * 3.0F) * 14.0F;
  drawRect(eyeScanX, cy - 17.0F, 10.0F, 8.0F,
           Color(255.0F, 230.0F, 120.0F, 128.0F));

  // Máscara respiradora industrial
  drawRect(cx - 20.0F, cy - 2.0F, 40.0F, 14.0F,
           Color(30.0F, 38.0F, 48.0F, 128.0F));
  drawRect(cx - 14.0F, cy + 2.0F, 6.0F, 8.0F,
           Color(120.0F, 140.0F, 160.0F, 128.0F));
  drawRect(cx - 3.0F, cy + 2.0F, 6.0F, 8.0F,
           Color(120.0F, 140.0F, 160.0F, 128.0F));
  drawRect(cx + 8.0F, cy + 2.0F, 6.0F, 8.0F,
           Color(120.0F, 140.0F, 160.0F, 128.0F));

  // Dossiê resumido abaixo do retrato
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
  font.drawText("TO PURGE S.I.G.M.A. CORE.", static_cast<int>(x + 14),
                static_cast<int>(y + 260),
                Color(0.0F, 220.0F, 250.0F, 128.0F));
  font.drawText("STATUS: VENGEANCE_ACTIVE", static_cast<int>(x + 14),
                static_cast<int>(y + 280),
                Color(240.0F, 70.0F, 50.0F, 128.0F));
}

void DroidCamposGame::renderMainMenu() {
  // Painel esquerdo com as 3 opções: NEW GAME, LOAD GAME, SETTINGS
  drawPanel(16.0F, 90.0F, 236.0F, 304.0F, Color(12.0F, 18.0F, 28.0F, 112.0F),
            Color(0.0F, 180.0F, 220.0F, 115.0F));

  font.drawText("COMMAND TERMINAL", 32, 102,
                Color(230.0F, 165.0F, 30.0F, 128.0F));
  drawRect(26.0F, 122.0F, 216.0F, 1.0F, Color(0.0F, 160.0F, 200.0F, 90.0F));

  const char* labels[3] = {"NEW GAME", "LOAD GAME", "SETTINGS"};
  const char* subLabels[3] = {"INITIATE PROTOCOL DC-01",
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
      // Indicador lateral amarelo/ciano
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

  // Rodapé interno do menu esquerdo
  font.drawText("MATH ENGINE: EQUATION READY", 30, 346,
                Color(90.0F, 190.0F, 140.0F, 128.0F));
  font.drawText("BUILD: PS2-TYRA-2147", 30, 366,
                Color(120.0F, 140.0F, 160.0F, 128.0F));

  // Painel direito: Retrato e Dossiê do Ciborgue DroidCampos
  renderCyborgPortrait(268.0F, 90.0F);
}

void DroidCamposGame::renderNewGameScreen() {
  drawPanel(16.0F, 90.0F, 480.0F, 304.0F, Color(10.0F, 16.0F, 26.0F, 118.0F),
            Color(240.0F, 160.0F, 20.0F, 125.0F));

  font.drawText("CLASSIFIED DOSSIER // THE FALL OF DR. VICTOR CAMPOS", 32, 104,
                Color(255.0F, 190.0F, 30.0F, 128.0F));
  drawRect(28.0F, 124.0F, 456.0F, 2.0F, Color(240.0F, 160.0F, 20.0F, 100.0F));

  font.drawText(
      "YEAR 2147 - NEO-CAMPOS INDUSTRIAL WASTELAND.\n"
      "DR. VICTOR CAMPOS, CHIEF MATHEMATICIAN AT AXIOM-7,\n"
      "LED THE CREATION OF S.I.G.M.A., A QUANTUM SUPER-AI.\n\n"
      "WHEN CAMPOS DISCOVERED THE EXECUTIVE BOARD SECRETLY\n"
      "REPROGRAMMED S.I.G.M.A. FOR THE 'OMEGA-PURGE' GENOCIDE,\n"
      "HE TRIED TO INJECT A MATHEMATICAL PARADOX INTO ITS CORE.\n\n"
      "BETRAYED AND CAST INTO THE SUB-LEVEL PLASMA FURNACES BY\n"
      "THE EXECUTIVES, HIS SURVIVING CORTEX WAS FUSED INTO\n"
      "HEAVY INDUSTRIAL CHASSIS DC-01: DROID CAMPOS.",
      32, 136, Color(200.0F, 225.0F, 235.0F, 128.0F));

  float pulse = (std::sin(animTick * 4.0F) + 1.0F) * 0.5F;
  drawPanel(86.0F, 344.0F, 340.0F, 36.0F,
            Color(0.0F, 70.0F + pulse * 40.0F, 100.0F, 120.0F),
            Color(0.0F, 235.0F, 255.0F, 128.0F));
  font.drawText(">> PRESS CROSS TO INITIALIZE SECTOR 09 <<", 104, 354,
                Color(255.0F, 240.0F, 150.0F, 128.0F));
}

void DroidCamposGame::renderLoadGameScreen() {
  drawPanel(16.0F, 90.0F, 480.0F, 304.0F, Color(10.0F, 16.0F, 26.0F, 118.0F),
            Color(0.0F, 200.0F, 240.0F, 125.0F));

  font.drawText("LOAD GAME // MEMORY CARD [mc0:/DROIDCAMPOS]", 32, 104,
                Color(0.0F, 225.0F, 255.0F, 128.0F));
  drawRect(28.0F, 124.0F, 456.0F, 2.0F, Color(0.0F, 180.0F, 220.0F, 100.0F));

  const char* slotTitles[3] = {
      "SLOT 01 // SECTOR 09: PLASMA FOUNDRY",
      "SLOT 02 // AXIOM-7 QUANTUM RESEARCH LAB",
      "SLOT 03 // [ EMPTY / UNFORMATTED DATA BLOCK ]"};
  const char* slotDetails[3] = {
      "DR. CAMPOS LVL 04   NEURAL SYNC: 94%   TIME: 02:41:18",
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

    // Barras de volume para os itens 0 e 1
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

void DroidCamposGame::renderGameplayTemplate() {
  drawPanel(16.0F, 90.0F, 480.0F, 304.0F, Color(10.0F, 18.0F, 24.0F, 118.0F),
            Color(80.0F, 230.0F, 140.0F, 125.0F));

  if (activeSaveSlot == 0) {
    font.drawText("MISSION ACTIVE // NEW GAME: SECTOR 09 - FURNACE CORE", 32,
                  106, Color(80.0F, 240.0F, 140.0F, 128.0F));
  } else if (activeSaveSlot == 1) {
    font.drawText("MISSION ACTIVE // LOADED SLOT 01: PLASMA FOUNDRY", 32, 106,
                  Color(80.0F, 240.0F, 140.0F, 128.0F));
  } else {
    font.drawText("MISSION ACTIVE // LOADED SLOT 02: QUANTUM LABS", 32, 106,
                  Color(80.0F, 240.0F, 140.0F, 128.0F));
  }

  drawRect(28.0F, 126.0F, 456.0F, 2.0F, Color(80.0F, 230.0F, 140.0F, 100.0F));

  font.drawText(
      "COMBAT CHASSIS DC-01 ONLINE.\n"
      "MATHEMATICAL TARGETING MATRIX: CALIBRATED.\n"
      "PRIMARY OBJECTIVE: ASCEND AXIOM-7 TOWER AND\n"
      "INJECT THE CAMPOS PARADOX INTO S.I.G.M.A. MAINFRAME.\n\n"
      "[ GAMEPLAY STATE PLACEHOLDER ]\n"
      "CONNECT YOUR 3D LEVEL OR ACTION SCENE HERE.",
      32, 148, Color(200.0F, 230.0F, 220.0F, 128.0F));
}

}  // namespace DroidCampos
