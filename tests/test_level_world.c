#include "jpb/level_world.h"
#include "jpb/camera.h"
#include "jpb/game.h"
#include "jpb/jonny.h"
#include "jpb/jonnywin.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/streets_cull_map.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <math.h>
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
    return fabsf(left - right) < 0.01f;
}

static int test_exact_globals(void)
{
    CHECK(sizeof(sLevelNames) / sizeof(sLevelNames[0]) ==
          JPB_LEVEL_NAME_COUNT);
    CHECK(sizeof(level_offset) / sizeof(level_offset[0]) ==
          JPB_LEVEL_COUNT);
    CHECK(sizeof(level_scale) / sizeof(level_scale[0]) ==
          JPB_LEVEL_COUNT);
    CHECK(sizeof(startPos) == 416);
    CHECK(strcmp(sLevelNames[8], "streets") == 0);
    CHECK(strcmp(sLevelNames[25], "arena") == 0);
    CHECK(strcmp(sLevelNames[27], "june") == 0);
    CHECK(close_float(level_offset[8][0], 1.0f));
    CHECK(close_float(level_offset[8][1], -0.5f));
    CHECK(close_float(level_scale[8][0], 256.0f));
    CHECK(close_float(level_scale[6][0], -256.0f));
    CHECK(close_float(level_scale[15][1], -256.0f));
    CHECK(startPos[8][0].vx == 7);
    CHECK(startPos[8][0].vy == 31);
    CHECK(startPos[8][0].vz == 13);
    CHECK(sizeof(cullmesh) == 128);
    CHECK(cullmesh[0] == 1);
    CHECK(cullmesh[1] == 0);
    return 0;
}

static int test_streets_cullmesh(void)
{
    int32_t saved[JPB_CULL_MESH_COUNT];

    memcpy(saved, cullmesh, sizeof(saved));
    memset(cullmesh, 0, sizeof(cullmesh));

    CHECK(jpb_StreetsCullMeshIndexFromName("streets_A0") == 0);
    CHECK(jpb_StreetsCullMeshIndexFromName("Wall01_Broken") == 1);
    CHECK(jpb_StreetsCullMeshIndexFromName("Wall01_Solid") == 2);
    CHECK(jpb_StreetsCullMeshIndexFromName("Wall06_Broken") == 11);
    CHECK(jpb_StreetsCullMeshIndexFromName("Wall06_Solid") == 12);
    CHECK(jpb_StreetsCullMeshIndexFromName("Wall12_Broken") == 23);
    CHECK(jpb_StreetsCullMeshIndexFromName("Wall12_Solid") == 24);
    CHECK(jpb_StreetsCullMeshIndexFromName("Wall99_Solid") ==
          JPB_LEVEL_INDEX_NONE);
    CHECK(jpb_StreetsCullMeshIndexFromName("bad") ==
          JPB_LEVEL_INDEX_NONE);
    CHECK(jpb_StreetsCullMeshIndexFromName(NULL) ==
          JPB_LEVEL_INDEX_NONE);

    cullmesh[0] = 1;
    cullmesh[1] = 1;
    cullmesh[2] = 0;
    cullmesh[5] = 1;
    CHECK(jpb_ShouldDrawFbxMesh(
        8, JPB_LEVEL_FBX_PASS_OPAQUE, 25, 0, "streets_A0"));
    CHECK(jpb_ShouldDrawFbxMesh(
        8, JPB_LEVEL_FBX_PASS_TRANSPARENT,
        25, 2, "Wall01_Broken"));
    CHECK(!jpb_ShouldDrawFbxMesh(
        8, JPB_LEVEL_FBX_PASS_GLASS, 25, 1, "Wall01_Solid"));
    CHECK(!jpb_ShouldDrawFbxMesh(
        8, JPB_LEVEL_FBX_PASS_OPAQUE,
        25, 25, "Wall01_Broken"));
    CHECK(!jpb_ShouldDrawFbxMesh(
        8, JPB_LEVEL_FBX_PASS_OPAQUE, 25, 1, "Wall99_Solid"));

    CHECK(jpb_ShouldDrawFbxMesh(
        7, JPB_LEVEL_FBX_PASS_OPAQUE, 31, 5, "unused"));
    CHECK(jpb_ShouldDrawFbxMesh(
        7, JPB_LEVEL_FBX_PASS_OPAQUE, 31, 4, "unused"));
    CHECK(jpb_ShouldDrawFbxMesh(
        7, JPB_LEVEL_FBX_PASS_OPAQUE, 32, 4, "unused"));
    CHECK(jpb_ShouldDrawFbxMesh(
        7, JPB_LEVEL_FBX_PASS_TRANSPARENT, 31, 4, "unused"));
    CHECK(jpb_ShouldDrawFbxMesh(
        7, JPB_LEVEL_FBX_PASS_GLASS, 31, 4, "unused"));
    CHECK(!jpb_ShouldDrawFbxMesh(
        7, (JPBLevelFbxMeshPass)99, 31, 4, "unused"));

    memcpy(cullmesh, saved, sizeof(saved));
    return 0;
}

static int test_cube_init_visibility(void)
{
    int32_t saved_meshes[JPB_CULL_MESH_COUNT];
    JPBCullState saved_cull = cull;
    char saved_level = LevelSelect;
    char saved_tato = tato_wallfrigflag;
    char saved_fed = fed_wallfrigflag;
    size_t index;

    memcpy(saved_meshes, cullmesh, sizeof(saved_meshes));
    memset(&cull, 0, sizeof(cull));
    LevelSelect = 8;
    cube_InitVisibility();
    for (index = 0; index < sizeof(cull.visarray); ++index) {
        CHECK(cull.visarray[index] == 1);
    }
    CHECK(cullmesh[0] == 1);
    for (index = 1; index < 25; ++index) {
        CHECK(cullmesh[index] == ((index & 1u) != 0 ? 0 : 1));
    }
    CHECK(cullmesh[25] == 1);
    CHECK(cullmesh[29] == 1);
    CHECK(cullmesh[31] == 8);

    LevelSelect = 1;
    fed_wallfrigflag = 0;
    cube_InitVisibility();
    CHECK(cullmesh[0] == 1);
    CHECK(cullmesh[1] == 0);
    fed_wallfrigflag = 1;
    cube_InitVisibility();
    CHECK(cullmesh[0] == 0);
    CHECK(cullmesh[1] == 1);

    LevelSelect = 5;
    tato_wallfrigflag = 0;
    cube_InitVisibility();
    CHECK(cullmesh[1] == 0);
    CHECK(cullmesh[2] == 1);
    tato_wallfrigflag = 1;
    cube_InitVisibility();
    CHECK(cullmesh[1] == 1);
    CHECK(cullmesh[2] == 0);

    LevelSelect = 7;
    cube_InitVisibility();
    for (index = 0; index < JPB_CULL_MESH_COUNT; ++index) {
        CHECK(cullmesh[index] == 1);
    }

    memcpy(cullmesh, saved_meshes, sizeof(saved_meshes));
    cull = saved_cull;
    LevelSelect = saved_level;
    tato_wallfrigflag = saved_tato;
    fed_wallfrigflag = saved_fed;
    return 0;
}

static int test_streets_jpx_cull_map(void)
{
    JPBJpxPatchSite site = {0};
    JPBJpxView view = {0};
    int32_t saved[JPB_CULL_MESH_COUNT];

    site.offset = 3351232u;
    CHECK(jpb_StreetsJpxTriangleCullMask(&site, 0) ==
          UINT32_C(0x00000006));
    CHECK(jpb_StreetsJpxTriangleCullMask(&site, 2) ==
          UINT32_C(0x00000004));
    CHECK(jpb_StreetsJpxTriangleCullMask(&site, 3) == 0);
    site.offset = 3368828u;
    CHECK(jpb_StreetsJpxTriangleCullMask(&site, 4) ==
          UINT32_C(0x01000000));
    site.offset = 1;
    CHECK(jpb_StreetsJpxTriangleCullMask(&site, 0) == 0);
    CHECK(jpb_StreetsJpxTriangleCullMask(NULL, 0) == 0);
    CHECK(!jpb_IsMatchedStreetsJpx(&view));
    CHECK(!jpb_IsMatchedStreetsJpx(NULL));

    memcpy(saved, cullmesh, sizeof(saved));
    memset(cullmesh, 0, sizeof(cullmesh));
    CHECK(jpb_StreetsJpxCullMaskVisible(0));
    CHECK(!jpb_StreetsJpxCullMaskVisible(UINT32_C(0x00000006)));
    cullmesh[1] = 1;
    CHECK(jpb_StreetsJpxCullMaskVisible(UINT32_C(0x00000002)));
    CHECK(!jpb_StreetsJpxCullMaskVisible(UINT32_C(0x00000004)));
    CHECK(jpb_StreetsJpxCullMaskVisible(UINT32_C(0x00000006)));
    cullmesh[1] = 0;
    cullmesh[2] = 1;
    CHECK(!jpb_StreetsJpxCullMaskVisible(UINT32_C(0x00000002)));
    CHECK(jpb_StreetsJpxCullMaskVisible(UINT32_C(0x00000004)));
    CHECK(jpb_StreetsJpxCullMaskVisible(UINT32_C(0x00000006)));
    memcpy(cullmesh, saved, sizeof(saved));
    return 0;
}

static int test_path_lookup(void)
{
    CHECK(jpb_LevelIndexFromPath(
              "res/level/jpx/STREETS/STREETS.jpx") == 8);
    CHECK(jpb_LevelIndexFromPath(
              "res\\level\\jpx\\STREETS\\xstreets.jpx") == 8);
    CHECK(jpb_LevelIndexFromPath(
              "res/level/jpx/hangar/modhangar.jpx") == 9);
    CHECK(jpb_LevelIndexFromPath("TRAIN7.JPX") == 21);
    CHECK(jpb_LevelIndexFromPath("unknown.jpx") ==
          JPB_LEVEL_INDEX_NONE);
    CHECK(jpb_LevelIndexFromPath(NULL) == JPB_LEVEL_INDEX_NONE);
    return 0;
}

static int test_streets_spawn_alignment(void)
{
    /*
     * Shipped streets.fbx contains the exact vertex {-120,-97,13}.
     * cube_NewWorldRender maps it to the player-0 startPos calculation:
     * {(128-7)*256, 13*256, (31-127)*256}.
     */
    FVECTOR game;

    CHECK(jpb_LevelTransformFbxVertex(
        8, -120.0f, -97.0f, 13.0f, &game));
    CHECK(game.vx == 30976.0f);
    CHECK(game.vy == 3328.0f);
    CHECK(game.vz == -24576.0f);

    return 0;
}

static int test_custom_level_transforms(void)
{
    FVECTOR game;

    /* Palace uses non-uniform table values rather than the common defaults. */
    CHECK(jpb_LevelTransformFbxVertex(
        4, 5.05857445f, -20.41496119f, -0.13549753f, &game));
    CHECK(close_float(game.vx, (128.0f - 36.0f) * 256.0f));
    CHECK(close_float(game.vy, 13.0f * 256.0f));
    CHECK(close_float(game.vz, (69.0f - 127.0f) * 256.0f));

    /* Coruscant 1 flips both FBX horizontal axes. */
    CHECK(jpb_LevelTransformFbxVertex(
        6, -28.5f, -46.4f, 32.5f, &game));
    CHECK(close_float(game.vx, (128.0f - 28.0f) * 256.0f));
    CHECK(close_float(game.vy, 30.0f * 256.0f));
    CHECK(close_float(game.vz, (50.0f - 127.0f) * 256.0f));

    CHECK(!jpb_LevelTransformFbxVertex(
        JPB_LEVEL_INDEX_NONE, 0.0f, 0.0f, 0.0f, &game));
    CHECK(!jpb_LevelTransformFbxVertex(
        0, 0.0f, 0.0f, 0.0f, NULL));
    return 0;
}

typedef struct LegacyRenderCapture {
    int calls;
    JPBCubeRenderBounds bounds;
} LegacyRenderCapture;

static int capture_legacy_render(
    void *user_data,
    int min_x,
    int min_z,
    int max_x,
    int max_z)
{
    LegacyRenderCapture *capture = (LegacyRenderCapture *)user_data;

    ++capture->calls;
    capture->bounds.minX = min_x;
    capture->bounds.minZ = min_z;
    capture->bounds.maxX = max_x;
    capture->bounds.maxZ = max_z;
    return 73;
}

static void prepare_cube_render_player(
    int player_index,
    int32_t *cube,
    float x,
    float z)
{
    memset(&gaPlayerData[player_index], 0, sizeof(gaPlayerData[player_index]));
    memset(&maPhysicsData[player_index], 0, sizeof(maPhysicsData[player_index]));
    gaPlayerData[player_index].playerRoot.objectID = player_index;
    maPhysicsData[player_index].mapinfo.cube = cube;
    maPhysicsData[player_index].pos.vx = x;
    maPhysicsData[player_index].pos.vz = z;
}

static int test_cube_new_world_render(void)
{
    WorldData world;
    MATRIX matrix = {
        {{1.0f, 0.0f, 0.0f},
         {0.0f, 1.0f, 0.0f},
         {0.0f, 0.0f, 1.0f}},
        {10, 20, 30}
    };
    int32_t map_storage[8] = {0};
    int32_t cube0[2] = {
        (int32_t)UINT32_C(0x04000000),
        9 << 17
    };
    int32_t cube1[2] = {
        (int32_t)UINT32_C(0x04000000),
        7 << 17
    };
    JPBCubeRenderBounds bounds;
    const JPBLevelTransformation *transformation;
    LegacyRenderCapture capture = {0};
    WorldData *saved_world = gpWorld;
    int32_t *saved_leveldata = leveldata;
    char saved_level_select = LevelSelect;
    char saved_players = GameStruct.NumPlayers;
    uint8_t saved_current_level = GameStruct.CurrentLevel;
    char saved_maxdraw = GameStruct.maxdraw;
    Camera saved_camera = gCamera;
    VECTOR saved_camera_position = cameraposition;
    physicsObject saved_physics[2] = {
        maPhysicsData[0], maPhysicsData[1]
    };
    playerObject saved_player[2] = {
        gaPlayerData[0], gaPlayerData[1]
    };
    uint32_t saved_cube_flags = jpb_CubeRuntimeFlags;
    int saved_stairs = streets_reached_stairs;
    MATRIX *saved_world_matrix = worldTURTLEMatrix;

    memset(&world, 0, sizeof(world));
    gpWorld = &world;
    leveldata = map_storage + 4;
    leveldata[-4] = 0;
    leveldata[-2] = 0;
    LevelSelect = 1;
    GameStruct.CurrentLevel = 1;
    GameStruct.NumPlayers = 1;
    GameStruct.maxdraw = 100;
    world.location.vx = 0x7000;
    world.location.vz = -0x6f00;
    prepare_cube_render_player(0, cube0, 0.0f, 0.0f);
    memset(&gCamera, 0, sizeof(gCamera));
    gCamera.viewType = JPB_CAMERA_VIEW_ABSOLUTE_FOCUS;
    gCamera.focus.vx = 100;
    gCamera.focus.vy = 200;
    gCamera.focus.vz = 300;

    cube_NewWorldRender(&matrix);
    jpb_CubeGetLastRenderBounds(&bounds);
    CHECK(bounds.minX == 7);
    CHECK(bounds.minZ == 7);
    CHECK(bounds.maxX == 26);
    CHECK(bounds.maxZ == 26);
    CHECK(worldTURTLEMatrix == &matrix);
    CHECK(close_float(globalwinmatrix.m[0][0], 1.0f));
    CHECK(close_float(globalwinmatrix.m[1][1], 1.0f));
    CHECK(close_float(globalwinmatrix.t[0], 10.0f));
    CHECK(close_float(globalwinmatrix.t[1], 20.0f));
    CHECK(close_float(globalwinmatrix.t[2], 30.0f));
    transformation = jpb_WHookLevelTransformation();
    CHECK(close_float(transformation->world[0][0], -1.0f));
    CHECK(close_float(transformation->world[1][1], 0.0f));
    CHECK(close_float(transformation->world[1][2], 1.0f));
    CHECK(close_float(transformation->world[2][1], 1.0f));
    CHECK(close_float(transformation->world[2][2], 0.0f));
    CHECK(close_float(transformation->world[3][0], 156.0f));
    CHECK(close_float(transformation->world[3][1], -200.0f));
    CHECK(close_float(transformation->world[3][2], -44.0f));
    CHECK(close_float(transformation->scale[0], 256.0f));
    CHECK(close_float(transformation->scale[1], 256.0f));
    CHECK(close_float(transformation->scale[2], 256.0f));
    CHECK(close_float(transformation->scale[3], 1.0f));

    GameStruct.NumPlayers = 2;
    GameStruct.maxdraw = 12;
    cube0[1] = 5 << 17;
    prepare_cube_render_player(
        0, cube0, (float)(0x80ff - (10 << 8)),
        (float)((20 << 8) - 0x7f00));
    prepare_cube_render_player(
        1, cube1, (float)(0x80ff - (30 << 8)),
        (float)((40 << 8) - 0x7f00));
    cube_NewWorldRender(&matrix);
    jpb_CubeGetLastRenderBounds(&bounds);
    CHECK(bounds.minX == 9);
    CHECK(bounds.minZ == 19);
    CHECK(bounds.maxX == 33);
    CHECK(bounds.maxZ == 43);

    GameStruct.NumPlayers = 1;
    GameStruct.maxdraw = 100;
    world.currentDolly = 0;
    world.aDolly[0].flags = UINT32_C(0x00000400);
    world.aDolly[0].offset.vx = 0x80ff - (50 << 8);
    world.aDolly[0].offset.vz = (60 << 8) - 0x7f00;
    cube_NewWorldRender(&matrix);
    jpb_CubeGetLastRenderBounds(&bounds);
    CHECK(bounds.minX == 34);
    CHECK(bounds.minZ == 44);
    CHECK(bounds.maxX == 67);
    CHECK(bounds.maxZ == 77);
    world.aDolly[0].flags = 0;

    GameStruct.NumPlayers = 1;
    GameStruct.CurrentLevel = 8;
    GameStruct.maxdraw = 127;
    LevelSelect = 8;
    world.location.vx = 0x80ff - (77 << 8);
    world.location.vy = 15 << 8;
    world.location.vz = (119 << 8) - 0x7f00;
    cameraposition.vx = 0x80ff - (20 << 8);
    jpb_CubeRuntimeFlags = 0;
    streets_reached_stairs = 0;
    cube_NewWorldRender(&matrix);
    jpb_CubeGetLastRenderBounds(&bounds);
    CHECK((jpb_CubeRuntimeFlags & UINT32_C(8)) != 0);
    CHECK(streets_reached_stairs == 1);
    CHECK(bounds.minX == 20);
    CHECK(bounds.minZ == 116);
    CHECK(bounds.maxX == 68);
    CHECK(bounds.maxZ == 141);

    GameStruct.CurrentLevel = 12;
    LevelSelect = 12;
    world.location.vx = 0x7000;
    world.location.vy = 0;
    world.location.vz = -0x6f00;
    jpb_CubeSetLegacyRenderHook(capture_legacy_render, &capture);
    cube_NewWorldRender(&matrix);
    jpb_CubeGetLastRenderBounds(&bounds);
    CHECK(capture.calls == 1);
    CHECK(memcmp(&capture.bounds, &bounds, sizeof(bounds)) == 0);
    CHECK(plotsomecubes(1, 2, 3, 4) == 73);
    CHECK(capture.calls == 2);

    jpb_CubeSetLegacyRenderHook(NULL, NULL);
    gpWorld = saved_world;
    leveldata = saved_leveldata;
    LevelSelect = saved_level_select;
    GameStruct.NumPlayers = saved_players;
    GameStruct.CurrentLevel = saved_current_level;
    GameStruct.maxdraw = saved_maxdraw;
    gCamera = saved_camera;
    cameraposition = saved_camera_position;
    maPhysicsData[0] = saved_physics[0];
    maPhysicsData[1] = saved_physics[1];
    gaPlayerData[0] = saved_player[0];
    gaPlayerData[1] = saved_player[1];
    jpb_CubeRuntimeFlags = saved_cube_flags;
    streets_reached_stairs = saved_stairs;
    worldTURTLEMatrix = saved_world_matrix;
    return 0;
}

int main(void)
{
    CHECK(test_exact_globals() == 0);
    CHECK(test_streets_cullmesh() == 0);
    CHECK(test_cube_init_visibility() == 0);
    CHECK(test_streets_jpx_cull_map() == 0);
    CHECK(test_path_lookup() == 0);
    CHECK(test_streets_spawn_alignment() == 0);
    CHECK(test_custom_level_transforms() == 0);
    CHECK(test_cube_new_world_render() == 0);
    puts("level world tests passed");
    return 0;
}
