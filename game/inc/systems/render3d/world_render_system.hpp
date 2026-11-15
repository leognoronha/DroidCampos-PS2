#pragma once

#include <tyra>
#include <memory>
#include <map>
#include "core/isubsystem.hpp"
#include "systems/weapon/weapon_geometry.hpp"

namespace DroidCampos {
namespace Systems {
namespace Render3D {

class WorldRenderSystem final : public Core::ISubsystem, public Weapon::WeaponScene {
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
  bool validateScene(Core::SceneId scene) const;
  bool traceWeapon(const Core::WorldState& state,const Tyra::Vec4& origin,
                   const Tyra::Vec4& direction,Weapon::RayHit& hit) const override {
    return weaponRays_.trace(state,origin,direction,hit);
  }
  Tyra::Vec4 weaponMuzzle(const Core::WorldState& state) override;
  void unloadScene(Tyra::Engine* engine);
  void loadSceneStep(Tyra::Engine* engine, Core::SceneId scene, int step);

 private:
  void loadScenario3D(Tyra::Engine* engine);
  void loadMist3D(Tyra::Engine* engine);
  void loadPistol3D(Tyra::Engine* engine);
  void loadWeaponEffects();
  void renderWeaponEffects(Tyra::Engine*,const Core::WorldState&);
  void registerWeaponGeometry(Tyra::StaticMesh*,int door=-1);
  Weapon::WeaponRayScene weaponRays_;
  std::unique_ptr<Tyra::StaticMesh> beamCore_, beamGlow_, impactMesh_;
  Tyra::StaPipOptions laserOptions_;
  void renderTransparentPass(Tyra::Engine* engine, Tyra::StaticMesh* mesh,
                             const Tyra::StaPipOptions& options);
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
  std::vector<std::unique_ptr<Tyra::StaticMesh>> interiorMeshes_;
  std::unique_ptr<Tyra::StaticMesh> doorMesh_;
  std::unique_ptr<Tyra::StaticMesh> capsuleGlassMesh_;
  Tyra::StaPipOptions capsuleGlassOptions_;
  std::map<std::string, u32> interiorTextures_;
};

}  // namespace Render3D
}  // namespace Systems
}  // namespace DroidCampos
