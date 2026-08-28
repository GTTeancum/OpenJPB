#ifndef JPB_D3DUTIL_H
#define JPB_D3DUTIL_H

#ifndef DIRECTDRAW_VERSION
#define DIRECTDRAW_VERSION 0x0700
#endif
#ifndef DIRECT3D_VERSION
#define DIRECT3D_VERSION 0x0700
#endif

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>

void D3DUtil_InitLight(
    D3DLIGHT7 &light,
    D3DLIGHTTYPE type,
    float x,
    float y,
    float z);
void D3DUtil_InitMaterial(
    D3DMATERIAL7 &material,
    float red,
    float green,
    float blue,
    float alpha);
void D3DUtil_InitSurfaceDesc(
    DDSURFACEDESC2 &surface,
    unsigned long flags,
    unsigned long caps);
HRESULT D3DUtil_SetProjectionMatrix(
    D3DMATRIX &matrix,
    float field_of_view,
    float aspect,
    float near_plane,
    float far_plane);
void D3DUtil_SetRotateXMatrix(D3DMATRIX &matrix, float radians);
void D3DUtil_SetRotateYMatrix(D3DMATRIX &matrix, float radians);
void D3DUtil_SetRotateZMatrix(D3DMATRIX &matrix, float radians);
void D3DUtil_SetRotationMatrix(
    D3DMATRIX &matrix,
    D3DVECTOR &axis,
    float radians);
HRESULT D3DUtil_SetViewMatrix(
    D3DMATRIX &matrix,
    D3DVECTOR &from,
    D3DVECTOR &at,
    D3DVECTOR &world_up);
HRESULT _DbgOut(
    char *file,
    unsigned long line,
    HRESULT result,
    char *message);

#endif
