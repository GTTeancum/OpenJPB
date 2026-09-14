"""Background Blender integration checks for the installed level importer."""
import bpy
import sys
import json
from pathlib import Path

addon_dir,game_dir,output_dir=sys.argv[sys.argv.index('--')+1:]
sys.path.insert(0,addon_dir)
import io_bmd_v3_6_1 as addon

game=Path(game_dir);output=Path(output_dir);output.mkdir(parents=True,exist_ok=True)
addon.register()
assert hasattr(bpy.types,'IMPORT_SCENE_OT_bmd') or hasattr(addon,'ImportBMD')
results=[]
for name in ('fed','theed','palace'):
    print('Importing',name,flush=True)
    path=game/'res/level/jpx'/name/(name+'.fbx')
    root=addon.import_level_scene(bpy.context,str(path))
    data=json.loads(bpy.data.texts[root['jpb_metadata_text']].as_string())
    assert root['jpb_has_gameplay'] and root['jpb_has_visuals']
    assert root['jpb_missing_textures']==0,(name,data['missing_textures'])
    visuals=root.children['Visual Geometry']
    assert len(visuals.objects)>0 and data['placements']
    collision=root.children['Collision (enable in Outliner)'].objects[0]
    assert len(collision.data.polygons)>0 and collision.hide_get()
    first=data['placements'][0]['position']
    obj=root.children['Placements'].objects[0]
    assert (obj.location-__import__('mathutils').Vector((-first[0]*.01,first[2]*.01,first[1]*.01))).length<.0001
    for mesh_obj in visuals.objects:
        mesh=mesh_obj.data
        assert len(mesh.loops)==len(mesh.uv_layers.active.data)==len(mesh.color_attributes['JPB Color'].data)
        assert len(mesh.materials)==1
    results.append({'level':name,'visual_batches':len(visuals.objects),'collision_faces':len(collision.data.polygons),
                    'placements':len(data['placements']),'scripts':len(data['scripts']),'missing_textures':0})
    print('Validated',name,flush=True)
    if name=='fed':
        # Native import values must survive saving and reopening without decoding.
        bpy.ops.wm.save_as_mainfile(filepath=str(output/'fed-import.blend'))
        bpy.ops.wm.open_mainfile(filepath=str(output/'fed-import.blend'))
        assert any(c.get('jpb_level_index')==1 for c in bpy.data.collections)
    # Keep tests isolated without unregistering the add-on.
    for block in list(bpy.data.objects):bpy.data.objects.remove(block,do_unlink=True)
    for block in list(bpy.data.collections):bpy.data.collections.remove(block)

# Missing source must leave the scene untouched.
counts=(len(bpy.data.objects),len(bpy.data.collections))
try:addon.import_level_scene(bpy.context,str(output/'missing.fbx'))
except ValueError:pass
else:raise AssertionError('Missing source accepted')
assert counts==(len(bpy.data.objects),len(bpy.data.collections))
# Gameplay-only and geometry-only entry points are both supported.
root=addon.import_level_scene(bpy.context,str(game/'res/level/W3D/fed.j3d'))
assert root['jpb_has_visuals'] and root['jpb_level_index']==1
addon.unregister();addon.register();addon.unregister()
(output/'blender-validation.json').write_text(json.dumps(results,indent=2))
print('JPB_LEVEL_VALIDATION_PASS',json.dumps(results))
