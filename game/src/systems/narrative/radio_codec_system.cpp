#include "systems/narrative/radio_codec_system.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace DroidCampos {
namespace Systems {
namespace Narrative {
using namespace Tyra;

RadioCodecSystem::RadioCodecSystem(Ui::UiContextDrawer* drawer)
    : drawer_(drawer), currentAnimTick_(0.0F) {}
RadioCodecSystem::RadioCodecSystem(Ui::UiContextDrawer& drawer)
    : drawer_(&drawer), currentAnimTick_(0.0F) {}

void RadioCodecSystem::init(Engine* engine, Core::WorldState& state, Events::EventBus&) {
  revealTimer_.prime();
  if (drawer_) drawer_->init(engine);
  if (!state.radio.currentMessage) state.radio.currentMessage = Core::kDefaultCodecMessage;
}

void RadioCodecSystem::executeStage(Core::HardwareStage stage, Engine*,
                                    Core::WorldState& state, Events::EventBus& bus,
                                    Hardware::Ps2ScratchpadArena&) {
  if (stage == Core::HardwareStage::Stage1_EeVu0Simulation) {
    updateSimulation(state, bus);
  } else if (stage == Core::HardwareStage::Stage3_GifPath3Render2D && drawer_ &&
             state.menu.currentScreen == Core::ScreenState::GameplayFps &&
             state.radio.popupActive && !state.intro.active && !state.scene.terminalActive) {
    currentAnimTick_ = state.anim.animTick;
    if (state.radio.phase == Core::RadioPhase::Alert) renderIncomingIndicator();
    else if (state.radio.phase == Core::RadioPhase::Message) renderRadioPopup(state);
  }
}

void RadioCodecSystem::shutdown(Engine*) {}

void RadioCodecSystem::updateSimulation(Core::WorldState& state, Events::EventBus& bus) {
  const u32 elapsed = revealTimer_.getTimeDelta();
  revealTimer_.prime();
  u32 count = 0;
  const auto* events = bus.getFrameEvents(count);
  for (u32 i = 0; i < count; ++i) {
    if (events[i].type == Events::EventType::RadioDismissRequested &&
        state.radio.phase == Core::RadioPhase::Message) {
      state.radio.popupActive = false;
      state.radio.reviewing = false;
      state.radio.phase = Core::RadioPhase::Idle;
    } else if (events[i].type == Events::EventType::RadioReviewRequested &&
               !state.radio.popupActive) {
      // Review the scene's retained message without restarting the call/audio.
      state.radio.popupActive = true;
      state.radio.reviewing = true;
      state.radio.phase = Core::RadioPhase::Message;
      if (!state.radio.currentMessage) state.radio.currentMessage = Core::kDefaultCodecMessage;
      state.radio.visibleChars = static_cast<int>(std::strlen(state.radio.currentMessage));
      state.radio.revealTicks = 0;
    } else if (events[i].type == Events::EventType::RadioTransmissionTriggered) {
      state.radio.popupActive = true;
      state.radio.reviewing = false;
      state.radio.phase = Core::RadioPhase::Waiting;
      state.radio.visibleChars = 0;
      state.radio.revealTicks = 0;
      if (events[i].textPayload) state.radio.currentMessage = events[i].textPayload;
      else if (!state.radio.currentMessage) state.radio.currentMessage = Core::kDefaultCodecMessage;
    }
  }
  if (state.menu.currentScreen != Core::ScreenState::GameplayFps ||
      state.intro.active || !state.radio.popupActive || state.radio.phase != Core::RadioPhase::Message) return;
  const char* message = state.radio.currentMessage ? state.radio.currentMessage : Core::kDefaultCodecMessage;
  const int length = static_cast<int>(std::strlen(message));
  // Tyra Timer ticks at 15625 Hz. Start each pause when its punctuation
  // becomes visible so a slow frame cannot swallow the pause.
  constexpr u32 characterTicks = 500;  // 32 ms
  constexpr u32 commaTicks = 2813;     // 180 ms
  constexpr u32 sentenceTicks = 5000;  // 320 ms
  constexpr u32 lineTicks = 7032;      // 450 ms
  auto& radio = state.radio;
  radio.revealTicks += std::min(elapsed, lineTicks);
  while (radio.visibleChars < length) {
    const char previous = radio.visibleChars > 0 ? message[radio.visibleChars - 1] : '\0';
    const bool nextLine = message[radio.visibleChars] == '\n';
    const u32 delay = nextLine ? lineTicks
        : (previous == '.' || previous == '!' || previous == '?') ? sentenceTicks
        : (previous == ',' || previous == ';' || previous == ':') ? commaTicks
        : characterTicks;
    if (radio.revealTicks < delay) break;
    radio.revealTicks -= delay;
    if (nextLine) ++radio.visibleChars;
    if (radio.visibleChars == length) break;
    const char revealed = message[radio.visibleChars++];
    if (revealed == '.' || revealed == '!' || revealed == '?' ||
        revealed == ',' || revealed == ';' || revealed == ':' ||
        message[radio.visibleChars] == '\n') {
      radio.revealTicks = 0;
      break;
    }
  }
}

void RadioCodecSystem::renderIncomingIndicator() {
  const float pulse = 0.65F + 0.35F * (std::sin(currentAnimTick_ * 5) + 1) * 0.5F;
  const Color cyan(80 * pulse, 120 * pulse, 116 * pulse, 128);
  drawer_->drawBlueprintPanel(18, 52, 32, 34, Color(0, 0, 0, 116), cyan);
  drawer_->drawRect(36, 57, 1, 9, cyan);  // Antenna.
  drawer_->drawRect(35, 56, 3, 2, cyan);
  drawer_->drawRect(24, 65, 20, 14, Color(0, 0, 0, 128));
  drawer_->drawRect(24, 64, 20, 1, cyan);
  drawer_->drawRect(23, 65, 1, 14, cyan);
  drawer_->drawRect(44, 65, 1, 14, cyan);
  drawer_->drawRect(24, 79, 20, 1, cyan);
  drawer_->drawRect(27, 68, 7, 3, cyan);  // Receiver display.
  for (int i = 0; i < 3; ++i) drawer_->drawRect(38, 67 + i * 3, 3, 1, cyan);
  drawer_->drawRect(26, 80, 3, 2, cyan);
  drawer_->drawRect(39, 80, 3, 2, cyan);
}

void RadioCodecSystem::renderSignalMonitor(float x, float y, bool speaking) {
  const Color cyan(80, 120, 116, 128);
  const Color dim(27, 56, 58, 128);
  drawer_->drawBlueprintPanel(x, y, 96, 112, Color(0, 0, 0, 128), dim);
  drawer_->getFont().drawText("RX 140.85", static_cast<int>(x + 12), static_cast<int>(y + 10), cyan);
  drawer_->drawRect(x + 8, y + 34, 80, 1, dim);
  drawer_->drawRect(x + 8, y + 67, 80, 1, Color(12, 24, 25, 128));
  for (int i = 0; i < 12; ++i) {
    const float height = speaking ? 4 + 26 * std::fabs(std::sin(currentAnimTick_ * 3 + i * 1.7F)) : 3;
    drawer_->drawRect(x + 10 + i * 6, y + 67 - height * 0.5F, 2, height, cyan);
  }
  drawer_->drawRect(x + 8, y + 90, 80, 1, dim);
  drawer_->getFont().drawText("UNKNOWN", static_cast<int>(x + 20), static_cast<int>(y + 97), cyan);
}

void RadioCodecSystem::renderRadioPopup(const Core::WorldState& state) {
  const Color cyan(80, 120, 116, 128);
  const Color dim(27, 56, 58, 128);
  const Color black(0, 0, 0, 128);
  // Same cut corners and pale-cyan drafting lines as the main menu.
  drawer_->drawBlueprintPanel(18, 226, 476, 176, black, cyan);
  drawer_->drawRect(28, 236, 302, 20, cyan);
  drawer_->getFont().drawText("ETIP // RADIO LINK", 36, 240, black);
  drawer_->getFont().drawText("DC-01 / SECURE", 370, 240, cyan);
  drawer_->drawRect(28, 262, 336, 1, dim);
  // Drafting ruler and separated signal frame leave the message area uncluttered.
  for (int i = 0; i < 8; ++i)
    drawer_->drawRect(286 + i * 6, 250, 3, 2, black);
  renderSignalMonitor(382, 280, state.radio.visibleChars < static_cast<int>(std::strlen(
      state.radio.currentMessage ? state.radio.currentMessage : Core::kDefaultCodecMessage)));

  const char* message = state.radio.currentMessage ? state.radio.currentMessage : Core::kDefaultCodecMessage;
  const int length = static_cast<int>(std::strlen(message));
  const int visible = std::max(0, std::min(length, state.radio.visibleChars));
  // The text area fits six lines. Follow the latest line in longer calls.
  int start = 0;
  int lines = 1;
  for (int i = visible - 1; i >= 0; --i) {
    if (message[i] == '\n' && ++lines > 6) {
      start = i + 1;
      break;
    }
  }
  drawer_->getFont().drawText(std::string(message + start, visible - start), 34, 269, cyan);
  drawer_->drawRect(28, 376, 336, 1, dim);
  drawer_->getFont().drawText(state.radio.reviewing
                                  ? "L2: FECHAR RADIO // ENCRYPTED CHANNEL"
                                  : "O: CLOSE // ENCRYPTED CHANNEL",
                              34, 383, cyan);
  // Short layered brackets reinforce the blueprint frame without a full grid.
  drawer_->drawRect(22, 244, 1, 134, dim);
  drawer_->drawRect(30, 398, 42, 1, dim);
}

}  // namespace Narrative
}  // namespace Systems
}  // namespace DroidCampos
