#pragma once
#include <vector>
#include "math/vec4.hpp"
#include "renderer/renderer_settings.hpp"
namespace Tyra {
struct Plane {};struct M4x4 {};
enum CoreBBoxFrustum {OUTSIDE_FRUSTUM,PARTIALLY_IN_FRUSTUM,IN_FRUSTUM};
struct CoreBBox {
  inline static CoreBBoxFrustum classification=PARTIALLY_IN_FRUSTUM;
  inline static int marginChecks=0;
  template<class... Args> explicit CoreBBox(Args&&...){}
  CoreBBoxFrustum frustumCheck(const Plane*,const M4x4&,const float* margins=nullptr)const{
    if(margins){++marginChecks;return IN_FRUSTUM;}return classification;
  }
};
class RenderBBox:public CoreBBox {
 public:
  explicit RenderBBox(const Vec4*,const u32*,const u32&);
  explicit RenderBBox(const Vec4*,const u32&);
  explicit RenderBBox(const Vec4*);
  explicit RenderBBox(CoreBBox**,const u32&);
  explicit RenderBBox(const std::vector<CoreBBox>&,const u32&,const u32&);
  explicit RenderBBox(const RenderBBox&,const M4x4&);
  CoreBBoxFrustum clipFrustumCheck(const Plane*,const M4x4&)const;
  RenderBBox getTransformed(const M4x4&)const;
};
}
