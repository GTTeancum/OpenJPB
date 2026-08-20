#include "jpb/player.h"
#include "jpb/alloc.h"
#include "jpb/anim.h"
#include "jpb/bullet.h"
#include "jpb/brainutl.h"
#include "jpb/braindmg.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/combo.h"
#include "jpb/debugtext.h"
#include "jpb/effects.h"
#include "jpb/game.h"
#include "jpb/fx.h"
#include "jpb/input.h"
#include "jpb/jedi.h"
#include "jpb/jonny.h"
#include "jpb/level_world.h"
#include "jpb/loader.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/wrender.h"
#include "jpb/whook.h"
#include "jpb/world.h"

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

#define CHECK_FLOAT_CLOSE(actual, expected, tolerance)                       \
    do {                                                                     \
        float jpb_actual_value = (actual);                                    \
        float jpb_expected_value = (expected);                                \
        float jpb_delta = jpb_actual_value - jpb_expected_value;              \
        if (jpb_delta < 0.0f) {                                               \
            jpb_delta = -jpb_delta;                                           \
        }                                                                    \
        if (jpb_delta > (tolerance)) {                                        \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK_FLOAT_CLOSE failed at %s:%d: %s=%f expected %f\n",    \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #actual,                                                     \
                (double)jpb_actual_value,                                    \
                (double)jpb_expected_value);                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static uint32_t raw_pad_bits;
static int player_init_calls;
static int player_init_saw_constructor_state;

typedef struct SabreGlowTrace {
    _svector start[8];
    _svector end[8];
    int width[8];
    uint32_t color[8];
    int count;
} SabreGlowTrace;

typedef struct SabreCylinderTrace {
    VECTOR location[8];
    _svector rotation[8];
    float radius1[8];
    float radius2[8];
    float height1[8];
    float height2[8];
    uint32_t color1[8];
    uint32_t color2[8];
    int count;
} SabreCylinderTrace;

typedef struct PlayerTileTrace {
    FVECTOR position[8];
    float width[8];
    float height[8];
    uint32_t color[8];
    float projectionDepth[8];
    int count;
} PlayerTileTrace;

typedef struct ScreenDrawTrace {
    SCREENRECT destination[8];
    CVECTOR color[8];
    int count;
} ScreenDrawTrace;

typedef struct Draw3dTrace {
    float x[8];
    float y[8];
    float z[8];
    float scale[8];
    uint32_t color[8];
    char text[8][256];
    int count;
} Draw3dTrace;

typedef struct ProjectileLaunchTrace {
    int type[32];
    uint32_t flags[32];
    VECTOR start[32];
    VECTOR target[32];
    const playerObject *owner[32];
    int count;
} ProjectileLaunchTrace;

static void trace_projectile_launch(
    void *user_data,
    const Projectile *projectile,
    const playerObject *player,
    const VECTOR *start,
    const VECTOR *target)
{
    ProjectileLaunchTrace *trace =
        (ProjectileLaunchTrace *)user_data;

    if (trace->count < 32) {
        trace->type[trace->count] = projectile->pj_Type;
        trace->flags[trace->count] = (uint32_t)projectile->pj_Flags;
        trace->start[trace->count] = *start;
        trace->target[trace->count] = *target;
        trace->owner[trace->count] = player;
    }
    ++trace->count;
}

static void trace_screen_draw(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth)
{
    ScreenDrawTrace *trace = (ScreenDrawTrace *)user_data;

    (void)texture;
    (void)source;
    (void)layer_depth;
    if (trace->count < 8) {
        trace->destination[trace->count] = *destination;
        trace->color[trace->count] = color;
    }
    ++trace->count;
}

static void trace_draw3d_text(
    float x,
    float y,
    float z,
    float scale,
    uint32_t color,
    const char *text,
    void *user_data)
{
    Draw3dTrace *trace = (Draw3dTrace *)user_data;

    if (trace->count < 8) {
        trace->x[trace->count] = x;
        trace->y[trace->count] = y;
        trace->z[trace->count] = z;
        trace->scale[trace->count] = scale;
        trace->color[trace->count] = color;
        (void)snprintf(
            trace->text[trace->count],
            sizeof(trace->text[trace->count]),
            "%s",
            text != NULL ? text : "");
    }
    ++trace->count;
}

static void trace_player_tile(
    void *user_data,
    const FVECTOR *position,
    float width,
    float height,
    uint32_t color,
    float projection_depth)
{
    PlayerTileTrace *trace = (PlayerTileTrace *)user_data;

    if (trace->count < 8) {
        trace->position[trace->count] = *position;
        trace->width[trace->count] = width;
        trace->height[trace->count] = height;
        trace->color[trace->count] = color;
        trace->projectionDepth[trace->count] =
            projection_depth;
    }
    ++trace->count;
}

static void trace_sabre_glow(
    void *user_data,
    const _svector *start,
    const _svector *end,
    int width,
    uint32_t color)
{
    SabreGlowTrace *trace = (SabreGlowTrace *)user_data;

    if (trace->count < 8) {
        trace->start[trace->count] = *start;
        trace->end[trace->count] = *end;
        trace->width[trace->count] = width;
        trace->color[trace->count] = color;
    }
    ++trace->count;
}

static void trace_sabre_cylinder(
    void *user_data,
    const VECTOR *location,
    const _svector *rotation,
    float radius1,
    float radius2,
    float height1,
    float height2,
    uint32_t color1,
    uint32_t color2)
{
    SabreCylinderTrace *trace =
        (SabreCylinderTrace *)user_data;

    if (trace->count < 8) {
        trace->location[trace->count] = *location;
        trace->rotation[trace->count] = *rotation;
        trace->radius1[trace->count] = radius1;
        trace->radius2[trace->count] = radius2;
        trace->height1[trace->count] = height1;
        trace->height2[trace->count] = height2;
        trace->color1[trace->count] = color1;
        trace->color2[trace->count] = color2;
    }
    ++trace->count;
}

static uint32_t test_read_pad(int32_t pad_index, void *user_data)
{
    (void)user_data;
    return pad_index == 0 ? raw_pad_bits : 0;
}

static int test_player_initializer(playerObject *player)
{
    ++player_init_calls;
    player_init_saw_constructor_state =
        player->fLife == 0 &&
        player->fStun == 0 &&
        player->fForce == 0 &&
        player->pFlags == 0 &&
        player->playerPad.mask0 == 0 &&
        player->playerPad.mask1 == UINT32_MAX &&
        player->playerPad.oldbits0 == 0 &&
        player->playerPad.oldbits1 == 0 &&
        player->ACTION_LOCK == 0 &&
        player->pMotion != NULL;
    player->fScale = 1234;
    return 1;
}

static int test_initialization_and_access(void)
{
    playerObject before;
    int index;

    memset(gaPlayerData, 0xa5, sizeof(gaPlayerData));
    player_gInitPlayers(0);
    for (index = 0; index < JPB_PLAYER_CAPACITY; ++index) {
        playerObject *player = player_gGetPlayerPtr(index);

        CHECK(player == &gaPlayerData[index]);
        CHECK(player_GetPlayerPad(index) == &player->playerPad);
        CHECK(player->playerRoot.pParent == NULL);
        CHECK(player->playerRoot.flags == 0);
        CHECK(player->playerRoot.objectID == -1);
        CHECK(memcmp(
                  player->playerRoot.objectName,
                  "PLAYER",
                  sizeof("PLAYER")) == 0);
        CHECK(player->playerRoot.objectName[7] == 0);
        CHECK(player->maxCombos == 0x30);
        CHECK(player->playerPad.padnum == 0);
        CHECK(player->pMainCallBack == NULL);
        CHECK(player->comboUserData == 0);
    }

    memset(&before, 0x6b, sizeof(before));
    gaPlayerData[9] = before;
    gaPlayerData[10] = before;
    player_gInitPlayers(10);
    CHECK(memcmp(&gaPlayerData[9], &before, sizeof(before)) == 0);
    CHECK(gaPlayerData[10].playerRoot.objectID == -1);
    CHECK(gaPlayerData[10].maxCombos == 0x30);

    before = gaPlayerData[0];
    player_gInitPlayers(-1);
    CHECK(memcmp(&gaPlayerData[0], &before, sizeof(before)) == 0);
    return 0;
}

static int test_motion_data_connection_and_loader_names(void)
{
    union {
        uint32_t alignment;
        uint8_t bytes[32 + sizeof(Motion) * 3];
    } storage;
    playerObject player;
    Motion *motions;
    int32_t motion_offset = 32;
    int16_t motion_count = 3;

    memset(&storage, 0, sizeof(storage));
    memset(&player, 0, sizeof(player));
    memcpy(&storage.bytes[8],
           &motion_offset,
           sizeof(motion_offset));
    memcpy(&storage.bytes[0x10],
           &motion_count,
           sizeof(motion_count));
    motions = (Motion *)(void *)&storage.bytes[motion_offset];
    memcpy(motions[0].name, "sabrhitAjedihitB", 16);
    motions[0].motionFlags = UINT32_C(0x100010);
    motions[0].FunctPtr = -1;
    memcpy(motions[1].name, "ordinary", 9);
    motions[1].motionFlags = UINT32_C(0x100000);
    motions[1].FunctPtr = -1;
    memcpy(motions[2].name, "another", 8);
    motions[2].motionFlags = UINT32_C(0x10);
    motions[2].FunctPtr = -1;
    player.playerID = 17;

    player_gConnectMotionData(
        &player, (char *)(void *)storage.bytes);
    CHECK(player.paMotions == motions);
    CHECK(player.maxMotions == 3);
    CHECK(memcmp(motions[0].name, "0abrhitA0edihitB", 16) == 0);
    CHECK(motions[0].FunctPtr == 2);
    CHECK(motions[1].FunctPtr == 1);
    CHECK(motions[2].FunctPtr == 2);

    player.paMotions = NULL;
    player.maxMotions = -1;
    player_gConnectMotionData(&player, NULL);
    CHECK(player.paMotions == NULL);
    CHECK(player.maxMotions == -1);

    CHECK(JPB_MODEL_NAME_COUNT == 115);
    CHECK(JPB_ACTOR_NAME_COUNT == 115);
    CHECK(JPB_ANIMATION_NAME_COUNT == 46);
    CHECK(strcmp(loader_GetEnemyName(0), "obi_wan") == 0);
    CHECK(strcmp(loader_GetModelName(47), "droid_f") == 0);
    CHECK(strcmp(sModelNames[79], "jar_jar_playable") == 0);
    CHECK(strcmp(sModelNames[113], "sithbike") == 0);
    CHECK(strcmp(sModelNames[114], "hngdr") == 0);
    CHECK(strcmp(sObiNames[114], "ishitib") == 0);
    CHECK(strcmp(sAnimNames[26], "palace") == 0);
    CHECK(model_anim_table[114].modelID == 114);
    CHECK(model_anim_table[114].poolID == 26);
    CHECK(model_anim_table[114].poolOffset == 16);
    CHECK(loader_GetEnemyName(17) == loader_GetModelName(17));
    LevelSelect = 8;
    CHECK(loader_GetLevelName() == sLevelNames[8]);
    CHECK(loader_GetALevelName(15) == sLevelNames[15]);
    LevelSelect = 0;
    return 0;
}

static int test_pool_allocation(void)
{
    WorldData world;
    playerObject *player0;
    playerObject *player1;
    playerObject *fallback;

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    player_gInitPlayers(0);

    player1 = player_gGetNewPlayerObject(1);
    CHECK(player1 == &gaPlayerData[1]);
    CHECK(player1->playerRoot.objectID == 1);
    CHECK(world.player1 == player1);
    CHECK(world.player0 == NULL);

    player0 = player_gGetNewPlayerObject(0);
    CHECK(player0 == &gaPlayerData[0]);
    CHECK(player0->playerRoot.objectID == 0);
    CHECK(world.player0 == player0);

    fallback = player_gGetNewPlayerObject(0);
    CHECK(fallback == &gaPlayerData[2]);
    CHECK(fallback->playerRoot.objectID == 2);

    fallback = player_gGetNewPlayerObject(99);
    CHECK(fallback == &gaPlayerData[3]);
    CHECK(fallback->playerRoot.objectID == 3);
    gpWorld = NULL;
    return 0;
}

static int test_scene_player_construction(void)
{
    WorldData world;
    sceneObject *scene;
    animObject *animation;
    playerObject *player;
    playerObject *fallback;
    _animFrame frame;

    memset(&world, 0, sizeof(world));
    memset(&frame, 0, sizeof(frame));
    gpWorld = &world;
    jpb_SceneInitPool(0);
    anim_InitAnimations(0);
    player_gInitPlayers(0);
    scene = scene_gGetNewSceneObject(0);
    CHECK(scene != NULL);
    obj_gSetChildObject(scene, &scene->sceneRoot, 0);
    animation = &maAnimationData[0];
    animation->animRoot.objectID = 0;
    animation->pCurrentAnimFrame = &frame;
    obj_gSetChildObject(scene, &animation->animRoot, 3);

    gaPlayerData[0].fLife = 9;
    gaPlayerData[0].fStun = 8;
    gaPlayerData[0].fForce = 7;
    gaPlayerData[0].pFlags = UINT32_MAX;
    gaPlayerData[0].playerPad.mask0 = UINT32_MAX;
    gaPlayerData[0].playerPad.mask1 = 0;
    gaPlayerData[0].playerPad.oldbits0 = UINT32_MAX;
    gaPlayerData[0].playerPad.oldbits1 = UINT32_MAX;
    gaPlayerData[0].ACTION_LOCK = 99;
    player_init_calls = 0;
    player_init_saw_constructor_state = 0;

    player = player_gCreateObject(
        scene, 17, test_player_initializer);
    CHECK(player == &gaPlayerData[0]);
    CHECK(player->playerRoot.objectID == 0);
    CHECK(player->playerRoot.pParent == &scene->sceneRoot);
    CHECK(scene->pPlayer == &player->playerRoot);
    CHECK(world.player0 == player);
    CHECK(world.player1 == NULL);
    CHECK(player->playernum == 0);
    CHECK(player->playerID == 17);
    CHECK(player->pMotion == &animation->pMotion);
    CHECK(scene->pKeyFrameModel == &frame);
    CHECK(player_init_calls == 1);
    CHECK(player_init_saw_constructor_state == 1);
    CHECK(player->fScale == 1234);

    fallback = player_gCreateObject(scene, 23, NULL);
    CHECK(fallback == &gaPlayerData[1]);
    CHECK(fallback->playerRoot.objectID == 1);
    CHECK(fallback->playernum == 0);
    CHECK(fallback->playerID == 23);
    CHECK(scene->pPlayer == &fallback->playerRoot);
    CHECK(world.player0 == player);
    CHECK(world.player1 == NULL);
    gpWorld = NULL;
    return 0;
}

static int test_jedi_initialization_and_main_callback(void)
{
    sceneObject scene;
    modelObject model;
    playerObject player;
    Mnode force_node;
    int32_t cpad = 0;
    int loader_unlocked;

    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&player, 0, sizeof(player));
    memset(&force_node, 0, sizeof(force_node));
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    loader_unlocked = GetCharacterByID(loader_model)->Unlocked;
    GetCharacterByID(loader_model)->Unlocked = 0;
    CHECK(jedi_HasProgression(obi_wan_model) == 1);
    CHECK(jedi_HasProgression(battle_d_model) == 0);
    CHECK(jedi_HasProgression(loader_model) == 1);
    GetCharacterByID(loader_model)->Unlocked = 1;
    CHECK(jedi_HasProgression(loader_model) == 0);
    GetCharacterByID(loader_model)->Unlocked = loader_unlocked;
    CHECK(jedi_IsMelee(obi_wan_model) == 0);
    CHECK(jedi_IsMelee(pilot_model) == 1);
    CHECK(jedi_IsMelee(loader_model) == 1);
    CHECK(jedi_IsMelee(jar_jar_playable_model) == 1);
    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pModel = &model.modelRoot;

    player.playernum = 0;
    player.playerID = 0;
    CHECK(jedi_InitPlayer(&player) == 0);
    CHECK(model.v3Scale.vx == 0x78a);
    CHECK(model.v3Scale.vy == 0x78a);
    CHECK(model.v3Scale.vz == 0x78a);
    CHECK(player.fScale == 0x78a);
    CHECK(player.paCombos == combos1);
    CHECK(player.numCollisionNodes == 19);
    CHECK(player.paNodesSizes[0].radius1 == 0x40);
    CHECK(player.paNodesSizes[16].id == 0x11);
    CHECK(player.pMainCallBack == jedi_Main);
    CHECK(player.pSettings.JumpVel == 0x7a);
    CHECK(player.pSettings.minClosingDist == 0x14);
    CHECK(strcmp(combos1[0].String, "n") == 0);
    CHECK(combos1[0].kdmax == 20);
    CHECK(combos1[18].Index == 53);
    CHECK(strcmp(combos1[18].String, "s.s.n.w.w.n") == 0);
    CHECK(combos1[19].Len == -1);
    CHECK(combos1[20].Len == 0);
    CHECK(memcmp(combos1, combos2, sizeof(combos1)) == 0);

    player.playernum = 1;
    player.playerID = 1;
    CHECK(jedi_InitPlayer(&player) == 0);
    CHECK(player.paCombos == combos2);
    CHECK(player.numCollisionNodes == 19);
    CHECK(player.paNodesSizes[12].radius1 == 0x60);
    CHECK(player.paNodesSizes[16].id == 0x14);

    player.playerID = 3;
    CHECK(jedi_InitPlayer(&player) == 0);
    CHECK(player.paNodesSizes[16].id == 0x13);

    player.playerID = 5;
    CHECK(jedi_InitPlayer(&player) == 0);
    CHECK(player.numCollisionNodes == 22);
    CHECK(player.paNodesSizes[19].id == 0x13);
    CHECK((uint16_t)jediUpgrades[5].forcePowers ==
          UINT16_C(0xf800));

    player.playerID = 6;
    CHECK(jedi_InitPlayer(&player) == 0);
    CHECK(player.numCollisionNodes == 16);
    CHECK(player.paNodesSizes[11].radius1 == 0x60);

    player.playerID = 10;
    CHECK(jedi_InitPlayer(&player) == 0);
    CHECK(player.paNodesSizes == maGunganNodeSizes);

    player.playerID = 0x1a;
    CHECK(jedi_InitPlayer(&player) == 0);
    CHECK(player.numCollisionNodes == 8);
    CHECK(player.paNodesSizes[7].id == 0x12);

    model.flags = 0;
    player.playerID = 0x1e;
    CHECK(jedi_InitPlayer(&player) == 0);
    CHECK(player.numCollisionNodes == 9);
    CHECK(player.paNodesSizes[0].radius1 == 0x100);
    CHECK((model.flags & UINT32_C(0x00000008)) != 0);

    player.playerID = 0x33;
    CHECK(jedi_InitPlayer(&player) == 0);
    CHECK(player.numCollisionNodes == 16);
    CHECK(model.v3Scale.vx == 0x9cd);
    CHECK(player.paNodesSizes[11].radius1 == 0x80);

    coll_ResetCollisionSystem();
    force_node.id = (modelNodeId)(NODE_DYNAMIC | 12);
    coll_gRegisterNode(0, &force_node);
    player.playernum = 0;
    player.playerID = 2;
    player.currentMotion = 0;
    player.pFlags = UINT32_C(0x2004);
    player.forceFlags = UINT32_C(0x0092);
    player.forceData[0] = 123;
    force_node.flags = UINT32_C(0x04000021);
    force_node.v3Velocity2.vx = 1;
    force_node.v3Velocity2.vy = 2;
    force_node.v3Velocity2.vz = 3;
    force_node.v3Translation2.vx = 4;
    force_node.v3Translation2.vy = 5;
    force_node.v3Translation2.vz = 6;
    CHECK(jedi_Main(&cpad, &player) == 0);
    CHECK(player.pFlags == UINT32_C(0x0004));
    CHECK(player.forceFlags == UINT32_C(0x0080));
    CHECK(player.forceData[0] == 0);
    CHECK(force_node.flags == UINT32_C(0x00000001));
    CHECK(force_node.v3Velocity2.vx == 0);
    CHECK(force_node.v3Velocity2.vy == 0);
    CHECK(force_node.v3Velocity2.vz == 0);
    CHECK(force_node.v3Translation2.vx == 0);
    CHECK(force_node.v3Translation2.vy == 0);
    CHECK(force_node.v3Translation2.vz == 0);

    player.currentMotion = 0x60;
    player.forceData[0] = 456;
    force_node.flags = UINT32_C(0x04000000);
    CHECK(jedi_Main(&cpad, &player) == 0);
    CHECK(player.forceData[0] == 456);
    CHECK(force_node.flags == UINT32_C(0x04000000));

    player.playerID = 8;
    player.currentMotion = 0x8d;
    CHECK(jedi_Main(&cpad, &player) == 0);
    CHECK(player.forceData[0] == 456);
    player.currentMotion = 0;
    CHECK(jedi_Main(&cpad, &player) == 0);
    CHECK(player.forceData[0] == 0);
    CHECK(force_node.flags == 0);
    return 0;
}

static int test_player_pad_sampling(void)
{
    playerObject *player;

    player_gInitPlayers(0);
    player = player_gGetPlayerPtr(0);
    player->playerPad.mask0 = JPB_PAD_UP;
    player->playerPad.mask1 = 0;
    raw_pad_bits = 0;
    jpb_InputSetProvider(test_read_pad, NULL);
    ClearInput();
    maskPadBits(0);

    /* The original held-button mask must observe one released sample first. */
    jpb_PlayerSamplePad(player, 0, 1);
    CHECK(player->playerPad.cpad[0] == 0);
    CHECK(player->playerPad.cpad[1] == 0);

    raw_pad_bits = JPB_PAD_UP | JPB_PAD_BUTTON_1;
    jpb_PlayerSamplePad(player, 0, 1);
    CHECK(player->playerPad.cpad[0] ==
          (JPB_PAD_UP | JPB_PAD_BUTTON_1));
    CHECK(player->playerPad.cpad[1] ==
          (JPB_PAD_UP | JPB_PAD_BUTTON_1));
    jpb_PlayerSamplePad(player, 0, 1);
    CHECK(player->playerPad.cpad[0] == JPB_PAD_UP);
    CHECK(player->playerPad.cpad[1] == 0);

    raw_pad_bits = 0;
    jpb_PlayerSamplePad(player, 0, 0);
    CHECK(player->playerPad.cpad[0] == 0);
    CHECK(player->playerPad.cpad[1] == 0);

    /* A released sample re-arms the exact rising-edge channel. */
    raw_pad_bits = JPB_PAD_UP | JPB_PAD_BUTTON_1;
    jpb_PlayerSamplePad(player, 0, 1);
    CHECK(player->playerPad.cpad[0] ==
          (JPB_PAD_UP | JPB_PAD_BUTTON_1));
    CHECK(player->playerPad.cpad[1] ==
          (JPB_PAD_UP | JPB_PAD_BUTTON_1));
    jpb_InputSetProvider(NULL, NULL);
    return 0;
}

static int test_player_process_scheduler(void)
{
    WorldData world;
    sceneObject scenes[2];
    physicsObject physics[2];
    playerObject *player0;
    playerObject *player1;
    int32_t pause_pad[2] = {
        INT32_C(0x800), INT32_C(0x900)};
    int stack_level;

    memset(&world, 0, sizeof(world));
    memset(scenes, 0, sizeof(scenes));
    memset(physics, 0, sizeof(physics));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    player_gInitPlayers(0);
    player0 = &gaPlayerData[0];
    player1 = &gaPlayerData[1];
    player0->playerRoot.objectID = 0;
    player0->playerRoot.pParent =
        &scenes[0].sceneRoot;
    player0->pFlags = UINT32_C(0x80);
    player0->playerPad.mask0 = JPB_PAD_ZOOM_IN;
    player0->playerPad.mask1 = JPB_PAD_UP;
    scenes[0].pScene = &scenes[0].sceneRoot;
    scenes[0].pPhysics = &physics[0].physicsRoot;
    world.player0 = player0;
    world.player1 = player1;
    gpWorld = &world;
    GameStruct.CurrentLevel = 1;
    GameStruct.NumPlayers = 1;

    raw_pad_bits = 0;
    jpb_InputSetProvider(test_read_pad, NULL);
    ClearInput();
    maskPadBits(0);
    player_gProcessPlayers();
    raw_pad_bits = JPB_PAD_ZOOM_IN | JPB_PAD_UP;
    stack_level = jpb_WRenderMatrixStackLevel();
    player_gProcessPlayers();
    CHECK(jpb_WRenderMatrixStackLevel() == stack_level);
    CHECK(player0->numAttackers == 2);
    CHECK(player0->playerPad.cpad[0] ==
          (JPB_PAD_ZOOM_IN | JPB_PAD_UP));
    CHECK(player0->playerPad.cpad[1] ==
          (JPB_PAD_ZOOM_IN | JPB_PAD_UP));
    CHECK(game_GET_GLOBALBIT(0x10U) == 0);

    player1->playerRoot.objectID = 1;
    player1->playerRoot.pParent =
        &scenes[1].sceneRoot;
    player1->pFlags = UINT32_C(0x80);
    scenes[1].pScene = &scenes[1].sceneRoot;
    scenes[1].pPhysics = &physics[1].physicsRoot;
    GameStruct.NumPlayers = 2;
    raw_pad_bits = 0;
    player_gProcessPlayers();
    CHECK(player1->numAttackers == 2);
    CHECK(game_GET_GLOBALBIT(0x10U) != 0);

    player1->playerRoot.flags = UINT32_C(0x20);
    scenes[1].sceneRoot.flags = UINT32_C(0x20);
    player_gProcessPlayers();
    CHECK(game_GET_GLOBALBIT(0x10U) == 0);

    GameStruct.GameState = 0;
    player0->playernum = 0;
    CHECK(brainutil_PauseControl(
              pause_pad, player0) == 0);
    CHECK((GameStruct.GameState &
           UINT32_C(0x00201000)) ==
          UINT32_C(0x00201000));

    jpb_InputSetProvider(NULL, NULL);
    gpWorld = NULL;
    return 0;
}

static int test_after_life_cleanup(void)
{
    int32_t lock_ring[16];
    SCB scb;
    Sprite shadow;
    playerObject *player;

    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(lock_ring, 0xa5, sizeof(lock_ring));
    memset(&scb, 0, sizeof(scb));
    memset(&shadow, 0, sizeof(shadow));
    player_gInitPlayers(0);
    player = &gaPlayerData[3];
    player->playerRoot.objectID = 3;
    player->lockRing = lock_ring;
    player->shadow = (int32_t *)(void *)&shadow;
    shadow.sp_SCB = &scb;
    shadow.sp_Flags = 0x20;
    scb.scb_flags = 0x40;
    GameStruct.aCharacterData[3].Items = 4;
    GameStruct.aCharacterData[3].PowerType = 2;
    GameStruct.aCharacterData[3].PowerLevel = 100;
    gGlobalTimer = 500;

    player_AfterLife(player);
    CHECK(GameStruct.aCharacterData[3].Items == 0);
    CHECK(GameStruct.aCharacterData[3].PowerType == 0);
    CHECK(GameStruct.aCharacterData[3].PowerLevel == 500);
    CHECK(lock_ring[15] == 0);
    CHECK(player->lockRing == NULL);
    CHECK(shadow.sp_Flags == 0x21);
    CHECK(scb.scb_flags == 0x41);
    CHECK(player->shadow == NULL);
    return 0;
}

static int test_combo_and_player_reset(void)
{
    sceneObject scenes[2];
    modelObject models[2];
    playerObject combo_player;
    int32_t old_totalframes = totalframes;
    int index;

    memset(scenes, 0, sizeof(scenes));
    memset(models, 0, sizeof(models));
    memset(&combo_player, 0, sizeof(combo_player));
    combo_player.pFlags = UINT32_C(0x00220000);
    combo_player.heldMask = UINT32_C(0xffffffff);
    combo_player.releaseMask = UINT32_C(0xffffffff);
    combo_player.bheld[1] = 1;
    combo_player.PreMotion[0] = 'x';
    combo_player.playerPad.bufferedbits = UINT32_C(0xffffffff);
    combo_player.chainSlack = 7;
    combo_player.chainSlackEnd = 8;
    combo_ResetComboEngine(456, &combo_player);
    CHECK(combo_player.vtime == 456);
    CHECK(combo_player.ctime == 456);
    CHECK(combo_player.dtime == 456);
    CHECK(combo_player.PreMotion[0] == '\0');
    CHECK(combo_player.playerPad.bufferedbits == 0);
    CHECK(combo_player.chainSlack == 0);
    CHECK(combo_player.chainSlackEnd == 0);
    CHECK(combo_player.pFlags == UINT32_C(0x01200000));
    CHECK(combo_player.heldMask == UINT32_C(0xffff0002));
    CHECK(combo_player.releaseMask == UINT32_C(0xffff0000));

    player_gInitPlayers(0);
    totalframes = 1234;
    CHECK(brainutl_ElapsedTime(0, 0) == 1234);
    CHECK(brainutl_ElapsedTime(1200, 33) == 1234);
    CHECK(brainutl_ElapsedTime(1200, 34) == 0);

    for (index = 0; index < 2; ++index) {
        playerObject *player = &gaPlayerData[index];

        scenes[index].pModel = &models[index].modelRoot;
        player->playerRoot.pParent =
            &scenes[index].sceneRoot;
        player->fLife = 1;
        player->fStun = 2;
        player->fForce = 3;
        player->playerPad.oldbits0 = 4;
        player->playerPad.bufferedbits = 5;
        player->ACTION_LOCK = 6;
        player->runCounter = 7;
        player->groundDelay = 8;
        player->pFlags = UINT32_C(0xffffffff);
        player->hitMask = 9;
        player->hitDelay = 10;
        player->hitNumber = 11;
        player->numAttackers = 12;
        player->projectile =
            (ProjType *)(uintptr_t)UINT32_C(0x1234);
        models[index].flags = UINT32_C(0x11);
        player_ResetJedi(index);
        CHECK(player->fLife == 0);
        CHECK(player->fStun == 0);
        CHECK(player->fForce == 0);
        CHECK(player->playerPad.oldbits0 == 0);
        CHECK(player->playerPad.bufferedbits == 0);
        CHECK(player->ACTION_LOCK == 0);
        CHECK(player->runCounter == 0);
        CHECK(player->groundDelay == 0);
        CHECK(player->pFlags == 0);
        CHECK(player->hitMask == 0);
        CHECK(player->hitDelay == 0);
        CHECK(player->hitNumber == 0);
        CHECK(player->numAttackers == 0);
        CHECK(player->projectile == NULL);
        CHECK(player->vtime == totalframes);
        CHECK(player->ctime == totalframes);
        CHECK(player->dtime == totalframes);
        CHECK(models[index].flags == UINT32_C(1));
    }
    totalframes = old_totalframes;
    return 0;
}

static void connect_refresh_player(
    int index,
    sceneObject *scene,
    modelObject *model)
{
    playerObject *player = &gaPlayerData[index];
    physicsObject *physics = &maPhysicsData[index];

    memset(model, 0, sizeof(*model));
    player->playerRoot.objectID = index;
    player->playerRoot.pParent = &scene->sceneRoot;
    physics->physicsRoot.objectID = index;
    physics->physicsRoot.pParent = &scene->sceneRoot;
    scene->sceneRoot.objectID = index;
    scene->pScene = &scene->sceneRoot;
    scene->pModel = &model->modelRoot;
    scene->pPhysics = &physics->physicsRoot;
    scene->pPlayer = &player->playerRoot;
    scene->pAnim = NULL;
}

static int test_player_refresh_start_and_checkpoint(void)
{
    int32_t map_storage[8] = {0};
    int32_t *old_leveldata = leveldata;
    sceneObject *scene0;
    sceneObject *scene1;
    playerObject *player0;
    playerObject *player1;
    physicsObject *physics0;
    physicsObject *physics1;
    modelObject model0;
    modelObject model1;
    Sprite *shadow;
    _Material marker_material;

    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&marker_material, 0, sizeof(marker_material));
    player_gInitPlayers(0);
    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    meminit();
    sprite_gInitSprites();
    leveldata = map_storage + 4;
    leveldata[-2] = 0;
    numsolids = 0;
    afterLife = NULL;
    gCheckPoint = 0;
    corusPoints[0] = 777;
    corusPoints[1] = 888;
    effects1Handle[32] = &marker_material;

    scene0 = &maSceneData[0];
    scene1 = &maSceneData[1];
    player0 = &gaPlayerData[0];
    player1 = &gaPlayerData[1];
    physics0 = &maPhysicsData[0];
    physics1 = &maPhysicsData[1];
    connect_refresh_player(0, scene0, &model0);
    connect_refresh_player(1, scene1, &model1);

    LevelSelect = 1;
    GameStruct.CurrentLevel = 1;
    GameStruct.ModelSelect[0] = 2;
    GameStruct.maxEnergyLevels[2] = 180;
    GameStruct.maxEnergyLineLength[2] = 30;
    GameStruct.maxForceLevels[2] = 160;
    GameStruct.maxForceLineLength[2] = 28;
    GameStruct.Counter = 99;
    GameStruct.aCharacterData[0].Score = 12;
    player0->playernum = 0;
    player0->playerID = 2;
    player0->pFlags = UINT32_C(0xffffffff);
    player0->forceFlags = UINT32_C(0xffffffff);
    player0->fLife = 1;
    player0->fStun = 2;
    player0->fForce = 3;
    player0->hitMask = UINT32_C(0xffffffff);
    player0->hitDelay = UINT32_C(0xffffffff);
    player0->ctime = 7;
    memset(player0->bheld, 0xa5, sizeof(player0->bheld));
    player0->chainSlack = 8;
    player0->playerPad.oldbits0 = 9;
    player0->playerPad.oldbits1 = 10;
    player0->pMotionCallBack =
        (JPBPlayerCallback)(uintptr_t)1;
    player0->pForceCallBack =
        (JPBPlayerCallback)(uintptr_t)1;
    player0->comboUserData = 11;
    player0->ACTION_LOCK = 12;
    player0->airVelocity = 13;
    player0->airAngle = 14;
    player0->subOffset = 15;
    physics0->constmov.vx = 1.0f;
    physics0->constmov.vy = 2.0f;
    physics0->constmov.vz = 3.0f;
    scene0->sceneRoot.flags = UINT32_C(0x20);
    model0.flags = UINT32_C(0x10);

    player_RefreshPlayer(player0);
    CHECK(physics0->pos.vx == 22528.0f);
    CHECK(physics0->pos.vy == 3328.0f);
    CHECK(physics0->pos.vz == -13056.0f);
    CHECK(physics0->lastpos.vx == physics0->pos.vx);
    CHECK(physics0->lastpos.vy == physics0->pos.vy);
    CHECK(physics0->lastpos.vz == physics0->pos.vz);
    CHECK(physics0->angle.vy == 0x800);
    CHECK(physics0->validairground == -32760.0f);
    CHECK(GameStruct.Counter == 0);
    CHECK(GameStruct.aCharacterData[0].Score == 0);
    CHECK(GameStruct.aCharacterData[0].Energy == 180);
    CHECK(GameStruct.aCharacterData[0].MaxEnergy == 180);
    CHECK(GameStruct.aCharacterData[0].Force == 160);
    CHECK(GameStruct.aCharacterData[0].MaxForce == 160);
    CHECK((scene0->sceneRoot.flags & UINT32_C(0x20)) == 0);
    CHECK((model0.flags & UINT32_C(0x10)) == 0);
    CHECK(player0->pFlags == UINT32_C(0x00802000));
    CHECK(player0->forceFlags == 0);
    CHECK(player0->fLife == 0);
    CHECK(player0->fStun == 0);
    CHECK(player0->fForce == 0);
    CHECK(player0->hitMask == 0);
    CHECK(player0->hitDelay == 0);
    CHECK(player0->ctime == 0);
    CHECK(player0->bheld[0] == 0);
    CHECK(player0->bheld[15] == 0);
    CHECK(player0->chainSlack == 0);
    CHECK(player0->playerPad.oldbits0 == 0);
    CHECK(player0->playerPad.oldbits1 == 0);
    CHECK(player0->pMotionCallBack == NULL);
    CHECK(player0->pForceCallBack == NULL);
    CHECK(player0->comboUserData == 0);
    CHECK(player0->ACTION_LOCK == 0);
    CHECK(player0->airVelocity == 0);
    CHECK(player0->airAngle == 0);
    CHECK(player0->subOffset == 0);
    CHECK(physics0->constmov.vx == 0.0f);
    CHECK(physics0->constmov.vy == 0.0f);
    CHECK(physics0->constmov.vz == 0.0f);
    CHECK(player0->shadow != NULL);
    shadow = (Sprite *)(void *)player0->shadow;
    CHECK(shadow->sp_SCB->scb_Texture == &marker_material);
    CHECK(shadow->sp_SCB->scb_flags == 0x00400004);
    CHECK(shadow->sp_User == (int32_t *)(void *)&physics0->vpos);
    CHECK(shadow->sp_Num == 0x30);
    CHECK(shadow->sp_Func == sprite_Center);
    CHECK(shadow->sp_Func((int32_t *)(void *)shadow) == 0);
    CHECK(shadow->sp_SCB->scb_vertex0.vx == 22504.0f);
    CHECK(shadow->sp_SCB->scb_vertex1.vx == 22552.0f);

    gCheckPoint = 1;
    reStartPos[1].vx = 100;
    reStartPos[1].vy = -200;
    reStartPos[1].vz = 300;
    reStartScore[1] = 456;
    reStartCounter = 789;
    GameStruct.NumPlayers = 1;
    GameStruct.CurrentLevel = 0;
    player1->playernum = 1;
    player1->playerID = 12;
    scene1->sceneRoot.flags = UINT32_C(0x20);
    model1.flags = UINT32_C(0x10);

    player_RefreshPlayer(player1);
    CHECK(physics1->pos.vx == 100.0f);
    CHECK(physics1->pos.vy == -200.0f);
    CHECK(physics1->pos.vz == 300.0f);
    CHECK(GameStruct.aCharacterData[1].Score == 456);
    CHECK(GameStruct.Counter == 789);
    CHECK(GameStruct.aCharacterData[1].Energy == 200);
    CHECK(GameStruct.aCharacterData[1].Force == 200);
    CHECK((scene1->sceneRoot.flags & UINT32_C(0x20)) != 0);
    CHECK((model1.flags & UINT32_C(0x10)) != 0);
    CHECK(player1->shadow == NULL);

    /* Exact player_gRefreshPlayers level-eight reset tail. */
    player0->playerRoot.objectID = -1;
    player1->playerRoot.objectID = -1;
    player0->shadow = NULL;
    player1->shadow = NULL;
    physics0->flags = UINT32_C(0xffffffff);
    physics1->flags = UINT32_C(0xffffffff);
    physics0->movemode = MOVE_FLY;
    physics1->movemode = MOVE_FLY;
    for (int index = 0;
         index < JPB_PHYSICS_CAPACITY;
         ++index) {
        maPhysicsData[index].anycollidetime =
            UINT32_C(0x12345678);
    }
    memset(&bestinfo, 0xa5, sizeof(bestinfo));
    totalframes = 99;
    streets_reached_stairs = 1;
    gSCENE_READY = 1;
    LevelSelect = 8;
    player_gRefreshPlayers();
    CHECK(totalframes == 0);
    CHECK(streets_reached_stairs == 0);
    CHECK(gSCENE_READY == 0);
    CHECK(bestinfo.type == 0);
    CHECK(bestinfo.flags == 0);
    CHECK(bestinfo.dist == 0.0f);
    CHECK(bestinfo.edge == 0);
    CHECK(bestinfo.washack == 0);
    CHECK(physics0->movemode == MOVE_NORMAL);
    CHECK(physics1->movemode == MOVE_NORMAL);
    for (int index = 0;
         index < JPB_PHYSICS_CAPACITY;
         ++index) {
        CHECK(maPhysicsData[index].anycollidetime == 0);
    }

    sprite_gFreeSprite(shadow);
    effects1Handle[32] = NULL;
    leveldata = old_leveldata;
    LevelSelect = 0;
    gCheckPoint = 0;
    return 0;
}

static int test_player_refresh_all_authored_starts(void)
{
    static const int expected_facing[JPB_LEVEL_COUNT][2] = {
        {0x000, 0x000}, {0x800, 0x800},
        {0x000, 0x000}, {0xc00, 0xc00},
        {0x000, 0x000}, {0x800, 0x800},
        {0xc00, 0xc00}, {0xc00, 0xc00},
        {0x000, 0x000}, {0x400, 0x400},
        {0x800, 0x800}, {0x800, 0x800},
        {0x000, 0x000}, {0x800, 0x800},
        {0x800, 0x800}, {0x000, 0x000},
        {0x400, 0x000}, {0x400, 0x000},
        {0x400, 0x000}, {0x400, 0x000},
        {0x800, 0x000}, {0x000, 0x000},
        {0x000, 0x000}, {0x000, 0x000},
        {0x000, 0x000}, {0x800, 0x000},
    };
    int32_t map_storage[8] = {0};
    int32_t *old_leveldata = leveldata;
    sceneObject *scenes[2];
    playerObject *players[2];
    physicsObject *physics[2];
    modelObject models[2];
    _Material marker_material;

    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&marker_material, 0, sizeof(marker_material));
    player_gInitPlayers(0);
    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    meminit();
    sprite_gInitSprites();
    leveldata = map_storage + 4;
    leveldata[-2] = 0;
    numsolids = 0;
    afterLife = NULL;
    gCheckPoint = 0;
    gSCENE_READY = 0;
    effects1Handle[32] = &marker_material;
    GameStruct.NumPlayers = 2;
    raw_pad_bits = 0;
    jpb_InputSetProvider(test_read_pad, NULL);
    ClearInput();
    maskPadBits(0);
    maskPadBits(1);

    for (int player_index = 0;
         player_index < 2;
         ++player_index) {
        scenes[player_index] = &maSceneData[player_index];
        players[player_index] = &gaPlayerData[player_index];
        physics[player_index] = &maPhysicsData[player_index];
        connect_refresh_player(
            player_index,
            scenes[player_index],
            &models[player_index]);
        players[player_index]->playernum =
            (int16_t)player_index;
        players[player_index]->playerID = 12;
        /* player_gCreateObject's exact edge/held channel configuration. */
        players[player_index]->playerPad.mask0 = 0;
        players[player_index]->playerPad.mask1 = UINT32_MAX;
    }

    /* maskPadBits suppresses the state present when it is called.  The
     * loading path observes a neutral sample before actors become playable,
     * which re-arms the shared physical-pad masks before RefreshPlayer resets
     * each actor's private edge history. */
    jpb_PlayerSamplePad(players[0], 0, 1);
    jpb_PlayerSamplePad(players[1], 1, 1);
    CHECK(players[0]->playerPad.cpad[0] == 0);
    CHECK(players[0]->playerPad.cpad[1] == 0);
    CHECK(players[1]->playerPad.cpad[0] == 0);
    CHECK(players[1]->playerPad.cpad[1] == 0);

    for (int level = 0; level < JPB_LEVEL_COUNT; ++level) {
        LevelSelect = (int16_t)level;
        GameStruct.CurrentLevel = level;

        for (int player_index = 0;
             player_index < 2;
             ++player_index) {
            playerObject *player = players[player_index];
            physicsObject *body = physics[player_index];
            int expected_x =
                0x8000 -
                (int)startPos[level][player_index].vx * 0x100;
            int expected_y =
                (int)startPos[level][player_index].vz * 0x100;
            int expected_z =
                ((int)startPos[level][player_index].vy - 0x7f) *
                0x100;

            if (player_index == 0 && level == 20) {
                expected_x += 0x80;
                expected_z += 0x80;
            }

            /* Start each level as a newly initialized player would. */
            body->angle.vy = 0;
            body->constmov.vx = 1.0f;
            body->constmov.vy = 2.0f;
            body->constmov.vz = 3.0f;
            player->playerPad.oldbits0 = UINT16_C(0xffff);
            player->playerPad.oldbits1 = UINT16_C(0xffff);
            player->pMotionCallBack =
                (JPBPlayerCallback)(uintptr_t)1;
            player->pForceCallBack =
                (JPBPlayerCallback)(uintptr_t)1;
            player->ACTION_LOCK = UINT32_C(0xffffffff);

            player_RefreshPlayer(player);

            CHECK(body->pos.vx == (float)expected_x);
            CHECK(body->pos.vy == (float)expected_y);
            CHECK(body->pos.vz == (float)expected_z);
            CHECK(body->lastpos.vx == body->pos.vx);
            CHECK(body->lastpos.vy == body->pos.vy);
            CHECK(body->lastpos.vz == body->pos.vz);
            CHECK(body->angle.vy ==
                  expected_facing[level][player_index]);
            CHECK(player->playerPad.oldbits0 == 0);
            CHECK(player->playerPad.oldbits1 == 0);
            CHECK(player->pMotionCallBack == NULL);
            CHECK(player->pForceCallBack == NULL);
            CHECK(player->ACTION_LOCK == 0);
            CHECK(body->constmov.vx == 0.0f);
            CHECK(body->constmov.vy == 0.0f);
            CHECK(body->constmov.vz == 0.0f);

            /*
             * Exercise the first physical sample, not just the reset stores.
             * A newly refreshed player sees a held direction/action as a
             * rising edge on cpad[0] and as held input on cpad[1].  The next
             * sample retains only the held channel, and release re-arms both.
             * P2 uses its independent pad history and remains neutral here.
             */
            if (player_index == 0) {
                raw_pad_bits = JPB_PAD_UP | JPB_PAD_BUTTON_1;
                jpb_PlayerSamplePad(player, player_index, 1);
                if (player->playerPad.cpad[0] !=
                        (JPB_PAD_UP | JPB_PAD_BUTTON_1) ||
                    player->playerPad.cpad[1] !=
                        (JPB_PAD_UP | JPB_PAD_BUTTON_1)) {
                    fprintf(
                        stderr,
                        "authored start input mismatch level=%d "
                        "pressed=%08x held=%08x old=%08x/%08x\n",
                        level,
                        (unsigned)player->playerPad.cpad[0],
                        (unsigned)player->playerPad.cpad[1],
                        (unsigned)player->playerPad.oldbits0,
                        (unsigned)player->playerPad.oldbits1);
                    return 1;
                }

                jpb_PlayerSamplePad(player, player_index, 1);
                CHECK(player->playerPad.cpad[0] == 0);
                CHECK(player->playerPad.cpad[1] ==
                      (JPB_PAD_UP | JPB_PAD_BUTTON_1));

                raw_pad_bits = 0;
                jpb_PlayerSamplePad(player, player_index, 1);
                CHECK(player->playerPad.cpad[0] == 0);
                CHECK(player->playerPad.cpad[1] == 0);
                CHECK(player->playerPad.oldbits0 == 0);
                CHECK(player->playerPad.oldbits1 == 0);
            } else {
                jpb_PlayerSamplePad(player, player_index, 1);
                CHECK(player->playerPad.cpad[0] == 0);
                CHECK(player->playerPad.cpad[1] == 0);
            }
        }
    }

    for (int player_index = 0;
         player_index < 2;
         ++player_index) {
        if (players[player_index]->shadow != NULL) {
            sprite_gFreeSprite(
                (Sprite *)(void *)players[player_index]->shadow);
            players[player_index]->shadow = NULL;
        }
    }
    effects1Handle[32] = NULL;
    jpb_InputSetProvider(NULL, NULL);
    raw_pad_bits = 0;
    leveldata = old_leveldata;
    LevelSelect = 0;
    GameStruct.CurrentLevel = 0;
    GameStruct.NumPlayers = 0;
    return 0;
}

static int test_player_free(void)
{
    playerObject *player;
    sceneObject scene;
    objectRoot model;
    objectRoot physics;
    objectRoot animation;
    wsl_ENEMY enemy;
    Sprite shadow;
    SCB scb;

    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&physics, 0, sizeof(physics));
    memset(&animation, 0, sizeof(animation));
    memset(&enemy, 0, sizeof(enemy));
    memset(&shadow, 0, sizeof(shadow));
    memset(&scb, 0, sizeof(scb));
    player_gInitPlayers(0);
    player = &gaPlayerData[2];
    player->playerRoot.objectID = 2;
    player->playerRoot.pParent = &scene.sceneRoot;
    player->pEnemy = &enemy;
    player->pFlags = UINT32_C(0xffffffff);
    player->shadow = (int32_t *)(void *)&shadow;
    shadow.sp_SCB = &scb;
    shadow.sp_Flags = 4;
    scb.scb_flags = 4;
    scene.sceneRoot.flags = UINT32_C(0xffffffff);
    scene.sceneRoot.objectID = 2;
    model.objectID = 2;
    physics.objectID = 2;
    animation.objectID = 2;
    scene.pModel = &model;
    scene.pPhysics = &physics;
    scene.pAnim = &animation;
    scene.pPlayer = &player->playerRoot;

    player_FreePlayer(player);
    CHECK(gaPlayerData[0].target == &gaPlayerData[1]);
    CHECK(gaPlayerData[1].target == &gaPlayerData[0]);
    CHECK(player->pFlags == UINT32_C(0x00800000));
    CHECK(enemy.exit_flag == 1);
    CHECK((shadow.sp_Flags & 1) != 0);
    CHECK((scb.scb_flags & 1) != 0);
    CHECK(scene.sceneRoot.flags == 0);
    CHECK(scene.sceneRoot.objectID == -1);
    CHECK(model.objectID == -1);
    CHECK(physics.objectID == -1);
    CHECK(animation.objectID == -1);
    CHECK(player->playerRoot.objectID == -1);
    return 0;
}

static int test_player_collision_owner(void)
{
    CollisionData attacker_collision = {64, 1, -1};
    CollisionData target_collision = {64, 2, -1};
    Mnode attacker_node;
    Mnode target_node;
    Motion *authored_motion = (Motion *)(uintptr_t)0x1234;
    playerObject *attacker;
    playerObject *target;
    sceneObject *attacker_scene;
    sceneObject *target_scene;

    memset(&attacker_node, 0, sizeof(attacker_node));
    memset(&target_node, 0, sizeof(target_node));
    memset(&GameStruct, 0, sizeof(GameStruct));
    jpb_SceneInitPool(0);
    physics_gInitObjects(0);
    player_gInitPlayers(0);
    physics_InitPhysics();
    coll_ResetCollisionSystem();

    attacker = &gaPlayerData[0];
    target = &gaPlayerData[2];
    attacker_scene = &maSceneData[0];
    target_scene = &maSceneData[2];
    attacker->playerRoot.objectID = 0;
    attacker->playerRoot.pParent = &attacker_scene->sceneRoot;
    attacker->playernum = 0;
    attacker->fScale = 4096;
    attacker->paNodesSizes = &attacker_collision;
    attacker->numCollisionNodes = 1;
    attacker->pMotion = &authored_motion;
    target->playerRoot.objectID = 2;
    target->playerRoot.pParent = &target_scene->sceneRoot;
    target->playernum = 2;
    target->fScale = 4096;
    target->paNodesSizes = &target_collision;
    target->numCollisionNodes = 1;

    attacker_scene->sceneRoot.objectID = 0;
    attacker_scene->sceneRoot.flags = UINT32_C(0x10);
    attacker_scene->pScene = &attacker_scene->sceneRoot;
    attacker_scene->pPhysics =
        &maPhysicsData[0].physicsRoot;
    attacker_scene->pPlayer = &attacker->playerRoot;
    maPhysicsData[0].physicsRoot.objectID = 0;
    maPhysicsData[0].physicsRoot.pParent =
        &attacker_scene->sceneRoot;
    target_scene->sceneRoot.objectID = 2;
    target_scene->pScene = &target_scene->sceneRoot;
    target_scene->pPhysics =
        &maPhysicsData[2].physicsRoot;
    target_scene->pPlayer = &target->playerRoot;
    maPhysicsData[2].physicsRoot.objectID = 2;
    maPhysicsData[2].physicsRoot.pParent =
        &target_scene->sceneRoot;
    maRange[0][2] = 100.0f;

    attacker_node.id = (modelNodeId)(NODE_DYNAMIC | 1);
    attacker_node.flags = JPB_COLLISION_FLAG_HOT;
    attacker_node.v3RotCenter.vx = 100;
    attacker_node.v3Velocity.vx = 10;
    target_node.id = (modelNodeId)(NODE_DYNAMIC | 2);
    target_node.v3RotCenter.vx = 110;
    coll_gRegisterNode(0, &attacker_node);
    coll_gRegisterNode(2, &target_node);
    totalframes = 100;

    CHECK(player_DoCollisions() == 1);
    CHECK((attacker_scene->sceneRoot.flags & UINT32_C(0x10)) == 0);
    CHECK(attacker->target == target);
    CHECK(target->target == attacker);
    CHECK(target->whohitme == attacker);
    CHECK(target->hitNumber == 1);
    CHECK((attacker->pFlags & UINT32_C(0x10000)) != 0);

    CHECK(player_DoCollisions() == 1);
    CHECK(target->hitNumber == 1);
    attacker->forceFlags = UINT32_C(0x40);
    attacker->target = NULL;
    target->target = NULL;
    target->whohitme = NULL;
    target->hitNumber = 0;
    CHECK(player_DoCollisions() == 1);
    CHECK(attacker->target == target);
    CHECK(target->target == attacker);
    CHECK(target->whohitme == attacker);
    CHECK(target->hitNumber == 1);
    return 0;
}

static int test_jedi_ranged_weapon_owner(void)
{
    typedef struct WeaponRoute {
        int16_t playerId;
        int16_t conflictingMotion;
        int primaryMuzzle;
        int primaryAim;
        int secondaryMuzzle;
        int secondaryAim;
    } WeaponRoute;
    static const WeaponRoute routes[] = {
        {0x11, 0x1a, 0x0b, 0x0c, 0, 0},
        {0x12, 0x1a, 0x0c, 0x13, 0, 0},
        {0x1a, 0x00, 0x0c, 0x1a, 0x0b, 0x1b},
        {0x1c, 0x00, 0x0c, 0x1a, 0x0b, 0x1b},
        {0x30, 0x1a, 0x0c, 0x12, 0, 0},
        {amidala_model, 0x1a, 0x0c, 0x11, 0, 0}
    };
    ProjectileLaunchTrace trace;
    playerObject player;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    Motion motion;
    Motion *motion_pointer = &motion;
    Mnode nodes[28];
    _Material material;
    ProjType *types = (ProjType *)(void *)maProjTypes;
    size_t route_index;
    int node_index;

    memset(&trace, 0, sizeof(trace));
    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&physics, 0, sizeof(physics));
    memset(&motion, 0, sizeof(motion));
    memset(nodes, 0, sizeof(nodes));
    memset(&material, 0, sizeof(material));
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(effects1Handle, 0, sizeof(effects1Handle));
    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    bullet_InitProjectilePool();
    player_gInitPlayers(0);
    for (node_index = 0;
         node_index < JPB_PLAYER_CAPACITY;
         ++node_index) {
        gaPlayerData[node_index].playerRoot.objectID = -1;
    }
    for (node_index = 0;
         node_index < (int)(sizeof(nodes) / sizeof(nodes[0]));
         ++node_index) {
        nodes[node_index].id =
            (modelNodeId)(NODE_DYNAMIC | node_index);
        nodes[node_index].v3RotCenter.vx = node_index * 100 + 3;
        nodes[node_index].v3RotCenter.vy = node_index * 100 + 5;
        nodes[node_index].v3RotCenter.vz = node_index * 100 + 7;
        coll_gRegisterNode(0, &nodes[node_index]);
    }
    material.iw = 8;
    material.ih = 8;
    effects1Handle[0] = &material;
    for (node_index = 0; node_index < 32; ++node_index) {
        types[node_index].range = 20;
        types[node_index].length = 8;
        types[node_index].width = 2;
        types[node_index].muzzelEffect = -1;
        types[node_index].bulletSprite = 0;
        types[node_index].speed = 8;
    }
    player.playerRoot.objectID = 0;
    player.playerRoot.pParent = &scene.sceneRoot;
    player.playernum = 0;
    player.pMotion = &motion_pointer;
    scene.pScene = &scene.sceneRoot;
    scene.pModel = &model.modelRoot;
    scene.pPhysics = &physics.physicsRoot;
    model.eventMask = 1;
    motion.fx2 = 2;
    GameStruct.GameState = 0;
    GameStruct.versusModeFlag = 0;
    OptionStruct.FunFactor = 0;
    gGlobalTimer = 100;
    (void)game_gSetPowerLevel(0, 0);
    (void)game_gSetPowerType(0, 0);
    jpb_BulletSetLaunchObserver(trace_projectile_launch, &trace);

    for (route_index = 0;
         route_index < sizeof(routes) / sizeof(routes[0]);
         ++route_index) {
        const WeaponRoute *route = &routes[route_index];
        int first = trace.count;
        int expected_count = route->secondaryMuzzle != 0 ? 2 : 1;

        player.playerID = route->playerId;
        player.currentMotion = route->conflictingMotion;
        player.target = NULL;
        CHECK(jedi_FireWeapon(NULL, &player) == 0);
        CHECK(trace.count == first + expected_count);
        CHECK(trace.type[first] == 2);
        CHECK((trace.flags[first] & UINT32_C(0x10)) != 0);
        CHECK(trace.owner[first] == &player);
        CHECK(trace.start[first].vx ==
              nodes[route->primaryMuzzle].v3RotCenter.vx);
        CHECK(trace.target[first].vx ==
              nodes[route->primaryAim].v3RotCenter.vx);
        if (expected_count == 2) {
            CHECK(trace.type[first + 1] == 2);
            CHECK((trace.flags[first + 1] & UINT32_C(0x10)) != 0);
            CHECK(trace.start[first + 1].vx ==
                  nodes[route->secondaryMuzzle].v3RotCenter.vx);
            CHECK(trace.target[first + 1].vx ==
                  nodes[route->secondaryAim].v3RotCenter.vx);
        }
    }

    player.playerID = amidala_model;
    player.currentMotion = 0x1a;
    player.target = NULL;
    (void)game_gSetPowerLevel(0, 200);
    (void)game_gSetPowerType(0, 10);
    node_index = trace.count;
    CHECK(jedi_FireWeapon(NULL, &player) == 0);
    CHECK(trace.count == node_index + 1);
    CHECK(trace.type[node_index] == 4);
    (void)game_gSetPowerType(0, 0);
    node_index = trace.count;
    CHECK(jedi_FireWeapon(NULL, &player) == 0);
    CHECK(trace.count == node_index + 1);
    CHECK(trace.type[node_index] == 0x11);

    (void)game_gSetPowerLevel(0, 0);
    model.eventMask = 0;
    model.flags = 0;
    node_index = trace.count;
    CHECK(jedi_FireWeapon(NULL, &player) == 0);
    CHECK(trace.count == node_index);
    model.flags = UINT32_C(4);
    CHECK(jedi_FireWeapon(NULL, &player) == 1);
    CHECK(trace.count == node_index + 1);
    CHECK(trace.type[node_index] == 2);

    jpb_BulletSetLaunchObserver(NULL, NULL);
    effects1Handle[0] = NULL;
    return 0;
}

static int test_player_sabre_owner(void)
{
    static const struct {
        int16_t playerId;
        unsigned baseNode;
        unsigned tipNode;
        unsigned secondBaseNode;
        uint32_t outerColor;
        int expectedDraws;
    } roster[] = {
        {0, 0x11, 0x13, 0,    UINT32_C(0x7f45a6ff), 2},
        {1, 0x14, 0x16, 0,    UINT32_C(0x7f01c03c), 2},
        {2, 0x14, 0x16, 0,    UINT32_C(0x7fd870ff), 2},
        {3, 0x13, 0x15, 0,    UINT32_C(0x7f45a6ff), 2},
        {4, 0x11, 0x13, 0,    UINT32_C(0x7f45a6ff), 2},
        {5, 0x12, 0x13, 0x17, UINT32_C(0x7fff3434), 4},
        {6, 0,    0,    0,    UINT32_C(0),          0},
        {7, 0,    0,    0,    UINT32_C(0),          0},
        {8, 0x11, 0x13, 0,    UINT32_C(0x7f45a6ff), 2},
    };
    SabreGlowTrace trace;
    SabreCylinderTrace cylinder_trace;
    sceneObject scene;
    modelObject model;
    Motion motion;
    Motion *current_motion = &motion;
    Mnode base;
    Mnode tip;
    playerObject *player;

    memset(&trace, 0, sizeof(trace));
    memset(&cylinder_trace, 0, sizeof(cylinder_trace));
    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&motion, 0, sizeof(motion));
    memset(&base, 0, sizeof(base));
    memset(&tip, 0, sizeof(tip));
    memset(&GameStruct, 0, sizeof(GameStruct));
    player_gInitPlayers(0);
    coll_ResetCollisionSystem();
    player = &gaPlayerData[0];
    player->playerRoot.objectID = 0;
    player->playerRoot.pParent = &scene.sceneRoot;
    player->playernum = 0;
    player->playerID = 0;
    player->pMotion = &current_motion;
    scene.pScene = &scene.sceneRoot;
    scene.pModel = &model.modelRoot;
    base.id = (modelNodeId)(NODE_DYNAMIC | 0x11);
    base.v3RotCenter.vx = 100;
    base.v3RotCenter.vy = 200;
    base.v3RotCenter.vz = 300;
    tip.id = (modelNodeId)(NODE_DYNAMIC | 0x13);
    tip.v3RotCenter.vx = 212;
    tip.v3RotCenter.vy = 200;
    tip.v3RotCenter.vz = 300;
    coll_gRegisterNode(0, &base);
    coll_gRegisterNode(0, &tip);
    jpb_FxSetScreenGlowHook(trace_sabre_glow, &trace);

    player_HandleSabre();
    CHECK(trace.count == 2);
    CHECK(trace.start[0].vx == 212);
    CHECK(trace.start[0].vy == 200);
    CHECK(trace.start[0].vz == 300);
    CHECK(trace.end[0].vx == 100);
    CHECK(trace.end[0].vy == 200);
    CHECK(trace.end[0].vz == 300);
    CHECK(trace.width[0] >= 0x0e && trace.width[0] <= 0x13);
    CHECK(trace.color[0] ==
          (jedi_GetColour32(0) | UINT32_C(0x7f000000)));
    CHECK(trace.width[1] == 2);
    CHECK(trace.color[1] == UINT32_C(0xffffffff));
    CHECK(tip.time == 0xd0);

    /*
     * The matched owner selects both attachment nodes and colour from the
     * character ID. Exercise the complete base roster so a shared default
     * blade cannot stand in for character-owned behavior. Maul's second
     * blade begins 0x20 units along its direction and therefore spans 0x50,
     * rather than incorrectly extending the full 0x70 from the hilt.
     */
    for (size_t roster_index = 0;
         roster_index < sizeof(roster) / sizeof(roster[0]);
         ++roster_index) {
        Mnode roster_base;
        Mnode roster_tip;
        Mnode roster_second_base;
        const unsigned second_tip = 0x13;

        memset(&trace, 0, sizeof(trace));
        memset(&roster_base, 0, sizeof(roster_base));
        memset(&roster_tip, 0, sizeof(roster_tip));
        memset(&roster_second_base, 0, sizeof(roster_second_base));
        coll_ResetCollisionSystem();
        player->playerID = roster[roster_index].playerId;
        if (roster[roster_index].baseNode != 0) {
            roster_base.id = (modelNodeId)(
                NODE_DYNAMIC | roster[roster_index].baseNode);
            roster_base.v3RotCenter.vx = 100;
            roster_base.v3RotCenter.vy = 200;
            roster_base.v3RotCenter.vz = 300;
            roster_tip.id = (modelNodeId)(
                NODE_DYNAMIC | roster[roster_index].tipNode);
            roster_tip.v3RotCenter.vx = 212;
            roster_tip.v3RotCenter.vy = 200;
            roster_tip.v3RotCenter.vz = 300;
            coll_gRegisterNode(0, &roster_base);
            coll_gRegisterNode(0, &roster_tip);
        }
        if (roster[roster_index].secondBaseNode != 0) {
            roster_second_base.id = (modelNodeId)(
                NODE_DYNAMIC | roster[roster_index].secondBaseNode);
            roster_second_base.v3RotCenter.vx = 400;
            roster_second_base.v3RotCenter.vy = 200;
            roster_second_base.v3RotCenter.vz = 300;
            CHECK(roster[roster_index].tipNode == second_tip);
            coll_gRegisterNode(0, &roster_second_base);
        }

        CHECK(jedi_HandleSabre(NULL, player) == 1);
        CHECK(trace.count == roster[roster_index].expectedDraws);
        if (trace.count != 0) {
            CHECK(trace.start[0].vx == 212);
            CHECK(trace.end[0].vx == 100);
            CHECK(trace.width[0] >= 0x0e && trace.width[0] <= 0x13);
            CHECK(trace.color[0] == roster[roster_index].outerColor);
            CHECK(trace.width[1] == 2);
            CHECK(trace.color[1] == UINT32_C(0xffffffff));
        }
        if (trace.count == 4) {
            CHECK(trace.start[2].vx == 512);
            CHECK(trace.end[2].vx == 432);
            CHECK(trace.width[2] >= 0x0e && trace.width[2] <= 0x13);
            CHECK(trace.color[2] == roster[roster_index].outerColor);
            CHECK(trace.width[3] == 2);
            CHECK(trace.color[3] == UINT32_C(0xffffffff));
        }
    }

    /* Blade Extender keeps the ordinary core/halo ownership but lengthens
     * the blade to 0xc4 and uses the authored wider core. */
    memset(&trace, 0, sizeof(trace));
    coll_ResetCollisionSystem();
    player->playerID = obi_wan_model;
    coll_gRegisterNode(0, &base);
    coll_gRegisterNode(0, &tip);
    gGlobalTimer = 0;
    (void)game_gSetPowerType(0, 10);
    (void)game_gSetPowerLevel(0, 100);
    CHECK(jedi_HandleSabre(NULL, player) == 1);
    CHECK(trace.count == 2);
    CHECK(trace.width[0] >= 0x18 && trace.width[0] <= 0x1f);
    CHECK(trace.width[1] == 6);
    CHECK(trace.start[0].vx == 296);
    CHECK(trace.end[0].vx == 100);
    CHECK(tip.time == 0x16c);

    /* Blade Amplifier adds the exact base-to-tip glow and three short,
     * constant-radius cylinder bands. It must not reinterpret their height
     * phase as three increasingly wide full-blade glows. */
    memset(&trace, 0, sizeof(trace));
    memset(&cylinder_trace, 0, sizeof(cylinder_trace));
    jpb_SpriteSetCylinderHook(
        trace_sabre_cylinder, &cylinder_trace);
    (void)game_gSetPowerType(0, 9);
    CHECK(jedi_HandleSabre(NULL, player) == 1);
    CHECK(trace.count == 3);
    CHECK(trace.width[2] == 0x10);
    CHECK(trace.start[2].vx == 100);
    CHECK(trace.end[2].vx == 212);
    CHECK(trace.color[2] == UINT32_C(0xff45a6ff));
    CHECK(cylinder_trace.count == 3);
    for (int cylinder = 0; cylinder < cylinder_trace.count; ++cylinder) {
        CHECK(cylinder_trace.radius1[cylinder] == 26.0f);
        CHECK(cylinder_trace.radius2[cylinder] == 26.0f);
        CHECK(cylinder_trace.height2[cylinder] -
              cylinder_trace.height1[cylinder] == 8.0f);
        CHECK(cylinder_trace.color1[cylinder] ==
              UINT32_C(0x7f45a6ff));
        CHECK(cylinder_trace.color2[cylinder] ==
              UINT32_C(0x7f45a6ff));
    }
    CHECK(cylinder_trace.height1[0] == 4.0f);
    CHECK(cylinder_trace.height1[1] == 36.0f);
    CHECK(cylinder_trace.height1[2] == 68.0f);
    jpb_SpriteSetCylinderHook(NULL, NULL);
    (void)game_gSetPowerType(0, 0);
    (void)game_gSetPowerLevel(0, 0);

    trace.count = 0;
    model.flags = UINT32_C(4);
    player_HandleSabre();
    CHECK(trace.count == 0);
    model.flags = 0;
    player->pFlags = UINT32_C(0x80);
    player_HandleSabre();
    CHECK(trace.count == 0);
    player->pFlags = 0;
    player->playerID = 10;
    player_HandleSabre();
    CHECK(trace.count == 0);

    jpb_FxSetScreenGlowHook(NULL, NULL);
    return 0;
}

static int test_player_life_tile_owner(void)
{
    PlayerTileTrace trace;
    playerObject player;
    VECTOR center = {0, 0, 0, 0};
    size_t index;

    memset(&trace, 0, sizeof(trace));
    memset(&player, 0, sizeof(player));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(&CameraMatrix, 0, sizeof(CameraMatrix));
    CameraMatrix.m[0][0] = 1.0f;
    CameraMatrix.m[1][1] = 1.0f;
    CameraMatrix.m[2][2] = 1.0f;
    CameraMatrix.t[2] = 500;
    OptionStruct.overlayMode = 1;
    GameStruct.CurrentLevel = 1;
    player.playerRoot.objectID = 0;
    player.playernum = 0;
    player.playerID = 0;
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.aCharacterData[0].MaxEnergy = 100;
    GameStruct.aCharacterData[0].MaxEnergyPerc =
        UINT32_C(0x4000);
    GameStruct.aCharacterData[0].Force = 80;
    GameStruct.aCharacterData[0].MaxForce = 100;
    GameStruct.aCharacterData[0].MaxForcePerc =
        INT16_C(0x4000);
    jpb_PlayerSetTileHook(trace_player_tile, &trace);

    _AddLifeTile(&player, &center);
    CHECK(trace.count == 6);
    CHECK(trace.projectionDepth[0] == 500.0f);
    CHECK_FLOAT_CLOSE(trace.position[0].vx, -22.55f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[0].vy, -9.0f, 0.0001f);
    CHECK(trace.position[0].vz == 0.0001f);
    CHECK(trace.width[0] == 3.0f);
    CHECK(trace.height[0] == 3.0f);
    CHECK(trace.color[0] == UINT32_C(0xa5808080));
    CHECK_FLOAT_CLOSE(trace.position[1].vx, -18.75f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[1].vy, -9.0f, 0.0001f);
    CHECK(trace.width[1] == 37.5f);
    CHECK(trace.color[1] == UINT32_C(0xbd108010));
    CHECK_FLOAT_CLOSE(trace.position[2].vx, 19.55f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[2].vy, -9.0f, 0.0001f);
    CHECK(trace.color[2] == UINT32_C(0xa5808080));
    CHECK_FLOAT_CLOSE(trace.position[3].vx, -22.55f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[3].vy, -3.0f, 0.0001f);
    CHECK(trace.color[3] == UINT32_C(0xa5808080));
    CHECK_FLOAT_CLOSE(trace.position[4].vx, -18.75f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[4].vy, -3.0f, 0.0001f);
    CHECK(trace.width[4] == 30.0f);
    CHECK(trace.color[4] == UINT32_C(0xbd101080));
    CHECK_FLOAT_CLOSE(trace.position[5].vx, 19.55f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[5].vy, -3.0f, 0.0001f);
    CHECK(trace.color[5] == UINT32_C(0xa5808080));

    trace.count = 0;
    player.forceFlags = UINT32_C(0x10);
    _AddLifeTile(&player, &center);
    CHECK(trace.count == 3);
    CHECK_FLOAT_CLOSE(trace.position[0].vx, -22.55f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[0].vy, -3.0f, 0.0001f);
    CHECK(trace.color[0] == UINT32_C(0xa5808080));
    CHECK_FLOAT_CLOSE(trace.position[1].vx, -18.75f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[1].vy, -3.0f, 0.0001f);
    CHECK(trace.width[1] == 30.0f);
    CHECK(trace.color[1] == UINT32_C(0xbd101080));
    CHECK_FLOAT_CLOSE(trace.position[2].vx, 19.55f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[2].vy, -3.0f, 0.0001f);
    CHECK(trace.color[2] == UINT32_C(0xa5808080));
    for (index = 0; index < (size_t)trace.count; ++index) {
        CHECK(trace.projectionDepth[index] == 500.0f);
        CHECK(trace.position[index].vz == 0.0001f);
    }

    trace.count = 0;
    player.forceFlags = 0;
    player.playerRoot.objectID = 2;
    player.playernum = 2;
    player.playerID = 12;
    game_gSetMaxEnergy(2, 100);
    game_gSetEnergy(2, 50);
    _AddLifeTile(&player, &center);
    CHECK(trace.count == 3);
    CHECK_FLOAT_CLOSE(trace.position[0].vx, -22.55f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[0].vy, -9.0f, 0.0001f);
    CHECK(trace.color[0] == UINT32_C(0xa5808080));
    CHECK_FLOAT_CLOSE(trace.position[1].vx, -18.75f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[1].vy, -9.0f, 0.0001f);
    CHECK(trace.width[1] == 18.0f);
    CHECK(trace.color[1] == UINT32_C(0xbd108010));
    CHECK_FLOAT_CLOSE(trace.position[2].vx, 19.55f, 0.0001f);
    CHECK_FLOAT_CLOSE(trace.position[2].vy, -9.0f, 0.0001f);
    CHECK(trace.color[2] == UINT32_C(0xa5808080));
    CHECK(trace.position[0].vz == 430.0f);
    CHECK(trace.projectionDepth[0] == 500.0f);

    player.playerRoot.objectID = 0;
    player.playernum = 0;
    player.playerID = 0;

    trace.count = 0;
    OptionStruct.overlayMode = 2;
    _AddLifeTile(&player, &center);
    CHECK(trace.count == 0);
    OptionStruct.overlayMode = 1;

    trace.count = 0;
    CameraMatrix.t[2] = 0;
    _AddLifeTile(&player, &center);
    CHECK(trace.count == 0);
    CameraMatrix.t[2] = 500;

    trace.count = 0;
    OptionStruct.overlayMode = 0;
    _AddLifeTile(&player, &center);
    CHECK(trace.count == 0);
    OptionStruct.overlayMode = 1;
    GameStruct.screenShotFlag = 1;
    _AddLifeTile(&player, &center);
    CHECK(trace.count == 0);
    jpb_PlayerSetTileHook(NULL, NULL);
    return 0;
}

static int test_player_damage_tracker_owner(void)
{
    ScreenDrawTrace trace;
    sceneObject scene;
    physicsObject physics;
    playerObject *player;

    memset(&trace, 0, sizeof(trace));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(damageTracking, 0, sizeof(damageTracking));
    player_gInitPlayers(0);
    player = &gaPlayerData[0];
    player->playerRoot.objectID = 0;
    player->playerRoot.pParent = &scene.sceneRoot;
    player->playernum = 0;
    player->playerID = 0;
    player->pFlags = UINT32_C(0x80);
    scene.pScene = &scene.sceneRoot;
    scene.pPhysics = &physics.physicsRoot;
    GameStruct.CurrentLevel = 1;
    GameStruct.NumPlayers = 1;
    OptionStruct.overlayMode = 1;
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    gGlobalFrameRate = 1;
    damageTracking[0].total = 20.0f;
    gpWorld = NULL;
    raw_pad_bits = 0;
    jpb_InputSetProvider(test_read_pad, NULL);
    jpb_WHookSetDrawTextureHook(trace_screen_draw, &trace);

    player_gProcessPlayers();
    CHECK(trace.count == 1);
    CHECK(trace.destination[0].left == 50);
    CHECK(trace.destination[0].top == 127);
    CHECK(trace.destination[0].right == 70);
    CHECK(trace.destination[0].bottom == 140);
    CHECK(trace.color[0].r == UINT8_C(0xfc));
    CHECK(trace.color[0].g == UINT8_C(0xd4));
    CHECK(trace.color[0].b == UINT8_C(0x00));
    CHECK(trace.color[0].cd == UINT8_C(0x7f));
    CHECK(damageTracking[0].total == 19.25f);

    memset(&trace, 0, sizeof(trace));
    memset(damageTracking, 0, sizeof(damageTracking));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    CHECK(scaleAdjustment == 0.5f);
    damageTracking[0].total = 20.0f;
    player_gProcessPlayers();
    CHECK(trace.count == 1);
    CHECK(trace.destination[0].left == 25);
    CHECK(trace.destination[0].top == 63);
    CHECK(trace.destination[0].right == 35);
    CHECK(trace.destination[0].bottom == 70);
    CHECK(trace.color[0].r == UINT8_C(0xfc));
    CHECK(trace.color[0].g == UINT8_C(0xd4));
    CHECK(trace.color[0].b == UINT8_C(0x00));
    CHECK(trace.color[0].cd == UINT8_C(0x7f));
    CHECK(damageTracking[0].total == 19.25f);

    memset(&trace, 0, sizeof(trace));
    memset(damageTracking, 0, sizeof(damageTracking));
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustment = 1.0f;
    GameStruct.NumPlayers = 2;
    OptionStruct.overlayMode = 2;
    damageTracking[0].total = 20.0f;
    damageTracking[1].total = 30.0f;
    player = &gaPlayerData[1];
    player->playerRoot.objectID = 1;
    player->playerRoot.pParent = &scene.sceneRoot;
    player->playernum = 1;
    player->playerID = 1;
    player->pFlags = UINT32_C(0x80);

    player_gProcessPlayers();
    CHECK(trace.count == 2);
    CHECK(trace.destination[0].left == 211);
    CHECK(trace.destination[0].top == 127);
    CHECK(trace.destination[0].right == 231);
    CHECK(trace.destination[0].bottom == 140);
    CHECK(trace.color[0].r == UINT8_C(0xfc));
    CHECK(trace.color[0].g == UINT8_C(0xd4));
    CHECK(trace.color[0].b == UINT8_C(0x00));
    CHECK(trace.color[0].cd == UINT8_C(0x7f));
    CHECK(trace.destination[1].left == 399);
    CHECK(trace.destination[1].top == 127);
    CHECK(trace.destination[1].right == 429);
    CHECK(trace.destination[1].bottom == 140);
    CHECK(trace.color[1].r == UINT8_C(0xfc));
    CHECK(trace.color[1].g == UINT8_C(0xc0));
    CHECK(trace.color[1].b == UINT8_C(0x00));
    CHECK(trace.color[1].cd == UINT8_C(0x7f));
    CHECK(damageTracking[0].total == 19.25f);
    CHECK(damageTracking[1].total == 29.25f);

    memset(&trace, 0, sizeof(trace));
    memset(damageTracking, 0, sizeof(damageTracking));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    CHECK(scaleAdjustment == 0.5f);
    damageTracking[0].total = 20.0f;
    damageTracking[1].total = 30.0f;
    player_gProcessPlayers();
    CHECK(trace.count == 2);
    CHECK(trace.destination[0].left == 105);
    CHECK(trace.destination[0].top == 63);
    CHECK(trace.destination[0].right == 115);
    CHECK(trace.destination[0].bottom == 70);
    CHECK(trace.color[0].r == UINT8_C(0xfc));
    CHECK(trace.color[0].g == UINT8_C(0xd4));
    CHECK(trace.color[0].b == UINT8_C(0x00));
    CHECK(trace.color[0].cd == UINT8_C(0x7f));
    CHECK(trace.destination[1].left == 839);
    CHECK(trace.destination[1].top == 63);
    CHECK(trace.destination[1].right == 854);
    CHECK(trace.destination[1].bottom == 70);
    CHECK(trace.color[1].r == UINT8_C(0xfc));
    CHECK(trace.color[1].g == UINT8_C(0xc0));
    CHECK(trace.color[1].b == UINT8_C(0x00));
    CHECK(trace.color[1].cd == UINT8_C(0x7f));
    CHECK(damageTracking[0].total == 19.25f);
    CHECK(damageTracking[1].total == 29.25f);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    jpb_InputSetProvider(NULL, NULL);
    memset(damageTracking, 0, sizeof(damageTracking));
    return 0;
}

static int test_player_debug_hud_owner(void)
{
    Draw3dTrace trace;
    sceneObject scene[2];
    physicsObject physics[2];
    playerObject *player0;
    playerObject *player1;
    wsl_ENEMY enemy0;
    wsl_ENEMY enemy1;
    wsl_BAP_PLACEMENT place0;
    wsl_BAP_PLACEMENT place1;

    memset(&trace, 0, sizeof(trace));
    memset(scene, 0, sizeof(scene));
    memset(physics, 0, sizeof(physics));
    memset(&enemy0, 0, sizeof(enemy0));
    memset(&enemy1, 0, sizeof(enemy1));
    memset(&place0, 0, sizeof(place0));
    memset(&place1, 0, sizeof(place1));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(damageTracking, 0, sizeof(damageTracking));
    player_gInitPlayers(0);
    player0 = &gaPlayerData[0];
    player1 = &gaPlayerData[1];
    player0->playerRoot.objectID = 7;
    player0->playerRoot.pParent = &scene[0].sceneRoot;
    player0->playernum = 0;
    player0->playerID = 0;
    player0->pEnemy = &enemy0;
    player0->pFlags =
        UINT32_C(0x80) | UINT32_C(1) | UINT32_C(8);
    player0->forceFlags = UINT32_C(0x10) | UINT32_C(0x2000);
    scene[0].pScene = &scene[0].sceneRoot;
    scene[0].pPhysics = &physics[0].physicsRoot;
    enemy0.aiNum = 13;
    memcpy(enemy0.aName, "DROID", 6);
    enemy0.pPlace = &place0;
    place0.status = 5;
    memcpy(place0.aName, "SPAWN", 6);

    player1->playerRoot.objectID = 8;
    player1->playerRoot.pParent = &scene[1].sceneRoot;
    player1->playernum = 1;
    player1->playerID = 1;
    player1->pEnemy = &enemy1;
    player1->pFlags = UINT32_C(0x80);
    scene[1].pScene = &scene[1].sceneRoot;
    scene[1].pPhysics = &physics[1].physicsRoot;
    enemy1.aiNum = 21;
    memcpy(enemy1.aName, "BATTLE", 7);
    enemy1.pPlace = &place1;
    place1.status = 2;
    memcpy(place1.aName, "NODE", 5);

    GameStruct.CurrentLevel = 1;
    GameStruct.NumPlayers = 2;
    gpWorld = NULL;
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    OptionStruct.overlayMode = 1;
    OptionStruct.DebugLevel = 2;
    scaleAdjustment = 1.0f;
    raw_pad_bits = 0;
    jpb_InputSetProvider(test_read_pad, NULL);
    jpb_DebugTextSetDraw3dHook(trace_draw3d_text, &trace);

    player_gProcessPlayers();
    CHECK(trace.count == 2);
    CHECK(trace.x[0] == 18.0f);
    CHECK(trace.y[0] == 136.0f);
    CHECK(trace.z[0] == 50.0f);
    CHECK(trace.scale[0] == 1.0f);
    CHECK(trace.color[0] == UINT32_C(0xff8090a0));
    CHECK(strcmp(
              trace.text[0],
              "7-DROID ai 13\nt-\nLAND BOSS IMMORTAL AIR") == 0);
    CHECK(trace.x[1] == 487.0f);
    CHECK(trace.y[1] == 136.0f);
    CHECK(trace.z[1] == 519.0f);
    CHECK(strcmp(
              trace.text[1],
              "8-BATTLE ai 21\nt-\n   ") == 0);

    memset(&trace, 0, sizeof(trace));
    OptionStruct.DebugLevel = 3;
    physics[0].vpos.vx = 100;
    physics[0].vpos.vy = 200;
    physics[0].vpos.vz = 300;
    physics[1].vpos.vx = 400;
    physics[1].vpos.vy = 500;
    physics[1].vpos.vz = 600;
    player_gProcessPlayers();
    CHECK(trace.count == 2);
    CHECK(trace.x[0] == 100.0f);
    CHECK(trace.y[0] == 200.0f);
    CHECK(trace.z[0] == 300.0f);
    CHECK(strcmp(trace.text[0], "7-DROID SPAWN") == 0);
    CHECK(trace.x[1] == 400.0f);
    CHECK(trace.y[1] == 500.0f);
    CHECK(trace.z[1] == 600.0f);
    CHECK(strcmp(trace.text[1], "8-BATTLE NODE") == 0);

    jpb_DebugTextSetDraw3dHook(NULL, NULL);
    jpb_InputSetProvider(NULL, NULL);
    raw_pad_bits = 0;
    return 0;
}

int main(void)
{
    CHECK(ch_blipad(UINT16_C(0xffff)) == 0);
    CHECK(ch_pad(UINT16_C(0xffff)) == 0);
    ch_padadmin();
    CHECK(ch_unblipad(UINT16_C(0xffff)) == 0);
    CHECK(test_initialization_and_access() == 0);
    CHECK(test_motion_data_connection_and_loader_names() == 0);
    CHECK(test_pool_allocation() == 0);
    CHECK(test_scene_player_construction() == 0);
    CHECK(test_jedi_initialization_and_main_callback() == 0);
    CHECK(test_player_pad_sampling() == 0);
    CHECK(test_player_process_scheduler() == 0);
    CHECK(test_after_life_cleanup() == 0);
    CHECK(test_combo_and_player_reset() == 0);
    CHECK(test_player_refresh_start_and_checkpoint() == 0);
    CHECK(test_player_refresh_all_authored_starts() == 0);
    CHECK(test_player_free() == 0);
    CHECK(test_player_collision_owner() == 0);
    CHECK(test_jedi_ranged_weapon_owner() == 0);
    CHECK(test_player_sabre_owner() == 0);
    CHECK(test_player_life_tile_owner() == 0);
    CHECK(test_player_damage_tracker_owner() == 0);
    CHECK(test_player_debug_hud_owner() == 0);
    puts("player tests passed");
    return 0;
}
