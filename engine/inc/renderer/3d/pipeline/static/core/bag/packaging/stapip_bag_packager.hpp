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

#include "renderer/3d/pipeline/static/core/bag/packaging/stapip_bag_package.hpp"

namespace Tyra {

class StaPipBagPackagesBBox;
class Renderer3DFrustumPlanes;

class StaPipBagPackager {
 public:
  StaPipBagPackager();
  ~StaPipBagPackager();

  void init(Renderer3DFrustumPlanes* frustumPlanes);
  void setRenderBBox(StaPipBagPackagesBBox* bbox) { renderBBox = bbox; }
  void setMaxVertCount(const u32& count);

  /**
   * @brief Create render packages from provided render data
   *
   * @param size Max maxVertCount verts (VU1 buffer size)
   */
  StaPipBagPackage* create(u16* o_size, StaPipBag* data, u16 size,
      std::vector<StaPipBagPackage>* storage = nullptr);
  /**
   * @brief Split render package to smaller packages
   *
   * @param size Max maxVertCount verts (VU1 buffer size)
   */
  StaPipBagPackage* create(u16* o_size, const StaPipBagPackage& pkg, u16 size,
      std::vector<StaPipBagPackage>* storage = nullptr);

  CoreBBoxFrustum checkFrustum(const StaPipBagPackage& pkg);

 private:
  u32 maxVertCount;
  Renderer3DFrustumPlanes* frustumPlanes;
  StaPipBagPackagesBBox* renderBBox;
};

}  // namespace Tyra
