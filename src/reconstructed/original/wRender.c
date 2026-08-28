/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\wRender.c.
 *
 * Provenance:
 *   direct   - exact names, signatures, module-local matrix globals, and
 *              16-entry storage extent from the PDB.
 *   assembly - stack bounds, copy width, and underflow/overflow behavior
 *              checked at RVAs 0x12D500 and 0x12D550.
 *
 * PDB module: 0104
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\wRender.obj
 * Primary source: W:\SWJediPowerBattles\Work\wRender.c
 * Compiler language: c
 * Emitted procedures: 11
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/wrender.h"
#include "jpb/bmd.h"
#include "jpb/brainutl.h"
#include "jpb/globalarrays.h"
#include "jpb/jonny.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <string.h>

enum {
    WRENDER_SCRATCH_ORIGIN_X = 0,
    WRENDER_SCRATCH_ORIGIN_Y = 2,
    WRENDER_SCRATCH_ORIGIN_Z = 4,
    WRENDER_SCRATCH_SOURCE_X = 8,
    WRENDER_SCRATCH_SOURCE_Y = 12,
    WRENDER_SCRATCH_SOURCE_Z = 16,
    WRENDER_SCRATCH_CUBE_COLORS = 0x50,
    WRENDER_SCRATCH_COLOR_TABLE = 0x90
};

static const CVECTOR mMaterial[16] = {
    {0xc0, 0x00, 0x00, 0x00}, {0xc0, 0xc0, 0x00, 0x00},
    {0xc0, 0xc0, 0xc0, 0x00}, {0x00, 0xc0, 0x00, 0x00},
    {0x00, 0xc0, 0xc0, 0x00}, {0x00, 0x00, 0xc0, 0x00},
    {0xc0, 0xc0, 0x40, 0x00}, {0x40, 0x40, 0x40, 0x00},
    {0x60, 0x00, 0x00, 0x00}, {0x60, 0x60, 0x00, 0x00},
    {0x60, 0x60, 0x60, 0x00}, {0x00, 0x60, 0x00, 0x00},
    {0x00, 0x60, 0x60, 0x00}, {0x00, 0x20, 0x80, 0x00},
    {0x60, 0x60, 0x40, 0x00}, {0x40, 0x40, 0x40, 0x00}
};

static int16_t wrender_read_i16(size_t offset)
{
    int16_t value;

    memcpy(&value, &gaScratch[offset], sizeof(value));
    return value;
}

static void wrender_write_i16(size_t offset, int16_t value)
{
    memcpy(&gaScratch[offset], &value, sizeof(value));
}

static uint8_t *wrender_read_pointer(size_t offset)
{
    uintptr_t value;

    memcpy(&value, &gaScratch[offset], sizeof(value));
    return (uint8_t *)value;
}

static int16_t wrender_add_i16(int16_t left, int16_t right)
{
    return (int16_t)((uint16_t)left + (uint16_t)right);
}

static int16_t wrender_sub_i16(int16_t left, int16_t right)
{
    return (int16_t)((uint16_t)left - (uint16_t)right);
}

static uint32_t wrender_color(const uint8_t *color)
{
    return UINT32_C(0xff000000) |
           ((uint32_t)color[0] << 16) |
           ((uint32_t)color[1] << 8) |
           (uint32_t)color[2];
}

static void wrender_copy_scratch_origin(void)
{
    wrender_write_i16(
        WRENDER_SCRATCH_ORIGIN_X,
        wrender_read_i16(WRENDER_SCRATCH_SOURCE_X));
    wrender_write_i16(
        WRENDER_SCRATCH_ORIGIN_Y,
        wrender_read_i16(WRENDER_SCRATCH_SOURCE_Y));
    wrender_write_i16(
        WRENDER_SCRATCH_ORIGIN_Z,
        wrender_read_i16(WRENDER_SCRATCH_SOURCE_Z));
}

static int wrender_transform_project(
    MATRIX *matrix, VECTOR *source, fPoint4 *destination)
{
    VECTOR transformed;
    double projection;

    fApplyMatrixLV(matrix, source, &transformed);
    transformed.vx += matrix->t[0];
    transformed.vy += matrix->t[1];
    transformed.vz += matrix->t[2];
    destination->x = (float)transformed.vx;
    destination->y = (float)transformed.vy;
    destination->z = (float)transformed.vz;
    if (destination->z < 768.0f || destination->z > 9472.0f) {
        return 0;
    }
    projection = 768.0 / (double)transformed.vz;
    destination->x =
        (float)((double)transformed.vx * projection + 320.0);
    destination->y =
        (float)((double)transformed.vy * projection + 240.0);
    destination->z = (float)((double)transformed.vz / 10240.0);
    return 1;
}

static VECTOR wrender_unpack_map_vertex(uint16_t packed)
{
    VECTOR vertex;

    vertex.vx = (int)wrender_read_i16(WRENDER_SCRATCH_ORIGIN_X) -
                (int)(packed & 0x1fU) * 16;
    vertex.vy = (int)wrender_read_i16(WRENDER_SCRATCH_ORIGIN_Y) +
                (int)((packed >> 5) & 0x3fU) * 8;
    vertex.vz = (int)wrender_read_i16(WRENDER_SCRATCH_ORIGIN_Z) +
                (int)(packed >> 11) * 16;
    vertex.pad = 0;
    return vertex;
}

static int32_t wrender_unpack_signed_10(uint32_t value)
{
    value &= UINT32_C(0x3ff);
    if ((value & UINT32_C(0x200)) != 0) {
        value |= UINT32_C(0xfffffc00);
    }
    return (int32_t)value;
}

/* Exact PDB-named wRender.c module locals at RVAs 0x92DAE0..0x92DE18. */
static MATRIX gte_matrix_stack[16];
static int matrix_stack_level;
static MATRIX gte_matrix;

MATRIX *jpb_WRenderCurrentMatrix(void)
{
    return &gte_matrix;
}

int jpb_WRenderMatrixStackLevel(void)
{
    return matrix_stack_level;
}

int32_t jpb_WRenderMatrixStackBaseLow32(void)
{
    return (int32_t)(uint32_t)(uintptr_t)gte_matrix_stack;
}

/* 0x12D500, 70 bytes, global, 0 named locals
 * PopMatrix
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
void PopMatrix(void)
{
    if (matrix_stack_level != 0) {
        --matrix_stack_level;
        gte_matrix = gte_matrix_stack[matrix_stack_level];
    }
}

/* 0x12D550, 70 bytes, global, 0 named locals
 * PushMatrix
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
void PushMatrix(void)
{
    if (matrix_stack_level < 15) {
        gte_matrix_stack[matrix_stack_level] = gte_matrix;
        ++matrix_stack_level;
    }
}

/* 0x12D5A0, 916 bytes, global, 17 named locals
 * _CubeRender
 * PDB type: int (wsl_mapEntry*, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
int _CubeRender(wsl_mapEntry *entry, MATRIX *matrix)
{
    fPoint4 point_cache[256];
    wsl_libPart *part = gpWorld->pLib[entry->libPart];
    int vertex_count;
    int face;
    int color_offset;
    uint8_t *face_colors;

    wrender_copy_scratch_origin();
    vertex_count = (part->numverts & 0x1f) * 3 + (part->numverts >> 5);
    for (face = 0; face < vertex_count; ++face) {
        VECTOR source = wrender_unpack_map_vertex((uint16_t)part->shared[face]);

        if (!wrender_transform_project(matrix, &source, &point_cache[face])) {
            return 0;
        }
    }

    memcpy(&color_offset, entry->color.b, sizeof(color_offset));
    face_colors =
        wrender_read_pointer(WRENDER_SCRATCH_CUBE_COLORS) + color_offset;
    for (face = 0; face < part->numpolys; ++face) {
        const wsl_libPoly *poly = &part->polys[face];
        const TVECTOR *uv = gpWorld->pTexture[poly->textureID].c;
        uint32_t indices = (uint32_t)part->index[face];
        int corners = 3 + (int)((uint32_t)poly->n >> 31);
        int corner;

        _StartPoly(corners, NULL);
        for (corner = 0; corner < corners; ++corner) {
            unsigned vertex = indices & 0xffU;
            unsigned color_index = face_colors[vertex + 1];
            const uint8_t *color =
                wrender_read_pointer(WRENDER_SCRATCH_COLOR_TABLE) +
                color_index * 4U;

            _SetVert(
                corner,
                point_cache[vertex].x,
                point_cache[vertex].y,
                point_cache[vertex].z,
                wrender_color(color),
                (float)uv[corner].u,
                (float)uv[corner].v);
            indices = (uint32_t)((int32_t)indices >> 8);
        }
        _EndPoly();
    }
    return 1;
}

/* 0x12D940, 284 bytes, global, 5 named locals
 * _Cull
 * PDB type: int (MATRIX*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
int _Cull(MATRIX *matrix, VECTOR *point)
{
    VECTOR source = *point;
    VECTOR transformed;
    float x;
    float y;
    float z;
    double projection;

    source.vz += 0x100;
    fApplyMatrixLV(matrix, &source, &transformed);
    transformed.vx += matrix->t[0];
    transformed.vy += matrix->t[1];
    transformed.vz += matrix->t[2];
    projection = 768.0 / (double)transformed.vz;
    x = (float)((double)transformed.vx * projection + 320.0);
    y = (float)((double)transformed.vy * projection + 240.0);
    z = (float)((double)transformed.vz / 10240.0);
    if (x < -64.0f || x > 640.0f ||
        y < 0.0f || y > 576.0f ||
        (double)z < 0.05 || (double)z > 0.9) {
        return 1;
    }
    return 0;
}

/* 0x12DA60, 688 bytes, global, 10 named locals
 * _FatRender
 * PDB type: int (wsl_fatPoly*, MATRIX*, char...
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
int _FatRender(wsl_fatPoly *entry, MATRIX *matrix, char *color)
{
    fPoint4 point_cache[4];
    int corners = 3 + (int)((uint32_t)entry->n >> 31);
    int corner;

    (void)color;
    wrender_copy_scratch_origin();
    for (corner = 0; corner < corners; ++corner) {
        VECTOR source;

        source.vx = (int)wrender_read_i16(WRENDER_SCRATCH_ORIGIN_X) -
                    (int)entry->verts[corner].vx;
        source.vy = (int)wrender_read_i16(WRENDER_SCRATCH_ORIGIN_Y) +
                    (int)entry->verts[corner].vy;
        source.vz = (int)wrender_read_i16(WRENDER_SCRATCH_ORIGIN_Z) +
                    (int)entry->verts[corner].vz;
        source.pad = 0;
        if (!wrender_transform_project(matrix, &source, &point_cache[corner])) {
            return 0;
        }
    }

    _StartPoly(corners, NULL);
    for (corner = 0; corner < corners; ++corner) {
        const CVECTOR *material =
            &mMaterial[(uint8_t)entry->textureID & 0x0fU];

        _SetVert(
            corner,
            point_cache[corner].x,
            point_cache[corner].y,
            point_cache[corner].z,
            wrender_color((const uint8_t *)material),
            0.0f,
            0.0f);
    }
    _EndPoly();
    return 1;
}

/* 0x12DD10, 199 bytes, global, 4 named locals
 * _PerspectiveTransform
 * PDB type: void (MATRIX*, _svector*, fPoint...
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
void _PerspectiveTransform(
    MATRIX *matrix, _svector *source, fPoint4 *destination)
{
    VECTOR transformed;
    double depth;

    fApplyMatrix(matrix, source, &transformed);
    transformed.vx += matrix->t[0];
    transformed.vy += matrix->t[1];
    transformed.vz += matrix->t[2];
    depth = (double)transformed.vz;
    destination->x =
        (float)((double)transformed.vx * 768.0 / depth + 320.0);
    destination->y =
        (float)((double)transformed.vy * 768.0 / depth + 240.0);
    destination->z = (float)(depth / 10240.0);
}

/* 0x12DDE0, 1397 bytes, global, 10 named locals
 * _RenderParticle
 * PDB type: void (MATRIX*, PCB*)
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
void _RenderParticle(MATRIX *matrix, PCB *pcb)
{
    MATRIX rotation = {{{1.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f},
                        {0.0f, 0.0f, 1.0f}}, {0, 0, 0}};
    MATRIX combined;
    _svector origin;
    VECTOR camera_origin;
    int particle_index;

    fRotMatrixY(pcb->pcb_Interp, &rotation);
    fMulMatrix0(matrix, &rotation, &combined);
    origin.vx = wrender_add_i16(
        (int16_t)pcb->pcb_Pos->vx, gSceneGeometryEnv.posDest.vx);
    origin.vy = wrender_add_i16(
        (int16_t)pcb->pcb_Pos->vy, gSceneGeometryEnv.posDest.vy);
    origin.vz = wrender_add_i16(
        (int16_t)pcb->pcb_Pos->vz, gSceneGeometryEnv.posDest.vz);
    origin.pad = 0;
    fApplyMatrix(matrix, &origin, &camera_origin);
    pcb->pcb_Interp += 0x20;

    for (particle_index = 0; particle_index < 8; ++particle_index) {
        Particle *particle = &pcb->pcb_Particle[particle_index];
        uint32_t active_bit = UINT32_C(1) << (particle_index + 8);

        if ((pcb->pcb_Bits & active_bit) != 0) {
            fPoint4 point;
            VECTOR transformed;
            _svector previous;
            uint32_t color = wrender_color((const uint8_t *)&particle->color);

            _StartPoly(4, NULL);
            previous.vx = wrender_sub_i16(particle->pos.vx, particle->vx);
            previous.vy = wrender_sub_i16(particle->pos.vy, particle->vy);
            previous.vz = wrender_sub_i16(particle->pos.vz, particle->vz);
            previous.pad = 0;
            fApplyMatrix(&combined, &previous, &transformed);
            transformed.vx += camera_origin.vx;
            transformed.vy += camera_origin.vy;
            transformed.vz += camera_origin.vz;
            point.x = (float)((double)transformed.vx * 768.0 /
                              (double)transformed.vz + 320.0);
            point.y = (float)((double)transformed.vy * 768.0 /
                              (double)transformed.vz + 240.0);
            point.z = (float)((double)transformed.vz / 10240.0);
            _SetVert(0, point.x, point.y, point.z, 0, 0.0f, 0.0f);
            _SetVert(1, point.x + 1.0f, point.y, point.z, 0, 0.0f, 0.0f);

            particle->pos.vx = wrender_add_i16(particle->pos.vx, particle->vx);
            particle->pos.vy = wrender_add_i16(particle->pos.vy, particle->vy);
            particle->pos.vz = wrender_add_i16(particle->pos.vz, particle->vz);
            _PerspectiveTransform(&combined, &particle->pos, &point);
            _SetVert(2, point.x, point.y + 1.0f, point.z, color, 0.0f, 0.0f);
            _SetVert(3, point.x + 1.0f, point.y + 1.0f, point.z,
                     color, 0.0f, 0.0f);
            _EndPoly();

            _StartPoly(4, NULL);
            _PerspectiveTransform(&combined, &particle->pos, &point);
            _SetVert(0, point.x, point.y, point.z, color, 0.0f, 0.0f);
            _SetVert(1, point.x + 1.0f, point.y, point.z,
                     color, 0.0f, 0.0f);
            _SetVert(3, point.x, point.y + 1.0f, point.z,
                     color, 0.0f, 0.0f);
            _SetVert(2, point.x + 1.0f, point.y + 1.0f, point.z,
                     color, 0.0f, 0.0f);
            _EndPoly();

            if (pcb->pcb_Interp != 0) {
                particle->vx = 0;
                particle->vy = 0;
                particle->vz = 0;
            }
            if (particle->vel.pad-- <= 0) {
                pcb->pcb_Bits &= ~active_bit;
            }
        }
    }

    if ((pcb->pcb_Bits & 0xff) != 0) {
        if (pcb->pcb_fLaunch < 0) {
            ++pcb->pcb_fLaunch;
        } else {
            pcb->pcb_fLaunch += pcb->pcb_fRate;
            while ((pcb->pcb_fLaunch & (int32_t)UINT32_C(0xfffff000)) > 0) {
                int slot;
                Particle *particle;

                pcb->pcb_fLaunch -= 0x1000;
                slot = brainutl_FindLSB((uint32_t)pcb->pcb_Bits);
                if (slot == 0) {
                    break;
                }
                pcb->pcb_Bits |= UINT32_C(1) << (slot + 7);
                pcb->pcb_Bits &= ~(UINT32_C(1) << (slot - 1));
                particle = &pcb->pcb_Particle[slot - 1];
                particle->pos.vx =
                    wrender_add_i16(particle->pos.vx, particle->vx);
                particle->pos.vy =
                    wrender_add_i16(particle->pos.vy, particle->vy);
                particle->pos.vz =
                    wrender_add_i16(particle->pos.vz, particle->vz);
            }
        }
    }
}

/* 0x12E360, 869 bytes, global, 12 named locals
 * _ThinRender
 * PDB type: int (wsl_thinPoly*, MATRIX*, cha...
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
int _ThinRender(wsl_thinPoly *entry, MATRIX *matrix, char *color)
{
    fPoint4 point_cache[4];
    int corners = 3 + (int)((uint32_t)entry->n >> 31);
    float area;
    int corner;

    wrender_copy_scratch_origin();
    for (corner = 0; corner < corners; ++corner) {
        VECTOR source = wrender_unpack_map_vertex(entry->verts[corner]);

        if (!wrender_transform_project(matrix, &source, &point_cache[corner])) {
            return 0;
        }
    }
    area = point_cache[0].x * point_cache[1].y +
           point_cache[1].x * point_cache[2].y +
           point_cache[2].x * point_cache[0].y -
           point_cache[0].x * point_cache[2].y -
           point_cache[0].y * point_cache[1].x -
           point_cache[2].x * point_cache[1].y;
    if ((int)area < 0) {
        return 0;
    }

    _StartPoly(corners, NULL);
    for (corner = 0; corner < corners; ++corner) {
        unsigned color_index = (uint8_t)color[corner];
        const uint8_t *vertex_color =
            wrender_read_pointer(WRENDER_SCRATCH_COLOR_TABLE) +
            color_index * 4U;
        const TVECTOR *uv = gpWorld->pTexture[entry->textureID].c;

        _SetVert(
            corner,
            point_cache[corner].x,
            point_cache[corner].y,
            point_cache[corner].z,
            wrender_color(vertex_color),
            (float)uv[corner].u,
            (float)uv[corner].v);
    }
    _EndPoly();
    return 1;
}

/* 0x12E6D0, 3 bytes, global, 3 named locals
 * __InitDisplay
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
void __InitDisplay(int xres, int yres, int depth)
{
    (void)xres;
    (void)yres;
    (void)depth;
}

/* 0x12E6E0, 1305 bytes, global, 22 named locals
 * gl_RenderNode
 * PDB type: int (geomData*, MATRIX*, MATRIX*...
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
int gl_RenderNode(
    geomData *geometry,
    MATRIX *matrix,
    MATRIX *light,
    fPoint4 *point_cache)
{
    uint32_t *indices =
        (uint32_t *)getPtr(geometry->pIndex, JPB_POINTER_ARRAY_INDEX);
    uint32_t *vertices =
        (uint32_t *)getPtr(geometry->pVertex, JPB_POINTER_ARRAY_VERTEX);
    faceUV *uv = (faceUV *)getPtr(geometry->pUV, JPB_POINTER_ARRAY_UV);
    CVECTOR *colors =
        (CVECTOR *)getPtr(geometry->pColor, JPB_POINTER_ARRAY_COLOR);
    int vertex;
    int face;

    (void)light;
    (void)getPtr(geometry->pNormal, JPB_POINTER_ARRAY_NORMAL);
    for (vertex = 0; vertex < geometry->numVerts * 3; ++vertex) {
        uint32_t packed = vertices[vertex];
        VECTOR source;
        VECTOR transformed;
        fPoint4 *destination =
            &point_cache[geometry->numShareVerts + vertex];
        double depth;

        source.vx = wrender_unpack_signed_10(packed);
        source.vy = wrender_unpack_signed_10(packed >> 10);
        source.vz = wrender_unpack_signed_10(packed >> 20);
        source.pad = 0;
        fApplyMatrixLV(matrix, &source, &transformed);
        transformed.vx += matrix->t[0];
        transformed.vy += matrix->t[1];
        transformed.vz += matrix->t[2];
        depth = (double)transformed.vz;
        destination->x =
            (float)((double)transformed.vx * 768.0 / depth + 320.0);
        destination->y =
            (float)((double)transformed.vy * 768.0 / depth + 240.0);
        destination->z = (float)(depth / 10240.0);
    }

    for (face = 0; face < geometry->numFaces; ++face) {
        uint32_t packed = indices[face];
        int corners = ((packed & UINT32_C(0xff000000)) !=
                       UINT32_C(0xff000000)) + 3;
        fPoint4 points[4];
        float area;
        int corner;

        for (corner = 0; corner < corners; ++corner) {
            points[corner] = point_cache[packed & 0xffU];
            packed = (uint32_t)((int32_t)packed >> 8);
        }
        area = points[0].x * points[1].y +
               points[1].x * points[2].y +
               points[2].x * points[0].y -
               points[0].x * points[2].y -
               points[0].y * points[1].x -
               points[2].x * points[1].y;
        if ((int)area >= 0 &&
            (double)points[0].z >= 0.05 &&
            (double)points[1].z >= 0.05 &&
            (double)points[2].z >= 0.05 &&
            (corners == 3 || (double)points[3].z >= 0.05)) {
            _StartPoly(corners, NULL);
            for (corner = 0; corner < corners; ++corner) {
                _SetVert(
                    corner,
                    points[corner].x,
                    points[corner].y,
                    points[corner].z,
                    wrender_color((const uint8_t *)&colors[corner]),
                    uv->uv[corner].u,
                    uv->uv[corner].v);
            }
            _EndPoly();
        }
        colors += corners;
        ++uv;
    }
    return 1;
}

/* 0x12EC00, 3 bytes, global, 0 named locals
 * psx_LoadBar
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
void psx_LoadBar(void)
{
}
