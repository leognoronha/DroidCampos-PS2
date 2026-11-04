#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include "loaders/3d/obj_loader/obj_loader.hpp"
using namespace Tyra;
void release(MeshBuilderData* data) {
 for(auto* m:data->materials)for(auto* f:m->frames){delete[] f->vertices;delete[] f->textureCoords;delete[] f->normals;delete[] f->colors;}
}
int main(int argc,char** argv) {
 assert(argc==2);std::string root=argv[1],obj=root+"droid_preview/actor.obj";
 std::ifstream file(obj);std::string line;std::vector<std::pair<float,float>> uv;
 while(std::getline(file,line)){std::istringstream in(line);std::string tag;in>>tag;if(tag=="vt"){float u,v;in>>u>>v;uv.emplace_back(u,1-v);}}
 std::ifstream bank(root+"droid_preview/motion.bin",std::ios::binary);bank.seekg(16);
 std::vector<float> positions(6900*3);bank.read(reinterpret_cast<char*>(positions.data()),positions.size()*4);assert(bank);
 unsigned legacyMismatches=0;
 for(bool preserve:{false,true}) {
  ObjLoaderOptions opt;opt.flipUVs=true;opt.preserveTriangleOrder=preserve;
  auto data=ObjLoader::load(obj,opt);assert(data->materials.size()==2);
  unsigned offset=0,mismatches=0;
  for(auto* m:data->materials)for(unsigned i=0;i<m->frames[0]->count;++i,++offset) {
   auto& p=m->frames[0]->vertices[i];auto& tex=m->frames[0]->textureCoords[i];
   bool match=std::fabs(p.x-positions[offset*3])<1e-6F&&std::fabs(p.y-positions[offset*3+1])<1e-6F&&std::fabs(p.z-positions[offset*3+2])<1e-6F;
   if(!match)++mismatches;
   if(preserve){assert(match);assert(std::fabs(tex.x-uv[offset].first)<1e-6F&&std::fabs(tex.y-uv[offset].second)<1e-6F);}
  }
  assert(offset==6900);release(data.get());
  if(!preserve)legacyMismatches=mismatches;else assert(mismatches==0);
 }
 assert(legacyMismatches>0);
 std::cout<<"PASS: actual Tyra OBJ loader; previous corner mismatches="<<legacyMismatches<<"; preserved positions/UV pairs=6900/6900\n";
}
