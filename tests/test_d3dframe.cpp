#include "jpb/d3dframe.h"
#include "jpb/d3dtextr.h"
#include "jpb/resources.h"

#include <wrl/client.h>

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "check failed: %s (%s:%d)\n", \
                     #condition, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

static void zero_range(unsigned char *bytes, std::size_t offset,
                       std::size_t size)
{
    std::memset(bytes + offset, 0, size);
}

struct FakeSwapChain {
    void **vtable;
    UINT sync_interval;
    UINT flags;
    unsigned int calls;
    HRESULT result;
};

struct FakeUnknown {
    void **vtable;
    unsigned int release_calls;
    ULONG release_result;
};

static ULONG STDMETHODCALLTYPE fake_unknown_release(IUnknown *unknown)
{
    auto *fake = reinterpret_cast<FakeUnknown *>(unknown);
    ++fake->release_calls;
    return fake->release_result;
}

struct FakeDirectDraw7 {
    void **vtable;
    unsigned int release_calls;
    ULONG release_result;
    unsigned int cooperative_calls;
    HWND cooperative_window;
    DWORD cooperative_flags;
    unsigned int gdi_flip_calls;
    unsigned int restore_all_calls;
    void *query_output;
    HRESULT query_result;
    unsigned int set_display_mode_calls;
    DWORD display_width;
    DWORD display_height;
    DWORD display_bits;
    DWORD display_refresh;
    DWORD display_flags;
    HRESULT set_display_mode_result;
    unsigned int create_surface_calls;
    DDSURFACEDESC2 surface_descriptions[3];
    IDirectDrawSurface7 *surface_outputs[3];
    HRESULT surface_results[3];
    unsigned int create_clipper_calls;
    DWORD clipper_flags;
    IDirectDrawClipper *clipper_output;
    HRESULT create_clipper_result;
};

static ULONG STDMETHODCALLTYPE fake_direct_draw_release(IDirectDraw7 *draw)
{
    auto *fake = reinterpret_cast<FakeDirectDraw7 *>(draw);
    ++fake->release_calls;
    return fake->release_result;
}

static HRESULT STDMETHODCALLTYPE fake_direct_draw_flip_to_gdi(
    IDirectDraw7 *draw)
{
    auto *fake = reinterpret_cast<FakeDirectDraw7 *>(draw);
    ++fake->gdi_flip_calls;
    return static_cast<HRESULT>(0x81234001);
}

static HRESULT STDMETHODCALLTYPE fake_direct_draw_set_cooperative_level(
    IDirectDraw7 *draw, HWND window, DWORD flags)
{
    auto *fake = reinterpret_cast<FakeDirectDraw7 *>(draw);
    ++fake->cooperative_calls;
    fake->cooperative_window = window;
    fake->cooperative_flags = flags;
    return static_cast<HRESULT>(0x81234002);
}

static HRESULT STDMETHODCALLTYPE fake_direct_draw_restore_all(
    IDirectDraw7 *draw)
{
    auto *fake = reinterpret_cast<FakeDirectDraw7 *>(draw);
    ++fake->restore_all_calls;
    return static_cast<HRESULT>(0x81234003);
}

static HRESULT STDMETHODCALLTYPE fake_direct_draw_set_display_mode(
    IDirectDraw7 *draw,
    DWORD width,
    DWORD height,
    DWORD bits,
    DWORD refresh,
    DWORD flags)
{
    auto *fake = reinterpret_cast<FakeDirectDraw7 *>(draw);
    ++fake->set_display_mode_calls;
    fake->display_width = width;
    fake->display_height = height;
    fake->display_bits = bits;
    fake->display_refresh = refresh;
    fake->display_flags = flags;
    return fake->set_display_mode_result;
}

static HRESULT STDMETHODCALLTYPE fake_direct_draw_create_surface(
    IDirectDraw7 *draw,
    DDSURFACEDESC2 *description,
    IDirectDrawSurface7 **surface,
    IUnknown *outer)
{
    auto *fake = reinterpret_cast<FakeDirectDraw7 *>(draw);
    const unsigned int index = fake->create_surface_calls++;
    if (index >= 3 || outer != nullptr) {
        return E_UNEXPECTED;
    }
    fake->surface_descriptions[index] = *description;
    *surface = fake->surface_outputs[index];
    return fake->surface_results[index];
}

static HRESULT STDMETHODCALLTYPE fake_direct_draw_create_clipper(
    IDirectDraw7 *draw,
    DWORD flags,
    IDirectDrawClipper **clipper,
    IUnknown *outer)
{
    auto *fake = reinterpret_cast<FakeDirectDraw7 *>(draw);
    ++fake->create_clipper_calls;
    fake->clipper_flags = flags;
    if (outer != nullptr) {
        return E_UNEXPECTED;
    }
    *clipper = fake->clipper_output;
    return fake->create_clipper_result;
}

struct FakeDirect3D7 {
    void **vtable;
    unsigned int release_calls;
    ULONG release_result;
    unsigned int create_device_calls;
    GUID device_guid;
    IDirectDrawSurface7 *render_target;
    IDirect3DDevice7 *device_output;
    HRESULT create_device_result;
    unsigned int enum_z_calls;
    GUID enum_z_guid;
    DDPIXELFORMAT enum_z_candidates[2];
    HRESULT enum_z_result;
};

struct FakeDirect3DDevice7 {
    void **vtable;
    unsigned int release_calls;
    ULONG release_result;
    unsigned int viewport_calls;
    D3DVIEWPORT7 viewport;
    HRESULT viewport_result;
    unsigned int get_caps_calls;
    D3DDEVICEDESC7 caps;
    HRESULT get_caps_result;
    unsigned int set_render_target_calls;
    IDirectDrawSurface7 *set_render_target_surface;
    DWORD set_render_target_flags;
    HRESULT set_render_target_result;
};

static HRESULT STDMETHODCALLTYPE fake_direct_draw_query_interface(
    IDirectDraw7 *draw, REFIID, void **output)
{
    auto *fake = reinterpret_cast<FakeDirectDraw7 *>(draw);
    *output = fake->query_output;
    return fake->query_result;
}

static ULONG STDMETHODCALLTYPE fake_direct3d_release(IDirect3D7 *direct3d)
{
    auto *fake = reinterpret_cast<FakeDirect3D7 *>(direct3d);
    ++fake->release_calls;
    return fake->release_result;
}

static HRESULT STDMETHODCALLTYPE fake_direct3d_create_device(
    IDirect3D7 *direct3d,
    REFCLSID device_guid,
    IDirectDrawSurface7 *render_target,
    IDirect3DDevice7 **device)
{
    auto *fake = reinterpret_cast<FakeDirect3D7 *>(direct3d);
    ++fake->create_device_calls;
    fake->device_guid = device_guid;
    fake->render_target = render_target;
    *device = fake->device_output;
    return fake->create_device_result;
}

static HRESULT STDMETHODCALLTYPE fake_direct3d_enum_zbuffer_formats(
    IDirect3D7 *direct3d,
    REFCLSID device_guid,
    LPD3DENUMPIXELFORMATSCALLBACK callback,
    void *context)
{
    auto *fake = reinterpret_cast<FakeDirect3D7 *>(direct3d);
    const unsigned int index = fake->enum_z_calls++;
    fake->enum_z_guid = device_guid;
    if (index < 2 && fake->enum_z_candidates[index].dwSize != 0) {
        callback(&fake->enum_z_candidates[index], context);
    }
    return fake->enum_z_result;
}

static ULONG STDMETHODCALLTYPE fake_direct3d_device_release(
    IDirect3DDevice7 *device)
{
    auto *fake = reinterpret_cast<FakeDirect3DDevice7 *>(device);
    ++fake->release_calls;
    return fake->release_result;
}

static HRESULT STDMETHODCALLTYPE fake_direct3d_set_viewport(
    IDirect3DDevice7 *device, LPD3DVIEWPORT7 viewport)
{
    auto *fake = reinterpret_cast<FakeDirect3DDevice7 *>(device);
    ++fake->viewport_calls;
    fake->viewport = *viewport;
    return fake->viewport_result;
}

static HRESULT STDMETHODCALLTYPE fake_direct3d_get_caps(
    IDirect3DDevice7 *device, D3DDEVICEDESC7 *caps)
{
    auto *fake = reinterpret_cast<FakeDirect3DDevice7 *>(device);
    ++fake->get_caps_calls;
    *caps = fake->caps;
    return fake->get_caps_result;
}

static HRESULT STDMETHODCALLTYPE fake_direct3d_set_render_target(
    IDirect3DDevice7 *device,
    IDirectDrawSurface7 *surface,
    DWORD flags)
{
    auto *fake = reinterpret_cast<FakeDirect3DDevice7 *>(device);
    ++fake->set_render_target_calls;
    fake->set_render_target_surface = surface;
    fake->set_render_target_flags = flags;
    return fake->set_render_target_result;
}

static int test_framework7_creation_helpers()
{
    void *draw_vtable[26] = {};
    draw_vtable[0] =
        reinterpret_cast<void *>(&fake_direct_draw_query_interface);
    draw_vtable[2] = reinterpret_cast<void *>(&fake_direct_draw_release);
    void *direct3d_vtable[5] = {};
    direct3d_vtable[2] = reinterpret_cast<void *>(&fake_direct3d_release);
    direct3d_vtable[4] =
        reinterpret_cast<void *>(&fake_direct3d_create_device);
    void *device_vtable[14] = {};
    device_vtable[2] =
        reinterpret_cast<void *>(&fake_direct3d_device_release);
    device_vtable[13] =
        reinterpret_cast<void *>(&fake_direct3d_set_viewport);

    FakeDirect3DDevice7 device = {};
    device.vtable = device_vtable;
    device.viewport_result = S_OK;
    FakeDirect3D7 direct3d = {};
    direct3d.vtable = direct3d_vtable;
    direct3d.device_output = reinterpret_cast<IDirect3DDevice7 *>(&device);
    direct3d.create_device_result = S_OK;
    FakeDirectDraw7 draw = {};
    draw.vtable = draw_vtable;
    draw.query_output = &direct3d;
    draw.query_result = S_OK;
    FakeUnknown render_target = {};

    CD3DFramework7 framework;
    framework.m_pDD = reinterpret_cast<IDirectDraw7 *>(&draw);
    framework.m_pddsBackBuffer =
        reinterpret_cast<IDirectDrawSurface7 *>(&render_target);
    framework.m_dwRenderWidth = 640;
    framework.m_dwRenderHeight = 480;
    GUID device_guid = IID_IDirect3DHALDevice;
    CHECK(framework.CreateDirect3D(&device_guid) == S_OK);
    CHECK(direct3d.create_device_calls == 1);
    CHECK(IsEqualGUID(direct3d.device_guid, device_guid));
    CHECK(direct3d.render_target == framework.m_pddsBackBuffer);
    CHECK(device.viewport_calls == 1);
    CHECK(device.viewport.dwX == 0);
    CHECK(device.viewport.dwY == 0);
    CHECK(device.viewport.dwWidth == 640);
    CHECK(device.viewport.dwHeight == 480);
    CHECK(device.viewport.dvMinZ == 0.0f);
    CHECK(device.viewport.dvMaxZ == 1.0f);

    draw.query_result = E_FAIL;
    CHECK(framework.CreateDirect3D(&device_guid) ==
          static_cast<HRESULT>(0x82000003));
    draw.query_result = S_OK;
    direct3d.create_device_result = E_FAIL;
    CHECK(framework.CreateDirect3D(&device_guid) ==
          static_cast<HRESULT>(0x82000004));
    direct3d.create_device_result = S_OK;
    device.viewport_result = E_FAIL;
    CHECK(framework.CreateDirect3D(&device_guid) ==
          static_cast<HRESULT>(0x82000007));

    framework.m_pDD = nullptr;
    framework.m_pD3D = nullptr;
    framework.m_pd3dDevice = nullptr;
    framework.m_pddsBackBuffer = nullptr;
    return 0;
}

struct FakeDirectDrawSurface7 {
    void **vtable;
    unsigned int release_calls;
    ULONG release_result;
    unsigned int add_ref_calls;
    ULONG add_ref_result;
    unsigned int add_attached_calls;
    IDirectDrawSurface7 *added_surface;
    HRESULT add_attached_result;
    unsigned int blt_calls;
    RECT blt_destination;
    IDirectDrawSurface7 *blt_source;
    DWORD blt_flags;
    unsigned int flip_calls;
    IDirectDrawSurface7 *flip_target;
    DWORD flip_flags;
    unsigned int get_dc_calls;
    HDC surface_context;
    unsigned int release_dc_calls;
    HRESULT get_dc_result;
    HRESULT operation_result;
    unsigned int get_attached_calls;
    DDSCAPS2 attached_caps;
    IDirectDrawSurface7 *attached_output;
    HRESULT get_attached_result;
    unsigned int get_description_calls;
    DDSURFACEDESC2 description;
    HRESULT get_description_result;
    unsigned int set_clipper_calls;
    IDirectDrawClipper *clipper;
    HRESULT set_clipper_result;
};

struct FakeDirectDrawClipper {
    void **vtable;
    unsigned int release_calls;
    ULONG release_result;
    unsigned int set_window_calls;
    DWORD set_window_flags;
    HWND window;
    HRESULT set_window_result;
};

static ULONG STDMETHODCALLTYPE fake_surface_add_ref(
    IDirectDrawSurface7 *surface)
{
    auto *fake = reinterpret_cast<FakeDirectDrawSurface7 *>(surface);
    ++fake->add_ref_calls;
    return fake->add_ref_result;
}

static ULONG STDMETHODCALLTYPE fake_surface_release(
    IDirectDrawSurface7 *surface)
{
    auto *fake = reinterpret_cast<FakeDirectDrawSurface7 *>(surface);
    ++fake->release_calls;
    return fake->release_result;
}

static HRESULT STDMETHODCALLTYPE fake_surface_add_attached(
    IDirectDrawSurface7 *surface, IDirectDrawSurface7 *attached)
{
    auto *fake = reinterpret_cast<FakeDirectDrawSurface7 *>(surface);
    ++fake->add_attached_calls;
    fake->added_surface = attached;
    return fake->add_attached_result;
}

static HRESULT STDMETHODCALLTYPE fake_surface_blt(
    IDirectDrawSurface7 *surface,
    LPRECT destination,
    IDirectDrawSurface7 *source,
    LPRECT source_rectangle,
    DWORD flags,
    LPDDBLTFX effects)
{
    auto *fake = reinterpret_cast<FakeDirectDrawSurface7 *>(surface);
    ++fake->blt_calls;
    fake->blt_destination = *destination;
    fake->blt_source = source;
    fake->blt_flags = flags;
    CHECK(source_rectangle == nullptr);
    CHECK(effects == nullptr);
    return fake->operation_result;
}

static HRESULT STDMETHODCALLTYPE fake_surface_flip(
    IDirectDrawSurface7 *surface,
    IDirectDrawSurface7 *target_override,
    DWORD flags)
{
    auto *fake = reinterpret_cast<FakeDirectDrawSurface7 *>(surface);
    ++fake->flip_calls;
    fake->flip_target = target_override;
    fake->flip_flags = flags;
    return fake->operation_result;
}

static HRESULT STDMETHODCALLTYPE fake_surface_get_attached(
    IDirectDrawSurface7 *surface,
    DDSCAPS2 *caps,
    IDirectDrawSurface7 **attached)
{
    auto *fake = reinterpret_cast<FakeDirectDrawSurface7 *>(surface);
    ++fake->get_attached_calls;
    fake->attached_caps = *caps;
    *attached = fake->attached_output;
    return fake->get_attached_result;
}

static HRESULT STDMETHODCALLTYPE fake_surface_get_dc(
    IDirectDrawSurface7 *surface, HDC *device_context)
{
    auto *fake = reinterpret_cast<FakeDirectDrawSurface7 *>(surface);
    ++fake->get_dc_calls;
    *device_context = fake->surface_context;
    return fake->get_dc_result;
}

static HRESULT STDMETHODCALLTYPE fake_surface_release_dc(
    IDirectDrawSurface7 *surface, HDC device_context)
{
    auto *fake = reinterpret_cast<FakeDirectDrawSurface7 *>(surface);
    CHECK(device_context == fake->surface_context);
    ++fake->release_dc_calls;
    return fake->operation_result;
}

static HRESULT STDMETHODCALLTYPE fake_surface_get_description(
    IDirectDrawSurface7 *surface, DDSURFACEDESC2 *description)
{
    auto *fake = reinterpret_cast<FakeDirectDrawSurface7 *>(surface);
    ++fake->get_description_calls;
    *description = fake->description;
    return fake->get_description_result;
}

static HRESULT STDMETHODCALLTYPE fake_surface_set_clipper(
    IDirectDrawSurface7 *surface, IDirectDrawClipper *clipper)
{
    auto *fake = reinterpret_cast<FakeDirectDrawSurface7 *>(surface);
    ++fake->set_clipper_calls;
    fake->clipper = clipper;
    return fake->set_clipper_result;
}

static ULONG STDMETHODCALLTYPE fake_clipper_release(IDirectDrawClipper *clipper)
{
    auto *fake = reinterpret_cast<FakeDirectDrawClipper *>(clipper);
    ++fake->release_calls;
    return fake->release_result;
}

static HRESULT STDMETHODCALLTYPE fake_clipper_set_window(
    IDirectDrawClipper *clipper, DWORD flags, HWND window)
{
    auto *fake = reinterpret_cast<FakeDirectDrawClipper *>(clipper);
    ++fake->set_window_calls;
    fake->set_window_flags = flags;
    fake->window = window;
    return fake->set_window_result;
}

static int test_framework7_buffer_and_depth_creation()
{
    void *draw_vtable[30] = {};
    draw_vtable[2] = reinterpret_cast<void *>(&fake_direct_draw_release);
    draw_vtable[4] =
        reinterpret_cast<void *>(&fake_direct_draw_create_clipper);
    draw_vtable[6] =
        reinterpret_cast<void *>(&fake_direct_draw_create_surface);
    draw_vtable[21] =
        reinterpret_cast<void *>(&fake_direct_draw_set_display_mode);
    void *surface_vtable[49] = {};
    surface_vtable[1] = reinterpret_cast<void *>(&fake_surface_add_ref);
    surface_vtable[2] = reinterpret_cast<void *>(&fake_surface_release);
    surface_vtable[3] =
        reinterpret_cast<void *>(&fake_surface_add_attached);
    surface_vtable[12] =
        reinterpret_cast<void *>(&fake_surface_get_attached);
    surface_vtable[22] =
        reinterpret_cast<void *>(&fake_surface_get_description);
    surface_vtable[28] =
        reinterpret_cast<void *>(&fake_surface_set_clipper);
    void *clipper_vtable[9] = {};
    clipper_vtable[2] = reinterpret_cast<void *>(&fake_clipper_release);
    clipper_vtable[8] = reinterpret_cast<void *>(&fake_clipper_set_window);
    void *direct3d_vtable[7] = {};
    direct3d_vtable[2] = reinterpret_cast<void *>(&fake_direct3d_release);
    direct3d_vtable[6] =
        reinterpret_cast<void *>(&fake_direct3d_enum_zbuffer_formats);
    void *device_vtable[14] = {};
    device_vtable[2] =
        reinterpret_cast<void *>(&fake_direct3d_device_release);
    device_vtable[3] = reinterpret_cast<void *>(&fake_direct3d_get_caps);
    device_vtable[8] =
        reinterpret_cast<void *>(&fake_direct3d_set_render_target);

    FakeDirectDrawSurface7 front = {};
    FakeDirectDrawSurface7 back = {};
    FakeDirectDrawSurface7 depth = {};
    front.vtable = surface_vtable;
    back.vtable = surface_vtable;
    depth.vtable = surface_vtable;
    back.add_ref_result = 2;
    front.attached_output = reinterpret_cast<IDirectDrawSurface7 *>(&back);
    front.get_attached_result = S_OK;
    back.add_attached_result = S_OK;
    back.get_description_result = S_OK;
    back.description.dwSize = sizeof(back.description);
    back.description.dwWidth = 320;
    back.description.dwHeight = 200;
    back.description.ddpfPixelFormat.dwSize =
        sizeof(back.description.ddpfPixelFormat);
    back.description.ddpfPixelFormat.dwZBufferBitDepth = 32;

    FakeDirectDrawClipper clipper = {};
    clipper.vtable = clipper_vtable;
    clipper.set_window_result = S_OK;
    FakeDirectDraw7 draw = {};
    draw.vtable = draw_vtable;
    draw.set_display_mode_result = S_OK;
    draw.surface_outputs[0] = reinterpret_cast<IDirectDrawSurface7 *>(&front);
    draw.surface_results[0] = S_OK;
    draw.clipper_output = reinterpret_cast<IDirectDrawClipper *>(&clipper);
    draw.create_clipper_result = S_OK;

    FakeDirect3D7 direct3d = {};
    direct3d.vtable = direct3d_vtable;
    direct3d.enum_z_candidates[0].dwSize =
        sizeof(direct3d.enum_z_candidates[0]);
    direct3d.enum_z_candidates[0].dwFlags = DDPF_ZBUFFER;
    direct3d.enum_z_candidates[0].dwZBufferBitDepth = 24;
    direct3d.enum_z_candidates[1].dwSize =
        sizeof(direct3d.enum_z_candidates[1]);
    direct3d.enum_z_candidates[1].dwFlags = DDPF_ZBUFFER;
    direct3d.enum_z_candidates[1].dwZBufferBitDepth = 16;
    direct3d.enum_z_result = S_OK;
    FakeDirect3DDevice7 device = {};
    device.vtable = device_vtable;
    device.get_caps_result = S_OK;
    device.set_render_target_result = S_OK;

    CD3DFramework7 framework;
    framework.m_pDD = reinterpret_cast<IDirectDraw7 *>(&draw);
    DDSURFACEDESC2 mode = {};
    mode.dwSize = sizeof(mode);
    mode.dwWidth = 320;
    mode.dwHeight = 200;
    mode.dwRefreshRate = 70;
    mode.ddpfPixelFormat.dwRGBBitCount = 8;
    CHECK(framework.CreateFullscreenBuffers(&mode) == S_OK);
    CHECK(draw.set_display_mode_calls == 1);
    CHECK(draw.display_width == 320);
    CHECK(draw.display_height == 200);
    CHECK(draw.display_bits == 8);
    CHECK(draw.display_refresh == 70);
    CHECK(draw.display_flags == 1);
    CHECK(draw.create_surface_calls == 1);
    CHECK(draw.surface_descriptions[0].dwFlags ==
          (DDSD_CAPS | DDSD_BACKBUFFERCOUNT));
    CHECK(draw.surface_descriptions[0].dwBackBufferCount == 1);
    CHECK(draw.surface_descriptions[0].ddsCaps.dwCaps ==
          (DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP |
           DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE));
    CHECK(front.get_attached_calls == 1);
    CHECK(front.attached_caps.dwCaps == DDSCAPS_BACKBUFFER);
    CHECK(back.add_ref_calls == 1);

    draw.set_display_mode_result = E_FAIL;
    CHECK(framework.CreateFullscreenBuffers(&mode) ==
          static_cast<HRESULT>(0x8200000A));
    draw.set_display_mode_result = S_OK;
    draw.create_surface_calls = 0;
    draw.surface_results[0] = E_FAIL;
    CHECK(framework.CreateFullscreenBuffers(&mode) ==
          static_cast<HRESULT>(0x82000008));
    draw.create_surface_calls = 0;
    draw.surface_results[0] = DDERR_OUTOFVIDEOMEMORY;
    CHECK(framework.CreateFullscreenBuffers(&mode) ==
          DDERR_OUTOFVIDEOMEMORY);

    HWND window = CreateWindowExA(
        0, "STATIC", "d3dframe7-buffer-test", WS_POPUP,
        50, 60, 160, 90, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(window != nullptr);
    framework.m_hWnd = window;
    draw.create_surface_calls = 0;
    draw.surface_outputs[0] = reinterpret_cast<IDirectDrawSurface7 *>(&front);
    draw.surface_outputs[1] = reinterpret_cast<IDirectDrawSurface7 *>(&back);
    draw.surface_results[0] = S_OK;
    draw.surface_results[1] = S_OK;
    CHECK(framework.CreateWindowedBuffers() == S_OK);
    CHECK(framework.m_dwRenderWidth == 160);
    CHECK(framework.m_dwRenderHeight == 90);
    CHECK(draw.create_surface_calls == 2);
    CHECK(draw.surface_descriptions[0].dwFlags == DDSD_CAPS);
    CHECK(draw.surface_descriptions[0].ddsCaps.dwCaps ==
          DDSCAPS_PRIMARYSURFACE);
    CHECK(draw.create_clipper_calls == 1);
    CHECK(draw.clipper_flags == 0);
    CHECK(clipper.set_window_calls == 1);
    CHECK(clipper.set_window_flags == 0);
    CHECK(clipper.window == window);
    CHECK(front.set_clipper_calls == 1);
    CHECK(front.clipper == reinterpret_cast<IDirectDrawClipper *>(&clipper));
    CHECK(clipper.release_calls == 1);
    CHECK(draw.surface_descriptions[1].dwFlags ==
          (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH));
    CHECK(draw.surface_descriptions[1].dwWidth == 160);
    CHECK(draw.surface_descriptions[1].dwHeight == 90);
    CHECK(draw.surface_descriptions[1].ddsCaps.dwCaps ==
          (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE));
    CHECK(DestroyWindow(window) != FALSE);

    framework.m_pD3D = reinterpret_cast<IDirect3D7 *>(&direct3d);
    framework.m_pd3dDevice = reinterpret_cast<IDirect3DDevice7 *>(&device);
    framework.m_pddsBackBuffer =
        reinterpret_cast<IDirectDrawSurface7 *>(&back);
    framework.m_dwDeviceMemType = DDSCAPS_VIDEOMEMORY;
    draw.create_surface_calls = 0;
    draw.surface_outputs[0] = reinterpret_cast<IDirectDrawSurface7 *>(&depth);
    draw.surface_results[0] = S_OK;
    GUID device_guid = IID_IDirect3DHALDevice;
    CHECK(framework.CreateZBuffer(&device_guid) == S_OK);
    CHECK(direct3d.enum_z_calls == 2);
    CHECK(IsEqualGUID(direct3d.enum_z_guid, device_guid));
    CHECK(draw.create_surface_calls == 1);
    CHECK(draw.surface_descriptions[0].dwFlags ==
          (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT));
    CHECK(draw.surface_descriptions[0].ddsCaps.dwCaps ==
          (DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER));
    CHECK(draw.surface_descriptions[0].ddpfPixelFormat.dwZBufferBitDepth == 16);
    CHECK(back.add_attached_calls == 1);
    CHECK(back.added_surface ==
          reinterpret_cast<IDirectDrawSurface7 *>(&depth));
    CHECK(device.set_render_target_calls == 1);
    CHECK(device.set_render_target_surface ==
          reinterpret_cast<IDirectDrawSurface7 *>(&back));
    CHECK(device.set_render_target_flags == 0);

    device.caps.dwDevCaps = D3DDEVCAPS_DRAWPRIMITIVES2EX;
    const unsigned int enum_calls = direct3d.enum_z_calls;
    CHECK(framework.CreateZBuffer(&device_guid) == S_OK);
    CHECK(direct3d.enum_z_calls == enum_calls);

    framework.m_pDD = nullptr;
    framework.m_pD3D = nullptr;
    framework.m_pd3dDevice = nullptr;
    framework.m_pddsFrontBuffer = nullptr;
    framework.m_pddsBackBuffer = nullptr;
    framework.m_pddsZBuffer = nullptr;
    return 0;
}

static int test_framework7_initialize_arguments()
{
    CD3DFramework7 framework;
    GUID driver_guid = IID_IDirectDraw7;
    GUID device_guid = IID_IDirect3DHALDevice;
    DDSURFACEDESC2 mode = {};
    mode.dwSize = sizeof(mode);
    CHECK(framework.Initialize(
              nullptr, &driver_guid, &device_guid, &mode, 0) ==
          E_INVALIDARG);
    CHECK(framework.Initialize(
              reinterpret_cast<HWND>(1), &driver_guid, nullptr, &mode, 0) ==
          E_INVALIDARG);
    CHECK(framework.Initialize(
              reinterpret_cast<HWND>(1), &driver_guid, &device_guid,
              nullptr, 1) == E_INVALIDARG);
    CHECK(framework.m_hWnd == nullptr);
    return 0;
}

static int test_framework7_paths()
{
    auto *storage = static_cast<unsigned char *>(
        ::operator new(sizeof(CD3DFramework7)));
    unsigned char expected[sizeof(CD3DFramework7)];
    std::memset(storage, 0xcd, sizeof(CD3DFramework7));
    std::memset(expected, 0xcd, sizeof(expected));
    zero_range(expected, 0, 20);
    zero_range(expected, 40, 52);
    auto *framework = new (storage) CD3DFramework7;
    CHECK(std::memcmp(storage, expected, sizeof(expected)) == 0);

    framework->m_dwRenderWidth = 320;
    framework->m_dwRenderHeight = 200;
    framework->m_bIsFullscreen = FALSE;
    framework->Move(11, 22);
    CHECK(framework->m_rcScreenRect.left == 11);
    CHECK(framework->m_rcScreenRect.top == 22);
    CHECK(framework->m_rcScreenRect.right == 331);
    CHECK(framework->m_rcScreenRect.bottom == 222);
    const RECT moved_rectangle = framework->m_rcScreenRect;
    framework->m_bIsFullscreen = TRUE;
    framework->Move(90, 91);
    CHECK(std::memcmp(
        &framework->m_rcScreenRect, &moved_rectangle,
        sizeof(moved_rectangle)) == 0);
    framework->m_bIsFullscreen = 2;
    framework->Move(3, 4);
    CHECK(framework->m_rcScreenRect.left == 3);
    CHECK(framework->m_rcScreenRect.top == 4);
    void *surface_vtable[27] = {};
    surface_vtable[2] = reinterpret_cast<void *>(&fake_surface_release);
    surface_vtable[5] = reinterpret_cast<void *>(&fake_surface_blt);
    surface_vtable[11] = reinterpret_cast<void *>(&fake_surface_flip);
    surface_vtable[17] = reinterpret_cast<void *>(&fake_surface_get_dc);
    surface_vtable[26] = reinterpret_cast<void *>(&fake_surface_release_dc);
    FakeDirectDrawSurface7 front = {};
    FakeDirectDrawSurface7 back = {};
    FakeDirectDrawSurface7 depth = {};
    front.vtable = surface_vtable;
    back.vtable = surface_vtable;
    depth.vtable = surface_vtable;
    front.operation_result = static_cast<HRESULT>(0x81235001);
    back.operation_result = static_cast<HRESULT>(0x81235002);
    framework->m_pddsFrontBuffer =
        reinterpret_cast<IDirectDrawSurface7 *>(&front);
    framework->m_pddsBackBuffer =
        reinterpret_cast<IDirectDrawSurface7 *>(&back);
    framework->m_pddsZBuffer =
        reinterpret_cast<IDirectDrawSurface7 *>(&depth);

    framework->m_bIsFullscreen = TRUE;
    CHECK(framework->ShowFrame() == front.operation_result);
    CHECK(front.flip_calls == 1);
    CHECK(front.flip_target == nullptr);
    CHECK(front.flip_flags == DDFLIP_WAIT);
    framework->m_bIsFullscreen = FALSE;
    CHECK(framework->ShowFrame() == front.operation_result);
    CHECK(front.blt_calls == 1);
    CHECK(front.blt_source == framework->m_pddsBackBuffer);
    CHECK(front.blt_flags == DDBLT_WAIT);
    CHECK(std::memcmp(
        &front.blt_destination, &framework->m_rcScreenRect,
        sizeof(RECT)) == 0);
    void *draw_vtable[26] = {};
    draw_vtable[2] = reinterpret_cast<void *>(&fake_direct_draw_release);
    draw_vtable[10] =
        reinterpret_cast<void *>(&fake_direct_draw_flip_to_gdi);
    draw_vtable[20] = reinterpret_cast<void *>(
        &fake_direct_draw_set_cooperative_level);
    draw_vtable[25] =
        reinterpret_cast<void *>(&fake_direct_draw_restore_all);
    FakeDirectDraw7 draw = {};
    draw.vtable = draw_vtable;
    framework->m_pDD = reinterpret_cast<IDirectDraw7 *>(&draw);
    CHECK(framework->RestoreSurfaces() == S_OK);
    CHECK(draw.restore_all_calls == 1);
    framework->m_bIsFullscreen = TRUE;
    CHECK(framework->FlipToGDISurface(0) == S_OK);
    CHECK(draw.gdi_flip_calls == 1);

    HWND repaint_window = CreateWindowExA(
        0, "STATIC", "d3dframe7-repaint-test", WS_POPUP,
        20, 30, 64, 48, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(repaint_window != nullptr);
    HDC screen_context = GetDC(nullptr);
    CHECK(screen_context != nullptr);
    HDC source_context = CreateCompatibleDC(screen_context);
    CHECK(source_context != nullptr);
    back.surface_context = source_context;
    back.get_dc_result = S_OK;
    framework->m_hWnd = repaint_window;
    framework->m_bIsFullscreen = FALSE;
    InvalidateRect(repaint_window, nullptr, FALSE);
    framework->Repaint();
    CHECK(back.get_dc_calls == 1);
    CHECK(back.release_dc_calls == 1);
    framework->m_bIsFullscreen = TRUE;
    framework->Repaint();
    CHECK(back.get_dc_calls == 1);
    CHECK(DeleteDC(source_context) != FALSE);
    CHECK(ReleaseDC(nullptr, screen_context) != 0);
    CHECK(DestroyWindow(repaint_window) != FALSE);

    void *unknown_vtable[3] = {};
    unknown_vtable[2] = reinterpret_cast<void *>(&fake_unknown_release);
    FakeUnknown d3d = {unknown_vtable, 0, 0};
    framework->m_pD3D = reinterpret_cast<IDirect3D7 *>(&d3d);
    framework->m_pd3dDevice = reinterpret_cast<IDirect3DDevice7 *>(&d3d);
    framework->m_hWnd = reinterpret_cast<HWND>(0x1234);
    CHECK(framework->DestroyObjects() == S_OK);
    CHECK(draw.cooperative_calls == 1);
    CHECK(draw.cooperative_window == framework->m_hWnd);
    CHECK(draw.cooperative_flags == DDSCL_NORMAL);
    CHECK(draw.release_calls == 1);
    CHECK(d3d.release_calls == 2);
    CHECK(front.release_calls == 1);
    CHECK(back.release_calls == 1);
    CHECK(depth.release_calls == 1);
    CHECK(framework->m_pDD == nullptr);
    CHECK(framework->m_pd3dDevice == nullptr);
    CHECK(framework->m_pddsFrontBuffer == nullptr);

    FakeUnknown leaking_device = {unknown_vtable, 0, 1};
    framework->m_pd3dDevice =
        reinterpret_cast<IDirect3DDevice7 *>(&leaking_device);
    CHECK(framework->DestroyObjects() ==
          static_cast<HRESULT>(0x8200000C));

    framework->~CD3DFramework7();
    ::operator delete(storage);
    return 0;
}

static HRESULT STDMETHODCALLTYPE fake_swap_chain_present(
    IDXGISwapChain3 *swap_chain, UINT sync_interval, UINT flags)
{
    auto *fake = reinterpret_cast<FakeSwapChain *>(swap_chain);
    fake->sync_interval = sync_interval;
    fake->flags = flags;
    ++fake->calls;
    return fake->result;
}

static int test_window_move_and_present(CD3DFramework12 *framework)
{
    HWND window = CreateWindowExA(
        0, "STATIC", "d3dframe-move-test", WS_POPUP,
        0, 0, 64, 48, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(window != nullptr);

    framework->m_hWnd = window;
    framework->Move(123, 234);
    RECT rectangle = {};
    CHECK(GetWindowRect(window, &rectangle) != FALSE);
    CHECK(rectangle.left == 123);
    CHECK(rectangle.top == 234);
    CHECK(rectangle.right - rectangle.left == 64);
    CHECK(rectangle.bottom - rectangle.top == 48);
    CHECK(DestroyWindow(window) != FALSE);

    framework->m_pSwapChain = nullptr;
    CHECK(framework->Present() == static_cast<HRESULT>(0x8200000F));

    void *vtable[9] = {};
    vtable[8] = reinterpret_cast<void *>(&fake_swap_chain_present);
    FakeSwapChain fake = {};
    fake.vtable = vtable;
    fake.result = static_cast<HRESULT>(0x81234567);
    framework->m_pSwapChain = reinterpret_cast<IDXGISwapChain3 *>(&fake);
    CHECK(framework->Present() == fake.result);
    CHECK(fake.calls == 1);
    CHECK(fake.sync_interval == 1);
    CHECK(fake.flags == 0);
    framework->m_pSwapChain = nullptr;
    return 0;
}

#ifdef JPB_D3DFRAME_REAL_ASSET_DIR
struct SDL_Surface;

template <typename Function>
static Function get_sdl_export(HMODULE module, const char *name)
{
    return reinterpret_cast<Function>(GetProcAddress(module, name));
}

static int test_sdl_texture_paths(CD3DFramework12 *framework)
{
    CHECK(SetDllDirectoryA(JPB_D3DFRAME_REAL_ASSET_DIR) != FALSE);
    HMODULE sdl_module = LoadLibraryA("SDL2.dll");
    HMODULE image_module = LoadLibraryA("SDL2_image.dll");
    CHECK(sdl_module != nullptr);
    CHECK(image_module != nullptr);

    using CreateSurfaceFunction = SDL_Surface *(*)(
        std::uint32_t, int, int, int, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t);
    using CreateRendererFunction = SDL_Renderer *(*)(SDL_Surface *);
    using ReadPixelsFunction = int (*)(
        SDL_Renderer *, const void *, std::uint32_t, void *, int);
    using SetDrawColorFunction = int (*)(
        SDL_Renderer *, std::uint8_t, std::uint8_t,
        std::uint8_t, std::uint8_t);
    using RenderClearFunction = int (*)(SDL_Renderer *);
    using DestroyRendererFunction = void (*)(SDL_Renderer *);
    using DestroyTextureFunction = void (*)(SDL_Texture *);
    using FreeSurfaceFunction = void (*)(SDL_Surface *);

    CreateSurfaceFunction create_surface =
        get_sdl_export<CreateSurfaceFunction>(
            sdl_module, "SDL_CreateRGBSurface");
    CreateRendererFunction create_renderer =
        get_sdl_export<CreateRendererFunction>(
            sdl_module, "SDL_CreateSoftwareRenderer");
    ReadPixelsFunction read_pixels = get_sdl_export<ReadPixelsFunction>(
        sdl_module, "SDL_RenderReadPixels");
    SetDrawColorFunction set_draw_color =
        get_sdl_export<SetDrawColorFunction>(
            sdl_module, "SDL_SetRenderDrawColor");
    RenderClearFunction render_clear = get_sdl_export<RenderClearFunction>(
        sdl_module, "SDL_RenderClear");
    DestroyRendererFunction destroy_renderer =
        get_sdl_export<DestroyRendererFunction>(
            sdl_module, "SDL_DestroyRenderer");
    DestroyTextureFunction destroy_texture =
        get_sdl_export<DestroyTextureFunction>(
            sdl_module, "SDL_DestroyTexture");
    FreeSurfaceFunction free_surface = get_sdl_export<FreeSurfaceFunction>(
        sdl_module, "SDL_FreeSurface");
    CHECK(create_surface != nullptr);
    CHECK(create_renderer != nullptr);
    CHECK(read_pixels != nullptr);
    CHECK(set_draw_color != nullptr);
    CHECK(render_clear != nullptr);
    CHECK(destroy_renderer != nullptr);
    CHECK(destroy_texture != nullptr);
    CHECK(free_surface != nullptr);

    SDL_Surface *surface = create_surface(
        0, 64, 64, 32, 0x00ff0000, 0x0000ff00,
        0x000000ff, 0xff000000);
    CHECK(surface != nullptr);
    SDL_Renderer *renderer = create_renderer(surface);
    CHECK(renderer != nullptr);

    const std::string asset_path =
        std::string(JPB_D3DFRAME_REAL_ASSET_DIR) +
        "\\res\\default\\a_blob.tga";
    SDL_Texture *texture =
        framework->LoadTextureTGA(renderer, asset_path.c_str());
    CHECK(texture != nullptr);
    framework->RenderTexture(renderer, texture);

    std::vector<std::uint32_t> pixels(64 * 64, 0);
    constexpr std::uint32_t argb8888_format = 0x16362004;
    CHECK(read_pixels(
        renderer, nullptr, argb8888_format, pixels.data(),
        64 * sizeof(std::uint32_t)) == 0);
    bool has_rendered_pixel = false;
    for (std::uint32_t pixel : pixels) {
        has_rendered_pixel = has_rendered_pixel || pixel != 0;
    }
    CHECK(has_rendered_pixel);
    CHECK(framework->LoadTextureTGA(
        renderer, "jpb_missing_d3dframe_asset.tga") == nullptr);

    destroy_texture(texture);

    CHECK(set_draw_color(renderer, 0, 0, 0, 0xff) == 0);
    CHECK(render_clear(renderer) == 0);
    const std::uint32_t bitmap_pixels[4] = {
        0xff204080, 0xff406020, 0xff802040, 0xff208060};
    HBITMAP bitmap = CreateBitmap(2, 2, 1, 32, bitmap_pixels);
    CHECK(bitmap != nullptr);
    framework->RenderTextureInUI(renderer, bitmap, 7, 9, 2, 2);
    std::fill(pixels.begin(), pixels.end(), 0);
    CHECK(read_pixels(
        renderer, nullptr, argb8888_format, pixels.data(),
        64 * sizeof(std::uint32_t)) == 0);
    CHECK(pixels[9 * 64 + 7] != pixels[0]);
    CHECK(DeleteObject(bitmap) != FALSE);

    CHECK(jpb_ResourceSetBasePath(JPB_D3DFRAME_REAL_ASSET_DIR) == 1);
    framework->m_pSDLWindow = nullptr;
    framework->m_pSDLRenderer = renderer;
    framework->RenderUI();
    std::fill(pixels.begin(), pixels.end(), 0);
    CHECK(read_pixels(
        renderer, nullptr, argb8888_format, pixels.data(),
        64 * sizeof(std::uint32_t)) == 0);
    has_rendered_pixel = false;
    for (std::uint32_t pixel : pixels) {
        has_rendered_pixel = has_rendered_pixel || pixel != 0;
    }
    CHECK(has_rendered_pixel);
    CHECK(jpb_ResourceSetBasePath(nullptr) == 0);

    destroy_renderer(renderer);
    framework->m_pSDLRenderer = nullptr;
    free_surface(surface);
    CHECK(SetDllDirectoryA(nullptr) != FALSE);
    return 0;
}

static int test_destroy_paths(CD3DFramework12 *framework)
{
    void *vtable[3] = {};
    vtable[2] = reinterpret_cast<void *>(&fake_unknown_release);
    FakeUnknown fake = {vtable, 0, 0};
    IUnknown *unknown = reinterpret_cast<IUnknown *>(&fake);

    framework->m_pDepthStencil = reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_pRenderTargets[0] =
        reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_pRenderTargets[1] =
        reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_pCommandAllocators[0] =
        reinterpret_cast<ID3D12CommandAllocator *>(unknown);
    framework->m_pCommandAllocators[1] =
        reinterpret_cast<ID3D12CommandAllocator *>(unknown);
    framework->m_pFence[0] = reinterpret_cast<ID3D12Fence *>(unknown);
    framework->m_pFence[1] = reinterpret_cast<ID3D12Fence *>(unknown);
    framework->m_pCommandList =
        reinterpret_cast<ID3D12GraphicsCommandList *>(unknown);
    framework->m_pCommandQueue =
        reinterpret_cast<ID3D12CommandQueue *>(unknown);
    framework->m_pRtvDescriptorHeap =
        reinterpret_cast<ID3D12DescriptorHeap *>(unknown);
    framework->m_pSwapChain = reinterpret_cast<IDXGISwapChain3 *>(unknown);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(unknown);
    framework->m_pFactory = reinterpret_cast<IDXGIFactory6 *>(unknown);
    framework->m_pRootSignature =
        reinterpret_cast<ID3D12RootSignature *>(unknown);
    framework->m_pPipelineState =
        reinterpret_cast<ID3D12PipelineState *>(unknown);
    framework->m_pLevelPipelineState =
        reinterpret_cast<ID3D12PipelineState *>(unknown);
    framework->m_pTransparentPipelineState =
        reinterpret_cast<ID3D12PipelineState *>(unknown);
    framework->m_pTransparentGlassPipelineState =
        reinterpret_cast<ID3D12PipelineState *>(unknown);
    framework->m_vertexBuffer = reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_indexBuffer = reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_constantBuffer = reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_3DVertexBuffer =
        reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_3DIndexBuffer =
        reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_pMainDescriptorHeap =
        reinterpret_cast<ID3D12DescriptorHeap *>(unknown);
    framework->m_pDsDescriptorHeap =
        reinterpret_cast<ID3D12DescriptorHeap *>(unknown);
    framework->m_vBufferUploadHeap =
        reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_mappedConstantBuffer[0] = reinterpret_cast<void *>(1);
    framework->m_mappedConstantBuffer[1] = reinterpret_cast<void *>(2);
    framework->m_hFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CHECK(framework->m_hFenceEvent != nullptr);

    framework->m_cbvHeap = reinterpret_cast<ID3D12DescriptorHeap *>(unknown);
    framework->m_constantBufferUploadHeaps[0] =
        reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_vertexUploadBuffer =
        reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_indexUploadBuffer =
        reinterpret_cast<ID3D12Resource *>(unknown);
    framework->PAIN = reinterpret_cast<ID3D12Resource *>(unknown);
    framework->PAINTEX = reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_levelVertexBuffer =
        reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_levelIndexBuffer =
        reinterpret_cast<ID3D12Resource *>(unknown);
    framework->m_pSDLRenderTarget =
        reinterpret_cast<SDL_Texture *>(unknown);

    CHECK(framework->DestroyObjects() == S_OK);
    CHECK(fake.release_calls == 26);
    CHECK(framework->m_pDepthStencil == nullptr);
    CHECK(framework->m_pRenderTargets[0] == nullptr);
    CHECK(framework->m_pRenderTargets[1] == nullptr);
    CHECK(framework->m_pCommandList == nullptr);
    CHECK(framework->m_pDevice == nullptr);
    CHECK(framework->m_hFenceEvent == nullptr);
    CHECK(framework->m_mappedConstantBuffer[0] == nullptr);
    CHECK(framework->m_mappedConstantBuffer[1] == nullptr);
    CHECK(framework->m_cbvHeap != nullptr);
    CHECK(framework->m_constantBufferUploadHeaps[0] != nullptr);
    CHECK(framework->m_vertexUploadBuffer != nullptr);
    CHECK(framework->PAIN != nullptr);
    CHECK(framework->m_levelVertexBuffer != nullptr);
    CHECK(framework->m_pSDLRenderTarget != nullptr);

    framework->m_cbvHeap = nullptr;
    framework->m_constantBufferUploadHeaps[0] = nullptr;
    framework->m_vertexUploadBuffer = nullptr;
    framework->m_indexUploadBuffer = nullptr;
    framework->PAIN = nullptr;
    framework->PAINTEX = nullptr;
    framework->m_levelVertexBuffer = nullptr;
    framework->m_levelIndexBuffer = nullptr;
    framework->m_pSDLRenderTarget = nullptr;

    auto *storage = static_cast<unsigned char *>(
        ::operator new(sizeof(CD3DFramework12)));
    std::memset(storage, 0, sizeof(CD3DFramework12));
    auto *destructor_framework = new (storage) CD3DFramework12;
    destructor_framework->FreeShaderResourceDescriptor(0x1234);
    destructor_framework->m_levelVertexBuffer =
        reinterpret_cast<ID3D12Resource *>(unknown);
    destructor_framework->m_levelIndexBuffer =
        reinterpret_cast<ID3D12Resource *>(unknown);
    destructor_framework->vertexShaderBlob = unknown;
    destructor_framework->pixelShaderBlob = unknown;
    const unsigned int before_destructor = fake.release_calls;
    destructor_framework->~CD3DFramework12();
    CHECK(fake.release_calls == before_destructor + 4);
    ::operator delete(storage);
    return 0;
}
#endif

static int test_constructor_write_footprint(CD3DFramework12 **framework_out,
                                            unsigned char **storage_out)
{
    auto *storage = static_cast<unsigned char *>(
        ::operator new(sizeof(CD3DFramework12)));
    unsigned char expected[sizeof(CD3DFramework12)];
    std::memset(storage, 0xcd, sizeof(CD3DFramework12));
    std::memset(expected, 0xcd, sizeof(expected));

    const std::uint32_t aligned_size = 0x100;
    std::memcpy(expected, &aligned_size, sizeof(aligned_size));
    zero_range(expected, 8, 16);
    zero_range(expected, 24, 4);
    zero_range(expected, 48, 24);
    zero_range(expected, 72, 4);
    zero_range(expected, 80, 56);
    zero_range(expected, 136, 40);
    zero_range(expected, 176, 4);
    zero_range(expected, 180, 40);
    zero_range(expected, 224, 32);
    zero_range(expected, 264, 16);
    zero_range(expected, 280, 4);
    zero_range(expected, 288, 8);
    zero_range(expected, 312, 32);
    zero_range(expected, 360, 16);
    zero_range(expected, 384, 32);
    zero_range(expected, 432, 24);
    zero_range(expected, 480, 32);
    expected[520] = 1;
    zero_range(expected, 528, 16);
    zero_range(expected, 576, 16);
    zero_range(expected, 600, 24);

    auto *framework = new (storage) CD3DFramework12;
    CHECK(std::memcmp(storage, expected, sizeof(expected)) == 0);
    *framework_out = framework;
    *storage_out = storage;
    return 0;
}

static int test_descriptor_and_command_list_paths(CD3DFramework12 *framework)
{
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12DescriptorHeap> heap;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12Fence> fence;
    D3D12_DESCRIPTOR_HEAP_DESC heap_description = {};
    D3D12_COMMAND_QUEUE_DESC queue_description = {};

    CHECK(SUCCEEDED(D3D12CreateDevice(
        nullptr, D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(device.GetAddressOf()))));

    heap_description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_description.NumDescriptors = 3;
    CHECK(SUCCEEDED(device->CreateDescriptorHeap(
        &heap_description, IID_PPV_ARGS(heap.GetAddressOf()))));

    framework->m_pDevice = device.Get();
    framework->m_pMainDescriptorHeap = heap.Get();
    framework->m_nDescriptorCount = 0;
    const std::uint64_t start =
        heap->GetCPUDescriptorHandleForHeapStart().ptr;
    const std::uint64_t increment = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const std::uint64_t first =
        framework->AllocateShaderResourceDescriptor();
    const std::uint64_t second =
        framework->AllocateShaderResourceDescriptor();
    CHECK(first == start);
    CHECK(second == start + increment);
    CHECK(framework->GetShaderResourceDescriptorIndex(second) == 1);

    framework->FreeShaderResourceDescriptor(first);
    CHECK(framework->AllocateShaderResourceDescriptor() == first);
    CHECK(framework->m_nDescriptorCount == 2);
    CHECK(framework->AllocateShaderResourceDescriptor() ==
          start + 2 * increment);
    CHECK(framework->AllocateShaderResourceDescriptor() == 0);

    framework->FreeShaderResourceDescriptor(first);
    framework->FreeShaderResourceDescriptor(second);
    CHECK(framework->AllocateShaderResourceDescriptor() == second);
    CHECK(framework->AllocateShaderResourceDescriptor() == first);

    CHECK(SUCCEEDED(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(allocator.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
        IID_PPV_ARGS(command_list.GetAddressOf()))));
    framework->m_pCommandList = command_list.Get();
    framework->m_commandListOpen = true;
    CHECK(framework->IsCommandListOpen());
    CHECK(framework->TryCloseCommandList() == S_OK);
    CHECK(!framework->IsCommandListOpen());
    CHECK(framework->TryCloseCommandList() == S_OK);

    queue_description.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    CHECK(SUCCEEDED(device->CreateCommandQueue(
        &queue_description, IID_PPV_ARGS(queue.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()))));
    framework->m_pCommandQueue = queue.Get();
    framework->m_pFence[0] = fence.Get();
    framework->m_nFrameIndex = 0;
    framework->m_nFenceValues[0] = 1;
    framework->m_hFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CHECK(framework->m_hFenceEvent != nullptr);
    WaitForGpuTexture(framework);
    CHECK(fence->GetCompletedValue() >= 1);
    CHECK(framework->m_nFenceValues[0] == 2);
    CloseHandle(framework->m_hFenceEvent);

    const std::size_t capacity = static_cast<std::size_t>(
        framework->m_freeShaderResourceDescriptors.capacity_end -
        framework->m_freeShaderResourceDescriptors.begin);
    ::operator delete(framework->m_freeShaderResourceDescriptors.begin,
                      capacity * sizeof(std::uint64_t));
    framework->m_freeShaderResourceDescriptors = {};
    return 0;
}

static int test_render_target_and_depth_paths(CD3DFramework12 *framework)
{
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGIFactory4> factory;
    ComPtr<IDXGISwapChain1> swap_chain_base;
    ComPtr<IDXGISwapChain3> swap_chain;
    ComPtr<ID3D12DescriptorHeap> rtv_heap;
    ComPtr<ID3D12DescriptorHeap> dsv_heap;

    CHECK(SUCCEEDED(D3D12CreateDevice(
        nullptr, D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(device.GetAddressOf()))));
    D3D12_COMMAND_QUEUE_DESC queue_description = {};
    queue_description.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    CHECK(SUCCEEDED(device->CreateCommandQueue(
        &queue_description, IID_PPV_ARGS(queue.GetAddressOf()))));
    CHECK(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()))));

    HWND window = CreateWindowExA(
        0, "STATIC", "d3dframe-swap-chain-test", WS_POPUP,
        0, 0, 320, 200, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(window != nullptr);
    DXGI_SWAP_CHAIN_DESC1 swap_chain_description = {};
    swap_chain_description.Width = 320;
    swap_chain_description.Height = 200;
    swap_chain_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_description.SampleDesc.Count = 1;
    swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_description.BufferCount = 2;
    swap_chain_description.Scaling = DXGI_SCALING_STRETCH;
    swap_chain_description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_description.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    CHECK(SUCCEEDED(factory->CreateSwapChainForHwnd(
        queue.Get(), window, &swap_chain_description,
        nullptr, nullptr, swap_chain_base.GetAddressOf())));
    CHECK(SUCCEEDED(swap_chain_base.As(&swap_chain)));

    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_description = {};
    rtv_heap_description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_description.NumDescriptors = 2;
    CHECK(SUCCEEDED(device->CreateDescriptorHeap(
        &rtv_heap_description, IID_PPV_ARGS(rtv_heap.GetAddressOf()))));
    D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_description = {};
    dsv_heap_description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_heap_description.NumDescriptors = 1;
    CHECK(SUCCEEDED(device->CreateDescriptorHeap(
        &dsv_heap_description, IID_PPV_ARGS(dsv_heap.GetAddressOf()))));

    framework->m_pDevice = device.Get();
    framework->m_pSwapChain = swap_chain.Get();
    framework->m_pRtvDescriptorHeap = rtv_heap.Get();
    framework->m_pDsDescriptorHeap = dsv_heap.Get();
    CHECK(framework->CreateRenderTargetViews() == S_OK);
    CHECK(framework->m_pRenderTargets[0] != nullptr);
    CHECK(framework->m_pRenderTargets[1] != nullptr);

    framework->m_dwRenderWidth = 320;
    framework->m_dwRenderHeight = 200;
    CHECK(framework->ResizeDepthBuffer() == S_OK);
    CHECK(framework->m_pDepthStencil != nullptr);
    const D3D12_RESOURCE_DESC depth_description =
        framework->m_pDepthStencil->GetDesc();
    CHECK(depth_description.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);
    CHECK(depth_description.Width == 320);
    CHECK(depth_description.Height == 200);
    CHECK(depth_description.MipLevels == 9);
    CHECK(depth_description.Format == DXGI_FORMAT_D32_FLOAT);
    CHECK(depth_description.Flags ==
          D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    CHECK(framework->ResizeResources(0, 200) == E_INVALIDARG);
    CHECK(framework->m_dwRenderWidth == 320);
    CHECK(framework->ResizeResources(640, 360) == S_OK);
    CHECK(framework->m_dwRenderWidth == 640);
    CHECK(framework->m_dwRenderHeight == 360);
    CHECK(framework->m_Viewport.Width == 640.0f);
    CHECK(framework->m_Viewport.Height == 360.0f);
    CHECK(framework->m_ScissorRect.right == 640);
    CHECK(framework->m_ScissorRect.bottom == 360);
    CHECK(framework->m_pRenderTargets[0] != nullptr);
    CHECK(framework->m_pRenderTargets[1] != nullptr);
    CHECK(framework->m_pDepthStencil != nullptr);
    const D3D12_RESOURCE_DESC resized_depth_description =
        framework->m_pDepthStencil->GetDesc();
    CHECK(resized_depth_description.Width == 640);
    CHECK(resized_depth_description.Height == 360);

    framework->m_pDepthStencil->Release();
    framework->m_pDepthStencil = nullptr;
    for (ID3D12Resource *&render_target : framework->m_pRenderTargets) {
        render_target->Release();
        render_target = nullptr;
    }
    framework->m_pSwapChain = nullptr;
    framework->m_pDevice = nullptr;
    framework->m_pRtvDescriptorHeap = nullptr;
    framework->m_pDsDescriptorHeap = nullptr;
    swap_chain.Reset();
    swap_chain_base.Reset();
    CHECK(DestroyWindow(window) != FALSE);
    return 0;
}

int main()
{
    CD3DFramework12 *framework = nullptr;
    unsigned char *storage = nullptr;

    CHECK(test_framework7_paths() == 0);
    CHECK(test_framework7_creation_helpers() == 0);
    CHECK(test_framework7_buffer_and_depth_creation() == 0);
    CHECK(test_framework7_initialize_arguments() == 0);
    CHECK(test_constructor_write_footprint(&framework, &storage) == 0);
    CHECK(test_window_move_and_present(framework) == 0);
    CHECK(test_descriptor_and_command_list_paths(framework) == 0);
    CHECK(test_render_target_and_depth_paths(framework) == 0);
#ifdef JPB_D3DFRAME_REAL_ASSET_DIR
    CHECK(test_sdl_texture_paths(framework) == 0);
    CHECK(test_destroy_paths(framework) == 0);
#endif
    ::operator delete(storage);

    std::puts("d3dframe tests passed");
    return 0;
}
