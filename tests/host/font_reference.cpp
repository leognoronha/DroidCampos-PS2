// Frozen pre-optimization font implementation for layout equivalence tests.
#define Font ReferenceFont
#include "systems/ui/font_sprite.hpp"

namespace DroidCampos {

using namespace Tyra;

const int Font::chars[FONT_CHAR_SIZE]{
    ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-',
    '.', '/', '0', '1', '2', '3', '4', '5',  '6', '7', '8', '9', ':', ';',
    '<', '=', '>', '?', '@', 'A', 'B', 'C',  'D', 'E', 'F', 'G', 'H', 'I',
    'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',  'R', 'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', ' ', ' ', ' ', '^', '_',  ' ', 'a', 'b', 'c', 'd', 'e',
    'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',  'n', 'o', 'p', 'q', 'r', 's',
    't', 'u', 'v', 'w', 'x', 'y', 'z', '[',  '|', ']', '~', ' '};

const int Font::charWidths[FONT_CHAR_SIZE]{
    0, 1, 3, 5, 5, 9, 7, 2, 3, 3, 5, 5, 2, 4, 1, 4, 4, 2, 4, 4, 5, 4, 4, 4,
    4, 4, 1, 2, 4, 5, 4, 4, 4, 6, 5, 5, 5, 4, 4, 5, 5, 1, 4, 5, 4, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 6, 7, 5, 5, 4, 0, 0, 0, 6, 6, 0, 5, 4, 4, 4, 4, 3, 4,
    4, 1, 2, 4, 1, 7, 4, 4, 4, 4, 3, 4, 3, 4, 5, 7, 4, 4, 4, 2, 5, 2, 6, 0,
};

Font::Font() : renderer2D(nullptr) {}

void Font::load(TextureRepository& repository, Renderer2D* renderer) {
  renderer2D = renderer;

  float height = 16.0F;
  float width = 16.0F;

  allFont.mode = MODE_REPEAT;
  allFont.size = Vec2(255.0F, 127.0F);

  auto filepath = FileUtils::fromCwd("earthbound-Font.png");
  auto* texture = repository.add(filepath);
  texture->addLink(allFont.id);

  int column = 0;
  int row = 0;

  for (int i = 0; i < FONT_CHAR_SIZE; i++) {
    font[i].id = allFont.id;
    font[i].mode = MODE_REPEAT;
    font[i].size = Vec2(width, height);
    font[i].offset = Vec2(width * column, height * row);
    column++;

    if (column == 16) {
      row++;
      column = 0;
    }
  }
}

void Font::free(TextureRepository& repository) {
  repository.freeBySprite(allFont);
}

void Font::drawText(const char* text, const int& x, const int& y, Color color,
                    float scale) {
  drawText(std::string(text), x, y, color, scale);
}

void Font::drawText(const std::string& text, const int& x, const int& y,
                    Color color, float scale) {
  int sizeText = static_cast<int>(text.size());

  float offsetY = 0.0F;
  float offsetX = 0.0F;

  for (int i = 0; i < sizeText; i++) {
    char ch = text[i];
    if (ch == '\n') {
      offsetY += 18.0F * scale;
      offsetX = 0.0F;
      continue;
    }

    if (ch == ' ' || ch == '\t') {
      offsetX += 4.0F * scale;
      continue;
    }

    int foundIdx = -1;
    for (int j = 0; j < FONT_CHAR_SIZE; j++) {
      if (ch == chars[j]) {
        foundIdx = j;
        break;
      }
    }

    if (foundIdx != -1) {
      Sprite fontSpr = font[foundIdx];
      fontSpr.color = color;
      fontSpr.scale = scale;
      fontSpr.position = Vec2(static_cast<float>(x) + offsetX,
                              static_cast<float>(y) + offsetY);
      renderer2D->render(fontSpr);
      offsetX += static_cast<float>(charWidths[foundIdx] + 2) * scale;
    } else {
      offsetX += 4.0F * scale;
    }
  }
}

}  // namespace DroidCampos

namespace DroidCampos {
void referenceFontDraw(Tyra::Renderer2D& renderer, Tyra::TextureRepository& repository,
                       const std::string& text, int x, int y, Tyra::Color color, float scale) {
  ReferenceFont font; font.load(repository, &renderer);
  font.drawText(text, x, y, color, scale); font.free(repository);
}
}
