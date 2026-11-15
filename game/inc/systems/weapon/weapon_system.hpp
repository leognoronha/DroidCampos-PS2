#pragma once
#include "core/isubsystem.hpp"
#include "systems/weapon/weapon_geometry.hpp"
namespace DroidCampos {
namespace Systems {
namespace Weapon {
class WeaponSystem final : public Core::ISubsystem {
 public:
  explicit WeaponSystem(WeaponScene* scene) : scene_(scene) {}
  const char* getName() const override { return "WeaponSystem"; }
  void init(Tyra::Engine*, Core::WorldState&, Events::EventBus&) override;
  void executeStage(Core::HardwareStage, Tyra::Engine*, Core::WorldState&,
                    Events::EventBus&, Hardware::Ps2ScratchpadArena&) override;
  void shutdown(Tyra::Engine*) override {}

 private:
  WeaponScene* scene_;
  Tyra::Timer timer_;
  bool wasActive_ = false;
};
}  // namespace Weapon
}  // namespace Systems
}  // namespace DroidCampos
