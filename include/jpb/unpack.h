#ifndef JPB_UNPACK_H
#define JPB_UNPACK_H

#include "jpb/anim.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exact PDB type 0x114C. */
typedef struct _optab {
    int32_t vals;
    int32_t gotto;
} _optab;

void unpack_grabsvectors_raw(
    _dpcontext *context, int count, int16_t *vectors);
void unpack_grabsvectors_s(
    _dpcontext *context, int count, int16_t *vectors);
void unpack_init(
    _optab *optable,
    uint16_t *values,
    uint32_t *tree,
    int tree_size);
void unpack_initcontext(_dpcontext *context, char *data);
void unpack_seekcontext(_dpcontext *context, int seek);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
static_assert(sizeof(_optab) == 8, "_optab layout changed");
#else
_Static_assert(sizeof(_optab) == 8, "_optab layout changed");
#endif

#endif
