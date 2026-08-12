#ifndef JPB_TEXT_H
#define JPB_TEXT_H

#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TTF_Font _TTF_Font;

/*
 * Exact matched-PC PDB type FONTSPEC (0x78FB). The executable keeps 460
 * records at RVA 0x944780; menu_winLoadTextures publishes each menu
 * material's dimensions and material index through this original table.
 */
typedef struct FONTSPEC {
    uint16_t xypage;
    uint16_t clut;
    uint16_t y;
    uint16_t x;
    uint16_t h;
    uint16_t w;
} FONTSPEC;

enum { JPB_FONT_SPEC_COUNT = 460 };

extern FONTSPEC fontSpec[JPB_FONT_SPEC_COUNT];
extern float gPSXDrawScaleX;
extern float gPSXDrawScaleY;
extern float gPSXDrawScaleW;
extern float gPSXDrawScaleH;
extern float frontGamma;
extern uint8_t frontRGBoff;
extern int player2IconOverride;

typedef _TTF_Font *(*JPBTextFontLoadHook)(
    void *user_data, const char *path, int point_size);

typedef int (*JPBTextDrawHook)(
    void *user_data,
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    float scale_adjustment,
    int font_style,
    const wchar_t *text);

typedef int (*JPBPsxTextureDrawHook)(
    void *user_data,
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int red,
    int green,
    int blue);

void jpb_TextSetDrawHook(
    JPBTextDrawHook hook, void *user_data);
void jpb_TextSetPsxTextureHook(
    JPBPsxTextureDrawHook hook, void *user_data);
void jpb_TextSetFontLoadHook(
    JPBTextFontLoadHook hook, void *user_data);
void jpb_TextSetClipRect(
    int left, int top, int right, int bottom);
void jpb_TextClearClipRect(void);
int jpb_TextGetClipRect(
    int *left, int *top, int *right, int *bottom);
void jpb_TextResetFontCache(void);
void text_gInitialise(int gamma);

_TTF_Font *LoadFont(int fontStyle, int pointSize);
void UpdateCurrentlyLoadedFont(int language);

int SDLTextWriteScale(
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    wchar_t *format,
    ...);

int SDLTextWriteScaleMM(
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    wchar_t *format,
    ...);

int psxDrawTexture(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int red,
    int green,
    int blue);
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
    float layer_depth);
int jpb_PsxDrawTextureColorIndexLayer(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int color_index,
    float layer_depth);
void winDrawBackground(int texture_index);

#ifdef __cplusplus
}
#endif

#endif
