#include "systems/weapon/weapon_geometry.hpp"
#include "core/interior_layout.hpp"
namespace DroidCampos {
namespace Systems {
namespace Weapon {
namespace {
bool bounds(const RayGeometry& g, const Tyra::Vec4& o, const Tyra::Vec4& d,
            float limit) {
  const float p[3] = {o.x, o.y, o.z}, v[3] = {d.x, d.y, d.z};
  const float lo[3] = {g.min.x, g.min.y, g.min.z},
              hi[3] = {g.max.x, g.max.y, g.max.z};
  float near = 0, far = limit;
  for (int i = 0; i < 3; ++i) {
    if (std::fabs(v[i]) < 1e-7F) {
      if (p[i] < lo[i] || p[i] > hi[i]) return false;
    } else {
      float a = (lo[i] - p[i]) / v[i], b = (hi[i] - p[i]) / v[i];
      if (a > b) std::swap(a, b);
      near = std::max(near, a);
      far = std::min(far, b);
      if (near > far) return false;
    }
  }
  return true;
}
}  // namespace
bool WeaponRayScene::trace(const Core::WorldState& state,
                           const Tyra::Vec4& origin,
                           const Tyra::Vec4& direction, RayHit& result) const {
  bool found = false;
  for (const auto& g : geometry_) {
    Tyra::Vec4 offset(0, 0, 0, 0);
    if (g.door >= 0)
      offset.set(Core::kInsideDoorX[g.door], 1.1F,
                 Core::kInsideDoorZ[g.door] -
                     state.scene.doors[g.door].openAmount * Core::kDoorTravel,
                 0);
    const auto local = sub(origin, offset);
    if (!bounds(g, local, direction, result.distance)) continue;
    for (u32 i = 0; i + 2 < g.count; i += 3) {
      const auto& a = g.vertices[i];
      const auto e1 = sub(g.vertices[i + 1], a), e2 = sub(g.vertices[i + 2], a);
      const auto p = cross(direction, e2);
      const float determinant = dot(e1, p);
      if (std::fabs(determinant) < 1e-7F)
        continue;  // two-sided, includes thin glass
      const float inv = 1 / determinant;
      const auto t = sub(local, a);
      const float u = dot(t, p) * inv;
      if (u < 0 || u > 1) continue;
      const auto q = cross(t, e1);
      const float v = dot(direction, q) * inv;
      if (v < 0 || u + v > 1) continue;
      const float distance = dot(e2, q) * inv;
      if (distance < 0.0001F || distance > result.distance) continue;
      result.actor = -2;
      result.distance = distance;
      result.position = Weapon::add(origin, mul(direction, distance));
      result.normal = unit(cross(e1, e2));
      if (dot(result.normal, direction) > 0)
        result.normal = mul(result.normal, -1);
      found = true;
    }
  }
  return found;
}
}  // namespace Weapon
}  // namespace Systems
}  // namespace DroidCampos
