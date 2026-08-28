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
#include <stdlib.h>
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
    CHECK(!jpb_ShouldDrawFbxMesh(
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

static int test_cube_init_uvs(void)
{
    uint32_t texture_words[(JPB_CUBE_UV_SET_COUNT / 8) * 48] = {0};
    int32_t *saved_texturebase = texturebase;
    const float *uv;

    texture_words[1] = UINT32_C(0xff804001);
    texture_words[2] = UINT32_C(0x000102fe);
    texture_words[48 + 43] = UINT32_C(0x10203040);
    texture_words[48 + 44] = UINT32_C(0x50607080);
    texture_words[(JPB_CUBE_UV_SET_COUNT / 8 - 1) * 48 + 44] =
        UINT32_C(0xa5c3e17f);
    texturebase = (int32_t *)(void *)texture_words;

    CHECK(initUVs() == 0xa5);
    uv = jpb_CubeUVTable();
    CHECK(uv[0] == 1.0f / 256.0f);
    CHECK(uv[1] == 0.75f);
    CHECK(uv[2] == 0.5f);
    CHECK(uv[3] == 1.0f / 256.0f);
    CHECK(uv[4] == 254.0f / 256.0f);
    CHECK(uv[5] == 254.0f / 256.0f);
    CHECK(uv[6] == 1.0f / 256.0f);
    CHECK(uv[7] == 1.0f);
    CHECK(uv[8] == 0.0f);
    CHECK(uv[9] == 1.0f);
    CHECK(uv[JPB_CUBE_UV_FLOATS_PER_SET * 8 + 56] ==
          64.0f / 256.0f);
    CHECK(uv[JPB_CUBE_UV_FLOATS_PER_SET * 8 + 63] ==
          1.0f - 80.0f / 256.0f);
    CHECK(uv[(JPB_CUBE_UV_SET_COUNT - 1) *
                 JPB_CUBE_UV_FLOATS_PER_SET +
             7] ==
          1.0f - 165.0f / 256.0f);

    texturebase = saved_texturebase;
    return 0;
}

static int test_gpu_clut_cache(void)
{
    int32_t buffer[1024] = {0};

    buffer[1023] = (int32_t)UINT32_C(0x4d6e6f4a);
    buffer[1022] = 1;
    CHECK(GPUcluts(NULL, buffer) == 1);
    buffer[1022] = 0;
    CHECK(GPUcluts(NULL, buffer) == 0);
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

static int test_fbx_uv_scroll(void)
{
    float u;
    float v;

    jpb_LevelFbxUvScroll(1, "belt.bmp", &u, &v);
    CHECK(close_float(u, 0.082f));
    CHECK(v == 0.0f);
    jpb_LevelFbxUvScroll(9, "t_water.tga", &u, &v);
    CHECK(close_float(u, 0.001f));
    CHECK(close_float(v, 0.001f));
    jpb_LevelFbxUvScroll(9, "t_waterStatic.tga", &u, &v);
    CHECK(u == 0.0f);
    CHECK(close_float(v, -0.1f));
    jpb_LevelFbxUvScroll(1, "other.tga", &u, &v);
    CHECK(u == 0.0f);
    CHECK(v == 0.0f);
    jpb_LevelFbxUvScroll(1, NULL, NULL, NULL);
    return 0;
}

typedef struct CubePolygonCapture {
    int calls;
    _Material *material;
    uint32_t material_flags;
    int vertex_count;
    JPBScreenPolyVertex vertices[4];
    int no_scale;
} CubePolygonCapture;

static void capture_cube_polygon(
    void *user_data,
    _Material *material,
    uint32_t material_flags,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale)
{
    CubePolygonCapture *capture = (CubePolygonCapture *)user_data;

    ++capture->calls;
    capture->material = material;
    capture->material_flags = material_flags;
    capture->vertex_count = vertex_count;
    memcpy(
        capture->vertices,
        vertices,
        (size_t)vertex_count * sizeof(*vertices));
    capture->no_scale = no_scale;
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

static int test_plot_some_cubes(void)
{
    enum {
        MAP_WORDS = 65536,
        CUBE_INDEX = 100,
        LIBRARY_INDEX = 1000,
        TEXTURE_WORDS = (JPB_CUBE_UV_SET_COUNT / 8) * 48
    };
    int32_t *storage =
        (int32_t *)calloc(MAP_WORDS + 4, sizeof(*storage));
    int32_t *mapbase;
    int32_t texture_words[TEXTURE_WORDS] = {0};
    int32_t palette[256] = {0};
    _Material material = {0};
    CubePolygonCapture capture = {0};
    int32_t *saved_leveldata = leveldata;
    int32_t *saved_texturebase = texturebase;
    int32_t *saved_colorbase = colorbase;
    int32_t saved_mapyend = mapyend;
    _Material *saved_leveltexture[4];
    MATRIX saved_camera_matrix = CameraMatrix;
    _winmat saved_winmatrix = globalwinmatrix;
    Camera saved_camera = gCamera;

    CHECK(storage != NULL);
    mapbase = storage + 4;
    mapbase[0] =
        (int32_t)(UINT32_C(0x80000000) | CUBE_INDEX);
    mapbase[CUBE_INDEX] = (int32_t)UINT32_C(0x48000000);
    mapbase[CUBE_INDEX + 1] = 0;
    mapbase[CUBE_INDEX + 2] = LIBRARY_INDEX;
    mapbase[LIBRARY_INDEX] = 0;
    mapbase[LIBRARY_INDEX + 1] = 0;
    mapbase[LIBRARY_INDEX + 2] =
        (int32_t)UINT32_C(0x40000000);
    mapbase[LIBRARY_INDEX + 3] =
        (int32_t)(UINT32_C(1) << 5 |
                  UINT32_C(2) << 10 |
                  UINT32_C(3) << 15);

    texture_words[1] = (int32_t)UINT32_C(0x40400000);
    texture_words[2] = (int32_t)UINT32_C(0xc0c08080);
    memcpy(saved_leveltexture, leveltexture, sizeof(saved_leveltexture));
    memset(leveltexture, 0, sizeof(saved_leveltexture));
    material.texture = &material;
    material.flags = UINT32_C(0x12345678);
    leveltexture[0] = &material;
    leveldata = mapbase;
    texturebase = texture_words;
    colorbase = palette + 255;
    mapyend = 1;
    memset(&gCamera, 0, sizeof(gCamera));
    memset(&CameraMatrix, 0, sizeof(CameraMatrix));
    CameraMatrix.m[0][0] = 1.0f;
    CameraMatrix.m[1][1] = 1.0f;
    CameraMatrix.m[2][2] = 1.0f;
    memset(&globalwinmatrix, 0, sizeof(globalwinmatrix));
    globalwinmatrix.m[0][0] = 1.0f;
    globalwinmatrix.m[1][1] = 1.0f;
    globalwinmatrix.m[2][2] = 1.0f;
    CHECK(initUVs() == 0);

    jpb_WHookSetScreenPolyHook(capture_cube_polygon, &capture);
    CHECK(plotsomecubes(0, 0, 1, 0) == 0);
    jpb_WHookSetScreenPolyHook(NULL, NULL);

    CHECK(capture.calls == 1);
    CHECK(capture.material == &material);
    CHECK(capture.material_flags == UINT32_C(0x12345678));
    CHECK(capture.vertex_count == 4);
    CHECK(capture.no_scale == 1);
    CHECK(capture.vertices[0].x == -32512.0f);
    CHECK(capture.vertices[0].y == 0.0f);
    CHECK(capture.vertices[0].z == -32512.0f);
    CHECK(capture.vertices[1].x == -32768.0f);
    CHECK(capture.vertices[1].z == -32512.0f);
    CHECK(capture.vertices[2].x == -32512.0f);
    CHECK(capture.vertices[2].z == -32256.0f);
    CHECK(capture.vertices[3].x == -32768.0f);
    CHECK(capture.vertices[3].z == -32256.0f);
    CHECK(capture.vertices[0].argb == UINT32_C(0x00ff00ff));
    CHECK(capture.vertices[0].tu == 0.0f);
    CHECK(capture.vertices[0].tv == 1.0f);
    CHECK(capture.vertices[1].tu == 0.25f);
    CHECK(capture.vertices[1].tv == 0.75f);
    CHECK(capture.vertices[2].tu == 0.5f);
    CHECK(capture.vertices[2].tv == 0.5f);
    CHECK(capture.vertices[3].tu == 0.75f);
    CHECK(capture.vertices[3].tv == 0.25f);

    leveldata = saved_leveldata;
    texturebase = saved_texturebase;
    colorbase = saved_colorbase;
    mapyend = saved_mapyend;
    memcpy(leveltexture, saved_leveltexture, sizeof(saved_leveltexture));
    CameraMatrix = saved_camera_matrix;
    globalwinmatrix = saved_winmatrix;
    gCamera = saved_camera;
    free(storage);
    return 0;
}

int main(void)
{
    CHECK(test_exact_globals() == 0);
    CHECK(test_streets_cullmesh() == 0);
    CHECK(test_cube_init_visibility() == 0);
    CHECK(test_cube_init_uvs() == 0);
    CHECK(test_gpu_clut_cache() == 0);
    CHECK(test_streets_jpx_cull_map() == 0);
    CHECK(test_path_lookup() == 0);
    CHECK(test_streets_spawn_alignment() == 0);
    CHECK(test_custom_level_transforms() == 0);
    CHECK(test_fbx_uv_scroll() == 0);
    CHECK(test_cube_new_world_render() == 0);
    CHECK(test_plot_some_cubes() == 0);
    puts("level world tests passed");
    return 0;
}
