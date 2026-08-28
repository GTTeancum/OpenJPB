#include "jpb/filesys.h"
#include "jpb/effects.h"
#include "jpb/globalarrays.h"
#include "jpb/io.h"
#include "jpb/jonny.h"
#include "jpb/memory.h"
#include "jpb/world.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <crtdbg.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static char test_path[64];
static int jonny_clear_events_calls;
static int jonny_initialize_uvs_calls;
static int chunks_post_load_calls;
static int chunks_post_load_flag;
static int asset_resolver_uses_test_path;
static int texture_load_calls;
static int texture_load_types[JPB_RESIDENT_SPRITE_COUNT + 2];
static uint32_t texture_load_options[JPB_RESIDENT_SPRITE_COUNT + 2];
static int texture_load_null_path[JPB_RESIDENT_SPRITE_COUNT + 2];
static char texture_load_names[JPB_RESIDENT_SPRITE_COUNT + 2][32];
static int append_open_calls;
static char append_open_name[64];
static char append_open_mode[8];
#if defined(_MSC_VER)
static int invalid_parameter_calls;

static void test_invalid_parameter_handler(
    const wchar_t *expression,
    const wchar_t *function,
    const wchar_t *file,
    unsigned int line,
    uintptr_t reserved)
{
    (void)expression;
    (void)function;
    (void)file;
    (void)line;
    (void)reserved;
    ++invalid_parameter_calls;
}
#endif

static void *test_append_open_hook(const char *name, const char *mode)
{
    ++append_open_calls;
    (void)snprintf(append_open_name, sizeof(append_open_name), "%s", name);
    (void)snprintf(append_open_mode, sizeof(append_open_mode), "%s", mode);
    return NULL;
}

static void test_clear_events_hook(void)
{
    ++jonny_clear_events_calls;
}

static void test_initialize_uvs_hook(void)
{
    ++jonny_initialize_uvs_calls;
}

static void test_chunks_post_load_hook(
    const char *resource_name,
    int loadtextures)
{
    if (strcmp(resource_name, test_path) == 0) {
        ++chunks_post_load_calls;
    }
    chunks_post_load_flag = loadtextures;
}

static const char *test_asset_path_resolver(
    const char *resource_name,
    int resource_type,
    const char *extension)
{
    (void)resource_type;
    (void)extension;
    return asset_resolver_uses_test_path ? test_path : resource_name;
}

static void *test_texture_load_hook(
    const char *path,
    int texture_type,
    uint32_t option)
{
    int call = texture_load_calls++;

    texture_load_types[call] = texture_type;
    texture_load_options[call] = option;
    texture_load_null_path[call] = path == NULL;
    if (path != NULL) {
        (void)snprintf(
            texture_load_names[call],
            sizeof(texture_load_names[call]),
            "%s",
            path);
    }
    return (void *)(uintptr_t)(call + 1);
}

static int test_stream_io(void)
{
    char first[] = {'J', 'P', 'B'};
    char second[] = {'!', '?'};
    char readback[5] = {0};
    JPBFileHandle fd = 0;

    (void)remove(test_path);
    CHECK(file_WriteFile(test_path, first, 3) == 1);
    append_open_calls = 0;
    append_open_name[0] = '\0';
    append_open_mode[0] = '\0';
    jpb_IOSetFileAppendOpenTestHook(test_append_open_hook);
    CHECK(file_AppendFile(test_path, second, 2) == 0);
    jpb_IOSetFileAppendOpenTestHook(NULL);
    CHECK(append_open_calls == 1);
    CHECK(strcmp(append_open_name, test_path) == 0);
    CHECK(strcmp(append_open_mode, "awb") == 0);
    CHECK(file_getFileSize(test_path) == 3);
    {
        char complete[] = {'J', 'P', 'B', '!', '?'};

        CHECK(file_WriteFile(test_path, complete, 5) == 1);
    }
    CHECK(file_getFileSize(test_path) == 5);
    CHECK(file_OPEN(test_path, &fd) == 1);
    CHECK(file_GETSIZE(&fd) == 5);
    CHECK(file_READ(&fd, readback, 2, JPB_FILE_READ_STREAM) == 2);
    CHECK(memcmp(readback, "JP", 2) == 0);
    CHECK(file_SEEK(&fd, 1) == 0);
    CHECK(file_READ(&fd, readback, 4, JPB_FILE_READ_STREAM) == 4);
    CHECK(memcmp(readback, "PB!?", 4) == 0);
    CHECK(file_CLOSE(&fd) == 0);

    fd = (uintptr_t)0x1234;
    CHECK(file_OPEN(NULL, &fd) == 0);
    CHECK(fd == (uintptr_t)0x1234);
    CHECK(file_OPEN("jpb_missing_file.bin", &fd) == 0);
    CHECK(fd == (uintptr_t)0x1234);
    return 0;
}

static int test_retail_noops(void)
{
    char name[] = "unchanged";
    char buffer[] = "untouched";

    file_gInitialise();
    (void)file_ReadPC(name, buffer);
    CHECK(strcmp(name, "unchanged") == 0);
    CHECK(strcmp(buffer, "untouched") == 0);
    return 0;
}

static int test_read_modes(void)
{
    char source[] = {'a', 'b', 'c', 'd'};
    char output[4] = {0};
    JPBFileHandle fd = (JPBFileHandle)(uintptr_t)source;
    void *all;

    CHECK(file_READ(&fd, output, 3, JPB_FILE_READ_MEMORY) == 1);
    CHECK(memcmp(output, "abc", 3) == 0);
    CHECK((char *)(uintptr_t)fd == source + 3);

    CHECK(file_OPEN(test_path, &fd) == 1);
    CHECK(file_READ(&fd, NULL, 0, JPB_FILE_READ_ALL) != 0);
    all = (void *)(uintptr_t)fd;
    CHECK(memcmp(all, "JPB!?", 5) == 0);
    free(all);
    return 0;
}

static int test_high_level_loaders(void)
{
    char output[8] = {0};
    unsigned char *output_ptr = (unsigned char *)output;
    int32_t size = -1;
    char *pooled;

    gFileNotFound = 0;
    CHECK(file_LoadFile(test_path, output) == 5);
    CHECK(memcmp(output, "JPB!?", 5) == 0);
    CHECK(gFileNotFound == 0);
    CHECK(io_file_LoadFile(
              (unsigned char *)test_path, &output_ptr) == 5);

    (void)memory_InitMemorySystem();
    pooled = io_file_LoadFile2Pool(test_path, &size, 2);
    CHECK(pooled != NULL);
    CHECK(size == 5);
    CHECK(memcmp(pooled, "JPB!?", 5) == 0);

    CHECK(file_LoadFile("jpb_missing_file.bin", output) == 0);
    CHECK(gFileNotFound == 1);
    CHECK(file_getFileSize("jpb_missing_file.bin") == 0);
    return 0;
}

static int test_chunk_decoder(void)
{
    uint8_t bytes[sizeof(wapChunk) * 2] = {0};
    uint8_t *cursor = bytes;
    wapChunk *chunk;

    memcpy(bytes, "B3D_VER ", 8);
    ((wapChunk *)bytes)->size = UINT32_C(0x11223344);
    ((wapChunk *)bytes)->realsize = UINT32_C(0x55667788);
    CHECK(readchunk(&cursor, &chunk) == 23);
    CHECK(chunk == (wapChunk *)bytes);
    CHECK(cursor == bytes + sizeof(wapChunk));
    CHECK(chunk->size == UINT32_C(0x11223344));
    CHECK(chunk->realsize == UINT32_C(0x55667788));

    memcpy(cursor, "UNKNOWN!", 8);
    CHECK(readchunk(&cursor, &chunk) == -1);
    CHECK(cursor == bytes + sizeof(wapChunk) * 2);
    CHECK(file_LoadVersionChunk(&cursor, 7, chunk) == 0);
    return 0;
}

static void write_u32(uint8_t *destination, uint32_t value)
{
    memcpy(destination, &value, sizeof(value));
}

static int test_chunk_relocation(void)
{
    _Alignas(8) uint8_t payload[2048] = {0};
    uint8_t *cursor;
    wapChunk chunk = {{0}, 0, 0};
    WorldData world;
    BAP_CAMERADOLLY *dolly = (BAP_CAMERADOLLY *)payload;

    memset(&world, 0, sizeof(world));
    gpWorld = &world;

    cursor = payload;
    chunk.size = 3;
    CHECK(file_LoadAnimMapChunk(&cursor, 99, &chunk) == 1);
    CHECK(world.animMapEnemies == (int32_t *)payload);
    CHECK(world.nAnimMap == 3);
    CHECK(cursor == payload + 12);

    cursor = payload;
    chunk.size = 20;
    CHECK(file_LoadColorChunk(&cursor, 99, &chunk) == 1);
    CHECK(world.pPalette == (int16_t *)payload);
    CHECK(world.pColor == (char *)payload + 0x400);
    CHECK(cursor == payload + 0x400 + 20);

    dolly[0].flags = UINT32_C(0xAABBCCDD);
    dolly[0].pitch = -123;
    dolly[0].offset.vz = 4567;
    cursor = payload;
    CHECK(file_LoadDollyChunk(&cursor, 99, &chunk) == 1);
    CHECK(world.pObiDolly == dolly);
    CHECK(world.aDolly[0].flags == UINT32_C(0xAABBCCDD));
    CHECK(world.aDolly[0].pitch == -123);
    CHECK(world.aDolly[0].offset.vz == 4567);
    CHECK(cursor == payload + sizeof(BAP_CAMERADOLLY) * 32);

    cursor = payload;
    chunk.size = 32;
    CHECK(file_LoadEmiterChunk(&cursor, 99, &chunk) == 1);
    CHECK(world.nPowerups == 2);
    CHECK(world.pPowerups == (wsl_Powerup *)payload);
    CHECK(cursor == payload + 32);

    cursor = payload;
    world.pPowerups = (wsl_Powerup *)(uintptr_t)1;
    chunk.size = 15;
    CHECK(file_LoadEmiterChunk(&cursor, 99, &chunk) == 1);
    CHECK(world.nPowerups == 0);
    CHECK(world.pPowerups == (wsl_Powerup *)(uintptr_t)1);
    CHECK(cursor == payload);

    cursor = payload;
    chunk.size = 17;
    CHECK(file_LoadEntryChunk(&cursor, 99, &chunk) == 1);
    CHECK(world.pEntry == (wsl_mapEntry *)payload);
    CHECK(cursor == payload + 17);

    cursor = payload;
    chunk.size = 2;
    CHECK(file_LoadFatChunk(&cursor, 99, &chunk) == 1);
    CHECK(world.pFat == (wsl_fatPoly *)payload);
    CHECK(cursor == payload + 88);

    cursor = payload;
    chunk.size = 48;
    CHECK(file_LoadMaterialChunk(&cursor, 99, &chunk) == 0);
    CHECK(world.pTexture == (wsl_BAP_TEXTURE *)payload);
    CHECK(world.numTexture == 3);
    CHECK(cursor == payload + 48);

    cursor = payload;
    chunk.size = 24;
    CHECK(file_LoadTagChunk(&cursor, 99, &chunk) == 1);
    CHECK(world.pLibTags == (wsl_libTags *)payload);
    CHECK(world.numTags == 2);
    CHECK(cursor == payload + 24);

    cursor = payload;
    chunk.size = 2;
    CHECK(file_LoadThinChunk(&cursor, 99, &chunk) == 1);
    CHECK(world.pThin == (wsl_thinPoly *)payload);
    CHECK(cursor == payload + 40);

    cursor = payload;
    chunk.size = 19;
    CHECK(file_loadEntryTags(&cursor, 99, &chunk) == 1);
    CHECK(world.pEntryTags == (wsl_entryTags *)payload);
    CHECK(cursor == payload + 19);
    return 0;
}

static int test_map_relocation(void)
{
    _Alignas(8) uint8_t payload[128] = {0};
    uint8_t *cursor = payload;
    wapChunk chunk = {{0}, UINT32_C(0x1234), 0};
    WorldData world;
    CVECTOR color = {1, 2, 3, 4};

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    memcpy(payload, &color, sizeof(color));
    write_u32(payload + 4, 1000);
    write_u32(payload + 8, 2000);
    write_u32(payload + 12, 3000);
    write_u32(payload + 16, UINT32_C(0xFFECFFF6));
    write_u32(payload + 20, UINT32_C(0x0028001E));
    write_u32(payload + 24, 9);

    CHECK(file_LoadMapChunk(&cursor, 77, &chunk) == 1);
    CHECK(world.sizeX == 0x12);
    CHECK(world.sizeZ == 0x34);
    CHECK(memcmp(&world.bkColor, &color, sizeof(color)) == 0);
    CHECK(world.minX == -10);
    CHECK(world.minZ == -20);
    CHECK(world.maxX == 30);
    CHECK(world.maxZ == 40);
    CHECK(world.start.vx == 990);
    CHECK(world.start.vy == 3000);
    CHECK(world.start.vz == 1980);
    CHECK(world.pNewMap == (wsl_mapSlot *)(payload + 28));
    CHECK(cursor == payload + 37);
    return 0;
}

static int test_actor_enemy_relocation(void)
{
    _Alignas(8) uint8_t actor_payload[64] = {0};
    _Alignas(8) uint8_t enemy_payload[
        sizeof(int32_t) * 3 + sizeof(wsl_BAP_PLACEMENT) * 2] = {0};
    uint8_t *cursor;
    wapChunk chunk = {{0}, 0, 0};
    WorldData world;
    wsl_BAP_PLACEMENT *first_enemy;
    wsl_BAP_PLACEMENT *second_enemy;

    memset(&world, 0, sizeof(world));
    gpWorld = &world;

    write_u32(actor_payload, 28);
    write_u32(actor_payload + 4, 8);
    write_u32(actor_payload + 8, 12);
    memcpy(actor_payload + 12, "alpha", 6);
    memcpy(actor_payload + 20, "second", 7);
    cursor = actor_payload;
    chunk.size = 2;
    CHECK(file_LoadActorChunk(&cursor, 0, &chunk) == 1);
    CHECK(world.nActor == 2);
    CHECK(world.apActorNames != NULL);
    CHECK(world.apActorNames[0] == (char *)actor_payload + 12);
    CHECK(world.apActorNames[1] == (char *)actor_payload + 20);
    CHECK(strcmp(world.apActorNames[0], "alpha") == 0);
    CHECK(strcmp(world.apActorNames[1], "second") == 0);
    CHECK(cursor == actor_payload + 32);
    free(world.apActorNames);

    write_u32(enemy_payload, 2);
    write_u32(enemy_payload + 4, sizeof(wsl_BAP_PLACEMENT));
    write_u32(enemy_payload + 8, sizeof(wsl_BAP_PLACEMENT));
    first_enemy =
        (wsl_BAP_PLACEMENT *)(enemy_payload + sizeof(int32_t) * 3);
    second_enemy = first_enemy + 1;
    first_enemy->genDelay = 44;
    first_enemy->pLastEnemy = 55;
    first_enemy->wayPoints[0].loc.vx = 0x12;
    first_enemy->wayPoints[0].loc.vy = 0x34;
    first_enemy->wayPoints[0].loc.vz = 0x56;
    second_enemy->genDelay = 66;
    second_enemy->pLastEnemy = 77;
    second_enemy->wayPoints[0].loc.vx = 0x78;
    second_enemy->wayPoints[0].loc.vy = 0x9A;
    second_enemy->wayPoints[0].loc.vz = 0xBC;
    cursor = enemy_payload;
    chunk.size = sizeof(enemy_payload) - sizeof(int32_t);
    CHECK(file_LoadEnemyChunk(&cursor, 0, &chunk) == 1);
    CHECK(world.nEnemy == 2);
    CHECK(world.apEnemy[0] == first_enemy);
    CHECK(world.apEnemy[1] == second_enemy);
    CHECK(first_enemy->genDelay == 0);
    CHECK(first_enemy->pLastEnemy == UINT32_MAX);
    CHECK(memcmp(&first_enemy->loc, &first_enemy->wayPoints[0].loc,
                 sizeof(first_enemy->loc)) == 0);
    CHECK(second_enemy->genDelay == 0);
    CHECK(second_enemy->pLastEnemy == UINT32_MAX);
    CHECK(memcmp(&second_enemy->loc, &second_enemy->wayPoints[0].loc,
                 sizeof(second_enemy->loc)) == 0);
    CHECK(cursor == enemy_payload + sizeof(enemy_payload));
    free(world.apEnemy);
    return 0;
}

static int test_animation_relocation(void)
{
    _Alignas(8) uint8_t payload[
        sizeof(int32_t) + sizeof(wsl_BT_ANIMDEF) +
        sizeof(wsl_BT_ANIMNODE)] = {0};
    uint8_t *cursor = payload;
    wapChunk chunk = {{0}, 1, 0};
    WorldData world;
    wsl_BT_ANIMDEF *definition =
        (wsl_BT_ANIMDEF *)(payload + sizeof(int32_t));
    wsl_BT_ANIMNODE *second_node = definition->aNodes + 1;

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    write_u32(payload, sizeof(payload) - sizeof(int32_t));
    definition->numFrames = -2;
    definition->totalNodes = 2;
    definition->aNodes[0].numEntries = 2;
    definition->aNodes[0].nodeSpeed = 0;
    definition->aNodes[0].aEntry[0].frame = 3;
    definition->aNodes[0].aEntry[1].frame = -4;
    second_node->numEntries = 1;
    second_node->nodeSpeed = 17;
    second_node->aEntry[0].frame = 5;

    CHECK(file_LoadAnimDefChunk(&cursor, 0, &chunk) == 1);
    CHECK(world.nADef == 1);
    CHECK(world.animDef[0] == definition);
    CHECK(definition->numFrames == -8192);
    CHECK(definition->aNodes[0].nodeSpeed == 0x1000);
    CHECK(definition->aNodes[0].aEntry[0].frame == 12288);
    CHECK(definition->aNodes[0].aEntry[1].frame == -16384);
    CHECK(second_node->nodeSpeed == 17);
    CHECK(second_node->aEntry[0].frame == 20480);
    CHECK(cursor == payload + sizeof(payload));
    return 0;
}

static int test_ai_relocation(void)
{
    enum {
        FIRST_AI_SIZE = sizeof(BAP_AI) + 24 + 8,
        SECOND_AI_SIZE = sizeof(BAP_AI) + 4
    };
    _Alignas(8) uint8_t payload[
        sizeof(int32_t) * 2 + FIRST_AI_SIZE + SECOND_AI_SIZE] = {0};
    uint8_t *cursor = payload;
    wapChunk chunk = {{0}, 2, sizeof(payload)};
    WorldData world;
    BAP_AI *first;
    BAP_AI *second;
    uint8_t *first_variables;
    uint8_t *second_variables;

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    pointerRegistry_Reset();
    write_u32(payload, FIRST_AI_SIZE);
    write_u32(payload + sizeof(int32_t), SECOND_AI_SIZE);
    first = (BAP_AI *)(payload + sizeof(int32_t) * 2);
    second = (BAP_AI *)((uint8_t *)first + FIRST_AI_SIZE);
    first->numNodes = 4;
    first->numAvailable = 2;
    second->numNodes = 1;
    second->numAvailable = 1;
    first_variables = (uint8_t *)first + sizeof(BAP_AI) + 12;
    second_variables = (uint8_t *)second + sizeof(BAP_AI);

    CHECK(file_LoadAiChunk(&cursor, 0, &chunk) == 1);
    CHECK(world.nAI == 2);
    CHECK(world.apAI[0] == first);
    CHECK(world.apAI[1] == second);
    CHECK(first->pVars == 0);
    CHECK(second->pVars == 1);
    CHECK(getPtr((int)first->pVars, JPB_POINTER_ARRAY_AI) ==
          first_variables);
    CHECK(getPtr((int)second->pVars, JPB_POINTER_ARRAY_AI) ==
          second_variables);
    CHECK(cursor == payload + sizeof(payload));
    free(world.apAI);
    pointerRegistry_Reset();
    return 0;
}

static int test_library_relocation(void)
{
    _Alignas(8) uint8_t payload[
        sizeof(void *) * 2 + 8 +
        sizeof(wsl_libPoly) * 2 +
        sizeof(int32_t) * 2 +
        sizeof(int16_t) * 4] = {0};
    uint8_t *cursor = payload;
    wapChunk chunk = {{0}, 1, 0};
    WorldData world;
    wsl_libPart *part = (wsl_libPart *)payload;

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    part->numverts = 1;
    part->numpolys = 2;

    CHECK(file_LoadLibChunk(&cursor, 0, &chunk) == 1);
    CHECK(world.numLibs == 1);
    CHECK(world.pLib != NULL);
    CHECK(world.pLib[0] == part);
    CHECK(part->index ==
          (int32_t *)(payload + sizeof(void *) * 2 + 8 +
                      sizeof(wsl_libPoly) * 2));
    CHECK(part->shared == (int16_t *)((uint8_t *)part->index +
                                     sizeof(int32_t) * 2));
    CHECK(cursor == payload + sizeof(payload));
    return 0;
}

static int test_jonny_relocation(void)
{
    enum {
        COLOR_OFFSET = 1024,
        VERTEX_OFFSET = 1032,
        JONNY_DATA_SIZE = 1064
    };
    _Alignas(8) uint8_t payload[
        sizeof(CVECTOR) + sizeof(rdVECTOR) + JONNY_DATA_SIZE] = {0};
    _Alignas(8) uint8_t relocate_stream[
        sizeof(wapChunk) + sizeof(payload)] = {0};
    uint8_t *cursor = payload;
    uint8_t *jonny_data = payload + sizeof(CVECTOR) + sizeof(rdVECTOR);
    int32_t *expected_leveldata = (int32_t *)(jonny_data + 16);
    uint8_t *expected_colorbase =
        (uint8_t *)expected_leveldata + COLOR_OFFSET;
    uint8_t *relocated_end = NULL;
    wapChunk chunk = {{0}, 0, JONNY_DATA_SIZE};
    WorldData world;
    CVECTOR background = {10, 20, 30, 40};
    rdVECTOR start = {100, -200, 300};

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    memcpy(payload, &background, sizeof(background));
    memcpy(payload + sizeof(background), &start, sizeof(start));
    write_u32(jonny_data + 4, 0);
    write_u32(jonny_data + 8, COLOR_OFFSET);
    write_u32(jonny_data + 12, VERTEX_OFFSET);
    write_u32(expected_colorbase, UINT32_C(0x007F4080));
    write_u32(expected_colorbase - 4, UINT32_C(0x00010203));
    jonny_clear_events_calls = 0;
    jonny_initialize_uvs_calls = 0;
    file_SetJonnyPostLoadHooks(
        test_clear_events_hook, test_initialize_uvs_hook);

    CHECK(file_LoadJonnyChunk(&cursor, 0, &chunk) == 1);
    CHECK(memcmp(&world.bkColor, &background, sizeof(background)) == 0);
    CHECK(memcmp(&world.start, &start, sizeof(start)) == 0);
    CHECK(jonnylevel == (char *)jonny_data);
    CHECK(leveldata == expected_leveldata);
    CHECK(mapyend == 1);
    CHECK(texturebase == expected_leveldata);
    CHECK(colorbase == (int32_t *)expected_colorbase);
    CHECK(vertbase ==
          (int32_t *)((uint8_t *)expected_leveldata + VERTEX_OFFSET));
    CHECK(*(uint32_t *)expected_colorbase == UINT32_C(0x00FF80FE));
    CHECK(*(uint32_t *)(expected_colorbase - 4) ==
          UINT32_C(0x00060402));
    CHECK(jonny_clear_events_calls == 1);
    CHECK(jonny_initialize_uvs_calls == 1);
    CHECK(cursor == payload + sizeof(payload));
    file_SetJonnyPostLoadHooks(NULL, NULL);

    memcpy(((wapChunk *)relocate_stream)->id, "JONCHUNK", 8);
    ((wapChunk *)relocate_stream)->size = sizeof(payload);
    ((wapChunk *)relocate_stream)->realsize = sizeof(payload);
    memcpy(relocate_stream + sizeof(wapChunk), payload, sizeof(payload));
    CHECK(file_RelocateChunks(
              relocate_stream, sizeof(relocate_stream), &relocated_end) ==
          JPB_CHUNKS_OK);
    CHECK(relocated_end == relocate_stream + sizeof(relocate_stream));
    return 0;
}

static int test_chunk_stream(void)
{
    enum {
        MATERIAL_BYTES = 16,
        ANIM_MAP_COUNT = 2,
        STREAM_BYTES =
            sizeof(wapChunk) + MATERIAL_BYTES +
            sizeof(wapChunk) + ANIM_MAP_COUNT * sizeof(int32_t)
    };
    _Alignas(8) uint8_t stream[STREAM_BYTES] = {0};
    uint8_t unknown[sizeof(wapChunk)] = {0};
    uint8_t version_only[sizeof(wapChunk)] = {0};
    uint8_t unsupported[sizeof(wapChunk)] = {0};
    uint8_t *end_cursor = NULL;
    wapChunk *material = (wapChunk *)stream;
    wapChunk *anim_map =
        (wapChunk *)(stream + sizeof(wapChunk) + MATERIAL_BYTES);
    WorldData world;
    int32_t loaded_size = -1;
    char *loaded_end;
    uint8_t *loaded_base;
    int relocate_result;

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    memcpy(material->id, "B3D_MAT ", 8);
    material->size = MATERIAL_BYTES;
    material->realsize = MATERIAL_BYTES;
    memcpy(anim_map->id, "B3D_TAMP", 8);
    anim_map->size = ANIM_MAP_COUNT;
    anim_map->realsize = ANIM_MAP_COUNT * sizeof(int32_t);
    write_u32((uint8_t *)(anim_map + 1), 11);
    write_u32((uint8_t *)(anim_map + 1) + sizeof(int32_t), 22);

    relocate_result =
        file_RelocateChunks(stream, sizeof(stream), &end_cursor);
    if (relocate_result != JPB_CHUNKS_OK) {
        fprintf(stderr, "chunk relocation result: %d\n", relocate_result);
    }
    CHECK(relocate_result == JPB_CHUNKS_OK);
    CHECK(end_cursor == stream + sizeof(stream));
    CHECK(world.pTexture == (wsl_BAP_TEXTURE *)(material + 1));
    CHECK(world.numTexture == 1);
    CHECK(world.animMapEnemies == (int32_t *)(anim_map + 1));
    CHECK(world.nAnimMap == ANIM_MAP_COUNT);

    memcpy(unknown, "NOTACHNK", 8);
    CHECK(file_RelocateChunks(unknown, sizeof(unknown), NULL) ==
          JPB_CHUNKS_UNKNOWN);
    memcpy(version_only, "B3D_VER ", 8);
    CHECK(file_RelocateChunks(version_only, sizeof(version_only), NULL) ==
          JPB_CHUNKS_OK);
    memcpy(unsupported, "B3D_APRT", 8);
    CHECK(file_RelocateChunks(unsupported, sizeof(unsupported), NULL) ==
          JPB_CHUNKS_UNSUPPORTED);
    CHECK(file_RelocateChunks(stream, sizeof(wapChunk) - 1, NULL) ==
          JPB_CHUNKS_TRUNCATED);
    end_cursor = stream;
    CHECK(file_RelocateChunks(NULL, 0, &end_cursor) == JPB_CHUNKS_OK);
    CHECK(end_cursor == NULL);

    CHECK(file_WriteFile(test_path, (char *)stream, sizeof(stream)) == 1);
    chunks_post_load_calls = 0;
    chunks_post_load_flag = 0;
    file_SetChunkLoadHooks(NULL, test_chunks_post_load_hook);
    loaded_end = file_LoadChunks2Pool(
        "", test_path, "", &loaded_size, 37);
    CHECK(loaded_end != NULL);
    CHECK(loaded_size == sizeof(stream));
    loaded_base = (uint8_t *)loaded_end - sizeof(stream);
    CHECK(world.pTexture ==
          (wsl_BAP_TEXTURE *)(loaded_base + sizeof(wapChunk)));
    CHECK(world.animMapEnemies ==
          (int32_t *)(loaded_base +
                      sizeof(wapChunk) + MATERIAL_BYTES +
                      sizeof(wapChunk)));
    CHECK(chunks_post_load_calls == 1);
    CHECK(chunks_post_load_flag == 37);
    file_SetChunkLoadHooks(NULL, NULL);
    return 0;
}

static int test_effect_and_sprite_boundaries(void)
{
    int index;

    asset_resolver_uses_test_path = 1;
    file_SetChunkLoadHooks(test_asset_path_resolver, NULL);
    gFileNotFound = 0;
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(aEmiter, 0, sizeof(aEmiter));
    memset(paEffects, 0, sizeof(paEffects));
    file_LoadEffects();
    CHECK(gFileNotFound == 0);
    CHECK(memcmp(maProjTypes, "B3D_MAT ", 8) == 0);
    CHECK(memcmp(aEmiter, "B3D_MAT ", 8) == 0);
    CHECK(gMaxEffect == JPB_EFFECT_COUNT);
    for (index = 0; index < JPB_EFFECT_COUNT; ++index) {
        CHECK(paEffects[index] != NULL);
        CHECK(memcmp(paEffects[index], "B3D_MAT ", 8) == 0);
    }

    asset_resolver_uses_test_path = 0;
    texture_load_calls = 0;
    memset(texture_load_options, 0, sizeof(texture_load_options));
    memset(texture_load_types, 0, sizeof(texture_load_types));
    memset(texture_load_null_path, 0, sizeof(texture_load_null_path));
    memset(texture_load_names, 0, sizeof(texture_load_names));
    file_SetTextureLoadHook(test_texture_load_hook);
    file_LoadResidentSprites();
    CHECK(texture_load_calls == JPB_RESIDENT_SPRITE_COUNT + 2);
    CHECK(texture_load_types[0] == 1);
    CHECK(texture_load_types[51] == 1);
    CHECK(strcmp(texture_load_names[0], "a_pal.tga") == 0);
    CHECK(strcmp(texture_load_names[38], "a_gball.tga") == 0);
    CHECK(strcmp(texture_load_names[39], "a_meter_lights.tga") == 0);
    CHECK(strcmp(texture_load_names[45], "a_detonator.tga") == 0);
    CHECK(strcmp(texture_load_names[46], "a_bolt.tga") == 0);
    CHECK(strcmp(texture_load_names[47], "a_battery.tga") == 0);
    CHECK(strcmp(texture_load_names[48], "a_shield.tga") == 0);
    CHECK(strcmp(texture_load_names[49], "a_credit.tga") == 0);
    CHECK(texture_load_options[0] == 2);
    CHECK(texture_load_options[38] == 2);
    CHECK(texture_load_options[39] == 1);
    CHECK(texture_load_options[45] == 1);
    CHECK(texture_load_options[46] == 1);
    CHECK(texture_load_options[47] == 1);
    CHECK(texture_load_options[48] == 1);
    CHECK(texture_load_options[49] == 1);
    CHECK(texture_load_null_path[50] == 1);
    CHECK(texture_load_options[50] == 1);
    CHECK(texture_load_null_path[51] == 1);
    CHECK(texture_load_options[51] == 2);
    CHECK(effects1Handle[0] == (void *)(uintptr_t)1);
    CHECK(effects1Handle[49] == (void *)(uintptr_t)50);
    CHECK(transHandle == (void *)(uintptr_t)51);
    CHECK(addHandle == (void *)(uintptr_t)52);
    file_SetTextureLoadHook(NULL);
    file_SetChunkLoadHooks(NULL, NULL);
    return 0;
}

int main(void)
{
#if defined(_MSC_VER)
    (void)snprintf(
        test_path, sizeof(test_path),
        "jpb_io_filesys_test_%d.bin", _getpid());
    (void)_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    (void)_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _invalid_parameter_handler previous_invalid_parameter_handler =
        _set_invalid_parameter_handler(test_invalid_parameter_handler);
#else
    (void)snprintf(
        test_path, sizeof(test_path),
        "jpb_io_filesys_test_%ld.bin", (long)getpid());
#endif
    int result =
        test_stream_io() != 0 ||
        test_retail_noops() != 0 ||
        test_read_modes() != 0 ||
        test_high_level_loaders() != 0 ||
        test_chunk_decoder() != 0 ||
        test_chunk_relocation() != 0 ||
        test_map_relocation() != 0 ||
        test_actor_enemy_relocation() != 0 ||
        test_animation_relocation() != 0 ||
        test_ai_relocation() != 0 ||
        test_library_relocation() != 0 ||
        test_jonny_relocation() != 0 ||
        test_chunk_stream() != 0 ||
        test_effect_and_sprite_boundaries() != 0;

#if defined(_MSC_VER)
    if (invalid_parameter_calls != 0) {
        fprintf(
            stderr,
            "unexpected CRT invalid-parameter calls: %d\n",
            invalid_parameter_calls);
        result = 1;
    }
    (void)_set_invalid_parameter_handler(previous_invalid_parameter_handler);
#endif
    (void)remove(test_path);
    if (result) {
        return 1;
    }
    puts("portable IO/filesys tests passed");
    return 0;
}
