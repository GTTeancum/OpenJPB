#ifndef JPB_WRENDER_H
#define JPB_WRENDER_H

#include "jpb/fmath.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct geomData geomData;
typedef struct PCB PCB;
typedef struct wsl_fatPoly wsl_fatPoly;
typedef struct wsl_mapEntry wsl_mapEntry;
typedef struct wsl_thinPoly wsl_thinPoly;

void PopMatrix(void);
void PushMatrix(void);
typedef struct fPoint4 {
    float x;
    float y;
    float z;
    float p;
} fPoint4;

int _CubeRender(wsl_mapEntry *entry, MATRIX *matrix);
int _Cull(MATRIX *matrix, VECTOR *point);
int _FatRender(wsl_fatPoly *entry, MATRIX *matrix, char *color);
void _PerspectiveTransform(
    MATRIX *matrix, _svector *source, fPoint4 *destination);
void _RenderParticle(MATRIX *matrix, PCB *pcb);
int _ThinRender(wsl_thinPoly *entry, MATRIX *matrix, char *color);
void __InitDisplay(int xres, int yres, int depth);
int gl_RenderNode(
    geomData *geometry,
    MATRIX *matrix,
    MATRIX *light,
    fPoint4 *point_cache);
void psx_LoadBar(void);

/*
 * Descriptive inspection seam for the PDB-named wRender.c module-local
 * matrix state. It exists for portable renderer integration and tests.
 */
MATRIX *jpb_WRenderCurrentMatrix(void);
int jpb_WRenderMatrixStackLevel(void);
int32_t jpb_WRenderMatrixStackBaseLow32(void);

#if defined(__cplusplus)
static_assert(sizeof(fPoint4) == 16, "fPoint4 PDB layout changed");
#else
_Static_assert(sizeof(fPoint4) == 16, "fPoint4 PDB layout changed");
#endif

#ifdef __cplusplus
}
#endif

#endif
