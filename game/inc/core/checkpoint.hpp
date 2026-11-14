#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace DroidCampos { namespace Core {

enum class Checkpoint : std::uint8_t { None, EnteredEtip, WavesStarted, WavesCleared };
enum class SaveStatus : std::uint8_t {
  Checking, Empty, Ready, NoCard, InvalidCard, Corrupt, Incompatible, Error
};
enum class SaveNotice : std::uint8_t { None, Writing, Saved, Failed, Full, Disabled };

struct CheckpointData {
  Checkpoint stage = Checkpoint::None;
  std::uint8_t collected = 0;
  std::uint8_t medkits = 0;
  std::uint8_t health = 0;
  std::uint8_t ammo = 0;
};

struct SaveRecord {
  CheckpointData data;
  std::uint32_t sequence = 0;
};

constexpr std::size_t kSaveRecordSize = 64;
using SaveBytes = std::array<std::uint8_t, kSaveRecordSize>;

struct WorldState;
SaveBytes encodeSave(const SaveRecord& record);
SaveStatus decodeSave(const std::uint8_t* bytes, std::size_t size, SaveRecord& record);
CheckpointData captureCheckpoint(const WorldState& state, Checkpoint stage);
void restoreCheckpoint(WorldState& state, const CheckpointData& data);

inline const char* checkpointName(Checkpoint stage) {
  switch (stage) {
    case Checkpoint::EnteredEtip: return "ENTRADA NA ETIP";
    case Checkpoint::WavesStarted: return "INICIO DAS ONDAS";
    case Checkpoint::WavesCleared: return "ONDAS CONCLUIDAS";
    default: return "";
  }
}

} }
