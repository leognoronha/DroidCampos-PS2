#pragma once

#include <cstdint>

namespace Tyra {

class AnalogAxis {
 public:
  static constexpr std::uint8_t kCenter = 128;
  static constexpr int kEngage = 36;
  static constexpr int kRelease = 32;

  std::uint8_t update(std::uint8_t raw) {
    const int delta = static_cast<int>(raw) - kCenter;
    const int magnitude = delta < 0 ? -delta : delta;
    const int sign = delta < 0 ? -1 : 1;
    if (magnitude <= kRelease) {
      armed_ = true;
      activeDirection_ = pendingDirection_ = 0;
      return kCenter;
    }
    if (!armed_) return kCenter;
    if (activeDirection_ == sign) return raw;
    activeDirection_ = 0;
    if (magnitude < kEngage) {
      pendingDirection_ = 0;
      return kCenter;
    }
    if (pendingDirection_ != sign) {
      pendingDirection_ = sign;
      return kCenter;
    }
    activeDirection_ = sign;
    pendingDirection_ = 0;
    return raw;
  }

  void reset() { *this = AnalogAxis{}; }

  static float normalize(std::uint8_t raw) {
    const int delta = static_cast<int>(raw) - kCenter;
    if (delta < -kRelease)
      return static_cast<float>(delta + kRelease) / (128 - kRelease);
    if (delta > kRelease)
      return static_cast<float>(delta - kRelease) / (127 - kRelease);
    return 0.0F;
  }

 private:
  bool armed_ = false;
  int activeDirection_ = 0;
  int pendingDirection_ = 0;
};

constexpr bool isAnalogPadMode(std::uint8_t mode) {
  return mode == 0x53 || mode == 0x73 || mode == 0x79;
}

constexpr bool isCompletePadPacket(int bytes, std::uint8_t status, std::uint8_t mode) {
  return status == 0 && (mode == 0x41 || isAnalogPadMode(mode)) &&
         bytes >= 2 + 2 * (mode & 0x0f);
}

}  // namespace Tyra
