/*
 * COMPLETE REVIEWED RECONSTRUCTION.
 * PDB module: 0021
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\d3denum.obj
 * Primary source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 * Compiler language: c++
 * Emitted procedures: 25
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/d3denum.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <new>
#include <vector>

using Microsoft::WRL::ComPtr;

static D3DEnum_DeviceInfo g_pDeviceList[JPB_D3DENUM_DEVICE_CAPACITY];
static DWORD g_dwNumDevicesEnumerated;
static DWORD g_dwNumDevices;
static DWORD dwDefaultWidth;
static DWORD dwDefaultHeight;
static DWORD dwDefaultBitsPixel;

/* 0x39B90, 40 bytes, global, 2 named locals
 * IID_PPV_ARGS_Helper<Microsoft::WRL::ComPtr<IDXGIFactory6> >
 * PDB type: void** (Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x39BC0, 75 bytes, global, 4 named locals
 * std::_Destroy_range<std::allocator<Microsoft::WRL::ComPtr<IDXGIAdapter1> > >
 * PDB type: void (Microsoft::WRL::ComPtr<IDX...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x39C10, 629 bytes, global, 29 named locals
 * std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>,std::allocator<Microsoft::WRL::ComPtr<IDXGIAdapter1> > >::_Emplace_reallocate<Microsoft::WRL::ComPtr<IDXGIAdapter1> const &>
 * PDB type: Microsoft::WRL::ComPtr<IDXGIAdap...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x39E90, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IDXGIFactory6>::~ComPtr<IDXGIFactory6>
 * PDB type: void Microsoft::WRL::ComPtr<IDXG...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x39EC0, 165 bytes, global, 8 named locals
 * std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>,std::allocator<Microsoft::WRL::ComPtr<IDXGIAdapter1> > >::~vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>,std::allocator<Microsoft::WRL::ComPtr<IDXGIAdapter1> > >
 * PDB type: void std::vector<Microsoft::WRL:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x39F70, 113 bytes, global, 1 named locals
 * D3DEnum_FreeResources
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 */
void D3DEnum_FreeResources(void)
{
    for (DWORD index = 0; index < g_dwNumDevices; ++index) {
        if (g_pDeviceList[index].pddsdModes != nullptr) {
            ::operator delete(
                g_pDeviceList[index].pddsdModes,
                sizeof(DDSURFACEDESC2));
            g_pDeviceList[index].pddsdModes = nullptr;
        }
    }
}

/* 0x39FF0, 266 bytes, global, 8 named locals
 * D3DEnum_GetAdapters
 * PDB type: HRESULT (std::vector<Microsoft::...
 * Source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 */

/* 0x3A100, 29 bytes, global, 2 named locals
 * D3DEnum_GetDevices
 * PDB type: void (D3DEnum_DeviceInfo**, unsi...
 * Source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 */
HRESULT D3DEnum_GetAdapters(
    std::vector<ComPtr<IDXGIAdapter1>> &adapters)
{
    ComPtr<IDXGIFactory6> factory;
    HRESULT result = CreateDXGIFactory1(
        IID_PPV_ARGS(factory.GetAddressOf()));

    if (FAILED(result)) {
        std::printf("Failed to create DXGI Factory\n");
        return result;
    }

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT index = 0;; ++index) {
        result = factory->EnumAdapters1(
            index, adapter.ReleaseAndGetAddressOf());
        if (result == DXGI_ERROR_NOT_FOUND) {
            return S_OK;
        }
        adapters.push_back(adapter);
    }
}
void D3DEnum_GetDevices(
    D3DEnum_DeviceInfo **devices,
    DWORD *count)
{
    if (devices != nullptr) {
        *devices = g_pDeviceList;
    }
    if (count != nullptr) {
        *count = g_dwNumDevices;
    }
}

/* 0x3A120, 854 bytes, global, 23 named locals
 * D3DEnum_SelectDefaultAdapter
 * PDB type: HRESULT (Microsoft::WRL::ComPtr<...
 * Source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 */

/* 0x3A480, 302 bytes, global, 8 named locals
 * D3DEnum_SelectDefaultDevice
 * PDB type: HRESULT (D3DEnum_DeviceInfo**, u...
 * Source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 */

/* 0x3A5B0, 1244 bytes, local, 9 named locals
 * DeviceEnumCallback
 * PDB type: HRESULT (char*, char*, _D3DDevic...
 * Source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 */

/* 0x3AA90, 441 bytes, local, 9 named locals
 * DriverEnumCallback
 * PDB type: int (_GUID*, char*, char*, void*...
 * Source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 */

/* 0x3AC50, 82 bytes, global, 4 named locals
 * EnumerateD3DDevices
 * PDB type: HRESULT (HRESULT (_DDCAPS_DX7*, ...
 * Source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 */

/* 0x3ACB0, 240 bytes, local, 3 named locals
 * ModeEnumCallback
 * PDB type: HRESULT (_DDSURFACEDESC2*, void*...
 * Source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 */
static HRESULT CALLBACK ModeEnumCallback(
    DDSURFACEDESC2 *mode,
    void *context);

static HRESULT CALLBACK DeviceEnumCallback(
    char *description,
    char *name,
    D3DDEVICEDESC7 *device_description,
    void *context)
{
    D3DEnum_DeviceInfo *parent =
        static_cast<D3DEnum_DeviceInfo *>(context);
    D3DEnum_DeviceInfo *device = &g_pDeviceList[g_dwNumDevices];

    (void)description;
    ++g_dwNumDevicesEnumerated;
    std::memset(device, 0, sizeof(*device));
    device->bHardware =
        device_description->dwDevCaps & D3DDEVCAPS_HWRASTERIZATION;
    std::memcpy(
        &device->ddDeviceDesc,
        device_description,
        sizeof(device->ddDeviceDesc));
    device->bDesktopCompatible = parent->bDesktopCompatible;
    std::memcpy(
        &device->ddDriverCaps,
        &parent->ddDriverCaps,
        sizeof(device->ddDriverCaps));
    std::memcpy(
        &device->ddHELCaps,
        &parent->ddHELCaps,
        sizeof(device->ddHELCaps));

    device->pDeviceGUID = &device->guidDevice;
    device->guidDevice = device_description->deviceGUID;
    device->pddsdModes = static_cast<DDSURFACEDESC2 *>(
        ::operator new[](
            static_cast<size_t>(parent->dwNumModes) *
            sizeof(DDSURFACEDESC2)));

    const char *display_name;
    if (parent->pDriverGUID != nullptr) {
        device->guidDriver = parent->guidDriver;
        device->pDriverGUID = &device->guidDriver;
        display_name = parent->strDesc;
    } else {
        device->pDriverGUID = nullptr;
        display_name = name;
    }
    lstrcpynA(device->strDesc, display_name, 39);

    if (device->pDriverGUID != nullptr && !device->bHardware) {
        return TRUE;
    }

    for (DWORD index = 0; index < parent->dwNumModes; ++index) {
        const DDSURFACEDESC2 *mode = &parent->pddsdModes[index];
        const DWORD bits = mode->ddpfPixelFormat.dwRGBBitCount;
        DWORD required_depth = 0;

        if (bits == 32) {
            required_depth = DDBD_32;
        } else if (bits == 24) {
            required_depth = DDBD_24;
        } else if (bits == 16) {
            required_depth = DDBD_16;
        }
        if ((device->ddDeviceDesc.dwDeviceRenderBitDepth &
             required_depth) != 0) {
            device->pddsdModes[device->dwNumModes] = *mode;
            ++device->dwNumModes;
        }
    }

    if (device->dwNumModes == 0) {
        return TRUE;
    }

    for (DWORD index = 0; index < device->dwNumModes; ++index) {
        const DDSURFACEDESC2 *mode = &device->pddsdModes[index];

        if (mode->dwWidth == dwDefaultWidth &&
            mode->dwHeight == dwDefaultHeight &&
            mode->ddpfPixelFormat.dwRGBBitCount ==
                dwDefaultBitsPixel) {
            device->ddsdFullscreenMode = *mode;
            device->dwCurrentMode = index;
        }
    }

    ++g_dwNumDevices;
    device->bWindowed = device->bDesktopCompatible;
    return TRUE;
}

static BOOL CALLBACK DriverEnumCallback(
    GUID *driver_guid,
    LPSTR description,
    LPSTR name,
    LPVOID context,
    HMONITOR monitor)
{
    IDirectDraw7 *direct_draw = nullptr;
    IDirect3D7 *direct_3d = nullptr;
    D3DEnum_DeviceInfo driver = {};

    (void)name;
    (void)context;
    (void)monitor;

    if (FAILED(DirectDrawCreateEx(
            driver_guid,
            reinterpret_cast<void **>(&direct_draw),
            IID_IDirectDraw7,
            nullptr))) {
        return TRUE;
    }
    if (FAILED(direct_draw->QueryInterface(
            IID_IDirect3D7,
            reinterpret_cast<void **>(&direct_3d)))) {
        direct_draw->Release();
        return TRUE;
    }

    lstrcpynA(driver.strDesc, description, 39);
    driver.ddDriverCaps.dwSize = sizeof(driver.ddDriverCaps);
    driver.ddHELCaps.dwSize = sizeof(driver.ddHELCaps);
    direct_draw->GetCaps(&driver.ddDriverCaps, &driver.ddHELCaps);

    if (driver_guid != nullptr) {
        driver.guidDriver = *driver_guid;
        driver.pDriverGUID = &driver.guidDriver;
    }
    driver.bDesktopCompatible =
        (driver.ddDriverCaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED) != 0 &&
        driver_guid == nullptr;

    direct_draw->EnumDisplayModes(
        0, nullptr, &driver, ModeEnumCallback);
    std::qsort(
        driver.pddsdModes,
        driver.dwNumModes,
        sizeof(DDSURFACEDESC2),
        SortModesCallback);
    direct_3d->EnumDevices(DeviceEnumCallback, &driver);

    if (driver.pddsdModes != nullptr) {
        ::operator delete(
            driver.pddsdModes,
            sizeof(DDSURFACEDESC2));
        driver.pddsdModes = nullptr;
    }
    direct_3d->Release();
    direct_draw->Release();
    return TRUE;
}

HRESULT EnumerateD3DDevices(
    D3DEnum_ConfirmDeviceCallback confirm_device,
    DWORD width,
    DWORD height,
    DWORD bits_per_pixel)
{
    (void)confirm_device;
    dwDefaultWidth = width;
    dwDefaultHeight = height;
    dwDefaultBitsPixel = bits_per_pixel;

    DirectDrawEnumerateExA(
        DriverEnumCallback,
        nullptr,
        DDENUM_ATTACHEDSECONDARYDEVICES |
            DDENUM_DETACHEDSECONDARYDEVICES |
            DDENUM_NONDISPLAYDEVICES);

    if (g_dwNumDevicesEnumerated == 0) {
        return static_cast<HRESULT>(0x81000002L);
    }
    if (g_dwNumDevices == 0) {
        return static_cast<HRESULT>(0x81000003L);
    }
    return S_OK;
}
HRESULT D3DEnum_SelectDefaultAdapter(
    ComPtr<IDXGIAdapter1> &default_adapter,
    DWORD flags)
{
    std::vector<ComPtr<IDXGIAdapter1>> adapters;
    HRESULT result = D3DEnum_GetAdapters(adapters);

    if (FAILED(result)) {
        std::printf("Failed to get adapters\n");
        return result;
    }

    ComPtr<IDXGIAdapter1> software_adapter;
    ComPtr<IDXGIAdapter1> hardware_adapter;
    for (const ComPtr<IDXGIAdapter1> &adapter : adapters) {
        DXGI_ADAPTER_DESC1 description;

        adapter->GetDesc1(&description);
        if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) {
            if ((flags & 1) != 0) {
                software_adapter = adapter;
            }
        } else if (hardware_adapter == nullptr) {
            hardware_adapter = adapter;
        }
        if (software_adapter != nullptr && hardware_adapter != nullptr) {
            break;
        }
    }

    if (hardware_adapter != nullptr && (flags & 1) == 0) {
        default_adapter = hardware_adapter;
        return S_OK;
    }
    if (software_adapter != nullptr) {
        default_adapter = software_adapter;
        return S_OK;
    }

    std::printf("No compatible adapters found\n");
    return E_FAIL;
}
HRESULT D3DEnum_SelectDefaultDevice(
    D3DEnum_DeviceInfo **device,
    DWORD flags)
{
    D3DEnum_DeviceInfo *tnl_hardware = nullptr;
    D3DEnum_DeviceInfo *other_hardware = nullptr;
    D3DEnum_DeviceInfo *reference = nullptr;
    D3DEnum_DeviceInfo *other_software = nullptr;

    if (device == nullptr) {
        return E_INVALIDARG;
    }

    for (DWORD index = 0; index < g_dwNumDevices; ++index) {
        D3DEnum_DeviceInfo *candidate = &g_pDeviceList[index];

        if (!candidate->bDesktopCompatible) {
            continue;
        }
        if (candidate->bHardware) {
            if (*candidate->pDeviceGUID == IID_IDirect3DTnLHalDevice) {
                tnl_hardware = candidate;
            } else {
                other_hardware = candidate;
            }
        } else if (*candidate->pDeviceGUID == IID_IDirect3DRefDevice) {
            reference = candidate;
        } else {
            other_software = candidate;
        }
    }

    D3DEnum_DeviceInfo *selected = nullptr;
    if ((flags & 1) == 0) {
        selected = tnl_hardware != nullptr
            ? tnl_hardware
            : other_hardware;
    }
    if (selected == nullptr) {
        selected = other_software != nullptr
            ? other_software
            : reference;
    }
    if (selected == nullptr) {
        return static_cast<HRESULT>(0x81000004L);
    }

    *device = selected;
    selected->bWindowed = TRUE;
    return S_OK;
}
static HRESULT CALLBACK ModeEnumCallback(
    DDSURFACEDESC2 *mode,
    void *context)
{
    D3DEnum_DeviceInfo *device =
        static_cast<D3DEnum_DeviceInfo *>(context);
    const DWORD old_count = device->dwNumModes;
    DDSURFACEDESC2 *new_modes = static_cast<DDSURFACEDESC2 *>(
        ::operator new[](
            static_cast<size_t>(old_count + 1) *
            sizeof(DDSURFACEDESC2)));

    std::memcpy(
        new_modes,
        device->pddsdModes,
        static_cast<size_t>(old_count) * sizeof(DDSURFACEDESC2));
    ::operator delete(device->pddsdModes, sizeof(DDSURFACEDESC2));
    device->pddsdModes = new_modes;
    std::memcpy(
        &new_modes[old_count], mode, sizeof(DDSURFACEDESC2));
    ++device->dwNumModes;
    return TRUE;
}

/* 0x3ADA0, 53 bytes, global, 2 named locals
 * SortModesCallback
 * PDB type: int (const void*, const void*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3denum.cpp
 */
int SortModesCallback(const void *first, const void *second)
{
    const DDSURFACEDESC2 *left =
        static_cast<const DDSURFACEDESC2 *>(first);
    const DDSURFACEDESC2 *right =
        static_cast<const DDSURFACEDESC2 *>(second);

    if (left->dwWidth < right->dwWidth) {
        return -1;
    }
    if (left->dwWidth > right->dwWidth) {
        return 1;
    }
    if (left->dwHeight < right->dwHeight) {
        return -1;
    }
    if (left->dwHeight > right->dwHeight) {
        return 1;
    }
    if (left->ddpfPixelFormat.dwRGBBitCount <
        right->ddpfPixelFormat.dwRGBBitCount) {
        return -1;
    }
    return left->ddpfPixelFormat.dwRGBBitCount >
        right->ddpfPixelFormat.dwRGBBitCount;
}

HRESULT jpb_D3DEnumModeCallbackForTest(
    DDSURFACEDESC2 *mode,
    D3DEnum_DeviceInfo *device)
{
    return ModeEnumCallback(mode, device);
}

HRESULT jpb_D3DEnumDeviceCallbackForTest(
    char *description,
    char *name,
    D3DDEVICEDESC7 *device_description,
    D3DEnum_DeviceInfo *parent)
{
    return DeviceEnumCallback(
        description, name, device_description, parent);
}

void jpb_D3DEnumResetForTest(DWORD device_count)
{
    D3DEnum_FreeResources();
    std::memset(g_pDeviceList, 0, sizeof(g_pDeviceList));
    g_dwNumDevicesEnumerated = 0;
    g_dwNumDevices = device_count;
    dwDefaultWidth = 0;
    dwDefaultHeight = 0;
    dwDefaultBitsPixel = 0;
}

DWORD jpb_D3DEnumGetEnumeratedCountForTest(void)
{
    return g_dwNumDevicesEnumerated;
}

void jpb_D3DEnumSetDefaultsForTest(
    DWORD width,
    DWORD height,
    DWORD bits_per_pixel)
{
    dwDefaultWidth = width;
    dwDefaultHeight = height;
    dwDefaultBitsPixel = bits_per_pixel;
}

/* 0x3ADE0, 17 bytes, global, 0 named locals
 * std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>,std::allocator<Microsoft::WRL::ComPtr<IDXGIAdapter1> > >::_Xlength
 * PDB type: void std::vector<Microsoft::WRL:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x3AE00, 66 bytes, global, 7 named locals
 * std::allocator<Microsoft::WRL::ComPtr<IDXGIAdapter1> >::deallocate
 * PDB type: void std::allocator<Microsoft::W...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x26FAE0, 70 bytes, local, 1 named locals
 * `std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>,std::allocator<Microsoft::WRL::ComPtr<IDXGIAdapter1> > >::_Emplace_reallocate<Microsoft::WRL::ComPtr<IDXGIAdapter1> const &>'::`1'::catch$12
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x26FB30, 12 bytes, local, 0 named locals
 * `D3DEnum_GetAdapters'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FB40, 12 bytes, local, 0 named locals
 * `D3DEnum_GetAdapters'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FB50, 12 bytes, local, 2 named locals
 * `D3DEnum_SelectDefaultAdapter'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FB60, 12 bytes, local, 2 named locals
 * `D3DEnum_SelectDefaultAdapter'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FB70, 12 bytes, local, 2 named locals
 * `D3DEnum_SelectDefaultAdapter'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FB80, 12 bytes, local, 2 named locals
 * `D3DEnum_SelectDefaultAdapter'::`1'::dtor$10
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FB90, 12 bytes, local, 2 named locals
 * `D3DEnum_SelectDefaultAdapter'::`1'::dtor$11
 * PDB type: unknown
 * Source: no line mapping
 */
