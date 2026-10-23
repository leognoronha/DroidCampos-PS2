#include "events/event_bus.hpp"
#include <cstring>

namespace DroidCampos {
namespace Events {

EventBus::EventBus() : head_(0), tail_(0), count_(0), frameCount_(0) {
  clear();
}

void EventBus::clear() {
  head_ = 0;
  tail_ = 0;
  count_ = 0;
  frameCount_ = 0;
  std::memset(ringBuffer_, 0, sizeof(ringBuffer_));
  std::memset(frameEvents_, 0, sizeof(frameEvents_));
}

bool EventBus::publish(const GameEvent& ev) {
  bool pushedToRing = false;
  if (count_ < kMaxQueueCapacity) {
    ringBuffer_[tail_] = ev;
    tail_ = (tail_ + 1U) & (kMaxQueueCapacity - 1U);
    ++count_;
    pushedToRing = true;
  }

  if (frameCount_ < kMaxQueueCapacity) {
    frameEvents_[frameCount_] = ev;
    ++frameCount_;
  }

  return pushedToRing;
}

bool EventBus::poll(GameEvent& outEv) {
  if (count_ == 0U) {
    return false;
  }
  outEv = ringBuffer_[head_];
  head_ = (head_ + 1U) & (kMaxQueueCapacity - 1U);
  --count_;
  return true;
}

u32 EventBus::size() const { return count_; }

const GameEvent* EventBus::getFrameEvents(u32& outCount) const {
  outCount = frameCount_;
  return frameEvents_;
}

void EventBus::flushFrameEvents() {
  head_ = 0;
  tail_ = 0;
  count_ = 0;
  frameCount_ = 0;
}

}  // namespace Events
}  // namespace DroidCampos
