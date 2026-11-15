#include "systems/render3d/world_render_system.hpp"
#include <cmath>
#include <cstdio>
#include <string>
#include "core/interior_layout.hpp"
#include "systems/render3d/spatial_order.hpp"

namespace DroidCampos {
namespace Systems {
namespace Render3D {

using namespace Tyra;

WorldRenderSystem::WorldRenderSystem()
    : etipLogoMat_(nullptr),
      etipBoardMat_(nullptr),
      cardReaderLightMat_(nullptr),
      scenarioLoaded_(false),
      mistLoaded_(false),
      pistolLoaded_(false) {}

void WorldRenderSystem::init(Engine* engine, Core::WorldState& state,
                             Events::EventBus& bus) {
  if (engine == nullptr) {
    return;
  }

  stapip_.setRenderer(&engine->renderer.core);
  enemyRenderer_.init(engine);

  scenarioOptions_.frustumCulling = PipelineFrustumCulling_Precise;
  scenarioOptions_.fullClipChecks = true;
  scenarioOptions_.textureMappingType = TyraNearest;
  scenarioOptions_.transformationType = TyraMVP;
  capsuleGlassOptions_ = scenarioOptions_;
  capsuleGlassOptions_.blendingEnabled = true;
  capsuleGlassOptions_.zTestType = PipelineZTest_Standard;
  loadPistol3D(engine);
  loadWeaponEffects();
  updatePistolViewmodelTransform(state);
  updateAbandonedSignFlicker(state);
  updateCardReaderLight(state, bus);
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

  etipLogoMat_ = nullptr;
  etipBoardMat_ = nullptr;
  cardReaderLightMat_ = nullptr;
  for (auto* mat : scenarioMesh_->materials) {
    mat->ambient.set(128.0F, 128.0F, 128.0F, 128.0F);
    if (mat->name == "PS2_Mat_Logo_ETIP") {
      etipLogoMat_ = mat;
    } else if (mat->name == "PS2_Mat_PlacaCyber") {
      etipBoardMat_ = mat;
    } else if (mat->name == "PS2_Mat_Luz_Leitor") {
      cardReaderLightMat_ = mat;
      cardReaderLightMat_->ambient.set(220.0F, 18.0F, 18.0F, 128.0F);
    }
  }

  scenarioOptions_.frustumCulling = PipelineFrustumCulling_Precise;
  scenarioOptions_.fullClipChecks = true;
  scenarioOptions_.textureMappingType = TyraNearest;
  scenarioOptions_.transformationType = TyraMVP;

  prepareOpaqueMesh(scenarioMesh_.get(), repo, stapip_);
  registerWeaponGeometry(scenarioMesh_.get());
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
    // Light, delicate ground-hugging mist that preserves full map visibility
    mat->ambient.set(120.0F, 126.0F, 134.0F, 38.0F);
  }

  // Render in back-to-front order with precise frustum/near-plane clipping & alpha blending
  mistOptions_.transformationType = TyraMVP;
  mistOptions_.frustumCulling = PipelineFrustumCulling_Precise;
  mistOptions_.fullClipChecks = true;
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
  const float recoil =
      static_cast<float>(state.weapon.beamTicks) / Core::kBeamTicks;
  const float reload =
      state.weapon.reloadTicks
          ? std::sin((1.0F - static_cast<float>(state.weapon.reloadTicks) /
                                 Core::kReloadTicks) *
                     3.14159265F)
          : 0;

  // Lissajous figure-8 walking bob + idle breathing + camera look inertia sway
  const float idleBreath = std::sin(state.anim.animTick * 0.75F) * 0.0035F;
  const float bobX = std::cos(p.walkBobPhase) * 0.015F * p.walkBobAmount;
  const float bobY =
      -std::fabs(std::sin(p.walkBobPhase)) * 0.018F * p.walkBobAmount;
  const float bobZ = std::sin(p.walkBobPhase * 2.0F) * 0.008F * p.walkBobAmount;

  pistolMesh_->translation.identity();
  pistolMesh_->rotation.identity();
  pistolMesh_->scale.identity();

  // Position in lower-right FPS hand (View Space: +X right, -Y down, -Z
  // forward)
  pistolMesh_->translation.translate(
      Vec4(0.145F + bobX + p.weaponSwayX,
           -0.135F + bobY + p.weaponSwayY + idleBreath - reload * .12F,
           -0.320F + bobZ + recoil * .018F, 1.0F));

  // Slight natural inward angle toward center crosshair + dynamic sway tilt
  pistolMesh_->rotation.rotateY(0.11F - p.weaponSwayX * 1.8F);
  pistolMesh_->rotation.rotateX(0.03F + p.weaponSwayY * 1.6F + bobY * 0.9F +
                                recoil * .045F + reload * .25F);
  pistolMesh_->rotation.rotateZ(-bobX * 1.4F - p.weaponSwayX * 1.2F);
}

void WorldRenderSystem::registerWeaponGeometry(StaticMesh* mesh, int door) {
  if (!mesh) return;
  for (const auto* mat : mesh->materials) {
    if (mat->frames.empty()) continue;
    const auto* f = mat->frames[0];
    weaponRays_.add(f->vertices, f->count, door);
  }
}

Vec4 WorldRenderSystem::weaponMuzzle(const Core::WorldState& state) {
  using namespace Weapon;
  updatePistolViewmodelTransform(state);
  // Barrel end in pistol_scifi.obj coordinates; exactly the viewmodel
  // transform.
  Vec4 view(.145F, -.135F, -.5095F, 1);
  if (pistolMesh_)
    view = pistolMesh_->getModelMatrix() * Vec4(0, .034F, -.1895F, 1);
  const auto forward =
      unit(sub(state.player.cameraLookAt, state.player.cameraPos));
  const auto right = unit(cross(forward, Vec4(0, 1, 0, 0)));
  const auto up = unit(cross(right, forward));
  return add(
      state.player.cameraPos,
      add(mul(right, view.x), add(mul(up, view.y), mul(forward, -view.z))));
}

void WorldRenderSystem::loadWeaponEffects() {
  ObjLoaderOptions options;
  options.scale = 1;
  options.flipUVs = false;
  auto beam =
      ObjLoader::load(FileUtils::fromCwd("weapon/laser_beam.obj"), options);
  beamCore_ = std::make_unique<StaticMesh>(beam.get());
  // Separate loads preserve ownership of the immutable vertex arrays.
  beam.reset();
  beam = ObjLoader::load(FileUtils::fromCwd("weapon/laser_beam.obj"), options);
  beamGlow_ = std::make_unique<StaticMesh>(beam.get());
  auto impact =
      ObjLoader::load(FileUtils::fromCwd("weapon/laser_impact.obj"), options);
  impactMesh_ = std::make_unique<StaticMesh>(impact.get());
  laserOptions_ = scenarioOptions_;
  laserOptions_.blendingEnabled = true;
  laserOptions_.zTestType = PipelineZTest_Standard;
}

void WorldRenderSystem::renderWeaponEffects(Engine* engine,
                                            const Core::WorldState& state) {
  renderOneWeaponEffect(engine,state.weapon);
  for(const auto& enemy:state.enemies)
    if(enemy.active)renderOneWeaponEffect(engine,enemy.weapon);
}
void WorldRenderSystem::renderOneWeaponEffect(Engine* engine,const Core::WeaponState& w) {
  using namespace Weapon;
  if (!beamCore_ || !beamGlow_ || !impactMesh_) return;
  // The unit geometry never changes: model transforms keep Tyra's bbox cache
  // valid.
  auto pose = [](StaticMesh* mesh, const Vec4& position, const Vec4& x,
                 const Vec4& y, const Vec4& z, float sx, float sy, float sz,
                 const Color& color) {
    mesh->translation.identity();
    mesh->translation.translate(position);
    mesh->rotation.identity();
    mesh->scale.identity();
    mesh->rotation.data[0] = x.x;
    mesh->rotation.data[1] = x.y;
    mesh->rotation.data[2] = x.z;
    mesh->rotation.data[4] = y.x;
    mesh->rotation.data[5] = y.y;
    mesh->rotation.data[6] = y.z;
    mesh->rotation.data[8] = z.x;
    mesh->rotation.data[9] = z.y;
    mesh->rotation.data[10] = z.z;
    mesh->scale.scale(Vec4(sx, sy, sz, 1));
    for (auto* mat : mesh->materials)
      mat->ambient.set(color.r, color.g, color.b, color.a);
  };
  if (w.beamTicks) {
    const auto segment = sub(w.beamEnd, w.beamStart);
    const float length = std::sqrt(dot(segment, segment));
    if (length > .001F) {
      const auto forward = unit(segment);
      const auto axis =
          std::fabs(forward.y) < .95F ? Vec4(0, 1, 0, 0) : Vec4(1, 0, 0, 0);
      const auto right = unit(cross(axis, forward));
      const auto up = unit(cross(forward, right));
      pose(beamGlow_.get(), w.beamStart, right, up, forward, .018F, .018F,
           length, Color(0, 100, 128, 44));
      pose(beamCore_.get(), w.beamStart, right, up, forward, .004F, .004F,
           length, Color(100, 128, 128, 128));
      renderTransparentPass(engine, beamGlow_.get(), laserOptions_);
      renderTransparentPass(engine, beamCore_.get(), laserOptions_);
    }
  }
  if (w.impact && w.impactTicks) {
    const auto normal = unit(w.impactNormal);
    const auto axis =
        std::fabs(normal.y) < .95F ? Vec4(0, 1, 0, 0) : Vec4(1, 0, 0, 0);
    const auto right = unit(cross(axis, normal));
    const auto up = unit(cross(normal, right));
    const auto position = add(w.beamEnd, mul(normal, .006F));
    const float radius =
        .025F + .035F * static_cast<float>(w.impactTicks) / Core::kImpactTicks;
    pose(impactMesh_.get(), position, right, up, normal, radius, radius, 1,
         Color(30, 128, 128, 100));
    renderTransparentPass(engine, impactMesh_.get(), laserOptions_);
  }
}

void WorldRenderSystem::updateAbandonedSignFlicker(
    const Core::WorldState& state) {
  if (!scenarioLoaded_ || scenarioMesh_ == nullptr) {
    return;
  }

  // Deterministic pseudo-random fluorescent/neon ballast flicker pattern
  const float t = state.anim.animTick * 0.65F;
  const float waveA = std::sin(t * 17.3F);
  const float waveB = std::sin(t * 41.7F);
  const float waveC = std::cos(t * 7.9F);
  const float cycle = std::fmod(t, 6.2831853F);

  float intensity = 1.0F;
  if (cycle > 1.20F && cycle < 1.36F) {
    // Quick double-drop short-circuit stutter
    intensity = (waveB > 0.0F) ? 0.16F : 0.72F;
  } else if (cycle > 3.80F && cycle < 4.35F) {
    // Prolonged dying-ballast brownout with high-frequency buzz
    intensity = (waveA * waveB > 0.12F) ? 0.22F : 0.45F;
  } else if (cycle > 5.45F && cycle < 5.56F) {
    // Sudden voltage spike before brief blackout
    intensity = (cycle < 5.50F) ? 1.22F : 0.12F;
  } else {
    // Subtle 60Hz fluorescent hum when lit
    intensity = 0.94F + 0.06F * waveC;
  }

  if (etipLogoMat_ != nullptr) {
    const float r = 128.0F * intensity;
    const float g = 124.0F * intensity;
    const float b = 114.0F * intensity;
    etipLogoMat_->ambient.set(r, g, b, 128.0F);
  }
  if (etipBoardMat_ != nullptr) {
    const float boardInt = 0.45F + 0.55F * intensity;
    etipBoardMat_->ambient.set(120.0F * boardInt, 124.0F * boardInt,
                               128.0F * boardInt, 128.0F);
  }
}

void WorldRenderSystem::updateCardReaderLight(Core::WorldState& state,
                                              Events::EventBus& bus) {
  u32 eventCount = 0;
  const Events::GameEvent* events = bus.getFrameEvents(eventCount);
  for (u32 i = 0; i < eventCount; ++i) {
    const auto& ev = events[i];
    if (ev.type == Events::EventType::CardReaderInteracted) {
      state.cardReader.isUnlocked = true;
    } else if (ev.type == Events::EventType::PlayerSpawnResetRequested) {
      state.cardReader.isUnlocked = false;
    }
  }

  if (cardReaderLightMat_ == nullptr) {
    return;
  }

  if (state.cardReader.isUnlocked) {
    // Bright Emerald Green LED when interacted via Square button
    cardReaderLightMat_->ambient.set(18.0F, 235.0F, 48.0F, 128.0F);
  } else {
    // Glowing Crimson Red LED before interaction
    const float pulse = 0.92F + 0.08F * std::sin(state.anim.animTick * 3.5F);
    cardReaderLightMat_->ambient.set(225.0F * pulse, 18.0F, 18.0F, 128.0F);
  }
}

void WorldRenderSystem::renderTransparentPass(Engine* engine, StaticMesh* mesh,
                                              const StaPipOptions& options) {
  if (!stapip_.isVisible(mesh)) return;
  auto& core = engine->renderer.core;
  const bool previousWriteEnabled = core.gs.zBuffer.mask == 0;
  // Drain PATH1 before updating ZBUF on PATH3. Drain PATH3 before sending
  // subsequent PATH1 geometry, so neither pass can overtake the state change.
  core.sync.align3D();
  core.gs.setZBufferWriteEnabled(false);
  core.sync.align2D();
  stapip_.render(mesh, options);
  core.sync.align3D();
  core.gs.setZBufferWriteEnabled(previousWriteEnabled);
  core.sync.align2D();
}

void WorldRenderSystem::executeStage(Core::HardwareStage stage, Engine* engine,
                                     Core::WorldState& state,
                                     Events::EventBus& bus,
                                     Hardware::Ps2ScratchpadArena& scratchpad) {
  (void)scratchpad;


  if (state.menu.currentScreen != Core::ScreenState::GameplayFps) {
    return;
  }

  if (stage == Core::HardwareStage::Stage1_EeVu0Simulation) {
    updatePistolViewmodelTransform(state);
    updateAbandonedSignFlicker(state);
    updateCardReaderLight(state, bus);
    return;
  }

  if (stage == Core::HardwareStage::Stage2_Vu1GsRender3D) {
    if (engine == nullptr) {
      return;
    }

    engine->renderer.renderer3D.usePipeline(stapip_);

    for (auto& mesh : interiorMeshes_) stapip_.render(mesh.get(), scenarioOptions_);
    if (doorMesh_) {
      for (int i = 0; i < 4; ++i) {
        doorMesh_->translation.identity();
        doorMesh_->translation.translate(Vec4(Core::kInsideDoorX[i], 1.1F,
            Core::kInsideDoorZ[i] - state.scene.doors[i].openAmount * Core::kDoorTravel, 1.0F));
        stapip_.render(doorMesh_.get(), scenarioOptions_);
      }
    }

    if (scenarioLoaded_ && scenarioMesh_ != nullptr) {
      stapip_.render(scenarioMesh_.get(), scenarioOptions_);
    }

    if (mistLoaded_ && mistMesh_ != nullptr) {
      renderTransparentPass(engine, mistMesh_.get(), mistOptions_);
    }

    enemyRenderer_.render(engine, state);
    engine->renderer.renderer3D.usePipeline(stapip_);

    if (capsuleGlassMesh_) {
      renderTransparentPass(engine, capsuleGlassMesh_.get(), capsuleGlassOptions_);
    }

    renderWeaponEffects(engine, state);

    if (pistolLoaded_ && pistolMesh_ != nullptr) {
      stapip_.render(pistolMesh_.get(), pistolOptions_);
    }
  }
}

void WorldRenderSystem::shutdown(Engine* engine) {
  unloadScene(engine);
  if (engine != nullptr) {
    auto& repo = engine->renderer.getTextureRepository();
    if (pistolMesh_ != nullptr) {
      repo.freeByMesh(pistolMesh_.get());
    }
  }

  scenarioMesh_.reset();
  mistMesh_.reset();
  pistolMesh_.reset();
  beamCore_.reset(); beamGlow_.reset(); impactMesh_.reset();
  etipLogoMat_ = nullptr;
  etipBoardMat_ = nullptr;
  cardReaderLightMat_ = nullptr;
  scenarioLoaded_ = false;
  mistLoaded_ = false;
  pistolLoaded_ = false;
}

bool WorldRenderSystem::validateScene(Core::SceneId scene) const {
  if (scene == Core::SceneId::Interior && !Enemy::EnemyRenderSystem::validateAssets()) return false;
  // Check OBJ, its MTL and every referenced PNG before destroying the old scene.
  std::vector<std::string> bases;
  if (scene == Core::SceneId::Exterior) bases = {"etip/etip_cenario", "etip/etip_nevoa"};
  else for (const auto* part : Core::kInsideParts) bases.push_back(std::string("etip_inside/") + part);
  for (const auto& base : bases) {
    for (const char* ext : {".obj", ".mtl"}) {
      FILE* f = std::fopen(FileUtils::fromCwd(base + ext).c_str(), "rb");
      if (!f) return false;
      std::fclose(f);
    }
    FILE* mtl = std::fopen(FileUtils::fromCwd(base + ".mtl").c_str(), "rb");
    if (!mtl) return false;
    char line[256], name[192];
    bool valid = true;
    while (std::fgets(line, sizeof(line), mtl)) {
      if (std::sscanf(line, "map_Kd %191s", name) == 1) {
        const std::string path = base.substr(0, base.find_last_of('/') + 1) + name;
        FILE* png = std::fopen(FileUtils::fromCwd(path).c_str(), "rb");
        if (!png) { valid = false; break; }
        std::fclose(png);
      }
    }
    std::fclose(mtl);
    if (!valid) return false;
  }
  return true;
}

void WorldRenderSystem::unloadScene(Engine* engine) {
  if (!engine) return;
  // New scene allocations may reuse the same vertex addresses. Do not keep
  // address-keyed bounding boxes from the previously unloaded geometry.
  stapip_.core.clearGeometryCache();
  weaponRays_.clear();
  auto& repo = engine->renderer.getTextureRepository();
  if (scenarioMesh_) repo.freeByMesh(scenarioMesh_.get());
  if (mistMesh_) repo.freeByMesh(mistMesh_.get());
  scenarioMesh_.reset(); mistMesh_.reset();
  enemyRenderer_.unload(engine);
  // Each interior PNG is owned once, regardless of how many material links use it.
  for (const auto& entry : interiorTextures_) repo.free(entry.second);
  interiorTextures_.clear(); interiorMeshes_.clear(); doorMesh_.reset();
  capsuleGlassMesh_.reset();
  etipLogoMat_ = etipBoardMat_ = cardReaderLightMat_ = nullptr;
  scenarioLoaded_ = mistLoaded_ = false;
}

bool WorldRenderSystem::loadSceneStep(Engine* engine, Core::SceneId scene, int step) {
  if (!engine || step < 0) return false;
  if (scene == Core::SceneId::Exterior) {
    if (step == 0) loadScenario3D(engine);
    else loadMist3D(engine);
    return true;
  }
  if (step >= Core::kInsideLoadStepCount) return false;
  if (step >= Core::kInsidePartCount)
    return enemyRenderer_.loadSlot(engine, step - Core::kInsidePartCount);
  const std::string base = std::string("etip_inside/") + Core::kInsideParts[step];
  ObjLoaderOptions options; options.scale = 1.0F; options.flipUVs = true;
  auto data = ObjLoader::load(FileUtils::fromCwd((base + ".obj").c_str()), options);
  auto mesh = std::make_unique<StaticMesh>(data.get());
  data.reset(); // Mesh owns the vertex arrays; release builder metadata now.
  auto& repo = engine->renderer.getTextureRepository();
  for (auto* mat : mesh->materials) {
    const bool luminous = mat->name == "PS2_Mat_Inside_Luz" ||
        mat->name == "PS2_Mat_Inside_Emergencia" || mat->name == "PS2_Mat_Inside_Terminal";
    const bool capsuleMaterial = mat->name.find("PS2_Mat_Capsule_") == 0;
    if (capsuleMaterial) {
      const bool bright = mat->name.find("Verde_Energia") != std::string::npos ||
          mat->name.find("Led_Alerta") != std::string::npos ||
          mat->name.find("Vidro_Contencao") != std::string::npos;
      const float intensity = bright ? 1.0F : 0.75F;
      mat->ambient.set(mat->ambient.r * intensity, mat->ambient.g * intensity,
                       mat->ambient.b * intensity, 128);
    } else {
      const float tint = luminous ? 128.0F : 72.0F;
      mat->ambient.set(tint, tint, tint, 128);
    }
    if (!mat->textureName.has_value()) continue;
    const std::string name = mat->textureName.value();
    auto it = interiorTextures_.find(name);
    Texture* tex;
    if (it == interiorTextures_.end()) {
      tex = repo.add(FileUtils::fromCwd(("etip_inside/" + name + ".png").c_str()));
      interiorTextures_[name] = tex->id;
    } else tex = repo.getByTextureId(it->second);
    tex->addLink(mat->id);
  }
  if (std::string(Core::kInsideParts[step]) != "capsula_vidro")
    prepareOpaqueMesh(mesh.get(), repo, stapip_);
  if (step == Core::kInsidePartCount - 1) {
    for(int door=0;door<4;++door) registerWeaponGeometry(mesh.get(),door);
  } else registerWeaponGeometry(mesh.get());
  if (std::string(Core::kInsideParts[step]) == "capsula_vidro") capsuleGlassMesh_ = std::move(mesh);
  else if (step == Core::kInsidePartCount - 1) doorMesh_ = std::move(mesh);
  else interiorMeshes_.push_back(std::move(mesh));
  return true;
}

}  // namespace Render3D
}  // namespace Systems
}  // namespace DroidCampos
