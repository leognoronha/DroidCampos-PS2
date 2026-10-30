#pragma once
#include "core/isubsystem.hpp"
#include "systems/enemy/interior_navigation.hpp"
#include "systems/weapon/combat_queries.hpp"

namespace DroidCampos { namespace Systems { namespace Enemy {
class EnemySystem final : public Core::ISubsystem {
 public:
  explicit EnemySystem(Weapon::WeaponScene* scene):scene_(scene){}
  const char* getName()const override{return "EnemySystem";}
  void init(Tyra::Engine*,Core::WorldState&,Events::EventBus&) override;
  void executeStage(Core::HardwareStage,Tyra::Engine*,Core::WorldState&,
                    Events::EventBus&,Hardware::Ps2ScratchpadArena&) override;
  void shutdown(Tyra::Engine*)override{}
  // Deterministic simulation seam, also used by the real scheduler.
  void advance(Core::WorldState&,Events::EventBus&,u32 elapsed);
  static void reset(Core::WorldState&);
 private:
  void decide(Core::WorldState&);
  void move(Core::WorldState&,int,float);
  void fire(Core::WorldState&,Events::EventBus&,int);
  void doors(Core::WorldState&,float);
  Weapon::WeaponScene* scene_;
  InteriorNavigation navigation_;
  Tyra::Timer timer_;
  bool wasActive_=false;
  bool navigationReady_=false;
  u32 decisionTicks_=0;
  std::array<std::array<int,InteriorNavigation::kCapacity>,Core::kMaxEnemies> paths_{};
  std::array<int,Core::kMaxEnemies> pathSizes_{},pathCursors_{};
};
}}}
