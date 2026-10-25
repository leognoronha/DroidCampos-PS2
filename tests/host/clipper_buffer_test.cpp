#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <iostream>
#include <vector>
#include "renderer/3d/pipeline/static/core/stapip_clipper.hpp"
#include "renderer/3d/pipeline/static/core/bag/packaging/stapip_bag_package.hpp"
namespace Tyra {
u8 referenceClip(PlanesClipVertex*, PlanesClipVertexPtrs*, const EEClipAlgorithmSettings&);
}
using namespace Tyra;
static std::size_t allocations=0;
void* operator new(std::size_t size) {
  ++allocations;
  if(auto* p=std::malloc(size?size:1)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
static bool same(const Vec4& a,const Vec4& b) {
  return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w;
}
int main() {
  constexpr u32 limit=54;
  Vec4 positions[18], attrs[18];
  for(int i=0;i<18;++i) {
    const Vec4 triangle[3]={{-1,-1,0,1},{1,-1,0,1},{0,1,0,1}};
    positions[i]=triangle[i%3]; attrs[i]={float(i),float(i+1),float(i+2),1};
  }
  HostColorBag colors; colors.many=attrs;
  HostTextureBag texture; HostLightingBag lighting;
  StaPipBag bag;bag.color=&colors;bag.texture=&texture;bag.lighting=&lighting;
  StaPipBagPackage pkg;pkg.bag=&bag;pkg.vertices=positions;pkg.sts=attrs;
  pkg.colors=attrs;pkg.normals=attrs;pkg.size=18;
  StaPipQBuffer source, destination;
  source.setMaxVertCount(limit);destination.setMaxVertCount(limit);
  source.fillByCopy1By3(pkg);
  assert(reinterpret_cast<std::uintptr_t>(source.vertices)%16==0);
  auto* owned=source.vertices;auto* ownedST=source.sts;
  const auto warmedAllocations=allocations;
  for(int i=0;i<1000;++i) {
    source.fillByPointer(pkg);
    assert(source.vertices==positions);
    source.fillByCopy1By3(pkg);
    assert(source.vertices==owned && source.sts==ownedST);
  }
  assert(allocations==warmedAllocations);
  // Disabled attributes must not expose old pointers; re-enable without growth.
  bag.texture=nullptr;bag.lighting=nullptr;colors.many=nullptr;
  source.fillByCopy1By3(pkg);
  assert(!source.sts && !source.normals && !source.colors);
  bag.texture=&texture;bag.lighting=&lighting;colors.many=attrs;
  source.fillByCopy1By3(pkg);assert(source.sts==ownedST);
  M4x4 identity;RendererSettings settings;StaPipClipper clipper;
  clipper.init(settings);clipper.setMVP(&identity);clipper.setMaxVertCount(limit);
  clipper.clip(&source);
  assert(clipper.getVertexCount()>limit); // Previous code exceeded the QBuffer.
  std::vector<PlanesClipVertex> expected;
  for(int i=0;i<18;i+=3) {
    PlanesClipVertexPtrs input[3];PlanesClipVertex out[9];
    for(int j=0;j<3;++j) input[j]={&positions[i+j],&attrs[i+j],&attrs[i+j],&attrs[i+j]};
    const auto n=referenceClip(out,input,{true,true,true});
    for(int j=1;j+1<n;++j) {
      expected.push_back(out[0]);expected.push_back(out[j]);expected.push_back(out[j+1]);
    }
  }
  assert(clipper.getVertexCount()==expected.size());
  u32 batches=0;
  for(u32 offset=0;offset<clipper.getVertexCount();) {
    const auto count=std::min(limit,clipper.getVertexCount()-offset);
    destination.bag=&bag;clipper.fillBuffer(&destination,offset,count);
    assert(destination.size<=limit && destination.size%3==0);
    for(u32 i=0;i<count;++i) {
      const auto& e=expected[offset+i];
      assert(same(destination.vertices[i],e.position));
      assert(same(destination.sts[i],e.st) && same(destination.colors[i],e.color));
      assert(same(destination.normals[i],e.normal));
    }
    offset+=count;++batches;
  }
  assert(batches>1);
  // Scratch is cleared between calls; batches and owned memory are reusable.
  for(auto& p:positions)p={0,0,2,1};
  source.fillByPointer(pkg);clipper.clip(&source);assert(clipper.getVertexCount()==0);
  destination.bag=&bag;clipper.fillBuffer(&destination,0,0);assert(!destination.any());
  // Grow owned arrays, then shrink and alternate source pointers without losing ownership.
  destination.reallocateManually(limit);auto* grown=destination.vertices;
  destination.reallocateManually(3);assert(destination.vertices==grown);
  destination.fillByPointer(pkg);destination.reallocateManually(limit);
  assert(destination.vertices==grown);
  std::cout<<"PASS: retained per-QBuffer storage, zero allocations in 1000 warmed reuse cycles, attributes, growth, empty output and overflow split into "<<batches<<" exact batches\n";
}
