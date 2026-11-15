#pragma once

#include <tyra>
#include "events/game_events.hpp"

namespace DroidCampos {
namespace Events {

constexpr u32 kMaxQueueCapacity = 64;

class alignas(64) EventBus {
 public:
  EventBus();
  ~EventBus() = default;

  void clear();
  bool publish(const GameEvent& ev);
  bool poll(GameEvent& outEv);
  u32 size() const;

  const GameEvent* getFrameEvents(u32& outCount) const;
  void flushFrameEvents();

 private:
  alignas(64) GameEvent ringBuffer_[kMaxQueueCapacity];
  alignas(64) GameEvent frameEvents_[kMaxQueueCapacity];
  u32 head_;
  u32 tail_;
  u32 count_;
  u32 frameCount_;
};

}  // namespace Events
}  // namespace DroidCampos
