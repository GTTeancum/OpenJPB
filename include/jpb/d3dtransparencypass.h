#ifndef JPB_D3DTRANSPARENCYPASS_H
#define JPB_D3DTRANSPARENCYPASS_H

#include <DirectXMath.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>

#include <cstddef>
#include <cstdint>
#include <vector>

class CD3DFramework12;

struct Vertex {
    DirectX::XMFLOAT4 pos;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 texCoord;
    int texIndex;
    DirectX::XMFLOAT2 uvScroll;
};

enum class D3DMaterialType : int {
    Opaque = 0,
    Transluscent = 1,
    Additive = 2,
};

class D3DTransparencyPass {
public:
    static const std::uint64_t m_vertexBufferSize;
    static const std::uint64_t m_indexBufferSize;

    explicit D3DTransparencyPass(CD3DFramework12 *framework);
    void AddIndexed(
        D3DMaterialType type,
        const std::vector<Vertex> &vertices,
        const std::vector<unsigned int> &indices);
    void CreateBuffers();
    void CreatePipelineState();
    void CreateShaders();
    void Render();
    void Update();

    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<Vertex> m_additiveVertices;
    std::vector<unsigned int> m_additiveIndices;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_TransparentPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_AdditivePSO;
    Microsoft::WRL::ComPtr<IDxcBlob> m_PixelShaderBlob;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexUploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexUploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_additiveVertexUploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_additiveIndexUploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_additiveVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_additiveIndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    D3D12_VERTEX_BUFFER_VIEW m_additiveVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_additiveIndexBufferView;
    CD3DFramework12 *m_pFramework;
};

#if defined(JPB_D3DAPP_TESTING)
using JPBTransparencySteamDeckTestHook = bool (*)();
void jpb_d3dtransparency_set_steam_deck_test_hook(
    JPBTransparencySteamDeckTestHook hook);
#endif

static_assert(sizeof(Vertex) == 52, "Vertex PDB size changed");
static_assert(offsetof(Vertex, pos) == 0, "Vertex.pos offset changed");
static_assert(offsetof(Vertex, color) == 16, "Vertex.color offset changed");
static_assert(offsetof(Vertex, texCoord) == 32,
              "Vertex.texCoord offset changed");
static_assert(offsetof(Vertex, texIndex) == 40,
              "Vertex.texIndex offset changed");
static_assert(offsetof(Vertex, uvScroll) == 44,
              "Vertex.uvScroll offset changed");
static_assert(sizeof(D3DTransparencyPass) == 256,
              "D3DTransparencyPass PDB size changed");
static_assert(offsetof(D3DTransparencyPass, m_vertices) == 0,
              "D3DTransparencyPass.m_vertices offset changed");
static_assert(offsetof(D3DTransparencyPass, m_indices) == 24,
              "D3DTransparencyPass.m_indices offset changed");
static_assert(offsetof(D3DTransparencyPass, m_additiveVertices) == 48,
              "D3DTransparencyPass.m_additiveVertices offset changed");
static_assert(offsetof(D3DTransparencyPass, m_additiveIndices) == 72,
              "D3DTransparencyPass.m_additiveIndices offset changed");
static_assert(offsetof(D3DTransparencyPass, m_TransparentPSO) == 96,
              "D3DTransparencyPass.m_TransparentPSO offset changed");
static_assert(offsetof(D3DTransparencyPass, m_vertexUploadBuffer) == 120,
              "D3DTransparencyPass.m_vertexUploadBuffer offset changed");
static_assert(offsetof(D3DTransparencyPass, m_vertexBufferView) == 184,
              "D3DTransparencyPass.m_vertexBufferView offset changed");
static_assert(offsetof(D3DTransparencyPass, m_pFramework) == 248,
              "D3DTransparencyPass.m_pFramework offset changed");

#endif
