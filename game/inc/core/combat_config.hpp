#pragma once
#include <cstdint>

namespace DroidCampos { namespace Core {
constexpr int kMaxEnemies = 5;
constexpr int kPlayerHealth = 100;
constexpr int kEnemyHealth = 30;
constexpr int kShotDamage = 10;
constexpr float kPlayerBodyRadius = .48F;
constexpr float kEnemyRadius = .35F;
constexpr float kEnemyHeight = 1.75F;
constexpr float kEnemySpeed = 3.0F;
constexpr std::uint32_t kCombatTicksPerSecond = 15625;
constexpr std::uint32_t kPathInterval = 7813;
enum class EnemyBehavior : std::uint8_t { Alert, Pursuit, Combat };
}}  // namespace DroidCampos::Core
