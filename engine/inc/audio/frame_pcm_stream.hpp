#pragma once

#include <algorithm>
#include <cstdio>

namespace Tyra {

// Preserve PCM until the device has acknowledged the bytes, including short writes.
class FramePcmStream {
 public:
  static constexpr int kChunkSize = 2048;
  static constexpr int kSampleBytes = 4;

  void reset(long bytes = 0) {
    remaining_ = bytes;
    begin_ = end_ = stalled_ = 0;
  }

  bool submitted() const { return remaining_ == 0 && begin_ == end_; }

  template <class Send>
  bool pump(std::FILE* file, int available, Send send) {
    if (submitted()) return true;
    const int capacity = std::min(available, kChunkSize);
    if (capacity < kSampleBytes) return ++stalled_ < 180;
    if (begin_ == end_) {
      const int expected = static_cast<int>(std::min<long>(remaining_, kChunkSize));
      if (std::fread(data_, 1, expected, file) != static_cast<size_t>(expected)) return false;
      remaining_ -= expected;
      begin_ = 0;
      end_ = expected;
    }
    const int count = std::min(end_ - begin_, capacity - capacity % kSampleBytes);
    const int accepted = send(data_ + begin_, count);
    if (accepted <= 0) return ++stalled_ < 180;
    if (accepted > count || accepted % kSampleBytes != 0) return false;
    begin_ += accepted;
    stalled_ = 0;
    return true;
  }

 private:
  alignas(64) char data_[kChunkSize];
  long remaining_ = 0;
  int begin_ = 0, end_ = 0, stalled_ = 0;
};

}  // namespace Tyra
