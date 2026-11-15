/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2022, tyra - https://github.com/h4570/tyra
# Licensed under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#include "renderer/3d/pipeline/static/static_pipeline.hpp"
#include "debug/debug.hpp"
#include "renderer/3d/pipeline/static/core/bag/stapip_info_bag.hpp"
#include <optional>
namespace Tyra {

StaticPipeline::StaticPipeline() {}

StaticPipeline::~StaticPipeline() {
  if (onDestroy) onDestroy(this);
}

void StaticPipeline::setRenderer(RendererCore* t_core) {
  rendererCore = t_core;
  core.init(t_core);
}

void StaticPipeline::onUse() {
  dma_channel_fast_waits(DMA_CHANNEL_VIF1);

  colorsCache = new Vec4[4];
  core.allocateOnUse();
  core.reinitVU1Programs();
}

void StaticPipeline::onUseEnd() {
  delete[] colorsCache;

  core.deallocateOnUse();
}

void StaticPipeline::onFrameEnd() { core.onFrameEnd(); }

bool StaticPipeline::isVisible(const StaticMesh* mesh) const {
  return mesh->frame->bbox->frustumCheck(
      rendererCore->renderer3D.frustumPlanes.getAll(), mesh->getModelMatrix()) !=
      CoreBBoxFrustum::OUTSIDE_FRUSTUM;
}

void StaticPipeline::render(const StaticMesh* mesh) { render(mesh, nullptr); }

void StaticPipeline::render(const StaticMesh* mesh,
                            const StaPipOptions& options) {
  render(mesh, &options);
}

void StaticPipeline::render(const StaticMesh* mesh,
                            const StaPipOptions* options) {
  StaPipOptions defaultOptions;
  if (!options) options = &defaultOptions;

  auto model = mesh->getModelMatrix();

  TYRA_ASSERT(
      !(options->frustumCulling != PipelineFrustumCulling_Precise &&
        options->fullClipChecks == true),
      "Full clip checks are only supported with frustum culling == Precise!");

  TYRA_ASSERT(options->transformationType == TyraMVP ||
                  (!options->fullClipChecks &&
                   options->frustumCulling == PipelineFrustumCulling_None),
              "Please disable clip checks and frustum culling if not using MVP "
              "matrix!");

  if (options->frustumCulling != PipelineFrustumCulling_None) {
    auto* frame = mesh->frame;
    if (frame->bbox->frustumCheck(
            rendererCore->renderer3D.frustumPlanes.getAll(), model) ==
        CoreBBoxFrustum::OUTSIDE_FRUSTUM) {
      return;
    }
  }

  auto infoBag = getInfoBag(mesh, options, &model);
  std::optional<PipelineDirLightsBag> dirLightsBag;
  if (options->lighting) dirLightsBag.emplace(true);

  if (options && options->lighting) setLightingColorsCache(options->lighting);

  for (u32 i = 0; i < mesh->materials.size(); i++) {
    auto* material = mesh->materials[i];
    auto* materialFrame = material->frames[0];
    StaPipBag bag;
    addVertices(materialFrame, &bag);
    auto colorBag = getColorBag(material, materialFrame);
    auto textureBag = getTextureBag(material, materialFrame);
    auto lightingBag = getLightingBag(materialFrame,
        dirLightsBag ? &*dirLightsBag : nullptr, &model, options);
    bag.info = &infoBag;
    bag.color = &colorBag;
    bag.texture = textureBag.texture ? &textureBag : nullptr;
    bag.lighting = lightingBag.normals ? &lightingBag : nullptr;

    // core.render builds and flushes all packets before these descriptors die.
    // DMA references vertex/attribute arrays, not the local bag objects.
    core.render(&bag);
  }

}

void StaticPipeline::addVertices(const MeshMaterialFrame* materialFrame,
                                 StaPipBag* bag) const {
  bag->count = materialFrame->count;
  bag->vertices = materialFrame->vertices;
}

StaPipInfoBag StaticPipeline::getInfoBag(const StaticMesh*,
                                          const StaPipOptions* options,
                                          M4x4* model) const {
  StaPipInfoBag result;

  result.antiAliasingEnabled = options->antiAliasingEnabled;
  result.blendingEnabled = options->blendingEnabled;
  result.shadingType = options->shadingType;
  result.textureMappingType = options->textureMappingType;
  result.transformationType = options->transformationType;
  result.frustumCulling =
      options->frustumCulling == PipelineFrustumCulling_Precise
          ? PipelineInfoBagFrustumCulling_Precise
          : PipelineInfoBagFrustumCulling_None;
  result.fullClipChecks = options->fullClipChecks;
  result.zTestType = options->zTestType;
  result.model = model;

  return result;
}

StaPipColorBag StaticPipeline::getColorBag(
    const MeshMaterial* material,
    const MeshMaterialFrame* materialFrame) const {
  StaPipColorBag result;

  if (material->lightmapFlag) {
    result.many = materialFrame->colors;
  } else {
    result.single = &material->ambient;
  }

  return result;
}

StaPipTextureBag StaticPipeline::getTextureBag(
    const MeshMaterial* material, const MeshMaterialFrame* materialFrame) {
  if (!materialFrame->textureCoords || !material->textureName.has_value())
    return {};

  StaPipTextureBag result;

  result.texture =
      rendererCore->texture.repository.getByMeshMaterialId(material->id);

  TYRA_ASSERT(result.texture, "Texture for material: ", material->name,
              "Id: ", material->id,
              "Was not found in texture repository! Did you forget to add "
              "texture or disable texture coords loading in mesh?");

  result.coordinates = materialFrame->textureCoords;

  return result;
}

StaPipLightingBag StaticPipeline::getLightingBag(
    const MeshMaterialFrame* materialFrame, PipelineDirLightsBag* dirLightsBag,
    M4x4* model, const StaPipOptions* options) const {
  if (!materialFrame->normals || options == nullptr ||
      options->lighting == nullptr)
    return {};

  StaPipLightingBag result;

  result.dirLights = dirLightsBag;
  result.lightMatrix = model;

  result.dirLights->setLightsManually(
      colorsCache, options->lighting->directionalDirections);

  result.normals = materialFrame->normals;

  return result;
}

void StaticPipeline::setLightingColorsCache(
    PipelineLightingOptions* lightingOptions) {
  for (int i = 0; i < 3; i++) {
    colorsCache[i] =
        reinterpret_cast<Vec4&>(lightingOptions->directionalColors[i]);
  }
  colorsCache[3] = reinterpret_cast<Vec4&>(*lightingOptions->ambientColor);
}

}  // namespace Tyra
