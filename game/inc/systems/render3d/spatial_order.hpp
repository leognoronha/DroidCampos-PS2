#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

namespace DroidCampos { namespace Systems { namespace Render3D {

// Call only during loading, before any address-keyed package bounds are cached.
// Arrays are deindexed triangle lists; reorder every attribute with the same
// permutation, preserving the three vertices and winding of each triangle.
template<class T>
void permuteTriangleArray(T* values, const std::vector<std::uint32_t>& order) {
  if (!values) return;
  std::vector<T> copy(order.size() * 3);
  for (std::size_t i = 0; i < order.size(); ++i)
    for (unsigned j = 0; j < 3; ++j) copy[i * 3 + j] = values[order[i] * 3 + j];
  std::copy(copy.begin(), copy.end(), values);
}

template<class Frame>
bool spatiallyOrderTriangles(Frame& frame, std::uint32_t packageVertices = 54) {
  if (!frame.vertices || frame.count < 6 || frame.count % 3) return false;
  if (packageVertices < 9 || packageVertices % 9) return false;
  if (frame.colors)
    for (std::uint32_t i = 0; i < frame.count; ++i)
      if (frame.colors[i].a != 128.F) return false;
  struct Center {float axis[3];};
  const std::uint32_t count = frame.count / 3;
  std::vector<Center> centers(count);
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto* v = frame.vertices + i * 3;
    centers[i] = {{v[0].x / 3 + v[1].x / 3 + v[2].x / 3,
                   v[0].y / 3 + v[1].y / 3 + v[2].y / 3,
                   v[0].z / 3 + v[1].z / 3 + v[2].z / 3}};
    for (float c : centers[i].axis) if (!std::isfinite(c)) return false;
  }
  auto lo = centers[0], hi = lo;
  for (const auto& c : centers)
    for (unsigned a = 0; a < 3; ++a) {
      lo.axis[a] = std::min(lo.axis[a], c.axis[a]);
      hi.axis[a] = std::max(hi.axis[a], c.axis[a]);
    }
  std::vector<std::uint32_t> keys(count, 0), order(count);
  for (std::uint32_t i = 0; i < count; ++i)
    for (unsigned a = 0; a < 3; ++a) {
      const float span = hi.axis[a] - lo.axis[a];
      if (!std::isfinite(span)) return false;
      const auto q = span > 0 ? static_cast<std::uint32_t>(std::min(1023.F,
          std::max(0.F, (centers[i].axis[a] - lo.axis[a]) / span * 1023.F))) : 0;
      for (unsigned bit = 0; bit < 10; ++bit)
        keys[i] |= ((q >> bit) & 1U) << (bit * 3 + a);
    }
  std::iota(order.begin(), order.end(), 0U);
  std::sort(order.begin(), order.end(), [&](std::uint32_t a, std::uint32_t b) {
    return keys[a] != keys[b] ? keys[a] < keys[b] : a < b;
  });
  auto boundsArea = [&](const std::vector<std::uint32_t>& permutation,
                        std::uint32_t batchVertices) {
    double area = 0;
    const auto batchTriangles = batchVertices / 3;
    for (std::uint32_t start = 0; start < count; start += batchTriangles) {
      const auto& first = frame.vertices[permutation[start] * 3];
      float low[3] = {first.x, first.y, first.z};
      float high[3] = {first.x, first.y, first.z};
      for (auto i = start; i < std::min(count, start + batchTriangles); ++i)
        for (unsigned j = 0; j < 3; ++j) {
          const auto& v = frame.vertices[permutation[i] * 3 + j];
          const float axis[3] = {v.x, v.y, v.z};
          for (unsigned a = 0; a < 3; ++a) {
            low[a] = std::min(low[a], axis[a]);
            high[a] = std::max(high[a], axis[a]);
          }
        }
      const double x = high[0] - low[0], y = high[1] - low[1], z = high[2] - low[2];
      area += 2 * (x * y + x * z + y * z);
    }
    return area;
  };
  std::vector<std::uint32_t> best(count);
  std::iota(best.begin(), best.end(), 0U);
  const auto originalPackages = boundsArea(best, packageVertices);
  const auto originalSubpackages = boundsArea(best, packageVertices / 3);
  double bestScore = originalPackages + originalSubpackages;
  auto consider = [&]() {
    const auto packages = boundsArea(order, packageVertices);
    const auto subpackages = boundsArea(order, packageVertices / 3);
    if (packages <= originalPackages && subpackages <= originalSubpackages &&
        packages + subpackages < bestScore) {
      bestScore = packages + subpackages;
      best = order;
    }
  };
  consider();
  // Long corridors are often better ordered along one axis than by Morton.
  const unsigned axes[6][3] = {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
  for (const auto& axis : axes) {
    std::iota(order.begin(), order.end(), 0U);
    std::sort(order.begin(), order.end(), [&](std::uint32_t a, std::uint32_t b) {
      for (unsigned k : axis) {
        if (centers[a].axis[k] < centers[b].axis[k]) return true;
        if (centers[a].axis[k] > centers[b].axis[k]) return false;
      }
      return a < b;
    });
    consider();
  }
  order = best;
  bool changed = false;
  for (std::uint32_t i = 0; i < count; ++i) changed |= order[i] != i;
  if (!changed) return false;
  permuteTriangleArray(frame.vertices, order);
  permuteTriangleArray(frame.textureCoords, order);
  permuteTriangleArray(frame.normals, order);
  permuteTriangleArray(frame.colors, order);
  return true;
}

template<class Texture>
bool textureIsOpaque(const Texture* texture) {
  if (!texture || !texture->core || !texture->core->data) return false;
  const auto& core = *texture->core;
  if (core.width <= 0 || core.height <= 0) return false;
  if (core.bpp == 24) return true;
  if (core.bpp != 32) return false; // Unknown/paletted formats keep original order.
  const auto pixels = static_cast<std::size_t>(core.width) * core.height;
  for (std::size_t i = 0; i < pixels; ++i)
    if (core.data[i * 4 + 3] != 128) return false; // Tyra stores GS alpha 0..128.
  return true;
}

template<class Mesh, class Repository, class Pipeline>
void prepareOpaqueMesh(Mesh* mesh, Repository& repository, Pipeline& pipeline) {
  for (auto* material : mesh->materials) {
    if (material->ambient.a != 128.F) continue;
    if (material->textureName.has_value() &&
        !textureIsOpaque(repository.getByMeshMaterialId(material->id))) continue;
    for (auto* frame : material->frames) {
      const auto capacity = pipeline.core.getMaxVertCountByParams(
          !material->lightmapFlag, false, frame->textureCoords != nullptr);
      spatiallyOrderTriangles(*frame, capacity);
    }
  }
}

} } }
