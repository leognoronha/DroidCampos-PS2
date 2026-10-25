#include <cassert>
#include <iostream>
#include "renderer/2d/renderer_2d.hpp"
using namespace Tyra;
int main() {
  RendererCore core;Renderer2D renderer;renderer.init(&core);
  Texture a{10},b{20};core.texture.repository.links={{1,&a},{2,&a},{3,&b}};
  Sprite sprites[]={{1},{2},{3},{1}};
  renderer.renderBatch(nullptr,0);assert(core.renderer2D.draws.empty());
  renderer.renderBatch(sprites,4);
  assert((core.renderer2D.groups==std::vector<unsigned>{2,1,1}));
  assert((core.renderer2D.textures==std::vector<unsigned>{10,20,10}));
  assert((core.renderer2D.draws==std::vector<unsigned>{1,2,3,1}));
  assert(core.texture.uploads==core.renderer2D.textures&&core.texture.clutUpdates==3);
  renderer.render(sprites);renderer.render(sprites[1]);
  assert(core.renderer2D.groups.size()==5&&core.renderer2D.draws.size()==6);
  std::cout<<"PASS: actual Renderer2D preserves A/A-alias/B/A texture order, empty input and legacy sprite APIs\n";
}
