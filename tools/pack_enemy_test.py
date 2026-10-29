"""Package the approved Blender exports for the initial PS2 animation test."""
import argparse, hashlib, json, math, shutil, struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT.parent / 'DroidCampos-models/enemies/cerberus/animations-v3-fit'

def read_frame(path):
    positions, uvs, faces = [], [], []
    for line in path.read_text().splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == 'v':
            positions.append(tuple(map(float, fields[1:])))
        elif fields[0] == 'vt':
            uvs.append(tuple(map(float, fields[1:])))
        elif fields[0] == 'f':
            assert len(fields) == 4, path
            faces.extend(tuple(map(int, corner.split('/'))) for corner in fields[1:])
    assert positions and len(faces) % 3 == 0
    points = [positions[corner[0] - 1] for corner in faces]
    texcoords = [uvs[corner[1] - 1] for corner in faces]
    assert all(math.isfinite(c) and abs(c) < 3 for point in points for c in point)
    assert all(0 <= c <= 1 for uv in texcoords for c in uv)
    return points, texcoords

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, default=DEFAULT_SOURCE)
    parser.add_argument('--output', type=Path, default=ROOT / 'game/res/enemies')
    args = parser.parse_args()
    source = args.source.resolve()
    manifest = json.loads((source / 'manifest.json').read_text())
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    provenance = {'source_version': 'animations-v3-fit', 'variants': []}
    for variant in manifest['variants']:
        kind = variant['name'].split('-')[-1]
        dest = output / kind
        dest.mkdir(exist_ok=True)
        frames, reference_uvs = [], None
        for action, expected in [('attack_ready', 4), ('run', 8), ('shoot', 6)]:
            files = variant['actions'][action]['frame_files']
            assert len(files) == expected
            for filename in files:
                points, uv = read_frame(source / 'runtime' / variant['name'] / filename)
                if reference_uvs is None:
                    reference_uvs = uv
                assert uv == reference_uvs
                assert not frames or len(points) == len(frames[0])
                frames.append(points)
        count = len(frames[0])
        assert count == (1781 if kind == 'leader' else 1779) * 3
        lines = ['mtllib enemy.mtl', 'o ETIP_' + kind, 'usemtl PS2_ETIP_' + kind]
        lines += ['v %.7f %.7f %.7f' % p for p in frames[0]]
        lines += ['vt %.7f %.7f' % uv for uv in reference_uvs]
        lines += ['f ' + ' '.join('%d/%d' % (j, j) for j in range(i, i+3))
                  for i in range(1, count+1, 3)]
        (dest / 'enemy.obj').write_text('\n'.join(lines) + '\n')
        (dest / 'enemy.mtl').write_text('newmtl PS2_ETIP_' + kind + '\nKd 1 1 1\nmap_Kd atlas.png\n')
        shutil.copy2(source / (variant['name'] + '-atlas.png'), dest / 'atlas.png')
        bank = bytearray(struct.pack('<8sII', b'ETIPANM1', count, 18))
        for frame in frames:
            for point in frame:
                bank.extend(struct.pack('<fff', *point))
        (dest / 'motion.bin').write_bytes(bank)
        provenance['variants'].append({'name': kind, 'vertices': count, 'frames': 18,
            'position_bytes': count * 18 * 12, 'bank_sha256': hashlib.sha256(bank).hexdigest(),
            'source_blend_sha256': hashlib.sha256((source / 'etip-enemies-animations-v3-fit.blend').read_bytes()).hexdigest()})
    shutil.copy2(source / 'SOURCE-LICENSE.md', output / 'SOURCE-LICENSE.md')
    (output / 'assets.json').write_text(json.dumps(provenance, indent=2))
    print(json.dumps(provenance, indent=2))

if __name__ == '__main__':
    main()
