#pragma once

#include <tyra>
#include "core/world_state.hpp"
#include "systems/ui/font_sprite.hpp"

namespace DroidCampos {
namespace Systems {
namespace Ui {

class UiContextDrawer {
 public:
  UiContextDrawer();
  ~UiContextDrawer() = default;

  void init(Tyra::Engine* engine);
  void shutdown(Tyra::Engine* engine);

  void drawRect(float x, float y, float w, float h, const Tyra::Color& color);
  void drawPanel(float x, float y, float w, float h,
                 const Tyra::Color& bgColor, const Tyra::Color& borderColor);
  void renderBackground(const Core::WorldState& state);
  void renderHeader();
  void renderFooter(const char* controlsHint);
  void renderCyborgPortrait(float x, float y, float animTick);

  Font& getFont();

 private:
  Tyra::Engine* engine_;
  Font font_;
  Tyra::Sprite bgSprite_;
  Tyra::Sprite uiRectSprite_;
  bool initialized_;
};

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
