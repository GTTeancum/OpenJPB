#include "jpb/fmath.h"
#include "jpb/vectors.h"
#include "jpb/wrender.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

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

static int test_invert_matrix(void)
{
    MATRIX source;
    MATRIX output;

    fill_matrix(
        &source,
        1.0f, 1.25f, 3.5f,
        2.75f, 5.0f, 5.25f,
        -4.75f, 6.75f, 9.0f);
    source.t[0] = 10;
    source.t[1] = 20;
    source.t[2] = 30;
    output.t[0] = 40;
    output.t[1] = 50;
    output.t[2] = 60;

    CHECK(InvertMatrix(&source, &output) == &output);
    CHECK(output.m[0][0] == 1.0f);
    CHECK(output.m[1][1] == 5.0f);
    CHECK(output.m[2][2] == 9.0f);
    CHECK(output.m[1][0] == 1.25f);
    CHECK(output.m[0][1] == 2.0f);
    CHECK(output.m[2][0] == 3.5f);
    CHECK(output.m[0][2] == -4.0f);
    CHECK(output.m[2][1] == 5.25f);
    CHECK(output.m[1][2] == 6.0f);
    CHECK(output.t[0] == 40);
    CHECK(output.t[1] == 50);
    CHECK(output.t[2] == 60);

    CHECK(InvertMatrix(&source, &source) == &source);
    CHECK(source.m[1][0] == 1.25f);
    CHECK(source.m[0][1] == 2.0f);
    CHECK(source.m[2][0] == 3.5f);
    CHECK(source.m[0][2] == -4.0f);
    CHECK(source.m[2][1] == 5.25f);
    CHECK(source.m[1][2] == 6.0f);
    CHECK(source.t[0] == 10);
    CHECK(source.t[1] == 20);
    CHECK(source.t[2] == 30);
    return 0;
}

static int test_area_and_linear_combine(void)
{
    VECTOR a = {0, 0, 0, 0};
    VECTOR b = {3, 0, 0, 0};
    VECTOR c = {0, 4, 0, 0};
    VECTOR world = {100, 200, 300, 0};
    _svector light = {2, -3, 4, 0};
    _svector point = {0, 0, 0, 77};

    CHECK(vec_AreaTriangle(&a, &b, &c) == 6);
    vec_LinearCombine(&point, &world, &light, 10);
    CHECK(point.vx == 120);
    CHECK(point.vy == 170);
    CHECK(point.vz == 340);
    CHECK(point.pad == 77);
    return 0;
}

static int test_point_to_line_distance(void)
{
    VECTOR a = {0, 0, 0, 0};
    VECTOR b = {10, 0, 0, 0};
    VECTOR point = {5, 3, 4, 0};

    CHECK(vec_DistPoint2Line(&point, &a, &b) == 5);
    point.vx = -1;
    point.vy = 0;
    point.vz = 0;
    CHECK(vec_DistPoint2Line(&point, &a, &b) == -1);
    point.vx = 11;
    CHECK(vec_DistPoint2Line(&point, &a, &b) == -1);

    /*
     * Projection uses low words, but the final distance uses the full
     * VECTOR. The square of 65536 then wraps to zero in the reference.
     */
    point.vx = 65541;
    CHECK(vec_DistPoint2Line(&point, &a, &b) == 0);
    return 0;
}

static int test_distance_and_length_entries(void)
{
    VECTOR long_a = {1, 2, 3, 71};
    VECTOR long_b = {4, 6, 15, 72};
    VECTOR length = {3, 4, 12, 73};
    _svector short_a = {1, 2, 3, 74};
    _svector short_b = {4, 6, 15, 75};

    CHECK(vec_Distance2DLV(&long_a, &long_b) == 12);
    CHECK(vec_DistanceLV(&long_a, &long_b) == 13);
    CHECK(vec_DistanceSV(&short_a, &short_b) == 13);
    CHECK(vec_DistanceSVLV(&short_a, &long_b) == 13);
    CHECK(vec_LengthLV(&length) == 13);
    return 0;
}

static int test_segment_and_polar(void)
{
    VECTOR a = {0, 0, 0, 0};
    VECTOR b = {10, 10, 10, 0};
    VECTOR p = {-1, 5, 5, 0};
    VECTOR polar = {0, 0, 4096, 0};
    VECTOR rectangular = {0, 0, 0, 88};

    CHECK(vec_PointNearSegment(2, &p, &a, &b) == 1);
    p.vx = -2;
    CHECK(vec_PointNearSegment(2, &p, &a, &b) == 0);
    p.vx = 11;
    CHECK(vec_PointNearSegment(2, &p, &a, &b) == 1);
    p.vx = 12;
    CHECK(vec_PointNearSegment(2, &p, &a, &b) == 0);

    vec_Polar2Rect(&polar, &rectangular);
    CHECK(rectangular.vx == 4096);
    CHECK(rectangular.vy == 0);
    CHECK(rectangular.vz == 0);
    CHECK(rectangular.pad == 88);

    polar.vx = 1024;
    vec_Polar2Rect(&polar, &rectangular);
    CHECK(rectangular.vx == 0);
    CHECK(rectangular.vy == 4095);
    CHECK(rectangular.vz == 0);
    return 0;
}

static int test_quick_checks(void)
{
    VECTOR origin = {0, 0, 0, 0};
    VECTOR point = {160, 320, 480, 0};
    VECTOR planar = {3, 100000, 4, 0};
    FVECTOR float_origin = {0.0f, 0.0f, 0.0f};
    FVECTOR float_point = {1.0f, -2.0f, 3.0f};

    CHECK(vec_QuickDistanceLV(&origin, &point) == 10);
    CHECK(vec_QuickRangeCheck(&origin, &point, 480) == 1);
    CHECK(vec_QuickRangeCheck(&origin, &point, 479) == 0);
    CHECK(vec_QuickRangeCheckFV(&float_origin, &float_point, 3.0f) == 1);
    CHECK(vec_QuickRangeCheckFV(&float_origin, &float_point, 2.9f) == 0);

    /* comiss/setbe make unordered (NaN) components pass in the reference. */
    float_point.vx = NAN;
    float_point.vy = 0.0f;
    float_point.vz = 0.0f;
    CHECK(vec_QuickRangeCheckFV(&float_origin, &float_point, 0.0f) == 1);

    CHECK(vec_RangeCheck(&origin, &planar, 5) == 1);
    CHECK(vec_RangeCheck(&origin, &planar, 4) == 0);
    return 0;
}

static int test_rotation_and_scale(void)
{
    VECTOR normal_long = {0, 4096, 4096, 0};
    FVECTOR normal_float = {0.0f, 4096.0f, 4096.0f};
    _svector normal_short = {0, 1000, 1000, 0};
    _svector rotation = {0, 0, 0, 91};
    _svector scaled = {4096, -4096, 2048, 92};

    CHECK(vec_RotFromNormal(&rotation, &normal_long) == 65025);
    CHECK(rotation.vx == -511);
    CHECK(rotation.vy == 0);
    CHECK(rotation.vz == 0);
    CHECK(rotation.pad == 91);

    rotation.pad = 93;
    CHECK(vec_RotFromNormalF(&rotation, &normal_float) == 65025);
    CHECK(rotation.vx == -511);
    CHECK(rotation.vy == 0);
    CHECK(rotation.vz == 0);
    CHECK(rotation.pad == 93);

    rotation.pad = 94;
    CHECK(vec_RotFromNormalS(&rotation, &normal_short) == 65025);
    CHECK(rotation.vx == -511);
    CHECK(rotation.vy == 0);
    CHECK(rotation.vz == 0);
    CHECK(rotation.pad == 94);

    vec_ScaleVector(&scaled, 2048);
    CHECK(scaled.vx == 2048);
    CHECK(scaled.vy == -2048);
    CHECK(scaled.vz == 1024);
    CHECK(scaled.pad == 92);
    return 0;
}

static int test_rotation_wrappers(void)
{
    _svector rotation = {256, 512, 768, 101};
    VECTOR source = {10, -20, 30, 102};
    VECTOR expected;
    VECTOR actual = {0, 0, 0, 103};
    MATRIX matrix;
    MATRIX matrix_x;
    MATRIX matrix_y;
    MATRIX saved_current;
    MATRIX expected_current;
    VECTOR temp;
    int32_t pop_result;

    while (jpb_WRenderMatrixStackLevel() != 0) {
        PopMatrix();
    }
    saved_current = *jpb_WRenderCurrentMatrix();
    fill_matrix(
        &expected_current,
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f);
    expected_current.t[0] = 10;
    expected_current.t[1] = 11;
    expected_current.t[2] = 12;
    *jpb_WRenderCurrentMatrix() = expected_current;
    pop_result = jpb_WRenderMatrixStackBaseLow32();

    (void)vec_IdentMatrix(&matrix);
    (void)fRotMatrix(&rotation, &matrix);
    expected.pad = 104;
    (void)fApplyMatrixLV(&matrix, &source, &expected);
    CHECK(vec_RotVectorLV(&rotation, &source, &actual) == pop_result);
    CHECK(jpb_WRenderMatrixStackLevel() == 0);
    CHECK(memcmp(
              jpb_WRenderCurrentMatrix(),
              &expected_current,
              sizeof(expected_current)) == 0);
    CHECK(actual.vx == expected.vx);
    CHECK(actual.vy == expected.vy);
    CHECK(actual.vz == expected.vz);
    CHECK(actual.pad == 103);

    (void)vec_IdentMatrix(&matrix);
    (void)fRotMatrixY(1024, &matrix);
    expected.pad = 105;
    (void)fApplyMatrixLV(&matrix, &source, &expected);
    actual.pad = 106;
    CHECK(vec_RotVectorY(1024, &source, &actual) == pop_result);
    CHECK(jpb_WRenderMatrixStackLevel() == 0);
    CHECK(memcmp(
              jpb_WRenderCurrentMatrix(),
              &expected_current,
              sizeof(expected_current)) == 0);
    CHECK(actual.vx == expected.vx);
    CHECK(actual.vy == expected.vy);
    CHECK(actual.vz == expected.vz);
    CHECK(actual.pad == 106);

    (void)vec_IdentMatrix(&matrix_x);
    matrix_x.m[0][0] = 4096.0f;
    matrix_x.m[1][1] = 4096.0f;
    matrix_x.m[2][2] = 4096.0f;
    (void)vec_IdentMatrix(&matrix_y);
    matrix_y.m[0][0] = 4096.0f;
    matrix_y.m[1][1] = 4096.0f;
    matrix_y.m[2][2] = 4096.0f;
    temp = source;
    (void)fRotMatrixX((int)rotation.vx, &matrix_x);
    (void)fApplyMatrixLV(&matrix_x, &temp, &expected);
    temp = expected;
    (void)fRotMatrixY((int)rotation.vy, &matrix_y);
    (void)fApplyMatrixLV(&matrix_y, &temp, &expected);
    actual.pad = 107;
    CHECK(vec_InvRotVectorLV(&rotation, &source, &actual) == pop_result);
    CHECK(jpb_WRenderMatrixStackLevel() == 0);
    CHECK(memcmp(
              jpb_WRenderCurrentMatrix(),
              &expected_current,
              sizeof(expected_current)) == 0);
    CHECK(actual.vx == expected.vx);
    CHECK(actual.vy == expected.vy);
    CHECK(actual.vz == expected.vz);
    CHECK(actual.pad == 107);

    /* vec_InvRotVectorLV stages through a temp, so source/dest may alias. */
    actual = source;
    actual.pad = 108;
    CHECK(vec_InvRotVectorLV(&rotation, &actual, &actual) == pop_result);
    CHECK(actual.vx == expected.vx);
    CHECK(actual.vy == expected.vy);
    CHECK(actual.vz == expected.vz);
    CHECK(actual.pad == 108);

    while (jpb_WRenderMatrixStackLevel() < 15) {
        PushMatrix();
    }
    CHECK(vec_RotVectorY(0, &source, &actual) == pop_result);
    CHECK(jpb_WRenderMatrixStackLevel() == 14);
    while (jpb_WRenderMatrixStackLevel() != 0) {
        PopMatrix();
    }
    *jpb_WRenderCurrentMatrix() = saved_current;
    return 0;
}

static int test_normal_wrappers(void)
{
    VECTOR source = {8193, 4095, 0, 0};
    VECTOR half = {4096, 2047, 0, 0};
    VECTOR expected_long = {0, 0, 0, 0};
    VECTOR actual_long = {0, 0, 0, 95};
    _svector expected_short = {0, 0, 0, 0};
    _svector actual_short = {0, 0, 0, 96};

    (void)VectorNormal(&half, &expected_long);
    vec_VectorNormalLV(&source, &actual_long);
    CHECK(actual_long.vx == expected_long.vx);
    CHECK(actual_long.vy == expected_long.vy);
    CHECK(actual_long.vz == expected_long.vz);
    CHECK(actual_long.pad == 95);

    (void)VectorNormalS(&half, &expected_short);
    vec_VectorNormalSV(&source, &actual_short);
    CHECK(actual_short.vx == expected_short.vx);
    CHECK(actual_short.vy == expected_short.vy);
    CHECK(actual_short.vz == expected_short.vz);
    CHECK(actual_short.pad == 96);
    return 0;
}

static int test_planes(void)
{
    VECTOR p = {0, 0, 5, 0};
    VECTOR q = {3, 0, 5, 0};
    VECTOR r = {0, 4, 5, 0};
    VECTOR normal = {0, 0, 4096, 0};
    VECTOR world = {0, 0, 0, 0};
    VECTOR light_f12 = {0, 0, 4096, 0};
    _svector light = {0, 0, 1, 0};
    _svector projected = {0, 0, 0, 97};
    Plane plane = {{0, 0, 0, 98}, 0};

    vec_gDefinePlane(&p, &q, &r, &plane);
    CHECK(plane.plane_normal.vx == 0);
    CHECK(plane.plane_normal.vy == 0);
    CHECK(plane.plane_normal.vz == 12);
    CHECK(plane.plane_normal.pad == 98);
    CHECK(plane.plane_const == 60);

    plane.plane_normal.pad = 99;
    vec_gDefinePlaneNormal(&normal, &p, &plane);
    CHECK(plane.plane_normal.vx == 0);
    CHECK(plane.plane_normal.vy == 0);
    CHECK(plane.plane_normal.vz == 4096);
    CHECK(plane.plane_normal.pad == 99);
    CHECK(plane.plane_const == 20480);

    plane.plane_normal.vz = 1;
    plane.plane_const = 5;
    vec_gProject2Plane(&projected, &world, &light, &plane);
    CHECK(projected.vx == 0);
    CHECK(projected.vy == 0);
    CHECK(projected.vz == 5);
    CHECK(projected.pad == 97);

    projected.pad = 100;
    vec_gProject2PlaneF12(&projected, &world, &light_f12, &plane);
    CHECK(projected.vx == 0);
    CHECK(projected.vy == 0);
    CHECK(projected.vz == 5);
    CHECK(projected.pad == 100);
    return 0;
}

int main(void)
{
    if (test_invert_matrix() != 0 ||
        test_area_and_linear_combine() != 0 ||
        test_point_to_line_distance() != 0 ||
        test_distance_and_length_entries() != 0 ||
        test_segment_and_polar() != 0 ||
        test_quick_checks() != 0 ||
        test_rotation_and_scale() != 0 ||
        test_rotation_wrappers() != 0 ||
        test_normal_wrappers() != 0 ||
        test_planes() != 0) {
        return 1;
    }

    puts("vector geometry tests passed");
    return 0;
}
