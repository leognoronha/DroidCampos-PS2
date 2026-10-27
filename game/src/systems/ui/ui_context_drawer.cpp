#include "systems/ui/ui_context_drawer.hpp"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace DroidCampos {
namespace Systems {
namespace Ui {

using namespace Tyra;

UiContextDrawer::UiContextDrawer() : engine_(nullptr), initialized_(false) {}

void UiContextDrawer::init(Engine* engine) {
  if (initialized_ || engine == nullptr) {
    return;
  }
  engine_ = engine;

  auto& repo = engine_->renderer.getTextureRepository();

  font_.load(repo, &engine_->renderer.renderer2D);

  bgSprite_.mode = SpriteMode::MODE_REPEAT;
  bgSprite_.size = Vec2(512.0F, 448.0F);
  bgSprite_.position = Vec2(0.0F, 0.0F);
  bgSprite_.color = Color(128.0F, 128.0F, 128.0F, 128.0F);
  auto* bgTex = repo.add(FileUtils::fromCwd("ui_blueprint.png"));
  bgTex->addLink(bgSprite_.id);

  menuButtons_.mode = SpriteMode::MODE_REPEAT;
  menuButtons_.size = Vec2(216.0F, 56.0F);
  menuButtons_.color = Color(128, 128, 128, 128);
  repo.add(FileUtils::fromCwd("ui_menu_buttons.png"))->addLink(menuButtons_.id);

  uiRectSprite_.mode = SpriteMode::MODE_STRETCH;
  uiRectSprite_.size = Vec2(16.0F, 16.0F);
  uiRectSprite_.position = Vec2(0.0F, 0.0F);
  auto* whiteTex = repo.add(FileUtils::fromCwd("ui_white.png"));
  whiteTex->addLink(uiRectSprite_.id);

  initialized_ = true;
}

void UiContextDrawer::shutdown(Engine* engine) {
  if (!initialized_) {
    return;
  }
  Engine* targetEngine = (engine != nullptr) ? engine : engine_;
  if (targetEngine != nullptr) {
    auto& repo = targetEngine->renderer.getTextureRepository();
    repo.freeBySprite(bgSprite_);
    repo.freeBySprite(menuButtons_);
    repo.freeBySprite(uiRectSprite_);
    font_.free(repo);
  }
  engine_ = nullptr;
  initialized_ = false;
}

void UiContextDrawer::drawRect(float x, float y, float w, float h,
                               const Color& color) {
  if (engine_ == nullptr) {
    return;
  }
  uiRectSprite_.position.set(x, y);
  uiRectSprite_.size.set(w, h);
  uiRectSprite_.scale = 1.0F;
  uiRectSprite_.color = color;
  engine_->renderer.renderer2D.render(uiRectSprite_);
}

void UiContextDrawer::drawPanel(float x, float y, float w, float h,
                                const Color& bgColor,
                                const Color& borderColor) {
  drawRect(x, y, w, h, bgColor);
  drawRect(x, y, w, 2.0F, borderColor);
  drawRect(x, y + h - 2.0F, w, 2.0F, borderColor);
  drawRect(x, y, 2.0F, h, borderColor);
  drawRect(x + w - 2.0F, y, 2.0F, h, borderColor);
  drawRect(x, y, 8.0F, 4.0F, borderColor);
  drawRect(x + w - 8.0F, y, 8.0F, 4.0F, borderColor);
  drawRect(x, y + h - 4.0F, 8.0F, 4.0F, borderColor);
  drawRect(x + w - 8.0F, y + h - 4.0F, 8.0F, 4.0F, borderColor);
}

// Chamfered panels for menu pages. Keep drawPanel unchanged for the FPS HUD.
void UiContextDrawer::drawBlueprintPanel(float x, float y, float w, float h,
                                         const Color& fill, const Color& edge) {
  const int cut = 8;
  drawRect(x, y + cut, w, h - cut * 2, fill);
  Sprite fills[16];
  for (int i = 0; i < cut; ++i) {
    for (int side = 0; side < 2; ++side) {
      auto& sprite = fills[i * 2 + side];
      sprite = uiRectSprite_;
      sprite.position.set(x + cut - i, side == 0 ? y + i : y + h - i - 1);
      sprite.size.set(w - 2 * (cut - i), 1);
      sprite.color = fill;
    }
  }
  if (engine_) engine_->renderer.renderer2D.renderBatch(fills, 16);
  drawRect(x + cut, y, w - cut * 2, 1, edge);
  drawRect(x + cut, y + h - 1, w - cut * 2, 1, edge);
  drawRect(x, y + cut, 1, h - cut * 2, edge);
  drawRect(x + w - 1, y + cut, 1, h - cut * 2, edge);
  // Submit the four diagonal corners in one bounded sprite batch.
  Sprite corners[32];
  for (int i = 0; i < cut; ++i) {
    const float px[4] = {x + cut - i - 1, x + w - cut + i,
                         x + i, x + w - i - 1};
    const float py[4] = {y + i, y + i, y + h - cut + i,
                         y + h - cut + i};
    for (int j = 0; j < 4; ++j) {
      auto& sprite = corners[i * 4 + j];
      sprite = uiRectSprite_;
      sprite.position.set(px[j], py[j]);
      sprite.size.set(1, 1);
      sprite.color = edge;
    }
  }
  if (engine_) engine_->renderer.renderer2D.renderBatch(corners, 32);
}

void UiContextDrawer::renderBackground(const Core::WorldState& state) {
  if (!engine_) return;
  // Black clear color belongs to the menu pass. Do not cover the future 3D
  // viewport with an opaque full-screen background sprite.
  if (state.menu.currentScreen == Core::ScreenState::MainMenu) {
    bgSprite_.position.set(0, 100);
    bgSprite_.offset.set(0, 100);
    bgSprite_.size.set(512, 294);
    engine_->renderer.renderer2D.render(bgSprite_);
    if (state.menu.scanlinesEnabled && state.anim.scanlineY >= 140 &&
        state.anim.scanlineY < 374)
      drawRect(28, state.anim.scanlineY, 212, 1, Color(99, 180, 179, 20));
  }
}

void UiContextDrawer::renderHeader() {
  if (!engine_) return;
  bgSprite_.position.set(0, 0);
  bgSprite_.offset.set(0, 0);
  bgSprite_.size.set(512, 100);
  engine_->renderer.renderer2D.render(bgSprite_);
}

void UiContextDrawer::renderFooter(const char* controlsHint) {
  if (!engine_) return;
  bgSprite_.position.set(0, 394);
  bgSprite_.offset.set(0, 394);
  bgSprite_.size.set(512, 54);
  engine_->renderer.renderer2D.render(bgSprite_);
  font_.drawText(controlsHint, 20, 416, Color(80, 120, 116, 128), 1.0F);
}

void UiContextDrawer::renderMenuButton(int index, bool selected) {
  if (!engine_ || index < 0 || index > 2) return;
  menuButtons_.position.set(26, 148 + index * 70);
  menuButtons_.offset.set(selected ? 256 : 0, index * 64);
  engine_->renderer.renderer2D.render(menuButtons_);
}

void UiContextDrawer::renderSync(const Core::WorldState& state) {
  if (!engine_) return;
  // Startup synchronization is decorative. Loading is actual scene IO progress.
  const float ratio = state.menu.currentScreen == Core::ScreenState::Loading
      ? (state.scene.totalSteps > 0
          ? static_cast<float>(state.scene.loadStep) / state.scene.totalSteps : 0.0F)
      : state.anim.animTick / 12.0F;
  const int percent = static_cast<int>(std::max(0.0F, std::min(1.0F, ratio)) * 100);
  char value[8];
  std::snprintf(value, sizeof(value), "%d%%", percent);
  font_.drawText(value, 391, 421, Color(80, 120, 116, 128), 1.0F);
  Sprite segments[14];
  for (int i = 0; i < 14; ++i) {
    segments[i] = uiRectSprite_;
    segments[i].position.set(430 + i * 4, 422);
    segments[i].size.set(2, 8);
    segments[i].color = (i + 1) * 100 <= percent * 14
        ? Color(80, 120, 116, 128) : Color(12, 24, 25, 128);
  }
  engine_->renderer.renderer2D.renderBatch(segments, 14);
}

Font& UiContextDrawer::getFont() { return font_; }

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
