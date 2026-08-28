/*
 * COMPLETE REVIEWED RECONSTRUCTION.
 * PDB module: 0025
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\d3dtextr.obj
 * Primary source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 * Compiler language: c++
 * Emitted procedures: 54
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/d3dtextr.h"
#include "jpb/d3dapp.h"
#include "jpb/d3dframe.h"

#include <cstdio>
#include <cstring>
#include <mbstring.h>
#include <string>
#include <vector>

namespace {

struct CanonicalSDLPixelFormat {
    std::uint32_t format;
    void *palette;
    std::uint8_t bits_per_pixel;
    std::uint8_t bytes_per_pixel;
    std::uint8_t padding[2];
    std::uint32_t red_mask;
    std::uint32_t green_mask;
    std::uint32_t blue_mask;
    std::uint32_t alpha_mask;
    std::uint8_t red_loss;
    std::uint8_t green_loss;
    std::uint8_t blue_loss;
    std::uint8_t alpha_loss;
    std::uint8_t red_shift;
    std::uint8_t green_shift;
    std::uint8_t blue_shift;
    std::uint8_t alpha_shift;
    int reference_count;
    CanonicalSDLPixelFormat *next;
};

struct CanonicalSDLRect {
    int x;
    int y;
    int width;
    int height;
};

struct CanonicalSDLSurface {
    std::uint32_t flags;
    CanonicalSDLPixelFormat *format;
    int width;
    int height;
    int pitch;
    void *pixels;
    void *user_data;
    int locked;
    void *list_blitmap;
    CanonicalSDLRect clip_rectangle;
    void *map;
    int reference_count;
};

struct CanonicalSDLImageImports {
    CanonicalSDLSurface *(*image_load)(const char *);
    int (*lock_surface)(CanonicalSDLSurface *);
    const char *(*get_pixel_format_name)(std::uint32_t);
    CanonicalSDLSurface *(*convert_surface_format)(
        CanonicalSDLSurface *, std::uint32_t, std::uint32_t);
    void (*free_surface)(CanonicalSDLSurface *);
    void (*unlock_surface)(CanonicalSDLSurface *);
    const char *(*get_error)();
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

const CanonicalSDLImageImports &GetCanonicalSDLImageImports()
{
    static CanonicalSDLImageImports imports = {};
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
        imports.lock_surface = ResolveCanonicalSDLImport<
            decltype(imports.lock_surface)>(sdl_module, "SDL_LockSurface");
        imports.get_pixel_format_name = ResolveCanonicalSDLImport<
            decltype(imports.get_pixel_format_name)>(
                sdl_module, "SDL_GetPixelFormatName");
        imports.convert_surface_format = ResolveCanonicalSDLImport<
            decltype(imports.convert_surface_format)>(
                sdl_module, "SDL_ConvertSurfaceFormat");
        imports.free_surface = ResolveCanonicalSDLImport<
            decltype(imports.free_surface)>(sdl_module, "SDL_FreeSurface");
        imports.unlock_surface = ResolveCanonicalSDLImport<
            decltype(imports.unlock_surface)>(
                sdl_module, "SDL_UnlockSurface");
        imports.get_error = ResolveCanonicalSDLImport<
            decltype(imports.get_error)>(sdl_module, "SDL_GetError");
        initialized = true;
    }
    return imports;
}

static_assert(sizeof(CanonicalSDLPixelFormat) == 56,
              "SDL_PixelFormat ABI changed");
static_assert(sizeof(CanonicalSDLSurface) == 96,
              "SDL_Surface ABI changed");

void DestroyCanonicalSDLTexture(SDL_Texture *texture)
{
    HMODULE sdl_module = GetModuleHandleA("SDL2.dll");
    if (sdl_module == nullptr) {
        RaiseFailFastException(nullptr, nullptr, 0);
    }

    using DestroyTextureFunction = void (*)(SDL_Texture *);
    auto destroy_texture = reinterpret_cast<DestroyTextureFunction>(
        GetProcAddress(sdl_module, "SDL_DestroyTexture"));
    if (destroy_texture == nullptr) {
        RaiseFailFastException(nullptr, nullptr, 0);
    }
    destroy_texture(texture);
}

} // namespace

static char g_strTexturePath[260] = "c:\\el_chavo\\work\\level";
static Texture *g_ptcTextureList;

/* 0x3DED0, 40 bytes, global, 2 named locals
 * IID_PPV_ARGS_Helper<Microsoft::WRL::ComPtr<ID3D12Resource> >
 * PDB type: void** (Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x3DF00, 8 bytes, global, 3 named locals
 * std::_Fill_memset<unsigned char *,unsigned char>
 * PDB type: void (unsigned char*, const unsi...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x3DF10, 288 bytes, global, 6 named locals
 * Texture::Texture
 * PDB type: void Texture::(char*, unsigned l...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
Texture::Texture(char *name, DWORD stage, DWORD flags)
    : PHL::Texture2D(0, 0, static_cast<PHL::TextureFormat>(0))
{
    if (name == nullptr) {
        std::memcpy(m_strName, "UNASSIGNED!", 12);
    } else {
        lstrcpyA(m_strName, name);
    }

    m_dwWidth = 0;
    m_dwHeight = 0;
    m_dwStage = stage;
    m_dwBPP = 0;
    m_dwFlags = flags;
    m_bHasAlpha = FALSE;
    m_hbmBitmap = nullptr;
    m_pRGBAData = nullptr;
    m_pSDLTexture = nullptr;
    m_pDescriptor = nullptr;
    m_isTransparent = false;

    if (g_ptcTextureList != nullptr) {
        g_ptcTextureList->prev = this;
    }
    next = g_ptcTextureList;
    prev = nullptr;
    g_ptcTextureList = this;
}

/* 0x3E030, 213 bytes, global, 4 named locals
 * Texture::Texture
 * PDB type: void Texture::(unsigned __int64,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
Texture::Texture(
    std::uint64_t width,
    std::uint64_t height,
    PHL::TextureFormat format)
    : PHL::Texture2D(width, height, format)
{
    m_dwWidth = static_cast<DWORD>(width);
    m_dwHeight = static_cast<DWORD>(height);
    m_dwBPP = 32;
    m_bHasAlpha = TRUE;
    m_dwPitch = static_cast<DWORD>(
        (width & 0x07ffffffffffffffULL) << 2);
    m_hbmBitmap = nullptr;
    m_pRGBAData = nullptr;
    m_pSDLTexture = nullptr;
    m_pDescriptor = nullptr;
    m_isTransparent = false;
    std::memcpy(m_strName, "UNASSIGNED!", 12);

    CreateTextureResource();
    if (g_ptcTextureList != nullptr) {
        g_ptcTextureList->prev = this;
    }
    next = g_ptcTextureList;
    prev = nullptr;
    g_ptcTextureList = this;
}

/* 0x3E110, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>::~ComPtr<ID3D12DescriptorHeap>
 * PDB type: void Microsoft::WRL::ComPtr<ID3D...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x3E140, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<ID3D12Resource>::~ComPtr<ID3D12Resource>
 * PDB type: void Microsoft::WRL::ComPtr<ID3D...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x3E170, 94 bytes, global, 5 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::~basic_string<char,std::char_traits<char>,std::allocator<char> >
 * PDB type: void std::basic_string<char,std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x3E1D0, 87 bytes, global, 6 named locals
 * std::vector<unsigned char,std::allocator<unsigned char> >::~vector<unsigned char,std::allocator<unsigned char> >
 * PDB type: void std::vector<unsigned char,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x3E230, 11 bytes, global, 1 named locals
 * PHL::Texture2D::~Texture2D
 * PDB type: void PHL::Texture2D::()
 * Source: W:\SWJediPowerBattles\work\rendering\Texture2D.h
 */
PHL::Texture2D::~Texture2D() = default;

/* 0x3E240, 303 bytes, global, 7 named locals
 * Texture::~Texture
 * PDB type: void Texture::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
Texture::~Texture()
{
    if (m_pRGBAData != nullptr) {
        ::operator delete(m_pRGBAData, static_cast<std::size_t>(1));
        m_pRGBAData = nullptr;
    }
    if (m_hbmBitmap != nullptr) {
        DeleteObject(m_hbmBitmap);
    }
    if (m_pSDLTexture != nullptr) {
        DestroyCanonicalSDLTexture(m_pSDLTexture);
    }

    Texture *old_prev = prev;
    Texture *old_next = next;
    g_pD3DApp->debugtrace(
        const_cast<char *>("Deleting %s\n"), m_strName);
    Texture *new_head = old_next;
    if (old_prev != nullptr) {
        old_prev->next = old_next;
        new_head = g_ptcTextureList;
    }
    g_ptcTextureList = new_head;
    if (old_next != nullptr) {
        old_next->prev = old_prev;
    }

    if (m_pDescriptor != nullptr && g_pD3DApp != nullptr &&
        g_pD3DApp->m_pFramework != nullptr) {
        g_pD3DApp->m_pFramework->FreeShaderResourceDescriptor(
            reinterpret_cast<std::uint64_t>(m_pDescriptor));
    }
}

/* 0x3E370, 43 bytes, global, 1 named locals
 * PHL::Texture2D::`scalar deleting destructor'
 * PDB type: void* PHL::Texture2D::(unsigned)
 * Source: no line mapping
 */

/* 0x3E3A0, 341 bytes, global, 7 named locals
 * Texture::`scalar deleting destructor'
 * PDB type: void* Texture::(unsigned)
 * Source: no line mapping
 */

/* 0x3E500, 894 bytes, global, 34 named locals
 * Texture::CopyBitmapToSurface
 * PDB type: HRESULT Texture::(ID3D12Device*,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT Texture::CopyBitmapToSurface(
    ID3D12Device *device,
    ID3D12GraphicsCommandList *command_list)
{
    if (m_hbmBitmap == nullptr || device == nullptr) {
        return E_INVALIDARG;
    }

    BITMAP bitmap;
    GetObjectA(m_hbmBitmap, sizeof(bitmap), &bitmap);

    const int pixel_count = bitmap.bmWidth * bitmap.bmHeight;
    const std::uint32_t bitmap_byte_count =
        static_cast<std::uint32_t>(bitmap.bmBitsPixel) *
        static_cast<std::uint32_t>(pixel_count);
    std::vector<unsigned char> bitmap_data(bitmap_byte_count);
    GetBitmapBits(
        m_hbmBitmap,
        static_cast<LONG>(bitmap_byte_count),
        bitmap_data.data());

    const std::uint32_t rgba_byte_count =
        static_cast<std::uint32_t>(pixel_count) * 4;
    std::vector<unsigned char> cpu_data(rgba_byte_count);
    std::memcpy(cpu_data.data(), bitmap_data.data(), cpu_data.size());

    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heap_properties.CreationNodeMask = 1;
    heap_properties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resource_description = {};
    resource_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_description.Width = cpu_data.size();
    resource_description.Height = 1;
    resource_description.DepthOrArraySize = 1;
    resource_description.MipLevels = 1;
    resource_description.SampleDesc.Count = 1;
    resource_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> upload_buffer;
    HRESULT result = device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(upload_buffer.GetAddressOf()));
    if (SUCCEEDED(result)) {
        result = S_OK;
        D3D12_RANGE read_range = {};
        void *mapped_data = nullptr;
        if (SUCCEEDED(upload_buffer->Map(0, &read_range, &mapped_data))) {
            std::memcpy(mapped_data, cpu_data.data(), cpu_data.size());
            upload_buffer->Unmap(0, nullptr);
        }

        D3D12_SUBRESOURCE_DATA texture_data = {};
        texture_data.pData = bitmap_data.data();
        texture_data.RowPitch =
            static_cast<LONG_PTR>(bitmap.bmBitsPixel * bitmap.bmWidth);
        texture_data.SlicePitch = m_dwPitch;
        UpdateSubresources(
            command_list,
            m_pTextureResource.Get(),
            upload_buffer.Get(),
            0,
            0,
            1,
            &texture_data);
    }
    return result;
}

/* 0x3E880, 683 bytes, global, 13 named locals
 * Texture::CopyRGBADataToSurface
 * PDB type: HRESULT Texture::(ID3D12Device*,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT Texture::CopyRGBADataToSurface(
    ID3D12Device *device,
    ID3D12GraphicsCommandList *command_list)
{
    if (m_pRGBAData == nullptr || m_pTextureResource == nullptr) {
        return E_FAIL;
    }

    D3D12_HEAP_PROPERTIES default_heap = {};
    default_heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    default_heap.CreationNodeMask = 1;
    default_heap.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC texture_description = {};
    texture_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture_description.Width = 256;
    texture_description.Height = 256;
    texture_description.DepthOrArraySize = 1;
    texture_description.MipLevels = 1;
    texture_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_description.SampleDesc.Count = 1;

    device->CreateCommittedResource(
        &default_heap,
        D3D12_HEAP_FLAG_NONE,
        &texture_description,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_pTextureResource.ReleaseAndGetAddressOf()));

    const std::uint64_t required_size =
        GetRequiredIntermediateSize(m_pTextureResource.Get(), 0, 1);
    D3D12_HEAP_PROPERTIES upload_heap = {};
    upload_heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    upload_heap.CreationNodeMask = 1;
    upload_heap.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC upload_description = {};
    upload_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload_description.Width = required_size;
    upload_description.Height = 1;
    upload_description.DepthOrArraySize = 1;
    upload_description.MipLevels = 1;
    upload_description.SampleDesc.Count = 1;
    upload_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> upload_buffer;
    HRESULT result = device->CreateCommittedResource(
        &upload_heap,
        D3D12_HEAP_FLAG_NONE,
        &upload_description,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(upload_buffer.GetAddressOf()));
    if (SUCCEEDED(result)) {
        D3D12_RANGE read_range = {};
        void *mapped_data = nullptr;
        result = upload_buffer->Map(0, &read_range, &mapped_data);
        if (SUCCEEDED(result)) {
            std::memcpy(
                mapped_data,
                m_pRGBAData,
                static_cast<std::size_t>(m_dwWidth) *
                    static_cast<std::size_t>(m_dwHeight) * 4);
            upload_buffer->Unmap(0, nullptr);

            D3D12_TEXTURE_COPY_LOCATION destination = {};
            destination.pResource = m_pTextureResource.Get();
            destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            destination.SubresourceIndex = 0;

            D3D12_TEXTURE_COPY_LOCATION source = {};
            source.pResource = upload_buffer.Get();
            source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            source.PlacedFootprint.Offset = 0;
            source.PlacedFootprint.Footprint.Format =
                DXGI_FORMAT_R8G8B8A8_UNORM;
            source.PlacedFootprint.Footprint.Width = m_dwWidth;
            source.PlacedFootprint.Footprint.Height = m_dwHeight;
            source.PlacedFootprint.Footprint.Depth = 1;
            source.PlacedFootprint.Footprint.RowPitch =
                (m_dwWidth * 4 + 255) & ~255U;

            command_list->CopyTextureRegion(
                &destination, 0, 0, 0, &source, nullptr);
        }
    }
    return result;
}

/* 0x3EB30, 104 bytes, global, 6 named locals
 * CreateEmptyTexture
 * PDB type: Texture* (char*, unsigned long, ...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
Texture *CreateEmptyTexture(
    char *name,
    DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwStage,
    DWORD dwFlags)
{
    (void)dwWidth;
    (void)dwHeight;

    Texture *texture = new Texture(name, dwStage, dwFlags);
    if (texture != nullptr) {
        texture->m_dwWidth = 0;
        texture->m_dwHeight = 0;
        texture->m_dwBPP = 0;
    }
    return texture;
}

/* 0x3EBA0, 111 bytes, global, 2 named locals
 * CreateNonTexturedTexture
 * PDB type: Texture* (int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
Texture *CreateNonTexturedTexture(int type)
{
    Texture *texture = new Texture(nullptr, 0, 0);
    if (texture != nullptr) {
        texture->m_dwWidth = 0;
        texture->m_dwHeight = 0;
        texture->m_pRGBAData = nullptr;
        texture->m_hbmBitmap = nullptr;
        texture->m_dwStage = 0;
        texture->m_type = static_cast<unsigned>(type);
        texture->m_nIndex = -1;
    }
    return texture;
}

/* 0x3EC10, 1705 bytes, global, 44 named locals
 * Texture::CreateSRVHeap
 * PDB type: HRESULT Texture::(CD3DFramework1...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT Texture::CreateSRVHeap(CD3DFramework12 *framework)
{
    if (framework->IsCommandListOpen()) {
        framework->TryCloseCommandList();
        ID3D12CommandList *command_lists[] = {framework->m_pCommandList};
        framework->m_pCommandQueue->ExecuteCommandLists(1, command_lists);
    }
    WaitForGpuTexture(framework);

    const UINT frame_index = framework->m_nFrameIndex;
    HRESULT result = framework->m_pCommandAllocators[frame_index]->Reset();
    if (FAILED(result)) {
        return result;
    }
    if (!framework->IsCommandListOpen()) {
        result = framework->m_pCommandList->Reset(
            framework->m_pCommandAllocators[frame_index],
            framework->m_pPipelineState);
        if (FAILED(result)) {
            return result;
        }
        framework->m_commandListOpen = true;
    }

    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resource_description = {};
    resource_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_description.Width = m_dwWidth;
    resource_description.Height = m_dwHeight;
    resource_description.DepthOrArraySize = 1;
    resource_description.MipLevels = 1;
    resource_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resource_description.SampleDesc.Count = 1;

    result = framework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(m_pTextureResource.ReleaseAndGetAddressOf()));
    if (FAILED(result)) {
        return result;
    }

    std::string resource_name(m_strName);
    const std::size_t resource_prefix = resource_name.find("/res/");
    if (resource_prefix != std::string::npos) {
        resource_name = resource_name.substr(resource_prefix + 1);
    }
    m_pTextureResource->SetPrivateData(
        WKPDID_D3DDebugObjectName,
        static_cast<UINT>(resource_name.size()),
        resource_name.c_str());

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_description = {};
    srv_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_description.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_description.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_description.Texture2D.MipLevels = 1;

    const std::uint64_t descriptor =
        framework->AllocateShaderResourceDescriptor();
    m_nIndex = static_cast<int>(
        framework->GetShaderResourceDescriptorIndex(descriptor));
    m_pDescriptor = reinterpret_cast<void *>(descriptor);
    if (descriptor == 0) {
        return E_OUTOFMEMORY;
    }
    if (m_hbmBitmap == nullptr || framework->m_pDevice == nullptr) {
        return E_INVALIDARG;
    }

    BITMAP bitmap;
    GetObjectA(m_hbmBitmap, sizeof(bitmap), &bitmap);
    const std::uint32_t bitmap_byte_count =
        static_cast<std::uint32_t>(bitmap.bmHeight) *
        static_cast<std::uint32_t>(bitmap.bmWidthBytes);
    std::vector<unsigned char> bitmap_data(bitmap_byte_count);
    GetBitmapBits(
        m_hbmBitmap,
        static_cast<LONG>(bitmap_byte_count),
        bitmap_data.data());

    const std::uint64_t required_size =
        GetRequiredIntermediateSize(m_pTextureResource.Get(), 0, 1);
    D3D12_HEAP_PROPERTIES upload_heap = {};
    upload_heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    upload_heap.CreationNodeMask = 1;
    upload_heap.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC upload_description = {};
    upload_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload_description.Width = required_size;
    upload_description.Height = 1;
    upload_description.DepthOrArraySize = 1;
    upload_description.MipLevels = 1;
    upload_description.SampleDesc.Count = 1;
    upload_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> upload_buffer;
    result = framework->m_pDevice->CreateCommittedResource(
        &upload_heap,
        D3D12_HEAP_FLAG_NONE,
        &upload_description,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(upload_buffer.GetAddressOf()));
    if (SUCCEEDED(result)) {
        D3D12_SUBRESOURCE_DATA source = {};
        source.pData = bitmap_data.data();
        source.RowPitch = bitmap.bmWidthBytes;
        const int slice_pitch = bitmap.bmHeight * bitmap.bmWidthBytes;
        source.SlicePitch = slice_pitch;
        UpdateSubresources(
            framework->m_pCommandList,
            m_pTextureResource.Get(),
            upload_buffer.Get(),
            0,
            0,
            1,
            &source);
        framework->m_pDevice->CreateShaderResourceView(
            m_pTextureResource.Get(),
            &srv_description,
            D3D12_CPU_DESCRIPTOR_HANDLE{descriptor});

        framework->TryCloseCommandList();
        WaitForGpuTexture(framework);
        ID3D12CommandList *command_lists[] = {framework->m_pCommandList};
        framework->m_pCommandQueue->ExecuteCommandLists(1, command_lists);
        WaitForGpuTexture(framework);

        result = framework->m_pCommandAllocators[frame_index]->Reset();
        if (SUCCEEDED(result)) {
            if (!framework->IsCommandListOpen()) {
                result = framework->m_pCommandList->Reset(
                    framework->m_pCommandAllocators[frame_index],
                    framework->m_pPipelineState);
                if (SUCCEEDED(result)) {
                    framework->m_commandListOpen = true;
                }
            }
            if (SUCCEEDED(result)) {
                result = S_OK;
            }
        }
    }
    return result;
}

/* 0x3F2C0, 564 bytes, global, 16 named locals
 * CreateTextureFromFile
 * PDB type: Texture* (char*, unsigned long, ...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
Texture *CreateTextureFromFile(
    char *name,
    DWORD stage,
    DWORD flags,
    int flip,
    int desired_format,
    int type,
    CD3DFramework12 *framework)
{
    if (framework->PAIN != nullptr) {
        framework->PAIN->Release();
    }
    if (framework->PAINTEX != nullptr) {
        framework->PAINTEX->Release();
    }
    if (name == nullptr) {
        return nullptr;
    }

    Texture *texture = new Texture(name, stage, flags);
    if (texture == nullptr) {
        return nullptr;
    }
    texture->m_hbmBitmap = nullptr;
    if (FAILED(texture->LoadImageData(flip))) {
        HBITMAP bitmap = nullptr;
        HDC device_context = CreateCompatibleDC(nullptr);
        if (device_context != nullptr) {
            BITMAPINFO bitmap_information = {};
            bitmap_information.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmap_information.bmiHeader.biWidth = 256;
            bitmap_information.bmiHeader.biHeight = -256;
            bitmap_information.bmiHeader.biPlanes = 1;
            bitmap_information.bmiHeader.biBitCount = 32;
            void *bits = nullptr;
            bitmap = CreateDIBSection(
                device_context,
                &bitmap_information,
                DIB_RGB_COLORS,
                &bits,
                nullptr,
                0);
            if (bitmap != nullptr) {
                auto *pixels = static_cast<std::uint32_t *>(bits);
                for (int y = 0; y < 256; ++y) {
                    const int row_mask = (y & 0x0c) != 0 ? 3 : 0;
                    for (int x = 0; x < 256; ++x) {
                        pixels[y * 256 + x] =
                            (((x >> 2) & row_mask) == 0)
                            ? UINT32_C(0x00ff7f3f)
                            : UINT32_C(0x007f3fff);
                    }
                }
            }
            DeleteDC(device_context);
        }
        texture->m_hbmBitmap = bitmap;
    }

    texture->m_type = static_cast<unsigned>(type);
    if (texture->m_hbmBitmap != nullptr) {
        BITMAP bitmap;
        GetObjectA(texture->m_hbmBitmap, sizeof(bitmap), &bitmap);
        texture->m_dwWidth = bitmap.bmWidth;
        texture->m_dwHeight = bitmap.bmHeight;
        texture->m_dwBPP = bitmap.bmBitsPixel;
    } else if (texture->m_pRGBAData == nullptr) {
        delete texture;
        return nullptr;
    }

    texture->m_tpfDesired = desired_format;
    if (FAILED(texture->CreateSRVHeap(framework))) {
        delete texture;
        return nullptr;
    }
    return texture;
}

/* 0x3F500, 1290 bytes, global, 29 named locals
 * Texture::CreateTextureResource
 * PDB type: HRESULT Texture::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT Texture::CreateTextureResource()
{
    CD3DFramework12 *framework = g_pD3DApp->m_pFramework;
    const UINT frame_index = framework->m_nFrameIndex;

    if (framework->IsCommandListOpen()) {
        framework->TryCloseCommandList();
        ID3D12CommandList *command_lists[] = {framework->m_pCommandList};
        framework->m_pCommandQueue->ExecuteCommandLists(1, command_lists);
    }
    WaitForGpuTexture(framework);

    HRESULT result = framework->m_pCommandAllocators[frame_index]->Reset();
    if (FAILED(result)) {
        return result;
    }
    if (!framework->IsCommandListOpen()) {
        result = framework->m_pCommandList->Reset(
            framework->m_pCommandAllocators[frame_index],
            framework->m_pPipelineState);
        if (FAILED(result)) {
            return result;
        }
        framework->m_commandListOpen = true;
    }

    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resource_description = {};
    resource_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_description.Width = GetWidth();
    resource_description.Height = static_cast<UINT>(GetHeight());
    resource_description.DepthOrArraySize = 1;
    resource_description.MipLevels = 1;
    resource_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resource_description.SampleDesc.Count = 1;

    if (framework->m_pDevice == nullptr) {
        result = E_INVALIDARG;
    } else {
        result = framework->m_pDevice->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_description,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(m_pTextureResource.ReleaseAndGetAddressOf()));
    }
    if (FAILED(result)) {
        return result;
    }

    std::string resource_name(m_strName);
    const std::size_t resource_prefix = resource_name.find("/res/");
    if (resource_prefix != std::string::npos) {
        resource_name = resource_name.substr(resource_prefix + 1);
    }
    m_pTextureResource->SetPrivateData(
        WKPDID_D3DDebugObjectName,
        static_cast<UINT>(resource_name.size()),
        resource_name.c_str());

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_description = {};
    srv_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_description.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_description.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_description.Texture2D.MipLevels = 1;

    const std::uint64_t descriptor =
        framework->AllocateShaderResourceDescriptor();
    m_nIndex = static_cast<int>(
        framework->GetShaderResourceDescriptorIndex(descriptor));
    m_pDescriptor = reinterpret_cast<void *>(descriptor);
    if (descriptor == 0) {
        return E_OUTOFMEMORY;
    }
    framework->m_pDevice->CreateShaderResourceView(
        m_pTextureResource.Get(), &srv_description,
        D3D12_CPU_DESCRIPTOR_HANDLE{descriptor});

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_pTextureResource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    framework->m_pCommandList->ResourceBarrier(1, &barrier);

    framework->TryCloseCommandList();
    WaitForGpuTexture(framework);
    ID3D12CommandList *command_lists[] = {framework->m_pCommandList};
    framework->m_pCommandQueue->ExecuteCommandLists(1, command_lists);
    WaitForGpuTexture(framework);

    result = framework->m_pCommandAllocators[frame_index]->Reset();
    if (SUCCEEDED(result) && !framework->IsCommandListOpen()) {
        result = framework->m_pCommandList->Reset(
            framework->m_pCommandAllocators[frame_index],
            framework->m_pPipelineState);
        if (SUCCEEDED(result)) {
            framework->m_commandListOpen = true;
            result = S_OK;
        }
    }
    return result;
}

/* 0x3FA10, 118 bytes, global, 3 named locals
 * DeleteAllTextures
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
void DeleteAllTextures()
{
    Texture *texture = g_ptcTextureList;
    while (texture != nullptr) {
        Texture *next = texture->next;
        if (texture->m_pTextureResource != nullptr) {
            texture->m_pTextureResource->Release();
            texture->m_pTextureResource.Reset();
        }
        delete texture;
        texture = next;
    }
}

/* 0x3FA90, 14 bytes, global, 1 named locals
 * Texture::GetBitsPerPixel
 * PDB type: unsigned __int64 Texture::(DXGI_...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
std::uint64_t Texture::GetBitsPerPixel(DXGI_FORMAT format)
{
    return format == DXGI_FORMAT_R8G8B8A8_UNORM ? 32 : 0;
}

/* 0x3FAA0, 8 bytes, global, 1 named locals
 * Texture::GetNativeResource
 * PDB type: void* Texture::()
 * Source: W:\SWJediPowerBattles\work\d3d\D3DTextr.h
 */
void *Texture::GetNativeResource()
{
    return m_pTextureResource.Get();
}

/* 0x3FAB0, 191 bytes, global, 6 named locals
 * GetRequiredIntermediateSize
 * PDB type: unsigned __int64 (ID3D12Resource...
 * Source: W:\SWJediPowerBattles\work\d3d\directx\d3dx12_resource_helpers.h
 */
std::uint64_t GetRequiredIntermediateSize(
    ID3D12Resource *destination_resource,
    UINT first_subresource,
    UINT subresource_count)
{
    const D3D12_RESOURCE_DESC description =
        destination_resource->GetDesc();
    ID3D12Device *device = nullptr;
    std::uint64_t required_size = 0;

    destination_resource->GetDevice(
        IID_PPV_ARGS(&device));
    device->GetCopyableFootprints(
        &description,
        first_subresource,
        subresource_count,
        0,
        nullptr,
        nullptr,
        nullptr,
        &required_size);
    device->Release();
    return required_size;
}

/* 0x3FB70, 64 bytes, global, 2 named locals
 * Texture::Invalidate
 * PDB type: void Texture::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
void Texture::Invalidate()
{
    if (m_pTextureResource != nullptr) {
        m_pTextureResource->Release();
        m_pTextureResource.Reset();
    }
}

/* 0x3FBB0, 99 bytes, global, 2 named locals
 * InvalidateAllTextures
 * PDB type: HRESULT ()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT InvalidateAllTextures()
{
    for (Texture *texture = g_ptcTextureList;
         texture != nullptr;
         texture = texture->next) {
        texture->Invalidate();
    }
    return S_OK;
}

/* 0x3FC20, 231 bytes, global, 3 named locals
 * Texture::LoadBitmapFile
 * PDB type: HRESULT Texture::(char*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT Texture::LoadBitmapFile(char *path)
{
    char real_name[MAX_PATH];

    if (path[1] == ':') {
        char *destination = real_name;
        do {
            *destination++ = *path;
        } while (*path++ != '\0');
    } else {
        std::strncpy(real_name, g_strTexturePath, MAX_PATH);
        std::strncat(real_name, "\\", MAX_PATH);
        std::strncat(real_name, path, MAX_PATH);
    }

    m_hbmBitmap = static_cast<HBITMAP>(LoadImageA(
        nullptr,
        real_name,
        IMAGE_BITMAP,
        0,
        0,
        LR_CREATEDIBSECTION | LR_LOADFROMFILE));
    return m_hbmBitmap != nullptr
        ? S_OK
        : static_cast<HRESULT>(0x887600ffL);
}

/* 0x3FD10, 749 bytes, global, 15 named locals
 * Texture::LoadImageData
 * PDB type: HRESULT Texture::(int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT Texture::LoadImageData(int flip)
{
    HINSTANCE instance = GetModuleHandleA(nullptr);
    m_hbmBitmap = static_cast<HBITMAP>(LoadImageA(
        instance,
        m_strName,
        IMAGE_BITMAP,
        0,
        0,
        LR_CREATEDIBSECTION));
    if (m_hbmBitmap != nullptr) {
        return S_OK;
    }

    char *extension = reinterpret_cast<char *>(
        _mbsrchr(reinterpret_cast<unsigned char *>(m_strName), '.'));
    if (extension == nullptr) {
        return E_NOTIMPL;
    }

    const int bitmap_extension = lstrcmpiA(extension, ".bmp");
    const CanonicalSDLImageImports &sdl = GetCanonicalSDLImageImports();
    CanonicalSDLSurface *surface = sdl.image_load(m_strName);
    if (surface == nullptr) {
        return E_NOTIMPL;
    }

    sdl.lock_surface(surface);
    std::string format_name(
        sdl.get_pixel_format_name(surface->format->format));
    (void)format_name;
    const std::uint32_t converted_format = bitmap_extension == 0
        ? UINT32_C(0x16561804)
        : UINT32_C(0x16762004);
    CanonicalSDLSurface *converted_surface =
        sdl.convert_surface_format(surface, converted_format, 0);
    sdl.free_surface(surface);

    m_dwWidth = converted_surface->width;
    m_dwHeight = converted_surface->height;
    m_dwBPP = converted_surface->format->bits_per_pixel;
    m_isTransparent = converted_surface->format->alpha_mask != 0;
    const int allocation_size =
        converted_surface->pitch * converted_surface->height;
    m_pRGBAData = new unsigned char[allocation_size];

    sdl.lock_surface(converted_surface);
    unsigned char *destination = m_pRGBAData;
    const auto *source = static_cast<const unsigned char *>(
        converted_surface->pixels);
    if (flip != 0) {
        for (int row = m_dwHeight - 1; row >= 0; --row) {
            std::memcpy(
                destination,
                source + row * converted_surface->pitch,
                converted_surface->pitch);
            destination += converted_surface->pitch;
        }
    } else {
        const std::uint32_t copy_size =
            static_cast<std::uint32_t>(
                converted_surface->format->bytes_per_pixel) *
            static_cast<std::uint32_t>(m_dwHeight) *
            static_cast<std::uint32_t>(m_dwWidth);
        std::memcpy(destination, source, copy_size);
    }
    sdl.unlock_surface(converted_surface);
    sdl.free_surface(converted_surface);

    HDC device_context = GetDC(nullptr);
    m_hbmBitmap = CreateBitmap(
        m_dwWidth,
        m_dwHeight,
        1,
        m_dwBPP,
        m_pRGBAData);
    if (m_hbmBitmap == nullptr) {
        std::printf(
            "Failed to create bitmap. Error code: %lu\n",
            GetLastError());
        delete[] m_pRGBAData;
        m_pRGBAData = nullptr;
        return E_FAIL;
    }

    ReleaseDC(nullptr, device_context);
    return S_OK;
}

/* 0x40000, 486 bytes, global, 11 named locals
 * Texture::LoadPNGFile
 * PDB type: HRESULT Texture::(int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT Texture::LoadPNGFile(int flip)
{
    const CanonicalSDLImageImports &sdl = GetCanonicalSDLImageImports();
    CanonicalSDLSurface *surface = sdl.image_load(m_strName);
    if (surface == nullptr) {
        return E_NOTIMPL;
    }

    sdl.lock_surface(surface);
    if (surface->format->format != UINT32_C(0x16762004)) {
        CanonicalSDLSurface *converted_surface =
            sdl.convert_surface_format(
                surface, UINT32_C(0x16762004), 0);
        sdl.free_surface(surface);
        surface = converted_surface;
    }

    m_dwWidth = surface->width;
    m_dwHeight = surface->height;
    m_dwBPP = surface->format->bits_per_pixel;
    const int allocation_size = surface->height * surface->pitch;
    m_pRGBAData = new unsigned char[allocation_size];

    sdl.lock_surface(surface);
    unsigned char *destination = m_pRGBAData;
    const auto *source = static_cast<const unsigned char *>(surface->pixels);
    if (flip != 0) {
        for (int row = m_dwHeight - 1; row >= 0; --row) {
            std::memcpy(
                destination,
                source + row * surface->pitch,
                surface->pitch);
            destination += surface->pitch;
        }
    } else {
        const std::uint32_t copy_size =
            static_cast<std::uint32_t>(surface->format->bytes_per_pixel) *
            static_cast<std::uint32_t>(m_dwHeight) *
            static_cast<std::uint32_t>(m_dwWidth);
        std::memcpy(destination, source, copy_size);
    }
    sdl.unlock_surface(surface);
    sdl.free_surface(surface);

    HDC device_context = GetDC(nullptr);
    m_hbmBitmap = CreateBitmap(
        m_dwWidth,
        m_dwHeight,
        1,
        m_dwBPP,
        m_pRGBAData);
    if (m_hbmBitmap == nullptr) {
        std::printf(
            "Failed to create bitmap. Error code: %lu\n",
            GetLastError());
        delete[] m_pRGBAData;
        m_pRGBAData = nullptr;
        return E_FAIL;
    }

    ReleaseDC(nullptr, device_context);
    return S_OK;
}

/* 0x401F0, 681 bytes, global, 12 named locals
 * Texture::LoadTargaFile
 * PDB type: HRESULT Texture::(char*, int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT Texture::LoadTargaFile(char *path_name, int flip)
{
    char path[MAX_PATH];
    strcpy_s(path, sizeof(path), path_name);

    const CanonicalSDLImageImports &sdl = GetCanonicalSDLImageImports();
    CanonicalSDLSurface *surface = sdl.image_load(path);
    if (surface == nullptr) {
        std::printf("Failed to load TGA file: %s\n", sdl.get_error());
        return E_FAIL;
    }

    CanonicalSDLSurface *converted_surface = sdl.convert_surface_format(
        surface, UINT32_C(0x16762004), 0);
    if (converted_surface == nullptr) {
        std::printf(
            "Failed to convert surface format: %s\n",
            sdl.get_error());
        sdl.free_surface(surface);
        return E_FAIL;
    }

    m_dwWidth = converted_surface->width;
    m_dwHeight = converted_surface->height;
    m_dwBPP = converted_surface->format->bits_per_pixel;
    m_isTransparent = converted_surface->format->alpha_mask != 0;
    const std::uint32_t allocation_size =
        (static_cast<std::uint32_t>(m_dwBPP) >> 3) *
        static_cast<std::uint32_t>(m_dwHeight) *
        static_cast<std::uint32_t>(m_dwWidth);
    m_pRGBAData = new unsigned char[allocation_size];
    if (m_pRGBAData == nullptr) {
        std::printf("Failed to allocate memory for RGBA data\n");
        sdl.free_surface(converted_surface);
        sdl.free_surface(surface);
        return E_FAIL;
    }

    sdl.lock_surface(converted_surface);
    const auto *source = static_cast<const unsigned char *>(
        converted_surface->pixels);
    unsigned char *destination = m_pRGBAData;
    m_dwPitch = converted_surface->pitch;
    if (flip != 0) {
        for (int row = m_dwHeight - 1; row >= 0; --row) {
            std::memcpy(
                destination,
                source + row * converted_surface->pitch,
                converted_surface->pitch);
            destination += converted_surface->pitch;
        }
    } else {
        const std::uint32_t copy_size =
            static_cast<std::uint32_t>(
                converted_surface->format->bytes_per_pixel) *
            static_cast<std::uint32_t>(m_dwHeight) *
            static_cast<std::uint32_t>(m_dwWidth);
        std::memcpy(destination, source, copy_size);
    }
    sdl.unlock_surface(converted_surface);
    sdl.free_surface(converted_surface);
    sdl.free_surface(surface);

    HDC device_context = GetDC(nullptr);
    m_hbmBitmap = CreateBitmap(
        m_dwWidth,
        m_dwHeight,
        1,
        m_dwBPP,
        m_pRGBAData);
    if (m_hbmBitmap == nullptr) {
        std::printf(
            "Failed to create bitmap. Error code: %lu\n",
            GetLastError());
        delete[] m_pRGBAData;
        m_pRGBAData = nullptr;
        return E_FAIL;
    }

    ReleaseDC(nullptr, device_context);
    return S_OK;
}

/* 0x404A0, 6 bytes, global, 2 named locals
 * Texture::LoadTimFile
 * PDB type: HRESULT Texture::(char*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT Texture::LoadTimFile(char *path)
{
    (void)path;
    return E_FAIL;
}

/* 0x404B0, 48 bytes, global, 2 named locals
 * Texture::Restore
 * PDB type: HRESULT Texture::(CD3DFramework1...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */

/* 0x404E0, 3 bytes, global, 1 named locals
 * RestoreAllTextures
 * PDB type: HRESULT (CD3DFramework12*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
HRESULT Texture::Restore(CD3DFramework12 *framework)
{
    if ((m_hbmBitmap != nullptr || m_pRGBAData != nullptr) &&
        m_pTextureResource == nullptr) {
        if (framework->m_pDevice == nullptr) {
            return E_INVALIDARG;
        }
    }
    return S_OK;
}
HRESULT RestoreAllTextures(CD3DFramework12 *framework)
{
    (void)framework;
    return S_OK;
}

/* 0x404F0, 3 bytes, global, 2 named locals
 * Texture::SetDebugName
 * PDB type: void Texture::(const char*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
void Texture::SetDebugName(const char *name)
{
    (void)name;
}

/* 0x40500, 28 bytes, global, 1 named locals
 * SetTexturePath
 * PDB type: void (char*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
void SetTexturePath(char *path)
{
    lstrcpyA(g_strTexturePath, path != nullptr ? path : "");
}

const char *jpb_D3DTexturePathForTest()
{
    return g_strTexturePath;
}

/* 0x40520, 376 bytes, global, 15 named locals
 * UpdateSubresources
 * PDB type: unsigned __int64 (ID3D12Graphics...
 * Source: W:\SWJediPowerBattles\work\d3d\directx\d3dx12_resource_helpers.h
 */
std::uint64_t UpdateSubresources(
    ID3D12GraphicsCommandList *command_list,
    ID3D12Resource *destination_resource,
    ID3D12Resource *intermediate_resource,
    std::uint64_t intermediate_offset,
    UINT first_subresource,
    UINT subresource_count,
    D3D12_SUBRESOURCE_DATA *source_data)
{
    const D3D12_RESOURCE_DESC description =
        destination_resource->GetDesc();
    ID3D12Device *device = nullptr;
    const SIZE_T allocation_size =
        static_cast<SIZE_T>(subresource_count) *
        (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) +
         sizeof(UINT) + sizeof(std::uint64_t));
    void *allocation = HeapAlloc(
        GetProcessHeap(), 0, allocation_size);
    std::uint64_t result = 0;

    if (allocation != nullptr) {
        auto *layouts = static_cast<
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT *>(allocation);
        auto *row_sizes = reinterpret_cast<std::uint64_t *>(
            layouts + subresource_count);
        auto *row_counts = reinterpret_cast<UINT *>(
            row_sizes + subresource_count);
        std::uint64_t required_size = 0;

        destination_resource->GetDevice(
            IID_PPV_ARGS(&device));
        device->GetCopyableFootprints(
            &description,
            first_subresource,
            subresource_count,
            intermediate_offset,
            layouts,
            row_counts,
            row_sizes,
            &required_size);
        device->Release();

        result = UpdateSubresources(
            command_list,
            destination_resource,
            intermediate_resource,
            first_subresource,
            subresource_count,
            required_size,
            layouts,
            row_counts,
            row_sizes,
            source_data);
        HeapFree(GetProcessHeap(), 0, allocation);
    }
    return result;
}

/* 0x406A0, 511 bytes, global, 9 named locals
 * Texture::UpdateTexture
 * PDB type: void Texture::(void*, unsigned _...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */

/* 0x408A0, 116 bytes, global, 2 named locals
 * WaitForGpuTexture
 * PDB type: void (CD3DFramework12*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtextr.cpp
 */
void Texture::UpdateTexture(void *data, std::uint64_t size)
{
    (void)size;
    const D3D12_RESOURCE_DESC texture_description =
        m_pTextureResource->GetDesc();
    const std::uint64_t bytes_per_pixel =
        texture_description.Format == DXGI_FORMAT_R8G8B8A8_UNORM ? 4 : 0;
    CD3DFramework12 *framework = g_pD3DApp->m_pFramework;
    ID3D12Device *device = framework->m_pDevice;
    ID3D12GraphicsCommandList *command_list = framework->m_pCommandList;

    if (m_pUploadBuffer == nullptr) {
        const std::uint64_t required_size =
            GetRequiredIntermediateSize(m_pTextureResource.Get(), 0, 1);
        D3D12_HEAP_PROPERTIES heap_properties = {};
        D3D12_RESOURCE_DESC upload_description = {};

        heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
        heap_properties.CreationNodeMask = 1;
        heap_properties.VisibleNodeMask = 1;
        upload_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        upload_description.Width = required_size;
        upload_description.Height = 1;
        upload_description.DepthOrArraySize = 1;
        upload_description.MipLevels = 1;
        upload_description.SampleDesc.Count = 1;
        upload_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &upload_description,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_pUploadBuffer.ReleaseAndGetAddressOf()));
    }

    D3D12_SUBRESOURCE_DATA source = {};
    source.pData = data;
    source.RowPitch = static_cast<LONG_PTR>(
        bytes_per_pixel * texture_description.Width);
    source.SlicePitch = source.RowPitch * texture_description.Height;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_pTextureResource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    command_list->ResourceBarrier(1, &barrier);

    UpdateSubresources(
        command_list,
        m_pTextureResource.Get(),
        m_pUploadBuffer.Get(),
        0,
        0,
        1,
        &source);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    command_list->ResourceBarrier(1, &barrier);
}
void WaitForGpuTexture(CD3DFramework12 *framework)
{
    const UINT frame_index = framework->m_nFrameIndex;
    if (SUCCEEDED(framework->m_pCommandQueue->Signal(
            framework->m_pFence[frame_index],
            framework->m_nFenceValues[frame_index]))) {
        if (SUCCEEDED(framework->m_pFence[frame_index]->SetEventOnCompletion(
                framework->m_nFenceValues[frame_index],
                framework->m_hFenceEvent))) {
            WaitForSingleObject(framework->m_hFenceEvent, INFINITE);
            ++framework->m_nFenceValues[frame_index];
        }
    }
}

/* 0x40920, 17 bytes, global, 0 named locals
 * std::_String_val<std::_Simple_types<char> >::_Xran
 * PDB type: void std::_String_val<std::_Simp...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x26FBD0, 12 bytes, local, 1 named locals
 * `Texture::Texture'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FBE0, 19 bytes, local, 1 named locals
 * `Texture::Texture'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FC00, 19 bytes, local, 1 named locals
 * `Texture::Texture'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FC20, 19 bytes, local, 1 named locals
 * `Texture::Texture'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FC40, 12 bytes, local, 9 named locals
 * `Texture::CopyBitmapToSurface'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FC50, 12 bytes, local, 9 named locals
 * `Texture::CopyBitmapToSurface'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FC60, 12 bytes, local, 9 named locals
 * `Texture::CopyBitmapToSurface'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FC70, 12 bytes, local, 6 named locals
 * `Texture::CopyRGBADataToSurface'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FC80, 29 bytes, local, 2 named locals
 * `CreateEmptyTexture'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FCA0, 29 bytes, local, 0 named locals
 * `CreateNonTexturedTexture'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FCC0, 12 bytes, local, 7 named locals
 * `Texture::CreateSRVHeap'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FCD0, 12 bytes, local, 7 named locals
 * `Texture::CreateSRVHeap'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FCE0, 12 bytes, local, 7 named locals
 * `Texture::CreateSRVHeap'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FCF0, 29 bytes, local, 1 named locals
 * `CreateTextureFromFile'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FD10, 12 bytes, local, 5 named locals
 * `Texture::CreateTextureResource'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FD20, 12 bytes, local, 1 named locals
 * `Texture::LoadImageData'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */
