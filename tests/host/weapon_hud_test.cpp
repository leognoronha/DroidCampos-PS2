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
    bool panel = false;
    for (const auto& d : draws) {
      assert(d.position.x >= 0 && d.position.y >= 0);
      assert(d.position.x + d.size.x * d.scale <= 512 &&
             d.position.y + d.size.y * d.scale <= 448);
      if (d.position.x == 380 && d.position.y == 352 && d.size.x == 120)
        panel = true;
      if (d.position.y == 381 && d.position.x >= 390 && d.size.x == 9 &&
          d.size.y == 3)
        ++slots;
    }
    assert(panel == expected && slots == (expected ? 8 : 0));
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
  render("weapon_radio_hidden", false);
  s.radio.popupActive = false;
  s.scene.terminalNearby = true;
  render("weapon_terminal_hidden", false);
  s.scene.terminalNearby=false;
  for(auto phase:{Core::EncounterPhase::Countdown,Core::EncounterPhase::Wave,
                 Core::EncounterPhase::Intermission,Core::EncounterPhase::Complete}) {
    s.encounter.phase=phase;s.encounter.wave=2;s.encounter.remaining=5;
    s.encounter.countdownTicks=78125;s.playerHealth=10;s.damageFlashTicks=100;
    render("wave_health",true);
  }
  s.intro.active = true;
  render("weapon_intro_hidden", false);
  drawer.shutdown(&e);
  assert(e.renderer.repo.getTexturesCount() == 0);
  std::cout << "PASS: ammo HUD bounds, 8 indicators, full/empty/reload, "
               "contextual prompts and overlay hiding\n";
}
