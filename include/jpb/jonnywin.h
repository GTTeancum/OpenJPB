#ifndef JPB_JONNYWIN_H
#define JPB_JONNYWIN_H

#include "jpb/fmath.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact matched-PC PDB type used by globalwinmatrix. */
typedef struct _winmat {
    float m[3][3];
    float t[3];
} _winmat;

extern _winmat globalwinmatrix;

void SetupTransformMatrix(MATRIX *matrix);
void SetupWorldmeshMatrix(MATRIX *matrix);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
static_assert(sizeof(_winmat) == 48, "_winmat layout changed");
static_assert(offsetof(_winmat, t) == 36, "_winmat.t offset changed");
#else
_Static_assert(sizeof(_winmat) == 48, "_winmat layout changed");
_Static_assert(offsetof(_winmat, t) == 36, "_winmat.t offset changed");
#endif

#endif
