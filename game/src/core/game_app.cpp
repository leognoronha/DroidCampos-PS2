#include "core/game_app.hpp"

namespace DroidCampos {
namespace Core {

GameApp::GameApp(Tyra::Engine* engine)
    : engine_(engine),
      worldState_{},
      eventBus_(),
      scratchpad_(false),
      scheduler_(),
      uiDrawer_(),
      inputSystem_(),
      playerSystem_(),
      render3dSystem_(),
      radioCodecSystem_(&uiDrawer_),
      hudSystem_(&uiDrawer_),
      menuSystem_(&uiDrawer_) {
  worldState_.resetToDefaults();
}

GameApp::~GameApp() {
  scheduler_.shutdownAll(engine_);
  uiDrawer_.shutdown(engine_);
}

void GameApp::init() {
  if (engine_ == nullptr) {
    return;
  }

  engine_->renderer.setClearScreenColor(Tyra::Color(12.0F, 15.0F, 22.0F));
  engine_->renderer.core.renderer2D.setTextureMappingType(Tyra::TyraNearest);

  worldState_.resetToDefaults();
  eventBus_.clear();
  scratchpad_.resetFrame();
  uiDrawer_.init(engine_);

  scheduler_.registerSubsystem(&inputSystem_);
  scheduler_.registerSubsystem(&playerSystem_);
  scheduler_.registerSubsystem(&radioCodecSystem_);
  scheduler_.registerSubsystem(&render3dSystem_);
  scheduler_.registerSubsystem(&hudSystem_);
  scheduler_.registerSubsystem(&menuSystem_);

  scheduler_.initAll(engine_, worldState_, eventBus_);

  TYRA_LOG("DroidCampos-PS2 GameApp initialized (Modular ECS-Lite + PS2 Stages Ready)!");
}

void GameApp::loop() {
  if (engine_ == nullptr) {
    return;
  }

  scratchpad_.resetFrame();
  eventBus_.flushFrameEvents();

  // Stage 0: IOP Input Poll (DualShock 2 / SIF RPC)
  scheduler_.runStage(HardwareStage::Stage0_IopInputPoll, engine_, worldState_,
                      eventBus_, scratchpad_);

  // Stage 1: EE + VU0 + Scratchpad Simulation
  scheduler_.runStage(HardwareStage::Stage1_EeVu0Simulation, engine_,
                      worldState_, eventBus_, scratchpad_);

  auto& renderer = engine_->renderer;
  renderer.core.renderer2D.setTextureMappingType(
      worldState_.menu.nearestFilter ? Tyra::TyraNearest : Tyra::TyraLinear);

  if (worldState_.menu.currentScreen == ScreenState::GameplayFps) {
    renderer.setClearScreenColor(Tyra::Color(16.0F, 20.0F, 30.0F));

    Tyra::CameraInfo3D camInfo(&worldState_.player.cameraPos,
                               &worldState_.player.cameraLookAt,
                               &worldState_.player.cameraUp);
    renderer.beginFrame(camInfo);

    // Stage 2: VU1 / GS 3D DMA Kick (StaticPipeline: Scenario, Mist, Viewmodel)
    scheduler_.runStage(HardwareStage::Stage2_Vu1GsRender3D, engine_,
                        worldState_, eventBus_, scratchpad_);

    // Stage 3: GIF Path 3 2D HUD & Codec Overlay
    scheduler_.runStage(HardwareStage::Stage3_GifPath3Render2D, engine_,
                        worldState_, eventBus_, scratchpad_);

    renderer.endFrame();
  } else {
    renderer.setClearScreenColor(Tyra::Color(12.0F, 15.0F, 22.0F));
    renderer.beginFrame();

    // Stage 3: GIF Path 3 2D Menus
    scheduler_.runStage(HardwareStage::Stage3_GifPath3Render2D, engine_,
                        worldState_, eventBus_, scratchpad_);

    renderer.endFrame();
  }

  scheduler_.yieldToIopAudioThread();
}

}  // namespace Core
}  // namespace DroidCampos
