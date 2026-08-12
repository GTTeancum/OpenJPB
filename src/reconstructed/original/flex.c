/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\flex.c.
 *
 * All 32 PDB-emitted procedures are represented here. Fixed-point overflow,
 * signed shifts, short writes, output aliasing, and untouched padding follow
 * the matched retail instructions rather than host-C undefined behavior.
 *
 * Provenance:
 *   direct     - name/signature and named locals from the exact PDB.
 *   decompiled - expression checked against the raw Ghidra export.
 *   assembly   - float/double conversion boundaries and zero-length behavior
 *                checked at RVAs 0x9A470..0x9A69D; 32-bit multiply,
 *                negative adjustment, and arithmetic shift checked at
 *                RVA 0x9A6F0; all remaining scalar, cross/dot, projection,
 *                packed-normal, and short-vector procedures checked at
 *                RVAs 0x9A1C0..0x9AEE7. The projection rejection constant
 *                at 0x45FA70 is the executable's exact -1.0f bit pattern.
 *
 * PDB module: 0035
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\flex.obj
 * Primary source: W:\SWJediPowerBattles\Work\flex.c
 * Compiler language: c
 * Emitted procedures: 32
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/flex.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static int16_t jpb_flex_wrap_short(int32_t value)
{
    uint16_t bits = (uint16_t)value;
    int16_t result;

    memcpy(&result, &bits, sizeof(result));
    return result;
}

static int32_t jpb_flex_wrap_word(uint32_t value)
{
    int32_t result;

    memcpy(&result, &value, sizeof(result));
    return result;
}

static int32_t jpb_flex_shift_right(int32_t value, unsigned shift)
{
    uint32_t bits = (uint32_t)value;

    if ((bits & UINT32_C(0x80000000)) != 0) {
        bits = ~(~bits >> shift);
    } else {
        bits >>= shift;
    }
    return jpb_flex_wrap_word(bits);
}

static uint32_t jpb_flex_abs_bits(int32_t value)
{
    uint32_t bits = (uint32_t)value;

    return value < 0 ? UINT32_C(0) - bits : bits;
}

/* 0x9A1C0, 112 bytes, global, 3 named locals
 * CROSS
 * PDB type: _svector* (_svector*, _svector*,...
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *CROSS(_svector *left, _svector *right, _svector *output)
{
    output->vx = jpb_flex_wrap_short(jpb_flex_wrap_word(
        (uint32_t)((int32_t)left->vy * right->vz) -
        (uint32_t)((int32_t)left->vz * right->vy)));
    output->vy = jpb_flex_wrap_short(jpb_flex_wrap_word(
        (uint32_t)((int32_t)left->vz * right->vx) -
        (uint32_t)((int32_t)left->vx * right->vz)));
    output->vz = jpb_flex_wrap_short(jpb_flex_wrap_word(
        (uint32_t)((int32_t)left->vx * right->vy) -
        (uint32_t)((int32_t)right->vx * left->vy)));
    return output;
}

/* 0x9A230, 201 bytes, global, 3 named locals
 * CROSS12
 * PDB type: _svector* (_svector*, _svector*,...
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *CROSS12(_svector *left, _svector *right, _svector *output)
{
    int16_t product_left;
    int16_t product_right;

    product_left = jpb_flex_wrap_short(flexmul(left->vy, right->vz));
    product_right = jpb_flex_wrap_short(flexmul(left->vz, right->vy));
    output->vx = jpb_flex_wrap_short((int32_t)product_left - product_right);
    product_left = jpb_flex_wrap_short(flexmul(left->vz, right->vx));
    product_right = jpb_flex_wrap_short(flexmul(left->vx, right->vz));
    output->vy = jpb_flex_wrap_short((int32_t)product_left - product_right);
    product_left = jpb_flex_wrap_short(flexmul(left->vx, right->vy));
    product_right = jpb_flex_wrap_short(flexmul(right->vx, left->vy));
    output->vz = jpb_flex_wrap_short((int32_t)product_left - product_right);
    return output;
}

/* 0x9A300, 213 bytes, global, 3 named locals
 * CROSSnormal
 * PDB type: _svector* (_svector*, _svector*,...
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *CROSSnormal(_svector *left, _svector *right, _svector *output)
{
    (void)CROSS12(left, right, output);
    (void)normalize_svector(output, output);
    return output;
}

/* 0x9A3E0, 45 bytes, global, 2 named locals
 * DOT
 * PDB type: int (_svector*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int DOT(_svector *left, _svector *right)
{
    return jpb_flex_wrap_word(
        (uint32_t)((int32_t)left->vz * right->vz) +
        (uint32_t)((int32_t)left->vy * right->vy) +
        (uint32_t)((int32_t)left->vx * right->vx));
}

/* 0x9A410, 85 bytes, global, 2 named locals
 * DOT12
 * PDB type: int (_svector*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int DOT12(_svector *left, _svector *right)
{
    return jpb_flex_wrap_word(
        (uint32_t)flexmul(left->vz, right->vz) +
        (uint32_t)flexmul(left->vy, right->vy) +
        (uint32_t)flexmul(left->vx, right->vx));
}

/* 0x9A470, 179 bytes, global, 3 named locals
 * VectorNormalize
 * PDB type: float (FVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\flex.c
 */
float VectorNormalize(FVECTOR *v)
{
    double r;
    double l;
    float length_squared =
        v->vx * v->vx + v->vy * v->vy + v->vz * v->vz;

    l = sqrt((double)length_squared);
    r = 1.0;
    if (l != 0.0) {
        r = 1.0 / l;
    }
    v->vx = (float)((double)v->vx * r);
    v->vy = (float)((double)v->vy * r);
    v->vz = (float)((double)v->vz * r);
    return (float)l;
}

/* 0x9A530, 173 bytes, global, 4 named locals
 * VectorNormalize2
 * PDB type: float (FVECTOR*, FVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\flex.c
 */
float VectorNormalize2(FVECTOR *v, FVECTOR *w)
{
    float r;
    float l;
    float x = v->vx;
    float length_squared =
        x * x + v->vy * v->vy + v->vz * v->vz;

    l = (float)sqrt((double)length_squared);
    r = 1.0f;
    if (l != 0.0f) {
        r = 1.0f / l;
    }
    w->vx = x * r;
    w->vy = v->vy * r;
    w->vz = v->vz * r;
    return l;
}

/* 0x9A5E0, 189 bytes, global, 6 named locals
 * VectorNormalize3
 * PDB type: float (float, float, float, FVEC...
 * Source: W:\SWJediPowerBattles\Work\flex.c
 */
float VectorNormalize3(float x, float y, float z, FVECTOR *v)
{
    float r;
    float l;
    float length_squared = x * x + y * y + z * z;

    l = (float)sqrt((double)length_squared);
    r = 1.0f;
    if (l != 0.0f) {
        r = 1.0f / l;
    }
    v->vz = r * z;
    v->vx = r * x;
    v->vy = r * y;
    return l;
}

/* 0x9A6A0, 17 bytes, global, 3 named locals
 * distance_squared
 * PDB type: long (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int32_t distance_squared(int x, int y, int z)
{
    return jpb_flex_wrap_word(
        (uint32_t)y * (uint32_t)y +
        (uint32_t)x * (uint32_t)x +
        (uint32_t)z * (uint32_t)z);
}

/* 0x9A6C0, 8 bytes, global, 1 named locals
 * flexabs
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int flexabs(int value)
{
    return value < 0
        ? jpb_flex_wrap_word(UINT32_C(0) - (uint32_t)value)
        : value;
}

/* 0x9A6D0, 17 bytes, global, 2 named locals
 * flexdiv
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int flexdiv(int numerator, int denominator)
{
    int32_t shifted = jpb_flex_wrap_word((uint32_t)numerator << 12);

    if (denominator == 0) {
        return shifted;
    }
    return jpb_flex_wrap_word(
        (uint32_t)((int64_t)shifted / (int64_t)denominator));
}

/* 0x9A6F0, 18 bytes, global, 2 named locals
 * flexmul
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int flexmul(int left, int right)
{
    int32_t product =
        (int32_t)((uint32_t)left * (uint32_t)right);
    uint32_t adjusted =
        (uint32_t)product +
        (product < 0 ? UINT32_C(0x00000fff) : UINT32_C(0));

    return jpb_flex_shift_right(jpb_flex_wrap_word(adjusted), 12);
}

/* 0x9A710, 9 bytes, global, 2 named locals
 * flexmul12
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int flexmul12(int a, int b)
{
    int32_t product = (int32_t)((uint32_t)a * (uint32_t)b);

    return jpb_flex_shift_right(product, 12);
}

/* 0x9A720, 396 bytes, global, 11 named locals
 * fvectorpointlinesquared
 * PDB type: float (FVECTOR*, FVECTOR*, FVECT...
 * Source: W:\SWJediPowerBattles\Work\flex.c
 */
float fvectorpointlinesquared(
    FVECTOR *p0,
    FVECTOR *p1,
    FVECTOR *point,
    float *dist)
{
    float z0 = p0->vz;
    float dz = p1->vz - z0;
    float y0 = p0->vy;
    float dy = p1->vy - y0;
    float x0 = p0->vx;
    float dx = p1->vx - x0;
    float length = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
    float inverse = 1.0f;
    float at;

    if (length != 0.0f) {
        inverse = 1.0f / length;
    }
    at = inverse * dy * (point->vy - y0) +
         inverse * dx * (point->vx - x0) +
         inverse * dz * (point->vz - z0);
    if (at < 0.0f || length < at) {
        return -1.0f;
    }
    if (dist != NULL) {
        *dist = at;
    }
    return
        (point->vx - x0) * (point->vx - x0) +
        (point->vy - y0) * (point->vy - y0) +
        (point->vz - z0) * (point->vz - z0) - at * at;
}

/* 0x9A8B0, 3 bytes, global, 2 named locals
 * intersec_2dlines
 * PDB type: _svector* (_svector*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\flex.c
 */
_svector *intersec_2dlines(_svector *left, _svector *right)
{
    /* Retail is only a ret; make its indeterminate RAX portable. */
    (void)left;
    (void)right;
    return NULL;
}

/* 0x9A8C0, 7 bytes, global, 1 named locals
 * mul4105
 * PDB type: long (long)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int32_t mul4105(int32_t value)
{
    return jpb_flex_wrap_word((uint32_t)value * UINT32_C(0x1009));
}

/* 0x9A8D0, 158 bytes, global, 6 named locals
 * normalize_s
 * PDB type: long (long, long, long, _svector...
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int32_t normalize_s(
    int32_t x,
    int32_t y,
    int32_t z,
    _svector *output)
{
    uint32_t magnitude_bits =
        jpb_flex_abs_bits(x) |
        jpb_flex_abs_bits(y) |
        jpb_flex_abs_bits(z);
    int length;

    while (jpb_flex_wrap_word(magnitude_bits) > INT32_C(0x7fff)) {
        magnitude_bits >>= 1;
        x = jpb_flex_shift_right(x, 1);
        y = jpb_flex_shift_right(y, 1);
        z = jpb_flex_shift_right(z, 1);
    }
    output->vx = jpb_flex_wrap_short(x);
    output->vy = jpb_flex_wrap_short(y);
    output->vz = jpb_flex_wrap_short(z);
    length = SquareRoot0(jpb_flex_wrap_word(
        (uint32_t)((int32_t)output->vx * output->vx) +
        (uint32_t)((int32_t)output->vy * output->vy) +
        (uint32_t)((int32_t)output->vz * output->vz)));
    (void)normalize(output->vx, output->vy, output->vz, output);
    return length;
}

/* 0x9A970, 197 bytes, global, 3 named locals
 * normalize_svector
 * PDB type: long (_svector*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int normalize_svector(_svector *input, _svector *output)
{
    int x = input->vx;
    int y = input->vy;
    int z = input->vz;
    unsigned magnitude_bits =
        (unsigned)(x < 0 ? -x : x) |
        (unsigned)(y < 0 ? -y : y) |
        (unsigned)(z < 0 ? -z : z);
    int length;

    while (magnitude_bits > UINT32_C(0x7fff)) {
        x = (int16_t)x >> 1;
        y = (int16_t)y >> 1;
        z = (int16_t)z >> 1;
        magnitude_bits >>= 1;
        input->vx = (int16_t)x;
    }
    input->vy = (int16_t)y;
    input->vz = (int16_t)z;
    output->vx = (int16_t)x;
    output->vy = (int16_t)y;
    output->vz = (int16_t)z;
    length = SquareRoot0(x * x + y * y + z * z);
    (void)normalize(x, y, z, output);
    return length;
}

/* 0x9AA40, 74 bytes, global, 5 named locals
 * normalize_vector
 * PDB type: long (long, long, long, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int32_t normalize_vector(
    int32_t x,
    int32_t y,
    int32_t z,
    VECTOR *output)
{
    int32_t squared = jpb_flex_wrap_word(
        (uint32_t)x * (uint32_t)x +
        (uint32_t)y * (uint32_t)y +
        (uint32_t)z * (uint32_t)z);

    output->vx = jpb_flex_wrap_word(
        (uint32_t)((int64_t)jpb_flex_wrap_word((uint32_t)x << 12) / squared));
    output->vy = jpb_flex_wrap_word(
        (uint32_t)((int64_t)jpb_flex_wrap_word((uint32_t)y << 12) / squared));
    output->vz = jpb_flex_wrap_word(
        (uint32_t)((int64_t)jpb_flex_wrap_word((uint32_t)z << 12) / squared));
    return squared;
}

/* 0x9AA90, 38 bytes, global, 2 named locals
 * unpack10bitnormal
 * PDB type: void (long, _svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
void unpack10bitnormal(int32_t packed, _svector *output)
{
    output->vx = jpb_flex_wrap_short(
        jpb_flex_shift_right(jpb_flex_wrap_word((uint32_t)packed << 22), 19));
    output->vy = jpb_flex_wrap_short(
        jpb_flex_shift_right(jpb_flex_wrap_word((uint32_t)packed << 12), 19));
    output->vz = jpb_flex_wrap_short(
        jpb_flex_shift_right(
            jpb_flex_wrap_word((uint32_t)packed * UINT32_C(4)), 19));
}

/* 0x9AAC0, 28 bytes, global, 1 named locals
 * vecSqr
 * PDB type: int (_svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int vecSqr(_svector *vector)
{
    return jpb_flex_wrap_word(
        (uint32_t)((int32_t)vector->vx * vector->vx) +
        (uint32_t)((int32_t)vector->vy * vector->vy) +
        (uint32_t)((int32_t)vector->vz * vector->vz));
}

/* 0x9AAE0, 72 bytes, global, 1 named locals
 * vecSqr12
 * PDB type: int (_svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int vecSqr12(_svector *vector)
{
    return jpb_flex_wrap_word(
        (uint32_t)flexmul(vector->vz, vector->vz) +
        (uint32_t)flexmul(vector->vx, vector->vx) +
        (uint32_t)flexmul(vector->vy, vector->vy));
}

/* 0x9AB30, 40 bytes, global, 3 named locals
 * vecadd
 * PDB type: _svector* (_svector*, _svector*,...
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *vecadd(_svector *left, _svector *right, _svector *output)
{
    output->vx = jpb_flex_wrap_short((int32_t)left->vx + right->vx);
    output->vy = jpb_flex_wrap_short((int32_t)left->vy + right->vy);
    output->vz = jpb_flex_wrap_short((int32_t)left->vz + right->vz);
    return output;
}

/* 0x9AB60, 29 bytes, global, 1 named locals
 * veclength
 * PDB type: int (_svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
int veclength(_svector *vector)
{
    return SquareRoot0(vecSqr(vector));
}

/* 0x9AB80, 35 bytes, global, 2 named locals
 * vecnegate
 * PDB type: _svector* (_svector*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *vecnegate(_svector *vector, _svector *output)
{
    output->vx = jpb_flex_wrap_short(-(int32_t)vector->vx);
    output->vy = jpb_flex_wrap_short(-(int32_t)vector->vy);
    output->vz = jpb_flex_wrap_short(-(int32_t)vector->vz);
    return output;
}

/* 0x9ABB0, 38 bytes, global, 1 named locals
 * vecnormalize
 * PDB type: _svector* (_svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *vecnormalize(_svector *vector)
{
    (void)normalize(vector->vx, vector->vy, vector->vz, vector);
    return vector;
}

/* 0x9ABE0, 23 bytes, global, 2 named locals
 * vecnormalize2
 * PDB type: _svector* (_svector*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *vecnormalize2(_svector *vector, _svector *output)
{
    (void)normalize_svector(vector, output);
    return output;
}

/* 0x9AC00, 38 bytes, global, 2 named locals
 * vecnormalizevec
 * PDB type: _svector* (_svector*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *vecnormalizevec(_svector *vector, _svector *output)
{
    (void)normalize_s(vector->vx, vector->vy, vector->vz, output);
    return output;
}

/* 0x9AC30, 17 bytes, global, 4 named locals
 * vecoffset
 * PDB type: _svector* (_svector*, int, int, ...
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *vecoffset(_svector *vector, int x, int y, int z)
{
    vector->vx = jpb_flex_wrap_short((int32_t)vector->vx + x);
    vector->vy = jpb_flex_wrap_short((int32_t)vector->vy + y);
    vector->vz = jpb_flex_wrap_short((int32_t)vector->vz + z);
    return vector;
}

/* 0x9AC50, 540 bytes, global, 14 named locals
 * vecpointlinesquared
 * PDB type: int (_svector*, _svector*, _svec...
 * Source: W:\SWJediPowerBattles\Work\flex.c
 */
static int32_t jpb_flex_shift_right_one(int32_t value)
{
    return jpb_flex_shift_right(value, 1);
}

int vecpointlinesquared(
    _svector *p0,
    _svector *p1,
    _svector *point,
    void *dist)
{
    int d = p1->pad;
    _svector tmp1;
    _svector *s = p1;
    int len = d;
    int32_t x;
    int32_t y;
    int32_t z;
    int at;
    uint32_t squared;

    if (d == 0) {
        int32_t dx = (int32_t)p1->vx - (int32_t)p0->vx;
        int32_t dy = (int32_t)p1->vy - (int32_t)p0->vy;
        int32_t dz = (int32_t)p1->vz - (int32_t)p0->vz;
        uint32_t largest =
            (uint32_t)(dx < 0 ? -dx : dx) |
            (uint32_t)(dy < 0 ? -dy : dy) |
            (uint32_t)(dz < 0 ? -dz : dz);

        while (largest > UINT32_C(0x7fff)) {
            largest >>= 1;
            dx = jpb_flex_shift_right_one(dx);
            dy = jpb_flex_shift_right_one(dy);
            dz = jpb_flex_shift_right_one(dz);
        }

        tmp1.vx = (int16_t)dx;
        tmp1.vy = (int16_t)dy;
        tmp1.vz = (int16_t)dz;
        squared =
            (uint32_t)((int32_t)tmp1.vx * (int32_t)tmp1.vx) +
            (uint32_t)((int32_t)tmp1.vy * (int32_t)tmp1.vy) +
            (uint32_t)((int32_t)tmp1.vz * (int32_t)tmp1.vz);
        len = SquareRoot0(jpb_flex_wrap_word(squared));
        (void)normalize(dx, dy, dz, &tmp1);
        s = &tmp1;
    }

    x = jpb_flex_wrap_short(
        (int32_t)point->vx - (int32_t)p0->vx);
    y = jpb_flex_wrap_short(
        (int32_t)point->vy - (int32_t)p0->vy);
    z = jpb_flex_wrap_short(
        (int32_t)point->vz - (int32_t)p0->vz);
    at = jpb_flex_wrap_word(
        (uint32_t)flexmul12(s->vx, x) +
        (uint32_t)flexmul12(s->vy, y) +
        (uint32_t)flexmul12(s->vz, z));

    if (at < 0 || len < at) {
        return -1;
    }

    if (dist != NULL) {
        if (d == 0) {
            _svector closest;

            closest.vx = jpb_flex_wrap_short(
                flexmul12(s->vx, at) + p0->vx);
            closest.vy = jpb_flex_wrap_short(
                flexmul12(s->vy, at) + p0->vy);
            closest.vz = jpb_flex_wrap_short(
                flexmul12(s->vz, at) + p0->vz);
            closest.pad = jpb_flex_wrap_short(at);
            memcpy(dist, &closest, sizeof(closest));
        } else {
            memcpy(dist, &at, sizeof(at));
        }
    }

    squared =
        (uint32_t)(x * x) +
        (uint32_t)(y * y) +
        (uint32_t)(z * z) -
        (uint32_t)(at * at);
    return jpb_flex_wrap_word(squared);
}

/* 0x9AE70, 79 bytes, global, 3 named locals
 * vecscale
 * PDB type: _svector* (_svector*, int, _svec...
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *vecscale(_svector *vector, int scale, _svector *output)
{
    output->vx = jpb_flex_wrap_short(flexmul(vector->vx, scale));
    output->vy = jpb_flex_wrap_short(flexmul(vector->vy, scale));
    output->vz = jpb_flex_wrap_short(flexmul(vector->vz, scale));
    return output;
}

/* 0x9AEC0, 40 bytes, global, 3 named locals
 * vecsub
 * PDB type: _svector* (_svector*, _svector*,...
 * Source: W:\SWJediPowerBattles\Work\include\flex.h
 */
_svector *vecsub(_svector *left, _svector *right, _svector *output)
{
    output->vx = jpb_flex_wrap_short((int32_t)left->vx - right->vx);
    output->vy = jpb_flex_wrap_short((int32_t)left->vy - right->vy);
    output->vz = jpb_flex_wrap_short((int32_t)left->vz - right->vz);
    return output;
}
