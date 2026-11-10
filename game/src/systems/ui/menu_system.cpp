#include "systems/ui/menu_system.hpp"
#include <cmath>
#include <cstdio>

namespace DroidCampos {
namespace Systems {
namespace Ui {

using namespace Tyra;

namespace {
// GS MODULATE uses 128 as neutral; ui_white and the font atlas are 255 white.
Color menuInk(float r, float g, float b, float a = 128.0F) {
  return Color(r * (128.0F / 255.0F), g * (128.0F / 255.0F),
               b * (128.0F / 255.0F), a);
}
}  // namespace

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
      drawer_->drawBlueprintPanel(32, 144, 448, 170, menuInk(0, 0, 0, 128), menuInk(0, 190, 210, 128));
      const bool failed = state.scene.error != nullptr;
      drawer_->getFont().drawText(failed ? "FALHA NO CARREGAMENTO" : "CARREGANDO INSTALACAO ETIP", 52, 170,
                                  menuInk(170, 235, 240, 128));
      if (failed) {
        drawer_->getFont().drawText(state.scene.error, 52, 210, menuInk(250, 140, 90, 128));
        drawer_->getFont().drawText("O / X: VOLTAR AO MENU", 52, 270, menuInk(220, 220, 220, 128));
      } else {
        char progress[48];
        std::snprintf(progress, sizeof(progress), "ETAPA %d / %d", state.scene.loadStep, state.scene.totalSteps);
        drawer_->getFont().drawText(progress, 52, 216, menuInk(180, 210, 220, 128));
        drawer_->drawRect(52, 254, 408, 12, menuInk(18, 26, 35, 128));
        if (state.scene.totalSteps > 0 && state.scene.loadStep > 0)
          drawer_->drawRect(52, 254, 408.0F * state.scene.loadStep / state.scene.totalSteps, 12, menuInk(0, 190, 210, 128));
      }
      drawer_->renderFooter(failed ? "O / X: RETURN TO MENU" : "LOADING ETIP...");
      break;
    }
    case Core::ScreenState::Defeated:
      if(state.droid.phase==Core::DroidPhase::GameOver) {
        drawer_->drawBlueprintPanel(56,144,400,180,menuInk(0,0,0),menuInk(230,70,50));
        drawer_->getFont().drawText("GAME OVER",176,158,menuInk(250,130,100));
        drawer_->getFont().drawText("DROIDCAMPOS E INEVITAVEL",92,186,menuInk(250,130,100));
        drawer_->getFont().drawText(state.menu.pauseSelectedIndex==0?"> NEW GAME":"  NEW GAME",144,224,menuInk(200,225,235));
        drawer_->getFont().drawText(state.menu.pauseSelectedIndex==1?"> LOAD GAME":"  LOAD GAME",144,264,menuInk(200,225,235));
        drawer_->renderFooter("D-PAD: SELECIONAR  X: CONFIRMAR");break;
      }
      drawer_->drawBlueprintPanel(80,144,352,170,menuInk(0,0,0),menuInk(230,70,50));
      drawer_->getFont().drawText("VOCE FOI DERROTADO",120,168,menuInk(250,130,100));
      drawer_->getFont().drawText(state.menu.pauseSelectedIndex==0
          ? "> TENTAR NOVAMENTE" : "  TENTAR NOVAMENTE",112,218,menuInk(200,225,235));
      drawer_->getFont().drawText(state.menu.pauseSelectedIndex==1
          ? "> MENU PRINCIPAL" : "  MENU PRINCIPAL",112,256,menuInk(200,225,235));
      drawer_->renderFooter("D-PAD: SELECIONAR  X: CONFIRMAR");
      break;
    case Core::ScreenState::Paused:
      drawer_->drawBlueprintPanel(96, 144, 320, 160, menuInk(0, 0, 0), menuInk(0, 190, 210));
      drawer_->getFont().drawText("JOGO PAUSADO", 152, 164, menuInk(159, 239, 232));
      drawer_->getFont().drawText(state.menu.pauseSelectedIndex == 0
          ? "> CONTINUAR" : "  CONTINUAR", 128, 214, menuInk(200, 225, 235));
      drawer_->getFont().drawText(state.menu.pauseSelectedIndex == 1
          ? "> MENU PRINCIPAL" : "  MENU PRINCIPAL", 128, 250, menuInk(200, 225, 235));
      drawer_->renderFooter("D-PAD: SELECIONAR  X: CONFIRMAR  START/O: VOLTAR");
      break;
    case Core::ScreenState::MainMenu:
      renderMainMenu(state);
      drawer_->renderFooter(
          "D-PAD: NAVEGAR  X: SELECIONAR");
      break;
    case Core::ScreenState::NewGame:
      renderNewGameScreen(state);
      drawer_->renderFooter("X: ENTRAR NA ETIP  O: VOLTAR");
      break;
    case Core::ScreenState::LoadGame:
      renderLoadGameScreen(state);
      drawer_->renderFooter(
          "X: CARREGAR  O: VOLTAR");
      break;
    case Core::ScreenState::Settings:
      renderSettingsScreen(state);
      drawer_->renderFooter(
          "O: VOLTAR");
      break;
    case Core::ScreenState::GameplayFps:
      break;
  }
  drawer_->renderSync(state);
}

void MenuSystem::shutdown(Engine* engine) { (void)engine; }

void MenuSystem::renderMainMenu(const Core::WorldState& state) {
  for (int i = 0; i < 3; ++i)
    drawer_->renderMenuButton(i, state.menu.mainSelectedIndex == i);
}

void MenuSystem::renderNewGameScreen(const Core::WorldState& state) {
  drawer_->drawBlueprintPanel(16.0F, 108.0F, 480.0F, 286.0F,
                     menuInk(0, 0, 0, 128),
                     menuInk(99, 180, 179, 128));

  drawer_->getFont().drawText(
      "DOSSIE DA MISSAO // COMPLEXO ETIP", 32, 118,
      menuInk(159, 239, 232, 128));
  drawer_->drawRect(28.0F, 136.0F, 456.0F, 2.0F,
                    menuInk(54, 112, 115, 128));

  drawer_->getFont().drawText(
      "ANO 2066 - DISTRITO INDUSTRIAL DE UTINGA.\n"
      "A CORPORACAO ETIP MANTEM SUAS INSTALACOES\n"
      "SOB RIGIDO CONTROLE DE SEGURANCA.\n\n"
      "GUIADO PELA ORGANIZACAO REBELDE CJE,\n"
      "O AGENTE PM INICIA SUA INFILTRACAO\n"
      "NO COMPLEXO INDUSTRIAL DA ETIP.\n\n"
      "OBJETIVOS\n"
      "- AVANCE ATE O PORTAO PRINCIPAL.\n"
      "- LOCALIZE A GUARITA DE SEGURANCA.\n"
      "- UTILIZE O LEITOR DE CARTAO PARA ENTRAR.",
      32, 148, menuInk(200.0F, 225.0F, 235.0F, 128.0F), .9F);

  float pulse = (std::sin(state.anim.animTick * 4.0F) + 1.0F) * 0.5F;
  drawer_->drawBlueprintPanel(86.0F, 350.0F, 340.0F, 32.0F,
                     menuInk(145 + pulse * 14, 225 + pulse * 14, 218 + pulse * 14, 128),
                     menuInk(159, 239, 232, 128));
  drawer_->getFont().drawText("X: INICIAR MISSAO", 208, 358,
                              menuInk(0, 0, 0, 128));
}

void MenuSystem::renderLoadGameScreen(const Core::WorldState& state) {
  drawer_->drawBlueprintPanel(16.0F, 108.0F, 480.0F, 286.0F,
                     menuInk(0, 0, 0, 128),
                     menuInk(99, 180, 179, 128));

  drawer_->getFont().drawText("LOAD GAME // MEMORY CARD", 32,
                              118, menuInk(159, 239, 232, 128));
  drawer_->drawRect(28.0F, 136.0F, 456.0F, 2.0F,
                    menuInk(54, 112, 115, 128));

  const char* slotTitles[3] = {
      "SLOT 01 // ETIP PERIMETER GATE (ZONA A)",
      "SLOT 02 // ETIP QUANTUM RESEARCH LAB",
      "SLOT 03 // [ EMPTY / UNFORMATTED DATA BLOCK ]"};
  const char* slotDetails[3] = {
      "PM LVL 01 // TIME: 00:05:12",
      "PM LVL 02 // TIME: 00:54:09",
      "NO TELEMETRY CHECKSUM FOUND IN THIS MEMORY SECTOR"};

  for (int i = 0; i < 3; i++) {
    float slotY = 148.0F + static_cast<float>(i) * 74.0F;
    bool selected = (state.menu.loadSelectedIndex == i);

    if (selected) {
      drawer_->drawBlueprintPanel(32.0F, slotY, 448.0F, 60.0F,
                         menuInk(8, 35, 36, 128),
                         menuInk(159, 239, 232, 128));
      drawer_->drawRect(36.0F, slotY + 6.0F, 6.0F, 48.0F,
                        menuInk(159, 239, 232, 128));
      drawer_->getFont().drawText(slotTitles[i], 52,
                                  static_cast<int>(slotY + 12.0F),
                                  menuInk(255.0F, 255.0F, 255.0F, 128.0F), 1.1F);
      drawer_->getFont().drawText(slotDetails[i], 52,
                                  static_cast<int>(slotY + 36.0F),
                                  menuInk(140.0F, 235.0F, 255.0F, 128.0F), 1.0F);
    } else {
      drawer_->drawBlueprintPanel(32.0F, slotY, 448.0F, 60.0F,
                         menuInk(0, 0, 0, 128),
                         menuInk(60.0F, 80.0F, 100.0F, 95.0F));
      drawer_->getFont().drawText(slotTitles[i], 52,
                                  static_cast<int>(slotY + 12.0F),
                                  menuInk(160.0F, 175.0F, 190.0F, 128.0F), 1.0F);
      drawer_->getFont().drawText(slotDetails[i], 52,
                                  static_cast<int>(slotY + 36.0F),
                                  menuInk(100.0F, 115.0F, 130.0F, 128.0F), 1.0F);
    }
  }

  if (state.menu.loadSelectedIndex == 2) {
    drawer_->getFont().drawText("STATUS: CANNOT LOAD EMPTY DATA BLOCK", 32, 366,
                                menuInk(240.0F, 80.0F, 60.0F, 128.0F));
  } else {
    drawer_->getFont().drawText("STATUS: CHECKSUM VALID - PRESS CROSS TO LOAD",
                                32, 366, menuInk(80.0F, 230.0F, 130.0F, 128.0F));
  }
}

void MenuSystem::renderSettingsScreen(const Core::WorldState& state) {
  drawer_->drawBlueprintPanel(16.0F, 108.0F, 480.0F, 286.0F,
                     menuInk(0, 0, 0, 128),
                     menuInk(99, 180, 179, 128));

  drawer_->getFont().drawText(
      "SYSTEM SETTINGS // CALIBRATION", 32, 118,
      menuInk(159, 239, 232, 128));
  drawer_->drawRect(28.0F, 136.0F, 456.0F, 2.0F,
                    menuInk(54, 112, 115, 128));

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
    float rowY = 148.0F + static_cast<float>(i) * 56.0F;
    bool selected = (state.menu.settingsSelectedIndex == i);

    if (selected) {
      drawer_->drawBlueprintPanel(32.0F, rowY, 448.0F, 46.0F,
                         menuInk(8, 35, 36, 128),
                         menuInk(159, 239, 232, 128));
      drawer_->drawRect(36.0F, rowY + 6.0F, 5.0F, 34.0F,
                        menuInk(159, 239, 232, 128));
    } else {
      drawer_->drawBlueprintPanel(32.0F, rowY, 448.0F, 46.0F,
                         menuInk(0, 0, 0, 128),
                         menuInk(55.0F, 75.0F, 95.0F, 95.0F));
    }

    Color labelCol = selected ? menuInk(255.0F, 255.0F, 255.0F, 128.0F)
                              : menuInk(170.0F, 185.0F, 200.0F, 128.0F);
    drawer_->getFont().drawText(optionNames[i], 50,
                                static_cast<int>(rowY + 14.0F), labelCol);

    if (i == 0 || i == 1) {
      int vol = (i == 0) ? state.menu.bgmVolume : state.menu.sfxVolume;
      float barX = 245.0F;
      float barY = rowY + 15.0F;
      drawer_->drawRect(barX, barY, 140.0F, 14.0F,
                        menuInk(8.0F, 12.0F, 18.0F, 128.0F));
      float fillW = (136.0F * static_cast<float>(vol)) / 100.0F;
      if (fillW > 0.0F) {
        drawer_->drawRect(barX + 2.0F, barY + 2.0F, fillW, 10.0F,
                          menuInk(159, 239, 232, 128));
      }
      drawer_->getFont().drawText(valBuf[i], 398,
                                  static_cast<int>(rowY + 14.0F),
                                  menuInk(159, 239, 232, 128));
    } else {
      drawer_->getFont().drawText(valBuf[i], 270,
                                  static_cast<int>(rowY + 14.0F),
                                  menuInk(159, 239, 232, 128));
    }
  }
}

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
