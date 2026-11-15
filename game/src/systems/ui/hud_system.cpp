#include "systems/ui/hud_system.hpp"
#include "core/pickup_layout.hpp"
#include <cstdio>

#include "core/terminal_documents.hpp"

namespace DroidCampos {
namespace Systems {
namespace Ui {

using namespace Tyra;

HudSystem::HudSystem(UiContextDrawer* drawer) : drawer_(drawer) {}

HudSystem::HudSystem(UiContextDrawer& drawer) : drawer_(&drawer) {}

void HudSystem::init(Engine* engine, Core::WorldState& state,
                     Events::EventBus& bus) {
  (void)state;
  (void)bus;
  if (drawer_ != nullptr) {
    drawer_->init(engine);
  }
}

void HudSystem::executeStage(Core::HardwareStage stage, Engine* engine,
                             Core::WorldState& state, Events::EventBus& bus,
                             Hardware::Ps2ScratchpadArena& scratchpad) {
  (void)engine;
  (void)bus;
  (void)scratchpad;

  if (stage == Core::HardwareStage::Stage3_GifPath3Render2D) {
    if (state.menu.currentScreen == Core::ScreenState::GameplayFps &&
        !state.intro.active && drawer_ != nullptr) {
      renderFpsHud(state);
    }
  }
}

void HudSystem::shutdown(Engine* engine) { (void)engine; }

void HudSystem::renderTerminal(const Core::WorldState& state) {
  const Color cyan(80, 120, 116, 128), dim(27, 56, 58, 128);
  const Color black(0, 0, 0, 128), paper(100, 113, 118, 128);
  auto& font = drawer_->getFont();
  const auto& terminal = state.scene;
  const Color red(125, 42, 42, 128), darkRed(70, 22, 22, 128);
  const bool truthSelected = terminal.terminalTopic == Core::kTruthTopic;
  const Color accent = truthSelected ? red : cyan;
  const bool corrupted = truthSelected && terminal.terminalReading &&
      terminal.terminalPage >= Core::kTruthCorruptionFirstPage;
  const int topicCount = Core::kTruthTopic + (state.encounter.newEntryUnlocked ? 1 : 0);
  static const char* topics[] = {"CORPORATIVO", "HISTORICO", "OCORRENCIAS", "A VERDADE"};
  static const char* summaries[] = {
      "PROTOCOLO DROIDCAMPOS\n\nSTATUS: DESCONTINUADO\nNIVEL DE SIGILO: RESTRITO",
      "RELATORIO PESSOAL - FUNCIONARIO\n\nIDENTIFICACAO: [DADOS SUPRIMIDOS]\nSTATUS: NAO AUTORIZADO",
      "DOSSIE DE INVESTIGACAO - CJE\n\nAMEACA INTERNA / EXTERNA\nNIVEL DE SIGILO: RESTRITO",
      "ARQUIVO DE DIRETORIA\n\nSIGILO ABSOLUTO\nSTATUS: [REGISTRO ADULTERADO]\n\nX: ABRIR NOVO REGISTRO"};
  const auto& document = Core::kTerminalDocuments[terminal.terminalTopic];

  drawer_->drawRect(0, 0, 512, 448, black);
  drawer_->drawBlueprintPanel(16, 20, 480, 66, black, cyan);
  font.drawText("ETIP Corporation.", 30, 32, cyan, 1.25F);
  font.drawText("ARQUIVO INTERNO // TERMINAL 01", 30, 62, paper, .9F);
  drawer_->drawBlueprintPanel(16, 98, 156, 294, black, dim);
  drawer_->drawBlueprintPanel(182, 98, 314, 294, black, accent);
  font.drawText("ACESSO LOCAL", 28, 112, cyan, .9F);
  for (int i = 0; i < topicCount; ++i) {
    const int y = 148 + i * (topicCount == 4 ? 53 : 66);
    const int height = topicCount == 4 ? 47 : 54;
    const Color tabAccent = i == Core::kTruthTopic ? red : cyan;
    const Color tabBorder = i == Core::kTruthTopic ? darkRed : dim;
    const bool selected = terminal.terminalTopic == i;
    drawer_->drawBlueprintPanel(24, y, 140, height,
                                selected ? tabAccent : black, selected ? tabAccent : tabBorder);
    char label[32];
    std::snprintf(label, sizeof(label), "0%d / ARQUIVO", i + 1);
    font.drawText(label, 34, y + 7, selected ? black : tabBorder, .8F);
    font.drawText(topics[i], 34, y + 26, selected ? black : tabAccent, .85F);
  }
  font.drawText("USO INTERNO ETIP", 28, 366, dim, .75F);
  if (terminal.terminalBootFrames > 0) {
    font.drawText("ESTABELECENDO ACESSO...", 198, 160, cyan, .9F);
    drawer_->drawRect(198, 198, 276, 4, dim);
    drawer_->drawRect(198, 198, 276.0F * (24-terminal.terminalBootFrames)/24, 4, cyan);
  } else {
    font.drawText(topics[terminal.terminalTopic], 198, 112, accent, 1.0F);
    drawer_->drawRect(198, 156, 282, 1, dim);
    const char* body = terminal.terminalReading ? document.pages[terminal.terminalPage]
                                               : summaries[terminal.terminalTopic];
    if (corrupted) {
      const unsigned int frame = terminal.terminalAnimationFrame;
      const int shift = frame % 24 < 6 ? 3 : 0;
      font.drawText(body, 198 - shift, 174 + shift, red, .9F);
      drawer_->drawRect(195, 173 + (frame * 3 % 176), 286, 2, darkRed);
      drawer_->drawRect(201 + (frame * 7 % 180), 174 + (frame * 5 % 175),
                        68, 1, red);
    } else {
      font.drawText(body, 198, 174, paper, .9F);
    }
    char page[48];
    if (terminal.terminalReading) {
      std::snprintf(page, sizeof(page), "DOC 0%d // PAGINA %d/%d",
                    terminal.terminalTopic + 1, terminal.terminalPage + 1,
                    document.pageCount);
    } else {
      std::snprintf(page, sizeof(page), "DOC 0%d // %s",
                    terminal.terminalTopic + 1, truthSelected ? "NOVO REGISTRO" : "PREVIA");
    }
    font.drawText(page, 198, 366, accent, .85F);
  }
  drawer_->drawBlueprintPanel(16, 404, 480, 28, black, dim);
  font.drawText(terminal.terminalReading
      ? "ESQ/DIR: PAGINA   O: VOLTAR   TRIANGULO: SAIR"
      : "CIMA/BAIXO: TOPICO   X: ABRIR   O / TRIANGULO: SAIR",
      26, 412, cyan, .85F);
  if (state.menu.scanlinesEnabled)
    for (int y = 22; y < 432; y += 4)
      drawer_->drawRect(18, y, 476, 1, Color(0, 0, 0, 16));
}

void HudSystem::renderFpsHud(const Core::WorldState& state) {
  if (drawer_ == nullptr) {
    return;
  }
  if (state.scene.terminalActive) {
    renderTerminal(state);
    return;
  }

  const Color cyan(55,120,120,128),dim(20,48,50,128);
  auto& font=drawer_->getFont();
  if(state.damageFlashTicks) {
    const Color flash(110,10,5,40);
    drawer_->drawRect(0,0,512,6,flash);drawer_->drawRect(0,442,512,6,flash);
    drawer_->drawRect(0,0,6,448,flash);drawer_->drawRect(506,0,6,448,flash);
  }
  constexpr int statusBottom = 404;
  constexpr int statusHeight = 46;
  constexpr int statusY = statusBottom - statusHeight;
  constexpr int healthX = 12;
  constexpr int healthWidth = 138;
  constexpr int kitX = healthX + healthWidth + 8;
  constexpr int kitWidth = 44;
  constexpr int promptWidth = kitX + kitWidth - healthX;
  constexpr int promptHeight = 26;
  constexpr int promptGap = 4;
  constexpr int promptY = statusY - promptHeight - promptGap;
  if (!state.radio.popupActive) {
    constexpr int barWidth = healthWidth - 20;
    const Color vitalsFill(2, 7, 10, 112);

    drawer_->drawBlueprintPanel(healthX, statusY, healthWidth, statusHeight,
                                vitalsFill, cyan);
    char health[24];
    std::snprintf(health, sizeof(health), "VIDA %03d / 100", state.playerHealth);
    const Color life = state.playerHealth < 25 ? Color(128, 35, 18, 128) : cyan;
    font.drawText(health, healthX + 10, statusY + 7, life, .9F);
    drawer_->drawRect(healthX + 10, statusY + 29, barWidth, 4, dim);
    if (state.playerHealth > 0) {
      drawer_->drawRect(healthX + 10, statusY + 29,
                        barWidth * static_cast<float>(state.playerHealth) / Core::kPlayerHealth,
                        4, life);
    }

    const Color kitGreen(34, 120, 60, 128);
    drawer_->drawBlueprintPanel(kitX, statusY, kitWidth, statusHeight, vitalsFill, cyan);
    drawer_->drawRect(kitX + 11, statusY + 10, 4, 12, kitGreen);
    drawer_->drawRect(kitX + 7, statusY + 14, 12, 4, kitGreen);
    char medkits[8];
    std::snprintf(medkits, sizeof(medkits), "%d/%d", state.inventory.medkits,
                  Core::kMedkitCapacity);
    font.drawText(medkits, kitX + 22, statusY + 8, kitGreen, .9F);
    font.drawText("R2", kitX + 22, statusY + 23, cyan, .75F);
  }

  const auto& encounter=state.encounter;
  char waveTitle[32]{};
  char waveDetail[40]{};
  const auto seconds = (encounter.countdownTicks + 15624) / 15625;
  switch (encounter.phase) {
    case Core::EncounterPhase::Countdown:
      std::snprintf(waveTitle, sizeof(waveTitle), "ONDA 1");
      std::snprintf(waveDetail, sizeof(waveDetail), "Comeca em %u s", seconds);
      break;
    case Core::EncounterPhase::Wave:
      std::snprintf(waveTitle, sizeof(waveTitle), "ONDA %d", encounter.wave);
      std::snprintf(waveDetail, sizeof(waveDetail),
                    encounter.remaining == 1 ? "Resta %d" : "Restam %d",
                    encounter.remaining);
      break;
    case Core::EncounterPhase::Intermission:
      std::snprintf(waveTitle, sizeof(waveTitle), "ONDA %d", encounter.wave + 1);
      std::snprintf(waveDetail, sizeof(waveDetail), "Em %u s  .  Vida +25", seconds);
      break;
    case Core::EncounterPhase::Complete:
      if (state.droid.phase >= Core::DroidPhase::Release) {
        std::snprintf(waveTitle, sizeof(waveTitle), "DROIDCAMPOS");
        std::snprintf(waveDetail, sizeof(waveDetail), "Contencao corrompida");
      } else if (encounter.completionNoticeTicks > 0) {
        std::snprintf(waveTitle, sizeof(waveTitle), "AMEACA ELIMINADA");
        std::snprintf(waveDetail, sizeof(waveDetail), "3/3 ondas");
      }
      break;
    default:
      break;
  }
  const bool showNewEntry = encounter.newEntryUnlocked &&
                            !encounter.objectiveComplete && !state.radio.popupActive;
  constexpr int newEntryY = 50;
  constexpr int newEntryHeight = 42;
  if (waveTitle[0]) {
    const auto drawWaveLine = [&](const char* label, int y, float scale, Color ink) {
      constexpr int screenCenterX = 256;
      const int x = screenCenterX -
                    static_cast<int>(font.lineWidth(label) * scale / 2.0F + 0.5F);
      font.drawText(label, x + 1, y + 1, Color(0, 0, 0, 128), scale);
      font.drawText(label, x, y, ink, scale);
    };
    const int titleY = showNewEntry ? newEntryY + newEntryHeight + 4 : 67;
    drawWaveLine(waveTitle, titleY, 2.25F, Color(105, 128, 128, 128));
    drawWaveLine(waveDetail, titleY + 34, 1.25F, Color(70, 120, 128, 128));
  }
  if (showNewEntry) {
    drawer_->drawBlueprintPanel(healthX, newEntryY, 174, newEntryHeight,
                                Color(2, 7, 10, 112), cyan);
    font.drawText("NOVA ENTRADA", healthX + 10, newEntryY + 8, cyan, .9F);
    font.drawText("CONSULTAR TERMINAL", healthX + 10, newEntryY + 24, cyan, .9F);
  }

  if(state.droid.phase==Core::DroidPhase::ButtonRising)
    font.drawText("DISPOSITIVO DE LIBERACAO EMERGINDO",22,192,cyan,.85F);
  if(state.droid.phase==Core::DroidPhase::ButtonReady)
    font.drawText(state.droid.buttonNearby?"TRIANGULO: LIBERAR CAPSULA":"ATIVAR DISPOSITIVO NA SALA",22,192,cyan,.85F);
  if (!state.radio.popupActive) {
    // Center First-Person Reticle (Crosshair)
    const float cx = 256.0F;
    const float cy = 224.0F;
    const Color crosshairCol(0.0F, 235.0F, 255.0F, 110.0F);
    drawer_->drawRect(cx - 8.0F, cy - 1.0F, 5.0F, 2.0F, crosshairCol);
    drawer_->drawRect(cx + 3.0F, cy - 1.0F, 5.0F, 2.0F, crosshairCol);
    drawer_->drawRect(cx - 1.0F, cy - 8.0F, 2.0F, 5.0F, crosshairCol);
    drawer_->drawRect(cx - 1.0F, cy + 3.0F, 2.0F, 5.0F, crosshairCol);
  }

  // Compact Top Tactical Telemetry Bar
  drawer_->drawPanel(12.0F, 10.0F, 488.0F, 26.0F,
                     Color(8.0F, 12.0F, 18.0F, 92.0F),
                     Color(0.0F, 180.0F, 220.0F, 95.0F));

  const char* zoneLabel = "ZONA B // VIA DE ACESSO";
  if (state.player.playerPos.z > 13.0F) {
    zoneLabel = "ZONA A // PONTO INICIAL";
  } else if (state.player.playerPos.z < 6.5F) {
    zoneLabel = "ZONA C // PORTAO & CONTROLE ETIP";
  }

  if (state.scene.active == Core::SceneId::Interior) zoneLabel = "ETIP // INTERIOR";
  drawer_->getFont().drawText(zoneLabel, 22, 16,
                              Color(240.0F, 185.0F, 40.0F, 128.0F), 1.0F);

  char posBuf[48];
  std::snprintf(posBuf, sizeof(posBuf), "POS X:%.1f Z:%.1f",
                state.player.playerPos.x, -state.player.playerPos.z);
  drawer_->getFont().drawText(posBuf, 350, 16,
                              Color(140.0F, 235.0F, 255.0F, 128.0F), 1.0F);

  const char* saveText = nullptr;
  switch (state.menu.saveNotice) {
    case Core::SaveNotice::Writing: saveText = "SALVANDO..."; break;
    case Core::SaveNotice::Saved: saveText = "PROGRESSO SALVO"; break;
    case Core::SaveNotice::Full: saveText = "MEMORY CARD CHEIO"; break;
    case Core::SaveNotice::Failed: saveText = "CHECKPOINT NAO SALVO"; break;
    case Core::SaveNotice::Disabled: saveText = "AUTOSAVE INDISPONIVEL"; break;
    default: break;
  }
  if (saveText) {
    const int width = font.lineWidth(saveText) + 16;
    drawer_->drawPanel(500 - width, 42, width, 20, Color(8, 16, 22, 112), cyan);
    font.drawText(saveText, 508 - width, 46, Color(160, 240, 240, 128));
  }

  if (!state.radio.popupActive && state.nearbyPickup != Core::Pickup::Count &&
      state.simulationActive()) {
    const auto item = state.nearbyPickup;
    const bool medkit = item == Core::Pickup::RearMedkit || item == Core::Pickup::SideRoomMedkit;
    const char* prompt = medkit && state.inventory.medkits >= Core::kMedkitCapacity
        ? "KIT MEDICO // INVENTARIO CHEIO" : Core::kPickups[static_cast<int>(item)].prompt;
    drawer_->drawPanel(healthX, promptY, promptWidth, promptHeight,
                       Color(8, 16, 22, 112), cyan);
    font.drawText(prompt, healthX + 12, promptY + 6, cyan, .9F);
  } else if (state.cardReader.isPlayerNearby && !state.radio.popupActive) {
    if (state.cardReader.isUnlocked) {
      drawer_->drawPanel(12, promptY, 356, 26,
                         Color(8.0F, 26.0F, 14.0F, 102.0F),
                         Color(30.0F, 230.0F, 80.0F, 115.0F));
      drawer_->getFont().drawText("[ACESSO LIBERADO - LUZ VERDE ATIVA]", 24,
                                  promptY + 6, Color(80.0F, 250.0F, 120.0F, 128.0F),
                                  1.0F);
    } else {
      drawer_->drawPanel(12, promptY, 356, 26,
                         Color(28.0F, 10.0F, 12.0F, 102.0F),
                         Color(235.0F, 50.0F, 50.0F, 115.0F));
      const char* prompt = "[QUADRADO] USAR CARTAO";
      if (!state.inventory.has(Core::Pickup::AccessCard)) {
        prompt = state.inventory.armed() ? "ACESSO BLOQUEADO // PEGUE O CARTAO"
                                         : "ACESSO BLOQUEADO // PEGUE A ARMA E O CARTAO";
      } else if (!state.inventory.armed()) {
        prompt = "ACESSO BLOQUEADO // PEGUE A ARMA";
      }
      drawer_->getFont().drawText(
          prompt, 24, promptY + 6,
          Color(255.0F, 140.0F, 140.0F, 128.0F), .85F);
    }
  }

  const bool showDoorPrompt = state.scene.nearbyDoor >= 0 &&
                              !state.radio.popupActive &&
                              state.nearbyPickup == Core::Pickup::Count;
  const auto drawInteraction = [&](const char* label, int y) {
    drawer_->drawPanel(healthX, y, promptWidth, promptHeight,
                       Color(8, 16, 22, 112), Color(0, 190, 210, 128));
    font.drawText(label, healthX + 12, y + 6, Color(160, 240, 240, 128), .9F);
  };
  if (state.scene.terminalNearby && !state.radio.popupActive) {
    const bool lowerPrompt = showDoorPrompt || state.cardReader.isPlayerNearby ||
        (state.nearbyPickup != Core::Pickup::Count && state.simulationActive());
    const int terminalY = lowerPrompt ? promptY - promptHeight - promptGap : promptY;
    drawInteraction("[TRIANGULO] INTERAGIR TERMINAL", terminalY);
  }

  if (!state.radio.popupActive && state.inventory.armed()) {
    const auto& w = state.weapon;
    const Color cyan(40, 120, 128, 128), dim(18, 48, 54, 128);
    const Color ink = w.ammo ? cyan : Color(128, 70, 30, 128);
    // Match the top and bottom edges of the vitals panels.
    drawer_->drawBlueprintPanel(380, statusY, 120, statusHeight,
                                Color(2, 7, 10, 120), cyan);
    drawer_->getFont().drawText("LASER", 390, statusY + 3, cyan, 1.0F);
    drawer_->getFont().drawText("INF", 470, statusY + 3, cyan, 1.0F);
    char ammo[16];
    std::snprintf(ammo, sizeof(ammo), "%02d / 08", w.ammo);
    drawer_->getFont().drawText(ammo, 390, statusY + 15, ink, 1.0F);
    for (int i = 0; i < Core::kMagazineSize; ++i)
      drawer_->drawRect(390 + i * 12, statusY + 26, 9, 3, i < w.ammo ? cyan : dim);
    if (w.reloadTicks) {
      drawer_->getFont().drawText("RECARREGANDO", 390, statusY + 31, cyan, 1.0F);
      drawer_->drawRect(390, statusBottom - 2, 96, 1, dim);
      drawer_->drawRect(
          390, statusBottom - 2,
          96 * (1.0F - static_cast<float>(w.reloadTicks) / Core::kReloadTicks),
          1, cyan);
    } else if (w.ammo == 0)
      drawer_->getFont().drawText("O: RECARREGAR", 390, statusY + 31, ink, 1.0F);
  }

  if (showDoorPrompt) {
    const auto& door = state.scene.doors[state.scene.nearbyDoor];
    drawInteraction(door.targetOpen ? "[QUADRADO] FECHAR PORTA"
                                    : "[QUADRADO] ABRIR PORTA", promptY);
  }
  // Bottom Controls Bar
  drawer_->drawPanel(12.0F, 412.0F, 488.0F, 24.0F,
                     Color(8.0F, 12.0F, 18.0F, 92.0F),
                     Color(80.0F, 100.0F, 120.0F, 85.0F));
  if (state.radio.popupActive) {
    drawer_->getFont().drawText(state.radio.phase == Core::RadioPhase::Message
                                    ? (state.radio.reviewing
                                           ? "L2: FECHAR RADIO // O: RECARGA // START: PAUSA"
                                           : "O: CLOSE RADIO // START: PAUSA")
                                    : "RADIO // INCOMING TRANSMISSION...",
                                20, 417, Color(80, 120, 116, 128), 1.0F);
  } else {
    drawer_->getFont().drawText(
        state.inventory.armed()
            ? "R1: TIRO  O: RECARGA  R2: KIT  X: PULO  L1: CORRER  L2: RADIO  START: PAUSA"
            : "QUADRADO: PEGAR  R2: KIT  X: PULO  L1: CORRER  L2: RADIO  START: PAUSA",
        20, 417, Color(190.0F, 215.0F, 230.0F, 128.0F), .85F);
  }
}

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
