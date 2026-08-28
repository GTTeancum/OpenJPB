#ifndef JPB_D3DENUM_H
#define JPB_D3DENUM_H

#include "jpb/d3dutil.h"

#include <stddef.h>

#if defined(__cplusplus)
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <vector>
#endif

enum {
    JPB_D3DENUM_DEVICE_CAPACITY = 20
};

typedef struct D3DEnum_DeviceInfo {
    char strDesc[40];
    GUID *pDeviceGUID;
    D3DDEVICEDESC7 ddDeviceDesc;
    BOOL bHardware;
    GUID *pDriverGUID;
    DDCAPS ddDriverCaps;
    DDCAPS ddHELCaps;
    DDSURFACEDESC2 ddsdFullscreenMode;
    BOOL bWindowed;
    GUID guidDevice;
    GUID guidDriver;
    DDSURFACEDESC2 *pddsdModes;
    DWORD dwNumModes;
    DWORD dwCurrentMode;
    BOOL bDesktopCompatible;
} D3DEnum_DeviceInfo;

typedef HRESULT (*D3DEnum_ConfirmDeviceCallback)(
    DDCAPS *driver_caps,
    D3DDEVICEDESC7 *device_description);

void D3DEnum_FreeResources(void);
#if defined(__cplusplus)
HRESULT D3DEnum_GetAdapters(
    std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> &adapters);
HRESULT D3DEnum_SelectDefaultAdapter(
    Microsoft::WRL::ComPtr<IDXGIAdapter1> &default_adapter,
    DWORD flags);
#endif
void D3DEnum_GetDevices(
    D3DEnum_DeviceInfo **devices,
    DWORD *count);
HRESULT D3DEnum_SelectDefaultDevice(
    D3DEnum_DeviceInfo **device,
    DWORD flags);
HRESULT EnumerateD3DDevices(
    D3DEnum_ConfirmDeviceCallback confirm_device,
    DWORD width,
    DWORD height,
    DWORD bits_per_pixel);
int SortModesCallback(const void *first, const void *second);

/* Focused access to the two file-local retail owners. */
HRESULT jpb_D3DEnumModeCallbackForTest(
    DDSURFACEDESC2 *mode,
    D3DEnum_DeviceInfo *device);
HRESULT jpb_D3DEnumDeviceCallbackForTest(
    char *description,
    char *name,
    D3DDEVICEDESC7 *device_description,
    D3DEnum_DeviceInfo *parent);
void jpb_D3DEnumResetForTest(DWORD device_count);
DWORD jpb_D3DEnumGetEnumeratedCountForTest(void);
void jpb_D3DEnumSetDefaultsForTest(
    DWORD width,
    DWORD height,
    DWORD bits_per_pixel);

#if defined(__cplusplus)
static_assert(sizeof(D3DEnum_DeviceInfo) == 1256,
              "D3DEnum_DeviceInfo PDB size changed");
static_assert(offsetof(D3DEnum_DeviceInfo, pDeviceGUID) == 40,
              "D3DEnum_DeviceInfo device GUID offset changed");
static_assert(offsetof(D3DEnum_DeviceInfo, ddDeviceDesc) == 48,
              "D3DEnum_DeviceInfo device description offset changed");
static_assert(offsetof(D3DEnum_DeviceInfo, pddsdModes) == 1232,
              "D3DEnum_DeviceInfo mode pointer offset changed");
static_assert(offsetof(D3DEnum_DeviceInfo, dwNumModes) == 1240,
              "D3DEnum_DeviceInfo mode count offset changed");
static_assert(offsetof(D3DEnum_DeviceInfo, bDesktopCompatible) == 1248,
              "D3DEnum_DeviceInfo desktop flag offset changed");
#endif

#endif
