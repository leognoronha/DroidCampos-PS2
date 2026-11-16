"""Convert the ETIP secret video and confirmation to PS2 runtime assets."""
import argparse
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('video', type=Path)
    parser.add_argument('confirmation', nargs='?', type=Path)
    parser.add_argument('--ffmpeg', default='ffmpeg')
    parser.add_argument('--adpenc', default='adpenc')
    parser.add_argument('--docker', action='store_true')
    args = parser.parse_args()

    def convert(source, *options):
        subprocess.run([args.ffmpeg, '-loglevel', 'error', '-xerror', '-y', '-i',
                        str(source.resolve()), '-map_metadata', '-1',
                        *map(str, options)], check=True)

    def encode_adpcm(wav, encoded):
        if args.docker:
            subprocess.run(['docker', 'run', '--rm', '--network', 'none',
                            '--mount', f'type=bind,source={wav.parent},target=/audio',
                            '--entrypoint', args.adpenc, 'h4570/tyra:latest',
                            f'/audio/{wav.name}', f'/audio/{encoded.name}'], check=True)
        else:
            subprocess.run([args.adpenc, str(wav), str(encoded)], check=True)

    video_dir = root / 'game/res/video'
    video_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='terminal-video-', dir=video_dir) as scratch:
        scratch = Path(scratch)
        image = scratch / 'claudio.rgb565'
        audio = scratch / 'claudio.adpcm'
        pcm = scratch / 'claudio.wav'
        convert(args.video, '-map', '0:v:0', '-vf', 'fps=12,scale=160:160:flags=lanczos',
                '-pix_fmt', 'rgb565le', '-f', 'rawvideo', image)
        frames, remainder = divmod(image.stat().st_size, 160 * 160 * 2)
        if not frames or remainder:
            raise ValueError('Incomplete RGB565 video')
        samples = (frames * 11025 + 11) // 12
        convert(args.video, '-map', '0:a:0', '-af',
                f'aresample=11025,apad,atrim=end_sample={samples}',
                '-ac', 1, '-ar', 11025, '-c:a', 'pcm_s16le', pcm)
        encode_adpcm(pcm, audio)
        data = audio.read_bytes()
        if data[:7] != b'APCM\x01\x01\x00' or not 32 <= len(data) <= 320 * 1024:
            raise ValueError('Invalid terminal ADPCM')
        for asset in (image, audio):
            asset.replace(video_dir / asset.name)
        print(f'{frames} frames, {frames / 12:.2f} seconds')

    if args.confirmation is None:
        return

    with tempfile.TemporaryDirectory(prefix='terminal-secret-') as scratch:
        pcm = Path(scratch) / 'confirmation.wav'
        encoded = Path(scratch) / 'terminal_secret.adpcm'
        convert(args.confirmation, '-ac', 1, '-ar', 22050, '-c:a', 'pcm_s16le',
                '-fflags', '+bitexact', '-flags:a', '+bitexact', pcm)
        encode_adpcm(pcm, encoded)
        (root / 'game/res/audio/terminal_secret.adpcm').write_bytes(encoded.read_bytes())


if __name__ == '__main__':
    main()
