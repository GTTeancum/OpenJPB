#ifndef JPB_LEVEL_WORLD_H
#define JPB_LEVEL_WORLD_H

#include "jpb/cube.h"
#include "jpb/fmath.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_LEVEL_COUNT = 26,
    JPB_LEVEL_NAME_COUNT = 28,
    JPB_LEVEL_INDEX_NONE = -1
};

typedef enum JPBLevelFbxMeshPass {
    JPB_LEVEL_FBX_PASS_OPAQUE = 0,
    JPB_LEVEL_FBX_PASS_TRANSPARENT,
    JPB_LEVEL_FBX_PASS_GLASS
} JPBLevelFbxMeshPass;

/*
 * Exact PDB globals from game.pdb's global stream:
 *   sLevelNames  RVA 0x4BD650, char *[28]
 *   level_offset RVA 0x4AFDB0, float [26][3]
 *   level_scale  RVA 0x4AFEF0, float [26][3]
 *   startPos     RVA 0x4CB930, _svector [26][2]
 *   cullmesh     RVA 0x4B0BD0, int [32]
 */
extern char *sLevelNames[JPB_LEVEL_NAME_COUNT];
extern float level_offset[JPB_LEVEL_COUNT][3];
extern float level_scale[JPB_LEVEL_COUNT][3];
extern _svector startPos[JPB_LEVEL_COUNT][2];

/*
 * Portable, dependency-free extraction of cube_NewWorldRender's level
 * placement. FBX stores the authoring axes in x/z/y game order. The function
 * accepts the raw FBX x/y/z order and returns the fixed-unit game x/y/z order.
 */
int jpb_LevelTransformFbxVertex(
    int level_index,
    float fbx_x,
    float fbx_y,
    float fbx_z,
    FVECTOR *game_vertex);

/*
 * Exact InitFBXLevelData per-material UV scroll selector. The returned pair
 * is stored on every vertex and consumed by LevelVertexShader.hlsl together
 * with g_levelUVScroll.
 */
void jpb_LevelFbxUvScroll(
    int level_index,
    const char *texture_name,
    float *scroll_u,
    float *scroll_v);

/*
 * DrawLevel derives the Streets cullmesh slot from each FBX node name:
 * WallNN_Broken -> NN * 2 - 1, WallNN_Solid -> NN * 2. Other name
 * lengths retain atoi(name + 4), which maps the shipped streets_A0 base
 * mesh to slot zero. Returns JPB_LEVEL_INDEX_NONE when no safe slot exists.
 */
int jpb_StreetsCullMeshIndexFromName(const char *mesh_name);

/*
 * Portable statement of the exact DrawLevel/DrawLevelTransparent/
 * DrawLevelTransparentGlass gates. All three use the name-derived rule on
 * Streets. Elsewhere only DrawLevel consults cullmesh by vector position;
 * transparent and glass vectors draw unconditionally.
 */
int jpb_ShouldDrawFbxMesh(
    int level_index,
    JPBLevelFbxMeshPass pass,
    size_t opaque_mesh_count,
    size_t mesh_index,
    const char *mesh_name);

/*
 * Resolves a case-insensitive level-name component from a path. This checks
 * both the filename stem and parent components, so alternate meshes such as
 * STREETS/xstreets.jpx retain the owning level's transform.
 */
int jpb_LevelIndexFromPath(const char *path);

#ifdef __cplusplus
}
#endif

#endif
