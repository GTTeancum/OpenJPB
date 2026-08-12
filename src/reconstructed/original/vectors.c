/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\vectors.c.
 *
 * All 29 emitted procedures now have reviewed, dependency-light bodies.
 * Original procedure records remain below the bodies as provenance.
 *
 * Provenance:
 *   direct     - names/signatures/locals from the exact PDB and data layouts
 *                from TPI.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - 32-bit multiply/add wrap and tail calls checked at the exact
 *                RVAs.
 *
 * PDB module: 0090
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\vectors.obj
 * Primary source: W:\SWJediPowerBattles\Work\vectors.c
 * Compiler language: c
 * Emitted procedures: 29
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

/* 0x103970, 100 bytes, global, 3 named locals
 * InvertMatrix
 * PDB type: MATRIX* (MATRIX*, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

#include "jpb/fmath.h"
#include "jpb/vectors.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

/*
 * The reference uses 32-bit imul/add/sub instructions. Unsigned arithmetic
 * below makes their two's-complement wrap defined in portable C before the
 * bit pattern is passed to SquareRoot0 as a signed argument.
 */
static uint32_t vectors_square_mod32(uint32_t value)
{
    return value * value;
}

static uint32_t vectors_squared_sum2(uint32_t a, uint32_t b)
{
    return vectors_square_mod32(a) + vectors_square_mod32(b);
}

static uint32_t vectors_squared_sum3(uint32_t a, uint32_t b, uint32_t c)
{
    return vectors_squared_sum2(a, b) + vectors_square_mod32(c);
}

static int32_t vectors_i32_from_bits(uint32_t bits)
{
    int32_t value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int16_t vectors_i16_from_bits(uint16_t bits)
{
    int16_t value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int32_t vectors_wrap_add(int32_t a, int32_t b)
{
    return vectors_i32_from_bits((uint32_t)a + (uint32_t)b);
}

static int32_t vectors_wrap_subtract(int32_t a, int32_t b)
{
    return vectors_i32_from_bits((uint32_t)a - (uint32_t)b);
}

static int32_t vectors_wrap_multiply(int32_t a, int32_t b)
{
    return vectors_i32_from_bits((uint32_t)a * (uint32_t)b);
}

static int32_t vectors_arithmetic_shift_right(int32_t value, unsigned bits)
{
    uint32_t shifted = (uint32_t)value >> bits;

    if (value < 0) {
        shifted |= UINT32_MAX << (32U - bits);
    }
    return vectors_i32_from_bits(shifted);
}

static int32_t vectors_fixed12_truncate(int32_t value)
{
    if (value < 0) {
        value = vectors_wrap_add(value, 4095);
    }
    return vectors_arithmetic_shift_right(value, 12);
}

static int32_t vectors_abs_wrap(int32_t value)
{
    if (value >= 0) {
        return value;
    }
    return vectors_i32_from_bits(0U - (uint32_t)value);
}

static int16_t vectors_low_i16(int32_t value)
{
    return vectors_i16_from_bits((uint16_t)(uint32_t)value);
}

static int32_t vectors_trunc_float_to_i32(float value)
{
    if (!(value >= -2147483648.0f && value < 2147483648.0f)) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static float vectors_flip_float_sign(float value)
{
    uint32_t bits;

    memcpy(&bits, &value, sizeof(bits));
    bits ^= UINT32_C(0x80000000);
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int32_t vectors_return_after_negw(int32_t value, int16_t *stored)
{
    uint32_t bits = (uint32_t)value;
    uint16_t low = (uint16_t)(0U - (uint16_t)bits);

    *stored = vectors_i16_from_bits(low);
    bits = (bits & UINT32_C(0xFFFF0000)) | low;
    return vectors_i32_from_bits(bits);
}

/* Reference RVA 0x103970, 100 bytes. */
MATRIX *InvertMatrix(MATRIX *src, MATRIX *dst)
{
    float upper;
    int32_t lower;

    upper = src->m[0][1];
    lower = vectors_trunc_float_to_i32(src->m[1][0]);
    dst->m[1][0] = upper;
    dst->m[0][1] = (float)lower;

    upper = src->m[0][2];
    lower = vectors_trunc_float_to_i32(src->m[2][0]);
    dst->m[2][0] = upper;
    dst->m[0][2] = (float)lower;

    upper = src->m[1][2];
    lower = vectors_trunc_float_to_i32(src->m[2][1]);
    dst->m[2][1] = upper;
    dst->m[1][2] = (float)lower;

    if (src != dst) {
        dst->m[0][0] = src->m[0][0];
        dst->m[1][1] = src->m[1][1];
        dst->m[2][2] = src->m[2][2];
    }
    return dst;
}

/* Reference RVA 0x1039E0, 212 bytes. */
int32_t vec_AreaTriangle(VECTOR *a, VECTOR *b, VECTOR *c)
{
    int32_t side_a = vectors_i32_from_bits(vec_DistanceLV(b, c));
    int32_t side_b = vectors_i32_from_bits(vec_DistanceLV(a, c));
    int32_t side_c = vectors_i32_from_bits(vec_DistanceLV(a, b));
    int32_t perimeter = vectors_wrap_add(
        vectors_wrap_add(side_c, side_b), side_a);
    int32_t semiperimeter = perimeter / 2;
    int32_t product = vectors_wrap_multiply(
        vectors_wrap_subtract(semiperimeter, side_c),
        vectors_wrap_subtract(semiperimeter, side_b));

    product = vectors_wrap_multiply(
        product, vectors_wrap_subtract(semiperimeter, side_a));
    product = vectors_wrap_multiply(product, semiperimeter);
    return SquareRoot0(product);
}

/* Reference RVA 0x103AC0, 422 bytes. */
int32_t vec_DistPoint2Line(VECTOR *p, VECTOR *a, VECTOR *b)
{
    int16_t ax = vectors_low_i16(a->vx);
    int16_t ay = vectors_low_i16(a->vy);
    int16_t az = vectors_low_i16(a->vz);
    int16_t px = vectors_low_i16(p->vx);
    int16_t py = vectors_low_i16(p->vy);
    int16_t pz = vectors_low_i16(p->vz);
    int32_t direction_x = (int32_t)vectors_i16_from_bits(
        (uint16_t)b->vx - (uint16_t)ax);
    int32_t direction_y = (int32_t)vectors_i16_from_bits(
        (uint16_t)b->vy - (uint16_t)ay);
    int32_t direction_z = (int32_t)vectors_i16_from_bits(
        (uint16_t)b->vz - (uint16_t)az);
    _svector norm;
    int32_t length =
        normalize(direction_x, direction_y, direction_z, &norm);
    int32_t dp = vectors_fixed12_truncate(vectors_wrap_multiply(
        (int32_t)vectors_i16_from_bits((uint16_t)px - (uint16_t)ax),
        (int32_t)norm.vx));
    int32_t term = vectors_fixed12_truncate(vectors_wrap_multiply(
        (int32_t)vectors_i16_from_bits((uint16_t)pz - (uint16_t)az),
        (int32_t)norm.vz));
    int16_t nearest_x;
    int16_t nearest_y;
    int16_t nearest_z;
    int32_t dx;
    int32_t dy;
    int32_t dz;

    dp = vectors_wrap_add(dp, term);
    term = vectors_fixed12_truncate(vectors_wrap_multiply(
        (int32_t)vectors_i16_from_bits((uint16_t)py - (uint16_t)ay),
        (int32_t)norm.vy));
    dp = vectors_wrap_add(dp, term);
    if (dp < 0 || length < dp) {
        return -1;
    }

    nearest_x = vectors_i16_from_bits(
        (uint16_t)vectors_fixed12_truncate(
            vectors_wrap_multiply((int32_t)norm.vx, dp)) +
        (uint16_t)ax);
    nearest_y = vectors_i16_from_bits(
        (uint16_t)vectors_fixed12_truncate(
            vectors_wrap_multiply((int32_t)norm.vy, dp)) +
        (uint16_t)ay);
    nearest_z = vectors_i16_from_bits(
        (uint16_t)vectors_fixed12_truncate(
            vectors_wrap_multiply((int32_t)norm.vz, dp)) +
        (uint16_t)az);

    dx = vectors_abs_wrap(vectors_wrap_subtract(p->vx, (int32_t)nearest_x));
    dy = vectors_abs_wrap(vectors_wrap_subtract(p->vy, (int32_t)nearest_y));
    dz = vectors_abs_wrap(vectors_wrap_subtract(p->vz, (int32_t)nearest_z));
    return SquareRoot0(vectors_i32_from_bits(
        vectors_squared_sum3((uint32_t)dx, (uint32_t)dy, (uint32_t)dz)));
}

/* Reference RVA 0x103C70, 28 bytes. */
uint32_t vec_Distance2DLV(VECTOR *v0, VECTOR *v1)
{
    uint32_t dx = (uint32_t)v1->vx - (uint32_t)v0->vx;
    uint32_t dz = (uint32_t)v1->vz - (uint32_t)v0->vz;

    return (uint32_t)SquareRoot0(
        vectors_i32_from_bits(vectors_squared_sum2(dx, dz)));
}

/* Reference RVA 0x103C90, 43 bytes. */
uint32_t vec_DistanceLV(VECTOR *v0, VECTOR *v1)
{
    uint32_t dx = (uint32_t)v1->vx - (uint32_t)v0->vx;
    uint32_t dy = (uint32_t)v1->vy - (uint32_t)v0->vy;
    uint32_t dz = (uint32_t)v1->vz - (uint32_t)v0->vz;

    return (uint32_t)SquareRoot0(
        vectors_i32_from_bits(vectors_squared_sum3(dx, dy, dz)));
}

/* Reference RVA 0x103CC0, 81 bytes. */
uint32_t vec_DistanceSV(_svector *v0, _svector *v1)
{
    uint32_t dx = (uint32_t)((int32_t)v1->vx - (int32_t)v0->vx);
    uint32_t dy = (uint32_t)((int32_t)v1->vy - (int32_t)v0->vy);
    uint32_t dz = (uint32_t)((int32_t)v1->vz - (int32_t)v0->vz);

    return (uint32_t)SquareRoot0(
        vectors_i32_from_bits(vectors_squared_sum3(dx, dy, dz)));
}

/* Reference RVA 0x103D20, 78 bytes. */
uint32_t vec_DistanceSVLV(_svector *v0, VECTOR *v1)
{
    uint32_t dx = (uint32_t)v1->vx - (uint32_t)(int32_t)v0->vx;
    uint32_t dy = (uint32_t)v1->vy - (uint32_t)(int32_t)v0->vy;
    uint32_t dz = (uint32_t)v1->vz - (uint32_t)(int32_t)v0->vz;

    return (uint32_t)SquareRoot0(
        vectors_i32_from_bits(vectors_squared_sum3(dx, dy, dz)));
}

/* Reference RVA 0x103D70, 41 bytes. */
MATRIX *vec_IdentMatrix(MATRIX *m)
{
    m->m[0][0] = 1.0f;
    m->m[0][1] = 0.0f;
    m->m[0][2] = 0.0f;
    m->m[1][0] = 0.0f;
    m->m[1][1] = 1.0f;
    m->m[1][2] = 0.0f;
    m->m[2][0] = 0.0f;
    m->m[2][1] = 0.0f;
    m->m[2][2] = 1.0f;
    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    return m;
}

static void vectors_scaled_identity(MATRIX *matrix, float scale)
{
    (void)vec_IdentMatrix(matrix);
    matrix->m[0][0] = scale;
    matrix->m[1][1] = scale;
    matrix->m[2][2] = scale;
}

/* Reference RVA 0x103DA0, 224 bytes. */
int vec_InvRotVectorLV(_svector *rot, VECTOR *src, VECTOR *dest)
{
    VECTOR temp = *src;
    MATRIX m3x;
    MATRIX m3y;

    vectors_scaled_identity(&m3x, 4096.0f);
    vectors_scaled_identity(&m3y, 4096.0f);
    (void)fRotMatrixX((int)rot->vx, &m3x);
    (void)fApplyMatrixLV(&m3x, &temp, dest);
    temp = *dest;
    (void)fRotMatrixY((int)rot->vy, &m3y);
    (void)fApplyMatrixLV(&m3y, &temp, dest);

    /*
     * The reference brackets this local-only work with the renderer's
     * PushMatrix/PopMatrix. At normal stack depths the pair has no lasting
     * state effect, and callers ignore the accidental stack-depth value left
     * in EAX despite the PDB's int return type. The portable form omits that
     * renderer dependency and defines the otherwise indeterminate return.
     */
    return 0;
}

/* Reference RVA 0x103E80, 26 bytes. */
uint32_t vec_LengthLV(VECTOR *v0)
{
    uint32_t sum = vectors_squared_sum3(
        (uint32_t)v0->vx,
        (uint32_t)v0->vy,
        (uint32_t)v0->vz);

    return (uint32_t)SquareRoot0(vectors_i32_from_bits(sum));
}

/* Reference RVA 0x103EA0, 68 bytes. */
void vec_LinearCombine(
    _svector *s_point, VECTOR *w_point, _svector *light, int16_t t)
{
    int32_t value = vectors_wrap_multiply((int32_t)light->vx, (int32_t)t);

    s_point->vx = vectors_i16_from_bits(
        (uint16_t)((uint16_t)value + (uint16_t)w_point->vx));
    value = vectors_wrap_multiply((int32_t)light->vy, (int32_t)t);
    s_point->vy = vectors_i16_from_bits(
        (uint16_t)((uint16_t)value + (uint16_t)w_point->vy));
    value = vectors_wrap_multiply((int32_t)light->vz, (int32_t)t);
    s_point->vz = vectors_i16_from_bits(
        (uint16_t)((uint16_t)value + (uint16_t)w_point->vz));
}

/* Reference RVA 0x103EF0, 164 bytes. */
int vec_PointNearSegment(int dist, VECTOR *p, VECTOR *a, VECTOR *b)
{
    int32_t minimum;
    int32_t maximum;

    minimum = a->vx < b->vx ? a->vx : b->vx;
    maximum = a->vx > b->vx ? a->vx : b->vx;
    if (!(vectors_wrap_subtract(minimum, dist) < p->vx) ||
        !(p->vx < vectors_wrap_add(maximum, dist))) {
        return 0;
    }

    minimum = a->vy < b->vy ? a->vy : b->vy;
    maximum = a->vy > b->vy ? a->vy : b->vy;
    if (!(vectors_wrap_subtract(minimum, dist) < p->vy) ||
        !(p->vy < vectors_wrap_add(maximum, dist))) {
        return 0;
    }

    minimum = a->vz < b->vz ? a->vz : b->vz;
    maximum = a->vz > b->vz ? a->vz : b->vz;
    return vectors_wrap_subtract(minimum, dist) < p->vz &&
           p->vz < vectors_wrap_add(maximum, dist);
}

/* Reference RVA 0x103FA0, 119 bytes. */
void vec_Polar2Rect(VECTOR *polar, VECTOR *dest)
{
    int32_t value = vectors_wrap_multiply(rcos(polar->vx), rcos(polar->vy));

    value = vectors_arithmetic_shift_right(value, 12);
    value = vectors_wrap_multiply(value, polar->vz);
    dest->vx = vectors_arithmetic_shift_right(value, 12);

    value = vectors_wrap_multiply(rsin(polar->vx), polar->vz);
    dest->vy = vectors_arithmetic_shift_right(value, 12);

    value = vectors_wrap_multiply(rcos(polar->vx), rsin(polar->vy));
    value = vectors_arithmetic_shift_right(value, 12);
    value = vectors_wrap_multiply(value, polar->vz);
    dest->vz = vectors_arithmetic_shift_right(value, 12);
}

/* Reference RVA 0x104020, 100 bytes. */
uint32_t vec_QuickDistanceLV(VECTOR *v0, VECTOR *v1)
{
    int32_t x = vectors_abs_wrap(
        vectors_wrap_subtract(v1->vx, v0->vx) / 16);
    int32_t y = vectors_abs_wrap(
        vectors_wrap_subtract(v1->vy, v0->vy) / 16);
    int32_t z = vectors_abs_wrap(
        vectors_wrap_subtract(v1->vz, v0->vz) / 16);
    int32_t minimum = x < y ? x : y;

    if (z < minimum) {
        minimum = z;
    }
    return (uint32_t)minimum;
}

/* Reference RVA 0x104090, 68 bytes. */
uint32_t vec_QuickRangeCheck(VECTOR *v0, VECTOR *v1, int range)
{
    if (vectors_abs_wrap(vectors_wrap_subtract(v1->vx, v0->vx)) > range) {
        return 0;
    }
    if (vectors_abs_wrap(vectors_wrap_subtract(v1->vz, v0->vz)) > range) {
        return 0;
    }
    return (uint32_t)(
        vectors_abs_wrap(vectors_wrap_subtract(v1->vy, v0->vy)) <= range);
}

/* Reference RVA 0x1040E0, 85 bytes. */
uint32_t vec_QuickRangeCheckFV(FVECTOR *v0, FVECTOR *v1, float range)
{
    float x = v1->vx - v0->vx;
    float y = v1->vy - v0->vy;
    float z = v1->vz - v0->vz;

    if (!(x > 0.0f)) {
        x = vectors_flip_float_sign(x);
    }
    if (x > range) {
        return 0;
    }
    if (!(y > 0.0f)) {
        y = vectors_flip_float_sign(y);
    }
    if (y > range) {
        return 0;
    }
    if (!(z > 0.0f)) {
        z = vectors_flip_float_sign(z);
    }
    return (uint32_t)!(z > range);
}

/* Reference RVA 0x104140, 52 bytes. */
uint32_t vec_RangeCheck(VECTOR *v0, VECTOR *v1, int range)
{
    int32_t distance = vectors_i32_from_bits(vec_Distance2DLV(v0, v1));

    return (uint32_t)(distance <= range);
}

static int32_t vectors_rot_from_components(
    _svector *rot, int32_t x, int32_t y, int32_t z)
{
    uint32_t squared = vectors_squared_sum2((uint32_t)x, (uint32_t)z);
    int32_t length = SquareRoot0(vectors_i32_from_bits(squared));
    int32_t angle;

    rot->vz = 0;
    rot->vy = vectors_low_i16(ratan2(x, z));
    angle = ratan2(y, length);
    return vectors_return_after_negw(angle, &rot->vx);
}

/* Reference RVA 0x104180, 93 bytes. */
int32_t vec_RotFromNormal(_svector *rot, VECTOR *tp)
{
    return vectors_rot_from_components(rot, tp->vx, tp->vy, tp->vz);
}

/* Reference RVA 0x1041E0, 111 bytes. */
int32_t vec_RotFromNormalF(_svector *rot, FVECTOR *tp)
{
    float squared = tp->vx * tp->vx + tp->vz * tp->vz;
    int32_t length = SquareRoot0(vectors_trunc_float_to_i32(squared));
    int32_t x = vectors_trunc_float_to_i32(tp->vx);
    int32_t y = vectors_trunc_float_to_i32(tp->vy);
    int32_t z = vectors_trunc_float_to_i32(tp->vz);
    int32_t angle;

    rot->vz = 0;
    rot->vy = vectors_low_i16(ratan2(x, z));
    angle = ratan2(y, length);
    return vectors_return_after_negw(angle, &rot->vx);
}

/* Reference RVA 0x104250, 98 bytes. */
int32_t vec_RotFromNormalS(_svector *rot, _svector *tp)
{
    return vectors_rot_from_components(
        rot, (int32_t)tp->vx, (int32_t)tp->vy, (int32_t)tp->vz);
}

/* Reference RVA 0x1042C0, 125 bytes. */
int vec_RotVectorLV(_svector *rot, VECTOR *src, VECTOR *dest)
{
    MATRIX matrix;

    vectors_scaled_identity(&matrix, 4096.0f);
    (void)fRotMatrix(rot, &matrix);
    (void)fApplyMatrixLV(&matrix, src, dest);

    /* See vec_InvRotVectorLV for the omitted balanced render-stack pair. */
    return 0;
}

/* Reference RVA 0x104340, 123 bytes. */
int vec_RotVectorY(int y, VECTOR *src, VECTOR *dest)
{
    MATRIX matrix;

    vectors_scaled_identity(&matrix, 1.0f);
    (void)fRotMatrixY(y, &matrix);
    (void)fApplyMatrixLV(&matrix, src, dest);

    /* See vec_InvRotVectorLV for the omitted balanced render-stack pair. */
    return 0;
}

/* Reference RVA 0x1043C0, 41 bytes. */
void vec_ScaleVector(_svector *vector, int scale)
{
    int32_t value =
        vectors_wrap_multiply((int32_t)vector->vx, (int32_t)scale);

    vector->vx = vectors_low_i16(vectors_arithmetic_shift_right(value, 12));
    value = vectors_wrap_multiply((int32_t)vector->vy, (int32_t)scale);
    vector->vy = vectors_low_i16(vectors_arithmetic_shift_right(value, 12));
    value = vectors_wrap_multiply((int32_t)vector->vz, (int32_t)scale);
    vector->vz = vectors_low_i16(vectors_arithmetic_shift_right(value, 12));
}

/* Reference RVA 0x1043F0, 140 bytes. */
void vec_VectorNormalLV(VECTOR *vector, VECTOR *normal)
{
    VECTOR half;

    if (vectors_abs_wrap(vector->vx) > 8192 ||
        vectors_abs_wrap(vector->vy) > 8192 ||
        vectors_abs_wrap(vector->vz) > 8192) {
        half.vx = vector->vx / 2;
        half.vy = vector->vy / 2;
        half.vz = vector->vz / 2;
        half.pad = 0;
        vector = &half;
    }
    (void)VectorNormal(vector, normal);
}

/* Reference RVA 0x104480, 140 bytes. */
void vec_VectorNormalSV(VECTOR *vector, _svector *normal)
{
    VECTOR half;

    if (vectors_abs_wrap(vector->vx) > 4095 ||
        vectors_abs_wrap(vector->vy) > 4095 ||
        vectors_abs_wrap(vector->vz) > 4095) {
        half.vx = vector->vx / 2;
        half.vy = vector->vy / 2;
        half.vz = vector->vz / 2;
        half.pad = 0;
        vector = &half;
    }
    (void)VectorNormalS(vector, normal);
}

/* Reference RVA 0x104510, 203 bytes. */
void vec_gDefinePlane(VECTOR *p, VECTOR *q, VECTOR *r, Plane *plane)
{
    int32_t qx = vectors_wrap_subtract(q->vx, p->vx);
    int32_t qy = vectors_wrap_subtract(q->vy, p->vy);
    int32_t qz = vectors_wrap_subtract(q->vz, p->vz);
    int32_t rx = vectors_wrap_subtract(r->vx, p->vx);
    int32_t ry = vectors_wrap_subtract(r->vy, p->vy);
    int32_t rz = vectors_wrap_subtract(r->vz, p->vz);
    int32_t nx = vectors_wrap_subtract(
        vectors_wrap_multiply(rz, qy), vectors_wrap_multiply(ry, qz));
    int32_t ny = vectors_wrap_subtract(
        vectors_wrap_multiply(rx, qz), vectors_wrap_multiply(rz, qx));
    int32_t nz = vectors_wrap_subtract(
        vectors_wrap_multiply(ry, qx), vectors_wrap_multiply(rx, qy));
    int32_t constant = vectors_wrap_add(
        vectors_wrap_multiply(nx, p->vx),
        vectors_wrap_multiply(nz, p->vz));

    plane->plane_normal.vx = nx;
    plane->plane_normal.vy = ny;
    plane->plane_normal.vz = nz;
    plane->plane_const = vectors_wrap_add(
        constant, vectors_wrap_multiply(ny, p->vy));

    /*
     * Substituted diagnostic side effect: the reference sends the values to
     * its internal console. Geometry and stored state are exact; the portable
     * foundation deliberately does not depend on that console module.
     */
}

/* Reference RVA 0x1045E0, 45 bytes. */
void vec_gDefinePlaneNormal(VECTOR *normal, VECTOR *p, Plane *plane)
{
    int32_t constant;

    plane->plane_normal.vx = normal->vx;
    plane->plane_normal.vy = normal->vy;
    plane->plane_normal.vz = normal->vz;
    constant = vectors_wrap_add(
        vectors_wrap_multiply(normal->vz, p->vz),
        vectors_wrap_multiply(normal->vy, p->vy));
    plane->plane_const = vectors_wrap_add(
        constant, vectors_wrap_multiply(normal->vx, p->vx));
}

/* Reference RVA 0x104610, 166 bytes. */
void vec_gProject2Plane(
    _svector *s_point, VECTOR *w_point, _svector *light, Plane *plane)
{
    int32_t divisor = vectors_wrap_add(
        vectors_wrap_multiply((int32_t)light->vx, plane->plane_normal.vx),
        vectors_wrap_multiply((int32_t)light->vy, plane->plane_normal.vy));
    int32_t numerator;
    int16_t distance;
    int32_t value;

    divisor = vectors_wrap_add(
        divisor,
        vectors_wrap_multiply((int32_t)light->vz, plane->plane_normal.vz));
    if (divisor == 0) {
        distance = 0;
    } else {
        numerator = vectors_wrap_subtract(
            plane->plane_const,
            vectors_wrap_multiply(plane->plane_normal.vx, w_point->vx));
        numerator = vectors_wrap_subtract(
            numerator,
            vectors_wrap_multiply(plane->plane_normal.vy, w_point->vy));
        numerator = vectors_wrap_subtract(
            numerator,
            vectors_wrap_multiply(plane->plane_normal.vz, w_point->vz));
        distance = vectors_low_i16(numerator / divisor);
    }

    value = vectors_wrap_multiply((int32_t)distance, (int32_t)light->vx);
    s_point->vx = vectors_i16_from_bits(
        (uint16_t)((uint16_t)value + (uint16_t)w_point->vx));
    value = vectors_wrap_multiply((int32_t)distance, (int32_t)light->vy);
    s_point->vy = vectors_i16_from_bits(
        (uint16_t)((uint16_t)value + (uint16_t)w_point->vy));
    value = vectors_wrap_multiply((int32_t)distance, (int32_t)light->vz);
    s_point->vz = vectors_i16_from_bits(
        (uint16_t)((uint16_t)value + (uint16_t)w_point->vz));
}

static int32_t vectors_project_f12_dot_term(int32_t normal, int32_t light)
{
    int32_t shifted = vectors_i32_from_bits((uint32_t)normal << 12);
    int32_t product = vectors_wrap_multiply(shifted, light);

    if (product < 0) {
        product = vectors_wrap_add(product, 4095);
    }
    return vectors_arithmetic_shift_right(product, 12);
}

/* Reference RVA 0x1046C0, 229 bytes. */
void vec_gProject2PlaneF12(
    _svector *s_point, VECTOR *w_point, VECTOR *light, Plane *plane)
{
    int32_t divisor = vectors_wrap_add(
        vectors_project_f12_dot_term(
            plane->plane_normal.vx, light->vx),
        vectors_project_f12_dot_term(
            plane->plane_normal.vz, light->vz));
    int32_t numerator;
    int32_t distance;
    int32_t value;

    divisor = vectors_wrap_add(
        divisor,
        vectors_project_f12_dot_term(
            plane->plane_normal.vy, light->vy));
    if (divisor == 0) {
        distance = 0;
    } else {
        numerator = vectors_wrap_subtract(
            plane->plane_const,
            vectors_wrap_multiply(plane->plane_normal.vz, w_point->vz));
        numerator = vectors_wrap_subtract(
            numerator,
            vectors_wrap_multiply(plane->plane_normal.vy, w_point->vy));
        numerator = vectors_wrap_subtract(
            numerator,
            vectors_wrap_multiply(plane->plane_normal.vx, w_point->vx));
        numerator = vectors_i32_from_bits((uint32_t)numerator << 12);
        distance = numerator / divisor;
    }

    value = vectors_wrap_multiply(light->vx, distance);
    value = vectors_arithmetic_shift_right(value, 12);
    s_point->vx = vectors_i16_from_bits(
        (uint16_t)((uint16_t)value + (uint16_t)w_point->vx));
    value = vectors_wrap_multiply(light->vy, distance);
    value = vectors_arithmetic_shift_right(value, 12);
    s_point->vy = vectors_i16_from_bits(
        (uint16_t)((uint16_t)value + (uint16_t)w_point->vy));
    value = vectors_wrap_multiply(light->vz, distance);
    value = vectors_arithmetic_shift_right(value, 12);
    s_point->vz = vectors_i16_from_bits(
        (uint16_t)((uint16_t)value + (uint16_t)w_point->vz));
}

/* 0x1039E0, 212 bytes, global, 15 named locals
 * vec_AreaTriangle
 * PDB type: long (VECTOR*, VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103AC0, 422 bytes, global, 10 named locals
 * vec_DistPoint2Line
 * PDB type: long (VECTOR*, VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103C70, 28 bytes, global, 4 named locals
 * vec_Distance2DLV
 * PDB type: unsigned long (VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103C90, 43 bytes, global, 5 named locals
 * vec_DistanceLV
 * PDB type: unsigned long (VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103CC0, 81 bytes, global, 5 named locals
 * vec_DistanceSV
 * PDB type: unsigned long (_svector*, _svect...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103D20, 78 bytes, global, 5 named locals
 * vec_DistanceSVLV
 * PDB type: unsigned long (_svector*, VECTOR...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103D70, 41 bytes, global, 1 named locals
 * vec_IdentMatrix
 * PDB type: MATRIX* (MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103DA0, 224 bytes, global, 6 named locals
 * vec_InvRotVectorLV
 * PDB type: int (_svector*, VECTOR*, VECTOR*...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103E80, 26 bytes, global, 1 named locals
 * vec_LengthLV
 * PDB type: unsigned long (VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103EA0, 68 bytes, global, 4 named locals
 * vec_LinearCombine
 * PDB type: void (_svector*, VECTOR*, _svect...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103EF0, 164 bytes, global, 4 named locals
 * vec_PointNearSegment
 * PDB type: int (int, VECTOR*, VECTOR*, VECT...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x103FA0, 119 bytes, global, 2 named locals
 * vec_Polar2Rect
 * PDB type: void (VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x104020, 100 bytes, global, 2 named locals
 * vec_QuickDistanceLV
 * PDB type: unsigned long (VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x104090, 68 bytes, global, 3 named locals
 * vec_QuickRangeCheck
 * PDB type: unsigned long (VECTOR*, VECTOR*,...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x1040E0, 85 bytes, global, 6 named locals
 * vec_QuickRangeCheckFV
 * PDB type: unsigned long (FVECTOR*, FVECTOR...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x104140, 52 bytes, global, 5 named locals
 * vec_RangeCheck
 * PDB type: unsigned long (VECTOR*, VECTOR*,...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x104180, 93 bytes, global, 3 named locals
 * vec_RotFromNormal
 * PDB type: long (_svector*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x1041E0, 111 bytes, global, 3 named locals
 * vec_RotFromNormalF
 * PDB type: long (_svector*, FVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x104250, 98 bytes, global, 3 named locals
 * vec_RotFromNormalS
 * PDB type: long (_svector*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x1042C0, 125 bytes, global, 4 named locals
 * vec_RotVectorLV
 * PDB type: int (_svector*, VECTOR*, VECTOR*...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x104340, 123 bytes, global, 4 named locals
 * vec_RotVectorY
 * PDB type: int (int, VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x1043C0, 41 bytes, global, 2 named locals
 * vec_ScaleVector
 * PDB type: void (_svector*, int)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x1043F0, 140 bytes, global, 3 named locals
 * vec_VectorNormalLV
 * PDB type: void (VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x104480, 140 bytes, global, 3 named locals
 * vec_VectorNormalSV
 * PDB type: void (VECTOR*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x104510, 203 bytes, global, 6 named locals
 * vec_gDefinePlane
 * PDB type: void (VECTOR*, VECTOR*, VECTOR*,...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x1045E0, 45 bytes, global, 3 named locals
 * vec_gDefinePlaneNormal
 * PDB type: void (VECTOR*, VECTOR*, Plane*)
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x104610, 166 bytes, global, 6 named locals
 * vec_gProject2Plane
 * PDB type: void (_svector*, VECTOR*, _svect...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */

/* 0x1046C0, 229 bytes, global, 8 named locals
 * vec_gProject2PlaneF12
 * PDB type: void (_svector*, VECTOR*, VECTOR...
 * Source: W:\SWJediPowerBattles\Work\vectors.c
 */
