#pragma once
#include <algorithm>
#include <cstdint>

namespace DroidCampos { namespace Systems { namespace Enemy {
constexpr std::uint32_t kEnemyTicksPerSecond = 15625;
constexpr std::uint32_t kEnemyCycleTicks = 75522;
constexpr int kEnemyVariantCount = 2;
constexpr int kEnemyFrameCount = 18;
enum class TestAction { Ready, Run, Shoot };
struct TestPose {
  TestAction action;
  int from, to;
  float blend, x, z, yaw;
};
inline TestPose sampleEnemyTest(std::uint32_t ticks, int variant) {
  auto t = (ticks + static_cast<std::uint32_t>(variant) * 11719U) % kEnemyCycleTicks;
  TestPose result{TestAction::Ready, 0, 1, 0, variant == 0 ? -1.5F : 0.2F,
                  -30.8F, 3.1415926535F};
  float frame = 0;
  int base = 0, count = 4;
  if (t < 31250U) {
    frame = t * (4.0F / 31250.0F);
  } else if (t < 41667U) {
    t -= 31250U; result.action = TestAction::Run; base = 4; count = 8;
    frame = t * (8.0F / 10417.0F);
    result.z += std::min(2.0F, t * (3.0F / kEnemyTicksPerSecond));
  } else if (t < 57292U) {
    t -= 41667U; result.z += 2.0F; frame = t * (4.0F / 31250.0F);
  } else if (t < 65105U) {
    t -= 57292U; result.action = TestAction::Shoot; result.z += 2.0F;
    base = 12; count = 6; frame = std::min(5.0F, t * (5.0F / 7813.0F));
  } else {
    t -= 65105U; result.action = TestAction::Run; base = 4; count = 8;
    result.yaw = 0; frame = t * (8.0F / 10417.0F);
    result.z += std::max(0.0F, 2.0F - t * (3.0F / kEnemyTicksPerSecond));
  }
  int index = static_cast<int>(frame);
  index = std::min(index, count - 1);
  result.from = base + index;
  result.to = base + (index + 1 < count ? index + 1 :
                      result.action == TestAction::Shoot ? index : 0);
  result.blend = frame - index;
  return result;
}
}}}
