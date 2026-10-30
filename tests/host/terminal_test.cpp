#include <cassert>
#include <iostream>
#include "systems/input/input_system.hpp"
#include "systems/player/player_controller_system.hpp"
#include "systems/ui/hud_system.hpp"
using namespace DroidCampos;
int main(int argc, char** argv) {
  assert(argc >= 2);
  Tyra::FileUtils::root = argv[1];
  Core::WorldState s{}; s.resetToDefaults();
  Tyra::Engine e; Events::EventBus bus; Hardware::Ps2ScratchpadArena spr;
  Systems::Input::InputSystem input;
  Systems::Player::PlayerControllerSystem player;
  Systems::Ui::UiContextDrawer drawer;
  Systems::Ui::HudSystem hud(drawer); hud.init(&e, s, bus);
  auto poll = [&]() {
    bus.flushFrameEvents();
    input.executeStage(Core::HardwareStage::Stage0_IopInputPoll, &e, s, bus, spr);
    assert(!s.inputCmd.fireRequested && !s.inputCmd.reloadRequested &&
           !s.inputCmd.jumpRequested && s.inputCmd.moveForward == 0 &&
           s.inputCmd.lookYawDelta == 0);
  };
  auto render = [&]() {
    auto& draws = e.renderer.renderer2D.draws; draws.clear();
    hud.executeStage(Core::HardwareStage::Stage3_GifPath3Render2D, &e, s, bus, spr);
    assert(!draws.empty());
    for (const auto& d : draws) {
      assert(d.position.x >= 0 && d.position.y >= 0);
      assert(d.position.x+d.size.x*d.scale <= 512);
      assert(d.position.y+d.size.y*d.scale <= 448);
      // Gameplay reticle and weapon panel must be absent.
      assert(!(d.position.x == 248 && d.position.y == 223));
      assert(!(d.position.x == 380 && d.position.y == 352));
    }
  };
  s.menu.currentScreen = Core::ScreenState::GameplayFps;
  s.scene.active = Core::SceneId::Interior;
  s.radio.popupActive = false;
  s.player.playerPos.set(-2.5F, 1.7F, -28);
  e.pad.clicked.Triangle = true; e.pad.clicked.R1 = true;
  e.pad.pressed.DpadUp = true; e.pad.right.h = 255;
  poll(); assert(s.scene.terminalActive); render();
  const auto position = s.player.playerPos;
  const auto camera = s.player.cameraLookAt;
  s.player.verticalVelocity = .2F; // Even an airborne access freezes the view.
  player.executeStage(Core::HardwareStage::Stage1_EeVu0Simulation, &e, s, bus, spr);
  assert(s.player.playerPos.x == position.x && s.player.playerPos.y == position.y);
  assert(s.player.cameraLookAt.x == camera.x);
  e.pad.clicked = {};
  for (int i=0; i<24; ++i) poll();
  assert(s.scene.terminalBootFrames == 0);
  for (int topic=0; topic<3; ++topic) {
    assert(s.scene.terminalTopic == topic); render();
    e.pad.clicked = {}; e.pad.clicked.Cross = true; poll();
    assert(s.scene.terminalReading && s.scene.terminalPage == 0); render();
    e.pad.clicked = {}; e.pad.clicked.DpadLeft = true; poll();
    assert(s.scene.terminalPage == 0);
    e.pad.clicked = {}; e.pad.clicked.DpadRight = true; poll();
    assert(s.scene.terminalPage == 1); render();
    poll(); assert(s.scene.terminalPage == 1);
    e.pad.clicked = {}; e.pad.clicked.Circle = true; poll();
    assert(!s.scene.terminalReading && s.scene.terminalActive);
    e.pad.clicked = {}; e.pad.clicked.DpadDown = true; poll();
  }
  assert(s.scene.terminalTopic == 0);
  s.menu.scanlinesEnabled = false; render();
  e.pad.clicked = {}; e.pad.clicked.Circle = true; e.pad.clicked.R1 = true;
  poll(); assert(!s.scene.terminalActive);
  e.pad.clicked = {}; e.pad.clicked.Triangle = true; poll();
  assert(s.scene.terminalActive && s.scene.terminalBootFrames == 24);
  poll(); assert(!s.scene.terminalActive); // Exit even during boot.
  // The final-wave record is gated, highlighted and leaves the original accessible.
  s.encounter.newEntryUnlocked=true;s.scene.terminalActive=true;
  s.scene.terminalBootFrames=0;s.scene.terminalTopic=1;
  e.pad.clicked={};e.pad.clicked.DpadDown=true;poll();
  assert(s.scene.terminalTopic==2 && s.scene.terminalEntry==1);render();
  e.pad.clicked={};e.pad.clicked.DpadLeft=true;poll();
  assert(s.scene.terminalEntry==0);render();
  e.pad.clicked={};e.pad.clicked.DpadRight=true;poll();
  assert(s.scene.terminalEntry==1);
  e.pad.clicked={};e.pad.clicked.Cross=true;poll();
  assert(s.scene.terminalReading && s.encounter.objectiveComplete);render();
  drawer.shutdown(&e);
  assert(e.renderer.repo.getTexturesCount() == 0);
  std::cout << "PASS: terminal boot, 3 topics, pages, bounds, frozen camera, input ownership and exit\n";
}
