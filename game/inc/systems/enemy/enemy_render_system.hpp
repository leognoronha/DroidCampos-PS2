#pragma once
#include <array>
#include <memory>
#include <vector>
#include "systems/enemy/enemy_animation.hpp"
namespace DroidCampos { namespace Systems { namespace Enemy {
class EnemyRenderSystem {
 public:
  void init(Tyra::Engine*);
  static bool validateAssets();
  bool loadSlot(Tyra::Engine*,int slot);
  void render(Tyra::Engine*,const Core::WorldState&);
  void unload(Tyra::Engine*);
 private:
  struct Actor {std::unique_ptr<Tyra::StaticMesh> mesh;u32 vertices=0;};
  std::array<Actor,Core::kMaxEnemies> actors_;
  std::array<std::vector<float>,kEnemyVariantCount> banks_;
  std::array<u32,kEnemyVariantCount> textures_{};
  Tyra::StaticPipeline pipeline_;
  Tyra::StaPipOptions options_;
};
}}}
