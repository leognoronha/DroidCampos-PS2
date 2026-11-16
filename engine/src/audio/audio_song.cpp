/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2022, tyra - https://github.com/h4570/tyra
# Licensed under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#include "audio/audio_song.hpp"
#include "debug/debug.hpp"
#include <tamtypes.h>
#include <malloc.h>
#include <cstdlib>
#include <cstring>
#include <audsrv.h>

namespace Tyra {

const u16 AudioSong::chunkSize = 4 * 1024;

AudioSong::AudioSong() {
  chunkReadStatus = 0;
  tyraVolume = 0;
  audsrvVolume = 0;

  songLoaded = false;
  songPlaying = false;
  inLoop = false;
  songFinished = false;

  chunk = nullptr;
}

AudioSong::~AudioSong() {
  std::free(chunk);
}

void AudioSong::init() {
  chunk = static_cast<char*>(memalign(sizeof(char), chunkSize));

  initSema();
  setSongFormat();
}

void AudioSong::load(const char* t_path) {
  const bool loaded = tryLoad(t_path, 0x30, false);
  TYRA_ASSERT(loaded, "Failed to open wav file!");
  (void)loaded;
}

void AudioSong::load(const std::string& t_path) { load(t_path.c_str()); }

bool AudioSong::tryLoadFrameStream(const std::string& path, long offset) {
  return tryLoad(path.c_str(), offset, true);
}

bool AudioSong::tryLoad(const char* path, long offset, bool frameDriven) {
  FILE* file = fopen(path, "rb");
  if (!file) return false;
  if (fseek(file, offset, SEEK_SET) != 0 || fgetc(file) == EOF) {
    fclose(file);
    return false;
  }
  long pcmBytes = 0;
  if (frameDriven) {
    // The converter writes a canonical PCM header and whole stereo samples.
    unsigned char header[44];
    auto read32 = [&](int offset) {
      return u32(header[offset]) | (u32(header[offset + 1]) << 8) |
             (u32(header[offset + 2]) << 16) | (u32(header[offset + 3]) << 24);
    };
    if (offset != 44 || std::fseek(file, 0, SEEK_SET) != 0 ||
        std::fread(header, 1, sizeof(header), file) != sizeof(header) ||
        std::memcmp(header, "RIFF", 4) || std::memcmp(header + 8, "WAVEfmt ", 8) ||
        read32(16) != 16 || read32(20) != 0x00020001 || read32(24) != 22050 ||
        read32(28) != 88200 || read32(32) != 0x00100004 ||
        std::memcmp(header + 36, "data", 4) || read32(40) == 0 ||
        read32(40) % FramePcmStream::kSampleBytes != 0 ||
        std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) < 44 ||
        static_cast<unsigned long>(std::ftell(file) - 44) != read32(40)) {
      std::fclose(file);
      return false;
    }
    pcmBytes = read32(40);
  }
  if (songLoaded) stopAndUnload();
  WaitSema(streamSema);
  wav = file;
  pcmOffset = offset;
  frameStream = frameDriven;
  framePcm_.reset(pcmBytes);
  if (frameStream) {
    setSongFormat();
  } else {
    initAUDSRV();
  }
  rewindSongToStart();
  songLoaded = true;
  SignalSema(streamSema);
  return true;
}

void AudioSong::stopAndUnload() {
  WaitSema(streamSema);
  if (songLoaded) {
    stop();
    if (frameStream) audsrv_stop_audio();
    unloadSong();
  }
  SignalSema(streamSema);
}

void AudioSong::play() {
  TYRA_ASSERT(songLoaded, "Cant play song because was not loaded!");
  if (songFinished) rewindSongToStart();
  tyraVolume = audsrvVolume;
  audsrv_set_volume(tyraVolume);
  songPlaying = true;
}

void AudioSong::stop() {
  tyraVolume = 0;
  audsrv_set_volume(tyraVolume);
  songPlaying = false;
}

const bool& AudioSong::isPlaying() const { return songPlaying; }

const bool& AudioSong::isLoaded() const { return songLoaded; }

const u8& AudioSong::getVolume() const { return tyraVolume; }

std::size_t AudioSong::getListenersCount() const {
  return songListeners.size();
}

void AudioSong::initAUDSRV() {
  int ret = audsrv_on_fillbuf(chunkSize, (audsrv_callback_t)iSignalSema,
                              (void*)fillbufferSema);

  TYRA_ASSERT(ret >= 0,
              "AUDSRV returned error string:", audsrv_get_error_string());
}

/** Initialize semaphore which will wait until chunk of the song is not
 * finished. */
void AudioSong::initSema() {
  TYRA_LOG("Creating audio semaphore");
  sema.init_count = 0;
  sema.max_count = 1;
  sema.option = 0;
  fillbufferSema = CreateSema(&sema);
  sema.init_count = 1;
  streamSema = CreateSema(&sema);
  TYRA_ASSERT(streamSema >= 0, "Could not create audio stream semaphore");
  TYRA_LOG("AudioSong semaphore created");
}

/**
 * Close file.
 * Delete song path from memory.
 */
void AudioSong::unloadSong() {
  songLoaded = false;
  fclose(wav);
  wav = nullptr;
}

/** Fseek on wav. */
void AudioSong::rewindSongToStart() {
  if (wav != nullptr) fseek(wav, pcmOffset, SEEK_SET);
  chunkReadStatus = 0;
  songFinished = false;
}

/** Set WAV format to 16bit, 22050Hz, stereo. */
void AudioSong::setSongFormat() {
  format.bits = 16;
  format.freq = 22050;
  format.channels = 2;

  if (audsrv_set_format(&format)) {
    TYRA_ERROR("AUDSRV returned error string: ", audsrv_get_error_string());
  }
}

void AudioSong::setVolume(const u8& t_vol) {
  audsrvVolume = t_vol;
  if (songPlaying) tyraVolume = t_vol;

  if (audsrv_set_volume(tyraVolume)) {
    TYRA_ERROR("AUDSRV returned error string: ", audsrv_get_error_string());
  }
}

void AudioSong::work() {
  if (PollSema(streamSema) < 0) return;
  if (!frameStream) pumpStream();
  SignalSema(streamSema);
}

bool AudioSong::pumpFrameStream() {
  if (!frameStream || !songPlaying || songFinished) return true;
  return framePcm_.pump(wav, audsrv_available(), audsrv_play_audio);
}

bool AudioSong::frameStreamDrained() const {
  // The file ends with a short silence guard for the DMA tail. The remaining
  // queue may contain a partial hardware block that will never play.
  constexpr int silenceGuardBytes = 8192;
  if (!frameStream || !framePcm_.submitted()) return false;
  const int queued = audsrv_queued();
  return queued >= 0 && queued <= silenceGuardBytes / 2;
}

void AudioSong::pumpStream() {
  if (!songPlaying || !songLoaded) return;
  if (songFinished) {
    if (inLoop) {
      for (u32 i = 0; i < getListenersCount(); i++)
        songListeners[i]->listener->onAudioFinish();
      rewindSongToStart();
    } else {
      // Let the last samples already queued in AUDSRV finish playing.
      songPlaying = false;
      return;
    }
  }

  if (chunkReadStatus > 0) {
    if (PollSema(fillbufferSema) < 0) return;
    audsrv_wait_audio(chunkReadStatus);
    audsrv_play_audio(chunk, chunkReadStatus);
    for (u32 i = 0; i < getListenersCount(); i++)
      songListeners[i]->listener->onAudioTick();
  }

  chunkReadStatus = fread(chunk, 1, chunkSize, wav);

  if (chunkReadStatus <= 0) songFinished = true;
}

u32 AudioSong::addListener(AudioListener* t_listener) {
  AudioListenerRef* ref = new AudioListenerRef;
  ref->id = rand() % 1000000;
  ref->listener = t_listener;
  songListeners.push_back(ref);
  return ref->id;
}

void AudioSong::removeListener(const u32& t_id) {
  s32 index = -1;
  for (u32 i = 0; i < songListeners.size(); i++)
    if (songListeners[i]->id == t_id) {
      index = i;
      break;
    }

  TYRA_ASSERT(index != -1,
              "Cant remove listener because given id was not found!");

  delete songListeners[index];
  songListeners.erase(songListeners.begin() + index);
}

}  // namespace Tyra
