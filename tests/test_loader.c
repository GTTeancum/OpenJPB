#include "jpb/bmd.h"
#include "jpb/brain.h"
#include "jpb/game.h"
#include "jpb/loader.h"
#include "jpb/memory.h"
#include "jpb/player.h"
#include "jpb/resources.h"

#include <direct.h>
#include <io.h>
#include <stdint.h>
#include <stdlib.h>
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

static int test_player_character_boundary(void)
{
    playerObject player;

    memset(&player, 0, sizeof(player));
    player.playernum = -1;
    CHECK(IsPlayerCharacter(&player) == 1);
    player.playernum = 1;
    CHECK(IsPlayerCharacter(&player) == 1);
    player.playernum = 2;
    CHECK(IsPlayerCharacter(&player) == 0);
    return 0;
}

static int test_node_name_search(void)
{
    geomData nodes[3];

    memset(nodes, 0, sizeof(nodes));
    memcpy(nodes[0].name, "root", sizeof("root"));
    memcpy(nodes[1].name, "obi_r_hand", sizeof("obi_r_hand"));
    memcpy(nodes[2].name, "qui_head", sizeof("qui_head"));

    CHECK(getNodeByName(nodes, (long)sizeof(nodes), "r_hand") == &nodes[1]);
    CHECK(getNodeByName(
              nodes,
              (long)(sizeof(nodes[0]) * 3 - 1),
              "qui_head") == NULL);
    CHECK(getNodeByName(nodes, (long)sizeof(nodes), "qui_head") == &nodes[2]);
    CHECK(getNodeByName(nodes, (long)sizeof(nodes), "missing") == NULL);
    return 0;
}

static int test_data_array_initialization(void)
{
    size_t i;

    for (i = 0; i < JPB_ANIMATION_NAME_COUNT; ++i) {
        maAnimData[i] = (char *)(uintptr_t)(i + 1);
    }
    for (i = 0; i < JPB_MODEL_NAME_COUNT; ++i) {
        maModelData[i] = (char *)(uintptr_t)(i + 1);
    }

    initDataArrays();

    for (i = 0; i < JPB_ANIMATION_NAME_COUNT; ++i) {
        CHECK(maAnimData[i] == NULL);
    }
    for (i = 0; i < JPB_MODEL_NAME_COUNT; ++i) {
        CHECK(maModelData[i] == NULL);
    }
    return 0;
}

static int test_level_load_specials(void)
{
    WorldData world;
    WorldData *saved_world = gpWorld;

    memset(&world, 0, sizeof(world));
    gpWorld = &world;

    GameStruct.GameState = UINT32_C(0x01000020);
    jpb_LoaderApplyLevelLoadSpecialsForTest(1);
    CHECK(GameStruct.GameState == UINT32_C(0x20));

    world.aDolly[39].flags = UINT32_C(0x40);
    world.aBkDolly[39].flags = UINT32_C(0x80);
    jpb_LoaderApplyLevelLoadSpecialsForTest(5);
    CHECK(world.aDolly[39].flags == UINT32_C(0x2040));
    CHECK(world.aBkDolly[39].flags == UINT32_C(0x2080));

    GameStruct.GameState = UINT32_C(0x20);
    jpb_LoaderApplyLevelLoadSpecialsForTest(9);
    CHECK(GameStruct.GameState == UINT32_C(0x01000020));

    world.aDolly[8].flags = UINT32_C(0x100);
    world.aBkDolly[8].flags = UINT32_C(0x200);
    jpb_LoaderApplyLevelLoadSpecialsForTest(11);
    CHECK(world.aDolly[8].flags == UINT32_C(0x2100));
    CHECK(world.aBkDolly[8].flags == UINT32_C(0x2200));

    world.aDolly[0].flags = UINT32_MAX;
    world.aDolly[0].offset.vx = 0x6000;
    world.aDolly[0].offset.vy = 0x0400;
    world.aDolly[0].offset.vz = 0x0100;
    world.aBkDolly[0].flags = UINT32_MAX;
    world.aBkDolly[0].offset.vx = 0x7000;
    world.aBkDolly[0].offset.vy = 0x0500;
    world.aBkDolly[0].offset.vz = 0x0200;
    jpb_LoaderApplyLevelLoadSpecialsForTest(12);
    CHECK(world.aDolly[0].flags == 0);
    CHECK(world.aDolly[0].offset.vx == 0x0d30);
    CHECK(world.aDolly[0].offset.vy == 0x0144);
    CHECK(world.aDolly[0].offset.vz == 0x009c);
    CHECK(world.aBkDolly[0].flags == 0);
    CHECK(world.aBkDolly[0].offset.vx == 0x1d30);
    CHECK(world.aBkDolly[0].offset.vy == 0x0244);
    CHECK(world.aBkDolly[0].offset.vz == 0x019c);

    GameStruct.GameState = UINT32_C(0x01000020);
    jpb_LoaderApplyLevelLoadSpecialsForTest(13);
    CHECK(GameStruct.GameState == UINT32_C(0x20));

    gpWorld = saved_world;
    return 0;
}

static int test_jarjar_weapon_override(void)
{
    enum {
        GUNGAN_NODE_COUNT = 10,
        ORIGINAL_NODE_COUNT = 40,
        VERTEX_BYTES = 8,
        UV_BYTES = 128,
        COLOR_BYTES = 16,
        INDEX_BYTES = 8,
        EXTRA_BYTES =
            VERTEX_BYTES * 2 + UV_BYTES + COLOR_BYTES + INDEX_BYTES,
        GUNGAN_PAYLOAD_BYTES =
            GUNGAN_NODE_COUNT * sizeof(geomData) + EXTRA_BYTES,
        GUNGAN_FILE_BYTES = 4 + GUNGAN_PAYLOAD_BYTES,
        ORIGINAL_BYTES = ORIGINAL_NODE_COUNT * sizeof(geomData)
    };
    static char pool[16 * 1024];
    static const uint8_t vertex_data[VERTEX_BYTES] =
        {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
    static const uint8_t normal_data[VERTEX_BYTES] =
        {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27};
    uint8_t gungan_file[GUNGAN_FILE_BYTES];
    geomData *gungan_model = (geomData *)(void *)(gungan_file + 4);
    geomData *gungan_weapon = &gungan_model[1];
    geomData original[ORIGINAL_NODE_COUNT];
    geomData *result = original;
    geomData *result_weapon;
    long result_size = ORIGINAL_BYTES;
    FILE *file;
    size_t stream_offset = GUNGAN_NODE_COUNT * sizeof(geomData);
    size_t index;

    memset(gungan_file, 0, sizeof(gungan_file));
    memset(original, 0, sizeof(original));
    memcpy(gungan_weapon->name, "weapon1", sizeof("weapon1"));
    gungan_weapon->numFaces = 1;
    gungan_weapon->numVerts = 2;
    gungan_weapon->numChildren = 1;
    gungan_weapon->aChildren[0] = 2;
    gungan_weapon->pVertex = (int32_t)stream_offset;
    gungan_weapon->pNormal = gungan_weapon->pVertex + VERTEX_BYTES;
    gungan_weapon->pUV = gungan_weapon->pNormal + VERTEX_BYTES;
    gungan_weapon->pColor = gungan_weapon->pUV + UV_BYTES;
    gungan_weapon->pIndex = gungan_weapon->pColor + COLOR_BYTES;
    memcpy(gungan_model[2].name, "weapon_child", sizeof("weapon_child"));
    gungan_model[2].id = UINT32_C(0x12345678);
    memcpy(
        (uint8_t *)gungan_model + gungan_weapon->pVertex,
        vertex_data,
        sizeof(vertex_data));
    memcpy(
        (uint8_t *)gungan_model + gungan_weapon->pNormal,
        normal_data,
        sizeof(normal_data));
    memset(
        (uint8_t *)gungan_model + gungan_weapon->pUV,
        0x30,
        UV_BYTES);
    memset(
        (uint8_t *)gungan_model + gungan_weapon->pColor,
        0x40,
        COLOR_BYTES);
    memset(
        (uint8_t *)gungan_model + gungan_weapon->pIndex,
        0x50,
        INDEX_BYTES);
    memcpy(original[5].name, "weapon1", sizeof("weapon1"));

    (void)_mkdir("jpb_loader_test_data");
    (void)_mkdir("jpb_loader_test_data/res");
    (void)_mkdir("jpb_loader_test_data/res/model");
    file = fopen(
        "jpb_loader_test_data/res/model/gungan_1.bmd", "wb");
    CHECK(file != NULL);
    CHECK(fwrite(gungan_file, 1, sizeof(gungan_file), file) ==
          sizeof(gungan_file));
    CHECK(fclose(file) == 0);

    memset(maMemoryBanks, 0, sizeof(maMemoryBanks));
    CHECK(memory_InitMemoryPool(pool, 16, 1) != 0);
    CHECK(jpb_ResourceSetBasePath("jpb_loader_test_data") == 1);
    loader_loadJarJarOverrideModel(&result, &result_size);

    CHECK(result != original);
    CHECK(result_size == ORIGINAL_BYTES + EXTRA_BYTES);
    result_weapon = &result[5];
    CHECK(result_weapon->pVertex == ORIGINAL_BYTES);
    CHECK(result_weapon->pNormal == ORIGINAL_BYTES + VERTEX_BYTES);
    CHECK(result_weapon->pUV == ORIGINAL_BYTES + VERTEX_BYTES * 2);
    CHECK(result_weapon->pColor ==
          ORIGINAL_BYTES + VERTEX_BYTES * 2 + UV_BYTES);
    CHECK(result_weapon->pIndex ==
          ORIGINAL_BYTES + VERTEX_BYTES * 2 + UV_BYTES + COLOR_BYTES);
    CHECK(result_weapon->aChildren[0] == 39);
    CHECK(memcmp(&result[39], &gungan_model[2], sizeof(geomData)) == 0);
    CHECK(memcmp(
              (uint8_t *)result + result_weapon->pVertex,
              vertex_data,
              sizeof(vertex_data)) == 0);
    CHECK(memcmp(
              (uint8_t *)result + result_weapon->pNormal,
              normal_data,
              sizeof(normal_data)) == 0);
    for (index = 0; index < UV_BYTES; ++index) {
        CHECK(((uint8_t *)result)[result_weapon->pUV + index] == 0x30);
    }
    for (index = 0; index < COLOR_BYTES; ++index) {
        CHECK(((uint8_t *)result)[result_weapon->pColor + index] == 0x40);
    }
    for (index = 0; index < INDEX_BYTES; ++index) {
        CHECK(((uint8_t *)result)[result_weapon->pIndex + index] == 0x50);
    }

    free(result);
    CHECK(_unlink("jpb_loader_test_data/res/model/gungan_1.bmd") == 0);
    CHECK(_rmdir("jpb_loader_test_data/res/model") == 0);
    CHECK(_rmdir("jpb_loader_test_data/res") == 0);
    CHECK(_rmdir("jpb_loader_test_data") == 0);
    return 0;
}

int main(void)
{
    int result = 0;

    result |= test_player_character_boundary();
    result |= test_node_name_search();
    result |= test_data_array_initialization();
    result |= test_level_load_specials();
    result |= test_jarjar_weapon_override();
    if (result == 0) {
        puts("loader tests passed");
    }
    return result;
}
