/*
 * REVIEWED RECONSTRUCTION.
 * PDB module: 0019
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\cube.obj
 * Primary source: W:\SWJediPowerBattles\Work\cube.c
 * Compiler language: c
 * Emitted procedures: 9
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/cube.h"
#include "jpb/camera.h"
#include "jpb/game.h"
#include "jpb/intersec.h"
#include "jpb/jonny.h"
#include "jpb/jonnywin.h"
#include "jpb/level_world.h"
#include "jpb/linkstubs.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/prim.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <string.h>

/* Exact cube.c module locals at RVAs 0x50CEE4..0x50CEF0. */
static int minx;
static int maxx;
static int minz;
static int maxz;
/* Exact cube.c local at RVA 0x50DB00 (0x8000 bytes). */
typedef struct CubeUV4 {
    float uv[JPB_CUBE_UV_FLOATS_PER_SET];
} CubeUV4;

static FVECTOR pcache[256];
static CubeUV4 fUV[JPB_CUBE_UV_SET_COUNT];
static FVECTOR4 cubepos[2];
static uint32_t libpartcols[32];
static _Material *oldspice;

static const int32_t uvredir[16][4] = {
    {0, 2, 4, 6}, {0, 2, 4, 6}, {0, 2, 4, 6}, {0, 2, 4, 6},
    {4, 6, 0, 2}, {4, 6, 0, 2}, {4, 6, 0, 2}, {4, 6, 0, 2},
    {0, 6, 4, 4}, {0, 2, 4, 4}, {2, 6, 0, 0}, {2, 6, 4, 4},
    {4, 2, 0, 0}, {4, 6, 0, 0}, {6, 2, 4, 4}, {6, 2, 0, 0}
};

static JPBCubeRenderBounds jpb_last_render_bounds;

static int cube_player_is_active(int index)
{
    playerObject *player = &gaPlayerData[index];

    return player->playerRoot.objectID != -1 &&
           obj_gCheckObjectFlag(
               &player->playerRoot, 0, UINT32_C(0x20)) == 0 &&
           (player->pFlags & UINT32_C(0x00040200)) == 0;
}

static int cube_world_x_cell(int world_x)
{
    return (int)(int16_t)(
        (UINT32_C(0x80ff) - (uint32_t)world_x) >> 8);
}

static int cube_world_z_cell(int world_z)
{
    return (int)(int16_t)(
        ((uint32_t)world_z + UINT32_C(0x7f00)) >> 8);
}

static int cube_sight_from_map_cube(int32_t *cube)
{
    uint32_t sight;

    if (cube == NULL) {
        return 6;
    }
    if (((uint32_t)cube[0] & UINT32_C(0x3c000000)) == 0) {
        uint32_t library = ((uint32_t)cube[0] >> 14) & 0xffU;
        int32_t library_base;

        library_base = leveldata[-4] >> 11;
        cube = leveldata + library_base + (int)(library * 9U);
    }
    sight = ((uint32_t)cube[1] >> 17) & 0x1fU;
    return sight != 0 ? (int)sight : 6;
}

static float cube_toggle_sign(float value)
{
    uint32_t bits;

    memcpy(&bits, &value, sizeof(bits));
    bits ^= UINT32_C(0x80000000);
    memcpy(&value, &bits, sizeof(value));
    return value;
}

/* 0x2ABA0, 565 bytes, global, 8 named locals
 * GPUcluts
 * PDB type: int (void*, int*)
 * Source: W:\SWJediPowerBattles\Work\cube.c
 */
int GPUcluts(void *mapbase, int32_t *buffer4k)
{
    SRECT rectangle = {0, 508, 512, 4};
    SRECT clut_rectangle = {0, 508, 4, 3};
    POLY_GT4 polygon = {
        .tag = UINT32_C(0x0c000000),
        .code = 0x3c,
        .x0 = 0,
        .y0 = 0,
        .u0 = 32,
        .v0 = 252,
        .x1 = 4,
        .y1 = 0,
        .u1 = 32,
        .v1 = 252,
        .tpage = UINT16_C(0x0110),
        .r2 = 8,
        .x2 = 0,
        .y2 = 3,
        .u2 = 32,
        .v2 = 252,
        .r3 = 8,
        .x3 = 4,
        .y3 = 3,
        .u3 = 32,
        .v3 = 252
    };
    uint32_t result;

    DrawSync(0);
    StoreImage(&rectangle, (unsigned *)(void *)buffer4k);
    DrawSync(0);
    if ((uint32_t)buffer4k[1023] == UINT32_C(0x4d6e6f4a)) {
        result = (uint32_t)buffer4k[1022];
    } else {
        DRAWENV environment;
        volatile uint16_t place[12];
        uint32_t occupied = 0;
        int index;

        SetDefDrawEnv(&environment, 0, 508, 512, 4);
        environment.isbg = 1;
        environment.r0 = 248;
        environment.g0 = 248;
        environment.b0 = 248;
        DrawSync(0);
        PutDrawEnv(&environment);
        DrawSync(0);
        DrawPrim(&polygon);
        DrawSync(0);
        StoreImage(
            &clut_rectangle,
            (unsigned *)(void *)place);
        DrawSync(0);

        for (index = 0; index < 12; ++index) {
            occupied |= (uint32_t)place[index] & UINT32_C(0x7fff);
        }
        if (occupied == 0) {
            contraband_cluts(buffer4k);
            contraband_lights(mapbase);
        } else {
            band_lights();
        }
        buffer4k[1023] = (int32_t)UINT32_C(0x4d6e6f4a);
        result = occupied == 0;
        buffer4k[1022] = (int32_t)result;
        LoadImage(&rectangle, (unsigned *)(void *)buffer4k);
        DrawSync(0);
    }
    return (int)result;
}

/* 0x2ADE0, 3 bytes, global, 2 named locals
 * cube_GetCubeAmbientLight
 * PDB type: void (VECTOR*, CVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\cube.c
 */

void cube_GetCubeAmbientLight(VECTOR *position, CVECTOR *color)
{
    (void)position;
    (void)color;
}

/* 0x2ADF0, 18 bytes, global, 1 named locals
 * cube_HideMesh
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\cube.c
 */
void cube_HideMesh(int mesh)
{
    cullmesh[mesh] = 0;
}

/* 0x2AE10, 241 bytes, global, 1 named locals
 * cube_InitVisibility
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\cube.c
 */
void cube_InitVisibility(void)
{
    size_t index;

    memset(cull.visarray, 1, sizeof(cull.visarray));
    for (index = 0; index < JPB_CULL_MESH_COUNT; ++index) {
        cullmesh[index] = 1;
    }

    if (LevelSelect == 8) {
        /* DrawLevel's three pass procedures use this slot as a sentinel. */
        cullmesh[31] = 8;
    }

    switch (LevelSelect) {
    case 1:
        if (fed_wallfrigflag != 0) {
            cullmesh[1] = 1;
            cullmesh[0] = 0;
        } else {
            cullmesh[0] = 1;
            cullmesh[1] = 0;
        }
        break;
    case 5:
        if (tato_wallfrigflag != 0) {
            cullmesh[2] = 0;
        } else {
            cullmesh[1] = 0;
        }
        break;
    case 8:
        for (index = 1; index < 25; index += 2) {
            cullmesh[index] = 0;
        }
        break;
    default:
        break;
    }
}

/* 0x2AF10, 2156 bytes, global, 29 named locals
 * cube_NewWorldRender
 * PDB type: void (MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\cube.c
 */

void cube_NewWorldRender(MATRIX *matrix)
{
    _jheightstuff thatthing = {0};
    int32_t *cube0 = maPhysicsData[0].mapinfo.cube;
    int32_t *cube1;
    int xmin;
    int xmax;
    int zmin;
    int zmax;
    int sight;
    int center;
    int level;
    int frustum_distance;
    MATRIX temp;
    VECTOR v_campos = {0};
    float dx;
    float dy;
    float dz;

    if ((gpWorld->aDolly[gpWorld->currentDolly].flags &
         UINT32_C(0x00000400)) != 0) {
        BAP_CAMERADOLLY *dolly =
            &gpWorld->aDolly[gpWorld->currentDolly];
        VECTOR dolly_position = {
            dolly->offset.vx,
            dolly->offset.vy,
            dolly->offset.vz,
            0
        };

        (void)intersec_FindWalkHeight(
            &dolly_position,
            NULL,
            (objectRoot *)(void *)&thatthing,
            1);
        sight = thatthing.cube != NULL
                    ? cube_sight_from_map_cube(thatthing.cube)
                    : 0x10;
        center = cube_world_x_cell(dolly->offset.vx);
        minx = center - sight;
        maxx = center + sight;
        center = cube_world_z_cell(dolly->offset.vz);
        minz = center - sight;
        maxz = center + sight;
    } else if (GameStruct.NumPlayers == 2 &&
               GameStruct.CurrentLevel != 12) {
        int player0_active = cube_player_is_active(0);
        int player1_active = cube_player_is_active(1);
        int player0_x = cube_world_x_cell((int)maPhysicsData[0].pos.vx);
        int player0_z = cube_world_z_cell((int)maPhysicsData[0].pos.vz);
        int player1_x = cube_world_x_cell((int)maPhysicsData[1].pos.vx);
        int player1_z = cube_world_z_cell((int)maPhysicsData[1].pos.vz);

        cube1 = maPhysicsData[1].mapinfo.cube;
        if (player0_active || player1_active) {
            if (player0_active) {
                minx = maxx = player0_x;
                minz = maxz = player0_z;
            } else {
                minx = maxx = player1_x;
                minz = maxz = player1_z;
            }
        }
        if (player0_active && cube0 != NULL) {
            sight = cube_sight_from_map_cube(cube0);
            if (player0_x - sight <= minx) {
                minx = player0_x - sight;
            }
            if (maxx <= player0_x + sight) {
                maxx = player0_x + sight;
            }
            if (player0_z - sight <= minz) {
                minz = player0_z - sight;
            }
            if (maxz <= player0_z + sight) {
                maxz = player0_z + sight;
            }
        }
        if (player1_active && cube1 != NULL) {
            sight = cube_sight_from_map_cube(cube1);
            if (player1_x - sight <= minx) {
                minx = player1_x - sight;
            }
            if (maxx <= player1_x + sight) {
                maxx = player1_x + sight;
            }
            if (player1_z - sight <= minz) {
                minz = player1_z - sight;
            }
            if (maxz <= player1_z + sight) {
                maxz = player1_z + sight;
            }
        }
    } else {
        int player0_active = cube_player_is_active(0);
        VECTOR world_location = {
            gpWorld->location.vx,
            gpWorld->location.vy,
            gpWorld->location.vz,
            0
        };

        if (!player0_active) {
            cube0 = NULL;
        }
        (void)intersec_FindWalkHeight(
            &world_location,
            NULL,
            (objectRoot *)(void *)&thatthing,
            1);
        if (cube0 == NULL) {
            cube0 = thatthing.cube;
        }
        sight = cube_sight_from_map_cube(cube0);
        if (player0_active) {
            center = cube_world_x_cell(gpWorld->location.vx);
            minx = center - sight;
            maxx = center + sight;
            center = cube_world_z_cell(gpWorld->location.vz);
            minz = center - sight;
            maxz = center + sight;
        }
    }

    if (LevelSelect == 8) {
        int streets_z = cube_world_z_cell(gpWorld->location.vz);
        int streets_height = (int)(int16_t)(
            (uint32_t)gpWorld->location.vy >> 8);
        int streets_x = cube_world_x_cell(gpWorld->location.vx);

        if (streets_z < 0x30) {
            zmin = 0x19;
            zmax = 0x28;
        } else if (streets_z < 0x50) {
            zmin = 0x39;
            zmax = 0x48;
        } else if (streets_z < 0x70) {
            zmin = 0x59;
            zmax = 0x68;
        } else {
            zmin = 0x79;
            zmax = 0x88;
        }
        if ((unsigned)(streets_height - 0x0f) < 5U &&
            streets_x > 0x4c &&
            (unsigned)(streets_z - 0x77) < 0x13U &&
            streets_reached_stairs == 0) {
            jpb_CubeRuntimeFlags |= UINT32_C(8);
            streets_reached_stairs = 1;
        }
        xmin = (0x80ff - cameraposition.vx) >> 8;
        xmax = xmin + 0x30;
        if ((jpb_CubeRuntimeFlags & UINT32_C(8)) != 0) {
            zmin -= 5;
            zmax += 5;
        }
        frustum_distance = 0x180;
    } else {
        xmin = minx;
        xmax = maxx + 1;
        zmin = minz;
        zmax = maxz + 1;
        frustum_distance = 200;
    }

    jon_texscroll(leveldata, gGlobalFrameRate / 0x800);
    calc_frustrum(&twattedcameramatrix, frustum_distance);
    set_camera(&twattedcameramatrix);

    sight = (int)(int8_t)GameStruct.maxdraw;
    center = (xmax + xmin) / 2;
    if (xmax - xmin > sight) {
        xmax = center + sight;
        xmin = center - sight;
    }
    center = (zmax + zmin) / 2;
    if (zmax - zmin > sight) {
        zmax = center + sight;
        zmin = center - sight;
    }
    jpb_last_render_bounds.minX = xmin;
    jpb_last_render_bounds.minZ = zmin;
    jpb_last_render_bounds.maxX = xmax;
    jpb_last_render_bounds.maxZ = zmax;

    temp = *matrix;
    temp.m[0][0] = cube_toggle_sign(matrix->m[0][0]);
    temp.m[0][1] = matrix->m[0][2];
    temp.m[0][2] = matrix->m[0][1];
    temp.m[1][0] = cube_toggle_sign(matrix->m[1][0]);
    temp.m[1][1] = matrix->m[1][2];
    temp.m[1][2] = matrix->m[1][1];
    temp.m[2][0] = cube_toggle_sign(matrix->m[2][0]);
    temp.m[2][1] = matrix->m[2][2];
    temp.m[2][2] = matrix->m[2][1];
    camera_GetLocation(&v_campos);
    level = (int)(int8_t)LevelSelect;
    dy = level_offset[level][1] * 512.0f + (float)v_campos.vz;
    dz = level_offset[level][2] * 256.0f + (float)v_campos.vy;
    dx = cube_toggle_sign(
        level_offset[level][0] * 256.0f -
        (float)v_campos.vx);
    temp.t[0] = (int)(
        (temp.m[0][0] * dx - temp.m[0][1] * dy) -
        temp.m[0][2] * dz);
    temp.t[1] = (int)(
        (temp.m[1][0] * dx - temp.m[1][1] * dy) -
        temp.m[1][2] * dz);
    temp.t[2] = (int)(
        (temp.m[2][0] * dx - temp.m[2][1] * dy) -
        temp.m[2][2] * dz);

    SetupTransformMatrix(matrix);
    worldTURTLEMatrix = matrix;
    if (LevelSelect == 12) {
        (void)plotsomecubes(xmin, zmin, xmax, zmax);
    } else if (LevelSelect != 0) {
        _ApplyLevelTransformation(
            &temp,
            level_scale[level][0],
            level_scale[level][1],
            level_scale[level][2]);
    }
}

/* 0x2B780, 18 bytes, global, 1 named locals
 * cube_ShowMesh
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\cube.c
 */
void cube_ShowMesh(int mesh)
{
    cullmesh[mesh] = 1;
}

/* 0x2B7A0, 1987 bytes, global, 2 named locals
 * initUVs
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\cube.c
 */
int initUVs(void)
{
    uint32_t block;
    uint32_t second = 0;

    for (block = 0; block < JPB_CUBE_UV_SET_COUNT / 8U; ++block) {
        const uint32_t *source =
            (const uint32_t *)(const void *)texturebase +
            block * 48U;
        float *destination = fUV[block * 8U].uv;
        uint32_t face;

        for (face = 0; face < 8U; ++face) {
            uint32_t first = source[face * 6U + 1U];
            uint32_t output = face * 8U;

            second = source[face * 6U + 2U];
            destination[output + 0U] =
                (float)(first & UINT32_C(0xff)) * (1.0f / 256.0f);
            destination[output + 1U] =
                1.0f - (float)((first >> 8) & UINT32_C(0xff)) *
                           (1.0f / 256.0f);
            destination[output + 2U] =
                (float)((first >> 16) & UINT32_C(0xff)) *
                (1.0f / 256.0f);
            destination[output + 3U] =
                1.0f - (float)(first >> 24) * (1.0f / 256.0f);
            destination[output + 4U] =
                (float)(second & UINT32_C(0xff)) * (1.0f / 256.0f);
            destination[output + 5U] =
                1.0f - (float)((second >> 8) & UINT32_C(0xff)) *
                           (1.0f / 256.0f);
            destination[output + 6U] =
                (float)((second >> 16) & UINT32_C(0xff)) *
                (1.0f / 256.0f);
            destination[output + 7U] =
                1.0f - (float)(second >> 24) * (1.0f / 256.0f);
        }
    }

    return (int)(second >> 24);
}

const float *jpb_CubeUVTable(void)
{
    return &fUV[0].uv[0];
}

/* 0x2BF70, 2994 bytes, global, 40 named locals
 * plotsomecubes
 * PDB type: int (int, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\cube.c
 */

int plotsomecubes(int min_x, int min_z, int max_x, int max_z)
{
    VECTOR camera_location;
    FVECTOR camera_position;
    int16_t z_origin = (int16_t)(((int16_t)min_z - 127) * 256);
    int z;

    camera_GetLocation(&camera_location);
    camera_position.vx = (float)camera_location.vx;
    camera_position.vy = (float)camera_location.vy;
    camera_position.vz = (float)camera_location.vz;
    makecull(
        cull.planes,
        &CameraMatrix,
        &camera_position,
        222.0f,
        1920.0f,
        1080.0f,
        460.0f,
        24.0f);
    oldspice = NULL;

    for (z = min_z; z <= max_z; ++z) {
        int16_t x_origin =
            (int16_t)(-32768 - (int16_t)min_x * 256);
        int x;

        for (x = min_x; x < max_x; ++x) {
            if (x >= 0 && x < 256 && z >= 0 && z < mapyend) {
                uint32_t map_entry = (uint32_t)leveldata[x + z * 256];

                if ((int32_t)map_entry < 0) {
                    int32_t *cube =
                        leveldata + (map_entry & UINT32_C(0x1ffff));
                    uint32_t cube_header;

                    do {
                        int32_t *next_cube;
                        int cube_height;

                        cube_header = (uint32_t)cube[0];
                        cube_height = (int)(cube_header & UINT32_C(0x7f)) *
                                      256;
                        next_cube = cube +
                            ((cube_header >> 26) & UINT32_C(0x0f)) + 1;
                        cubepos[0].vx = (float)((int)x_origin + 128);
                        cubepos[0].vy = (float)(cube_height + 128);
                        cubepos[0].vz = (float)((int)z_origin + 128);
                        cubepos[0].vw = 1.0f;

                        if (next_cube != cube + 1) {
                            int cull_result = seecull(cubepos, cull.planes);

                            if (cull_result == 2) {
                                break;
                            }
                            if (cull_result == 0) {
                                int32_t *entry = cube + 2;

                                while (entry < next_cube) {
                                    uint32_t entry_word =
                                        (uint32_t)entry[0];
                                    FVECTOR cube_origin = {
                                        (float)(int)x_origin,
                                        (float)cube_height,
                                        (float)(int)z_origin
                                    };
                                    int vertex_count;
                                    int32_t *library = jon_getlibpartfloat(
                                        pcache,
                                        entry,
                                        &cube_origin,
                                        leveldata,
                                        &vertex_count);
                                    const uint8_t *colors =
                                        (const uint8_t *)(const void *)
                                            colorbase +
                                        ((entry_word >> 16) &
                                         UINT32_C(0x3fff)) * 4U;
                                    uint32_t library_header =
                                        (uint32_t)library[0];
                                    uint8_t color_flags =
                                        (uint8_t)(library_header >> 21);
                                    uint32_t color_cursor = 0;
                                    int transformed;
                                    int index;
                                    int32_t *polygon;

                                    for (index = 0; index < 8; ++index) {
                                        if ((color_flags &
                                             (uint8_t)(0x80U >> index)) != 0) {
                                            libpartcols[index] =
                                                (uint32_t)colorbase[
                                                    -(int)colors[color_cursor]];
                                            ++color_cursor;
                                        } else {
                                            libpartcols[index] =
                                                UINT32_C(0x00ff00ff);
                                        }
                                    }
                                    for (index = 0;
                                         index <
                                             (int)((library_header >> 16) &
                                                   UINT32_C(0x1f));
                                         ++index) {
                                        libpartcols[index + 8] =
                                            (uint32_t)colorbase[
                                                -(int)colors[color_cursor++]];
                                    }

                                    transformed = RotTransPersManyFV(
                                        pcache,
                                        vertex_count,
                                        pcache);
                                    polygon = library + 2;
                                    {
                                        uint32_t polygon_header;

                                    do {
                                        polygon_header = (uint32_t)polygon[0];

                                        if ((((polygon_header |
                                               (uint32_t)leveldata[
                                                   polygon_header &
                                                   UINT32_C(0x1ffff)]) >>
                                              17) &
                                             UINT32_C(1)) == 0 &&
                                            transformed == 0) {
                                            uint32_t polygon_indices =
                                                (uint32_t)polygon[1];
                                            int is_triangle =
                                                (int)((polygon_indices >> 20) &
                                                      UINT32_C(1));
                                            int texture_id =
                                                (int)((polygon_header >> 18) &
                                                      UINT32_C(0x3ff));
                                            int uv_mode =
                                                (int)(((polygon_indices >> 20) &
                                                       UINT32_C(1)) << 3) |
                                                (int)((polygon_indices >> 15) &
                                                      UINT32_C(3)) |
                                                (int)(((polygon_header >> 28) &
                                                       UINT32_C(1)) << 2);
                                            int polygon_vertex_count =
                                                4 - is_triangle;
                                            int polygon_vertex;

                                            _StartPoly(
                                                polygon_vertex_count,
                                                leveltexture[
                                                    ((uint32_t)texturebase[
                                                         texture_id * 6] >>
                                                     17) &
                                                    UINT32_C(3)]);
                                            for (polygon_vertex = 0;
                                                 polygon_vertex <
                                                     polygon_vertex_count;
                                                 ++polygon_vertex) {
                                                int vertex_index =
                                                    (int)((polygon_indices >>
                                                           (polygon_vertex * 5)) &
                                                          UINT32_C(0x1f));
                                                int uv_index =
                                                    uvredir[uv_mode]
                                                           [polygon_vertex];

                                                _SetVert(
                                                    polygon_vertex,
                                                    pcache[vertex_index].vx,
                                                    pcache[vertex_index].vy,
                                                    pcache[vertex_index].vz,
                                                    libpartcols[vertex_index],
                                                    fUV[texture_id].uv[uv_index],
                                                    fUV[texture_id]
                                                        .uv[uv_index + 1]);
                                            }
                                            _NoScaleEndPoly();
                                        }
                                        polygon += 2;
                                    } while ((polygon_header &
                                              UINT32_C(0xc0000000)) == 0);
                                    }

                                    entry += (entry_word >> 30) + 1;
                                }
                            }
                        }
                        cube = next_cube;
                    } while ((cube_header & UINT32_C(0x40000000)) == 0);
                }
            }
            x_origin = (int16_t)(x_origin - 256);
        }
        z_origin = (int16_t)(z_origin + 256);
    }
    return 0;
}

void jpb_CubeGetLastRenderBounds(JPBCubeRenderBounds *bounds)
{
    if (bounds != NULL) {
        *bounds = jpb_last_render_bounds;
    }
}

/* 0x2CB30, 515 bytes, global, 5 named locals
 * twatcameramatrix
 * PDB type: void (MATRIX*, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\cube.c
 */
static void cube_toggle_float_sign(float *value)
{
    uint32_t bits;

    memcpy(&bits, value, sizeof(bits));
    bits ^= UINT32_C(0x80000000);
    memcpy(value, &bits, sizeof(bits));
}

void twatcameramatrix(MATRIX *matrix, MATRIX *m)
{
    VECTOR camera_location;
    int row;
    int column;
    int index;
    int tmp;

    *m = *matrix;
    camera_GetLocation(&camera_location);
    m->t[0] = camera_location.vx;
    m->t[1] = camera_location.vy;
    m->t[2] = camera_location.vz;
    m->t[0] = 0x8000 - m->t[0];
    tmp = m->t[2];
    m->t[2] = m->t[1];
    m->t[1] = tmp + 0x8000;

    for (row = 1; row < 3; ++row) {
        for (column = 0; column < row; ++column) {
            float swap = m->m[row][column];

            m->m[row][column] = m->m[column][row];
            m->m[column][row] = swap;
        }
    }
    for (column = 0; column < 3; ++column) {
        float swap = m->m[1][column];

        m->m[1][column] = m->m[2][column];
        m->m[2][column] = swap;
    }
    for (index = 0; index < 9; ++index) {
        cube_toggle_float_sign(
            &m->m[index / 3][index % 3]);
    }
    m->t[0] += 0x100;
    m->t[1] -= 0x100;
    cube_toggle_float_sign(&m->m[0][1]);
    cube_toggle_float_sign(&m->m[0][2]);
    cube_toggle_float_sign(&m->m[1][0]);
}
