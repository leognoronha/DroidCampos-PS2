#include <cassert>
#include <iostream>
#include "renderer/core/2d/renderer_core_2d.hpp"
namespace Tyra {void referenceSpriteRects(const Sprite*,u32,RendererSettings&,const RendererCoreTextureBuffers&,Texture*);}
using namespace Tyra;
static std::vector<float> fields(const texrect_t& r) {
  return {r.t0.s,r.t0.t,r.t1.s,r.t1.t,r.color.r,r.color.g,r.color.b,r.color.a,r.color.q,
          r.v0.x,r.v0.y,float(r.v0.z),r.v1.x,r.v1.y,float(r.v1.z)};
}
int main() {
  RendererSettings settings;RendererCoreTextureBuffers buffers;Texture texture;clutbuffer_t clut;
  std::vector<Sprite> sprites(97);
  for(unsigned i=0;i<sprites.size();++i) {
    auto& s=sprites[i];s.size={float(8+i%23),float(16+i%11)};s.position={float(i*3),float(i%5)};
    s.offset={float(i),float(i+1)};s.scale=.5F+float(i%3);s.color={float(i),27,38,64};
    s.flipHorizontal=i%2;s.flipVertical=i%3;s.mode=i%2?MODE_REPEAT:MODE_STRETCH;
  }
  for(unsigned count:{0U,1U,31U,32U,33U,64U,97U}) {
    HostPackets::rects.clear();HostPackets::sends=0;
    referenceSpriteRects(sprites.data(),count,settings,buffers,&texture);
    const auto reference=HostPackets::rects;assert(HostPackets::sends==count);
    HostPackets::rects.clear();HostPackets::sends=0;
    {
      RendererCore2D renderer;renderer.init(&settings,&clut);
      renderer.renderBatch(sprites.data(),count,buffers,&texture);
      dma_channel_wait(DMA_CHANNEL_GIF,0);
    }
    assert(HostPackets::rects.size()==reference.size());
    for(unsigned i=0;i<reference.size();++i)assert(fields(HostPackets::rects[i])==fields(reference[i]));
    assert(HostPackets::sends==(count+31)/32);
  }
  // Destruction must wait for its last packet before freeing either bank.
  {RendererCore2D renderer;renderer.init(&settings,&clut);renderer.render(sprites[0],buffers,&texture);}
  assert(!HostPackets::pending);
  std::cout<<"PASS: exact real 2D rectangle equivalence, scale/flips/UV/color/order, 0/1/31/32/33/64/97 sprites, batching and simulated asynchronous packet ownership\n";
}
