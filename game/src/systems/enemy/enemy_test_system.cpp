#include "systems/enemy/enemy_test_system.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <limits>

namespace DroidCampos { namespace Systems { namespace Enemy {
namespace {
const char* variantName(int variant) { return variant == 0 ? "leader" : "guard"; }
std::string pathFor(int variant, const char* file) {
  return Tyra::FileUtils::fromCwd(std::string("enemies/") + variantName(variant) + "/" + file);
}
bool readBank(int variant, std::vector<float>* output) {
  FILE* file = std::fopen(pathFor(variant, "motion.bin").c_str(), "rb");
  if (!file) return false;
  char magic[8]; u32 count = 0, frames = 0;
  bool valid = std::fread(magic, 1, 8, file) == 8 &&
      std::fread(&count, 4, 1, file) == 1 && std::fread(&frames, 4, 1, file) == 1 &&
      std::memcmp(magic, "ETIPANM1", 8) == 0 && frames == kEnemyFrameCount &&
      count == static_cast<u32>((variant == 0 ? 1781 : 1779) * 3);
  if (!valid) { std::fclose(file); return false; }
  const long expected = 16L + static_cast<long>(count) * frames * 12L;
  valid = std::fseek(file, 0, SEEK_END) == 0 && std::ftell(file) == expected;
  if (valid && output) {
    valid = std::fseek(file, 16, SEEK_SET) == 0;
    output->resize(count * frames * 3);
    valid = valid && std::fread(output->data(), sizeof(float), output->size(), file) == output->size();
    if (valid) for (float value : *output)
      if (!std::isfinite(value) || std::fabs(value) >= 3.0F) { valid = false; break; }
    if (!valid) output->clear();
  }
  std::fclose(file); return valid;
}
bool exists(const std::string& path) {
  FILE* file = std::fopen(path.c_str(), "rb");
  if (!file) return false;
  std::fclose(file); return true;
}
}
void EnemyTestSystem::init(Tyra::Engine* engine) {
  pipeline_.setRenderer(&engine->renderer.core);
  options_.frustumCulling = Tyra::PipelineFrustumCulling_Precise;
  options_.fullClipChecks = true;
  options_.textureMappingType = Tyra::TyraNearest;
  options_.transformationType = Tyra::TyraMVP;
  timer_.prime();
}
bool EnemyTestSystem::validateAssets() {
  for (int i = 0; i < kEnemyVariantCount; ++i)
    if (!exists(pathFor(i, "enemy.obj")) || !exists(pathFor(i, "enemy.mtl")) ||
        !exists(pathFor(i, "atlas.png")) || !readBank(i, nullptr)) return false;
  return true;
}
bool EnemyTestSystem::loadVariant(Tyra::Engine* engine, int variant) {
  if (!engine || variant < 0 || variant >= kEnemyVariantCount) return false;
  auto& actor = actors_[variant];
  if (actor.mesh) return false;
  if (!readBank(variant, &actor.frames)) return false;
  Tyra::ObjLoaderOptions options; options.scale = 1; options.flipUVs = true;
  auto data = Tyra::ObjLoader::load(pathFor(variant, "enemy.obj"), options);
  actor.mesh = std::make_unique<Tyra::StaticMesh>(data.get());
  data.reset();
  if (actor.mesh->materials.size() != 1 ||
      actor.mesh->materials[0]->frames[0]->count != static_cast<u32>((variant == 0 ? 1781 : 1779) * 3)) {
    actor.mesh.reset(); actor.frames.clear(); return false;
  }
  auto* material = actor.mesh->materials[0];
  actor.vertices = material->frames[0]->count;
  Tyra::Vec4 minimum(3,3,3,1), maximum(-3,-3,-3,1);
  for (size_t i = 0; i < actor.frames.size(); i += 3) {
    minimum.x = std::min(minimum.x, actor.frames[i]); maximum.x = std::max(maximum.x, actor.frames[i]);
    minimum.y = std::min(minimum.y, actor.frames[i+1]); maximum.y = std::max(maximum.y, actor.frames[i+1]);
    minimum.z = std::min(minimum.z, actor.frames[i+2]); maximum.z = std::max(maximum.z, actor.frames[i+2]);
  }
  const Tyra::Vec4 bounds[] = {minimum, maximum};
  *actor.mesh->frame->bbox = Tyra::BBox(bounds, 2);
  *material->frames[0]->bbox = Tyra::BBox(bounds, 2);
  material->ambient.set(128,128,128,128);
  engine->renderer.getTextureRepository().addByMesh(actor.mesh.get(),
      Tyra::FileUtils::fromCwd(std::string("enemies/") + variantName(variant) + "/"), "png");
  timer_.prime(); wasActive_ = false; ticks_ = 0;
  std::printf("[ENEMY TEST] %s: %u vertices, %u position bytes\n", variantName(variant),
              actor.vertices, static_cast<unsigned>(actor.frames.size() * sizeof(float)));
  return true;
}
void EnemyTestSystem::update(const Core::WorldState& state, Events::EventBus& bus) {
  const u32 elapsed = timer_.getTimeDelta(); timer_.prime();
  u32 eventCount = 0; const auto* events = bus.getFrameEvents(eventCount);
  for (u32 i = 0; i < eventCount; ++i)
    if (events[i].type == Events::EventType::PlayerSpawnResetRequested) { ticks_ = 0; wasActive_ = false; }
  const bool active = state.menu.currentScreen == Core::ScreenState::GameplayFps &&
      state.scene.active == Core::SceneId::Interior && state.scene.resident &&
      state.scene.presented && !state.scene.requested && state.scene.accessFrames == 0 &&
      !state.intro.active && !state.radio.popupActive;
  if (active && wasActive_) ticks_ = (ticks_ + std::min(elapsed, 3907U)) % kEnemyCycleTicks;
  wasActive_ = active;
}
void EnemyTestSystem::render(Tyra::Engine* engine) {
  if (!actors_[0].mesh && !actors_[1].mesh) return;
  // Last frame's DMA must finish before reusing the mutable animation vertices.
  engine->renderer.core.sync.align3D();
  pipeline_.core.clearGeometryCache();
  engine->renderer.renderer3D.usePipeline(pipeline_);
  for (int i = 0; i < kEnemyVariantCount; ++i) {
    auto& actor = actors_[i]; if (!actor.mesh) continue;
    const auto pose = sampleEnemyTest(ticks_, i);
    auto* vertices = actor.mesh->materials[0]->frames[0]->vertices;
    const float* from = actor.frames.data() + pose.from * actor.vertices * 3;
    const float* to = actor.frames.data() + pose.to * actor.vertices * 3;
    for (u32 v = 0; v < actor.vertices; ++v)
      vertices[v].set(from[v*3] + (to[v*3]-from[v*3])*pose.blend,
                      from[v*3+1] + (to[v*3+1]-from[v*3+1])*pose.blend,
                      from[v*3+2] + (to[v*3+2]-from[v*3+2])*pose.blend, 1);
    actor.mesh->rotation.identity(); actor.mesh->rotation.rotateY(pose.yaw);
    actor.mesh->translation.identity();
    actor.mesh->translation.translate(Tyra::Vec4(pose.x, .02F, pose.z, 1));
    pipeline_.render(actor.mesh.get(), options_);
  }
}
void EnemyTestSystem::unload(Tyra::Engine* engine) {
  if (engine) engine->renderer.core.sync.align3D();
  pipeline_.core.clearGeometryCache();
  for (auto& actor : actors_) {
    if (engine && actor.mesh) engine->renderer.getTextureRepository().freeByMesh(actor.mesh.get());
    actor.mesh.reset(); std::vector<float>().swap(actor.frames); actor.vertices = 0;
  }
  ticks_ = 0; wasActive_ = false; timer_.prime();
}
}}}
