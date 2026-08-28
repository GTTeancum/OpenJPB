#ifndef JPB_VRAM_H
#define JPB_VRAM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JPBVramRect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} JPBVramRect;

typedef struct JPBVramPoint {
    int16_t x;
    int16_t y;
} JPBVramPoint;

extern const JPBVramRect maTexturePageCoords[16];
extern const JPBVramPoint maTextureSubPageCoords[16];
extern int gGlobalPaletteScale;
extern uint32_t gClutID[3];
extern uint32_t gSubClutID[16];
extern int32_t mr;
extern int32_t mg;
extern int32_t mb;
extern int32_t sb;
extern int32_t sg;
extern int32_t sr;

const JPBVramRect *vram_GetPageCoord(int page, JPBVramRect *rectangle);
void vram_GetPaletteRange(uint16_t clutID, int length);
const JPBVramPoint *vram_GetSubPageCoord(
    int page, JPBVramPoint *point);
void vram_GlobalPaletteScale(int scale);
void vram_MakeTranslucent8Palette(uint16_t clutID);
void vram_NormalizePalette(uint16_t clutID, int length);
void vram_gResetVram(void);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_VRAM_STATIC_ASSERT static_assert
#else
#define JPB_VRAM_STATIC_ASSERT _Static_assert
#endif

JPB_VRAM_STATIC_ASSERT(sizeof(JPBVramRect) == 8,
                       "VRAM RECT PDB layout changed");
JPB_VRAM_STATIC_ASSERT(sizeof(JPBVramPoint) == 4,
                       "VRAM POINT PDB layout changed");

#undef JPB_VRAM_STATIC_ASSERT

#endif
