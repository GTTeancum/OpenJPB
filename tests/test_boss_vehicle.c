#include "jpb/boss.h"
#include "jpb/anim.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/cube.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/vehicle.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                             \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct DebugSphereCapture {
    int calls;
    int x;
    int y;
    int z;
    int radius;
    uint32_t color;
} DebugSphereCapture;

typedef struct BarTrace {
    int calls;
    int x[8];
    int y[8];
    int width[8];
    int height[8];
    uint32_t color[8];
} BarTrace;

static void capture_debug_sphere(
    void *user_data,
    int32_t x,
    int32_t y,
    int32_t z,
    int32_t radius,
    uint32_t color)
{
    DebugSphereCapture *capture = (DebugSphereCapture *)user_data;

    ++capture->calls;
    capture->x = x;
    capture->y = y;
    capture->z = z;
    capture->radius = radius;
    capture->color = color;
}

static void capture_bar(
    void *user_data,
    int x,
    int y,
    int width,
    int height,
    uint32_t color)
{
    BarTrace *trace = (BarTrace *)user_data;
    int index = trace->calls++;

    if (index < 8) {
        trace->x[index] = x;
        trace->y[index] = y;
        trace->width[index] = width;
        trace->height[index] = height;
        trace->color[index] = color;
    }
}

static int test_deadly_callback(void)
{
    playerObject player;

    memset(&player, 0, sizeof(player));
    player.forceFlags = UINT32_C(0x100);
    CHECK(ai_Deadly(NULL, &player) == 1);
    CHECK(player.forceFlags == UINT32_C(0x140));
    CHECK(ai_Deadly(NULL, &player) == 1);
    CHECK(player.forceFlags == UINT32_C(0x140));
    return 0;
}

static int test_jar_jar_callback(void)
{
    WorldData world;
    playerObject jar_jar;
    playerObject players[2];
    playerObject target;
    sceneObject scene;
    physicsObject physics;

    memset(&world, 0, sizeof(world));
    memset(&jar_jar, 0, sizeof(jar_jar));
    memset(players, 0, sizeof(players));
    memset(&target, 0, sizeof(target));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    jar_jar.playerRoot.pParent = &scene.sceneRoot;
    scene.pPhysics = &physics.physicsRoot;
    physics.pos.vx = 12.75f;
    physics.pos.vy = -34.5f;
    physics.pos.vz = 56.25f;
    players[0].playerRoot.objectID = -1;
    players[1].playerRoot.objectID = -1;
    world.player0 = &players[0];
    world.player1 = &players[1];
    gpWorld = &world;

    GameStruct.CurrentLevel = 0;
    GameStruct.NumPlayers = 1;
    CHECK(ai_JarJar(NULL, &jar_jar) == 1);
    CHECK(gJarJarPos.vx == 12);
    CHECK(gJarJarPos.vy == -34);
    CHECK(gJarJarPos.vz == 56);
    CHECK(camera_GetCurrentCameraType() == 1);

    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    GameStruct.CurrentLevel = 13;
    jar_jar.fStun = 1;
    jar_jar.target = &target;
    target.playernum = 3;
    CHECK(ai_JarJar(NULL, &jar_jar) == -1);
    CHECK((abGlobalBits[6] & UINT8_C(8)) != 0);
    gpWorld = NULL;
    GameStruct.CurrentLevel = 0;
    return 0;
}

static int test_kadu_level_gate(void)
{
    playerObject kadu;
    sceneObject scene;
    physicsObject physics;
    animObject animation;
    Motion motion;
    Motion *current = &motion;
    wsl_ENEMY enemy;
    wsl_BAP_PLACEMENT placement;

    memset(&kadu, 0, sizeof(kadu));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&animation, 0, sizeof(animation));
    memset(&motion, 0, sizeof(motion));
    memset(&enemy, 0, sizeof(enemy));
    memset(&placement, 0, sizeof(placement));
    kadu.playerRoot.pParent = &scene.sceneRoot;
    kadu.pMotion = &current;
    kadu.pEnemy = &enemy;
    enemy.pPlace = &placement;
    placement.aiDf.daRange = 1234;
    scene.pPhysics = &physics.physicsRoot;
    scene.pAnim = &animation.animRoot;
    GameStruct.CurrentLevel = 0;
    zerobss_levelReset = 93;
    zerobss_ResetBoss = 1;

    CHECK(ai_Kadu(NULL, &kadu) == 1);
    CHECK(placement.aiDf.daRange == 0);
    return 0;
}

static int test_kadu_race_state(void)
{
    WorldData world;
    playerObject kadu;
    playerObject riders[2];
    sceneObject kaduScene;
    sceneObject riderScenes[2];
    physicsObject kaduPhysics;
    physicsObject riderPhysics[2];
    animObject animation;
    Motion motion;
    Motion *current = &motion;
    wsl_ENEMY enemy;
    wsl_BAP_PLACEMENT placement;

    memset(&world, 0, sizeof(world));
    memset(&kadu, 0, sizeof(kadu));
    memset(riders, 0, sizeof(riders));
    memset(&kaduScene, 0, sizeof(kaduScene));
    memset(riderScenes, 0, sizeof(riderScenes));
    memset(&kaduPhysics, 0, sizeof(kaduPhysics));
    memset(riderPhysics, 0, sizeof(riderPhysics));
    memset(&animation, 0, sizeof(animation));
    memset(&motion, 0, sizeof(motion));
    memset(&enemy, 0, sizeof(enemy));
    memset(&placement, 0, sizeof(placement));
    kadu.playerRoot.pParent = &kaduScene.sceneRoot;
    kadu.playernum = 4;
    kadu.pMotion = &current;
    kadu.pEnemy = &enemy;
    enemy.pPlace = &placement;
    kaduScene.pPhysics = &kaduPhysics.physicsRoot;
    kaduScene.pAnim = &animation.animRoot;
    motion.Seq = 1;
    riders[0].playerRoot.pParent = &riderScenes[0].sceneRoot;
    riders[1].playerRoot.pParent = &riderScenes[1].sceneRoot;
    riderScenes[0].pPhysics = &riderPhysics[0].physicsRoot;
    riderScenes[1].pPhysics = &riderPhysics[1].physicsRoot;
    riderPhysics[0].pos.vz = 100.0f;
    riderPhysics[1].pos.vx = 25.0f;
    riderPhysics[1].pos.vz = 50.0f;
    maPhysicsData[3].pos.vy = 75.0f;
    world.player0 = &riders[0];
    world.player1 = &riders[1];
    gpWorld = &world;
    GameStruct.CurrentLevel = 12;
    GameStruct.NumPlayers = 2;
    GameStruct.GameState = 0;
    OptionStruct.ControllerConfig[1] = 0;
    zerobss_levelReset = 95;
    zerobss_ResetBoss = 1;

    CHECK(ai_Kadu(NULL, &kadu) == -1);
    CHECK(ai_Kadu(NULL, &kadu) == -1);
    CHECK((riders[0].pFlags & UINT32_C(0x80)) != 0);
    CHECK((riders[1].pFlags & UINT32_C(0x80)) != 0);
    CHECK(kaduPhysics.angle.vx == 300);
    CHECK(kaduPhysics.constmov.vz == 16.0f);
    CHECK(animation.animFrameRate == 0x800);
    CHECK(gJarJarPos.vx == 25);
    CHECK(gJarJarPos.vy == 75);
    CHECK(gJarJarPos.vz == 50);
    CHECK(camera_GetCurrentCameraType() == 6);
    gpWorld = NULL;
    GameStruct.CurrentLevel = 0;
    return 0;
}

static int test_kadu_race_hud_bars(void)
{
    BarTrace trace;
    WorldData world;
    playerObject kadu;
    playerObject riders[2];
    sceneObject kaduScene;
    sceneObject riderScenes[2];
    physicsObject kaduPhysics;
    physicsObject riderPhysics[2];
    Mnode kaduRoot;
    Mnode riderNodes[2][2];
    animObject animation;
    animObject riderAnimations[2];
    Motion riderMotions[2][79];
    animListNode riderAnimNodes[2];
    _animTemplate riderAnimTemplates[2];
    Motion motion;
    Motion *current = &motion;
    wsl_ENEMY enemy;
    wsl_BAP_PLACEMENT placement;

    memset(&trace, 0, sizeof(trace));
    memset(&world, 0, sizeof(world));
    memset(&kadu, 0, sizeof(kadu));
    memset(riders, 0, sizeof(riders));
    memset(&kaduScene, 0, sizeof(kaduScene));
    memset(riderScenes, 0, sizeof(riderScenes));
    memset(&kaduPhysics, 0, sizeof(kaduPhysics));
    memset(riderPhysics, 0, sizeof(riderPhysics));
    memset(&kaduRoot, 0, sizeof(kaduRoot));
    memset(riderNodes, 0, sizeof(riderNodes));
    memset(&animation, 0, sizeof(animation));
    memset(riderAnimations, 0, sizeof(riderAnimations));
    memset(riderMotions, 0, sizeof(riderMotions));
    memset(riderAnimNodes, 0, sizeof(riderAnimNodes));
    memset(riderAnimTemplates, 0, sizeof(riderAnimTemplates));
    memset(&motion, 0, sizeof(motion));
    memset(&enemy, 0, sizeof(enemy));
    memset(&placement, 0, sizeof(placement));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(&GameStruct, 0, sizeof(GameStruct));
    coll_ResetCollisionSystem();
    kadu.playerRoot.pParent = &kaduScene.sceneRoot;
    kadu.playerRoot.objectID = 2;
    kadu.playernum = 2;
    kadu.pMotion = &current;
    kadu.pEnemy = &enemy;
    enemy.pPlace = &placement;
    kaduScene.pPhysics = &kaduPhysics.physicsRoot;
    kaduScene.pAnim = &animation.animRoot;
    motion.Seq = 1;
    riders[0].playerRoot.pParent = &riderScenes[0].sceneRoot;
    riders[1].playerRoot.pParent = &riderScenes[1].sceneRoot;
    riders[0].paMotions = riderMotions[0];
    riders[1].paMotions = riderMotions[1];
    riders[0].maxMotions = 79;
    riders[1].maxMotions = 79;
    riderScenes[0].pPhysics = &riderPhysics[0].physicsRoot;
    riderScenes[1].pPhysics = &riderPhysics[1].physicsRoot;
    riderScenes[0].pAnim = &riderAnimations[0].animRoot;
    riderScenes[1].pAnim = &riderAnimations[1].animRoot;
    riderScenes[0].pPlayer = &riders[0].playerRoot;
    riderScenes[1].pPlayer = &riders[1].playerRoot;
    riderAnimations[0].animRoot.pParent = &riderScenes[0].sceneRoot;
    riderAnimations[1].animRoot.pParent = &riderScenes[1].sceneRoot;
    riderAnimations[0].animRoot.objectID = 0;
    riderAnimations[1].animRoot.objectID = 1;
    riderAnimations[0].depack_context.seqdata = &riderAnimTemplates[0];
    riderAnimations[1].depack_context.seqdata = &riderAnimTemplates[1];
    list_AddTail(
        &riderAnimations[0].animFreeList,
        &riderAnimNodes[0].anm_Node);
    list_AddTail(
        &riderAnimations[1].animFreeList,
        &riderAnimNodes[1].anm_Node);
    riderPhysics[0].pos.vz = 100.0f;
    riderPhysics[1].pos.vz = 50.0f;
    riderPhysics[0].physicsRoot.pParent = &riderScenes[0].sceneRoot;
    riderPhysics[1].physicsRoot.pParent = &riderScenes[1].sceneRoot;
    riderPhysics[0].physicsRoot.objectID = 0;
    riderPhysics[1].physicsRoot.objectID = 1;
    maPhysicsData[0].physicsRoot.pParent = &riderScenes[0].sceneRoot;
    maPhysicsData[1].physicsRoot.pParent = &riderScenes[1].sceneRoot;
    maPhysicsData[0].physicsRoot.objectID = 0;
    maPhysicsData[1].physicsRoot.objectID = 1;
    kaduRoot.id = NODE_DYNAMIC;
    riderNodes[0][0].id = NODE_DYNAMIC | 10;
    riderNodes[0][1].id = NODE_DYNAMIC | 14;
    riderNodes[1][0].id = NODE_DYNAMIC | 10;
    riderNodes[1][1].id = NODE_DYNAMIC | 14;
    coll_gRegisterNode(kadu.playerRoot.objectID, &kaduRoot);
    coll_gRegisterNode(0, &riderNodes[0][0]);
    coll_gRegisterNode(0, &riderNodes[0][1]);
    coll_gRegisterNode(1, &riderNodes[1][0]);
    coll_gRegisterNode(1, &riderNodes[1][1]);
    world.player0 = &riders[0];
    world.player1 = &riders[1];
    gpWorld = &world;
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    GameStruct.CurrentLevel = 12;
    GameStruct.NumPlayers = 2;
    GameStruct.GameState = 0;
    OptionStruct.ControllerConfig[0] = 0;
    OptionStruct.ControllerConfig[1] = 0;
    zerobss_levelReset = 101;
    zerobss_ResetBoss = 1;
    jpb_GameSetBarHook(capture_bar, &trace);

    CHECK(ai_Kadu(NULL, &kadu) == -1);
    CHECK(trace.calls == 1);
    CHECK(trace.x[0] == -30);
    CHECK(trace.y[0] == 428);
    CHECK(trace.width[0] == 64);
    CHECK(trace.height[0] == 12);
    CHECK(trace.color[0] == UINT32_C(0x7fff4010));

    kadu.playernum = 3;
    zerobss_levelReset = 102;
    zerobss_ResetBoss = 1;
    CHECK(ai_Kadu(NULL, &kadu) == -1);
    CHECK(trace.calls == 2);
    CHECK(trace.x[1] == 606);
    CHECK(trace.y[1] == 428);
    CHECK(trace.width[1] == 64);
    CHECK(trace.height[1] == 12);
    CHECK(trace.color[1] == UINT32_C(0x7f1040ff));

    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    kadu.playernum = 2;
    zerobss_levelReset = 103;
    zerobss_ResetBoss = 1;
    CHECK(ai_Kadu(NULL, &kadu) == -1);
    CHECK(trace.calls == 3);
    CHECK(trace.x[2] == 305);
    CHECK(trace.y[2] == 514);
    CHECK(trace.width[2] == 64);
    CHECK(trace.height[2] == 6);
    CHECK(trace.color[2] == UINT32_C(0x7fff4010));

    kadu.playernum = 3;
    zerobss_levelReset = 104;
    zerobss_ResetBoss = 1;
    CHECK(ai_Kadu(NULL, &kadu) == -1);
    CHECK(trace.calls == 4);
    CHECK(trace.x[3] == 591);
    CHECK(trace.y[3] == 514);
    CHECK(trace.width[3] == 64);
    CHECK(trace.height[3] == 6);
    CHECK(trace.color[3] == UINT32_C(0x7f1040ff));

    jpb_GameSetBarHook(NULL, NULL);
    gpWorld = NULL;
    memset(&maPhysicsData[0], 0, sizeof(maPhysicsData[0]));
    memset(&maPhysicsData[1], 0, sizeof(maPhysicsData[1]));
    return 0;
}

static int test_maul_callback(void)
{
    playerObject player;
    playerObject target;
    wsl_ENEMY enemy;
    Motion motions[87];
    Motion current;
    Motion *current_ptr = &current;

    memset(&player, 0, sizeof(player));
    memset(&target, 0, sizeof(target));
    memset(&enemy, 0, sizeof(enemy));
    memset(motions, 0, sizeof(motions));
    memset(&current, 0, sizeof(current));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    player.playernum = 4;
    player.playerID = 9;
    player.target = &target;
    player.pEnemy = &enemy;
    player.paMotions = motions;
    player.pMotion = &current_ptr;
    current.Damage = 1;
    enemy.aRange = -1;
    abGlobalBits[5] = UINT8_C(4);
    GameStruct.CurrentLevel = 5;
    gGlobalTimer = UINT32_C(0x1000);
    isTatoMaul = 0;
    zerobss_levelReset = 91;
    zerobss_ResetBoss = 1;

    CHECK(ai_Maul(NULL, &player) == -1);
    CHECK(isTatoMaul == 1);
    CHECK(enemy.aRange == 1);
    CHECK(motions[81].FunctPtr == 27);
    CHECK(motions[82].FunctPtr == 28);
    CHECK(motions[86].FunctPtr == 29);
    CHECK(player.locked == &target);
    CHECK((player.pFlags & UINT32_C(0x00400000)) != 0);
    GameStruct.CurrentLevel = 0;
    abGlobalBits[5] = 0;
    return 0;
}

static int test_thug_callback_setup(void)
{
    playerObject player;
    Motion motions[66];
    EffectHeader shield_effect;
    EffectHeader *old_effect = paEffects[61];

    memset(&player, 0, sizeof(player));
    memset(motions, 0, sizeof(motions));
    memset(&shield_effect, 0, sizeof(shield_effect));
    player.playernum = 6;
    player.playerRoot.objectID = 6;
    player.paMotions = motions;
    paEffects[61] = &shield_effect;
    zerobss_levelReset = 92;
    zerobss_ResetBoss = 1;

    CHECK(ai_Thug(NULL, &player) == -1);
    CHECK(motions[65].FunctPtr == 25);
    CHECK(player.forceData[0] == 0);
    paEffects[61] = old_effect;
    return 0;
}

static int test_turret_droid_idle_state(void)
{
    playerObject turret;
    sceneObject scene;
    physicsObject physics;
    modelObject model;
    Mnode arms[2];
    EffectHeader armEffect;
    EffectHeader *oldArmEffect = paEffects[2];

    memset(&turret, 0, sizeof(turret));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&model, 0, sizeof(model));
    memset(arms, 0, sizeof(arms));
    memset(&armEffect, 0, sizeof(armEffect));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    turret.playerRoot.objectID = 7;
    turret.playerRoot.pParent = &scene.sceneRoot;
    turret.playernum = 7;
    scene.pPhysics = &physics.physicsRoot;
    scene.pModel = &model.modelRoot;
    arms[0].id = (modelNodeId)(NODE_DYNAMIC | 12);
    arms[1].id = (modelNodeId)(NODE_DYNAMIC | 11);
    coll_ResetCollisionSystem();
    coll_gRegisterNode(turret.playernum, &arms[0]);
    coll_gRegisterNode(turret.playernum, &arms[1]);
    GameStruct.Counter = 0;
    abGlobalBits[5] = UINT8_C(8);
    zerobss_levelReset = 94;
    zerobss_ResetBoss = 1;

    CHECK(ai_TurretDroid(NULL, &turret) == -1);
    CHECK((abGlobalBits[3] & UINT8_C(1)) != 0);
    CHECK((turret.forceFlags & UINT32_C(0x20)) != 0);
    CHECK(arms[0].v3RotationAbs.vx == 0x10);
    CHECK(arms[1].v3RotationAbs.vx == 0x10);
    CHECK((arms[0].flags &
           JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) != 0);

    CHECK(ai_TurretDroid(NULL, &turret) == -1);
    CHECK((turret.forceFlags & UINT32_C(0x20)) == 0);
    CHECK(arms[0].v3RotationAbs.vx == 0x20);

    paEffects[2] = &armEffect;
    arms[0].v3RotCenter.vx = 10;
    arms[0].v3RotCenter.vy = 20;
    arms[0].v3RotCenter.vz = 30;
    turret.hitVelocity.vx = 1;
    turret.hitVelocity.vy = 2;
    turret.hitVelocity.vz = 3;
    turret.currentMotion = 17;
    CHECK(ai_TurretDroid(NULL, &turret) == -1);
    CHECK((arms[0].flags & UINT32_C(0x04000000)) != 0);
    CHECK(arms[0].v3Translation2.vx == 10);
    CHECK(arms[0].v3Translation2.vy == 20);
    CHECK(arms[0].v3Translation2.vz == 30);
    CHECK(arms[0].v3Velocity2.vy == 0x40);
    CHECK(arms[0].v3Velocity2.vx >= -8);
    CHECK(arms[0].v3Velocity2.vx <= -5);
    CHECK(arms[0].v3Velocity2.vz >= -8);
    CHECK(arms[0].v3Velocity2.vz <= -5);
    paEffects[2] = oldArmEffect;
    return 0;
}

static int test_maul_callback_windows(void)
{
    playerObject player;
    sceneObject scene;
    animObject animation;
    animListNode sequence;
    _animTemplate template_data;

    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&animation, 0, sizeof(animation));
    memset(&sequence, 0, sizeof(sequence));
    memset(&template_data, 0, sizeof(template_data));
    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pAnim = &animation.animRoot;
    animation.pCurrentAnimSeq = &sequence;
    animation.animFrameIndex = JPB_FIXED_ONE;
    sequence.pAnimTemplate = &template_data;

    CHECK(maul_PushCallBack(NULL, &player) == 0);
    CHECK(maul_RingCallBack(NULL, &player) == 0);
    CHECK(maul_ZapCallBack(NULL, &player) == 0);
    return 0;
}

static int test_sphere_callbacks(void)
{
    playerObject player;
    CollisionData collision;
    Mnode node;
    DebugSphereCapture capture;

    memset(&player, 0, sizeof(player));
    memset(&node, 0, sizeof(node));
    memset(&capture, 0, sizeof(capture));
    collision.radius1 = 0x100;
    collision.id = 2;
    collision.parentid = -1;
    node.id = (modelNodeId)(NODE_DYNAMIC | 2);
    node.v3RotCenter.vx = 10;
    node.v3RotCenter.vy = -20;
    node.v3RotCenter.vz = 30;
    node.flags = JPB_COLLISION_FLAG_HOT;
    player.playernum = 3;
    player.fScale = 0x1800;
    player.paNodesSizes = &collision;
    player.numCollisionNodes = 1;

    coll_ResetCollisionSystem();
    coll_gRegisterNode(player.playernum, &node);
    jpb_WHookSetDebugSphereHook(
        capture_debug_sphere, &capture);
    gGlobalTimer = 100;
    player.hitDelay = 100;
    CHECK(ai_Sphere(NULL, &player) == -1);
    CHECK(capture.calls == 1);
    CHECK(capture.x == 10);
    CHECK(capture.y == -20);
    CHECK(capture.z == 30);
    CHECK(capture.radius == 0x180);
    CHECK(capture.color == UINT32_C(0x00ff0000));

    memset(&capture, 0, sizeof(capture));
    player.hitDelay = 101;
    CHECK(ai_Sphere(NULL, &player) == 100);
    CHECK(capture.calls == 0);

    player.numCollisionNodes = 0;
    player.hitDelay = 100;
    CHECK(ai_Krakis(NULL, &player) == -1);
    CHECK(ai_Worm(NULL, &player) == -1);
    jpb_WHookSetDebugSphereHook(NULL, NULL);
    return 0;
}

static int test_mtt_damage_volume(void)
{
    WorldData world;
    playerObject mtt;
    sceneObject mtt_scene;
    physicsObject mtt_physics;
    sceneObject target_scenes[2];
    physicsObject target_physics[2];
    objectRoot target_scene_roots[2];

    memset(&world, 0, sizeof(world));
    memset(&mtt, 0, sizeof(mtt));
    memset(&mtt_scene, 0, sizeof(mtt_scene));
    memset(&mtt_physics, 0, sizeof(mtt_physics));
    memset(gaPlayerData, 0, sizeof(gaPlayerData));
    memset(target_scenes, 0, sizeof(target_scenes));
    memset(target_physics, 0, sizeof(target_physics));
    memset(target_scene_roots, 0, sizeof(target_scene_roots));
    memset(&GameStruct, 0, sizeof(GameStruct));

    mtt.playerRoot.pParent = &mtt_scene.sceneRoot;
    mtt_scene.pPhysics = &mtt_physics.physicsRoot;
    mtt_physics.pos.vx = 0.0f;
    mtt_physics.pos.vy = 0.0f;
    mtt_physics.pos.vz = 0.0f;

    gaPlayerData[0].playerRoot.objectID = 0;
    gaPlayerData[0].playerRoot.pParent = &target_scenes[0].sceneRoot;
    target_scenes[0].pScene = &target_scene_roots[0];
    target_scenes[0].pPhysics = &target_physics[0].physicsRoot;
    target_physics[0].pos.vx = 511.0f;

    gaPlayerData[1].playerRoot.objectID = 1;
    gaPlayerData[1].playerRoot.pParent = &target_scenes[1].sceneRoot;
    target_scenes[1].pScene = &target_scene_roots[1];
    target_scenes[1].pPhysics = &target_physics[1].physicsRoot;
    target_physics[1].pos.vx = 513.0f;

    world.player0 = &gaPlayerData[0];
    world.player1 = &gaPlayerData[1];
    gpWorld = &world;
    LevelSelect = 0;
    GameStruct.aCharacterData[0].Energy = 255;
    GameStruct.aCharacterData[0].MaxEnergy = 255;
    GameStruct.aCharacterData[1].Energy = 255;
    GameStruct.aCharacterData[1].MaxEnergy = 255;

    CHECK(ai_Mtt(NULL, &mtt) == -1);
    CHECK(GameStruct.aCharacterData[0].Energy == 0);
    CHECK(GameStruct.aCharacterData[1].Energy == 255);
    gpWorld = NULL;
    return 0;
}

static int test_blades_callback(void)
{
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    Mnode pelvis;

    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&pelvis, 0, sizeof(pelvis));
    player.playerRoot.objectID = 4;
    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pPhysics = &physics.physicsRoot;
    pelvis.id = NODE_DYNAMIC;
    pelvis.v3RotationAbs.vy = 99;
    physics.userdata[0] = 0x40;
    gGlobalFrameRate = 0x800;

    coll_ResetCollisionSystem();
    coll_gRegisterNode(player.playerRoot.objectID, &pelvis);
    CHECK(ai_Blades(NULL, &player) == 0);
    CHECK(pelvis.v3RotationAbs.vy == 0);
    CHECK((pelvis.flags &
           JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) != 0);
    CHECK(physics.angle.vy == 0x40);
    CHECK(physics.userdata[0] == 0x41);
    return 0;
}

static int test_centre_turret(void)
{
    Mnode turret;

    memset(&turret, 0, sizeof(turret));
    turret.v3RotationAbs.vy = 100;
    gGlobalFrameRate = 0x800;
    centreturret(&turret);
    CHECK(turret.v3RotationAbs.vy == 96);

    turret.v3RotationAbs.vy = -100;
    centreturret(&turret);
    CHECK(turret.v3RotationAbs.vy == -96);
    return 0;
}

static int test_aat_turret_tracking(void)
{
    playerObject aat;
    sceneObject scene;
    physicsObject physics;
    Mnode turret;

    memset(&aat, 0, sizeof(aat));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&turret, 0, sizeof(turret));
    memset(&gaPlayerData[0], 0, sizeof(gaPlayerData[0]));
    memset(&maPhysicsData[0], 0, sizeof(maPhysicsData[0]));
    aat.playerRoot.objectID = 2;
    aat.playerRoot.pParent = &scene.sceneRoot;
    aat.playernum = 5;
    scene.pPhysics = &physics.physicsRoot;
    turret.id = (modelNodeId)(NODE_DYNAMIC | 9);
    turret.v3RotationAbs.vy = 0x40;
    physics.flags = UINT32_C(0x00400000);
    physics.userdata[0] = 0x1000;
    gaPlayerData[0].playerRoot.objectID = 0;
    maPhysicsData[0].pos.vz = 100.0f;
    maRange[0][2] = 0x800;
    playertankindex = 1;
    gGlobalFrameRate = 0x800;

    coll_ResetCollisionSystem();
    coll_gRegisterNode(aat.playernum, &turret);
    CHECK(ai_AAT(NULL, &aat) == -1);
    CHECK((physics.flags & UINT32_C(0x00400000)) == 0);
    CHECK(turret.v3RotationAbs.vy == 0x3a);
    CHECK((turret.flags &
           JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) != 0);
    CHECK(physics.userdata[0] == 0x800);
    playertankindex = 0;
    return 0;
}

static int test_stap_drive_state(void)
{
    playerObject stapPlayer;
    sceneObject scene;
    physicsObject physics;
    Mnode stapNode;
    Mnode jediNode;
    int32_t cpad[2] = {0, 0x40};

    memset(&stapPlayer, 0, sizeof(stapPlayer));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&stapNode, 0, sizeof(stapNode));
    memset(&jediNode, 0, sizeof(jediNode));
    stapPlayer.playerRoot.pParent = &scene.sceneRoot;
    stapPlayer.playernum = 6;
    scene.pPhysics = &physics.physicsRoot;
    stapNode.id = NODE_DYNAMIC;
    jediNode.id = NODE_DYNAMIC;
    stapbikeindex[0] = 7;
    stapbikeindex[1] = 0;
    GameStruct.GameState = UINT32_C(0x02000000);
    GameStruct.NumPlayers = 1;
    g_p1X = 0.0f;
    g_p1Y = 0.0f;
    gGlobalFrameRate = 0x1000;
    jpb_CubeRuntimeFlags = 0;
    zerobss_levelReset = 111;

    coll_ResetCollisionSystem();
    coll_gRegisterNode(stapPlayer.playernum, &stapNode);
    coll_gRegisterNode(0, &jediNode);
    CHECK(ai_Stap(cpad, &stapPlayer) == -1);
    CHECK((physics.flags & UINT32_C(0x00400000)) != 0);
    CHECK(physics.userdata[0] == 0xa0);
    CHECK(physics.userdata[1] == 4);
    CHECK(physics.userdata[2] == 0x80);
    CHECK(physics.angle.vy == -0x10);
    CHECK(physics.constmov.vz == 2.0f);
    CHECK(jediNode.v3RotationAbs.vx == -0x100);
    CHECK((stapNode.flags &
           JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) != 0);
    CHECK((jediNode.flags &
           JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) != 0);
    stapbikeindex[0] = 0;
    GameStruct.GameState = 0;
    return 0;
}

static int test_tank_drive_state(void)
{
    playerObject tank;
    playerObject driver;
    sceneObject scene;
    physicsObject physics;
    animObject animation;
    Motion motions[2];
    wsl_ENEMY enemy;
    wsl_BAP_PLACEMENT placement;
    Mnode nodes[3];

    memset(&tank, 0, sizeof(tank));
    memset(&driver, 0, sizeof(driver));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&animation, 0, sizeof(animation));
    memset(motions, 0, sizeof(motions));
    memset(&enemy, 0, sizeof(enemy));
    memset(&placement, 0, sizeof(placement));
    memset(nodes, 0, sizeof(nodes));
    tank.playerRoot.pParent = &scene.sceneRoot;
    tank.playernum = 5;
    tank.pEnemy = &enemy;
    tank.paMotions = motions;
    enemy.pPlace = &placement;
    placement.aiDf.activeFlags = UINT32_C(0x04000000);
    scene.pPhysics = &physics.physicsRoot;
    scene.pAnim = &animation.animRoot;
    driver.playerPad.cpad[1] = UINT32_C(0x1000);
    tankdrivers[0] = &driver;
    tankdrivers[1] = NULL;
    jumpheld[0] = 0;
    nodes[0].id = (modelNodeId)(NODE_DYNAMIC | 7);
    nodes[1].id = (modelNodeId)(NODE_DYNAMIC | 8);
    nodes[2].id = (modelNodeId)(NODE_DYNAMIC | 9);
    gGlobalFrameRate = 0x1000;
    tanknoise = 0;
    turretnoise = 0;
    zerobss_levelReset = 112;

    coll_ResetCollisionSystem();
    coll_gRegisterNode(tank.playernum, &nodes[0]);
    coll_gRegisterNode(tank.playernum, &nodes[1]);
    coll_gRegisterNode(tank.playernum, &nodes[2]);
    CHECK(ai_Tank(NULL, &tank) == -1);
    CHECK(playertankindex == tank.playernum + 1);
    CHECK(physics.trajectory == 0x7ffe);
    CHECK(physics.constmov.vz == 6.0f);
    CHECK((driver.pFlags & UINT32_C(0x80)) != 0);
    CHECK((nodes[0].flags & UINT32_C(0x20000000)) != 0);
    CHECK((nodes[1].flags & UINT32_C(0x20000000)) != 0);
    CHECK((nodes[2].flags & UINT32_C(0x20000000)) != 0);
    tankdrivers[0] = NULL;
    playertankindex = 0;
    return 0;
}

static int test_machinegun_target_selection(void)
{
    playerObject tank;
    sceneObject tankScene;
    physicsObject tankPhysics;
    sceneObject targetScene;
    physicsObject targetPhysics;
    wsl_ENEMY targetEnemy;
    wsl_BAP_PLACEMENT targetPlacement;
    VECTOR muzzle = {0, 0, 0, 0};
    VECTOR facing = {0, 0, 0, 0};
    int i;

    memset(&tank, 0, sizeof(tank));
    memset(&tankScene, 0, sizeof(tankScene));
    memset(&tankPhysics, 0, sizeof(tankPhysics));
    memset(&targetScene, 0, sizeof(targetScene));
    memset(&targetPhysics, 0, sizeof(targetPhysics));
    memset(&targetEnemy, 0, sizeof(targetEnemy));
    memset(&targetPlacement, 0, sizeof(targetPlacement));
    for (i = 2; i < JPB_PLAYER_CAPACITY; ++i) {
        gaPlayerData[i].playerRoot.objectID = -1;
    }
    tank.playerRoot.objectID = 10;
    tank.playerRoot.pParent = &tankScene.sceneRoot;
    tankScene.pPhysics = &tankPhysics.physicsRoot;
    gaPlayerData[2].playerRoot.objectID = 2;
    gaPlayerData[2].playerRoot.pParent = &targetScene.sceneRoot;
    gaPlayerData[2].pEnemy = &targetEnemy;
    targetScene.pScene = &targetScene.sceneRoot;
    targetEnemy.pPlace = &targetPlacement;
    targetPlacement.aiDf.daDelay = 2;
    targetScene.pPhysics = &targetPhysics.physicsRoot;
    targetPhysics.pos.vz = 100.0f;
    GameStruct.aCharacterData[2].Energy = 10;
    maRange[2][10] = 100.0f;

    CHECK(FindBestMachineGunTarget(
              &muzzle, &facing, &tank, 0x500, 0xc8, 0x100, 0) ==
          &gaPlayerData[2]);
    return 0;
}

int main(void)
{
    CHECK(test_deadly_callback() == 0);
    CHECK(test_jar_jar_callback() == 0);
    CHECK(test_kadu_level_gate() == 0);
    CHECK(test_kadu_race_state() == 0);
    CHECK(test_kadu_race_hud_bars() == 0);
    CHECK(test_maul_callback() == 0);
    CHECK(test_thug_callback_setup() == 0);
    CHECK(test_turret_droid_idle_state() == 0);
    CHECK(test_maul_callback_windows() == 0);
    CHECK(test_sphere_callbacks() == 0);
    CHECK(test_mtt_damage_volume() == 0);
    CHECK(test_blades_callback() == 0);
    CHECK(test_centre_turret() == 0);
    CHECK(test_aat_turret_tracking() == 0);
    CHECK(test_stap_drive_state() == 0);
    CHECK(test_tank_drive_state() == 0);
    CHECK(test_machinegun_target_selection() == 0);
    puts("boss/vehicle tests passed");
    return 0;
}
