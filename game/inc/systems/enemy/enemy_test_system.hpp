#pragma once
#include <tyra>
#include <array>
#include <memory>
#include <vector>
#include "core/world_state.hpp"
#include "events/event_bus.hpp"
#include "systems/enemy/enemy_test_motion.hpp"

namespace DroidCampos { namespace Systems { namespace Enemy {
class EnemyTestSystem {
 public:
  void init(Tyra::Engine* engine);
  static bool validateAssets();
  bool loadVariant(Tyra::Engine* engine, int variant);
  void update(const Core::WorldState& state, Events::EventBus& bus);
  void render(Tyra::Engine* engine);
  void unload(Tyra::Engine* engine);
 private:
  struct Actor {
    std::unique_ptr<Tyra::StaticMesh> mesh;
    std::vector<float> frames;
    u32 vertices = 0;
  };
  std::array<Actor, kEnemyVariantCount> actors_;
  Tyra::StaticPipeline pipeline_;
  Tyra::StaPipOptions options_;
  Tyra::Timer timer_;
  u32 ticks_ = 0;
  bool wasActive_ = false;
};
}}}
