"""Blender generator. Only writes ETIP-inside* and res/etip_inside outputs."""
import bpy, json, random, argparse, sys, math
from pathlib import Path
from mathutils import Vector, Matrix
parser=argparse.ArgumentParser()
repo=Path(__file__).resolve().parents[1]
parser.add_argument('--models',type=Path,default=repo.parent/'DroidCampos-models')
parser.add_argument('--resources',type=Path,default=repo/'game/res/etip_inside')
parser.add_argument('--skip-previews',action='store_true')
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
    # Spatially small quads preserve clipping precision and localize packages.
    # Interpolate the original face UVs; do not stretch/reset the texture per tile.
    original=o.data; vertices=[]; faces=[]; face_uvs=[]
    for polygon in original.polygons:
        a,c1,c2,d=[original.vertices[i].co.copy() for i in polygon.vertices]
        horizontal=c1-a; vertical=d-a
        nx=max(1,math.ceil(horizontal.length/2.0)); ny=max(1,math.ceil(vertical.length/2.0))
        start=len(vertices)
        for j in range(ny+1):
            for i in range(nx+1): vertices.append(a+horizontal*(i/nx)+vertical*(j/ny))
        for j in range(ny):
            for i in range(nx):
                p=start+j*(nx+1)+i
                faces.append((p,p+1,p+nx+2,p+nx+1))
                face_uvs.append(((i/nx,j/ny),((i+1)/nx,j/ny),((i+1)/nx,(j+1)/ny),(i/nx,(j+1)/ny)))
    mesh=bpy.data.meshes.new(o.name+'_Painelado'); mesh.from_pydata(vertices,[],faces); mesh.update()
    o.data=mesh; bpy.data.meshes.remove(original)
    for c in list(o.users_collection): c.objects.unlink(o)
    groups[group].objects.link(o); o.data.materials.append(mat)
    uv=o.data.uv_layers.active or o.data.uv_layers.new()
    for p,coords in zip(o.data.polygons,face_uvs):
        for li,co in zip(p.loop_indices,coords): uv.data[li].uv=co
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
                # The side-room front supplies these wall spans. Do not place
                # a second coplanar corridor wall behind the same textured face.
                if z<=d-2 or a>=d+2: new.append((a,z))
                else:
                    if a<d-2: new.append((a,d-2))
                    if z>d+2: new.append((d+2,z))
            intervals=new
        for a,z in intervals: b('Parede_%s_%s_%s'%(s,side,a),side*1.6,(a+z)/2,1.4,.2,z-a,2.8,wall,'estrutura',part)
        for d in (8,16):
            if lo<=d<hi: b('Verga_%s_%s'%(side,d),side*1.6,d,2.5,.2,1.4,.6,metal,'estrutura',part)
        for j in range(4): b('Nervura_%s_%s_%s'%(s,side,j),side*1.46,lo+j*2+.8,1.4,.08,.12,2.8,metal,'estrutura',part)
        b('Duto_%s_%s'%(s,side),side*1.19,y,2.58,.22,7.95,.24,rust,'estrutura',part)
        b('Baliza_%s_%s'%(s,side),side*1.475,y,.25,.035,.26,.12,red,'estrutura',part)
    b('Luminaria_'+str(s),0,y,2.77,.65,.45,.045,cyan,'estrutura',part)
b('Entrada_Selada',0,0,1.4,3.4,.24,2.8,metal,'estrutura','corredor_1')
# Open connection to a 10 x 8 m terminal room, from Y=24 to Y=32.
b('Sala_Final_Piso',0,28,-.1,10.4,8,.2,floor,'estrutura','sala_terminal')
ROOM_HEIGHT=6.25
b('Sala_Final_Teto',0,28,ROOM_HEIGHT+.1,10.4,8,.2,metal,'estrutura','sala_terminal')
b('Sala_Final_Fundo',0,32.1,ROOM_HEIGHT/2,10.4,.2,ROOM_HEIGHT,wall,'estrutura','sala_terminal')
for side in (-1,1):
    b('Sala_Final_Lateral_'+str(side),side*5.1,28,ROOM_HEIGHT/2,.2,8,ROOM_HEIGHT,wall,'estrutura','sala_terminal')
    b('Sala_Final_Entrada_'+str(side),side*3.35,24,ROOM_HEIGHT/2,3.3,.2,ROOM_HEIGHT,wall,'estrutura','sala_terminal')
    b('Sala_Final_Luminaria_'+str(side),side*2.5,28,ROOM_HEIGHT-.04,.7,.7,.05,cyan,'estrutura','sala_terminal')
    b('Sala_Final_Baliza_'+str(side),side*4.98,30,.3,.025,.3,.14,red,'estrutura','sala_terminal')
b('Sala_Final_Verga',0,24,(2.8+ROOM_HEIGHT)/2,3.4,.2,ROOM_HEIGHT-2.8,metal,'estrutura','sala_terminal')
door_objects=[]
for i,(side,y) in enumerate([(-1,8),(1,8),(-1,16),(1,16)]):
    part='sala_'+str(i+1); x=side*3.65
    b('Sala_Piso_'+str(i),x,y,-.1,4.1,4,.2,floor,'salas',part)
    b('Soleira_'+str(i),side*1.6,y,-.013,.4,1.4,.05,hazard,'portas',part)
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
# Rotate the complete terminal to face +X and place its back on the left wall.
for obj in parts['terminal']:
    old=obj.location.copy(); obj.location=(-4.45-(old.y-22.85),28+old.x,old.z)
    obj.rotation_euler.z=math.pi/2
bpy.context.view_layer.update()

# Append the detailed asset without touching ETIP-capsule.blend or importing
# its presentation cameras/lights. The game gets a separate, hidden LOD.
with bpy.data.libraries.load(str(MODELS/'ETIP-capsule.blend'),link=False) as (src,dst):
    assert 'ETIP_Capsule' in src.collections
    dst.collections=['ETIP_Capsule']
capsule=dst.collections[0];capsule.name='Inside_capsula';scene.collection.children.link(capsule);groups['capsula']=capsule
for o in list(capsule.all_objects):
    if o.type!='MESH':bpy.data.objects.remove(o,do_unlink=True)
bpy.context.view_layer.update()
placement=Matrix.Translation((3.58,29,.012))@Matrix.Rotation(-math.pi/2,4,'Z')
capsule_meshes=list(capsule.all_objects)
for o in capsule_meshes:o.matrix_world=placement@o.matrix_world
bpy.context.view_layer.update()
capsule_bounds=[o.matrix_world@Vector(p) for o in capsule_meshes for p in o.bound_box]
caps_min=[min(v[i] for v in capsule_bounds) for i in range(3)];caps_max=[max(v[i] for v in capsule_bounds) for i in range(3)]
assert caps_max[0]<4.99 and caps_max[2]<ROOM_HEIGHT-.4
assert caps_min[2]>.005 and caps_min[1]>24.5 and caps_max[1]<31.5,(caps_min,caps_max,[(o.name,list(o.location)) for o in capsule_meshes[:4]])
runtime=bpy.data.collections.new('Inside_Capsula_Runtime_LOD');scene.collection.children.link(runtime)
def merged_runtime(name,source):
    vertices=[];faces=[];face_mats=[];face_uvs=[];materials=[]
    for o in source:
        offset=len(vertices);vertices.extend(o.matrix_world@v.co for v in o.data.vertices)
        for p in o.data.polygons:
            faces.append(tuple(offset+i for i in p.vertices));m=o.data.materials[p.material_index]
            if m not in materials:materials.append(m)
            face_mats.append(materials.index(m))
            face_uvs.append([tuple(o.data.uv_layers.active.data[i].uv) if o.data.uv_layers.active else (0,0) for i in p.loop_indices])
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.update()
    for m in materials:mesh.materials.append(m)
    uv=mesh.uv_layers.new(name='UVMap')
    for p,mi,coords in zip(mesh.polygons,face_mats,face_uvs):
        p.material_index=mi
        for li,co in zip(p.loop_indices,coords):uv.data[li].uv=co
    obj=bpy.data.objects.new(name,mesh);runtime.objects.link(obj);return obj
glass_objects=[o for o in capsule_meshes if 'Vidro_Cilindrico' in o.name]
def triangle_count(objs):
    count=0
    for o in objs:o.data.calc_loop_triangles();count+=len(o.data.loop_triangles)
    return count
architecture_triangles=sum(triangle_count(objs) for name,objs in parts.items() if name!='doors')+triangle_count(door_objects)
glass_lod=merged_runtime('PS2_Capsula_Runtime_Vidro',glass_objects)
# Controlled parametric LOD: free edge-collapse distorted rings and panels.
# Keep clean 12-sided cylinders, straight supports and an intact glass shell.
lod_vertices=[];lod_faces=[];lod_mats=[];lod_uvs=[];lod_materials=[]
caps_mats={m.name:m for o in capsule_meshes for m in o.data.materials}
def lod_face(coords,material):
    start=len(lod_vertices);lod_vertices.extend(placement@Vector(p) for p in coords)
    lod_faces.append(tuple(range(start,start+len(coords))))
    m=caps_mats['PS2_Mat_Capsule_'+material]
    if m not in lod_materials:lod_materials.append(m)
    lod_mats.append(lod_materials.index(m))
    lod_uvs.append([(0,0),(1,0),(1,1),(0,1)] if len(coords)==4 else [(0.5+0.5*math.cos(i*math.tau/len(coords)),.5+.5*math.sin(i*math.tau/len(coords))) for i in range(len(coords))])
def lod_cylinder(r,depth,z,material,n=12,xy=(0,0)):
    bottom=[(xy[0]+r*math.cos(i*math.tau/n),xy[1]+r*math.sin(i*math.tau/n),z-depth/2) for i in range(n)]
    top=[(x,y,z+depth/2) for x,y,_ in bottom]
    for i in range(n):j=(i+1)%n;lod_face([bottom[i],bottom[j],top[j],top[i]],material)
    lod_face(top,material)
def lod_ring(r,width,depth,z,material,closed=False,n=12):
    for i in range(n):
        a=i*math.tau/n;b=(i+1)*math.tau/n
        def p(radius,angle,zz):return (radius*math.cos(angle),radius*math.sin(angle),zz)
        lo=z-depth/2;hi=z+depth/2
        lod_face([p(r,a,lo),p(r,b,lo),p(r,b,hi),p(r,a,hi)],material)
        lod_face([p(r,a,hi),p(r,b,hi),p(r-width,b,hi),p(r-width,a,hi)],material)
        if closed:
            lod_face([p(r-width,a,lo),p(r-width,a,hi),p(r-width,b,hi),p(r-width,b,lo)],material)
            lod_face([p(r,b,lo),p(r,a,lo),p(r-width,a,lo),p(r-width,b,lo)],material)
def lod_box(loc,size,material,angle=0):
    x,y,z=loc;sx,sy,sz=[v/2 for v in size];rot=Matrix.Rotation(angle,3,'Z')
    v=[Vector(loc)+rot@Vector(p) for p in [(-sx,-sy,-sz),(sx,-sy,-sz),(sx,sy,-sz),(-sx,sy,-sz),(-sx,-sy,sz),(sx,-sy,sz),(sx,sy,sz),(-sx,sy,sz)]]
    for f in [(3,2,1,0),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7),(4,5,6,7)]:lod_face([v[j] for j in f],material)
for r,depth,z,material in [(1.31,.16,.08,'Aco_Escuro'),(1.34,.095,.205,'Borda_Metal_Gasta'),(1.24,.43,.465,'Pintura_Industrial'),(1.17,.22,.79,'Aco_Escuro'),(1.20,.10,.94,'Borda_Metal_Gasta'),(.98,.10,1.04,'Aco_Escuro'),(1.17,.20,4.93,'Aco_Escuro'),(1.23,.38,5.20,'Pintura_Industrial'),(1.13,.10,5.49,'Pintura_Industrial')]:lod_cylinder(r,depth,z,material)
lod_ring(1.265,.045,.045,.305,'Verde_Energia');lod_ring(.94,.12,.035,1.11,'Verde_Energia')
lod_ring(1.075,.06,.14,2.72,'Pintura_Industrial',True)
for a in (math.radians(25),math.radians(155),math.radians(90)):
    lod_box((1.09*math.cos(a),1.09*math.sin(a),2.94),(.16,.18,3.65),'Aco_Escuro',a-math.pi/2)
lod_box((0,.78,2.98),(.68,.10,3.25),'Aco_Escuro')
for side in (-1,1):lod_box((side*.47,.64,2.91),(.075,.10,3.4),'Borda_Metal_Gasta')
for z in (1.65,2.2,3.25,3.80,4.35):lod_face([(-.14,.702,z-.0175),(.14,.702,z-.0175),(.14,.702,z+.0175),(-.14,.702,z+.0175)],'Verde_Energia')
for side in (-1,1):lod_box((side*.94,-.50,2.72),(.18,.22,.34),'Aco_Escuro')
lod_box((.94,-.79,1.82),(.33,.13,.47),'Aco_Escuro')
lod_face([(.82,-.866,1.75),(1.06,-.866,1.75),(1.06,-.866,2.01),(.82,-.866,2.01)],'Verde_Energia')
for i in range(3):
    xx=.85+i*.09;lod_face([(xx-.022,-.867,1.648),(xx+.022,-.867,1.648),(xx+.022,-.867,1.692),(xx-.022,-.867,1.692)],'Led_Alerta' if i==0 else 'Borda_Metal_Gasta')
lod_box((0,-1.205,.51),(1.02,.10,.30),'Aco_Escuro')
lod_cylinder(.25,.09,5.585,'Aco_Escuro',6)
for i in range(4):
    a=i*math.tau/4;lod_cylinder(.063,.045,5.565,'Borda_Metal_Gasta',6,(1.03*math.cos(a),1.03*math.sin(a)))
# One broad external supply hose maintains the silhouette without tiny clamps.
points=[Vector(p) for p in [( .99,.48,.82),(1.38,.49,1.05),(1.42,.60,2.0),(1.33,.70,3.65),(1.15,.64,4.95)]]
rings=[]
for i,p in enumerate(points):
    tangent=(points[min(i+1,len(points)-1)]-points[max(i-1,0)]).normalized();ref=Vector((1,0,0)) if abs(tangent.z)>.9 else Vector((0,0,1));u=tangent.cross(ref).normalized();v=tangent.cross(u).normalized()
    rings.append([p+.065*(math.cos(j*math.tau/6)*u+math.sin(j*math.tau/6)*v) for j in range(6)])
for i in range(len(rings)-1):
    for j in range(6):k=(j+1)%6;lod_face([rings[i][j],rings[i][k],rings[i+1][k],rings[i+1][j]],'Borracha_Cabos')
lod_face(list(reversed(rings[0])),'Borracha_Cabos');lod_face(rings[-1],'Borracha_Cabos')
lod_face([(-.56,-1.264,5.09),(.56,-1.264,5.09),(.56,-1.264,5.33),(-.56,-1.264,5.33)],'Aco_Escuro')
mesh=bpy.data.meshes.new('Capsula_LOD_Controlado');mesh.from_pydata(lod_vertices,[],lod_faces);mesh.update()
for m in lod_materials:mesh.materials.append(m)
uv=mesh.uv_layers.new(name='UVMap')
for p,mi,coords in zip(mesh.polygons,lod_mats,lod_uvs):
    p.material_index=mi
    for li,co in zip(p.loop_indices,coords):uv.data[li].uv=co
opaque=bpy.data.objects.new('PS2_Capsula_Runtime_Opaca',mesh);runtime.objects.link(opaque)
assert architecture_triangles+triangle_count([opaque,glass_lod])<=5000
for obj in (opaque,glass_lod):
    for corner in obj.data.uv_layers.active.data:
        corner.uv=(max(0,min(1,corner.uv.x)),max(0,min(1,corner.uv.y)))
parts['capsula']=[opaque];parts['capsula_vidro']=[glass_lod]
runtime.hide_render=True;runtime.hide_viewport=True
# Mark capsule bounds for downstream collision and clearance audits.
scene['capsule_bounds_min']=caps_min;scene['capsule_bounds_max']=caps_max
texpaths={}
for objs in parts.values():
    for o in objs:
        for m in o.data.materials:
            if m.name in texpaths: continue
            img=next((n.image for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image),None)
            if img is None and 'Vidro_Contencao' in m.name:
                img=bpy.data.images.new('ps2_tex_capsula_vidro',width=64,height=64)
                img.pixels=[.10,.48,.27,.20]*(64*64);img.pack()
            if img is None:continue
            path=RES/(img.name.lower().removesuffix('.png')+'.png'); img.save_render(str(path)); texpaths[m.name]=path.stem
def export(name,objs,local=False):
    lines=['mtllib '+name+'.mtl']; mats={}; idx=1
    for o in objs:
        mesh=o.data; mesh.calc_loop_triangles(); uv=mesh.uv_layers.active.data
        for tri in mesh.loop_triangles:
            m=mesh.materials[tri.material_index]; mats[m.name]=m; lines.append('usemtl '+m.name)
            for vi,li in zip(tri.vertices,tri.loops):
                p=mesh.vertices[vi].co if local else o.matrix_world@mesh.vertices[vi].co
                n=tri.normal if local else (o.matrix_world.to_3x3().inverted().transposed()@tri.normal).normalized()
                u=uv[li].uv
                lines.extend(['v %.6f %.6f %.6f'%(p.x,p.z,-p.y),'vt %.6f %.6f'%(u.x,u.y),'vn %.6f %.6f %.6f'%(n.x,n.z,-n.y)])
            lines.append('f '+' '.join('%d/%d/%d'%(j,j,j) for j in range(idx,idx+3))); idx+=3
    (RES/(name+'.obj')).write_text('\n'.join(lines)+'\n',encoding='utf8')
    mtl=[]
    for name_m,m in mats.items():
        color=(1,1,1) if name_m in texpaths else tuple(m.diffuse_color[:3])
        mtl.append('newmtl %s\nKd %.6f %.6f %.6f'%(name_m,*color))
        if name_m in texpaths:mtl.append('map_Kd '+texpaths[name_m]+'.png')
    (RES/(name+'.mtl')).write_text('\n'.join(mtl)+'\n',encoding='utf8')
for part,objs in parts.items():
    if part!='doors': export(part,objs)
export('porta',door_objects[:1],True)
scene.world.color=(.035,.04,.05)
print('EXPORTS COMPLETE',flush=True)
bpy.ops.object.camera_add(location=(0,1.8,1.7)); cam=bpy.context.object; cam.name='Camera_Inside_Spawn'; cam.rotation_euler=(Vector((0,22,1.5))-cam.location).to_track_quat('-Z','Y').to_euler(); scene.camera=cam; cam.data.lens=20
main_camera=cam
bpy.ops.object.camera_add(location=(0,24.35,2.75)); room_camera=bpy.context.object; room_camera.name='Camera_Inside_Sala_Final'
room_camera.rotation_euler=(Vector((0,29,2.9))-room_camera.location).to_track_quat('-Z','Y').to_euler(); room_camera.data.lens=12
scene.camera=main_camera
for y in (4,12,20):
    bpy.ops.object.light_add(type='AREA',location=(0,y,2.65)); l=bpy.context.object; l.name='Preview_Luz_'+str(y); l.data.energy=65; l.data.color=(.4,.72,.82); l.data.shape='RECTANGLE'; l.data.size=2
for x in (-2.5,2.5):
    bpy.ops.object.light_add(type='AREA',location=(x,28,ROOM_HEIGHT-.16)); l=bpy.context.object; l.name='Preview_Sala_Final_'+str(x); l.data.energy=350; l.data.color=(.4,.72,.82); l.data.size=3
scene.render.engine='CYCLES'; scene.cycles.samples=16; scene.render.resolution_x=960; scene.render.resolution_y=640; scene.render.resolution_percentage=100
scene.camera=room_camera
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_perspective='CAMERA'
            area.spaces.active.shading.type='MATERIAL'
meshes=[o for o in scene.objects if o.type=='MESH' and o.name not in runtime.objects]
visual_tris=triangle_count(meshes)
tris=sum(triangle_count(objs) for name,objs in parts.items() if name!='doors')+triangle_count(door_objects)
assert tris<=5000,tris
print('SAVING MASTER',flush=True)
audit={'triangles':tris,'visual_triangles':visual_tris,'meshes':len(meshes),'capsule':{'center':[3.58,29,.012],'bounds_min':caps_min,'bounds_max':caps_max,'wall_clearance_m':5-caps_max[0],'ceiling_clearance_m':ROOM_HEIGHT-caps_max[2],'source_triangles':triangle_count(capsule_meshes),'runtime_triangles':triangle_count([opaque,glass_lod])},'max_panel_edge_m':2.0,'terminal_room':{'width':10,'depth':8,'height':ROOM_HEIGHT,'y_start':24,'y_end':32},'parts':{k:len(v) for k,v in parts.items()},'textures':texpaths,'coordinates':'Tyra=(Blender X, Blender Z, -Blender Y)','doors':[{'x':o.location.x,'z':-o.location.y} for o in door_objects]}
(RES/'audit.json').write_text(json.dumps(audit,indent=2),encoding='utf8')
bpy.ops.wm.save_as_mainfile(filepath=str(MODELS/'ETIP-inside.blend'))
if not args.skip_previews:
    scene.camera=main_camera
    scene.render.filepath=str(MODELS/'ETIP-inside-preview.png'); bpy.ops.render.render(write_still=True)
    scene.camera=room_camera; scene.render.filepath=str(MODELS/'ETIP-inside-sala-final-preview.png'); bpy.ops.render.render(write_still=True)
print('ETIP_INSIDE_AUDIT='+json.dumps(audit))
for name in ('estrutura','portas','salas','terminal','capsula'):
    print('SAVING MODULE '+name,flush=True)
    bpy.ops.wm.open_mainfile(filepath=str(MODELS/'ETIP-inside.blend'))
    keep=bpy.data.collections['Inside_'+name]
    keep_objects=set(keep.all_objects)
    def descendants(c):return {c}|set().union(*(descendants(ch) for ch in c.children)) if c.children else {c}
    keep_collections=descendants(keep)
    for o in list(bpy.data.objects):
        if o not in keep_objects: bpy.data.objects.remove(o,do_unlink=True)
    for c in list(bpy.data.collections):
        if c not in keep_collections: bpy.data.collections.remove(c)
    bpy.ops.wm.save_as_mainfile(filepath=str(MODELS/('ETIP-inside-'+name+'.blend')))
