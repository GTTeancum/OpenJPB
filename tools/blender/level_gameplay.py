"""Authored level camera, script and animation import. No scripts execute here."""
import bpy
import json
import math
import struct
from pathlib import Path
from mathutils import Matrix, Euler, Vector
from bpy.props import StringProperty, PointerProperty

# Names verified against the switch in reconstructed/original/enemy.c.
COMMAND_NAMES={0x100:'Scan',0x101:'Compare counter',0x400:'Change AI mode',
               0x401:'Return AI mode',0x405:'Set counter',0x40c:'Change health',
               0x40f:'Reset AI timer',0x410:'Set waypoint'}


class JPBScriptTree(bpy.types.NodeTree):
    bl_idname='JPBScriptTree';bl_label='JPB Script';bl_icon='NODETREE'


class JPBScriptNode(bpy.types.Node):
    bl_idname='JPBScriptNode';bl_label='Game command'
    def init(self,context):
        self.inputs.new('NodeSocketFloat','Parent')
        self.inputs.new('NodeSocketFloat','Previous')
        for socket in self.inputs:socket.link_limit=4095
        self.outputs.new('NodeSocketFloat','Child')
        self.outputs.new('NodeSocketFloat','Sibling')
    def draw_buttons(self,context,layout):
        layout.label(text=self.get('description',''))
        layout.label(text='Opcode: '+self.get('opcode_hex',''))
        layout.label(text='Value: '+str(self.get('value',0)))
        if self.get('parameters'):layout.label(text='Variables: '+self['parameters'])
    @classmethod
    def poll(cls,tree):return tree.bl_idname=='JPBScriptTree'


class JPBOpenScript(bpy.types.Operator):
    bl_idname='jpb.open_script';bl_label='Open Script Graph'
    tree_name:StringProperty()
    def execute(self,context):
        tree=bpy.data.node_groups.get(self.tree_name)
        if tree is None:return {'CANCELLED'}
        context.area.type='NODE_EDITOR';context.area.ui_type='JPBScriptTree'
        context.space_data.pin=True;context.space_data.node_tree=tree
        return {'FINISHED'}


class JPBSelectReference(bpy.types.Operator):
    bl_idname='jpb.select_reference';bl_label='Select Referenced Object'
    object_name:StringProperty()
    def execute(self,context):
        obj=bpy.data.objects.get(self.object_name)
        if obj is None:return {'CANCELLED'}
        for collection in obj.users_collection:collection.hide_viewport=False
        obj.hide_set(False)
        for selected in context.selected_objects:selected.select_set(False)
        obj.select_set(True);context.view_layer.objects.active=obj
        return {'FINISHED'}


def jpb_camera_poll(self,obj):
    return obj.type=='CAMERA' and obj.get('jpb_kind')=='Camera dolly'


def browser_camera(context):
    chosen=context.scene.jpb_camera_browser
    if chosen and jpb_camera_poll(None,chosen):return chosen
    selected=context.object
    if selected and selected.get('jpb_kind')=='Camera dolly':return selected
    if selected and selected.get('jpb_camera_object'):
        return bpy.data.objects.get(selected['jpb_camera_object'])
    used={o.get('jpb_camera_object') for o in context.scene.objects if o.get('jpb_camera_object')}
    cameras=[o for o in context.scene.objects if jpb_camera_poll(None,o)]
    return next((o for o in cameras if o.name in used),cameras[0] if cameras else None)


def draw_camera_record(layout,obj):
    record=json.loads(obj['jpb_record'])
    layout.label(text=f"{obj.name} — ID {record['id']}",icon='CAMERA_DATA')
    layout.label(text=f"Flags: 0x{record['flags']:08X}")
    layout.label(text=f"Pitch: {record['pitch']}  ({record['pitch']*360/4096:.2f} deg)")
    layout.label(text=f"Yaw: {record['yaw']}  ({record['yaw']*360/4096:.2f} deg)")
    for key,title in (('offset','Authored position / offset'),('slack','Tracking slack'),('off','Tracking adjustment')):
        box=layout.box();box.label(text=title+' (game units)')
        for axis,value in zip('XYZ',record[key]):box.label(text=f'{axis}: {value}')
    layout.label(text='Authored data; follow-camera motion is runtime controlled.')


class JPBCameraBrowser(bpy.types.Panel):
    bl_label='JPB Cameras';bl_idname='VIEW3D_PT_jpb_cameras'
    bl_space_type='VIEW_3D';bl_region_type='UI';bl_category='JPB';bl_order=-10
    def draw(self,context):
        layout=self.layout
        layout.prop(context.scene,'jpb_camera_browser',text='Browse')
        obj=browser_camera(context)
        if obj is None:layout.label(text='No imported JPB cameras');return
        draw_camera_record(layout,obj)
        button=layout.operator('jpb.select_reference',text='Show and Select Camera',icon='CAMERA_DATA')
        button.object_name=obj.name
        regions=[o for o in context.scene.objects if o.get('jpb_camera_object')==obj.name]
        layout.label(text=f'Regions: {len(regions)}')
        for region in regions:
            button=layout.operator('jpb.select_reference',text='Show '+region.name,icon='MESH_GRID')
            button.object_name=region.name


class JPBCameraProperties(bpy.types.Panel):
    bl_label='JPB Camera Data';bl_idname='DATA_PT_jpb_camera'
    bl_space_type='PROPERTIES';bl_region_type='WINDOW';bl_context='data';bl_order=-10
    @classmethod
    def poll(cls,context):return context.object is not None and jpb_camera_poll(None,context.object)
    def draw(self,context):draw_camera_record(self.layout,context.object)


GAMEPLAY_CLASSES=(JPBScriptTree,JPBScriptNode,JPBOpenScript,JPBSelectReference,JPBCameraBrowser,JPBCameraProperties)


def register_camera_browser():
    bpy.types.Scene.jpb_camera_browser=PointerProperty(name='JPB Camera',type=bpy.types.Object,poll=jpb_camera_poll)


def unregister_camera_browser():
    del bpy.types.Scene.jpb_camera_browser


def load_level_sidecars(data,resource_root):
    data['pickup_layouts']=[]
    if resource_root is None:return
    for players in (1,2):
        path=resource_root/'level/powerups'/(data['level_name']+('2' if players==2 else '')+'.pwr')
        if not path.is_file():continue
        raw=path.read_bytes()
        if len(raw)%12:raise ValueError(f'Invalid pickup layout size: {path}')
        records=[]
        for offset in range(0,len(raw),12):
            reserved,x,y,z,kind=struct.unpack_from('<IhhhH',raw,offset)
            records.append(dict(position=[x,y,z],type=kind&32767,raw_type=kind,reserved=reserved,source=str(path),players=players))
        data['pickup_layouts'].append(dict(players=players,source=str(path),records=records))


def archive_inventory(path):
    raw=path.read_bytes();offset=0;records=[]
    while offset<len(raw):
        if offset+16>len(raw):raise ValueError('Truncated J3D chunk header')
        name,count,size=struct.unpack_from('<8sII',raw,offset)
        if offset+16+size>len(raw):raise ValueError('Truncated J3D chunk payload')
        records.append(dict(name=name.decode('ascii'),offset=offset,count=count,payload_bytes=size))
        offset+=16+size
    return records


def import_gameplay(context,root,data,resource_root,scale):
    def xyz(v):return (-v[0]*scale,v[2]*scale,v[1]*scale)
    def group(name,hidden=True):
        c=bpy.data.collections.new(name);root.children.link(c);c.hide_viewport=hidden;return c
    def record_object(name,record,collection,position=(0,0,0)):
        obj=bpy.data.objects.new(name,None);collection.objects.link(obj)
        obj.location=xyz(position);obj.hide_render=True
        obj['jpb_record']=json.dumps(record);obj['jpb_label']=name
        for key,value in record.items():
            if isinstance(value,(str,int,float)):obj['jpb_'+key]=value
        return obj
    def line(name,positions,collection):
        curve=bpy.data.curves.new(name,'CURVE');curve.dimensions='3D';curve.bevel_depth=scale*3
        spline=curve.splines.new('POLY');spline.points.add(len(positions)-1)
        for p,co in zip(spline.points,positions):p.co=(*co,1)
        obj=bpy.data.objects.new(name,curve);collection.objects.link(obj);obj.hide_render=True
        obj.show_in_front=True;obj.color=(1,.65,.1,1);return obj

    # loader_LevelLoad overrides the 32 legacy records with the full 8192-byte CAM.
    camera_path=resource_root/'level/CAMERAS'/f"{data['level_name']}.cam" if resource_root else None
    if camera_path and camera_path.is_file():
        raw=camera_path.read_bytes()
        if len(raw)!=8192:raise ValueError(f'Unexpected CAM size: {camera_path}')
        cameras=[]
        for i in range(256):
            v=struct.unpack_from('<I6h3i2h',raw,i*32)
            cameras.append(dict(id=i,flags=v[0],pitch=v[1],yaw=v[3],slack=[v[5],v[2],v[10]],
                                off=[v[6],v[4],v[11]],offset=list(v[7:10])))
        data['legacy_cameras']=data['cameras'];data['cameras']=cameras
        data['camera_source']=str(camera_path)
    else:data['camera_source']='J3D legacy camera records (CAM unavailable)'

    cameras_group=group('Camera Rigs');regions_group=group('Camera Regions')
    camera_faces={}
    for face in data['collision']:
        if face['camera']>=0:camera_faces.setdefault(face['camera'],[]).append(face)
    for record in data['cameras']:
        identifier=record['id']
        cam_data=bpy.data.cameras.new(f'Dolly {identifier:03d}')
        cam=bpy.data.objects.new(cam_data.name,cam_data);cameras_group.objects.link(cam)
        cam.hide_render=True;cam.location=xyz(record['offset'])
        cam['jpb_record']=json.dumps(record);cam['jpb_kind']='Camera dolly'
        cam['jpb_id']=identifier;cam['jpb_label']=f'Dolly {identifier:03d}'
        cam['jpb_preview_note']='Authored offset/orientation; game tracking and focus are evaluated at runtime'
        cam_data.display_size=scale*100;cam_data.clip_end=100000
        # This is the authored orientation, not a simulated player-follow pose.
        axes=Matrix(((-1,0,0),(0,0,1),(0,1,0)))
        rotation=Euler((record['pitch']*math.tau/4096,record['yaw']*math.tau/4096,0),'XYZ').to_matrix()
        cam.rotation_euler=(axes@rotation).to_euler()
        cam.hide_set(identifier not in camera_faces and not record['flags'])
        records=camera_faces.get(identifier,[])
        if records:
            points=[];faces=[]
            for r in records:
                faces.append(tuple(range(len(points),len(points)+len(r['vertices']))));points.extend(xyz(v) for v in r['vertices'])
            mesh=bpy.data.meshes.new(f'Camera region {identifier:03d}');mesh.from_pydata(points,[],faces)
            region=bpy.data.objects.new(mesh.name,mesh);regions_group.objects.link(region)
            region.hide_render=True;region.display_type='WIRE';region.show_in_front=True
            region['jpb_camera_id']=identifier;region['jpb_label']=mesh.name;region['jpb_record']=json.dumps(record)
            region['jpb_camera_object']=cam.name
    root['jpb_camera_count']=len(data['cameras'])
    root['jpb_camera_source']=data['camera_source']

    # Every encoded node and every edge are retained, including control nodes.
    scripts={}
    for script in data['scripts']:
        tree=bpy.data.node_groups.new(f"JPB {root.name} AI {script['id']:03d}",'JPBScriptTree')
        tree.use_fake_user=True;tree['jpb_script_id']=script['id'];tree['jpb_variables']=json.dumps(script['variables'])
        tree['jpb_record']=json.dumps(script);nodes=[]
        for i,record in enumerate(script['nodes']):
            parent,child,sibling,encoded,value=record;encoded&=65535
            opcode=encoded&4095 if encoded&0x4000 else encoded
            n=tree.nodes.new('JPBScriptNode');n.name=f'Node {i:03d}'
            n.label=f'{i}: '+COMMAND_NAMES.get(opcode,f'Command {opcode:04X}')
            n['opcode_hex']=f'0x{encoded:04X}';n['value']=str(value);n['jpb_record']=json.dumps(record)
            n['description']='Inline operand' if encoded&0x4000 else 'Variable-table offset'
            if not encoded&0x4000 and value<len(script['variables']):n['parameters']=', '.join(str(v) for v in script['variables'][value:value+3])
            depth=0;ancestor=parent;seen={i}
            while 0<=ancestor<len(script['nodes']) and ancestor not in seen:
                seen.add(ancestor);depth+=1;ancestor=script['nodes'][ancestor][0]
            n.location=(depth*300,-i*160);n.width=270;nodes.append(n)
        for i,record in enumerate(script['nodes']):
            for edge,index in (('Child',record[1]),('Sibling',record[2])):
                if 0<=index<len(nodes):tree.links.new(nodes[i].outputs[edge],nodes[index].inputs['Parent' if edge=='Child' else 'Previous'],verify_limits=False)
        scripts[script['id']]=tree
    placements={}
    for obj in root.all_objects:
        if obj.get('jpb_kind')=='Placement':
            placements[obj['jpb_id']]=obj
            tree=scripts.get(obj['jpb_ai_id'])
            if tree:obj['jpb_script_tree']=tree.name
    relations=group('Placement Links');routes=group('Waypoint Routes')
    triggers=group('Trigger Regions');trigger_faces={}
    for face in data['collision']:
        if face.get('trigger',255)!=255:trigger_faces.setdefault(face['trigger'],[]).append(face)
    for identifier,records in trigger_faces.items():
        target=data['animation_map'][identifier] if identifier<len(data['animation_map']) else -1
        points=[];faces=[]
        for record in records:
            faces.append(tuple(range(len(points),len(points)+len(record['vertices']))));points.extend(xyz(v) for v in record['vertices'])
        mesh=bpy.data.meshes.new(f'Trigger {identifier:03d} -> placement {target}')
        mesh.from_pydata(points,[],faces);obj=bpy.data.objects.new(mesh.name,mesh);triggers.objects.link(obj)
        obj.hide_render=True;obj.display_type='WIRE';obj.show_in_front=True;obj.color=(1,.1,.15,1)
        obj['jpb_kind']='Map trigger';obj['jpb_label']=mesh.name
        obj['jpb_record']=json.dumps(dict(trigger_id=identifier,target_placement=target))
        if target in placements:
            obj['jpb_target_object']=placements[target].name
            if placements[target].get('jpb_script_tree'):obj['jpb_script_tree']=placements[target]['jpb_script_tree']
    root['jpb_trigger_region_count']=len(trigger_faces)
    for record in data['placements']:
        owner=placements.get(record['id'])
        if not owner:continue
        for target in record['links']:
            if target in placements:
                obj=line(f"Placement {record['id']} -> {target}",[owner.location,placements[target].location],relations)
                obj['jpb_source_id']=record['id'];obj['jpb_target_id']=target
        if len(record['waypoints'])>1:
            obj=line(f"Route {record['id']}",[xyz(p['position']) for p in record['waypoints']],routes)
            obj['jpb_placement_id']=record['id']

    animations=group('Level Animation Tracks')
    axes=Matrix(((-1,0,0),(0,0,1),(0,1,0)))
    for animation in data.get('animations',[]):
        for node in animation['nodes']:
            obj=record_object(f"Animation {animation['id']} node {node['id']}",node,animations)
            obj.empty_display_type='ARROWS';obj.empty_display_size=scale*100
            obj['jpb_animation_id']=animation['id'];obj['jpb_animation_header']=json.dumps({k:v for k,v in animation.items() if k!='nodes'})
            for key in node['keys']:
                obj.location=xyz(key['position'])
                rotation=Euler(tuple(v*math.tau/4096 for v in key['rotation']),'XYZ').to_matrix()
                obj.rotation_euler=(axes@rotation@axes).to_euler()
                obj.scale=tuple(struct.unpack('<f',struct.pack('<I',key['scale'][i]&0xffffffff))[0] for i in (0,2,1))
                for prop in ('location','rotation_euler','scale'):obj.keyframe_insert(prop,frame=key['frame']/4096)
            if obj.animation_data and obj.animation_data.action:
                obj.animation_data.action.use_fake_user=True
                obj.animation_data.action['jpb_raw_keys']=json.dumps(node['keys'])
                action=obj.animation_data.action
                # Native interpolation is linear. Preserve event flags on the keys.
                for layer in action.layers:
                    for strip in layer.strips:
                        bag=strip.channelbag(obj.animation_data.action_slot)
                        if bag:
                            for curve in bag.fcurves:
                                for point in curve.keyframe_points:point.interpolation='LINEAR'
    root['jpb_script_count']=len(scripts);root['jpb_animation_count']=len(data.get('animations',[]))
    context.scene.frame_set(context.scene.frame_current)
