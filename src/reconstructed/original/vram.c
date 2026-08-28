/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\Work\vram.c.
 * PDB module: 0093
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\vram.obj
 * Primary source: W:\SWJediPowerBattles\Work\vram.c
 * Compiler language: c
 * Emitted procedures: 6
 */

#include "jpb/vram.h"

#include "jpb/linkstubs.h"

#include <stdint.h>

const JPBVramRect maTexturePageCoords[16] = {
    {512, 0, 64, 1}, {576, 0, 64, 1}, {640, 0, 64, 1}, {704, 0, 64, 1},
    {768, 0, 64, 1}, {832, 0, 64, 1}, {896, 0, 64, 1}, {960, 0, 64, 1},
    {512, 1, 64, 1}, {576, 1, 64, 1}, {640, 1, 64, 1}, {704, 1, 64, 1},
    {768, 1, 64, 1}, {832, 1, 64, 1}, {896, 1, 64, 1}, {960, 1, 64, 1},
};

const JPBVramPoint maTextureSubPageCoords[16] = {
    {0, 0}, {64, 0}, {128, 0}, {192, 0},
    {0, 64}, {64, 64}, {128, 64}, {192, 64},
    {0, 128}, {64, 128}, {128, 128}, {192, 128},
    {0, 192}, {64, 192}, {128, 192}, {192, 192},
};

int gGlobalPaletteScale = 0x80000;
uint32_t gClutID[3];
uint32_t gSubClutID[16];
int32_t mr;
int32_t mg;
int32_t mb;
int32_t sb;
int32_t sg;
int32_t sr;

static int32_t vram_fixed12_truncate(int32_t left, int32_t right)
{
    int32_t product = (int32_t)((uint32_t)left * (uint32_t)right);
    uint32_t adjusted = (uint32_t)product +
        (product < 0 ? UINT32_C(0xfff) : UINT32_C(0));
    uint32_t shifted = adjusted >> 12;

    if ((int32_t)adjusted < 0) {
        shifted |= UINT32_C(0xfff00000);
    }
    return (int32_t)shifted;
}

/* 0x1086D0, 66 bytes, global, 2 named locals
 * vram_GetPageCoord
 * PDB type: const RECT* (int, RECT*)
 * Source: W:\SWJediPowerBattles\Work\vram.c
 */
const JPBVramRect *vram_GetPageCoord(
    int page, JPBVramRect *rectangle)
{
    const JPBVramRect *result = &maTexturePageCoords[page];
    if (rectangle != NULL) {
        *rectangle = *result;
    }
    return result;
}

/* 0x108720, 256 bytes, global, 9 named locals
 * vram_GetPaletteRange
 * PDB type: void (unsigned short, int)
 * Source: W:\SWJediPowerBattles\Work\vram.c
 */
void vram_GetPaletteRange(uint16_t clutID, int length)
{
    JPBVramRect rectangle;
    uint16_t buffer[256];
    uint16_t index = 0;
    int32_t red;
    int32_t green;
    int32_t blue;

    rectangle.x = (int16_t)((clutID & 0x3f) << 4);
    rectangle.y = (int16_t)(clutID >> 6);
    rectangle.w = (int16_t)length;
    rectangle.h = 1;
    StoreImage((SRECT *)(void *)&rectangle, (unsigned *)(void *)buffer);
    (void)DrawSync(0);

    red = green = blue =
        (uint16_t)rectangle.x | ((int32_t)(uint16_t)rectangle.y << 16);
    while ((int)index < length) {
        uint16_t color = buffer[index];
        if (color != 0) {
            red = color & 0x1f;
            green = (color >> 5) & 0x1f;
            blue = (color >> 10) & 0x1f;
        }
        if (red < mr) {
            red = mr;
        }
        mr = red;
        mg = green < mr ? mr : green;
        mb = blue < mr ? mr : blue;
        ++index;
    }
}

/* 0x108820, 68 bytes, global, 2 named locals
 * vram_GetSubPageCoord
 * PDB type: const _POINT* (int, _POINT*)
 * Source: W:\SWJediPowerBattles\Work\vram.c
 */
const JPBVramPoint *vram_GetSubPageCoord(
    int page, JPBVramPoint *point)
{
    int clamped = page < 15 ? page : 15;
    const JPBVramPoint *result;

    if (clamped < 0) {
        clamped = 0;
    }
    result = &maTextureSubPageCoords[clamped];
    if (point != NULL) {
        *point = *result;
    }
    return result;
}

/* 0x108870, 289 bytes, global, 1 named locals
 * vram_GlobalPaletteScale
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\vram.c
 */
void vram_GlobalPaletteScale(int scale)
{
    int index;
    int red_scale;
    int green_scale;
    int blue_scale;
    int maximum;

    (void)scale;
    for (index = 0; index < 3; ++index) {
        vram_GetPaletteRange((uint16_t)gClutID[index], 256);
    }
    for (index = 0; index < 16; ++index) {
        vram_GetPaletteRange((uint16_t)gSubClutID[index], 16);
    }

    red_scale = 0xf8000 / mr;
    green_scale = 0xf8000 / mg;
    blue_scale = 0xf8000 / mb;
    maximum = red_scale > green_scale ? red_scale : green_scale;
    if (blue_scale > maximum) {
        maximum = blue_scale;
    }
    sr = maximum;
    sg = maximum;
    sb = maximum;

    for (index = 0; index < 3; ++index) {
        vram_NormalizePalette((uint16_t)gClutID[index], 256);
    }
    for (index = 0; index < 16; ++index) {
        vram_NormalizePalette((uint16_t)gSubClutID[index], 16);
    }
}

/* 0x1089A0, 176 bytes, global, 3 named locals
 * vram_MakeTranslucent8Palette
 * PDB type: void (unsigned short)
 * Source: W:\SWJediPowerBattles\Work\vram.c
 */
void vram_MakeTranslucent8Palette(uint16_t clutID)
{
    JPBVramRect rectangle;
    uint16_t buffer[256];
    int index;

    rectangle.x = (int16_t)((clutID & 0x3f) << 4);
    rectangle.y = (int16_t)(clutID >> 6);
    rectangle.w = 256;
    rectangle.h = 1;
    StoreImage((SRECT *)(void *)&rectangle, (unsigned *)(void *)buffer);
    (void)DrawSync(0);

    for (index = 0; index < 256; ++index) {
        if (buffer[index] != 0 && (int16_t)buffer[index] >= 0) {
            buffer[index] |= 0x8000;
        }
    }
    (void)LoadClut(
        (unsigned *)(void *)buffer, rectangle.x, rectangle.y);
    (void)DrawSync(0);
}

/* 0x108A50, 369 bytes, global, 11 named locals
 * vram_NormalizePalette
 * PDB type: void (unsigned short, int)
 * Source: W:\SWJediPowerBattles\Work\vram.c
 */
void vram_NormalizePalette(uint16_t clutID, int length)
{
    uint16_t buffer[256];
    uint16_t index = 0;
    int32_t red;
    int32_t green;
    int32_t blue;
    int32_t translucent;

    while ((int)index < length) {
        uint16_t color = buffer[index];
        if (color != 0) {
            red = color & 0x1f;
            green = (color >> 5) & 0x1f;
            blue = (color >> 10) & 0x1f;
            translucent = color >> 15;
        }
        red = vram_fixed12_truncate(red, sr);
        green = vram_fixed12_truncate(green, sg);
        blue = vram_fixed12_truncate(blue, sb);
        buffer[index] = (uint16_t)((((translucent << 5) | blue) << 5 |
                                    green) << 5 | red);
        ++index;
    }

    (void)LoadClut(
        (unsigned *)(void *)buffer,
        (int16_t)((clutID & 0x3f) << 4),
        (int16_t)(clutID >> 6));
    (void)DrawSync(0);
}
