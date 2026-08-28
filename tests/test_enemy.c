#include "jpb/ai.h"
#include "jpb/alloc.h"
#include "jpb/anim.h"
#include "jpb/audio_stream.h"
#include "jpb/camera.h"
#include "jpb/combo.h"
#include "jpb/enemy.h"
#include "jpb/extracharacters.h"
#include "jpb/effects.h"
#include "jpb/fx.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/jonny.h"
#include "jpb/loader.h"
#include "jpb/menu.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/whook.h"

#include <stdio.h>
#include <string.h>

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

typedef struct EnemyFixture {
    wsl_ENEMY enemy;
    playerObject player;
    sceneObject scene;
    physicsObject physics;
} EnemyFixture;

typedef struct RadarDraw {
    _Material *texture;
    SCREENRECT destination;
    int has_source;
    CVECTOR color;
    float layer_depth;
} RadarDraw;

typedef struct RadarTrace {
    RadarDraw draws[8];
    int count;
} RadarTrace;

static void capture_radar_draw(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth)
{
    RadarTrace *trace = (RadarTrace *)user_data;

    if (trace->count < 8) {
        RadarDraw *draw = &trace->draws[trace->count];

        draw->texture = texture;
        draw->destination = *destination;
        draw->has_source = source != NULL;
        draw->color = color;
        draw->layer_depth = layer_depth;
    }
    ++trace->count;
}

static void init_enemy(
    EnemyFixture *fixture,
    int object_id,
    int owner_type,
    int x,
    int y,
    int z)
{
    memset(fixture, 0, sizeof(*fixture));
    fixture->enemy.pPlayer = &fixture->player;
    fixture->enemy.ownerType = owner_type;
    fixture->player.playerRoot.pParent =
        &fixture->scene.sceneRoot;
    fixture->scene.pPhysics =
        &fixture->physics.physicsRoot;
    fixture->physics.physicsRoot.objectID =
        object_id;
    fixture->physics.vpos.vx = x;
    fixture->physics.vpos.vy = y;
    fixture->physics.vpos.vz = z;
}

static int list_length(const List *list)
{
    int length = 0;
    const Node *node;

    for (node = list->head; node != NULL; node = node->next) {
        ++length;
    }
    return length;
}

static int test_ai_defend_and_preframe(void)
{
    WorldData world;
    playerObject *target;
    playerObject *inactive;
    playerObject *defender;
    sceneObject *target_scene;
    sceneObject *inactive_scene;
    sceneObject *defender_scene;
    Motion target_motion;
    Motion defender_motion;
    Motion *target_motion_ptr;
    Motion *defender_motion_ptr;
    Combo target_combo;
    aiData ai_data;
    wsl_ENEMY enemy;

    memset(&world, 0, sizeof(world));
    memset(&target_motion, 0, sizeof(target_motion));
    memset(&defender_motion, 0, sizeof(defender_motion));
    memset(&target_combo, 0, sizeof(target_combo));
    memset(&ai_data, 0, sizeof(ai_data));
    memset(&enemy, 0, sizeof(enemy));
    memset(&GameStruct, 0, sizeof(GameStruct));
    player_gInitPlayers(0);
    physics_gInitObjects(0);
    jpb_SceneInitPool(0);

    target = &gaPlayerData[0];
    inactive = &gaPlayerData[1];
    defender = &gaPlayerData[2];
    target_scene = &maSceneData[0];
    inactive_scene = &maSceneData[1];
    defender_scene = &maSceneData[2];
    target->playerRoot.objectID = 0;
    inactive->playerRoot.objectID = 1;
    defender->playerRoot.objectID = 2;
    target->playerRoot.pParent =
        &target_scene->sceneRoot;
    inactive->playerRoot.pParent =
        &inactive_scene->sceneRoot;
    defender->playerRoot.pParent =
        &defender_scene->sceneRoot;
    target_scene->pScene = &target_scene->sceneRoot;
    inactive_scene->pScene = &inactive_scene->sceneRoot;
    defender_scene->pScene = &defender_scene->sceneRoot;
    target_scene->pPhysics =
        &maPhysicsData[0].physicsRoot;
    inactive_scene->pPhysics =
        &maPhysicsData[1].physicsRoot;
    defender_scene->pPhysics =
        &maPhysicsData[2].physicsRoot;
    target_scene->pPlayer = &target->playerRoot;
    inactive_scene->pPlayer = &inactive->playerRoot;
    defender_scene->pPlayer = &defender->playerRoot;
    maPhysicsData[0].physicsRoot.objectID = 0;
    maPhysicsData[1].physicsRoot.objectID = 1;
    maPhysicsData[2].physicsRoot.objectID = 2;
    maPhysicsData[0].physicsRoot.pParent =
        &target_scene->sceneRoot;
    maPhysicsData[1].physicsRoot.pParent =
        &inactive_scene->sceneRoot;
    maPhysicsData[2].physicsRoot.pParent =
        &defender_scene->sceneRoot;
    inactive_scene->sceneRoot.flags = UINT32_C(0x20);
    world.player0 = target;
    world.player1 = inactive;
    gpWorld = &world;

    target_motion.Damage = 1;
    target_motion.combo = 0;
    target_motion_ptr = &target_motion;
    target->pMotion = &target_motion_ptr;
    target->paCombos = &target_combo;
    defender_motion.Damage = 0;
    defender_motion_ptr = &defender_motion;
    defender->pMotion = &defender_motion_ptr;
    ai_data.block[0] = 100;
    ai_data.block[1] = 1;
    defender->paiMemory = &ai_data;
    defender->pEnemy = &enemy;
    enemy.pPlayer = defender;
    enemy.ownerType = 2;
    GameStruct.BlockRate = 14;
    LevelSelect = 0;
    maRange[0][2] = 100.0f;

    CHECK(ai_DefendCheck(defender) == 1);
    CHECK((defender->pFlags & UINT32_C(0x20)) != 0);
    CHECK(defender->target == target);
    CHECK(enemy.counter[2] == 3);

    maRange[0][2] = 400.0f;
    CHECK(ai_DefendCheck(defender) == 0);
    CHECK((defender->pFlags & UINT32_C(0x20)) == 0);

    defender->playerID = 10;
    defender->currentMotion = 3;
    enemy.active = 0;
    enemy.exit_flag = 0;
    enemy.aiLevel = 7;
    GameStruct.aCharacterData[2].Energy = 50;
    gGlobalTimer = 100;
    CHECK(bapenemy_preFrame(&enemy) == 1);
    CHECK(enemy.hitPoints == 50);
    CHECK(enemy.aiLevel == 107);
    GameStruct.aCharacterData[2].Energy = 0;
    CHECK(bapenemy_preFrame(&enemy) == 0);
    CHECK(enemy.active == 7);
    gpWorld = NULL;
    return 0;
}

typedef struct EnemyCreateProbe {
    int result;
    int calls;
    wsl_ENEMY *last_enemy;
} EnemyCreateProbe;

typedef struct AudioStreamProbe {
    int calls;
    int strIndex;
    int volume;
    int bLoop;
    char streamName[64];
} AudioStreamProbe;

typedef struct SpecialMessageProbe {
    int calls;
    const uint8_t *mess;
    uint16_t message_menu;
    uint16_t response_menu;
} SpecialMessageProbe;

typedef struct ScreenGlowProbe {
    int calls;
    int widths[4];
    uint32_t colors[4];
} ScreenGlowProbe;

static void screen_glow_probe(
    void *user_data,
    const _svector *start,
    const _svector *end,
    int width,
    uint32_t color)
{
    ScreenGlowProbe *probe =
        (ScreenGlowProbe *)user_data;

    if (probe->calls < 4) {
        probe->widths[probe->calls] = width;
        probe->colors[probe->calls] = color;
    }
    ++probe->calls;
    CHECK(start != NULL);
    CHECK(end != NULL);
}

static void audio_stream_probe(
    int strIndex,
    const char *streamName,
    int volume,
    int bLoop,
    void *user_data)
{
    AudioStreamProbe *probe =
        (AudioStreamProbe *)user_data;

    ++probe->calls;
    probe->strIndex = strIndex;
    probe->volume = volume;
    probe->bLoop = bLoop;
    snprintf(
        probe->streamName,
        sizeof(probe->streamName),
        "%s",
        streamName);
}

static void special_message_probe(
    const uint8_t *mess,
    uint16_t message_menu,
    uint16_t response_menu,
    void *user_data)
{
    SpecialMessageProbe *probe =
        (SpecialMessageProbe *)user_data;

    ++probe->calls;
    probe->mess = mess;
    probe->message_menu = message_menu;
    probe->response_menu = response_menu;
}

static int create_enemy_probe(
    wsl_ENEMY *enemy, void *user_data)
{
    EnemyCreateProbe *probe = (EnemyCreateProbe *)user_data;

    ++probe->calls;
    probe->last_enemy = enemy;
    return probe->result;
}

static void test_enemy_pool_initialization(void)
{
    WorldData world;
    wsl_BAP_PLACEMENT first;
    wsl_BAP_PLACEMENT second;
    wsl_BAP_PLACEMENT *placements[2];
    int index;

    memset(&world, 0, sizeof(world));
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    placements[0] = &first;
    placements[1] = &second;
    world.nEnemy = 2;
    world.apEnemy = placements;
    gpWorld = &world;
    first.aiDf.activeFlags = 0xffffffffU;
    second.aiDf.activeFlags = 0x10000001U;
    first.status = 17;
    second.status = 18;
    first.pLastEnemy = 23;
    second.pLastEnemy = 24;
    memset(abGlobalBits, 0xff, sizeof(abGlobalBits));
    for (index = 0; index < 4; ++index) {
        _aiFlagsTimer[index] = index + 1;
        _aiFlagsSave[index] = index + 5;
        _aiFlags[index] = index + 9;
    }

    enemy_InitEnemies();

    CHECK(enemyList[0].head == NULL);
    CHECK(enemyList[1].head == NULL);
    CHECK(list_length(&enemyFreeList) == 20);
    CHECK(enemyFreeList.head == &aEnemyListNodes[0].node);
    CHECK(enemyFreeList.tail == &aEnemyListNodes[19].node);
    CHECK(first.aiDf.activeFlags == 0xefffffffU);
    CHECK(second.aiDf.activeFlags == 1U);
    CHECK(first.status == 0);
    CHECK(second.status == 0);
    CHECK(first.pLastEnemy == UINT32_MAX);
    CHECK(second.pLastEnemy == UINT32_MAX);
    for (index = 0; index < 16; ++index) {
        CHECK(abGlobalBits[index] == 0);
    }
    for (index = 0; index < 4; ++index) {
        CHECK(_aiFlagsTimer[index] == 0);
        CHECK(_aiFlagsSave[index] == 0);
        CHECK(_aiFlags[index] == 0);
    }
}

static void test_init_enemy(void)
{
    WorldData world;
    wsl_BAP_PLACEMENT placement;
    wsl_ENEMY *enemy;

    memset(&world, 0, sizeof(world));
    memset(&placement, 0, sizeof(placement));
    gpWorld = &world;
    pointerRegistry_Reset();
    enemy_InitEnemies();

    placement.aiDf.activeFlags = 0x12345678U;
    placement.aiDf.startMode = 13;
    placement.aiDf.movementMode = 17;
    placement.aiDf.hitPoints = 325;
    placement.aiDf.movementSpeed = 91;
    placement.aiDf.range = 4096;
    placement.aiDf.ownerType = 2;
    placement.aiDf.skillLevel = 24;
    placement.aiNum = 7;
    placement.actorNum = 11;
    placement.loc.vx = 100;
    placement.loc.vy = -200;
    placement.loc.vz = 300;
    strcpy(placement.aName, "BATTLE");

    enemy = _initEnemy(&placement);

    CHECK(enemy == &aEnemyListNodes[0]);
    CHECK(list_length(&enemyFreeList) == 19);
    CHECK(enemy->pPlace == &placement);
    CHECK(enemy->aiNum == 7);
    CHECK(enemy->actorNum == 0);
    CHECK(enemy->pPlace->actorNum == 11);
    CHECK(enemy->ownerType == 2);
    CHECK(enemy->active == 1);
    CHECK(enemy->enemyFlags == 0x12345678U);
    CHECK(strcmp(enemy->aName, "BATTLE") == 0);
    CHECK(enemy->currAIMode == 13);
    CHECK(enemy->prevAIMode[0] == 13);
    CHECK(enemy->prevAIMode[1] == 13);
    CHECK(enemy->prevAIMode[2] == 13);
    CHECK(enemy->stackID == 0);
    CHECK(enemy->aiLocation == 0);
    CHECK(enemy->movementMode == 17);
    CHECK(enemy->hitPoints == 325);
    CHECK(enemy->movementSpeed == 91);
    CHECK(enemy->range == 4096);
    CHECK(enemy->location.vx == 100);
    CHECK(enemy->location.vy == -200);
    CHECK(enemy->location.vz == 300);
    CHECK(enemy->aiLevel == 4);
    CHECK(enemy->exit_flag == 0);
    CHECK(placement.pLastEnemy == 0);
    CHECK(
        getPtr(
            (int)placement.pLastEnemy,
            JPB_POINTER_ARRAY_ENEMY) == enemy);
}

static void test_add_enemy_orchestration(void)
{
    WorldData world;
    wsl_BAP_PLACEMENT first;
    wsl_BAP_PLACEMENT second;
    BAP_AI first_ai;
    BAP_AI second_ai;
    BAP_AI *ai[2];
    EnemyCreateProbe probe;
    wsl_ENEMY *created;

    memset(&world, 0, sizeof(world));
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    memset(&first_ai, 0, sizeof(first_ai));
    memset(&second_ai, 0, sizeof(second_ai));
    memset(&probe, 0, sizeof(probe));
    ai[0] = &first_ai;
    ai[1] = &second_ai;
    world.nAI = 2;
    world.apAI = ai;
    gpWorld = &world;
    pointerRegistry_Reset();
    enemy_InitEnemies();
    mCurEnemyList = 0;
    jpb_LoaderSetEnemyCreateTestHook(
        create_enemy_probe, &probe);

    first.aiNum = 0;
    first.aiDf.activeFlags =
        UINT32_C(0x10000001);
    strcpy(first.aName, "SPAWN");
    probe.result = 17;
    CHECK(_addEnemy(&first, 9, 1, 73) == 1);
    created = (wsl_ENEMY *)enemyList[0].head;
    CHECK(probe.calls == 1);
    CHECK(probe.last_enemy == created);
    CHECK(created == &aEnemyListNodes[0]);
    CHECK(created->enemyID == 9);
    CHECK(created->pAI == &second_ai);
    CHECK(first.aiDf.activeFlags == 1);
    CHECK(list_length(&enemyList[0]) == 1);
    CHECK(list_length(&enemyFreeList) == 19);

    second.aiNum = 0;
    second.pLastEnemy = 41;
    probe.result = 0;
    CHECK(_addEnemy(&second, 10, -1, 1) == 0);
    CHECK(probe.calls == 2);
    CHECK(probe.last_enemy == &aEnemyListNodes[1]);
    CHECK(probe.last_enemy->pAI == &first_ai);
    CHECK(second.pLastEnemy == UINT32_MAX);
    CHECK(list_length(&enemyList[0]) == 1);
    CHECK(list_length(&enemyFreeList) == 19);
    CHECK(enemyFreeList.tail == &aEnemyListNodes[1].node);

    second.aiDf.activeFlags = UINT32_C(0x10);
    second.pLastEnemy = 52;
    abGlobalBits[2] = 2;
    CHECK(_addEnemy(&second, 11, -1, 1) == 0);
    CHECK(probe.calls == 2);
    CHECK(second.pLastEnemy == 52);
    CHECK(list_length(&enemyFreeList) == 19);

    abGlobalBits[2] = 0;
    jpb_LoaderSetEnemyCreateTestHook(NULL, NULL);
}

static void initialize_placement_array(
    wsl_BAP_PLACEMENT *storage,
    wsl_BAP_PLACEMENT **placements,
    int count)
{
    int index;

    memset(
        storage,
        0,
        (size_t)count * sizeof(*storage));
    for (index = 0; index < count; ++index) {
        placements[index] = &storage[index];
    }
}

static void test_check_for_new_enemies(void)
{
    enum { PLACEMENT_COUNT = 30 };
    WorldData world;
    wsl_BAP_PLACEMENT storage[PLACEMENT_COUNT];
    wsl_BAP_PLACEMENT *placements[PLACEMENT_COUNT];
    BAP_AI ai_record;
    BAP_AI *ai[1];
    EnemyCreateProbe probe;

    memset(&world, 0, sizeof(world));
    memset(&ai_record, 0, sizeof(ai_record));
    memset(&probe, 0, sizeof(probe));
    initialize_placement_array(
        storage, placements, PLACEMENT_COUNT);
    ai[0] = &ai_record;
    world.location.vx = 100;
    world.location.vy = 200;
    world.location.vz = 300;
    world.nEnemy = PLACEMENT_COUNT;
    world.apEnemy = placements;
    world.nAI = 1;
    world.apAI = ai;
    gpWorld = &world;
    pointerRegistry_Reset();
    enemy_InitEnemies();
    mCurEnemyList = 0;
    probe.result = 1;
    jpb_LoaderSetEnemyCreateTestHook(
        create_enemy_probe, &probe);

    storage[0].aiDf.activeFlags =
        UINT32_C(0x10000001);
    storage[0].aiDf.aRange = 10;
    storage[0].loc.vx = 105;
    storage[0].loc.vy = 190;
    storage[0].loc.vz = 309;

    storage[1].aiDf.activeFlags = 1;
    storage[1].aiDf.aRange = 10;
    storage[1].loc.vx = 111;
    storage[1].loc.vy = 200;
    storage[1].loc.vz = 300;

    storage[2].aiDf.activeFlags = 1;
    storage[2].aiDf.aRange = 0;
    storage[2].loc.vx = 100000;

    storage[3].aiDf.activeFlags = 0;
    storage[3].aiDf.aRange = 1000;

    storage[4].aiDf.activeFlags = 1;
    storage[4].aiDf.aRange = 1000;
    storage[4].status = 2;

    _checkForNewEnemies();

    CHECK(probe.calls == 2);
    CHECK(storage[0].status == 1);
    CHECK(storage[1].status == 0);
    CHECK(storage[2].status == 1);
    CHECK(storage[3].status == 0);
    CHECK(storage[4].status == 2);
    CHECK(
        ((wsl_ENEMY *)enemyList[0].head)->enemyID == 0);
    CHECK(
        ((wsl_ENEMY *)enemyList[0].tail)->enemyID == 2);

    /*
     * The level-15 pair is outside its authored cube, but placement 24's
     * active status is the original explicit override.
     */
    initialize_placement_array(
        storage, placements, PLACEMENT_COUNT);
    pointerRegistry_Reset();
    enemy_InitEnemies();
    probe.calls = 0;
    probe.last_enemy = NULL;
    GameStruct.CurrentLevel = 15;
    storage[24].status = 1;
    storage[27].aiDf.activeFlags = 1;
    storage[27].aiDf.aRange = 1;
    storage[27].loc.vx = 10000;
    storage[29].aiDf.activeFlags = 1;
    storage[29].aiDf.aRange = 1;
    storage[29].loc.vz = -10000;

    _checkForNewEnemies();

    CHECK(probe.calls == 2);
    CHECK(storage[27].status == 1);
    CHECK(storage[29].status == 1);
    CHECK(
        ((wsl_ENEMY *)enemyList[0].head)->enemyID == 27);
    CHECK(
        ((wsl_ENEMY *)enemyList[0].tail)->enemyID == 29);

    memset(&GameStruct, 0, sizeof(GameStruct));
    jpb_LoaderSetEnemyCreateTestHook(NULL, NULL);
}

static void test_enemy_map_triggers(void)
{
    WorldData world;
    wsl_BAP_PLACEMENT first;
    wsl_BAP_PLACEMENT second;
    wsl_BAP_PLACEMENT *placements[2];
    int32_t anim_map[4] = {0, 0, 1, 0};
    int32_t direct_cube[2] = {
        (int32_t)UINT32_C(0x04000000),
        2 << 8
    };
    int32_t level_storage[40];
    int32_t indirect_cube[2] = {2 << 14, 0};

    memset(&world, 0, sizeof(world));
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    memset(level_storage, 0, sizeof(level_storage));
    placements[0] = &first;
    placements[1] = &second;
    world.apEnemy = placements;
    world.animMapEnemies = anim_map;
    gpWorld = &world;

    enemy_HandleMapTriggers(NULL);
    enemy_HandleMapTriggers(direct_cube);
    CHECK(first.aiDf.activeFlags == 0);
    CHECK(
        second.aiDf.activeFlags ==
        UINT32_C(0x10000000));

    second.aiDf.activeFlags = 1;
    enemy_HandleMapTriggers(direct_cube);
    CHECK(second.aiDf.activeFlags == 1);

    leveldata = &level_storage[4];
    leveldata[-4] = 0;
    leveldata[2 * 9] =
        (int32_t)UINT32_C(0x04000000);
    leveldata[2 * 9 + 1] = 2 << 8;
    second.aiDf.activeFlags = 0;
    enemy_HandleMapTriggers(indirect_cube);
    CHECK(
        second.aiDf.activeFlags ==
        UINT32_C(0x10000000));

    direct_cube[1] = 0xff << 8;
    second.aiDf.activeFlags = 0;
    enemy_HandleMapTriggers(direct_cube);
    CHECK(second.aiDf.activeFlags == 0);

    world.apEnemy = NULL;
    direct_cube[1] = 2 << 8;
    enemy_HandleMapTriggers(direct_cube);
}

static void test_enemy_pointer_index(void)
{
    WorldData world;
    wsl_BAP_PLACEMENT placement;
    wsl_BAP_PLACEMENT *placements[60];
    int index;

    memset(&world, 0, sizeof(world));
    memset(&placement, 0, sizeof(placement));
    for (index = 0; index < 60; ++index) {
        placements[index] = &placement;
    }
    world.apEnemy = placements;
    gpWorld = &world;
    placement.pLastEnemy = 20;

    GameStruct.CurrentLevel = 1;
    CHECK(enemy_getPointerIndex(0) == 20);
    GameStruct.CurrentLevel = 12;
    CHECK(enemy_getPointerIndex(0) == 20);
    GameStruct.CurrentLevel = 13;
    CHECK(enemy_getPointerIndex(0) == 20);
    GameStruct.CurrentLevel = 3;
    CHECK(enemy_getPointerIndex(0) == 20);

    GameStruct.CurrentLevel = 0;
    CHECK(enemy_getPointerIndex(0) == 19);
    GameStruct.CurrentLevel = 9;
    CHECK(enemy_getPointerIndex(45) == 20);
    CHECK(enemy_getPointerIndex(46) == 20);
    CHECK(enemy_getPointerIndex(49) == 20);
    CHECK(enemy_getPointerIndex(52) == 20);
    CHECK(enemy_getPointerIndex(56) == 20);
    CHECK(enemy_getPointerIndex(58) == 20);
    CHECK(enemy_getPointerIndex(44) == 19);
    CHECK(enemy_getPointerIndex(59) == 19);

    memset(&GameStruct, 0, sizeof(GameStruct));
}

static int test_ai_data_access(void)
{
    uint8_t storage[32] = {0};
    aiData *data = (aiData *)(void *)storage;
    int8_t direct[2] = {42, 1};
    int8_t indirect[2] = {20, 0};
    int8_t sequence[2] = {21, 3};
    int32_t pad[2] = {0};
    playerObject player;

    memset(&player, 0, sizeof(player));
    storage[20] = 77;
    storage[21] = 4;
    storage[22] = 5;
    storage[23] = 6;
    CHECK(ai_GetAiDataValue(data, direct) == 42);
    CHECK(ai_GetAiDataValue(data, indirect) == 77);
    CHECK(ai_GetAiDataValueN(data, sequence, 0) == 4);
    CHECK(ai_GetAiDataValueN(data, sequence, 2) == 6);
    CHECK(ai_GetAiSeqValue(data, 0, 0) == 0);
    storage[0] = 21;
    storage[1] = 3;
    CHECK(ai_GetAiSeqValue(data, 0, 2) == 6);
    CHECK(ai_GetAiSeqValue(data, 0, 3) == 0);
    CHECK(jpb_AiRegisterData(17, 2, data) == 1);
    CHECK(ai_GetAIHandle(17, 2) == data);
    CHECK(jpb_AiRegisterData(20, 2, data) == 0);
    CHECK(jpb_AiRegisterData(17, 4, data) == 0);
    ai_Main(pad, &player);
    CHECK(jpb_ai_MainCallback(pad, &player) == 0);
    player.playerID = 0x47;
    CHECK((unsigned char)ai_ValidateData(&player) == 0x47);
    player.playerID = 0x48;
    CHECK(ai_ValidateData(&player) == 0);
    player.playerID = 0x50;
    CHECK(ai_ValidateData(&player) == 0);
    CHECK(jpb_AiRegisterData(17, 2, NULL) == 1);
    return 0;
}

static void test_ai_node_traversal_and_mode_stack(void)
{
    uint8_t ai_storage[
        offsetof(BAP_AI, aiNodes) +
        7 * sizeof(BAP_AINODE)] = {0};
    BAP_AI *ai = (BAP_AI *)(void *)ai_storage;
    BAP_AINODE *nodes = ai->aiNodes;
    wsl_BAP_PLACEMENT placement;
    wsl_ENEMY enemy;

    memset(&placement, 0, sizeof(placement));
    memset(&enemy, 0, sizeof(enemy));
    ai->numNodes = 7;
    enemy.pAI = ai;
    enemy.pPlace = &placement;

    nodes[0].iChild = 1;
    nodes[1].iParent = 0;
    nodes[1].iChild = 4;
    nodes[1].iSibling = 2;
    nodes[1].opcode = 0;
    nodes[2].iParent = 0;
    nodes[2].iChild = 5;
    nodes[2].iSibling = 3;
    nodes[2].opcode = 1;
    nodes[3].iParent = 0;
    nodes[3].iChild = 6;
    nodes[3].iSibling = -1;
    nodes[3].opcode = 1;
    nodes[4].iParent = 1;
    nodes[5].iParent = 2;
    nodes[6].iParent = 3;

    CHECK(_countChildNodes(&enemy, &nodes[0]) == 3);
    CHECK(enemy.aiLocation == 3);
    enemy.aiLocation = 99;
    CHECK(enemy_GetNodePointer(&enemy, -1) == NULL);
    CHECK(enemy.aiLocation == 99);
    CHECK(enemy_GetNodePointer(&enemy, 7) == NULL);
    CHECK(enemy.aiLocation == 99);
    CHECK(enemy_GetNodePointer(&enemy, 3) == &nodes[3]);
    CHECK(enemy.aiLocation == 3);

    CHECK(bapEnemyStartCycleLoop(&enemy) == &nodes[1]);
    CHECK(enemy.pAINode == &nodes[1]);
    CHECK(enemy.aiLocation == 1);
    CHECK(bapEnemyGetNextOpcode(&enemy, 0) == &nodes[2]);
    CHECK(enemy.aiLocation == 2);
    CHECK(bapEnemyGetNextOpcode(&enemy, 1) == &nodes[5]);
    CHECK(enemy.aiLocation == 5);
    CHECK(bapEnemySetContinue(&enemy) == &nodes[2]);
    CHECK(enemy.aiLocation == 2);

    enemy.pAINode = &nodes[3];
    enemy.aiLocation = 3;
    CHECK(bapEnemyGetNextOpcode(&enemy, 0) == NULL);
    CHECK(enemy.pAINode == NULL);
    CHECK(enemy.aiLocation == 3);
    CHECK(bapEnemyGetNextOpcode(&enemy, 0) == NULL);

    enemy.currAIMode = 0;
    CHECK(bapEnemyDoModeJump(&enemy) == &nodes[5]);
    CHECK(enemy.aiLocation == 5);
    enemy.currAIMode = 1;
    CHECK(bapEnemyDoModeJump(&enemy) == &nodes[6]);
    CHECK(enemy.aiLocation == 6);
    enemy.currAIMode = 2;
    enemy.aiLocation = 44;
    CHECK(bapEnemyDoModeJump(&enemy) == NULL);
    CHECK(enemy.pAINode == NULL);
    CHECK(enemy.aiLocation == 3);

    enemy.currAIMode = 3;
    enemy.prevAIMode[0] = 2;
    enemy.prevAIMode[1] = 1;
    enemy.prevAIMode[2] = 0;
    enemy.stackID = 4;
    placement.aiDf.startMode = 9;
    bapenemy_changeAIMode(&enemy, 3);
    CHECK(enemy.currAIMode == 3);
    CHECK(enemy.stackID == 4);
    bapenemy_changeAIMode(&enemy, 7);
    CHECK(enemy.currAIMode == 7);
    CHECK(enemy.prevAIMode[0] == 3);
    CHECK(enemy.prevAIMode[1] == 2);
    CHECK(enemy.prevAIMode[2] == 1);
    CHECK(enemy.stackID == 5);
    bapenemy_returnAIMode(&enemy);
    CHECK(enemy.currAIMode == 3);
    CHECK(enemy.prevAIMode[0] == 2);
    CHECK(enemy.prevAIMode[1] == 1);
    CHECK(enemy.prevAIMode[2] == 9);
    CHECK(enemy.stackID == 4);
}

static void test_authored_opcode_traversal_boundary(void)
{
    enum { node_count = 10 };
    uint8_t ai_storage[
        offsetof(BAP_AI, aiNodes) +
        node_count * sizeof(BAP_AINODE)] = {0};
    BAP_AI *ai = (BAP_AI *)(void *)ai_storage;
    BAP_AINODE *nodes = ai->aiNodes;
    wsl_BAP_PLACEMENT placement;
    wsl_ENEMY enemy;
    playerObject player;
    sceneObject scene;
    physicsObject physics;
    Motion motions[3];
    WorldData world;
    wsl_BAP_PLACEMENT *placements[1];
    wsl_Powerup powerups[1];
    EffectHeader effect_header;
    AudioStreamProbe audio_probe;
    SpecialMessageProbe message_probe;
    ScreenGlowProbe glow_probe;
    uint8_t special_message[] = "objective";
    sceneObject target_scene;
    modelObject target_model;
    Motion target_motions[3];
    _animTemplate target_template;
    int32_t reset_map_storage[8];
    wsl_ENEMY vehicle_enemy;
    playerObject vehicle_player;
    sceneObject vehicle_scene;
    physicsObject vehicle_physics;
    wsl_ENEMY status_enemy;
    BAP_AI *world_ais[1];
    UDATA variables[3];
    uint16_t unsupported = UINT16_MAX;
    int node;

    memset(&placement, 0, sizeof(placement));
    memset(&enemy, 0, sizeof(enemy));
    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(motions, 0, sizeof(motions));
    memset(&world, 0, sizeof(world));
    memset(powerups, 0, sizeof(powerups));
    memset(&effect_header, 0, sizeof(effect_header));
    memset(&audio_probe, 0, sizeof(audio_probe));
    memset(&message_probe, 0, sizeof(message_probe));
    memset(&glow_probe, 0, sizeof(glow_probe));
    memset(&target_scene, 0, sizeof(target_scene));
    memset(&target_model, 0, sizeof(target_model));
    memset(target_motions, 0, sizeof(target_motions));
    memset(&target_template, 0, sizeof(target_template));
    memset(reset_map_storage, 0, sizeof(reset_map_storage));
    memset(&vehicle_enemy, 0, sizeof(vehicle_enemy));
    memset(&vehicle_player, 0, sizeof(vehicle_player));
    memset(&vehicle_scene, 0, sizeof(vehicle_scene));
    memset(&vehicle_physics, 0, sizeof(vehicle_physics));
    memset(&status_enemy, 0, sizeof(status_enemy));
    placements[0] = &placement;
    memset(variables, 0, sizeof(variables));
    for (node = 0; node < node_count; ++node) {
        nodes[node].iParent = -1;
        nodes[node].iChild = -1;
        nodes[node].iSibling = -1;
    }

    ai->numNodes = node_count;
    ai->numAvailable = 0;
    ai->bSize = (int)sizeof(ai_storage);
    enemy.pAI = ai;
    enemy.pPlace = &placement;
    enemy.currAIMode = 0;
    player.playerRoot.pParent =
        (objectRoot *)(void *)&scene;
    player.paMotions = motions;
    player.maxMotions = 3;
    scene.pPhysics =
        (objectRoot *)(void *)&physics;

    /*
     * This is the exact root/mode shape used by FED AI 12: node 1 is the
     * mode-jump sentinel, node 2 is mode zero, and its child is executed.
     */
    nodes[0].iChild = 1;
    nodes[1].iParent = 0;
    nodes[1].iSibling = 2;
    nodes[1].opcode = 0x106;
    nodes[2].iParent = 0;
    nodes[2].iChild = 3;
    nodes[2].opcode = 1;
    nodes[3].iParent = 2;
    nodes[3].iSibling = 4;
    nodes[3].opcode = 0x4410;
    nodes[3].vx.si = 1;
    nodes[4].iParent = 2;
    nodes[4].iSibling = 5;
    nodes[4].opcode = 0x4400;
    nodes[4].vx.si = 1;
    nodes[5].iParent = 2;
    nodes[5].opcode = 0x200;

    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(unsupported == 0);
    CHECK(enemy.lastWayPoint == 1);
    CHECK(enemy.currAIMode == 1);
    CHECK(enemy.stackID == 1);

    pointerRegistry_Reset();
    variables[0].si = 2;
    variables[1].si = 77;
    ai->pVars = (uint32_t)addPtr(
        variables, JPB_POINTER_ARRAY_AI);
    ai->bSize += (int)sizeof(variables);

    /* Opcode 0x108 is the authored "continue" operation.  FED AI 31 uses
     * it beneath a successful branch to resume at the branch node's
     * sibling.  The original reads iParent (the first BAP_AINODE short),
     * not iChild. */
    variables[0].si = 2;
    variables[1].si = 77;
    enemy.currAIMode = 0;
    enemy.stackID = 0;
    memset(enemy.counter, 0, sizeof(enemy.counter));
    nodes[2].iSibling = 4;
    nodes[3].opcode = 0x108;
    nodes[3].iChild = 6;
    nodes[4].opcode = 0x405;
    nodes[4].vx.ui = 0;
    nodes[4].iSibling = 5;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.counter[2] == 77);

    nodes[2].iSibling = -1;
    nodes[3].iChild = -1;
    nodes[4].opcode = 0x4400;
    nodes[4].vx.si = 1;
    nodes[4].iSibling = 5;
    enemy.currAIMode = 0;
    enemy.stackID = 0;
    memset(enemy.counter, 0, sizeof(enemy.counter));
    nodes[3].opcode = 0x405;
    nodes[3].vx.ui = 0;
    nodes[3].iSibling = 5;

    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.counter[2] == 77);

    variables[0].si = 1;
    variables[1].f = 25.0f;
    enemy.currAIMode = 0;
    enemy.aiTimer = 50;
    enemy.lastWayPoint = 0;
    gGlobalTimer = 100;
    nodes[3].opcode = 0x109;
    nodes[3].vx.ui = 0;
    nodes[3].iChild = 4;
    nodes[3].iSibling = 5;
    nodes[4].opcode = 0x4410;
    nodes[4].vx.si = 2;
    nodes[4].iSibling = 5;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.lastWayPoint == 2);

    variables[0].si = 0;
    variables[1].si = 0;
    enemy.currAIMode = 0;
    enemy.exit_flag = 0;
    enemy.enemyID = 0x49;
    LevelSelect = 15;
    moveTaxi = 0;
    nodes[3].opcode = 0x40c;
    nodes[3].vx.ui = 0;
    nodes[3].iChild = -1;
    nodes[3].iSibling = 5;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.exit_flag == 1);
    CHECK(moveTaxi == 1);

    variables[0].si = 99;
    variables[1].si = 1;
    enemy.currAIMode = 0;
    nodes[3].opcode = 0x610;
    LevelSelect = 8;
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK((abGlobalBits[99 >> 3] &
           (1U << (99 & 7))) == 0);

    enemy.currAIMode = 0;
    LevelSelect = 7;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK((abGlobalBits[99 >> 3] &
           (1U << (99 & 7))) != 0);

    variables[1].si = 0;
    enemy.currAIMode = 0;
    LevelSelect = 8;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK((abGlobalBits[99 >> 3] &
           (1U << (99 & 7))) == 0);

    variables[0].si = 0;
    enemy.counter[0] = 20;
    enemy.currAIMode = 0;
    enemy.lastWayPoint = 0;
    nodes[3].opcode = 0x105;
    nodes[3].vx.ui = 0;
    nodes[3].iChild = 4;
    nodes[3].iSibling = 9;
    nodes[4].opcode = 0x180;
    nodes[4].vx.si = 10;
    nodes[4].iChild = 6;
    nodes[4].iSibling = 5;
    nodes[5].opcode = 0x180;
    nodes[5].vx.si = 20;
    nodes[5].iChild = 7;
    nodes[5].iSibling = -1;
    nodes[6].opcode = 0x4410;
    nodes[6].vx.si = 3;
    nodes[6].iSibling = 9;
    nodes[7].opcode = 0x4410;
    nodes[7].vx.si = 4;
    nodes[7].iSibling = 9;
    nodes[9].opcode = 0x200;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.lastWayPoint == 4);

    variables[0].si = 6;
    enemy.currAIMode = 0;
    enemy.lastWayPoint = 0;
    enemy.switchData[0] = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.lastWayPoint == 3);
    CHECK(enemy.switchData[0] == 1);
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.lastWayPoint == 4);
    CHECK(enemy.switchData[0] == 2);
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.lastWayPoint == 3);
    CHECK(enemy.switchData[0] == 1);

    variables[0].si = 1;
    variables[1].si = 0;
    variables[2].si = 0;
    enemy.currAIMode = 0;
    enemy.lastWayPoint = 0;
    nodes[3].opcode = 0x100;
    nodes[3].vx.ui = 0;
    nodes[3].iChild = 8;
    nodes[3].iSibling = 9;
    nodes[8].opcode = 0x4410;
    nodes[8].vx.si = 5;
    nodes[8].iSibling = 9;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.lastWayPoint == 5);

    variables[0].si = 84;
    enemy.currAIMode = 0;
    enemy.pPlayer = &player;
    player.currentMotion = 1;
    player.playerID = battle_d_model;
    nodes[3].opcode = 0x20e;
    nodes[3].iChild = -1;
    nodes[3].iSibling = 9;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);

    variables[0].f = 2.5f;
    enemy.currAIMode = 0;
    enemy.pPlayer = &player;
    nodes[3].opcode = 0x408;
    nodes[3].iChild = -1;
    nodes[3].iSibling = 9;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.movementSpeed == 40);
    CHECK(motions[1].vel == 40);
    CHECK(motions[2].vel == 40);

    variables[0].si = 3;
    enemy.currAIMode = 0;
    nodes[3].opcode = 0x409;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.movementMode == 3);
    CHECK(physics.movemode == MOVE_HOVER3D);
    CHECK((physics.flags & 0x2000U) != 0);

    variables[0].si = 99;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.movementMode == 0);
    CHECK(physics.movemode == MOVE_NORMAL);
    CHECK((physics.flags & 0x2000U) == 0);

    variables[0].si = 2;
    enemy.currAIMode = 0;
    nodes[3].opcode = 0x20c;
    nodes[3].iSibling = 9;
    world.currentDolly = 3;
    gpWorld = &world;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(world.aDolly[3].flags == 0x20U);
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(world.aDolly[3].flags == 0);

    variables[0].si = 6;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(world.aDolly[3].flags == 0x1e0U);

    variables[0].si = 7;
    variables[1].si = 1;
    enemy.currAIMode = 0;
    enemy.aiNum = 0;
    nodes[3].opcode = 0x604;
    player.pFlags = 0;
    world.player0 = &player;
    world.player1 = &player;
    GameStruct.CurrentLevel = 0;
    GameStruct.screenShotFlag = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(world.overRideDolly == 7);
    CHECK((player.pFlags & 2U) != 0);
    CHECK(GameStruct.screenShotFlag == 1);

    variables[0].si = -1;
    variables[1].si = 0;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(world.overRideDolly == 0);
    CHECK((player.pFlags & 2U) == 0);
    CHECK(GameStruct.screenShotFlag == 0);

    variables[0].si = 9;
    variables[1].si = 1;
    enemy.currAIMode = 0;
    enemy.aiNum = 0x10;
    GameStruct.CurrentLevel = 5;
    GameStruct.checkpoint[5] = 5;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(world.overRideDolly == 9);
    CHECK((player.pFlags & 2U) != 0);
    CHECK(GameStruct.screenShotFlag == 1);

    world.overRideDolly = 0;
    player.pFlags = 0;
    GameStruct.screenShotFlag = 0;
    GameStruct.checkpoint[5] = 6;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(world.overRideDolly == 0);
    CHECK((player.pFlags & 2U) == 0);
    CHECK(GameStruct.screenShotFlag == 0);

    variables[0].si = 0;
    variables[1].si = 0;
    variables[2].f = 2.0f;
    enemy.currAIMode = 0;
    enemy.aiNum = 0;
    enemy.enemyID = 77;
    nodes[3].opcode = 0x606;
    world.nEnemy = 1;
    world.apEnemy = placements;
    placement.loc.vx = 100;
    placement.loc.vy = 200;
    placement.loc.vz = 300;
    physics.pos.vx = 10.5f;
    physics.pos.vy = 20.5f;
    physics.pos.vz = 30.5f;
    physics.vpos.vx = 11;
    physics.vpos.vy = 22;
    physics.vpos.vz = 33;
    GameStruct.CurrentLevel = 9;
    tflag = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(tele == 77);
    CHECK(trange == 512);
    CHECK(toff.vx == 89);
    CHECK(toff.vy == 179);
    CHECK(toff.vz == 269);
    CHECK(tpos.vx == 11);
    CHECK(tpos.vy == 22);
    CHECK(tpos.vz == 33);
    CHECK(savedPlayerPos.vx == 11);
    CHECK(savedPlayerPos.vy == 22);
    CHECK(savedPlayerPos.vz == 33);
    CHECK(tflag == 1);

    variables[0].si = 1;
    variables[1].si = 0;
    variables[2].f = 0.0f;
    enemy.currAIMode = 0;
    world.nPowerups = 1;
    world.pPowerups = powerups;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK((powerups[0].data & 0x8000U) != 0);
    variables[2].f = 0.5f;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(powerups[0].rate == 15);
    CHECK((powerups[0].data & 0x8000U) == 0);

    variables[0].si = 2;
    variables[1].si = 1;
    variables[2].si = 0;
    enemy.currAIMode = 0;
    enemy.aiNum = 0;
    nodes[3].opcode = 0x606;
    GameStruct.CurrentLevel = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK((player.pFlags & 0x10U) != 0);
    variables[1].si = 0;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK((player.pFlags & 0x10U) == 0);

    variables[0].si = 3;
    variables[1].si = 12;
    variables[2].si = 0;
    enemy.currAIMode = 0;
    paEffects[12] = &effect_header;
    GameStruct.GameState |= UINT32_C(0x02000000);
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    GameStruct.GameState &=
        ~UINT32_C(0x02000000);
    paEffects[12] = NULL;

    variables[0].si = 4;
    enemy.currAIMode = 0;
    screenshake = 0;
    screenshakeamplitude = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(screenshake == 0x100);
    CHECK(screenshakeamplitude == 6);

    variables[0].si = 6;
    variables[1].si = 123;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(motions[0].globalID == 123);

    variables[0].si = 8;
    variables[1].si = 11;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(world.overRideDolly == 11);

    variables[0].si = 12;
    variables[1].si = 1;
    enemy.currAIMode = 0;
    physics.flags = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK((physics.flags & 0x400000U) != 0);

    variables[0].si = 14;
    variables[1].si = 0;
    variables[2].f = 2.0f;
    enemy.currAIMode = 0;
    physics.pos.vx = 10.75f;
    physics.pos.vy = -20.25f;
    physics.pos.vz = 30.5f;
    world.currentDolly = 4;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(uberXRange == 512);
    CHECK(uberPos.vx == 10);
    CHECK(uberPos.vy == -20);
    CHECK(uberPos.vz == 30);
    CHECK(world.overRideDolly == 4);

    variables[0].si = 19;
    variables[2].f = 0.0f;
    enemy.currAIMode = 0;
    uberLock = 1;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(uberXRange == 0);
    CHECK(world.overRideDolly == -1);
    CHECK(uberLock == 0);

    variables[0].si = 16;
    variables[1].si = 44;
    variables[2].si = 1;
    enemy.currAIMode = 0;
    player.currentMotion = 3;
    world.player0 = &player;
    GameStruct.gameMode = 0;
    OptionStruct.Music = 1;
    OptionStruct.musicVolume = 7;
    jpb_AudioStreamSetPlayHook(
        audio_stream_probe, &audio_probe);
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(audio_probe.calls == 1);
    CHECK(audio_probe.strIndex == 47);
    CHECK(strcmp(
              audio_probe.streamName,
              "AnakinHyperdrive_AG.wav") == 0);
    CHECK(audio_probe.volume == 14);
    CHECK(audio_probe.bLoop == 1);

    variables[1].si = 60;
    variables[2].si = 0;
    enemy.currAIMode = 0;
    GameStruct.gameMode = 2;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(audio_probe.calls == 2);
    CHECK(audio_probe.strIndex == 71);
    CHECK(strcmp(
              audio_probe.streamName,
              "RescueQueen_AG2.wav") == 0);
    CHECK(audio_probe.bLoop == 0);

    OptionStruct.Music = 0;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(audio_probe.calls == 2);
    jpb_AudioStreamSetPlayHook(NULL, NULL);
    OptionStruct.musicVolume = 0;
    GameStruct.gameMode = 0;

    variables[0].si = 17;
    variables[1].si = 3;
    variables[2].si = 0;
    enemy.currAIMode = 0;
    LevelSelect = 9;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    LevelSelect = 0;

    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    player_gInitPlayers(0);
    init_test_animations();
    gaPlayerData[0].playerRoot.objectID = 0;
    gaPlayerData[0].playerRoot.pParent =
        &target_scene.sceneRoot;
    gaPlayerData[0].playernum = 0;
    gaPlayerData[0].maxMotions = 3;
    gaPlayerData[0].paMotions = target_motions;
    maPhysicsData[0].physicsRoot.objectID = 0;
    maPhysicsData[0].physicsRoot.pParent =
        &target_scene.sceneRoot;
    maAnimationData[0].animRoot.objectID = 0;
    maAnimationData[0].animRoot.pParent =
        &target_scene.sceneRoot;
    maAnimationData[0].paMotions = target_motions;
    maAnimationData[0].depack_context.seqdata =
        &target_template;
    target_scene.pPlayer =
        &gaPlayerData[0].playerRoot;
    target_scene.pScene =
        &target_scene.sceneRoot;
    target_scene.pPhysics =
        &maPhysicsData[0].physicsRoot;
    target_scene.pAnim =
        &maAnimationData[0].animRoot;
    target_scene.pModel =
        &target_model.modelRoot;
    target_motions[0].Seq = 0;
    target_motions[0].FunctPtr = 0;
    target_motions[0].snd[0][0] = '0';
    target_motions[0].snd[1][0] = '0';
    target_template.Lframe = 10;
    physics.pos.vx = 111.0f;
    physics.pos.vy = 222.0f;
    physics.pos.vz = 333.0f;
    physics.angle.vx = 4.0f;
    physics.angle.vy = 5.0f;
    physics.angle.vz = 6.0f;
    world.player0 = &gaPlayerData[0];
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.GameState |= UINT32_C(0x1000);
    afterLife = NULL;
    leveldata = reset_map_storage + 4;
    leveldata[-2] = 0;
    numsolids = 0;
    LevelSelect = 1;
    variables[0].si = 18;
    variables[1].si = 0;
    variables[2].si = 0;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(maPhysicsData[0].pos.vx == 111.0f);
    CHECK(maPhysicsData[0].pos.vz == 333.0f);
    CHECK(maPhysicsData[0].angle.vx == 4.0f);
    CHECK(maPhysicsData[0].angle.vy == 5.0f);
    CHECK(maPhysicsData[0].angle.vz == 6.0f);
    CHECK((gaPlayerData[0].pFlags & 2U) != 0);
    CHECK(
        (GameStruct.GameState &
         UINT32_C(0x1000)) == 0);

    meminit();
    sprite_gInitSprites();
    GameStruct.NumPlayers = 2;
    GameStruct.aCharacterData[0].Energy = 0;
    GameStruct.aCharacterData[0].Force = 73;
    GameStruct.aCharacterData[0].Score = 4567;
    GameStruct.Counter = 321;
    afterLife = NULL;
    unsupported = 0;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(unsupported == 0);
    CHECK(afterLife == NULL);
    CHECK(gaPlayerData[0].shadow != NULL);
    CHECK(GameStruct.aCharacterData[0].Force == 73);
    CHECK(GameStruct.aCharacterData[0].Score == 4567);
    CHECK(GameStruct.Counter == 321);
    CHECK(maPhysicsData[0].pos.vx == 111.0f);
    CHECK(maPhysicsData[0].pos.vy == -32760.0f);
    CHECK(maPhysicsData[0].pos.vz == 333.0f);
    CHECK((gaPlayerData[0].pFlags & 2U) != 0);

    world_ais[0] = ai;
    world.apAI = world_ais;
    world.nAI = 1;
    world.nEnemy = 1;
    world.apEnemy = placements;
    placement.aiDf.ownerType = 4;
    placement.status = 0;
    placement.aiDf.enemyExt[0] = 0;
    target_motions[1].Lock = 7;
    target_motions[2].Lock = 0;
    GameStruct.aCharacterData[0].Force = 55;
    list_InitList(&enemyList[0]);
    list_InitList(&enemyFreeList);
    list_AddTail(
        &enemyFreeList, &status_enemy.node);
    mCurEnemyList = 0;
    variables[0].si = 0;
    variables[1].si = 0;
    enemy.currAIMode = 0;
    nodes[3].opcode = 0x60f;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(gaPlayerData[0].pEnemy == &status_enemy);
    CHECK(status_enemy.pPlayer == &gaPlayerData[0]);
    CHECK(status_enemy.pAI == ai);
    CHECK(status_enemy.enemyID == 0);
    CHECK((gaPlayerData[0].pFlags & 0x10U) != 0);
    CHECK(target_motions[2].Lock == 7);
    CHECK(enemyList[0].head == &status_enemy.node);
    CHECK(GameStruct.aCharacterData[0].Force == 55);
    CHECK(afterLife == NULL);
    CHECK(gaPlayerData[0].shadow != NULL);

    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(gaPlayerData[0].pEnemy == NULL);
    CHECK(status_enemy.exit_flag == 1);
    CHECK((gaPlayerData[0].pFlags & 0x10U) == 0);
    CHECK(target_motions[2].Lock == 0x19);
    placement.aiDf.ownerType = 0;
    nodes[3].opcode = 0x606;
    world.player0 = &player;
    leveldata = NULL;
    LevelSelect = 0;

    variables[0].si = 22;
    variables[1].si = 22;
    variables[2].si = 0;
    enemy.currAIMode = 0;
    allText[22] = special_message;
    allText[376] = NULL;
    GameStruct.GameState &=
        ~UINT32_C(0x02000000);
    GameStruct.inMenuFlag = 0;
    jpb_MenuSetSpecialMessageHook(
        special_message_probe,
        &message_probe);
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(message_probe.calls == 1);
    CHECK(message_probe.mess == special_message);
    CHECK(message_probe.message_menu == 0x41);
    CHECK(message_probe.response_menu == 0x2b);
    CHECK(
        (GameStruct.GameState &
         UINT32_C(0x02000000)) != 0);
    CHECK(GameStruct.inMenuFlag == 1);
    jpb_MenuSetSpecialMessageHook(NULL, NULL);
    allText[22] = NULL;

    variables[0].si = 23;
    variables[1].si = 0;
    enemy.currAIMode = 0;
    player.pFlags = 2;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK((player.pFlags & 2U) == 0);

    variables[0].si = 10;
    variables[1].si = 0;
    variables[2].si = 0;
    enemy.currAIMode = 0;
    enemy.lastWayPoint = 0;
    nodes[3].iChild = 8;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.lastWayPoint == 5);

    vehicle_enemy.pPlayer = &vehicle_player;
    vehicle_player.playerRoot.pParent =
        &vehicle_scene.sceneRoot;
    vehicle_player.playerID = 99;
    vehicle_scene.pPhysics =
        &vehicle_physics.physicsRoot;
    placement.aiDf.enemyExt[0] = 0;
    placement.pLastEnemy =
        (uint32_t)addPtr(
            &vehicle_enemy,
            JPB_POINTER_ARRAY_ENEMY);
    variables[0].si = 0;
    variables[1].si = 0;
    enemy.currAIMode = 0;
    nodes[3].opcode = 0x607;
    nodes[3].iChild = -1;
    GameStruct.CurrentLevel = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);

    vehicle_player.playerID = 0x23;
    vehicle_player.playernum = 1;
    vehicle_player.playerRoot.objectID = 7;
    vehicle_physics.vpos =
        maPhysicsData[0].vpos;
    gaPlayerData[0].pFlags = 1;
    gaPlayerData[0].pForceCallBack = NULL;
    world.player0 = &gaPlayerData[0];
    world.player1 = NULL;
    timesincetank[0] = 0;
    jumpheld[0] = 0;
    tankdrivers[0] = NULL;
    tankdrivers[1] = NULL;
    enemy.currAIMode = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(tankID == 7);
    CHECK((gaPlayerData[0].pFlags & 0x80U) != 0);
    CHECK(
        (vehicle_enemy.enemyFlags &
         UINT32_C(1) << 26) != 0);
    CHECK(
        (vehicle_player.pFlags &
         (UINT32_C(1) << 26)) == 0);
    CHECK(jumpheld[0] == 1);
    CHECK(tankdrivers[0] == &gaPlayerData[0]);
    CHECK(
        GameStruct.aCharacterData[1].Energy ==
        0xfe);
    CHECK(
        vehicle_player.pMainCallBack ==
        funcArray[30]);
    world.player0 = &player;
    world.player1 = &player;

    variables[0].si = 11;
    enemy.currAIMode = 0;
    nodes[3].opcode = 0x606;
    nodes[3].iChild = -1;
    unsupported = 0;
    jpb_FxSetScreenGlowHook(
        screen_glow_probe, &glow_probe);
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(unsupported == 0);
    CHECK(glow_probe.calls == 4);
    CHECK(glow_probe.widths[0] == 8);
    CHECK(glow_probe.widths[1] == 16);
    CHECK(glow_probe.widths[2] == 4);
    CHECK(glow_probe.widths[3] == 0x60);
    CHECK(
        glow_probe.colors[0] ==
        UINT32_C(0xc0ffffff));
    CHECK(
        glow_probe.colors[1] ==
        UINT32_C(0xc0ff8020));
    CHECK(
        glow_probe.colors[2] ==
        UINT32_C(0xc0ff8020));
    CHECK(
        glow_probe.colors[3] ==
        UINT32_C(0xc0ff4020));
    jpb_FxSetScreenGlowHook(NULL, NULL);

    gpWorld = NULL;
    GameStruct.CurrentLevel = 0;
    GameStruct.checkpoint[5] = 0;

    nodes[3].opcode = 0x411;
    nodes[3].iChild = -1;
    nodes[3].iSibling = 8;
    nodes[8].opcode = 0x4410;
    nodes[8].vx.si = 5;
    nodes[8].iChild = -1;
    nodes[8].iSibling = 9;
    enemy.currAIMode = 0;
    enemy.lastWayPoint = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(enemy.lastWayPoint == 5);

    /*
     * The matched void owner treats an unknown opcode as a debug-range
     * reset/no-op and continues. The diagnostic facade below deliberately
     * reports the same input so coverage gaps remain visible to tests.
     */
    nodes[3].opcode = 0x6ff;
    nodes[3].iChild = -1;
    nodes[3].iSibling = 8;
    enemy.pPlayer = &player;
    enemy.currAIMode = 0;
    enemy.lastWayPoint = 0;
    player.pFlags |= UINT32_C(0x80000000);
    enemy_ParseOpcodes(&enemy);
    CHECK(enemy.lastWayPoint == 5);
    CHECK(
        (player.pFlags & UINT32_C(0x80000000)) ==
        0);

    nodes[3].iSibling = 9;
    enemy.currAIMode = 0;
    unsupported = 0;
    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_UNSUPPORTED);
    CHECK(unsupported == 0x6ff);
    LevelSelect = 0;
    moveTaxi = 0;
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    pointerRegistry_Reset();
}

static void test_authored_opcode_cycle_enters_at_root_child(void)
{
    enum { node_count = 5 };
    uint8_t ai_storage[
        offsetof(BAP_AI, aiNodes) +
        node_count * sizeof(BAP_AINODE)] = {0};
    BAP_AI *ai = (BAP_AI *)(void *)ai_storage;
    BAP_AINODE *nodes = ai->aiNodes;
    wsl_ENEMY enemy;
    uint16_t unsupported = UINT16_MAX;
    int node;

    memset(&enemy, 0, sizeof(enemy));
    for (node = 0; node < node_count; ++node) {
        nodes[node].iParent = -1;
        nodes[node].iChild = -1;
        nodes[node].iSibling = -1;
    }
    ai->numNodes = node_count;
    ai->numAvailable = 0;
    ai->bSize = (int)sizeof(ai_storage);
    enemy.pAI = ai;
    enemy.currAIMode = 0;

    /*
     * The root-side command must run before 0x106 redirects into the
     * selected mode.  Entering directly through bapEnemyDoModeJump skips
     * node 1 and was the source of scene directors advancing incorrectly.
     */
    nodes[0].iChild = 1;
    nodes[1].iParent = 0;
    nodes[1].iSibling = 2;
    nodes[1].opcode = 0x4410;
    nodes[1].vx.si = 9;
    nodes[2].iParent = 0;
    nodes[2].iSibling = 3;
    nodes[2].opcode = 0x106;
    nodes[3].iParent = 0;
    nodes[3].iChild = 4;
    nodes[3].opcode = 1;
    nodes[4].iParent = 3;
    nodes[4].opcode = 0x200;

    CHECK(jpb_enemy_ParseOpcodes(
              &enemy, &unsupported) ==
          JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(unsupported == 0);
    CHECK(enemy.lastWayPoint == 9);
    CHECK(enemy.aiLocation == 4);
}

static void test_ai_script_arithmetic_and_comparisons(void)
{
    float float_value = 10.0f;
    int int_value = 10;

    aisub_arithmeticFVariables(&float_value, 0, 4.0f);
    CHECK(float_value == 4.0f);
    aisub_arithmeticFVariables(&float_value, 1, 3.0f);
    CHECK(float_value == 7.0f);
    aisub_arithmeticFVariables(&float_value, 2, 2.0f);
    CHECK(float_value == 5.0f);
    aisub_arithmeticFVariables(&float_value, 9, 100.0f);
    CHECK(float_value == 5.0f);

    aisub_arithmeticSIVariables(&int_value, 0, 4);
    CHECK(int_value == 4);
    aisub_arithmeticSIVariables(&int_value, 1, 3);
    CHECK(int_value == 7);
    aisub_arithmeticSIVariables(&int_value, 2, 2);
    CHECK(int_value == 5);
    aisub_arithmeticSIVariables(&int_value, 3, 5);
    CHECK(int_value >= 1 && int_value <= 5);
    aisub_arithmeticSIVariables(&int_value, 3, 0);
    CHECK(int_value == 0);
    int_value = 10;
    aisub_arithmeticSIVariables(&int_value, 4, 5);
    CHECK(int_value >= 11 && int_value <= 15);
    aisub_arithmeticSIVariables(&int_value, 4, -1);
    CHECK(int_value >= 11 && int_value <= 15);
    aisub_arithmeticSIVariables(&int_value, 9, 100);
    CHECK(int_value >= 11 && int_value <= 15);

    CHECK(aisub_compareFVariables(3.0f, 0, 3.0f));
    CHECK(aisub_compareFVariables(3.0f, 1, 2.0f));
    CHECK(aisub_compareFVariables(2.0f, 2, 3.0f));
    CHECK(aisub_compareFVariables(2.0f, 3, 3.0f));
    CHECK(aisub_compareFVariables(3.0f, 4, 2.0f));
    CHECK(aisub_compareFVariables(2.0f, 5, 3.0f));
    CHECK(!aisub_compareFVariables(2.0f, 9, 2.0f));
    CHECK(aisub_compareSIVariables(3, 0, 3));
    CHECK(aisub_compareSIVariables(3, 1, 2));
    CHECK(aisub_compareSIVariables(2, 2, 3));
    CHECK(aisub_compareSIVariables(2, 3, 3));
    CHECK(aisub_compareSIVariables(3, 4, 2));
    CHECK(aisub_compareSIVariables(2, 5, 3));
    CHECK(!aisub_compareSIVariables(2, 9, 2));
    CHECK(!aisub_compareLogicSense(0));
    CHECK(aisub_compareLogicSense(1));
    CHECK(!aisub_compareLogicSense(2));
    CHECK(aisub_compareLogicSense(3));
    CHECK(aisub_compareLogicSense(4));
    CHECK(!aisub_compareLogicSense(5));
}

static void test_ai_flags_and_waypoint_selection(void)
{
    wsl_BAP_PLACEMENT placement;
    wsl_ENEMY enemy;

    memset(&placement, 0, sizeof(placement));
    memset(&enemy, 0, sizeof(enemy));
    enemy.pPlace = &placement;
    aisub_clearglobalflags();
    gGlobalTimer = 100;

    aisub_setFlag(1, 7);
    CHECK(aisub_checkFlag(1, 7));
    CHECK(_aiFlagsTimer[1] == 0);
    aisub_timedFlag(1, 9, 50);
    CHECK(_aiFlags[1] == 9);
    CHECK(_aiFlagsSave[1] == 7);
    CHECK(_aiFlagsTimer[1] == 150);
    aisub_timedFlag(1, 11, 10);
    CHECK(_aiFlags[1] == 9);
    CHECK(_aiFlagsSave[1] == 7);
    CHECK(_aiFlagsTimer[1] == 150);
    gGlobalTimer = 150;
    aisub_flagsManager();
    CHECK(_aiFlags[1] == 9);
    CHECK(_aiFlagsTimer[1] == 150);
    gGlobalTimer = 151;
    aisub_flagsManager();
    CHECK(_aiFlags[1] == 7);
    CHECK(_aiFlagsTimer[1] == 0);

    _aiFlags[0] = 12;
    _aiFlagsTimer[0] = 45;
    aisub_setFlag(-1, 99);
    aisub_setFlag(4, 99);
    CHECK(_aiFlags[0] == 12);
    CHECK(_aiFlagsTimer[0] == 45);

    enemy.lastWayPoint = 8;
    placement.nWaypnt = 1;
    aisub_setNextWaypoint(&enemy, 0);
    CHECK(enemy.lastWayPoint == 8);
    placement.nWaypnt = 4;
    aisub_setNextWaypoint(&enemy, 2);
    CHECK(enemy.lastWayPoint == 2);
    aisub_setNextWaypoint(&enemy, 3);
    CHECK(enemy.lastWayPoint == 0);
    aisub_setNextWaypoint(&enemy, -1);
    CHECK(enemy.lastWayPoint == -1);
}

static void test_enemy_post_frame_and_activation(void)
{
    EnemyFixture fixture;
    WorldData world;
    wsl_BAP_PLACEMENT first;
    wsl_BAP_PLACEMENT second;
    wsl_BAP_PLACEMENT *placements[2];

    memset(&GameStruct, 0, sizeof(GameStruct));
    init_enemy(&fixture, 5, 1, 111, -222, 333);
    fixture.player.playernum = 2;
    fixture.enemy.hitPoints = 37;
    bapenemy_postFrame(&fixture.enemy);
    CHECK(game_gGetEnergy(2) == 37);
    CHECK(fixture.enemy.location.vx == 111);
    CHECK(fixture.enemy.location.vy == -222);
    CHECK(fixture.enemy.location.vz == 333);
    fixture.scene.pPhysics = NULL;
    fixture.enemy.hitPoints = 22;
    fixture.enemy.location.vx = 7;
    fixture.enemy.location.vy = 8;
    fixture.enemy.location.vz = 9;
    bapenemy_postFrame(&fixture.enemy);
    CHECK(game_gGetEnergy(2) == 22);
    CHECK(fixture.enemy.location.vx == 7);
    CHECK(fixture.enemy.location.vy == 8);
    CHECK(fixture.enemy.location.vz == 9);
    fixture.enemy.pPlayer = NULL;
    bapenemy_postFrame(&fixture.enemy);
    bapenemy_postFrame(NULL);

    memset(&world, 0, sizeof(world));
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    placements[0] = &first;
    placements[1] = &second;
    world.apEnemy = placements;
    world.nEnemy = 2;
    gpWorld = &world;
    enemy_ActivateEnemy(0);
    CHECK(
        first.aiDf.activeFlags ==
        UINT32_C(0x10000000));
    second.aiDf.activeFlags = 1;
    enemy_ActivateEnemy(1);
    CHECK(second.aiDf.activeFlags == 1);
    world.apEnemy = NULL;
    enemy_ActivateEnemy(0);
}

static void test_active_enemy_frame_owner(void)
{
    EnemyFixture fixture;
    WorldData world;
    wsl_BAP_PLACEMENT placement;
    wsl_BAP_PLACEMENT *placements[1];
    animObject animation;
    playerObject inactive_player;
    uint16_t unsupported_opcode = UINT16_MAX;

    memset(&world, 0, sizeof(world));
    memset(&placement, 0, sizeof(placement));
    memset(&animation, 0, sizeof(animation));
    memset(&inactive_player, 0, sizeof(inactive_player));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(_aiFlagsTimer, 0, sizeof(_aiFlagsTimer));
    memset(_aiFlagsSave, 0, sizeof(_aiFlagsSave));
    memset(_aiFlags, 0, sizeof(_aiFlags));
    timesincetank[0] = 0;
    timesincetank[1] = 0;
    init_enemy(&fixture, 5, 4, 100, 200, 300);
    fixture.enemy.pPlace = &placement;
    fixture.enemy.active = 1;
    fixture.enemy.hitPoints = 40;
    fixture.player.playerID = 99;
    fixture.player.playernum = 5;
    fixture.scene.pAnim = &animation.animRoot;
    placement.status = 1;
    placement.pLastEnemy = 7;
    placement.aiDf.ownerType = 4;
    placement.aiDf.daRange = 64;
    placements[0] = &placement;
    world.location.vx = 100;
    world.location.vy = 200;
    world.location.vz = 300;
    world.player0 = &fixture.player;
    world.player1 = &inactive_player;
    world.nEnemy = 1;
    world.apEnemy = placements;
    gpWorld = &world;

    list_InitList(&enemyList[0]);
    list_InitList(&enemyList[1]);
    list_InitList(&enemyFreeList);
    list_AddTail(&enemyList[0], &fixture.enemy.node);
    mCurEnemyList = 0;
    nEnemy = 0;

    enemy_HandleEnemies();
    CHECK(
        jpb_enemy_LastFrameResult(
            &unsupported_opcode) ==
        JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(unsupported_opcode == 0);
    CHECK(mCurEnemyList == 1);
    CHECK(nEnemy == 1);
    CHECK(enemyList[0].head == NULL);
    CHECK(enemyList[1].head == &fixture.enemy.node);
    CHECK(enemyFreeList.head == NULL);
    CHECK(fixture.enemy.location.vx == 100);
    CHECK(fixture.enemy.location.vy == 200);
    CHECK(fixture.enemy.location.vz == 300);
    CHECK(game_gGetEnergy(5) == 40);

    fixture.enemy.exit_flag = 1;
    CHECK(
        jpb_enemy_ProcessActiveFrame(
            &unsupported_opcode) ==
        JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(mCurEnemyList == 0);
    CHECK(nEnemy == 1);
    CHECK(enemyList[0].head == NULL);
    CHECK(enemyList[1].head == NULL);
    CHECK(enemyFreeList.head == &fixture.enemy.node);
    CHECK(placement.status == 2);
    CHECK(placement.pLastEnemy == UINT32_MAX);
    CHECK(fixture.enemy.exit_flag == 0);

    gpWorld = NULL;
}

static void test_active_enemy_frame_globals(void)
{
    EnemyFixture debug_enemy;
    WorldData world;
    playerObject player0;
    playerObject player1;
    uint16_t unsupported_opcode = UINT16_MAX;
    int old_frame_rate = gGlobalFrameRate;
    uint32_t old_timer = gGlobalTimer;
    int old_time_adjustment = timeAdj;

    memset(&world, 0, sizeof(world));
    memset(&player0, 0, sizeof(player0));
    memset(&player1, 0, sizeof(player1));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(_aiFlagsTimer, 0, sizeof(_aiFlagsTimer));
    memset(_aiFlagsSave, 0, sizeof(_aiFlagsSave));
    memset(_aiFlags, 0, sizeof(_aiFlags));
    init_enemy(&debug_enemy, 0, 1, 0, 0, 0);

    world.player0 = &player0;
    world.player1 = &player1;
    gpWorld = &world;
    list_InitList(&enemyList[0]);
    list_InitList(&enemyList[1]);
    list_InitList(&enemyFreeList);
    mCurEnemyList = 0;
    GameStruct.CurrentLevel = 3;
    GameStruct.Counter = 9;
    OptionStruct.AIDebug = 4;
    GameStruct.aCharacterData[0].Score = 10;
    GameStruct.aCharacterData[1].Score = 20;
    abGlobalBits[0] = 3;
    _aiFlags[0] = 99;
    _aiFlagsSave[0] = 17;
    _aiFlagsTimer[0] = 150;
    _aiFlags[1] = 33;
    _aiFlagsSave[1] = 44;
    _aiFlagsTimer[1] = 250;
    gGlobalTimer = 200;
    gGlobalFrameRate = 10;
    timeAdj = 99;
    timesincetank[0] = 5;
    timesincetank[1] = 15;
    tankID = 7;
    nextLevel = 0;
    pLatestDebugEnemy = &debug_enemy.enemy;
    pDebugEnemy = NULL;

    CHECK(
        jpb_enemy_ProcessActiveFrame(
            &unsupported_opcode) ==
        JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(unsupported_opcode == 0);
    CHECK(timeAdj == 1);
    CHECK(timesincetank[0] == 0);
    CHECK(timesincetank[1] == 5);
    CHECK(_aiFlags[0] == 17);
    CHECK(_aiFlagsTimer[0] == 0);
    CHECK(_aiFlags[1] == 33);
    CHECK(_aiFlagsTimer[1] == 250);
    CHECK(pDebugEnemy == pLatestDebugEnemy);
    CHECK(GameStruct.Counter == 10);
    CHECK(game_gGetScore(0) == 260);
    CHECK(game_gGetScore(1) == 270);
    CHECK(abGlobalBits[0] == 0);
    CHECK(nextLevel == 1);
    CHECK(tankID == -1);
    CHECK(gShowAI == 1);
    CHECK(nEnemy == 0);
    CHECK(mCurEnemyList == 1);

    GameStruct.CurrentLevel = 13;
    GameStruct.NumPlayers = 1;
    abGlobalBits[6] = 0;
    CHECK(
        jpb_enemy_ProcessActiveFrame(NULL) ==
        JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK((abGlobalBits[6] & 2U) != 0);

    GameStruct.NumPlayers = 2;
    CHECK(
        jpb_enemy_ProcessActiveFrame(NULL) ==
        JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK((abGlobalBits[6] & 2U) == 0);

    gGlobalFrameRate = old_frame_rate;
    gGlobalTimer = old_timer;
    timeAdj = old_time_adjustment;
    pLatestDebugEnemy = NULL;
    pDebugEnemy = NULL;
    nextLevel = 0;
    gpWorld = NULL;
}

static void test_active_enemy_level_frame_overrides(void)
{
    EnemyFixture fixture;
    WorldData world;
    wsl_BAP_PLACEMENT placement;
    wsl_BAP_PLACEMENT *placements[1];
    animObject animation;
    playerObject inactive_player;

    memset(&world, 0, sizeof(world));
    memset(&placement, 0, sizeof(placement));
    memset(&animation, 0, sizeof(animation));
    memset(&inactive_player, 0, sizeof(inactive_player));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(_aiFlagsTimer, 0, sizeof(_aiFlagsTimer));
    memset(_aiFlagsSave, 0, sizeof(_aiFlagsSave));
    memset(_aiFlags, 0, sizeof(_aiFlags));
    timesincetank[0] = 0;
    timesincetank[1] = 0;
    init_enemy(&fixture, 5, 4, 0, 0, 0);
    fixture.physics.physicsRoot.pParent =
        &fixture.scene.sceneRoot;
    fixture.enemy.pPlace = &placement;
    fixture.enemy.active = 1;
    fixture.enemy.enemyID = 0x75;
    fixture.player.playerID = 99;
    fixture.scene.pAnim = &animation.animRoot;
    placement.status = 1;
    placement.aiDf.daRange = 20000;
    placements[0] = &placement;
    world.location.vz = 0;
    world.player0 = &fixture.player;
    world.player1 = &inactive_player;
    world.nEnemy = 1;
    world.apEnemy = placements;
    gpWorld = &world;

    list_InitList(&enemyList[0]);
    list_InitList(&enemyList[1]);
    list_InitList(&enemyFreeList);
    list_AddTail(&enemyList[0], &fixture.enemy.node);
    mCurEnemyList = 0;
    GameStruct.CurrentLevel = 6;

    CHECK(
        jpb_enemy_ProcessActiveFrame(NULL) ==
        JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(placement.aiDf.daRange == 10000);
    CHECK(enemyList[1].head == &fixture.enemy.node);

    fixture.enemy.enemyID = 0x3a;
    fixture.physics.pos.vx = 123.75f;
    fixture.physics.pos.vy = -456.25f;
    fixture.physics.pos.vz = -14000.0f;
    GameStruct.CurrentLevel = 7;
    CHECK(
        jpb_enemy_ProcessActiveFrame(NULL) ==
        JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(fixture.physics.pos.vx == 123.0f);
    CHECK(fixture.physics.pos.vy == -456.0f);
    CHECK(fixture.physics.pos.vz == -14800.0f);
    CHECK(fixture.physics.lastpos.vz == -14800.0f);

    fixture.physics.pos.vz = -15000.0f;
    fixture.physics.lastpos.vz = 321.0f;
    CHECK(
        jpb_enemy_ProcessActiveFrame(NULL) ==
        JPB_ENEMY_OPCODE_PARSE_COMPLETE);
    CHECK(fixture.physics.pos.vz == -15000.0f);
    CHECK(fixture.physics.lastpos.vz == 321.0f);

    gpWorld = NULL;
}

static void test_enemy_reset_enemies(void)
{
    EnemyFixture first;
    EnemyFixture second;
    WorldData world;
    wsl_BAP_PLACEMENT placements_data[3];
    wsl_BAP_PLACEMENT *placements[3];
    animObject animations[2];
    modelObject models[2];
    int index;

    memset(&world, 0, sizeof(world));
    memset(placements_data, 0, sizeof(placements_data));
    memset(animations, 0, sizeof(animations));
    memset(models, 0, sizeof(models));
    init_enemy(&first, 2, 4, 10, 20, 30);
    init_enemy(&second, 3, 4, 40, 50, 60);
    first.enemy.pPlace = &placements_data[0];
    second.enemy.pPlace = &placements_data[1];
    first.scene.pAnim = &animations[0].animRoot;
    second.scene.pAnim = &animations[1].animRoot;
    first.scene.pModel = &models[0].modelRoot;
    second.scene.pModel = &models[1].modelRoot;
    first.scene.pPlayer = &first.player.playerRoot;
    second.scene.pPlayer = &second.player.playerRoot;
    first.player.pEnemy = &first.enemy;
    second.player.pEnemy = &second.enemy;
    placements[0] = &placements_data[0];
    placements[1] = &placements_data[1];
    placements[2] = &placements_data[2];
    for (index = 0; index < 3; ++index) {
        placements_data[index].aiDf.activeFlags =
            UINT32_C(0x10000021);
        placements_data[index].status = index + 1;
        placements_data[index].pLastEnemy =
            (uint32_t)(index + 10);
    }
    world.player0 = &first.player;
    world.player1 = &second.player;
    world.nEnemy = 3;
    world.apEnemy = placements;
    gpWorld = &world;

    list_InitList(&enemyList[0]);
    list_InitList(&enemyList[1]);
    list_InitList(&enemyFreeList);
    list_AddTail(&enemyList[0], &first.enemy.node);
    list_AddTail(&enemyList[0], &second.enemy.node);
    mCurEnemyList = 0;
    memset(abGlobalBits, 0xff, sizeof(abGlobalBits));

    enemy_ResetEnemies();

    CHECK(enemyList[0].head == NULL);
    CHECK(enemyList[0].tail == NULL);
    CHECK(list_length(&enemyFreeList) == 2);
    CHECK(enemyFreeList.head == &first.enemy.node);
    CHECK(enemyFreeList.tail == &second.enemy.node);
    for (index = 0; index < 3; ++index) {
        CHECK(
            placements_data[index].aiDf.activeFlags ==
            UINT32_C(0x21));
        CHECK(placements_data[index].status == 0);
        CHECK(
            placements_data[index].pLastEnemy ==
            UINT32_MAX);
    }
    CHECK((abGlobalBits[0] & UINT8_C(1)) == 0);
    for (index = 1; index < 18; ++index) {
        CHECK(
            (abGlobalBits[index >> 3] &
             (UINT8_C(1) << (index & 7))) != 0);
    }
    for (index = 18; index < 128; ++index) {
        CHECK(
            (abGlobalBits[index >> 3] &
             (UINT8_C(1) << (index & 7))) == 0);
    }

    gpWorld = NULL;
}

static void test_enemy_set_teleport(void)
{
    VECTOR position = {10, 20, 30, 40};
    VECTOR offset = {-1, -2, -3, -4};

    savedPlayerPos.vx = 99;
    savedPlayerPos.vy = 98;
    savedPlayerPos.vz = 97;
    savedPlayerPos.pad = 96;
    tpos.pad = 95;
    toff.pad = 94;
    GameStruct.CurrentLevel = 8;
    enemy_SetTeleport(&position, &offset, 512, 77);
    CHECK(tpos.vx == 10);
    CHECK(tpos.vy == 20);
    CHECK(tpos.vz == 30);
    CHECK(tpos.pad == 95);
    CHECK(toff.vx == -1);
    CHECK(toff.vy == -2);
    CHECK(toff.vz == -3);
    CHECK(toff.pad == 94);
    CHECK(trange == 512);
    CHECK(tele == 77);
    CHECK(tflag == 1);
    CHECK(savedPlayerPos.vx == 99);
    CHECK(savedPlayerPos.vy == 98);
    CHECK(savedPlayerPos.vz == 97);
    CHECK(savedPlayerPos.pad == 96);

    GameStruct.CurrentLevel = 9;
    enemy_SetTeleport(&position, &offset, 1024, 88);
    CHECK(savedPlayerPos.vx == 10);
    CHECK(savedPlayerPos.vy == 20);
    CHECK(savedPlayerPos.vz == 30);
    CHECK(savedPlayerPos.pad == 96);
    CHECK(trange == 1024);
    CHECK(tele == 88);
}

static void test_enemy_find_nearest_waypoint(void)
{
    struct WaypointPlacement {
        wsl_BAP_PLACEMENT placement;
        wsl_BAP_WAYPOINT extra[2];
    } authored;
    EnemyFixture fixture;

    memset(&authored, 0, sizeof(authored));
    init_enemy(&fixture, 2, 1, 100, 0, 100);
    fixture.enemy.pPlace = &authored.placement;
    authored.placement.nWaypnt = 3;
    authored.placement.wayPoints[0].loc.vx = 8000;
    authored.placement.wayPoints[0].loc.vz = 100;
    authored.extra[0].loc.vx = 400;
    authored.extra[0].loc.vz = 100;
    authored.extra[1].loc.vx = 50;
    authored.extra[1].loc.vz = 100;

    CHECK(aisub_findNearestWaypnt(&fixture.enemy) == 2);
    authored.placement.nWaypnt = 1;
    CHECK(aisub_findNearestWaypnt(&fixture.enemy) == 0);
    authored.placement.wayPoints[0].loc.vx = 9000;
    CHECK(aisub_findNearestWaypnt(&fixture.enemy) == -1);
}

static void test_enemy_calc_points(void)
{
    WorldData world;
    wsl_BAP_PLACEMENT placements_data[3];
    wsl_BAP_PLACEMENT *placements[3];
    WorldData *old_world = gpWorld;
    int count = -1;
    int index;

    memset(&world, 0, sizeof(world));
    memset(placements_data, 0, sizeof(placements_data));
    memset(maModelID, 0, sizeof(maModelID));
    for (index = 0; index < 3; ++index) {
        placements[index] = &placements_data[index];
    }
    placements_data[0].actorNum = 0;
    placements_data[1].actorNum = 1;
    placements_data[2].actorNum = 0;
    maModelID[0][0] = 17;
    maModelID[1][0] = 16;
    world.nEnemy = 3;
    world.apEnemy = placements;
    world.nActor = 2;
    gpWorld = &world;

    CHECK(enemy_CalcPoints(&count) == 200);
    CHECK(count == 3);
    CHECK(maModelID[0][2] == 2);
    CHECK(maModelID[1][2] == 0);
    CHECK(enemy_CalcPoints(NULL) == 200);
    CHECK(maModelID[0][2] == 4);

    memset(maModelID, 0, sizeof(maModelID));
    gpWorld = old_world;
}

static void test_enemy_console_command_state(void)
{
    WorldData world;
    WorldData *old_world = gpWorld;
    wsl_BAP_PLACEMENT placement;
    wsl_BAP_PLACEMENT *placements[1];
    char *flag_set[] = {(char *)"FLAGS", (char *)"set", (char *)"9"};
    char *flag_clear[] = {(char *)"flags", (char *)"CLR", (char *)"9"};
    char *active[] = {
        (char *)"active",
        (char *)"0",
        (char *)"10",
        (char *)"20",
        (char *)"30"
    };
    int arguments[6] = {0};

    memset(&world, 0, sizeof(world));
    memset(&placement, 0, sizeof(placement));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    placements[0] = &placement;
    world.nEnemy = 1;
    world.apEnemy = placements;
    gpWorld = &world;

    arguments[2] = 9;
    CHECK(console_EnemyCommand(
              3, flag_set, arguments, NULL) == 0);
    CHECK((abGlobalBits[1] & UINT8_C(2)) != 0);
    CHECK(console_EnemyCommand(
              3, flag_clear, arguments, NULL) == 0);
    CHECK((abGlobalBits[1] & UINT8_C(2)) == 0);

    arguments[1] = 0;
    arguments[2] = 10;
    arguments[3] = 20;
    arguments[4] = 30;
    CHECK(console_EnemyCommand(
              5, active, arguments, NULL) == 0);
    CHECK(placement.loc.vx == 0x7600);
    CHECK(placement.loc.vy == 0x1400);
    CHECK(placement.loc.vz == -0x6100);
    CHECK(placement.status == 0);
    CHECK(
        (placement.aiDf.activeFlags &
         UINT32_C(0x10000000)) != 0);

    gpWorld = old_world;
}

static void test_enemy_check_teleport(void)
{
    EnemyFixture nearby;
    WorldData world;
    int32_t map_storage[6] = {0};
    WorldData *old_world = gpWorld;
    int32_t *old_leveldata = leveldata;
    Camera old_camera = gCamera;
    int old_camera_type = camera_GetCurrentCameraType();
    int old_new_camera_flag = newcameraflag;
    char old_level = LevelSelect;

    memset(&world, 0, sizeof(world));
    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    player_gInitPlayers(0);
    gaPlayerData[0].playerRoot.objectID = 0;
    gaPlayerData[0].playerRoot.pParent =
        &maSceneData[0].sceneRoot;
    gaPlayerData[1].playerRoot.objectID = 1;
    gaPlayerData[1].playerRoot.pParent =
        &maSceneData[1].sceneRoot;
    maSceneData[0].pScene =
        &maSceneData[0].sceneRoot;
    maSceneData[1].pScene =
        &maSceneData[1].sceneRoot;
    maSceneData[0].pPhysics =
        &maPhysicsData[0].physicsRoot;
    maSceneData[1].pPhysics =
        &maPhysicsData[1].physicsRoot;
    maSceneData[0].sceneRoot.flags = 0;
    maSceneData[1].sceneRoot.flags = UINT32_C(0x20);
    world.player0 = &gaPlayerData[0];
    world.player1 = &gaPlayerData[1];
    gpWorld = &world;
    leveldata = &map_storage[2];

    maPhysicsData[0].pos.vx = 100.0f;
    maPhysicsData[0].pos.vy = 200.0f;
    maPhysicsData[0].pos.vz = 300.0f;
    maPhysicsData[1].pos.vx = 400.0f;
    maPhysicsData[1].pos.vy = 500.0f;
    maPhysicsData[1].pos.vz = 600.0f;
    init_enemy(&nearby, 2, 2, 110, 210, 310);
    nearby.enemy.enemyID = 7;
    nearby.enemy.location.vx = 110;
    nearby.enemy.location.vy = 210;
    nearby.enemy.location.vz = 310;
    nearby.physics.pos.vx = 110.0f;
    nearby.physics.pos.vy = 210.0f;
    nearby.physics.pos.vz = 310.0f;
    list_InitList(&enemyList[0]);
    list_AddTail(&enemyList[0], &nearby.enemy.node);
    mCurEnemyList = 0;

    tpos.vx = 100;
    tpos.vy = 200;
    tpos.vz = 300;
    toff.vx = 10;
    toff.vy = -20;
    toff.vz = 30;
    trange = 64;
    tele = 99;
    tflag = 1;
    LevelSelect = 0;
    memset(&gCamera, 0, sizeof(gCamera));
    gCamera.viewType = UINT32_C(0x1800);
    gCamera.focus.vx = 1;
    gCamera.focus.vy = 2;
    gCamera.focus.vz = 3;
    newcameraflag = 0;
    camera_SetCurrentCameraType(1);

    enemy_CheckTeleport();

    CHECK(tflag == 0);
    CHECK(tele == -1);
    CHECK(nearby.physics.pos.vx == 120.0f);
    CHECK(nearby.physics.pos.vy == 190.0f);
    CHECK(nearby.physics.pos.vz == 340.0f);
    CHECK(nearby.enemy.location.vx == 120);
    CHECK(nearby.enemy.location.vy == 190);
    CHECK(nearby.enemy.location.vz == 340);
    CHECK(maPhysicsData[0].pos.vx == 110.0f);
    CHECK(maPhysicsData[0].pos.vy == 180.0f);
    CHECK(maPhysicsData[0].pos.vz == 330.0f);
    CHECK(maPhysicsData[1].pos.vx == 400.0f);
    CHECK(maPhysicsData[1].pos.vy == 500.0f);
    CHECK(maPhysicsData[1].pos.vz == 600.0f);
    CHECK(camera_GetCurrentCameraType() == 1);
    CHECK((gCamera.viewType & UINT32_C(0x1000)) == 0);
    CHECK(gCamera.focus.vx == gCamera.focusDest.vx);
    CHECK(gCamera.focus.vy == gCamera.focusDest.vy);
    CHECK(gCamera.focus.vz == gCamera.focusDest.vz);
    CHECK(gCamera.angle.vx == gCamera.angleDest.vx);
    CHECK(gCamera.angle.vy == gCamera.angleDest.vy);
    CHECK(gCamera.angle.vz == gCamera.angleDest.vz);

    list_InitList(&enemyList[0]);
    gpWorld = old_world;
    leveldata = old_leveldata;
    gCamera = old_camera;
    newcameraflag = old_new_camera_flag;
    camera_SetCurrentCameraType(old_camera_type);
    LevelSelect = old_level;
}

static void test_ai_player_target_selection(void)
{
    EnemyFixture seeker;
    EnemyFixture first;
    EnemyFixture second;
    WorldData world;
    playerObject *target;
    int result;

    memset(&world, 0, sizeof(world));
    init_enemy(&seeker, 0, 1, 0, 0, 0);
    init_enemy(&first, 1, 1, 300, 0, 0);
    init_enemy(&second, 2, 1, 100, 0, 0);
    seeker.player.playerRoot.objectID = 0;
    first.player.playerRoot.objectID = 1;
    second.player.playerRoot.objectID = 2;
    seeker.scene.pScene = &seeker.scene.sceneRoot;
    first.scene.pScene = &first.scene.sceneRoot;
    second.scene.pScene = &second.scene.sceneRoot;
    world.player0 = &first.player;
    world.player1 = &second.player;
    gpWorld = &world;
    maRange[0][1] = 300.0f;
    maRange[0][2] = 100.0f;

    target = NULL;
    result = ai_FindNearestPlayer(&seeker.player, &target);
    CHECK(result == 100);
    CHECK(target == &second.player);
    second.scene.sceneRoot.flags = 0x20;
    result = ai_FindNearestPlayer(&seeker.player, &target);
    CHECK(result == 300);
    CHECK(target == &first.player);
    second.scene.sceneRoot.flags = 0;
    first.scene.sceneRoot.flags = 0x20;
    result = ai_FindNearestPlayer(&seeker.player, &target);
    CHECK(result == 100);
    CHECK(target == &second.player);

    first.scene.sceneRoot.flags = 0;
    target = NULL;
    result = ai_FindFarPlayer(
        &seeker.player, &target, 150);
    CHECK(result == 300);
    CHECK(target == &first.player);
    target = NULL;
    result = ai_FindFarPlayer(
        &seeker.player, &target, 250);
    CHECK(result == 0);
    CHECK(target == NULL);
    first.player.pFlags = 0x200;
    result = ai_FindFarPlayer(
        &seeker.player, &target, 50);
    CHECK(result == 100);
    CHECK(target == &second.player);
}

static void test_ai_waypoint_bounds(void)
{
    uint8_t placement_storage[
        offsetof(wsl_BAP_PLACEMENT, wayPoints) +
        3 * sizeof(wsl_BAP_WAYPOINT)] = {0};
    wsl_BAP_PLACEMENT *placement =
        (wsl_BAP_PLACEMENT *)(void *)placement_storage;
    EnemyFixture fixture;

    init_enemy(&fixture, 3, 1, 0, 0, 0);
    fixture.player.pEnemy = &fixture.enemy;
    fixture.enemy.pPlace = placement;
    placement->nWaypnt = 1;
    placement->wayPoints[0].loc.vx = 0x500;
    CHECK(ai_CheckBounds(&fixture.player) == 1);
    placement->wayPoints[0].loc.vx = 0x400;
    placement->wayPoints[0].loc.vy = 0x2000;
    CHECK(ai_CheckBounds(&fixture.player) == 0);

    placement->nWaypnt = 3;
    fixture.enemy.lastWayPoint = 1;
    placement->wayPoints[1].loc.vx = 0x300;
    CHECK(ai_CheckBounds(&fixture.player) == 0);
    placement->wayPoints[1].loc.vx = 0x600;
    CHECK(ai_CheckBounds(&fixture.player) == 1);
    placement->nWaypnt = 0;
    CHECK(ai_CheckBounds(&fixture.player) == 1);
}

static void test_ai_bap_evaluators(void)
{
    uint8_t placement_storage[
        offsetof(wsl_BAP_PLACEMENT, wayPoints) +
        3 * sizeof(wsl_BAP_WAYPOINT)] = {0};
    wsl_BAP_PLACEMENT *placement =
        (wsl_BAP_PLACEMENT *)(void *)
            placement_storage;
    EnemyFixture fixture;
    UDATA target;
    UDATA vars[3];

    init_enemy(&fixture, 3, 1, 0, 0, 0);
    fixture.player.pEnemy = &fixture.enemy;
    fixture.enemy.pPlace = placement;
    placement->nWaypnt = 3;
    placement->wayPoints[0].loc.vx = 10;
    placement->wayPoints[1].loc.vx = 200;
    placement->wayPoints[2].loc.vx = 400;

    memset(&target, 0, sizeof(target));
    target.sw[0] = 0;
    fixture.enemy.lastWayPoint = 2;
    CHECK(aisub_handleMoveFunction(
              &fixture.enemy,
              target,
              0,
              20) == 1);
    CHECK(fixture.enemy.lastWayPoint == 1);

    target.sw[0] = 1;
    fixture.enemy.lastWayPoint = 0;
    CHECK(aisub_handleMoveFunction(
              &fixture.enemy,
              target,
              0,
              20) == 1);
    CHECK(fixture.enemy.lastWayPoint == 1);

    memset(vars, 0, sizeof(vars));
    vars[0].si = 0;
    vars[1].si = 2;
    vars[2].f = 0.05f;
    CHECK(aisub_handleRangeFunction(
              &fixture.enemy, vars) == 1);
    vars[2].f = 0.03f;
    CHECK(aisub_handleRangeFunction(
              &fixture.enemy, vars) == 0);

    vars[1].si = 6;
    placement->aiDf.rangeExt[0] = 9;
    CHECK(aisub_handleRangeFunction(
              &fixture.enemy, vars) == 0);
    placement->aiDf.rangeExt[0] = 10;
    CHECK(aisub_handleRangeFunction(
              &fixture.enemy, vars) == 1);

    fixture.physics.face.vy = 100;
    CHECK(aisub_handleScanFunction(
              &fixture.enemy, 0, 25, 0) == 1);
    CHECK(fixture.physics.face.vy == 125);
    CHECK(aisub_handleScanFunction(
              &fixture.enemy, 0, 99, 700) == 1);
    CHECK(fixture.physics.face.vy == 700);
    CHECK(aisub_handleScanFunction(
              &fixture.enemy, 2, 0, 0) == 0);
}

static void test_physics_nearest_enemy(void)
{
    EnemyFixture seeker;
    sceneObject candidate_scene;
    playerObject candidate_player;
    wsl_ENEMY candidate_enemy;
    wsl_BAP_PLACEMENT candidate_placement;
    sceneObject machinery_scene;
    sceneObject non_enemy_scene;
    playerObject non_enemy_player;
    int index;

    init_enemy(&seeker, 0, 1, 0, 0, 0);
    seeker.physics.physicsRoot.pParent =
        &seeker.scene.sceneRoot;
    memset(&candidate_scene, 0, sizeof(candidate_scene));
    memset(&candidate_player, 0, sizeof(candidate_player));
    memset(&candidate_enemy, 0, sizeof(candidate_enemy));
    memset(&candidate_placement, 0, sizeof(candidate_placement));
    memset(&machinery_scene, 0, sizeof(machinery_scene));
    memset(&non_enemy_scene, 0, sizeof(non_enemy_scene));
    memset(&non_enemy_player, 0, sizeof(non_enemy_player));
    for (index = 0;
         index < JPB_PHYSICS_CAPACITY;
         ++index) {
        memset(
            &maPhysicsData[index],
            0,
            sizeof(maPhysicsData[index]));
        maPhysicsData[index]
            .physicsRoot.objectID = -1;
    }

    maPhysicsData[2].physicsRoot.objectID = 2;
    maPhysicsData[2].physicsRoot.pParent =
        &candidate_scene.sceneRoot;
    candidate_scene.pPlayer =
        &candidate_player.playerRoot;
    candidate_player.pEnemy =
        &candidate_enemy;
    candidate_enemy.pPlace =
        &candidate_placement;
    candidate_placement.aiDf.ownerType = 2;
    maRange[0][2] = 123.0f;
    maRange[2][0] = 123.0f;

    /* Occupied physics slots without an enemy-placement chain are authored
     * level machinery or ordinary actors, not range candidates. */
    maPhysicsData[3].physicsRoot.objectID = 3;
    maPhysicsData[3].physicsRoot.pParent =
        &machinery_scene.sceneRoot;
    maPhysicsData[4].physicsRoot.objectID = 4;
    maPhysicsData[4].physicsRoot.pParent =
        &non_enemy_scene.sceneRoot;
    non_enemy_scene.pPlayer =
        &non_enemy_player.playerRoot;

    CHECK(physics_FindNearestEnemy(
              &seeker.physics.physicsRoot,
              2) == 123);
    CHECK(physics_FindNearestEnemy(
              &seeker.physics.physicsRoot,
              3) == 0x1fffe);
}

static void test_enemy_radar(void)
{
    WorldData world;
    EnemyFixture enemies[2];
    wsl_BAP_PLACEMENT placements[2];
    _Material material;
    RadarTrace trace;
    _Material *old_trans_handle = transHandle;
    WorldData *old_world = gpWorld;
    uint32_t old_width = OptionStruct.ScreenWidth;
    uint32_t old_height = OptionStruct.ScreenHeight;
    int old_level = (int)(int8_t)LevelSelect;
    Camera old_camera = gCamera;

    memset(&world, 0, sizeof(world));
    memset(placements, 0, sizeof(placements));
    memset(&material, 0, sizeof(material));
    memset(&trace, 0, sizeof(trace));
    init_enemy(&enemies[0], 2, 2, 100, 200, 300);
    init_enemy(&enemies[1], 3, 3, 100, 200, 300);
    placements[0].aiDf.ownerType = 2;
    placements[1].aiDf.ownerType = 3;
    enemies[0].enemy.pPlace = &placements[0];
    enemies[1].enemy.pPlace = &placements[1];
    enemies[0].enemy.active = 1;
    enemies[1].enemy.active = 1;
    list_InitList(&enemyList[0]);
    list_AddTail(&enemyList[0], &enemies[0].enemy.node);
    list_AddTail(&enemyList[0], &enemies[1].enemy.node);
    mCurEnemyList = 0;
    world.location.vx = 100;
    world.location.vy = 200;
    world.location.vz = 300;
    gpWorld = &world;
    LevelSelect = 1;
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    memset(&gCamera, 0, sizeof(gCamera));
    transHandle = &material;
    jpb_WHookSetDrawTextureHook(
        capture_radar_draw, &trace);

    enemy_Radar();
    CHECK(trace.count == 4);
    CHECK(trace.draws[0].texture == &material);
    CHECK(trace.draws[0].has_source == 0);
    CHECK(trace.draws[0].destination.left == 284);
    CHECK(trace.draws[0].destination.top == 31);
    CHECK(trace.draws[0].destination.right == 355);
    CHECK(trace.draws[0].destination.bottom == 137);
    CHECK(trace.draws[0].color.r == 0);
    CHECK(trace.draws[0].color.g == 0);
    CHECK(trace.draws[0].color.b == 0);
    CHECK(trace.draws[0].color.cd == UINT8_C(0x9f));
    CHECK(trace.draws[0].layer_depth == 0.0001f);
    CHECK(trace.draws[1].destination.left == 318);
    CHECK(trace.draws[1].destination.top == 82);
    CHECK(trace.draws[1].destination.right == 320);
    CHECK(trace.draws[1].destination.bottom == 85);
    CHECK(trace.draws[1].texture == &material);
    CHECK(trace.draws[1].has_source == 0);
    CHECK(trace.draws[1].color.r == UINT8_C(0xff));
    CHECK(trace.draws[1].color.g == UINT8_C(0xff));
    CHECK(trace.draws[1].color.b == UINT8_C(0xff));
    CHECK(trace.draws[1].color.cd == UINT8_C(0xff));
    CHECK(trace.draws[1].layer_depth == 0.0001f);
    CHECK(trace.draws[2].destination.left == 318);
    CHECK(trace.draws[2].destination.top == 82);
    CHECK(trace.draws[2].destination.right == 320);
    CHECK(trace.draws[2].destination.bottom == 85);
    CHECK(trace.draws[2].texture == &material);
    CHECK(trace.draws[2].has_source == 0);
    CHECK(trace.draws[2].color.r == UINT8_C(0xff));
    CHECK(trace.draws[2].color.g == UINT8_C(0x20));
    CHECK(trace.draws[2].color.b == UINT8_C(0x20));
    CHECK(trace.draws[2].color.cd == UINT8_C(0xff));
    CHECK(trace.draws[2].layer_depth == 0.0001f);
    CHECK(trace.draws[3].destination.left == 318);
    CHECK(trace.draws[3].destination.top == 82);
    CHECK(trace.draws[3].destination.right == 320);
    CHECK(trace.draws[3].destination.bottom == 85);
    CHECK(trace.draws[3].texture == &material);
    CHECK(trace.draws[3].has_source == 0);
    CHECK(trace.draws[3].color.r == UINT8_C(0x20));
    CHECK(trace.draws[3].color.g == UINT8_C(0xff));
    CHECK(trace.draws[3].color.b == UINT8_C(0x20));
    CHECK(trace.draws[3].color.cd == UINT8_C(0xff));
    CHECK(trace.draws[3].layer_depth == 0.0001f);

    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    enemy_Radar();
    CHECK(trace.count == 4);
    CHECK(trace.draws[0].destination.left == 426);
    CHECK(trace.draws[0].destination.top == 34);
    CHECK(trace.draws[0].destination.right == 532);
    CHECK(trace.draws[0].destination.bottom == 154);
    CHECK(trace.draws[0].color.cd == UINT8_C(0x9f));
    CHECK(trace.draws[1].destination.left == 478);
    CHECK(trace.draws[1].destination.top == 93);
    CHECK(trace.draws[1].destination.right == 480);
    CHECK(trace.draws[1].destination.bottom == 95);
    CHECK(trace.draws[1].color.r == UINT8_C(0xff));
    CHECK(trace.draws[1].color.g == UINT8_C(0xff));
    CHECK(trace.draws[1].color.b == UINT8_C(0xff));
    CHECK(trace.draws[1].color.cd == UINT8_C(0xff));
    CHECK(trace.draws[2].destination.left == 478);
    CHECK(trace.draws[2].destination.top == 93);
    CHECK(trace.draws[2].destination.right == 480);
    CHECK(trace.draws[2].destination.bottom == 95);
    CHECK(trace.draws[2].color.r == UINT8_C(0xff));
    CHECK(trace.draws[2].color.g == UINT8_C(0x20));
    CHECK(trace.draws[2].color.b == UINT8_C(0x20));
    CHECK(trace.draws[2].color.cd == UINT8_C(0xff));
    CHECK(trace.draws[3].destination.left == 478);
    CHECK(trace.draws[3].destination.top == 93);
    CHECK(trace.draws[3].destination.right == 480);
    CHECK(trace.draws[3].destination.bottom == 95);
    CHECK(trace.draws[3].color.r == UINT8_C(0x20));
    CHECK(trace.draws[3].color.g == UINT8_C(0xff));
    CHECK(trace.draws[3].color.b == UINT8_C(0x20));
    CHECK(trace.draws[3].color.cd == UINT8_C(0xff));

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    transHandle = old_trans_handle;
    gpWorld = old_world;
    OptionStruct.ScreenWidth = old_width;
    OptionStruct.ScreenHeight = old_height;
    LevelSelect = (char)old_level;
    gCamera = old_camera;
    list_InitList(&enemyList[0]);
}

int main(void)
{
    EnemyFixture near_enemy;
    EnemyFixture far_enemy;
    EnemyFixture player_owned;
    VECTOR center = {100, 200, 300, 0};

    test_enemy_pool_initialization();
    test_init_enemy();
    test_add_enemy_orchestration();
    test_check_for_new_enemies();
    test_enemy_map_triggers();
    test_enemy_pointer_index();
    CHECK(test_ai_data_access() == 0);
    test_ai_node_traversal_and_mode_stack();
    test_authored_opcode_traversal_boundary();
    test_authored_opcode_cycle_enters_at_root_child();
    test_ai_script_arithmetic_and_comparisons();
    test_ai_flags_and_waypoint_selection();
    test_enemy_post_frame_and_activation();
    test_active_enemy_frame_owner();
    test_active_enemy_frame_globals();
    test_active_enemy_level_frame_overrides();
    test_enemy_reset_enemies();
    test_enemy_set_teleport();
    test_enemy_find_nearest_waypoint();
    test_enemy_calc_points();
    test_enemy_console_command_state();
    test_enemy_check_teleport();
    test_ai_player_target_selection();
    test_ai_waypoint_bounds();
    test_ai_bap_evaluators();
    test_physics_nearest_enemy();
    test_enemy_radar();
    CHECK(test_ai_defend_and_preframe() == 0);

    init_enemy(
        &near_enemy, 2, 1, 90, 210, 290);
    init_enemy(
        &far_enemy, 3, 1, 90, 210, 351);
    init_enemy(
        &player_owned, 4, 3, 100, 200, 300);
    near_enemy.enemy.node.next =
        &far_enemy.enemy.node;
    far_enemy.enemy.node.next =
        &player_owned.enemy.node;
    list_InitList(&enemyList[0]);
    enemyList[0].head = &near_enemy.enemy.node;
    enemyList[0].tail = &player_owned.enemy.node;
    mCurEnemyList = 0;

    enemy_KillKill(&center, 50);
    CHECK(near_enemy.enemy.exit_flag == 1);
    CHECK(far_enemy.enemy.exit_flag == 0);
    CHECK(player_owned.enemy.exit_flag == 0);

    if (failures != 0) {
        fprintf(
            stderr,
            "%d enemy test(s) failed\n",
            failures);
        return 1;
    }
    puts("enemy tests passed");
    return 0;
}
