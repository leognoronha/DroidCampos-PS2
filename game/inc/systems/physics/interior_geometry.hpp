#pragma once
#include <algorithm>
#include <cmath>
#include "core/world_state.hpp"
#include "core/interior_layout.hpp"

namespace DroidCampos { namespace Systems { namespace Physics {
struct InteriorBox { float minX, maxX, minZ, maxZ; };
// One authoritative collision layout for movement, route checks and spawning.
template<class Visit>
inline void visitInteriorBoxes(const Core::WorldState& s, bool doors, Visit visit) {
  for (int side : {-1, 1}) {
    const float wx = side * 1.6F;
    visit(InteriorBox{wx-.1F, wx+.1F, -7.3F, 0});
    visit(InteriorBox{wx-.1F, wx+.1F, -15.3F, -8.7F});
    visit(InteriorBox{wx-.1F, wx+.1F, -24, -16.7F});
    const float lo = side < 0 ? -5.8F : 1.5F;
    const float hi = side < 0 ? -1.5F : 5.8F;
    visit(InteriorBox{lo, hi, -6.1F, 0});
    visit(InteriorBox{lo, hi, -14.1F, -9.9F});
    visit(InteriorBox{lo, hi, -24, -17.9F});
    for (float rz : {-8.0F, -16.0F}) {
      const float cx=side*5.25F, bx=side*3.7F;
      for (int j=0;j<3;++j) {
        const float cz=rz+1.25F-j*.8F;
        visit(InteriorBox{cx-.25F,cx+.25F,cz-.32F,cz+.32F});
      }
      visit(InteriorBox{bx-.9F,bx+.9F,rz-1.9F,rz-1.2F});
    }
  }
  if (doors) for(int i=0;i<4;++i) {
    float x=Core::kInsideDoorX[i];
    float z=Core::kInsideDoorZ[i]-s.scene.doors[i].openAmount*Core::kDoorTravel;
    visit(InteriorBox{x-.08F,x+.08F,z-.7F,z+.7F});
  }
  visit(InteriorBox{-5,-1.7F,-24.1F,-23.9F});
  visit(InteriorBox{1.7F,5,-24.1F,-23.9F});
  visit(InteriorBox{-5,-3.58F,-29.175F,-26.825F});
  visit(InteriorBox{Core::kCapsuleMinX,Core::kCapsuleMaxX,
                    Core::kCapsuleMinZ,Core::kCapsuleMaxZ});
}
inline float distanceXZ(const Tyra::Vec4& a,const Tyra::Vec4& b) {
  float x=a.x-b.x,z=a.z-b.z; return std::sqrt(x*x+z*z);
}
inline bool circleHitsBox(float x,float z,float r,const InteriorBox& b) {
  float dx=x-std::max(b.minX,std::min(x,b.maxX));
  float dz=z-std::max(b.minZ,std::min(z,b.maxZ));
  return dx*dx+dz*dz < r*r-1e-6F;
}
inline bool interiorFree(const Core::WorldState& s,float x,float z,float r,
                         bool doors=true) {
  // Terminal-room side walls start at z=-24; room backs end at x=+-5.6.
  float limit=z < -24 ? 5-r : 5.6F-r;
  if(x < -limit || x > limit || z < -32+r || z > -.12F-r) return false;
  bool free=true;
  visitInteriorBoxes(s,doors,[&](const InteriorBox& b){
    if(circleHitsBox(x,z,r,b)) free=false;
  });
  return free;
}
inline bool interiorSegmentFree(const Core::WorldState& s,const Tyra::Vec4& a,
                                const Tyra::Vec4& b,float r,bool doors=true) {
  const int steps=std::max(1,static_cast<int>(std::ceil(distanceXZ(a,b)/.08F)));
  for(int i=0;i<=steps;++i) {
    float t=static_cast<float>(i)/steps;
    if(!interiorFree(s,a.x+(b.x-a.x)*t,a.z+(b.z-a.z)*t,r,doors)) return false;
  }
  return true;
}
inline Tyra::Vec4 projectInterior(const Core::WorldState& s,Tyra::Vec4 p,float r) {
  float limit=p.z < -24 ? 5-r : 5.6F-r;
  p.x=std::max(-limit,std::min(limit,p.x));
  p.z=std::max(-32+r,std::min(-.12F-r,p.z));
  for(int pass=0;pass<3;++pass)visitInteriorBoxes(s,true,[&](const InteriorBox& b) {
    if(!circleHitsBox(p.x,p.z,r,b))return;
    float x=std::max(b.minX,std::min(p.x,b.maxX));
    float z=std::max(b.minZ,std::min(p.z,b.maxZ));
    float dx=p.x-x,dz=p.z-z,length=std::sqrt(dx*dx+dz*dz);
    if(length>.00001F){p.x=x+dx/length*r;p.z=z+dz/length*r;return;}
    float distances[]={p.x-b.minX,b.maxX-p.x,p.z-b.minZ,b.maxZ-p.z};
    int axis=static_cast<int>(std::min_element(distances,distances+4)-distances);
    if(axis==0)p.x=b.minX-r;else if(axis==1)p.x=b.maxX+r;
    else if(axis==2)p.z=b.minZ-r;else p.z=b.maxZ+r;
  });
  return p;
}
inline bool actorPositionFree(const Core::WorldState& s,float x,float z,float r,
                             int self) {
  Tyra::Vec4 p(x,0,z,1);
  if(self>=0 && distanceXZ(p,s.player.playerPos)<r+Core::kPlayerBodyRadius-.001F)
    return false;
  for(int i=0;i<Core::kMaxEnemies;++i)
    if(i!=self && s.enemies[i].active &&
       distanceXZ(p,s.enemies[i].position)<r+Core::kEnemyRadius-.001F) return false;
  return true;
}
// Conservative swept substeps with axis sliding. No per-frame allocations.
inline Tyra::Vec4 moveInterior(const Core::WorldState& s,Tyra::Vec4 p,
                              float dx,float dz,float r,int self) {
  const int steps=std::max(1,static_cast<int>(std::ceil(std::max(std::fabs(dx),std::fabs(dz))/.04F)));
  dx/=steps; dz/=steps;
  for(int i=0;i<steps;++i) {
    auto free=[&](float x,float z){return interiorFree(s,x,z,r) && actorPositionFree(s,x,z,r,self);};
    if(free(p.x+dx,p.z+dz)){p.x+=dx;p.z+=dz;}
    else {
      if(free(p.x+dx,p.z))p.x+=dx;
      if(free(p.x,p.z+dz))p.z+=dz;
    }
  }
  return p;
}
}}}
