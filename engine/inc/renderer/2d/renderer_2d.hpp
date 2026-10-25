/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2022, tyra - https://github.com/h4570/tyra
# Licensed under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#pragma once

#include "renderer/core/2d/sprite/sprite.hpp"
#include "renderer/core/renderer_core.hpp"

namespace Tyra {

class Renderer2D {
 public:
  Renderer2D();
  ~Renderer2D();

  void init(RendererCore* rendererCore);
  void render(const Sprite* sprite);
  void render(const Sprite& sprite);
  // Consecutive sprites sharing a texture are submitted together, in order.
  void renderBatch(const Sprite* sprites, u32 count);

 private:
  RendererCore* core;
};

}  // namespace Tyra
