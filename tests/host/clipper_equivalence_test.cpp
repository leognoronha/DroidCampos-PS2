#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include "renderer/core/3d/clipper/planes_clip_algorithm.hpp"
namespace Tyra {
u8 referenceClip(PlanesClipVertex*, PlanesClipVertexPtrs*, const EEClipAlgorithmSettings&);
}
using namespace Tyra;
static bool same(const Vec4& a, const Vec4& b) {
  return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w;
}
int main() {
  PlanesClipAlgorithm algorithm; RendererSettings renderer; algorithm.init(renderer);
  PlanesClipVertex input[3], actual[9], expected[9];
  PlanesClipVertexPtrs pointers[3];
  for (int i=0;i<3;++i) {
    input[i].st={float(i),float(i+1),0,1};
    input[i].normal={float(i-1),float(2-i),1,0};
    input[i].color={float(i*30),float(i*60),128,128};
    pointers[i]={&input[i].position,&input[i].normal,&input[i].st,&input[i].color};
  }
  unsigned cases=0;
  auto check=[&]() {
    for (unsigned mask=0;mask<8;++mask) {
      EEClipAlgorithmSettings settings{bool(mask&1),bool(mask&2),bool(mask&4)};
      auto n=algorithm.clip(actual,pointers,settings);
      auto m=referenceClip(expected,pointers,settings);
      assert(n==m);
      for(unsigned i=0;i<n;++i) {
        assert(same(actual[i].position,expected[i].position));
        if(settings.lerpNormals) assert(same(actual[i].normal,expected[i].normal));
        if(settings.lerpTexCoords) assert(same(actual[i].st,expected[i].st));
        if(settings.lerpColors) assert(same(actual[i].color,expected[i].color));
      }
      ++cases;
    }
  };
  // Each inclusive boundary, its immediate float neighbors, crossings and rejection.
  for(int plane=0;plane<6;++plane) {
    const float boundary=plane<4?.5F:1.F;
    for(float value : {std::nextafter(boundary,0.F), boundary,
                        std::nextafter(boundary,2.F), 2.F}) {
      for(int outside=0;outside<=3;++outside) {
        for(int i=0;i<3;++i) {
          input[i].position={-.1F+.1F*i,.1F-.1F*i,0,1};
          if(i<outside) {
            float v=(plane%2==0)?value:-value;
            if(plane<2) input[i].position.x=v;
            else if(plane<4) input[i].position.y=v;
            else input[i].position.z=v;
          }
        }
        check();
      }
    }
  }
  // Degenerate triangles, zero w and vertices behind the eye.
  for(float w : {-2.F,0.F,1.F}) {
    for(auto& vertex:input) vertex.position={0,0,0,w};
    check();
  }
  std::mt19937 rng(0xD201D);
  std::uniform_real_distribution<float> coordinate(-4.F,4.F);
  for(int trial=0;trial<50000;++trial) {
    for(auto& vertex:input)
      vertex.position={coordinate(rng),coordinate(rng),coordinate(rng),coordinate(rng)};
    check();
  }
  std::cout<<"PASS: "<<cases<<" exact reference comparisons, all attribute flags, six boundaries, degenerates and eye crossings\n";
}
