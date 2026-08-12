#include "jpb/brain.h"
#include "jpb/alloc.h"
#include "jpb/anim.h"
#include "jpb/brainutl.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/game.h"
#include "jpb/jedi.h"
#include "jpb/jonny.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/world.h"

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

static void connect_player(
    playerObject *player, sceneObject *scene, physicsObject *physics)
{
    memset(player, 0, sizeof(*player));
    memset(scene, 0, sizeof(*scene));
    memset(physics, 0, sizeof(*physics));
    player->playerRoot.pParent = &scene->sceneRoot;
    physics->physicsRoot.pParent = &scene->sceneRoot;
    scene->pPhysics = &physics->physicsRoot;
}

static int test_fall_and_jump_selection(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;

    connect_player(&player, &scene, &physics);
    player.pSettings.JumpVel = -101;
    player.pSettings.RunningJumpVel = 222;
    player.pSettings.JumpAngle = 0x333;
    player.pSettings.RunningJumpAngle = 0x444;
    player.pSettings.dblJumpAngle = 0x555;

    player.playerRoot.objectID = 0;
    brain_SetFallTrajectory(&player, 1);
    CHECK(player.airVelocity == -25);
    CHECK(player.airAngle == 0x200);
    player.playerRoot.objectID = 2;
    brain_SetFallTrajectory(&player, 0);
    CHECK(player.airVelocity == -50);
    CHECK(player.airAngle == 0x555);

    physics.reversoi = 123;
    player.pFlags = 0x00400100u;
    brain_SetJumpTrajectory(&player, 0);
    CHECK(physics.reversoi == 0);
    CHECK((player.pFlags & 0x00400000u) == 0);
    CHECK(player.airVelocity == -101);
    CHECK(player.airAngle == 0x333);

    player.pFlags = 0;
    brain_SetJumpTrajectory(&player, 0);
    CHECK(player.airVelocity == 222);
    CHECK(player.airAngle == 0x444);
    brain_SetJumpTrajectory(&player, 1);
    CHECK(player.airVelocity == -101);
    CHECK(player.airAngle == 0x400);
    return 0;
}

static int test_air_trajectory(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;

    connect_player(&player, &scene, &physics);
    physics.airTime = 50;
    physics.realAirTime = 60;
    physics.mov.vy = 0.0f;
    player.pFlags = 0xc4000000u;
    brain_SetTrajectory(&player, 100, 0);
    CHECK(physics.trajectory == 0);
    CHECK(physics.airspeed == 100);
    CHECK(physics.airmov.vx == 0.0f);
    CHECK(physics.airmov.vy == 0.0f);
    CHECK(physics.airmov.vz == 100.0f);
    CHECK(physics.airTime == 0);
    CHECK(physics.realAirTime == 0);
    CHECK(player.pFlags == 0x80000001u);

    physics.airTime = 70;
    physics.realAirTime = 80;
    physics.mov.vy = 1.0f;
    brain_SetTrajectory(&player, 100, 1024);
    CHECK(physics.airmov.vy == 99.0f);
    CHECK(physics.airmov.vz == 0.0f);
    CHECK(physics.airTime == 70);
    CHECK(physics.realAirTime == 80);

    physics.trajectory = 0;
    physics.airspeed = 50;
    brain_SetTrajectory(&player, -1, -1);
    CHECK(physics.trajectory == 0);
    CHECK(physics.airspeed == 50);
    CHECK(physics.airmov.vz == 50.0f);
    return 0;
}

static int test_velocity_axis_callback(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;

    connect_player(&player, &scene, &physics);
    physics.constmov.vx = 3.0f;
    physics.constmov.vy = 4.0f;
    physics.constmov.vz = 5.0f;
    CHECK(brain_SwapVelDirCallBack(NULL, &player) == 1);
    CHECK(physics.constmov.vx == 5.0f);
    CHECK(physics.constmov.vy == 4.0f);
    CHECK(physics.constmov.vz == 3.0f);

    physics.constmov.vx = 7.0f;
    physics.constmov.vz = 0.0f;
    CHECK(brain_SwapVelDirCallBack(NULL, &player) == 1);
    CHECK(physics.constmov.vx == 7.0f);
    CHECK(physics.constmov.vz == 0.0f);
    return 0;
}

static int test_bounded_ground_direction(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[27];
    Motion motions[27];

    connect_player(&player, &scene, &physics);
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    memset(templates, 0, sizeof(templates));
    memset(motions, 0, sizeof(motions));
    player.playerRoot.objectID = 0;
    player.paMotions = motions;
    player.maxMotions = 27;
    scene.pPlayer = &player.playerRoot;
    scene.pAnim = &animation->animRoot;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    motions[1].Seq = 1;
    motions[1].Lock = 1;
    motions[1].Speed = 0;
    motions[1].vel = 27;
    motions[8].Seq = 8;
    motions[8].Lock = 1;
    motions[26].Seq = 26;
    motions[26].Lock = 1;
    gGlobalFrameRate = JPB_FIXED_ONE;
    gSCENE_READY = 0;

    CHECK(jpb_BrainDirectionAngle(
              0.0f, 1.0f, 0) == 4096);
    CHECK(jpb_BrainDirectionAngle(
              1.0f, 0.0f, 0) == 5120);
    CHECK(jpb_BrainGroundDirectionState(
              &player, 1.0f, 0.0f, 0) ==
          JPB_BRAIN_PARTIAL_OK);
    CHECK(physics.angle.vy == 512);
    CHECK(physics.constmov.vz == 27.0f);
    CHECK(motions[1].vel == 0x15);
    CHECK(player.currentMotion == 1);
    CHECK(animation->pMotion == &motions[1]);
    CHECK(jpb_BrainGroundDirectionState(
              &player, 1.0f, 0.0f, 0) ==
          JPB_BRAIN_PARTIAL_NO_CHANGE);

    player.playerID = 2;
    animation->Lock = 0;
    player.currentMotion = 0;
    motions[1].vel = 27;
    CHECK(jpb_BrainGroundDirectionState(
              &player, 1.0f, 0.0f, 0) ==
          JPB_BRAIN_PARTIAL_OK);
    CHECK(motions[1].vel == 27);

    animation->Lock = 0;
    player.currentMotion = 0;
    player.pFlags = 0x10;
    physics.angle.vy = 512;
    physics.vmov.vx = 1;
    CHECK(jpb_BrainGroundDirectionState(
              &player, 0.0f, 1.0f, 0) ==
          JPB_BRAIN_PARTIAL_OK);
    CHECK(physics.angle.vy == 256);
    CHECK((physics.flags & 0x00001000u) != 0);

    animation->Lock = 0;
    player.currentMotion = 0;
    player.pFlags = 0x00400000u;
    CHECK(jpb_BrainGroundDirectionState(
              &player, 0.0f, 1.0f, 0) ==
          JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 26);
    CHECK(physics.angle.vy == 4096);
    CHECK(motions[26].Speed == 0x24);
    CHECK(motions[26].frzin == 0);
    CHECK(motions[26].frzout == 0);
    CHECK(motions[26].twin == 2);
    CHECK(motions[26].twout == 2);

    animation->Lock = 0;
    player.currentMotion = 0;
    physics.angle.vy = 4096;
    CHECK(jpb_BrainGroundDirectionState(
              &player, 0.0f, -1.0f, 0) ==
          JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 8);
    CHECK(physics.angle.vy == 2084);
    return 0;
}

static int test_bounded_ground_idle(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[26];
    Motion motions[26];
    Motion *published_motion;
    int index;

    connect_player(&player, &scene, &physics);
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    memset(templates, 0, sizeof(templates));
    memset(motions, 0, sizeof(motions));
    player.playerRoot.objectID = 0;
    player.paMotions = motions;
    player.maxMotions = 26;
    scene.pPlayer = &player.playerRoot;
    scene.pAnim = &animation->animRoot;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    for (index = 0; index < 26; ++index) {
        motions[index].Seq = (uint16_t)index;
        motions[index].Lock = 1;
        motions[index].vel = (int16_t)(index + 1);
    }
    gGlobalFrameRate = JPB_FIXED_ONE;
    gSCENE_READY = 0;

    player.currentMotion = 1;
    player.runCounter = 7;
    player.hitDelay = 42;
    physics.constmov.vx = 3.0f;
    physics.constmov.vy = 4.0f;
    physics.constmov.vz = 5.0f;
    published_motion = &motions[0];
    player.pMotion = &published_motion;
    motions[0].motionFlags = 1;
    player.pFlags = 0x01300010u;
    CHECK(jpb_BrainGroundIdleState(
              &player, 26) == JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 0);
    CHECK(player.previousMotion == 1);
    CHECK(player.runCounter == 0);
    CHECK(player.hitDelay == 0);
    CHECK(physics.constmov.vx == 0.0f);
    CHECK(physics.constmov.vy == 0.0f);
    CHECK(physics.constmov.vz == 0.0f);
    CHECK(player.pFlags ==
          (0x01300010u & 0xfcdfffefu));

    animation->Lock = 0;
    player.currentMotion = 1;
    player.pMotion = NULL;
    player.pFlags = 0;
    CHECK(jpb_BrainGroundIdleState(
              &player, 25) == JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 19);
    CHECK(animation->pMotion == &motions[19]);

    animation->Lock = 0;
    player.currentMotion = 1;
    player.pFlags = 0x00400000u;
    CHECK(jpb_BrainGroundIdleState(
              &player, 100) == JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 20);
    CHECK(animation->pMotion == &motions[20]);

    animation->Lock = 0;
    player.currentMotion = 2;
    player.runCounter = 16;
    player.hitDelay = 99;
    player.pFlags = 0x00400000u;
    motions[25].FunctPtr = 0;
    CHECK(jpb_BrainGroundIdleState(
              &player, 100) == JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 25);
    CHECK(player.runCounter == 16);
    CHECK(player.hitDelay == 0);
    CHECK(motions[25].FunctPtr == 4);
    CHECK((player.pFlags & 0x00400000u) == 0);
    CHECK(animation->pMotion == &motions[25]);
    return 0;
}

static int test_bounded_lock_on_direction(void)
{
    playerObject player;
    playerObject target;
    sceneObject scene;
    sceneObject target_scene;
    physicsObject physics;
    physicsObject target_physics;
    animObject *animation;
    _animTemplate templates[31];
    Motion motions[31];
    int index;

    connect_player(&player, &scene, &physics);
    connect_player(
        &target, &target_scene, &target_physics);
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    memset(templates, 0, sizeof(templates));
    memset(motions, 0, sizeof(motions));
    player.target = &target;
    player.paMotions = motions;
    player.maxMotions = 31;
    player.pFlags = UINT32_C(0x00400000);
    scene.pPlayer = &player.playerRoot;
    scene.pAnim = &animation->animRoot;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    for (index = 0; index < 31; ++index) {
        motions[index].Seq = (uint16_t)index;
        motions[index].Lock = 1;
    }
    target_physics.vpos.vx = 100;
    gGlobalFrameRate = JPB_FIXED_ONE;
    gSCENE_READY = 0;

    physics.angle.vy = 1000;
    player.runCounter = 9;
    CHECK(jpb_BrainLockOnDirectionState(
              &player, 1000) == JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 26);
    CHECK(player.runCounter == 1);
    CHECK(physics.angle.vy == 1023);
    CHECK(motions[26].Speed == 0x24);
    CHECK(motions[26].frzin == 0);
    CHECK(motions[26].frzout == 0);
    CHECK(motions[26].twin == 2);
    CHECK(motions[26].twout == 2);

    animation->Lock = 0;
    player.currentMotion = 0;
    physics.angle.vy = 0;
    CHECK(jpb_BrainLockOnDirectionState(
              &player, 1000) == JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 29);
    CHECK((motions[29].motionFlags & UINT32_C(8)) != 0);

    animation->Lock = 0;
    player.currentMotion = 0;
    physics.angle.vy = 2000;
    CHECK(jpb_BrainLockOnDirectionState(
              &player, 1000) == JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 30);
    CHECK((motions[30].motionFlags & UINT32_C(8)) != 0);

    animation->Lock = 0;
    player.currentMotion = 0;
    physics.angle.vy = 2600;
    CHECK(jpb_BrainLockOnDirectionState(
              &player, 1000) == JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 12);
    return 0;
}

static int test_bounded_special_direction(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[61];
    Motion motions[61];
    int index;

    connect_player(&player, &scene, &physics);
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    memset(templates, 0, sizeof(templates));
    memset(motions, 0, sizeof(motions));
    player.playerRoot.objectID = 0;
    player.paMotions = motions;
    player.maxMotions = 61;
    scene.pPlayer = &player.playerRoot;
    scene.pAnim = &animation->animRoot;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    for (index = 0; index < 61; ++index) {
        motions[index].Seq = (uint16_t)index;
        motions[index].Lock = 1;
    }
    gGlobalFrameRate = JPB_FIXED_ONE;
    gSCENE_READY = 0;

    player.currentMotion = 0x3c;
    animation->Lock = 30;
    CHECK(jpb_BrainGroundSpecialDirectionState(
              &player, 26) == JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 0);
    CHECK(player.previousMotion == 0x3c);
    CHECK(animation->pMotion == &motions[0]);

    animation->Lock = 30;
    player.currentMotion = 2;
    player.runCounter = 16;
    player.pFlags = 0x00400000u;
    CHECK(jpb_BrainGroundSpecialDirectionState(
              &player, 26) == JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 25);
    CHECK(motions[25].FunctPtr == 4);
    CHECK((player.pFlags & 0x00400000u) == 0);

    player.currentMotion = 1;
    CHECK(jpb_BrainGroundSpecialDirectionState(
              &player, 26) ==
          JPB_BRAIN_PARTIAL_UNSUPPORTED_STATE);
    return 0;
}

static int test_attack_transition(void)
{
    char old_level = LevelSelect;
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[26];
    Motion motions[26];
    animListNode *queued;
    int index;

    connect_player(&player, &scene, &physics);
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    memset(templates, 0, sizeof(templates));
    memset(motions, 0, sizeof(motions));
    player.playerRoot.objectID = 0;
    player.paMotions = motions;
    player.maxMotions = 26;
    scene.pPlayer = &player.playerRoot;
    scene.pAnim = &animation->animRoot;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    for (index = 0; index < 26; ++index) {
        motions[index].Seq = (uint16_t)index;
        motions[index].Lock = 1;
    }
    gGlobalFrameRate = JPB_FIXED_ONE;
    gSCENE_READY = 0;
    LevelSelect = 1;

    player.currentMotion = 1;
    player.pFlags = UINT32_C(0x10);
    motions[15].motionFlags = UINT32_C(0x84000008);
    motions[15].disp = UINT8_C(7);
    CHECK(jpb_BrainGroundAttackState(&player) ==
          JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 15);
    CHECK((player.pFlags & UINT32_C(0x20)) != 0);
    CHECK((player.pFlags & UINT32_C(0x10)) == 0);
    CHECK(motions[15].motionFlags == UINT32_C(8));
    CHECK(motions[15].disp == 0);
    queued = (animListNode *)animation->animList.tail;
    CHECK(queued != NULL);
    CHECK(queued->pMotion == &motions[21]);

    animation->Lock = 0;
    player.currentMotion = 2;
    player.runCounter = 16;
    player.hitDelay = 99;
    player.pFlags = UINT32_C(0x00400000);
    motions[25].FunctPtr = 0;
    CHECK(jpb_BrainGroundAttackState(&player) ==
          JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 25);
    CHECK(player.hitDelay == 0);
    CHECK(motions[25].FunctPtr == 4);
    CHECK((player.pFlags & UINT32_C(0x00400000)) == 0);

    animation->Lock = 0;
    player.currentMotion = 2;
    player.runCounter = 16;
    player.hitDelay = 99;
    physics.constmov.vx = 3.0f;
    physics.constmov.vy = 4.0f;
    physics.constmov.vz = 5.0f;
    LevelSelect = 13;
    CHECK(jpb_BrainGroundAttackState(&player) ==
          JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 0);
    CHECK(player.hitDelay == 0);
    CHECK(physics.constmov.vx == 0.0f);
    CHECK(physics.constmov.vy == 0.0f);
    CHECK(physics.constmov.vz == 0.0f);

    LevelSelect = old_level;
    return 0;
}

typedef struct StopTrace {
    uint16_t handles[2];
    int count;
} StopTrace;

typedef struct PlayTrace {
    VECTOR *position;
    int bank;
    char sound[9];
    uint32_t flag;
    int count;
} PlayTrace;

static uint16_t trace_play_sound(
    VECTOR *position,
    int bank,
    char *sound,
    uint32_t flag,
    void *user_data)
{
    PlayTrace *trace = (PlayTrace *)user_data;

    trace->position = position;
    trace->bank = bank;
    memcpy(trace->sound, sound, 8);
    trace->sound[8] = '\0';
    trace->flag = flag;
    ++trace->count;
    return UINT16_C(1);
}

static void trace_stop_sound(uint16_t handle, void *user_data)
{
    StopTrace *trace = (StopTrace *)user_data;

    if (trace->count < 2) {
        trace->handles[trace->count] = handle;
    }
    ++trace->count;
}

static int test_effect_and_ring_control(void)
{
    playerObject player;
    playerObject target;
    sceneObject scene;
    sceneObject target_scene;
    physicsObject physics;
    physicsObject target_physics;
    modelObject model;
    Motion motion;
    Motion *current_motion = &motion;
    PlayTrace play_trace;
    EffectHeader ring_on;
    EffectHeader ring_off;
    RingData *ring_data;
    Ring *active_ring;
    CVECTOR colour;

    connect_player(&player, &scene, &physics);
    connect_player(
        &target, &target_scene, &target_physics);
    memset(&model, 0, sizeof(model));
    memset(&motion, 0, sizeof(motion));
    memset(&play_trace, 0, sizeof(play_trace));
    scene.pModel = &model.modelRoot;
    player.pMotion = &current_motion;
    player.playernum = 1;
    model.eventMask = UINT32_C(5);
    memcpy(motion.snd[1], "effects", 8);
    motion.sndDelay[1] = 7;
    maPhysicsData[1].vpos.vx = 10;
    maPhysicsData[1].vpos.vy = 20;
    maPhysicsData[1].vpos.vz = 30;

    CHECK(brainutl_FindLSB_LV(0) == 0);
    CHECK(brainutl_FindLSB_LV(1) == 1);
    CHECK(brainutl_FindLSB_LV(UINT32_C(0x80000000)) == 32);
    jpb_SoundSetPlaySfxHook(
        trace_play_sound, &play_trace);
    brain_CheckForEffects(&player);
    CHECK(play_trace.count == 2);
    CHECK(play_trace.position == &maPhysicsData[1].vpos);
    CHECK(play_trace.bank == 2);
    CHECK(strcmp(play_trace.sound, "effects") == 0);
    CHECK(play_trace.flag == 0);
    CHECK(model.eventMask == UINT32_C(5));

    motion.Damage = 1;
    brain_CheckForEffects(&player);
    CHECK(play_trace.count == 2);
    motion.Damage = 0;
    motion.snd[1][0] = '0';
    brain_CheckForEffects(&player);
    CHECK(play_trace.count == 2);
    maPhysicsData[4].vpos.vx = 40;
    brainutl_PlayMotionSound(4, "direct", 99);
    CHECK(play_trace.count == 3);
    CHECK(play_trace.position == &maPhysicsData[4].vpos);
    CHECK(play_trace.bank == 3);
    brainutl_PlayMotionSound(0, NULL, 0);
    brainutl_PlayMotionSound(0, "", 0);
    brainutl_PlayMotionSound(0, "0disabled", 0);
    CHECK(play_trace.count == 3);
    jpb_SoundSetPlaySfxHook(NULL, NULL);

    meminit();
    sprite_gInitSprites();
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    memset(&ring_on, 0, sizeof(ring_on));
    memset(&ring_off, 0, sizeof(ring_off));
    ring_on.num = 1;
    ring_off.num = 1;
    ring_data = (RingData *)(void *)&ring_on.aEffects[0];
    ring_data->bank = 4;
    ring_data->time = 100;
    ring_data = (RingData *)(void *)&ring_off.aEffects[0];
    ring_data->bank = 4;
    ring_data->time = 200;
    paEffects[44] = &ring_on;
    paEffects[63] = &ring_off;
    player.locked = &target;
    player.playerID = 0;
    target_physics.vpos.vx = 100;
    target_physics.vpos.vy = 200;
    target_physics.vpos.vz = 300;
    gGlobalTimer = 50;

    brain_DoRingOnEffect(&player);
    active_ring = (Ring *)(void *)player.lockRing;
    CHECK(active_ring != NULL);
    CHECK(target_physics.vpos.vy == 202);
    CHECK(active_ring->pos.vx == 100);
    CHECK(active_ring->pos.vy == 202);
    CHECK(active_ring->pos.vz == 300);
    CHECK(active_ring->time == 150);
    CHECK(((uint16_t)active_ring->rot.pad &
           UINT16_C(0x20)) != 0);
    colour = jedi_GetColour(0);
    CHECK(active_ring->pos.pad == colour.cd);
    CHECK(colour.cd == UINT8_C(0x1b));

    brain_DoRingOffEffect(&player);
    CHECK(target_physics.vpos.vy == 204);
    CHECK(active_ring->time == 0);
    CHECK(player.lockRing == NULL);
    colour = jedi_GetColour(9);
    CHECK(colour.r == UINT8_C(0xff));
    CHECK(colour.g == UINT8_C(0xff));
    CHECK(colour.b == UINT8_C(0xff));
    CHECK(colour.cd == 0);
    paEffects[44] = NULL;
    paEffects[63] = NULL;
    return 0;
}

static int test_ground_control(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    _animTemplate templates[15];
    animListNode current_node;
    Motion motions[15];
    wsl_ENEMY enemy;
    StopTrace stop_trace;
    int32_t cpad[2] = {0, 0};
    uint32_t timer;
    int index;

    connect_player(&player, &scene, &physics);
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    memset(templates, 0, sizeof(templates));
    memset(&current_node, 0, sizeof(current_node));
    memset(motions, 0, sizeof(motions));
    memset(&enemy, 0, sizeof(enemy));
    memset(&stop_trace, 0, sizeof(stop_trace));
    player.playerRoot.objectID = 0;
    player.playernum = 0;
    player.paMotions = motions;
    player.maxMotions = 15;
    player.pEnemy = &enemy;
    scene.pScene = &scene.sceneRoot;
    scene.pPlayer = &player.playerRoot;
    scene.pAnim = &animation->animRoot;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    animation->pCurrentAnimSeq = &current_node;
    current_node.pAnimTemplate = &templates[3];
    templates[3].Fframe = 4;
    templates[3].Lframe = 13;
    for (index = 0; index < 15; ++index) {
        motions[index].Seq = (uint16_t)index;
        motions[index].Lock = 1;
    }
    gGlobalFrameRate = JPB_FIXED_ONE;
    gSCENE_READY = 0;

    gGlobalTimer = UINT32_C(0x10000);
    timer = gGlobalTimer;
    GameStruct.aCharacterData[0].Energy = 0;
    CHECK(brain_GroundControl(
              cpad, &player, NULL) == 0);
    CHECK((player.pFlags & UINT32_C(0x200)) != 0);
    CHECK(player.groundDelay ==
          timer + UINT32_C(0x12c00));

    player.pFlags = 0;
    player.playernum = 2;
    GameStruct.aCharacterData[2].Energy = 0;
    CHECK(brain_GroundControl(
              cpad, &player, NULL) == 0);
    CHECK((player.pFlags & UINT32_C(0x200)) != 0);
    CHECK(player.groundDelay ==
          timer + UINT32_C((9 + 6) * 0x200));

    player.pFlags = UINT32_C(0x00000c00);
    player.playernum = 0;
    player.fStun = 77;
    player.PreMotion[0] = 'X';
    player.hitDelay = timer;
    player.groundDelay = timer;
    GameStruct.aCharacterData[0].Energy = 10;
    CHECK(brain_GroundControl(
              cpad, &player, NULL) == 1);
    CHECK(player.fStun == 0);
    CHECK(player.PreMotion[0] == '\0');
    CHECK(player.currentMotion == 0);
    CHECK(player.groundDelay == timer);

    cpad[0] = 1;
    CHECK(brain_GroundControl(
              cpad, &player, NULL) == 0);
    CHECK(player.currentMotion == 14);
    CHECK((player.pFlags & UINT32_C(0x00000c00)) == 0);
    CHECK((motions[14].motionFlags &
           UINT32_C(0x02000000)) != 0);
    CHECK(player.groundDelay == 0);
    CHECK(player.hitDelay ==
          timer + UINT32_C(0x5000));

    player.pFlags = 0;
    player.hitDelay = timer + 1;
    player.groundDelay = timer + UINT32_C(0x10000);
    CHECK(brain_GroundControl(
              cpad, &player, NULL) == 1);
    CHECK(player.groundDelay ==
          timer + UINT32_C(0x5a00));

    scene.sceneRoot.flags = 0;
    player.pFlags = UINT32_C(0x200);
    player.playernum = 0;
    player.groundDelay = timer - 1;
    animation->loopHandle[0] = UINT16_C(11);
    animation->loopHandle[1] = UINT16_C(12);
    GameStruct.GameState = 0;
    jpb_SoundSetStopHook(
        trace_stop_sound, &stop_trace);
    CHECK(brain_GroundControl(
              cpad, &player, NULL) == 1);
    CHECK((scene.sceneRoot.flags &
           UINT32_C(0x20)) != 0);
    CHECK((GameStruct.GameState &
           UINT32_C(0x20)) != 0);
    CHECK((player.pFlags &
           UINT32_C(0x40000)) != 0);
    CHECK(stop_trace.count == 2);
    CHECK(stop_trace.handles[0] == 11);
    CHECK(stop_trace.handles[1] == 12);
    jpb_SoundSetStopHook(NULL, NULL);

    player.pFlags = UINT32_C(0x200);
    player.playernum = 2;
    player.groundDelay = timer - 1;
    enemy.exit_flag = 0;
    CHECK(brain_GroundControl(
              cpad, &player, NULL) == 1);
    CHECK(enemy.exit_flag == 1);
    return 0;
}

static int trace_trajectory_callback(
    int32_t *cpad, playerObject *player)
{
    (void)cpad;
    (void)player;
    return 1;
}

static int test_lock_on_lifecycle(void)
{
    playerObject player;
    playerObject candidate;
    sceneObject scene;
    sceneObject candidate_scene;
    physicsObject *physics;
    physicsObject *candidate_physics;
    modelObject candidate_model;
    EffectHeader ring_on;
    EffectHeader ring_off;
    RingData *ring_data;
    Ring *active_ring;
    PlayTrace play_trace;
    int32_t cpad[2] = {8, 0};

    physics_gInitObjects(0);
    physics = &maPhysicsData[0];
    candidate_physics = &maPhysicsData[2];
    connect_player(&player, &scene, physics);
    memset(&candidate, 0, sizeof(candidate));
    memset(&candidate_scene, 0, sizeof(candidate_scene));
    memset(&candidate_model, 0, sizeof(candidate_model));
    memset(&ring_on, 0, sizeof(ring_on));
    memset(&ring_off, 0, sizeof(ring_off));
    memset(&play_trace, 0, sizeof(play_trace));
    scene.pScene = &scene.sceneRoot;
    scene.pPlayer = &player.playerRoot;
    player.playerRoot.objectID = 0;
    player.playernum = 0;
    player.playerID = 0;
    candidate.playerRoot.objectID = 2;
    candidate.playerRoot.pParent =
        &candidate_scene.sceneRoot;
    candidate.playernum = 2;
    candidate_scene.pScene =
        &candidate_scene.sceneRoot;
    candidate_scene.pModel =
        &candidate_model.modelRoot;
    candidate_scene.pPhysics =
        &candidate_physics->physicsRoot;
    candidate_scene.pPlayer =
        &candidate.playerRoot;
    candidate_physics->physicsRoot.objectID = 2;
    candidate_physics->physicsRoot.pParent =
        &candidate_scene.sceneRoot;
    candidate_physics->pos.vy = 0.0f;
    candidate_physics->vpos.vx = 100;
    candidate_physics->vpos.vy = 200;
    candidate_physics->vpos.vz = 300;
    physics->physicsRoot.objectID = 0;
    physics->pos.vy = 0.0f;
    physics->vpos.vx = 0;
    physics->vpos.vy = 0;
    physics->vpos.vz = 0;
    GameStruct.versusModeFlag = 0;
    GameStruct.CurrentLevel = 0;
    GameStruct.aCharacterData[2].Energy = 10;
    maRange[0][2] = 100.0f;
    OptionStruct.ControllerConfig[0] = 0;
    LevelSelect = 0;

    meminit();
    sprite_gInitSprites();
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    ring_on.num = 1;
    ring_off.num = 1;
    ring_data =
        (RingData *)(void *)&ring_on.aEffects[0];
    ring_data->bank = 4;
    ring_data->time = 100;
    ring_data =
        (RingData *)(void *)&ring_off.aEffects[0];
    ring_data->bank = 4;
    ring_data->time = 200;
    paEffects[44] = &ring_on;
    paEffects[63] = &ring_off;
    jpb_SoundSetPlaySfxHook(
        trace_play_sound, &play_trace);

    CHECK(brain_LockOn(cpad, &player) == 0);
    CHECK((player.pFlags &
           UINT32_C(0x00400000)) != 0);
    CHECK(player.locked == &candidate);
    active_ring = (Ring *)(void *)player.lockRing;
    CHECK(active_ring != NULL);
    CHECK(candidate_physics->vpos.vy == 202);
    CHECK(play_trace.count == 1);
    CHECK(play_trace.position == &physics->vpos);
    CHECK(play_trace.bank == 0);
    CHECK(strcmp(play_trace.sound, "xlockon") == 0);

    gGlobalTimer = UINT32_C(1000);
    candidate_physics->pos.vx = 100.75f;
    candidate_physics->pos.vy = 200.5f;
    candidate_physics->pos.vz = 300.9f;
    candidate_physics->mov.vx = 1.5f;
    candidate_physics->mov.vy = -2.25f;
    candidate_physics->mov.vz = 3.75f;
    brain_ValidateLockOn(&player);
    CHECK(active_ring->time == UINT32_C(0x21e8));
    CHECK(active_ring->pos.vx == 101);
    CHECK(active_ring->pos.vy == 199);
    CHECK(active_ring->pos.vz == 303);
    CHECK(physics->angle.vy != 0);

    GameStruct.aCharacterData[2].Energy = 0;
    brain_ValidateLockOn(&player);
    CHECK((player.pFlags &
           UINT32_C(0x00400000)) == 0);
    CHECK(player.locked == NULL);
    CHECK(player.lockRing == NULL);
    CHECK(active_ring->time == 0);

    LevelSelect = 8;
    player.pFlags = 0;
    GameStruct.aCharacterData[2].Energy = 10;
    CHECK(brain_LockOn(cpad, &player) == 0);
    CHECK(player.locked == NULL);
    CHECK(player.pFlags == 0);
    jpb_SoundSetPlaySfxHook(NULL, NULL);
    paEffects[44] = NULL;
    paEffects[63] = NULL;
    return 0;
}

static int test_animation_end_callbacks(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    animObject *animation;
    modelObject model;
    animListNode current_node;
    _animTemplate templates[52];
    Motion motions[52];
    Mnode event_node;
    Sprite shadow;
    SCB shadow_scb;
    JPBPlayerCallback old_callback =
        jpb_TrajectoryCallbackSlot;
    unsigned seed;
    int index;

    connect_player(&player, &scene, &physics);
    anim_InitAnimations(0);
    animation = &maAnimationData[0];
    memset(&current_node, 0, sizeof(current_node));
    memset(templates, 0, sizeof(templates));
    memset(motions, 0, sizeof(motions));
    memset(&model, 0, sizeof(model));
    memset(&event_node, 0, sizeof(event_node));
    memset(&shadow, 0, sizeof(shadow));
    memset(&shadow_scb, 0, sizeof(shadow_scb));
    scene.pScene = &scene.sceneRoot;
    scene.pModel = &model.modelRoot;
    scene.pPlayer = &player.playerRoot;
    scene.pAnim = &animation->animRoot;
    animation->animRoot.pParent = &scene.sceneRoot;
    animation->depack_context.seqdata = templates;
    animation->pCurrentAnimSeq = &current_node;
    current_node.pAnimTemplate = &templates[0];
    templates[0].Lframe = 10;
    player.playerRoot.objectID = 0;
    player.playernum = 3;
    player.paMotions = motions;
    player.maxMotions = 52;
    for (index = 0; index < 52; ++index) {
        motions[index].Seq = (uint16_t)index;
        motions[index].Lock = 1;
    }
    gGlobalFrameRate = JPB_FIXED_ONE;
    gSCENE_READY = 0;

    coll_ResetCollisionSystem();
    event_node.id = NODE_DYNAMIC;
    event_node.flags = JPB_COLLISION_FLAG_EVENT;
    coll_gRegisterNode(3, &event_node);
    shadow.sp_SCB = &shadow_scb;
    shadow_scb.scb_flags = INT32_C(0xc0);
    player.shadow = (int32_t *)(void *)&shadow;
    player.pFlags = UINT32_C(0x40000000);
    player.currentMotion = 7;
    player.ACTION_LOCK = 9;
    CHECK(brain_HangCallback(NULL, &player) == 1);
    CHECK((player.pFlags &
           UINT32_C(0x40000000)) == 0);
    CHECK(shadow_scb.scb_flags == INT32_C(0x80));
    CHECK(player.currentMotion == 7);
    CHECK(player.ACTION_LOCK == 0);
    event_node.flags = 0;
    CHECK(brain_HangCallback(NULL, &player) == -1);

    for (seed = 0; seed < 1000; ++seed) {
        srand(seed);
        if (rand() % 100 >= 8) {
            break;
        }
    }
    CHECK(seed < 1000);
    animation->animFrameIndex = 11 << 12;
    CHECK(brain_SkidCallBack(NULL, &player) == 1);
    srand(seed);
    animation->animFrameIndex = 10 << 12;
    CHECK(brain_SkidCallBack(NULL, &player) == 0);

    player.airVelocity = 200;
    player.airAngle = 0x200;
    jpb_TrajectoryCallbackSlot =
        trace_trajectory_callback;
    animation->animFrameIndex = 9 << 12;
    CHECK(brain_TakeOff(
              NULL, &player, NULL) == 0);
    CHECK(player.pMotionCallBack == NULL);
    animation->animFrameIndex = 10 << 12;
    CHECK(brain_TakeOff(
              NULL, &player, NULL) == 0);
    CHECK((motions[4].motionFlags &
           UINT32_C(0x04000000)) != 0);
    CHECK(player.currentMotion == 4);
    CHECK(player.pMotionCallBack ==
          trace_trajectory_callback);
    CHECK(physics.trajectory == 0x200);
    CHECK(physics.airspeed == 200);

    animation->Lock = 0;
    animation->pCurrentAnimSeq = &current_node;
    current_node.pAnimTemplate = &templates[0];
    animation->animFrameIndex = 8 << 12;
    player.pSettings.JumpVel = -600;
    player.pSettings.bkJumpAngle = 0x300;
    player.fLife = 99;
    player.hitNumber = 7;
    player.delayedMotion = 44;
    gGlobalTimer = UINT32_C(0x1000);
    CHECK(brain_ThrowEnder(NULL, &player) == -1);
    CHECK(player.currentMotion == 51);
    CHECK(player.airVelocity == 200);
    CHECK(physics.airspeed == -300);
    CHECK(physics.trajectory == 0x300);
    CHECK(player.fLife == 0);
    CHECK(player.hitNumber == 0);
    CHECK(player.hitDelay == UINT32_C(0x1c00));
    CHECK(player.groundDelay == UINT32_C(0x3000));
    CHECK(player.delayedMotion == 0);
    jpb_TrajectoryCallbackSlot = old_callback;
    return 0;
}

static int test_bounded_jump_launch(void)
{
    int32_t map_materials[8] = {0};
    int32_t *old_leveldata = leveldata;
    int32_t poly = INT32_C(0x20007);
    sceneObject *scene;
    physicsObject *physics;
    animObject *animation;
    playerObject player;
    modelObject model;
    _animTemplate templates[5];
    Motion motions[5];
    int index;

    jpb_SceneInitPool(0);
    physics_gInitObjects(0);
    anim_InitAnimations(0);
    scene = &maSceneData[0];
    physics = &maPhysicsData[0];
    animation = &maAnimationData[0];
    memset(&player, 0, sizeof(player));
    memset(&model, 0, sizeof(model));
    memset(templates, 0, sizeof(templates));
    memset(motions, 0, sizeof(motions));

    scene->sceneRoot.objectID = 0;
    scene->pScene = &scene->sceneRoot;
    scene->pModel = &model.modelRoot;
    scene->pPhysics = &physics->physicsRoot;
    scene->pAnim = &animation->animRoot;
    scene->pPlayer = &player.playerRoot;
    physics->physicsRoot.objectID = 0;
    physics->physicsRoot.pParent = &scene->sceneRoot;
    animation->animRoot.objectID = 0;
    animation->animRoot.pParent = &scene->sceneRoot;
    animation->depack_context.seqdata = templates;
    player.playerRoot.objectID = 0;
    player.playerRoot.pParent = &scene->sceneRoot;
    player.pSettings.JumpVel = -100;
    player.pSettings.RunningJumpVel = 222;
    player.pSettings.JumpAngle = 0x333;
    player.pSettings.RunningJumpAngle = 0x444;
    player.paMotions = motions;
    player.maxMotions = 5;
    model.modelRoot.objectID = 0;
    model.modelRoot.pParent = &scene->sceneRoot;
    for (index = 0; index < 5; ++index) {
        motions[index].Seq = (uint16_t)index;
        motions[index].Lock = 1;
    }
    scene->v3WorldPosition.vx = 10;
    scene->v3WorldPosition.vy = 20;
    scene->v3WorldPosition.vz = 30;
    scene->v3SnapShotPosition.vx = 100;
    scene->v3SnapShotPosition.vy = 200;
    scene->v3SnapShotPosition.vz = 300;
    physics->mapinfo.poly = &poly;
    physics->reversoi = 99;
    leveldata = map_materials;
    gGlobalFrameRate = JPB_FIXED_ONE;
    gSCENE_READY = 0;

    map_materials[7] = INT32_C(0x4000);
    CHECK(jpb_BrainJumpLaunchState(
              &player,
              1,
              trace_trajectory_callback) ==
          JPB_BRAIN_PARTIAL_NO_CHANGE);
    CHECK(player.currentMotion == 0);
    CHECK(player.pMotionCallBack == NULL);
    CHECK(physics->reversoi == 99);

    map_materials[7] = 0;
    CHECK(jpb_BrainJumpLaunchState(
              &player,
              1,
              trace_trajectory_callback) ==
          JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 4);
    CHECK(animation->pMotion == &motions[4]);
    CHECK(player.airVelocity == -100);
    CHECK(player.airAngle == 0x400);
    CHECK(player.pMotionCallBack == trace_trajectory_callback);
    CHECK(physics->trajectory == 0x400);
    CHECK(physics->airspeed == -100);
    CHECK(physics->airmov.vx == 0.0f);
    CHECK(physics->airmov.vy == 99.0f);
    CHECK(physics->airmov.vz == 0.0f);
    CHECK((player.pFlags & UINT32_C(1)) != 0);
    CHECK(physics->pos.vx == 10.0f);
    CHECK(physics->snapshotpos.vy == 260.0f);
    CHECK(physics->reversoi == 0);

    animation->Lock = 0;
    player.currentMotion = 0;
    player.pFlags = 0;
    player.pMotionCallBack = NULL;
    physics->angle.vy = 0x0fff;
    physics->reversoi = 99;
    physics->mapinfo.poly = &poly;
    CHECK(jpb_BrainAlternateJumpLaunchState(
              &player,
              trace_trajectory_callback) ==
          JPB_BRAIN_PARTIAL_OK);
    CHECK(player.currentMotion == 4);
    CHECK(animation->pMotion == &motions[4]);
    CHECK(physics->angle.vy == 0x0f80);
    CHECK(player.airVelocity == 222);
    CHECK(player.airAngle == 0x444);
    CHECK(player.pMotionCallBack == trace_trajectory_callback);
    CHECK(physics->trajectory == 0x444);
    CHECK(physics->airspeed == 222);
    CHECK(physics->reversoi == 0);

    leveldata = old_leveldata;
    return 0;
}

int main(void)
{
    CHECK(test_fall_and_jump_selection() == 0);
    CHECK(test_air_trajectory() == 0);
    CHECK(test_velocity_axis_callback() == 0);
    CHECK(test_bounded_ground_direction() == 0);
    CHECK(test_bounded_ground_idle() == 0);
    CHECK(test_bounded_lock_on_direction() == 0);
    CHECK(test_bounded_special_direction() == 0);
    CHECK(test_attack_transition() == 0);
    CHECK(test_effect_and_ring_control() == 0);
    CHECK(test_ground_control() == 0);
    CHECK(test_lock_on_lifecycle() == 0);
    CHECK(test_animation_end_callbacks() == 0);
    CHECK(test_bounded_jump_launch() == 0);
    puts("brain tests passed");
    return 0;
}
