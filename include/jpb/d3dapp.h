#ifndef JPB_D3DAPP_H
#define JPB_D3DAPP_H

#include <d3d.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "jpb/fmath.h"
#include "jpb/material.h"
#include "jpb/directxtk12_abi.h"
#include "jpb/d3dtransparencypass.h"

class CD3DFramework12;
class FontAtlas;
class Texture;
struct IDxcBlob;
struct SDL_Window;

namespace PHL {
class Texture2D;
}

struct alignas(16) SpriteDraw {
    Texture *Texture;
    std::optional<RECT> SrcRect;
    RECT DestRect;
    DirectX::XMVECTORF32 Color;
    DirectX::DX12::SpriteEffects Effects;
    DirectX::XMFLOAT2 Origin;
    float Rotation;
    float LayerDepth;
    std::optional<RECT> ScissorRect;
    TEXTURE_SAMPLE_TYPE SamplerType;
};

static_assert(sizeof(SpriteDraw) == 112, "SpriteDraw PDB size changed");
static_assert(alignof(SpriteDraw) == 16, "SpriteDraw PDB alignment changed");
static_assert(offsetof(SpriteDraw, Texture) == 0,
              "SpriteDraw.Texture offset changed");
static_assert(offsetof(SpriteDraw, SrcRect) == 8,
              "SpriteDraw.SrcRect offset changed");
static_assert(offsetof(SpriteDraw, DestRect) == 28,
              "SpriteDraw.DestRect offset changed");
static_assert(offsetof(SpriteDraw, Color) == 48,
              "SpriteDraw.Color offset changed");
static_assert(offsetof(SpriteDraw, Effects) == 64,
              "SpriteDraw.Effects offset changed");
static_assert(offsetof(SpriteDraw, Origin) == 68,
              "SpriteDraw.Origin offset changed");
static_assert(offsetof(SpriteDraw, Rotation) == 76,
              "SpriteDraw.Rotation offset changed");
static_assert(offsetof(SpriteDraw, LayerDepth) == 80,
              "SpriteDraw.LayerDepth offset changed");
static_assert(offsetof(SpriteDraw, ScissorRect) == 84,
              "SpriteDraw.ScissorRect offset changed");
static_assert(offsetof(SpriteDraw, SamplerType) == 104,
              "SpriteDraw.SamplerType offset changed");

/* Dependency-closed leading view of the matched-PDB application object. */
class CD3DApplication {
public:
    struct SubMeshSet {
        int baseVertexOffSet;
        unsigned char jpb_padding_004[4];
        std::vector<unsigned short> subMeshIndices;
        std::vector<Vertex> vertices;
    };

    struct FBX_MESH {
        DirectX::XMMATRIX worldMatrix;
        Vertex *vertices;
        unsigned short *indices;
        int vertexCount;
        int indexCount;
        int baseVertexIndex;
        unsigned char jpb_padding_05c[4];
        std::vector<SubMeshSet> subMeshes;
        const char *name;
    };

    struct ConstantBufferData {
        DirectX::XMMATRIX worldViewProjectionMatrix;
        DirectX::XMVECTOR levelScale;
        DirectX::XMFLOAT2 uvScrollSpeed;
        unsigned char jpb_padding_058[8];
        DirectX::XMVECTOR padding[10];
    };

    CD3DApplication();
    ~CD3DApplication();

    virtual HRESULT OneTimeSceneInit();
    virtual HRESULT InitDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
    virtual HRESULT RestoreSurfaces();
    virtual HRESULT Render(float interpolation);
    virtual HRESULT FinalCleanup();
    virtual void OnKeyDown(int key);
    virtual void OnKeyUp(int key);
    virtual void ShutDown();
    virtual std::int64_t OnQuerySuspend(unsigned long flags);
    virtual std::int64_t OnResumeSuspend(unsigned long data);
    virtual HRESULT Create(HINSTANCE instance, char *command_line);
    virtual int Run();
    virtual LRESULT MsgProc(
        HWND window,
        UINT message,
        WPARAM w_param,
        LPARAM l_param);
    virtual void Pause(int pause);
    virtual int ToggleFullScreen();

    void CapFrameRate();
    HRESULT Change3DEnvironment();
    void CheckGameBarInput(HWND window);
    void CheckGameBarVisibility(HWND window);
    void Cleanup3DEnvironment();
    DirectX::XMMATRIX ConvertMatrixToDXMatrix(MATRIX *matrix);
    unsigned long ConvertWindowSettingToFlags(int setting);
    HRESULT CreateFence();
    HRESULT CreateLevelPipelineStateObject();
    HRESULT CreatePipelineStateObject();
    HRESULT CreateSRVHeap();
    HRESULT CreateTransparentPipelineStateObject();
    void DisplayFrameworkError(HRESULT result, unsigned long type);
    void DestroyD3D12Objects();
    void DrawTexture(const SpriteDraw &sprite_draw);
    void DrawLevel(
        MATRIX *world_matrix,
        MATRIX *view_matrix,
        MATRIX *projection_matrix,
        int near_clip);
    void DrawLevelTransparent(
        MATRIX *world_matrix,
        MATRIX *view_matrix,
        MATRIX *projection_matrix,
        int near_clip);
    void DrawLevelTransparentGlass(
        MATRIX *world_matrix,
        MATRIX *view_matrix,
        MATRIX *projection_matrix,
        int near_clip);
    HRESULT EndRender();
    HRESULT FrameBegin();
    HRESULT FrameEnd();
    int GetRegistryBinary(char *name, void *data, unsigned size);
    unsigned long GetRegistryDWord(char *name, unsigned long default_value);
    int GetRegistryString(char *name, char *value, unsigned size);
    HRESULT InitD3D12Framework(HWND window);
    HRESULT InitializeCommandList();
    HRESULT InitializeDepthStencilBuffer();
    HRESULT InitializeDevice();
    HRESULT InitializeRenderTargets();
    HRESULT InitializeRootSignature();
    HRESULT InitializeSwapChain(HWND window);
    void InitializeViewportAndScissorRect();
    bool IsWindowed();
    void MessagePump();
    void MoveToNextFrame();
    void OutputText(char *text);
    void OutputTextXY(int x, int y, char *text);
    HRESULT Render3DEnvironment();
    void RenderMenuTexture(bool is_menu);
    HRESULT RenderUI();
    int RegCreate(HKEY *key);
    char *RegKeyName();
    int RegOpen(HKEY *key);
    void SelectTexture(
        unsigned index,
        ID3D12Resource *texture_resource,
        ID3D12DescriptorHeap *srv_heap);
    void SetAppTitle(char *title);
    void SetBitDepth(unsigned bits_per_pixel);
    void SetCompanyTitle(char *title);
    void SetFullScreen(int enabled);
    void SetRegistryBinary(char *name, void *data, unsigned size);
    void SetRegistryDWord(char *name, unsigned long value);
    void SetRegistryString(char *name, char *value);
    void SetRenderState(
        ID3D12Device *device,
        ID3D12PipelineState *pipeline_state,
        D3D12_BLEND source_blend,
        D3D12_BLEND destination_blend);
    void SetUseZBuffer(int enabled);
    void SetViewParams(
        D3DVECTOR *from,
        D3DVECTOR *at,
        D3DVECTOR *world_up,
        float field_of_view);
    void SetWidthHeight(unsigned width, unsigned height);
    void SetWindowFlags(unsigned long flags);
    HRESULT StartRender();
    void UpdateResolution(int width, int height, int window_mode);
    void WaitForGpu();
    void ERRTRACE(char *file, int line, char *format, ...);
    void SystemErrorReport(
        char *file,
        int line,
        unsigned long error,
        char *function);
    void debugtrace(char *format, ...);
    void debug_line(
        float x1, float y1, float z1,
        float x2, float y2, float z2,
        unsigned long color);
    void debug_point(
        float x, float y, float z, int width, unsigned long color);
    void debug_printf(char *format, ...);
    void svprintf(char *buffer, char *format, char *arguments);

    ID3D12Resource *m_texture;
    ID3D12Resource *textureUploadHeap;
    int m_bActive;
    int m_bReady;
    int m_bUseWarpDevice;
    int m_bFrameMoving;
    int m_bSingleStep;
    DWORD m_dwBaseTime;
    DWORD m_dwStopTime;
    int numTris;
    int numQuads;
    unsigned char jpb_padding_044[4];
    std::unique_ptr<DirectX::DX12::SpriteBatch> m_spriteBatch;
    std::unique_ptr<DirectX::DX12::CommonStates> m_states;
    std::vector<SpriteDraw> m_spriteDraws;
    PHL::Texture2D *m_pVidTex;
    CD3DFramework12 *m_pFramework;
    std::map<std::string, _Material *> m_LoadedTextures;
    std::unordered_map<std::string, _Material *> m_textureCacheMap;
    HWND m_hWnd;
    SDL_Window *m_pWindow;
    int m_bAppUseFullScreen;
    int m_bAppUseZBuffer;
    int m_bOldWindowedState;
    unsigned char jpb_padding_0ec[4];
    std::unique_ptr<D3DTransparencyPass> m_transparencyPass;
    std::unique_ptr<FontAtlas> m_fontAtlas;
    unsigned short triIndices[0xC000];
    unsigned short quadIndices[0x10000];
    int endPolyCallsPerFrame;
    int isFedMovie;
    char m_strCompanyName[256];
    char m_strAppName[256];
    char *m_strWindowTitle;
    void *m_fnConfirmDevice;
    DWORD m_dwWindowFlags;
    DWORD m_dwDefaultWidth;
    DWORD m_dwDefaultHeight;
    DWORD m_dwDefaultBitsPixel;
    DWORD m_dwScreenResW;
    DWORD m_dwScreenResH;
    DWORD m_dwSavedStyle;
    RECT m_rcSaved;
    RECT m_textrect;
    unsigned char jpb_padding_38364[4];
    HFONT courier;
    D3DMATRIX m_matView;
    Vertex triArray[0x6000];
    Vertex quadArray[0x8000];
    int totalOpaqueSubmeshes;
    int totalNonGlassTransparentSubmeshes;
    std::vector<FBX_MESH *> fbxLevelData;
    std::vector<FBX_MESH *> transparentMeshes;
    std::vector<FBX_MESH *> transparentGlassMeshes;
    DirectX::XMMATRIX m_fbxWorldMatrix;
    DirectX::XMMATRIX m_fbxProjectionMatrix;
    ConstantBufferData m_sceneConstantBufferData;
    unsigned char m_bKeyMap[256];
    unsigned char m_bRawKeyMap[256];
    unsigned char m_bKeyMapPressed[256];
    unsigned char m_bKeyMapReleased[256];
    int m_nLastKey;
    unsigned char jpb_padding_310974[12];
};

extern CD3DApplication *g_pD3DApp;
extern LARGE_INTEGER gFrequency;
extern int gQueryResult;
extern double gFramesPerSecond;
extern double gSecondsPerFrame;

void jpb_D3DAppGetSDLWindowSize(
    SDL_Window *window, int *width, int *height);
int jpb_D3DAppSetSDLWindowFullscreen(
    SDL_Window *window, std::uint32_t flags);
void *jpb_D3DAppSDLMalloc(std::size_t size);
void jpb_D3DAppSDLFree(void *memory);

#ifdef JPB_D3DAPP_TESTING
struct JPBD3DAppCreateTestHooks {
    int (*sdl_init)(std::uint32_t flags);
    const char *(*sdl_get_error)();
    int (*img_init)(int flags);
    int (*ttf_init)();
    void *(*sdl_create_window)(
        const char *title,
        int x,
        int y,
        int width,
        int height,
        std::uint32_t flags);
    void *(*sdl_create_renderer)(
        void *window, int index, std::uint32_t flags);
    void *(*sdl_create_texture)(
        void *renderer,
        std::uint32_t format,
        int access,
        int width,
        int height);
    int (*sdl_set_texture_blend_mode)(void *texture, int mode);
    int (*sdl_set_render_target)(void *renderer, void *texture);
    int (*sdl_set_render_draw_color)(
        void *renderer,
        std::uint8_t red,
        std::uint8_t green,
        std::uint8_t blue,
        std::uint8_t alpha);
    int (*sdl_render_clear)(void *renderer);
    int (*sdl_set_render_draw_blend_mode)(void *renderer, int mode);
    int (*sdl_get_window_handle)(void *window, HWND *native_window);
    HRESULT (*initialize_game_bar)(CD3DApplication *application);
    HRESULT (*initialize_framework)(
        CD3DApplication *application, HWND window);
    HRESULT (*initialize_textures)(CD3DApplication *application);
};
void jpb_d3dapp_set_create_test_hooks(
    const JPBD3DAppCreateTestHooks *hooks);
using JPBCreateDXGIFactory2TestHook = HRESULT (WINAPI *)(
    UINT flags, REFIID iid, void **factory);
void jpb_d3dapp_set_create_dxgi_factory_test_hook(
    JPBCreateDXGIFactory2TestHook hook);
using JPBFrameworkDestroyObjectsTestHook = HRESULT (*)(
    CD3DFramework12 *framework);
void jpb_d3dapp_set_framework_destroy_objects_test_hook(
    JPBFrameworkDestroyObjectsTestHook hook);
using JPBD3D12CreateDeviceTestHook = HRESULT (WINAPI *)(
    IUnknown *adapter,
    D3D_FEATURE_LEVEL minimum_feature_level,
    REFIID iid,
    void **device);
void jpb_d3dapp_set_create_device_test_hook(
    JPBD3D12CreateDeviceTestHook hook);
using JPBD3D12SerializeRootSignatureTestHook = HRESULT (WINAPI *)(
    const D3D12_ROOT_SIGNATURE_DESC *description,
    D3D_ROOT_SIGNATURE_VERSION version,
    ID3DBlob **signature,
    ID3DBlob **error);
void jpb_d3dapp_set_serialize_root_signature_test_hook(
    JPBD3D12SerializeRootSignatureTestHook hook);
using JPBResizeResourcesTestHook = HRESULT (*)(
    CD3DFramework12 *framework, UINT width, UINT height);
void jpb_d3dapp_set_resize_resources_test_hook(
    JPBResizeResourcesTestHook hook);
using JPBGraphicsMemoryCommitTestHook = void (*)(
    void *graphics_memory, ID3D12CommandQueue *command_queue);
void jpb_d3dapp_set_graphics_memory_commit_test_hook(
    JPBGraphicsMemoryCommitTestHook hook);
using JPBDebugMaterialTestHook = void (*)(
    D3DMATERIAL7 &material,
    float red,
    float green,
    float blue,
    float alpha);
void jpb_d3dapp_set_debug_material_test_hook(
    JPBDebugMaterialTestHook hook);
using JPBMessageBoxATestHook = int (WINAPI *)(
    HWND window,
    LPCSTR text,
    LPCSTR caption,
    UINT type);
void jpb_d3dapp_set_message_box_test_hook(
    JPBMessageBoxATestHook hook);
using JPBGameBarQueryTestHook = HRESULT (*)(
    int input_redirected, unsigned char *value);
void jpb_d3dapp_set_game_bar_query_test_hook(
    JPBGameBarQueryTestHook hook);
using JPBInvalidateRectTestHook = BOOL (WINAPI *)(
    HWND window, const RECT *rect, BOOL erase);
void jpb_d3dapp_set_invalidate_rect_test_hook(
    JPBInvalidateRectTestHook hook);
using JPBPauseMenuTestHook = void (*)();
void jpb_d3dapp_set_pause_menu_test_hook(JPBPauseMenuTestHook hook);
using JPBCompileShaderTestHook = bool (*)(
    const char *resource_name,
    const wchar_t **arguments,
    UINT32 argument_count,
    IDxcBlob **shader);
void jpb_d3dapp_set_compile_shader_test_hook(
    JPBCompileShaderTestHook hook);
using JPBSteamDeckTestHook = bool (*)();
void jpb_d3dapp_set_steam_deck_test_hook(JPBSteamDeckTestHook hook);
using JPBWaitForGpuTestHook = void (*)();
void jpb_d3dapp_set_wait_for_gpu_test_hook(JPBWaitForGpuTestHook hook);
using JPBSDLGetWindowFlagsTestHook = std::uint32_t (*)(void *window);
void jpb_d3dapp_set_sdl_get_window_flags_test_hook(
    JPBSDLGetWindowFlagsTestHook hook);
using JPBSDLGetWindowSizeTestHook = void (*)(
    void *window, int *width, int *height);
void jpb_d3dapp_set_sdl_get_window_size_test_hook(
    JPBSDLGetWindowSizeTestHook hook);
using JPBSDLSetWindowFullscreenTestHook = int (*)(
    void *window, std::uint32_t flags);
void jpb_d3dapp_set_sdl_set_window_fullscreen_test_hook(
    JPBSDLSetWindowFullscreenTestHook hook);
using JPBSDLMallocTestHook = void *(*)(std::size_t size);
void jpb_d3dapp_set_sdl_malloc_test_hook(JPBSDLMallocTestHook hook);
using JPBSDLFreeTestHook = void (*)(void *memory);
void jpb_d3dapp_set_sdl_free_test_hook(JPBSDLFreeTestHook hook);
using JPBSDLDestroyWindowTestHook = void (*)(void *window);
void jpb_d3dapp_set_sdl_destroy_window_test_hook(
    JPBSDLDestroyWindowTestHook hook);
using JPBSDLQuitTestHook = void (*)();
void jpb_d3dapp_set_sdl_quit_test_hook(JPBSDLQuitTestHook hook);
using JPBSDLQueryTextureTestHook = int (*)(
    void *texture,
    std::uint32_t *format,
    int *access,
    int *width,
    int *height);
void jpb_d3dapp_set_sdl_query_texture_test_hook(
    JPBSDLQueryTextureTestHook hook);
using JPBSDLRenderPresentTestHook = void (*)(void *renderer);
void jpb_d3dapp_set_sdl_render_present_test_hook(
    JPBSDLRenderPresentTestHook hook);
using JPBSDLRenderReadPixelsTestHook = int (*)(
    void *renderer,
    const void *rect,
    std::uint32_t format,
    void *pixels,
    int pitch);
void jpb_d3dapp_set_sdl_render_read_pixels_test_hook(
    JPBSDLRenderReadPixelsTestHook hook);
void jpb_d3dapp_sort_sprite_draws_for_test(
    SpriteDraw *draws, std::size_t count);
void jpb_d3dapp_initialize_scene_buffer_for_test(
    CD3DApplication *application);
void jpb_d3dapp_set_game_bar_state_for_test(
    unsigned char visible, unsigned char input_redirected);
unsigned char jpb_d3dapp_get_game_bar_visible_for_test();
unsigned char jpb_d3dapp_get_game_bar_input_redirected_for_test();
#endif

void TransitionResource(
    ID3D12GraphicsCommandList *command_list,
    ID3D12Resource *resource,
    D3D12_RESOURCE_STATES state_before,
    D3D12_RESOURCE_STATES state_after);

static_assert(offsetof(CD3DApplication, m_pFramework) == 120,
              "CD3DApplication framework offset changed");
static_assert(sizeof(CD3DApplication) == 0x310980,
              "CD3DApplication PDB size changed");
static_assert(offsetof(CD3DApplication, m_texture) == 16,
              "CD3DApplication texture offset changed");
static_assert(offsetof(CD3DApplication, m_LoadedTextures) == 128,
              "CD3DApplication loaded-texture map offset changed");
static_assert(offsetof(CD3DApplication, triIndices) == 0x100,
              "CD3DApplication triangle-index offset changed");
static_assert(offsetof(CD3DApplication, quadIndices) == 0x18100,
              "CD3DApplication quad-index offset changed");
static_assert(offsetof(CD3DApplication, triArray) == 0x383A0,
              "CD3DApplication triangle-vertex offset changed");
static_assert(offsetof(CD3DApplication, quadArray) == 0x1703A0,
              "CD3DApplication quad-vertex offset changed");
static_assert(offsetof(CD3DApplication, fbxLevelData) == 0x3103A8,
              "CD3DApplication FBX-level vector offset changed");
static_assert(offsetof(CD3DApplication, m_bKeyMap) == 0x310570,
              "CD3DApplication key-map offset changed");
static_assert(offsetof(CD3DApplication, m_nLastKey) == 0x310970,
              "CD3DApplication last-key offset changed");
static_assert(sizeof(CD3DApplication::SubMeshSet) == 56,
              "CD3DApplication::SubMeshSet PDB size changed");
static_assert(offsetof(CD3DApplication::SubMeshSet, subMeshIndices) == 8,
              "SubMeshSet index-vector offset changed");
static_assert(offsetof(CD3DApplication::SubMeshSet, vertices) == 32,
              "SubMeshSet vertex-vector offset changed");
static_assert(sizeof(CD3DApplication::FBX_MESH) == 128,
              "CD3DApplication::FBX_MESH PDB size changed");
static_assert(offsetof(CD3DApplication::FBX_MESH, subMeshes) == 96,
              "FBX_MESH submesh-vector offset changed");
static_assert(offsetof(CD3DApplication::FBX_MESH, name) == 120,
              "FBX_MESH name offset changed");
static_assert(sizeof(CD3DApplication::ConstantBufferData) == 256,
              "ConstantBufferData PDB size changed");
static_assert(offsetof(CD3DApplication::ConstantBufferData, uvScrollSpeed) == 80,
              "ConstantBufferData UV-scroll offset changed");

#endif
