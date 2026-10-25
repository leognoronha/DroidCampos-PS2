#pragma once
#include <cassert>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "math/vec4.hpp"
constexpr int DMA_CHANNEL_VIF1=0;
inline void dma_channel_fast_waits(int) {}
namespace Tyra {
enum CoreBBoxFrustum {OUTSIDE_FRUSTUM,PARTIALLY_IN_FRUSTUM,IN_FRUSTUM};
enum {PipelineFrustumCulling_None,PipelineFrustumCulling_Simple,PipelineFrustumCulling_Precise};
enum {PipelineInfoBagFrustumCulling_None,PipelineInfoBagFrustumCulling_Precise};
enum {TyraMVP,TyraMP};
using Color=Vec4;
struct M4x4 {float x=0;};
struct BBox {
  mutable unsigned checks=0;
  CoreBBoxFrustum classification=PARTIALLY_IN_FRUSTUM;
  CoreBBoxFrustum frustumCheck(const void*,const M4x4& m) const {
    ++checks;return m.x < -10 ? OUTSIDE_FRUSTUM : classification;
  }
};
struct MeshFrame {BBox* bbox=nullptr;};
struct MeshMaterialFrame {unsigned count=3;Vec4 *vertices=nullptr,*normals=nullptr,*textureCoords=nullptr,*colors=nullptr;};
struct MeshMaterial {unsigned id=1;std::string name;bool lightmapFlag=false;std::optional<std::string> textureName;Color ambient;std::vector<MeshMaterialFrame*> frames;};
struct StaticMesh {MeshFrame* frame=nullptr;M4x4 model;std::vector<MeshMaterial*> materials;M4x4 getModelMatrix()const{return model;}};
struct PipelineLightingOptions {Color* directionalColors=nullptr;Vec4* directionalDirections=nullptr;Color* ambientColor=nullptr;};
struct StaPipOptions {
  bool antiAliasingEnabled=false,blendingEnabled=false,fullClipChecks=false;
  int shadingType=0,textureMappingType=0,transformationType=TyraMVP,frustumCulling=PipelineFrustumCulling_Precise,zTestType=0;
  PipelineLightingOptions* lighting=nullptr;
};
struct StaPipInfoBag : StaPipOptions {M4x4* model=nullptr;};
struct StaPipColorBag {const Vec4* single=nullptr;Vec4* many=nullptr;};
struct Texture {};
struct StaPipTextureBag {Texture* texture=nullptr;Vec4* coordinates=nullptr;};
struct PipelineDirLightsBag {
  inline static unsigned constructions=0;
  explicit PipelineDirLightsBag(bool){++constructions;}
  void setLightsManually(Vec4*,Vec4*){}
};
struct StaPipLightingBag {PipelineDirLightsBag* dirLights=nullptr;M4x4* lightMatrix=nullptr;Vec4* normals=nullptr;};
struct StaPipBag {unsigned count=0;Vec4* vertices=nullptr;StaPipInfoBag* info=nullptr;StaPipColorBag* color=nullptr;StaPipTextureBag* texture=nullptr;StaPipLightingBag* lighting=nullptr;};
struct Repository {unsigned lookups=0;Texture texture;Texture* getByMeshMaterialId(unsigned){++lookups;return &texture;}};
struct RendererCore {
  struct {struct {const void* getAll()const{return nullptr;}}frustumPlanes;}renderer3D;
  struct {Repository repository;}texture;
};
struct StaPipCore {
  unsigned submissions=0;bool lastClip=false,lastTexture=false,lastLighting=false;
  void init(RendererCore*){}void allocateOnUse(){}void deallocateOnUse(){}
  void reinitVU1Programs(){}void onFrameEnd(){}
  void render(StaPipBag* bag) {
    assert(bag->info && bag->color && bag->vertices);
    ++submissions;lastClip=bag->info->fullClipChecks;
    lastTexture=bag->texture;lastLighting=bag->lighting;
  }
};
class StaticPipeline {
 public:
  StaticPipeline();~StaticPipeline();StaPipCore core;
  std::function<void(StaticPipeline*)> onDestroy;
  void setRenderer(RendererCore*);void onUse();void onUseEnd();void onFrameEnd();
  void render(const StaticMesh*);void render(const StaticMesh*,const StaPipOptions&);
  void render(const StaticMesh*,const StaPipOptions*);bool isVisible(const StaticMesh*)const;
 private:
  RendererCore* rendererCore=nullptr;Vec4* colorsCache=nullptr;
  void addVertices(const MeshMaterialFrame*,StaPipBag*)const;
  StaPipInfoBag getInfoBag(const StaticMesh*,const StaPipOptions*,M4x4*)const;
  StaPipColorBag getColorBag(const MeshMaterial*,const MeshMaterialFrame*)const;
  StaPipTextureBag getTextureBag(const MeshMaterial*,const MeshMaterialFrame*);
  StaPipLightingBag getLightingBag(const MeshMaterialFrame*,PipelineDirLightsBag*,M4x4*,const StaPipOptions*)const;
  void setLightingColorsCache(PipelineLightingOptions*);
};
}
