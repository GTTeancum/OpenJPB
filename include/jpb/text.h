#ifndef JPB_TEXT_H
#define JPB_TEXT_H

#include "jpb/fmath.h"

#include <stdint.h>

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

/* Exact matched-PC PDB type FONT (0x78F8). */
typedef struct FONT {
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t h;
    int8_t xoff;
    int8_t yoff;
} FONT;

/* Exact matched-PC PDB type MENU_ARTDEF (0x7CE5). */
typedef struct MENU_ARTDEF {
    uint16_t art;
    uint16_t x;
    uint16_t y;
    uint16_t sx;
    uint16_t sy;
} MENU_ARTDEF;

#if defined(__cplusplus)
static_assert(sizeof(MENU_ARTDEF) == 10, "MENU_ARTDEF PDB layout changed");
#else
_Static_assert(sizeof(MENU_ARTDEF) == 10, "MENU_ARTDEF PDB layout changed");
#endif

enum {
    JPB_FONT_SPEC_COUNT = 460,
    JPB_ASCII_REMAP_COUNT = 256,
    JPB_SMALL_FONT_COUNT = 102,
    JPB_TEXT_COLOR_COUNT = 17
};

extern FONTSPEC fontSpec[JPB_FONT_SPEC_COUNT];
extern uint8_t asciiRemap[JPB_ASCII_REMAP_COUNT];
extern uint8_t sfont18_nfont[608];
extern FONT SmallFont[JPB_SMALL_FONT_COUNT];
extern uint32_t MonospaceWidth;
extern CVECTOR Colors[JPB_TEXT_COLOR_COUNT];
extern float gPSXDrawScaleX;
extern float gPSXDrawScaleY;
extern float gPSXDrawScaleW;
extern float gPSXDrawScaleH;
extern float frontGamma;
extern float frontZ;
extern uint8_t frontRGBoff;
extern int comboIconOverride;
extern int player2IconOverride;

typedef _TTF_Font *(*JPBTextFontLoadHook)(
    void *user_data, const char *path, int point_size);

typedef void (*JPBTextDrawHook)(
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
    const uint16_t *text);

typedef void (*JPBTextDraw3DHook)(
    void *user_data,
    uint32_t color,
    int mode,
    float x,
    float y,
    float z,
    float scale,
    int font_style,
    const uint16_t *text);

void jpb_TextSetDrawHook(
    JPBTextDrawHook hook, void *user_data);
void jpb_TextSetDraw3DHook(
    JPBTextDraw3DHook hook, void *user_data);
void jpb_TextSetFontLoadHook(
    JPBTextFontLoadHook hook, void *user_data);
void jpb_TextSetClipRect(
    int left, int top, int right, int bottom);
void jpb_TextClearClipRect(void);
int jpb_TextGetClipRect(
    int *left, int *top, int *right, int *bottom);
void jpb_TextResetFontCache(void);
void text_gInitialise(int gamma);
void UpdateMenus(void);
void iDrawChar(
    unsigned tex,
    float *x,
    float y,
    unsigned letter,
    int color,
    int alpha,
    float scale);
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
    int alpha);
int itextWrite(
    unsigned tex,
    float scale,
    int tint,
    int mode,
    int x,
    int y,
    char *format,
    ...);
void menuBox(
    unsigned type,
    unsigned mode,
    int x,
    int y,
    int width,
    int height,
    int red,
    int green,
    int blue);
void menuBoxMM(
    unsigned type,
    unsigned mode,
    int x_offset,
    int y_offset,
    int width_border,
    int height_border,
    int red,
    int green,
    int blue);
void menuBoxTest(
    unsigned x,
    unsigned y,
    unsigned width,
    unsigned height,
    int red,
    int green,
    int blue);
void menuDrawGroup(MENU_ARTDEF *art_group, unsigned x, unsigned y);
void menuDrawGroupClip(
    MENU_ARTDEF *art_group,
    unsigned x,
    unsigned y,
    unsigned clip_x,
    unsigned clip_y,
    unsigned clip_width,
    unsigned clip_height);
void menuDrawGroupClipDepth(
    MENU_ARTDEF *art_group,
    unsigned x,
    unsigned y,
    unsigned clip_x,
    unsigned clip_y,
    unsigned clip_width,
    unsigned clip_height,
    float depth);
void menuDrawGroupScale(
    MENU_ARTDEF *art_group,
    unsigned x,
    unsigned y,
    unsigned width,
    unsigned height);

int Text_gWrite(
    int16_t scale,
    int8_t brightness,
    int color,
    int flags,
    int x,
    int y,
    FONT *font,
    char *format,
    ...);
int Text_gWriteSub(
    int scale,
    int8_t brightness,
    int color,
    int flags,
    int x,
    int y,
    uint8_t *text);

_TTF_Font *LoadFont(int fontStyle, int pointSize);
void UpdateCurrentlyLoadedFont(int language);
unsigned iDrawIcon(
    float x,
    float y,
    unsigned letter,
    unsigned color,
    float scale);

int SDLTextWriteScale(
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    char *format,
    ...);

int SDLTextWrite(
    int tint,
    int mode,
    int x,
    int y,
    int italic,
    char *format,
    ...);

int SDLTextWriteScale3D(
    uint32_t color,
    int mode,
    float x,
    float y,
    float z,
    float scale,
    int font_style,
    char *format,
    ...);

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
    ...);

int SDLTextWriteScaleMM(
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    int font_style,
    char *format,
    ...);

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
int psxDrawChar(
    int character,
    int *x,
    int y,
    int red,
    int green,
    int blue,
    short scale,
    int mode);
int psxStringOut(
    unsigned char *text,
    int x,
    int y,
    int width,
    int red,
    int green,
    int blue,
    short scale,
    int mode);
void setAreaBox(
    unsigned x, unsigned y, unsigned width, unsigned height);
void showRcount(
    unsigned count1, unsigned count2,
    unsigned count3, unsigned count4);
int textWrite(
    int tint, int mode, int x, int y, char *format, ...);
int textWriteColor(
    CVECTOR color, int mode, int x, int y,
    float scale, char *format, ...);
int textWriteScale(
    int tint, int mode, int x, int y,
    float scale, char *format, ...);
int text_gGetLength(FONT *font, char *format, ...);
int psxDrawTexture2(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int color_index);
int psxDrawTexture2Depth(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int color_index,
    float depth);
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
    unsigned clip_height);
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
    float depth);
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
    float depth);
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
    char flip);
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
    char flip);
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
    int blue);
int winDrawTexture(
    unsigned texture,
    unsigned x,
    unsigned y,
    int width,
    int height,
    unsigned transparency,
    int red,
    int green,
    int blue);
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
    char flip);
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
    char flip);
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
    int blue);
void winDrawBackground(int texture_index);

#ifdef __cplusplus
}
#endif

#endif
