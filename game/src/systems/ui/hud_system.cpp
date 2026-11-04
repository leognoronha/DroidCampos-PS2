#include "systems/ui/hud_system.hpp"
#include <cstdio>

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
  const bool newEntry=state.encounter.newEntryUnlocked &&
      terminal.terminalTopic==2 && terminal.terminalEntry==1;
  const char* newSummary="NOVA ENTRADA DETECTADA\n\nPROTOCOLO DE CONTENCAO.\n\nLEIA PARA LIBERAR O DISPOSITIVO.\n\nX: ABRIR NOVO REGISTRO\nESQ/DIR: TROCAR REGISTRO";
  const char* newPages[2]={
      "[PROTOCOLO DE LIBERACAO]\n\nSEGURANCA DESATIVADA.\nAO ENCERRAR ESTE TERMINAL,\nUM DISPOSITIVO VERMELHO\nSURGIRA NO CENTRO DA SALA.",
      "[ATIVACAO MANUAL]\n\nAPROXIME-SE DO BOTAO.\nMIRE E PRESSIONE TRIANGULO\nPARA LIBERAR A CAPSULA.\n\nCONTENCAO IRREVERSIVEL."};
  // Short, authored lines fit the PS2 font without runtime text allocation.
  static const char* topics[] = {"CORPORATIVO", "HISTORICO", "OCORRENCIAS"};
  static const char* summaries[] = {
      "COMUNICADOS E PROTOCOLOS\n\nRELATORIOS OFICIAIS DA ETIP.\n\nCONTEUDO PROVISORIO.\n\nX: ABRIR DOCUMENTO",
      "MEMORIA DA INSTALACAO\n\nREGISTROS DE EVENTOS\nANTERIORES A SUA CHEGADA.\n\nCONTEUDO PROVISORIO.\n\nX: ABRIR DOCUMENTO",
      "REGISTROS DE INCIDENTES\n\nRELATOS DE FUNCIONARIOS\nE ANOMALIAS OBSERVADAS.\n\nCONTEUDO PROVISORIO.\n\nX: ABRIR DOCUMENTO"};
  static const char* pages[3][2] = {
      {"[PLACEHOLDER 01]\n\nESPACO PARA UM RELATORIO\nCORPORATIVO DA ETIP.\n\nAPRESENTARA OS OBJETIVOS,\nAS ROTINAS E OS PROTOCOLOS\nOFICIAIS DA INSTALACAO.",
       "[CONTINUACAO / RASCUNHO]\n\nRESERVADO PARA DIRETRIZES\nINTERNAS E OBSERVACOES\nDA ADMINISTRACAO.\n\nTEXTO NARRATIVO PENDENTE."},
      {"[PLACEHOLDER 02]\n\nESPACO PARA O HISTORICO\nDA INSTALACAO.\n\nREGISTRARA MARCOS, DATAS\nE ACONTECIMENTOS ANTES\nDA CHEGADA DO PERSONAGEM.",
       "[CONTINUACAO / RASCUNHO]\n\nRESERVADO PARA UMA\nCRONOLOGIA DOS EVENTOS\nE SUAS CONSEQUENCIAS.\n\nTEXTO NARRATIVO PENDENTE."},
      {"[PLACEHOLDER 03]\n\nESPACO PARA RELATOS\nDE FUNCIONARIOS.\n\nREGISTRARA INCIDENTES,\nOBSERVACOES E EXPERIENCIAS\nDENTRO DA INSTALACAO.",
       "[CONTINUACAO / RASCUNHO]\n\nRESERVADO PARA TESTEMUNHOS\nQUE COMPLEMENTEM OU\nQUESTIONEM A VERSAO\nOFICIAL DA ETIP.\n\nTEXTO NARRATIVO PENDENTE."}};

  drawer_->drawRect(0, 0, 512, 448, black);
  drawer_->drawBlueprintPanel(16, 20, 480, 66, black, cyan);
  font.drawText("ETIP Corporation.", 30, 32, cyan, 1.25F);
  font.drawText("ARQUIVO INTERNO // TERMINAL 01", 30, 62, paper, .9F);
  drawer_->drawBlueprintPanel(16, 98, 156, 294, black, dim);
  drawer_->drawBlueprintPanel(182, 98, 314, 294, black, cyan);
  font.drawText("ACESSO LOCAL", 28, 112, cyan, .9F);
  for (int i = 0; i < 3; ++i) {
    const int y = 148 + i * 66;
    const bool selected = terminal.terminalTopic == i;
    drawer_->drawBlueprintPanel(24, y, 140, 54,
                                selected ? cyan : black, selected ? cyan : dim);
    char label[32];
    std::snprintf(label, sizeof(label), "0%d / ARQUIVO", i + 1);
    font.drawText(label, 34, y + 7, selected ? black : dim, .8F);
    font.drawText(i==2 && state.encounter.newEntryUnlocked ? "OCORRENCIAS *" : topics[i],
                  34,y+28,selected?black:cyan,.85F);
  }
  font.drawText("USO INTERNO ETIP", 28, 366, dim, .75F);
  if (terminal.terminalBootFrames > 0) {
    font.drawText("ESTABELECENDO ACESSO...", 198, 160, cyan, .9F);
    drawer_->drawRect(198, 198, 276, 4, dim);
    drawer_->drawRect(198, 198, 276.0F * (24-terminal.terminalBootFrames)/24, 4, cyan);
  } else {
    font.drawText(topics[terminal.terminalTopic], 198, 112, cyan, 1.0F);
    font.drawText("DATA: --/--/--  AUTOR: PENDENTE", 198, 136, dim, .8F);
    drawer_->drawRect(198, 156, 282, 1, dim);
    font.drawText(terminal.terminalReading
        ? (newEntry ? newPages[terminal.terminalPage] : pages[terminal.terminalTopic][terminal.terminalPage])
        : (newEntry ? newSummary : summaries[terminal.terminalTopic]), 198, 174, paper, .9F);
    char page[48];
    std::snprintf(page, sizeof(page), "DOC 0%d // %s", terminal.terminalTopic + 1,
        terminal.terminalReading ? (terminal.terminalPage ? "PAGINA 2/2" : "PAGINA 1/2")
                                : (newEntry ? "NOVO REGISTRO" : "PREVIA"));
    font.drawText(page, 198, 366, cyan, .85F);
  }
  drawer_->drawBlueprintPanel(16, 404, 480, 28, black, dim);
  font.drawText(terminal.terminalReading
      ? "ESQ/DIR: PAGINA   O: VOLTAR   TRIANGULO: SAIR"
      : (terminal.terminalTopic==2 && state.encounter.newEntryUnlocked
          ? "ESQ/DIR: REGISTRO  X: ABRIR  O / TRIANGULO: SAIR"
          : "CIMA/BAIXO: TOPICO   X: ABRIR   O / TRIANGULO: SAIR"),
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
  // Keep telemetry below the incoming-radio icon and above contextual prompts.
  drawer_->drawBlueprintPanel(12,100,174,42,Color(2,7,10,112),cyan);
  char health[24];std::snprintf(health,sizeof(health),"VIDA %03d / 100",state.playerHealth);
  const Color life=state.playerHealth<25?Color(128,35,18,128):cyan;
  font.drawText(health,22,107,life,.9F);
  drawer_->drawRect(22,129,152,4,dim);
  if(state.playerHealth>0)drawer_->drawRect(22,129,152.F*state.playerHealth/Core::kPlayerHealth,4,life);
  const auto& encounter=state.encounter;
  char wave[64]{};
  if(encounter.phase==Core::EncounterPhase::Countdown)
    std::snprintf(wave,sizeof(wave),"ALERTA // ONDA 1 EM %u",(encounter.countdownTicks+15624)/15625);
  else if(encounter.phase==Core::EncounterPhase::Wave)
    std::snprintf(wave,sizeof(wave),"ONDA %d/3 // RESTAM %d",encounter.wave,encounter.remaining);
  else if(encounter.phase==Core::EncounterPhase::Intermission)
    std::snprintf(wave,sizeof(wave),"ONDA %d/3 EM %u // VIDA +25",encounter.wave+1,(encounter.countdownTicks+15624)/15625);
  else if(encounter.phase==Core::EncounterPhase::Complete)
    std::snprintf(wave,sizeof(wave),state.droid.phase>=Core::DroidPhase::Release?"DROIDCAMPOS // CONTENCAO ROMPIDA":"AMEACA ELIMINADA // 3/3");
  if(wave[0]) {
    drawer_->drawBlueprintPanel(246,52,254,28,Color(2,7,10,112),cyan);
    font.drawText(wave,254,60,cyan,.85F);
  }
  if(encounter.newEntryUnlocked && !encounter.objectiveComplete && !state.radio.popupActive) {
    drawer_->drawBlueprintPanel(12,152,174,42,Color(2,7,10,112),cyan);
    font.drawText("NOVA ENTRADA",22,160,cyan,.9F);
    font.drawText("CONSULTAR TERMINAL",22,176,cyan,.9F);
  }

  if(state.droid.phase==Core::DroidPhase::ButtonRising)
    font.drawText("DISPOSITIVO DE LIBERACAO EMERGINDO",22,192,cyan,.85F);
  if(state.droid.phase==Core::DroidPhase::ButtonReady)
    font.drawText(state.droid.buttonNearby?"TRIANGULO: LIBERAR CAPSULA":"ATIVAR DISPOSITIVO NA SALA",22,192,cyan,.85F);
  // Center First-Person Reticle (Crosshair)
  const float cx = 256.0F;
  const float cy = 224.0F;
  const Color crosshairCol(0.0F, 235.0F, 255.0F, 110.0F);
  drawer_->drawRect(cx - 8.0F, cy - 1.0F, 5.0F, 2.0F, crosshairCol);
  drawer_->drawRect(cx + 3.0F, cy - 1.0F, 5.0F, 2.0F, crosshairCol);
  drawer_->drawRect(cx - 1.0F, cy - 8.0F, 2.0F, 5.0F, crosshairCol);
  drawer_->drawRect(cx - 1.0F, cy + 3.0F, 2.0F, 5.0F, crosshairCol);

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

  // Contextual Card Reader Interaction Prompt
  if (state.cardReader.isPlayerNearby && !state.radio.popupActive) {
    if (state.cardReader.isUnlocked) {
      drawer_->drawPanel(12.0F, 376.0F, 356.0F, 26.0F,
                         Color(8.0F, 26.0F, 14.0F, 102.0F),
                         Color(30.0F, 230.0F, 80.0F, 115.0F));
      drawer_->getFont().drawText("[ACESSO LIBERADO - LUZ VERDE ATIVA]", 24,
                                  382, Color(80.0F, 250.0F, 120.0F, 128.0F),
                                  1.0F);
    } else {
      drawer_->drawPanel(12.0F, 376.0F, 356.0F, 26.0F,
                         Color(28.0F, 10.0F, 12.0F, 102.0F),
                         Color(235.0F, 50.0F, 50.0F, 115.0F));
      drawer_->getFont().drawText(
          "[QUADRADO] INTERAGIR LEITOR (LUZ VERMELHA)", 24, 382,
          Color(255.0F, 140.0F, 140.0F, 128.0F), .85F);
    }
  }

  if (state.scene.terminalNearby && !state.radio.popupActive) {
    drawer_->drawPanel(12, 344, 356, 28, Color(8, 16, 22, 112), Color(0, 190, 210, 128));
    drawer_->getFont().drawText(state.scene.terminalActive
        ? "TERMINAL ETIP // CONECTADO" : "[TRIANGULO] INTERAGIR TERMINAL",
        24, 352, Color(160, 240, 240, 128));
    if (state.scene.terminalActive)
      drawer_->getFont().drawText("[TRIANGULO] FECHAR TERMINAL", 90, 378,
                                  Color(160, 240, 240, 128));
  }

  if (!state.radio.popupActive) {
    const auto& w = state.weapon;
    const Color cyan(40, 120, 128, 128), dim(18, 48, 54, 128);
    const Color ink = w.ammo ? cyan : Color(128, 70, 30, 128);
    drawer_->drawBlueprintPanel(380, 344, 120, 60, Color(2, 7, 10, 120), cyan);
    drawer_->getFont().drawText("LASER", 390, 347, cyan, 1.0F);
    drawer_->getFont().drawText("INF", 470, 347, cyan, 1.0F);
    char ammo[16];
    std::snprintf(ammo, sizeof(ammo), "%02d / 08", w.ammo);
    drawer_->getFont().drawText(ammo, 390, 363, ink, 1.0F);
    for (int i = 0; i < Core::kMagazineSize; ++i)
      drawer_->drawRect(390 + i * 12, 381, 9, 3, i < w.ammo ? cyan : dim);
    if (w.reloadTicks) {
      drawer_->getFont().drawText("RECARREGANDO", 390, 385, cyan, 1.0F);
      drawer_->drawRect(390, 402, 96, 1, dim);
      drawer_->drawRect(
          390, 402,
          96 * (1.0F - static_cast<float>(w.reloadTicks) / Core::kReloadTicks),
          1, cyan);
    } else if (w.ammo == 0)
      drawer_->getFont().drawText("O: RECARREGAR", 390, 385, ink, 1.0F);
  }

  // Bottom Controls Bar
  if (state.scene.nearbyDoor >= 0 && !state.radio.popupActive) {
    const auto& door = state.scene.doors[state.scene.nearbyDoor];
    drawer_->drawPanel(12, 376, 356, 26, Color(8, 16, 22, 112),
                       Color(0, 190, 210, 128));
    drawer_->getFont().drawText(
        door.targetOpen ? "[QUADRADO] FECHAR PORTA" : "[QUADRADO] ABRIR PORTA",
        24, 382, Color(160, 240, 240, 128));
  }
  drawer_->drawPanel(12.0F, 412.0F, 488.0F, 24.0F,
                     Color(8.0F, 12.0F, 18.0F, 92.0F),
                     Color(80.0F, 100.0F, 120.0F, 85.0F));
  if (state.radio.popupActive) {
    drawer_->getFont().drawText(state.radio.phase == Core::RadioPhase::Message
                                    ? "O: CLOSE RADIO // START: PAUSA"
                                    : "RADIO // INCOMING TRANSMISSION...",
                                20, 417, Color(80, 120, 116, 128), 1.0F);
  } else {
    drawer_->getFont().drawText(
        "R1: TIRO  O: RECARGA  X: PULO  L1: CORRER  L2: RADIO  START: PAUSA",
        20, 417, Color(190.0F, 215.0F, 230.0F, 128.0F), 0.95F);
  }
}

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
