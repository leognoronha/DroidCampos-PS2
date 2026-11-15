#pragma once

#include <tyra>
#include "core/isubsystem.hpp"
#include "core/world_state.hpp"
#include "events/event_bus.hpp"
#include "hardware/ps2_scratchpad.hpp"

namespace DroidCampos {
namespace Hardware {

constexpr u32 kMaxSubsystems = 16;

class alignas(64) Ps2TaskScheduler {
 public:
  Ps2TaskScheduler();
  ~Ps2TaskScheduler() = default;

  void registerSubsystem(Core::ISubsystem* subsystem);
  void initAll(Tyra::Engine* engine, Core::WorldState& state,
               Events::EventBus& bus);
  void runStage(Core::HardwareStage stage, Tyra::Engine* engine,
                Core::WorldState& state, Events::EventBus& bus,
                Ps2ScratchpadArena& scratchpad);
  void yieldToIopAudioThread();
  void shutdownAll(Tyra::Engine* engine);

  u32 getSubsystemCount() const { return count_; }

 private:
  Core::ISubsystem* subsystems_[kMaxSubsystems];
  u32 count_;
};

}  // namespace Hardware
}  // namespace DroidCampos
