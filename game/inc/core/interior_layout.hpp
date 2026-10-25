#pragma once

namespace DroidCampos {
namespace Core {
// Mirrors ETIP-inside.blend; meters, Tyra Y-up. Door order: L1,R1,L2,R2.
constexpr float kInsideDoorX[4] = {-1.6F, 1.6F, -1.6F, 1.6F};
constexpr float kInsideDoorZ[4] = {-8.0F, -8.0F, -16.0F, -16.0F};
constexpr float kDoorTravel = 1.55F;
constexpr float kDoorSpeed = 0.045F;
constexpr const char* kInsideParts[9] = {
    "corredor_1", "corredor_2", "corredor_3", "sala_1", "sala_2",
    "sala_3", "sala_4", "terminal", "porta"};
}  // namespace Core
}  // namespace DroidCampos
