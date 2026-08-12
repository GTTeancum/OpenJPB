/*
 * PARTIAL RECONSTRUCTION. PlotZap, fx_GlowingMan, fx_PlasmaZap, and the
 * dependency-light fx_screenGlow realization are reviewed; the remaining
 * emitted procedures retain explicit inventory markers below.
 * PDB module: 0038
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\fx.obj
 * Primary source: W:\SWJediPowerBattles\work\fx.c
 * Compiler language: c
 * Emitted procedures: 17
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/fx.h"

#include "jpb/camera.h"
#include "jpb/flex.h"
#include "jpb/game.h"
#include "jpb/menu.h"
#include "jpb/resources.h"
#include "jpb/scene.h"
#include "jpb/texture.h"
#include "jpb/whook.h"

#include <math.h>
#include <stdlib.h>

static JPBFxScreenGlowHook jpb_screen_glow_hook;
static void *jpb_screen_glow_user_data;
static JPBFxGlowingManHook jpb_glowing_man_hook;
static void *jpb_glowing_man_user_data;

/* Exact fx.c module-local material owners at RVAs 0x537D88..0x537DA0. */
static _Material *translucent_glowtexture;
static _Material *additive_glowtexture;
static _Material *translucent_watertexture;
static _Material *opaque_watertexture;
static _Material *particlematerial;

/*
 * Cleanup-only views of the two still-pending particle records. The pointer
 * links are exact matched-x64 offsets observed in fx_Init: particle next at
 * 0x158 and list next at 0x20. No live particle is created by the currently
 * reconstructed subset, but retaining this ownership prevents future list
 * work from acquiring a second cleanup path.
 */
typedef struct FxParticleCleanupView {
    unsigned char beforeNext[0x158];
    struct FxParticleCleanupView *next;
} FxParticleCleanupView;

typedef struct FxParticleListCleanupView {
    FxParticleCleanupView *particles;
    unsigned char beforeNext[0x18];
    struct FxParticleListCleanupView *next;
} FxParticleListCleanupView;

static FxParticleListCleanupView *globalparticlelist;

void jpb_FxSetScreenGlowHook(
    JPBFxScreenGlowHook hook, void *user_data)
{
    jpb_screen_glow_hook = hook;
    jpb_screen_glow_user_data = user_data;
}

void jpb_FxSetGlowingManHook(
    JPBFxGlowingManHook hook, void *user_data)
{
    jpb_glowing_man_hook = hook;
    jpb_glowing_man_user_data = user_data;
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
    /*
     * The original traverses the registered model hierarchy and emits glow
     * geometry for each child. The caller-visible behavior stays exact while
     * the dependency-free renderer owns that traversal's realization.
     */
    if (jpb_glowing_man_hook != NULL &&
        object != NULL) {
        jpb_glowing_man_hook(
            jpb_glowing_man_user_data,
            object,
            width,
            height,
            inner_color,
            outer_color);
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

    /*
     * The reference only tests the pointer. The texture-null half keeps the
     * portable host's supported shutdown/re-init cycle safe after its full
     * material-pool flush; it is indistinguishable during retail ownership.
     */
    if (translucent_glowtexture == NULL ||
        translucent_glowtexture->texture == NULL) {
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
    if (particlematerial == NULL ||
        particlematerial->texture == NULL) {
        path = resource_getPath(
            "a_blob.tga", JPB_RESOURCE_DEFAULT);
        particlematerial = _LoadTexture(
            (char *)(void *)path, TT_SPRITE, 2);
    }

    while (globalparticlelist != NULL) {
        FxParticleListCleanupView *list = globalparticlelist;
        FxParticleCleanupView *particle = list->particles;

        globalparticlelist = list->next;
        while (particle != NULL) {
            FxParticleCleanupView *next = particle->next;

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

    if (pzv == NULL || start == NULL || end == NULL) {
        return;
    }

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
    /*
     * The original procedure projects a twelve-vertex glow volume and emits
     * six immediate-mode quads. Keep its exact public call surface while the
     * dependency-free PC renderer owns realization of those draw commands.
     */
    if (jpb_screen_glow_hook != NULL &&
        start != NULL &&
        end != NULL) {
        jpb_screen_glow_hook(
            jpb_screen_glow_user_data,
            start,
            end,
            width,
            color);
    }
}

/* 0xA40A0, 82 bytes, global, 6 named locals
 * fx_screenGlowFV
 * PDB type: void (FVECTOR*, FVECTOR*, int, u...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */

/* 0xA4100, 946 bytes, global, 12 named locals
 * fx_screenSection
 * PDB type: void (_svector*, _svector*, int,...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */

/* 0xA44C0, 210 bytes, local, 8 named locals
 * getrandomcolor
 * PDB type: unsigned long (unsigned long, un...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */

/* 0xA45A0, 107 bytes, global, 4 named locals
 * particle_CleanUp
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\fx.c
 */

/* 0xA4610, 155 bytes, global, 5 named locals
 * particle_Init
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\fx.c
 */

/* 0xA46B0, 858 bytes, global, 14 named locals
 * particle_Launch
 * PDB type: void (_particle_launcher*, FVECT...
 * Source: W:\SWJediPowerBattles\work\fx.c
 */

/* 0xA4A10, 242 bytes, global, 7 named locals
 * particle_Update
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\fx.c
 */

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

/* 0xA5610, 3 bytes, global, 2 named locals
 * traverseModel2
 * PDB type: void (Mnode*, Mnode*)
 * Source: W:\SWJediPowerBattles\work\fx.c
 */
