/*
 * Portable realization of the matched PC SDL_ttf/FontAtlas text boundary.
 *
 * The game-owned call contract, point-size calculation, font selection,
 * tint table, alignment, and alpha are recovered from game.exe/game.pdb.
 * stb_truetype is used only as a platform-neutral TrueType rasterizer so the
 * PC runtime does not acquire SDL or SDL_ttf as a dependency.
 */

#include "jpb/portable_text.h"

#include "jpb/alltext.h"
#include "jpb/resources.h"
#include "jpb/text.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#ifdef _WIN32
typedef void *HMODULE;
__declspec(dllimport) HMODULE __stdcall LoadLibraryA(const char *);
__declspec(dllimport) void *__stdcall GetProcAddress(
    HMODULE, const char *);
#endif

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../../third_party/stb/stb_truetype.h"

enum {
    JPB_PORTABLE_TEXT_FONT_CACHE_CAPACITY = 4,
    JPB_PORTABLE_TEXT_TINT_COUNT = 17
};

typedef struct JPBPortableTextFont {
    char fileName[32];
    char sourcePath[512];
    uint8_t *fileData;
    size_t fileSize;
    stbtt_fontinfo info;
#ifdef _WIN32
    void *sdlFont;
    int sdlPointSize;
#endif
    int loaded;
} JPBPortableTextFont;

#ifdef _WIN32
typedef struct JPBPortableSDLColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} JPBPortableSDLColor;

typedef struct JPBPortableSDLRect {
    int x;
    int y;
    int w;
    int h;
} JPBPortableSDLRect;

typedef struct JPBPortableSDLPixelFormat {
    uint32_t format;
    void *palette;
    uint8_t bitsPerPixel;
    uint8_t bytesPerPixel;
    uint8_t padding[2];
    uint32_t rmask;
    uint32_t gmask;
    uint32_t bmask;
    uint32_t amask;
    uint8_t rloss;
    uint8_t gloss;
    uint8_t bloss;
    uint8_t aloss;
    uint8_t rshift;
    uint8_t gshift;
    uint8_t bshift;
    uint8_t ashift;
    int refcount;
    struct JPBPortableSDLPixelFormat *next;
} JPBPortableSDLPixelFormat;

typedef struct JPBPortableSDLSurface {
    uint32_t flags;
    JPBPortableSDLPixelFormat *format;
    int w;
    int h;
    int pitch;
    void *pixels;
    void *userdata;
    int locked;
    void *listBlitmap;
    JPBPortableSDLRect clipRect;
    void *map;
    int refcount;
} JPBPortableSDLSurface;

typedef int (*JPB_TTF_Init)(void);
typedef void *(*JPB_TTF_OpenFont)(const char *, int);
typedef int (*JPB_TTF_SetFontSize)(void *, int);
typedef int (*JPB_TTF_GlyphMetrics)(
    void *, uint16_t, int *, int *, int *, int *, int *);
typedef JPBPortableSDLSurface *(*JPB_TTF_RenderUNICODE_Blended)(
    void *, const uint16_t *, JPBPortableSDLColor);
typedef void (*JPB_TTF_CloseFont)(void *);
typedef int (*JPB_SDL_LockSurface)(JPBPortableSDLSurface *);
typedef void (*JPB_SDL_UnlockSurface)(JPBPortableSDLSurface *);
typedef void (*JPB_SDL_FreeSurface)(JPBPortableSDLSurface *);

static HMODULE portableTextSDLModule;
static HMODULE portableTextTTFModule;
static int portableTextSDLLoadAttempted;
static JPB_TTF_Init portable_TTF_Init;
static JPB_TTF_OpenFont portable_TTF_OpenFont;
static JPB_TTF_SetFontSize portable_TTF_SetFontSize;
static JPB_TTF_GlyphMetrics portable_TTF_GlyphMetrics;
static JPB_TTF_RenderUNICODE_Blended portable_TTF_RenderUNICODE_Blended;
static JPB_TTF_CloseFont portable_TTF_CloseFont;
static JPB_SDL_LockSurface portable_SDL_LockSurface;
static JPB_SDL_UnlockSurface portable_SDL_UnlockSurface;
static JPB_SDL_FreeSurface portable_SDL_FreeSurface;
#endif

static JPBPortableTextFont portableTextFonts[
    JPB_PORTABLE_TEXT_FONT_CACHE_CAPACITY];

#ifdef _WIN32
static int portable_text_copy_game_root(
    const char *font_path,
    char *root,
    size_t root_size)
{
    const char *marker;
    size_t length;

    if (font_path == NULL || root == NULL || root_size == 0) {
        return 0;
    }
    marker = strstr(font_path, "\\res\\font\\");
    if (marker == NULL) {
        marker = strstr(font_path, "/res/font/");
    }
    if (marker == NULL) {
        return 0;
    }
    length = (size_t)(marker - font_path);
    if (length == 0 || length >= root_size) {
        return 0;
    }
    memcpy(root, font_path, length);
    root[length] = '\0';
    return 1;
}

static int portable_text_load_sdl_ttf(const char *font_path)
{
    char root[512];
    char dll_path[640];

    if (portableTextSDLLoadAttempted) {
        return portableTextTTFModule != NULL;
    }
    portableTextSDLLoadAttempted = 1;
    if (portable_text_copy_game_root(font_path, root, sizeof(root))) {
        snprintf(dll_path, sizeof(dll_path), "%s\\SDL2.dll", root);
        portableTextSDLModule = LoadLibraryA(dll_path);
        snprintf(dll_path, sizeof(dll_path), "%s\\SDL2_ttf.dll", root);
        portableTextTTFModule = LoadLibraryA(dll_path);
    }
    if (portableTextSDLModule == NULL) {
        portableTextSDLModule = LoadLibraryA("SDL2.dll");
    }
    if (portableTextTTFModule == NULL) {
        portableTextTTFModule = LoadLibraryA("SDL2_ttf.dll");
    }
    if (portableTextTTFModule == NULL) {
        return 0;
    }
    portable_TTF_Init = (JPB_TTF_Init)GetProcAddress(
        portableTextTTFModule, "TTF_Init");
    portable_TTF_OpenFont = (JPB_TTF_OpenFont)GetProcAddress(
        portableTextTTFModule, "TTF_OpenFont");
    portable_TTF_SetFontSize = (JPB_TTF_SetFontSize)GetProcAddress(
        portableTextTTFModule, "TTF_SetFontSize");
    portable_TTF_GlyphMetrics = (JPB_TTF_GlyphMetrics)GetProcAddress(
        portableTextTTFModule, "TTF_GlyphMetrics");
    portable_TTF_RenderUNICODE_Blended =
        (JPB_TTF_RenderUNICODE_Blended)GetProcAddress(
            portableTextTTFModule, "TTF_RenderUNICODE_Blended");
    portable_TTF_CloseFont = (JPB_TTF_CloseFont)GetProcAddress(
        portableTextTTFModule, "TTF_CloseFont");
    if (portableTextSDLModule != NULL) {
        portable_SDL_LockSurface = (JPB_SDL_LockSurface)GetProcAddress(
            portableTextSDLModule, "SDL_LockSurface");
        portable_SDL_UnlockSurface = (JPB_SDL_UnlockSurface)GetProcAddress(
            portableTextSDLModule, "SDL_UnlockSurface");
        portable_SDL_FreeSurface = (JPB_SDL_FreeSurface)GetProcAddress(
            portableTextSDLModule, "SDL_FreeSurface");
    }
    if (portable_TTF_Init == NULL ||
        portable_TTF_OpenFont == NULL ||
        portable_TTF_SetFontSize == NULL ||
        portable_TTF_GlyphMetrics == NULL ||
        portable_TTF_RenderUNICODE_Blended == NULL ||
        portable_SDL_FreeSurface == NULL) {
        return 0;
    }
    return portable_TTF_Init() == 0;
}
#endif

/* Exact initialized DWORDs at matched-PC RVA 0x4CD100. */
static const uint32_t portableTextTints[
    JPB_PORTABLE_TEXT_TINT_COUNT] = {
    UINT32_C(0xff4060e0),
    UINT32_C(0xff000080),
    UINT32_C(0xff80a080),
    UINT32_C(0xff008000),
    UINT32_C(0xffe08080),
    UINT32_C(0xff800000),
    UINT32_C(0xff80e0e0),
    UINT32_C(0xffe080d0),
    UINT32_C(0xffe0e000),
    UINT32_C(0xff303030),
    UINT32_C(0xff606060),
    UINT32_C(0xfff0f0f0),
    UINT32_C(0xff3f61bf),
    UINT32_C(0xfff0f030),
    UINT32_C(0xffa5f5a5),
    UINT32_C(0xffc0c0c0),
    UINT32_C(0xffe09093)
};

const char *jpb_PortableTextFontFileName(
    int font_style, int language)
{
    if (language == 6) {
        if (font_style == 1) {
            return "NotoSansSC-Light.ttf";
        }
        if (font_style == 2) {
            return "NotoSansSC-Bold.ttf";
        }
        return "NotoSansSC-Regular.ttf";
    }
    if (font_style == 1) {
        return "NotoSans-Italic.ttf";
    }
    if (font_style == 2) {
        return "NotoSansSC-Bold.ttf";
    }
    return "NotoSansSC-Regular.ttf";
}

int jpb_PortableTextPointSize(
    float scale, float scale_adjustment)
{
    float point_size;

    if (!(scale > 0.0f)) {
        scale = 1.0f;
    }
    point_size = scale * scale_adjustment * 24.0f;
    if (!(point_size > 0.0f)) {
        return 0;
    }
    if (point_size >= (float)INT_MAX) {
        return INT_MAX;
    }
    return (int)point_size;
}

uint32_t jpb_PortableTextTint(int tint)
{
    if ((unsigned)tint >= JPB_PORTABLE_TEXT_TINT_COUNT) {
        return portableTextTints[11];
    }
    return portableTextTints[tint];
}

static int portable_text_read_file(
    const char *path, uint8_t **data_out, size_t *size_out)
{
    FILE *file;
    long length;
    uint8_t *data;

    if (path == NULL || data_out == NULL || size_out == NULL) {
        return 0;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    length = ftell(file);
    if (length <= 0 || (unsigned long)length > SIZE_MAX) {
        fclose(file);
        return 0;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    data = (uint8_t *)malloc((size_t)length);
    if (data == NULL) {
        fclose(file);
        return 0;
    }
    if (fread(data, 1, (size_t)length, file) != (size_t)length) {
        free(data);
        fclose(file);
        return 0;
    }
    fclose(file);
    *data_out = data;
    *size_out = (size_t)length;
    return 1;
}

static _TTF_Font *portable_text_load_font(
    void *user_data, const char *path, int point_size)
{
    const char *slash;
    const char *backslash;
    const char *file_name;
    size_t index;

    (void)user_data;
    (void)point_size;
    if (path == NULL) {
        return NULL;
    }
    slash = strrchr(path, '/');
    backslash = strrchr(path, '\\');
    if (slash == NULL ||
        (backslash != NULL && backslash > slash)) {
        slash = backslash;
    }
    file_name = slash != NULL ? slash + 1 : path;

    for (index = 0;
         index < JPB_PORTABLE_TEXT_FONT_CACHE_CAPACITY;
         ++index) {
        if (portableTextFonts[index].loaded &&
            strcmp(portableTextFonts[index].fileName, file_name) == 0) {
            return (_TTF_Font *)(void *)&portableTextFonts[index];
        }
    }
    for (index = 0;
         index < JPB_PORTABLE_TEXT_FONT_CACHE_CAPACITY;
         ++index) {
        JPBPortableTextFont *font = &portableTextFonts[index];
        int offset;

        if (font->loaded) {
            continue;
        }
        if (strlen(file_name) >= sizeof(font->fileName)) {
            return NULL;
        }
        if (strlen(path) >= sizeof(font->sourcePath)) {
            return NULL;
        }
        if (!portable_text_read_file(
                path, &font->fileData, &font->fileSize)) {
            return NULL;
        }
        offset = stbtt_GetFontOffsetForIndex(font->fileData, 0);
        if (offset < 0 || !stbtt_InitFont(
                &font->info, font->fileData, offset)) {
            free(font->fileData);
            memset(font, 0, sizeof(*font));
            return NULL;
        }
        memcpy(font->fileName, file_name, strlen(file_name) + 1);
        memcpy(font->sourcePath, path, strlen(path) + 1);
#ifdef _WIN32
        if (portable_text_load_sdl_ttf(path)) {
            font->sdlFont = portable_TTF_OpenFont(path, point_size);
            font->sdlPointSize = font->sdlFont != NULL ? point_size : 0;
        }
#endif
        font->loaded = 1;
        return (_TTF_Font *)(void *)font;
    }
    return NULL;
}

static JPBPortableTextFont *portable_text_get_font(
    int font_style, int language, int point_size)
{
    currentLanguage = language;
    jpb_TextSetFontLoadHook(portable_text_load_font, NULL);
    return (JPBPortableTextFont *)(void *)LoadFont(
        font_style, point_size);
}

static uint32_t portable_text_next_codepoint(
    const wchar_t **cursor)
{
    uint32_t codepoint;

    if (cursor == NULL || *cursor == NULL || **cursor == L'\0') {
        return 0;
    }
    codepoint = (uint32_t)(**cursor);
    ++*cursor;
#if WCHAR_MAX <= UINT16_MAX
    if (codepoint >= UINT32_C(0xd800) &&
        codepoint <= UINT32_C(0xdbff)) {
        uint32_t low = (uint32_t)(**cursor);

        if (low >= UINT32_C(0xdc00) &&
            low <= UINT32_C(0xdfff)) {
            ++*cursor;
            codepoint = UINT32_C(0x10000) +
                ((codepoint - UINT32_C(0xd800)) << 10) +
                (low - UINT32_C(0xdc00));
        }
    }
#endif
    return codepoint;
}

static int portable_text_rounded_metric(float value)
{
    return value >= 0.0f
        ? (int)(value + 0.5f)
        : (int)(value - 0.5f);
}

#ifdef _WIN32
static int portable_text_prepare_sdl_font(
    JPBPortableTextFont *font,
    int point_size)
{
    if (font == NULL || font->sdlFont == NULL || point_size <= 0 ||
        portable_TTF_SetFontSize == NULL) {
        return 0;
    }
    if (font->sdlPointSize != point_size) {
        if (portable_TTF_SetFontSize(font->sdlFont, point_size) != 0) {
            return 0;
        }
        font->sdlPointSize = point_size;
    }
    return 1;
}

static int portable_text_measure_sdl(
    JPBPortableTextFont *font,
    const wchar_t *text,
    int point_size,
    int *width_out,
    int *height_out,
    int *baseline_out)
{
    const wchar_t *cursor = text;
    int line_width = 0;
    int width = 0;
    int line_min_y = 0;
    int line_max_y = 0;
    int height = 0;

    if (!portable_text_prepare_sdl_font(font, point_size) ||
        text == NULL || width_out == NULL ||
        height_out == NULL || baseline_out == NULL ||
        portable_TTF_GlyphMetrics == NULL) {
        return 0;
    }
    while (*cursor != L'\0') {
        uint32_t codepoint = portable_text_next_codepoint(&cursor);
        int min_x = 0;
        int max_x = 0;
        int min_y = 0;
        int max_y = 0;
        int advance = 0;
        int glyph_width;
        int width_with_last_glyph;

        if (codepoint == 0) {
            break;
        }
        if (codepoint == L'\n') {
            height += line_max_y - line_min_y;
            if (width < line_width) {
                width = line_width;
            }
            line_width = 0;
            line_min_y = 0;
            line_max_y = 0;
            continue;
        }
        if (portable_TTF_GlyphMetrics(
                font->sdlFont,
                (uint16_t)codepoint,
                &min_x,
                &max_x,
                &min_y,
                &max_y,
                &advance) != 0) {
            return 0;
        }
        glyph_width = max_x - min_x;
        width_with_last_glyph = line_width + glyph_width;
        line_width += advance - min_x;
        if (*cursor == L'\0') {
            line_width = width_with_last_glyph;
        }
        if (line_max_y < max_y) {
            line_max_y = max_y;
        }
        if (line_min_y > min_y) {
            line_min_y = min_y;
        }
    }
    height += line_max_y - line_min_y;
    if (width < line_width) {
        width = line_width;
    }
    *width_out = width;
    *height_out = height;
    *baseline_out = line_max_y;
    return 1;
}
#endif

static int portable_text_measure(
    const JPBPortableTextFont *font,
    const wchar_t *text,
    int point_size,
    float *font_scale_out,
    int *width_out,
    int *height_out,
    int *baseline_out)
{
    const wchar_t *cursor = text;
    float font_scale;
    int line_width = 0;
    int width = 0;
    int line_min_y = 0;
    int line_max_y = 0;
    int height = 0;

    if (font == NULL || text == NULL || point_size <= 0 ||
        font_scale_out == NULL || width_out == NULL ||
        height_out == NULL || baseline_out == NULL) {
        return 0;
    }
    font_scale = stbtt_ScaleForPixelHeight(
        &font->info, (float)point_size);
    if (!(font_scale > 0.0f)) {
        return 0;
    }
    while (*cursor != L'\0') {
        uint32_t codepoint = portable_text_next_codepoint(&cursor);
        int advance;
        int bearing;
        int box_x0;
        int box_y0;
        int box_x1;
        int box_y1;
        int min_x;
        int max_x;
        int min_y;
        int max_y;
        int glyph_width;
        int width_with_last_glyph;

        if (codepoint == 0) {
            break;
        }
        if (codepoint == L'\n') {
            height += line_max_y - line_min_y;
            if (width < line_width) {
                width = line_width;
            }
            line_width = 0;
            line_min_y = 0;
            line_max_y = 0;
            continue;
        }
        stbtt_GetCodepointHMetrics(
            &font->info, (int)codepoint, &advance, &bearing);
        (void)bearing;
        stbtt_GetCodepointBox(
            &font->info, (int)codepoint,
            &box_x0, &box_y0, &box_x1, &box_y1);
        min_x = portable_text_rounded_metric(
            (float)box_x0 * font_scale);
        max_x = portable_text_rounded_metric(
            (float)box_x1 * font_scale);
        min_y = portable_text_rounded_metric(
            (float)box_y0 * font_scale);
        max_y = portable_text_rounded_metric(
            (float)box_y1 * font_scale);
        glyph_width = max_x - min_x;
        width_with_last_glyph = line_width + glyph_width;
        line_width += portable_text_rounded_metric(
            (float)advance * font_scale) - min_x;
        if (*cursor == L'\0') {
            line_width = width_with_last_glyph;
        }
        if (line_max_y < max_y) {
            line_max_y = max_y;
        }
        if (line_min_y > min_y) {
            line_min_y = min_y;
        }
    }
    height += line_max_y - line_min_y;
    if (width < line_width) {
        width = line_width;
    }
    *font_scale_out = font_scale;
    *width_out = width;
    *height_out = height;
    *baseline_out = line_max_y;
    return 1;
}

static int portable_text_control_icon(uint32_t codepoint)
{
    switch (codepoint) {
    case UINT32_C(0x2021): return 3;
    case UINT32_C(0x20ac): return 2;
    case UINT32_C(0x0192): return 4;
    case UINT32_C(0x2020): return 5;
    case UINT32_C(0x0160): return 7;
    case UINT32_C(0x017d): return 9;
    default: return -1;
    }
}

static int portable_text_tag_icon(wchar_t codepoint, int *alpha)
{
    int icon;

    if (alpha == NULL) {
        return -1;
    }
    *alpha = iswlower(codepoint) ? 128 : 255;
    switch (towupper(codepoint)) {
    case L'A': icon = 3; break;
    case L'B': icon = 2; break;
    case L'X': icon = 4; break;
    case L'Y': icon = 5; break;
    case L'F': icon = 7; break;
    default: return -1;
    }
    return icon;
}

static int portable_text_control_advance(
    JPBPortableTextFont *font,
    int point_size,
    float font_scale,
    uint32_t codepoint,
    int *advance_out)
{
    int min_x = 0;
    int max_x = 0;
    int min_y = 0;
    int max_y = 0;
    int advance = 0;

    if (font == NULL || advance_out == NULL) {
        return 0;
    }
#ifdef _WIN32
    if (font->sdlFont != NULL &&
        portable_text_prepare_sdl_font(font, point_size) &&
        portable_TTF_GlyphMetrics != NULL) {
        if (portable_TTF_GlyphMetrics(
                font->sdlFont,
                (uint16_t)codepoint,
                &min_x,
                &max_x,
                &min_y,
                &max_y,
                &advance) != 0) {
            return 0;
        }
        *advance_out = advance - min_x;
        return 1;
    }
#endif
    if (!(font_scale > 0.0f)) {
        return 0;
    }
    stbtt_GetCodepointHMetrics(
        &font->info, (int)codepoint, &advance, &max_x);
    stbtt_GetCodepointBox(
        &font->info, (int)codepoint,
        &min_x, &min_y, &max_x, &max_y);
    *advance_out = portable_text_rounded_metric(
        (float)advance * font_scale) -
        portable_text_rounded_metric((float)min_x * font_scale);
    return 1;
}

int jpb_PortableTextPrepareControlGlyphs(
    wchar_t *text,
    size_t text_capacity,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    int language,
    float scale_adjustment,
    int *origin_x,
    JPBPortableTextControlGlyph *glyphs,
    size_t glyph_capacity)
{
    JPBPortableTextFont *font;
    size_t length;
    size_t index = 0;
    size_t glyph_count = 0;
    float font_scale = 0.0f;
    int point_size;
    int text_width;
    int text_height;
    int baseline_offset;
    int pen_x;

    if (text == NULL || text_capacity == 0 || origin_x == NULL ||
        (glyph_capacity != 0 && glyphs == NULL)) {
        return -1;
    }
    length = wcslen(text);
    if (length >= text_capacity) {
        return -1;
    }
    point_size = jpb_PortableTextPointSize(scale, scale_adjustment);
    font = portable_text_get_font(font_style, language, point_size);
    if (font == NULL) {
        return -1;
    }
#ifdef _WIN32
    if (!portable_text_measure_sdl(
            font, text, point_size,
            &text_width, &text_height, &baseline_offset))
#endif
    {
        if (!portable_text_measure(
                font, text, point_size, &font_scale,
                &text_width, &text_height, &baseline_offset)) {
            return -1;
        }
    }
    (void)text_height;
    pen_x = x;
    if ((mode & 0x7f) == 1) {
        pen_x -= text_width;
    } else if ((mode & 0x7f) == 2) {
        pen_x -= text_width / 2;
    }
    *origin_x = pen_x;

    while (index < length) {
        int alpha = 255;
        int icon = portable_text_control_icon((uint32_t)text[index]);
        int tagged = 0;
        int advance;

        if (icon < 0 && index + 2 < length &&
            text[index] == L'<' && text[index + 2] == L'>') {
            icon = portable_text_tag_icon(text[index + 1], &alpha);
            tagged = icon >= 0;
        }
        if (icon >= 0 && !tagged && index < 3) {
            size_t leading_spaces = 3 - index;

            if (length + leading_spaces >= text_capacity) {
                return -1;
            }
            memmove(
                text + leading_spaces,
                text,
                (length + 1) * sizeof(text[0]));
            wmemset(text, L' ', leading_spaces);
            length += leading_spaces;
            index = 0;
            pen_x = *origin_x;
            continue;
        }
        if (icon >= 0) {
            if (glyph_count < glyph_capacity) {
                glyphs[glyph_count].iconIndex = icon;
                glyphs[glyph_count].alpha = alpha;
                glyphs[glyph_count].x = pen_x;
                glyphs[glyph_count].y =
                    (int)((float)y + (float)baseline_offset * 0.5f);
                ++glyph_count;
            }
            if (tagged) {
                text[index + 1] = L' ';
                text[index + 2] = L' ';
            }
            memmove(
                text + index,
                text + index + 1,
                (length - index) * sizeof(text[0]));
            --length;
            continue;
        }
        if (text[index] == L'\n') {
            pen_x = *origin_x;
            ++index;
            continue;
        }
        if (!portable_text_control_advance(
                font, point_size, font_scale,
                (uint32_t)text[index], &advance)) {
            return -1;
        }
        pen_x += advance;
        ++index;
    }
    return (int)glyph_count;
}

static uint32_t portable_text_blend(
    uint32_t destination,
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint8_t alpha)
{
    uint32_t inverse = UINT32_C(255) - alpha;
    uint32_t destination_red =
        (destination >> 16) & UINT32_C(0xff);
    uint32_t destination_green =
        (destination >> 8) & UINT32_C(0xff);
    uint32_t destination_blue =
        destination & UINT32_C(0xff);
    uint32_t output_red =
        ((uint32_t)red * alpha + destination_red * inverse + 127) /
        UINT32_C(255);
    uint32_t output_green =
        ((uint32_t)green * alpha + destination_green * inverse + 127) /
        UINT32_C(255);
    uint32_t output_blue =
        ((uint32_t)blue * alpha + destination_blue * inverse + 127) /
        UINT32_C(255);

    return UINT32_C(0xff000000) |
        (output_red << 16) |
        (output_green << 8) |
        output_blue;
}

#ifdef _WIN32
static uint8_t portable_text_surface_component(
    uint32_t pixel,
    uint32_t mask,
    uint8_t shift,
    uint8_t loss)
{
    uint32_t value;

    if (mask == 0) {
        return 0xff;
    }
    value = (pixel & mask) >> shift;
    if (loss < 8) {
        value <<= loss;
    }
    if (value > UINT32_C(0xff)) {
        value = UINT32_C(0xff);
    }
    return (uint8_t)value;
}

static int portable_text_draw_sdl(
    JPBPortableTextFont *font,
    const wchar_t *text,
    int point_size,
    int x,
    int y,
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint8_t alpha,
    int clip_enabled,
    int clip_left,
    int clip_top,
    int clip_right,
    int clip_bottom,
    JPBSoftwareFramebuffer *framebuffer,
    size_t *blended_pixels)
{
    JPBPortableSDLColor color;
    JPBPortableSDLSurface *surface;
    int surface_w;
    int surface_h;
    int min_blit_x;
    int min_blit_y;
    int max_blit_x = -1;
    int max_blit_y = -1;
    int leading_blank_width = 0;
    int row_index;

    if (!portable_text_prepare_sdl_font(font, point_size) ||
        portable_TTF_RenderUNICODE_Blended == NULL ||
        portable_SDL_FreeSurface == NULL ||
        text == NULL || framebuffer == NULL ||
        framebuffer->pixels == NULL) {
        return 0;
    }
    color.r = red;
    color.g = green;
    color.b = blue;
    color.a = alpha;
    surface = portable_TTF_RenderUNICODE_Blended(
        font->sdlFont,
        (const uint16_t *)text,
        color);
    if (surface == NULL || surface->pixels == NULL ||
        surface->format == NULL || surface->w <= 0 ||
        surface->h <= 0 || surface->pitch <= 0) {
        if (surface != NULL) {
            portable_SDL_FreeSurface(surface);
        }
        return 0;
    }
    {
        const wchar_t *cursor = text;

        while (*cursor == L' ') {
            int min_x = 0;
            int max_x = 0;
            int min_y = 0;
            int max_y = 0;
            int advance = 0;

            if (portable_TTF_GlyphMetrics(
                    font->sdlFont,
                    (uint16_t)*cursor,
                    &min_x,
                    &max_x,
                    &min_y,
                    &max_y,
                    &advance) != 0) {
                break;
            }
            leading_blank_width += advance - min_x;
            ++cursor;
        }
    }
    if (portable_SDL_LockSurface != NULL) {
        (void)portable_SDL_LockSurface(surface);
    }
    surface_w = surface->w;
    surface_h = surface->h;
    min_blit_x = surface_w;
    min_blit_y = surface_h;
    for (row_index = 0; row_index < surface_h; ++row_index) {
        const uint8_t *source_row =
            (const uint8_t *)surface->pixels +
            (size_t)row_index * (size_t)surface->pitch;
        int column_index;

        for (column_index = 0;
             column_index < surface_w;
             ++column_index) {
            const uint8_t *source_pixel =
                source_row +
                (size_t)column_index *
                    (size_t)surface->format->bytesPerPixel;
            uint32_t pixel = 0;
            uint8_t source_alpha;

            memcpy(
                &pixel,
                source_pixel,
                surface->format->bytesPerPixel <= 4
                    ? surface->format->bytesPerPixel
                    : 4);
            source_alpha = portable_text_surface_component(
                pixel,
                surface->format->amask,
                surface->format->ashift,
                surface->format->aloss);
            if (source_alpha == 0) {
                continue;
            }
            if (min_blit_x > column_index) {
                min_blit_x = column_index;
            }
            if (min_blit_y > row_index) {
                min_blit_y = row_index;
            }
            if (max_blit_x < column_index) {
                max_blit_x = column_index;
            }
            if (max_blit_y < row_index) {
                max_blit_y = row_index;
            }
        }
    }
    if (max_blit_x < min_blit_x || max_blit_y < min_blit_y) {
        if (portable_SDL_UnlockSurface != NULL) {
            portable_SDL_UnlockSurface(surface);
        }
        portable_SDL_FreeSurface(surface);
        return 1;
    }
    for (row_index = min_blit_y; row_index <= max_blit_y; ++row_index) {
        int output_y = y + row_index;
        const uint8_t *source_row =
            (const uint8_t *)surface->pixels +
            (size_t)row_index * (size_t)surface->pitch;
        int column_index;

        output_y -= min_blit_y;
        if (output_y < 0 ||
            output_y >= framebuffer->height ||
            (clip_enabled &&
             (output_y < clip_top ||
              output_y >= clip_bottom))) {
            continue;
        }
        for (column_index = min_blit_x;
             column_index <= max_blit_x;
             ++column_index) {
            int output_x =
                x + leading_blank_width + column_index;
            const uint8_t *source_pixel =
                source_row +
                (size_t)column_index *
                    (size_t)surface->format->bytesPerPixel;
            uint32_t pixel = 0;
            uint8_t source_red;
            uint8_t source_green;
            uint8_t source_blue;
            uint8_t source_alpha;

            output_x -= min_blit_x;
            if (output_x < 0 ||
                output_x >= framebuffer->width ||
                (clip_enabled &&
                 (output_x < clip_left ||
                  output_x >= clip_right))) {
                continue;
            }
            memcpy(
                &pixel,
                source_pixel,
                surface->format->bytesPerPixel <= 4
                    ? surface->format->bytesPerPixel
                    : 4);
            source_alpha = portable_text_surface_component(
                pixel,
                surface->format->amask,
                surface->format->ashift,
                surface->format->aloss);
            if (source_alpha == 0) {
                continue;
            }
            source_red = portable_text_surface_component(
                pixel,
                surface->format->rmask,
                surface->format->rshift,
                surface->format->rloss);
            source_green = portable_text_surface_component(
                pixel,
                surface->format->gmask,
                surface->format->gshift,
                surface->format->gloss);
            source_blue = portable_text_surface_component(
                pixel,
                surface->format->bmask,
                surface->format->bshift,
                surface->format->bloss);
            framebuffer->pixels[
                (size_t)output_y *
                    (size_t)framebuffer->stridePixels +
                (size_t)output_x] = portable_text_blend(
                    framebuffer->pixels[
                        (size_t)output_y *
                            (size_t)framebuffer->stridePixels +
                        (size_t)output_x],
                    source_red,
                    source_green,
                    source_blue,
                    (uint8_t)(
                        (uint32_t)source_alpha *
                        (uint32_t)alpha /
                        UINT32_C(255)));
            if (blended_pixels != NULL) {
                ++*blended_pixels;
            }
        }
    }
    if (portable_SDL_UnlockSurface != NULL) {
        portable_SDL_UnlockSurface(surface);
    }
    portable_SDL_FreeSurface(surface);
    return 1;
}
#endif

int jpb_PortableTextDraw(
    const wchar_t *text,
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    int language,
    float scale_adjustment,
    int clip_enabled,
    int clip_left,
    int clip_top,
    int clip_right,
    int clip_bottom,
    JPBSoftwareFramebuffer *framebuffer,
    JPBPortableTextMetrics *metrics)
{
    JPBPortableTextFont *font;
    const wchar_t *cursor;
    float font_scale;
    int point_size;
    int text_width;
    int text_height;
    int baseline_offset;
    int pen_x;
    int baseline;
    int clamped_alpha;
    uint32_t color;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    size_t blended_pixels = 0;

    if (metrics != NULL) {
        memset(metrics, 0, sizeof(*metrics));
    }
    if (text == NULL || framebuffer == NULL ||
        framebuffer->pixels == NULL || framebuffer->width <= 0 ||
        framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width) {
        return 0;
    }
    point_size = jpb_PortableTextPointSize(scale, scale_adjustment);
    font = portable_text_get_font(
        font_style, language, point_size);
    if (font == NULL) {
        return 0;
    }
#ifdef _WIN32
    if (portable_text_measure_sdl(
            font,
            text,
            point_size,
            &text_width,
            &text_height,
            &baseline_offset)) {
        font_scale = 0.0f;
    } else
#endif
    if (!portable_text_measure(
            font,
            text,
            point_size,
            &font_scale,
            &text_width,
            &text_height,
            &baseline_offset)) {
        return 0;
    }
    pen_x = x;
    if ((mode & 0x7f) == 1) {
        pen_x -= text_width;
    } else if ((mode & 0x7f) == 2) {
        pen_x -= text_width / 2;
    }
    baseline = y + baseline_offset;
    clamped_alpha = alpha;
    if (clamped_alpha < 0) clamped_alpha = 0;
    if (clamped_alpha > 255) clamped_alpha = 255;
    color = jpb_PortableTextTint(tint);
    red = (uint8_t)(color & UINT32_C(0xff));
    green = (uint8_t)((color >> 8) & UINT32_C(0xff));
    blue = (uint8_t)((color >> 16) & UINT32_C(0xff));
#ifdef _WIN32
    if (font->sdlFont != NULL &&
        portable_text_draw_sdl(
            font,
            text,
            point_size,
            pen_x,
            y,
            red,
            green,
            blue,
            (uint8_t)clamped_alpha,
            clip_enabled,
            clip_left,
            clip_top,
            clip_right,
            clip_bottom,
            framebuffer,
            &blended_pixels)) {
        if (metrics != NULL) {
            metrics->pointSize = point_size;
            metrics->width = text_width;
            metrics->height = text_height;
            metrics->blendedPixels = blended_pixels;
            metrics->usedTrueType = 1;
        }
        return 1;
    }
    if (!(font_scale > 0.0f) &&
        !portable_text_measure(
            font,
            text,
            point_size,
            &font_scale,
            &text_width,
            &text_height,
            &baseline_offset)) {
        return 0;
    }
#endif
    cursor = text;
    while (*cursor != L'\0') {
        uint32_t codepoint = portable_text_next_codepoint(&cursor);
        int bitmap_width;
        int bitmap_height;
        int offset_x;
        int offset_y;
        unsigned char *bitmap;
        int advance;
        int bearing;
        int box_x0;
        int box_y0;
        int box_x1;
        int box_y1;
        int min_x;
        int bitmap_y;

        if (codepoint == 0) {
            break;
        }
        if (codepoint == L'\n') {
            continue;
        }
        stbtt_GetCodepointHMetrics(
            &font->info, (int)codepoint, &advance, &bearing);
        (void)bearing;
        stbtt_GetCodepointBox(
            &font->info, (int)codepoint,
            &box_x0, &box_y0, &box_x1, &box_y1);
        (void)box_y0;
        (void)box_x1;
        (void)box_y1;
        min_x = portable_text_rounded_metric(
            (float)box_x0 * font_scale);
        bitmap = stbtt_GetCodepointBitmap(
            &font->info,
            font_scale,
            font_scale,
            (int)codepoint,
            &bitmap_width,
            &bitmap_height,
            &offset_x,
            &offset_y);
        if (bitmap != NULL) {
            for (bitmap_y = 0;
                 bitmap_y < bitmap_height;
                 ++bitmap_y) {
                int output_y = baseline + offset_y + bitmap_y;
                int bitmap_x;
                uint32_t *row;

                if (output_y < 0 ||
                    output_y >= framebuffer->height ||
                    (clip_enabled &&
                     (output_y < clip_top ||
                      output_y >= clip_bottom))) {
                    continue;
                }
                row = framebuffer->pixels +
                    (size_t)output_y *
                        (size_t)framebuffer->stridePixels;
                for (bitmap_x = 0;
                     bitmap_x < bitmap_width;
                     ++bitmap_x) {
                    int output_x =
                        pen_x - min_x + offset_x + bitmap_x;
                    uint32_t coverage;
                    uint8_t output_alpha;

                    if (output_x < 0 ||
                        output_x >= framebuffer->width ||
                        (clip_enabled &&
                         (output_x < clip_left ||
                          output_x >= clip_right))) {
                        continue;
                    }
                    coverage = bitmap[
                        (size_t)bitmap_y *
                            (size_t)bitmap_width +
                        (size_t)bitmap_x];
                    output_alpha = (uint8_t)(
                        coverage * (uint32_t)clamped_alpha /
                        UINT32_C(255));
                    if (output_alpha == 0) {
                        continue;
                    }
                    row[output_x] = portable_text_blend(
                        row[output_x],
                        red,
                        green,
                        blue,
                        output_alpha);
                    ++blended_pixels;
                }
            }
            stbtt_FreeBitmap(bitmap, font->info.userdata);
        }
        pen_x += portable_text_rounded_metric(
            (float)advance * font_scale) - min_x;
    }
    if (metrics != NULL) {
        metrics->pointSize = point_size;
        metrics->width = text_width;
        metrics->height = text_height;
        metrics->blendedPixels = blended_pixels;
        metrics->usedTrueType = 1;
    }
    return 1;
}

void jpb_PortableTextShutdown(void)
{
    size_t index;

    for (index = 0;
         index < JPB_PORTABLE_TEXT_FONT_CACHE_CAPACITY;
         ++index) {
#ifdef _WIN32
        if (portable_TTF_CloseFont != NULL &&
            portableTextFonts[index].sdlFont != NULL) {
            portable_TTF_CloseFont(portableTextFonts[index].sdlFont);
        }
#endif
        free(portableTextFonts[index].fileData);
    }
    memset(portableTextFonts, 0, sizeof(portableTextFonts));
    jpb_TextSetFontLoadHook(NULL, NULL);
    jpb_TextResetFontCache();
}
