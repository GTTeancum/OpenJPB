#include "jpb/d3denum.h"

#include <cstdio>
#include <cstring>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "check failed: %s (%s:%d)\n", \
                     #condition, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

static DDSURFACEDESC2 make_mode(
    DWORD width,
    DWORD height,
    DWORD bits,
    unsigned char marker)
{
    DDSURFACEDESC2 mode;

    std::memset(&mode, marker, sizeof(mode));
    mode.dwSize = sizeof(mode);
    mode.dwWidth = width;
    mode.dwHeight = height;
    mode.ddpfPixelFormat.dwRGBBitCount = bits;
    return mode;
}

static int test_get_devices_and_mode_ownership(void)
{
    D3DEnum_DeviceInfo *devices = nullptr;
    DWORD count = 99;
    DDSURFACEDESC2 first = make_mode(640, 480, 16, 0x31);
    DDSURFACEDESC2 second = make_mode(800, 600, 32, 0x72);

    jpb_D3DEnumResetForTest(2);
    D3DEnum_GetDevices(&devices, &count);
    CHECK(devices != nullptr);
    CHECK(count == 2);
    CHECK(devices[0].pddsdModes == nullptr);
    CHECK(devices[0].dwNumModes == 0);

    CHECK(jpb_D3DEnumModeCallbackForTest(&first, &devices[0]) == TRUE);
    CHECK(devices[0].dwNumModes == 1);
    CHECK(std::memcmp(&devices[0].pddsdModes[0], &first, sizeof(first)) == 0);

    CHECK(jpb_D3DEnumModeCallbackForTest(&second, &devices[0]) == TRUE);
    CHECK(devices[0].dwNumModes == 2);
    CHECK(std::memcmp(&devices[0].pddsdModes[0], &first, sizeof(first)) == 0);
    CHECK(std::memcmp(&devices[0].pddsdModes[1], &second, sizeof(second)) == 0);

    CHECK(jpb_D3DEnumModeCallbackForTest(&first, &devices[1]) == TRUE);
    D3DEnum_FreeResources();
    CHECK(devices[0].pddsdModes == nullptr);
    CHECK(devices[1].pddsdModes == nullptr);
    CHECK(devices[0].dwNumModes == 2);
    CHECK(devices[1].dwNumModes == 1);
    D3DEnum_GetDevices(nullptr, nullptr);
    return 0;
}

static int test_mode_sort_order(void)
{
    DDSURFACEDESC2 modes[] = {
        make_mode(800, 480, 16, 0),
        make_mode(640, 600, 16, 0),
        make_mode(640, 480, 32, 0),
        make_mode(640, 480, 16, 0),
        make_mode(640, 480, 16, 0)
    };

    CHECK(SortModesCallback(&modes[3], &modes[4]) == 0);
    CHECK(SortModesCallback(&modes[3], &modes[2]) == -1);
    CHECK(SortModesCallback(&modes[2], &modes[3]) == 1);
    CHECK(SortModesCallback(&modes[3], &modes[1]) == -1);
    CHECK(SortModesCallback(&modes[1], &modes[3]) == 1);
    CHECK(SortModesCallback(&modes[1], &modes[0]) == -1);
    CHECK(SortModesCallback(&modes[0], &modes[1]) == 1);
    return 0;
}

static int test_default_device_priority(void)
{
    D3DEnum_DeviceInfo *devices = nullptr;
    D3DEnum_DeviceInfo *selected =
        reinterpret_cast<D3DEnum_DeviceInfo *>(static_cast<uintptr_t>(1));
    DWORD count = 0;
    GUID ordinary_hardware = {
        0x10203040, 0x5060, 0x7080,
        {0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x11}
    };
    GUID ordinary_software = {
        0x20304050, 0x6070, 0x8090,
        {0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x11, 0x22}
    };

    CHECK(D3DEnum_SelectDefaultDevice(nullptr, 0) == E_INVALIDARG);
    jpb_D3DEnumResetForTest(4);
    D3DEnum_GetDevices(&devices, &count);
    CHECK(count == 4);

    devices[0].bDesktopCompatible = TRUE;
    devices[0].bHardware = FALSE;
    devices[0].guidDevice = IID_IDirect3DRefDevice;
    devices[0].pDeviceGUID = &devices[0].guidDevice;

    devices[1].bDesktopCompatible = TRUE;
    devices[1].bHardware = FALSE;
    devices[1].guidDevice = ordinary_software;
    devices[1].pDeviceGUID = &devices[1].guidDevice;

    devices[2].bDesktopCompatible = TRUE;
    devices[2].bHardware = TRUE;
    devices[2].guidDevice = ordinary_hardware;
    devices[2].pDeviceGUID = &devices[2].guidDevice;

    devices[3].bDesktopCompatible = TRUE;
    devices[3].bHardware = TRUE;
    devices[3].guidDevice = IID_IDirect3DTnLHalDevice;
    devices[3].pDeviceGUID = &devices[3].guidDevice;

    CHECK(D3DEnum_SelectDefaultDevice(&selected, 0) == S_OK);
    CHECK(selected == &devices[3]);
    CHECK(selected->bWindowed == TRUE);

    selected = nullptr;
    CHECK(D3DEnum_SelectDefaultDevice(&selected, 1) == S_OK);
    CHECK(selected == &devices[1]);

    devices[1].bDesktopCompatible = FALSE;
    selected = nullptr;
    CHECK(D3DEnum_SelectDefaultDevice(&selected, 1) == S_OK);
    CHECK(selected == &devices[0]);

    devices[0].bDesktopCompatible = FALSE;
    selected = reinterpret_cast<D3DEnum_DeviceInfo *>(
        static_cast<uintptr_t>(1));
    CHECK(D3DEnum_SelectDefaultDevice(&selected, 1) ==
          static_cast<HRESULT>(0x81000004L));
    CHECK(selected == reinterpret_cast<D3DEnum_DeviceInfo *>(
        static_cast<uintptr_t>(1)));

    devices[3].bDesktopCompatible = FALSE;
    selected = nullptr;
    CHECK(D3DEnum_SelectDefaultDevice(&selected, 0) == S_OK);
    CHECK(selected == &devices[2]);
    return 0;
}

static int test_live_adapter_enumeration(void)
{
    std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> adapters;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> selected;

    CHECK(D3DEnum_GetAdapters(adapters) == S_OK);
    CHECK(!adapters.empty());
    for (const auto &adapter : adapters) {
        CHECK(adapter != nullptr);
    }
    CHECK(D3DEnum_SelectDefaultAdapter(selected, 0) == S_OK);
    CHECK(selected != nullptr);
    return 0;
}

static int test_device_callback(void)
{
    D3DEnum_DeviceInfo parent = {};
    D3DEnum_DeviceInfo *devices = nullptr;
    D3DDEVICEDESC7 description = {};
    DDSURFACEDESC2 parent_modes[] = {
        make_mode(640, 480, 16, 0x11),
        make_mode(800, 600, 24, 0x22),
        make_mode(1024, 768, 32, 0x33)
    };
    char callback_description[] = "ignored-description";
    char callback_name[] = "primary-device";
    DWORD count = 0;

    jpb_D3DEnumResetForTest(0);
    jpb_D3DEnumSetDefaultsForTest(1024, 768, 32);
    parent.bDesktopCompatible = TRUE;
    parent.pddsdModes = parent_modes;
    parent.dwNumModes = 3;
    parent.ddDriverCaps.dwSize = sizeof(parent.ddDriverCaps);
    parent.ddDriverCaps.dwCaps = 0x12345678;
    parent.ddHELCaps.dwSize = sizeof(parent.ddHELCaps);
    parent.ddHELCaps.dwCaps = 0x87654321;

    description.dwDevCaps = D3DDEVCAPS_HWRASTERIZATION;
    description.dwDeviceRenderBitDepth = DDBD_16 | DDBD_32;
    description.deviceGUID.Data1 = 0xabcdef01;

    CHECK(jpb_D3DEnumDeviceCallbackForTest(
              callback_description,
              callback_name,
              &description,
              &parent) == TRUE);
    D3DEnum_GetDevices(&devices, &count);
    CHECK(count == 1);
    CHECK(std::strcmp(devices[0].strDesc, callback_name) == 0);
    CHECK(devices[0].bHardware == D3DDEVCAPS_HWRASTERIZATION);
    CHECK(devices[0].pDeviceGUID == &devices[0].guidDevice);
    CHECK(devices[0].guidDevice.Data1 == 0xabcdef01);
    CHECK(devices[0].pDriverGUID == nullptr);
    CHECK(devices[0].ddDriverCaps.dwCaps == 0x12345678);
    CHECK(devices[0].ddHELCaps.dwCaps == 0x87654321);
    CHECK(devices[0].dwNumModes == 2);
    CHECK(devices[0].pddsdModes[0].dwWidth == 640);
    CHECK(devices[0].pddsdModes[1].dwWidth == 1024);
    CHECK(devices[0].dwCurrentMode == 1);
    CHECK(devices[0].ddsdFullscreenMode.dwWidth == 1024);
    CHECK(devices[0].bWindowed == TRUE);

    D3DEnum_FreeResources();
    jpb_D3DEnumResetForTest(0);
    std::strcpy(parent.strDesc, "secondary-driver");
    parent.guidDriver.Data1 = 0x13572468;
    parent.pDriverGUID = &parent.guidDriver;
    description.dwDevCaps = 0;
    CHECK(jpb_D3DEnumDeviceCallbackForTest(
              callback_description,
              callback_name,
              &description,
              &parent) == TRUE);
    D3DEnum_GetDevices(&devices, &count);
    CHECK(count == 0);
    CHECK(std::strcmp(devices[0].strDesc, parent.strDesc) == 0);
    CHECK(devices[0].pDriverGUID == &devices[0].guidDriver);
    CHECK(devices[0].guidDriver.Data1 == 0x13572468);
    ::operator delete(devices[0].pddsdModes, sizeof(DDSURFACEDESC2));
    devices[0].pddsdModes = nullptr;
    return 0;
}

static int g_confirm_callback_calls;

static HRESULT reject_device(
    DDCAPS *driver_caps,
    D3DDEVICEDESC7 *device_description)
{
    (void)driver_caps;
    (void)device_description;
    ++g_confirm_callback_calls;
    return E_FAIL;
}

static int test_live_legacy_enumeration(void)
{
    D3DEnum_DeviceInfo *devices = nullptr;
    DWORD count = 0;
    HRESULT result;

    jpb_D3DEnumResetForTest(0);
    g_confirm_callback_calls = 0;
    result = EnumerateD3DDevices(reject_device, 640, 480, 16);
    CHECK(g_confirm_callback_calls == 0);

    D3DEnum_GetDevices(&devices, &count);
    CHECK(devices != nullptr);
    CHECK(count <= JPB_D3DENUM_DEVICE_CAPACITY);
    if (jpb_D3DEnumGetEnumeratedCountForTest() == 0) {
        CHECK(result == static_cast<HRESULT>(0x81000002L));
        CHECK(count == 0);
    } else if (count == 0) {
        CHECK(result == static_cast<HRESULT>(0x81000003L));
    } else {
        CHECK(result == S_OK);
    }
    for (DWORD index = 0; index < count; ++index) {
        CHECK(devices[index].pDeviceGUID == &devices[index].guidDevice);
        CHECK(devices[index].pddsdModes != nullptr);
        CHECK(devices[index].dwNumModes > 0);
    }
    D3DEnum_FreeResources();
    return 0;
}

int main(void)
{
    if (test_get_devices_and_mode_ownership() != 0 ||
        test_mode_sort_order() != 0 ||
        test_default_device_priority() != 0 ||
        test_live_adapter_enumeration() != 0 ||
        test_device_callback() != 0 ||
        test_live_legacy_enumeration() != 0) {
        return 1;
    }
    std::puts("d3denum tests passed");
    return 0;
}
