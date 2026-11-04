#include <cassert>
#include <iostream>
#include "systems/weapon/weapon_system.hpp"
using namespace DroidCampos;
int main(){
 Core::WorldState s{};s.resetToDefaults();s.menu.currentScreen=Core::ScreenState::GameplayFps;
 s.scene.resident=s.scene.presented=true;s.radio.popupActive=true;s.radio.phase=Core::RadioPhase::Message;
 Tyra::Engine e;Events::EventBus bus;Hardware::Ps2ScratchpadArena spr;
 Systems::Weapon::WeaponSystem weapon(nullptr);weapon.init(&e,s,bus);
 auto arm=[](Core::WeaponState& w){w.beamTicks=Core::kBeamTicks;w.impactTicks=Core::kImpactTicks;w.impact=true;w.reloadTicks=5000;};
 arm(s.weapon);arm(s.enemies[0].weapon);s.inputCmd.fireRequested=true;
 Tyra::Timer::ticks+=2000;
 weapon.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&e,s,bus,spr);
 assert(!s.weapon.beamTicks && !s.weapon.impactTicks && !s.weapon.impact);
 assert(!s.enemies[0].weapon.beamTicks && !s.enemies[0].weapon.impactTicks);
 assert(s.weapon.reloadTicks==5000 && s.weapon.ammo==Core::kMagazineSize);
 arm(s.weapon);s.menu.currentScreen=Core::ScreenState::Paused;Tyra::Timer::ticks+=2000;
 weapon.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation,&e,s,bus,spr);
 assert(s.weapon.beamTicks==Core::kBeamTicks);
 std::cout<<"PASS: player/enemy beams and impacts expire during radio; reload, fire and pause stay gated\n";
}
