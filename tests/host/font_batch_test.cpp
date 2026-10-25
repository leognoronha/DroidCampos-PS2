#include <cassert>
#include <iostream>
#include <random>
#include "systems/ui/font_sprite.hpp"
namespace DroidCampos {
void referenceFontDraw(Tyra::Renderer2D&,Tyra::TextureRepository&,const std::string&,int,int,Tyra::Color,float);
}
using namespace DroidCampos;
static bool same(const Tyra::Vec2& a,const Tyra::Vec2& b){return a.x==b.x&&a.y==b.y;}
int main() {
  Tyra::Renderer2D renderer,reference;Tyra::TextureRepository repository,referenceRepository;
  Font font;font.load(repository,&renderer);unsigned cases=0;
  auto check=[&](const std::string& text,float scale) {
    renderer.draws.clear();renderer.batches.clear();reference.draws.clear();reference.batches.clear();
    Tyra::Color color{17,48,129,77};font.drawText(text,-7,23,color,scale);
    referenceFontDraw(reference,referenceRepository,text,-7,23,color,scale);
    assert(renderer.draws.size()==reference.draws.size());
    for(unsigned i=0;i<renderer.draws.size();++i) {
      const auto& a=renderer.draws[i];const auto& b=reference.draws[i];
      assert(same(a.position,b.position)&&same(a.offset,b.offset)&&same(a.size,b.size));
      assert(a.scale==b.scale&&a.mode==b.mode);
      assert(a.color.r==b.color.r&&a.color.g==b.color.g&&a.color.b==b.color.b&&a.color.a==b.color.a);
      assert(a.flipHorizontal==b.flipHorizontal&&a.flipVertical==b.flipVertical);
    }
    assert(renderer.batches.size()==(renderer.draws.size()+31)/32);++cases;
  };
  for(float scale:{0.F,.5F,1.F,1.5F,2.F}) {
    check("",scale);check("A\tB C\nDROID CAMPOS [READY] ~ _ ^",scale);
    for(int size:{1,31,32,33,64,65,1024})check(std::string(size,'A'),scale);
    std::string bytes;for(unsigned i=0;i<256;++i)bytes.push_back(char(i));check(bytes,scale);
  }
  std::mt19937 rng(79);
  for(int i=0;i<100;++i){std::string text;for(int j=0;j<i*7;++j)text.push_back(char(rng()%256));check(text,1.25F);}
  check(std::string(96,'A'),1.F);assert(renderer.batches.size()==3&&reference.batches.size()==96);
  font.free(repository);
  std::cout<<"PASS: "<<cases<<" exact font layout comparisons, all 256 bytes, fallback/spacing/color/scale, 96 glyph calls reduced to 3 batches\n";
}
