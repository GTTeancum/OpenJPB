#include "jpb/d3dmath.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::fprintf(                                                    \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                             \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static float *values(DirectX::XMMATRIX &matrix)
{
    return reinterpret_cast<float *>(&matrix);
}

static float *values(__m128 &vector)
{
    return reinterpret_cast<float *>(&vector);
}

static bool close_float(float left, float right)
{
    return std::fabs(left - right) < 0.0001f;
}

static void identity(DirectX::XMMATRIX &matrix)
{
    float *m = values(matrix);

    std::memset(m, 0, sizeof(matrix));
    m[0] = 1.0f;
    m[5] = 1.0f;
    m[10] = 1.0f;
    m[15] = 1.0f;
}

static int test_matrix_paths(void)
{
    DirectX::XMMATRIX matrix;
    DirectX::XMMATRIX inverse;
    DirectX::XMMATRIX in_place;
    DirectX::XMMATRIX product;
    float *m;
    float *p;

    std::memset(&matrix, 0x7f, sizeof(matrix));
    D3DMath_MatrixFromQuaternion(matrix, 0.0f, 0.0f, 0.0f, 1.0f);
    m = values(matrix);
    CHECK(m[0] == 1.0f && m[5] == 1.0f);
    CHECK(m[10] == 1.0f && m[15] == 1.0f);
    CHECK(m[1] == 0.0f && m[14] == 0.0f);

    identity(matrix);
    m = values(matrix);
    m[0] = 2.0f;
    m[5] = 3.0f;
    m[10] = 4.0f;
    m[12] = 5.0f;
    m[13] = 6.0f;
    m[14] = 7.0f;
    CHECK(D3DMath_MatrixInvert(inverse, matrix) == 0);
    D3DMath_MatrixMultiply(product, matrix, inverse);
    p = values(product);
    CHECK(close_float(p[0], 1.0f));
    CHECK(close_float(p[5], 1.0f));
    CHECK(close_float(p[10], 1.0f));
    CHECK(close_float(p[15], 1.0f));
    CHECK(close_float(p[12], 0.0f));
    CHECK(close_float(p[13], 0.0f));
    CHECK(close_float(p[14], 0.0f));

    in_place = matrix;
    CHECK(D3DMath_MatrixInvert(in_place, in_place) == 0);
    CHECK(!close_float(
        values(in_place)[5], values(inverse)[5]));

    m[3] = 0.01f;
    CHECK(D3DMath_MatrixInvert(inverse, matrix) ==
          static_cast<std::int32_t>(0x80070057u));
    return 0;
}

static int test_quaternion_paths(void)
{
    DirectX::XMMATRIX matrix;
    DirectX::XMMATRIX expected;
    __m128 axis = _mm_setr_ps(0.0f, 0.0f, 1.0f, 19.0f);
    __m128 recovered = _mm_setr_ps(0.0f, 0.0f, 0.0f, 23.0f);
    float x;
    float y;
    float z;
    float w;
    float theta;

    D3DMath_QuaternionFromAngles(
        x, y, z, w, 0.0f, 0.0f, 0.0f);
    CHECK(x == 0.0f && y == 0.0f && z == 0.0f && w == 1.0f);

    D3DMath_QuaternionFromRotation(
        x, y, z, w, axis, 1.0f);
    CHECK(x == 0.0f && y == 0.0f);
    CHECK(close_float(z, std::sin(0.5f)));
    CHECK(close_float(w, std::cos(0.5f)));
    D3DMath_RotationFromQuaternion(recovered, theta, x, y, z, w);
    CHECK(close_float(theta, 1.0f));
    CHECK(close_float(values(recovered)[0], 0.0f));
    CHECK(close_float(values(recovered)[1], 0.0f));
    CHECK(close_float(values(recovered)[2], 1.0f));
    CHECK(values(recovered)[3] == 23.0f);

    identity(matrix);
    D3DMath_QuaternionFromMatrix(x, y, z, w, matrix);
    CHECK(x == 0.0f && y == 0.0f && z == 0.0f && w == 1.0f);

    x = 0.1f;
    y = 0.2f;
    z = 0.3f;
    w = 0.4f;
    identity(matrix);
    values(matrix)[0] = -1.0f;
    values(matrix)[5] = -1.0f;
    values(matrix)[10] = -1.0f;
    D3DMath_MatrixFromQuaternion(expected, x, y, z, w);
    D3DMath_QuaternionFromMatrix(x, y, z, w, matrix);
    CHECK(x == 0.1f && y == 0.2f && z == 0.3f && w == 0.4f);
    CHECK(std::memcmp(&matrix, &expected, sizeof(matrix)) == 0);
    return 0;
}

static int test_multiply_slerp_and_vector(void)
{
    DirectX::XMMATRIX matrix;
    __m128 source = _mm_setr_ps(2.0f, 3.0f, 4.0f, 9.0f);
    __m128 destination = _mm_setr_ps(-1.0f, -2.0f, -3.0f, 17.0f);
    float qx;
    float qy;
    float qz;
    float qw;

    D3DMath_QuaternionMultiply(
        qx, qy, qz, qw,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.1f, 0.2f, 0.3f, 0.4f);
    CHECK(close_float(qx, 0.1f));
    CHECK(close_float(qy, 0.2f));
    CHECK(close_float(qz, 0.3f));
    CHECK(close_float(qw, 0.4f));

    D3DMath_QuaternionSlerp(
        qx, qy, qz, qw,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f);
    CHECK(close_float(qz, 0.70710677f));
    CHECK(close_float(qw, 0.70710677f));

    identity(matrix);
    CHECK(D3DMath_VectorMatrixMultiply(
              destination, source, matrix) == 0);
    CHECK(values(destination)[0] == 2.0f);
    CHECK(values(destination)[1] == 3.0f);
    CHECK(values(destination)[2] == 4.0f);
    CHECK(values(destination)[3] == 17.0f);

    values(matrix)[15] = 0.0f;
    CHECK(D3DMath_VectorMatrixMultiply(
              destination, source, matrix) ==
          static_cast<std::int32_t>(0x80070057u));
    CHECK(values(destination)[3] == 17.0f);
    return 0;
}

int main()
{
    if (test_matrix_paths() != 0) {
        return 1;
    }
    if (test_quaternion_paths() != 0) {
        return 1;
    }
    if (test_multiply_slerp_and_vector() != 0) {
        return 1;
    }
    std::puts("d3dmath tests passed");
    return 0;
}
