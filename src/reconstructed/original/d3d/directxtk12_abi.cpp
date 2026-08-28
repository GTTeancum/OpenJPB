#include "jpb/directxtk12_abi.h"

#include <cfloat>
#include <climits>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <future>
#include <map>
#include <mutex>
#include <new>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

std::uint64_t UpdateSubresources(
    ID3D12GraphicsCommandList *command_list,
    ID3D12Resource *destination_resource,
    ID3D12Resource *intermediate_resource,
    UINT first_subresource,
    UINT subresource_count,
    std::uint64_t required_size,
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *layouts,
    UINT *row_counts,
    std::uint64_t *row_sizes,
    D3D12_SUBRESOURCE_DATA *source_data);

namespace DirectX {

namespace {
void ThrowIfFailed(HRESULT result);
}

LinearAllocatorPage::LinearAllocatorPage() noexcept
    : pPrevPage(nullptr),
      pNextPage(nullptr),
      mMemory(nullptr),
      mPendingFence(0),
      mGpuAddress(0),
      mOffset(0),
      mSize(0),
      mUploadResource(nullptr),
      mRefCount(1)
{
}

void LinearAllocatorPage::AddRef() noexcept
{
    InterlockedIncrement(&mRefCount);
}

void LinearAllocatorPage::Release() noexcept
{
    if (InterlockedDecrement(&mRefCount) != 0) {
        return;
    }

    mUploadResource->Unmap(0, nullptr);
    mUploadResource->Release();
    mUploadResource = nullptr;
    delete this;
}

std::uint64_t LinearAllocatorPage::Suballocate(
    std::uint64_t size,
    std::uint64_t alignment)
{
    std::uint64_t aligned_offset = mOffset;
    if (alignment != 0) {
        aligned_offset = (aligned_offset + alignment - 1) &
                         ~(alignment - 1);
    }
    const std::uint64_t end = aligned_offset + size;
    if (end > mSize) {
        throw std::runtime_error("LinearAllocatorPage::Suballocate");
    }
    mOffset = end;
    return aligned_offset;
}

LinearAllocator::LinearAllocator(
    ID3D12Device *device,
    std::uint64_t page_size,
    std::uint64_t preallocate_bytes)
    : m_pendingPages(nullptr),
      m_usedPages(nullptr),
      m_unusedPages(nullptr),
      m_increment(page_size),
      m_numPending(0),
      m_totalPages(0),
      m_fenceCount(0),
      m_device(device),
      m_fence()
{
    const std::uint64_t page_count =
        (preallocate_bytes + page_size - 1) / page_size;
    for (std::uint64_t index = 0; index < page_count; ++index) {
        if (GetNewPage() == nullptr) {
            throw std::bad_alloc();
        }
    }

    ThrowIfFailed(device->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
}

LinearAllocator::~LinearAllocator()
{
    while (m_pendingPages != nullptr) {
        RetirePendingPages();
    }
    FreePages(m_unusedPages);
    FreePages(m_usedPages);
    m_pendingPages = nullptr;
    m_usedPages = nullptr;
    m_unusedPages = nullptr;
    m_increment = 0;
}

LinearAllocatorPage *LinearAllocator::FindPageForAlloc(
    LinearAllocatorPage *list,
    std::uint64_t size,
    std::uint64_t alignment)
{
    for (LinearAllocatorPage *page = list;
         page != nullptr;
         page = page->pNextPage) {
        std::uint64_t offset = page->mOffset;
        if (alignment != 0) {
            offset = (offset + alignment - 1) & ~(alignment - 1);
        }
        if (offset + size <= m_increment) {
            return page;
        }
    }
    return nullptr;
}

LinearAllocatorPage *LinearAllocator::FindPageForAlloc(
    std::uint64_t size,
    std::uint64_t alignment)
{
    if (size == m_increment &&
        (alignment == 0 || alignment == m_increment)) {
        return GetCleanPageForAlloc();
    }

    LinearAllocatorPage *page = FindPageForAlloc(
        m_usedPages, size, alignment);
    return page != nullptr ? page : GetCleanPageForAlloc();
}

void LinearAllocator::FreePages(LinearAllocatorPage *page)
{
    while (page != nullptr) {
        LinearAllocatorPage *next = page->pNextPage;
        page->Release();
        --m_totalPages;
        page = next;
    }
}

LinearAllocatorPage *LinearAllocator::GetCleanPageForAlloc()
{
    LinearAllocatorPage *page = m_unusedPages;
    if (page == nullptr) {
        page = GetNewPage();
        if (page == nullptr) {
            return nullptr;
        }
    }
    UnlinkPage(page);
    LinkPage(page, m_usedPages);
    return page;
}

LinearAllocatorPage *LinearAllocator::GetNewPage()
{
    D3D12_HEAP_PROPERTIES upload_heap = {};
    upload_heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    upload_heap.CreationNodeMask = 1;
    upload_heap.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC buffer = {};
    buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = m_increment;
    buffer.Height = 1;
    buffer.DepthOrArraySize = 1;
    buffer.MipLevels = 1;
    buffer.SampleDesc.Count = 1;
    buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    const HRESULT create_result = m_device->CreateCommittedResource(
        &upload_heap,
        D3D12_HEAP_FLAG_NONE,
        &buffer,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(resource.ReleaseAndGetAddressOf()));
    if (FAILED(create_result)) {
        return nullptr;
    }

    void *memory = nullptr;
    ThrowIfFailed(resource->Map(0, nullptr, &memory));
    std::memset(memory, 0, static_cast<std::size_t>(m_increment));

    auto *page = new LinearAllocatorPage();
    page->mMemory = memory;
    page->mGpuAddress = resource->GetGPUVirtualAddress();
    page->mSize = m_increment;
    page->mUploadResource = resource.Detach();
    LinkPage(page, m_unusedPages);
    ++m_totalPages;
    return page;
}

void LinearAllocator::LinkPage(
    LinearAllocatorPage *page,
    LinearAllocatorPage *&list)
{
    page->pNextPage = list;
    if (list != nullptr) {
        list->pPrevPage = page;
    }
    list = page;
}

void LinearAllocator::LinkPageChain(
    LinearAllocatorPage *page,
    LinearAllocatorPage *&list)
{
    LinearAllocatorPage *last = page;
    while (last->pNextPage != nullptr) {
        last = last->pNextPage;
    }
    last->pNextPage = list;
    if (list != nullptr) {
        list->pPrevPage = last;
    }
    list = page;
}

void LinearAllocator::UnlinkPage(LinearAllocatorPage *page)
{
    if (page->pPrevPage != nullptr) {
        page->pPrevPage->pNextPage = page->pNextPage;
    } else if (page == m_unusedPages) {
        m_unusedPages = page->pNextPage;
    } else if (page == m_usedPages) {
        m_usedPages = page->pNextPage;
    } else if (page == m_pendingPages) {
        m_pendingPages = page->pNextPage;
    }
    if (page->pNextPage != nullptr) {
        page->pNextPage->pPrevPage = page->pPrevPage;
    }
    page->pPrevPage = nullptr;
    page->pNextPage = nullptr;
}

void LinearAllocator::ReleasePage(LinearAllocatorPage *page)
{
    --m_numPending;
    UnlinkPage(page);
    LinkPage(page, m_unusedPages);
    page->mOffset = 0;
}

void LinearAllocator::FenceCommittedPages(
    ID3D12CommandQueue *command_queue)
{
    LinearAllocatorPage *page = m_usedPages;
    if (page == nullptr) {
        return;
    }

    LinearAllocatorPage *pending_first = nullptr;
    LinearAllocatorPage *pending_last = nullptr;
    LinearAllocatorPage *used_first = nullptr;
    std::uint32_t retired_count = 0;
    while (page != nullptr) {
        LinearAllocatorPage *next = page->pNextPage;
        page->pPrevPage = nullptr;
        if (page->mRefCount == 1) {
            page->mPendingFence = ++m_fenceCount;
            ThrowIfFailed(command_queue->Signal(
                m_fence.Get(), m_fenceCount));
            page->pNextPage = pending_first;
            if (pending_first != nullptr) {
                pending_first->pPrevPage = page;
            }
            pending_first = page;
            if (pending_last == nullptr) {
                pending_last = page;
            }
            ++retired_count;
        } else {
            page->pNextPage = used_first;
            if (used_first != nullptr) {
                used_first->pPrevPage = page;
            }
            used_first = page;
        }
        page = next;
    }
    m_usedPages = used_first;

    if (retired_count != 0) {
        m_numPending += retired_count;
        pending_last->pNextPage = m_pendingPages;
        if (m_pendingPages != nullptr) {
            m_pendingPages->pPrevPage = pending_last;
        }
        m_pendingPages = pending_first;
    }
}

void LinearAllocator::RetirePendingPages()
{
    const std::uint64_t completed = m_fence->GetCompletedValue();
    LinearAllocatorPage *page = m_pendingPages;
    while (page != nullptr) {
        LinearAllocatorPage *next = page->pNextPage;
        if (completed >= page->mPendingFence) {
            ReleasePage(page);
        }
        page = next;
    }
}

void LinearAllocator::Shrink()
{
    FreePages(m_unusedPages);
    m_unusedPages = nullptr;
}

com_exception::com_exception(HRESULT failure) noexcept
    : result(failure)
{
}

const char *com_exception::what() const noexcept
{
    static char message[64];
    std::sprintf(message, "Failure with HRESULT of %08X", result);
    return message;
}

namespace {

#include "spritebatch_shaders.inc"

void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result)) {
        throw com_exception(result);
    }
}

} // namespace

struct ResourceUploadBatch::Impl {
    struct SharedGraphicsResourceStorage {
        std::uint64_t words[2];
    };

    template<typename T>
    struct ExactVector {
        ExactVector() noexcept
            : data_begin(nullptr), data_end(nullptr), capacity_end(nullptr)
        {
        }

        ~ExactVector()
        {
            std::allocator<T> allocator;
            for (T *item = data_begin; item != data_end; ++item) {
                item->~T();
            }
            if (data_begin != nullptr) {
                allocator.deallocate(
                    data_begin,
                    static_cast<std::size_t>(
                        capacity_end - data_begin));
            }
        }

        ExactVector(const ExactVector &) = delete;
        ExactVector &operator=(const ExactVector &) = delete;

        template<typename... Arguments>
        void emplace_back(Arguments &&...arguments)
        {
            if (data_end == capacity_end) {
                const std::size_t old_size = data_begin == nullptr
                    ? 0
                    : static_cast<std::size_t>(
                          data_end - data_begin);
                const std::size_t old_capacity = data_begin == nullptr
                    ? 0
                    : static_cast<std::size_t>(
                          capacity_end - data_begin);
                const std::size_t geometric =
                    old_capacity + old_capacity / 2;
                const std::size_t new_capacity = (std::max)(
                    old_size + 1,
                    geometric);
                std::allocator<T> allocator;
                T *new_data = allocator.allocate(new_capacity);
                std::size_t moved = 0;
                try {
                    for (; moved < old_size; ++moved) {
                        new (new_data + moved) T(
                            std::move(data_begin[moved]));
                    }
                    new (new_data + old_size) T(
                        std::forward<Arguments>(arguments)...);
                } catch (...) {
                    for (std::size_t index = 0;
                         index < moved;
                         ++index) {
                        new_data[index].~T();
                    }
                    allocator.deallocate(new_data, new_capacity);
                    throw;
                }
                for (T *item = data_begin;
                     item != data_end;
                     ++item) {
                    item->~T();
                }
                if (data_begin != nullptr) {
                    allocator.deallocate(data_begin, old_capacity);
                }
                data_begin = new_data;
                data_end = new_data + old_size + 1;
                capacity_end = new_data + new_capacity;
                return;
            }

            new (data_end) T(std::forward<Arguments>(arguments)...);
            ++data_end;
        }

        void swap(ExactVector &other) noexcept
        {
            std::swap(data_begin, other.data_begin);
            std::swap(data_end, other.data_end);
            std::swap(capacity_end, other.capacity_end);
        }

        T *data_begin;
        T *data_end;
        T *capacity_end;
    };

    struct UploadBatch {
        ExactVector<Microsoft::WRL::ComPtr<ID3D12DeviceChild>>
            TrackedObjects;
        ExactVector<SharedGraphicsResourceStorage>
            TrackedMemoryResources;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
        Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
        HANDLE GpuCompleteEvent = nullptr;

        ~UploadBatch()
        {
            if (GpuCompleteEvent != nullptr) {
                CloseHandle(GpuCompleteEvent);
            }
        }
    };

    static_assert(sizeof(UploadBatch) == 72,
                  "ResourceUploadBatch::UploadBatch PDB size changed");

    explicit Impl(ID3D12Device *device)
        : mDevice(device),
          mCmdAlloc(),
          mList(),
          mGenMipsResources(),
          mTrackedObjects(),
          mTrackedMemoryResources(),
          mCommandType(D3D12_COMMAND_LIST_TYPE_DIRECT),
          mInBeginEndBlock(false),
          mTypedUAVLoadAdditionalFormats(false),
          mStandardSwizzle64KBSupported(false),
          padding_087(0)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
        if (SUCCEEDED(mDevice->CheckFeatureSupport(
                D3D12_FEATURE_D3D12_OPTIONS,
                &options,
                sizeof(options)))) {
            mTypedUAVLoadAdditionalFormats =
                options.TypedUAVLoadAdditionalFormats != FALSE;
            mStandardSwizzle64KBSupported =
                options.StandardSwizzle64KBSupported != FALSE;
        }
    }

#if defined(JPB_D3DAPP_TESTING)
    Impl() noexcept
        : mDevice(),
          mCmdAlloc(),
          mList(),
          mGenMipsResources(),
          mTrackedObjects(),
          mTrackedMemoryResources(),
          mCommandType(D3D12_COMMAND_LIST_TYPE_DIRECT),
          mInBeginEndBlock(false),
          mTypedUAVLoadAdditionalFormats(false),
          mStandardSwizzle64KBSupported(false),
          padding_087(0)
    {
    }
#endif

    void Begin(D3D12_COMMAND_LIST_TYPE command_type)
    {
        if (mInBeginEndBlock) {
            throw std::logic_error(
                "Can't call Begin on an open ResourceUploadBatch.");
        }
        if (command_type != D3D12_COMMAND_LIST_TYPE_DIRECT &&
            command_type != D3D12_COMMAND_LIST_TYPE_COMPUTE &&
            command_type != D3D12_COMMAND_LIST_TYPE_COPY) {
            throw std::invalid_argument("commandType");
        }

        mCmdAlloc.Reset();
        ThrowIfFailed(mDevice->CreateCommandAllocator(
            command_type,
            IID_PPV_ARGS(mCmdAlloc.ReleaseAndGetAddressOf())));
        mList.Reset();
        ThrowIfFailed(mDevice->CreateCommandList(
            1,
            command_type,
            mCmdAlloc.Get(),
            nullptr,
            IID_PPV_ARGS(mList.ReleaseAndGetAddressOf())));
        mCommandType = command_type;
        mInBeginEndBlock = true;
    }

    std::future<void> End(ID3D12CommandQueue *command_queue)
    {
        if (!mInBeginEndBlock) {
            throw std::logic_error(
                "Can't call End on a closed ResourceUploadBatch.");
        }

        ThrowIfFailed(mList->Close());
        ID3D12CommandList *lists[] = {mList.Get()};
        command_queue->ExecuteCommandLists(1, lists);

        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        ThrowIfFailed(mDevice->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));

        HANDLE gpu_complete_event = CreateEventExW(
            nullptr,
            nullptr,
            0,
            EVENT_MODIFY_STATE | SYNCHRONIZE);
        if (gpu_complete_event == nullptr) {
            throw std::system_error(
                static_cast<int>(GetLastError()),
                std::system_category());
        }

        try {
            ThrowIfFailed(command_queue->Signal(fence.Get(), 1));
            ThrowIfFailed(fence->SetEventOnCompletion(
                1, gpu_complete_event));
        } catch (...) {
            CloseHandle(gpu_complete_event);
            throw;
        }

        auto upload_batch = std::make_unique<UploadBatch>();
        upload_batch->CommandList = mList;
        upload_batch->Fence = fence;
        upload_batch->GpuCompleteEvent = gpu_complete_event;
        mTrackedObjects.swap(upload_batch->TrackedObjects);
        mTrackedMemoryResources.swap(
            upload_batch->TrackedMemoryResources);

        auto completion = std::async(
            std::launch::async,
            [batch = std::move(upload_batch)]() mutable {
                const DWORD wait_result = WaitForSingleObject(
                    batch->GpuCompleteEvent, INFINITE);
                if (wait_result == WAIT_OBJECT_0) {
                    batch.reset();
                    return;
                }
                if (wait_result == WAIT_FAILED) {
                    throw std::system_error(
                        static_cast<int>(GetLastError()),
                        std::system_category());
                }
                throw std::runtime_error(
                    "Unexpected WaitForSingleObject result");
            });

        mCommandType = D3D12_COMMAND_LIST_TYPE_DIRECT;
        mInBeginEndBlock = false;
        mList.Reset();
        mCmdAlloc.Reset();
        return completion;
    }

    void Upload(
        ID3D12Resource *resource,
        UINT subresource_index_start,
        const D3D12_SUBRESOURCE_DATA *subresources,
        UINT num_subresources)
    {
        if (!mInBeginEndBlock) {
            throw std::logic_error(
                "Can't call Upload on a closed ResourceUploadBatch.");
        }

        const D3D12_RESOURCE_DESC destination_description =
            resource->GetDesc();
        std::uint64_t required_size = 0;
        {
            Microsoft::WRL::ComPtr<ID3D12Device> resource_device;
            resource->GetDevice(IID_PPV_ARGS(
                resource_device.ReleaseAndGetAddressOf()));
            resource_device->GetCopyableFootprints(
                &destination_description,
                subresource_index_start,
                num_subresources,
                0,
                nullptr,
                nullptr,
                nullptr,
                &required_size);
        }

        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
        heap_properties.CreationNodeMask = 1;
        heap_properties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC upload_description = {};
        upload_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        upload_description.Width = required_size;
        upload_description.Height = 1;
        upload_description.DepthOrArraySize = 1;
        upload_description.MipLevels = 1;
        upload_description.SampleDesc.Count = 1;
        upload_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        Microsoft::WRL::ComPtr<ID3D12Resource> upload_resource;
        ThrowIfFailed(mDevice->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &upload_description,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(upload_resource.ReleaseAndGetAddressOf())));

        const SIZE_T footprint_bytes =
            static_cast<SIZE_T>(num_subresources) * 44;
        void *footprint_memory = HeapAlloc(
            GetProcessHeap(), 0, footprint_bytes);
        if (footprint_memory != nullptr) {
            auto *layouts = static_cast<
                D3D12_PLACED_SUBRESOURCE_FOOTPRINT *>(
                    footprint_memory);
            auto *row_counts = reinterpret_cast<UINT *>(
                static_cast<unsigned char *>(footprint_memory) +
                static_cast<SIZE_T>(num_subresources) * 32);
            auto *row_sizes = reinterpret_cast<std::uint64_t *>(
                static_cast<unsigned char *>(footprint_memory) +
                static_cast<SIZE_T>(num_subresources) * 40);

            Microsoft::WRL::ComPtr<ID3D12Device> resource_device;
            resource->GetDevice(IID_PPV_ARGS(
                resource_device.ReleaseAndGetAddressOf()));
            resource_device->GetCopyableFootprints(
                &destination_description,
                subresource_index_start,
                num_subresources,
                0,
                layouts,
                row_counts,
                row_sizes,
                &required_size);

            UpdateSubresources(
                mList.Get(),
                resource,
                upload_resource.Get(),
                subresource_index_start,
                num_subresources,
                required_size,
                layouts,
                row_counts,
                row_sizes,
                const_cast<D3D12_SUBRESOURCE_DATA *>(subresources));
            HeapFree(GetProcessHeap(), 0, footprint_memory);
        }

        mTrackedObjects.emplace_back(upload_resource.Get());
    }

    void Transition(
        ID3D12Resource *resource,
        D3D12_RESOURCE_STATES state_before,
        D3D12_RESOURCE_STATES state_after)
    {
        if (!mInBeginEndBlock) {
            throw std::logic_error(
                "Can't call Upload on a closed ResourceUploadBatch.");
        }

        bool allowed = true;
        if (mCommandType == D3D12_COMMAND_LIST_TYPE_COPY) {
            allowed = state_after == D3D12_RESOURCE_STATE_COPY_DEST ||
                      state_after == D3D12_RESOURCE_STATE_COPY_SOURCE;
        } else if (mCommandType == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
            allowed = state_after ==
                          D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ||
                      state_after == D3D12_RESOURCE_STATE_INDEX_BUFFER ||
                      state_after ==
                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS ||
                      state_after ==
                          D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE ||
                      state_after == D3D12_RESOURCE_STATE_COPY_DEST ||
                      state_after == D3D12_RESOURCE_STATE_COPY_SOURCE;
        }
        if (!allowed || state_before == state_after) {
            return;
        }

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = state_before;
        barrier.Transition.StateAfter = state_after;
        mList->ResourceBarrier(1, &barrier);
    }

    Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCmdAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mList;
    std::unique_ptr<unsigned char> mGenMipsResources;
    ExactVector<Microsoft::WRL::ComPtr<ID3D12DeviceChild>>
        mTrackedObjects;
    ExactVector<SharedGraphicsResourceStorage>
        mTrackedMemoryResources;
    D3D12_COMMAND_LIST_TYPE mCommandType;
    bool mInBeginEndBlock;
    bool mTypedUAVLoadAdditionalFormats;
    bool mStandardSwizzle64KBSupported;
    unsigned char padding_087;
};

static_assert(sizeof(
                  ResourceUploadBatch::Impl::ExactVector<int>) == 24,
              "ResourceUploadBatch exact vector layout changed");

static_assert(sizeof(ResourceUploadBatch::Impl) == 88,
              "ResourceUploadBatch::Impl PDB size changed");
static_assert(offsetof(ResourceUploadBatch::Impl, mTrackedObjects) == 32,
              "ResourceUploadBatch tracked-object offset changed");
static_assert(offsetof(
                  ResourceUploadBatch::Impl,
                  mTrackedMemoryResources) == 56,
              "ResourceUploadBatch tracked-memory offset changed");
static_assert(offsetof(ResourceUploadBatch::Impl, mCommandType) == 80,
              "ResourceUploadBatch command-type offset changed");
static_assert(offsetof(
                  ResourceUploadBatch::Impl,
                  mInBeginEndBlock) == 84,
              "ResourceUploadBatch open-state offset changed");

ResourceUploadBatch::ResourceUploadBatch(ID3D12Device *device)
    : pImpl(std::make_unique<Impl>(device))
{
}

ResourceUploadBatch::ResourceUploadBatch(
    ResourceUploadBatch &&other) noexcept
    : pImpl(std::move(other.pImpl))
{
}

ResourceUploadBatch::~ResourceUploadBatch() = default;

ResourceUploadBatch &ResourceUploadBatch::operator=(
    ResourceUploadBatch &&other) noexcept
{
    pImpl = std::move(other.pImpl);
    return *this;
}

void ResourceUploadBatch::Begin(D3D12_COMMAND_LIST_TYPE command_type)
{
    pImpl->Begin(command_type);
}

std::future<void> ResourceUploadBatch::End(
    ID3D12CommandQueue *command_queue)
{
    return pImpl->End(command_queue);
}

void ResourceUploadBatch::Upload(
    ID3D12Resource *resource,
    UINT subresource_index_start,
    const D3D12_SUBRESOURCE_DATA *subresources,
    UINT num_subresources)
{
    pImpl->Upload(
        resource,
        subresource_index_start,
        subresources,
        num_subresources);
}

void ResourceUploadBatch::Transition(
    ID3D12Resource *resource,
    D3D12_RESOURCE_STATES state_before,
    D3D12_RESOURCE_STATES state_after)
{
    pImpl->Transition(resource, state_before, state_after);
}

#if defined(JPB_D3DAPP_TESTING)
namespace DX12 {

void jpb_directxtk12_test_resource_upload_transition(
    bool open,
    D3D12_COMMAND_LIST_TYPE command_type,
    ID3D12GraphicsCommandList *command_list,
    ID3D12Resource *resource,
    D3D12_RESOURCE_STATES state_before,
    D3D12_RESOURCE_STATES state_after)
{
    ResourceUploadBatch::Impl impl = {};
    impl.mInBeginEndBlock = open;
    impl.mCommandType = command_type;
    impl.mList.Attach(command_list);

    alignas(ResourceUploadBatch) unsigned char batch_storage[16] = {};
    ResourceUploadBatch::Impl *impl_pointer = &impl;
    std::memcpy(
        batch_storage + 8,
        &impl_pointer,
        sizeof(impl_pointer));
    auto *batch = reinterpret_cast<ResourceUploadBatch *>(batch_storage);

    try {
        batch->Transition(resource, state_before, state_after);
    } catch (...) {
        impl.mList.Detach();
        throw;
    }
    impl.mList.Detach();
}

} // namespace DX12
#endif

DescriptorHeap::DescriptorHeap(ID3D12DescriptorHeap *existing_heap)
    : m_pHeap(existing_heap)
{
    m_hCPU = existing_heap->GetCPUDescriptorHandleForHeapStart();
    m_hGPU = existing_heap->GetGPUDescriptorHandleForHeapStart();
    m_desc = existing_heap->GetDesc();

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    existing_heap->GetDevice(IID_PPV_ARGS(&device));
    m_increment = device->GetDescriptorHandleIncrementSize(m_desc.Type);
}

DescriptorHeap::DescriptorHeap(
    ID3D12Device *device,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    D3D12_DESCRIPTOR_HEAP_FLAGS flags,
    std::size_t count)
    : m_pHeap(), m_desc(), m_hCPU(), m_hGPU(), m_increment(0)
{
    if (count > UINT_MAX) {
        throw std::invalid_argument("Too many descriptors");
    }

    D3D12_DESCRIPTOR_HEAP_DESC description = {};
    description.Type = type;
    description.NumDescriptors = static_cast<UINT>(count);
    description.Flags = flags;
    Create(device, &description);
}

void DescriptorHeap::Create(
    ID3D12Device *device,
    const D3D12_DESCRIPTOR_HEAP_DESC *description)
{
    m_desc = *description;
    m_increment = device->GetDescriptorHandleIncrementSize(
        description->Type);

    m_pHeap.Reset();
    if (description->NumDescriptors == 0) {
        m_hCPU.ptr = 0;
        m_hGPU.ptr = 0;
        return;
    }

    ThrowIfFailed(device->CreateDescriptorHeap(
        description, IID_PPV_ARGS(m_pHeap.ReleaseAndGetAddressOf())));
    m_hCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart();
    if ((description->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0) {
        m_hGPU = m_pHeap->GetGPUDescriptorHandleForHeapStart();
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(
    std::size_t index) const
{
    if (index >= m_desc.NumDescriptors) {
        throw std::out_of_range("D3DX12_GPU_DESCRIPTOR_HANDLE");
    }

    D3D12_GPU_DESCRIPTOR_HANDLE handle = m_hGPU;
    handle.ptr += index * m_increment;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(
    std::size_t index) const
{
    if (index >= m_desc.NumDescriptors) {
        throw std::out_of_range("D3DX12_CPU_DESCRIPTOR_HANDLE");
    }

    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_hCPU;
    handle.ptr += index * m_increment;
    return handle;
}

namespace DX12 {

#if defined(JPB_D3DAPP_TESTING)
static JPBSpriteBatchRootSignatureTestHook
    g_sprite_batch_root_signature_test_hook = nullptr;
#endif

const D3D12_BLEND_DESC
    SpriteBatchPipelineStateDescription::s_DefaultBlendDesc = {
        FALSE,
        FALSE,
        {{TRUE,
          FALSE,
          D3D12_BLEND_SRC_ALPHA,
          D3D12_BLEND_INV_SRC_ALPHA,
          D3D12_BLEND_OP_ADD,
          D3D12_BLEND_SRC_ALPHA,
          D3D12_BLEND_INV_SRC_ALPHA,
          D3D12_BLEND_OP_ADD,
          D3D12_LOGIC_OP_NOOP,
          D3D12_COLOR_WRITE_ENABLE_ALL}}};

const D3D12_RASTERIZER_DESC
    SpriteBatchPipelineStateDescription::s_DefaultRasterizerDesc = {
        D3D12_FILL_MODE_SOLID,
        D3D12_CULL_MODE_BACK,
        FALSE,
        D3D12_DEFAULT_DEPTH_BIAS,
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        TRUE,
        TRUE,
        FALSE,
        0,
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF};

const D3D12_DEPTH_STENCIL_DESC
    SpriteBatchPipelineStateDescription::s_DefaultDepthStencilDesc = {
        FALSE,
        D3D12_DEPTH_WRITE_MASK_ZERO,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        FALSE,
        D3D12_DEFAULT_STENCIL_READ_MASK,
        D3D12_DEFAULT_STENCIL_WRITE_MASK,
        {D3D12_STENCIL_OP_KEEP,
         D3D12_STENCIL_OP_KEEP,
         D3D12_STENCIL_OP_KEEP,
         D3D12_COMPARISON_FUNC_ALWAYS},
        {D3D12_STENCIL_OP_KEEP,
         D3D12_STENCIL_OP_KEEP,
         D3D12_STENCIL_OP_KEEP,
         D3D12_COMPARISON_FUNC_ALWAYS}};

GraphicsResource::GraphicsResource() noexcept
    : mPage(nullptr),
      mGpuAddress(0),
      mResource(nullptr),
      mMemory(nullptr),
      mBufferOffset(0),
      mSize(0)
{
}

GraphicsResource::GraphicsResource(GraphicsResource &&other) noexcept
    : GraphicsResource()
{
    Reset(std::move(other));
}

GraphicsResource::GraphicsResource(
    LinearAllocatorPage *page,
    std::uint64_t gpu_address,
    ID3D12Resource *resource,
    void *memory,
    std::uint64_t offset,
    std::uint64_t size) noexcept
    : mPage(page),
      mGpuAddress(gpu_address),
      mResource(resource),
      mMemory(memory),
      mBufferOffset(offset),
      mSize(size)
{
    mPage->AddRef();
}

GraphicsResource &GraphicsResource::operator=(GraphicsResource &&other) noexcept
{
    Reset(std::move(other));
    return *this;
}

GraphicsResource::~GraphicsResource()
{
    if (mPage != nullptr) {
        mPage->Release();
    }
}

void GraphicsResource::Reset(GraphicsResource &&allocation) noexcept
{
    if (mPage != nullptr) {
        mPage->Release();
        mPage = nullptr;
    }

    mGpuAddress = allocation.mGpuAddress;
    mResource = allocation.mResource;
    mMemory = allocation.mMemory;
    mBufferOffset = allocation.mBufferOffset;
    mSize = allocation.mSize;
    mPage = allocation.mPage;

    allocation.mGpuAddress = 0;
    allocation.mResource = nullptr;
    allocation.mMemory = nullptr;
    allocation.mBufferOffset = 0;
    allocation.mSize = 0;
    allocation.mPage = nullptr;
}

void GraphicsResource::Reset() noexcept
{
    if (mPage != nullptr) {
        mPage->Release();
        mPage = nullptr;
    }

    mGpuAddress = 0;
    mResource = nullptr;
    mMemory = nullptr;
    mBufferOffset = 0;
    mSize = 0;
}

namespace {

constexpr std::size_t GraphicsMemoryPoolCount = 21;

std::uint64_t NextPowerOfTwo(std::uint64_t value) noexcept
{
    --value;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return value + 1;
}

std::size_t GetPoolIndexFromSize(std::uint64_t size) noexcept
{
    const std::uint64_t pages = size >> 12;
    if (pages == 0) {
        return 0;
    }
    unsigned long index = 0;
    _BitScanForward64(&index, pages);
    return static_cast<std::size_t>(index + 1);
}

std::uint64_t GetPageSizeFromPoolIndex(std::size_t index) noexcept
{
    const std::uint64_t scaled =
        (index == 0 ? 1ull : (1ull << (index - 1))) << 12;
    return (std::max)(UINT64_C(0x10000), scaled);
}

class DeviceAllocator {
public:
    explicit DeviceAllocator(ID3D12Device *device)
        : mDevice(device), mPools(), mMutex()
    {
        if (device == nullptr) {
            throw std::invalid_argument("Invalid device parameter");
        }
        for (std::size_t index = 0;
             index < GraphicsMemoryPoolCount;
             ++index) {
            mPools[index] = std::make_unique<LinearAllocator>(
                device, GetPageSizeFromPoolIndex(index), 0);
        }
    }

    GraphicsResource Alloc(
        std::uint64_t size,
        std::uint64_t alignment)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        const std::uint64_t rounded = NextPowerOfTwo(size + alignment);
        const std::size_t pool_index = GetPoolIndexFromSize(rounded);
        LinearAllocator *allocator = mPools[pool_index].get();
        LinearAllocatorPage *page = allocator->FindPageForAlloc(
            size, alignment);
        if (page == nullptr) {
            throw std::bad_alloc();
        }
        const std::uint64_t offset = page->Suballocate(size, alignment);
        return GraphicsResource(
            page,
            page->mGpuAddress + offset,
            page->mUploadResource,
            static_cast<unsigned char *>(page->mMemory) + offset,
            offset,
            size);
    }

    void KickFences(ID3D12CommandQueue *command_queue)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        for (auto &allocator : mPools) {
            allocator->RetirePendingPages();
            allocator->FenceCommittedPages(command_queue);
        }
    }

    void GarbageCollect()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        for (auto &allocator : mPools) {
            allocator->Shrink();
        }
    }

    GraphicsMemoryStatistics GetStatistics()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        GraphicsMemoryStatistics statistics = {};
        for (auto &allocator : mPools) {
            statistics.committedMemory +=
                allocator->CommittedMemoryUsage();
            statistics.totalMemory += allocator->TotalMemoryUsage();
            statistics.totalPages += allocator->TotalPageCount();
        }
        return statistics;
    }

    ID3D12Device *GetDevice() const noexcept { return mDevice.Get(); }

    Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
    std::array<std::unique_ptr<LinearAllocator>,
               GraphicsMemoryPoolCount> mPools;
    std::mutex mMutex;
};

static_assert(sizeof(DeviceAllocator) == 256,
              "DeviceAllocator PDB size changed");
static_assert(offsetof(DeviceAllocator, mPools) == 8,
              "DeviceAllocator pool offset changed");
static_assert(offsetof(DeviceAllocator, mMutex) == 176,
              "DeviceAllocator mutex offset changed");

} // namespace

struct GraphicsMemory::Impl {
    explicit Impl(GraphicsMemory *owner) noexcept
        : mOwner(owner),
          mDeviceAllocator(),
          m_peakCommited(0),
          m_peakBytes(0),
          m_peakPages(0)
    {
    }

    ~Impl()
    {
        if (mDeviceAllocator != nullptr) {
            const auto found = s_graphicsMemory.find(
                mDeviceAllocator->GetDevice());
            if (found != s_graphicsMemory.end() &&
                found->second == this) {
                s_graphicsMemory.erase(found);
            }
        }
    }

    void Initialize(ID3D12Device *device)
    {
        mDeviceAllocator = std::make_unique<DeviceAllocator>(device);
        const auto inserted = s_graphicsMemory.emplace(device, this);
        if (!inserted.second) {
            throw std::logic_error(
                "GraphicsMemory is a per-device singleton");
        }
    }

    GraphicsMemory *mOwner;
    std::unique_ptr<DeviceAllocator> mDeviceAllocator;
    std::uint64_t m_peakCommited;
    std::uint64_t m_peakBytes;
    std::uint64_t m_peakPages;

    static std::map<ID3D12Device *, Impl *> s_graphicsMemory;
};

std::map<ID3D12Device *, GraphicsMemory::Impl *>
    GraphicsMemory::Impl::s_graphicsMemory;

static_assert(sizeof(GraphicsMemory::Impl) == 40,
              "GraphicsMemory::Impl PDB size changed");
static_assert(offsetof(GraphicsMemory::Impl, mDeviceAllocator) == 8,
              "GraphicsMemory allocator offset changed");
static_assert(offsetof(GraphicsMemory::Impl, m_peakCommited) == 16,
              "GraphicsMemory committed peak offset changed");

GraphicsMemory::GraphicsMemory(ID3D12Device *device)
    : pImpl(std::make_unique<Impl>(this))
{
    pImpl->Initialize(device);
}

GraphicsMemory::GraphicsMemory(GraphicsMemory &&other) noexcept
    : pImpl(std::move(other.pImpl))
{
    pImpl->mOwner = this;
}

GraphicsMemory &GraphicsMemory::operator=(GraphicsMemory &&other) noexcept
{
    pImpl = std::move(other.pImpl);
    pImpl->mOwner = this;
    return *this;
}

GraphicsMemory::~GraphicsMemory() = default;

GraphicsResource GraphicsMemory::Allocate(
    std::uint64_t size,
    std::uint64_t alignment)
{
    return pImpl->mDeviceAllocator->Alloc(size, alignment);
}

void GraphicsMemory::Commit(ID3D12CommandQueue *command_queue)
{
    pImpl->mDeviceAllocator->KickFences(command_queue);
}

void GraphicsMemory::GarbageCollect()
{
    pImpl->mDeviceAllocator->GarbageCollect();
}

GraphicsMemoryStatistics GraphicsMemory::GetStatistics()
{
    GraphicsMemoryStatistics statistics =
        pImpl->mDeviceAllocator->GetStatistics();
    pImpl->m_peakCommited = (std::max)(
        pImpl->m_peakCommited, statistics.committedMemory);
    pImpl->m_peakBytes = (std::max)(
        pImpl->m_peakBytes, statistics.totalMemory);
    pImpl->m_peakPages = (std::max)(
        pImpl->m_peakPages, statistics.totalPages);
    statistics.peakCommitedMemory = pImpl->m_peakCommited;
    statistics.peakTotalMemory = pImpl->m_peakBytes;
    statistics.peakTotalPages = pImpl->m_peakPages;
    return statistics;
}

void GraphicsMemory::ResetStatistics()
{
    pImpl->m_peakCommited = 0;
    pImpl->m_peakBytes = 0;
    pImpl->m_peakPages = 0;
}

GraphicsMemory &GraphicsMemory::Get(ID3D12Device *device)
{
    if (Impl::s_graphicsMemory.empty()) {
        throw std::logic_error("GraphicsMemory singleton not created");
    }
    const auto found = device == nullptr
        ? Impl::s_graphicsMemory.begin()
        : Impl::s_graphicsMemory.find(device);
    if (found == Impl::s_graphicsMemory.end() ||
        found->second->mOwner == nullptr) {
        throw std::logic_error(
            "GraphicsMemory per-device singleton not created");
    }
    return *found->second->mOwner;
}

struct CommonStates::Impl {
    explicit Impl(ID3D12Device *device)
        : mDescriptors(
              device,
              D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
              D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
              6)
    {
        static const D3D12_SAMPLER_DESC sampler_descriptions[6] = {
            {D3D12_FILTER_MIN_MAG_MIP_POINT,
             D3D12_TEXTURE_ADDRESS_MODE_WRAP,
             D3D12_TEXTURE_ADDRESS_MODE_WRAP,
             D3D12_TEXTURE_ADDRESS_MODE_WRAP,
             0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
             {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, FLT_MAX},
            {D3D12_FILTER_MIN_MAG_MIP_POINT,
             D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
             D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
             D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
             0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
             {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, FLT_MAX},
            {D3D12_FILTER_MIN_MAG_MIP_LINEAR,
             D3D12_TEXTURE_ADDRESS_MODE_WRAP,
             D3D12_TEXTURE_ADDRESS_MODE_WRAP,
             D3D12_TEXTURE_ADDRESS_MODE_WRAP,
             0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
             {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, FLT_MAX},
            {D3D12_FILTER_MIN_MAG_MIP_LINEAR,
             D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
             D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
             D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
             0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
             {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, FLT_MAX},
            {D3D12_FILTER_ANISOTROPIC,
             D3D12_TEXTURE_ADDRESS_MODE_WRAP,
             D3D12_TEXTURE_ADDRESS_MODE_WRAP,
             D3D12_TEXTURE_ADDRESS_MODE_WRAP,
             0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
             {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, FLT_MAX},
            {D3D12_FILTER_ANISOTROPIC,
             D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
             D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
             D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
             0.0f, 16, D3D12_COMPARISON_FUNC_NEVER,
             {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f, FLT_MAX},
        };

        for (std::size_t index = 0; index < 6; ++index) {
            device->CreateSampler(
                &sampler_descriptions[index],
                mDescriptors.GetCpuHandle(index));
        }
    }

    DescriptorHeap mDescriptors;
};

CommonStates::CommonStates(ID3D12Device *device)
    : pImpl(std::make_unique<Impl>(device))
{
}

CommonStates::CommonStates(CommonStates &&other) noexcept = default;

CommonStates &CommonStates::operator=(CommonStates &&other) noexcept = default;

CommonStates::~CommonStates() = default;

D3D12_GPU_DESCRIPTOR_HANDLE CommonStates::PointWrap() const
{
    return pImpl->mDescriptors.GetGpuHandle(0);
}

D3D12_GPU_DESCRIPTOR_HANDLE CommonStates::PointClamp() const
{
    return pImpl->mDescriptors.GetGpuHandle(1);
}

D3D12_GPU_DESCRIPTOR_HANDLE CommonStates::LinearWrap() const
{
    return pImpl->mDescriptors.GetGpuHandle(2);
}

D3D12_GPU_DESCRIPTOR_HANDLE CommonStates::LinearClamp() const
{
    return pImpl->mDescriptors.GetGpuHandle(3);
}

D3D12_GPU_DESCRIPTOR_HANDLE CommonStates::AnisotropicWrap() const
{
    return pImpl->mDescriptors.GetGpuHandle(4);
}

D3D12_GPU_DESCRIPTOR_HANDLE CommonStates::AnisotropicClamp() const
{
    return pImpl->mDescriptors.GetGpuHandle(5);
}

ID3D12DescriptorHeap *CommonStates::Heap() const
{
    return pImpl->mDescriptors.Heap();
}

struct alignas(16) SpriteBatch::Impl {
    struct alignas(16) SpriteInfo {
        DirectX::XMVECTOR source;
        DirectX::XMVECTOR destination;
        DirectX::XMVECTOR color;
        DirectX::XMVECTOR originRotationDepth;
        D3D12_GPU_DESCRIPTOR_HANDLE texture;
        DirectX::XMVECTOR textureSize;
        std::uint32_t flags;
        unsigned char padding_100[12];
    };

    struct SpritePointerVector {
        SpritePointerVector() noexcept
            : data_begin(nullptr), data_end(nullptr), capacity_end(nullptr)
        {
        }

        ~SpritePointerVector()
        {
            std::allocator<const SpriteInfo *> allocator;
            if (data_begin != nullptr) {
                allocator.deallocate(
                    data_begin,
                    static_cast<std::size_t>(capacity_end - data_begin));
            }
        }

        SpritePointerVector(const SpritePointerVector &) = delete;
        SpritePointerVector &operator=(const SpritePointerVector &) = delete;

        std::size_t size() const noexcept
        {
            if (data_begin == nullptr) {
                return 0;
            }
            return static_cast<std::size_t>(data_end - data_begin);
        }

        void clear() noexcept { data_end = data_begin; }

        void resize(std::size_t new_size)
        {
            const std::size_t old_size = size();
            const std::size_t old_capacity = data_begin == nullptr
                ? 0
                : static_cast<std::size_t>(capacity_end - data_begin);
            if (new_size <= old_capacity) {
                if (new_size > old_size) {
                    std::fill(data_end, data_begin + new_size, nullptr);
                }
                data_end = data_begin + new_size;
                return;
            }

            const std::size_t geometric = old_capacity + old_capacity / 2;
            const std::size_t new_capacity = (std::max)(new_size, geometric);
            std::allocator<const SpriteInfo *> allocator;
            const SpriteInfo **new_data = allocator.allocate(new_capacity);
            std::copy(data_begin, data_end, new_data);
            std::fill(new_data + old_size, new_data + new_size, nullptr);
            if (data_begin != nullptr) {
                allocator.deallocate(data_begin, old_capacity);
            }
            data_begin = new_data;
            data_end = new_data + new_size;
            capacity_end = new_data + new_capacity;
        }

        const SpriteInfo *&operator[](std::size_t index) noexcept
        {
            return data_begin[index];
        }

        const SpriteInfo **begin() noexcept { return data_begin; }

        const SpriteInfo **data_begin;
        const SpriteInfo **data_end;
        const SpriteInfo **capacity_end;
    };

    struct DeviceResources {
#if defined(JPB_D3DAPP_TESTING)
        DeviceResources() noexcept
            : indexBuffer(),
              indexBufferView(),
              rootSignatureStatic(),
              rootSignatureHeap(),
              mDevice(nullptr)
        {
        }
#endif

        DeviceResources(
            ID3D12Device *device,
            DirectX::ResourceUploadBatch &upload)
            : indexBuffer(),
              indexBufferView(),
              rootSignatureStatic(),
              rootSignatureHeap(),
              mDevice(device)
        {
            CreateIndexBuffer(device, upload);
            CreateRootSignatures(device);
        }

        static std::vector<std::int16_t> CreateIndexValues()
        {
            std::vector<std::int16_t> indices;
            indices.reserve(2048 * 6);
            for (std::int16_t vertex = 0;
                 vertex < 2048 * 4;
                 vertex = static_cast<std::int16_t>(vertex + 4)) {
                indices.push_back(vertex);
                indices.push_back(
                    static_cast<std::int16_t>(vertex + 1));
                indices.push_back(
                    static_cast<std::int16_t>(vertex + 2));
                indices.push_back(
                    static_cast<std::int16_t>(vertex + 1));
                indices.push_back(
                    static_cast<std::int16_t>(vertex + 3));
                indices.push_back(
                    static_cast<std::int16_t>(vertex + 2));
            }
            return indices;
        }

        void CreateIndexBuffer(
            ID3D12Device *device,
            DirectX::ResourceUploadBatch &upload)
        {
            D3D12_HEAP_PROPERTIES heap_properties = {};
            heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
            heap_properties.CreationNodeMask = 1;
            heap_properties.VisibleNodeMask = 1;

            D3D12_RESOURCE_DESC resource_description = {};
            resource_description.Dimension =
                D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_description.Width = 2048 * 6 * sizeof(std::int16_t);
            resource_description.Height = 1;
            resource_description.DepthOrArraySize = 1;
            resource_description.MipLevels = 1;
            resource_description.SampleDesc.Count = 1;
            resource_description.Layout =
                D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            indexBuffer.Reset();
            ThrowIfFailed(device->CreateCommittedResource(
                &heap_properties,
                D3D12_HEAP_FLAG_NONE,
                &resource_description,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(indexBuffer.ReleaseAndGetAddressOf())));

            const std::vector<std::int16_t> indices =
                CreateIndexValues();
            D3D12_SUBRESOURCE_DATA source = {};
            source.pData = indices.data();
            source.RowPitch = static_cast<LONG_PTR>(
                indices.size() * sizeof(indices[0]));
            source.SlicePitch = source.RowPitch;
            upload.Upload(indexBuffer.Get(), 0, &source, 1);
            upload.Transition(
                indexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_INDEX_BUFFER);

            indexBufferView.BufferLocation =
                indexBuffer->GetGPUVirtualAddress();
            indexBufferView.SizeInBytes =
                static_cast<UINT>(source.RowPitch);
            indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        }

        static void CreateRootSignature(
            ID3D12Device *device,
            const D3D12_ROOT_SIGNATURE_DESC &description,
            Microsoft::WRL::ComPtr<ID3D12RootSignature> &signature)
        {
            Microsoft::WRL::ComPtr<ID3DBlob> blob;
            Microsoft::WRL::ComPtr<ID3DBlob> errors;
            HRESULT result = D3D12SerializeRootSignature(
                &description,
                D3D_ROOT_SIGNATURE_VERSION_1,
                blob.ReleaseAndGetAddressOf(),
                errors.ReleaseAndGetAddressOf());
            if (SUCCEEDED(result)) {
                result = device->CreateRootSignature(
                    0,
                    blob->GetBufferPointer(),
                    blob->GetBufferSize(),
                    IID_PPV_ARGS(signature.ReleaseAndGetAddressOf()));
            }
            ThrowIfFailed(result);
        }

        void CreateRootSignatures(ID3D12Device *device)
        {
            D3D12_DESCRIPTOR_RANGE texture_range = {};
            texture_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            texture_range.NumDescriptors = 1;
            texture_range.BaseShaderRegister = 0;
            texture_range.RegisterSpace = 0;
            texture_range.OffsetInDescriptorsFromTableStart =
                D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            D3D12_ROOT_PARAMETER static_parameters[2] = {};
            static_parameters[0].ParameterType =
                D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            static_parameters[0].DescriptorTable.NumDescriptorRanges = 1;
            static_parameters[0].DescriptorTable.pDescriptorRanges =
                &texture_range;
            static_parameters[0].ShaderVisibility =
                D3D12_SHADER_VISIBILITY_PIXEL;
            static_parameters[1].ParameterType =
                D3D12_ROOT_PARAMETER_TYPE_CBV;
            static_parameters[1].Descriptor.ShaderRegister = 0;
            static_parameters[1].Descriptor.RegisterSpace = 0;
            static_parameters[1].ShaderVisibility =
                D3D12_SHADER_VISIBILITY_ALL;

            D3D12_STATIC_SAMPLER_DESC static_sampler = {};
            static_sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            static_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            static_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            static_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            static_sampler.MaxAnisotropy = 16;
            static_sampler.ComparisonFunc =
                D3D12_COMPARISON_FUNC_LESS_EQUAL;
            static_sampler.BorderColor =
                D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            static_sampler.MinLOD = 0.0f;
            static_sampler.MaxLOD = FLT_MAX;
            static_sampler.ShaderRegister = 0;
            static_sampler.RegisterSpace = 0;
            static_sampler.ShaderVisibility =
                D3D12_SHADER_VISIBILITY_PIXEL;

            const D3D12_ROOT_SIGNATURE_FLAGS signature_flags =
                static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(
                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

            D3D12_ROOT_SIGNATURE_DESC static_description = {};
            static_description.NumParameters = 2;
            static_description.pParameters = static_parameters;
            static_description.NumStaticSamplers = 1;
            static_description.pStaticSamplers = &static_sampler;
            static_description.Flags = signature_flags;
#if defined(JPB_D3DAPP_TESTING)
            if (g_sprite_batch_root_signature_test_hook != nullptr) {
                g_sprite_batch_root_signature_test_hook(
                    0, static_description);
            }
#endif
            rootSignatureStatic.Reset();
            CreateRootSignature(
                device, static_description, rootSignatureStatic);

            D3D12_DESCRIPTOR_RANGE sampler_range = {};
            sampler_range.RangeType =
                D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            sampler_range.NumDescriptors = 1;
            sampler_range.BaseShaderRegister = 0;
            sampler_range.RegisterSpace = 0;
            sampler_range.OffsetInDescriptorsFromTableStart =
                D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            D3D12_ROOT_PARAMETER heap_parameters[3] = {};
            heap_parameters[0] = static_parameters[0];
            heap_parameters[1] = static_parameters[1];
            heap_parameters[2].ParameterType =
                D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            heap_parameters[2].DescriptorTable.NumDescriptorRanges = 1;
            heap_parameters[2].DescriptorTable.pDescriptorRanges =
                &sampler_range;
            heap_parameters[2].ShaderVisibility =
                D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_ROOT_SIGNATURE_DESC heap_description = {};
            heap_description.NumParameters = 3;
            heap_description.pParameters = heap_parameters;
            heap_description.Flags = signature_flags;
#if defined(JPB_D3DAPP_TESTING)
            if (g_sprite_batch_root_signature_test_hook != nullptr) {
                g_sprite_batch_root_signature_test_hook(
                    1, heap_description);
            }
#endif
            rootSignatureHeap.Reset();
            CreateRootSignature(
                device, heap_description, rootSignatureHeap);
        }

        Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureStatic;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureHeap;
        ID3D12Device *mDevice;
    };

#if defined(JPB_D3DAPP_TESTING)
    static std::shared_ptr<DeviceResources> &TestDeviceResources()
    {
        static std::shared_ptr<DeviceResources> resources;
        return resources;
    }
#endif

    static std::shared_ptr<DeviceResources> DemandCreateDeviceResources(
        ID3D12Device *device,
        DirectX::ResourceUploadBatch &upload)
    {
#if defined(JPB_D3DAPP_TESTING)
        if (TestDeviceResources() != nullptr) {
            return TestDeviceResources();
        }
#endif
        static std::mutex pool_mutex;
        static std::map<
            ID3D12Device *,
            std::weak_ptr<DeviceResources>> pool;
        std::lock_guard<std::mutex> lock(pool_mutex);
        const auto found = pool.find(device);
        if (found != pool.end()) {
            std::shared_ptr<DeviceResources> existing =
                found->second.lock();
            if (existing != nullptr) {
                return existing;
            }
            pool.erase(found);
        }

        std::shared_ptr<DeviceResources> created =
            std::make_shared<DeviceResources>(device, upload);
        pool.emplace(device, created);
        return created;
    }

#if defined(JPB_D3DAPP_TESTING)
    Impl() noexcept
        : mRotation(DXGI_MODE_ROTATION_UNSPECIFIED),
          mSetViewport(false),
          padding_005 {},
          mViewPort {},
          mSampler {},
          mSpriteQueue(),
          mSpriteQueueCount(0),
          mSpriteQueueArraySize(0),
          mSortedSprites(),
          mInBeginEndPair(false),
          padding_089 {},
          mSortMode(SpriteSortMode_Deferred),
          mPSO(),
          mRootSignature(),
          mTransformMatrix(DirectX::XMMatrixIdentity()),
          mCommandList(),
          mVertexSegment(),
          mVertexPageSize(0x48000),
          mSpriteCount(0),
          mConstantBuffer(),
          mDeviceResources()
    {
    }
#endif

    Impl(
        ID3D12Device *device,
        DirectX::ResourceUploadBatch &upload,
        const SpriteBatchPipelineStateDescription &pipeline_description,
        const D3D12_VIEWPORT *viewport)
        : mRotation(DXGI_MODE_ROTATION_IDENTITY),
          mSetViewport(false),
          padding_005 {},
          mViewPort {},
          mSampler {},
          mSpriteQueue(),
          mSpriteQueueCount(0),
          mSpriteQueueArraySize(0),
          mSortedSprites(),
          mInBeginEndPair(false),
          padding_089 {},
          mSortMode(SpriteSortMode_Deferred),
          mPSO(),
          mRootSignature(),
          mTransformMatrix(DirectX::XMMatrixIdentity()),
          mCommandList(),
          mVertexSegment(),
          mVertexPageSize(0x48000),
          mSpriteCount(0),
          mConstantBuffer(),
          mDeviceResources(DemandCreateDeviceResources(device, upload))
    {
        if (viewport != nullptr) {
            mViewPort = *viewport;
            mSetViewport = true;
        }

        static const D3D12_INPUT_ELEMENT_DESC input_elements[3] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
             0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
             0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
             0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC description = {};
        description.InputLayout.pInputElementDescs = input_elements;
        description.InputLayout.NumElements = 3;
        description.BlendState = pipeline_description.blendDesc;
        description.DepthStencilState =
            pipeline_description.depthStencilDesc;
        description.RasterizerState =
            pipeline_description.rasterizerDesc;
        description.SampleMask =
            pipeline_description.renderTargetState.sampleMask;
        description.IBStripCutValue =
            D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        description.PrimitiveTopologyType =
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        description.NumRenderTargets =
            pipeline_description.renderTargetState.numRenderTargets;
        std::copy_n(
            pipeline_description.renderTargetState.rtvFormats,
            8,
            description.RTVFormats);
        description.DSVFormat =
            pipeline_description.renderTargetState.dsvFormat;
        description.SampleDesc =
            pipeline_description.renderTargetState.sampleDesc;
        description.NodeMask =
            pipeline_description.renderTargetState.nodeMask;

        if (pipeline_description.customRootSignature != nullptr) {
            mRootSignature =
                pipeline_description.customRootSignature;
        } else if (pipeline_description.samplerDescriptor.ptr != 0) {
            mRootSignature = mDeviceResources->rootSignatureHeap;
        } else {
            mRootSignature = mDeviceResources->rootSignatureStatic;
        }
        description.pRootSignature = mRootSignature.Get();

        const D3D12_SHADER_BYTECODE default_vertex_shader =
            pipeline_description.samplerDescriptor.ptr != 0
            ? D3D12_SHADER_BYTECODE {
                  SpriteBatchHeapVertexShader,
                  sizeof(SpriteBatchHeapVertexShader)}
            : D3D12_SHADER_BYTECODE {
                  SpriteBatchStaticVertexShader,
                  sizeof(SpriteBatchStaticVertexShader)};
        const D3D12_SHADER_BYTECODE default_pixel_shader =
            pipeline_description.samplerDescriptor.ptr != 0
            ? D3D12_SHADER_BYTECODE {
                  SpriteBatchHeapPixelShader,
                  sizeof(SpriteBatchHeapPixelShader)}
            : D3D12_SHADER_BYTECODE {
                  SpriteBatchStaticPixelShader,
                  sizeof(SpriteBatchStaticPixelShader)};
        description.VS =
            pipeline_description.customVertexShader.pShaderBytecode !=
                    nullptr
            ? pipeline_description.customVertexShader
            : default_vertex_shader;
        description.PS =
            pipeline_description.customPixelShader.pShaderBytecode !=
                    nullptr
            ? pipeline_description.customPixelShader
            : default_pixel_shader;

        if (pipeline_description.samplerDescriptor.ptr != 0) {
            mSampler = pipeline_description.samplerDescriptor;
        }

        ThrowIfFailed(device->CreateGraphicsPipelineState(
            &description,
            IID_PPV_ARGS(mPSO.ReleaseAndGetAddressOf())));
    }

    void XM_CALLCONV Begin(
        ID3D12GraphicsCommandList *command_list,
        SpriteSortMode sort_mode,
        DirectX::FXMMATRIX transform_matrix)
    {
        if (mInBeginEndPair) {
            throw std::logic_error("SpriteBatch::Begin");
        }

        mTransformMatrix = transform_matrix;
        mSortMode = sort_mode;
        mCommandList = command_list;
        mSpriteCount = 0;
        if (sort_mode == SpriteSortMode_Immediate) {
            PrepareForRendering();
        }
        mInBeginEndPair = true;
    }

    void End()
    {
        if (!mInBeginEndPair) {
            throw std::logic_error("SpriteBatch::End");
        }

        if (mSortMode != SpriteSortMode_Immediate) {
            PrepareForRendering();
            FlushBatch();
        }
        mVertexSegment.Reset();
        mCommandList.Reset();
        mInBeginEndPair = false;
    }

    DirectX::XMMATRIX GetViewportTransform(
        DXGI_MODE_ROTATION rotation) const
    {
        if (!mSetViewport) {
            throw std::runtime_error("Viewport not set.");
        }

        const float x_scale = mViewPort.Width > 0.0f
            ? 2.0f / mViewPort.Width
            : 0.0f;
        const float y_scale = mViewPort.Height > 0.0f
            ? 2.0f / mViewPort.Height
            : 0.0f;

        switch (rotation) {
        case DXGI_MODE_ROTATION_ROTATE90:
            return DirectX::XMMatrixSet(
                0.0f, -y_scale, 0.0f, 0.0f,
                -x_scale, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 0.0f, 1.0f);

        case DXGI_MODE_ROTATION_ROTATE180:
            return DirectX::XMMatrixSet(
                -x_scale, 0.0f, 0.0f, 0.0f,
                0.0f, y_scale, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                1.0f, -1.0f, 0.0f, 1.0f);

        case DXGI_MODE_ROTATION_ROTATE270:
            return DirectX::XMMatrixSet(
                0.0f, y_scale, 0.0f, 0.0f,
                x_scale, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                -1.0f, -1.0f, 0.0f, 1.0f);

        default:
            return DirectX::XMMatrixSet(
                x_scale, 0.0f, 0.0f, 0.0f,
                0.0f, -y_scale, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                -1.0f, 1.0f, 0.0f, 1.0f);
        }
    }

    void PrepareForRendering()
    {
        mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
        mCommandList->SetPipelineState(mPSO.Get());
        mCommandList->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mCommandList->IASetIndexBuffer(
            &mDeviceResources->indexBufferView);

        DirectX::XMMATRIX transform = mTransformMatrix;
        if (mRotation != DXGI_MODE_ROTATION_UNSPECIFIED) {
            transform = DirectX::XMMatrixMultiply(
                mTransformMatrix,
                GetViewportTransform(mRotation));
        }

        GraphicsResource allocation = GraphicsMemory::Get(
            mDeviceResources->mDevice).Allocate(256, 256);
        std::memcpy(allocation.Memory(), &transform, sizeof(transform));
        mConstantBuffer = std::move(allocation);
        mCommandList->SetGraphicsRootConstantBufferView(
            1, mConstantBuffer.GpuAddress());
    }

    SpriteInfo *BuildSpriteRecord(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        const DirectX::XMUINT2 &texture_size,
        DirectX::FXMVECTOR destination,
        const RECT *source_rectangle,
        DirectX::FXMVECTOR color,
        DirectX::GXMVECTOR origin_rotation_depth,
        std::uint32_t flags)
    {
        if (!mInBeginEndPair) {
            throw std::logic_error("SpriteBatch::Draw");
        }
        if (texture.ptr == 0) {
            throw std::invalid_argument("Invalid texture for Draw");
        }
        if (mSpriteQueueCount >= mSpriteQueueArraySize) {
            GrowSpriteQueue();
        }

        SpriteInfo *sprite = &mSpriteQueue[mSpriteQueueCount];
        DirectX::XMVECTOR adjusted_destination = destination;
        if (source_rectangle != nullptr) {
            const DirectX::XMVECTOR source = DirectX::XMVectorSet(
                static_cast<float>(source_rectangle->left),
                static_cast<float>(source_rectangle->top),
                static_cast<float>(source_rectangle->right -
                                   source_rectangle->left),
                static_cast<float>(source_rectangle->bottom -
                                   source_rectangle->top));
            sprite->source = source;
            if ((flags & 8u) == 0) {
                adjusted_destination = DirectX::XMVectorSet(
                    DirectX::XMVectorGetX(adjusted_destination),
                    DirectX::XMVectorGetY(adjusted_destination),
                    DirectX::XMVectorGetZ(adjusted_destination) *
                        DirectX::XMVectorGetZ(source),
                    DirectX::XMVectorGetW(adjusted_destination) *
                        DirectX::XMVectorGetW(source));
            }
            flags |= 0x0Cu;
        } else {
            sprite->source = DirectX::XMVectorSet(
                0.0f, 0.0f, 1.0f, 1.0f);
        }

        sprite->destination = adjusted_destination;
        sprite->color = color;
        sprite->originRotationDepth = origin_rotation_depth;
        sprite->texture = texture;
        sprite->textureSize = DirectX::XMLoadUInt2(&texture_size);
        sprite->flags = flags;
        return sprite;
    }

    static void RenderSprite(
        const SpriteInfo *sprite,
        VertexPositionColorTexture *vertices,
        DirectX::FXMVECTOR texture_size,
        DirectX::GXMVECTOR inverse_texture_size)
    {
        static const DirectX::XMVECTORF32 corner_offsets[4] = {
            {{0.0f, 0.0f, 0.0f, 0.0f}},
            {{1.0f, 0.0f, 0.0f, 0.0f}},
            {{0.0f, 1.0f, 0.0f, 0.0f}},
            {{1.0f, 1.0f, 0.0f, 0.0f}},
        };

        DirectX::XMVECTOR source = sprite->source;
        DirectX::XMVECTOR destination = sprite->destination;
        DirectX::XMVECTOR source_size =
            DirectX::XMVectorSwizzle<2, 3, 2, 3>(source);
        DirectX::XMVECTOR destination_size =
            DirectX::XMVectorSwizzle<2, 3, 2, 3>(destination);
        const DirectX::XMVECTOR nonzero_source_size = DirectX::XMVectorSelect(
            source_size,
            DirectX::g_XMEpsilon,
            DirectX::XMVectorEqual(source_size, DirectX::g_XMZero));
        DirectX::XMVECTOR origin = DirectX::XMVectorDivide(
            sprite->originRotationDepth, nonzero_source_size);

        if ((sprite->flags & 4u) != 0) {
            source = DirectX::XMVectorMultiply(
                source, inverse_texture_size);
            source_size = DirectX::XMVectorMultiply(
                source_size, inverse_texture_size);
        } else {
            origin = DirectX::XMVectorMultiply(
                origin, inverse_texture_size);
        }
        if ((sprite->flags & 8u) == 0) {
            destination_size = DirectX::XMVectorMultiply(
                destination_size, texture_size);
        }

        DirectX::XMVECTOR rotation_row_0 = DirectX::g_XMIdentityR0;
        DirectX::XMVECTOR rotation_row_1 = DirectX::g_XMIdentityR1;
        const float rotation = DirectX::XMVectorGetZ(
            sprite->originRotationDepth);
        if (rotation != 0.0f) {
            float sine;
            float cosine;
            DirectX::XMScalarSinCos(&sine, &cosine, rotation);
            rotation_row_0 = DirectX::XMVectorSet(
                cosine, sine, 0.0f, 0.0f);
            rotation_row_1 = DirectX::XMVectorSet(
                -sine, cosine, 0.0f, 0.0f);
        }

        const std::uint32_t effects = sprite->flags & 3u;
        for (std::uint32_t index = 0; index < 4; ++index) {
            DirectX::XMVECTOR corner = DirectX::XMVectorSubtract(
                corner_offsets[index], origin);
            corner = DirectX::XMVectorMultiply(corner, destination_size);
            DirectX::XMVECTOR position = DirectX::XMVectorMultiplyAdd(
                DirectX::XMVectorSplatX(corner),
                rotation_row_0,
                destination);
            position = DirectX::XMVectorMultiplyAdd(
                DirectX::XMVectorSplatY(corner),
                rotation_row_1,
                position);
            position = DirectX::XMVectorPermute<0, 1, 7, 6>(
                position, sprite->originRotationDepth);
            DirectX::XMStoreFloat3(&vertices[index].position, position);
            DirectX::XMStoreFloat4(&vertices[index].color, sprite->color);

            const DirectX::XMVECTOR texture_coordinate =
                DirectX::XMVectorMultiplyAdd(
                    source_size,
                    corner_offsets[effects ^ index],
                    source);
            DirectX::XMStoreFloat2(
                &vertices[index].textureCoordinate,
                texture_coordinate);
        }
    }

    void RenderBatch(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        DirectX::FXMVECTOR texture_size,
        const SpriteInfo *const *sprites,
        std::size_t count)
    {
        mCommandList->SetGraphicsRootDescriptorTable(0, texture);
        if (mSampler.ptr != 0) {
            mCommandList->SetGraphicsRootDescriptorTable(2, mSampler);
        }

        const DirectX::XMVECTOR inverse_texture_size =
            DirectX::XMVectorReciprocal(texture_size);
        while (count != 0) {
            std::size_t batch_size = count;
            const std::size_t remaining_in_segment =
                2048 - mSpriteCount;
            if (batch_size > remaining_in_segment) {
                if (remaining_in_segment < 128) {
                    mSpriteCount = 0;
                    batch_size = (std::min)(
                        count, static_cast<std::size_t>(2048));
                } else {
                    batch_size = remaining_in_segment;
                }
            }

            if (mSpriteCount == 0) {
                GraphicsResource allocation = GraphicsMemory::Get(
                    mDeviceResources->mDevice).Allocate(
                        mVertexPageSize, 16);
                mVertexSegment = std::move(allocation);
            }

            auto *vertices = static_cast<VertexPositionColorTexture *>(
                mVertexSegment.Memory()) + mSpriteCount * 4;
            for (std::size_t index = 0; index < batch_size; ++index) {
                RenderSprite(
                    sprites[index],
                    vertices + index * 4,
                    texture_size,
                    inverse_texture_size);
            }

            D3D12_VERTEX_BUFFER_VIEW view = {};
            view.BufferLocation = mVertexSegment.GpuAddress() +
                                  mSpriteCount * 4 *
                                      sizeof(VertexPositionColorTexture);
            view.SizeInBytes = static_cast<UINT>(
                batch_size * 4 * sizeof(VertexPositionColorTexture));
            view.StrideInBytes = sizeof(VertexPositionColorTexture);
            mCommandList->IASetVertexBuffers(0, 1, &view);
            mCommandList->DrawIndexedInstanced(
                static_cast<UINT>(batch_size * 6), 1, 0, 0, 0);

            mSpriteCount += batch_size;
            sprites += batch_size;
            count -= batch_size;
        }
    }

    void XM_CALLCONV Draw(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        const DirectX::XMUINT2 &texture_size,
        DirectX::FXMVECTOR destination,
        const RECT *source_rectangle,
        DirectX::GXMVECTOR color,
        DirectX::HXMVECTOR origin_rotation_depth,
        std::uint32_t flags)
    {
        SpriteInfo *sprite = BuildSpriteRecord(
            texture,
            texture_size,
            destination,
            source_rectangle,
            color,
            origin_rotation_depth,
            flags);
        if (mSortMode == SpriteSortMode_Immediate) {
            const SpriteInfo *sprite_pointer = sprite;
            RenderBatch(
                texture,
                sprite->textureSize,
                &sprite_pointer,
                1);
        } else {
            ++mSpriteQueueCount;
        }
    }

    void GrowSpriteQueue()
    {
        const std::size_t new_size = (std::max)(
            mSpriteQueueArraySize * 2,
            static_cast<std::size_t>(64));
        auto new_queue = std::make_unique<SpriteInfo[]>(new_size);
        std::copy_n(
            mSpriteQueue.get(), mSpriteQueueCount, new_queue.get());
        mSpriteQueue = std::move(new_queue);
        mSpriteQueueArraySize = new_size;
        mSortedSprites.clear();
    }

    void GrowSortedSprites()
    {
        const std::size_t previous_size = mSortedSprites.size();
        mSortedSprites.resize(mSpriteQueueCount);
        for (std::size_t index = previous_size;
             index < mSpriteQueueCount;
             ++index) {
            mSortedSprites[index] = &mSpriteQueue[index];
        }
    }

    void SortSprites()
    {
        GrowSortedSprites();
        auto first = mSortedSprites.begin();
        auto last = first + static_cast<std::ptrdiff_t>(mSpriteQueueCount);
        switch (mSortMode) {
        case SpriteSortMode_Texture:
            std::sort(first, last, [](const SpriteInfo *left,
                                      const SpriteInfo *right) {
                return left->texture.ptr < right->texture.ptr;
            });
            break;
        case SpriteSortMode_BackToFront:
            std::sort(first, last, [](const SpriteInfo *left,
                                      const SpriteInfo *right) {
                return left->originRotationDepth.m128_f32[3] >
                       right->originRotationDepth.m128_f32[3];
            });
            break;
        case SpriteSortMode_FrontToBack:
            std::sort(first, last, [](const SpriteInfo *left,
                                      const SpriteInfo *right) {
                return left->originRotationDepth.m128_f32[3] <
                       right->originRotationDepth.m128_f32[3];
            });
            break;
        default:
            break;
        }
    }

    void FlushBatch()
    {
        if (mSpriteQueueCount == 0) {
            return;
        }

        SortSprites();

        D3D12_GPU_DESCRIPTOR_HANDLE texture = {};
        DirectX::XMVECTOR texture_size = DirectX::g_XMZero;
        std::size_t batch_start = 0;
        for (std::size_t index = 0;
             index < mSpriteQueueCount;
             ++index) {
            const SpriteInfo *sprite = mSortedSprites[index];
            if (sprite->texture.ptr != texture.ptr) {
                if (index > batch_start) {
                    RenderBatch(
                        texture,
                        texture_size,
                        mSortedSprites.data_begin + batch_start,
                        index - batch_start);
                }
                texture = sprite->texture;
                texture_size = sprite->textureSize;
                batch_start = index;
            }
        }

        RenderBatch(
            texture,
            texture_size,
            mSortedSprites.data_begin + batch_start,
            mSpriteQueueCount - batch_start);
        mSpriteQueueCount = 0;
        if (mSortMode != SpriteSortMode_Deferred) {
            mSortedSprites.clear();
        }
    }

    DXGI_MODE_ROTATION mRotation;
    bool mSetViewport;
    unsigned char padding_005[3];
    D3D12_VIEWPORT mViewPort;
    D3D12_GPU_DESCRIPTOR_HANDLE mSampler;
    std::unique_ptr<SpriteInfo[]> mSpriteQueue;
    std::size_t mSpriteQueueCount;
    std::size_t mSpriteQueueArraySize;
    SpritePointerVector mSortedSprites;
    bool mInBeginEndPair;
    unsigned char padding_089[3];
    SpriteSortMode mSortMode;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
    DirectX::XMMATRIX mTransformMatrix;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
    GraphicsResource mVertexSegment;
    std::size_t mVertexPageSize;
    std::size_t mSpriteCount;
    GraphicsResource mConstantBuffer;
    std::shared_ptr<DeviceResources> mDeviceResources;
};

static_assert(sizeof(SpriteBatch::Impl::SpriteInfo) == 112,
              "SpriteBatch::SpriteInfo PDB size changed");
static_assert(offsetof(SpriteBatch::Impl::SpriteInfo, source) == 0,
              "SpriteBatch::SpriteInfo source offset changed");
static_assert(offsetof(SpriteBatch::Impl::SpriteInfo, texture) == 64,
              "SpriteBatch::SpriteInfo texture offset changed");
static_assert(offsetof(SpriteBatch::Impl::SpriteInfo, textureSize) == 80,
              "SpriteBatch::SpriteInfo texture-size offset changed");
static_assert(offsetof(SpriteBatch::Impl::SpriteInfo, flags) == 96,
              "SpriteBatch::SpriteInfo flags offset changed");
static_assert(sizeof(SpriteBatch::Impl) == 320,
              "SpriteBatch::Impl PDB size changed");
static_assert(offsetof(SpriteBatch::Impl, mViewPort) == 8,
              "SpriteBatch::Impl viewport offset changed");
static_assert(offsetof(SpriteBatch::Impl, mSampler) == 32,
              "SpriteBatch::Impl sampler offset changed");
static_assert(offsetof(SpriteBatch::Impl, mSpriteQueue) == 40,
              "SpriteBatch::Impl queue offset changed");
static_assert(offsetof(SpriteBatch::Impl, mSortedSprites) == 64,
              "SpriteBatch::Impl sorted queue offset changed");
static_assert(offsetof(SpriteBatch::Impl, mInBeginEndPair) == 88,
              "SpriteBatch::Impl begin/end flag offset changed");
static_assert(offsetof(SpriteBatch::Impl, mTransformMatrix) == 112,
              "SpriteBatch::Impl transform offset changed");
static_assert(offsetof(SpriteBatch::Impl, mCommandList) == 176,
              "SpriteBatch::Impl command-list offset changed");
static_assert(offsetof(SpriteBatch::Impl, mVertexSegment) == 184,
              "SpriteBatch::Impl vertex segment offset changed");
static_assert(offsetof(SpriteBatch::Impl, mConstantBuffer) == 248,
              "SpriteBatch::Impl constant-buffer offset changed");
static_assert(offsetof(SpriteBatch::Impl, mDeviceResources) == 296,
              "SpriteBatch::Impl device-resources offset changed");
static_assert(sizeof(SpriteBatch::Impl::DeviceResources) == 48,
              "SpriteBatch::DeviceResources PDB size changed");
static_assert(offsetof(SpriteBatch::Impl::DeviceResources,
                       indexBufferView) == 8,
              "SpriteBatch index-view offset changed");
static_assert(offsetof(SpriteBatch::Impl::DeviceResources,
                       rootSignatureStatic) == 24,
              "SpriteBatch static-root offset changed");
static_assert(offsetof(SpriteBatch::Impl::DeviceResources,
                       rootSignatureHeap) == 32,
              "SpriteBatch heap-root offset changed");
static_assert(offsetof(SpriteBatch::Impl::DeviceResources,
                       mDevice) == 40,
              "SpriteBatch device offset changed");

SpriteBatch::SpriteBatch(
    ID3D12Device *device,
    DirectX::ResourceUploadBatch &upload,
    const SpriteBatchPipelineStateDescription &pipeline_description,
    const D3D12_VIEWPORT *viewport)
    : pImpl(std::make_unique<Impl>(
          device,
          upload,
          pipeline_description,
          viewport))
{
}

SpriteBatch::SpriteBatch(SpriteBatch &&other) noexcept = default;

SpriteBatch &SpriteBatch::operator=(SpriteBatch &&other) noexcept = default;

SpriteBatch::~SpriteBatch() = default;

void XM_CALLCONV SpriteBatch::Begin(
    ID3D12GraphicsCommandList *command_list,
    D3D12_GPU_DESCRIPTOR_HANDLE sampler,
    SpriteSortMode sort_mode,
    DirectX::FXMMATRIX transform_matrix)
{
    if (sampler.ptr == 0) {
        throw std::invalid_argument(
            "Invalid heap-based sampler for Begin");
    }
    if (pImpl->mRootSignature == nullptr) {
        throw std::runtime_error("SpriteBatch::Begin");
    }

    pImpl->mSampler = sampler;
    pImpl->Begin(command_list, sort_mode, transform_matrix);
}

void XM_CALLCONV SpriteBatch::Begin(
    ID3D12GraphicsCommandList *command_list,
    SpriteSortMode sort_mode,
    DirectX::FXMMATRIX transform_matrix)
{
    pImpl->Begin(command_list, sort_mode, transform_matrix);
}

void SpriteBatch::End()
{
    pImpl->End();
}

void XM_CALLCONV SpriteBatch::Draw(
    D3D12_GPU_DESCRIPTOR_HANDLE texture,
    const DirectX::XMUINT2 &texture_size,
    const DirectX::XMFLOAT2 &position,
    const RECT *source_rectangle,
    DirectX::FXMVECTOR color,
    float rotation,
    const DirectX::XMFLOAT2 &origin,
    const DirectX::XMFLOAT2 &scale,
    SpriteEffects effects,
    float layer_depth)
{
    const DirectX::XMVECTOR destination = DirectX::XMVectorSet(
        position.x, position.y, scale.x, scale.y);
    const DirectX::XMVECTOR origin_rotation_depth = DirectX::XMVectorSet(
        origin.x, origin.y, rotation, layer_depth);
    pImpl->Draw(
        texture,
        texture_size,
        destination,
        source_rectangle,
        color,
        origin_rotation_depth,
        static_cast<std::uint32_t>(effects));
}

void XM_CALLCONV SpriteBatch::Draw(
    D3D12_GPU_DESCRIPTOR_HANDLE texture,
    const DirectX::XMUINT2 &texture_size,
    const DirectX::XMFLOAT2 &position,
    const RECT *source_rectangle,
    DirectX::FXMVECTOR color,
    float rotation,
    const DirectX::XMFLOAT2 &origin,
    float scale,
    SpriteEffects effects,
    float layer_depth)
{
    const DirectX::XMVECTOR destination = DirectX::XMVectorSet(
        position.x, position.y, scale, scale);
    const DirectX::XMVECTOR origin_rotation_depth = DirectX::XMVectorSet(
        origin.x, origin.y, rotation, layer_depth);
    pImpl->Draw(
        texture,
        texture_size,
        destination,
        source_rectangle,
        color,
        origin_rotation_depth,
        static_cast<std::uint32_t>(effects));
}

void XM_CALLCONV SpriteBatch::Draw(
    D3D12_GPU_DESCRIPTOR_HANDLE texture,
    const DirectX::XMUINT2 &texture_size,
    const DirectX::XMFLOAT2 &position,
    DirectX::FXMVECTOR color)
{
    pImpl->Draw(
        texture,
        texture_size,
        DirectX::XMVectorSet(position.x, position.y, 1.0f, 1.0f),
        nullptr,
        color,
        DirectX::g_XMZero,
        0);
}

void XM_CALLCONV SpriteBatch::Draw(
    D3D12_GPU_DESCRIPTOR_HANDLE texture,
    const DirectX::XMUINT2 &texture_size,
    const RECT &destination_rectangle,
    const RECT *source_rectangle,
    DirectX::FXMVECTOR color,
    float rotation,
    const DirectX::XMFLOAT2 &origin,
    SpriteEffects effects,
    float layer_depth)
{
    const DirectX::XMVECTOR destination = DirectX::XMVectorSet(
        static_cast<float>(destination_rectangle.left),
        static_cast<float>(destination_rectangle.top),
        static_cast<float>(destination_rectangle.right -
                           destination_rectangle.left),
        static_cast<float>(destination_rectangle.bottom -
                           destination_rectangle.top));
    const DirectX::XMVECTOR origin_rotation_depth = DirectX::XMVectorSet(
        origin.x, origin.y, rotation, layer_depth);
    pImpl->Draw(
        texture,
        texture_size,
        destination,
        source_rectangle,
        color,
        origin_rotation_depth,
        static_cast<std::uint32_t>(effects) | 8u);
}

void XM_CALLCONV SpriteBatch::Draw(
    D3D12_GPU_DESCRIPTOR_HANDLE texture,
    const DirectX::XMUINT2 &texture_size,
    const RECT &destination_rectangle,
    DirectX::FXMVECTOR color)
{
    const DirectX::XMVECTOR destination = DirectX::XMVectorSet(
        static_cast<float>(destination_rectangle.left),
        static_cast<float>(destination_rectangle.top),
        static_cast<float>(destination_rectangle.right -
                           destination_rectangle.left),
        static_cast<float>(destination_rectangle.bottom -
                           destination_rectangle.top));
    pImpl->Draw(
        texture,
        texture_size,
        destination,
        nullptr,
        color,
        DirectX::g_XMZero,
        8u);
}

void XM_CALLCONV SpriteBatch::Draw(
    D3D12_GPU_DESCRIPTOR_HANDLE texture,
    const DirectX::XMUINT2 &texture_size,
    DirectX::FXMVECTOR position,
    DirectX::GXMVECTOR color)
{
    const DirectX::XMVECTOR destination = DirectX::XMVectorSelect(
        DirectX::g_XMOne,
        position,
        DirectX::g_XMSelect1100);
    pImpl->Draw(
        texture,
        texture_size,
        destination,
        nullptr,
        color,
        DirectX::g_XMZero,
        0);
}

void XM_CALLCONV SpriteBatch::Draw(
    D3D12_GPU_DESCRIPTOR_HANDLE texture,
    const DirectX::XMUINT2 &texture_size,
    DirectX::FXMVECTOR position,
    const RECT *source_rectangle,
    DirectX::GXMVECTOR color,
    float rotation,
    DirectX::HXMVECTOR origin,
    DirectX::CXMVECTOR scale,
    SpriteEffects effects,
    float layer_depth)
{
    const DirectX::XMVECTOR destination =
        DirectX::XMVectorPermute<0, 1, 4, 5>(position, scale);
    const DirectX::XMVECTOR origin_rotation_depth =
        DirectX::XMVectorSetW(
            DirectX::XMVectorSetZ(origin, rotation),
            layer_depth);
    pImpl->Draw(
        texture,
        texture_size,
        destination,
        source_rectangle,
        color,
        origin_rotation_depth,
        static_cast<std::uint32_t>(effects));
}

void XM_CALLCONV SpriteBatch::Draw(
    D3D12_GPU_DESCRIPTOR_HANDLE texture,
    const DirectX::XMUINT2 &texture_size,
    DirectX::FXMVECTOR position,
    const RECT *source_rectangle,
    DirectX::GXMVECTOR color,
    float rotation,
    DirectX::HXMVECTOR origin,
    float scale,
    SpriteEffects effects,
    float layer_depth)
{
    const DirectX::XMVECTOR destination = DirectX::XMVectorSelect(
        DirectX::XMVectorReplicate(scale),
        position,
        DirectX::g_XMSelect1100);
    const DirectX::XMVECTOR origin_rotation_depth =
        DirectX::XMVectorSetW(
            DirectX::XMVectorSetZ(origin, rotation),
            layer_depth);
    pImpl->Draw(
        texture,
        texture_size,
        destination,
        source_rectangle,
        color,
        origin_rotation_depth,
        static_cast<std::uint32_t>(effects));
}

DXGI_MODE_ROTATION SpriteBatch::GetRotation() const
{
    return pImpl->mRotation;
}

void SpriteBatch::SetRotation(DXGI_MODE_ROTATION rotation)
{
    pImpl->mRotation = rotation;
}

void SpriteBatch::SetViewport(const D3D12_VIEWPORT &viewport)
{
    pImpl->mSetViewport = true;
    pImpl->mViewPort = viewport;
}

std::size_t jpb_directxtk12_test_sprite_queue(
    SpriteSortMode mode,
    const JPBSpriteBatchSortRecord *records,
    std::size_t count,
    std::size_t *sorted_indices)
{
    SpriteBatch::Impl impl = {};
    impl.mSortMode = mode;
    for (std::size_t index = 0; index < count; ++index) {
        if (impl.mSpriteQueueCount >= impl.mSpriteQueueArraySize) {
            impl.GrowSpriteQueue();
        }
        auto &sprite = impl.mSpriteQueue[impl.mSpriteQueueCount++];
        sprite.texture.ptr = records[index].texture;
        sprite.originRotationDepth = DirectX::XMVectorSet(
            0.0f, 0.0f, 0.0f, records[index].layer_depth);
    }
    impl.SortSprites();
    for (std::size_t index = 0; index < count; ++index) {
        sorted_indices[index] = static_cast<std::size_t>(
            impl.mSortedSprites[index] - impl.mSpriteQueue.get());
    }
    return impl.mSpriteQueueArraySize;
}

void jpb_directxtk12_test_build_sprite_record(
    bool in_begin_end_pair,
    D3D12_GPU_DESCRIPTOR_HANDLE texture,
    const DirectX::XMUINT2 &texture_size,
    const DirectX::XMFLOAT4 &destination,
    const RECT *source_rectangle,
    const DirectX::XMFLOAT4 &color,
    const DirectX::XMFLOAT4 &origin_rotation_depth,
    std::uint32_t flags,
    JPBSpriteBatchDrawRecord *record)
{
    SpriteBatch::Impl impl = {};
    impl.mInBeginEndPair = in_begin_end_pair;
    const DirectX::XMVECTOR destination_vector =
        DirectX::XMLoadFloat4(&destination);
    const DirectX::XMVECTOR color_vector = DirectX::XMLoadFloat4(&color);
    const DirectX::XMVECTOR origin_vector =
        DirectX::XMLoadFloat4(&origin_rotation_depth);
    const SpriteBatch::Impl::SpriteInfo *sprite = impl.BuildSpriteRecord(
        texture,
        texture_size,
        destination_vector,
        source_rectangle,
        color_vector,
        origin_vector,
        flags);

    DirectX::XMStoreFloat4(&record->source, sprite->source);
    DirectX::XMStoreFloat4(&record->destination, sprite->destination);
    DirectX::XMStoreFloat4(&record->color, sprite->color);
    DirectX::XMStoreFloat4(
        &record->origin_rotation_depth,
        sprite->originRotationDepth);
    record->texture = sprite->texture.ptr;
    DirectX::XMStoreFloat4(&record->texture_size, sprite->textureSize);
    record->flags = sprite->flags;
}

void jpb_directxtk12_test_render_sprite(
    const JPBSpriteBatchDrawRecord &record,
    VertexPositionColorTexture *vertices)
{
    SpriteBatch::Impl::SpriteInfo sprite = {};
    sprite.source = DirectX::XMLoadFloat4(&record.source);
    sprite.destination = DirectX::XMLoadFloat4(&record.destination);
    sprite.color = DirectX::XMLoadFloat4(&record.color);
    sprite.originRotationDepth =
        DirectX::XMLoadFloat4(&record.origin_rotation_depth);
    sprite.texture.ptr = record.texture;
    sprite.textureSize = DirectX::XMLoadFloat4(&record.texture_size);
    sprite.flags = record.flags;
    SpriteBatch::Impl::RenderSprite(
        &sprite,
        vertices,
        sprite.textureSize,
        DirectX::XMVectorReciprocal(sprite.textureSize));
}

void jpb_directxtk12_test_viewport_transform(
    bool viewport_set,
    const D3D12_VIEWPORT &viewport,
    DXGI_MODE_ROTATION rotation,
    DirectX::XMFLOAT4X4 *transform)
{
    SpriteBatch::Impl impl = {};
    impl.mSetViewport = viewport_set;
    impl.mViewPort = viewport;
    DirectX::XMStoreFloat4x4(
        transform,
        impl.GetViewportTransform(rotation));
}

void jpb_directxtk12_test_public_draw_overloads(
    JPBSpriteBatchDrawRecord *records)
{
    SpriteBatch::Impl impl = {};
    impl.mInBeginEndPair = true;
    impl.mSortMode = SpriteSortMode_Deferred;

    alignas(SpriteBatch) unsigned char batch_storage[16] = {};
    SpriteBatch::Impl *impl_pointer = &impl;
    std::memcpy(
        batch_storage + 8,
        &impl_pointer,
        sizeof(impl_pointer));
    auto *batch = reinterpret_cast<SpriteBatch *>(batch_storage);

    const D3D12_GPU_DESCRIPTOR_HANDLE texture = {0x1234};
    const DirectX::XMUINT2 texture_size = {100, 50};
    const DirectX::XMFLOAT2 position = {10.0f, 20.0f};
    const RECT source = {2, 3, 12, 8};
    const DirectX::XMVECTOR color = DirectX::XMVectorSet(
        0.1f, 0.2f, 0.3f, 0.4f);
    const DirectX::XMFLOAT2 origin = {1.0f, 2.0f};
    const DirectX::XMFLOAT2 scale = {3.0f, 4.0f};
    const RECT destination = {10, 20, 40, 60};
    const DirectX::XMVECTOR vector_position = DirectX::XMVectorSet(
        10.0f, 20.0f, 99.0f, 88.0f);
    const DirectX::XMVECTOR vector_origin = DirectX::XMVectorSet(
        1.0f, 2.0f, 77.0f, 66.0f);
    const DirectX::XMVECTOR vector_scale = DirectX::XMVectorSet(
        3.0f, 4.0f, 55.0f, 44.0f);

    batch->Draw(
        texture, texture_size, position, &source, color,
        0.25f, origin, scale, SpriteEffects_FlipVertically, 0.75f);
    batch->Draw(
        texture, texture_size, position, &source, color,
        0.25f, origin, 5.0f, SpriteEffects_FlipVertically, 0.75f);
    batch->Draw(texture, texture_size, position, color);
    batch->Draw(
        texture, texture_size, destination, &source, color,
        0.25f, origin, SpriteEffects_FlipVertically, 0.75f);
    batch->Draw(texture, texture_size, destination, color);
    batch->Draw(texture, texture_size, vector_position, color);
    batch->Draw(
        texture, texture_size, vector_position, &source, color,
        0.25f, vector_origin, vector_scale,
        SpriteEffects_FlipVertically, 0.75f);
    batch->Draw(
        texture, texture_size, vector_position, &source, color,
        0.25f, vector_origin, 5.0f,
        SpriteEffects_FlipVertically, 0.75f);

    for (std::size_t index = 0; index < 8; ++index) {
        const SpriteBatch::Impl::SpriteInfo &sprite =
            impl.mSpriteQueue[index];
        DirectX::XMStoreFloat4(&records[index].source, sprite.source);
        DirectX::XMStoreFloat4(
            &records[index].destination, sprite.destination);
        DirectX::XMStoreFloat4(&records[index].color, sprite.color);
        DirectX::XMStoreFloat4(
            &records[index].origin_rotation_depth,
            sprite.originRotationDepth);
        records[index].texture = sprite.texture.ptr;
        DirectX::XMStoreFloat4(
            &records[index].texture_size, sprite.textureSize);
        records[index].flags = sprite.flags;
    }
}

std::size_t jpb_directxtk12_test_graphics_memory_pool(
    std::uint64_t size,
    std::uint64_t alignment,
    std::uint64_t *page_size)
{
    const std::size_t index = GetPoolIndexFromSize(
        NextPowerOfTwo(size + alignment));
    *page_size = GetPageSizeFromPoolIndex(index);
    return index;
}

void jpb_directxtk12_test_set_sprite_device_resources(
    ID3D12Device *device,
    ID3D12RootSignature *static_root_signature,
    ID3D12RootSignature *heap_root_signature,
    const D3D12_INDEX_BUFFER_VIEW &index_buffer_view)
{
    auto resources =
        std::make_shared<SpriteBatch::Impl::DeviceResources>();
    resources->mDevice = device;
    resources->rootSignatureStatic = static_root_signature;
    resources->rootSignatureHeap = heap_root_signature;
    resources->indexBufferView = index_buffer_view;
    SpriteBatch::Impl::TestDeviceResources() = std::move(resources);
}

void jpb_directxtk12_test_clear_sprite_device_resources()
{
    SpriteBatch::Impl::TestDeviceResources().reset();
}

void jpb_directxtk12_test_create_sprite_root_signatures(
    ID3D12Device *device,
    JPBSpriteBatchRootSignatureTestHook hook)
{
    SpriteBatch::Impl::DeviceResources resources;
    g_sprite_batch_root_signature_test_hook = hook;
    try {
        resources.CreateRootSignatures(device);
    } catch (...) {
        g_sprite_batch_root_signature_test_hook = nullptr;
        throw;
    }
    g_sprite_batch_root_signature_test_hook = nullptr;
}

std::size_t jpb_directxtk12_test_sprite_indices(
    std::int16_t *indices,
    std::size_t capacity)
{
    const std::vector<std::int16_t> values =
        SpriteBatch::Impl::DeviceResources::CreateIndexValues();
    std::copy_n(
        values.data(),
        (std::min)(capacity, values.size()),
        indices);
    return values.size();
}

const unsigned char *jpb_directxtk12_test_sprite_shader(
    std::size_t shader_index,
    std::size_t *size)
{
    switch (shader_index) {
    case 0:
        *size = sizeof(SpriteBatchStaticVertexShader);
        return SpriteBatchStaticVertexShader;
    case 1:
        *size = sizeof(SpriteBatchStaticPixelShader);
        return SpriteBatchStaticPixelShader;
    case 2:
        *size = sizeof(SpriteBatchHeapVertexShader);
        return SpriteBatchHeapVertexShader;
    case 3:
        *size = sizeof(SpriteBatchHeapPixelShader);
        return SpriteBatchHeapPixelShader;
    default:
        *size = 0;
        return nullptr;
    }
}

} // namespace DX12
} // namespace DirectX
