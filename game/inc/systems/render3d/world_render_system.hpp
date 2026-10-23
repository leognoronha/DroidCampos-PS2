#pragma once

#include <tyra>
#include <memory>
#include "core/isubsystem.hpp"

namespace DroidCampos {
namespace Systems {
namespace Render3D {

class WorldRenderSystem final : public Core::ISubsystem {
 public:
  WorldRenderSystem();
  ~WorldRenderSystem() override = default;

  const char* getName() const override { return "WorldRenderSystem"; }

  void init(Tyra::Engine* engine, Core::WorldState& state,
            Events::EventBus& bus) override;
  void executeStage(Core::HardwareStage stage, Tyra::Engine* engine,
                    Core::WorldState& state, Events::EventBus& bus,
                    Hardware::Ps2ScratchpadArena& scratchpad) override;
  void shutdown(Tyra::Engine* engine) override;

 private:
  void loadScenario3D(Tyra::Engine* engine);
  void loadMist3D(Tyra::Engine* engine);
  void loadPistol3D(Tyra::Engine* engine);
  void updatePistolViewmodelTransform(const Core::WorldState& state);
  void updateAbandonedSignFlicker(const Core::WorldState& state);
  void updateCardReaderLight(Core::WorldState& state, Events::EventBus& bus);

  Tyra::StaticPipeline stapip_;

  std::unique_ptr<Tyra::StaticMesh> scenarioMesh_;
  Tyra::StaPipOptions scenarioOptions_;
  Tyra::MeshMaterial* etipLogoMat_;
  Tyra::MeshMaterial* etipBoardMat_;
  Tyra::MeshMaterial* cardReaderLightMat_;
  bool scenarioLoaded_;

  std::unique_ptr<Tyra::StaticMesh> mistMesh_;
  Tyra::StaPipOptions mistOptions_;
  bool mistLoaded_;

  std::unique_ptr<Tyra::StaticMesh> pistolMesh_;
  Tyra::StaPipOptions pistolOptions_;
  bool pistolLoaded_;
};

}  // namespace Render3D
}  // namespace Systems
}  // namespace DroidCampos
