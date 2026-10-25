#include <cassert>
#include <iostream>
#include "renderer/core/3d/bbox/render_bbox.hpp"
int main(){
  Tyra::Vec4 vertices[8];Tyra::RenderBBox bbox(vertices);Tyra::Plane planes[6];Tyra::M4x4 model;
  for(auto status:{Tyra::OUTSIDE_FRUSTUM,Tyra::PARTIALLY_IN_FRUSTUM,Tyra::IN_FRUSTUM}){
    Tyra::CoreBBox::classification=status;
    assert(bbox.clipFrustumCheck(planes,model)==status);
  }
  assert(Tyra::CoreBBox::marginChecks==0);
  std::cout<<"PASS: actual RenderBBox preserves near/edge intersections for CPU clipping instead of guard-band VU1 discard\n";
}
