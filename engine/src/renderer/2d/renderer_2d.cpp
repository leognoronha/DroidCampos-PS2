/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2022, tyra - https://github.com/h4570/tyra
# Licensed under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#include "renderer/2d/renderer_2d.hpp"

namespace Tyra {

Renderer2D::Renderer2D() {}
Renderer2D::~Renderer2D() {}

void Renderer2D::init(RendererCore* t_rendererCore) { core = t_rendererCore; }

void Renderer2D::render(const Sprite* sprite) { render(*sprite); }

void Renderer2D::render(const Sprite& sprite) {
  renderBatch(&sprite, 1);
}

void Renderer2D::renderBatch(const Sprite* sprites, const u32 count) {
  u32 start = 0;
  while (start < count) {
    auto* texture = core->texture.repository.getBySpriteId(sprites[start].id);
    TYRA_ASSERT(texture, "Texture for sprite was not found in repository");
    u32 end = start + 1;
    while (end < count) {
      auto* next = core->texture.repository.getBySpriteId(sprites[end].id);
      TYRA_ASSERT(next, "Texture for sprite was not found in repository");
      if (next != texture) break;
      ++end;
    }
    auto texBuffers = core->texture.useTexture(texture);
    core->texture.updateClutBuffer(texBuffers.clut);
    core->renderer2D.renderBatch(sprites + start, end - start, texBuffers, texture);
    start = end;
  }
}

}  // namespace Tyra
