#pragma once
#include <cassert>
#include <map>
#include <vector>
using u32=unsigned;
#define TYRA_ASSERT(condition,...) assert(condition)
namespace Tyra {
struct Sprite {u32 id=0;};
struct Texture {unsigned id=0;};
struct RendererCoreTextureBuffers {void* clut=nullptr;};
struct RendererCore {
  struct Textures {
    struct Repository {std::map<u32,Texture*> links;Texture* getBySpriteId(u32 id){return links.at(id);}}repository;
    std::vector<unsigned> uploads;unsigned clutUpdates=0;
    RendererCoreTextureBuffers useTexture(Texture* texture){uploads.push_back(texture->id);return {};}
    void updateClutBuffer(void*){++clutUpdates;}
  }texture;
  struct Draw2D {
    std::vector<unsigned> groups,draws,textures;
    void renderBatch(const Sprite* sprites,u32 count,const RendererCoreTextureBuffers&,Texture* texture) {
      groups.push_back(count);textures.push_back(texture->id);
      for(unsigned i=0;i<count;++i)draws.push_back(sprites[i].id);
    }
  }renderer2D;
};
class Renderer2D {
 public:
  Renderer2D();~Renderer2D();void init(RendererCore*);
  void render(const Sprite*);void render(const Sprite&);void renderBatch(const Sprite*,u32);
 private:RendererCore* core=nullptr;
};
}
