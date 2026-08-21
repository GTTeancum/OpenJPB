/*
 * GENERATED RECONSTRUCTION SHELL - no function bodies recovered here.
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
#include "jpb/game.h"
#include "jpb/menu.h"
#include "jpb/sprite.h"
#include "jpb/whook.h"

#include <stdarg.h>
#include <wchar.h>

static JPBTextDrawHook jpb_text_draw_hook;
static void *jpb_text_draw_user_data;
static JPBPsxTextureDrawHook jpb_psx_texture_draw_hook;
static void *jpb_psx_texture_draw_user_data;
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
static const float jpb_menu_text_dpi_scale = 1.0f;

/* Exact PDB global spanning RVAs 0x944780..0x945D0F. */
FONTSPEC fontSpec[JPB_FONT_SPEC_COUNT];
float gPSXDrawScaleX = 1.0f;
float gPSXDrawScaleY = 1.0f;
float gPSXDrawScaleW = 3.75f;
float gPSXDrawScaleH = 4.5f;
/* Exact PDB global at matched-PC RVA 0x4CCC98. */
float frontGamma = 1.0f;
/* Exact PDB global at matched-PC RVA 0x94477C. */
uint8_t frontRGBoff;
int player2IconOverride;

static const CVECTOR jpb_psx_colors[] = {
    {0xe0, 0x60, 0x40, 0xff},
    {0x80, 0x00, 0x00, 0xff},
    {0x80, 0xa0, 0x80, 0xff},
    {0x00, 0x80, 0x00, 0xff},
    {0x80, 0x80, 0xe0, 0xff},
    {0x00, 0x00, 0x80, 0xff},
    {0xe0, 0xe0, 0x80, 0xff},
    {0xd0, 0x80, 0xe0, 0xff},
    {0x00, 0xe0, 0xe0, 0xff},
    {0x30, 0x30, 0x30, 0xff},
    {0x60, 0x60, 0x60, 0xff},
    {0xf0, 0xf0, 0xf0, 0xff},
    {0xbf, 0x61, 0x3f, 0xff},
    {0x30, 0xf0, 0xf0, 0xff},
    {0xa5, 0xf5, 0xa5, 0xff},
    {0xc0, 0xc0, 0xc0, 0xff},
    {0x93, 0x90, 0xe0, 0xff},
};

_Static_assert(
    sizeof(FONTSPEC) == 12,
    "FONTSPEC must match matched-PC PDB type 0x78FB");

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

void jpb_TextSetPsxTextureHook(
    JPBPsxTextureDrawHook hook, void *user_data)
{
    jpb_psx_texture_draw_hook = hook;
    jpb_psx_texture_draw_user_data = user_data;
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

/* 0xFDDC0, 371 bytes, global, 12 named locals
 * SDLTextWrite
 * PDB type: int (int, int, int, int, int, wc...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFDF40, 430 bytes, global, 15 named locals
 * SDLTextWriteScale
 * PDB type: int (int, int, int, int, int, fl...
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
int SDLTextWriteScale(
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    wchar_t *format,
    ...)
{
    wchar_t formatted_string[256];
    va_list arguments;
    int result;

    if (format == NULL) {
        return 0;
    }
    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    va_start(arguments, format);
    result = vswprintf(
        formatted_string,
        sizeof(formatted_string) / sizeof(formatted_string[0]),
        format,
        arguments);
    va_end(arguments);
    if (result < 0) {
        formatted_string[
            sizeof(formatted_string) /
                sizeof(formatted_string[0]) - 1] = L'\0';
    }
    if (jpb_text_draw_hook != NULL) {
        return jpb_text_draw_hook(
            jpb_text_draw_user_data,
            tint,
            alpha,
            mode,
            x,
            y,
            scale,
            scaleAdjustment,
            font_style,
            formatted_string);
    }
    return result < 0 ? 0 : result;
}

/*
 * 0xFE450, 430 bytes. The matched MM variant differs from the ordinary
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
    wchar_t *format,
    ...)
{
    wchar_t formatted_string[256];
    va_list arguments;
    int result;

    if (format == NULL) {
        return 0;
    }
    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    va_start(arguments, format);
    result = vswprintf(
        formatted_string,
        sizeof(formatted_string) / sizeof(formatted_string[0]),
        format,
        arguments);
    va_end(arguments);
    if (result < 0) {
        formatted_string[
            sizeof(formatted_string) /
                sizeof(formatted_string[0]) - 1] = L'\0';
    }
    if (jpb_text_draw_hook != NULL) {
        return jpb_text_draw_hook(
            jpb_text_draw_user_data,
            tint,
            alpha,
            mode,
            x,
            y,
            scale,
            scaleAdjustmentMM * jpb_menu_text_dpi_scale,
            font_style,
            formatted_string);
    }
    return result < 0 ? 0 : result;
}

int SDLTextWriteScaleMMDepth(
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    float depth,
    wchar_t *format,
    ...)
{
    wchar_t formatted_string[256];
    va_list arguments;
    int result;

    (void)depth;
    if (format == NULL) {
        return 0;
    }
    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    va_start(arguments, format);
    result = vswprintf(
        formatted_string,
        sizeof(formatted_string) / sizeof(formatted_string[0]),
        format,
        arguments);
    va_end(arguments);
    if (result < 0) {
        formatted_string[
            sizeof(formatted_string) /
                sizeof(formatted_string[0]) - 1] = L'\0';
    }
    if (jpb_text_draw_hook != NULL) {
        return jpb_text_draw_hook(
            jpb_text_draw_user_data,
            tint,
            alpha,
            mode,
            x,
            y,
            scale,
            scaleAdjustmentMM * jpb_menu_text_dpi_scale,
            font_style,
            formatted_string);
    }
    return result < 0 ? 0 : result;
}

/* 0xFE0F0, 401 bytes, global, 13 named locals
 * SDLTextWriteScale3D
 * PDB type: int (unsigned long, int, float, ...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFE290, 445 bytes, global, 16 named locals
 * SDLTextWriteScaleDepth
 * PDB type: int (int, int, int, int, int, fl...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFE450, 430 bytes, global, 15 named locals
 * SDLTextWriteScaleMM
 * PDB type: int (int, int, int, int, int, fl...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFE600, 445 bytes, global, 16 named locals
 * SDLTextWriteScaleMMDepth
 * PDB type: int (int, int, int, int, int, fl...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFE7C0, 177 bytes, global, 10 named locals
 * Text_gWrite
 * PDB type: int (short, char, int, int, int,...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFE880, 638 bytes, global, 23 named locals
 * Text_gWriteSub
 * PDB type: int (int, char, int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

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

/* 0xFEB30, 62 bytes, global, 7 named locals
 * iDrawChar
 * PDB type: void (unsigned, float*, float, u...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFEB70, 70 bytes, global, 8 named locals
 * iDrawIcon
 * PDB type: unsigned (float, float, unsigned...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFEBC0, 299 bytes, global, 17 named locals
 * iDrawString
 * PDB type: unsigned (unsigned, int, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFECF0, 273 bytes, global, 13 named locals
 * itextWrite
 * PDB type: int (unsigned, float, int, int, ...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFEE10, 881 bytes, global, 18 named locals
 * menuBox
 * PDB type: void (unsigned, unsigned, int, i...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFF190, 983 bytes, global, 20 named locals
 * menuBoxMM
 * PDB type: void (unsigned, unsigned, int, i...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFF570, 71 bytes, global, 7 named locals
 * menuBoxTest
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFF5C0, 577 bytes, global, 8 named locals
 * menuDrawGroup
 * PDB type: void (MENU_ARTDEF*, unsigned, un...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFF810, 610 bytes, global, 12 named locals
 * menuDrawGroupClip
 * PDB type: void (MENU_ARTDEF*, unsigned, un...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0xFFA80, 692 bytes, global, 13 named locals
 * menuDrawGroupClipDepth
 * PDB type: void (MENU_ARTDEF*, unsigned, un...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

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
int jpb_PsxDrawTextureLayer(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int red,
    int green,
    int blue,
    float layer_depth)
{
    if (texture < JPB_FONT_SPEC_COUNT) {
        const FONTSPEC *spec = &fontSpec[texture];

        if (spec->clut < 249u &&
            spec->w != 0u &&
            spec->h != 0u &&
            menuTextures[spec->clut] != NULL) {
            SCREENRECT source;
            SCREENRECT destination;
            CVECTOR color;
            float draw_width = width > 0.0f
                ? width
                : (float)spec->w * scaleAdjustmentMM;
            float draw_height = height > 0.0f
                ? height
                : (float)spec->h * scaleAdjustmentMM;

            x *= gPSXDrawScaleX;
            y *= gPSXDrawScaleY;
            draw_width *= gPSXDrawScaleW;
            draw_height *= gPSXDrawScaleH;
            if (draw_width < 1.0f) {
                draw_width = 1.0f;
            }
            if (draw_height < 1.0f) {
                draw_height = 1.0f;
            }
            source.left = (int32_t)spec->x * 4;
            source.top = (int32_t)spec->y * 4;
            source.right = (int32_t)(spec->x + spec->w) * 4;
            source.bottom = (int32_t)(spec->y + spec->h) * 4;
            destination.left = (int32_t)x;
            destination.top = (int32_t)y;
            destination.right = (int32_t)(x + draw_width);
            destination.bottom = (int32_t)(y + draw_height);
            {
                int rgb_offset = (int)frontRGBoff;
                int draw_red = red + rgb_offset;
                int draw_green = green + rgb_offset;
                int draw_blue = blue + rgb_offset;

                if (draw_red > 255) {
                    draw_red = 255;
                }
                if (draw_green > 255) {
                    draw_green = 255;
                }
                if (draw_blue > 255) {
                    draw_blue = 255;
                }
                color.r = (uint8_t)draw_red;
                color.g = (uint8_t)draw_green;
                color.b = (uint8_t)draw_blue;
            }
            if (transparency == 0x8100u) {
                color.cd = 0x7f;
            } else if (transparency == 0x8400u) {
                color.cd = 0x3f;
            } else {
                color.cd = 0xff;
            }
            _DrawTexture(
                menuTextures[spec->clut],
                destination,
                &source,
                color,
                layer_depth);
            return (int)draw_width;
        }
    }
    if (jpb_psx_texture_draw_hook != NULL) {
        return jpb_psx_texture_draw_hook(
            jpb_psx_texture_draw_user_data,
            texture,
            x,
            y,
            width,
            height,
            transparency,
            red,
            green,
            blue);
    }
    return width > 0.0f ? (int)width : 0;
}

int jpb_PsxDrawTextureColorIndexLayer(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int color_index,
    float layer_depth)
{
    const CVECTOR *color = &jpb_psx_colors[0];

    if (color_index >= 0 &&
        (unsigned)color_index <
            sizeof(jpb_psx_colors) / sizeof(jpb_psx_colors[0])) {
        color = &jpb_psx_colors[color_index];
    }
    return jpb_PsxDrawTextureLayer(
        texture,
        x,
        y,
        width,
        height,
        transparency,
        color->r,
        color->g,
        color->b,
        layer_depth);
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
    float layer = 0.2f;

    if (texture >= UINT32_C(0x115) &&
        texture <= UINT32_C(0x117)) {
        layer = 0.62222224f;
    }
    return jpb_PsxDrawTextureLayer(
        texture,
        x,
        y,
        width,
        height,
        transparency,
        red,
        green,
        blue,
        layer);
}

/* 0x1000E0, 85 bytes, global, 7 named locals
 * psxDrawTexture2
 * PDB type: int (unsigned, float, float, flo...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0x100140, 100 bytes, global, 8 named locals
 * psxDrawTexture2Depth
 * PDB type: int (unsigned, float, float, flo...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

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

/* 0x100B50, 84 bytes, global, 11 named locals
 * psxDrawZFlip
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

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

/* 0x100D80, 3 bytes, global, 4 named locals
 * setAreaBox
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

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

/* 0x100EF0, 342 bytes, global, 10 named locals
 * textWriteColor
 * PDB type: int (CVECTOR, int, int, int, flo...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0x101050, 286 bytes, global, 9 named locals
 * textWriteScale
 * PDB type: int (int, int, int, int, float, ...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

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

/* 0x101960, 892 bytes, global, 15 named locals
 * winDrawZFlip
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */

/* 0x101CE0, 719 bytes, global, 14 named locals
 * winDrawZTexture
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\Work\text.c
 */
