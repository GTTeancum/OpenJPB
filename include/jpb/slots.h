#ifndef JPB_SLOTS_H
#define JPB_SLOTS_H

#include "jpb/fmath.h"
#include "jpb/world.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JPBSlotsVec2 {
    int16_t x;
    int16_t y;
} JPBSlotsVec2;

typedef struct cubeStack {
    _svector trans;
    VECTOR animTrans;
    VECTOR camera;
    int32_t numV;
    int32_t *pOT;
    wsl_mapSlot *pMap;
    wsl_libPart **pLib;
    int32_t *pIndex;
    int32_t *pColor;
    int32_t *pUV;
    int32_t numF;
    int16_t tpage;
    int16_t clut;
    int16_t *pids;
    MATRIX *matrix;
    int32_t *tags;
    int32_t *material;
    int32_t depth;
    int32_t poly;
    CVECTOR *pPalette;
    int32_t count;
    int32_t texwin;
    int32_t polyptr;
    int32_t prim;
    _svector p0pos;
    int32_t besttargetlen;
    int32_t regStore[11];
    _svector temp[2];
    int32_t per[92];
    _svector cubeCorners[8];
    JPBSlotsVec2 cubeResults[8];
} cubeStack;

extern int maptarget[2];

int sampleWalk(
    cubeStack *sr,
    int minx,
    int minz,
    int maxx,
    int maxz,
    MATRIX *matrix);
int slot_levelstart(void);

#ifdef JPB_SLOTS_TESTING
typedef struct JPBSlotsState {
    int libpartanimtick;
    int bleft;
    int bright;
    int btop;
    int bbottom;
    int wasculled;
    int elapsedleveltime;
    unsigned char fatTrack[256];
} JPBSlotsState;

void jpb_slots_get_state(JPBSlotsState *state);
#endif

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_SLOTS_STATIC_ASSERT static_assert
#else
#define JPB_SLOTS_STATIC_ASSERT _Static_assert
#endif

JPB_SLOTS_STATIC_ASSERT(sizeof(JPBSlotsVec2) == 4,
                        "_vec2 PDB layout changed");
JPB_SLOTS_STATIC_ASSERT(offsetof(cubeStack, animTrans) == 8,
                        "cubeStack.animTrans PDB offset changed");
JPB_SLOTS_STATIC_ASSERT(offsetof(cubeStack, besttargetlen) == 176,
                        "cubeStack.besttargetlen PDB offset changed");
JPB_SLOTS_STATIC_ASSERT(offsetof(cubeStack, per) == 240,
                        "cubeStack.per PDB offset changed");
JPB_SLOTS_STATIC_ASSERT(offsetof(cubeStack, cubeResults) == 672,
                        "cubeStack.cubeResults PDB offset changed");
JPB_SLOTS_STATIC_ASSERT(sizeof(cubeStack) == 704,
                        "cubeStack PDB layout changed");

#undef JPB_SLOTS_STATIC_ASSERT

#endif
