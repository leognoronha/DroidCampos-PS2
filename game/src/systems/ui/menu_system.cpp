#include "systems/ui/menu_system.hpp"
#include <cmath>
#include <cstdio>

namespace DroidCampos {
namespace Systems {
namespace Ui {

using namespace Tyra;

MenuSystem::MenuSystem(UiContextDrawer* drawer) : drawer_(drawer) {}

MenuSystem::MenuSystem(UiContextDrawer& drawer) : drawer_(&drawer) {}

void MenuSystem::init(Engine* engine, Core::WorldState& state,
                      Events::EventBus& bus) {
  (void)state;
  (void)bus;
  if (drawer_ != nullptr) {
    drawer_->init(engine);
  }
}

void MenuSystem::executeStage(Core::HardwareStage stage, Engine* engine,
                              Core::WorldState& state, Events::EventBus& bus,
                              Hardware::Ps2ScratchpadArena& scratchpad) {
  (void)engine;
  (void)bus;
  (void)scratchpad;

  if (stage != Core::HardwareStage::Stage3_GifPath3Render2D ||
      drawer_ == nullptr) {
    return;
  }

  if (state.menu.currentScreen == Core::ScreenState::GameplayFps) {
    return;
  }

  drawer_->renderBackground(state);
  drawer_->renderHeader();

  switch (state.menu.currentScreen) {
    case Core::ScreenState::Loading: {
      drawer_->drawPanel(32, 144, 448, 170, Color(5, 10, 18, 128), Color(0, 190, 210, 128));
      const bool failed = state.scene.error != nullptr;
      drawer_->getFont().drawText(failed ? "FALHA NO CARREGAMENTO" : "CARREGANDO INSTALACAO ETIP", 52, 170,
                                  Color(170, 235, 240, 128));
      if (failed) {
        drawer_->getFont().drawText(state.scene.error, 52, 210, Color(250, 140, 90, 128));
        drawer_->getFont().drawText("X: VOLTAR AO MENU", 52, 270, Color(220, 220, 220, 128));
      } else {
        char progress[48];
        std::snprintf(progress, sizeof(progress), "ETAPA %d / %d", state.scene.loadStep, state.scene.totalSteps);
        drawer_->getFont().drawText(progress, 52, 216, Color(180, 210, 220, 128));
        drawer_->drawRect(52, 254, 408, 12, Color(18, 26, 35, 128));
        if (state.scene.totalSteps > 0 && state.scene.loadStep > 0)
          drawer_->drawRect(52, 254, 408.0F * state.scene.loadStep / state.scene.totalSteps, 12, Color(0, 190, 210, 128));
      }
      break;
    }
    case Core::ScreenState::MainMenu:
      renderMainMenu(state);
      drawer_->renderFooter(
          "DPAD: NAVIGATE   CROSS/START: SELECT   AXIOM-OS V4.09");
      break;
    case Core::ScreenState::NewGame:
      renderNewGameScreen(state);
      drawer_->renderFooter("CROSS/START: ENTER ETIP 3D FPS   CIRCLE: BACK");
      break;
    case Core::ScreenState::LoadGame:
      renderLoadGameScreen(state);
      drawer_->renderFooter(
          "DPAD: SELECT SLOT   CROSS: LOAD 3D FPS   CIRCLE: BACK");
      break;
    case Core::ScreenState::Settings:
      renderSettingsScreen(state);
      drawer_->renderFooter(
          "DPAD UP/DOWN: OPTION   LEFT/RIGHT: ADJUST   CIRCLE: BACK");
      break;
    case Core::ScreenState::GameplayFps:
      break;
  }
}

void MenuSystem::shutdown(Engine* engine) { (void)engine; }

void MenuSystem::renderMainMenu(const Core::WorldState& state) {
  drawer_->drawPanel(16.0F, 90.0F, 236.0F, 304.0F,
                     Color(12.0F, 18.0F, 28.0F, 112.0F),
                     Color(0.0F, 180.0F, 220.0F, 115.0F));

  drawer_->getFont().drawText("COMMAND TERMINAL", 32, 102,
                              Color(230.0F, 165.0F, 30.0F, 128.0F));
  drawer_->drawRect(26.0F, 122.0F, 216.0F, 1.0F,
                    Color(0.0F, 160.0F, 200.0F, 90.0F));

  const char* labels[3] = {"NEW GAME", "LOAD GAME", "SETTINGS"};
  const char* subLabels[3] = {"DEPLOY AT ETIP GATE (FPS)",
                              "READ MEMORY CARD SLOTS",
                              "AUDIO & OPTICAL CONFIG"};

  for (int i = 0; i < 3; i++) {
    float itemY = 138.0F + static_cast<float>(i) * 66.0F;
    bool selected = (state.menu.mainSelectedIndex == i);

    if (selected) {
      float pulse = (std::sin(state.anim.animTick * 3.0F) + 1.0F) * 0.5F;
      drawer_->drawPanel(28.0F, itemY, 212.0F, 52.0F,
                         Color(0.0F, 65.0F + pulse * 25.0F, 95.0F, 118.0F),
                         Color(0.0F, 230.0F, 255.0F, 128.0F));
      drawer_->drawRect(32.0F, itemY + 6.0F, 5.0F, 40.0F,
                        Color(255.0F, 190.0F, 20.0F, 128.0F));

      drawer_->getFont().drawText(">", 44, static_cast<int>(itemY + 8.0F),
                                  Color(255.0F, 200.0F, 40.0F, 128.0F), 1.3F);
      drawer_->getFont().drawText(labels[i], 62, static_cast<int>(itemY + 8.0F),
                                  Color(255.0F, 255.0F, 255.0F, 128.0F), 1.3F);
      drawer_->getFont().drawText(subLabels[i], 46,
                                  static_cast<int>(itemY + 32.0F),
                                  Color(140.0F, 235.0F, 255.0F, 128.0F), 1.0F);
    } else {
      drawer_->drawPanel(28.0F, itemY, 212.0F, 52.0F,
                         Color(16.0F, 24.0F, 36.0F, 95.0F),
                         Color(55.0F, 75.0F, 95.0F, 95.0F));
      drawer_->getFont().drawText(labels[i], 48, static_cast<int>(itemY + 8.0F),
                                  Color(160.0F, 180.0F, 195.0F, 128.0F), 1.2F);
      drawer_->getFont().drawText(subLabels[i], 46,
                                  static_cast<int>(itemY + 32.0F),
                                  Color(100.0F, 120.0F, 135.0F, 128.0F), 1.0F);
    }
  }

  drawer_->getFont().drawText("3D ENGINE: ETIP + PISTOL", 30, 346,
                              Color(90.0F, 190.0F, 140.0F, 128.0F));
  drawer_->getFont().drawText("BUILD: PS2-TYRA-v0.0.1", 30, 366,
                              Color(120.0F, 140.0F, 160.0F, 128.0F));

  drawer_->renderCyborgPortrait(268.0F, 90.0F, state.anim.animTick);
}

void MenuSystem::renderNewGameScreen(const Core::WorldState& state) {
  drawer_->drawPanel(16.0F, 90.0F, 480.0F, 304.0F,
                     Color(10.0F, 16.0F, 26.0F, 118.0F),
                     Color(240.0F, 160.0F, 20.0F, 125.0F));

  drawer_->getFont().drawText(
      "CLASSIFIED DOSSIER // INFILTRATING ETIP COMPLEX", 32, 104,
      Color(255.0F, 190.0F, 30.0F, 128.0F));
  drawer_->drawRect(28.0F, 124.0F, 456.0F, 2.0F,
                    Color(240.0F, 160.0F, 20.0F, 100.0F));

  drawer_->getFont().drawText(
      "YEAR 2147 - NEO-CAMPOS INDUSTRIAL WASTELAND.\n"
      "DR. VICTOR CAMPOS, CHIEF MATHEMATICIAN AT AXIOM-7,\n"
      "ARRIVES AT THE ABANDONED PERIMETER OF THE ETIP\n"
      "INDUSTRIAL INSTALLATION.\n\n"
      "OBJECTIVE:\n"
      "- ADVANCE THROUGH ACCESS ROAD (ZONA A -> ZONA B)\n"
      "- INSPECT SECURITY GUARITA & CARD READER AT GATE\n"
      "- BREACH MAIN INDUSTRIAL GATE INTO ETIP FACILITY.",
      32, 136, Color(200.0F, 225.0F, 235.0F, 128.0F));

  float pulse = (std::sin(state.anim.animTick * 4.0F) + 1.0F) * 0.5F;
  drawer_->drawPanel(86.0F, 344.0F, 340.0F, 36.0F,
                     Color(0.0F, 70.0F + pulse * 40.0F, 100.0F, 120.0F),
                     Color(0.0F, 235.0F, 255.0F, 128.0F));
  drawer_->getFont().drawText(">> PRESS CROSS TO START 3D FPS <<", 112, 354,
                              Color(255.0F, 240.0F, 150.0F, 128.0F));
}

void MenuSystem::renderLoadGameScreen(const Core::WorldState& state) {
  drawer_->drawPanel(16.0F, 90.0F, 480.0F, 304.0F,
                     Color(10.0F, 16.0F, 26.0F, 118.0F),
                     Color(0.0F, 200.0F, 240.0F, 125.0F));

  drawer_->getFont().drawText("LOAD GAME // MEMORY CARD [mc0:/DROIDCAMPOS]", 32,
                              104, Color(0.0F, 225.0F, 255.0F, 128.0F));
  drawer_->drawRect(28.0F, 124.0F, 456.0F, 2.0F,
                    Color(0.0F, 180.0F, 220.0F, 100.0F));

  const char* slotTitles[3] = {
      "SLOT 01 // ETIP PERIMETER GATE (ZONA A)",
      "SLOT 02 // AXIOM-7 QUANTUM RESEARCH LAB",
      "SLOT 03 // [ EMPTY / UNFORMATTED DATA BLOCK ]"};
  const char* slotDetails[3] = {
      "DR. CAMPOS LVL 01   NEURAL SYNC: 98%   TIME: 00:05:12",
      "DR. CAMPOS LVL 02   NEURAL SYNC: 78%   TIME: 00:54:09",
      "NO TELEMETRY CHECKSUM FOUND IN THIS MEMORY SECTOR"};

  for (int i = 0; i < 3; i++) {
    float slotY = 140.0F + static_cast<float>(i) * 74.0F;
    bool selected = (state.menu.loadSelectedIndex == i);

    if (selected) {
      drawer_->drawPanel(32.0F, slotY, 448.0F, 60.0F,
                         Color(0.0F, 60.0F, 90.0F, 118.0F),
                         Color(255.0F, 195.0F, 30.0F, 128.0F));
      drawer_->drawRect(36.0F, slotY + 6.0F, 6.0F, 48.0F,
                        Color(255.0F, 195.0F, 30.0F, 128.0F));
      drawer_->getFont().drawText(slotTitles[i], 52,
                                  static_cast<int>(slotY + 12.0F),
                                  Color(255.0F, 255.0F, 255.0F, 128.0F), 1.1F);
      drawer_->getFont().drawText(slotDetails[i], 52,
                                  static_cast<int>(slotY + 36.0F),
                                  Color(140.0F, 235.0F, 255.0F, 128.0F), 1.0F);
    } else {
      drawer_->drawPanel(32.0F, slotY, 448.0F, 60.0F,
                         Color(16.0F, 22.0F, 34.0F, 95.0F),
                         Color(60.0F, 80.0F, 100.0F, 95.0F));
      drawer_->getFont().drawText(slotTitles[i], 52,
                                  static_cast<int>(slotY + 12.0F),
                                  Color(160.0F, 175.0F, 190.0F, 128.0F), 1.0F);
      drawer_->getFont().drawText(slotDetails[i], 52,
                                  static_cast<int>(slotY + 36.0F),
                                  Color(100.0F, 115.0F, 130.0F, 128.0F), 1.0F);
    }
  }

  if (state.menu.loadSelectedIndex == 2) {
    drawer_->getFont().drawText("STATUS: CANNOT LOAD EMPTY DATA BLOCK", 32, 366,
                                Color(240.0F, 80.0F, 60.0F, 128.0F));
  } else {
    drawer_->getFont().drawText("STATUS: CHECKSUM VALID - PRESS CROSS TO LOAD",
                                32, 366, Color(80.0F, 230.0F, 130.0F, 128.0F));
  }
}

void MenuSystem::renderSettingsScreen(const Core::WorldState& state) {
  drawer_->drawPanel(16.0F, 90.0F, 480.0F, 304.0F,
                     Color(10.0F, 16.0F, 26.0F, 118.0F),
                     Color(0.0F, 200.0F, 240.0F, 125.0F));

  drawer_->getFont().drawText(
      "SYSTEM SETTINGS // CYBERNETIC & AUDIO CALIBRATION", 32, 104,
      Color(230.0F, 165.0F, 30.0F, 128.0F));
  drawer_->drawRect(28.0F, 124.0F, 456.0F, 2.0F,
                    Color(0.0F, 180.0F, 220.0F, 100.0F));

  const char* optionNames[4] = {"BGM SYNTH VOLUME", "INDUSTRIAL SFX VOL",
                                "OPTICAL SCANLINES", "GS TEXTURE FILTER"};

  char valBuf[4][32];
  std::snprintf(valBuf[0], sizeof(valBuf[0]), "%d%%", state.menu.bgmVolume);
  std::snprintf(valBuf[1], sizeof(valBuf[1]), "%d%%", state.menu.sfxVolume);
  std::snprintf(valBuf[2], sizeof(valBuf[2]), "%s",
                state.menu.scanlinesEnabled ? "[ ENABLED ]" : "[ DISABLED ]");
  std::snprintf(valBuf[3], sizeof(valBuf[3]), "%s",
                state.menu.nearestFilter ? "[ NEAREST / PIXEL ]"
                                         : "[ LINEAR / SMOOTH ]");

  for (int i = 0; i < 4; i++) {
    float rowY = 142.0F + static_cast<float>(i) * 56.0F;
    bool selected = (state.menu.settingsSelectedIndex == i);

    if (selected) {
      drawer_->drawPanel(32.0F, rowY, 448.0F, 46.0F,
                         Color(0.0F, 60.0F, 90.0F, 115.0F),
                         Color(0.0F, 230.0F, 255.0F, 128.0F));
      drawer_->drawRect(36.0F, rowY + 6.0F, 5.0F, 34.0F,
                        Color(255.0F, 195.0F, 30.0F, 128.0F));
    } else {
      drawer_->drawPanel(32.0F, rowY, 448.0F, 46.0F,
                         Color(16.0F, 22.0F, 34.0F, 95.0F),
                         Color(55.0F, 75.0F, 95.0F, 95.0F));
    }

    Color labelCol = selected ? Color(255.0F, 255.0F, 255.0F, 128.0F)
                              : Color(170.0F, 185.0F, 200.0F, 128.0F);
    drawer_->getFont().drawText(optionNames[i], 50,
                                static_cast<int>(rowY + 14.0F), labelCol);

    if (i == 0 || i == 1) {
      int vol = (i == 0) ? state.menu.bgmVolume : state.menu.sfxVolume;
      float barX = 245.0F;
      float barY = rowY + 15.0F;
      drawer_->drawRect(barX, barY, 140.0F, 14.0F,
                        Color(8.0F, 12.0F, 18.0F, 128.0F));
      float fillW = (136.0F * static_cast<float>(vol)) / 100.0F;
      if (fillW > 0.0F) {
        drawer_->drawRect(barX + 2.0F, barY + 2.0F, fillW, 10.0F,
                          Color(0.0F, 215.0F, 250.0F, 128.0F));
      }
      drawer_->getFont().drawText(valBuf[i], 398,
                                  static_cast<int>(rowY + 14.0F),
                                  Color(255.0F, 200.0F, 40.0F, 128.0F));
    } else {
      drawer_->getFont().drawText(valBuf[i], 270,
                                  static_cast<int>(rowY + 14.0F),
                                  Color(120.0F, 240.0F, 170.0F, 128.0F));
    }
  }
}

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
