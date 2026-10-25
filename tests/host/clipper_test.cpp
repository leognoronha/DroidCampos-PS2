#include <cassert>
#include <cmath>
#include <iostream>
#include "renderer/core/3d/clipper/planes_clip_algorithm.hpp"
using namespace Tyra;
int main(){
  PlanesClipAlgorithm clipper;RendererSettings renderer;clipper.init(renderer);
  EEClipAlgorithmSettings settings{false,true,false};
  PlanesClipVertex input[3],out[9];PlanesClipVertexPtrs ptrs[3];
  for(int i=0;i<3;i++){input[i].st={float(i)/2,0,0,1};ptrs[i]={&input[i].position,nullptr,&input[i].st,nullptr};}
  auto project=[](float x,float y,float depth){
    const float near=.1F,far=51200.F;
    return Vec4(x*.16F,y*.16F,-depth*(far+near)/(far-near)+2*far*near/(far-near),depth);
  };
  auto clip=[&](){
    auto n=clipper.clip(out,ptrs,settings);
    for(int i=0;i<n;i++){
      auto p=out[i].position;assert(p.w>0);
      assert(std::isfinite(p.x/p.w)&&std::isfinite(p.y/p.w));
      assert(p.z<=p.w+.001F && p.z>=-p.w-.001F);
      assert(std::fabs(p.x)<=.5F*p.w+.001F && std::fabs(p.y)<=.5F*p.w+.001F);
      assert(out[i].st.x>=-.001F && out[i].st.x<=1.001F);
    }
    return n;
  };
  // Walls only 0.6m from the camera must survive CPU clipping (old code: 0).
  input[0].position=project(-.1F,-.1F,.6F);input[1].position=project(.1F,-.1F,.6F);input[2].position=project(0,.1F,.6F);
  assert(clip()==3);
  input[0].position=project(-.1F,-.1F,.05F);assert(clip()>=3); // Straddles near.
  input[0].position=project(0,0,-.4F);assert(clip()>=3); // Vertex behind eye.
  for(int i=0;i<3;i++)input[i].position=project(0,0,-1);
  assert(clip()==0);
  for(int i=0;i<3;i++)input[i].position=project(0,0,60000);
  assert(clip()==0);
  // Rotating a camera beside a wall: clipping output remains bounded/finite.
  for(int angle=0;angle<360;angle++){
    float yaw=angle*3.14159265F/180;
    float xs[3]={-1.5F,-1.5F,-1.5F},zs[3]={-4,4,4},ys[3]={-1.7F,-1.7F,1.1F};
    for(int i=0;i<3;i++){
      float horizontal=xs[i]*std::cos(yaw)-zs[i]*std::sin(yaw);
      float depth=xs[i]*std::sin(yaw)+zs[i]*std::cos(yaw);
      input[i].position=project(horizontal,ys[i],depth);
    }
    clip();
  }
  std::cout<<"PASS: actual Tyra clipper, 0.6m wall, near crossing, eye crossing, behind/far rejection, 360 camera angles\n";
}
