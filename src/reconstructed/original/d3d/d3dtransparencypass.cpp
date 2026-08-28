/* COMPLETE REVIEWED RECONSTRUCTION. */

#include "jpb/d3dtransparencypass.h"

#include "jpb/d3dframe.h"
#include "jpb/resources.h"
#include "jpb/steam_interfaces.h"

#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <vector>

#if defined(JPB_D3DAPP_TESTING)
static JPBTransparencySteamDeckTestHook steam_deck_test_hook;

void jpb_d3dtransparency_set_steam_deck_test_hook(
    JPBTransparencySteamDeckTestHook hook)
{
    steam_deck_test_hook = hook;
}
#endif

static bool d3dtransparency_is_steam_deck()
{
#if defined(JPB_D3DAPP_TESTING)
    if (steam_deck_test_hook != nullptr) {
        return steam_deck_test_hook();
    }
#endif
    ISteamUtils *utils = SteamUtils();
    void **vtable = *reinterpret_cast<void ***>(utils);
    using IsSteamRunningOnSteamDeckFunction = bool (*)(ISteamUtils *);
    return reinterpret_cast<IsSteamRunningOnSteamDeckFunction>(vtable[34])(
        utils);
}

/*
 * GENERATED RECONSTRUCTION SHELL - no function bodies recovered here.
 * PDB module: 0026
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\d3dtransparencypass.obj
 * Primary source: W:\SWJediPowerBattles\work\d3d\d3dtransparencypass.cpp
 * Compiler language: c++
 * Emitted procedures: 51
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

/* 0x40940, 40 bytes, global, 2 named locals
 * IID_PPV_ARGS_Helper<Microsoft::WRL::ComPtr<ID3D12PipelineState> >
 * PDB type: void** (Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x40970, 23 bytes, global, 4 named locals
 * std::_Copy_backward_memmove<Vertex *,Vertex *>
 * PDB type: Vertex* (Vertex*, Vertex*, Verte...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x40990, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<unsigned int *,unsigned int *>
 * PDB type: unsigned* (unsigned*, unsigned*,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x409C0, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<Vertex *,Vertex *>
 * PDB type: Vertex* (Vertex*, Vertex*, Verte...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x409F0, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<Vertex const *,Vertex *>
 * PDB type: Vertex* (const Vertex*, const Ve...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x40A20, 46 bytes, global, 5 named locals
 * std::_Copy_memmove_n<Vertex const *,Vertex *>
 * PDB type: Vertex* (const Vertex*, const un...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x40A50, 414 bytes, global, 23 named locals
 * std::vector<unsigned int,std::allocator<unsigned int> >::_Emplace_reallocate<unsigned int>
 * PDB type: unsigned* std::vector<unsigned i...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x40BF0, 864 bytes, global, 28 named locals
 * std::vector<Vertex,std::allocator<Vertex> >::_Insert_counted_range<Vertex const *>
 * PDB type: void std::vector<Vertex,std::all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x40F50, 48 bytes, global, 4 named locals
 * std::_Move_unchecked<Vertex *,Vertex *>
 * PDB type: Vertex* (Vertex*, Vertex*, Verte...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x40F80, 78 bytes, global, 5 named locals
 * std::_Uninitialized_move<Vertex *,std::allocator<Vertex> >
 * PDB type: Vertex* (Vertex* const, Vertex* ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x40FD0, 199 bytes, global, 2 named locals
 * D3DTransparencyPass::D3DTransparencyPass
 * PDB type: void D3DTransparencyPass::(CD3DF...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtransparencypass.cpp
 */
D3DTransparencyPass::D3DTransparencyPass(CD3DFramework12 *framework)
    : m_vertexBufferView{},
      m_indexBufferView{},
      m_additiveVertexBufferView{},
      m_additiveIndexBufferView{},
      m_pFramework(framework)
{
    CreateShaders();
    CreatePipelineState();
    CreateBuffers();
}

/* 0x410A0, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<ID3D12PipelineState>::~ComPtr<ID3D12PipelineState>
 * PDB type: void Microsoft::WRL::ComPtr<ID3D...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x410D0, 91 bytes, global, 5 named locals
 * std::vector<unsigned int,std::allocator<unsigned int> >::~vector<unsigned int,std::allocator<unsigned int> >
 * PDB type: void std::vector<unsigned int,st...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x41130, 389 bytes, global, 12 named locals
 * D3DTransparencyPass::AddIndexed
 * PDB type: void D3DTransparencyPass::(D3DMa...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtransparencypass.cpp
 */
void D3DTransparencyPass::AddIndexed(
    D3DMaterialType type,
    const std::vector<Vertex> &vertices,
    const std::vector<unsigned int> &indices)
{
    if (type == D3DMaterialType::Transluscent) {
        const unsigned int vertex_offset =
            static_cast<unsigned int>(m_vertices.size());
        m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
        for (unsigned int index : indices) {
            m_indices.push_back(index + vertex_offset);
        }
    } else if (type == D3DMaterialType::Additive) {
        const unsigned int vertex_offset =
            static_cast<unsigned int>(m_additiveVertices.size());
        m_additiveVertices.insert(
            m_additiveVertices.end(), vertices.begin(), vertices.end());
        for (unsigned int index : indices) {
            m_additiveIndices.push_back(index + vertex_offset);
        }
    }
}

const std::uint64_t D3DTransparencyPass::m_vertexBufferSize = 0x72420;
const std::uint64_t D3DTransparencyPass::m_indexBufferSize = 0x11940;

/* 0x412C0, 1411 bytes, global, 14 named locals
 * D3DTransparencyPass::CreateBuffers
 * PDB type: void D3DTransparencyPass::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtransparencypass.cpp
 */
void D3DTransparencyPass::CreateBuffers()
{
    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heap_properties.CreationNodeMask = 1;
    heap_properties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resource_description = {};
    resource_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_description.Width = m_vertexBufferSize;
    resource_description.Height = 1;
    resource_description.DepthOrArraySize = 1;
    resource_description.MipLevels = 1;
    resource_description.SampleDesc.Count = 1;
    resource_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    m_pFramework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_vertexUploadBuffer.ReleaseAndGetAddressOf()));

    resource_description.Width = m_indexBufferSize;
    m_pFramework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_indexUploadBuffer.ReleaseAndGetAddressOf()));

    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    resource_description.Width = m_vertexBufferSize;
    m_pFramework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_vertexBuffer.ReleaseAndGetAddressOf()));

    resource_description.Width = m_indexBufferSize;
    m_pFramework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_indexBuffer.ReleaseAndGetAddressOf()));

    m_vertexBufferView.BufferLocation =
        m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes =
        static_cast<UINT>(m_vertexBufferSize);
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_indexBufferView.BufferLocation =
        m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.SizeInBytes =
        static_cast<UINT>(m_indexBufferSize);
    m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;

    heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
    resource_description.Width = m_vertexBufferSize;
    m_pFramework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(
            m_additiveVertexUploadBuffer.ReleaseAndGetAddressOf()));

    resource_description.Width = m_indexBufferSize;
    m_pFramework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(
            m_additiveIndexUploadBuffer.ReleaseAndGetAddressOf()));

    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    resource_description.Width = m_vertexBufferSize;
    m_pFramework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_additiveVertexBuffer.ReleaseAndGetAddressOf()));

    resource_description.Width = m_indexBufferSize;
    m_pFramework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_additiveIndexBuffer.ReleaseAndGetAddressOf()));

    m_additiveVertexBufferView.BufferLocation =
        m_additiveVertexBuffer->GetGPUVirtualAddress();
    m_additiveVertexBufferView.SizeInBytes =
        static_cast<UINT>(m_vertexBufferSize);
    m_additiveVertexBufferView.StrideInBytes = sizeof(Vertex);
    m_additiveIndexBufferView.BufferLocation =
        m_additiveIndexBuffer->GetGPUVirtualAddress();
    m_additiveIndexBufferView.SizeInBytes =
        static_cast<UINT>(m_indexBufferSize);
    m_additiveIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

/* 0x41850, 1460 bytes, global, 13 named locals
 * D3DTransparencyPass::CreatePipelineState
 * PDB type: void D3DTransparencyPass::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtransparencypass.cpp
 */
void D3DTransparencyPass::CreatePipelineState()
{
    const D3D12_INPUT_ELEMENT_DESC input_element_descriptions[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, 40,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_RENDER_TARGET_BLEND_DESC transparent_render_target = {};
    transparent_render_target.BlendEnable = TRUE;
    transparent_render_target.LogicOpEnable = FALSE;
    transparent_render_target.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    transparent_render_target.DestBlend = D3D12_BLEND_ONE;
    transparent_render_target.BlendOp = D3D12_BLEND_OP_ADD;
    transparent_render_target.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    transparent_render_target.DestBlendAlpha = D3D12_BLEND_ONE;
    transparent_render_target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    transparent_render_target.LogicOp = D3D12_LOGIC_OP_NOOP;
    transparent_render_target.RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_BLEND_DESC transparent_blend = {};
    transparent_blend.RenderTarget[0] = transparent_render_target;

    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.DepthClipEnable = TRUE;

    D3D12_DEPTH_STENCIL_DESC depth_stencil = {};
    depth_stencil.DepthEnable = TRUE;
    depth_stencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depth_stencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depth_stencil.StencilEnable = FALSE;
    depth_stencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depth_stencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depth_stencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depth_stencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depth_stencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depth_stencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    depth_stencil.BackFace = depth_stencil.FrontFace;

    IDxcBlob *vertex_shader =
        reinterpret_cast<IDxcBlob *>(m_pFramework->vertexShaderBlob);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC transparent_description = {};
    transparent_description.pRootSignature =
        m_pFramework->m_pRootSignature;
    transparent_description.VS = {
        vertex_shader->GetBufferPointer(),
        vertex_shader->GetBufferSize(),
    };
    transparent_description.PS = {
        m_PixelShaderBlob->GetBufferPointer(),
        m_PixelShaderBlob->GetBufferSize(),
    };
    transparent_description.BlendState = transparent_blend;
    transparent_description.SampleMask = UINT_MAX;
    transparent_description.RasterizerState = rasterizer;
    transparent_description.DepthStencilState = depth_stencil;
    transparent_description.InputLayout = {
        input_element_descriptions,
        static_cast<UINT>(std::size(input_element_descriptions)),
    };
    transparent_description.IBStripCutValue =
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    transparent_description.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    transparent_description.NumRenderTargets = 1;
    transparent_description.RTVFormats[0] =
        DXGI_FORMAT_R8G8B8A8_UNORM;
    transparent_description.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    transparent_description.SampleDesc.Count = 1;

    m_pFramework->m_pDevice->CreateGraphicsPipelineState(
        &transparent_description,
        IID_PPV_ARGS(m_TransparentPSO.ReleaseAndGetAddressOf()));

    D3D12_RENDER_TARGET_BLEND_DESC additive_render_target = {};
    additive_render_target.BlendEnable = TRUE;
    additive_render_target.LogicOpEnable = FALSE;
    additive_render_target.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    additive_render_target.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    additive_render_target.BlendOp = D3D12_BLEND_OP_ADD;
    additive_render_target.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    additive_render_target.DestBlendAlpha =
        D3D12_BLEND_INV_SRC_ALPHA;
    additive_render_target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    additive_render_target.LogicOp = D3D12_LOGIC_OP_NOOP;
    additive_render_target.RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC additive_description =
        transparent_description;
    additive_description.BlendState = {};
    additive_description.BlendState.RenderTarget[0] =
        additive_render_target;
    m_pFramework->m_pDevice->CreateGraphicsPipelineState(
        &additive_description,
        IID_PPV_ARGS(m_AdditivePSO.ReleaseAndGetAddressOf()));
}

/* 0x41E10, 1226 bytes, global, 26 named locals
 * D3DTransparencyPass::CreateShaders
 * PDB type: void D3DTransparencyPass::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtransparencypass.cpp
 */
void D3DTransparencyPass::CreateShaders()
{
    Microsoft::WRL::ComPtr<IDxcUtils> utils;
    Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
    DxcCreateInstance(
        CLSID_DxcUtils,
        IID_PPV_ARGS(utils.ReleaseAndGetAddressOf()));
    DxcCreateInstance(
        CLSID_DxcCompiler,
        IID_PPV_ARGS(compiler.ReleaseAndGetAddressOf()));

    Microsoft::WRL::ComPtr<IDxcIncludeHandler> include_handler;
    utils->CreateDefaultIncludeHandler(
        include_handler.ReleaseAndGetAddressOf());

    const char *shader_path = resource_getPath(
        "TransparencyPixelShader.hlsl", JPB_RESOURCE_SHADER);
    MultiByteToWideChar(
        CP_UTF8, 0, shader_path, -1, nullptr, 0);
    MultiByteToWideChar(
        CP_UTF8, 0, shader_path, -1, new wchar_t[256], 256);

    std::vector<const wchar_t *> arguments;
    arguments.push_back(L"-E");
    arguments.push_back(L"PSMain");
    arguments.push_back(L"-T");
    arguments.push_back(L"ps_6_0");
    if (d3dtransparency_is_steam_deck()) {
        arguments.push_back(L"-D");
        arguments.push_back(L"STEAM_DECK");
    }

    wchar_t wide_shader_path[256];
    std::mbstowcs(
        wide_shader_path,
        shader_path,
        std::strlen(shader_path) + 1);

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> source;
    utils->LoadFile(
        wide_shader_path, nullptr, source.ReleaseAndGetAddressOf());
    DxcBuffer source_buffer = {
        source->GetBufferPointer(),
        source->GetBufferSize(),
        0,
    };

    Microsoft::WRL::ComPtr<IDxcResult> results;
    compiler->Compile(
        &source_buffer,
        arguments.data(),
        static_cast<UINT32>(arguments.size()),
        include_handler.Get(),
        IID_PPV_ARGS(results.ReleaseAndGetAddressOf()));

    Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
    HRESULT result = results->GetOutput(
        DXC_OUT_ERRORS,
        IID_PPV_ARGS(errors.ReleaseAndGetAddressOf()),
        nullptr);
    if (errors != nullptr && errors->GetStringLength() != 0) {
        OutputDebugStringA(errors->GetStringPointer());
    }
    results->GetStatus(&result);
    result = results->GetOutput(
        DXC_OUT_OBJECT,
        IID_PPV_ARGS(m_PixelShaderBlob.ReleaseAndGetAddressOf()),
        nullptr);
    (void)result;
}

/* 0x422E0, 1076 bytes, global, 8 named locals
 * D3DTransparencyPass::Render
 * PDB type: void D3DTransparencyPass::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtransparencypass.cpp
 */
void D3DTransparencyPass::Render()
{
    ID3D12GraphicsCommandList *command_list =
        m_pFramework->m_pCommandList;
    ID3D12DescriptorHeap *descriptor_heaps[] = {
        m_pFramework->m_pMainDescriptorHeap,
    };
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle =
        m_pFramework->m_pRtvDescriptorHeap
            ->GetCPUDescriptorHandleForHeapStart();
    rtv_handle.ptr += static_cast<SIZE_T>(
        m_pFramework->m_RTVDescriptorSize *
        m_pFramework->m_nFrameIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle =
        m_pFramework->m_pDsDescriptorHeap
            ->GetCPUDescriptorHandleForHeapStart();
    command_list->OMSetRenderTargets(
        1, &rtv_handle, FALSE, &dsv_handle);

    if (!m_indices.empty()) {
        command_list->SetPipelineState(m_TransparentPSO.Get());
        command_list->SetDescriptorHeaps(1, descriptor_heaps);
        command_list->SetGraphicsRootSignature(
            m_pFramework->m_pRootSignature);
        command_list->SetGraphicsRootDescriptorTable(
            1,
            m_pFramework->m_pMainDescriptorHeap
                ->GetGPUDescriptorHandleForHeapStart());
        command_list->RSSetViewports(1, &m_pFramework->m_Viewport);
        command_list->RSSetScissorRects(1, &m_pFramework->m_ScissorRect);
        command_list->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        command_list->IASetIndexBuffer(&m_indexBufferView);
        command_list->OMSetRenderTargets(
            1, &rtv_handle, FALSE, &dsv_handle);
        if (m_pFramework->m_isAMD != FALSE) {
            for (UINT index = 0;
                 index < m_indices.size() * 3;
                 index += 3) {
                command_list->DrawIndexedInstanced(
                    3, 1, index, 0, 0);
            }
        } else {
            command_list->DrawIndexedInstanced(
                static_cast<UINT>(m_indices.size()), 1, 0, 0, 0);
        }
        m_vertices.clear();
        m_indices.clear();
    }

    if (!m_additiveIndices.empty()) {
        command_list->SetPipelineState(m_AdditivePSO.Get());
        command_list->SetDescriptorHeaps(1, descriptor_heaps);
        command_list->SetGraphicsRootSignature(
            m_pFramework->m_pRootSignature);
        command_list->SetGraphicsRootDescriptorTable(
            1,
            m_pFramework->m_pMainDescriptorHeap
                ->GetGPUDescriptorHandleForHeapStart());
        command_list->RSSetViewports(1, &m_pFramework->m_Viewport);
        command_list->RSSetScissorRects(1, &m_pFramework->m_ScissorRect);
        command_list->IASetPrimitiveTopology(
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        command_list->IASetVertexBuffers(
            0, 1, &m_additiveVertexBufferView);
        command_list->IASetIndexBuffer(&m_additiveIndexBufferView);
        command_list->OMSetRenderTargets(
            1, &rtv_handle, FALSE, &dsv_handle);
        if (m_pFramework->m_isAMD != FALSE) {
            for (UINT index = 0;
                 index < m_additiveIndices.size() * 3;
                 index += 3) {
                command_list->DrawIndexedInstanced(
                    3, 1, index, 0, 0);
            }
        } else {
            command_list->DrawIndexedInstanced(
                static_cast<UINT>(m_additiveIndices.size()),
                1,
                0,
                0,
                0);
        }
        m_additiveVertices.clear();
        m_additiveIndices.clear();
    }
}

/* 0x42720, 1199 bytes, global, 16 named locals
 * D3DTransparencyPass::Update
 * PDB type: void D3DTransparencyPass::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dtransparencypass.cpp
 */
void D3DTransparencyPass::Update()
{
    ID3D12GraphicsCommandList *command_list =
        m_pFramework->m_pCommandList;
    if (!m_vertices.empty() || !m_indices.empty()) {
        void *vertex_data_begin;
        void *index_data_begin;
        m_vertexUploadBuffer->Map(0, nullptr, &vertex_data_begin);
        m_indexUploadBuffer->Map(0, nullptr, &index_data_begin);
        std::memset(vertex_data_begin, 0, m_vertexBufferSize);
        std::memset(index_data_begin, 0, m_indexBufferSize);
        std::memcpy(
            vertex_data_begin,
            m_vertices.data(),
            m_vertices.size() * sizeof(Vertex));
        std::memcpy(
            index_data_begin,
            m_indices.data(),
            m_indices.size() * sizeof(unsigned int));
        m_vertexUploadBuffer->Unmap(0, nullptr);
        m_indexUploadBuffer->Unmap(0, nullptr);

        D3D12_RESOURCE_BARRIER to_copy_barriers[2] = {};
        to_copy_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        to_copy_barriers[0].Transition.pResource = m_vertexBuffer.Get();
        to_copy_barriers[0].Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        to_copy_barriers[0].Transition.StateBefore =
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        to_copy_barriers[0].Transition.StateAfter =
            D3D12_RESOURCE_STATE_COPY_DEST;
        to_copy_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        to_copy_barriers[1].Transition.pResource = m_indexBuffer.Get();
        to_copy_barriers[1].Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        to_copy_barriers[1].Transition.StateBefore =
            D3D12_RESOURCE_STATE_INDEX_BUFFER;
        to_copy_barriers[1].Transition.StateAfter =
            D3D12_RESOURCE_STATE_COPY_DEST;

        D3D12_RESOURCE_BARRIER to_vertex_index_barriers[2] =
            {to_copy_barriers[0], to_copy_barriers[1]};
        to_vertex_index_barriers[0].Transition.StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;
        to_vertex_index_barriers[0].Transition.StateAfter =
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        to_vertex_index_barriers[1].Transition.StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;
        to_vertex_index_barriers[1].Transition.StateAfter =
            D3D12_RESOURCE_STATE_INDEX_BUFFER;

        command_list->ResourceBarrier(2, to_copy_barriers);
        command_list->CopyBufferRegion(
            m_vertexBuffer.Get(),
            0,
            m_vertexUploadBuffer.Get(),
            0,
            m_vertexBufferSize);
        command_list->CopyBufferRegion(
            m_indexBuffer.Get(),
            0,
            m_indexUploadBuffer.Get(),
            0,
            m_indexBufferSize);
        command_list->ResourceBarrier(2, to_vertex_index_barriers);
    }

    if (!m_additiveVertices.empty() || !m_additiveIndices.empty()) {
        void *vertex_data_begin;
        void *index_data_begin;
        m_additiveVertexUploadBuffer->Map(
            0, nullptr, &vertex_data_begin);
        m_additiveIndexUploadBuffer->Map(
            0, nullptr, &index_data_begin);
        std::memset(vertex_data_begin, 0, m_vertexBufferSize);
        std::memset(index_data_begin, 0, m_indexBufferSize);
        std::memcpy(
            vertex_data_begin,
            m_additiveVertices.data(),
            m_additiveVertices.size() * sizeof(Vertex));
        std::memcpy(
            index_data_begin,
            m_additiveIndices.data(),
            m_additiveIndices.size() * sizeof(unsigned int));
        m_additiveVertexUploadBuffer->Unmap(0, nullptr);
        m_additiveIndexUploadBuffer->Unmap(0, nullptr);

        D3D12_RESOURCE_BARRIER to_copy_barriers[2] = {};
        to_copy_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        to_copy_barriers[0].Transition.pResource =
            m_additiveVertexBuffer.Get();
        to_copy_barriers[0].Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        to_copy_barriers[0].Transition.StateBefore =
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        to_copy_barriers[0].Transition.StateAfter =
            D3D12_RESOURCE_STATE_COPY_DEST;
        to_copy_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        to_copy_barriers[1].Transition.pResource =
            m_additiveIndexBuffer.Get();
        to_copy_barriers[1].Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        to_copy_barriers[1].Transition.StateBefore =
            D3D12_RESOURCE_STATE_INDEX_BUFFER;
        to_copy_barriers[1].Transition.StateAfter =
            D3D12_RESOURCE_STATE_COPY_DEST;

        D3D12_RESOURCE_BARRIER to_vertex_index_barriers[2] =
            {to_copy_barriers[0], to_copy_barriers[1]};
        to_vertex_index_barriers[0].Transition.StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;
        to_vertex_index_barriers[0].Transition.StateAfter =
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        to_vertex_index_barriers[1].Transition.StateBefore =
            D3D12_RESOURCE_STATE_COPY_DEST;
        to_vertex_index_barriers[1].Transition.StateAfter =
            D3D12_RESOURCE_STATE_INDEX_BUFFER;

        command_list->ResourceBarrier(2, to_copy_barriers);
        command_list->CopyBufferRegion(
            m_additiveVertexBuffer.Get(),
            0,
            m_additiveVertexUploadBuffer.Get(),
            0,
            m_vertexBufferSize);
        command_list->CopyBufferRegion(
            m_additiveIndexBuffer.Get(),
            0,
            m_additiveIndexUploadBuffer.Get(),
            0,
            m_indexBufferSize);
        command_list->ResourceBarrier(2, to_vertex_index_barriers);
    }
}

/* 0x42BD0, 17 bytes, global, 0 named locals
 * std::vector<unsigned int,std::allocator<unsigned int> >::_Xlength
 * PDB type: void std::vector<unsigned int,st...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x42BF0, 17 bytes, global, 0 named locals
 * std::vector<Vertex,std::allocator<Vertex> >::_Xlength
 * PDB type: void std::vector<Vertex,std::all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x42C10, 66 bytes, global, 7 named locals
 * std::allocator<unsigned int>::deallocate
 * PDB type: void std::allocator<unsigned int...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x42C60, 62 bytes, global, 7 named locals
 * std::allocator<Vertex>::deallocate
 * PDB type: void std::allocator<Vertex>::(Ve...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x26FD30, 40 bytes, local, 2 named locals
 * `std::vector<unsigned int,std::allocator<unsigned int> >::_Emplace_reallocate<unsigned int>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x26FD60, 49 bytes, local, 2 named locals
 * `std::vector<Vertex,std::allocator<Vertex> >::_Insert_counted_range<Vertex const *>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x26FDA0, 37 bytes, local, 2 named locals
 * `std::vector<Vertex,std::allocator<Vertex> >::_Insert_counted_range<Vertex const *>'::`1'::catch$1
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x26FDD0, 102 bytes, local, 7 named locals
 * `std::vector<Vertex,std::allocator<Vertex> >::_Insert_counted_range<Vertex const *>'::`1'::catch$2
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x26FE40, 37 bytes, local, 2 named locals
 * `std::vector<Vertex,std::allocator<Vertex> >::_Insert_counted_range<Vertex const *>'::`1'::catch$3
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x26FE70, 65 bytes, local, 5 named locals
 * `std::vector<Vertex,std::allocator<Vertex> >::_Insert_counted_range<Vertex const *>'::`1'::catch$4
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x26FEC0, 12 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FED0, 16 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FEE0, 16 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FEF0, 16 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FF00, 16 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FF10, 16 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FF20, 16 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FF30, 16 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$7
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FF40, 19 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$8
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FF60, 19 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FF80, 19 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$10
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FFA0, 19 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$11
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FFC0, 19 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$12
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FFE0, 19 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$13
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270000, 19 bytes, local, 1 named locals
 * `D3DTransparencyPass::D3DTransparencyPass'::`1'::dtor$14
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270020, 12 bytes, local, 10 named locals
 * `D3DTransparencyPass::CreateShaders'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270030, 12 bytes, local, 10 named locals
 * `D3DTransparencyPass::CreateShaders'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270040, 12 bytes, local, 10 named locals
 * `D3DTransparencyPass::CreateShaders'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270050, 12 bytes, local, 10 named locals
 * `D3DTransparencyPass::CreateShaders'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270060, 12 bytes, local, 10 named locals
 * `D3DTransparencyPass::CreateShaders'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270070, 12 bytes, local, 10 named locals
 * `D3DTransparencyPass::CreateShaders'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x270080, 12 bytes, local, 10 named locals
 * `D3DTransparencyPass::CreateShaders'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */
