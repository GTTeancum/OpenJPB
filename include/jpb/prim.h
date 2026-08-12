#ifndef JPB_PRIM_H
#define JPB_PRIM_H

#include "jpb/fmath.h"

#ifdef __cplusplus
extern "C" {
#endif

void prim_gSetBkColor(int red, int green, int blue);

/*
 * Descriptive inspection seam for the two retail draw-environment color
 * copies owned by prim_gSetBkColor.
 */
const CVECTOR *jpb_PrimGetBackgroundColor(int surface);

#ifdef __cplusplus
}
#endif

#endif
