"""Import-only level scene. The adjacent native helper owns format decoding."""
import bpy
import json
import subprocess
import tempfile
import hashlib
from pathlib import Path
from bpy.props import StringProperty, FloatProperty, BoolProperty, IntProperty
from bpy_extras.io_utils import ImportHelper
import math
from level_gameplay import GAMEPLAY_CLASSES, import_gameplay, load_level_sidecars, archive_inventory, register_camera_browser, unregister_camera_browser


MARKER_STYLES={
    'Placement':((1,.22,.06,1),'Actor placement'),
    'Pickup':((.12,1,.25,1),'Pickup'),
    'Player start':((.05,.65,1,1),'Player start'),
    'World start':((.7,.35,1,1),'Archive origin'),
    'Waypoint':((1,.7,.05,1),'Waypoint'),
}


def level_marker_visual(obj,kind,label,scale):
    """Persistent editor geometry: readable in Solid and Material Preview alike."""
    color,_=MARKER_STYLES[kind]
    mesh=bpy.data.meshes.new('JPB '+kind+' icon')
    vertices=[];faces=[]
    def ring(z,r,n=12):
        start=len(vertices)
        vertices.extend((math.cos(i*math.tau/n)*r,math.sin(i*math.tau/n)*r,z) for i in range(n))
        return list(range(start,start+n))
    def join(a,b):
        for i in range(len(a)):faces.append((a[i],a[(i+1)%len(a)],b[(i+1)%len(b)],b[i]))
    def cylinder(z1,z2,r1,r2):
        a=ring(z1,r1);b=ring(z2,r2);join(a,b);faces.extend((tuple(reversed(a)),tuple(b)))
    if kind=='Placement':
        # Upright pawn silhouette with shoulders and head.
        cylinder(0,12,65,65);cylinder(12,130,36,50);cylinder(135,190,28,28)
    elif kind=='Pickup':
        a=ring(85,60,4);bottom=len(vertices);vertices.extend(((0,0,10),(0,0,160)))
        for i in range(4):faces.extend(((bottom,a[(i+1)%4],a[i]),(bottom+1,a[i],a[(i+1)%4])))
    elif kind=='Waypoint':
        a=ring(8,50);b=ring(8,35);join(a,b)
    else:
        cylinder(0,12,65,65);cylinder(12,220,6,6)
        start=len(vertices);vertices.extend(((0,0,220),(115,0,220),(85,0,150),(0,0,150)))
        faces.append(tuple(range(start,start+4)))
    mesh.from_pydata([(x*scale,y*scale,z*scale) for x,y,z in vertices],[],faces)
    obj.data=mesh;obj.color=color;obj.show_in_front=True
    obj['jpb_kind']=kind;obj['jpb_label']=label;obj['jpb_marker_version']=2
    material=bpy.data.materials.get('JPB icon '+kind)
    if material is None:
        material=bpy.data.materials.new('JPB icon '+kind);material.diffuse_color=color;material.use_nodes=True
        shader=material.node_tree.nodes.get('Principled BSDF')
        shader.inputs['Base Color'].default_value=color
        shader.inputs['Emission Color'].default_value=color;shader.inputs['Emission Strength'].default_value=.7
    mesh.materials.append(material)
    text=bpy.data.curves.new('JPB label','FONT');text.body=label;text.size=scale*62
    text.align_x='CENTER';text.extrude=scale*.4;text.materials.append(material)
    caption=bpy.data.objects.new(label+' label',text)
    obj.users_collection[0].objects.link(caption);caption.parent=obj
    caption.location=(0,0,scale*(250 if kind!='Waypoint' else 65))
    caption.rotation_euler=(math.pi/2,0,0);caption.hide_render=True
    caption.hide_select=True;caption.show_in_front=True;caption.color=color
    caption['jpb_caption']=True
    # Waypoints remain identifiable when enabled without flooding the map with labels.
    if kind=='Waypoint':caption.hide_set(True)


class JPBLevelInspector(bpy.types.Panel):
    bl_label='JPB Level Objects'
    bl_idname='VIEW3D_PT_jpb_level_objects'
    bl_space_type='VIEW_3D';bl_region_type='UI';bl_category='JPB'

    def draw(self,context):
        layout=self.layout
        layout.label(text='Actors and pickups: game models')
        layout.label(text='Orange / green icons: missing models')
        layout.label(text='Blue flag: player start')
        layout.label(text='Purple flag: archive origin')
        layout.label(text='Gold ring: waypoint')
        obj=context.object
        if obj and obj.get('jpb_record'):
            layout.separator();layout.label(text=obj.get('jpb_label',obj.name),icon='OBJECT_DATA')
            record=json.loads(obj['jpb_record'])
            for key in ('actor_id','ai_id','enemy_id','type','rate','data','player','flags','trigger_id','target_placement'):
                if key in record:layout.label(text=key.replace('_',' ').title()+': '+str(record[key]))
            if obj.get('jpb_actor'):layout.label(text='Actor: '+obj['jpb_actor'])
            for field,title in (('jpb_target_object','Select Trigger Target'),('jpb_camera_object','Select Camera Rig')):
                if obj.get(field):
                    button=layout.operator('jpb.select_reference',text=title);button.object_name=obj[field]
            if obj.get('jpb_script_tree'):
                button=layout.operator('jpb.open_script',text='Open Script Graph',icon='NODETREE')
                button.tree_name=obj['jpb_script_tree']
            for key in ('offset','pitch','yaw','slack','off','keys'):
                if key in record:layout.label(text=key.title()+': '+str(record[key] if key!='keys' else len(record[key])))
            if 'links' in record:layout.label(text='Linked placements: '+(', '.join(map(str,record['links'])) or 'None'))
            if 'waypoints' in record:layout.label(text='Waypoints: '+str(len(record['waypoints'])))
        else:layout.label(text='Select a marker to inspect its data')
        for root in context.scene.collection.children:
            if 'jpb_import_version' in root:
                layout.separator();layout.label(text=root.name)
                for group in root.children:
                    if group.name.startswith(('Camera','Trigger','Powerup','Placement Links','Waypoint','Level Animation')):
                        layout.prop(group,'hide_viewport',text=group.name,toggle=True,invert_checkbox=True)


def level_paths(filename, gameplay_override=''):
    source=Path(filename).resolve()
    if not source.is_file():
        raise ValueError('Select an existing FBX or J3D level file')
    fbx=j3d=None
    if source.suffix.lower() in ('.fbx','.jpx'):
        fbx=source
        candidates=[source.with_suffix('.j3d'), source.parent.parent.parent/'W3D'/f'{source.parent.name}.j3d']
        j3d=next((p for p in candidates if p.is_file()),None)
    elif source.suffix.lower()=='.j3d':
        j3d=source
        candidates=[source.with_suffix('.fbx'),source.parent.parent/'jpx'/source.stem/f'{source.stem}.fbx']
        fbx=next((p for p in candidates if p.is_file()),None)
    else:
        raise ValueError('Select FBX/JPX visual geometry or J3D gameplay data')
    if gameplay_override:
        j3d=Path(gameplay_override).resolve()
        if not j3d.is_file(): raise ValueError('Gameplay file does not exist')
    return source,fbx,j3d


def level_native_data(fbx,j3d,index=-1,helper=None):
    helper=Path(helper) if helper else Path(__file__).with_name('jpb_level_import.exe')
    if not helper.is_file():
        raise ValueError('Missing jpb_level_import.exe beside the add-on; install the complete level importer package')
    with tempfile.TemporaryDirectory(prefix='openjpb-level-') as directory:
        output=Path(directory)/'scene.json'
        result=subprocess.run([str(helper),str(fbx) if fbx else '-',str(j3d) if j3d else '-',str(index),str(output)],
            capture_output=True,text=True,errors='replace',timeout=180,
            creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        if result.returncode or not output.is_file():
            raise ValueError(f'Native level reader failed ({result.returncode}): {result.stderr[-500:]}')
        data=json.loads(output.read_text(encoding='utf-8'))
        if data.get('version')!=1: raise ValueError('Unsupported native level reader version')
        return data


def import_level_scene(context,filename,scale=0.01,gameplay_override='',index=-1,helper=None):
    source,fbx,j3d=level_paths(filename,gameplay_override)
    data=level_native_data(fbx,j3d,index,helper)
    if fbx and fbx.suffix.lower()=='.jpx':
        # JPX emits one runtime batch per strip; group static strips by material
        # for editing, retaining every original strip index on the object.
        grouped={}
        for batch in data['meshes']:
            key=(batch['texture'],batch['pass'])
            if key not in grouped:
                grouped[key]={**batch,'vertices':[],'name':batch['texture'] or 'JPX geometry','strip_indices':[]}
            grouped[key]['vertices'].extend(batch['vertices'])
            grouped[key]['strip_indices'].append(batch['mesh_index'])
        data['meshes']=list(grouped.values())
    # Decode completely before touching the scene; roll back only new datablocks.
    stores=(bpy.data.objects,bpy.data.meshes,bpy.data.curves,bpy.data.materials,
            bpy.data.images,bpy.data.texts,bpy.data.collections,bpy.data.node_groups,bpy.data.actions,bpy.data.cameras)
    before=[set(s) for s in stores]
    model_cache={}
    resource_root=next((p for p in source.parents if p.name.lower()=='res'),None)
    load_level_sidecars(data,resource_root)
    if j3d:data['archive_inventory']=archive_inventory(j3d)
    def model_mesh(stem):
        if not stem or resource_root is None:return None
        if stem in model_cache:return model_cache[stem]
        path=resource_root/'MODEL'/(stem+'.bmd')
        if not path.is_file():return None
        import io_bmd_v3_6_1 as character
        raw=path.read_bytes();bones,count=character.parse_bones(raw)
        decoded=character.do_import(raw,bones,count,scale)
        model=character.make_obj(context,stem,str(path),scale,decoded,bones,count)
        mesh=model.data;bpy.data.objects.remove(model,do_unlink=True)
        model_cache[stem]=mesh
        return mesh
    def xyz(v): return (-v[0]*scale,v[2]*scale,v[1]*scale)
    def collection(name,parent):
        c=bpy.data.collections.new(name);parent.children.link(c);return c
    def marker(name,position,record,group):
        obj=bpy.data.objects.new(name,bpy.data.meshes.new(name));group.objects.link(obj)
        obj.location=xyz(position)
        obj.hide_render=True;obj['jpb_record']=json.dumps(record,separators=(',',':'))
        if 'actor_id' in record:
            actor=record['actor_id'];actor_name=data['actors'][actor] if 0<=actor<len(data['actors']) else 'Actor'
            actor_name=Path(actor_name.replace('\\','/')).stem.lower()
            label=f"{data.get('actor_models',{}).get(actor_name,actor_name)} #{record['id']}";kind='Placement'
        elif record.get('kind')=='player_start':label=name;kind='Player start'
        elif record.get('kind')=='world_start':label='Archive origin';kind='World start'
        elif 'type' in record:label=f"Pickup {record['type']}";kind='Pickup'
        else:label=name;kind='Waypoint'
        obj.name=label
        stem=''
        if kind=='Placement':stem=data.get('actor_models',{}).get(actor_name,'')
        elif kind=='Pickup' and record['type']<len(data.get('pickup_models',[])):stem=data['pickup_models'][record['type']]
        actual=model_mesh(stem)
        if actual is not None:
            old_mesh=obj.data;obj.data=actual;bpy.data.meshes.remove(old_mesh)
            obj['jpb_kind']=kind;obj['jpb_label']=label;obj['jpb_model']=stem;obj['jpb_marker_version']=2
            obj.hide_render=False
            if kind=='Pickup':obj.scale=(data['pickup_scales'][record['type']]/4096,)*3
            if kind=='Placement':
                import struct
                angle=struct.unpack_from('<i',bytes.fromhex(record['defaults_hex']),36)[0]
                obj.rotation_euler.z=angle*math.tau/4096
            return obj
        old_mesh=obj.data;level_marker_visual(obj,kind,label,scale);bpy.data.meshes.remove(old_mesh)
        return obj
    missing=set()
    try:
        root=collection(f'JPB {source.stem}',context.scene.collection)
        root['jpb_level_index']=data['level_index'];root['jpb_scale']=scale
        root['jpb_source']=str(source);root['jpb_import_version']=1
        visuals=collection('Visual Geometry',root)
        materials={}
        for batch in data['meshes']:
            vertices=batch['vertices']
            if not vertices: continue
            mesh=bpy.data.meshes.new(batch['name'] or 'Level mesh')
            mesh.from_pydata([xyz(v) for v in vertices],[],[(i,i+1,i+2) for i in range(0,len(vertices),3)])
            uv=mesh.uv_layers.new(name='UVMap')
            uv.data.foreach_set('uv',[value for v in vertices for value in (v[3],1-v[4])])
            color=mesh.color_attributes.new(name='JPB Color',type='FLOAT_COLOR',domain='CORNER')
            color.data.foreach_set('color',[value/255 for v in vertices for value in v[5:9]])
            light=mesh.color_attributes.new(name='JPB Preview Light',type='FLOAT_COLOR',domain='CORNER')
            light.data.foreach_set('color',[c for v in vertices for c in (max(v[5]/255,.1),max(v[6]/255,.1),max(v[7]/255,.1),v[8]/255)])
            obj=bpy.data.objects.new(batch['name'] or 'Level mesh',mesh);visuals.objects.link(obj)
            for key in ('texture','pass','mesh_index'): obj['jpb_'+key]=batch[key]
            if 'strip_indices' in batch:obj['jpb_strip_indices']=json.dumps(batch['strip_indices'])
            key=(batch['texture'],batch['pass'])
            if key not in materials:
                mat=bpy.data.materials.new(batch['texture'] or 'Untextured');mat.use_nodes=True
                nodes=mat.node_tree.nodes;shader=nodes.get('Principled BSDF');shader.inputs['Roughness'].default_value=1
                texture=Path(batch['texture'].replace('\\','/')).name
                candidates=[fbx.parent/texture,fbx.parent/(Path(texture).stem+'.tga')] if fbx else []
                # Retail _TryLoadTexture (VA 0x1401269A3) redirects the
                # arena FBX's textures to fed; game_runtime_load_texture
                # then falls back to LEVEL_3DS. Do not substitute JPX UVs.
                if resource_root and texture:
                    texture_level='fed' if data['level_name']=='arena' else data['level_name']
                    candidates.extend((resource_root/'level'/'jpx'/texture_level/(Path(texture).stem+'.tga'),
                                       resource_root/'level'/'3ds'/(Path(texture).stem+'.tga')))
                image_path=next((p for p in candidates if p.is_file()),None)
                if image_path:
                    image=nodes.new('ShaderNodeTexImage');image.image=bpy.data.images.load(str(image_path),check_existing=True)
                    # PSWorld is unlit texture * max(vertex RGB, 0.1).
                    light_node=nodes.new('ShaderNodeVertexColor');light_node.layer_name='JPB Preview Light'
                    multiply=nodes.new('ShaderNodeMixRGB');multiply.blend_type='MULTIPLY';multiply.inputs[0].default_value=1
                    mat.node_tree.links.new(image.outputs['Color'],multiply.inputs[1])
                    mat.node_tree.links.new(light_node.outputs['Color'],multiply.inputs[2])
                    shader.inputs['Base Color'].default_value=(0,0,0,1)
                    shader.inputs['Emission Strength'].default_value=1
                    mat.node_tree.links.new(multiply.outputs[0],shader.inputs['Emission Color'])
                    if batch['pass']:
                        mat.node_tree.links.new(image.outputs['Alpha'],shader.inputs['Alpha'])
                        mat.surface_render_method='DITHERED'
                elif texture: missing.add(texture)
                materials[key]=mat
            mesh.materials.append(materials[key]);mesh.update()
        collision=collection('Collision (enable in Outliner)',root)
        points=[];faces=[]
        for face in data['collision']:
            faces.append(tuple(range(len(points),len(points)+len(face['vertices']))))
            points.extend(xyz(v) for v in face['vertices'])
        mesh=bpy.data.meshes.new('JPB Collision');mesh.from_pydata(points,[],faces)
        obj=bpy.data.objects.new('JPB Collision',mesh);collision.objects.link(obj)
        obj.display_type='WIRE';obj.hide_render=True;obj.hide_set(True)
        for key in ('offset','camera','flags','trigger'):
            attr=mesh.attributes.new('jpb_'+key,'INT','FACE')
            attr.data.foreach_set('value',[(r[key]+2**31)%2**32-2**31 for r in data['collision']])
        placements=collection('Placements',root);waypoints=collection('Waypoints',root)
        for record in data['placements']:
            obj=marker(f"{record['id']:03d} {record['name']}",record['position'],record,placements)
            for key in ('id','actor_id','ai_id','enemy_id','active_flags'):obj['jpb_'+key]=record[key]
            import struct
            names=('activeFlags','startMode','movementMode','hitPoints','movementSpeed','mass','ftSpeed',
                   'fov','range','angle','aRange','daRange','daDelay','ownerType','latency','skillLevel','performanceLevel')
            defaults=dict(zip(names,struct.unpack_from('<17i',bytes.fromhex(record['defaults_hex']))))
            obj['jpb_defaults']=defaults
            actor=record['actor_id'];obj['jpb_actor']=data['actors'][actor] if 0<=actor<len(data['actors']) else ''
            for i,point in enumerate(record['waypoints']):
                wp=marker(f"Placement {record['id']:03d} waypoint {i}",point['position'],point,waypoints)
                wp['jpb_placement_id']=record['id'];wp['jpb_waypoint_index']=i
        waypoints.hide_viewport=True
        powerups=collection('Powerups',root)
        for i,record in enumerate(data['powerups']):marker(f"Powerup {i:03d} type {record['type']}",record['position'],record,powerups)
        if data['pickup_layouts']:
            powerups.name='Legacy J3D Powerups';powerups.hide_viewport=True
            for layout in data['pickup_layouts']:
                layer=collection(f"Powerups {layout['players']}P",root);layer.hide_viewport=layout['players']==2
                for i,record in enumerate(layout['records']):marker(f"Pickup {i:03d}",record['position'],record,layer)
        starts=collection('Start Points',root)
        if j3d:marker('Archive world start',data['world_start'],{'kind':'world_start'},starts)
        for i,p in enumerate(data['player_starts']):marker(f'Player {i+1} start',p,{'kind':'player_start','player':i},starts)
        import_gameplay(context,root,data,resource_root,scale)
        metadata={k:v for k,v in data.items() if k not in ('meshes','collision')}
        metadata['sources']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (fbx,j3d) if p}
        sidecars=[Path(layout['source']) for layout in data['pickup_layouts']]
        if Path(data['camera_source']).is_file():sidecars.append(Path(data['camera_source']))
        for path in sidecars:
            metadata['sources'][str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
            import base64
            saved=bpy.data.texts.new(f'JPB {path.name}.base64');saved.write(base64.b64encode(path.read_bytes()).decode('ascii'))
        metadata['script_node_columns']=['parent','child','sibling','opcode','value_uint32']
        metadata['missing_textures']=sorted(missing)
        text=bpy.data.texts.new(f'JPB {source.stem} gameplay.json');text.write(json.dumps(metadata,indent=2))
        root['jpb_metadata_text']=text.name
        # Keep the original archive intact, including chunks not yet interpreted.
        if j3d:
            import base64
            raw=bpy.data.texts.new(f'JPB {source.stem} original.j3d.base64')
            raw.write(base64.b64encode(j3d.read_bytes()).decode('ascii'));root['jpb_original_archive_text']=raw.name
        root['jpb_missing_textures']=len(missing)
        root['jpb_has_gameplay']=bool(j3d);root['jpb_has_visuals']=bool(fbx)
        return root
    except Exception:
        for store,existing in zip(stores,before):
            for block in set(store)-existing: store.remove(block,do_unlink=True)
        raise


class ImportJPBLevel(bpy.types.Operator,ImportHelper):
    bl_idname='import_scene.jpb_level'
    bl_label='Import JPB Level'
    bl_options={'REGISTER','UNDO'}
    filename_ext='.fbx'
    filter_glob:StringProperty(default='*.fbx;*.jpx;*.j3d',options={'HIDDEN'})
    scale:FloatProperty(name='Scale',default=0.01,min=0.00001,max=100,description='Matches BMD/CAD scale')
    gameplay_file:StringProperty(name='J3D override',subtype='FILE_PATH',description='Optional; paired stock gameplay file is found automatically')
    source_level:IntProperty(name='Source level index',default=-1,min=-1,max=25,description='-1 detects the stock level from its path; override for renamed assets')

    def execute(self,context):
        try:
            root=import_level_scene(context,self.filepath,self.scale,self.gameplay_file,self.source_level)
            warnings=[]
            if not root['jpb_has_visuals']:warnings.append('no paired FBX; gameplay only')
            if not root['jpb_has_gameplay']:warnings.append('no paired J3D; visuals only')
            if 'CAM unavailable' in root.get('jpb_camera_source',''):warnings.append('no CAM sidecar; legacy cameras only')
            if root['jpb_missing_textures']:warnings.append(f"{root['jpb_missing_textures']} missing textures; see gameplay JSON")
            self.report({'WARNING'} if warnings else {'INFO'},'Imported '+root.name+(' ('+'; '.join(warnings)+')' if warnings else ''))
            return {'FINISHED'}
        except Exception as exc:
            self.report({'ERROR'},str(exc));return {'CANCELLED'}
