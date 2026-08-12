/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\fmath.c.
 *
 * Reviewed bodies cover all 49 PDB-named procedures, including the scalar
 * fixed-point/trigonometric primitives, matrix operations, packed-vector
 * batch transforms, camera-space conversion, and projection paths. The PDB
 * procedure catalogue remains below the bodies as an address/size index.
 *
 * Provenance:
 *   direct     - function names/signatures/locals from the exact PDB; VECTOR,
 *                _svector, _sfvector, MATRIX, FVECTOR, and Plane layouts from
 *                TPI; numeric constants read from the matched executable.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - conversions, float-vs-double precision, tail call, and
 *                exceptional conversion behavior checked at the exact RVAs.
 *
 * PDB module: 0036
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\fmath.obj
 * Primary source: W:\SWJediPowerBattles\Work\fmath.c
 * Compiler language: c
 * Emitted procedures: 49
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/fmath.h"
#include "jpb/vectors.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>

/* Direct local symbol at reference RVA 0x537CB8. */
static MATRIX gte_matrix;

/* Exact PDB global at RVA 0x515C08, owned by the scene/camera boundary. */
extern MATRIX CameraMatrix;

static void fmath_transform_truncated(
    MATRIX *matrix, float x, float y, float z, VECTOR *result);
static void fmath_transform_camera_truncated(
    MATRIX *matrix, float x, float y, float z, VECTOR *result);
static int fmath_project_truncated(
    const VECTOR *transformed, FVECTOR *destination);

/*
 * SSE cvttss2si/cvttsd2si return INT32_MIN for NaN or an out-of-range
 * operand. C's corresponding cast is undefined in those cases, so spell out
 * the reference conversion before using a portable cast.
 */
static int32_t fmath_trunc_double_to_i32(double value)
{
    if (!(value >= -2147483648.0 && value < 2147483648.0)) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int32_t fmath_trunc_float_to_i32(float value)
{
    if (!(value >= -2147483648.0f && value < 2147483648.0f)) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

/* Store the low word exactly as the reference movw instructions do. */
static int16_t fmath_low_i16(int32_t value)
{
    uint16_t low = (uint16_t)(uint32_t)value;

    if (low <= INT16_MAX) {
        return (int16_t)low;
    }
    return (int16_t)((int32_t)low - 65536);
}

static int32_t fmath_i32_from_bits(uint32_t value)
{
    if (value <= INT32_MAX) {
        return (int32_t)value;
    }
    return -(int32_t)(~value) - 1;
}

static int32_t fmath_arithmetic_shift_right(
    uint32_t value, unsigned shift)
{
    shift &= 31U;
    if (shift == 0) {
        return fmath_i32_from_bits(value);
    }
    if ((value & UINT32_C(0x80000000)) == 0) {
        return (int32_t)(value >> shift);
    }
    return fmath_i32_from_bits(
        (value >> shift) |
        ~(UINT32_MAX >> shift));
}

static int32_t fmath_unpack_10bit_component(
    uint32_t packed, unsigned left_shift, unsigned shifter)
{
    uint32_t shifted = packed << left_shift;
    int32_t value =
        fmath_arithmetic_shift_right(shifted, shifter);

    return (int32_t)fmath_low_i16(value);
}

static int16_t fmath_add_low_i16(int16_t a, int32_t b)
{
    uint32_t sum =
        (uint32_t)(uint16_t)a +
        (uint32_t)(uint16_t)b;

    return fmath_low_i16((int32_t)sum);
}

static int32_t fmath_wrap_add_i32(int32_t a, int32_t b)
{
    return fmath_i32_from_bits((uint32_t)a + (uint32_t)b);
}

static void fmath_apply_gte_float(
    float x, float y, float z, FVECTOR *result)
{
    float value;

    value = y * gte_matrix.m[0][1];
    value += x * gte_matrix.m[0][0];
    value += z * gte_matrix.m[0][2];
    result->vx = value;

    value = y * gte_matrix.m[1][1];
    value += x * gte_matrix.m[1][0];
    value += z * gte_matrix.m[1][2];
    result->vy = value;

    value = y * gte_matrix.m[2][1];
    value += x * gte_matrix.m[2][0];
    value += z * gte_matrix.m[2][2];
    result->vz = value;
}

static void fmath_add_gte_translation(FVECTOR *value)
{
    value->vx += (float)gte_matrix.t[0];
    value->vy += (float)gte_matrix.t[1];
    value->vz += (float)gte_matrix.t[2];
}

static int32_t fmath_unpack_shifted_component(
    uint32_t packed, unsigned left_shift, unsigned shifter)
{
    return fmath_arithmetic_shift_right(
        packed << left_shift, shifter);
}

/*
 * Exact RVA 0x9AEF0, 1313 bytes.
 *
 * Input words contain signed ten-bit x/y/z fields at bit positions 0, 10,
 * and 20. The reference shifts each 32-bit word with x86 wrap/sign semantics,
 * rounds every matrix product to float, truncates to a signed low word, then
 * adds the low words of the active GTE translation. The optimized executable
 * writes an indeterminate temporary into _svector.pad; retaining the caller's
 * padding preserves all defined behavior without introducing undefined data.
 */
void ApplyMatrixMany10Bit(
    int *input, _svector *output, int n, int shifter)
{
    int i;
    unsigned shift = (unsigned)shifter & 31U;

    for (i = 0; i < n; ++i) {
        uint32_t packed = (uint32_t)input[i];
        float x = (float)fmath_unpack_10bit_component(
            packed, 22U, shift);
        float y = (float)fmath_unpack_10bit_component(
            packed, 12U, shift);
        float z = (float)fmath_unpack_10bit_component(
            packed, 2U, shift);
        float tx;
        float ty;
        float tz;
        _svector tmp = output[i];

        tx = x * gte_matrix.m[0][0];
        tx += y * gte_matrix.m[0][1];
        tx += z * gte_matrix.m[0][2];
        ty = x * gte_matrix.m[1][0];
        ty += y * gte_matrix.m[1][1];
        ty += z * gte_matrix.m[1][2];
        tz = x * gte_matrix.m[2][0];
        tz += y * gte_matrix.m[2][1];
        tz += z * gte_matrix.m[2][2];

        tmp.vx = fmath_add_low_i16(
            fmath_low_i16(fmath_trunc_float_to_i32(tx)),
            gte_matrix.t[0]);
        tmp.vy = fmath_add_low_i16(
            fmath_low_i16(fmath_trunc_float_to_i32(ty)),
            gte_matrix.t[1]);
        tmp.vz = fmath_add_low_i16(
            fmath_low_i16(fmath_trunc_float_to_i32(tz)),
            gte_matrix.t[2]);
        output[i] = tmp;
    }
}

/*
 * Exact RVA 0x9B420, 1386 bytes.
 *
 * Unlike the short-output path, the three shifted packed components remain
 * full signed 32-bit values before conversion to float. This distinction is
 * observable for nonstandard shifter values and is retained deliberately.
 */
void ApplyMatrixMany10BitFV(
    int *input, FVECTOR *output, int n, int shifter)
{
    unsigned shift = (unsigned)shifter & 31U;
    int i;

    for (i = 0; i < n; ++i) {
        uint32_t packed = (uint32_t)input[i];

        fmath_apply_gte_float(
            (float)fmath_unpack_shifted_component(
                packed, 22U, shift),
            (float)fmath_unpack_shifted_component(
                packed, 12U, shift),
            (float)fmath_unpack_shifted_component(
                packed, 2U, shift),
            &output[i]);
        fmath_add_gte_translation(&output[i]);
    }
}

/* Exact RVA 0x9B990, 1485 bytes. */
void ApplyMatrixMany10BitFVnormalize(
    int *input, FVECTOR *output, int n, int shifter)
{
    const float normalized_scale = 0.000244140625f;
    unsigned shift = (unsigned)shifter & 31U;
    int i;

    for (i = 0; i < n; ++i) {
        uint32_t packed = (uint32_t)input[i];

        fmath_apply_gte_float(
            (float)fmath_unpack_shifted_component(
                packed, 22U, shift) * normalized_scale,
            (float)fmath_unpack_shifted_component(
                packed, 12U, shift) * normalized_scale,
            (float)fmath_unpack_shifted_component(
                packed, 2U, shift) * normalized_scale,
            &output[i]);
        fmath_add_gte_translation(&output[i]);
    }
}

/* Exact RVA 0x9BF60, 1349 bytes. */
void ApplyMatrixMany10BitLong(
    int *input, VECTOR *output, int n, int shifter)
{
    unsigned shift = (unsigned)shifter & 31U;
    int i;

    for (i = 0; i < n; ++i) {
        uint32_t packed = (uint32_t)input[i];
        FVECTOR transformed;

        fmath_apply_gte_float(
            (float)fmath_low_i16(
                fmath_unpack_shifted_component(packed, 22U, shift)),
            (float)fmath_low_i16(
                fmath_unpack_shifted_component(packed, 12U, shift)),
            (float)fmath_low_i16(
                fmath_unpack_shifted_component(packed, 2U, shift)),
            &transformed);
        output[i].vx = fmath_wrap_add_i32(
            (int32_t)fmath_low_i16(
                fmath_trunc_float_to_i32(transformed.vx)),
            gte_matrix.t[0]);
        output[i].vy = fmath_wrap_add_i32(
            (int32_t)fmath_low_i16(
                fmath_trunc_float_to_i32(transformed.vy)),
            gte_matrix.t[1]);
        output[i].vz = fmath_wrap_add_i32(
            (int32_t)fmath_low_i16(
                fmath_trunc_float_to_i32(transformed.vz)),
            gte_matrix.t[2]);
    }
}

/* Exact RVA 0x9C4B0, 1353 bytes. `stride` is measured in input ints. */
void ApplyMatrixMany10BitStride(
    int *input,
    _svector *output,
    int n,
    int shifter,
    int stride)
{
    unsigned shift = (unsigned)shifter & 31U;
    int i;

    for (i = 0; i < n; ++i, input += stride) {
        uint32_t packed = (uint32_t)*input;
        FVECTOR transformed;

        fmath_apply_gte_float(
            (float)fmath_low_i16(
                fmath_unpack_shifted_component(packed, 22U, shift)),
            (float)fmath_low_i16(
                fmath_unpack_shifted_component(packed, 12U, shift)),
            (float)fmath_low_i16(
                fmath_unpack_shifted_component(packed, 2U, shift)),
            &transformed);
        output[i].vx = fmath_add_low_i16(
            fmath_low_i16(fmath_trunc_float_to_i32(transformed.vx)),
            gte_matrix.t[0]);
        output[i].vy = fmath_add_low_i16(
            fmath_low_i16(fmath_trunc_float_to_i32(transformed.vy)),
            gte_matrix.t[1]);
        output[i].vz = fmath_add_low_i16(
            fmath_low_i16(fmath_trunc_float_to_i32(transformed.vz)),
            gte_matrix.t[2]);
    }
}

/* Exact RVA 0x9CA00, 1195 bytes. */
void ApplyMatrixManyFV(FVECTOR *input, FVECTOR *output, int n)
{
    int i;

    for (i = 0; i < n; ++i) {
        float x = input[i].vx;
        float y = input[i].vy;
        float z = input[i].vz;

        fmath_apply_gte_float(x, y, z, &output[i]);
        fmath_add_gte_translation(&output[i]);
    }
}

/* Exact RVA 0x9CEB0, 1232 bytes. */
void ApplyMatrixManySV(
    _svector *input, _svector *output, int n)
{
    int i;

    for (i = 0; i < n; ++i) {
        float x = (float)input[i].vx;
        float y = (float)input[i].vy;
        float z = (float)input[i].vz;
        FVECTOR transformed;

        fmath_apply_gte_float(x, y, z, &transformed);
        output[i].vx = fmath_add_low_i16(
            fmath_low_i16(fmath_trunc_float_to_i32(transformed.vx)),
            gte_matrix.t[0]);
        output[i].vy = fmath_add_low_i16(
            fmath_low_i16(fmath_trunc_float_to_i32(transformed.vy)),
            gte_matrix.t[1]);
        output[i].vz = fmath_add_low_i16(
            fmath_low_i16(fmath_trunc_float_to_i32(transformed.vz)),
            gte_matrix.t[2]);
    }
}

static float fmath_fixed_angle_to_radians(int angle)
{
    double radians = (double)angle;

    radians *= 0.000244140625;
    radians *= 360.0;
    radians /= 57.2957;
    return (float)radians;
}

static float fmath_matrix_product_element(
    MATRIX *m0, MATRIX *m1, int row, int column)
{
    float value = m1->m[0][column] * m0->m[row][0];

    value += m1->m[1][column] * m0->m[row][1];
    value += m1->m[2][column] * m0->m[row][2];
    return value;
}

/* Reference RVA 0x9E120, 48 bytes. */
void SetGTETransLV(VECTOR *t)
{
    if (t != NULL) {
        gte_matrix.t[0] = t->vx;
        gte_matrix.t[1] = t->vy;
        gte_matrix.t[2] = t->vz;
    } else {
        gte_matrix.t[0] = 0;
        gte_matrix.t[1] = 0;
        gte_matrix.t[2] = 0;
    }
}

/* Reference RVA 0x9E150, 38 bytes. */
void SetTransformMatrix(MATRIX *m)
{
    if (m != NULL) {
        gte_matrix = *m;
    }
}

/*
 * Exact RVA 0x9E210, 1631 bytes.
 *
 * The packed result stores truncated screen y in the high word and screen x
 * in the low word. CameraMatrix is the PDB global consumed directly by the
 * retail body; this routine does not read the module-local GTE matrix.
 */
int TransformPoints(_svector *points, int *results, int n)
{
    const float minimum_depth = 0.00009765625f;
    int any_too_near = 0;
    int i;

    for (i = 0; i < n; ++i) {
        VECTOR transformed;
        FVECTOR projected;
        int32_t screen_x;
        int32_t screen_y;
        uint32_t packed;

        fmath_transform_camera_truncated(
            &CameraMatrix,
            (float)points[i].vx,
            (float)points[i].vy,
            (float)points[i].vz,
            &transformed);
        (void)fmath_project_truncated(&transformed, &projected);
        screen_x = fmath_trunc_float_to_i32(projected.vx);
        screen_y = fmath_trunc_float_to_i32(projected.vy);
        packed =
            ((uint32_t)screen_y << 16) |
            ((uint32_t)screen_x & UINT32_C(0xffff));
        results[i] = fmath_i32_from_bits(packed);
        if (projected.vz < minimum_depth) {
            any_too_near = 1;
        }
    }
    return any_too_near;
}

/* Exact RVA 0x9E870, 336 bytes. */
int TransformPointsFV(
    _svector *points, FVECTOR *results, int n)
{
    int i;

    for (i = 0; i < n; ++i) {
        VECTOR transformed;

        fmath_transform_camera_truncated(
            &CameraMatrix,
            (float)points[i].vx,
            (float)points[i].vy,
            (float)points[i].vz,
            &transformed);
        results[i].vx = (float)transformed.vx;
        results[i].vy = (float)transformed.vy;
        results[i].vz =
            transformed.vz > 1 ? (float)transformed.vz : 1.0f;
    }
    return 0;
}

#ifdef JPB_FMATH_TESTING
const MATRIX *jpb_fmath_test_transform_matrix(void)
{
    return &gte_matrix;
}
#endif

/*
 * Reference RVA 0x9D380, 114 bytes.
 *
 * The reference rounds rad to float, then promotes that rounded value back to
 * double for the CRT sin/cos calls. This differs slightly from rsin/rcos.
 */
void FindSinCos(int a, float *s, float *c)
{
    double rad_double = (double)a;
    float rad;

    rad_double *= 0.000244140625;
    rad_double *= 360.0;
    rad_double /= 57.2957;
    rad = (float)rad_double;

    *s = (float)sin((double)rad);
    *c = (float)cos((double)rad);
}

static void fmath_transform_truncated(
    MATRIX *matrix, float x, float y, float z, VECTOR *result)
{
    float value;

    value = y * matrix->m[0][1];
    value += x * matrix->m[0][0];
    value += z * matrix->m[0][2];
    result->vx = fmath_wrap_add_i32(
        fmath_trunc_float_to_i32(value), matrix->t[0]);

    value = x * matrix->m[1][0];
    value += y * matrix->m[1][1];
    value += z * matrix->m[1][2];
    result->vy = fmath_wrap_add_i32(
        fmath_trunc_float_to_i32(value), matrix->t[1]);

    value = x * matrix->m[2][0];
    value += y * matrix->m[2][1];
    value += z * matrix->m[2][2];
    result->vz = fmath_wrap_add_i32(
        fmath_trunc_float_to_i32(value), matrix->t[2]);
}

static void fmath_transform_camera_truncated(
    MATRIX *matrix, float x, float y, float z, VECTOR *result)
{
    float value;

    value = x * matrix->m[0][0];
    value += y * matrix->m[0][1];
    value += z * matrix->m[0][2];
    result->vx = fmath_wrap_add_i32(
        fmath_trunc_float_to_i32(value), matrix->t[0]);

    value = y * matrix->m[1][1];
    value += x * matrix->m[1][0];
    value += z * matrix->m[1][2];
    result->vy = fmath_wrap_add_i32(
        fmath_trunc_float_to_i32(value), matrix->t[1]);

    value = y * matrix->m[2][1];
    value += x * matrix->m[2][0];
    value += z * matrix->m[2][2];
    result->vz = fmath_wrap_add_i32(
        fmath_trunc_float_to_i32(value), matrix->t[2]);
}

static int fmath_project_truncated(
    const VECTOR *transformed, FVECTOR *destination)
{
    const float near_clip = 1.0f;
    const float projection_scale = 460.0f;
    const float screen_center_x = 320.0f;
    const float screen_center_y = 240.0f;
    const float depth_scale = 10240.0f;
    float depth = (float)transformed->vz;
    float projection;
    int clipped = depth <= near_clip;

    if (clipped) {
        depth = near_clip;
    }
    projection = projection_scale / depth;
    destination->vx =
        (float)transformed->vx * projection + screen_center_x;
    destination->vy =
        (float)transformed->vy * projection + screen_center_y;
    destination->vz = depth / depth_scale;
    return clipped;
}

/*
 * Exact RVA 0x9D400, 318 bytes. Despite its name this legacy entry only
 * applies the integer-truncating transform and clamps camera-space depth.
 */
int PerspectiveTransform(
    MATRIX *matrix, _svector *source, FVECTOR *destination)
{
    VECTOR transformed;

    fmath_transform_truncated(
        matrix,
        (float)source->vx,
        (float)source->vy,
        (float)source->vz,
        &transformed);
    destination->vx = (float)transformed.vx;
    destination->vy = (float)transformed.vy;
    destination->vz =
        transformed.vz > 1 ? (float)transformed.vz : 1.0f;
    return 0;
}

/*
 * Reference RVA 0x9D540, 181 bytes.
 *
 * The five constants are direct executable data at RVAs 0x45F938,
 * 0x29C200, 0x29B7B8, 0x335710, and 0x335714 respectively. The comparison
 * is deliberately written so NaN follows the reference COMISS/JB path:
 * it is not reported as near-clipped and remains NaN through projection.
 */
int PerspectiveTransformFV(
    MATRIX *matrix, FVECTOR *source, FVECTOR *destination)
{
    const float near_clip = 1.0f;
    const float projection_scale = 460.0f;
    const float screen_center_x = 320.0f;
    const float screen_center_y = 240.0f;
    const float depth_scale = 10240.0f;
    FVECTOR transformed;
    float depth;
    float projection;
    int clipped;

    fApplyMatrixFV(matrix, source, &transformed);
    transformed.vx += (float)matrix->t[0];
    transformed.vy += (float)matrix->t[1];
    depth = transformed.vz + (float)matrix->t[2];
    clipped = depth <= near_clip;
    if (clipped) {
        depth = near_clip;
    }
    projection = projection_scale / depth;
    destination->vx =
        transformed.vx * projection + screen_center_x;
    destination->vy =
        transformed.vy * projection + screen_center_y;
    destination->vz = depth / depth_scale;
    return clipped;
}

/* Exact RVA 0x9D600, 385 bytes. */
int PerspectiveTransformLV(
    MATRIX *matrix, VECTOR *source, FVECTOR *destination)
{
    VECTOR transformed;

    fmath_transform_truncated(
        matrix,
        (float)source->vx,
        (float)source->vy,
        (float)source->vz,
        &transformed);
    return fmath_project_truncated(&transformed, destination);
}

/* Reference RVA 0x9D790, 344 bytes. */
int PerspectiveTransformManyFV(
    MATRIX *matrix, FVECTOR *source, FVECTOR *destination, int count)
{
    int clipped_count = 0;
    int index;

    for (index = 0; index < count; ++index) {
        clipped_count +=
            PerspectiveTransformFV(
                matrix, &source[index], &destination[index]);
    }
    return clipped_count;
}

/* Exact RVA 0x9D8F0, 384 bytes. This compatibility path always returns 0. */
int PerspectiveTransformOLD(
    MATRIX *matrix, _svector *source, FVECTOR *destination)
{
    VECTOR transformed;

    fmath_transform_truncated(
        matrix,
        (float)source->vx,
        (float)source->vy,
        (float)source->vz,
        &transformed);
    (void)fmath_project_truncated(&transformed, destination);
    return 0;
}

/* Reference RVA 0x9E180, 51 bytes. */
int SquareRoot0(int a)
{
    if (a < 0) {
        return INT32_MIN;
    }
    return fmath_trunc_double_to_i32(sqrt((double)a));
}

/* Reference RVA 0x9E1C0, 67 bytes. */
int SquareRoot12(int a)
{
    if (a < 0) {
        return INT32_MIN;
    }
    return fmath_trunc_double_to_i32(sqrt((double)a) * 4096.0);
}

/* Reference RVA 0x9E9C0, 170 bytes. */
int VectorNormal(VECTOR *v0, VECTOR *v1)
{
    uint32_t raw_length = vec_LengthLV(v0);
    float length = (float)raw_length;
    float x = (float)v0->vx * 4096.0f;
    float y = (float)v0->vy * 4096.0f;
    float z = (float)v0->vz * 4096.0f;

    if (length != 0.0f) {
        x /= length;
        y /= length;
        z /= length;
    }

    v1->vx = fmath_trunc_float_to_i32(x);
    v1->vy = fmath_trunc_float_to_i32(y);
    v1->vz = fmath_trunc_float_to_i32(z);
    return fmath_trunc_float_to_i32(length);
}

/* Reference RVA 0x9EA70, 175 bytes. */
int VectorNormalS(VECTOR *v0, _svector *v1)
{
    uint32_t raw_length = vec_LengthLV(v0);
    float length = (float)raw_length;
    float x = (float)v0->vx * 4096.0f;
    float y = (float)v0->vy * 4096.0f;
    float z = (float)v0->vz * 4096.0f;

    if (length != 0.0f) {
        x /= length;
        y /= length;
        z /= length;
    }

    v1->vx = fmath_low_i16(fmath_trunc_float_to_i32(x));
    v1->vy = fmath_low_i16(fmath_trunc_float_to_i32(y));
    v1->vz = fmath_low_i16(fmath_trunc_float_to_i32(z));
    return fmath_trunc_float_to_i32(length);
}

/* Reference RVA 0x9FBA0, 251 bytes. */
int normalize(int a, int b, int c, _svector *d)
{
    VECTOR v0 = {a, b, c, 0};
    int32_t length =
        fmath_trunc_float_to_i32((float)vec_LengthLV(&v0));

    if (d != NULL) {
        double x = (double)a * 4096.0;
        double y = (double)b * 4096.0;
        double z = (double)c * 4096.0;

        if (length != 0) {
            x /= (double)length;
            y /= (double)length;
            z /= (double)length;
        }

        d->vx = fmath_low_i16(fmath_trunc_double_to_i32(x));
        d->vy = fmath_low_i16(fmath_trunc_double_to_i32(y));
        d->vz = fmath_low_i16(fmath_trunc_double_to_i32(z));
    }

    return length;
}

/* Reference RVA 0x9FCA0, 245 bytes. */
int normalize_l(int x, int y, int z, VECTOR *r)
{
    VECTOR bob = {x, y, z, 0};
    uint32_t raw_length = vec_LengthLV(&bob);
    float length = (float)raw_length;
    float out_x = (float)x * 4096.0f;
    float out_y = (float)y * 4096.0f;
    float out_z = (float)z * 4096.0f;

    if (length != 0.0f) {
        out_x /= length;
        out_y /= length;
        out_z /= length;
    }

    bob.vx = fmath_trunc_float_to_i32(out_x);
    bob.vy = fmath_trunc_float_to_i32(out_y);
    bob.vz = fmath_trunc_float_to_i32(out_z);

    if (r != NULL) {
        *r = bob;
    }

    return fmath_trunc_float_to_i32(length);
}

/* Reference RVA 0x9FDA0, 5 bytes: an exact tail jump to VectorNormal. */
int normalize_lvector(VECTOR *a, VECTOR *b)
{
    return VectorNormal(a, b);
}

/* Reference RVA 0x9FDB0, 42 bytes. */
int ratan2(int y, int x)
{
    return fmath_trunc_double_to_i32(
        atan2((double)y, (double)x) * 651.8986206054688);
}

/* Reference RVA 0x9FDE0, 58 bytes. */
int rcos(int a)
{
    double angle = (double)a;

    angle *= 0.000244140625;
    angle *= 360.0;
    angle /= 57.2957;
    return fmath_trunc_double_to_i32(cos(angle) * 4096.0);
}

/* Reference RVA 0x9FE20, 58 bytes. */
int rsin(int a)
{
    double angle = (double)a;

    angle *= 0.000244140625;
    angle *= 360.0;
    angle /= 57.2957;
    return fmath_trunc_double_to_i32(sin(angle) * 4096.0);
}

/* Reference RVA 0x9EB20, 126 bytes. */
void XRotMatrix(MATRIX *m, float angle)
{
    float radians = angle * 0.00153398083f;
    float ca = (float)cos((double)radians);
    float sa = (float)sin((double)radians);

    m->m[0][0] = 1.0f;
    m->m[0][1] = 0.0f;
    m->m[0][2] = 0.0f;
    m->m[1][0] = 0.0f;
    m->m[1][1] = ca;
    m->m[1][2] = -sa;
    m->m[2][0] = 0.0f;
    m->m[2][1] = sa;
    m->m[2][2] = ca;
}

/* Reference RVA 0x9EBA0, 125 bytes. */
void YRotMatrix(MATRIX *m, float angle)
{
    float radians = angle * 0.00153398083f;
    float ca = (float)cos((double)radians);
    float sa = (float)sin((double)radians);

    m->m[0][0] = ca;
    m->m[0][1] = 0.0f;
    m->m[0][2] = sa;
    m->m[1][0] = 0.0f;
    m->m[1][1] = 1.0f;
    m->m[1][2] = 0.0f;
    m->m[2][0] = -sa;
    m->m[2][1] = 0.0f;
    m->m[2][2] = ca;
}

/* Reference RVA 0x9EC20, 128 bytes. */
void ZRotMatrix(MATRIX *m, float angle)
{
    float radians = angle * 0.00153398083f;
    float ca = (float)cos((double)radians);
    float sa = (float)sin((double)radians);

    m->m[0][0] = ca;
    m->m[0][1] = -sa;
    m->m[0][2] = 0.0f;
    m->m[1][0] = sa;
    m->m[1][1] = ca;
    m->m[1][2] = 0.0f;
    m->m[2][0] = 0.0f;
    m->m[2][1] = 0.0f;
    m->m[2][2] = 1.0f;
}

/* Reference RVA 0x9ECA0, 191 bytes. */
VECTOR *fApplyMatrix(MATRIX *m, _svector *v0, VECTOR *v1)
{
    float value = (float)v0->vy * m->m[0][1];

    value += (float)v0->vx * m->m[0][0];
    value += (float)v0->vz * m->m[0][2];
    v1->vx = fmath_trunc_float_to_i32(value);

    value = (float)v0->vx * m->m[1][0];
    value += (float)v0->vy * m->m[1][1];
    value += (float)v0->vz * m->m[1][2];
    v1->vy = fmath_trunc_float_to_i32(value);

    value = (float)v0->vx * m->m[2][0];
    value += (float)v0->vy * m->m[2][1];
    value += (float)v0->vz * m->m[2][2];
    v1->vz = fmath_trunc_float_to_i32(value);
    return v1;
}

/* Reference RVA 0x9EFB0, 173 bytes. */
_svector *fApplyMatrixSV(
    MATRIX *m, _svector *v0, _svector *v1)
{
    VECTOR result;

    (void)fApplyMatrix(m, v0, &result);
    v1->vx = (int16_t)result.vx;
    v1->vy = (int16_t)result.vy;
    v1->vz = (int16_t)result.vz;
    return v1;
}

/* Reference RVA 0x9EF10, 154 bytes. */
_sfvector *fApplyMatrixSFV(
    MATRIX *m, _sfvector *v0, _sfvector *v1)
{
    _sfvector result = *v1;

    result.vx =
        v0->vy * m->m[0][1] +
        v0->vx * m->m[0][0] +
        v0->vz * m->m[0][2];
    result.vy =
        v0->vx * m->m[1][0] +
        v0->vy * m->m[1][1] +
        v0->vz * m->m[1][2];
    result.vz =
        v0->vx * m->m[2][0] +
        v0->vy * m->m[2][1] +
        v0->vz * m->m[2][2];
    /*
     * The optimized reference writes an indeterminate temporary into pad.
     * It has no semantic consumers. Retaining the destination's existing
     * padding avoids introducing undefined behavior into the portable form.
     */
    *v1 = result;
    return v1;
}

/* Reference RVA 0x9ED60, 242 bytes. */
FVECTOR *fApplyMatrixFV(MATRIX *m, FVECTOR *v0, FVECTOR *v1)
{
    FVECTOR tmp;

    tmp.vx = v0->vy * m->m[0][1];
    tmp.vx += v0->vx * m->m[0][0];
    tmp.vx += v0->vz * m->m[0][2];

    tmp.vy = v0->vy * m->m[1][1];
    tmp.vy += v0->vx * m->m[1][0];
    tmp.vy += v0->vz * m->m[1][2];

    tmp.vz = v0->vx * m->m[2][0];
    tmp.vz += v0->vy * m->m[2][1];
    tmp.vz += v0->vz * m->m[2][2];

    *v1 = tmp;
    return v1;
}

/* Reference RVA 0x9EE60, 164 bytes. */
VECTOR *fApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1)
{
    float value = (float)v0->vy * m->m[0][1];

    value += (float)v0->vx * m->m[0][0];
    value += (float)v0->vz * m->m[0][2];
    v1->vx = fmath_trunc_float_to_i32(value);

    value = (float)v0->vx * m->m[1][0];
    value += (float)v0->vy * m->m[1][1];
    value += (float)v0->vz * m->m[1][2];
    v1->vy = fmath_trunc_float_to_i32(value);

    value = (float)v0->vx * m->m[2][0];
    value += (float)v0->vy * m->m[2][1];
    value += (float)v0->vz * m->m[2][2];
    v1->vz = fmath_trunc_float_to_i32(value);
    return v1;
}

/* Reference RVA 0x9F060, 430 bytes. */
MATRIX *fMulMatrix(MATRIX *m0, MATRIX *m1)
{
    MATRIX result;
    int row;
    int column;

    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            result.m[row][column] =
                fmath_matrix_product_element(m0, m1, row, column);
        }
    }
    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            m0->m[row][column] = result.m[row][column];
        }
    }
    return m0;
}

/* Reference RVA 0x9F210, 162 bytes. */
MATRIX *fMulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *r)
{
    int row;
    int column;

    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            r->m[row][column] =
                fmath_matrix_product_element(m0, m1, row, column);
        }
    }
    return r;
}

/* Reference RVA 0x9F330, 256 bytes. */
MATRIX *fRotMatrixX(int a, MATRIX *m)
{
    float radians = fmath_fixed_angle_to_radians(a);
    float s = (float)sin((double)radians);
    float c = (float)cos((double)radians);
    int column;

    for (column = 0; column < 3; ++column) {
        float row1 = m->m[1][column];
        float row2 = m->m[2][column];

        m->m[2][column] = row2 * c + row1 * s;
        m->m[1][column] = row1 * c - row2 * s;
    }
    return m;
}

/* Reference RVA 0x9F430, 254 bytes. */
MATRIX *fRotMatrixY(int a, MATRIX *m)
{
    float radians = fmath_fixed_angle_to_radians(a);
    float s = (float)sin((double)radians);
    float c = (float)cos((double)radians);
    int column;

    for (column = 0; column < 3; ++column) {
        float row0 = m->m[0][column];
        float row2 = m->m[2][column];

        m->m[2][column] = row2 * c - row0 * s;
        m->m[0][column] = row2 * s + row0 * c;
    }
    return m;
}

/* Reference RVA 0x9F530, 254 bytes. */
MATRIX *fRotMatrixZ(int a, MATRIX *m)
{
    float radians = fmath_fixed_angle_to_radians(a);
    float s = (float)sin((double)radians);
    float c = (float)cos((double)radians);
    int column;

    for (column = 0; column < 3; ++column) {
        float row0 = m->m[0][column];
        float row1 = m->m[1][column];

        m->m[1][column] = row1 * c + row0 * s;
        m->m[0][column] = row0 * c - row1 * s;
    }
    return m;
}

/* Reference RVA 0x9F2C0, 99 bytes. */
MATRIX *fRotMatrix(_svector *r, MATRIX *m)
{
    vec_IdentMatrix(m);
    fRotMatrixZ((int)r->vz, m);
    fRotMatrixY((int)r->vy, m);
    fRotMatrixX((int)r->vx, m);
    return m;
}

/* Reference RVA 0x9F630, 99 bytes. */
MATRIX *fRotMatrixZYX(_svector *r, MATRIX *m)
{
    vec_IdentMatrix(m);
    fRotMatrixX((int)r->vx, m);
    fRotMatrixY((int)r->vy, m);
    fRotMatrixZ((int)r->vz, m);
    return m;
}

static void fmath_rot_trans_float(
    MATRIX *matrix, float x, float y, float z, FVECTOR *destination)
{
    float value;

    value = y * matrix->m[0][1];
    value += x * matrix->m[0][0];
    value += z * matrix->m[0][2];
    value += (float)matrix->t[0];
    destination->vx = value;

    value = x * matrix->m[1][0];
    value += y * matrix->m[1][1];
    value += z * matrix->m[1][2];
    value += (float)matrix->t[1];
    destination->vy = value;

    value = y * matrix->m[2][1];
    value += x * matrix->m[2][0];
    value += z * matrix->m[2][2];
    value += (float)matrix->t[2];
    destination->vz = value;
}

/* Exact RVA 0x9DA70, 856 bytes. */
int RotTransPersFloat(
    MATRIX *matrix, FVECTOR *input, FVECTOR *output, int n)
{
    int i;

    for (i = 0; i < n; ++i) {
        float x = input[i].vx;
        float y = input[i].vy;
        float z = input[i].vz;

        fmath_rot_trans_float(matrix, x, y, z, &output[i]);
    }
    return 0;
}

/* Exact RVA 0x9DDD0, 846 bytes. */
int RotTransPersSFV(
    MATRIX *matrix, _sfvector *input, FVECTOR *output, int n)
{
    int i;

    for (i = 0; i < n; ++i) {
        float x = input[i].vx;
        float y = input[i].vy;
        float z = input[i].vz;

        fmath_rot_trans_float(matrix, x, y, z, &output[i]);
    }
    return 0;
}

/*
 * Reference RVA 0x9F6A0, 944 bytes.
 *
 * The optimized x64 body handles four records per iteration before its
 * scalar tail. Each _svector uses its exact eight-byte stride and each
 * FVECTOR uses its exact twelve-byte stride. The result is camera-space;
 * projection remains the renderer's following operation despite the legacy
 * function name.
 */
int fRotTransPers(
    MATRIX *matrix,
    _svector *source,
    FVECTOR *destination,
    int count)
{
    int index;

    for (index = 0; index < count; ++index) {
        float x = (float)source[index].vx;
        float y = (float)source[index].vy;
        float z = (float)source[index].vz;

        destination[index].vx =
            x * matrix->m[0][0] +
            y * matrix->m[0][1] +
            z * matrix->m[0][2] +
            (float)matrix->t[0];
        destination[index].vy =
            x * matrix->m[1][0] +
            y * matrix->m[1][1] +
            z * matrix->m[1][2] +
            (float)matrix->t[1];
        destination[index].vz =
            x * matrix->m[2][0] +
            y * matrix->m[2][1] +
            z * matrix->m[2][2] +
            (float)matrix->t[2];
    }
    return 0;
}

/* Reference RVA 0x9FA50, 150 bytes. */
MATRIX *fScaleMatrix(MATRIX *m, VECTOR *v)
{
    float x = (float)v->vx * 0.000244140625f;
    float y = (float)v->vy * 0.000244140625f;
    float z = (float)v->vz * 0.000244140625f;
    int row;

    for (row = 0; row < 3; ++row) {
        m->m[row][0] *= x;
        m->m[row][1] *= y;
        m->m[row][2] *= z;
    }
    return m;
}

/* Reference RVA 0x9FAF0, 21 bytes. */
MATRIX *fTransMatrix(MATRIX *m, VECTOR *v)
{
    m->t[0] = v->vx;
    m->t[1] = v->vy;
    m->t[2] = v->vz;
    return m;
}

/* Reference RVA 0x9FB10, 65 bytes. */
MATRIX *fTransposeMatrix(MATRIX *m0, MATRIX *m1)
{
    m1->m[0][0] = m0->m[0][0];
    m1->m[0][1] = m0->m[1][0];
    m1->m[0][2] = m0->m[2][0];
    m1->m[1][0] = m0->m[0][1];
    m1->m[1][1] = m0->m[1][1];
    m1->m[1][2] = m0->m[2][1];
    m1->m[2][0] = m0->m[0][2];
    m1->m[2][1] = m0->m[1][2];
    m1->m[2][2] = m0->m[2][2];
    m1->t[0] = 0;
    m1->t[1] = 0;
    m1->t[2] = 0;
    return m1;
}

/* Reference RVA 0x9FB60, 59 bytes. */
float getscreenz(MATRIX *m, VECTOR *v)
{
    float result = (float)v->vy * m->m[2][1];

    result += (float)v->vx * m->m[2][0];
    result += (float)v->vz * m->m[2][2];
    result += (float)m->t[2];
    return result;
}

/* 0x9B420, 1386 bytes, global, 8 named locals
 * ApplyMatrixMany10BitFV
 * PDB type: void (int*, FVECTOR*, int, int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9B990, 1485 bytes, global, 8 named locals
 * ApplyMatrixMany10BitFVnormalize
 * PDB type: void (int*, FVECTOR*, int, int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9BF60, 1349 bytes, global, 7 named locals
 * ApplyMatrixMany10BitLong
 * PDB type: void (int*, VECTOR*, int, int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9C4B0, 1353 bytes, global, 8 named locals
 * ApplyMatrixMany10BitStride
 * PDB type: void (int*, _svector*, int, int,...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9CA00, 1195 bytes, global, 5 named locals
 * ApplyMatrixManyFV
 * PDB type: void (FVECTOR*, FVECTOR*, int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9CEB0, 1232 bytes, global, 5 named locals
 * ApplyMatrixManySV
 * PDB type: void (_svector*, _svector*, int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9D380, 114 bytes, global, 4 named locals
 * FindSinCos
 * PDB type: void (int, float*, float*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9D400, 318 bytes, global, 4 named locals
 * PerspectiveTransform
 * PDB type: int (MATRIX*, _svector*, FVECTOR...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9D540, 181 bytes, global, 6 named locals
 * PerspectiveTransformFV
 * PDB type: int (MATRIX*, FVECTOR*, FVECTOR*...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9D600, 385 bytes, global, 6 named locals
 * PerspectiveTransformLV
 * PDB type: int (MATRIX*, VECTOR*, FVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9D790, 344 bytes, global, 8 named locals
 * PerspectiveTransformManyFV
 * PDB type: int (MATRIX*, FVECTOR*, FVECTOR*...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9D8F0, 384 bytes, global, 5 named locals
 * PerspectiveTransformOLD
 * PDB type: int (MATRIX*, _svector*, FVECTOR...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9DA70, 856 bytes, global, 14 named locals
 * RotTransPersFloat
 * PDB type: int (MATRIX*, FVECTOR*, FVECTOR*...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9DDD0, 846 bytes, global, 14 named locals
 * RotTransPersSFV
 * PDB type: int (MATRIX*, _sfvector*, FVECTO...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9E120, 48 bytes, global, 1 named locals
 * SetGTETransLV
 * PDB type: void (VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9E150, 38 bytes, global, 1 named locals
 * SetTransformMatrix
 * PDB type: void (MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9E180, 51 bytes, global, 1 named locals
 * SquareRoot0
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9E1C0, 67 bytes, global, 1 named locals
 * SquareRoot12
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9E210, 1631 bytes, global, 6 named locals
 * TransformPoints
 * PDB type: int (_svector*, int*, int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9E870, 336 bytes, global, 4 named locals
 * TransformPointsFV
 * PDB type: int (_svector*, FVECTOR*, int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9E9C0, 170 bytes, global, 3 named locals
 * VectorNormal
 * PDB type: int (VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9EA70, 175 bytes, global, 3 named locals
 * VectorNormalS
 * PDB type: int (VECTOR*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9EB20, 126 bytes, global, 4 named locals
 * XRotMatrix
 * PDB type: void (MATRIX*, float)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9EBA0, 125 bytes, global, 4 named locals
 * YRotMatrix
 * PDB type: void (MATRIX*, float)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9EC20, 128 bytes, global, 4 named locals
 * ZRotMatrix
 * PDB type: void (MATRIX*, float)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9ECA0, 191 bytes, global, 3 named locals
 * fApplyMatrix
 * PDB type: VECTOR* (MATRIX*, _svector*, VEC...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9ED60, 242 bytes, global, 4 named locals
 * fApplyMatrixFV
 * PDB type: FVECTOR* (MATRIX*, FVECTOR*, FVE...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9EE60, 164 bytes, global, 3 named locals
 * fApplyMatrixLV
 * PDB type: VECTOR* (MATRIX*, VECTOR*, VECTO...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9EF10, 154 bytes, global, 4 named locals
 * fApplyMatrixSFV
 * PDB type: _sfvector* (MATRIX*, _sfvector*,...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9EFB0, 173 bytes, global, 4 named locals
 * fApplyMatrixSV
 * PDB type: _svector* (MATRIX*, _svector*, _...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9F060, 430 bytes, global, 3 named locals
 * fMulMatrix
 * PDB type: MATRIX* (MATRIX*, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9F210, 162 bytes, global, 3 named locals
 * fMulMatrix0
 * PDB type: MATRIX* (MATRIX*, MATRIX*, MATRI...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9F2C0, 99 bytes, global, 2 named locals
 * fRotMatrix
 * PDB type: MATRIX* (_svector*, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9F330, 256 bytes, global, 8 named locals
 * fRotMatrixX
 * PDB type: MATRIX* (int, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9F430, 254 bytes, global, 8 named locals
 * fRotMatrixY
 * PDB type: MATRIX* (int, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9F530, 254 bytes, global, 8 named locals
 * fRotMatrixZ
 * PDB type: MATRIX* (int, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9F630, 99 bytes, global, 2 named locals
 * fRotMatrixZYX
 * PDB type: MATRIX* (_svector*, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9F6A0, 944 bytes, global, 14 named locals
 * fRotTransPers
 * PDB type: int (MATRIX*, _svector*, FVECTOR...
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9FA50, 150 bytes, global, 5 named locals
 * fScaleMatrix
 * PDB type: MATRIX* (MATRIX*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9FAF0, 21 bytes, global, 2 named locals
 * fTransMatrix
 * PDB type: MATRIX* (MATRIX*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9FB10, 65 bytes, global, 2 named locals
 * fTransposeMatrix
 * PDB type: MATRIX* (MATRIX*, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9FB60, 59 bytes, global, 2 named locals
 * getscreenz
 * PDB type: float (MATRIX*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9FBA0, 251 bytes, global, 6 named locals
 * normalize
 * PDB type: int (int, int, int, _svector*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9FCA0, 245 bytes, global, 7 named locals
 * normalize_l
 * PDB type: int (int, int, int, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9FDA0, 5 bytes, global, 2 named locals
 * normalize_lvector
 * PDB type: int (VECTOR*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9FDB0, 42 bytes, global, 2 named locals
 * ratan2
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9FDE0, 58 bytes, global, 3 named locals
 * rcos
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */

/* 0x9FE20, 58 bytes, global, 3 named locals
 * rsin
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\fmath.c
 */
