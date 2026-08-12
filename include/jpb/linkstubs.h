#ifndef JPB_LINKSTUBS_H
#define JPB_LINKSTUBS_H

#include "jpb/fmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exact retail compatibility stub at matched-PC RVA 0xBB8F0. */
void SetCameraMatrix(void);
void SetRotMatrix(MATRIX *matrix);
void SetTransMatrix(MATRIX *matrix);
/* Exact retail compatibility stub at matched-PC RVA 0xBBA30. */
void _HandleBackDrop(void);

#ifdef __cplusplus
}
#endif

#endif
