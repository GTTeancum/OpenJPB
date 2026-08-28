#include "jpb/anim.h"
#include "jpb/jonny.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/world.h"

#include <math.h>
#include <stdlib.h>
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

static int close_float(float left, float right)
{
    return fabsf(left - right) < 0.0001f;
}

static int test_makecull_planes(void)
{
    MATRIX camera = {
        {{1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f},
         {0.0f, 0.0f, 1.0f}},
        {0, 0, 0}
    };
    FVECTOR camera_position = {1.0f, 2.0f, 3.0f};
    FVECTOR4 planes[7];
    float diagonal = 0.707106769f;

    memset(planes, 0, sizeof(planes));
    makecull(
        planes, &camera, &camera_position,
        2.0f, 6.0f, 8.0f, 4.0f, 10.0f);

    CHECK(close_float(planes[0].vx, 0.8f));
    CHECK(close_float(planes[0].vy, 0.0f));
    CHECK(close_float(planes[0].vz, 0.6f));
    CHECK(close_float(planes[0].vw, 0.4f));
    CHECK(close_float(planes[1].vx, -0.8f));
    CHECK(close_float(planes[1].vz, 0.6f));
    CHECK(close_float(planes[1].vw, 2.0f));
    CHECK(close_float(planes[2].vy, diagonal));
    CHECK(close_float(planes[2].vz, diagonal));
    CHECK(close_float(planes[2].vw, 3.0f - 5.0f * diagonal));
    CHECK(close_float(planes[3].vx, 0.0f));
    CHECK(close_float(planes[3].vy, 0.0f));
    CHECK(close_float(planes[3].vz, 1.0f));
    CHECK(close_float(planes[3].vw, 0.0f));
    CHECK(close_float(planes[4].vy, -diagonal));
    CHECK(close_float(planes[4].vz, diagonal));
    CHECK(close_float(planes[4].vw, 3.0f - diagonal));
    CHECK(signbit(planes[5].vx));
    CHECK(signbit(planes[5].vy));
    CHECK(close_float(planes[5].vz, -1.0f));
    CHECK(close_float(planes[5].vw, 16.0f));
    CHECK(memcmp(&planes[6], &planes[3], sizeof(planes[3])) == 0);
    return 0;
}

static uint16_t test_packed_vertex(void)
{
    return
        UINT16_C(3) |
        (UINT16_C(7) << 5) |
        (UINT16_C(5) << 10);
}

static int test_getlibpart_integer_vertices(void)
{
    int32_t storage[64];
    int32_t *mapbase = storage + 4;
    _svector *output = (_svector *)gaScratch;
    VECTOR cubeorg = {100, 200, 300, 0};
    int32_t cube = (int32_t)UINT32_C(0x12340000);
    uint16_t *verts;

    memset(storage, 0, sizeof(storage));
    memset(gaScratch, 0, 2048);
    mapbase[-1] = 64;
    mapbase[0] = 1 << 16;
    mapbase[1] = 1 << 15;
    verts = (uint16_t *)((uint8_t *)mapbase + 64);
    verts[0] = test_packed_vertex();

    CHECK(jon_getlibpart(&cube, &cubeorg, mapbase) == mapbase);
    CHECK(output[0].vx == 356);
    CHECK(output[0].vy == 200);
    CHECK(output[0].vz == 300);
    CHECK(output[1].vx == 100);
    CHECK(output[2].vz == 556);
    CHECK(output[4].vy == 712);
    CHECK(output[7].vx == 100);
    CHECK(output[7].vy == 712);
    CHECK(output[7].vz == 556);
    CHECK(output[8].vx == 308);
    CHECK(output[8].vy == 280);
    CHECK(output[8].vz == 412);
    return 0;
}

static int test_getlibpart_float_vertices(void)
{
    int32_t storage[64];
    int32_t *mapbase = storage + 4;
    FVECTOR output[9];
    FVECTOR cubeorg = {100.0f, 200.0f, 300.0f};
    int32_t cube = (int32_t)UINT32_C(0xabcd0000);
    uint16_t *verts;
    int32_t numverts = 0;

    memset(storage, 0, sizeof(storage));
    memset(output, 0, sizeof(output));
    mapbase[-1] = 64;
    mapbase[0] = 1 << 16;
    mapbase[1] = 1 << 15;
    verts = (uint16_t *)((uint8_t *)mapbase + 64);
    verts[0] = test_packed_vertex();

    CHECK(
        jon_getlibpartfloat(
            output, &cube, &cubeorg, mapbase, &numverts) == mapbase);
    CHECK(numverts == 9);
    CHECK(output[0].vx == 356.0f);
    CHECK(output[4].vy == 712.0f);
    CHECK(output[8].vx == 308.0f);
    CHECK(output[8].vy == 280.0f);
    CHECK(output[8].vz == 412.0f);
    return 0;
}

static int test_getlibpart_long_vertices(void)
{
    int32_t storage[64];
    int32_t *mapbase = storage + 4;
    VECTOR *output = (VECTOR *)(void *)gaScratch;
    VECTOR cubeorg = {100, 200, 300, 0};
    int32_t cube = (int32_t)UINT32_C(0x56780000);
    uint16_t *verts;

    memset(storage, 0, sizeof(storage));
    memset(gaScratch, 0x5a, 2048);
    mapbase[-1] = 64;
    mapbase[0] = 1 << 16;
    mapbase[1] = 1 << 15;
    verts = (uint16_t *)((uint8_t *)mapbase + 64);
    verts[0] = test_packed_vertex();

    CHECK(jon_getlibpartint32_t(&cube, &cubeorg, mapbase) == mapbase);
    CHECK(output[0].vx == 356);
    CHECK(output[0].vy == 200);
    CHECK(output[0].vz == 300);
    CHECK(output[0].pad == (int32_t)UINT32_C(0x5a5a5a5a));
    CHECK(output[4].vy == 712);
    CHECK(output[7].vx == 100);
    CHECK(output[7].vy == 712);
    CHECK(output[7].vz == 556);
    CHECK(output[8].vx == 308);
    CHECK(output[8].vy == 280);
    CHECK(output[8].vz == 412);
    CHECK(output[8].pad == (int32_t)UINT32_C(0x5a5a5a5a));
    return 0;
}

static int test_retail_jonny_stubs(void)
{
    MATRIX matrix;
    _svector svector;
    VECTOR vector;
    int marker;

    memset(&matrix, 0, sizeof(matrix));
    memset(&svector, 0, sizeof(svector));
    memset(&vector, 0, sizeof(vector));
    CHECK(jon_plumbgeneral(
              &svector, &svector, NULL, 0, &vector) == 0);
    CHECK(jpb_render(
              &matrix,
              NULL,
              &marker,
              0,
              NULL,
              0,
              0,
              0,
              0,
              0,
              0) == &marker);
    spack_frustrum(&matrix, &svector, &vector);
    spackdivver_frustrum(&matrix, &svector, &vector);
    return 0;
}

static int test_wank_check_triangle_height(void)
{
    int32_t normals[16];
    int32_t *old_leveldata = leveldata;
    _svector verts[3] = {
        {0, 50, 100, 0},
        {0, 50, 0, 0},
        {100, 50, 0, 0}
    };
    uint32_t polygons[2];
    int32_t *selected = (int32_t *)polygons;
    VECTOR pos = {10, 900, 10, 0};

    memset(normals, 0, sizeof(normals));
    leveldata = normals;
    normals[5] = 256 << 10;
    polygons[0] = UINT32_C(0x40000004);
    polygons[1] =
        (UINT32_C(1) << 5) |
        (UINT32_C(2) << 10) |
        UINT32_C(0x00100000);

    CHECK(jpb_JonnyWankCheck(verts, &selected, &pos) == 50);
    CHECK(selected == (int32_t *)polygons);

    pos.vx = 120;
    selected = (int32_t *)polygons;
    CHECK(jpb_JonnyWankCheck(verts, &selected, &pos) == INT32_MIN);

    leveldata = old_leveldata;
    return 0;
}

static int test_plumbline_thin_library_cube(void)
{
    enum {
        MAP_WORDS = 32700,
        CELL_INDEX = 128 + 127 * 256,
        CUBE_INDEX = 32,
        LIB_INDEX = 64,
        NORMAL_INDEX = 100
    };
    int32_t *storage =
        (int32_t *)calloc(MAP_WORDS + 4, sizeof(*storage));
    int32_t *mapbase;
    int32_t *old_leveldata = leveldata;
    VECTOR pos = {128, 100, 128, 0};
    _jheightstuff results = {
        (int32_t *)(uintptr_t)1,
        (int32_t *)(uintptr_t)2,
        (int32_t *)(uintptr_t)3
    };

    CHECK(storage != NULL);
    mapbase = storage + 4;
    mapbase[-2] = 128 << 10;
    mapbase[CELL_INDEX] =
        (int32_t)(UINT32_C(0x80000000) | CUBE_INDEX);
    mapbase[CUBE_INDEX] = (int32_t)UINT32_C(0x48000000);
    mapbase[CUBE_INDEX + 1] = 0;
    mapbase[CUBE_INDEX + 2] =
        (int32_t)(UINT32_C(0x40000000) | LIB_INDEX);
    mapbase[LIB_INDEX] = 0;
    mapbase[LIB_INDEX + 1] = 0;
    mapbase[LIB_INDEX + 2] =
        (int32_t)(UINT32_C(0x40000000) | NORMAL_INDEX);
    mapbase[LIB_INDEX + 3] =
        UINT32_C(3) |
        (UINT32_C(1) << 5) |
        (UINT32_C(2) << 10) |
        UINT32_C(0x00100000);
    mapbase[NORMAL_INDEX + 1] = 256 << 10;
    leveldata = mapbase;

    CHECK(
        jon_plumbline(mapbase, NULL, &pos, -100, &results) == 0);
    CHECK(results.cube == &mapbase[CUBE_INDEX]);
    CHECK(results.entry == &mapbase[CUBE_INDEX + 2]);
    CHECK(results.poly == &mapbase[LIB_INDEX + 2]);

    pos.vx = 0x10000;
    results.cube = (int32_t *)(uintptr_t)1;
    CHECK(
        jon_plumbline(mapbase, NULL, &pos, -100, &results) == 0);
    CHECK(results.cube == NULL);

    leveldata = old_leveldata;
    free(storage);
    return 0;
}

static int test_environment_effect_exception(void)
{
    playerObject old_player = gaPlayerData[0];
    Motion motion;
    Motion *current_motion = &motion;

    memset(&gaPlayerData[0], 0, sizeof(gaPlayerData[0]));
    memset(&motion, 0, sizeof(motion));
    gaPlayerData[0].playerID = 15;
    gaPlayerData[0].pMotion = &current_motion;
    CHECK(ExtraCharacterEnvironmentEffectExceptions() == 0);

    motion.Damage = 1;
    CHECK(ExtraCharacterEnvironmentEffectExceptions() == 1);

    motion.Damage = 0;
    gaPlayerData[0].playerID = 0x1e;
    CHECK(ExtraCharacterEnvironmentEffectExceptions() == 1);

    gaPlayerData[0].pMotion = NULL;
    CHECK(ExtraCharacterEnvironmentEffectExceptions() == 0);
    gaPlayerData[0] = old_player;
    return 0;
}

static int test_hits_hit_and_matching_channel_propagation(void)
{
    enum {
        MAP_WORDS = 700,
        MAIN_ENTRY = 10,
        MAIN_EVENT_OFFSET = 60,
        NEIGHBOR_CELL = 258,
        NEIGHBOR_CUBE = 100,
        NEIGHBOR_ENTRY = 102,
        NEIGHBOR_EVENT_OFFSET = 70
    };
    int32_t map[MAP_WORDS];
    int32_t coords[8] = {0};
    int32_t *old_leveldata = leveldata;
    playerObject old_player = gaPlayerData[0];
    int old_level = (int)(int8_t)LevelSelect;
    uint16_t old_event3 = eventarray[0][3];
    uint16_t old_event4 = eventarray[0][4];
    uint32_t main_event =
        (UINT32_C(3) << 25) |
        UINT32_C(0x00400000) |
        (UINT32_C(1) << 18) |
        UINT32_C(7);
    uint32_t neighbor_event =
        (UINT32_C(4) << 25) |
        UINT32_C(0x00400000) |
        UINT32_C(9);
    int n;

    memset(map, 0, sizeof(map));
    memset(&gaPlayerData[0], 0, sizeof(gaPlayerData[0]));
    leveldata = map;
    map[MAIN_ENTRY] = MAIN_EVENT_OFFSET;
    map[MAIN_EVENT_OFFSET - 1] = (int32_t)main_event;
    map[MAIN_EVENT_OFFSET] = (int32_t)UINT32_C(0x20000000);
    map[NEIGHBOR_CELL] =
        (int32_t)(UINT32_C(0x80000000) | NEIGHBOR_CUBE);
    map[NEIGHBOR_CUBE] = (int32_t)UINT32_C(0x48000000);
    map[NEIGHBOR_ENTRY] = NEIGHBOR_EVENT_OFFSET;
    map[NEIGHBOR_EVENT_OFFSET - 1] = (int32_t)neighbor_event;
    map[NEIGHBOR_EVENT_OFFSET] =
        (int32_t)UINT32_C(0x20000000);
    clear_eventlist();

    n = HitsHit(
        map, &map[MAIN_ENTRY], 4, 0x101, coords);
    CHECK(n == 2);
    CHECK((uint32_t)coords[0] ==
          ((UINT32_C(3) << 24) | UINT32_C(0x101)));
    CHECK((uint32_t)coords[1] ==
          ((UINT32_C(4) << 24) |
           (uint32_t)NEIGHBOR_CELL));
    CHECK(*(uint16_t *)(void *)&map[MAIN_ENTRY] == 7);
    CHECK(*(uint16_t *)(void *)&map[NEIGHBOR_ENTRY] == 9);
    CHECK(eventlist_start[2] == MAIN_ENTRY);
    CHECK(eventlist_start[3] == MAIN_EVENT_OFFSET);
    CHECK(eventlist_start[4] == NEIGHBOR_ENTRY);
    CHECK(eventlist_start[5] == NEIGHBOR_EVENT_OFFSET);

    memset(map, 0, sizeof(map));
    memset(coords, 0, sizeof(coords));
    map[MAIN_ENTRY] = MAIN_EVENT_OFFSET;
    map[MAIN_EVENT_OFFSET - 1] =
        (int32_t)((UINT32_C(2) << 18) | UINT32_C(5));
    map[MAIN_EVENT_OFFSET] = (int32_t)UINT32_C(0x20000000);
    CHECK(
        HitsHit(
            map, &map[MAIN_ENTRY], 4, 0x101, coords) == 0);
    CHECK(map[MAIN_ENTRY] == MAIN_EVENT_OFFSET);

    memset(map, 0, sizeof(map));
    map[MAIN_ENTRY] = MAIN_EVENT_OFFSET;
    map[MAIN_EVENT_OFFSET - 1] = (int32_t)main_event;
    map[MAIN_EVENT_OFFSET] = (int32_t)UINT32_C(0x20000000);
    map[NEIGHBOR_CELL] =
        (int32_t)(UINT32_C(0x80000000) | NEIGHBOR_CUBE);
    map[NEIGHBOR_CUBE] = (int32_t)UINT32_C(0x48000000);
    map[NEIGHBOR_ENTRY] = NEIGHBOR_EVENT_OFFSET;
    map[NEIGHBOR_EVENT_OFFSET - 1] = (int32_t)neighbor_event;
    map[NEIGHBOR_EVENT_OFFSET] =
        (int32_t)UINT32_C(0x20000000);
    clear_eventlist();
    LevelSelect = 0;
    eventarray[0][3] = 0;
    eventarray[0][4] = 0;
    {
        VECTOR worldpos = {0x7fff, 0, -0x7e00, 0};

        CHECK(BlowUp(&map[MAIN_ENTRY], &worldpos, 4) == 2);
        CHECK((uint32_t)((int32_t *)(void *)gaScratch)[0] ==
              ((UINT32_C(3) << 24) | UINT32_C(0x101)));
        CHECK((uint32_t)((int32_t *)(void *)gaScratch)[1] ==
              ((UINT32_C(4) << 24) |
               (uint32_t)NEIGHBOR_CELL));
    }

    eventarray[0][3] = old_event3;
    eventarray[0][4] = old_event4;
    LevelSelect = (char)old_level;
    gaPlayerData[0] = old_player;
    leveldata = old_leveldata;
    return 0;
}

static int test_block_buster_nearby_cube_scan(void)
{
    enum {
        MAP_WORDS = 700,
        CELL_INDEX = 1 * 256 + 1,
        CUBE_INDEX = 100,
        ENTRY_INDEX = CUBE_INDEX + 2,
        EVENT_OFFSET = 60
    };
    int32_t map[MAP_WORDS];
    int32_t *old_leveldata = leveldata;
    int32_t old_mapyend = mapyend;
    playerObject old_player = gaPlayerData[0];
    int cubeshite =
        (3 << 16) | (2 << 8) | 2;
    uint32_t eventword =
        (UINT32_C(3) << 25) |
        (UINT32_C(1) << 16) |
        UINT32_C(7);
    int n;

    memset(map, 0, sizeof(map));
    memset(&gaPlayerData[0], 0, sizeof(gaPlayerData[0]));
    leveldata = map;
    mapyend = 4;
    map[CELL_INDEX] =
        (int32_t)(UINT32_C(0x80000000) |
                  CUBE_INDEX);
    map[CUBE_INDEX] =
        (int32_t)UINT32_C(0x48000003);
    map[ENTRY_INDEX] = EVENT_OFFSET;
    map[EVENT_OFFSET - 1] =
        (int32_t)eventword;
    map[EVENT_OFFSET] =
        (int32_t)UINT32_C(0x20000000);
    clear_eventlist();
    memset(gaScratch, 0, sizeof(gaScratch));

    n = BlockBuster(map, 1, cubeshite);
    CHECK(n == 1);
    CHECK((uint32_t)
              ((int32_t *)(void *)gaScratch)[0] ==
          UINT32_C(0x03030101));
    CHECK(
        *(uint16_t *)(void *)&map[ENTRY_INDEX] ==
        7);
    CHECK(eventlist_start[2] == ENTRY_INDEX);
    CHECK(eventlist_start[3] == EVENT_OFFSET);
    restore_events(map);
    CHECK(
        *(uint16_t *)(void *)&map[ENTRY_INDEX] ==
        EVENT_OFFSET);
    CHECK(eventlist_start[0] == 0);
    CHECK(eventlist_start[1] == 0);
    CHECK(eventlist_next == eventlist_start + 2);

    gaPlayerData[0] = old_player;
    mapyend = old_mapyend;
    leveldata = old_leveldata;
    return 0;
}

int main(void)
{
    if (test_makecull_planes() != 0) {
        return 1;
    }
    if (test_getlibpart_integer_vertices() != 0) {
        return 1;
    }
    if (test_getlibpart_float_vertices() != 0) {
        return 1;
    }
    if (test_getlibpart_long_vertices() != 0) {
        return 1;
    }
    if (test_retail_jonny_stubs() != 0) {
        return 1;
    }
    if (test_wank_check_triangle_height() != 0) {
        return 1;
    }
    if (test_plumbline_thin_library_cube() != 0) {
        return 1;
    }
    if (test_environment_effect_exception() != 0) {
        return 1;
    }
    if (test_hits_hit_and_matching_channel_propagation() != 0) {
        return 1;
    }
    if (test_block_buster_nearby_cube_scan() != 0) {
        return 1;
    }

    puts("jonny tests passed");
    return 0;
}
