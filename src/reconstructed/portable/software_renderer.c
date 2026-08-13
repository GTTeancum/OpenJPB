/*
 * Dependency-light JPX/BMD software renderer.
 *
 * Mesh traversal, projection, clipping, and rasterization use only standard C
 * and caller-owned memory. Platform layers only need to present X8R8G8B8
 * pixels, which keeps later platform framebuffer adapters narrow. JPX keeps
 * a wireframe inspection path and a filled material path; BMD models share
 * the same perspective-correct texture/color rasterizer and optional
 * caller-owned depth surface.
 */

#include "jpb/software_renderer.h"
#include "jpb/level_world.h"
#include "jpb/material.h"
#include "jpb/projection.h"
#include "jpb/streets_cull_map.h"
#include "jpb/transparent_texture_database.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum {
    SOFTWARE_RENDER_MARGIN = 24,
    SOFTWARE_SCREEN_POLY_VERTEX_CAPACITY = 32,
    /* A convex triangle or quad gains at most one vertex at one clip plane. */
    SOFTWARE_CLIPPED_POLYGON_CAPACITY = 5,
    SOFTWARE_LEVEL_CORUS1 = 6
};

typedef struct SoftwareDraw {
    const JPBSoftwareJpxScene *scene;
    JPBSoftwareFramebuffer *framebuffer;
    MATRIX *view;
    float scale;
    float offsetX;
    float offsetZ;
    int modelGeometry;
    JPBSoftwareRenderStats stats;
} SoftwareDraw;

typedef struct SoftwareModelTransform {
    MATRIX rotation;
    FVECTOR translation;
} SoftwareModelTransform;

typedef enum SoftwareMaterialShader {
    SOFTWARE_MATERIAL_SHADER_MODEL = 0,
    SOFTWARE_MATERIAL_SHADER_LEVEL_OPAQUE,
    SOFTWARE_MATERIAL_SHADER_LEVEL_TRANSPARENT
} SoftwareMaterialShader;

typedef enum SoftwareJpxPass {
    SOFTWARE_JPX_PASS_OPAQUE = 0,
    SOFTWARE_JPX_PASS_TRANSPARENT,
    SOFTWARE_JPX_PASS_GLASS
} SoftwareJpxPass;

typedef struct SoftwareModelDraw {
    const JPBBmdView *bmd;
    SoftwareDraw draw;
    FVECTOR transformed[JPB_SOFTWARE_MODEL_VERTEX_CAPACITY];
    JPBSoftwareTextureResolver resolveTexture;
    void *textureUserData;
    float *depthBuffer;
    size_t depthStride;
    int repeatTexture;
    int blendTextureAlpha;
    int depthWrite;
    int wrapLevelWorld;
    FVECTOR levelCameraOrigin;
    SoftwareMaterialShader materialShader;
    SoftwareJpxPass jpxPass;
} SoftwareModelDraw;

static void software_set_level_camera_origin(
    SoftwareModelDraw *state,
    const JPBSoftwareJpxScene *scene,
    MATRIX *view)
{
    float translation_x;
    float translation_y;
    float translation_z;

    state->wrapLevelWorld =
        scene != NULL &&
        scene->levelIndex == SOFTWARE_LEVEL_CORUS1 &&
        view != NULL;
    if (!state->wrapLevelWorld) {
        memset(&state->levelCameraOrigin, 0,
               sizeof(state->levelCameraOrigin));
        return;
    }
    translation_x = (float)view->t[0];
    translation_y = (float)view->t[1];
    translation_z = (float)view->t[2];
    /* The recovered view matrix is orthonormal, so -R^T t recovers the
     * signed 16-bit camera-space world origin without reaching into camera
     * globals from this renderer boundary. */
    state->levelCameraOrigin.vx = -(
        view->m[0][0] * translation_x +
        view->m[1][0] * translation_y +
        view->m[2][0] * translation_z);
    state->levelCameraOrigin.vy = -(
        view->m[0][1] * translation_x +
        view->m[1][1] * translation_y +
        view->m[2][1] * translation_z);
    state->levelCameraOrigin.vz = -(
        view->m[0][2] * translation_x +
        view->m[1][2] * translation_y +
        view->m[2][2] * translation_z);
}

static float software_wrap_level_coordinate(
    float coordinate, float camera_coordinate)
{
    const float period = 65536.0f;
    const float half_period = 32768.0f;
    float delta = coordinate - camera_coordinate;

    /* Coruscant 1 gameplay positions and authored cameras cross the original
     * signed 16-bit world seam. Its live FBX vertices are floats, so select
     * the equivalent world image nearest the camera before applying the
     * recovered view. Other levels retain their ordinary unwrapped FBX
     * coordinates; wrapping them can pull remote geometry into the depth
     * buffer. */
    if (!isfinite(delta)) {
        return coordinate;
    }
    delta = fmodf(delta + half_period, period);
    if (delta < 0.0f) {
        delta += period;
    }
    return camera_coordinate + delta - half_period;
}

typedef struct SoftwareMaterialVertex {
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
} SoftwareMaterialVertex;

typedef struct SoftwareCameraMaterialVertex {
    FVECTOR position;
    float u;
    float v;
    float red;
    float green;
    float blue;
    float alpha;
} SoftwareCameraMaterialVertex;

static float software_material_polygon_winding(
    const SoftwareMaterialVertex *vertices,
    size_t vertex_count);
static void software_draw_material_triangle(
    SoftwareModelDraw *state,
    const SoftwareMaterialVertex *first,
    const SoftwareMaterialVertex *second,
    const SoftwareMaterialVertex *third,
    const JPBSoftwareTexture *texture);

static void software_jpx_world_vertex(
    const JPBSoftwareJpxScene *scene,
    const JPBJpxVertex *vertex,
    FVECTOR *world)
{
    if (scene->levelIndex != JPB_LEVEL_INDEX_NONE) {
        jpb_LevelTransformFbxVertex(
            scene->levelIndex,
            vertex->x,
            vertex->z,
            vertex->y,
            world);
    } else {
        world->vx = vertex->x;
        world->vy = vertex->y;
        world->vz = vertex->z;
    }
}

static int software_collect_bounds(
    const JPBJpxPatchSite *site, void *user_data)
{
    JPBSoftwareJpxScene *scene = (JPBSoftwareJpxScene *)user_data;
    uint16_t index;

    for (index = 0; index < site->vertexCount; ++index) {
        JPBJpxVertex vertex;
        FVECTOR world;

        if (jpx_DecodeVertex(site, index, &vertex) != JPB_JPX_OK) {
            return 1;
        }
        software_jpx_world_vertex(scene, &vertex, &world);
        if (world.vx < scene->minX) scene->minX = world.vx;
        if (world.vx > scene->maxX) scene->maxX = world.vx;
        if (world.vz < scene->minZ) scene->minZ = world.vz;
        if (world.vz > scene->maxZ) scene->maxZ = world.vz;
        if (world.vy < scene->minY) scene->minY = world.vy;
        if (world.vy > scene->maxY) scene->maxY = world.vy;
        ++scene->vertices;
    }
    ++scene->strips;
    return 0;
}

int jpb_SoftwarePrepareJpxScene(
    const JPBJpxView *view, JPBSoftwareJpxScene *scene)
{
    return jpb_SoftwarePrepareJpxLevelScene(
        view, JPB_LEVEL_INDEX_NONE, scene);
}

int jpb_SoftwarePrepareJpxLevelScene(
    const JPBJpxView *view,
    int level_index,
    JPBSoftwareJpxScene *scene)
{
    int result;
    uint16_t material_index;

    if (view == NULL || scene == NULL ||
        level_index < JPB_LEVEL_INDEX_NONE ||
        level_index >= JPB_LEVEL_COUNT) {
        return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
    }
    memset(scene, 0, sizeof(*scene));
    scene->view = view;
    scene->levelIndex = level_index;
    scene->streetsCullMapReady =
        level_index == 8 && jpb_IsMatchedStreetsJpx(view);
    if (level_index != JPB_LEVEL_INDEX_NONE) {
        for (material_index = 0;
             material_index < view->numMaterials;
             ++material_index) {
            if (jpb_IsTextureTransparentForJpxMirror(
                    jpx_GetMaterialName(view, material_index),
                    level_index)) {
                ++scene->fbxMaterialMatches;
            }
        }
    }
    scene->minX = FLT_MAX;
    scene->maxX = -FLT_MAX;
    scene->minZ = FLT_MAX;
    scene->maxZ = -FLT_MAX;
    scene->minY = FLT_MAX;
    scene->maxY = -FLT_MAX;
    result = jpx_ForEachPatchSite(
        view, software_collect_bounds, scene);
    if (result != JPB_JPX_OK) {
        return JPB_SOFTWARE_RENDER_JPX_ERROR;
    }
    if (scene->vertices == 0) {
        return JPB_SOFTWARE_RENDER_EMPTY_MESH;
    }
    return JPB_SOFTWARE_RENDER_OK;
}

typedef struct SoftwareCameraClip {
    const JPBSoftwareJpxScene *scene;
    FVECTOR origin;
    FVECTOR direction;
    float minimumFraction;
    float hitFraction;
} SoftwareCameraClip;

static void software_subtract_vector(
    const FVECTOR *left,
    const FVECTOR *right,
    FVECTOR *result)
{
    result->vx = left->vx - right->vx;
    result->vy = left->vy - right->vy;
    result->vz = left->vz - right->vz;
}

static void software_cross_vector(
    const FVECTOR *left,
    const FVECTOR *right,
    FVECTOR *result)
{
    result->vx =
        left->vy * right->vz - left->vz * right->vy;
    result->vy =
        left->vz * right->vx - left->vx * right->vz;
    result->vz =
        left->vx * right->vy - left->vy * right->vx;
}

static float software_dot_vector(
    const FVECTOR *left,
    const FVECTOR *right)
{
    return left->vx * right->vx +
           left->vy * right->vy +
           left->vz * right->vz;
}

static void software_clip_camera_triangle(
    SoftwareCameraClip *clip,
    const FVECTOR *first,
    const FVECTOR *second,
    const FVECTOR *third)
{
    const float determinant_epsilon = 0.00001f;
    FVECTOR edge1;
    FVECTOR edge2;
    FVECTOR cross;
    FVECTOR from_first;
    float determinant;
    float inverse_determinant;
    float u;
    float v;
    float fraction;

    software_subtract_vector(second, first, &edge1);
    software_subtract_vector(third, first, &edge2);
    software_cross_vector(&clip->direction, &edge2, &cross);
    determinant = software_dot_vector(&edge1, &cross);
    if (determinant > -determinant_epsilon &&
        determinant < determinant_epsilon) {
        return;
    }
    inverse_determinant = 1.0f / determinant;
    software_subtract_vector(
        &clip->origin, first, &from_first);
    u =
        software_dot_vector(&from_first, &cross) *
        inverse_determinant;
    if (u < 0.0f || u > 1.0f) {
        return;
    }
    software_cross_vector(&from_first, &edge1, &cross);
    v =
        software_dot_vector(&clip->direction, &cross) *
        inverse_determinant;
    if (v < 0.0f || u + v > 1.0f) {
        return;
    }
    fraction =
        software_dot_vector(&edge2, &cross) *
        inverse_determinant;
    if (fraction > clip->minimumFraction &&
        fraction < clip->hitFraction) {
        clip->hitFraction = fraction;
    }
}

static int software_clip_camera_strip(
    const JPBJpxPatchSite *site,
    void *user_data)
{
    SoftwareCameraClip *clip =
        (SoftwareCameraClip *)user_data;
    JPBJpxVertex decoded[3];
    FVECTOR vertices[3];
    uint16_t index;

    if (site->vertexCount < 3) {
        return 0;
    }
    for (index = 0; index < 2; ++index) {
        if (jpx_DecodeVertex(
                site, index, &decoded[index]) !=
            JPB_JPX_OK) {
            return 1;
        }
        software_jpx_world_vertex(
            clip->scene,
            &decoded[index],
            &vertices[index]);
    }
    for (index = 2; index < site->vertexCount; ++index) {
        unsigned slot = (unsigned)(index % 3);
        unsigned previous = (unsigned)((index + 2) % 3);
        unsigned before_previous =
            (unsigned)((index + 1) % 3);

        if (jpx_DecodeVertex(
                site, index, &decoded[slot]) !=
            JPB_JPX_OK) {
            return 1;
        }
        software_jpx_world_vertex(
            clip->scene,
            &decoded[slot],
            &vertices[slot]);
        software_clip_camera_triangle(
            clip,
            &vertices[before_previous],
            &vertices[previous],
            &vertices[slot]);
    }
    return 0;
}

int jpb_SoftwareClipCameraToJpx(
    const JPBSoftwareJpxScene *scene,
    const FVECTOR *focus,
    const FVECTOR *desired_eye,
    float padding,
    FVECTOR *clipped_eye,
    float *hit_fraction)
{
    SoftwareCameraClip clip;
    float segment_length;
    float clipped_fraction;
    int result;

    if (scene == NULL || scene->view == NULL ||
        focus == NULL || desired_eye == NULL ||
        clipped_eye == NULL || !(padding >= 0.0f)) {
        return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
    }
    memset(&clip, 0, sizeof(clip));
    clip.scene = scene;
    clip.origin = *focus;
    software_subtract_vector(
        desired_eye, focus, &clip.direction);
    segment_length = sqrtf(
        software_dot_vector(
            &clip.direction, &clip.direction));
    if (!(segment_length > 0.0001f)) {
        *clipped_eye = *desired_eye;
        if (hit_fraction != NULL) {
            *hit_fraction = 1.0f;
        }
        return JPB_SOFTWARE_RENDER_OK;
    }
    /*
     * Ignore intersections at the focus itself. Gameplay focus points can
     * lie exactly on authored contact geometry and must still be able to
     * move the eye away from the actor.
     */
    clip.minimumFraction = 0.001f;
    clip.hitFraction = 1.0f;
    result = jpx_ForEachPatchSite(
        scene->view,
        software_clip_camera_strip,
        &clip);
    if (result != JPB_JPX_OK) {
        return JPB_SOFTWARE_RENDER_JPX_ERROR;
    }
    clipped_fraction = clip.hitFraction;
    if (clip.hitFraction < 1.0f) {
        clipped_fraction -= padding / segment_length;
        if (clipped_fraction < clip.minimumFraction) {
            clipped_fraction = clip.minimumFraction;
        }
    }
    clipped_eye->vx =
        focus->vx + clip.direction.vx * clipped_fraction;
    clipped_eye->vy =
        focus->vy + clip.direction.vy * clipped_fraction;
    clipped_eye->vz =
        focus->vz + clip.direction.vz * clipped_fraction;
    if (hit_fraction != NULL) {
        *hit_fraction = clip.hitFraction;
    }
    return JPB_SOFTWARE_RENDER_OK;
}

static int software_clamp_byte(float value)
{
    if (value < 0.0f) return 0;
    if (value > 255.0f) return 255;
    return (int)value;
}

static int software_map_vertex(
    SoftwareDraw *draw,
    const JPBJpxVertex *vertex,
    int *x,
    int *y)
{
    FVECTOR world;

    if (draw->modelGeometry) {
        world.vx = vertex->x;
        world.vy = vertex->y;
        world.vz = vertex->z;
    } else {
        software_jpx_world_vertex(draw->scene, vertex, &world);
    }
    if (draw->view != NULL) {
        FVECTOR screen;

        if (jpb_ProjectPcToViewport(
                draw->view,
                &world,
                (float)draw->framebuffer->width,
                (float)draw->framebuffer->height,
                &screen) != 0) {
            return 0;
        }
        *x = (int)screen.vx;
        *y = (int)screen.vy;
        return 1;
    }
    *x = (int)(draw->offsetX +
               (world.vx - draw->scene->minX) * draw->scale);
    *y = draw->framebuffer->height - 1 -
         (int)(draw->offsetZ +
               (world.vz - draw->scene->minZ) * draw->scale);
    return 1;
}

static float software_vertex_height(
    const SoftwareDraw *draw, const JPBJpxVertex *vertex)
{
    FVECTOR world;

    if (draw->modelGeometry) {
        return vertex->y;
    }
    software_jpx_world_vertex(draw->scene, vertex, &world);
    return world.vy;
}

static void software_draw_pixel(
    SoftwareDraw *draw, int x, int y, float height)
{
    uint32_t *pixel;
    uint32_t previous;
    int red;
    int green;
    int blue;
    int previous_red;
    int previous_green;
    int previous_blue;

    if (x < 0 || x >= draw->framebuffer->width ||
        y < 0 || y >= draw->framebuffer->height) {
        return;
    }
    if (draw->modelGeometry) {
        red = 255;
        green = software_clamp_byte(176.0f + 64.0f * height);
        blue = 48;
    } else {
        red = software_clamp_byte(48.0f + 207.0f * height);
        green = software_clamp_byte(220.0f - 120.0f * height);
        blue = software_clamp_byte(255.0f - 180.0f * height);
    }
    pixel = draw->framebuffer->pixels +
            (size_t)y * (size_t)draw->framebuffer->stridePixels +
            (size_t)x;
    previous = *pixel;
    previous_red = (int)((previous >> 16) & UINT32_C(0xff));
    previous_green = (int)((previous >> 8) & UINT32_C(0xff));
    previous_blue = (int)(previous & UINT32_C(0xff));
    if (red < previous_red) red = previous_red;
    if (green < previous_green) green = previous_green;
    if (blue < previous_blue) blue = previous_blue;
    *pixel = ((uint32_t)red << 16) |
             ((uint32_t)green << 8) |
             (uint32_t)blue;
    ++draw->stats.pixels;
    if (draw->modelGeometry) {
        ++draw->stats.modelPixels;
    }
}

static int software_line_outcode(
    const SoftwareDraw *draw, int x, int y)
{
    int code = 0;

    if (x < 0) code |= 1;
    if (x >= draw->framebuffer->width) code |= 2;
    if (y < 0) code |= 4;
    if (y >= draw->framebuffer->height) code |= 8;
    return code;
}

static int software_clip_line(
    SoftwareDraw *draw, int *x0, int *y0, int *x1, int *y1)
{
    int code0 = software_line_outcode(draw, *x0, *y0);
    int code1 = software_line_outcode(draw, *x1, *y1);

    for (;;) {
        int code;
        int x;
        int y;

        if ((code0 | code1) == 0) {
            return 1;
        }
        if ((code0 & code1) != 0) {
            return 0;
        }
        code = code0 != 0 ? code0 : code1;
        if ((code & 4) != 0) {
            x = *x0 + (*x1 - *x0) * (0 - *y0) / (*y1 - *y0);
            y = 0;
        } else if ((code & 8) != 0) {
            x = *x0 + (*x1 - *x0) *
                (draw->framebuffer->height - 1 - *y0) / (*y1 - *y0);
            y = draw->framebuffer->height - 1;
        } else if ((code & 2) != 0) {
            y = *y0 + (*y1 - *y0) *
                (draw->framebuffer->width - 1 - *x0) / (*x1 - *x0);
            x = draw->framebuffer->width - 1;
        } else {
            y = *y0 + (*y1 - *y0) * (0 - *x0) / (*x1 - *x0);
            x = 0;
        }
        if (code == code0) {
            *x0 = x;
            *y0 = y;
            code0 = software_line_outcode(draw, *x0, *y0);
        } else {
            *x1 = x;
            *y1 = y;
            code1 = software_line_outcode(draw, *x1, *y1);
        }
    }
}

static void software_draw_line(
    SoftwareDraw *draw,
    const JPBJpxVertex *from,
    const JPBJpxVertex *to,
    float height)
{
    int x0;
    int y0;
    int x1;
    int y1;
    int dx;
    int dy;
    int sx;
    int sy;
    int error;

    if (!software_map_vertex(draw, from, &x0, &y0) ||
        !software_map_vertex(draw, to, &x1, &y1) ||
        !software_clip_line(draw, &x0, &y0, &x1, &y1)) {
        return;
    }
    dx = abs(x1 - x0);
    dy = -abs(y1 - y0);
    sx = x0 < x1 ? 1 : -1;
    sy = y0 < y1 ? 1 : -1;
    error = dx + dy;
    ++draw->stats.lines;
    if (draw->modelGeometry) {
        ++draw->stats.modelLines;
    }
    for (;;) {
        int doubled_error;

        software_draw_pixel(draw, x0, y0, height);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        doubled_error = 2 * error;
        if (doubled_error >= dy) {
            error += dy;
            x0 += sx;
        }
        if (doubled_error <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

static uint32_t software_glow_blend(
    uint32_t destination,
    uint32_t color,
    unsigned coverage)
{
    unsigned alpha = (color >> 24) & UINT32_C(0xff);
    /* fx_screenGlow publishes the matched D3D A8R8G8B8 word. */
    unsigned source_red = (color >> 16) & UINT32_C(0xff);
    unsigned source_green = (color >> 8) & UINT32_C(0xff);
    unsigned source_blue = color & UINT32_C(0xff);
    unsigned destination_red =
        (destination >> 16) & UINT32_C(0xff);
    unsigned destination_green =
        (destination >> 8) & UINT32_C(0xff);
    unsigned destination_blue =
        destination & UINT32_C(0xff);
    unsigned scale = alpha * coverage / UINT32_C(255);

    destination_red += source_red * scale / UINT32_C(255);
    destination_green += source_green * scale / UINT32_C(255);
    destination_blue += source_blue * scale / UINT32_C(255);
    if (destination_red > 255U) destination_red = 255U;
    if (destination_green > 255U) destination_green = 255U;
    if (destination_blue > 255U) destination_blue = 255U;
    return (destination_red << 16) |
           (destination_green << 8) |
           destination_blue;
}

static int software_glow_depth_visible(
    const JPBSoftwareDepthBuffer *depth_buffer,
    int x,
    int y,
    float depth)
{
    size_t depth_offset;

    if (depth_buffer == NULL || depth_buffer->values == NULL) {
        return 1;
    }
    if (x < 0 ||
        y < 0 ||
        (size_t)x >= depth_buffer->width ||
        (size_t)y >= depth_buffer->height ||
        depth_buffer->strideValues < depth_buffer->width) {
        return 0;
    }
    depth_offset =
        (size_t)y * depth_buffer->strideValues + (size_t)x;
    return depth < depth_buffer->values[depth_offset];
}

static void software_draw_glow_disc(
    JPBSoftwareFramebuffer *framebuffer,
    const JPBSoftwareDepthBuffer *depth_buffer,
    int center_x,
    int center_y,
    float depth,
    int radius,
    uint32_t color,
    JPBSoftwareRenderStats *stats)
{
    int y;
    int radius_squared = radius * radius;

    for (y = -radius; y <= radius; ++y) {
        int screen_y = center_y + y;
        int x;

        if (screen_y < 0 || screen_y >= framebuffer->height) {
            continue;
        }
        for (x = -radius; x <= radius; ++x) {
            int distance_squared = x * x + y * y;
            int screen_x = center_x + x;
            unsigned coverage;
            uint32_t *pixel;

            if (distance_squared > radius_squared ||
                screen_x < 0 ||
                screen_x >= framebuffer->width) {
                continue;
            }
            if (!software_glow_depth_visible(
                    depth_buffer, screen_x, screen_y, depth)) {
                if (stats != NULL) {
                    ++stats->glowDepthRejectedPixels;
                }
                continue;
            }
            coverage = radius > 1
                ? (unsigned)(
                    (radius_squared - distance_squared) * 255 /
                    radius_squared)
                : 255U;
            if (coverage < 24U) coverage = 24U;
            pixel = framebuffer->pixels +
                (size_t)screen_y *
                    (size_t)framebuffer->stridePixels +
                (size_t)screen_x;
            *pixel = software_glow_blend(
                *pixel, color, coverage);
            if (stats != NULL) {
                ++stats->pixels;
                ++stats->modelPixels;
            }
        }
    }
}

int jpb_SoftwareDrawGlowLine(
    const _svector *start,
    const _svector *end,
    int world_width,
    uint32_t color,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    const JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats)
{
    const float pdb_glow_depth_bias =
        0.0000016f / 10240.0f;
    FVECTOR world_start;
    FVECTOR world_end;
    FVECTOR radius_point;
    FVECTOR screen_start;
    FVECTOR screen_end;
    FVECTOR screen_radius;
    float radius_x;
    float radius_y;
    float radius_pixels;
    float dx;
    float dy;
    float dz;
    float length;
    int steps;
    int radius;
    int step;

    if (start == NULL || end == NULL ||
        view_matrix == NULL || framebuffer == NULL ||
        framebuffer->pixels == NULL ||
        framebuffer->width <= 0 || framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width ||
        (depth_buffer != NULL &&
         (depth_buffer->values == NULL ||
          depth_buffer->width < (size_t)framebuffer->width ||
          depth_buffer->height < (size_t)framebuffer->height ||
          depth_buffer->strideValues < depth_buffer->width)) ||
        world_width < 0) {
        return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
    }
    world_start.vx = (float)start->vx;
    world_start.vy = (float)start->vy;
    world_start.vz = (float)start->vz;
    world_end.vx = (float)end->vx;
    world_end.vy = (float)end->vy;
    world_end.vz = (float)end->vz;
    if (jpb_ProjectPcToViewport(
            view_matrix,
            &world_start,
            (float)framebuffer->width,
            (float)framebuffer->height,
            &screen_start) != 0 ||
        jpb_ProjectPcToViewport(
            view_matrix,
            &world_end,
            (float)framebuffer->width,
            (float)framebuffer->height,
            &screen_end) != 0 ||
        !(screen_start.vz > 0.0f) ||
        !(screen_end.vz > 0.0f)) {
        return JPB_SOFTWARE_RENDER_OK;
    }

    radius_point.vx =
        world_start.vx + view_matrix->m[0][0] *
            ((float)world_width * 0.5f);
    radius_point.vy =
        world_start.vy + view_matrix->m[0][1] *
            ((float)world_width * 0.5f);
    radius_point.vz =
        world_start.vz + view_matrix->m[0][2] *
            ((float)world_width * 0.5f);
    radius_pixels = 1.0f;
    if (world_width > 0 &&
        jpb_ProjectPcToViewport(
            view_matrix,
            &radius_point,
            (float)framebuffer->width,
            (float)framebuffer->height,
            &screen_radius) == 0) {
        radius_x = screen_radius.vx - screen_start.vx;
        radius_y = screen_radius.vy - screen_start.vy;
        radius_pixels = sqrtf(
            radius_x * radius_x + radius_y * radius_y);
    }
    if (radius_pixels < 1.0f) radius_pixels = 1.0f;
    if (radius_pixels > 48.0f) radius_pixels = 48.0f;
    radius = (int)(radius_pixels + 0.5f);
    dx = screen_end.vx - screen_start.vx;
    dy = screen_end.vy - screen_start.vy;
    dz = screen_end.vz - screen_start.vz;
    length = sqrtf(dx * dx + dy * dy);
    steps = (int)(length + 0.5f);
    if (steps < 1) steps = 1;
    for (step = 0; step <= steps; ++step) {
        float fraction = (float)step / (float)steps;
        float depth =
            screen_start.vz + dz * fraction -
            pdb_glow_depth_bias;

        software_draw_glow_disc(
            framebuffer,
            depth_buffer,
            (int)(screen_start.vx + dx * fraction + 0.5f),
            (int)(screen_start.vy + dy * fraction + 0.5f),
            depth,
            radius,
            color,
            stats);
    }
    if (stats != NULL) {
        ++stats->lines;
        ++stats->modelLines;
    }
    return JPB_SOFTWARE_RENDER_OK;
}

int jpb_SoftwareDrawScreenPoly(
    const _Material *material,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats)
{
    SoftwareModelDraw state;
    SoftwareMaterialVertex projected[
        SOFTWARE_SCREEN_POLY_VERTEX_CAPACITY];
    const JPBSoftwareTexture *texture = NULL;
    MATRIX perspective_marker;
    size_t triangle_count;
    int vertex;

    if (vertices == NULL || framebuffer == NULL ||
        framebuffer->pixels == NULL ||
        framebuffer->width <= 0 || framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width ||
        depth_buffer == NULL || depth_buffer->values == NULL ||
        depth_buffer->width < (size_t)framebuffer->width ||
        depth_buffer->height < (size_t)framebuffer->height ||
        depth_buffer->strideValues < depth_buffer->width ||
        vertex_count < 3 ||
        vertex_count > SOFTWARE_SCREEN_POLY_VERTEX_CAPACITY) {
        return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
    }

    memset(&state, 0, sizeof(state));
    memset(&perspective_marker, 0, sizeof(perspective_marker));
    perspective_marker.m[0][0] = 1.0f;
    perspective_marker.m[1][1] = 1.0f;
    perspective_marker.m[2][2] = 1.0f;
    state.draw.framebuffer = framebuffer;
    /*
     * software_draw_material_triangle uses a non-null view only to select
     * perspective-correct interpolation. The exact camera-space projection
     * is performed below because fx_Water has already applied CameraMatrix.
     */
    state.draw.view = no_scale != 0 ? &perspective_marker : NULL;
    state.depthBuffer = depth_buffer->values;
    state.depthStride = depth_buffer->strideValues;
    state.repeatTexture = 1;
    state.blendTextureAlpha = 0;
    state.depthWrite = 1;
    state.materialShader = SOFTWARE_MATERIAL_SHADER_LEVEL_OPAQUE;
    state.jpxPass = SOFTWARE_JPX_PASS_OPAQUE;

    if (material != NULL && material->texture != NULL) {
        const JPBSoftwareTexture *candidate =
            (const JPBSoftwareTexture *)material->texture;

        if (candidate->pixels != NULL &&
            candidate->width != 0 && candidate->height != 0 &&
            candidate->stridePixels >= candidate->width) {
            texture = candidate;
        }
    }
    /*
     * Exact el_chavo::StartPoly checks Texture2D::type: ordinary textures
     * (0) are submitted immediately, while p_ (1) and a_ (2) textures are
     * queued as transparent polygons. Preserve the defining alpha behavior
     * here; a_SMOKEGRY's authored 0..221 alpha otherwise became an opaque
     * rectangle in the FED introduction.
     */
    state.blendTextureAlpha =
        texture != NULL && texture->materialType != 0;

    for (vertex = 0; vertex < vertex_count; ++vertex) {
        const JPBScreenPolyVertex *source = &vertices[vertex];
        SoftwareMaterialVertex *destination = &projected[vertex];

        if (no_scale != 0) {
            FVECTOR camera_space;
            FVECTOR screen;

            /* fx_Water and the other NoScale owners reject at z == 1. */
            camera_space.vx = source->x;
            camera_space.vy = source->y;
            camera_space.vz = source->z;
            if (!(source->z > 1.0f) ||
                jpb_ProjectPcToViewport(
                    &perspective_marker,
                    &camera_space,
                    (float)framebuffer->width,
                    (float)framebuffer->height,
                    &screen) != 0) {
                return JPB_SOFTWARE_RENDER_OK;
            }
            destination->x = screen.vx;
            destination->y = screen.vy;
            destination->depth = screen.vz;
            destination->inverseDepth =
                1.0f / destination->depth;
        } else {
            destination->x = source->x;
            destination->y = source->y;
            destination->depth = source->z;
            destination->inverseDepth = 1.0f;
        }
        destination->u = source->tu;
        destination->v = source->tv;
        destination->red =
            (float)((source->argb >> 16) & UINT32_C(0xff));
        destination->green =
            (float)((source->argb >> 8) & UINT32_C(0xff));
        destination->blue =
            (float)(source->argb & UINT32_C(0xff));
        destination->alpha = (float)(source->argb >> 24);
    }

    if (material != NULL &&
        material->flags == JPB_MATERIAL_MODE_SCREEN_TILE) {
        for (vertex = 0; vertex < vertex_count; ++vertex) {
            projected[vertex].depth = 0.0001f;
            projected[vertex].inverseDepth = 10000.0f;
        }
    }

    triangle_count = (size_t)vertex_count - 2u;
    state.draw.stats.triangles = triangle_count;
    for (vertex = 1; vertex + 1 < vertex_count; ++vertex) {
        const SoftwareMaterialVertex *first =
            &projected[vertex - 1];
        const SoftwareMaterialVertex *second =
            &projected[vertex];
        const SoftwareMaterialVertex *third =
            &projected[vertex + 1];
        SoftwareMaterialVertex winding_vertices[3];

        /* _StartPoly's submitted vertices form the retail triangle strip. */
        if ((vertex & 1) == 0) {
            const SoftwareMaterialVertex *swap = first;

            first = second;
            second = swap;
        }
        winding_vertices[0] = *first;
        winding_vertices[1] = *second;
        winding_vertices[2] = *third;
        if ((material == NULL ||
             material->flags == JPB_MATERIAL_MODE_BACKFACE_REJECT) &&
            software_material_polygon_winding(
                winding_vertices, 3) < 0.0f) {
            continue;
        }
        software_draw_material_triangle(
            &state,
            first,
            second,
            third,
            texture);
    }
    if (stats != NULL) {
        stats->triangles += triangle_count;
        stats->pixels += state.draw.stats.pixels;
    }
    return JPB_SOFTWARE_RENDER_OK;
}

static int software_same_position(
    const JPBJpxVertex *left, const JPBJpxVertex *right)
{
    return left->x == right->x &&
           left->z == right->z &&
           left->y == right->y;
}

static int software_draw_strip(
    const JPBJpxPatchSite *site, void *user_data)
{
    SoftwareDraw *draw = (SoftwareDraw *)user_data;
    JPBJpxVertex first;
    JPBJpxVertex second;
    float span_y = draw->scene->maxY - draw->scene->minY;
    uint16_t index;

    if (site->vertexCount == 0) {
        return 0;
    }
    if (span_y <= 0.0f) {
        span_y = 1.0f;
    }
    if (jpx_DecodeVertex(site, 0, &first) != JPB_JPX_OK) {
        return 1;
    }
    if (site->vertexCount == 1) {
        int x;
        int y;

        if (software_map_vertex(draw, &first, &x, &y)) {
            software_draw_pixel(
                draw, x, y,
                (software_vertex_height(draw, &first) -
                 draw->scene->minY) / span_y);
        }
        return 0;
    }
    if (jpx_DecodeVertex(site, 1, &second) != JPB_JPX_OK) {
        return 1;
    }
    software_draw_line(
        draw,
        &first,
        &second,
        ((software_vertex_height(draw, &first) +
          software_vertex_height(draw, &second)) * 0.5f -
         draw->scene->minY) /
            span_y);
    for (index = 2; index < site->vertexCount; ++index) {
        JPBJpxVertex third;
        float height;

        if (jpx_DecodeVertex(site, index, &third) != JPB_JPX_OK) {
            return 1;
        }
        height =
            ((software_vertex_height(draw, &first) +
              software_vertex_height(draw, &second) +
              software_vertex_height(draw, &third)) / 3.0f -
             draw->scene->minY) /
            span_y;
        software_draw_line(draw, &second, &third, height);
        if (!software_same_position(&first, &second) &&
            !software_same_position(&second, &third) &&
            !software_same_position(&first, &third)) {
            software_draw_line(draw, &third, &first, height);
            ++draw->stats.triangles;
        }
        first = second;
        second = third;
    }
    return 0;
}

int jpb_SoftwareRenderJpxWireframe(
    const JPBSoftwareJpxScene *scene,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color,
    JPBSoftwareRenderStats *stats)
{
    SoftwareDraw draw;
    float span_x;
    float span_z;
    float scale_x;
    float scale_z;
    float used_width;
    float used_height;
    int y;
    int result;

    if (scene == NULL || scene->view == NULL ||
        framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->width <= 0 || framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width) {
        return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
    }
    for (y = 0; y < framebuffer->height; ++y) {
        uint32_t *row = framebuffer->pixels +
                        (size_t)y * (size_t)framebuffer->stridePixels;
        int x;

        for (x = 0; x < framebuffer->width; ++x) {
            row[x] = clear_color;
        }
    }
    memset(&draw, 0, sizeof(draw));
    draw.scene = scene;
    draw.framebuffer = framebuffer;
    draw.view = view_matrix;

    span_x = scene->maxX - scene->minX;
    span_z = scene->maxZ - scene->minZ;
    if (span_x <= 0.0f) span_x = 1.0f;
    if (span_z <= 0.0f) span_z = 1.0f;
    scale_x =
        (float)(framebuffer->width - 2 * SOFTWARE_RENDER_MARGIN - 1) /
        span_x;
    scale_z =
        (float)(framebuffer->height - 2 * SOFTWARE_RENDER_MARGIN - 1) /
        span_z;
    draw.scale = scale_x < scale_z ? scale_x : scale_z;
    used_width = span_x * draw.scale;
    used_height = span_z * draw.scale;
    draw.offsetX = ((float)framebuffer->width - used_width) * 0.5f;
    draw.offsetZ = ((float)framebuffer->height - used_height) * 0.5f;

    result = jpx_ForEachPatchSite(
        scene->view, software_draw_strip, &draw);
    if (result != JPB_JPX_OK) {
        return JPB_SOFTWARE_RENDER_JPX_ERROR;
    }
    if (stats != NULL) {
        *stats = draw.stats;
    }
    return JPB_SOFTWARE_RENDER_OK;
}

static void software_model_vertex(
    const FVECTOR *source, JPBJpxVertex *destination)
{
    memset(destination, 0, sizeof(*destination));
    destination->x = source->vx;
    destination->y = source->vy;
    destination->z = source->vz;
}

static int software_project_material_vertex(
    SoftwareModelDraw *state,
    const FVECTOR *world,
    const pairUV *uv,
    const CVECTOR *color,
    SoftwareMaterialVertex *vertex)
{
    if (state->draw.view != NULL) {
        FVECTOR screen;

        if (jpb_ProjectPcToViewport(
                state->draw.view,
                world,
                (float)state->draw.framebuffer->width,
                (float)state->draw.framebuffer->height,
                &screen) != 0 ||
            !(screen.vz > 0.0f)) {
            return 0;
        }
        vertex->x = screen.vx;
        vertex->y = screen.vy;
        vertex->depth = screen.vz;
        vertex->inverseDepth = 1.0f / screen.vz;
    } else {
        vertex->x =
            state->draw.offsetX +
            (world->vx - state->draw.scene->minX) *
                state->draw.scale;
        vertex->y =
            (float)state->draw.framebuffer->height - 1.0f -
            (state->draw.offsetZ +
             (world->vz - state->draw.scene->minZ) *
                 state->draw.scale);
        vertex->depth = -world->vy;
        vertex->inverseDepth = 1.0f;
    }
    vertex->u = uv->u;
    vertex->v = uv->v;
    vertex->red = (float)color->r;
    vertex->green = (float)color->g;
    vertex->blue = (float)color->b;
    /*
     * _RenderNode publishes 0xffRRGGBB through _SetVert regardless of the
     * source CVECTOR code byte. JPX level vertices retain all four packed
     * bytes for LevelVertexShader.hlsl.
     */
    vertex->alpha =
        state->materialShader ==
                SOFTWARE_MATERIAL_SHADER_MODEL
            ? 255.0f
            : (float)color->cd;
    return 1;
}

static void software_camera_material_vertex(
    SoftwareModelDraw *state,
    const FVECTOR *world,
    const pairUV *uv,
    const CVECTOR *color,
    SoftwareCameraMaterialVertex *vertex)
{
    FVECTOR source = *world;

    if (state->wrapLevelWorld) {
        source.vx = software_wrap_level_coordinate(
            source.vx, state->levelCameraOrigin.vx);
        source.vy = software_wrap_level_coordinate(
            source.vy, state->levelCameraOrigin.vy);
        source.vz = software_wrap_level_coordinate(
            source.vz, state->levelCameraOrigin.vz);
    }

    fApplyMatrixFV(
        state->draw.view, &source, &vertex->position);
    vertex->position.vx +=
        (float)state->draw.view->t[0];
    vertex->position.vy +=
        (float)state->draw.view->t[1];
    vertex->position.vz +=
        (float)state->draw.view->t[2];
    vertex->u = uv->u;
    vertex->v = uv->v;
    vertex->red = (float)color->r;
    vertex->green = (float)color->g;
    vertex->blue = (float)color->b;
    vertex->alpha =
        state->materialShader ==
                SOFTWARE_MATERIAL_SHADER_MODEL
            ? 255.0f
            : (float)color->cd;
}

static void software_lerp_camera_material_vertex(
    const SoftwareCameraMaterialVertex *from,
    const SoftwareCameraMaterialVertex *to,
    float fraction,
    SoftwareCameraMaterialVertex *result)
{
#define SOFTWARE_LERP_FIELD(field)                                           \
    result->field = from->field +                                           \
        (to->field - from->field) * fraction
    SOFTWARE_LERP_FIELD(position.vx);
    SOFTWARE_LERP_FIELD(position.vy);
    SOFTWARE_LERP_FIELD(position.vz);
    SOFTWARE_LERP_FIELD(u);
    SOFTWARE_LERP_FIELD(v);
    SOFTWARE_LERP_FIELD(red);
    SOFTWARE_LERP_FIELD(green);
    SOFTWARE_LERP_FIELD(blue);
    SOFTWARE_LERP_FIELD(alpha);
#undef SOFTWARE_LERP_FIELD
}

static size_t software_clip_project_material_polygon(
    SoftwareModelDraw *state,
    const FVECTOR *world,
    const pairUV *uv,
    const CVECTOR *color,
    size_t vertex_count,
    SoftwareMaterialVertex *projected)
{
    SoftwareCameraMaterialVertex camera[4];
    SoftwareCameraMaterialVertex clipped[
        SOFTWARE_CLIPPED_POLYGON_CAPACITY];
    size_t clipped_count = 0;
    size_t vertex;

    if (vertex_count < 3 || vertex_count > 4) {
        return 0;
    }
    if (state->draw.view == NULL) {
        for (vertex = 0; vertex < vertex_count; ++vertex) {
            if (!software_project_material_vertex(
                    state,
                    &world[vertex],
                    &uv[vertex],
                    &color[vertex],
                    &projected[vertex])) {
                return 0;
            }
        }
        return vertex_count;
    }

    for (vertex = 0; vertex < vertex_count; ++vertex) {
        software_camera_material_vertex(
            state,
            &world[vertex],
            &uv[vertex],
            &color[vertex],
            &camera[vertex]);
    }
    for (vertex = 0; vertex < vertex_count; ++vertex) {
        const SoftwareCameraMaterialVertex *from =
            &camera[(vertex + vertex_count - 1) % vertex_count];
        const SoftwareCameraMaterialVertex *to = &camera[vertex];
        int from_inside = from->position.vz >= 1.0f;
        int to_inside = to->position.vz >= 1.0f;

        if (from_inside != to_inside) {
            float fraction =
                (1.0f - from->position.vz) /
                (to->position.vz - from->position.vz);

            if (clipped_count >=
                SOFTWARE_CLIPPED_POLYGON_CAPACITY) {
                return 0;
            }
            software_lerp_camera_material_vertex(
                from,
                to,
                fraction,
                &clipped[clipped_count]);
            clipped[clipped_count].position.vz = 1.0f;
            ++clipped_count;
        }
        if (to_inside) {
            if (clipped_count >=
                SOFTWARE_CLIPPED_POLYGON_CAPACITY) {
                return 0;
            }
            clipped[clipped_count++] = *to;
        }
    }
    for (vertex = 0; vertex < clipped_count; ++vertex) {
        FVECTOR screen;

        if (jpb_ProjectPcCameraToViewport(
                &clipped[vertex].position,
                (float)state->draw.framebuffer->width,
                (float)state->draw.framebuffer->height,
                &screen) != 0) {
            return 0;
        }
        projected[vertex].x = screen.vx;
        projected[vertex].y = screen.vy;
        projected[vertex].depth = screen.vz;
        projected[vertex].inverseDepth = 1.0f / screen.vz;
        projected[vertex].u = clipped[vertex].u;
        projected[vertex].v = clipped[vertex].v;
        projected[vertex].red = clipped[vertex].red;
        projected[vertex].green = clipped[vertex].green;
        projected[vertex].blue = clipped[vertex].blue;
        projected[vertex].alpha = clipped[vertex].alpha;
    }
    return clipped_count;
}

static float software_edge(
    const SoftwareMaterialVertex *from,
    const SoftwareMaterialVertex *to,
    float x,
    float y)
{
    return (x - from->x) * (to->y - from->y) -
           (y - from->y) * (to->x - from->x);
}

static float software_material_polygon_winding(
    const SoftwareMaterialVertex *vertices,
    size_t vertex_count)
{
    float winding = 0.0f;
    size_t vertex;

    for (vertex = 0; vertex < vertex_count; ++vertex) {
        size_t next = (vertex + 1) % vertex_count;

        winding +=
            vertices[vertex].x * vertices[next].y -
            vertices[vertex].y * vertices[next].x;
    }
    return winding;
}

static float software_clamp_unit(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static uint32_t software_texture_texel(
    const JPBSoftwareTexture *texture, size_t x, size_t y)
{
    return texture->pixels[y * texture->stridePixels + x];
}

static uint32_t software_lerp_channel(
    uint32_t top_left,
    uint32_t top_right,
    uint32_t bottom_left,
    uint32_t bottom_right,
    unsigned shift,
    float fraction_x,
    float fraction_y)
{
    float top =
        (float)((top_left >> shift) & UINT32_C(0xff)) +
        ((float)((top_right >> shift) & UINT32_C(0xff)) -
         (float)((top_left >> shift) & UINT32_C(0xff))) *
            fraction_x;
    float bottom =
        (float)((bottom_left >> shift) & UINT32_C(0xff)) +
        ((float)((bottom_right >> shift) & UINT32_C(0xff)) -
         (float)((bottom_left >> shift) & UINT32_C(0xff))) *
            fraction_x;

    return (uint32_t)software_clamp_byte(
        top + (bottom - top) * fraction_y);
}

static uint32_t software_sample_texture(
    const JPBSoftwareTexture *texture,
    float u,
    float v,
    int repeat)
{
    float sample_x;
    float sample_y;
    size_t x0;
    size_t y0;
    size_t x1;
    size_t y1;
    float fraction_x;
    float fraction_y;
    uint32_t top_left;
    uint32_t top_right;
    uint32_t bottom_left;
    uint32_t bottom_right;
    uint32_t alpha;
    uint32_t red;
    uint32_t green;
    uint32_t blue;

    if (texture == NULL || texture->pixels == NULL ||
        texture->width == 0 || texture->height == 0 ||
        texture->stridePixels < texture->width) {
        return UINT32_C(0xffffffff);
    }
    if (repeat) {
        u -= floorf(u);
        v -= floorf(v);
        sample_x = u * (float)texture->width;
        sample_y = v * (float)texture->height;
        x0 = (size_t)sample_x % texture->width;
        y0 = (size_t)sample_y % texture->height;
        x1 = (x0 + 1u) % texture->width;
        y1 = (y0 + 1u) % texture->height;
    } else {
        sample_x =
            software_clamp_unit(u) *
            (float)(texture->width - 1u);
        sample_y =
            software_clamp_unit(v) *
            (float)(texture->height - 1u);
        x0 = (size_t)sample_x;
        y0 = (size_t)sample_y;
        x1 = x0 + 1u < texture->width ? x0 + 1u : x0;
        y1 = y0 + 1u < texture->height ? y0 + 1u : y0;
    }
    fraction_x = sample_x - (float)x0;
    fraction_y = sample_y - (float)y0;
    top_left = software_texture_texel(texture, x0, y0);
    top_right = software_texture_texel(texture, x1, y0);
    bottom_left = software_texture_texel(texture, x0, y1);
    bottom_right = software_texture_texel(texture, x1, y1);
    alpha = software_lerp_channel(
        top_left,
        top_right,
        bottom_left,
        bottom_right,
        24,
        fraction_x,
        fraction_y);
    red = software_lerp_channel(
        top_left,
        top_right,
        bottom_left,
        bottom_right,
        16,
        fraction_x,
        fraction_y);
    green = software_lerp_channel(
        top_left,
        top_right,
        bottom_left,
        bottom_right,
        8,
        fraction_x,
        fraction_y);
    blue = software_lerp_channel(
        top_left,
        top_right,
        bottom_left,
        bottom_right,
        0,
        fraction_x,
        fraction_y);
    return (alpha << 24) |
           (red << 16) |
           (green << 8) |
           blue;
}

static void software_store_material_pixel(
    SoftwareModelDraw *state,
    int x,
    int y,
    float depth,
    uint32_t texture_color,
    float red,
    float green,
    float blue,
    float alpha)
{
    size_t framebuffer_offset =
        (size_t)y *
            (size_t)state->draw.framebuffer->stridePixels +
        (size_t)x;
    size_t depth_offset =
        (size_t)y * state->depthStride + (size_t)x;
    uint32_t source_alpha =
        (texture_color >> 24) & UINT32_C(0xff);
    uint32_t source_red =
        (texture_color >> 16) & UINT32_C(0xff);
    uint32_t source_green =
        (texture_color >> 8) & UINT32_C(0xff);
    uint32_t source_blue =
        texture_color & UINT32_C(0xff);
    uint32_t destination;

    if (depth >= state->depthBuffer[depth_offset]) {
        return;
    }
    if (state->materialShader ==
        SOFTWARE_MATERIAL_SHADER_MODEL) {
        /*
         * Shipped PixelShader.hlsl applies vertex modulation only when
         * every interpolated CVECTOR component is nonzero, then discards
         * black RGB regardless of texture alpha.
         */
        if (source_red == 0 &&
            source_green == 0 &&
            source_blue == 0) {
            return;
        }
        if (red > 0.0f && green > 0.0f &&
            blue > 0.0f && alpha > 0.0f) {
            source_red =
                (source_red *
                 (uint32_t)software_clamp_byte(red)) /
                UINT32_C(255);
            source_green =
                (source_green *
                 (uint32_t)software_clamp_byte(green)) /
                UINT32_C(255);
            source_blue =
                (source_blue *
                 (uint32_t)software_clamp_byte(blue)) /
                UINT32_C(255);
        }
    } else {
        float minimum_color = 0.1f * 255.0f;

        /*
         * LevelPixelShader.hlsl discards only an all-zero sampled texel,
         * clamps vertex RGB to 0.1, and multiplies the complete RGBA
         * sample by the interpolated vertex color.
         */
        if (source_alpha == 0 &&
            source_red == 0 &&
            source_green == 0 &&
            source_blue == 0) {
            return;
        }
        if (state->materialShader ==
                SOFTWARE_MATERIAL_SHADER_LEVEL_TRANSPARENT &&
            source_alpha < UINT32_C(26)) {
            return;
        }
        if (red < minimum_color) red = minimum_color;
        if (green < minimum_color) green = minimum_color;
        if (blue < minimum_color) blue = minimum_color;
        source_red =
            (source_red *
             (uint32_t)software_clamp_byte(red)) /
            UINT32_C(255);
        source_green =
            (source_green *
             (uint32_t)software_clamp_byte(green)) /
            UINT32_C(255);
        source_blue =
            (source_blue *
             (uint32_t)software_clamp_byte(blue)) /
            UINT32_C(255);
        source_alpha =
            (source_alpha *
             (uint32_t)software_clamp_byte(alpha)) /
            UINT32_C(255);
    }
    destination =
        state->draw.framebuffer->pixels[
            framebuffer_offset];
    if (state->blendTextureAlpha &&
        source_alpha < UINT32_C(255)) {
        uint32_t inverse_alpha =
            UINT32_C(255) - source_alpha;

        source_red =
            (source_red * source_alpha +
             ((destination >> 16) & UINT32_C(0xff)) *
                 inverse_alpha) /
            UINT32_C(255);
        source_green =
            (source_green * source_alpha +
             ((destination >> 8) & UINT32_C(0xff)) *
                 inverse_alpha) /
            UINT32_C(255);
        source_blue =
            (source_blue * source_alpha +
             (destination & UINT32_C(0xff)) *
                 inverse_alpha) /
            UINT32_C(255);
    }
    state->draw.framebuffer->pixels[
        framebuffer_offset] =
        (source_red << 16) |
        (source_green << 8) |
        source_blue;
    if (state->depthWrite) {
        state->depthBuffer[depth_offset] = depth;
    }
    ++state->draw.stats.pixels;
    if (state->jpxPass != SOFTWARE_JPX_PASS_OPAQUE &&
        !state->draw.modelGeometry) {
        ++state->draw.stats.levelTransparentPixels;
        if (state->jpxPass == SOFTWARE_JPX_PASS_GLASS) {
            ++state->draw.stats.levelGlassPixels;
        }
    }
    if (state->draw.modelGeometry) {
        ++state->draw.stats.modelPixels;
    }
}

static void software_draw_material_triangle(
    SoftwareModelDraw *state,
    const SoftwareMaterialVertex *first,
    const SoftwareMaterialVertex *second,
    const SoftwareMaterialVertex *third,
    const JPBSoftwareTexture *texture)
{
    float area = software_edge(first, second, third->x, third->y);
    float minimum_x;
    float maximum_x;
    float minimum_y;
    float maximum_y;
    int x0;
    int x1;
    int y0;
    int y1;
    int y;

    if (area == 0.0f) {
        return;
    }
    minimum_x = first->x;
    maximum_x = first->x;
    minimum_y = first->y;
    maximum_y = first->y;
    if (second->x < minimum_x) minimum_x = second->x;
    if (second->x > maximum_x) maximum_x = second->x;
    if (third->x < minimum_x) minimum_x = third->x;
    if (third->x > maximum_x) maximum_x = third->x;
    if (second->y < minimum_y) minimum_y = second->y;
    if (second->y > maximum_y) maximum_y = second->y;
    if (third->y < minimum_y) minimum_y = third->y;
    if (third->y > maximum_y) maximum_y = third->y;
    x0 = (int)floorf(minimum_x);
    x1 = (int)ceilf(maximum_x);
    y0 = (int)floorf(minimum_y);
    y1 = (int)ceilf(maximum_y);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= state->draw.framebuffer->width) {
        x1 = state->draw.framebuffer->width - 1;
    }
    if (y1 >= state->draw.framebuffer->height) {
        y1 = state->draw.framebuffer->height - 1;
    }
    for (y = y0; y <= y1; ++y) {
        int x;

        for (x = x0; x <= x1; ++x) {
            float sample_x = (float)x + 0.5f;
            float sample_y = (float)y + 0.5f;
            float weight_first =
                software_edge(second, third, sample_x, sample_y);
            float weight_second =
                software_edge(third, first, sample_x, sample_y);
            float weight_third =
                software_edge(first, second, sample_x, sample_y);
            float inverse_depth;
            float u;
            float v;
            float red;
            float green;
            float blue;
            float alpha;
            float depth;
            uint32_t texture_color;

            if ((area > 0.0f &&
                 (weight_first < 0.0f ||
                  weight_second < 0.0f ||
                  weight_third < 0.0f)) ||
                (area < 0.0f &&
                 (weight_first > 0.0f ||
                  weight_second > 0.0f ||
                  weight_third > 0.0f))) {
                continue;
            }
            weight_first /= area;
            weight_second /= area;
            weight_third /= area;
            inverse_depth =
                weight_first * first->inverseDepth +
                weight_second * second->inverseDepth +
                weight_third * third->inverseDepth;
            if (!(inverse_depth > 0.0f)) {
                continue;
            }
            if (state->draw.view != NULL) {
                depth = 1.0f / inverse_depth;
            } else {
                depth =
                    weight_first * first->depth +
                    weight_second * second->depth +
                    weight_third * third->depth;
            }
            u =
                (weight_first * first->u * first->inverseDepth +
                 weight_second * second->u * second->inverseDepth +
                 weight_third * third->u * third->inverseDepth) /
                inverse_depth;
            v =
                (weight_first * first->v * first->inverseDepth +
                 weight_second * second->v * second->inverseDepth +
                 weight_third * third->v * third->inverseDepth) /
                inverse_depth;
            red =
                (weight_first * first->red *
                     first->inverseDepth +
                 weight_second * second->red *
                     second->inverseDepth +
                 weight_third * third->red *
                     third->inverseDepth) /
                inverse_depth;
            green =
                (weight_first * first->green *
                     first->inverseDepth +
                 weight_second * second->green *
                     second->inverseDepth +
                 weight_third * third->green *
                     third->inverseDepth) /
                inverse_depth;
            blue =
                (weight_first * first->blue *
                     first->inverseDepth +
                 weight_second * second->blue *
                     second->inverseDepth +
                 weight_third * third->blue *
                     third->inverseDepth) /
                inverse_depth;
            alpha =
                (weight_first * first->alpha *
                     first->inverseDepth +
                 weight_second * second->alpha *
                     second->inverseDepth +
                 weight_third * third->alpha *
                     third->inverseDepth) /
                inverse_depth;
            texture_color =
                software_sample_texture(
                    texture,
                    u,
                    v,
                    state->repeatTexture);
            software_store_material_pixel(
                state,
                x,
                y,
                depth,
                texture_color,
                red,
                green,
                blue,
                alpha);
        }
    }
}

static int software_material_triangle_on_screen(
    const SoftwareModelDraw *state,
    const SoftwareMaterialVertex *first,
    const SoftwareMaterialVertex *second,
    const SoftwareMaterialVertex *third)
{
    float minimum_x = first->x;
    float maximum_x = first->x;
    float minimum_y = first->y;
    float maximum_y = first->y;

    if (second->x < minimum_x) minimum_x = second->x;
    if (second->x > maximum_x) maximum_x = second->x;
    if (third->x < minimum_x) minimum_x = third->x;
    if (third->x > maximum_x) maximum_x = third->x;
    if (second->y < minimum_y) minimum_y = second->y;
    if (second->y > maximum_y) maximum_y = second->y;
    if (third->y < minimum_y) minimum_y = third->y;
    if (third->y > maximum_y) maximum_y = third->y;
    return maximum_x >= 0.0f &&
           maximum_y >= 0.0f &&
           minimum_x <
               (float)state->draw.framebuffer->width &&
           minimum_y <
               (float)state->draw.framebuffer->height;
}

static void software_jpx_material_vertex(
    const SoftwareModelDraw *state,
    const JPBJpxVertex *source,
    FVECTOR *world,
    pairUV *uv,
    CVECTOR *color)
{
    software_jpx_world_vertex(
        state->draw.scene, source, world);
    uv->u = source->u;
    uv->v = source->v;
    color->r =
        (uint8_t)(source->attributes &
                  UINT32_C(0xff));
    color->g =
        (uint8_t)((source->attributes >> 8) &
                  UINT32_C(0xff));
    color->b =
        (uint8_t)((source->attributes >> 16) &
                  UINT32_C(0xff));
    color->cd =
        (uint8_t)(source->attributes >> 24);
}

static int software_draw_material_strip(
    const JPBJpxPatchSite *site, void *user_data)
{
    SoftwareModelDraw *state =
        (SoftwareModelDraw *)user_data;
    JPBSoftwareTexture texture;
    const JPBSoftwareTexture *resolved_texture = NULL;
    JPBJpxVertex first;
    JPBJpxVertex second;
    int texture_attempted = 0;
    int8_t list_type;
    int is_transparent;
    int is_glass;
    const char *material_name;
    uint16_t index;

    if (site->vertexCount < 3) {
        return 0;
    }
    if (site->materialIndex >=
        state->draw.scene->view->numMaterials) {
        return 1;
    }
    list_type = jpx_GetMaterialListType(
        state->draw.scene->view,
        (uint16_t)site->materialIndex);
    material_name = jpx_GetMaterialName(
        state->draw.scene->view,
        (uint16_t)site->materialIndex);
    if (state->draw.scene->levelIndex != JPB_LEVEL_INDEX_NONE &&
        state->draw.scene->fbxMaterialMatches != 0) {
        /*
         * The live FBX loader uses isTextureTransparent rather than the
         * legacy JPX list type. The bridge resolves the mirror's generated
         * 8.3 IDs back to the exact per-level database.
         */
        is_transparent =
            jpb_IsTextureTransparentForJpxMirror(
                material_name, state->draw.scene->levelIndex);
    } else {
        /* Format-inspection scenes have no level identity. */
        is_transparent = list_type == 8;
    }
    is_glass =
        is_transparent &&
        (state->draw.scene->levelIndex != JPB_LEVEL_INDEX_NONE &&
         state->draw.scene->fbxMaterialMatches != 0
             ? jpb_IsTextureGlassForJpxMirror(
                   material_name, state->draw.scene->levelIndex)
             : jpb_IsGlassTextureForJpxMirror(material_name));
    if ((state->jpxPass == SOFTWARE_JPX_PASS_OPAQUE &&
         is_transparent) ||
        (state->jpxPass == SOFTWARE_JPX_PASS_TRANSPARENT &&
         (!is_transparent || is_glass)) ||
        (state->jpxPass == SOFTWARE_JPX_PASS_GLASS &&
         !is_glass)) {
        return 0;
    }
    if (jpx_DecodeVertex(site, 0, &first) != JPB_JPX_OK ||
        jpx_DecodeVertex(site, 1, &second) != JPB_JPX_OK) {
        return 1;
    }
    memset(&texture, 0, sizeof(texture));
    texture.colorOverride = -1;

    for (index = 2; index < site->vertexCount; ++index) {
        JPBJpxVertex third;

        if (jpx_DecodeVertex(
                site, index, &third) != JPB_JPX_OK) {
            return 1;
        }
        if (!software_same_position(&first, &second) &&
            !software_same_position(&second, &third) &&
            !software_same_position(&first, &third)) {
            const JPBJpxVertex *source_vertices[3];
            FVECTOR world_vertices[3];
            pairUV uv_vertices[3];
            CVECTOR color_vertices[3];
            SoftwareMaterialVertex projected[
                SOFTWARE_CLIPPED_POLYGON_CAPACITY];
            size_t projected_count;
            size_t corner;
            uint32_t streets_cull_mask =
                state->draw.scene->streetsCullMapReady
                    ? jpb_StreetsJpxTriangleCullMask(
                          site, (uint16_t)(index - 2))
                    : 0;

            if (!jpb_StreetsJpxCullMaskVisible(streets_cull_mask)) {
                ++state->draw.stats.levelCulledTriangles;
                first = second;
                second = third;
                continue;
            }
            ++state->draw.stats.triangles;
            if (state->jpxPass !=
                SOFTWARE_JPX_PASS_OPAQUE) {
                ++state->draw.stats.levelTransparentTriangles;
                if (state->jpxPass ==
                    SOFTWARE_JPX_PASS_GLASS) {
                    ++state->draw.stats.levelGlassTriangles;
                }
            }
            if ((index & 1U) != 0) {
                source_vertices[0] = &second;
                source_vertices[1] = &first;
            } else {
                source_vertices[0] = &first;
                source_vertices[1] = &second;
            }
            source_vertices[2] = &third;
            for (corner = 0; corner < 3; ++corner) {
                software_jpx_material_vertex(
                    state,
                    source_vertices[corner],
                    &world_vertices[corner],
                    &uv_vertices[corner],
                    &color_vertices[corner]);
            }
            projected_count =
                software_clip_project_material_polygon(
                    state,
                    world_vertices,
                    uv_vertices,
                    color_vertices,
                    3,
                    projected);
            /*
             * CD3DApplication::CreatePipelineStateObject (RVA 0x315E0)
             * publishes D3D12_CULL_MODE_NONE for the level PSO. Preserve
             * both strip windings here; actor materials have their own
             * recovered winding policy in the model path above.
             */
            for (corner = 1;
                 corner + 1 < projected_count;
                 ++corner) {
                const SoftwareMaterialVertex *triangle_first =
                    &projected[0];
                const SoftwareMaterialVertex *triangle_second =
                    &projected[corner];
                const SoftwareMaterialVertex *triangle_third =
                    &projected[corner + 1];
                float triangle_area = software_edge(
                    triangle_first,
                    triangle_second,
                    triangle_third->x,
                    triangle_third->y);

                if (triangle_area != 0.0f &&
                    software_material_triangle_on_screen(
                        state,
                        triangle_first,
                        triangle_second,
                        triangle_third)) {
                    if (!texture_attempted) {
                        texture_attempted = 1;
                        if (material_name != NULL &&
                            state->resolveTexture != NULL &&
                            state->resolveTexture(
                                state->textureUserData,
                                material_name,
                                &texture) &&
                            texture.pixels != NULL &&
                            texture.width != 0 &&
                            texture.height != 0 &&
                            texture.stridePixels >=
                                texture.width) {
                            resolved_texture = &texture;
                        }
                    }
                    software_draw_material_triangle(
                        state,
                        triangle_first,
                        triangle_second,
                        triangle_third,
                        resolved_texture);
                }
            }
        }
        first = second;
        second = third;
    }
    return 0;
}

int jpb_SoftwareClearDepthBuffer(
    JPBSoftwareDepthBuffer *depth_buffer)
{
    size_t count;
    size_t index;

    if (depth_buffer == NULL ||
        depth_buffer->values == NULL ||
        depth_buffer->width == 0 ||
        depth_buffer->height == 0 ||
        depth_buffer->strideValues <
            depth_buffer->width ||
        depth_buffer->height >
            SIZE_MAX / depth_buffer->strideValues) {
        return 0;
    }
    count =
        depth_buffer->height *
        depth_buffer->strideValues;
    for (index = 0; index < count; ++index) {
        depth_buffer->values[index] = FLT_MAX;
    }
    return 1;
}

int jpb_SoftwareRenderJpxMaterialized(
    const JPBSoftwareJpxScene *scene,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color,
    JPBSoftwareTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats)
{
    SoftwareModelDraw state;
    float span_x;
    float span_z;
    float scale_x;
    float scale_z;
    int y;
    int result;

    if (scene == NULL || scene->view == NULL ||
        framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->width <= 0 || framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width ||
        depth_buffer == NULL || depth_buffer->values == NULL ||
        depth_buffer->width < (size_t)framebuffer->width ||
        depth_buffer->height < (size_t)framebuffer->height ||
        depth_buffer->strideValues <
            (size_t)framebuffer->width) {
        return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
    }
    for (y = 0; y < framebuffer->height; ++y) {
        uint32_t *row =
            framebuffer->pixels +
            (size_t)y *
                (size_t)framebuffer->stridePixels;
        int x;

        for (x = 0; x < framebuffer->width; ++x) {
            row[x] = clear_color;
        }
    }
    if (!jpb_SoftwareClearDepthBuffer(depth_buffer)) {
        return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
    }

    memset(&state, 0, sizeof(state));
    state.draw.scene = scene;
    state.draw.framebuffer = framebuffer;
    state.draw.view = view_matrix;
    software_set_level_camera_origin(&state, scene, view_matrix);
    state.resolveTexture = resolve_texture;
    state.textureUserData = texture_user_data;
    state.depthBuffer = depth_buffer->values;
    state.depthStride = depth_buffer->strideValues;
    state.repeatTexture = 1;
    state.depthWrite = 1;
    state.materialShader =
        SOFTWARE_MATERIAL_SHADER_LEVEL_OPAQUE;
    state.jpxPass = SOFTWARE_JPX_PASS_OPAQUE;

    span_x = scene->maxX - scene->minX;
    span_z = scene->maxZ - scene->minZ;
    if (span_x <= 0.0f) span_x = 1.0f;
    if (span_z <= 0.0f) span_z = 1.0f;
    scale_x =
        (float)(framebuffer->width -
                2 * SOFTWARE_RENDER_MARGIN - 1) /
        span_x;
    scale_z =
        (float)(framebuffer->height -
                2 * SOFTWARE_RENDER_MARGIN - 1) /
        span_z;
    state.draw.scale =
        scale_x < scale_z ? scale_x : scale_z;
    state.draw.offsetX =
        ((float)framebuffer->width -
         span_x * state.draw.scale) * 0.5f;
    state.draw.offsetZ =
        ((float)framebuffer->height -
         span_z * state.draw.scale) * 0.5f;

    result = jpx_ForEachPatchSite(
        scene->view,
        software_draw_material_strip,
        &state);
    if (result != JPB_JPX_OK) {
        return JPB_SOFTWARE_RENDER_JPX_ERROR;
    }
    state.blendTextureAlpha = 1;
    state.materialShader =
        SOFTWARE_MATERIAL_SHADER_LEVEL_TRANSPARENT;
    state.jpxPass = SOFTWARE_JPX_PASS_TRANSPARENT;
    result = jpx_ForEachPatchSite(
        scene->view,
        software_draw_material_strip,
        &state);
    if (result != JPB_JPX_OK) {
        return JPB_SOFTWARE_RENDER_JPX_ERROR;
    }
    state.depthWrite = 0;
    state.jpxPass = SOFTWARE_JPX_PASS_GLASS;
    result = jpx_ForEachPatchSite(
        scene->view,
        software_draw_material_strip,
        &state);
    if (result != JPB_JPX_OK) {
        return JPB_SOFTWARE_RENDER_JPX_ERROR;
    }
    if (stats != NULL) {
        *stats = state.draw.stats;
    }
    return JPB_SOFTWARE_RENDER_OK;
}

int jpb_SoftwareRenderLevelMesh(
    const JPBSoftwareLevelMesh *mesh,
    const JPBSoftwareJpxScene *world_scene,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color,
    JPBSoftwareTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats)
{
    SoftwareModelDraw state;
    int y;
    int pass;

    if (mesh == NULL || mesh->batches == NULL ||
        mesh->batchCount == 0 || world_scene == NULL ||
        framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->width <= 0 || framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width ||
        depth_buffer == NULL || depth_buffer->values == NULL ||
        depth_buffer->width < (size_t)framebuffer->width ||
        depth_buffer->height < (size_t)framebuffer->height ||
        depth_buffer->strideValues < (size_t)framebuffer->width) {
        return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
    }
    for (y = 0; y < framebuffer->height; ++y) {
        uint32_t *row = framebuffer->pixels +
            (size_t)y * (size_t)framebuffer->stridePixels;
        int x;

        for (x = 0; x < framebuffer->width; ++x) {
            row[x] = clear_color;
        }
    }
    if (!jpb_SoftwareClearDepthBuffer(depth_buffer)) {
        return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
    }

    memset(&state, 0, sizeof(state));
    state.draw.scene = world_scene;
    state.draw.framebuffer = framebuffer;
    state.draw.view = view_matrix;
    software_set_level_camera_origin(&state, world_scene, view_matrix);
    state.resolveTexture = resolve_texture;
    state.textureUserData = texture_user_data;
    state.depthBuffer = depth_buffer->values;
    state.depthStride = depth_buffer->strideValues;
    state.repeatTexture = 1;

    for (pass = JPB_LEVEL_FBX_PASS_OPAQUE;
         pass <= JPB_LEVEL_FBX_PASS_GLASS;
         ++pass) {
        size_t batch_index;

        state.blendTextureAlpha =
            pass != JPB_LEVEL_FBX_PASS_OPAQUE;
        state.depthWrite =
            pass != JPB_LEVEL_FBX_PASS_GLASS;
        state.materialShader =
            pass == JPB_LEVEL_FBX_PASS_OPAQUE
                ? SOFTWARE_MATERIAL_SHADER_LEVEL_OPAQUE
                : SOFTWARE_MATERIAL_SHADER_LEVEL_TRANSPARENT;
        state.jpxPass =
            pass == JPB_LEVEL_FBX_PASS_OPAQUE
                ? SOFTWARE_JPX_PASS_OPAQUE
                : (pass == JPB_LEVEL_FBX_PASS_TRANSPARENT
                       ? SOFTWARE_JPX_PASS_TRANSPARENT
                       : SOFTWARE_JPX_PASS_GLASS);

        for (batch_index = 0;
             batch_index < mesh->batchCount;
             ++batch_index) {
            const JPBSoftwareLevelBatch *batch =
                &mesh->batches[batch_index];
            JPBSoftwareTexture texture;
            const JPBSoftwareTexture *resolved_texture = NULL;
            size_t vertex;

            if ((int)batch->pass != pass) {
                continue;
            }
            if (batch->vertices == NULL ||
                batch->vertexCount < 3 ||
                batch->vertexCount % 3 != 0 ||
                batch->meshName == NULL ||
                !jpb_ShouldDrawFbxMesh(
                    mesh->levelIndex,
                    batch->pass,
                    batch->meshCount,
                    batch->meshIndex,
                    batch->meshName)) {
                state.draw.stats.levelCulledTriangles +=
                    batch->vertexCount / 3;
                continue;
            }
            memset(&texture, 0, sizeof(texture));
            texture.colorOverride = -1;
            if (batch->textureName != NULL &&
                state.resolveTexture != NULL &&
                state.resolveTexture(
                    state.textureUserData,
                    batch->textureName,
                    &texture) &&
                texture.pixels != NULL &&
                texture.width != 0 && texture.height != 0 &&
                texture.stridePixels >= texture.width) {
                resolved_texture = &texture;
            }

            for (vertex = 0;
                 vertex < batch->vertexCount;
                 vertex += 3) {
                FVECTOR world[3];
                pairUV uv[3];
                CVECTOR color[3];
                SoftwareMaterialVertex projected[
                    SOFTWARE_CLIPPED_POLYGON_CAPACITY];
                size_t projected_count;
                size_t corner;

                for (corner = 0; corner < 3; ++corner) {
                    const JPBSoftwareLevelVertex *source =
                        &batch->vertices[vertex + corner];

                    world[corner] = source->position;
                    uv[corner].u = source->u;
                    uv[corner].v = source->v;
                    color[corner].r = (uint8_t)
                        software_clamp_byte(source->red);
                    color[corner].g = (uint8_t)
                        software_clamp_byte(source->green);
                    color[corner].b = (uint8_t)
                        software_clamp_byte(source->blue);
                    color[corner].cd = (uint8_t)
                        software_clamp_byte(source->alpha);
                }
                projected_count =
                    software_clip_project_material_polygon(
                        &state, world, uv, color, 3, projected);
                ++state.draw.stats.triangles;
                if (pass != JPB_LEVEL_FBX_PASS_OPAQUE) {
                    ++state.draw.stats.levelTransparentTriangles;
                    if (pass == JPB_LEVEL_FBX_PASS_GLASS) {
                        ++state.draw.stats.levelGlassTriangles;
                    }
                }
                for (corner = 1;
                     corner + 1 < projected_count;
                     ++corner) {
                    if (software_material_triangle_on_screen(
                            &state,
                            &projected[0],
                            &projected[corner],
                            &projected[corner + 1])) {
                        software_draw_material_triangle(
                            &state,
                            &projected[0],
                            &projected[corner],
                            &projected[corner + 1],
                            resolved_texture);
                    }
                }
            }
        }
    }
    if (stats != NULL) {
        *stats = state.draw.stats;
    }
    return JPB_SOFTWARE_RENDER_OK;
}

static int software_draw_model_node(
    SoftwareModelDraw *state,
    Mnode *node,
    const SoftwareModelTransform *parent)
{
    JPBBmdGeometryView geometry;
    JPBSoftwareTexture texture;
    _Material *material;
    const JPBSoftwareTexture *material_texture;
    const JPBSoftwareTexture *resolved_texture = NULL;
    SoftwareModelTransform current = *parent;
    MATRIX local_rotation;
    FVECTOR local_translation = {
        (float)node->v3Translation.vx,
        (float)node->v3Translation.vy,
        (float)node->v3Translation.vz
    };
    FVECTOR transformed_translation;
    size_t vertex;
    size_t face;
    int child;

    fApplyMatrixFV(
        &current.rotation,
        &local_translation,
        &transformed_translation);
    if ((node->flags & UINT32_C(0x04000004)) ==
        UINT32_C(0x04000000)) {
        current.translation.vx =
            (float)node->v3Translation2.vx;
        current.translation.vy =
            (float)node->v3Translation2.vy;
        current.translation.vz =
            (float)node->v3Translation2.vz;
    } else if (((uint32_t)node->id & NODE_INDEX_MASK) != 0) {
        current.translation.vx += transformed_translation.vx;
        current.translation.vy += transformed_translation.vy;
        current.translation.vz += transformed_translation.vz;
    }

    /*
     * render_RenderNode publishes each node's current world center and
     * per-frame velocity at matched-PC RVAs 0x129C9B..0x129D48. The
     * collision solver consumes these exact Mnode fields.
     */
    node->v3Velocity.vx = (int16_t)(
        (int32_t)current.translation.vx -
        node->v3RotCenter.vx);
    node->v3Velocity.vy = (int16_t)(
        (int32_t)current.translation.vy -
        node->v3RotCenter.vy);
    node->v3Velocity.vz = (int16_t)(
        (int32_t)current.translation.vz -
        node->v3RotCenter.vz);
    node->v3RotCenter.vx =
        (int32_t)current.translation.vx;
    node->v3RotCenter.vy =
        (int32_t)current.translation.vy;
    node->v3RotCenter.vz =
        (int32_t)current.translation.vz;

    fRotMatrixZYX(
        (_svector *)&node->v3CurrentRotation,
        &local_rotation);
    if ((node->flags & UINT32_C(0x00400000)) != 0) {
        fScaleMatrix(
            &local_rotation,
            (VECTOR *)&node->v3Scale);
    }
    fMulMatrix(&current.rotation, &local_rotation);

    if (jpb_BmdGetGeometry(
            state->bmd, node->pGeomData, &geometry) != JPB_BMD_OK) {
        return JPB_SOFTWARE_RENDER_BMD_ERROR;
    }
    if (geometry.total_vertex_count >
        JPB_SOFTWARE_MODEL_VERTEX_CAPACITY) {
        return JPB_SOFTWARE_RENDER_MODEL_TOO_LARGE;
    }
    memset(&texture, 0, sizeof(texture));
    material = jpb_BmdGetMaterial(
        state->bmd, geometry.geometry);
    material_texture =
        material != NULL
            ? (const JPBSoftwareTexture *)material->texture
            : NULL;
    if (state->depthBuffer != NULL &&
        material_texture != NULL &&
        material_texture->pixels != NULL &&
        material_texture->width != 0 &&
        material_texture->height != 0 &&
        material_texture->stridePixels >= material_texture->width) {
        resolved_texture = material_texture;
    } else if (state->depthBuffer != NULL &&
        state->resolveTexture != NULL &&
        state->resolveTexture(
            state->textureUserData,
            geometry.geometry->t.Texture,
            &texture) &&
        texture.pixels != NULL &&
        texture.width != 0 &&
        texture.height != 0 &&
        texture.stridePixels >= texture.width) {
        resolved_texture = &texture;
    }
    for (vertex = 0;
         vertex < geometry.local_vertex_count;
         ++vertex) {
        FVECTOR decoded;
        FVECTOR transformed;

        jpb_BmdDecodePackedVertex(
            geometry.packed_vertices[vertex], &decoded);
        fApplyMatrixFV(
            &current.rotation, &decoded, &transformed);
        transformed.vx += current.translation.vx;
        transformed.vy += current.translation.vy;
        transformed.vz += current.translation.vz;
        state->transformed[
            geometry.shared_vertex_count + vertex] =
                transformed;
    }

    for (face = 0; face < geometry.face_count; ++face) {
        size_t corners =
            jpb_BmdFaceCornerCount(&geometry, face);
        FVECTOR material_world[4];
        pairUV material_uv[4];
        CVECTOR material_color[4];
        int material_face_visible =
            state->depthBuffer != NULL;
        float height = 0.0f;
        size_t corner;

        for (corner = 0; corner < corners; ++corner) {
            size_t index;
            const pairUV *uv;
            const CVECTOR *color;

            if (!jpb_BmdFaceVertexIndex(
                    &geometry, face, corner, &index)) {
                return JPB_SOFTWARE_RENDER_BMD_ERROR;
            }
            height += state->transformed[index].vy;
            uv = jpb_BmdFaceUv(&geometry, face, corner);
            color =
                jpb_BmdFaceColor(&geometry, face, corner);
            if (uv == NULL || color == NULL) {
                return JPB_SOFTWARE_RENDER_BMD_ERROR;
            }
            material_world[corner] =
                state->transformed[index];
            material_uv[corner] = *uv;
            material_color[corner] = *color;
            if (resolved_texture != NULL &&
                resolved_texture->colorOverride != -1) {
                int32_t color_override =
                    resolved_texture->colorOverride;

                if (color_override >= 0) {
                    material_color[corner].r =
                        (uint8_t)color_override;
                    material_color[corner].g =
                        (uint8_t)color_override;
                    material_color[corner].b =
                        (uint8_t)color_override;
                } else if (color_override == -1000 &&
                           color->r < 3) {
                    material_color[corner].r = UINT8_C(0x12);
                    material_color[corner].g = UINT8_C(0x12);
                    material_color[corner].b = UINT8_C(0x12);
                }
            }
        }
        if (material_face_visible) {
            size_t primitive;

            /* _RenderNode submits faces through _StartPoly's recovered
             * triangle-strip topology. Assemble and clip each source
             * triangle independently; treating an articulated non-planar
             * quad as one fan polygon can reverse or discard droid panels. */
            for (primitive = 0;
                 primitive + 2 < corners;
                 ++primitive) {
                size_t triangle_corner[3] = {
                    primitive,
                    primitive + 1,
                    primitive + 2
                };
                FVECTOR triangle_world[3];
                pairUV triangle_uv[3];
                CVECTOR triangle_color[3];
                SoftwareMaterialVertex material_vertices[
                    SOFTWARE_CLIPPED_POLYGON_CAPACITY];
                size_t projected_corners;
                size_t clipped_corner;

                if ((primitive & 1u) != 0) {
                    size_t swap = triangle_corner[0];

                    triangle_corner[0] = triangle_corner[1];
                    triangle_corner[1] = swap;
                }
                for (clipped_corner = 0;
                     clipped_corner < 3;
                     ++clipped_corner) {
                    size_t source_corner =
                        triangle_corner[clipped_corner];

                    triangle_world[clipped_corner] =
                        material_world[source_corner];
                    triangle_uv[clipped_corner] =
                        material_uv[source_corner];
                    triangle_color[clipped_corner] =
                        material_color[source_corner];
                }
                projected_corners =
                    software_clip_project_material_polygon(
                        state,
                        triangle_world,
                        triangle_uv,
                        triangle_color,
                        3,
                        material_vertices);
                if (projected_corners < 3 ||
                    ((resolved_texture == NULL ||
                      resolved_texture->materialFlags ==
                          JPB_MATERIAL_MODE_BACKFACE_REJECT) &&
                     software_material_polygon_winding(
                         material_vertices,
                         projected_corners) < 0.0f)) {
                    continue;
                }
                if (resolved_texture != NULL &&
                    resolved_texture->materialFlags ==
                        JPB_MATERIAL_MODE_SCREEN_TILE) {
                    for (clipped_corner = 0;
                         clipped_corner < projected_corners;
                         ++clipped_corner) {
                        material_vertices[clipped_corner].depth =
                            0.0001f;
                        material_vertices[clipped_corner].inverseDepth =
                            10000.0f;
                    }
                }
                for (clipped_corner = 1;
                     clipped_corner + 1 < projected_corners;
                     ++clipped_corner) {
                    software_draw_material_triangle(
                        state,
                        &material_vertices[0],
                        &material_vertices[clipped_corner],
                        &material_vertices[clipped_corner + 1],
                        resolved_texture);
                }
            }
        }
        height /= (float)corners;
        if (state->draw.scene->maxY >
            state->draw.scene->minY) {
            height =
                (height - state->draw.scene->minY) /
                (state->draw.scene->maxY -
                 state->draw.scene->minY);
        } else {
            height = 0.5f;
        }
        if (state->depthBuffer == NULL) {
            for (corner = 0; corner < corners; ++corner) {
                size_t next = (corner + 1) % corners;
                size_t from_index;
                size_t to_index;
                JPBJpxVertex from;
                JPBJpxVertex to;

                if (!jpb_BmdFaceVertexIndex(
                        &geometry,
                        face,
                        corner,
                        &from_index) ||
                    !jpb_BmdFaceVertexIndex(
                        &geometry,
                        face,
                        next,
                        &to_index)) {
                    return JPB_SOFTWARE_RENDER_BMD_ERROR;
                }
                software_model_vertex(
                    &state->transformed[from_index],
                    &from);
                software_model_vertex(
                    &state->transformed[to_index],
                    &to);
                software_draw_line(
                    &state->draw, &from, &to, height);
            }
        }
        state->draw.stats.triangles += corners - 2;
        state->draw.stats.modelTriangles += corners - 2;
    }

    for (child = 0; child < node->numChildNodes; ++child) {
        int result = software_draw_model_node(
            state, &node->aChildNode[child], &current);

        if (result != JPB_SOFTWARE_RENDER_OK) {
            return result;
        }
    }
    return JPB_SOFTWARE_RENDER_OK;
}

static int software_render_bmd(
    const JPBBmdView *bmd,
    modelObject *model,
    const _animFrame *key_frame,
    const FVECTOR *world_position,
    int32_t world_facing,
    MATRIX *view_matrix,
    const JPBSoftwareJpxScene *world_scene,
    JPBSoftwareFramebuffer *framebuffer,
    int materialized,
    JPBSoftwareTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *shared_depth_buffer,
    JPBSoftwareRenderStats *stats)
{
    SoftwareModelDraw state;
    SoftwareModelTransform root_parent;
    FVECTOR root_translation;
    FVECTOR transformed_root_translation;
    float span_x;
    float span_z;
    float scale_x;
    float scale_z;
    int owns_depth_buffer = 0;
    int result;

    if (bmd == NULL || bmd->root == NULL ||
        model == NULL || model->pRootNode == NULL ||
        world_position == NULL ||
        world_scene == NULL || world_scene->view == NULL ||
        framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->width <= 0 || framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width) {
        return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
    }

    memset(&state, 0, sizeof(state));
    state.bmd = bmd;
    state.draw.scene = world_scene;
    state.draw.framebuffer = framebuffer;
    state.draw.view = view_matrix;
    state.draw.modelGeometry = 1;
    state.resolveTexture = resolve_texture;
    state.textureUserData = texture_user_data;
    state.repeatTexture = 0;
    state.blendTextureAlpha = 1;
    state.depthWrite = 1;
    state.materialShader =
        SOFTWARE_MATERIAL_SHADER_MODEL;
    if (stats != NULL) {
        state.draw.stats = *stats;
    }
    if (materialized) {
        size_t depth_count;
        size_t depth_index;

        if (shared_depth_buffer != NULL) {
            if (shared_depth_buffer->values == NULL ||
                shared_depth_buffer->width <
                    (size_t)framebuffer->width ||
                shared_depth_buffer->height <
                    (size_t)framebuffer->height ||
                shared_depth_buffer->strideValues <
                    (size_t)framebuffer->width) {
                return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
            }
            state.depthBuffer =
                shared_depth_buffer->values;
            state.depthStride =
                shared_depth_buffer->strideValues;
        } else {
            if ((size_t)framebuffer->height >
                SIZE_MAX /
                    (size_t)framebuffer->stridePixels) {
                return
                    JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
            }
            depth_count =
                (size_t)framebuffer->height *
                (size_t)framebuffer->stridePixels;
            if (depth_count >
                SIZE_MAX / sizeof(float)) {
                return
                    JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
            }
            state.depthBuffer =
                (float *)malloc(
                    depth_count * sizeof(float));
            if (state.depthBuffer == NULL) {
                return
                    JPB_SOFTWARE_RENDER_MODEL_TOO_LARGE;
            }
            state.depthStride =
                (size_t)framebuffer->stridePixels;
            owns_depth_buffer = 1;
            for (depth_index = 0;
                 depth_index < depth_count;
                 ++depth_index) {
                state.depthBuffer[depth_index] =
                    FLT_MAX;
            }
        }
    }
    span_x = world_scene->maxX - world_scene->minX;
    span_z = world_scene->maxZ - world_scene->minZ;
    if (span_x <= 0.0f) span_x = 1.0f;
    if (span_z <= 0.0f) span_z = 1.0f;
    scale_x =
        (float)(framebuffer->width -
                2 * SOFTWARE_RENDER_MARGIN - 1) /
        span_x;
    scale_z =
        (float)(framebuffer->height -
                2 * SOFTWARE_RENDER_MARGIN - 1) /
        span_z;
    state.draw.scale = scale_x < scale_z ? scale_x : scale_z;
    state.draw.offsetX =
        ((float)framebuffer->width -
         span_x * state.draw.scale) * 0.5f;
    state.draw.offsetZ =
        ((float)framebuffer->height -
         span_z * state.draw.scale) * 0.5f;

    vec_IdentMatrix(&root_parent.rotation);
    fRotMatrixY(world_facing, &root_parent.rotation);
    fScaleMatrix(
        &root_parent.rotation,
        (VECTOR *)&model->v3Scale);
    root_parent.translation = *world_position;
    /*
     * Exact render_RenderModel applies the decoded key frame's root motion
     * before entering render_RenderNode. The static BMD root-node offset is
     * not a substitute and must not be applied here (or applied twice).
     */
    if (key_frame != NULL) {
        root_translation.vx =
            (float)key_frame->v3RootTranslation.vx;
        root_translation.vy =
            (float)key_frame->v3RootTranslation.vy;
        root_translation.vz =
            (float)key_frame->v3RootTranslation.vz;
    } else {
        memset(&root_translation, 0, sizeof(root_translation));
    }
    fApplyMatrixFV(
        &root_parent.rotation,
        &root_translation,
        &transformed_root_translation);
    root_parent.translation.vx +=
        transformed_root_translation.vx;
    root_parent.translation.vy +=
        transformed_root_translation.vy;
    root_parent.translation.vz +=
        transformed_root_translation.vz;

    result = software_draw_model_node(
        &state, model->pRootNode, &root_parent);
    if (result != JPB_SOFTWARE_RENDER_OK) {
        if (owns_depth_buffer) {
            free(state.depthBuffer);
        }
        return result;
    }
    if (stats != NULL) {
        *stats = state.draw.stats;
    }
    if (owns_depth_buffer) {
        free(state.depthBuffer);
    }
    return JPB_SOFTWARE_RENDER_OK;
}

int jpb_SoftwareRenderBmdWireframe(
    const JPBBmdView *bmd,
    modelObject *model,
    const _animFrame *key_frame,
    const FVECTOR *world_position,
    int32_t world_facing,
    MATRIX *view_matrix,
    const JPBSoftwareJpxScene *world_scene,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareRenderStats *stats)
{
    return software_render_bmd(
        bmd,
        model,
        key_frame,
        world_position,
        world_facing,
        view_matrix,
        world_scene,
        framebuffer,
        0,
        NULL,
        NULL,
        NULL,
        stats);
}

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
    JPBSoftwareRenderStats *stats)
{
    return software_render_bmd(
        bmd,
        model,
        key_frame,
        world_position,
        world_facing,
        view_matrix,
        world_scene,
        framebuffer,
        1,
        resolve_texture,
        texture_user_data,
        NULL,
        stats);
}

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
    JPBSoftwareRenderStats *stats)
{
    return software_render_bmd(
        bmd,
        model,
        key_frame,
        world_position,
        world_facing,
        view_matrix,
        world_scene,
        framebuffer,
        1,
        resolve_texture,
        texture_user_data,
        depth_buffer,
        stats);
}
