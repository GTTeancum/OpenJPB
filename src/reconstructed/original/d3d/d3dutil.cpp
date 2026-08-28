#include "jpb/d3dutil.h"

#include <cmath>
#include <cfloat>
#include <cstdint>
#include <cstring>

/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\d3d\d3dutil.cpp.
 *
 * All ten PDB procedures were checked against direct shipped-executable
 * instructions, including exact structure extents, floating-point operation
 * order, unordered comparisons, degenerate view-axis recovery, alias-sensitive
 * matrix writes, and debug-output formatting.
 *
 * PDB module: 0027
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\d3dutil.obj
 * Compiler language: c++
 */

namespace {

constexpr std::uint32_t kViewToleranceBits = 0x358637bdu;
constexpr std::uint32_t kProjectionToleranceBits = 0x3c23d70au;

float from_bits(std::uint32_t bits)
{
    float value;

    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

float *values(D3DMATRIX &matrix)
{
    return reinterpret_cast<float *>(&matrix);
}

float vector_length(float x, float y, float z)
{
    return static_cast<float>(
        std::sqrt(static_cast<double>(x * x + y * y + z * z)));
}

} // namespace

static_assert(sizeof(D3DLIGHT7) == 104);
static_assert(sizeof(D3DMATERIAL7) == 68);
static_assert(sizeof(DDSURFACEDESC2) == 136);
static_assert(sizeof(D3DMATRIX) == 64);
static_assert(sizeof(D3DVECTOR) == 12);

void D3DUtil_InitLight(
    D3DLIGHT7 &light,
    D3DLIGHTTYPE type,
    float x,
    float y,
    float z)
{
    std::memset(&light, 0, sizeof(light));
    light.dcvDiffuse.r = 1.0f;
    light.dcvDiffuse.g = 1.0f;
    light.dcvDiffuse.b = 1.0f;
    light.dltType = type;
    light.dvDirection.x = x;
    light.dcvSpecular = light.dcvDiffuse;
    light.dvAttenuation0 = 1.0f;
    light.dvDirection.z = z;
    light.dvPosition.z = z;
    light.dvRange = std::sqrt(FLT_MAX);
    light.dvPosition.x = x;
    light.dvDirection.y = y;
    light.dvPosition.y = y;
}

void D3DUtil_InitMaterial(
    D3DMATERIAL7 &material,
    float red,
    float green,
    float blue,
    float alpha)
{
    std::memset(&material, 0, sizeof(material));
    material.dcvAmbient.r = red;
    material.dcvDiffuse.r = red;
    material.dcvAmbient.g = green;
    material.dcvDiffuse.g = green;
    material.dcvAmbient.b = blue;
    material.dcvDiffuse.b = blue;
    material.dcvAmbient.a = alpha;
    material.dcvDiffuse.a = alpha;
}

void D3DUtil_InitSurfaceDesc(
    DDSURFACEDESC2 &surface,
    unsigned long flags,
    unsigned long caps)
{
    std::memset(&surface, 0, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = flags;
    surface.ddsCaps.dwCaps = caps;
    surface.ddpfPixelFormat.dwSize = sizeof(surface.ddpfPixelFormat);
}

HRESULT D3DUtil_SetProjectionMatrix(
    D3DMATRIX &matrix,
    float field_of_view,
    float aspect,
    float near_plane,
    float far_plane)
{
    const float tolerance = from_bits(kProjectionToleranceBits);
    const float depth = far_plane - near_plane;
    float *m;
    float half_fov;
    float sine;
    float cotangent;
    float q;

    if (std::fabs(depth) < tolerance) {
        return E_INVALIDARG;
    }
    half_fov = field_of_view * 0.5f;
    sine = std::sin(half_fov);
    if (std::fabs(sine) < tolerance) {
        return E_INVALIDARG;
    }

    m = values(matrix);
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[15] = 0.0f;
    q = far_plane / depth;
    cotangent = std::cos(half_fov) / std::sin(half_fov);
    m[11] = 1.0f;
    m[10] = q;
    m[0] = cotangent * aspect;
    m[5] = cotangent;
    m[14] = -q * near_plane;
    return S_OK;
}

void D3DUtil_SetRotateXMatrix(D3DMATRIX &matrix, float radians)
{
    float *m = values(matrix);

    std::memset(m, 0, sizeof(matrix));
    m[15] = 1.0f;
    m[0] = 1.0f;
    m[5] = std::cos(radians);
    m[6] = std::sin(radians);
    m[9] = -std::sin(radians);
    m[10] = std::cos(radians);
}

void D3DUtil_SetRotateYMatrix(D3DMATRIX &matrix, float radians)
{
    float *m = values(matrix);

    std::memset(m, 0, sizeof(matrix));
    m[15] = 1.0f;
    m[5] = 1.0f;
    m[0] = std::cos(radians);
    m[2] = -std::sin(radians);
    m[8] = std::sin(radians);
    m[10] = std::cos(radians);
}

void D3DUtil_SetRotateZMatrix(D3DMATRIX &matrix, float radians)
{
    float *m = values(matrix);

    std::memset(m, 0, sizeof(matrix));
    m[15] = 1.0f;
    m[10] = 1.0f;
    m[0] = std::cos(radians);
    m[1] = std::sin(radians);
    m[4] = -std::sin(radians);
    m[5] = std::cos(radians);
}

void D3DUtil_SetRotationMatrix(
    D3DMATRIX &matrix,
    D3DVECTOR &axis,
    float radians)
{
    float *m = values(matrix);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    const float length = vector_length(axis.x, axis.y, axis.z);
    const float one_minus_cosine = 1.0f - cosine;
    const float x = axis.x / length;
    const float y = axis.y / length;
    const float z = axis.z / length;
    const float xy = y * x * one_minus_cosine;
    const float xz = z * x * one_minus_cosine;
    const float yz = y * z * one_minus_cosine;

    m[11] = 0.0f;
    m[12] = 0.0f;
    m[7] = 0.0f;
    m[3] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
    m[0] = x * x * one_minus_cosine + cosine;
    m[1] = xy - z * sine;
    m[4] = z * sine + xy;
    m[2] = y * sine + xz;
    m[8] = xz - y * sine;
    m[5] = y * y * one_minus_cosine + cosine;
    m[6] = yz - x * sine;
    m[9] = x * sine + yz;
    m[10] = z * z * one_minus_cosine + cosine;
}

HRESULT D3DUtil_SetViewMatrix(
    D3DMATRIX &matrix,
    D3DVECTOR &from,
    D3DVECTOR &at,
    D3DVECTOR &world_up)
{
    const float tolerance = from_bits(kViewToleranceBits);
    float view_x = at.x - from.x;
    float view_y = at.y - from.y;
    float view_z = at.z - from.z;
    float length = vector_length(view_x, view_y, view_z);
    float up_x;
    float up_y;
    float up_z;
    float dot;
    float right_x;
    float right_y;
    float right_z;
    float *m;

    if (length < tolerance) {
        return E_INVALIDARG;
    }
    view_x /= length;
    view_y /= length;
    view_z /= length;

    dot = world_up.y * view_y + world_up.x * view_x +
          world_up.z * view_z;
    up_z = world_up.z - dot * view_z;
    up_x = world_up.x - dot * view_x;
    up_y = world_up.y - dot * view_y;
    length = vector_length(up_x, up_y, up_z);
    if (length < tolerance) {
        up_x = -view_y * view_x;
        up_y = 1.0f - view_y * view_y;
        up_z = -view_y * view_z;
        length = vector_length(up_x, up_y, up_z);
        if (length < tolerance) {
            const float yz = up_z;

            up_x = -view_z * view_x;
            up_z = 1.0f - view_z * view_z;
            up_y = yz;
            length = vector_length(up_x, up_y, up_z);
            if (length < tolerance) {
                return E_INVALIDARG;
            }
        }
    }

    up_x /= length;
    up_y /= length;
    up_z /= length;
    m = values(matrix);
    m[15] = 1.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[7] = 0.0f;
    m[3] = 0.0f;
    m[2] = view_x;
    m[1] = up_x;
    m[5] = up_y;
    m[6] = view_y;
    m[9] = up_z;
    m[10] = view_z;
    right_x = view_z * up_y - view_y * up_z;
    right_y = view_x * up_z - view_z * up_x;
    right_z = view_y * up_x - view_x * up_y;
    m[0] = right_x;
    m[4] = right_y;
    m[8] = right_z;
    m[12] = -(right_x * from.x + right_y * from.y + right_z * from.z);
    m[13] = -(up_x * from.x + up_y * from.y + up_z * from.z);
    m[14] = -(view_x * from.x + view_y * from.y + view_z * from.z);
    return S_OK;
}

HRESULT _DbgOut(
    char *file,
    unsigned long line,
    HRESULT result,
    char *message)
{
    char buffer[256];

    wsprintfA(buffer, "%s(%ld): ", file, line);
    OutputDebugStringA(buffer);
    OutputDebugStringA(message);
    if (result != S_OK) {
        wsprintfA(buffer, "(hr=%08lx)\n", result);
        OutputDebugStringA(buffer);
    }
    OutputDebugStringA("\n");
    return result;
}
