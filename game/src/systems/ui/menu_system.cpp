#include "systems/ui/menu_system.hpp"
#include <cmath>
#include <cstdio>

namespace DroidCampos {
namespace Systems {
namespace Ui {

using namespace Tyra;

namespace {
// GS MODULATE uses 128 as neutral; ui_white and the font atlas are 255 white.
constexpr u32 kGalleryImageTicks = 6 * 15625;
constexpr u32 kGalleryCycleTicks = 4 * kGalleryImageTicks;

Color menuInk(float r, float g, float b, float a = 128.0F) {
  return Color(r * (128.0F / 255.0F), g * (128.0F / 255.0F),
               b * (128.0F / 255.0F), a);
}
const char* cardStatusText(Core::SaveStatus status) {
  switch (status) {
    case Core::SaveStatus::Checking: return "VERIFICANDO MEMORY CARD...";
    case Core::SaveStatus::Empty: return "NENHUM PROGRESSO SALVO";
    case Core::SaveStatus::NoCard: return "MEMORY CARD AUSENTE";
    case Core::SaveStatus::InvalidCard: return "USE UM MEMORY CARD PS2 FORMATADO";
    case Core::SaveStatus::Corrupt: return "PROGRESSO ILEGIVEL";
    case Core::SaveStatus::Incompatible: return "VERSAO DE SAVE INCOMPATIVEL";
    case Core::SaveStatus::Error: return "FALHA AO LER O MEMORY CARD";
    default: return "";
  }
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

  if (state.menu.currentScreen != Core::ScreenState::MainMenu) galleryActive_ = false;
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
      if (state.droid.phase == Core::DroidPhase::GameOver) {
        constexpr int panelX = 56;
        constexpr int panelWidth = 400;
        constexpr int centerX = panelX + panelWidth / 2;
        drawer_->drawBlueprintPanel(panelX, 130, panelWidth, 207,
                                    menuInk(0, 0, 0), menuInk(230, 70, 50));

        auto& font = drawer_->getFont();
        const auto drawCentered = [&](const char* label, int y, Color color,
                                      bool selected = false) {
          const int x = centerX - font.lineWidth(label) / 2;
          font.drawText(label, x, y, color);
          if (selected) {
            font.drawText(">", x - font.lineWidth(">") - 4, y, color);
          }
        };

        drawCentered("GAME OVER", 158, menuInk(250, 130, 100));
        drawCentered("DROIDCAMPOS E INEVITAVEL", 186, menuInk(250, 130, 100));
        drawCentered("TENTAR NOVAMENTE", 220, menuInk(200, 225, 235),
                     state.menu.pauseSelectedIndex == 0);
        drawCentered("NEW GAME", 255, menuInk(200, 225, 235),
                     state.menu.pauseSelectedIndex == 1);
        drawCentered("LOAD GAME", 290, menuInk(200, 225, 235),
                     state.menu.pauseSelectedIndex == 2);
        drawer_->renderFooter("D-PAD: SELECIONAR  X: CONFIRMAR");
        break;
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
          ? "> CONTINUAR" : "  CONTINUAR", 128, 207, menuInk(200, 225, 235));
      drawer_->getFont().drawText(state.menu.pauseSelectedIndex == 1
          ? "> CONFIGURACOES" : "  CONFIGURACOES", 128, 237, menuInk(200, 225, 235));
      drawer_->getFont().drawText(state.menu.pauseSelectedIndex == 2
          ? "> MENU PRINCIPAL" : "  MENU PRINCIPAL", 128, 267, menuInk(200, 225, 235));
      drawer_->renderFooter("D-PAD: SELECIONAR  X: CONFIRMAR  START/O: VOLTAR");
      break;
    case Core::ScreenState::MainMenu:
      renderMainMenu(state);
      drawer_->renderFooter(
          "D-PAD: NAVEGAR  X: SELECIONAR");
      break;
    case Core::ScreenState::NewGame:
      renderNewGameScreen(state);
      drawer_->renderFooter(!state.menu.saveDialog ? "X: INICIAR  O: VOLTAR"
          : state.menu.saveStatus == Core::SaveStatus::Checking ? "O: VOLTAR"
          : state.menu.confirmNewGame ? "X: CONFIRMAR  O: CANCELAR"
          : "D-PAD: SELECIONAR  X: CONFIRMAR  O: VOLTAR");
      break;
    case Core::ScreenState::LoadGame:
      renderLoadGameScreen(state);
      drawer_->renderFooter(state.menu.saveStatus == Core::SaveStatus::Ready
          ? "X: CARREGAR  O: VOLTAR" : "O: VOLTAR");
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
  galleryTicks_ = galleryActive_
      ? (galleryTicks_ + galleryTimer_.getTimeDelta()) % kGalleryCycleTicks : 0;
  galleryTimer_.prime();
  galleryActive_ = true;
  const u32 imageTicks = galleryTicks_ % kGalleryImageTicks;
  const bool changing = imageTicks < 1875 || imageTicks >= kGalleryImageTicks - 1875;
  const bool disturbance = imageTicks >= 43750 && imageTicks < 45000;
  const int glitchPhase = changing || disturbance ? (galleryTicks_ / 400) % 4 : -1;
  drawer_->renderGalleryImage(galleryTicks_ / kGalleryImageTicks, glitchPhase);
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
      "- PEGUE A ARMA E O CARTAO NA GUARITA.\n"
      "- UTILIZE O LEITOR DE CARTAO PARA ENTRAR.",
      32, 148, menuInk(200.0F, 225.0F, 235.0F, 128.0F), .9F);

  if (state.menu.saveDialog) {
    const auto status = state.menu.saveStatus;
    auto& font = drawer_->getFont();
    drawer_->drawRect(0, 0, 512, 448, menuInk(0, 0, 0, 88));
    drawer_->drawBlueprintPanel(28, 101, 456, 248, menuInk(5, 15, 23),
                                menuInk(99, 180, 179));
    font.drawText("MEMORY CARD PS2 / ENTRADA 1", 52, 123, menuInk(159, 239, 232));
    if (status == Core::SaveStatus::Checking) {
      font.drawText("VERIFICANDO MEMORY CARD...", 52, 185, menuInk(200, 225, 235));
    } else if (state.menu.confirmNewGame) {
      font.drawText("SUBSTITUIR O PROGRESSO ANTERIOR?", 52, 175, menuInk(240, 190, 95));
      font.drawText("O SAVE ANTERIOR SERA PRESERVADO ATE\nO PRIMEIRO AUTOSAVE DA NOVA PARTIDA.",
                    52, 218, menuInk(200, 225, 235));
    } else {
      const bool ready = Core::canEnableAutosave(status);
      font.drawText(ready ? "DESEJA SALVAR O PROGRESSO NO\nMEMORY CARD?" : cardStatusText(status),
                    52, 170, menuInk(200, 225, 235));
      font.drawText(state.menu.saveChoice == 0 ? ">" : " ", 52, 250, menuInk(159, 239, 232));
      font.drawText(ready ? "SALVAR AUTOMATICAMENTE" : "VERIFICAR NOVAMENTE",
                    76, 250, menuInk(200, 225, 235));
      font.drawText(state.menu.saveChoice == 1 ? ">" : " ", 52, 292, menuInk(159, 239, 232));
      font.drawText("JOGAR SEM SALVAR", 76, 292, menuInk(200, 225, 235));
    }
    return;
  }
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

  const auto status = state.menu.saveStatus;
  const char* label = status == Core::SaveStatus::Ready
      ? Core::checkpointName(state.menu.savedStage) : cardStatusText(status);
  auto& font = drawer_->getFont();
  font.drawText("MEMORY CARD PS2 // ENTRADA 1", 32, 169, menuInk(159, 239, 232));
  font.drawText(label, 32, 218, menuInk(220, 238, 245));
}

void MenuSystem::renderSettingsScreen(const Core::WorldState& state) {
  constexpr int optionCount = Core::MenuNavigationState::kSettingsOptionCount;
  constexpr float panelY = 108.0F;
  constexpr float firstRowY = 148.0F;
  constexpr float rowHeight = 41.0F;
  constexpr float rowPitch = 47.0F;
  constexpr float bottomPadding = 17.0F;
  constexpr float panelHeight =
      firstRowY - panelY + (optionCount - 1) * rowPitch + rowHeight + bottomPadding;
  drawer_->drawBlueprintPanel(16.0F, panelY, 480.0F, panelHeight,
                     menuInk(0, 0, 0, 128),
                     menuInk(99, 180, 179, 128));

  drawer_->getFont().drawText(
      "SYSTEM SETTINGS // CALIBRATION", 32, 118,
      menuInk(159, 239, 232, 128));
  drawer_->drawRect(28.0F, 136.0F, 456.0F, 2.0F,
                    menuInk(54, 112, 115, 128));

  const char* optionNames[optionCount] = {
      "BGM SYNTH VOLUME", "INDUSTRIAL SFX VOL", "VOLUME DO AMBIENTE"};

  const int volumes[] = {state.menu.bgmVolume, state.menu.sfxVolume, state.menu.ambientVolume};
  for (int i = 0; i < optionCount; ++i) {
    const float rowY = firstRowY + i * rowPitch;
    bool selected = (state.menu.settingsSelectedIndex == i);

    if (selected) {
      drawer_->drawBlueprintPanel(32.0F, rowY, 448.0F, rowHeight,
                         menuInk(8, 35, 36, 128),
                         menuInk(159, 239, 232, 128));
      drawer_->drawRect(36.0F, rowY + 6.0F, 5.0F, 29.0F,
                        menuInk(159, 239, 232, 128));
    } else {
      drawer_->drawBlueprintPanel(32.0F, rowY, 448.0F, rowHeight,
                         menuInk(0, 0, 0, 128),
                         menuInk(55.0F, 75.0F, 95.0F, 95.0F));
    }

    Color labelCol = selected ? menuInk(255.0F, 255.0F, 255.0F, 128.0F)
                              : menuInk(170.0F, 185.0F, 200.0F, 128.0F);
    drawer_->getFont().drawText(optionNames[i], 50,
                                static_cast<int>(rowY + 14.0F), labelCol);

    constexpr float barX = 245.0F;
    const float barY = rowY + 15.0F;
    drawer_->drawRect(barX, barY, 140.0F, 14.0F,
                      menuInk(8.0F, 12.0F, 18.0F, 128.0F));
    const float fillW = 136.0F * volumes[i] / 100.0F;
    if (fillW > 0.0F) {
      drawer_->drawRect(barX + 2.0F, barY + 2.0F, fillW, 10.0F,
                        menuInk(159, 239, 232, 128));
    }
    char value[8];
    std::snprintf(value, sizeof(value), "%d%%", volumes[i]);
    drawer_->getFont().drawText(value, 398, static_cast<int>(rowY + 14.0F),
                                menuInk(159, 239, 232, 128));
  }
}

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
