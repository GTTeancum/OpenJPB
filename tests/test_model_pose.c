#include "jpb/model.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                               \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int test_authored_pose_tree(void)
{
    _animFrame frame;
    Mnode root;
    Mnode children[2];

    memset(&frame, 0, sizeof(frame));
    memset(&root, 0, sizeof(root));
    memset(children, 0, sizeof(children));
    frame.av3JointAngle[3].vx = 101;
    frame.av3JointAngle[3].vy = 202;
    frame.av3JointAngle[3].vz = 303;
    frame.av3JointAngle[3].pad = 0x7fff;
    frame.event[0] = 2;
    frame.event[3] = 1;
    frame.event[5] = 8;
    root.id = NODE_STATIC;
    root.flags = 0x10u;
    root.v3CurrentRotation.vx = 9;
    root.v3CurrentRotation.vy = 8;
    root.v3CurrentRotation.vz = 7;
    root.numChildNodes = 2;
    root.aChildNode = children;
    children[0].id = (modelNodeId)3;
    children[0].flags = 0x24u;
    children[0].v3CurrentRotation.pad = 0x1234;
    children[1].id =
        (modelNodeId)(NODE_VIRTUAL | 5);
    children[1].flags = 0x40000000u;
    children[1].v3CurrentRotation.vx = 77;
    children[1].v3CurrentRotation.pad = 0x2345;

    CHECK(jpb_ModelApplyAnimFrame(
              &root, &frame) == JPB_MODEL_POSE_OK);
    CHECK(root.v3CurrentRotation.vx == 0);
    CHECK(root.v3CurrentRotation.vy == 0);
    CHECK(root.v3CurrentRotation.vz == 0);
    CHECK(root.flags == 0x02u);
    CHECK(children[0].v3CurrentRotation.vx == 101);
    CHECK(children[0].v3CurrentRotation.vy == 202);
    CHECK(children[0].v3CurrentRotation.vz == 303);
    CHECK(children[0].v3CurrentRotation.pad == 0x1234);
    CHECK(children[0].flags == 0x05u);
    CHECK(children[1].v3CurrentRotation.vx == 0);
    CHECK(children[1].v3CurrentRotation.pad == 0x2345);
    CHECK(children[1].flags == 0x40000009u);
    return 0;
}

static int test_absolute_rotation_override(void)
{
    _animFrame frame;
    Mnode node;

    memset(&frame, 0, sizeof(frame));
    memset(&node, 0, sizeof(node));
    frame.av3JointAngle[2].vx = 11;
    node.id = (modelNodeId)2;
    node.v3RotationAbs.vx = 401;
    node.v3RotationAbs.vy = 402;
    node.v3RotationAbs.vz = 403;
    node.v3RotationAbs.pad = 0x6fff;
    node.v3CurrentRotation.pad = 0x3456;
    node.flags =
        JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY;

    CHECK(jpb_ModelApplyAnimFrame(
              &node, &frame) == JPB_MODEL_POSE_OK);
    CHECK(node.v3CurrentRotation.vx == 401);
    CHECK(node.v3CurrentRotation.vy == 402);
    CHECK(node.v3CurrentRotation.vz == 403);
    CHECK(node.v3CurrentRotation.pad == 0x3456);
    CHECK((node.flags &
           JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) == 0);

    node.v3RotationAbs.vx = 501;
    node.flags = 0x20000000u;
    CHECK(jpb_ModelApplyAnimFrame(
              &node, &frame) == JPB_MODEL_POSE_OK);
    CHECK(node.v3CurrentRotation.vx == 501);
    CHECK(node.flags == 0x20000000u);
    return 0;
}

static int test_hot_node_publishes_scene_attack(void)
{
    _animFrame frame;
    Mnode root;
    Mnode child;
    objectRoot scene_root;

    memset(&frame, 0, sizeof(frame));
    memset(&root, 0, sizeof(root));
    memset(&child, 0, sizeof(child));
    memset(&scene_root, 0, sizeof(scene_root));
    root.id = NODE_STATIC;
    root.numChildNodes = 1;
    root.aChildNode = &child;
    child.id = (modelNodeId)4;

    frame.event[4] = 1;
    CHECK(jpb_ModelApplyAnimFrameForScene(
              &root,
              &frame,
              &scene_root) == JPB_MODEL_POSE_OK);
    CHECK((child.flags & JPB_COLLISION_FLAG_HOT) != 0);
    CHECK((scene_root.flags & UINT32_C(0x10)) != 0);

    scene_root.flags = UINT32_C(0x20);
    frame.event[4] = 2;
    CHECK(jpb_ModelApplyAnimFrameForScene(
              &root,
              &frame,
              &scene_root) == JPB_MODEL_POSE_OK);
    CHECK((child.flags & JPB_COLLISION_FLAG_HOT) == 0);
    CHECK(scene_root.flags == UINT32_C(0x20));
    return 0;
}

static int test_model_event_and_effect_publication(void)
{
    _animFrame frame;
    modelObject model;
    Mnode root;
    Mnode children[2];
    objectRoot scene_root;

    memset(&frame, 0, sizeof(frame));
    memset(&model, 0, sizeof(model));
    memset(&root, 0, sizeof(root));
    memset(children, 0, sizeof(children));
    memset(&scene_root, 0, sizeof(scene_root));
    root.id = (modelNodeId)0;
    root.numChildNodes = 2;
    root.aChildNode = children;
    children[0].id = (modelNodeId)4;
    children[1].id = (modelNodeId)17;
    model.pRootNode = &root;
    model.eventMask = UINT32_MAX;
    model.effectMask = UINT32_MAX;
    frame.event[0] = 2;
    frame.event[4] = 8;
    frame.event[17] = 10;

    CHECK(jpb_ModelPublishAnimFrame(
              &model,
              &frame,
              &scene_root) == JPB_MODEL_POSE_OK);
    CHECK(model.eventMask ==
          (UINT32_C(1) | (UINT32_C(1) << 17)));
    CHECK(model.effectMask ==
          ((UINT32_C(1) << 4) | (UINT32_C(1) << 17)));

    memset(frame.event, 0, sizeof(frame.event));
    CHECK(jpb_ModelPublishAnimFrame(
              &model,
              &frame,
              &scene_root) == JPB_MODEL_POSE_OK);
    CHECK(model.eventMask == 0);
    CHECK(model.effectMask == 0);
    CHECK(jpb_ModelPublishAnimFrame(
              NULL,
              &frame,
              &scene_root) ==
          JPB_MODEL_POSE_INVALID_ARGUMENT);
    return 0;
}

static int test_pose_bounds(void)
{
    _animFrame frame;
    Mnode node;

    memset(&frame, 0, sizeof(frame));
    memset(&node, 0, sizeof(node));
    node.id =
        (modelNodeId)JPB_ANIM_JOINT_CAPACITY;
    CHECK(jpb_ModelApplyAnimFrame(
              &node, &frame) ==
          JPB_MODEL_POSE_UNSUPPORTED_NODE_ID);

    node.id = NODE_STATIC;
    node.numChildNodes = 1;
    node.aChildNode = NULL;
    CHECK(jpb_ModelApplyAnimFrame(
              &node, &frame) ==
          JPB_MODEL_POSE_INVALID_ARGUMENT);
    CHECK(jpb_ModelApplyAnimFrame(
              NULL, &frame) ==
          JPB_MODEL_POSE_INVALID_ARGUMENT);
    return 0;
}

int main(void)
{
    CHECK(test_authored_pose_tree() == 0);
    CHECK(test_absolute_rotation_override() == 0);
    CHECK(test_hot_node_publishes_scene_attack() == 0);
    CHECK(test_model_event_and_effect_publication() == 0);
    CHECK(test_pose_bounds() == 0);
    puts("model pose tests passed");
    return 0;
}
