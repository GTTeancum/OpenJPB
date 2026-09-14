"""Installed add-on smoke test and application-native level overview render."""
import bpy
import sys
import json
from pathlib import Path
from mathutils import Vector

addon_dir,game_dir,output_dir=sys.argv[sys.argv.index('--')+1:]
sys.path.insert(0,addon_dir)
import io_bmd_v3_6_1 as addon
addon.register()
assert addon.bl_info['version']>=(4,1,0)
output=Path(output_dir)
root=addon.import_level_scene(bpy.context,str(Path(game_dir)/'res/level/jpx/theed/theed.fbx'))
assert not root['jpb_missing_textures']
visuals=root.children['Visual Geometry']
points=[v.co for obj in visuals.objects for v in obj.data.vertices]
low=Vector(tuple(min(v[i] for v in points) for i in range(3)))
high=Vector(tuple(max(v[i] for v in points) for i in range(3)))
center=(low+high)/2
camera=bpy.data.cameras.new('Overview');obj=bpy.data.objects.new('Overview',camera)
bpy.context.scene.collection.objects.link(obj)
obj.location=(center.x,center.y,high.z+max(high-low)*2)
camera.type='ORTHO';camera.ortho_scale=max(high.x-low.x,high.y-low.y)*1.1
camera.clip_end=100000
scene=bpy.context.scene;scene.camera=obj
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=1
scene.render.resolution_x=900;scene.render.resolution_y=900;scene.render.resolution_percentage=100
scene.view_settings.view_transform='Standard'
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(output/'theed-overview.png')
# Confirm the preview graph is the installed, updated material implementation.
assert all(any(n.bl_idname=='ShaderNodeVertexColor' for n in mat.node_tree.nodes)
           for obj in visuals.objects for mat in obj.data.materials)
bpy.ops.wm.save_as_mainfile(filepath=str(output/'theed-import.blend'))
bpy.ops.render.render(write_still=True)
print('INSTALLED_LEVEL_IMPORT_PASS',flush=True)
