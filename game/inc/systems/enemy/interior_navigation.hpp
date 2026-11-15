#pragma once
#include <array>
#include <limits>
#include "systems/physics/interior_geometry.hpp"

namespace DroidCampos { namespace Systems { namespace Enemy {
class InteriorNavigation {
 public:
  static constexpr int kCapacity=64;
  void build(const Core::WorldState& s) {
    count_=0;
    auto node=[&](float x,float z){
      if(count_<kCapacity && Physics::interiorFree(s,x,z,Core::kEnemyRadius,false))
        nodes_[count_++]=Tyra::Vec4(x,0,z,1);
    };
    for(float z : {-1.8F,-5.F,-8.F,-12.F,-16.F,-20.F,-23.F,-25.F})node(0,z);
    for(float z : {-8.F,-16.F})for(int side : {-1,1}) {
      node(side*.9F,z);node(side*2.3F,z);node(side*3.8F,z);
      node(side*3.8F,z+.95F);
      node(side*4.6F,z-.7F);node(side*5.05F,z-1.35F);
    }
    for(float z : {-25.F,-26.F,-27.6F,-29.8F,-31.25F})
      for(float x : {-4.3F,-2.6F,-.8F,1.F,3.5F,4.5F})node(x,z);
    for(int i=0;i<count_;++i)for(int j=0;j<count_;++j)
      edges_[i][j]=i!=j && Physics::distanceXZ(nodes_[i],nodes_[j])<=8.1F &&
        Physics::interiorSegmentFree(s,nodes_[i],nodes_[j],Core::kEnemyRadius,false);
  }
  bool route(const Core::WorldState& s,const Tyra::Vec4& from,const Tyra::Vec4& to,
             std::array<int,kCapacity>& path,int& length)const {
    length=0;
    std::array<float,kCapacity> cost;
    std::array<int,kCapacity> parent;
    std::array<bool,kCapacity> closed{};
    cost.fill(std::numeric_limits<float>::max());parent.fill(-1);
    for(int i=0;i<count_;++i)
      if(Physics::interiorSegmentFree(s,from,nodes_[i],Core::kEnemyRadius,false))
        cost[i]=Physics::distanceXZ(from,nodes_[i]);
    int end=-1;
    for(int iteration=0;iteration<count_;++iteration) {
      int best=-1;float score=std::numeric_limits<float>::max();
      for(int i=0;i<count_;++i)if(!closed[i] && cost[i]<1e30F) {
        float f=cost[i]+Physics::distanceXZ(nodes_[i],to);
        if(f<score){score=f;best=i;}
      }
      if(best<0)break;
      closed[best]=true;
      if(Physics::interiorSegmentFree(s,nodes_[best],to,Core::kEnemyRadius,false)) {
        end=best;break;
      }
      for(int j=0;j<count_;++j)if(edges_[best][j] && !closed[j]) {
        float next=cost[best]+Physics::distanceXZ(nodes_[best],nodes_[j]);
        if(next<cost[j]){cost[j]=next;parent[j]=best;}
      }
    }
    if(end<0)return false;
    while(end>=0 && length<kCapacity){path[length++]=end;end=parent[end];}
    std::reverse(path.begin(),path.begin()+length);return true;
  }
  const Tyra::Vec4& node(int i)const{return nodes_[i];}
  int count()const{return count_;}
 private:
  int count_=0;
  std::array<Tyra::Vec4,kCapacity> nodes_{};
  std::array<std::array<bool,kCapacity>,kCapacity> edges_{};
};
}}}
