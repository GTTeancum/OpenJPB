#include "jpb/intersec.h"

#include "jpb/bmd.h"
#include "jpb/camera.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/jonny.h"
#include "jpb/physics.h"
#include "jpb/world.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

static int test_clip_to_frustrum(void)
{
    FVECTOR4 frustrum[5] = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 4.0f},
        {0.0f, 0.0f, 1.0f, 1.0f},
        {-1.0f, 0.0f, 0.0f, -5.0f},
        {0.0f, -1.0f, 0.0f, -10.0f}
    };
    FVECTOR pos = {2.0f, 3.0f, 4.0f};
    _svector svpos = {2, 3, 4, 0};
    int distances[5] = {0};

    CHECK(cliptofrustrum(frustrum, &pos, 3, distances) == 24);
    CHECK(distances[0] == 2);
    CHECK(distances[1] == -1);
    CHECK(distances[2] == 3);
    CHECK(distances[3] == 3);
    CHECK(distances[4] == 7);
    CHECK(cliptofrustrum(frustrum, &pos, 3, NULL) == 24);
    CHECK(cliptofrustrumSV(frustrum, &svpos, 3, distances) == 24);
    CHECK(distances[0] == 2);
    CHECK(distances[1] == -1);
    CHECK(distances[2] == 3);
    CHECK(distances[3] == 3);
    CHECK(distances[4] == 7);

    frustrum[0].vw = NAN;
    CHECK(cliptofrustrum(frustrum, &pos, 3, distances) == 8);
    CHECK(distances[0] == INT32_MIN);
    return 0;
}

static int test_line_and_plane(void)
{
    FVECTOR4 plane = {0.0f, 1.0f, 0.0f, 5.0f};
    FVECTOR start = {2.0f, 15.0f, 3.0f};
    FVECTOR down = {0.0f, -1.0f, 0.0f};
    FVECTOR up = {0.0f, 1.0f, 0.0f};

    CHECK(LineAndPlane(&plane, &start, &down, 20.0f, 0) == 10.0f);
    CHECK(LineAndPlane(&plane, &start, &down, 7.0f, 0) == 7.0f);
    CHECK(LineAndPlane(&plane, &start, &down, 20.0f, 3) == 7.0f);
    CHECK(LineAndPlane(&plane, &start, &up, 20.0f, 0) == 20.0f);
    start.vy = 2.0f;
    CHECK(LineAndPlane(&plane, &start, &down, 20.0f, 0) == 0.0f);
    start.vy = NAN;
    CHECK(LineAndPlane(&plane, &start, &down, 20.0f, 0) == 20.0f);
    return 0;
}

static int test_static_map_walk_height(void)
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
    VECTOR pos = {128, 300, 128, 0};
    VECTOR normal = {1, 2, 3, 4};
    FVECTOR float_pos = {128.9f, 300.9f, 128.9f};

    CHECK(storage != NULL);
    mapbase = storage + 4;
    mapbase[-2] = 128 << 10;
    mapbase[CELL_INDEX] =
        (int32_t)(UINT32_C(0x80000000) | CUBE_INDEX);
    mapbase[CUBE_INDEX] = (int32_t)UINT32_C(0x48000001);
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
        intersec_FindWalkHeight(&pos, &normal, NULL, 0) == 256);
    CHECK(normal.vx == 0);
    CHECK(normal.vy == 2048);
    CHECK(normal.vz == 0);
    CHECK(
        intersec_FindWalkHeightFV(
            &float_pos, NULL, NULL, 0) == 256);

    pos.vx = 0x10000;
    CHECK(
        intersec_FindWalkHeight(&pos, NULL, NULL, 0) == -0x7ff8);

    leveldata = old_leveldata;
    free(storage);
    return 0;
}

static int test_static_map_raycast(void)
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
    int old_mapyend = mapyend;
    physicsObject old_physics[JPB_PHYSICS_CAPACITY];
    FVECTOR start = {128.0f, 300.0f, 128.0f};
    FVECTOR direction = {0.0f, -1.0f, 0.0f};
    FVECTOR hitpoint = {0.0f, 0.0f, 0.0f};
    _svector svstart = {128, 300, 128, 0};
    _svector svdirection = {0, -4096, 0, 0};
    _svector svhit = {0, 0, 0, 0};
    VECTOR moved = {128, 300, 128, 0};
    _mvector movement = {0, -4096, 0, 100};
    _svector hitnormal = {1, 2, 3, 4};
    float old_frame_rate = fGlobalFrameRate;
    int old_level = (int)(int8_t)LevelSelect;
    int *cube = NULL;
    int *entry = NULL;
    int *poly = NULL;
    int len = 0;
    int i;

    CHECK(storage != NULL);
    memcpy(old_physics, maPhysicsData, sizeof(old_physics));
    for (i = 0; i < JPB_PHYSICS_CAPACITY; ++i) {
        memset(&maPhysicsData[i], 0, sizeof(maPhysicsData[i]));
        maPhysicsData[i].physicsRoot.objectID = -1;
    }
    mapbase = storage + 4;
    mapbase[-2] = 128 << 10;
    mapbase[CELL_INDEX] =
        (int32_t)(UINT32_C(0x80000000) | CUBE_INDEX);
    mapbase[CUBE_INDEX] = (int32_t)UINT32_C(0x48000001);
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
    mapyend = 128;

    CHECK(RaycastCheck(
              &start,
              &direction,
              100.0f,
              &cube,
              &entry,
              &poly,
              &len,
              &hitpoint) == 1);
    CHECK(cube == &mapbase[CUBE_INDEX + 2]);
    CHECK(entry == &mapbase[CUBE_INDEX + 2]);
    CHECK(poly == &mapbase[LIB_INDEX + 2]);
    CHECK(len == 44);
    CHECK(hitpoint.vx == 128.0f);
    CHECK(hitpoint.vy == 256.0f);
    CHECK(hitpoint.vz == 128.0f);

    CHECK(RaycastCheckSV(
              &svstart,
              &svdirection,
              100,
              &cube,
              &entry,
              &poly,
              &len,
              &svhit) == 1);
    CHECK(len == 44);
    CHECK(svhit.vx == 128);
    CHECK(svhit.vy == 256);
    CHECK(svhit.vz == 128);

    fGlobalFrameRate = 1.0f;
    LevelSelect = 1;
    CHECK(MoveObjectNormal(
              &movement,
              &moved,
              MEDIUM_HIT,
              &hitnormal) == 0);
    CHECK(moved.vx == 128);
    CHECK(moved.vy == 256);
    CHECK(moved.vz == 128);
    CHECK(hitnormal.vx == 0);
    CHECK(hitnormal.vy == 2048);
    CHECK(hitnormal.vz == 0);

    LevelSelect = (int8_t)old_level;
    fGlobalFrameRate = old_frame_rate;
    leveldata = old_leveldata;
    mapyend = old_mapyend;
    memcpy(maPhysicsData, old_physics, sizeof(old_physics));
    free(storage);
    return 0;
}

static int test_dynamic_solid_raycast_and_move(void)
{
    enum { MAP_WORDS = 256 * 256 };
    int32_t *storage =
        (int32_t *)calloc(MAP_WORDS + 4, sizeof(*storage));
    int32_t *old_leveldata = leveldata;
    int old_mapyend = mapyend;
    physicsObject old_physics[JPB_PHYSICS_CAPACITY];
    float old_frame_rate = fGlobalFrameRate;
    int old_fixed_frame_rate = gGlobalFrameRate;
    int old_level = (int)(int8_t)LevelSelect;
    geomData geometry;
    _solid solid;
    _svector vertices[4] = {
        {-10, 0, -10, 0},
        {10, 0, -10, 0},
        {10, 0, 10, 0},
        {-10, 0, 10, 0}
    };
    _svector normals[1] = {{0, 4096, 0, 0}};
    int16_t indices[4] = {0, 1, 2, 3};
    FVECTOR start = {0.0f, 10.0f, 0.0f};
    FVECTOR direction = {0.0f, -1.0f, 0.0f};
    FVECTOR hitpoint;
    VECTOR curpos = {0, 10, 0, 0};
    _mvector movement = {0, -4096, 0, 20};
    _svector hitnormal = {1, 2, 3, 4};
    int *cube = (int *)(uintptr_t)1;
    int *entry = NULL;
    int *poly = NULL;
    int len = 0;
    int i;

    CHECK(storage != NULL);
    memcpy(old_physics, maPhysicsData, sizeof(old_physics));
    memset(&geometry, 0, sizeof(geometry));
    memset(&solid, 0, sizeof(solid));
    for (i = 0; i < JPB_PHYSICS_CAPACITY; ++i) {
        memset(&maPhysicsData[i], 0, sizeof(maPhysicsData[i]));
        maPhysicsData[i].physicsRoot.objectID = -1;
    }
    pointerRegistry_Reset();
    geometry.numFaces = 1;
    geometry.pIndex = addPtr(indices, JPB_POINTER_ARRAY_INDEX);
    CHECK(geometry.pIndex >= 0);
    solid.geometry = &geometry;
    solid.coords = vertices;
    solid.normals = normals;
    maPhysicsData[5].physicsRoot.objectID = 5;
    maPhysicsData[5].solid = &solid;
    leveldata = storage + 4;
    mapyend = 256;

    CHECK(RaycastCheck(
              &start,
              &direction,
              20.0f,
              &cube,
              &entry,
              &poly,
              &len,
              &hitpoint) == 4);
    CHECK(cube == NULL);
    CHECK(entry == (int *)(void *)&solid);
    CHECK(poly == (int *)(void *)&normals[0]);
    CHECK(len == 10);
    CHECK(hitpoint.vx == 0.0f);
    CHECK(hitpoint.vy == 0.0f);
    CHECK(hitpoint.vz == 0.0f);

    fGlobalFrameRate = 1.0f;
    gGlobalFrameRate = 4096;
    LevelSelect = 1;
    CHECK(MoveObjectNormal(
              &movement,
              &curpos,
              SMALL_HIT,
              &hitnormal) == 0);
    CHECK(curpos.vx == 0);
    CHECK(curpos.vy == 0);
    CHECK(curpos.vz == 0);
    CHECK(hitnormal.vx == 0);
    CHECK(hitnormal.vy == 4096);
    CHECK(hitnormal.vz == 0);

    curpos.vy = 10;
    CHECK(MoveObject(&movement, &curpos, SMALL_HIT) == 0);
    CHECK(curpos.vy == 0);

    LevelSelect = (int8_t)old_level;
    fGlobalFrameRate = old_frame_rate;
    gGlobalFrameRate = old_fixed_frame_rate;
    leveldata = old_leveldata;
    mapyend = old_mapyend;
    memcpy(maPhysicsData, old_physics, sizeof(old_physics));
    pointerRegistry_Reset();
    free(storage);
    return 0;
}

int main(void)
{
    if (test_clip_to_frustrum() != 0 ||
        test_line_and_plane() != 0 ||
        test_static_map_walk_height() != 0 ||
        test_static_map_raycast() != 0 ||
        test_dynamic_solid_raycast_and_move() != 0) {
        return 1;
    }

    puts("intersec tests passed");
    return 0;
}
