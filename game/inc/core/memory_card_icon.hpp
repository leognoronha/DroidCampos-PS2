#pragma once

#include <cstdint>

namespace DroidCampos { namespace Core {
namespace {

// Static diamond with a one-color 128 x 128 RLE texture in the PS2 browser format.
struct IconVertex {
  std::int16_t position[4];
  std::int16_t normal[4];
  std::int16_t uv[2];
  std::uint8_t color[4];
};

constexpr IconVertex iconVertex(int x, int y, int z, int facing) {
  return {{static_cast<std::int16_t>(x), static_cast<std::int16_t>(y),
           static_cast<std::int16_t>(z), 4096},
          {0, 0, static_cast<std::int16_t>(facing), 0},
          {0, 0}, {128, 128, 128, 128}};
}

alignas(64) constexpr struct {
  std::uint32_t header[5];
  IconVertex vertices[12];
  std::uint32_t animationVersion, animationLength;
  float animationSpeed;
  std::uint32_t playOffset, frames, shape, keys;
  float time, weight;
  std::uint32_t textureBytes;
  std::uint16_t repetitions, color;
} kMemoryCardIcon = {
    {0x00010000, 1, 15, 0, 12},
    {iconVertex(0, -4096, 0, 4096), iconVertex(-4096, 0, 0, 4096), iconVertex(4096, 0, 0, 4096),
     iconVertex(-4096, 0, 0, 4096), iconVertex(0, 4096, 0, 4096), iconVertex(4096, 0, 0, 4096),
     iconVertex(0, -4096, 0, -4096), iconVertex(4096, 0, 0, -4096), iconVertex(-4096, 0, 0, -4096),
     iconVertex(-4096, 0, 0, -4096), iconVertex(4096, 0, 0, -4096), iconVertex(0, 4096, 0, -4096)},
    1, 1, 1.0F, 0, 1, 0, 1, 0.0F, 1.0F,
    4, 128 * 128, 0xfe84
};
static_assert(sizeof(kMemoryCardIcon) == 352, "Unexpected PS2 icon layout");

}  // namespace
} }
