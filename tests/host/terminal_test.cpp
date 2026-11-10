#include <cassert>
#include "core/terminal_documents.hpp"
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
    const int lastPage = Core::kTerminalDocuments[topic].pageCount - 1;
    for (int page = 2; page <= lastPage; ++page) {
      poll();
      assert(s.scene.terminalPage == page);
      render();
    }
    poll();
    assert(s.scene.terminalPage == lastPage);
    e.pad.clicked = {}; e.pad.clicked.Circle = true; poll();
    assert(!s.scene.terminalReading && s.scene.terminalActive);
    e.pad.clicked = {}; e.pad.clicked.DpadDown = true; poll();
  }
  assert(s.scene.terminalTopic == 0);
  e.pad.pressed = {};
  e.pad.clicked = {};
  for (int i = 0; i < 12; ++i) poll();
  const int progressBeforeAnalog = s.scene.terminalCodeProgress;
  e.pad.left.v = 255;
  poll();
  assert(s.scene.terminalTopic == 1 && s.scene.terminalCodeProgress == progressBeforeAnalog);
  e.pad.left.v = 128;
  auto direction = [&](int key) {
    e.pad.clicked = {};
    e.pad.pressed = {};
    e.pad.clicked.DpadDown = key == 1;
    e.pad.clicked.DpadLeft = key == 2;
    e.pad.clicked.DpadRight = key == 3;
    e.pad.clicked.DpadUp = key == 4;
    e.pad.pressed = e.pad.clicked;
    poll();
  };
  direction(4);
  direction(1);
  e.pad.clicked = {}; poll(); poll();
  assert(s.scene.terminalCodeProgress == 1);
  e.pad.clicked.DpadRight = e.pad.pressed.DpadRight = true;
  poll();
  assert(s.scene.terminalCodeProgress == 0);
  direction(1); direction(1); direction(2); direction(4);
  assert(!s.scene.terminalSecretUnlocked && s.scene.terminalCodeProgress == 0);
  direction(1); direction(1); direction(1); direction(2); direction(3); direction(4);
  assert(s.scene.terminalSecretUnlocked && s.scene.terminalTopic == Core::kSecretTopic);
  u32 eventCount = 0;
  const auto* events = bus.getFrameEvents(eventCount);
  assert(eventCount == 1 && events[0].type == Events::EventType::TerminalSecretUnlocked);
  render();
  direction(1); direction(1); direction(2); direction(3); direction(4);
  events = bus.getFrameEvents(eventCount);
  for (u32 i = 0; i < eventCount; ++i)
    assert(events[i].type != Events::EventType::TerminalSecretUnlocked);
  s.scene.terminalTopic = Core::kSecretTopic;
  e.pad.clicked = {}; e.pad.clicked.Cross = true; poll();
  assert(s.scene.terminalVideoActive && !s.scene.terminalReading && !s.encounter.objectiveComplete);
  e.pad.clicked = {}; e.pad.clicked.Circle = true; poll();
  assert(!s.scene.terminalVideoActive && s.scene.terminalActive &&
         s.scene.terminalTopic == Core::kSecretTopic);
  e.pad.pressed = {};
  s.menu.scanlinesEnabled = false; render();
  e.pad.clicked = {}; e.pad.clicked.Circle = true; e.pad.clicked.R1 = true;
  poll(); assert(!s.scene.terminalActive);
  e.pad.clicked = {}; e.pad.clicked.Triangle = true; poll();
  assert(s.scene.terminalActive && s.scene.terminalBootFrames == 24);
  assert(!s.scene.terminalSecretUnlocked && s.scene.terminalCodeProgress == 0);
  poll(); assert(!s.scene.terminalActive); // Exit even during boot.
  // The fourth tab is available only after the final wave.
  s.encounter.newEntryUnlocked = true;
  e.pad.clicked = {}; e.pad.clicked.Triangle = true; poll();
  assert(s.scene.terminalActive && s.scene.terminalTopic == Core::kTruthTopic);
  e.pad.clicked = {};
  for (int i = 0; i < 24; ++i) poll();
  render();
  e.pad.clicked.DpadUp = true; poll();
  assert(s.scene.terminalTopic == 2); render();
  e.pad.clicked = {}; e.pad.clicked.DpadDown = true; poll();
  assert(s.scene.terminalTopic == Core::kTruthTopic);
  e.pad.clicked = {}; e.pad.clicked.Cross = true; poll();
  assert(s.scene.terminalReading && s.encounter.objectiveComplete); render();
  const int truthPages = Core::kTerminalDocuments[Core::kTruthTopic].pageCount;
  e.pad.clicked = {}; e.pad.clicked.DpadRight = true;
  for (int page = 1; page < truthPages; ++page) {
    poll();
    assert(s.scene.terminalPage == page);
    render();
  }
  poll();
  assert(s.scene.terminalPage == truthPages - 1);
  e.pad.clicked = {};
  for (int i = 0; i < 24; ++i) { poll(); render(); }
  drawer.shutdown(&e);
  assert(e.renderer.repo.getTexturesCount() == 0);
  std::cout << "PASS: terminal boot, 3 topics, pages, bounds, frozen camera, input ownership and exit\n";
}
