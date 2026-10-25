#pragma once

namespace DroidCampos {
namespace Core {
// Mirrors ETIP-inside.blend; meters, Tyra Y-up. Door order: L1,R1,L2,R2.
constexpr float kInsideDoorX[4] = {-1.6F, 1.6F, -1.6F, 1.6F};
constexpr float kInsideDoorZ[4] = {-8.0F, -8.0F, -16.0F, -16.0F};
constexpr float kDoorTravel = 1.55F;
constexpr float kDoorSpeed = 0.045F;
constexpr float kTerminalRoomMinX = -5.0F;
constexpr float kTerminalRoomMaxX = 5.0F;
constexpr float kTerminalRoomFrontZ = -24.0F;
constexpr float kTerminalRoomBackZ = -32.0F;
constexpr int kInsidePartCount = 10;
constexpr const char* kInsideParts[kInsidePartCount] = {
    "corredor_1", "corredor_2", "corredor_3", "sala_1", "sala_2",
    "sala_3", "sala_4", "sala_terminal", "terminal", "porta"};
}  // namespace Core
}  // namespace DroidCampos
