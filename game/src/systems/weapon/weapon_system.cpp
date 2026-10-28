#include "systems/weapon/weapon_system.hpp"
namespace DroidCampos {
namespace Systems {
namespace Weapon {
void WeaponSystem::init(Tyra::Engine*, Core::WorldState&, Events::EventBus&) {
  timer_.prime();
  wasActive_ = false;
}
void WeaponSystem::executeStage(Core::HardwareStage stage, Tyra::Engine*,
                                Core::WorldState& s, Events::EventBus& bus,
                                Hardware::Ps2ScratchpadArena&) {
  if (stage != Core::HardwareStage::Stage1_EeVu0Simulation) return;
  u32 elapsed = timer_.getTimeDelta();
  timer_.prime();
  const bool active = s.menu.currentScreen == Core::ScreenState::GameplayFps &&
                      !s.intro.active && s.scene.resident &&
                      s.scene.presented && !s.scene.requested &&
                      s.scene.accessFrames == 0;
  if (!active) {
    wasActive_ = false;
    return;
  }
  if (!wasActive_) elapsed = 0;
  wasActive_ = true;
  auto& w = s.weapon;
  auto subtract = [elapsed](u32& t) { t = elapsed >= t ? 0 : t - elapsed; };
  subtract(w.cooldownTicks);
  subtract(w.beamTicks);
  subtract(w.impactTicks);
  const bool wasReloading = w.reloadTicks > 0;
  subtract(w.reloadTicks);
  if (wasReloading && w.reloadTicks == 0) w.ammo = Core::kMagazineSize;
  if (s.inputCmd.reloadRequested && !w.reloadTicks &&
      w.ammo < Core::kMagazineSize) {
    w.reloadTicks = Core::kReloadTicks;
    bus.publish(
        Events::GameEvent::makeSimple(Events::EventType::WeaponReloadStarted));
    return;
  }
  if (!s.inputCmd.fireRequested || w.reloadTicks || w.cooldownTicks ||
      w.ammo == 0 || !scene_)
    return;
  --w.ammo;
  w.cooldownTicks = Core::kShotIntervalTicks;
  w.beamTicks = Core::kBeamTicks;
  const auto direction = unit(sub(s.player.cameraLookAt, s.player.cameraPos));
  RayHit hit;
  const bool aimedHit =
      scene_->traceWeapon(s, s.player.cameraPos, direction, hit);
  const auto target =
      aimedHit ? hit.position : add(s.player.cameraPos, mul(direction, 50));
  w.beamStart = scene_->weaponMuzzle(s);
  // Check camera-to-muzzle too: never start a laser on the far side of a wall.
  const auto toMuzzle = sub(w.beamStart, s.player.cameraPos);
  RayHit obstruction;
  obstruction.distance = std::sqrt(dot(toMuzzle, toMuzzle));
  if (scene_->traceWeapon(s, s.player.cameraPos, unit(toMuzzle), obstruction)) {
    w.beamStart = s.player.cameraPos;
    hit = obstruction;
    w.impact = true;
  } else {
    const auto segment = sub(target, w.beamStart);
    RayHit muzzleHit;
    muzzleHit.distance = std::sqrt(dot(segment, segment));
    if (scene_->traceWeapon(s, w.beamStart, unit(segment), muzzleHit)) {
      hit = muzzleHit;
      w.impact = true;
    } else {
      w.impact = aimedHit;
      hit.position = target;
    }
  }
  w.beamEnd = hit.position;
  w.impactNormal = hit.normal;
  w.impactTicks = w.impact ? Core::kImpactTicks : 0;
  bus.publish(Events::GameEvent::makeSimple(Events::EventType::WeaponFired));
}
}  // namespace Weapon
}  // namespace Systems
}  // namespace DroidCampos
