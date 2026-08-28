#include "jpb/anim.h"
#include "jpb/bmd.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/effects.h"
#include "jpb/force.h"
#include "jpb/fx.h"
#include "jpb/game.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    TEST_MOTION_COUNT = 160,
    TEST_NODE_COUNT = 23
};

typedef struct ForceFixture {
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    modelObject model;
    animObject *animation;
    animListNode current_sequence;
    Mnode nodes[TEST_NODE_COUNT];
    geomData geometry[3];
    Motion motions[TEST_MOTION_COUNT];
    _animTemplate templates[TEST_MOTION_COUNT];
} ForceFixture;

static int failures;
static uint32_t test_pose_words[3];

static void init_test_animations(void)
{
    int index;

    (anim_InitAnimations)(0);
    for (index = 0; index < JPB_ANIMATION_CAPACITY; ++index) {
        maAnimationData[index].depack_context.huffdataorigin =
            test_pose_words;
        maAnimationData[index].depack_context3.huffdataorigin =
            test_pose_words;
    }
}

typedef struct GlowTrace {
    int calls;
    _svector first_start;
    _svector first_end;
    _svector last_start;
    _svector last_end;
    int first_width;
    uint32_t first_color;
    int width;
    uint32_t color;
} GlowTrace;

typedef struct CylinderTrace {
    int calls;
    const VECTOR *location;
    _svector rotation;
    float radius1;
    float radius2;
    float h1;
    float h2;
    uint32_t color1;
    uint32_t color2;
} CylinderTrace;

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            fprintf( \
                stderr, \
                "FAIL %s:%d: %s\n", \
                __FILE__, \
                __LINE__, \
                #condition); \
            ++failures; \
        } \
    } while (0)

static void trace_screen_glow(
    void *user_data,
    const _svector *start,
    const _svector *end,
    int width,
    uint32_t color)
{
    GlowTrace *trace = (GlowTrace *)user_data;

    if (trace->calls == 0) {
        trace->first_start = *start;
        trace->first_end = *end;
        trace->first_width = width;
        trace->first_color = color;
    }
    trace->last_start = *start;
    trace->last_end = *end;
    trace->width = width;
    trace->color = color;
    ++trace->calls;
}

static void trace_cylinder(
    void *user_data,
    const VECTOR *location,
    const _svector *rotation,
    float radius1,
    float radius2,
    float h1,
    float h2,
    uint32_t color1,
    uint32_t color2)
{
    CylinderTrace *trace =
        (CylinderTrace *)user_data;

    ++trace->calls;
    trace->location = location;
    trace->rotation = *rotation;
    trace->radius1 = radius1;
    trace->radius2 = radius2;
    trace->h1 = h1;
    trace->h2 = h2;
    trace->color1 = color1;
    trace->color2 = color2;
}

static void reset_fixture(ForceFixture *fixture)
{
    int index;

    memset(fixture, 0, sizeof(*fixture));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(gaPlayerData, 0, sizeof(gaPlayerData));
    memset(maPhysicsData, 0, sizeof(maPhysicsData));
    for (index = 0;
         index < JPB_PLAYER_CAPACITY;
         ++index) {
        gaPlayerData[index].playerRoot.objectID = -1;
    }
    for (index = 0;
         index < JPB_PHYSICS_CAPACITY;
         ++index) {
        maPhysicsData[index].physicsRoot.objectID = -1;
    }
    init_test_animations();
    fixture->animation = &maAnimationData[0];

    fixture->player.playerRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->player.playernum = 0;
    fixture->player.target = &fixture->player;
    fixture->player.paMotions = fixture->motions;
    fixture->player.maxMotions = TEST_MOTION_COUNT;
    fixture->player.oldmaxCMotions = TEST_MOTION_COUNT;
    fixture->player.pMotion =
        &fixture->animation->pMotion;

    fixture->scene.pPhysics =
        &fixture->physics.physicsRoot;
    fixture->scene.pModel =
        &fixture->model.modelRoot;
    fixture->scene.pAnim =
        &fixture->animation->animRoot;
    fixture->scene.pPlayer =
        &fixture->player.playerRoot;
    fixture->animation->animRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->animation->depack_context.seqdata =
        fixture->templates;

    fixture->physics.physicsRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->model.modelRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->physics.pos.vx = 12.75f;
    fixture->physics.pos.vy = -3.5f;
    fixture->physics.pos.vz = 99.25f;
    fixture->player.playerRoot.objectID = 0;
    coll_ResetPlayerCollision(0);
    for (index = 0;
         index < TEST_NODE_COUNT;
         ++index) {
        fixture->nodes[index].id =
            (modelNodeId)(NODE_DYNAMIC | index);
        coll_gRegisterNode(
            0, &fixture->nodes[index]);
    }

    for (index = 0; index < TEST_MOTION_COUNT; ++index) {
        fixture->motions[index].Seq =
            (uint16_t)index;
        fixture->motions[index].Speed = -1;
        fixture->templates[index].Lframe = 10;
    }
    GameStruct.aCharacterData[0].Force = 100;
    GameStruct.aCharacterData[0].MaxForce = 100;
    storeAnim = NULL;
    jpb_FxSetScreenGlowHook(NULL, NULL);
}

static void prepare_glowing_man_hierarchy(
    ForceFixture *fixture)
{
    Mnode *root = &fixture->nodes[0];
    Mnode *joint = &fixture->nodes[1];
    Mnode *leaf = &fixture->nodes[2];

    fixture->geometry[0].numFaces = 1;
    fixture->geometry[1].numFaces = 1;
    fixture->geometry[2].numFaces = 1;
    root->pGeomData = &fixture->geometry[0];
    root->numChildNodes = 1;
    root->aChildNode = joint;
    joint->pGeomData = &fixture->geometry[1];
    joint->numChildNodes = 1;
    joint->aChildNode = leaf;
    joint->v3RotCenter.vx = 100;
    leaf->pGeomData = &fixture->geometry[2];
    leaf->v3RotCenter.vx = 200;
}

static void test_primary_force_sequences(void)
{
    ForceFixture fixture;
    ForceSlot attack = {{-40, 0, 0, 0}, 6};
    ForceSlot toss = {{65, 0, 0, 0}, 0};
    ForceSlot empty = {{0, 0, 0, 0}, 6};

    reset_fixture(&fixture);
    CHECK(force_PlaySeq(NULL, &fixture.player) == 0);
    CHECK(force_PlaySeq(&empty, &fixture.player) == 0);
    CHECK(force_PlaySeq(&attack, &fixture.player) == 1);
    CHECK(fixture.motions[99].FunctPtr == 8);
    CHECK(fixture.animation->pMotion ==
          &fixture.motions[99]);
    CHECK(fixture.player.currentMotion == 99);

    reset_fixture(&fixture);
    CHECK(force_PlaySeq(&toss, &fixture.player) == 1);
    CHECK(fixture.motions[65].FunctPtr == 23);
    CHECK(storeAnim == fixture.animation);
}

static void test_color_interpolation(void)
{
    uint32_t color = UINT32_C(0xff204060);
    uint32_t base = UINT32_C(0x00102030);

    CHECK(color_interpolate(
              color, base, 0) == base);
    CHECK(color_interpolate(
              color, base, 256) == color);
    CHECK(color_interpolate(
              color, base, 128) ==
          UINT32_C(0x7f183048));
    CHECK(color_interpolate4k(
              color, base, 2048) ==
          UINT32_C(0x7f183048));
    CHECK(color_interpolate4k(
              color, base, 4096) == color);
}

static void test_chained_force_sequences(void)
{
    ForceFixture fixture;
    ForceSlot ranged = {{-75, -76, -77, 0}, 3};
    ForceSlot reflected = {{-40, -41, 0, 0}, 2};
    animListNode *queued;

    reset_fixture(&fixture);
    CHECK(force_PlaySeq(&ranged, &fixture.player) == 1);
    CHECK(fixture.motions[134].FunctPtr == 16);
    CHECK(fixture.player.forceData[0] == 0);
    CHECK(fixture.player.forceData[1] == 1);
    CHECK(fixture.player.forceData[2] == 22);
    CHECK(fixture.player.forceData[3] == 0);
    queued =
        (animListNode *)fixture.animation->animList.head;
    CHECK(queued != NULL);
    CHECK(queued->pMotion == &fixture.motions[135]);
    CHECK(fixture.motions[135].FunctPtr == 16);
    CHECK((fixture.motions[135].motionFlags &
           UINT32_C(0x80000000)) != 0);
    CHECK(queued->anm_Node.next != NULL);
    CHECK(((animListNode *)queued->anm_Node.next)
              ->pMotion == &fixture.motions[136]);

    reset_fixture(&fixture);
    CHECK(force_PlaySeq(&reflected, &fixture.player) == 1);
    CHECK(fixture.player.forceData[0] == 0);
    CHECK(fixture.player.forceData[1] == 0);
    CHECK(fixture.player.forceData[2] == 50);
    CHECK(fixture.motions[100].FunctPtr == 7);
    CHECK((fixture.motions[100].motionFlags &
           UINT32_C(0x80000000)) != 0);
}

static void test_persistent_force_callbacks(void)
{
    ForceFixture fixture;
    ForceSlot shield = {{74, 0, 0, 0}, 8};
    ForceSlot star = {{67, 0, 0, 0}, 13};
    ForceSlot cloak = {{-58, 0, 0, 0}, 14};
    ForceSlot mesmerize = {{-42, 0, 0, 0}, 11};

    reset_fixture(&fixture);
    fixture.player.forceData[0] = 1;
    fixture.player.forceData[1] = 2;
    fixture.player.forceData[2] = 3;
    fixture.player.forceData[3] = 4;
    CHECK(force_PlaySeq(&shield, &fixture.player) == 1);
    CHECK(fixture.player.pForceCallBack ==
          force_ShieldCallBack);
    CHECK(fixture.player.forceData[0] == 0);
    CHECK(fixture.player.forceData[3] == 0);

    reset_fixture(&fixture);
    CHECK(force_PlaySeq(&star, &fixture.player) == 1);
    CHECK(fixture.player.pForceCallBack ==
          force_StarCallBack);

    reset_fixture(&fixture);
    CHECK(force_PlaySeq(&cloak, &fixture.player) == 1);
    CHECK(fixture.player.pForceCallBack ==
          force_CloakCallBack);

    reset_fixture(&fixture);
    GameStruct.aCharacterData[0].Force = 35;
    CHECK(force_PlaySeq(&mesmerize, &fixture.player) == 0);
    CHECK(GameStruct.aCharacterData[0].Force == 35);
    CHECK(fixture.player.pForceCallBack == NULL);

    reset_fixture(&fixture);
    CHECK(force_PlaySeq(&mesmerize, &fixture.player) == 1);
    CHECK(GameStruct.aCharacterData[0].Force == 68);
    CHECK(fixture.player.forceData[0] == 12);
    CHECK(fixture.player.forceData[1] == 124);
    CHECK(fixture.player.forceData[2] == 99);
    CHECK(fixture.player.forceData[3] == 0);
    CHECK(fixture.player.pForceCallBack ==
          force_MesmerizeCallBack);
}

static void test_healing_callback(void)
{
    ForceFixture fixture;

    reset_fixture(&fixture);
    fixture.player.playerRoot.objectID = 0;
    GameStruct.aCharacterData[0].Energy = 50;
    GameStruct.aCharacterData[0].MaxEnergy = 100;
    GameStruct.aCharacterData[0].Force = 50;
    GameStruct.aCharacterData[0].MaxForce = 100;
    CHECK(force_HealingCallBack(
              NULL, &fixture.player) == 1);
    CHECK(GameStruct.aCharacterData[0].Energy == 75);
    CHECK(GameStruct.aCharacterData[0].Force == 40);
}

static void test_cloak_callback(void)
{
    ForceFixture fixture;
    GlowTrace trace;
    WorldData world;

    reset_fixture(&fixture);
    memset(&trace, 0, sizeof(trace));
    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    fixture.player.playerID = 3;
    fixture.player.forceFlags =
        UINT32_C(0x100);
    fixture.model.flags = UINT32_C(0x20);
    prepare_glowing_man_hierarchy(&fixture);
    jpb_FxSetScreenGlowHook(
        trace_screen_glow, &trace);

    CHECK(force_CloakCallBack(
              NULL, &fixture.player) == 0);
    CHECK(fixture.player.forceData[0] == 1);
    CHECK(fixture.player.forceData[1] == 599);
    CHECK(GameStruct.aCharacterData[0].Force == 80);
    CHECK(fixture.player.forceFlags ==
          UINT32_C(0x180));
    CHECK(fixture.model.flags ==
          UINT32_C(0x30));
    CHECK(trace.calls == 2);
    CHECK(trace.first_width == 48);
    CHECK(trace.width == 54);
    CHECK(trace.first_color ==
          UINT32_C(0xc0200808));
    CHECK(trace.color ==
          UINT32_C(0xc0301010));
    CHECK(trace.first_start.vx == 0);
    CHECK(trace.first_end.vx == 100);
    CHECK(trace.last_start.vx == 100);
    CHECK(trace.last_end.vx == 208);

    fixture.player.forceData[1] = 0;
    CHECK(force_CloakCallBack(
              NULL, &fixture.player) == 1);
    CHECK(fixture.player.forceData[1] == -1);
    CHECK(fixture.player.forceFlags ==
          UINT32_C(0x100));
    CHECK(fixture.model.flags ==
          UINT32_C(0x20));

    reset_fixture(&fixture);
    gpWorld = &world;
    world.currentDolly = 2;
    world.aDolly[2].flags = UINT32_C(0x400);
    fixture.player.forceData[0] = 1;
    fixture.player.forceData[1] = 500;
    fixture.player.forceFlags =
        UINT32_C(0x80);
    fixture.model.flags = UINT32_C(0x10);
    CHECK(force_CloakCallBack(
              NULL, &fixture.player) == 1);
    CHECK(fixture.player.forceData[1] == 499);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x80)) == 0);
    CHECK((fixture.model.flags &
           UINT32_C(0x10)) == 0);

    gpWorld = NULL;
    jpb_FxSetScreenGlowHook(NULL, NULL);
}

static void test_screen_glow_fv_conversion(void)
{
    FVECTOR start = {12.75f, -3.75f, 40000.0f};
    FVECTOR end = {-9.5f, 8.5f, -40000.0f};
    GlowTrace trace;

    memset(&trace, 0, sizeof(trace));
    jpb_FxSetScreenGlowHook(
        trace_screen_glow, &trace);
    fx_screenGlowFV(
        &start,
        &end,
        17,
        UINT32_C(0x12345678));
    CHECK(trace.calls == 1);
    CHECK(trace.first_start.vx == 12);
    CHECK(trace.first_start.vy == -3);
    CHECK(trace.first_start.vz == (int16_t)40000);
    CHECK(trace.first_end.vx == -9);
    CHECK(trace.first_end.vy == 8);
    CHECK(trace.first_end.vz == (int16_t)-40000);
    CHECK(trace.width == 17);
    CHECK(trace.color == UINT32_C(0x12345678));
    jpb_FxSetScreenGlowHook(NULL, NULL);
}

static void test_mesmerize_callback(void)
{
    ForceFixture fixture;
    sceneObject target_scene;
    modelObject target_model;
    objectRoot target_scene_component;
    Motion target_motions[62];
    _animTemplate target_templates[62];
    animObject *target_animation;
    playerObject *target;
    WorldData world;
    int index;

    reset_fixture(&fixture);
    memset(&target_scene, 0, sizeof(target_scene));
    memset(&target_model, 0, sizeof(target_model));
    memset(
        &target_scene_component,
        0,
        sizeof(target_scene_component));
    memset(
        target_motions,
        0,
        sizeof(target_motions));
    memset(
        target_templates,
        0,
        sizeof(target_templates));
    memset(
        maPhysicsData,
        0,
        sizeof(maPhysicsData));
    memset(
        gaPlayerData,
        0,
        sizeof(gaPlayerData));
    for (index = 0;
         index < JPB_PLAYER_CAPACITY;
         ++index) {
        gaPlayerData[index]
            .playerRoot
            .objectID = -1;
    }

    target = &gaPlayerData[2];
    target_animation = &maAnimationData[2];
    target->playerRoot.objectID = 2;
    target->playerRoot.pParent =
        &target_scene.sceneRoot;
    target->playerID = 1;
    target->pFlags = UINT32_C(0x20);
    target->paMotions = target_motions;
    target->maxMotions = 62;
    target->pMotion =
        &target_animation->pMotion;
    target_motions[61].Seq = 61;
    target_motions[61].Speed = -1;
    target_motions[61].Lock = 1;
    target_templates[61].Fframe = 0;
    target_templates[61].Lframe = 10;

    target_scene.pScene =
        &target_scene_component;
    target_scene.pModel =
        &target_model.modelRoot;
    target_scene.pPhysics =
        &maPhysicsData[2].physicsRoot;
    target_scene.pAnim =
        &target_animation->animRoot;
    target_scene.pPlayer =
        &target->playerRoot;
    target_model.modelRoot.pParent =
        &target_scene.sceneRoot;
    maPhysicsData[2]
        .physicsRoot
        .pParent =
        &target_scene.sceneRoot;
    target_animation
        ->animRoot
        .pParent =
        &target_scene.sceneRoot;
    target_animation
        ->depack_context
        .seqdata = target_templates;

    fixture.player.forceData[0] = 0;
    fixture.player.forceData[1] = 0;
    fixture.player.forceData[2] = 0;
    fixture.player.forceData[3] = 0;
    fixture.player.forceData[4] = 2;
    gpWorld = NULL;

    CHECK(force_MesmerizeCallBack(
              NULL, &fixture.player) == 0);
    CHECK((target->pFlags &
           UINT32_C(0x20)) == 0);
    CHECK(target->currentMotion == 61);
    CHECK(fixture.player.forceData[3] == 1);
    CHECK(fixture.player.forceData[4] == 3);

    fixture.player.forceData[3] = 300;
    CHECK(force_MesmerizeCallBack(
              NULL, &fixture.player) == 1);
    CHECK(fixture.player.forceData[3] == 301);

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    world.currentDolly = 1;
    world.aDolly[1].flags = UINT32_C(0x400);
    fixture.player.forceData[3] = 10;
    CHECK(force_MesmerizeCallBack(
              NULL, &fixture.player) == 1);
    CHECK(fixture.player.forceData[3] == 10);
    gpWorld = NULL;
}

static void test_attack_spin_callback(void)
{
    ForceFixture fixture;
    int32_t cpad[2] = {0, INT32_C(0x10)};

    reset_fixture(&fixture);
    fixture.current_sequence.pMotion =
        &fixture.motions[99];
    fixture.animation->pCurrentAnimSeq =
        &fixture.current_sequence;

    CHECK(force_AttackSpinCallBack(
              cpad, &fixture.player) == 0);
    CHECK(fixture.player.forceData[0] == 1);
    CHECK(GameStruct.aCharacterData[0].Force == 100);
    CHECK((fixture.motions[99].motionFlags &
           UINT32_C(0x80000000)) != 0);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x22)) == UINT32_C(0x22));

    CHECK(force_AttackSpinCallBack(
              cpad, &fixture.player) == 0);
    CHECK(force_AttackSpinCallBack(
              cpad, &fixture.player) == 0);
    CHECK(fixture.player.forceData[0] == 0);
    CHECK(GameStruct.aCharacterData[0].Force == 98);

    cpad[1] = INT32_C(0x1010);
    CHECK(force_AttackSpinCallBack(
              cpad, &fixture.player) == 0);
    CHECK(force_AttackSpinCallBack(
              cpad, &fixture.player) == 0);
    CHECK(force_AttackSpinCallBack(
              cpad, &fixture.player) == 0);
    CHECK(GameStruct.aCharacterData[0].Force == 94);

    cpad[1] = 0;
    CHECK(force_AttackSpinCallBack(
              cpad, &fixture.player) == 1);
    CHECK((fixture.motions[99].motionFlags &
           UINT32_C(0x80000000)) == 0);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x22)) == 0);

    GameStruct.aCharacterData[0].Force = 3;
    cpad[1] = INT32_C(0x10);
    CHECK(force_AttackSpinCallBack(
              cpad, &fixture.player) == 1);
}

static void test_sabre_spin_callback(void)
{
    ForceFixture fixture;
    Mnode *sabre;
    Mnode *target;

    reset_fixture(&fixture);
    fixture.current_sequence.pAnimTemplate =
        &fixture.templates[0];
    fixture.animation->pCurrentAnimSeq =
        &fixture.current_sequence;
    fixture.templates[0].Fframe = 0;
    sabre = &fixture.nodes[12];
    target = &fixture.nodes[20];
    target->v3RotCenter.vx = 1234;
    target->v3RotCenter.vy = -2345;
    target->v3RotCenter.vz = 3456;
    sabre->v3Velocity2.vx = 10;
    sabre->v3Velocity2.vy = 20;
    sabre->v3Velocity2.vz = 30;
    sabre->time = 99;

    fixture.animation->animFrameIndex =
        10 << JPB_FIXED_SHIFT;
    CHECK(force_SabreSpinCallBack(
              NULL, &fixture.player) == 0);
    CHECK(sabre->v3Velocity2.vx == 0);
    CHECK(sabre->v3Velocity2.vy == 0);
    CHECK(sabre->v3Velocity2.vz == 0);
    CHECK(sabre->v3Translation2.vx == 1234);
    CHECK(sabre->v3Translation2.vy == -2345);
    CHECK(sabre->v3Translation2.vz == 3456);
    CHECK((sabre->flags &
           UINT32_C(0x04000000)) != 0);
    CHECK((fixture.nodes[20].flags &
           UINT32_C(1)) != 0);
    CHECK((fixture.nodes[21].flags &
           UINT32_C(1)) != 0);
    CHECK((fixture.nodes[22].flags &
           UINT32_C(1)) != 0);
    CHECK(sabre->time == 0);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x12)) ==
          UINT32_C(0x12));
    CHECK((fixture.player.pFlags &
           UINT32_C(0x2000)) != 0);

    sabre->v3Velocity2.vx = 1;
    sabre->v3Velocity2.vy = 2;
    sabre->v3Velocity2.vz = 3;
    fixture.animation->animFrameIndex =
        64 << JPB_FIXED_SHIFT;
    CHECK(force_SabreSpinCallBack(
              NULL, &fixture.player) == 1);
    CHECK(GameStruct.aCharacterData[0].Force == 85);
    CHECK((sabre->flags &
           UINT32_C(0x04000000)) == 0);
    CHECK(sabre->v3Velocity2.vx == 0);
    CHECK(sabre->v3Translation2.vx == 0);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x12)) == 0);
    CHECK((fixture.player.pFlags &
           UINT32_C(0x2000)) == 0);
}

static void test_sabre_yoyo_callback(void)
{
    ForceFixture fixture;
    Mnode *sabre;
    Mnode *target;

    reset_fixture(&fixture);
    fixture.current_sequence.pAnimTemplate =
        &fixture.templates[0];
    fixture.animation->pCurrentAnimSeq =
        &fixture.current_sequence;
    fixture.templates[0].Fframe = 0;
    sabre = &fixture.nodes[12];
    target = &fixture.nodes[20];
    target->v3RotCenter.vx = -321;
    target->v3RotCenter.vy = 654;
    target->v3RotCenter.vz = -987;
    sabre->v3Velocity2.vx = 10;
    sabre->time = 44;

    fixture.animation->animFrameIndex =
        7 << JPB_FIXED_SHIFT;
    CHECK(force_SabreYoYoBack(
              NULL, &fixture.player) == 0);
    CHECK(sabre->v3Velocity2.vx == 0);
    CHECK(sabre->v3Translation2.vx == -321);
    CHECK(sabre->v3Translation2.vy == 654);
    CHECK(sabre->v3Translation2.vz == -987);
    CHECK((sabre->flags &
           UINT32_C(0x04000000)) != 0);
    CHECK(sabre->time == 0);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x10)) != 0);
    CHECK((fixture.player.pFlags &
           UINT32_C(0x2000)) != 0);

    fixture.animation->animFrameIndex =
        23 << JPB_FIXED_SHIFT;
    CHECK(force_SabreYoYoBack(
              NULL, &fixture.player) == 1);
    CHECK(GameStruct.aCharacterData[0].Force == 85);
    CHECK((sabre->flags &
           UINT32_C(0x04000000)) == 0);
    CHECK(sabre->v3Translation2.vx == 0);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x10)) == 0);
    CHECK((fixture.player.pFlags &
           UINT32_C(0x2000)) == 0);
}

static void test_sabre_toss_callback(void)
{
    ForceFixture fixture;
    Mnode *sabre;
    Mnode *target;
    int old_frame_rate = gGlobalFrameRate;

    reset_fixture(&fixture);
    fixture.current_sequence.pAnimTemplate =
        &fixture.templates[0];
    fixture.animation->pCurrentAnimSeq =
        &fixture.current_sequence;
    fixture.templates[0].Fframe = 0;
    sabre = &fixture.nodes[12];
    target = &fixture.nodes[0];
    sabre->v3CurrentRotation.vx = 10;
    sabre->v3CurrentRotation.vy = 20;
    sabre->v3CurrentRotation.vz = 30;
    sabre->v3RotCenter.vx = 100;
    sabre->v3RotCenter.vy = 200;
    sabre->v3RotCenter.vz = 300;
    sabre->v3Velocity2.vx = 1;
    sabre->v3Velocity2.vy = 2;
    sabre->v3Velocity2.vz = 3;
    target->v3RotCenter.vx = 200;
    target->v3RotCenter.vy = 200;
    target->v3RotCenter.vz = 300;
    gGlobalFrameRate = JPB_FIXED_ONE;

    fixture.animation->animFrameIndex =
        10 << JPB_FIXED_SHIFT;
    CHECK(force_SabreTossCallBack(
              NULL, &fixture.player) == 0);
    CHECK(fixture.player.forceData[0] == 1);
    CHECK(sabre->v3Velocity2.vx == 0);
    CHECK(sabre->v3Velocity2.vy == 0);
    CHECK(sabre->v3Velocity2.vz == 0);
    CHECK(sabre->v3RotCenter.vx == 125);
    CHECK(sabre->v3RotCenter.vy == 200);
    CHECK(sabre->v3RotCenter.vz == 300);
    CHECK(sabre->v3Translation2.vx == 125);
    CHECK((sabre->flags &
           UINT32_C(0x04000020)) ==
          UINT32_C(0x04000020));
    CHECK(sabre->v3RotationAbs.vx == 202);
    CHECK(sabre->v3RotationAbs.vy == 20);
    CHECK(sabre->v3RotationAbs.vz == 286);
    CHECK(sabre->time == 0);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x10)) != 0);
    CHECK((fixture.player.pFlags &
           UINT32_C(0x2000)) != 0);

    fixture.animation->animFrameIndex =
        42 << JPB_FIXED_SHIFT;
    CHECK(force_SabreTossCallBack(
              NULL, &fixture.player) == 1);
    CHECK(GameStruct.aCharacterData[0].Force == 80);
    CHECK(fixture.player.forceData[0] == 0);
    CHECK(sabre->v3Velocity2.vx == 0);
    CHECK(sabre->v3Translation2.vx == 0);
    CHECK((sabre->flags &
           UINT32_C(0x04000020)) == 0);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x10)) == 0);
    CHECK((fixture.player.pFlags &
           UINT32_C(0x2000)) == 0);

    gGlobalFrameRate = old_frame_rate;
}

static void test_attack_callback(void)
{
    static const int glow_node_ids[8] = {
        0, 7, 2, 5, 3, 6, 9, 13
    };
    ForceFixture fixture;
    GlowTrace trace;
    int index;

    reset_fixture(&fixture);
    memset(&trace, 0, sizeof(trace));
    fixture.current_sequence.pAnimTemplate =
        &fixture.templates[0];
    fixture.animation->pCurrentAnimSeq =
        &fixture.current_sequence;
    fixture.templates[0].Fframe = 10;
    fixture.animation->animFrameIndex =
        (10 + 4) << JPB_FIXED_SHIFT;

    GameStruct.aCharacterData[0].Force = 80;
    CHECK(force_AttackCallBack(
              NULL, &fixture.player) == 0);
    CHECK(GameStruct.aCharacterData[0].Force == 55);
    CHECK(trace.calls == 0);
    CHECK((fixture.player.pFlags &
           UINT32_C(0x2000)) == 0);

    for (index = 0; index < 8; ++index) {
        Mnode *node =
            &fixture.nodes[glow_node_ids[index]];

        node->v3RotCenter.vx =
            1000 + glow_node_ids[index] * 10;
        node->v3RotCenter.vy =
            2000 + glow_node_ids[index] * 10;
        node->v3RotCenter.vz =
            3000 + glow_node_ids[index] * 10;
        node->v3Velocity.vx =
            (int16_t)(1 + glow_node_ids[index]);
        node->v3Velocity.vy =
            (int16_t)(2 + glow_node_ids[index]);
        node->v3Velocity.vz =
            (int16_t)(3 + glow_node_ids[index]);
    }
    jpb_FxSetScreenGlowHook(
        trace_screen_glow, &trace);
    fixture.animation->animFrameIndex =
        (10 + 5) << JPB_FIXED_SHIFT;
    fixture.player.pFlags = UINT32_C(0x4000);
    CHECK(force_AttackCallBack(
              NULL, &fixture.player) == 0);
    CHECK(trace.calls == 8);
    CHECK(trace.width == 30);
    CHECK(trace.color ==
          UINT32_C(0xc0202020));
    CHECK(trace.first_end.vx == 1000);
    CHECK(trace.first_end.vy == 2000);
    CHECK(trace.first_end.vz == 3000);
    CHECK(trace.first_start.vx == 999);
    CHECK(trace.first_start.vy == 1998);
    CHECK(trace.first_start.vz == 2997);
    CHECK(trace.last_end.vx == 1130);
    CHECK(trace.last_end.vy == 2130);
    CHECK(trace.last_end.vz == 3130);
    CHECK(trace.last_start.vx == 1116);
    CHECK(trace.last_start.vy == 2115);
    CHECK(trace.last_start.vz == 3114);
    CHECK(fixture.player.pFlags ==
          UINT32_C(0x4000));

    jpb_FxSetScreenGlowHook(NULL, NULL);
}

static void test_absorb_reflect_callback(void)
{
    ForceFixture fixture;
    int32_t cpad[2] = {0, INT32_C(0x20)};
    int frame;

    reset_fixture(&fixture);
    fixture.current_sequence.pMotion =
        &fixture.motions[41];
    fixture.animation->pCurrentAnimSeq =
        &fixture.current_sequence;
    fixture.player.currentMotion = 41;
    fixture.player.fLife = 0;
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.aCharacterData[0].MaxEnergy = 100;

    CHECK(force_AbsorbReflectCallBack(
              cpad, &fixture.player) == 0);
    CHECK(fixture.player.forceData[0] == 1);
    CHECK(fixture.player.forceData[2] == 1);
    CHECK(fixture.player.fScale == INT32_C(0x36d8));
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x51)) == UINT32_C(0x51));
    CHECK((fixture.motions[41].motionFlags &
           UINT32_C(0x80000000)) != 0);

    for (frame = 1; frame < 17; ++frame) {
        CHECK(force_AbsorbReflectCallBack(
                  cpad, &fixture.player) == 0);
    }
    CHECK(fixture.player.forceData[0] == 0);
    CHECK(GameStruct.aCharacterData[0].Force == 99);

    cpad[1] = 0;
    CHECK(force_AbsorbReflectCallBack(
              cpad, &fixture.player) == 1);
    CHECK(fixture.player.fScale == INT32_C(0x6db));
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x51)) == 0);
    CHECK((fixture.motions[41].motionFlags &
           UINT32_C(0x80000000)) == 0);

    GameStruct.aCharacterData[0].Energy = 0;
    cpad[1] = INT32_C(0x20);
    CHECK(force_AbsorbReflectCallBack(
              cpad, &fixture.player) == 1);
}

static void test_plasma_zap(void)
{
    _plasma_zapvars pzv;
    GlowTrace trace;
    VECTOR start = {0, 0, 0, 0};
    VECTOR end = {100, 200, 300, 0};
    int old_frame_rate = gGlobalFrameRate;

    memset(&pzv, 0, sizeof(pzv));
    memset(&trace, 0, sizeof(trace));
    pzv.inited = 1;
    gGlobalFrameRate = 4096;
    srand(1);
    jpb_FxSetScreenGlowHook(
        trace_screen_glow, &trace);

    fx_PlasmaZap(
        &pzv,
        &start,
        &end,
        UINT32_C(0x11223344),
        UINT32_C(0x55667788),
        0x800);
    CHECK(trace.calls == 9);
    CHECK(trace.first_start.vx == 0);
    CHECK(trace.first_start.vy == 0);
    CHECK(trace.first_start.vz == 0);
    CHECK(trace.first_end.vx == 10);
    CHECK(trace.first_end.vy == 20);
    CHECK(trace.first_end.vz == 30);
    CHECK(trace.last_start.vx == 80);
    CHECK(trace.last_start.vy == 160);
    CHECK(trace.last_start.vz == 240);
    CHECK(trace.last_end.vx == 100);
    CHECK(trace.last_end.vy == 200);
    CHECK(trace.last_end.vz == 300);
    CHECK(trace.width == 0x38);
    CHECK(trace.color == UINT32_C(0x55667788));

    jpb_FxSetScreenGlowHook(NULL, NULL);
    gGlobalFrameRate = old_frame_rate;
}

static void test_force_zap_callback(void)
{
    ForceFixture fixture;
    sceneObject target_scene;
    objectRoot target_scene_component;
    playerObject target_player;
    Mnode target_body;
    GlowTrace trace;

    reset_fixture(&fixture);
    memset(&target_scene, 0, sizeof(target_scene));
    memset(
        &target_scene_component,
        0,
        sizeof(target_scene_component));
    memset(&target_player, 0, sizeof(target_player));
    memset(&target_body, 0, sizeof(target_body));
    memset(&trace, 0, sizeof(trace));
    memset(clippingfrustrum, 0, sizeof(clippingfrustrum));

    fixture.current_sequence.pAnimTemplate =
        &fixture.templates[0];
    fixture.animation->pCurrentAnimSeq =
        &fixture.current_sequence;
    fixture.templates[0].Fframe = 0;
    fixture.animation->animFrameIndex =
        19 << JPB_FIXED_SHIFT;
    fixture.player.playerID = 0;
    fixture.nodes[13].v3RotCenter.vx = 0;
    fixture.nodes[13].v3RotCenter.vy = 0;
    fixture.nodes[13].v3RotCenter.vz = -100;
    fixture.nodes[15].v3RotCenter.vx = 0;
    fixture.nodes[15].v3RotCenter.vy = 0;
    fixture.nodes[15].v3RotCenter.vz = 0;

    target_player.playerRoot.objectID = 2;
    target_player.playerRoot.pParent =
        &target_scene.sceneRoot;
    target_scene.sceneRoot.objectID = 2;
    target_scene.pScene = &target_scene_component;
    target_scene.pPhysics =
        &maPhysicsData[2].physicsRoot;
    target_scene.pPlayer = &target_player.playerRoot;
    maPhysicsData[2].physicsRoot.objectID = 2;
    maPhysicsData[2].physicsRoot.pParent =
        &target_scene.sceneRoot;
    maPhysicsData[2].height = 32;
    target_body.id = (modelNodeId)(NODE_DYNAMIC | 0);
    target_body.v3RotCenter.vz = 400;
    coll_ResetPlayerCollision(2);
    coll_gRegisterNode(2, &target_body);

    GameStruct.aCharacterData[0].Force = 100;
    GameStruct.aCharacterData[0].MaxForce = 100;
    jpb_FxSetScreenGlowHook(
        trace_screen_glow, &trace);

    CHECK(force_ZapCallBack(
              NULL, &fixture.player) == 0);
    CHECK(GameStruct.aCharacterData[0].Force == 80);
    CHECK(trace.calls == 4);
    CHECK(trace.first_start.vz == 0);
    CHECK(trace.first_end.vz == 768);
    CHECK(trace.last_start.vz == 0);
    CHECK(trace.last_end.vz == 768);
    CHECK(trace.width == 0x20);
    CHECK(trace.color == UINT32_C(0x00ff8000));
    CHECK(target_player.whohitme == &fixture.player);
    CHECK(target_player.hitNumber == 1);
    CHECK(target_player.projectile ==
          &((ProjType *)(void *)maProjTypes)[5]);

    GameStruct.aCharacterData[0].Force = 4;
    CHECK(force_ZapCallBack(
              NULL, &fixture.player) == 1);
    CHECK(trace.calls == 4);

    jpb_FxSetScreenGlowHook(NULL, NULL);
}

static void test_force_reflect_callback(void)
{
    ForceFixture fixture;
    CylinderTrace trace;
    int32_t cpad[2] = {0, 0};
    int old_frame_rate = gGlobalFrameRate;
    int old_surface = mDrawingSurfaceId;

    reset_fixture(&fixture);
    memset(&trace, 0, sizeof(trace));
    fixture.current_sequence.pAnimTemplate =
        &fixture.templates[0];
    fixture.current_sequence.pMotion =
        &fixture.motions[0];
    fixture.animation->pCurrentAnimSeq =
        &fixture.current_sequence;
    fixture.motions[0].motionFlags =
        UINT32_C(0x80000000);
    fixture.player.forceData[0] = 1;
    fixture.player.forceData[1] = 2;
    fixture.player.forceData[2] = 3;
    fixture.player.forceData[3] = 4;

    CHECK(force_ReflectCallBack(
              cpad, &fixture.player) == 1);
    CHECK(fixture.motions[0].motionFlags == 0);
    CHECK(fixture.motions[0].Delay == 1);
    CHECK(fixture.player.forceData[0] == 0);
    CHECK(fixture.player.forceData[3] == 0);
    CHECK(fixture.player.fScale == INT32_C(0x6db));
    CHECK(fixture.physics.mass == 0x800);

    gGlobalFrameRate = JPB_FIXED_ONE;
    mDrawingSurfaceId = 1;
    GameStruct.aCharacterData[0].Force = 100;
    GameStruct.aCharacterData[0].MaxForce = 100;
    cpad[1] = INT32_C(0x20);
    jpb_SpriteSetCylinderHook(
        trace_cylinder, &trace);

    CHECK(force_ReflectCallBack(
              cpad, &fixture.player) == 0);
    CHECK(trace.calls == 16);
    CHECK(trace.location == &fixture.physics.vpos);
    CHECK(trace.radius1 == 0.0f);
    CHECK(trace.radius2 == 0.0f);
    CHECK(trace.h1 == 0.0f);
    CHECK(trace.h2 == 0.0f);
    CHECK(GameStruct.aCharacterData[0].Force == 99);
    CHECK(fixture.player.forceData[3] == 1);
    CHECK(fixture.player.fScale == INT32_C(0x36d8));
    CHECK(fixture.physics.mass == 0x2000);
    CHECK(fixture.motions[0].Delay == 5);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x52)) == UINT32_C(0x52));

    CHECK(force_ReflectCallBack(
              cpad, &fixture.player) == 0);
    CHECK(trace.calls == 32);
    CHECK(trace.rotation.vy == 0x20);
    CHECK(trace.radius1 == 30.0f);
    CHECK(trace.radius2 == 32.0f);
    CHECK(trace.h1 == 11.0f);
    CHECK(trace.h2 == 0.0f);
    CHECK(GameStruct.aCharacterData[0].Force == 98);

    cpad[1] = 0;
    CHECK(force_ReflectCallBack(
              cpad, &fixture.player) == 1);
    CHECK(trace.calls == 32);
    CHECK(fixture.player.fScale == INT32_C(0x6db));
    CHECK(fixture.physics.mass == 0x800);
    CHECK(fixture.motions[0].Delay == 1);
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x52)) == 0);
    CHECK(fixture.player.forceData[3] == 0);

    jpb_SpriteSetCylinderHook(NULL, NULL);
    gGlobalFrameRate = old_frame_rate;
    mDrawingSurfaceId = old_surface;
}

static void test_shield_callback(void)
{
    ForceFixture fixture;

    reset_fixture(&fixture);
    GameStruct.aCharacterData[0].Items = 2;

    CHECK(force_ShieldCallBack(
              NULL, &fixture.player) == 0);
    CHECK(fixture.player.forceData[0] == 1);
    CHECK(fixture.player.forceData[1] == 599);
    CHECK(GameStruct.aCharacterData[0].Items == 1);
    CHECK(fixture.player.fScale == INT32_C(0x2922));
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x52)) == UINT32_C(0x52));

    fixture.player.forceData[1] = 0;
    CHECK(force_ShieldCallBack(
              NULL, &fixture.player) == 1);
    CHECK(fixture.player.forceData[1] == -1);
    CHECK(fixture.player.fScale == INT32_C(0x6db));
    CHECK((fixture.player.forceFlags &
           UINT32_C(0x52)) == 0);
}

static void test_star_callback(void)
{
    ForceFixture fixture;
    GlowTrace trace;

    reset_fixture(&fixture);
    memset(&trace, 0, sizeof(trace));
    GameStruct.aCharacterData[0].Items = 2;
    fixture.player.forceFlags =
        UINT32_C(0x100);
    prepare_glowing_man_hierarchy(&fixture);
    jpb_FxSetScreenGlowHook(
        trace_screen_glow, &trace);

    CHECK(force_StarCallBack(
              NULL, &fixture.player) == 0);
    CHECK(fixture.player.forceData[0] == 1);
    CHECK(fixture.player.forceData[1] == 599);
    CHECK(GameStruct.aCharacterData[0].Items == 1);
    CHECK(fixture.player.forceFlags ==
          UINT32_C(0x160));
    CHECK(trace.calls == 2);
    CHECK(trace.first_width == 48);
    CHECK(trace.width == 54);
    CHECK(trace.first_color ==
          UINT32_C(0x00302010));
    CHECK(trace.color ==
          UINT32_C(0xc0482814));

    fixture.player.forceData[1] = 0;
    CHECK(force_StarCallBack(
              NULL, &fixture.player) == 1);
    CHECK(fixture.player.forceData[1] == -1);
    CHECK(fixture.player.forceFlags ==
          UINT32_C(0x100));

    jpb_FxSetScreenGlowHook(NULL, NULL);
}

int main(void)
{
    test_color_interpolation();
    test_primary_force_sequences();
    test_chained_force_sequences();
    test_persistent_force_callbacks();
    test_healing_callback();
    test_cloak_callback();
    test_mesmerize_callback();
    test_attack_callback();
    test_attack_spin_callback();
    test_sabre_spin_callback();
    test_sabre_toss_callback();
    test_sabre_yoyo_callback();
    test_plasma_zap();
    test_force_zap_callback();
    test_force_reflect_callback();
    test_absorb_reflect_callback();
    test_shield_callback();
    test_star_callback();
    test_screen_glow_fv_conversion();

    if (failures != 0) {
        fprintf(
            stderr,
            "%d force sequence test(s) failed\n",
            failures);
        return 1;
    }
    puts("force sequence tests passed");
    return 0;
}
