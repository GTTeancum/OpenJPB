#include "jpb/portable_text.h"
#include "jpb/alltext.h"
#include "jpb/generic_hook.h"
#include "jpb/level_world.h"
#include "jpb/loader.h"
#include "jpb/menu.h"
#include "jpb/resources.h"
#include "jpb/sprite.h"
#include "jpb/text.h"
#include "jpb/utf16.h"
#include "jpb/whook.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed: %s (%s:%d)\n", \
                #condition, __FILE__, __LINE__); \
        jpb_PortableTextShutdown(); \
        return 1; \
    } \
} while (0)

typedef struct TestFontLoadTrace {
    int calls;
    int pointSize;
    char path[256];
    uintptr_t token[6];
} TestFontLoadTrace;

typedef struct Text3DTrace {
    int calls;
    _Material *materials[4];
    JPBScreenPolyVertex vertices[4][4];
} Text3DTrace;

typedef struct UITextWrapperTrace {
    int calls;
    const uint16_t *text;
    uint16_t copiedText[32];
    SCREENRECT destination;
    int fontStyle;
    int pointSize;
    CVECTOR color;
    int depthEnabled;
    float depth;
} UITextWrapperTrace;

typedef struct UITextWrapper3DTrace {
    int calls;
    const uint16_t *text;
    float x;
    float y;
    float z;
    int fontStyle;
    int pointSize;
    uint32_t color;
} UITextWrapper3DTrace;

static void capture_ui_text_wrapper(
    void *user_data,
    const uint16_t *text,
    const SCREENRECT *destination,
    int font_style,
    int point_size,
    CVECTOR color,
    int depth_enabled,
    float depth)
{
    UITextWrapperTrace *trace = (UITextWrapperTrace *)user_data;

    ++trace->calls;
    trace->text = text;
    {
        size_t index = 0;
        while (index + 1 < sizeof(trace->copiedText) /
                              sizeof(trace->copiedText[0]) &&
               text[index] != 0) {
            trace->copiedText[index] = text[index];
            ++index;
        }
        trace->copiedText[index] = 0;
    }
    trace->destination = *destination;
    trace->fontStyle = font_style;
    trace->pointSize = point_size;
    trace->color = color;
    trace->depthEnabled = depth_enabled;
    trace->depth = depth;
}

static void capture_ui_text_wrapper_3d(
    void *user_data,
    const uint16_t *text,
    float x,
    float y,
    float z,
    int font_style,
    int point_size,
    uint32_t color)
{
    UITextWrapper3DTrace *trace =
        (UITextWrapper3DTrace *)user_data;

    ++trace->calls;
    trace->text = text;
    trace->x = x;
    trace->y = y;
    trace->z = z;
    trace->fontStyle = font_style;
    trace->pointSize = point_size;
    trace->color = color;
}

static int test_exact_ui_text_wrappers(void)
{
    uint16_t text[] = {L'J', L'P', L'B', 0};
    SCREENRECT destination = {10, 20, 110, 60};
    CVECTOR color = {1, 2, 3, 4};
    UITextWrapperTrace trace;
    UITextWrapper3DTrace trace_3d;

    memset(&trace, 0, sizeof(trace));
    memset(&trace_3d, 0, sizeof(trace_3d));
    jpb_WHookSetDrawUITextUTF16Hook(
        capture_ui_text_wrapper, &trace);
    jpb_WHookSetDrawUITextUTF163DHook(
        capture_ui_text_wrapper_3d, &trace_3d);
    _DrawUITextUTF16(text, destination, 2, 48, color);
    CHECK(trace.calls == 1);
    CHECK(trace.text == text);
    CHECK(trace.destination.left == 10);
    CHECK(trace.destination.top == 20);
    CHECK(trace.destination.right == 110);
    CHECK(trace.destination.bottom == 60);
    CHECK(trace.fontStyle == 2);
    CHECK(trace.pointSize == 48);
    CHECK(trace.color.r == 1 && trace.color.g == 2);
    CHECK(trace.color.b == 3 && trace.color.cd == 4);
    CHECK(trace.depthEnabled == 0);
    _DrawUITextUTF16Depth(
        text, destination, 1, 36, color, 0.625f);
    CHECK(trace.calls == 2);
    CHECK(trace.fontStyle == 1);
    CHECK(trace.pointSize == 36);
    CHECK(trace.depthEnabled == 1);
    CHECK(trace.depth == 0.625f);
    _DrawUIText(
        "JPB narrow path",
        destination,
        3,
        60,
        color);
    CHECK(trace.calls == 3);
    CHECK(trace.copiedText[0] == 'J');
    CHECK(trace.copiedText[1] == 'P');
    CHECK(trace.copiedText[2] == 'B');
    CHECK(trace.copiedText[3] == ' ');
    CHECK(trace.copiedText[4] == 'n');
    CHECK(trace.copiedText[15] == 0);
    CHECK(trace.destination.left == 10);
    CHECK(trace.destination.bottom == 60);
    CHECK(trace.fontStyle == 3);
    CHECK(trace.pointSize == 60);
    CHECK(trace.depthEnabled == 0);
    _DrawUITextUTF16_3D(
        text, 1.0f, 2.0f, 3.0f,
        2, 72, UINT32_C(0x80402010));
    CHECK(trace_3d.calls == 1);
    CHECK(trace_3d.text == text);
    CHECK(trace_3d.x == 1.0f);
    CHECK(trace_3d.y == 2.0f);
    CHECK(trace_3d.z == 3.0f);
    CHECK(trace_3d.fontStyle == 2);
    CHECK(trace_3d.pointSize == 72);
    CHECK(trace_3d.color == UINT32_C(0x80402010));
    jpb_WHookSetDrawUITextUTF16Hook(NULL, NULL);
    jpb_WHookSetDrawUITextUTF163DHook(NULL, NULL);
    return 0;
}

static void capture_text_3d_glyph(
    void *user_data,
    _Material *material,
    const JPBScreenPolyVertex *vertices)
{
    Text3DTrace *trace = (Text3DTrace *)user_data;
    int index = trace->calls++;

    if (index < 4) {
        trace->materials[index] = material;
        memcpy(
            trace->vertices[index],
            vertices,
            sizeof(trace->vertices[index]));
    }
}

static _TTF_Font *test_load_font(
    void *user_data, const char *path, int point_size)
{
    TestFontLoadTrace *trace = (TestFontLoadTrace *)user_data;
    size_t length = path != NULL ? strlen(path) : 0;
    int slot = trace->calls;

    if (length >= sizeof(trace->path) || slot >= 6) {
        return NULL;
    }
    memcpy(trace->path, path, length + 1);
    trace->pointSize = point_size;
    ++trace->calls;
    return (_TTF_Font *)(void *)&trace->token[slot];
}

static int test_exact_font_contract(void)
{
    TestFontLoadTrace trace;
    _TTF_Font *font;

    memset(&trace, 0, sizeof(trace));
    CHECK(strcmp(
              jpb_PortableTextFontFileName(0, 0),
              "NotoSansSC-Regular.ttf") == 0);
    CHECK(strcmp(
              jpb_PortableTextFontFileName(1, 0),
              "NotoSans-Italic.ttf") == 0);
    CHECK(strcmp(
              jpb_PortableTextFontFileName(2, 0),
              "NotoSansSC-Bold.ttf") == 0);
    CHECK(strcmp(
              jpb_PortableTextFontFileName(0, 6),
              "NotoSansSC-Regular.ttf") == 0);
    CHECK(strcmp(
              jpb_PortableTextFontFileName(1, 6),
              "NotoSansSC-Light.ttf") == 0);
    CHECK(strcmp(
              jpb_PortableTextFontFileName(2, 6),
              "NotoSansSC-Bold.ttf") == 0);
    CHECK(jpb_PortableTextPointSize(3.24f, 1.125f) == 87);
    CHECK(jpb_PortableTextPointSize(0.0f, 1.0f) == 24);
    CHECK(jpb_PortableTextPointSize(1.0f, 0.0f) == 0);
    CHECK(jpb_PortableTextTint(0) == UINT32_C(0xff4060e0));
    CHECK(jpb_PortableTextTint(11) == UINT32_C(0xfff0f0f0));
    CHECK(jpb_PortableTextTint(16) == UINT32_C(0xffe09093));
    CHECK(jpb_ResourceSetBasePath("C:/game") == 1);
    currentLanguage = 0;
    CHECK(strcmp(
              getDefaultFontFile(0),
              "C:/game/res/font/NotoSansSC-Regular.ttf") == 0);
    CHECK(strcmp(
              getDefaultFontFile(1),
              "C:/game/res/font/NotoSans-Italic.ttf") == 0);
    CHECK(strcmp(
              getFontFile(2),
              "C:/game/res/font/NotoSansSC-Bold.ttf") == 0);
    jpb_TextResetFontCache();
    jpb_TextSetFontLoadHook(test_load_font, &trace);
    font = LoadFont(1, 72);
    CHECK(font != NULL);
    CHECK(trace.calls == 1);
    CHECK(trace.pointSize == 72);
    CHECK(strstr(trace.path, "NotoSans-Italic.ttf") != NULL);
    CHECK(LoadFont(1, 24) == font);
    CHECK(trace.calls == 1);
    currentLanguage = 6;
    CHECK(LoadFont(1, 48) != NULL);
    CHECK(trace.calls == 2);
    CHECK(trace.pointSize == 48);
    CHECK(strstr(trace.path, "NotoSansSC-Light.ttf") != NULL);
    jpb_TextSetFontLoadHook(NULL, NULL);
    jpb_TextResetFontCache();
    return 0;
}

static int test_recovered_localization(void)
{
    uint64_t aggregate_hash = UINT64_C(14695981039346656037);
    size_t aggregate_bytes = 0;
    int language;
    int index;

    CHECK(jpb_AllTextUtf8(-1, 0) == NULL);
    CHECK(jpb_AllTextUtf8(7, 0) == NULL);
    CHECK(jpb_AllTextUtf8(0, -1) == NULL);
    CHECK(jpb_AllTextUtf8(0, 498) == NULL);
    CHECK(sizeof(allTextEverything) / sizeof(allTextEverything[0]) ==
          JPB_ALL_TEXT_STORAGE_COUNT);
    CHECK(jpb_AllTextUtf8(0, 0) ==
          (const char *)(const void *)sModelNames);
    CHECK(jpb_AllTextUtf8(0, 1) ==
          (const char *)(const void *)sLevelNames);
    CHECK(allTextEverything[JPB_ALL_TEXT_STORAGE_COUNT - 1][0] == '\0');
    CHECK(strcmp(
              jpb_AllTextUtf8(0, 435),
              "Objective Complete!") == 0);
    CHECK(strcmp(
              jpb_AllTextUtf8(1, 435),
              "Ziel abgeschlossen!") == 0);
    CHECK(strcmp(
              jpb_AllTextUtf8(3, 435),
              "Obiettivo completato!") == 0);
    CHECK(strcmp(
              jpb_AllTextUtf8(6, 435),
              "\344\273\273\345\212\241\345\256\214\346\210\220"
              "\357\274\201") == 0);

    for (language = 0;
         language < JPB_ALL_TEXT_LANGUAGE_COUNT;
         ++language) {
        for (index = 2; index < JPB_ALL_TEXT_ENTRY_COUNT; ++index) {
            const unsigned char *text = (const unsigned char *)
                jpb_AllTextUtf8(language, index);

            CHECK(text != NULL);
            do {
                aggregate_hash ^= *text;
                aggregate_hash *= UINT64_C(1099511628211);
                ++aggregate_bytes;
            } while (*text++ != 0);
        }
    }
    CHECK(aggregate_bytes == 68317);
    CHECK(aggregate_hash == UINT64_C(0x61EB339CCD58C0EF));

    refreshFontAtlasFlag = 0;
    jpb_TextSetFontLoadHook(NULL, NULL);
    jpb_TextResetFontCache();
    generateAllText(0);
    CHECK(refreshFontAtlasFlag == 1);
    CHECK(currentLanguage == 0);
    CHECK(allText[0] == (char *)(void *)sModelNames);
    CHECK(allText[1] == (char *)(void *)sLevelNames);
    CHECK(allText[3] == jpb_AllTextUtf8(0, 3));
    CHECK(allText[435] == jpb_AllTextUtf8(0, 435));
    CHECK(strcmp(allText[435], "Objective Complete!") == 0);

    generateAllText(6);
    CHECK(currentLanguage == 6);
    CHECK(allText[3] == jpb_AllTextUtf8(6, 3));
    CHECK(allText[435] == jpb_AllTextUtf8(6, 435));
    CHECK(allText[435] != NULL);
    CHECK((unsigned char)allText[435][0] == UINT32_C(0xe4));
    CHECK((unsigned char)allText[435][1] == UINT32_C(0xbb));
    return 0;
}

static int test_real_font(const char *game_root)
{
    enum { WIDTH = 640, HEIGHT = 180 };
    JPBSoftwareFramebuffer framebuffer;
    JPBPortableTextMetrics metrics;
    uint32_t *pixels = (uint32_t *)calloc(
        (size_t)WIDTH * HEIGHT, sizeof(*pixels));
    size_t index;
    size_t changed = 0;
    uint16_t exit_prompt[32] = L"\x017d   Exit";
    uint16_t select_prompt[32] = L"\x20ac   Select";
    uint16_t multiline_prompt[32] = L"Line\n<A>";
    JPBPortableTextControlGlyph control_glyphs[2];
    Text3DTrace text_3d_trace;
    UITextWrapperTrace wrapper_trace;
    UITextWrapper3DTrace wrapper_3d_trace;
    int origin_x;
    int text_width;

    CHECK(pixels != NULL);
    memset(&text_3d_trace, 0, sizeof(text_3d_trace));
    memset(&wrapper_trace, 0, sizeof(wrapper_trace));
    memset(&wrapper_3d_trace, 0, sizeof(wrapper_3d_trace));
    CHECK(jpb_ResourceSetBasePath(game_root) == 1);
    jpb_PortableTextInstallHooks();
    currentLanguage = 0;
    CHECK(strstr(
              getFontFile(2),
              "res/font/NotoSansSC-Bold.ttf") != NULL);
    CHECK(strstr(
              getDefaultFontFile(1),
              "res/font/NotoSans-Italic.ttf") != NULL);
    currentLanguage = 6;
    CHECK(strstr(
              getFontFile(1),
              "res/font/NotoSansSC-Light.ttf") != NULL);
    currentLanguage = 0;
    jpb_WHookSetDrawUITextUTF16Hook(
        capture_ui_text_wrapper, &wrapper_trace);
    text_width = SDLTextWriteScale(
        11, 127, 1, 500, 25, 2.0f, 0, "%s", "TEST");
    CHECK(wrapper_trace.calls == 1);
    CHECK(wrapper_trace.destination.left == 500 - text_width);
    CHECK(wrapper_trace.destination.right == 500);
    CHECK(wrapper_trace.destination.top == 25);
    CHECK(wrapper_trace.destination.bottom > 25);
    CHECK(wrapper_trace.pointSize ==
          jpb_PortableTextPointSize(2.0f, scaleAdjustment));
    CHECK(wrapper_trace.color.r == Colors[11].r);
    CHECK(wrapper_trace.color.g == Colors[11].g);
    CHECK(wrapper_trace.color.b == Colors[11].b);
    CHECK(wrapper_trace.color.cd == 127);
    jpb_WHookSetDrawUITextUTF163DHook(
        capture_ui_text_wrapper_3d, &wrapper_3d_trace);
    text_width = SDLTextWriteScale3D(
        UINT32_C(0xff123456), 1,
        500.0f, 30.0f, 40.0f,
        2.0f, 0, "%s", "TEST");
    CHECK(wrapper_3d_trace.calls == 1);
    CHECK(wrapper_3d_trace.x == 500.0f - (float)text_width);
    CHECK(wrapper_3d_trace.y == 30.0f);
    CHECK(wrapper_3d_trace.z == 40.0f);
    CHECK(wrapper_3d_trace.pointSize == 48);
    CHECK(wrapper_3d_trace.color == UINT32_C(0xff123456));
    jpb_WHookSetDrawUITextUTF16Hook(NULL, NULL);
    jpb_WHookSetDrawUITextUTF163DHook(NULL, NULL);
    framebuffer.pixels = pixels;
    framebuffer.width = WIDTH;
    framebuffer.height = HEIGHT;
    framebuffer.stridePixels = WIDTH;
    CHECK(jpb_PortableTextDraw(
              L"0000000",
              11,
              255,
              0,
              24,
              12,
              3.24f,
              2,
              0,
              1.125f,
              0,
              0,
              0,
              0,
              0,
              &framebuffer,
              &metrics) == 1);
    CHECK(metrics.usedTrueType == 1);
    CHECK(metrics.pointSize == 87);
    CHECK(metrics.width > 200);
    CHECK(metrics.height == 68);
    CHECK(metrics.blendedPixels > 1000);
    for (index = 0;
         index < (size_t)WIDTH * HEIGHT;
         ++index) {
        if (pixels[index] != 0) {
            ++changed;
        }
    }
    CHECK(changed == metrics.blendedPixels);

    memset(pixels, 0, (size_t)WIDTH * HEIGHT * sizeof(*pixels));
    changed = 0;
    CHECK(jpb_PortableTextDraw(
              L"0000000",
              11,
              255,
              0,
              24,
              12,
              3.24f,
              2,
              0,
              1.125f,
              1,
              64,
              20,
              128,
              100,
              &framebuffer,
              &metrics) == 1);
    for (index = 0;
         index < (size_t)WIDTH * HEIGHT;
         ++index) {
        if (pixels[index] != 0) {
            int pixel_x = (int)(index % WIDTH);
            int pixel_y = (int)(index / WIDTH);

            CHECK(pixel_x >= 64 && pixel_x < 128);
            CHECK(pixel_y >= 20 && pixel_y < 100);
            ++changed;
        }
    }
    CHECK(changed > 0);
    CHECK(changed == metrics.blendedPixels);
    CHECK(jpb_PortableTextPrepareControlGlyphs(
              exit_prompt,
              sizeof(exit_prompt) / sizeof(exit_prompt[0]),
              0,
              100,
              100,
              2.25f,
              0,
              0,
              1.0f,
              &origin_x,
              &text_width,
              control_glyphs,
              2) == 1);
    CHECK(origin_x == 100);
    CHECK(control_glyphs[0].iconIndex == 9);
    CHECK(control_glyphs[0].alpha == 255);
    CHECK(control_glyphs[0].x > 100);
    CHECK(control_glyphs[0].y > 100);
    CHECK(jpb_utf16_compare(exit_prompt, L"       Exit") == 0);
    CHECK(jpb_PortableTextPrepareControlGlyphs(
              select_prompt,
              sizeof(select_prompt) / sizeof(select_prompt[0]),
              1,
              1820,
              100,
              2.25f,
              0,
              0,
              1.0f,
              &origin_x,
              &text_width,
              control_glyphs,
              2) == 1);
    CHECK(origin_x < 1820);
    CHECK(control_glyphs[0].iconIndex == 2);
    CHECK(jpb_utf16_compare(select_prompt, L"       Select") == 0);
    CHECK(jpb_PortableTextPrepareControlGlyphs(
              multiline_prompt,
              sizeof(multiline_prompt) / sizeof(multiline_prompt[0]),
              0,
              100,
              100,
              2.25f,
              0,
              0,
              1.0f,
              &origin_x,
              &text_width,
              control_glyphs,
              2) == 1);
    CHECK(control_glyphs[0].iconIndex == 3);
    CHECK(control_glyphs[0].alpha == 255);
    CHECK(control_glyphs[0].x == 100);
    CHECK(control_glyphs[0].y > 150);
    CHECK(jpb_utf16_compare(multiline_prompt, L"Line\n   ") == 0);
    CHECK(jpb_PortableTextEmit3D(
              L"A\nB",
              UINT32_C(0xff123456),
              0,
              10.0f,
              20.0f,
              30.0f,
              2.0f,
              0,
              0,
              capture_text_3d_glyph,
              &text_3d_trace) == 1);
    CHECK(text_3d_trace.calls == 2);
    CHECK(text_3d_trace.materials[0] != NULL);
    CHECK(text_3d_trace.materials[0]->texture != NULL);
    CHECK(text_3d_trace.vertices[0][0].x == 10.0f);
    CHECK(text_3d_trace.vertices[0][0].z == 30.0f);
    CHECK(text_3d_trace.vertices[0][0].argb ==
          UINT32_C(0xff123456));
    CHECK(text_3d_trace.vertices[1][0].x == 10.0f);
    CHECK(text_3d_trace.vertices[1][0].y >
          text_3d_trace.vertices[0][0].y);
    memset(pixels, 0, (size_t)WIDTH * HEIGHT * sizeof(*pixels));
    CHECK(jpb_PortableTextDraw(
              exit_prompt,
              15,
              255,
              0,
              100,
              100,
              2.25f,
              0,
              0,
              1.0f,
              0,
              0,
              0,
              0,
              0,
              &framebuffer,
              &metrics) == 1);
    {
        int first_visible_x = WIDTH;

        for (index = 0;
             index < (size_t)WIDTH * HEIGHT;
             ++index) {
            if (pixels[index] != 0 &&
                first_visible_x > (int)(index % WIDTH)) {
                first_visible_x = (int)(index % WIDTH);
            }
        }
        CHECK(first_visible_x >= 150);
    }
    free(pixels);
    jpb_PortableTextShutdown();
    return 0;
}

int main(int argc, char **argv)
{
    if (test_exact_ui_text_wrappers() != 0) {
        return 1;
    }
    if (test_exact_font_contract() != 0) {
        return 1;
    }
    if (test_recovered_localization() != 0) {
        return 1;
    }
    if (argc == 2 && test_real_font(argv[1]) != 0) {
        return 1;
    }
    puts("portable text tests passed");
    return 0;
}
