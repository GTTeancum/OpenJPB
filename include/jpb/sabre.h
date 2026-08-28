#ifndef JPB_SABRE_H
#define JPB_SABRE_H

#include "jpb/fmath.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_SABRE_EDGE_CAPACITY = 48,
    JPB_SABRE_SUBEDGE_COUNT = 8,
    JPB_SABRE_POOL_COUNT = 2,
    JPB_SABRE_PRIMITIVE_COUNT = 240
};

/* Direct PDB type POLY_G4, 36 bytes in the matched x64 build. */
typedef struct POLY_G4 {
    uint32_t tag;
    uint8_t r0;
    uint8_t g0;
    uint8_t b0;
    uint8_t code;
    int16_t x0;
    int16_t y0;
    uint8_t r1;
    uint8_t g1;
    uint8_t b1;
    uint8_t pad1;
    int16_t x1;
    int16_t y1;
    uint8_t r2;
    uint8_t g2;
    uint8_t b2;
    uint8_t pad2;
    int16_t x2;
    int16_t y2;
    uint8_t r3;
    uint8_t g3;
    uint8_t b3;
    uint8_t pad3;
    int16_t x3;
    int16_t y3;
} POLY_G4;

typedef struct subSabreEdge {
    _svector point[2];
    _svector tanvec[2];
} subSabreEdge;

typedef struct SabreEdge {
    _svector point[2];
    _svector tanvec[2];
    int16_t brightness;
    int16_t flag;
    subSabreEdge aSubs[JPB_SABRE_SUBEDGE_COUNT];
} SabreEdge;

typedef struct Sabre {
    SabreEdge aEdges[JPB_SABRE_EDGE_CAPACITY];
    POLY_G4 *pPrims[2];
    int16_t head;
    /* Retail uses the PDB-named tail field as its active edge count. */
    int16_t tail;
    int16_t length;
    int16_t decay;
    int32_t sabreFlags;
} Sabre;

extern Sabre maSabreData[JPB_SABRE_POOL_COUNT];
extern int32_t mSabreIndex;

int sabre_AddEdge(
    int index, VECTOR *p0, VECTOR *p1, VECTOR *t0, VECTOR *t1);
void sabre_CatMullData(Sabre *pSabre, int x, int point);
void sabre_ConformSubEdgeData(subSabreEdge *pSub, int point);
void sabre_gCreateSabre(int decay, int32_t clut, int32_t tpage);
void sabre_gInitSabrePool(void);
void sabre_gRenderSabres(void);
void sabre_gSabreCatMullSpline(Sabre *pSabre);
void sabre_gSabreHermiteInterpolation(Sabre *pSabre);
int sabre_gSpline(int i, int32_t *v);
int spline_gHermiteInterpolation(int i, int *v);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_SABRE_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_SABRE_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_SABRE_STATIC_ASSERT(sizeof(POLY_G4) == 36, "POLY_G4 layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(POLY_G4, x0) == 8, "POLY_G4.x0 layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(POLY_G4, x3) == 32, "POLY_G4.x3 layout changed");
JPB_SABRE_STATIC_ASSERT(sizeof(subSabreEdge) == 32, "subSabreEdge layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(subSabreEdge, tanvec) == 16, "subSabreEdge.tanvec layout changed");
JPB_SABRE_STATIC_ASSERT(sizeof(SabreEdge) == 292, "SabreEdge layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(SabreEdge, brightness) == 32, "SabreEdge.brightness layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(SabreEdge, flag) == 34, "SabreEdge.flag layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(SabreEdge, aSubs) == 36, "SabreEdge.aSubs layout changed");
JPB_SABRE_STATIC_ASSERT(sizeof(Sabre) == 14048, "Sabre layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(Sabre, pPrims) == 14016, "Sabre.pPrims layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(Sabre, head) == 14032, "Sabre.head layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(Sabre, tail) == 14034, "Sabre.tail layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(Sabre, length) == 14036, "Sabre.length layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(Sabre, decay) == 14038, "Sabre.decay layout changed");
JPB_SABRE_STATIC_ASSERT(offsetof(Sabre, sabreFlags) == 14040, "Sabre.sabreFlags layout changed");

#undef JPB_SABRE_STATIC_ASSERT

#endif
