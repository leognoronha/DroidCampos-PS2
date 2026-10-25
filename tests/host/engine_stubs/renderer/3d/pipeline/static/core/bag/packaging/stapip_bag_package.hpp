#pragma once
#include "renderer/3d/pipeline/static/core/bag/stapip_bag.hpp"
#include "renderer/core/3d/bbox/core_bbox_frustum.hpp"
namespace Tyra {
class StaPipBagPackage {
 public:
  StaPipBag* bag=nullptr;
  const Vec4* vertices=nullptr;
  const Vec4* sts=nullptr;
  const Vec4* colors=nullptr;
  const Vec4* normals=nullptr;
  u16 size=0;
  u32 indexOf1By3BBox=0;
  CoreBBoxFrustum isInFrustum=OUTSIDE_FRUSTUM;
};
}
