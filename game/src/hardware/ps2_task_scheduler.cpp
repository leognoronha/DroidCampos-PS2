#include "hardware/ps2_task_scheduler.hpp"
#include <cstring>

namespace DroidCampos {
namespace Hardware {

Ps2TaskScheduler::Ps2TaskScheduler() : count_(0U) {
  std::memset(subsystems_, 0, sizeof(subsystems_));
}

void Ps2TaskScheduler::registerSubsystem(Core::ISubsystem* subsystem) {
  if (subsystem == nullptr || count_ >= kMaxSubsystems) {
    return;
  }
  subsystems_[count_] = subsystem;
  ++count_;
}

void Ps2TaskScheduler::initAll(Tyra::Engine* engine, Core::WorldState& state,
                               Events::EventBus& bus) {
  for (u32 i = 0U; i < count_; ++i) {
    if (subsystems_[i] != nullptr) {
      subsystems_[i]->init(engine, state, bus);
    }
  }
}

void Ps2TaskScheduler::runStage(Core::HardwareStage stage,
                                Tyra::Engine* engine, Core::WorldState& state,
                                Events::EventBus& bus,
                                Ps2ScratchpadArena& scratchpad) {
  for (u32 i = 0U; i < count_; ++i) {
    if (subsystems_[i] != nullptr) {
      subsystems_[i]->executeStage(stage, engine, state, bus, scratchpad);
    }
  }
}

void Ps2TaskScheduler::yieldToIopAudioThread() {
  Tyra::Threading::switchThread();
}

void Ps2TaskScheduler::shutdownAll(Tyra::Engine* engine) {
  for (u32 i = count_; i > 0U; --i) {
    Core::ISubsystem* sub = subsystems_[i - 1U];
    if (sub != nullptr) {
      sub->shutdown(engine);
    }
  }
  count_ = 0U;
}

}  // namespace Hardware
}  // namespace DroidCampos
