#include "systems/enemy/enemy_render_system.hpp"
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
void EnemyRenderSystem::init(Tyra::Engine* engine) {
  pipeline_.setRenderer(&engine->renderer.core);
  options_.frustumCulling=Tyra::PipelineFrustumCulling_Precise;
  options_.fullClipChecks=true;options_.textureMappingType=Tyra::TyraNearest;
  options_.transformationType=Tyra::TyraMVP;
}
bool EnemyRenderSystem::validateAssets() {
  for(int v=0;v<kEnemyVariantCount;++v)
    if(!exists(pathFor(v,"enemy.obj")) || !exists(pathFor(v,"enemy.mtl")) ||
       !exists(pathFor(v,"atlas.png")) || !readBank(v,nullptr))return false;
  return true;
}
bool EnemyRenderSystem::loadSlot(Tyra::Engine* engine,int slot) {
  if(!engine || slot<0 || slot>=Core::kMaxEnemies || actors_[slot].mesh)return false;
  int variant=slot%2;
  if(banks_[variant].empty() && !readBank(variant,&banks_[variant]))return false;
  auto& actor=actors_[slot];
  Tyra::ObjLoaderOptions options;options.scale=1;options.flipUVs=true;
  auto data=Tyra::ObjLoader::load(pathFor(variant,"enemy.obj"),options);
  actor.mesh=std::make_unique<Tyra::StaticMesh>(data.get());data.reset();
  if(actor.mesh->materials.size()!=1 || actor.mesh->materials[0]->frames[0]->count!=
      static_cast<u32>((variant==0?1781:1779)*3)) {actor.mesh.reset();return false;}
  auto* material=actor.mesh->materials[0];actor.vertices=material->frames[0]->count;
  Tyra::Vec4 minimum(3,3,3,1),maximum(-3,-3,-3,1);
  const auto& frames=banks_[variant];
  for(size_t i=0;i<frames.size();i+=3) {
    minimum.x=std::min(minimum.x,frames[i]);maximum.x=std::max(maximum.x,frames[i]);
    minimum.y=std::min(minimum.y,frames[i+1]);maximum.y=std::max(maximum.y,frames[i+1]);
    minimum.z=std::min(minimum.z,frames[i+2]);maximum.z=std::max(maximum.z,frames[i+2]);
  }
  const Tyra::Vec4 bounds[]={minimum,maximum};
  *actor.mesh->frame->bbox=Tyra::BBox(bounds,2);
  *material->frames[0]->bbox=Tyra::BBox(bounds,2);
  auto& repo=engine->renderer.getTextureRepository();
  if(!textures_[variant])textures_[variant]=repo.add(
      Tyra::FileUtils::fromCwd(std::string("enemies/")+variantName(variant)+"/atlas.png"))->id;
  repo.getByTextureId(textures_[variant])->addLink(material->id);
  std::printf("[ENEMY] slot=%d variant=%d vertices=%u shared-bank=%u bytes\n",
      slot,variant,actor.vertices,static_cast<unsigned>(frames.size()*sizeof(float)));
  return true;
}
void EnemyRenderSystem::render(Tyra::Engine* engine,const Core::WorldState& s) {
  bool visibleActor=false;
  for(int i=0;i<Core::kMaxEnemies;++i)if(actors_[i].mesh && s.enemies[i].active)visibleActor=true;
  if(!visibleActor)return;
  // Each actor owns mutable geometry. Drain the previous frame before touching it.
  engine->renderer.core.sync.align3D();pipeline_.core.clearGeometryCache();
  engine->renderer.renderer3D.usePipeline(pipeline_);
  for(int i=0;i<Core::kMaxEnemies;++i) {
    auto& actor=actors_[i];const auto& enemy=s.enemies[i];
    if(!actor.mesh || !enemy.active)continue;
    const auto pose=enemyPose(enemy);const auto& frames=banks_[i%2];
    auto* vertices=actor.mesh->materials[0]->frames[0]->vertices;
    const float* from=frames.data()+pose.from*actor.vertices*3;
    const float* to=frames.data()+pose.to*actor.vertices*3;
    for(u32 v=0;v<actor.vertices;++v)
      vertices[v].set(from[v*3]+(to[v*3]-from[v*3])*pose.blend,
          from[v*3+1]+(to[v*3+1]-from[v*3+1])*pose.blend,
          from[v*3+2]+(to[v*3+2]-from[v*3+2])*pose.blend,1);
    auto* mat=actor.mesh->materials[0];
    mat->ambient.set(128,enemy.hitTicks?55:128,enemy.hitTicks?55:128,128);
    actor.mesh->rotation.identity();actor.mesh->rotation.rotateY(enemy.yaw);
    actor.mesh->translation.identity();actor.mesh->translation.translate(enemy.position);
    pipeline_.render(actor.mesh.get(),options_);
  }
}
void EnemyRenderSystem::unload(Tyra::Engine* engine) {
  if(engine)engine->renderer.core.sync.align3D();
  pipeline_.core.clearGeometryCache();
  for(auto& actor:actors_){actor.mesh.reset();actor.vertices=0;}
  for(int v=0;v<kEnemyVariantCount;++v) {
    if(engine && textures_[v])engine->renderer.getTextureRepository().free(textures_[v]);
    textures_[v]=0;std::vector<float>().swap(banks_[v]);
  }
}
}}}
