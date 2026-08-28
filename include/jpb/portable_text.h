#ifndef JPB_PORTABLE_TEXT_H
#define JPB_PORTABLE_TEXT_H

#include "jpb/software_renderer.h"

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JPBPortableTextMetrics {
    int pointSize;
    int width;
    int height;
    size_t blendedPixels;
    int usedTrueType;
} JPBPortableTextMetrics;

enum { JPB_PORTABLE_TEXT_CONTROL_GLYPH_CAPACITY = 16 };

typedef struct JPBPortableTextControlGlyph {
    int iconIndex;
    int alpha;
    int x;
    int y;
} JPBPortableTextControlGlyph;

typedef void (*JPBPortableText3DGlyphHook)(
    void *user_data,
    _Material *material,
    const JPBScreenPolyVertex *vertices);

/*
 * Exact matched-PC font selection performed by getFontFile. Language 6 uses
 * the three Simplified-Chinese faces; the remaining languages use the shared
 * regular/bold faces and the compact Latin italic face.
 */
const char *jpb_PortableTextFontFileName(
    int font_style, int language);

/* Exact SDLTextWriteScale point-size calculation. */
int jpb_PortableTextPointSize(
    float scale, float scale_adjustment);

/* Exact 17-entry text tint table, returned as CVECTOR byte order. */
uint32_t jpb_PortableTextTint(int tint);

/*
 * Recovered DrawUITextUTF16 control-marker pass. The aligned origin is
 * measured before the marker pass mutates the string, matching the retail
 * renderer's SizeText -> DrawUITextUTF16 call order.
 */
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
    size_t glyph_capacity);

/* Install the SDL_ttf font and metric owners used by the reconstructed API. */
void jpb_PortableTextInstallHooks(void);

/*
 * SDL_ttf realization of the matched font boundary. A missing SDL_ttf/font
 * resource is a hard draw failure; there is no substitute glyph renderer.
 */
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
    JPBPortableTextMetrics *metrics);

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
    JPBPortableTextMetrics *metrics);

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
    void *user_data);

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
    void *user_data);

void jpb_PortableTextShutdown(void);

#ifdef __cplusplus
}
#endif

#endif
