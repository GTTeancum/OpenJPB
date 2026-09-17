#ifndef JPB_UNPACK_H
#define JPB_UNPACK_H

#include "jpb/anim.h"

#include <stddef.h>

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
static_assert(offsetof(_optab, gotto) == 4, "_optab.gotto offset changed");
static_assert(offsetof(_dpcontext, wordbuffer) == 0, "_dpcontext.wordbuffer offset changed");
static_assert(offsetof(_dpcontext, wordsinbuffer) == (sizeof(void *) == 8 ? 8 : 4), "_dpcontext.wordsinbuffer offset changed");
static_assert(offsetof(_dpcontext, handyvalues) == (sizeof(void *) == 8 ? 16 : 8), "_dpcontext.handyvalues offset changed");
static_assert(offsetof(_dpcontext, huffdata) == (sizeof(void *) == 8 ? 24 : 12), "_dpcontext.huffdata offset changed");
static_assert(offsetof(_dpcontext, huffdataorigin) == (sizeof(void *) == 8 ? 32 : 16), "_dpcontext.huffdataorigin offset changed");
static_assert(offsetof(_dpcontext, huffdword) == (sizeof(void *) == 8 ? 40 : 20), "_dpcontext.huffdword offset changed");
static_assert(offsetof(_dpcontext, huffbits) == (sizeof(void *) == 8 ? 44 : 24), "_dpcontext.huffbits offset changed");
static_assert(offsetof(_dpcontext, n_bitmask) == (sizeof(void *) == 8 ? 48 : 28), "_dpcontext.n_bitmask offset changed");
static_assert(offsetof(_dpcontext, numseq) == (sizeof(void *) == 8 ? 52 : 32), "_dpcontext.numseq offset changed");
static_assert(offsetof(_dpcontext, numparts) == (sizeof(void *) == 8 ? 54 : 34), "_dpcontext.numparts offset changed");
static_assert(offsetof(_dpcontext, seqdata) == (sizeof(void *) == 8 ? 56 : 36), "_dpcontext.seqdata offset changed");
#else
_Static_assert(sizeof(_optab) == 8, "_optab layout changed");
_Static_assert(offsetof(_optab, gotto) == 4, "_optab.gotto offset changed");
_Static_assert(offsetof(_dpcontext, wordbuffer) == 0, "_dpcontext.wordbuffer offset changed");
_Static_assert(offsetof(_dpcontext, wordsinbuffer) == (sizeof(void *) == 8 ? 8 : 4), "_dpcontext.wordsinbuffer offset changed");
_Static_assert(offsetof(_dpcontext, handyvalues) == (sizeof(void *) == 8 ? 16 : 8), "_dpcontext.handyvalues offset changed");
_Static_assert(offsetof(_dpcontext, huffdata) == (sizeof(void *) == 8 ? 24 : 12), "_dpcontext.huffdata offset changed");
_Static_assert(offsetof(_dpcontext, huffdataorigin) == (sizeof(void *) == 8 ? 32 : 16), "_dpcontext.huffdataorigin offset changed");
_Static_assert(offsetof(_dpcontext, huffdword) == (sizeof(void *) == 8 ? 40 : 20), "_dpcontext.huffdword offset changed");
_Static_assert(offsetof(_dpcontext, huffbits) == (sizeof(void *) == 8 ? 44 : 24), "_dpcontext.huffbits offset changed");
_Static_assert(offsetof(_dpcontext, n_bitmask) == (sizeof(void *) == 8 ? 48 : 28), "_dpcontext.n_bitmask offset changed");
_Static_assert(offsetof(_dpcontext, numseq) == (sizeof(void *) == 8 ? 52 : 32), "_dpcontext.numseq offset changed");
_Static_assert(offsetof(_dpcontext, numparts) == (sizeof(void *) == 8 ? 54 : 34), "_dpcontext.numparts offset changed");
_Static_assert(offsetof(_dpcontext, seqdata) == (sizeof(void *) == 8 ? 56 : 36), "_dpcontext.seqdata offset changed");
#endif

#endif
