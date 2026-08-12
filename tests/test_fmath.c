#include "jpb/fmath.h"
#include "jpb/flex.h"
#include "jpb/scene.h"
#include "jpb/vectors.h"
#include "jpb/wrender.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#define CHECK_NEAR(actual, expected, tolerance)                              \
    CHECK(fabsf((actual) - (expected)) <= (tolerance))

static int32_t pack_10bit_vector(int x, int y, int z)
{
    return
        (int32_t)(
            ((uint32_t)x & UINT32_C(0x3ff)) |
            (((uint32_t)y & UINT32_C(0x3ff)) << 10) |
            (((uint32_t)z & UINT32_C(0x3ff)) << 20));
}

static int test_apply_matrix_many_10bit(void)
{
    MATRIX matrix = {
        {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}
        },
        {0, 0, 0}
    };
    VECTOR translation = {10, -20, 30, 0};
    int input[5] = {
        pack_10bit_vector(1, 2, 3),
        pack_10bit_vector(-1, -2, -3),
        pack_10bit_vector(511, -512, 0),
        pack_10bit_vector(-300, 400, -500),
        pack_10bit_vector(12, 34, 56)
    };
    _svector output[5] = {
        {0, 0, 0, 0x5a5a},
        {0, 0, 0, 0x5a5a},
        {0, 0, 0, 0x5a5a},
        {0, 0, 0, 0x5a5a},
        {0, 0, 0, 0x5a5a}
    };
    int index;

    SetTransformMatrix(&matrix);
    SetGTETransLV(&translation);
    ApplyMatrixMany10Bit(input, output, 5, 22);

    CHECK(output[0].vx == 11);
    CHECK(output[0].vy == -18);
    CHECK(output[0].vz == 33);
    CHECK(output[1].vx == 9);
    CHECK(output[1].vy == -22);
    CHECK(output[1].vz == 27);
    CHECK(output[2].vx == 521);
    CHECK(output[2].vy == -532);
    CHECK(output[2].vz == 30);
    CHECK(output[3].vx == -290);
    CHECK(output[3].vy == 380);
    CHECK(output[3].vz == -470);
    CHECK(output[4].vx == 22);
    CHECK(output[4].vy == 14);
    CHECK(output[4].vz == 86);
    for (index = 0; index < 5; ++index) {
        CHECK(output[index].pad == (int16_t)0x5a5a);
    }

    matrix.m[0][0] = 0.5f;
    matrix.m[0][1] = -2.0f;
    matrix.m[0][2] = 0.25f;
    matrix.m[1][0] = -1.5f;
    matrix.m[1][1] = 0.25f;
    matrix.m[1][2] = 2.0f;
    matrix.m[2][0] = 1.0f;
    matrix.m[2][1] = 1.0f;
    matrix.m[2][2] = 1.0f;
    matrix.t[0] = 0;
    matrix.t[1] = 0;
    matrix.t[2] = 0;
    SetTransformMatrix(&matrix);
    input[0] = pack_10bit_vector(3, -4, 5);
    ApplyMatrixMany10Bit(input, output, 1, 22);
    CHECK(output[0].vx == 10);
    CHECK(output[0].vy == 4);
    CHECK(output[0].vz == 4);

    ApplyMatrixMany10Bit(NULL, NULL, 0, 22);
    return 0;
}

static int test_apply_matrix_many_variants(void)
{
    MATRIX matrix = {
        {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}
        },
        {10, -20, 30}
    };
    int packed[4] = {
        pack_10bit_vector(4, -8, 12),
        pack_10bit_vector(99, 99, 99),
        pack_10bit_vector(-2, 6, -10),
        pack_10bit_vector(88, 88, 88)
    };
    FVECTOR float_output[2];
    VECTOR long_output[2] = {
        {0, 0, 0, 101},
        {0, 0, 0, 102}
    };
    _svector short_output[2] = {
        {0, 0, 0, 103},
        {0, 0, 0, 104}
    };
    _svector short_input[2] = {
        {4, -8, 12, 105},
        {-2, 6, -10, 106}
    };
    FVECTOR float_input[2] = {
        {4.0f, -8.0f, 12.0f},
        {-2.0f, 6.0f, -10.0f}
    };

    SetTransformMatrix(&matrix);
    ApplyMatrixMany10BitFV(packed, float_output, 1, 22);
    CHECK(float_output[0].vx == 14.0f);
    CHECK(float_output[0].vy == -28.0f);
    CHECK(float_output[0].vz == 42.0f);

    ApplyMatrixMany10BitFVnormalize(packed, float_output, 1, 22);
    CHECK(float_output[0].vx == 10.0f + 4.0f / 4096.0f);
    CHECK(float_output[0].vy == -20.0f - 8.0f / 4096.0f);
    CHECK(float_output[0].vz == 30.0f + 12.0f / 4096.0f);

    ApplyMatrixMany10BitLong(packed, long_output, 2, 22);
    CHECK(long_output[0].vx == 14);
    CHECK(long_output[0].vy == -28);
    CHECK(long_output[0].vz == 42);
    CHECK(long_output[1].vx == 109);
    CHECK(long_output[1].vy == 79);
    CHECK(long_output[1].vz == 129);
    CHECK(long_output[0].pad == 101);
    CHECK(long_output[1].pad == 102);

    ApplyMatrixMany10BitStride(
        packed, short_output, 2, 22, 2);
    CHECK(short_output[0].vx == 14);
    CHECK(short_output[0].vy == -28);
    CHECK(short_output[0].vz == 42);
    CHECK(short_output[1].vx == 8);
    CHECK(short_output[1].vy == -14);
    CHECK(short_output[1].vz == 20);
    CHECK(short_output[0].pad == 103);
    CHECK(short_output[1].pad == 104);

    short_output[0].pad = 107;
    short_output[1].pad = 108;
    ApplyMatrixManySV(short_input, short_output, 2);
    CHECK(short_output[0].vx == 14);
    CHECK(short_output[0].vy == -28);
    CHECK(short_output[0].vz == 42);
    CHECK(short_output[1].vx == 8);
    CHECK(short_output[1].vy == -14);
    CHECK(short_output[1].vz == 20);
    CHECK(short_output[0].pad == 107);
    CHECK(short_output[1].pad == 108);

    ApplyMatrixManyFV(float_input, float_input, 2);
    CHECK(float_input[0].vx == 14.0f);
    CHECK(float_input[0].vy == -28.0f);
    CHECK(float_input[0].vz == 42.0f);
    CHECK(float_input[1].vx == 8.0f);
    CHECK(float_input[1].vy == -14.0f);
    CHECK(float_input[1].vz == 20.0f);

    matrix.t[0] = 40000;
    matrix.t[1] = 0;
    matrix.t[2] = 0;
    SetTransformMatrix(&matrix);
    packed[0] = pack_10bit_vector(300, 0, 0);
    long_output[0].pad = 109;
    ApplyMatrixMany10BitLong(packed, long_output, 1, 22);
    CHECK(long_output[0].vx == 40300);
    CHECK(long_output[0].pad == 109);
    short_input[0].vx = 30000;
    short_input[0].vy = 0;
    short_input[0].vz = 0;
    short_output[0].pad = 110;
    ApplyMatrixManySV(short_input, short_output, 1);
    CHECK(short_output[0].vx == 4464);
    CHECK(short_output[0].pad == 110);

    ApplyMatrixMany10BitFV(NULL, NULL, 0, 22);
    ApplyMatrixMany10BitFVnormalize(NULL, NULL, 0, 22);
    ApplyMatrixMany10BitLong(NULL, NULL, 0, 22);
    ApplyMatrixMany10BitStride(NULL, NULL, 0, 22, 1);
    ApplyMatrixManyFV(NULL, NULL, 0);
    ApplyMatrixManySV(NULL, NULL, 0);
    return 0;
}

static int test_square_roots(void)
{
    CHECK(SquareRoot0(0) == 0);
    CHECK(SquareRoot0(1) == 1);
    CHECK(SquareRoot0(2) == 1);
    CHECK(SquareRoot0(2147395600) == 46340);
    CHECK(SquareRoot0(-1) == INT32_MIN);

    CHECK(SquareRoot12(0) == 0);
    CHECK(SquareRoot12(1) == 4096);
    CHECK(SquareRoot12(2) == 5792);
    CHECK(SquareRoot12(4) == 8192);
    CHECK(SquareRoot12(-1) == INT32_MIN);
    return 0;
}

static int test_angles(void)
{
    float sine;
    float cosine;

    CHECK(rsin(0) == 0);
    CHECK(rcos(0) == 4096);
    CHECK(rsin(1024) == 4095);
    CHECK(rcos(1024) == 0);
    CHECK(rsin(2048) == 0);
    CHECK(rcos(2048) == -4095);
    CHECK(rsin(-1024) == -4095);
    CHECK(rcos(4096) == 4095);

    CHECK(ratan2(0, 1) == 0);
    CHECK(ratan2(1, 0) == 1023);
    CHECK(ratan2(0, -1) == 2047);
    CHECK(ratan2(-1, 0) == -1023);
    CHECK(ratan2(1, 1) == 511);

    FindSinCos(1024, &sine, &cosine);
    CHECK(fabsf(sine - 1.0f) < 0.000001f);
    CHECK(fabsf(cosine - (-0.000002189479f)) < 0.0000001f);

    FindSinCos(512, &sine, &cosine);
    CHECK(fabsf(sine - 0.7071075f) < 0.000001f);
    CHECK(fabsf(cosine - 0.7071060f) < 0.000001f);
    return 0;
}

static int test_lengths_and_distances(void)
{
    VECTOR origin = {0, 0, 0, 99};
    VECTOR point = {3, 4, 12, 88};
    VECTOR planar = {3, 400, 4, 77};
    VECTOR overflow = {46341, 0, 0, 0};
    _svector short_origin = {-1, -2, -3, 66};
    _svector short_point = {2, 2, 9, 55};
    VECTOR long_point = {2, 2, 9, 44};

    CHECK(vec_LengthLV(&point) == 13);
    CHECK(vec_DistanceLV(&origin, &point) == 13);
    CHECK(vec_Distance2DLV(&origin, &planar) == 5);
    CHECK(vec_DistanceSV(&short_origin, &short_point) == 13);
    CHECK(vec_DistanceSVLV(&short_origin, &long_point) == 13);

    /*
     * 46341^2 wraps to a negative 32-bit SquareRoot0 input. The reference
     * returns the cvtt sentinel, exposed through unsigned long.
     */
    CHECK(vec_LengthLV(&overflow) == UINT32_C(0x80000000));
    return 0;
}

static int test_point_line_squared(void)
{
    _svector p0 = {0, 0, 0, 0};
    _svector p1 = {4096, 0, 0, 0};
    _svector point = {2048, 100, 0, 0};
    _svector closest = {0, 0, 0, 0};
    _svector direction = {4096, 0, 0, 4096};
    _svector beyond = {5000, 0, 0, 0};
    int along = -1;

    CHECK(
        vecpointlinesquared(
            &p0, &p1, &point, &closest) ==
        10000);
    CHECK(closest.vx == 2048);
    CHECK(closest.vy == 0);
    CHECK(closest.vz == 0);
    CHECK(closest.pad == 2048);

    CHECK(
        vecpointlinesquared(
            &p0, &direction, &point, &along) ==
        10000);
    CHECK(along == 2048);
    CHECK(
        vecpointlinesquared(
            &p0, &direction, &beyond, NULL) ==
        -1);
    return 0;
}

static int test_normalization(void)
{
    VECTOR source = {3, 4, 0, 91};
    VECTOR zero = {0, 0, 0, 92};
    VECTOR output = {0, 0, 0, 93};
    _svector short_output = {0, 0, 0, 94};
    _svector short_input = {3, 4, 0, 98};
    _svector large_input = {INT16_MIN, 0, 0, 99};

    CHECK(VectorNormal(&source, &output) == 5);
    CHECK(output.vx == 2457);
    CHECK(output.vy == 3276);
    CHECK(output.vz == 0);
    CHECK(output.pad == 93);

    CHECK(VectorNormal(&zero, &output) == 0);
    CHECK(output.vx == 0);
    CHECK(output.vy == 0);
    CHECK(output.vz == 0);
    CHECK(output.pad == 93);

    CHECK(VectorNormalS(&source, &short_output) == 5);
    CHECK(short_output.vx == 2457);
    CHECK(short_output.vy == 3276);
    CHECK(short_output.vz == 0);
    CHECK(short_output.pad == 94);

    short_output.pad = 95;
    CHECK(normalize(3, 4, 0, &short_output) == 5);
    CHECK(short_output.vx == 2457);
    CHECK(short_output.vy == 3276);
    CHECK(short_output.vz == 0);
    CHECK(short_output.pad == 95);
    CHECK(normalize(3, 4, 0, NULL) == 5);

    output.pad = 96;
    CHECK(normalize_l(3, 4, 0, &output) == 5);
    CHECK(output.vx == 2457);
    CHECK(output.vy == 3276);
    CHECK(output.vz == 0);
    CHECK(output.pad == 0);
    CHECK(normalize_l(3, 4, 0, NULL) == 5);

    output.pad = 97;
    CHECK(normalize_lvector(&source, &output) == 5);
    CHECK(output.vx == 2457);
    CHECK(output.vy == 3276);
    CHECK(output.vz == 0);
    CHECK(output.pad == 97);

    short_output.pad = 100;
    CHECK(normalize_svector(&short_input, &short_output) == 5);
    CHECK(short_input.vx == 3);
    CHECK(short_input.vy == 4);
    CHECK(short_output.vx == 2457);
    CHECK(short_output.vy == 3276);
    CHECK(short_output.vz == 0);
    CHECK(short_output.pad == 100);

    CHECK(normalize_svector(&large_input, &short_output) == 16384);
    CHECK(large_input.vx == -16384);
    CHECK(short_output.vx == -4096);
    CHECK(short_output.vy == 0);
    CHECK(short_output.vz == 0);
    return 0;
}

static int test_flex_vector_family(void)
{
    _svector left = {1, 2, 3, 101};
    _svector right = {4, 5, 6, 102};
    _svector output = {0, 0, 0, 103};
    _svector fixed_x = {4096, 0, 0, 104};
    _svector fixed_y = {0, 4096, 0, 105};
    _svector scaled = {4096, -2048, 8192, 106};
    _svector packed_output = {0, 0, 0, 107};
    _svector normalize_input = {3, 4, 0, 108};
    VECTOR long_output = {0, 0, 0, 109};
    FVECTOR p0 = {0.0f, 0.0f, 0.0f};
    FVECTOR p1 = {4.0f, 0.0f, 0.0f};
    FVECTOR point = {2.0f, 3.0f, 0.0f};
    FVECTOR beyond = {5.0f, 1.0f, 0.0f};
    float along = -99.0f;

    CHECK(CROSS(&left, &right, &output) == &output);
    CHECK(output.vx == -3);
    CHECK(output.vy == 6);
    CHECK(output.vz == -3);
    CHECK(output.pad == 103);
    CHECK(DOT(&left, &right) == 32);

    output.pad = 110;
    CHECK(CROSS12(&fixed_x, &fixed_y, &output) == &output);
    CHECK(output.vx == 0);
    CHECK(output.vy == 0);
    CHECK(output.vz == 4096);
    CHECK(output.pad == 110);
    CHECK(DOT12(&fixed_x, &fixed_x) == 4096);

    output.pad = 111;
    CHECK(CROSSnormal(&fixed_x, &fixed_y, &output) == &output);
    CHECK(output.vx == 0);
    CHECK(output.vy == 0);
    CHECK(output.vz == 4096);
    CHECK(output.pad == 111);

    /* The retail stores each component before reading later aliased inputs. */
    left.vx = 1;
    left.vy = 2;
    left.vz = 3;
    CHECK(CROSS(&left, &right, &left) == &left);
    CHECK(left.vx == -3);
    CHECK(left.vy == 30);
    CHECK(left.vz == -135);

    CHECK(distance_squared(3, 4, 12) == 169);
    CHECK(flexabs(-123) == 123);
    CHECK(flexabs(INT32_MIN) == INT32_MIN);
    CHECK(flexdiv(3, 2) == 6144);
    CHECK(flexdiv(-3, 2) == -6144);
    CHECK(flexdiv(3, 0) == 12288);
    CHECK(mul4105(2) == 8210);

    CHECK_NEAR(
        fvectorpointlinesquared(&p0, &p1, &point, &along),
        9.0f,
        0.00001f);
    CHECK_NEAR(along, 2.0f, 0.00001f);
    CHECK(
        fvectorpointlinesquared(&p0, &p1, &beyond, NULL) ==
        -1.0f);
    CHECK(intersec_2dlines(&fixed_x, &fixed_y) == NULL);

    CHECK(normalize_s(3, 4, 0, &output) == 5);
    CHECK(output.vx == 2457);
    CHECK(output.vy == 3276);
    CHECK(output.vz == 0);
    CHECK(output.pad == 111);
    output.pad = 116;
    CHECK(normalize_s(INT32_MIN, 0, 0, &output) == 0);
    CHECK(output.vx == 0);
    CHECK(output.vy == 0);
    CHECK(output.vz == 0);
    CHECK(output.pad == 116);
    CHECK(normalize_vector(3, 4, 0, &long_output) == 25);
    CHECK(long_output.vx == 491);
    CHECK(long_output.vy == 655);
    CHECK(long_output.vz == 0);
    CHECK(long_output.pad == 109);

    unpack10bitnormal(pack_10bit_vector(-1, -2, 3), &packed_output);
    CHECK(packed_output.vx == -8);
    CHECK(packed_output.vy == -9);
    CHECK(packed_output.vz == 31);
    CHECK(packed_output.pad == 107);

    CHECK(vecSqr(&right) == 77);
    CHECK(vecSqr12(&fixed_x) == 4096);
    CHECK(veclength(&normalize_input) == 5);

    output.pad = 112;
    CHECK(vecadd(&fixed_x, &fixed_y, &output) == &output);
    CHECK(output.vx == 4096);
    CHECK(output.vy == 4096);
    CHECK(output.vz == 0);
    CHECK(output.pad == 112);
    CHECK(vecsub(&fixed_x, &fixed_y, &output) == &output);
    CHECK(output.vx == 4096);
    CHECK(output.vy == -4096);
    CHECK(output.vz == 0);
    CHECK(vecnegate(&output, &output) == &output);
    CHECK(output.vx == -4096);
    CHECK(output.vy == 4096);

    output.pad = 113;
    CHECK(vecscale(&scaled, 2048, &output) == &output);
    CHECK(output.vx == 2048);
    CHECK(output.vy == -1024);
    CHECK(output.vz == 4096);
    CHECK(output.pad == 113);
    CHECK(vecoffset(&output, 10, -20, 30) == &output);
    CHECK(output.vx == 2058);
    CHECK(output.vy == -1044);
    CHECK(output.vz == 4126);
    CHECK(output.pad == 113);

    normalize_input.vx = 3;
    normalize_input.vy = 4;
    normalize_input.vz = 0;
    CHECK(vecnormalize(&normalize_input) == &normalize_input);
    CHECK(normalize_input.vx == 2457);
    CHECK(normalize_input.vy == 3276);
    normalize_input.vx = 3;
    normalize_input.vy = 4;
    normalize_input.vz = 0;
    output.pad = 114;
    CHECK(vecnormalize2(&normalize_input, &output) == &output);
    CHECK(output.vx == 2457);
    CHECK(output.vy == 3276);
    CHECK(output.pad == 114);
    output.pad = 115;
    CHECK(vecnormalizevec(&normalize_input, &output) == &output);
    CHECK(output.vx == 2457);
    CHECK(output.vy == 3276);
    CHECK(output.pad == 115);
    return 0;
}

static int test_identity_matrix(void)
{
    MATRIX matrix;
    int row;
    int column;

    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            matrix.m[row][column] = 7.0f;
        }
        matrix.t[row] = 123;
    }

    CHECK(vec_IdentMatrix(&matrix) == &matrix);
    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            CHECK(matrix.m[row][column] ==
                  (row == column ? 1.0f : 0.0f));
        }
        CHECK(matrix.t[row] == 0);
    }
    return 0;
}

static void fill_matrix(
    MATRIX *matrix,
    float m00, float m01, float m02,
    float m10, float m11, float m12,
    float m20, float m21, float m22)
{
    matrix->m[0][0] = m00;
    matrix->m[0][1] = m01;
    matrix->m[0][2] = m02;
    matrix->m[1][0] = m10;
    matrix->m[1][1] = m11;
    matrix->m[1][2] = m12;
    matrix->m[2][0] = m20;
    matrix->m[2][1] = m21;
    matrix->m[2][2] = m22;
}

static int test_transform_state(void)
{
    MATRIX matrix;
    VECTOR translation = {10, 20, 30, 999};
    const MATRIX *transform;

    fill_matrix(
        &matrix,
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f);
    matrix.t[0] = 40;
    matrix.t[1] = 50;
    matrix.t[2] = 60;

    SetTransformMatrix(&matrix);
    transform = jpb_fmath_test_transform_matrix();
    CHECK(transform->m[0][0] == 1.0f);
    CHECK(transform->m[2][2] == 9.0f);
    CHECK(transform->t[0] == 40);
    CHECK(transform->t[1] == 50);
    CHECK(transform->t[2] == 60);

    SetTransformMatrix(NULL);
    CHECK(transform->t[2] == 60);

    SetGTETransLV(&translation);
    CHECK(transform->t[0] == 10);
    CHECK(transform->t[1] == 20);
    CHECK(transform->t[2] == 30);
    SetGTETransLV(NULL);
    CHECK(transform->t[0] == 0);
    CHECK(transform->t[1] == 0);
    CHECK(transform->t[2] == 0);
    return 0;
}

static int test_wrender_matrix_stack(void)
{
    MATRIX *current = jpb_WRenderCurrentMatrix();
    int index;

    while (jpb_WRenderMatrixStackLevel() != 0) {
        PopMatrix();
    }
    fill_matrix(
        current,
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f);
    current->t[0] = 10;
    current->t[1] = 20;
    current->t[2] = 30;
    PushMatrix();
    CHECK(jpb_WRenderMatrixStackLevel() == 1);
    current->m[0][0] = 11.0f;
    current->t[0] = 40;
    PushMatrix();
    CHECK(jpb_WRenderMatrixStackLevel() == 2);
    current->m[0][0] = 21.0f;
    current->t[0] = 50;
    PopMatrix();
    CHECK(jpb_WRenderMatrixStackLevel() == 1);
    CHECK(current->m[0][0] == 11.0f);
    CHECK(current->t[0] == 40);
    PopMatrix();
    CHECK(jpb_WRenderMatrixStackLevel() == 0);
    CHECK(current->m[0][0] == 1.0f);
    CHECK(current->m[2][2] == 9.0f);
    CHECK(current->t[0] == 10);
    CHECK(current->t[1] == 20);
    CHECK(current->t[2] == 30);
    PopMatrix();
    CHECK(jpb_WRenderMatrixStackLevel() == 0);

    for (index = 0; index < 20; ++index) {
        PushMatrix();
    }
    CHECK(jpb_WRenderMatrixStackLevel() == 15);
    current->m[0][0] = 99.0f;
    PushMatrix();
    CHECK(jpb_WRenderMatrixStackLevel() == 15);
    PopMatrix();
    CHECK(jpb_WRenderMatrixStackLevel() == 14);
    CHECK(current->m[0][0] == 1.0f);
    while (jpb_WRenderMatrixStackLevel() != 0) {
        PopMatrix();
    }
    return 0;
}

static int test_axis_rotation_matrices(void)
{
    MATRIX matrix;

    matrix.t[0] = 10;
    matrix.t[1] = 20;
    matrix.t[2] = 30;
    XRotMatrix(&matrix, 1024.0f);
    CHECK_NEAR(matrix.m[0][0], 1.0f, 0.000001f);
    CHECK_NEAR(matrix.m[1][1], 0.0f, 0.000001f);
    CHECK_NEAR(matrix.m[1][2], -1.0f, 0.000001f);
    CHECK_NEAR(matrix.m[2][1], 1.0f, 0.000001f);
    CHECK_NEAR(matrix.m[2][2], 0.0f, 0.000001f);
    CHECK(matrix.t[0] == 10);
    CHECK(matrix.t[1] == 20);
    CHECK(matrix.t[2] == 30);

    YRotMatrix(&matrix, 1024.0f);
    CHECK_NEAR(matrix.m[0][0], 0.0f, 0.000001f);
    CHECK_NEAR(matrix.m[0][2], 1.0f, 0.000001f);
    CHECK_NEAR(matrix.m[2][0], -1.0f, 0.000001f);
    CHECK_NEAR(matrix.m[2][2], 0.0f, 0.000001f);

    ZRotMatrix(&matrix, 1024.0f);
    CHECK_NEAR(matrix.m[0][0], 0.0f, 0.000001f);
    CHECK_NEAR(matrix.m[0][1], -1.0f, 0.000001f);
    CHECK_NEAR(matrix.m[1][0], 1.0f, 0.000001f);
    CHECK_NEAR(matrix.m[1][1], 0.0f, 0.000001f);
    return 0;
}

static int test_apply_matrix(void)
{
    MATRIX matrix;
    _svector short_input = {2, 3, 4, 71};
    VECTOR long_input = {2, 3, 4, 72};
    VECTOR long_output = {0, 0, 0, 73};
    FVECTOR float_input = {2.0f, 3.0f, 4.0f};
    FVECTOR float_output = {0.0f, 0.0f, 0.0f};

    fill_matrix(
        &matrix,
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f);

    CHECK(fApplyMatrix(&matrix, &short_input, &long_output) == &long_output);
    CHECK(long_output.vx == 20);
    CHECK(long_output.vy == 47);
    CHECK(long_output.vz == 74);
    CHECK(long_output.pad == 73);

    long_output.pad = 74;
    CHECK(fApplyMatrixLV(&matrix, &long_input, &long_output) == &long_output);
    CHECK(long_output.vx == 20);
    CHECK(long_output.vy == 47);
    CHECK(long_output.vz == 74);
    CHECK(long_output.pad == 74);

    CHECK(fApplyMatrixFV(&matrix, &float_input, &float_output) ==
          &float_output);
    CHECK(float_output.vx == 20.0f);
    CHECK(float_output.vy == 47.0f);
    CHECK(float_output.vz == 74.0f);

    /*
     * LV is sequential and not alias-safe in the reference, while FV uses
     * its PDB-named temporary and is alias-safe.
     */
    fill_matrix(
        &matrix,
        1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f);
    long_input.vx = 2;
    long_input.vy = 3;
    long_input.vz = 4;
    CHECK(fApplyMatrixLV(&matrix, &long_input, &long_input) == &long_input);
    CHECK(long_input.vx == 5);
    CHECK(long_input.vy == 5);
    CHECK(long_input.vz == 4);

    float_input.vx = 2.0f;
    float_input.vy = 3.0f;
    float_input.vz = 4.0f;
    CHECK(fApplyMatrixFV(&matrix, &float_input, &float_input) == &float_input);
    CHECK(float_input.vx == 5.0f);
    CHECK(float_input.vy == 2.0f);
    CHECK(float_input.vz == 4.0f);
    return 0;
}

static int test_perspective_transform(void)
{
    MATRIX matrix;
    FVECTOR source = {2.0f, 3.0f, 10.0f};
    FVECTOR destination;
    FVECTOR many_source[3] = {
        {0.0f, 0.0f, 0.0f},
        {2.0f, -1.0f, 2.0f},
        {-4.0f, 5.0f, 10.0f},
    };
    FVECTOR many_destination[3];
    float projection;

    vec_IdentMatrix(&matrix);
    matrix.t[0] = 1;
    matrix.t[1] = -2;
    matrix.t[2] = 5;
    CHECK(PerspectiveTransformFV(
              &matrix, &source, &destination) == 0);
    projection = 460.0f / 15.0f;
    CHECK(destination.vx == 3.0f * projection + 320.0f);
    CHECK(destination.vy == 1.0f * projection + 240.0f);
    CHECK(destination.vz == 15.0f / 10240.0f);

    vec_IdentMatrix(&matrix);
    CHECK(PerspectiveTransformFV(
              &matrix, &many_source[0], &destination) == 1);
    CHECK(destination.vx == 320.0f);
    CHECK(destination.vy == 240.0f);
    CHECK(destination.vz == 1.0f / 10240.0f);

    source.vx = 1.0f;
    source.vy = 2.0f;
    source.vz = 4.0f;
    CHECK(PerspectiveTransformFV(&matrix, &source, &source) == 0);
    CHECK(source.vx == 435.0f);
    CHECK(source.vy == 470.0f);
    CHECK(source.vz == 4.0f / 10240.0f);

    CHECK(PerspectiveTransformManyFV(
              &matrix, many_source, many_destination, 3) == 1);
    CHECK(many_destination[0].vx == 320.0f);
    CHECK(many_destination[1].vx == 780.0f);
    CHECK(many_destination[1].vy == 10.0f);
    CHECK(many_destination[2].vx == 136.0f);
    CHECK(many_destination[2].vy == 470.0f);
    CHECK(PerspectiveTransformManyFV(
              &matrix, many_source, many_destination, 0) == 0);

    source.vx = 1.0f;
    source.vy = 2.0f;
    source.vz = NAN;
    CHECK(PerspectiveTransformFV(
              &matrix, &source, &destination) == 0);
    CHECK(isnan(destination.vx));
    CHECK(isnan(destination.vy));
    CHECK(isnan(destination.vz));
    return 0;
}

static int test_legacy_perspective_transforms(void)
{
    MATRIX matrix;
    _svector short_source = {2, 3, 10, 201};
    VECTOR long_source = {2, 3, 10, 202};
    FVECTOR destination;
    float projection;

    vec_IdentMatrix(&matrix);
    matrix.t[0] = 1;
    matrix.t[1] = -2;
    matrix.t[2] = 5;

    CHECK(PerspectiveTransform(
              &matrix, &short_source, &destination) == 0);
    CHECK(destination.vx == 3.0f);
    CHECK(destination.vy == 1.0f);
    CHECK(destination.vz == 15.0f);

    projection = 460.0f / 15.0f;
    CHECK(PerspectiveTransformLV(
              &matrix, &long_source, &destination) == 0);
    CHECK(destination.vx == 3.0f * projection + 320.0f);
    CHECK(destination.vy == 1.0f * projection + 240.0f);
    CHECK(destination.vz == 15.0f / 10240.0f);

    CHECK(PerspectiveTransformOLD(
              &matrix, &short_source, &destination) == 0);
    CHECK(destination.vx == 3.0f * projection + 320.0f);
    CHECK(destination.vy == 1.0f * projection + 240.0f);
    CHECK(destination.vz == 15.0f / 10240.0f);

    long_source.vx = 0;
    long_source.vy = 0;
    long_source.vz = -5;
    CHECK(PerspectiveTransformLV(
              &matrix, &long_source, &destination) == 1);
    CHECK(destination.vx == 780.0f);
    CHECK(destination.vy == -680.0f);
    CHECK(destination.vz == 1.0f / 10240.0f);
    return 0;
}

static int test_matrix_multiply(void)
{
    MATRIX left;
    MATRIX right;
    MATRIX result;
    static const float expected[3][3] = {
        {30.0f, 24.0f, 18.0f},
        {84.0f, 69.0f, 54.0f},
        {138.0f, 114.0f, 90.0f},
    };
    int row;
    int column;

    fill_matrix(
        &left,
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f);
    fill_matrix(
        &right,
        9.0f, 8.0f, 7.0f,
        6.0f, 5.0f, 4.0f,
        3.0f, 2.0f, 1.0f);
    left.t[0] = 10;
    left.t[1] = 20;
    left.t[2] = 30;

    result.t[0] = 91;
    result.t[1] = 92;
    result.t[2] = 93;
    CHECK(fMulMatrix0(&left, &right, &result) == &result);
    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            CHECK(result.m[row][column] == expected[row][column]);
        }
    }
    CHECK(result.t[0] == 91);
    CHECK(result.t[1] == 92);
    CHECK(result.t[2] == 93);

    CHECK(fMulMatrix(&left, &right) == &left);
    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            CHECK(left.m[row][column] == expected[row][column]);
        }
    }
    CHECK(left.t[0] == 10);
    CHECK(left.t[1] == 20);
    CHECK(left.t[2] == 30);
    return 0;
}

static int test_matrix_composition(void)
{
    _svector rotation = {123, -456, 789, 100};
    MATRIX composed;
    MATRIX expected;
    MATRIX zyx;
    int row;
    int column;

    CHECK(fRotMatrix(&rotation, &composed) == &composed);
    vec_IdentMatrix(&expected);
    fRotMatrixZ(rotation.vz, &expected);
    fRotMatrixY(rotation.vy, &expected);
    fRotMatrixX(rotation.vx, &expected);
    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            CHECK(composed.m[row][column] == expected.m[row][column]);
        }
        CHECK(composed.t[row] == 0);
    }

    CHECK(fRotMatrixZYX(&rotation, &zyx) == &zyx);
    vec_IdentMatrix(&expected);
    fRotMatrixX(rotation.vx, &expected);
    fRotMatrixY(rotation.vy, &expected);
    fRotMatrixZ(rotation.vz, &expected);
    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            CHECK(zyx.m[row][column] == expected.m[row][column]);
        }
        CHECK(zyx.t[row] == 0);
    }
    return 0;
}

static int test_scale_translate_transpose_and_depth(void)
{
    MATRIX matrix;
    MATRIX transpose;
    VECTOR scale = {8192, 4096, 2048, 0};
    VECTOR translation = {11, 22, 33, 0};
    VECTOR point = {2, 3, 4, 0};

    vec_IdentMatrix(&matrix);
    matrix.t[0] = 1;
    matrix.t[1] = 2;
    matrix.t[2] = 3;
    CHECK(fScaleMatrix(&matrix, &scale) == &matrix);
    CHECK(matrix.m[0][0] == 2.0f);
    CHECK(matrix.m[1][1] == 1.0f);
    CHECK(matrix.m[2][2] == 0.5f);
    CHECK(matrix.t[0] == 1);
    CHECK(matrix.t[1] == 2);
    CHECK(matrix.t[2] == 3);

    CHECK(fTransMatrix(&matrix, &translation) == &matrix);
    CHECK(matrix.t[0] == 11);
    CHECK(matrix.t[1] == 22);
    CHECK(matrix.t[2] == 33);

    fill_matrix(
        &matrix,
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f);
    matrix.t[0] = 10;
    matrix.t[1] = 20;
    matrix.t[2] = 30;
    CHECK(fTransposeMatrix(&matrix, &transpose) == &transpose);
    CHECK(transpose.m[0][0] == 1.0f);
    CHECK(transpose.m[0][1] == 4.0f);
    CHECK(transpose.m[0][2] == 7.0f);
    CHECK(transpose.m[1][0] == 2.0f);
    CHECK(transpose.m[1][1] == 5.0f);
    CHECK(transpose.m[1][2] == 8.0f);
    CHECK(transpose.m[2][0] == 3.0f);
    CHECK(transpose.m[2][1] == 6.0f);
    CHECK(transpose.m[2][2] == 9.0f);
    CHECK(transpose.t[0] == 0);
    CHECK(transpose.t[1] == 0);
    CHECK(transpose.t[2] == 0);

    CHECK(getscreenz(&matrix, &point) == 104.0f);

    /*
     * The reference's direct assignment sequence is not an in-place
     * transpose: overwritten upper-triangle values are read back.
     */
    CHECK(fTransposeMatrix(&matrix, &matrix) == &matrix);
    CHECK(matrix.m[0][0] == 1.0f);
    CHECK(matrix.m[0][1] == 4.0f);
    CHECK(matrix.m[0][2] == 7.0f);
    CHECK(matrix.m[1][0] == 4.0f);
    CHECK(matrix.m[1][1] == 5.0f);
    CHECK(matrix.m[1][2] == 8.0f);
    CHECK(matrix.m[2][0] == 7.0f);
    CHECK(matrix.m[2][1] == 8.0f);
    CHECK(matrix.m[2][2] == 9.0f);
    CHECK(matrix.t[0] == 0);
    CHECK(matrix.t[1] == 0);
    CHECK(matrix.t[2] == 0);
    return 0;
}

static int test_rotate_translate_many_short_vectors(void)
{
    MATRIX matrix = {
        {
            {2.0f, 0.0f, 0.5f},
            {0.0f, -1.0f, 0.0f},
            {1.0f, 0.0f, 3.0f}
        },
        {10, 20, 30}
    };
    _svector source[2] = {
        {4, -5, 6, 111},
        {-2, 3, 8, 222}
    };
    FVECTOR destination[2] = {
        {99.0f, 99.0f, 99.0f},
        {99.0f, 99.0f, 99.0f}
    };

    CHECK(fRotTransPers(
              &matrix, source, destination, 2) == 0);
    CHECK_NEAR(destination[0].vx, 21.0f, 0.000001f);
    CHECK_NEAR(destination[0].vy, 25.0f, 0.000001f);
    CHECK_NEAR(destination[0].vz, 52.0f, 0.000001f);
    CHECK_NEAR(destination[1].vx, 10.0f, 0.000001f);
    CHECK_NEAR(destination[1].vy, 17.0f, 0.000001f);
    CHECK_NEAR(destination[1].vz, 52.0f, 0.000001f);
    CHECK(fRotTransPers(
              &matrix, source, destination, 0) == 0);
    return 0;
}

static int test_rotate_translate_float_vectors(void)
{
    MATRIX matrix = {
        {
            {2.0f, 0.0f, 0.5f},
            {0.0f, -1.0f, 0.0f},
            {1.0f, 0.0f, 3.0f}
        },
        {10, 20, 30}
    };
    FVECTOR float_vectors[2] = {
        {4.0f, -5.0f, 6.0f},
        {-2.0f, 3.0f, 8.0f}
    };
    _sfvector padded_vectors[2] = {
        {4.0f, -5.0f, 6.0f, 301},
        {-2.0f, 3.0f, 8.0f, 302}
    };
    FVECTOR output[2];

    CHECK(RotTransPersFloat(
              &matrix, float_vectors, output, 2) == 0);
    CHECK(output[0].vx == 21.0f);
    CHECK(output[0].vy == 25.0f);
    CHECK(output[0].vz == 52.0f);
    CHECK(output[1].vx == 10.0f);
    CHECK(output[1].vy == 17.0f);
    CHECK(output[1].vz == 52.0f);

    CHECK(RotTransPersSFV(
              &matrix, padded_vectors, output, 2) == 0);
    CHECK(output[0].vx == 21.0f);
    CHECK(output[0].vy == 25.0f);
    CHECK(output[0].vz == 52.0f);
    CHECK(output[1].vx == 10.0f);
    CHECK(output[1].vy == 17.0f);
    CHECK(output[1].vz == 52.0f);
    CHECK(padded_vectors[0].pad == 301);
    CHECK(padded_vectors[1].pad == 302);

    CHECK(RotTransPersFloat(NULL, NULL, NULL, 0) == 0);
    CHECK(RotTransPersSFV(NULL, NULL, NULL, 0) == 0);
    return 0;
}

static int test_camera_matrix_point_transforms(void)
{
    _svector points[3] = {
        {0, 0, 10, 401},
        {2, -1, 2, 402},
        {0, 0, 0, 403}
    };
    int packed[3] = {0, 0, 0};
    FVECTOR camera_space[3];

    vec_IdentMatrix(&CameraMatrix);
    CHECK(TransformPoints(points, packed, 3) == 0);
    CHECK((uint32_t)packed[0] == UINT32_C(0x00f00140));
    CHECK((uint32_t)packed[1] == UINT32_C(0x000a030c));
    CHECK((uint32_t)packed[2] == UINT32_C(0x00f00140));

    CHECK(TransformPointsFV(points, camera_space, 3) == 0);
    CHECK(camera_space[0].vx == 0.0f);
    CHECK(camera_space[0].vy == 0.0f);
    CHECK(camera_space[0].vz == 10.0f);
    CHECK(camera_space[1].vx == 2.0f);
    CHECK(camera_space[1].vy == -1.0f);
    CHECK(camera_space[1].vz == 2.0f);
    CHECK(camera_space[2].vx == 0.0f);
    CHECK(camera_space[2].vy == 0.0f);
    CHECK(camera_space[2].vz == 1.0f);

    CameraMatrix.t[0] = 1;
    CameraMatrix.t[1] = -2;
    CameraMatrix.t[2] = 5;
    CHECK(TransformPointsFV(points, camera_space, 1) == 0);
    CHECK(camera_space[0].vx == 1.0f);
    CHECK(camera_space[0].vy == -2.0f);
    CHECK(camera_space[0].vz == 15.0f);

    CHECK(TransformPoints(NULL, NULL, 0) == 0);
    CHECK(TransformPointsFV(NULL, NULL, 0) == 0);
    return 0;
}

int main(void)
{
    if (test_square_roots() != 0 ||
        test_apply_matrix_many_10bit() != 0 ||
        test_apply_matrix_many_variants() != 0 ||
        test_angles() != 0 ||
        test_lengths_and_distances() != 0 ||
        test_point_line_squared() != 0 ||
        test_normalization() != 0 ||
        test_flex_vector_family() != 0 ||
        test_identity_matrix() != 0 ||
        test_transform_state() != 0 ||
        test_wrender_matrix_stack() != 0 ||
        test_axis_rotation_matrices() != 0 ||
        test_apply_matrix() != 0 ||
        test_perspective_transform() != 0 ||
        test_legacy_perspective_transforms() != 0 ||
        test_matrix_multiply() != 0 ||
        test_matrix_composition() != 0 ||
        test_rotate_translate_many_short_vectors() != 0 ||
        test_rotate_translate_float_vectors() != 0 ||
        test_camera_matrix_point_transforms() != 0 ||
        test_scale_translate_transpose_and_depth() != 0) {
        return 1;
    }

    puts("fmath/vector tests passed");
    return 0;
}
