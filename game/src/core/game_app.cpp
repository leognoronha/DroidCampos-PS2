#include "core/game_app.hpp"
#include <cstdio>
#include "core/interior_layout.hpp"
#include "irx/irx_loader.hpp"

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
  memoryCard_.init(Tyra::IrxLoader::isMemoryCardLoaded());

  TYRA_LOG("DroidCampos-PS2 GameApp initialized (Modular ECS-Lite + PS2 Stages Ready)!");
}

void GameApp::loop() {
  if (engine_ == nullptr) {
    return;
  }

  scratchpad_.resetFrame();
  eventBus_.flushFrameEvents();

  auto& state = worldState_;
  const auto previousScreen = state.menu.currentScreen;
  const bool inSaveMenu = (previousScreen == ScreenState::NewGame && state.menu.saveDialog) ||
      previousScreen == ScreenState::LoadGame;
  memoryCard_.update(inSaveMenu);
  const auto status = memoryCard_.status();
  const auto epoch = memoryCard_.mediaEpoch();
  const auto sequence = memoryCard_.saved().sequence;
  if (!memoryCard_.busy() && pendingSaveMenuAction_ == Events::EventType::None &&
      (menuCardStatus_ != status || menuMediaEpoch_ != epoch || menuSaveSequence_ != sequence)) {
    state.menu.confirmNewGame = false;
    menuCardStatus_ = status;
    menuMediaEpoch_ = epoch;
    menuSaveSequence_ = sequence;
  }
  state.menu.saveStatus = pendingSaveMenuAction_ != Events::EventType::None
      ? SaveStatus::Checking : status;
  state.menu.savedStage = memoryCard_.saved().data.stage;

  // Stage 0: IOP Input Poll (DualShock 2 / SIF RPC)
  scheduler_.runStage(HardwareStage::Stage0_IopInputPoll, engine_, worldState_,
                      eventBus_, scratchpad_);

  updateSaveMenu(previousScreen);

  // Convert scene requests to Loading before any simulation/audio can observe
  // the input system's temporary GameplayFps state on confirmation.
  updateSceneTransition();

  // Stage 1: EE + VU0 + Scratchpad Simulation
  const auto phaseBefore = state.encounter.phase;
  stageTimer_.prime();
  scheduler_.runStage(HardwareStage::Stage1_EeVu0Simulation, engine_,
                      worldState_, eventBus_, scratchpad_);

  const u32 simulationTicks=stageTimer_.getTimeDelta();
  if (state.scene.active == SceneId::Interior && state.scene.resident &&
      state.playerHealth > 0 && state.inventory.canEnter()) {
    if (phaseBefore == EncounterPhase::Armed && state.encounter.phase == EncounterPhase::Countdown)
      queueCheckpoint(Checkpoint::WavesStarted);
    else if (phaseBefore == EncounterPhase::Wave && state.encounter.phase == EncounterPhase::Complete)
      queueCheckpoint(Checkpoint::WavesCleared);
  }

  const auto notice = memoryCard_.notice();
  if (notice != lastSaveNotice_) {
    lastSaveNotice_ = notice;
    saveNoticeTicks_ = notice == SaveNotice::Saved ? 180 : 300;
  }
  if (state.menu.currentScreen == ScreenState::GameplayFps && !state.scene.terminalActive &&
      !state.intro.active && saveNoticeTicks_ > 0 && notice != SaveNotice::Writing)
    --saveNoticeTicks_;
  state.menu.saveNotice = saveNoticeTicks_ > 0 ? notice : SaveNotice::None;
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

void GameApp::queueCheckpoint(Checkpoint stage) {
  sessionCheckpoint_ = captureCheckpoint(worldState_, stage);
  if (autosaveEnabled_) {
    memoryCard_.queueSave(sessionCheckpoint_);
    saveNoticeTicks_ = 300;
  }
}

void GameApp::updateSaveMenu(ScreenState previousScreen) {
  auto& state = worldState_;
  auto& menu = state.menu;
  const auto screen = menu.currentScreen;
  const auto requestScene = [&](CheckpointData checkpoint) {
    pendingLoad_ = checkpoint;
    state.inventory = InventoryState{};
    state.scene.accessTicks = 0;
    state.scene.target = checkpoint.stage == Checkpoint::None ? SceneId::Exterior : SceneId::Interior;
    state.scene.requested = true;
    eventBus_.publish(Events::GameEvent::makeSimple(Events::EventType::PlayerSpawnResetRequested));
  };

  u32 count = 0;
  const auto* events = eventBus_.getFrameEvents(count);
  if (screen == ScreenState::Defeated && sessionCheckpoint_.stage == Checkpoint::WavesCleared) {
    for (u32 i = 0; i < count; ++i) {
      if (events[i].type == Events::EventType::BossRetryRequested) {
        requestScene(sessionCheckpoint_);
        return;
      }
    }
  }

  if (screen != ScreenState::LoadGame && !(screen == ScreenState::NewGame && menu.saveDialog)) {
    pendingSaveMenuAction_ = Events::EventType::None;
    if (screen == ScreenState::MainMenu) pendingLoad_ = {};
    return;
  }

  for (u32 i = 0; i < count; ++i) {
    if (events[i].type == Events::EventType::MenuBackRequested) {
      pendingSaveMenuAction_ = Events::EventType::None;
      return;
    }
  }

  const auto startCampaign = [&](bool autosave, CheckpointData checkpoint) {
    autosaveEnabled_ = autosave;
    memoryCard_.bindSession(autosave);
    lastSaveNotice_ = SaveNotice::None;
    saveNoticeTicks_ = 0;
    sessionCheckpoint_ = checkpoint;
    menu.saveDialog = menu.confirmNewGame = false;
    requestScene(checkpoint);
  };

  if (screen == ScreenState::LoadGame && screen != previousScreen) {
    memoryCard_.refresh();
    menu.saveStatus = memoryCard_.status();
    return;
  }
  if (pendingSaveMenuAction_ == Events::EventType::None) {
    for (u32 i = 0; i < count; ++i) {
      const auto type = events[i].type;
      if (type == Events::EventType::NewCampaignWithoutSaveRequested) {
        startCampaign(false, {});
        return;
      }
      if (type == Events::EventType::CheckCardRequested) {
        memoryCard_.refresh();
        return;
      }
      if (type == Events::EventType::LoadCheckpointRequested ||
          type == Events::EventType::NewCampaignRequested) {
        pendingSaveMenuAction_ = type;
        memoryCard_.refresh();
        menu.saveStatus = SaveStatus::Checking;
        return;
      }
    }
    return;
  }
  if (memoryCard_.busy()) return;

  const bool loading = pendingSaveMenuAction_ == Events::EventType::LoadCheckpointRequested;
  pendingSaveMenuAction_ = Events::EventType::None;
  const auto status = memoryCard_.status();
  menu.saveStatus = status;
  const bool sameRecord = menuCardStatus_ == status && menuMediaEpoch_ == memoryCard_.mediaEpoch() &&
      menuSaveSequence_ == memoryCard_.saved().sequence;
  if (!sameRecord || (loading ? status != SaveStatus::Ready : !canEnableAutosave(status))) {
    menu.confirmNewGame = false;
    menu.saveChoice = 0;
    return;
  }

  startCampaign(true, loading ? memoryCard_.saved().data : CheckpointData{});
}

void GameApp::updateSceneTransition() {
  auto& state = worldState_;
  auto& scene = state.scene;
  if (state.menu.currentScreen == ScreenState::Paused ||
      (state.menu.currentScreen == ScreenState::Settings &&
       state.menu.settingsReturnScreen == ScreenState::Paused)) return;
  if (scene.accessTicks > 0) {
    const u32 elapsed = accessTimer_.getTimeDelta();
    accessTimer_.prime();
    if (elapsed < scene.accessTicks) {
      scene.accessTicks -= elapsed;
    } else {
      scene.accessTicks = 0;
      scene.target = SceneId::Interior;
      scene.requested = true;
    }
  }
  u32 count = 0;
  const auto* events = eventBus_.getFrameEvents(count);
  for (u32 i = 0; i < count; ++i) {
    if (events[i].type == Events::EventType::CardReaderInteracted &&
        scene.active == SceneId::Exterior && state.simulationActive() &&
        state.inventory.canEnter()) {
      scene.accessTicks = menuAudioSystem_.playCardReader(engine_, state.menu.sfxVolume);
      accessTimer_.prime();
      state.cardReader.isUnlocked = true;
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
  state.radio.reviewing = false;
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
    const auto target = scene.target;
    scene = SceneState{};
    scene.active = scene.target = target;
    scene.resident = true;
    scene.presented = false;
    worldState_.intro = SceneIntroState{true, 0};
    const int health = worldState_.playerHealth;
    Systems::Enemy::EnemySystem::reset(worldState_);
    if (scene.active == SceneId::Interior) worldState_.playerHealth = health;
    worldState_.resetPlayerToSpawn();
    if (pendingLoad_.stage != Checkpoint::None) {
      restoreCheckpoint(worldState_, pendingLoad_);
      pendingLoad_ = {};
    } else if (scene.active == SceneId::Interior && worldState_.inventory.canEnter()) {
      queueCheckpoint(Checkpoint::EnteredEtip);
    }
    worldState_.menu.currentScreen = ScreenState::GameplayFps;
    logSceneMemory("scene ready");
    return;
  }
  ++scene.loadStep;
}

}  // namespace Core
}  // namespace DroidCampos
