#include "jpb/d3dtextr.h"
#include "jpb/d3dapp.h"
#include "jpb/d3dframe.h"

#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

using Microsoft::WRL::ComPtr;

struct FakeResource {
    void **vtable;
    ULONG references;
    ULONG release_calls;
};

using CopyTextureRegionFunction = void (STDMETHODCALLTYPE *)(
    ID3D12GraphicsCommandList *,
    const D3D12_TEXTURE_COPY_LOCATION *,
    UINT,
    UINT,
    UINT,
    const D3D12_TEXTURE_COPY_LOCATION *,
    const D3D12_BOX *);

static CopyTextureRegionFunction original_copy_texture_region;
static ID3D12Resource *retained_copy_source;

static void STDMETHODCALLTYPE retain_copy_texture_region(
    ID3D12GraphicsCommandList *command_list,
    const D3D12_TEXTURE_COPY_LOCATION *destination,
    UINT destination_x,
    UINT destination_y,
    UINT destination_z,
    const D3D12_TEXTURE_COPY_LOCATION *source,
    const D3D12_BOX *source_box)
{
    retained_copy_source = source->pResource;
    retained_copy_source->AddRef();
    original_copy_texture_region(
        command_list,
        destination,
        destination_x,
        destination_y,
        destination_z,
        source,
        source_box);
}

static HRESULT STDMETHODCALLTYPE fake_query_interface(
    FakeResource *, REFIID, void **)
{
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE fake_add_ref(FakeResource *resource)
{
    return ++resource->references;
}

static ULONG STDMETHODCALLTYPE fake_release(FakeResource *resource)
{
    ++resource->release_calls;
    return --resource->references;
}

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "check failed: %s (%s:%d)\n", \
                     #condition, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

static D3D12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE type)
{
    D3D12_HEAP_PROPERTIES properties = {};

    properties.Type = type;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
}

static D3D12_RESOURCE_DESC buffer_description(std::uint64_t size)
{
    D3D12_RESOURCE_DESC description = {};

    description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    description.Width = size;
    description.Height = 1;
    description.DepthOrArraySize = 1;
    description.MipLevels = 1;
    description.SampleDesc.Count = 1;
    description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    return description;
}

static int test_upload_round_trip()
{
    const unsigned char pixels[] = {
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
        0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0xff
    };
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> readback;
    ComPtr<ID3D12Fence> fence;
    D3D12_RESOURCE_DESC texture_description = {};
    D3D12_COMMAND_QUEUE_DESC queue_description = {};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT readback_layout = {};
    D3D12_TEXTURE_COPY_LOCATION copy_source = {};
    D3D12_TEXTURE_COPY_LOCATION copy_destination = {};
    D3D12_RESOURCE_BARRIER barrier = {};
    UINT readback_rows = 0;
    std::uint64_t readback_row_size = 0;
    std::uint64_t readback_size = 0;
    HANDLE fence_event;
    void *mapped = nullptr;
    auto *texture_storage = static_cast<unsigned char *>(
        ::operator new(sizeof(Texture)));
    auto *framework_storage = static_cast<unsigned char *>(
        ::operator new(sizeof(CD3DFramework12)));
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto &application = *reinterpret_cast<CD3DApplication *>(
        application_storage.data());

    CHECK(SUCCEEDED(D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(device.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(allocator.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        allocator.Get(),
        nullptr,
        IID_PPV_ARGS(command_list.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandQueue(
        &queue_description,
        IID_PPV_ARGS(queue.GetAddressOf()))));

    texture_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture_description.Width = 2;
    texture_description.Height = 2;
    texture_description.DepthOrArraySize = 1;
    texture_description.MipLevels = 1;
    texture_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_description.SampleDesc.Count = 1;
    texture_description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    D3D12_HEAP_PROPERTIES default_heap =
        heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    CHECK(SUCCEEDED(device->CreateCommittedResource(
        &default_heap,
        D3D12_HEAP_FLAG_NONE,
        &texture_description,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(texture.GetAddressOf()))));

    std::memset(texture_storage, 0xcd, sizeof(Texture));
    std::memset(framework_storage, 0xcd, sizeof(CD3DFramework12));
    auto *concrete_texture = new (texture_storage) Texture(nullptr, 0, 0);
    auto *framework = new (framework_storage) CD3DFramework12;
    concrete_texture->m_pTextureResource = texture;
    framework->m_pDevice = device.Get();
    framework->m_pCommandList = command_list.Get();
    application.m_pFramework = framework;
    g_pD3DApp = &application;
    concrete_texture->UpdateTexture(
        const_cast<unsigned char *>(pixels), sizeof(pixels));
    CHECK(concrete_texture->m_pUploadBuffer != nullptr);
    CHECK(GetRequiredIntermediateSize(texture.Get(), 0, 1) > sizeof(pixels));

    device->GetCopyableFootprints(
        &texture_description,
        0,
        1,
        0,
        &readback_layout,
        &readback_rows,
        &readback_row_size,
        &readback_size);
    D3D12_RESOURCE_DESC readback_description =
        buffer_description(readback_size);
    D3D12_HEAP_PROPERTIES readback_heap =
        heap_properties(D3D12_HEAP_TYPE_READBACK);
    CHECK(SUCCEEDED(device->CreateCommittedResource(
        &readback_heap,
        D3D12_HEAP_FLAG_NONE,
        &readback_description,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(readback.GetAddressOf()))));

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    command_list->ResourceBarrier(1, &barrier);

    copy_source.pResource = texture.Get();
    copy_source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    copy_source.SubresourceIndex = 0;
    copy_destination.pResource = readback.Get();
    copy_destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    copy_destination.PlacedFootprint = readback_layout;
    command_list->CopyTextureRegion(
        &copy_destination, 0, 0, 0, &copy_source, nullptr);
    CHECK(SUCCEEDED(command_list->Close()));

    ID3D12CommandList *command_lists[] = {command_list.Get()};
    queue->ExecuteCommandLists(1, command_lists);
    CHECK(SUCCEEDED(device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(fence.GetAddressOf()))));
    fence_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    CHECK(fence_event != nullptr);
    CHECK(SUCCEEDED(queue->Signal(fence.Get(), 1)));
    CHECK(SUCCEEDED(fence->SetEventOnCompletion(1, fence_event)));
    CHECK(WaitForSingleObject(fence_event, INFINITE) == WAIT_OBJECT_0);
    CloseHandle(fence_event);

    CHECK(SUCCEEDED(readback->Map(0, nullptr, &mapped)));
    CHECK(std::memcmp(mapped, pixels, 8) == 0);
    CHECK(std::memcmp(
              static_cast<unsigned char *>(mapped) +
                  readback_layout.Footprint.RowPitch,
              pixels + 8,
              8) == 0);
    readback->Unmap(0, nullptr);
    g_pD3DApp = nullptr;
    ::operator delete(framework_storage);
    return 0;
}

static int test_copy_rgba_data_to_surface()
{
    constexpr UINT width = 64;
    constexpr UINT height = 2;
    constexpr std::size_t pixel_bytes = width * height * 4;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12Resource> initial_texture;
    ComPtr<ID3D12Resource> readback;
    ComPtr<ID3D12Fence> fence;
    D3D12_COMMAND_QUEUE_DESC queue_description = {};
    D3D12_RESOURCE_DESC initial_description = {};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT readback_layout = {};
    UINT readback_rows = 0;
    std::uint64_t readback_row_size = 0;
    std::uint64_t readback_size = 0;
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto &application = *reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    void *mapped = nullptr;
    void **original_command_vtable = nullptr;
    void *command_vtable[64] = {};

    CHECK(SUCCEEDED(D3D12CreateDevice(
        nullptr, D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(device.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(allocator.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
        IID_PPV_ARGS(command_list.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandQueue(
        &queue_description, IID_PPV_ARGS(queue.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()))));

    initial_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    initial_description.Width = 1;
    initial_description.Height = 1;
    initial_description.DepthOrArraySize = 1;
    initial_description.MipLevels = 1;
    initial_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    initial_description.SampleDesc.Count = 1;
    D3D12_HEAP_PROPERTIES default_heap =
        heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    CHECK(SUCCEEDED(device->CreateCommittedResource(
        &default_heap, D3D12_HEAP_FLAG_NONE, &initial_description,
        D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(initial_texture.GetAddressOf()))));

    char name[] = "RGBA_COPY";
    auto *texture = new Texture(name, 0, 0);
    texture->m_dwWidth = width;
    texture->m_dwHeight = height;
    texture->m_pRGBAData = static_cast<unsigned char *>(
        ::operator new(pixel_bytes));
    for (std::size_t index = 0; index < pixel_bytes; ++index) {
        texture->m_pRGBAData[index] = static_cast<unsigned char>(index);
    }
    CHECK(texture->CopyRGBADataToSurface(
        device.Get(), command_list.Get()) == E_FAIL);
    texture->m_pTextureResource = initial_texture;
    original_command_vtable =
        *reinterpret_cast<void ***>(command_list.Get());
    std::memcpy(
        command_vtable,
        original_command_vtable,
        sizeof(command_vtable));
    original_copy_texture_region =
        reinterpret_cast<CopyTextureRegionFunction>(command_vtable[16]);
    command_vtable[16] =
        reinterpret_cast<void *>(&retain_copy_texture_region);
    *reinterpret_cast<void ***>(command_list.Get()) = command_vtable;
    CHECK(SUCCEEDED(texture->CopyRGBADataToSurface(
        device.Get(), command_list.Get())));
    *reinterpret_cast<void ***>(command_list.Get()) = original_command_vtable;
    CHECK(retained_copy_source != nullptr);

    const D3D12_RESOURCE_DESC result_description =
        texture->m_pTextureResource->GetDesc();
    CHECK(result_description.Width == 256);
    CHECK(result_description.Height == 256);
    CHECK(result_description.Format == DXGI_FORMAT_R8G8B8A8_UNORM);

    device->GetCopyableFootprints(
        &result_description, 0, 1, 0, &readback_layout,
        &readback_rows, &readback_row_size, &readback_size);
    D3D12_RESOURCE_DESC readback_description =
        buffer_description(readback_size);
    D3D12_HEAP_PROPERTIES readback_heap =
        heap_properties(D3D12_HEAP_TYPE_READBACK);
    CHECK(SUCCEEDED(device->CreateCommittedResource(
        &readback_heap, D3D12_HEAP_FLAG_NONE, &readback_description,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(readback.GetAddressOf()))));

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture->m_pTextureResource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    command_list->ResourceBarrier(1, &barrier);

    D3D12_TEXTURE_COPY_LOCATION copy_source = {};
    copy_source.pResource = texture->m_pTextureResource.Get();
    copy_source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION copy_destination = {};
    copy_destination.pResource = readback.Get();
    copy_destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    copy_destination.PlacedFootprint = readback_layout;
    command_list->CopyTextureRegion(
        &copy_destination, 0, 0, 0, &copy_source, nullptr);
    CHECK(SUCCEEDED(command_list->Close()));

    ID3D12CommandList *command_lists[] = {command_list.Get()};
    queue->ExecuteCommandLists(1, command_lists);
    HANDLE fence_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    CHECK(fence_event != nullptr);
    CHECK(SUCCEEDED(queue->Signal(fence.Get(), 1)));
    CHECK(SUCCEEDED(fence->SetEventOnCompletion(1, fence_event)));
    CHECK(WaitForSingleObject(fence_event, INFINITE) == WAIT_OBJECT_0);
    CloseHandle(fence_event);

    CHECK(SUCCEEDED(readback->Map(0, nullptr, &mapped)));
    CHECK(std::memcmp(mapped, texture->m_pRGBAData, width * 4) == 0);
    CHECK(std::memcmp(
        static_cast<unsigned char *>(mapped) +
            readback_layout.Footprint.RowPitch,
        texture->m_pRGBAData + width * 4,
        width * 4) == 0);
    readback->Unmap(0, nullptr);
    retained_copy_source->Release();
    retained_copy_source = nullptr;

    g_pD3DApp = &application;
    delete texture;
    g_pD3DApp = nullptr;
    return 0;
}

static int test_copy_bitmap_to_surface()
{
    constexpr UINT width = 64;
    constexpr UINT height = 2;
    constexpr UINT row_bytes = width * 4;
    constexpr UINT bitmap_bytes = row_bytes * height;
    unsigned char pixels[bitmap_bytes];
    unsigned char bitmap_data[width * height * 32] = {};
    unsigned char zero_row[row_bytes] = {};
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12Resource> destination_texture;
    ComPtr<ID3D12Resource> readback;
    ComPtr<ID3D12Fence> fence;
    D3D12_COMMAND_QUEUE_DESC queue_description = {};
    D3D12_RESOURCE_DESC texture_description = {};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT readback_layout = {};
    UINT readback_rows = 0;
    std::uint64_t readback_row_size = 0;
    std::uint64_t readback_size = 0;
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto &application = *reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    void *mapped = nullptr;
    void **original_command_vtable = nullptr;
    void *command_vtable[64] = {};

    for (UINT index = 0; index < bitmap_bytes; ++index) {
        pixels[index] = static_cast<unsigned char>(index * 3 + 1);
    }

    CHECK(SUCCEEDED(D3D12CreateDevice(
        nullptr, D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(device.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(allocator.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
        IID_PPV_ARGS(command_list.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandQueue(
        &queue_description, IID_PPV_ARGS(queue.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()))));

    texture_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture_description.Width = width;
    texture_description.Height = height;
    texture_description.DepthOrArraySize = 1;
    texture_description.MipLevels = 1;
    texture_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_description.SampleDesc.Count = 1;
    D3D12_HEAP_PROPERTIES default_heap =
        heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    CHECK(SUCCEEDED(device->CreateCommittedResource(
        &default_heap, D3D12_HEAP_FLAG_NONE, &texture_description,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(destination_texture.GetAddressOf()))));

    char name[] = "BITMAP_COPY";
    auto *texture = new Texture(name, 0, 0);
    CHECK(texture->CopyBitmapToSurface(
        device.Get(), command_list.Get()) == E_INVALIDARG);
    texture->m_hbmBitmap = CreateBitmap(width, height, 1, 32, pixels);
    CHECK(texture->m_hbmBitmap != nullptr);
    CHECK(texture->CopyBitmapToSurface(
        nullptr, command_list.Get()) == E_INVALIDARG);
    CHECK(GetBitmapBits(
        texture->m_hbmBitmap, sizeof(bitmap_data), bitmap_data) ==
        bitmap_bytes);
    texture->m_dwPitch = bitmap_bytes;
    texture->m_pTextureResource = destination_texture;

    original_command_vtable =
        *reinterpret_cast<void ***>(command_list.Get());
    std::memcpy(
        command_vtable,
        original_command_vtable,
        sizeof(command_vtable));
    original_copy_texture_region =
        reinterpret_cast<CopyTextureRegionFunction>(command_vtable[16]);
    command_vtable[16] =
        reinterpret_cast<void *>(&retain_copy_texture_region);
    retained_copy_source = nullptr;
    *reinterpret_cast<void ***>(command_list.Get()) = command_vtable;
    const HRESULT copy_result = texture->CopyBitmapToSurface(
        device.Get(), command_list.Get());
    *reinterpret_cast<void ***>(command_list.Get()) = original_command_vtable;
    CHECK(copy_result == S_OK);
    CHECK(retained_copy_source != nullptr);

    device->GetCopyableFootprints(
        &texture_description, 0, 1, 0, &readback_layout,
        &readback_rows, &readback_row_size, &readback_size);
    D3D12_RESOURCE_DESC readback_description =
        buffer_description(readback_size);
    D3D12_HEAP_PROPERTIES readback_heap =
        heap_properties(D3D12_HEAP_TYPE_READBACK);
    CHECK(SUCCEEDED(device->CreateCommittedResource(
        &readback_heap, D3D12_HEAP_FLAG_NONE, &readback_description,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(readback.GetAddressOf()))));

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = destination_texture.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    command_list->ResourceBarrier(1, &barrier);

    D3D12_TEXTURE_COPY_LOCATION copy_source = {};
    copy_source.pResource = destination_texture.Get();
    copy_source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION copy_destination = {};
    copy_destination.pResource = readback.Get();
    copy_destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    copy_destination.PlacedFootprint = readback_layout;
    command_list->CopyTextureRegion(
        &copy_destination, 0, 0, 0, &copy_source, nullptr);
    CHECK(SUCCEEDED(command_list->Close()));

    ID3D12CommandList *command_lists[] = {command_list.Get()};
    queue->ExecuteCommandLists(1, command_lists);
    HANDLE fence_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    CHECK(fence_event != nullptr);
    CHECK(SUCCEEDED(queue->Signal(fence.Get(), 1)));
    CHECK(SUCCEEDED(fence->SetEventOnCompletion(1, fence_event)));
    CHECK(WaitForSingleObject(fence_event, INFINITE) == WAIT_OBJECT_0);
    CloseHandle(fence_event);

    CHECK(SUCCEEDED(readback->Map(0, nullptr, &mapped)));
    CHECK(std::memcmp(mapped, bitmap_data, row_bytes) == 0);
    CHECK(std::memcmp(
        static_cast<unsigned char *>(mapped) +
            readback_layout.Footprint.RowPitch,
        zero_row,
        row_bytes) == 0);
    readback->Unmap(0, nullptr);
    retained_copy_source->Release();
    retained_copy_source = nullptr;

    g_pD3DApp = &application;
    delete texture;
    g_pD3DApp = nullptr;
    return 0;
}

static int test_load_image_data()
{
    char no_extension_name[] = "missing_image";
    auto *no_extension = new Texture(no_extension_name, 0, 0);
    CHECK(no_extension->LoadImageData(0) == E_NOTIMPL);
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto &application = *reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    g_pD3DApp = &application;
    delete no_extension;
    g_pD3DApp = nullptr;

#ifdef JPB_D3DTEXTR_REAL_ASSET_DIR
    CHECK(SetDllDirectoryA(JPB_D3DTEXTR_REAL_ASSET_DIR) != FALSE);
    char missing_name[] =
        JPB_D3DTEXTR_REAL_ASSET_DIR "/res/default/missing.png";
    auto *missing = new Texture(missing_name, 0, 0);
    CHECK(missing->LoadImageData(0) == E_NOTIMPL);

    char image_name[] =
        JPB_D3DTEXTR_REAL_ASSET_DIR "/res/default/a_blob.png";
    auto *ordinary = new Texture(image_name, 0, 0);
    auto *flipped = new Texture(image_name, 0, 0);
    CHECK(ordinary->LoadImageData(0) == S_OK);
    CHECK(flipped->LoadImageData(1) == S_OK);
    CHECK(SetDllDirectoryA(nullptr) != FALSE);

    CHECK(ordinary->m_dwWidth > 0);
    CHECK(ordinary->m_dwHeight > 1);
    CHECK(ordinary->m_dwWidth == flipped->m_dwWidth);
    CHECK(ordinary->m_dwHeight == flipped->m_dwHeight);
    CHECK(ordinary->m_dwBPP == 32);
    CHECK(flipped->m_dwBPP == 32);
    CHECK(ordinary->m_isTransparent);
    CHECK(flipped->m_isTransparent);
    CHECK(ordinary->m_pRGBAData != nullptr);
    CHECK(flipped->m_pRGBAData != nullptr);
    CHECK(ordinary->m_hbmBitmap != nullptr);
    CHECK(flipped->m_hbmBitmap != nullptr);

    const std::size_t row_size =
        static_cast<std::size_t>(ordinary->m_dwWidth) * 4;
    for (DWORD row = 0; row < ordinary->m_dwHeight; ++row) {
        CHECK(std::memcmp(
            ordinary->m_pRGBAData + row * row_size,
            flipped->m_pRGBAData +
                (ordinary->m_dwHeight - row - 1) * row_size,
            row_size) == 0);
    }

    g_pD3DApp = &application;
    delete flipped;
    delete ordinary;
    delete missing;
    g_pD3DApp = nullptr;
#endif
    return 0;
}

static int check_vertical_image_pair(
    const Texture *ordinary,
    const Texture *flipped)
{
    CHECK(ordinary->m_dwWidth > 0);
    CHECK(ordinary->m_dwHeight > 1);
    CHECK(ordinary->m_dwWidth == flipped->m_dwWidth);
    CHECK(ordinary->m_dwHeight == flipped->m_dwHeight);
    CHECK(ordinary->m_dwBPP == 32);
    CHECK(flipped->m_dwBPP == 32);
    CHECK(ordinary->m_pRGBAData != nullptr);
    CHECK(flipped->m_pRGBAData != nullptr);
    CHECK(ordinary->m_hbmBitmap != nullptr);
    CHECK(flipped->m_hbmBitmap != nullptr);

    const std::size_t row_size =
        static_cast<std::size_t>(ordinary->m_dwWidth) * 4;
    CHECK(std::memcmp(
        ordinary->m_pRGBAData,
        ordinary->m_pRGBAData +
            (ordinary->m_dwHeight - 1) * row_size,
        row_size) != 0);
    for (DWORD row = 0; row < ordinary->m_dwHeight; ++row) {
        CHECK(std::memcmp(
            ordinary->m_pRGBAData + row * row_size,
            flipped->m_pRGBAData +
                (ordinary->m_dwHeight - row - 1) * row_size,
            row_size) == 0);
    }
    return 0;
}

static int test_specialized_image_loaders()
{
#ifdef JPB_D3DTEXTR_REAL_ASSET_DIR
    CHECK(SetDllDirectoryA(JPB_D3DTEXTR_REAL_ASSET_DIR) != FALSE);
    char missing_png_name[] =
        JPB_D3DTEXTR_REAL_ASSET_DIR "/res/default/missing.png";
    auto *missing_png = new Texture(missing_png_name, 0, 0);
    CHECK(missing_png->LoadPNGFile(0) == E_NOTIMPL);

    char png_name[] =
        JPB_D3DTEXTR_REAL_ASSET_DIR "/res/default/a_blob.png";
    auto *png = new Texture(png_name, 0, 0);
    auto *flipped_png = new Texture(png_name, 0, 0);
    CHECK(png->LoadPNGFile(0) == S_OK);
    CHECK(flipped_png->LoadPNGFile(1) == S_OK);
    CHECK(check_vertical_image_pair(png, flipped_png) == 0);
    CHECK(!png->m_isTransparent);
    CHECK(!flipped_png->m_isTransparent);

    char targa_name[] = "TARGA";
    char targa_path[] =
        JPB_D3DTEXTR_REAL_ASSET_DIR "/res/default/a_blob.tga";
    auto *targa = new Texture(targa_name, 0, 0);
    auto *flipped_targa = new Texture(targa_name, 0, 0);
    CHECK(targa->LoadTargaFile(targa_path, 0) == S_OK);
    CHECK(flipped_targa->LoadTargaFile(targa_path, 1) == S_OK);
    CHECK(check_vertical_image_pair(targa, flipped_targa) == 0);
    CHECK(targa->m_isTransparent);
    CHECK(flipped_targa->m_isTransparent);
    CHECK(targa->m_dwPitch == targa->m_dwWidth * 4);
    CHECK(flipped_targa->m_dwPitch == flipped_targa->m_dwWidth * 4);
    CHECK(SetDllDirectoryA(nullptr) != FALSE);

    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto &application = *reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    g_pD3DApp = &application;
    delete flipped_targa;
    delete targa;
    delete flipped_png;
    delete png;
    delete missing_png;
    g_pD3DApp = nullptr;
#endif
    return 0;
}

static int test_texture_constructor_and_restore()
{
    auto *first_storage = static_cast<unsigned char *>(
        ::operator new(sizeof(Texture)));
    auto *second_storage = static_cast<unsigned char *>(
        ::operator new(sizeof(Texture)));
    auto *framework_storage = static_cast<unsigned char *>(
        ::operator new(sizeof(CD3DFramework12)));
    std::memset(first_storage, 0xcd, sizeof(Texture));
    std::memset(second_storage, 0xcd, sizeof(Texture));
    std::memset(framework_storage, 0xcd, sizeof(CD3DFramework12));

    auto *first = new (first_storage) Texture(nullptr, 7, 0x1234);
    CHECK(std::strcmp(first->m_strName, "UNASSIGNED!") == 0);
    CHECK(first->GetWidth() == 0);
    CHECK(first->GetHeight() == 0);
    CHECK(first->m_dwWidth == 0);
    CHECK(first->m_dwHeight == 0);
    CHECK(first->m_dwStage == 7);
    CHECK(first->m_dwBPP == 0);
    CHECK(first->m_dwFlags == 0x1234);
    CHECK(first->m_bHasAlpha == FALSE);
    CHECK(first->m_dwPitch == 0xcdcdcdcd);
    CHECK(static_cast<unsigned>(first->m_nIndex) == 0xcdcdcdcd);
    CHECK(static_cast<unsigned>(first->m_tpfDesired) == 0xcdcdcdcd);
    CHECK(first->next == nullptr);
    CHECK(first->prev == nullptr);
    CHECK(first->m_pTextureResource == nullptr);
    CHECK(first->m_pSRVHeap == nullptr);
    CHECK(first->m_pUploadBuffer == nullptr);
    CHECK(first->m_hbmBitmap == nullptr);
    CHECK(first->m_pRGBAData == nullptr);
    CHECK(first->m_pSDLTexture == nullptr);
    CHECK(first->m_pDescriptor == nullptr);
    CHECK(!first->m_isTransparent);

    char assigned_name[] = "TEST_TEXTURE";
    auto *second = new (second_storage) Texture(assigned_name, 3, 9);
    CHECK(std::strcmp(second->m_strName, assigned_name) == 0);
    CHECK(second->next == first);
    CHECK(second->prev == nullptr);
    CHECK(first->prev == second);

    auto *framework = new (framework_storage) CD3DFramework12;
    second->m_pRGBAData = reinterpret_cast<unsigned char *>(1);
    CHECK(second->Restore(framework) == E_INVALIDARG);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(1);
    CHECK(second->Restore(framework) == S_OK);
    second->m_pRGBAData = nullptr;

    ::operator delete(framework_storage);
    return 0;
}

static int test_legacy_texture_factories_and_delete_all()
{
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto &application = *reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    char name[] = "EMPTY";
    void *resource_vtable[3] = {
        reinterpret_cast<void *>(&fake_query_interface),
        reinterpret_cast<void *>(&fake_add_ref),
        reinterpret_cast<void *>(&fake_release)
    };
    FakeResource resource = {resource_vtable, 2, 0};

    g_pD3DApp = &application;
    Texture *empty = CreateEmptyTexture(name, 320, 240, 7, 0x1234);
    CHECK(empty != nullptr);
    CHECK(std::strcmp(empty->m_strName, name) == 0);
    CHECK(empty->GetWidth() == 0);
    CHECK(empty->GetHeight() == 0);
    CHECK(empty->m_dwWidth == 0);
    CHECK(empty->m_dwHeight == 0);
    CHECK(empty->m_dwStage == 7);
    CHECK(empty->m_dwBPP == 0);
    CHECK(empty->m_dwFlags == 0x1234);

    Texture *non_textured = CreateNonTexturedTexture(37);
    CHECK(non_textured != nullptr);
    unsigned type = 0;
    std::memcpy(
        &type,
        reinterpret_cast<unsigned char *>(non_textured) + 8,
        sizeof(type));
    CHECK(type == 37);
    CHECK(non_textured->m_dwWidth == 0);
    CHECK(non_textured->m_dwHeight == 0);
    CHECK(non_textured->m_dwStage == 0);
    CHECK(non_textured->m_hbmBitmap == nullptr);
    CHECK(non_textured->m_pRGBAData == nullptr);
    CHECK(non_textured->m_nIndex == -1);
    CHECK(non_textured->next == empty);
    CHECK(empty->prev == non_textured);

    non_textured->m_pTextureResource.Attach(
        reinterpret_cast<ID3D12Resource *>(&resource));
    DeleteAllTextures();
    CHECK(resource.release_calls == 2);
    CHECK(resource.references == 0);

    Texture *after_delete = CreateNonTexturedTexture(9);
    CHECK(after_delete != nullptr);
    CHECK(after_delete->next == nullptr);
    CHECK(after_delete->prev == nullptr);
    delete after_delete;
    g_pD3DApp = nullptr;
    return 0;
}

static int test_texture_factory_resource_path()
{
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12DescriptorHeap> descriptor_heap;
    ComPtr<ID3D12Fence> fence;
    D3D12_COMMAND_QUEUE_DESC queue_description = {};
    D3D12_DESCRIPTOR_HEAP_DESC heap_description = {};
    auto *framework_storage = static_cast<unsigned char *>(
        ::operator new(sizeof(CD3DFramework12)));
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto &application = *reinterpret_cast<CD3DApplication *>(
        application_storage.data());

    CHECK(SUCCEEDED(D3D12CreateDevice(
        nullptr, D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(device.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(allocator.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
        IID_PPV_ARGS(command_list.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandQueue(
        &queue_description, IID_PPV_ARGS(queue.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()))));

    heap_description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_description.NumDescriptors = 4;
    CHECK(SUCCEEDED(device->CreateDescriptorHeap(
        &heap_description, IID_PPV_ARGS(descriptor_heap.GetAddressOf()))));

    std::memset(framework_storage, 0xcd, sizeof(CD3DFramework12));
    auto *framework = new (framework_storage) CD3DFramework12;
    framework->m_pDevice = device.Get();
    framework->m_pCommandQueue = queue.Get();
    framework->m_pCommandAllocators[0] = allocator.Get();
    framework->m_pCommandList = command_list.Get();
    framework->m_pFence[0] = fence.Get();
    framework->m_nFenceValues[0] = 1;
    framework->m_nFrameIndex = 0;
    framework->m_pPipelineState = nullptr;
    framework->m_pMainDescriptorHeap = descriptor_heap.Get();
    framework->m_nDescriptorCount = 0;
    framework->m_commandListOpen = true;
    framework->m_hFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CHECK(framework->m_hFenceEvent != nullptr);
    application.m_pFramework = framework;
    g_pD3DApp = &application;

    PHL::Texture2D *texture_base = PHL::Texture2D::CreateTexture(
        2, 2, PHL::TextureFormat::RGBA8888);
    CHECK(texture_base != nullptr);
    auto *texture = static_cast<Texture *>(texture_base);
    CHECK(texture->GetWidth() == 2);
    CHECK(texture->GetHeight() == 2);
    CHECK(texture->m_dwWidth == 2);
    CHECK(texture->m_dwHeight == 2);
    CHECK(texture->m_dwBPP == 32);
    CHECK(texture->m_bHasAlpha == TRUE);
    CHECK(texture->m_dwPitch == 8);
    CHECK(texture->m_pTextureResource != nullptr);
    CHECK(texture->m_pDescriptor != nullptr);
    CHECK(texture->m_nIndex == 0);
    CHECK(framework->m_nDescriptorCount == 1);
    CHECK(framework->IsCommandListOpen());

    const D3D12_RESOURCE_DESC resource_description =
        texture->m_pTextureResource->GetDesc();
    CHECK(resource_description.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);
    CHECK(resource_description.Width == 2);
    CHECK(resource_description.Height == 2);
    CHECK(resource_description.Format == DXGI_FORMAT_R8G8B8A8_UNORM);

    const std::uint64_t descriptor =
        reinterpret_cast<std::uint64_t>(texture->m_pDescriptor);
    delete texture_base;
    CHECK(framework->AllocateShaderResourceDescriptor() == descriptor);
    CHECK(framework->TryCloseCommandList() == S_OK);

    const std::size_t capacity = static_cast<std::size_t>(
        framework->m_freeShaderResourceDescriptors.capacity_end -
        framework->m_freeShaderResourceDescriptors.begin);
    ::operator delete(framework->m_freeShaderResourceDescriptors.begin,
                      capacity * sizeof(std::uint64_t));
    CloseHandle(framework->m_hFenceEvent);
    g_pD3DApp = nullptr;
    ::operator delete(framework_storage);
    return 0;
}

static int test_srv_heap_and_file_factory_path()
{
    constexpr UINT width = 64;
    constexpr UINT height = 2;
    constexpr std::size_t pixel_bytes = width * height * 4;
    unsigned char pixels[pixel_bytes];
    unsigned char bitmap_data[pixel_bytes] = {};
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> command_list;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12DescriptorHeap> descriptor_heap;
    ComPtr<ID3D12Resource> readback;
    ComPtr<ID3D12Fence> fence;
    D3D12_COMMAND_QUEUE_DESC queue_description = {};
    D3D12_DESCRIPTOR_HEAP_DESC heap_description = {};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT readback_layout = {};
    UINT readback_rows = 0;
    std::uint64_t readback_row_size = 0;
    std::uint64_t readback_size = 0;
    auto *framework_storage = static_cast<unsigned char *>(
        ::operator new(sizeof(CD3DFramework12)));
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto &application = *reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    void *mapped = nullptr;

    for (std::size_t index = 0; index < pixel_bytes; ++index) {
        pixels[index] = static_cast<unsigned char>(index * 5 + 3);
    }

    CHECK(SUCCEEDED(D3D12CreateDevice(
        nullptr, D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(device.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(allocator.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
        IID_PPV_ARGS(command_list.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateCommandQueue(
        &queue_description, IID_PPV_ARGS(queue.GetAddressOf()))));
    CHECK(SUCCEEDED(device->CreateFence(
        0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()))));

    heap_description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_description.NumDescriptors = 4;
    CHECK(SUCCEEDED(device->CreateDescriptorHeap(
        &heap_description, IID_PPV_ARGS(descriptor_heap.GetAddressOf()))));

    std::memset(framework_storage, 0xcd, sizeof(CD3DFramework12));
    auto *framework = new (framework_storage) CD3DFramework12;
    framework->m_pDevice = device.Get();
    framework->m_pCommandQueue = queue.Get();
    framework->m_pCommandAllocators[0] = allocator.Get();
    framework->m_pCommandList = command_list.Get();
    framework->m_pFence[0] = fence.Get();
    framework->m_nFenceValues[0] = 1;
    framework->m_nFrameIndex = 0;
    framework->m_pPipelineState = nullptr;
    framework->m_pMainDescriptorHeap = descriptor_heap.Get();
    framework->m_nDescriptorCount = 0;
    framework->m_commandListOpen = true;
    framework->m_hFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CHECK(framework->m_hFenceEvent != nullptr);
    application.m_pFramework = framework;
    g_pD3DApp = &application;

    char texture_name[] = "C:/retail/res/test.bmp";
    auto *texture = new Texture(texture_name, 0, 0);
    texture->m_dwWidth = width;
    texture->m_dwHeight = height;
    texture->m_dwBPP = 32;
    texture->m_hbmBitmap = CreateBitmap(width, height, 1, 32, pixels);
    CHECK(texture->m_hbmBitmap != nullptr);
    CHECK(GetBitmapBits(
        texture->m_hbmBitmap,
        sizeof(bitmap_data),
        bitmap_data) == sizeof(bitmap_data));
    CHECK(texture->CreateSRVHeap(framework) == S_OK);
    CHECK(texture->m_pTextureResource != nullptr);
    CHECK(texture->m_pDescriptor != nullptr);
    CHECK(texture->m_nIndex == 0);
    CHECK(framework->m_nDescriptorCount == 1);
    CHECK(framework->IsCommandListOpen());

    const D3D12_RESOURCE_DESC texture_description =
        texture->m_pTextureResource->GetDesc();
    CHECK(texture_description.Width == width);
    CHECK(texture_description.Height == height);
    CHECK(texture_description.Format == DXGI_FORMAT_R8G8B8A8_UNORM);
    char debug_name[64] = {};
    UINT debug_name_size = sizeof(debug_name);
    CHECK(SUCCEEDED(texture->m_pTextureResource->GetPrivateData(
        WKPDID_D3DDebugObjectName, &debug_name_size, debug_name)));
    CHECK(debug_name_size == std::strlen("res/test.bmp"));
    CHECK(std::memcmp(
        debug_name, "res/test.bmp", debug_name_size) == 0);

    device->GetCopyableFootprints(
        &texture_description, 0, 1, 0, &readback_layout,
        &readback_rows, &readback_row_size, &readback_size);
    D3D12_RESOURCE_DESC readback_description =
        buffer_description(readback_size);
    D3D12_HEAP_PROPERTIES readback_heap =
        heap_properties(D3D12_HEAP_TYPE_READBACK);
    CHECK(SUCCEEDED(device->CreateCommittedResource(
        &readback_heap, D3D12_HEAP_FLAG_NONE, &readback_description,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(readback.GetAddressOf()))));

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture->m_pTextureResource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    command_list->ResourceBarrier(1, &barrier);
    D3D12_TEXTURE_COPY_LOCATION copy_source = {};
    copy_source.pResource = texture->m_pTextureResource.Get();
    copy_source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION copy_destination = {};
    copy_destination.pResource = readback.Get();
    copy_destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    copy_destination.PlacedFootprint = readback_layout;
    command_list->CopyTextureRegion(
        &copy_destination, 0, 0, 0, &copy_source, nullptr);
    CHECK(framework->TryCloseCommandList() == S_OK);
    ID3D12CommandList *command_lists[] = {command_list.Get()};
    queue->ExecuteCommandLists(1, command_lists);
    const std::uint64_t readback_fence_value =
        framework->m_nFenceValues[0];
    CHECK(SUCCEEDED(queue->Signal(fence.Get(), readback_fence_value)));
    CHECK(SUCCEEDED(fence->SetEventOnCompletion(
        readback_fence_value, framework->m_hFenceEvent)));
    CHECK(WaitForSingleObject(
        framework->m_hFenceEvent, INFINITE) == WAIT_OBJECT_0);
    ++framework->m_nFenceValues[0];

    CHECK(SUCCEEDED(readback->Map(0, nullptr, &mapped)));
    CHECK(std::memcmp(mapped, bitmap_data, width * 4) == 0);
    CHECK(std::memcmp(
        static_cast<unsigned char *>(mapped) +
            readback_layout.Footprint.RowPitch,
        bitmap_data + width * 4,
        width * 4) == 0);
    readback->Unmap(0, nullptr);
    delete texture;

    void *resource_vtable[3] = {
        reinterpret_cast<void *>(&fake_query_interface),
        reinterpret_cast<void *>(&fake_add_ref),
        reinterpret_cast<void *>(&fake_release)
    };
    FakeResource pain = {resource_vtable, 1, 0};
    FakeResource pain_texture = {resource_vtable, 1, 0};
    framework->PAIN = reinterpret_cast<ID3D12Resource *>(&pain);
    framework->PAINTEX =
        reinterpret_cast<ID3D12Resource *>(&pain_texture);
    CHECK(CreateTextureFromFile(
        nullptr, 0, 0, 0, 0, 0, framework) == nullptr);
    CHECK(pain.release_calls == 1);
    CHECK(pain_texture.release_calls == 1);
    CHECK(framework->PAIN == reinterpret_cast<ID3D12Resource *>(&pain));
    CHECK(framework->PAINTEX ==
          reinterpret_cast<ID3D12Resource *>(&pain_texture));
    framework->PAIN = nullptr;
    framework->PAINTEX = nullptr;

    char missing_name[] = "missing_texture";
    Texture *fallback = CreateTextureFromFile(
        missing_name, 7, 0x1234, 0, 19, 37, framework);
    CHECK(fallback != nullptr);
    CHECK(fallback->m_dwWidth == 256);
    CHECK(fallback->m_dwHeight == 256);
    CHECK(fallback->m_dwBPP == 32);
    CHECK(fallback->m_dwStage == 7);
    CHECK(fallback->m_dwFlags == 0x1234);
    CHECK(fallback->m_tpfDesired == 19);
    unsigned type = 0;
    std::memcpy(
        &type,
        reinterpret_cast<unsigned char *>(fallback) + 8,
        sizeof(type));
    CHECK(type == 37);
    CHECK(fallback->m_hbmBitmap != nullptr);
    CHECK(fallback->m_pTextureResource != nullptr);

    std::vector<std::uint32_t> fallback_pixels(256 * 256);
    CHECK(GetBitmapBits(
        fallback->m_hbmBitmap,
        static_cast<LONG>(fallback_pixels.size() * sizeof(std::uint32_t)),
        fallback_pixels.data()) ==
        fallback_pixels.size() * sizeof(std::uint32_t));
    CHECK(fallback_pixels[0] == UINT32_C(0x00ff7f3f));
    CHECK(fallback_pixels[4 * 256 + 0] == UINT32_C(0x00ff7f3f));
    CHECK(fallback_pixels[4 * 256 + 4] == UINT32_C(0x007f3fff));
    delete fallback;
    CHECK(framework->TryCloseCommandList() == S_OK);

    const std::size_t capacity = static_cast<std::size_t>(
        framework->m_freeShaderResourceDescriptors.capacity_end -
        framework->m_freeShaderResourceDescriptors.begin);
    ::operator delete(framework->m_freeShaderResourceDescriptors.begin,
                      capacity * sizeof(std::uint64_t));
    CloseHandle(framework->m_hFenceEvent);
    g_pD3DApp = nullptr;
    ::operator delete(framework_storage);
    return 0;
}

int main()
{
    char path[] = "D:\\retail\\textures";

    CHECK(Texture::GetBitsPerPixel(DXGI_FORMAT_R8G8B8A8_UNORM) == 32);
    CHECK(Texture::GetBitsPerPixel(DXGI_FORMAT_UNKNOWN) == 0);
    CHECK(Texture::GetBitsPerPixel(DXGI_FORMAT_B8G8R8A8_UNORM) == 0);

    CHECK(std::strcmp(
              jpb_D3DTexturePathForTest(),
              "c:\\el_chavo\\work\\level") == 0);
    SetTexturePath(path);
    CHECK(std::strcmp(jpb_D3DTexturePathForTest(), path) == 0);
    SetTexturePath(nullptr);
    CHECK(jpb_D3DTexturePathForTest()[0] == '\0');

    CHECK(RestoreAllTextures(nullptr) == S_OK);
    CHECK(test_texture_constructor_and_restore() == 0);
    CHECK(test_legacy_texture_factories_and_delete_all() == 0);
    CHECK(test_load_image_data() == 0);
    CHECK(test_specialized_image_loaders() == 0);
    CHECK(test_copy_bitmap_to_surface() == 0);
    CHECK(test_copy_rgba_data_to_surface() == 0);
    CHECK(test_upload_round_trip() == 0);
    CHECK(test_texture_factory_resource_path() == 0);
    CHECK(test_srv_heap_and_file_factory_path() == 0);
    std::puts("d3dtextr leaf tests passed");
    return 0;
}
