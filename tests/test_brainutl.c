#include "jpb/anim.h"
#include "jpb/brainutl.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/jedi.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/platform.h"
#include "jpb/scene.h"
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

typedef struct AchievementTrace {
    int complete_calls;
    int complete_ids[4];
    int get_calls;
    int get_ids[4];
} AchievementTrace;

static int trace_complete_achievement(int id, void *user_data)
{
    AchievementTrace *trace = (AchievementTrace *)user_data;

    trace->complete_ids[trace->complete_calls++] = id;
    return 1;
}

static int trace_get_complete_achievement(int id, void *user_data)
{
    AchievementTrace *trace = (AchievementTrace *)user_data;

    trace->get_ids[trace->get_calls++] = id;
    return 0;
}

static uint32_t test_cheat_chord_provider(void *user_data)
{
    return *(const uint32_t *)user_data;
}

static void reset_conform_cheats(void)
{
    memset(cheat_bigHeadPressed, 0, sizeof(cheat_bigHeadPressed));
    memset(cheat_bigHead, 0, sizeof(cheat_bigHead));
    memset(cheat_smallModePressed, 0, sizeof(cheat_smallModePressed));
    memset(cheat_smallMode, 0, sizeof(cheat_smallMode));
    memset(
        cheat_bigFeetAndSaberPressed,
        0,
        sizeof(cheat_bigFeetAndSaberPressed));
    memset(
        cheat_bigFeetAndSaber,
        0,
        sizeof(cheat_bigFeetAndSaber));
    cheat_bigHeadKeyPressed = 0;
    cheat_smallModeKeyPressed = 0;
    cheat_bigFeetAndSaberKeyPressed = 0;
}

static int test_conform_geom_nodes(void)
{
    static const int node_ids[] = {2, 3, 5, 6, 8, 10, 11, 14, 15};
    playerObject player;
    sceneObject scene;
    modelObject model;
    Mnode nodes[sizeof(node_ids) / sizeof(node_ids[0])];
    uint32_t keyboard_chords = 0;
    size_t index;

    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(nodes, 0, sizeof(nodes));
    coll_ResetCollisionSystem();
    reset_conform_cheats();
    player.playernum = 0;
    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pModel = &model.modelRoot;
    player.pSettings.minClosingDist = 99;
    for (index = 0; index < sizeof(node_ids) / sizeof(node_ids[0]); ++index) {
        nodes[index].id =
            (modelNodeId)(NODE_DYNAMIC | node_ids[index]);
        coll_gRegisterNode(0, &nodes[index]);
    }

    lastUsedInputType = 1;
    brainutl_ConformGeomNodes(&player);
    CHECK(model.v3Scale.vx == 0x78a);
    CHECK(model.v3Scale.vy == 0x78a);
    CHECK(model.v3Scale.vz == 0x78a);
    CHECK(player.fScale == 0x78a);
    CHECK(player.pSettings.minClosingDist == 0x14);
    CHECK(coll_GetNode(0, 8)->v3Scale.vx == 4000);
    CHECK((coll_GetNode(0, 8)->flags &
           JPB_COLLISION_FLAG_SCALE_OVERRIDE) != 0);

    player.playerPad.cpad[1] = INT32_C(0x4028);
    brainutl_ConformGeomNodes(&player);
    CHECK(cheat_bigFeetAndSaber[0] == 1);
    CHECK((coll_GetNode(0, 2)->flags &
           JPB_COLLISION_FLAG_SCALE_OVERRIDE) == 0);
    brainutl_ConformGeomNodes(&player);
    CHECK(cheat_bigFeetAndSaber[0] == 1);
    CHECK(coll_GetNode(0, 2)->v3Scale.vx == 5500);
    CHECK(coll_GetNode(0, 3)->v3Scale.vx == 6200);
    CHECK(coll_GetNode(0, 15)->v3Scale.vz == 6200);
    CHECK((coll_GetNode(0, 15)->flags &
           JPB_COLLISION_FLAG_SCALE_OVERRIDE) != 0);

    player.playerPad.cpad[1] = 0;
    brainutl_ConformGeomNodes(&player);
    player.playerPad.cpad[1] = INT32_C(0x4028);
    brainutl_ConformGeomNodes(&player);
    CHECK(cheat_bigFeetAndSaber[0] == 0);
    brainutl_ConformGeomNodes(&player);
    CHECK((coll_GetNode(0, 2)->flags &
           JPB_COLLISION_FLAG_SCALE_OVERRIDE) == 0);

    player.playerPad.cpad[1] = 0;
    brainutl_ConformGeomNodes(&player);
    player.playerPad.cpad[1] = INT32_C(0x4048);
    brainutl_ConformGeomNodes(&player);
    CHECK(cheat_bigHead[0] == 1);
    CHECK(coll_GetNode(0, 8)->v3Scale.vx == 4000);
    brainutl_ConformGeomNodes(&player);
    CHECK(coll_GetNode(0, 8)->v3Scale.vx == 20000);

    player.playerPad.cpad[1] = INT32_C(0x400a);
    brainutl_ConformGeomNodes(&player);
    CHECK(cheat_smallMode[0] == 1);
    CHECK(model.v3Scale.vx == 0x78a);
    brainutl_ConformGeomNodes(&player);
    CHECK(model.v3Scale.vx == 0x500);
    CHECK(player.fScale == 0x4fd);
    CHECK(player.pSettings.minClosingDist == 6);

    reset_conform_cheats();
    player.playerPad.cpad[1] = 0;
    lastUsedInputType = 0;
    CHECK(WInput_IsKBM() == 1);
    lastUsedInputType = 1;
    CHECK(WInput_IsKBM() == 0);
    jpb_BrainutlSetCheatChordProvider(
        test_cheat_chord_provider, &keyboard_chords);
    /* P1 raw keyboard chords remain live while XInput is last-used. */
    keyboard_chords = JPB_BRAINUTL_CHEAT_BIG_HEAD;
    brainutl_ConformGeomNodes(&player);
    brainutl_ConformGeomNodes(&player);
    CHECK(cheat_bigHead[0] == 1);
    CHECK(coll_GetNode(0, 8)->v3Scale.vx == 20000);
    keyboard_chords = 0;
    brainutl_ConformGeomNodes(&player);
    keyboard_chords = JPB_BRAINUTL_CHEAT_BIG_HEAD;
    brainutl_ConformGeomNodes(&player);
    CHECK(cheat_bigHead[0] == 0);
    jpb_BrainutlSetCheatChordProvider(NULL, NULL);
    return 0;
}

static int test_lsb_and_saber_edge_helpers(void)
{
    Mnode base;
    Mnode tip;

    CHECK(brainutl_FindLSB(0) == 0);
    CHECK(brainutl_FindLSB(1) == 1);
    CHECK(brainutl_FindLSB(UINT32_C(0x8000)) == 16);
    CHECK(brainutl_FindLSB(UINT32_C(0x10000)) == 0);
    CHECK(brainutl_FindLSB_LV(UINT32_C(0x10000)) == 17);

    memset(&base, 0, sizeof(base));
    memset(&tip, 0, sizeof(tip));
    base.id = (modelNodeId)(NODE_DYNAMIC | 0);
    tip.id = (modelNodeId)(NODE_DYNAMIC | 12);
    coll_ResetCollisionSystem();
    coll_gRegisterNode(0, &base);
    coll_gRegisterNode(0, &tip);
    CHECK(brainutl_AddSabreEdge(0, 0, 12) ==
          (int)(uint32_t)(uintptr_t)&tip.v3Velocity);
    return 0;
}

static animObject *prepare_landing_actor(
    playerObject *player,
    sceneObject *scene,
    physicsObject *physics,
    Motion motions[80],
    _animTemplate templates[80])
{
    animObject *animation;
    static Motion *motion_table[1];
    int index;

    memset(player, 0, sizeof(*player));
    memset(scene, 0, sizeof(*scene));
    memset(physics, 0, sizeof(*physics));
    memset(motions, 0, sizeof(Motion) * 80);
    memset(templates, 0, sizeof(_animTemplate) * 80);
    anim_InitAnimations(0);
    animation = &maAnimationData[0];

    player->playerRoot.pParent = &scene->sceneRoot;
    motion_table[0] = &motions[0];
    player->pMotion = motion_table;
    player->paMotions = motions;
    player->maxMotions = 80;
    scene->pPhysics = &physics->physicsRoot;
    scene->pAnim = &animation->animRoot;
    scene->pPlayer = &player->playerRoot;
    animation->animRoot.pParent = &scene->sceneRoot;
    animation->depack_context.seqdata = templates;
    for (index = 0; index < 80; ++index) {
        motions[index].Seq = (uint16_t)index;
        motions[index].Speed = -1;
    }
    return animation;
}

static int test_basic_landing_motion(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    Motion motions[80];
    _animTemplate templates[80];
    animObject *animation = prepare_landing_actor(
        &player, &scene, &physics, motions, templates);

    player.playerID = 9;
    player.pFlags = UINT32_C(1);
    physics.airTime = 123;
    brainutl_Land(&player);

    CHECK(physics.airTime == 0);
    CHECK((player.pFlags & UINT32_C(1)) == 0);
    CHECK(player.currentMotion == 5);
    CHECK(animation->pMotion == &motions[5]);
    CHECK(animation->animList.head == NULL);
    return 0;
}

static int test_special_landing_chain(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    Motion motions[80];
    _animTemplate templates[80];
    animObject *animation = prepare_landing_actor(
        &player, &scene, &physics, motions, templates);
    animListNode *queued;

    player.playerID = 0;
    player.currentMotion = 0;
    player.pFlags = UINT32_C(1);
    gGlobalTimer = 100;
    brainutl_Land(&player);

    CHECK(player.currentMotion == 53);
    CHECK(animation->pMotion == &motions[53]);
    CHECK((player.pFlags & UINT32_C(0x400)) != 0);
    CHECK((player.pFlags & UINT32_C(1)) == 0);
    CHECK(player.groundDelay == UINT32_C(0x1e64));
    CHECK(animation->animList.head != NULL);
    queued = (animListNode *)animation->animList.head;
    CHECK(queued->pMotion == &motions[59]);
    return 0;
}

static int test_long_fall_side_effects(void)
{
    JPBPlatformAchievementHooks hooks = {
        trace_complete_achievement,
        trace_get_complete_achievement
    };
    AchievementTrace trace;
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    Motion motions[80];
    _animTemplate templates[80];
    wsl_ENEMY enemy;
    wsl_BAP_PLACEMENT placement;

    (void)prepare_landing_actor(
        &player, &scene, &physics, motions, templates);
    memset(&trace, 0, sizeof(trace));
    memset(&enemy, 0, sizeof(enemy));
    memset(&placement, 0, sizeof(placement));
    enemy.pPlace = &placement;
    placement.aiDf.ownerType = 2;
    player.pEnemy = &enemy;
    player.playerID = 0;
    player.playernum = 0;
    physics.airTime = 0x16800;
    GameStruct.aCharacterData[0].Energy = 500;
    GameStruct.aCharacterData[0].MaxEnergy = 500;
    LevelSelect = 1;
    gDeathCount = 10;
    jpb_PlatformSetAchievementHooks(&hooks, &trace);

    brainutl_Land(&player);

    CHECK(GameStruct.aCharacterData[0].Energy == 245);
    CHECK(gDeathCount == 11);
    CHECK(trace.complete_calls == 1);
    CHECK(trace.complete_ids[0] == 3);
    CHECK(trace.get_calls == 1);
    CHECK(trace.get_ids[0] == 2);
    jpb_PlatformSetAchievementHooks(NULL, NULL);
    return 0;
}

static int test_nearest_target_selection(void)
{
    playerObject source;
    playerObject candidates[2];
    sceneObject source_scene;
    sceneObject candidate_scenes[2];
    physicsObject source_physics;
    modelObject models[2];
    objectRoot *target;
    int index;

    memset(&source, 0, sizeof(source));
    memset(candidates, 0, sizeof(candidates));
    memset(&source_scene, 0, sizeof(source_scene));
    memset(candidate_scenes, 0, sizeof(candidate_scenes));
    memset(&source_physics, 0, sizeof(source_physics));
    memset(models, 0, sizeof(models));
    physics_gInitObjects(0);
    source.playerRoot.objectID = 0;
    source.playerRoot.pParent = &source_scene.sceneRoot;
    source_scene.pScene = &source_scene.sceneRoot;
    source_scene.pPhysics = &source_physics.physicsRoot;
    source_physics.pos.vy = 100.0f;

    for (index = 0; index < 2; ++index) {
        int object_id = index + 2;
        physicsObject *physics =
            &maPhysicsData[object_id];
        sceneObject *scene =
            &candidate_scenes[index];
        playerObject *player =
            &candidates[index];

        physics->physicsRoot.objectID = object_id;
        physics->physicsRoot.pParent =
            &scene->sceneRoot;
        physics->pos.vy = 100.0f;
        scene->pScene = &scene->sceneRoot;
        scene->pModel = &models[index].modelRoot;
        scene->pPhysics = &physics->physicsRoot;
        scene->pPlayer = &player->playerRoot;
        player->playerRoot.objectID = object_id;
        player->playerRoot.pParent =
            &scene->sceneRoot;
        player->playernum = (int16_t)object_id;
        GameStruct.aCharacterData[object_id].Energy = 10;
    }
    GameStruct.versusModeFlag = 0;
    GameStruct.CurrentLevel = 0;
    maRange[0][2] = 200.0f;
    maRange[0][3] = 100.0f;

    target = brainutl_gGetNearestTarget(
        &source.playerRoot, 2);
    CHECK(target == &maPhysicsData[3].physicsRoot);

    maPhysicsData[3].pos.vy = 400.0f;
    target = brainutl_gGetNearestTarget(
        &source.playerRoot, 2);
    CHECK(target == &maPhysicsData[2].physicsRoot);

    models[0].flags = UINT32_C(4);
    target = brainutl_gGetNearestTarget(
        &source.playerRoot, 2);
    CHECK(target == NULL);
    candidates[0].pFlags = UINT32_C(0x2000);
    target = brainutl_gGetNearestTarget(
        &source.playerRoot, 2);
    CHECK(target == &maPhysicsData[2].physicsRoot);
    return 0;
}

static int test_pad_motion_names(void)
{
    playerObject player;
    int32_t cpad[2] = {0, 0};

    memset(&player, 0, sizeof(player));
    cpad[1] = 0xb0;
    brainutl_HeldPad(&player, cpad);
    CHECK(strcmp(player.HeldMotion, "n+e+w") == 0);

    strcpy(player.PreMotion, "idle:");
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    cpad[0] = 0x34;
    brainutl_MultiPad(&player, cpad);
    CHECK(strcmp(player.PreMotion, "idle:fn+e") == 0);

    memset(player.PreMotion, 'x', 31);
    player.PreMotion[31] = '\0';
    brainutl_MultiPad(&player, cpad);
    CHECK(strlen(player.PreMotion) == 31);
    return 0;
}

static int test_reverse_and_limit_helpers(void)
{
    playerObject player;
    playerObject target;
    sceneObject player_scene;
    sceneObject target_scene;
    physicsObject player_physics;
    physicsObject target_physics;
    int value;

    memset(&player, 0, sizeof(player));
    memset(&target, 0, sizeof(target));
    memset(&player_scene, 0, sizeof(player_scene));
    memset(&target_scene, 0, sizeof(target_scene));
    memset(&player_physics, 0, sizeof(player_physics));
    memset(&target_physics, 0, sizeof(target_physics));
    player.playerRoot.pParent = &player_scene.sceneRoot;
    player_scene.pPhysics = &player_physics.physicsRoot;
    target.playerRoot.pParent = &target_scene.sceneRoot;
    target_scene.pPhysics = &target_physics.physicsRoot;
    player.target = &target;

    player_physics.angle.vy = 0;
    target_physics.vpos.vx = 0;
    target_physics.vpos.vz = -100;
    CHECK(brainutil_ReverseCheck(&player) == 1);
    target_physics.vpos.vz = 100;
    CHECK(brainutil_ReverseCheck(&player) == 0);

    value = -5;
    brainutil_limitRange(&value, 0, 10);
    CHECK(value == 0);
    value = 15;
    brainutil_limitRange(&value, 0, 10);
    CHECK(value == 10);
    value = 5;
    brainutil_limitRange(&value, 0, 10);
    CHECK(value == 5);
    return 0;
}

static int test_trajectory_callback(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    Motion motions[80];
    _animTemplate templates[80];
    int32_t cpad[2] = {0, 0};

    (void)prepare_landing_actor(
        &player,
        &scene,
        &physics,
        motions,
        templates);
    game_setFuncArray();
    player.playernum = 0;
    player.playerID = 0;
    player.currentMotion = 4;
    motions[0].motionFlags =
        UINT32_C(0x40000000);
    physics.airmov.vy = -1.0f;
    physics.airTime = 100;
    physics.realAirTime = 200;
    gGlobalFrameRate = 50;
    OptionStruct.JumpCheat = 0;

    CHECK(brainutil_PlotTrajectory(
              cpad, &player) == -1);
    CHECK(physics.airTime == 150);
    CHECK(physics.realAirTime == 250);

    physics.constmov.vx = 1.0f;
    physics.constmov.vy = 2.0f;
    physics.constmov.vz = 3.0f;
    motions[0].motionFlags = 0;
    CHECK(brainutil_PlotTrajectory(
              cpad, &player) == 1);
    CHECK(physics.constmov.vx == 0.0f);
    CHECK(physics.constmov.vy == 0.0f);
    CHECK(physics.constmov.vz == 0.0f);

    motions[0].motionFlags =
        UINT32_C(0x40000000);
    player.pSettings.JumpVel = 800;
    player.pSettings.JumpAngle = 0x300;
    physics.mov.vy = 0.0f;
    physics.falltimer = 0;
    gGlobalTimer = 100;
    cpad[0] = INT32_C(0x20);
    CHECK(brainutil_PlotTrajectory(
              cpad, &player) == -1);
    CHECK(physics.airspeed == 800);
    CHECK(physics.trajectory == 0x300);
    CHECK((player.pFlags &
           UINT32_C(1)) != 0);
    CHECK(player.pMotionCallBack ==
          brainutil_PlotTrajectory);
    CHECK((motions[22].motionFlags &
           UINT32_C(0x04000000)) != 0);
    return 0;
}

static int test_maul_trajectory_callback(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    Motion motions[80];
    _animTemplate templates[80];

    (void)prepare_landing_actor(
        &player,
        &scene,
        &physics,
        motions,
        templates);
    game_setFuncArray();
    player.playernum = 2;
    player.currentMotion = 4;
    player.pSettings.dblJumpVel = 900;
    player.pSettings.dblJumpAngle = 0x280;
    physics.airTime = 0x8000;
    physics.airGround = 0.0f;
    physics.mov.vy = 0.0f;

    CHECK(brainutil_PlotMaulTrajectory(
              NULL, &player) == -1);
    CHECK(player.currentMotion == 22);
    CHECK(physics.airspeed == 900);
    CHECK(physics.trajectory == 0x280);
    CHECK(player.pMotionCallBack ==
          brainutil_PlotMaulTrajectory);
    CHECK((motions[22].motionFlags &
           UINT32_C(0x04000000)) != 0);

    player.currentMotion = 3;
    physics.airmov.vy = -1.0f;
    physics.airTime = 0xc900;
    physics.realAirTime = 20;
    gGlobalFrameRate = 100;
    CHECK(brainutil_PlotMaulTrajectory(
              NULL, &player) == -1);
    CHECK(physics.airTime == 0xc800);
    CHECK(physics.realAirTime == 120);
    return 0;
}

static int test_jedi_player_settings(void)
{
    playerObject player;

    memset(&player, 0, sizeof(player));
    CHECK(jpb_jedi_ApplyPlayerSettings(
              &player) == 1);
    CHECK(player.pSettings.JumpVel == 0x7a);
    CHECK(player.pSettings.RunningJumpVel == 0x73);
    CHECK(player.pSettings.dblJumpVel == 0x73);
    CHECK(player.pSettings.JumpAngle == 0x2f9);
    CHECK(player.pSettings.RunningJumpAngle == 0x341);
    CHECK(player.pSettings.dblJumpAngle == 0x35a);
    CHECK(player.pSettings.bkJumpAngle == 0x555);
    CHECK(player.pSettings.gravity ==
          UINT16_C(0xe314));
    CHECK(player.pSettings.dblgravity == 0);
    CHECK(player.pSettings.minClosingDist == 0x14);
    return 0;
}

int main(void)
{
    CHECK(test_lsb_and_saber_edge_helpers() == 0);
    CHECK(test_conform_geom_nodes() == 0);
    CHECK(test_basic_landing_motion() == 0);
    CHECK(test_special_landing_chain() == 0);
    CHECK(test_long_fall_side_effects() == 0);
    CHECK(test_nearest_target_selection() == 0);
    CHECK(test_pad_motion_names() == 0);
    CHECK(test_reverse_and_limit_helpers() == 0);
    CHECK(test_trajectory_callback() == 0);
    CHECK(test_maul_trajectory_callback() == 0);
    CHECK(test_jedi_player_settings() == 0);
    puts("brainutl tests passed");
    return 0;
}
