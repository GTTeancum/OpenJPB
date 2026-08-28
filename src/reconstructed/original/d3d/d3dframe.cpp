/* COMPLETE REVIEWED RECONSTRUCTION. */

#include "jpb/d3dframe.h"
#include "jpb/resources.h"

#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

namespace {

struct CanonicalSDLSurface {
    std::uint32_t flags;
    std::uint32_t padding_004;
    void *format;
    int width;
    int height;
    int pitch;
    std::uint32_t padding_028;
    void *pixels;
};

struct CanonicalSDLRect {
    int x;
    int y;
    int width;
    int height;
};

struct CanonicalSDLFrameImports {
    CanonicalSDLSurface *(*image_load)(const char *);
    const char *(*get_error)();
    SDL_Texture *(*create_texture_from_surface)(
        SDL_Renderer *, CanonicalSDLSurface *);
    void (*free_surface)(CanonicalSDLSurface *);
    int (*render_clear)(SDL_Renderer *);
    int (*render_copy)(
        SDL_Renderer *, SDL_Texture *, const void *, const void *);
    void (*render_present)(SDL_Renderer *);
    CanonicalSDLSurface *(*create_rgb_surface)(
        std::uint32_t, int, int, int, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t);
    int (*lock_surface)(CanonicalSDLSurface *);
    void (*unlock_surface)(CanonicalSDLSurface *);
    void (*destroy_texture)(SDL_Texture *);
    int (*set_render_draw_color)(
        SDL_Renderer *, std::uint8_t, std::uint8_t,
        std::uint8_t, std::uint8_t);
    void (*destroy_renderer)(SDL_Renderer *);
    void (*destroy_window)(SDL_Window *);
    void (*quit)();
};

template <typename Function>
Function ResolveCanonicalSDLImport(HMODULE module, const char *name)
{
    FARPROC procedure = GetProcAddress(module, name);
    if (procedure == nullptr) {
        RaiseFailFastException(nullptr, nullptr, 0);
    }
    return reinterpret_cast<Function>(procedure);
}

const CanonicalSDLFrameImports &GetCanonicalSDLFrameImports()
{
    static CanonicalSDLFrameImports imports = {};
    static bool initialized = false;
    if (!initialized) {
        HMODULE sdl_module = GetModuleHandleA("SDL2.dll");
        if (sdl_module == nullptr) {
            sdl_module = LoadLibraryA("SDL2.dll");
        }
        HMODULE image_module = GetModuleHandleA("SDL2_image.dll");
        if (image_module == nullptr) {
            image_module = LoadLibraryA("SDL2_image.dll");
        }
        if (sdl_module == nullptr || image_module == nullptr) {
            RaiseFailFastException(nullptr, nullptr, 0);
        }

        imports.image_load = ResolveCanonicalSDLImport<
            decltype(imports.image_load)>(image_module, "IMG_Load");
        imports.get_error = ResolveCanonicalSDLImport<
            decltype(imports.get_error)>(sdl_module, "SDL_GetError");
        imports.create_texture_from_surface = ResolveCanonicalSDLImport<
            decltype(imports.create_texture_from_surface)>(
                sdl_module, "SDL_CreateTextureFromSurface");
        imports.free_surface = ResolveCanonicalSDLImport<
            decltype(imports.free_surface)>(sdl_module, "SDL_FreeSurface");
        imports.render_clear = ResolveCanonicalSDLImport<
            decltype(imports.render_clear)>(sdl_module, "SDL_RenderClear");
        imports.render_copy = ResolveCanonicalSDLImport<
            decltype(imports.render_copy)>(sdl_module, "SDL_RenderCopy");
        imports.render_present = ResolveCanonicalSDLImport<
            decltype(imports.render_present)>(
                sdl_module, "SDL_RenderPresent");
        imports.create_rgb_surface = ResolveCanonicalSDLImport<
            decltype(imports.create_rgb_surface)>(
                sdl_module, "SDL_CreateRGBSurface");
        imports.lock_surface = ResolveCanonicalSDLImport<
            decltype(imports.lock_surface)>(sdl_module, "SDL_LockSurface");
        imports.unlock_surface = ResolveCanonicalSDLImport<
            decltype(imports.unlock_surface)>(
                sdl_module, "SDL_UnlockSurface");
        imports.destroy_texture = ResolveCanonicalSDLImport<
            decltype(imports.destroy_texture)>(
                sdl_module, "SDL_DestroyTexture");
        imports.set_render_draw_color = ResolveCanonicalSDLImport<
            decltype(imports.set_render_draw_color)>(
                sdl_module, "SDL_SetRenderDrawColor");
        imports.destroy_renderer = ResolveCanonicalSDLImport<
            decltype(imports.destroy_renderer)>(
                sdl_module, "SDL_DestroyRenderer");
        imports.destroy_window = ResolveCanonicalSDLImport<
            decltype(imports.destroy_window)>(
                sdl_module, "SDL_DestroyWindow");
        imports.quit = ResolveCanonicalSDLImport<decltype(imports.quit)>(
            sdl_module, "SDL_Quit");
        initialized = true;
    }
    return imports;
}

static_assert(offsetof(CanonicalSDLSurface, pixels) == 32,
              "SDL_Surface pixel offset changed");

constexpr std::size_t kVectorLargeAllocationThreshold = 0x1000;
constexpr std::size_t kVectorAlignment = 32;
constexpr std::size_t kVectorAlignmentOverhead = 39;

std::uint64_t *AllocateVectorStorage(std::size_t count)
{
    const std::size_t bytes = count * sizeof(std::uint64_t);
    if (bytes == 0) {
        return nullptr;
    }
    if (bytes < kVectorLargeAllocationThreshold) {
        return static_cast<std::uint64_t *>(::operator new(bytes));
    }

    void *allocation = ::operator new(bytes + kVectorAlignmentOverhead);
    const std::uintptr_t aligned_address =
        (reinterpret_cast<std::uintptr_t>(allocation) +
         kVectorAlignmentOverhead) &
        ~(static_cast<std::uintptr_t>(kVectorAlignment) - 1);
    auto *aligned = reinterpret_cast<std::uint64_t *>(aligned_address);
    reinterpret_cast<void **>(aligned)[-1] = allocation;
    return aligned;
}

void FreeVectorStorage(std::uint64_t *storage, std::size_t capacity)
{
    if (storage == nullptr) {
        return;
    }

    const std::size_t bytes = capacity * sizeof(std::uint64_t);
    if (bytes < kVectorLargeAllocationThreshold) {
        ::operator delete(storage, bytes);
        return;
    }

    void *allocation = reinterpret_cast<void **>(storage)[-1];
    ::operator delete(allocation, bytes + kVectorAlignmentOverhead);
}

void PushFreeDescriptor(JpbPdbVectorUint64 &descriptors,
                        std::uint64_t descriptor)
{
    if (descriptors.end != descriptors.capacity_end) {
        *descriptors.end++ = descriptor;
        return;
    }

    const std::size_t old_size =
        descriptors.begin == nullptr
            ? 0
            : static_cast<std::size_t>(descriptors.end - descriptors.begin);
    constexpr std::size_t max_size =
        (std::numeric_limits<std::ptrdiff_t>::max)() /
        sizeof(std::uint64_t);
    if (old_size == max_size) {
        throw std::length_error("vector too long");
    }

    const std::size_t old_capacity =
        descriptors.begin == nullptr
            ? 0
            : static_cast<std::size_t>(descriptors.capacity_end -
                                       descriptors.begin);
    const std::size_t new_size = old_size + 1;
    if (old_capacity > max_size - old_capacity / 2) {
        throw std::length_error("vector too long");
    }
    const std::size_t geometric_capacity =
        old_capacity + old_capacity / 2;
    const std::size_t new_capacity =
        geometric_capacity < new_size ? new_size : geometric_capacity;

    std::uint64_t *new_storage = AllocateVectorStorage(new_capacity);
    new_storage[old_size] = descriptor;
    if (old_size != 0) {
        std::memcpy(new_storage, descriptors.begin,
                    old_size * sizeof(std::uint64_t));
    }
    FreeVectorStorage(descriptors.begin, old_capacity);
    descriptors.begin = new_storage;
    descriptors.end = new_storage + new_size;
    descriptors.capacity_end = new_storage + new_capacity;
}

template <typename T>
void StoreAt(CD3DFramework12 *framework, std::size_t offset, T value)
{
    std::memcpy(reinterpret_cast<unsigned char *>(framework) + offset,
                &value, sizeof(value));
}

void StoreZeroRange(CD3DFramework12 *framework, std::size_t offset,
                    std::size_t size)
{
    std::memset(reinterpret_cast<unsigned char *>(framework) + offset,
                0, size);
}

template <typename Interface>
void ReleaseAndClear(Interface *&object)
{
    if (object != nullptr) {
        object->Release();
        object = nullptr;
    }
}

} // namespace

static HRESULT CALLBACK EnumZBufferFormatsCallback(
    DDPIXELFORMAT *candidate, void *context);

/*
 * COMPLETE REVIEWED RECONSTRUCTION.
 * PDB module: 0023
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\d3dframe.obj
 * Primary source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 * Compiler language: c++
 * Emitted procedures: 38
 *
 * All project-owned procedures are reconstructed from the matched PDB and
 * direct shipped-executable disassembly.
 */

/* 0x3AF50, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<unsigned __int64 *,unsigned __int64 *>
 * PDB type: unsigned __int64* (unsigned __in...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x3AF80, 415 bytes, global, 23 named locals
 * std::vector<unsigned __int64,std::allocator<unsigned __int64> >::_Emplace_reallocate<unsigned __int64 const &>
 * PDB type: unsigned __int64* std::vector<un...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x3B120, 326 bytes, global, 1 named locals
 * CD3DFramework12::CD3DFramework12
 * PDB type: void CD3DFramework12::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */

/* 0x3B270, 43 bytes, global, 1 named locals
 * CD3DFramework7::CD3DFramework7
 * PDB type: void CD3DFramework7::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
CD3DFramework7::CD3DFramework7()
{
    m_hWnd = nullptr;
    m_bIsFullscreen = FALSE;
    m_dwRenderWidth = 0;
    m_dwRenderHeight = 0;
    m_pddsFrontBuffer = nullptr;
    m_pddsBackBuffer = nullptr;
    m_pddsZBuffer = nullptr;
    m_pd3dDevice = nullptr;
    m_pDD = nullptr;
    m_pD3D = nullptr;
    m_dwDeviceMemType = 0;
}

/* 0x3B2A0, 238 bytes, global, 10 named locals
 * CD3DFramework12::~CD3DFramework12
 * PDB type: void CD3DFramework12::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
CD3DFramework12::~CD3DFramework12()
{
    DestroyObjects();

    const std::size_t capacity =
        m_freeShaderResourceDescriptors.begin == nullptr
            ? 0
            : static_cast<std::size_t>(
                  m_freeShaderResourceDescriptors.capacity_end -
                  m_freeShaderResourceDescriptors.begin);
    FreeVectorStorage(m_freeShaderResourceDescriptors.begin, capacity);
    m_freeShaderResourceDescriptors = {};

    if (m_levelIndexBuffer != nullptr) {
        ID3D12Resource *resource = m_levelIndexBuffer;
        m_levelIndexBuffer = nullptr;
        resource->Release();
    }
    if (m_levelVertexBuffer != nullptr) {
        ID3D12Resource *resource = m_levelVertexBuffer;
        m_levelVertexBuffer = nullptr;
        resource->Release();
    }
    if (pixelShaderBlob != nullptr) {
        IUnknown *blob = pixelShaderBlob;
        pixelShaderBlob = nullptr;
        blob->Release();
    }
    if (vertexShaderBlob != nullptr) {
        IUnknown *blob = vertexShaderBlob;
        vertexShaderBlob = nullptr;
        blob->Release();
    }
}

/* 0x3B390, 15 bytes, global, 1 named locals
 * CD3DFramework7::~CD3DFramework7
 * PDB type: void CD3DFramework7::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
CD3DFramework7::~CD3DFramework7()
{
    DestroyObjects();
}

/* 0x3B3A0, 190 bytes, global, 5 named locals
 * CD3DFramework12::AllocateShaderResourceDescriptor
 * PDB type: unsigned __int64 CD3DFramework12...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
CD3DFramework12::CD3DFramework12()
{
    StoreAt<std::uint32_t>(this, 0, 0x100);
    StoreZeroRange(this, 8, 16);
    StoreAt<std::uint32_t>(this, 24, 0);
    StoreZeroRange(this, 48, 24);
    StoreAt<std::uint32_t>(this, 72, 0);
    StoreZeroRange(this, 80, 56);
    StoreZeroRange(this, 136, 40);
    StoreAt<std::uint32_t>(this, 176, 0);
    StoreZeroRange(this, 180, 40);
    StoreZeroRange(this, 224, 32);
    StoreZeroRange(this, 264, 16);
    StoreAt<std::uint32_t>(this, 280, 0);
    StoreAt<std::uint64_t>(this, 288, 0);
    StoreZeroRange(this, 312, 32);
    StoreZeroRange(this, 360, 16);
    StoreZeroRange(this, 384, 32);
    StoreZeroRange(this, 432, 24);
    StoreZeroRange(this, 480, 32);
    StoreAt<std::uint8_t>(this, 520, 1);
    StoreZeroRange(this, 528, 16);
    StoreZeroRange(this, 576, 16);
    StoreZeroRange(this, 600, 24);
}
std::uint64_t CD3DFramework12::AllocateShaderResourceDescriptor()
{
    if (m_freeShaderResourceDescriptors.begin !=
        m_freeShaderResourceDescriptors.end) {
        const std::uint64_t descriptor =
            m_freeShaderResourceDescriptors.end[-1];
        --m_freeShaderResourceDescriptors.end;
        return descriptor;
    }

    const D3D12_DESCRIPTOR_HEAP_DESC heap_description =
        m_pMainDescriptorHeap->GetDesc();
    const UINT descriptor_index = m_nDescriptorCount;
    D3D12_CPU_DESCRIPTOR_HANDLE handle = {};

    if (descriptor_index < heap_description.NumDescriptors) {
        ++m_nDescriptorCount;
        handle = m_pMainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(descriptor_index) *
            m_pDevice->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    return handle.ptr;
}

/* 0x3B460, 187 bytes, global, 3 named locals
 * CD3DFramework7::CreateDirect3D
 * PDB type: HRESULT CD3DFramework7::(_GUID*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::CreateDirect3D(GUID *device_guid)
{
    if (FAILED(m_pDD->QueryInterface(
            IID_IDirect3D7, reinterpret_cast<void **>(&m_pD3D)))) {
        return static_cast<HRESULT>(0x82000003);
    }
    if (FAILED(m_pD3D->CreateDevice(
            *device_guid, m_pddsBackBuffer, &m_pd3dDevice))) {
        return static_cast<HRESULT>(0x82000004);
    }

    D3DVIEWPORT7 viewport;
    viewport.dwX = 0;
    viewport.dwY = 0;
    viewport.dwWidth = m_dwRenderWidth;
    viewport.dwHeight = m_dwRenderHeight;
    viewport.dvMinZ = 0.0f;
    viewport.dvMaxZ = 1.0f;
    if (FAILED(m_pd3dDevice->SetViewport(&viewport))) {
        return static_cast<HRESULT>(0x82000007);
    }
    return S_OK;
}

/* 0x3B520, 215 bytes, global, 4 named locals
 * CD3DFramework7::CreateDirectDraw
 * PDB type: HRESULT CD3DFramework7::(_GUID*,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::CreateDirectDraw(
    GUID *driver_guid, DWORD flags)
{
    if (FAILED(DirectDrawCreateEx(
            driver_guid,
            reinterpret_cast<void **>(&m_pDD),
            IID_IDirectDraw7,
            nullptr))) {
        return static_cast<HRESULT>(0x82000001);
    }

    DWORD cooperative_flags = m_bIsFullscreen != FALSE
        ? DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        : DDSCL_NORMAL;
    if ((flags & 8) == 0) {
        cooperative_flags |= DDSCL_FPUSETUP;
    }
    if (FAILED(m_pDD->SetCooperativeLevel(m_hWnd, cooperative_flags))) {
        return static_cast<HRESULT>(0x82000002);
    }

    DDSURFACEDESC2 display_mode;
    display_mode.dwSize = sizeof(display_mode);
    m_pDD->GetDisplayMode(&display_mode);
    if (display_mode.ddpfPixelFormat.dwRGBBitCount <= 8) {
        return static_cast<HRESULT>(0x8200000E);
    }
    return S_OK;
}

/* 0x3B600, 1390 bytes, global, 18 named locals
 * CD3DFramework7::CreateEnvironment
 * PDB type: HRESULT CD3DFramework7::(_GUID*,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::CreateEnvironment(
    GUID *driver_guid,
    GUID *device_guid,
    DDSURFACEDESC2 *mode,
    DWORD flags)
{
    m_dwDeviceMemType =
        IsEqualGUID(*device_guid, IID_IDirect3DHALDevice) ||
                IsEqualGUID(*device_guid, IID_IDirect3DTnLHalDevice)
            ? DDSCAPS_VIDEOMEMORY
            : DDSCAPS_SYSTEMMEMORY;

    HRESULT result = CreateDirectDraw(driver_guid, flags);
    if (FAILED(result)) {
        return result;
    }
    result = m_bIsFullscreen != FALSE
        ? CreateFullscreenBuffers(mode)
        : CreateWindowedBuffers();
    if (FAILED(result)) {
        return result;
    }
    result = CreateDirect3D(device_guid);
    if (FAILED(result)) {
        return result;
    }
    return CreateZBuffer(device_guid);
}

/* 0x3BB70, 406 bytes, global, 6 named locals
 * CD3DFramework7::CreateFullscreenBuffers
 * PDB type: HRESULT CD3DFramework7::(_DDSURF...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::CreateFullscreenBuffers(DDSURFACEDESC2 *mode)
{
    SetRect(
        &m_rcScreenRect, 0, 0,
        static_cast<int>(mode->dwWidth),
        static_cast<int>(mode->dwHeight));
    m_dwRenderWidth = static_cast<DWORD>(
        m_rcScreenRect.right - m_rcScreenRect.left);
    m_dwRenderHeight = static_cast<DWORD>(
        m_rcScreenRect.bottom - m_rcScreenRect.top);

    const DWORD display_flags =
        m_dwRenderWidth == 320 &&
        m_dwRenderHeight == 200 &&
        mode->ddpfPixelFormat.dwRGBBitCount == 8
            ? 1
            : 0;
    if (FAILED(m_pDD->SetDisplayMode(
            m_dwRenderWidth,
            m_dwRenderHeight,
            mode->ddpfPixelFormat.dwRGBBitCount,
            mode->dwRefreshRate,
            display_flags))) {
        return static_cast<HRESULT>(0x8200000A);
    }

    DDSURFACEDESC2 description = {};
    description.dwSize = sizeof(description);
    description.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    description.dwBackBufferCount = 1;
    description.ddsCaps.dwCaps =
        DDSCAPS_PRIMARYSURFACE |
        DDSCAPS_FLIP |
        DDSCAPS_COMPLEX |
        DDSCAPS_3DDEVICE;
    HRESULT result = m_pDD->CreateSurface(
        &description, &m_pddsFrontBuffer, nullptr);
    if (FAILED(result)) {
        return result == DDERR_OUTOFVIDEOMEMORY
            ? result
            : static_cast<HRESULT>(0x82000008);
    }

    DDSCAPS2 back_buffer_caps = {};
    back_buffer_caps.dwCaps = DDSCAPS_BACKBUFFER;
    if (FAILED(m_pddsFrontBuffer->GetAttachedSurface(
            &back_buffer_caps, &m_pddsBackBuffer))) {
        return static_cast<HRESULT>(0x8200000B);
    }
    m_pddsBackBuffer->AddRef();
    return S_OK;
}

/* 0x3BD10, 165 bytes, global, 5 named locals
 * CD3DFramework12::CreateRenderTargetViews
 * PDB type: HRESULT CD3DFramework12::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework12::CreateRenderTargetViews()
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        m_pRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    const UINT descriptor_size = m_pDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    for (UINT index = 0; index < 2; ++index) {
        const HRESULT result = m_pSwapChain->GetBuffer(
            index, IID_PPV_ARGS(&m_pRenderTargets[index]));
        if (FAILED(result)) {
            return result;
        }
        m_pDevice->CreateRenderTargetView(
            m_pRenderTargets[index], nullptr, handle);
        handle.ptr += descriptor_size;
    }
    return S_OK;
}

/* 0x3BDC0, 446 bytes, global, 4 named locals
 * CD3DFramework7::CreateWindowedBuffers
 * PDB type: HRESULT CD3DFramework7::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::CreateWindowedBuffers()
{
    GetClientRect(m_hWnd, &m_rcScreenRect);
    ClientToScreen(
        m_hWnd, reinterpret_cast<POINT *>(&m_rcScreenRect.left));
    ClientToScreen(
        m_hWnd, reinterpret_cast<POINT *>(&m_rcScreenRect.right));
    m_dwRenderWidth = static_cast<DWORD>(
        m_rcScreenRect.right - m_rcScreenRect.left);
    m_dwRenderHeight = static_cast<DWORD>(
        m_rcScreenRect.bottom - m_rcScreenRect.top);

    DDSURFACEDESC2 description = {};
    description.dwSize = sizeof(description);
    description.dwFlags = DDSD_CAPS;
    description.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    HRESULT result = m_pDD->CreateSurface(
        &description, &m_pddsFrontBuffer, nullptr);
    if (FAILED(result)) {
        return result == DDERR_OUTOFVIDEOMEMORY
            ? result
            : static_cast<HRESULT>(0x82000008);
    }

    IDirectDrawClipper *clipper = nullptr;
    if (FAILED(m_pDD->CreateClipper(0, &clipper, nullptr))) {
        return static_cast<HRESULT>(0x82000009);
    }
    clipper->SetHWnd(0, m_hWnd);
    m_pddsFrontBuffer->SetClipper(clipper);
    if (clipper != nullptr) {
        clipper->Release();
        clipper = nullptr;
    }

    description = {};
    description.dwSize = sizeof(description);
    description.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    description.dwWidth = m_dwRenderWidth;
    description.dwHeight = m_dwRenderHeight;
    description.ddsCaps.dwCaps =
        DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    result = m_pDD->CreateSurface(
        &description, &m_pddsBackBuffer, nullptr);
    if (FAILED(result)) {
        return result == DDERR_OUTOFVIDEOMEMORY
            ? result
            : static_cast<HRESULT>(0x8200000B);
    }
    return S_OK;
}

/* 0x3BF80, 342 bytes, global, 5 named locals
 * CD3DFramework7::CreateZBuffer
 * PDB type: HRESULT CD3DFramework7::(_GUID*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::CreateZBuffer(GUID *device_guid)
{
    D3DDEVICEDESC7 device_description;
    m_pd3dDevice->GetCaps(&device_description);
    if ((device_description.dwDevCaps &
         D3DDEVCAPS_DRAWPRIMITIVES2EX) != 0) {
        return S_OK;
    }

    DDSURFACEDESC2 surface_description;
    surface_description.dwSize = sizeof(surface_description);
    m_pddsBackBuffer->GetSurfaceDesc(&surface_description);
    surface_description.dwFlags =
        DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surface_description.ddsCaps.dwCaps =
        m_dwDeviceMemType | DDSCAPS_ZBUFFER;
    surface_description.ddpfPixelFormat.dwSize = 0;

    m_pD3D->EnumZBufferFormats(
        *device_guid,
        EnumZBufferFormatsCallback,
        &surface_description.ddpfPixelFormat);
    if (surface_description.ddpfPixelFormat.dwSize == 0) {
        surface_description.ddpfPixelFormat.dwZBufferBitDepth = 16;
        m_pD3D->EnumZBufferFormats(
            *device_guid,
            EnumZBufferFormatsCallback,
            &surface_description.ddpfPixelFormat);
        if (surface_description.ddpfPixelFormat.dwSize == 0) {
            return static_cast<HRESULT>(0x82000005);
        }
    }

    HRESULT result = m_pDD->CreateSurface(
        &surface_description, &m_pddsZBuffer, nullptr);
    if (FAILED(result)) {
        return result == DDERR_OUTOFVIDEOMEMORY
            ? result
            : static_cast<HRESULT>(0x82000005);
    }
    if (FAILED(m_pddsBackBuffer->AddAttachedSurface(m_pddsZBuffer))) {
        return static_cast<HRESULT>(0x82000005);
    }
    if (FAILED(m_pd3dDevice->SetRenderTarget(
            m_pddsBackBuffer, 0))) {
        return static_cast<HRESULT>(0x82000005);
    }
    return S_OK;
}

/* 0x3C0E0, 717 bytes, global, 1 named locals
 * CD3DFramework12::DestroyObjects
 * PDB type: HRESULT CD3DFramework12::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework12::DestroyObjects()
{
    ReleaseAndClear(m_pDepthStencil);
    ReleaseAndClear(m_pRenderTargets[0]);
    ReleaseAndClear(m_pCommandAllocators[0]);
    ReleaseAndClear(m_pFence[0]);
    m_mappedConstantBuffer[0] = nullptr;
    ReleaseAndClear(m_pRenderTargets[1]);
    ReleaseAndClear(m_pCommandAllocators[1]);
    ReleaseAndClear(m_pFence[1]);
    m_mappedConstantBuffer[1] = nullptr;
    ReleaseAndClear(m_pCommandList);
    if (m_hFenceEvent != nullptr) {
        CloseHandle(m_hFenceEvent);
        m_hFenceEvent = nullptr;
    }
    ReleaseAndClear(m_pCommandQueue);
    ReleaseAndClear(m_pRtvDescriptorHeap);
    ReleaseAndClear(m_pSwapChain);
    ReleaseAndClear(m_pDevice);
    ReleaseAndClear(m_pFactory);
    ReleaseAndClear(m_pRootSignature);
    ReleaseAndClear(m_pPipelineState);
    ReleaseAndClear(m_pLevelPipelineState);
    ReleaseAndClear(m_pTransparentPipelineState);
    ReleaseAndClear(m_pTransparentGlassPipelineState);
    ReleaseAndClear(m_vertexBuffer);
    ReleaseAndClear(m_indexBuffer);
    ReleaseAndClear(m_constantBuffer);
    ReleaseAndClear(m_3DVertexBuffer);
    ReleaseAndClear(m_3DIndexBuffer);
    ReleaseAndClear(m_pMainDescriptorHeap);
    ReleaseAndClear(m_pDsDescriptorHeap);
    ReleaseAndClear(m_vBufferUploadHeap);

    const CanonicalSDLFrameImports &sdl = GetCanonicalSDLFrameImports();
    if (m_pSDLRenderer != nullptr) {
        sdl.destroy_renderer(m_pSDLRenderer);
        m_pSDLRenderer = nullptr;
    }
    if (m_pSDLWindow != nullptr) {
        sdl.destroy_window(m_pSDLWindow);
        m_pSDLWindow = nullptr;
    }
    sdl.quit();
    return S_OK;
}

/* 0x3C3B0, 208 bytes, global, 3 named locals
 * CD3DFramework7::DestroyObjects
 * PDB type: HRESULT CD3DFramework7::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::DestroyObjects()
{
    if (m_pDD != nullptr) {
        m_pDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL);
    }

    ULONG device_references = 0;
    if (m_pd3dDevice != nullptr) {
        device_references = m_pd3dDevice->Release();
    }
    m_pd3dDevice = nullptr;

    ReleaseAndClear(m_pddsBackBuffer);
    ReleaseAndClear(m_pddsZBuffer);
    ReleaseAndClear(m_pddsFrontBuffer);
    ReleaseAndClear(m_pD3D);

    ULONG direct_draw_references = 0;
    if (m_pDD != nullptr) {
        direct_draw_references = m_pDD->Release();
    }
    m_pDD = nullptr;

    if (device_references != 0 || direct_draw_references != 0) {
        return static_cast<HRESULT>(0x8200000C);
    }
    return S_OK;
}

/* 0x3C480, 31 bytes, local, 2 named locals
 * EnumZBufferFormatsCallback
 * PDB type: HRESULT (_DDPIXELFORMAT*, void*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
static HRESULT CALLBACK EnumZBufferFormatsCallback(
    DDPIXELFORMAT *candidate, void *context)
{
    auto *selected = static_cast<DDPIXELFORMAT *>(context);
    if (candidate->dwZBufferBitDepth != selected->dwZBufferBitDepth) {
        return D3DENUMRET_OK;
    }
    *selected = *candidate;
    return D3DENUMRET_CANCEL;
}

/* 0x3C4A0, 82 bytes, global, 2 named locals
 * CD3DFramework7::FlipToGDISurface
 * PDB type: HRESULT CD3DFramework7::(int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::FlipToGDISurface(int draw_frame)
{
    if (m_pDD != nullptr && m_bIsFullscreen != FALSE) {
        m_pDD->FlipToGDISurface();
        if (draw_frame != 0) {
            DrawMenuBar(m_hWnd);
            RedrawWindow(m_hWnd, nullptr, nullptr, RDW_FRAME);
        }
    }
    return S_OK;
}

/* 0x3C500, 66 bytes, global, 2 named locals
 * CD3DFramework12::FreeShaderResourceDescriptor
 * PDB type: void CD3DFramework12::(unsigned ...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
void CD3DFramework12::FreeShaderResourceDescriptor(
    std::uint64_t descriptor)
{
    PushFreeDescriptor(m_freeShaderResourceDescriptors, descriptor);
}

/* 0x3C550, 72 bytes, global, 5 named locals
 * CD3DFramework12::GetShaderResourceDescriptorIndex
 * PDB type: unsigned __int64 CD3DFramework12...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
std::uint64_t CD3DFramework12::GetShaderResourceDescriptorIndex(
    std::uint64_t descriptor)
{
    const D3D12_CPU_DESCRIPTOR_HANDLE start =
        m_pMainDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    const UINT increment =
        m_pDevice->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    return (descriptor - start.ptr) / increment;
}

/* 0x3C5A0, 128 bytes, global, 7 named locals
 * CD3DFramework7::Initialize
 * PDB type: HRESULT CD3DFramework7::(HWND__*...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::Initialize(
    HWND window,
    GUID *driver_guid,
    GUID *device_guid,
    DDSURFACEDESC2 *mode,
    DWORD flags)
{
    if (window == nullptr || device_guid == nullptr ||
        (mode == nullptr && (flags & 1) != 0)) {
        return E_INVALIDARG;
    }

    m_hWnd = window;
    m_bIsFullscreen = flags & 1;
    const HRESULT result =
        CreateEnvironment(driver_guid, device_guid, mode, flags);
    if (FAILED(result)) {
        DestroyObjects();
        return result;
    }
    return S_OK;
}

/* 0x3C620, 8 bytes, global, 1 named locals
 * CD3DFramework12::IsCommandListOpen
 * PDB type: bool CD3DFramework12::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
bool CD3DFramework12::IsCommandListOpen()
{
    return m_commandListOpen;
}

/* 0x3C630, 144 bytes, global, 5 named locals
 * CD3DFramework12::LoadTextureTGA
 * PDB type: SDL_Texture* CD3DFramework12::(S...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
SDL_Texture *CD3DFramework12::LoadTextureTGA(
    SDL_Renderer *renderer, const char *filename)
{
    const CanonicalSDLFrameImports &sdl = GetCanonicalSDLFrameImports();
    CanonicalSDLSurface *surface = sdl.image_load(filename);
    if (surface == nullptr) {
        std::printf("Failed to load TGA file: %s\n", sdl.get_error());
        return nullptr;
    }

    SDL_Texture *texture =
        sdl.create_texture_from_surface(renderer, surface);
    if (texture == nullptr) {
        std::printf(
            "Failed to create texture from surface: %s\n",
            sdl.get_error());
        sdl.free_surface(surface);
        return nullptr;
    }

    sdl.free_surface(surface);
    return texture;
}

/* 0x3C6C0, 45 bytes, global, 3 named locals
 * CD3DFramework12::Move
 * PDB type: void CD3DFramework12::(int, int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
void CD3DFramework12::Move(int x, int y)
{
    SetWindowPos(
        m_hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/* 0x3C6F0, 44 bytes, global, 3 named locals
 * CD3DFramework7::Move
 * PDB type: void CD3DFramework7::(int, int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
void CD3DFramework7::Move(int x, int y)
{
    if (m_bIsFullscreen != TRUE) {
        SetRect(
            &m_rcScreenRect,
            x,
            y,
            x + static_cast<int>(m_dwRenderWidth),
            y + static_cast<int>(m_dwRenderHeight));
    }
}

/* 0x3C720, 29 bytes, global, 1 named locals
 * CD3DFramework12::Present
 * PDB type: HRESULT CD3DFramework12::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework12::Present()
{
    if (m_pSwapChain == nullptr) {
        return static_cast<HRESULT>(0x8200000F);
    }
    return m_pSwapChain->Present(1, 0);
}

/* 0x3C740, 59 bytes, global, 3 named locals
 * CD3DFramework12::RenderTexture
 * PDB type: void CD3DFramework12::(SDL_Rende...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
void CD3DFramework12::RenderTexture(
    SDL_Renderer *renderer, SDL_Texture *texture)
{
    const CanonicalSDLFrameImports &sdl = GetCanonicalSDLFrameImports();
    sdl.render_clear(renderer);
    sdl.render_copy(renderer, texture, nullptr, nullptr);
    sdl.render_present(renderer);
}

/* 0x3C780, 320 bytes, global, 12 named locals
 * CD3DFramework12::RenderTextureInUI
 * PDB type: void CD3DFramework12::(SDL_Rende...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
void CD3DFramework12::RenderTextureInUI(
    SDL_Renderer *renderer,
    HBITMAP bitmap,
    int x,
    int y,
    int width,
    int height)
{
    const CanonicalSDLFrameImports &sdl = GetCanonicalSDLFrameImports();
    BITMAP bitmap_data;
    GetObjectA(bitmap, sizeof(bitmap_data), &bitmap_data);
    HDC device_context = CreateCompatibleDC(nullptr);
    SelectObject(device_context, bitmap);

    CanonicalSDLSurface *surface = sdl.create_rgb_surface(
        0,
        bitmap_data.bmWidth,
        bitmap_data.bmHeight,
        32,
        0xff000000,
        0x00ff0000,
        0x0000ff00,
        0x000000ff);
    if (surface != nullptr) {
        sdl.lock_surface(surface);
        GetBitmapBits(
            bitmap,
            bitmap_data.bmWidthBytes * bitmap_data.bmHeight,
            surface->pixels);
        sdl.unlock_surface(surface);

        SDL_Texture *texture =
            sdl.create_texture_from_surface(renderer, surface);
        if (texture != nullptr) {
            CanonicalSDLRect destination = {x, y, width, height};
            sdl.render_copy(renderer, texture, nullptr, &destination);
            sdl.render_present(renderer);
            sdl.destroy_texture(texture);
        }
        sdl.free_surface(surface);
    }
    DeleteDC(device_context);
}

/* 0x3C8C0, 283 bytes, global, 6 named locals
 * CD3DFramework12::RenderUI
 * PDB type: void CD3DFramework12::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
void CD3DFramework12::RenderUI()
{
    const CanonicalSDLFrameImports &sdl = GetCanonicalSDLFrameImports();
    sdl.set_render_draw_color(m_pSDLRenderer, 0, 0, 0, 0);
    sdl.render_clear(m_pSDLRenderer);

    const char *path =
        resource_getPath("title00_full.tga", JPB_RESOURCE_FRONT);
    SDL_Texture *texture = LoadTextureTGA(m_pSDLRenderer, path);
    if (texture == nullptr) {
        sdl.destroy_renderer(m_pSDLRenderer);
        sdl.destroy_window(m_pSDLWindow);
        sdl.quit();
        return;
    }

    RenderTexture(m_pSDLRenderer, texture);
    sdl.render_present(m_pSDLRenderer);
}

/* 0x3C9E0, 298 bytes, global, 3 named locals
 * CD3DFramework7::Repaint
 * PDB type: void CD3DFramework7::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
void CD3DFramework7::Repaint()
{
    if (m_bIsFullscreen != FALSE || m_pddsBackBuffer == nullptr) {
        return;
    }

    GetClientRect(m_hWnd, &m_rcScreenRect);
    ClientToScreen(
        m_hWnd, reinterpret_cast<POINT *>(&m_rcScreenRect.left));
    ClientToScreen(
        m_hWnd, reinterpret_cast<POINT *>(&m_rcScreenRect.right));

    PAINTSTRUCT paint;
    HDC paint_context = BeginPaint(m_hWnd, &paint);
    HDC surface_context;
    if (SUCCEEDED(m_pddsBackBuffer->GetDC(&surface_context))) {
        StretchBlt(
            paint_context,
            0,
            0,
            m_rcScreenRect.right - m_rcScreenRect.left,
            m_rcScreenRect.bottom - m_rcScreenRect.top,
            surface_context,
            0,
            0,
            static_cast<int>(m_dwRenderWidth),
            static_cast<int>(m_dwRenderHeight),
            SRCCOPY);
        m_pddsBackBuffer->ReleaseDC(surface_context);
    }
    EndPaint(m_hWnd, &paint);
    OutputDebugStringA("REPAINT!\n");
}

/* 0x3CB10, 348 bytes, global, 5 named locals
 * CD3DFramework12::ResizeDepthBuffer
 * PDB type: HRESULT CD3DFramework12::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework12::ResizeDepthBuffer()
{
    if (m_pDepthStencil != nullptr) {
        m_pDepthStencil->Release();
        m_pDepthStencil = nullptr;
    }

    D3D12_CLEAR_VALUE clear_value = {};
    clear_value.Format = DXGI_FORMAT_D32_FLOAT;
    clear_value.DepthStencil.Depth = 1.0f;
    clear_value.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_properties.CreationNodeMask = 1;
    heap_properties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC depth_description = {};
    depth_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depth_description.Width = m_dwRenderWidth;
    depth_description.Height = m_dwRenderHeight;
    depth_description.DepthOrArraySize = 1;
    depth_description.MipLevels = 0;
    depth_description.Format = DXGI_FORMAT_D32_FLOAT;
    depth_description.SampleDesc.Count = 1;
    depth_description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depth_description.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    const HRESULT result = m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &depth_description,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clear_value,
        IID_PPV_ARGS(&m_pDepthStencil));

    D3D12_DEPTH_STENCIL_VIEW_DESC view_description = {};
    view_description.Format = DXGI_FORMAT_D32_FLOAT;
    view_description.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    view_description.Flags = D3D12_DSV_FLAG_NONE;
    const D3D12_CPU_DESCRIPTOR_HANDLE handle =
        m_pDsDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    m_pDevice->CreateDepthStencilView(
        m_pDepthStencil, &view_description, handle);
    return result;
}

/* 0x3CC70, 692 bytes, global, 12 named locals
 * CD3DFramework12::ResizeResources
 * PDB type: HRESULT CD3DFramework12::(unsign...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework12::ResizeResources(
    UINT new_width, UINT new_height)
{
    if (new_width == 0 || new_height == 0) {
        return E_INVALIDARG;
    }

    for (ID3D12Resource *&render_target : m_pRenderTargets) {
        if (render_target != nullptr) {
            render_target->Release();
            render_target = nullptr;
        }
    }

    HRESULT result = m_pSwapChain->ResizeBuffers(
        2,
        new_width,
        new_height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    if (FAILED(result)) {
        return result;
    }

    m_dwRenderWidth = new_width;
    m_dwRenderHeight = new_height;
    result = CreateRenderTargetViews();
    if (FAILED(result)) {
        return result;
    }
    result = ResizeDepthBuffer();
    if (FAILED(result)) {
        return result;
    }

    m_ScissorRect.right = static_cast<LONG>(new_width);
    m_ScissorRect.bottom = static_cast<LONG>(new_height);
    m_Viewport.Width = static_cast<float>(new_width);
    m_Viewport.Height = static_cast<float>(new_height);
    return S_OK;
}

/* 0x3CF30, 24 bytes, global, 1 named locals
 * CD3DFramework7::RestoreSurfaces
 * PDB type: HRESULT CD3DFramework7::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::RestoreSurfaces()
{
    m_pDD->RestoreAllSurfaces();
    return S_OK;
}

/* 0x3CF50, 86 bytes, global, 1 named locals
 * CD3DFramework7::ShowFrame
 * PDB type: HRESULT CD3DFramework7::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework7::ShowFrame()
{
    if (m_pddsFrontBuffer == nullptr) {
        return static_cast<HRESULT>(0x8200000F);
    }
    if (m_bIsFullscreen != FALSE) {
        return m_pddsFrontBuffer->Flip(nullptr, DDFLIP_WAIT);
    }
    return m_pddsFrontBuffer->Blt(
        &m_rcScreenRect,
        m_pddsBackBuffer,
        nullptr,
        DDBLT_WAIT,
        nullptr);
}

/* 0x3CFB0, 47 bytes, global, 2 named locals
 * CD3DFramework12::TryCloseCommandList
 * PDB type: HRESULT CD3DFramework12::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dframe.cpp
 */
HRESULT CD3DFramework12::TryCloseCommandList()
{
    if (m_commandListOpen) {
        const HRESULT result = m_pCommandList->Close();
        m_commandListOpen = false;
        if (FAILED(result)) {
            return result;
        }
    }
    return S_OK;
}

/* 0x3CFE0, 17 bytes, global, 0 named locals
 * std::vector<unsigned __int64,std::allocator<unsigned __int64> >::_Xlength
 * PDB type: void std::vector<unsigned __int6...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x3D000, 66 bytes, global, 7 named locals
 * std::allocator<unsigned __int64>::deallocate
 * PDB type: void std::allocator<unsigned __i...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x26FBA0, 40 bytes, local, 2 named locals
 * `std::vector<unsigned __int64,std::allocator<unsigned __int64> >::_Emplace_reallocate<unsigned __int64 const &>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */
