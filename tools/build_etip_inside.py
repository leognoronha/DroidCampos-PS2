"""Blender generator. Only writes ETIP-inside* and res/etip_inside outputs."""
import bpy, json, random, argparse, sys
from pathlib import Path
from mathutils import Vector
parser=argparse.ArgumentParser()
repo=Path(__file__).resolve().parents[1]
parser.add_argument('--models',type=Path,default=repo.parent/'DroidCampos-models')
parser.add_argument('--resources',type=Path,default=repo/'game/res/etip_inside')
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
MODELS=args.models; RES=args.resources
RES.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(MODELS/'ETIP-matriz.blend'))
bpy.context.preferences.filepaths.file_preview_type='NONE'
rust=bpy.data.materials['PS2_Mat_Metal_Ferrugem']; hazard=bpy.data.materials['PS2_Mat_Advertencia']
for o in list(bpy.data.objects): bpy.data.objects.remove(o,do_unlink=True)
for c in list(bpy.data.collections): bpy.data.collections.remove(c)
scene=bpy.context.scene; scene.unit_settings.system='METRIC'
groups={}; parts={}
for name in ('estrutura','portas','salas','terminal'):
    c=bpy.data.collections.new('Inside_'+name); scene.collection.children.link(c); groups[name]=c
def material(name,base,glow=False,screen=False):
    random.seed(name); img=bpy.data.images.new('PS2_Tex_Inside_'+name,width=64,height=64); pixels=[]
    for y in range(64):
        for x in range(64):
            shade=(.50+.50*min(min(x,63-x,y,63-y)/9,1))*(.9+random.random()*.2)
            if y%16==0: shade*=.65
            if screen:
                shade=.25 if y%4==0 else .55
                if (8<y<13 and 7<x<55) or (20<y<51 and x%13<9 and y%7<2): shade=1.2
            pixels.extend([min(1,c*shade) for c in base]+[1])
    img.pixels=pixels; img.filepath_raw=str(RES/(img.name.lower()+'.png')); img.file_format='PNG'; img.save(); img.pack()
    m=bpy.data.materials.new('PS2_Mat_Inside_'+name); m.use_nodes=True; n=m.node_tree.nodes; n.clear()
    out=n.new('ShaderNodeOutputMaterial'); tex=n.new('ShaderNodeTexImage'); tex.image=img; tex.interpolation='Closest'
    s=n.new('ShaderNodeEmission') if glow else n.new('ShaderNodeBsdfPrincipled')
    m.node_tree.links.new(tex.outputs['Color'],s.inputs['Color' if glow else 'Base Color'])
    if not glow: s.inputs['Roughness'].default_value=.85
    m.node_tree.links.new(s.outputs[0],out.inputs['Surface']); return m
metal=material('Metal',(.19,.23,.27)); floor=material('Piso',(.24,.26,.27)); wall=material('Parede',(.27,.30,.31))
cyan=material('Luz',(.26,.78,.86),True); red=material('Emergencia',(.8,.12,.035),True); monitor=material('Terminal',(.10,.85,.64),True,True)
def box(name,loc,size,mat,group,part):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc); o=bpy.context.object; o.name='PS2_'+name; o.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    for c in list(o.users_collection): c.objects.unlink(o)
    groups[group].objects.link(o); o.data.materials.append(mat)
    uv=o.data.uv_layers.active or o.data.uv_layers.new()
    for p in o.data.polygons:
        for li,co in zip(p.loop_indices,[(0,0),(1,0),(1,1),(0,1)]): uv.data[li].uv=co
    parts.setdefault(part,[]).append(o); return o
def b(name,x,y,z,sx,sy,sz,mat,group,part): return box(name,(x,y,z),(sx,sy,sz),mat,group,part)
for s in range(3):
    part='corredor_'+str(s+1); lo=s*8; hi=lo+8; y=lo+4
    b('Piso_'+str(s),0,y,-.1,3,8,.2,floor,'estrutura',part)
    b('Teto_'+str(s),0,y,2.9,3.4,8,.2,metal,'estrutura',part)
    for side in (-1,1):
        # Clip the wall intervals against openings even at segment boundaries.
        intervals=[(lo,hi)]
        for d in (8,16):
            new=[]
            for a,z in intervals:
                if z<=d-.7 or a>=d+.7: new.append((a,z))
                else:
                    if a<d-.7: new.append((a,d-.7))
                    if z>d+.7: new.append((d+.7,z))
            intervals=new
        for a,z in intervals: b('Parede_%s_%s_%s'%(s,side,a),side*1.6,(a+z)/2,1.4,.2,z-a,2.8,wall,'estrutura',part)
        for d in (8,16):
            if lo<=d<hi: b('Verga_%s_%s'%(side,d),side*1.6,d,2.5,.2,1.4,.6,metal,'estrutura',part)
        for j in range(4): b('Nervura_%s_%s_%s'%(s,side,j),side*1.46,lo+j*2+.8,1.4,.08,.12,2.8,metal,'estrutura',part)
        b('Duto_%s_%s'%(s,side),side*1.19,y,2.58,.22,7.95,.24,rust,'estrutura',part)
        b('Baliza_%s_%s'%(s,side),side*1.475,y,.25,.035,.26,.12,red,'estrutura',part)
    b('Luminaria_'+str(s),0,y,2.77,.65,.45,.045,cyan,'estrutura',part)
b('Entrada_Selada',0,0,1.4,3.4,.24,2.8,metal,'estrutura','corredor_1')
b('Fundo',0,24,1.4,3.4,.24,2.8,wall,'estrutura','corredor_3')
door_objects=[]
for i,(side,y) in enumerate([(-1,8),(1,8),(-1,16),(1,16)]):
    part='sala_'+str(i+1); x=side*3.65
    b('Sala_Piso_'+str(i),x,y,-.1,4.1,4,.2,floor,'salas',part)
    b('Soleira_'+str(i),side*1.6,y,-.025,.4,1.4,.05,hazard,'portas',part)
    b('Sala_Teto_'+str(i),x,y,2.9,4.1,4,.2,metal,'salas',part)
    b('Sala_Fundo_'+str(i),side*5.7,y,1.4,.2,4,2.8,wall,'salas',part)
    for off in (-2,2): b('Sala_Lateral_%s_%s'%(i,off),x,y+off,1.4,4.1,.2,2.8,wall,'salas',part)
    for off in (-1.35,1.35): b('Sala_Frente_%s_%s'%(i,off),side*1.6,y+off,1.4,.2,1.3,2.8,wall,'salas',part)
    b('Sala_Luz_'+str(i),side*4.8,y,2.75,.65,.35,.04,cyan,'salas',part)
    for j in range(3):
        b('Armario_%s_%s'%(i,j),side*5.25,y-1.25+j*.8,.9,.5,.64,1.8,metal,'salas',part)
        b('Puxador_%s_%s'%(i,j),side*4.98,y-1.25+j*.8,1,.025,.05,.3,hazard,'salas',part)
    b('Bancada_'+str(i),side*3.7,y+1.55,.8,1.8,.7,.12,rust,'salas',part)
    for off in (-.78,.78): b('Batente_%s_%s'%(i,off),side*1.49,y+off,1.1,.26,.12,2.2,hazard,'portas',part)
    b('Batente_Topo_'+str(i),side*1.49,y,2.25,.26,1.68,.1,hazard,'portas',part)
    door_objects.append(b('Porta_Deslizante_'+str(i+1),side*1.6,y,1.1,.16,1.4,2.2,metal,'portas','doors'))
    b('Porta_Indicador_'+str(i),side*1.445,y+.9,1.3,.035,.1,.16,cyan,'portas',part)
b('Terminal_Base',0,22.85,.5,2.1,1.1,1,metal,'terminal','terminal')
b('Terminal_Monitor',0,23.1,1.55,2.35,.4,1.05,metal,'terminal','terminal')
b('Terminal_Tela',0,22.888,1.58,2.08,.02,.78,monitor,'terminal','terminal')
b('Terminal_Teclado',0,22.23,1.01,1.75,.42,.08,metal,'terminal','terminal')
for i in range(5): b('Terminal_LED_'+str(i),-.8+i*.15,22.21,1.06,.07,.16,.015,cyan,'terminal','terminal')
texpaths={}
for objs in parts.values():
    for o in objs:
        for m in o.data.materials:
            if m.name in texpaths: continue
            img=next(n.image for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image)
            path=RES/(img.name.lower().removesuffix('.png')+'.png'); img.save_render(str(path)); texpaths[m.name]=path.stem
def export(name,objs,local=False):
    lines=['mtllib '+name+'.mtl']; mats={}; idx=1
    for o in objs:
        mesh=o.data; mesh.calc_loop_triangles(); uv=mesh.uv_layers.active.data
        for tri in mesh.loop_triangles:
            m=mesh.materials[tri.material_index]; mats[m.name]=m; lines.append('usemtl '+m.name)
            for vi,li in zip(tri.vertices,tri.loops):
                p=mesh.vertices[vi].co if local else o.matrix_world@mesh.vertices[vi].co; n=tri.normal; u=uv[li].uv
                lines.extend(['v %.6f %.6f %.6f'%(p.x,p.z,-p.y),'vt %.6f %.6f'%(u.x,u.y),'vn %.6f %.6f %.6f'%(n.x,n.z,-n.y)])
            lines.append('f '+' '.join('%d/%d/%d'%(j,j,j) for j in range(idx,idx+3))); idx+=3
    (RES/(name+'.obj')).write_text('\n'.join(lines)+'\n',encoding='utf8')
    (RES/(name+'.mtl')).write_text('\n'.join('newmtl %s\nKd 1 1 1\nmap_Kd %s.png\n'%(m,texpaths[m]) for m in mats),encoding='utf8')
for part,objs in parts.items():
    if part!='doors': export(part,objs)
export('porta',door_objects[:1],True)
scene.world.color=(.035,.04,.05)
print('EXPORTS COMPLETE',flush=True)
bpy.ops.object.camera_add(location=(0,1.8,1.7)); cam=bpy.context.object; cam.name='Camera_Inside_Spawn'; cam.rotation_euler=(Vector((0,22,1.5))-cam.location).to_track_quat('-Z','Y').to_euler(); scene.camera=cam; cam.data.lens=20
for y in (4,12,20):
    bpy.ops.object.light_add(type='AREA',location=(0,y,2.65)); l=bpy.context.object; l.name='Preview_Luz_'+str(y); l.data.energy=65; l.data.color=(.4,.72,.82); l.data.shape='RECTANGLE'; l.data.size=2
scene.render.engine='CYCLES'; scene.cycles.samples=16; scene.render.resolution_x=960; scene.render.resolution_y=640; scene.render.resolution_percentage=100
meshes=[o for o in scene.objects if o.type=='MESH']; tris=sum(len(o.data.polygons)*2 for o in meshes); assert tris<5000,tris
print('SAVING MASTER',flush=True)
audit={'triangles':tris,'meshes':len(meshes),'parts':{k:len(v) for k,v in parts.items()},'textures':texpaths,'coordinates':'Tyra=(Blender X, Blender Z, -Blender Y)','doors':[{'x':o.location.x,'z':-o.location.y} for o in door_objects]}
(RES/'audit.json').write_text(json.dumps(audit,indent=2),encoding='utf8')
bpy.ops.wm.save_as_mainfile(filepath=str(MODELS/'ETIP-inside.blend'))
scene.render.filepath=str(MODELS/'ETIP-inside-preview.png'); bpy.ops.render.render(write_still=True)
print('ETIP_INSIDE_AUDIT='+json.dumps(audit))
for name in ('estrutura','portas','salas','terminal'):
    print('SAVING MODULE '+name,flush=True)
    bpy.ops.wm.open_mainfile(filepath=str(MODELS/'ETIP-inside.blend'))
    keep=bpy.data.collections['Inside_'+name]
    for o in list(bpy.data.objects):
        if o.name not in keep.objects: bpy.data.objects.remove(o,do_unlink=True)
    for c in list(bpy.data.collections):
        if c!=keep: bpy.data.collections.remove(c)
    bpy.ops.wm.save_as_mainfile(filepath=str(MODELS/('ETIP-inside-'+name+'.blend')))
