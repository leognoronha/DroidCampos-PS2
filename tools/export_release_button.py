import bpy
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(root.parent/'DroidCampos-models/authoring/mastro-botao/mastro-botao.blend'))
objects=[o for o in bpy.data.objects if o.type=='MESH' and o.parent and o.parent.name.startswith('Mastro com botao')]
assert len(objects)==6
out=root/'game/res/droid_preview';lines=['mtllib button.mtl'];mats={};idx=1
for o in objects:
 mesh=o.data;mesh.calc_loop_triangles()
 for tri in mesh.loop_triangles:
  m=mesh.materials[tri.material_index];name='red' if 'Vermelho' in m.name else 'black';mats[name]=m;lines.append('usemtl '+name)
  for vi in tri.vertices:
   p=o.matrix_world@mesh.vertices[vi].co;lines.append('v %.7f %.7f %.7f'%(p.x,p.z,-p.y))
  lines.append('f '+' '.join(str(i) for i in range(idx,idx+3)));idx+=3
(out/'button.obj').write_text('\n'.join(lines)+'\n')
(out/'button.mtl').write_text('newmtl black\nKd 0.035 0.045 0.055\nnewmtl red\nKd 0.85 0.01 0.015\n')
print('BUTTON',len(objects),'parts', (idx-1)//3,'triangles')
