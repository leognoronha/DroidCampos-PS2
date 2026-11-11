#include <cassert>
#include <fstream>
#include <iostream>
#include <algorithm>
#include "systems/ui/hud_system.hpp"
using namespace DroidCampos;
int main(int argc, char** argv) {
  assert(argc >= 2);
  Tyra::FileUtils::root = argv[1];
  Core::WorldState s{};
  s.resetToDefaults();
  s.menu.currentScreen = Core::ScreenState::GameplayFps;
  s.radio.popupActive = false;
  Tyra::Engine e;
  Events::EventBus bus;
  Hardware::Ps2ScratchpadArena spr;
  Systems::Ui::UiContextDrawer drawer;
  Systems::Ui::HudSystem hud(drawer);
  hud.init(&e, s, bus);
  auto render = [&](const char* name, bool expected) {
    auto& draws = e.renderer.renderer2D.draws;
    draws.clear();
    hud.executeStage(Core::HardwareStage::Stage3_GifPath3Render2D, &e, s, bus,
                     spr);
    int slots = 0;
    bool healthTop = false, kitTop = false, ammoTop = false;
    bool healthBottom = false, kitBottom = false, ammoBottom = false;
    bool kitCrossVertical = false, kitCrossHorizontal = false;
    for (const auto& d : draws) {
      assert(d.position.x >= 0 && d.position.y >= 0);
      assert(d.position.x + d.size.x * d.scale <= 512 &&
             d.position.y + d.size.y * d.scale <= 448);
      if (d.position.y == 358 && d.size.y == 1) {
        healthTop |= d.position.x == 20 && d.size.x == 122;
        kitTop |= d.position.x == 166 && d.size.x == 28;
        ammoTop |= d.position.x == 388 && d.size.x == 104;
      }
      if (d.position.y == 384 && d.position.x >= 390 && d.size.x == 9 &&
          d.size.y == 3)
        ++slots;
      if (d.position.y == 403 && d.size.y == 1) {
        healthBottom |= d.position.x == 20 && d.size.x == 122;
        kitBottom |= d.position.x == 166 && d.size.x == 28;
        ammoBottom |= d.position.x == 388 && d.size.x == 104;
      }
      if (d.color.g > d.color.r && d.color.g > d.color.b) {
        kitCrossVertical |= d.position.x == 169 && d.position.y == 368 &&
                            d.size.x == 4 && d.size.y == 12;
        kitCrossHorizontal |= d.position.x == 165 && d.position.y == 372 &&
                              d.size.x == 12 && d.size.y == 4;
      }
    }
    const bool showVitals = !s.intro.active && !s.radio.popupActive;
    assert(healthBottom == showVitals && kitBottom == showVitals);
    assert(ammoBottom == expected);
    assert(healthTop == showVitals && kitTop == showVitals);
    assert(ammoTop == expected && slots == (expected ? 8 : 0));
    assert(kitCrossVertical == showVitals && kitCrossHorizontal == showVitals);
    if (argc > 2) {
      std::ofstream out(std::string(argv[2]) + "/" + name + ".txt");
      assert(out);
      for (const auto& d : draws) {
        auto* tex = e.renderer.repo.getByMeshMaterialId(d.id);
        assert(tex);
        out << tex->path << " " << d.position.x << " " << d.position.y << " "
            << d.size.x << " " << d.size.y << " " << d.offset.x << " "
            << d.offset.y << " " << d.scale << " " << d.color.r << " "
            << d.color.g << " " << d.color.b << " " << d.color.a << "\n";
      }
    }
  };
  render("weapon_uncollected", false);
  s.inventory.collect(Core::Pickup::Pistol);
  render("weapon_full", true);
  s.weapon.ammo = 0;
  render("weapon_empty", true);
  s.weapon.reloadTicks = Core::kReloadTicks / 2;
  render("weapon_reload", true);
  s.weapon.ammo = 3;
  s.weapon.reloadTicks = 0;
  s.scene.nearbyDoor = 0;
  render("weapon_door", true);
  s.scene.nearbyDoor = -1;
  s.cardReader.isPlayerNearby = true;
  render("weapon_reader", true);
  s.radio.popupActive = true;
  s.encounter.phase = Core::EncounterPhase::Complete;
  render("weapon_radio_hidden", false);
  s.encounter.phase = Core::EncounterPhase::Dormant;
  s.radio.popupActive = false;
  s.scene.terminalNearby = true;
  render("weapon_near_terminal", true);
  s.scene.terminalNearby=false;
  for(auto phase:{Core::EncounterPhase::Countdown,Core::EncounterPhase::Wave,
                 Core::EncounterPhase::Intermission,Core::EncounterPhase::Complete}) {
    s.encounter.phase=phase;s.encounter.wave=2;s.encounter.remaining=5;
    s.encounter.countdownTicks=78125;s.playerHealth=10;s.damageFlashTicks=100;
    s.encounter.newEntryUnlocked=phase==Core::EncounterPhase::Complete;
    render("wave_health",true);
  }
  s.intro.active = true;
  render("weapon_intro_hidden", false);
  drawer.shutdown(&e);
  assert(e.renderer.repo.getTexturesCount() == 0);
  std::cout << "PASS: ammo HUD bounds, 8 indicators, full/empty/reload, "
               "contextual prompts and overlay hiding\n";
}
