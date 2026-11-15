/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2022, tyra - https://github.com/h4570/tyra
# Licensed under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#include "renderer/3d/pipeline/static/core/stapip_clipper.hpp"

#include "renderer/3d/pipeline/static/core/bag/stapip_bag.hpp"

namespace Tyra {

StaPipClipper::StaPipClipper() {}
StaPipClipper::~StaPipClipper() {}

void StaPipClipper::setMVP(M4x4* t_mvp) { mvp = t_mvp; }

void StaPipClipper::init(const RendererSettings& settings) {
  algorithm.init(settings);
}

void StaPipClipper::setMaxVertCount(const u32& count) { maxVertCount = count; }

void StaPipClipper::clip(StaPipQBuffer* buffer) {
  TYRA_ASSERT(buffer->size <= maxVertCount / 3, "Buffer should have max ",
              maxVertCount / 3, " verts if we want to clip it.");

  EEClipAlgorithmSettings algoSettings = {buffer->bag->lighting != nullptr,
                                          buffer->bag->texture != nullptr,
                                          buffer->bag->color->many != nullptr};

  clippedVertices.clear();

  for (u32 i = 0; i < buffer->size / 3; i++) {
    for (u8 j = 0; j < 3; j++) {
      inputVerts[j] = *mvp * buffer->vertices[i * 3 + j];

      inputTriangle[j] = {
          &inputVerts[j],
          buffer->bag->lighting ? &buffer->normals[i * 3 + j] : nullptr,
          buffer->bag->texture ? &buffer->sts[i * 3 + j] : nullptr,
          buffer->bag->color->many ? &buffer->colors[i * 3 + j] : nullptr};
    }

    u8 clippedSize =
        algorithm.clip(clippedTriangle, inputTriangle, algoSettings);

    if (clippedSize < 3) continue;

    auto va = clippedTriangle[0];
    for (u8 j = 1; j <= clippedSize - 2; j++) {
      auto vb = clippedTriangle[j];
      auto vc = clippedTriangle[(j + 1) % clippedSize];
      clippedVertices.push_back(va);
      clippedVertices.push_back(vb);
      clippedVertices.push_back(vc);
    }
  }

  perspectiveDivide(&clippedVertices);

}

void StaPipClipper::perspectiveDivide(std::vector<PlanesClipVertex>* vertices) {
  for (u32 i = 0; i < vertices->size(); i++) {
    (*vertices)[i].position /= (*vertices)[i].position.w;
  }
}

void StaPipClipper::fillBuffer(StaPipQBuffer* buffer, u32 offset, u32 count) {
  TYRA_ASSERT(count <= maxVertCount && count % 3 == 0 && offset % 3 == 0,
              "Clipped batch exceeds VU1 capacity or splits a triangle");
  TYRA_ASSERT(offset <= clippedVertices.size() &&
                  count <= clippedVertices.size() - offset,
              "Clipped batch is out of range");
  buffer->reallocateManually(count);
  for (u32 i = 0; i < count; ++i) {
    const auto& vertex = clippedVertices[offset + i];
    buffer->vertices[i] = vertex.position;
    if (buffer->bag->texture) buffer->sts[i] = vertex.st;
    if (buffer->bag->color->many) buffer->colors[i] = vertex.color;
    if (buffer->bag->lighting) buffer->normals[i] = vertex.normal;
  }
}

}  // namespace Tyra
