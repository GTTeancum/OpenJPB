#include "jpb/objroot.h"
#include "jpb/alloc.h"
#include "jpb/anim.h"
#include "jpb/camera.h"
#include "jpb/console.h"
#include "jpb/enemy.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/level.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/prim.h"
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

static int test_scene_pool(void)
{
    sceneObject before;
    sceneObject *scene;
    int index;

    memset(maSceneData, 0xa5, sizeof(maSceneData));
    jpb_SceneInitPool(0);
    for (index = 0; index < JPB_SCENE_CAPACITY; ++index) {
        CHECK(maSceneData[index].sceneRoot.pParent == NULL);
        CHECK(maSceneData[index].sceneRoot.flags == 0);
        CHECK(maSceneData[index].sceneRoot.objectID == -1);
        CHECK(maSceneData[index].pPhysics == NULL);
        CHECK(maSceneData[index].pPlayer == NULL);
        CHECK(maSceneData[index].m3LocalModelMatrix.t[2] == 0.0f);
    }

    scene = scene_gGetNewSceneObject(4);
    CHECK(scene == &maSceneData[4]);
    CHECK(scene->sceneRoot.objectID == 4);
    CHECK(scene_gGetNewSceneObject(4) == NULL);
    scene = scene_gGetNewSceneObject(-1);
    CHECK(scene == &maSceneData[0]);
    CHECK(scene->sceneRoot.objectID == 0);
    CHECK(scene_gGetNewSceneObject(JPB_SCENE_CAPACITY) == NULL);

    memset(&before, 0x6b, sizeof(before));
    maSceneData[9] = before;
    maSceneData[10] = before;
    jpb_SceneInitPool(10);
    CHECK(memcmp(&maSceneData[9], &before, sizeof(before)) == 0);
    CHECK(maSceneData[10].sceneRoot.objectID == -1);

    before = maSceneData[0];
    jpb_SceneInitPool(-1);
    CHECK(memcmp(&maSceneData[0], &before, sizeof(before)) == 0);
    return 0;
}

static int test_scene_root_and_console_initialization(void)
{
    memset(&gSceneRoot, 0xa5, sizeof(gSceneRoot));
    gSCENE_READY = 1;
    gCamera.viewType = 0;

    scene_gInitRoot();
    CHECK(gSCENE_READY == 0);
    CHECK(gSceneRoot.paSceneModels == maSceneData);
    CHECK(gSceneRoot.GeometryEnv.angle.vx == 0);
    CHECK(gSceneRoot.GeometryEnv.matrix.m[0][0] == 0.0f);
    CHECK(gSceneRoot.pCamera == NULL);
    CHECK(gSceneRoot.camType == 0);
    CHECK(gCamera.viewType == 0x901);

    memset(maSceneData, 0xa5, sizeof(maSceneData));
    jpb_ConsoleResetCommands();
    scene_gInitScenes(5);
    CHECK(maSceneData[4].sceneRoot.objectID != -1);
    CHECK(maSceneData[5].sceneRoot.objectID == -1);
    CHECK(maSceneData[19].sceneRoot.objectID == -1);
    CHECK(jpb_ConsoleCommandCount() == 3);
    CHECK(strcmp(jpb_ConsoleCommandName(0), "cameras") == 0);
    CHECK(strcmp(jpb_ConsoleCommandShortName(0), "cam") == 0);
    CHECK(jpb_ConsoleCommandHandler(0) == console_CamerasCommand);
    CHECK(strcmp(jpb_ConsoleCommandName(1), "anim") == 0);
    CHECK(jpb_ConsoleCommandHandler(1) == console_AnimCommand);
    CHECK(strcmp(jpb_ConsoleCommandName(2), "enemy") == 0);
    CHECK(jpb_ConsoleCommandHandler(2) == console_EnemyCommand);
    CHECK(console_AddCommand(
        "CAMERAS", "different", console_EnemyCommand) == 1);
    CHECK(jpb_ConsoleCommandCount() == 3);
    scene_gInitScenes(0);
    CHECK(jpb_ConsoleCommandCount() == 3);
    CHECK(jpb_ConsoleCommandName(3) == NULL);
    return 0;
}

static int test_scene_create_exhaustion(void)
{
    int index;

    jpb_SceneInitPool(0);
    for (index = 0; index < JPB_SCENE_CAPACITY; ++index) {
        maSceneData[index].sceneRoot.objectID = index;
    }
    gCurrentSceneObject = 7;
    CHECK(scene_gCreateObject("unused", NULL, -1) == NULL);
    CHECK(scene_gCreateObject(
        "unused", NULL, JPB_SCENE_CAPACITY) == NULL);
    CHECK(gCurrentSceneObject == 7);
    return 0;
}

static int test_scene_matrix_accessors(void)
{
    _svector short_angle = {11, 22, 33, 44};
    _svector screen_position = {0, 0, 10, 45};
    VECTOR long_angle = {101, 202, 303, 404};
    VECTOR long_position = {40000, -40000, 70000, 405};
    VECTOR position = {0, 0, 0, 406};
    VECTOR snapshot = {0, 0x12345678, 0, 407};
    FVECTOR float_position = {12.75f, -34.5f, 56.875f};
    FVECTOR read_float_position = {0.0f, 0.0f, 0.0f};
    FVECTOR read_float_snapshot = {0.0f, 0.0f, 0.0f};
    MATRIX *matrix_pointer = &CameraMatrix;
    CVECTOR strobe = {1, 2, 3, 4};
    int packed_screen = 0;

    jpb_SceneInitPool(0);
    memset(&maPhysicsData[3], 0, sizeof(maPhysicsData[3]));

    scene_gSetSceneModelMatrix(3, &short_angle, &long_position);
    CHECK(maSceneData[3].v3WorldAngle.vx == 11);
    CHECK(maSceneData[3].v3WorldAngle.vy == 22);
    CHECK(maSceneData[3].v3WorldAngle.vz == 33);
    CHECK(maSceneData[3].v3WorldAngle.pad == 0);
    CHECK(maSceneData[3].v3WorldPosition.vx == -25536);
    CHECK(maSceneData[3].v3WorldPosition.vy == 25536);
    CHECK(maSceneData[3].v3WorldPosition.vz == 4464);

    maSceneData[3].v3SnapShotPosition.vx = 700;
    maSceneData[3].v3SnapShotPosition.vy = 800;
    maSceneData[3].v3SnapShotPosition.vz = 900;
    short_angle.vx = -1;
    short_angle.vy = -2;
    short_angle.vz = -3;
    scene_gGetSceneModelMatrix(
        3, &short_angle, &position, &snapshot);
    CHECK(short_angle.vx == -1);
    CHECK(short_angle.vy == -2);
    CHECK(short_angle.vz == -3);
    CHECK(position.vx == -25536);
    CHECK(position.vy == 25536);
    CHECK(position.vz == 4464);
    CHECK(position.pad == 406);
    CHECK(snapshot.vx == 700);
    CHECK(snapshot.vy == 800);
    CHECK(snapshot.vz == 900);
    CHECK(snapshot.pad == 407);

    snapshot.vy = 0x12345678;
    snapshot.pad = 408;
    scene_gGetSnapShotPosition(3, &snapshot);
    CHECK(snapshot.vx == 700);
    CHECK(snapshot.vy == 0x12345678);
    CHECK(snapshot.vz == 900);
    CHECK(snapshot.pad == 408);

    scene_gSetSceneModelMatrixLV(3, &long_angle, &long_position);
    CHECK(maSceneData[3].v3WorldAngle.vx == 101);
    CHECK(maSceneData[3].v3WorldAngle.vy == 202);
    CHECK(maSceneData[3].v3WorldAngle.vz == 303);
    CHECK(maSceneData[3].v3WorldPosition.vx == -25536);
    CHECK(maSceneData[3].v3WorldPosition.vy == 25536);
    CHECK(maSceneData[3].v3WorldPosition.vz == 4464);

    scene_gSetSceneModelMatrixFV(3, &long_angle, &float_position);
    CHECK(maSceneData[3].v3WorldAngle.vx == 101);
    CHECK(maSceneData[3].v3WorldAngle.vy == 202);
    CHECK(maSceneData[3].v3WorldAngle.vz == 303);
    CHECK(maSceneData[3].v3WorldPosition.vx == 12);
    CHECK(maSceneData[3].v3WorldPosition.vy == -34);
    CHECK(maSceneData[3].v3WorldPosition.vz == 56);
    scene_gGetSceneModelMatrixFV(
        3,
        &short_angle,
        &read_float_position,
        &read_float_snapshot);
    CHECK(read_float_position.vx == 12.0f);
    CHECK(read_float_position.vy == -34.0f);
    CHECK(read_float_position.vz == 56.0f);
    CHECK(read_float_snapshot.vx == 700.0f);
    CHECK(read_float_snapshot.vy == 800.0f);
    CHECK(read_float_snapshot.vz == 900.0f);

    scene_gSetWorldPosition(3, &long_position);
    CHECK(maSceneData[3].v3WorldPosition.vx == -25536);
    CHECK(maSceneData[3].v3WorldPosition.vy == 25536);
    CHECK(maSceneData[3].v3WorldPosition.vz == 4464);
    CHECK(maSceneData[3].v3SnapShotPosition.vx == -25536);
    CHECK(maSceneData[3].v3SnapShotPosition.vy == 25536);
    CHECK(maSceneData[3].v3SnapShotPosition.vz == 4464);

    memset(&CameraMatrix, 0, sizeof(CameraMatrix));
    CameraMatrix.m[0][0] = 1.0f;
    CameraMatrix.m[1][1] = 1.0f;
    CameraMatrix.m[2][2] = 1.0f;
    scene_gProject2Screen(&screen_position, &packed_screen);
    CHECK((uint32_t)packed_screen == UINT32_C(0x00f00140));

    scene_AspectCorrectMatrix(NULL, NULL);
    scene_DimScreen();
    scene_gSetStrobe(&strobe);
    scene_preRender(&matrix_pointer);
    CHECK(matrix_pointer == &CameraMatrix);
    return 0;
}

static int test_scene_post_render_state(void)
{
    WorldData world;
    WorldData *saved_world = gpWorld;
    const CVECTOR *background0;
    const CVECTOR *background1;

    memset(&world, 0, sizeof(world));
    world.bkColor.r = 40;
    world.bkColor.g = 80;
    world.bkColor.b = 120;
    gpWorld = &world;
    GameStruct.GameState = 0;
    gGlobalFrameRate = 2048;
    gGlobalTimer = 100;
    mDrawingSurfaceId = 0;
    totalframes = 2;
    gSCENE_READY = 0;
    gSTROBE_MODE = 0;

    scene_postRender();
    CHECK(gGlobalTimer == 356);
    CHECK(mDrawingSurfaceId == 1);
    CHECK(gSCENE_READY == 1);
    CHECK(gSTROBE_MODE == 0);
    background0 = jpb_PrimGetBackgroundColor(0);
    background1 = jpb_PrimGetBackgroundColor(1);
    CHECK(background0 != NULL);
    CHECK(background1 != NULL);
    CHECK(background0->r == 30);
    CHECK(background0->g == 20);
    CHECK(background0->b == 10);
    CHECK(background1->r == 30);
    CHECK(background1->g == 20);
    CHECK(background1->b == 10);

    GameStruct.GameState = UINT32_C(0x02000000);
    gGlobalTimer = 500;
    totalframes = 1;
    gSCENE_READY = 0;
    gSTROBE_MODE = 2;
    scene_postRender();
    CHECK(gGlobalTimer == 500);
    CHECK(mDrawingSurfaceId == 0);
    CHECK(gSCENE_READY == 0);
    CHECK(gSTROBE_MODE == 1);

    gpWorld = saved_world;
    GameStruct.GameState = 0;
    return 0;
}

typedef struct SceneMiddleTrace {
    int afterAnimations;
    int afterWorld;
    int afterModels;
    int beforePlayerProcess;
    int levelCalls;
    int level;
    int arguments[3];
} SceneMiddleTrace;

static void trace_scene_middle_stage(
    void *user_data, MATRIX *matrix)
{
    SceneMiddleTrace *trace = (SceneMiddleTrace *)user_data;

    if (matrix == scene_GetSceneMatrix()) {
        ++trace->afterModels;
    } else {
        trace->afterModels = -100;
    }
}

static void trace_scene_middle_level(
    void *user_data,
    int level,
    int argument0,
    int argument1,
    int argument2)
{
    SceneMiddleTrace *trace = (SceneMiddleTrace *)user_data;

    ++trace->levelCalls;
    trace->level = level;
    trace->arguments[0] = argument0;
    trace->arguments[1] = argument1;
    trace->arguments[2] = argument2;
}

static int test_scene_middle_render_paused_owner(void)
{
    JPBSceneMiddleRenderHooks hooks;
    SceneMiddleTrace trace;

    memset(&hooks, 0, sizeof(hooks));
    memset(&trace, 0, sizeof(trace));
    meminit();
    sprite_gInitSprites();
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(&gCamera, 0, sizeof(gCamera));
    gCamera.viewType = JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    gpWorld = NULL;
    totalframes = 10;
    globaltimer = 20;
    gGlobalFrameRate = 2048;
    gSCENE_READY = 1;
    initialLevelPauseDelay = 2;
    gSTROBE_MODE = 2;
    GameStruct.GameState = UINT32_C(0x02000000);
    OptionStruct.overlayMode = 0;
    LevelSelect = 22;

    hooks.renderModels = trace_scene_middle_stage;
    hooks.levelOwner = trace_scene_middle_level;
    jpb_SceneSetMiddleRenderHooks(&hooks, &trace);
    scene_middleRender(NULL);
    jpb_SceneSetMiddleRenderHooks(NULL, NULL);

    CHECK(totalframes == 11);
    CHECK(globaltimer == 21);
    CHECK(trace.afterAnimations == 0);
    CHECK(trace.afterWorld == 0);
    CHECK(trace.afterModels == 1);
    CHECK(trace.beforePlayerProcess == 0);
    CHECK(trace.levelCalls == 0);
    CHECK(initialLevelPauseDelay == 2);
    return 0;
}

static int test_level_fed(void)
{
    physicsObject *inside = &maPhysicsData[0];
    physicsObject *outside = &maPhysicsData[1];

    jpb_SceneInitPool(0);
    memset(inside, 0, sizeof(*inside));
    memset(outside, 0, sizeof(*outside));
    inside->physicsRoot.objectID = 0;
    outside->physicsRoot.objectID = 1;
    inside->physicsRoot.pParent = &maSceneData[0].sceneRoot;
    outside->physicsRoot.pParent = &maSceneData[1].sceneRoot;
    maSceneData[0].pPhysics = &inside->physicsRoot;
    maSceneData[1].pPhysics = &outside->physicsRoot;
    inside->pos.vx = 12500.75f;
    inside->pos.vy = 4000.5f;
    inside->pos.vz = -9500.25f;
    outside->pos.vx = 11000.0f;
    outside->pos.vy = 4100.0f;
    outside->pos.vz = -9500.0f;
    g_levelUVScroll.vx = 0.0f;
    g_levelUVScroll.vy = 5.0f;
    GameStruct.GameState = 0;

    level_Fed();
    CHECK(inside->pos.vx == 12500.0f);
    CHECK(inside->pos.vy == 3500.0f);
    CHECK(inside->pos.vz == -9500.0f);
    CHECK(inside->lastpos.vx == 12500.0f);
    CHECK(inside->lastpos.vy == 3500.0f);
    CHECK(inside->lastpos.vz == -9500.0f);
    CHECK(outside->pos.vy == 4100.0f);
    CHECK(g_levelUVScroll.vx > 2.958f);
    CHECK(g_levelUVScroll.vx < 2.960f);
    CHECK(g_levelUVScroll.vy == 0.0f);

    GameStruct.GameState = UINT32_C(0x02000000);
    g_levelUVScroll.vx = 1.25f;
    g_levelUVScroll.vy = 2.5f;
    level_Fed();
    CHECK(g_levelUVScroll.vx == 1.25f);
    CHECK(g_levelUVScroll.vy == 2.5f);
    GameStruct.GameState = 0;
    return 0;
}

static int test_level_corus(void)
{
    physicsObject *inside = &maPhysicsData[0];
    physicsObject *outside = &maPhysicsData[1];

    jpb_SceneInitPool(0);
    memset(inside, 0, sizeof(*inside));
    memset(outside, 0, sizeof(*outside));
    inside->physicsRoot.objectID = 0;
    outside->physicsRoot.objectID = 1;
    inside->physicsRoot.pParent = &maSceneData[0].sceneRoot;
    outside->physicsRoot.pParent = &maSceneData[1].sceneRoot;
    maSceneData[0].pPhysics = &inside->physicsRoot;
    maSceneData[1].pPhysics = &outside->physicsRoot;
    inside->pos.vx = 11500.75f;
    inside->pos.vy = 9000.5f;
    inside->pos.vz = -18500.25f;
    outside->pos.vx = 10500.0f;
    outside->pos.vy = 9000.0f;
    outside->pos.vz = -18500.0f;

    level_Corus();
    CHECK(inside->pos.vx == 11500.0f);
    CHECK(inside->pos.vy == 9000.0f);
    CHECK(inside->pos.vz == -17875.0f);
    CHECK(inside->lastpos.vx == 11500.0f);
    CHECK(inside->lastpos.vy == 9000.0f);
    CHECK(inside->lastpos.vz == -17875.0f);
    CHECK(outside->pos.vz == -18500.0f);
    return 0;
}

static int test_level_palace(void)
{
    WorldData world;
    wsl_BAP_PLACEMENT first_placement;
    wsl_BAP_PLACEMENT second_placement;
    wsl_BAP_PLACEMENT *placements[165];
    wsl_ENEMY boss;
    physicsObject *player_physics = &maPhysicsData[0];
    physicsObject *boss_physics = &maPhysicsData[5];
    playerObject *player = &gaPlayerData[0];
    playerObject *boss_player = &gaPlayerData[5];

    memset(&world, 0, sizeof(world));
    memset(&first_placement, 0, sizeof(first_placement));
    memset(&second_placement, 0, sizeof(second_placement));
    memset(placements, 0, sizeof(placements));
    memset(&boss, 0, sizeof(boss));
    jpb_SceneInitPool(0);
    memset(maPhysicsData, 0, sizeof(maPhysicsData));
    memset(gaPlayerData, 0, sizeof(gaPlayerData));
    pointerRegistry_Reset();

    player_physics->physicsRoot.objectID = 0;
    player_physics->physicsRoot.pParent =
        &maSceneData[0].sceneRoot;
    maSceneData[0].pPhysics =
        &player_physics->physicsRoot;
    player->playerRoot.objectID = 0;
    player->playerRoot.pParent =
        &maSceneData[0].sceneRoot;
    player_physics->pos.vx = 13300.75f;
    player_physics->pos.vy = 4500.5f;
    player_physics->pos.vz = -7500.25f;
    player_physics->vpos.vx = 1000;

    boss_physics->physicsRoot.objectID = 5;
    boss_physics->physicsRoot.pParent =
        &maSceneData[5].sceneRoot;
    maSceneData[5].pPhysics = &boss_physics->physicsRoot;
    boss_player->playerRoot.objectID = 5;
    boss_player->playerRoot.pParent =
        &maSceneData[5].sceneRoot;
    boss_player->playernum = 5;
    boss.pPlayer = boss_player;
    boss.location.vx = -8000;

    first_placement.status = 1;
    first_placement.pLastEnemy = (uint32_t)addPtr(
        &boss, JPB_POINTER_ARRAY_ENEMY);
    placements[163] = &first_placement;
    placements[164] = &second_placement;
    world.apEnemy = placements;
    world.player0 = player;
    gpWorld = &world;
    maRange[0][5] = -1;
    (void)game_gSetEnergy(5, 100);

    level_Palace();
    CHECK(player_physics->pos.vx == 13300.0f);
    CHECK(player_physics->pos.vy == 4500.0f);
    CHECK(player_physics->pos.vz == -7601.0f);
    CHECK(game_gGetEnergy(5) == 0);

    boss.location.vx = -9000;
    (void)game_gSetEnergy(5, 100);
    level_Palace_KillOffscreenBoss(163);
    CHECK(game_gGetEnergy(5) == 100);

    gpWorld = NULL;
    pointerRegistry_Reset();
    return 0;
}

static int test_standingonit(void)
{
    _svector upper = {100, 200, 300, 0};
    _svector lower = {-100, 0, 0, 0};
    playerObject *player = &gaPlayerData[0];
    physicsObject *physics = &maPhysicsData[0];

    memset(player, 0, sizeof(*player));
    memset(physics, 0, sizeof(*physics));
    physics->pos.vx = 0.0f;
    physics->pos.vy = 200.0f;
    physics->pos.vz = 365.0f;
    CHECK(standingonit(0, &upper, &lower) == 1);

    physics->pos.vz = 366.0f;
    CHECK(standingonit(0, &upper, &lower) == 0);
    physics->pos.vz = 300.0f;
    player->pFlags = 1;
    CHECK(standingonit(0, &upper, &lower) == 0);
    player->pFlags = 0;
    player->playerID = 0x48;
    CHECK(standingonit(0, &upper, &lower) == 0);
    player->playerID = 0;
    physics->pos.vx = -100.0f;
    CHECK(standingonit(0, &upper, &lower) == 0);
    return 0;
}

static int test_child_links(void)
{
    sceneObject scene;
    objectRoot children[5];
    objectRoot existing_parent;
    int type;

    memset(&scene, 0, sizeof(scene));
    memset(children, 0, sizeof(children));
    memset(&existing_parent, 0, sizeof(existing_parent));
    children[4].pParent = &existing_parent;

    for (type = 0; type < 5; ++type) {
        CHECK(obj_gSetChildObject(&scene, &children[type], type) == NULL);
    }
    CHECK(scene.sceneRoot.pParent == &children[0]);
    CHECK(scene.pScene == &children[0]);
    CHECK(scene.pModel == &children[1]);
    CHECK(scene.pPhysics == &children[2]);
    CHECK(scene.pAnim == &children[3]);
    CHECK(scene.pPlayer == &children[4]);
    CHECK(children[0].pParent == &scene.sceneRoot);
    CHECK(children[1].pParent == &scene.sceneRoot);
    CHECK(children[2].pParent == &scene.sceneRoot);
    CHECK(children[3].pParent == &scene.sceneRoot);
    CHECK(children[4].pParent == &existing_parent);

    children[0].flags = 0x20u;
    children[2].flags = 0x40u;
    CHECK(obj_gCheckObjectFlag(&children[1], 0, 0x20u) == 1);
    CHECK(obj_gCheckObjectFlag(&children[1], 0, 0x40u) == 0);
    CHECK(obj_gCheckObjectFlag(&children[1], 2, 0x40u) == 1);
    obj_gSetObjectFlag(&children[1], 1, 0x24u);
    CHECK(children[1].flags == 0x24u);
    obj_gSetObjectFlag(&children[1], 1, 0x40u);
    CHECK(children[1].flags == 0x64u);
    obj_gSetObjectFlag(&children[1], 1, 0x20u);
    CHECK(children[1].flags == 0x64u);
    obj_gClrObjectFlag(&children[1], 1, 0x24u);
    CHECK(children[1].flags == 0x40u);
    obj_gClrObjectFlag(&children[1], 2, 0x40u);
    CHECK(children[2].flags == 0);
    children[1].pParent = NULL;
    CHECK(obj_gCheckObjectFlag(&children[1], 0, 0x20u) == 0);

    scene.pPlayer = NULL;
    CHECK(obj_gSetChildObject(&scene, &children[4], 99) == NULL);
    CHECK(scene.pPlayer == NULL);
    CHECK(children[4].pParent == &existing_parent);
    return 0;
}

static int test_scene_key_frame_publication(void)
{
    _animFrame frame;

    memset(&frame, 0, sizeof(frame));
    jpb_SceneInitPool(0);
    scene_gSetSceneModelKeyFrame(3, &frame);
    CHECK(maSceneData[3].pKeyFrameModel == &frame);
    CHECK(maSceneData[2].pKeyFrameModel == NULL);
    return 0;
}

static int test_clear_complete_object(void)
{
    sceneObject scene;
    objectRoot model;
    objectRoot physics;
    objectRoot animation;
    objectRoot player;

    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&physics, 0, sizeof(physics));
    memset(&animation, 0, sizeof(animation));
    memset(&player, 0, sizeof(player));
    scene.sceneRoot.flags = UINT32_C(0xffffffff);
    scene.sceneRoot.objectID = 4;
    model.objectID = 4;
    physics.objectID = 4;
    animation.objectID = 4;
    player.objectID = 4;
    player.pParent = &scene.sceneRoot;
    scene.pModel = &model;
    scene.pPhysics = &physics;
    scene.pAnim = &animation;
    scene.pPlayer = &player;

    CHECK(obj_gClearObject(&player) == NULL);
    CHECK(scene.sceneRoot.flags == 0);
    CHECK(scene.sceneRoot.objectID == -1);
    CHECK(model.objectID == -1);
    CHECK(physics.objectID == -1);
    CHECK(animation.objectID == -1);
    CHECK(player.objectID == -1);
    return 0;
}

static int hurt_sound_calls;
static int hurt_sound_bank;

static uint16_t test_hurt_sound(
    VECTOR *position,
    int bankId,
    char *sound,
    uint32_t flag,
    void *user_data)
{
    (void)user_data;
    CHECK(position == NULL);
    CHECK(strcmp(sound, "jedihit") == 0);
    CHECK(flag == 3);
    ++hurt_sound_calls;
    hurt_sound_bank = bankId;
    return 77;
}

static int test_hurtplayer_nonfatal_and_death(void)
{
    sceneObject scene;
    physicsObject physics;
    modelObject model;
    playerObject player;
    wsl_ENEMY enemy;
    int32_t lock_ring[16];
    Sprite shadow;
    SCB scb;

    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&model, 0, sizeof(model));
    memset(&player, 0, sizeof(player));
    memset(&enemy, 0, sizeof(enemy));
    memset(lock_ring, 0xa5, sizeof(lock_ring));
    memset(&shadow, 0, sizeof(shadow));
    memset(&scb, 0, sizeof(scb));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(gaPlayerData, 0, sizeof(gaPlayerData));
    memset(timesincetank, 0, sizeof(timesincetank));
    memset(tankdrivers, 0, sizeof(tankdrivers));

    scene.pScene = &scene.sceneRoot;
    scene.pPhysics = &physics.physicsRoot;
    scene.pModel = &model.modelRoot;
    scene.pPlayer = &player.playerRoot;
    player.playerRoot.pParent = &scene.sceneRoot;
    player.playerRoot.objectID = 0;
    player.playernum = 0;
    player.pFlags = 0x80;
    player.hitMotion = (Motion *)(uintptr_t)1;
    physics.flags = UINT32_MAX;
    physics.falltimer = 123;
    physics.movemode = MOVE_FLY;
    model.flags = UINT32_MAX;
    GameStruct.aCharacterData[0].Energy = 10;
    GameStruct.aCharacterData[0].MaxEnergy = 10;
    GameStruct.aCharacterData[0].Items = 4;
    GameStruct.aCharacterData[0].PowerType = 2;
    GameStruct.aCharacterData[0].PowerLevel = 100;
    gGlobalTimer = 500;
    LevelSelect = 0;
    hurt_sound_calls = 0;
    jpb_SoundSetPlaySfxHook(test_hurt_sound, NULL);

    hurtplayer(&player, -3);
    CHECK(GameStruct.aCharacterData[0].Energy == 7);
    CHECK(scene.sceneRoot.flags == 0);
    CHECK(hurt_sound_calls == 0);

    player.lockRing = lock_ring;
    player.shadow = (int32_t *)(void *)&shadow;
    shadow.sp_SCB = &scb;
    shadow.sp_Flags = 0x20;
    scb.scb_flags = 0x40;
    enemy.enemyFlags = (UINT32_C(1) << 26) | 4;
    gaPlayerData[0].pEnemy = &enemy;
    playertankindex = 1;
    tankdrivers[0] = &player;

    hurtplayer(&player, -20);
    CHECK(GameStruct.aCharacterData[0].Energy == 0);
    CHECK(GameStruct.GameState == 0x20u);
    CHECK(scene.sceneRoot.flags == 0x20u);
    CHECK(player.pFlags == 0x200u);
    CHECK(player.hitNumber == 1);
    CHECK(player.hitMotion == NULL);
    CHECK(GameStruct.aCharacterData[0].Items == 0);
    CHECK(GameStruct.aCharacterData[0].PowerType == 0);
    CHECK(GameStruct.aCharacterData[0].PowerLevel == 500);
    CHECK(lock_ring[15] == 0);
    CHECK(player.lockRing == NULL);
    CHECK(player.shadow == NULL);
    CHECK(shadow.sp_Flags == 0x21);
    CHECK(scb.scb_flags == 0x41);
    CHECK(physics.falltimer == 0);
    CHECK(physics.flags == UINT32_C(0xffffff40));
    CHECK(physics.movemode == MOVE_NORMAL);
    CHECK(model.flags == UINT32_C(0xffffffef));
    CHECK(hurt_sound_calls == 1);
    CHECK(hurt_sound_bank == 3);
    CHECK(playertankindex == 0);
    CHECK(tankdrivers[0] == NULL);
    CHECK(timesincetank[0] == 0x1e000);
    CHECK(enemy.enemyFlags == 4);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    return 0;
}

int main(void)
{
    CHECK(test_scene_pool() == 0);
    CHECK(test_scene_root_and_console_initialization() == 0);
    CHECK(test_scene_create_exhaustion() == 0);
    CHECK(test_scene_matrix_accessors() == 0);
    CHECK(test_scene_post_render_state() == 0);
    CHECK(test_scene_middle_render_paused_owner() == 0);
    CHECK(test_level_fed() == 0);
    CHECK(test_level_corus() == 0);
    CHECK(test_level_palace() == 0);
    CHECK(test_standingonit() == 0);
    CHECK(test_child_links() == 0);
    CHECK(test_scene_key_frame_publication() == 0);
    CHECK(test_clear_complete_object() == 0);
    CHECK(test_hurtplayer_nonfatal_and_death() == 0);
    puts("object/scene tests passed");
    return 0;
}
