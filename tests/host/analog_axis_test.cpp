#include <cassert>

#include "pad/analog_axis.hpp"

int main() {
  using Tyra::AnalogAxis;
  constexpr auto neutral = AnalogAxis::kCenter;
  AnalogAxis movement, camera;
  assert(movement.update(0) == neutral);
  assert(movement.update(0) == neutral);
  assert(movement.update(127) == neutral);
  assert(camera.update(128) == neutral);
  assert(movement.update(97) == neutral);
  assert(camera.update(159) == neutral);
  assert(movement.update(70) == neutral);
  assert(movement.update(128) == neutral);
  assert(movement.update(70) == neutral);
  assert(movement.update(70) == 70);
  assert(camera.update(153) == neutral);
  assert(movement.update(94) == 94);
  assert(movement.update(96) == neutral);
  assert(movement.update(94) == neutral);
  assert(movement.update(255) == neutral);
  assert(movement.update(255) == 255);
  assert(movement.update(0) == neutral);
  assert(movement.update(0) == 0);
  movement.reset();
  assert(movement.update(0) == neutral);
  assert(movement.update(128) == neutral);
  assert(movement.update(0) == neutral);
  assert(movement.update(0) == 0);

  assert(AnalogAxis::normalize(0) == -1.0F);
  assert(AnalogAxis::normalize(255) == 1.0F);
  assert(AnalogAxis::normalize(127) == 0.0F);
  assert(AnalogAxis::normalize(96) == 0.0F);
  assert(AnalogAxis::normalize(160) == 0.0F);
  assert(AnalogAxis::normalize(161) > 0.0F && AnalogAxis::normalize(161) < .02F);
  assert(Tyra::isCompletePadPacket(4, 0, 0x41) && !Tyra::isAnalogPadMode(0x41));
  assert(Tyra::isCompletePadPacket(8, 0, 0x73));
  assert(Tyra::isCompletePadPacket(20, 0, 0x79));
  assert(!Tyra::isCompletePadPacket(4, 0, 0x73));
  assert(!Tyra::isCompletePadPacket(20, 0xff, 0x79));
  assert(!Tyra::isCompletePadPacket(0, 0, 0x79));
}
