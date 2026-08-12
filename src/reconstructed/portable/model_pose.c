/*
 * Portable publication of authored animation angles into the model-node
 * hierarchy.
 *
 * This is a descriptive jpb_ extraction, not an original PDB procedure. The
 * state updates come from the front of render_RenderNode in
 * W:\SWJediPowerBattles\Work\win32\nodes.c, checked against assembly RVAs
 * 0x129A0B..0x129B12 and 0x129D7B..0x129DD8. The original renderer applies
 * these stores before building node matrices, rendering geometry, and
 * recursing into children.
 */

#include "jpb/model.h"

enum {
    JPB_MODEL_NODE_FLAG_ABSOLUTE_ROTATION = 0x20000000u,
    JPB_MODEL_NODE_FLAG_ALWAYS_HOT = 0x40000000u,
    JPB_MODEL_NODE_EVENT_MASK = 0x0000000bu,
    JPB_MODEL_NODE_EVENT_PRESERVE_MASK = 0xffffff04u
};

static void model_copy_rotation_xyz(
    _svector *destination, const _svector *source)
{
    destination->vx = source->vx;
    destination->vy = source->vy;
    destination->vz = source->vz;
}

static JPBModelPoseResult model_apply_node_pose(
    Mnode *node,
    const _animFrame *frame,
    objectRoot *scene_root,
    modelObject *model)
{
    uint32_t node_id;
    uint32_t joint_index;
    int child_index;

    if (node == NULL || frame == NULL) {
        return JPB_MODEL_POSE_INVALID_ARGUMENT;
    }
    node_id = (uint32_t)node->id;
    joint_index = node_id & NODE_INDEX_MASK;
    if (joint_index >= JPB_ANIM_EVENT_BYTES) {
        return JPB_MODEL_POSE_UNSUPPORTED_NODE_ID;
    }
    if ((node_id & (NODE_STATIC | NODE_VIRTUAL)) == 0) {
        model_copy_rotation_xyz(
            &node->v3CurrentRotation,
            &frame->av3JointAngle[joint_index]);
    } else {
        node->v3CurrentRotation.vx = 0;
        node->v3CurrentRotation.vy = 0;
        node->v3CurrentRotation.vz = 0;
    }

    if ((node->flags &
         ((uint32_t)JPB_MODEL_NODE_FLAG_ABSOLUTE_ROTATION |
          (uint32_t)JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY)) != 0) {
        model_copy_rotation_xyz(
            &node->v3CurrentRotation,
            &node->v3RotationAbs);
        node->flags &=
            ~JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY;
    }

    /*
     * Exact render_RenderNode event publication at matched-PC
     * RVAs 0x129D7B..0x129DB0. Authored CAD event bytes drive the hot-node
     * collision state; bit 30 makes a node permanently hot.
     */
    node->flags =
        ((uint32_t)(int32_t)(int8_t)frame->event[joint_index] &
         JPB_MODEL_NODE_EVENT_MASK) |
        (node->flags & JPB_MODEL_NODE_EVENT_PRESERVE_MASK);
    if ((node->flags & JPB_MODEL_NODE_FLAG_ALWAYS_HOT) != 0 &&
        (node->flags & JPB_COLLISION_FLAG_HOT) == 0) {
        node->flags |= JPB_COLLISION_FLAG_HOT;
    }
    /*
     * Exact render_RenderNode scene publication at matched-PC
     * RVAs 0x129DBD..0x129DD8. A hot authored node marks the current scene
     * object for the player_DoCollisions pass. That owner clears the bit
     * after consuming it, so this is intentionally republished per pose.
     */
    if (scene_root != NULL &&
        (node->flags & JPB_COLLISION_FLAG_HOT) != 0) {
        scene_root->flags |= UINT32_C(0x10);
    }
    if (model != NULL) {
        uint32_t node_mask =
            UINT32_C(1) << (joint_index & 31U);

        if ((node->flags & UINT32_C(0x2)) != 0) {
            model->eventMask |= node_mask;
        }
        if ((node->flags & UINT32_C(0x8)) != 0) {
            model->effectMask |= node_mask;
        }
    }

    if (node->numChildNodes < 0 ||
        (node->numChildNodes != 0 &&
         node->aChildNode == NULL)) {
        return JPB_MODEL_POSE_INVALID_ARGUMENT;
    }
    for (child_index = 0;
         child_index < node->numChildNodes;
         ++child_index) {
        JPBModelPoseResult result =
            model_apply_node_pose(
                &node->aChildNode[child_index],
                frame,
                scene_root,
                model);

        if (result != JPB_MODEL_POSE_OK) {
            return result;
        }
    }
    return JPB_MODEL_POSE_OK;
}

JPBModelPoseResult jpb_ModelApplyAnimFrame(
    Mnode *root, const _animFrame *frame)
{
    return model_apply_node_pose(root, frame, NULL, NULL);
}

JPBModelPoseResult jpb_ModelApplyAnimFrameForScene(
    Mnode *root,
    const _animFrame *frame,
    objectRoot *scene_root)
{
    return model_apply_node_pose(
        root, frame, scene_root, NULL);
}

JPBModelPoseResult jpb_ModelPublishAnimFrame(
    modelObject *model,
    const _animFrame *frame,
    objectRoot *scene_root)
{
    if (model == NULL || model->pRootNode == NULL ||
        frame == NULL) {
        return JPB_MODEL_POSE_INVALID_ARGUMENT;
    }

    /*
     * Exact render_RenderModel stores at matched-PC RVAs
     * 0x12960F..0x129620, immediately before render_RenderNode rebuilds
     * these masks from the current CAD frame's per-node event bytes.
     */
    model->eventMask = 0;
    model->effectMask = 0;
    return model_apply_node_pose(
        model->pRootNode,
        frame,
        scene_root,
        model);
}
