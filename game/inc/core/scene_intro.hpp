#pragma once
#include "core/world_state.hpp"
#include <algorithm>
#include <cstring>

namespace DroidCampos {
namespace Core {

// Tyra Timer ticks at 15625 Hz. Loading time never advances this presentation.
constexpr u32 kIntroFadeTicks = 23438;       // 1.5 seconds.
constexpr u32 kIntroTextStartTicks = 5469;   // 0.35 seconds.
constexpr u32 kIntroCharacterTicks = 521;   // About 30 characters/second.
constexpr u32 kIntroTextHoldTicks = 15625;   // 1 second after the last character.
constexpr u32 kIntroTextFadeTicks = 7813;    // 0.5 seconds.
constexpr u32 kIntroRadioGraceTicks = 11719; // 0.75 seconds of unobstructed view.
constexpr const char* kIntroLocationText =
    "Ano 2066.\nDistrito Industrial UTINGA.\nMatriz ETIP Corporation.";

inline u32 introTextEndTicks() {
  return kIntroTextStartTicks + static_cast<u32>(std::strlen(kIntroLocationText)) * kIntroCharacterTicks;
}
inline u32 introDurationTicks(SceneId scene) {
  return scene == SceneId::Exterior
      ? introTextEndTicks() + kIntroTextHoldTicks + kIntroTextFadeTicks + kIntroRadioGraceTicks
      : kIntroFadeTicks + kIntroRadioGraceTicks;
}
inline int introVisibleCharacters(u32 ticks) {
  if (ticks <= kIntroTextStartTicks) return 0;
  return static_cast<int>(std::min(static_cast<u32>(std::strlen(kIntroLocationText)),
                                  (ticks - kIntroTextStartTicks) / kIntroCharacterTicks));
}
inline float introSmoothStep(float progress) {
  const float t = std::max(0.0F, std::min(1.0F, progress));
  return t * t * (3 - 2 * t);
}
inline float introBlackAlpha(u32 ticks) {
  return 128 * (1 - introSmoothStep(static_cast<float>(ticks) / kIntroFadeTicks));
}
inline float introTextAlpha(u32 ticks) {
  const u32 start = introTextEndTicks() + kIntroTextHoldTicks;
  return ticks <= start ? 128.0F
      : 128 * (1 - introSmoothStep(static_cast<float>(ticks - start) / kIntroTextFadeTicks));
}

}  // namespace Core
}  // namespace DroidCampos
