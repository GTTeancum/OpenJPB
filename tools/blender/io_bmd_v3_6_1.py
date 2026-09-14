bl_info = {
    "name": "Jedi Power Battles Models, Animations and Levels",
    "author": "Steve & Claude",
    "version": (4, 2, 2),
    "blender": (4, 0, 0),
    "location": "File > Import/Export > BMD / CAD; File > Import > JPB Level",
    "description": "BMD/CAD import and export, plus native FBX/J3D level import with gameplay metadata",
    "category": "Import-Export",
}

import bpy, bmesh, struct, os, json, base64, math
from bpy.props import StringProperty, BoolProperty, FloatProperty, IntProperty, EnumProperty
from bpy_extras.io_utils import ImportHelper, ExportHelper
from mathutils import Matrix, Vector, Euler

HEADER_SIZE = 4
BONE_STRIDE = 144

# ---------------------------------------------------------------------------
# BMD helpers (native geomData layout)
# ---------------------------------------------------------------------------

def decode_10bit(val):
    x=val&0x3FF; y=(val>>10)&0x3FF; z=(val>>20)&0x3FF
    if x>=512: x-=1024
    if y>=512: y-=1024
    if z>=512: z-=1024
    return (x, y, z)

def encode_10bit(x, y, z):
    x=max(-512,min(511,int(round(x)))); y=max(-512,min(511,int(round(y)))); z=max(-512,min(511,int(round(z))))
    return (x&0x3FF)|((y&0x3FF)<<10)|((z&0x3FF)<<20)

def parse_bones(data):
    if len(data)<292 or struct.unpack_from('<I',data)[0]!=len(data)-4:
        raise ValueError('Invalid BMD size')
    visited=set()
    def visit(index):
        if index in visited or index<1 or 4+(index+1)*144>len(data):
            raise ValueError('Invalid or cyclic BMD hierarchy')
        visited.add(index)
        off=4+index*144
        count=struct.unpack_from('<i',data,off+0x6c)[0]
        if not 0<=count<=8:
            raise ValueError('Invalid BMD child count')
        for j in range(count):
            visit(struct.unpack_from('<i',data,off+0x70+j*4)[0])
    visit(1)
    n_bones=max(visited)+1
    bones=[]; first_oB=None
    for i in range(n_bones):
        off=HEADER_SIZE+i*BONE_STRIDE
        if off+BONE_STRIDE>len(data): break
        oB=struct.unpack_from('<i',data,off+0x38)[0]; vc=struct.unpack_from('<i',data,off+0x30)[0]
        if vc>0 and oB>0 and (first_oB is None or oB<first_oB): first_oB=oB
        nm=data[off:off+32].split(b'\x00')[0].decode('ascii','replace')
        bones.append({
            'idx':i,'name':nm or f'bone_{i}','raw':bytes(data[off:off+BONE_STRIDE]),
            # modelNodeId at +0x20: high byte = bone category (0x10=body, 0x20=auxiliary),
            # low 12 bits (& 0xFFF) = CAD svec slot index for this bone (= av3JointAngle index).
            # RE'd from render_RenderNode (RVA 0x129960) reading [Mnode+0] & 0xFFF as bone idx.
            'node_id':struct.unpack_from('<I',data,off+0x20)[0],
            'cad_slot':struct.unpack_from('<I',data,off+0x20)[0] & 0xFFF,
            'lx':struct.unpack_from('<h',data,off+0x24)[0],
            'ly':struct.unpack_from('<h',data,off+0x26)[0],
            'lz':struct.unpack_from('<h',data,off+0x28)[0],
            'fc':struct.unpack_from('<i',data,off+0x2C)[0],
            'vc':vc*3,
            'oA':struct.unpack_from('<i',data,off+0x34)[0],
            'oB':oB,
            'oD':struct.unpack_from('<i',data,off+0x40)[0],
            'oE':struct.unpack_from('<i',data,off+0x44)[0],
            'face_off':struct.unpack_from('<i',data,off+0x68)[0],
            'texture':data[off+0x48:off+0x68].split(b'\x00')[0].decode('ascii','replace'),
            'parent':None,'children':[],'ax':0,'ay':0,'az':0,
        })
    if first_oB is not None and first_oB<n_bones*BONE_STRIDE:
        raise ValueError("BMD geometry overlaps its node records")
    for i in range(n_bones):
        off=HEADER_SIZE+i*BONE_STRIDE; cc=struct.unpack_from('<i',data,off+0x6C)[0]
        if i in visited and 0<cc<=8:
            for c in range(cc):
                ci=struct.unpack_from('<i',data,off+0x70+c*4)[0]
                if ci<n_bones: bones[ci]['parent']=i; bones[i]['children'].append(ci)
    return bones,n_bones

def compute_cum(bones,bi,px,py,pz):
    b=bones[bi]; b['ax']=px+b['lx']; b['ay']=py+b['ly']; b['az']=pz+b['lz']
    for ci in b['children']: compute_cum(bones,ci,b['ax'],b['ay'],b['az'])

def dfs_order(bones,bi):
    r=[bi]
    for ci in bones[bi]['children']: r.extend(dfs_order(bones,ci))
    return r


# ── Import BMD ────────────────────────────────────────────────────────

def do_import(data, bones, n_bones, scale):
    compute_cum(bones,1,0,0,0) if n_bones>1 else None
    traversal=dfs_order(bones,1) if n_bones>1 else []
    pool={}; vl=[]; vm={}; p2b={}; vb={}
    af=[]; afu=[]; afc=[]; fg=[]; fm=[]; mn=[]; mm={}
    fbm=[]; ffi=[]; fiq=[]
    data_start=n_bones*BONE_STRIDE
    payload=data[HEADER_SIZE+data_start:]

    for bi in traversal:
        b=bones[bi]
        if b['vc']>0 and b['oB']>0:
            for j in range(b['vc']):
                val=struct.unpack_from('<I',data,HEADER_SIZE+b['oB']+j*4)[0]
                pos=decode_10bit(val); pidx=b['oA']+j
                pool[pidx]=((bi,j), (-(pos[0]+b['ax'])*scale,(pos[2]+b['az'])*scale,(pos[1]+b['ay'])*scale))
        if b['fc']<=0: continue
        fs=len(af); fb=HEADER_SIZE+b['face_off']
        tex=b['texture'] or b['name']
        if tex not in mm: mm[tex]=len(mn); mn.append(tex)
        mi=mm[tex]; sec_e_c=0
        for fi in range(b['fc']):
            raw=struct.unpack_from('<4h',data,fb+fi*8)
            iq=raw[3]!=0x7FFF; inds=[abs(v) for v in raw if v!=0x7FFF]; nc=len(inds)
            fv=[]; ok=True
            for idx in inds:
                if idx not in pool: ok=False; break
                key,v=pool[idx]
                if key not in vm:
                    vm[key]=len(vl); vl.append(v); vb[vm[key]]=key[0]
                fv.append(vm[key]); p2b[idx]=vm[key]
            if ok and len(fv)>=3:
                ub=HEADER_SIZE+b['oD']+fi*32; ur=struct.unpack_from('<8f',data,ub)
                uf=[(ur[i*2],ur[i*2+1]) for i in range(4)]
                cf=[]
                for c in range(nc):
                    ce=HEADER_SIZE+b['oE']+(sec_e_c+c)*4
                    cr,cg,cb,ca=struct.unpack_from('<4B',data,ce)
                    cf.append((cr/255.0,cg/255.0,cb/255.0,ca/255.0))
                if iq and len(fv)==4:
                    fv=[fv[2],fv[3],fv[1],fv[0]]
                    fuv=[uf[2],uf[3],uf[1],uf[0]]; fcl=[cf[2],cf[3],cf[1],cf[0]]
                else:
                    fv.reverse(); fuv=[uf[2],uf[1],uf[0]]; fcl=list(reversed(cf))
                af.append(tuple(fv)); afu.append(fuv); afc.append(fcl)
                fm.append(mi); fbm.append(bi); ffi.append(fi); fiq.append(iq)
            sec_e_c+=nc
        fcc=len(af)-fs
        if fcc>0: fg.append((b['name'],fs,fcc))

    return {'verts':vl,'faces':af,'face_uvs':afu,'face_colors':afc,
            'face_groups':fg,'face_mats':fm,'mat_names':mn,
            'vert_bone':vb,'pool_to_blender':p2b,'face_bone_map':fbm,'face_file_idx':ffi,
            'face_is_quad':fiq,'payload':payload,'data_start':data_start}


def make_obj(ctx, name, filepath, scale, md, bones, n_bones):
    mesh=bpy.data.meshes.new(name); obj=bpy.data.objects.new(name,mesh)
    obj['bmd_scale']=scale

    td_tga=os.path.join(os.path.dirname(filepath),"tga")
    td_char=os.path.join(os.path.dirname(filepath),name)
    for mn in md['mat_names']:
        mat=bpy.data.materials.get(mn)
        if mat is None:
            mat=bpy.data.materials.new(name=mn); mat.use_nodes=True
            base=os.path.splitext(mn)[0]+".tga"
            tp=os.path.join(td_tga,base)
            if not os.path.isfile(tp):
                tp=os.path.join(td_char,base)
            if os.path.isfile(tp):
                bs=mat.node_tree.nodes.get("Principled BSDF")
                if bs:
                    tn=mat.node_tree.nodes.new('ShaderNodeTexImage')
                    tn.image=bpy.data.images.load(tp)
                    mat.node_tree.links.new(tn.outputs['Color'],bs.inputs['Base Color'])
                    tn.location=(-300,300)
        mesh.materials.append(mat)

    mesh.from_pydata(md['verts'],[],md['faces'])
    ul=mesh.uv_layers.new(name='UVMap')
    cl=mesh.color_attributes.new(name='Col',type='FLOAT_COLOR',domain='CORNER')
    # Adding a CustomData layer can invalidate an earlier RNA layer reference.
    ul=mesh.uv_layers['UVMap']
    for fi,poly in enumerate(mesh.polygons):
        poly.use_smooth=True
        poly.material_index=md['face_mats'][fi]
        for corner,li in enumerate(poly.loop_indices):
            u,v=md['face_uvs'][fi][corner]
            ul.data[li].uv=(u,1-v)
            cl.data[li].color=md['face_colors'][fi][corner]
    bone_name_to_vg={}
    for gn in dict.fromkeys(b['name'] for b in bones[1:n_bones]):
        vg=obj.vertex_groups.new(name=gn)
        bone_name_to_vg[gn]=vg
    bi_to_name={b['idx']:b['name'] for b in bones[:n_bones]}
    vb=md.get('vert_bone',{})
    bone_verts={}
    for vi,bi in vb.items():
        nm=bi_to_name.get(bi,'')
        if nm: bone_verts.setdefault(nm,[]).append(vi)
    for nm,vis in bone_verts.items():
        vg=bone_name_to_vg.get(nm)
        if vg: vg.add(vis,1.0,'REPLACE')
    mesh.update(); ctx.collection.objects.link(obj)
    ctx.view_layer.objects.active=obj; obj.select_set(True)
    return obj

def make_arm(ctx, name, bones, n_bones, scale):
    ad=bpy.data.armatures.new(name+"_Armature"); ao=bpy.data.objects.new(name+"_Armature",ad)
    ao['bmd_bone_raws']=json.dumps([base64.b64encode(bones[i]['raw']).decode() for i in range(n_bones)])
    ao['bmd_n_bones']=n_bones
    ao['bmd_scale']=scale
    # Store bone name order (file index 1..n_bones-1) for CAD import
    ao['bmd_bone_names']=json.dumps([bones[i]['name'] for i in range(n_bones)])
    # Store per-bone modelNodeId (BMD bone +0x20 field). Low 12 bits = CAD av3JointAngle slot.
    # Used by the CAD import to map (av3JointAngle index → BMD bone) correctly,
    # since BMD bone DFS order is NOT the same as CAD frame slot order — bones in
    # different BMDs can have the same modelNodeId mapping to the same canonical
    # rig slot. RE'd from render_RenderNode reading Mnode.id & 0xFFF as the slot index.
    ao['bmd_bone_node_ids']=json.dumps([bones[i].get('node_id', 0) for i in range(n_bones)])
    ao['bmd_bone_cad_slots']=json.dumps([bones[i].get('cad_slot', 0) for i in range(n_bones)])
    ctx.collection.objects.link(ao); ctx.view_layer.objects.active=ao; ao.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT'); ebs={}
    for bi in (dfs_order(bones,1) if n_bones>1 else []):
        b=bones[bi]
        if not b['name']: continue
        eb=ad.edit_bones.new(b['name'])
        h=(-(b['ax'])*scale,b['az']*scale,b['ay']*scale)
        eb.head=h; eb.tail=(h[0],h[1],h[2]+5*scale)
        if b['parent'] is not None and b['parent'] in ebs: eb.parent=ebs[b['parent']]
        ebs[bi]=eb
    bpy.ops.object.mode_set(mode='OBJECT'); return ao


class ImportBMD(bpy.types.Operator, ImportHelper):
    """Import a BMD character model"""
    bl_idname="import_mesh.bmd"; bl_label="Import BMD"; bl_options={'REGISTER','UNDO'}
    filename_ext=".bmd"
    filter_glob: StringProperty(default="*.bmd",options={'HIDDEN'})
    import_armature: BoolProperty(name="Import Armature",default=True)
    scale_factor: FloatProperty(name="Scale",default=0.01,min=0.001,max=10.0)

    def execute(self,ctx):
        try:
            with open(self.filepath,'rb') as f: data=f.read()
            bones,nb=parse_bones(data)
        except (OSError,ValueError,struct.error) as exc:
            self.report({'ERROR'},str(exc)); return {'CANCELLED'}
        name=os.path.splitext(os.path.basename(self.filepath))[0]; scale=self.scale_factor
        if nb<2: self.report({'ERROR'},"No valid bones"); return {'CANCELLED'}
        md=do_import(data,bones,nb,scale)
        if not md['verts'] or not md['faces']:
            self.report({'ERROR'},"No geometry"); return {'CANCELLED'}
        bpy.ops.object.select_all(action='DESELECT')
        mo=make_obj(ctx,name,self.filepath,scale,md,bones,nb)
        if self.import_armature:
            ao=make_arm(ctx,name,bones,nb,scale)
            mo.select_set(True); ao.select_set(True)
            ctx.view_layer.objects.active=ao; bpy.ops.object.parent_set(type='ARMATURE')
        if self.import_armature:
            mo['bmd_archive']=base64.b64encode(data).decode('ascii')
            mo['bmd_snapshot']=bmd_snapshot(mo,ao)
        self.report({'INFO'},f"Imported {name}: {len(md['verts'])} v, {len(md['faces'])} f, {nb} bones")
        return {'FINISHED'}


# The codec is shared with the non-Blender regression suite.
import sys
_addon_dir = os.path.dirname(os.path.abspath(__file__))
if _addon_dir not in sys.path:
    sys.path.insert(0, _addon_dir)
from cad_operators import ImportCAD, ExportCAD

# ---------------------------------------------------------------------------
# BMD Export
# ---------------------------------------------------------------------------

def bmd_snapshot(obj, arm):
    import hashlib
    mesh=obj.data
    state=[[[round(c,6) for c in v.co] for v in mesh.vertices],
           [g.name for g in obj.vertex_groups],
           [list(n.vector) for n in mesh.corner_normals],
           [[(g.group,round(g.weight,6)) for g in v.groups] for v in mesh.vertices],
           [(list(p.vertices),p.material_index) for p in mesh.polygons],
           [m.name if m else None for m in mesh.materials],
           [[list(d.uv) for d in layer.data] for layer in mesh.uv_layers],
           [[list(d.color) for d in layer.data] for layer in mesh.color_attributes],
           [(b.name,b.parent.name if b.parent else None,list(b.head_local),list(b.tail_local)) for b in arm.data.bones],
           obj.get('bmd_scale',0.01)]
    return hashlib.sha256(json.dumps(state,sort_keys=True).encode()).hexdigest()


class ExportBMD(bpy.types.Operator, ExportHelper):
    """Export an edited mesh on its imported BMD rig"""
    bl_idname="export_mesh.bmd"; bl_label="Export BMD"; bl_options={'REGISTER','UNDO'}
    filename_ext=".bmd"
    filter_glob: StringProperty(default="*.bmd",options={'HIDDEN'})

    def execute(self,ctx):
        obj=ctx.active_object
        mesh_obj=None; arm_obj=None
        if obj is not None:
            if obj.type=='MESH':
                mesh_obj=obj
                arm_obj=obj.parent if obj.parent and obj.parent.type=='ARMATURE' else None
                if arm_obj is None:
                    for mod in obj.modifiers:
                        if mod.type=='ARMATURE' and mod.object:
                            arm_obj=mod.object; break
            elif obj.type=='ARMATURE':
                arm_obj=obj
                for child in obj.children:
                    if child.type=='MESH':
                        mesh_obj=child; break
        if mesh_obj is None:
            for o in ctx.scene.objects:
                if o.type=='MESH' and o.parent and o.parent.type=='ARMATURE':
                    mesh_obj=o; arm_obj=o.parent; break
        if mesh_obj is None:
            self.report({'ERROR'},"No mesh with armature found"); return {'CANCELLED'}
        if arm_obj is None:
            self.report({'ERROR'},"Mesh must be parented to an armature"); return {'CANCELLED'}
        try: result=self._build(mesh_obj,arm_obj)
        except Exception as e:
            import traceback; traceback.print_exc()
            self.report({'ERROR'},f"{type(e).__name__}: {e}"); return {'CANCELLED'}
        with open(self.filepath,'wb') as f: f.write(result)
        self.report({'INFO'},f"Exported BMD: {len(result)} bytes")
        return {'FINISHED'}

    def _build(self,obj,arm_obj):
        if any(m.type!='ARMATURE' and m.show_viewport for m in obj.modifiers):
            raise ValueError('Apply mesh modifiers before BMD export')
        if obj.data.shape_keys and any(abs(k.value)>0.00001 for k in obj.data.shape_keys.key_blocks):
            raise ValueError('Apply the desired shape to the base mesh before BMD export')
        if obj.get('bmd_snapshot') == bmd_snapshot(obj,arm_obj):
            return base64.b64decode(obj['bmd_archive'])
        scale=obj.get('bmd_scale',0.01)
        mesh=obj.data; arm=arm_obj.data
        if scale<=0 or any(abs(obj.matrix_local[r][c]-(1 if r==c else 0))>0.00001 for r in range(4) for c in range(4)):
            raise ValueError('Apply mesh object transforms before BMD export')
        bone_raws=None; file_nb=0
        if 'bmd_bone_raws' in arm_obj:
            bone_raws=[bytearray(base64.b64decode(r)) for r in json.loads(arm_obj['bmd_bone_raws'])]
            file_nb=len(bone_raws)
        arm_name_to_file={}
        if bone_raws:
            for fi_b in range(file_nb):
                nm=bone_raws[fi_b][0:32].split(b'\x00')[0].decode('ascii','replace')
                if nm: arm_name_to_file[nm]=fi_b
        else:
            raise ValueError('Import a BMD rig first to preserve canonical node IDs and attachments')
            arm_name_to_file={b.name:i+1 for i,b in enumerate(arm.bones)}
            file_nb=len(arm.bones)+1
        bones=[None]*file_nb
        for i in range(file_nb):
            bones[i]={'idx':i,'name':'','parent':None,'children':[],
                       'lx':0,'ly':0,'lz':0,'ax':0,'ay':0,'az':0,'vc':0,'oA':0,'fc':0}
        for ab in arm.bones:
            fi_b=arm_name_to_file.get(ab.name)
            if fi_b is None: continue
            parent_fi=arm_name_to_file.get(ab.parent.name) if ab.parent else None
            hx,hy,hz=ab.head_local
            bones[fi_b]['name']=ab.name; bones[fi_b]['parent']=parent_fi
            bones[fi_b]['wx']=-hx/scale; bones[fi_b]['wy']=hz/scale; bones[fi_b]['wz']=hy/scale
        for b in bones:
            if b['parent'] is not None and b['parent']<file_nb:
                bones[b['parent']]['children'].append(b['idx'])
        root_bi=1
        for i in range(1,file_nb):
            if bones[i]['name'] and bones[i]['parent'] is None:
                root_bi=i; break
        for b in bones:
            if 'wx' not in b: continue
            if b['parent'] is not None and 'wx' in bones[b['parent']]:
                pb=bones[b['parent']]
                b['lx']=int(round(b['wx']-pb['wx']))
                b['ly']=int(round(b['wy']-pb['wy']))
                b['lz']=int(round(b['wz']-pb['wz']))
            else:
                b['lx']=int(round(b.get('wx',0))); b['ly']=int(round(b.get('wy',0))); b['lz']=int(round(b.get('wz',0)))
        def recompute_cum(bi,px,py,pz):
            b=bones[bi]; b['ax']=px+b['lx']; b['ay']=py+b['ly']; b['az']=pz+b['lz']
            for ci in b['children']: recompute_cum(ci,b['ax'],b['ay'],b['az'])
        recompute_cum(root_bi,0,0,0)
        traversal=dfs_order(bones,root_bi)
        vg_map={}
        for vg in obj.vertex_groups:
            fi_b=arm_name_to_file.get(vg.name)
            if fi_b is not None: vg_map[vg.index]=fi_b
        vert_bone={}
        for v in mesh.vertices:
            influences=[g for g in v.groups if g.group in vg_map and g.weight>0.00001]
            if len(influences)!=1:
                raise ValueError(f'Vertex {v.index} must have exactly one bone weight; BMD uses rigid joints')
            best_bi=None; best_w=0
            for g in v.groups:
                bi=vg_map.get(g.group)
                if bi is not None and g.weight>best_w:
                    best_w=g.weight; best_bi=bi
            if best_bi is not None: vert_bone[v.index]=best_bi
        face_bone=[]
        corner_normals=[n.vector.copy() for n in mesh.corner_normals]
        uv_layer=mesh.uv_layers[0] if mesh.uv_layers else None
        # Preserve imported per-corner colors; use canonical defaults only for new meshes.
        col_attr=mesh.color_attributes.get('Col')
        if any(len(p.vertices) not in (3,4) for p in mesh.polygons):
            raise ValueError('BMD supports triangles and quads; triangulate n-gons before export')
        if any(len(b['children']) > 8 for b in bones):
            raise ValueError('BMD supports at most eight children per node')
        if any(b.name not in arm_name_to_file for b in arm.bones):
            raise ValueError('New bones need explicit game joint IDs; use the imported rig')
        dfs_pos={bi:pos for pos,bi in enumerate(traversal)}
        for poly in mesh.polygons:
            latest_bi=None; latest_pos=-1
            for vi in poly.vertices:
                bi=vert_bone.get(vi)
                if bi is not None:
                    p=dfs_pos.get(bi,0)
                    if p>latest_pos: latest_pos=p; latest_bi=bi
            face_bone.append(latest_bi if latest_bi is not None else (traversal[0] if traversal else root_bi))
        bone_faces={bi:[] for bi in range(file_nb)}
        for fi,bi in enumerate(face_bone): bone_faces[bi].append(fi)
        for bi,faces in bone_faces.items():
            if len({mesh.polygons[fi].material_index for fi in faces})>1:
                raise ValueError(f'Node {bones[bi]["name"]} uses multiple materials; BMD supports one material per node')
        for fi,bi in enumerate(face_bone):
            for vi in mesh.polygons[fi].vertices:
                if vi not in vert_bone: vert_bone[vi]=bi
        bone_owned={bi:[] for bi in range(file_nb)}
        for vi,bi in vert_bone.items(): bone_owned[bi].append(vi)
        vert_to_pool={}; pool_cursor=[0]
        def assign_pool(bi):
            owned=bone_owned[bi]; oA=pool_cursor[0]
            for vi in owned:
                if vi not in vert_to_pool: vert_to_pool[vi]=pool_cursor[0]; pool_cursor[0]+=1
            bones[bi]['vc']=((pool_cursor[0]-oA+2)//3)*3; bones[bi]['oA']=oA; bones[bi]['fc']=len(bone_faces[bi])
            pool_cursor[0]=oA+bones[bi]['vc']
            for ci in bones[bi]['children']: assign_pool(ci)
            # Keep stable cache indices across sibling subtrees.
        assign_pool(traversal[0])
        if pool_cursor[0]>3072:
            raise ValueError('BMD exceeds the original 3072-vertex transformed cache; reduce vertex count')
        sec_b=bytearray(); clamp=0
        for bi in traversal:
            b=bones[bi]; oA=b['oA']; vc=b['vc']
            pidx_to_vi={vert_to_pool[vi]:vi for vi in bone_owned[bi] if oA<=vert_to_pool[vi]<oA+vc}
            for j in range(vc):
                vi=pidx_to_vi.get(oA+j)
                if vi is not None:
                    bx,by,bz=mesh.vertices[vi].co
                    wx,wy,wz=-bx/scale,bz/scale,by/scale
                    lx,ly,lz=wx-b['ax'],wy-b['ay'],wz-b['az']
                    for c in (lx,ly,lz):
                        if c<-512 or c>511: clamp+=1
                    sec_b+=struct.pack('<I',encode_10bit(lx,ly,lz))
                else:
                    sec_b+=struct.pack('<I',0)
        sec_a=bytearray()
        for bi in traversal:
            for fi in bone_faces[bi]:
                poly=mesh.polygons[fi]
                loops=list(poly.loop_indices)
                loops=[loops[3],loops[2],loops[0],loops[1]] if len(loops)==4 else list(reversed(loops))
                for li in loops:
                    normal=corner_normals[li]
                    sec_a+=struct.pack('<I',encode_10bit(-normal.x*511,normal.z*511,normal.y*511))
        if clamp:
            raise ValueError(f'{clamp} coordinates exceed the BMD signed 10-bit range; export cancelled')
        face_data=bytearray()
        for bi in traversal:
            for fi in bone_faces[bi]:
                poly=mesh.polygons[fi]; verts=list(poly.vertices); nc=len(verts)
                if nc==4: file_verts=[verts[3],verts[2],verts[0],verts[1]]
                else: file_verts=list(reversed(verts))
                pidxs=[vert_to_pool[v] for v in file_verts]
                if nc==4: face_data+=struct.pack('<4h',pidxs[0],pidxs[1],pidxs[2],pidxs[3])
                else: face_data+=struct.pack('<4h',pidxs[0],pidxs[1],pidxs[2],0x7FFF)
        sec_d=bytearray()
        for bi in traversal:
            for fi in bone_faces[bi]:
                poly=mesh.polygons[fi]; nc=len(poly.loop_indices)
                if uv_layer:
                    buvs=[(uv_layer.data[li].uv[0],1.0-uv_layer.data[li].uv[1]) for li in poly.loop_indices]
                else:
                    buvs=[(0,0)]*nc
                if nc==4: fuvs=[buvs[3],buvs[2],buvs[0],buvs[1]]
                else: fuvs=[buvs[2],buvs[1],buvs[0]]
                for ci in range(4):
                    if ci<len(fuvs): sec_d+=struct.pack('<ff',fuvs[ci][0],fuvs[ci][1])
                    else: sec_d+=struct.pack('<ff',-1.0,-1.0)
        sec_e=bytearray()
        for bi in traversal:
            for fi in bone_faces[bi]:
                poly=mesh.polygons[fi]; nc=len(poly.loop_indices)
                if col_attr:
                    bcols=[]
                    for li in poly.loop_indices:
                        c=col_attr.data[li].color
                        bcols.append((int(round(c[0]*255)),int(round(c[1]*255)),
                                      int(round(c[2]*255)),int(round(c[3]*255)) if len(c)>3 else 255))
                else:
                    bcols=[(192,192,192,52)]*nc
                if nc==4: fcols=[bcols[3],bcols[2],bcols[0],bcols[1]]
                else: fcols=[bcols[2],bcols[1],bcols[0]]
                for cr,cg,cb,ca in fcols: sec_e+=struct.pack('<4B',cr,cg,cb,ca)
        bone_block=file_nb*BONE_STRIDE
        sec_b_off=bone_block; sec_a_off=sec_b_off+len(sec_b)
        sec_d_off=sec_a_off+len(sec_a); face_off_base=sec_d_off+len(sec_d)
        sec_e_off=face_off_base+len(face_data)
        cur_b=sec_b_off; cur_a=sec_a_off; cur_d=sec_d_off; cur_f=face_off_base; cur_e=sec_e_off
        for bi in traversal:
            b=bones[bi]
            b['oB_abs']=cur_b; b['oA_abs']=cur_a
            b['oD_abs']=cur_d; b['face_off_abs']=cur_f; b['oE_abs']=cur_e
            cur_b+=b['vc']*4; fc=b['fc']
            cur_a+=sum(len(mesh.polygons[fi].loop_indices) for fi in bone_faces[bi])*4
            cur_d+=fc*32; cur_f+=fc*8
            ec=sum(len(mesh.polygons[fi].loop_indices) for fi in bone_faces[bi])
            cur_e+=ec*4
        bone_block_data=bytearray(bone_block)
        for bi in range(file_nb):
            b=bones[bi]; off=bi*BONE_STRIDE; raw=bytearray(BONE_STRIDE)
            if bone_raws and bi<len(bone_raws): raw[:]=bone_raws[bi]
            if not b['name']: bone_block_data[off:off+BONE_STRIDE]=raw; continue
            nm=b['name'].encode('ascii','strict')[:31]
            raw[0:32]=nm+b'\x00'*(32-len(nm))
            struct.pack_into('<h',raw,0x24,b['lx']); struct.pack_into('<h',raw,0x26,b['ly'])
            struct.pack_into('<h',raw,0x28,b['lz']); struct.pack_into('<i',raw,0x2C,b['fc'])
            struct.pack_into('<i',raw,0x30,b['vc']//3); struct.pack_into('<i',raw,0x34,b['oA'])
            struct.pack_into('<i',raw,0x38,b.get('oB_abs',0)); struct.pack_into('<i',raw,0x3C,b.get('oA_abs',0))
            struct.pack_into('<i',raw,0x40,b.get('oD_abs',0)); struct.pack_into('<i',raw,0x44,b.get('oE_abs',0))
            tex_name=b''
            if bone_faces[bi]:
                fi0=bone_faces[bi][0]; p=mesh.polygons[fi0]
                if p.material_index<len(mesh.materials) and mesh.materials[p.material_index]:
                    tex_name=mesh.materials[p.material_index].name.encode('ascii','replace')[:31]
            raw[0x48:0x68]=tex_name+b'\x00'*(32-len(tex_name))
            struct.pack_into('<i',raw,0x68,b.get('face_off_abs',0))
            cc=len(b['children']); struct.pack_into('<i',raw,0x6C,cc)
            raw[0x70:0x90]=bytes(32)
            for ci_idx,ci in enumerate(b['children']):
                if ci_idx<8: struct.pack_into('<i',raw,0x70+ci_idx*4,ci)
            bone_block_data[off:off+BONE_STRIDE]=raw
        payload=sec_b+sec_a+sec_d+face_data+sec_e
        total=len(bone_block_data)+len(payload)
        if total+4>0x40000:
            raise ValueError('BMD exceeds the runtime 256 KiB capacity')
        out=bytearray(HEADER_SIZE+total)
        struct.pack_into('<I',out,0,total)
        out[HEADER_SIZE:HEADER_SIZE+len(bone_block_data)]=bone_block_data
        out[HEADER_SIZE+bone_block:]=payload
        if clamp>0: self.report({'WARNING'},f"{clamp} coords clamped to 10-bit range")
        return bytes(out)


# ---------------------------------------------------------------------------
# Registration
# ---------------------------------------------------------------------------

from level_operators import ImportJPBLevel, JPBLevelInspector, GAMEPLAY_CLASSES, register_camera_browser, unregister_camera_browser

def menu_func_import_level(self, ctx): self.layout.operator(ImportJPBLevel.bl_idname, text="JPB Level (.fbx / .jpx / .j3d)")

def menu_func_import_bmd(self, ctx): self.layout.operator(ImportBMD.bl_idname, text="BMD Model (.bmd)")
def menu_func_import_cad(self, ctx): self.layout.operator(ImportCAD.bl_idname, text="CAD Animation (.cad)")
def menu_func_export_bmd(self, ctx): self.layout.operator(ExportBMD.bl_idname, text="BMD Model (.bmd)")

def menu_func_export_cad(self, ctx): self.layout.operator(ExportCAD.bl_idname, text="CAD Animation (.cad)")

def register():
    register_camera_browser()
    for cls in GAMEPLAY_CLASSES:bpy.utils.register_class(cls)
    bpy.utils.register_class(JPBLevelInspector)
    bpy.utils.register_class(ImportJPBLevel)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import_level)
    bpy.utils.register_class(ImportBMD)
    bpy.utils.register_class(ImportCAD)
    bpy.utils.register_class(ExportBMD)
    bpy.utils.register_class(ExportCAD)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import_bmd)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import_cad)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export_bmd)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export_cad)

def unregister():
    unregister_camera_browser()
    for cls in reversed(GAMEPLAY_CLASSES):bpy.utils.unregister_class(cls)
    bpy.utils.unregister_class(JPBLevelInspector)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import_level)
    bpy.utils.unregister_class(ImportJPBLevel)
    bpy.utils.unregister_class(ImportBMD)
    bpy.utils.unregister_class(ImportCAD)
    bpy.utils.unregister_class(ExportBMD)
    bpy.utils.unregister_class(ExportCAD)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import_bmd)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import_cad)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export_bmd)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export_cad)

if __name__ == "__main__":
    register()
