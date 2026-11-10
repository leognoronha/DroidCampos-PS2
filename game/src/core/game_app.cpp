#include "core/game_app.hpp"
#include <cstdio>
#include "core/interior_layout.hpp"

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
      menuAudioSystem_(),
      playerSystem_(),
      render3dSystem_(),
      weaponSystem_(&render3dSystem_),
      enemySystem_(&render3dSystem_),
      radioCodecSystem_(&uiDrawer_),
      hudSystem_(&uiDrawer_),
      menuSystem_(&uiDrawer_),
      sceneIntroSystem_(&uiDrawer_) {
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

  engine_->renderer.setClearScreenColor(Tyra::Color(0.0F, 0.0F, 0.0F));
  engine_->renderer.core.renderer2D.setTextureMappingType(Tyra::TyraNearest);

  worldState_.resetToDefaults();
  eventBus_.clear();
  scratchpad_.resetFrame();
  uiDrawer_.init(engine_);

  scheduler_.registerSubsystem(&inputSystem_);
  scheduler_.registerSubsystem(&playerSystem_);
  scheduler_.registerSubsystem(&weaponSystem_);
  scheduler_.registerSubsystem(&enemySystem_);
  scheduler_.registerSubsystem(&radioCodecSystem_);
  scheduler_.registerSubsystem(&menuAudioSystem_);
  scheduler_.registerSubsystem(&render3dSystem_);
  scheduler_.registerSubsystem(&hudSystem_);
  scheduler_.registerSubsystem(&menuSystem_);
  scheduler_.registerSubsystem(&sceneIntroSystem_);

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

  // Convert scene requests to Loading before any simulation/audio can observe
  // the input system's temporary GameplayFps state on confirmation.
  updateSceneTransition();

  // Stage 1: EE + VU0 + Scratchpad Simulation
  stageTimer_.prime();
  scheduler_.runStage(HardwareStage::Stage1_EeVu0Simulation, engine_,
                      worldState_, eventBus_, scratchpad_);

  const u32 simulationTicks=stageTimer_.getTimeDelta();
  u32 renderTicks=0;
  auto& renderer = engine_->renderer;
  renderer.core.renderer2D.setTextureMappingType(
      worldState_.menu.nearestFilter ? Tyra::TyraNearest : Tyra::TyraLinear);

  if (worldState_.menu.currentScreen == ScreenState::GameplayFps) {
    renderer.setClearScreenColor(Tyra::Color(16.0F, 20.0F, 30.0F));

    Tyra::CameraInfo3D camInfo(&worldState_.player.cameraPos,
                               &worldState_.player.cameraLookAt,
                               &worldState_.player.cameraUp);
    if (worldState_.scene.terminalActive && worldState_.scene.terminalVideoActive) {
      renderer.beginFrame();
    } else {
      renderer.beginFrame(camInfo);
      // Stage 2: VU1 / GS 3D DMA Kick (StaticPipeline: Scenario, Mist, Viewmodel)
      stageTimer_.prime();
      scheduler_.runStage(HardwareStage::Stage2_Vu1GsRender3D, engine_,
                          worldState_, eventBus_, scratchpad_);
      renderTicks=stageTimer_.getTimeDelta();
    }
    // Stage 3: GIF Path 3 2D HUD & Codec Overlay
    scheduler_.runStage(HardwareStage::Stage3_GifPath3Render2D, engine_,
                        worldState_, eventBus_, scratchpad_);

    renderer.endFrame();
    worldState_.scene.presented = true;
  } else {
    renderer.setClearScreenColor(Tyra::Color(0.0F, 0.0F, 0.0F));
    renderer.beginFrame();

    // Stage 3: GIF Path 3 2D Menus
    scheduler_.runStage(HardwareStage::Stage3_GifPath3Render2D, engine_,
                        worldState_, eventBus_, scratchpad_);

    renderer.endFrame();
  }

  recordCombatMetrics(simulationTicks,renderTicks);
  scheduler_.yieldToIopAudioThread();
  // Loading was presented before any synchronous IO or destruction begins.
  if (worldState_.menu.currentScreen == ScreenState::Loading && !worldState_.scene.error)
    advanceLoading();
}

void GameApp::recordCombatMetrics(u32 simulationTicks,u32 renderTicks) {
  if(worldState_.scene.active!=SceneId::Interior || !worldState_.simulationActive()) {
    metricsFrames_=simulationTicks_=renderTicks_=0;
    return;
  }
  int actors=0;for(const auto& enemy:worldState_.enemies)if(enemy.active)++actors;
  if(actors!=metricsActors_) {
    metricsActors_=actors;metricsFrames_=simulationTicks_=renderTicks_=0;
    std::printf("[COMBAT MEMORY] actors=%d free=%.3f MiB textures=%lu\n",actors,
        engine_->info.getAvailableRAM(),static_cast<unsigned long>(
        engine_->renderer.getTextureRepository().getTexturesCount()));
  }
  simulationTicks_+=simulationTicks;renderTicks_+=renderTicks;
  if(++metricsFrames_<120)return;
  const float ms=1000.F/(kCombatTicksPerSecond*static_cast<float>(metricsFrames_));
  std::printf("[COMBAT PERF] actors=%d fps=%lu simulation-ms=%.3f render-submit-ms=%.3f\n",
      actors,static_cast<unsigned long>(engine_->info.getFps()),
      simulationTicks_*ms,renderTicks_*ms);
  metricsFrames_=simulationTicks_=renderTicks_=0;
}

void GameApp::logSceneMemory(const char* phase) {
  std::printf("[ETIP MEMORY] %s free=%.3f MiB textures=%lu\n", phase,
      engine_->info.getAvailableRAM(),
      static_cast<unsigned long>(engine_->renderer.getTextureRepository().getTexturesCount()));
}

void GameApp::updateSceneTransition() {
  auto& state = worldState_;
  auto& scene = state.scene;
  if (state.menu.currentScreen == ScreenState::Paused ||
      (state.menu.currentScreen == ScreenState::Settings &&
       state.menu.settingsReturnScreen == ScreenState::Paused)) return;
  if (scene.accessFrames > 0 && --scene.accessFrames == 0) {
    scene.target = SceneId::Interior;
    scene.requested = true;
  }
  u32 count = 0;
  const auto* events = eventBus_.getFrameEvents(count);
  for (u32 i = 0; i < count; ++i) {
    if (events[i].type == Events::EventType::CardReaderInteracted &&
        scene.active == SceneId::Exterior && scene.resident &&
        scene.accessFrames == 0 && !scene.requested && !state.radio.popupActive) {
      scene.accessFrames = 20;
      state.inputCmd.clear();
    }
  }
  if (!scene.requested) return;
  scene.requested = false;
  scene.loadStep = 0;
  scene.presented = false;
  state.intro = SceneIntroState{};
  scene.totalSteps = scene.target == SceneId::Interior ? kInsideLoadStepCount + 1 : 3;
  scene.error = render3dSystem_.validateScene(scene.target) ? nullptr : "ASSET AUSENTE. VERIFIQUE OS ARQUIVOS.";
  state.menu.currentScreen = ScreenState::Loading;
  state.inputCmd.clear();
  state.weapon.cancelTransient();
  state.radio.popupActive = false;
  state.radio.phase = RadioPhase::Idle;
}

void GameApp::advanceLoading() {
  auto& scene = worldState_.scene;
  if (scene.loadStep == 0) {
    // endFrame swaps buffers, but does not itself wait for outstanding DMA/GS.
    engine_->renderer.core.sync.align3D();
    engine_->renderer.core.sync.align2D();
    logSceneMemory("before unload");
    render3dSystem_.unloadScene(engine_);
    scene.resident = false;
    logSceneMemory("after unload");
  } else if (scene.loadStep < scene.totalSteps) {
    if (!render3dSystem_.loadSceneStep(engine_, scene.target, scene.loadStep - 1)) {
      render3dSystem_.unloadScene(engine_);
      scene.resident = false;
      scene.error = "ERRO AO CARREGAR INIMIGOS. VERIFIQUE OS ARQUIVOS.";
      return;
    }
    logSceneMemory("load step");
  } else {
    scene.active = scene.target;
    scene.resident = true;
    scene.presented = false;
    worldState_.intro = SceneIntroState{true, 0};
    scene.nearbyDoor = -1;
    for (auto& door : scene.doors) door = DoorState{};
    Systems::Enemy::EnemySystem::reset(worldState_);
    worldState_.resetPlayerToSpawn();
    worldState_.menu.currentScreen = ScreenState::GameplayFps;
    logSceneMemory("scene ready");
    return;
  }
  ++scene.loadStep;
}

}  // namespace Core
}  // namespace DroidCampos
