#include "jpb/collision.h"
#include "jpb/anim.h"
#include "jpb/boss.h"
#include "jpb/bullet.h"
#include "jpb/combo.h"
#include "jpb/effects.h"
#include "jpb/game.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/settings.h"
#include "jpb/world.h"

#include <limits.h>
#include <stdint.h>
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

#define DEFINE_PROFILE_CALLBACK(index)                                      \
    static int profile_callback_##index(                                    \
        int32_t *controls, playerObject *player)                             \
    {                                                                        \
        (void)controls;                                                       \
        (void)player;                                                         \
        return index;                                                         \
    }

DEFINE_PROFILE_CALLBACK(33)
DEFINE_PROFILE_CALLBACK(34)
DEFINE_PROFILE_CALLBACK(35)
DEFINE_PROFILE_CALLBACK(36)
DEFINE_PROFILE_CALLBACK(37)
DEFINE_PROFILE_CALLBACK(38)
DEFINE_PROFILE_CALLBACK(39)
DEFINE_PROFILE_CALLBACK(40)
DEFINE_PROFILE_CALLBACK(41)
DEFINE_PROFILE_CALLBACK(42)
DEFINE_PROFILE_CALLBACK(43)
DEFINE_PROFILE_CALLBACK(44)
DEFINE_PROFILE_CALLBACK(45)
DEFINE_PROFILE_CALLBACK(46)
DEFINE_PROFILE_CALLBACK(47)

#undef DEFINE_PROFILE_CALLBACK

static JPBPlayerCallback profile_callbacks[] = {
    profile_callback_33, profile_callback_34,
    profile_callback_35, profile_callback_36,
    profile_callback_37, profile_callback_38,
    profile_callback_39, profile_callback_40,
    profile_callback_41, profile_callback_42,
    profile_callback_43, profile_callback_44,
    profile_callback_45, profile_callback_46,
    profile_callback_47
};

static uint32_t hash_bytes(
    const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t hash = UINT32_C(2166136261);
    size_t index;

    for (index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= UINT32_C(16777619);
    }
    return hash;
}

static int test_4d_collision(void)
{
    VECTOR p0 = {15, 15, 15, 0};
    VECTOR p1 = {20, 20, 20, 0};
    _svector v0 = {1, 2, 3, 0};
    _svector v1 = {4, 5, 6, 0};

    CHECK(coll_4DCollision(&p0, &v0, &p1, &v1, 4) == 1);
    p0.vy = 100;
    CHECK(coll_4DCollision(&p0, &v0, &p1, &v1, 4) == 0);
    return 0;
}

static int test_registry_and_accessors(void)
{
    Mnode root;
    Mnode node;
    Mnode replacement;
    _svector rotation;
    _svector output;
    VECTOR translation;

    memset(&root, 0, sizeof(root));
    memset(&node, 0, sizeof(node));
    memset(&replacement, 0, sizeof(replacement));
    root.id = NODE_DYNAMIC;
    root.v3RotCenter.vx = 100;
    node.id = (modelNodeId)(NODE_DYNAMIC | 5);
    node.v3RotCenter.vx = 500;
    node.v3CurrentRotation.vx = 11;
    node.v3Velocity.vz = 33;
    node.flags =
        JPB_COLLISION_FLAG_HOT |
        JPB_COLLISION_FLAG_EVENT |
        JPB_COLLISION_FLAG_SABRE;

    coll_ResetCollisionSystem();
    coll_gRegisterNode(2, &root);
    coll_gRegisterNode(2, &node);
    CHECK(coll_GetNode(2, NODE_DYNAMIC | 5) == &node);
    CHECK(coll_GetNodeCenter(2, 5) == &node.v3RotCenter);
    CHECK(coll_GetNodeCenter(2, 6) == &root.v3RotCenter);
    CHECK(coll_GetNodeCenter(3, 0) == NULL);
    CHECK(coll_GetNodeRotation(2, 5) == &node.v3CurrentRotation);
    CHECK(coll_GetNodeRotationAbs(2, 5) == &node.v3RotationAbs);
    CHECK(coll_GetNodeTranslation(2, 5) == &node.v3Translation);
    CHECK(coll_GetNodeVelocity(2, 5) == &node.v3Velocity);
    CHECK(coll_CheckForHotNode(2, 5) == JPB_COLLISION_FLAG_HOT);
    CHECK(coll_CheckForEventNode(2, 5) == JPB_COLLISION_FLAG_EVENT);
    CHECK(coll_CheckForSabreNode(2, 5) == JPB_COLLISION_FLAG_SABRE);
    CHECK(coll_ChkNodeFlags(
              2,
              5,
              JPB_COLLISION_FLAG_EVENT |
                  JPB_COLLISION_FLAG_SABRE) ==
          (JPB_COLLISION_FLAG_EVENT |
           JPB_COLLISION_FLAG_SABRE));
    coll_ClrNodeFlags(2, 5, JPB_COLLISION_FLAG_EVENT);
    CHECK(coll_CheckForEventNode(2, 5) == 0);
    coll_SetNodeFlags(2, 5, JPB_COLLISION_FLAG_EVENT);
    CHECK(coll_CheckForEventNode(2, 5) == JPB_COLLISION_FLAG_EVENT);

    rotation.vx = 101;
    rotation.vy = -202;
    rotation.vz = 303;
    rotation.pad = 404;
    node.v3RotationAbs.pad = 505;
    coll_SetNodeRotationAbs(2, 5, &rotation);
    CHECK(node.v3RotationAbs.vx == 101);
    CHECK(node.v3RotationAbs.vy == -202);
    CHECK(node.v3RotationAbs.vz == 303);
    CHECK(node.v3RotationAbs.pad == 505);
    CHECK((node.flags &
           JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) != 0);

    node.v3RotationDelta.pad = 606;
    coll_SetNodeRotationDelta(2, 5, &rotation);
    CHECK(node.v3RotationDelta.vx == 101);
    CHECK(node.v3RotationDelta.vy == -202);
    CHECK(node.v3RotationDelta.vz == 303);
    CHECK(node.v3RotationDelta.pad == 606);
    CHECK((node.flags &
           JPB_COLLISION_FLAG_ROTATION_DELTA_DIRTY) != 0);
    memset(&output, 0x4b, sizeof(output));
    CHECK(coll_GetNodeRotationDelta(2, 5, &output) == &output);
    CHECK(output.vx == 101);
    CHECK(output.vy == -202);
    CHECK(output.vz == 303);
    CHECK(output.pad == INT16_C(0x4b4b));

    node.v3RotationAbs.vx = INT16_MAX;
    node.v3RotationAbs.vy = INT16_MIN;
    node.v3RotationAbs.vz = -1;
    rotation.vx = 1;
    rotation.vy = -1;
    rotation.vz = 2;
    coll_IncNodeRotationAbs(2, 5, &rotation);
    CHECK(node.v3RotationAbs.vx == INT16_MIN);
    CHECK(node.v3RotationAbs.vy == INT16_MAX);
    CHECK(node.v3RotationAbs.vz == 1);

    node.v3RotationDelta.vx = INT16_MAX;
    node.v3RotationDelta.vy = INT16_MIN;
    node.v3RotationDelta.vz = -1;
    coll_IncNodeRotationDelta(2, 5, &rotation);
    CHECK(node.v3RotationDelta.vx == INT16_MIN);
    CHECK(node.v3RotationDelta.vy == INT16_MAX);
    CHECK(node.v3RotationDelta.vz == 1);

    translation.vx = 0x12345;
    translation.vy = -65537;
    translation.vz = 0x18000;
    coll_SetNodeTranslation(2, 5, &translation);
    CHECK(node.v3Translation.vx == INT16_C(0x2345));
    CHECK(node.v3Translation.vy == -1);
    CHECK(node.v3Translation.vz == INT16_MIN);
    coll_SetNodeZBufferOffset(2, 5, 0x1ffff);
    CHECK(node.ZBufferOffset == -1);
    old_coll_ZeroNodeTranslation(2, 5);
    CHECK(node.v3Translation.vx == 0);
    CHECK(node.v3Translation.vy == -1);
    CHECK(node.v3Translation.vz == 0);

    replacement.id = (modelNodeId)(NODE_VIRTUAL | 5);
    coll_gRegisterNode(2, &replacement);
    CHECK(coll_GetNode(2, 5) == &replacement);

    coll_ResetPlayerCollision(-1);
    CHECK(coll_GetNode(2, 5) == &replacement);
    coll_ResetPlayerCollision(2);
    CHECK(coll_GetNode(2, 0) == NULL);
    CHECK(coll_GetNode(2, 5) == NULL);
    return 0;
}

static int test_hot_node_player_collision(void)
{
    CollisionData attacker_collision = {64, 1, -1};
    CollisionData target_collision = {64, 2, -1};
    playerObject attacker;
    playerObject target;
    Mnode attacker_node;
    Mnode target_node;
    Motion *authored_motion = (Motion *)(uintptr_t)0x1234;

    memset(&attacker, 0, sizeof(attacker));
    memset(&target, 0, sizeof(target));
    memset(&attacker_node, 0, sizeof(attacker_node));
    memset(&target_node, 0, sizeof(target_node));
    attacker.playernum = 2;
    attacker.fScale = 4096;
    attacker.paNodesSizes = &attacker_collision;
    attacker.numCollisionNodes = 1;
    attacker.pMotion = &authored_motion;
    target.playernum = 3;
    target.fScale = 4096;
    target.paNodesSizes = &target_collision;
    target.numCollisionNodes = 1;
    attacker_node.id = (modelNodeId)(NODE_DYNAMIC | 1);
    attacker_node.flags = JPB_COLLISION_FLAG_HOT;
    attacker_node.v3RotCenter.vx = 100;
    attacker_node.v3Velocity.vx = 10;
    target_node.id = (modelNodeId)(NODE_DYNAMIC | 2);
    target_node.v3RotCenter.vx = 110;

    coll_ResetCollisionSystem();
    coll_gRegisterNode(attacker.playernum, &attacker_node);
    coll_gRegisterNode(target.playernum, &target_node);
    totalframes = 100;
    CHECK(coll_gCheckHotNodes(&attacker, &target) == 1);
    CHECK((attacker.pFlags & 0x00010000u) != 0);
    CHECK((target_node.flags & 0x00020000u) != 0);
    CHECK(target.hitLocation.vx == 105);
    CHECK(target.hitLocation.vy == 0);
    CHECK(target.hitLocation.vz == 0);
    CHECK(target.hitVelocity.vx == 4096);
    CHECK(target.hitVelocity.vy == 0);
    CHECK(target.hitVelocity.vz == 0);
    CHECK(target.hitVelocity.speed == 10);
    CHECK(target.hitMotion == authored_motion);

    target.hitDelay = 101;
    attacker.pFlags |= 0x00010000u;
    CHECK(coll_gCheckHotNodes(&attacker, &target) == 0);
    CHECK((attacker.pFlags & 0x00010000u) == 0);
    return 0;
}

static int test_hot_node_signed_parent_lookup(void)
{
    CollisionData attacker_collision = {64, 0, -2};
    CollisionData target_collision = {64, 1, -1};
    playerObject attacker;
    playerObject target;
    Mnode attacker_node;
    Mnode parent_node;
    Mnode target_node;
    Motion *authored_motion = (Motion *)(uintptr_t)0x5678;

    memset(&attacker, 0, sizeof(attacker));
    memset(&target, 0, sizeof(target));
    memset(&attacker_node, 0, sizeof(attacker_node));
    memset(&parent_node, 0, sizeof(parent_node));
    memset(&target_node, 0, sizeof(target_node));
    attacker.playernum = 2;
    attacker.fScale = 4096;
    attacker.paNodesSizes = &attacker_collision;
    attacker.numCollisionNodes = 1;
    attacker.pMotion = &authored_motion;
    target.playernum = 3;
    target.fScale = 4096;
    target.paNodesSizes = &target_collision;
    target.numCollisionNodes = 1;
    attacker_node.id = NODE_DYNAMIC;
    attacker_node.v3RotCenter.vx = 100;
    parent_node.id = (modelNodeId)(NODE_DYNAMIC | 30);
    parent_node.flags = JPB_COLLISION_FLAG_HOT;
    target_node.id = (modelNodeId)(NODE_DYNAMIC | 1);
    target_node.v3RotCenter.vx = 100;

    coll_ResetCollisionSystem();
    coll_gRegisterNode(2, &attacker_node);
    coll_gRegisterNode(1, &parent_node);
    coll_gRegisterNode(3, &target_node);
    totalframes = 100;
    CHECK(coll_gCheckHotNodes(&attacker, &target) == 1);
    CHECK((attacker.pFlags & UINT32_C(0x00010000)) != 0);
    CHECK(target.hitMotion == authored_motion);
    return 0;
}

static int test_projectile_collision(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    CollisionData collisions[2] = {
        {10, 3, -1},
        {10, 4, -1}
    };
    playerObject owner;
    playerObject target;
    sceneObject owner_scene;
    Mnode node0;
    Mnode node1;
    Projectile proj;
    Motion hit_motion;

    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&owner, 0, sizeof(owner));
    memset(&target, 0, sizeof(target));
    memset(&owner_scene, 0, sizeof(owner_scene));
    memset(&node0, 0, sizeof(node0));
    memset(&node1, 0, sizeof(node1));
    memset(&proj, 0, sizeof(proj));
    memset(&hit_motion, 0, sizeof(hit_motion));
    GameStruct.CurrentLevel = 0;
    types[2].radius = 10;
    /* Flag 4 disables the independent reflection branch. */
    types[2].flag = UINT16_C(4);
    owner.playerRoot.pParent = &owner_scene.sceneRoot;
    owner_scene.pPlayer = &owner.playerRoot;
    owner.playernum = 6;
    owner.playerID = 20;
    target.playernum = 7;
    target.playerID = 20;
    target.paNodesSizes = collisions;
    target.numCollisionNodes = 2;
    node0.id = (modelNodeId)(NODE_DYNAMIC | 3);
    node0.v3RotCenter.vx = 100;
    node0.v3RotCenter.vy = 200;
    node0.v3RotCenter.vz = 300;
    node1.id = (modelNodeId)(NODE_DYNAMIC | 4);
    node1.v3RotCenter.vx = 500;
    node1.v3RotCenter.vy = 600;
    node1.v3RotCenter.vz = 700;
    proj.pj_Type = 2;
    proj.pj_Owner = (int32_t *)(void *)&owner;
    proj.pj_Target = (int32_t *)(void *)&target;
    proj.pj_User = (int32_t *)(void *)&hit_motion;
    proj.pj_Start.vx = 106;
    proj.pj_Start.vy = 194;
    proj.pj_Start.vz = 304;

    coll_ResetCollisionSystem();
    coll_gRegisterNode(target.playernum, &node0);
    coll_gRegisterNode(target.playernum, &node1);
    CHECK(coll_CheckProjectileCollision(&proj) == 1);
    CHECK(target.target == &owner);
    CHECK(target.hitLocation.vx == 103);
    CHECK(target.hitLocation.vy == 197);
    CHECK(target.hitLocation.vz == 302);
    CHECK(target.hitMotion == &hit_motion);
    CHECK((node0.flags & UINT32_C(0x20000)) != 0);

    node0.flags = 0;
    target.hitMotion = NULL;
    proj.pj_Start.vx = 111;
    CHECK(coll_CheckProjectileCollision(&proj) == 0);
    CHECK(target.hitMotion == NULL);
    proj.pj_Flags = UINT32_C(0x20);
    CHECK(coll_CheckProjectileCollision(&proj) == 1);

    node0.flags = 0;
    target.pFlags = UINT32_C(0x400);
    CHECK(coll_CheckProjectileCollision(&proj) == 0);
    CHECK(node0.flags == 0);
    target.pFlags = 0;

    /* Level 8 forces radius 0x100 and examines only collision entry zero. */
    GameStruct.CurrentLevel = UINT8_C(8);
    proj.pj_Flags = 0;
    proj.pj_Start.vx = 500;
    proj.pj_Start.vy = 600;
    proj.pj_Start.vz = 700;
    CHECK(coll_CheckProjectileCollision(&proj) == 0);
    proj.pj_Start.vx = 300;
    proj.pj_Start.vy = 200;
    proj.pj_Start.vz = 300;
    CHECK(coll_CheckProjectileCollision(&proj) == 1);

    /* The original owner/target pairing grants the starfighter 4x radius. */
    GameStruct.CurrentLevel = 0;
    node0.flags = 0;
    owner.playernum = 0;
    owner.playerID = 0x12;
    target.playerID = 0x2f;
    target.numCollisionNodes = 1;
    proj.pj_Start.vx = 130;
    proj.pj_Start.vy = 200;
    proj.pj_Start.vz = 300;
    CHECK(coll_CheckProjectileCollision(&proj) == 1);

    proj.pj_Target = NULL;
    CHECK(coll_CheckProjectileCollision(&proj) == 0);
    GameStruct.CurrentLevel = 0;
    return 0;
}

static int test_default_character_collision_settings(void)
{
    static const CollisionData expected[19] = {
        {0x20, 0x00, -1}, {0x00, 0x01, -1},
        {0x10, 0x02, -1}, {0x20, 0x03, -1},
        {0x00, 0x04, -1}, {0x10, 0x05, -1},
        {0x20, 0x06, -1}, {0x00, 0x07, -1},
        {0x20, 0x08, -1}, {0x00, 0x09, -1},
        {0x18, 0x0a, -1}, {0x18, 0x0b, -1},
        {0x10, 0x0c, -1}, {0x00, 0x0d, -1},
        {0x18, 0x0e, -1}, {0x20, 0x0f, -1},
        {0x10, 0x11, 0x0c}, {0x20, 0x12, 0x0c},
        {0x20, 0x13, 0x0c}
    };
    playerObject player;
    sceneObject scene;
    modelObject model;
    physicsObject physics;

    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&physics, 0, sizeof(physics));
    game_setFuncArray();
    player.playerID = 0;
    player.pSettings.minClosingDist = 0x14;
    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pModel = &model.modelRoot;
    scene.pPhysics = &physics.physicsRoot;
    CHECK(jpb_ai_ApplyDefaultPlayerSettings(
              &player) == 1);
    CHECK(player.fScale == 0x78a);
    CHECK(model.v3Scale.vx == 0x78a);
    CHECK(model.v3Scale.vy == 0x78a);
    CHECK(model.v3Scale.vz == 0x78a);
    CHECK(model.clipradius == -0x78);
    CHECK(player.numCollisionNodes == 19);
    CHECK(player.pMainCallBack == funcArray[33]);
    CHECK(player.pSettings.minClosingDist == 0x33);
    CHECK(physics.radius == 0x33);
    CHECK(physics.mass == 0x800);
    CHECK(physics.height == 200);
    CHECK(player.pSettings.JumpVel == 0x7a);
    CHECK(player.pSettings.RunningJumpVel == 0x73);
    CHECK(player.pSettings.dblJumpVel == 0x73);
    CHECK(player.pSettings.JumpAngle == 0x2f9);
    CHECK(player.pSettings.RunningJumpAngle == 0x341);
    CHECK(player.pSettings.dblJumpAngle == 0x35a);
    CHECK(player.pSettings.bkJumpAngle == 0x555);
    CHECK(player.pSettings.gravity == UINT16_C(0xe314));
    CHECK(memcmp(
              player.paNodesSizes,
              expected,
              sizeof(expected)) == 0);

    player.playerID = 17;
    CHECK(jpb_ai_ApplyDefaultPlayerSettings(
              &player) == 1);
    CHECK(player.paNodesSizes != NULL);
    return 0;
}

static int test_destroyer_character_collision_settings(void)
{
    static const CollisionData expected[8] = {
        {0x20, 0x00, -1}, {0x40, 0x03, -1},
        {0x40, 0x06, -1}, {0x40, 0x09, -1},
        {0x40, 0x0b, -1}, {0x40, 0x0c, -1},
        {0x40, 0x0e, -1}, {0x40, 0x12, -1}
    };
    playerObject player;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    int32_t controls[2] = {0};

    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&physics, 0, sizeof(physics));
    game_setFuncArray();
    player.playerID = 26;
    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pModel = &model.modelRoot;
    scene.pPhysics = &physics.physicsRoot;
    CHECK(jpb_ai_ApplyPlayerSettings(&player) == 1);
    CHECK(model.v3Scale.vx == 0x78a);
    CHECK(model.v3Scale.vy == 0x78a);
    CHECK(model.v3Scale.vz == 0x78a);
    CHECK(model.clipradius == -0x78);
    CHECK(player.fScale == 0x78a);
    CHECK(player.numCollisionNodes == 8);
    CHECK(player.pSettings.minClosingDist == 0x33);
    CHECK(physics.radius == 0x33);
    CHECK(physics.mass == 0xc00);
    CHECK(physics.height == 0xe0);
    CHECK(player.pMainCallBack == funcArray[44]);
    CHECK(player.pMainCallBack(controls, &player) == -1);
    CHECK(memcmp(
              player.paNodesSizes,
              expected,
              sizeof(expected)) == 0);
    return 0;
}

static int test_loader_droid_collision_settings(void)
{
    static const CollisionData expected[9] = {
        {0x100, 0x00, -1}, {0x100, 0x01, -1},
        {0x080, 0x02, -1}, {0x100, 0x03, -1},
        {0x100, 0x04, -1}, {0x080, 0x05, -1},
        {0x100, 0x06, -1}, {0x100, 0x07, -1},
        {0x040, 0x0a, -1}
    };
    playerObject player;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;

    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&physics, 0, sizeof(physics));
    memset(&animation, 0, sizeof(animation));
    game_setFuncArray();
    player.playerID = 30;
    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pModel = &model.modelRoot;
    scene.pPhysics = &physics.physicsRoot;
    scene.pAnim = &animation.animRoot;
    CHECK(jpb_ai_ApplyPlayerSettings(&player) == 1);
    CHECK(model.v3Scale.vx == 0xd31);
    CHECK(model.v3Scale.vy == 0xd31);
    CHECK(model.v3Scale.vz == 0xd31);
    CHECK(model.clipradius == -0x78);
    CHECK((model.flags & UINT32_C(0x00000008)) != 0);
    CHECK(player.fScale == 0xd31);
    CHECK(player.numCollisionNodes == 9);
    CHECK(player.pSettings.minClosingDist == 0x66);
    CHECK(physics.radius == 0x66);
    CHECK(physics.mass == 0x7fff);
    CHECK(physics.height == 0x100);
    CHECK((player.pFlags & UINT32_C(0x00002000)) != 0);
    CHECK(player.pMainCallBack == funcArray[42]);
    CHECK(memcmp(
              player.paNodesSizes,
              expected,
              sizeof(expected)) == 0);
    return 0;
}

static int test_loader_droid_neutral_callback(void)
{
    playerObject player;
    sceneObject scene;
    animObject animation;
    Mnode root;
    Mnode arm0;
    Mnode arm1;
    int32_t controls[2] = {0};

    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&animation, 0, sizeof(animation));
    memset(&root, 0, sizeof(root));
    memset(&arm0, 0, sizeof(arm0));
    memset(&arm1, 0, sizeof(arm1));
    player.playernum = 6;
    player.playerRoot.objectID = 6;
    player.playerRoot.pParent = &scene.sceneRoot;
    player.forceFlags = UINT32_C(0x000001ef);
    scene.pAnim = &animation.animRoot;
    root.id = NODE_DYNAMIC;
    arm0.id = (modelNodeId)(NODE_DYNAMIC | 11);
    arm1.id = (modelNodeId)(NODE_DYNAMIC | 12);
    coll_ResetCollisionSystem();
    coll_gRegisterNode(6, &root);
    coll_gRegisterNode(6, &arm0);
    coll_gRegisterNode(6, &arm1);
    zerobss_ResetBoss = 1;
    CHECK(ai_LoaderDroid(controls, &player) == -1);
    CHECK(zerobss_ResetBoss == 0);
    CHECK((arm0.flags & UINT32_C(0x00000004)) != 0);
    CHECK((arm1.flags & UINT32_C(0x00000004)) != 0);
    return 0;
}

static int test_star_fighter_collision_settings(void)
{
    static const CollisionData expected[6] = {
        {0x200, 0x00, -1}, {0x100, 0x12, -1},
        {0x100, 0x03, -1}, {0x100, 0x07, -1},
        {0x100, 0x10, -1}, {0x100, 0x0c, -1}
    };
    playerObject player;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    Motion motions[13];
    Mnode root;
    int32_t controls[2] = {0};

    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&physics, 0, sizeof(physics));
    memset(motions, 0, sizeof(motions));
    memset(&root, 0, sizeof(root));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    game_setFuncArray();
    player.playerID = 47;
    player.playernum = 7;
    player.playerRoot.objectID = 7;
    player.playerRoot.pParent = &scene.sceneRoot;
    player.paMotions = motions;
    player.maxMotions = 13;
    scene.pModel = &model.modelRoot;
    scene.pPhysics = &physics.physicsRoot;
    model.pRootNode = &root;
    root.id = NODE_DYNAMIC;
    coll_ResetCollisionSystem();
    coll_gRegisterNode(7, &root);

    CHECK(jpb_ai_ApplyPlayerSettings(&player) == 1);
    CHECK(model.v3Scale.vx == 0xf14);
    CHECK(model.v3Scale.vy == 0xf14);
    CHECK(model.v3Scale.vz == 0xf14);
    CHECK(model.clipradius == -0x78);
    CHECK((model.flags & UINT32_C(0x00000008)) != 0);
    CHECK(player.fScale == 0xf14);
    CHECK(player.numCollisionNodes == 6);
    CHECK(player.pSettings.minClosingDist == 0xcc);
    CHECK(physics.radius == 0xcc);
    CHECK(physics.mass == 0x7fff);
    CHECK(physics.height == 0x200);
    CHECK((player.pFlags & UINT32_C(0x00002000)) != 0);
    CHECK(player.pMainCallBack == funcArray[40]);
    CHECK(memcmp(
              player.paNodesSizes,
              expected,
              sizeof(expected)) == 0);

    motions[1].motionFlags = UINT32_C(0x12345678);
    motions[1].Lock = 9;
    zerobss_ResetBoss = 1;
    CHECK(player.pMainCallBack(controls, &player) == -1);
    CHECK(zerobss_ResetBoss == 0);
    CHECK(motions[6].vel == 0x38);
    CHECK(motions[10].vel == 0x28);
    CHECK(motions[12].vel == 0x28);
    CHECK(motions[12].motionFlags == motions[1].motionFlags);
    CHECK(motions[12].Lock == motions[1].Lock);
    return 0;
}

static int test_fed_environment_model_settings(void)
{
    static const struct {
        int modelId;
        int scale;
        int nodeCount;
        int minimumClosingDistance;
        int mass;
        int height;
        uint32_t modelFlags;
        uint32_t rootFlags;
        int zBufferOffset;
    } cases[] = {
        {72, 0x1e2, 0, 0x00, 0x0800, 0x0c8, 0x10, 0x0000, 0},
        {86, 0x169e, 19, 0x00, 0x7fff, 0x190, 0x0a, 0x1000, 8},
        {87, 0x10f6, 1, 0xd8, 0x7fff, 0x168, 0x0a, 0x1000, 0},
        {94, 0x12d9, 19, 0x33, 0x7fff, 0x010, 0x0a, 0x1000, 0x10},
        {95, 0x14bb, 19, 0x33, 0x4000, 0x010, 0x0a, 0x1000, 8}
    };
    size_t index;

    game_setFuncArray();
    for (index = 0;
         index < sizeof(cases) / sizeof(cases[0]);
         ++index) {
        playerObject player;
        sceneObject scene;
        modelObject model;
        physicsObject physics;
        Mnode root;

        memset(&player, 0, sizeof(player));
        memset(&scene, 0, sizeof(scene));
        memset(&model, 0, sizeof(model));
        memset(&physics, 0, sizeof(physics));
        memset(&root, 0, sizeof(root));
        player.playerID = (int16_t)cases[index].modelId;
        player.playerRoot.objectID = 7;
        player.playerRoot.pParent = &scene.sceneRoot;
        scene.pModel = &model.modelRoot;
        scene.pPhysics = &physics.physicsRoot;
        model.pRootNode = &root;
        root.id = NODE_DYNAMIC;
        coll_ResetCollisionSystem();
        coll_gRegisterNode(7, &root);

        CHECK(jpb_ai_ApplyPlayerSettings(&player) == 1);
        CHECK(model.v3Scale.vx == cases[index].scale);
        CHECK(model.v3Scale.vy == cases[index].scale);
        CHECK(model.v3Scale.vz == cases[index].scale);
        CHECK(model.clipradius == -0x78);
        CHECK(player.fScale == cases[index].scale);
        CHECK(player.numCollisionNodes ==
              cases[index].nodeCount);
        CHECK(player.pSettings.minClosingDist ==
              cases[index].minimumClosingDistance);
        CHECK(physics.radius ==
              cases[index].minimumClosingDistance);
        CHECK(physics.mass == cases[index].mass);
        CHECK(physics.height == cases[index].height);
        CHECK(model.flags == cases[index].modelFlags);
        CHECK(root.flags == cases[index].rootFlags);
        CHECK(root.ZBufferOffset ==
              cases[index].zBufferOffset);
        CHECK(player.pMainCallBack == funcArray[33]);
        if (cases[index].nodeCount == 0) {
            CHECK(player.paNodesSizes == NULL);
        } else {
            CHECK(player.paNodesSizes != NULL);
        }
        if (cases[index].modelId == 87) {
            CHECK(player.paNodesSizes[0].radius1 == 0x40);
            CHECK(player.paNodesSizes[0].id == 0);
            CHECK(player.paNodesSizes[0].parentid == -1);
        }
    }
    return 0;
}

typedef struct ProfileExpectation {
    int modelId;
    int scaleX;
    int scaleY;
    int scaleZ;
    int clipRadius;
    int nodeCount;
    int minimumClosingDistance;
    int mass;
    int height;
    int callbackIndex;
    int jumpVelocity;
    int runningJumpVelocity;
    uint32_t modelFlags;
    uint32_t playerFlags;
    uint32_t physicsFlags;
    uint32_t rootFlags;
    int zBufferOffset;
} ProfileExpectation;

#define P(id, sx, sy, sz, clip, nodes, close, mass, height, callback, jump,  \
          run, model_flags, player_flags, physics_flags, root_flags, z)     \
    {id, sx, sy, sz, clip, nodes, close, mass, height, callback, jump, run, \
     model_flags, player_flags, physics_flags, root_flags, z}

static int test_all_ai_init_player_profiles(void)
{
    static const ProfileExpectation cases[] = {
        P(0,   0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x800, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(9,   0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x7fff, 0xc8, 37, 0xb3, 0xc0, 0, 0, 0, 0, 0),
        P(10,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x7fff, 0xc8, 38, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(79,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x7fff, 0xc8, 38, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(11,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x7fff, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(16,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x7fff, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(58,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x7fff, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(59,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x7fff, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(19,  0x96c, 0x96c, 0x96c, 0x78, 19, 0x40, 0x2000, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(22,  0xd31, 0xb4f, 0xd31, 0x78, 19, 0x40, 0x7fff, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(24,  0x78a, 0x78a, 0x78a, 0x100, 1, 0x66, 0x7fff, 0x200, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(25,  0x78a, 0x78a, 0x78a, 0x78, 2, 0x55, 0x7fff, 0x160, 34, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(33,  0x78a, 0x78a, 0x78a, 0x78, 2, 0x55, 0x7fff, 0x160, 34, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(26,  0x78a, 0x78a, 0x78a, 0x78, 8, 0x33, 0xc00, 0xe0, 44, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(29,  0xb4f, 0xb4f, 0xb4f, 0x78, 6, 0x99, 0x7fff, 0x190, 33, 0x7a, 0x73, 8, 0x2000, 0, 0, 0),
        P(30,  0xd31, 0xd31, 0xd31, 0x78, 9, 0x66, 0x7fff, 0x100, 42, 0x7a, 0x73, 8, 0x2000, 0, 0, 0),
        P(31,  0x25b2, 0x25b2, 0x25b2, 0x78, 8, 0x133, 0x7fff, 0x200, 43, 0x7a, 0x73, 8, 0x2000, 0, 0, 0),
        P(32,  0x78a, 0x78a, 0x78a, 0x78, 4, 0x80, 0x7fff, 0x200, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(34,  0x96c, 0x96c, 0x96c, 0x78, 19, 0x4c, 0x7fff, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(35,  0x608, 0x608, 0x608, 0x200, 9, 0x120, 0x7fff, 0x240, 45, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(38,  0x96c, 0x96c, 0x96c, 0x78, 2, 0x33, 0x400, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(39,  0x96c, 0x96c, 0x96c, 0x78, 2, 0x33, 0x400, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(40,  0xf14, 0xf14, 0xf14, 0x78, 4, 0xcc, 0x7fff, 0x200, 41, 0x7a, 0x73, 8, 0x2000, 0x800000, 0, 0),
        P(41,  0xb4f, 0xb4f, 0xb4f, 0x78, 6, 0x99, 0x7fff, 0x190, 46, 0x7a, 0x73, 8, 0x2000, 0, 0, 0),
        P(43,  0x78a, 0x78a, 0x78a, 0x78, 22, 0x33, 0x7fff, 0xc8, 37, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(44,  0x78a, 0x78a, 0x78a, 0x78, 2, 0x55, 0x7fff, 0x160, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(45,  0x78a, 0x78a, 0x78a, 0x78, 1, 0x0c, 0x200, 0x32, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(46,  0x78a, 0x78a, 0x78a, 0x78, 1, 0x19, 0x800, 0x64, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(47,  0xf14, 0xf14, 0xf14, 0x78, 6, 0xcc, 0x7fff, 0x200, 40, 0x7a, 0x73, 8, 0x2000, 0, 0, 0),
        P(48,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x800, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(49,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x2000, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(50,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0x800, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(51,  0xb4f, 0xb4f, 0xb4f, 0x78, 19, 0x4c, 0x7fff, 0xc8, 35, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(53,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0xc00, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(56,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x33, 0xc00, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(57,  0x78a, 0x78a, 0x78a, 0x78, 19, 0x26, 0x400, 0x64, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(67,  0x1a63, 0x1a63, 0x1a63, 0x200, 0, 0x100, 0x7fff, -0x80, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 0),
        P(68,  0x1a63, 0x1a63, 0x1a63, 0x200, 0, 0x100, 0x7fff, -0x80, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 0),
        P(69,  0x1a63, 0x1a63, 0x1a63, 0x200, 0, 0x100, 0x7fff, -0x80, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 0),
        P(70,  0x25b2, 0x25b2, 0x25b2, 0x200, 1, 0x200, 0x7fff, 0xc8, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 0),
        P(72,  0x1e2, 0x1e2, 0x1e2, 0x78, 0, 0, 0x800, 0xc8, 33, 0x7a, 0x73, 0x10, 0, 0, 0, 0),
        P(73,  0x78a, 0x78a, 0x78a, 0x78, 0, 0, 0x7fff, 0xc8, 47, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(74,  0x486, 0x486, 0x486, 0x78, 6, 0x33, 0x800, 0xc8, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(75,  0x96c, 0x96c, 0x96c, 0x78, 1, 0x10, 0x7fff, 0x40, 33, 0x3d, 0x73, 0, 0, 0, 0, 0),
        P(76,  0x2d3c, 0x2d3c, 0x2d3c, 0x78, 19, 0x400, 0x7fff, 0x10, 36, 0x7a, 0x73, 0x0a, 0, 0, 0, 0),
        P(77,  0xd31, 0xd31, 0xd31, 0x78, 6, 0x66, 0x7fff, 0x200, 39, 0x7a, 0x73, 8, 0x2000, 0x800000, 0, 0),
        P(78,  0x169e, 0x169e, 0x169e, 0x78, 2, 0x10, 0x400, 0x40, 33, 0x3d, 0x73, 0, 0, 0, 0, 0),
        P(86,  0x169e, 0x169e, 0x169e, 0x78, 19, 0, 0x7fff, 0x190, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 8),
        P(87,  0x10f6, 0x10f6, 0x10f6, 0x78, 1, 0xd8, 0x7fff, 0x168, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 0),
        P(89,  0x78a, 0x78a, 0x78a, 0x78, 19, 0, 0x7fff, 0x40, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(93,  0x12d9, 0x12d9, 0x12d9, 0x78, 1, 0x38, 0x7fff, 0x40, 33, 0x7a, 0x73, 2, 0, 0, 0, 0),
        P(94,  0x12d9, 0x12d9, 0x12d9, 0x78, 19, 0x33, 0x7fff, 0x10, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 0x10),
        P(96,  0x12d9, 0x12d9, 0x12d9, 0x78, 19, 0x33, 0x7fff, 0x10, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 0x10),
        P(95,  0x14bb, 0x14bb, 0x14bb, 0x78, 19, 0x33, 0x4000, 0x10, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 8),
        P(97,  0x1c45, 0x1c45, 0x1c45, 0x78, 19, 0x80, 0x7fff, 0x100, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 0),
        P(98,  0x14bb, 0x14bb, 0x14bb, 0x78, 19, 0, 0x7fff, 0x190, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 8),
        P(99,  0x12d9, 0x12d9, 0x12d9, 0x78, 19, 0, 0x7fff, 0x320, 33, 0x7a, 0x73, 0, 0, 0, 0, 0),
        P(100, 0x10f6, 0x10f6, 0x10f6, 0x78, 19, 0, 0x7fff, 0x190, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 8),
        P(114, 0x10f6, 0x10f6, 0x10f6, 0x78, 19, 0, 0x7fff, 0x190, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 8),
        P(102, 0xd92, 0xd92, 0xd92, 0x78, 1, 0x33, 0x7fff, 0x200, 33, 0x7a, 0x73, 8, 0, 0, 0, 0),
        P(103, 0x5956, 0x5956, 0x5956, 0x78, 19, 0x33, 0x4000, 0x10, 33, 0x7a, 0x73, 0x0a, 0, 0, 0x1000, 0),
        P(105, 0x1096, 0x1096, 0x1096, 0x78, 19, 0, 0x7fff, 0x190, 33, 0x7a, 0x73, 0x0a, 0, 0, 0, 8),
        P(112, 0x25b2, 0x25b2, 0x25b2, 0x78, 19, 0x66, 0x7fff, 0x200, 33, 0x7a, 0x73, 8, 0, 0, 0, 0)
    };
    size_t index;
    int callback;

    game_setFuncArray();
    for (callback = 33; callback <= 47; ++callback) {
        funcArray[callback] =
            profile_callbacks[callback - 33];
    }
    for (index = 0;
         index < sizeof(cases) / sizeof(cases[0]);
         ++index) {
        playerObject player;
        sceneObject scene;
        modelObject model;
        physicsObject physics;
        Mnode root;

        memset(&player, 0, sizeof(player));
        memset(&scene, 0, sizeof(scene));
        memset(&model, 0, sizeof(model));
        memset(&physics, 0, sizeof(physics));
        memset(&root, 0, sizeof(root));
        player.playerID = (int16_t)cases[index].modelId;
        player.playerRoot.objectID = 7;
        player.playerRoot.pParent = &scene.sceneRoot;
        scene.pModel = &model.modelRoot;
        scene.pPhysics = &physics.physicsRoot;
        model.pRootNode = &root;
        root.id = NODE_DYNAMIC;
        coll_ResetCollisionSystem();
        coll_gRegisterNode(7, &root);
        gpWorld = NULL;
        LevelSelect = 0;
        gHidePikobisModel = 0;

        CHECK(ai_InitPlayer(&player) == 1);
        CHECK(model.v3Scale.vx == cases[index].scaleX);
        CHECK(model.v3Scale.vy == cases[index].scaleY);
        CHECK(model.v3Scale.vz == cases[index].scaleZ);
        CHECK(model.clipradius == -cases[index].clipRadius);
        CHECK(player.fScale == cases[index].scaleX);
        CHECK(player.numCollisionNodes == cases[index].nodeCount);
        CHECK(player.pSettings.minClosingDist ==
              cases[index].minimumClosingDistance);
        CHECK(physics.radius ==
              cases[index].minimumClosingDistance);
        CHECK(physics.mass == cases[index].mass);
        CHECK(physics.height == cases[index].height);
        CHECK(player.pMainCallBack ==
              profile_callbacks[cases[index].callbackIndex - 33]);
        CHECK(player.pSettings.JumpVel ==
              cases[index].jumpVelocity);
        CHECK(player.pSettings.RunningJumpVel ==
              cases[index].runningJumpVelocity);
        CHECK(model.flags == cases[index].modelFlags);
        CHECK(player.pFlags == cases[index].playerFlags);
        CHECK(physics.flags == cases[index].physicsFlags);
        CHECK(root.flags == cases[index].rootFlags);
        CHECK(root.ZBufferOffset == cases[index].zBufferOffset);
        CHECK(player.paCombos != NULL);
        CHECK(hash_bytes(
                  player.paCombos,
                  22 * sizeof(Combo)) == UINT32_C(0x99ac948f));
    }
    return 0;
}

static int test_ai_init_player_collision_table_bytes(void)
{
    static const struct {
        int modelId;
        int nodeCount;
        uint32_t hash;
    } cases[] = {
        {40, 4, UINT32_C(0x92cfd7d6)},
        {77, 6, UINT32_C(0x7a67e06e)},
        {10, 19, UINT32_C(0x454c1de2)},
        {34, 19, UINT32_C(0x786708ff)},
        {45, 1, UINT32_C(0x221f2038)},
        {70, 1, UINT32_C(0x5184b898)},
        {75, 1, UINT32_C(0x6fafdda3)},
        {0, 19, UINT32_C(0x35f22b01)},
        {24, 1, UINT32_C(0x28fc7d84)},
        {43, 22, UINT32_C(0x69dd24af)},
        {38, 2, UINT32_C(0xe01daa9a)},
        {35, 9, UINT32_C(0xf371278f)},
        {93, 1, UINT32_C(0x51840d01)},
        {30, 9, UINT32_C(0x3c0b82aa)},
        {47, 6, UINT32_C(0x288d217e)},
        {29, 6, UINT32_C(0x9569c60d)},
        {31, 8, UINT32_C(0xb68b76f2)},
        {25, 2, UINT32_C(0x6c04a176)},
        {32, 4, UINT32_C(0xa1ced5fd)},
        {78, 2, UINT32_C(0x2644c29a)},
        {26, 8, UINT32_C(0xa0911612)}
    };
    size_t index;
    uint32_t actual_hash;

    for (index = 0;
         index < sizeof(cases) / sizeof(cases[0]);
         ++index) {
        playerObject player;
        sceneObject scene;
        modelObject model;
        physicsObject physics;
        Mnode root;

        memset(&player, 0, sizeof(player));
        memset(&scene, 0, sizeof(scene));
        memset(&model, 0, sizeof(model));
        memset(&physics, 0, sizeof(physics));
        memset(&root, 0, sizeof(root));
        player.playerID = (int16_t)cases[index].modelId;
        player.playerRoot.objectID = 7;
        player.playerRoot.pParent = &scene.sceneRoot;
        scene.pModel = &model.modelRoot;
        scene.pPhysics = &physics.physicsRoot;
        model.pRootNode = &root;
        root.id = NODE_DYNAMIC;
        coll_ResetCollisionSystem();
        coll_gRegisterNode(7, &root);
        gpWorld = NULL;
        LevelSelect = 0;

        CHECK(ai_InitPlayer(&player) == 1);
        CHECK(player.numCollisionNodes == cases[index].nodeCount);
        actual_hash = hash_bytes(
            player.paNodesSizes,
            (size_t)cases[index].nodeCount *
                sizeof(CollisionData));
        if (actual_hash != cases[index].hash) {
            fprintf(
                stderr,
                "model %d collision hash 0x%08x, expected 0x%08x\n",
                cases[index].modelId,
                (unsigned int)actual_hash,
                (unsigned int)cases[index].hash);
            return 1;
        }
    }
    return 0;
}

static int test_pikobis_visibility_branch(void)
{
    playerObject player;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    WorldData world;

    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&physics, 0, sizeof(physics));
    memset(&world, 0, sizeof(world));
    player.playerID = 46;
    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pModel = &model.modelRoot;
    scene.pPhysics = &physics.physicsRoot;
    gpWorld = &world;
    LevelSelect = 2;
    gHidePikobisModel = 1;

    CHECK(ai_InitPlayer(&player) == 1);
    CHECK((player.forceFlags & UINT32_C(0x80)) != 0);
    CHECK((model.flags & UINT32_C(0x10)) != 0);
    CHECK(gHidePikobisModel == 0);

    player.forceFlags = UINT32_C(0x80000000);
    model.flags = 0;
    world.aDolly[0].flags = UINT32_C(0x400);
    gHidePikobisModel = 1;
    CHECK(ai_InitPlayer(&player) == 1);
    CHECK(player.forceFlags == UINT32_C(0x80000000));
    CHECK(model.flags == 0);
    CHECK(gHidePikobisModel == 1);
    gpWorld = NULL;
    LevelSelect = 0;
    return 0;
}

#undef P

int main(int argc, char **argv)
{
    if (argc == 2 &&
        strcmp(argv[1], "--registration-fatal") == 0) {
        Mnode invalid_node;

        memset(&invalid_node, 0, sizeof(invalid_node));
        invalid_node.id =
            (modelNodeId)((uint32_t)NODE_DYNAMIC |
                          (uint32_t)JPB_COLLISION_NODE_CAPACITY);
        coll_ResetCollisionSystem();
        coll_gRegisterNode(0, &invalid_node);
        return 0;
    }

    CHECK(test_4d_collision() == 0);
    CHECK(test_registry_and_accessors() == 0);
    CHECK(test_hot_node_player_collision() == 0);
    CHECK(test_hot_node_signed_parent_lookup() == 0);
    CHECK(test_projectile_collision() == 0);
    CHECK(test_default_character_collision_settings() == 0);
    CHECK(test_destroyer_character_collision_settings() == 0);
    CHECK(test_loader_droid_collision_settings() == 0);
    CHECK(test_loader_droid_neutral_callback() == 0);
    CHECK(test_star_fighter_collision_settings() == 0);
    CHECK(test_fed_environment_model_settings() == 0);
    CHECK(test_all_ai_init_player_profiles() == 0);
    CHECK(test_ai_init_player_collision_table_bytes() == 0);
    CHECK(test_pikobis_visibility_branch() == 0);
    puts("collision tests passed");
    return 0;
}
