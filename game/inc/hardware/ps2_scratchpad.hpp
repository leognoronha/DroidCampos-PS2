#pragma once

#include <tyra>
#include <cstdint>

namespace DroidCampos {
namespace Hardware {

constexpr u32 kScratchpadSizeBytes = 16U * 1024U;
constexpr uintptr_t kPs2ScratchpadPhysAddr = 0x70000000U;

class alignas(64) Ps2ScratchpadArena {
 public:
  explicit Ps2ScratchpadArena(bool useHardwareSpr = false);
  ~Ps2ScratchpadArena() = default;

  void resetFrame();
  void* allocAligned16(u32 bytes);

  template <typename T>
  T* allocArray(u32 count) {
    if (count == 0U) {
      return nullptr;
    }
    return static_cast<T*>(allocAligned16(static_cast<u32>(sizeof(T)) * count));
  }

  u32 getUsedBytes() const { return offset_; }
  u32 getRemainingBytes() const {
    return (offset_ < kScratchpadSizeBytes) ? (kScratchpadSizeBytes - offset_)
                                            : 0U;
  }
  bool isUsingHardwareSpr() const { return useHardwareSpr_; }

 private:
  alignas(64) u8 fallbackBuffer_[kScratchpadSizeBytes];
  u32 offset_;
  bool useHardwareSpr_;
};

}  // namespace Hardware
}  // namespace DroidCampos
