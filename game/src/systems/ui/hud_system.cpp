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
        drawer_ != nullptr) {
      renderFpsHud(state);
    }
  }
}

void HudSystem::shutdown(Engine* engine) { (void)engine; }

void HudSystem::renderFpsHud(const Core::WorldState& state) {
  if (drawer_ == nullptr) {
    return;
  }

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
      drawer_->drawPanel(96.0F, 376.0F, 320.0F, 26.0F,
                         Color(8.0F, 26.0F, 14.0F, 102.0F),
                         Color(30.0F, 230.0F, 80.0F, 115.0F));
      drawer_->getFont().drawText("[ACESSO LIBERADO - LUZ VERDE ATIVA]", 112,
                                  382, Color(80.0F, 250.0F, 120.0F, 128.0F),
                                  1.0F);
    } else {
      drawer_->drawPanel(76.0F, 376.0F, 360.0F, 26.0F,
                         Color(28.0F, 10.0F, 12.0F, 102.0F),
                         Color(235.0F, 50.0F, 50.0F, 115.0F));
      drawer_->getFont().drawText(
          "[QUADRADO] INTERAGIR LEITOR (LUZ VERMELHA)", 90, 382,
          Color(255.0F, 140.0F, 140.0F, 128.0F), 1.0F);
    }
  }

  // Bottom Controls Bar
  if (state.scene.nearbyDoor >= 0 && !state.radio.popupActive) {
    const auto& door = state.scene.doors[state.scene.nearbyDoor];
    drawer_->drawPanel(116, 376, 280, 26, Color(8, 16, 22, 112), Color(0, 190, 210, 128));
    drawer_->getFont().drawText(door.targetOpen ? "[QUADRADO] FECHAR PORTA" : "[QUADRADO] ABRIR PORTA",
                                132, 382, Color(160, 240, 240, 128));
  }
  drawer_->drawPanel(12.0F, 412.0F, 488.0F, 24.0F,
                     Color(8.0F, 12.0F, 18.0F, 92.0F),
                     Color(80.0F, 100.0F, 120.0F, 85.0F));
  if (state.radio.popupActive) {
    drawer_->getFont().drawText(
        "X: CLOSE RADIO   L-STICK/DPAD: WALK   R-STICK: LOOK   SELECT: MENU",
        20, 417, Color(140.0F, 245.0F, 190.0F, 128.0F), 0.95F);
  } else {
    drawer_->getFont().drawText(
        "L-STICK/DPAD: WALK   R-STICK/BTNS: LOOK   L2: RADIO   SELECT: MENU",
        20, 417, Color(190.0F, 215.0F, 230.0F, 128.0F), 0.95F);
  }
}

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
