#ifndef JPB_PRIM_H
#define JPB_PRIM_H

#include "jpb/fmath.h"
#include "jpb/sabre.h"

#include <stddef.h>
#include <stdint.h>

typedef struct POLY_GT4 {
    uint32_t tag;
    uint8_t r0;
    uint8_t g0;
    uint8_t b0;
    uint8_t code;
    int16_t x0;
    int16_t y0;
    uint8_t u0;
    uint8_t v0;
    uint16_t clut;
    uint8_t r1;
    uint8_t g1;
    uint8_t b1;
    uint8_t p1;
    int16_t x1;
    int16_t y1;
    uint8_t u1;
    uint8_t v1;
    uint16_t tpage;
    uint8_t r2;
    uint8_t g2;
    uint8_t b2;
    uint8_t p2;
    int16_t x2;
    int16_t y2;
    uint8_t u2;
    uint8_t v2;
    uint16_t pad2;
    uint8_t r3;
    uint8_t g3;
    uint8_t b3;
    uint8_t p3;
    int16_t x3;
    int16_t y3;
    uint8_t u3;
    uint8_t v3;
    uint16_t pad3;
} POLY_GT4;

/* Exact matched-PC PDB layouts owned by prim.obj. */
#ifndef JPB_SRECT_DEFINED
#define JPB_SRECT_DEFINED
typedef struct SRECT {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} SRECT;
#endif

typedef SRECT JPB_RECT;

typedef struct DR_ENV {
    uint32_t tag;
    uint32_t code[15];
} DR_ENV;

typedef struct DR_MODE {
    uint32_t tag;
    uint32_t code[2];
} DR_MODE;

typedef struct DRAWENV {
    SRECT clip;
    int16_t ofs[2];
    SRECT tw;
    uint16_t tpage;
    uint8_t dtd;
    uint8_t dfe;
    uint8_t isbg;
    uint8_t r0;
    uint8_t g0;
    uint8_t b0;
    DR_ENV dr_env;
} DRAWENV;

typedef struct DISPENV {
    SRECT disp;
    SRECT screen;
    uint8_t isinter;
    uint8_t isrgb24;
    uint8_t pad0;
    uint8_t pad1;
} DISPENV;

typedef struct POLY_FT4 {
    uint32_t tag;
    uint8_t r0;
    uint8_t g0;
    uint8_t b0;
    uint8_t code;
    int16_t x0;
    int16_t y0;
    uint8_t u0;
    uint8_t v0;
    uint16_t clut;
    int16_t x1;
    int16_t y1;
    uint8_t u1;
    uint8_t v1;
    uint16_t tpage;
    int16_t x2;
    int16_t y2;
    uint8_t u2;
    uint8_t v2;
    uint16_t pad1;
    int16_t x3;
    int16_t y3;
    uint8_t u3;
    uint8_t v3;
    uint16_t pad2;
} POLY_FT4;

typedef struct TIM_IMAGE {
    uint32_t mode;
    uint32_t alignment_pad;
    SRECT *crect;
    uint32_t *caddr;
    SRECT *prect;
    uint32_t *paddr;
} TIM_IMAGE;

typedef struct primDrawingSurface {
    DRAWENV draw;
    DISPENV disp;
    uint32_t aaOT_BG[32];
    uint32_t ot_underflow[32];
    uint32_t aOT[1024];
    uint32_t aOT_BG[32];
    POLY_G4 cel;
} primDrawingSurface;

/* Direct PDB SramFloorStack layout used by prim_gRendSabre. */
typedef struct SramFloorStack {
    int32_t xleft;
    int32_t xright;
    int32_t ytop;
    int32_t ybottom;
    _svector v3TopLeft;
    _svector v3TopRight;
    _svector v3BotLeft;
    _svector v3BotRight;
    int16_t pt0[2];
    int16_t pt1[2];
    int16_t pt2[2];
    int16_t pt3[2];
    int32_t clip;
    int32_t z;
    POLY_GT4 *pCurrentPrim;
    POLY_G4 *pCurrentSabrePrim;
    int32_t countX;
    int32_t countY;
    int32_t tileX;
    int32_t tileY;
    int32_t p;
    int32_t Quad[5];
    int32_t quad;
    int16_t cacheX;
    int16_t cacheY;
    int32_t CACHE;
    CVECTOR ocol;
    CVECTOR col;
    VECTOR direct;
} SramFloorStack;

#ifdef __cplusplus
extern "C" {
#endif

void AddBlur(int DrawId);
void QuickEnd(void);
void QuickStart(void);
void initscoredigits(void);
void plotscorenumber(VECTOR *pos, int number, uint32_t color, int alpha);
void prim_GpuCallback(void);
void prim_gSetBkColor(int red, int green, int blue);
void prim_RendSabreEdges(
    SramFloorStack *sr,
    Sabre *pSabre,
    SabreEdge *edge1,
    SabreEdge *edge2,
    int brightness,
    int brightness2);
void prim_SetTextureWindow(JPB_RECT *tw);
void prim_SetTranslucency(
    uint32_t *p1, uint32_t *p2, int translucency);
void prim_gRendSabre(Sabre *pSabre);

extern POLY_FT4 blurquad[2][4];
extern uint8_t digitsTim[244];
extern uint8_t *primptr;
extern uint8_t *primlimit;
extern uint16_t scoretpageplus;
extern uint16_t scoretpageminus;
extern uint16_t scoreclut;
extern uint16_t scorex;
extern uint16_t scorey;
extern DR_MODE tw_mode[2];
extern uint32_t *maCurrentOT;
extern uint32_t *maCurrentBGOT;
extern primDrawingSurface *mpCurrentDrawingSurface;
extern primDrawingSurface maDrawingSurface[2];
extern int mDrawingSurfaceId;

/*
 * Descriptive inspection seam for the two retail draw-environment color
 * copies owned by prim_gSetBkColor.
 */
const CVECTOR *jpb_PrimGetBackgroundColor(int surface);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_PRIM_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_PRIM_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_PRIM_STATIC_ASSERT(sizeof(SramFloorStack) == 168, "SramFloorStack layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(SramFloorStack, pCurrentPrim) == 72, "SramFloorStack.pCurrentPrim layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(SramFloorStack, pCurrentSabrePrim) == 80, "SramFloorStack.pCurrentSabrePrim layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(SramFloorStack, countX) == 88, "SramFloorStack.countX layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(SramFloorStack, direct) == 148, "SramFloorStack.direct layout changed");
JPB_PRIM_STATIC_ASSERT(sizeof(SRECT) == 8, "SRECT layout changed");
JPB_PRIM_STATIC_ASSERT(sizeof(DR_ENV) == 64, "DR_ENV layout changed");
JPB_PRIM_STATIC_ASSERT(sizeof(DR_MODE) == 12, "DR_MODE layout changed");
JPB_PRIM_STATIC_ASSERT(sizeof(DRAWENV) == 92, "DRAWENV layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(DRAWENV, r0) == 25, "DRAWENV.r0 layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(DRAWENV, dr_env) == 28, "DRAWENV.dr_env layout changed");
JPB_PRIM_STATIC_ASSERT(sizeof(DISPENV) == 20, "DISPENV layout changed");
JPB_PRIM_STATIC_ASSERT(sizeof(POLY_FT4) == 40, "POLY_FT4 layout changed");
JPB_PRIM_STATIC_ASSERT(sizeof(POLY_GT4) == 52, "POLY_GT4 layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(POLY_GT4, code) == 7, "POLY_GT4.code layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(POLY_GT4, tpage) == 26, "POLY_GT4.tpage layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(POLY_GT4, u3) == 48, "POLY_GT4.u3 layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(POLY_FT4, tpage) == 22, "POLY_FT4.tpage layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(POLY_FT4, u3) == 36, "POLY_FT4.u3 layout changed");
JPB_PRIM_STATIC_ASSERT(sizeof(TIM_IMAGE) == 40, "TIM_IMAGE layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(TIM_IMAGE, crect) == 8, "TIM_IMAGE.crect layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(TIM_IMAGE, prect) == 24, "TIM_IMAGE.prect layout changed");
JPB_PRIM_STATIC_ASSERT(sizeof(primDrawingSurface) == 4628, "primDrawingSurface layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(primDrawingSurface, disp) == 92, "primDrawingSurface.disp layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(primDrawingSurface, aOT) == 368, "primDrawingSurface.aOT layout changed");
JPB_PRIM_STATIC_ASSERT(offsetof(primDrawingSurface, cel) == 4592, "primDrawingSurface.cel layout changed");

#undef JPB_PRIM_STATIC_ASSERT

#endif
