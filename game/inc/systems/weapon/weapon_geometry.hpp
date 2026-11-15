#pragma once
#include <tyra>
#include <algorithm>
#include <cmath>
#include <vector>
#include "core/world_state.hpp"
namespace DroidCampos {
namespace Systems {
namespace Weapon {
inline Tyra::Vec4 add(const Tyra::Vec4& a, const Tyra::Vec4& b) {
  return Tyra::Vec4(a.x + b.x, a.y + b.y, a.z + b.z, 1);
}
inline Tyra::Vec4 sub(const Tyra::Vec4& a, const Tyra::Vec4& b) {
  return Tyra::Vec4(a.x - b.x, a.y - b.y, a.z - b.z, 0);
}
inline Tyra::Vec4 mul(const Tyra::Vec4& a, float s) {
  return Tyra::Vec4(a.x * s, a.y * s, a.z * s, 0);
}
inline float dot(const Tyra::Vec4& a, const Tyra::Vec4& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline Tyra::Vec4 cross(const Tyra::Vec4& a, const Tyra::Vec4& b) {
  return Tyra::Vec4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x, 0);
}
inline Tyra::Vec4 unit(const Tyra::Vec4& a) {
  float n = std::sqrt(dot(a, a));
  return n > 1e-7F ? mul(a, 1 / n) : Tyra::Vec4(0, 0, -1, 0);
}
struct RayHit {
  float distance = 50.0F;
  Tyra::Vec4 position = Tyra::Vec4(0, 0, 0, 1);
  Tyra::Vec4 normal = Tyra::Vec4(0, 1, 0, 0);
};
// Non-owning deindexed arrays. Build bounds at load, clear before mesh
// destruction.
struct RayGeometry {
  const Tyra::Vec4* vertices;
  u32 count;
  int door;  // -1 static world space; 0..3 translated door instance
  Tyra::Vec4 min, max;
};
class WeaponRayScene {
 public:
  void clear() { geometry_.clear(); }
  void add(const Tyra::Vec4* vertices, u32 count, int door = -1) {
    if (!vertices || count < 3) return;
    RayGeometry item{vertices, count, door, vertices[0], vertices[0]};
    for (u32 i = 1; i < count; ++i) {
      item.min.x = std::min(item.min.x, vertices[i].x);
      item.max.x = std::max(item.max.x, vertices[i].x);
      item.min.y = std::min(item.min.y, vertices[i].y);
      item.max.y = std::max(item.max.y, vertices[i].y);
      item.min.z = std::min(item.min.z, vertices[i].z);
      item.max.z = std::max(item.max.z, vertices[i].z);
    }
    geometry_.push_back(item);
  }
  bool trace(const Core::WorldState& state, const Tyra::Vec4& origin,
             const Tyra::Vec4& direction, RayHit& result) const;
  u32 size() const { return static_cast<u32>(geometry_.size()); }

 private:
  std::vector<RayGeometry> geometry_;
};
class WeaponScene {
 public:
  virtual ~WeaponScene() = default;
  virtual bool traceWeapon(const Core::WorldState&, const Tyra::Vec4&,
                           const Tyra::Vec4&, RayHit&) const = 0;
  virtual Tyra::Vec4 weaponMuzzle(const Core::WorldState&) = 0;
};
}  // namespace Weapon
}  // namespace Systems
}  // namespace DroidCampos
