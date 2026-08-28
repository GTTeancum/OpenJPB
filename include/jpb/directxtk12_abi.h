#ifndef JPB_DIRECTXTK12_ABI_H
#define JPB_DIRECTXTK12_ABI_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <future>
#include <memory>

namespace DirectX {

class LinearAllocatorPage {
public:
    LinearAllocatorPage() noexcept;
    void AddRef() noexcept;
    void Release() noexcept;
    std::uint64_t Suballocate(
        std::uint64_t size,
        std::uint64_t alignment);
    unsigned long RefCount() const noexcept { return mRefCount; }

    LinearAllocatorPage *pPrevPage;
    LinearAllocatorPage *pNextPage;
    void *mMemory;
    std::uint64_t mPendingFence;
    std::uint64_t mGpuAddress;
    std::uint64_t mOffset;
    std::uint64_t mSize;
    ID3D12Resource *mUploadResource;
    volatile long mRefCount;
};

static_assert(sizeof(LinearAllocatorPage) == 72,
              "LinearAllocatorPage PDB size changed");
static_assert(offsetof(LinearAllocatorPage, mUploadResource) == 56,
              "LinearAllocatorPage resource offset changed");
static_assert(offsetof(LinearAllocatorPage, mRefCount) == 64,
              "LinearAllocatorPage reference offset changed");

class LinearAllocator {
public:
    LinearAllocator(
        ID3D12Device *device,
        std::uint64_t page_size,
        std::uint64_t preallocate_bytes);
    ~LinearAllocator();

    LinearAllocator(const LinearAllocator &) = delete;
    LinearAllocator &operator=(const LinearAllocator &) = delete;

    LinearAllocatorPage *FindPageForAlloc(
        std::uint64_t size,
        std::uint64_t alignment);
    void FenceCommittedPages(ID3D12CommandQueue *command_queue);
    void RetirePendingPages();
    void Shrink();

    std::uint64_t CommittedPageCount() const noexcept
    {
        return m_numPending;
    }
    std::uint64_t TotalPageCount() const noexcept { return m_totalPages; }
    std::uint64_t CommittedMemoryUsage() const noexcept
    {
        return m_numPending * m_increment;
    }
    std::uint64_t TotalMemoryUsage() const noexcept
    {
        return m_totalPages * m_increment;
    }
    std::uint64_t PageSize() const noexcept { return m_increment; }

    LinearAllocatorPage *m_pendingPages;
    LinearAllocatorPage *m_usedPages;
    LinearAllocatorPage *m_unusedPages;
    std::uint64_t m_increment;
    std::uint64_t m_numPending;
    std::uint64_t m_totalPages;
    std::uint64_t m_fenceCount;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

private:
    LinearAllocatorPage *FindPageForAlloc(
        LinearAllocatorPage *list,
        std::uint64_t size,
        std::uint64_t alignment);
    LinearAllocatorPage *GetCleanPageForAlloc();
    LinearAllocatorPage *GetNewPage();
    void LinkPage(
        LinearAllocatorPage *page,
        LinearAllocatorPage *&list);
    void LinkPageChain(
        LinearAllocatorPage *page,
        LinearAllocatorPage *&list);
    void UnlinkPage(LinearAllocatorPage *page);
    void ReleasePage(LinearAllocatorPage *page);
    void FreePages(LinearAllocatorPage *page);
};

static_assert(sizeof(LinearAllocator) == 72,
              "LinearAllocator PDB size changed");
static_assert(offsetof(LinearAllocator, m_increment) == 24,
              "LinearAllocator page-size offset changed");
static_assert(offsetof(LinearAllocator, m_numPending) == 32,
              "LinearAllocator pending-count offset changed");
static_assert(offsetof(LinearAllocator, m_totalPages) == 40,
              "LinearAllocator total-count offset changed");
static_assert(offsetof(LinearAllocator, m_fenceCount) == 48,
              "LinearAllocator fence-count offset changed");
static_assert(offsetof(LinearAllocator, m_device) == 56,
              "LinearAllocator device offset changed");
static_assert(offsetof(LinearAllocator, m_fence) == 64,
              "LinearAllocator fence offset changed");

class com_exception : public std::exception {
public:
    explicit com_exception(HRESULT result) noexcept;
    const char *what() const noexcept override;
    HRESULT get_result() const noexcept { return result; }

private:
    HRESULT result;
};

static_assert(sizeof(com_exception) == 32,
              "com_exception PDB size changed");

class ResourceUploadBatch {
public:
    struct Impl;

    explicit ResourceUploadBatch(ID3D12Device *device);
    ResourceUploadBatch(ResourceUploadBatch &&other) noexcept;
    virtual ~ResourceUploadBatch();
    ResourceUploadBatch &operator=(ResourceUploadBatch &&other) noexcept;

    void Begin(
        D3D12_COMMAND_LIST_TYPE command_type =
            D3D12_COMMAND_LIST_TYPE_DIRECT);
    std::future<void> End(ID3D12CommandQueue *command_queue);

    void Upload(
        ID3D12Resource *resource,
        UINT subresource_index_start,
        const D3D12_SUBRESOURCE_DATA *subresources,
        UINT num_subresources);
    void Transition(
        ID3D12Resource *resource,
        D3D12_RESOURCE_STATES state_before,
        D3D12_RESOURCE_STATES state_after);

private:
    ResourceUploadBatch(const ResourceUploadBatch &) = delete;
    ResourceUploadBatch &operator=(const ResourceUploadBatch &) = delete;

    std::unique_ptr<Impl> pImpl;
};

static_assert(sizeof(ResourceUploadBatch) == 16,
              "ResourceUploadBatch PDB size changed");

struct RenderTargetState {
    RenderTargetState() noexcept
        : sampleMask(UINT_MAX),
          numRenderTargets(0),
          rtvFormats {},
          dsvFormat(DXGI_FORMAT_UNKNOWN),
          sampleDesc {1, 0},
          nodeMask(0)
    {
    }

    RenderTargetState(
        DXGI_FORMAT render_target_format,
        DXGI_FORMAT depth_stencil_format) noexcept
        : sampleMask(UINT_MAX),
          numRenderTargets(1),
          rtvFormats {render_target_format},
          dsvFormat(depth_stencil_format),
          sampleDesc {1, 0},
          nodeMask(0)
    {
    }

    RenderTargetState(
        const DXGI_SWAP_CHAIN_DESC *swap_chain_description,
        DXGI_FORMAT depth_stencil_format) noexcept
        : sampleMask(UINT_MAX),
          numRenderTargets(1),
          rtvFormats {swap_chain_description->BufferDesc.Format},
          dsvFormat(depth_stencil_format),
          sampleDesc(swap_chain_description->SampleDesc),
          nodeMask(0)
    {
    }

    RenderTargetState(
        const DXGI_SWAP_CHAIN_DESC1 *swap_chain_description,
        DXGI_FORMAT depth_stencil_format) noexcept
        : sampleMask(UINT_MAX),
          numRenderTargets(1),
          rtvFormats {swap_chain_description->Format},
          dsvFormat(depth_stencil_format),
          sampleDesc(swap_chain_description->SampleDesc),
          nodeMask(0)
    {
    }

    RenderTargetState(const RenderTargetState &) noexcept = default;

    UINT sampleMask;
    UINT numRenderTargets;
    DXGI_FORMAT rtvFormats[8];
    DXGI_FORMAT dsvFormat;
    DXGI_SAMPLE_DESC sampleDesc;
    UINT nodeMask;
};

static_assert(sizeof(RenderTargetState) == 56,
              "RenderTargetState PDB size changed");
static_assert(offsetof(RenderTargetState, rtvFormats) == 8,
              "RenderTargetState RTV offset changed");
static_assert(offsetof(RenderTargetState, dsvFormat) == 40,
              "RenderTargetState DSV offset changed");
static_assert(offsetof(RenderTargetState, sampleDesc) == 44,
              "RenderTargetState sample offset changed");
static_assert(offsetof(RenderTargetState, nodeMask) == 52,
              "RenderTargetState node-mask offset changed");

class DescriptorHeap {
public:
    explicit DescriptorHeap(ID3D12DescriptorHeap *existing_heap);
    DescriptorHeap(
        ID3D12Device *device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        D3D12_DESCRIPTOR_HEAP_FLAGS flags,
        std::size_t count);
    ~DescriptorHeap() = default;

    DescriptorHeap(const DescriptorHeap &) = delete;
    DescriptorHeap &operator=(const DescriptorHeap &) = delete;

    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(std::size_t index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(std::size_t index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetFirstCpuHandle() const
    {
        return m_hCPU;
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GetFirstGpuHandle() const
    {
        return m_hGPU;
    }
    std::size_t Count() const { return m_desc.NumDescriptors; }
    D3D12_DESCRIPTOR_HEAP_FLAGS Flags() const { return m_desc.Flags; }
    D3D12_DESCRIPTOR_HEAP_TYPE Type() const { return m_desc.Type; }
    UINT Increment() const { return m_increment; }
    ID3D12DescriptorHeap *Heap() const { return m_pHeap.Get(); }

    /* Public so the reconstructed PDB layout remains directly auditable. */
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pHeap;
    D3D12_DESCRIPTOR_HEAP_DESC m_desc;
    D3D12_CPU_DESCRIPTOR_HANDLE m_hCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE m_hGPU;
    UINT m_increment;

private:
    void Create(
        ID3D12Device *device,
        const D3D12_DESCRIPTOR_HEAP_DESC *description);
};

static_assert(sizeof(DescriptorHeap) == 48,
              "DescriptorHeap PDB size changed");
static_assert(offsetof(DescriptorHeap, m_pHeap) == 0,
              "DescriptorHeap heap offset changed");
static_assert(offsetof(DescriptorHeap, m_desc) == 8,
              "DescriptorHeap description offset changed");
static_assert(offsetof(DescriptorHeap, m_hCPU) == 24,
              "DescriptorHeap CPU handle offset changed");
static_assert(offsetof(DescriptorHeap, m_hGPU) == 32,
              "DescriptorHeap GPU handle offset changed");
static_assert(offsetof(DescriptorHeap, m_increment) == 40,
              "DescriptorHeap increment offset changed");

namespace DX12 {

struct SpriteBatchPipelineStateDescription {
    SpriteBatchPipelineStateDescription(
        const DirectX::RenderTargetState &render_target_state,
        const D3D12_BLEND_DESC *blend_description = nullptr,
        const D3D12_DEPTH_STENCIL_DESC *depth_stencil_description = nullptr,
        const D3D12_RASTERIZER_DESC *rasterizer_description = nullptr,
        const D3D12_GPU_DESCRIPTOR_HANDLE *sampler_descriptor = nullptr)
        noexcept
        : blendDesc(
              blend_description != nullptr
                  ? *blend_description
                  : s_DefaultBlendDesc),
          depthStencilDesc(
              depth_stencil_description != nullptr
                  ? *depth_stencil_description
                  : s_DefaultDepthStencilDesc),
          rasterizerDesc(
              rasterizer_description != nullptr
                  ? *rasterizer_description
                  : s_DefaultRasterizerDesc),
          renderTargetState(render_target_state),
          samplerDescriptor(
              sampler_descriptor != nullptr
                  ? *sampler_descriptor
                  : D3D12_GPU_DESCRIPTOR_HANDLE {}),
          customRootSignature(nullptr),
          customVertexShader {},
          customPixelShader {}
    {
    }

    static const D3D12_BLEND_DESC s_DefaultBlendDesc;
    static const D3D12_RASTERIZER_DESC s_DefaultRasterizerDesc;
    static const D3D12_DEPTH_STENCIL_DESC s_DefaultDepthStencilDesc;

    D3D12_BLEND_DESC blendDesc;
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
    D3D12_RASTERIZER_DESC rasterizerDesc;
    DirectX::RenderTargetState renderTargetState;
    D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor;
    ID3D12RootSignature *customRootSignature;
    D3D12_SHADER_BYTECODE customVertexShader;
    D3D12_SHADER_BYTECODE customPixelShader;
};

static_assert(sizeof(SpriteBatchPipelineStateDescription) == 528,
              "SpriteBatch pipeline description PDB size changed");
static_assert(offsetof(
                  SpriteBatchPipelineStateDescription,
                  depthStencilDesc) == 328,
              "SpriteBatch depth-stencil offset changed");
static_assert(offsetof(
                  SpriteBatchPipelineStateDescription,
                  rasterizerDesc) == 380,
              "SpriteBatch rasterizer offset changed");
static_assert(offsetof(
                  SpriteBatchPipelineStateDescription,
                  renderTargetState) == 424,
              "SpriteBatch render-target offset changed");
static_assert(offsetof(
                  SpriteBatchPipelineStateDescription,
                  samplerDescriptor) == 480,
              "SpriteBatch sampler offset changed");
static_assert(offsetof(
                  SpriteBatchPipelineStateDescription,
                  customRootSignature) == 488,
              "SpriteBatch custom-root offset changed");
static_assert(offsetof(
                  SpriteBatchPipelineStateDescription,
                  customVertexShader) == 496,
              "SpriteBatch custom-VS offset changed");
static_assert(offsetof(
                  SpriteBatchPipelineStateDescription,
                  customPixelShader) == 512,
              "SpriteBatch custom-PS offset changed");

class GraphicsResource {
public:
    GraphicsResource() noexcept;
    GraphicsResource(GraphicsResource &&other) noexcept;
    GraphicsResource(
        LinearAllocatorPage *page,
        std::uint64_t gpu_address,
        ID3D12Resource *resource,
        void *memory,
        std::uint64_t offset,
        std::uint64_t size) noexcept;
    GraphicsResource &operator=(GraphicsResource &&other) noexcept;
    ~GraphicsResource();

    GraphicsResource(const GraphicsResource &) = delete;
    GraphicsResource &operator=(const GraphicsResource &) = delete;

    std::uint64_t GpuAddress() const noexcept { return mGpuAddress; }
    ID3D12Resource *Resource() const noexcept { return mResource; }
    void *Memory() const noexcept { return mMemory; }
    std::uint64_t ResourceOffset() const noexcept { return mBufferOffset; }
    std::uint64_t Size() const noexcept { return mSize; }
    explicit operator bool() const noexcept { return mPage != nullptr; }

    void Reset(GraphicsResource &&allocation) noexcept;
    void Reset() noexcept;

    LinearAllocatorPage *mPage;
    std::uint64_t mGpuAddress;
    ID3D12Resource *mResource;
    void *mMemory;
    std::uint64_t mBufferOffset;
    std::uint64_t mSize;
};

static_assert(sizeof(GraphicsResource) == 48,
              "GraphicsResource PDB size changed");
static_assert(offsetof(GraphicsResource, mPage) == 0,
              "GraphicsResource page offset changed");
static_assert(offsetof(GraphicsResource, mGpuAddress) == 8,
              "GraphicsResource GPU address offset changed");
static_assert(offsetof(GraphicsResource, mResource) == 16,
              "GraphicsResource resource offset changed");
static_assert(offsetof(GraphicsResource, mMemory) == 24,
              "GraphicsResource memory offset changed");
static_assert(offsetof(GraphicsResource, mBufferOffset) == 32,
              "GraphicsResource offset field changed");
static_assert(offsetof(GraphicsResource, mSize) == 40,
              "GraphicsResource size field changed");

struct GraphicsMemoryStatistics {
    std::uint64_t committedMemory;
    std::uint64_t totalMemory;
    std::uint64_t totalPages;
    std::uint64_t peakCommitedMemory;
    std::uint64_t peakTotalMemory;
    std::uint64_t peakTotalPages;
};

static_assert(sizeof(GraphicsMemoryStatistics) == 48,
              "GraphicsMemoryStatistics PDB size changed");

class GraphicsMemory {
public:
    struct Impl;

    explicit GraphicsMemory(ID3D12Device *device);
    GraphicsMemory(GraphicsMemory &&other) noexcept;
    GraphicsMemory &operator=(GraphicsMemory &&other) noexcept;
    virtual ~GraphicsMemory();

    GraphicsMemory(const GraphicsMemory &) = delete;
    GraphicsMemory &operator=(const GraphicsMemory &) = delete;

    GraphicsResource Allocate(
        std::uint64_t size,
        std::uint64_t alignment);
    void Commit(ID3D12CommandQueue *command_queue);
    void GarbageCollect();
    GraphicsMemoryStatistics GetStatistics();
    void ResetStatistics();

    static GraphicsMemory &Get(ID3D12Device *device = nullptr);

private:
    std::unique_ptr<Impl> pImpl;
};

static_assert(sizeof(GraphicsMemory) == 16,
              "GraphicsMemory PDB size changed");

enum SpriteSortMode : std::uint32_t {
    SpriteSortMode_Deferred = 0,
    SpriteSortMode_Immediate = 1,
    SpriteSortMode_Texture = 2,
    SpriteSortMode_BackToFront = 3,
    SpriteSortMode_FrontToBack = 4,
};

enum SpriteEffects : std::uint32_t {
    SpriteEffects_None = 0,
    SpriteEffects_FlipHorizontally = 1,
    SpriteEffects_FlipVertically = 2,
    SpriteEffects_FlipBoth = 3,
};

struct VertexPositionColorTexture {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 textureCoordinate;
};

static_assert(sizeof(VertexPositionColorTexture) == 36,
              "VertexPositionColorTexture PDB size changed");
static_assert(offsetof(VertexPositionColorTexture, color) == 12,
              "VertexPositionColorTexture color offset changed");
static_assert(offsetof(VertexPositionColorTexture, textureCoordinate) == 28,
              "VertexPositionColorTexture texture-coordinate offset changed");

class CommonStates {
public:
    explicit CommonStates(ID3D12Device *device);
    CommonStates(CommonStates &&other) noexcept;
    CommonStates &operator=(CommonStates &&other) noexcept;
    virtual ~CommonStates();

    D3D12_GPU_DESCRIPTOR_HANDLE PointWrap() const;
    D3D12_GPU_DESCRIPTOR_HANDLE PointClamp() const;
    D3D12_GPU_DESCRIPTOR_HANDLE LinearWrap() const;
    D3D12_GPU_DESCRIPTOR_HANDLE LinearClamp() const;
    D3D12_GPU_DESCRIPTOR_HANDLE AnisotropicWrap() const;
    D3D12_GPU_DESCRIPTOR_HANDLE AnisotropicClamp() const;
    ID3D12DescriptorHeap *Heap() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

static_assert(sizeof(CommonStates) == 16,
              "CommonStates PDB size changed");

class SpriteBatch {
public:
    struct Impl;

    SpriteBatch(
        ID3D12Device *device,
        DirectX::ResourceUploadBatch &upload,
        const SpriteBatchPipelineStateDescription &pipeline_description,
        const D3D12_VIEWPORT *viewport);
    SpriteBatch(SpriteBatch &&other) noexcept;
    SpriteBatch &operator=(SpriteBatch &&other) noexcept;
    virtual ~SpriteBatch();

    SpriteBatch(const SpriteBatch &) = delete;
    SpriteBatch &operator=(const SpriteBatch &) = delete;

    void XM_CALLCONV Begin(
        ID3D12GraphicsCommandList *command_list,
        D3D12_GPU_DESCRIPTOR_HANDLE sampler,
        SpriteSortMode sort_mode,
        DirectX::FXMMATRIX transform_matrix);
    void XM_CALLCONV Begin(
        ID3D12GraphicsCommandList *command_list,
        SpriteSortMode sort_mode,
        DirectX::FXMMATRIX transform_matrix);
    void End();

    void XM_CALLCONV Draw(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        const DirectX::XMUINT2 &texture_size,
        const DirectX::XMFLOAT2 &position,
        const RECT *source_rectangle,
        DirectX::FXMVECTOR color,
        float rotation,
        const DirectX::XMFLOAT2 &origin,
        const DirectX::XMFLOAT2 &scale,
        SpriteEffects effects,
        float layer_depth);
    void XM_CALLCONV Draw(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        const DirectX::XMUINT2 &texture_size,
        const DirectX::XMFLOAT2 &position,
        const RECT *source_rectangle,
        DirectX::FXMVECTOR color,
        float rotation,
        const DirectX::XMFLOAT2 &origin,
        float scale,
        SpriteEffects effects,
        float layer_depth);
    void XM_CALLCONV Draw(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        const DirectX::XMUINT2 &texture_size,
        const DirectX::XMFLOAT2 &position,
        DirectX::FXMVECTOR color);
    void XM_CALLCONV Draw(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        const DirectX::XMUINT2 &texture_size,
        const RECT &destination_rectangle,
        const RECT *source_rectangle,
        DirectX::FXMVECTOR color,
        float rotation,
        const DirectX::XMFLOAT2 &origin,
        SpriteEffects effects,
        float layer_depth);
    void XM_CALLCONV Draw(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        const DirectX::XMUINT2 &texture_size,
        const RECT &destination_rectangle,
        DirectX::FXMVECTOR color);
    void XM_CALLCONV Draw(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        const DirectX::XMUINT2 &texture_size,
        DirectX::FXMVECTOR position,
        DirectX::GXMVECTOR color);
    void XM_CALLCONV Draw(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        const DirectX::XMUINT2 &texture_size,
        DirectX::FXMVECTOR position,
        const RECT *source_rectangle,
        DirectX::GXMVECTOR color,
        float rotation,
        DirectX::HXMVECTOR origin,
        DirectX::CXMVECTOR scale,
        SpriteEffects effects,
        float layer_depth);
    void XM_CALLCONV Draw(
        D3D12_GPU_DESCRIPTOR_HANDLE texture,
        const DirectX::XMUINT2 &texture_size,
        DirectX::FXMVECTOR position,
        const RECT *source_rectangle,
        DirectX::GXMVECTOR color,
        float rotation,
        DirectX::HXMVECTOR origin,
        float scale,
        SpriteEffects effects,
        float layer_depth);

    DXGI_MODE_ROTATION GetRotation() const;
    void SetRotation(DXGI_MODE_ROTATION rotation);
    void SetViewport(const D3D12_VIEWPORT &viewport);

private:
    std::unique_ptr<Impl> pImpl;
};

static_assert(sizeof(SpriteBatch) == 16,
              "SpriteBatch PDB size changed");

struct JPBSpriteBatchSortRecord {
    std::uint64_t texture;
    float layer_depth;
};

struct JPBSpriteBatchDrawRecord {
    DirectX::XMFLOAT4 source;
    DirectX::XMFLOAT4 destination;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT4 origin_rotation_depth;
    std::uint64_t texture;
    DirectX::XMFLOAT4 texture_size;
    std::uint32_t flags;
};

#if defined(JPB_D3DAPP_TESTING)
std::size_t jpb_directxtk12_test_sprite_queue(
    SpriteSortMode mode,
    const JPBSpriteBatchSortRecord *records,
    std::size_t count,
    std::size_t *sorted_indices);

void jpb_directxtk12_test_build_sprite_record(
    bool in_begin_end_pair,
    D3D12_GPU_DESCRIPTOR_HANDLE texture,
    const DirectX::XMUINT2 &texture_size,
    const DirectX::XMFLOAT4 &destination,
    const RECT *source_rectangle,
    const DirectX::XMFLOAT4 &color,
    const DirectX::XMFLOAT4 &origin_rotation_depth,
    std::uint32_t flags,
    JPBSpriteBatchDrawRecord *record);

void jpb_directxtk12_test_render_sprite(
    const JPBSpriteBatchDrawRecord &record,
    VertexPositionColorTexture *vertices);

void jpb_directxtk12_test_viewport_transform(
    bool viewport_set,
    const D3D12_VIEWPORT &viewport,
    DXGI_MODE_ROTATION rotation,
    DirectX::XMFLOAT4X4 *transform);

void jpb_directxtk12_test_public_draw_overloads(
    JPBSpriteBatchDrawRecord *records);

void jpb_directxtk12_test_resource_upload_transition(
    bool open,
    D3D12_COMMAND_LIST_TYPE command_type,
    ID3D12GraphicsCommandList *command_list,
    ID3D12Resource *resource,
    D3D12_RESOURCE_STATES state_before,
    D3D12_RESOURCE_STATES state_after);

void jpb_directxtk12_test_set_sprite_device_resources(
    ID3D12Device *device,
    ID3D12RootSignature *static_root_signature,
    ID3D12RootSignature *heap_root_signature,
    const D3D12_INDEX_BUFFER_VIEW &index_buffer_view);
void jpb_directxtk12_test_clear_sprite_device_resources();

using JPBSpriteBatchRootSignatureTestHook = void (*)(
    std::size_t signature_index,
    const D3D12_ROOT_SIGNATURE_DESC &description);
void jpb_directxtk12_test_create_sprite_root_signatures(
    ID3D12Device *device,
    JPBSpriteBatchRootSignatureTestHook hook);

std::size_t jpb_directxtk12_test_sprite_indices(
    std::int16_t *indices,
    std::size_t capacity);
const unsigned char *jpb_directxtk12_test_sprite_shader(
    std::size_t shader_index,
    std::size_t *size);

std::size_t jpb_directxtk12_test_graphics_memory_pool(
    std::uint64_t size,
    std::uint64_t alignment,
    std::uint64_t *page_size);
#endif

} // namespace DX12
} // namespace DirectX

#endif
