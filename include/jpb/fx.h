#ifndef JPB_FX_H
#define JPB_FX_H

#include "jpb/fmath.h"
#include "jpb/objroot.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Optional observer for the reconstructed immediate-mode glow submission. */
typedef void (*JPBFxScreenGlowHook)(
    void *user_data,
    const _svector *start,
    const _svector *end,
    int width,
    uint32_t color);
typedef void (*JPBFxGlowingManHook)(
    void *user_data,
    objectRoot *object,
    int width,
    int height,
    uint32_t inner_color,
    uint32_t outer_color);

/* Exact matched-PC PDB type 0x142A. */
typedef struct _plasma_zapvars {
    int32_t inited;
    int32_t blobz[16];
    int32_t blobv[16];
    int32_t sinusx;
    int32_t sinusy;
    int32_t sinusz;
    int32_t sinusxvel;
    int32_t sinusyvel;
    int32_t sinuszvel;
} _plasma_zapvars;

enum { JPB_PLASMA_ZAP_CHANNELS = 8 };

/*
 * Exact storage at matched-PC RVA 0x4F15B0. The PDB has no public name for
 * this array, so the jpb_ prefix records that only its descriptive name is
 * inferred; its element type, address, capacity, and use are direct evidence.
 */
extern _plasma_zapvars
    jpb_PlasmaZapVars[JPB_PLASMA_ZAP_CHANNELS];
/* Exact PDB global `zpush` at matched-PC RVA 0x537D84. */
extern int32_t zpush;

void jpb_FxSetScreenGlowHook(
    JPBFxScreenGlowHook hook, void *user_data);
void jpb_FxSetGlowingManHook(
    JPBFxGlowingManHook hook, void *user_data);

void fx_Init(void);
int fx_DefaultTexturesReady(void);
void fx_Water(
    VECTOR *pos,
    int width,
    int height,
    uint32_t color,
    float factor,
    int speed);
void fx_GlowingMan(
    objectRoot *object,
    int width,
    int height,
    uint32_t inner_color,
    uint32_t outer_color);
void PlotZap(
    uint32_t col1,
    uint32_t col2,
    uint32_t col3,
    _svector *start,
    _svector *end,
    int level,
    int radius);
void fx_PlasmaZap(
    _plasma_zapvars *pzv,
    VECTOR *start,
    VECTOR *end,
    uint32_t color1,
    uint32_t color2,
    int radius);
void fx_screenGlow(
    _svector *start,
    _svector *end,
    int width,
    uint32_t color);

#if defined(__cplusplus)
#define JPB_FX_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_FX_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_FX_STATIC_ASSERT(
    sizeof(_plasma_zapvars) == 156,
    "_plasma_zapvars must match PDB type 0x142A");
JPB_FX_STATIC_ASSERT(
    offsetof(_plasma_zapvars, blobv) == 68,
    "_plasma_zapvars.blobv layout changed");
JPB_FX_STATIC_ASSERT(
    offsetof(_plasma_zapvars, sinusxvel) == 144,
    "_plasma_zapvars sinus velocity layout changed");

#undef JPB_FX_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
