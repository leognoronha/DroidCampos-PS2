#pragma once

#include <tyra>
#include "core/world_state.hpp"
#include "events/event_bus.hpp"
#include "hardware/ps2_scratchpad.hpp"

namespace DroidCampos {
namespace Core {

enum class HardwareStage : u8 {
  Stage0_IopInputPoll = 0,
  Stage1_EeVu0Simulation = 1,
  Stage2_Vu1GsRender3D = 2,
  Stage3_GifPath3Render2D = 3
};

class ISubsystem {
 public:
  virtual ~ISubsystem() = default;
  virtual const char* getName() const = 0;
  virtual void init(Tyra::Engine* engine, WorldState& state,
                    Events::EventBus& bus) = 0;
  virtual void executeStage(HardwareStage stage, Tyra::Engine* engine,
                            WorldState& state, Events::EventBus& bus,
                            Hardware::Ps2ScratchpadArena& scratchpad) = 0;
  virtual void shutdown(Tyra::Engine* engine) = 0;
};

}  // namespace Core
}  // namespace DroidCampos
