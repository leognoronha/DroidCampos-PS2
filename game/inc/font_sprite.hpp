#pragma once

#include <tyra>
#include <array>
#include <string>

#define FONT_CHAR_SIZE 96

namespace DroidCampos {

class Font {
 public:
  Font();
  void load(Tyra::TextureRepository& repository, Tyra::Renderer2D* renderer);
  void free(Tyra::TextureRepository& repository);
  void drawText(const char* text, const int& x, const int& y,
                Tyra::Color color, float scale = 1.0F);
  void drawText(const std::string& text, const int& x, const int& y,
                Tyra::Color color, float scale = 1.0F);

 private:
  const static int chars[FONT_CHAR_SIZE];
  const static int charWidths[FONT_CHAR_SIZE];

  Tyra::Renderer2D* renderer2D;
  Tyra::Sprite allFont;
  std::array<Tyra::Sprite, FONT_CHAR_SIZE> font;
};

}  // namespace DroidCampos
