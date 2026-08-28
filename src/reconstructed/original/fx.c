/*
 * REVIEWED RECONSTRUCTION of the complete matched fx.c owner.
 * PDB module: 0038
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\fx.obj
 * Primary source: W:\SWJediPowerBattles\work\fx.c
 * Compiler language: c
 * Emitted procedures: 17
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/fx.h"

#include "jpb/bmd.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/flex.h"
#include "jpb/game.h"
#include "jpb/jonnywin.h"
#include "jpb/linkstubs.h"
#include "jpb/menu.h"
#include "jpb/physics.h"
#include "jpb/resources.h"
#include "jpb/scene.h"
#include "jpb/texture.h"
#include "jpb/whook.h"

#include <math.h>
#include <stdlib.h>

static JPBFxScreenGlowHook jpb_screen_glow_hook;
static void *jpb_screen_glow_user_data;

/* Exact fx.c module-local material owners at RVAs 0x537D88..0x537DA0. */
static _Material *translucent_glowtexture;
static _Material *additive_glowtexture;
static _Material *translucent_watertexture;
static _Material *opaque_watertexture;
static _Material *particlematerial;
static _Material *glowtexture;
static uint32_t seed;
static uint32_t gm_col1;
static uint32_t gm_col2;
static physicsObject *gm_p0;
static int32_t gm_radius1;
static int32_t gm_radius2;
static _particle_list *globalparticlelist;

void jpb_FxSetScreenGlowHook(
    JPBFxScreenGlowHook hook, void *user_data)
{
    jpb_screen_glow_hook = hook;
    jpb_screen_glow_user_data = user_data;
}

void jpb_FxInvalidateTextureCache(void)
{
    translucent_glowtexture = NULL;
    additive_glowtexture = NULL;
    translucent_watertexture = NULL;
    opaque_watertexture = NULL;
    particlematerial = NULL;
    glowtexture = NULL;
}

/* 0xA2CB0, 119 bytes, global, 7 named locals
 * PlotZap
 * PDB type: void (unsigned long, unsigned lo...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
void PlotZap(
    uint32_t col1,
    uint32_t col2,
    uint32_t col3,
    _svector *start,
    _svector *end,
    int level,
    int radius)
{
    fx_screenGlow(start, end, radius, col1);
    fx_screenGlow(start, end, radius * 2, col2);
    fx_screenGlow(start, end, radius / 2, col3);
    (void)level;
}

/* 0xA2D30, 155 bytes, global, 7 named locals
 * fx_GlowingMan
 * PDB type: void (objectRoot*, int, int, uns...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
void fx_GlowingMan(
    objectRoot *object,
    int width,
    int height,
    uint32_t inner_color,
    uint32_t outer_color)
{
    sceneObject *scene = (sceneObject *)object->pParent;
    Mnode *root;
    int child;

    gm_col2 = outer_color;
    gm_p0 = (physicsObject *)scene->pPhysics;
    gm_col1 = inner_color;
    gm_radius1 = width;
    gm_radius2 = height;
    root = coll_GetNode(gm_p0->physicsRoot.objectID, 0);
    if (root != NULL &&
        (root->flags & UINT32_C(4)) == 0 &&
        root->pGeomData != NULL &&
        root->pGeomData->numFaces != 0) {
        for (child = 0; child < root->numChildNodes; ++child) {
            traverseModel(&root->aChildNode[child], root);
        }
    }
}

/* 0xA2DD0, 331 bytes, global, 6 named locals
 * fx_Init
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
void fx_Init(void)
{
    const char *path;

    if (translucent_glowtexture == NULL) {
        menu_addTotal(100);
        path = resource_getPath(
            "a_glow.tga", JPB_RESOURCE_DEFAULT);
        translucent_glowtexture = _LoadTexture(
            (char *)(void *)path, TT_SPRITE, 1);
        additive_glowtexture = _LoadTexture(
            (char *)(void *)path, TT_SPRITE, 2);

        path = resource_getPath(
            "a_water.tga", JPB_RESOURCE_DEFAULT);
        translucent_watertexture = _LoadTexture(
            (char *)(void *)path, TT_SPRITE, 1);
        menu_addTotal(100);
        path = resource_getPath(
            "a_water.tga", JPB_RESOURCE_DEFAULT);
        opaque_watertexture = _LoadTexture(
            (char *)(void *)path, TT_SPRITE, 0);
    }
    if (particlematerial == NULL) {
        path = resource_getPath(
            "a_blob.tga", JPB_RESOURCE_DEFAULT);
        particlematerial = _LoadTexture(
            (char *)(void *)path, TT_SPRITE, 2);
    }

    while (globalparticlelist != NULL) {
        _particle_list *list = globalparticlelist;
        _particle_8 *particle = list->plist;

        globalparticlelist = list->next;
        while (particle != NULL) {
            _particle_8 *next = particle->next;

            free(particle);
            particle = next;
        }
        free(list);
    }
}

int fx_DefaultTexturesReady(void)
{
    return translucent_glowtexture != NULL &&
        translucent_glowtexture->texture != NULL &&
        additive_glowtexture != NULL &&
        additive_glowtexture->texture != NULL &&
        translucent_watertexture != NULL &&
        translucent_watertexture->texture != NULL &&
        opaque_watertexture != NULL &&
        opaque_watertexture->texture != NULL &&
        particlematerial != NULL &&
        particlematerial->texture != NULL;
}

/* 0xA2F20, 1316 bytes, global, 23 named locals
 * fx_PlasmaZap
 * PDB type: void (_plasma_zapvars*, VECTOR*,...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */

/* 0xA3450, 2007 bytes, global, 16 named locals
 * fx_Water
 * PDB type: void (VECTOR*, int, int, unsigne...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
static float fx_water_uv_offset(
    int32_t x,
    int32_t z,
    float x_multiplier,
    float z_multiplier,
    float speed,
    float factor)
{
    const float pi = 3.1415927f;
    float coordinate =
        (float)x * x_multiplier +
        (float)z * z_multiplier;
    float phase = (coordinate * speed) / pi;
    float first_sine = (float)sin((double)phase);
    float second_phase =
        (first_sine * 16384.0f) / pi;

    return (float)sin((double)second_phase) * factor;
}

static int fx_water_transform_vertex(
    const VECTOR *source, FVECTOR *destination)
{
    float x = (float)source->vx;
    float y = (float)source->vy;
    float z = (float)source->vz;

    destination->vz =
        x * CameraMatrix.m[2][0] +
        y * CameraMatrix.m[2][1] +
        z * CameraMatrix.m[2][2] +
        (float)CameraMatrix.t[2];
    if (destination->vz < 1.0f) {
        return 0;
    }
    destination->vx =
        x * CameraMatrix.m[0][0] +
        y * CameraMatrix.m[0][1] +
        z * CameraMatrix.m[0][2] +
        (float)CameraMatrix.t[0];
    destination->vy =
        x * CameraMatrix.m[1][0] +
        y * CameraMatrix.m[1][1] +
        z * CameraMatrix.m[1][2] +
        (float)CameraMatrix.t[1];
    return 1;
}

void fx_Water(
    VECTOR *pos,
    int width,
    int height,
    uint32_t color,
    float factor,
    int speed)
{
    const float timer_divisor = 5120000.0f;
    float wave_factor = factor * 0.05f;
    float water_speed =
        (((float)(uint64_t)gGlobalTimer / timer_divisor) *
         (float)speed) /
        timer_divisor;
    int row;

    for (row = 0; row < height; ++row) {
        int32_t z = pos->vz + row * 0x100;
        int column;

        for (column = 0; column < width; ++column) {
            int32_t x = pos->vx + column * 0x100;
            int32_t next_x = x + 0x100;
            int32_t next_z = z + 0x100;
            VECTOR world[4] = {
                {x, pos->vy, next_z, pos->pad},
                {next_x, pos->vy, next_z, pos->pad},
                {x, pos->vy, z, pos->pad},
                {next_x, pos->vy, z, pos->pad}
            };
            FVECTOR transformed[4];
            float u[4];
            float v[4];
            int vertex;

            for (vertex = 0; vertex < 4; ++vertex) {
                if (!fx_water_transform_vertex(
                        &world[vertex],
                        &transformed[vertex])) {
                    break;
                }
            }
            if (vertex != 4) {
                continue;
            }

            u[0] = fx_water_uv_offset(
                x, next_z, 13.0f, 16.0f,
                water_speed, wave_factor);
            v[0] = fx_water_uv_offset(
                x, next_z, 14.0f, 15.0f,
                water_speed, wave_factor) + 1.0f;
            u[1] = fx_water_uv_offset(
                next_x, next_z, 13.0f, 16.0f,
                water_speed, wave_factor) + 1.0f;
            v[1] = fx_water_uv_offset(
                next_x, next_z, 14.0f, 15.0f,
                water_speed, wave_factor) + 1.0f;
            u[2] = fx_water_uv_offset(
                x, z, 13.0f, 16.0f,
                water_speed, wave_factor);
            v[2] = fx_water_uv_offset(
                x, z, 14.0f, 15.0f,
                water_speed, wave_factor);
            u[3] = fx_water_uv_offset(
                next_x, z, 13.0f, 16.0f,
                water_speed, wave_factor) + 1.0f;
            v[3] = fx_water_uv_offset(
                next_x, z, 14.0f, 15.0f,
                water_speed, wave_factor);

            _StartPoly(4, opaque_watertexture);
            for (vertex = 0; vertex < 4; ++vertex) {
                _SetVert(
                    vertex,
                    transformed[vertex].vx,
                    transformed[vertex].vy,
                    transformed[vertex].vz,
                    color,
                    u[vertex],
                    v[vertex]);
            }
            _NoScaleEndPoly();
        }
    }
}

/* 0xA3C30, 3 bytes, global, 2 named locals
 * fx_ZappingMan
 * PDB type: void (objectRoot*, unsigned long...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */

/* 0xA3C40, 1114 bytes, global, 13 named locals
 * fx_screenGlow
 * PDB type: void (_svector*, _svector*, int,...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
void fx_ZappingMan(objectRoot *object, uint32_t color)
{
    (void)object;
    (void)color;
}
void fx_PlasmaZap(
    _plasma_zapvars *pzv,
    VECTOR *start,
    VECTOR *end,
    uint32_t color1,
    uint32_t color2,
    int radius)
{
    VECTOR traverse;
    VECTOR diff;
    _svector s;
    _svector e;
    int xsin;
    int ysin;
    int zsin;
    int xsv;
    int ysv;
    int zsv;
    int i;

    s.vx = (int16_t)start->vx;
    s.vy = (int16_t)start->vy;
    s.vz = (int16_t)start->vz;
    s.pad = 0;
    if (pzv->inited == 0) {
        for (i = 0; i < 8; ++i) {
            int high = rand();
            int low = rand();

            pzv->blobz[i] = low | (high << 9);
            high = rand();
            low = rand();
            pzv->blobv[i] = low | (high << 9);
        }
        pzv->inited = 1;
        pzv->sinusx = 0;
        pzv->sinusy = 0;
        pzv->sinusz = 0;
        pzv->sinusxvel = (rand() >> 7) - 0x80;
        pzv->sinusyvel = (rand() >> 7) - 0x80;
        pzv->sinuszvel = (rand() >> 7) - 0x80;
    }

    xsin = pzv->sinusx;
    pzv->sinusx += pzv->sinusxvel;
    pzv->sinusy += pzv->sinusyvel;
    pzv->sinusz += pzv->sinuszvel;

    traverse = *start;
    diff.vx = ((end->vx - start->vx) + 9) / 10;
    diff.vy = ((end->vy - start->vy) + 9) / 10;
    diff.vz = ((end->vz - start->vz) + 9) / 10;
    diff.pad = 0;
    ysin = xsin;
    zsin = xsin;
    xsv = flexmul12(pzv->sinusyvel, gGlobalFrameRate);
    ysv = flexmul12(pzv->sinuszvel, gGlobalFrameRate);
    zsv = flexmul12(pzv->sinusxvel, gGlobalFrameRate);

    for (i = 0; i < 8; ++i) {
        int blob = (int8_t)(uint8_t)pzv->blobz[i] +
            flexmul12(
                ((int8_t)(uint8_t)pzv->blobv[i]) >> 3,
                gGlobalFrameRate);
        int blov = (int8_t)(uint8_t)(pzv->blobz[i] >> 8) +
            flexmul12(
                ((int8_t)(uint8_t)(pzv->blobv[i] >> 8)) >> 3,
                gGlobalFrameRate);
        int mag = (int8_t)(uint8_t)(pzv->blobz[i] >> 16) +
            flexmul12(
                ((int8_t)(uint8_t)(pzv->blobv[i] >> 16)) >> 3,
                gGlobalFrameRate);

        pzv->blobz[i] =
            ((mag & 0xff) << 16) |
            ((blov & 0xff) << 8) |
            (blob & 0xff);
        traverse.vx += diff.vx;
        traverse.vy += diff.vy;
        traverse.vz += diff.vz;

        e.vx = (int16_t)(
            traverse.vx +
            (rsin(xsin) * flexmul(rsin(i << 7), radius) >> 17) +
            (rsin(blob * 8) >> 7));
        e.vy = (int16_t)(
            traverse.vy +
            (rsin(ysin) * flexmul(rsin(i << 7), radius) >> 17) +
            (rsin(blov * 8) >> 7));
        e.vz = (int16_t)(
            traverse.vz +
            (rsin(zsin) * flexmul(rsin(i << 7), radius) >> 17) +
            (rsin(mag * 8) >> 7));
        e.pad = 0;

        SetCameraMatrix();
        fx_screenGlow(&s, &e, 0x30, color2);
        s = e;
        ysin += ysv;
        zsin += zsv;
        xsin += xsv;
    }

    if (rand() < 0x800) {
        pzv->inited = 0;
    }
    e.vx = (int16_t)end->vx;
    e.vy = (int16_t)end->vy;
    e.vz = (int16_t)end->vz;
    e.pad = 0;
    fx_screenGlow(&s, &e, 0x38, color2);
    (void)color1;
}
void fx_screenGlow(
    _svector *start,
    _svector *end,
    int width,
    uint32_t color)
{
    static const uint32_t quad_topology[6] = {
        UINT32_C(0x54103210),
        UINT32_C(0x98541032),
        UINT32_C(0x65213311),
        UINT32_C(0xa9651133),
        UINT32_C(0x76322301),
        UINT32_C(0xba760123)
    };
    static const float texture_coordinates[4][2] = {
        {0.01f, 0.01f},
        {0.99f, 0.01f},
        {0.01f, 0.99f},
        {0.99f, 0.99f}
    };
    const double depth_bias = 0.0000016;
    FVECTOR transformed_start;
    FVECTOR transformed_end;
    FVECTOR vertices[12];
    float x_difference;
    float y_difference;
    float inverse_length = 1.0f;
    float x_offset;
    float y_offset;
    double length_squared;
    int quad;

    if (jpb_screen_glow_hook != NULL) {
        jpb_screen_glow_hook(
            jpb_screen_glow_user_data,
            start,
            end,
            width,
            color);
    }

    PerspectiveTransform(&CameraMatrix, start, &transformed_start);
    PerspectiveTransform(&CameraMatrix, end, &transformed_end);
    x_difference = transformed_end.vx - transformed_start.vx;
    y_difference = transformed_end.vy - transformed_start.vy;
    length_squared =
        (double)y_difference * (double)y_difference +
        (double)x_difference * (double)x_difference;
    if (length_squared != 0.0) {
        inverse_length = 1.0f / (float)sqrt(length_squared);
    }
    x_offset = y_difference * inverse_length * (float)width;
    y_offset = x_difference * inverse_length * (float)width;
    transformed_start.vz =
        (float)((double)transformed_start.vz - depth_bias);
    transformed_end.vz =
        (float)((double)transformed_end.vz - depth_bias);

    vertices[0] = (FVECTOR){
        transformed_start.vx - x_offset - y_offset,
        transformed_start.vy + y_offset - x_offset,
        transformed_start.vz};
    vertices[1] = (FVECTOR){
        transformed_start.vx - x_offset,
        transformed_start.vy + y_offset,
        transformed_start.vz};
    vertices[2] = (FVECTOR){
        transformed_end.vx - x_offset,
        transformed_end.vy + y_offset,
        transformed_end.vz};
    vertices[3] = (FVECTOR){
        transformed_end.vx + y_offset - x_offset,
        transformed_end.vy + x_offset + y_offset,
        transformed_end.vz};
    vertices[4] = (FVECTOR){
        transformed_start.vx - y_offset,
        transformed_start.vy - x_offset,
        transformed_start.vz};
    vertices[5] = transformed_start;
    vertices[6] = transformed_end;
    vertices[7] = (FVECTOR){
        transformed_end.vx + y_offset,
        transformed_end.vy + x_offset,
        transformed_end.vz};
    vertices[8] = (FVECTOR){
        transformed_start.vx + x_offset - y_offset,
        transformed_start.vy - x_offset - y_offset,
        transformed_start.vz};
    vertices[9] = (FVECTOR){
        transformed_start.vx + x_offset,
        transformed_start.vy - y_offset,
        transformed_start.vz};
    vertices[10] = (FVECTOR){
        transformed_end.vx + x_offset,
        transformed_end.vy - y_offset,
        transformed_end.vz};
    vertices[11] = (FVECTOR){
        transformed_end.vx + x_offset + y_offset,
        transformed_end.vy + x_offset - y_offset,
        transformed_end.vz};

    for (quad = 0; quad < 6; ++quad) {
        uint32_t packed = quad_topology[quad];
        int vertex;

        _StartPoly(4, additive_glowtexture);
        for (vertex = 0; vertex < 4; ++vertex) {
            unsigned texture_index =
                (packed >> (vertex * 4)) & UINT32_C(0x0f);
            unsigned position_index =
                (packed >> (16 + vertex * 4)) & UINT32_C(0x0f);
            const FVECTOR *position = &vertices[position_index];

            _SetVert(
                vertex,
                position->vx,
                position->vy,
                position->vz,
                color,
                texture_coordinates[texture_index][0],
                texture_coordinates[texture_index][1]);
        }
        _NoScaleEndPoly();
    }
}

/* 0xA40A0, 82 bytes, global, 6 named locals
 * fx_screenGlowFV
 * PDB type: void (FVECTOR*, FVECTOR*, int, u...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
void fx_screenGlowFV(
    FVECTOR *start,
    FVECTOR *end,
    int width,
    uint32_t color)
{
    _svector short_start = {
        (int16_t)(int)start->vx,
        (int16_t)(int)start->vy,
        (int16_t)(int)start->vz,
        0};
    _svector short_end = {
        (int16_t)(int)end->vx,
        (int16_t)(int)end->vy,
        (int16_t)(int)end->vz,
        0};

    fx_screenGlow(&short_start, &short_end, width, color);
}

/* 0xA4100, 946 bytes, global, 12 named locals
 * fx_screenSection
 * PDB type: void (_svector*, _svector*, int,...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
void fx_screenSection(
    _svector *start,
    _svector *end,
    int width,
    uint32_t color)
{
    const double depth_bias = 0.0000016;
    FVECTOR transformed_start;
    FVECTOR transformed_end;
    float dx;
    float dy;
    float inverse_length = 1.0f;
    float x_offset;
    float y_offset;
    double length_squared;

    PerspectiveTransform(
        &CameraMatrix, start, &transformed_start);
    PerspectiveTransform(
        &CameraMatrix, end, &transformed_end);
    if (glowtexture == NULL) {
        glowtexture = _LoadTexture(NULL, TT_SPRITE, 2);
    }
    glowtexture->flags = JPB_MATERIAL_MODE_TWO_SIDED;

    dx = transformed_end.vx - transformed_start.vx;
    dy = transformed_end.vy - transformed_start.vy;
    length_squared =
        (double)dy * (double)dy +
        (double)dx * (double)dx;
    if (length_squared != 0.0) {
        inverse_length = 1.0f / (float)sqrt(length_squared);
    }
    x_offset = dy * inverse_length * (float)width;
    y_offset = dx * inverse_length * (float)width;
    transformed_start.vz =
        (float)((double)transformed_start.vz - depth_bias);
    transformed_end.vz =
        (float)((double)transformed_end.vz - depth_bias);

    _StartPoly(4, glowtexture);
    _SetVert(
        0,
        transformed_start.vx - x_offset,
        transformed_start.vy + y_offset,
        transformed_start.vz,
        0, 0.0f, 0.0f);
    _SetVert(
        1,
        transformed_end.vx - x_offset,
        transformed_end.vy + y_offset,
        transformed_end.vz,
        0, 0.0f, 0.0f);
    _SetVert(
        2,
        transformed_start.vx,
        transformed_start.vy,
        transformed_start.vz,
        color, 0.0f, 0.0f);
    _SetVert(
        3,
        transformed_end.vx,
        transformed_end.vy,
        transformed_end.vz,
        color, 0.0f, 0.0f);
    _NoScaleEndPoly();

    _StartPoly(4, glowtexture);
    _SetVert(
        0,
        transformed_start.vx,
        transformed_start.vy,
        transformed_start.vz,
        color, 0.0f, 0.0f);
    _SetVert(
        1,
        transformed_end.vx,
        transformed_end.vy,
        transformed_end.vz,
        color, 0.0f, 0.0f);
    _SetVert(
        2,
        transformed_start.vx + x_offset,
        transformed_start.vy - y_offset,
        transformed_start.vz,
        0, 0.0f, 0.0f);
    _SetVert(
        3,
        transformed_end.vx + x_offset,
        transformed_end.vy - y_offset,
        transformed_end.vz,
        0, 0.0f, 0.0f);
    _NoScaleEndPoly();
}

/* 0xA44C0, 210 bytes, local, 8 named locals
 * getrandomcolor
 * PDB type: unsigned long (unsigned long, un...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
static uint32_t getrandomcolor(
    uint32_t base, uint32_t random_range)
{
    uint32_t random_red = (seed + UINT32_C(0x6347)) *
        UINT32_C(0x731);
    uint32_t random_green =
        (random_red + UINT32_C(0x6347)) *
        UINT32_C(0x731);
    uint32_t red = ((base >> 16) & UINT32_C(0xff)) +
        (random_red & UINT32_C(0x7fff)) %
            ((random_range >> 16) & UINT32_C(0xff));
    uint32_t green;
    uint32_t blue;

    seed = (random_green + UINT32_C(0x6347)) *
        UINT32_C(0x731);
    green = ((base >> 8) & UINT32_C(0xff)) +
        (random_green & UINT32_C(0x7fff)) %
            ((random_range >> 8) & UINT32_C(0xff));
    blue = (base & UINT32_C(0xff)) +
        (seed & UINT32_C(0x7fff)) %
            (random_range & UINT32_C(0xff));
    if ((int32_t)red < 0) {
        red = 0;
    } else if (red > UINT32_C(0xff)) {
        red = UINT32_C(0xff);
    }
    if ((int32_t)green < 0) {
        green = 0;
    } else if (green > UINT32_C(0xff)) {
        green = UINT32_C(0xff);
    }
    if (blue > UINT32_C(0xff)) {
        blue = UINT32_C(0xff);
    }
    return (red << 16) | (green << 8) | blue;
}

/* 0xA45A0, 107 bytes, global, 4 named locals
 * particle_CleanUp
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
void particle_CleanUp(void)
{
    while (globalparticlelist != NULL) {
        _particle_list *list = globalparticlelist;
        _particle_8 *particle = list->plist;

        globalparticlelist = list->next;
        while (particle != NULL) {
            _particle_8 *next = particle->next;

            free(particle);
            particle = next;
        }
        free(list);
    }
}

/* 0xA4610, 155 bytes, global, 5 named locals
 * particle_Init
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
void particle_Init(void)
{
    if (particlematerial == NULL) {
        const char *path = resource_getPath(
            "a_blob.tga", JPB_RESOURCE_DEFAULT);

        particlematerial = _LoadTexture(
            (char *)(void *)path, TT_SPRITE, 2);
    }
    particle_CleanUp();
}

/* 0xA46B0, 858 bytes, global, 14 named locals
 * particle_Launch
 * PDB type: void (_particle_launcher*, FVECT...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
void particle_Launch(
    _particle_launcher *launcher,
    FVECTOR *origin,
    float groundplane)
{
    const float pi = 3.1415927f;
    const float reciprocal_random = 1.0f / 32767.0f;
    const float angle_scale = 32768.0f;
    _particle_list *list =
        (_particle_list *)malloc(sizeof(*list));
    int remaining;

    (void)groundplane;
    if (list == NULL) {
        return;
    }
    list->next = globalparticlelist;
    globalparticlelist = list;
    remaining = launcher->number;
    while (remaining > 0) {
        _particle_8 *group =
            (_particle_8 *)malloc(sizeof(*group));
        int count;
        int particle;
        float spread;

        if (group == NULL) {
            return;
        }
        group->next = list->plist;
        list->plist = group;
        count = remaining < 8 ? remaining : 8;
        spread = launcher->angle * pi / 180.0f;
        for (particle = 0; particle < count; ++particle) {
            _particle *output = &group->p[particle];
            uint32_t random_pitch =
                (seed + UINT32_C(0x6347)) * UINT32_C(0x731);
            uint32_t random_yaw =
                (random_pitch + UINT32_C(0x6347)) *
                UINT32_C(0x731);
            float pitch_random =
                (float)(random_pitch & UINT32_C(0x7fff)) *
                reciprocal_random;
            float yaw_random =
                (float)(random_yaw & UINT32_C(0x7fff)) *
                reciprocal_random;
            float pitch =
                pitch_random * spread + launcher->pitch;
            float yaw =
                (1.0f - pitch_random * pitch_random) *
                    yaw_random * spread +
                launcher->yaw;
            double pitch_angle =
                (double)((float)(int)(pitch * angle_scale) *
                         pi / angle_scale);
            float pitch_sine = (float)sin(pitch_angle);
            uint32_t random_velocity =
                (random_yaw + UINT32_C(0x6347)) *
                UINT32_C(0x731);
            int velocity =
                (int)(random_velocity & UINT32_C(0x7fff)) %
                    launcher->velocityrand +
                launcher->velocity;
            float x_velocity =
                -(float)cos(pitch_angle) * (float)velocity;
            double yaw_angle =
                (double)((float)(int)(yaw * angle_scale) *
                         pi / angle_scale);
            float yaw_sine = (float)sin(yaw_angle);
            float yaw_cosine = (float)cos(yaw_angle);

            output->vel.vx = x_velocity;
            output->vel.vy =
                yaw_sine * pitch_sine * (float)velocity;
            output->vel.vz =
                yaw_cosine * pitch_sine * (float)velocity;
            output->org = *origin;
            seed =
                (random_velocity + UINT32_C(0x6347)) *
                UINT32_C(0x731);
            output->life =
                (int)(seed & UINT32_C(0x7fff)) %
                    launcher->lifeoffsetrand +
                launcher->lifeoffset;
            output->color1 = getrandomcolor(
                launcher->startcolor1,
                launcher->startcolorrand1);
            output->color2 = getrandomcolor(
                launcher->startcolor2,
                launcher->startcolorrand2);
            output->decay =
                launcher->decayrate +
                rand() % launcher->decayrand;
        }
        remaining -= 8;
    }
}

/* 0xA4A10, 242 bytes, global, 7 named locals
 * particle_Update
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
static uint32_t particle_interpolate_color(
    uint32_t target,
    uint32_t source,
    uint32_t scalar)
{
    uint32_t target_alpha = target >> 24;
    uint32_t target_red = (target >> 16) & UINT32_C(0xff);
    uint32_t target_green = (target >> 8) & UINT32_C(0xff);
    uint32_t target_blue = target & UINT32_C(0xff);
    uint32_t source_alpha = source >> 24;
    uint32_t source_red = (source >> 16) & UINT32_C(0xff);
    uint32_t source_green = (source >> 8) & UINT32_C(0xff);
    uint32_t source_blue = source & UINT32_C(0xff);
    uint32_t alpha =
        ((target_alpha - source_alpha) * scalar +
         source_alpha) >> 12;
    uint32_t red =
        ((target_red - source_red) * scalar +
         source_red) >> 12;
    uint32_t green =
        ((target_green - source_green) * scalar +
         source_green) >> 12;
    uint32_t blue =
        ((target_blue - source_blue) * scalar +
         source_blue) >> 12;

    return (alpha << 24) |
        ((red & UINT32_C(0xff)) << 16) |
        ((green & UINT32_C(0xff)) << 8) |
        (blue & UINT32_C(0xff));
}

static int particle_process8(
    _particle_8 *group,
    _particle_list *list)
{
    _particle_launcher *launcher = list->launcher;
    FVECTOR positions[16];
    _particle *active[8];
    int tail = (int)launcher->tail;
    int scale = (int)launcher->scale;
    int live = 8;
    int active_count = 0;
    int position_count = 0;
    int index;

    for (index = 0; index < 8; ++index) {
        _particle *particle = &group->p[index];
        int lifetime;

        particle->life += particle->decay;
        lifetime = particle->life;
        if (lifetime > 0) {
            if (lifetime < 0x8000) {
                float lifetime1 = (float)(lifetime - tail);
                float lifetime2 = lifetime1 * lifetime1;
                float lifetime3 = (float)lifetime;
                float lifetime4 = lifetime3 * lifetime3;

                active[active_count++] = particle;
                positions[position_count].vx =
                    particle->org.vx +
                    particle->vel.vx * lifetime1 +
                    list->accel.vx * lifetime2;
                positions[position_count].vy =
                    particle->org.vy +
                    particle->vel.vy * lifetime1 +
                    list->accel.vy * lifetime2;
                positions[position_count].vz =
                    particle->org.vz +
                    particle->vel.vz * lifetime1 +
                    list->accel.vz * lifetime2;
                ++position_count;
                positions[position_count].vx =
                    particle->org.vx +
                    particle->vel.vx * lifetime3 +
                    list->accel.vx * lifetime4;
                positions[position_count].vy =
                    particle->org.vy +
                    particle->vel.vy * lifetime3 +
                    list->accel.vy * lifetime4;
                positions[position_count].vz =
                    particle->org.vz +
                    particle->vel.vz * lifetime3 +
                    list->accel.vz * lifetime4;
                ++position_count;
            } else {
                --live;
            }
        }
    }
    if (active_count != 0) {
        RotTransPersManyFV(
            positions, position_count, positions);
        for (index = 0; index < active_count; ++index) {
            _particle *particle = active[index];
            FVECTOR *start = &positions[index * 2];
            FVECTOR *end = &positions[index * 2 + 1];
            uint32_t scalar = (uint32_t)(particle->life >> 4);
            uint32_t color1 = particle_interpolate_color(
                launcher->endcolor1,
                particle->color1,
                scalar);
            uint32_t color2 = particle_interpolate_color(
                launcher->endcolor2,
                particle->color2,
                scalar);
            float xd = end->vx - start->vx;
            float yd = end->vy - start->vy;
            float length = (float)sqrt(
                (double)(xd * xd + yd * yd));
            float x2 = end->vx;
            float y2 = end->vy;
            float perpendicular_scale;
            float x_offset;
            float y_offset;

            if (length < (float)scale) {
                if (length == 0.0f) {
                    yd = 0.0f;
                    length = 1.0f;
                    x2 = start->vx + (float)scale;
                    y2 = start->vy;
                    xd = 1.0f;
                } else {
                    float extension = (float)scale / length;

                    x2 = start->vx + xd * extension;
                    y2 = start->vy + yd * extension;
                }
            }
            perpendicular_scale = (1.0f / length) * (float)scale;
            x_offset = xd * perpendicular_scale;
            y_offset = yd * perpendicular_scale;

            _StartPoly(4, particlematerial);
            _SetVert(
                0,
                start->vx,
                start->vy,
                start->vz,
                color2,
                0.01f,
                0.01f);
            _SetVert(
                1,
                x2 - x_offset,
                y2 + x_offset,
                end->vz,
                color1,
                0.99f,
                0.01f);
            _SetVert(
                2,
                x2 + y_offset,
                y2 - x_offset,
                end->vz,
                color1,
                0.01f,
                0.99f);
            _SetVert(
                3,
                x2 + x_offset,
                y2 + y_offset,
                end->vz,
                color1,
                0.99f,
                0.99f);
            _NoScaleEndPoly();
        }
    }
    return live;
}

void particle_Update(void)
{
    _particle_list *list = globalparticlelist;
    _particle_list *previous_list = NULL;

    SetupTransformMatrix(&CameraMatrix);
    while (list != NULL) {
        _particle_list *next_list = list->next;
        _particle_8 *group = list->plist;
        _particle_8 *previous_group = NULL;
        int remaining_groups = 8;

        while (group != NULL) {
            _particle_8 *next_group = group->next;

            if (particle_process8(group, list) == 0) {
                if (previous_group == NULL) {
                    list->plist = next_group;
                } else {
                    previous_group->next = next_group;
                }
                free(group);
                --remaining_groups;
            } else {
                previous_group = group;
            }
            group = next_group;
        }
        if (remaining_groups == 0) {
            if (previous_list == NULL) {
                globalparticlelist = next_list;
            } else {
                previous_list->next = next_list;
            }
            free(list);
        } else {
            previous_list = list;
        }
        list = next_list;
    }
}

/* 0xA4B10, 2411 bytes, local, 49 named locals
 * particle_process8
 * PDB type: int (_particle_8*, _particle_lis...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */

/* 0xA5480, 399 bytes, global, 7 named locals
 * traverseModel
 * PDB type: void (Mnode*, Mnode*)
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
static int16_t fx_scale_leaf_component(
    int16_t component, int scale)
{
    int32_t product = (int32_t)component * scale;
    int32_t rounding =
        (int32_t)((uint32_t)(product >> 31) & UINT32_C(0xfff));

    return (int16_t)((product + rounding) >> 12);
}

void traverseModel(Mnode *node, Mnode *parent)
{
    int child;

    if ((node->flags & UINT32_C(4)) != 0 ||
        node->pGeomData == NULL ||
        node->pGeomData->numFaces == 0) {
        return;
    }
    if (parent != NULL) {
        _svector start = {
            (int16_t)node->v3RotCenter.vx,
            (int16_t)node->v3RotCenter.vy,
            (int16_t)node->v3RotCenter.vz,
            0};
        _svector end = {
            (int16_t)parent->v3RotCenter.vx,
            (int16_t)parent->v3RotCenter.vy,
            (int16_t)parent->v3RotCenter.vz,
            0};
        int radius;
        uint32_t color;

        if (node->numChildNodes == 0) {
            _svector direction;
            int length;

            if (((uint32_t)node->id & UINT32_C(0xa000)) != 0) {
                goto process_children;
            }
            direction.vx = (int16_t)(start.vx - end.vx);
            direction.vy = (int16_t)(start.vy - end.vy);
            direction.vz = (int16_t)(start.vz - end.vz);
            direction.pad = 0;
            length = normalize(
                direction.vx,
                direction.vy,
                direction.vz,
                &direction) + 8;
            start.vx = (int16_t)(
                end.vx +
                fx_scale_leaf_component(direction.vx, length));
            start.vy = (int16_t)(
                end.vy +
                fx_scale_leaf_component(direction.vy, length));
            start.vz = (int16_t)(
                end.vz +
                fx_scale_leaf_component(direction.vz, length));
            radius = gm_radius2;
            color = gm_col2;
        } else {
            radius = gm_radius1;
            color = gm_col1;
            if (((uint32_t)node->id & UINT32_C(0xa000)) != 0) {
                goto process_children;
            }
        }
        fx_screenGlow(&end, &start, radius, color);
    }

process_children:
    for (child = 0; child < node->numChildNodes; ++child) {
        traverseModel(&node->aChildNode[child], node);
    }
}

/* 0xA5610, 3 bytes, global, 2 named locals
 * traverseModel2
 * PDB type: void (Mnode*, Mnode*)
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
void traverseModel2(Mnode *node, Mnode *parent)
{
    (void)node;
    (void)parent;
}
