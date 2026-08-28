/*
 * COMPLETE REVIEWED RECONSTRUCTION.
 *
 * All 84 emitted procedures were checked against matched PDB symbols and
 * types plus direct disassembly/decompilation of the shipped executable at
 * RVAs 0xF7E60..0xFDB49. Control flow, constants, field offsets, list order,
 * matrix-stack boundaries, draw arguments, callback timing, allocation and
 * delayed-free behavior are restored. Rendering submission crosses the
 * established portable wHook boundary without changing Sprite-owned state.
 * PDB module: 0081
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\sprite.obj
 * Primary source: W:\SWJediPowerBattles\Work\sprite.c
 * Compiler language: c
 * Emitted procedures: 84
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/sprite.h"

#include "jpb/alloc.h"
#include "jpb/bullet.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/debugtext.h"
#include "jpb/flex.h"
#include "jpb/game.h"
#include "jpb/intersec.h"
#include "jpb/linkstubs.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/text.h"
#include "jpb/whook.h"
#include "jpb/wrender.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

List mSCBDraw[2];
int mCurSCBList;
/* Exact PDB globals at RVAs 0x4CC870, 0x945D10, and 0x945D14. */
uint32_t cluts[32] = {
    UINT32_C(0x0030f838), UINT32_C(0x00f82030),
    UINT32_C(0x00e81890), UINT32_C(0x00f008e8),
    UINT32_C(0x0010f810), UINT32_C(0x0088f810),
    UINT32_C(0x00b8f800), UINT32_C(0x0010f890),
    UINT32_C(0x00ffffff), UINT32_C(0x00e838a8),
    UINT32_C(0x00b02020), UINT32_C(0x00180050),
    UINT32_C(0x0058b850), UINT32_C(0x0068f8a0),
    UINT32_C(0x00ffffff), UINT32_C(0x00ffffff),
    UINT32_C(0x00ffffff), UINT32_C(0x00ffffff),
    UINT32_C(0x00ffffff), UINT32_C(0x00f00078),
    UINT32_C(0x00e80000), UINT32_C(0x0068e038),
    UINT32_C(0x00f83870), UINT32_C(0x00f89038),
    UINT32_C(0x00e0f838), UINT32_C(0x00e00008),
    UINT32_C(0x00f018f8), UINT32_C(0x0030f8f8),
    UINT32_C(0x001000f8), UINT32_C(0x00f838d0),
    UINT32_C(0x0090f800), UINT32_C(0x00e00080)
};
int numSprite;
int numSCB;
int16_t aCircle[17] = {
    0x100, 0xff, 0xfd, 0xfb, 0xf7, 0xf3, 0xed, 0xe6, 0xdd,
    0xd3, 0xc7, 0xb9, 0xa9, 0x95, 0x7b, 0x59, 0
};

static List mSpriteWork[2];
static int mCurSpriteList;
static int mEffectRot;
static _svector effectRotation;
static Sprite *shot[16];
static _svector rot = {0, 0x100, 0, 0};
static JPBSpriteCylinderHook jpb_sprite_cylinder_hook;
static void *jpb_sprite_cylinder_user_data;
static JPBSpriteDisplayHook jpb_sprite_display_hook;
static void *jpb_sprite_display_user_data;

static void jpb_sprite_mark_free(Sprite *sptr)
{
    SCB *scb;

    if ((sptr->sp_Flags & 1) == 0) {
        sptr->sp_Flags |= 1;
    }
    scb = sptr->sp_SCB;
    if (scb != NULL &&
        scb->scb_flags != 0 &&
        (scb->scb_flags & 1) == 0) {
        scb->scb_flags |= 1;
    }
}

/* 0xF7E60, 1608 bytes, global, 29 named locals
 * _RenderSprite
 * PDB type: void (MATRIX*, SCB*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

static void sprite_find_render_sin_cos(
    int angle, float *sine, float *cosine)
{
    float radians = (float)(angle << 4);

    radians *= 3.1415927f;
    radians *= 0.000030517578125f;
    *sine = (float)sin((double)radians);
    *cosine = (float)cos((double)radians);
}

static void sprite_submit_projected_quad(
    _Material *texture, const FVECTOR *vertices, uint32_t color)
{
    static const float u[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    static const float v[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    int index;

    _StartPoly(4, texture);
    for (index = 0; index < 4; ++index) {
        _SetVert(
            index,
            vertices[index].vx,
            vertices[index].vy,
            vertices[index].vz - 55.0f,
            color,
            u[index],
            v[index]);
    }
    _NoScaleEndPoly();
}

void _RenderSprite(MATRIX *matrix, SCB *scb)
{
    _Material *texture = scb->scb_Texture;
    uint32_t color;
    uint32_t flags;
    int color_index;

    if (texture == NULL) {
        return;
    }

    color_index =
        (uint16_t)scb->scb_cvertex.pad < 32
            ? (int)scb->scb_cvertex.pad
            : 0;
    color = cluts[color_index] |
        ((uint32_t)(uint16_t)scb->scb_vertex0.pad << 24);
    flags = (uint32_t)scb->scb_flags;

    if ((flags & UINT32_C(2)) != 0) {
        SCREENRECT destination;
        SCREENRECT source;
        CVECTOR vertex_color;
        float x0 = scb->scb_vertex0.vx;
        float y0 = scb->scb_vertex0.vy;
        float x1 = scb->scb_vertex1.vx;
        float y1 = scb->scb_vertex2.vy;
        int flip_x = x0 > x1;
        int flip_y = y0 > y1;

        destination.left = (int32_t)(flip_x ? x1 : x0);
        destination.top = (int32_t)(flip_y ? y1 : y0);
        destination.right = (int32_t)(flip_x ? x0 : x1);
        destination.bottom = (int32_t)(flip_y ? y0 : y1);
        source.left = flip_x ? texture->iw : 0;
        source.top = flip_y ? texture->ih : 0;
        source.right = flip_x ? 0 : texture->iw;
        source.bottom = flip_y ? 0 : texture->ih;
        vertex_color.r = (uint8_t)(color >> 16);
        vertex_color.g = (uint8_t)(color >> 8);
        vertex_color.b = (uint8_t)color;
        vertex_color.cd = (uint8_t)(color >> 24);
        _DrawTexture(
            texture,
            destination,
            &source,
            vertex_color,
            0.001f);
        return;
    }

    if ((flags & UINT32_C(4)) != 0) {
        FVECTOR projected[4];

        if (RotTransPersSFV(
                matrix, &scb->scb_vertex0, projected, 4) == 0) {
            sprite_submit_projected_quad(texture, projected, color);
        }
        return;
    }

    if ((flags & UINT32_C(8)) != 0) {
        MATRIX sprite_matrix;
        FVECTOR projected[4];
        int x_angle =
            (flags & UINT32_C(0x100)) != 0 ? 0x500 : 0;
        int y_angle =
            (flags & UINT32_C(0x200)) != 0
                ? 0x400 - gSceneGeometryEnv.angle.vx
                : 0;
        int z_angle = scb->scb_vertex3.pad != 0
            ? scb->scb_vertex3.pad
            : 0;
        float sx;
        float cx;
        float sy;
        float cy;
        float sz;
        float cz;
        float center_x;
        float center_y;
        float center_z;

        sprite_find_render_sin_cos(x_angle, &sx, &cx);
        sprite_find_render_sin_cos(y_angle, &sy, &cy);
        sprite_find_render_sin_cos(z_angle, &sz, &cz);
        sprite_matrix.m[0][0] = cz * cy;
        sprite_matrix.m[0][1] = sy * sx * cz - sz * cx;
        sprite_matrix.m[0][2] = sy * cx * cz + sx * sz;
        sprite_matrix.m[1][0] = sz * cy;
        sprite_matrix.m[1][1] = sy * sx * sz + cz * cx;
        sprite_matrix.m[1][2] = sy * cx * sz - cz * sx;
        sprite_matrix.m[2][0] = -sy;
        sprite_matrix.m[2][1] = cy * sx;
        sprite_matrix.m[2][2] = cy * cx;

        center_x =
            scb->scb_cvertex.vy * matrix->m[0][1] +
            scb->scb_cvertex.vx * matrix->m[0][0] +
            scb->scb_cvertex.vz * matrix->m[0][2];
        center_y =
            scb->scb_cvertex.vx * matrix->m[1][0] +
            scb->scb_cvertex.vy * matrix->m[1][1] +
            scb->scb_cvertex.vz * matrix->m[1][2];
        center_z =
            scb->scb_cvertex.vx * matrix->m[2][0] +
            scb->scb_cvertex.vy * matrix->m[2][1] +
            scb->scb_cvertex.vz * matrix->m[2][2];
        sprite_matrix.t[0] = (int32_t)center_x + matrix->t[0];
        sprite_matrix.t[1] = (int32_t)center_y + matrix->t[1];
        sprite_matrix.t[2] = (int32_t)center_z + matrix->t[2];

        if (RotTransPersSFV(
                &sprite_matrix, &scb->scb_vertex0, projected, 4) == 0) {
            sprite_submit_projected_quad(texture, projected, color);
        }
    }
}

/* 0xF84B0, 801 bytes, global, 19 named locals
 * drawCylinder
 * PDB type: void (VECTOR*, _svector*, float,...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void drawCylinder(
    VECTOR *loc,
    _svector *rotation,
    float radius1,
    float radius2,
    float height1,
    float height2,
    uint32_t color1,
    uint32_t ratio,
    int id,
    int clut,
    int tmode)
{
    FVECTOR local_vertices[34];
    FVECTOR transformed[34];
    MATRIX local_rotation;
    MATRIX combined;
    _svector translated_location;
    VECTOR camera_location;
    int segment;

    (void)ratio;
    (void)clut;
    (void)tmode;
    if (jpb_sprite_cylinder_hook != NULL) {
        jpb_sprite_cylinder_hook(
            jpb_sprite_cylinder_user_data,
            loc,
            rotation,
            radius1,
            radius2,
            height1,
            height2,
            color1,
            color1);
    }

    for (segment = 0; segment < 17; ++segment) {
        float angle =
            (float)(((double)segment * 22.5) / 57.2957);
        float cosine = (float)cos((double)angle);
        float sine = (float)sin((double)angle);

        local_vertices[segment].vx = cosine * radius1;
        local_vertices[segment].vy = height1;
        local_vertices[segment].vz = sine * radius1;
        local_vertices[17 + segment].vx = cosine * radius2;
        local_vertices[17 + segment].vy = height2;
        local_vertices[17 + segment].vz = sine * radius2;
    }
    (void)fRotMatrixZYX(rotation, &local_rotation);
    (void)fMulMatrix0(&CameraMatrix, &local_rotation, &combined);
    translated_location.vx = (int16_t)(
        (int16_t)loc->vx + gSceneGeometryEnv.pos.vx);
    translated_location.vy = (int16_t)(
        (int16_t)loc->vy + gSceneGeometryEnv.pos.vy);
    translated_location.vz = (int16_t)(
        (int16_t)loc->vz + gSceneGeometryEnv.pos.vz);
    translated_location.pad = 0;
    (void)fApplyMatrix(
        &CameraMatrix, &translated_location, &camera_location);
    combined.t[0] = camera_location.vx;
    combined.t[1] = camera_location.vy;
    combined.t[2] = camera_location.vz;
    (void)RotTransPersFloat(
        &combined, local_vertices, transformed, 34);

    for (segment = 0; segment < 16; ++segment) {
        _StartPoly(4, effects1Handle[id]);
        _SetVert(0, transformed[segment].vx,
                 transformed[segment].vy, transformed[segment].vz,
                 color1, 0.0f, 0.0f);
        _SetVert(1, transformed[segment + 1].vx,
                 transformed[segment + 1].vy,
                 transformed[segment + 1].vz,
                 color1, 1.0f, 0.0f);
        _SetVert(2, transformed[17 + segment].vx,
                 transformed[17 + segment].vy,
                 transformed[17 + segment].vz,
                 color1, 0.0f, 1.0f);
        _SetVert(3, transformed[18 + segment].vx,
                 transformed[18 + segment].vy,
                 transformed[18 + segment].vz,
                 color1, 1.0f, 1.0f);
        _NoScaleEndPoly();
    }
}

/* 0xF87E0, 777 bytes, global, 15 named locals
 * drawCylinderG
 * PDB type: void (VECTOR*, _svector*, float,...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void jpb_SpriteSetCylinderHook(
    JPBSpriteCylinderHook hook, void *user_data)
{
    jpb_sprite_cylinder_hook = hook;
    jpb_sprite_cylinder_user_data = user_data;
}

void jpb_SpriteSetDisplayHook(
    JPBSpriteDisplayHook hook, void *user_data)
{
    jpb_sprite_display_hook = hook;
    jpb_sprite_display_user_data = user_data;
}

void drawCylinderG(
    VECTOR *loc,
    _svector *rotation,
    float radius1,
    float radius2,
    float h1,
    float h2,
    uint32_t color1,
    uint32_t color2)
{
    FVECTOR local_vertices[34];
    FVECTOR transformed[34];
    MATRIX local_rotation;
    MATRIX combined;
    _svector translated_location;
    VECTOR camera_location;
    int segment;

    if (jpb_sprite_cylinder_hook != NULL) {
        jpb_sprite_cylinder_hook(
            jpb_sprite_cylinder_user_data,
            loc,
            rotation,
            radius1,
            radius2,
            h1,
            h2,
            color1,
            color2);
    }

    for (segment = 0; segment < 17; ++segment) {
        float angle =
            (float)(((double)segment * 22.5) / 57.2957);
        float cosine = (float)cos((double)angle);
        float sine = (float)sin((double)angle);

        local_vertices[segment].vx = cosine * radius1;
        local_vertices[segment].vy = h1;
        local_vertices[segment].vz = sine * radius1;
        local_vertices[17 + segment].vx = cosine * radius2;
        local_vertices[17 + segment].vy = h2;
        local_vertices[17 + segment].vz = sine * radius2;
    }
    (void)fRotMatrixZYX(rotation, &local_rotation);
    (void)fMulMatrix0(&CameraMatrix, &local_rotation, &combined);
    translated_location.vx = (int16_t)(
        (int16_t)loc->vx + gSceneGeometryEnv.pos.vx);
    translated_location.vy = (int16_t)(
        (int16_t)loc->vy + gSceneGeometryEnv.pos.vy);
    translated_location.vz = (int16_t)(
        (int16_t)loc->vz + gSceneGeometryEnv.pos.vz);
    translated_location.pad = 0;
    (void)fApplyMatrix(
        &CameraMatrix, &translated_location, &camera_location);
    combined.t[0] = camera_location.vx;
    combined.t[1] = camera_location.vy;
    combined.t[2] = camera_location.vz;
    (void)RotTransPersFloat(
        &combined, local_vertices, transformed, 34);

    for (segment = 0; segment < 16; ++segment) {
        _StartPoly(4, whitemat);
        _SetVert(0, transformed[segment].vx,
                 transformed[segment].vy, transformed[segment].vz,
                 color1, 0.0f, 0.0f);
        _SetVert(1, transformed[segment + 1].vx,
                 transformed[segment + 1].vy,
                 transformed[segment + 1].vz,
                 color1, 0.0f, 0.0f);
        _SetVert(2, transformed[17 + segment].vx,
                 transformed[17 + segment].vy,
                 transformed[17 + segment].vz,
                 color2, 0.0f, 0.0f);
        _SetVert(3, transformed[18 + segment].vx,
                 transformed[18 + segment].vy,
                 transformed[18 + segment].vz,
                 color2, 0.0f, 0.0f);
        _NoScaleEndPoly();
    }
}

/* 0xF8AF0, 23 bytes, global, 0 named locals
 * getScaleAdjustment
 * PDB type: float (<no type>)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
float getScaleAdjustment(void)
{
    return (float)OptionStruct.ScreenHeight / 1080.0f;
}

/* 0xF8B10, 73 bytes, global, 1 named locals
 * getScaleAdjustmentMM
 * PDB type: float (<no type>)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
float getScaleAdjustmentMM(void)
{
    float width = (float)OptionStruct.ScreenWidth;
    float height = (float)OptionStruct.ScreenHeight;

    if (width / height < (16.0f / 9.0f)) {
        return (width / (16.0f / 9.0f)) / 1080.0f;
    }
    return height / 1080.0f;
}

/* 0xF8B60, 436 bytes, global, 3 named locals
 * setPivotPosition
 * PDB type: void (float*, float*, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void setPivotPosition(float *x, float *y, int pivot)
{
    *x *= scaleAdjustment;
    *y *= scaleAdjustment;

    switch (pivot) {
    case 1:
        *x += (float)OptionStruct.ScreenWidth * 0.5f;
        break;
    case 2:
        *x = (float)OptionStruct.ScreenWidth - *x;
        break;
    case 3:
        *y += (float)OptionStruct.ScreenHeight * 0.5f;
        break;
    case 4:
        *x += (float)OptionStruct.ScreenWidth * 0.5f;
        *y += (float)OptionStruct.ScreenHeight * 0.5f;
        break;
    case 5:
        *x = (float)OptionStruct.ScreenWidth - *x;
        *y += (float)OptionStruct.ScreenHeight * 0.5f;
        break;
    case 6:
        *y = (float)OptionStruct.ScreenHeight - *y;
        break;
    case 7:
        *x += (float)OptionStruct.ScreenWidth * 0.5f;
        *y = (float)OptionStruct.ScreenHeight - *y;
        break;
    case 8:
        *x = (float)OptionStruct.ScreenWidth - *x;
        *y = (float)OptionStruct.ScreenHeight - *y;
        break;
    default:
        break;
    }
}

/* 0xF8D20, 552 bytes, global, 5 named locals
 * setPivotPositionAndFixScale
 * PDB type: void (float*, float*, float*, fl...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void setPivotPositionAndFixScale(
    float *x,
    float *y,
    float *width,
    float *height,
    int pivot)
{
    *x *= scaleAdjustment;
    *y *= scaleAdjustment;
    *width *= scaleAdjustment;
    *height *= scaleAdjustment;

    switch (pivot) {
    case 1:
        *x += ((float)OptionStruct.ScreenWidth - *width) * 0.5f;
        break;
    case 2:
        *x = (float)OptionStruct.ScreenWidth - *x - *width;
        break;
    case 3:
        *y += (float)OptionStruct.ScreenHeight * 0.5f;
        break;
    case 4:
        *x += ((float)OptionStruct.ScreenWidth - *width) * 0.5f;
        *y += (float)OptionStruct.ScreenHeight * 0.5f;
        break;
    case 5:
        *x = (float)OptionStruct.ScreenWidth - *x - *width;
        *y += (float)OptionStruct.ScreenHeight * 0.5f;
        break;
    case 6:
        *y = (float)OptionStruct.ScreenHeight - *y - *height;
        break;
    case 7:
        *x += ((float)OptionStruct.ScreenWidth - *width) * 0.5f;
        *y = (float)OptionStruct.ScreenHeight - *y - *height;
        break;
    case 8:
        *x = (float)OptionStruct.ScreenWidth - *x - *width;
        *y = (float)OptionStruct.ScreenHeight - *y - *height;
        break;
    default:
        break;
    }
}

/* 0xF8F50, 552 bytes, global, 5 named locals
 * setPivotPositionAndFixScaleMM
 * PDB type: void (float*, float*, float*, fl...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void setPivotPositionAndFixScaleMM(
    float *x,
    float *y,
    float *width,
    float *height,
    int pivot)
{
    *x *= scaleAdjustmentMM;
    *y *= scaleAdjustmentMM;
    *width *= scaleAdjustmentMM;
    *height *= scaleAdjustmentMM;

    switch (pivot) {
    case 1:
        *x += ((float)OptionStruct.ScreenWidth - *width) * 0.5f;
        break;
    case 2:
        *x = (float)OptionStruct.ScreenWidth - *x - *width;
        break;
    case 3:
        *y += (float)OptionStruct.ScreenHeight * 0.5f;
        break;
    case 4:
        *x += ((float)OptionStruct.ScreenWidth - *width) * 0.5f;
        *y += (float)OptionStruct.ScreenHeight * 0.5f;
        break;
    case 5:
        *x = (float)OptionStruct.ScreenWidth - *x - *width;
        *y += (float)OptionStruct.ScreenHeight * 0.5f;
        break;
    case 6:
        *y = (float)OptionStruct.ScreenHeight - *y - *height;
        break;
    case 7:
        *x += ((float)OptionStruct.ScreenWidth - *width) * 0.5f;
        *y = (float)OptionStruct.ScreenHeight - *y - *height;
        break;
    case 8:
        *x = (float)OptionStruct.ScreenWidth - *x - *width;
        *y = (float)OptionStruct.ScreenHeight - *y - *height;
        break;
    default:
        break;
    }
}

/* 0xF9180, 544 bytes, global, 7 named locals
 * setPivotPositionMM
 * PDB type: void (float*, float*, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void setPivotPositionMM(float *x, float *y, int pivot)
{
    float screen_width = (float)OptionStruct.ScreenWidth;
    float screen_height = (float)OptionStruct.ScreenHeight;
    float content_width;
    float content_height;
    float x_offset;
    float y_offset;

    if (screen_width / screen_height >= (16.0f / 9.0f)) {
        content_height = screen_height;
        content_width = content_height * (16.0f / 9.0f);
        x_offset = (screen_width - content_width) * 0.5f;
        y_offset = 0.0f;
    } else {
        content_width = screen_width;
        content_height = content_width / (16.0f / 9.0f);
        x_offset = 0.0f;
        y_offset = (screen_height - content_height) * 0.5f;
    }
    *x *= scaleAdjustmentMM;
    *y *= scaleAdjustmentMM;
    switch (pivot) {
    case 0:
        *x += x_offset;
        *y += y_offset;
        break;
    case 1:
        *x += screen_width * 0.5f;
        *y += y_offset;
        break;
    case 2:
        *x = x_offset + content_width - *x;
        *y += y_offset;
        break;
    case 3:
        *x += x_offset;
        *y += screen_height * 0.5f;
        break;
    case 4:
        *x += screen_width * 0.5f;
        *y += screen_height * 0.5f;
        break;
    case 5:
        *x = x_offset + content_width - *x;
        *y += screen_height * 0.5f;
        break;
    case 6:
        *x += x_offset;
        *y = y_offset + content_height - *y;
        break;
    case 7:
        *x += screen_width * 0.5f;
        *y = y_offset + content_height - *y;
        break;
    case 8:
        *x = x_offset + content_width - *x;
        *y = y_offset + content_height - *y;
        break;
    default:
        break;
    }
}

/* 0xF93A0, 560 bytes, global, 7 named locals
 * setPivotPositionMM_PSX
 * PDB type: void (float*, float*, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void setPivotPositionMM_PSX(float *x, float *y, int pivot)
{
    float screen_width = (float)OptionStruct.ScreenWidth;
    float screen_height = (float)OptionStruct.ScreenHeight;
    float content_width;
    float content_height;
    float x_offset = 0.0f;
    float y_offset = 0.0f;

    if (screen_width / screen_height < (16.0f / 9.0f)) {
        content_width = screen_width;
        content_height = screen_width / (16.0f / 9.0f);
        y_offset =
            (screen_height - content_height) * 0.5f /
            gPSXDrawScaleY;
    } else {
        content_height = screen_height;
        content_width = screen_height * (16.0f / 9.0f);
        x_offset =
            (screen_width - content_width) * 0.5f /
            gPSXDrawScaleX;
    }
    *x *= scaleAdjustmentMM;
    *y *= scaleAdjustmentMM;
    switch (pivot) {
    case 0:
        *x += x_offset;
        *y += y_offset;
        break;
    case 1:
        *x += screen_width * 0.5f;
        *y += y_offset;
        break;
    case 2:
        *x = x_offset + content_width - *x;
        *y += y_offset;
        break;
    case 3:
        *x += x_offset;
        *y += screen_height * 0.5f;
        break;
    case 4:
        *x += screen_width * 0.5f;
        *y += screen_height * 0.5f;
        break;
    case 5:
        *x = x_offset + content_width - *x;
        *y += screen_height * 0.5f;
        break;
    case 6:
        *x += x_offset;
        *y = y_offset + content_height - *y;
        break;
    case 7:
        *x += screen_width * 0.5f;
        *y = y_offset + content_height - *y;
        break;
    case 8:
        *x = x_offset + content_width - *x;
        *y = y_offset + content_height - *y;
        break;
    default:
        break;
    }
}

/* 0xF95D0, 41 bytes, global, 4 named locals
 * setPositionOffPivot
 * PDB type: void (float*, float*, float, flo...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void setPositionOffPivot(
    float *x, float *y, float pivot_x, float pivot_y)
{
    *x = *x * scaleAdjustment + pivot_x;
    *y = *y * scaleAdjustment + pivot_y;
}

/* 0xF9600, 41 bytes, global, 4 named locals
 * setPositionOffPivotMM
 * PDB type: void (float*, float*, float, flo...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void setPositionOffPivotMM(
    float *x, float *y, float pivot_x, float pivot_y)
{
    *x = *x * scaleAdjustmentMM + pivot_x;
    *y = *y * scaleAdjustmentMM + pivot_y;
}

/* 0xF9630, 607 bytes, global, 3 named locals
 * sprite_AddCallBack
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
int sprite_AddCallBack(int32_t *cb)
{
    Sprite *sptr = (Sprite *)cb;
    VECTOR pos;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return 0;
    }

    --sptr->sp_Delay;
    if (sptr->sp_Delay > 0) {
        return 0;
    }

    pos.vx = (int32_t)sptr->sp_Pos.vx;
    pos.vy = (int32_t)sptr->sp_Pos.vy;
    pos.vz = (int32_t)sptr->sp_Pos.vz;
    pos.pad = 0;
    if (((uint32_t)sptr->sp_Flags & UINT32_C(0x10000)) == 0) {
        pos.vx = (int32_t)((float)pos.vx + sptr->sp_Vel.vx);
        pos.vy = (int32_t)((float)pos.vy + sptr->sp_Vel.vy);
        pos.vz = (int32_t)((float)pos.vz + sptr->sp_Vel.vz);
    } else {
        pos.vx = (int32_t)(
            sptr->sp_Vel.vx * (float)rand() / 32767.0f -
            sptr->sp_Vel.vx * 0.5f +
            (float)pos.vx);
        pos.vy = (int32_t)(
            sptr->sp_Vel.vy * (float)rand() / 32767.0f -
            sptr->sp_Vel.vy * 0.5f +
            (float)pos.vy);
        pos.vz = (int32_t)(
            sptr->sp_Vel.vz * (float)rand() / 32767.0f -
            sptr->sp_Vel.vz * 0.5f +
            (float)pos.vz);
    }

    /*
     * scene_gSetStrobe is an exact three-byte return-only leaf in the
     * matched executable. Its 0x20000 branch therefore has no state effect.
     */
    if (((uint32_t)sptr->sp_Flags & UINT32_C(0x40000)) != 0) {
        camera_SetShake(6);
    }

    if (sptr->sp_Rot.vx != 0 ||
        sptr->sp_Rot.vy != 0 ||
        sptr->sp_Rot.vz != 0) {
        effectRotation = sptr->sp_Rot;
        mEffectRot = 1;
    }
    sprite_AddSpriteEffect(
        paEffects[sptr->sp_Type]->aEffects,
        (int)paEffects[sptr->sp_Type]->num,
        &pos,
        NULL);
    mEffectRot = 0;

    if (sptr->sp_cBright.limit == 0) {
        jpb_sprite_mark_free(sptr);
    } else {
        sptr->sp_Delay = sptr->sp_cBright.vel;
        --sptr->sp_cBright.limit;
    }
    return 0;
}

/* 0xF9890, 56 bytes, global, 5 named locals
 * sprite_AddProjectile
 * PDB type: Sprite* (Projectile*, VECTOR*, i...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Sprite *sprite_AddProjectile(
    Projectile *proj, VECTOR *pos, int32_t *callback, int type)
{
    Sprite *sptr = sprite_AddSpriteAtLoc(NULL, type, pos);

    if (sptr != NULL) {
        proj->pj_Parent = sptr;
        sptr->sp_User = (int32_t *)(void *)proj;
        sptr->sp_Func = (SpriteFunction)(void *)callback;
    }
    return sptr;
}

/* 0xF98D0, 177 bytes, global, 4 named locals
 * sprite_AddSpriteAtLoc
 * PDB type: Sprite* (Sprite*, int, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Sprite *sprite_AddSpriteAtLoc(Sprite *sptr, int type, VECTOR *pos)
{
    _Material *th;

    if ((unsigned)type >= JPB_RESIDENT_SPRITE_COUNT) {
        return NULL;
    }
    th = effects1Handle[type];
    if (sptr == NULL) {
        sptr = sprite_gAllocSprite(8);
        if (sptr == NULL) {
            return NULL;
        }
    }
    sptr->sp_SCB->scb_Texture = th;
    sprite_gSetSpritePosition(sptr, pos->vx, pos->vy, pos->vz);
    sptr->sp_cScale.init = 0x1000;
    sptr->sp_Time = 0;
    sptr->sp_SCB->scb_vertex0.pad = 0x80;
    sptr->sp_Func = (SpriteFunction)(void *)sprite_Flash;
    sptr->sp_User = (int32_t *)(void *)pos;
    sptr->sp_Num = 3;
    return sptr;
}

/* 0xF9990, 270 bytes, global, 7 named locals
 * sprite_AddSpriteAtNode
 * PDB type: Sprite* (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

Sprite *sprite_AddSpriteAtNode(int playernum, int nodeID, int type)
{
    VECTOR dummypos = {0, 0, 0, 0};
    Sprite *sptr = NULL;

    if ((unsigned)type < JPB_RESIDENT_SPRITE_COUNT) {
        sptr = sprite_gAllocSprite(8);
        if (sptr != NULL) {
            sptr->sp_SCB->scb_Texture = effects1Handle[type];
            sprite_gSetSpritePosition(sptr, 0, 0, 0);
            sptr->sp_Time = 0;
            sptr->sp_cScale.init = 0x1000;
            sptr->sp_SCB->scb_vertex0.pad = 0x80;
            sptr->sp_Num = 3;
            sptr->sp_User = (int32_t *)(void *)&dummypos;
            sptr->sp_Func = (SpriteFunction)(void *)sprite_Flash;
            sptr->sp_Flags |= 4;
            sptr->sp_Delay = 0;
            sptr->sp_Flags |=
                (int32_t)(((uint32_t)playernum << 8 |
                           (uint32_t)nodeID) << 16);
            (void)debug_printf(
                "ADD SPRITE AT NODE %d of PLAYER %d\n",
                nodeID,
                playernum);
        }
    }
    return sptr;
}

/* 0xF9AA0, 1581 bytes, global, 14 named locals
 * sprite_AddSpriteEffect
 * PDB type: Sprite** (EffectData*, int, VECT...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Sprite **sprite_AddSpriteEffect(
    EffectData *data, int num, VECTOR *loc, _svector *vel)
{
    MATRIX m = {
        {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}
        },
        {0, 0, 0}
    };
    int index;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return NULL;
    }

    memset(shot, 0, sizeof(shot));
    if (mEffectRot != 0) {
        PushMatrix();
        fRotMatrix(&effectRotation, &m);
    }
    if (data == NULL) {
        if (mEffectRot != 0) {
            PopMatrix();
        }
        return NULL;
    }

    for (index = 0; index < num; ++index) {
        EffectData *effect = &data[index];

        if ((effect->bank & UINT8_C(0xfd)) == 0) {
            continue;
        }
        if (effect->bank == 4) {
            shot[index] = (Sprite *)sprite_FireRing(
                (RingData *)effect, loc);
            continue;
        }
        if (effect->bank == 3) {
            VECTOR pos = {
                (int32_t)effect->pos.vx + loc->vx,
                (int32_t)effect->pos.vy + loc->vy,
                (int32_t)effect->pos.vz + loc->vz,
                0
            };

            if (effect->delay == 0) {
                EffectHeader *header = paEffects[effect->type];

                sprite_AddSpriteEffect(
                    header->aEffects,
                    (int)header->num,
                    &pos,
                    vel);
            } else {
                Sprite *sptr = sprite_gAllocSprite(0);

                if (sptr != NULL) {
                    sptr->sp_Pos.vx = (float)loc->vx;
                    sptr->sp_Pos.vy = (float)loc->vy;
                    sptr->sp_Pos.vz = (float)loc->vz;
                    sptr->sp_Vel.vx = (float)effect->pos.vx;
                    sptr->sp_Vel.vy = (float)effect->pos.vy;
                    sptr->sp_Vel.vz = (float)effect->pos.vz;
                    sptr->sp_Type = effect->type;
                    sptr->sp_cBright.init = effect->delay;
                    sptr->sp_Delay = effect->delay;
                    sptr->sp_cBright.limit = effect->acc.vx;
                    sptr->sp_cBright.vel = effect->acc.vy;
                    sptr->sp_Rot.vx = effect->vel.vx;
                    sptr->sp_Rot.vy = effect->vel.vy;
                    sptr->sp_Rot.vz = effect->vel.vz;
                    sptr->sp_Flags |=
                        (int32_t)((uint32_t)effect->flags << 16);
                    sptr->sp_Func = sprite_AddCallBack;
                }
            }
            continue;
        }

        {
            VECTOR pos = {
                (int32_t)effect->pos.vx + loc->vx,
                (int32_t)effect->pos.vy + loc->vy,
                (int32_t)effect->pos.vz + loc->vz,
                0
            };
            _Material *th;
            Sprite *sptr;
            SCB *scb;

            if (effect->type >= JPB_RESIDENT_SPRITE_COUNT) {
                break;
            }
            th = effects1Handle[effect->type];
            sptr = sprite_gAllocSprite(8);
            if (sptr == NULL) {
                break;
            }

            scb = sptr->sp_SCB;
            scb->scb_Texture = th;
            sprite_gSetSpritePosition(
                sptr, pos.vx, pos.vy, pos.vz);
            sptr->sp_cScale.init = 0x1000;
            scb->scb_vertex0.pad = 0x80;
            sptr->sp_User = (int32_t *)&pos;
            sptr->sp_Num = 3;
            shot[index] = sptr;
            sptr->sp_Func = sprite_MainCallBack;
            sptr->sp_Time = 0;
            sptr->sp_Delay = effect->delay;
            sptr->sp_Vel.vx = (float)effect->vel.vx;
            sptr->sp_Vel.vy = (float)effect->vel.vy;
            sptr->sp_Vel.vz = (float)effect->vel.vz;
            sptr->sp_Acc.vx = (float)effect->acc.vx;
            sptr->sp_Acc.vy = (float)effect->acc.vy;
            sptr->sp_Acc.vz = (float)effect->acc.vz;
            if (mEffectRot != 0) {
                fApplyMatrixSFV(&m, &sptr->sp_Vel, &sptr->sp_Vel);
                fApplyMatrixSFV(&m, &sptr->sp_Acc, &sptr->sp_Acc);
            }
            if (vel != NULL) {
                sptr->sp_Vel.vx += (float)vel->vx;
                sptr->sp_Vel.vy += (float)vel->vy;
                sptr->sp_Vel.vz += (float)vel->vz;
            }
            sptr->sp_Rot.vx = effect->rx;
            sptr->sp_RVel.vx = effect->rvx;
            scb->scb_cvertex.vx = (float)effect->rx;
            sptr->sp_cBright = effect->bright;
            sptr->sp_cScale = effect->scale;
            sptr->sp_Flags |=
                (int32_t)((uint32_t)effect->flags << 16);

            if ((effect->flags & UINT32_C(0x10)) != 0) {
                scb->scb_flags |= 0x200;
                scb->scb_flags &= ~0x80;
            }
            if ((effect->flags & UINT32_C(0x20)) != 0) {
                _svector dir = {0x400, 0, 0, 0};
                VECTOR temp;
                MATRIX rmat = {
                    {
                        {1.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f},
                        {0.0f, 0.0f, 1.0f}
                    },
                    {0, 0, 0}
                };

                scb->scb_flags = 4;
                rot.vx = (int16_t)((uint16_t)rot.vx + 0x100u);
                rot.vz = (int16_t)((uint16_t)rot.vz + 0x100u);
                PushMatrix();
                fRotMatrix(&rot, &rmat);
                fApplyMatrix(&rmat, &dir, &temp);
                PopMatrix();
                temp.vx += loc->vx;
                temp.vy += loc->vy;
                temp.vz += loc->vz;

                scb->scb_vertex0.vx = (float)loc->vx;
                scb->scb_vertex0.vy = (float)loc->vy;
                scb->scb_vertex0.vz = (float)loc->vz;
                scb->scb_vertex2.vx = (float)loc->vx;
                scb->scb_vertex2.vy = (float)(loc->vy + 8);
                scb->scb_vertex2.vz = (float)loc->vz;
                scb->scb_vertex1.vx = (float)temp.vx;
                scb->scb_vertex1.vy = (float)temp.vy;
                scb->scb_vertex1.vz = (float)temp.vz;
                scb->scb_vertex3.vx = (float)temp.vx;
                scb->scb_vertex3.vy = (float)(temp.vy + 0x30);
                scb->scb_vertex3.vz = (float)temp.vz;
            }
            scb->scb_cvertex.pad = effect->vel.pad;
            scb->scb_flags |=
                (int32_t)((uint32_t)effect->flags << 16);
        }
    }
    if (mEffectRot != 0) {
        PopMatrix();
    }
    return shot;
}

/* 0xFA0D0, 102 bytes, global, 6 named locals
 * sprite_AddSpriteEffectAtNode
 * PDB type: Sprite** (EffectData*, int, int,...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Sprite **sprite_AddSpriteEffectAtNode(
    EffectData *data, int num, int playernum, int nodeID)
{
    VECTOR *loc = coll_GetNodeCenter(playernum, nodeID);
    _svector *vel =
        coll_GetNodeVelocity(playernum, nodeID);

    return sprite_AddSpriteEffect(data, num, loc, vel);
}

/* 0xFA140, 113 bytes, global, 2 named locals
 * sprite_AllocRing
 * PDB type: Ring* ()
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Ring *sprite_AllocRing(void)
{
    Ring *ring = (Ring *)memalloc((unsigned)sizeof(*ring));

    if (ring == NULL) {
        return NULL;
    }
    memset(ring, 0, sizeof(*ring));
    list_AddTail(&mSpriteWork[mCurSpriteList], (Node *)ring);
    ring->sp_Next = NULL;
    ring->sp_Type = -1;
    return ring;
}

/* 0xFA1C0, 263 bytes, global, 5 named locals
 * sprite_Center
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
int sprite_Center(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    VECTOR *position = (VECTOR *)(void *)sptr->sp_User;
    SCB *scb = sptr->sp_SCB;
    int walk_height =
        intersec_FindWalkHeight(position, NULL, NULL, 0);
    int radius =
        (int)sptr->sp_Num -
        (position->vy - walk_height) / 16;

    if (radius < 24) {
        radius = 24;
    } else if (radius > 256) {
        radius = 256;
    }

    scb->scb_vertex0.vx = (float)(position->vx - radius);
    scb->scb_vertex0.vy = (float)walk_height;
    scb->scb_vertex0.vz = (float)(position->vz + radius);
    scb->scb_vertex2.vx = (float)(position->vx - radius);
    scb->scb_vertex2.vy = (float)walk_height;
    scb->scb_vertex2.vz = (float)(position->vz - radius);
    scb->scb_vertex1.vx = (float)(position->vx + radius);
    scb->scb_vertex1.vy = (float)walk_height;
    scb->scb_vertex1.vz = (float)(position->vz + radius);
    scb->scb_vertex3.vx = (float)(position->vx + radius);
    scb->scb_vertex3.vy = (float)walk_height;
    scb->scb_vertex3.vz = (float)(position->vz - radius);
    return 0;
}

/* 0xFA2D0, 18 bytes, global, 2 named locals
 * sprite_ClearProjectilePool
 * PDB type: void (Projectile*, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_ClearProjectilePool(Projectile *pool, int num)
{
    if (num > 0) {
        memset(pool, 0, (size_t)num * sizeof(*pool));
    }
}

/* 0xFA2F0, 190 bytes, global, 2 named locals
 * sprite_CommentsCallBack
 * PDB type: void (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_CommentsCallBack(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;

    Draw3dText(
        sptr->sp_Pos.vx,
        sptr->sp_Pos.vy,
        sptr->sp_Pos.vz,
        1.5f,
        (uint32_t)(uintptr_t)sptr->sp_PAnim,
        "%s",
        (char *)(void *)sptr->sp_Anim);
    sptr->sp_Pos.vx += sptr->sp_Vel.vx;
    sptr->sp_Pos.vy += sptr->sp_Vel.vy;
    sptr->sp_Pos.vz += sptr->sp_Vel.vz;
    if ((uint32_t)(uintptr_t)sptr->sp_User < gGlobalTimer) {
        jpb_sprite_mark_free(sptr);
    }
}

/* 0xFA3B0, 234 bytes, global, 7 named locals
 * sprite_DisplaySprite
 * PDB type: SCB* (SCB*, int, int, int, int, ...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
SCB *sprite_DisplaySprite(
    SCB *scb,
    int type,
    int x,
    int y,
    int w,
    int h,
    int clut)
{
    if (scb == NULL) {
        scb = sprite_gAllocSCB();
        if (scb == NULL) {
            return NULL;
        }
    }
    scb->scb_Texture = effects1Handle[type];
    scb->scb_cvertex.pad = (int16_t)clut;
    if (jpb_sprite_display_hook != NULL) {
        jpb_sprite_display_hook(
            jpb_sprite_display_user_data,
            type,
            x,
            y,
            w,
            h,
            clut,
            scb->scb_Texture);
    }
    scb->scb_flags |= 2;
    scb->scb_vertex0.vx = (float)x;
    scb->scb_vertex0.vy = (float)y;
    scb->scb_vertex0.vz = 0.0f;
    scb->scb_vertex0.pad = -1;
    scb->scb_vertex1.vx = (float)(x + w);
    scb->scb_vertex1.vy = (float)y;
    scb->scb_vertex1.vz = 0.0f;
    scb->scb_vertex1.pad = -1;
    scb->scb_vertex2.vx = (float)x;
    scb->scb_vertex2.vy = (float)(y + h);
    scb->scb_vertex2.vz = 0.0f;
    scb->scb_vertex2.pad = -1;
    scb->scb_vertex3.vx = (float)(x + w);
    scb->scb_vertex3.vy = (float)(y + h);
    scb->scb_vertex3.vz = 0.0f;
    return scb;
}

/* 0xFA4A0, 193 bytes, global, 6 named locals
 * sprite_DisplayTpage
 * PDB type: SCB* (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

SCB *sprite_DisplayTpage(int tpage, int x, int y)
{
    SCB *scb = sprite_gAllocSCB();
    int half_width;
    int half_height;

    (void)tpage;
    if (scb != NULL) {
        scb->scb_flags |= 2;
        if (scb->scb_Texture == NULL) {
            half_width = 64;
            half_height = 64;
        } else {
            half_width = scb->scb_Texture->iw / 2;
            half_height = scb->scb_Texture->ih / 2;
        }
        sprite_Set2DSCBPos(
            scb, x, y, 0, half_width, half_height);
    }
    return scb;
}

/* 0xFA570, 3 bytes, global, 2 named locals
 * sprite_FireEmiter
 * PDB type: void (Emiter*, Sprite*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_FireEmiter(Emiter *emit, Sprite *shot0)
{
    (void)emit;
    (void)shot0;
}

/* 0xFA580, 633 bytes, global, 9 named locals
 * sprite_FireRing
 * PDB type: Ring* (RingData*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Ring *sprite_FireRing(RingData *r, VECTOR *pos0)
{
    Ring *ring;

    if (((uint16_t)r->rot.pad & UINT16_C(0x40)) == 0) {
        ring = sprite_AllocRing();
        if (ring != NULL) {
            ring->pRingData = r;
            ring->pos = *pos0;
            ring->rot.vx = r->rot.vx;
            ring->rot.vy = r->rot.vy;
            ring->rot.vz = r->rot.vz;
            ring->rad1 = r->r1.init;
            ring->rad2 = r->r2.init;
            ring->rad1v = r->r1.vel;
            ring->rad2v = r->r2.vel;
            ring->b1 = r->b.init;
            ring->b1v = r->b.vel;
            ring->h1 = r->h1.init;
            ring->h2 = r->h2.init;
            ring->h1v = r->h1.vel;
            ring->h2v = r->h2.vel;
            ring->time = r->time + gGlobalTimer;
        }
        return ring;
    }

    {
        Ring *ring0 = NULL;
        int scale = (int)r->r1.init * 4;
        int angle_step = (int)r->r1.init * 0x40;
        int angle = 0;
        int index;

        for (index = 0; index < 16; ++index) {
            ring = sprite_AllocRing();
            if (ring != NULL) {
                int h1 = flexmul((int)aCircle[index], scale);
                int h2 = flexmul((int)aCircle[index + 1], scale);
                int next_angle = angle + angle_step;

                ring->pos = *pos0;
                ring->rot.vx = r->rot.vx;
                ring->rot.vy = r->rot.vy;
                ring->rot.vz = r->rot.vz;
                ring->pRingData = r;
                ring->rad1 = (int16_t)flexmul(angle, 1);
                ring->rad2 = (int16_t)flexmul(next_angle, 1);
                ring->rad1v = r->h1.vel;
                ring->rad2v = r->h2.vel;
                ring->b1 = 0;
                ring->b1v = r->b.vel;
                ring->h1 = (int16_t)h1;
                ring->h2 = (int16_t)h2;
                ring->h1v = r->r1.vel;
                ring->h2v = r->r2.vel;
                ring->time = r->time + gGlobalTimer;
                if (index == 0) {
                    ring0 = ring;
                }
            }
            angle += angle_step;
        }
        return ring0;
    }
}

/* 0xFA800, 439 bytes, global, 8 named locals
 * sprite_FireSphere
 * PDB type: Ring* (RingData*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

Ring *sprite_FireSphere(RingData *r, VECTOR *pos0)
{
    Ring *ring0 = NULL;
    int scale = (int)r->r1.init * 4;
    int angle_step = (int)r->r1.init * 0x40;
    int angle = 0;
    int index;

    for (index = 0; index < 16; ++index) {
        Ring *ring = sprite_AllocRing();

        if (ring != NULL) {
            int h1 = flexmul((int)aCircle[index], scale);
            int h2 = flexmul((int)aCircle[index + 1], scale);
            int next_angle = angle + angle_step;

            ring->pos = *pos0;
            ring->rot.vx = r->rot.vx;
            ring->rot.vy = r->rot.vy;
            ring->rot.vz = r->rot.vz;
            ring->pRingData = r;
            ring->rad1 = (int16_t)flexmul(angle, 1);
            ring->rad2 = (int16_t)flexmul(next_angle, 1);
            ring->rad1v = r->h1.vel;
            ring->rad2v = r->h2.vel;
            ring->b1 = 0;
            ring->b1v = r->b.vel;
            ring->h1 = (int16_t)h1;
            ring->h2 = (int16_t)h2;
            ring->h1v = r->r1.vel;
            ring->h2v = r->r2.vel;
            ring->time = r->time + gGlobalTimer;
            if (index == 0) {
                ring0 = ring;
            }
        }
        angle += angle_step;
    }
    return ring0;
}

/* 0xFA9C0, 176 bytes, global, 3 named locals
 * sprite_Flash
 * PDB type: void (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_Flash(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    int16_t old_time = sptr->sp_Time++;

    if (old_time <= 90) {
        sprite_gMoveSpritePosition(
            sptr,
            -sptr->sp_Vel.vx,
            -sptr->sp_Vel.vy,
            -sptr->sp_Vel.vz);
        if (sptr->sp_Delay <= 0) {
            sptr->sp_SCB->scb_flags &= ~UINT32_C(0x40);
        } else {
            --sptr->sp_Delay;
            if ((sptr->sp_SCB->scb_flags & UINT32_C(0x40)) == 0) {
                sptr->sp_SCB->scb_flags |= UINT32_C(0x40);
            }
        }
    } else {
        jpb_sprite_mark_free(sptr);
    }
}

/* 0xFAA70, 716 bytes, global, 13 named locals
 * sprite_Get3DProjectile
 * PDB type: Sprite* (Projectile*, VECTOR*, _...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Sprite *sprite_Get3DProjectile(
    Projectile *proj, VECTOR *cpos, _svector *rotation)
{
    ProjType *type = &((ProjType *)(void *)maProjTypes)[proj->pj_Type];
    int16_t length = (int16_t)type->length;
    int16_t width = (int16_t)type->width;
    Sprite *sptr = proj->pj_Parent;
    Sprite *child = proj->pj_Child;
    SCB *scb;
    VECTOR center;

    if (sptr == NULL) {
        sptr = sprite_AddSpriteAtLoc(
            NULL, (int)type->bulletSprite, cpos);
        if (sptr == NULL) {
            return NULL;
        }
        proj->pj_Parent = sptr;
    }
    if (child == NULL) {
        child = sprite_AddSpriteAtLoc(
            NULL, (int)type->bulletSprite, cpos);
        if (child == NULL) {
            return NULL;
        }
        proj->pj_Child = child;
    }

    center.vx = cpos->vx;
    center.vy = cpos->vy;
    center.vz = cpos->vz;
    center.pad = 0;

    scb = sptr->sp_SCB;
    scb->scb_flags = 4;
    scb->scb_vertex0.vx = 0.0f;
    scb->scb_vertex0.vy = (float)width;
    scb->scb_vertex0.vz = 0.0f;
    scb->scb_vertex1.vx = 0.0f;
    scb->scb_vertex1.vy = (float)width;
    scb->scb_vertex1.vz = (float)-(int)length;
    scb->scb_vertex2.vx = 0.0f;
    scb->scb_vertex2.vy = (float)-(int)width;
    scb->scb_vertex2.vz = 0.0f;
    scb->scb_vertex3.vx = 0.0f;
    scb->scb_vertex3.vy = (float)-(int)width;
    scb->scb_vertex3.vz = (float)-(int)length;
    sprite_Rotate3DSprite(sptr, rotation);
    sprite_Move3DSprite(sptr, &center);

    if (length == 0 && width == 0) {
        length = 1;
        width = 1;
    }
    scb = child->sp_SCB;
    scb->scb_flags = 4;
    scb->scb_vertex0.vx = 0.0f;
    scb->scb_vertex0.vy = (float)width;
    scb->scb_vertex0.vz = 0.0f;
    scb->scb_vertex1.vx = 0.0f;
    scb->scb_vertex1.vy = (float)width;
    scb->scb_vertex1.vz = (float)-(int)length;
    scb->scb_vertex2.vx = 0.0f;
    scb->scb_vertex2.vy = (float)-(int)width;
    scb->scb_vertex2.vz = 0.0f;
    scb->scb_vertex3.vx = 0.0f;
    scb->scb_vertex3.vy = (float)-(int)width;
    scb->scb_vertex3.vz = (float)-(int)length;
    rotation->vz = (int16_t)(rotation->vz + 0x400);
    sprite_Rotate3DSprite(child, rotation);
    sprite_Move3DSprite(child, &center);
    proj->pj_Child = child;
    return sptr;
}

/* 0xFAD40, 381 bytes, global, 7 named locals
 * sprite_Get3DShear
 * PDB type: void (Sprite*, VECTOR*, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_Get3DShear(Sprite *sptr, VECTOR *center, int dist)
{
    MATRIX rmat = {
        {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}
        },
        {0, 0, 0}
    };
    _svector dir = {(int16_t)dist, 0, 0, 0};
    VECTOR pos;
    SCB *scb;

    if (sptr == NULL) {
        return;
    }
    scb = sptr->sp_SCB;
    scb->scb_flags = 4;
    rot.vx = (int16_t)(rot.vx + 0x100);
    rot.vz = (int16_t)(rot.vz + 0x100);
    PushMatrix();
    fRotMatrix(&rot, &rmat);
    fApplyMatrix(&rmat, &dir, &pos);
    PopMatrix();
    pos.vx += center->vx;
    pos.vy += center->vy;
    pos.vz += center->vz;
    scb->scb_vertex0.vx = (float)center->vx;
    scb->scb_vertex0.vy = (float)center->vy;
    scb->scb_vertex0.vz = (float)center->vz;
    scb->scb_vertex2.vx = (float)center->vx;
    scb->scb_vertex2.vy = (float)(center->vy + 8);
    scb->scb_vertex2.vz = (float)center->vz;
    scb->scb_vertex1.vx = (float)pos.vx;
    scb->scb_vertex1.vy = (float)pos.vy;
    scb->scb_vertex1.vz = (float)pos.vz;
    scb->scb_vertex3.vx = (float)pos.vx;
    scb->scb_vertex3.vy = (float)(pos.vy + 0x30);
    scb->scb_vertex3.vz = (float)pos.vz;
}

/* 0xFAEC0, 225 bytes, global, 6 named locals
 * sprite_GetBaseNodeMarker
 * PDB type: Sprite* (int, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Sprite *sprite_GetBaseNodeMarker(int player, int distance)
{
    playerObject *player_object =
        player_gGetPlayerPtr(player);
    VECTOR *position =
        physics_gGetPosition(&player_object->playerRoot);
    Sprite *sptr = sprite_gAllocSprite(8);
    SCB *scb;

    if (sptr == NULL) {
        return NULL;
    }
    scb = sptr->sp_SCB;
    scb->scb_Texture = effects1Handle[32];
    sprite_gSetSpritePosition(
        sptr, position->vx, position->vy, position->vz);
    sptr->sp_cScale.init = 0x1000;
    sptr->sp_Time = 0;
    scb->scb_vertex0.pad = 0x80;
    scb->scb_flags = 0x00400004;
    scb->scb_cvertex.pad = 8;
    scb->scb_vertex0.pad = 0x40;
    scb->scb_vertex1.pad = 0x40;
    scb->scb_vertex2.pad = 0x40;
    sptr->sp_Func = sprite_Center;
    sptr->sp_User = (int32_t *)(void *)position;
    sptr->sp_Num = (int16_t)distance;
    return sptr;
}

/* 0xFAFB0, 300 bytes, global, 5 named locals
 * sprite_GetCommentsSprite
 * PDB type: Sprite* (char*, VECTOR*, _svecto...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

Sprite *sprite_GetCommentsSprite(
    char *string,
    VECTOR *pos,
    _svector *vel,
    uint32_t color)
{
    Sprite *sptr = sprite_gAllocSprite(0);

    if (sptr != NULL) {
        sptr->sp_Pos.vx = (float)pos->vx;
        sptr->sp_Pos.vy = (float)pos->vy + 16.0f;
        sptr->sp_Pos.vz = (float)pos->vz;
        sptr->sp_Pos.vx += (float)(rand() % 16 - 8);
        sptr->sp_Pos.vz += (float)(rand() % 16 - 8);
        sptr->sp_Vel.vx = (float)vel->vx;
        sptr->sp_Vel.vy = (float)vel->vy;
        sptr->sp_Vel.vz = (float)vel->vz;
        sptr->sp_PAnim = (PalAnim *)(uintptr_t)color;
        sptr->sp_User = (int32_t *)(uintptr_t)(
            gGlobalTimer + UINT32_C(0x2000));
        sptr->sp_Func =
            (SpriteFunction)(void *)sprite_CommentsCallBack;
        sptr->sp_Anim = (TexAnim *)(void *)string;
    }
    return sptr;
}

/* 0xFB0E0, 319 bytes, global, 6 named locals
 * sprite_GetPointsSprite
 * PDB type: Sprite* (int, VECTOR*, _svector*...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Sprite *sprite_GetPointsSprite(
    int points,
    VECTOR *pos,
    _svector *velocity,
    uint32_t colour,
    int small)
{
    Sprite *sptr = sprite_gAllocSprite(0);

    if (sptr == NULL) {
        return NULL;
    }
    sptr->sp_Num = (int16_t)points;
    sptr->sp_Pos.vx = (float)pos->vx;
    sptr->sp_Pos.vy = (float)pos->vy + 8.0f;
    sptr->sp_Pos.vz = (float)pos->vz;
    sptr->sp_Pos.vx += (float)(rand() % 8 - 4);
    sptr->sp_Pos.vz += (float)(rand() % 8 - 4);
    sptr->sp_Vel.vx = (float)velocity->vx;
    sptr->sp_Vel.vy = (float)velocity->vy;
    sptr->sp_Vel.vz = (float)velocity->vz;
    sptr->sp_PAnim = (PalAnim *)(uintptr_t)colour;
    sptr->sp_User = (int32_t *)(uintptr_t)(
        gGlobalTimer + UINT32_C(0x8000));
    sptr->sp_Func = small != 0
        ? (SpriteFunction)sprite_SmallPointsCallBack
        : (SpriteFunction)sprite_PointsCallBack;
    return sptr;
}

/* 0xFB220, 204 bytes, global, 6 named locals
 * sprite_GetSpotMarker
 * PDB type: Sprite* (int, int, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

Sprite *sprite_GetSpotMarker(int playernum, int dist, VECTOR *pos)
{
    Sprite *sptr = sprite_gAllocSprite(8);

    (void)playernum;
    if (sptr != NULL) {
        SCB *scb = sptr->sp_SCB;

        scb->scb_Texture = effects1Handle[4];
        sprite_gSetSpritePosition(sptr, pos->vx, pos->vy, pos->vz);
        sptr->sp_cScale.init = 0x1000;
        sptr->sp_Time = 0;
        scb->scb_vertex0.pad = 0x80;
        scb->scb_flags = 4;
        scb->scb_cvertex.pad = 0;
        scb->scb_vertex0.pad = 0x80;
        scb->scb_vertex1.pad = 0x80;
        scb->scb_vertex2.pad = 0x80;
        sptr->sp_Func = sprite_Spot;
        sptr->sp_Num = (int16_t)dist;
        sptr->sp_User = (int32_t *)(void *)pos;
    }
    return sptr;
}

/* 0xFB2F0, 181 bytes, global, 4 named locals
 * sprite_GetSuckSpritePos
 * PDB type: void (VECTOR*, _svector*, VECTOR...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_GetSuckSpritePos(
    VECTOR *pos, _svector *dir, VECTOR *center)
{
    MATRIX rmat = {
        {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}
        },
        {0, 0, 0}
    };

    rot.vx = (int16_t)(rot.vx + 0x100);
    rot.vz = (int16_t)(rot.vz + 0x100);
    PushMatrix();
    fRotMatrix(&rot, &rmat);
    fApplyMatrix(&rmat, dir, pos);
    PopMatrix();
    pos->vx += center->vx;
    pos->vy += center->vy;
    pos->vz += center->vz;
}

/* 0xFB3B0, 148 bytes, global, 5 named locals
 * sprite_GetTargetMarker
 * PDB type: Sprite* (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

Sprite *sprite_GetTargetMarker(int playernum, int dist, int type)
{
    playerObject *player = player_gGetPlayerPtr(playernum);
    VECTOR *pos = physics_gGetPosition(&player->playerRoot);
    Sprite *sptr = sprite_AddSpriteAtLoc(NULL, type, pos);

    if (sptr != NULL) {
        SCB *scb = sptr->sp_SCB;

        scb->scb_flags = 0x8004;
        scb->scb_cvertex.pad = 8;
        scb->scb_vertex0.pad = 0x40;
        scb->scb_vertex1.pad = 0x40;
        scb->scb_vertex2.pad = 0x40;
        sptr->sp_Func = sprite_Lock;
        sptr->sp_User = (int32_t *)(void *)pos;
        sptr->sp_Num = (int16_t)dist;
    }
    return sptr;
}

/* 0xFB450, 108 bytes, global, 2 named locals
 * sprite_Glow
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_Glow(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    SCB *scb = sptr->sp_SCB;
    int value;

    value = rand() % 128 + 0x80;
    scb->scb_vertex0.pad = (int16_t)value;
    value = rand() % 128 + 0x80;
    scb->scb_vertex1.pad = (int16_t)value;
    value = rand() % 128 + 0x80;
    scb->scb_vertex2.pad = (int16_t)value;
    return value;
}

/* 0xFB4C0, 170 bytes, global, 6 named locals
 * sprite_InitProjectilePool
 * PDB type: void (Projectile*, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_InitProjectilePool(Projectile *pool, int num)
{
    int index;

    for (index = 0; index < num; ++index) {
        if (pool[index].pj_Child != NULL) {
            jpb_sprite_mark_free(pool[index].pj_Child);
        }
        if (pool[index].pj_Parent != NULL) {
            jpb_sprite_mark_free(pool[index].pj_Parent);
        }
        bullet_FreeProjectile(&pool[index]);
    }
}

/* 0xFB570, 43 bytes, global, 0 named locals
 * sprite_InitSCBPool
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_InitSCBPool(void)
{
    list_InitList(&mSCBDraw[0]);
    list_InitList(&mSCBDraw[1]);
    mCurSCBList = 0;
}

/* 0xFB5A0, 51 bytes, global, 1 named locals
 * sprite_LightMotion
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_LightMotion(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;

    sptr->sp_Rot.vx =
        (int16_t)((sptr->sp_Rot.vx + sptr->sp_RVel.vx) & 0xfff);
    sptr->sp_Rot.vy =
        (int16_t)((sptr->sp_Rot.vy + sptr->sp_RVel.vy) & 0xfff);
    sptr->sp_Rot.vz =
        (int16_t)((sptr->sp_Rot.vz + sptr->sp_RVel.vz) & 0xfff);
    return sptr->sp_Rot.vz;
}

/* 0xFB5E0, 221 bytes, global, 5 named locals
 * sprite_Lock
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_Lock(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    VECTOR *pos = (VECTOR *)(void *)sptr->sp_User;
    SCB *scb = sptr->sp_SCB;
    int size = (int)sptr->sp_Num;
    int y = intersec_FindWalkHeight(pos, NULL, NULL, 0);

    scb->scb_vertex0.vx = (float)(pos->vx - size);
    scb->scb_vertex0.vy = (float)y;
    scb->scb_vertex0.vz = (float)(pos->vz + size);
    scb->scb_vertex2.vx = (float)(pos->vx - size);
    scb->scb_vertex2.vy = (float)y;
    scb->scb_vertex2.vz = (float)(pos->vz - size);
    scb->scb_vertex1.vx = (float)(pos->vx + size);
    scb->scb_vertex1.vy = (float)y;
    scb->scb_vertex1.vz = (float)(pos->vz + size);
    scb->scb_vertex3.vx = (float)(pos->vx + size);
    scb->scb_vertex3.vy = (float)y;
    scb->scb_vertex3.vz = (float)(pos->vz - size);
    return y;
}

/* 0xFB6C0, 49 bytes, global, 4 named locals
 * sprite_LockNode
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_LockNode(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    VECTOR *rpos = (VECTOR *)(void *)sptr->sp_User;

    sprite_gSetSpriteCenter(sptr, rpos->vx, rpos->vy, rpos->vz);
    return rpos->vz;
}

/* 0xFB700, 801 bytes, global, 5 named locals
 * sprite_MainCallBack
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
int sprite_MainCallBack(int32_t *cb)
{
    Sprite *sptr = (Sprite *)cb;
    SCB *scb;
    int16_t limit;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return 0;
    }
    scb = sptr->sp_SCB;

    if (gGlobalFrameRate != 0) {
        int16_t old_time = sptr->sp_Time;

        sptr->sp_Time =
            (int16_t)((uint16_t)sptr->sp_Time + UINT16_C(1));
        if (old_time > 300) {
            jpb_sprite_mark_free(sptr);
        }
    }

    if (sptr->sp_Delay < 0) {
        if (gGlobalFrameRate != 0) {
            ++sptr->sp_Delay;
        }
        if ((scb->scb_flags & 0x40) == 0) {
            scb->scb_flags |= 0x40;
        }
        return 0;
    }

    sptr->sp_Rot.vx =
        (int16_t)((uint16_t)sptr->sp_Rot.vx +
                  (uint16_t)sptr->sp_RVel.vx);
    sptr->sp_Rot.vy =
        (int16_t)((uint16_t)sptr->sp_Rot.vy +
                  (uint16_t)sptr->sp_RVel.vy);
    sptr->sp_Rot.vz =
        (int16_t)((uint16_t)sptr->sp_Rot.vz +
                  (uint16_t)sptr->sp_RVel.vz);
    scb->scb_cvertex.vx = (float)sptr->sp_Rot.vx;

    if (gGlobalFrameRate != 0) {
        sptr->sp_Vel.vx += framerate * sptr->sp_Acc.vx;
        sptr->sp_Vel.vy += framerate * sptr->sp_Acc.vy;
        sptr->sp_Vel.vz += framerate * sptr->sp_Acc.vz;
        sprite_gMoveSpritePosition(
            sptr,
            sptr->sp_Vel.vx * 0.5f * framerate,
            sptr->sp_Vel.vy * 0.5f * framerate,
            sptr->sp_Vel.vz * 0.5f * framerate);
    }

    if (sptr->sp_Delay > 0) {
        if (gGlobalFrameRate != 0) {
            --sptr->sp_Delay;
        }
        if ((scb->scb_flags & 0x40) == 0) {
            scb->scb_flags |= 0x40;
        }
        return 0;
    }

    scb->scb_flags &= ~0x40;
    if (gGlobalFrameRate != 0) {
        sptr->sp_cScale.init = (int16_t)(
            (uint16_t)sptr->sp_cScale.init +
            (uint16_t)flexmul12(
                (int)sptr->sp_cScale.vel, gGlobalFrameRate));
        sptr->sp_cScale.vel = (int16_t)(
            (uint16_t)sptr->sp_cScale.vel +
            (uint16_t)flexmul12(
                (int)sptr->sp_cScale.acc, gGlobalFrameRate));
    }

    limit = sptr->sp_cScale.limit;
    if (limit != 0) {
        int free_sprite;

        if (limit > 0) {
            free_sprite = sptr->sp_cScale.init >= limit;
        } else {
            int absolute_limit = -(int)limit;

            free_sprite = sptr->sp_cScale.init <= absolute_limit;
        }
        if (free_sprite) {
            jpb_sprite_mark_free(sptr);
        }
    }

    if (sptr->sp_cBright.init == 0) {
        return 0;
    }

    if ((((uint32_t)sptr->sp_Flags & UINT32_C(0x80000)) != 0) &&
        mDrawingSurfaceId != 0) {
        scb->scb_vertex0.pad = 0;
        scb->scb_vertex1.pad = 0;
        scb->scb_vertex2.pad = 0;
    } else {
        scb->scb_vertex0.pad = sptr->sp_cBright.init;
        scb->scb_vertex1.pad = sptr->sp_cBright.init;
        scb->scb_vertex2.pad = sptr->sp_cBright.init;
        if (gGlobalFrameRate != 0) {
            sptr->sp_cBright.init = (int16_t)(
                (uint16_t)sptr->sp_cBright.init +
                (uint16_t)flexmul12(
                    (int)sptr->sp_cBright.vel,
                    gGlobalFrameRate));
            sptr->sp_cBright.vel = (int16_t)(
                (uint16_t)sptr->sp_cBright.vel +
                (uint16_t)flexmul12(
                    (int)sptr->sp_cBright.acc,
                    gGlobalFrameRate));
        }
    }

    limit = sptr->sp_cBright.limit;
    if (limit == 0) {
        return 0;
    }
    if (limit > 0) {
        if (sptr->sp_cBright.init < limit) {
            return (uint16_t)limit;
        }
    } else {
        int absolute_limit = -(int)limit;

        if (sptr->sp_cBright.init > absolute_limit) {
            return sptr->sp_cBright.init;
        }
    }
    jpb_sprite_mark_free(sptr);
    return 0;
}

/* 0xFBA30, 233 bytes, global, 3 named locals
 * sprite_Move3DSprite
 * PDB type: void (Sprite*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_Move3DSprite(Sprite *sptr, VECTOR *pos)
{
    SCB *scb;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }
    scb = sptr->sp_SCB;
    scb->scb_vertex0.vx += (float)pos->vx;
    scb->scb_vertex0.vy += (float)pos->vy;
    scb->scb_vertex0.vz += (float)pos->vz;
    scb->scb_vertex1.vx += (float)pos->vx;
    scb->scb_vertex1.vy += (float)pos->vy;
    scb->scb_vertex1.vz += (float)pos->vz;
    scb->scb_vertex2.vx += (float)pos->vx;
    scb->scb_vertex2.vy += (float)pos->vy;
    scb->scb_vertex2.vz += (float)pos->vz;
    scb->scb_vertex3.vx += (float)pos->vx;
    scb->scb_vertex3.vy += (float)pos->vy;
    scb->scb_vertex3.vz += (float)pos->vz;
}

/* 0xFBB20, 366 bytes, global, 10 named locals
 * sprite_OrbitNode
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_OrbitNode(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    VECTOR *rpos = (VECTOR *)(void *)sptr->sp_User;
    int16_t old_time = sptr->sp_Time;
    int time_step = old_time / 8;
    int orbit_step = 0x100;
    _svector dir;
    VECTOR pos;
    MATRIX rmat;

    if (time_step > 0xff) {
        orbit_step = time_step;
        if (time_step > 0x1000) {
            orbit_step = 0x1000;
        }
    }
    sptr->sp_Time = (int16_t)(old_time + time_step);
    dir.vx = (int16_t)((0x800 - old_time) - orbit_step);
    dir.vy = 0;
    dir.vz = 0;
    dir.pad = 0;
    memset(&pos, 0, sizeof(pos));
    PushMatrix();
    fRotMatrix(&sptr->sp_Rot, &rmat);
    fApplyMatrix(&rmat, &dir, &pos);
    sptr->sp_Rot.vx =
        (int16_t)(sptr->sp_Rot.vx + sptr->sp_RVel.vx);
    sptr->sp_Rot.vy =
        (int16_t)(sptr->sp_Rot.vy + sptr->sp_RVel.vy);
    sptr->sp_Rot.vz =
        (int16_t)(sptr->sp_Rot.vz + sptr->sp_RVel.vz);
    sprite_gSetSpriteCenter(
        sptr,
        rpos->vx + pos.vx,
        rpos->vy + pos.vy,
        rpos->vz + pos.vz);
    if (sptr->sp_Time > 0x6ff) {
        sprite_gFreeSprite(sptr);
    }
    PopMatrix();
    return sptr->sp_Time;
}

/* 0xFBC90, 305 bytes, global, 5 named locals
 * sprite_PointsCallBack
 * PDB type: void (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_PointsCallBack(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }
    {
        int remaining =
            (int)(uint32_t)(uintptr_t)sptr->sp_User -
            (int)gGlobalTimer;

        if (remaining > 0) {
            int elapsed = 64 - remaining / 512;
            int alpha = 255 - elapsed * 4;
            char *format =
                sptr->sp_Num < 0 ? "%d" : "+%d";

            if (alpha < 0) {
                alpha = 0;
            }
            Draw3dText(
                sptr->sp_Pos.vx,
                sptr->sp_Pos.vy,
                sptr->sp_Pos.vz,
                (float)elapsed / 256.0f + 1.5f,
                (uint32_t)alpha << 24 |
                    (uint32_t)(uintptr_t)sptr->sp_PAnim,
                format,
                (int)sptr->sp_Num);
            sptr->sp_Pos.vx += sptr->sp_Vel.vx;
            sptr->sp_Pos.vy += sptr->sp_Vel.vy;
            sptr->sp_Pos.vz += sptr->sp_Vel.vz;
            return;
        }
    }
    jpb_sprite_mark_free(sptr);
}

/* 0xFBDD0, 120 bytes, global, 5 named locals
 * sprite_RemoveProjectile
 * PDB type: void (Projectile*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_RemoveProjectile(Projectile *proj)
{
    if (proj->pj_Child != NULL) {
        jpb_sprite_mark_free(proj->pj_Child);
    }
    if (proj->pj_Parent != NULL) {
        jpb_sprite_mark_free(proj->pj_Parent);
    }
    bullet_FreeProjectile(proj);
}

/* 0xFBE50, 582 bytes, global, 4 named locals
 * sprite_RingCallBack
 * PDB type: int (Ring*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_RingCallBack(Ring *ring)
{
    RingData *data = ring->pRingData;
    unsigned color_index;
    uint32_t color;

    if (gGlobalTimer >= ring->time) {
        return 1;
    }

    color_index =
        ((uint16_t)ring->rot.pad & UINT16_C(0x20)) == 0
            ? data->clut
            : (unsigned)ring->pos.pad;
    color = cluts[color_index] |
        ((uint32_t)(uint16_t)ring->b1 << 24);
    if ((int8_t)ring->rot.pad < 0) {
        drawCylinderG(
            &ring->pos,
            &ring->rot,
            (float)ring->rad1,
            (float)ring->rad2,
            (float)ring->h1,
            (float)ring->h2,
            color,
            color);
    } else {
        drawCylinder(
            &ring->pos,
            &ring->rot,
            (float)ring->rad1,
            (float)ring->rad2,
            (float)ring->h1,
            (float)ring->h2,
            color,
            data->ratio,
            data->type,
            0,
            data->tmode);
    }

    ring->rot.vy = (int16_t)(ring->rot.vy + data->rvel);
    if (gGlobalFrameRate != 0) {
        ring->rad1 = (int16_t)(ring->rad1 + ring->rad1v);
        ring->rad2 = (int16_t)(ring->rad2 + ring->rad2v);
        ring->rad1v = (int16_t)(ring->rad1v + data->r1.acc);
        ring->rad2v = (int16_t)(ring->rad2v + data->r2.acc);
        ring->b1 = (int16_t)(ring->b1 + ring->b1v);
        ring->b1v = (int16_t)(ring->b1v + data->b.acc);
        ring->h1 = (int16_t)(ring->h1 + ring->h1v);
        ring->h2 = (int16_t)(ring->h2 + ring->h2v);
        ring->h1v = (int16_t)(ring->h1v + data->h1.acc);
        ring->h2v = (int16_t)(ring->h2v + data->h2.acc);
    }

    if (data->b.limit > 0 && ring->b1 > data->b.limit) {
        ring->b1 = data->b.limit;
        if ((data->rot.pad & 1) != 0) {
            ring->b1v = (int16_t)-data->b.vel;
        }
    } else if (ring->b1 < 1) {
        ring->b1 = 0;
        ring->b1v = 0;
    }
    if (data->r1.limit > 0 && ring->rad1 > data->r1.limit) {
        ring->rad1 = data->r1.limit;
        if ((data->rot.pad & 2) != 0) {
            ring->rad1v = (int16_t)-data->r1.vel;
        }
    }
    if (ring->rad1 < 1) {
        ring->rad1 = 0;
        ring->rad1v = 0;
    }
    if (data->r2.limit > 0 && ring->rad2 > data->r2.limit) {
        ring->rad2 = data->r2.limit;
        if ((data->rot.pad & 8) != 0) {
            ring->rad2v = (int16_t)-data->r2.vel;
        }
    }
    /* The retail body repeats the rad1 low clamp after rad2. */
    if (ring->rad1 < 1) {
        ring->rad1 = 0;
        ring->rad1v = 0;
    }
    if (data->h1.limit > 0 && ring->h1 > data->h1.limit) {
        ring->h1 = data->h1.limit;
    }
    if (data->h2.limit > 0 && ring->h2 > data->h2.limit) {
        ring->h2 = data->h2.limit;
    }
    return 0;
}

/* 0xFC0A0, 843 bytes, global, 7 named locals
 * sprite_Rotate3DSprite
 * PDB type: void (Sprite*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_Rotate3DSprite(Sprite *sptr, _svector *rotation)
{
    MATRIX matrix;
    SCB *scb = sptr->sp_SCB;
    _sfvector *vertices[4] = {
        &scb->scb_vertex0,
        &scb->scb_vertex1,
        &scb->scb_vertex2,
        &scb->scb_vertex3
    };
    int index;

    sptr->sp_Rot.vx = rotation->vx;
    sptr->sp_Rot.vy = rotation->vy;
    sptr->sp_Rot.vz = rotation->vz;

    PushMatrix();
    vec_IdentMatrix(&matrix);
    fRotMatrixZ((int)rotation->vz, &matrix);
    for (index = 0; index < 4; ++index) {
        fApplyMatrixSFV(&matrix, vertices[index], vertices[index]);
    }
    vec_IdentMatrix(&matrix);
    fRotMatrixX((int)rotation->vx, &matrix);
    for (index = 0; index < 4; ++index) {
        fApplyMatrixSFV(&matrix, vertices[index], vertices[index]);
    }
    vec_IdentMatrix(&matrix);
    fRotMatrixY((int)rotation->vy, &matrix);
    for (index = 0; index < 4; ++index) {
        fApplyMatrixSFV(&matrix, vertices[index], vertices[index]);
    }
    PopMatrix();
}

/* 0xFC3F0, 281 bytes, global, 3 named locals
 * sprite_SCBDraw
 * PDB type: void (void*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_SCBDraw(void *matrix0)
{
    MATRIX *matrix = (MATRIX *)matrix0;
    List *source = &mSCBDraw[mCurSCBList];
    List *destination = &mSCBDraw[mCurSCBList ^ 1];
    SCB *scb;

    numSCB = 0;
    SetRotMatrix(matrix);
    SetTransMatrix(matrix);
    list_InitList(destination);
    while ((scb = (SCB *)list_RemoveHead(source)) != NULL) {
        if ((scb->scb_flags & 1) != 0) {
            memfree(scb);
            continue;
        }
        if ((scb->scb_flags & 0x800) != 0) {
            if (scb->scb_vertex1.pad == 0) {
                scb->scb_flags |= 1;
            }
        } else if ((scb->scb_flags & 0x40) == 0) {
            _RenderSprite(matrix, scb);
        }
        ++numSCB;
        list_AddTail(destination, (Node *)scb);
    }
    mCurSCBList ^= 1;
}

/* 0xFC510, 109 bytes, global, 6 named locals
 * sprite_Set2DSCBPos
 * PDB type: void (SCB*, int, int, int, int, ...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_Set2DSCBPos(
    SCB *scb, int x, int y, int z, int w, int h)
{
    scb->scb_vertex0.vx = (float)x;
    scb->scb_vertex1.vx = (float)(x + w);
    scb->scb_vertex3.vx = (float)(x + w);
    scb->scb_vertex2.vx = (float)x;
    scb->scb_vertex0.vy = (float)y;
    scb->scb_vertex1.vy = (float)y;
    scb->scb_vertex2.vy = (float)(y + h);
    scb->scb_vertex3.vy = (float)(y + h);
    scb->scb_vertex0.vz = (float)z;
    scb->scb_vertex1.vz = (float)z;
    scb->scb_vertex2.vz = (float)z;
    scb->scb_vertex3.vz = (float)z;
}

/* 0xFC580, 102 bytes, global, 5 named locals
 * sprite_SetColorCycle
 * PDB type: Sprite* (Sprite*, int, int, int,...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

Sprite *sprite_SetColorCycle(
    Sprite *sptr, int type, int ps, int pe, int pr)
{
    PalAnim *anim = sptr->sp_PAnim;
    ColorCycle *cycle = &anim->pal_cycle[0];

    anim->pal_Data = effects1Handle[type];
    anim->pal_flags |= 1;
    cycle->cc_dcolor = 0;
    cycle->cc_fcolor = (int16_t)ps;
    cycle->cc_current = ps << 12;
    cycle->cc_lcolor = (int16_t)pe;
    cycle->cc_rate = pr;
    cycle->cc_flags |= 2;
    return sptr;
}

/* 0xFC5F0, 12 bytes, global, 3 named locals
 * sprite_SetProjectile
 * PDB type: Projectile* (Sprite*, Projectile...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Projectile *sprite_SetProjectile(
    Sprite *sptr, Projectile *proj, int32_t *callback)
{
    proj->pj_Parent = sptr;
    sptr->sp_User = (int32_t *)(void *)proj;
    sptr->sp_Func = (SpriteFunction)(void *)callback;
    return proj;
}

/* 0xFC600, 105 bytes, global, 6 named locals
 * sprite_SetSpriteEffect
 * PDB type: void (Sprite*, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_SetSpriteEffect(
    Sprite *sptr, int effect, int w, int h)
{
    int scale = 0x1000;

    (void)w;
    (void)h;
    sptr->sp_Num = (int16_t)effect;
    if ((effect & ~4) != 0) {
        if (effect == 8) {
            scale = 0x400;
        } else if (effect == 10) {
            sptr->sp_Time = 0x20;
            sptr->sp_cScale.init = 0x4000;
            return;
        } else if (effect == 12) {
            sptr->sp_Num = -1;
            sptr->sp_Time = 0x80;
            sptr->sp_cScale.init = 0x1000;
            return;
        }
    }
    sptr->sp_Time = 0x80;
    sptr->sp_cScale.init = (int16_t)scale;
}

/* 0xFC670, 155 bytes, global, 3 named locals
 * sprite_SetTrajectory
 * PDB type: int (Sprite*, int, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_SetTrajectory(Sprite *sptr, int velocity, int angle)
{
    int component;

    sptr->sp_Time = 0;
    component = flexmul(rcos(angle), velocity);
    sptr->sp_Vel.vx = (float)component;
    component = flexmul(rsin(angle), velocity);
    if (component < 0) {
        component = -component;
    }
    sptr->sp_Vel.vy = (float)component;
    component = flexmul(rcos(angle), velocity);
    sptr->sp_Vel.vz = (float)component;
    return component;
}

/* 0xFC710, 288 bytes, global, 5 named locals
 * sprite_SmallPointsCallBack
 * PDB type: void (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_SmallPointsCallBack(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }
    {
        int remaining =
            (int)(uint32_t)(uintptr_t)sptr->sp_User -
            (int)gGlobalTimer;

        if (remaining > 0) {
            int elapsed = 64 - remaining / 512;
            int alpha = 255 - elapsed * 4;

            if (alpha < 0) {
                alpha = 0;
            }
            Draw3dText(
                sptr->sp_Pos.vx,
                sptr->sp_Pos.vy,
                sptr->sp_Pos.vz,
                (float)elapsed / 256.0f + 0.8f,
                (uint32_t)alpha << 24 |
                    (uint32_t)(uintptr_t)sptr->sp_PAnim,
                "+%d",
                (int)sptr->sp_Num);
            sptr->sp_Pos.vx += sptr->sp_Vel.vx;
            sptr->sp_Pos.vy += sptr->sp_Vel.vy;
            sptr->sp_Pos.vz += sptr->sp_Vel.vz;
            return;
        }
    }
    jpb_sprite_mark_free(sptr);
}

/* 0xFC830, 166 bytes, global, 3 named locals
 * sprite_Sparks
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_Sparks(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    SCB *scb = sptr->sp_SCB;

    sptr->sp_Vel.vx -= sptr->sp_Acc.vx;
    sptr->sp_Vel.vy -= sptr->sp_Acc.vy;
    sptr->sp_Vel.vz -= sptr->sp_Acc.vz;
    sprite_gMoveSpritePosition(
        sptr, sptr->sp_Vel.vx, sptr->sp_Vel.vy, sptr->sp_Vel.vz);
    scb->scb_vertex0.pad = 0xff;
    scb->scb_vertex1.pad = 0xff;
    scb->scb_vertex2.pad = 0xff;
    sptr->sp_cScale.init =
        (int16_t)(sptr->sp_cScale.init + 0x199);
    if (sptr->sp_cScale.init > 0x17ff) {
        sprite_gFreeSprite(sptr);
    }
    return sptr->sp_cScale.init;
}

/* 0xFC8E0, 199 bytes, global, 5 named locals
 * sprite_Spot
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_Spot(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    VECTOR *pos = (VECTOR *)(void *)sptr->sp_User;
    SCB *scb = sptr->sp_SCB;
    int size = (int)sptr->sp_Num;

    scb->scb_vertex0.vx = (float)(pos->vx - size);
    scb->scb_vertex0.vy = (float)pos->vy;
    scb->scb_vertex0.vz = (float)(pos->vz + size);
    scb->scb_vertex2.vx = (float)(pos->vx - size);
    scb->scb_vertex2.vy = (float)pos->vy;
    scb->scb_vertex2.vz = (float)(pos->vz - size);
    scb->scb_vertex1.vx = (float)(pos->vx + size);
    scb->scb_vertex1.vy = (float)pos->vy;
    scb->scb_vertex1.vz = (float)(pos->vz + size);
    scb->scb_vertex3.vx = (float)(pos->vx + size);
    scb->scb_vertex3.vy = (float)pos->vy;
    scb->scb_vertex3.vz = (float)(pos->vz - size);
    return pos->vz - size;
}

/* 0xFC9B0, 119 bytes, global, 2 named locals
 * sprite_SpriteMotion
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_SpriteMotion(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;

    sprite_gMoveSpritePosition(
        sptr, sptr->sp_Vel.vx, sptr->sp_Vel.vy, sptr->sp_Vel.vz);
    sprite_SpriteRotScale(sptr, 0x18, 0, 0);
    sptr->sp_cScale.init =
        (int16_t)(sptr->sp_cScale.init - 0x14);
    if (sptr->sp_cScale.init < 1) {
        sprite_gFreeSprite(sptr);
    }
    return sptr->sp_cScale.init;
}

/* 0xFCA30, 714 bytes, global, 11 named locals
 * sprite_SpriteRotScale
 * PDB type: void (Sprite*, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

static void sprite_set_rotated_vertex(
    Sprite *sptr,
    MATRIX *rotation,
    _sfvector *destination,
    int x,
    int y)
{
    _svector corner = {(int16_t)x, (int16_t)y, 0, 0};
    _svector transformed;

    fApplyMatrixSV(rotation, &corner, &transformed);
    destination->vx = (float)transformed.vx + sptr->sp_Pos.vx;
    destination->vy = (float)transformed.vy + sptr->sp_Pos.vy;
    destination->vz = (float)transformed.vz + sptr->sp_Pos.vz;
}

void sprite_SpriteRotScale(
    Sprite *sptr, int x_rotation, int y_rotation, int z_rotation)
{
    MATRIX rotation;
    SCB *scb;
    int half_width;
    int half_height;
    int width_product;
    int height_product;
    int scaled_width;
    int scaled_height;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }

    scb = sptr->sp_SCB;
    if (scb->scb_Texture == NULL) {
        half_width = 64;
        half_height = 64;
    } else {
        half_width = scb->scb_Texture->iw / 2;
        half_height = scb->scb_Texture->ih / 2;
    }

    sptr->sp_Rot.vx = (int16_t)(sptr->sp_Rot.vx + x_rotation);
    sptr->sp_Rot.vy = (int16_t)(sptr->sp_Rot.vy + y_rotation);
    sptr->sp_Rot.vz = (int16_t)(sptr->sp_Rot.vz + z_rotation);
    fRotMatrix(&sptr->sp_Rot, &rotation);
    PushMatrix();
    width_product = sptr->sp_cScale.init * half_width;
    height_product = sptr->sp_cScale.init * half_height;
    scaled_width =
        (width_product + (width_product < 0 ? 0xfff : 0)) >> 12;
    scaled_height =
        (height_product + (height_product < 0 ? 0xfff : 0)) >> 12;

    sprite_set_rotated_vertex(
        sptr, &rotation, &scb->scb_vertex0,
        -scaled_width, -scaled_height);
    sprite_set_rotated_vertex(
        sptr, &rotation, &scb->scb_vertex1,
        scaled_width, -scaled_height);
    sprite_set_rotated_vertex(
        sptr, &rotation, &scb->scb_vertex2,
        -scaled_width, scaled_height);
    sprite_set_rotated_vertex(
        sptr, &rotation, &scb->scb_vertex3,
        scaled_width, scaled_height);
    PopMatrix();
}

/* 0xFCD00, 1015 bytes, global, 16 named locals
 * sprite_SpriteWork
 * PDB type: void (MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
static void sprite_update_billboard(Sprite *sptr)
{
    SCB *scb = sptr->sp_SCB;
    int half_width;
    int half_height;
    int width;
    int height;
    int flip_flags = sptr->sp_Flags >> 16;

    if (scb == NULL || (scb->scb_flags & 8) == 0) {
        return;
    }
    if (scb->scb_Texture == NULL) {
        half_width = 64;
        half_height = 64;
    } else {
        half_width = scb->scb_Texture->iw / 2;
        half_height = scb->scb_Texture->ih / 2;
    }

    width = sptr->sp_cScale.init * half_width * 4;
    height = sptr->sp_cScale.init * half_height * 4;
    width = (width + (width < 0 ? 0xfff : 0)) >> 12;
    height = (height + (height < 0 ? 0xfff : 0)) >> 12;
    if ((flip_flags & 1) != 0) {
        if (mDrawingSurfaceId == 0) {
            height = -height;
        } else {
            width = -width;
        }
    }
    if ((flip_flags & 2) != 0 && mDrawingSurfaceId != 0) {
        width = -width;
    }
    if ((flip_flags & 4) != 0 && mDrawingSurfaceId != 0) {
        height = -height;
    }

    scb->scb_vertex0.vx = (float)-width;
    scb->scb_vertex0.vy = (float)-height;
    scb->scb_vertex0.vz = 0.0f;
    scb->scb_vertex1.vx = (float)width;
    scb->scb_vertex1.vy = (float)-height;
    scb->scb_vertex1.vz = 0.0f;
    scb->scb_vertex2.vx = (float)-width;
    scb->scb_vertex2.vy = (float)height;
    scb->scb_vertex2.vz = 0.0f;
    scb->scb_vertex3.vx = (float)width;
    scb->scb_vertex3.vy = (float)height;
    scb->scb_vertex3.vz = 0.0f;
    scb->scb_cvertex.vx = (float)(int32_t)sptr->sp_Pos.vx;
    scb->scb_cvertex.vy = (float)(int32_t)sptr->sp_Pos.vy;
    scb->scb_cvertex.vz = (float)(int32_t)sptr->sp_Pos.vz;
}

static void sprite_update_attached_position(Sprite *sptr)
{
    unsigned player =
        ((uint32_t)sptr->sp_Flags >> 24) & 0xffu;
    int node_id = ((uint32_t)sptr->sp_Flags >> 16) & 0xffu;

    if (maSceneData[player].sceneRoot.objectID != -1) {
        VECTOR *position = coll_GetNodeCenter((int)player, node_id);

        if (position != NULL) {
            sptr->sp_Pos.vx = (float)position->vx;
            sptr->sp_Pos.vy = (float)position->vy;
            sptr->sp_Pos.vz = (float)position->vz;
        }
    }
    sptr->sp_Flags &= ~4;
}

void sprite_SpriteWork(MATRIX *matrix)
{
    int sprite_list = mCurSpriteList;
    List *source_sprites;
    List *destination_sprites;
    Sprite *sptr;

    source_sprites = &mSpriteWork[sprite_list];
    destination_sprites = &mSpriteWork[sprite_list ^ 1];

    numSprite = 0;
    list_InitList(destination_sprites);
    while ((sptr = (Sprite *)list_RemoveHead(source_sprites)) != NULL) {
        uintptr_t address = (uintptr_t)sptr;

        if ((address & 3u) != 0 ||
            address < (uintptr_t)mem_heap ||
            address >= (uintptr_t)mem_heapend) {
            continue;
        }
        if (sptr->sp_Type == -1) {
            if (sprite_RingCallBack((Ring *)sptr) == 0) {
                list_AddTail(destination_sprites, (Node *)sptr);
            } else {
                memfree(sptr);
            }
            continue;
        }

        if (sptr->sp_Func == NULL) {
            jpb_sprite_mark_free(sptr);
        } else {
            sptr->sp_Func((int32_t *)sptr);
        }
        if ((sptr->sp_Flags & 1) != 0) {
            memfree(sptr);
            continue;
        }
        if ((sptr->sp_Flags & 4) != 0) {
            sprite_update_attached_position(sptr);
        }
        sprite_update_billboard(sptr);
        ++numSprite;
        list_AddTail(destination_sprites, (Node *)sptr);
    }
    mCurSpriteList ^= 1;
    sprite_SCBDraw(matrix);
}

/* 0xFD100, 3 bytes, global, 1 named locals
 * sprite_SwapData
 * PDB type: void (SCB*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_SwapData(SCB *scb)
{
    (void)scb;
}

/* 0xFD110, 266 bytes, global, 6 named locals
 * sprite_TrackNode
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_TrackNode(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    VECTOR *rpos = (VECTOR *)(void *)sptr->sp_User;
    const float step = 32.0f;

    sptr->sp_cScale.init =
        (int16_t)(sptr->sp_cScale.init - 0x147);
    sptr->sp_Pos.vx +=
        sptr->sp_Pos.vx <= (float)rpos->vx ? step : -step;
    sptr->sp_Pos.vy +=
        sptr->sp_Pos.vy <= (float)rpos->vy ? step : -step;
    sptr->sp_Pos.vz +=
        sptr->sp_Pos.vz <= (float)rpos->vz ? step : -step;
    sptr->sp_Time = (int16_t)(sptr->sp_Time - 0x10);
    if (sptr->sp_Time < 1 || sptr->sp_cScale.init < 0x199) {
        sprite_gFreeSprite(sptr);
    }
    sprite_gSetSpriteCenter(
        sptr,
        (int)sptr->sp_Pos.vx,
        (int)sptr->sp_Pos.vy,
        (int)sptr->sp_Pos.vz);
    return (int)sptr->sp_Pos.vz;
}

/* 0xFD220, 83 bytes, global, 3 named locals
 * sprite_ViewPoint
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

int sprite_ViewPoint(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    _svector v3Focus;
    VECTOR v3PolarCoord;

    camera_GetCamera(&v3PolarCoord, &v3Focus);
    sprite_gSetSpritePosition(
        sptr, (int)v3Focus.vx, (int)v3Focus.vy, (int)v3Focus.vz);
    return (int)v3Focus.vz;
}

/* 0xFD280, 119 bytes, global, 2 named locals
 * sprite_gAllocPCB
 * PDB type: PCB* ()
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

PCB *sprite_gAllocPCB(void)
{
    PCB *pcb = (PCB *)memalloc((unsigned)sizeof(*pcb));

    if (pcb != NULL && (OptionStruct.FunFactor & 1u) == 0) {
        memset(pcb, 0, sizeof(*pcb));
        list_AddTail(
            &mSCBDraw[mCurSCBList], (Node *)(void *)pcb);
        pcb->pcb_next_PCB = NULL;
        return pcb;
    }
    return NULL;
}

/* 0xFD300, 125 bytes, global, 2 named locals
 * sprite_gAllocSCB
 * PDB type: SCB* ()
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
SCB *sprite_gAllocSCB(void)
{
    SCB *scb = (SCB *)memalloc((unsigned)sizeof(*scb));

    if (scb == NULL || (OptionStruct.FunFactor & 1u) != 0) {
        return NULL;
    }
    memset(scb, 0, sizeof(*scb));
    list_AddTail(&mSCBDraw[mCurSCBList], (Node *)scb);
    scb->scb_next_SCB = NULL;
    return scb;
}

/* 0xFD380, 276 bytes, global, 4 named locals
 * sprite_gAllocSprite
 * PDB type: Sprite* (int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Sprite *sprite_gAllocSprite(int flag)
{
    SCB *scb = NULL;
    Sprite *sptr;

    if (flag != 0) {
        scb = sprite_gAllocSCB();
        if (scb == NULL) {
            return NULL;
        }
        scb->scb_flags = flag;
    }

    sptr = (Sprite *)memalloc((unsigned)sizeof(*sptr));
    if (sptr != NULL && (OptionStruct.FunFactor & 1u) == 0) {
        memset(sptr, 0, sizeof(*sptr));
        list_AddTail(&mSpriteWork[mCurSpriteList], (Node *)sptr);
        sptr->sp_Next = NULL;
        sptr->sp_SCB = scb;
        sptr->sp_cScale.init = 0x1000;
        if (scb != NULL) {
            scb->scb_vertex0.pad = 0x80;
            scb->scb_vertex1.pad = 0x80;
            scb->scb_vertex2.pad = 0x80;
            sptr->sp_Func = NULL;
        }
        return sptr;
    }

    if (scb != NULL &&
        scb->scb_flags != 0 &&
        (scb->scb_flags & 1) == 0) {
        scb->scb_flags |= 1;
    }
    return NULL;
}

/* 0xFD4A0, 23 bytes, global, 1 named locals
 * sprite_gFreeSCB
 * PDB type: void (SCB*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_gFreeSCB(SCB *scb)
{
    if (scb != NULL &&
        scb->scb_flags != 0 &&
        (scb->scb_flags & 1) == 0) {
        scb->scb_flags |= 1;
    }
}

/* 0xFD4C0, 51 bytes, global, 2 named locals
 * sprite_gFreeSprite
 * PDB type: void (Sprite*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_gFreeSprite(Sprite *sptr)
{
    SCB *scb;

    if (sptr == NULL) {
        return;
    }
    if ((sptr->sp_Flags & 1) == 0) {
        sptr->sp_Flags |= 1;
    }
    scb = sptr->sp_SCB;
    if (scb != NULL &&
        scb->scb_flags != 0 &&
        (scb->scb_flags & 1) == 0) {
        scb->scb_flags |= 1;
    }
}

/* 0xFD500, 14 bytes, global, 1 named locals
 * sprite_gHideSCB
 * PDB type: void (SCB*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_gHideSCB(SCB *scb)
{
    if ((scb->scb_flags & 0x40) == 0) {
        scb->scb_flags |= 0x40;
    }
}

/* 0xFD510, 18 bytes, global, 2 named locals
 * sprite_gHideSprite
 * PDB type: void (Sprite*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_gHideSprite(Sprite *sptr)
{
    SCB *scb = sptr->sp_SCB;

    if ((scb->scb_flags & 0x40) == 0) {
        scb->scb_flags |= 0x40;
    }
}

/* 0xFD530, 77 bytes, global, 0 named locals
 * sprite_gInitSprites
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_gInitSprites(void)
{
    list_InitList(&mSCBDraw[0]);
    list_InitList(&mSCBDraw[1]);
    mCurSCBList = 0;
    list_InitList(&mSpriteWork[0]);
    list_InitList(&mSpriteWork[1]);
    mCurSpriteList = 0;
}

/* 0xFD580, 172 bytes, global, 4 named locals
 * sprite_gMoveSCBPosition
 * PDB type: void (SCB*, float, float, float)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_gMoveSCBPosition(
    SCB *scb, float dx, float dy, float dz)
{
    if (dx != 0.0f) {
        scb->scb_vertex0.vx += dx;
        scb->scb_vertex1.vx += dx;
        scb->scb_vertex2.vx += dx;
        scb->scb_vertex3.vx += dx;
    }
    if (dy != 0.0f) {
        scb->scb_vertex0.vy += dy;
        scb->scb_vertex1.vy += dy;
        scb->scb_vertex2.vy += dy;
        scb->scb_vertex3.vy += dy;
    }
    if (dz != 0.0f) {
        scb->scb_vertex0.vz += dz;
        scb->scb_vertex1.vz += dz;
        scb->scb_vertex2.vz += dz;
        scb->scb_vertex3.vz += dz;
    }
}

/* 0xFD630, 306 bytes, global, 11 named locals
 * sprite_gMoveSpritePosition
 * PDB type: void (Sprite*, float, float, flo...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_gMoveSpritePosition(
    Sprite *sptr, float dx, float dy, float dz)
{
    SCB *scb;
    float cx;
    float cy;
    float cz;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }

    scb = sptr->sp_SCB;
    cx = (sptr->sp_Pos.vx + dx) - sptr->sp_Pos.vx;
    cy = (sptr->sp_Pos.vy + dy) - sptr->sp_Pos.vy;
    cz = (sptr->sp_Pos.vz + dz) - sptr->sp_Pos.vz;
    if (cx != 0.0f) {
        scb->scb_vertex0.vx += cx;
        scb->scb_vertex1.vx += cx;
        scb->scb_vertex2.vx += cx;
        scb->scb_vertex3.vx += cx;
    }
    if (cy != 0.0f) {
        scb->scb_vertex0.vy += cy;
        scb->scb_vertex1.vy += cy;
        scb->scb_vertex2.vy += cy;
        scb->scb_vertex3.vy += cy;
    }
    if (cz != 0.0f) {
        scb->scb_vertex0.vz += cz;
        scb->scb_vertex1.vz += cz;
        scb->scb_vertex2.vz += cz;
        scb->scb_vertex3.vz += cz;
    }
    sptr->sp_Pos.vx += dx;
    sptr->sp_Pos.vy += dy;
    sptr->sp_Pos.vz += dz;
}

/* 0xFD770, 296 bytes, global, 5 named locals
 * sprite_gSetBillBrd
 * PDB type: void (Sprite*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_gSetBillBrd(Sprite *sptr)
{
    SCB *scb = sptr->sp_SCB;
    _Material *texture = scb->scb_Texture;
    int half_width;
    int half_height;
    int width;
    int height;
    uint16_t flags =
        (uint16_t)((uint32_t)sptr->sp_Flags >> 16);

    if (texture == NULL) {
        half_width = 64;
        half_height = 64;
    } else {
        half_width = texture->iw / 2;
        half_height = texture->ih / 2;
    }
    width = flexmul(sptr->sp_cScale.init * half_width * 4, 1);
    height = flexmul(sptr->sp_cScale.init * half_height * 4, 1);
    if ((flags & 1) != 0) {
        if (mDrawingSurfaceId == 0) {
            height = -height;
        } else {
            width = -width;
        }
    }
    if ((flags & 2) != 0 && mDrawingSurfaceId != 0) {
        width = -width;
    }
    if ((flags & 4) != 0 && mDrawingSurfaceId != 0) {
        height = -height;
    }
    scb->scb_vertex0.vx = (float)-width;
    scb->scb_vertex0.vy = (float)-height;
    scb->scb_vertex0.vz = 0.0f;
    scb->scb_vertex1.vx = (float)width;
    scb->scb_vertex1.vy = (float)-height;
    scb->scb_vertex1.vz = 0.0f;
    scb->scb_vertex2.vx = (float)-width;
    scb->scb_vertex2.vy = (float)height;
    scb->scb_vertex2.vz = 0.0f;
    scb->scb_vertex3.vx = (float)width;
    scb->scb_vertex3.vy = (float)height;
    scb->scb_vertex3.vz = 0.0f;
    scb->scb_cvertex.vx = sptr->sp_Pos.vx;
    scb->scb_cvertex.vy = sptr->sp_Pos.vy;
    scb->scb_cvertex.vz = sptr->sp_Pos.vz;
}

/* 0xFD8A0, 31 bytes, global, 3 named locals
 * sprite_gSetLineColor
 * PDB type: void (Sprite*, CVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_gSetLineColor(Sprite *sptr, CVECTOR *color)
{
    SCB *scb = sptr->sp_SCB;

    scb->scb_vertex0.pad = color->r;
    scb->scb_vertex1.pad = color->g;
    scb->scb_vertex2.pad = color->b;
}

/* 0xFD8C0, 84 bytes, global, 4 named locals
 * sprite_gSetLinePosition
 * PDB type: void (Sprite*, VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_gSetLinePosition(
    Sprite *sptr, VECTOR *p0, VECTOR *p1)
{
    SCB *scb = sptr->sp_SCB;

    scb->scb_vertex0.vx = (float)p0->vx;
    scb->scb_vertex0.vy = (float)p0->vy;
    scb->scb_vertex0.vz = (float)p0->vz;
    scb->scb_vertex1.vx = (float)p1->vx;
    scb->scb_vertex1.vy = (float)p1->vy;
    scb->scb_vertex1.vz = (float)p1->vz;
}

/* 0xFD920, 24 bytes, global, 4 named locals
 * sprite_gSetRGB
 * PDB type: void (SCB*, char, char, char)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_gSetRGB(SCB *scb, char r, char g, char b)
{
    scb->scb_vertex0.pad = (int16_t)r;
    scb->scb_vertex1.pad = (int16_t)g;
    scb->scb_vertex2.pad = (int16_t)b;
}

/* 0xFD940, 163 bytes, global, 6 named locals
 * sprite_gSetSCBPosition
 * PDB type: void (SCB*, int, int, int, int, ...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_gSetSCBPosition(
    SCB *scb, int x, int y, int z, int w, int h)
{
    _Material *texture = scb->scb_Texture;
    int half_width;
    int half_height;

    (void)w;
    (void)h;
    if (texture == NULL) {
        half_width = 64;
        half_height = 64;
    } else {
        half_width = texture->iw / 2;
        half_height = texture->ih / 2;
    }
    sprite_Set2DSCBPos(
        scb, x, y, z, half_width, half_height);
}

/* 0xFD9F0, 39 bytes, global, 4 named locals
 * sprite_gSetSpriteCenter
 * PDB type: void (Sprite*, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_gSetSpriteCenter(Sprite *sptr, int x, int y, int z)
{
    sptr->sp_Pos.vx = (float)x;
    sptr->sp_Pos.vy = (float)y;
    sptr->sp_Pos.vz = (float)z;
}

/* 0xFDA20, 239 bytes, global, 7 named locals
 * sprite_gSetSpritePosition
 * PDB type: void (Sprite*, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_gSetSpritePosition(
    Sprite *sptr, int x, int y, int z)
{
    SCB *scb;
    _Material *texture;
    int w2 = 0;
    int h2 = 0;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }

    scb = sptr->sp_SCB;
    texture = scb->scb_Texture;
    if (texture != NULL) {
        w2 = texture->iw / 2;
        h2 = texture->ih / 2;
    }

    sptr->sp_Pos.vx = (float)x;
    sptr->sp_Pos.vy = (float)y;
    sptr->sp_Pos.vz = (float)z;
    scb->scb_vertex0.vy = (float)(y - h2);
    scb->scb_vertex1.vy = (float)(y - h2);
    scb->scb_vertex0.vz = (float)(x - w2);
    scb->scb_vertex1.vz = (float)(x + w2);
    scb->scb_vertex2.vz = (float)(x - w2);
    scb->scb_vertex3.vz = (float)(x + w2);
    scb->scb_vertex2.vy = (float)(y + h2);
    scb->scb_vertex3.vy = (float)(y + h2);
    scb->scb_vertex0.vx = (float)z;
    scb->scb_vertex1.vx = (float)z;
    scb->scb_vertex2.vx = (float)z;
    scb->scb_vertex3.vx = (float)z;
}

/* 0xFDB10, 31 bytes, global, 5 named locals
 * sprite_gSetSpriteRGB
 * PDB type: void (Sprite*, char, char, char)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_gSetSpriteRGB(
    Sprite *sptr, char r, char g, char b)
{
    sprite_gSetRGB(sptr->sp_SCB, r, g, b);
}

/* 0xFDB30, 5 bytes, global, 1 named locals
 * sprite_gUnHideSCB
 * PDB type: void (SCB*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */

void sprite_gUnHideSCB(SCB *scb)
{
    scb->scb_flags &= ~INT32_C(0x40);
}

/* 0xFDB40, 9 bytes, global, 2 named locals
 * sprite_gUnHideSprite
 * PDB type: void (Sprite*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_gUnHideSprite(Sprite *sptr)
{
    SCB *scb = sptr->sp_SCB;

    scb->scb_flags &= ~INT32_C(0x40);
}
