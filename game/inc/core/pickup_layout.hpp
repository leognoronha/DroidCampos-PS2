#pragma once

#include "core/world_state.hpp"

namespace DroidCampos::Core {

struct PickupPlacement {
  Pickup item;
  SceneId scene;
  float x, z;
  const char* prompt;
};

constexpr float kPickupRadius = 1.15F;
// Indexed by Pickup for the contextual HUD prompt.
constexpr PickupPlacement kPickups[] = {
    {Pickup::Pistol, SceneId::Exterior, -1.90F, 6.55F, "[QUADRADO] PEGAR PISTOLA"},
    {Pickup::AccessCard, SceneId::Exterior, -1.90F, 5.15F, "[QUADRADO] PEGAR CARTAO"},
    {Pickup::RearMedkit, SceneId::Exterior, -2.20F, 17.50F, "[QUADRADO] PEGAR KIT MEDICO"},
    {Pickup::SideRoomMedkit, SceneId::Interior, -3.45F, -7.55F, "[QUADRADO] PEGAR KIT MEDICO"},
};

}  // namespace DroidCampos::Core
