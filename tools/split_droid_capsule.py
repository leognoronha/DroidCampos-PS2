"""Split the audited capsule OBJ into independent runtime components; source stays intact."""
from pathlib import Path
import json
root=Path(__file__).resolve().parents[1];folder=root/'game/res/etip_inside'
positions=[];uvs=[];normals=[];groups={'capsula_casca':[],'capsula_ocupante':[],'capsula_travas':[]};material=''
for line in (folder/'capsula.obj').read_text().splitlines():
 fields=line.split()
 if not fields:continue
 if fields[0]=='v':positions.append(tuple(map(float,fields[1:])))
 elif fields[0]=='vt':uvs.append(tuple(map(float,fields[1:])))
 elif fields[0]=='vn':normals.append(tuple(map(float,fields[1:])))
 elif fields[0]=='usemtl':material=fields[1]
 elif fields[0]=='f':
  corners=[tuple(int(v) if v else 0 for v in c.split('/')) for c in fields[1:]]
  points=[positions[c[0]-1] for c in corners]
  group='capsula_ocupante' if 'DroidCampos_' in material else 'capsula_travas' if all(2.65<p[1]<2.82 for p in points) else 'capsula_casca'
  groups[group].append((material,corners))
mtls={}
for block in (folder/'capsula.mtl').read_text().split('newmtl ')[1:]:mtls[block.splitlines()[0]]=block
for name,faces in groups.items():
 assert faces,name
 lines=['mtllib '+name+'.mtl'];used=[];index=1
 for mat,corners in faces:
  if mat not in used:used.append(mat)
  lines.append('usemtl '+mat)
  for corner in corners:
   p=positions[corner[0]-1];uv=uvs[corner[1]-1];n=normals[corner[2]-1]
   lines+=['v %s'%' '.join(map(str,p)),'vt %s'%' '.join(map(str,uv)),'vn %s'%' '.join(map(str,n))]
  lines.append('f '+' '.join(f'{i}/{i}/{i}' for i in range(index,index+len(corners))));index+=len(corners)
 (folder/(name+'.obj')).write_text('\n'.join(lines)+'\n')
 (folder/(name+'.mtl')).write_text(''.join('newmtl '+mtls[m] for m in used))
print('Capsule split:',{k:len(v) for k,v in groups.items()})
