#include "systems/render3d/droid_preview.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
namespace DroidCampos { namespace Systems { namespace Render3D {
namespace {
constexpr unsigned kVertices=6900,kFrames=84;
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
 pipeline_.setRenderer(&e->renderer.core);
 options_.frustumCulling=Tyra::PipelineFrustumCulling_Precise;
 options_.fullClipChecks=true;options_.textureMappingType=Tyra::TyraNearest;
 options_.transformationType=Tyra::TyraMVP;
}
bool DroidPreview::validateAssets() {
 for(const char* name:{"actor.obj","actor.mtl","body.png","head.png","button.obj","button.mtl"}) {
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
  const Tyra::Vec4 bounds[]={Tyra::Vec4(-1.6F,-.01F,-1.7F,1),Tyra::Vec4(1.6F,3.4F,1.7F,1)};
  *m->frames[0]->bbox=Tyra::BBox(bounds,2);*mesh_->frame->bbox=Tyra::BBox(bounds,2);
 }
 if(count!=kVertices){unload(e);return false;}
 auto buttonData=Tyra::ObjLoader::load(path("button.obj"),opt);
 button_=std::make_unique<Tyra::StaticMesh>(buttonData.get());
 return true;
}
void DroidPreview::render(Tyra::Engine* e,const Core::WorldState& s) {
 if(!mesh_)return;
 const auto& boss=s.droid;
 if(boss.phase!=Core::DroidPhase::Inactive && button_) {
   float rise=boss.phase==Core::DroidPhase::ButtonRising?std::min(1.F,boss.phaseTicks/(4.F*Core::kDroidSecond)):1.F;
   button_->translation.identity();button_->translation.translate(Tyra::Vec4(0,-1.02F*(1-rise),-26,1));
   e->renderer.renderer3D.usePipeline(pipeline_);pipeline_.render(button_.get(),options_);
 }
 if(!boss.mobile())return;
 float pose=boss.attacking?12.F+std::min(boss.attackTicks/15625.F*8.F,23.F):std::fmod(boss.animationTicks/15625.F,2.F)*6.F;
 int base=boss.passage?72:(boss.crouched?36:0);
 int local=static_cast<int>(pose),from=base+local;
 int to=base+(boss.attacking?std::min(local+1,35):(local+1)%12);
 float blend=pose-local;
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
 mesh_->rotation.identity();mesh_->rotation.rotateY(boss.yaw+(boss.passage?1.570796327F:0));
 mesh_->translation.identity();mesh_->translation.translate(boss.position);
 e->renderer.renderer3D.usePipeline(pipeline_);pipeline_.render(mesh_.get(),options_);
}
void DroidPreview::unload(Tyra::Engine* e) {
 if(e)e->renderer.core.sync.align3D();
 pipeline_.core.clearGeometryCache();
 if(e && mesh_)e->renderer.getTextureRepository().freeByMesh(mesh_.get());
 button_.reset();mesh_.reset();std::vector<float>().swap(bank_);
}
}}}
