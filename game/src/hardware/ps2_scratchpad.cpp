#include "hardware/ps2_scratchpad.hpp"
#include <cstring>

namespace DroidCampos {
namespace Hardware {

Ps2ScratchpadArena::Ps2ScratchpadArena(bool useHardwareSpr)
    : offset_(0U), useHardwareSpr_(useHardwareSpr) {
  std::memset(fallbackBuffer_, 0, sizeof(fallbackBuffer_));
}

void Ps2ScratchpadArena::resetFrame() { offset_ = 0U; }

void* Ps2ScratchpadArena::allocAligned16(u32 bytes) {
  if (bytes == 0U) {
    return nullptr;
  }

  const u32 alignedOffset = (offset_ + 15U) & ~15U;
  const u32 alignedBytes = (bytes + 15U) & ~15U;

  if (alignedOffset + alignedBytes > kScratchpadSizeBytes) {
    return nullptr;
  }

  offset_ = alignedOffset + alignedBytes;

#if defined(_EE)
  if (useHardwareSpr_) {
    u8* hwBase = reinterpret_cast<u8*>(kPs2ScratchpadPhysAddr);
    return hwBase + alignedOffset;
  }
#endif

  return fallbackBuffer_ + alignedOffset;
}

}  // namespace Hardware
}  // namespace DroidCampos
