/*
 * REVIEWED RECONSTRUCTION.
 * PDB module: 0024
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\d3dmath.obj
 * Primary source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 * Compiler language: c++
 * Emitted procedures: 10
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/d3dmath.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace {

constexpr std::int32_t kInvalidArgument =
    static_cast<std::int32_t>(0x80070057u);

float *matrix_values(DirectX::XMMATRIX &matrix)
{
    return reinterpret_cast<float *>(&matrix);
}

float *vector_values(__m128 &vector)
{
    return reinterpret_cast<float *>(&vector);
}

float float_from_bits(std::uint32_t bits)
{
    float value;

    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

float absolute_bits(float value)
{
    std::uint32_t bits;

    std::memcpy(&bits, &value, sizeof(bits));
    bits &= UINT32_C(0x7fffffff);
    return float_from_bits(bits);
}

float toggle_sign(float value)
{
    std::uint32_t bits;

    std::memcpy(&bits, &value, sizeof(bits));
    bits ^= UINT32_C(0x80000000);
    return float_from_bits(bits);
}

} // namespace

/* 0x3D050, 341 bytes, global, 14 named locals
 * D3DMath_MatrixFromQuaternion
 * PDB type: void (DirectX::XMMATRIX&, float,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 */
void D3DMath_MatrixFromQuaternion(
    DirectX::XMMATRIX &mat,
    float x,
    float y,
    float z,
    float w)
{
    float *m = matrix_values(mat);
    float zz = z * z;
    float yz = y * z;
    float xz = x * z;
    float wy = w * y;
    float xx = x * x;
    float yy = y * y;
    float wx = w * x;
    float wz = w * z;
    float xy = x * y;

    m[15] = 1.0f;
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[7] = 0.0f;
    m[3] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[0] = 1.0f - 2.0f * (zz + yy);
    m[1] = 2.0f * (xy - wz);
    m[4] = 2.0f * (wz + xy);
    m[2] = 2.0f * (wy + xz);
    m[8] = 2.0f * (xz - wy);
    m[10] = 1.0f - 2.0f * (yy + xx);
    m[5] = 1.0f - 2.0f * (zz + xx);
    m[9] = 2.0f * (wx + yz);
    m[6] = 2.0f * (yz - wx);
}

/* 0x3D1B0, 711 bytes, global, 4 named locals
 * D3DMath_MatrixInvert
 * PDB type: HRESULT (DirectX::XMMATRIX&, Dir...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 */
std::int32_t D3DMath_MatrixInvert(
    DirectX::XMMATRIX &q,
    DirectX::XMMATRIX &a)
{
    float *out = matrix_values(q);
    float *in = matrix_values(a);
    const float epsilon = float_from_bits(UINT32_C(0x3a83126f));
    float f_det_inv;
    float negative_det_inv;

    if (absolute_bits(in[15] - 1.0f) > epsilon ||
        absolute_bits(in[3]) > epsilon ||
        absolute_bits(in[7]) > epsilon ||
        absolute_bits(in[11]) > epsilon) {
        return kInvalidArgument;
    }

    f_det_inv = 1.0f /
        (((in[5] * in[10] - in[6] * in[9]) * in[0] -
          (in[4] * in[10] - in[8] * in[6]) * in[1]) +
         (in[4] * in[9] - in[8] * in[5]) * in[2]);
    out[0] = (in[5] * in[10] - in[6] * in[9]) * f_det_inv;
    negative_det_inv = toggle_sign(f_det_inv);
    out[1] =
        (in[10] * in[1] - in[2] * in[9]) * negative_det_inv;
    out[3] = 0.0f;
    out[2] = (in[6] * in[1] - in[2] * in[5]) * f_det_inv;
    out[4] =
        (in[10] * in[4] - in[8] * in[6]) * negative_det_inv;
    out[5] = (in[10] * in[0] - in[2] * in[8]) * f_det_inv;
    out[7] = 0.0f;
    out[6] =
        (in[6] * in[0] - in[2] * in[4]) * negative_det_inv;
    out[8] = (in[4] * in[9] - in[8] * in[5]) * f_det_inv;
    out[9] =
        (in[0] * in[9] - in[8] * in[1]) * negative_det_inv;
    out[11] = 0.0f;
    out[10] = (in[5] * in[0] - in[4] * in[1]) * f_det_inv;
    out[12] = toggle_sign(
        out[4] * in[13] + out[0] * in[12] + out[8] * in[14]);
    out[13] = toggle_sign(
        out[5] * in[13] + out[1] * in[12] + out[9] * in[14]);
    out[15] = 1.0f;
    out[14] = toggle_sign(
        out[2] * in[12] + out[6] * in[13] + out[10] * in[14]);
    return 0;
}

/* 0x3D480, 254 bytes, global, 7 named locals
 * D3DMath_MatrixMultiply
 * PDB type: void (DirectX::XMMATRIX&, Direct...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 */
void D3DMath_MatrixMultiply(
    DirectX::XMMATRIX &q,
    DirectX::XMMATRIX &a,
    DirectX::XMMATRIX &b)
{
    DirectX::XMMATRIX result;
    float *out = matrix_values(result);
    float *left = matrix_values(a);
    float *right = matrix_values(b);
    unsigned short i;

    std::memset(out, 0, sizeof(result));
    for (i = 0; i < 4; ++i) {
        unsigned short j;

        for (j = 0; j < 4; ++j) {
            unsigned short k;

            for (k = 0; k < 4; ++k) {
                out[i * 4 + j] +=
                    right[k * 4 + j] * left[i * 4 + k];
            }
        }
    }
    q = result;
}

/* 0x3D580, 365 bytes, global, 13 named locals
 * D3DMath_QuaternionFromAngles
 * PDB type: void (float&, float&, float&, fl...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 */
void D3DMath_QuaternionFromAngles(
    float &x,
    float &y,
    float &z,
    float &w,
    float yaw,
    float pitch,
    float roll)
{
    float sin_yaw = static_cast<float>(std::sin(yaw * 0.5f));
    float sin_pitch = static_cast<float>(std::sin(pitch * 0.5f));
    float sin_roll = static_cast<float>(std::sin(roll * 0.5f));
    float cos_yaw = static_cast<float>(std::cos(yaw * 0.5f));
    float cos_pitch = static_cast<float>(std::cos(pitch * 0.5f));
    float cos_roll = static_cast<float>(std::cos(roll * 0.5f));

    x = cos_pitch * sin_roll * cos_yaw -
        cos_roll * sin_pitch * sin_yaw;
    y = cos_roll * sin_pitch * cos_yaw +
        cos_pitch * sin_roll * sin_yaw;
    z = cos_roll * cos_pitch * sin_yaw -
        sin_roll * sin_pitch * cos_yaw;
    w = cos_roll * cos_pitch * cos_yaw +
        sin_roll * sin_pitch * sin_yaw;
}

/* 0x3D6F0, 569 bytes, global, 15 named locals
 * D3DMath_QuaternionFromMatrix
 * PDB type: void (float&, float&, float&, fl...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 */
void D3DMath_QuaternionFromMatrix(
    float &x,
    float &y,
    float &z,
    float &w,
    DirectX::XMMATRIX &mat)
{
    float *m = matrix_values(mat);
    float trace = m[0] + m[5] + m[10];

    if (trace > 0.0f) {
        float s = static_cast<float>(std::sqrt(trace + m[15]));
        float divisor = s + s;

        x = (m[6] - m[9]) / divisor;
        y = (m[8] - m[2]) / divisor;
        z = (m[1] - m[4]) / divisor;
        w = s * 0.5f;
    }

    D3DMath_MatrixFromQuaternion(mat, x, y, z, w);
}

/* 0x3D930, 158 bytes, global, 6 named locals
 * D3DMath_QuaternionFromRotation
 * PDB type: void (float&, float&, float&, fl...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 */
void D3DMath_QuaternionFromRotation(
    float &x,
    float &y,
    float &z,
    float &w,
    __m128 &v,
    float theta)
{
    float *axis = vector_values(v);
    float half_theta = theta * 0.5f;

    x = static_cast<float>(std::sin(half_theta)) * axis[0];
    y = static_cast<float>(std::sin(half_theta)) * axis[1];
    z = static_cast<float>(std::sin(half_theta)) * axis[2];
    w = static_cast<float>(std::cos(half_theta));
}

/* 0x3D9D0, 337 bytes, global, 16 named locals
 * D3DMath_QuaternionMultiply
 * PDB type: void (float&, float&, float&, fl...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 */
void D3DMath_QuaternionMultiply(
    float &qx,
    float &qy,
    float &qz,
    float &qw,
    float ax,
    float ay,
    float az,
    float aw,
    float bx,
    float by,
    float bz,
    float bw)
{
    qx = ((ay * bz + ax * bw) - az * by) + aw * bx;
    qy = (ay * bw - ax * bz) + az * bx + aw * by;
    qz = (ax * by - ay * bx) + az * bw + aw * bz;
    qw = ((toggle_sign(ax) * bx - ay * by) - az * bz) + aw * bw;
}

/* 0x3DB30, 484 bytes, global, 16 named locals
 * D3DMath_QuaternionSlerp
 * PDB type: void (float&, float&, float&, fl...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 */
void D3DMath_QuaternionSlerp(
    float &qx,
    float &qy,
    float &qz,
    float &qw,
    float ax,
    float ay,
    float az,
    float aw,
    float bx,
    float by,
    float bz,
    float bw,
    float alpha)
{
    const float epsilon = float_from_bits(UINT32_C(0x3a83126f));
    float cos_theta = ax * bx + ay * by + az * bz + aw * bw;
    float beta;

    if (cos_theta < 0.0f) {
        cos_theta = toggle_sign(cos_theta);
        bx = toggle_sign(bx);
        by = toggle_sign(by);
        bz = toggle_sign(bz);
        bw = toggle_sign(bw);
    }
    beta = 1.0f - alpha;
    if (1.0f - cos_theta > epsilon) {
        float theta = static_cast<float>(std::acos(cos_theta));

        beta = static_cast<float>(std::sin(theta * beta)) /
            static_cast<float>(std::sin(theta));
        alpha = static_cast<float>(std::sin(theta * alpha)) /
            static_cast<float>(std::sin(theta));
    }
    qx = beta * ax + bx * alpha;
    qy = beta * ay + by * alpha;
    qz = beta * az + bz * alpha;
    qw = beta * aw + bw * alpha;
}

/* 0x3DD20, 156 bytes, global, 6 named locals
 * D3DMath_RotationFromQuaternion
 * PDB type: void (__m128&, float&, float, fl...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 */
void D3DMath_RotationFromQuaternion(
    __m128 &v,
    float &theta,
    float x,
    float y,
    float z,
    float w)
{
    float *axis = vector_values(v);
    float half_sine;

    theta = static_cast<float>(std::acos(w)) * 2.0f;
    half_sine = static_cast<float>(std::sin(theta * 0.5f));
    axis[0] = x / half_sine;
    half_sine = static_cast<float>(std::sin(theta * 0.5f));
    axis[1] = y / half_sine;
    half_sine = static_cast<float>(std::sin(theta * 0.5f));
    axis[2] = z / half_sine;
}

/* 0x3DDC0, 265 bytes, global, 7 named locals
 * D3DMath_VectorMatrixMultiply
 * PDB type: HRESULT (__m128&, __m128&, Direc...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dmath.cpp
 */
std::int32_t D3DMath_VectorMatrixMultiply(
    __m128 &destination,
    __m128 &source,
    DirectX::XMMATRIX &mat)
{
    float *dest = vector_values(destination);
    float *src = vector_values(source);
    float *m = matrix_values(mat);
    const float epsilon = float_from_bits(UINT32_C(0x3727c5ac));
    float x = src[0];
    float y = src[1];
    float z = src[2];
    float w = x * m[3] + y * m[7] + z * m[11] + m[15];

    if (absolute_bits(w) < epsilon) {
        return kInvalidArgument;
    }
    dest[1] = (x * m[1] + y * m[5] + z * m[9] + m[13]) / w;
    dest[2] = (x * m[2] + y * m[6] + z * m[10] + m[14]) / w;
    dest[0] = (y * m[4] + x * m[0] + z * m[8] + m[12]) / w;
    return 0;
}
