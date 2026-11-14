#include "core/checkpoint.hpp"
#include "core/world_state.hpp"

namespace DroidCampos { namespace Core {
namespace {

std::uint32_t read32(const std::uint8_t* bytes) {
  return bytes[0] | (std::uint32_t(bytes[1]) << 8) |
      (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
}

void write32(std::uint8_t* bytes, std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) bytes[i] = value >> (8 * i);
}

std::uint32_t checksum(const std::uint8_t* bytes, std::size_t size) {
  std::uint32_t crc = 0xffffffffU;
  while (size--) {
    crc ^= *bytes++;
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0);
  }
  return ~crc;
}

bool valid(const CheckpointData& data) {
  const int pickedUpKits = ((data.collected >> 2) & 1) + ((data.collected >> 3) & 1);
  return data.stage >= Checkpoint::EnteredEtip && data.stage <= Checkpoint::WavesCleared &&
      (data.collected & 3) == 3 && data.collected < 16 && data.medkits <= pickedUpKits &&
      data.health > 0 && data.health <= kPlayerHealth && data.ammo <= kMagazineSize;
}

}  // namespace

SaveBytes encodeSave(const SaveRecord& record) {
  SaveBytes bytes = {'D', 'C', 'P', 'S', 1, 0, kSaveRecordSize, 0};
  write32(bytes.data() + 8, record.sequence);
  bytes[12] = static_cast<std::uint8_t>(record.data.stage);
  bytes[13] = record.data.collected;
  bytes[14] = record.data.medkits;
  bytes[15] = record.data.health;
  bytes[16] = record.data.ammo;
  write32(bytes.data() + bytes.size() - 4, checksum(bytes.data(), bytes.size() - 4));
  return bytes;
}

SaveStatus decodeSave(const std::uint8_t* bytes, std::size_t size, SaveRecord& record) {
  if (size < 8 || bytes[0] != 'D' || bytes[1] != 'C' || bytes[2] != 'P' || bytes[3] != 'S')
    return SaveStatus::Corrupt;
  const auto version = bytes[4] | (unsigned(bytes[5]) << 8);
  const auto declaredSize = bytes[6] | (unsigned(bytes[7]) << 8);
  // Larger revisions may exceed our read buffer. Keep these files untouched.
  if (version != 1 && declaredSize > kSaveRecordSize && size > kSaveRecordSize)
    return SaveStatus::Incompatible;
  if (size != kSaveRecordSize || declaredSize != size ||
      checksum(bytes, size - 4) != read32(bytes + size - 4)) return SaveStatus::Corrupt;
  if (version != 1) return SaveStatus::Incompatible;
  SaveRecord decoded{{static_cast<Checkpoint>(bytes[12]), bytes[13], bytes[14],
                      bytes[15], bytes[16]}, read32(bytes + 8)};
  if (!decoded.sequence || !valid(decoded.data)) return SaveStatus::Corrupt;
  record = decoded;
  return SaveStatus::Ready;
}

CheckpointData captureCheckpoint(const WorldState& state, Checkpoint stage) {
  CheckpointData data{};
  data.stage = stage;
  for (std::size_t i = 0; i < state.inventory.collected.size(); ++i)
    if (state.inventory.collected[i]) data.collected |= 1U << i;
  data.medkits = state.inventory.medkits;
  data.health = state.playerHealth;
  data.ammo = state.weapon.ammo;
  return data;
}

void restoreCheckpoint(WorldState& state, const CheckpointData& data) {
  for (std::size_t i = 0; i < state.inventory.collected.size(); ++i)
    state.inventory.collected[i] = (data.collected & (1U << i)) != 0;
  state.inventory.medkits = data.medkits;
  state.playerHealth = data.health;
  state.weapon.ammo = data.ammo;
  if (data.stage == Checkpoint::EnteredEtip) return;

  auto& player = state.player;
  auto& encounter = state.encounter;
  const bool startingWaves = data.stage == Checkpoint::WavesStarted;
  player.playerPos.set(startingWaves ? -1.95F : 0.0F, kDefaultEyeHeight,
                       startingWaves ? -28.0F : -22.0F, 1.0F);
  player.cameraPos = player.playerPos;
  player.cameraLookAt = player.playerPos;
  if (startingWaves) {
    player.yaw = kDefaultPi;
    player.cameraLookAt.x -= 1.0F;
  } else {
    player.cameraLookAt.z -= 1.0F;
  }

  state.radio.popupActive = false;
  state.radio.phase = RadioPhase::Idle;
  if (startingWaves) {
    state.playerHealth = kPlayerHealth;
    state.weapon.ammo = kMagazineSize;
    encounter.phase = EncounterPhase::Countdown;
    encounter.countdownTicks = kEncounterDelay;
    encounter.checkpointValid = true;
    encounter.checkpointPlayer = player;
    encounter.checkpointInventory = state.inventory;
    for (int i = 0; i < 4; ++i) encounter.checkpointDoors[i] = state.scene.doors[i];
  } else {
    encounter.phase = EncounterPhase::Complete;
    encounter.wave = sizeof(kWaveSizes) / sizeof(kWaveSizes[0]);
    encounter.newEntryUnlocked = true;
    encounter.finalRadioPending = true;
  }
}

} }
