#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include "systems/render3d/spatial_order.hpp"
using namespace DroidCampos::Systems::Render3D;
struct V {float x=0,y=0,z=0,w=1;};
struct C {float r=0,g=0,b=0,a=128;};
struct Frame {unsigned count=0;V *vertices=nullptr,*textureCoords=nullptr,*normals=nullptr;C* colors=nullptr;};
struct Data {int width=1,height=1,bpp=32;unsigned char* data;};
struct Texture {Data* core;};
static bool same(const V& a,const V& b) {return a.x==b.x&&a.y==b.y&&a.z==b.z&&a.w==b.w;}
static double area(const std::vector<V>& v,unsigned batch) {
  double total=0;
  for(unsigned i=0;i<v.size();i+=batch) {
    V lo=v[i],hi=v[i];
    for(unsigned j=i;j<std::min<unsigned>(i+batch,v.size());++j) {
      lo.x=std::min(lo.x,v[j].x);lo.y=std::min(lo.y,v[j].y);lo.z=std::min(lo.z,v[j].z);
      hi.x=std::max(hi.x,v[j].x);hi.y=std::max(hi.y,v[j].y);hi.z=std::max(hi.z,v[j].z);
    }
    const double x=hi.x-lo.x,y=hi.y-lo.y,z=hi.z-lo.z;
    total+=2*(x*y+x*z+y*z);
  }
  return total;
}
static void checkPermutation(std::vector<V>& v,unsigned packageVertices=54) {
  const auto original=v;
  std::vector<V> uv(v.size()),normals(v.size());std::vector<C> colors(v.size());
  for(unsigned i=0;i<v.size();++i){uv[i]={float(i),1,2,3};normals[i]={float(i),4,5,6};colors[i].r=float(i);}
  Frame frame{static_cast<unsigned>(v.size()),v.data(),uv.data(),normals.data(),colors.data()};
  spatiallyOrderTriangles(frame,packageVertices);
  std::vector<bool> seen(v.size()/3,false);
  for(unsigned i=0;i<v.size();i+=3) {
    const auto src=static_cast<unsigned>(uv[i].x);assert(src%3==0 && src<v.size());
    assert(!seen[src/3]);seen[src/3]=true;
    for(unsigned j=0;j<3;++j) {
      assert(same(v[i+j],original[src+j]));
      assert(uv[i+j].x==src+j && normals[i+j].x==src+j && colors[i+j].r==src+j);
    }
  }
  assert(!spatiallyOrderTriangles(frame,packageVertices));
  colors[0].a=64;const auto sorted=v;
  assert(!spatiallyOrderTriangles(frame));
  colors[0].a=200;assert(!spatiallyOrderTriangles(frame));
  for(unsigned i=0;i<v.size();++i)assert(same(v[i],sorted[i]));
}
int main(int argc,char** argv) {
  assert(argc==2);
  unsigned char rgba[4]={1,2,3,128};Data data{1,1,32,rgba};Texture texture{&data};
  assert(textureIsOpaque(&texture));rgba[3]=26;assert(!textureIsOpaque(&texture));
  data.bpp=8;assert(!textureIsOpaque(&texture));assert(!textureIsOpaque<Texture>(nullptr));
  std::vector<V> interleaved;
  for(unsigned i=0;i<48;++i) {
    float x=float(i%8)*100;
    interleaved.push_back({x,0,0,1});interleaved.push_back({x+1,1,0,1});interleaved.push_back({x,0,1,1});
  }
  const auto before=area(interleaved,18);checkPermutation(interleaved);
  assert(area(interleaved,18)<before);
  double prior[3]={},after[3]={};unsigned triangles=0;
  const unsigned batches[3]={18,27,36};
  for(const char* part: {"corredor_1","corredor_2","corredor_3","sala_1","sala_2","sala_3","sala_4","sala_terminal","terminal","capsula","porta"}) {
    std::ifstream file(std::string(argv[1])+"etip_inside/"+part+".obj");assert(file.good());
    std::vector<V> positions;std::map<std::string,std::vector<V>> groups;
    std::string line,material;
    while(std::getline(file,line)) {
      std::istringstream in(line);std::string token;in>>token;
      if(token=="v"){V v;in>>v.x>>v.y>>v.z;positions.push_back(v);}
      else if(token=="usemtl")in>>material;
      else if(token=="f") {
        std::string index;unsigned corners=0;
        while(in>>index){const int id=std::stoi(index);assert(id>0&&unsigned(id)<=positions.size());groups[material].push_back(positions[id-1]);++corners;}
        assert(corners==3);
      }
    }
    for(auto& entry:groups) {
      auto& v=entry.second;triangles+=v.size()/3;
      for(unsigned b=0;b<3;++b) {
        auto candidate=v;
        const auto beforeArea=area(candidate,batches[b]);
        const auto beforePackageArea=area(candidate,batches[b]*3);
        prior[b]+=beforeArea;
        checkPermutation(candidate,batches[b]*3);
        assert(area(candidate,batches[b])<=beforeArea);
        assert(area(candidate,batches[b]*3)<=beforePackageArea);
        after[b]+=area(candidate,batches[b]);
      }
    }
  }
  std::cout<<"PASS: exact attribute/winding permutation, opacity guards, stable keys, "<<triangles<<" opaque asset triangles\n";
  for(unsigned b=0;b<3;++b)
    std::cout<<"AABB surface-area proxy, "<<batches[b]<<" vertices: "<<prior[b]<<" -> "<<after[b]<<" (not FPS)\n";
}
