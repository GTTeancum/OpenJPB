/*
 * REVIEWED RECONSTRUCTION.
 * PDB module: 0044
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\intersec.obj
 * Primary source: W:\SWJediPowerBattles\Work\intersec.c
 * Compiler language: c
 * Emitted procedures: 15
 *
 * All 15 emitted procedures are recovered in original PDB order. Map and
 * dynamic-solid raycasts retain the executable's packed world encodings and
 * output-pointer roles; local typed scratch replaces the shared temporary
 * byte arena without changing valid runtime behavior.
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/intersec.h"

#include "jpb/animctrl.h"
#include "jpb/bmd.h"
#include "jpb/camera.h"
#include "jpb/flex.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/jonny.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

/* Exact intersec.c module static at RVA 0x5380E0. */
static _solid *whichsolid;

_solid *jpb_IntersecGetWhichSolid(void)
{
    return whichsolid;
}

static int32_t intersec_wrapped_orient_xz(
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

    memcpy(&signed_result, &result, sizeof(signed_result));
    return signed_result;
}

static int32_t intersec_sign_extend_10(uint32_t value)
{
    value &= UINT32_C(0x3ff);
    if ((value & UINT32_C(0x200)) != 0) {
        return (int32_t)value - 0x400;
    }
    return (int32_t)value;
}

static int32_t intersec_trunc_float_to_i32(float value)
{
    if (!(value >= -2147483648.0f && value < 2147483648.0f)) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int16_t intersec_read_i16(const void *base, size_t offset)
{
    int16_t value;

    memcpy(&value, (const uint8_t *)base + offset, sizeof(value));
    return value;
}

static uint16_t intersec_read_u16(const void *base, size_t offset)
{
    uint16_t value;

    memcpy(&value, (const uint8_t *)base + offset, sizeof(value));
    return value;
}

static void intersec_decode_map_normal(
    const int32_t *mapbase,
    uint32_t normal_index,
    FVECTOR *normal)
{
    uint32_t packed = (uint32_t)mapbase[normal_index + 1U];

    normal->vx =
        (float)(intersec_sign_extend_10(packed) * 8) *
        (1.0f / 4096.0f);
    normal->vy =
        (float)(intersec_sign_extend_10(packed >> 10) * 8) *
        (1.0f / 4096.0f);
    normal->vz =
        (float)(intersec_sign_extend_10(packed >> 20) * 8) *
        (1.0f / 4096.0f);
}

static int raycastpoly(
    FVECTOR *points,
    int sides,
    FVECTOR *normal,
    FVECTOR *start,
    FVECTOR *direction,
    float length);
static int raycheckgeneral(
    _svector *verts,
    _svector *normals,
    int16_t *index,
    int npolys,
    FVECTOR *start,
    FVECTOR *direction,
    float length,
    int *polyindex);

static int intersec_GeneralCheck(
    _svector *verts,
    _svector *norms,
    int16_t *index,
    int npolys,
    VECTOR *pos);

/* 0xAF1A0, 19 bytes, global, 6 named locals
 * HitSomething
 * PDB type: int (VECTOR*, int*, int*, int*, ...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
int HitSomething(
    VECTOR *pos,
    int *cube,
    int *entry,
    int *poly,
    int type,
    int force)
{
    (void)cube;
    (void)poly;
    (void)type;
    return BlowUp(entry, pos, force);
}

/* 0xAF1C0, 148 bytes, global, 7 named locals
 * LineAndPlane
 * PDB type: float (FVECTOR4*, FVECTOR*, FVEC...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
float LineAndPlane(
    FVECTOR4 *plane,
    FVECTOR *start,
    FVECTOR *dir,
    float length,
    int padding)
{
    float moveToPlaneDot = plane->vy * dir->vy;
    float distToPlane;

    moveToPlaneDot += plane->vx * dir->vx;
    moveToPlaneDot += plane->vz * dir->vz;
    moveToPlaneDot = -moveToPlaneDot;
    if (!(moveToPlaneDot > 0.0f)) {
        return length;
    }

    distToPlane = plane->vy * start->vy;
    distToPlane += plane->vx * start->vx;
    distToPlane += plane->vz * start->vz;
    distToPlane -= plane->vw;
    distToPlane -= (float)padding;
    if (distToPlane < 0.0f) {
        return 0.0f;
    }
    distToPlane /= moveToPlaneDot;
    return distToPlane < length ? distToPlane : length;
}

/* 0xAF260, 299 bytes, global, 10 named locals
 * MoveObject
 * PDB type: int (_mvector*, VECTOR*, _hit_am...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
int MoveObject(
    _mvector *movement,
    VECTOR *curpos,
    _hit_amount force)
{
    FVECTOR cpos = {
        (float)curpos->vx,
        (float)curpos->vy,
        (float)curpos->vz
    };
    FVECTOR blob = {
        (float)movement->vx * (1.0f / 4096.0f),
        (float)movement->vy * (1.0f / 4096.0f),
        (float)movement->vz * (1.0f / 4096.0f)
    };
    int *cube;
    int *entry;
    int *poly;
    int len;
    int hit;

    hit = RaycastCheck(
        &cpos,
        &blob,
        (float)flexmul((int)movement->speed, gGlobalFrameRate),
        &cube,
        &entry,
        &poly,
        &len,
        &cpos);
    curpos->vx = intersec_trunc_float_to_i32(cpos.vx);
    curpos->vy = intersec_trunc_float_to_i32(cpos.vy);
    curpos->vz = intersec_trunc_float_to_i32(cpos.vz);
    if (hit != 0) {
        return HitSomething(
            curpos, cube, entry, poly, hit, force);
    }
    return -1;
}

/* 0xAF390, 474 bytes, global, 12 named locals
 * MoveObjectNormal
 * PDB type: int (_mvector*, VECTOR*, _hit_am...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
int MoveObjectNormal(
    _mvector *movement,
    VECTOR *curpos,
    _hit_amount force,
    _svector *normal)
{
    FVECTOR blob3 = {
        (float)curpos->vx,
        (float)curpos->vy,
        (float)curpos->vz
    };
    FVECTOR blob2 = {
        (float)movement->vx * (1.0f / 4096.0f),
        (float)movement->vy * (1.0f / 4096.0f),
        (float)movement->vz * (1.0f / 4096.0f)
    };
    int *cube;
    int *entry;
    int *poly;
    int len;
    int hittype;
    int result;

    hittype = RaycastCheck(
        &blob3,
        &blob2,
        (float)movement->speed * fGlobalFrameRate,
        &cube,
        &entry,
        &poly,
        &len,
        &blob3);
    curpos->vx = intersec_trunc_float_to_i32(blob3.vx);
    curpos->vy = intersec_trunc_float_to_i32(blob3.vy);
    curpos->vz = intersec_trunc_float_to_i32(blob3.vz);
    if (hittype == 0) {
        return -1;
    }

    result = HitSomething(
        curpos, cube, entry, poly, hittype, force);
    if (normal != NULL) {
        if (hittype == 1) {
            uint32_t packed =
                (uint32_t)leveldata[((uint32_t)*poly &
                                     UINT32_C(0x1ffff)) + 1U];

            normal->vx = (int16_t)(
                intersec_sign_extend_10(packed) * 8);
            normal->vy = (int16_t)(
                intersec_sign_extend_10(packed >> 10) * 8);
            normal->vz = (int16_t)(
                intersec_sign_extend_10(packed >> 20) * 8);
        } else if (hittype == 4) {
            memcpy(normal, poly, 3U * sizeof(int16_t));
        }
    }
    return result;
}

/* 0xAF570, 2626 bytes, global, 45 named locals
 * RaycastCheck
 * PDB type: int (FVECTOR*, FVECTOR*, float, ...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
int RaycastCheck(
    FVECTOR *startpos,
    FVECTOR *direction,
    float length,
    int **cube_result,
    int **entry_result,
    int **poly_result,
    int *len,
    FVECTOR *hitpoint)
{
    FVECTOR endpos = {
        startpos->vx + direction->vx * length,
        startpos->vy + direction->vy * length,
        startpos->vz + direction->vz * length
    };
    FVECTOR points[40];
    FVECTOR face[4];
    FVECTOR normal;
    FVECTOR cubeorg;
    float min_x = direction->vx > 0.0f ? startpos->vx : endpos.vx;
    float max_x = direction->vx > 0.0f ? endpos.vx : startpos->vx;
    float min_y = direction->vy > 0.0f ? startpos->vy : endpos.vy;
    float max_y = direction->vy > 0.0f ? endpos.vy : startpos->vy;
    float min_z = direction->vz > 0.0f ? startpos->vz : endpos.vz;
    float max_z = direction->vz > 0.0f ? endpos.vz : startpos->vz;
    int x_start = (int32_t)(
        UINT32_C(0x80ff) -
        (uint32_t)intersec_trunc_float_to_i32(min_x)) >> 8;
    int x_end = (int32_t)(
        UINT32_C(0x80ff) -
        (uint32_t)intersec_trunc_float_to_i32(max_x)) >> 8;
    int z_start = (int32_t)(
        (uint32_t)intersec_trunc_float_to_i32(min_z) +
        UINT32_C(0x7f00)) >> 8;
    int z_end = (int32_t)(
        (uint32_t)intersec_trunc_float_to_i32(max_z) +
        UINT32_C(0x7f00)) >> 8;
    int *whichcube = NULL;
    int *whichentry = NULL;
    int *whichpoly = NULL;
    int bestsofar = intersec_trunc_float_to_i32(length);
    int hittype = 0;
    int z;

    cubeorg.vx = (float)(int32_t)(
        (uint32_t)intersec_trunc_float_to_i32(min_x) &
        UINT32_C(0xffffff00));
    cubeorg.vy = (float)(int32_t)(
        (uint32_t)intersec_trunc_float_to_i32(min_y) &
        UINT32_C(0xffffff00));
    cubeorg.vz = (float)(int32_t)(
        (uint32_t)intersec_trunc_float_to_i32(min_z) &
        UINT32_C(0xffffff00));

    for (z = z_start; z <= z_end; ++z) {
        int x;

        cubeorg.vx = (float)(int32_t)(
            (uint32_t)intersec_trunc_float_to_i32(min_x) &
            UINT32_C(0xffffff00));
        for (x = x_start; x >= x_end; --x) {
            if ((unsigned)x < 0x100U &&
                z >= 0 && z < mapyend) {
                int32_t cell = leveldata[x + z * 0x100];

                if (cell < 0) {
                    int32_t *cube =
                        leveldata +
                        ((uint32_t)cell & UINT32_C(0x1ffff));

                    for (;;) {
                        uint32_t thiscube = (uint32_t)cube[0];
                        int lastcube =
                            (thiscube & UINT32_C(0x40000000)) != 0;
                        int cubebase =
                            (int)(thiscube & UINT32_C(0x7f)) * 0x100;
                        int cubetop =
                            cubebase +
                            (int)((thiscube >> 2) & UINT32_C(0xfe0));
                        int32_t *nextcube =
                            cube + 1 +
                            ((thiscube >> 26) & UINT32_C(0x0f));

                        if (max_y < (float)cubebase) {
                            break;
                        }
                        if (min_y <= (float)cubetop) {
                            cubeorg.vy = (float)cubebase;
                            if (nextcube == cube + 1) {
                                int32_t *fat =
                                    leveldata +
                                    (leveldata[-4] >> 11) +
                                    (int)((thiscube >> 14) &
                                          UINT32_C(0xff)) * 9;

                                if (fat[0] >= 0) {
                                    int distance;

                                    intersec_decode_map_normal(
                                        leveldata,
                                        (uint32_t)fat[0] &
                                            UINT32_C(0x1ffff),
                                        &normal);
                                    face[0].vx = (float)(
                                        0x8100 -
                                        (int)intersec_read_u16(fat, 20));
                                    face[0].vy = (float)(
                                        intersec_read_i16(fat, 22) -
                                        0x7f00);
                                    face[0].vz = (float)fat[3];
                                    face[1].vx = (float)(
                                        0x8100 -
                                        (int)intersec_read_u16(fat, 24));
                                    face[1].vy = (float)(
                                        intersec_read_i16(fat, 26) -
                                        0x7f00);
                                    face[1].vz = (float)
                                        intersec_read_i16(fat, 14);
                                    face[2].vx = (float)(
                                        0x8100 -
                                        (int)intersec_read_u16(fat, 28));
                                    face[2].vy = (float)(
                                        intersec_read_i16(fat, 30) -
                                        0x7f00);
                                    face[2].vz = (float)fat[4];
                                    face[3].vx = (float)(
                                        0x8100 -
                                        (int)intersec_read_u16(fat, 32));
                                    face[3].vy = (float)(
                                        intersec_read_i16(fat, 34) -
                                        0x7f00);
                                    face[3].vz = (float)
                                        intersec_read_i16(fat, 18);
                                    distance = raycastpoly(
                                        face,
                                        4,
                                        &normal,
                                        startpos,
                                        direction,
                                        length);
                                    if (distance < bestsofar) {
                                        hittype = 3;
                                        whichcube = cube + 1;
                                        whichentry = NULL;
                                        whichpoly = fat;
                                        bestsofar = distance;
                                    }
                                }
                            } else {
                                int32_t *entry = cube + 2;

                                while (entry < nextcube) {
                                    int nump;
                                    int32_t *libpart =
                                        jon_getlibpartfloat(
                                            points,
                                            entry,
                                            &cubeorg,
                                            leveldata,
                                            &nump);
                                    int32_t *poly = libpart + 2;

                                    (void)nump;
                                    for (;;) {
                                        uint32_t poly0 =
                                            (uint32_t)poly[0];
                                        uint32_t poly1 =
                                            (uint32_t)poly[1];
                                        int endpoly =
                                            (poly0 &
                                             UINT32_C(0xc0000000)) != 0;

                                        if ((poly1 &
                                             UINT32_C(0xc0000000)) == 0) {
                                            int istri =
                                                (int)((poly1 >> 20) & 1U);
                                            int distance;

                                            intersec_decode_map_normal(
                                                leveldata,
                                                poly0 &
                                                    UINT32_C(0x1ffff),
                                                &normal);
                                            face[0] =
                                                points[poly1 & 0x1fU];
                                            face[1] =
                                                points[(poly1 >> 5) & 0x1fU];
                                            face[2] =
                                                points[(poly1 >> 10) & 0x1fU];
                                            if (istri == 0) {
                                                face[3] = face[2];
                                                face[2] = points[
                                                    (poly1 >> 15) & 0x1fU];
                                            }
                                            distance = raycastpoly(
                                                face,
                                                4 - istri,
                                                &normal,
                                                startpos,
                                                direction,
                                                length);
                                            if (distance < bestsofar) {
                                                hittype = 1;
                                                whichcube = cube + 2;
                                                whichentry = entry;
                                                whichpoly = poly;
                                                bestsofar = distance;
                                            }
                                        }
                                        poly += 2;
                                        if (endpoly) {
                                            break;
                                        }
                                    }
                                    entry +=
                                        ((uint32_t)entry[0] >> 30) + 1U;
                                }
                            }
                        }
                        if (lastcube) {
                            break;
                        }
                        cube = nextcube;
                    }
                }
            }
            cubeorg.vx += 256.0f;
        }
        cubeorg.vz += 256.0f;
    }

    {
        int slot;

        for (slot = 0; slot < JPB_PHYSICS_CAPACITY; ++slot) {
            physicsObject *physics = &maPhysicsData[slot];
            _solid *s;

            if (physics->physicsRoot.objectID == -1 ||
                obj_gCheckObjectFlag(
                    &physics->physicsRoot,
                    0,
                    UINT32_C(0x20)) != 0) {
                continue;
            }
            s = physics->solid;
            if (s != NULL &&
                s->coords != NULL &&
                s->normals != NULL &&
                s->geometry != NULL) {
                int16_t *index =
                    (int16_t *)jpb_PhysicsResolveGeometryStream(
                        s->geometry,
                        JPB_POINTER_ARRAY_INDEX);

                if (index != NULL) {
                    int polynum;
                    int hitlen = raycheckgeneral(
                        s->coords,
                        s->normals,
                        index,
                        s->geometry->numFaces,
                        startpos,
                        direction,
                        length,
                        &polynum);

                    if (hitlen < bestsofar) {
                        _svector *norm = &s->normals[polynum];

                        hittype = 4;
                        whichcube = NULL;
                        whichentry = (int *)(void *)s;
                        whichpoly = (int *)(void *)norm;
                        bestsofar = hitlen;
                    }
                }
            }
        }
    }

    if (hitpoint != NULL) {
        float distance = (float)bestsofar;

        hitpoint->vx = startpos->vx + direction->vx * distance;
        hitpoint->vy = startpos->vy + direction->vy * distance;
        hitpoint->vz = startpos->vz + direction->vz * distance;
    }
    if (poly_result != NULL) {
        *poly_result = whichpoly;
    }
    if (entry_result != NULL) {
        *entry_result = whichentry;
    }
    if (cube_result != NULL) {
        *cube_result = whichcube;
    }
    if (len != NULL) {
        *len = bestsofar;
    }
    return hittype;
}

/* 0xAFFC0, 239 bytes, global, 12 named locals
 * RaycastCheckSV
 * PDB type: int (_svector*, _svector*, int, ...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */

int RaycastCheckSV(
    _svector *startpos,
    _svector *direction,
    int length,
    int **cube,
    int **entry,
    int **poly,
    int *len,
    _svector *hitpoint)
{
    FVECTOR sp = {
        (float)startpos->vx,
        (float)startpos->vy,
        (float)startpos->vz
    };
    FVECTOR dir = {
        (float)direction->vx * (1.0f / 4096.0f),
        (float)direction->vy * (1.0f / 4096.0f),
        (float)direction->vz * (1.0f / 4096.0f)
    };
    FVECTOR hp;
    int result = RaycastCheck(
        &sp,
        &dir,
        (float)length,
        cube,
        entry,
        poly,
        len,
        &hp);

    if (hitpoint != NULL) {
        hitpoint->vx = (int16_t)intersec_trunc_float_to_i32(hp.vx);
        hitpoint->vy = (int16_t)intersec_trunc_float_to_i32(hp.vy);
        hitpoint->vz = (int16_t)intersec_trunc_float_to_i32(hp.vz);
    }
    return result;
}

/* 0xB00B0, 420 bytes, global, 7 named locals
 * cliptofrustrum
 * PDB type: int (FVECTOR4*, FVECTOR*, int, i...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
int cliptofrustrum(
    FVECTOR4 *frustrum,
    FVECTOR *pos,
    int radius,
    int *distances)
{
    float r = (float)radius;
    int m = 0;
    int plane;

    for (plane = 0; plane < 5; ++plane) {
        FVECTOR4 *current = &frustrum[plane];
        float d;

        /*
         * The first optimized plane accumulates y+x+z; the remaining four
         * accumulate x+y+z. Keep those independently rounded float steps.
         */
        if (plane == 0) {
            d = pos->vy * current->vy;
            d += pos->vx * current->vx;
        } else {
            d = pos->vx * current->vx;
            d += pos->vy * current->vy;
        }
        d += pos->vz * current->vz;
        d -= current->vw;
        if (distances != NULL) {
            distances[plane] = intersec_trunc_float_to_i32(d);
        }
        m = (m << 1) | (r > d ? 1 : 0);
    }
    return m;
}

/* 0xB0260, 526 bytes, global, 7 named locals
 * cliptofrustrumSV
 * PDB type: int (FVECTOR4*, _svector*, int, ...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
int cliptofrustrumSV(
    FVECTOR4 *frustrum,
    _svector *pos,
    int radius,
    int *distances)
{
    float r = (float)radius;
    int m = 0;
    int plane;

    for (plane = 0; plane < 5; ++plane) {
        FVECTOR4 *current = &frustrum[plane];
        float d;

        if (plane == 0) {
            d = (float)pos->vy * current->vy;
            d += (float)pos->vx * current->vx;
        } else {
            d = (float)pos->vx * current->vx;
            d += (float)pos->vy * current->vy;
        }
        d += (float)pos->vz * current->vz;
        d -= current->vw;
        if (distances != NULL) {
            distances[plane] = intersec_trunc_float_to_i32(d);
        }
        m = (m << 1) | (r > d ? 1 : 0);
    }
    return m;
}

/* 0xB0470, 742 bytes, global, 17 named locals
 * intersec_FindWalkHeight
 * PDB type: int (VECTOR*, VECTOR*, objectRoo...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
int intersec_FindWalkHeight(
    VECTOR *pos,
    VECTOR *normal,
    objectRoot *object,
    unsigned flags)
{
    playerObject *player = NULL;
    physicsObject *physics = NULL;
    _jheightstuff localheightstuff;
    _jheightstuff *heightstuff;
    int minheight = 0;
    int height;
    int standingon;

    if ((flags & 1U) != 0) {
        /*
         * This is an exact but unusual original convention: with bit zero
         * set, `object` is already a caller-owned _jheightstuff pointer.
         */
        heightstuff =
            object != NULL
                ? (_jheightstuff *)(void *)object
                : &localheightstuff;
    } else {
        if (object != NULL && object->pParent != NULL) {
            sceneObject *scene = (sceneObject *)object->pParent;

            physics = (physicsObject *)scene->pPhysics;
            player = (playerObject *)scene->pPlayer;
        }
        heightstuff =
            physics != NULL
                ? &physics->currentmapinfo
                : &localheightstuff;
    }

    memset(heightstuff, 0, sizeof(*heightstuff));
    height = jon_plumbline(
        leveldata, NULL, pos, minheight, heightstuff);
    standingon = height != 0;

    if (player != NULL &&
        player->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(
            &player->playerRoot, 0, UINT32_C(0x20)) == 0 &&
        (flags & 1U) == 0 &&
        numsolids != 0) {
        int id = player->playerRoot.objectID;
        int i;

        for (i = 2; i < JPB_PHYSICS_CAPACITY; ++i) {
            physicsObject *candidate_physics = &maPhysicsData[i];
            _solid *s;

            if (i == id ||
                candidate_physics->physicsRoot.objectID == -1 ||
                obj_gCheckObjectFlag(
                    &candidate_physics->physicsRoot,
                    0,
                    UINT32_C(0x20)) != 0) {
                continue;
            }
            s = candidate_physics->solid;
            if (s == NULL ||
                s->coords == NULL ||
                (s->flags & (UINT32_C(1) << (id & 31))) == 0 ||
                s->geometry == NULL) {
                continue;
            }

            {
                int16_t *index =
                    (int16_t *)jpb_PhysicsResolveGeometryStream(
                        s->geometry, JPB_POINTER_ARRAY_INDEX);
                int polyY;

                if (index == NULL || s->normals == NULL) {
                    continue;
                }
                polyY = intersec_GeneralCheck(
                    s->coords,
                    s->normals,
                    index,
                    s->geometry->numFaces,
                    pos);
                if (height < polyY && polyY < pos->vy + 0x40) {
                    standingon = 3;
                    memset(heightstuff, 0, sizeof(*heightstuff));
                    whichsolid = s;
                    height = polyY;
                }
            }
        }
    }

    if (height == 0) {
        height = -0x7ff8;
    } else if (standingon == 1) {
        if (heightstuff->poly != NULL) {
            uint32_t tag_index =
                (uint32_t)*heightstuff->poly & UINT32_C(0x1ffff);
            int32_t *tag = leveldata + tag_index;

            if (((uint32_t)tag[0] & UINT32_C(0x8000)) == 0) {
                if (physics != NULL &&
                    pos->vy - height < 0x80) {
                    physics->mapinfo = *heightstuff;
                    physics->solidgrabbed = NULL;
                }
                if (normal != NULL) {
                    uint32_t packed = (uint32_t)tag[1];

                    normal->vx =
                        intersec_sign_extend_10(packed) * 8;
                    normal->vy =
                        intersec_sign_extend_10(packed >> 10) * 8;
                    normal->vz =
                        intersec_sign_extend_10(packed >> 20) * 8;
                }
            } else {
                height = -0x8000;
            }
        }
    } else if (standingon == 3 && physics != NULL) {
        physics->solidgrabbed = whichsolid->physics;
    }
    return height;
}

/* 0xB0760, 73 bytes, global, 5 named locals
 * intersec_FindWalkHeightFV
 * PDB type: int (FVECTOR*, VECTOR*, objectRo...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
int intersec_FindWalkHeightFV(
    FVECTOR *pos,
    VECTOR *normal,
    objectRoot *object,
    unsigned flags)
{
    VECTOR crap = {
        (int32_t)pos->vx,
        (int32_t)pos->vy,
        (int32_t)pos->vz,
        0
    };

    return intersec_FindWalkHeight(&crap, normal, object, flags);
}

/* 0xB07B0, 70 bytes, global, 5 named locals
 * intersec_FindWalkHeightSV
 * PDB type: int (_svector*, VECTOR*, objectR...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
int intersec_FindWalkHeightSV(
    _svector *pos,
    VECTOR *normal,
    objectRoot *object,
    unsigned flags)
{
    VECTOR crap = {pos->vx, pos->vy, pos->vz, 0};

    return intersec_FindWalkHeight(&crap, normal, object, flags);
}

/* 0xB0800, 517 bytes, local, 24 named locals
 * intersec_GeneralCheck
 * PDB type: int (_svector*, _svector*, short...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
static int intersec_GeneralCheck(
    _svector *verts,
    _svector *norms,
    int16_t *index,
    int npolys,
    VECTOR *pos)
{
    int highest = INT_MIN;
    int i;

    for (i = 0; i < npolys; ++i, index += 4) {
        _svector *pnt1;
        _svector *pnt2;
        _svector *edge_a;
        _svector *edge_b;
        int candidate_index;

        if (norms[i].vy <= 0) {
            continue;
        }
        pnt1 = &verts[index[1]];
        pnt2 = &verts[index[2]];
        edge_a = pnt1;
        edge_b = pnt2;
        candidate_index = index[0];

        if (intersec_wrapped_orient_xz(
                pos->vx,
                pos->vz,
                pnt1->vx,
                pnt1->vz,
                pnt2->vx,
                pnt2->vz) < 0) {
            candidate_index = index[3];
            edge_a = pnt2;
            edge_b = pnt1;
            if (candidate_index == 0x7fff) {
                continue;
            }
        }

        {
            _svector *pnt0 = &verts[candidate_index];

            if (intersec_wrapped_orient_xz(
                    pos->vx,
                    pos->vz,
                    edge_a->vx,
                    edge_a->vz,
                    pnt0->vx,
                    pnt0->vz) <= 0 &&
                intersec_wrapped_orient_xz(
                    pos->vx,
                    pos->vz,
                    pnt0->vx,
                    pnt0->vz,
                    edge_b->vx,
                    edge_b->vz) <= 0) {
                int polyY =
                    (norms[i].vz * (pnt1->vz - pos->vz) +
                     norms[i].vx * (pnt1->vx - pos->vx)) /
                        norms[i].vy +
                    pnt1->vy;

                if (polyY > highest) {
                    highest = polyY;
                }
            }
        }
    }
    return highest;
}

/* 0xB0A10, 627 bytes, local, 13 named locals
 * raycastpoly
 * PDB type: int (FVECTOR*, int, FVECTOR*, FV...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
static int raycastpoly(
    FVECTOR *points,
    int sides,
    FVECTOR *normal,
    FVECTOR *start,
    FVECTOR *direction,
    float length)
{
    float angleToPlane =
        (start->vy - points[0].vy) * normal->vy;
    float moveToPlaneDot;
    float distance;
    FVECTOR hitpoint;
    int side;

    angleToPlane +=
        (start->vx - points[0].vx) * normal->vx;
    angleToPlane +=
        (start->vz - points[0].vz) * normal->vz;
    moveToPlaneDot = direction->vx * normal->vx;
    moveToPlaneDot += direction->vy * normal->vy;
    moveToPlaneDot += direction->vz * normal->vz;

    if (!(angleToPlane >= -1.0f) ||
        !(length > angleToPlane) ||
        !(moveToPlaneDot < 0.0f)) {
        return INT_MAX;
    }
    moveToPlaneDot = -moveToPlaneDot;
    if (!(length >= moveToPlaneDot * length)) {
        return INT_MAX;
    }

    distance = angleToPlane / moveToPlaneDot;
    hitpoint.vx = start->vx + direction->vx * distance;
    hitpoint.vy = start->vy + direction->vy * distance;
    hitpoint.vz = start->vz + direction->vz * distance;
    for (side = 0; side < sides; ++side) {
        FVECTOR *current = &points[side];
        FVECTOR *next = &points[(side + 1) % sides];
        float edge_x = next->vx - current->vx;
        float edge_y = next->vy - current->vy;
        float edge_z = next->vz - current->vz;
        float inside =
            (edge_z * normal->vx - edge_x * normal->vz) *
            (hitpoint.vy - current->vy);

        inside +=
            (edge_y * normal->vz - edge_z * normal->vy) *
            (hitpoint.vx - current->vx);
        inside +=
            (edge_x * normal->vy - edge_y * normal->vx) *
            (hitpoint.vz - current->vz);
        if (inside < -0.125f) {
            return INT_MAX;
        }
    }
    if (distance < 0.0f) {
        distance = 0.0f;
    }
    return intersec_trunc_float_to_i32(distance);
}

/* 0xB0C90, 653 bytes, local, 16 named locals
 * raycheckgeneral
 * PDB type: int (_svector*, _svector*, short...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
static int raycheckgeneral(
    _svector *verts,
    _svector *normals,
    int16_t *index,
    int npolys,
    FVECTOR *start,
    FVECTOR *direction,
    float length,
    int *polyindex)
{
    int bestsofar = INT_MAX;
    int thispoly = 0;
    int i;

    for (i = 0; i < npolys; ++i) {
        FVECTOR realpoints[4];
        FVECTOR localnormal;
        int npoints;
        int hitlen;
        int v0 = index[i * 4 + 0];
        int v1 = index[i * 4 + 1];
        int v2 = index[i * 4 + 2];
        int v3 = index[i * 4 + 3];

        realpoints[0].vx = (float)verts[v0].vx;
        realpoints[0].vy = (float)verts[v0].vy;
        realpoints[0].vz = (float)verts[v0].vz;
        realpoints[1].vx = (float)verts[v1].vx;
        realpoints[1].vy = (float)verts[v1].vy;
        realpoints[1].vz = (float)verts[v1].vz;
        realpoints[2].vx = (float)verts[v2].vx;
        realpoints[2].vy = (float)verts[v2].vy;
        realpoints[2].vz = (float)verts[v2].vz;
        if (v3 == 0x7fff) {
            npoints = 3;
        } else {
            realpoints[3].vx = (float)verts[v3].vx;
            realpoints[3].vy = (float)verts[v3].vy;
            realpoints[3].vz = (float)verts[v3].vz;
            npoints = 4;
        }
        localnormal.vx =
            (float)normals[i].vx * (1.0f / 4096.0f);
        localnormal.vy =
            (float)normals[i].vy * (1.0f / 4096.0f);
        localnormal.vz =
            (float)normals[i].vz * (1.0f / 4096.0f);
        hitlen = raycastpoly(
            realpoints,
            npoints,
            &localnormal,
            start,
            direction,
            length);
        if (hitlen < bestsofar) {
            bestsofar = hitlen;
            thispoly = i;
        }
    }
    if (polyindex != NULL) {
        *polyindex = thispoly;
    }
    return bestsofar;
}

/* 0xB0F20, 389 bytes, global, 10 named locals
 * zapcheck
 * PDB type: int (playerObject*, _svector*, _...
 * Source: W:\SWJediPowerBattles\Work\intersec.c
 */
int zapcheck(
    playerObject *player,
    _svector *start,
    _svector *end,
    int damage,
    playerObject *hitter,
    int extraradius)
{
    sceneObject *scene;
    physicsObject *p;
    _svector pos;
    int dist;
    int motion;

    if (player->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(
            &player->playerRoot, 0, UINT32_C(0x20)) != 0 ||
        (player->pFlags & UINT32_C(0x00040200)) != 0) {
        return 0;
    }

    scene = (sceneObject *)player->playerRoot.pParent;
    p = (physicsObject *)scene->pPhysics;
    pos.vx = (int16_t)(int)p->pos.vx;
    pos.vy = (int16_t)(int)p->pos.vy;
    pos.vz = (int16_t)(int)p->pos.vz;
    pos.pad = 0;
    end->pad = 0;
    dist = vecpointlinesquared(
        start, end, &pos, &pos);

    if (dist < 0 ||
        dist >=
            (int)p->radius *
                (int)p->radius *
                extraradius ||
        player->currentMotion == 44 ||
        player->currentMotion == 39) {
        return 0;
    }

    (void)game_gModEnergy(player->playernum, damage);
    if ((p->flags & UINT32_C(1)) == 0) {
        motion = 39;
    } else {
        p->constmov.vx = -p->constmov.vx;
        p->constmov.vz = -p->constmov.vz;
        motion = 44;
    }
    (void)animctrl_MotionNoLock(
        &player->playerRoot,
        &player->paMotions[motion]);
    player->whohitme = hitter;
    player->hitNumber = 1;
    return 1;
}
