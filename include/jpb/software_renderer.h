#ifndef JPB_SOFTWARE_RENDERER_H
#define JPB_SOFTWARE_RENDERER_H

#include "jpb/bmd.h"
#include "jpb/fmath.h"
#include "jpb/jpx.h"
#include "jpb/level_world.h"
#include "jpb/material.h"
#include "jpb/model.h"
#include "jpb/whook.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum JPBSoftwareRenderResult {
    JPB_SOFTWARE_RENDER_OK = 0,
    JPB_SOFTWARE_RENDER_INVALID_ARGUMENT = -1,
    JPB_SOFTWARE_RENDER_EMPTY_MESH = -2,
    JPB_SOFTWARE_RENDER_JPX_ERROR = -3,
    JPB_SOFTWARE_RENDER_BMD_ERROR = -4,
    JPB_SOFTWARE_RENDER_MODEL_TOO_LARGE = -5
};

enum {
    /*
     * Exact capacity of _RenderPackets' 36,864-byte transformed-FVECTOR
     * scratch array at RVA 0x129440.
     */
    JPB_SOFTWARE_MODEL_VERTEX_CAPACITY = 3072
};

typedef struct JPBSoftwareFramebuffer {
    uint32_t *pixels;
    int width;
    int height;
    int stridePixels;
} JPBSoftwareFramebuffer;

typedef struct JPBSoftwareJpxScene {
    const JPBJpxView *view;
    int levelIndex;
    float minX;
    float maxX;
    float minZ;
    float maxZ;
    float minY;
    float maxY;
    size_t strips;
    size_t vertices;
    /* JPX IDs matched to the exact live-FBX transparency database. */
    size_t fbxMaterialMatches;
    /* Exact shipped Streets JPX fingerprint accepted by the FBX cull map. */
    int streetsCullMapReady;
} JPBSoftwareJpxScene;

typedef struct JPBSoftwareRenderStats {
    size_t triangles;
    size_t lines;
    size_t pixels;
    size_t levelTransparentTriangles;
    size_t levelTransparentPixels;
    size_t levelGlassTriangles;
    size_t levelGlassPixels;
    size_t levelCulledTriangles;
    size_t modelTriangles;
    size_t modelLines;
    size_t modelPixels;
} JPBSoftwareRenderStats;

typedef struct JPBSoftwareTexture {
    const uint32_t *pixels;
    size_t width;
    size_t height;
    size_t stridePixels;
    /*
     * Portable carrier for the exact _Material state selected during
     * texture loading. A resolver that does not provide these fields gets
     * the reference defaults: flags zero, linear clamp, no color override.
     */
    uint32_t materialFlags;
    TEXTURE_SAMPLE_TYPE samplerType;
    int32_t colorOverride;
    /*
     * Exact Texture2D render class selected by _LoadTexture: ordinary=0,
     * p_=1, and a_=2. StartPoly routes classes 1 and 2 through the
     * transparent immediate-polygon queue.
     */
    int32_t materialType;
} JPBSoftwareTexture;

typedef struct JPBSoftwareDepthBuffer {
    float *values;
    size_t width;
    size_t height;
    size_t strideValues;
} JPBSoftwareDepthBuffer;

typedef struct JPBSoftwareMaterialVertex {
    float x;
    float y;
    float depth;
    float inverseDepth;
    float u;
    float v;
    float red;
    float green;
    float blue;
    float alpha;
} JPBSoftwareMaterialVertex;

typedef int (*JPBSoftwareTriangleSink)(
    void *user_data,
    const JPBSoftwareMaterialVertex *first,
    const JPBSoftwareMaterialVertex *second,
    const JPBSoftwareMaterialVertex *third,
    const JPBSoftwareTexture *texture);

/*
 * Dependency-free render form of the live FBX level data assembled by
 * el_chavo::InitFBXLevelData. Platform importers flatten their source scene
 * into triangle-list batches; the gameplay renderer never depends on the
 * importer library or its object model.
 */
typedef struct JPBSoftwareLevelVertex {
    FVECTOR position;
    float u;
    float v;
    float red;
    float green;
    float blue;
    float alpha;
} JPBSoftwareLevelVertex;

typedef struct JPBSoftwareLevelBatch {
    const JPBSoftwareLevelVertex *vertices;
    size_t vertexCount;
    const char *textureName;
    const char *meshName;
    JPBLevelFbxMeshPass pass;
    size_t meshIndex;
    size_t meshCount;
} JPBSoftwareLevelBatch;

typedef struct JPBSoftwareLevelMesh {
    const JPBSoftwareLevelBatch *batches;
    size_t batchCount;
    int levelIndex;
    size_t vertices;
    size_t triangles;
} JPBSoftwareLevelMesh;

typedef struct JPBSoftwareOwnedLevelMesh {
    JPBSoftwareLevelMesh mesh;
    JPBSoftwareLevelBatch *batches;
    JPBSoftwareLevelVertex *vertices;
} JPBSoftwareOwnedLevelMesh;

/*
 * Portable addition used to keep asset lookup and texture ownership outside
 * the renderer. texture_name is the exact geomData.t.Texture string.
 */
typedef int (*JPBSoftwareTextureResolver)(
    void *user_data,
    const char *texture_name,
    JPBSoftwareTexture *texture);

int jpb_SoftwareClearDepthBuffer(
    JPBSoftwareDepthBuffer *depth_buffer);

int jpb_SoftwarePrepareJpxScene(
    const JPBJpxView *view, JPBSoftwareJpxScene *scene);
int jpb_SoftwarePrepareJpxLevelScene(
    const JPBJpxView *view,
    int level_index,
    JPBSoftwareJpxScene *scene);
int jpb_SoftwareBuildJpxLevelMesh(
    const JPBSoftwareJpxScene *scene,
    JPBSoftwareOwnedLevelMesh *mesh);
void jpb_SoftwareFreeOwnedLevelMesh(
    JPBSoftwareOwnedLevelMesh *mesh);

/*
 * Portable camera/scene seam. Clips the segment from a gameplay focus point
 * to its desired eye against the filled JPX triangle strips and leaves the
 * eye padding units in front of the nearest hit. hit_fraction is 1.0 when
 * the segment is unobstructed.
 */
int jpb_SoftwareClipCameraToJpx(
    const JPBSoftwareJpxScene *scene,
    const FVECTOR *focus,
    const FVECTOR *desired_eye,
    float padding,
    FVECTOR *clipped_eye,
    float *hit_fraction);

/*
 * Draws height-colored JPX triangle-strip wireframes into caller-owned
 * X8R8G8B8 pixels. A non-null view matrix selects the live PC FOV/aspect
 * projection; null selects the format-inspection top view.
 */
int jpb_SoftwareRenderJpxWireframe(
    const JPBSoftwareJpxScene *scene,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color,
    JPBSoftwareRenderStats *stats);

/*
 * Filled JPX triangle-strip realization using the shipped per-vertex UV/color
 * payload and material names. Identified levels use the exact live-FBX
 * levelTextures/isTextureTransparent database through a documented JPX 8.3
 * name bridge; unidentified mirrors retain MATHEAD.listtype as a fallback.
 * The reference glass-name set selects its separate no-depth-write pass. The
 * caller owns the reusable depth storage so character models can share it
 * after the world pass.
 */
int jpb_SoftwareRenderJpxMaterialized(
    const JPBSoftwareJpxScene *scene,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color,
    JPBSoftwareTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats);

/*
 * Renders the exact live-FBX triangle/material partition after a platform
 * importer has converted it to the portable form above. The JPX scene stays
 * authoritative for collision, camera clipping, and world bounds.
 */
int jpb_SoftwareRenderLevelMesh(
    const JPBSoftwareLevelMesh *mesh,
    const JPBSoftwareJpxScene *world_scene,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color,
    JPBSoftwareTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats);
int jpb_SoftwareRenderLevelMeshPass(
    const JPBSoftwareLevelMesh *mesh,
    JPBLevelFbxMeshPass pass,
    const JPBSoftwareJpxScene *world_scene,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color,
    JPBSoftwareTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats);

/*
 * Dependency-free diagnostic realization of the PDB model hierarchy and BMD
 * geometry. The descriptive jpb_ boundary overlays posed model wireframes on
 * the existing framebuffer; it is not an original renderer symbol. A null
 * key_frame selects zero root motion for static inspection.
 */
int jpb_SoftwareRenderBmdWireframe(
    const JPBBmdView *bmd,
    modelObject *model,
    const _animFrame *key_frame,
    const FVECTOR *world_position,
    int32_t world_facing,
    MATRIX *view_matrix,
    const JPBSoftwareJpxScene *world_scene,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareRenderStats *stats);

/*
 * Materialized BMD path. It reproduces _RenderNode's faceUV/CVECTOR inputs and
 * the shipped PC shader's texture-times-vertex-color behavior. Texture loading
 * remains behind the callback so an nxdk backend can replace the PC asset
 * cache without changing model code. key_frame supplies the exact
 * render_RenderModel root translation; null retains the static-inspection
 * zero-root convention.
 */
int jpb_SoftwareRenderBmdMaterialized(
    const JPBBmdView *bmd,
    modelObject *model,
    const _animFrame *key_frame,
    const FVECTOR *world_position,
    int32_t world_facing,
    MATRIX *view_matrix,
    const JPBSoftwareJpxScene *world_scene,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareRenderStats *stats);

int jpb_SoftwareRenderBmdMaterializedWithDepth(
    const JPBBmdView *bmd,
    modelObject *model,
    const _animFrame *key_frame,
    const FVECTOR *world_position,
    int32_t world_facing,
    MATRIX *view_matrix,
    const JPBSoftwareJpxScene *world_scene,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats);
int jpb_SoftwareRenderBmdMaterializedWithDepthToSink(
    const JPBBmdView *bmd,
    modelObject *model,
    const _animFrame *key_frame,
    const FVECTOR *world_position,
    int32_t world_facing,
    MATRIX *view_matrix,
    const JPBSoftwareJpxScene *world_scene,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareTriangleSink triangle_sink,
    void *triangle_user_data,
    JPBSoftwareRenderStats *stats);

/*
 * Dependency-free realization of the exact _StartPoly/_SetVert completion
 * payload. _NoScaleEndPoly vertices are already in camera space and use the
 * live PC FOV/aspect projection; ordinary _EndPoly vertices are already
 * screen-space. The caller supplies the shared depth surface so water,
 * sprites, and other immediate primitives preserve world occlusion.
 */
int jpb_SoftwareDrawScreenPoly(
    const _Material *material,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats);
int jpb_SoftwareDrawScreenPolyToSink(
    const _Material *material,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareTriangleSink triangle_sink,
    void *triangle_user_data,
    JPBSoftwareRenderStats *stats);

#ifdef __cplusplus
}
#endif

#endif
