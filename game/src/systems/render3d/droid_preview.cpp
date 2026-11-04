#include "systems/render3d/droid_preview.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
namespace DroidCampos { namespace Systems { namespace Render3D {
namespace {
constexpr unsigned kVertices=6900,kFrames=36;
std::string path(const char* name) {return Tyra::FileUtils::fromCwd(std::string("droid_preview/")+name);}
bool readBank(std::vector<float>* bank) {
 FILE* f=std::fopen(path("motion.bin").c_str(),"rb");if(!f)return false;
 char magic[8];unsigned n=0,frames=0;
 bool ok=std::fread(magic,1,8,f)==8 && std::fread(&n,4,1,f)==1 && std::fread(&frames,4,1,f)==1 &&
   std::memcmp(magic,"DROIDAN1",8)==0 && n==kVertices && frames==kFrames;
 ok=ok && std::fseek(f,0,SEEK_END)==0 && std::ftell(f)==16L+kVertices*kFrames*12L;
 if(ok && bank) {
  bank->resize(kVertices*kFrames*3);std::fseek(f,16,SEEK_SET);
  ok=std::fread(bank->data(),4,bank->size(),f)==bank->size();
  for(float v:*bank)if(!std::isfinite(v)||std::fabs(v)>4)ok=false;
  if(!ok)bank->clear();
 }
 std::fclose(f);return ok;
}
}
void DroidPreview::init(Tyra::Engine* e) {
 pipeline_.setRenderer(&e->renderer.core);timer_.prime();
 options_.frustumCulling=Tyra::PipelineFrustumCulling_Precise;
 options_.fullClipChecks=true;options_.textureMappingType=Tyra::TyraNearest;
 options_.transformationType=Tyra::TyraMVP;
}
bool DroidPreview::validateAssets() {
 for(const char* name:{"actor.obj","actor.mtl","body.png","head.png"}) {
  FILE* f=std::fopen(path(name).c_str(),"rb");if(!f)return false;std::fclose(f);
 }
 return readBank(nullptr);
}
bool DroidPreview::load(Tyra::Engine* e) {
 if(!e || mesh_ || !readBank(&bank_))return false;
 Tyra::ObjLoaderOptions opt;opt.scale=1;opt.flipUVs=true;opt.preserveTriangleOrder=true;
 auto data=Tyra::ObjLoader::load(path("actor.obj"),opt);
 mesh_=std::make_unique<Tyra::StaticMesh>(data.get());data.reset();
 unsigned count=0;
 for(auto* m:mesh_->materials) {
  count+=m->frames[0]->count;
  auto* texture=e->renderer.getTextureRepository().add(path((m->name+".png").c_str()));
  texture->addLink(m->id);
  const Tyra::Vec4 bounds[]={Tyra::Vec4(-1.3F,-.01F,-1.2F,1),Tyra::Vec4(1.3F,3.4F,1.2F,1)};
  *m->frames[0]->bbox=Tyra::BBox(bounds,2);*mesh_->frame->bbox=Tyra::BBox(bounds,2);
 }
 if(count!=kVertices){unload(e);return false;}
 seconds_=0;wasActive_=false;timer_.prime();return true;
}
void DroidPreview::render(Tyra::Engine* e,const Core::WorldState& s) {
 float dt=std::min(timer_.getTimeDelta()/15625.F,.1F);timer_.prime();
 bool active=s.simulationActive();if(active && wasActive_)seconds_+=dt;wasActive_=active;
 if(!mesh_)return;
 // Four two-metre legs, each followed by the complete three-second punch.
 const float t=std::fmod(seconds_,28.F);int leg=static_cast<int>(t/7.F);
 float phase=t-leg*7.F,travel=std::min(phase/4.F,1.F);
 const float x[5]={-2.F,0.F,0.F,-2.F,-2.F},z[5]={-26.F,-26.F,-30.F,-30.F,-26.F};
 float pose=phase<4?std::fmod(phase,2.F)*6.F:12.F+std::min((phase-4)*8.F,23.F);
 int from=static_cast<int>(pose),to=phase<4?(from+1)%12:std::min(from+1,35);
 float blend=pose-from;
 e->renderer.core.sync.align3D();pipeline_.core.clearGeometryCache();
 unsigned offset=0;
 for(auto* m:mesh_->materials) {
  auto* f=m->frames[0];
  for(unsigned i=0;i<f->count;++i) {
   unsigned j=(offset+i)*3;const float* a=bank_.data()+from*kVertices*3+j;
   const float* b=bank_.data()+to*kVertices*3+j;
   f->vertices[i].set(a[0]+(b[0]-a[0])*blend,a[1]+(b[1]-a[1])*blend,
                       a[2]+(b[2]-a[2])*blend,1);
  }
  offset+=f->count;
 }
 mesh_->rotation.identity();mesh_->rotation.rotateY(std::atan2(x[leg+1]-x[leg],z[leg+1]-z[leg]));
 mesh_->translation.identity();mesh_->translation.translate(Tyra::Vec4(x[leg]+(x[leg+1]-x[leg])*travel,0,z[leg]+(z[leg+1]-z[leg])*travel,1));
 e->renderer.renderer3D.usePipeline(pipeline_);pipeline_.render(mesh_.get(),options_);
}
void DroidPreview::unload(Tyra::Engine* e) {
 if(e)e->renderer.core.sync.align3D();
 pipeline_.core.clearGeometryCache();
 if(e && mesh_)e->renderer.getTextureRepository().freeByMesh(mesh_.get());
 mesh_.reset();std::vector<float>().swap(bank_);seconds_=0;wasActive_=false;
}
}}}
