#pragma once
#include <algorithm>
#include <cassert>
#include <vector>
using u8=unsigned char;using u32=unsigned;
#define TYRA_ASSERT(condition,...) assert(condition)
enum {P2_TYPE_NORMAL,P2_MODE_NORMAL,DMA_CHANNEL_GIF,PRIM_TRIANGLE,PRIM_SHADE_GOURAUD,DRAW_ENABLE,DRAW_DISABLE,PRIM_MAP_ST,PRIM_UNFIXED,LOD_USE_K,LOD_MAG_LINEAR,LOD_MIN_LINEAR,LOD_MIPMAP_REGISTER};
struct prim_t {int type,shading,mapping,fogging,blending,antialiasing,mapping_type,colorfix;};
struct lod_t {int calculation,max_level,mag_filter,min_filter,mipmap_select,l,k;};
struct clutbuffer_t {};
struct texrect_t {
  struct UV {float s=0,t=0;}t0,t1;
  struct Color {float r=0,g=0,b=0,a=0,q=0;}color;
  struct Vertex {float x=0,y=0;u32 z=0;}v0,v1;
};
struct qword_t {bool hasRect=false;texrect_t rect;};
struct packet2_t {std::vector<qword_t> storage;qword_t *base=nullptr,*next=nullptr;};
namespace HostPackets {
inline packet2_t* pending=nullptr;
inline std::vector<texrect_t> rects;
inline unsigned sends=0,maxQwords=0;
}
inline packet2_t* packet2_create(u32 size,int,int,int) {
  auto* p=new packet2_t;p->storage.resize(size);p->base=p->next=p->storage.data();return p;
}
inline void packet2_free(packet2_t* p){assert(p!=HostPackets::pending);delete p;}
inline void packet2_reset(packet2_t* p,bool){assert(p!=HostPackets::pending);std::fill(p->storage.begin(),p->storage.end(),qword_t{});p->next=p->base;}
inline void packet2_update(packet2_t* p,qword_t* next){p->next=next;assert(next<=p->base+p->storage.size());}
inline void packet2_utils_gif_add_set(packet2_t* p,int){++p->next;}
inline void packet2_utils_gs_add_lod(packet2_t* p,lod_t*){++p->next;}
inline void packet2_utils_gs_add_texbuff_clut(packet2_t* p,void*,clutbuffer_t*){++p->next;}
inline qword_t* draw_primitive_xyoffset(qword_t* p,int,float,float){return p+1;}
inline qword_t* draw_rect_textured(qword_t* p,int,const texrect_t* r){p->hasRect=true;p->rect=*r;return p+4;}
inline qword_t* draw_finish(qword_t* p){return p+1;}
inline void draw_enable_blending(){}inline void draw_disable_blending(){}
inline u32 packet2_get_qw_count(packet2_t* p){return p->next-p->base;}
inline void dma_channel_wait(int,int) {
  auto* p=HostPackets::pending;if(!p)return;
  for(auto* q=p->base;q<p->next;++q)if(q->hasRect)HostPackets::rects.push_back(q->rect);
  HostPackets::pending=nullptr;
}
inline void dma_channel_send_packet2(packet2_t* p,int,bool) {
  assert(!HostPackets::pending);HostPackets::pending=p;++HostPackets::sends;
  HostPackets::maxQwords=std::max(HostPackets::maxQwords,packet2_get_qw_count(p));
}
namespace Tyra {
enum {MODE_REPEAT,MODE_STRETCH};
struct Vec2 {float x=0,y=0;};
struct Color {float r=0,g=0,b=0,a=0;};
struct Sprite {int mode=MODE_REPEAT;Vec2 size,offset,position;Color color;float scale=1;bool flipHorizontal=false,flipVertical=false;};
struct RendererSettings {float getWidth()const{return 640;}float getHeight()const{return 448;}};
struct RendererCoreTextureBuffers {void* core=nullptr;};
struct Texture {int getWidth()const{return 256;}int getHeight()const{return 128;}};
using PipelineTextureMappingType=int;
class RendererCore2D {
 public:
  RendererCore2D();~RendererCore2D();
  void init(RendererSettings*,clutbuffer_t*);
  void render(const Sprite&,const RendererCoreTextureBuffers&,Texture*);
  void renderBatch(const Sprite*,u32,const RendererCoreTextureBuffers&,Texture*);
  void setTextureMappingType(PipelineTextureMappingType);
  static constexpr u32 MaxSpritesPerBatch=32,PacketQuadwords=16*(MaxSpritesPerBatch+1);
 private:
  void setPrim();void setLod();void appendSprite(packet2_t*,const Sprite&,Texture*);
  prim_t prim;lod_t lod;static const float GS_DRAW_AREA,SCREEN_CENTER;
  u8 context;RendererSettings* settings;clutbuffer_t* clutBuffer;packet2_t* packets[2];texrect_t* rects[2];
};
}
