#include "systems/ui/ui_context_drawer.hpp"
#include <cmath>

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

  bgSprite_.mode = SpriteMode::MODE_STRETCH;
  bgSprite_.size = Vec2(512.0F, 448.0F);
  bgSprite_.position = Vec2(0.0F, 0.0F);
  bgSprite_.color = Color(128.0F, 128.0F, 128.0F, 128.0F);
  auto* bgTex = repo.add(FileUtils::fromCwd("bg_cyber.png"));
  bgTex->addLink(bgSprite_.id);

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

void UiContextDrawer::renderBackground(const Core::WorldState& state) {
  if (engine_ == nullptr) {
    return;
  }
  engine_->renderer.renderer2D.render(bgSprite_);

  for (int i = 0; i < 6; i++) {
    float lineY =
        70.0F + static_cast<float>(i) * 60.0F + (state.anim.bgScrollOffset * 0.5F);
    if (lineY < 410.0F) {
      drawRect(0.0F, lineY, 512.0F, 1.0F, Color(0.0F, 80.0F, 110.0F, 28.0F));
    }
  }

  if (state.menu.scanlinesEnabled) {
    drawRect(0.0F, state.anim.scanlineY, 512.0F, 3.0F,
             Color(0.0F, 200.0F, 255.0F, 24.0F));
    drawRect(0.0F, state.anim.scanlineY + 3.0F, 512.0F, 1.0F,
             Color(255.0F, 180.0F, 0.0F, 18.0F));
  }
}

void UiContextDrawer::renderHeader() {
  drawPanel(16.0F, 14.0F, 480.0F, 64.0F, Color(10.0F, 16.0F, 26.0F, 112.0F),
            Color(0.0F, 190.0F, 230.0F, 120.0F));

  drawRect(20.0F, 18.0F, 6.0F, 56.0F, Color(230.0F, 140.0F, 10.0F, 128.0F));

  font_.drawText("DROID CAMPOS", 36, 22, Color(200.0F, 40.0F, 20.0F, 100.0F),
                 2.0F);
  font_.drawText("DROID CAMPOS", 34, 20, Color(0.0F, 230.0F, 255.0F, 128.0F),
                 2.0F);

  font_.drawText("AXIOM-7 HEAVY IND. // PROJECT S.I.G.M.A.", 35, 56,
                 Color(230.0F, 165.0F, 30.0F, 128.0F), 1.0F);

  font_.drawText("UNIT: DC-01", 380, 22, Color(180.0F, 220.0F, 230.0F, 128.0F),
                 1.0F);
  font_.drawText("SYNC: 98.4%", 380, 38, Color(60.0F, 220.0F, 120.0F, 128.0F),
                 1.0F);
  font_.drawText("SECTOR: ETIP-01", 356, 54,
                 Color(160.0F, 170.0F, 185.0F, 128.0F), 1.0F);
}

void UiContextDrawer::renderFooter(const char* controlsHint) {
  drawPanel(16.0F, 406.0F, 480.0F, 28.0F, Color(8.0F, 12.0F, 20.0F, 118.0F),
            Color(90.0F, 110.0F, 130.0F, 100.0F));
  font_.drawText(controlsHint, 28, 413, Color(190.0F, 215.0F, 230.0F, 128.0F),
                 1.0F);
}

void UiContextDrawer::renderCyborgPortrait(float x, float y, float animTick) {
  drawPanel(x, y, 228.0F, 304.0F, Color(12.0F, 18.0F, 28.0F, 112.0F),
            Color(0.0F, 160.0F, 200.0F, 115.0F));

  font_.drawText("BIOMETRIC FEED // DC-01", static_cast<int>(x + 14.0F),
                 static_cast<int>(y + 10.0F),
                 Color(0.0F, 210.0F, 240.0F, 128.0F));
  drawRect(x + 10.0F, y + 28.0F, 208.0F, 1.0F,
           Color(0.0F, 160.0F, 200.0F, 90.0F));

  float cx = x + 114.0F;
  float cy = y + 98.0F;
  float pulse = (std::sin(animTick * 2.0F) + 1.0F) * 0.5F;

  drawRect(cx - 64.0F, cy + 28.0F, 128.0F, 46.0F,
           Color(35.0F, 45.0F, 58.0F, 128.0F));
  drawRect(cx - 72.0F, cy + 34.0F, 22.0F, 36.0F,
           Color(90.0F, 65.0F, 20.0F, 128.0F));
  drawRect(cx + 50.0F, cy + 34.0F, 22.0F, 36.0F,
           Color(90.0F, 65.0F, 20.0F, 128.0F));

  drawRect(cx - 18.0F, cy + 40.0F, 36.0F, 24.0F,
           Color(15.0F, 25.0F, 35.0F, 128.0F));
  drawRect(cx - 12.0F, cy + 44.0F, 24.0F, 16.0F,
           Color(0.0F, 160.0F + pulse * 80.0F, 230.0F, 128.0F));

  drawRect(cx - 16.0F, cy + 14.0F, 32.0F, 16.0F,
           Color(25.0F, 32.0F, 42.0F, 128.0F));
  drawRect(cx - 24.0F, cy + 8.0F, 4.0F, 22.0F,
           Color(180.0F, 50.0F, 30.0F, 128.0F));
  drawRect(cx + 20.0F, cy + 8.0F, 4.0F, 22.0F,
           Color(0.0F, 180.0F, 220.0F, 128.0F));

  drawRect(cx - 32.0F, cy - 38.0F, 64.0F, 54.0F,
           Color(50.0F, 62.0F, 78.0F, 128.0F));
  drawRect(cx - 26.0F, cy - 44.0F, 52.0F, 8.0F,
           Color(70.0F, 85.0F, 100.0F, 128.0F));

  drawRect(cx - 28.0F, cy - 20.0F, 56.0F, 14.0F,
           Color(18.0F, 10.0F, 15.0F, 128.0F));
  drawRect(cx - 24.0F, cy - 17.0F, 48.0F, 8.0F,
           Color(220.0F, 40.0F + pulse * 60.0F, 20.0F, 128.0F));
  float eyeScanX = cx - 20.0F + std::sin(animTick * 3.0F) * 14.0F;
  drawRect(eyeScanX, cy - 17.0F, 10.0F, 8.0F,
           Color(255.0F, 230.0F, 120.0F, 128.0F));

  drawRect(cx - 20.0F, cy - 2.0F, 40.0F, 14.0F,
           Color(30.0F, 38.0F, 48.0F, 128.0F));
  drawRect(cx - 14.0F, cy + 2.0F, 6.0F, 8.0F,
           Color(120.0F, 140.0F, 160.0F, 128.0F));
  drawRect(cx - 3.0F, cy + 2.0F, 6.0F, 8.0F,
           Color(120.0F, 140.0F, 160.0F, 128.0F));
  drawRect(cx + 8.0F, cy + 2.0F, 6.0F, 8.0F,
           Color(120.0F, 140.0F, 160.0F, 128.0F));

  drawRect(x + 10.0F, y + 180.0F, 208.0F, 1.0F,
           Color(0.0F, 160.0F, 200.0F, 90.0F));

  font_.drawText("SUBJECT: DR. V. CAMPOS", static_cast<int>(x + 14.0F),
                 static_cast<int>(y + 188.0F),
                 Color(240.0F, 180.0F, 40.0F, 128.0F));
  font_.drawText("FORMER CHIEF MATHEMATICIAN", static_cast<int>(x + 14.0F),
                 static_cast<int>(y + 206.0F),
                 Color(180.0F, 200.0F, 215.0F, 128.0F));
  font_.drawText("BETRAYED BY AXIOM BOARD.", static_cast<int>(x + 14.0F),
                 static_cast<int>(y + 224.0F),
                 Color(180.0F, 200.0F, 215.0F, 128.0F));
  font_.drawText("REBUILT IN CHASSIS DC-01", static_cast<int>(x + 14.0F),
                 static_cast<int>(y + 242.0F),
                 Color(180.0F, 200.0F, 215.0F, 128.0F));
  font_.drawText("TARGET: ETIP COMPLEX.", static_cast<int>(x + 14.0F),
                 static_cast<int>(y + 260.0F),
                 Color(0.0F, 220.0F, 250.0F, 128.0F));
  font_.drawText("STATUS: VENGEANCE_ACTIVE", static_cast<int>(x + 14.0F),
                 static_cast<int>(y + 280.0F),
                 Color(240.0F, 70.0F, 50.0F, 128.0F));
}

Font& UiContextDrawer::getFont() { return font_; }

}  // namespace Ui
}  // namespace Systems
}  // namespace DroidCampos
