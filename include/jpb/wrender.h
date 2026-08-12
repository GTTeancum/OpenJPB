#ifndef JPB_WRENDER_H
#define JPB_WRENDER_H

#include "jpb/fmath.h"

#ifdef __cplusplus
extern "C" {
#endif

void PopMatrix(void);
void PushMatrix(void);

/*
 * Descriptive inspection seam for the PDB-named wRender.c module-local
 * matrix state. It exists for portable renderer integration and tests.
 */
MATRIX *jpb_WRenderCurrentMatrix(void);
int jpb_WRenderMatrixStackLevel(void);

#ifdef __cplusplus
}
#endif

#endif
