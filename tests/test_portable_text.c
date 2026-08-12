#include "jpb/portable_text.h"
#include "jpb/alltext.h"
#include "jpb/menu.h"
#include "jpb/resources.h"
#include "jpb/text.h"
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
    CHECK(jpb_PortableTextTint(-1) == UINT32_C(0xfff0f0f0));
    CHECK(jpb_PortableTextTint(17) == UINT32_C(0xfff0f0f0));
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
    CHECK(jpb_AllTextUtf8(0, 0) == NULL);
    CHECK(jpb_AllTextUtf8(0, 1) == NULL);
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
    CHECK(allText[0] == NULL);
    CHECK(allText[1] == NULL);
    CHECK(wcscmp(allText[435], L"Objective Complete!") == 0);

    generateAllText(6);
    CHECK(currentLanguage == 6);
    CHECK(allText[435] != NULL);
    CHECK((uint32_t)allText[435][0] == UINT32_C(0x4EFB));
    CHECK((uint32_t)allText[435][1] == UINT32_C(0x52A1));
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

    CHECK(pixels != NULL);
    CHECK(jpb_ResourceSetBasePath(game_root) == 1);
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
    CHECK(metrics.height >= 80);
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
    free(pixels);
    jpb_PortableTextShutdown();
    return 0;
}

int main(int argc, char **argv)
{
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
