#include "jpb/projection.h"
#include "jpb/fx.h"
#include "jpb/game.h"
#include "jpb/level.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/software_renderer.h"
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
    int vertexCount;
    int noScale;
    JPBScreenPolyVertex vertices[4];
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

static void capture_screen_poly(
    void *user_data,
    _Material *material,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale)
{
    ScreenPolyTrace *trace = (ScreenPolyTrace *)user_data;

    ++trace->calls;
    trace->material = material;
    trace->vertexCount = vertex_count;
    trace->noScale = no_scale;
    if (vertex_count > 0 && vertex_count <= 4) {
        memcpy(
            trace->vertices,
            vertices,
            (size_t)vertex_count * sizeof(trace->vertices[0]));
    }
}

int main(void)
{
    FVECTOR eye = {0.0f, 0.0f, -10.0f};
    FVECTOR target = {0.0f, 0.0f, 0.0f};
    FVECTOR up = {0.0f, 1.0f, 0.0f};
    FVECTOR screen;
    MATRIX view;
    uint32_t glow_pixels[64 * 64] = {0};
    JPBSoftwareFramebuffer glow_framebuffer = {
        glow_pixels, 64, 64, 64};
    JPBSoftwareRenderStats glow_stats = {0};
    uint32_t poly_pixels[64 * 64] = {0};
    float poly_depth_values[64 * 64];
    JPBSoftwareFramebuffer poly_framebuffer = {
        poly_pixels, 64, 64, 64};
    JPBSoftwareDepthBuffer poly_depth = {
        poly_depth_values, 64, 64, 64};
    JPBSoftwareRenderStats poly_stats = {0};
    JPBScreenPolyVertex software_quad[4] = {
        {20.0f, 20.0f, 0.5f, UINT32_C(0xffffffff), 0.0f, 0.0f},
        {20.0f, 44.0f, 0.5f, UINT32_C(0xffffffff), 0.0f, 1.0f},
        {44.0f, 20.0f, 0.5f, UINT32_C(0xffffffff), 1.0f, 0.0f},
        {44.0f, 44.0f, 0.5f, UINT32_C(0xffffffff), 1.0f, 1.0f}
    };
    JPBScreenPolyVertex camera_quad[4] = {
        {-50.0f, 20.0f, 200.0f, UINT32_C(0xffffffff), 0.0f, 0.0f},
        {50.0f, 20.0f, 200.0f, UINT32_C(0xffffffff), 1.0f, 0.0f},
        {-50.0f, 20.0f, 100.0f, UINT32_C(0xffffffff), 0.0f, 1.0f},
        {50.0f, 20.0f, 100.0f, UINT32_C(0xffffffff), 1.0f, 1.0f}
    };
    _svector glow_start = {-10, 0, 0, 0};
    _svector glow_end = {10, 0, 0, 0};
    short clipped[2];
    ScreenPolyTrace poly_trace = {0};
    _Material material = {0};
    physicsObject arrow_physics = {0};
    playerObject arrow_player = {0};
    sceneObject arrow_scene = {0};
    WorldData arrow_world = {0};

    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
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
    _StartPoly(4, &material);
    _SetVert(0, 10.0f, 20.0f, 0.0001f, 0xff010203, 0.0f, 1.0f);
    _SetVert(1, 30.0f, 40.0f, 0.0001f, 0xff040506, 1.0f, 0.0f);
    _SetVert(2, 50.0f, 60.0f, 0.0001f, 0xff070809, 1.0f, 1.0f);
    _SetVert(3, 70.0f, 80.0f, 0.0001f, 0xff0a0b0c, 0.0f, 0.0f);
    _EndPoly();
    CHECK(poly_trace.calls == 1);
    CHECK(poly_trace.material == &material);
    CHECK(poly_trace.vertexCount == 4);
    CHECK(poly_trace.noScale == 0);
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
    CHECK(jpb_SoftwareDrawGlowLine(
              &glow_start,
              &glow_end,
              4,
              UINT32_C(0x7f40c0ff),
              &view,
              &glow_framebuffer,
              NULL,
              &glow_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(glow_stats.lines == 1);
    CHECK(glow_stats.modelLines == 1);
    CHECK(glow_stats.pixels > 0);
    CHECK(glow_pixels[32 * 64 + 32] != 0);
    memset(glow_pixels, 0, sizeof(glow_pixels));
    memset(&glow_stats, 0, sizeof(glow_stats));
    CHECK(jpb_SoftwareClearDepthBuffer(&poly_depth));
    poly_depth_values[32 * 64 + 32] = 500.0f / 10240.0f;
    CHECK(jpb_SoftwareDrawGlowLine(
              &glow_start,
              &glow_end,
              4,
              UINT32_C(0x7f40c0ff),
              &view,
              &glow_framebuffer,
              &poly_depth,
              &glow_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(glow_stats.lines == 1);
    CHECK(glow_stats.pixels > 0);
    CHECK(glow_pixels[32 * 64 + 32] == 0);
    CHECK(glow_stats.glowDepthRejectedPixels > 0);
    memset(glow_pixels, 0, sizeof(glow_pixels));
    memset(&glow_stats, 0, sizeof(glow_stats));
    for (int depth_index = 0; depth_index < 64 * 64; ++depth_index) {
        poly_depth_values[depth_index] = 500.0f / 10240.0f;
    }
    CHECK(jpb_SoftwareDrawGlowLine(
              &glow_start,
              &glow_end,
              4,
              UINT32_C(0x7f40c0ff),
              &view,
              &glow_framebuffer,
              &poly_depth,
              &glow_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(glow_stats.lines == 1);
    CHECK(glow_stats.pixels == 0);
    CHECK(glow_stats.glowDepthRejectedPixels > 0);
    memset(glow_pixels, 0, sizeof(glow_pixels));
    memset(&glow_stats, 0, sizeof(glow_stats));
    glow_start.vx = 0;
    glow_end = glow_start;
    CHECK(jpb_SoftwareDrawGlowLine(
              &glow_start,
              &glow_end,
              0,
              UINT32_C(0xffff0000),
              &view,
              &glow_framebuffer,
              NULL,
              &glow_stats) == JPB_SOFTWARE_RENDER_OK);
    CHECK(glow_pixels[32 * 64 + 32] == UINT32_C(0x00ff0000));
    CHECK(jpb_SoftwareDrawGlowLine(
              NULL,
              &glow_end,
              4,
              UINT32_C(0xffffffff),
              &view,
              &glow_framebuffer,
              NULL,
              &glow_stats) ==
          JPB_SOFTWARE_RENDER_INVALID_ARGUMENT);

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
            CHECK(triangle_trace.vertices[0].x == software_quad[0].x);
            CHECK(triangle_trace.vertices[0].y == software_quad[0].y);
            CHECK(triangle_trace.vertices[1].x == software_quad[1].x);
            CHECK(triangle_trace.vertices[1].y == software_quad[1].y);
            CHECK(triangle_trace.vertices[2].x == software_quad[2].x);
            CHECK(triangle_trace.vertices[2].y == software_quad[2].y);
            CHECK(triangle_trace.vertices[3].x == software_quad[1].x);
            CHECK(triangle_trace.vertices[3].y == software_quad[1].y);
            CHECK(triangle_trace.vertices[4].x == software_quad[3].x);
            CHECK(triangle_trace.vertices[4].y == software_quad[3].y);
            CHECK(triangle_trace.vertices[5].x == software_quad[2].x);
            CHECK(triangle_trace.vertices[5].y == software_quad[2].y);
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
        material.texture = NULL;
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
    return 0;
}
