/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\work\sabre.c.
 *
 * All ten emitted procedures are restored from the matched PDB and shipped
 * x64 executable. Executable-wide direct-call scanning proves this legacy
 * trail owner has no external callers in the remaster; preserving that
 * dormant call graph is part of the reconstruction.
 */

#include "jpb/sabre.h"

#include "jpb/linkstubs.h"
#include "jpb/memory.h"
#include "jpb/prim.h"
#include "jpb/vectors.h"
#include "jpb/wrender.h"

#include <stdint.h>
#include <string.h>

Sabre maSabreData[JPB_SABRE_POOL_COUNT];
int32_t mSabreIndex;

static int32_t sabre_i32_from_bits(uint32_t bits)
{
    int32_t value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int32_t sabre_add32(int32_t a, int32_t b)
{
    return sabre_i32_from_bits((uint32_t)a + (uint32_t)b);
}

static int32_t sabre_sub32(int32_t a, int32_t b)
{
    return sabre_i32_from_bits((uint32_t)a - (uint32_t)b);
}

static int32_t sabre_mul32(int32_t a, int32_t b)
{
    return sabre_i32_from_bits((uint32_t)a * (uint32_t)b);
}

static int32_t sabre_sar(int32_t value, unsigned count)
{
    uint32_t bits = (uint32_t)value;

    if ((bits & UINT32_C(0x80000000)) != 0) {
        bits = (bits >> count) | (~UINT32_C(0) << (32u - count));
    } else {
        bits >>= count;
    }
    return sabre_i32_from_bits(bits);
}

static int32_t sabre_trunc_shift12(int32_t value)
{
    if (value < 0) {
        value = sabre_add32(value, 0xfff);
    }
    return sabre_sar(value, 12);
}

static int16_t sabre_vector_component(const VECTOR *vector, int component)
{
    const int32_t *values = &vector->vx;

    return (int16_t)values[component];
}

static int16_t *sabre_svector_component(_svector *vector, int component)
{
    return &vector->vx + component;
}

static void sabre_set_primitive_rgb(POLY_G4 *primitive, uint8_t value)
{
    primitive->r0 = value;
    primitive->g0 = value;
    primitive->b0 = value;
    primitive->r1 = value;
    primitive->g1 = value;
    primitive->b1 = value;
    primitive->r2 = value;
    primitive->g2 = value;
    primitive->b2 = value;
    primitive->r3 = value;
    primitive->g3 = value;
    primitive->b3 = value;
}

/* 0xEF160, 704 bytes. */
int sabre_AddEdge(
    int index, VECTOR *p0, VECTOR *p1, VECTOR *t0, VECTOR *t1)
{
    Sabre *pSabre = &maSabreData[index];
    SabreEdge *pEdge;
    int component;

    if (pSabre->tail >= 24) {
        pSabre->decay = 24;
        return 0;
    }

    pSabre->decay = 16;
    if (pSabre->tail > 0) {
        SabreEdge *previous = &pSabre->aEdges[pSabre->tail - 1];
        int dist1 = (int)vec_DistanceSVLV(&previous->point[0], p0);
        int dist2 = (int)vec_DistanceSVLV(&previous->point[1], p1);

        if (dist2 < 4) {
            for (component = 0; component < 3; ++component) {
                *sabre_svector_component(&previous->point[0], component) =
                    sabre_vector_component(p0, component);
                *sabre_svector_component(&previous->point[1], component) =
                    sabre_vector_component(p1, component);
            }
            return 1;
        }
        if (dist1 < 4) {
            for (component = 0; component < 3; ++component) {
                *sabre_svector_component(&previous->point[0], component) =
                    sabre_vector_component(p0, component);
            }
        }
    }

    pEdge = &pSabre->aEdges[pSabre->tail];
    pEdge->brightness = 255;
    pEdge->flag = 0;
    for (component = 0; component < 3; ++component) {
        *sabre_svector_component(&pEdge->point[0], component) =
            sabre_vector_component(p0, component);
        *sabre_svector_component(&pEdge->point[1], component) =
            sabre_vector_component(p1, component);
        *sabre_svector_component(&pEdge->tanvec[0], component) =
            sabre_vector_component(t0, component);
        *sabre_svector_component(&pEdge->tanvec[1], component) =
            sabre_vector_component(t1, component);
    }

    sabre_set_primitive_rgb(pSabre->pPrims[0] + pSabre->tail, 0x60);
    sabre_set_primitive_rgb(pSabre->pPrims[1] + pSabre->tail, 0x60);
    ++pSabre->tail;
    return pSabre->tail;
}

/* 0xEF420, 2443 bytes. */
void sabre_CatMullData(Sabre *pSabre, int x, int point)
{
    static const int32_t parameters[8] = {
        0x1c7, 0x38e, 0x555, 0x71c, 0x8e3, 0xaaa, 0xc71, 0xe38
    };
    int component;

    for (component = 0; component < 3; ++component) {
        int32_t values[4];
        int32_t middle;
        int sub;

        values[0] = *sabre_svector_component(
            &pSabre->aEdges[x].point[point], component);
        values[1] = *sabre_svector_component(
            &pSabre->aEdges[x + 1].point[point], component);
        values[2] = *sabre_svector_component(
            &pSabre->aEdges[x + 2].point[point], component);
        values[3] = *sabre_svector_component(
            &pSabre->aEdges[x + 3].point[point], component);
        middle = sabre_add32(values[1], values[2]) / 2;

        *sabre_svector_component(
            &pSabre->aEdges[x + 1].point[point], component) =
            (int16_t)sabre_sub32(
                values[1], sabre_sub32(values[1], middle) / 4);
        for (sub = 0; sub < 8; ++sub) {
            *sabre_svector_component(
                &pSabre->aEdges[x + 1].aSubs[sub].point[point],
                component) =
                (int16_t)sabre_gSpline(parameters[sub], values);
        }

        *sabre_svector_component(
            &pSabre->aEdges[x + 2].point[point], component) =
            (int16_t)sabre_gSpline(0x1000, values);
    }
}

/* 0xEFDB0, 706 bytes. */
void sabre_ConformSubEdgeData(subSabreEdge *pSub, int point)
{
    int sub;

    for (sub = 0; sub < JPB_SABRE_SUBEDGE_COUNT - 1; ++sub) {
        int component;

        for (component = 0; component < 3; ++component) {
            int16_t *current =
                sabre_svector_component(&pSub[sub].point[point], component);
            int16_t next = *sabre_svector_component(
                &pSub[sub + 1].point[point], component);
            int delta = (int)*current - (int)next;

            if (delta < 0) {
                delta = -delta;
            }
            if (delta < 16) {
                *current = next;
            }
        }
    }
}

/* 0xF0080, 376 bytes. The matched executable does not consume clut/tpage. */
void sabre_gCreateSabre(int decay, int32_t clut, int32_t tpage)
{
    Sabre *pSabre;
    int primitive;

    (void)clut;
    (void)tpage;
    /* The exact guard permits index two despite the PDB's two-Sabre array. */
    if (mSabreIndex >= 3) {
        return;
    }

    pSabre = &maSabreData[mSabreIndex];
    pSabre->decay = (int16_t)decay;
    pSabre->head = 0;
    pSabre->tail = 0;
    pSabre->length = 0;
    pSabre->pPrims[0] = (POLY_G4 *)memory_gCallocAnyMemory(
        JPB_SABRE_PRIMITIVE_COUNT, sizeof(POLY_G4));
    pSabre->pPrims[1] = (POLY_G4 *)memory_gCallocAnyMemory(
        JPB_SABRE_PRIMITIVE_COUNT, sizeof(POLY_G4));

    for (primitive = 0; primitive < 192; ++primitive) {
        SetPolyG4(&pSabre->pPrims[0][primitive]);
        SetPolyG4(&pSabre->pPrims[1][primitive]);
        SetSemiTrans(&pSabre->pPrims[0][primitive], 1);
        SetSemiTrans(&pSabre->pPrims[1][primitive], 1);
        SetShadeTex(&pSabre->pPrims[0][primitive], 0);
        SetShadeTex(&pSabre->pPrims[1][primitive], 0);
        sabre_set_primitive_rgb(&pSabre->pPrims[0][primitive], 0x80);
        sabre_set_primitive_rgb(&pSabre->pPrims[1][primitive], 0x80);
    }
    ++mSabreIndex;
}

/* 0xF0200, 11 bytes. */
void sabre_gInitSabrePool(void)
{
    mSabreIndex = 0;
}

/* 0xF0210, 118 bytes. */
void sabre_gRenderSabres(void)
{
    int x;

    PushMatrix();
    for (x = 0; x < mSabreIndex; ++x) {
        Sabre *pSabre = &maSabreData[x];

        if (pSabre->head + 1 < pSabre->tail) {
            sabre_gSabreHermiteInterpolation(pSabre);
            prim_gRendSabre(pSabre);
        }
    }
    PopMatrix();
}

/* 0xF0290, 215 bytes. */
void sabre_gSabreCatMullSpline(Sabre *pSabre)
{
    int x;

    if (pSabre->tail - pSabre->head < 4 || pSabre->tail <= 4) {
        return;
    }
    for (x = 1; x + 3 < pSabre->tail; ++x) {
        if (pSabre->aEdges[x].flag != 1 &&
            vec_DistanceSV(
                &pSabre->aEdges[x].point[1],
                &pSabre->aEdges[x + 1].point[1]) >= 64) {
            sabre_CatMullData(pSabre, x - 1, 0);
            sabre_CatMullData(pSabre, x - 1, 1);
            pSabre->aEdges[x].flag = 1;
        }
    }
}

static int32_t sabre_hermite_sample(
    int16_t tangent0,
    int16_t tangent1,
    int16_t point,
    int32_t coefficient0,
    int32_t coefficient1)
{
    int32_t value = sabre_mul32((int32_t)tangent0, coefficient0);

    value = sabre_sub32(
        value, sabre_mul32((int32_t)tangent1, coefficient1));
    value = sabre_add32(value, (int32_t)point);
    return sabre_sar(value, 12);
}

/* 0xF0370, 1669 bytes. */
void sabre_gSabreHermiteInterpolation(Sabre *pSabre)
{
    static const int32_t coefficient0[8] = {
        0x1cc, 0x3ba, 0x5ec, 0x883, 0xba0, 0xf66, 0x13f5, 0x1970
    };
    static const int32_t coefficient1[8] = {
        0x2d, 0x9e, 0x12f, 0x1c1, 0x232, 0x25f, 0x227, 0x169
    };
    int edge;

    if (pSabre->tail - pSabre->head < 3 || pSabre->tail <= 2) {
        return;
    }

    for (edge = 0; edge + 2 < pSabre->tail; ++edge) {
        int point;

        if (pSabre->aEdges[edge].flag == 1) {
            continue;
        }
        for (point = 0; point < 2; ++point) {
            int component;

            for (component = 0; component < 3; ++component) {
                int sample;
                int16_t tangent0 = *sabre_svector_component(
                    &pSabre->aEdges[edge + 1].tanvec[point], component);
                int16_t tangent1 = *sabre_svector_component(
                    &pSabre->aEdges[edge + 2].tanvec[point], component);

                for (sample = 0; sample < 8; ++sample) {
                    int source_edge = sample < 5 ? edge : edge + 1;
                    int16_t source_point = *sabre_svector_component(
                        &pSabre->aEdges[source_edge].point[point],
                        component);
                    *sabre_svector_component(
                        &pSabre->aEdges[edge].aSubs[sample].point[point],
                        component) =
                        (int16_t)sabre_hermite_sample(
                            tangent0,
                            tangent1,
                            source_point,
                            coefficient0[sample],
                            coefficient1[sample]);
                }
            }
        }
        pSabre->aEdges[edge].flag = 1;
    }
}

/* 0xF0A00, 118 bytes. */
int sabre_gSpline(int i, int32_t *v)
{
    int32_t p;
    int32_t term;
    int32_t middle;

    term = sabre_sub32(v[1], v[2]);
    term = sabre_mul32(term, 3);
    term = sabre_sub32(term, v[0]);
    term = sabre_add32(term, v[3]);
    term = sabre_sar(sabre_mul32(term, i), 12);

    p = sabre_add32(v[0], sabre_mul32(v[2], 2));
    p = sabre_mul32(p, 2);
    p = sabre_add32(p, term);
    p = sabre_sub32(p, sabre_mul32(v[1], 5));
    p = sabre_sub32(p, v[3]);
    p = sabre_sar(sabre_mul32(p, i), 12);
    p = sabre_sub32(p, v[0]);
    p = sabre_add32(p, v[2]);
    p = sabre_sar(sabre_mul32(p, i), 12);
    p = sabre_add32(p, v[1]);

    middle = sabre_add32(v[2], v[1]) / 2;
    return sabre_sub32(p, sabre_sub32(p, middle) / 4);
}

/* 0xF0A80, 190 bytes. */
int spline_gHermiteInterpolation(int i, int *v)
{
    int32_t i2 = sabre_sar(sabre_mul32(i, i), 12);
    int32_t i3 = sabre_trunc_shift12(sabre_mul32(i2, i));
    int32_t c0 = sabre_trunc_shift12(sabre_mul32(i3, 2));
    int32_t c1 = sabre_trunc_shift12(sabre_mul32(i2, 3));
    int32_t c2 = sabre_sub32(
        i3, sabre_trunc_shift12(sabre_mul32(i2, 2)));
    int32_t term0;
    int32_t term1;
    int32_t result;

    term0 = sabre_add32(i, c2);
    term0 = sabre_mul32(term0, v[2]);
    term0 = sabre_mul32(term0, 0x1000);
    term0 = sabre_trunc_shift12(term0);

    term1 = sabre_mul32(v[3], 0x1000);
    term1 = sabre_mul32(term1, sabre_sub32(i3, i2));
    term1 = sabre_trunc_shift12(term1);

    result = sabre_add32(term0, term1);
    result = sabre_add32(
        result, sabre_mul32(sabre_sub32(c1, c0), v[1]));
    result = sabre_add32(
        result,
        sabre_mul32(sabre_add32(sabre_sub32(c0, c1), 1), v[0]));
    return sabre_sar(result, 12);
}
