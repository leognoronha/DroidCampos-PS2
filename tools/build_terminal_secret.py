"""Convert the ETIP secret video and confirmation to PS2 runtime assets."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import wave

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

    video_dir = root / 'game/res/video'
    video_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='terminal-video-', dir=video_dir) as scratch:
        scratch = Path(scratch)
        image = scratch / 'claudio.rgb565'
        audio = scratch / 'claudio.wav'
        pcm = scratch / 'claudio.pcm'
        convert(args.video, '-map', '0:v:0', '-vf', 'fps=12,scale=160:160:flags=lanczos',
                '-pix_fmt', 'rgb565le', '-f', 'rawvideo', image)
        convert(args.video, '-map', '0:a:0', '-ac', 2, '-ar', 22050, '-f', 's16le', pcm)
        frames, remainder = divmod(image.stat().st_size, 160 * 160 * 2)
        if not frames or remainder:
            raise ValueError('Incomplete RGB565 video')
        soundtrack = pcm.read_bytes()
        # Keep a silent stereo tail beyond the last picture to drain AUDSRV.
        picture_bytes = ((frames * 22050 + 11) // 12) * 4
        if not soundtrack or len(soundtrack) % 4 or len(soundtrack) > picture_bytes + 7350:
            raise ValueError('Invalid PCM or audio and video durations do not match')
        with wave.open(str(audio), 'wb') as output:
            output.setparams((2, 2, 22050, 0, 'NONE', 'not compressed'))
            output.writeframes(soundtrack.ljust(picture_bytes, b'\0') + bytes(8192))
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
        if args.docker:
            subprocess.run(['docker', 'run', '--rm', '--mount',
                            f'type=bind,source={scratch},target=/audio',
                            '--entrypoint', args.adpenc, 'h4570/tyra:latest',
                            '/audio/confirmation.wav', '/audio/terminal_secret.adpcm'],
                           check=True)
        else:
            subprocess.run([args.adpenc, str(pcm), str(encoded)], check=True)
        (root / 'game/res/audio/terminal_secret.adpcm').write_bytes(encoded.read_bytes())


if __name__ == '__main__':
    main()
