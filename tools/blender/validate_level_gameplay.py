"""Full gameplay import and .blend persistence regression in background Blender."""
import bpy,sys,json,struct,os
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,sys.argv[sys.argv.index('--')+1])
import io_bmd_v3_6_1 as addon
addon.register()
out=Path('C:/Programming/GitHub/Jedi Power Battles recomp/out/level-import')
source=Path(os.environ.get('JPB_LEVEL_TEST_SOURCE','C:/Games/Star Wars Jedi Power Battles/res/level/jpx/theed/theed.fbx'))
root=addon.import_level_scene(bpy.context,str(source));name=root.name
data=json.loads(bpy.data.texts[root['jpb_metadata_text']].as_string())
assert root['jpb_camera_count']==256
camera_bytes=Path(data['camera_source']).read_bytes()
for cam in data['cameras']:
    assert cam['flags']==struct.unpack_from('<I',camera_bytes,cam['id']*32)[0]
    assert cam['offset']==list(struct.unpack_from('<3i',camera_bytes,cam['id']*32+16))
trees=[t for t in bpy.data.node_groups if t.bl_idname=='JPBScriptTree']
assert len(trees)==len(data['scripts'])
for tree in trees:
    script=data['scripts'][tree['jpb_script_id']]
    assert len(tree.nodes)==len(script['nodes'])
    expected=sum(0<=target<len(script['nodes']) for n in script['nodes'] for target in n[1:3])
    assert len(tree.links)==expected,(tree.name,len(tree.links),expected)
    assert json.loads(tree['jpb_variables'])==script['variables']
assert len([o for o in root.children['Powerups 1P'].objects if o.get('jpb_record')])==43
assert len([o for o in root.children['Powerups 2P'].objects if o.get('jpb_record')])==69
assert len(root.children['Level Animation Tracks'].objects)==sum(len(a['nodes']) for a in data['animations'])
assert data['archive_inventory'][-1]['name']=='JONCHUNK'
area=next(a for a in bpy.context.screen.areas if a.type=='VIEW_3D')
with bpy.context.temp_override(area=area):
    assert bpy.ops.jpb.open_script(tree_name=trees[0].name)=={'FINISHED'}
    assert area.spaces.active.node_tree==trees[0]
area.type='VIEW_3D'
for obj in list(bpy.data.objects):
    if obj.name in ('Cube','Camera','Light'):bpy.data.objects.remove(obj,do_unlink=True)
start=next(o for o in root.children['Start Points'].objects if o.get('jpb_record') and json.loads(o['jpb_record']).get('player')==0)
for obj in bpy.context.selected_objects:obj.select_set(False)
start.select_set(True);bpy.context.view_layer.objects.active=start
space=area.spaces.active;space.region_3d.view_location=start.location+Vector((-8,15,0))
space.region_3d.view_distance=70;space.region_3d.view_rotation=Vector((-25,40,-40)).to_track_quat('-Z','Y')
space.shading.type='MATERIAL';space.clip_end=100000;space.show_region_ui=True
destination=out/('theed-full-jpx.blend' if source.suffix=='.jpx' else 'theed-full.blend')
bpy.ops.wm.save_as_mainfile(filepath=str(destination))
bpy.ops.wm.open_mainfile(filepath=str(destination))
root=bpy.data.collections[name]
assert root['jpb_camera_count']==256 and root['jpb_script_count']==86
assert len([t for t in bpy.data.node_groups if t.bl_idname=='JPBScriptTree'])==86
result={'cameras':256,'scripts':86,'script_nodes':sum(len(s['nodes']) for s in data['scripts']),
        'pickups_1p':43,'pickups_2p':69,'animation_definitions':len(data['animations']),
        'camera_regions':len(root.children['Camera Regions'].objects),'save_reopen':'PASS'}
(out/('full-import-validation-jpx.json' if source.suffix=='.jpx' else 'full-import-validation.json')).write_text(json.dumps(result,indent=2))
print('FULL_IMPORT_PASS',result,flush=True)
