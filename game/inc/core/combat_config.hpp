#pragma once
#include <cstdint>

namespace DroidCampos { namespace Core {
constexpr int kMaxEnemies = 5;
constexpr int kPlayerHealth = 100;
constexpr int kEnemyHealth = 30;
constexpr int kShotDamage = 10;
constexpr int kWaveSizes[3] = {2, 3, 5};
constexpr int kWaveHealing = 25;
constexpr float kPlayerBodyRadius = .48F;
constexpr float kEnemyRadius = .35F;
constexpr float kEnemyHeight = 1.75F;
constexpr float kEnemySpeed = 3.0F;
constexpr float kCombatEnterRange = 6.0F;
constexpr float kCombatExitRange = 8.0F;
constexpr float kEnemySpreadRadians = .104719755F;
constexpr float kSpawnDistance = 3.0F;
constexpr std::uint32_t kCombatTicksPerSecond = 15625;
constexpr std::uint32_t kEncounterDelay = 46875;
constexpr std::uint32_t kWaveInterval = 78125;
constexpr std::uint32_t kEnemyReaction = 12500;
constexpr std::uint32_t kEnemyFireInterval = 12500;
constexpr std::uint32_t kDecisionInterval = 1563;
constexpr std::uint32_t kPathInterval = 7813;
constexpr std::uint32_t kAttackRotation = 31250;
enum class EnemyBehavior : std::uint8_t { Alert, Pursuit, Combat };
enum class EncounterPhase : std::uint8_t {
  Dormant, Armed, Countdown, Wave, Intermission, Complete
};
}}  // namespace DroidCampos::Core
