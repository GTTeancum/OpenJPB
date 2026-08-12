#ifndef JPB_CUBE_H
#define JPB_CUBE_H

#include "jpb/fmath.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_CULL_MESH_COUNT = 32,
    JPB_CULL_VISIBILITY_COUNT = 1000,
    JPB_CULL_PLANE_COUNT = 7
};

/* Exact matched-PC PDB global and procedures. */
typedef struct JPBCullState {
    char visarray[JPB_CULL_VISIBILITY_COUNT];
    FVECTOR4 planes[JPB_CULL_PLANE_COUNT];
} JPBCullState;

extern JPBCullState cull;
extern int32_t cullmesh[JPB_CULL_MESH_COUNT];
extern char tato_wallfrigflag;
extern char fed_wallfrigflag;
/*
 * Inferred name for the anonymous cube/runtime flag word at matched-PC
 * RVA 0x10DBEFC. Bit 3 is published by cube_NewWorldRender and selects
 * the stronger MOVE_HOVER descent limit used at the end of Streets.
 */
extern uint32_t jpb_CubeRuntimeFlags;
/* Exact PDB globals consumed and published by cube_NewWorldRender. */
extern MATRIX twattedcameramatrix;
extern MATRIX *worldTURTLEMatrix;

typedef struct JPBCubeRenderBounds {
    int32_t minX;
    int32_t minZ;
    int32_t maxX;
    int32_t maxZ;
} JPBCubeRenderBounds;

/*
 * The matched level-12 path submits the old JPX cube stream directly from
 * plotsomecubes. Dependency-light hosts realize that immediate renderer at
 * this boundary while the recovered culling and scheduling remain game-owned.
 */
typedef int (*JPBCubeLegacyRenderHook)(
    void *user_data,
    int min_x,
    int min_z,
    int max_x,
    int max_z);

void cube_GetCubeAmbientLight(VECTOR *position, CVECTOR *color);
void cube_HideMesh(int mesh);
void cube_InitVisibility(void);
void cube_NewWorldRender(MATRIX *matrix);
void cube_ShowMesh(int mesh);
void jpb_CubeGetLastRenderBounds(JPBCubeRenderBounds *bounds);
void jpb_CubeSetLegacyRenderHook(
    JPBCubeLegacyRenderHook hook, void *user_data);
int plotsomecubes(int min_x, int min_z, int max_x, int max_z);
void twatcameramatrix(MATRIX *matrix, MATRIX *m);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
static_assert(sizeof(JPBCullState) == 1112, "PDB cull layout changed");
static_assert(offsetof(JPBCullState, planes) == 1000,
              "PDB cull.planes offset changed");
#else
_Static_assert(sizeof(JPBCullState) == 1112, "PDB cull layout changed");
_Static_assert(offsetof(JPBCullState, planes) == 1000,
               "PDB cull.planes offset changed");
#endif

#endif
