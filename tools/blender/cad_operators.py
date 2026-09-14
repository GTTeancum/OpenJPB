"""Blender bridge for the authoritative CAD codec; loaded by the BMD add-on."""
import base64
import json
import math
import uuid
from pathlib import Path
import bpy
from bpy.props import BoolProperty, IntProperty, StringProperty
from bpy_extras.io_utils import ImportHelper, ExportHelper
from mathutils import Matrix, Euler, Vector
from jpb_cad import Cad, Huffman, wrap

AXES = Matrix(((-1,0,0),(0,0,1),(0,1,0)))
ANGLE = 2 * math.pi / 4096


def rotation(vector):
    return AXES @ Euler(tuple(x*ANGLE for x in vector[:3]), 'XYZ').to_matrix() @ AXES


def mapped_bones(arm):
    if 'bmd_bone_names' not in arm or 'bmd_bone_node_ids' not in arm:
        raise ValueError('Import the BMD rig with this add-on first')
    names = json.loads(arm['bmd_bone_names'])
    ids = json.loads(arm['bmd_bone_node_ids'])
    return [(arm.pose.bones[name], node & 4095) for name,node in zip(names[1:],ids[1:])
            if name in arm.pose.bones and not node & 0xa000]


def curve(action, path, index, values, start, interpolation='LINEAR'):
    fc = action.fcurves.new(path, index=index)
    fc.keyframe_points.add(len(values))
    for i,value in enumerate(values):
        fc.keyframe_points[i].co = (start+i, value)
        fc.keyframe_points[i].interpolation = interpolation
    fc.update()


class ImportCAD(bpy.types.Operator, ImportHelper):
    """Import CAD clips as independent Actions; select the BMD armature first"""
    bl_idname = 'import_anim.cad'
    bl_label = 'Import CAD Animation'
    bl_options = {'REGISTER','UNDO'}
    filename_ext = '.cad'
    filter_glob: StringProperty(default='*.cad', options={'HIDDEN'})
    import_all: BoolProperty(name='Import All Clips', default=True)
    clip_index: IntProperty(name='Clip Index', default=0, min=0)
    frame_start: IntProperty(name='Start Frame', default=1)

    def execute(self, ctx):
        arm = ctx.active_object
        if not arm or arm.type != 'ARMATURE':
            self.report({'ERROR'}, 'Select an imported BMD armature')
            return {'CANCELLED'}
        try:
            cad = Cad.load(self.filepath)
            huffman = Huffman(Path(self.filepath).parent)
            mapping = mapped_bones(arm)
            indices = range(cad.count) if self.import_all else [self.clip_index]
            # Decode before modifying Blender; report failures instead of silently skipping them.
            clips = [(i,cad.decode(i,huffman)) for i in indices]
            arm['cad_archive'] = base64.b64encode(cad.data).decode('ascii')
            arm['cad_tables_directory'] = str(Path(self.filepath).parent)
            arm['cad_source'] = str(Path(self.filepath).resolve())
            arm['cad_rig_id'] = str(uuid.uuid4())
            arm.animation_data_create()
            arm.animation_data.use_nla = False
            scale = arm.get('bmd_scale', 0.01)
            actions = []
            for index, frames in clips:
                if not frames:
                    continue
                action = bpy.data.actions.new(cad.name(index))
                action.use_fake_user = True
                action['cad_index'] = index
                action['cad_source'] = arm['cad_source']
                action['cad_rig_id'] = arm['cad_rig_id']
                action['cad_start'] = self.frame_start
                action['cad_frames'] = len(frames)
                action['cad_events'] = json.dumps([[v[3] for v in f] for f in frames])
                for channel in range(3):
                    values = [(AXES @ Vector(f[0][:3]))[channel]*scale for f in frames]
                    curve(action, 'location', channel, values, self.frame_start)
                slots = {pb.name:slot for pb,slot in mapping}
                for pb in arm.pose.bones:
                    slot = slots.get(pb.name, -1)
                    rest = pb.bone.matrix_local.to_3x3()
                    quats = []
                    for frame in frames:
                        # Pose matrices include the bone's rest basis on the right.
                        local = rotation(frame[slot+1]) if slot >= 0 and slot+1 < len(frame) else Matrix.Identity(3)
                        q = (rest.inverted() @ local @ rest).to_quaternion()
                        if quats and q.dot(quats[-1]) < 0:
                            q.negate()
                        quats.append(q)
                    pb.rotation_mode = 'QUATERNION'
                    for channel in range(4):
                        curve(action, pb.path_from_id('rotation_quaternion'), channel,
                              [q[channel] for q in quats], self.frame_start)
                actions.append(action)
            if not actions:
                raise ValueError('CAD has no nonempty clips')
            arm.animation_data.action = actions[0]
            # The engine's authored frame duration is 1/30 s (gGlobalFrameRate / 4096).
            ctx.scene.render.fps = 30
            ctx.scene.render.fps_base = 1.0
            ctx.scene.frame_start = self.frame_start
            ctx.scene.frame_end = self.frame_start + actions[0]['cad_frames'] - 1
            ctx.scene.frame_set(self.frame_start)
            self.report({'INFO'}, f'Imported {len(actions)} independent clips; choose clips in the Action Editor')
            return {'FINISHED'}
        except Exception as exc:
            self.report({'ERROR'}, str(exc))
            return {'CANCELLED'}


def sample_action(ctx, arm, action, original):
    mapping = mapped_bones(arm)
    scale = arm.get('bmd_scale', 0.01)
    events = json.loads(action['cad_events'])
    if len(events) != len(original):
        raise ValueError('Event data length must match the original clip')
    start = action.get('cad_start',1)
    if tuple(action.frame_range) != (float(start),float(start+len(original)-1)):
        raise ValueError('Keep the imported clip frame range; changing duration requires gameplay timing edits')
    if any(len(e)!=len(f) for e,f in zip(events,original)):
        raise ValueError('Event vector count must match the original clip')
    arm.animation_data.action = action
    frames = []
    for fi, source in enumerate(original):
        ctx.scene.frame_set(action.get('cad_start',1) + fi)
        evaluated = arm.evaluated_get(ctx.evaluated_depsgraph_get())
        if any(abs(x-1)>0.00001 for x in evaluated.matrix_basis.to_scale()) or evaluated.matrix_basis.to_quaternion().angle>0.00001:
            raise ValueError('CAD cannot store armature object scale or rotation; edit the pose bones')
        frame = [list(v) for v in source]
        root = AXES @ (evaluated.location / scale)
        frame[0][:3] = [round(root.x),round(root.y/2)*2,round(root.z)]
        for pb,slot in mapping:
            if slot+1 >= len(frame):
                continue
            posed = evaluated.pose.bones[pb.name]
            if posed.location.length>0.00001 or any(abs(x-1)>0.00001 for x in posed.scale):
                raise ValueError(f'CAD cannot store per-bone translation or scale ({pb.name})')
            world = posed.matrix.to_3x3() @ pb.bone.matrix_local.to_3x3().inverted()
            if pb.parent:
                parent = evaluated.pose.bones[pb.parent.name]
                parent_world = parent.matrix.to_3x3() @ pb.parent.bone.matrix_local.to_3x3().inverted()
                world = parent_world.inverted() @ world
            old = rotation(source[slot+1])
            if max(abs(world[r][c]-old[r][c]) for r in range(3) for c in range(3)) > 0.00002:
                angles = (AXES @ world @ AXES).to_euler('XYZ')
                frame[slot+1][:3] = [wrap(round(x/ANGLE/2)*2) for x in angles]
        for vector,event in zip(frame,events[fi]):
            if not 0 <= event <= 65535:
                raise ValueError('CAD event flags must fit an unsigned 16-bit value')
            vector[3] = event
        frames.append(frame)
    return frames


class ExportCAD(bpy.types.Operator, ExportHelper):
    """Export edited clips into the source CAD, preserving other clips and gameplay records"""
    bl_idname = 'export_anim.cad'
    bl_label = 'Export CAD Animation'
    filename_ext = '.cad'
    filter_glob: StringProperty(default='*.cad', options={'HIDDEN'})
    export_all: BoolProperty(name='Export All Imported Clips', default=True)

    def execute(self, ctx):
        arm = ctx.active_object
        if not arm or arm.type != 'ARMATURE' or 'cad_archive' not in arm:
            self.report({'ERROR'}, 'Select the armature containing imported CAD clips')
            return {'CANCELLED'}
        ad = arm.animation_data
        if ad is None:
            self.report({'ERROR'}, 'The armature has no animation data')
            return {'CANCELLED'}
        previous, frame, nla = ad.action, ctx.scene.frame_current, ad.use_nla
        try:
            cad = Cad(base64.b64decode(arm['cad_archive']))
            huffman = Huffman(arm['cad_tables_directory'])
            actions = ([a for a in bpy.data.actions if a.get('cad_rig_id') == arm['cad_rig_id']]
                       if self.export_all else [ad.action])
            replacements = {}
            ad.use_nla = False
            seen = set()
            for action in actions:
                if not action or 'cad_index' not in action:
                    raise ValueError('Select an imported CAD action')
                index = action['cad_index']
                if index in seen:
                    raise ValueError('Multiple actions target the same CAD clip; export the active action separately')
                seen.add(index)
                original = cad.decode(index,huffman)
                frames = sample_action(ctx,arm,action,original)
                if frames != original:
                    replacements[index] = frames
            output = cad.export(replacements,huffman) if replacements else cad.data
            # Validate encoded poses and event flags before touching the destination.
            check = Cad(output)
            for index,frames in replacements.items():
                if check.decode(index,huffman) != frames:
                    raise ValueError(f'CAD verification failed for clip {index}')
            Path(self.filepath).write_bytes(output)
            self.report({'INFO'}, f'Exported CAD: {len(replacements)} edited clips; gameplay records preserved')
            return {'FINISHED'}
        except Exception as exc:
            self.report({'ERROR'}, str(exc))
            return {'CANCELLED'}
        finally:
            ad.action, ad.use_nla = previous, nla
            ctx.scene.frame_set(frame)
