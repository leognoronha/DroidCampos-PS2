#pragma once
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
struct audsrv_adpcm_t {int pitch=0,loop=0,channels=0;void* buffer=nullptr;int size=0;};
namespace HostAudio {
inline std::map<audsrv_adpcm_t*,int> loaded;
inline std::vector<audsrv_adpcm_t*> played;
inline std::vector<int> channels;
inline int volumes[24] = {};
inline int playResult=0;
inline bool failLoad=false;
inline int resets=0;
}
inline int audsrv_load_adpcm(audsrv_adpcm_t* sample,void* data,int size) {
  assert(reinterpret_cast<uintptr_t>(data)%64==0);
  if(HostAudio::failLoad)return -1;
  sample->buffer=data;sample->size=size;sample->channels=1;
  HostAudio::loaded[sample]=size;return 0;
}
// Deliberately do not expose audsrv_free_adpcm: the project's PS2SDK lacks it.
