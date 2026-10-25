#include <cassert>
#include <iostream>
#include <vector>
#include "renderer/3d/pipeline/static/core/bag/packaging/stapip_bag_packager.hpp"
using namespace Tyra;
int main() {
  Vec4 vertices[108], attrs[108];
  HostColorBag color; color.many=attrs;
  HostTextureBag texture;texture.coordinates=attrs;
  HostLightingBag lighting;lighting.normals=attrs;
  StaPipBag bag;bag.vertices=vertices;bag.count=108;
  bag.color=&color;bag.texture=&texture;bag.lighting=&lighting;
  StaPipBagPackager packager;packager.setMaxVertCount(54);
  std::vector<StaPipBagPackage> mainStorage, subStorage;
  u16 count=0, subCount=0;
  auto* main=packager.create(&count,&bag,54,&mainStorage);
  assert(count==2 && main[0].vertices==vertices && main[1].vertices==vertices+54);
  auto* children=packager.create(&subCount,main[0],18,&subStorage);
  assert(subCount==3 && main==mainStorage.data());
  assert(main[1].size==54 && main[1].vertices==vertices+54);
  for(u32 i=0;i<subCount;++i) {
    assert(children[i].size==18 && children[i].vertices==vertices+i*18);
    assert(children[i].sts==attrs+i*18 && children[i].normals==attrs+i*18);
    assert(children[i].colors==attrs+i*18 && children[i].indexOf1By3BBox==i);
  }
  const auto* warmed=main;
  bag.count=3;bag.texture=nullptr;bag.lighting=nullptr;color.many=nullptr;
  main=packager.create(&count,&bag,54,&mainStorage);
  assert(count==1 && main==warmed && main[0].size==3);
  assert(!main[0].sts && !main[0].normals && !main[0].colors);
  bag.count=108;
  main=packager.create(&count,&bag,54,&mainStorage);
  assert(main==warmed && count==2);
  // The allocating API remains valid for callers outside StaPipCore.
  auto* legacy=packager.create(&count,&bag,54);
  assert(count==2 && legacy[1].vertices==vertices+54);delete[] legacy;
  bag.count=0;packager.create(&count,&bag,54,&mainStorage);assert(count==0);
  std::cout<<"PASS: independent parent/child package storage, shrink/growth reuse, optional attributes, empty and legacy ownership\n";
}
