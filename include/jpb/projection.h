#ifndef JPB_PROJECTION_H
#define JPB_PROJECTION_H

#include "jpb/camera.h"
#include "jpb/fmath.h"

#ifdef __cplusplus
extern "C" {
#endif

enum JPBProjectionResult {
    JPB_PROJECTION_OK = 0,
    JPB_PROJECTION_DEGENERATE_CAMERA = -1,
    JPB_PROJECTION_INVALID_VIEWPORT = -2
};

/*
 * Portable inspection-camera adapter. This creates the game-owned MATRIX
 * consumed by the recovered projection routines; it does not stand in for
 * the still-unreviewed gameplay camera state machine.
 */
int jpb_BuildLookAtView(
    const FVECTOR *eye,
    const FVECTOR *target,
    const FVECTOR *world_up,
    MATRIX *view);

/*
 * Projects through the reference game's 640x480 projection and scales the
 * result to a caller-owned viewport. Returns one when near-clipped, zero
 * otherwise, or a negative JPBProjectionResult on invalid input.
 */
int jpb_ProjectToViewport(
    MATRIX *view,
    const FVECTOR *world,
    float viewport_width,
    float viewport_height,
    FVECTOR *screen);

/*
 * Projects through the matched PC renderer's D3D perspective. The executable
 * builds XMMatrixPerspectiveFovLH with a 53-degree vertical field of view,
 * the live render-target aspect ratio, and a 1.0/10000.0 depth range. This
 * adapter performs the corresponding viewport map while retaining the
 * reconstruction's legacy-compatible linear depth value for software
 * rasterization. The exact 640x480 legacy path above remains unchanged.
 */
int jpb_ProjectPcToViewport(
    MATRIX *view,
    const FVECTOR *world,
    float viewport_width,
    float viewport_height,
    FVECTOR *screen);

/*
 * Projects a position already transformed into the matched PC renderer's
 * left-handed camera space. This prefixed portable seam lets software
 * rasterizers clip polygons at camera Z == 1 before the perspective divide,
 * matching the D3D primitive-clipping stage without duplicating projection
 * constants. Positions on the near plane are retained.
 */
int jpb_ProjectPcCameraToViewport(
    const FVECTOR *camera,
    float viewport_width,
    float viewport_height,
    FVECTOR *screen);
void jpb_PcGameplayViewport(
    float framebuffer_width,
    float framebuffer_height,
    float *viewport_x,
    float *viewport_y,
    float *viewport_width,
    float *viewport_height);
int jpb_ProjectPcGameplayToViewport(
    MATRIX *view,
    const FVECTOR *world,
    float framebuffer_width,
    float framebuffer_height,
    FVECTOR *screen);
int jpb_ProjectPcGameplayCameraToViewport(
    const FVECTOR *camera,
    float framebuffer_width,
    float framebuffer_height,
    FVECTOR *screen);

/*
 * Exact gameplay-camera conversion and world-to-screen matrix construction,
 * followed by the same viewport adapter as jpb_ProjectToViewport.
 */
int jpb_ProjectCameraToViewport(
    Camera *camera,
    sceneGeometryEnv *environment,
    const FVECTOR *world,
    float viewport_width,
    float viewport_height,
    FVECTOR *screen);

#ifdef __cplusplus
}
#endif

#endif
