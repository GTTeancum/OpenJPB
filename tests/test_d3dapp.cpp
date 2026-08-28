#include "jpb/d3dapp.h"
#include "jpb/d3dframe.h"
#include "jpb/d3dtransparencypass.h"
#include "jpb/d3dutil.h"
#include "jpb/directxtk12_abi.h"
#include "jpb/cube.h"
#include "jpb/level.h"
#include "jpb/resources.h"
#include "jpb/whook.h"

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <dxgi1_6.h>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "check failed: %s (%s:%d)\n", \
                     #condition, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

static bool near_float(float left, float right)
{
    return std::fabs(left - right) <= 0.0001f;
}

struct FakeCommandList {
    void **vtable;
    unsigned int calls;
    UINT barrier_count;
    D3D12_RESOURCE_BARRIER barrier;
};

struct FakeCommandQueue {
    void **vtable;
    unsigned int set_name_calls;
    wchar_t last_name[32];
    HRESULT set_name_result;
    unsigned int signal_calls;
    ID3D12Fence *fence;
    UINT64 value;
    HRESULT result;
    unsigned int execute_calls;
    UINT execute_count;
    ID3D12CommandList *executed_list;
};

struct FakeFence {
    void **vtable;
    unsigned int add_ref_calls;
    unsigned int release_calls;
    unsigned int event_calls;
    UINT64 value;
    HANDLE event;
    HRESULT result;
    unsigned int completed_value_calls;
    UINT64 completed_value;
};

struct FakeUnknown {
    void **vtable;
    unsigned int add_ref_calls;
    unsigned int release_calls;
};

struct FakePipelineState {
    void **vtable;
    unsigned int release_calls;
    unsigned int set_name_calls;
    wchar_t last_name[64];
};

struct FakeDevice {
    void **vtable;
    unsigned int add_ref_calls;
    unsigned int release_calls;
    unsigned int command_queue_calls;
    D3D12_COMMAND_QUEUE_DESC command_queue_description;
    const IID *command_queue_iid;
    ID3D12CommandQueue *command_queue_result;
    HRESULT command_queue_result_hr;
    unsigned int allocator_calls;
    D3D12_COMMAND_LIST_TYPE allocator_types[2];
    const IID *allocator_iids[2];
    ID3D12CommandAllocator *allocator_results[2];
    HRESULT allocator_results_hr[2];
    unsigned int command_list_calls;
    UINT node_mask;
    D3D12_COMMAND_LIST_TYPE command_list_type;
    ID3D12CommandAllocator *command_list_allocator;
    ID3D12PipelineState *initial_pipeline_state;
    const IID *command_list_iid;
    ID3D12GraphicsCommandList *command_list_result;
    HRESULT command_list_result_hr;
    unsigned int fence_calls;
    UINT64 fence_initial_values[2];
    D3D12_FENCE_FLAGS fence_flags[2];
    const IID *fence_iids[2];
    ID3D12Fence *fence_results[2];
    HRESULT fence_results_hr[2];
    unsigned int descriptor_heap_calls;
    D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_description;
    const IID *descriptor_heap_iid;
    ID3D12DescriptorHeap *descriptor_heap_result;
    HRESULT descriptor_heap_result_hr;
    D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_description2;
    const IID *descriptor_heap_iid2;
    ID3D12DescriptorHeap *descriptor_heap_result2;
    HRESULT descriptor_heap_result_hr2;
    unsigned int increment_size_calls;
    D3D12_DESCRIPTOR_HEAP_TYPE increment_size_type;
    UINT increment_size_result;
    unsigned int sampler_calls;
    D3D12_SAMPLER_DESC sampler_descriptions[6];
    D3D12_CPU_DESCRIPTOR_HANDLE sampler_handles[6];
    unsigned int render_target_view_calls;
    ID3D12Resource *render_target_view_resources[2];
    const D3D12_RENDER_TARGET_VIEW_DESC *render_target_view_descriptions[2];
    D3D12_CPU_DESCRIPTOR_HANDLE render_target_view_handles[2];
    unsigned int constant_buffer_view_calls;
    D3D12_CONSTANT_BUFFER_VIEW_DESC constant_buffer_view_description;
    D3D12_CPU_DESCRIPTOR_HANDLE constant_buffer_view_handle;
    unsigned int committed_resource_calls;
    D3D12_HEAP_PROPERTIES committed_heap_properties;
    D3D12_HEAP_FLAGS committed_heap_flags;
    D3D12_RESOURCE_DESC committed_resource_description;
    D3D12_RESOURCE_STATES committed_initial_state;
    D3D12_CLEAR_VALUE committed_clear_value;
    const IID *committed_resource_iid;
    ID3D12Resource *committed_resource_result;
    HRESULT committed_resource_result_hr;
    D3D12_HEAP_PROPERTIES committed_heap_properties_by_call[8];
    D3D12_HEAP_FLAGS committed_heap_flags_by_call[8];
    D3D12_RESOURCE_DESC committed_resource_descriptions_by_call[8];
    D3D12_RESOURCE_STATES committed_initial_states_by_call[8];
    bool committed_clear_value_present_by_call[8];
    const IID *committed_resource_iids_by_call[8];
    ID3D12Resource *committed_resource_results[8];
    unsigned int copyable_footprint_calls;
    D3D12_RESOURCE_DESC copyable_descriptions[2];
    UINT copyable_first_subresources[2];
    UINT copyable_subresource_counts[2];
    UINT64 copyable_base_offsets[2];
    UINT64 copyable_required_size;
    unsigned int shader_resource_view_calls;
    ID3D12Resource *shader_resource_view_resource;
    D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_description;
    D3D12_CPU_DESCRIPTOR_HANDLE shader_resource_view_handle;
    unsigned int depth_stencil_view_calls;
    ID3D12Resource *depth_stencil_view_resource;
    D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_description;
    D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_view_handle;
    unsigned int feature_support_calls;
    D3D12_FEATURE feature;
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS feature_data;
    UINT feature_data_size;
    HRESULT feature_support_result;
    BOOL typed_uav_load_additional_formats;
    BOOL standard_swizzle_64kb_supported;
    unsigned int root_signature_calls;
    UINT root_signature_node_mask;
    const void *root_signature_blob;
    SIZE_T root_signature_blob_size;
    const IID *root_signature_iid;
    ID3D12RootSignature *root_signature_result;
    HRESULT root_signature_result_hr;
    unsigned int graphics_pipeline_calls;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphics_pipeline_description;
    D3D12_INPUT_ELEMENT_DESC graphics_pipeline_input_elements[5];
    const IID *graphics_pipeline_iid;
    ID3D12PipelineState *graphics_pipeline_result;
    HRESULT graphics_pipeline_result_hr;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC
        graphics_pipeline_descriptions_by_call[2];
    D3D12_INPUT_ELEMENT_DESC
        graphics_pipeline_input_elements_by_call[2][5];
    const IID *graphics_pipeline_iids_by_call[2];
    ID3D12PipelineState *graphics_pipeline_results[2];
};

class FakeDescriptorHeap : public ID3D12DescriptorHeap {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **) override
    {
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        ++add_ref_calls;
        return 1;
    }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ++release_calls;
        return 1;
    }
    HRESULT STDMETHODCALLTYPE GetPrivateData(
        REFGUID, UINT *, void *) override
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetPrivateData(
        REFGUID, UINT, const void *) override
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        REFGUID, const IUnknown *) override
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetName(LPCWSTR name) override
    {
        ++set_name_calls;
        std::wcsncpy(last_name, name, 31);
        last_name[31] = L'\0';
        return set_name_result;
    }
    HRESULT STDMETHODCALLTYPE GetDevice(REFIID iid, void **device) override
    {
        ++get_device_calls;
        device_iid = &iid;
        if (SUCCEEDED(get_device_result)) {
            *device = device_result;
        }
        return get_device_result;
    }
    D3D12_DESCRIPTOR_HEAP_DESC STDMETHODCALLTYPE GetDesc() override
    {
        ++get_desc_calls;
        return description;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE
    GetCPUDescriptorHandleForHeapStart() override
    {
        ++cpu_handle_calls;
        return cpu_handle;
    }
    D3D12_GPU_DESCRIPTOR_HANDLE STDMETHODCALLTYPE
    GetGPUDescriptorHandleForHeapStart() override
    {
        ++gpu_handle_calls;
        return gpu_handle;
    }

    unsigned int set_name_calls = 0;
    wchar_t last_name[32] = {};
    HRESULT set_name_result = S_OK;
    unsigned int cpu_handle_calls = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = {};
    unsigned int gpu_handle_calls = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = {};
    unsigned int add_ref_calls = 0;
    unsigned int release_calls = 0;
    unsigned int get_device_calls = 0;
    const IID *device_iid = nullptr;
    ID3D12Device *device_result = nullptr;
    HRESULT get_device_result = S_OK;
    unsigned int get_desc_calls = 0;
    D3D12_DESCRIPTOR_HEAP_DESC description = {};
};

class FakeMenuResource : public ID3D12Resource {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **) override
    {
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return 1;
    }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ++release_calls;
        return 1;
    }
    HRESULT STDMETHODCALLTYPE GetPrivateData(
        REFGUID, UINT *, void *) override
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetPrivateData(
        REFGUID, UINT, const void *) override
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        REFGUID, const IUnknown *) override
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetName(LPCWSTR name) override
    {
        ++set_name_calls;
        std::wcsncpy(last_name, name, 31);
        last_name[31] = L'\0';
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetDevice(REFIID iid, void **result) override
    {
        ++get_device_calls;
        device_iid = &iid;
        *result = device;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Map(
        UINT subresource,
        const D3D12_RANGE *read_range,
        void **result) override
    {
        ++map_calls;
        map_subresource = subresource;
        map_read_range = read_range;
        *result = mapped_data;
        return S_OK;
    }
    void STDMETHODCALLTYPE Unmap(
        UINT subresource,
        const D3D12_RANGE *written_range) override
    {
        ++unmap_calls;
        unmap_subresource = subresource;
        unmap_written_range = written_range;
    }
    D3D12_RESOURCE_DESC STDMETHODCALLTYPE GetDesc() override
    {
        ++get_desc_calls;
        return description;
    }
    D3D12_GPU_VIRTUAL_ADDRESS STDMETHODCALLTYPE
    GetGPUVirtualAddress() override
    {
        return 0;
    }
    HRESULT STDMETHODCALLTYPE WriteToSubresource(
        UINT, const D3D12_BOX *, const void *, UINT, UINT) override
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE ReadFromSubresource(
        void *, UINT, UINT, UINT, const D3D12_BOX *) override
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE GetHeapProperties(
        D3D12_HEAP_PROPERTIES *, D3D12_HEAP_FLAGS *) override
    {
        return E_NOTIMPL;
    }

    D3D12_RESOURCE_DESC description = {};
    ID3D12Device *device = nullptr;
    unsigned char *mapped_data = nullptr;
    unsigned int release_calls = 0;
    unsigned int set_name_calls = 0;
    wchar_t last_name[32] = {};
    unsigned int get_device_calls = 0;
    const IID *device_iid = nullptr;
    unsigned int map_calls = 0;
    UINT map_subresource = 0;
    const D3D12_RANGE *map_read_range = nullptr;
    unsigned int unmap_calls = 0;
    UINT unmap_subresource = 0;
    const D3D12_RANGE *unmap_written_range = nullptr;
    unsigned int get_desc_calls = 0;
};

class FakeDxcBlob : public IDxcBlob {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **) override
    {
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override
    {
        ++add_ref_calls;
        return 1;
    }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ++release_calls;
        return 1;
    }
    LPVOID STDMETHODCALLTYPE GetBufferPointer() override
    {
        ++get_buffer_pointer_calls;
        return buffer;
    }
    SIZE_T STDMETHODCALLTYPE GetBufferSize() override
    {
        ++get_buffer_size_calls;
        return size;
    }

    unsigned int add_ref_calls = 0;
    unsigned int release_calls = 0;
    unsigned int get_buffer_pointer_calls = 0;
    unsigned int get_buffer_size_calls = 0;
    void *buffer = nullptr;
    SIZE_T size = 0;
};

struct FakeRenderCommandAllocator {
    void **vtable;
    unsigned int reset_calls;
    HRESULT reset_result;
    unsigned int release_calls;
};

struct FakeRenderResource {
    void **vtable;
    unsigned int gpu_address_calls;
    unsigned int release_calls;
    D3D12_GPU_VIRTUAL_ADDRESS gpu_address;
};

struct FakeTransparencyResource {
    void **vtable;
    unsigned int release_calls;
    unsigned int map_calls;
    UINT map_subresource;
    const D3D12_RANGE *map_read_range;
    D3D12_RANGE map_read_range_value;
    void *mapped_data;
    HRESULT map_result;
    unsigned int unmap_calls;
    UINT unmap_subresource;
    const D3D12_RANGE *unmap_written_range;
    unsigned int gpu_address_calls;
    D3D12_GPU_VIRTUAL_ADDRESS gpu_address;
};

struct FakeRenderCommandList {
    void **vtable;
    unsigned int calls[64];
    unsigned int call_count;
    unsigned int close_calls;
    HRESULT close_result;
    unsigned int reset_calls;
    ID3D12CommandAllocator *reset_allocator;
    ID3D12PipelineState *reset_pipeline_state;
    HRESULT reset_result;
    ID3D12PipelineState *pipeline_state;
    unsigned int pipeline_state_calls;
    ID3D12PipelineState *pipeline_states[2];
    ID3D12RootSignature *root_signature;
    unsigned int root_signature_calls;
    UINT viewport_count;
    D3D12_VIEWPORT viewport;
    UINT scissor_count;
    D3D12_RECT scissor;
    unsigned int descriptor_heap_calls;
    ID3D12DescriptorHeap *descriptor_heaps[2];
    UINT root_cbv_index;
    D3D12_GPU_VIRTUAL_ADDRESS root_cbv_address;
    UINT root_table_index;
    D3D12_GPU_DESCRIPTOR_HANDLE root_table_handle;
    unsigned int root_table_calls;
    UINT root_table_indices[2];
    D3D12_GPU_DESCRIPTOR_HANDLE root_table_handles[2];
    UINT barrier_count;
    D3D12_RESOURCE_BARRIER barrier;
    unsigned int barrier_calls;
    UINT barrier_counts[4];
    D3D12_RESOURCE_BARRIER barriers[4][2];
    unsigned int copy_buffer_calls;
    ID3D12Resource *copy_destinations[4];
    UINT64 copy_destination_offsets[4];
    ID3D12Resource *copy_sources[4];
    UINT64 copy_source_offsets[4];
    UINT64 copy_sizes[4];
    unsigned int copy_texture_calls;
    D3D12_TEXTURE_COPY_LOCATION copy_texture_destinations[2];
    D3D12_TEXTURE_COPY_LOCATION copy_texture_sources[2];
    UINT copy_texture_x[2];
    UINT copy_texture_y[2];
    UINT copy_texture_z[2];
    unsigned int topology_calls;
    D3D12_PRIMITIVE_TOPOLOGY topologies[2];
    unsigned int vertex_buffer_calls;
    UINT vertex_buffer_start_slots[2];
    UINT vertex_buffer_counts[2];
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_views[2];
    unsigned int index_buffer_calls;
    D3D12_INDEX_BUFFER_VIEW index_buffer_views[2];
    unsigned int draw_indexed_calls;
    UINT draw_index_counts[16];
    UINT draw_instance_counts[16];
    UINT draw_start_indices[16];
    INT draw_base_vertices[16];
    UINT draw_start_instances[16];
    UINT render_target_count;
    D3D12_CPU_DESCRIPTOR_HANDLE render_target_handle;
    BOOL render_targets_single_range;
    D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_handle;
    unsigned int render_target_calls;
    D3D12_CPU_DESCRIPTOR_HANDLE render_target_handles[3];
    D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_handles[3];
    D3D12_CPU_DESCRIPTOR_HANDLE clear_render_target_handle;
    float clear_color[4];
    UINT clear_render_target_rect_count;
    const D3D12_RECT *clear_render_target_rects;
    D3D12_CPU_DESCRIPTOR_HANDLE clear_depth_handle;
    D3D12_CLEAR_FLAGS clear_depth_flags;
    float clear_depth;
    UINT8 clear_stencil;
    UINT clear_depth_rect_count;
    const D3D12_RECT *clear_depth_rects;
    unsigned int add_ref_calls;
    unsigned int release_calls;
};

static void record_render_call(FakeRenderCommandList *list, unsigned int call)
{
    if (list->call_count >= 64) {
        std::abort();
    }
    list->calls[list->call_count++] = call;
}

static HRESULT STDMETHODCALLTYPE fake_render_allocator_reset(
    ID3D12CommandAllocator *allocator)
{
    auto *fake = reinterpret_cast<FakeRenderCommandAllocator *>(allocator);
    ++fake->reset_calls;
    return fake->reset_result;
}

static D3D12_GPU_VIRTUAL_ADDRESS STDMETHODCALLTYPE
fake_render_resource_gpu_address(ID3D12Resource *resource)
{
    auto *fake = reinterpret_cast<FakeRenderResource *>(resource);
    ++fake->gpu_address_calls;
    return fake->gpu_address;
}

static ULONG STDMETHODCALLTYPE fake_render_resource_release(
    ID3D12Resource *resource)
{
    auto *fake = reinterpret_cast<FakeRenderResource *>(resource);
    ++fake->release_calls;
    return 1;
}

static ULONG STDMETHODCALLTYPE fake_transparency_resource_release(
    ID3D12Resource *resource)
{
    auto *fake = reinterpret_cast<FakeTransparencyResource *>(resource);
    ++fake->release_calls;
    return 1;
}

static HRESULT STDMETHODCALLTYPE fake_transparency_resource_map(
    ID3D12Resource *resource,
    UINT subresource,
    const D3D12_RANGE *read_range,
    void **data)
{
    auto *fake = reinterpret_cast<FakeTransparencyResource *>(resource);
    ++fake->map_calls;
    fake->map_subresource = subresource;
    fake->map_read_range = read_range;
    if (read_range != nullptr) {
        fake->map_read_range_value = *read_range;
    }
    *data = fake->mapped_data;
    return fake->map_result;
}

static D3D12_GPU_VIRTUAL_ADDRESS STDMETHODCALLTYPE
fake_transparency_resource_gpu_address(ID3D12Resource *resource)
{
    auto *fake = reinterpret_cast<FakeTransparencyResource *>(resource);
    ++fake->gpu_address_calls;
    return fake->gpu_address;
}

static void STDMETHODCALLTYPE fake_transparency_resource_unmap(
    ID3D12Resource *resource,
    UINT subresource,
    const D3D12_RANGE *written_range)
{
    auto *fake = reinterpret_cast<FakeTransparencyResource *>(resource);
    ++fake->unmap_calls;
    fake->unmap_subresource = subresource;
    fake->unmap_written_range = written_range;
}

static HRESULT STDMETHODCALLTYPE fake_render_command_list_close(
    ID3D12GraphicsCommandList *command_list)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    ++fake->close_calls;
    return fake->close_result;
}

static HRESULT STDMETHODCALLTYPE fake_render_command_list_reset(
    ID3D12GraphicsCommandList *command_list,
    ID3D12CommandAllocator *allocator,
    ID3D12PipelineState *pipeline_state)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 1);
    ++fake->reset_calls;
    fake->reset_allocator = allocator;
    fake->reset_pipeline_state = pipeline_state;
    return fake->reset_result;
}

static void STDMETHODCALLTYPE fake_render_set_pipeline_state(
    ID3D12GraphicsCommandList *command_list,
    ID3D12PipelineState *pipeline_state)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 2);
    fake->pipeline_state = pipeline_state;
    const unsigned int call = fake->pipeline_state_calls++;
    if (call >= 2) {
        std::abort();
    }
    fake->pipeline_states[call] = pipeline_state;
}

static void STDMETHODCALLTYPE fake_render_set_root_signature(
    ID3D12GraphicsCommandList *command_list,
    ID3D12RootSignature *root_signature)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 3);
    fake->root_signature = root_signature;
    ++fake->root_signature_calls;
}

static void STDMETHODCALLTYPE fake_render_set_viewports(
    ID3D12GraphicsCommandList *command_list,
    UINT count,
    const D3D12_VIEWPORT *viewports)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 4);
    fake->viewport_count = count;
    fake->viewport = viewports[0];
}

static void STDMETHODCALLTYPE fake_render_set_scissors(
    ID3D12GraphicsCommandList *command_list,
    UINT count,
    const D3D12_RECT *rects)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 5);
    fake->scissor_count = count;
    fake->scissor = rects[0];
}

static void STDMETHODCALLTYPE fake_render_set_descriptor_heaps(
    ID3D12GraphicsCommandList *command_list,
    UINT count,
    ID3D12DescriptorHeap *const *heaps)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, fake->descriptor_heap_calls == 0 ? 6 : 8);
    if (count != 1 || fake->descriptor_heap_calls >= 2) {
        std::abort();
    }
    fake->descriptor_heaps[fake->descriptor_heap_calls++] = heaps[0];
}

static void STDMETHODCALLTYPE fake_render_set_root_cbv(
    ID3D12GraphicsCommandList *command_list,
    UINT root_index,
    D3D12_GPU_VIRTUAL_ADDRESS address)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 7);
    fake->root_cbv_index = root_index;
    fake->root_cbv_address = address;
}

static void STDMETHODCALLTYPE fake_render_set_root_table(
    ID3D12GraphicsCommandList *command_list,
    UINT root_index,
    D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 9);
    fake->root_table_index = root_index;
    fake->root_table_handle = handle;
    const unsigned int call = fake->root_table_calls++;
    if (call >= 2) {
        std::abort();
    }
    fake->root_table_indices[call] = root_index;
    fake->root_table_handles[call] = handle;
}

static void STDMETHODCALLTYPE fake_render_resource_barrier(
    ID3D12GraphicsCommandList *command_list,
    UINT count,
    const D3D12_RESOURCE_BARRIER *barriers)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 10);
    fake->barrier_count = count;
    fake->barrier = barriers[0];
    const unsigned int call = fake->barrier_calls++;
    if (call >= 4 || count > 2) {
        std::abort();
    }
    fake->barrier_counts[call] = count;
    std::memcpy(
        fake->barriers[call],
        barriers,
        count * sizeof(D3D12_RESOURCE_BARRIER));
}

static void STDMETHODCALLTYPE fake_render_copy_buffer_region(
    ID3D12GraphicsCommandList *command_list,
    ID3D12Resource *destination,
    UINT64 destination_offset,
    ID3D12Resource *source,
    UINT64 source_offset,
    UINT64 byte_count)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 14);
    const unsigned int call = fake->copy_buffer_calls++;
    if (call >= 4) {
        std::abort();
    }
    fake->copy_destinations[call] = destination;
    fake->copy_destination_offsets[call] = destination_offset;
    fake->copy_sources[call] = source;
    fake->copy_source_offsets[call] = source_offset;
    fake->copy_sizes[call] = byte_count;
}

static void STDMETHODCALLTYPE fake_render_copy_texture_region(
    ID3D12GraphicsCommandList *command_list,
    const D3D12_TEXTURE_COPY_LOCATION *destination,
    UINT destination_x,
    UINT destination_y,
    UINT destination_z,
    const D3D12_TEXTURE_COPY_LOCATION *source,
    const D3D12_BOX *)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    const unsigned int call = fake->copy_texture_calls++;
    if (call >= 2) {
        std::abort();
    }
    fake->copy_texture_destinations[call] = *destination;
    fake->copy_texture_sources[call] = *source;
    fake->copy_texture_x[call] = destination_x;
    fake->copy_texture_y[call] = destination_y;
    fake->copy_texture_z[call] = destination_z;
}

static void STDMETHODCALLTYPE fake_render_set_topology(
    ID3D12GraphicsCommandList *command_list,
    D3D12_PRIMITIVE_TOPOLOGY topology)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 15);
    const unsigned int call = fake->topology_calls++;
    if (call >= 2) {
        std::abort();
    }
    fake->topologies[call] = topology;
}

static void STDMETHODCALLTYPE fake_render_set_vertex_buffers(
    ID3D12GraphicsCommandList *command_list,
    UINT start_slot,
    UINT count,
    const D3D12_VERTEX_BUFFER_VIEW *views)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 16);
    const unsigned int call = fake->vertex_buffer_calls++;
    if (call >= 2 || count != 1) {
        std::abort();
    }
    fake->vertex_buffer_start_slots[call] = start_slot;
    fake->vertex_buffer_counts[call] = count;
    fake->vertex_buffer_views[call] = views[0];
}

static void STDMETHODCALLTYPE fake_render_set_index_buffer(
    ID3D12GraphicsCommandList *command_list,
    const D3D12_INDEX_BUFFER_VIEW *view)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 17);
    const unsigned int call = fake->index_buffer_calls++;
    if (call >= 2) {
        std::abort();
    }
    fake->index_buffer_views[call] = *view;
}

static void STDMETHODCALLTYPE fake_render_draw_indexed(
    ID3D12GraphicsCommandList *command_list,
    UINT index_count,
    UINT instance_count,
    UINT start_index,
    INT base_vertex,
    UINT start_instance)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 18);
    const unsigned int call = fake->draw_indexed_calls++;
    if (call >= 16) {
        std::abort();
    }
    fake->draw_index_counts[call] = index_count;
    fake->draw_instance_counts[call] = instance_count;
    fake->draw_start_indices[call] = start_index;
    fake->draw_base_vertices[call] = base_vertex;
    fake->draw_start_instances[call] = start_instance;
}

static void STDMETHODCALLTYPE fake_render_set_render_targets(
    ID3D12GraphicsCommandList *command_list,
    UINT count,
    const D3D12_CPU_DESCRIPTOR_HANDLE *render_targets,
    BOOL single_range,
    const D3D12_CPU_DESCRIPTOR_HANDLE *depth_stencil)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 11);
    fake->render_target_count = count;
    fake->render_target_handle = render_targets[0];
    fake->render_targets_single_range = single_range;
    fake->depth_stencil_handle = *depth_stencil;
    const unsigned int call = fake->render_target_calls++;
    if (call >= 3) {
        std::abort();
    }
    fake->render_target_handles[call] = render_targets[0];
    fake->depth_stencil_handles[call] = *depth_stencil;
}

static void STDMETHODCALLTYPE fake_render_clear_render_target(
    ID3D12GraphicsCommandList *command_list,
    D3D12_CPU_DESCRIPTOR_HANDLE handle,
    const FLOAT color[4],
    UINT rect_count,
    const D3D12_RECT *rects)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 12);
    fake->clear_render_target_handle = handle;
    std::memcpy(fake->clear_color, color, sizeof(fake->clear_color));
    fake->clear_render_target_rect_count = rect_count;
    fake->clear_render_target_rects = rects;
}

static void STDMETHODCALLTYPE fake_render_clear_depth(
    ID3D12GraphicsCommandList *command_list,
    D3D12_CPU_DESCRIPTOR_HANDLE handle,
    D3D12_CLEAR_FLAGS flags,
    FLOAT depth,
    UINT8 stencil,
    UINT rect_count,
    const D3D12_RECT *rects)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    record_render_call(fake, 13);
    fake->clear_depth_handle = handle;
    fake->clear_depth_flags = flags;
    fake->clear_depth = depth;
    fake->clear_stencil = stencil;
    fake->clear_depth_rect_count = rect_count;
    fake->clear_depth_rects = rects;
}

struct FakeSwapChain {
    void **vtable;
    unsigned int query_interface_calls;
    const IID *query_interface_iid;
    IDXGISwapChain3 *query_interface_result;
    HRESULT query_interface_result_hr;
    unsigned int release_calls;
    unsigned int get_buffer_calls;
    UINT indices[2];
    const IID *iids[2];
    ID3D12Resource *results[2];
    HRESULT results_hr[2];
    unsigned int current_index_calls;
    UINT current_index;
    unsigned int present_calls;
    UINT sync_interval;
    UINT present_flags;
    HRESULT present_result;
};

struct FakeFactory {
    void **vtable;
    unsigned int preferred_adapter_calls;
    UINT preferred_adapter_index;
    DXGI_GPU_PREFERENCE gpu_preference;
    const IID *preferred_adapter_iid;
    IDXGIAdapter1 *preferred_adapter_result;
    HRESULT preferred_adapter_result_hr;
    unsigned int enum_adapters_calls;
    UINT enum_adapter_indices[8];
    IDXGIAdapter1 *enum_adapter_results[8];
    HRESULT enum_adapter_results_hr[8];
    unsigned int create_swap_chain_calls;
    IUnknown *queue;
    HWND window;
    DXGI_SWAP_CHAIN_DESC1 description;
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_description;
    IDXGIOutput *restrict_output;
    IDXGISwapChain1 *swap_chain_result;
    HRESULT result;
};

struct FakeAdapter {
    void **vtable;
    unsigned int release_calls;
    unsigned int get_desc1_calls;
    DXGI_ADAPTER_DESC1 description;
    HRESULT get_desc1_result;
};

struct FakeCreateDeviceState {
    unsigned int calls;
    IUnknown *adapters[8];
    D3D_FEATURE_LEVEL feature_levels[8];
    const IID *iids[8];
    ID3D12Device *results[8];
    HRESULT result_codes[8];
};

struct FakeCreateFactoryState {
    unsigned int calls;
    UINT flags;
    const IID *iid;
    IDXGIFactory6 *factory;
    HRESULT result;
};

static FakeCreateFactoryState fake_create_factory_state;
static unsigned int fake_change_delete_device_objects_calls;
static unsigned int fake_destroy_framework_calls;
static CD3DFramework12 *fake_destroyed_framework;
static HRESULT fake_destroy_framework_result;
static unsigned int fake_render_environment_calls;
static float fake_render_environment_time;
static HRESULT fake_render_environment_result;
static unsigned int fake_sdl_window_flags_calls;
static void *fake_sdl_window_flags_window;
static std::uint32_t fake_sdl_window_flags_result;
static unsigned int fake_sdl_destroy_window_calls;
static void *fake_sdl_destroyed_window;
static unsigned int fake_sdl_quit_calls;
static unsigned int fake_sdl_query_texture_calls;
static void *fake_sdl_query_texture_texture;
static std::uint32_t fake_sdl_query_texture_format;
static int fake_sdl_query_texture_access;
static int fake_sdl_query_texture_width;
static int fake_sdl_query_texture_height;
static unsigned int fake_sdl_render_present_calls;
static void *fake_sdl_render_present_renderer;
static unsigned int fake_sdl_render_read_pixels_calls;
static void *fake_sdl_render_read_pixels_renderer;
static const void *fake_sdl_render_read_pixels_rect;
static std::uint32_t fake_sdl_render_read_pixels_format;
static int fake_sdl_render_read_pixels_pitch;

static HRESULT fake_change_delete_device_objects(CD3DApplication *)
{
    ++fake_change_delete_device_objects_calls;
    return S_OK;
}

static HRESULT fake_destroy_framework_objects(CD3DFramework12 *framework)
{
    ++fake_destroy_framework_calls;
    fake_destroyed_framework = framework;
    return fake_destroy_framework_result;
}

static HRESULT fake_render_environment(
    CD3DApplication *, float time)
{
    ++fake_render_environment_calls;
    fake_render_environment_time = time;
    return fake_render_environment_result;
}

static std::uint32_t fake_sdl_get_window_flags(void *window)
{
    ++fake_sdl_window_flags_calls;
    fake_sdl_window_flags_window = window;
    return fake_sdl_window_flags_result;
}

static void fake_sdl_destroy_window(void *window)
{
    ++fake_sdl_destroy_window_calls;
    fake_sdl_destroyed_window = window;
}

static void fake_sdl_quit()
{
    ++fake_sdl_quit_calls;
}

static int fake_sdl_query_texture(
    void *texture,
    std::uint32_t *format,
    int *access,
    int *width,
    int *height)
{
    ++fake_sdl_query_texture_calls;
    fake_sdl_query_texture_texture = texture;
    *format = fake_sdl_query_texture_format;
    *access = fake_sdl_query_texture_access;
    *width = fake_sdl_query_texture_width;
    *height = fake_sdl_query_texture_height;
    return 0;
}

static void fake_sdl_render_present(void *renderer)
{
    ++fake_sdl_render_present_calls;
    fake_sdl_render_present_renderer = renderer;
}

static int fake_sdl_render_read_pixels(
    void *renderer,
    const void *rect,
    std::uint32_t format,
    void *pixels,
    int pitch)
{
    ++fake_sdl_render_read_pixels_calls;
    fake_sdl_render_read_pixels_renderer = renderer;
    fake_sdl_render_read_pixels_rect = rect;
    fake_sdl_render_read_pixels_format = format;
    fake_sdl_render_read_pixels_pitch = pitch;
    for (int index = 0;
         index < fake_sdl_query_texture_width *
                     fake_sdl_query_texture_height * 4;
         ++index) {
        static_cast<unsigned char *>(pixels)[index] =
            static_cast<unsigned char>(index + 1);
    }
    return 0;
}

static FakeCreateDeviceState fake_create_device_state;

struct FakeBlob {
    void **vtable;
    unsigned int pointer_calls;
    unsigned int size_calls;
    unsigned int call_order[2];
    unsigned int call_count;
    void *data;
    SIZE_T size;
};

struct FakeRootSignature {
    void **vtable;
    unsigned int set_name_calls;
    wchar_t last_name[32];
    HRESULT set_name_result;
};

struct FakeSerializeRootSignatureState {
    unsigned int calls;
    D3D12_ROOT_SIGNATURE_DESC description;
    D3D12_ROOT_PARAMETER parameters[3];
    D3D12_DESCRIPTOR_RANGE range;
    D3D12_STATIC_SAMPLER_DESC sampler;
    D3D_ROOT_SIGNATURE_VERSION version;
    ID3DBlob *signature_result;
    ID3DBlob *error_result;
    HRESULT result;
};

static FakeSerializeRootSignatureState fake_serialize_root_signature_state;
static unsigned int fake_delete_device_objects_calls;
static unsigned int fake_final_cleanup_calls;
static unsigned int fake_pause_calls;
static int fake_pause_value;
static unsigned int fake_message_key_down_calls;
static unsigned int fake_message_key_up_calls;
static int fake_message_key_down_value;
static int fake_message_key_up_value;
static unsigned int fake_message_toggle_calls;
static unsigned int fake_message_query_calls;
static unsigned int fake_message_resume_calls;
static unsigned long fake_message_power_value;
static unsigned int fake_resize_resources_calls;
static CD3DFramework12 *fake_resize_framework;
static UINT fake_resize_width;
static UINT fake_resize_height;
static HRESULT fake_resize_result;
static unsigned int fake_graphics_memory_commit_calls;
static void *fake_graphics_memory_commit_memory;
static ID3D12CommandQueue *fake_graphics_memory_commit_queue;

static HRESULT fake_resize_resources(
    CD3DFramework12 *framework, UINT width, UINT height)
{
    ++fake_resize_resources_calls;
    fake_resize_framework = framework;
    fake_resize_width = width;
    fake_resize_height = height;
    framework->m_dwRenderWidth = width;
    framework->m_dwRenderHeight = height;
    return fake_resize_result;
}

static void fake_graphics_memory_commit(
    void *graphics_memory, ID3D12CommandQueue *command_queue)
{
    ++fake_graphics_memory_commit_calls;
    fake_graphics_memory_commit_memory = graphics_memory;
    fake_graphics_memory_commit_queue = command_queue;
}

static HRESULT fake_application_delete_device_objects(
    CD3DApplication *)
{
    ++fake_delete_device_objects_calls;
    return E_FAIL;
}

static HRESULT fake_application_final_cleanup(CD3DApplication *)
{
    ++fake_final_cleanup_calls;
    return E_ACCESSDENIED;
}

static void fake_application_pause(CD3DApplication *, int pause)
{
    ++fake_pause_calls;
    fake_pause_value = pause;
}

static void fake_message_key_down(CD3DApplication *, int key)
{
    ++fake_message_key_down_calls;
    fake_message_key_down_value = key;
}

static void fake_message_key_up(CD3DApplication *, int key)
{
    ++fake_message_key_up_calls;
    fake_message_key_up_value = key;
}

static int fake_message_toggle(CD3DApplication *)
{
    ++fake_message_toggle_calls;
    return 19;
}

static std::int64_t fake_message_query(
    CD3DApplication *, unsigned long value)
{
    ++fake_message_query_calls;
    fake_message_power_value = value;
    return 23;
}

static std::int64_t fake_message_resume(
    CD3DApplication *, unsigned long value)
{
    ++fake_message_resume_calls;
    fake_message_power_value = value;
    return 29;
}

static HRESULT WINAPI fake_serialize_root_signature(
    const D3D12_ROOT_SIGNATURE_DESC *description,
    D3D_ROOT_SIGNATURE_VERSION version,
    ID3DBlob **signature,
    ID3DBlob **error)
{
    ++fake_serialize_root_signature_state.calls;
    fake_serialize_root_signature_state.description = *description;
    std::memcpy(
        fake_serialize_root_signature_state.parameters,
        description->pParameters,
        sizeof(fake_serialize_root_signature_state.parameters));
    fake_serialize_root_signature_state.range =
        description->pParameters[1].DescriptorTable.pDescriptorRanges[0];
    fake_serialize_root_signature_state.sampler =
        description->pStaticSamplers[0];
    fake_serialize_root_signature_state.version = version;
    if (fake_serialize_root_signature_state.signature_result != nullptr) {
        *signature = fake_serialize_root_signature_state.signature_result;
    }
    if (fake_serialize_root_signature_state.error_result != nullptr) {
        *error = fake_serialize_root_signature_state.error_result;
    }
    return fake_serialize_root_signature_state.result;
}

static void *STDMETHODCALLTYPE fake_blob_get_buffer_pointer(ID3DBlob *blob)
{
    auto *fake = reinterpret_cast<FakeBlob *>(blob);
    ++fake->pointer_calls;
    fake->call_order[fake->call_count++] = 1;
    return fake->data;
}

static SIZE_T STDMETHODCALLTYPE fake_blob_get_buffer_size(ID3DBlob *blob)
{
    auto *fake = reinterpret_cast<FakeBlob *>(blob);
    ++fake->size_calls;
    fake->call_order[fake->call_count++] = 2;
    return fake->size;
}

static HRESULT STDMETHODCALLTYPE fake_root_signature_set_name(
    ID3D12RootSignature *root_signature, LPCWSTR name)
{
    auto *fake = reinterpret_cast<FakeRootSignature *>(root_signature);
    ++fake->set_name_calls;
    std::wcsncpy(fake->last_name, name, 31);
    fake->last_name[31] = L'\0';
    return fake->set_name_result;
}

static HRESULT WINAPI fake_d3d12_create_device(
    IUnknown *adapter,
    D3D_FEATURE_LEVEL minimum_feature_level,
    REFIID iid,
    void **device)
{
    const unsigned int index = fake_create_device_state.calls++;
    if (index >= 8) {
        std::abort();
    }
    fake_create_device_state.adapters[index] = adapter;
    fake_create_device_state.feature_levels[index] = minimum_feature_level;
    fake_create_device_state.iids[index] = &iid;
    if (fake_create_device_state.results[index] != nullptr) {
        *device = fake_create_device_state.results[index];
    }
    return fake_create_device_state.result_codes[index];
}

static HRESULT WINAPI fake_create_dxgi_factory(
    UINT flags, REFIID iid, void **factory)
{
    ++fake_create_factory_state.calls;
    fake_create_factory_state.flags = flags;
    fake_create_factory_state.iid = &iid;
    if (fake_create_factory_state.factory != nullptr) {
        *factory = fake_create_factory_state.factory;
    }
    return fake_create_factory_state.result;
}

static ULONG STDMETHODCALLTYPE fake_adapter_release(IDXGIAdapter1 *adapter)
{
    auto *fake = reinterpret_cast<FakeAdapter *>(adapter);
    ++fake->release_calls;
    return 0;
}

static HRESULT STDMETHODCALLTYPE fake_adapter_get_desc1(
    IDXGIAdapter1 *adapter, DXGI_ADAPTER_DESC1 *description)
{
    auto *fake = reinterpret_cast<FakeAdapter *>(adapter);
    ++fake->get_desc1_calls;
    *description = fake->description;
    return fake->get_desc1_result;
}

static HRESULT STDMETHODCALLTYPE fake_enum_adapter_by_gpu_preference(
    IDXGIFactory6 *factory,
    UINT adapter_index,
    DXGI_GPU_PREFERENCE gpu_preference,
    REFIID iid,
    void **adapter)
{
    auto *fake = reinterpret_cast<FakeFactory *>(factory);
    ++fake->preferred_adapter_calls;
    fake->preferred_adapter_index = adapter_index;
    fake->gpu_preference = gpu_preference;
    fake->preferred_adapter_iid = &iid;
    if (fake->preferred_adapter_result != nullptr) {
        *adapter = fake->preferred_adapter_result;
    }
    return fake->preferred_adapter_result_hr;
}

static HRESULT STDMETHODCALLTYPE fake_enum_adapters1(
    IDXGIFactory4 *factory,
    UINT adapter_index,
    IDXGIAdapter1 **adapter)
{
    auto *fake = reinterpret_cast<FakeFactory *>(factory);
    const unsigned int call = fake->enum_adapters_calls++;
    if (call >= 8) {
        std::abort();
    }
    fake->enum_adapter_indices[call] = adapter_index;
    if (fake->enum_adapter_results[call] != nullptr) {
        *adapter = fake->enum_adapter_results[call];
    }
    return fake->enum_adapter_results_hr[call];
}

static HRESULT STDMETHODCALLTYPE fake_create_command_allocator(
    ID3D12Device *device,
    D3D12_COMMAND_LIST_TYPE type,
    REFIID iid,
    void **allocator)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    const unsigned int index = fake->allocator_calls++;
    CHECK(index < 2);
    fake->allocator_types[index] = type;
    fake->allocator_iids[index] = &iid;
    if (SUCCEEDED(fake->allocator_results_hr[index])) {
        *allocator = fake->allocator_results[index];
    }
    return fake->allocator_results_hr[index];
}

static HRESULT STDMETHODCALLTYPE fake_check_feature_support(
    ID3D12Device *device,
    D3D12_FEATURE feature,
    void *feature_data,
    UINT feature_data_size)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    ++fake->feature_support_calls;
    fake->feature = feature;
    if (feature == D3D12_FEATURE_D3D12_OPTIONS) {
        auto *options = static_cast<D3D12_FEATURE_DATA_D3D12_OPTIONS *>(
            feature_data);
        options->TypedUAVLoadAdditionalFormats =
            fake->typed_uav_load_additional_formats;
        options->StandardSwizzle64KBSupported =
            fake->standard_swizzle_64kb_supported;
        fake->feature_data_size = feature_data_size;
        return fake->feature_support_result;
    }
    fake->feature_data =
        *static_cast<D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS *>(
            feature_data);
    fake->feature_data_size = feature_data_size;
    return fake->feature_support_result;
}

static HRESULT STDMETHODCALLTYPE fake_create_root_signature(
    ID3D12Device *device,
    UINT node_mask,
    const void *blob,
    SIZE_T blob_size,
    REFIID iid,
    void **root_signature)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    ++fake->root_signature_calls;
    fake->root_signature_node_mask = node_mask;
    fake->root_signature_blob = blob;
    fake->root_signature_blob_size = blob_size;
    fake->root_signature_iid = &iid;
    if (fake->root_signature_result != nullptr) {
        *root_signature = fake->root_signature_result;
    }
    return fake->root_signature_result_hr;
}

static HRESULT STDMETHODCALLTYPE fake_create_command_queue(
    ID3D12Device *device,
    const D3D12_COMMAND_QUEUE_DESC *description,
    REFIID iid,
    void **queue)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    ++fake->command_queue_calls;
    fake->command_queue_description = *description;
    fake->command_queue_iid = &iid;
    if (fake->command_queue_result != nullptr) {
        *queue = fake->command_queue_result;
    }
    return fake->command_queue_result_hr;
}

static HRESULT STDMETHODCALLTYPE fake_create_command_list(
    ID3D12Device *device,
    UINT node_mask,
    D3D12_COMMAND_LIST_TYPE type,
    ID3D12CommandAllocator *allocator,
    ID3D12PipelineState *initial_pipeline_state,
    REFIID iid,
    void **command_list)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    ++fake->command_list_calls;
    fake->node_mask = node_mask;
    fake->command_list_type = type;
    fake->command_list_allocator = allocator;
    fake->initial_pipeline_state = initial_pipeline_state;
    fake->command_list_iid = &iid;
    if (SUCCEEDED(fake->command_list_result_hr)) {
        *command_list = fake->command_list_result;
    }
    return fake->command_list_result_hr;
}

static HRESULT STDMETHODCALLTYPE fake_create_fence(
    ID3D12Device *device,
    UINT64 initial_value,
    D3D12_FENCE_FLAGS flags,
    REFIID iid,
    void **fence)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    const unsigned int index = fake->fence_calls++;
    CHECK(index < 2);
    fake->fence_initial_values[index] = initial_value;
    fake->fence_flags[index] = flags;
    fake->fence_iids[index] = &iid;
    if (SUCCEEDED(fake->fence_results_hr[index])) {
        *fence = fake->fence_results[index];
    }
    return fake->fence_results_hr[index];
}

static HRESULT STDMETHODCALLTYPE fake_create_descriptor_heap(
    ID3D12Device *device,
    const D3D12_DESCRIPTOR_HEAP_DESC *description,
    REFIID iid,
    void **heap)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    const unsigned int index = fake->descriptor_heap_calls++;
    if (index == 0) {
        fake->descriptor_heap_description = *description;
        fake->descriptor_heap_iid = &iid;
        *heap = fake->descriptor_heap_result;
        return fake->descriptor_heap_result_hr;
    }
    if (index == 1) {
        fake->descriptor_heap_description2 = *description;
        fake->descriptor_heap_iid2 = &iid;
        *heap = fake->descriptor_heap_result2;
        return fake->descriptor_heap_result_hr2;
    }
    std::abort();
}

static UINT STDMETHODCALLTYPE fake_get_descriptor_increment_size(
    ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    ++fake->increment_size_calls;
    fake->increment_size_type = type;
    return fake->increment_size_result;
}

static ULONG STDMETHODCALLTYPE fake_device_release(ID3D12Device *device)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    ++fake->release_calls;
    return 1;
}

static ULONG STDMETHODCALLTYPE fake_device_add_ref(ID3D12Device *device)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    ++fake->add_ref_calls;
    return 2;
}

static ULONG STDMETHODCALLTYPE fake_render_allocator_release(
    ID3D12CommandAllocator *allocator)
{
    auto *fake = reinterpret_cast<FakeRenderCommandAllocator *>(allocator);
    ++fake->release_calls;
    return 0;
}

static ULONG STDMETHODCALLTYPE fake_render_command_list_add_ref(
    ID3D12GraphicsCommandList *command_list)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    ++fake->add_ref_calls;
    return 2;
}

static ULONG STDMETHODCALLTYPE fake_render_command_list_release(
    ID3D12GraphicsCommandList *command_list)
{
    auto *fake = reinterpret_cast<FakeRenderCommandList *>(command_list);
    ++fake->release_calls;
    return 0;
}

static ULONG STDMETHODCALLTYPE fake_fence_add_ref(ID3D12Fence *fence)
{
    auto *fake = reinterpret_cast<FakeFence *>(fence);
    ++fake->add_ref_calls;
    return 2;
}

static ULONG STDMETHODCALLTYPE fake_fence_release(ID3D12Fence *fence)
{
    auto *fake = reinterpret_cast<FakeFence *>(fence);
    ++fake->release_calls;
    return 0;
}

static void STDMETHODCALLTYPE fake_create_sampler(
    ID3D12Device *device,
    const D3D12_SAMPLER_DESC *description,
    D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    const unsigned int index = fake->sampler_calls++;
    if (index >= 6) {
        std::abort();
    }
    fake->sampler_descriptions[index] = *description;
    fake->sampler_handles[index] = handle;
}

static void STDMETHODCALLTYPE fake_create_render_target_view(
    ID3D12Device *device,
    ID3D12Resource *resource,
    const D3D12_RENDER_TARGET_VIEW_DESC *description,
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    const unsigned int index = fake->render_target_view_calls++;
    if (index >= 2) {
        std::abort();
    }
    fake->render_target_view_resources[index] = resource;
    fake->render_target_view_descriptions[index] = description;
    fake->render_target_view_handles[index] = descriptor;
}

static void STDMETHODCALLTYPE fake_create_constant_buffer_view(
    ID3D12Device *device,
    const D3D12_CONSTANT_BUFFER_VIEW_DESC *description,
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    ++fake->constant_buffer_view_calls;
    fake->constant_buffer_view_description = *description;
    fake->constant_buffer_view_handle = descriptor;
}

static void STDMETHODCALLTYPE fake_create_shader_resource_view(
    ID3D12Device *device,
    ID3D12Resource *resource,
    const D3D12_SHADER_RESOURCE_VIEW_DESC *description,
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    ++fake->shader_resource_view_calls;
    fake->shader_resource_view_resource = resource;
    fake->shader_resource_view_description = *description;
    fake->shader_resource_view_handle = descriptor;
}

static void STDMETHODCALLTYPE fake_get_copyable_footprints(
    ID3D12Device *device,
    const D3D12_RESOURCE_DESC *description,
    UINT first_subresource,
    UINT subresource_count,
    UINT64 base_offset,
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *layouts,
    UINT *row_counts,
    UINT64 *row_sizes,
    UINT64 *total_bytes)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    const unsigned int call = fake->copyable_footprint_calls++;
    if (call >= 2) {
        std::abort();
    }
    fake->copyable_descriptions[call] = *description;
    fake->copyable_first_subresources[call] = first_subresource;
    fake->copyable_subresource_counts[call] = subresource_count;
    fake->copyable_base_offsets[call] = base_offset;
    *total_bytes = fake->copyable_required_size;
    if (layouts != nullptr) {
        layouts[0].Offset = base_offset;
        layouts[0].Footprint.Format = description->Format;
        layouts[0].Footprint.Width = static_cast<UINT>(description->Width);
        layouts[0].Footprint.Height = description->Height;
        layouts[0].Footprint.Depth = 1;
        layouts[0].Footprint.RowPitch = 256;
    }
    if (row_counts != nullptr) {
        row_counts[0] = description->Height;
    }
    if (row_sizes != nullptr) {
        row_sizes[0] = description->Width * 4;
    }
}

static HRESULT STDMETHODCALLTYPE fake_create_committed_resource(
    ID3D12Device *device,
    const D3D12_HEAP_PROPERTIES *heap_properties,
    D3D12_HEAP_FLAGS heap_flags,
    const D3D12_RESOURCE_DESC *resource_description,
    D3D12_RESOURCE_STATES initial_state,
    const D3D12_CLEAR_VALUE *clear_value,
    REFIID iid,
    void **resource)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    const unsigned int call = fake->committed_resource_calls++;
    if (call >= 8) {
        std::abort();
    }
    fake->committed_heap_properties = *heap_properties;
    fake->committed_heap_flags = heap_flags;
    fake->committed_resource_description = *resource_description;
    fake->committed_initial_state = initial_state;
    if (clear_value != nullptr) {
        fake->committed_clear_value = *clear_value;
    }
    fake->committed_resource_iid = &iid;
    fake->committed_heap_properties_by_call[call] = *heap_properties;
    fake->committed_heap_flags_by_call[call] = heap_flags;
    fake->committed_resource_descriptions_by_call[call] =
        *resource_description;
    fake->committed_initial_states_by_call[call] = initial_state;
    fake->committed_clear_value_present_by_call[call] =
        clear_value != nullptr;
    fake->committed_resource_iids_by_call[call] = &iid;
    ID3D12Resource *result = fake->committed_resource_results[call];
    if (result == nullptr) {
        result = fake->committed_resource_result;
    }
    if (result != nullptr) {
        *resource = result;
    }
    return fake->committed_resource_result_hr;
}

static void STDMETHODCALLTYPE fake_create_depth_stencil_view(
    ID3D12Device *device,
    ID3D12Resource *resource,
    const D3D12_DEPTH_STENCIL_VIEW_DESC *description,
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    ++fake->depth_stencil_view_calls;
    fake->depth_stencil_view_resource = resource;
    fake->depth_stencil_view_description = *description;
    fake->depth_stencil_view_handle = descriptor;
}

static HRESULT STDMETHODCALLTYPE fake_get_buffer(
    IDXGISwapChain3 *swap_chain, UINT index, REFIID iid, void **surface)
{
    auto *fake = reinterpret_cast<FakeSwapChain *>(swap_chain);
    const unsigned int call = fake->get_buffer_calls++;
    CHECK(call < 2);
    fake->indices[call] = index;
    fake->iids[call] = &iid;
    if (SUCCEEDED(fake->results_hr[call])) {
        *surface = fake->results[call];
    }
    return fake->results_hr[call];
}

static UINT STDMETHODCALLTYPE fake_get_current_back_buffer_index(
    IDXGISwapChain3 *swap_chain)
{
    auto *fake = reinterpret_cast<FakeSwapChain *>(swap_chain);
    ++fake->current_index_calls;
    return fake->current_index;
}

static HRESULT STDMETHODCALLTYPE fake_swap_chain_query_interface(
    IDXGISwapChain3 *swap_chain, REFIID iid, void **result)
{
    auto *fake = reinterpret_cast<FakeSwapChain *>(swap_chain);
    ++fake->query_interface_calls;
    fake->query_interface_iid = &iid;
    if (fake->query_interface_result != nullptr) {
        *result = fake->query_interface_result;
    }
    return fake->query_interface_result_hr;
}

static ULONG STDMETHODCALLTYPE fake_swap_chain_release(
    IDXGISwapChain3 *swap_chain)
{
    auto *fake = reinterpret_cast<FakeSwapChain *>(swap_chain);
    ++fake->release_calls;
    return 0;
}

static HRESULT STDMETHODCALLTYPE fake_swap_chain_present(
    IDXGISwapChain3 *swap_chain, UINT sync_interval, UINT flags)
{
    auto *fake = reinterpret_cast<FakeSwapChain *>(swap_chain);
    ++fake->present_calls;
    fake->sync_interval = sync_interval;
    fake->present_flags = flags;
    return fake->present_result;
}

static HRESULT STDMETHODCALLTYPE fake_create_swap_chain_for_hwnd(
    IDXGIFactory4 *factory,
    IUnknown *queue,
    HWND window,
    const DXGI_SWAP_CHAIN_DESC1 *description,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_description,
    IDXGIOutput *restrict_output,
    IDXGISwapChain1 **swap_chain)
{
    auto *fake = reinterpret_cast<FakeFactory *>(factory);
    ++fake->create_swap_chain_calls;
    fake->queue = queue;
    fake->window = window;
    fake->description = *description;
    fake->fullscreen_description = fullscreen_description;
    fake->restrict_output = restrict_output;
    if (fake->swap_chain_result != nullptr) {
        *swap_chain = fake->swap_chain_result;
    }
    return fake->result;
}

static HRESULT STDMETHODCALLTYPE fake_queue_signal(
    ID3D12CommandQueue *queue, ID3D12Fence *fence, UINT64 value)
{
    auto *fake = reinterpret_cast<FakeCommandQueue *>(queue);
    ++fake->signal_calls;
    fake->fence = fence;
    fake->value = value;
    return fake->result;
}

static void STDMETHODCALLTYPE fake_queue_execute_command_lists(
    ID3D12CommandQueue *queue,
    UINT count,
    ID3D12CommandList *const *command_lists)
{
    auto *fake = reinterpret_cast<FakeCommandQueue *>(queue);
    ++fake->execute_calls;
    fake->execute_count = count;
    fake->executed_list = command_lists[0];
}

static HRESULT STDMETHODCALLTYPE fake_queue_set_name(
    ID3D12CommandQueue *queue, LPCWSTR name)
{
    auto *fake = reinterpret_cast<FakeCommandQueue *>(queue);
    ++fake->set_name_calls;
    std::wcsncpy(fake->last_name, name, 31);
    fake->last_name[31] = L'\0';
    return fake->set_name_result;
}

static HRESULT STDMETHODCALLTYPE fake_fence_set_event(
    ID3D12Fence *fence, UINT64 value, HANDLE event)
{
    auto *fake = reinterpret_cast<FakeFence *>(fence);
    ++fake->event_calls;
    fake->value = value;
    fake->event = event;
    if (SUCCEEDED(fake->result)) {
        SetEvent(event);
    }
    return fake->result;
}

static UINT64 STDMETHODCALLTYPE fake_fence_get_completed_value(
    ID3D12Fence *fence)
{
    auto *fake = reinterpret_cast<FakeFence *>(fence);
    ++fake->completed_value_calls;
    return fake->completed_value;
}

static ULONG STDMETHODCALLTYPE fake_unknown_release(IUnknown *object)
{
    auto *fake = reinterpret_cast<FakeUnknown *>(object);
    ++fake->release_calls;
    return 0;
}

static ULONG STDMETHODCALLTYPE fake_unknown_add_ref(IUnknown *object)
{
    auto *fake = reinterpret_cast<FakeUnknown *>(object);
    ++fake->add_ref_calls;
    return 1;
}

static ULONG STDMETHODCALLTYPE fake_pipeline_release(
    ID3D12PipelineState *pipeline_state)
{
    auto *fake = reinterpret_cast<FakePipelineState *>(pipeline_state);
    return ++fake->release_calls;
}

static HRESULT STDMETHODCALLTYPE fake_pipeline_set_name(
    ID3D12PipelineState *pipeline_state,
    LPCWSTR name)
{
    auto *fake = reinterpret_cast<FakePipelineState *>(pipeline_state);
    ++fake->set_name_calls;
    std::wcsncpy(fake->last_name, name, 63);
    fake->last_name[63] = L'\0';
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE fake_create_graphics_pipeline_state(
    ID3D12Device *device,
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC *description,
    REFIID iid,
    void **pipeline_state)
{
    auto *fake = reinterpret_cast<FakeDevice *>(device);
    const unsigned int call = fake->graphics_pipeline_calls++;
    if (call >= 2) {
        std::abort();
    }
    fake->graphics_pipeline_description = *description;
    CHECK(description->InputLayout.NumElements <= 5);
    std::memcpy(
        fake->graphics_pipeline_input_elements,
        description->InputLayout.pInputElementDescs,
        description->InputLayout.NumElements *
            sizeof(D3D12_INPUT_ELEMENT_DESC));
    fake->graphics_pipeline_description.InputLayout.pInputElementDescs =
        fake->graphics_pipeline_input_elements;
    fake->graphics_pipeline_iid = &iid;
    fake->graphics_pipeline_descriptions_by_call[call] = *description;
    std::memcpy(
        fake->graphics_pipeline_input_elements_by_call[call],
        description->InputLayout.pInputElementDescs,
        description->InputLayout.NumElements *
            sizeof(D3D12_INPUT_ELEMENT_DESC));
    fake->graphics_pipeline_descriptions_by_call[call]
        .InputLayout.pInputElementDescs =
        fake->graphics_pipeline_input_elements_by_call[call];
    fake->graphics_pipeline_iids_by_call[call] = &iid;
    ID3D12PipelineState *result =
        fake->graphics_pipeline_results[call];
    if (result == nullptr) {
        result = fake->graphics_pipeline_result;
    }
    if (SUCCEEDED(fake->graphics_pipeline_result_hr)) {
        *pipeline_state = result;
    }
    return fake->graphics_pipeline_result_hr;
}

static void STDMETHODCALLTYPE fake_resource_barrier(
    ID3D12GraphicsCommandList *command_list,
    UINT barrier_count,
    const D3D12_RESOURCE_BARRIER *barriers)
{
    auto *fake = reinterpret_cast<FakeCommandList *>(command_list);
    ++fake->calls;
    fake->barrier_count = barrier_count;
    fake->barrier = barriers[0];
}

static int test_leaf_methods_and_exact_offsets()
{
    std::vector<unsigned char> storage(0x383A0, 0xCD);
    auto *application =
        reinterpret_cast<CD3DApplication *>(storage.data());

    CHECK(application->ConvertWindowSettingToFlags(0) == 0x1101);
    CHECK(application->ConvertWindowSettingToFlags(1) == 0x10);
    CHECK(application->ConvertWindowSettingToFlags(2) == 0);
    CHECK(application->CD3DApplication::DeleteDeviceObjects() == S_OK);
    CHECK(application->CD3DApplication::FinalCleanup() == S_OK);
    CHECK(application->CD3DApplication::InitDeviceObjects() == S_OK);
    CHECK(application->CD3DApplication::OneTimeSceneInit() == S_OK);
    CHECK(application->CD3DApplication::Render(123.0f) == S_OK);
    CHECK(application->CD3DApplication::RestoreSurfaces() == S_OK);
    CHECK(application->CD3DApplication::ToggleFullScreen() == 1);

    const std::vector<unsigned char> before_noops = storage;
    application->CD3DApplication::OnKeyDown(0x41);
    application->CD3DApplication::OnKeyUp(0x41);
    application->OutputText(const_cast<char *>("ignored"));
    application->OutputTextXY(10, 20, const_cast<char *>("ignored"));
    application->SelectTexture(7, nullptr, nullptr);
    application->SetRenderState(
        nullptr, nullptr, D3D12_BLEND_ONE, D3D12_BLEND_ZERO);
    CHECK(storage == before_noops);

    application->SetFullScreen(0x12345678);
    application->SetUseZBuffer(0x76543210);
    application->SetWindowFlags(0xA1B2C3D4UL);
    application->SetWidthHeight(1920, 1080);
    application->SetBitDepth(32);
    CHECK(*reinterpret_cast<int *>(storage.data() + 0xE0) == 0x12345678);
    CHECK(*reinterpret_cast<int *>(storage.data() + 0xE4) == 0x76543210);
    CHECK(*reinterpret_cast<unsigned long *>(storage.data() + 0x38318) ==
          0xA1B2C3D4UL);
    CHECK(*reinterpret_cast<unsigned *>(storage.data() + 0x3831C) == 1920);
    CHECK(*reinterpret_cast<unsigned *>(storage.data() + 0x38320) == 1080);
    CHECK(*reinterpret_cast<unsigned *>(storage.data() + 0x38324) == 32);

    char company[] = "LucasArts";
    char application_title[] = "Jedi Power Battles";
    application->SetCompanyTitle(company);
    application->SetAppTitle(application_title);
    CHECK(std::strcmp(
              reinterpret_cast<char *>(storage.data() + 0x38108),
              company) == 0);
    CHECK(std::strcmp(
              reinterpret_cast<char *>(storage.data() + 0x38208),
              application_title) == 0);
    CHECK(storage[0x38108 + 0xFF] == 0);
    CHECK(storage[0x38208 + 0xFF] == 0);

    D3DVECTOR from = {1.0f, 2.0f, -3.0f};
    D3DVECTOR at = {4.0f, 2.0f, 5.0f};
    D3DVECTOR up = {0.0f, 1.0f, 0.0f};
    D3DMATRIX expected = {};
    CHECK(D3DUtil_SetViewMatrix(expected, from, at, up) == S_OK);
    application->SetViewParams(&from, &at, &up, 99.0f);
    CHECK(std::memcmp(
              storage.data() + 0x38360, &expected, sizeof(expected)) == 0);
    return 0;
}

static unsigned char game_bar_visible_result;
static unsigned char game_bar_input_result;
static unsigned game_bar_visible_queries;
static unsigned game_bar_input_queries;
static unsigned game_bar_invalidate_calls;
static unsigned game_bar_pause_calls;
static HWND game_bar_invalidated_window;
static const RECT *game_bar_invalidated_rect;
static BOOL game_bar_invalidated_erase;

static HRESULT query_game_bar_state(
    int input_redirected, unsigned char *value)
{
    if (input_redirected != 0) {
        ++game_bar_input_queries;
        *value = game_bar_input_result;
    } else {
        ++game_bar_visible_queries;
        *value = game_bar_visible_result;
    }
    return E_FAIL;
}

static BOOL WINAPI record_game_bar_invalidation(
    HWND window, const RECT *rect, BOOL erase)
{
    ++game_bar_invalidate_calls;
    game_bar_invalidated_window = window;
    game_bar_invalidated_rect = rect;
    game_bar_invalidated_erase = erase;
    return FALSE;
}

static void record_game_bar_pause()
{
    ++game_bar_pause_calls;
}

static int test_game_bar_state_handlers()
{
    std::vector<unsigned char> storage(sizeof(CD3DApplication), 0);
    auto *application = reinterpret_cast<CD3DApplication *>(storage.data());
    const HWND window = reinterpret_cast<HWND>(0x12345678);
    jpb_d3dapp_set_game_bar_query_test_hook(query_game_bar_state);
    jpb_d3dapp_set_invalidate_rect_test_hook(
        record_game_bar_invalidation);
    jpb_d3dapp_set_pause_menu_test_hook(record_game_bar_pause);
    jpb_d3dapp_set_game_bar_state_for_test(0, 0);
    game_bar_visible_queries = 0;
    game_bar_input_queries = 0;
    game_bar_invalidate_calls = 0;
    game_bar_pause_calls = 0;

    game_bar_input_result = 0;
    application->CheckGameBarInput(window);
    CHECK(game_bar_input_queries == 1);
    CHECK(game_bar_invalidate_calls == 0);
    CHECK(jpb_d3dapp_get_game_bar_visible_for_test() == 0);

    game_bar_input_result = 1;
    application->CheckGameBarInput(window);
    CHECK(game_bar_input_queries == 2);
    CHECK(game_bar_invalidate_calls == 1);
    CHECK(game_bar_invalidated_window == window);
    CHECK(game_bar_invalidated_rect == nullptr);
    CHECK(game_bar_invalidated_erase == TRUE);
    CHECK(jpb_d3dapp_get_game_bar_visible_for_test() == 1);
    CHECK(jpb_d3dapp_get_game_bar_input_redirected_for_test() == 0);

    application->CheckGameBarInput(window);
    CHECK(game_bar_invalidate_calls == 2);

    game_bar_visible_result = 1;
    application->CheckGameBarVisibility(window);
    CHECK(game_bar_visible_queries == 1);
    CHECK(game_bar_pause_calls == 0);
    CHECK(game_bar_invalidate_calls == 2);

    game_bar_visible_result = 0;
    application->CheckGameBarVisibility(window);
    CHECK(game_bar_visible_queries == 2);
    CHECK(game_bar_pause_calls == 1);
    CHECK(game_bar_invalidate_calls == 3);
    CHECK(jpb_d3dapp_get_game_bar_visible_for_test() == 0);

    jpb_d3dapp_set_game_bar_query_test_hook(nullptr);
    jpb_d3dapp_set_invalidate_rect_test_hook(nullptr);
    jpb_d3dapp_set_pause_menu_test_hook(nullptr);
    return 0;
}

static int test_matrix_conversion()
{
    std::vector<unsigned char> storage(sizeof(CD3DApplication), 0);
    auto *application = reinterpret_cast<CD3DApplication *>(storage.data());
    MATRIX source = {
        {
            {1.25f, -2.5f, 3.75f},
            {4.5f, 5.25f, -6.0f},
            {-7.5f, 8.125f, 9.0f},
        },
        {-1234567, 7654321, -42},
    };
    DirectX::XMMATRIX converted =
        application->ConvertMatrixToDXMatrix(&source);
    const float *values = reinterpret_cast<const float *>(&converted);
    const float expected[16] = {
        1.25f, -2.5f, 3.75f, 0.0f,
        4.5f, 5.25f, -6.0f, 0.0f,
        -7.5f, 8.125f, 9.0f, 0.0f,
        static_cast<float>(source.t[0]),
        static_cast<float>(source.t[1]),
        static_cast<float>(source.t[2]),
        1.0f,
    };
    CHECK(std::memcmp(values, expected, sizeof(expected)) == 0);

    DirectX::XMMATRIX identity =
        application->ConvertMatrixToDXMatrix(nullptr);
    const float expected_identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    CHECK(std::memcmp(
              &identity, expected_identity, sizeof(expected_identity)) == 0);
    return 0;
}

static int test_transparency_pass_index_batching()
{
    alignas(D3DTransparencyPass)
        unsigned char storage[sizeof(D3DTransparencyPass)] = {};
    auto *pass = reinterpret_cast<D3DTransparencyPass *>(storage);
    auto *vertices = new (&pass->m_vertices) std::vector<Vertex>();
    auto *indices = new (&pass->m_indices) std::vector<unsigned int>();
    auto *additive_vertices = new (&pass->m_additiveVertices)
        std::vector<Vertex>();
    auto *additive_indices = new (&pass->m_additiveIndices)
        std::vector<unsigned int>();

    Vertex base = {};
    base.texIndex = 10;
    vertices->push_back(base);
    base.texIndex = 11;
    vertices->push_back(base);
    indices->push_back(99);
    base.texIndex = 20;
    additive_vertices->push_back(base);
    additive_indices->push_back(77);

    std::vector<Vertex> incoming(2);
    incoming[0].texIndex = 30;
    incoming[1].texIndex = 31;
    const std::vector<unsigned int> incoming_indices = {0, 1, 0};

    pass->AddIndexed(
        D3DMaterialType::Opaque, incoming, incoming_indices);
    CHECK(vertices->size() == 2);
    CHECK(indices->size() == 1);
    CHECK(additive_vertices->size() == 1);
    CHECK(additive_indices->size() == 1);

    pass->AddIndexed(
        D3DMaterialType::Transluscent, incoming, incoming_indices);
    CHECK(vertices->size() == 4);
    CHECK((*vertices)[2].texIndex == 30);
    CHECK((*vertices)[3].texIndex == 31);
    CHECK(*indices == std::vector<unsigned int>({99, 2, 3, 2}));

    pass->AddIndexed(
        D3DMaterialType::Additive, incoming, incoming_indices);
    CHECK(additive_vertices->size() == 3);
    CHECK((*additive_vertices)[1].texIndex == 30);
    CHECK((*additive_vertices)[2].texIndex == 31);
    CHECK(*additive_indices ==
          std::vector<unsigned int>({77, 1, 2, 1}));

    additive_indices->~vector();
    additive_vertices->~vector();
    indices->~vector();
    vertices->~vector();
    return 0;
}

static int test_transparency_pass_buffer_creation()
{
    void *device_vtable[28] = {};
    device_vtable[27] =
        reinterpret_cast<void *>(&fake_create_committed_resource);
    void *resource_vtable[12] = {};
    resource_vtable[2] =
        reinterpret_cast<void *>(&fake_render_resource_release);
    resource_vtable[11] =
        reinterpret_cast<void *>(&fake_render_resource_gpu_address);

    FakeRenderResource resources[8] = {};
    FakeDevice device = {};
    device.vtable = device_vtable;
    device.committed_resource_result_hr = S_OK;
    for (unsigned int index = 0; index < 8; ++index) {
        resources[index].vtable = resource_vtable;
        resources[index].gpu_address = 0x100000 + index * 0x1000;
        device.committed_resource_results[index] =
            reinterpret_cast<ID3D12Resource *>(&resources[index]);
    }

    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework =
        reinterpret_cast<CD3DFramework12 *>(framework_storage);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);

    alignas(D3DTransparencyPass)
        unsigned char pass_storage[sizeof(D3DTransparencyPass)] = {};
    auto *pass = reinterpret_cast<D3DTransparencyPass *>(pass_storage);
    using ResourcePtr = Microsoft::WRL::ComPtr<ID3D12Resource>;
    auto construct_resource_ptr = [](ResourcePtr &member) {
        return ::new (static_cast<void *>(std::addressof(member)))
            ResourcePtr();
    };
    ResourcePtr *resource_members[] = {
        construct_resource_ptr(pass->m_vertexUploadBuffer),
        construct_resource_ptr(pass->m_indexUploadBuffer),
        construct_resource_ptr(pass->m_additiveVertexUploadBuffer),
        construct_resource_ptr(pass->m_additiveIndexUploadBuffer),
        construct_resource_ptr(pass->m_vertexBuffer),
        construct_resource_ptr(pass->m_indexBuffer),
        construct_resource_ptr(pass->m_additiveVertexBuffer),
        construct_resource_ptr(pass->m_additiveIndexBuffer),
    };
    pass->m_pFramework = framework;

    pass->CreateBuffers();

    CHECK(D3DTransparencyPass::m_vertexBufferSize == 0x72420);
    CHECK(D3DTransparencyPass::m_indexBufferSize == 0x11940);
    CHECK(device.committed_resource_calls == 8);
    const D3D12_HEAP_TYPE expected_heap_types[8] = {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_HEAP_TYPE_DEFAULT,
    };
    const std::uint64_t expected_widths[8] = {
        0x72420, 0x11940, 0x72420, 0x11940,
        0x72420, 0x11940, 0x72420, 0x11940,
    };
    const D3D12_RESOURCE_STATES expected_states[8] = {
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COMMON,
    };
    for (unsigned int index = 0; index < 8; ++index) {
        const D3D12_HEAP_PROPERTIES &heap =
            device.committed_heap_properties_by_call[index];
        const D3D12_RESOURCE_DESC &description =
            device.committed_resource_descriptions_by_call[index];
        CHECK(heap.Type == expected_heap_types[index]);
        CHECK(heap.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_UNKNOWN);
        CHECK(heap.MemoryPoolPreference == D3D12_MEMORY_POOL_UNKNOWN);
        CHECK(heap.CreationNodeMask == 1);
        CHECK(heap.VisibleNodeMask == 1);
        CHECK(device.committed_heap_flags_by_call[index] ==
              D3D12_HEAP_FLAG_NONE);
        CHECK(description.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
        CHECK(description.Alignment == 0);
        CHECK(description.Width == expected_widths[index]);
        CHECK(description.Height == 1);
        CHECK(description.DepthOrArraySize == 1);
        CHECK(description.MipLevels == 1);
        CHECK(description.Format == DXGI_FORMAT_UNKNOWN);
        CHECK(description.SampleDesc.Count == 1);
        CHECK(description.SampleDesc.Quality == 0);
        CHECK(description.Layout == D3D12_TEXTURE_LAYOUT_ROW_MAJOR);
        CHECK(description.Flags == D3D12_RESOURCE_FLAG_NONE);
        CHECK(device.committed_initial_states_by_call[index] ==
              expected_states[index]);
        CHECK(!device.committed_clear_value_present_by_call[index]);
        CHECK(*device.committed_resource_iids_by_call[index] ==
              __uuidof(ID3D12Resource));
    }

    CHECK(pass->m_vertexUploadBuffer.Get() ==
          device.committed_resource_results[0]);
    CHECK(pass->m_indexUploadBuffer.Get() ==
          device.committed_resource_results[1]);
    CHECK(pass->m_vertexBuffer.Get() ==
          device.committed_resource_results[2]);
    CHECK(pass->m_indexBuffer.Get() ==
          device.committed_resource_results[3]);
    CHECK(pass->m_additiveVertexUploadBuffer.Get() ==
          device.committed_resource_results[4]);
    CHECK(pass->m_additiveIndexUploadBuffer.Get() ==
          device.committed_resource_results[5]);
    CHECK(pass->m_additiveVertexBuffer.Get() ==
          device.committed_resource_results[6]);
    CHECK(pass->m_additiveIndexBuffer.Get() ==
          device.committed_resource_results[7]);

    CHECK(pass->m_vertexBufferView.BufferLocation ==
          resources[2].gpu_address);
    CHECK(pass->m_vertexBufferView.SizeInBytes == 0x72420);
    CHECK(pass->m_vertexBufferView.StrideInBytes == sizeof(Vertex));
    CHECK(pass->m_indexBufferView.BufferLocation ==
          resources[3].gpu_address);
    CHECK(pass->m_indexBufferView.SizeInBytes == 0x11940);
    CHECK(pass->m_indexBufferView.Format == DXGI_FORMAT_R32_UINT);
    CHECK(pass->m_additiveVertexBufferView.BufferLocation ==
          resources[6].gpu_address);
    CHECK(pass->m_additiveVertexBufferView.SizeInBytes == 0x72420);
    CHECK(pass->m_additiveVertexBufferView.StrideInBytes == sizeof(Vertex));
    CHECK(pass->m_additiveIndexBufferView.BufferLocation ==
          resources[7].gpu_address);
    CHECK(pass->m_additiveIndexBufferView.SizeInBytes == 0x11940);
    CHECK(pass->m_additiveIndexBufferView.Format == DXGI_FORMAT_R32_UINT);
    for (unsigned int index = 0; index < 8; ++index) {
        CHECK(resources[index].gpu_address_calls ==
              ((index == 2 || index == 3 || index == 6 || index == 7)
                   ? 1U
                   : 0U));
    }

    for (unsigned int index = 8; index-- > 0;) {
        resource_members[index]->~ResourcePtr();
    }
    for (const FakeRenderResource &resource : resources) {
        CHECK(resource.release_calls == 1);
    }
    return 0;
}

static IDxcBlob *application_shader_outputs[2];
static unsigned int application_shader_compile_calls;
static std::string application_shader_names[2];
static std::vector<std::wstring> application_shader_arguments[2];
static unsigned int application_wait_for_gpu_calls;
static unsigned int application_steam_deck_calls;
static bool application_steam_deck_result;

static bool compile_application_shader_for_test(
    const char *resource_name,
    const wchar_t **arguments,
    UINT32 argument_count,
    IDxcBlob **shader)
{
    const unsigned int call = application_shader_compile_calls++;
    if (call >= 2) {
        std::abort();
    }
    application_shader_names[call] = resource_name;
    application_shader_arguments[call].clear();
    for (UINT32 index = 0; index < argument_count; ++index) {
        application_shader_arguments[call].emplace_back(arguments[index]);
    }
    *shader = application_shader_outputs[call];
    return true;
}

static void wait_for_application_gpu_for_test()
{
    ++application_wait_for_gpu_calls;
}

static bool query_application_steam_deck_for_test()
{
    ++application_steam_deck_calls;
    return application_steam_deck_result;
}

static int check_application_pipeline_description(
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC &description,
    UINT input_count,
    BOOL alpha_to_coverage,
    BOOL independent_blend,
    BOOL blend_enabled,
    D3D12_DEPTH_WRITE_MASK depth_write_mask)
{
    CHECK(description.InputLayout.NumElements == input_count);
    const char *expected_semantics[5] = {
        "POSITION", "COLOR", "TEXCOORD", "BLENDINDICES", "TEXCOORD"};
    const UINT expected_indices[5] = {0, 0, 0, 0, 1};
    const DXGI_FORMAT expected_formats[5] = {
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32_UINT,
        DXGI_FORMAT_R32G32_FLOAT,
    };
    const UINT expected_offsets[5] = {0, 16, 32, 40, 44};
    for (UINT index = 0; index < input_count; ++index) {
        const D3D12_INPUT_ELEMENT_DESC &element =
            description.InputLayout.pInputElementDescs[index];
        CHECK(std::strcmp(element.SemanticName, expected_semantics[index]) ==
              0);
        CHECK(element.SemanticIndex == expected_indices[index]);
        CHECK(element.Format == expected_formats[index]);
        CHECK(element.InputSlot == 0);
        CHECK(element.AlignedByteOffset == expected_offsets[index]);
        CHECK(element.InputSlotClass ==
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
        CHECK(element.InstanceDataStepRate == 0);
    }

    CHECK(description.BlendState.AlphaToCoverageEnable == alpha_to_coverage);
    CHECK(description.BlendState.IndependentBlendEnable == independent_blend);
    const D3D12_RENDER_TARGET_BLEND_DESC &blend =
        description.BlendState.RenderTarget[0];
    CHECK(blend.BlendEnable == blend_enabled);
    CHECK(blend.LogicOpEnable == FALSE);
    CHECK(blend.SrcBlend == D3D12_BLEND_SRC_ALPHA);
    CHECK(blend.DestBlend == D3D12_BLEND_INV_SRC_ALPHA);
    CHECK(blend.BlendOp == D3D12_BLEND_OP_ADD);
    CHECK(blend.SrcBlendAlpha == D3D12_BLEND_SRC_ALPHA);
    CHECK(blend.DestBlendAlpha == D3D12_BLEND_DEST_ALPHA);
    CHECK(blend.BlendOpAlpha == D3D12_BLEND_OP_ADD);
    CHECK(blend.LogicOp == D3D12_LOGIC_OP_CLEAR);
    CHECK(blend.RenderTargetWriteMask == D3D12_COLOR_WRITE_ENABLE_ALL);

    CHECK(description.RasterizerState.FillMode == D3D12_FILL_MODE_SOLID);
    CHECK(description.RasterizerState.CullMode == D3D12_CULL_MODE_NONE);
    CHECK(description.RasterizerState.DepthClipEnable == TRUE);
    CHECK(description.DepthStencilState.DepthEnable == TRUE);
    CHECK(description.DepthStencilState.DepthWriteMask == depth_write_mask);
    CHECK(description.DepthStencilState.DepthFunc ==
          D3D12_COMPARISON_FUNC_LESS_EQUAL);
    CHECK(description.DepthStencilState.StencilEnable == FALSE);
    CHECK(description.DepthStencilState.StencilReadMask ==
          D3D12_DEFAULT_STENCIL_READ_MASK);
    CHECK(description.DepthStencilState.StencilWriteMask ==
          D3D12_DEFAULT_STENCIL_WRITE_MASK);
    CHECK(description.SampleMask == UINT_MAX);
    CHECK(description.IBStripCutValue ==
          D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
    CHECK(description.PrimitiveTopologyType ==
          D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    CHECK(description.NumRenderTargets == 1);
    CHECK(description.RTVFormats[0] == DXGI_FORMAT_R8G8B8A8_UNORM);
    CHECK(description.DSVFormat == DXGI_FORMAT_D32_FLOAT);
    CHECK(description.SampleDesc.Count == 1);
    CHECK(description.SampleDesc.Quality == 0);
    return 0;
}

static int test_application_pipeline_creation()
{
    jpb_d3dapp_set_compile_shader_test_hook(
        compile_application_shader_for_test);
    jpb_d3dapp_set_steam_deck_test_hook(
        query_application_steam_deck_for_test);
    jpb_d3dapp_set_wait_for_gpu_test_hook(
        wait_for_application_gpu_for_test);

    void *device_vtable[11] = {};
    device_vtable[10] =
        reinterpret_cast<void *>(&fake_create_graphics_pipeline_state);
    FakeDevice device = {};
    device.vtable = device_vtable;
    device.graphics_pipeline_result_hr = S_OK;

    void *pipeline_vtable[7] = {};
    pipeline_vtable[2] = reinterpret_cast<void *>(&fake_pipeline_release);
    pipeline_vtable[6] = reinterpret_cast<void *>(&fake_pipeline_set_name);
    FakePipelineState pipelines[4] = {};
    for (FakePipelineState &pipeline : pipelines) {
        pipeline.vtable = pipeline_vtable;
    }

    unsigned char shader_bytes[6][8] = {};
    FakeDxcBlob shaders[6];
    for (unsigned int index = 0; index < 6; ++index) {
        shaders[index].buffer = shader_bytes[index];
        shaders[index].size = sizeof(shader_bytes[index]);
    }

    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework =
        reinterpret_cast<CD3DFramework12 *>(framework_storage);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    framework->m_pRootSignature =
        reinterpret_cast<ID3D12RootSignature *>(0x12345678);
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto *application =
        reinterpret_cast<CD3DApplication *>(application_storage.data());
    application->m_pFramework = framework;

    application_shader_compile_calls = 0;
    application_wait_for_gpu_calls = 0;
    application_steam_deck_calls = 0;
    application_shader_outputs[0] = &shaders[0];
    application_shader_outputs[1] = &shaders[1];
    device.graphics_pipeline_calls = 0;
    device.graphics_pipeline_results[0] =
        reinterpret_cast<ID3D12PipelineState *>(&pipelines[0]);
    CHECK(application->CreateLevelPipelineStateObject() == S_OK);
    CHECK(application_shader_compile_calls == 2);
    CHECK(application_shader_names[0] == "LevelVertexShader.hlsl");
    CHECK(application_shader_names[1] == "LevelPixelShader.hlsl");
    CHECK(application_shader_arguments[0] ==
          std::vector<std::wstring>({L"-E", L"VSMain", L"-T", L"vs_6_0"}));
    CHECK(application_shader_arguments[1] ==
          std::vector<std::wstring>({L"-E", L"PSMain", L"-T", L"ps_6_0"}));
    CHECK(application_wait_for_gpu_calls == 1);
    CHECK(application_steam_deck_calls == 0);
    CHECK(device.graphics_pipeline_calls == 1);
    CHECK(framework->m_pLevelPipelineState ==
          reinterpret_cast<ID3D12PipelineState *>(&pipelines[0]));
    CHECK(pipelines[0].set_name_calls == 1);
    CHECK(std::wcscmp(pipelines[0].last_name, L"Level Pipeline State") == 0);
    CHECK(check_application_pipeline_description(
              device.graphics_pipeline_descriptions_by_call[0],
              5, FALSE, FALSE, FALSE, D3D12_DEPTH_WRITE_MASK_ALL) == 0);
    CHECK(shaders[0].release_calls == 1);
    CHECK(shaders[1].release_calls == 1);

    application_shader_compile_calls = 0;
    application_wait_for_gpu_calls = 0;
    application_shader_outputs[0] = &shaders[2];
    application_shader_outputs[1] = &shaders[3];
    device.graphics_pipeline_calls = 0;
    device.graphics_pipeline_results[0] =
        reinterpret_cast<ID3D12PipelineState *>(&pipelines[1]);
    device.graphics_pipeline_results[1] =
        reinterpret_cast<ID3D12PipelineState *>(&pipelines[2]);
    CHECK(application->CreateTransparentPipelineStateObject() == S_OK);
    CHECK(application_shader_names[0] == "LevelVertexShader.hlsl");
    CHECK(application_shader_names[1] ==
          "LevelTransparencyPixelShader.hlsl");
    CHECK(application_wait_for_gpu_calls == 1);
    CHECK(application_steam_deck_calls == 0);
    CHECK(device.graphics_pipeline_calls == 2);
    CHECK(pipelines[1].set_name_calls == 1);
    CHECK(pipelines[2].set_name_calls == 1);
    CHECK(std::wcscmp(
              pipelines[1].last_name, L"Transparent Pipeline State") == 0);
    CHECK(std::wcscmp(
              pipelines[2].last_name,
              L"Transparent Glass Pipeline State") == 0);
    CHECK(check_application_pipeline_description(
              device.graphics_pipeline_descriptions_by_call[0],
              5, FALSE, TRUE, TRUE, D3D12_DEPTH_WRITE_MASK_ALL) == 0);
    CHECK(check_application_pipeline_description(
              device.graphics_pipeline_descriptions_by_call[1],
              5, FALSE, TRUE, TRUE, D3D12_DEPTH_WRITE_MASK_ZERO) == 0);

    application_shader_compile_calls = 0;
    application_wait_for_gpu_calls = 0;
    application_steam_deck_result = true;
    application_shader_outputs[0] = &shaders[4];
    application_shader_outputs[1] = &shaders[5];
    device.graphics_pipeline_calls = 0;
    device.graphics_pipeline_results[0] =
        reinterpret_cast<ID3D12PipelineState *>(&pipelines[3]);
    CHECK(application->CreatePipelineStateObject() == S_OK);
    CHECK(application_shader_names[0] == "VertexShader.hlsl");
    CHECK(application_shader_names[1] == "PixelShader.hlsl");
    CHECK(application_shader_arguments[1] ==
          std::vector<std::wstring>({
              L"-E", L"PSMain", L"-T", L"ps_6_0",
              L"-D", L"STEAM_DECK"}));
    CHECK(application_wait_for_gpu_calls == 1);
    CHECK(application_steam_deck_calls == 1);
    CHECK(device.graphics_pipeline_calls == 1);
    CHECK(pipelines[3].set_name_calls == 1);
    CHECK(std::wcscmp(pipelines[3].last_name, L"Pipeline State") == 0);
    CHECK(check_application_pipeline_description(
              device.graphics_pipeline_descriptions_by_call[0],
              4, TRUE, TRUE, TRUE, D3D12_DEPTH_WRITE_MASK_ALL) == 0);
    CHECK(shaders[2].release_calls == 1);
    CHECK(shaders[3].release_calls == 1);

    framework->vertexShaderBlob->Release();
    framework->pixelShaderBlob->Release();
    framework->m_pLevelPipelineState->Release();
    framework->m_pTransparentPipelineState->Release();
    framework->m_pTransparentGlassPipelineState->Release();
    framework->m_pPipelineState->Release();
    CHECK(shaders[4].release_calls == 1);
    CHECK(shaders[5].release_calls == 1);
    for (const FakePipelineState &pipeline : pipelines) {
        CHECK(pipeline.release_calls == 1);
    }

    jpb_d3dapp_set_compile_shader_test_hook(nullptr);
    jpb_d3dapp_set_steam_deck_test_hook(nullptr);
    jpb_d3dapp_set_wait_for_gpu_test_hook(nullptr);
    return 0;
}

static int test_transparency_pass_pipeline_creation()
{
    void *device_vtable[11] = {};
    device_vtable[10] =
        reinterpret_cast<void *>(&fake_create_graphics_pipeline_state);
    void *pipeline_vtable[3] = {};
    pipeline_vtable[2] = reinterpret_cast<void *>(&fake_unknown_release);
    FakeUnknown transparent_pipeline = {pipeline_vtable};
    FakeUnknown additive_pipeline = {pipeline_vtable};

    FakeDevice device = {};
    device.vtable = device_vtable;
    device.graphics_pipeline_result_hr = S_OK;
    device.graphics_pipeline_results[0] =
        reinterpret_cast<ID3D12PipelineState *>(&transparent_pipeline);
    device.graphics_pipeline_results[1] =
        reinterpret_cast<ID3D12PipelineState *>(&additive_pipeline);

    unsigned char vertex_shader_bytes[13] = {};
    unsigned char pixel_shader_bytes[17] = {};
    FakeDxcBlob vertex_shader;
    vertex_shader.buffer = vertex_shader_bytes;
    vertex_shader.size = sizeof(vertex_shader_bytes);
    FakeDxcBlob pixel_shader;
    pixel_shader.buffer = pixel_shader_bytes;
    pixel_shader.size = sizeof(pixel_shader_bytes);

    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework =
        reinterpret_cast<CD3DFramework12 *>(framework_storage);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    framework->m_pRootSignature =
        reinterpret_cast<ID3D12RootSignature *>(0x12345678);
    framework->vertexShaderBlob = &vertex_shader;

    alignas(D3DTransparencyPass)
        unsigned char pass_storage[sizeof(D3DTransparencyPass)] = {};
    auto *pass = reinterpret_cast<D3DTransparencyPass *>(pass_storage);
    using PipelinePtr = Microsoft::WRL::ComPtr<ID3D12PipelineState>;
    auto *transparent_ptr = ::new (
        static_cast<void *>(std::addressof(pass->m_TransparentPSO)))
        PipelinePtr();
    auto *additive_ptr = ::new (
        static_cast<void *>(std::addressof(pass->m_AdditivePSO)))
        PipelinePtr();
    auto *pixel_ptr = ::new (
        static_cast<void *>(std::addressof(pass->m_PixelShaderBlob)))
        Microsoft::WRL::ComPtr<IDxcBlob>();
    pixel_ptr->Attach(&pixel_shader);
    pass->m_pFramework = framework;

    pass->CreatePipelineState();

    CHECK(device.graphics_pipeline_calls == 2);
    CHECK(*device.graphics_pipeline_iids_by_call[0] ==
          __uuidof(ID3D12PipelineState));
    CHECK(*device.graphics_pipeline_iids_by_call[1] ==
          __uuidof(ID3D12PipelineState));
    CHECK(pass->m_TransparentPSO.Get() ==
          device.graphics_pipeline_results[0]);
    CHECK(pass->m_AdditivePSO.Get() ==
          device.graphics_pipeline_results[1]);

    const D3D12_GRAPHICS_PIPELINE_STATE_DESC &transparent =
        device.graphics_pipeline_descriptions_by_call[0];
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC &additive =
        device.graphics_pipeline_descriptions_by_call[1];
    CHECK(transparent.pRootSignature == framework->m_pRootSignature);
    CHECK(transparent.VS.pShaderBytecode == vertex_shader_bytes);
    CHECK(transparent.VS.BytecodeLength == sizeof(vertex_shader_bytes));
    CHECK(transparent.PS.pShaderBytecode == pixel_shader_bytes);
    CHECK(transparent.PS.BytecodeLength == sizeof(pixel_shader_bytes));
    CHECK(transparent.InputLayout.NumElements == 4);
    const char *expected_semantics[4] = {
        "POSITION", "COLOR", "TEXCOORD", "BLENDINDICES"};
    const DXGI_FORMAT expected_formats[4] = {
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32_UINT,
    };
    const UINT expected_offsets[4] = {0, 16, 32, 40};
    for (unsigned int index = 0; index < 4; ++index) {
        const D3D12_INPUT_ELEMENT_DESC &element =
            transparent.InputLayout.pInputElementDescs[index];
        CHECK(std::strcmp(element.SemanticName, expected_semantics[index]) ==
              0);
        CHECK(element.SemanticIndex == 0);
        CHECK(element.Format == expected_formats[index]);
        CHECK(element.InputSlot == 0);
        CHECK(element.AlignedByteOffset == expected_offsets[index]);
        CHECK(element.InputSlotClass ==
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
        CHECK(element.InstanceDataStepRate == 0);
    }

    const D3D12_RENDER_TARGET_BLEND_DESC &transparent_blend =
        transparent.BlendState.RenderTarget[0];
    const D3D12_RENDER_TARGET_BLEND_DESC &additive_blend =
        additive.BlendState.RenderTarget[0];
    CHECK(transparent_blend.BlendEnable == TRUE);
    CHECK(transparent_blend.LogicOpEnable == FALSE);
    CHECK(transparent_blend.SrcBlend == D3D12_BLEND_SRC_ALPHA);
    CHECK(transparent_blend.DestBlend == D3D12_BLEND_ONE);
    CHECK(transparent_blend.BlendOp == D3D12_BLEND_OP_ADD);
    CHECK(transparent_blend.SrcBlendAlpha == D3D12_BLEND_SRC_ALPHA);
    CHECK(transparent_blend.DestBlendAlpha == D3D12_BLEND_ONE);
    CHECK(transparent_blend.BlendOpAlpha == D3D12_BLEND_OP_ADD);
    CHECK(transparent_blend.LogicOp == D3D12_LOGIC_OP_NOOP);
    CHECK(transparent_blend.RenderTargetWriteMask ==
          D3D12_COLOR_WRITE_ENABLE_ALL);
    CHECK(additive_blend.BlendEnable == TRUE);
    CHECK(additive_blend.SrcBlend == D3D12_BLEND_SRC_ALPHA);
    CHECK(additive_blend.DestBlend == D3D12_BLEND_INV_SRC_ALPHA);
    CHECK(additive_blend.SrcBlendAlpha == D3D12_BLEND_SRC_ALPHA);
    CHECK(additive_blend.DestBlendAlpha ==
          D3D12_BLEND_INV_SRC_ALPHA);
    CHECK(additive_blend.BlendOp == D3D12_BLEND_OP_ADD);
    CHECK(additive_blend.BlendOpAlpha == D3D12_BLEND_OP_ADD);
    CHECK(additive_blend.RenderTargetWriteMask ==
          D3D12_COLOR_WRITE_ENABLE_ALL);

    CHECK(transparent.RasterizerState.FillMode == D3D12_FILL_MODE_SOLID);
    CHECK(transparent.RasterizerState.CullMode == D3D12_CULL_MODE_NONE);
    CHECK(transparent.RasterizerState.FrontCounterClockwise == FALSE);
    CHECK(transparent.RasterizerState.DepthBias == 0);
    CHECK(transparent.RasterizerState.DepthBiasClamp == 0.0f);
    CHECK(transparent.RasterizerState.SlopeScaledDepthBias == 0.0f);
    CHECK(transparent.RasterizerState.DepthClipEnable == TRUE);
    CHECK(transparent.RasterizerState.MultisampleEnable == FALSE);
    CHECK(transparent.RasterizerState.AntialiasedLineEnable == FALSE);
    CHECK(transparent.RasterizerState.ForcedSampleCount == 0);
    CHECK(transparent.RasterizerState.ConservativeRaster ==
          D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
    CHECK(transparent.DepthStencilState.DepthEnable == TRUE);
    CHECK(transparent.DepthStencilState.DepthWriteMask ==
          D3D12_DEPTH_WRITE_MASK_ZERO);
    CHECK(transparent.DepthStencilState.DepthFunc ==
          D3D12_COMPARISON_FUNC_LESS_EQUAL);
    CHECK(transparent.DepthStencilState.StencilEnable == FALSE);
    CHECK(transparent.DepthStencilState.StencilReadMask == 0xff);
    CHECK(transparent.DepthStencilState.StencilWriteMask == 0xff);
    CHECK(transparent.DepthStencilState.FrontFace.StencilFailOp ==
          D3D12_STENCIL_OP_KEEP);
    CHECK(transparent.DepthStencilState.FrontFace.StencilDepthFailOp ==
          D3D12_STENCIL_OP_KEEP);
    CHECK(transparent.DepthStencilState.FrontFace.StencilPassOp ==
          D3D12_STENCIL_OP_KEEP);
    CHECK(transparent.DepthStencilState.FrontFace.StencilFunc ==
          D3D12_COMPARISON_FUNC_ALWAYS);
    CHECK(std::memcmp(
              &transparent.DepthStencilState.FrontFace,
              &transparent.DepthStencilState.BackFace,
              sizeof(D3D12_DEPTH_STENCILOP_DESC)) == 0);
    CHECK(transparent.SampleMask == UINT_MAX);
    CHECK(transparent.IBStripCutValue ==
          D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED);
    CHECK(transparent.PrimitiveTopologyType ==
          D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    CHECK(transparent.NumRenderTargets == 1);
    CHECK(transparent.RTVFormats[0] == DXGI_FORMAT_R8G8B8A8_UNORM);
    CHECK(transparent.DSVFormat == DXGI_FORMAT_D32_FLOAT);
    CHECK(transparent.SampleDesc.Count == 1);
    CHECK(transparent.SampleDesc.Quality == 0);
    CHECK(std::memcmp(
              &transparent.RasterizerState,
              &additive.RasterizerState,
              sizeof(D3D12_RASTERIZER_DESC)) == 0);
    CHECK(std::memcmp(
              &transparent.DepthStencilState,
              &additive.DepthStencilState,
              sizeof(D3D12_DEPTH_STENCIL_DESC)) == 0);
    CHECK(vertex_shader.get_buffer_pointer_calls == 1);
    CHECK(vertex_shader.get_buffer_size_calls == 1);
    CHECK(pixel_shader.get_buffer_pointer_calls == 1);
    CHECK(pixel_shader.get_buffer_size_calls == 1);

    additive_ptr->~PipelinePtr();
    transparent_ptr->~PipelinePtr();
    pixel_ptr->Detach();
    pixel_ptr->~ComPtr();
    CHECK(transparent_pipeline.release_calls == 1);
    CHECK(additive_pipeline.release_calls == 1);
    CHECK(pixel_shader.release_calls == 0);
    return 0;
}

#if defined(JPB_D3DAPP_REAL_ASSET_DIR)
static bool transparency_steam_deck_result;
static unsigned int transparency_steam_deck_calls;

static bool query_transparency_steam_deck_for_test()
{
    ++transparency_steam_deck_calls;
    return transparency_steam_deck_result;
}

static int test_transparency_pass_shader_creation()
{
    char previous_dll_directory[1024] = {};
    const DWORD previous_dll_directory_length = GetDllDirectoryA(
        static_cast<DWORD>(sizeof(previous_dll_directory)),
        previous_dll_directory);
    CHECK(jpb_ResourceSetBasePath(JPB_D3DAPP_REAL_ASSET_DIR) == 1);
    CHECK(SetDllDirectoryA(JPB_D3DAPP_REAL_ASSET_DIR) != FALSE);
    jpb_d3dtransparency_set_steam_deck_test_hook(
        query_transparency_steam_deck_for_test);
    transparency_steam_deck_calls = 0;

    alignas(D3DTransparencyPass)
        unsigned char pass_storage[sizeof(D3DTransparencyPass)] = {};
    auto *pass = reinterpret_cast<D3DTransparencyPass *>(pass_storage);
    auto *pixel_ptr = ::new (
        static_cast<void *>(std::addressof(pass->m_PixelShaderBlob)))
        Microsoft::WRL::ComPtr<IDxcBlob>();

    transparency_steam_deck_result = false;
    pass->CreateShaders();
    CHECK(pass->m_PixelShaderBlob != nullptr);
    CHECK(pass->m_PixelShaderBlob->GetBufferSize() > 4);
    CHECK(std::memcmp(
              pass->m_PixelShaderBlob->GetBufferPointer(),
              "DXBC",
              4) == 0);
    const auto *desktop_begin = static_cast<const unsigned char *>(
        pass->m_PixelShaderBlob->GetBufferPointer());
    std::vector<unsigned char> desktop_shader(
        desktop_begin,
        desktop_begin + pass->m_PixelShaderBlob->GetBufferSize());

    transparency_steam_deck_result = true;
    pass->CreateShaders();
    CHECK(pass->m_PixelShaderBlob != nullptr);
    CHECK(pass->m_PixelShaderBlob->GetBufferSize() > 4);
    CHECK(std::memcmp(
              pass->m_PixelShaderBlob->GetBufferPointer(),
              "DXBC",
              4) == 0);
    const bool same_size =
        desktop_shader.size() == pass->m_PixelShaderBlob->GetBufferSize();
    CHECK(!same_size || std::memcmp(
              desktop_shader.data(),
              pass->m_PixelShaderBlob->GetBufferPointer(),
              desktop_shader.size()) != 0);
    CHECK(transparency_steam_deck_calls == 2);

    pixel_ptr->~ComPtr();
    jpb_d3dtransparency_set_steam_deck_test_hook(nullptr);
    jpb_ResourceSetBasePath(nullptr);
    if (previous_dll_directory_length == 0) {
        CHECK(SetDllDirectoryA(nullptr) != FALSE);
    } else {
        CHECK(SetDllDirectoryA(previous_dll_directory) != FALSE);
    }
    return 0;
}
#else
static int test_transparency_pass_shader_creation()
{
    return 0;
}
#endif

static int test_transparency_pass_update()
{
    void *resource_vtable[10] = {};
    resource_vtable[2] = reinterpret_cast<void *>(
        &fake_transparency_resource_release);
    resource_vtable[8] = reinterpret_cast<void *>(
        &fake_transparency_resource_map);
    resource_vtable[9] = reinterpret_cast<void *>(
        &fake_transparency_resource_unmap);
    void *command_list_vtable[27] = {};
    command_list_vtable[15] = reinterpret_cast<void *>(
        &fake_render_copy_buffer_region);
    command_list_vtable[26] = reinterpret_cast<void *>(
        &fake_render_resource_barrier);

    std::vector<unsigned char> normal_vertex_data(0x72420, 0xcd);
    std::vector<unsigned char> normal_index_data(0x11940, 0xcd);
    std::vector<unsigned char> additive_vertex_data(0x72420, 0xcd);
    std::vector<unsigned char> additive_index_data(0x11940, 0xcd);
    FakeTransparencyResource resources[8] = {};
    for (FakeTransparencyResource &resource : resources) {
        resource.vtable = resource_vtable;
        resource.map_result = S_OK;
    }
    resources[0].mapped_data = normal_vertex_data.data();
    resources[1].mapped_data = normal_index_data.data();
    resources[2].mapped_data = additive_vertex_data.data();
    resources[3].mapped_data = additive_index_data.data();

    FakeRenderCommandList command_list = {};
    command_list.vtable = command_list_vtable;
    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework =
        reinterpret_cast<CD3DFramework12 *>(framework_storage);
    framework->m_pCommandList =
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list);

    alignas(D3DTransparencyPass)
        unsigned char pass_storage[sizeof(D3DTransparencyPass)] = {};
    auto *pass = reinterpret_cast<D3DTransparencyPass *>(pass_storage);
    auto *vertices = new (&pass->m_vertices) std::vector<Vertex>();
    auto *indices = new (&pass->m_indices) std::vector<unsigned int>();
    auto *additive_vertices = new (&pass->m_additiveVertices)
        std::vector<Vertex>();
    auto *additive_indices = new (&pass->m_additiveIndices)
        std::vector<unsigned int>();
    using ResourcePtr = Microsoft::WRL::ComPtr<ID3D12Resource>;
    auto construct_resource_ptr = [](ResourcePtr &member) {
        return ::new (static_cast<void *>(std::addressof(member)))
            ResourcePtr();
    };
    ResourcePtr *resource_members[] = {
        construct_resource_ptr(pass->m_vertexUploadBuffer),
        construct_resource_ptr(pass->m_indexUploadBuffer),
        construct_resource_ptr(pass->m_additiveVertexUploadBuffer),
        construct_resource_ptr(pass->m_additiveIndexUploadBuffer),
        construct_resource_ptr(pass->m_vertexBuffer),
        construct_resource_ptr(pass->m_indexBuffer),
        construct_resource_ptr(pass->m_additiveVertexBuffer),
        construct_resource_ptr(pass->m_additiveIndexBuffer),
    };
    for (unsigned int index = 0; index < 8; ++index) {
        resource_members[index]->Attach(
            reinterpret_cast<ID3D12Resource *>(&resources[index]));
    }
    pass->m_pFramework = framework;

    Vertex first = {};
    first.pos = {1.0f, 2.0f, 3.0f, 4.0f};
    first.texIndex = 7;
    Vertex second = {};
    second.color = {0.1f, 0.2f, 0.3f, 0.4f};
    second.texIndex = 19;
    vertices->push_back(first);
    vertices->push_back(second);
    *indices = {2, 1, 0};
    Vertex additive = {};
    additive.texCoord = {0.25f, 0.75f};
    additive.texIndex = 31;
    additive_vertices->push_back(additive);
    *additive_indices = {0};

    pass->Update();

    for (unsigned int index = 0; index < 4; ++index) {
        CHECK(resources[index].map_calls == 1);
        CHECK(resources[index].map_subresource == 0);
        CHECK(resources[index].map_read_range == nullptr);
        CHECK(resources[index].unmap_calls == 1);
        CHECK(resources[index].unmap_subresource == 0);
        CHECK(resources[index].unmap_written_range == nullptr);
    }
    CHECK(std::memcmp(
              normal_vertex_data.data(),
              vertices->data(),
              vertices->size() * sizeof(Vertex)) == 0);
    CHECK(normal_vertex_data[vertices->size() * sizeof(Vertex)] == 0);
    CHECK(normal_vertex_data.back() == 0);
    CHECK(std::memcmp(
              normal_index_data.data(),
              indices->data(),
              indices->size() * sizeof(unsigned int)) == 0);
    CHECK(normal_index_data[indices->size() * sizeof(unsigned int)] == 0);
    CHECK(normal_index_data.back() == 0);
    CHECK(std::memcmp(
              additive_vertex_data.data(),
              additive_vertices->data(),
              additive_vertices->size() * sizeof(Vertex)) == 0);
    CHECK(additive_vertex_data[
              additive_vertices->size() * sizeof(Vertex)] == 0);
    CHECK(additive_vertex_data.back() == 0);
    CHECK(std::memcmp(
              additive_index_data.data(),
              additive_indices->data(),
              additive_indices->size() * sizeof(unsigned int)) == 0);
    CHECK(additive_index_data[
              additive_indices->size() * sizeof(unsigned int)] == 0);
    CHECK(additive_index_data.back() == 0);

    CHECK(command_list.barrier_calls == 4);
    CHECK(command_list.copy_buffer_calls == 4);
    const unsigned int barrier_resource_indices[4][2] = {
        {4, 5}, {4, 5}, {6, 7}, {6, 7}};
    const D3D12_RESOURCE_STATES barrier_before[4][2] = {
        {D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
         D3D12_RESOURCE_STATE_INDEX_BUFFER},
        {D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COPY_DEST},
        {D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
         D3D12_RESOURCE_STATE_INDEX_BUFFER},
        {D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COPY_DEST},
    };
    const D3D12_RESOURCE_STATES barrier_after[4][2] = {
        {D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COPY_DEST},
        {D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
         D3D12_RESOURCE_STATE_INDEX_BUFFER},
        {D3D12_RESOURCE_STATE_COPY_DEST,
         D3D12_RESOURCE_STATE_COPY_DEST},
        {D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
         D3D12_RESOURCE_STATE_INDEX_BUFFER},
    };
    for (unsigned int call = 0; call < 4; ++call) {
        CHECK(command_list.barrier_counts[call] == 2);
        for (unsigned int index = 0; index < 2; ++index) {
            const D3D12_RESOURCE_BARRIER &barrier =
                command_list.barriers[call][index];
            CHECK(barrier.Type ==
                  D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);
            CHECK(barrier.Flags == D3D12_RESOURCE_BARRIER_FLAG_NONE);
            CHECK(barrier.Transition.pResource ==
                  reinterpret_cast<ID3D12Resource *>(
                      &resources[barrier_resource_indices[call][index]]));
            CHECK(barrier.Transition.Subresource ==
                  D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
            CHECK(barrier.Transition.StateBefore ==
                  barrier_before[call][index]);
            CHECK(barrier.Transition.StateAfter ==
                  barrier_after[call][index]);
        }
    }
    const unsigned int copy_destinations[4] = {4, 5, 6, 7};
    const unsigned int copy_sources[4] = {0, 1, 2, 3};
    const UINT64 copy_sizes[4] = {0x72420, 0x11940, 0x72420, 0x11940};
    for (unsigned int call = 0; call < 4; ++call) {
        CHECK(command_list.copy_destinations[call] ==
              reinterpret_cast<ID3D12Resource *>(
                  &resources[copy_destinations[call]]));
        CHECK(command_list.copy_sources[call] ==
              reinterpret_cast<ID3D12Resource *>(
                  &resources[copy_sources[call]]));
        CHECK(command_list.copy_destination_offsets[call] == 0);
        CHECK(command_list.copy_source_offsets[call] == 0);
        CHECK(command_list.copy_sizes[call] == copy_sizes[call]);
    }
    CHECK(vertices->size() == 2);
    CHECK(indices->size() == 3);
    CHECK(additive_vertices->size() == 1);
    CHECK(additive_indices->size() == 1);

    vertices->clear();
    indices->clear();
    additive_vertices->clear();
    additive_indices->clear();
    pass->Update();
    CHECK(command_list.barrier_calls == 4);
    CHECK(command_list.copy_buffer_calls == 4);
    for (unsigned int index = 0; index < 4; ++index) {
        CHECK(resources[index].map_calls == 1);
    }

    for (unsigned int index = 8; index-- > 0;) {
        resource_members[index]->~ResourcePtr();
    }
    additive_indices->~vector();
    additive_vertices->~vector();
    indices->~vector();
    vertices->~vector();
    for (const FakeTransparencyResource &resource : resources) {
        CHECK(resource.release_calls == 1);
    }
    return 0;
}

static int test_transparency_pass_render()
{
    void *command_list_vtable[47] = {};
    command_list_vtable[13] =
        reinterpret_cast<void *>(&fake_render_draw_indexed);
    command_list_vtable[20] =
        reinterpret_cast<void *>(&fake_render_set_topology);
    command_list_vtable[21] =
        reinterpret_cast<void *>(&fake_render_set_viewports);
    command_list_vtable[22] =
        reinterpret_cast<void *>(&fake_render_set_scissors);
    command_list_vtable[25] =
        reinterpret_cast<void *>(&fake_render_set_pipeline_state);
    command_list_vtable[28] =
        reinterpret_cast<void *>(&fake_render_set_descriptor_heaps);
    command_list_vtable[30] =
        reinterpret_cast<void *>(&fake_render_set_root_signature);
    command_list_vtable[32] =
        reinterpret_cast<void *>(&fake_render_set_root_table);
    command_list_vtable[43] =
        reinterpret_cast<void *>(&fake_render_set_index_buffer);
    command_list_vtable[44] =
        reinterpret_cast<void *>(&fake_render_set_vertex_buffers);
    command_list_vtable[46] =
        reinterpret_cast<void *>(&fake_render_set_render_targets);

    FakeRenderCommandList command_list = {};
    command_list.vtable = command_list_vtable;
    FakeDescriptorHeap rtv_heap;
    rtv_heap.cpu_handle.ptr = 0x1000;
    FakeDescriptorHeap dsv_heap;
    dsv_heap.cpu_handle.ptr = 0x2000;
    FakeDescriptorHeap main_heap;
    main_heap.gpu_handle.ptr = 0x3000;
    void *pipeline_vtable[3] = {};
    pipeline_vtable[2] = reinterpret_cast<void *>(&fake_unknown_release);
    FakeUnknown transparent_pipeline = {pipeline_vtable};
    FakeUnknown additive_pipeline = {pipeline_vtable};

    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework =
        reinterpret_cast<CD3DFramework12 *>(framework_storage);
    framework->m_pCommandList =
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list);
    framework->m_pRtvDescriptorHeap = &rtv_heap;
    framework->m_RTVDescriptorSize = 0x20;
    framework->m_pRootSignature =
        reinterpret_cast<ID3D12RootSignature *>(0x4000);
    framework->m_nFrameIndex = 1;
    framework->m_pMainDescriptorHeap = &main_heap;
    framework->m_pDsDescriptorHeap = &dsv_heap;
    framework->m_Viewport = {1.0f, 2.0f, 640.0f, 480.0f, 0.0f, 1.0f};
    framework->m_ScissorRect = {3, 4, 643, 484};
    framework->m_isAMD = FALSE;

    alignas(D3DTransparencyPass)
        unsigned char pass_storage[sizeof(D3DTransparencyPass)] = {};
    auto *pass = reinterpret_cast<D3DTransparencyPass *>(pass_storage);
    auto *vertices = new (&pass->m_vertices) std::vector<Vertex>();
    auto *indices = new (&pass->m_indices) std::vector<unsigned int>();
    auto *additive_vertices = new (&pass->m_additiveVertices)
        std::vector<Vertex>();
    auto *additive_indices = new (&pass->m_additiveIndices)
        std::vector<unsigned int>();
    using PipelinePtr = Microsoft::WRL::ComPtr<ID3D12PipelineState>;
    auto *transparent_ptr = ::new (
        static_cast<void *>(std::addressof(pass->m_TransparentPSO)))
        PipelinePtr();
    auto *additive_ptr = ::new (
        static_cast<void *>(std::addressof(pass->m_AdditivePSO)))
        PipelinePtr();
    transparent_ptr->Attach(reinterpret_cast<ID3D12PipelineState *>(
        &transparent_pipeline));
    additive_ptr->Attach(reinterpret_cast<ID3D12PipelineState *>(
        &additive_pipeline));
    pass->m_vertexBufferView = {0x5000, 0x72420, sizeof(Vertex)};
    pass->m_indexBufferView = {0x6000, 0x11940, DXGI_FORMAT_R32_UINT};
    pass->m_additiveVertexBufferView = {0x7000, 0x72420, sizeof(Vertex)};
    pass->m_additiveIndexBufferView = {
        0x8000, 0x11940, DXGI_FORMAT_R32_UINT};
    pass->m_pFramework = framework;
    vertices->resize(1);
    *indices = {0, 1, 2};
    additive_vertices->resize(1);
    *additive_indices = {0, 1, 2, 2, 3, 0};

    pass->Render();

    CHECK(command_list.render_target_calls == 3);
    for (unsigned int call = 0; call < 3; ++call) {
        CHECK(command_list.render_target_handles[call].ptr == 0x1020);
        CHECK(command_list.depth_stencil_handles[call].ptr == 0x2000);
    }
    CHECK(command_list.pipeline_state_calls == 2);
    CHECK(command_list.pipeline_states[0] ==
          reinterpret_cast<ID3D12PipelineState *>(&transparent_pipeline));
    CHECK(command_list.pipeline_states[1] ==
          reinterpret_cast<ID3D12PipelineState *>(&additive_pipeline));
    CHECK(command_list.descriptor_heap_calls == 2);
    CHECK(command_list.descriptor_heaps[0] == &main_heap);
    CHECK(command_list.descriptor_heaps[1] == &main_heap);
    CHECK(command_list.root_signature_calls == 2);
    CHECK(command_list.root_signature == framework->m_pRootSignature);
    CHECK(command_list.root_table_calls == 2);
    CHECK(command_list.root_table_indices[0] == 1);
    CHECK(command_list.root_table_indices[1] == 1);
    CHECK(command_list.root_table_handles[0].ptr == 0x3000);
    CHECK(command_list.root_table_handles[1].ptr == 0x3000);
    CHECK(main_heap.gpu_handle_calls == 2);
    CHECK(command_list.viewport_count == 1);
    CHECK(std::memcmp(
              &command_list.viewport,
              &framework->m_Viewport,
              sizeof(D3D12_VIEWPORT)) == 0);
    CHECK(command_list.scissor_count == 1);
    CHECK(std::memcmp(
              &command_list.scissor,
              &framework->m_ScissorRect,
              sizeof(D3D12_RECT)) == 0);
    CHECK(command_list.topology_calls == 2);
    CHECK(command_list.topologies[0] ==
          D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CHECK(command_list.topologies[1] ==
          D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CHECK(command_list.vertex_buffer_calls == 2);
    CHECK(std::memcmp(
              &command_list.vertex_buffer_views[0],
              &pass->m_vertexBufferView,
              sizeof(D3D12_VERTEX_BUFFER_VIEW)) == 0);
    CHECK(std::memcmp(
              &command_list.vertex_buffer_views[1],
              &pass->m_additiveVertexBufferView,
              sizeof(D3D12_VERTEX_BUFFER_VIEW)) == 0);
    CHECK(command_list.index_buffer_calls == 2);
    CHECK(std::memcmp(
              &command_list.index_buffer_views[0],
              &pass->m_indexBufferView,
              sizeof(D3D12_INDEX_BUFFER_VIEW)) == 0);
    CHECK(std::memcmp(
              &command_list.index_buffer_views[1],
              &pass->m_additiveIndexBufferView,
              sizeof(D3D12_INDEX_BUFFER_VIEW)) == 0);
    CHECK(command_list.draw_indexed_calls == 2);
    CHECK(command_list.draw_index_counts[0] == 3);
    CHECK(command_list.draw_index_counts[1] == 6);
    CHECK(command_list.draw_instance_counts[0] == 1);
    CHECK(command_list.draw_instance_counts[1] == 1);
    CHECK(command_list.draw_start_indices[0] == 0);
    CHECK(command_list.draw_start_indices[1] == 0);
    CHECK(vertices->empty());
    CHECK(indices->empty());
    CHECK(additive_vertices->empty());
    CHECK(additive_indices->empty());

    command_list = {};
    command_list.vtable = command_list_vtable;
    framework->m_isAMD = TRUE;
    vertices->resize(1);
    *indices = {0, 1};
    pass->Render();
    CHECK(command_list.render_target_calls == 2);
    CHECK(command_list.draw_indexed_calls == 2);
    CHECK(command_list.draw_index_counts[0] == 3);
    CHECK(command_list.draw_index_counts[1] == 3);
    CHECK(command_list.draw_start_indices[0] == 0);
    CHECK(command_list.draw_start_indices[1] == 3);
    CHECK(vertices->empty());
    CHECK(indices->empty());

    command_list = {};
    command_list.vtable = command_list_vtable;
    pass->Render();
    CHECK(command_list.render_target_calls == 1);
    CHECK(command_list.pipeline_state_calls == 0);
    CHECK(command_list.draw_indexed_calls == 0);

    additive_ptr->~PipelinePtr();
    transparent_ptr->~PipelinePtr();
    additive_indices->~vector();
    additive_vertices->~vector();
    indices->~vector();
    vertices->~vector();
    CHECK(transparent_pipeline.release_calls == 1);
    CHECK(additive_pipeline.release_calls == 1);
    return 0;
}

static int test_draw_texture_queue()
{
    SpriteDraw first = {};
    SpriteDraw second = {};
    first.Texture = reinterpret_cast<Texture *>(0x1111222233334444ULL);
    first.SrcRect = RECT{1, 2, 31, 42};
    first.DestRect = RECT{11, 12, 113, 124};
    first.Color = DirectX::XMVECTORF32{0.1f, 0.2f, 0.3f, 0.4f};
    first.Effects = DirectX::DX12::SpriteEffects_FlipHorizontally;
    first.Origin = DirectX::XMFLOAT2{5.5f, 6.5f};
    first.Rotation = 0.75f;
    first.LayerDepth = -3.25f;
    first.ScissorRect = RECT{7, 8, 97, 108};
    first.SamplerType = TEXTURSAMPLER_POINTCLAMP;

    second.Texture = reinterpret_cast<Texture *>(0xAAAABBBBCCCCDDDDULL);
    second.SrcRect = std::nullopt;
    second.DestRect = RECT{-1, -2, 300, 400};
    second.Color = DirectX::XMVECTORF32{0.9f, 0.8f, 0.7f, 0.6f};
    second.Effects = DirectX::DX12::SpriteEffects_FlipBoth;
    second.Origin = DirectX::XMFLOAT2{-5.0f, 12.0f};
    second.Rotation = -1.5f;
    second.LayerDepth = 17.0f;
    second.ScissorRect = std::nullopt;
    second.SamplerType = TEXTURESAMPLER_LINEARCLAMP;

    std::vector<unsigned char> storage(0x80, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(storage.data());
    auto *draws = new (storage.data() + 0x58) std::vector<SpriteDraw>();
    CHECK(application->RenderUI() == S_OK);
    application->DrawTexture(first);
    CHECK(draws->size() == 1);
    CHECK(std::memcmp(&(*draws)[0], &first, sizeof(first)) == 0);
    draws->~vector();

    std::fill(storage.begin(), storage.end(), 0);
    draws = new (storage.data() + 0x58) std::vector<SpriteDraw>();
    draws->reserve(2);
    SpriteDraw *reserved_data = draws->data();
    application->DrawTexture(first);
    application->DrawTexture(second);
    CHECK(draws->data() == reserved_data);
    CHECK(draws->size() == 2);
    CHECK(std::memcmp(&(*draws)[0], &first, sizeof(first)) == 0);
    CHECK(std::memcmp(&(*draws)[1], &second, sizeof(second)) == 0);
    draws->~vector();

    SpriteDraw ordered[4] = {};
    ordered[0].LayerDepth = 7.0f;
    ordered[1].LayerDepth = -2.0f;
    ordered[2].LayerDepth = 3.5f;
    ordered[3].LayerDepth = 0.0f;
    jpb_d3dapp_sort_sprite_draws_for_test(ordered, _countof(ordered));
    CHECK(ordered[0].LayerDepth == -2.0f);
    CHECK(ordered[1].LayerDepth == 0.0f);
    CHECK(ordered[2].LayerDepth == 3.5f);
    CHECK(ordered[3].LayerDepth == 7.0f);
    return 0;
}

struct DebugMaterialCall {
    float red;
    float green;
    float blue;
    float alpha;
};

static DebugMaterialCall debug_material_calls[4];
static unsigned debug_material_call_count;

static void record_debug_material(
    D3DMATERIAL7 &,
    float red,
    float green,
    float blue,
    float alpha)
{
    if (debug_material_call_count >= 4) {
        std::abort();
    }
    debug_material_calls[debug_material_call_count++] =
        {red, green, blue, alpha};
}

static void call_svprintf(
    CD3DApplication *application,
    char *buffer,
    char *format,
    ...)
{
    va_list arguments;
    va_start(arguments, format);
    application->svprintf(buffer, format, arguments);
    va_end(arguments);
}

static int test_debug_helpers()
{
    std::vector<unsigned char> storage(sizeof(CD3DApplication), 0);
    auto *application = reinterpret_cast<CD3DApplication *>(storage.data());
    jpb_d3dapp_set_debug_material_test_hook(record_debug_material);

    debug_material_call_count = 0;
    application->debug_line(
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0x123456UL);
    CHECK(debug_material_call_count == 1);
    CHECK(debug_material_calls[0].red == 0x12 / 255.0f);
    CHECK(debug_material_calls[0].green == 0x34 / 255.0f);
    CHECK(debug_material_calls[0].blue == 0x56 / 255.0f);
    CHECK(debug_material_calls[0].alpha == 1.0f);

    debug_material_call_count = 0;
    application->debug_point(-1.0f, -2.0f, -3.0f, 99, 0xFEDCBAUL);
    CHECK(debug_material_call_count == 3);
    for (unsigned index = 0; index < 3; ++index) {
        const DebugMaterialCall &call = debug_material_calls[index];
        CHECK(call.red == 0xFE / 255.0f);
        CHECK(call.green == 0xDC / 255.0f);
        CHECK(call.blue == 0xBA / 255.0f);
        CHECK(call.alpha == 1.0f);
    }

    char output[256] = {};
    char format[] = "%s:%d:%08X";
    call_svprintf(application, output, format, "audit", -17, 0xC0FFEE);
    CHECK(std::strcmp(output, "audit:-17:00C0FFEE") == 0);

    char marker[] = "!";
    application->svprintf(nullptr, marker, nullptr);
    char discarded_format[] = "%s %d";
    application->debug_printf(discarded_format, "ignored", 42);

    jpb_d3dapp_set_debug_material_test_hook(nullptr);
    return 0;
}

static std::string message_box_text;
static std::string message_box_caption;
static HWND message_box_window;
static UINT message_box_type;
static unsigned message_box_call_count;

static int WINAPI record_message_box(
    HWND window,
    LPCSTR text,
    LPCSTR caption,
    UINT type)
{
    ++message_box_call_count;
    message_box_window = window;
    message_box_text = text;
    message_box_caption = caption;
    message_box_type = type;
    return 17;
}

static int test_framework_error_messages()
{
    std::vector<unsigned char> storage(0x38420, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(storage.data());
    std::strcpy(
        reinterpret_cast<char *>(application) + 0x38308,
        "JPB canonical caption");
    jpb_d3dapp_set_message_box_test_hook(record_message_box);

    struct ErrorMessageCase {
        unsigned long result;
        const char *text;
    };
    const ErrorMessageCase cases[] = {
        {0x8007000EUL, "Not enough memory!"},
        {0x81000001UL, "Could not create DirectDraw!"},
        {0x81000002UL,
         "Enumeration failed. Your system may be in an\n"
         "unstable state and need to be rebooted"},
        {0x81000003UL,
         "Could not find any compatible devices.\n\n"
         "Try enabling the reference rasterizer using\n"
         "EnableRefRast.reg."},
        {0x81000004UL,
         "Could not find any compatible Direct3D devices."},
        {0x82000000UL,
         "Generic initialization error.\n\n"
         "Enable debug output for detailed information."},
        {0x82000001UL, "No DirectDraw"},
        {0x82000002UL, "No Direct3D"},
        {0x82000003UL,
         "This program requires a 16-bit (or higher) display mode\n"
         "to run in a window.\n\n"
         "Please switch your desktop settings accordingly."},
        {0x82000004UL, "Could not set Cooperative Level"},
        {0x82000005UL, "Could not create the Direct3DDevice object."},
        {0x82000006UL, "No ZBuffer"},
        {0x82000007UL,
         "Invalid Z-buffer depth. Try switching modes\n"
         "from 16- to 32-bit (or vice versa)"},
        {0x82000008UL, "No Viewport"},
        {0x82000009UL, "No primary"},
        {0x8200000AUL, "No Clipper"},
        {0x8200000BUL, "Bad display mode"},
        {0x8200000CUL, "No backbuffer"},
        {0x8200000DUL,
         "A DDraw object has a non-zero reference\n"
         "count (meaning it was not properly cleaned up)."},
        {0x8200000EUL, "No render target"},
        {0x8876017CUL,
         "There was insufficient video memory to use the\n"
         "hardware device."},
    };

    message_box_call_count = 0;
    for (const ErrorMessageCase &test_case : cases) {
        application->DisplayFrameworkError(
            static_cast<HRESULT>(test_case.result), 0);
        CHECK(message_box_text == test_case.text);
        CHECK(message_box_window == nullptr);
        CHECK(message_box_caption == "JPB canonical caption");
        CHECK(message_box_type == MB_ICONEXCLAMATION);
    }
    CHECK(message_box_call_count == _countof(cases));

    application->DisplayFrameworkError(
        static_cast<HRESULT>(0x82000005UL), 2);
    CHECK(message_box_call_count == _countof(cases) + 1);
    CHECK(message_box_text ==
          "Could not create the Direct3DDevice object.\n"
          "The 3D hardware chipset may not support\n"
          "rendering in the current display mode.\n\n"
          "Attempting software rasterization.");
    CHECK(message_box_type == MB_ICONEXCLAMATION);

    application->DisplayFrameworkError(E_OUTOFMEMORY, 1);
    CHECK(message_box_call_count == _countof(cases) + 2);
    CHECK(message_box_text ==
          "Not enough memory!\n\nThis program will now exit.");
    CHECK(message_box_type == MB_ICONHAND);

    application->DisplayFrameworkError(E_FAIL, 99);
    CHECK(message_box_call_count == _countof(cases) + 3);
    CHECK(message_box_text ==
          "Generic application error.\n\n"
          "Enable debug output for detailed information.");
    CHECK(message_box_type == MB_ICONEXCLAMATION);

    jpb_d3dapp_set_message_box_test_hook(nullptr);
    return 0;
}

static int test_registry_accessors()
{
    std::vector<unsigned char> storage(0x383A0, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(storage.data());
    char company[64];
    std::snprintf(
        company,
        sizeof(company),
        "OpenJPB-Codex-Audit-%lu",
        GetCurrentProcessId());
    char title[] = "d3dapp-registry-test";
    application->SetCompanyTitle(company);
    application->SetAppTitle(title);
    char expected_key[160];
    std::snprintf(
        expected_key,
        sizeof(expected_key),
        "software\\%s\\%s",
        company,
        title);
    CHECK(std::strcmp(application->RegKeyName(), expected_key) == 0);
    RegDeleteTreeA(HKEY_CURRENT_USER, expected_key);

    HKEY key = nullptr;
    CHECK(application->RegOpen(&key) == 0);
    char dword_name[] = "AuditDWord";
    CHECK(application->GetRegistryDWord(dword_name, 0xAABBCCDD) ==
          0xAABBCCDD);
    CHECK(application->RegCreate(&key) == 1);
    CHECK(key != nullptr);
    CHECK(RegCloseKey(key) == ERROR_SUCCESS);

    g_pD3DApp = application;
    application->SetRegistryDWord(dword_name, 0x1234ABCD);
    CHECK(application->GetRegistryDWord(dword_name, 0) == 0x1234ABCD);
    CHECK(application->RegOpen(&key) == 1);
    CHECK(key != nullptr);
    CHECK(RegCloseKey(key) == ERROR_SUCCESS);

    unsigned char binary_value[7] = {0, 1, 2, 0x7F, 0x80, 0xFE, 0xFF};
    unsigned char binary_output[7] = {};
    char binary_name[] = "AuditBinary";
    application->SetRegistryBinary(
        binary_name, binary_value, sizeof(binary_value));
    CHECK(application->GetRegistryBinary(
              binary_name, binary_output, sizeof(binary_output)) == 1);
    CHECK(std::memcmp(
              binary_output, binary_value, sizeof(binary_value)) == 0);
    CHECK(application->GetRegistryBinary(
              dword_name, binary_output, sizeof(binary_output)) == 0);
    char missing_name[] = "MissingBinary";
    CHECK(application->GetRegistryBinary(
              missing_name, binary_output, sizeof(binary_output)) == 0);

    char string_name[] = "AuditString";
    char string_value[] = "canonical registry text";
    application->SetRegistryString(string_name, string_value);
    CHECK(RegOpenKeyExA(
              HKEY_CURRENT_USER,
              expected_key,
              0,
              KEY_QUERY_VALUE,
              &key) == ERROR_SUCCESS);
    DWORD type = 0;
    DWORD size = 0;
    CHECK(RegQueryValueExA(
              key, string_name, nullptr, &type, nullptr, &size) ==
          ERROR_SUCCESS);
    CHECK(type == REG_SZ);
    CHECK(size == sizeof(string_value));
    char string_output[64] = {};
    CHECK(RegQueryValueExA(
              key,
              string_name,
              nullptr,
              &type,
              reinterpret_cast<BYTE *>(string_output),
              &size) == ERROR_SUCCESS);
    CHECK(std::strcmp(string_output, string_value) == 0);
    CHECK(RegCloseKey(key) == ERROR_SUCCESS);
    char missing_string[] = "MissingString";
    CHECK(application->GetRegistryString(
              missing_string, string_output, sizeof(string_output)) == 0);

    CHECK(RegDeleteTreeA(HKEY_CURRENT_USER, expected_key) == ERROR_SUCCESS);
    g_pD3DApp = nullptr;
    return 0;
}

static int test_resource_transition_and_resolution()
{
    void *vtable[27] = {};
    vtable[26] = reinterpret_cast<void *>(&fake_resource_barrier);
    FakeCommandList command_list = {};
    command_list.vtable = vtable;
    auto *resource = reinterpret_cast<ID3D12Resource *>(0x12345678);
    TransitionResource(
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list),
        resource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    CHECK(command_list.calls == 1);
    CHECK(command_list.barrier_count == 1);
    CHECK(command_list.barrier.Type ==
          D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);
    CHECK(command_list.barrier.Flags == D3D12_RESOURCE_BARRIER_FLAG_NONE);
    CHECK(command_list.barrier.Transition.pResource == resource);
    CHECK(command_list.barrier.Transition.Subresource ==
          D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    CHECK(command_list.barrier.Transition.StateBefore ==
          D3D12_RESOURCE_STATE_COPY_DEST);
    CHECK(command_list.barrier.Transition.StateAfter ==
          D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    std::vector<unsigned char> storage(sizeof(CD3DApplication), 0);
    auto *application =
        reinterpret_cast<CD3DApplication *>(storage.data());
    resolutionUpdated = 0;
    newWidth = 0;
    newHeight = 0;
    newWindowMode = 0;
    application->UpdateResolution(2560, 1440, 2);
    CHECK(resolutionUpdated == 1);
    CHECK(newWidth == 2560);
    CHECK(newHeight == 1440);
    CHECK(newWindowMode == 2);
    return 0;
}

static int test_command_list_and_viewport_initialization()
{
    void *device_vtable[13] = {};
    device_vtable[9] =
        reinterpret_cast<void *>(&fake_create_command_allocator);
    device_vtable[12] = reinterpret_cast<void *>(&fake_create_command_list);

    FakeDevice device = {};
    device.vtable = device_vtable;
    device.allocator_results[0] =
        reinterpret_cast<ID3D12CommandAllocator *>(0x11110000);
    device.allocator_results[1] =
        reinterpret_cast<ID3D12CommandAllocator *>(0x22220000);
    device.allocator_results_hr[0] = S_OK;
    device.allocator_results_hr[1] = S_OK;
    device.command_list_result =
        reinterpret_cast<ID3D12GraphicsCommandList *>(0x33330000);
    device.command_list_result_hr = S_OK;

    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0xCD);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    framework->m_nCurrentFrameIndex = 1;

    std::vector<unsigned char> application_storage(0x383A0, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;

    CHECK(application->InitializeCommandList() == S_OK);
    CHECK(device.allocator_calls == 2);
    CHECK(device.allocator_types[0] == D3D12_COMMAND_LIST_TYPE_DIRECT);
    CHECK(device.allocator_types[1] == D3D12_COMMAND_LIST_TYPE_DIRECT);
    CHECK(*device.allocator_iids[0] == __uuidof(ID3D12CommandAllocator));
    CHECK(*device.allocator_iids[1] == __uuidof(ID3D12CommandAllocator));
    CHECK(framework->m_pCommandAllocators[0] ==
          device.allocator_results[0]);
    CHECK(framework->m_pCommandAllocators[1] ==
          device.allocator_results[1]);
    CHECK(device.command_list_calls == 1);
    CHECK(device.node_mask == 0);
    CHECK(device.command_list_type == D3D12_COMMAND_LIST_TYPE_DIRECT);
    CHECK(device.command_list_allocator == device.allocator_results[1]);
    CHECK(device.initial_pipeline_state == nullptr);
    CHECK(*device.command_list_iid == __uuidof(ID3D12GraphicsCommandList));
    CHECK(framework->m_pCommandList == device.command_list_result);

    device = {};
    device.vtable = device_vtable;
    device.allocator_results_hr[0] = E_ACCESSDENIED;
    CHECK(application->InitializeCommandList() == E_ACCESSDENIED);
    CHECK(device.allocator_calls == 1);
    CHECK(device.command_list_calls == 0);

    device = {};
    device.vtable = device_vtable;
    device.allocator_results[0] =
        reinterpret_cast<ID3D12CommandAllocator *>(0x44440000);
    device.allocator_results_hr[0] = S_OK;
    device.allocator_results_hr[1] = E_OUTOFMEMORY;
    CHECK(application->InitializeCommandList() == E_OUTOFMEMORY);
    CHECK(device.allocator_calls == 2);
    CHECK(device.command_list_calls == 0);

    device = {};
    device.vtable = device_vtable;
    device.allocator_results[0] =
        reinterpret_cast<ID3D12CommandAllocator *>(0x55550000);
    device.allocator_results[1] =
        reinterpret_cast<ID3D12CommandAllocator *>(0x66660000);
    device.allocator_results_hr[0] = S_OK;
    device.allocator_results_hr[1] = S_OK;
    device.command_list_result_hr = DXGI_ERROR_DEVICE_REMOVED;
    CHECK(application->InitializeCommandList() == DXGI_ERROR_DEVICE_REMOVED);
    CHECK(device.allocator_calls == 2);
    CHECK(device.command_list_calls == 1);

    *reinterpret_cast<UINT *>(application_storage.data() + 0x3831C) = 1920;
    *reinterpret_cast<UINT *>(application_storage.data() + 0x38320) = 1080;
    application->InitializeViewportAndScissorRect();
    CHECK(framework->m_Viewport.TopLeftX == 0.0f);
    CHECK(framework->m_Viewport.TopLeftY == 0.0f);
    CHECK(framework->m_Viewport.Width == 1920.0f);
    CHECK(framework->m_Viewport.Height == 1080.0f);
    CHECK(framework->m_Viewport.MinDepth == 0.0f);
    CHECK(framework->m_Viewport.MaxDepth == 1.0f);
    CHECK(framework->m_ScissorRect.left == 0);
    CHECK(framework->m_ScissorRect.top == 0);
    CHECK(framework->m_ScissorRect.right == 1920);
    CHECK(framework->m_ScissorRect.bottom == 1080);
    return 0;
}

static int test_change_environment_failure_gates()
{
    jpb_d3dapp_set_framework_destroy_objects_test_hook(
        &fake_destroy_framework_objects);
    jpb_d3dapp_set_create_dxgi_factory_test_hook(
        &fake_create_dxgi_factory);
    jpb_d3dapp_set_message_box_test_hook(record_message_box);

    void *application_vtable[3] = {};
    application_vtable[2] = reinterpret_cast<void *>(
        &fake_change_delete_device_objects);
    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework =
        reinterpret_cast<CD3DFramework12 *>(framework_storage);
    std::vector<unsigned char> application_storage(0x38350, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    *reinterpret_cast<void ***>(application) = application_vtable;
    application->m_pFramework = framework;

    fake_change_delete_device_objects_calls = 0;
    fake_destroy_framework_calls = 0;
    fake_destroyed_framework = nullptr;
    fake_destroy_framework_result = E_ACCESSDENIED;
    fake_create_factory_state = {};
    fake_create_factory_state.result = E_NOINTERFACE;
    message_box_call_count = 0;
    CHECK(application->Change3DEnvironment() == E_ACCESSDENIED);
    CHECK(fake_change_delete_device_objects_calls == 1);
    CHECK(fake_destroy_framework_calls == 1);
    CHECK(fake_destroyed_framework == framework);
    CHECK(fake_create_factory_state.calls == 0);
    CHECK(message_box_call_count == 1);

    fake_destroy_framework_result = S_OK;
    CHECK(application->Change3DEnvironment() == E_NOINTERFACE);
    CHECK(fake_change_delete_device_objects_calls == 2);
    CHECK(fake_destroy_framework_calls == 2);
    CHECK(fake_create_factory_state.calls == 1);
    CHECK(message_box_call_count == 2);
    CHECK(*reinterpret_cast<BOOL *>(
              application_storage.data() + 0xE8) == FALSE);
    CHECK(framework->m_graphicsMemory == nullptr);

    jpb_d3dapp_set_create_dxgi_factory_test_hook(nullptr);
    jpb_d3dapp_set_framework_destroy_objects_test_hook(nullptr);
    jpb_d3dapp_set_message_box_test_hook(nullptr);
    return 0;
}

static int test_render_3d_environment()
{
    jpb_d3dapp_set_create_dxgi_factory_test_hook(
        &fake_create_dxgi_factory);
    jpb_d3dapp_set_wait_for_gpu_test_hook(
        wait_for_application_gpu_for_test);
    jpb_d3dapp_set_sdl_get_window_flags_test_hook(
        fake_sdl_get_window_flags);
    jpb_d3dapp_set_sdl_destroy_window_test_hook(
        fake_sdl_destroy_window);
    jpb_d3dapp_set_sdl_quit_test_hook(fake_sdl_quit);

    void *application_vtable[5] = {};
    application_vtable[4] = reinterpret_cast<void *>(
        &fake_render_environment);
    void *swap_chain_vtable[9] = {};
    swap_chain_vtable[2] = reinterpret_cast<void *>(
        &fake_swap_chain_release);
    swap_chain_vtable[8] = reinterpret_cast<void *>(
        &fake_swap_chain_present);
    FakeSwapChain swap_chain = {};
    swap_chain.vtable = swap_chain_vtable;

    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage);
    framework->m_pSwapChain = reinterpret_cast<IDXGISwapChain3 *>(
        &swap_chain);
    std::vector<unsigned char> application_storage(0x310978, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    *reinterpret_cast<void ***>(application) = application_vtable;
    application->m_pFramework = framework;
    *reinterpret_cast<DWORD *>(application_storage.data() + 0x34) =
        GetTickCount();

    fake_render_environment_calls = 0;
    fake_render_environment_result = E_ABORT;
    swap_chain.present_result = S_OK;
    CHECK(application->Render3DEnvironment() == E_ABORT);
    CHECK(fake_render_environment_calls == 1);
    CHECK(swap_chain.present_calls == 0);

    fake_render_environment_result = S_OK;
    *reinterpret_cast<int *>(
        application_storage.data() + 0x310970) = 17;
    CHECK(application->Render3DEnvironment() == S_OK);
    CHECK(fake_render_environment_calls == 2);
    CHECK(fake_render_environment_time >= 0.0f);
    CHECK(swap_chain.present_calls == 1);
    CHECK(swap_chain.sync_interval == 1);
    CHECK(swap_chain.present_flags == 0);
    CHECK(*reinterpret_cast<int *>(
              application_storage.data() + 0x310970) == 0);

    swap_chain.present_result = E_NOINTERFACE;
    CHECK(application->Render3DEnvironment() == E_NOINTERFACE);

    swap_chain.present_result = DXGI_ERROR_DEVICE_RESET;
    CHECK(application->Render3DEnvironment() == S_OK);
    CHECK(fake_sdl_window_flags_calls == 0);

    void *sdl_window = reinterpret_cast<void *>(0x12340000);
    *reinterpret_cast<void **>(application_storage.data() + 0xD8) =
        sdl_window;
    fake_sdl_window_flags_result = 0x1000;
    CHECK(application->Render3DEnvironment() == S_OK);
    CHECK(fake_sdl_window_flags_calls == 1);
    CHECK(fake_sdl_window_flags_window == sdl_window);

    void *unknown_vtable[3] = {};
    unknown_vtable[2] = reinterpret_cast<void *>(
        &fake_unknown_release);
    FakeUnknown owners[5] = {};
    for (FakeUnknown &owner : owners) {
        owner.vtable = unknown_vtable;
    }
    framework->m_pRenderTargets[0] =
        reinterpret_cast<ID3D12Resource *>(&owners[0]);
    framework->m_pRenderTargets[1] =
        reinterpret_cast<ID3D12Resource *>(&owners[1]);
    framework->m_pRtvDescriptorHeap =
        reinterpret_cast<ID3D12DescriptorHeap *>(&owners[2]);
    framework->m_pDevice =
        reinterpret_cast<ID3D12Device *>(&owners[3]);
    framework->m_pFactory =
        reinterpret_cast<IDXGIFactory6 *>(&owners[4]);
    swap_chain.present_result = DXGI_ERROR_DEVICE_HUNG;
    fake_create_factory_state = {};
    fake_create_factory_state.result = E_NOINTERFACE;
    application_wait_for_gpu_calls = 0;
    fake_sdl_destroy_window_calls = 0;
    fake_sdl_destroyed_window = nullptr;
    fake_sdl_quit_calls = 0;
    CHECK(application->Render3DEnvironment() == S_OK);
    CHECK(application_wait_for_gpu_calls == 1);
    CHECK(swap_chain.release_calls == 1);
    for (const FakeUnknown &owner : owners) {
        CHECK(owner.release_calls == 1);
    }
    CHECK(framework->m_pRenderTargets[0] == nullptr);
    CHECK(framework->m_pRenderTargets[1] == nullptr);
    CHECK(framework->m_pRtvDescriptorHeap == nullptr);
    CHECK(framework->m_pSwapChain == nullptr);
    CHECK(framework->m_pDevice == nullptr);
    CHECK(framework->m_pFactory == nullptr);
    CHECK(fake_sdl_destroy_window_calls == 1);
    CHECK(fake_sdl_destroyed_window == sdl_window);
    CHECK(*reinterpret_cast<void **>(
              application_storage.data() + 0xD8) == nullptr);
    CHECK(fake_sdl_quit_calls == 1);
    CHECK(fake_create_factory_state.calls == 1);

    jpb_d3dapp_set_sdl_quit_test_hook(nullptr);
    jpb_d3dapp_set_sdl_destroy_window_test_hook(nullptr);
    jpb_d3dapp_set_sdl_get_window_flags_test_hook(nullptr);
    jpb_d3dapp_set_wait_for_gpu_test_hook(nullptr);
    jpb_d3dapp_set_create_dxgi_factory_test_hook(nullptr);
    return 0;
}

static int test_render_menu_texture_upload()
{
    std::vector<unsigned char> empty_application_storage(
        sizeof(CD3DApplication), 0);
    auto &empty_application = *reinterpret_cast<CD3DApplication *>(
        empty_application_storage.data());
    application_wait_for_gpu_calls = 0;
    empty_application.RenderMenuTexture(false);
    CHECK(application_wait_for_gpu_calls == 0);

    void *device_vtable[39] = {};
    device_vtable[2] = reinterpret_cast<void *>(&fake_device_release);
    device_vtable[18] = reinterpret_cast<void *>(
        &fake_create_shader_resource_view);
    device_vtable[27] = reinterpret_cast<void *>(
        &fake_create_committed_resource);
    device_vtable[38] = reinterpret_cast<void *>(
        &fake_get_copyable_footprints);
    void *command_list_vtable[27] = {};
    command_list_vtable[16] = reinterpret_cast<void *>(
        &fake_render_copy_texture_region);
    command_list_vtable[26] = reinterpret_cast<void *>(
        &fake_render_resource_barrier);

    FakeDevice device = {};
    device.vtable = device_vtable;
    device.committed_resource_result_hr = S_OK;
    device.copyable_required_size = 512;

    FakeMenuResource texture;
    texture.device = reinterpret_cast<ID3D12Device *>(&device);
    texture.description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture.description.Width = 2;
    texture.description.Height = 2;
    texture.description.DepthOrArraySize = 1;
    texture.description.MipLevels = 1;
    texture.description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture.description.SampleDesc.Count = 1;
    unsigned char upload_memory[512];
    std::memset(upload_memory, 0xCC, sizeof(upload_memory));
    FakeMenuResource upload;
    upload.device = reinterpret_cast<ID3D12Device *>(&device);
    upload.mapped_data = upload_memory;
    upload.description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload.description.Width = sizeof(upload_memory);
    upload.description.Height = 1;
    upload.description.DepthOrArraySize = 1;
    upload.description.MipLevels = 1;
    upload.description.SampleDesc.Count = 1;
    upload.description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    device.committed_resource_results[0] = &texture;
    device.committed_resource_results[1] = &upload;

    FakeRenderCommandList command_list = {};
    command_list.vtable = command_list_vtable;
    FakeDescriptorHeap descriptor_heap;
    descriptor_heap.cpu_handle.ptr = 0x12345678;

    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework =
        reinterpret_cast<CD3DFramework12 *>(framework_storage);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    framework->m_pCommandList =
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list);
    framework->m_pMainDescriptorHeap = &descriptor_heap;
    framework->m_pSDLRenderer =
        reinterpret_cast<SDL_Renderer *>(0x1234);
    framework->m_pSDLRenderTarget =
        reinterpret_cast<SDL_Texture *>(0x5678);

    std::vector<unsigned char> application_storage(0x100, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;

    fake_sdl_query_texture_calls = 0;
    fake_sdl_query_texture_format = 0x16762004;
    fake_sdl_query_texture_access = 2;
    fake_sdl_query_texture_width = 2;
    fake_sdl_query_texture_height = 2;
    fake_sdl_render_present_calls = 0;
    fake_sdl_render_read_pixels_calls = 0;
    application_wait_for_gpu_calls = 0;
    jpb_d3dapp_set_wait_for_gpu_test_hook(
        &wait_for_application_gpu_for_test);
    jpb_d3dapp_set_sdl_query_texture_test_hook(
        &fake_sdl_query_texture);
    jpb_d3dapp_set_sdl_render_present_test_hook(
        &fake_sdl_render_present);
    jpb_d3dapp_set_sdl_render_read_pixels_test_hook(
        &fake_sdl_render_read_pixels);

    application->RenderMenuTexture(true);

    CHECK(application_wait_for_gpu_calls == 1);
    CHECK(fake_sdl_query_texture_calls == 1);
    CHECK(fake_sdl_query_texture_texture ==
          framework->m_pSDLRenderTarget);
    CHECK(fake_sdl_render_present_calls == 1);
    CHECK(fake_sdl_render_present_renderer ==
          framework->m_pSDLRenderer);
    CHECK(fake_sdl_render_read_pixels_calls == 1);
    CHECK(fake_sdl_render_read_pixels_renderer ==
          framework->m_pSDLRenderer);
    CHECK(fake_sdl_render_read_pixels_rect == nullptr);
    CHECK(fake_sdl_render_read_pixels_format == 0x16762004);
    CHECK(fake_sdl_render_read_pixels_pitch == 8);

    CHECK(device.committed_resource_calls == 2);
    const D3D12_HEAP_PROPERTIES &default_heap =
        device.committed_heap_properties_by_call[0];
    CHECK(default_heap.Type == D3D12_HEAP_TYPE_DEFAULT);
    CHECK(default_heap.CPUPageProperty ==
          D3D12_CPU_PAGE_PROPERTY_UNKNOWN);
    CHECK(default_heap.MemoryPoolPreference == D3D12_MEMORY_POOL_UNKNOWN);
    CHECK(default_heap.CreationNodeMask == 1);
    CHECK(default_heap.VisibleNodeMask == 1);
    const D3D12_RESOURCE_DESC &texture_description =
        device.committed_resource_descriptions_by_call[0];
    CHECK(texture_description.Dimension ==
          D3D12_RESOURCE_DIMENSION_TEXTURE2D);
    CHECK(texture_description.Width == 2);
    CHECK(texture_description.Height == 2);
    CHECK(texture_description.DepthOrArraySize == 1);
    CHECK(texture_description.MipLevels == 1);
    CHECK(texture_description.Format == DXGI_FORMAT_R8G8B8A8_UNORM);
    CHECK(texture_description.SampleDesc.Count == 1);
    CHECK(texture_description.SampleDesc.Quality == 0);
    CHECK(texture_description.Layout == D3D12_TEXTURE_LAYOUT_UNKNOWN);
    CHECK(texture_description.Flags == D3D12_RESOURCE_FLAG_NONE);
    CHECK(device.committed_initial_states_by_call[0] ==
          D3D12_RESOURCE_STATE_COMMON);
    CHECK(!device.committed_clear_value_present_by_call[0]);

    const D3D12_HEAP_PROPERTIES &upload_heap =
        device.committed_heap_properties_by_call[1];
    CHECK(upload_heap.Type == D3D12_HEAP_TYPE_UPLOAD);
    CHECK(upload_heap.CreationNodeMask == 1);
    CHECK(upload_heap.VisibleNodeMask == 1);
    const D3D12_RESOURCE_DESC &upload_description =
        device.committed_resource_descriptions_by_call[1];
    CHECK(upload_description.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
    CHECK(upload_description.Width == 512);
    CHECK(upload_description.Height == 1);
    CHECK(upload_description.DepthOrArraySize == 1);
    CHECK(upload_description.MipLevels == 1);
    CHECK(upload_description.Format == DXGI_FORMAT_UNKNOWN);
    CHECK(upload_description.SampleDesc.Count == 1);
    CHECK(upload_description.Layout == D3D12_TEXTURE_LAYOUT_ROW_MAJOR);
    CHECK(upload_description.Flags == D3D12_RESOURCE_FLAG_NONE);
    CHECK(device.committed_initial_states_by_call[1] ==
          D3D12_RESOURCE_STATE_GENERIC_READ);
    CHECK(!device.committed_clear_value_present_by_call[1]);

    CHECK(texture.set_name_calls == 1);
    CHECK(std::wcscmp(texture.last_name, L"SDLRenderTexture") == 0);
    CHECK(device.copyable_footprint_calls == 2);
    CHECK(device.copyable_first_subresources[0] == 0);
    CHECK(device.copyable_subresource_counts[0] == 1);
    CHECK(device.copyable_base_offsets[0] == 0);
    CHECK(device.copyable_first_subresources[1] == 0);
    CHECK(device.copyable_subresource_counts[1] == 1);
    CHECK(device.copyable_base_offsets[1] == 0);
    CHECK(device.release_calls == 2);
    CHECK(upload.map_calls == 1);
    CHECK(upload.map_subresource == 0);
    CHECK(upload.map_read_range == nullptr);
    CHECK(upload.unmap_calls == 1);
    CHECK(upload.unmap_subresource == 0);
    CHECK(upload.unmap_written_range == nullptr);
    for (int index = 0; index < 8; ++index) {
        CHECK(upload_memory[index] == index + 1);
        CHECK(upload_memory[256 + index] == index + 9);
    }

    CHECK(command_list.copy_texture_calls == 1);
    CHECK(command_list.copy_texture_destinations[0].pResource == &texture);
    CHECK(command_list.copy_texture_destinations[0].Type ==
          D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX);
    CHECK(command_list.copy_texture_destinations[0].SubresourceIndex == 0);
    CHECK(command_list.copy_texture_sources[0].pResource == &upload);
    CHECK(command_list.copy_texture_sources[0].Type ==
          D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT);
    CHECK(command_list.copy_texture_sources[0]
              .PlacedFootprint.Footprint.RowPitch == 256);
    CHECK(command_list.copy_texture_x[0] == 0);
    CHECK(command_list.copy_texture_y[0] == 0);
    CHECK(command_list.copy_texture_z[0] == 0);
    CHECK(command_list.barrier_calls == 1);
    CHECK(command_list.barrier_count == 1);
    CHECK(command_list.barrier.Type ==
          D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);
    CHECK(command_list.barrier.Transition.pResource == &texture);
    CHECK(command_list.barrier.Transition.Subresource ==
          D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    CHECK(command_list.barrier.Transition.StateBefore ==
          D3D12_RESOURCE_STATE_COPY_DEST);
    CHECK(command_list.barrier.Transition.StateAfter ==
          D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    CHECK(device.shader_resource_view_calls == 1);
    CHECK(device.shader_resource_view_resource == &texture);
    CHECK(device.shader_resource_view_description.Format ==
          DXGI_FORMAT_R8G8B8A8_UNORM);
    CHECK(device.shader_resource_view_description.ViewDimension ==
          D3D12_SRV_DIMENSION_TEXTURE2D);
    CHECK(device.shader_resource_view_description.Shader4ComponentMapping ==
          D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
    CHECK(device.shader_resource_view_description.Texture2D.MipLevels == 1);
    CHECK(device.shader_resource_view_handle.ptr == 0x12345678);
    CHECK(descriptor_heap.cpu_handle_calls == 1);
    CHECK(*reinterpret_cast<ID3D12Resource **>(
              application_storage.data() + 0x10) == &texture);
    CHECK(*reinterpret_cast<ID3D12Resource **>(
              application_storage.data() + 0x18) == &upload);

    jpb_d3dapp_set_sdl_render_read_pixels_test_hook(nullptr);
    jpb_d3dapp_set_sdl_render_present_test_hook(nullptr);
    jpb_d3dapp_set_sdl_query_texture_test_hook(nullptr);
    jpb_d3dapp_set_wait_for_gpu_test_hook(nullptr);
    return 0;
}

static int test_level_draw_passes()
{
    void *resource_vtable[12] = {};
    resource_vtable[11] = reinterpret_cast<void *>(
        &fake_render_resource_gpu_address);
    void *command_list_vtable[45] = {};
    command_list_vtable[13] = reinterpret_cast<void *>(
        &fake_render_draw_indexed);
    command_list_vtable[20] = reinterpret_cast<void *>(
        &fake_render_set_topology);
    command_list_vtable[25] = reinterpret_cast<void *>(
        &fake_render_set_pipeline_state);
    command_list_vtable[28] = reinterpret_cast<void *>(
        &fake_render_set_descriptor_heaps);
    command_list_vtable[30] = reinterpret_cast<void *>(
        &fake_render_set_root_signature);
    command_list_vtable[32] = reinterpret_cast<void *>(
        &fake_render_set_root_table);
    command_list_vtable[38] = reinterpret_cast<void *>(
        &fake_render_set_root_cbv);
    command_list_vtable[43] = reinterpret_cast<void *>(
        &fake_render_set_index_buffer);
    command_list_vtable[44] = reinterpret_cast<void *>(
        &fake_render_set_vertex_buffers);

    FakeRenderCommandList command_list = {};
    command_list.vtable = command_list_vtable;
    FakeRenderResource constant_buffer = {};
    constant_buffer.vtable = resource_vtable;
    constant_buffer.gpu_address = 0x123456789ABCULL;
    FakeDescriptorHeap cbv_heap;
    cbv_heap.gpu_handle.ptr = 0x111000;
    FakeDescriptorHeap main_heap;
    main_heap.gpu_handle.ptr = 0x222000;
    unsigned char mapped_constant_buffer[256] = {};

    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework =
        reinterpret_cast<CD3DFramework12 *>(framework_storage);
    framework->m_pCommandList =
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list);
    framework->m_pRootSignature =
        reinterpret_cast<ID3D12RootSignature *>(0x1000);
    framework->m_pPipelineState =
        reinterpret_cast<ID3D12PipelineState *>(0x2000);
    framework->m_pLevelPipelineState =
        reinterpret_cast<ID3D12PipelineState *>(0x3000);
    framework->m_pTransparentPipelineState =
        reinterpret_cast<ID3D12PipelineState *>(0x4000);
    framework->m_pTransparentGlassPipelineState =
        reinterpret_cast<ID3D12PipelineState *>(0x5000);
    framework->m_pMainDescriptorHeap = &main_heap;
    framework->m_cbvHeap = &cbv_heap;
    framework->m_constantBuffer =
        reinterpret_cast<ID3D12Resource *>(&constant_buffer);
    framework->m_pCbvDataBegin = mapped_constant_buffer;
    framework->m_levelVertexBufferView = {
        0x300000, 0x4000, sizeof(Vertex)};
    framework->m_levelIndexBufferView = {
        0x400000, 0x2000, DXGI_FORMAT_R16_UINT};

    std::vector<unsigned char> application_storage(0x310570, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;
    auto *opaque_meshes = new (
        application_storage.data() + 0x3103A8)
        std::vector<CD3DApplication::FBX_MESH *>();
    auto *transparent_meshes = new (
        application_storage.data() + 0x3103C0)
        std::vector<CD3DApplication::FBX_MESH *>();
    auto *glass_meshes = new (
        application_storage.data() + 0x3103D8)
        std::vector<CD3DApplication::FBX_MESH *>();

    CD3DApplication::FBX_MESH mesh = {};
    mesh.name = "mesh0";
    CD3DApplication::SubMeshSet first = {};
    first.subMeshIndices.resize(3);
    first.vertices.resize(2);
    CD3DApplication::SubMeshSet second = {};
    second.subMeshIndices.resize(6);
    second.vertices.resize(1);
    mesh.subMeshes.push_back(first);
    mesh.subMeshes.push_back(second);
    opaque_meshes->push_back(&mesh);
    transparent_meshes->push_back(&mesh);
    glass_meshes->push_back(&mesh);

    auto *world = reinterpret_cast<DirectX::XMMATRIX *>(
        application_storage.data() + 0x3103F0);
    auto *projection = reinterpret_cast<DirectX::XMMATRIX *>(
        application_storage.data() + 0x310430);
    *world = DirectX::XMMatrixIdentity();
    *projection = DirectX::XMMatrixIdentity();
    auto *constant_data = reinterpret_cast<
        CD3DApplication::ConstantBufferData *>(
            application_storage.data() + 0x310470);
    constant_data->levelScale = DirectX::XMVectorSet(
        2.0f, 3.0f, 4.0f, 5.0f);

    const int saved_cull_zero = cullmesh[0];
    const int saved_cull_level = cullmesh[31];
    const FVECTOR saved_scroll = g_levelUVScroll;
    cullmesh[0] = 1;
    cullmesh[31] = 0;
    g_levelUVScroll.vx = 0.25f;
    g_levelUVScroll.vy = -0.5f;
    MATRIX legacy_matrix = {};

    application->DrawLevel(&legacy_matrix, nullptr, nullptr, 17);
    CHECK(command_list.root_signature_calls == 1);
    CHECK(command_list.root_signature == framework->m_pRootSignature);
    CHECK(command_list.descriptor_heap_calls == 2);
    CHECK(command_list.descriptor_heaps[0] == &cbv_heap);
    CHECK(command_list.descriptor_heaps[1] == &main_heap);
    CHECK(command_list.root_table_calls == 1);
    CHECK(command_list.root_table_indices[0] == 1);
    CHECK(command_list.root_table_handles[0].ptr == 0x222000);
    CHECK(command_list.vertex_buffer_calls == 1);
    CHECK(command_list.vertex_buffer_start_slots[0] == 0);
    CHECK(command_list.vertex_buffer_counts[0] == 1);
    CHECK(std::memcmp(
              &command_list.vertex_buffer_views[0],
              &framework->m_levelVertexBufferView,
              sizeof(D3D12_VERTEX_BUFFER_VIEW)) == 0);
    CHECK(command_list.index_buffer_calls == 1);
    CHECK(std::memcmp(
              &command_list.index_buffer_views[0],
              &framework->m_levelIndexBufferView,
              sizeof(D3D12_INDEX_BUFFER_VIEW)) == 0);
    CHECK(command_list.topology_calls == 1);
    CHECK(command_list.topologies[0] ==
          D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CHECK(command_list.root_cbv_index == 0);
    CHECK(command_list.root_cbv_address == 0x123456789ABCULL);
    CHECK(command_list.pipeline_state_calls == 2);
    CHECK(command_list.pipeline_states[0] ==
          framework->m_pLevelPipelineState);
    CHECK(command_list.pipeline_states[1] == framework->m_pPipelineState);
    CHECK(command_list.draw_indexed_calls == 2);
    CHECK(command_list.draw_index_counts[0] == 3);
    CHECK(command_list.draw_start_indices[0] == 0);
    CHECK(command_list.draw_base_vertices[0] == 0);
    CHECK(command_list.draw_index_counts[1] == 6);
    CHECK(command_list.draw_start_indices[1] == 3);
    CHECK(command_list.draw_base_vertices[1] == 2);
    CHECK(*reinterpret_cast<int *>(
              application_storage.data() + 0x3103A0) == 9);
    CHECK(constant_data->uvScrollSpeed.x == 0.25f);
    CHECK(constant_data->uvScrollSpeed.y == -0.5f);
    const auto *uploaded = reinterpret_cast<const
        CD3DApplication::ConstantBufferData *>(mapped_constant_buffer);
    CHECK(uploaded->worldViewProjectionMatrix.r[0].m128_f32[0] == 1.0f);
    CHECK(uploaded->worldViewProjectionMatrix.r[3].m128_f32[3] == 1.0f);
    CHECK(uploaded->levelScale.m128_f32[0] == 2.0f);
    CHECK(std::memcmp(
              uploaded,
              constant_data,
              sizeof(*constant_data)) == 0);

    command_list = {};
    command_list.vtable = command_list_vtable;
    g_levelUVScroll.vx = 0.75f;
    g_levelUVScroll.vy = 0.5f;
    application->DrawLevelTransparent(
        &legacy_matrix, nullptr, nullptr, 23);
    CHECK(command_list.descriptor_heap_calls == 2);
    CHECK(command_list.descriptor_heaps[0] == &cbv_heap);
    CHECK(command_list.descriptor_heaps[1] == &main_heap);
    CHECK(command_list.root_table_calls == 2);
    CHECK(command_list.root_table_handles[0].ptr == 0x111000);
    CHECK(command_list.root_table_handles[1].ptr == 0x222000);
    CHECK(command_list.pipeline_states[0] ==
          framework->m_pTransparentPipelineState);
    CHECK(command_list.pipeline_states[1] == framework->m_pPipelineState);
    CHECK(command_list.draw_indexed_calls == 2);
    CHECK(command_list.draw_start_indices[0] == 9);
    CHECK(command_list.draw_base_vertices[0] == 9);
    CHECK(command_list.draw_start_indices[1] == 12);
    CHECK(command_list.draw_base_vertices[1] == 12);
    CHECK(*reinterpret_cast<int *>(
              application_storage.data() + 0x3103A4) == 9);
    CHECK(constant_data->uvScrollSpeed.x == 0.25f);
    CHECK(constant_data->uvScrollSpeed.y == -0.5f);

    command_list = {};
    command_list.vtable = command_list_vtable;
    application->DrawLevelTransparentGlass(
        &legacy_matrix, nullptr, nullptr, 31);
    CHECK(command_list.root_table_calls == 2);
    CHECK(command_list.root_table_handles[0].ptr == 0x111000);
    CHECK(command_list.root_table_handles[1].ptr == 0x222000);
    CHECK(command_list.pipeline_states[0] ==
          framework->m_pTransparentGlassPipelineState);
    CHECK(command_list.pipeline_states[1] == framework->m_pPipelineState);
    CHECK(command_list.draw_indexed_calls == 2);
    CHECK(command_list.draw_start_indices[0] == 18);
    CHECK(command_list.draw_base_vertices[0] == 18);
    CHECK(command_list.draw_start_indices[1] == 21);
    CHECK(command_list.draw_base_vertices[1] == 21);

    command_list = {};
    command_list.vtable = command_list_vtable;
    application->DrawLevel(nullptr, nullptr, nullptr, 0);
    CHECK(command_list.root_signature_calls == 0);

    g_levelUVScroll = saved_scroll;
    cullmesh[31] = saved_cull_level;
    cullmesh[0] = saved_cull_zero;
    glass_meshes->~vector();
    transparent_meshes->~vector();
    opaque_meshes->~vector();
    return 0;
}

static int test_run_quit_message()
{
    MSG message;
    while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE) != FALSE) {
    }
    PostQuitMessage(123);

    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    CHECK(application->CD3DApplication::Run() == 123);
    return 0;
}

static int test_message_procedure_core_paths()
{
    jpb_d3dapp_set_sdl_get_window_flags_test_hook(
        fake_sdl_get_window_flags);

    void *application_vtable[16] = {};
    application_vtable[6] = reinterpret_cast<void *>(
        &fake_message_key_down);
    application_vtable[7] = reinterpret_cast<void *>(
        &fake_message_key_up);
    application_vtable[9] = reinterpret_cast<void *>(
        &fake_message_query);
    application_vtable[10] = reinterpret_cast<void *>(
        &fake_message_resume);
    application_vtable[14] = reinterpret_cast<void *>(
        &fake_application_pause);
    application_vtable[15] = reinterpret_cast<void *>(
        &fake_message_toggle);

    void *swap_chain_vtable[9] = {};
    swap_chain_vtable[8] = reinterpret_cast<void *>(
        &fake_swap_chain_present);
    FakeSwapChain swap_chain = {};
    swap_chain.vtable = swap_chain_vtable;
    swap_chain.present_result = S_OK;
    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage);
    framework->m_pSwapChain = reinterpret_cast<IDXGISwapChain3 *>(
        &swap_chain);

    std::vector<unsigned char> application_storage(0x310978, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    *reinterpret_cast<void ***>(application) = application_vtable;
    application->m_pFramework = framework;

    MINMAXINFO minimums = {};
    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_GETMINMAXINFO, 0,
        reinterpret_cast<LPARAM>(&minimums));
    CHECK(minimums.ptMinTrackSize.x == 100);
    CHECK(minimums.ptMinTrackSize.y == 100);
    CHECK(application->CD3DApplication::MsgProc(
              nullptr, WM_ERASEBKGND, 0, 0) == 1);

    framework->m_bIsFullscreen = TRUE;
    CHECK(application->CD3DApplication::MsgProc(
              nullptr, WM_NCHITTEST, 0, 0) == 1);
    *reinterpret_cast<int *>(application_storage.data() + 0x20) = 1;
    *reinterpret_cast<int *>(application_storage.data() + 0x24) = 1;
    void *sdl_window = reinterpret_cast<void *>(0x12340000);
    *reinterpret_cast<void **>(application_storage.data() + 0xD8) =
        sdl_window;
    fake_sdl_window_flags_result = 0x1001;
    fake_sdl_window_flags_calls = 0;
    CHECK(application->CD3DApplication::MsgProc(
              nullptr, WM_SETCURSOR, 0, 0) == 1);
    CHECK(application->CD3DApplication::MsgProc(
              nullptr, WM_CHAR, 'A', 0) == 1);
    CHECK(application->CD3DApplication::MsgProc(
              nullptr, WM_SYSCOMMAND, SC_SIZE, 0) == 1);
    CHECK(fake_sdl_window_flags_calls == 3);

    fake_message_toggle_calls = 0;
    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_SYSCOMMAND, 0x323, 0);
    CHECK(fake_message_toggle_calls == 1);

    fake_pause_calls = 0;
    fake_pause_value = -1;
    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_ENTERMENULOOP, 0, 0);
    CHECK(fake_pause_calls == 1);
    CHECK(fake_pause_value == 1);
    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_EXITMENULOOP, 0, 0);
    CHECK(fake_pause_calls == 2);
    CHECK(fake_pause_value == 0);

    fake_message_query_calls = 0;
    CHECK(application->CD3DApplication::MsgProc(
              nullptr,
              WM_POWERBROADCAST,
              PBT_APMQUERYSUSPEND,
              0x55667788) == 23);
    CHECK(fake_message_query_calls == 1);
    CHECK(fake_message_power_value == 0x55667788UL);
    fake_message_resume_calls = 0;
    CHECK(application->CD3DApplication::MsgProc(
              nullptr,
              WM_POWERBROADCAST,
              PBT_APMRESUMESUSPEND,
              0x11223344) == 29);
    CHECK(fake_message_resume_calls == 1);
    CHECK(fake_message_power_value == 0x11223344UL);

    fake_message_key_down_calls = 0;
    fake_message_key_up_calls = 0;
    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_KEYDOWN, VK_F1, 0);
    CHECK(fake_message_key_down_calls == 1);
    CHECK(fake_message_key_down_value == 0xF1);
    CHECK(application_storage[0x310570 + 0xF1] == 1);
    CHECK(application_storage[0x310770 + 0xF1] == 1);
    CHECK(*reinterpret_cast<int *>(
              application_storage.data() + 0x310970) == 0xF1);
    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_KEYUP, VK_F1, 0);
    CHECK(fake_message_key_up_calls == 1);
    CHECK(fake_message_key_up_value == 0xF1);
    CHECK(application_storage[0x310570 + 0xF1] == 0);
    CHECK(application_storage[0x310870 + 0xF1] == 1);

    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_ACTIVATE, 0, 0);
    CHECK(*reinterpret_cast<int *>(
              application_storage.data() + 0x20) == 0);
    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_ACTIVATE, 1, 0);
    CHECK(*reinterpret_cast<int *>(
              application_storage.data() + 0x20) == 1);
    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_SIZE, SIZE_MINIMIZED, 0);
    CHECK(*reinterpret_cast<int *>(
              application_storage.data() + 0x20) == 0);
    *reinterpret_cast<int *>(application_storage.data() + 0x24) = 0;
    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_SIZE, SIZE_RESTORED, 0);
    CHECK(*reinterpret_cast<int *>(
              application_storage.data() + 0x20) == 1);

    framework->m_bIsFullscreen = FALSE;
    swap_chain.present_calls = 0;
    (void)application->CD3DApplication::MsgProc(
        nullptr, WM_PAINT, 0, 0);
    CHECK(swap_chain.present_calls == 1);
    CHECK(swap_chain.sync_interval == 1);
    CHECK(swap_chain.present_flags == 0);

    jpb_d3dapp_set_sdl_get_window_flags_test_hook(nullptr);
    return 0;
}

static int test_application_initializer_failure_gates()
{
    jpb_d3dapp_set_create_dxgi_factory_test_hook(
        &fake_create_dxgi_factory);

    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework =
        reinterpret_cast<CD3DFramework12 *>(framework_storage);
    std::vector<unsigned char> application_storage(0x100, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;

    fake_create_factory_state = {};
    fake_create_factory_state.result = E_ACCESSDENIED;
    CHECK(application->InitD3D12Framework(nullptr) == E_ACCESSDENIED);
    CHECK(fake_create_factory_state.calls == 1);
    CHECK(fake_create_factory_state.flags == 0);
    CHECK(*fake_create_factory_state.iid == __uuidof(IDXGIFactory6));
    CHECK(framework->m_pFactory == nullptr);
    CHECK(framework->m_graphicsMemory == nullptr);

    void *factory_vtable[30] = {};
    factory_vtable[29] = reinterpret_cast<void *>(
        &fake_enum_adapter_by_gpu_preference);
    FakeFactory factory = {};
    factory.vtable = factory_vtable;
    factory.preferred_adapter_result_hr = E_NOINTERFACE;
    fake_create_factory_state = {};
    fake_create_factory_state.factory =
        reinterpret_cast<IDXGIFactory6 *>(&factory);
    fake_create_factory_state.result = S_OK;
    CHECK(application->InitD3D12Framework(nullptr) == E_NOINTERFACE);
    CHECK(fake_create_factory_state.calls == 1);
    CHECK(framework->m_pFactory ==
          reinterpret_cast<IDXGIFactory6 *>(&factory));
    CHECK(factory.preferred_adapter_calls == 1);
    CHECK(factory.preferred_adapter_index == 0);
    CHECK(factory.gpu_preference ==
          DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE);
    CHECK(*factory.preferred_adapter_iid == __uuidof(IDXGIAdapter1));
    CHECK(framework->m_graphicsMemory == nullptr);

    jpb_d3dapp_set_create_dxgi_factory_test_hook(nullptr);
    return 0;
}

static int test_application_initializer_scene_buffer()
{
    void *device_vtable[28] = {};
    device_vtable[17] = reinterpret_cast<void *>(
        &fake_create_constant_buffer_view);
    device_vtable[27] = reinterpret_cast<void *>(
        &fake_create_committed_resource);
    void *resource_vtable[12] = {};
    resource_vtable[8] = reinterpret_cast<void *>(
        &fake_transparency_resource_map);
    resource_vtable[11] = reinterpret_cast<void *>(
        &fake_transparency_resource_gpu_address);

    unsigned char mapped_data[0x100] = {};
    FakeTransparencyResource resource = {};
    resource.vtable = resource_vtable;
    resource.mapped_data = mapped_data;
    resource.map_result = S_OK;
    resource.gpu_address = 0x1234567800ULL;
    FakeDescriptorHeap cbv_heap;
    cbv_heap.cpu_handle.ptr = 0xABC000;

    FakeDevice device = {};
    device.vtable = device_vtable;
    device.committed_resource_result =
        reinterpret_cast<ID3D12Resource *>(&resource);
    device.committed_resource_result_hr = S_OK;

    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework =
        reinterpret_cast<CD3DFramework12 *>(framework_storage);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    framework->m_cbvHeap = &cbv_heap;
    framework->m_dwRenderWidth = 1920;
    framework->m_dwRenderHeight = 1080;
    framework->m_vertexUploadBuffer =
        reinterpret_cast<ID3D12Resource *>(0x1);
    framework->m_indexUploadBuffer =
        reinterpret_cast<ID3D12Resource *>(0x2);

    std::vector<unsigned char> application_storage(0x310570, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;
    for (unsigned int index = 0; index < 0x100; ++index) {
        application_storage[0x310470 + index] =
            static_cast<unsigned char>(index ^ 0xA5);
    }

    jpb_d3dapp_initialize_scene_buffer_for_test(application);

    CHECK(device.committed_resource_calls == 1);
    CHECK(device.committed_heap_properties.Type ==
          D3D12_HEAP_TYPE_UPLOAD);
    CHECK(device.committed_heap_properties.CPUPageProperty ==
          D3D12_CPU_PAGE_PROPERTY_UNKNOWN);
    CHECK(device.committed_heap_properties.MemoryPoolPreference ==
          D3D12_MEMORY_POOL_UNKNOWN);
    CHECK(device.committed_heap_properties.CreationNodeMask == 1);
    CHECK(device.committed_heap_properties.VisibleNodeMask == 1);
    CHECK(device.committed_heap_flags == D3D12_HEAP_FLAG_NONE);
    CHECK(device.committed_resource_description.Dimension ==
          D3D12_RESOURCE_DIMENSION_BUFFER);
    CHECK(device.committed_resource_description.Width == 0x10000);
    CHECK(device.committed_resource_description.Height == 1);
    CHECK(device.committed_resource_description.DepthOrArraySize == 1);
    CHECK(device.committed_resource_description.MipLevels == 1);
    CHECK(device.committed_resource_description.Format ==
          DXGI_FORMAT_UNKNOWN);
    CHECK(device.committed_resource_description.SampleDesc.Count == 1);
    CHECK(device.committed_resource_description.SampleDesc.Quality == 0);
    CHECK(device.committed_resource_description.Layout ==
          D3D12_TEXTURE_LAYOUT_ROW_MAJOR);
    CHECK(device.committed_resource_description.Flags ==
          D3D12_RESOURCE_FLAG_NONE);
    CHECK(device.committed_initial_state ==
          D3D12_RESOURCE_STATE_GENERIC_READ);
    CHECK(!device.committed_clear_value_present_by_call[0]);
    CHECK(*device.committed_resource_iid == __uuidof(ID3D12Resource));

    CHECK(resource.gpu_address_calls == 1);
    CHECK(device.constant_buffer_view_calls == 1);
    CHECK(device.constant_buffer_view_description.BufferLocation ==
          0x1234567800ULL);
    CHECK(device.constant_buffer_view_description.SizeInBytes == 0x100);
    CHECK(device.constant_buffer_view_handle.ptr == 0xABC000);
    CHECK(resource.map_calls == 1);
    CHECK(resource.map_subresource == 0);
    CHECK(resource.map_read_range_value.Begin == 0);
    CHECK(resource.map_read_range_value.End == 0);
    CHECK(framework->m_pCbvDataBegin == mapped_data);
    CHECK(std::memcmp(
              mapped_data,
              application_storage.data() + 0x310470,
              sizeof(mapped_data)) == 0);
    CHECK(framework->m_vertexUploadBuffer == nullptr);
    CHECK(framework->m_indexUploadBuffer == nullptr);

    const auto *world = reinterpret_cast<const DirectX::XMFLOAT4X4 *>(
        application_storage.data() + 0x3103F0);
    CHECK(world->_11 == 1.0f && world->_22 == 1.0f);
    CHECK(world->_33 == 1.0f && world->_44 == 1.0f);
    CHECK(world->_12 == 0.0f && world->_43 == 0.0f);
    const auto *projection =
        reinterpret_cast<const DirectX::XMFLOAT4X4 *>(
            application_storage.data() + 0x310430);
    CHECK(near_float(projection->_11, 1.1282004f));
    CHECK(near_float(projection->_22, 2.0056896f));
    CHECK(near_float(projection->_33, 1.0001f));
    CHECK(projection->_34 == 1.0f);
    CHECK(near_float(projection->_43, -1.0001f));
    CHECK(projection->_44 == 0.0f);
    return 0;
}

static int test_depth_stencil_initialization()
{
    void *device_vtable[28] = {};
    device_vtable[14] =
        reinterpret_cast<void *>(&fake_create_descriptor_heap);
    device_vtable[21] =
        reinterpret_cast<void *>(&fake_create_depth_stencil_view);
    device_vtable[27] =
        reinterpret_cast<void *>(&fake_create_committed_resource);

    FakeDescriptorHeap descriptor_heap;
    descriptor_heap.cpu_handle.ptr = 0x12345000;
    FakeDevice device = {};
    device.vtable = device_vtable;
    device.descriptor_heap_result = &descriptor_heap;
    device.descriptor_heap_result_hr = S_OK;
    device.committed_resource_result = reinterpret_cast<ID3D12Resource *>(
        static_cast<std::uintptr_t>(0xABCDEF00));
    device.committed_resource_result_hr = S_OK;

    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    std::vector<unsigned char> application_storage(0x383A0, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;
    *reinterpret_cast<UINT *>(application_storage.data() + 0x3831C) = 1920;
    *reinterpret_cast<UINT *>(application_storage.data() + 0x38320) = 1080;

    CHECK(application->InitializeDepthStencilBuffer() == S_OK);
    CHECK(device.descriptor_heap_calls == 1);
    CHECK(device.descriptor_heap_description.Type ==
          D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CHECK(device.descriptor_heap_description.NumDescriptors == 1);
    CHECK(device.descriptor_heap_description.Flags ==
          D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    CHECK(device.descriptor_heap_description.NodeMask == 0);
    CHECK(framework->m_pDsDescriptorHeap == &descriptor_heap);
    CHECK(device.committed_resource_calls == 1);
    CHECK(device.committed_heap_properties.Type == D3D12_HEAP_TYPE_DEFAULT);
    CHECK(device.committed_heap_properties.CPUPageProperty ==
          D3D12_CPU_PAGE_PROPERTY_UNKNOWN);
    CHECK(device.committed_heap_properties.MemoryPoolPreference ==
          D3D12_MEMORY_POOL_UNKNOWN);
    CHECK(device.committed_heap_properties.CreationNodeMask == 1);
    CHECK(device.committed_heap_properties.VisibleNodeMask == 1);
    CHECK(device.committed_heap_flags == D3D12_HEAP_FLAG_NONE);
    CHECK(device.committed_resource_description.Dimension ==
          D3D12_RESOURCE_DIMENSION_TEXTURE2D);
    CHECK(device.committed_resource_description.Alignment == 0);
    CHECK(device.committed_resource_description.Width == 1920);
    CHECK(device.committed_resource_description.Height == 1080);
    CHECK(device.committed_resource_description.DepthOrArraySize == 1);
    CHECK(device.committed_resource_description.MipLevels == 0);
    CHECK(device.committed_resource_description.Format ==
          DXGI_FORMAT_D32_FLOAT);
    CHECK(device.committed_resource_description.SampleDesc.Count == 1);
    CHECK(device.committed_resource_description.SampleDesc.Quality == 0);
    CHECK(device.committed_resource_description.Layout ==
          D3D12_TEXTURE_LAYOUT_UNKNOWN);
    CHECK(device.committed_resource_description.Flags ==
          D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    CHECK(device.committed_initial_state ==
          D3D12_RESOURCE_STATE_DEPTH_WRITE);
    CHECK(device.committed_clear_value.Format == DXGI_FORMAT_D32_FLOAT);
    CHECK(device.committed_clear_value.DepthStencil.Depth == 1.0f);
    CHECK(device.committed_clear_value.DepthStencil.Stencil == 0);
    CHECK(*device.committed_resource_iid == __uuidof(ID3D12Resource));
    CHECK(framework->m_pDepthStencil == device.committed_resource_result);
    CHECK(device.depth_stencil_view_calls == 1);
    CHECK(device.depth_stencil_view_resource ==
          device.committed_resource_result);
    CHECK(device.depth_stencil_view_description.Format ==
          DXGI_FORMAT_D32_FLOAT);
    CHECK(device.depth_stencil_view_description.ViewDimension ==
          D3D12_DSV_DIMENSION_TEXTURE2D);
    CHECK(device.depth_stencil_view_description.Flags ==
          D3D12_DSV_FLAG_NONE);
    CHECK(device.depth_stencil_view_description.Texture2D.MipSlice == 0);
    CHECK(device.depth_stencil_view_handle.ptr == 0x12345000);

    device = {};
    device.vtable = device_vtable;
    device.descriptor_heap_result_hr = E_ACCESSDENIED;
    CHECK(application->InitializeDepthStencilBuffer() == E_ACCESSDENIED);
    CHECK(device.descriptor_heap_calls == 1);
    CHECK(device.committed_resource_calls == 0);
    CHECK(device.depth_stencil_view_calls == 0);

    descriptor_heap = {};
    descriptor_heap.cpu_handle.ptr = 0x56789000;
    device = {};
    device.vtable = device_vtable;
    device.descriptor_heap_result = &descriptor_heap;
    device.descriptor_heap_result_hr = S_OK;
    device.committed_resource_result_hr = DXGI_ERROR_DEVICE_REMOVED;
    framework->m_pDepthStencil = nullptr;
    CHECK(application->InitializeDepthStencilBuffer() ==
          DXGI_ERROR_DEVICE_REMOVED);
    CHECK(device.committed_resource_calls == 1);
    CHECK(device.depth_stencil_view_calls == 1);
    CHECK(device.depth_stencil_view_resource == nullptr);
    CHECK(device.depth_stencil_view_handle.ptr == 0x56789000);
    return 0;
}

static int test_fence_initialization()
{
    void *device_vtable[37] = {};
    device_vtable[36] = reinterpret_cast<void *>(&fake_create_fence);

    FakeDevice device = {};
    device.vtable = device_vtable;
    device.fence_results[0] = reinterpret_cast<ID3D12Fence *>(
        static_cast<std::uintptr_t>(0x77770000));
    device.fence_results[1] = reinterpret_cast<ID3D12Fence *>(
        static_cast<std::uintptr_t>(0x88880000));
    device.fence_results_hr[0] = S_OK;
    device.fence_results_hr[1] = S_OK;

    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    framework->m_nFenceValues[0] = 0x1111111111111111ULL;
    framework->m_nFenceValues[1] = 0x2222222222222222ULL;

    std::vector<unsigned char> application_storage(0x100, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;

    CHECK(application->CreateFence() == S_OK);
    CHECK(device.fence_calls == 2);
    CHECK(device.fence_initial_values[0] == 0);
    CHECK(device.fence_initial_values[1] == 0);
    CHECK(device.fence_flags[0] == D3D12_FENCE_FLAG_NONE);
    CHECK(device.fence_flags[1] == D3D12_FENCE_FLAG_NONE);
    CHECK(*device.fence_iids[0] == __uuidof(ID3D12Fence));
    CHECK(*device.fence_iids[1] == __uuidof(ID3D12Fence));
    CHECK(framework->m_pFence[0] == device.fence_results[0]);
    CHECK(framework->m_pFence[1] == device.fence_results[1]);
    CHECK(framework->m_nFenceValues[0] == 0);
    CHECK(framework->m_nFenceValues[1] == 0);
    CHECK(framework->m_hFenceEvent != nullptr);
    CHECK(CloseHandle(framework->m_hFenceEvent) != FALSE);

    device = {};
    device.vtable = device_vtable;
    device.fence_results_hr[0] = E_ACCESSDENIED;
    framework->m_nFenceValues[0] = 17;
    framework->m_nFenceValues[1] = 29;
    framework->m_hFenceEvent = nullptr;
    CHECK(application->CreateFence() == E_ACCESSDENIED);
    CHECK(device.fence_calls == 1);
    CHECK(framework->m_nFenceValues[0] == 17);
    CHECK(framework->m_nFenceValues[1] == 29);
    CHECK(framework->m_hFenceEvent == nullptr);

    device = {};
    device.vtable = device_vtable;
    device.fence_results[0] = reinterpret_cast<ID3D12Fence *>(
        static_cast<std::uintptr_t>(0x99990000));
    device.fence_results_hr[0] = S_OK;
    device.fence_results_hr[1] = DXGI_ERROR_DEVICE_REMOVED;
    framework->m_nFenceValues[0] = 31;
    framework->m_nFenceValues[1] = 37;
    CHECK(application->CreateFence() == DXGI_ERROR_DEVICE_REMOVED);
    CHECK(device.fence_calls == 2);
    CHECK(framework->m_nFenceValues[0] == 0);
    CHECK(framework->m_nFenceValues[1] == 37);
    CHECK(framework->m_hFenceEvent == nullptr);
    return 0;
}

static int test_srv_heap_initialization()
{
    void *device_vtable[15] = {};
    device_vtable[14] =
        reinterpret_cast<void *>(&fake_create_descriptor_heap);

    FakeDescriptorHeap srv_heap;
    FakeDescriptorHeap cbv_heap;
    FakeDevice device = {};
    device.vtable = device_vtable;
    device.descriptor_heap_result = &srv_heap;
    device.descriptor_heap_result_hr = S_OK;
    device.descriptor_heap_result2 = &cbv_heap;
    device.descriptor_heap_result_hr2 = S_OK;
    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    framework->m_nDescriptorCount = 73;
    std::vector<unsigned char> application_storage(0x100, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;

    CHECK(application->CreateSRVHeap() == S_OK);
    CHECK(device.descriptor_heap_calls == 2);
    CHECK(device.descriptor_heap_description.Type ==
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CHECK(device.descriptor_heap_description.NumDescriptors == 0x400);
    CHECK(device.descriptor_heap_description.Flags ==
          D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    CHECK(device.descriptor_heap_description.NodeMask == 0);
    CHECK(device.descriptor_heap_description2.Type ==
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CHECK(device.descriptor_heap_description2.NumDescriptors == 1);
    CHECK(device.descriptor_heap_description2.Flags ==
          D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    CHECK(device.descriptor_heap_description2.NodeMask == 0);
    CHECK(*device.descriptor_heap_iid == __uuidof(ID3D12DescriptorHeap));
    CHECK(*device.descriptor_heap_iid2 == __uuidof(ID3D12DescriptorHeap));
    CHECK(framework->m_pMainDescriptorHeap == &srv_heap);
    CHECK(framework->m_cbvHeap == &cbv_heap);
    CHECK(framework->m_nDescriptorCount == 0);
    CHECK(srv_heap.set_name_calls == 1);
    CHECK(std::wcscmp(srv_heap.last_name, L"SRV Heap") == 0);
    CHECK(cbv_heap.set_name_calls == 0);

    srv_heap = {};
    device = {};
    device.vtable = device_vtable;
    device.descriptor_heap_result = &srv_heap;
    device.descriptor_heap_result_hr = E_ACCESSDENIED;
    framework->m_nDescriptorCount = 41;
    CHECK(application->CreateSRVHeap() == E_ACCESSDENIED);
    CHECK(device.descriptor_heap_calls == 1);
    CHECK(srv_heap.set_name_calls == 1);
    CHECK(framework->m_nDescriptorCount == 0);

    srv_heap = {};
    cbv_heap = {};
    device = {};
    device.vtable = device_vtable;
    device.descriptor_heap_result = &srv_heap;
    device.descriptor_heap_result_hr = S_OK;
    device.descriptor_heap_result2 = &cbv_heap;
    device.descriptor_heap_result_hr2 = E_OUTOFMEMORY;
    CHECK(application->CreateSRVHeap() == E_OUTOFMEMORY);
    CHECK(device.descriptor_heap_calls == 2);
    CHECK(srv_heap.set_name_calls == 1);
    CHECK(cbv_heap.set_name_calls == 0);
    return 0;
}

static int test_render_target_initialization()
{
    void *device_vtable[21] = {};
    device_vtable[14] =
        reinterpret_cast<void *>(&fake_create_descriptor_heap);
    device_vtable[15] =
        reinterpret_cast<void *>(&fake_get_descriptor_increment_size);
    device_vtable[20] =
        reinterpret_cast<void *>(&fake_create_render_target_view);
    void *swap_chain_vtable[10] = {};
    swap_chain_vtable[9] = reinterpret_cast<void *>(&fake_get_buffer);

    FakeDescriptorHeap heap;
    heap.cpu_handle.ptr = 0xABC000;
    FakeDevice device = {};
    device.vtable = device_vtable;
    device.descriptor_heap_result = &heap;
    device.descriptor_heap_result_hr = S_OK;
    device.increment_size_result = 32;
    FakeSwapChain swap_chain = {};
    swap_chain.vtable = swap_chain_vtable;
    swap_chain.results[0] = reinterpret_cast<ID3D12Resource *>(
        static_cast<std::uintptr_t>(0xAAAA0000));
    swap_chain.results[1] = reinterpret_cast<ID3D12Resource *>(
        static_cast<std::uintptr_t>(0xBBBB0000));
    swap_chain.results_hr[0] = S_OK;
    swap_chain.results_hr[1] = S_OK;

    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    framework->m_pSwapChain = reinterpret_cast<IDXGISwapChain3 *>(
        &swap_chain);
    std::vector<unsigned char> application_storage(0x100, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;

    CHECK(application->InitializeRenderTargets() == S_OK);
    CHECK(device.descriptor_heap_calls == 1);
    CHECK(device.descriptor_heap_description.Type ==
          D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CHECK(device.descriptor_heap_description.NumDescriptors == 2);
    CHECK(device.descriptor_heap_description.Flags ==
          D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    CHECK(device.descriptor_heap_description.NodeMask == 0);
    CHECK(*device.descriptor_heap_iid == __uuidof(ID3D12DescriptorHeap));
    CHECK(framework->m_pRtvDescriptorHeap == &heap);
    CHECK(heap.set_name_calls == 1);
    CHECK(std::wcscmp(heap.last_name, L"RTV Descriptor Heap") == 0);
    CHECK(heap.cpu_handle_calls == 1);
    CHECK(device.increment_size_calls == 1);
    CHECK(device.increment_size_type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CHECK(framework->m_RTVDescriptorSize == 32);
    CHECK(swap_chain.get_buffer_calls == 2);
    CHECK(swap_chain.indices[0] == 0);
    CHECK(swap_chain.indices[1] == 1);
    CHECK(*swap_chain.iids[0] == __uuidof(ID3D12Resource));
    CHECK(*swap_chain.iids[1] == __uuidof(ID3D12Resource));
    CHECK(framework->m_pRenderTargets[0] == swap_chain.results[0]);
    CHECK(framework->m_pRenderTargets[1] == swap_chain.results[1]);
    CHECK(device.render_target_view_calls == 2);
    CHECK(device.render_target_view_resources[0] == swap_chain.results[0]);
    CHECK(device.render_target_view_resources[1] == swap_chain.results[1]);
    CHECK(device.render_target_view_descriptions[0] == nullptr);
    CHECK(device.render_target_view_descriptions[1] == nullptr);
    CHECK(device.render_target_view_handles[0].ptr == 0xABC000);
    CHECK(device.render_target_view_handles[1].ptr == 0xABC020);

    heap = {};
    device = {};
    device.vtable = device_vtable;
    device.descriptor_heap_result = &heap;
    device.descriptor_heap_result_hr = E_ACCESSDENIED;
    CHECK(application->InitializeRenderTargets() == E_ACCESSDENIED);
    CHECK(heap.set_name_calls == 1);
    CHECK(heap.cpu_handle_calls == 0);
    CHECK(device.increment_size_calls == 0);
    CHECK(swap_chain.get_buffer_calls == 2);

    heap = {};
    heap.cpu_handle.ptr = 0xDEF000;
    device = {};
    device.vtable = device_vtable;
    device.descriptor_heap_result = &heap;
    device.descriptor_heap_result_hr = S_OK;
    device.increment_size_result = 64;
    swap_chain = {};
    swap_chain.vtable = swap_chain_vtable;
    swap_chain.results_hr[0] = DXGI_ERROR_DEVICE_RESET;
    CHECK(application->InitializeRenderTargets() == DXGI_ERROR_DEVICE_RESET);
    CHECK(swap_chain.get_buffer_calls == 1);
    CHECK(device.render_target_view_calls == 0);

    device.descriptor_heap_calls = 0;
    device.render_target_view_calls = 0;
    swap_chain = {};
    swap_chain.vtable = swap_chain_vtable;
    swap_chain.results[0] = reinterpret_cast<ID3D12Resource *>(
        static_cast<std::uintptr_t>(0xCCCC0000));
    swap_chain.results_hr[0] = S_OK;
    swap_chain.results_hr[1] = E_OUTOFMEMORY;
    CHECK(application->InitializeRenderTargets() == E_OUTOFMEMORY);
    CHECK(swap_chain.get_buffer_calls == 2);
    CHECK(device.render_target_view_calls == 1);
    return 0;
}

static int test_device_initialization()
{
    void *device_vtable[14] = {};
    device_vtable[13] = reinterpret_cast<void *>(
        &fake_check_feature_support);
    void *adapter_vtable[11] = {};
    adapter_vtable[2] = reinterpret_cast<void *>(
        &fake_adapter_release);
    adapter_vtable[10] = reinterpret_cast<void *>(
        &fake_adapter_get_desc1);
    void *factory_vtable[30] = {};
    factory_vtable[12] = reinterpret_cast<void *>(&fake_enum_adapters1);
    factory_vtable[29] = reinterpret_cast<void *>(
        &fake_enum_adapter_by_gpu_preference);

    FakeDevice device = {};
    device.vtable = device_vtable;
    device.feature_support_result = DXGI_ERROR_UNSUPPORTED;
    FakeAdapter preferred_adapter = {};
    preferred_adapter.vtable = adapter_vtable;
    preferred_adapter.description.VendorId = 0x1002;
    preferred_adapter.get_desc1_result = E_FAIL;
    FakeFactory factory = {};
    factory.vtable = factory_vtable;
    factory.preferred_adapter_result =
        reinterpret_cast<IDXGIAdapter1 *>(&preferred_adapter);
    factory.preferred_adapter_result_hr = S_OK;

    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0xCD);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pFactory = reinterpret_cast<IDXGIFactory6 *>(&factory);
    std::vector<unsigned char> application_storage(0x383A0, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;
    HWND window = reinterpret_cast<HWND>(
        static_cast<std::uintptr_t>(0x12345678));
    *reinterpret_cast<HWND *>(application_storage.data() + 0xD0) = window;
    *reinterpret_cast<UINT *>(application_storage.data() + 0x3831C) = 1920;
    *reinterpret_cast<UINT *>(application_storage.data() + 0x38320) = 1080;

    fake_create_device_state = {};
    fake_create_device_state.results[0] =
        reinterpret_cast<ID3D12Device *>(&device);
    fake_create_device_state.result_codes[0] = E_OUTOFMEMORY;
    jpb_d3dapp_set_create_device_test_hook(&fake_d3d12_create_device);

    RECT expected_work_area = {};
    CHECK(SystemParametersInfoA(
              SPI_GETWORKAREA, 0, &expected_work_area, 0) != FALSE);
    CHECK(application->InitializeDevice() == DXGI_ERROR_UNSUPPORTED);
    CHECK(factory.preferred_adapter_calls == 1);
    CHECK(factory.preferred_adapter_index == 0);
    CHECK(factory.gpu_preference ==
          DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE);
    CHECK(*factory.preferred_adapter_iid == __uuidof(IDXGIAdapter1));
    CHECK(preferred_adapter.get_desc1_calls == 1);
    CHECK(preferred_adapter.release_calls == 1);
    CHECK(framework->m_isAMD == TRUE);
    CHECK(fake_create_device_state.calls == 1);
    CHECK(fake_create_device_state.adapters[0] ==
          reinterpret_cast<IUnknown *>(&preferred_adapter));
    CHECK(fake_create_device_state.feature_levels[0] ==
          D3D_FEATURE_LEVEL_11_0);
    CHECK(*fake_create_device_state.iids[0] == __uuidof(ID3D12Device));
    CHECK(framework->m_pDevice ==
          reinterpret_cast<ID3D12Device *>(&device));
    CHECK(framework->m_hWnd == window);
    CHECK(framework->m_dwRenderWidth == 1920);
    CHECK(framework->m_dwRenderHeight == 1080);
    CHECK(framework->m_bIsFullscreen == FALSE);
    CHECK(framework->m_nFrameIndex == 0);
    CHECK(framework->m_nCurrentFrameIndex == 0xCDCDCDCDu);
    CHECK(std::memcmp(
              &framework->m_rcScreenRect,
              &expected_work_area,
              sizeof(expected_work_area)) == 0);
    CHECK(device.feature_support_calls == 1);
    CHECK(device.feature == D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS);
    CHECK(device.feature_data.Format == DXGI_FORMAT_R8G8B8A8_UNORM);
    CHECK(device.feature_data.SampleCount == 4);
    CHECK(device.feature_data.Flags ==
          D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE);
    CHECK(device.feature_data.NumQualityLevels == 0);
    CHECK(device.feature_data_size == sizeof(device.feature_data));

    factory = {};
    factory.vtable = factory_vtable;
    factory.preferred_adapter_result_hr = E_ACCESSDENIED;
    framework->m_pFactory = reinterpret_cast<IDXGIFactory6 *>(&factory);
    fake_create_device_state = {};
    CHECK(application->InitializeDevice() == E_ACCESSDENIED);
    CHECK(factory.preferred_adapter_calls == 1);
    CHECK(fake_create_device_state.calls == 0);

    FakeAdapter software_adapter = {};
    software_adapter.vtable = adapter_vtable;
    software_adapter.description.Flags = DXGI_ADAPTER_FLAG_SOFTWARE;
    FakeAdapter rejected_adapter = {};
    rejected_adapter.vtable = adapter_vtable;
    FakeAdapter selected_adapter = {};
    selected_adapter.vtable = adapter_vtable;
    preferred_adapter = {};
    preferred_adapter.vtable = adapter_vtable;
    preferred_adapter.description.VendorId = 0x10DE;
    factory = {};
    factory.vtable = factory_vtable;
    factory.preferred_adapter_result =
        reinterpret_cast<IDXGIAdapter1 *>(&preferred_adapter);
    factory.preferred_adapter_result_hr = S_OK;
    factory.enum_adapter_results[0] =
        reinterpret_cast<IDXGIAdapter1 *>(&software_adapter);
    factory.enum_adapter_results[1] =
        reinterpret_cast<IDXGIAdapter1 *>(&rejected_adapter);
    factory.enum_adapter_results[2] =
        reinterpret_cast<IDXGIAdapter1 *>(&selected_adapter);
    factory.enum_adapter_results_hr[0] = S_OK;
    factory.enum_adapter_results_hr[1] = S_OK;
    factory.enum_adapter_results_hr[2] = S_OK;
    device = {};
    device.vtable = device_vtable;
    device.feature_support_result = S_FALSE;
    std::memset(framework_storage.data(), 0, framework_storage.size());
    framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pFactory = reinterpret_cast<IDXGIFactory6 *>(&factory);
    application->m_pFramework = framework;
    *reinterpret_cast<BOOL *>(application_storage.data() + 0x28) = TRUE;
    fake_create_device_state = {};
    fake_create_device_state.result_codes[0] = E_FAIL;
    fake_create_device_state.results[1] =
        reinterpret_cast<ID3D12Device *>(&device);
    fake_create_device_state.result_codes[1] = S_OK;

    CHECK(application->InitializeDevice() == S_FALSE);
    CHECK(framework->m_isAMD == FALSE);
    CHECK(preferred_adapter.get_desc1_calls == 1);
    CHECK(preferred_adapter.release_calls == 1);
    CHECK(factory.enum_adapters_calls == 3);
    CHECK(factory.enum_adapter_indices[0] == 0);
    CHECK(factory.enum_adapter_indices[1] == 1);
    CHECK(factory.enum_adapter_indices[2] == 2);
    CHECK(software_adapter.get_desc1_calls == 1);
    CHECK(rejected_adapter.get_desc1_calls == 1);
    CHECK(selected_adapter.get_desc1_calls == 1);
    CHECK(software_adapter.release_calls == 0);
    CHECK(rejected_adapter.release_calls == 0);
    CHECK(selected_adapter.release_calls == 0);
    CHECK(fake_create_device_state.calls == 2);
    CHECK(fake_create_device_state.adapters[0] ==
          reinterpret_cast<IUnknown *>(&rejected_adapter));
    CHECK(fake_create_device_state.adapters[1] ==
          reinterpret_cast<IUnknown *>(&selected_adapter));
    CHECK(framework->m_pDevice ==
          reinterpret_cast<ID3D12Device *>(&device));
    CHECK(device.feature_support_calls == 1);

    factory = {};
    factory.vtable = factory_vtable;
    factory.preferred_adapter_result =
        reinterpret_cast<IDXGIAdapter1 *>(&preferred_adapter);
    factory.preferred_adapter_result_hr = S_OK;
    factory.enum_adapter_results_hr[0] = DXGI_ERROR_NOT_FOUND;
    device = {};
    device.vtable = device_vtable;
    device.feature_support_result = E_NOTIMPL;
    std::memset(framework_storage.data(), 0, framework_storage.size());
    framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pFactory = reinterpret_cast<IDXGIFactory6 *>(&factory);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    application->m_pFramework = framework;
    fake_create_device_state = {};

    CHECK(application->InitializeDevice() == E_NOTIMPL);
    CHECK(factory.enum_adapters_calls == 1);
    CHECK(fake_create_device_state.calls == 0);
    CHECK(device.feature_support_calls == 1);

    jpb_d3dapp_set_create_device_test_hook(nullptr);
    return 0;
}

static int test_root_signature_initialization()
{
    void *device_vtable[17] = {};
    device_vtable[16] = reinterpret_cast<void *>(
        &fake_create_root_signature);
    void *blob_vtable[5] = {};
    blob_vtable[3] = reinterpret_cast<void *>(
        &fake_blob_get_buffer_pointer);
    blob_vtable[4] = reinterpret_cast<void *>(
        &fake_blob_get_buffer_size);
    void *root_signature_vtable[7] = {};
    root_signature_vtable[6] = reinterpret_cast<void *>(
        &fake_root_signature_set_name);

    unsigned char serialized_bytes[19] = {};
    FakeBlob blob = {};
    blob.vtable = blob_vtable;
    blob.data = serialized_bytes;
    blob.size = sizeof(serialized_bytes);
    FakeRootSignature root_signature = {};
    root_signature.vtable = root_signature_vtable;
    FakeDevice device = {};
    device.vtable = device_vtable;
    device.root_signature_result =
        reinterpret_cast<ID3D12RootSignature *>(&root_signature);
    device.root_signature_result_hr = S_OK;
    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    std::vector<unsigned char> application_storage(0x100, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;

    fake_serialize_root_signature_state = {};
    fake_serialize_root_signature_state.signature_result =
        reinterpret_cast<ID3DBlob *>(&blob);
    fake_serialize_root_signature_state.error_result =
        reinterpret_cast<ID3DBlob *>(
            static_cast<std::uintptr_t>(0x12340000));
    fake_serialize_root_signature_state.result = S_OK;
    jpb_d3dapp_set_serialize_root_signature_test_hook(
        &fake_serialize_root_signature);

    CHECK(application->InitializeRootSignature() == S_OK);
    CHECK(fake_serialize_root_signature_state.calls == 1);
    CHECK(fake_serialize_root_signature_state.version ==
          D3D_ROOT_SIGNATURE_VERSION_1);
    CHECK(fake_serialize_root_signature_state.description.NumParameters == 3);
    CHECK(fake_serialize_root_signature_state.description.NumStaticSamplers ==
          1);
    CHECK(fake_serialize_root_signature_state.description.Flags ==
          static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(0x1D));
    const auto *parameters =
        fake_serialize_root_signature_state.parameters;
    CHECK(parameters[0].ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV);
    CHECK(parameters[0].Descriptor.ShaderRegister == 0);
    CHECK(parameters[0].Descriptor.RegisterSpace == 0);
    CHECK(parameters[0].ShaderVisibility == D3D12_SHADER_VISIBILITY_VERTEX);
    CHECK(parameters[1].ParameterType ==
          D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
    CHECK(parameters[1].DescriptorTable.NumDescriptorRanges == 1);
    CHECK(parameters[1].ShaderVisibility == D3D12_SHADER_VISIBILITY_PIXEL);
    CHECK(parameters[2].ParameterType ==
          D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);
    CHECK(parameters[2].Constants.ShaderRegister == 2);
    CHECK(parameters[2].Constants.RegisterSpace == 0);
    CHECK(parameters[2].Constants.Num32BitValues == 1);
    CHECK(parameters[2].ShaderVisibility == D3D12_SHADER_VISIBILITY_VERTEX);
    const auto &range = fake_serialize_root_signature_state.range;
    CHECK(range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
    CHECK(range.NumDescriptors == 0x400);
    CHECK(range.BaseShaderRegister == 0);
    CHECK(range.RegisterSpace == 0);
    CHECK(range.OffsetInDescriptorsFromTableStart ==
          D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
    const auto &sampler = fake_serialize_root_signature_state.sampler;
    CHECK(sampler.Filter == D3D12_FILTER_ANISOTROPIC);
    CHECK(sampler.AddressU == D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    CHECK(sampler.AddressV == D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    CHECK(sampler.AddressW == D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    CHECK(sampler.MipLODBias == 0.0f);
    CHECK(sampler.MaxAnisotropy == 8);
    CHECK(sampler.ComparisonFunc == D3D12_COMPARISON_FUNC_NEVER);
    CHECK(sampler.BorderColor ==
          D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK);
    CHECK(sampler.MinLOD == 0.0f);
    CHECK(sampler.MaxLOD == FLT_MAX);
    CHECK(sampler.ShaderRegister == 0);
    CHECK(sampler.RegisterSpace == 0);
    CHECK(sampler.ShaderVisibility == D3D12_SHADER_VISIBILITY_PIXEL);
    CHECK(blob.size_calls == 1);
    CHECK(blob.pointer_calls == 1);
    CHECK(blob.call_count == 2);
    CHECK(blob.call_order[0] == 2);
    CHECK(blob.call_order[1] == 1);
    CHECK(device.root_signature_calls == 1);
    CHECK(device.root_signature_node_mask == 0);
    CHECK(device.root_signature_blob == serialized_bytes);
    CHECK(device.root_signature_blob_size == sizeof(serialized_bytes));
    CHECK(*device.root_signature_iid == __uuidof(ID3D12RootSignature));
    CHECK(framework->m_pRootSignature ==
          reinterpret_cast<ID3D12RootSignature *>(&root_signature));
    CHECK(root_signature.set_name_calls == 1);
    CHECK(std::wcscmp(root_signature.last_name, L"Root Signature") == 0);

    fake_serialize_root_signature_state = {};
    fake_serialize_root_signature_state.result = E_INVALIDARG;
    device.root_signature_calls = 0;
    root_signature.set_name_calls = 0;
    CHECK(application->InitializeRootSignature() == E_INVALIDARG);
    CHECK(fake_serialize_root_signature_state.calls == 1);
    CHECK(device.root_signature_calls == 0);
    CHECK(root_signature.set_name_calls == 0);

    blob = {};
    blob.vtable = blob_vtable;
    blob.data = serialized_bytes;
    blob.size = sizeof(serialized_bytes);
    root_signature = {};
    root_signature.vtable = root_signature_vtable;
    device = {};
    device.vtable = device_vtable;
    device.root_signature_result_hr = E_ACCESSDENIED;
    framework->m_pRootSignature =
        reinterpret_cast<ID3D12RootSignature *>(&root_signature);
    fake_serialize_root_signature_state = {};
    fake_serialize_root_signature_state.signature_result =
        reinterpret_cast<ID3DBlob *>(&blob);
    fake_serialize_root_signature_state.result = S_OK;

    CHECK(application->InitializeRootSignature() == E_ACCESSDENIED);
    CHECK(device.root_signature_calls == 1);
    CHECK(root_signature.set_name_calls == 1);

    jpb_d3dapp_set_serialize_root_signature_test_hook(nullptr);
    return 0;
}

static int test_swap_chain_initialization()
{
    void *device_vtable[9] = {};
    device_vtable[8] = reinterpret_cast<void *>(
        &fake_create_command_queue);
    void *queue_vtable[7] = {};
    queue_vtable[6] = reinterpret_cast<void *>(&fake_queue_set_name);
    void *factory_vtable[16] = {};
    factory_vtable[15] = reinterpret_cast<void *>(
        &fake_create_swap_chain_for_hwnd);
    void *swap_chain_vtable[37] = {};
    swap_chain_vtable[0] = reinterpret_cast<void *>(
        &fake_swap_chain_query_interface);
    swap_chain_vtable[2] = reinterpret_cast<void *>(
        &fake_swap_chain_release);
    swap_chain_vtable[36] = reinterpret_cast<void *>(
        &fake_get_current_back_buffer_index);

    FakeCommandQueue queue = {};
    queue.vtable = queue_vtable;
    FakeSwapChain intermediate = {};
    intermediate.vtable = swap_chain_vtable;
    FakeSwapChain final_swap_chain = {};
    final_swap_chain.vtable = swap_chain_vtable;
    final_swap_chain.current_index = 1;
    intermediate.query_interface_result =
        reinterpret_cast<IDXGISwapChain3 *>(&final_swap_chain);
    intermediate.query_interface_result_hr = S_OK;
    FakeDevice device = {};
    device.vtable = device_vtable;
    device.command_queue_result =
        reinterpret_cast<ID3D12CommandQueue *>(&queue);
    device.command_queue_result_hr = S_OK;
    FakeFactory factory = {};
    factory.vtable = factory_vtable;
    factory.swap_chain_result =
        reinterpret_cast<IDXGISwapChain1 *>(&intermediate);
    factory.result = S_OK;

    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    framework->m_pFactory = reinterpret_cast<IDXGIFactory6 *>(&factory);
    std::vector<unsigned char> application_storage(0x383A0, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;
    *reinterpret_cast<UINT *>(application_storage.data() + 0x3831C) = 1920;
    *reinterpret_cast<UINT *>(application_storage.data() + 0x38320) = 1080;
    HWND window = reinterpret_cast<HWND>(
        static_cast<std::uintptr_t>(0x12340000));

    CHECK(application->InitializeSwapChain(window) == S_OK);
    CHECK(device.command_queue_calls == 1);
    CHECK(device.command_queue_description.Type ==
          D3D12_COMMAND_LIST_TYPE_DIRECT);
    CHECK(device.command_queue_description.Priority ==
          D3D12_COMMAND_QUEUE_PRIORITY_NORMAL);
    CHECK(device.command_queue_description.Flags ==
          D3D12_COMMAND_QUEUE_FLAG_NONE);
    CHECK(device.command_queue_description.NodeMask == 0);
    CHECK(*device.command_queue_iid == __uuidof(ID3D12CommandQueue));
    CHECK(framework->m_pCommandQueue ==
          reinterpret_cast<ID3D12CommandQueue *>(&queue));
    CHECK(queue.set_name_calls == 1);
    CHECK(std::wcscmp(queue.last_name, L"Command Queue") == 0);
    CHECK(factory.create_swap_chain_calls == 1);
    CHECK(factory.queue == reinterpret_cast<IUnknown *>(&queue));
    CHECK(factory.window == window);
    CHECK(factory.description.Width == 1920);
    CHECK(factory.description.Height == 1080);
    CHECK(factory.description.Format == DXGI_FORMAT_R8G8B8A8_UNORM);
    CHECK(factory.description.Stereo == FALSE);
    CHECK(factory.description.SampleDesc.Count == 1);
    CHECK(factory.description.SampleDesc.Quality == 0);
    CHECK(factory.description.BufferUsage == DXGI_USAGE_RENDER_TARGET_OUTPUT);
    CHECK(factory.description.BufferCount == 2);
    CHECK(factory.description.Scaling == DXGI_SCALING_STRETCH);
    CHECK(factory.description.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD);
    CHECK(factory.description.AlphaMode == DXGI_ALPHA_MODE_UNSPECIFIED);
    CHECK(factory.description.Flags == 0);
    CHECK(factory.fullscreen_description == nullptr);
    CHECK(factory.restrict_output == nullptr);
    CHECK(framework->m_pSwapChainDesc != nullptr);
    CHECK(std::memcmp(
              framework->m_pSwapChainDesc,
              &factory.description,
              sizeof(factory.description)) == 0);
    CHECK(intermediate.query_interface_calls == 1);
    CHECK(*intermediate.query_interface_iid ==
          __uuidof(IDXGISwapChain3));
    CHECK(intermediate.release_calls == 1);
    CHECK(framework->m_pSwapChain ==
          reinterpret_cast<IDXGISwapChain3 *>(&final_swap_chain));
    CHECK(final_swap_chain.current_index_calls == 1);
    CHECK(framework->m_nCurrentFrameIndex == 1);
    delete framework->m_pSwapChainDesc;
    framework->m_pSwapChainDesc = nullptr;

    queue = {};
    queue.vtable = queue_vtable;
    device = {};
    device.vtable = device_vtable;
    device.command_queue_result =
        reinterpret_cast<ID3D12CommandQueue *>(&queue);
    device.command_queue_result_hr = E_FAIL;
    factory = {};
    factory.vtable = factory_vtable;
    factory.result = E_ACCESSDENIED;
    final_swap_chain.current_index_calls = 0;
    final_swap_chain.current_index = 0;
    framework->m_pSwapChain =
        reinterpret_cast<IDXGISwapChain3 *>(&final_swap_chain);
    CHECK(application->InitializeSwapChain(window) == E_ACCESSDENIED);
    CHECK(device.command_queue_calls == 1);
    CHECK(queue.set_name_calls == 1);
    CHECK(factory.create_swap_chain_calls == 1);
    CHECK(final_swap_chain.current_index_calls == 1);
    CHECK(framework->m_nCurrentFrameIndex == 0);
    delete framework->m_pSwapChainDesc;
    framework->m_pSwapChainDesc = nullptr;

    device.command_queue_calls = 0;
    queue.set_name_calls = 0;
    factory = {};
    factory.vtable = factory_vtable;
    factory.swap_chain_result =
        reinterpret_cast<IDXGISwapChain1 *>(&intermediate);
    factory.result = S_OK;
    intermediate.query_interface_calls = 0;
    intermediate.query_interface_result = nullptr;
    intermediate.query_interface_result_hr = E_NOINTERFACE;
    intermediate.release_calls = 0;
    final_swap_chain.current_index_calls = 0;
    final_swap_chain.current_index = 1;
    framework->m_pSwapChain =
        reinterpret_cast<IDXGISwapChain3 *>(&final_swap_chain);
    CHECK(application->InitializeSwapChain(window) == E_NOINTERFACE);
    CHECK(intermediate.query_interface_calls == 1);
    CHECK(intermediate.release_calls == 1);
    CHECK(final_swap_chain.current_index_calls == 1);
    CHECK(framework->m_nCurrentFrameIndex == 1);
    delete framework->m_pSwapChainDesc;
    framework->m_pSwapChainDesc = nullptr;
    return 0;
}

static int test_move_to_next_frame()
{
    void *queue_vtable[15] = {};
    queue_vtable[14] = reinterpret_cast<void *>(&fake_queue_signal);
    void *fence_vtable[10] = {};
    fence_vtable[8] =
        reinterpret_cast<void *>(&fake_fence_get_completed_value);
    fence_vtable[9] = reinterpret_cast<void *>(&fake_fence_set_event);
    void *swap_chain_vtable[37] = {};
    swap_chain_vtable[36] = reinterpret_cast<void *>(
        &fake_get_current_back_buffer_index);

    FakeCommandQueue queue = {};
    queue.vtable = queue_vtable;
    queue.result = S_OK;
    FakeFence old_fence = {};
    old_fence.vtable = fence_vtable;
    FakeFence new_fence = {};
    new_fence.vtable = fence_vtable;
    new_fence.result = S_OK;
    new_fence.completed_value = 2;
    FakeSwapChain swap_chain = {};
    swap_chain.vtable = swap_chain_vtable;
    swap_chain.current_index = 1;
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CHECK(event != nullptr);

    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pCommandQueue =
        reinterpret_cast<ID3D12CommandQueue *>(&queue);
    framework->m_pSwapChain = reinterpret_cast<IDXGISwapChain3 *>(
        &swap_chain);
    framework->m_nCurrentFrameIndex = 0;
    framework->m_pFence[0] = reinterpret_cast<ID3D12Fence *>(&old_fence);
    framework->m_pFence[1] = reinterpret_cast<ID3D12Fence *>(&new_fence);
    framework->m_hFenceEvent = event;
    framework->m_nFenceValues[0] = 7;
    framework->m_nFenceValues[1] = 3;
    std::vector<unsigned char> application_storage(0x100, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;

    const double saved_seconds_per_frame = gSecondsPerFrame;
    gSecondsPerFrame = 0.0;
    application->MoveToNextFrame();
    CHECK(queue.signal_calls == 1);
    CHECK(queue.fence == framework->m_pFence[0]);
    CHECK(queue.value == 7);
    CHECK(swap_chain.current_index_calls == 1);
    CHECK(framework->m_nCurrentFrameIndex == 1);
    CHECK(new_fence.completed_value_calls == 2);
    CHECK(new_fence.event_calls == 1);
    CHECK(new_fence.value == 3);
    CHECK(new_fence.event == event);
    CHECK(framework->m_nFenceValues[0] == 7);
    CHECK(framework->m_nFenceValues[1] == 8);

    queue.result = E_ACCESSDENIED;
    framework->m_nCurrentFrameIndex = 0;
    framework->m_nFenceValues[0] = 11;
    application->MoveToNextFrame();
    CHECK(queue.signal_calls == 2);
    CHECK(swap_chain.current_index_calls == 1);
    CHECK(framework->m_nCurrentFrameIndex == 0);
    CHECK(framework->m_nFenceValues[0] == 11);

    queue.result = S_OK;
    new_fence.result = E_FAIL;
    new_fence.completed_value = 1;
    framework->m_nCurrentFrameIndex = 0;
    framework->m_nFenceValues[0] = 13;
    framework->m_nFenceValues[1] = 5;
    application->MoveToNextFrame();
    CHECK(new_fence.completed_value_calls == 4);
    CHECK(new_fence.event_calls == 2);
    CHECK(framework->m_nFenceValues[1] == 5);

    new_fence.result = S_OK;
    new_fence.completed_value = 99;
    framework->m_nCurrentFrameIndex = 0;
    framework->m_nFenceValues[0] = 17;
    framework->m_nFenceValues[1] = 9;
    application->MoveToNextFrame();
    CHECK(new_fence.completed_value_calls == 6);
    CHECK(new_fence.event_calls == 2);
    CHECK(framework->m_nFenceValues[1] == 18);
    gSecondsPerFrame = saved_seconds_per_frame;
    CHECK(CloseHandle(event) != FALSE);
    return 0;
}

static int test_frame_pacing_pause_and_gpu_wait()
{
    std::vector<unsigned char> application_storage(0x100, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->CD3DApplication::Pause(1);
    CHECK(*reinterpret_cast<int *>(application_storage.data() + 0x24) == 0);
    application->CD3DApplication::Pause(1);
    CHECK(*reinterpret_cast<int *>(application_storage.data() + 0x24) == 0);
    application->CD3DApplication::Pause(0);
    CHECK(*reinterpret_cast<int *>(application_storage.data() + 0x24) == 0);
    application->CD3DApplication::Pause(0);
    CHECK(*reinterpret_cast<int *>(application_storage.data() + 0x24) == 1);

    CHECK(gQueryResult != 0);
    CHECK(gFrequency.QuadPart > 0);
    CHECK(gFramesPerSecond == 60.0);
    const double saved_seconds_per_frame = gSecondsPerFrame;
    gSecondsPerFrame = 0.0;
    application->CapFrameRate();
    gSecondsPerFrame = saved_seconds_per_frame;

    void *queue_vtable[15] = {};
    queue_vtable[14] = reinterpret_cast<void *>(&fake_queue_signal);
    void *fence_vtable[10] = {};
    fence_vtable[9] = reinterpret_cast<void *>(&fake_fence_set_event);
    FakeCommandQueue queue = {};
    queue.vtable = queue_vtable;
    queue.result = S_OK;
    FakeFence fence = {};
    fence.vtable = fence_vtable;
    fence.result = S_OK;
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CHECK(event != nullptr);

    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pCommandQueue =
        reinterpret_cast<ID3D12CommandQueue *>(&queue);
    framework->m_nCurrentFrameIndex = 1;
    framework->m_pFence[1] = reinterpret_cast<ID3D12Fence *>(&fence);
    framework->m_hFenceEvent = event;
    framework->m_nFenceValues[1] = 41;
    application->m_pFramework = framework;
    application->WaitForGpu();
    CHECK(queue.signal_calls == 1);
    CHECK(queue.fence == framework->m_pFence[1]);
    CHECK(queue.value == 41);
    CHECK(fence.event_calls == 1);
    CHECK(fence.value == 41);
    CHECK(fence.event == event);
    CHECK(framework->m_nFenceValues[1] == 42);

    queue.result = E_FAIL;
    application->WaitForGpu();
    CHECK(queue.signal_calls == 2);
    CHECK(fence.event_calls == 1);
    CHECK(framework->m_nFenceValues[1] == 42);
    queue.result = S_OK;
    fence.result = E_FAIL;
    application->WaitForGpu();
    CHECK(queue.signal_calls == 3);
    CHECK(fence.event_calls == 2);
    CHECK(framework->m_nFenceValues[1] == 42);
    application->m_pFramework = nullptr;
    application->WaitForGpu();
    CHECK(queue.signal_calls == 3);
    CHECK(CloseHandle(event) != FALSE);
    return 0;
}

static int test_destroy_d3d12_objects()
{
#ifdef JPB_D3DAPP_REAL_ASSET_DIR
    CHECK(SetDllDirectoryA(JPB_D3DAPP_REAL_ASSET_DIR) != FALSE);
    HMODULE sdl = LoadLibraryA("SDL2.dll");
    CHECK(sdl != nullptr);
    using InitFunction = int (*)(unsigned int);
    using CreateWindowFunction = void *(*)(
        const char *, int, int, int, int, unsigned int);
    using WasInitFunction = unsigned int (*)(unsigned int);
    auto init = reinterpret_cast<InitFunction>(
        GetProcAddress(sdl, "SDL_Init"));
    auto create_window = reinterpret_cast<CreateWindowFunction>(
        GetProcAddress(sdl, "SDL_CreateWindow"));
    auto was_init = reinterpret_cast<WasInitFunction>(
        GetProcAddress(sdl, "SDL_WasInit"));
    CHECK(init != nullptr);
    CHECK(create_window != nullptr);
    CHECK(was_init != nullptr);
    CHECK(init(0x20) == 0);
    void *window = create_window(
        "d3dapp-destroy-test", 0x1FFF0000, 0x1FFF0000,
        64, 48, 0x8);
    CHECK(window != nullptr);

    void *unknown_vtable[3] = {};
    unknown_vtable[2] = reinterpret_cast<void *>(&fake_unknown_release);
    FakeUnknown owners[6] = {};
    for (auto &owner : owners) {
        owner.vtable = unknown_vtable;
    }
    void *queue_vtable[15] = {};
    queue_vtable[14] = reinterpret_cast<void *>(&fake_queue_signal);
    void *fence_vtable[10] = {};
    fence_vtable[9] = reinterpret_cast<void *>(&fake_fence_set_event);
    FakeCommandQueue queue = {};
    queue.vtable = queue_vtable;
    queue.result = S_OK;
    FakeFence fence = {};
    fence.vtable = fence_vtable;
    fence.result = S_OK;
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CHECK(event != nullptr);

    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pCommandQueue =
        reinterpret_cast<ID3D12CommandQueue *>(&queue);
    framework->m_nCurrentFrameIndex = 0;
    framework->m_pFence[0] = reinterpret_cast<ID3D12Fence *>(&fence);
    framework->m_hFenceEvent = event;
    framework->m_nFenceValues[0] = 4;
    framework->m_pRenderTargets[0] =
        reinterpret_cast<ID3D12Resource *>(&owners[0]);
    framework->m_pRenderTargets[1] =
        reinterpret_cast<ID3D12Resource *>(&owners[1]);
    framework->m_pRtvDescriptorHeap =
        reinterpret_cast<ID3D12DescriptorHeap *>(&owners[2]);
    framework->m_pSwapChain =
        reinterpret_cast<IDXGISwapChain3 *>(&owners[3]);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&owners[4]);
    framework->m_pFactory = reinterpret_cast<IDXGIFactory6 *>(&owners[5]);
    std::vector<unsigned char> application_storage(0x100, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;
    *reinterpret_cast<void **>(application_storage.data() + 0xD8) = window;

    application->DestroyD3D12Objects();
    CHECK(queue.signal_calls == 1);
    CHECK(queue.fence == framework->m_pFence[0]);
    CHECK(queue.value == 4);
    CHECK(fence.event_calls == 1);
    CHECK(framework->m_nFenceValues[0] == 5);
    for (const auto &owner : owners) {
        CHECK(owner.release_calls == 1);
    }
    CHECK(framework->m_pRenderTargets[0] == nullptr);
    CHECK(framework->m_pRenderTargets[1] == nullptr);
    CHECK(framework->m_pRtvDescriptorHeap == nullptr);
    CHECK(framework->m_pSwapChain == nullptr);
    CHECK(framework->m_pDevice == nullptr);
    CHECK(framework->m_pFactory == nullptr);
    CHECK(*reinterpret_cast<void **>(application_storage.data() + 0xD8) ==
          nullptr);
    CHECK(was_init(0) == 0);
    CHECK(CloseHandle(event) != FALSE);
    CHECK(FreeLibrary(sdl) != FALSE);
    CHECK(SetDllDirectoryA(nullptr) != FALSE);
#endif
    return 0;
}

static int test_start_render_and_frame_begin()
{
    void *queue_vtable[15] = {};
    queue_vtable[14] = reinterpret_cast<void *>(&fake_queue_signal);
    void *fence_vtable[10] = {};
    fence_vtable[9] = reinterpret_cast<void *>(&fake_fence_set_event);
    void *allocator_vtable[9] = {};
    allocator_vtable[8] = reinterpret_cast<void *>(
        &fake_render_allocator_reset);
    void *resource_vtable[12] = {};
    resource_vtable[11] = reinterpret_cast<void *>(
        &fake_render_resource_gpu_address);
    void *command_list_vtable[49] = {};
    command_list_vtable[9] = reinterpret_cast<void *>(
        &fake_render_command_list_close);
    command_list_vtable[10] = reinterpret_cast<void *>(
        &fake_render_command_list_reset);
    command_list_vtable[21] = reinterpret_cast<void *>(
        &fake_render_set_viewports);
    command_list_vtable[22] = reinterpret_cast<void *>(
        &fake_render_set_scissors);
    command_list_vtable[25] = reinterpret_cast<void *>(
        &fake_render_set_pipeline_state);
    command_list_vtable[26] = reinterpret_cast<void *>(
        &fake_render_resource_barrier);
    command_list_vtable[28] = reinterpret_cast<void *>(
        &fake_render_set_descriptor_heaps);
    command_list_vtable[30] = reinterpret_cast<void *>(
        &fake_render_set_root_signature);
    command_list_vtable[32] = reinterpret_cast<void *>(
        &fake_render_set_root_table);
    command_list_vtable[38] = reinterpret_cast<void *>(
        &fake_render_set_root_cbv);
    command_list_vtable[46] = reinterpret_cast<void *>(
        &fake_render_set_render_targets);
    command_list_vtable[47] = reinterpret_cast<void *>(
        &fake_render_clear_depth);
    command_list_vtable[48] = reinterpret_cast<void *>(
        &fake_render_clear_render_target);

    FakeCommandQueue queue = {};
    queue.vtable = queue_vtable;
    queue.result = E_FAIL;
    FakeFence fence = {};
    fence.vtable = fence_vtable;
    fence.result = S_OK;
    FakeRenderCommandAllocator allocator = {};
    allocator.vtable = allocator_vtable;
    FakeRenderResource constant_buffer = {};
    constant_buffer.vtable = resource_vtable;
    constant_buffer.gpu_address = 0x123456789ABCULL;
    FakeRenderCommandList command_list = {};
    command_list.vtable = command_list_vtable;
    FakeDescriptorHeap cbv_heap;
    FakeDescriptorHeap main_heap;
    main_heap.gpu_handle.ptr = 0xABC000;
    FakeDescriptorHeap rtv_heap;
    rtv_heap.cpu_handle.ptr = 0x1000;
    FakeDescriptorHeap dsv_heap;
    dsv_heap.cpu_handle.ptr = 0x2000;
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CHECK(event != nullptr);

    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pCommandQueue =
        reinterpret_cast<ID3D12CommandQueue *>(&queue);
    framework->m_pFence[1] = reinterpret_cast<ID3D12Fence *>(&fence);
    framework->m_hFenceEvent = event;
    framework->m_nCurrentFrameIndex = 1;
    framework->m_nFrameIndex = 1;
    framework->m_nFenceValues[1] = 5;
    std::vector<unsigned char> application_storage(0x310974, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;

    CHECK(application->StartRender() ==
          static_cast<HRESULT>(0x88760028));
    CHECK(queue.signal_calls == 1);

    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(
        static_cast<std::uintptr_t>(0x11110000));
    framework->m_pCommandAllocators[1] =
        reinterpret_cast<ID3D12CommandAllocator *>(&allocator);
    allocator.reset_result = E_ACCESSDENIED;
    CHECK(application->StartRender() == E_ACCESSDENIED);
    CHECK(allocator.reset_calls == 1);

    allocator.reset_calls = 0;
    allocator.reset_result = S_OK;
    framework->m_pCommandList =
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list);
    command_list.reset_result = E_OUTOFMEMORY;
    framework->m_pPipelineState = reinterpret_cast<ID3D12PipelineState *>(
        static_cast<std::uintptr_t>(0x22220000));
    framework->m_commandListOpen = false;
    CHECK(application->StartRender() == E_OUTOFMEMORY);
    CHECK(allocator.reset_calls == 1);
    CHECK(command_list.reset_calls == 1);
    CHECK(command_list.reset_allocator ==
          reinterpret_cast<ID3D12CommandAllocator *>(&allocator));
    CHECK(command_list.reset_pipeline_state == framework->m_pPipelineState);
    CHECK(framework->m_commandListOpen == true);

    queue = {};
    queue.vtable = queue_vtable;
    queue.result = S_OK;
    fence = {};
    fence.vtable = fence_vtable;
    fence.result = S_OK;
    allocator = {};
    allocator.vtable = allocator_vtable;
    allocator.reset_result = S_OK;
    command_list = {};
    command_list.vtable = command_list_vtable;
    command_list.reset_result = S_OK;
    framework->m_pCommandQueue =
        reinterpret_cast<ID3D12CommandQueue *>(&queue);
    framework->m_pFence[1] = reinterpret_cast<ID3D12Fence *>(&fence);
    framework->m_nFenceValues[1] = 5;
    framework->m_pCommandAllocators[1] =
        reinterpret_cast<ID3D12CommandAllocator *>(&allocator);
    framework->m_pCommandList =
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list);
    framework->m_commandListOpen = false;
    framework->m_pRootSignature = reinterpret_cast<ID3D12RootSignature *>(
        static_cast<std::uintptr_t>(0x33330000));
    framework->m_Viewport = {1.0f, 2.0f, 640.0f, 480.0f, 0.1f, 0.9f};
    framework->m_ScissorRect = {3, 4, 603, 404};
    framework->m_cbvHeap = &cbv_heap;
    framework->m_constantBuffer =
        reinterpret_cast<ID3D12Resource *>(&constant_buffer);
    framework->m_pMainDescriptorHeap = &main_heap;
    framework->m_pRenderTargets[1] = reinterpret_cast<ID3D12Resource *>(
        static_cast<std::uintptr_t>(0x44440000));
    framework->m_pRtvDescriptorHeap = &rtv_heap;
    framework->m_RTVDescriptorSize = 64;
    framework->m_pDsDescriptorHeap = &dsv_heap;

    CHECK(application->StartRender() == S_OK);
    CHECK(queue.signal_calls == 3);
    CHECK(fence.event_calls == 3);
    CHECK(framework->m_nFenceValues[1] == 8);
    CHECK(allocator.reset_calls == 1);
    CHECK(command_list.close_calls == 0);
    CHECK(command_list.reset_calls == 1);
    CHECK(command_list.call_count == 13);
    for (unsigned int index = 0; index < 13; ++index) {
        CHECK(command_list.calls[index] == index + 1);
    }
    CHECK(command_list.pipeline_state == framework->m_pPipelineState);
    CHECK(command_list.root_signature == framework->m_pRootSignature);
    CHECK(command_list.viewport_count == 1);
    CHECK(std::memcmp(
              &command_list.viewport,
              &framework->m_Viewport,
              sizeof(framework->m_Viewport)) == 0);
    CHECK(command_list.scissor_count == 1);
    CHECK(std::memcmp(
              &command_list.scissor,
              &framework->m_ScissorRect,
              sizeof(framework->m_ScissorRect)) == 0);
    CHECK(command_list.descriptor_heap_calls == 2);
    CHECK(command_list.descriptor_heaps[0] == &cbv_heap);
    CHECK(command_list.descriptor_heaps[1] == &main_heap);
    CHECK(constant_buffer.gpu_address_calls == 1);
    CHECK(command_list.root_cbv_index == 0);
    CHECK(command_list.root_cbv_address == constant_buffer.gpu_address);
    CHECK(main_heap.gpu_handle_calls == 1);
    CHECK(command_list.root_table_index == 1);
    CHECK(command_list.root_table_handle.ptr == 0xABC000);
    CHECK(command_list.barrier_count == 1);
    CHECK(command_list.barrier.Type ==
          D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);
    CHECK(command_list.barrier.Flags == D3D12_RESOURCE_BARRIER_FLAG_NONE);
    CHECK(command_list.barrier.Transition.pResource ==
          framework->m_pRenderTargets[1]);
    CHECK(command_list.barrier.Transition.Subresource ==
          D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    CHECK(command_list.barrier.Transition.StateBefore ==
          D3D12_RESOURCE_STATE_PRESENT);
    CHECK(command_list.barrier.Transition.StateAfter ==
          D3D12_RESOURCE_STATE_RENDER_TARGET);
    CHECK(rtv_heap.cpu_handle_calls == 1);
    CHECK(dsv_heap.cpu_handle_calls == 2);
    CHECK(command_list.render_target_count == 1);
    CHECK(command_list.render_target_handle.ptr == 0x1040);
    CHECK(command_list.render_targets_single_range == FALSE);
    CHECK(command_list.depth_stencil_handle.ptr == 0x2000);
    CHECK(command_list.clear_render_target_handle.ptr == 0x1040);
    CHECK(command_list.clear_color[0] == 0.0f);
    CHECK(command_list.clear_color[1] == 0.0f);
    CHECK(command_list.clear_color[2] == 0.0f);
    CHECK(command_list.clear_color[3] == 1.0f);
    CHECK(command_list.clear_render_target_rect_count == 0);
    CHECK(command_list.clear_render_target_rects == nullptr);
    CHECK(command_list.clear_depth_handle.ptr == 0x2000);
    CHECK(command_list.clear_depth_flags == D3D12_CLEAR_FLAG_DEPTH);
    CHECK(command_list.clear_depth == 1.0f);
    CHECK(command_list.clear_stencil == 0);
    CHECK(command_list.clear_depth_rect_count == 0);
    CHECK(command_list.clear_depth_rects == nullptr);

    HWND window = CreateWindowExA(
        0, "STATIC", "d3dapp-frame-begin-test", WS_POPUP,
        0, 0, 32, 32, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(window != nullptr);
    framework->m_pDevice = nullptr;
    *reinterpret_cast<HWND *>(application_storage.data() + 0xD0) = window;
    *reinterpret_cast<int *>(application_storage.data() + 0x310970) =
        0x12345678;
    CHECK(PostMessageA(window, WM_NULL, 0, 0) != FALSE);
    CHECK(application->FrameBegin() == S_OK);
    CHECK(*reinterpret_cast<int *>(application_storage.data() + 0x310970) ==
          0);
    MSG message = {};
    CHECK(PeekMessageA(&message, window, 0, 0, PM_NOREMOVE) == FALSE);
    CHECK(DestroyWindow(window) != FALSE);
    CHECK(CloseHandle(event) != FALSE);
    return 0;
}

static int test_frame_end()
{
    void *device_vtable[28] = {};
    device_vtable[27] = reinterpret_cast<void *>(
        &fake_create_committed_resource);
    void *resource_vtable[12] = {};
    resource_vtable[2] = reinterpret_cast<void *>(
        &fake_transparency_resource_release);
    resource_vtable[8] = reinterpret_cast<void *>(
        &fake_transparency_resource_map);
    resource_vtable[9] = reinterpret_cast<void *>(
        &fake_transparency_resource_unmap);
    resource_vtable[11] = reinterpret_cast<void *>(
        &fake_transparency_resource_gpu_address);
    void *command_list_vtable[47] = {};
    command_list_vtable[9] = reinterpret_cast<void *>(
        &fake_render_command_list_close);
    command_list_vtable[13] = reinterpret_cast<void *>(
        &fake_render_draw_indexed);
    command_list_vtable[15] = reinterpret_cast<void *>(
        &fake_render_copy_buffer_region);
    command_list_vtable[20] = reinterpret_cast<void *>(
        &fake_render_set_topology);
    command_list_vtable[25] = reinterpret_cast<void *>(
        &fake_render_set_pipeline_state);
    command_list_vtable[26] = reinterpret_cast<void *>(
        &fake_render_resource_barrier);
    command_list_vtable[43] = reinterpret_cast<void *>(
        &fake_render_set_index_buffer);
    command_list_vtable[44] = reinterpret_cast<void *>(
        &fake_render_set_vertex_buffers);
    command_list_vtable[46] = reinterpret_cast<void *>(
        &fake_render_set_render_targets);
    void *queue_vtable[15] = {};
    queue_vtable[10] = reinterpret_cast<void *>(
        &fake_queue_execute_command_lists);
    queue_vtable[14] = reinterpret_cast<void *>(&fake_queue_signal);
    void *fence_vtable[10] = {};
    fence_vtable[8] = reinterpret_cast<void *>(
        &fake_fence_get_completed_value);
    fence_vtable[9] = reinterpret_cast<void *>(
        &fake_fence_set_event);
    void *swap_chain_vtable[37] = {};
    swap_chain_vtable[8] = reinterpret_cast<void *>(
        &fake_swap_chain_present);
    swap_chain_vtable[36] = reinterpret_cast<void *>(
        &fake_get_current_back_buffer_index);

    std::vector<unsigned char> uploaded_vertices(0x2D8000, 0);
    std::vector<unsigned char> uploaded_indices(0x38000, 0);
    FakeTransparencyResource resources[4] = {};
    for (unsigned int index = 0; index < 4; ++index) {
        resources[index].vtable = resource_vtable;
        resources[index].map_result = S_OK;
        resources[index].gpu_address = 0x100000 + index * 0x10000;
    }
    resources[0].mapped_data = uploaded_vertices.data();
    resources[1].mapped_data = uploaded_indices.data();

    FakeDevice device = {};
    device.vtable = device_vtable;
    device.committed_resource_result_hr = S_OK;
    for (unsigned int index = 0; index < 4; ++index) {
        device.committed_resource_results[index] =
            reinterpret_cast<ID3D12Resource *>(&resources[index]);
    }
    FakeRenderCommandList command_list = {};
    command_list.vtable = command_list_vtable;
    command_list.close_result = S_OK;
    FakeCommandQueue queue = {};
    queue.vtable = queue_vtable;
    queue.result = S_OK;
    FakeFence fences[2] = {};
    for (FakeFence &fence : fences) {
        fence.vtable = fence_vtable;
        fence.result = S_OK;
        fence.completed_value = 100;
    }
    FakeSwapChain swap_chain = {};
    swap_chain.vtable = swap_chain_vtable;
    swap_chain.present_result = S_OK;
    swap_chain.current_index = 1;
    FakeDescriptorHeap rtv_heap;
    rtv_heap.cpu_handle.ptr = 0x1000;
    FakeDescriptorHeap dsv_heap;
    dsv_heap.cpu_handle.ptr = 0x2000;
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CHECK(event != nullptr);

    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    framework->m_pCommandList =
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list);
    framework->m_pCommandQueue =
        reinterpret_cast<ID3D12CommandQueue *>(&queue);
    framework->m_pSwapChain =
        reinterpret_cast<IDXGISwapChain3 *>(&swap_chain);
    framework->m_pRtvDescriptorHeap = &rtv_heap;
    framework->m_pDsDescriptorHeap = &dsv_heap;
    framework->m_RTVDescriptorSize = 32;
    framework->m_nFrameIndex = 0;
    framework->m_nCurrentFrameIndex = 0;
    framework->m_pRenderTargets[0] = reinterpret_cast<ID3D12Resource *>(
        static_cast<std::uintptr_t>(0x3000));
    framework->m_pPipelineState = reinterpret_cast<ID3D12PipelineState *>(
        static_cast<std::uintptr_t>(0x4000));
    framework->m_pFence[0] = reinterpret_cast<ID3D12Fence *>(&fences[0]);
    framework->m_pFence[1] = reinterpret_cast<ID3D12Fence *>(&fences[1]);
    framework->m_nFenceValues[0] = 7;
    framework->m_nFenceValues[1] = 11;
    framework->m_hFenceEvent = event;
    framework->m_graphicsMemory = reinterpret_cast<void *>(
        static_cast<std::uintptr_t>(0x5000));
    framework->m_commandListOpen = true;

    std::vector<unsigned char> application_storage(0x3103D0, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;
    auto *sprite_draws = new (application_storage.data() + 0x58)
        std::vector<SpriteDraw>();
    auto *level_meshes = new (application_storage.data() + 0x3103A8)
        std::vector<CD3DApplication::FBX_MESH *>();

    alignas(D3DTransparencyPass)
        unsigned char transparency_storage[sizeof(D3DTransparencyPass)] = {};
    auto *transparency_pass = reinterpret_cast<D3DTransparencyPass *>(
        transparency_storage);
    auto *transparent_vertices = new (&transparency_pass->m_vertices)
        std::vector<Vertex>();
    auto *transparent_indices = new (&transparency_pass->m_indices)
        std::vector<unsigned int>();
    auto *additive_vertices = new (
        &transparency_pass->m_additiveVertices) std::vector<Vertex>();
    auto *additive_indices = new (
        &transparency_pass->m_additiveIndices) std::vector<unsigned int>();
    transparency_pass->m_pFramework = framework;
    *reinterpret_cast<D3DTransparencyPass **>(
        application_storage.data() + 0xF0) = transparency_pass;

    std::memset(application_storage.data() + 0x383A0, 0x31, 0x138000);
    std::memset(application_storage.data() + 0x1703A0, 0x42, 0x1A0000);
    std::memset(application_storage.data() + 0x100, 0x53, 0x18000);
    std::memset(application_storage.data() + 0x18100, 0x64, 0x20000);
    *reinterpret_cast<int *>(application_storage.data() + 0x3C) = 2;
    *reinterpret_cast<int *>(application_storage.data() + 0x40) = 1;
    *reinterpret_cast<int *>(application_storage.data() + 0x38100) = 19;
    std::vector<unsigned char> expected_indices(0x38000);
    std::memcpy(
        expected_indices.data(),
        application_storage.data() + 0x18100,
        0x20000);
    std::memcpy(
        expected_indices.data() + 0x20000,
        application_storage.data() + 0x100,
        0x18000);

    fake_graphics_memory_commit_calls = 0;
    fake_graphics_memory_commit_memory = nullptr;
    fake_graphics_memory_commit_queue = nullptr;
    jpb_d3dapp_set_graphics_memory_commit_test_hook(
        &fake_graphics_memory_commit);
    resolutionUpdated = 0;
    const double saved_seconds_per_frame = gSecondsPerFrame;
    gSecondsPerFrame = 0.0;

    CHECK(application->FrameEnd() == S_OK);
    CHECK(device.committed_resource_calls == 4);
    const D3D12_HEAP_TYPE expected_heap_types[4] = {
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_HEAP_TYPE_DEFAULT,
    };
    const UINT64 expected_widths[4] = {
        0x2D8000, 0x38000, 0x2D8000, 0x38000};
    const D3D12_RESOURCE_STATES expected_states[4] = {
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COMMON,
    };
    for (unsigned int index = 0; index < 4; ++index) {
        CHECK(device.committed_heap_properties_by_call[index].Type ==
              expected_heap_types[index]);
        CHECK(device.committed_heap_properties_by_call[index]
                  .CreationNodeMask == 1);
        CHECK(device.committed_heap_properties_by_call[index]
                  .VisibleNodeMask == 1);
        CHECK(device.committed_resource_descriptions_by_call[index]
                  .Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
        CHECK(device.committed_resource_descriptions_by_call[index]
                  .Width == expected_widths[index]);
        CHECK(device.committed_resource_descriptions_by_call[index]
                  .Layout == D3D12_TEXTURE_LAYOUT_ROW_MAJOR);
        CHECK(device.committed_initial_states_by_call[index] ==
              expected_states[index]);
    }
    CHECK(resources[0].map_calls == 1);
    CHECK(resources[1].map_calls == 1);
    CHECK(resources[0].unmap_calls == 1);
    CHECK(resources[1].unmap_calls == 1);
    CHECK(std::memcmp(
              uploaded_vertices.data(),
              application_storage.data() + 0x1703A0,
              0x1A0000) == 0);
    CHECK(std::memcmp(
              uploaded_vertices.data() + 0x1A0000,
              application_storage.data() + 0x383A0,
              0x138000) == 0);
    CHECK(uploaded_indices == expected_indices);
    CHECK(command_list.copy_buffer_calls == 2);
    CHECK(command_list.copy_sizes[0] == 0x2D8000);
    CHECK(command_list.copy_sizes[1] == 0x38000);
    CHECK(command_list.barrier_calls == 3);
    CHECK(command_list.barriers[0][0].Transition.pResource ==
          reinterpret_cast<ID3D12Resource *>(&resources[2]));
    CHECK(command_list.barriers[0][0].Transition.StateBefore ==
          D3D12_RESOURCE_STATE_COPY_DEST);
    CHECK(command_list.barriers[0][0].Transition.StateAfter ==
          D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    CHECK(command_list.barriers[1][0].Transition.pResource ==
          reinterpret_cast<ID3D12Resource *>(&resources[3]));
    CHECK(command_list.barriers[1][0].Transition.StateAfter ==
          D3D12_RESOURCE_STATE_INDEX_BUFFER);
    CHECK(command_list.topology_calls == 1);
    CHECK(command_list.topologies[0] ==
          D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    CHECK(command_list.vertex_buffer_views[0].BufferLocation ==
          resources[2].gpu_address);
    CHECK(command_list.vertex_buffer_views[0].SizeInBytes == 0x2D8000);
    CHECK(command_list.vertex_buffer_views[0].StrideInBytes ==
          sizeof(Vertex));
    CHECK(command_list.index_buffer_views[0].BufferLocation ==
          resources[3].gpu_address);
    CHECK(command_list.index_buffer_views[0].SizeInBytes == 0x38000);
    CHECK(command_list.index_buffer_views[0].Format ==
          DXGI_FORMAT_R16_UINT);
    CHECK(command_list.draw_indexed_calls == 3);
    CHECK(command_list.draw_index_counts[0] == 4);
    CHECK(command_list.draw_start_indices[0] == 0);
    CHECK(command_list.draw_index_counts[1] == 3);
    CHECK(command_list.draw_start_indices[1] == 0x8000);
    CHECK(command_list.draw_index_counts[2] == 3);
    CHECK(command_list.draw_start_indices[2] == 0x8003);
    CHECK(command_list.render_target_calls == 1);
    CHECK(command_list.render_target_handles[0].ptr == 0x1000);
    CHECK(command_list.depth_stencil_handles[0].ptr == 0x2000);
    CHECK(command_list.close_calls == 1);
    CHECK(queue.execute_calls == 1);
    CHECK(swap_chain.present_calls == 1);
    CHECK(swap_chain.sync_interval == 1);
    CHECK(swap_chain.present_flags == 0);
    CHECK(fake_graphics_memory_commit_calls == 1);
    CHECK(fake_graphics_memory_commit_memory == framework->m_graphicsMemory);
    CHECK(fake_graphics_memory_commit_queue == framework->m_pCommandQueue);
    CHECK(queue.signal_calls == 1);
    CHECK(swap_chain.current_index_calls == 1);
    CHECK(*reinterpret_cast<std::uint64_t *>(
              application_storage.data() + 0x3C) == 0);
    CHECK(*reinterpret_cast<int *>(
              application_storage.data() + 0x38100) == 0);
    for (std::size_t index = 0x100; index < 0x38100; ++index) {
        CHECK(application_storage[index] == 0);
    }

    gSecondsPerFrame = saved_seconds_per_frame;
    jpb_d3dapp_set_graphics_memory_commit_test_hook(nullptr);
    additive_indices->~vector();
    additive_vertices->~vector();
    transparent_indices->~vector();
    transparent_vertices->~vector();
    level_meshes->~vector();
    sprite_draws->~vector();
    CHECK(CloseHandle(event) != FALSE);
    return 0;
}

static int test_end_render()
{
    void *queue_vtable[15] = {};
    queue_vtable[10] = reinterpret_cast<void *>(
        &fake_queue_execute_command_lists);
    queue_vtable[14] = reinterpret_cast<void *>(&fake_queue_signal);
    void *command_list_vtable[49] = {};
    command_list_vtable[9] = reinterpret_cast<void *>(
        &fake_render_command_list_close);
    command_list_vtable[26] = reinterpret_cast<void *>(
        &fake_render_resource_barrier);

    FakeCommandQueue queue = {};
    queue.vtable = queue_vtable;
    FakeRenderCommandList command_list = {};
    command_list.vtable = command_list_vtable;
    command_list.close_result = E_ACCESSDENIED;
    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pCommandQueue =
        reinterpret_cast<ID3D12CommandQueue *>(&queue);
    framework->m_pCommandList =
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list);
    framework->m_pRenderTargets[1] = reinterpret_cast<ID3D12Resource *>(
        static_cast<std::uintptr_t>(0x55550000));
    framework->m_nFrameIndex = 1;
    framework->m_commandListOpen = true;
    std::vector<unsigned char> application_storage(0x310480, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    application->m_pFramework = framework;

    resolutionUpdated = 0;
    CHECK(application->EndRender() == S_OK);
    CHECK(command_list.barrier_count == 1);
    CHECK(command_list.barrier.Type ==
          D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);
    CHECK(command_list.barrier.Flags == D3D12_RESOURCE_BARRIER_FLAG_NONE);
    CHECK(command_list.barrier.Transition.pResource ==
          framework->m_pRenderTargets[1]);
    CHECK(command_list.barrier.Transition.Subresource ==
          D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    CHECK(command_list.barrier.Transition.StateBefore ==
          D3D12_RESOURCE_STATE_RENDER_TARGET);
    CHECK(command_list.barrier.Transition.StateAfter ==
          D3D12_RESOURCE_STATE_PRESENT);
    CHECK(command_list.close_calls == 1);
    CHECK(framework->m_commandListOpen == false);
    CHECK(queue.execute_calls == 1);
    CHECK(queue.execute_count == 1);
    CHECK(queue.executed_list ==
          reinterpret_cast<ID3D12CommandList *>(&command_list));

#ifdef JPB_D3DAPP_REAL_ASSET_DIR
    CHECK(SetDllDirectoryA(JPB_D3DAPP_REAL_ASSET_DIR) != FALSE);
    HMODULE sdl = LoadLibraryA("SDL2.dll");
    CHECK(sdl != nullptr);
    using InitFunction = int (*)(unsigned int);
    using CreateWindowFunction = void *(*)(
        const char *, int, int, int, int, unsigned int);
    using GetWindowSizeFunction = void (*)(void *, int *, int *);
    using DestroyWindowFunction = void (*)(void *);
    using QuitFunction = void (*)();
    auto init = reinterpret_cast<InitFunction>(
        GetProcAddress(sdl, "SDL_Init"));
    auto create_window = reinterpret_cast<CreateWindowFunction>(
        GetProcAddress(sdl, "SDL_CreateWindow"));
    auto get_window_size = reinterpret_cast<GetWindowSizeFunction>(
        GetProcAddress(sdl, "SDL_GetWindowSize"));
    auto destroy_window = reinterpret_cast<DestroyWindowFunction>(
        GetProcAddress(sdl, "SDL_DestroyWindow"));
    auto quit = reinterpret_cast<QuitFunction>(
        GetProcAddress(sdl, "SDL_Quit"));
    CHECK(init != nullptr);
    CHECK(create_window != nullptr);
    CHECK(get_window_size != nullptr);
    CHECK(destroy_window != nullptr);
    CHECK(quit != nullptr);
    CHECK(init(0x20) == 0);
    void *sdl_window = create_window(
        "d3dapp-end-render-test", 0x1FFF0000, 0x1FFF0000,
        64, 48, 0x8);
    CHECK(sdl_window != nullptr);

    void *fence_vtable[10] = {};
    fence_vtable[8] = reinterpret_cast<void *>(
        &fake_fence_get_completed_value);
    fence_vtable[9] = reinterpret_cast<void *>(
        &fake_fence_set_event);
    FakeFence fence0 = {};
    fence0.vtable = fence_vtable;
    fence0.result = S_OK;
    fence0.completed_value = 0;
    FakeFence fence1 = {};
    fence1.vtable = fence_vtable;
    fence1.result = S_OK;
    fence1.completed_value = 100;
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    CHECK(event != nullptr);
    queue = {};
    queue.vtable = queue_vtable;
    queue.result = S_OK;
    command_list = {};
    command_list.vtable = command_list_vtable;
    command_list.close_result = S_OK;
    std::memset(framework_storage.data(), 0, framework_storage.size());
    framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    framework->m_pCommandQueue =
        reinterpret_cast<ID3D12CommandQueue *>(&queue);
    framework->m_pCommandList =
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list);
    framework->m_pRenderTargets[1] = reinterpret_cast<ID3D12Resource *>(
        static_cast<std::uintptr_t>(0x66660000));
    framework->m_nFrameIndex = 1;
    framework->m_commandListOpen = true;
    framework->m_pFence[0] = reinterpret_cast<ID3D12Fence *>(&fence0);
    framework->m_pFence[1] = reinterpret_cast<ID3D12Fence *>(&fence1);
    framework->m_nFenceValues[0] = 10;
    framework->m_nFenceValues[1] = 20;
    framework->m_hFenceEvent = event;
    framework->m_pSDLWindow =
        reinterpret_cast<SDL_Window *>(sdl_window);
    application->m_pFramework = framework;
    newWidth = 320;
    newHeight = 240;
    newWindowMode = 2;
    resolutionUpdated = 1;
    fake_resize_resources_calls = 0;
    fake_resize_framework = nullptr;
    fake_resize_width = 0;
    fake_resize_height = 0;
    fake_resize_result = E_FAIL;
    jpb_d3dapp_set_resize_resources_test_hook(&fake_resize_resources);

    CHECK(application->EndRender() == S_OK);
    CHECK(resolutionUpdated == 0);
    CHECK(queue.execute_calls == 1);
    CHECK(queue.signal_calls == 3);
    CHECK(fence0.completed_value_calls == 1);
    CHECK(fence0.event_calls == 2);
    CHECK(fence1.completed_value_calls == 1);
    CHECK(fence1.event_calls == 0);
    CHECK(framework->m_nFenceValues[0] == 12);
    CHECK(framework->m_nFenceValues[1] == 21);
    CHECK(framework->m_nFrameIndex == 0);
    CHECK(fake_resize_resources_calls == 1);
    CHECK(fake_resize_framework == framework);
    CHECK(fake_resize_width == 320);
    CHECK(fake_resize_height == 240);
    CHECK(framework->m_dwRenderWidth == 320);
    CHECK(framework->m_dwRenderHeight == 240);
    CHECK(framework->m_hWnd != nullptr);
    CHECK(*reinterpret_cast<HWND *>(application_storage.data() + 0xD0) ==
          framework->m_hWnd);
    int actual_width = 0;
    int actual_height = 0;
    get_window_size(sdl_window, &actual_width, &actual_height);
    CHECK(actual_width == 320);
    CHECK(actual_height == 240);

    float field_of_view;
    const std::uint32_t field_of_view_bits = 0x3F6CCE68;
    std::memcpy(
        &field_of_view, &field_of_view_bits, sizeof(field_of_view));
    const DirectX::XMMATRIX expected_projection =
        DirectX::XMMatrixPerspectiveFovLH(
            field_of_view, 320.0f / 240.0f, 1.0f, 10000.0f);
    CHECK(std::memcmp(
              application_storage.data() + 0x310430,
              &expected_projection,
              sizeof(expected_projection)) == 0);

    jpb_d3dapp_set_resize_resources_test_hook(nullptr);
    CHECK(CloseHandle(event) != FALSE);
    destroy_window(sdl_window);
    quit();
    CHECK(FreeLibrary(sdl) != FALSE);
    CHECK(SetDllDirectoryA(nullptr) != FALSE);
#endif
    return 0;
}

static int test_lifecycle_callbacks()
{
    void *application_vtable[15] = {};
    application_vtable[2] = reinterpret_cast<void *>(
        &fake_application_delete_device_objects);
    application_vtable[5] = reinterpret_cast<void *>(
        &fake_application_final_cleanup);
    application_vtable[14] = reinterpret_cast<void *>(
        &fake_application_pause);
    std::vector<unsigned char> storage(0x383A0, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(storage.data());
    *reinterpret_cast<void ***>(application) = application_vtable;

    fake_pause_calls = 0;
    fake_pause_value = -1;
    CHECK(application->CD3DApplication::OnQuerySuspend(0xDEADBEEFu) == 1);
    CHECK(fake_pause_calls == 1);
    CHECK(fake_pause_value == 1);
    CHECK(application->CD3DApplication::OnResumeSuspend(0xA5A5A5A5u) == 1);
    CHECK(fake_pause_calls == 2);
    CHECK(fake_pause_value == 0);

    *reinterpret_cast<int *>(storage.data() + 0x20) = 1;
    *reinterpret_cast<int *>(storage.data() + 0x24) = 1;
#ifdef JPB_D3DAPP_REAL_ASSET_DIR
    CHECK(SetDllDirectoryA(JPB_D3DAPP_REAL_ASSET_DIR) != FALSE);
#endif
    application->m_pFramework = new CD3DFramework12();
    CHECK(application->m_pFramework->m_pDepthStencil == nullptr);
    CHECK(application->m_pFramework->m_pRenderTargets[0] == nullptr);
    CHECK(application->m_pFramework->m_pRenderTargets[1] == nullptr);
    CHECK(application->m_pFramework->m_pCommandAllocators[0] == nullptr);
    CHECK(application->m_pFramework->m_pCommandAllocators[1] == nullptr);
    CHECK(application->m_pFramework->m_pFence[0] == nullptr);
    CHECK(application->m_pFramework->m_pFence[1] == nullptr);
    CHECK(application->m_pFramework->m_pCommandList == nullptr);
    CHECK(application->m_pFramework->m_hFenceEvent == nullptr);
    CHECK(application->m_pFramework->m_pCommandQueue == nullptr);
    CHECK(application->m_pFramework->m_pRtvDescriptorHeap == nullptr);
    CHECK(application->m_pFramework->m_pSwapChain == nullptr);
    CHECK(application->m_pFramework->m_pDevice == nullptr);
    CHECK(application->m_pFramework->m_pFactory == nullptr);
    CHECK(application->m_pFramework->m_pRootSignature == nullptr);
    CHECK(application->m_pFramework->m_pPipelineState == nullptr);
    CHECK(application->m_pFramework->m_pLevelPipelineState == nullptr);
    CHECK(application->m_pFramework->m_pTransparentPipelineState == nullptr);
    application->m_pFramework->m_pTransparentGlassPipelineState = nullptr;
    CHECK(application->m_pFramework->m_vertexBuffer == nullptr);
    CHECK(application->m_pFramework->m_indexBuffer == nullptr);
    CHECK(application->m_pFramework->m_constantBuffer == nullptr);
    CHECK(application->m_pFramework->m_3DVertexBuffer == nullptr);
    CHECK(application->m_pFramework->m_3DIndexBuffer == nullptr);
    CHECK(application->m_pFramework->m_pMainDescriptorHeap == nullptr);
    CHECK(application->m_pFramework->m_pDsDescriptorHeap == nullptr);
    CHECK(application->m_pFramework->m_vBufferUploadHeap == nullptr);
    CHECK(application->m_pFramework->m_pSDLRenderer == nullptr);
    CHECK(application->m_pFramework->m_pSDLWindow == nullptr);
    fake_delete_device_objects_calls = 0;
    fake_final_cleanup_calls = 0;
    application->Cleanup3DEnvironment();
    CHECK(*reinterpret_cast<std::uint64_t *>(storage.data() + 0x20) == 0);
    CHECK(application->m_pFramework == nullptr);
    CHECK(fake_delete_device_objects_calls == 1);
    CHECK(fake_final_cleanup_calls == 1);
#ifdef JPB_D3DAPP_REAL_ASSET_DIR
    CHECK(SetDllDirectoryA(nullptr) != FALSE);
#endif

    *reinterpret_cast<int *>(storage.data() + 0x20) = 1;
    *reinterpret_cast<int *>(storage.data() + 0x24) = 1;
    application->Cleanup3DEnvironment();
    CHECK(*reinterpret_cast<std::uint64_t *>(storage.data() + 0x20) == 0);
    CHECK(fake_delete_device_objects_calls == 1);
    CHECK(fake_final_cleanup_calls == 1);

    HWND window = CreateWindowExA(
        0, "STATIC", "d3dapp-shutdown-test", WS_POPUP,
        0, 0, 32, 32, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(window != nullptr);
    *reinterpret_cast<HWND *>(storage.data() + 0xD0) = window;
    *reinterpret_cast<int *>(storage.data() + 0x20) = 1;
    *reinterpret_cast<int *>(storage.data() + 0x24) = 1;
    application->CD3DApplication::ShutDown();
    CHECK(*reinterpret_cast<std::uint64_t *>(storage.data() + 0x20) == 0);
    CHECK(IsWindow(window) == FALSE);
    return 0;
}

static int create_sdl_init_result;
static int create_img_init_result;
static int create_ttf_init_result;
static void *create_window_result;
static void *create_renderer_result;
static void *create_texture_result;
static HWND create_native_window_result;
static HRESULT create_game_bar_result;
static HRESULT create_framework_result;
static HRESULT create_textures_result;
static unsigned create_sdl_init_calls;
static unsigned create_img_init_calls;
static unsigned create_ttf_init_calls;
static unsigned create_window_calls;
static unsigned create_renderer_calls;
static unsigned create_texture_calls;
static unsigned create_texture_blend_calls;
static unsigned create_render_target_calls;
static unsigned create_draw_color_calls;
static unsigned create_render_clear_calls;
static unsigned create_draw_blend_calls;
static unsigned create_window_handle_calls;
static unsigned create_game_bar_calls;
static unsigned create_framework_calls;
static unsigned create_textures_calls;
static unsigned create_one_time_scene_calls;
static std::uint32_t create_sdl_init_flags;
static std::uint32_t create_window_flags;
static int create_window_width;
static int create_window_height;

static int fake_create_sdl_init(std::uint32_t flags)
{
    ++create_sdl_init_calls;
    create_sdl_init_flags = flags;
    return create_sdl_init_result;
}

static const char *fake_create_sdl_get_error()
{
    return "test error";
}

static int fake_create_img_init(int flags)
{
    ++create_img_init_calls;
    CHECK(flags == 0);
    return create_img_init_result;
}

static int fake_create_ttf_init()
{
    ++create_ttf_init_calls;
    return create_ttf_init_result;
}

static void *fake_create_sdl_window(
    const char *, int x, int y, int width, int height, std::uint32_t flags)
{
    ++create_window_calls;
    if (x != 0x1FFF0000 || y != 0x1FFF0000) {
        std::abort();
    }
    create_window_width = width;
    create_window_height = height;
    create_window_flags = flags;
    return create_window_result;
}

static void *fake_create_sdl_renderer(
    void *window, int index, std::uint32_t flags)
{
    ++create_renderer_calls;
    if (window != create_window_result || index != -1 || flags != 2) {
        std::abort();
    }
    return create_renderer_result;
}

static void *fake_create_sdl_texture(
    void *renderer,
    std::uint32_t format,
    int access,
    int width,
    int height)
{
    ++create_texture_calls;
    if (renderer != create_renderer_result || format != 0x16762004 ||
        access != 2 || width != 1920 || height != 1080) {
        std::abort();
    }
    return create_texture_result;
}

static int fake_create_set_texture_blend_mode(void *texture, int mode)
{
    ++create_texture_blend_calls;
    CHECK(texture == create_texture_result);
    CHECK(mode == 1);
    return 0;
}

static int fake_create_set_render_target(void *renderer, void *texture)
{
    ++create_render_target_calls;
    CHECK(renderer == create_renderer_result);
    CHECK(texture == create_texture_result);
    return 0;
}

static int fake_create_set_draw_color(
    void *renderer,
    std::uint8_t,
    std::uint8_t,
    std::uint8_t,
    std::uint8_t)
{
    ++create_draw_color_calls;
    CHECK(renderer == create_renderer_result);
    return 0;
}

static int fake_create_render_clear(void *renderer)
{
    ++create_render_clear_calls;
    CHECK(renderer == create_renderer_result);
    return 0;
}

static int fake_create_set_draw_blend_mode(void *renderer, int mode)
{
    ++create_draw_blend_calls;
    CHECK(renderer == create_renderer_result);
    CHECK(mode == 1);
    return 0;
}

static int fake_create_get_window_handle(void *window, HWND *native_window)
{
    ++create_window_handle_calls;
    CHECK(window == create_window_result);
    *native_window = create_native_window_result;
    return 1;
}

static HRESULT fake_create_initialize_game_bar(CD3DApplication *)
{
    ++create_game_bar_calls;
    return create_game_bar_result;
}

static HRESULT fake_create_initialize_framework(
    CD3DApplication *, HWND window)
{
    ++create_framework_calls;
    CHECK(window == create_native_window_result);
    return create_framework_result;
}

static HRESULT fake_create_initialize_textures(CD3DApplication *)
{
    ++create_textures_calls;
    return create_textures_result;
}

static HRESULT fake_create_one_time_scene(CD3DApplication *)
{
    ++create_one_time_scene_calls;
    return S_OK;
}

static void reset_create_test_state()
{
    create_sdl_init_result = 0;
    create_img_init_result = 0;
    create_ttf_init_result = 0;
    create_window_result = reinterpret_cast<void *>(0x11110000);
    create_renderer_result = reinterpret_cast<void *>(0x22220000);
    create_texture_result = reinterpret_cast<void *>(0x33330000);
    create_native_window_result = reinterpret_cast<HWND>(0x44440000);
    create_game_bar_result = S_OK;
    create_framework_result = S_OK;
    create_textures_result = S_OK;
    create_sdl_init_calls = 0;
    create_img_init_calls = 0;
    create_ttf_init_calls = 0;
    create_window_calls = 0;
    create_renderer_calls = 0;
    create_texture_calls = 0;
    create_texture_blend_calls = 0;
    create_render_target_calls = 0;
    create_draw_color_calls = 0;
    create_render_clear_calls = 0;
    create_draw_blend_calls = 0;
    create_window_handle_calls = 0;
    create_game_bar_calls = 0;
    create_framework_calls = 0;
    create_textures_calls = 0;
    create_one_time_scene_calls = 0;
    fake_sdl_destroy_window_calls = 0;
    fake_sdl_destroyed_window = nullptr;
    fake_sdl_quit_calls = 0;
}

static int test_application_create()
{
    const JPBD3DAppCreateTestHooks hooks = {
        fake_create_sdl_init,
        fake_create_sdl_get_error,
        fake_create_img_init,
        fake_create_ttf_init,
        fake_create_sdl_window,
        fake_create_sdl_renderer,
        fake_create_sdl_texture,
        fake_create_set_texture_blend_mode,
        fake_create_set_render_target,
        fake_create_set_draw_color,
        fake_create_render_clear,
        fake_create_set_draw_blend_mode,
        fake_create_get_window_handle,
        fake_create_initialize_game_bar,
        fake_create_initialize_framework,
        fake_create_initialize_textures};
    void *application_vtable[16] = {};
    application_vtable[0] =
        reinterpret_cast<void *>(&fake_create_one_time_scene);
    std::vector<unsigned char> storage(sizeof(CD3DApplication), 0);
    auto *application = reinterpret_cast<CD3DApplication *>(storage.data());
    *reinterpret_cast<void ***>(application) = application_vtable;
    std::strcpy(application->m_strAppName, "JPB create test");
    application->m_strWindowTitle = application->m_strAppName;
    application->m_dwDefaultWidth = 1920;
    application->m_dwDefaultHeight = 1080;
    application->m_dwWindowFlags = 0x80;

    jpb_d3dapp_set_create_test_hooks(&hooks);
    jpb_d3dapp_set_sdl_destroy_window_test_hook(
        fake_sdl_destroy_window);
    jpb_d3dapp_set_sdl_quit_test_hook(fake_sdl_quit);

    reset_create_test_state();
    create_sdl_init_result = 1;
    CHECK(application->CD3DApplication::Create(nullptr, nullptr) == E_FAIL);
    CHECK(create_sdl_init_calls == 1);
    CHECK(create_sdl_init_flags == 0x3230);
    CHECK(create_img_init_calls == 0);

    reset_create_test_state();
    create_ttf_init_result = 1;
    CHECK(application->CD3DApplication::Create(nullptr, nullptr) == E_FAIL);
    CHECK(create_img_init_calls == 1);
    CHECK(create_ttf_init_calls == 1);
    CHECK(create_window_calls == 0);

    reset_create_test_state();
    create_renderer_result = nullptr;
    CHECK(application->CD3DApplication::Create(nullptr, nullptr) == E_FAIL);
    CHECK(create_window_calls == 1);
    CHECK(create_renderer_calls == 1);
    CHECK(fake_sdl_destroy_window_calls == 1);
    CHECK(fake_sdl_destroyed_window == create_window_result);
    CHECK(fake_sdl_quit_calls == 1);
    ::operator delete(application->m_pFramework);
    application->m_pFramework = nullptr;

    reset_create_test_state();
    CHECK(application->CD3DApplication::Create(nullptr, nullptr) == S_OK);
    CHECK(create_window_width == 1920);
    CHECK(create_window_height == 1080);
    CHECK(create_window_flags == 0x84);
    CHECK(create_texture_calls == 1);
    CHECK(create_texture_blend_calls == 1);
    CHECK(create_render_target_calls == 1);
    CHECK(create_draw_color_calls == 2);
    CHECK(create_render_clear_calls == 1);
    CHECK(create_draw_blend_calls == 1);
    CHECK(create_window_handle_calls == 1);
    CHECK(application->m_hWnd == create_native_window_result);
    CHECK(application->m_pFramework->m_hWnd == create_native_window_result);
    CHECK(create_game_bar_calls == 1);
    CHECK(create_framework_calls == 1);
    CHECK(create_textures_calls == 1);
    CHECK(application->m_bReady == 1);
    CHECK(create_one_time_scene_calls == 1);
    ::operator delete(application->m_pFramework);
    application->m_pFramework = nullptr;

    jpb_d3dapp_set_sdl_quit_test_hook(nullptr);
    jpb_d3dapp_set_sdl_destroy_window_test_hook(nullptr);
    jpb_d3dapp_set_create_test_hooks(nullptr);
    return 0;
}

static int test_application_constructor_and_destructor()
{
    auto *storage = ::operator new(sizeof(CD3DApplication));
    auto *application = new (storage) CD3DApplication;

    CHECK(g_pD3DApp == application);
    CHECK(application->m_texture == nullptr);
    CHECK(application->textureUploadHeap == nullptr);
    CHECK(application->m_bActive == 0);
    CHECK(application->m_bReady == 0);
    CHECK(application->m_bFrameMoving == 1);
    CHECK(application->m_bSingleStep == 0);
    CHECK(application->m_spriteBatch == nullptr);
    CHECK(application->m_states == nullptr);
    CHECK(application->m_spriteDraws.empty());
    CHECK(application->m_pVidTex == nullptr);
    CHECK(application->m_pFramework == nullptr);
    CHECK(application->m_LoadedTextures.empty());
    CHECK(application->m_textureCacheMap.empty());
    CHECK(application->m_hWnd == nullptr);
    CHECK(application->m_pWindow == nullptr);
    CHECK(application->m_bAppUseFullScreen == 1);
    CHECK(application->m_bOldWindowedState == 1);
    CHECK(application->m_transparencyPass == nullptr);
    CHECK(application->m_fontAtlas == nullptr);
    CHECK(application->endPolyCallsPerFrame == 0);
    CHECK(application->isFedMovie == 0);
    CHECK(application->m_strWindowTitle == application->m_strAppName);
    CHECK(application->m_dwWindowFlags == 0);
    CHECK(application->m_dwDefaultWidth == 1920);
    CHECK(application->m_dwDefaultHeight == 1080);
    CHECK(application->m_dwDefaultBitsPixel == 16);
    CHECK(application->fbxLevelData.empty());
    CHECK(application->transparentMeshes.empty());
    CHECK(application->transparentGlassMeshes.empty());
    CHECK(std::all_of(
        reinterpret_cast<const unsigned char *>(application->triArray),
        reinterpret_cast<const unsigned char *>(application->triArray) +
            sizeof(application->triArray),
        [](unsigned char value) { return value == 0; }));
    CHECK(std::all_of(
        reinterpret_cast<const unsigned char *>(application->quadArray),
        reinterpret_cast<const unsigned char *>(application->quadArray) +
            sizeof(application->quadArray),
        [](unsigned char value) { return value == 0; }));

    HWND window = CreateWindowExA(
        0,
        "STATIC",
        "d3dapp-destructor-test",
        WS_POPUP,
        0,
        0,
        32,
        32,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);
    CHECK(window != nullptr);
    application->m_hWnd = window;
    application_wait_for_gpu_calls = 0;
    jpb_d3dapp_set_wait_for_gpu_test_hook(
        wait_for_application_gpu_for_test);
    application->~CD3DApplication();
    CHECK(application_wait_for_gpu_calls == 1);
    CHECK(IsWindow(window) == FALSE);
    CHECK(application->courier == nullptr);
    jpb_d3dapp_set_wait_for_gpu_test_hook(nullptr);

    g_pD3DApp = nullptr;
    ::operator delete(storage);
    return 0;
}

static int test_window_state_and_message_pump()
{
    std::vector<unsigned char> storage(0x310974, 0);
    auto *application = reinterpret_cast<CD3DApplication *>(storage.data());
    CHECK(application->IsWindowed() == false);

    HWND window = CreateWindowExA(
        0, "STATIC", "d3dapp-message-pump-test", WS_POPUP,
        0, 0, 32, 32, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(window != nullptr);
    *reinterpret_cast<HWND *>(storage.data() + 0xD0) = window;
    *reinterpret_cast<int *>(storage.data() + 0x310970) = 0x12345678;
    CHECK(PostMessageA(window, WM_NULL, 0, 0) != FALSE);
    application->MessagePump();
    CHECK(*reinterpret_cast<int *>(storage.data() + 0x310970) == 0);
    CHECK(DestroyWindow(window) != FALSE);

#ifdef JPB_D3DAPP_REAL_ASSET_DIR
    CHECK(SetDllDirectoryA(JPB_D3DAPP_REAL_ASSET_DIR) != FALSE);
    HMODULE sdl = LoadLibraryA("SDL2.dll");
    CHECK(sdl != nullptr);
    using InitFunction = int (*)(unsigned int);
    using CreateWindowFunction = void *(*)(
        const char *, int, int, int, int, unsigned int);
    using SetFullscreenFunction = int (*)(void *, unsigned int);
    using DestroyWindowFunction = void (*)(void *);
    using QuitFunction = void (*)();
    auto init = reinterpret_cast<InitFunction>(
        GetProcAddress(sdl, "SDL_Init"));
    auto create_window = reinterpret_cast<CreateWindowFunction>(
        GetProcAddress(sdl, "SDL_CreateWindow"));
    auto set_fullscreen = reinterpret_cast<SetFullscreenFunction>(
        GetProcAddress(sdl, "SDL_SetWindowFullscreen"));
    auto destroy_window = reinterpret_cast<DestroyWindowFunction>(
        GetProcAddress(sdl, "SDL_DestroyWindow"));
    auto quit = reinterpret_cast<QuitFunction>(
        GetProcAddress(sdl, "SDL_Quit"));
    CHECK(init != nullptr);
    CHECK(create_window != nullptr);
    CHECK(set_fullscreen != nullptr);
    CHECK(destroy_window != nullptr);
    CHECK(quit != nullptr);
    CHECK(init(0x20) == 0);
    void *sdl_window = create_window(
        "d3dapp-window-state-test", 0x1FFF0000, 0x1FFF0000,
        64, 48, 0x8);
    CHECK(sdl_window != nullptr);
    *reinterpret_cast<void **>(storage.data() + 0xD8) = sdl_window;
    CHECK(application->IsWindowed() == true);
    CHECK(set_fullscreen(sdl_window, 0x1001) == 0);
    CHECK(application->IsWindowed() == false);
    destroy_window(sdl_window);
    quit();
    CHECK(FreeLibrary(sdl) != FALSE);
    CHECK(SetDllDirectoryA(nullptr) != FALSE);
#endif
    return 0;
}

struct SpriteRootSignatureCapture {
    UINT num_parameters;
    UINT num_static_samplers;
    D3D12_ROOT_SIGNATURE_FLAGS flags;
    D3D12_ROOT_PARAMETER parameters[3];
    D3D12_DESCRIPTOR_RANGE ranges[3];
    D3D12_STATIC_SAMPLER_DESC static_sampler;
};

static SpriteRootSignatureCapture sprite_root_captures[2];
static unsigned int sprite_root_capture_calls;
static bool sprite_root_capture_invalid;

static void capture_sprite_root_signature(
    std::size_t signature_index,
    const D3D12_ROOT_SIGNATURE_DESC &description)
{
    if (signature_index >= _countof(sprite_root_captures)) {
        sprite_root_capture_invalid = true;
        return;
    }
    SpriteRootSignatureCapture &capture =
        sprite_root_captures[signature_index];
    capture = {};
    capture.num_parameters = description.NumParameters;
    capture.num_static_samplers = description.NumStaticSamplers;
    capture.flags = description.Flags;
    if (description.NumParameters > _countof(capture.parameters)) {
        sprite_root_capture_invalid = true;
        return;
    }
    for (UINT index = 0; index < description.NumParameters; ++index) {
        capture.parameters[index] = description.pParameters[index];
        if (description.pParameters[index].ParameterType ==
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            if (description.pParameters[index]
                    .DescriptorTable.NumDescriptorRanges != 1) {
                sprite_root_capture_invalid = true;
                return;
            }
            capture.ranges[index] = *description.pParameters[index]
                .DescriptorTable.pDescriptorRanges;
        }
    }
    if (description.NumStaticSamplers != 0) {
        if (description.NumStaticSamplers != 1) {
            sprite_root_capture_invalid = true;
            return;
        }
        capture.static_sampler = description.pStaticSamplers[0];
    }
    ++sprite_root_capture_calls;
}

static int test_resource_upload_batch_lifecycle()
{
    void *device_vtable[37] = {};
    device_vtable[1] = reinterpret_cast<void *>(&fake_device_add_ref);
    device_vtable[2] = reinterpret_cast<void *>(&fake_device_release);
    device_vtable[9] = reinterpret_cast<void *>(
        &fake_create_command_allocator);
    device_vtable[12] = reinterpret_cast<void *>(
        &fake_create_command_list);
    device_vtable[13] = reinterpret_cast<void *>(
        &fake_check_feature_support);
    device_vtable[36] = reinterpret_cast<void *>(&fake_create_fence);

    void *allocator_vtable[3] = {};
    allocator_vtable[2] = reinterpret_cast<void *>(
        &fake_render_allocator_release);
    void *command_list_vtable[10] = {};
    command_list_vtable[1] = reinterpret_cast<void *>(
        &fake_render_command_list_add_ref);
    command_list_vtable[2] = reinterpret_cast<void *>(
        &fake_render_command_list_release);
    command_list_vtable[9] = reinterpret_cast<void *>(
        &fake_render_command_list_close);
    void *queue_vtable[15] = {};
    queue_vtable[10] = reinterpret_cast<void *>(
        &fake_queue_execute_command_lists);
    queue_vtable[14] = reinterpret_cast<void *>(&fake_queue_signal);
    void *fence_vtable[10] = {};
    fence_vtable[1] = reinterpret_cast<void *>(&fake_fence_add_ref);
    fence_vtable[2] = reinterpret_cast<void *>(&fake_fence_release);
    fence_vtable[9] = reinterpret_cast<void *>(&fake_fence_set_event);

    FakeRenderCommandAllocator allocator = {};
    allocator.vtable = allocator_vtable;
    FakeRenderCommandList command_list = {};
    command_list.vtable = command_list_vtable;
    command_list.close_result = S_OK;
    FakeCommandQueue queue = {};
    queue.vtable = queue_vtable;
    queue.result = S_OK;
    FakeFence fence = {};
    fence.vtable = fence_vtable;
    fence.result = S_OK;
    FakeDevice device = {};
    device.vtable = device_vtable;
    device.feature_support_result = S_OK;
    device.typed_uav_load_additional_formats = TRUE;
    device.standard_swizzle_64kb_supported = TRUE;
    device.allocator_results[0] =
        reinterpret_cast<ID3D12CommandAllocator *>(&allocator);
    device.allocator_results_hr[0] = S_OK;
    device.command_list_result =
        reinterpret_cast<ID3D12GraphicsCommandList *>(&command_list);
    device.command_list_result_hr = S_OK;
    device.fence_results[0] = reinterpret_cast<ID3D12Fence *>(&fence);
    device.fence_results_hr[0] = S_OK;

    {
        DirectX::ResourceUploadBatch upload(
            reinterpret_cast<ID3D12Device *>(&device));
        CHECK(device.add_ref_calls == 1);
        CHECK(device.feature_support_calls == 1);
        CHECK(device.feature == D3D12_FEATURE_D3D12_OPTIONS);
        CHECK(device.feature_data_size ==
              sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));

        bool invalid_type = false;
        try {
            upload.Begin(D3D12_COMMAND_LIST_TYPE_BUNDLE);
        } catch (const std::invalid_argument &error) {
            invalid_type = std::strcmp(error.what(), "commandType") == 0;
        }
        CHECK(invalid_type);
        CHECK(device.allocator_calls == 0);

        upload.Begin(D3D12_COMMAND_LIST_TYPE_COPY);
        CHECK(device.allocator_calls == 1);
        CHECK(device.allocator_types[0] ==
              D3D12_COMMAND_LIST_TYPE_COPY);
        CHECK(*device.allocator_iids[0] ==
              __uuidof(ID3D12CommandAllocator));
        CHECK(device.command_list_calls == 1);
        CHECK(device.node_mask == 1);
        CHECK(device.command_list_type ==
              D3D12_COMMAND_LIST_TYPE_COPY);
        CHECK(device.command_list_allocator ==
              reinterpret_cast<ID3D12CommandAllocator *>(&allocator));
        CHECK(device.initial_pipeline_state == nullptr);
        CHECK(*device.command_list_iid ==
              __uuidof(ID3D12GraphicsCommandList));

        bool duplicate_begin = false;
        try {
            upload.Begin();
        } catch (const std::logic_error &error) {
            duplicate_begin = std::strcmp(
                error.what(),
                "Can't call Begin on an open ResourceUploadBatch.") == 0;
        }
        CHECK(duplicate_begin);

        std::future<void> completion = upload.End(
            reinterpret_cast<ID3D12CommandQueue *>(&queue));
        completion.get();
        CHECK(command_list.close_calls == 1);
        CHECK(queue.execute_calls == 1);
        CHECK(queue.execute_count == 1);
        CHECK(queue.executed_list ==
              reinterpret_cast<ID3D12CommandList *>(&command_list));
        CHECK(device.fence_calls == 1);
        CHECK(device.fence_initial_values[0] == 0);
        CHECK(device.fence_flags[0] == D3D12_FENCE_FLAG_NONE);
        CHECK(*device.fence_iids[0] == __uuidof(ID3D12Fence));
        CHECK(queue.signal_calls == 1);
        CHECK(queue.fence == reinterpret_cast<ID3D12Fence *>(&fence));
        CHECK(queue.value == 1);
        CHECK(fence.event_calls == 1);
        CHECK(fence.value == 1);
        CHECK(command_list.add_ref_calls == 1);
        CHECK(command_list.release_calls == 2);
        CHECK(fence.add_ref_calls == 1);
        CHECK(fence.release_calls == 2);
        CHECK(allocator.release_calls == 1);

        bool duplicate_end = false;
        try {
            (void)upload.End(
                reinterpret_cast<ID3D12CommandQueue *>(&queue));
        } catch (const std::logic_error &error) {
            duplicate_end = std::strcmp(
                error.what(),
                "Can't call End on a closed ResourceUploadBatch.") == 0;
        }
        CHECK(duplicate_end);
    }
    CHECK(device.release_calls == 1);
    return 0;
}

static int test_directxtk_descriptor_abi()
{
    void *device_vtable[16] = {};
    device_vtable[2] = reinterpret_cast<void *>(&fake_device_release);
    device_vtable[15] =
        reinterpret_cast<void *>(&fake_get_descriptor_increment_size);

    FakeDevice device = {};
    device.vtable = device_vtable;
    device.increment_size_result = 32;

    FakeDescriptorHeap heap;
    heap.device_result = reinterpret_cast<ID3D12Device *>(&device);
    heap.cpu_handle.ptr = 0x12340000;
    heap.gpu_handle.ptr = 0xABC00000;
    heap.description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    heap.description.NumDescriptors = 6;
    heap.description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap.description.NodeMask = 7;

    {
        DirectX::DescriptorHeap descriptors(&heap);
        CHECK(heap.add_ref_calls == 1);
        CHECK(heap.cpu_handle_calls == 1);
        CHECK(heap.gpu_handle_calls == 1);
        CHECK(heap.get_desc_calls == 1);
        CHECK(heap.get_device_calls == 1);
        CHECK(*heap.device_iid == __uuidof(ID3D12Device));
        CHECK(device.increment_size_calls == 1);
        CHECK(device.increment_size_type ==
              D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        CHECK(device.release_calls == 1);
        CHECK(descriptors.Heap() == &heap);
        CHECK(descriptors.Count() == 6);
        CHECK(descriptors.Type() == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        CHECK(descriptors.Flags() ==
              D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        CHECK(descriptors.Increment() == 32);
        CHECK(descriptors.GetFirstCpuHandle().ptr == 0x12340000);
        CHECK(descriptors.GetFirstGpuHandle().ptr == 0xABC00000);
        CHECK(descriptors.GetGpuHandle(5).ptr == 0xABC000A0);

        bool threw = false;
        try {
            (void)descriptors.GetGpuHandle(6);
        } catch (const std::out_of_range &error) {
            threw = std::strcmp(
                error.what(), "D3DX12_GPU_DESCRIPTOR_HANDLE") == 0;
        }
        CHECK(threw);
    }
    CHECK(heap.release_calls == 1);

    alignas(DirectX::DescriptorHeap) unsigned char impl_storage[48] = {};
    auto *impl_descriptors = new (impl_storage)
        DirectX::DescriptorHeap(&heap);
    alignas(DirectX::DX12::CommonStates)
        unsigned char states_storage[16] = {};
    void *impl = impl_storage;
    std::memcpy(states_storage + 8, &impl, sizeof(impl));
    const auto *states = reinterpret_cast<
        const DirectX::DX12::CommonStates *>(states_storage);

    CHECK(states->Heap() == &heap);
    CHECK(states->PointWrap().ptr == 0xABC00000);
    CHECK(states->PointClamp().ptr == 0xABC00020);
    CHECK(states->LinearWrap().ptr == 0xABC00040);
    CHECK(states->LinearClamp().ptr == 0xABC00060);
    CHECK(states->AnisotropicWrap().ptr == 0xABC00080);
    CHECK(states->AnisotropicClamp().ptr == 0xABC000A0);

    impl_descriptors->~DescriptorHeap();
    CHECK(heap.add_ref_calls == 2);
    CHECK(heap.release_calls == 2);
    CHECK(device.increment_size_calls == 2);
    CHECK(device.release_calls == 2);

    void *common_states_device_vtable[23] = {};
    common_states_device_vtable[2] =
        reinterpret_cast<void *>(&fake_device_release);
    common_states_device_vtable[14] =
        reinterpret_cast<void *>(&fake_create_descriptor_heap);
    common_states_device_vtable[15] =
        reinterpret_cast<void *>(&fake_get_descriptor_increment_size);
    common_states_device_vtable[22] =
        reinterpret_cast<void *>(&fake_create_sampler);

    FakeDescriptorHeap sampler_heap;
    sampler_heap.cpu_handle.ptr = 0x50000000;
    sampler_heap.gpu_handle.ptr = 0x60000000;
    device = {};
    device.vtable = common_states_device_vtable;
    device.increment_size_result = 16;
    device.descriptor_heap_result = &sampler_heap;
    device.descriptor_heap_result_hr = S_OK;
    {
        DirectX::DX12::CommonStates common_states(
            reinterpret_cast<ID3D12Device *>(&device));
        CHECK(device.descriptor_heap_calls == 1);
        CHECK(device.descriptor_heap_description.Type ==
              D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        CHECK(device.descriptor_heap_description.NumDescriptors == 6);
        CHECK(device.descriptor_heap_description.Flags ==
              D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        CHECK(device.descriptor_heap_description.NodeMask == 0);
        CHECK(device.increment_size_calls == 1);
        CHECK(sampler_heap.cpu_handle_calls == 1);
        CHECK(sampler_heap.gpu_handle_calls == 1);
        CHECK(device.sampler_calls == 6);
        CHECK(common_states.Heap() == &sampler_heap);
        CHECK(common_states.LinearClamp().ptr == 0x60000030);

        for (unsigned int index = 0; index < 6; ++index) {
            const D3D12_SAMPLER_DESC &description =
                device.sampler_descriptions[index];
            const bool clamp = (index & 1U) != 0;
            CHECK(device.sampler_handles[index].ptr ==
                  0x50000000 + index * 16);
            CHECK(description.AddressU ==
                  (clamp ? D3D12_TEXTURE_ADDRESS_MODE_CLAMP
                         : D3D12_TEXTURE_ADDRESS_MODE_WRAP));
            CHECK(description.AddressV == description.AddressU);
            CHECK(description.AddressW == description.AddressU);
            CHECK(description.Filter ==
                  (index < 2 ? D3D12_FILTER_MIN_MAG_MIP_POINT
                   : index < 4 ? D3D12_FILTER_MIN_MAG_MIP_LINEAR
                               : D3D12_FILTER_ANISOTROPIC));
            CHECK(description.MipLODBias == 0.0f);
            CHECK(description.MaxAnisotropy == 16);
            CHECK(description.ComparisonFunc ==
                  D3D12_COMPARISON_FUNC_NEVER);
            CHECK(description.BorderColor[0] == 0.0f);
            CHECK(description.BorderColor[1] == 0.0f);
            CHECK(description.BorderColor[2] == 0.0f);
            CHECK(description.BorderColor[3] == 0.0f);
            CHECK(description.MinLOD == 0.0f);
            CHECK(description.MaxLOD == FLT_MAX);
        }

        DirectX::DX12::CommonStates moved(std::move(common_states));
        CHECK(moved.AnisotropicClamp().ptr == 0x60000050);
    }
    CHECK(sampler_heap.release_calls == 1);

    bool too_many_threw = false;
    try {
        DirectX::DescriptorHeap too_many(
            reinterpret_cast<ID3D12Device *>(&device),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            static_cast<std::size_t>(UINT_MAX) + 1);
    } catch (const std::invalid_argument &error) {
        too_many_threw = std::strcmp(
            error.what(), "Too many descriptors") == 0;
    }
    CHECK(too_many_threw);

    DirectX::LinearAllocatorPage page;
    CHECK(page.RefCount() == 1);
    CHECK(page.pPrevPage == nullptr);
    CHECK(page.pNextPage == nullptr);
    CHECK(page.mOffset == 0);
    CHECK(page.mSize == 0);
    page.mSize = 128;
    CHECK(page.Suballocate(17, 0) == 0);
    CHECK(page.mOffset == 17);
    CHECK(page.Suballocate(16, 16) == 32);
    CHECK(page.mOffset == 48);
    CHECK(page.Suballocate(32, 64) == 64);
    CHECK(page.mOffset == 96);
    bool suballocate_threw = false;
    try {
        (void)page.Suballocate(33, 1);
    } catch (const std::runtime_error &error) {
        suballocate_threw = std::strcmp(
            error.what(), "LinearAllocatorPage::Suballocate") == 0;
    }
    CHECK(suballocate_threw);
    CHECK(page.mOffset == 96);

    page.mRefCount = 2;
    auto *resource = reinterpret_cast<ID3D12Resource *>(
        static_cast<std::uintptr_t>(0x1111222233334444));
    void *memory = reinterpret_cast<void *>(
        static_cast<std::uintptr_t>(0x5555666677778888));
    {
        DirectX::DX12::GraphicsResource allocation(
            &page, 0x123456780000, resource, memory, 0x240, 0x800);
        CHECK(page.RefCount() == 3);
        CHECK(static_cast<bool>(allocation));
        CHECK(allocation.GpuAddress() == 0x123456780000);
        CHECK(allocation.Resource() == resource);
        CHECK(allocation.Memory() == memory);
        CHECK(allocation.ResourceOffset() == 0x240);
        CHECK(allocation.Size() == 0x800);

        DirectX::DX12::GraphicsResource moved(std::move(allocation));
        CHECK(!static_cast<bool>(allocation));
        CHECK(allocation.GpuAddress() == 0);
        CHECK(allocation.Resource() == nullptr);
        CHECK(allocation.Memory() == nullptr);
        CHECK(allocation.ResourceOffset() == 0);
        CHECK(allocation.Size() == 0);
        CHECK(moved.GpuAddress() == 0x123456780000);
        CHECK(page.RefCount() == 3);

        DirectX::DX12::GraphicsResource assigned;
        assigned = std::move(moved);
        CHECK(!static_cast<bool>(moved));
        CHECK(assigned.GpuAddress() == 0x123456780000);
        CHECK(page.RefCount() == 3);
        assigned.Reset();
        CHECK(!static_cast<bool>(assigned));
        CHECK(assigned.GpuAddress() == 0);
        CHECK(assigned.Resource() == nullptr);
        CHECK(assigned.Memory() == nullptr);
        CHECK(assigned.ResourceOffset() == 0);
        CHECK(assigned.Size() == 0);
        CHECK(page.RefCount() == 2);
    }
    CHECK(page.RefCount() == 2);
    page.AddRef();
    CHECK(page.RefCount() == 3);
    page.Release();
    CHECK(page.RefCount() == 2);

    bool missing_graphics_memory_threw = false;
    try {
        (void)DirectX::DX12::GraphicsMemory::Get();
    } catch (const std::logic_error &error) {
        missing_graphics_memory_threw = std::strcmp(
            error.what(), "GraphicsMemory singleton not created") == 0;
    }
    CHECK(missing_graphics_memory_threw);

    void *upload_command_list_vtable[27] = {};
    upload_command_list_vtable[26] = reinterpret_cast<void *>(
        &fake_resource_barrier);
    FakeCommandList upload_command_list = {};
    upload_command_list.vtable = upload_command_list_vtable;
    auto *upload_resource = reinterpret_cast<ID3D12Resource *>(
        static_cast<std::uintptr_t>(0x12340000));

    bool closed_upload_threw = false;
    try {
        DirectX::DX12::jpb_directxtk12_test_resource_upload_transition(
            false,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            reinterpret_cast<ID3D12GraphicsCommandList *>(
                &upload_command_list),
            upload_resource,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
    } catch (const std::logic_error &error) {
        closed_upload_threw = std::strcmp(
            error.what(),
            "Can't call Upload on a closed ResourceUploadBatch.") == 0;
    }
    CHECK(closed_upload_threw);
    CHECK(upload_command_list.calls == 0);

    DirectX::DX12::jpb_directxtk12_test_resource_upload_transition(
        true,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        reinterpret_cast<ID3D12GraphicsCommandList *>(
            &upload_command_list),
        upload_resource,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    CHECK(upload_command_list.calls == 1);
    CHECK(upload_command_list.barrier_count == 1);
    CHECK(upload_command_list.barrier.Type ==
          D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);
    CHECK(upload_command_list.barrier.Transition.pResource ==
          upload_resource);
    CHECK(upload_command_list.barrier.Transition.Subresource ==
          D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    CHECK(upload_command_list.barrier.Transition.StateBefore ==
          D3D12_RESOURCE_STATE_COMMON);
    CHECK(upload_command_list.barrier.Transition.StateAfter ==
          D3D12_RESOURCE_STATE_RENDER_TARGET);

    upload_command_list.calls = 0;
    DirectX::DX12::jpb_directxtk12_test_resource_upload_transition(
        true,
        D3D12_COMMAND_LIST_TYPE_COPY,
        reinterpret_cast<ID3D12GraphicsCommandList *>(
            &upload_command_list),
        upload_resource,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    CHECK(upload_command_list.calls == 0);
    DirectX::DX12::jpb_directxtk12_test_resource_upload_transition(
        true,
        D3D12_COMMAND_LIST_TYPE_COPY,
        reinterpret_cast<ID3D12GraphicsCommandList *>(
            &upload_command_list),
        upload_resource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    CHECK(upload_command_list.calls == 1);

    upload_command_list.calls = 0;
    DirectX::DX12::jpb_directxtk12_test_resource_upload_transition(
        true,
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        reinterpret_cast<ID3D12GraphicsCommandList *>(
            &upload_command_list),
        upload_resource,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    CHECK(upload_command_list.calls == 0);
    DirectX::DX12::jpb_directxtk12_test_resource_upload_transition(
        true,
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        reinterpret_cast<ID3D12GraphicsCommandList *>(
            &upload_command_list),
        upload_resource,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    CHECK(upload_command_list.calls == 1);

    upload_command_list.calls = 0;
    DirectX::DX12::jpb_directxtk12_test_resource_upload_transition(
        true,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        reinterpret_cast<ID3D12GraphicsCommandList *>(
            &upload_command_list),
        upload_resource,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COMMON);
    CHECK(upload_command_list.calls == 0);

    std::uint64_t graphics_page_size = 0;
    CHECK(DirectX::DX12::jpb_directxtk12_test_graphics_memory_pool(
              1, 0, &graphics_page_size) == 0);
    CHECK(graphics_page_size == 0x10000);
    CHECK(DirectX::DX12::jpb_directxtk12_test_graphics_memory_pool(
              4096, 0, &graphics_page_size) == 1);
    CHECK(graphics_page_size == 0x10000);
    CHECK(DirectX::DX12::jpb_directxtk12_test_graphics_memory_pool(
              4096, 16, &graphics_page_size) == 2);
    CHECK(graphics_page_size == 0x10000);
    CHECK(DirectX::DX12::jpb_directxtk12_test_graphics_memory_pool(
              65536, 16, &graphics_page_size) == 6);
    CHECK(graphics_page_size == 0x20000);

    alignas(16) unsigned char sprite_impl[320] = {};
    alignas(DirectX::DX12::SpriteBatch)
        unsigned char sprite_batch_storage[16] = {};
    void *sprite_impl_pointer = sprite_impl;
    std::memcpy(
        sprite_batch_storage + 8,
        &sprite_impl_pointer,
        sizeof(sprite_impl_pointer));
    auto *sprite_batch = reinterpret_cast<DirectX::DX12::SpriteBatch *>(
        sprite_batch_storage);

    sprite_batch->SetRotation(DXGI_MODE_ROTATION_ROTATE270);
    CHECK(*reinterpret_cast<DXGI_MODE_ROTATION *>(sprite_impl) ==
          DXGI_MODE_ROTATION_ROTATE270);
    CHECK(sprite_batch->GetRotation() ==
          DXGI_MODE_ROTATION_ROTATE270);

    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 17.5f;
    viewport.TopLeftY = 23.25f;
    viewport.Width = 1920.0f;
    viewport.Height = 1080.0f;
    viewport.MinDepth = 0.125f;
    viewport.MaxDepth = 0.875f;
    sprite_batch->SetViewport(viewport);
    CHECK(sprite_impl[4] == 1);
    CHECK(std::memcmp(sprite_impl + 8, &viewport, sizeof(viewport)) == 0);
    CHECK(*reinterpret_cast<std::uint64_t *>(sprite_impl + 32) == 0);

    bool end_outside_pair_threw = false;
    try {
        sprite_batch->End();
    } catch (const std::logic_error &error) {
        end_outside_pair_threw =
            std::strcmp(error.what(), "SpriteBatch::End") == 0;
    }
    CHECK(end_outside_pair_threw);

    const DirectX::XMMATRIX begin_transform = DirectX::XMMatrixSet(
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f);
    bool null_sampler_threw = false;
    try {
        sprite_batch->Begin(
            nullptr,
            D3D12_GPU_DESCRIPTOR_HANDLE {},
            DirectX::DX12::SpriteSortMode_Deferred,
            begin_transform);
    } catch (const std::invalid_argument &error) {
        null_sampler_threw = std::strcmp(
            error.what(),
            "Invalid heap-based sampler for Begin") == 0;
    }
    CHECK(null_sampler_threw);

    bool missing_root_signature_threw = false;
    try {
        sprite_batch->Begin(
            nullptr,
            D3D12_GPU_DESCRIPTOR_HANDLE {0x1234},
            DirectX::DX12::SpriteSortMode_Deferred,
            begin_transform);
    } catch (const std::runtime_error &error) {
        missing_root_signature_threw =
            std::strcmp(error.what(), "SpriteBatch::Begin") == 0;
    }
    CHECK(missing_root_signature_threw);

    sprite_batch->Begin(
        nullptr,
        DirectX::DX12::SpriteSortMode_BackToFront,
        begin_transform);
    CHECK(sprite_impl[88] == 1);
    CHECK(*reinterpret_cast<DirectX::DX12::SpriteSortMode *>(
              sprite_impl + 92) ==
          DirectX::DX12::SpriteSortMode_BackToFront);
    CHECK(std::memcmp(
              sprite_impl + 112,
              &begin_transform,
              sizeof(begin_transform)) == 0);
    CHECK(*reinterpret_cast<void **>(sprite_impl + 176) == nullptr);
    CHECK(*reinterpret_cast<std::size_t *>(sprite_impl + 240) == 0);

    bool repeated_begin_threw = false;
    try {
        sprite_batch->Begin(
            nullptr,
            DirectX::DX12::SpriteSortMode_Deferred,
            DirectX::XMMatrixIdentity());
    } catch (const std::logic_error &error) {
        repeated_begin_threw =
            std::strcmp(error.what(), "SpriteBatch::Begin") == 0;
    }
    CHECK(repeated_begin_threw);

    DirectX::XMFLOAT4X4 viewport_transform = {};
    bool missing_viewport_threw = false;
    try {
        DirectX::DX12::jpb_directxtk12_test_viewport_transform(
            false,
            viewport,
            DXGI_MODE_ROTATION_IDENTITY,
            &viewport_transform);
    } catch (const std::runtime_error &error) {
        missing_viewport_threw =
            std::strcmp(error.what(), "Viewport not set.") == 0;
    }
    CHECK(missing_viewport_threw);

    const float x_scale = 2.0f / viewport.Width;
    const float y_scale = 2.0f / viewport.Height;
    DirectX::DX12::jpb_directxtk12_test_viewport_transform(
        true,
        viewport,
        DXGI_MODE_ROTATION_IDENTITY,
        &viewport_transform);
    CHECK(near_float(viewport_transform._11, x_scale));
    CHECK(near_float(viewport_transform._22, -y_scale));
    CHECK(viewport_transform._33 == 1.0f);
    CHECK(viewport_transform._41 == -1.0f);
    CHECK(viewport_transform._42 == 1.0f);
    CHECK(viewport_transform._44 == 1.0f);

    DirectX::DX12::jpb_directxtk12_test_viewport_transform(
        true,
        viewport,
        DXGI_MODE_ROTATION_ROTATE90,
        &viewport_transform);
    CHECK(near_float(viewport_transform._12, -y_scale));
    CHECK(near_float(viewport_transform._21, -x_scale));
    CHECK(viewport_transform._41 == 1.0f);
    CHECK(viewport_transform._42 == 1.0f);

    DirectX::DX12::jpb_directxtk12_test_viewport_transform(
        true,
        viewport,
        DXGI_MODE_ROTATION_ROTATE180,
        &viewport_transform);
    CHECK(near_float(viewport_transform._11, -x_scale));
    CHECK(near_float(viewport_transform._22, y_scale));
    CHECK(viewport_transform._41 == 1.0f);
    CHECK(viewport_transform._42 == -1.0f);

    DirectX::DX12::jpb_directxtk12_test_viewport_transform(
        true,
        viewport,
        DXGI_MODE_ROTATION_ROTATE270,
        &viewport_transform);
    CHECK(near_float(viewport_transform._12, y_scale));
    CHECK(near_float(viewport_transform._21, x_scale));
    CHECK(viewport_transform._41 == -1.0f);
    CHECK(viewport_transform._42 == -1.0f);

    DirectX::DX12::JPBSpriteBatchDrawRecord public_draws[8] = {};
    DirectX::DX12::jpb_directxtk12_test_public_draw_overloads(
        public_draws);
    const float expected_public_sizes[8][2] = {
        {30.0f, 20.0f},
        {50.0f, 25.0f},
        {1.0f, 1.0f},
        {30.0f, 40.0f},
        {30.0f, 40.0f},
        {1.0f, 1.0f},
        {30.0f, 20.0f},
        {50.0f, 25.0f},
    };
    const std::uint32_t expected_public_flags[8] = {
        0x0Eu, 0x0Eu, 0u, 0x0Eu, 8u, 0u, 0x0Eu, 0x0Eu,
    };
    for (std::size_t index = 0; index < 8; ++index) {
        CHECK(public_draws[index].texture == 0x1234);
        CHECK(public_draws[index].texture_size.x == 100.0f);
        CHECK(public_draws[index].texture_size.y == 50.0f);
        CHECK(public_draws[index].destination.x == 10.0f);
        CHECK(public_draws[index].destination.y == 20.0f);
        CHECK(near_float(
            public_draws[index].destination.z,
            expected_public_sizes[index][0]));
        CHECK(near_float(
            public_draws[index].destination.w,
            expected_public_sizes[index][1]));
        CHECK(public_draws[index].flags ==
              expected_public_flags[index]);
        CHECK(public_draws[index].color.x == 0.1f);
        CHECK(public_draws[index].color.y == 0.2f);
        CHECK(public_draws[index].color.z == 0.3f);
        CHECK(public_draws[index].color.w == 0.4f);
    }
    for (std::size_t index : {0u, 1u, 3u, 6u, 7u}) {
        CHECK(public_draws[index].source.x == 2.0f);
        CHECK(public_draws[index].source.y == 3.0f);
        CHECK(public_draws[index].source.z == 10.0f);
        CHECK(public_draws[index].source.w == 5.0f);
        CHECK(public_draws[index].origin_rotation_depth.x == 1.0f);
        CHECK(public_draws[index].origin_rotation_depth.y == 2.0f);
        CHECK(public_draws[index].origin_rotation_depth.z == 0.25f);
        CHECK(public_draws[index].origin_rotation_depth.w == 0.75f);
    }
    for (std::size_t index : {2u, 4u, 5u}) {
        CHECK(public_draws[index].source.x == 0.0f);
        CHECK(public_draws[index].source.y == 0.0f);
        CHECK(public_draws[index].source.z == 1.0f);
        CHECK(public_draws[index].source.w == 1.0f);
        CHECK(public_draws[index].origin_rotation_depth.x == 0.0f);
        CHECK(public_draws[index].origin_rotation_depth.y == 0.0f);
        CHECK(public_draws[index].origin_rotation_depth.z == 0.0f);
        CHECK(public_draws[index].origin_rotation_depth.w == 0.0f);
    }

    const DirectX::DX12::JPBSpriteBatchSortRecord sort_records[] = {
        {30, 0.30f},
        {10, 0.10f},
        {20, 0.20f},
    };
    std::size_t sorted_indices[3] = {};
    CHECK(DirectX::DX12::jpb_directxtk12_test_sprite_queue(
              DirectX::DX12::SpriteSortMode_Deferred,
              sort_records, 3, sorted_indices) == 64);
    CHECK(sorted_indices[0] == 0);
    CHECK(sorted_indices[1] == 1);
    CHECK(sorted_indices[2] == 2);

    CHECK(DirectX::DX12::jpb_directxtk12_test_sprite_queue(
              DirectX::DX12::SpriteSortMode_Texture,
              sort_records, 3, sorted_indices) == 64);
    CHECK(sorted_indices[0] == 1);
    CHECK(sorted_indices[1] == 2);
    CHECK(sorted_indices[2] == 0);

    CHECK(DirectX::DX12::jpb_directxtk12_test_sprite_queue(
              DirectX::DX12::SpriteSortMode_BackToFront,
              sort_records, 3, sorted_indices) == 64);
    CHECK(sorted_indices[0] == 0);
    CHECK(sorted_indices[1] == 2);
    CHECK(sorted_indices[2] == 1);

    CHECK(DirectX::DX12::jpb_directxtk12_test_sprite_queue(
              DirectX::DX12::SpriteSortMode_FrontToBack,
              sort_records, 3, sorted_indices) == 64);
    CHECK(sorted_indices[0] == 1);
    CHECK(sorted_indices[1] == 2);
    CHECK(sorted_indices[2] == 0);

    std::vector<DirectX::DX12::JPBSpriteBatchSortRecord>
        growth_records(130);
    std::vector<std::size_t> growth_order(130);
    for (std::size_t index = 0; index < growth_records.size(); ++index) {
        growth_records[index].texture = growth_records.size() - index;
        growth_records[index].layer_depth = static_cast<float>(index);
    }
    CHECK(DirectX::DX12::jpb_directxtk12_test_sprite_queue(
              DirectX::DX12::SpriteSortMode_Texture,
              growth_records.data(), growth_records.size(),
              growth_order.data()) == 256);
    for (std::size_t index = 0; index < growth_order.size(); ++index) {
        CHECK(growth_order[index] == growth_order.size() - index - 1);
    }

    const DirectX::XMUINT2 texture_size = {100, 50};
    const DirectX::XMFLOAT4 destination = {10.0f, 20.0f, 1.0f, 1.0f};
    const DirectX::XMFLOAT4 color = {0.25f, 0.5f, 0.75f, 1.0f};
    const DirectX::XMFLOAT4 origin = {0.0f, 0.0f, 0.0f, 0.625f};
    DirectX::DX12::JPBSpriteBatchDrawRecord draw_record = {};

    bool draw_outside_pair_threw = false;
    try {
        DirectX::DX12::jpb_directxtk12_test_build_sprite_record(
            false, {0x1234}, texture_size, destination, nullptr,
            color, origin, 0, &draw_record);
    } catch (const std::logic_error &error) {
        draw_outside_pair_threw =
            std::strcmp(error.what(), "SpriteBatch::Draw") == 0;
    }
    CHECK(draw_outside_pair_threw);

    bool null_texture_threw = false;
    try {
        DirectX::DX12::jpb_directxtk12_test_build_sprite_record(
            true, {0}, texture_size, destination, nullptr,
            color, origin, 0, &draw_record);
    } catch (const std::invalid_argument &error) {
        null_texture_threw =
            std::strcmp(error.what(), "Invalid texture for Draw") == 0;
    }
    CHECK(null_texture_threw);

    DirectX::DX12::jpb_directxtk12_test_build_sprite_record(
        true, {0x1234}, texture_size, destination, nullptr,
        color, origin, 0, &draw_record);
    CHECK(draw_record.source.x == 0.0f);
    CHECK(draw_record.source.y == 0.0f);
    CHECK(draw_record.source.z == 1.0f);
    CHECK(draw_record.source.w == 1.0f);
    CHECK(draw_record.destination.x == 10.0f);
    CHECK(draw_record.destination.y == 20.0f);
    CHECK(draw_record.destination.z == 1.0f);
    CHECK(draw_record.destination.w == 1.0f);
    CHECK(draw_record.texture == 0x1234);
    CHECK(draw_record.texture_size.x == 100.0f);
    CHECK(draw_record.texture_size.y == 50.0f);
    CHECK(draw_record.texture_size.z == 0.0f);
    CHECK(draw_record.texture_size.w == 0.0f);
    CHECK(draw_record.flags == 0);

    DirectX::DX12::VertexPositionColorTexture vertices[4] = {};
    DirectX::DX12::jpb_directxtk12_test_render_sprite(
        draw_record, vertices);
    const float expected_positions[4][3] = {
        {10.0f, 20.0f, 0.625f},
        {110.0f, 20.0f, 0.625f},
        {10.0f, 70.0f, 0.625f},
        {110.0f, 70.0f, 0.625f},
    };
    const float expected_uvs[4][2] = {
        {0.0f, 0.0f}, {1.0f, 0.0f},
        {0.0f, 1.0f}, {1.0f, 1.0f},
    };
    for (std::size_t index = 0; index < 4; ++index) {
        CHECK(near_float(vertices[index].position.x,
                         expected_positions[index][0]));
        CHECK(near_float(vertices[index].position.y,
                         expected_positions[index][1]));
        CHECK(near_float(vertices[index].position.z,
                         expected_positions[index][2]));
        CHECK(vertices[index].color.x == color.x);
        CHECK(vertices[index].color.y == color.y);
        CHECK(vertices[index].color.z == color.z);
        CHECK(vertices[index].color.w == color.w);
        CHECK(near_float(vertices[index].textureCoordinate.x,
                         expected_uvs[index][0]));
        CHECK(near_float(vertices[index].textureCoordinate.y,
                         expected_uvs[index][1]));
    }

    const RECT source_rectangle = {20, 10, 60, 30};
    const DirectX::XMFLOAT4 scaled_destination = {
        5.0f, 6.0f, 2.0f, 3.0f};
    DirectX::DX12::jpb_directxtk12_test_build_sprite_record(
        true, {0x5678}, texture_size, scaled_destination,
        &source_rectangle, color, origin,
        DirectX::DX12::SpriteEffects_FlipHorizontally,
        &draw_record);
    CHECK(draw_record.source.x == 20.0f);
    CHECK(draw_record.source.y == 10.0f);
    CHECK(draw_record.source.z == 40.0f);
    CHECK(draw_record.source.w == 20.0f);
    CHECK(draw_record.destination.z == 80.0f);
    CHECK(draw_record.destination.w == 60.0f);
    CHECK(draw_record.flags == 0x0Du);

    DirectX::DX12::jpb_directxtk12_test_render_sprite(
        draw_record, vertices);
    CHECK(near_float(vertices[0].position.x, 5.0f));
    CHECK(near_float(vertices[0].position.y, 6.0f));
    CHECK(near_float(vertices[3].position.x, 85.0f));
    CHECK(near_float(vertices[3].position.y, 66.0f));
    CHECK(near_float(vertices[0].textureCoordinate.x, 0.6f));
    CHECK(near_float(vertices[0].textureCoordinate.y, 0.2f));
    CHECK(near_float(vertices[1].textureCoordinate.x, 0.2f));
    CHECK(near_float(vertices[1].textureCoordinate.y, 0.2f));
    CHECK(near_float(vertices[2].textureCoordinate.x, 0.6f));
    CHECK(near_float(vertices[2].textureCoordinate.y, 0.6f));
    CHECK(near_float(vertices[3].textureCoordinate.x, 0.2f));
    CHECK(near_float(vertices[3].textureCoordinate.y, 0.6f));

    std::vector<std::int16_t> sprite_indices(2048 * 6);
    CHECK(DirectX::DX12::jpb_directxtk12_test_sprite_indices(
              sprite_indices.data(), sprite_indices.size()) ==
          sprite_indices.size());
    const std::int16_t first_sprite_indices[6] = {
        0, 1, 2, 1, 3, 2};
    CHECK(std::memcmp(
              sprite_indices.data(),
              first_sprite_indices,
              sizeof(first_sprite_indices)) == 0);
    const std::int16_t last_sprite_indices[6] = {
        8188, 8189, 8190, 8189, 8191, 8190};
    CHECK(std::memcmp(
              sprite_indices.data() + sprite_indices.size() - 6,
              last_sprite_indices,
              sizeof(last_sprite_indices)) == 0);

    const std::size_t expected_shader_sizes[4] = {
        0xB96, 0x999, 0xB96, 0x99D};
    for (std::size_t index = 0; index < 4; ++index) {
        std::size_t shader_size = 0;
        const unsigned char *shader =
            DirectX::DX12::jpb_directxtk12_test_sprite_shader(
                index, &shader_size);
        CHECK(shader != nullptr);
        CHECK(shader_size == expected_shader_sizes[index]);
        CHECK(std::memcmp(shader, "DXBC", 4) == 0);
    }

    void *sprite_root_vtable[3] = {};
    sprite_root_vtable[2] = reinterpret_cast<void *>(
        &fake_unknown_release);
    FakeUnknown sprite_root = {};
    sprite_root.vtable = sprite_root_vtable;
    void *sprite_root_device_vtable[17] = {};
    sprite_root_device_vtable[16] = reinterpret_cast<void *>(
        &fake_create_root_signature);
    FakeDevice sprite_root_device = {};
    sprite_root_device.vtable = sprite_root_device_vtable;
    sprite_root_device.root_signature_result =
        reinterpret_cast<ID3D12RootSignature *>(&sprite_root);
    sprite_root_device.root_signature_result_hr = S_OK;
    std::memset(sprite_root_captures, 0, sizeof(sprite_root_captures));
    sprite_root_capture_calls = 0;
    sprite_root_capture_invalid = false;
    DirectX::DX12::jpb_directxtk12_test_create_sprite_root_signatures(
        reinterpret_cast<ID3D12Device *>(&sprite_root_device),
        &capture_sprite_root_signature);
    CHECK(sprite_root_capture_calls == 2);
    CHECK(!sprite_root_capture_invalid);
    CHECK(sprite_root_device.root_signature_calls == 2);
    CHECK(sprite_root_device.root_signature_node_mask == 0);
    CHECK(*sprite_root_device.root_signature_iid ==
          __uuidof(ID3D12RootSignature));
    CHECK(sprite_root.release_calls == 2);

    const D3D12_ROOT_SIGNATURE_FLAGS sprite_signature_flags =
        static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(0x1D);
    const SpriteRootSignatureCapture &static_signature =
        sprite_root_captures[0];
    CHECK(static_signature.num_parameters == 2);
    CHECK(static_signature.num_static_samplers == 1);
    CHECK(static_signature.flags == sprite_signature_flags);
    CHECK(static_signature.parameters[0].ParameterType ==
          D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
    CHECK(static_signature.parameters[0].ShaderVisibility ==
          D3D12_SHADER_VISIBILITY_PIXEL);
    CHECK(static_signature.ranges[0].RangeType ==
          D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
    CHECK(static_signature.ranges[0].NumDescriptors == 1);
    CHECK(static_signature.ranges[0].BaseShaderRegister == 0);
    CHECK(static_signature.ranges[0].RegisterSpace == 0);
    CHECK(static_signature.ranges[0]
              .OffsetInDescriptorsFromTableStart ==
          D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
    CHECK(static_signature.parameters[1].ParameterType ==
          D3D12_ROOT_PARAMETER_TYPE_CBV);
    CHECK(static_signature.parameters[1].Descriptor.ShaderRegister == 0);
    CHECK(static_signature.parameters[1].Descriptor.RegisterSpace == 0);
    CHECK(static_signature.parameters[1].ShaderVisibility ==
          D3D12_SHADER_VISIBILITY_ALL);
    CHECK(static_signature.static_sampler.Filter ==
          D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    CHECK(static_signature.static_sampler.AddressU ==
          D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    CHECK(static_signature.static_sampler.AddressV ==
          D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    CHECK(static_signature.static_sampler.AddressW ==
          D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    CHECK(static_signature.static_sampler.MipLODBias == 0.0f);
    CHECK(static_signature.static_sampler.MaxAnisotropy == 16);
    CHECK(static_signature.static_sampler.ComparisonFunc ==
          D3D12_COMPARISON_FUNC_LESS_EQUAL);
    CHECK(static_signature.static_sampler.BorderColor ==
          D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);
    CHECK(static_signature.static_sampler.MinLOD == 0.0f);
    CHECK(static_signature.static_sampler.MaxLOD == FLT_MAX);
    CHECK(static_signature.static_sampler.ShaderRegister == 0);
    CHECK(static_signature.static_sampler.RegisterSpace == 0);
    CHECK(static_signature.static_sampler.ShaderVisibility ==
          D3D12_SHADER_VISIBILITY_PIXEL);

    const SpriteRootSignatureCapture &heap_signature =
        sprite_root_captures[1];
    CHECK(heap_signature.num_parameters == 3);
    CHECK(heap_signature.num_static_samplers == 0);
    CHECK(heap_signature.flags == sprite_signature_flags);
    CHECK(heap_signature.parameters[0].ParameterType ==
          D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
    CHECK(heap_signature.ranges[0].RangeType ==
          D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
    CHECK(heap_signature.parameters[1].ParameterType ==
          D3D12_ROOT_PARAMETER_TYPE_CBV);
    CHECK(heap_signature.parameters[2].ParameterType ==
          D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
    CHECK(heap_signature.parameters[2].ShaderVisibility ==
          D3D12_SHADER_VISIBILITY_PIXEL);
    CHECK(heap_signature.ranges[2].RangeType ==
          D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);
    CHECK(heap_signature.ranges[2].NumDescriptors == 1);
    CHECK(heap_signature.ranges[2].BaseShaderRegister == 0);
    CHECK(heap_signature.ranges[2].RegisterSpace == 0);
    CHECK(heap_signature.ranges[2]
              .OffsetInDescriptorsFromTableStart ==
          D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

    void *sprite_unknown_vtable[3] = {};
    sprite_unknown_vtable[1] = reinterpret_cast<void *>(
        &fake_unknown_add_ref);
    sprite_unknown_vtable[2] = reinterpret_cast<void *>(
        &fake_unknown_release);
    FakeUnknown static_root = {};
    static_root.vtable = sprite_unknown_vtable;
    FakeUnknown heap_root = {};
    heap_root.vtable = sprite_unknown_vtable;
    FakeUnknown pipeline_state = {};
    pipeline_state.vtable = sprite_unknown_vtable;

    void *sprite_device_vtable[11] = {};
    sprite_device_vtable[10] = reinterpret_cast<void *>(
        &fake_create_graphics_pipeline_state);
    FakeDevice sprite_device = {};
    sprite_device.vtable = sprite_device_vtable;
    sprite_device.graphics_pipeline_result =
        reinterpret_cast<ID3D12PipelineState *>(&pipeline_state);
    sprite_device.graphics_pipeline_result_hr = S_OK;

    const D3D12_INDEX_BUFFER_VIEW sprite_index_view = {
        0xABCDEF000ULL, 0x6000, DXGI_FORMAT_R16_UINT};
    DirectX::DX12::jpb_directxtk12_test_set_sprite_device_resources(
        reinterpret_cast<ID3D12Device *>(&sprite_device),
        reinterpret_cast<ID3D12RootSignature *>(&static_root),
        reinterpret_cast<ID3D12RootSignature *>(&heap_root),
        sprite_index_view);

    alignas(DirectX::ResourceUploadBatch)
        unsigned char upload_storage[16] = {};
    auto *upload_batch =
        reinterpret_cast<DirectX::ResourceUploadBatch *>(
            upload_storage);

    const DirectX::RenderTargetState default_target_state;
    CHECK(default_target_state.sampleMask == UINT_MAX);
    CHECK(default_target_state.numRenderTargets == 0);
    CHECK(default_target_state.rtvFormats[0] == DXGI_FORMAT_UNKNOWN);
    CHECK(default_target_state.rtvFormats[7] == DXGI_FORMAT_UNKNOWN);
    CHECK(default_target_state.dsvFormat == DXGI_FORMAT_UNKNOWN);
    CHECK(default_target_state.sampleDesc.Count == 1);
    CHECK(default_target_state.sampleDesc.Quality == 0);
    CHECK(default_target_state.nodeMask == 0);

    const DirectX::RenderTargetState format_target_state(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT);
    CHECK(format_target_state.numRenderTargets == 1);
    CHECK(format_target_state.rtvFormats[0] ==
          DXGI_FORMAT_R8G8B8A8_UNORM);
    CHECK(format_target_state.rtvFormats[1] == DXGI_FORMAT_UNKNOWN);
    CHECK(format_target_state.dsvFormat == DXGI_FORMAT_D32_FLOAT);
    CHECK(format_target_state.sampleDesc.Count == 1);

    DXGI_SWAP_CHAIN_DESC swap_chain_description = {};
    swap_chain_description.BufferDesc.Format =
        DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_description.SampleDesc = {4, 3};
    const DirectX::RenderTargetState swap_chain_target_state(
        &swap_chain_description,
        DXGI_FORMAT_D24_UNORM_S8_UINT);
    CHECK(swap_chain_target_state.rtvFormats[0] ==
          DXGI_FORMAT_B8G8R8A8_UNORM);
    CHECK(swap_chain_target_state.dsvFormat ==
          DXGI_FORMAT_D24_UNORM_S8_UINT);
    CHECK(swap_chain_target_state.sampleDesc.Count == 4);
    CHECK(swap_chain_target_state.sampleDesc.Quality == 3);

    DXGI_SWAP_CHAIN_DESC1 swap_chain_description1 = {};
    swap_chain_description1.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
    swap_chain_description1.SampleDesc = {2, 1};
    const DirectX::RenderTargetState swap_chain_target_state1(
        &swap_chain_description1,
        DXGI_FORMAT_D16_UNORM);
    CHECK(swap_chain_target_state1.rtvFormats[0] ==
          DXGI_FORMAT_R10G10B10A2_UNORM);
    CHECK(swap_chain_target_state1.dsvFormat == DXGI_FORMAT_D16_UNORM);
    CHECK(swap_chain_target_state1.sampleDesc.Count == 2);
    CHECK(swap_chain_target_state1.sampleDesc.Quality == 1);

    DirectX::DX12::SpriteBatchPipelineStateDescription
        default_pipeline_description(format_target_state);
    const D3D12_RENDER_TARGET_BLEND_DESC &default_blend =
        default_pipeline_description.blendDesc.RenderTarget[0];
    CHECK(default_blend.BlendEnable == TRUE);
    CHECK(default_blend.LogicOpEnable == FALSE);
    CHECK(default_blend.SrcBlend == D3D12_BLEND_SRC_ALPHA);
    CHECK(default_blend.DestBlend == D3D12_BLEND_INV_SRC_ALPHA);
    CHECK(default_blend.BlendOp == D3D12_BLEND_OP_ADD);
    CHECK(default_blend.SrcBlendAlpha == D3D12_BLEND_SRC_ALPHA);
    CHECK(default_blend.DestBlendAlpha ==
          D3D12_BLEND_INV_SRC_ALPHA);
    CHECK(default_blend.BlendOpAlpha == D3D12_BLEND_OP_ADD);
    CHECK(default_blend.LogicOp == D3D12_LOGIC_OP_NOOP);
    CHECK(default_blend.RenderTargetWriteMask ==
          D3D12_COLOR_WRITE_ENABLE_ALL);
    CHECK(default_pipeline_description.blendDesc
              .RenderTarget[1].BlendEnable == FALSE);
    CHECK(default_pipeline_description.depthStencilDesc.DepthEnable ==
          FALSE);
    CHECK(default_pipeline_description.depthStencilDesc.DepthWriteMask ==
          D3D12_DEPTH_WRITE_MASK_ZERO);
    CHECK(default_pipeline_description.depthStencilDesc.DepthFunc ==
          D3D12_COMPARISON_FUNC_LESS_EQUAL);
    CHECK(default_pipeline_description.depthStencilDesc
              .FrontFace.StencilFunc == D3D12_COMPARISON_FUNC_ALWAYS);
    CHECK(default_pipeline_description.rasterizerDesc.FillMode ==
          D3D12_FILL_MODE_SOLID);
    CHECK(default_pipeline_description.rasterizerDesc.CullMode ==
          D3D12_CULL_MODE_BACK);
    CHECK(default_pipeline_description.rasterizerDesc.DepthClipEnable ==
          TRUE);
    CHECK(default_pipeline_description.rasterizerDesc.MultisampleEnable ==
          TRUE);
    CHECK(default_pipeline_description.samplerDescriptor.ptr == 0);
    CHECK(default_pipeline_description.customRootSignature == nullptr);
    CHECK(default_pipeline_description.customVertexShader
              .pShaderBytecode == nullptr);
    CHECK(default_pipeline_description.customPixelShader.BytecodeLength ==
          0);

    DirectX::DX12::SpriteBatchPipelineStateDescription
        pipeline_description(format_target_state);
    D3D12_RENDER_TARGET_BLEND_DESC &blend =
        pipeline_description.blendDesc.RenderTarget[0];
    blend.BlendEnable = TRUE;
    blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend.BlendOp = D3D12_BLEND_OP_ADD;
    blend.SrcBlendAlpha = D3D12_BLEND_ONE;
    blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend.LogicOp = D3D12_LOGIC_OP_NOOP;
    blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC &depth =
        pipeline_description.depthStencilDesc;
    depth.DepthEnable = FALSE;
    depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depth.StencilEnable = FALSE;
    depth.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depth.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depth.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depth.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depth.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depth.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    depth.BackFace = depth.FrontFace;

    D3D12_RASTERIZER_DESC &rasterizer =
        pipeline_description.rasterizerDesc;
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.ConservativeRaster =
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    pipeline_description.renderTargetState.sampleMask = UINT_MAX;
    pipeline_description.renderTargetState.numRenderTargets = 1;
    pipeline_description.renderTargetState.rtvFormats[0] =
        DXGI_FORMAT_R8G8B8A8_UNORM;
    pipeline_description.renderTargetState.dsvFormat =
        DXGI_FORMAT_UNKNOWN;
    pipeline_description.renderTargetState.sampleDesc.Count = 1;

    const D3D12_VIEWPORT sprite_viewport = {
        7.0f, 9.0f, 1280.0f, 720.0f, 0.0f, 1.0f};
    auto read_sprite_impl = [](DirectX::DX12::SpriteBatch &batch) {
        void *impl = nullptr;
        std::memcpy(
            &impl,
            reinterpret_cast<unsigned char *>(&batch) + 8,
            sizeof(impl));
        return static_cast<unsigned char *>(impl);
    };

    {
        DirectX::DX12::SpriteBatch sprite_batch(
            reinterpret_cast<ID3D12Device *>(&sprite_device),
            *upload_batch,
            pipeline_description,
            &sprite_viewport);
        CHECK(sprite_device.graphics_pipeline_calls == 1);
        CHECK(*sprite_device.graphics_pipeline_iid ==
              __uuidof(ID3D12PipelineState));
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC &static_pipeline =
            sprite_device.graphics_pipeline_description;
        CHECK(static_pipeline.pRootSignature ==
              reinterpret_cast<ID3D12RootSignature *>(&static_root));
        CHECK(static_pipeline.VS.BytecodeLength == 0xB96);
        CHECK(static_pipeline.PS.BytecodeLength == 0x999);
        CHECK(std::memcmp(
                  static_pipeline.VS.pShaderBytecode,
                  "DXBC",
                  4) == 0);
        CHECK(std::memcmp(
                  static_pipeline.PS.pShaderBytecode,
                  "DXBC",
                  4) == 0);
        CHECK(static_pipeline.InputLayout.NumElements == 3);
        CHECK(std::strcmp(
                  static_pipeline.InputLayout
                      .pInputElementDescs[0].SemanticName,
                  "POSITION") == 0);
        CHECK(static_pipeline.InputLayout
                  .pInputElementDescs[0].Format ==
              DXGI_FORMAT_R32G32B32_FLOAT);
        CHECK(static_pipeline.InputLayout
                  .pInputElementDescs[1].AlignedByteOffset == 12);
        CHECK(static_pipeline.InputLayout
                  .pInputElementDescs[2].AlignedByteOffset == 28);
        CHECK(std::memcmp(
                  &static_pipeline.BlendState,
                  &pipeline_description.blendDesc,
                  sizeof(pipeline_description.blendDesc)) == 0);
        CHECK(std::memcmp(
                  &static_pipeline.DepthStencilState,
                  &pipeline_description.depthStencilDesc,
                  sizeof(pipeline_description.depthStencilDesc)) == 0);
        CHECK(std::memcmp(
                  &static_pipeline.RasterizerState,
                  &pipeline_description.rasterizerDesc,
                  sizeof(pipeline_description.rasterizerDesc)) == 0);
        CHECK(static_pipeline.SampleMask == UINT_MAX);
        CHECK(static_pipeline.PrimitiveTopologyType ==
              D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        CHECK(static_pipeline.NumRenderTargets == 1);
        CHECK(static_pipeline.RTVFormats[0] ==
              DXGI_FORMAT_R8G8B8A8_UNORM);
        CHECK(static_pipeline.SampleDesc.Count == 1);

        unsigned char *sprite_state = read_sprite_impl(sprite_batch);
        CHECK(sprite_state != nullptr);
        CHECK(*reinterpret_cast<DXGI_MODE_ROTATION *>(sprite_state) ==
              DXGI_MODE_ROTATION_IDENTITY);
        CHECK(sprite_state[4] == 1);
        CHECK(std::memcmp(
                  sprite_state + 8,
                  &sprite_viewport,
                  sizeof(sprite_viewport)) == 0);
        CHECK(*reinterpret_cast<std::uint64_t *>(
                  sprite_state + 32) == 0);
        CHECK(*reinterpret_cast<void **>(sprite_state + 96) != nullptr);
        CHECK(*reinterpret_cast<void **>(sprite_state + 104) != nullptr);
        CHECK(*reinterpret_cast<std::size_t *>(
                  sprite_state + 232) == 0x48000);

        unsigned char *original_state = sprite_state;
        DirectX::DX12::SpriteBatch moved(std::move(sprite_batch));
        CHECK(read_sprite_impl(sprite_batch) == nullptr);
        CHECK(read_sprite_impl(moved) == original_state);

        pipeline_description.samplerDescriptor.ptr = 0x1234;
        DirectX::DX12::SpriteBatch heap_sampler_batch(
            reinterpret_cast<ID3D12Device *>(&sprite_device),
            *upload_batch,
            pipeline_description,
            nullptr);
        CHECK(sprite_device.graphics_pipeline_calls == 2);
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC &heap_pipeline =
            sprite_device.graphics_pipeline_description;
        CHECK(heap_pipeline.pRootSignature ==
              reinterpret_cast<ID3D12RootSignature *>(&heap_root));
        CHECK(heap_pipeline.VS.BytecodeLength == 0xB96);
        CHECK(heap_pipeline.PS.BytecodeLength == 0x99D);
        unsigned char *heap_state =
            read_sprite_impl(heap_sampler_batch);
        CHECK(heap_state[4] == 0);
        CHECK(*reinterpret_cast<std::uint64_t *>(heap_state + 32) ==
              0x1234);
        CHECK(*reinterpret_cast<void **>(heap_state + 96) != nullptr);
        CHECK(*reinterpret_cast<void **>(heap_state + 104) != nullptr);

        heap_sampler_batch = std::move(moved);
        CHECK(read_sprite_impl(moved) == nullptr);
        CHECK(read_sprite_impl(heap_sampler_batch) == original_state);
    }
    DirectX::DX12::jpb_directxtk12_test_clear_sprite_device_resources();
    CHECK(static_root.add_ref_calls == 2);
    CHECK(static_root.release_calls == 2);
    CHECK(heap_root.add_ref_calls == 2);
    CHECK(heap_root.release_calls == 2);
    CHECK(pipeline_state.release_calls == 2);
    return 0;
}

int main()
{
    CHECK(test_leaf_methods_and_exact_offsets() == 0);
    CHECK(test_game_bar_state_handlers() == 0);
    CHECK(test_matrix_conversion() == 0);
    CHECK(test_transparency_pass_index_batching() == 0);
    CHECK(test_transparency_pass_buffer_creation() == 0);
    CHECK(test_application_pipeline_creation() == 0);
    CHECK(test_transparency_pass_pipeline_creation() == 0);
    CHECK(test_transparency_pass_shader_creation() == 0);
    CHECK(test_transparency_pass_update() == 0);
    CHECK(test_transparency_pass_render() == 0);
    CHECK(test_draw_texture_queue() == 0);
    CHECK(test_debug_helpers() == 0);
    CHECK(test_framework_error_messages() == 0);
    CHECK(test_registry_accessors() == 0);
    CHECK(test_resource_transition_and_resolution() == 0);
    CHECK(test_command_list_and_viewport_initialization() == 0);
    CHECK(test_change_environment_failure_gates() == 0);
    CHECK(test_render_3d_environment() == 0);
    CHECK(test_render_menu_texture_upload() == 0);
    CHECK(test_level_draw_passes() == 0);
    CHECK(test_run_quit_message() == 0);
    CHECK(test_message_procedure_core_paths() == 0);
    CHECK(test_application_initializer_failure_gates() == 0);
    CHECK(test_application_initializer_scene_buffer() == 0);
    CHECK(test_depth_stencil_initialization() == 0);
    CHECK(test_fence_initialization() == 0);
    CHECK(test_srv_heap_initialization() == 0);
    CHECK(test_render_target_initialization() == 0);
    CHECK(test_device_initialization() == 0);
    CHECK(test_root_signature_initialization() == 0);
    CHECK(test_swap_chain_initialization() == 0);
    CHECK(test_move_to_next_frame() == 0);
    CHECK(test_frame_pacing_pause_and_gpu_wait() == 0);
    CHECK(test_destroy_d3d12_objects() == 0);
    CHECK(test_start_render_and_frame_begin() == 0);
    CHECK(test_frame_end() == 0);
    CHECK(test_end_render() == 0);
    CHECK(test_application_create() == 0);
    CHECK(test_application_constructor_and_destructor() == 0);
    CHECK(test_lifecycle_callbacks() == 0);
    CHECK(test_window_state_and_message_pump() == 0);
    CHECK(test_resource_upload_batch_lifecycle() == 0);
    CHECK(test_directxtk_descriptor_abi() == 0);
    std::puts("d3dapp leaf tests passed");
    return 0;
}
