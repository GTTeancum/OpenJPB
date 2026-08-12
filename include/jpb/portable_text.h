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
 * Dependency-light realization of the matched SDL_ttf/FontAtlas boundary.
 * It consumes the shipped fonts through the recovered resource owner and
 * rasterizes directly into the caller-owned software framebuffer.
 */
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
    JPBPortableTextMetrics *metrics);

void jpb_PortableTextShutdown(void);

#ifdef __cplusplus
}
#endif

#endif
