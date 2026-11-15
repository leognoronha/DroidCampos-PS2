/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2022, tyra - https://github.com/h4570/tyra
# Licensed under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#include <string>
#include <sstream>
#include "renderer/core/3d/bbox/render_bbox.hpp"

namespace Tyra {

RenderBBox::RenderBBox(CoreBBox** t_bboxes, const u32& count)
    : CoreBBox(t_bboxes, count) {}

RenderBBox::RenderBBox(const std::vector<CoreBBox>& t_bboxes,
                       const u32& startIndex, const u32& stopIndex)
    : CoreBBox(t_bboxes, startIndex, stopIndex) {}

RenderBBox::RenderBBox(const Vec4* t_vertices, const u32* faces,
                       const u32& count)
    : CoreBBox(t_vertices, faces, count) {}

RenderBBox::RenderBBox(const Vec4* t_vertices, const u32& count)
    : CoreBBox(t_vertices, count) {}

RenderBBox::RenderBBox(const Vec4* t_vertices) : CoreBBox(t_vertices) {}

RenderBBox::RenderBBox(const RenderBBox& t_bbox, const M4x4& t_matrix)
    : CoreBBox(t_bbox, t_matrix) {}

RenderBBox RenderBBox::getTransformed(const M4x4& t_matrix) const {
  return RenderBBox(*this, t_matrix);
}

/**
 * @brief Frustum checker for renderer.
 * Packages intersecting the frustum must reach the CPU clipper when full
 * clipping is enabled. VU1's cull path cannot reconstruct crossing triangles.
 */
CoreBBoxFrustum RenderBBox::clipFrustumCheck(const Plane* frustumPlanes,
                                             const M4x4& model) const {
  // Keep intersecting packages on the CPU clipping path. The former 10-25
  // world-unit guard band classified meter-scale rooms as fully inside even
  // when triangles crossed the eye/near plane; VU1 then rejected them whole.
  return frustumCheck(frustumPlanes, model);
}

}  // namespace Tyra
