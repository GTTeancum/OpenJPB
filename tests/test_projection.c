#include "jpb/projection.h"
#include "jpb/fx.h"
#include "jpb/game.h"
#include "jpb/generic_hook.h"
#include "jpb/jedi.h"
#include "jpb/level.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/resources.h"
#include "jpb/scene.h"
#include "jpb/software_renderer.h"
#include "jpb/sprite.h"
#include "jpb/texture.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                               \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct ScreenPolyTrace {
    int calls;
    _Material *material;
    uint32_t materialFlags;
    int vertexCount;
    int noScale;
    JPBScreenPolyVertex vertices[4];
    JPBScreenPolyVertex captured[8][4];
} ScreenPolyTrace;

typedef struct ScreenPolyTriangleTrace {
    int calls;
    JPBSoftwareMaterialVertex vertices[6];
    const JPBSoftwareTexture *textures[2];
} ScreenPolyTriangleTrace;

static int capture_screen_poly_triangle(
    void *user_data,
    const JPBSoftwareMaterialVertex *first,
    const JPBSoftwareMaterialVertex *second,
    const JPBSoftwareMaterialVertex *third,
    const JPBSoftwareTexture *texture)
{
    ScreenPolyTriangleTrace *trace =
        (ScreenPolyTriangleTrace *)user_data;
    int offset;

    if (trace == NULL || trace->calls >= 2) return 0;
    offset = trace->calls * 3;
    trace->vertices[offset] = *first;
    trace->vertices[offset + 1] = *second;
    trace->vertices[offset + 2] = *third;
    trace->textures[trace->calls] = texture;
    ++trace->calls;
    return 1;
}

static void *load_test_texture(
    void *user_data,
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height)
{
    (void)filename;
    (void)option;
    (void)material_type;
    *width = 1;
    *height = 1;
    return user_data;
}

static void capture_screen_poly(
    void *user_data,
    _Material *material,
    uint32_t material_flags,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale)
{
    ScreenPolyTrace *trace = (ScreenPolyTrace *)user_data;
    int call = trace->calls;

    ++trace->calls;
    trace->material = material;
    trace->materialFlags = material_flags;
    trace->vertexCount = vertex_count;
    trace->noScale = no_scale;
    if (vertex_count > 0 && vertex_count <= 4) {
        memcpy(
            trace->vertices,
            vertices,
            (size_t)vertex_count * sizeof(trace->vertices[0]));
        if (call < 8) {
            memcpy(
                trace->captured[call],
                vertices,
                (size_t)vertex_count * sizeof(trace->captured[0][0]));
        }
    }
}

int main(void)
{
    FVECTOR eye = {0.0f, 0.0f, -10.0f};
    FVECTOR target = {0.0f, 0.0f, 0.0f};
    FVECTOR up = {0.0f, 1.0f, 0.0f};
    FVECTOR screen;
    MATRIX view;
    uint32_t poly_pixels[64 * 64] = {0};
    float poly_depth_values[64 * 64];
    JPBSoftwareFramebuffer poly_framebuffer = {
        poly_pixels, 64, 64, 64};
    JPBSoftwareDepthBuffer poly_depth = {
        poly_depth_values, 64, 64, 64};
    JPBSoftwareRenderStats poly_stats = {0};
    JPBScreenPolyVertex software_quad[4] = {
        {267.857147f, 153.940887f, 0.5f,
         UINT32_C(0xffffffff), 0.0f, 0.0f},
        {267.857147f, 338.669952f, 0.5f,
         UINT32_C(0xffffffff), 0.0f, 1.0f},
        {589.285706f, 153.940887f, 0.5f,
         UINT32_C(0xffffffff), 1.0f, 0.0f},
        {589.285706f, 338.669952f, 0.5f,
         UINT32_C(0xffffffff), 1.0f, 1.0f}
    };
    JPBScreenPolyVertex camera_quad[4] = {
        {-50.0f, 20.0f, 200.0f, UINT32_C(0xffffffff), 0.0f, 0.0f},
        {50.0f, 20.0f, 200.0f, UINT32_C(0xffffffff), 1.0f, 0.0f},
        {-50.0f, 20.0f, 100.0f, UINT32_C(0xffffffff), 0.0f, 1.0f},
        {50.0f, 20.0f, 100.0f, UINT32_C(0xffffffff), 1.0f, 1.0f}
    };
    FRONTENDVERT frontend_vertices[2] = {
        {12.0f, 34.0f, 0.25f, 0.5f, 1, 2, 3, (int)UINT32_C(0xff123456)},
        {56.0f, 78.0f, 0.75f, 1.0f, 4, 5, 6, (int)UINT32_C(0xffabcdef)}
    };
    _svector glow_start = {-10, 0, 0, 0};
    _svector glow_end = {10, 0, 0, 0};
    short clipped[2];
    ScreenPolyTrace poly_trace = {0};
    _Material material = {0};
    JPBSoftwareTexture hook_texture = {0};
    physicsObject arrow_physics = {0};
    playerObject arrow_player = {0};
    sceneObject arrow_scene = {0};
    WorldData arrow_world = {0};

    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    CHECK(jpb_ResourceSetBasePath("C:/jpb-projection-test"));
    jpb_TextureSetPlatformHooks(
        load_test_texture, NULL, &hook_texture);
    fx_Init();
    CHECK(fx_DefaultTexturesReady());
    clipped[0] = 320;
    clipped[1] = 240;
    CHECK(cliptoscreen(clipped) == 255);
    CHECK(clipped[0] == 320);
    CHECK(clipped[1] == 240);
    clipped[0] = 0;
    clipped[1] = 540;
    CHECK(cliptoscreen(clipped) == 231);
    CHECK(clipped[0] == 24);
    CHECK(clipped[1] == 540);

    jpb_WHookSetScreenPolyHook(
        capture_screen_poly, &poly_trace);
    material.texture = &hook_texture;
    material.flags = JPB_MATERIAL_MODE_TWO_SIDED;
    _StartPoly(4, &material);
    material.flags = JPB_MATERIAL_MODE_SCREEN_TILE;
    _SetVert(0, 10.0f, 20.0f, 0.0001f, 0xff010203, 0.0f, 1.0f);
    _SetVert(1, 30.0f, 40.0f, 0.0001f, 0xff040506, 1.0f, 0.0f);
    _SetVert(2, 50.0f, 60.0f, 0.0001f, 0xff070809, 1.0f, 1.0f);
    _SetVert(3, 70.0f, 80.0f, 0.0001f, 0xff0a0b0c, 0.0f, 0.0f);
    _EndPoly();
    CHECK(poly_trace.calls == 1);
    CHECK(poly_trace.material == &material);
    CHECK(poly_trace.vertexCount == 4);
    CHECK(poly_trace.noScale == 0);
    CHECK(poly_trace.materialFlags == JPB_MATERIAL_MODE_TWO_SIDED);
    CHECK(poly_trace.vertices[0].x == 10.0f);
    CHECK(poly_trace.vertices[0].y == 20.0f);
    CHECK(poly_trace.vertices[0].argb == UINT32_C(0xff010203));
    CHECK(poly_trace.vertices[3].x == 70.0f);
    _StartPoly(1, &material);
    _SetVert(0, 90.0f, 100.0f, 0.0001f, 0xffffffff, 0.0f, 0.0f);
    _NoScaleEndPoly();
    CHECK(poly_trace.calls == 2);
    CHECK(poly_trace.vertexCount == 1);
    CHECK(poly_trace.noScale == 1);
    CHECK(poly_trace.vertices[0].x == 90.0f);
    CHECK(sizeof(FRONTENDVERT) == 32);
    frontEndPoly(&material, 2, frontend_vertices, 0.625f);
    CHECK(poly_trace.calls == 3);
    CHECK(poly_trace.vertexCount == 2);
    CHECK(poly_trace.vertices[0].x == 12.0f);
    CHECK(poly_trace.vertices[0].y == 34.0f);
    CHECK(poly_trace.vertices[0].z == 0.625f);
    CHECK(poly_trace.vertices[0].tu == 0.25f);
    CHECK(poly_trace.vertices[0].tv == 0.5f);
    CHECK(poly_trace.vertices[0].argb == UINT32_C(0xff123456));
    CHECK(poly_trace.vertices[1].argb == UINT32_C(0xff123456));

    memset(&CameraMatrix, 0, sizeof(CameraMatrix));
    CameraMatrix.m[0][0] = 1.0f;
    CameraMatrix.m[1][1] = 1.0f;
    CameraMatrix.m[2][2] = 1.0f;
    arrow_physics.physicsRoot.pParent = &arrow_scene.sceneRoot;
    arrow_physics.physicsRoot.objectID = 0;
    arrow_physics.pos.vx = -1000.0f;
    arrow_physics.pos.vy = 0.0f;
    arrow_physics.pos.vz = 100.0f;
    arrow_scene.pPlayer = &arrow_player.playerRoot;
    arrow_player.playerRoot.objectID = 0;
    gpWorld = &arrow_world;
    GameStruct.CurrentLevel = 1;
    LevelSelect = 1;
    gSCENE_READY = 1;
    poly_trace.calls = 0;
    playerOffScreenArrow(&arrow_physics, UINT32_C(0x004080ff));
    CHECK(poly_trace.calls == 1);
    CHECK(poly_trace.vertexCount == 4);
    CHECK(poly_trace.noScale == 0);
    CHECK(playeronscreen[0] == 0);
    CHECK(poly_trace.vertices[0].x == 73.0f);
    CHECK(poly_trace.vertices[0].y == 572.0f);
    CHECK(poly_trace.vertices[0].z == 0.0001f);
    CHECK(poly_trace.vertices[0].argb == UINT32_C(0xff4080ff));
    CHECK(poly_trace.vertices[1].x == 56.0f);
    CHECK(poly_trace.vertices[1].y == 565.0f);
    CHECK(poly_trace.vertices[1].z == 0.0001f);
    CHECK(poly_trace.vertices[1].argb == 0);
    CHECK(poly_trace.vertices[2].x == 24.0f);
    CHECK(poly_trace.vertices[2].y == 565.0f);
    CHECK(poly_trace.vertices[2].z == 0.0001f);
    CHECK(poly_trace.vertices[2].argb == UINT32_C(0xdae3ecff));
    CHECK(poly_trace.vertices[3].x == 73.0f);
    CHECK(poly_trace.vertices[3].y == 558.0f);
    CHECK(poly_trace.vertices[3].z == 0.0001f);
    CHECK(poly_trace.vertices[3].argb == UINT32_C(0xff4080ff));

    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    zerobss_levelReset = 202;
    playeronscreen[0] = 1;
    poly_trace.calls = 0;
    playerOffScreenArrow(&arrow_physics, UINT32_C(0x004080ff));
    CHECK(poly_trace.calls == 1);
    CHECK(poly_trace.vertexCount == 4);
    CHECK(poly_trace.noScale == 0);
    CHECK(playeronscreen[0] == 0);
    CHECK(poly_trace.vertices[0].x == 74.0f);
    CHECK(poly_trace.vertices[0].y == 315.0f);
    CHECK(poly_trace.vertices[0].z == 0.0001f);
    CHECK(poly_trace.vertices[0].argb == UINT32_C(0xff4080ff));
    CHECK(poly_trace.vertices[1].x == 56.0f);
    CHECK(poly_trace.vertices[1].y == 308.0f);
    CHECK(poly_trace.vertices[1].z == 0.0001f);
    CHECK(poly_trace.vertices[1].argb == 0);
    CHECK(poly_trace.vertices[2].x == 24.0f);
    CHECK(poly_trace.vertices[2].y == 310.0f);
    CHECK(poly_trace.vertices[2].z == 0.0001f);
    CHECK(poly_trace.vertices[2].argb == UINT32_C(0xdae3ecff));
    CHECK(poly_trace.vertices[3].x == 72.0f);
    CHECK(poly_trace.vertices[3].y == 301.0f);
    CHECK(poly_trace.vertices[3].z == 0.0001f);
    CHECK(poly_trace.vertices[3].argb == UINT32_C(0xff4080ff));

    {
        static const unsigned expected_positions[6][4] = {
            {0, 1, 4, 5}, {4, 5, 8, 9}, {1, 2, 5, 6},
            {5, 6, 9, 10}, {2, 3, 6, 7}, {6, 7, 10, 11}
        };
        static const unsigned expected_uvs[6][4] = {
            {0, 1, 2, 3}, {2, 3, 0, 1}, {1, 1, 3, 3},
            {3, 3, 1, 1}, {1, 0, 3, 2}, {3, 2, 1, 0}
        };
        static const float expected_vertices[12][2] = {
            {-10.0f, 10.0f}, {0.0f, 10.0f},
            {100.0f, 10.0f}, {110.0f, 10.0f},
            {-10.0f, 0.0f}, {0.0f, 0.0f},
            {100.0f, 0.0f}, {110.0f, 0.0f},
            {-10.0f, -10.0f}, {0.0f, -10.0f},
            {100.0f, -10.0f}, {110.0f, -10.0f}
        };
        static const float expected_uv_values[4][2] = {
            {0.01f, 0.01f}, {0.99f, 0.01f},
            {0.01f, 0.99f}, {0.99f, 0.99f}
        };
        int glow_quad;

        glow_start = (_svector){0, 0, 1, 0};
        glow_end = (_svector){100, 0, 1, 0};
        memset(&poly_trace, 0, sizeof(poly_trace));
        fx_screenGlow(
            &glow_start, &glow_end, 10, UINT32_C(0x7f40c0ff));
        CHECK(poly_trace.calls == 6);
        CHECK(poly_trace.vertexCount == 4);
        CHECK(poly_trace.noScale == 1);
        for (glow_quad = 0; glow_quad < 6; ++glow_quad) {
            int glow_vertex;

            for (glow_vertex = 0; glow_vertex < 4; ++glow_vertex) {
                unsigned position =
                    expected_positions[glow_quad][glow_vertex];
                unsigned uv = expected_uvs[glow_quad][glow_vertex];
                const JPBScreenPolyVertex *actual =
                    &poly_trace.captured[glow_quad][glow_vertex];

                CHECK(actual->x == expected_vertices[position][0]);
                CHECK(actual->y == expected_vertices[position][1]);
                CHECK(fabsf(actual->z - 0.9999984f) < 0.0000001f);
                CHECK(actual->argb == UINT32_C(0x7f40c0ff));
                CHECK(actual->tu == expected_uv_values[uv][0]);
                CHECK(actual->tv == expected_uv_values[uv][1]);
            }
        }
    }
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    clipped[0] = 1900;
    clipped[1] = 540;
    CHECK(cliptoscreen(clipped) == 251);
    CHECK(clipped[0] == 1896);
    CHECK(clipped[1] == 540);
    clipped[0] = 960;
    clipped[1] = 0;
    CHECK(cliptoscreen(clipped) == 239);
    CHECK(clipped[0] == 960);
    CHECK(clipped[1] == 8);
    clipped[0] = 960;
    clipped[1] = 1080;
    CHECK(cliptoscreen(clipped) == 239);
    CHECK(clipped[0] == 960);
    CHECK(clipped[1] == 1072);
    clipped[0] = -1000;
    clipped[1] = 540;
    CHECK(cliptoscreen(clipped) == 0);
    CHECK(clipped[0] == 24);
    CHECK(clipped[1] == 540);

    CHECK(jpb_BuildLookAtView(
              &eye, &target, &up, &view) == JPB_PROJECTION_OK);
    CHECK(view.m[0][0] == 1.0f);
    CHECK(view.m[1][1] == 1.0f);
    CHECK(view.m[2][2] == 1.0f);
    CHECK(view.t[0] == 0);
    CHECK(view.t[1] == 0);
    CHECK(view.t[2] == 10);
    CHECK(jpb_ProjectToViewport(
              &view, &target, 1280.0f, 960.0f, &screen) == 0);
    CHECK(screen.vx == 640.0f);
    CHECK(screen.vy == 480.0f);
    CHECK(screen.vz == 10.0f / 10240.0f);

    {
        FVECTOR pc_world = {100.0f, 50.0f, 990.0f};
        FVECTOR wide_screen;
        FVECTOR narrow_screen;

        /*
         * The camera translation makes this (100, 50, 1000) in view space.
         * A 53-degree vertical FOV keeps the center-relative X offset fixed
         * when only viewport width changes, unlike scaled 4:3 projection.
         */
        CHECK(jpb_ProjectPcToViewport(
                  &view,
                  &pc_world,
                  960.0f,
                  540.0f,
                  &wide_screen) == 0);
        CHECK(fabsf(wide_screen.vx - 534.1536f) < 0.001f);
        CHECK(fabsf(wide_screen.vy - 297.0768f) < 0.001f);
        CHECK(wide_screen.vz == 1000.0f / 10240.0f);
        CHECK(jpb_ProjectPcToViewport(
                  &view,
                  &pc_world,
                  640.0f,
                  540.0f,
                  &narrow_screen) == 0);
        CHECK(fabsf(
                  (wide_screen.vx - 480.0f) -
                  (narrow_screen.vx - 320.0f)) < 0.001f);
        CHECK(fabsf(wide_screen.vy - narrow_screen.vy) < 0.001f);
    }

    CHECK(jpb_ProjectToViewport(
              &view, &eye, 640.0f, 480.0f, &screen) == 1);
    CHECK(screen.vx == 320.0f);
    CHECK(screen.vy == 240.0f);
    CHECK(screen.vz == 1.0f / 10240.0f);
    CHECK(jpb_ProjectPcToViewport(
              &view, &eye, 960.0f, 540.0f, &screen) == 1);
    CHECK(screen.vx == 480.0f);
    CHECK(screen.vy == 270.0f);
    CHECK(screen.vz == 1.0f / 10240.0f);
    {
        float viewport_x;
        float viewport_y;
        float viewport_width;
        float viewport_height;

        jpb_PcGameplayViewport(
            1920.0f,
            1080.0f,
            &viewport_x,
            &viewport_y,
            &viewport_width,
            &viewport_height);
        CHECK(viewport_x == 0.0f);
        CHECK(viewport_y == 0.0f);
        CHECK(viewport_width == 1920.0f);
        CHECK(viewport_height == 1080.0f);
        CHECK(jpb_ProjectPcGameplayToViewport(
                  &view, &eye, 1920.0f, 1080.0f, &screen) == 1);
        CHECK(screen.vx == 960.0f);
        CHECK(screen.vy == 540.0f);
        CHECK(screen.vz == 1.0f / 10240.0f);
    }
    {
        FVECTOR near_plane = {10.0f, -5.0f, 1.0f};

        CHECK(jpb_ProjectPcCameraToViewport(
                  &near_plane,
                  960.0f,
                  540.0f,
                  &screen) == 0);
        CHECK(screen.vz == 1.0f / 10240.0f);
    }

    target = eye;
    CHECK(jpb_BuildLookAtView(
              &eye, &target, &up, &view) ==
          JPB_PROJECTION_DEGENERATE_CAMERA);
    target.vz = 0.0f;
    up.vx = 0.0f;
    up.vy = 0.0f;
    up.vz = 1.0f;
    CHECK(jpb_BuildLookAtView(
              &eye, &target, &up, &view) ==
          JPB_PROJECTION_DEGENERATE_CAMERA);
    CHECK(jpb_ProjectToViewport(
              &view, &target, 0.0f, 480.0f, &screen) ==
          JPB_PROJECTION_INVALID_VIEWPORT);
    CHECK(jpb_ProjectToViewport(
              &view, &target, NAN, 480.0f, &screen) ==
          JPB_PROJECTION_INVALID_VIEWPORT);
    CHECK(jpb_ProjectPcToViewport(
              &view, &target, 640.0f, 0.0f, &screen) ==
          JPB_PROJECTION_INVALID_VIEWPORT);
    CHECK(jpb_ProjectPcToViewport(
              NULL, &target, 640.0f, 480.0f, &screen) ==
          JPB_PROJECTION_INVALID_VIEWPORT);

    eye.vx = 0.0f;
    eye.vy = 0.0f;
    eye.vz = -1000.0f;
    target.vx = 0.0f;
    target.vy = 0.0f;
    target.vz = 0.0f;
    up.vx = 0.0f;
    up.vy = 1.0f;
    up.vz = 0.0f;
    CHECK(jpb_BuildLookAtView(
              &eye, &target, &up, &view) == JPB_PROJECTION_OK);
    material.flags = JPB_MATERIAL_MODE_TWO_SIDED;
    CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
    CHECK(jpb_SoftwareDrawScreenPoly(
              &material,
              4,
              software_quad,
              0,
              &poly_framebuffer,
              &poly_depth,
              &poly_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(poly_stats.triangles == 2);
    CHECK(poly_stats.pixels > 0);
    CHECK(poly_pixels[32 * 64 + 32] != 0);

    {
        const uint32_t smoke_texel = UINT32_C(0x8080c040);
        uint32_t clamp_texels[2] = {
            UINT32_C(0xffff0000),
            UINT32_C(0xff00ff00)
        };
        JPBSoftwareTexture smoke_texture = {
            &smoke_texel, 1, 1, 1,
            0, TEXTURESAMPLER_LINEARCLAMP, -1, 2
        };
        JPBSoftwareTexture clamp_texture = {
            clamp_texels, 2, 1, 2,
            0, TEXTURESAMPLER_LINEARCLAMP, -1, 2
        };
        JPBScreenPolyVertex deeper_quad[4];
        JPBScreenPolyVertex clamp_quad[4];

        memset(poly_pixels, 0x20, sizeof(poly_pixels));
        memset(&poly_stats, 0, sizeof(poly_stats));
        CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
        material.texture = &smoke_texture;
        material.flags = JPB_MATERIAL_MODE_TWO_SIDED;
        CHECK(jpb_SoftwareDrawScreenPoly(
                  &material,
                  4,
                  software_quad,
                  0,
                  &poly_framebuffer,
                  &poly_depth,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        /*
         * D3DTransparencyPass::CreatePipelineState builds the class-2 a_
         * queue from the second PSO at this+0x68 with SrcAlpha/InvSrcAlpha
         * blending. a_SMOKEGRY has colored zero-alpha border texels, so this
         * split is what prevents the moving gas cards from outlining.
         */
        CHECK(poly_pixels[32 * 64 + 32] == UINT32_C(0x00507030));
        /*
         * D3DTransparencyPass::Render consumes the explicit triangle-list
         * indices built by el_chavo::NoScaleEndPoly: 0,1,2 and 1,3,2.
         */
        CHECK(poly_pixels[42 * 64 + 42] == UINT32_C(0x00507030));

        {
            ScreenPolyTriangleTrace triangle_trace = {0};
            JPBSoftwareRenderStats sink_stats = {0};

            CHECK(jpb_SoftwareDrawScreenPolyToSink(
                      &material,
                      4,
                      software_quad,
                      0,
                      &poly_framebuffer,
                      &poly_depth,
                      capture_screen_poly_triangle,
                      &triangle_trace,
                      &sink_stats) == JPB_SOFTWARE_RENDER_OK);
            CHECK(triangle_trace.calls == 2);
            CHECK(triangle_trace.textures[0] == &smoke_texture);
            CHECK(triangle_trace.textures[1] == &smoke_texture);
            CHECK(fabsf(triangle_trace.vertices[0].x - 20.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[0].y - 20.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[1].x - 20.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[1].y - 44.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[2].x - 44.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[2].y - 20.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[3].x - 20.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[3].y - 44.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[4].x - 44.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[4].y - 44.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[5].x - 44.0f) <
                  0.0001f);
            CHECK(fabsf(triangle_trace.vertices[5].y - 20.0f) <
                  0.0001f);
            CHECK(sink_stats.triangles == 2);
            CHECK(sink_stats.pixels == 2);
        }

        memcpy(deeper_quad, software_quad, sizeof(deeper_quad));
        for (int smoke_vertex = 0; smoke_vertex < 4; ++smoke_vertex) {
            deeper_quad[smoke_vertex].z = 0.75f;
        }
        CHECK(jpb_SoftwareDrawScreenPoly(
                  &material,
                  4,
                  deeper_quad,
                  0,
                  &poly_framebuffer,
                  &poly_depth,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        /*
         * D3DTransparencyPass cards depth-test against the scene but do not
         * stamp depth into later transparent cards. The FED gas is layered
         * from overlapping a_SMOKEGRY quads; depth writes expose card edges.
         */
        CHECK(poly_pixels[32 * 64 + 32] == UINT32_C(0x00689838));

        smoke_texture.materialType = 1;
        memset(poly_pixels, 0x20, sizeof(poly_pixels));
        memset(&poly_stats, 0, sizeof(poly_stats));
        CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
        CHECK(jpb_SoftwareDrawScreenPoly(
                  &material,
                  4,
                  software_quad,
                  0,
                  &poly_framebuffer,
                  &poly_depth,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(poly_pixels[32 * 64 + 32] == UINT32_C(0x00608040));

        /*
         * D3DTransparencyPass::CreatePipelineState installs LESS_EQUAL with
         * depth writes disabled for both transparency classes. Equal-depth
         * glow cards therefore draw where an opaque card is rejected.
         */
        for (size_t depth_index = 0;
             depth_index < sizeof(poly_depth_values) /
                               sizeof(poly_depth_values[0]);
             ++depth_index) {
            poly_depth_values[depth_index] = 0.5f;
        }
        memset(poly_pixels, 0x20, sizeof(poly_pixels));
        CHECK(jpb_SoftwareDrawScreenPoly(
                  &material,
                  4,
                  software_quad,
                  0,
                  &poly_framebuffer,
                  &poly_depth,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(poly_pixels[32 * 64 + 32] == UINT32_C(0x00608040));

        smoke_texture.materialType = 0;
        for (size_t depth_index = 0;
             depth_index < sizeof(poly_depth_values) /
                               sizeof(poly_depth_values[0]);
             ++depth_index) {
            poly_depth_values[depth_index] = 0.49f;
        }
        memset(poly_pixels, 0x20, sizeof(poly_pixels));
        CHECK(jpb_SoftwareDrawScreenPoly(
                  &material,
                  4,
                  software_quad,
                  0,
                  &poly_framebuffer,
                  &poly_depth,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(poly_pixels[32 * 64 + 32] == UINT32_C(0x20202020));
        smoke_texture.materialType = 2;

        memcpy(clamp_quad, software_quad, sizeof(clamp_quad));
        for (int clamp_vertex = 0; clamp_vertex < 4; ++clamp_vertex) {
            clamp_quad[clamp_vertex].tu = 1.0f;
            clamp_quad[clamp_vertex].tv = 0.0f;
        }
        memset(poly_pixels, 0, sizeof(poly_pixels));
        memset(&poly_stats, 0, sizeof(poly_stats));
        CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
        material.texture = &clamp_texture;
        CHECK(jpb_SoftwareDrawScreenPoly(
                  &material,
                  4,
                  clamp_quad,
                  0,
                  &poly_framebuffer,
                  &poly_depth,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(poly_pixels[32 * 64 + 32] == UINT32_C(0x0000ff00));
        material.texture = NULL;
    }

    CHECK(jpb_SoftwareDrawScreenPoly(
              &material,
              2,
              software_quad,
              0,
              &poly_framebuffer,
              &poly_depth,
              &poly_stats) ==
          JPB_SOFTWARE_RENDER_INVALID_ARGUMENT);
    memset(poly_pixels, 0, sizeof(poly_pixels));
    memset(&poly_stats, 0, sizeof(poly_stats));
    CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
    material.flags = JPB_MATERIAL_MODE_BACKFACE_REJECT;
    CHECK(jpb_SoftwareDrawScreenPoly(
              &material,
              4,
              camera_quad,
              1,
              &poly_framebuffer,
              &poly_depth,
              &poly_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(poly_stats.triangles == 2);
    CHECK(poly_stats.pixels > 0);

    {
        const uint32_t white_texel = UINT32_C(0xffffffff);
        JPBSoftwareTexture class_two_texture = {
            &white_texel, 1, 1, 1,
            0, TEXTURESAMPLER_LINEARCLAMP, -1, 2
        };
        JPBScreenPolyVertex offscreen_quad[4] = {
            {2000.0f, -40.0f, 100.0f, UINT32_C(0xffffffff), 0.0f, 0.0f},
            {2100.0f, -40.0f, 100.0f, UINT32_C(0xffffffff), 1.0f, 0.0f},
            {2000.0f, 40.0f, 100.0f, UINT32_C(0xffffffff), 0.0f, 1.0f},
            {2100.0f, 40.0f, 100.0f, UINT32_C(0xffffffff), 1.0f, 1.0f}
        };
        JPBScreenPolyVertex behind_camera_quad[4] = {
            {4600.0f, -300.0f, -400.0f, UINT32_C(0xffffffff), 0.0f, 0.0f},
            {5400.0f, -300.0f, -400.0f, UINT32_C(0xffffffff), 1.0f, 0.0f},
            {4600.0f, 300.0f, 350.0f, UINT32_C(0xffffffff), 0.0f, 1.0f},
            {5400.0f, 300.0f, 350.0f, UINT32_C(0xffffffff), 1.0f, 1.0f}
        };
        JPBScreenPolyVertex saber_cap_quad[4] = {
            {3.0f, 88.8f, 496.0f, UINT32_C(0xffffffff), 0.99f, 0.01f},
            {-10.2f, 97.9f, 496.0f, UINT32_C(0xffffffff), 0.01f, 0.01f},
            {12.0f, 102.0f, 496.0f, UINT32_C(0xffffffff), 0.99f, 0.99f},
            {-1.2f, 111.0f, 496.0f, UINT32_C(0xffffffff), 0.01f, 0.99f}
        };

        material.texture = &class_two_texture;
        material.flags = JPB_MATERIAL_MODE_BACKFACE_REJECT;
        memset(poly_pixels, 0, sizeof(poly_pixels));
        memset(&poly_stats, 0, sizeof(poly_stats));
        CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
        CHECK(jpb_SoftwareDrawScreenPoly(
                  &material,
                  4,
                  offscreen_quad,
                  1,
                  &poly_framebuffer,
                  &poly_depth,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(poly_stats.triangles == 2);
        CHECK(poly_stats.pixels == 0);
        {
            ScreenPolyTriangleTrace saber_trace = {0};

            memset(&poly_stats, 0, sizeof(poly_stats));
            CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
            CHECK(jpb_SoftwareDrawScreenPolyToSink(
                      &material,
                      4,
                      saber_cap_quad,
                      1,
                      &poly_framebuffer,
                      &poly_depth,
                      capture_screen_poly_triangle,
                      &saber_trace,
                      &poly_stats) == JPB_SOFTWARE_RENDER_OK);
            /*
             * NoScaleEndPoly truncates this tiny negative NDC winding to
             * zero. The former pixel-space test rejected the same canonical
             * level-two saber cap and made the blade flash as it rotated.
             */
            CHECK(saber_trace.calls == 2);
            CHECK(poly_stats.triangles == 2);
        }
        class_two_texture.materialType = 0;
        memset(poly_pixels, 0, sizeof(poly_pixels));
        memset(&poly_stats, 0, sizeof(poly_stats));
        CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
        CHECK(jpb_SoftwareDrawScreenPoly(
                  &material,
                  4,
                  behind_camera_quad,
                  1,
                  &poly_framebuffer,
                  &poly_depth,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(poly_stats.triangles == 2);
        CHECK(poly_stats.pixels == 0);
        class_two_texture.materialType = 2;
        material.flags = JPB_MATERIAL_MODE_SCREEN_TILE;
        memset(poly_pixels, 0, sizeof(poly_pixels));
        memset(&poly_stats, 0, sizeof(poly_stats));
        CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
        CHECK(jpb_SoftwareDrawScreenPoly(
                  &material,
                  4,
                  camera_quad,
                  1,
                  &poly_framebuffer,
                  &poly_depth,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(poly_stats.pixels > 0);
        {
            ScreenPolyTriangleTrace depth_trace = {0};

            CHECK(jpb_SoftwareDrawScreenPolyToSink(
                      &material,
                      4,
                      camera_quad,
                      1,
                      &poly_framebuffer,
                      &poly_depth,
                      capture_screen_poly_triangle,
                      &depth_trace,
                      &poly_stats) == JPB_SOFTWARE_RENDER_OK);
            CHECK(depth_trace.calls == 2);
            CHECK(depth_trace.vertices[0].depth == 0.0001f);
            CHECK(depth_trace.vertices[0].clipDepth == 0.0001f);
            CHECK(depth_trace.vertices[0].inverseDepth == 1.0f);

            class_two_texture.materialType = 0;
            memset(&depth_trace, 0, sizeof(depth_trace));
            CHECK(jpb_SoftwareDrawScreenPolyToSink(
                      &material,
                      4,
                      camera_quad,
                      1,
                      &poly_framebuffer,
                      &poly_depth,
                      capture_screen_poly_triangle,
                      &depth_trace,
                      &poly_stats) == JPB_SOFTWARE_RENDER_OK);
            CHECK(depth_trace.calls == 2);
            CHECK(depth_trace.vertices[0].depth ==
                  camera_quad[0].z / 10240.0f);
            CHECK(fabsf(
                      depth_trace.vertices[0].clipDepth -
                      0.9950995f) < 0.000001f);
            CHECK(depth_trace.vertices[0].inverseDepth == 1.0f);
        }
        material.texture = NULL;
    }

    {
        const uint32_t white_texel = UINT32_C(0xffffffff);
        JPBSoftwareTexture option_texture = {
            &white_texel, 1, 1, 1,
            0, TEXTURESAMPLER_LINEARCLAMP, -1, 2, 17
        };
        JPBScreenPolyVertex negative_triangle[3] = {
            {-40.0f, -40.0f, 100.0f,
             UINT32_C(0xffffffff), 0.0f, 0.0f},
            {-40.0f, 40.0f, 100.0f,
             UINT32_C(0xffffffff), 0.0f, 1.0f},
            {40.0f, -40.0f, 100.0f,
             UINT32_C(0xffffffff), 1.0f, 0.0f}
        };
        JPBScreenPolyVertex screen_outside_triangle[3] = {
            {2001.0f, 10.0f, 0.0001f,
             UINT32_C(0xffffffff), 0.0f, 0.0f},
            {2101.0f, 10.0f, 0.0001f,
             UINT32_C(0xffffffff), 1.0f, 0.0f},
            {2001.0f, 110.0f, 0.0001f,
             UINT32_C(0xffffffff), 0.0f, 1.0f}
        };
        ScreenPolyTriangleTrace triangle_trace = {0};
        char old_level = LevelSelect;

        material.texture = &option_texture;
        material.flags = JPB_MATERIAL_MODE_TWO_SIDED;
        CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
        CHECK(jpb_SoftwareDrawScreenPolyToSink(
                  &material,
                  3,
                  negative_triangle,
                  1,
                  &poly_framebuffer,
                  &poly_depth,
                  capture_screen_poly_triangle,
                  &triangle_trace,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(triangle_trace.calls == 1);

        option_texture.materialType = 0;
        memset(&triangle_trace, 0, sizeof(triangle_trace));
        CHECK(jpb_SoftwareDrawScreenPolyToSink(
                  &material,
                  3,
                  negative_triangle,
                  1,
                  &poly_framebuffer,
                  &poly_depth,
                  capture_screen_poly_triangle,
                  &triangle_trace,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(triangle_trace.calls == 0);

        memset(&triangle_trace, 0, sizeof(triangle_trace));
        CHECK(jpb_SoftwareDrawScreenPolyToSink(
                  &material,
                  3,
                  negative_triangle,
                  0,
                  &poly_framebuffer,
                  &poly_depth,
                  capture_screen_poly_triangle,
                  &triangle_trace,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(triangle_trace.calls == 1);

        memset(&triangle_trace, 0, sizeof(triangle_trace));
        CHECK(jpb_SoftwareDrawScreenPolyToSink(
                  &material,
                  3,
                  screen_outside_triangle,
                  0,
                  &poly_framebuffer,
                  &poly_depth,
                  capture_screen_poly_triangle,
                  &triangle_trace,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(triangle_trace.calls == 0);

        ClearCachedTextureIndices();
        LevelSelect = 6;
        memcpy(
            material.filename,
            "models/bus.tga",
            sizeof("models/bus.tga"));
        memset(&triangle_trace, 0, sizeof(triangle_trace));
        CHECK(jpb_SoftwareDrawScreenPolyToSink(
                  &material,
                  3,
                  negative_triangle,
                  1,
                  &poly_framebuffer,
                  &poly_depth,
                  capture_screen_poly_triangle,
                  &triangle_trace,
                  &poly_stats) == JPB_SOFTWARE_RENDER_OK);
        CHECK(triangle_trace.calls == 1);
        LevelSelect = old_level;
        material.filename[0] = '\0';
        material.texture = NULL;
    }

    {
        _Material blur_material = {0};
        VECTOR base = {100, 200, 300, 0};
        _svector base_velocity = {4, 0, 0, 0};
        _svector tip = {200, 300, 400, 0};
        _svector tip_velocity = {0, 40, 0, 0};

        memset(&poly_trace, 0, sizeof(poly_trace));
        blur_material.texture = &hook_texture;
        memset(&CameraMatrix, 0, sizeof(CameraMatrix));
        CameraMatrix.m[0][0] = 1.0f;
        CameraMatrix.m[1][1] = 1.0f;
        CameraMatrix.m[2][2] = 1.0f;
        whitematAdd = &blur_material;
        jpb_WHookSetScreenPolyHook(
            capture_screen_poly, &poly_trace);
        jedi_DrawBlur(
            &base, &base_velocity, &tip, &tip_velocity,
            UINT32_C(0x1f112233));
        CHECK(poly_trace.calls == 1);
        CHECK(poly_trace.material == &blur_material);
        CHECK(poly_trace.vertexCount == 4);
        CHECK(poly_trace.noScale == 1);
        CHECK(poly_trace.vertices[0].x == 100.0f);
        CHECK(poly_trace.vertices[0].y == 200.0f);
        CHECK(poly_trace.vertices[0].z == 300.0f);
        CHECK(poly_trace.vertices[1].x == 90.0f);
        CHECK(poly_trace.vertices[1].y == 200.0f);
        CHECK(poly_trace.vertices[2].x == 200.0f);
        CHECK(poly_trace.vertices[2].y == 300.0f);
        CHECK(poly_trace.vertices[3].x == 200.0f);
        CHECK(poly_trace.vertices[3].y == 260.0f);
        CHECK(poly_trace.vertices[0].argb ==
              UINT32_C(0x5f112233));
        CHECK(poly_trace.vertices[0].tu == 0.0f);
        CHECK(poly_trace.vertices[0].tv == 0.0f);
        whitematAdd = NULL;
        jpb_WHookSetScreenPolyHook(NULL, NULL);
    }

    {
        _Material cylinder_material = {0};
        VECTOR location = {10, 20, 100, 0};
        _svector rotation = {0, 0, 0, 0};

        memset(&poly_trace, 0, sizeof(poly_trace));
        cylinder_material.texture = &hook_texture;
        memset(&CameraMatrix, 0, sizeof(CameraMatrix));
        memset(&gSceneGeometryEnv, 0, sizeof(gSceneGeometryEnv));
        CameraMatrix.m[0][0] = 1.0f;
        CameraMatrix.m[1][1] = 1.0f;
        CameraMatrix.m[2][2] = 1.0f;
        effects1Handle[3] = &cylinder_material;
        jpb_WHookSetScreenPolyHook(
            capture_screen_poly, &poly_trace);
        drawCylinder(
            &location, &rotation, 2.0f, 4.0f, 1.0f, 5.0f,
            UINT32_C(0x7f123456), 0, 3, 0, 2);
        CHECK(poly_trace.calls == 16);
        CHECK(poly_trace.material == &cylinder_material);
        CHECK(poly_trace.vertexCount == 4);
        CHECK(poly_trace.noScale == 1);
        CHECK(poly_trace.captured[0][0].x == 12.0f);
        CHECK(poly_trace.captured[0][0].y == 21.0f);
        CHECK(poly_trace.captured[0][0].z == 100.0f);
        CHECK(poly_trace.captured[0][2].x == 14.0f);
        CHECK(poly_trace.captured[0][2].y == 25.0f);
        CHECK(poly_trace.captured[0][2].z == 100.0f);
        CHECK(poly_trace.captured[0][0].argb ==
              UINT32_C(0x7f123456));
        CHECK(poly_trace.captured[0][0].tu == 0.0f);
        CHECK(poly_trace.captured[0][1].tu == 1.0f);
        CHECK(poly_trace.captured[0][2].tv == 1.0f);
        CHECK(poly_trace.captured[0][3].tv == 1.0f);
        effects1Handle[3] = NULL;
        jpb_WHookSetScreenPolyHook(NULL, NULL);
    }

    {
        VECTOR water_position = {0, 32, 0, 0};

        memset(&poly_trace, 0, sizeof(poly_trace));
        memset(&CameraMatrix, 0, sizeof(CameraMatrix));
        CameraMatrix.m[0][0] = 1.0f;
        CameraMatrix.m[1][1] = 1.0f;
        CameraMatrix.m[2][2] = 1.0f;
        CameraMatrix.t[2] = 1024;
        gGlobalTimer = 0;
        jpb_WHookSetScreenPolyHook(
            capture_screen_poly, &poly_trace);
        fx_Water(
            &water_position,
            2,
            1,
            UINT32_C(0x80402010),
            1.0f,
            4);
        CHECK(poly_trace.calls == 2);
        CHECK(poly_trace.vertexCount == 4);
        CHECK(poly_trace.noScale == 1);
        CHECK(poly_trace.vertices[0].x == 256.0f);
        CHECK(poly_trace.vertices[0].y == 32.0f);
        CHECK(poly_trace.vertices[0].z == 1280.0f);
        CHECK(poly_trace.vertices[1].x == 512.0f);
        CHECK(poly_trace.vertices[2].z == 1024.0f);
        CHECK(poly_trace.vertices[0].argb ==
              UINT32_C(0x80402010));
        CHECK(poly_trace.vertices[0].tu == 0.0f);
        CHECK(poly_trace.vertices[0].tv == 1.0f);
        CHECK(poly_trace.vertices[1].tu == 1.0f);
        CHECK(poly_trace.vertices[1].tv == 1.0f);
        CHECK(poly_trace.vertices[2].tu == 0.0f);
        CHECK(poly_trace.vertices[2].tv == 0.0f);
        CHECK(poly_trace.vertices[3].tu == 1.0f);
        CHECK(poly_trace.vertices[3].tv == 0.0f);
        fx_Water(
            &water_position,
            0,
            1,
            UINT32_C(0xffffffff),
            1.0f,
            4);
        CHECK(poly_trace.calls == 2);
    }

    {
        WorldData water_world = {0};
        _svector water_patch = {9, 10, 22, 0x0406};

        water_world.location.vx = 0x80ff - 9 * 0x100;
        water_world.location.vz = 22 * 0x100 - 0x7f00;
        gpWorld = &water_world;
        memset(clippingfrustrum, 0, sizeof(clippingfrustrum));
        memset(&CameraMatrix, 0, sizeof(CameraMatrix));
        CameraMatrix.m[0][0] = 1.0f;
        CameraMatrix.m[1][1] = 1.0f;
        CameraMatrix.m[2][2] = 1.0f;
        CameraMatrix.t[2] = 30000;
        gGlobalTimer = 0;
        memset(&poly_trace, 0, sizeof(poly_trace));
        drawsomecrappywater(
            &water_patch,
            1,
            1.0f,
            2.0f,
            3,
            4,
            UINT32_C(0x11223344),
            UINT32_C(0x55667788));
        CHECK(poly_trace.calls == 48);
        CHECK(poly_trace.noScale == 1);
        CHECK(poly_trace.vertices[0].x == 31872.0f);
        CHECK(poly_trace.vertices[0].y == 2552.0f);
        CHECK(poly_trace.vertices[0].z == 4272.0f);
        CHECK(poly_trace.vertices[1].x == 32128.0f);
        CHECK(poly_trace.vertices[2].z == 4016.0f);
        CHECK(poly_trace.vertices[0].argb ==
              UINT32_C(0x55667788));
        gpWorld = NULL;
        jpb_WHookSetScreenPolyHook(NULL, NULL);
    }

    puts("projection tests passed");
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    return 0;
}
