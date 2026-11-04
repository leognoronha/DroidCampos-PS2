import bpy,struct,json,hashlib
from pathlib import Path
root=Path(r'C:\Users\leogn\OneDrive\Documents\DroidCampos-PS2')
source=root.parent/'DroidCampos-models/DroidCampos/animacoes/DroidCampos-animacoes-v07.blend'
bpy.ops.wm.open_mainfile(filepath=str(source))
rig=bpy.data.objects['DroidCampos_Rig']; scene=bpy.context.scene
objects=sorted([o for o in scene.objects if o.type=='MESH' and not o.name.startswith('PREVIA')],key=lambda o:o.name)
out=root/'game/res/droid_preview';out.mkdir(exist_ok=True)
frames=[];uvs=[];counts=[]
for action,count,step in [('DC_Caminhada_Pesada',12,4),('DC_Direto_Carregado_Horizontal',24,3)]:
 rig.animation_data.action=bpy.data.actions[action]
 if hasattr(rig.animation_data,'action_slot'): rig.animation_data.action_slot=bpy.data.actions[action].slots[0]
 for track in rig.animation_data.nla_tracks: track.mute=True
 for f in range(count):
  scene.frame_set(1+f*step);deps=bpy.context.evaluated_depsgraph_get();points=[];frame_uv=[];part_counts=[]
  for matname in ['DC_body','DC_head']:
   start=len(points)
   for obj in objects:
    ev=obj.evaluated_get(deps);mesh=ev.to_mesh();mesh.calc_loop_triangles()
    for tri in mesh.loop_triangles:
     if mesh.materials[tri.material_index].name!=matname:continue
     # Blender Z-up to Tyra Y-up (rotation preserves winding).
     for li in tri.loops:
      p=ev.matrix_world@mesh.vertices[mesh.loops[li].vertex_index].co
      points.append((p.x,p.z,-p.y));frame_uv.append(tuple(mesh.uv_layers.active.data[li].uv))
    ev.to_mesh_clear()
   part_counts.append(len(points)-start)
  if not frames: uvs=frame_uv;counts=part_counts
  assert counts==part_counts and uvs==frame_uv
  frames.append(points)
n=len(frames[0]);lines=['mtllib actor.mtl']
lines+=['v %.7f %.7f %.7f'%p for p in frames[0]]
lines+=['vt %.7f %.7f'%uv for uv in uvs]
start=0
for name,count in zip(['body','head'],counts):
 lines+=['usemtl '+name]
 lines+=['f '+' '.join(f'{j}/{j}' for j in range(i,i+3)) for i in range(start+1,start+count+1,3)]
 start+=count
(out/'actor.obj').write_text('\n'.join(lines)+'\n')
(out/'actor.mtl').write_text('newmtl body\nKd 1 1 1\nmap_Kd body.png\nnewmtl head\nKd 1 1 1\nmap_Kd head.png\n')
for name in ['body','head']:
 im=next(i for i in bpy.data.images if 'modelo-'+name in i.name);im.scale(256 if name=='body' else 128,256 if name=='body' else 128);im.filepath_raw=str(out/(name+'.png'));im.save()
with (out/'motion.bin').open('wb') as file:
 file.write(struct.pack('<8sII',b'DROIDAN1',n,len(frames)))
 for frame in frames:
  for p in frame:file.write(struct.pack('<fff',*p))
meta={'source':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'vertices':n,'material_counts':counts,'frames':len(frames),'walk_frames':12,'punch_frames':24,'walk_seconds':2,'punch_seconds':3,'min':[min(p[i] for f in frames for p in f) for i in range(3)],'max':[max(p[i] for f in frames for p in f) for i in range(3)]}
(out/'assets.json').write_text(json.dumps(meta,indent=2));print(meta)
