"""Convert the ETIP secret video and confirmation to PS2 runtime assets."""
import argparse
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('video', type=Path)
    parser.add_argument('confirmation', type=Path)
    parser.add_argument('--ffmpeg', default='ffmpeg')
    parser.add_argument('--adpenc', default='adpenc')
    parser.add_argument('--docker', action='store_true')
    args = parser.parse_args()

    def convert(source, *options):
        subprocess.run([args.ffmpeg, '-loglevel', 'error', '-y', '-i',
                        str(source.resolve()), '-map_metadata', '-1',
                        *map(str, options)], check=True)

    video_dir = root / 'game/res/video'
    video_dir.mkdir(parents=True, exist_ok=True)
    convert(args.video, '-map', '0:v:0', '-vf', 'fps=15,scale=240:240:flags=lanczos',
            '-pix_fmt', 'rgb565le', '-f', 'rawvideo', video_dir / 'claudio.rgb565')
    audio = video_dir / 'claudio.wav'
    convert(args.video, '-map', '0:a:0', '-ac', 2, '-ar', 22050, '-c:a', 'pcm_s16le',
            '-fflags', '+bitexact', '-flags:a', '+bitexact', audio)
    with audio.open('rb') as stream:
        header = stream.read(44)
    if header[36:40] != b'data':
        raise ValueError('Expected PCM data at WAV offset 44')

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
