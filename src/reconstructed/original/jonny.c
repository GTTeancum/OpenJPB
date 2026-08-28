/*
 * REVIEWED RECONSTRUCTION.
 * PDB module: 0046
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\jonny.obj
 * Primary source: W:\SWJediPowerBattles\work\jonny.c
 * Compiler language: c
 * Emitted procedures: 24
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/jonny.h"
#include "jpb/anim.h"
#include "jpb/io.h"
#include "jpb/jpx.h"
#include "jpb/loader.h"
#include "jpb/memory.h"
#include "jpb/menu.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/texture.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int32_t jonny_wrapped_orient_xz(
    int32_t px,
    int32_t pz,
    int32_t ax,
    int32_t az,
    int32_t bx,
    int32_t bz)
{
    uint32_t result =
        ((uint32_t)px - (uint32_t)bx) *
            ((uint32_t)az - (uint32_t)bz) -
        ((uint32_t)pz - (uint32_t)bz) *
            ((uint32_t)ax - (uint32_t)bx);
    int32_t signed_result;

    /*
     * The original x64 instructions perform their edge products in 32-bit
     * registers. Copying the result bits avoids C signed-overflow undefined
     * behavior while preserving that two's-complement wrap on PC and Xbox.
     */
    memcpy(&signed_result, &result, sizeof(signed_result));
    return signed_result;
}

static int32_t jonny_sign_extend_10(uint32_t value)
{
    value &= UINT32_C(0x3ff);
    if ((value & UINT32_C(0x200)) != 0) {
        return (int32_t)value - 0x400;
    }
    return (int32_t)value;
}

static int16_t jonny_read_i16(const void *base, size_t offset)
{
    int16_t value;

    memcpy(&value, (const uint8_t *)base + offset, sizeof(value));
    return value;
}

static float jonny_toggle_float_sign(float value)
{
    uint32_t bits;

    memcpy(&bits, &value, sizeof(bits));
    bits ^= UINT32_C(0x80000000);
    memcpy(&value, &bits, sizeof(value));
    return value;
}

/* Exact PDB-named jonny.c module statics at RVAs 0x5381C0/0x5381B8. */
static int32_t *coordlist;
static int32_t channel;

static void jonny_record_changed_entry(
    int32_t *mapbase, int32_t *entry, uint32_t replacement)
{
    *eventlist_next++ = (int32_t)(entry - mapbase);
    *eventlist_next++ = (int32_t)(int16_t)*(uint16_t *)(void *)entry;
    *(uint16_t *)(void *)entry = (uint16_t)replacement;
    if (eventlist_next == eventlist_end) {
        eventlist_next = eventlist_start;
    }
}

static int32_t jonny_plane_height(
    uint32_t packed_normal,
    int anchor_x,
    int anchor_y,
    int anchor_z,
    const VECTOR *pos)
{
    int normal_x = jonny_sign_extend_10(packed_normal) * 8;
    int normal_y = jonny_sign_extend_10(packed_normal >> 10) * 8;
    int normal_z = jonny_sign_extend_10(packed_normal >> 20) * 8;

    return
        (normal_x * (anchor_x - pos->vx) +
         normal_z * (anchor_z - pos->vz)) /
            normal_y +
        anchor_y;
}

/* 0xB45C0, 912 bytes, global, 28 named locals
 * BlockBuster
 * PDB type: int (int*, int, int)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
int BlockBuster(
    int32_t *mapbase,
    int hitforce,
    int cubeshite)
{
    int32_t *coords = (int32_t *)(void *)gaScratch;
    int x = cubeshite & 0xff;
    int my = (cubeshite >> 8) & 0xff;
    int mz = (cubeshite >> 16) & 0xff;
    int total = 0;
    int mx;

    if (mapbase == NULL ||
        eventlist_start == NULL ||
        eventlist_next == NULL ||
        eventlist_end == NULL) {
        return 0;
    }

    coordlist = coords;
    for (mx = x - 1; mx < x + 1; ++mx) {
        int y;

        for (y = my - 1; y < my + 1; ++y) {
            uint32_t cell;
            int32_t *cube;

            if ((unsigned)mx >= 256U ||
                y < 0 ||
                y >= mapyend) {
                continue;
            }
            cell =
                (uint32_t)mapbase[y * 256 + mx];
            if ((int32_t)cell >= 0) {
                continue;
            }

            cube =
                &mapbase[cell & UINT32_C(0x1ffff)];
            for (;;) {
                uint32_t cube_header =
                    (uint32_t)*cube;
                int32_t *nextcube =
                    cube +
                    ((cube_header >> 26) &
                     UINT32_C(0xf)) +
                    1;
                int cube_z =
                    (int)(cube_header &
                          UINT32_C(0x7f));

                if (nextcube != cube + 1 &&
                    mz - 1 <= cube_z) {
                    int32_t *entry;
                    int cubebase;

                    if (mz + 1 < cube_z) {
                        break;
                    }
                    cubebase =
                        (cube_z << 16) |
                        (y << 8) |
                        mx;
                    for (entry = cube + 2;
                         entry < nextcube;) {
                        uint16_t entryofs =
                            *(uint16_t *)(void *)entry;

                        if (((uint32_t)
                                 mapbase[entryofs] &
                             UINT32_C(0x20000000)) !=
                            0) {
                            uint32_t eventword =
                                (uint32_t)
                                    mapbase[
                                        (int)entryofs -
                                        1];
                            int etouch =
                                (int)(
                                    (eventword >> 16) &
                                    UINT32_C(3));
                            int touchforce =
                                (int)(
                                    (eventword >> 18) &
                                    UINT32_C(0xf));

                            if (!((etouch == 0 ||
                                   (hitforce & 3) <
                                       etouch) &&
                                  (touchforce == 0 ||
                                   (hitforce >> 2) <
                                       touchforce) &&
                                  !ExtraCharacterEnvironmentEffectExceptions())) {
                                int ehit;

                                jonny_record_changed_entry(
                                    mapbase,
                                    entry,
                                    eventword);
                                channel =
                                    (int)(
                                        eventword &
                                        UINT32_C(
                                            0x01c00000));
                                ehit =
                                    (int)(
                                        ((eventword >>
                                          1) &
                                         UINT32_C(
                                             0x0f000000)) |
                                        ((uint32_t)
                                             cubebase &
                                         UINT32_C(
                                             0x00ffffff)));
                                *coordlist++ = ehit;
                                if (channel != 0) {
                                    MTV(
                                        cubebase -
                                        0x100);
                                    MTV(
                                        cubebase + 1);
                                    MTV(
                                        cubebase +
                                        0x100);
                                    MTV(
                                        cubebase - 1);
                                }
                                total =
                                    (int)(
                                        coordlist -
                                        coords);
                            }
                        }
                        entry +=
                            ((uint32_t)*entry >>
                             30) +
                            1;
                    }
                }
                cube = nextcube;
                if ((cube_header &
                     UINT32_C(0x40000000)) != 0) {
                    break;
                }
            }
        }
    }
    return total;
}

/* 0xB4950, 94 bytes, global, 1 named locals
 * ExtraCharacterEnvironmentEffectExceptions
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
int ExtraCharacterEnvironmentEffectExceptions(void)
{
    uint16_t player_id = (uint16_t)gaPlayerData[0].playerID;
    Motion **motion = gaPlayerData[0].pMotion;
    int uses_motion_damage = 0;

    if (player_id <= UINT16_C(0x1a) &&
        (UINT32_C(0x04228000) & (UINT32_C(1) << player_id)) != 0) {
        uses_motion_damage = 1;
    } else if (player_id == UINT16_C(0x1e)) {
        return motion != NULL;
    } else {
        uint16_t extra_id = (uint16_t)(player_id - UINT16_C(0x24));

        if (extra_id <= UINT16_C(0x2b) &&
            (UINT64_C(0x8000002a001) &
             (UINT64_C(1) << extra_id)) != 0) {
            uses_motion_damage = 1;
        }
    }

    if (!uses_motion_damage || motion == NULL) {
        return 0;
    }
    return (*motion)->Damage != 0;
}

/* 0xB49B0, 378 bytes, global, 12 named locals
 * HitsHit
 * PDB type: int (int*, int*, int, int, int*)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
int HitsHit(
    int32_t *mapbase,
    int32_t *entry,
    int hitforce,
    int cubeshite,
    int32_t *coords)
{
    uint16_t entryofs = *(uint16_t *)(void *)entry;
    uint32_t eventword;
    int etouch;
    int touchforce;
    int ehit;

    coordlist = coords;
    if (((uint32_t)mapbase[entryofs] & UINT32_C(0x20000000)) == 0) {
        return 0;
    }

    eventword = (uint32_t)mapbase[(int)entryofs - 1];
    etouch = (int)((eventword >> 16) & UINT32_C(3));
    touchforce = (int)((eventword >> 18) & UINT32_C(0xf));
    if ((etouch == 0 || (hitforce & 3) < etouch) &&
        (touchforce == 0 || (hitforce >> 2) < touchforce) &&
        !ExtraCharacterEnvironmentEffectExceptions()) {
        return 0;
    }

    jonny_record_changed_entry(mapbase, entry, eventword);
    channel = (int)(eventword & UINT32_C(0x01c00000));
    ehit =
        (int)(((eventword >> 1) & UINT32_C(0x0f000000)) |
              ((uint32_t)cubeshite & UINT32_C(0x00ffffff)));
    *coordlist++ = ehit;

    if (channel != 0) {
        MTV(cubeshite - 0x100);
        MTV(cubeshite + 1);
        MTV(cubeshite + 0x100);
        MTV(cubeshite - 1);
    }
    return (int)(coordlist - coords);
}

/* 0xB4B30, 644 bytes, global, 10 named locals
 * InitJPX
 * PDB type: int (char*)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */

/* 0xB4DC0, 407 bytes, global, 11 named locals
 * MTV
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
int InitJPX(char *name)
{
    JPBJpxBinHeader knobber;
    JPBFileHandle fd;
    uint64_t file_size;
    int i;
    int32_t *strip;

    if (!file_OPEN(name, &fd)) {
        exit(0);
    }
    file_size = file_GETSIZE(&fd);
    (void)file_READ(
        &fd, (char *)&knobber, 0x1000, JPB_FILE_READ_STREAM);
    if (knobber.worldOffset > 0x1000) {
        exit(0);
    }
    (void)file_CLOSE(&fd);

    for (i = 0; i < knobber.numMaterials; ++i) {
        JPBJpxMaterialDef *md = &knobber.materialDefs[i];

        menu_addTotal(100);
        if (md->nameOffset != 0) {
            char buf[256];
            char *p;

            sprintf(
                buf,
                "../../../res/level\\jpx\\\\%s\\%s",
                loader_GetLevelName(),
                (char *)&knobber + md->nameOffset);
            p = buf;
            while (*p != '.') {
                ++p;
            }
            p[1] = '\0';
            p = buf;
            while (*p != '\0') {
                ++p;
            }
            memcpy(p, "pvr", 4);
            level_materials[i] =
                _LoadTexture(buf, TT_LEVEL, UINT32_C(0x02000000));
        }
    }

    WorldmeshData = (int32_t *)(void *)mem_pool_3a;
    (void)file_OPEN(name, &fd);
    (void)file_READ(
        &fd,
        (char *)WorldmeshData,
        (int32_t)file_size,
        4);
    WorldmeshData = (int32_t *)(
        (uint8_t *)WorldmeshData + knobber.worldOffset);
    strip = (int32_t *)(
        (uint8_t *)WorldmeshData + knobber.firstStripOffset);
    do {
        int stripofs = strip[0];
        int32_t *std =
            (int32_t *)level_materials[strip[1]]->texture;

        menu_addTotal(100);
        strip[0] = std[0];
        strip[1] = std[1];
        strip[2] = std[2];
        strip[3] = std[3];
        strip = (int32_t *)((uint8_t *)strip + stripofs);
        if (stripofs == 0) {
            break;
        }
    } while (1);
    (void)file_CLOSE(&fd);
    return 0;
}
void MTV(int cubeshite)
{
    int slot = cubeshite + 1;

    for (;;) {
        int anyhits = 0;
        int thiscube = slot - 1;
        uint32_t cell = (uint32_t)leveldata[(uint16_t)thiscube];
        int32_t *cube;
        uint32_t lastcube;

        if ((int32_t)cell >= 0) {
            return;
        }

        cube = &leveldata[cell & UINT32_C(0x1ffff)];
        do {
            uint32_t cube_header = (uint32_t)*cube;
            int32_t *nextcube =
                cube + ((cube_header >> 26) & UINT32_C(0xf)) + 1;
            int32_t *entry;

            lastcube = cube_header & UINT32_C(0x40000000);
            for (entry = cube + 2; entry < nextcube;) {
                uint16_t entryofs = *(uint16_t *)(void *)entry;
                uint32_t eventword =
                    (uint32_t)leveldata[(int)entryofs - 1];

                if (((uint32_t)leveldata[entryofs] &
                     UINT32_C(0x20000000)) != 0 &&
                    (eventword & UINT32_C(0x01c00000)) ==
                        (uint32_t)channel) {
                    jonny_record_changed_entry(
                        leveldata, entry, eventword);
                    *coordlist++ =
                        (int32_t)(((eventword >> 1) &
                                   UINT32_C(0x0f000000)) |
                                  ((uint32_t)thiscube &
                                   UINT32_C(0x00ffffff)));
                    anyhits = 1;
                }
                entry +=
                    ((uint32_t)*entry >> 30) + UINT32_C(1);
            }
            cube = nextcube;
        } while (lastcube == 0);

        if (!anyhits) {
            return;
        }
        MTV(thiscube - 0x100);
        MTV(thiscube + 1);
        MTV(thiscube + 0x100);
        slot = thiscube;
    }
}

/* 0xB4F60, 3 bytes, global, 0 named locals
 * band_lights
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
void band_lights(void)
{
}

/* 0xB4F70, 3 bytes, global, 2 named locals
 * calc_frustrum
 * PDB type: void (MATRIX*, int)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */

void calc_frustrum(MATRIX *matrix, int distance)
{
    (void)matrix;
    (void)distance;
}

/* 0xB4F80, 40 bytes, global, 0 named locals
 * clear_eventlist
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
void clear_eventlist(void)
{
    eventlist_start[0] = 0;
    eventlist_start[1] = 0;
    eventlist_next = eventlist_start + 2;
}

/* 0xB4FB0, 3 bytes, global, 1 named locals
 * contraband_cluts
 * PDB type: void (void*)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
void contraband_cluts(void *cluts)
{
    (void)cluts;
}

/* 0xB4FC0, 3 bytes, global, 1 named locals
 * contraband_lights
 * PDB type: void (void*)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
void contraband_lights(void *mapbase)
{
    (void)mapbase;
}

/* 0xB4FD0, 620 bytes, local, 23 named locals
 * intersec_WankCheck
 * PDB type: int (_svector*, int**, VECTOR*)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
static int intersec_WankCheck(
    _svector *verts,
    int32_t **polyptr,
    VECTOR *pos)
{
    uint32_t *polys = (uint32_t *)*polyptr;
    int highest = INT_MIN;

    do {
        uint32_t poly = polys[0];
        uint32_t indices = polys[1];

        if ((indices & UINT32_C(0x20000000)) == 0) {
            unsigned p0_index = indices & UINT32_C(0x1f);
            unsigned p1_index =
                (indices >> 5) & UINT32_C(0x1f);
            unsigned p2_index =
                (indices >> 10) & UINT32_C(0x1f);
            int p3_index =
                (indices & UINT32_C(0x100000)) != 0
                    ? -1
                    : (int)((indices >> 15) & UINT32_C(0x1f));
            _svector *pnt1 = &verts[p1_index];
            _svector *pnt2 = &verts[p2_index];
            _svector *edge_a = pnt1;
            _svector *edge_b = pnt2;
            int candidate_index = (int)p0_index;

            if (jonny_wrapped_orient_xz(
                    pos->vx,
                    pos->vz,
                    pnt1->vx,
                    pnt1->vz,
                    pnt2->vx,
                    pnt2->vz) < 0) {
                if (p3_index < 0) {
                    goto next_polygon;
                }
                edge_a = pnt2;
                edge_b = pnt1;
                candidate_index = p3_index;
            }

            {
                _svector *candidate = &verts[candidate_index];

                if (jonny_wrapped_orient_xz(
                        pos->vx,
                        pos->vz,
                        edge_a->vx,
                        edge_a->vz,
                        candidate->vx,
                        candidate->vz) <= 0 &&
                    jonny_wrapped_orient_xz(
                        pos->vx,
                        pos->vz,
                        candidate->vx,
                        candidate->vz,
                        edge_b->vx,
                        edge_b->vz) <= 0) {
                    uint32_t packed_normal =
                        (uint32_t)leveldata[
                            (poly & UINT32_C(0x1ffff)) + 1U];
                    int normal_y =
                        jonny_sign_extend_10(packed_normal >> 10) * 8;

                    if (normal_y != 0) {
                        int height = jonny_plane_height(
                            packed_normal,
                            pnt1->vx,
                            pnt1->vy,
                            pnt1->vz,
                            pos);

                        if (height > highest) {
                            *polyptr = (int32_t *)polys;
                            highest = height;
                        }
                    }
                }
            }
        }

next_polygon:
        polys += 2;
    } while ((polys[-2] & UINT32_C(0xc0000000)) == 0);

    return highest;
}

int jpb_JonnyWankCheck(
    _svector *verts,
    int32_t **polyptr,
    VECTOR *pos)
{
    return intersec_WankCheck(verts, polyptr, pos);
}

/* 0xB5240, 533 bytes, global, 8 named locals
 * jon_getlibpart
 * PDB type: int* (int*, VECTOR*, int*)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
int32_t *jon_getlibpart(
    int32_t *cube,
    VECTOR *cubeorg,
    int32_t *mapbase)
{
    _svector *output = (_svector *)gaScratch;
    int32_t *lib;
    const uint16_t *verts;
    uint32_t libword;
    int vs;
    int numun;
    int vst;

    /*
     * The matched code deliberately reads only the low 16 bits of *cube.
     * The upper bits carry flags in several callers.
     */
    lib = mapbase + (uint16_t)*cube;
    libword = (uint32_t)lib[0];
    vs = (int)((libword >> 16) & UINT32_C(0x1f));
    numun = (int)(((uint32_t)lib[1] >> 15) & UINT32_C(0x0f)) + 1;
    verts = (const uint16_t *)(
        (const uint8_t *)mapbase +
        ((libword & UINT32_C(0xffff)) +
         (uint32_t)((mapbase[-1] >> 2) * 2)) *
            2U);

    output[0].vx = (int16_t)(cubeorg->vx + 256);
    output[0].vy = (int16_t)cubeorg->vy;
    output[0].vz = (int16_t)cubeorg->vz;
    output[1].vx = (int16_t)cubeorg->vx;
    output[1].vy = (int16_t)cubeorg->vy;
    output[1].vz = (int16_t)cubeorg->vz;
    output[2].vx = (int16_t)(cubeorg->vx + 256);
    output[2].vy = (int16_t)cubeorg->vy;
    output[2].vz = (int16_t)(cubeorg->vz + 256);
    output[3].vx = (int16_t)cubeorg->vx;
    output[3].vy = (int16_t)cubeorg->vy;
    output[3].vz = (int16_t)(cubeorg->vz + 256);

    output[4].vx = (int16_t)(cubeorg->vx + 256);
    output[4].vy = (int16_t)(cubeorg->vy + numun * 256);
    output[4].vz = (int16_t)cubeorg->vz;
    output[5].vx = (int16_t)cubeorg->vx;
    output[5].vy = output[4].vy;
    output[5].vz = (int16_t)cubeorg->vz;
    output[6].vx = (int16_t)(cubeorg->vx + 256);
    output[6].vy = output[4].vy;
    output[6].vz = (int16_t)(cubeorg->vz + 256);
    output[7].vx = (int16_t)cubeorg->vx;
    output[7].vy = output[4].vy;
    output[7].vz = (int16_t)(cubeorg->vz + 256);

    for (vst = 0; vst < vs; ++vst) {
        uint16_t packed = verts[vst];
        int vertex = vst + 8;

        output[vertex].vx = (int16_t)(
            cubeorg->vx + 256 -
            (int)((packed & UINT16_C(0x001f)) << 4));
        output[vertex].vy = (int16_t)(
            cubeorg->vy +
            (int)(((packed >> 7) & UINT16_C(0x01f8)) * numun));
        output[vertex].vz = (int16_t)(
            cubeorg->vz +
            (int)(((int16_t)packed >> 1) & INT16_C(0x01f0)));
    }
    return lib;
}

/* 0xB5460, 1062 bytes, global, 10 named locals
 * jon_getlibpartfloat
 * PDB type: int* (FVECTOR*, int*, FVECTOR*, ...
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
int32_t *jon_getlibpartfloat(
    FVECTOR *output,
    int32_t *cube,
    FVECTOR *cubeorg,
    int32_t *mapbase,
    int32_t *numverts)
{
    int32_t *lib;
    const uint16_t *verts;
    uint32_t libword;
    int vs;
    int numun;
    int vst;
    int vertex;

    lib = mapbase + (uint16_t)*cube;
    libword = (uint32_t)lib[0];
    vs = (int)((libword >> 16) & UINT32_C(0x1f));
    numun = (int)(((uint32_t)lib[1] >> 15) & UINT32_C(0x0f)) + 1;
    verts = (const uint16_t *)(
        (const uint8_t *)mapbase +
        ((libword & UINT32_C(0xffff)) +
         (uint32_t)((mapbase[-1] >> 2) * 2)) *
            2U);

    output[0].vx = cubeorg->vx + 256.0f;
    output[0].vy = cubeorg->vy;
    output[0].vz = cubeorg->vz;
    output[1] = *cubeorg;
    output[2].vx = cubeorg->vx + 256.0f;
    output[2].vy = cubeorg->vy;
    output[2].vz = cubeorg->vz + 256.0f;
    output[3].vx = cubeorg->vx;
    output[3].vy = cubeorg->vy;
    output[3].vz = cubeorg->vz + 256.0f;

    output[4].vx = cubeorg->vx + 256.0f;
    output[4].vy = cubeorg->vy + (float)(numun * 256);
    output[4].vz = cubeorg->vz;
    output[5].vx = cubeorg->vx;
    output[5].vy = output[4].vy;
    output[5].vz = cubeorg->vz;
    output[6].vx = cubeorg->vx + 256.0f;
    output[6].vy = output[4].vy;
    output[6].vz = cubeorg->vz + 256.0f;
    output[7].vx = cubeorg->vx;
    output[7].vy = output[4].vy;
    output[7].vz = cubeorg->vz + 256.0f;

    for (vst = 0; vst < vs; ++vst) {
        uint16_t packed = verts[vst];

        vertex = vst + 8;
        output[vertex].vx =
            cubeorg->vx + 256.0f -
            (float)((packed & UINT16_C(0x001f)) << 4);
        output[vertex].vy =
            cubeorg->vy +
            (float)(((packed >> 7) & UINT16_C(0x01f8)) * numun);
        output[vertex].vz =
            cubeorg->vz +
            (float)(((int16_t)packed >> 1) & INT16_C(0x01f0));
    }

    *numverts = vs + 8;
    return lib;
}

/* 0xB5890, 451 bytes, global, 8 named locals
 * jon_getlibpartint32_t
 * PDB type: int* (int*, VECTOR*, int*)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */

/* 0xB5A60, 3 bytes, global, 3 named locals
 * jon_otagpos
 * PDB type: int (int, int, int)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
int32_t *jon_getlibpartint32_t(
    int32_t *cube,
    VECTOR *cubeorg,
    int32_t *mapbase)
{
    VECTOR *output = (VECTOR *)(void *)gaScratch;
    int32_t *lib = mapbase + (uint16_t)*cube;
    uint32_t libword = (uint32_t)lib[0];
    int numun = ((lib[1] >> 15) & 0xf) + 1;
    const uint16_t *verts = (const uint16_t *)(
        (const uint8_t *)mapbase +
        ((libword & UINT32_C(0xffff)) +
         (uint32_t)((mapbase[-1] >> 2) * 2)) *
            2U);
    int vs = (int)((libword >> 16) & UINT32_C(0x1f));
    int vst;

    output[0].vx = cubeorg->vx + 256;
    output[0].vy = cubeorg->vy;
    output[0].vz = cubeorg->vz;
    output[1].vx = cubeorg->vx;
    output[1].vy = cubeorg->vy;
    output[1].vz = cubeorg->vz;
    output[2].vx = cubeorg->vx + 256;
    output[2].vy = cubeorg->vy;
    output[2].vz = cubeorg->vz + 256;
    output[3].vx = cubeorg->vx;
    output[3].vy = cubeorg->vy;
    output[3].vz = cubeorg->vz + 256;
    output[4].vx = cubeorg->vx + 256;
    output[4].vy = cubeorg->vy + numun * 256;
    output[4].vz = cubeorg->vz;
    output[5].vx = cubeorg->vx;
    output[5].vy = cubeorg->vy + numun * 256;
    output[5].vz = cubeorg->vz;
    output[6].vx = cubeorg->vx + 256;
    output[6].vy = cubeorg->vy + numun * 256;
    output[6].vz = cubeorg->vz + 256;
    output[7].vx = cubeorg->vx;
    output[7].vy = cubeorg->vy + numun * 256;
    output[7].vz = cubeorg->vz + 256;

    for (vst = 0; vst < vs; ++vst) {
        uint16_t packed = verts[vst];
        VECTOR *vertex = &output[vst + 8];

        vertex->vx =
            cubeorg->vx + 256 -
            (int)((packed & UINT16_C(0x001f)) << 4);
        vertex->vy =
            cubeorg->vy +
            (int)(((packed >> 7) & UINT16_C(0x01f8)) * numun);
        vertex->vz =
            cubeorg->vz +
            (int)(((int16_t)packed >> 1) & INT16_C(0x01f0));
    }
    return lib;
}
int jon_otagpos(int x, int y, int z)
{
    (void)x;
    (void)y;
    (void)z;
    return 0;
}

/* 0xB5A70, 3 bytes, global, 5 named locals
 * jon_plumbgeneral
 * PDB type: int (_svector*, _svector*, unsig...
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */

/* 0xB5A80, 1169 bytes, global, 35 named locals
 * jon_plumbline
 * PDB type: int (void*, void*, VECTOR*, int,...
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
int jon_plumbgeneral(
    _svector *verts,
    _svector *norms,
    unsigned *index,
    int npolys,
    VECTOR *pos)
{
    (void)verts;
    (void)norms;
    (void)index;
    (void)npolys;
    (void)pos;
    return 0;
}
int jon_plumbline(
    void *mapbase_pointer,
    void *pointspace,
    VECTOR *pos,
    int minheight,
    struct _jheightstuff *results)
{
    int32_t *mapbase = (int32_t *)mapbase_pointer;
    uint32_t mx;
    int slot;
    int32_t cell;
    int32_t *cube;
    VECTOR cubby;

    (void)pointspace;
    results->cube = NULL;

    mx = (UINT32_C(0x8100) - (uint32_t)pos->vx) >> 8;
    {
        uint32_t wrapped_slot =
            ((uint32_t)pos->vz + UINT32_C(0x7f00)) >> 8;

        memcpy(&slot, &wrapped_slot, sizeof(slot));
    }
    if ((mx & UINT32_C(0xff)) != mx ||
        slot < 0 ||
        slot >= (mapbase[-2] >> 10)) {
        return 0;
    }

    cell = mapbase[(int)mx + slot * 0x100];
    if (cell >= 0) {
        return 0;
    }

    cube = mapbase + ((uint32_t)cell & UINT32_C(0x1ffff));
    cubby.vx =
        (int32_t)((uint32_t)(pos->vx - 1) & UINT32_C(0xffffff00));
    cubby.vz =
        (int32_t)((uint32_t)pos->vz & UINT32_C(0xffffff00));

    for (;;) {
        uint32_t cubword = (uint32_t)cube[0];
        int entry_words = (int)((cubword >> 26) & UINT32_C(0x0f));

        cubby.vy = (int)(cubword & UINT32_C(0x7f)) << 8;
        if ((cubword & UINT32_C(0x3c000000)) == 0) {
            int32_t *poly =
                leveldata +
                (leveldata[-4] >> 11) +
                (int)(((cubword >> 14) & UINT32_C(0xff)) * 9U);

            if (((uint32_t)poly[0] & UINT32_C(0x40000000)) == 0) {
                int16_t anchor_x =
                    (int16_t)(-0x7f00 - jonny_read_i16(poly, 24));
                int16_t anchor_y = jonny_read_i16(poly, 14);
                int16_t anchor_z =
                    (int16_t)(jonny_read_i16(poly, 26) - 0x7f00);
                int16_t opposite_x =
                    (int16_t)(-0x7f00 - jonny_read_i16(poly, 28));
                int16_t opposite_z =
                    (int16_t)(jonny_read_i16(poly, 30) - 0x7f00);
                int16_t candidate_x =
                    (int16_t)(-0x7f00 - jonny_read_i16(poly, 20));
                int16_t candidate_z =
                    (int16_t)(jonny_read_i16(poly, 22) - 0x7f00);
                int16_t edge_a_x = anchor_x;
                int16_t edge_a_z = anchor_z;
                int16_t edge_b_x = opposite_x;
                int16_t edge_b_z = opposite_z;

                if (jonny_wrapped_orient_xz(
                        pos->vx,
                        pos->vz,
                        anchor_x,
                        anchor_z,
                        opposite_x,
                        opposite_z) < 0) {
                    candidate_x =
                        (int16_t)(
                            -0x7f00 - jonny_read_i16(poly, 32));
                    candidate_z =
                        (int16_t)(
                            jonny_read_i16(poly, 34) - 0x7f00);
                    edge_a_x = opposite_x;
                    edge_a_z = opposite_z;
                    edge_b_x = anchor_x;
                    edge_b_z = anchor_z;
                }

                if (jonny_wrapped_orient_xz(
                        pos->vx,
                        pos->vz,
                        edge_a_x,
                        edge_a_z,
                        candidate_x,
                        candidate_z) <= 0 &&
                    jonny_wrapped_orient_xz(
                        pos->vx,
                        pos->vz,
                        candidate_x,
                        candidate_z,
                        edge_b_x,
                        edge_b_z) <= 0) {
                    uint32_t packed_normal =
                        (uint32_t)leveldata[
                            ((uint32_t)poly[0] &
                             UINT32_C(0x1ffff)) +
                            1U];
                    int normal_y =
                        jonny_sign_extend_10(
                            packed_normal >> 10) *
                        8;

                    if (normal_y != 0) {
                        int height = jonny_plane_height(
                            packed_normal,
                            anchor_x,
                            anchor_y,
                            anchor_z,
                            pos);

                        if (height < pos->vy + 0x40 &&
                            minheight < height) {
                            results->cube = cube;
                            results->entry = NULL;
                            results->poly = poly;
                            minheight = height;
                        }
                    }
                }
            }
        } else {
            int32_t *entry = cube + 1;
            int32_t *entryend = cube + entry_words;

            while (entry < entryend) {
                int32_t *lib;
                int32_t *polys;
                int height;

                ++entry;
                lib = jon_getlibpart(entry, &cubby, mapbase);
                polys = lib + 2;
                height = intersec_WankCheck(
                    (_svector *)gaScratch, &polys, pos);
                if (height < pos->vy + 0x40 &&
                    minheight < height) {
                    results->cube = cube;
                    results->entry = entry;
                    results->poly = polys;
                    minheight = height;
                }
                entry += (uint32_t)*entry >> 30;
            }
        }

        cube += entry_words + 1;
        if ((cubword & UINT32_C(0x40000000)) != 0) {
            return minheight;
        }
    }
}

/* 0xB5F20, 3 bytes, global, 2 named locals
 * jon_texscroll
 * PDB type: void (void*, int)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
void jon_texscroll(void *mapbase, int frame_step)
{
    (void)mapbase;
    (void)frame_step;
}

/* 0xB5F30, 4 bytes, global, 11 named locals
 * jpb_render
 * PDB type: void* (MATRIX*, void*, void*, in...
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */

/* 0xB5F40, 938 bytes, global, 16 named locals
 * makecull
 * PDB type: void (FVECTOR4*, MATRIX*, FVECTO...
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
void *jpb_render(
    MATRIX *cammat,
    void *mapbase,
    void *pbuf,
    int globaltimer,
    void *otagbase,
    int showmask,
    int minx,
    int maxx,
    int miny,
    int maxy,
    int subdisable)
{
    (void)cammat;
    (void)mapbase;
    (void)globaltimer;
    (void)otagbase;
    (void)showmask;
    (void)minx;
    (void)maxx;
    (void)miny;
    (void)maxy;
    (void)subdisable;
    return pbuf;
}
void makecull(
    FVECTOR4 *planes,
    MATRIX *camera,
    FVECTOR *camera_position,
    float clip_radius,
    float screen_width,
    float screen_height,
    float screen_distance,
    float far_clip)
{
    float half_width = screen_width * 0.5f;
    float half_height = screen_height * 0.5f;
    float hyp_x = (float)sqrt((double)(
        half_width * half_width + screen_distance * screen_distance));
    float hyp_y = (float)sqrt((double)(
        half_height * half_height + screen_distance * screen_distance));
    float width_normal = half_width / hyp_x;
    float height_normal = half_height / hyp_y;
    float distance_x = screen_distance / hyp_x;
    float distance_y = screen_distance / hyp_y;
    float depth_x = camera->m[2][0];
    float depth_y = camera->m[2][1];
    float depth_z = camera->m[2][2];
    float depth_plane;

    planes[0].vx = distance_x * camera->m[0][0] +
                   width_normal * depth_x;
    planes[0].vy = distance_x * camera->m[0][1] +
                   width_normal * depth_y;
    planes[0].vz = distance_x * camera->m[0][2] +
                   width_normal * depth_z;
    planes[0].vw = 1.0f -
        (planes[0].vx * camera_position->vx +
         planes[0].vy * camera_position->vy +
         planes[0].vz * camera_position->vz) +
        clip_radius;

    planes[1].vx = width_normal * depth_x -
                   distance_x * camera->m[0][0];
    planes[1].vy = width_normal * depth_y -
                   distance_x * camera->m[0][1];
    planes[1].vz = width_normal * depth_z -
                   distance_x * camera->m[0][2];
    planes[1].vw = 1.0f -
        (planes[1].vx * camera_position->vx +
         planes[1].vy * camera_position->vy +
         planes[1].vz * camera_position->vz) +
        clip_radius;

    planes[2].vx = height_normal * depth_x +
                   distance_y * camera->m[1][0];
    planes[2].vy = height_normal * depth_y +
                   distance_y * camera->m[1][1];
    planes[2].vz = height_normal * depth_z +
                   distance_y * camera->m[1][2];
    planes[2].vw = 1.0f -
        (planes[2].vx * camera_position->vx +
         planes[2].vy * camera_position->vy +
         planes[2].vz * camera_position->vz) +
        clip_radius;

    depth_plane = 1.0f -
        (depth_x * camera_position->vx +
         depth_y * camera_position->vy +
         depth_z * camera_position->vz) +
        clip_radius;
    planes[3].vx = depth_x;
    planes[3].vy = depth_y;
    planes[3].vz = depth_z;
    planes[3].vw = depth_plane;
    planes[6] = planes[3];

    planes[4].vx = height_normal * depth_x -
                   distance_y * camera->m[1][0];
    planes[4].vy = height_normal * depth_y -
                   distance_y * camera->m[1][1];
    planes[4].vz = height_normal * depth_z -
                   distance_y * camera->m[1][2];
    planes[4].vw = 1.0f -
        (planes[4].vx * camera_position->vx +
         planes[4].vy * camera_position->vy +
         planes[4].vz * camera_position->vz) +
        clip_radius;

    planes[5].vx = jonny_toggle_float_sign(depth_x);
    planes[5].vy = jonny_toggle_float_sign(depth_y);
    planes[5].vz = jonny_toggle_float_sign(depth_z);
    planes[5].vw = far_clip -
        (planes[5].vx * camera_position->vx +
         planes[5].vy * camera_position->vy +
         planes[5].vz * camera_position->vz) +
        1.0f + clip_radius;
}

/* 0xB62F0, 122 bytes, global, 6 named locals
 * restore_events
 * PDB type: void (int*)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
void restore_events(int32_t *mapbase)
{
    int32_t *initial_next = eventlist_next;
    int32_t *cursor = initial_next;

    for (;;) {
        int32_t *entry = cursor - 2;
        uint32_t byte_offset;

        if (entry == eventlist_start) {
            entry = eventlist_end - 2;
        }
        byte_offset = (uint32_t)entry[0] << 2;
        if (byte_offset == 0) {
            break;
        }
        *(uint16_t *)((uint8_t *)mapbase + byte_offset) =
            (uint16_t)entry[1];
        cursor = entry;
        if (cursor == initial_next) {
            break;
        }
    }
    eventlist_start[0] = 0;
    eventlist_start[1] = 0;
    eventlist_next = eventlist_start + 2;
}

/* 0xB6370, 3 bytes, global, 1 named locals
 * set_camera
 * PDB type: void (MATRIX*)
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
void set_camera(MATRIX *matrix)
{
    (void)matrix;
}

/* 0xB6380, 3 bytes, global, 3 named locals
 * spack_frustrum
 * PDB type: void (MATRIX*, _svector*, VECTOR...
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
void spack_frustrum(
    MATRIX *cammat,
    _svector *fuckoff,
    VECTOR *camtwat)
{
    (void)cammat;
    (void)fuckoff;
    (void)camtwat;
}

/* 0xB6390, 3 bytes, global, 3 named locals
 * spackdivver_frustrum
 * PDB type: void (MATRIX*, _svector*, VECTOR...
 * Source: W:\SWJediPowerBattles\work\jonny.c
 */
void spackdivver_frustrum(
    MATRIX *cammat,
    _svector *fuckoff,
    VECTOR *camtwat)
{
    (void)cammat;
    (void)fuckoff;
    (void)camtwat;
}
