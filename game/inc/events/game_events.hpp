#pragma once

#include <tyra>

namespace DroidCampos {
namespace Events {

enum class EventType : u8 {
  None = 0,
  ScreenTransitionRequested = 1,
  PlayerSpawnResetRequested = 2,
  RadioDismissRequested = 3,
  RadioTransmissionTriggered = 4,
  AudioSettingChanged = 5,
  VideoFilterChanged = 6,
  CardReaderInteracted = 7
};

struct alignas(16) GameEvent {
  EventType type;
  u8 targetScreen;
  s16 intParam;
  float floatParam;
  const char* textPayload;

  static GameEvent makeScreenTransition(u8 screen) {
    GameEvent ev{};
    ev.type = EventType::ScreenTransitionRequested;
    ev.targetScreen = screen;
    ev.intParam = 0;
    ev.floatParam = 0.0F;
    ev.textPayload = nullptr;
    return ev;
  }

  static GameEvent makeSimple(EventType t, s16 iParam = 0,
                              float fParam = 0.0F,
                              const char* text = nullptr) {
    GameEvent ev{};
    ev.type = t;
    ev.targetScreen = 0;
    ev.intParam = iParam;
    ev.floatParam = fParam;
    ev.textPayload = text;
    return ev;
  }
};

}  // namespace Events
}  // namespace DroidCampos
