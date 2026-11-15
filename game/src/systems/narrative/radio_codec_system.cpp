#include "systems/narrative/radio_codec_system.hpp"
#include <cmath>
#include <cstring>

namespace DroidCampos {
namespace Systems {
namespace Narrative {

using namespace Tyra;

RadioCodecSystem::RadioCodecSystem(Ui::UiContextDrawer* drawer)
    : drawer_(drawer), currentAnimTick_(0.0F) {}

RadioCodecSystem::RadioCodecSystem(Ui::UiContextDrawer& drawer)
    : drawer_(&drawer), currentAnimTick_(0.0F) {}

void RadioCodecSystem::init(Engine* engine, Core::WorldState& state,
                            Events::EventBus& bus) {
  (void)bus;
  if (drawer_ != nullptr) {
    drawer_->init(engine);
  }
  if (state.radio.currentMessage == nullptr) {
    state.radio.currentMessage = Core::kDefaultCodecMessage;
  }
}

void RadioCodecSystem::executeStage(Core::HardwareStage stage, Engine* engine,
                                    Core::WorldState& state,
                                    Events::EventBus& bus,
                                    Hardware::Ps2ScratchpadArena& scratchpad) {
  (void)engine;
  (void)scratchpad;

  if (stage == Core::HardwareStage::Stage1_EeVu0Simulation) {
    updateSimulation(state, bus);
  } else if (stage == Core::HardwareStage::Stage3_GifPath3Render2D) {
    if (state.menu.currentScreen == Core::ScreenState::GameplayFps &&
        state.radio.popupActive && drawer_ != nullptr) {
      currentAnimTick_ = state.anim.animTick;
      renderRadioPopup(state);
    }
  }
}

void RadioCodecSystem::shutdown(Engine* engine) { (void)engine; }

void RadioCodecSystem::updateSimulation(Core::WorldState& state,
                                        Events::EventBus& bus) {
  u32 eventCount = 0;
  const Events::GameEvent* events = bus.getFrameEvents(eventCount);
  for (u32 i = 0; i < eventCount; ++i) {
    const auto& ev = events[i];
    if (ev.type == Events::EventType::RadioDismissRequested) {
      state.radio.popupActive = false;
    } else if (ev.type == Events::EventType::RadioTransmissionTriggered) {
      state.radio.popupActive = true;
      state.radio.visibleChars = 0;
      state.radio.typeTimer = 0.0F;
      if (ev.textPayload != nullptr) {
        state.radio.currentMessage = ev.textPayload;
      } else if (state.radio.currentMessage == nullptr) {
        state.radio.currentMessage = Core::kDefaultCodecMessage;
      }
    }
  }

  if (state.menu.currentScreen == Core::ScreenState::GameplayFps &&
      state.radio.popupActive) {
    const char* msg = (state.radio.currentMessage != nullptr)
                          ? state.radio.currentMessage
                          : Core::kDefaultCodecMessage;
    const int totalLen = static_cast<int>(std::strlen(msg));
    if (state.radio.visibleChars < totalLen) {
      state.radio.typeTimer += 0.65F;
      if (state.radio.typeTimer >= 1.0F) {
        int step = static_cast<int>(state.radio.typeTimer);
        state.radio.typeTimer -= static_cast<float>(step);
        state.radio.visibleChars += step;
        if (state.radio.visibleChars > totalLen) {
          state.radio.visibleChars = totalLen;
        }
      }
    }
  }
}

void RadioCodecSystem::renderAnonymousCodecPortrait(float x, float y,
                                                    bool speaking) {
  if (drawer_ == nullptr) {
    return;
  }

  // Metal Gear Solid-style Codec Portrait Box on the right side
  drawer_->drawPanel(x, y, 98.0F, 114.0F, Color(6.0F, 18.0F, 14.0F, 122.0F),
                     Color(40.0F, 210.0F, 145.0F, 125.0F));

  // Inner dark green/cyan CRT screen background
  drawer_->drawRect(x + 4.0F, y + 4.0F, 90.0F, 90.0F,
                    Color(8.0F, 26.0F, 20.0F, 128.0F));

  const float cx = x + 49.0F;
  const float cy = y + 48.0F;

  // Anonymous Shadowed Silhouette (shoulders, high tactical collar, hooded head)
  // Shoulders & tactical coat collar
  drawer_->drawRect(cx - 36.0F, cy + 20.0F, 72.0F, 24.0F,
                    Color(16.0F, 54.0F, 42.0F, 128.0F));
  drawer_->drawRect(cx - 26.0F, cy + 10.0F, 52.0F, 14.0F,
                    Color(22.0F, 72.0F, 56.0F, 128.0F));

  // Outer hood / shadowy head contour
  drawer_->drawRect(cx - 22.0F, cy - 28.0F, 44.0F, 40.0F,
                    Color(26.0F, 86.0F, 68.0F, 128.0F));
  drawer_->drawRect(cx - 18.0F, cy - 32.0F, 36.0F, 6.0F,
                    Color(32.0F, 102.0F, 80.0F, 128.0F));

  // Deep shadow obscuring the face (anonymous informant aesthetic)
  drawer_->drawRect(cx - 16.0F, cy - 22.0F, 32.0F, 28.0F,
                    Color(6.0F, 18.0F, 14.0F, 128.0F));

  // Subtle glinting encrypted eyes / visor slits in the shadow
  drawer_->drawRect(cx - 11.0F, cy - 12.0F, 8.0F, 3.0F,
                    Color(90.0F, 245.0F, 185.0F, 128.0F));
  drawer_->drawRect(cx + 3.0F, cy - 12.0F, 8.0F, 3.0F,
                    Color(90.0F, 245.0F, 185.0F, 128.0F));

  // Nose bridge & lower jaw shadow highlight
  drawer_->drawRect(cx - 1.0F, cy - 9.0F, 2.0F, 7.0F,
                    Color(22.0F, 75.0F, 58.0F, 128.0F));

  // Animated voice waveform / mouth transmission bar while text is printing
  if (speaking && (static_cast<int>(currentAnimTick_ * 4.0F) % 2 == 0)) {
    drawer_->drawRect(cx - 7.0F, cy + 1.0F, 14.0F, 3.0F,
                      Color(70.0F, 230.0F, 165.0F, 128.0F));
  } else {
    drawer_->drawRect(cx - 5.0F, cy + 2.0F, 10.0F, 1.0F,
                      Color(28.0F, 95.0F, 72.0F, 128.0F));
  }

  // CRT Codec Scanlines across portrait
  for (int sl = 0; sl < 9; sl++) {
    drawer_->drawRect(x + 4.0F, y + 8.0F + static_cast<float>(sl) * 10.0F,
                      90.0F, 1.0F, Color(0.0F, 10.0F, 8.0F, 55.0F));
  }

  // Rolling interference bar inside portrait
  const float glitchY = y + 6.0F + std::fmod(currentAnimTick_ * 9.0F, 84.0F);
  drawer_->drawRect(x + 4.0F, glitchY, 90.0F, 2.0F,
                    Color(90.0F, 255.0F, 190.0F, 36.0F));

  // Bottom label inside portrait frame: "UNKNOWN"
  drawer_->getFont().drawText("UNKNOWN", static_cast<int>(x + 16.0F),
                              static_cast<int>(y + 96.0F),
                              Color(80.0F, 240.0F, 175.0F, 128.0F), 0.9F);
}

void RadioCodecSystem::renderRadioPopup(const Core::WorldState& state) {
  if (drawer_ == nullptr) {
    return;
  }

  // Main Metal Gear-style Radio / Codec Popup Window
  const float px = 18.0F;
  const float py = 266.0F;
  const float pw = 476.0F;
  const float ph = 136.0F;

  drawer_->drawPanel(px, py, pw, ph, Color(6.0F, 16.0F, 14.0F, 116.0F),
                     Color(45.0F, 215.0F, 150.0F, 122.0F));

  // Header strip (PTT / Frequency 140.85)
  drawer_->drawRect(px + 8.0F, py + 6.0F, 356.0F, 18.0F,
                    Color(12.0F, 38.0F, 30.0F, 120.0F));
  drawer_->getFont().drawText("PTT // FREQ 140.85   [ENCRYPTED CODEC]",
                              static_cast<int>(px + 14.0F),
                              static_cast<int>(py + 8.0F),
                              Color(90.0F, 245.0F, 180.0F, 128.0F), 0.95F);

  // Animated signal strength bars
  for (int b = 0; b < 5; b++) {
    const float bh = 3.0F + static_cast<float>(b) * 2.5F;
    const float bx = px + 328.0F + static_cast<float>(b) * 6.0F;
    const float by = py + 20.0F - bh;
    drawer_->drawRect(bx, by, 4.0F, bh, Color(80.0F, 245.0F, 175.0F, 128.0F));
  }

  const char* msg = (state.radio.currentMessage != nullptr)
                        ? state.radio.currentMessage
                        : Core::kDefaultCodecMessage;

  // Typewriter text area on the left (x = 28..370)
  char partialBuf[256];
  int copyLen = state.radio.visibleChars;
  if (copyLen < 0) {
    copyLen = 0;
  }
  const int totalLen = static_cast<int>(std::strlen(msg));
  if (copyLen > totalLen) {
    copyLen = totalLen;
  }
  if (copyLen > static_cast<int>(sizeof(partialBuf)) - 1) {
    copyLen = static_cast<int>(sizeof(partialBuf)) - 1;
  }
  std::memcpy(partialBuf, msg, static_cast<size_t>(copyLen));
  partialBuf[copyLen] = '\0';

  drawer_->getFont().drawText(partialBuf, static_cast<int>(px + 14.0F),
                              static_cast<int>(py + 32.0F),
                              Color(205.0F, 255.0F, 225.0F, 128.0F), 1.0F);

  const bool stillTyping = (state.radio.visibleChars < totalLen);

  // Blinking prompt to dismiss radio with 'X'
  const float pulse = (std::sin(state.anim.animTick * 4.5F) + 1.0F) * 0.5F;
  drawer_->drawRect(px + 10.0F, py + 110.0F, 354.0F, 18.0F,
                    Color(10.0F, 32.0F, 26.0F, 110.0F));
  drawer_->getFont().drawText(
      "[ PRESS 'X' TO CLOSE RADIO TRANSMISSION ]",
      static_cast<int>(px + 22.0F), static_cast<int>(py + 112.0F),
      Color(120.0F + pulse * 135.0F, 245.0F, 175.0F, 128.0F), 0.95F);

  // Fixed Anonymous Portrait on the right corner of the radio popup
  renderAnonymousCodecPortrait(px + 368.0F, py + 11.0F, stillTyping);
}

}  // namespace Narrative
}  // namespace Systems
}  // namespace DroidCampos
