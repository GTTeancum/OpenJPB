/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\text.c.
 *
 * All 45 emitted procedures and the module-owned tables are checked against
 * the matched PDB and shipped executable. Focused and mapped-retail
 * regressions cover text metrics and writers, clipping, frontend quads,
 * menu nine-slices, menu art groups, depth, and initialized data.
 *
 * PDB module: 0084
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\text.obj
 * Primary source: W:\SWJediPowerBattles\Work\text.c
 * Compiler language: c
 * Emitted procedures: 45
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/text.h"

#include "jpb/alltext.h"
#include "jpb/debugtext.h"
#include "jpb/game.h"
#include "jpb/menu.h"
#include "jpb/sprite.h"
#include "jpb/textutil.h"
#include "jpb/whook.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static JPBTextDrawHook jpb_text_draw_hook;
static void *jpb_text_draw_user_data;
static JPBTextDraw3DHook jpb_text_draw_3d_hook;
static void *jpb_text_draw_3d_user_data;
static JPBTextFontLoadHook jpb_text_font_load_hook;
static void *jpb_text_font_load_user_data;
static int jpb_text_clip_enabled;
static int jpb_text_clip_left;
static int jpb_text_clip_top;
static int jpb_text_clip_right;
static int jpb_text_clip_bottom;

/*
 * The recovered MM/menu writer uses scaleAdjustmentMM directly. Keep this
 * owner separate from ordinary HUD text, but do not inflate the point size
 * when the portable renderer consumes the draw.
 */

/* Exact PDB global spanning RVAs 0x944780..0x945D0F. */
FONTSPEC fontSpec[JPB_FONT_SPEC_COUNT];
/* Exact initialized PDB global at matched-PC RVA 0x4CD6A0. */
uint8_t sfont18_nfont[608] = {
    9, 17, 241, 237, 254, 254, 11, 17, 0, 0, 11, 20, 10, 17, 11, 0,
    21, 7, 19, 17, 21, 0, 40, 20, 23, 18, 40, 0, 63, 23, 24, 18,
    63, 0, 87, 21, 21, 17, 87, 0, 108, 20, 6, 17, 108, 0, 114, 7,
    14, 17, 114, 0, 128, 23, 14, 17, 128, 0, 142, 23, 13, 17, 142, 0,
    155, 12, 15, 12, 155, 0, 170, 14, 6, 1, 170, 0, 176, 7, 10, 7,
    176, 0, 186, 5, 5, 1, 186, 0, 191, 4, 27, 18, 191, 0, 218, 24,
    22, 17, 218, 0, 241, 20, 22, 17, 241, 0, 254, 20, 22, 17, 0, 24,
    21, 44, 22, 17, 21, 24, 42, 44, 22, 17, 42, 24, 62, 44, 22, 17,
    62, 24, 83, 44, 22, 17, 83, 24, 104, 44, 22, 17, 104, 24, 124, 44,
    22, 17, 124, 24, 145, 44, 22, 17, 145, 24, 167, 44, 9, 12, 167, 24,
    176, 39, 10, 12, 176, 24, 186, 42, 17, 11, 186, 24, 203, 38, 17, 9,
    203, 24, 220, 32, 17, 11, 220, 24, 237, 38, 15, 17, 237, 24, 252, 44,
    25, 17, 0, 44, 25, 64, 18, 17, 25, 44, 43, 64, 21, 17, 43, 44,
    64, 64, 21, 17, 64, 44, 85, 64, 23, 17, 85, 44, 108, 64, 20, 17,
    108, 44, 128, 64, 19, 17, 128, 44, 147, 64, 23, 17, 147, 44, 170, 64,
    24, 17, 170, 44, 194, 64, 11, 17, 194, 44, 205, 64, 20, 17, 205, 44,
    225, 64, 23, 18, 225, 44, 248, 66, 14, 17, 0, 66, 14, 86, 29, 17,
    14, 66, 43, 86, 25, 17, 43, 66, 68, 87, 22, 17, 68, 66, 90, 86,
    21, 17, 90, 66, 111, 86, 22, 17, 111, 66, 133, 86, 22, 17, 133, 66,
    155, 86, 21, 17, 155, 66, 176, 86, 18, 17, 176, 66, 194, 86, 23, 17,
    194, 66, 217, 86, 19, 17, 217, 66, 236, 86, 30, 17, 0, 87, 30, 107,
    24, 18, 30, 87, 54, 109, 19, 18, 54, 87, 73, 108, 23, 17, 73, 87,
    96, 107, 15, 17, 96, 87, 111, 110, 5, 17, 111, 87, 116, 107, 14, 17,
    131, 87, 145, 101, 15, 253, 145, 87, 160, 90, 7, 17, 160, 87, 167, 93,
    7, 17, 160, 87, 167, 93, 17, 12, 167, 87, 184, 102, 17, 17, 184, 87,
    201, 107, 16, 12, 201, 87, 217, 102, 19, 17, 217, 87, 236, 107, 16, 12,
    236, 87, 252, 102, 14, 17, 0, 110, 14, 130, 20, 12, 14, 110, 34, 130,
    17, 17, 34, 110, 51, 130, 11, 17, 51, 110, 62, 130, 19, 17, 62, 110,
    81, 135, 17, 17, 81, 110, 98, 131, 11, 17, 98, 110, 109, 130, 26, 12,
    109, 110, 135, 125, 17, 12, 135, 110, 152, 125, 17, 12, 152, 110,
    169, 125, 19, 12, 169, 110, 188, 130, 18, 12, 188, 110, 206, 130, 16, 12,
    206, 110, 222, 125, 17, 12, 222, 110, 239, 125, 12, 15, 239, 110,
    251, 128, 17, 12, 0, 135, 17, 150, 14, 12, 17, 135, 31, 150, 22, 12,
    31, 135, 53, 151, 18, 13, 53, 135, 71, 152, 18, 12, 71, 135, 89, 155,
    17, 12, 89, 135, 106, 150, 14, 17, 106, 135, 120, 158, 11, 17,
    120, 135, 131, 155, 13, 17, 131, 135, 144, 158, 15, 10, 144, 135,
    159, 144, 24, 24, 16, 240, 32, 255, 24, 17, 24, 210, 48, 234,
    24, 17, 48, 210, 72, 234, 24, 17, 72, 210, 96, 234, 24, 17,
    0, 210, 24, 234, 24, 17, 96, 210, 120, 234, 0, 0,
};
/* Exact initialized PDB global at matched-PC RVA 0x4CC930. */
uint8_t asciiRemap[JPB_ASCII_REMAP_COUNT] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x99, 0x3f, 0x55, 0x41, 0x42, 0x43, 0x45, 0x53,
    0x47, 0x48, 0x46, 0x51, 0x58, 0x49, 0x59, 0x5b,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x54, 0x52, 0x56, 0x4a, 0x57, 0x5a,
    0x40, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x4c, 0x50, 0x4d, 0x44, 0x49,
    0x5d, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
    0x22, 0x23, 0x24, 0x4e, 0x51, 0x4f, 0x49, 0x01,
    0xa6, 0xcd, 0xd1, 0xd3, 0xab, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x5d, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x61, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x96, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x98, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x01, 0x01, 0x6a,
    0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72,
    0x01, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x01,
    0x01, 0x79, 0x7a, 0x7b, 0x7c, 0x01, 0x01, 0x7d,
    0x7e, 0x7f, 0x80, 0x81, 0x82, 0x01, 0x01, 0x83,
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b,
    0x01, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x01,
    0x01, 0x92, 0x93, 0x94, 0x95, 0x01, 0x01, 0x01,
};
/* Exact initialized PDB global at matched-PC RVA 0x4CCA30. */
FONT SmallFont[JPB_SMALL_FONT_COUNT] = {
    {0, 0, 6, 0, 0, 0},
    {31, 22, 4, 12, 0, -1}, {19, 22, 5, 12, 0, -2},
    {176, 22, 12, 12, 0, -2}, {200, 22, 12, 12, 0, -2},
    {128, 22, 13, 12, 0, 0}, {22, 36, 15, 8, 0, 1},
    {16, 22, 3, 12, 0, -2}, {47, 22, 5, 12, 0, -2},
    {52, 22, 5, 12, 0, -2}, {162, 22, 14, 12, 0, -2},
    {37, 36, 8, 7, 0, 2}, {0, 22, 4, 12, 0, -1},
    {141, 22, 7, 12, 0, 0}, {4, 22, 4, 12, 0, -1},
    {115, 22, 13, 12, 0, -2}, {247, 10, 7, 12, 0, 0},
    {186, 10, 5, 12, 0, 0}, {191, 10, 7, 12, 0, 0},
    {198, 10, 7, 12, 0, 0}, {205, 10, 8, 12, 0, 0},
    {213, 10, 7, 12, 0, 0}, {220, 10, 7, 12, 0, 0},
    {227, 10, 6, 12, 0, 0}, {233, 10, 7, 12, 0, 0},
    {240, 10, 7, 12, 0, 0}, {8, 22, 4, 12, 0, -2},
    {12, 22, 4, 12, 0, -2}, {212, 22, 12, 12, 0, -2},
    {0, 0, 0, 0, 0, 0}, {188, 22, 12, 12, 0, -2},
    {24, 22, 7, 12, 0, -1}, {148, 22, 14, 12, 0, -2},
    {0, 0, 9, 10, 0, 0}, {9, 0, 9, 10, 0, 0},
    {18, 0, 8, 10, 0, 0}, {26, 0, 9, 10, 0, 0},
    {35, 0, 7, 10, 0, 0}, {42, 0, 7, 10, 0, 0},
    {49, 0, 9, 10, 0, 0}, {58, 0, 8, 10, 0, 0},
    {66, 0, 4, 10, 0, 0}, {70, 0, 7, 10, 0, 0},
    {77, 0, 8, 10, 0, 0}, {85, 0, 8, 10, 0, 0},
    {93, 0, 9, 10, 0, 0}, {102, 0, 8, 10, 0, 0},
    {110, 0, 10, 10, 0, 0}, {120, 0, 8, 10, 0, 0},
    {128, 0, 10, 10, 0, 0}, {138, 0, 9, 10, 0, 0},
    {147, 0, 9, 10, 0, 0}, {156, 0, 8, 10, 0, 0},
    {164, 0, 8, 10, 0, 0}, {172, 0, 9, 10, 0, 0},
    {181, 0, 13, 10, 0, 0}, {194, 0, 8, 10, 0, 0},
    {202, 0, 8, 10, 0, 0}, {210, 0, 8, 10, 0, 0},
    {35, 22, 6, 12, 0, -2}, {57, 22, 14, 12, 0, -2},
    {41, 22, 6, 12, 0, -2}, {71, 22, 14, 12, 0, -2},
    {85, 22, 16, 12, 0, -2}, {101, 22, 14, 12, 0, -2},
    {0, 10, 7, 12, 0, 0}, {7, 10, 8, 12, 0, 0},
    {15, 10, 7, 12, 0, 0}, {22, 10, 8, 12, 0, 0},
    {30, 10, 8, 12, 0, 0}, {38, 10, 6, 12, 0, 0},
    {44, 10, 8, 12, 0, 0}, {52, 10, 7, 12, 0, 0},
    {59, 10, 4, 12, 0, 0}, {63, 10, 5, 12, 0, 0},
    {68, 10, 7, 12, 0, 0}, {75, 10, 4, 12, 0, 0},
    {79, 10, 10, 12, 0, 0}, {89, 10, 7, 12, 0, 0},
    {96, 10, 8, 12, 0, 0}, {104, 10, 8, 12, 0, 0},
    {112, 10, 8, 12, 0, 0}, {120, 10, 5, 12, 0, 0},
    {125, 10, 7, 12, 0, 0}, {132, 10, 6, 12, 0, 0},
    {138, 10, 7, 12, 0, 0}, {145, 10, 7, 12, 0, 0},
    {152, 10, 11, 12, 0, 0}, {163, 10, 7, 12, 0, 0},
    {170, 10, 9, 12, -1, 0}, {179, 10, 7, 12, 0, 0},
    {64, 219, 20, 14, 0, 0}, {84, 219, 20, 14, 0, 0},
    {104, 219, 20, 14, 0, 0}, {124, 219, 20, 14, 0, 0},
    {144, 219, 20, 14, 0, 0}, {164, 219, 20, 14, 0, 0},
    {184, 219, 20, 14, 0, 0}, {224, 22, 12, 12, 0, -2},
    {236, 22, 12, 12, 0, -2}, {0, 35, 11, 11, 0, -2},
    {11, 35, 11, 11, 0, -2},
};
/* Exact initialized PDB globals at RVAs 0x4CCC94 and 0x4CD100. */
uint32_t MonospaceWidth = 10;
CVECTOR Colors[JPB_TEXT_COLOR_COUNT] = {
    {0xe0, 0x60, 0x40, 0xff}, {0x80, 0x00, 0x00, 0xff},
    {0x80, 0xa0, 0x80, 0xff}, {0x00, 0x80, 0x00, 0xff},
    {0x80, 0x80, 0xe0, 0xff}, {0x00, 0x00, 0x80, 0xff},
    {0xe0, 0xe0, 0x80, 0xff}, {0xd0, 0x80, 0xe0, 0xff},
    {0x00, 0xe0, 0xe0, 0xff}, {0x30, 0x30, 0x30, 0xff},
    {0x60, 0x60, 0x60, 0xff}, {0xf0, 0xf0, 0xf0, 0xff},
    {0xbf, 0x61, 0x3f, 0xff}, {0x30, 0xf0, 0xf0, 0xff},
    {0xa5, 0xf5, 0xa5, 0xff}, {0xc0, 0xc0, 0xc0, 0xff},
    {0x93, 0x90, 0xe0, 0xff},
};
float gPSXDrawScaleX = 1.0f;
float gPSXDrawScaleY = 1.0f;
float gPSXDrawScaleW = 3.75f;
float gPSXDrawScaleH = 4.5f;
/* Exact PDB global at matched-PC RVA 0x4CCC98. */
float frontGamma = 1.0f;
/* Exact zero-initialized PDB global at matched-PC RVA 0x539DC4. */
float frontZ;
/* Exact PDB global at matched-PC RVA 0x94477C. */
uint8_t frontRGBoff;
/* Exact zero-initialized PDB global at matched-PC RVA 0x539D94. */
int comboIconOverride;
int player2IconOverride;

_Static_assert(
    sizeof(FONTSPEC) == 12,
    "FONTSPEC must match matched-PC PDB type 0x78FB");
_Static_assert(sizeof(FONT) == 6, "FONT must match matched-PC PDB type 0x78F8");

/* Exact PDB globals at matched-PC RVAs 0x548120..0x548158. */
int currentFontStyle;
_TTF_Font *currentlyLoadedFont;
_TTF_Font *currentRegularFont;
_TTF_Font *currentItalicFont;
_TTF_Font *currentBoldFont;
_TTF_Font *currentSCRegularFont;
_TTF_Font *currentSCItalicFont;
_TTF_Font *currentSCBoldFont;

void jpb_TextSetDrawHook(
    JPBTextDrawHook hook, void *user_data)
{
    jpb_text_draw_hook = hook;
    jpb_text_draw_user_data = user_data;
}

void jpb_TextSetDraw3DHook(
    JPBTextDraw3DHook hook, void *user_data)
{
    jpb_text_draw_3d_hook = hook;
    jpb_text_draw_3d_user_data = user_data;
}

void jpb_TextSetFontLoadHook(
    JPBTextFontLoadHook hook, void *user_data)
{
    jpb_text_font_load_hook = hook;
    jpb_text_font_load_user_data = user_data;
}

void jpb_TextSetClipRect(
    int left, int top, int right, int bottom)
{
    jpb_text_clip_left = left;
    jpb_text_clip_top = top;
    jpb_text_clip_right = right;
    jpb_text_clip_bottom = bottom;
    jpb_text_clip_enabled = left < right && top < bottom;
}

void jpb_TextClearClipRect(void)
{
    jpb_text_clip_enabled = 0;
}

int jpb_TextGetClipRect(
    int *left, int *top, int *right, int *bottom)
{
    if (!jpb_text_clip_enabled) {
        return 0;
    }
    if (left != NULL) *left = jpb_text_clip_left;
    if (top != NULL) *top = jpb_text_clip_top;
    if (right != NULL) *right = jpb_text_clip_right;
    if (bottom != NULL) *bottom = jpb_text_clip_bottom;
    return 1;
}

void jpb_TextResetFontCache(void)
{
    currentFontStyle = 0;
    currentlyLoadedFont = NULL;
    currentRegularFont = NULL;
    currentItalicFont = NULL;
    currentBoldFont = NULL;
    currentSCRegularFont = NULL;
    currentSCItalicFont = NULL;
    currentSCBoldFont = NULL;
}

/* 0xFDC90, 289 bytes, global, 9 named locals
 * LoadFont
 * PDB type: _TTF_Font* (int, int)
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

_TTF_Font *LoadFont(int fontStyle, int pointSize)
{
    char *font = getFontFile(fontStyle);
    _TTF_Font **slot;

    if (currentLanguage == 6) {
        if (fontStyle == 1) {
            slot = &currentSCItalicFont;
        } else if (fontStyle == 2) {
            slot = &currentSCBoldFont;
        } else {
            slot = &currentSCRegularFont;
        }
    } else if (fontStyle == 1) {
        slot = &currentItalicFont;
    } else if (fontStyle == 2) {
        slot = &currentBoldFont;
    } else {
        slot = &currentRegularFont;
    }
    if (*slot == NULL && jpb_text_font_load_hook != NULL) {
        *slot = jpb_text_font_load_hook(
            jpb_text_font_load_user_data, font, pointSize);
    }
    return *slot;
}
static uint16_t *jpb_text_format_utf16(
    char formatted_string[512], char *format, va_list arguments)
{
    unsigned short *utf_string = NULL;

    (void)vsnprintf(formatted_string, 512, format, arguments);
    ConvertToUTF16(formatted_string, &utf_string);
    return (uint16_t *)(void *)utf_string;
}

static int jpb_text_draw_2d(
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
    char *format,
    va_list arguments)
{
    char formatted_string[512];
    uint16_t *utf_string;
    SCREENRECT destination;
    CVECTOR color;
    int point_size;
    int width = 0;
    int height = 0;
    int requested_x = x;

    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    point_size = (int)(scale * scale_adjustment * 24.0f);
    if (currentlyLoadedFont == NULL ||
        currentFontStyle != font_style) {
        currentFontStyle = font_style;
        currentlyLoadedFont = LoadFont(font_style, point_size);
    }
    utf_string = jpb_text_format_utf16(
        formatted_string, format, arguments);
    SizeText(
        currentlyLoadedFont,
        point_size,
        (const unsigned short *)(const void *)utf_string,
        &width,
        &height);
    if ((mode & 0x7f) == 1) {
        x -= width;
    } else if ((mode & 0x7f) == 2) {
        x -= width / 2;
    }
    destination.left = x;
    destination.top = y;
    destination.right = x + width;
    destination.bottom = y + height;
    color = Colors[tint];
    color.cd = (uint8_t)alpha;
    if (depth_enabled) {
        _DrawUITextUTF16Depth(
            utf_string,
            destination,
            font_style,
            point_size,
            color,
            depth);
    } else {
        _DrawUITextUTF16(
            utf_string,
            destination,
            font_style,
            point_size,
            color);
    }
    if (jpb_text_draw_hook != NULL) {
        jpb_text_draw_hook(
            jpb_text_draw_user_data,
            tint,
            alpha,
            mode,
            requested_x,
            y,
            scale,
            scale_adjustment,
            font_style,
            depth_enabled,
            depth,
            utf_string);
    }
    free(utf_string);
    return width;
}

/* 0xFDDC0, 371 bytes, global, 12 named locals
 * SDLTextWrite
 * PDB type: int (int, int, int, int, int, wc...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int SDLTextWrite(
    int tint,
    int mode,
    int x,
    int y,
    int italic,
    char *format,
    ...)
{
    va_list arguments;
    int width;

    va_start(arguments, format);
    width = jpb_text_draw_2d(
        tint, 255, mode, x, y, 1.0f, 1.0f,
        italic != 0, 0, 0.0f, format, arguments);
    va_end(arguments);
    return width;
}

/* 0xFDF40, 430 bytes, global, 15 named locals
 * SDLTextWriteScale
 * PDB type: int (int, int, int, int, int, fl...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int SDLTextWriteScale(
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    char *format,
    ...)
{
    va_list arguments;
    int width;

    va_start(arguments, format);
    width = jpb_text_draw_2d(
        tint, alpha, mode, x, y, scale, scaleAdjustment,
        font_style, 0, 0.0f, format, arguments);
    va_end(arguments);
    return width;
}

/* 0xFE0F0, 401 bytes, global, 13 named locals
 * SDLTextWriteScale3D
 * PDB type: int (unsigned long, int, float, ...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int SDLTextWriteScale3D(
    uint32_t color,
    int mode,
    float x,
    float y,
    float z,
    float scale,
    int font_style,
    char *format,
    ...)
{
    char formatted_string[512];
    uint16_t *utf_string;
    va_list arguments;
    int point_size;
    int width = 0;
    int height = 0;
    float requested_x = x;

    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    point_size = (int)(scale * 24.0f);
    if (currentlyLoadedFont == NULL ||
        currentFontStyle != font_style) {
        currentFontStyle = font_style;
        currentlyLoadedFont = LoadFont(font_style, point_size);
    }
    va_start(arguments, format);
    utf_string = jpb_text_format_utf16(
        formatted_string, format, arguments);
    va_end(arguments);
    SizeText(
        currentlyLoadedFont,
        point_size,
        (const unsigned short *)(const void *)utf_string,
        &width,
        &height);
    if ((mode & 0x7f) == 1) {
        x -= (float)width;
    } else if ((mode & 0x7f) == 2) {
        x -= (float)(width / 2);
    }
    _DrawUITextUTF16_3D(
        utf_string,
        x,
        y,
        z,
        font_style,
        point_size,
        color);
    if (jpb_text_draw_3d_hook != NULL) {
        jpb_text_draw_3d_hook(
            jpb_text_draw_3d_user_data,
            color,
            mode,
            requested_x,
            y,
            z,
            scale,
            font_style,
            utf_string);
    }
    free(utf_string);
    return width;
}

/* 0xFE290, 445 bytes, global, 16 named locals
 * SDLTextWriteScaleDepth
 * PDB type: int (int, int, int, int, int, fl...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int SDLTextWriteScaleDepth(
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    float depth,
    char *format,
    ...)
{
    va_list arguments;
    int width;

    va_start(arguments, format);
    width = jpb_text_draw_2d(
        tint, alpha, mode, x, y, scale, scaleAdjustment,
        font_style, 1, depth, format, arguments);
    va_end(arguments);
    return width;
}

/* 0xFE450, 430 bytes, global, 15 named locals
 * SDLTextWriteScaleMM
 * PDB type: int (int, int, int, int, int, fl...
 * Source: W:\SWJediPowerBattles\Work\text.c
 *
 * The matched MM variant differs from the ordinary
 * path only by using scaleAdjustmentMM when selecting its point size and
 * the menu-text draw owner. The publication hook carries the adjustment
 * explicitly so ordinary HUD text and 16:9 menu text retain their distinct
 * point-size owners at the framebuffer boundary.
 */
int SDLTextWriteScaleMM(
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    char *format,
    ...)
{
    va_list arguments;
    int width;

    va_start(arguments, format);
    width = jpb_text_draw_2d(
        tint, alpha, mode, x, y, scale,
        scaleAdjustmentMM,
        font_style, 0, 0.0f, format, arguments);
    va_end(arguments);
    return width;
}

/* 0xFE600, 445 bytes, global, 16 named locals
 * SDLTextWriteScaleMMDepth
 * PDB type: int (int, int, int, int, int, fl...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int SDLTextWriteScaleMMDepth(
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    float depth,
    char *format,
    ...)
{
    va_list arguments;
    int width;

    va_start(arguments, format);
    width = jpb_text_draw_2d(
        tint, alpha, mode, x, y, scale,
        scaleAdjustmentMM,
        font_style, 1, depth, format, arguments);
    va_end(arguments);
    return width;
}

/* 0xFE7C0, 177 bytes, global, 10 named locals
 * Text_gWrite
 * PDB type: int (short, char, int, int, int,...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int Text_gWrite(
    int16_t scale,
    int8_t brightness,
    int color,
    int flags,
    int x,
    int y,
    FONT *font,
    char *format,
    ...)
{
    char text[256];
    va_list arguments;

    (void)scale;
    (void)font;
    va_start(arguments, format);
    vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);
    return Text_gWriteSub(
        0x1000,
        brightness,
        color,
        flags,
        x,
        y,
        (uint8_t *)(void *)text);
}

/* 0xFE880, 638 bytes, global, 23 named locals
 * Text_gWriteSub
 * PDB type: int (int, char, int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int Text_gWriteSub(
    int scale,
    int8_t brightness,
    int color,
    int flags,
    int x,
    int y,
    uint8_t *text)
{
    uint8_t *cursor;
    int saw_newline = 0;
    int total_width = 0;
    int current_x;
    int red;
    int green;
    int blue;

    for (cursor = text; *cursor != 0; ++cursor) {
        unsigned glyph;
        unsigned glyph_width;

        if (*cursor == '\n') {
            saw_newline = 1;
        }
        glyph = asciiRemap[*cursor];
        glyph_width = fontSpec[glyph].w;
        if ((flags & 4) != 0) {
            glyph_width = MonospaceWidth;
        }
        if (!saw_newline) {
            total_width +=
                ((scale != 0x2000) + 1 + (int)glyph_width) * scale >> 12;
        }
    }

    if ((flags & 0x23) != 0) {
        if ((flags & 1) != 0) {
            x -= total_width;
        } else if ((flags & 2) != 0) {
            x -= total_width >> 1;
        }
    }

    red = (int)Colors[color].r * (int)brightness >> 7;
    green = (int)Colors[color].g * (int)brightness >> 7;
    blue = (int)Colors[color].b * (int)brightness >> 7;
    if (red != 0) {
        red += frontRGBoff;
    }
    if (green != 0) {
        green += frontRGBoff;
    }
    if (blue != 0) {
        blue += frontRGBoff;
    }
    if (red > 255) {
        red = 255;
    }
    if (green > 255) {
        green = 255;
    }
    if (blue > 255) {
        blue = 255;
    }

    current_x = x;
    for (cursor = text; *cursor != 0; ++cursor) {
        unsigned character = *cursor;
        unsigned glyph;
        int drawn_width;

        if (character == '\n') {
            if ((flags & 0x40) != 0) {
                character = ' ';
            } else {
                y += 26;
                current_x = x;
                continue;
            }
        }
        if ((flags & 0x20) != 0 &&
            total_width - (((int16_t)scale * 20) >> 12) <= current_x) {
            current_x -= total_width;
        }
        glyph = asciiRemap[character];
        drawn_width = winDrawTexture(
            glyph,
            (unsigned)current_x,
            (unsigned)(y + (uint8_t)fontSpec[glyph].xypage -
                       fontSpec[glyph].h),
            0,
            0,
            0,
            red,
            green,
            blue);
        current_x += 2 + drawn_width;
    }
    return total_width;
}

/* 0xFEB00, 32 bytes, global, 1 named locals
 * UpdateCurrentlyLoadedFont
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void UpdateCurrentlyLoadedFont(int language)
{
    (void)language;
    currentlyLoadedFont = LoadFont(currentFontStyle, 24);
}

/* 0xFEB20, 3 bytes, global, 0 named locals
 * UpdateMenus
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void UpdateMenus(void)
{
}

/* 0xFEB30, 62 bytes, global, 7 named locals
 * iDrawChar
 * PDB type: void (unsigned, float*, float, u...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void iDrawChar(
    unsigned tex,
    float *x,
    float y,
    unsigned letter,
    int color,
    int alpha,
    float scale)
{
    float absolute_scale = scale;

    (void)tex;
    (void)y;
    (void)color;
    (void)alpha;
    if (!(scale > 0.0f)) {
        absolute_scale = -scale;
    }
    *x +=
        (float)sfont18_nfont[(letter - 32u) * 6u] * absolute_scale;
}

/* 0xFEB70, 70 bytes, global, 8 named locals
 * iDrawIcon
 * PDB type: unsigned (float, float, unsigned...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
unsigned iDrawIcon(
    float x,
    float y,
    unsigned letter,
    unsigned color,
    float scale)
{
    float absolute_scale = scale;
    float x2;

    (void)y;
    (void)color;
    if (!(scale > 0.0f)) {
        absolute_scale = -scale;
    }
    x2 = x + (float)sfont18_nfont[(letter - 32u) * 6u] * absolute_scale;
    return (unsigned)(int64_t)(x2 - x);
}

/* 0xFEBC0, 299 bytes, global, 17 named locals
 * iDrawString
 * PDB type: unsigned (unsigned, int, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
unsigned iDrawString(
    unsigned tex,
    int mode,
    unsigned char *string,
    float x,
    float y,
    float scale,
    int red,
    int green,
    int blue,
    int alpha)
{
    unsigned char *cursor = string;
    float original_x = x;
    float length = 0.0f;

    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
    if (mode != 0) {
        while (*cursor != 0) {
            float character_scale = scale;

            if (!(scale > 0.0f)) {
                character_scale = -scale;
            }
            length +=
                (float)sfont18_nfont[(*cursor - 32u) * 6u] *
                character_scale;
            ++cursor;
        }
        --mode;
        if (mode == 0) {
            x -= length;
        } else if (mode == 1) {
            x -= length * 0.5f;
        }
    }
    cursor = string;
    while (*cursor != 0) {
        if (*cursor == '\n') {
            y += scale * 24.0f;
            x = original_x;
        } else {
            iDrawChar(tex, &x, y, *cursor, 0, 0, scale);
        }
        ++cursor;
    }
    return (unsigned)(int64_t)length;
}

/* 0xFECF0, 273 bytes, global, 13 named locals
 * itextWrite
 * PDB type: int (unsigned, float, int, int, ...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int itextWrite(
    unsigned tex,
    float scale,
    int tint,
    int mode,
    int x,
    int y,
    char *format,
    ...)
{
    char formatted[256];
    va_list arguments;

    (void)tint;
    va_start(arguments, format);
    (void)vsprintf(formatted, format, arguments);
    va_end(arguments);
    return (int)iDrawString(
        tex,
        mode,
        (unsigned char *)formatted,
        (float)x,
        (float)y,
        scale,
        0,
        0,
        0,
        0);
}

/* 0xFEE10, 881 bytes, global, 18 named locals
 * menuBox
 * PDB type: void (unsigned, unsigned, int, i...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
static void text_draw_menu_box_slices(
    unsigned type,
    unsigned mode,
    float base_x,
    float base_y,
    float middle_width,
    float middle_height,
    float corner_width,
    int red,
    int green,
    int blue)
{
    float middle_x =
        base_x + corner_width * gPSXDrawScaleW;
    float right_x =
        base_x + (middle_width + corner_width) * gPSXDrawScaleW;
    float middle_y =
        base_y + corner_width * gPSXDrawScaleH;
    float bottom_y =
        base_y + (middle_height + corner_width) * gPSXDrawScaleH;

    psxDrawTexture(type, base_x, base_y, 0.0f, 0.0f,
                   mode, red, green, blue);
    psxDrawTexture(type + 1u, middle_x, base_y, middle_width, 0.0f,
                   mode, red, green, blue);
    psxDrawTexture(type + 2u, right_x, base_y, 0.0f, 0.0f,
                   mode, red, green, blue);
    psxDrawTexture(type + 3u, base_x, middle_y, 0.0f, middle_height,
                   mode, red, green, blue);
    psxDrawTexture(
        type + 4u,
        middle_x,
        middle_y,
        middle_width,
        middle_height,
        mode,
        red,
        green,
        blue);
    psxDrawTexture(type + 5u, right_x, middle_y, 0.0f, middle_height,
                   mode, red, green, blue);
    psxDrawTexture(type + 6u, base_x, bottom_y, 0.0f, 0.0f,
                   mode, red, green, blue);
    psxDrawTexture(type + 7u, middle_x, bottom_y, middle_width, 0.0f,
                   mode, red, green, blue);
    psxDrawTexture(type + 8u, right_x, bottom_y, 0.0f, 0.0f,
                   mode, red, green, blue);
}

void menuBox(
    unsigned type,
    unsigned mode,
    int x,
    int y,
    int width,
    int height,
    int red,
    int green,
    int blue)
{
    float scale = scaleAdjustmentMM;
    float corner_width = scale * 8.0f;
    float base_x =
        (float)x * scale * gPSXDrawScaleW +
        (float)OptionStruct.ScreenWidth * 0.5f;
    float base_y =
        (float)y * scale * gPSXDrawScaleH +
        (float)OptionStruct.ScreenHeight * 0.5f;
    float middle_width =
        (float)width * scale - (corner_width + corner_width);
    float middle_height =
        (float)height * scale - (corner_width + corner_width);

    text_draw_menu_box_slices(
        type,
        mode,
        base_x,
        base_y,
        middle_width,
        middle_height,
        corner_width,
        red,
        green,
        blue);
}

/* 0xFF190, 983 bytes, global, 20 named locals
 * menuBoxMM
 * PDB type: void (unsigned, unsigned, int, i...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void menuBoxMM(
    unsigned type,
    unsigned mode,
    int x_offset,
    int y_offset,
    int width_border,
    int height_border,
    int red,
    int green,
    int blue)
{
    const float aspect = 1.7777778f;
    float screen_width = (float)OptionStruct.ScreenWidth;
    float screen_height = (float)OptionStruct.ScreenHeight;
    float viewport_x = 0.0f;
    float viewport_y = 0.0f;
    float mm_width;
    float mm_height;
    float scale = scaleAdjustmentMM;
    float corner_width = scale * 8.0f;
    float base_x;
    float base_y;
    float middle_width;
    float middle_height;

    if (screen_width / screen_height < aspect) {
        mm_width = screen_width;
        mm_height = screen_width / aspect;
        viewport_y = (screen_height - mm_height) * 0.5f;
    } else {
        mm_height = screen_height;
        mm_width = screen_height * aspect;
        viewport_x = (screen_width - mm_width) * 0.5f;
    }
    mm_width /= gPSXDrawScaleW;
    mm_height /= gPSXDrawScaleH;
    base_x =
        (float)(x_offset + width_border) * scale * gPSXDrawScaleW +
        viewport_x;
    base_y =
        (float)(y_offset + height_border) * scale * gPSXDrawScaleH +
        viewport_y;
    middle_width =
        mm_width - (float)width_border * scale * 2.0f -
        (corner_width + corner_width);
    middle_height =
        mm_height - (float)height_border * scale * 2.0f -
        (corner_width + corner_width);

    text_draw_menu_box_slices(
        type,
        mode,
        base_x,
        base_y,
        middle_width,
        middle_height,
        corner_width,
        red,
        green,
        blue);
}

/* 0xFF570, 71 bytes, global, 7 named locals
 * menuBoxTest
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void menuBoxTest(
    unsigned x,
    unsigned y,
    unsigned width,
    unsigned height,
    int red,
    int green,
    int blue)
{
    menuBox(157u, 200u, (int)x, (int)y, (int)width, (int)height,
            red, green, blue);
}

/* 0xFF5C0, 577 bytes, global, 8 named locals
 * menuDrawGroup
 * PDB type: void (MENU_ARTDEF*, unsigned, un...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void menuDrawGroup(MENU_ARTDEF *art_group, unsigned x, unsigned y)
{
    unsigned art_flag = 0;

    if (art_group == NULL) {
        return;
    }
    while (art_group->art != 0) {
        unsigned art = art_group->art;

        if (art == 0x800u) {
            art_flag = art_group->x;
        } else if (art == 0x9cu) {
            float horizontal_scale =
                (float)OptionStruct.ScreenWidth * 0.001953125f;
            float vertical_scale =
                (float)OptionStruct.ScreenHeight / 240.0f;
            uint16_t new_x =
                (uint16_t)((uint16_t)x + art_group->x);
            uint16_t new_y =
                (uint16_t)((uint16_t)y + art_group->y);
            SCREENRECT destination;
            CVECTOR black = {0, 0, 0, 255};

            frontZ = (float)((double)frontZ + 0.001);
            destination.left =
                (int32_t)((float)new_x * horizontal_scale);
            destination.top =
                (int32_t)((float)new_y * horizontal_scale);
            destination.right = destination.left +
                (int32_t)((float)art_group->sx * horizontal_scale);
            destination.bottom = destination.top +
                (int32_t)((float)art_group->sy * vertical_scale);
            _DrawTexture(
                whitemat, destination, NULL, black, frontZ);
        } else {
            psxDrawTexture(
                (art & 0x7ffu) | art_flag,
                (float)x +
                    (float)art_group->x * scaleAdjustmentMM,
                (float)y +
                    (float)art_group->y * scaleAdjustmentMM,
                (float)art_group->sx * scaleAdjustmentMM,
                (float)art_group->sy * scaleAdjustmentMM,
                255u,
                128,
                128,
                128);
        }
        ++art_group;
    }
}

/* 0xFF810, 610 bytes, global, 12 named locals
 * menuDrawGroupClip
 * PDB type: void (MENU_ARTDEF*, unsigned, un...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void menuDrawGroupClip(
    MENU_ARTDEF *art_group,
    unsigned x,
    unsigned y,
    unsigned clip_x,
    unsigned clip_y,
    unsigned clip_width,
    unsigned clip_height)
{
    unsigned art_flag = 0;
    unsigned clamped_y = y <= 100000u ? y : 0u;

    if (art_group == NULL) {
        return;
    }
    while (art_group->art != 0) {
        unsigned art = art_group->art;

        if (art == 0x800u) {
            art_flag = art_group->x;
        } else if (art == 0x9cu) {
            float horizontal_scale =
                (float)OptionStruct.ScreenWidth * 0.001953125f;
            float vertical_scale =
                (float)OptionStruct.ScreenHeight / 240.0f;
            uint16_t new_x =
                (uint16_t)((uint16_t)x + art_group->x);
            uint16_t new_y =
                (uint16_t)((uint16_t)clamped_y + art_group->y);
            SCREENRECT destination;
            CVECTOR black = {0, 0, 0, 255};

            frontZ = (float)((double)frontZ + 0.001);
            destination.left =
                (int32_t)((float)new_x * horizontal_scale);
            destination.top =
                (int32_t)((float)new_y * vertical_scale);
            destination.right = destination.left +
                (int32_t)((float)art_group->sx * horizontal_scale);
            destination.bottom = destination.top +
                (int32_t)((float)art_group->sy * vertical_scale);
            _DrawTexture(
                whitemat, destination, NULL, black, frontZ);
        } else if (clip_width != 0u && clip_height != 0u) {
            psxDrawTextureClip(
                (art & 0x7ffu) | art_flag,
                x + art_group->x,
                clamped_y + art_group->y,
                art_group->sx,
                art_group->sy,
                255u,
                128,
                128,
                128,
                clip_x,
                clip_y,
                clip_width,
                clip_height);
        }
        ++art_group;
    }
}

/* 0xFFA80, 692 bytes, global, 13 named locals
 * menuDrawGroupClipDepth
 * PDB type: void (MENU_ARTDEF*, unsigned, un...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void menuDrawGroupClipDepth(
    MENU_ARTDEF *art_group,
    unsigned x,
    unsigned y,
    unsigned clip_x,
    unsigned clip_y,
    unsigned clip_width,
    unsigned clip_height,
    float depth)
{
    unsigned art_flag = 0;
    unsigned clamped_y = y <= 100000u ? y : 0u;

    if (art_group == NULL) {
        return;
    }
    while (art_group->art != 0) {
        unsigned art = art_group->art;

        if (art == 0x800u) {
            art_flag = art_group->x;
        } else if (art == 0x9cu) {
            float horizontal_scale =
                (float)OptionStruct.ScreenWidth * 0.001953125f;
            float vertical_scale =
                (float)OptionStruct.ScreenHeight / 240.0f;
            uint16_t new_x =
                (uint16_t)((uint16_t)x + art_group->x);
            uint16_t new_y =
                (uint16_t)((uint16_t)clamped_y + art_group->y);
            SCREENRECT destination;
            CVECTOR black = {0, 0, 0, 255};

            destination.left =
                (int32_t)((float)new_x * horizontal_scale);
            destination.top =
                (int32_t)((float)new_y * vertical_scale);
            destination.right = destination.left +
                (int32_t)((float)art_group->sx * horizontal_scale);
            destination.bottom = destination.top +
                (int32_t)((float)art_group->sy * vertical_scale);
            _DrawTexture(
                whitemat, destination, NULL, black, depth);
        } else if (clip_width != 0u && clip_height != 0u) {
            psxDrawTextureClipDepth(
                (art & 0x7ffu) | art_flag,
                (unsigned)(int64_t)(
                    (float)x +
                    (float)art_group->x * scaleAdjustmentMM),
                (unsigned)(int64_t)(
                    (float)clamped_y +
                    (float)art_group->y * scaleAdjustmentMM),
                (int)((float)art_group->sx * scaleAdjustmentMM),
                (int)((float)art_group->sy * scaleAdjustmentMM),
                255u,
                128,
                128,
                128,
                clip_x,
                clip_y,
                clip_width,
                clip_height,
                depth);
        }
        ++art_group;
    }
}

/* 0xFFD40, 3 bytes, global, 5 named locals
 * menuDrawGroupScale
 * PDB type: void (MENU_ARTDEF*, unsigned, un...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFFD50, 115 bytes, global, 8 named locals
 * psxDrawChar
 * PDB type: int (int, int*, int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void menuDrawGroupScale(
    MENU_ARTDEF *art_group,
    unsigned x,
    unsigned y,
    unsigned width,
    unsigned height)
{
    (void)art_group;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}
int psxDrawChar(
    int character,
    int *x,
    int y,
    int red,
    int green,
    int blue,
    short scale,
    int mode)
{
    const FONTSPEC *spec = &fontSpec[character];
    int width;

    (void)scale;
    (void)mode;
    y += (int)(uint8_t)spec->xypage - (int)spec->w;
    width = winDrawTexture(
        (unsigned)character,
        (unsigned)*x,
        (unsigned)y,
        0,
        0,
        0,
        red,
        green,
        blue);
    *x += width + 2;
    return 1;
}

/* 0xFFDD0, 84 bytes, global, 10 named locals
 * psxDrawFlip
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFFE30, 687 bytes, global, 20 named locals
 * psxDrawTexture
 * PDB type: int (unsigned, float, float, flo...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int psxDrawFlip(
    unsigned texture,
    unsigned x,
    unsigned y,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue,
    char flip)
{
    red += frontRGBoff;
    green += frontRGBoff;
    blue += frontRGBoff;
    if (red > 255) red = 255;
    if (green > 255) green = 255;
    if (blue > 255) blue = 255;
    return winDrawFlip(
        texture,
        x,
        y,
        width,
        height,
        transparency,
        red,
        green,
        blue,
        flip);
}
int psxDrawTexture(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int red,
    int green,
    int blue)
{
    const FONTSPEC *spec = &fontSpec[texture & 0x7ffu];
    float draw_width = (float)spec->w * scaleAdjustmentMM;
    float draw_height = (float)spec->h * scaleAdjustmentMM;
    float source_left = (float)spec->x;
    float source_right = source_left + (float)spec->w;
    float source_top = (float)spec->y;
    float source_bottom = source_top + (float)spec->h;
    SCREENRECT destination;
    SCREENRECT source;
    CVECTOR color;

    red += frontRGBoff;
    green += frontRGBoff;
    blue += frontRGBoff;
    if (red > 255) red = 255;
    if (green > 255) green = 255;
    if (blue > 255) blue = 255;
    x *= gPSXDrawScaleX;
    y *= gPSXDrawScaleY;
    if ((texture & 0x8000u) != 0) {
        float swap = source_left;

        source_left = source_right;
        source_right = swap;
    }
    if ((texture & 0x2000u) != 0) {
        transparency = 200;
    }
    if (width > 0.0f) {
        draw_width = width;
    }
    if (height > 0.0f) {
        draw_height = height;
    }
    draw_width *= gPSXDrawScaleW;
    draw_height *= gPSXDrawScaleH;
    source_left *= 4.0f;
    source_right *= 4.0f;
    source_top *= 4.0f;
    source_bottom *= 4.0f;
    frontZ = (float)((double)frontZ + 0.001);
    if (transparency == 0x8100u) {
        transparency = 0x7fu;
    } else if (transparency == 0x8400u) {
        transparency = 0x3fu;
    } else if (transparency == 0x8000u || transparency > 0xffu) {
        transparency = 0xffu;
    }
    destination.left = (int32_t)x;
    destination.top = (int32_t)y;
    destination.right = (int32_t)(x + draw_width);
    destination.bottom = (int32_t)(y + draw_height);
    source.left = (int32_t)source_left;
    source.top = (int32_t)source_top;
    source.right = (int32_t)source_right;
    source.bottom = (int32_t)source_bottom;
    color.r = (uint8_t)red;
    color.g = (uint8_t)green;
    color.b = (uint8_t)blue;
    color.cd = (uint8_t)transparency;
    _DrawTexture(
        menuTextures[spec->clut],
        destination,
        &source,
        color,
        frontZ);
    return (int)draw_width;
}

/* 0x1000E0, 85 bytes, global, 7 named locals
 * psxDrawTexture2
 * PDB type: int (unsigned, float, float, flo...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int psxDrawTexture2(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int color_index)
{
    const CVECTOR *color = &Colors[color_index];

    return psxDrawTexture(
        texture,
        x,
        y,
        width,
        height,
        transparency,
        color->r,
        color->g,
        color->b);
}

/* 0x100140, 100 bytes, global, 8 named locals
 * psxDrawTexture2Depth
 * PDB type: int (unsigned, float, float, flo...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int psxDrawTexture2Depth(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int color_index,
    float depth)
{
    const CVECTOR *color = &Colors[color_index];

    return psxDrawTextureDepth(
        texture,
        x,
        y,
        width,
        height,
        transparency,
        color->r,
        color->g,
        color->b,
        depth);
}

static int text_psx_draw_texture_clip(
    unsigned texture,
    unsigned x,
    unsigned y,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue,
    unsigned clip_x,
    unsigned clip_y,
    unsigned clip_width,
    unsigned clip_height,
    int use_explicit_depth,
    float depth)
{
    const FONTSPEC *spec = &fontSpec[texture & 0x7ffu];
    int draw_width = spec->w;
    int draw_height = spec->h;
    float destination_left = (float)x * gPSXDrawScaleX;
    float destination_top = (float)y * gPSXDrawScaleY;
    float source_left = (float)spec->x;
    float source_right = source_left + (float)spec->w;
    float source_top = (float)spec->y;
    float source_bottom = source_top + (float)spec->h;
    float clip_left;
    float clip_top;
    float clip_right;
    float clip_bottom;
    float destination_right;
    float destination_bottom;
    SCREENRECT destination;
    SCREENRECT source;
    CVECTOR color;

    red += frontRGBoff;
    green += frontRGBoff;
    blue += frontRGBoff;
    if (red > 255) red = 255;
    if (green > 255) green = 255;
    if (blue > 255) blue = 255;
    if (draw_width <= 8 && width != 0) {
        --draw_width;
    }
    if (draw_height <= 8 && height != 0) {
        --draw_height;
    }
    if ((texture & 0x8000u) != 0) {
        float swap = source_left;

        source_left = source_right;
        source_right = swap;
    }
    if ((texture & 0x2000u) != 0) {
        transparency = 200;
    }
    if (width != 0) {
        draw_width = width;
    }
    if (height != 0) {
        draw_height = height;
    }

    clip_left = (float)clip_x * gPSXDrawScaleX;
    clip_top = (float)clip_y * gPSXDrawScaleY;
    clip_right = clip_left +
        (float)clip_width * gPSXDrawScaleW;
    clip_bottom = clip_top +
        (float)clip_height * gPSXDrawScaleH;
    destination_right = destination_left +
        (float)draw_width * gPSXDrawScaleW;
    destination_bottom = destination_top +
        (float)draw_height * gPSXDrawScaleH;
    source_left *= 4.0f;
    source_right *= 4.0f;
    source_top *= 4.0f;
    source_bottom *= 4.0f;
    if (!use_explicit_depth) {
        float surface_y =
            (float)((mDrawingSurfaceId ^ 1) * 240);

        clip_top += surface_y;
        clip_bottom += surface_y;
        frontZ = (float)((double)frontZ + 0.001);
        depth = frontZ;
    }
    if (transparency == 0x8100u) {
        transparency = 0x7fu;
    } else if (transparency == 0x8400u) {
        transparency = 0x3fu;
    } else if (transparency == 0x8000u || transparency > 0xffu) {
        transparency = 0xffu;
    }

    destination.left = (int32_t)(
        clip_left > destination_left ? clip_left : destination_left);
    destination.top = (int32_t)(
        clip_top > destination_top ? clip_top : destination_top);
    destination.right = (int32_t)(
        clip_right < destination_right ? clip_right : destination_right);
    destination.bottom = (int32_t)(
        clip_bottom < destination_bottom ? clip_bottom : destination_bottom);
    source.left = (int32_t)source_left;
    source.top = (int32_t)source_top;
    source.right = (int32_t)source_right;
    source.bottom = (int32_t)source_bottom;
    color.r = (uint8_t)red;
    color.g = (uint8_t)green;
    color.b = (uint8_t)blue;
    color.cd = (uint8_t)transparency;
    _DrawTexture(
        menuTextures[spec->clut],
        destination,
        &source,
        color,
        depth);
    return draw_width;
}

/* 0x1001B0, 899 bytes, global, 25 named locals
 * psxDrawTextureClip
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0x100540, 860 bytes, global, 25 named locals
 * psxDrawTextureClipDepth
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0x1008A0, 683 bytes, global, 21 named locals
 * psxDrawTextureDepth
 * PDB type: int (unsigned, float, float, flo...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int psxDrawTextureDepth(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int red,
    int green,
    int blue,
    float depth)
{
    const FONTSPEC *spec = &fontSpec[texture & 0x7ffu];
    float draw_width = (float)spec->w * scaleAdjustmentMM;
    float draw_height = (float)spec->h * scaleAdjustmentMM;
    float source_left = (float)spec->x;
    float source_right = source_left + (float)spec->w;
    float source_top = (float)spec->y;
    float source_bottom = source_top + (float)spec->h;
    SCREENRECT destination;
    SCREENRECT source;
    CVECTOR color;

    red += frontRGBoff;
    green += frontRGBoff;
    blue += frontRGBoff;
    if (red > 255) red = 255;
    if (green > 255) green = 255;
    if (blue > 255) blue = 255;
    x *= gPSXDrawScaleX;
    y *= gPSXDrawScaleY;
    if ((texture & 0x8000u) != 0) {
        float swap = source_left;

        source_left = source_right;
        source_right = swap;
    }
    if ((texture & 0x2000u) != 0) {
        transparency = 200;
    }
    if (width > 0.0f) {
        draw_width = width;
    }
    if (height > 0.0f) {
        draw_height = height;
    }
    draw_width *= gPSXDrawScaleW;
    draw_height *= gPSXDrawScaleH;
    source_left *= 4.0f;
    source_right *= 4.0f;
    source_top *= 4.0f;
    source_bottom *= 4.0f;
    frontZ = (float)((double)frontZ + 0.001);
    if (transparency == 0x8100u) {
        transparency = 0x7fu;
    } else if (transparency == 0x8400u) {
        transparency = 0x3fu;
    } else if (transparency == 0x8000u || transparency > 0xffu) {
        transparency = 0xffu;
    }
    destination.left = (int32_t)x;
    destination.top = (int32_t)y;
    destination.right = (int32_t)(x + draw_width);
    destination.bottom = (int32_t)(y + draw_height);
    source.left = (int32_t)source_left;
    source.top = (int32_t)source_top;
    source.right = (int32_t)source_right;
    source.bottom = (int32_t)source_bottom;
    color.r = (uint8_t)red;
    color.g = (uint8_t)green;
    color.b = (uint8_t)blue;
    color.cd = (uint8_t)transparency;
    _DrawTexture(
        menuTextures[spec->clut],
        destination,
        &source,
        color,
        depth);
    return (int)draw_width;
}

/* 0x100B50, 84 bytes, global, 11 named locals
 * psxDrawZFlip
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int psxDrawZFlip(
    unsigned texture,
    unsigned x,
    unsigned y,
    unsigned depth,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue,
    char flip)
{
    red += frontRGBoff;
    green += frontRGBoff;
    blue += frontRGBoff;
    if (red > 255) red = 255;
    if (green > 255) green = 255;
    if (blue > 255) blue = 255;
    return winDrawZFlip(
        texture,
        x,
        y,
        depth,
        width,
        height,
        transparency,
        red,
        green,
        blue,
        flip);
}

/* 0x100BB0, 75 bytes, global, 10 named locals
 * psxDrawZTexture
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0x100C00, 380 bytes, global, 14 named locals
 * psxStringOut
 * PDB type: int (unsigned char*, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int psxDrawZTexture(
    unsigned texture,
    unsigned x,
    unsigned y,
    unsigned depth,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue)
{
    red += frontRGBoff;
    green += frontRGBoff;
    blue += frontRGBoff;
    if (red > 255) red = 255;
    if (green > 255) green = 255;
    if (blue > 255) blue = 255;
    return winDrawZTexture(
        texture,
        x,
        y,
        depth,
        width,
        height,
        transparency,
        red,
        green,
        blue);
}
int psxStringOut(
    unsigned char *text,
    int x,
    int y,
    int width,
    int red,
    int green,
    int blue,
    short scale,
    int mode)
{
    int start_x = x;
    unsigned char character;

    if (red != 0) red += frontRGBoff;
    if (green != 0) green += frontRGBoff;
    if (blue != 0) blue += frontRGBoff;
    if (red > 255) red = 255;
    if (green > 255) green = 255;
    if (blue > 255) blue = 255;

    while ((character = *text++) != 0) {
        unsigned glyph;
        const FONTSPEC *spec;
        int drawn_width;
        int wrap_margin;

        if (character == '\n') {
            if ((mode & 0x40) == 0) {
                x = start_x;
                y += 26;
                continue;
            }
            character = ' ';
        }
        wrap_margin = ((int)scale * 20) >> 12;
        if (x >= width - wrap_margin && (mode & 0x20) != 0) {
            x -= width;
        }
        glyph = asciiRemap[character];
        spec = &fontSpec[glyph];
        drawn_width = winDrawTexture(
            glyph,
            (unsigned)x,
            (unsigned)(
                y + (int)(uint8_t)spec->xypage - (int)spec->w),
            0,
            0,
            0,
            red,
            green,
            blue);
        x += drawn_width + 2;
    }
    return 0;
}

/* 0x100D80, 3 bytes, global, 4 named locals
 * setAreaBox
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void setAreaBox(
    unsigned x, unsigned y, unsigned width, unsigned height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

/* 0x100D90, 3 bytes, global, 4 named locals
 * showRcount
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0x100DA0, 321 bytes, global, 9 named locals
 * textWrite
 * PDB type: int (int, int, int, int, char*, ...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int textWrite(
    int tint, int mode, int x, int y, char *format, ...)
{
    char formatted[256];
    va_list arguments;
    uint32_t packed_color;
    float depth;
    int width;
    int height;
    int draw;

    va_start(arguments, format);
    (void)vsprintf(formatted, format, arguments);
    va_end(arguments);
    GetStringSize(0.75f, &width, &height, formatted);
    if ((mode & 0x7f) == 1) {
        x -= width;
    } else if ((mode & 0x7f) == 2) {
        x -= width / 2;
    }
    memcpy(&packed_color, &Colors[tint], sizeof(packed_color));
    depth = (mode & 0x100) != 0 ? frontZ : 0.0001f;
    draw = _DrawText(
        (float)x,
        (float)y,
        depth,
        0.75f,
        packed_color,
        formatted);
    if ((mode & 0x100) != 0) {
        frontZ -= 1.0f;
    }
    return draw;
}

/* 0x100EF0, 342 bytes, global, 10 named locals
 * textWriteColor
 * PDB type: int (CVECTOR, int, int, int, flo...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int textWriteColor(
    CVECTOR color,
    int mode,
    int x,
    int y,
    float scale,
    char *format,
    ...)
{
    char formatted[256];
    va_list arguments;
    uint32_t packed_color;
    float depth;
    int width = 0;
    int height;
    int draw;

    va_start(arguments, format);
    (void)vsprintf(formatted, format, arguments);
    va_end(arguments);
    if ((mode & 0x7f) != 0) {
        GetStringSize(0.75f, &width, &height, formatted);
        if ((mode & 0x7f) == 2) {
            x -= width / 2;
        } else if ((mode & 0x7f) == 1) {
            x -= width;
        }
    }
    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    memcpy(&packed_color, &color, sizeof(packed_color));
    depth = (mode & 0x100) != 0 ? frontZ : 0.0001f;
    draw = _DrawText(
        (float)x,
        (float)y,
        depth,
        scale,
        packed_color,
        formatted);
    if ((mode & 0x100) != 0) {
        frontZ -= 1.0f;
    }
    return draw;
}

/* 0x101050, 286 bytes, global, 9 named locals
 * textWriteScale
 * PDB type: int (int, int, int, int, float, ...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int textWriteScale(
    int tint,
    int mode,
    int x,
    int y,
    float scale,
    char *format,
    ...)
{
    char formatted[256];
    va_list arguments;
    uint32_t packed_color;
    int width;
    int height;

    va_start(arguments, format);
    (void)vsprintf(formatted, format, arguments);
    va_end(arguments);
    GetStringSize(0.75f, &width, &height, formatted);
    if ((mode & 0x7f) == 1) {
        x -= width;
    } else if ((mode & 0x7f) == 2) {
        x -= width / 2;
    }
    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    memcpy(&packed_color, &Colors[tint], sizeof(packed_color));
    return _DrawText(
        (float)x,
        (float)y,
        0.0001f,
        scale,
        packed_color,
        formatted);
}

/* 0x101170, 182 bytes, global, 5 named locals
 * text_gGetLength
 * PDB type: int (FONT*, char*, <no type>)
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0x101230, 51 bytes, global, 1 named locals
 * text_gInitialise
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int text_gGetLength(FONT *font, char *format, ...)
{
    char formatted[256];
    va_list arguments;
    unsigned width = 0;
    unsigned index;

    (void)font;
    va_start(arguments, format);
    (void)vsprintf(formatted, format, arguments);
    va_end(arguments);
    for (index = 0; formatted[index] != '\0'; ++index) {
        int character = (int8_t)formatted[index];
        unsigned glyph = asciiRemap[character];

        width += (unsigned)fontSpec[glyph].w + 1u;
    }
    return (int)width;
}
void showRcount(
    unsigned count1, unsigned count2,
    unsigned count3, unsigned count4)
{
    (void)count1;
    (void)count2;
    (void)count3;
    (void)count4;
}
void text_gInitialise(int gamma)
{
    float clamped_gamma = frontGamma;
    int rgb_offset;

    (void)gamma;
    if (clamped_gamma > 1.0f) {
        clamped_gamma = 1.0f;
        frontGamma = clamped_gamma;
    }
    rgb_offset = (int)(clamped_gamma * 255.0f);
    if (rgb_offset < 0) {
        rgb_offset = 0;
    } else if (rgb_offset > 255) {
        rgb_offset = 255;
    }
    frontRGBoff = (uint8_t)rgb_offset;
}

/* 0x101270, 203 bytes, global, 7 named locals
 * winDrawBackground
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
void winDrawBackground(int texture_index)
{
    const float aspect = 16.0f / 9.0f;
    float screen_width = (float)OptionStruct.ScreenWidth;
    float screen_height = (float)OptionStruct.ScreenHeight;
    float left;
    float top;
    float width;
    float height;
    SCREENRECT destination;
    CVECTOR white = {255, 255, 255, 255};

    if (screen_height <= 0.0f ||
        texture_index < 0 || texture_index >= 249) {
        return;
    }
    if (screen_width / screen_height >= aspect) {
        width = screen_height * aspect;
        left = (screen_width - width) * 0.5f;
        top = 0.0f;
        height = screen_height;
    } else {
        left = 0.0f;
        top = (screen_height - screen_width / aspect) * 0.5f;
        width = screen_width;
        height = screen_width / aspect;
    }
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)(left + width);
    destination.bottom = (int32_t)(top + height);
    _DrawTexture(
        menuTextures[texture_index],
        destination,
        NULL,
        white,
        1.0f);
}

/* 0x101340, 912 bytes, global, 14 named locals
 * winDrawFlip
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0x1016D0, 641 bytes, global, 16 named locals
 * winDrawTexture
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
static void text_set_frontend_vertex(
    FRONTENDVERT *vertex,
    float x,
    float y,
    float u,
    float v,
    int red,
    int green,
    int blue)
{
    vertex->x = x;
    vertex->y = y;
    vertex->u = u;
    vertex->v = v;
    vertex->r = red;
    vertex->g = green;
    vertex->b = blue;
    /* Retail leaves FRONTENDVERT::color untouched in these builders. */
}

static int text_win_draw_frontend_quad(
    unsigned texture,
    unsigned x,
    unsigned y,
    unsigned depth,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue,
    char flip,
    int use_flip_character,
    int advance_front_z)
{
    const FONTSPEC *spec = &fontSpec[texture & 0x7ffu];
    uint32_t draw_width = spec->w;
    uint32_t draw_height = spec->h;
    float source_left = (float)spec->x;
    float source_top = (float)spec->y;
    float source_right = source_left + (float)spec->w;
    float source_bottom = source_top + (float)spec->h;
    _Material *material = menuTextures[spec->clut];
    FRONTENDVERT vertices[4];
    float destination_left;
    float destination_top;
    float destination_right;
    float destination_bottom;
    float draw_depth;

    (void)transparency;
    if (draw_width <= 8u && width != 0) {
        --draw_width;
    }
    if (draw_height <= 8u && height != 0) {
        --draw_height;
    }
    if (use_flip_character) {
        if (flip == 'H' || flip == 'h') {
            float swap = source_left;

            source_left = source_right;
            source_right = swap;
        } else if (flip == 'V' || flip == 'v') {
            float swap = source_top;

            source_top = source_bottom;
            source_bottom = swap;
        }
    } else if ((texture & 0x8000u) != 0) {
        float swap = source_left;

        source_left = source_right;
        source_right = swap;
    }
    if (width != 0) {
        draw_width = (uint32_t)width;
    }
    if (height != 0) {
        draw_height = (uint32_t)height;
    }

    destination_left = (float)(uint64_t)x;
    destination_top = (float)(uint64_t)y;
    destination_right = (float)(uint64_t)(x + draw_width);
    destination_bottom = (float)(uint64_t)(y + draw_height);
    source_left /= (float)material->iw;
    source_right /= (float)material->iw;
    source_top /= (float)material->ih;
    source_bottom /= (float)material->ih;

    text_set_frontend_vertex(
        &vertices[0], destination_left, destination_top,
        source_left, source_top, red, green, blue);
    text_set_frontend_vertex(
        &vertices[1], destination_right, destination_top,
        source_right, source_top, red, green, blue);
    text_set_frontend_vertex(
        &vertices[2], destination_left, destination_bottom,
        source_left, source_bottom, red, green, blue);
    text_set_frontend_vertex(
        &vertices[3], destination_right, destination_bottom,
        source_right, source_bottom, red, green, blue);

    draw_depth = advance_front_z ? frontZ : (float)(uint64_t)depth;
    frontEndPoly(material, 4, vertices, draw_depth);
    if (advance_front_z) {
        frontZ = (float)((double)frontZ + 0.001);
    }
    return (int)draw_width;
}

int winDrawFlip(
    unsigned texture,
    unsigned x,
    unsigned y,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue,
    char flip)
{
    return text_win_draw_frontend_quad(
        texture,
        x,
        y,
        0,
        width,
        height,
        transparency,
        red,
        green,
        blue,
        flip,
        1,
        1);
}
int psxDrawTextureClip(
    unsigned texture,
    unsigned x,
    unsigned y,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue,
    unsigned clip_x,
    unsigned clip_y,
    unsigned clip_width,
    unsigned clip_height)
{
    return text_psx_draw_texture_clip(
        texture,
        x,
        y,
        width,
        height,
        transparency,
        red,
        green,
        blue,
        clip_x,
        clip_y,
        clip_width,
        clip_height,
        0,
        0.0f);
}
static int32_t text_unsigned_screen_coordinate(uint32_t value)
{
    volatile float converted = (float)(int64_t)(uint64_t)value;

    return (int32_t)converted;
}

int winDrawTexture(
    unsigned texture,
    unsigned x,
    unsigned y,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue)
{
    const FONTSPEC *spec = &fontSpec[texture & 0x7ffu];
    uint32_t draw_width = spec->w;
    uint32_t draw_height = spec->h;
    float source_left = (float)spec->x;
    float source_right = source_left + (float)spec->w;
    SCREENRECT destination;
    SCREENRECT source;
    CVECTOR color;

    if (draw_width < 9u && width != 0) {
        --draw_width;
    }
    if (draw_height < 9u && height != 0) {
        --draw_height;
    }
    if ((texture & 0x8000u) != 0) {
        float swap = source_left;

        source_left = source_right;
        source_right = swap;
    }
    if ((texture & 0x2000u) != 0) {
        transparency = 200;
    }
    if (width != 0) {
        draw_width = (uint32_t)(int64_t)((float)width * scaleAdjustmentMM);
    }
    if (height != 0) {
        draw_height = (uint32_t)(int64_t)((float)height * scaleAdjustmentMM);
    }

    frontZ = (float)((double)frontZ + 0.001);
    if (transparency == 0x8100u) {
        transparency = 0x7fu;
    } else if (transparency == 0x8400u) {
        transparency = 0x3fu;
    } else if (transparency == 0x8000u || transparency > 0xffu) {
        transparency = 0xffu;
    }

    destination.left = text_unsigned_screen_coordinate(x);
    destination.top = text_unsigned_screen_coordinate(y);
    destination.right = text_unsigned_screen_coordinate(x + draw_width);
    destination.bottom = text_unsigned_screen_coordinate(y + draw_height);
    source.left = (int32_t)source_left;
    source.top = spec->y;
    source.right = (int32_t)source_right;
    source.bottom = spec->y + spec->h;
    color.r = (uint8_t)red;
    color.g = (uint8_t)green;
    color.b = (uint8_t)blue;
    color.cd = (uint8_t)transparency;
    _DrawTexture(
        menuTextures[spec->clut],
        destination,
        &source,
        color,
        frontZ);
    return (int)draw_width;
}

/* 0x101960, 892 bytes, global, 15 named locals
 * winDrawZFlip
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int winDrawZFlip(
    unsigned texture,
    unsigned x,
    unsigned y,
    unsigned depth,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue,
    char flip)
{
    return text_win_draw_frontend_quad(
        texture,
        x,
        y,
        depth,
        width,
        height,
        transparency,
        red,
        green,
        blue,
        flip,
        1,
        0);
}
int psxDrawTextureClipDepth(
    unsigned texture,
    unsigned x,
    unsigned y,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue,
    unsigned clip_x,
    unsigned clip_y,
    unsigned clip_width,
    unsigned clip_height,
    float depth)
{
    return text_psx_draw_texture_clip(
        texture,
        x,
        y,
        width,
        height,
        transparency,
        red,
        green,
        blue,
        clip_x,
        clip_y,
        clip_width,
        clip_height,
        1,
        depth);
}

/* 0x101CE0, 719 bytes, global, 14 named locals
 * winDrawZTexture
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
int winDrawZTexture(
    unsigned texture,
    unsigned x,
    unsigned y,
    unsigned depth,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue)
{
    return text_win_draw_frontend_quad(
        texture,
        x,
        y,
        depth,
        width,
        height,
        transparency,
        red,
        green,
        blue,
        0,
        0,
        0);
}
