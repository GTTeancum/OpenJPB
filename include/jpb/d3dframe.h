#ifndef JPB_D3DFRAME_H
#define JPB_D3DFRAME_H

#include <d3d12.h>
#include <d3d.h>
#include <ddraw.h>
#include <dxgi1_6.h>
#include <windows.h>
#include <cstddef>
#include <cstdint>

struct JpbPdbVectorUint64 {
    std::uint64_t *begin;
    std::uint64_t *end;
    std::uint64_t *capacity_end;
};

static_assert(sizeof(JpbPdbVectorUint64) == 24,
              "MSVC release vector representation changed");

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;

class CD3DFramework7 {
public:
    CD3DFramework7();
    ~CD3DFramework7();

    HRESULT CreateDirect3D(GUID *device_guid);
    HRESULT CreateDirectDraw(GUID *driver_guid, DWORD flags);
    HRESULT CreateEnvironment(
        GUID *driver_guid,
        GUID *device_guid,
        DDSURFACEDESC2 *mode,
        DWORD flags);
    HRESULT CreateFullscreenBuffers(DDSURFACEDESC2 *mode);
    HRESULT CreateWindowedBuffers();
    HRESULT CreateZBuffer(GUID *device_guid);
    HRESULT DestroyObjects();
    HRESULT FlipToGDISurface(int draw_frame);
    HRESULT Initialize(
        HWND window,
        GUID *driver_guid,
        GUID *device_guid,
        DDSURFACEDESC2 *mode,
        DWORD flags);
    void Move(int x, int y);
    void Repaint();
    HRESULT RestoreSurfaces();
    HRESULT ShowFrame();

    HWND m_hWnd;
    BOOL m_bIsFullscreen;
    DWORD m_dwRenderWidth;
    DWORD m_dwRenderHeight;
    RECT m_rcScreenRect;
    unsigned char jpb_unknown_036[4];
    IDirectDraw7 *m_pDD;
    IDirect3D7 *m_pD3D;
    IDirect3DDevice7 *m_pd3dDevice;
    IDirectDrawSurface7 *m_pddsFrontBuffer;
    IDirectDrawSurface7 *m_pddsBackBuffer;
    IDirectDrawSurface7 *m_pddsZBuffer;
    DWORD m_dwDeviceMemType;
    unsigned char jpb_unknown_092[4];
};

static_assert(sizeof(CD3DFramework7) == 96,
              "CD3DFramework7 PDB size changed");
static_assert(offsetof(CD3DFramework7, m_pDD) == 40,
              "CD3DFramework7 DirectDraw offset changed");
static_assert(offsetof(CD3DFramework7, m_pddsFrontBuffer) == 64,
              "CD3DFramework7 front-buffer offset changed");

class CD3DFramework12 {
public:
    struct ConstantBuffer {
        float position[4];
    };

    CD3DFramework12();
    ~CD3DFramework12();

    SDL_Texture *LoadTextureTGA(
        SDL_Renderer *renderer, const char *filename);
    void Move(int x, int y);
    HRESULT Present();
    void RenderTexture(SDL_Renderer *renderer, SDL_Texture *texture);
    void RenderTextureInUI(
        SDL_Renderer *renderer,
        HBITMAP bitmap,
        int x,
        int y,
        int width,
        int height);
    void RenderUI();
    HRESULT CreateRenderTargetViews();
    HRESULT ResizeDepthBuffer();
    HRESULT ResizeResources(UINT new_width, UINT new_height);
    HRESULT DestroyObjects();
    std::uint64_t AllocateShaderResourceDescriptor();
    void FreeShaderResourceDescriptor(std::uint64_t descriptor);
    std::uint64_t GetShaderResourceDescriptorIndex(
        std::uint64_t descriptor);
    bool IsCommandListOpen();
    HRESULT TryCloseCommandList();

    ID3D12Device *GetDevice() const { return m_pDevice; }

    unsigned char jpb_unknown_000[8];
    HWND m_hWnd;
    BOOL m_bIsFullscreen;
    DWORD m_dwRenderWidth;
    DWORD m_dwRenderHeight;
    RECT m_rcScreenRect;
    ID3D12Device *m_pDevice;
    IDXGISwapChain3 *m_pSwapChain;
    ID3D12CommandQueue *m_pCommandQueue;
    UINT m_nCurrentFrameIndex;
    unsigned char jpb_unknown_076[4];
    ID3D12Resource *m_pRenderTargets[2];
    ID3D12CommandAllocator *m_pCommandAllocators[2];
    ID3D12GraphicsCommandList *m_pCommandList;
    ID3D12Fence *m_pFence[2];
    HANDLE m_hFenceEvent;
    std::uint64_t m_nFenceValues[2];
    IDXGIFactory6 *m_pFactory;
    ID3D12DescriptorHeap *m_pRtvDescriptorHeap;
    UINT m_RTVDescriptorSize;
    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;
    unsigned char jpb_unknown_220[4];
    ID3D12RootSignature *m_pRootSignature;
    ID3D12PipelineState *m_pPipelineState;
    ID3D12PipelineState *m_pLevelPipelineState;
    ID3D12PipelineState *m_pTransparentPipelineState;
    ID3D12PipelineState *m_pTransparentGlassPipelineState;
    DXGI_SWAP_CHAIN_DESC1 *m_pSwapChainDesc;
    ID3D12Resource *m_pDepthStencil;
    UINT m_nFrameIndex;
    unsigned char jpb_unknown_284[4];
    ID3D12DescriptorHeap *m_pMainDescriptorHeap;
    ID3D12DescriptorHeap *m_cbvHeap;
    UINT m_nDescriptorCount;
    unsigned char jpb_unknown_308[4];
    ID3D12DescriptorHeap *m_pDsDescriptorHeap;
    ID3D12Resource *m_vertexBuffer;
    ID3D12Resource *m_indexBuffer;
    ID3D12Resource *m_constantBuffer;
    ID3D12Resource *m_constantBufferUploadHeaps[2];
    void *m_mappedConstantBuffer[2];
    unsigned char *m_pCbvDataBegin;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    ConstantBuffer m_cbData;
    ID3D12Resource *m_vBufferUploadHeap;
    ID3D12Resource *m_3DVertexBuffer;
    ID3D12Resource *m_3DIndexBuffer;
    UINT m_vertexBufferSize;
    UINT m_indexBufferSize;
    ID3D12Resource *m_vertexUploadBuffer;
    ID3D12Resource *m_indexUploadBuffer;
    IUnknown *vertexShaderBlob;
    IUnknown *pixelShaderBlob;
    SDL_Window *m_pSDLWindow;
    SDL_Renderer *m_pSDLRenderer;
    SDL_Texture *m_pSDLRenderTarget;
    bool inMenu;
    unsigned char jpb_unknown_521[7];
    ID3D12Resource *PAIN;
    ID3D12Resource *PAINTEX;
    D3D12_VERTEX_BUFFER_VIEW m_levelVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_levelIndexBufferView;
    ID3D12Resource *m_levelVertexBuffer;
    ID3D12Resource *m_levelIndexBuffer;
    void *m_graphicsMemory;
    JpbPdbVectorUint64 m_freeShaderResourceDescriptors;
    BOOL m_isAMD;
    BOOL m_isSteamDeck;
    bool m_commandListOpen;
    unsigned char jpb_unknown_633[7];
};

static_assert(sizeof(CD3DFramework12) == 640,
              "CD3DFramework12 PDB size changed");
static_assert(offsetof(CD3DFramework12, m_hWnd) == 8,
              "CD3DFramework12 window-handle offset changed");
static_assert(offsetof(CD3DFramework12, m_rcScreenRect) == 28,
              "CD3DFramework12 screen-rectangle offset changed");
static_assert(offsetof(CD3DFramework12, m_pDevice) == 48,
              "CD3DFramework12 device offset changed");
static_assert(offsetof(CD3DFramework12, m_pCommandList) == 112,
              "CD3DFramework12 command-list offset changed");
static_assert(offsetof(CD3DFramework12, m_pFence) == 120,
              "CD3DFramework12 fence-array offset changed");
static_assert(offsetof(CD3DFramework12, m_nFrameIndex) == 280,
              "CD3DFramework12 frame-index offset changed");
static_assert(offsetof(CD3DFramework12, m_pRtvDescriptorHeap) == 168,
              "CD3DFramework12 RTV-heap offset changed");
static_assert(offsetof(CD3DFramework12, m_pPipelineState) == 232,
              "CD3DFramework12 pipeline-state offset changed");
static_assert(offsetof(CD3DFramework12, m_pDepthStencil) == 272,
              "CD3DFramework12 depth-stencil offset changed");
static_assert(offsetof(CD3DFramework12, m_pMainDescriptorHeap) == 288,
              "CD3DFramework12 descriptor-heap offset changed");
static_assert(offsetof(CD3DFramework12, m_nDescriptorCount) == 304,
              "CD3DFramework12 descriptor-count offset changed");
static_assert(offsetof(CD3DFramework12, m_pDsDescriptorHeap) == 312,
              "CD3DFramework12 DSV-heap offset changed");
static_assert(offsetof(CD3DFramework12, m_vertexBuffer) == 320,
              "CD3DFramework12 vertex-buffer offset changed");
static_assert(offsetof(CD3DFramework12, m_pCbvDataBegin) == 376,
              "CD3DFramework12 mapped-CBV offset changed");
static_assert(offsetof(CD3DFramework12, m_vertexBufferView) == 384,
              "CD3DFramework12 vertex-buffer-view offset changed");
static_assert(offsetof(CD3DFramework12, m_indexBufferView) == 400,
              "CD3DFramework12 index-buffer-view offset changed");
static_assert(offsetof(CD3DFramework12, m_cbData) == 416,
              "CD3DFramework12 constant-buffer data offset changed");
static_assert(offsetof(CD3DFramework12, m_vBufferUploadHeap) == 432,
              "CD3DFramework12 upload-buffer offset changed");
static_assert(offsetof(CD3DFramework12, vertexShaderBlob) == 480,
              "CD3DFramework12 vertex-shader blob offset changed");
static_assert(offsetof(CD3DFramework12, m_pSDLWindow) == 496,
              "CD3DFramework12 SDL-window offset changed");
static_assert(offsetof(CD3DFramework12, m_pSDLRenderer) == 504,
              "CD3DFramework12 SDL-renderer offset changed");
static_assert(offsetof(CD3DFramework12, inMenu) == 520,
              "CD3DFramework12 menu-state offset changed");
static_assert(offsetof(CD3DFramework12, PAIN) == 528,
              "CD3DFramework12 PAIN offset changed");
static_assert(offsetof(CD3DFramework12, PAINTEX) == 536,
              "CD3DFramework12 PAINTEX offset changed");
static_assert(offsetof(CD3DFramework12, m_levelVertexBufferView) == 544,
              "CD3DFramework12 level-vertex-view offset changed");
static_assert(offsetof(CD3DFramework12, m_levelIndexBufferView) == 560,
              "CD3DFramework12 level-index-view offset changed");
static_assert(offsetof(CD3DFramework12, m_levelVertexBuffer) == 576,
              "CD3DFramework12 level-vertex-buffer offset changed");
static_assert(
    offsetof(CD3DFramework12, m_freeShaderResourceDescriptors) == 600,
    "CD3DFramework12 free-descriptor offset changed");
static_assert(offsetof(CD3DFramework12, m_isAMD) == 624,
              "CD3DFramework12 AMD flag offset changed");
static_assert(offsetof(CD3DFramework12, m_isSteamDeck) == 628,
              "CD3DFramework12 Steam Deck flag offset changed");
static_assert(offsetof(CD3DFramework12, m_commandListOpen) == 632,
              "CD3DFramework12 command-list flag offset changed");

#endif
