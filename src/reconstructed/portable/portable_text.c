/*
 * Portable realization of the matched PC SDL_ttf/FontAtlas text boundary.
 *
 * The game-owned call contract, point-size calculation, font selection,
 * tint table, alignment, and alpha are recovered from game.exe/game.pdb.
 * SDL_ttf remains the required font owner, matching the shipped executable.
 */

#include "jpb/portable_text.h"

#include "jpb/alltext.h"
#include "jpb/resources.h"
#include "jpb/text.h"
#include "jpb/textutil.h"
#include "jpb/utf16.h"

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

enum {
    JPB_PORTABLE_TEXT_FONT_CACHE_CAPACITY = 6,
    JPB_PORTABLE_TEXT_TINT_COUNT = 17
};

typedef struct JPBPortableTextFont {
    char fileName[32];
    char sourcePath[512];
    void *sdlFont;
    int sdlPointSize;
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
typedef JPBPortableSDLSurface *(*JPB_TTF_RenderGlyph_Blended)(
    void *, uint16_t, JPBPortableSDLColor);
typedef int (*JPB_TTF_FontAscent)(void *);
typedef void (*JPB_TTF_CloseFont)(void *);
typedef int (*JPB_SDL_LockSurface)(JPBPortableSDLSurface *);
typedef void (*JPB_SDL_UnlockSurface)(JPBPortableSDLSurface *);
typedef void (*JPB_SDL_FreeSurface)(JPBPortableSDLSurface *);
typedef const char *(*JPB_SDL_GetError)(void);

static HMODULE portableTextSDLModule;
static HMODULE portableTextTTFModule;
static int portableTextSDLLoadAttempted;
static JPB_TTF_Init portable_TTF_Init;
static JPB_TTF_OpenFont portable_TTF_OpenFont;
static JPB_TTF_SetFontSize portable_TTF_SetFontSize;
static JPB_TTF_GlyphMetrics portable_TTF_GlyphMetrics;
static JPB_TTF_RenderGlyph_Blended portable_TTF_RenderGlyph_Blended;
static JPB_TTF_FontAscent portable_TTF_FontAscent;
static JPB_TTF_CloseFont portable_TTF_CloseFont;
static JPB_SDL_LockSurface portable_SDL_LockSurface;
static JPB_SDL_UnlockSurface portable_SDL_UnlockSurface;
static JPB_SDL_FreeSurface portable_SDL_FreeSurface;
static JPB_SDL_GetError portable_SDL_GetError;
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
    portable_TTF_RenderGlyph_Blended =
        (JPB_TTF_RenderGlyph_Blended)GetProcAddress(
            portableTextTTFModule, "TTF_RenderGlyph_Blended");
    portable_TTF_FontAscent = (JPB_TTF_FontAscent)GetProcAddress(
        portableTextTTFModule, "TTF_FontAscent");
    portable_TTF_CloseFont = (JPB_TTF_CloseFont)GetProcAddress(
        portableTextTTFModule, "TTF_CloseFont");
    if (portableTextSDLModule != NULL) {
        portable_SDL_LockSurface = (JPB_SDL_LockSurface)GetProcAddress(
            portableTextSDLModule, "SDL_LockSurface");
        portable_SDL_UnlockSurface = (JPB_SDL_UnlockSurface)GetProcAddress(
            portableTextSDLModule, "SDL_UnlockSurface");
        portable_SDL_FreeSurface = (JPB_SDL_FreeSurface)GetProcAddress(
            portableTextSDLModule, "SDL_FreeSurface");
        portable_SDL_GetError = (JPB_SDL_GetError)GetProcAddress(
            portableTextSDLModule, "SDL_GetError");
    }
    if (portable_TTF_Init == NULL ||
        portable_TTF_OpenFont == NULL ||
        portable_TTF_SetFontSize == NULL ||
        portable_TTF_GlyphMetrics == NULL ||
        portable_TTF_RenderGlyph_Blended == NULL ||
        portable_TTF_FontAscent == NULL ||
        portable_SDL_FreeSurface == NULL ||
        portable_SDL_GetError == NULL) {
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
    return portableTextTints[tint];
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
        if (font->loaded) {
            continue;
        }
        if (strlen(file_name) >= sizeof(font->fileName)) {
            return NULL;
        }
        if (strlen(path) >= sizeof(font->sourcePath)) {
            return NULL;
        }
#ifndef _WIN32
        return NULL;
#else
        if (!portable_text_load_sdl_ttf(path)) {
            return NULL;
        }
        font->sdlFont = portable_TTF_OpenFont(path, point_size);
        if (font->sdlFont == NULL) {
            return NULL;
        }
        font->sdlPointSize = point_size;
        memcpy(font->fileName, file_name, strlen(file_name) + 1);
        memcpy(font->sourcePath, path, strlen(path) + 1);
        font->loaded = 1;
        return (_TTF_Font *)(void *)font;
#endif
    }
    return NULL;
}

static int portable_text_set_font_size(
    _TTF_Font *font, int point_size)
{
#ifdef _WIN32
    JPBPortableTextFont *portable_font =
        (JPBPortableTextFont *)(void *)font;
    int result;

    if (portable_font == NULL || portable_font->sdlFont == NULL ||
        portable_TTF_SetFontSize == NULL) {
        return -1;
    }
    result = portable_TTF_SetFontSize(
        portable_font->sdlFont, point_size);
    if (result == 0) {
        portable_font->sdlPointSize = point_size;
    }
    return result;
#else
    (void)font;
    (void)point_size;
    return -1;
#endif
}

static int portable_text_glyph_metrics(
    _TTF_Font *font,
    unsigned short glyph,
    int *minimum_x,
    int *maximum_x,
    int *minimum_y,
    int *maximum_y,
    int *advance)
{
#ifdef _WIN32
    JPBPortableTextFont *portable_font =
        (JPBPortableTextFont *)(void *)font;

    if (portable_font == NULL || portable_font->sdlFont == NULL ||
        portable_TTF_GlyphMetrics == NULL) {
        return -1;
    }
    return portable_TTF_GlyphMetrics(
        portable_font->sdlFont,
        glyph,
        minimum_x,
        maximum_x,
        minimum_y,
        maximum_y,
        advance);
#else
    (void)font;
    (void)glyph;
    (void)minimum_x;
    (void)maximum_x;
    (void)minimum_y;
    (void)maximum_y;
    (void)advance;
    return -1;
#endif
}

static const char *portable_text_get_error(void)
{
#ifdef _WIN32
    if (portable_SDL_GetError == NULL) {
        abort();
    }
    return portable_SDL_GetError();
#else
    abort();
#endif
}

void jpb_PortableTextInstallHooks(void)
{
    jpb_TextSetFontLoadHook(portable_text_load_font, NULL);
    jpb_TextUtilSetFontMetricsHooks(
        portable_text_set_font_size,
        portable_text_glyph_metrics,
        portable_text_get_error);
}

static JPBPortableTextFont *portable_text_get_font(
    int font_style, int language, int point_size)
{
    currentLanguage = language;
    jpb_PortableTextInstallHooks();
    return (JPBPortableTextFont *)(void *)LoadFont(
        font_style, point_size);
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
    const uint16_t *text,
    int point_size,
    int *width_out,
    int *height_out,
    int *baseline_out)
{
    const uint16_t *cursor = text;
    int line_width = 0;
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
        uint16_t codepoint = *cursor++;
        int min_x = 0;
        int max_x = 0;
        int min_y = 0;
        int max_y = 0;
        int advance = 0;
        int glyph_width;
        int width_with_last_glyph;

        if (codepoint == L'\n') {
            height += line_max_y - line_min_y;
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
    *width_out = line_width;
    *height_out = height;
    *baseline_out = line_max_y;
    return 1;
}
#endif

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

static int portable_text_tag_icon(uint16_t codepoint, int *alpha)
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
#else
    (void)point_size;
    (void)codepoint;
#endif
    return 0;
}

int jpb_PortableTextPrepareControlGlyphs(
    uint16_t *text,
    size_t text_capacity,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    int language,
    float scale_adjustment,
    int *origin_x,
    int *text_width_out,
    JPBPortableTextControlGlyph *glyphs,
    size_t glyph_capacity)
{
    JPBPortableTextFont *font;
    size_t length;
    size_t index = 0;
    size_t glyph_count = 0;
    int point_size;
    int text_width;
    int text_height;
    int baseline_offset;
    int pen_x;
    int pen_y;

    if (text == NULL || text_capacity == 0 || origin_x == NULL ||
        text_width_out == NULL ||
        (glyph_capacity != 0 && glyphs == NULL)) {
        return -1;
    }
    length = jpb_utf16_length(text);
    if (length >= text_capacity) {
        return -1;
    }
    point_size = jpb_PortableTextPointSize(scale, scale_adjustment);
    font = portable_text_get_font(font_style, language, point_size);
    if (font == NULL) {
        return -1;
    }
#ifndef _WIN32
    return -1;
#else
    if (!portable_text_measure_sdl(
            font, text, point_size,
            &text_width, &text_height, &baseline_offset)) {
        return -1;
    }
#endif
    *text_width_out = text_width;
    (void)text_height;
    pen_x = x;
    if ((mode & 0x7f) == 1) {
        pen_x -= text_width;
    } else if ((mode & 0x7f) == 2) {
        pen_x -= text_width / 2;
    }
    *origin_x = pen_x;
    pen_y = y;

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
            jpb_utf16_fill(text, (uint16_t)' ', leading_spaces);
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
                    (int)((float)pen_y +
                          (float)baseline_offset * 0.5f);
                ++glyph_count;
            }
            if (tagged) {
                text[index] = L' ';
                text[index + 1] = L' ';
                text[index + 2] = L' ';
            } else {
                text[index] = L' ';
            }
            continue;
        }
        if (text[index] == L'\n') {
            pen_x = *origin_x;
            pen_y = (int)(
                (float)pen_y + (float)baseline_offset * 1.5f);
            ++index;
            continue;
        }
        if (!portable_text_control_advance(
                font, point_size,
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
typedef struct JPBPortableGlyphBitmap {
    struct JPBPortableGlyphBitmap *next;
    void *sdlFont;
    int pointSize;
    uint16_t glyph;
    int minX;
    int maxX;
    int minY;
    int maxY;
    int advance;
    int width;
    int height;
    uint32_t *pixels;
    JPBSoftwareTexture texture;
    _Material material;
} JPBPortableGlyphBitmap;

static JPBPortableGlyphBitmap *portableTextGlyphBitmaps;

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

static JPBPortableGlyphBitmap *portable_text_get_glyph_bitmap(
    JPBPortableTextFont *font,
    int point_size,
    uint16_t glyph)
{
    JPBPortableGlyphBitmap *entry;
    JPBPortableSDLColor white = {255, 255, 255, 255};
    JPBPortableSDLSurface *surface;
    int crop_x;
    int crop_y;
    int row;

    if (!portable_text_prepare_sdl_font(font, point_size) ||
        portable_TTF_RenderGlyph_Blended == NULL ||
        portable_TTF_FontAscent == NULL ||
        portable_SDL_FreeSurface == NULL ||
        portable_TTF_GlyphMetrics == NULL) {
        return NULL;
    }
    for (entry = portableTextGlyphBitmaps;
         entry != NULL;
         entry = entry->next) {
        if (entry->sdlFont == font->sdlFont &&
            entry->pointSize == point_size && entry->glyph == glyph) {
            return entry;
        }
    }
    entry = (JPBPortableGlyphBitmap *)calloc(1, sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    if (portable_TTF_GlyphMetrics(
            font->sdlFont,
            glyph,
            &entry->minX,
            &entry->maxX,
            &entry->minY,
            &entry->maxY,
            &entry->advance) != 0) {
        free(entry);
        return NULL;
    }
    entry->sdlFont = font->sdlFont;
    entry->pointSize = point_size;
    entry->glyph = glyph;
    entry->width = entry->maxX - entry->minX;
    entry->height = entry->maxY - entry->minY;
    surface = portable_TTF_RenderGlyph_Blended(
        font->sdlFont, glyph, white);
    if (surface == NULL || surface->pixels == NULL ||
        surface->format == NULL || surface->pitch <= 0 ||
        entry->width < 0 || entry->height < 0) {
        if (surface != NULL) {
            portable_SDL_FreeSurface(surface);
        }
        free(entry);
        return NULL;
    }
    if (entry->width != 0 && entry->height != 0) {
        size_t pixel_count;

        if ((size_t)entry->width >
            SIZE_MAX / (size_t)entry->height) {
            portable_SDL_FreeSurface(surface);
            free(entry);
            return NULL;
        }
        pixel_count =
            (size_t)entry->width * (size_t)entry->height;
        entry->pixels = (uint32_t *)calloc(
            pixel_count, sizeof(entry->pixels[0]));
        if (entry->pixels == NULL) {
            portable_SDL_FreeSurface(surface);
            free(entry);
            return NULL;
        }
    }
    if (portable_SDL_LockSurface != NULL) {
        (void)portable_SDL_LockSurface(surface);
    }
    crop_x = entry->minX < 0 ? 0 : entry->minX;
    crop_y = portable_TTF_FontAscent(font->sdlFont) - entry->maxY;
    for (row = 0; row < entry->height; ++row) {
        int source_y = crop_y + row;
        int column;

        if (source_y < 0 || source_y >= surface->h) {
            continue;
        }
        for (column = 0; column < entry->width; ++column) {
            int source_x = crop_x + column;
            const uint8_t *source_pixel;
            uint32_t pixel = 0;

            if (source_x < 0 || source_x >= surface->w) {
                continue;
            }
            source_pixel =
                (const uint8_t *)surface->pixels +
                (size_t)source_y * (size_t)surface->pitch +
                (size_t)source_x *
                    (size_t)surface->format->bytesPerPixel;
            memcpy(
                &pixel,
                source_pixel,
                surface->format->bytesPerPixel <= 4
                    ? surface->format->bytesPerPixel
                    : 4);
            entry->pixels[
                (size_t)row * (size_t)entry->width +
                (size_t)column] = UINT32_C(0x00ffffff) |
                    ((uint32_t)portable_text_surface_component(
                        pixel,
                        surface->format->amask,
                        surface->format->ashift,
                        surface->format->aloss) << 24);
        }
    }
    if (portable_SDL_UnlockSurface != NULL) {
        portable_SDL_UnlockSurface(surface);
    }
    portable_SDL_FreeSurface(surface);
    entry->texture.pixels = entry->pixels;
    entry->texture.width = (size_t)entry->width;
    entry->texture.height = (size_t)entry->height;
    entry->texture.stridePixels = (size_t)entry->width;
    entry->texture.materialFlags = JPB_MATERIAL_MODE_TWO_SIDED;
    entry->texture.samplerType = TEXTURESAMPLER_LINEARCLAMP;
    entry->texture.colorOverride = -1;
    entry->texture.materialType = 2;
    entry->material.texture = &entry->texture;
    entry->material.type = 2;
    entry->material.iw = (int16_t)entry->width;
    entry->material.ih = (int16_t)entry->height;
    entry->material.m_isTransparent = 1;
    entry->material.flags = JPB_MATERIAL_MODE_TWO_SIDED;
    entry->material.samplerType = TEXTURESAMPLER_LINEARCLAMP;
    entry->material.colorOverride = -1;
    entry->next = portableTextGlyphBitmaps;
    portableTextGlyphBitmaps = entry;
    return entry;
}

static void portable_text_draw_glyph_bitmap(
    const JPBPortableGlyphBitmap *glyph,
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
    int row;

    for (row = 0; row < glyph->height; ++row) {
        int output_y = y + row;
        int column;

        if (output_y < 0 || output_y >= framebuffer->height ||
            (clip_enabled &&
             (output_y < clip_top || output_y >= clip_bottom))) {
            continue;
        }
        for (column = 0; column < glyph->width; ++column) {
            int output_x = x + column;
            uint8_t source_alpha = (uint8_t)(glyph->pixels[
                (size_t)row * (size_t)glyph->width +
                (size_t)column] >> 24);

            if (source_alpha == 0 || output_x < 0 ||
                output_x >= framebuffer->width ||
                (clip_enabled &&
                 (output_x < clip_left || output_x >= clip_right))) {
                continue;
            }
            source_alpha = (uint8_t)(
                (uint32_t)source_alpha * (uint32_t)alpha /
                UINT32_C(255));
            if (source_alpha == 0) {
                continue;
            }
            framebuffer->pixels[
                (size_t)output_y *
                    (size_t)framebuffer->stridePixels +
                (size_t)output_x] = portable_text_blend(
                    framebuffer->pixels[
                        (size_t)output_y *
                            (size_t)framebuffer->stridePixels +
                        (size_t)output_x],
                    red, green, blue, source_alpha);
            if (blended_pixels != NULL) {
                ++*blended_pixels;
            }
        }
    }
}

static int portable_text_draw_sdl(
    JPBPortableTextFont *font,
    const uint16_t *text,
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
    const uint16_t *cursor;
    int text_max_y = 0;
    int pen_x = x;
    int pen_y = y;

    if (!portable_text_prepare_sdl_font(font, point_size) ||
        text == NULL || framebuffer == NULL ||
        framebuffer->pixels == NULL) {
        return 0;
    }
    for (cursor = text; *cursor != L'\0'; ++cursor) {
        JPBPortableGlyphBitmap *glyph;

        if (*cursor == L'\n') {
            continue;
        }
        glyph = portable_text_get_glyph_bitmap(
            font, point_size, *cursor);
        if (glyph != NULL && text_max_y < glyph->maxY) {
            text_max_y = glyph->maxY;
        }
    }
    for (cursor = text; *cursor != L'\0'; ++cursor) {
        JPBPortableGlyphBitmap *glyph;

        if (*cursor == L'\n') {
            pen_x = x;
            pen_y = (int)(
                (float)pen_y + (float)text_max_y * 1.5f);
            continue;
        }
        glyph = portable_text_get_glyph_bitmap(
            font, point_size, *cursor);
        if (glyph == NULL) {
            continue;
        }
        portable_text_draw_glyph_bitmap(
            glyph,
            pen_x,
            pen_y + text_max_y - glyph->maxY,
            red,
            green,
            blue,
            alpha,
            clip_enabled,
            clip_left,
            clip_top,
            clip_right,
            clip_bottom,
            framebuffer,
            blended_pixels);
        pen_x += glyph->advance - glyph->minX;
    }
    return 1;
}
#endif

int jpb_PortableTextDrawPointSize(
    const uint16_t *text,
    uint32_t color,
    int mode,
    int x,
    int y,
    int point_size,
    int font_style,
    int language,
    int clip_enabled,
    int clip_left,
    int clip_top,
    int clip_right,
    int clip_bottom,
    JPBSoftwareFramebuffer *framebuffer,
    JPBPortableTextMetrics *metrics)
{
    JPBPortableTextFont *font;
    int text_width;
    int text_height;
    int baseline_offset;
    int pen_x;
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
    font = portable_text_get_font(
        font_style, language, point_size);
    if (font == NULL) {
        return 0;
    }
#ifndef _WIN32
    return 0;
#else
    if (!portable_text_measure_sdl(
            font,
            text,
            point_size,
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
    (void)baseline_offset;
    red = (uint8_t)(color & UINT32_C(0xff));
    green = (uint8_t)((color >> 8) & UINT32_C(0xff));
    blue = (uint8_t)((color >> 16) & UINT32_C(0xff));
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
            (uint8_t)(color >> 24),
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
    return 0;
#endif
}

int jpb_PortableTextDraw(
    const uint16_t *text,
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
    uint32_t color = jpb_PortableTextTint(tint);

    color = (color & UINT32_C(0x00ffffff)) |
        ((uint32_t)(uint8_t)alpha << 24);
    return jpb_PortableTextDrawPointSize(
        text,
        color,
        mode,
        x,
        y,
        jpb_PortableTextPointSize(scale, scale_adjustment),
        font_style,
        language,
        clip_enabled,
        clip_left,
        clip_top,
        clip_right,
        clip_bottom,
        framebuffer,
        metrics);
}

int jpb_PortableTextEmit3D(
    const uint16_t *text,
    uint32_t color,
    int mode,
    float x,
    float y,
    float z,
    float scale,
    int font_style,
    int language,
    JPBPortableText3DGlyphHook glyph_hook,
    void *user_data)
{
    return jpb_PortableTextEmit3DPointSize(
        text,
        color,
        mode,
        x,
        y,
        z,
        jpb_PortableTextPointSize(scale, 1.0f),
        font_style,
        language,
        glyph_hook,
        user_data);
}

int jpb_PortableTextEmit3DPointSize(
    const uint16_t *text,
    uint32_t color,
    int mode,
    float x,
    float y,
    float z,
    int point_size,
    int font_style,
    int language,
    JPBPortableText3DGlyphHook glyph_hook,
    void *user_data)
{
#ifndef _WIN32
    (void)text;
    (void)color;
    (void)mode;
    (void)x;
    (void)y;
    (void)z;
    (void)point_size;
    (void)font_style;
    (void)language;
    (void)glyph_hook;
    (void)user_data;
    return 0;
#else
    JPBPortableTextFont *font;
    const uint16_t *cursor;
    int text_width;
    int text_height;
    int baseline;
    float size_ratio;
    float max_height = 0.0f;
    float origin_x;
    float pen_x;
    float pen_y = y;

    if (text == NULL || glyph_hook == NULL) {
        return 0;
    }
    font = portable_text_get_font(font_style, language, point_size);
    if (font == NULL || !portable_text_measure_sdl(
            font, text, point_size,
            &text_width, &text_height, &baseline)) {
        return 0;
    }
    (void)text_height;
    (void)baseline;
    origin_x = x;
    if ((mode & 0x7f) == 1) {
        origin_x -= (float)text_width;
    } else if ((mode & 0x7f) == 2) {
        origin_x -= (float)(text_width / 2);
    }
    pen_x = origin_x;
    size_ratio = (float)point_size / 50.0f;
    for (cursor = text; *cursor != L'\0'; ++cursor) {
        JPBPortableGlyphBitmap *glyph;

        if (*cursor == L'\n') {
            continue;
        }
        glyph = portable_text_get_glyph_bitmap(
            font, point_size, *cursor);
        if (glyph != NULL &&
            max_height < (float)glyph->maxY * size_ratio) {
            max_height = (float)glyph->maxY * size_ratio;
        }
    }
    for (cursor = text; *cursor != L'\0'; ++cursor) {
        JPBPortableGlyphBitmap *glyph;
        JPBScreenPolyVertex vertices[4];
        float width;
        float height;
        float y_offset;

        if (*cursor == L'\n') {
            pen_x = origin_x;
            pen_y += max_height * 1.5f;
            continue;
        }
        glyph = portable_text_get_glyph_bitmap(
            font, point_size, *cursor);
        if (glyph == NULL) {
            continue;
        }
        width = (float)glyph->width * size_ratio;
        height = (float)glyph->height * size_ratio;
        y_offset = (max_height - height) * 0.5f;
        vertices[0] = (JPBScreenPolyVertex){
            pen_x, pen_y + y_offset, z, color, 0.0f, 0.0f};
        vertices[1] = (JPBScreenPolyVertex){
            pen_x + width, pen_y + y_offset, z, color, 1.0f, 0.0f};
        vertices[2] = (JPBScreenPolyVertex){
            pen_x, pen_y + y_offset + height, z, color, 0.0f, 1.0f};
        vertices[3] = (JPBScreenPolyVertex){
            pen_x + width, pen_y + y_offset + height,
            z, color, 1.0f, 1.0f};
        glyph_hook(user_data, &glyph->material, vertices);
        pen_x += width + size_ratio + size_ratio;
    }
    return 1;
#endif
}

void jpb_PortableTextShutdown(void)
{
    size_t index;

#ifdef _WIN32
    while (portableTextGlyphBitmaps != NULL) {
        JPBPortableGlyphBitmap *next = portableTextGlyphBitmaps->next;

        free(portableTextGlyphBitmaps->pixels);
        free(portableTextGlyphBitmaps);
        portableTextGlyphBitmaps = next;
    }
#endif
    for (index = 0;
         index < JPB_PORTABLE_TEXT_FONT_CACHE_CAPACITY;
         ++index) {
#ifdef _WIN32
        if (portable_TTF_CloseFont != NULL &&
            portableTextFonts[index].sdlFont != NULL) {
            portable_TTF_CloseFont(portableTextFonts[index].sdlFont);
        }
#endif
    }
    memset(portableTextFonts, 0, sizeof(portableTextFonts));
    jpb_TextSetFontLoadHook(NULL, NULL);
    jpb_TextUtilSetFontMetricsHooks(NULL, NULL, NULL);
    ClearGlyphCache();
    jpb_TextResetFontCache();
}
