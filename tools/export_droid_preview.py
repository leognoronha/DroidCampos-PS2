import bpy,struct,json,hashlib,math
from mathutils import Quaternion
from pathlib import Path
root=Path(r'C:\Users\leogn\OneDrive\Documents\DroidCampos-PS2')
source=root.parent/'DroidCampos-models/DroidCampos/animacoes/DroidCampos-animacoes-v07.blend'
bpy.ops.wm.open_mainfile(filepath=str(source))
rig=bpy.data.objects['DroidCampos_Rig']; scene=bpy.context.scene
objects=sorted([o for o in scene.objects if o.type=='MESH' and not o.name.startswith('PREVIA')],key=lambda o:o.name)
out=root/'game/res/droid_preview';out.mkdir(exist_ok=True)
frames=[];uvs=[];counts=[]
for action,count,step,crouch in [('DC_Caminhada_Pesada',12,4,False),('DC_Direto_Carregado_Horizontal',24,3,False),('DC_Caminhada_Pesada',12,4,True),('DC_Direto_Carregado_Horizontal',24,3,True),('DC_Caminhada_Pesada',12,4,'passage')]:
 rig.animation_data.action=bpy.data.actions[action]
 if hasattr(rig.animation_data,'action_slot'): rig.animation_data.action_slot=bpy.data.actions[action].slots[0]
 for track in rig.animation_data.nla_tracks: track.mute=True
 for f in range(count):
  scene.frame_set(1+f*step)
  if crouch:
   # Add joint rotations to the authored actions; bone lengths and mesh scale stay intact.
   for name,angle in ([('thigh.L',-110),('thigh.R',-110),('shin.L',180),('shin.R',180),('spine',0),('head',65)] if crouch=='passage' else [('thigh.L',-75),('thigh.R',-75),('shin.L',125),('shin.R',125),('spine',65),('head',-15)]):
    bone=rig.pose.bones[name];bone.rotation_quaternion=bone.rotation_quaternion@Quaternion((1,0,0),math.radians(angle))
   rig.pose.bones['pelvis'].location.y-=.62
   if crouch=='passage':
    rig.pose.bones['pelvis'].rotation_quaternion=rig.pose.bones['pelvis'].rotation_quaternion@Quaternion((0,0,1),math.radians(50))
   bpy.context.view_layer.update()
  assert all(abs(v-1)<.001 for bone in rig.pose.bones for v in bone.scale), "Scaled bone in exported pose"
  deps=bpy.context.evaluated_depsgraph_get();points=[];frame_uv=[];part_counts=[]
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
  if crouch:
   floor=min(p[1] for p in points)
   points=[(x,y-floor,z) for x,y,z in points]
  if not frames: uvs=frame_uv;counts=part_counts
  assert counts==part_counts and uvs==frame_uv
  frames.append(points)
# Sideways doorway shuffle: center its transverse envelope on the collision capsule.
passage_min=min(p[2] for frame in frames[72:] for p in frame)
passage_max=max(p[2] for frame in frames[72:] for p in frame)
center=(passage_min+passage_max)/2
frames[72:]=[[(x,y,z-center) for x,y,z in frame] for frame in frames[72:]]
assert max(p[1] for frame in frames[36:72] for p in frame)<2.2
assert max(p[1] for frame in frames[72:] for p in frame)<2.2
assert passage_max-passage_min<1.4
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
meta={'source':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'vertices':n,'material_counts':counts,'frames':len(frames),'walk_frames':12,'punch_frames':24,'walk_seconds':2,'punch_seconds':3,'crouch_walk_start':36,'crouch_punch_start':48,'passage_start':72,'passage_width':passage_max-passage_min,'passage_height':max(p[1] for frame in frames[72:] for p in frame),'min':[min(p[i] for f in frames for p in f) for i in range(3)],'max':[max(p[i] for f in frames for p in f) for i in range(3)]}
(out/'assets.json').write_text(json.dumps(meta,indent=2));print(meta)
