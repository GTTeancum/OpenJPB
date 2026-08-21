#include "jpb/alloc.h"
#include "jpb/game.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/pwrup.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
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

static void put_u16(uint8_t *destination, uint16_t value)
{
    memcpy(destination, &value, sizeof(value));
}

static void put_record(
    uint8_t *destination,
    uint32_t legacy_node,
    int16_t x,
    int16_t y,
    int16_t z,
    uint16_t type_bits)
{
    memcpy(destination, &legacy_node, sizeof(legacy_node));
    put_u16(destination + 4, (uint16_t)x);
    put_u16(destination + 6, (uint16_t)y);
    put_u16(destination + 8, (uint16_t)z);
    put_u16(destination + 10, type_bits);
}

static size_t list_count(const List *list)
{
    const Node *node = list->head;
    size_t count = 0;

    while (node != NULL) {
        ++count;
        node = node->next;
    }
    return count;
}

typedef struct PowerupDrawObservation {
    int count;
    unsigned type;
    _svector position;
    _svector rotation;
    VECTOR scale;
    _svector offset;
} PowerupDrawObservation;

static void observe_powerup_draw(
    void *user_data,
    _svector *position,
    unsigned type,
    _svector *rotation,
    VECTOR *scale,
    _svector *offset)
{
    PowerupDrawObservation *observation =
        (PowerupDrawObservation *)user_data;

    ++observation->count;
    observation->type = type;
    observation->position = *position;
    observation->rotation = *rotation;
    observation->scale = *scale;
    observation->offset = *offset;
}

static void setup_dispatcher_world(
    WorldData *world,
    int x,
    int y,
    int z)
{
    memset(world, 0, sizeof(*world));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(gaPlayerData, 0, sizeof(gaPlayerData));
    memset(maPhysicsData, 0, sizeof(maPhysicsData));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(reStartPos, 0, sizeof(reStartPos));
    memset(reStartScore, 0, sizeof(reStartScore));
    afterLife = NULL;
    maxCheckPoints = 0;
    usedCheckPoints = 0;
    gCheckPoint = 0;
    reStartCounter = 0;
    LevelSelect = 0;
    initialLevelPauseDelay = 0;
    mDrawingSurfaceId = 0;
    gGlobalTimer = 0;
    world->player0 = &gaPlayerData[0];
    world->player1 = &gaPlayerData[1];
    world->location.vx = x;
    world->location.vy = y;
    world->location.vz = z;
    world->p0location = world->location;
    world->p1location.vx = 100000;
    world->p1location.vy = 100000;
    world->p1location.vz = 100000;
    gpWorld = world;
    gaPlayerData[0].playerRoot.objectID = 0;
    gaPlayerData[0].playernum = 0;
    gaPlayerData[0].playerID = 0;
    gaPlayerData[1].playerRoot.objectID = 1;
    gaPlayerData[1].playernum = 1;
    gaPlayerData[1].playerID = 1;
    maPhysicsData[0].vpos.vx = x;
    maPhysicsData[0].vpos.vy = y;
    maPhysicsData[0].vpos.vz = z;
    maPhysicsData[1].vpos.vx = 100000;
    maPhysicsData[1].vpos.vy = 100000;
    maPhysicsData[1].vpos.vz = 100000;
    GameStruct.NumPlayers = 1;
    GameStruct.aCharacterData[0].MaxEnergy = 200;
    GameStruct.aCharacterData[0].Energy = 25;
    GameStruct.aCharacterData[0].MaxForce = 200;
    GameStruct.aCharacterData[0].Force = 30;
    GameStruct.aCharacterData[1].MaxEnergy = 200;
    GameStruct.aCharacterData[1].MaxForce = 200;
    meminit();
    sprite_gInitSprites();
}

static int test_stream_and_checkpoint_lifecycle(void)
{
    uint8_t data[3 * JPB_POWERUP_DISK_RECORD_SIZE];

    put_record(data, UINT32_C(0x11111111), 10, 20, 30, UINT16_C(0x8005));
    put_record(data + 12, UINT32_C(0x22222222), -40, 50, -60, 2);
    put_record(data + 24, UINT32_C(0x33333333), 70, 80, 90, 5);

    CHECK(jpb_PwrupLoadData(data, sizeof(data)) == 1);
    CHECK(jpb_PwrupLoadedCount() == 3);
    CHECK(list_count(&poopList[0]) == 3);
    CHECK(list_IsListEmpty(&poopList[1]) == 1);
    CHECK(poopArray[0].pos.vx == 10);
    CHECK(poopArray[1].pos.vx == -40);
    CHECK(poopArray[1].pos.vz == -60);
    CHECK(poopArray[2].pos.pad == 5);

    mDrawingSurfaceId = 0;
    memset(aCheckPoints, 0, sizeof(aCheckPoints));
    pwrup_Init();
    CHECK(maxCheckPoints == 3);
    CHECK(poopArray[0].pos.pad == 5);
    CHECK(aCheckPoints[1].vx == 10);
    CHECK(aCheckPoints[1].vy == 20);
    CHECK(aCheckPoints[1].vz == 30);
    CHECK(aCheckPoints[2].vx == 70);
    CHECK(aCheckPoints[2].vy == 80);
    CHECK(aCheckPoints[2].vz == 90);

    poopArray[2].pos.pad = (int16_t)UINT16_C(0x8005);
    usedCheckPoints = 17;
    pwrup_LevelStart();
    CHECK(maxCheckPoints == 2);
    CHECK(usedCheckPoints == 0);
    CHECK(poopArray[2].pos.pad == 5);

    list_MoveList(&poopList[1], &poopList[0]);
    CHECK(list_IsListEmpty(&poopList[0]) == 1);
    pwrup_Init();
    CHECK(maxCheckPoints == 3);
    list_MoveList(&poopList[0], &poopList[1]);
    return 0;
}

static int test_level_end_scores(void)
{
    memset(&GameStruct, 0, sizeof(GameStruct));
    maxCheckPoints = 4;
    usedCheckPoints = 2;
    GameStruct.NumPlayers = 1;
    pwrup_LevelEnd();
    CHECK(GameStruct.aCharacterData[0].Score == 300);
    CHECK(GameStruct.aCharacterData[1].Score == 0);

    memset(GameStruct.aCharacterData, 0, sizeof(GameStruct.aCharacterData));
    GameStruct.NumPlayers = 2;
    pwrup_LevelEnd();
    CHECK(GameStruct.aCharacterData[0].Score == 300);
    CHECK(GameStruct.aCharacterData[1].Score == 300);
    return 0;
}

static int test_checkpoint_jump(void)
{
    WorldData world;
    playerObject players[2];
    sceneObject scenes[2];
    physicsObject physics[2];

    memset(&world, 0, sizeof(world));
    memset(players, 0, sizeof(players));
    memset(scenes, 0, sizeof(scenes));
    memset(physics, 0, sizeof(physics));
    memset(&GameStruct, 0, sizeof(GameStruct));
    scenes[0].pPhysics = &physics[0].physicsRoot;
    scenes[1].pPhysics = &physics[1].physicsRoot;
    players[0].playerRoot.pParent = &scenes[0].sceneRoot;
    players[1].playerRoot.pParent = &scenes[1].sceneRoot;
    physics[0].physicsRoot.objectID = 0;
    physics[1].physicsRoot.objectID = 1;
    world.player0 = &players[0];
    world.player1 = &players[1];
    gpWorld = &world;

    maxCheckPoints = 3;
    aCheckPoints[1].vx = 101;
    aCheckPoints[1].vy = -202;
    aCheckPoints[1].vz = 303;
    aCheckPoints[2].vx = 404;
    aCheckPoints[2].vy = 505;
    aCheckPoints[2].vz = -606;
    GameStruct.CurrentLevel = 2;
    GameStruct.checkpoint[2] = 1;
    GameStruct.NumPlayers = 2;
    CHECK(pwrup_JumpCheckPoint() == 1);
    CHECK(physics[0].pos.vx == 101.0f);
    CHECK(physics[0].pos.vy == -202.0f);
    CHECK(physics[0].pos.vz == 303.0f);
    CHECK(physics[1].pos.vx == 101.0f);
    CHECK(physics[1].pos.vy == -202.0f);
    CHECK(physics[1].pos.vz == 303.0f);

    GameStruct.checkpoint[2] = 99;
    GameStruct.NumPlayers = 1;
    CHECK(pwrup_JumpCheckPoint() == 1);
    CHECK(physics[0].pos.vx == 404.0f);
    CHECK(physics[0].pos.vy == 505.0f);
    CHECK(physics[0].pos.vz == -606.0f);
    CHECK(physics[1].pos.vx == 101.0f);

    GameStruct.checkpoint[2] = 0;
    CHECK(pwrup_JumpCheckPoint() == 0);
    GameStruct.CurrentLevel = JPB_GAME_CHECKPOINT_CAPACITY;
    CHECK(pwrup_JumpCheckPoint() == 0);
    gpWorld = NULL;
    return 0;
}

static int test_rejections(void)
{
    uint8_t oversized[
        (JPB_POWERUP_CAPACITY + 1) * JPB_POWERUP_DISK_RECORD_SIZE] = {0};

    CHECK(jpb_PwrupLoadData(NULL, 1) == 0);
    CHECK(jpb_PwrupLoadData(oversized, 11) == 0);
    CHECK(jpb_PwrupLoadData(oversized, sizeof(oversized)) == 0);
    return 0;
}

static int test_initialized_tables_and_leaf_functions(void)
{
    CHECK(strcmp(powerUpNames[0], "HEAL") == 0);
    CHECK(strcmp(powerUpNames[5], "CHECK POINT") == 0);
    CHECK(strcmp(powerUpFiles[14], "g_art") == 0);
    CHECK(powerUpScales[4] == 2048);
    CHECK(powerUpScales[5] == 4096);
    CHECK(mRandomPower[0] == 15);
    CHECK(mRandomPower[8] == 15);
    CHECK(pwrIcons[3].b == 0xc0);
    CHECK(powColorLimit == 0x40);
    CHECK(fixPowColor(UINT32_C(0x001020f0)) ==
          UINT32_C(0xff4040ff));
    CHECK(kmAudioSFX_DumpBank(3) == -1);
    return 0;
}

static int test_dispatcher_draw_collection_and_publication(void)
{
    uint8_t data[JPB_POWERUP_DISK_RECORD_SIZE];
    WorldData world;
    PowerupDrawObservation observation = {0};

    setup_dispatcher_world(&world, 100, 200, 300);
    put_record(data, 0, 100, 200, 300, 0);
    CHECK(jpb_PwrupLoadData(data, sizeof(data)) == 1);
    jpb_PwrupSetDrawHook(observe_powerup_draw, &observation);
    pwrup_CheckPowerUps();
    CHECK(observation.count == 1);
    CHECK(observation.type == 0);
    CHECK(observation.position.vx == 100);
    CHECK(observation.offset.vy == 100);
    CHECK(observation.scale.vx == 4096);
    CHECK(GameStruct.aCharacterData[0].Energy == 75);
    CHECK(GameStruct.aCharacterData[0].Score == 50);
    CHECK(((uint16_t)poopArray[0].pos.pad &
           JPB_POWERUP_COLLECTED_FLAG) != 0);
    CHECK(list_IsListEmpty(&poopList[0]) == 1);
    CHECK(list_count(&poopList[1]) == 1);

    mDrawingSurfaceId = 1;
    pwrup_CheckPowerUps();
    CHECK(observation.count == 1);
    CHECK(GameStruct.aCharacterData[0].Energy == 75);
    CHECK(GameStruct.aCharacterData[0].Score == 50);
    CHECK(list_IsListEmpty(&poopList[1]) == 1);
    CHECK(list_count(&poopList[0]) == 1);
    jpb_PwrupSetDrawHook(NULL, NULL);
    jpb_PwrupReleaseData();
    gpWorld = NULL;
    return 0;
}

static int test_combo_level_and_afterlife_marker_noncollection(void)
{
    uint8_t data[2 * JPB_POWERUP_DISK_RECORD_SIZE];
    WorldData world;
    PowerupDrawObservation observation = {0};

    setup_dispatcher_world(&world, -50, 60, 70);
    put_record(data, 0, -50, 60, 70, 2);
    put_record(data + 12, 0, -50, 60, 70, 6);
    CHECK(jpb_PwrupLoadData(data, sizeof(data)) == 1);
    jpb_PwrupSetDrawHook(observe_powerup_draw, &observation);
    initialLevelPauseDelay = 2;
    GameStruct.ComboLevel = 1;
    pwrup_CheckPowerUps();
    CHECK(gPoopMode == 1);
    CHECK(observation.count == 1);
    CHECK((uint16_t)poopArray[0].pos.pad == 2);
    CHECK((uint16_t)poopArray[1].pos.pad == 6);
    CHECK(GameStruct.aCharacterData[0].Force == 30);

    GameStruct.ComboLevel = 0;
    mDrawingSurfaceId = 1;
    pwrup_CheckPowerUps();
    CHECK(gPoopMode == 0);
    CHECK(GameStruct.aCharacterData[0].Force == 80);
    CHECK(((uint16_t)poopArray[0].pos.pad &
           JPB_POWERUP_COLLECTED_FLAG) != 0);
    CHECK((uint16_t)poopArray[1].pos.pad == 6);
    jpb_PwrupSetDrawHook(NULL, NULL);
    jpb_PwrupReleaseData();
    gpWorld = NULL;
    return 0;
}

static int test_dispatcher_checkpoint_and_artifact(void)
{
    uint8_t data[2 * JPB_POWERUP_DISK_RECORD_SIZE];
    WorldData world;

    setup_dispatcher_world(&world, 400, 500, -600);
    put_record(data, 0, 400, 500, -600, 5);
    put_record(data + 12, 0, 400, 500, -600, 14);
    CHECK(jpb_PwrupLoadData(data, sizeof(data)) == 1);
    pwrup_Init();
    GameStruct.CurrentLevel = 5;
    GameStruct.Counter = 77;
    GameStruct.aCharacterData[0].Score = 10;
    GameStruct.aCharacterData[1].Score = 20;
    LevelSelect = 5;
    pwrup_CheckPowerUps();
    CHECK(gCheckPoint == 1);
    CHECK(reStartPos[0].vx == 400);
    CHECK(reStartPos[0].vy == 500);
    CHECK(reStartPos[0].vz == -600);
    CHECK(reStartScore[0] == 10);
    CHECK(reStartScore[1] == 20);
    CHECK(reStartCounter == 77);
    CHECK(usedCheckPoints == 1);
    CHECK(GameStruct.checkpoint[5] == 1);
    CHECK(GameStruct.aCharacterData[0].Score == 110);
    CHECK(GameStruct.aCharacterData[1].Score == 70);
    CHECK((abGlobalBits[1] & UINT8_C(0x40)) != 0);
    jpb_PwrupReleaseData();
    gpWorld = NULL;
    return 0;
}

static int test_asset_file(const char *path)
{
    FILE *file;
    long length;
    uint8_t *data;
    size_t read_size;

    file = fopen(path, "rb");
    CHECK(file != NULL);
    CHECK(fseek(file, 0, SEEK_END) == 0);
    length = ftell(file);
    CHECK(length > 0);
    CHECK(fseek(file, 0, SEEK_SET) == 0);
    data = (uint8_t *)malloc((size_t)length);
    CHECK(data != NULL);
    read_size = fread(data, 1, (size_t)length, file);
    CHECK(fclose(file) == 0);
    CHECK(read_size == (size_t)length);
    CHECK(jpb_PwrupLoadData(data, read_size) == 1);
    CHECK(jpb_PwrupLoadedCount() ==
          read_size / JPB_POWERUP_DISK_RECORD_SIZE);
    CHECK(list_count(&poopList[0]) == jpb_PwrupLoadedCount());
    CHECK(poopList[0].tail ==
          (Node *)&poopArray[jpb_PwrupLoadedCount() - 1]);
    free(data);
    jpb_PwrupReleaseData();
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "--file") == 0) {
        return test_asset_file(argv[2]);
    }
    CHECK(argc == 1);
    CHECK(test_stream_and_checkpoint_lifecycle() == 0);
    CHECK(test_level_end_scores() == 0);
    CHECK(test_checkpoint_jump() == 0);
    CHECK(test_rejections() == 0);
    CHECK(test_initialized_tables_and_leaf_functions() == 0);
    CHECK(test_dispatcher_draw_collection_and_publication() == 0);
    CHECK(test_combo_level_and_afterlife_marker_noncollection() == 0);
    CHECK(test_dispatcher_checkpoint_and_artifact() == 0);
    jpb_PwrupReleaseData();
    puts("power-up tests passed");
    return 0;
}
