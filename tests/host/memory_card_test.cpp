#include "core/memory_card.hpp"
#include "core/world_state.hpp"
#include "systems/physics/interior_geometry.hpp"

#include <libmc.h>
#include <cassert>
#include <cstring>
#include <fileio.h>
#include <fileXio_rpc.h>
#include <map>
#include <string>
#include <vector>

namespace {

using namespace DroidCampos::Core;
constexpr char kCopyA[] = "/DROIDCAMPOS/save_a";
constexpr char kCopyB[] = "/DROIDCAMPOS/save_b";
std::map<std::string, std::vector<std::uint8_t>> files;
std::string openPath;
bool present = true, changed = true, formatted = true;
bool directory = false, full = false, interruptedWrite = false;
bool pending = false;
int rpcResult = 0;
int rpcDelay = 0;
int fileXioMode = FXIO_WAIT;

int post(int result) {
  assert(!pending);
  pending = true;
  rpcDelay = 1;
  rpcResult = result;
  return 0;
}

void settle(MemoryCard& card) {
  for (int i = 0; i < 240 && card.busy(); ++i) card.update(true);
  assert(!card.busy() && !pending);
}

void resetCard() {
  files.clear();
  openPath.clear();
  present = changed = formatted = true;
  directory = full = interruptedWrite = false;
}

CheckpointData checkpoint(Checkpoint stage) {
  return {stage, 7, 0, 43, 2};
}

}  // namespace

int mcInit(int type) {
  assert(type == MC_TYPE_XMC);
  return 0;
}

int mcSync(int mode, int*, int* result) {
  assert(mode == MC_NOWAIT && pending);
  if (rpcDelay-- > 0) return 0;
  pending = false;
  *result = rpcResult;
  return 1;
}

int mcGetInfo(int port, int slot, int* type, int* free, int* format) {
  assert(port == 0 && slot == 0);
  assert(!type && !free && !format);
  if (!present) return post(-12);
  if (!formatted) return post(sceMcResNoFormat);
  const bool wasChanged = changed;
  changed = false;
  return post(wasChanged ? sceMcResChangedCard : 0);
}

int mcMkDir(int port, int slot, const char* path) {
  assert(port == 0 && slot == 0 && std::string(path) == "/DROIDCAMPOS");
  if (!present) return post(-12);
  if (directory) return post(sceMcResDeniedPermit);
  if (full) return post(sceMcResFullDevice);
  directory = true;
  return post(0);
}

int mcOpen(int port, int slot, const char* path, int mode) {
  assert(port == 0 && slot == 0 && openPath.empty());
  if (!present) return post(-12);
  if ((mode & FIO_O_CREAT) && directory) {
    assert(mode & FIO_O_WRONLY);
    if (full) return post(sceMcResFullDevice);
    if (mode & FIO_O_TRUNC) files[path].clear();
  }
  if (!(mode & FIO_O_CREAT)) assert(mode == FIO_O_RDONLY);
  if (!files.count(path)) return post(sceMcResNoEntry);
  openPath = path;
  return post(0);
}

int fileXioInitSkipOverride() { return 0; }

void fileXioSetBlockMode(int mode) { fileXioMode = mode; }

int fileXioWaitAsync(int mode, int* result) {
  assert(mode == FXIO_NOWAIT && fileXioMode == FXIO_NOWAIT);
  return mcSync(MC_NOWAIT, nullptr, result) ? FXIO_COMPLETE : FXIO_INCOMPLETE;
}

int fileXioOpen(const char* path, int mode) {
  assert(fileXioMode == FXIO_NOWAIT && std::strncmp(path, "mc0:", 4) == 0);
  return mcOpen(0, 0, path + 4, mode);
}

int fileXioRead(int fd, void* bytes, int length) {
  assert(fileXioMode == FXIO_NOWAIT && fd == 0 && !openPath.empty());
  const auto& file = files.at(openPath);
  const auto size = std::min<int>(file.size(), length);
  if (size > 0) std::memcpy(bytes, file.data(), size);
  return post(size);
}

int fileXioClose(int fd) {
  assert(fileXioMode == FXIO_NOWAIT);
  return mcClose(fd);
}

int mcWrite(int fd, const void* bytes, int length) {
  assert(fd == 0 && !openPath.empty());
  auto* src = static_cast<const std::uint8_t*>(bytes);
  if (interruptedWrite) {
    files[openPath].assign(src, src + length / 2);
    present = false;
    changed = true;
    interruptedWrite = false;
    return post(-12);
  }
  files[openPath].assign(src, src + length);
  return post(length);
}

int mcFlush(int fd) {
  assert(fd == 0 && !openPath.empty());
  return post(0);
}

int mcClose(int fd) {
  assert(fd == 0 && !openPath.empty());
  openPath.clear();
  return post(present ? 0 : -12);
}

void checkRestore() {
  for (auto stage : {Checkpoint::EnteredEtip, Checkpoint::WavesStarted, Checkpoint::WavesCleared}) {
    auto data = checkpoint(stage);
    const auto bytes = encodeSave({data, 9});
    SaveRecord record;
    assert(decodeSave(bytes.data(), bytes.size(), record) == SaveStatus::Ready);
    assert(record.sequence == 9);
    assert(decodeSave(bytes.data(), bytes.size() - 1, record) == SaveStatus::Corrupt);
    auto damaged = bytes;
    damaged[16] ^= 1;
    assert(decodeSave(damaged.data(), damaged.size(), record) == SaveStatus::Corrupt);
    damaged = bytes;
    damaged[4] = 2;
    assert(decodeSave(damaged.data(), damaged.size(), record) == SaveStatus::Corrupt);
    auto invalid = encodeSave({{stage, 7, 2, 43, 2}, 9});
    assert(decodeSave(invalid.data(), invalid.size(), record) == SaveStatus::Corrupt);

    WorldState world{};
    world.resetToDefaults();
    world.scene.active = SceneId::Interior;
    world.resetPlayerToSpawn();
    restoreCheckpoint(world, record.data);
    assert(world.inventory.canEnter() && world.inventory.has(Pickup::RearMedkit));
    assert(!world.inventory.has(Pickup::SideRoomMedkit) && world.inventory.medkits == 0);
    assert(world.inventory.collect(Pickup::SideRoomMedkit));
    assert(world.droid.phase == DroidPhase::Inactive && !world.scene.terminalActive);
    assert(DroidCampos::Systems::Physics::interiorFree(world, world.player.playerPos.x,
                                                      world.player.playerPos.z, kPlayerBodyRadius));
    if (stage == Checkpoint::EnteredEtip) {
      assert(world.encounter.phase == EncounterPhase::Dormant && world.radio.popupActive);
    } else if (stage == Checkpoint::WavesStarted) {
      assert(world.encounter.phase == EncounterPhase::Countdown && !world.radio.popupActive);
      assert(world.encounter.countdownTicks == kEncounterDelay && world.encounter.checkpointValid);
      assert(world.encounter.checkpointPlayer.playerPos.z == world.player.playerPos.z);
      assert(world.encounter.checkpointInventory.medkits == 0);
      assert(world.playerHealth == kPlayerHealth && world.weapon.ammo == kMagazineSize);
    } else {
      assert(world.encounter.phase == EncounterPhase::Complete && world.encounter.finalRadioPending);
      assert(world.encounter.newEntryUnlocked && !world.encounter.objectiveComplete);
    }
    if (stage != Checkpoint::WavesStarted)
      assert(world.playerHealth == 43 && world.weapon.ammo == 2);
  }
}

int main() {
  checkRestore();
  resetCard();
  MemoryCard card;
  card.init(true);
  settle(card);
  assert(card.status() == SaveStatus::Empty);
  card.bindSession();
  card.queueSave(checkpoint(Checkpoint::EnteredEtip));
  assert(card.notice() == SaveNotice::Writing);
  settle(card);
  assert(card.notice() == SaveNotice::Saved && card.saved().sequence == 1);
  assert(files.count("/DROIDCAMPOS/icon.sys") && files.count("/DROIDCAMPOS/droid.ico"));

  // A full card leaves the previous record intact, and the next checkpoint can retry.
  full = true;
  card.queueSave(checkpoint(Checkpoint::WavesStarted));
  settle(card);
  assert(card.notice() == SaveNotice::Full && card.saved().data.stage == Checkpoint::EnteredEtip);
  full = false;
  card.queueSave(checkpoint(Checkpoint::WavesCleared));
  settle(card);
  assert(card.notice() == SaveNotice::Saved && card.saved().sequence == 2);

  MemoryCard afterBoot;
  afterBoot.init(true);
  settle(afterBoot);
  assert(afterBoot.saved().data.stage == Checkpoint::WavesCleared);
  const auto priorFiles = files;
  afterBoot.bindSession(false);
  present = false;
  afterBoot.refresh();
  settle(afterBoot);
  assert(afterBoot.status() == SaveStatus::NoCard);
  present = true;
  changed = true;
  const auto epochBeforeInsertion = afterBoot.mediaEpoch();
  afterBoot.refresh();
  settle(afterBoot);
  assert(afterBoot.status() == SaveStatus::Ready && afterBoot.mediaEpoch() != epochBeforeInsertion);
  settle(afterBoot);
  assert(afterBoot.notice() == SaveNotice::None && files == priorFiles);
  afterBoot.bindSession();  // Starting again alone must not modify the previous save.
  assert(files.at(kCopyB).size() == kSaveRecordSize);

  interruptedWrite = true;
  afterBoot.queueSave(checkpoint(Checkpoint::EnteredEtip));
  settle(afterBoot);
  assert(afterBoot.status() == SaveStatus::NoCard && afterBoot.notice() == SaveNotice::Disabled);
  present = true;
  afterBoot.refresh();
  settle(afterBoot);
  assert(afterBoot.saved().data.stage == Checkpoint::WavesCleared);
  afterBoot.queueSave(checkpoint(Checkpoint::WavesStarted));
  settle(afterBoot);
  assert(afterBoot.notice() == SaveNotice::Disabled && afterBoot.saved().sequence == 2);

  // A confirmed replacement starts its own campaign without erasing the backup.
  afterBoot.bindSession();
  afterBoot.queueSave(checkpoint(Checkpoint::EnteredEtip));
  settle(afterBoot);
  assert(afterBoot.saved().data.stage == Checkpoint::EnteredEtip && afterBoot.saved().sequence == 3);
  assert(files.at(kCopyA).size() == kSaveRecordSize && files.at(kCopyB).size() == kSaveRecordSize);

  files[kCopyA] = {68, 67, 80, 83, 2, 0, 64, 0, 3, 0, 0, 0, 1, 7, 0, 43, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 47, 245, 48, 70};
  afterBoot.refresh();
  settle(afterBoot);
  assert(afterBoot.status() == SaveStatus::Incompatible);

  formatted = false;
  afterBoot.refresh();
  settle(afterBoot);
  assert(afterBoot.status() == SaveStatus::InvalidCard);

  resetCard();
  files[kCopyA] = {0, 0};
  directory = true;
  MemoryCard damaged;
  damaged.init(true);
  settle(damaged);
  assert(damaged.status() == SaveStatus::Corrupt);
  damaged.bindSession();
  damaged.queueSave(checkpoint(Checkpoint::EnteredEtip));
  settle(damaged);
  assert(damaged.status() == SaveStatus::Ready && damaged.notice() == SaveNotice::Saved);
}
