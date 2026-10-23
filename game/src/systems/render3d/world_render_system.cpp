#include "systems/render3d/world_render_system.hpp"
#include <cmath>

namespace DroidCampos {
namespace Systems {
namespace Render3D {

using namespace Tyra;

WorldRenderSystem::WorldRenderSystem()
    : scenarioLoaded_(false), mistLoaded_(false), pistolLoaded_(false) {}

void WorldRenderSystem::init(Engine* engine, Core::WorldState& state,
                             Events::EventBus& bus) {
  (void)bus;
  if (engine == nullptr) {
    return;
  }

  stapip_.setRenderer(&engine->renderer.core);

  loadScenario3D(engine);
  loadMist3D(engine);
  loadPistol3D(engine);
  updatePistolViewmodelTransform(state);
}

void WorldRenderSystem::loadScenario3D(Engine* engine) {
  ObjLoaderOptions options;
  options.scale = 1.0F;
  options.flipUVs = true;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("etip/etip_cenario.obj"), options);
  scenarioMesh_ = std::make_unique<StaticMesh>(data.get());

  auto& repo = engine->renderer.getTextureRepository();
  repo.addByMesh(scenarioMesh_.get(), FileUtils::fromCwd("etip/"), "png");

  for (auto* mat : scenarioMesh_->materials) {
    mat->ambient.set(128.0F, 128.0F, 128.0F, 128.0F);
  }

  scenarioOptions_.frustumCulling = PipelineFrustumCulling_Precise;
  scenarioOptions_.fullClipChecks = true;
  scenarioOptions_.textureMappingType = TyraNearest;
  scenarioOptions_.transformationType = TyraMVP;

  scenarioLoaded_ = true;
  TYRA_LOG("ETIP 3D Scenario loaded successfully!");
}

void WorldRenderSystem::loadMist3D(Engine* engine) {
  ObjLoaderOptions options;
  options.scale = 1.0F;
  options.flipUVs = true;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("etip/etip_nevoa.obj"), options);
  mistMesh_ = std::make_unique<StaticMesh>(data.get());

  auto& repo = engine->renderer.getTextureRepository();
  repo.addByMesh(mistMesh_.get(), FileUtils::fromCwd("etip/"), "png");

  for (auto* mat : mistMesh_->materials) {
    // Soft translucent cool mist color modulated with 32-bit RGBA texture alpha
    mat->ambient.set(118.0F, 126.0F, 134.0F, 88.0F);
  }

  // Render in strict back-to-front order with bilinear filtering & alpha blending
  mistOptions_.transformationType = TyraMVP;
  mistOptions_.frustumCulling = PipelineFrustumCulling_None;
  mistOptions_.fullClipChecks = false;
  mistOptions_.textureMappingType = TyraLinear;
  mistOptions_.blendingEnabled = true;
  mistOptions_.zTestType = PipelineZTest_Standard;

  mistLoaded_ = true;
  TYRA_LOG("Blender 3D Atmospheric Mist loaded successfully!");
}

void WorldRenderSystem::loadPistol3D(Engine* engine) {
  ObjLoaderOptions options;
  options.scale = 1.0F;
  options.flipUVs = true;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("pistol/pistol_scifi.obj"), options);
  pistolMesh_ = std::make_unique<StaticMesh>(data.get());

  auto& repo = engine->renderer.getTextureRepository();
  repo.addByMesh(pistolMesh_.get(), FileUtils::fromCwd("pistol/"), "png");

  for (auto* mat : pistolMesh_->materials) {
    mat->ambient.set(126.0F, 128.0F, 132.0F, 128.0F);
  }

  // Render pistol in Camera/View space (TyraMP) so it stays locked to FPS hand
  pistolOptions_.transformationType = TyraMP;
  pistolOptions_.frustumCulling = PipelineFrustumCulling_None;
  pistolOptions_.fullClipChecks = false;
  pistolOptions_.textureMappingType = TyraNearest;
  pistolOptions_.zTestType = PipelineZTest_Standard;

  pistolLoaded_ = true;
  TYRA_LOG("Sci-Fi Low-Poly Pistol loaded successfully!");
}

void WorldRenderSystem::updatePistolViewmodelTransform(
    const Core::WorldState& state) {
  if (!pistolLoaded_ || pistolMesh_ == nullptr) {
    return;
  }

  const auto& p = state.player;

  // Lissajous figure-8 walking bob + idle breathing + camera look inertia sway
  const float idleBreath = std::sin(state.anim.animTick * 0.75F) * 0.0035F;
  const float bobX = std::cos(p.walkBobPhase) * 0.015F * p.walkBobAmount;
  const float bobY =
      -std::fabs(std::sin(p.walkBobPhase)) * 0.018F * p.walkBobAmount;
  const float bobZ = std::sin(p.walkBobPhase * 2.0F) * 0.008F * p.walkBobAmount;

  pistolMesh_->translation.identity();
  pistolMesh_->rotation.identity();
  pistolMesh_->scale.identity();

  // Position in lower-right FPS hand (View Space: +X right, -Y down, -Z forward)
  pistolMesh_->translation.translate(Vec4(
      0.145F + bobX + p.weaponSwayX,
      -0.135F + bobY + p.weaponSwayY + idleBreath, -0.320F + bobZ, 1.0F));

  // Slight natural inward angle toward center crosshair + dynamic sway tilt
  pistolMesh_->rotation.rotateY(0.11F - p.weaponSwayX * 1.8F);
  pistolMesh_->rotation.rotateX(0.03F + p.weaponSwayY * 1.6F + bobY * 0.9F);
  pistolMesh_->rotation.rotateZ(-bobX * 1.4F - p.weaponSwayX * 1.2F);
}

void WorldRenderSystem::executeStage(Core::HardwareStage stage, Engine* engine,
                                     Core::WorldState& state,
                                     Events::EventBus& bus,
                                     Hardware::Ps2ScratchpadArena& scratchpad) {
  (void)bus;
  (void)scratchpad;

  if (state.menu.currentScreen != Core::ScreenState::GameplayFps) {
    return;
  }

  if (stage == Core::HardwareStage::Stage1_EeVu0Simulation) {
    updatePistolViewmodelTransform(state);
    return;
  }

  if (stage == Core::HardwareStage::Stage2_Vu1GsRender3D) {
    if (engine == nullptr) {
      return;
    }

    engine->renderer.renderer3D.usePipeline(stapip_);

    if (scenarioLoaded_ && scenarioMesh_ != nullptr) {
      stapip_.render(scenarioMesh_.get(), scenarioOptions_);
    }

    if (mistLoaded_ && mistMesh_ != nullptr) {
      stapip_.render(mistMesh_.get(), mistOptions_);
    }

    if (pistolLoaded_ && pistolMesh_ != nullptr) {
      stapip_.render(pistolMesh_.get(), pistolOptions_);
    }
  }
}

void WorldRenderSystem::shutdown(Engine* engine) {
  if (engine != nullptr) {
    auto& repo = engine->renderer.getTextureRepository();
    if (scenarioMesh_ != nullptr) {
      repo.freeByMesh(scenarioMesh_.get());
    }
    if (mistMesh_ != nullptr) {
      repo.freeByMesh(mistMesh_.get());
    }
    if (pistolMesh_ != nullptr) {
      repo.freeByMesh(pistolMesh_.get());
    }
  }

  scenarioMesh_.reset();
  mistMesh_.reset();
  pistolMesh_.reset();
  scenarioLoaded_ = false;
  mistLoaded_ = false;
  pistolLoaded_ = false;
}

}  // namespace Render3D
}  // namespace Systems
}  // namespace DroidCampos
