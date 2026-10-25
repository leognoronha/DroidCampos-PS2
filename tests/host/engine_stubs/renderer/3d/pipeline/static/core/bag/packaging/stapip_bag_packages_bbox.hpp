#pragma once
#include "math/m4x4.hpp"
#include "renderer/core/3d/bbox/core_bbox_frustum.hpp"
namespace Tyra {
class StaPipBagPackagesBBox {
 public:
  struct Box {
    CoreBBoxFrustum clipFrustumCheck(const void*,const M4x4&) const {return PARTIALLY_IN_FRUSTUM;}
  } box;
  Box& getChildBBox1By3(u32) {return box;}
  Box createChildBBox(u32,u32) {return box;}
};
}
