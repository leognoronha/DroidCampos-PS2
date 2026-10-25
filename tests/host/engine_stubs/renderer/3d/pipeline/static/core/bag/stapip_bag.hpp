#pragma once
#include "math/vec4.hpp"
#include "math/m4x4.hpp"
namespace Tyra {
struct HostColorBag {Vec4* many=nullptr;};
struct HostTextureBag {Vec4* coordinates=nullptr;};
struct HostLightingBag {Vec4* normals=nullptr;};
struct HostInfoBag {M4x4* model=nullptr;};
class StaPipBag {
 public:
  HostColorBag* color=nullptr;
  HostTextureBag* texture=nullptr;
  HostLightingBag* lighting=nullptr;
  HostInfoBag* info=nullptr;
  u32 count=0;
  Vec4* vertices=nullptr;
};
}
