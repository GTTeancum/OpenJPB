/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\prim.c.
 *
 * PDB module: 0065
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\prim.obj
 * Compiler language: c
 * Emitted procedures: 11
 */

#include "jpb/prim.h"

#include "jpb/filesys.h"
#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/jonny.h"
#include "jpb/linkstubs.h"
#include "jpb/wrender.h"

#include <stdint.h>
#include <string.h>

POLY_FT4 blurquad[2][4];
uint8_t *primptr;
uint8_t *primlimit;
uint16_t scoretpageplus;
uint16_t scoretpageminus;
uint16_t scoreclut;
uint16_t scorex;
uint16_t scorey;
DR_MODE tw_mode[2];
uint32_t *maCurrentOT;
uint32_t *maCurrentBGOT;
primDrawingSurface *mpCurrentDrawingSurface;
primDrawingSurface maDrawingSurface[2];

/* Exact initialized data at matched-PC RVA 0x4CBAF0. */
uint8_t digitsTim[244] = {
    0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x2c, 0x00, 0x00, 0x00, 0x40, 0x02, 0xff, 0x00,
    0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0x7f,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00,
    0x40, 0x02, 0xfa, 0x00, 0x12, 0x00, 0x05, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11,
    0x00, 0x00, 0x01, 0x00, 0x11, 0x11, 0x00, 0x11,
    0x11, 0x00, 0x00, 0x01, 0x00, 0x11, 0x11, 0x00,
    0x10, 0x11, 0x00, 0x11, 0x11, 0x01, 0x10, 0x11,
    0x00, 0x10, 0x11, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x01, 0x10, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x10, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01,
    0x11, 0x11, 0x01, 0x10, 0x11, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x10, 0x11, 0x00, 0x10,
    0x11, 0x00, 0x01, 0x10, 0x00, 0x11, 0x11, 0x00,
    0x11, 0x11, 0x00, 0x00, 0x10, 0x00, 0x10, 0x11,
    0x00, 0x10, 0x11, 0x01, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x11, 0x11,
    0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11,
    0x00, 0x00, 0x01, 0x00, 0x11, 0x11, 0x01, 0x11,
    0x11, 0x00, 0x00, 0x10, 0x00, 0x11, 0x11, 0x00,
    0x10, 0x11, 0x00, 0x00, 0x01, 0x00, 0x10, 0x11,
    0x00, 0x10, 0x11, 0x00
};

/* PDB local-static byte at RVA 0x53D308; no retail writer exists. */
static uint8_t c;

const CVECTOR *jpb_PrimGetBackgroundColor(int surface)
{
    if ((unsigned)surface >= 2) {
        return NULL;
    }
    return (const CVECTOR *)(const void *)&maDrawingSurface[surface].draw.r0;
}

/* 0xE8B80, 343 bytes, global, 4 named locals
 * AddBlur
 * PDB type: void (int)
 */
void AddBlur(int DrawId)
{
    static const int16_t y[4] = {1, 1, 128, 128};
    static const uint8_t v[4] = {1, 1, 128, 128};
    POLY_FT4 *primitive = blurquad[DrawId];
    int i;

    for (i = 0; i < 4; ++i, ++primitive) {
        int side = i & 1;
        uint8_t left_u = (uint8_t)(side * 0xa0 + side * 0x80);
        uint8_t right_u =
            (uint8_t)(side * 0xa0 - 0x60 + side * 0x80);

        primitive->r0 = 0x80;
        primitive->g0 = 0x80;
        primitive->b0 = (uint8_t)(c - 0x40);
        primitive->x0 = (int16_t)(side * 0xa0);
        primitive->x1 = (int16_t)(primitive->x0 + 0xa0);
        primitive->x2 = primitive->x0;
        primitive->x3 = primitive->x1;
        primitive->y0 = y[i];
        primitive->y1 = y[i];
        primitive->y2 = (int16_t)(y[i] + 0x7f);
        primitive->y3 = primitive->y2;
        primitive->u0 = left_u;
        primitive->u1 = right_u;
        primitive->u2 = left_u;
        primitive->u3 = right_u;
        primitive->v0 = v[i];
        primitive->v1 = v[i];
        primitive->v2 = (uint8_t)(v[i] + 0x7f);
        primitive->v3 = primitive->v2;
        /* GetTPage is a bare retail return, leaving AX as v + 0x7f. */
        primitive->tpage = primitive->v2;
    }
}

/* 0xE8CE0, 102 bytes, global, 1 named locals
 * QuickEnd
 * PDB type: void (<no type>)
 */
void QuickEnd(void)
{
    mDrawingSurfaceId ^= 1;
}

/* 0xE8D50, 55 bytes, global, 1 named locals
 * QuickStart
 * PDB type: void (<no type>)
 */
void QuickStart(void)
{
    mpCurrentDrawingSurface = &maDrawingSurface[mDrawingSurfaceId];
    maCurrentOT = mpCurrentDrawingSurface->aOT;
}

/* 0xE8D90, 213 bytes, global, 1 named locals
 * initscoredigits
 * PDB type: void (<no type>)
 */
void initscoredigits(void)
{
    static char digits_path[] =
        "c:\\el_chavo\\work\\art\\sprites\\digits.tim";

    /*
     * No retail call reaches this procedure. On the shipped installation the
     * absolute developer path fails. Its remaining instructions consume an
     * uninitialized TIM_IMAGE because OpenTIM and ReadTIM are bare returns;
     * adding a parser would create behavior absent from the executable.
     */
    (void)file_LoadFile(digits_path, digitsTim);
}

static void prim_write_score_quad(
    POLY_FT4 *primitive,
    uint32_t color,
    uint16_t x,
    uint16_t y,
    uint16_t u,
    uint16_t v,
    uint16_t clut,
    uint16_t tpage)
{
    uint32_t command = color | UINT32_C(0x2e000000);

    memcpy(&primitive->r0, &command, sizeof(command));
    primitive->x0 = (int16_t)x;
    primitive->y0 = (int16_t)y;
    primitive->u0 = (uint8_t)u;
    primitive->v0 = (uint8_t)v;
    primitive->clut = clut;
    primitive->x1 = (int16_t)(x + 6);
    primitive->y1 = (int16_t)y;
    primitive->u1 = (uint8_t)(u + 6);
    primitive->v1 = (uint8_t)v;
    primitive->tpage = tpage;
    primitive->x2 = (int16_t)x;
    primitive->y2 = (int16_t)(y + 5);
    primitive->u2 = (uint8_t)u;
    primitive->v2 = (uint8_t)(v + 5);
    primitive->pad1 = 0;
    primitive->x3 = (int16_t)(x + 6);
    primitive->y3 = (int16_t)(y + 5);
    primitive->u3 = (uint8_t)(u + 6);
    primitive->v3 = (uint8_t)(v + 5);
    primitive->pad2 = 0;
}

static void prim_link_score_quad(POLY_FT4 *primitive)
{
    uint32_t old_tag = *maCurrentOT;
    uint32_t pointer_low =
        (uint32_t)(uintptr_t)primitive & UINT32_C(0xffffff);

    primitive->tag =
        (old_tag & UINT32_C(0xffffff)) ^ UINT32_C(0x09000000);
    *maCurrentOT =
        old_tag ^ ((old_tag ^ pointer_low) & UINT32_C(0xffffff));
}

/* 0xE8E70, 934 bytes, global, 14 named locals
 * plotscorenumber
 * PDB type: void (VECTOR*, int, unsigned long, int)
 */
void plotscorenumber(VECTOR *pos, int number, uint32_t color, int alpha)
{
    uint8_t text[16];
    uint8_t *end = text;
    int32_t value;
    intptr_t available;
    intptr_t required;
    uint32_t color2;
    uint32_t white2;
    uint16_t x;
    uint16_t y;

    if (number == 0) {
        return;
    }
    if (alpha < 0 || alpha > 0x1000) {
        alpha = 0;
    }
    color2 = color_interpolate4k(
        color & UINT32_C(0xffffff), 0, alpha);
    white2 = color_interpolate4k(UINT32_C(0xffffff), 0, alpha);

    value = number;
    if (value < 0) {
        uint32_t magnitude = UINT32_C(0) - (uint32_t)value;
        memcpy(&value, &magnitude, sizeof(value));
    }
    do {
        int32_t quotient = value / 10;
        *end++ = (uint8_t)(value - quotient * 10 + 2);
        value = quotient;
    } while (value != 0);
    *end = (uint8_t)((uint32_t)number >> 31);

    required = (intptr_t)(end - text + 2) * 0x50;
    available =
        (intptr_t)(uintptr_t)primlimit - (intptr_t)(uintptr_t)primptr;
    if (available < required) {
        return;
    }

    PushMatrix();
    ((_svector *)(void *)gaScratch)->vx = (int16_t)pos->vx;
    ((_svector *)(void *)gaScratch)->vy = (int16_t)pos->vy;
    ((_svector *)(void *)gaScratch)->vz = (int16_t)pos->vz;
    (void)TransformPoints(
        (_svector *)(void *)gaScratch, (int *)(void *)gaScratch, 1);
    PopMatrix();

    x = *(uint16_t *)(void *)&gaScratch[0];
    y = *(uint16_t *)(void *)&gaScratch[2];
    if (x > 0x1ff || y > OptionStruct.ScreenHeight) {
        return;
    }

    for (;;) {
        uint16_t u = (uint16_t)((int8_t)*end * 6 + scorex);
        POLY_FT4 *primitive = (POLY_FT4 *)(void *)primptr;

        primptr += sizeof(*primitive);
        prim_write_score_quad(
            primitive,
            color2,
            x,
            y,
            u,
            scorey,
            scoreclut,
            scoretpageplus);
        prim_link_score_quad(primitive);

        primitive = (POLY_FT4 *)(void *)primptr;
        primptr += sizeof(*primitive);
        prim_write_score_quad(
            primitive,
            white2,
            x,
            y,
            u,
            scorey,
            scoreclut,
            scoretpageminus);
        prim_link_score_quad(primitive);

        x = (uint16_t)(x + 8);
        if (end == text) {
            break;
        }
        --end;
    }
}

/* 0xE9220, 3 bytes, global, 0 named locals
 * prim_GpuCallback
 * PDB type: void (<no type>)
 */
void prim_GpuCallback(void)
{
}

/* 0xE9230, 3 bytes, global, 6 named locals
 * prim_RendSabreEdges
 * PDB type: void (SramFloorStack*, Sabre*, SabreEdge*, SabreEdge*, int, int)
 */
void prim_RendSabreEdges(
    SramFloorStack *sr,
    Sabre *pSabre,
    SabreEdge *edge1,
    SabreEdge *edge2,
    int brightness,
    int brightness2)
{
    (void)sr;
    (void)pSabre;
    (void)edge1;
    (void)edge2;
    (void)brightness;
    (void)brightness2;
}

/* 0xE9240, 44 bytes, global, 1 named locals
 * prim_SetTextureWindow
 * PDB type: void (RECT*)
 */
void prim_SetTextureWindow(JPB_RECT *tw)
{
    (void)tw;
}

/* 0xE9270, 48 bytes, global, 3 named locals
 * prim_SetTranslucency
 * PDB type: void (unsigned long*, unsigned long*, int)
 */
void prim_SetTranslucency(
    uint32_t *p1, uint32_t *p2, int translucency)
{
    (void)p1;
    (void)p2;
    (void)translucency;
}

/* 0xE92A0, 189 bytes, global, 2 named locals
 * prim_gRendSabre
 * PDB type: void (Sabre*)
 */
void prim_gRendSabre(Sabre *pSabre)
{
    SramFloorStack *sr = (SramFloorStack *)(void *)getScratchAddr(0);
    int edge = pSabre->head;

    sr->pCurrentSabrePrim = pSabre->pPrims[mDrawingSurfaceId];
    sr->countX = edge;
    if (edge + 1 >= pSabre->tail) {
        return;
    }

    do {
        pSabre->aEdges[edge].brightness =
            (int16_t)(pSabre->aEdges[edge].brightness - pSabre->decay);
        if (pSabre->aEdges[edge].brightness <= pSabre->decay) {
            pSabre->aEdges[edge].brightness = 0;
            ++pSabre->head;
            if (pSabre->head == pSabre->tail - 1) {
                pSabre->head = 0;
                pSabre->tail = 0;
                return;
            }
        }
        ++sr->countX;
        edge = sr->countX;
    } while (edge + 1 < pSabre->tail);
}

/* 0xE9360, 39 bytes, global, 3 named locals
 * prim_gSetBkColor
 * PDB type: void (int, int, int)
 */
void prim_gSetBkColor(int red, int green, int blue)
{
    int surface;

    for (surface = 0; surface < 2; ++surface) {
        maDrawingSurface[surface].draw.r0 = (uint8_t)red;
        maDrawingSurface[surface].draw.g0 = (uint8_t)green;
        maDrawingSurface[surface].draw.b0 = (uint8_t)blue;
    }
}
