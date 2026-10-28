#include <cassert>
#include <cmath>
#include <iostream>
#include "systems/weapon/weapon_system.hpp"
#include "systems/input/input_system.hpp"
#include "systems/audio/menu_audio_system.hpp"
#include "systems/narrative/radio_codec_system.hpp"
#include "systems/render3d/world_render_system.hpp"
#include "systems/ui/hud_system.hpp"
#include "core/interior_layout.hpp"
using namespace DroidCampos;
using namespace Systems::Weapon;
struct Scene : WeaponScene {
  mutable int queries = 0;
  bool wall = false;
  bool traceWeapon(const Core::WorldState&, const Tyra::Vec4& o,
                   const Tyra::Vec4& d, RayHit& hit) const override {
    ++queries;
    if (!wall || std::fabs(d.z) < 1e-5F) return false;
    float t = (-2 - o.z) / d.z;
    if (t < 0 || t > hit.distance) return false;
    hit.distance = t;
    hit.position = add(o, mul(d, t));
    hit.normal = Tyra::Vec4(0, 0, 1, 0);
    return true;
  }
  Tyra::Vec4 weaponMuzzle(const Core::WorldState&) override {
    return Tyra::Vec4(.15F, 1.5F, -.5F, 1);
  }
};
struct NearWall : WeaponScene {
  bool traceWeapon(const Core::WorldState&, const Tyra::Vec4& o,
                   const Tyra::Vec4& d, RayHit& hit) const override {
    if (std::fabs(d.x) < 1e-6F) return false;
    const float t = (.075F - o.x) / d.x;
    if (t < 0 || t > hit.distance) return false;
    hit.distance = t;
    hit.position = add(o, mul(d, t));
    hit.normal = Tyra::Vec4(-1, 0, 0, 0);
    return true;
  }
  Tyra::Vec4 weaponMuzzle(const Core::WorldState&) override {
    return Tyra::Vec4(.15F, 1.5F, -.5F, 1);
  }
};
int main(int argc, char** argv) {
  assert(argc == 2);
  Tyra::FileUtils::root = argv[1];
  Core::WorldState s{};
  s.resetToDefaults();
  s.menu.currentScreen = Core::ScreenState::GameplayFps;
  s.scene.resident = s.scene.presented = true;
  s.radio.popupActive = false;
  s.player.cameraPos.set(0, 1.7F, 0);
  s.player.cameraLookAt.set(0, 1.7F, -1);
  Tyra::Engine e;
  Events::EventBus bus;
  Hardware::Ps2ScratchpadArena spr;
  Scene scene;
  Systems::Input::InputSystem input;
  WeaponSystem weapon(&scene);
  Systems::Audio::MenuAudioSystem audio;
  Systems::Narrative::RadioCodecSystem radio(nullptr);
  weapon.init(&e, s, bus);
  audio.init(&e, s, bus);
  auto poll = [&]() {
    bus.flushFrameEvents();
    input.executeStage(Core::HardwareStage::Stage0_IopInputPoll, &e, s, bus,
                       spr);
    audio.executeStage(Core::HardwareStage::Stage0_IopInputPoll, &e, s, bus,
                       spr);
  };
  auto sim = [&](u32 ticks) {
    Tyra::Timer::ticks += ticks;
    weapon.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation, &e, s, bus,
                        spr);
    radio.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation, &e, s, bus,
                       spr);
    audio.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation, &e, s, bus,
                       spr);
  };
  auto click = [&](bool fire, bool reload, u32 ticks) {
    e.pad.clicked = {};
    e.pad.clicked.R1 = fire;
    e.pad.clicked.Circle = reload;
    poll();
    sim(ticks);
  };
  click(true, false, 0);
  assert(s.weapon.ammo == 7 && s.weapon.beamTicks == Core::kBeamTicks &&
         !s.weapon.impact);
  assert(HostAudio::played.size() == 1 && HostAudio::channels.back() == 10);
  click(true, false, Core::kShotIntervalTicks - 1);
  assert(s.weapon.ammo == 7 && HostAudio::played.size() == 1);
  click(true, false, 1);
  assert(s.weapon.ammo == 6);
  e.pad.pressed.R1 = true;
  click(false, false, Core::kShotIntervalTicks);
  assert(s.weapon.ammo == 6);
  e.pad.pressed = {};
  scene.wall = true;
  click(true, false, 0);
  assert(s.weapon.ammo == 5 && s.weapon.impact &&
         std::fabs(s.weapon.beamEnd.z + 2) < 1e-5F);
  for (int i = 0; i < 5; i++) click(true, false, Core::kShotIntervalTicks);
  assert(s.weapon.ammo == 0);
  int queries = scene.queries;
  auto sounds = HostAudio::played.size();
  click(true, false, Core::kShotIntervalTicks);
  assert(s.weapon.ammo == 0 && scene.queries == queries &&
         HostAudio::played.size() == sounds);
  click(true, true, 0);
  assert(s.weapon.reloadTicks == Core::kReloadTicks && s.weapon.ammo == 0 &&
         HostAudio::channels.back() == 21);
  click(true, true, Core::kReloadTicks - 1);
  assert(s.weapon.reloadTicks == 1 && s.weapon.ammo == 0);
  click(false, false, 1);
  assert(s.weapon.ammo == 8 && s.weapon.reloadTicks == 0);
  sounds = HostAudio::played.size();
  click(false, true, 0);
  assert(!s.weapon.reloadTicks && HostAudio::played.size() == sounds);
  click(true, false, 0);
  click(false, true, 0);
  assert(s.weapon.ammo == 7 && s.weapon.reloadTicks == Core::kReloadTicks);
  s.menu.currentScreen = Core::ScreenState::Paused;
  click(true, false, 40000);
  assert(s.weapon.reloadTicks == Core::kReloadTicks);
  s.menu.currentScreen = Core::ScreenState::GameplayFps;
  click(false, false, 40000);
  assert(s.weapon.reloadTicks == Core::kReloadTicks);
  click(false, false, Core::kReloadTicks);
  assert(s.weapon.ammo == 8);
  // Bolinha closes the radio and never starts a reload on that same press.
  s.radio.popupActive = true;
  s.radio.phase = Core::RadioPhase::Message;
  click(true, false, Core::kShotIntervalTicks);
  assert(s.weapon.ammo == 7 && s.radio.popupActive);
  click(false, true, 0);
  assert(!s.radio.popupActive && !s.weapon.reloadTicks && s.weapon.ammo == 7);
  click(false, true, 0);
  assert(s.weapon.reloadTicks == Core::kReloadTicks);
  s.weapon.cancelTransient();
  const int saved = s.weapon.ammo;
  s.scene.active = Core::SceneId::Interior;
  s.resetPlayerToSpawn();
  assert(s.weapon.ammo == saved && !s.weapon.reloadTicks);
  s.radio.popupActive = false;
  s.intro.active = true;
  click(true, true, 10000);
  assert(s.weapon.ammo == saved && !s.weapon.reloadTicks);
  s.intro.active = false;
  s.menu.currentScreen = Core::ScreenState::Loading;
  click(true, true, 10000);
  assert(s.weapon.ammo == saved);
  s.menu.currentScreen = Core::ScreenState::GameplayFps;
  s.weapon = Core::WeaponState{};
  s.menu.sfxVolume = 0;
  click(false, false, 0);
  sounds = HostAudio::played.size();
  click(true, false, 0);
  assert(s.weapon.ammo == 7 && HostAudio::played.size() == sounds);
  s.menu.sfxVolume = 90;
  HostAudio::playResult = -1;
  click(true, false, Core::kShotIntervalTicks);
  assert(s.weapon.ammo == 6);
  HostAudio::playResult = 0;
  s.menu.currentScreen = Core::ScreenState::NewGame;
  e.pad.clicked = {};
  e.pad.clicked.Cross = true;
  poll();
  assert(s.weapon.ammo == 8);
  audio.shutdown(&e);
  assert(HostAudio::loaded.empty());
  // A wall between camera and barrel must stop the beam on the near side.
  NearWall nearWall;
  WeaponSystem blocked(&nearWall);
  s.resetToDefaults();
  s.scene.resident = s.scene.presented = true;
  s.menu.currentScreen = Core::ScreenState::GameplayFps;
  s.radio.popupActive = false;
  s.player.cameraPos.set(0, 1.7F, 0);
  s.player.cameraLookAt.set(0, 1.7F, -1);
  blocked.init(&e, s, bus);
  s.inputCmd.fireRequested = true;
  blocked.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation, &e, s, bus,
                       spr);
  assert(s.weapon.impact && std::fabs(s.weapon.beamEnd.x - .075F) < 1e-6F &&
         s.weapon.beamStart.x == 0);
  s.weapon.cancelTransient();
  s.inputCmd.clear();
  // Actual scene geometry: doors, ceiling/floor, glass, reader; no mist/pistol.
  Systems::Render3D::WorldRenderSystem world;
  s.resetToDefaults();
  world.init(&e, s, bus);
  world.loadSceneStep(&e, Core::SceneId::Exterior, 0);
  RayHit beforeMist;
  const bool expectedMist = world.traceWeapon(
      s, Tyra::Vec4(0, 2, 10), Tyra::Vec4(0, -1, 0, 0), beforeMist);
  world.loadSceneStep(&e, Core::SceneId::Exterior, 1);
  RayHit reader;
  assert(world.traceWeapon(s, Tyra::Vec4(2.48F, 1.3F, 4.7F),
                           Tyra::Vec4(0, 0, -1, 0), reader));
  assert(reader.distance > 1 && reader.distance < 1.7F);
  RayHit shortRange;
  shortRange.distance = .1F;
  assert(!world.traceWeapon(s, Tyra::Vec4(2.48F, 1.3F, 4.7F),
                            Tyra::Vec4(0, 0, -1, 0), shortRange));
  RayHit mist;
  assert(world.traceWeapon(s, Tyra::Vec4(0, 2, 10), Tyra::Vec4(0, -1, 0, 0),
                           mist) == expectedMist);
  assert(std::fabs(mist.distance - beforeMist.distance) < 1e-5F);
  world.unloadScene(&e);
  RayHit empty;
  assert(!world.traceWeapon(s, Tyra::Vec4(0, 1, 0), Tyra::Vec4(0, 0, -1, 0),
                            empty));
  for (int i = 0; i < Core::kInsidePartCount; i++)
    world.loadSceneStep(&e, Core::SceneId::Interior, i);
  s.scene.active = Core::SceneId::Interior;
  for (int door = 0; door < 4; door++) {
    const float sign = Core::kInsideDoorX[door] < 0 ? -1 : 1;
    RayHit closed;
    assert(world.traceWeapon(s, Tyra::Vec4(0, 1.7F, Core::kInsideDoorZ[door]),
                             Tyra::Vec4(sign, 0, 0, 0), closed));
    assert(closed.distance < 2);
    s.scene.doors[door].openAmount = 1;
    RayHit opened;
    assert(world.traceWeapon(s, Tyra::Vec4(0, 1.7F, Core::kInsideDoorZ[door]),
                             Tyra::Vec4(sign, 0, 0, 0), opened));
    assert(opened.distance > 2);
  }
  RayHit floor;
  assert(world.traceWeapon(s, Tyra::Vec4(0, 1.7F, -12), Tyra::Vec4(0, -1, 0, 0),
                           floor));
  assert(floor.distance < 1.8F);
  RayHit ceiling;
  assert(world.traceWeapon(s, Tyra::Vec4(0, 1.7F, -12), Tyra::Vec4(0, 1, 0, 0),
                           ceiling));
  assert(ceiling.distance < 2);
  RayHit glass;
  assert(world.traceWeapon(s, Tyra::Vec4(0, 1.5F, -29), Tyra::Vec4(1, 0, 0, 0),
                           glass));
  assert(glass.distance > 2 && glass.distance < 4);
  // Weapon effects: persistent geometry, depth write restored, no new textures.
  s.menu.currentScreen = Core::ScreenState::GameplayFps;
  s.weapon.beamTicks = Core::kBeamTicks;
  s.weapon.impactTicks = Core::kImpactTicks;
  s.weapon.impact = true;
  s.weapon.beamStart.set(0, 1.7F, -10);
  s.weapon.beamEnd.set(0, 1.7F, -12);
  s.weapon.impactNormal.set(0, 0, 1, 0);
  auto textures = e.renderer.repo.getTexturesCount();
  for (int mask : {0, 1}) {
    e.renderer.core.gs.zBuffer.mask = mask;
    world.executeStage(Core::HardwareStage::Stage2_Vu1GsRender3D, &e, s, bus,
                       spr);
    assert(e.renderer.core.gs.zBuffer.mask == mask);
  }
  assert(e.renderer.repo.getTexturesCount() == textures);
  world.shutdown(&e);
  assert(e.renderer.repo.getTexturesCount() == 0);
  std::cout << "PASS: magazine, edge input, cooldown, reload, pause, radio "
               "precedence, mute/busy, real geometry/doors/glass, effects and "
               "cleanup\n";
}
