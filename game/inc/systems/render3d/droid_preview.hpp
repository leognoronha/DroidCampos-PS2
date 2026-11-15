#pragma once
#include <tyra>
#include <memory>
#include <vector>
#include "core/world_state.hpp"
namespace DroidCampos { namespace Systems { namespace Render3D {
class DroidPreview {
 public:
  void init(Tyra::Engine*);
  static bool validateAssets();
  bool load(Tyra::Engine*);
  void render(Tyra::Engine*,const Core::WorldState&);
  void unload(Tyra::Engine*);
 private:
  std::unique_ptr<Tyra::StaticMesh> mesh_,button_;
  std::vector<float> bank_;
  Tyra::StaticPipeline pipeline_;
  Tyra::StaPipOptions options_;
};
}}}
