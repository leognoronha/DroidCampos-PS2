"""Read-only OBJ/MTL/PNG budget and reference audit; Python standard library only."""
import json, struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'game/res/etip_inside'
parts=['corredor_1','corredor_2','corredor_3','sala_1','sala_2','sala_3','sala_4','sala_terminal','terminal','capsula','capsula_vidro','porta']
totals={}; textures=set(); occupant_triangles=0
for part in parts:
    lines=(root/(part+'.obj')).read_text().splitlines()
    mtl=(root/(part+'.mtl')).read_text().splitlines()
    materials={line.split(maxsplit=1)[1] for line in mtl if line.startswith('newmtl ')}
    positions=[];uvs=[];normals=[];faces=[]
    for line in lines:
        values=line.split()
        if not values: continue
        if values[0]=='v':positions.append(tuple(map(float,values[1:])))
        elif values[0]=='vt':uvs.append(tuple(map(float,values[1:])))
        elif values[0]=='vn':normals.append(tuple(map(float,values[1:])))
        elif values[0]=='usemtl':assert values[1] in materials,(part,values[1])
        elif values[0]=='mtllib':assert (root/values[1]).is_file()
        elif values[0]=='f':
            assert len(values)==4,(part,'Non-triangle')
            face=[tuple(map(int,v.split('/'))) for v in values[1:]];faces.append(face)
    for face in faces:
        for vi,ti,ni in face:
            assert 0<vi<=len(positions) and 0<ti<=len(uvs) and 0<ni<=len(normals)
            assert all(0<=c<=1 for c in uvs[ti-1]),(part,uvs[ti-1])
        a,b,c=[positions[v[0]-1] for v in face]
        ab=[b[i]-a[i] for i in range(3)];ac=[c[i]-a[i] for i in range(3)]
        cross=(ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])
        n=normals[face[0][2]-1]
        assert sum(cross[i]*n[i] for i in range(3))>0,(part,'Degenerate/reversed normal')
        # A tiled quad has at most a 2*sqrt(2) diagonal after triangulation.
        if not part.startswith('capsula'):
            for v1,v2 in ((a,b),(b,c),(c,a)):
                assert sum((v1[j]-v2[j])**2 for j in range(3))<=8.001,(part,'Oversized face')
    for line in mtl:
        if line.startswith('map_Kd '):textures.add(line.split()[1])
    totals[part]=len(faces)
    if part=='capsula':
        active=''
        for line in lines:
            if line.startswith('usemtl '):active=line.split()[1]
            elif line.startswith('f ') and active.startswith('PS2_Mat_Capsule_DroidCampos_'):occupant_triangles+=1
        assert {'PS2_Mat_Capsule_DroidCampos_body','PS2_Mat_Capsule_DroidCampos_head'}<=materials
        assert 2000<=occupant_triangles<=2500, ('Missing/oversized occupant',occupant_triangles)
    if part=='porta':
        assert max(abs(v[0]) for v in positions)<.081
        assert abs(max(v[1] for v in positions)-1.1)<.001
        assert abs(max(v[2] for v in positions)-.7)<.001
    if part=='terminal':
        assert all(-5.01<=v[0]<=-3.5 and -29.2<=v[2]<=-26.8 for v in positions),'Terminal not on left wall'
    if part.startswith('capsula'):
        assert all(2.18<=v[0]<=4.97 and 0.005<v[1]<5.68 and -30.55<=v[2]<=-27.45 for v in positions),(part,'Capsule out of reserved volume',[min(v[i] for v in positions) for i in range(3)],[max(v[i] for v in positions) for i in range(3)])
decoded_bytes=0
for texture in textures:
    data=(root/texture).read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n'
    w,h=struct.unpack('>II',data[16:24])
    expected={'ps2_tex_droidcampos_body.png':256,'ps2_tex_droidcampos_head.png':128}.get(texture,64)
    assert w==h==expected,(texture,w,h)
    decoded_bytes+=w*h*4
    assert data[24]==8 and data[25] in (2,6)
runtime_tris=sum(totals.values())+totals['porta']*3
assert runtime_tris<=7500
assert runtime_tris-occupant_triangles<=5000
assert decoded_bytes<=512*1024
audit=json.loads((root/'audit.json').read_text());assert audit['triangles']==runtime_tris
assert audit['capsule']['ceiling_clearance_m']>=.4 and audit['capsule']['wall_clearance_m']>=.05
assert audit['terminal_room']['height']==6.25
assert audit['capsule']['occupant']['triangles']==occupant_triangles
# Release-scene resources must partition the original occupied capsule exactly.
split_totals={}
for name in ('capsula_casca','capsula_ocupante','capsula_travas'):
    obj=(root/(name+'.obj')).read_text()
    split_totals[name]=sum(line.startswith('f ') for line in obj.splitlines())
    assert split_totals[name]>0
assert sum(split_totals.values())==totals['capsula']
assert split_totals['capsula_ocupante']==occupant_triangles
preview=root.parent/'droid_preview'
bank=(preview/'motion.bin').read_bytes()
magic,vertices,frames=struct.unpack('<8sII',bank[:16])
assert (magic,vertices,frames)==(b'DROIDAN1',6900,84)
assert len(bank)==16+vertices*frames*12
manifest=json.loads((preview/'assets.json').read_text())
assert manifest['passage_height']<2.2 and manifest['passage_width']<1.4
assert (preview/'button.obj').is_file() and (preview/'button.mtl').is_file()
print(json.dumps({'status':'PASS','triangles_with_4_doors':runtime_tris,'parts':totals,'unique_textures':len(textures),'decoded_RGBA_bytes':decoded_bytes,'occupant_triangles':occupant_triangles,'capsule_components':split_totals,'droid_frames':frames,'passage_height':manifest['passage_height'],'passage_width':manifest['passage_width']},indent=2))
