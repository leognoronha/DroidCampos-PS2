#include <cassert>
#include <cstdio>
#include <string>

#include "audio/frame_pcm_stream.hpp"

int main() {
  Tyra::FramePcmStream stream;
  auto* input = std::tmpfile();
  assert(input);
  std::string pcm(4096, '\0');
  for (size_t i = 0; i < pcm.size(); ++i) pcm[i] = static_cast<char>(i % 251);
  assert(std::fwrite(pcm.data(), 1, pcm.size(), input) == pcm.size());

  for (int pass = 0; pass < 2; ++pass) {
    std::rewind(input);
    stream.reset(pcm.size());
    std::string accepted;
    auto shortWrite = [&](const char* bytes, int size) {
      const int count = std::min(size, 256);
      accepted.append(bytes, count);
      return count;
    };
    assert(stream.pump(input, 0, shortWrite));
    assert(stream.pump(input, 2048, [](const char*, int) { return 0; }));
    assert(stream.pump(input, 2048, [](const char*, int) { return -1; }));
    for (int call = 0; call < 16; ++call) assert(stream.pump(input, 2048, shortWrite));
    assert(stream.submitted() && accepted == pcm);
  }

  std::rewind(input);
  stream.reset(pcm.size() + 4);
  auto writeAll = [](const char*, int size) { return size; };
  assert(stream.pump(input, 2048, writeAll));
  assert(stream.pump(input, 2048, writeAll));
  assert(!stream.pump(input, 2048, writeAll));

  stream.reset(pcm.size());
  for (int call = 0; call < 179; ++call) assert(stream.pump(input, 0, writeAll));
  assert(!stream.pump(input, 0, writeAll));
  std::fclose(input);
}
