#pragma once

#include "core/checkpoint.hpp"

namespace DroidCampos { namespace Core {

class MemoryCard {
 public:
  void init(bool modulesLoaded);
  void update(bool inSaveMenu);
  bool busy() const;
  SaveStatus status() const { return status_; }
  const SaveRecord& saved() const { return latest_; }
  std::uint32_t mediaEpoch() const { return mediaEpoch_; }
  void refresh();
  void bindSession(bool enabled = true);
  void queueSave(const CheckpointData& data);
  SaveNotice notice() const { return notice_; }

 private:
  enum class Step {
    Idle, Info, OpenCopy, ReadCopy, CloseCopy, CheckScannedCard,
    MakeDirectory, OpenOutput, WriteOutput, FlushOutput, CloseOutput,
    CheckWrittenCard, OpenVerification, ReadVerification, CloseVerification
  };

  bool readingFile() const;
  void complete(int result);
  void finishScan();
  void loseCard(SaveStatus status);
  void writeFailed(int result);
  void beginWrite();
  void selectOutput(int index);

  Step step_ = Step::Idle;
  SaveStatus status_ = SaveStatus::Checking;
  SaveNotice notice_ = SaveNotice::None;
  bool available_ = false;
  bool pendingRpc_ = false;
  bool scanRequested_ = true;
  bool bound_ = false;
  bool writing_ = false;
  bool scanDamaged_ = false;
  bool scanIncompatible_ = false;
  bool scanError_ = false;
  int pollFrames_ = 0;
  std::uint32_t mediaEpoch_ = 0;
  int copy_ = 0;
  int latestCopy_ = -1;
  int fd_ = -1;
  int ioResult_ = 0;
  int outputIndex_ = 0;
  int outputSize_ = 0;
  const char* outputPath_ = nullptr;
  const void* outputBytes_ = nullptr;
  CheckpointData queued_{};
  SaveRecord latest_{};
  SaveRecord writingRecord_{};
  alignas(64) SaveBytes encoded_{};
  // An extra byte distinguishes the expected length from an oversized file.
  alignas(64) std::array<std::uint8_t, kSaveRecordSize + 64> input_{};
};

} }
