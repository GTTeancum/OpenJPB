import bpy,sys,json
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,sys.argv[sys.argv.index('--')+1])
import io_bmd_v3_6_1 as addon
addon.register()
out=Path('C:/Programming/GitHub/Jedi Power Battles recomp/out/level-import')
root=addon.import_level_scene(bpy.context,'C:/Games/Star Wars Jedi Power Battles/res/level/jpx/theed/theed.fbx')
actors=list(root.children['Placements'].objects)
pickups=list(root.children['Powerups'].objects)
assert all(o.get('jpb_model') for o in actors),[(o.name,o.get('jpb_model')) for o in actors if not o.get('jpb_model')]
assert all(o.get('jpb_model') for o in pickups)
assert len({o.data.as_pointer() for o in actors})<len(actors)
assert all(o.type=='MESH' and len(o.data.polygons)>0 for o in actors+pickups)
for o in list(bpy.data.objects):
    if o.name in ('Cube','Camera','Light'):bpy.data.objects.remove(o,do_unlink=True)
start=next(o for o in root.children['Start Points'].objects if o.get('jpb_record') and json.loads(o['jpb_record']).get('player')==0)
target=start.location+Vector((-8,15,0))
camera=bpy.data.cameras.new('Review');obj=bpy.data.objects.new('Review',camera);bpy.context.scene.collection.objects.link(obj)
obj.location=target+Vector((25,-40,40));obj.rotation_euler=(target-obj.location).to_track_quat('-Z','Y').to_euler()
camera.type='ORTHO';camera.ortho_scale=75;camera.clip_end=100000
scene=bpy.context.scene;scene.camera=obj;scene.render.engine='CYCLES';scene.cycles.samples=8
scene.render.resolution_x=1100;scene.render.resolution_y=800;scene.render.resolution_percentage=100
scene.view_settings.view_transform='Standard';scene.world.color=(.6,.6,.6)
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':
        area.spaces.active.region_3d.view_distance=70
        area.spaces.active.region_3d.view_location=target
        area.spaces.active.region_3d.view_rotation=obj.rotation_euler.to_quaternion()
        area.spaces.active.shading.type='MATERIAL'
        area.spaces.active.clip_end=100000
scene.render.filepath=str(out/'theed-models.png')
bpy.ops.wm.save_as_mainfile(filepath=str(out/'theed-models.blend'))
bpy.ops.render.render(write_still=True)
(out/'models-validation.json').write_text(json.dumps({'actors':len(actors),'pickups':len(pickups),'unique_actor_meshes':len({o.data.as_pointer() for o in actors})}))
print('MODEL_PREVIEWS_PASS',flush=True)
