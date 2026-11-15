#include "core/memory_card.hpp"

#include <cerrno>
#include <cstring>
// libmc receives IOP flags, which differ from the newlib POSIX flags.
#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include <fileXio_rpc.h>
#include <libmc.h>

#include "core/memory_card_icon.hpp"

namespace DroidCampos { namespace Core {
namespace {

constexpr int kPort = 0;
constexpr int kSlot = 0;
constexpr char kDirectory[] = "/DROIDCAMPOS";
const char* const kCopies[] = {"mc0:/DROIDCAMPOS/save_a", "mc0:/DROIDCAMPOS/save_b"};
constexpr int kDevicePrefixLength = 4;
constexpr int kReadLimit = kSaveRecordSize + 1;

const mcIcon& browserMetadata() {
  alignas(64) static const mcIcon icon = [] {
    mcIcon data{};
    std::memcpy(data.head, "PS2D", 4);
    data.trans = 0x60;
    for (auto& corner : data.bgCol) {
      corner[0] = 9;
      corner[1] = 24;
      corner[2] = 36;
      corner[3] = 128;
    }
    data.lightDir[0][2] = 1.0F;
    for (int c = 0; c < 3; ++c) {
      data.lightCol[0][c] = 0.65F;
      data.lightAmbient[c] = 0.65F;
    }
    // Full-width Shift-JIS title, in the byte order expected by the browser.
    constexpr char title[] = "DroidCampos";
    for (unsigned i = 0; i < sizeof(title) - 1; ++i) {
      const auto c = title[i];
      const int trail = c >= 'a' ? 0x81 + c - 'a' : 0x60 + c - 'A';
      data.title[i] = static_cast<unsigned short>((trail << 8) | 0x82);
    }
    std::strcpy(reinterpret_cast<char*>(data.view), "droid.ico");
    std::strcpy(reinterpret_cast<char*>(data.copy), "droid.ico");
    std::strcpy(reinterpret_cast<char*>(data.del), "droid.ico");
    return data;
  }();
  return icon;
}

bool cardLost(int result) {
  return result == sceMcResChangedCard || result == sceMcResNoFormat || result <= -10;
}

bool newer(std::uint32_t candidate, std::uint32_t current) {
  const auto distance = candidate - current;
  return distance != 0 && distance < 0x80000000U;
}

}  // namespace

void MemoryCard::init(bool modulesLoaded) {
  // The PS2SDK used by the project selects the ROM protocol explicitly.
  available_ = modulesLoaded && mcInit(MC_TYPE_XMC) >= 0 && fileXioInitSkipOverride() >= 0;
  if (!available_) status_ = SaveStatus::Error;
}

bool MemoryCard::busy() const {
  return available_ && (step_ != Step::Idle || scanRequested_ ||
      queued_.stage != Checkpoint::None);
}

void MemoryCard::refresh() {
  if (!available_) return;
  scanRequested_ = true;
  status_ = SaveStatus::Checking;
}

void MemoryCard::bindSession(bool enabled) {
  bound_ = enabled && canEnableAutosave(status_);
  notice_ = SaveNotice::None;
}

void MemoryCard::queueSave(const CheckpointData& data) {
  if (!bound_) {
    notice_ = SaveNotice::Disabled;
    return;
  }
  queued_ = data;
  notice_ = SaveNotice::Writing;
}

bool MemoryCard::readingFile() const {
  return step_ == Step::OpenCopy || step_ == Step::ReadCopy || step_ == Step::CloseCopy ||
      step_ == Step::OpenVerification || step_ == Step::ReadVerification ||
      step_ == Step::CloseVerification;
}

void MemoryCard::update(bool inSaveMenu) {
  if (!available_) return;
  if (pendingRpc_) {
    int result = 0;
    if (readingFile()) {
      const int sync = fileXioWaitAsync(FXIO_NOWAIT, &result);
      if (sync == FXIO_INCOMPLETE) return;
      fileXioSetBlockMode(FXIO_WAIT);
      if (sync != FXIO_COMPLETE) result = sceMcResDeniedPermit;
    } else {
      const int sync = mcSync(MC_NOWAIT, nullptr, &result);
      if (sync == 0) return;
      if (sync < 0) result = sceMcResDeniedPermit;
    }
    pendingRpc_ = false;
    complete(result);
    return;
  }

  if (step_ == Step::Idle) {
    const bool needsWrite = queued_.stage != Checkpoint::None;
    if (!scanRequested_ && !needsWrite &&
        (!(inSaveMenu || bound_) || ++pollFrames_ < 45)) return;
    pollFrames_ = 0;
    step_ = Step::Info;
  }

  int accepted = -1;
  const bool reading = readingFile();
  // The ROM mcRead DMA callback is incompatible with the project's old SDK.
  // fileXio reads the same filesystem through iomanX without overriding libc I/O.
  if (reading) fileXioSetBlockMode(FXIO_NOWAIT);
  switch (step_) {
    case Step::Info:
    case Step::CheckScannedCard:
    case Step::CheckWrittenCard:
      // The ROM server reports card changes and format through the RPC result.
      // The auxiliary type/format DMA fields vary across SDK and BIOS versions.
      accepted = mcGetInfo(kPort, kSlot, nullptr, nullptr, nullptr);
      break;
    case Step::OpenCopy:
    case Step::OpenVerification:
      accepted = fileXioOpen(kCopies[copy_], FIO_O_RDONLY);
      break;
    case Step::ReadCopy:
    case Step::ReadVerification:
      accepted = fileXioRead(fd_, input_.data(), kReadLimit);
      break;
    case Step::MakeDirectory:
      accepted = mcMkDir(kPort, kSlot, kDirectory);
      break;
    case Step::OpenOutput:
      accepted = mcOpen(kPort, kSlot, outputPath_, FIO_O_WRONLY | FIO_O_CREAT | FIO_O_TRUNC);
      break;
    case Step::WriteOutput:
      accepted = mcWrite(fd_, outputBytes_, outputSize_);
      break;
    case Step::FlushOutput:
      accepted = mcFlush(fd_);
      break;
    case Step::CloseCopy:
    case Step::CloseVerification:
      accepted = fileXioClose(fd_);
      break;
    case Step::CloseOutput:
      accepted = mcClose(fd_);
      break;
    case Step::Idle:
      return;
  }

  pendingRpc_ = accepted >= 0;
  if (!pendingRpc_) {
    if (reading) fileXioSetBlockMode(FXIO_WAIT);
    complete(sceMcResDeniedPermit);
  }
}

void MemoryCard::complete(int result) {
  switch (step_) {
    case Step::Info:
    case Step::CheckScannedCard:
    case Step::CheckWrittenCard: {
      const bool changed = result == sceMcResChangedCard;
      if (changed) {
        ++mediaEpoch_;
        if (bound_) loseCard(SaveStatus::Checking);
      }
      if (result == sceMcResNoFormat || result == sceMcResDeniedPS1Permit) {
        loseCard(SaveStatus::InvalidCard);
      } else if (result <= -10) {
        loseCard(SaveStatus::NoCard);
      } else if (result < 0 && !changed) {
        loseCard(SaveStatus::Error);
      } else if (step_ == Step::CheckWrittenCard && !changed) {
        latest_ = writingRecord_;
        latestCopy_ = copy_;
        status_ = SaveStatus::Ready;
        writing_ = false;
        notice_ = queued_.stage == Checkpoint::None ? SaveNotice::Saved : SaveNotice::Writing;
        step_ = Step::Idle;
        return;
      } else if (step_ == Step::CheckScannedCard && !changed) {
        finishScan();
        return;
      } else if (scanRequested_ || changed || status_ == SaveStatus::NoCard ||
                 status_ == SaveStatus::InvalidCard || status_ == SaveStatus::Error) {
        status_ = SaveStatus::Checking;
        latest_ = {};
        latestCopy_ = -1;
        scanDamaged_ = scanIncompatible_ = scanError_ = false;
        copy_ = 0;
        step_ = Step::OpenCopy;
        return;
      } else if (queued_.stage != Checkpoint::None) {
        beginWrite();
        return;
      }
      scanRequested_ = false;
      step_ = Step::Idle;
      return;
    }
    case Step::OpenCopy:
      if (result < 0) {
        if (result != -ENOENT && result != sceMcResNoEntry) scanError_ = true;
        if (copy_++ == 1) step_ = Step::CheckScannedCard;
      } else {
        fd_ = result;
        step_ = Step::ReadCopy;
      }
      return;
    case Step::ReadCopy:
    case Step::ReadVerification:
      ioResult_ = result;
      step_ = step_ == Step::ReadCopy ? Step::CloseCopy : Step::CloseVerification;
      return;
    case Step::CloseCopy:
      if (result < 0 || ioResult_ < 0) {
        scanError_ = true;
        if ((cardLost(result) || cardLost(ioResult_)) && bound_) loseCard(SaveStatus::Checking);
      } else {
        SaveRecord record;
        const auto decoded = decodeSave(input_.data(), ioResult_, record);
        if (decoded == SaveStatus::Ready && (latestCopy_ < 0 || newer(record.sequence, latest_.sequence))) {
          latest_ = record;
          latestCopy_ = copy_;
        }
        scanDamaged_ |= decoded == SaveStatus::Corrupt;
        scanIncompatible_ |= decoded == SaveStatus::Incompatible;
      }
      step_ = ++copy_ < 2 ? Step::OpenCopy : Step::CheckScannedCard;
      return;
    case Step::MakeDirectory:
      // A prior interrupted save can leave the directory in place.
      if (result < 0 && result != sceMcResDeniedPermit) writeFailed(result);
      else selectOutput(0);
      return;
    case Step::OpenOutput:
    case Step::OpenVerification:
      if (result < 0) {
        writeFailed(result);
      } else {
        fd_ = result;
        step_ = step_ == Step::OpenOutput ? Step::WriteOutput : Step::ReadVerification;
      }
      return;
    case Step::WriteOutput:
      ioResult_ = result == outputSize_ ? 0 : result < 0 ? result : sceMcResFullDevice;
      step_ = ioResult_ == 0 ? Step::FlushOutput : Step::CloseOutput;
      return;
    case Step::FlushOutput:
      ioResult_ = result;
      step_ = Step::CloseOutput;
      return;
    case Step::CloseOutput:
      if (ioResult_ < 0 || result < 0) writeFailed(ioResult_ < 0 ? ioResult_ : result);
      else if (outputIndex_ < 2) selectOutput(outputIndex_ + 1);
      else step_ = Step::OpenVerification;
      return;
    case Step::CloseVerification:
      if (result < 0 || ioResult_ != kSaveRecordSize ||
          std::memcmp(input_.data(), encoded_.data(), encoded_.size()) != 0) {
        writeFailed(result < 0 ? result : ioResult_ < 0 ? ioResult_ : sceMcResDeniedPermit);
      } else {
        step_ = Step::CheckWrittenCard;
      }
      return;
    case Step::Idle:
      return;
  }
}

void MemoryCard::finishScan() {
  // Preserve unknown-version files. A reader for this version must not replace them.
  status_ = scanIncompatible_ ? SaveStatus::Incompatible :
      latestCopy_ >= 0 ? SaveStatus::Ready :
      scanError_ ? SaveStatus::Error :
      scanDamaged_ ? SaveStatus::Corrupt : SaveStatus::Empty;
  if (status_ == SaveStatus::Incompatible || status_ == SaveStatus::Error)
    loseCard(status_);
  scanRequested_ = false;
  step_ = Step::Idle;
}

void MemoryCard::loseCard(SaveStatus status) {
  if (bound_ || writing_) notice_ = SaveNotice::Disabled;
  bound_ = writing_ = false;
  queued_ = {};
  latest_ = {};
  latestCopy_ = -1;
  status_ = status;
}

void MemoryCard::writeFailed(int result) {
  if (cardLost(result)) {
    loseCard(SaveStatus::Checking);
    scanRequested_ = true;
  } else {
    notice_ = result == sceMcResFullDevice ? SaveNotice::Full : SaveNotice::Failed;
  }
  writing_ = false;
  step_ = Step::Idle;
}

void MemoryCard::beginWrite() {
  writing_ = true;
  notice_ = SaveNotice::Writing;
  auto sequence = latest_.sequence + 1;
  if (sequence == 0) sequence = 1;
  writingRecord_ = {queued_, sequence};
  queued_ = {};
  encoded_ = encodeSave(writingRecord_);
  copy_ = latestCopy_ == 0 ? 1 : 0;
  if (latestCopy_ < 0) step_ = Step::MakeDirectory;
  else selectOutput(2);
}

void MemoryCard::selectOutput(int index) {
  outputIndex_ = index;
  if (index == 0) {
    outputPath_ = "/DROIDCAMPOS/droid.ico";
    outputBytes_ = &kMemoryCardIcon;
    outputSize_ = sizeof(kMemoryCardIcon);
  } else if (index == 1) {
    outputPath_ = "/DROIDCAMPOS/icon.sys";
    outputBytes_ = &browserMetadata();
    outputSize_ = sizeof(mcIcon);
  } else {
    outputPath_ = kCopies[copy_] + kDevicePrefixLength;
    outputBytes_ = encoded_.data();
    outputSize_ = encoded_.size();
  }
  step_ = Step::OpenOutput;
}

} }
