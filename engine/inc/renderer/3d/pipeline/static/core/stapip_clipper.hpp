/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2022, tyra - https://github.com/h4570/tyra
# Licensed under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#pragma once

#include <vector>
#include "math/m4x4.hpp"
#include "debug/debug.hpp"
#include "./stapip_qbuffer.hpp"
#include "renderer/renderer_settings.hpp"
#include "renderer/core/3d/clipper/planes_clip_algorithm.hpp"

namespace Tyra {

/**
 * @brief Clips an input package into reusable CPU scratch storage. Output must
 * be copied into triangle-aligned VU1-sized batches before submission.
 */
class StaPipClipper {
 public:
  StaPipClipper();
  ~StaPipClipper();
  void clip(StaPipQBuffer* buffer);
  u32 getVertexCount() const { return clippedVertices.size(); }
  void fillBuffer(StaPipQBuffer* buffer, u32 offset, u32 count);
  void init(const RendererSettings& settings);
  void setMaxVertCount(const u32& count);
  void setMVP(M4x4* mvp);

 private:
  u32 maxVertCount;
  PlanesClipAlgorithm algorithm;
  M4x4* mvp;

  Vec4 inputVerts[3];
  PlanesClipVertexPtrs inputTriangle[3];
  PlanesClipVertex clippedTriangle[9];
  std::vector<PlanesClipVertex> clippedVertices;

  void perspectiveDivide(std::vector<PlanesClipVertex>* vertices);
};

}  // namespace Tyra
