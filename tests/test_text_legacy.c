#include "jpb/game.h"
#include "jpb/debugtext.h"
#include "jpb/game.h"
#include "jpb/menu.h"
#include "jpb/resources.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/text.h"
#include "jpb/textutil.h"
#include "jpb/texture.h"
#include "jpb/whook.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                              \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct DrawEntry {
    _Material *texture;
    SCREENRECT destination;
    SCREENRECT source;
    CVECTOR color;
    float depth;
} DrawEntry;

typedef struct DrawCapture {
    DrawEntry entries[128];
    size_t count;
} DrawCapture;

typedef struct FrontendQuadCapture {
    int calls;
    int noScale;
    _Material *material;
    int vertexCount;
    JPBScreenPolyVertex vertices[4];
} FrontendQuadCapture;

typedef struct LegacyTextCapture {
    int calls;
    int tint;
    int alpha;
    int mode;
    int x;
    int y;
    float scale;
    int fontStyle;
    uint16_t text[32];
} LegacyTextCapture;

typedef struct LegacyText3DCapture {
    int calls;
    uint32_t color;
    int mode;
    float x;
    float y;
    float z;
    float scale;
    int fontStyle;
    uint16_t text[32];
} LegacyText3DCapture;

static DrawCapture *active_retail_capture;
static int debug_texture_load_count;
static unsigned debug_texture_option;
static int debug_texture_type;
static char debug_texture_filename[256];

static void *capture_debug_texture_load(
    void *user_data,
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height)
{
    ++debug_texture_load_count;
    debug_texture_option = option;
    debug_texture_type = material_type;
    (void)snprintf(
        debug_texture_filename,
        sizeof(debug_texture_filename),
        "%s",
        filename);
    *width = 64;
    *height = 32;
    return user_data;
}

static void capture_frontend_quad(
    void *user_data,
    _Material *material,
    uint32_t material_flags,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale)
{
    FrontendQuadCapture *capture = (FrontendQuadCapture *)user_data;

    (void)material_flags;
    ++capture->calls;
    capture->noScale = no_scale;
    capture->material = material;
    capture->vertexCount = vertex_count;
    if (vertex_count > 0 && vertex_count <= 4) {
        memcpy(
            capture->vertices,
            vertices,
            (size_t)vertex_count * sizeof(vertices[0]));
    }
}

static _TTF_Font *load_unit_metric_font(
    void *user_data, const char *path, int point_size)
{
    (void)path;
    (void)point_size;
    return (_TTF_Font *)user_data;
}

static int set_unit_metric_font_size(
    _TTF_Font *font, int point_size)
{
    (void)font;
    (void)point_size;
    return 0;
}

static int get_unit_glyph_metrics(
    _TTF_Font *font,
    unsigned short glyph,
    int *minimum_x,
    int *maximum_x,
    int *minimum_y,
    int *maximum_y,
    int *advance)
{
    (void)font;
    (void)glyph;
    *minimum_x = 0;
    *maximum_x = 1;
    *minimum_y = 0;
    *maximum_y = 1;
    *advance = 1;
    return 0;
}

static const char *get_unit_metric_error(void)
{
    return "unit metric error";
}

static void capture_legacy_text(
    void *user_data,
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    float scale_adjustment,
    int font_style,
    int depth_enabled,
    float depth,
    const uint16_t *text)
{
    LegacyTextCapture *capture = (LegacyTextCapture *)user_data;
    size_t index;

    (void)scale_adjustment;
    (void)depth_enabled;
    (void)depth;
    ++capture->calls;
    capture->tint = tint;
    capture->alpha = alpha;
    capture->mode = mode;
    capture->x = x;
    capture->y = y;
    capture->scale = scale;
    capture->fontStyle = font_style;
    for (index = 0; index + 1 < 32 && text[index] != 0; ++index) {
        capture->text[index] = text[index];
    }
    capture->text[index] = 0;
}

static void capture_legacy_text_3d(
    void *user_data,
    uint32_t color,
    int mode,
    float x,
    float y,
    float z,
    float scale,
    int font_style,
    const uint16_t *text)
{
    LegacyText3DCapture *capture =
        (LegacyText3DCapture *)user_data;
    size_t index;

    ++capture->calls;
    capture->color = color;
    capture->mode = mode;
    capture->x = x;
    capture->y = y;
    capture->z = z;
    capture->scale = scale;
    capture->fontStyle = font_style;
    for (index = 0; index + 1 < 32 && text[index] != 0; ++index) {
        capture->text[index] = text[index];
    }
    capture->text[index] = 0;
}

static uint32_t fnv1a(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t hash = UINT32_C(2166136261);
    size_t index;

    for (index = 0; index < size; ++index) {
        hash = (hash ^ bytes[index]) * UINT32_C(16777619);
    }
    return hash;
}

static void record_draw(
    DrawCapture *capture,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float depth)
{
    DrawEntry *entry;

    if (capture == NULL || capture->count >= 128) {
        return;
    }
    entry = &capture->entries[capture->count++];
    entry->texture = texture;
    entry->destination = *destination;
    if (source != NULL) {
        entry->source = *source;
    } else {
        memset(&entry->source, 0, sizeof(entry->source));
    }
    entry->color = color;
    entry->depth = depth;
}

static void linked_draw_hook(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float depth)
{
    record_draw(
        (DrawCapture *)user_data,
        texture,
        destination,
        source,
        color,
        depth);
}

#if defined(_WIN32)
typedef struct RetailFrontendCapture {
    int calls;
    _Material *material;
    int vertexCount;
    FRONTENDVERT vertices[4];
    float depth;
} RetailFrontendCapture;

static RetailFrontendCapture *active_retail_frontend_capture;

static void retail_draw_hook(
    _Material *texture,
    SCREENRECT destination,
    const SCREENRECT *source,
    CVECTOR color,
    float depth)
{
    record_draw(
        active_retail_capture,
        texture,
        &destination,
        source,
        color,
        depth);
}

static void retail_frontend_hook(
    _Material *material,
    int vertex_count,
    FRONTENDVERT *vertices,
    float depth)
{
    RetailFrontendCapture *capture = active_retail_frontend_capture;

    if (capture == NULL) {
        return;
    }
    ++capture->calls;
    capture->material = material;
    capture->vertexCount = vertex_count;
    capture->depth = depth;
    if (vertex_count > 0 && vertex_count <= 4) {
        memcpy(
            capture->vertices,
            vertices,
            (size_t)vertex_count * sizeof(vertices[0]));
    }
}
#endif

static void configure_font_specs(void)
{
    unsigned index;

    memset(fontSpec, 0, sizeof(fontSpec));
    for (index = 0; index < JPB_FONT_SPEC_COUNT; ++index) {
        fontSpec[index].xypage = (uint16_t)(30u + index % 100u);
        fontSpec[index].clut = (uint16_t)(index % 7u);
        fontSpec[index].y = (uint16_t)(index % 23u);
        fontSpec[index].x = (uint16_t)(index % 91u);
        fontSpec[index].h = (uint16_t)(7u + index % 5u);
        fontSpec[index].w = (uint16_t)(3u + index % 11u);
    }
    for (index = 0; index < 249; ++index) {
        menuTextures[index] =
            (_Material *)(uintptr_t)(UINT64_C(0x100000) + index * 0x100u);
    }
}

static int compare_captures(
    const DrawCapture *actual, const DrawCapture *expected)
{
    size_t index;

    CHECK(actual->count == expected->count);
    for (index = 0; index < actual->count; ++index) {
        const DrawEntry *a = &actual->entries[index];
        const DrawEntry *e = &expected->entries[index];

        CHECK(a->texture == e->texture);
        if (memcmp(
                &a->destination,
                &e->destination,
                sizeof(a->destination)) != 0) {
            fprintf(
                stderr,
                "draw %zu destination linked=%d,%d,%d,%d retail=%d,%d,%d,%d\n",
                index,
                a->destination.left,
                a->destination.top,
                a->destination.right,
                a->destination.bottom,
                e->destination.left,
                e->destination.top,
                e->destination.right,
                e->destination.bottom);
            return 1;
        }
        CHECK(memcmp(&a->source, &e->source, sizeof(a->source)) == 0);
        CHECK(memcmp(&a->color, &e->color, sizeof(a->color)) == 0);
        CHECK(memcmp(&a->depth, &e->depth, sizeof(a->depth)) == 0);
    }
    return 0;
}

#if defined(_WIN32)
static int compare_frontend_captures(
    const FrontendQuadCapture *linked,
    const RetailFrontendCapture *retail)
{
    int index;

    CHECK(linked->calls == retail->calls);
    CHECK(linked->material == retail->material);
    CHECK(linked->vertexCount == retail->vertexCount);
    for (index = 0; index < linked->vertexCount; ++index) {
        CHECK(linked->vertices[index].x == retail->vertices[index].x);
        CHECK(linked->vertices[index].y == retail->vertices[index].y);
        CHECK(linked->vertices[index].z == retail->depth);
        CHECK(linked->vertices[index].tu == retail->vertices[index].u);
        CHECK(linked->vertices[index].tv == retail->vertices[index].v);
    }
    return 0;
}
#endif

static int test_initialized_globals(void)
{
    CHECK(sizeof(FONT) == 6);
    CHECK(fnv1a(asciiRemap, sizeof(asciiRemap)) == UINT32_C(0x0e40f45c));
    CHECK(fnv1a(SmallFont, sizeof(SmallFont)) == UINT32_C(0x223cb424));
    CHECK(fnv1a(Colors, sizeof(Colors)) == UINT32_C(0xc7178a8f));
    CHECK(fnv1a(Size_Bold, sizeof(Size_Bold)) == UINT32_C(0x9af546d4));
    CHECK(U_Bold['A'] == 0.0039f);
    CHECK(V_Bold['A'] == 0.5313f);
    CHECK(U_System['A'] == 0.0078f);
    CHECK(U_System['z'] == 0.7891f);
    CHECK(V_System['A'] == 0.0469f);
    CHECK(V_System['z'] == 0.1055f);
    CHECK(MonospaceWidth == 10);
    CHECK(textClipping == 0);
    CHECK(textclipRect[0] == 280.0f);
    CHECK(textclipRect[1] == 304.0f);
    CHECK(textclipRect[2] == 381.0f);
    CHECK(textclipRect[3] == 357.0f);
    return 0;
}

static int test_legacy_text_metrics(void)
{
    int width;
    int height;
    float italic_x;
    unsigned italic_width;

    GetStringSize(0.75f, &width, &height, "AB\nC ");
    CHECK(width == 33);
    CHECK(height == 45);
    GetTextSize(0.75f, &width, &height, "%s\n%s", "AB", "C ");
    CHECK(width == 33);
    CHECK(height == 45);
    DebugTextSize(&width, &height, "%s", "AB\nC\fD");
    CHECK(width == 20);
    CHECK(height == 45);
    DebugStringSize(&width, &height, "ignored");
    CHECK(width == 20);
    CHECK(height == 45);

    configure_font_specs();
    CHECK(text_gGetLength(NULL, "%s", "AB") == 17);

    italic_width =
        (unsigned)sfont18_nfont[('A' - 32) * 6] +
        (unsigned)sfont18_nfont[('B' - 32) * 6];
    italic_x = 100.0f;
    iDrawChar(0, &italic_x, 0.0f, 'A', 0, 0, 1.0f);
    CHECK(italic_x ==
          100.0f + (float)sfont18_nfont[('A' - 32) * 6]);
    CHECK(iDrawString(
              0, 0, (unsigned char *)"AB\nC",
              10.0f, 20.0f, 1.0f,
              0, 0, 0, 0) == 0u);
    CHECK(iDrawString(
              0, 1, (unsigned char *)"AB",
              10.0f, 20.0f, 1.0f,
              0, 0, 0, 0) == italic_width);
    CHECK(iDrawString(
              0, 2, (unsigned char *)"AB",
              10.0f, 20.0f, -0.5f,
              0, 0, 0, 0) == italic_width / 2u);
    CHECK(itextWrite(
              0, 1.0f, 3, 1, 10, 20,
              "%s%c", "A", 'B') == (int)italic_width);
    UpdateMenus();
    return 0;
}

static int test_menu_group_and_box_behavior(void)
{
    MENU_ARTDEF group[] = {
        {0x800u, 0x8000u, 0, 0, 0},
        {17u, 3u, 4u, 5u, 6u},
        {0x9cu, 1u, 2u, 3u, 4u},
        {0, 0, 0, 0, 0},
    };
    DrawCapture capture = {0};
    DrawCapture box_capture = {0};
    DrawCapture test_box_capture = {0};
    _Material white_material = {0};
    optionstruct saved_options = OptionStruct;
    _Material *saved_white_material = whitemat;
    float saved_scale_adjustment = scaleAdjustmentMM;
    float saved_scale_x = gPSXDrawScaleX;
    float saved_scale_y = gPSXDrawScaleY;
    float saved_scale_w = gPSXDrawScaleW;
    float saved_scale_h = gPSXDrawScaleH;
    float saved_front_z = frontZ;
    uint8_t saved_front_rgb_offset = frontRGBoff;
    int saved_surface = mDrawingSurfaceId;

    CHECK(sizeof(MENU_ARTDEF) == 10);
    configure_font_specs();
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    whitemat = &white_material;
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    gPSXDrawScaleW = 1.0f;
    gPSXDrawScaleH = 1.0f;
    frontRGBoff = 0;
    mDrawingSurfaceId = 1;
    frontZ = 0.25f;

    jpb_WHookSetDrawTextureHook(linked_draw_hook, &capture);
    menuDrawGroup(group, 10u, 20u);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    CHECK(capture.count == 2);
    CHECK(capture.entries[0].destination.left == 13);
    CHECK(capture.entries[0].destination.top == 24);
    CHECK(capture.entries[0].destination.right == 18);
    CHECK(capture.entries[0].destination.bottom == 30);
    CHECK(capture.entries[0].color.r == 128);
    CHECK(capture.entries[0].color.g == 128);
    CHECK(capture.entries[0].color.b == 128);
    CHECK(capture.entries[0].color.cd == 255);
    CHECK(capture.entries[1].texture == &white_material);
    CHECK(capture.entries[1].destination.left == 13);
    CHECK(capture.entries[1].destination.top == 27);
    CHECK(capture.entries[1].destination.right == 16);
    CHECK(capture.entries[1].destination.bottom == 35);
    CHECK(capture.entries[1].color.r == 0);
    CHECK(capture.entries[1].color.g == 0);
    CHECK(capture.entries[1].color.b == 0);
    CHECK(capture.entries[1].color.cd == 255);

    memset(&capture, 0, sizeof(capture));
    frontZ = 0.25f;
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &capture);
    menuDrawGroupClip(
        group, 10u, 100001u, 0u, 0u, 1000u, 1000u);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    CHECK(capture.count == 2);
    CHECK(capture.entries[0].destination.left == 13);
    CHECK(capture.entries[0].destination.top == 4);
    CHECK(capture.entries[1].destination.left == 13);
    CHECK(capture.entries[1].destination.top == 4);
    CHECK(capture.entries[1].destination.right == 16);
    CHECK(capture.entries[1].destination.bottom == 12);

    memset(&capture, 0, sizeof(capture));
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &capture);
    menuDrawGroupClip(
        group, 10u, 20u, 0u, 0u, 0u, 1000u);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    CHECK(capture.count == 1);
    CHECK(capture.entries[0].texture == &white_material);

    memset(&capture, 0, sizeof(capture));
    frontZ = 0.25f;
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &capture);
    menuDrawGroupClipDepth(
        group, 10u, 100001u, 0u, 0u, 1000u, 1000u, 0.75f);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    CHECK(capture.count == 2);
    CHECK(capture.entries[0].destination.left == 13);
    CHECK(capture.entries[0].destination.top == 4);
    CHECK(capture.entries[0].depth == 0.75f);
    CHECK(capture.entries[1].destination.left == 13);
    CHECK(capture.entries[1].destination.top == 4);
    CHECK(capture.entries[1].depth == 0.75f);
    CHECK(frontZ == 0.25f);

    memset(&capture, 0, sizeof(capture));
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &capture);
    menuDrawGroup(NULL, 1u, 2u);
    menuDrawGroupClip(NULL, 1u, 2u, 3u, 4u, 5u, 6u);
    menuDrawGroupClipDepth(NULL, 1u, 2u, 3u, 4u, 5u, 6u, 0.5f);
    menuDrawGroupScale(group, 1u, 2u, 3u, 4u);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    CHECK(capture.count == 0);

    frontZ = 0.5f;
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &box_capture);
    menuBox(40u, 200u, -10, -20, 40, 30, 1, 2, 3);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    CHECK(box_capture.count == 9);
    CHECK(box_capture.entries[0].destination.left == 310);
    CHECK(box_capture.entries[0].destination.top == 220);
    CHECK(box_capture.entries[1].destination.left == 318);
    CHECK(box_capture.entries[1].destination.right == 342);
    CHECK(box_capture.entries[2].destination.left == 342);
    CHECK(box_capture.entries[3].destination.top == 228);
    CHECK(box_capture.entries[3].destination.bottom == 242);
    CHECK(box_capture.entries[6].destination.top == 242);
    CHECK(box_capture.entries[8].destination.left == 342);
    CHECK(box_capture.entries[8].destination.top == 242);
    CHECK(box_capture.entries[4].color.r == 1);
    CHECK(box_capture.entries[4].color.g == 2);
    CHECK(box_capture.entries[4].color.b == 3);
    CHECK(box_capture.entries[4].color.cd == 200);

    frontZ = 0.5f;
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &test_box_capture);
    menuBoxTest(
        (unsigned)-10, (unsigned)-20, 40u, 30u, 1, 2, 3);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    CHECK(test_box_capture.count == 9);

    OptionStruct = saved_options;
    whitemat = saved_white_material;
    scaleAdjustmentMM = saved_scale_adjustment;
    gPSXDrawScaleX = saved_scale_x;
    gPSXDrawScaleY = saved_scale_y;
    gPSXDrawScaleW = saved_scale_w;
    gPSXDrawScaleH = saved_scale_h;
    frontZ = saved_front_z;
    frontRGBoff = saved_front_rgb_offset;
    mDrawingSurfaceId = saved_surface;
    return 0;
}

static int test_debug_font_and_text_clip(void)
{
    FrontendQuadCapture capture = {0};
    int texture_token;
    float x;
    float y;
    float width;
    float height;
    float u;
    float u_width;
    float v;
    float v_height;

    memset(g_material, 0, sizeof(g_material));
    debug_texture_load_count = 0;
    FreeFont();
    CHECK(jpb_ResourceSetBasePath("C:/jpb") == 1);
    jpb_TextureSetPlatformHooks(
        capture_debug_texture_load, NULL, &texture_token);
    InitFont();
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    CHECK(debug_texture_load_count == 1);
    CHECK(debug_texture_option == UINT32_C(0x02000001));
    CHECK(debug_texture_type == 0);
    CHECK(strcmp(
              debug_texture_filename,
              "C:/jpb/res/default/a_dbfont.tga") == 0);
    FreeFont();

    debug_texture_load_count = 0;
    memset(g_material, 0, sizeof(g_material));
    CHECK(jpb_ResourceSetBasePath("C:/jpb-lazy") == 1);
    x = 12.0f;
    y = 34.0f;
    jpb_TextureSetPlatformHooks(
        capture_debug_texture_load, NULL, &texture_token);
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    DebugText(12.0f, &x, &y, UINT32_C(0x7f123456), "%s", "A");
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    CHECK(debug_texture_load_count == 1);
    CHECK(capture.calls == 1);
    CHECK(capture.vertexCount == 4);
    CHECK(capture.vertices[0].x == 12.0f);
    CHECK(capture.vertices[0].y == 34.0f);
    CHECK(capture.vertices[0].z == 9.9e-05f);
    CHECK(capture.vertices[0].argb == UINT32_C(0x7f123456));
    CHECK(capture.vertices[0].tu == U_System['A']);
    CHECK(capture.vertices[0].tv == V_System['A']);
    CHECK(capture.vertices[1].x == 22.0f);
    CHECK(capture.vertices[2].y == 49.0f);
    CHECK(x == 22.0f);
    CHECK(y == 34.0f);
    FreeFont();
    CHECK(jpb_ResourceSetBasePath("C:/jpb") == 1);

    x = 20.0f;
    y = 30.0f;
    jpb_TextureSetPlatformHooks(
        capture_debug_texture_load, NULL, &texture_token);
    DebugString(7.0f, &x, &y, 0, "\n\f");
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    CHECK(x == 7.0f);
    CHECK(y == 60.0f);
    FreeFont();

    memset(&capture, 0, sizeof(capture));
    debugReset();
    jpb_TextureSetPlatformHooks(
        capture_debug_texture_load, NULL, &texture_token);
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    scr_debugPrintfRed("%s", "A");
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    CHECK(capture.calls == 1);
    CHECK(capture.vertices[0].x == 20.0f);
    CHECK(capture.vertices[0].y == 40.0f);
    CHECK(capture.vertices[0].argb == UINT32_C(0xff80ff80));

    memset(&capture, 0, sizeof(capture));
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    scr_debugPrintfXYC(123, 234, 0x7f654321, "%c", 'A');
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    CHECK(capture.calls == 1);
    CHECK(capture.vertices[0].x == 123.0f);
    CHECK(capture.vertices[0].y == 234.0f);
    CHECK(capture.vertices[0].argb == UINT32_C(0x7f654321));
    FreeFont();

    memset(&capture, 0, sizeof(capture));
    memset(&CameraMatrix, 0, sizeof(CameraMatrix));
    CameraMatrix.m[0][0] = 1.0f;
    CameraMatrix.m[1][1] = 1.0f;
    CameraMatrix.m[2][2] = 1.0f;
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    scr_debugPrintfXYZ(0, 0, 100, "A");
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    CHECK(capture.calls == 1);
    CHECK(capture.vertices[0].x == 315.0f);
    CHECK(capture.vertices[0].y == 232.5f);
    CHECK(capture.vertices[0].argb == UINT32_C(0xff8090a0));
    FreeFont();

    {
        LegacyText3DCapture text_3d_capture = {0};
        MATRIX saved_camera_matrix = CameraMatrix;
        uint8_t saved_overlay_mode = OptionStruct.overlayMode;
        float expected_width =
            (Size_Bold['A'] + Size_Bold['B']) * 255.0f * 2.0f;
        int font_token;

        memset(&CameraMatrix, 0, sizeof(CameraMatrix));
        CameraMatrix.m[0][0] = 1.0f;
        CameraMatrix.m[1][1] = 1.0f;
        CameraMatrix.m[2][2] = 1.0f;
        CameraMatrix.t[0] = 100;
        CameraMatrix.t[1] = 200;
        CameraMatrix.t[2] = 1000;
        OptionStruct.overlayMode = 0;
        jpb_TextSetFontLoadHook(load_unit_metric_font, &font_token);
        jpb_TextUtilSetFontMetricsHooks(
            set_unit_metric_font_size,
            get_unit_glyph_metrics,
            get_unit_metric_error);
        ClearGlyphCache();
        jpb_TextResetFontCache();
        jpb_TextSetDraw3DHook(
            capture_legacy_text_3d, &text_3d_capture);
        Draw3dText(
            10.9f, 20.9f, 30.9f, 2.0f,
            UINT32_C(0xa1b2c3d4), "%s", "AB\nC");
        CHECK(text_3d_capture.calls == 0);

        OptionStruct.overlayMode = 1;
        Draw3dText(
            10.9f, 20.9f, 30.9f, 2.0f,
            UINT32_C(0xa1b2c3d4), "%s", "AB\nC");
        CHECK(text_3d_capture.calls == 1);
        CHECK(text_3d_capture.color == UINT32_C(0xa1b2c3d4));
        CHECK(text_3d_capture.mode == 0);
        CHECK(text_3d_capture.x ==
              110.0f - (float)((int)expected_width / 2));
        CHECK(text_3d_capture.y == 100.0f);
        CHECK(text_3d_capture.z == 1030.0f);
        CHECK(text_3d_capture.scale == 3.0f);
        CHECK(text_3d_capture.fontStyle == 2);
        CHECK(text_3d_capture.text[0] == 'A');
        CHECK(text_3d_capture.text[1] == 'B');
        CHECK(text_3d_capture.text[2] == '\n');
        CHECK(text_3d_capture.text[3] == 'C');

        memset(&text_3d_capture, 0, sizeof(text_3d_capture));
        CameraMatrix.t[2] = 400;
        Draw3dText(
            10.9f, 20.9f, 30.9f, 2.0f,
            UINT32_C(0xa1b2c3d4), "%s", "AB\nC");
        CHECK(text_3d_capture.calls == 0);
        jpb_TextSetDraw3DHook(NULL, NULL);
        jpb_TextSetFontLoadHook(NULL, NULL);
        jpb_TextUtilSetFontMetricsHooks(NULL, NULL, NULL);
        ClearGlyphCache();
        jpb_TextResetFontCache();
        CameraMatrix = saved_camera_matrix;
        OptionStruct.overlayMode = saved_overlay_mode;
    }

    memset(&capture, 0, sizeof(capture));
    jpb_TextureSetPlatformHooks(
        capture_debug_texture_load, NULL, &texture_token);
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    (void)DrawRectangle(2.0f, 3.0f, 4.0f, 5.0f, 0x10203040);
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    CHECK(capture.calls == 1);
    CHECK(capture.vertices[0].x == 2.0f);
    CHECK(capture.vertices[0].y == 3.0f);
    CHECK(capture.vertices[0].z == 9.68e-05f);
    CHECK(capture.vertices[0].argb == UINT32_C(0x10203040));
    CHECK(capture.vertices[1].x == 7.0f);
    CHECK(capture.vertices[2].y == 9.0f);

    memset(&capture, 0, sizeof(capture));
    frontZ = 0.75f;
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    (void)DrawZRectangle(4.0f, 6.0f, 8.0f, 10.0f, 0x50607080);
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    CHECK(capture.calls == 1);
    CHECK(capture.vertices[0].z == 0.75f);
    CHECK(capture.vertices[1].x == 13.0f);
    CHECK(capture.vertices[2].y == 17.0f);

    memset(&capture, 0, sizeof(capture));
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    (void)console_rectangle(1.0f, 2.0f, 3.0f, 4.0f, 0x11223344);
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    CHECK(capture.calls == 1);
    CHECK(capture.vertices[0].z == 9.36e-05f);
    CHECK(capture.vertices[1].x == 5.0f);
    CHECK(capture.vertices[2].y == 7.0f);

    memset(&capture, 0, sizeof(capture));
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    (void)Draw2dBox(10.0f, 20.0f, 30.0f, 40.0f, 0x55667788);
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    CHECK(capture.calls == 4);
    CHECK(capture.vertices[0].x == 40.0f);
    CHECK(capture.vertices[0].y == 20.0f);
    CHECK(capture.vertices[1].x == 42.0f);
    CHECK(capture.vertices[2].y == 61.0f);

    memset(&capture, 0, sizeof(capture));
    jpb_TextureSetPlatformHooks(
        capture_debug_texture_load, NULL, &texture_token);
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    console_text(50.0f, 60.0f, 0xaabbccdd, "A");
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    CHECK(capture.calls == 1);
    CHECK(capture.vertices[0].x == 50.0f);
    CHECK(capture.vertices[0].y == 60.0f);
    CHECK(capture.vertices[0].z == 8.72e-05f);
    CHECK(capture.vertices[0].argb == UINT32_C(0xaabbccdd));
    CHECK(capture.vertices[0].tu == 0.0625f);
    CHECK(capture.vertices[0].tv == 0.234375f);
    CHECK(capture.vertices[1].x == 58.0f);
    CHECK(capture.vertices[1].tu == 0.125f);
    CHECK(capture.vertices[2].y == 75.0f);
    CHECK(capture.vertices[2].tv == 0.3515625f);

    clearTextClip();
    memset(&capture, 0, sizeof(capture));
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    CHECK(DrawString(
              10.0f, 20.0f, 0.25f, 1.0f,
              0x12345678, "A") == 23);
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    CHECK(capture.calls == 1);
    CHECK(capture.noScale == 0);
    CHECK(capture.vertices[0].x == 10.0f);
    CHECK(capture.vertices[0].y == 20.0f);
    CHECK(capture.vertices[0].z == 0.25f);
    CHECK(capture.vertices[0].tu == U_Bold['A']);
    CHECK(capture.vertices[0].tv == V_Bold['A']);
    CHECK(capture.vertices[1].x ==
          10.0f + Size_Bold['A'] * 255.0f);
    CHECK(capture.vertices[2].y == 50.0f);
    CHECK(capture.vertices[2].tv ==
          V_Bold['A'] - 0.11764706f);

    memset(&capture, 0, sizeof(capture));
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);
    CHECK(DrawString3D(
              3.0f, 4.0f, 0.75f, 1.0f,
              0x87654321, "A") == 23);
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    CHECK(capture.calls == 1);
    CHECK(capture.noScale == 1);
    CHECK(capture.vertices[0].z == 0.75f);

    {
        DrawCapture draw_capture = {0};
        const DrawEntry *entry;
        uint32_t saved_width = OptionStruct.ScreenWidth;
        uint32_t saved_height = OptionStruct.ScreenHeight;

        OptionStruct.ScreenWidth = 640;
        OptionStruct.ScreenHeight = 480;
        jpb_WHookSetDrawTextureHook(linked_draw_hook, &draw_capture);
        CHECK(DrawString2D(
                  10.0f, 20.0f, 0.375f, 1.0f,
                  UINT32_C(0xa1b2c3d4), "%s", "A") == 23);
        jpb_WHookSetDrawTextureHook(NULL, NULL);
        OptionStruct.ScreenWidth = saved_width;
        OptionStruct.ScreenHeight = saved_height;
        CHECK(draw_capture.count == 1);
        entry = &draw_capture.entries[0];
        CHECK(entry->destination.left == 10);
        CHECK(entry->destination.top == 20);
        CHECK(entry->destination.right == 33);
        CHECK(entry->destination.bottom == 50);
        CHECK(entry->source.left ==
              (int)roundf((float)entry->texture->iw * U_Bold['A']));
        CHECK(entry->source.top ==
              (int)roundf((float)entry->texture->ih * V_Bold['A']));
        CHECK(entry->source.right == (int)roundf(
                  (float)entry->texture->iw *
                  (U_Bold['A'] + Size_Bold['A'])));
        CHECK(entry->source.bottom == (int)roundf(
                  (float)entry->texture->ih *
                  (V_Bold['A'] - 0.11764706f)));
        CHECK(entry->color.r == 0xb2);
        CHECK(entry->color.g == 0xc3);
        CHECK(entry->color.b == 0xd4);
        CHECK(entry->color.cd == 0xa1);
        CHECK(entry->depth == 0.375f);
    }

    CHECK(iDrawIcon(100.0f, 0.0f, 128, 0, 1.0f) == 24u);
    CHECK(iDrawIcon(100.0f, 0.0f, 128, 0, -0.5f) == 12u);

    setTextClip(10, 20, 100, 50);
    CHECK(textClipping == 1);
    CHECK(textclipRect[0] == 10.0f);
    CHECK(textclipRect[1] == 20.0f);
    CHECK(textclipRect[2] == 110.0f);
    CHECK(textclipRect[3] == 70.0f);

    x = 5.0f;
    y = 15.0f;
    width = 20.0f;
    height = 20.0f;
    u = 0.0f;
    u_width = 1.0f;
    v = 0.0f;
    v_height = 1.0f;
    CHECK(textClip(
              &x, &y, &width, &height,
              &u, &u_width, &v, &v_height) == 1);
    CHECK(x == 10.0f);
    CHECK(y == 20.0f);
    CHECK(width == 15.0f);
    CHECK(height == 15.0f);
    CHECK(u == 0.25f);
    CHECK(u_width == 0.75f);
    CHECK(v == -0.25f);
    CHECK(v_height == 0.75f);

    x = 110.0f;
    y = 20.0f;
    width = 1.0f;
    height = 1.0f;
    CHECK(textClip(
              &x, &y, &width, &height,
              &u, &u_width, &v, &v_height) == 0);

    clearTextClip();
    CHECK(textClipping == 0);
    x = -100.0f;
    y = -100.0f;
    width = 1.0f;
    height = 1.0f;
    CHECK(textClip(
              &x, &y, &width, &height,
              &u, &u_width, &v, &v_height) == 1);
    return 0;
}

static int test_legacy_text_writers(void)
{
    LegacyTextCapture capture = {0};
    CVECTOR color = {1, 2, 3, 77};
    int font_token;

    scaleAdjustment = 1.0f;
    jpb_TextSetFontLoadHook(load_unit_metric_font, &font_token);
    jpb_TextUtilSetFontMetricsHooks(
        set_unit_metric_font_size,
        get_unit_glyph_metrics,
        get_unit_metric_error);
    ClearGlyphCache();
    jpb_TextResetFontCache();
    jpb_TextSetDrawHook(capture_legacy_text, &capture);

    frontZ = 0.5f;
    (void)textWrite(4, 0x102, 100, 50, "%s", "AB");
    CHECK(capture.calls == 1);
    CHECK(capture.x == 84);
    CHECK(capture.y == 50);
    CHECK(fabsf(capture.scale - 2.25f) < 0.0001f);
    CHECK(capture.alpha == 255);
    CHECK(capture.text[0] == 'A');
    CHECK(capture.text[1] == 'B');
    CHECK(frontZ == -0.5f);

    memset(&capture, 0, sizeof(capture));
    (void)textWriteColor(
        color, 1, 100, 60, 0.0f, "%s", "AB");
    CHECK(capture.calls == 1);
    CHECK(capture.x == 67);
    CHECK(capture.y == 60);
    CHECK(fabsf(capture.scale - 3.0f) < 0.0001f);
    CHECK(capture.alpha == 77);
    CHECK(frontZ == -0.5f);

    memset(&capture, 0, sizeof(capture));
    (void)textWriteScale(4, 1, 100, 70, 0.5f, "%s", "AB");
    CHECK(capture.calls == 1);
    CHECK(capture.x == 67);
    CHECK(capture.y == 70);
    CHECK(fabsf(capture.scale - 1.5f) < 0.0001f);
    CHECK(capture.alpha == 255);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_TextSetFontLoadHook(NULL, NULL);
    jpb_TextUtilSetFontMetricsHooks(NULL, NULL, NULL);
    ClearGlyphCache();
    jpb_TextResetFontCache();
    return 0;
}

static int test_win_draw_texture_machine_behavior(void)
{
    DrawCapture capture = {0};
    const DrawEntry *entry;
    int width;

    configure_font_specs();
    fontSpec[17].clut = 3;
    fontSpec[17].y = 11;
    fontSpec[17].x = 7;
    fontSpec[17].h = 5;
    fontSpec[17].w = 4;
    scaleAdjustmentMM = 1.5f;
    frontZ = 0.25f;
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &capture);
    width = winDrawTexture(
        17u | 0x8000u,
        10,
        20,
        6,
        8,
        0x8100u,
        0x123,
        0x245,
        -1);
    jpb_WHookSetDrawTextureHook(NULL, NULL);

    CHECK(width == 9);
    CHECK(capture.count == 1);
    entry = &capture.entries[0];
    CHECK(entry->texture == menuTextures[3]);
    CHECK(entry->destination.left == 10);
    CHECK(entry->destination.top == 20);
    CHECK(entry->destination.right == 19);
    CHECK(entry->destination.bottom == 32);
    CHECK(entry->source.left == 11);
    CHECK(entry->source.top == 11);
    CHECK(entry->source.right == 7);
    CHECK(entry->source.bottom == 16);
    CHECK(entry->color.r == 0x23);
    CHECK(entry->color.g == 0x45);
    CHECK(entry->color.b == 0xff);
    CHECK(entry->color.cd == 0x7f);
    CHECK(memcmp(&entry->depth, &frontZ, sizeof(frontZ)) == 0);
    return 0;
}

static int test_frontend_quad_machine_behavior(void)
{
    FrontendQuadCapture capture = {0};
    _Material material = {0};
    int texture_token;
    int width;
    float after_flip;

    configure_font_specs();
    fontSpec[17].clut = 3;
    fontSpec[17].x = 32;
    fontSpec[17].y = 16;
    fontSpec[17].w = 8;
    fontSpec[17].h = 4;
    material.texture = &texture_token;
    material.iw = 128;
    material.ih = 64;
    menuTextures[3] = &material;
    frontRGBoff = 9;
    frontZ = 0.25f;
    jpb_WHookSetScreenPolyHook(capture_frontend_quad, &capture);

    width = psxDrawFlip(
        17u, 100, 200, 40, 20, 0x8400u,
        250, -20, 30, 'H');
    CHECK(width == 40);
    CHECK(capture.calls == 1);
    CHECK(capture.material == &material);
    CHECK(capture.vertexCount == 4);
    CHECK(capture.vertices[0].x == 100.0f);
    CHECK(capture.vertices[0].y == 200.0f);
    CHECK(capture.vertices[1].x == 140.0f);
    CHECK(capture.vertices[2].y == 220.0f);
    CHECK(capture.vertices[0].tu == 40.0f / 128.0f);
    CHECK(capture.vertices[1].tu == 32.0f / 128.0f);
    CHECK(capture.vertices[0].tv == 16.0f / 64.0f);
    CHECK(capture.vertices[2].tv == 20.0f / 64.0f);
    CHECK(capture.vertices[0].z == 0.25f);
    after_flip = (float)((double)0.25f + 0.001);
    CHECK(memcmp(&frontZ, &after_flip, sizeof(frontZ)) == 0);

    memset(&capture, 0, sizeof(capture));
    width = winDrawZFlip(
        17u, 3, 4, 7, 0, 0, 0x1234u,
        1, 2, 3, 'v');
    CHECK(width == 8);
    CHECK(capture.calls == 1);
    CHECK(capture.vertices[0].x == 3.0f);
    CHECK(capture.vertices[1].x == 11.0f);
    CHECK(capture.vertices[2].y == 8.0f);
    CHECK(capture.vertices[0].tu == 32.0f / 128.0f);
    CHECK(capture.vertices[0].tv == 20.0f / 64.0f);
    CHECK(capture.vertices[2].tv == 16.0f / 64.0f);
    CHECK(capture.vertices[0].z == 7.0f);
    CHECK(memcmp(&frontZ, &after_flip, sizeof(frontZ)) == 0);

    memset(&capture, 0, sizeof(capture));
    width = psxDrawZTexture(
        17u | 0x8000u, 12, 14, 9, 16, 6, 0,
        4, 5, 6);
    CHECK(width == 16);
    CHECK(capture.calls == 1);
    CHECK(capture.vertices[0].x == 12.0f);
    CHECK(capture.vertices[1].x == 28.0f);
    CHECK(capture.vertices[2].y == 20.0f);
    CHECK(capture.vertices[0].tu == 40.0f / 128.0f);
    CHECK(capture.vertices[1].tu == 32.0f / 128.0f);
    CHECK(capture.vertices[0].tv == 16.0f / 64.0f);
    CHECK(capture.vertices[0].z == 9.0f);
    CHECK(memcmp(&frontZ, &after_flip, sizeof(frontZ)) == 0);

    jpb_WHookSetScreenPolyHook(NULL, NULL);
    return 0;
}

static int test_psx_draw_char_machine_behavior(void)
{
    DrawCapture capture = {0};
    int x = 10;
    int result;

    configure_font_specs();
    fontSpec[17].xypage = 40;
    fontSpec[17].clut = 3;
    fontSpec[17].y = 11;
    fontSpec[17].x = 7;
    fontSpec[17].h = 5;
    fontSpec[17].w = 4;
    scaleAdjustmentMM = 1.0f;
    frontZ = 0.0f;
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &capture);
    result = psxDrawChar(
        17, &x, 20, 12, 34, 56, 0x1000, 2);
    jpb_WHookSetDrawTextureHook(NULL, NULL);

    CHECK(result == 1);
    CHECK(x == 16);
    CHECK(capture.count == 1);
    CHECK(capture.entries[0].texture == menuTextures[3]);
    CHECK(capture.entries[0].destination.left == 10);
    CHECK(capture.entries[0].destination.top == 56);
    CHECK(capture.entries[0].destination.right == 14);
    CHECK(capture.entries[0].destination.bottom == 61);
    CHECK(capture.entries[0].color.r == 12);
    CHECK(capture.entries[0].color.g == 34);
    CHECK(capture.entries[0].color.b == 56);
    CHECK(capture.entries[0].color.cd == 0);
    return 0;
}

static int test_psx_string_out_machine_behavior(void)
{
    DrawCapture capture = {0};
    unsigned glyph_a = asciiRemap['A'];
    unsigned glyph_b = asciiRemap['B'];
    unsigned char text[] = "A\nB";
    int result;

    configure_font_specs();
    fontSpec[glyph_a].xypage = 40;
    fontSpec[glyph_a].h = 10;
    fontSpec[glyph_a].w = 5;
    fontSpec[glyph_b].xypage = 50;
    fontSpec[glyph_b].h = 12;
    fontSpec[glyph_b].w = 7;
    scaleAdjustmentMM = 1.0f;
    frontRGBoff = 2;
    frontZ = 0.0f;
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &capture);
    result = psxStringOut(
        text, 100, 30, 300, 0, 10, 250, 0x1000, 0);
    jpb_WHookSetDrawTextureHook(NULL, NULL);

    CHECK(result == 0);
    CHECK(capture.count == 2);
    CHECK(capture.entries[0].destination.left == 100);
    CHECK(capture.entries[0].destination.top == 65);
    CHECK(capture.entries[1].destination.left == 100);
    CHECK(capture.entries[1].destination.top == 99);
    CHECK(capture.entries[0].color.r == 0);
    CHECK(capture.entries[0].color.g == 12);
    CHECK(capture.entries[0].color.b == 252);
    return 0;
}

static int test_text_write_sub_behavior(void)
{
    DrawCapture capture = {0};
    uint8_t text[] = "AB\nA";
    unsigned glyph_a;
    unsigned glyph_b;
    int width;

    configure_font_specs();
    glyph_a = asciiRemap['A'];
    glyph_b = asciiRemap['B'];
    fontSpec[glyph_a].xypage = 40;
    fontSpec[glyph_a].h = 10;
    fontSpec[glyph_a].w = 5;
    fontSpec[glyph_b].xypage = 50;
    fontSpec[glyph_b].h = 12;
    fontSpec[glyph_b].w = 7;
    scaleAdjustmentMM = 1.0f;
    frontRGBoff = 2;
    frontZ = 0.0f;
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &capture);
    width = Text_gWriteSub(0x1000, 127, 3, 2, 100, 30, text);
    jpb_WHookSetDrawTextureHook(NULL, NULL);

    CHECK(width == 16);
    CHECK(capture.count == 3);
    CHECK(capture.entries[0].destination.left == 92);
    CHECK(capture.entries[0].destination.top == 60);
    CHECK(capture.entries[1].destination.left == 99);
    CHECK(capture.entries[1].destination.top == 68);
    CHECK(capture.entries[2].destination.left == 92);
    CHECK(capture.entries[2].destination.top == 86);
    CHECK(capture.entries[0].color.r == 0);
    CHECK(capture.entries[0].color.g == 129);
    CHECK(capture.entries[0].color.b == 0);
    CHECK(capture.entries[0].color.cd == 0);
    return 0;
}

#if defined(_WIN32)
typedef struct RetailImage {
    uint8_t *base;
    size_t size;
} RetailImage;

static int load_file(
    const char *path, uint8_t **image_out, size_t *size_out)
{
    FILE *file = fopen(path, "rb");
    long length;
    uint8_t *image;

    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) {
            fclose(file);
        }
        return 0;
    }
    length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    image = (uint8_t *)malloc((size_t)length);
    if (image == NULL ||
        fread(image, 1, (size_t)length, file) != (size_t)length) {
        free(image);
        fclose(file);
        return 0;
    }
    fclose(file);
    *image_out = image;
    *size_out = (size_t)length;
    return 1;
}

static RetailImage map_retail_image(const char *path)
{
    uint8_t *file_image = NULL;
    size_t file_size = 0;
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS64 *nt;
    const IMAGE_SECTION_HEADER *section;
    RetailImage mapped = {0};
    unsigned index;

    if (!load_file(path, &file_image, &file_size) ||
        file_size < sizeof(*dos)) {
        free(file_image);
        return mapped;
    }
    dos = (const IMAGE_DOS_HEADER *)(const void *)file_image;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 ||
        (size_t)dos->e_lfanew + sizeof(*nt) > file_size) {
        free(file_image);
        return mapped;
    }
    nt = (const IMAGE_NT_HEADERS64 *)(const void *)(
        file_image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        free(file_image);
        return mapped;
    }
    mapped.size = nt->OptionalHeader.SizeOfImage;
    mapped.base = (uint8_t *)VirtualAlloc(
        NULL,
        mapped.size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE);
    if (mapped.base == NULL) {
        mapped.size = 0;
        free(file_image);
        return mapped;
    }
    memset(mapped.base, 0, mapped.size);
    memcpy(
        mapped.base,
        file_image,
        nt->OptionalHeader.SizeOfHeaders < file_size
            ? nt->OptionalHeader.SizeOfHeaders
            : file_size);
    section = IMAGE_FIRST_SECTION(nt);
    for (index = 0; index < nt->FileHeader.NumberOfSections; ++index) {
        size_t source = section[index].PointerToRawData;
        size_t destination = section[index].VirtualAddress;
        size_t length = section[index].SizeOfRawData;

        if (source <= file_size && length <= file_size - source &&
            destination <= mapped.size && length <= mapped.size - destination) {
            memcpy(mapped.base + destination, file_image + source, length);
        }
    }
    free(file_image);
    return mapped;
}

static void patch_absolute_jump(uint8_t *address, const void *target)
{
    uint8_t patch[12] = {0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xe0};

    memcpy(patch + 2, &target, sizeof(target));
    memcpy(address, patch, sizeof(patch));
    FlushInstructionCache(GetCurrentProcess(), address, sizeof(patch));
}

static int test_retail_differential(const char *path)
{
    typedef int (*RetailWriteSub)(
        int, int8_t, int, int, int, int, uint8_t *);
    typedef int (*RetailWinDrawTexture)(
        unsigned, unsigned, unsigned, int, int, unsigned, int, int, int);
    typedef int (*RetailPsxDrawTextureClip)(
        unsigned, unsigned, unsigned, int, int, unsigned,
        int, int, int, unsigned, unsigned, unsigned, unsigned);
    typedef int (*RetailPsxDrawTextureClipDepth)(
        unsigned, unsigned, unsigned, int, int, unsigned,
        int, int, int, unsigned, unsigned, unsigned, unsigned, float);
    typedef int (*RetailPsxDrawFlip)(
        unsigned, unsigned, unsigned, int, int, unsigned,
        int, int, int, char);
    typedef int (*RetailPsxDrawZFlip)(
        unsigned, unsigned, unsigned, unsigned, int, int, unsigned,
        int, int, int, char);
    typedef int (*RetailPsxDrawZTexture)(
        unsigned, unsigned, unsigned, unsigned, int, int, unsigned,
        int, int, int);
    typedef void (*RetailMenuBox)(
        unsigned, unsigned, int, int, int, int, int, int, int);
    typedef void (*RetailMenuBoxTest)(
        unsigned, unsigned, unsigned, unsigned, int, int, int);
    typedef void (*RetailMenuDrawGroup)(
        MENU_ARTDEF *, unsigned, unsigned);
    typedef void (*RetailMenuDrawGroupClip)(
        MENU_ARTDEF *, unsigned, unsigned, unsigned, unsigned,
        unsigned, unsigned);
    typedef void (*RetailMenuDrawGroupClipDepth)(
        MENU_ARTDEF *, unsigned, unsigned, unsigned, unsigned,
        unsigned, unsigned, float);
    typedef void (*RetailMenuDrawGroupScale)(
        MENU_ARTDEF *, unsigned, unsigned, unsigned, unsigned);
    typedef void (*RetailGetStringSize)(float, int *, int *, char *);
    typedef void (*RetailDebugStringSize)(int *, int *, char *);
    typedef void (*RetailSetTextClip)(
        unsigned, unsigned, unsigned, unsigned);
    typedef int (*RetailTextClip)(
        float *, float *, float *, float *,
        float *, float *, float *, float *);
    RetailImage image = map_retail_image(path);
    RetailWriteSub retail_write_sub;
    RetailWinDrawTexture retail_win_draw_texture;
    RetailPsxDrawTextureClip retail_psx_draw_texture_clip;
    RetailPsxDrawTextureClipDepth retail_psx_draw_texture_clip_depth;
    RetailPsxDrawFlip retail_psx_draw_flip;
    RetailPsxDrawZFlip retail_psx_draw_z_flip;
    RetailPsxDrawZTexture retail_psx_draw_z_texture;
    RetailMenuBox retail_menu_box;
    RetailMenuBox retail_menu_box_mm;
    RetailMenuBoxTest retail_menu_box_test;
    RetailMenuDrawGroup retail_menu_draw_group;
    RetailMenuDrawGroupClip retail_menu_draw_group_clip;
    RetailMenuDrawGroupClipDepth retail_menu_draw_group_clip_depth;
    RetailMenuDrawGroupScale retail_menu_draw_group_scale;
    RetailGetStringSize retail_get_string_size;
    RetailDebugStringSize retail_debug_string_size;
    RetailSetTextClip retail_set_text_clip;
    RetailTextClip retail_text_clip;
    DrawCapture linked_capture = {0};
    DrawCapture retail_capture = {0};
    FrontendQuadCapture linked_frontend_capture = {0};
    RetailFrontendCapture retail_frontend_capture = {0};
    _Material frontend_material = {0};
    int frontend_texture_token;
    uint8_t text[] = "AB\n C";
    float retail_front_z;
    int linked_result;
    int retail_result;
    int linked_width;
    int linked_height;
    int retail_width;
    int retail_height;

    CHECK(image.base != NULL);
    CHECK(image.size > 0x125bd0u + 12u);
    CHECK(memcmp(
              U_Bold,
              image.base + 0x4b72e0u,
              sizeof(U_Bold)) == 0);
    CHECK(memcmp(
              V_Bold,
              image.base + 0x4b76e0u,
              sizeof(V_Bold)) == 0);
    CHECK(memcmp(
              U_System,
              image.base + 0x4b7ee0u,
              sizeof(U_System)) == 0);
    CHECK(memcmp(
              V_System,
              image.base + 0x4b82e0u,
              sizeof(V_System)) == 0);
    CHECK(memcmp(
              sfont18_nfont,
              image.base + 0x4cd6a0u,
              sizeof(sfont18_nfont)) == 0);
    patch_absolute_jump(image.base + 0x125bd0u, retail_draw_hook);
    patch_absolute_jump(image.base + 0x127cc0u, retail_frontend_hook);
    memcpy(image.base + 0x944780u, fontSpec, sizeof(fontSpec));
    memcpy(image.base + 0x5395b0u, menuTextures, sizeof(menuTextures));
    memcpy(image.base + 0x94477cu, &frontRGBoff, sizeof(frontRGBoff));
    memcpy(image.base + 0x547eb4u, &scaleAdjustmentMM, sizeof(scaleAdjustmentMM));

    retail_write_sub =
        (RetailWriteSub)(void *)(image.base + 0xfe880u);
    retail_win_draw_texture =
        (RetailWinDrawTexture)(void *)(image.base + 0x1016d0u);
    retail_psx_draw_texture_clip =
        (RetailPsxDrawTextureClip)(void *)(image.base + 0x1001b0u);
    retail_psx_draw_texture_clip_depth =
        (RetailPsxDrawTextureClipDepth)(void *)(image.base + 0x100540u);
    retail_psx_draw_flip =
        (RetailPsxDrawFlip)(void *)(image.base + 0xffdd0u);
    retail_psx_draw_z_flip =
        (RetailPsxDrawZFlip)(void *)(image.base + 0x100b50u);
    retail_psx_draw_z_texture =
        (RetailPsxDrawZTexture)(void *)(image.base + 0x100bb0u);
    retail_menu_box =
        (RetailMenuBox)(void *)(image.base + 0xfee10u);
    retail_menu_box_mm =
        (RetailMenuBox)(void *)(image.base + 0xff190u);
    retail_menu_box_test =
        (RetailMenuBoxTest)(void *)(image.base + 0xff570u);
    retail_menu_draw_group =
        (RetailMenuDrawGroup)(void *)(image.base + 0xff5c0u);
    retail_menu_draw_group_clip =
        (RetailMenuDrawGroupClip)(void *)(image.base + 0xff810u);
    retail_menu_draw_group_clip_depth =
        (RetailMenuDrawGroupClipDepth)(void *)(image.base + 0xffa80u);
    retail_menu_draw_group_scale =
        (RetailMenuDrawGroupScale)(void *)(image.base + 0xffd40u);
    retail_get_string_size =
        (RetailGetStringSize)(void *)(image.base + 0x45170u);
    retail_debug_string_size =
        (RetailDebugStringSize)(void *)(image.base + 0x43920u);
    retail_set_text_clip =
        (RetailSetTextClip)(void *)(image.base + 0x45df0u);
    retail_text_clip =
        (RetailTextClip)(void *)(image.base + 0x45ee0u);

    GetStringSize(
        0.625f, &linked_width, &linked_height, "AB\nC ");
    retail_get_string_size(
        0.625f, &retail_width, &retail_height, "AB\nC ");
    CHECK(linked_width == retail_width);
    CHECK(linked_height == retail_height);
    DebugTextSize(
        &linked_width, &linked_height, "%s", "AB\nC\fD");
    memcpy(
        image.base + 0x51c0b0u,
        "AB\nC\fD",
        sizeof("AB\nC\fD"));
    retail_debug_string_size(
        &retail_width, &retail_height, "ignored");
    CHECK(linked_width == retail_width);
    CHECK(linked_height == retail_height);
    {
        float linked_values[8] = {
            5.0f, 15.0f, 20.0f, 20.0f,
            0.0f, 1.0f, 0.0f, 1.0f};
        float retail_values[8];

        memcpy(retail_values, linked_values, sizeof(retail_values));
        setTextClip(10, 20, 100, 50);
        retail_set_text_clip(10, 20, 100, 50);
        linked_result = textClip(
            &linked_values[0], &linked_values[1],
            &linked_values[2], &linked_values[3],
            &linked_values[4], &linked_values[5],
            &linked_values[6], &linked_values[7]);
        retail_result = retail_text_clip(
            &retail_values[0], &retail_values[1],
            &retail_values[2], &retail_values[3],
            &retail_values[4], &retail_values[5],
            &retail_values[6], &retail_values[7]);
        CHECK(linked_result == retail_result);
        if (memcmp(
                linked_values,
                retail_values,
                sizeof(linked_values)) != 0) {
            int value_index;

            for (value_index = 0; value_index < 8; ++value_index) {
                fprintf(
                    stderr,
                    "textClip[%d] linked=%.9g retail=%.9g\n",
                    value_index,
                    linked_values[value_index],
                    retail_values[value_index]);
            }
        }
        CHECK(memcmp(
                  linked_values,
                  retail_values,
                  sizeof(linked_values)) == 0);

        linked_values[0] = 0.0f;
        linked_values[1] = 0.0f;
        linked_values[2] = 200.0f;
        linked_values[3] = 100.0f;
        linked_values[4] = 0.0f;
        linked_values[5] = 1.0f;
        linked_values[6] = 0.0f;
        linked_values[7] = 1.0f;
        memcpy(retail_values, linked_values, sizeof(retail_values));
        linked_result = textClip(
            &linked_values[0], &linked_values[1],
            &linked_values[2], &linked_values[3],
            &linked_values[4], &linked_values[5],
            &linked_values[6], &linked_values[7]);
        retail_result = retail_text_clip(
            &retail_values[0], &retail_values[1],
            &retail_values[2], &retail_values[3],
            &retail_values[4], &retail_values[5],
            &retail_values[6], &retail_values[7]);
        CHECK(linked_result == retail_result);
        if (memcmp(
                linked_values,
                retail_values,
                sizeof(linked_values)) != 0) {
            int value_index;

            for (value_index = 0; value_index < 8; ++value_index) {
                fprintf(
                    stderr,
                    "textClip-wide[%d] linked=%.9g retail=%.9g\n",
                    value_index,
                    linked_values[value_index],
                    retail_values[value_index]);
            }
        }
        CHECK(memcmp(
                  linked_values,
                  retail_values,
                  sizeof(linked_values)) == 0);
    }

    frontZ = 0.125f;
    memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
    linked_result = Text_gWriteSub(
        0x2000, -97, 12, 0x66, 73, 41, text);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    active_retail_capture = &retail_capture;
    retail_result = retail_write_sub(
        0x2000, -97, 12, 0x66, 73, 41, text);
    active_retail_capture = NULL;
    CHECK(linked_result == retail_result);
    CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
    memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
    CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

    {
        MENU_ARTDEF group[] = {
            {0x800u, 0x8000u, 0, 0, 0},
            {17u, 3u, 4u, 5u, 6u},
            {0x9cu, 1u, 2u, 3u, 4u},
            {0, 0, 0, 0, 0},
        };
        _Material white_material = {0};
        _Material *white_pointer = &white_material;

        configure_font_specs();
        fontSpec[17].clut = 3;
        fontSpec[17].x = 7;
        fontSpec[17].y = 11;
        fontSpec[17].w = 4;
        fontSpec[17].h = 5;
        OptionStruct.ScreenWidth = 1024;
        OptionStruct.ScreenHeight = 768;
        whitemat = white_pointer;
        scaleAdjustmentMM = 1.25f;
        gPSXDrawScaleX = 1.25f;
        gPSXDrawScaleY = 0.75f;
        gPSXDrawScaleW = 1.5f;
        gPSXDrawScaleH = 0.5f;
        frontRGBoff = 7;
        mDrawingSurfaceId = 1;
        memcpy(image.base + 0x944780u, fontSpec, sizeof(fontSpec));
        memcpy(image.base + 0x5395b0u, menuTextures, sizeof(menuTextures));
        memcpy(image.base + 0x10da100u, &OptionStruct, sizeof(OptionStruct));
        memcpy(image.base + 0x539d78u, &white_pointer, sizeof(white_pointer));
        memcpy(image.base + 0x547eb4u, &scaleAdjustmentMM, sizeof(float));
        memcpy(image.base + 0x4ccc9cu, &gPSXDrawScaleX, sizeof(float));
        memcpy(image.base + 0x4cd144u, &gPSXDrawScaleY, sizeof(float));
        memcpy(image.base + 0x4cd148u, &gPSXDrawScaleW, sizeof(float));
        memcpy(image.base + 0x4cd14cu, &gPSXDrawScaleH, sizeof(float));
        memcpy(image.base + 0x94477cu, &frontRGBoff, sizeof(frontRGBoff));
        memcpy(
            image.base + 0x53d270u,
            &mDrawingSurfaceId,
            sizeof(mDrawingSurfaceId));

        memset(&linked_capture, 0, sizeof(linked_capture));
        memset(&retail_capture, 0, sizeof(retail_capture));
        frontZ = 0.125f;
        memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
        jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
        menuBox(33u, 0x8400u, -27, 13, 73, 51, 11, 22, 33);
        jpb_WHookSetDrawTextureHook(NULL, NULL);
        active_retail_capture = &retail_capture;
        retail_menu_box(
            33u, 0x8400u, -27, 13, 73, 51, 11, 22, 33);
        active_retail_capture = NULL;
        CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
        memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
        CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

        memset(&linked_capture, 0, sizeof(linked_capture));
        memset(&retail_capture, 0, sizeof(retail_capture));
        frontZ = 0.25f;
        memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
        jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
        menuBoxMM(44u, 200u, -9, 7, 21, 17, 44, 55, 66);
        jpb_WHookSetDrawTextureHook(NULL, NULL);
        active_retail_capture = &retail_capture;
        retail_menu_box_mm(
            44u, 200u, -9, 7, 21, 17, 44, 55, 66);
        active_retail_capture = NULL;
        CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
        memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
        CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

        OptionStruct.ScreenWidth = 2560;
        OptionStruct.ScreenHeight = 1080;
        memcpy(image.base + 0x10da100u, &OptionStruct, sizeof(OptionStruct));
        memset(&linked_capture, 0, sizeof(linked_capture));
        memset(&retail_capture, 0, sizeof(retail_capture));
        frontZ = 0.375f;
        memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
        jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
        menuBoxMM(52u, 0x8100u, 5, -11, 19, 23, 70, 80, 90);
        jpb_WHookSetDrawTextureHook(NULL, NULL);
        active_retail_capture = &retail_capture;
        retail_menu_box_mm(
            52u, 0x8100u, 5, -11, 19, 23, 70, 80, 90);
        active_retail_capture = NULL;
        CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
        memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
        CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

        OptionStruct.ScreenWidth = 1024;
        OptionStruct.ScreenHeight = 768;
        memcpy(image.base + 0x10da100u, &OptionStruct, sizeof(OptionStruct));
        memset(&linked_capture, 0, sizeof(linked_capture));
        memset(&retail_capture, 0, sizeof(retail_capture));
        frontZ = 0.5f;
        memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
        jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
        menuBoxTest(17u, 19u, 61u, 47u, 101, 102, 103);
        jpb_WHookSetDrawTextureHook(NULL, NULL);
        active_retail_capture = &retail_capture;
        retail_menu_box_test(17u, 19u, 61u, 47u, 101, 102, 103);
        active_retail_capture = NULL;
        CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
        memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
        CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

        memset(&linked_capture, 0, sizeof(linked_capture));
        memset(&retail_capture, 0, sizeof(retail_capture));
        frontZ = 0.625f;
        memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
        jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
        menuDrawGroup(group, 70000u, 90000u);
        jpb_WHookSetDrawTextureHook(NULL, NULL);
        active_retail_capture = &retail_capture;
        retail_menu_draw_group(group, 70000u, 90000u);
        active_retail_capture = NULL;
        CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
        memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
        CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

        memset(&linked_capture, 0, sizeof(linked_capture));
        memset(&retail_capture, 0, sizeof(retail_capture));
        frontZ = 0.75f;
        memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
        jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
        menuDrawGroupClip(
            group, 70000u, 100001u, 5u, 7u, 300u, 200u);
        jpb_WHookSetDrawTextureHook(NULL, NULL);
        active_retail_capture = &retail_capture;
        retail_menu_draw_group_clip(
            group, 70000u, 100001u, 5u, 7u, 300u, 200u);
        active_retail_capture = NULL;
        CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
        memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
        CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

        memset(&linked_capture, 0, sizeof(linked_capture));
        memset(&retail_capture, 0, sizeof(retail_capture));
        frontZ = 0.875f;
        memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
        jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
        menuDrawGroupClipDepth(
            group, 70000u, 100001u, 5u, 7u, 300u, 200u, 0.333f);
        jpb_WHookSetDrawTextureHook(NULL, NULL);
        active_retail_capture = &retail_capture;
        retail_menu_draw_group_clip_depth(
            group, 70000u, 100001u, 5u, 7u, 300u, 200u, 0.333f);
        active_retail_capture = NULL;
        CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
        memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
        CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

        memset(&linked_capture, 0, sizeof(linked_capture));
        memset(&retail_capture, 0, sizeof(retail_capture));
        jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
        menuDrawGroupScale(group, 1u, 2u, 3u, 4u);
        jpb_WHookSetDrawTextureHook(NULL, NULL);
        active_retail_capture = &retail_capture;
        retail_menu_draw_group_scale(group, 1u, 2u, 3u, 4u);
        active_retail_capture = NULL;
        CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
    }

    fontSpec[17].clut = 3;
    fontSpec[17].x = 32;
    fontSpec[17].y = 16;
    fontSpec[17].w = 8;
    fontSpec[17].h = 4;
    frontend_material.texture = &frontend_texture_token;
    frontend_material.iw = 128;
    frontend_material.ih = 64;
    menuTextures[3] = &frontend_material;
    frontRGBoff = 9;
    memcpy(image.base + 0x944780u, fontSpec, sizeof(fontSpec));
    memcpy(image.base + 0x5395b0u, menuTextures, sizeof(menuTextures));
    memcpy(image.base + 0x94477cu, &frontRGBoff, sizeof(frontRGBoff));

    frontZ = 0.25f;
    memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
    jpb_WHookSetScreenPolyHook(
        capture_frontend_quad, &linked_frontend_capture);
    linked_result = psxDrawFlip(
        17u, 100, 200, 40, 20, 0x8400u,
        250, -20, 30, 'H');
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    active_retail_frontend_capture = &retail_frontend_capture;
    retail_result = retail_psx_draw_flip(
        17u, 100, 200, 40, 20, 0x8400u,
        250, -20, 30, 'H');
    active_retail_frontend_capture = NULL;
    CHECK(linked_result == retail_result);
    CHECK(compare_frontend_captures(
              &linked_frontend_capture,
              &retail_frontend_capture) == 0);
    CHECK(retail_frontend_capture.vertices[0].r == 255);
    CHECK(retail_frontend_capture.vertices[0].g == -11);
    CHECK(retail_frontend_capture.vertices[0].b == 39);
    memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
    CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

    memset(&linked_frontend_capture, 0, sizeof(linked_frontend_capture));
    memset(&retail_frontend_capture, 0, sizeof(retail_frontend_capture));
    jpb_WHookSetScreenPolyHook(
        capture_frontend_quad, &linked_frontend_capture);
    linked_result = psxDrawZFlip(
        17u, 3, 4, 7, 0, 0, 0x1234u,
        1, 2, 3, 'v');
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    active_retail_frontend_capture = &retail_frontend_capture;
    retail_result = retail_psx_draw_z_flip(
        17u, 3, 4, 7, 0, 0, 0x1234u,
        1, 2, 3, 'v');
    active_retail_frontend_capture = NULL;
    CHECK(linked_result == retail_result);
    CHECK(compare_frontend_captures(
              &linked_frontend_capture,
              &retail_frontend_capture) == 0);
    memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
    CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

    memset(&linked_frontend_capture, 0, sizeof(linked_frontend_capture));
    memset(&retail_frontend_capture, 0, sizeof(retail_frontend_capture));
    jpb_WHookSetScreenPolyHook(
        capture_frontend_quad, &linked_frontend_capture);
    linked_result = psxDrawZTexture(
        17u | 0x8000u, 12, 14, 9, 16, 6, 0,
        4, 5, 6);
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    active_retail_frontend_capture = &retail_frontend_capture;
    retail_result = retail_psx_draw_z_texture(
        17u | 0x8000u, 12, 14, 9, 16, 6, 0,
        4, 5, 6);
    active_retail_frontend_capture = NULL;
    CHECK(linked_result == retail_result);
    CHECK(compare_frontend_captures(
              &linked_frontend_capture,
              &retail_frontend_capture) == 0);
    memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
    CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

    memset(&linked_capture, 0, sizeof(linked_capture));
    memset(&retail_capture, 0, sizeof(retail_capture));
    frontZ = -0.5f;
    memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
    linked_result = winDrawTexture(
        37u | 0xa000u, 0xfffffff0u, 18, 13, 7, 0x8400u, -3, 511, 64);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    active_retail_capture = &retail_capture;
    retail_result = retail_win_draw_texture(
        37u | 0xa000u, 0xfffffff0u, 18, 13, 7, 0x8400u, -3, 511, 64);
    active_retail_capture = NULL;
    CHECK(linked_result == retail_result);
    CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
    memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
    CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

    memset(&linked_capture, 0, sizeof(linked_capture));
    memset(&retail_capture, 0, sizeof(retail_capture));
    fontSpec[17].clut = 3;
    fontSpec[17].y = 11;
    fontSpec[17].x = 7;
    fontSpec[17].h = 5;
    fontSpec[17].w = 4;
    memcpy(image.base + 0x944780u, fontSpec, sizeof(fontSpec));
    gPSXDrawScaleX = 2.0f;
    gPSXDrawScaleY = 3.0f;
    gPSXDrawScaleW = 4.0f;
    gPSXDrawScaleH = 5.0f;
    memcpy(image.base + 0x4ccc9cu, &gPSXDrawScaleX, sizeof(float));
    memcpy(image.base + 0x4cd144u, &gPSXDrawScaleY, sizeof(float));
    memcpy(image.base + 0x4cd148u, &gPSXDrawScaleW, sizeof(float));
    memcpy(image.base + 0x4cd14cu, &gPSXDrawScaleH, sizeof(float));
    mDrawingSurfaceId = 1;
    memcpy(
        image.base + 0x53d270u,
        &mDrawingSurfaceId,
        sizeof(mDrawingSurfaceId));
    frontZ = 0.375f;
    memcpy(image.base + 0x539dc4u, &frontZ, sizeof(frontZ));
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
    linked_result = psxDrawTextureClip(
        17u | 0xa000u, 6, 8, 3, 4, 5,
        10, 20, 30, 7, 9, 2, 2);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    active_retail_capture = &retail_capture;
    retail_result = retail_psx_draw_texture_clip(
        17u | 0xa000u, 6, 8, 3, 4, 5,
        10, 20, 30, 7, 9, 2, 2);
    active_retail_capture = NULL;
    CHECK(linked_result == retail_result);
    CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
    memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
    CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

    memset(&linked_capture, 0, sizeof(linked_capture));
    memset(&retail_capture, 0, sizeof(retail_capture));
    jpb_WHookSetDrawTextureHook(linked_draw_hook, &linked_capture);
    linked_result = psxDrawTextureClipDepth(
        17u, 6, 8, 3, 4, 0x8100u,
        10, 20, 30, 7, 9, 2, 2, 0.77f);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    active_retail_capture = &retail_capture;
    retail_result = retail_psx_draw_texture_clip_depth(
        17u, 6, 8, 3, 4, 0x8100u,
        10, 20, 30, 7, 9, 2, 2, 0.77f);
    active_retail_capture = NULL;
    CHECK(linked_result == retail_result);
    CHECK(compare_captures(&linked_capture, &retail_capture) == 0);
    memcpy(&retail_front_z, image.base + 0x539dc4u, sizeof(retail_front_z));
    CHECK(memcmp(&frontZ, &retail_front_z, sizeof(frontZ)) == 0);

    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    gPSXDrawScaleW = 3.75f;
    gPSXDrawScaleH = 4.5f;
    mDrawingSurfaceId = 0;

    VirtualFree(image.base, 0, MEM_RELEASE);
    return 0;
}
#endif

int main(int argc, char **argv)
{
    CHECK(test_initialized_globals() == 0);
    CHECK(test_legacy_text_metrics() == 0);
    CHECK(test_menu_group_and_box_behavior() == 0);
    CHECK(test_debug_font_and_text_clip() == 0);
    CHECK(test_legacy_text_writers() == 0);
    CHECK(test_win_draw_texture_machine_behavior() == 0);
    CHECK(test_frontend_quad_machine_behavior() == 0);
    CHECK(test_psx_draw_char_machine_behavior() == 0);
    CHECK(test_psx_string_out_machine_behavior() == 0);
    CHECK(test_text_write_sub_behavior() == 0);
#if defined(_WIN32)
    if (argc == 3 && strcmp(argv[1], "--retail-exe") == 0) {
        CHECK(test_retail_differential(argv[2]) == 0);
    } else {
        CHECK(argc == 1);
    }
#else
    (void)argc;
    (void)argv;
#endif
    puts("legacy text tests passed");
    return 0;
}
