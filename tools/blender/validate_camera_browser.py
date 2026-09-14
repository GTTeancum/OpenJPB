import bpy,sys,json
from pathlib import Path
sys.path.insert(0,sys.argv[sys.argv.index('--')+1])
import io_bmd_v3_6_1 as addon
addon.register()
out=Path('C:/Programming/GitHub/Jedi Power Battles recomp/out/level-import')
bpy.ops.wm.open_mainfile(filepath=str(out/'theed-full.blend'))
root=next(c for c in bpy.data.collections if c.get('jpb_camera_count')==256)
pickup=next(o for o in root.children['Powerups 1P'].objects if o.get('jpb_record'))
bpy.context.view_layer.objects.active=pickup
first=addon.browser_camera(bpy.context)
assert first is not None and first.type=='CAMERA'
cams=list(root.children['Camera Rigs'].objects)
chosen=cams[2];bpy.context.scene.jpb_camera_browser=chosen
assert addon.browser_camera(bpy.context)==chosen
assert bpy.ops.jpb.select_reference(object_name=chosen.name)=={'FINISHED'}
assert bpy.context.object==chosen and not chosen.hide_get()
assert not root.children['Camera Rigs'].hide_viewport
assert addon.JPBCameraProperties.poll(bpy.context)
class Layout:
    def __init__(self):self.labels=[]
    def label(self,**kw):self.labels.append(kw['text'])
    def box(self):return self
layout=Layout();addon.draw_camera_record(layout,chosen)
assert any('Flags:' in s for s in layout.labels)
assert any('Pitch:' in s for s in layout.labels)
assert sum(s.startswith(('X:','Y:','Z:')) for s in layout.labels)==9
record=json.loads(chosen['jpb_record'])
# Restore a useful map view while leaving the camera browser ready.
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':
        area.spaces.active.show_region_ui=True
    elif area.type=='PROPERTIES':area.spaces.active.context='DATA'
bpy.ops.wm.save_as_mainfile(filepath=str(out/'theed-camera-info.blend'))
bpy.ops.wm.open_mainfile(filepath=str(out/'theed-camera-info.blend'))
assert bpy.context.scene.jpb_camera_browser is not None
assert json.loads(bpy.context.scene.jpb_camera_browser['jpb_record'])==record
(out/'camera-browser-validation.json').write_text(json.dumps({'version':'4.2.1','selected_id':record['id'],'labels':layout.labels,'save_reopen':'PASS'},indent=2))
print('CAMERA_BROWSER_PASS',flush=True)
