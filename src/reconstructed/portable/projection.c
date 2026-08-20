/*
 * Dependency-light PC/Xbox-neutral projection adapter.
 *
 * The look-at construction is a substituted integration aid. The legacy
 * projection delegates to the reviewed PerspectiveTransformFV recovered from
 * fmath.c. The live PC adapter separately mirrors the D3D renderer's recovered
 * XMMatrixPerspectiveFovLH setup without adding a graphics dependency.
 */

#include "jpb/projection.h"

#include <limits.h>
#include <math.h>

static float projection_dot(
    const FVECTOR *left, const FVECTOR *right)
{
    return left->vx * right->vx +
           left->vy * right->vy +
           left->vz * right->vz;
}

static FVECTOR projection_cross(
    const FVECTOR *left, const FVECTOR *right)
{
    FVECTOR result;

    result.vx = left->vy * right->vz - left->vz * right->vy;
    result.vy = left->vz * right->vx - left->vx * right->vz;
    result.vz = left->vx * right->vy - left->vy * right->vx;
    return result;
}

static int projection_normalize(FVECTOR *vector)
{
    float length_squared = projection_dot(vector, vector);
    float inverse_length;

    if (!(length_squared > 0.0f)) {
        return 0;
    }
    inverse_length = 1.0f / sqrtf(length_squared);
    vector->vx *= inverse_length;
    vector->vy *= inverse_length;
    vector->vz *= inverse_length;
    return 1;
}

static int projection_float_to_translation(
    float value, int32_t *translation)
{
    if (!(value >= (float)INT32_MIN &&
          value < 2147483648.0f)) {
        return 0;
    }
    *translation = (int32_t)value;
    return 1;
}

int jpb_BuildLookAtView(
    const FVECTOR *eye,
    const FVECTOR *target,
    const FVECTOR *world_up,
    MATRIX *view)
{
    FVECTOR forward;
    FVECTOR right;
    FVECTOR up;

    if (eye == NULL || target == NULL ||
        world_up == NULL || view == NULL) {
        return JPB_PROJECTION_DEGENERATE_CAMERA;
    }
    forward.vx = target->vx - eye->vx;
    forward.vy = target->vy - eye->vy;
    forward.vz = target->vz - eye->vz;
    if (!projection_normalize(&forward)) {
        return JPB_PROJECTION_DEGENERATE_CAMERA;
    }
    right = projection_cross(world_up, &forward);
    if (!projection_normalize(&right)) {
        return JPB_PROJECTION_DEGENERATE_CAMERA;
    }
    up = projection_cross(&forward, &right);

    view->m[0][0] = right.vx;
    view->m[0][1] = right.vy;
    view->m[0][2] = right.vz;
    view->m[1][0] = up.vx;
    view->m[1][1] = up.vy;
    view->m[1][2] = up.vz;
    view->m[2][0] = forward.vx;
    view->m[2][1] = forward.vy;
    view->m[2][2] = forward.vz;
    if (!projection_float_to_translation(
            -projection_dot(&right, eye), &view->t[0]) ||
        !projection_float_to_translation(
            -projection_dot(&up, eye), &view->t[1]) ||
        !projection_float_to_translation(
            -projection_dot(&forward, eye), &view->t[2])) {
        return JPB_PROJECTION_DEGENERATE_CAMERA;
    }
    return JPB_PROJECTION_OK;
}

int jpb_ProjectToViewport(
    MATRIX *view,
    const FVECTOR *world,
    float viewport_width,
    float viewport_height,
    FVECTOR *screen)
{
    FVECTOR source;
    int clipped;

    if (view == NULL || world == NULL || screen == NULL ||
        !(viewport_width > 0.0f) ||
        !(viewport_height > 0.0f)) {
        return JPB_PROJECTION_INVALID_VIEWPORT;
    }
    source = *world;
    clipped = PerspectiveTransformFV(view, &source, screen);
    screen->vx *= viewport_width / 640.0f;
    screen->vy *= viewport_height / 480.0f;
    return clipped;
}

int jpb_ProjectPcToViewport(
    MATRIX *view,
    const FVECTOR *world,
    float viewport_width,
    float viewport_height,
    FVECTOR *screen)
{
    /*
     * CD3DApplication's device setup and EndRender resolution-change path
     * pass the executable constant at RVA 0x334404 to
     * XMMatrixPerspectiveFovLH. That constant is 0.9250245094299316 radians
     * (53 degrees); near and far are 1.0 and 10000.0. The recovered game
     * camera matrix already carries the renderer's screen-Y convention, so
     * both screen axes use its signed camera-space components here.
     */
    FVECTOR source;
    FVECTOR transformed;

    if (view == NULL || world == NULL || screen == NULL ||
        !(viewport_width > 0.0f) ||
        !(viewport_height > 0.0f)) {
        return JPB_PROJECTION_INVALID_VIEWPORT;
    }
    source = *world;
    fApplyMatrixFV(view, &source, &transformed);
    transformed.vx += (float)view->t[0];
    transformed.vy += (float)view->t[1];
    transformed.vz += (float)view->t[2];
    return jpb_ProjectPcCameraToViewport(
        &transformed,
        viewport_width,
        viewport_height,
        screen);
}

int jpb_ProjectPcCameraToViewport(
    const FVECTOR *camera,
    float viewport_width,
    float viewport_height,
    FVECTOR *screen)
{
    const float vertical_fov = 0.9250245094299316f;
    const float near_clip = 1.0f;
    const float depth_scale = 10240.0f;
    float depth;
    float focal_scale;
    int clipped;

    if (camera == NULL || screen == NULL ||
        !(viewport_width > 0.0f) ||
        !(viewport_height > 0.0f)) {
        return JPB_PROJECTION_INVALID_VIEWPORT;
    }
    depth = camera->vz;
    /* D3D retains primitives exactly on the configured near plane. */
    clipped = depth < near_clip;
    if (clipped) {
        depth = near_clip;
    }
    focal_scale =
        (viewport_height * 0.5f) / tanf(vertical_fov * 0.5f);
    screen->vx =
        camera->vx * focal_scale / depth + viewport_width * 0.5f;
    screen->vy =
        camera->vy * focal_scale / depth + viewport_height * 0.5f;
    /*
     * A constant depth normalization does not alter ordering or perspective
     * interpolation. Keep the exact legacy 10240 divisor so PC-projected
     * geometry continues to share the software depth surface with recovered
     * screen-space effects.
     */
    screen->vz = depth / depth_scale;
    return clipped;
}

void jpb_PcGameplayViewport(
    float framebuffer_width,
    float framebuffer_height,
    float *viewport_x,
    float *viewport_y,
    float *viewport_width,
    float *viewport_height)
{
    float width = framebuffer_width > 0.0f ? framebuffer_width : 0.0f;
    float height = framebuffer_height > 0.0f ? framebuffer_height : 0.0f;

    if (viewport_x != NULL) *viewport_x = 0.0f;
    if (viewport_y != NULL) *viewport_y = 0.0f;
    if (viewport_width != NULL) *viewport_width = width;
    if (viewport_height != NULL) *viewport_height = height;
}

int jpb_ProjectPcGameplayCameraToViewport(
    const FVECTOR *camera,
    float framebuffer_width,
    float framebuffer_height,
    FVECTOR *screen)
{
    float viewport_x;
    float viewport_y;
    float viewport_width;
    float viewport_height;
    int result;

    if (screen == NULL) {
        return JPB_PROJECTION_INVALID_VIEWPORT;
    }
    jpb_PcGameplayViewport(
        framebuffer_width,
        framebuffer_height,
        &viewport_x,
        &viewport_y,
        &viewport_width,
        &viewport_height);
    result = jpb_ProjectPcCameraToViewport(
        camera,
        viewport_width,
        viewport_height,
        screen);
    if (result >= 0) {
        screen->vx += viewport_x;
        screen->vy += viewport_y;
    }
    return result;
}

int jpb_ProjectPcGameplayToViewport(
    MATRIX *view,
    const FVECTOR *world,
    float framebuffer_width,
    float framebuffer_height,
    FVECTOR *screen)
{
    FVECTOR source;
    FVECTOR transformed;

    if (view == NULL || world == NULL || screen == NULL ||
        !(framebuffer_width > 0.0f) ||
        !(framebuffer_height > 0.0f)) {
        return JPB_PROJECTION_INVALID_VIEWPORT;
    }
    source = *world;
    fApplyMatrixFV(view, &source, &transformed);
    transformed.vx += (float)view->t[0];
    transformed.vy += (float)view->t[1];
    transformed.vz += (float)view->t[2];
    return jpb_ProjectPcGameplayCameraToViewport(
        &transformed,
        framebuffer_width,
        framebuffer_height,
        screen);
}

int jpb_ProjectCameraToViewport(
    Camera *camera,
    sceneGeometryEnv *environment,
    const FVECTOR *world,
    float viewport_width,
    float viewport_height,
    FVECTOR *screen)
{
    if (camera == NULL || environment == NULL) {
        return JPB_PROJECTION_DEGENERATE_CAMERA;
    }
    camera_Camera2ViewVector(camera, environment);
    scene_UpdateWorld2ScreenMatrix(environment);
    return jpb_ProjectToViewport(
        &environment->matrix,
        world,
        viewport_width,
        viewport_height,
        screen);
}
