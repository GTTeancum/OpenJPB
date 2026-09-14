"""Run with Blender --background --factory-startup --python-exit-code 1 --python.

Arguments after --: game res directory, output directory.
"""
import json
import math
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import os
if os.environ.get("JPB_BLENDER_ADDON_DIR"):
    sys.path.insert(0,os.environ["JPB_BLENDER_ADDON_DIR"])
import bpy
from mathutils import Matrix, Vector
import io_bmd_v3_6_1 as addon
from jpb_cad import Cad, Huffman
from cad_operators import rotation, AXES

addon.register()
args = sys.argv[sys.argv.index('--')+1:]
res, out = map(Path,args)
out.mkdir(parents=True,exist_ok=True)
results = []
for name in ('adi','obi_wan','mace','plo','aayla','kitfisto','shaakti'):
    model, animation = res/'MODEL'/f'{name}.bmd', res/'animation'/f'{name}.cad'
    if not model.exists() or not animation.exists():
        continue
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)
    assert bpy.ops.import_mesh.bmd(filepath=str(model)) == {'FINISHED'}
    arm = bpy.context.active_object
    mesh = next(c for c in arm.children if c.type=='MESH')
    assert bpy.ops.import_anim.cad(filepath=str(animation)) == {'FINISHED'}
    assert len(arm.animation_data.nla_tracks)==0
    cad, huffman = Cad.load(animation), Huffman(animation.parent)
    bones, count = addon.parse_bones(model.read_bytes())
    order = addon.dfs_order(bones,1)
    error = 0.0
    checked = 0
    for action in bpy.data.actions:
        arm.animation_data.action = action
        for fi,frame in enumerate(cad.decode(action['cad_index'],huffman)):
            bpy.context.scene.frame_set(fi+1)
            evaluated = arm.evaluated_get(bpy.context.evaluated_depsgraph_get())
            world = {}
            for bi in order:
                bone = bones[bi]
                local = Matrix.Translation(AXES @ Vector((bone['lx'],bone['ly'],bone['lz']))*0.01)
                slot = bone['cad_slot']+1
                if not bone['node_id'] & 0xa000 and slot < len(frame):
                    local = local @ rotation(frame[slot]).to_4x4()
                world[bi] = world[bone['parent']] @ local if bone['parent'] in world else local
                pb = evaluated.pose.bones[bone['name']]
                expected = world[bi] @ pb.bone.matrix_local.to_3x3().to_4x4()
                diff=max(abs(expected[r][c]-pb.matrix[r][c]) for r in range(4) for c in range(4))
                if diff>0.0001:
                    raise AssertionError((name,action.name,fi,bi,bone['name'],diff,list(pb.rotation_quaternion),[list(r) for r in expected],[list(r) for r in pb.matrix]))
                error=max(error,diff)
            checked += 1
    assert error < 0.0001,(name,error)
    assert bpy.ops.export_anim.cad(filepath=str(out/f'{name}.cad'))=={'FINISHED'}
    assert (out/f'{name}.cad').read_bytes()==animation.read_bytes()
    assert bpy.ops.export_mesh.bmd(filepath=str(out/f'{name}.bmd'))=={'FINISHED'}
    assert (out/f'{name}.bmd').read_bytes()==model.read_bytes()
    # Verify an actual pose edit survives CAD encoding and native decoding later.
    action = next(a for a in bpy.data.actions if a['cad_frames']>2)
    arm.animation_data.action=action
    pb = next(pb for pb in arm.pose.bones if pb.name!='Root')
    bpy.context.scene.frame_set(2)
    pb.rotation_quaternion = (pb.rotation_quaternion.to_matrix() @ Matrix.Rotation(0.08,3,'X')).to_quaternion()
    pb.keyframe_insert('rotation_quaternion',frame=2)
    assert bpy.ops.export_anim.cad(filepath=str(out/f'{name}-edited.cad'),export_all=False)=={'FINISHED'}
    edited = Cad.load(out/f'{name}-edited.cad')
    assert edited.data != cad.data
    assert edited.motions == cad.motions
    # One representable vertex edit; exporter must build geometry, not copy source.
    mesh.data.vertices[0].co.x += 0.01
    assert bpy.ops.export_mesh.bmd(filepath=str(out/f'{name}-edited.bmd'))=={'FINISHED'}
    assert (out/f'{name}-edited.bmd').read_bytes()!=model.read_bytes()
    rebuilt=(out/f'{name}-edited.bmd').read_bytes()
    new_bones,new_count=addon.parse_bones(rebuilt)
    decoded_mesh=addon.do_import(rebuilt,new_bones,new_count,0.01)
    def canonical(corners):
        return min(tuple(corners[i:]+corners[:i]) for i in range(len(corners)))
    def corner(position,uv,color):
        return (tuple(round(x,5) for x in position),tuple(round(x,5) for x in uv),
                tuple(round(x*255) for x in color))
    expected=[]
    for poly in mesh.data.polygons:
        corners=[]
        for li in poly.loop_indices:
            uv=mesh.data.uv_layers[0].data[li].uv
            corners.append(corner(mesh.data.vertices[mesh.data.loops[li].vertex_index].co,
                                  (uv.x,1-uv.y),mesh.data.color_attributes['Col'].data[li].color))
        expected.append(canonical(corners))
    actual=[canonical([corner(decoded_mesh['verts'][vi],uv,col) for vi,uv,col in
                       zip(face,decoded_mesh['face_uvs'][i],decoded_mesh['face_colors'][i])])
            for i,face in enumerate(decoded_mesh['faces'])]
    assert sorted(actual)==sorted(expected),(name,'edited BMD geometry/UV/color mismatch',len(actual),len(expected))
    record = {'model':name,'frames_checked':checked,'max_pose_matrix_error':error,
              'unedited_byte_roundtrip':True,'edited_exports':True}
    results.append(record)
    print('VALIDATED',record,flush=True)
(out/'blender-results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
print('BLENDER_VALIDATION_PASS',flush=True)
