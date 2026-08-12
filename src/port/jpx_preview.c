/*
 * One-shot JPX preview using the portable caller-owned software framebuffer.
 * Binary PPM output keeps this inspection tool free of windowing and image
 * library dependencies.
 */

#include "jpb/jpx.h"
#include "jpb/level_world.h"
#include "jpb/projection.h"
#include "jpb/software_renderer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    PREVIEW_WIDTH = 1024,
    PREVIEW_HEIGHT = 1024
};

static int write_ppm(
    const char *path, const JPBSoftwareFramebuffer *framebuffer)
{
    FILE *output = fopen(path, "wb");
    int y;

    if (output == NULL) {
        return 0;
    }
    fprintf(
        output,
        "P6\n%d %d\n255\n",
        framebuffer->width,
        framebuffer->height);
    for (y = 0; y < framebuffer->height; ++y) {
        const uint32_t *row =
            framebuffer->pixels +
            (size_t)y * (size_t)framebuffer->stridePixels;
        int x;

        for (x = 0; x < framebuffer->width; ++x) {
            uint32_t pixel = row[x];
            uint8_t rgb[3];

            rgb[0] = (uint8_t)(pixel >> 16);
            rgb[1] = (uint8_t)(pixel >> 8);
            rgb[2] = (uint8_t)pixel;
            if (fwrite(rgb, sizeof(rgb), 1, output) != 1) {
                fclose(output);
                return 0;
            }
        }
    }
    return fclose(output) == 0;
}

int main(int argc, char **argv)
{
    JPBJpxLoadConfig config;
    JPBJpxView view;
    JPBSoftwareJpxScene scene;
    JPBSoftwareFramebuffer framebuffer;
    JPBSoftwareRenderStats stats;
    MATRIX view_matrix;
    MATRIX *active_view = NULL;
    uint8_t *storage;
    uint32_t *pixels;
    int perspective;
    int level_index;
    int result;

    if (argc < 3 || argc > 4 ||
        (argc == 4 &&
         strcmp(argv[3], "top") != 0 &&
         strcmp(argv[3], "perspective") != 0)) {
        fprintf(
            stderr,
            "usage: %s <world.jpx> <preview.ppm> "
            "[top|perspective]\n",
            argv[0]);
        return 2;
    }
    perspective =
        argc == 4 && strcmp(argv[3], "perspective") == 0;
    storage = (uint8_t *)malloc(JPB_JPX_REFERENCE_WORLD_CAPACITY);
    pixels = (uint32_t *)malloc(
        (size_t)PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(*pixels));
    if (storage == NULL || pixels == NULL) {
        free(pixels);
        free(storage);
        fputs("could not allocate preview buffers\n", stderr);
        return 3;
    }
    memset(&config, 0, sizeof(config));
    config.storage = storage;
    config.storageCapacity = JPB_JPX_REFERENCE_WORLD_CAPACITY;
    level_index = jpb_LevelIndexFromPath(argv[1]);
    if (level_index != JPB_LEVEL_INDEX_NONE) {
        config.levelName = sLevelNames[level_index];
    }
    result = jpx_LoadFile(argv[1], &config, &view);
    if (result != JPB_JPX_OK) {
        fprintf(stderr, "JPX load failed: status=%d\n", result);
        free(pixels);
        free(storage);
        return 4;
    }
    result = jpb_SoftwarePrepareJpxLevelScene(
        &view, level_index, &scene);
    if (result != JPB_SOFTWARE_RENDER_OK) {
        fprintf(stderr, "strip inspection failed: status=%d\n", result);
        free(pixels);
        free(storage);
        return 5;
    }
    if (perspective) {
        float span_x = scene.maxX - scene.minX;
        float span_y = scene.maxY - scene.minY;
        float span_z = scene.maxZ - scene.minZ;
        float span = span_x;
        FVECTOR target;
        FVECTOR eye;
        FVECTOR up = {0.0f, 1.0f, 0.0f};

        if (span_y > span) span = span_y;
        if (span_z > span) span = span_z;
        if (span <= 0.0f) span = 1.0f;
        target.vx = (scene.minX + scene.maxX) * 0.5f;
        target.vy = (scene.minY + scene.maxY) * 0.5f;
        target.vz = (scene.minZ + scene.maxZ) * 0.5f;
        eye.vx = target.vx + span * 0.7f;
        eye.vy = target.vy + span * 0.5f;
        eye.vz = target.vz - span * 0.8f;
        result = jpb_BuildLookAtView(
            &eye, &target, &up, &view_matrix);
        if (result != JPB_PROJECTION_OK) {
            fprintf(
                stderr,
                "camera construction failed: status=%d\n",
                result);
            free(pixels);
            free(storage);
            return 5;
        }
        active_view = &view_matrix;
    }
    framebuffer.pixels = pixels;
    framebuffer.width = PREVIEW_WIDTH;
    framebuffer.height = PREVIEW_HEIGHT;
    framebuffer.stridePixels = PREVIEW_WIDTH;
    result = jpb_SoftwareRenderJpxWireframe(
        &scene, active_view, &framebuffer, 0, &stats);
    if (result != JPB_SOFTWARE_RENDER_OK) {
        fprintf(stderr, "strip draw failed: status=%d\n", result);
        free(pixels);
        free(storage);
        return 5;
    }
    if (!write_ppm(argv[2], &framebuffer)) {
        fprintf(stderr, "could not write %s\n", argv[2]);
        free(pixels);
        free(storage);
        return 6;
    }
    printf(
        "mode=%s strips=%zu vertices=%zu triangles=%zu "
        "x=%.3f..%.3f z=%.3f..%.3f y=%.3f..%.3f\n",
        perspective ? "perspective" : "top",
        scene.strips,
        scene.vertices,
        stats.triangles,
        scene.minX,
        scene.maxX,
        scene.minZ,
        scene.maxZ,
        scene.minY,
        scene.maxY);
    free(pixels);
    free(storage);
    return 0;
}
