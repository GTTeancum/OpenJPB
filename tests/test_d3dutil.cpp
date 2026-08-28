#include "jpb/d3dutil.h"

#include <cmath>
#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

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

static float *values(D3DMATRIX &matrix)
{
    return reinterpret_cast<float *>(&matrix);
}

static bool close_float(float left, float right)
{
    return std::fabs(left - right) < 0.0001f;
}

static float float_from_bits(std::uint32_t bits)
{
    float value;

    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

static int test_initializers(void)
{
    D3DLIGHT7 light;
    D3DMATERIAL7 material;
    DDSURFACEDESC2 surface;

    std::memset(&light, 0x7f, sizeof(light));
    D3DUtil_InitLight(light, D3DLIGHT_POINT, 2.0f, 3.0f, 4.0f);
    CHECK(light.dltType == D3DLIGHT_POINT);
    CHECK(light.dcvDiffuse.r == 1.0f);
    CHECK(light.dcvDiffuse.g == 1.0f);
    CHECK(light.dcvDiffuse.b == 1.0f);
    CHECK(light.dcvDiffuse.a == 0.0f);
    CHECK(std::memcmp(
              &light.dcvDiffuse,
              &light.dcvSpecular,
              sizeof(light.dcvDiffuse)) == 0);
    CHECK(light.dvPosition.x == 2.0f && light.dvDirection.x == 2.0f);
    CHECK(light.dvPosition.y == 3.0f && light.dvDirection.y == 3.0f);
    CHECK(light.dvPosition.z == 4.0f && light.dvDirection.z == 4.0f);
    CHECK(light.dvRange == std::sqrt(FLT_MAX));
    CHECK(light.dvAttenuation0 == 1.0f);
    CHECK(light.dvFalloff == 0.0f && light.dvPhi == 0.0f);

    std::memset(&material, 0x7f, sizeof(material));
    D3DUtil_InitMaterial(material, 0.1f, 0.2f, 0.3f, 0.4f);
    CHECK(material.dcvDiffuse.r == 0.1f);
    CHECK(material.dcvDiffuse.g == 0.2f);
    CHECK(material.dcvDiffuse.b == 0.3f);
    CHECK(material.dcvDiffuse.a == 0.4f);
    CHECK(std::memcmp(
              &material.dcvDiffuse,
              &material.dcvAmbient,
              sizeof(material.dcvDiffuse)) == 0);
    CHECK(material.dcvSpecular.r == 0.0f);
    CHECK(material.dcvEmissive.a == 0.0f);
    CHECK(material.dvPower == 0.0f);

    std::memset(&surface, 0x7f, sizeof(surface));
    D3DUtil_InitSurfaceDesc(surface, 0x1234u, 0x5678u);
    CHECK(surface.dwSize == sizeof(surface));
    CHECK(surface.dwFlags == 0x1234u);
    CHECK(surface.ddsCaps.dwCaps == 0x5678u);
    CHECK(surface.ddpfPixelFormat.dwSize == sizeof(surface.ddpfPixelFormat));
    CHECK(surface.dwHeight == 0u && surface.dwTextureStage == 0u);
    return 0;
}

static int test_projection(void)
{
    D3DMATRIX matrix;
    float *m = values(matrix);
    const float half_pi = 1.57079632679489661923f;
    const float tolerance = float_from_bits(UINT32_C(0x3c23d70a));
    const float quiet_nan = std::numeric_limits<float>::quiet_NaN();

    std::memset(&matrix, 0x7f, sizeof(matrix));
    CHECK(D3DUtil_SetProjectionMatrix(
              matrix, half_pi, 1.5f, 1.0f, 101.0f) == S_OK);
    CHECK(close_float(m[0], 1.5f));
    CHECK(close_float(m[5], 1.0f));
    CHECK(close_float(m[10], 1.01f));
    CHECK(m[11] == 1.0f);
    CHECK(close_float(m[14], -1.01f));
    CHECK(m[1] == 0.0f && m[15] == 0.0f);
    CHECK(D3DUtil_SetProjectionMatrix(
              matrix, half_pi, 1.0f, 1.0f, 1.005f) == E_INVALIDARG);
    CHECK(D3DUtil_SetProjectionMatrix(
              matrix, 0.001f, 1.0f, 1.0f, 100.0f) == E_INVALIDARG);
    CHECK(D3DUtil_SetProjectionMatrix(
              matrix, half_pi, 1.0f, 0.0f, tolerance) == S_OK);
    CHECK(D3DUtil_SetProjectionMatrix(
              matrix,
              half_pi,
              1.0f,
              0.0f,
              std::nextafter(tolerance, 0.0f)) == E_INVALIDARG);
    CHECK(D3DUtil_SetProjectionMatrix(
              matrix, half_pi, 1.0f, 0.0f, quiet_nan) == S_OK);
    CHECK(std::isnan(m[10]));
    CHECK(D3DUtil_SetProjectionMatrix(
              matrix, quiet_nan, 1.0f, 1.0f, 100.0f) == S_OK);
    CHECK(std::isnan(m[0]));
    return 0;
}

static int test_rotation(void)
{
    D3DMATRIX x_matrix;
    D3DMATRIX y_matrix;
    D3DMATRIX z_matrix;
    D3DMATRIX axis_matrix;
    D3DVECTOR axis = {0.0f, 0.0f, 2.0f};
    float *x = values(x_matrix);
    float *y = values(y_matrix);
    float *z = values(z_matrix);
    float *a = values(axis_matrix);
    const float half_pi = 1.57079632679489661923f;

    D3DUtil_SetRotateXMatrix(x_matrix, half_pi);
    CHECK(x[0] == 1.0f && x[15] == 1.0f);
    CHECK(close_float(x[6], 1.0f) && close_float(x[9], -1.0f));

    D3DUtil_SetRotateYMatrix(y_matrix, half_pi);
    CHECK(y[5] == 1.0f && y[15] == 1.0f);
    CHECK(close_float(y[2], -1.0f) && close_float(y[8], 1.0f));

    D3DUtil_SetRotateZMatrix(z_matrix, half_pi);
    CHECK(z[10] == 1.0f && z[15] == 1.0f);
    CHECK(close_float(z[1], 1.0f) && close_float(z[4], -1.0f));

    D3DUtil_SetRotationMatrix(axis_matrix, axis, half_pi);
    CHECK(close_float(a[0], 0.0f));
    CHECK(close_float(a[1], -1.0f));
    CHECK(close_float(a[4], 1.0f));
    CHECK(close_float(a[5], 0.0f));
    CHECK(close_float(a[10], 1.0f));
    CHECK(close_float(a[15], 1.0f));

    axis = {0.0f, 0.0f, 0.0f};
    D3DUtil_SetRotationMatrix(axis_matrix, axis, half_pi);
    CHECK(std::isnan(a[0]));
    CHECK(std::isnan(a[10]));
    CHECK(a[15] == 1.0f);
    return 0;
}

static int test_view(void)
{
    D3DMATRIX matrix;
    D3DVECTOR from = {0.0f, 0.0f, -5.0f};
    D3DVECTOR at = {0.0f, 0.0f, 0.0f};
    D3DVECTOR up = {0.0f, 1.0f, 0.0f};
    float *m = values(matrix);
    const float tolerance = float_from_bits(UINT32_C(0x358637bd));
    const float quiet_nan = std::numeric_limits<float>::quiet_NaN();

    CHECK(D3DUtil_SetViewMatrix(matrix, from, at, up) == S_OK);
    CHECK(close_float(m[0], 1.0f));
    CHECK(close_float(m[5], 1.0f));
    CHECK(close_float(m[10], 1.0f));
    CHECK(close_float(m[14], 5.0f));
    CHECK(m[15] == 1.0f);

    from = {0.0f, 0.0f, 0.0f};
    at = {0.0f, 1.0f, 0.0f};
    up = {0.0f, 1.0f, 0.0f};
    CHECK(D3DUtil_SetViewMatrix(matrix, from, at, up) == S_OK);
    CHECK(close_float(m[0], -1.0f));
    CHECK(close_float(m[9], 1.0f));
    CHECK(close_float(m[6], 1.0f));

    at = from;
    CHECK(D3DUtil_SetViewMatrix(matrix, from, at, up) == E_INVALIDARG);

    at = {tolerance, 0.0f, 0.0f};
    CHECK(D3DUtil_SetViewMatrix(matrix, from, at, up) == S_OK);
    at.x = std::nextafter(tolerance, 0.0f);
    CHECK(D3DUtil_SetViewMatrix(matrix, from, at, up) == E_INVALIDARG);
    at = {quiet_nan, 0.0f, 0.0f};
    CHECK(D3DUtil_SetViewMatrix(matrix, from, at, up) == S_OK);
    CHECK(std::isnan(m[0]));
    return 0;
}

int main(void)
{
    char file[] = "d3dutil.cpp";
    char message[] = "test";

    CHECK(test_initializers() == 0);
    CHECK(test_projection() == 0);
    CHECK(test_rotation() == 0);
    CHECK(test_view() == 0);
    CHECK(_DbgOut(file, 42, E_FAIL, message) == E_FAIL);
    return 0;
}
