"""Regenerate weapon effects with FFmpeg and the official PS2SDK adpenc."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--ffmpeg', default='ffmpeg')
parser.add_argument('--adpenc', default='adpenc')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='droid-menu-audio-') as scratch:
    for source, name in [('laser-weapon-shot.wav', 'weapon_shot'), ('reload-weapon.wav', 'weapon_reload')]:
        pcm = Path(scratch) / (name + '.wav')
        encoded = Path(scratch) / (name + '.adpcm')
        subprocess.run([args.ffmpeg, '-hide_banner', '-loglevel', 'error', '-y',
                        '-i', str(root / 'game/assets/audio' / source),
                        '-map_metadata', '-1', '-map_chapters', '-1',
                        '-ac', '1', '-ar', '22050', '-c:a', 'pcm_s16le',
                        '-fflags', '+bitexact', '-flags:a', '+bitexact', str(pcm)], check=True)
        subprocess.run([args.adpenc, str(pcm), str(encoded)], check=True)
        data = encoded.read_bytes()
        assert 32 <= len(data) <= 128 * 1024 and len(data) % 16 == 0
        magic, version, channels, loop, _, pitch, samples = struct.unpack('<4sBBBBII', data[:16])
        assert (magic, version, channels, loop) == (b'APCM', 1, 1, 0)
        assert pitch == 22050 * 4096 // 48000 and samples > 0
        output = root / 'game/res/audio'
        output.mkdir(parents=True, exist_ok=True)
        (output / encoded.name).write_bytes(data)
        print(f'{encoded.name}: {len(data)} bytes, {samples / 22050:.3f}s, no loop')
