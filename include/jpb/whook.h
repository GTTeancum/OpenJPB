#ifndef JPB_WHOOK_H
#define JPB_WHOOK_H

#include "jpb/fmath.h"
#include "jpb/material.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact matched-PC PDB type 0x69A2. */
typedef struct SCREENRECT {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} SCREENRECT;

/*
 * Portable realization seam for the original platform renderer's textured
 * screen rectangle. Gameplay retains exact _DrawTexture; each platform owns
 * how the queued rectangle is presented.
 */
typedef void (*JPBDrawTextureHook)(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth);

/*
 * Dependency-light realization seam for exact PDB procedure
 * debug_drawsphere. The matched release body performs no visible draw, so
 * PC and later platform renderers may consume the authored debug primitive
 * without coupling gameplay to a graphics API.
 */
typedef void (*JPBDebugSphereHook)(
    void *user_data,
    int32_t x,
    int32_t y,
    int32_t z,
    int32_t radius,
    uint32_t color);

/*
 * Exact _SetVert payload retained at the platform boundary. The original
 * renderer builder owns the same x/y/z, diffuse-color, and UV fields; this
 * fixed record lets dependency-light renderers consume completed polygons.
 */
typedef struct JPBScreenPolyVertex {
    float x;
    float y;
    float z;
    uint32_t argb;
    float tu;
    float tv;
} JPBScreenPolyVertex;

enum { JPB_SCREEN_POLY_VERTEX_CAPACITY = 32 };

/*
 * Portable mirror of the two constant-buffer publications made by exact
 * PDB procedure _ApplyLevelTransformation.
 */
typedef struct JPBLevelTransformation {
    float world[4][4];
    float scale[4];
} JPBLevelTransformation;

typedef void (*JPBScreenPolyHook)(
    void *user_data,
    _Material *material,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale);

void jpb_WHookSetDrawTextureHook(
    JPBDrawTextureHook hook, void *user_data);
void jpb_WHookSetDebugSphereHook(
    JPBDebugSphereHook hook, void *user_data);
void jpb_WHookSetScreenPolyHook(
    JPBScreenPolyHook hook, void *user_data);
extern int refreshFontAtlasFlag;
extern _Material *whitemat;
void MarkFontAtlasForRefresh(void);
const JPBLevelTransformation *jpb_WHookLevelTransformation(void);
void _ApplyLevelTransformation(
    MATRIX *matrix, float x_scale, float y_scale, float z_scale);
void _DrawTexture(
    _Material *texture,
    SCREENRECT destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth);
void debug_drawsphere(
    int x, int y, int z, int radius, uint32_t color);
void _EndPoly(void);
void _NoScaleEndPoly(void);
void _SetVert(
    int vertex,
    float x,
    float y,
    float z,
    unsigned long argb,
    float tu,
    float tv);
void _StartPoly(int vertex_count, _Material *material);
int cliptoscreen(short *position);

#if defined(__cplusplus)
#define JPB_WHOOK_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_WHOOK_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_WHOOK_STATIC_ASSERT(
    sizeof(SCREENRECT) == 16,
    "SCREENRECT must match PDB type 0x69A2");
JPB_WHOOK_STATIC_ASSERT(
    offsetof(SCREENRECT, bottom) == 12,
    "SCREENRECT.bottom layout changed");
JPB_WHOOK_STATIC_ASSERT(
    sizeof(JPBScreenPolyVertex) == 24,
    "JPBScreenPolyVertex layout changed");

#undef JPB_WHOOK_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
