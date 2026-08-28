/* COMPLETE REVIEWED RECONSTRUCTION. */

#include "jpb/d3dapp.h"
#include "jpb/d3denum.h"
#include "jpb/d3dframe.h"
#include "jpb/d3dtextr.h"
#include "jpb/d3dtransparencypass.h"
#include "jpb/d3dutil.h"
#include "jpb/cube.h"
#include "jpb/font_atlas.h"
#include "jpb/level.h"
#include "jpb/level_world.h"
#include "jpb/resources.h"
#include "jpb/steam_interfaces.h"
#include "jpb/texture.h"
#include "jpb/texture2d.h"
#include "jpb/whook.h"

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cfloat>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <memory>
#include <new>
#include <vector>
#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.h>
#include <windows.gaming.ui.h>

extern "C" void menu_enterPauseMode(void);
char getasciicodefromvirtualkey(
    int w_param,
    int l_param,
    unsigned char *raw_key_map,
    int press);

CD3DApplication *g_pD3DApp;
double gFramesPerSecond = 60.0;
LARGE_INTEGER gFrequency;
static LARGE_INTEGER lastTime;
static unsigned long dwAppPausedCount;
static int beenResized;
static RECT originalwinrect;
static char mainkeyname[256];
static char other;
unsigned char g_isVisible;
unsigned char g_isInputRedirected;
Microsoft::WRL::ComPtr<ABI::Windows::Gaming::UI::IGameBarStatics>
    g_gameBarStatics = {};
_Material *atlasHandle;

namespace {

void SortCanonicalSpriteDraws(SpriteDraw *begin, SpriteDraw *end)
{
    std::sort(begin, end, [](const SpriteDraw &left,
                             const SpriteDraw &right) {
        return left.LayerDepth < right.LayerDepth;
    });
}

void PrepareCanonicalLevelDraw(
    CD3DApplication *application,
    ID3D12PipelineState *pipeline_state,
    bool bind_cbv_descriptor,
    bool update_uv_scroll)
{
    auto *bytes = reinterpret_cast<unsigned char *>(application);
    CD3DFramework12 *framework = application->m_pFramework;
    ID3D12GraphicsCommandList *command_list = framework->m_pCommandList;

    if (update_uv_scroll) {
        auto *constant_data = reinterpret_cast<
            CD3DApplication::ConstantBufferData *>(bytes + 0x310470);
        constant_data->uvScrollSpeed = DirectX::XMFLOAT2(
            g_levelUVScroll.vx, g_levelUVScroll.vy);
    }

    command_list->SetGraphicsRootSignature(framework->m_pRootSignature);
    ID3D12DescriptorHeap *descriptor_heap = framework->m_cbvHeap;
    command_list->SetDescriptorHeaps(1, &descriptor_heap);
    if (bind_cbv_descriptor) {
        command_list->SetGraphicsRootDescriptorTable(
            1,
            framework->m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
    }
    descriptor_heap = framework->m_pMainDescriptorHeap;
    command_list->SetDescriptorHeaps(1, &descriptor_heap);
    command_list->SetGraphicsRootDescriptorTable(
        1,
        framework->m_pMainDescriptorHeap
            ->GetGPUDescriptorHandleForHeapStart());
    command_list->IASetVertexBuffers(
        0, 1, &framework->m_levelVertexBufferView);
    command_list->IASetIndexBuffer(&framework->m_levelIndexBufferView);
    command_list->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const auto *world = reinterpret_cast<const DirectX::XMMATRIX *>(
        bytes + 0x3103F0);
    const auto *projection = reinterpret_cast<const DirectX::XMMATRIX *>(
        bytes + 0x310430);
    auto *constant_data = reinterpret_cast<
        CD3DApplication::ConstantBufferData *>(bytes + 0x310470);
    constant_data->worldViewProjectionMatrix = DirectX::XMMatrixTranspose(
        DirectX::XMMatrixMultiply(*world, *projection));
    std::memcpy(
        framework->m_pCbvDataBegin,
        constant_data,
        sizeof(*constant_data));
    command_list->SetGraphicsRootConstantBufferView(
        0, framework->m_constantBuffer->GetGPUVirtualAddress());
    command_list->SetPipelineState(pipeline_state);
}

using SDLGetWindowFlagsFunction = std::uint32_t (*)(void *window);
using SDLGetWindowSizeFunction = void (*)(
    void *window, int *width, int *height);
using SDLInitFunction = int (*)(std::uint32_t flags);
using SDLGetErrorFunction = const char *(*)();
using IMGInitFunction = int (*)(int flags);
using TTFInitFunction = int (*)();
using SDLCreateWindowFunction = void *(*)(
    const char *title,
    int x,
    int y,
    int width,
    int height,
    std::uint32_t flags);
using SDLCreateRendererFunction = void *(*)(
    void *window, int index, std::uint32_t flags);
using SDLCreateTextureFunction = void *(*)(
    void *renderer,
    std::uint32_t format,
    int access,
    int width,
    int height);
using SDLSetTextureBlendModeFunction = int (*)(void *texture, int mode);
using SDLSetRenderTargetFunction = int (*)(void *renderer, void *texture);
using SDLSetRenderDrawColorFunction = int (*)(
    void *renderer,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue,
    std::uint8_t alpha);
using SDLRenderClearFunction = int (*)(void *renderer);
using SDLSetRenderDrawBlendModeFunction = int (*)(
    void *renderer, int mode);
using SDLDestroyWindowFunction = void (*)(void *window);
using SDLQuitFunction = void (*)();
using SDLQueryTextureFunction = int (*)(
    void *texture,
    std::uint32_t *format,
    int *access,
    int *width,
    int *height);
using SDLRenderPresentFunction = void (*)(void *renderer);
using SDLRenderReadPixelsFunction = int (*)(
    void *renderer,
    const void *rect,
    std::uint32_t format,
    void *pixels,
    int pitch);
using SDLShowCursorFunction = int (*)(int toggle);
using SDLSetWindowGrabFunction = void (*)(void *window, int grabbed);
using SDLSetWindowFullscreenFunction = int (*)(
    void *window, std::uint32_t flags);
using SDLMallocFunction = void *(*)(std::size_t size);
using SDLFreeFunction = void (*)(void *memory);
using SDLSetWindowBorderedFunction = void (*)(void *window, int bordered);
using SDLGetWindowDisplayIndexFunction = int (*)(void *window);

struct CanonicalSDLDisplayMode {
    std::uint32_t format;
    int width;
    int height;
    int refresh_rate;
    void *driver_data;
};

struct CanonicalSDLRect {
    int x;
    int y;
    int width;
    int height;
};

struct CanonicalSDLSysWMinfo {
    std::uint8_t major;
    std::uint8_t minor;
    std::uint8_t patch;
    std::uint8_t padding;
    std::uint32_t subsystem;
    HWND window;
    unsigned char remaining[112];
};

using SDLGetCurrentDisplayModeFunction = int (*)(
    int display_index, CanonicalSDLDisplayMode *mode);
using SDLGetDisplayBoundsFunction = int (*)(
    int display_index, CanonicalSDLRect *bounds);
using SDLSetWindowSizeFunction = void (*)(void *window, int width, int height);
using SDLSetWindowPositionFunction = void (*)(void *window, int x, int y);
using SDLGetWindowWMInfoFunction = int (*)(
    void *window, CanonicalSDLSysWMinfo *information);
using TimeGetTimeFunction = DWORD (WINAPI *)();

CanonicalSDLDisplayMode canonical_display_mode;
CanonicalSDLRect canonical_display_bounds;

#ifdef JPB_D3DAPP_TESTING
const JPBD3DAppCreateTestHooks *create_test_hooks = nullptr;
JPBCreateDXGIFactory2TestHook create_dxgi_factory_test_hook = nullptr;
JPBFrameworkDestroyObjectsTestHook
    framework_destroy_objects_test_hook = nullptr;
JPBD3D12CreateDeviceTestHook d3d12_create_device_test_hook = nullptr;
JPBD3D12SerializeRootSignatureTestHook
    d3d12_serialize_root_signature_test_hook = nullptr;
JPBResizeResourcesTestHook resize_resources_test_hook = nullptr;
JPBGraphicsMemoryCommitTestHook graphics_memory_commit_test_hook = nullptr;
JPBDebugMaterialTestHook debug_material_test_hook = nullptr;
JPBMessageBoxATestHook message_box_test_hook = nullptr;
JPBGameBarQueryTestHook game_bar_query_test_hook = nullptr;
JPBInvalidateRectTestHook invalidate_rect_test_hook = nullptr;
JPBPauseMenuTestHook pause_menu_test_hook = nullptr;
JPBCompileShaderTestHook compile_shader_test_hook = nullptr;
JPBSteamDeckTestHook steam_deck_test_hook = nullptr;
JPBWaitForGpuTestHook wait_for_gpu_test_hook = nullptr;
JPBSDLGetWindowFlagsTestHook sdl_get_window_flags_test_hook = nullptr;
JPBSDLGetWindowSizeTestHook sdl_get_window_size_test_hook = nullptr;
JPBSDLSetWindowFullscreenTestHook
    sdl_set_window_fullscreen_test_hook = nullptr;
JPBSDLMallocTestHook sdl_malloc_test_hook = nullptr;
JPBSDLFreeTestHook sdl_free_test_hook = nullptr;
JPBSDLDestroyWindowTestHook sdl_destroy_window_test_hook = nullptr;
JPBSDLQuitTestHook sdl_quit_test_hook = nullptr;
JPBSDLQueryTextureTestHook sdl_query_texture_test_hook = nullptr;
JPBSDLRenderPresentTestHook sdl_render_present_test_hook = nullptr;
JPBSDLRenderReadPixelsTestHook
    sdl_render_read_pixels_test_hook = nullptr;
#endif

HRESULT CreateCanonicalDXGIFactory2(
    UINT flags, REFIID iid, void **factory)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_dxgi_factory_test_hook != nullptr) {
        return create_dxgi_factory_test_hook(flags, iid, factory);
    }
#endif
    return CreateDXGIFactory2(flags, iid, factory);
}

HRESULT DestroyCanonicalFrameworkObjects(CD3DFramework12 *framework)
{
#ifdef JPB_D3DAPP_TESTING
    if (framework_destroy_objects_test_hook != nullptr) {
        return framework_destroy_objects_test_hook(framework);
    }
#endif
    return framework->DestroyObjects();
}

HRESULT QueryCanonicalGameBarState(
    bool input_redirected, unsigned char *value)
{
#ifdef JPB_D3DAPP_TESTING
    if (game_bar_query_test_hook != nullptr) {
        return game_bar_query_test_hook(input_redirected ? 1 : 0, value);
    }
#endif
    if (input_redirected) {
        return g_gameBarStatics->get_IsInputRedirected(
            reinterpret_cast<boolean *>(value));
    }
    return g_gameBarStatics->get_Visible(
        reinterpret_cast<boolean *>(value));
}

BOOL InvalidateCanonicalRect(HWND window, const RECT *rect, BOOL erase)
{
#ifdef JPB_D3DAPP_TESTING
    if (invalidate_rect_test_hook != nullptr) {
        return invalidate_rect_test_hook(window, rect, erase);
    }
#endif
    return InvalidateRect(window, rect, erase);
}

void EnterCanonicalPauseMode()
{
#ifdef JPB_D3DAPP_TESTING
    if (pause_menu_test_hook != nullptr) {
        pause_menu_test_hook();
        return;
    }
#endif
    menu_enterPauseMode();
}

HRESULT CreateCanonicalD3D12Device(
    IUnknown *adapter,
    D3D_FEATURE_LEVEL minimum_feature_level,
    REFIID iid,
    void **device)
{
#ifdef JPB_D3DAPP_TESTING
    if (d3d12_create_device_test_hook != nullptr) {
        return d3d12_create_device_test_hook(
            adapter, minimum_feature_level, iid, device);
    }
#endif
    return D3D12CreateDevice(
        adapter, minimum_feature_level, iid, device);
}

HRESULT SerializeCanonicalRootSignature(
    const D3D12_ROOT_SIGNATURE_DESC *description,
    D3D_ROOT_SIGNATURE_VERSION version,
    ID3DBlob **signature,
    ID3DBlob **error)
{
#ifdef JPB_D3DAPP_TESTING
    if (d3d12_serialize_root_signature_test_hook != nullptr) {
        return d3d12_serialize_root_signature_test_hook(
            description, version, signature, error);
    }
#endif
    return D3D12SerializeRootSignature(
        description, version, signature, error);
}

HRESULT ResizeCanonicalResources(
    CD3DFramework12 *framework, UINT width, UINT height)
{
#ifdef JPB_D3DAPP_TESTING
    if (resize_resources_test_hook != nullptr) {
        return resize_resources_test_hook(framework, width, height);
    }
#endif
    return framework->ResizeResources(width, height);
}

void CommitCanonicalGraphicsMemory(
    void *graphics_memory, ID3D12CommandQueue *command_queue)
{
#ifdef JPB_D3DAPP_TESTING
    if (graphics_memory_commit_test_hook != nullptr) {
        graphics_memory_commit_test_hook(graphics_memory, command_queue);
        return;
    }
#endif
    static_cast<DirectX::DX12::GraphicsMemory *>(graphics_memory)->Commit(
        command_queue);
}

void InitializeCanonicalDebugMaterial(
    D3DMATERIAL7 &material,
    float red,
    float green,
    float blue,
    float alpha)
{
#ifdef JPB_D3DAPP_TESTING
    if (debug_material_test_hook != nullptr) {
        debug_material_test_hook(material, red, green, blue, alpha);
        return;
    }
#endif
    D3DUtil_InitMaterial(material, red, green, blue, alpha);
}

void CopyCanonicalDebugFormat(char *destination, const char *format)
{
    int length = 0;
    while (*format != '\0' && length < 255) {
        destination[length++] = *format++;
    }
    destination[length] = '\0';
}

int ShowCanonicalMessageBox(
    HWND window, LPCSTR text, LPCSTR caption, UINT type)
{
#ifdef JPB_D3DAPP_TESTING
    if (message_box_test_hook != nullptr) {
        return message_box_test_hook(window, text, caption, type);
    }
#endif
    return MessageBoxA(window, text, caption, type);
}

HMODULE GetCanonicalSDLModule()
{
    static HMODULE module = LoadLibraryA("SDL2.dll");
    if (module == nullptr) {
        RaiseFailFastException(nullptr, nullptr, 0);
    }
    return module;
}

HMODULE GetCanonicalSDLImageModule()
{
    static HMODULE module = LoadLibraryA("SDL2_image.dll");
    if (module == nullptr) {
        RaiseFailFastException(nullptr, nullptr, 0);
    }
    return module;
}

HMODULE GetCanonicalSDLTTFModule()
{
    static HMODULE module = LoadLibraryA("SDL2_ttf.dll");
    if (module == nullptr) {
        RaiseFailFastException(nullptr, nullptr, 0);
    }
    return module;
}

template <typename Function>
Function GetCanonicalSDLProcedure(const char *name)
{
    FARPROC procedure = GetProcAddress(GetCanonicalSDLModule(), name);
    if (procedure == nullptr) {
        RaiseFailFastException(nullptr, nullptr, 0);
    }
    return reinterpret_cast<Function>(procedure);
}

template <typename Function>
Function GetCanonicalModuleProcedure(HMODULE module, const char *name)
{
    FARPROC procedure = GetProcAddress(module, name);
    if (procedure == nullptr) {
        RaiseFailFastException(nullptr, nullptr, 0);
    }
    return reinterpret_cast<Function>(procedure);
}

SDLInitFunction GetCanonicalSDLInit()
{
    static SDLInitFunction function =
        GetCanonicalSDLProcedure<SDLInitFunction>("SDL_Init");
    return function;
}

SDLGetErrorFunction GetCanonicalSDLGetError()
{
    static SDLGetErrorFunction function =
        GetCanonicalSDLProcedure<SDLGetErrorFunction>("SDL_GetError");
    return function;
}

IMGInitFunction GetCanonicalIMGInit()
{
    static IMGInitFunction function =
        GetCanonicalModuleProcedure<IMGInitFunction>(
            GetCanonicalSDLImageModule(), "IMG_Init");
    return function;
}

TTFInitFunction GetCanonicalTTFInit()
{
    static TTFInitFunction function =
        GetCanonicalModuleProcedure<TTFInitFunction>(
            GetCanonicalSDLTTFModule(), "TTF_Init");
    return function;
}

SDLCreateWindowFunction GetCanonicalSDLCreateWindow()
{
    static SDLCreateWindowFunction function =
        GetCanonicalSDLProcedure<SDLCreateWindowFunction>(
            "SDL_CreateWindow");
    return function;
}

SDLCreateRendererFunction GetCanonicalSDLCreateRenderer()
{
    static SDLCreateRendererFunction function =
        GetCanonicalSDLProcedure<SDLCreateRendererFunction>(
            "SDL_CreateRenderer");
    return function;
}

SDLCreateTextureFunction GetCanonicalSDLCreateTexture()
{
    static SDLCreateTextureFunction function =
        GetCanonicalSDLProcedure<SDLCreateTextureFunction>(
            "SDL_CreateTexture");
    return function;
}

SDLSetTextureBlendModeFunction GetCanonicalSDLSetTextureBlendMode()
{
    static SDLSetTextureBlendModeFunction function =
        GetCanonicalSDLProcedure<SDLSetTextureBlendModeFunction>(
            "SDL_SetTextureBlendMode");
    return function;
}

SDLSetRenderTargetFunction GetCanonicalSDLSetRenderTarget()
{
    static SDLSetRenderTargetFunction function =
        GetCanonicalSDLProcedure<SDLSetRenderTargetFunction>(
            "SDL_SetRenderTarget");
    return function;
}

SDLSetRenderDrawColorFunction GetCanonicalSDLSetRenderDrawColor()
{
    static SDLSetRenderDrawColorFunction function =
        GetCanonicalSDLProcedure<SDLSetRenderDrawColorFunction>(
            "SDL_SetRenderDrawColor");
    return function;
}

SDLRenderClearFunction GetCanonicalSDLRenderClear()
{
    static SDLRenderClearFunction function =
        GetCanonicalSDLProcedure<SDLRenderClearFunction>(
            "SDL_RenderClear");
    return function;
}

SDLSetRenderDrawBlendModeFunction GetCanonicalSDLSetRenderDrawBlendMode()
{
    static SDLSetRenderDrawBlendModeFunction function =
        GetCanonicalSDLProcedure<SDLSetRenderDrawBlendModeFunction>(
            "SDL_SetRenderDrawBlendMode");
    return function;
}

SDLGetWindowFlagsFunction GetCanonicalSDLGetWindowFlags()
{
    static SDLGetWindowFlagsFunction function =
        GetCanonicalSDLProcedure<SDLGetWindowFlagsFunction>(
            "SDL_GetWindowFlags");
    return function;
}

SDLGetWindowSizeFunction GetCanonicalSDLGetWindowSize()
{
    static SDLGetWindowSizeFunction function =
        GetCanonicalSDLProcedure<SDLGetWindowSizeFunction>(
            "SDL_GetWindowSize");
    return function;
}

SDLDestroyWindowFunction GetCanonicalSDLDestroyWindow()
{
    static SDLDestroyWindowFunction function =
        GetCanonicalSDLProcedure<SDLDestroyWindowFunction>(
            "SDL_DestroyWindow");
    return function;
}

SDLQuitFunction GetCanonicalSDLQuit()
{
    static SDLQuitFunction function =
        GetCanonicalSDLProcedure<SDLQuitFunction>("SDL_Quit");
    return function;
}

SDLQueryTextureFunction GetCanonicalSDLQueryTexture()
{
    static SDLQueryTextureFunction function =
        GetCanonicalSDLProcedure<SDLQueryTextureFunction>(
            "SDL_QueryTexture");
    return function;
}

SDLRenderPresentFunction GetCanonicalSDLRenderPresent()
{
    static SDLRenderPresentFunction function =
        GetCanonicalSDLProcedure<SDLRenderPresentFunction>(
            "SDL_RenderPresent");
    return function;
}

SDLRenderReadPixelsFunction GetCanonicalSDLRenderReadPixels()
{
    static SDLRenderReadPixelsFunction function =
        GetCanonicalSDLProcedure<SDLRenderReadPixelsFunction>(
            "SDL_RenderReadPixels");
    return function;
}

int QueryCanonicalSDLTexture(
    void *texture,
    std::uint32_t *format,
    int *access,
    int *width,
    int *height)
{
#ifdef JPB_D3DAPP_TESTING
    if (sdl_query_texture_test_hook != nullptr) {
        return sdl_query_texture_test_hook(
            texture, format, access, width, height);
    }
#endif
    return GetCanonicalSDLQueryTexture()(
        texture, format, access, width, height);
}

void PresentCanonicalSDLRenderer(void *renderer)
{
#ifdef JPB_D3DAPP_TESTING
    if (sdl_render_present_test_hook != nullptr) {
        sdl_render_present_test_hook(renderer);
        return;
    }
#endif
    GetCanonicalSDLRenderPresent()(renderer);
}

int ReadCanonicalSDLRendererPixels(
    void *renderer,
    const void *rect,
    std::uint32_t format,
    void *pixels,
    int pitch)
{
#ifdef JPB_D3DAPP_TESTING
    if (sdl_render_read_pixels_test_hook != nullptr) {
        return sdl_render_read_pixels_test_hook(
            renderer, rect, format, pixels, pitch);
    }
#endif
    return GetCanonicalSDLRenderReadPixels()(
        renderer, rect, format, pixels, pitch);
}

std::uint32_t GetCanonicalSDLWindowFlags(void *window)
{
#ifdef JPB_D3DAPP_TESTING
    if (sdl_get_window_flags_test_hook != nullptr) {
        return sdl_get_window_flags_test_hook(window);
    }
#endif
    return GetCanonicalSDLGetWindowFlags()(window);
}

void DestroyCanonicalSDLWindow(void *window)
{
#ifdef JPB_D3DAPP_TESTING
    if (sdl_destroy_window_test_hook != nullptr) {
        sdl_destroy_window_test_hook(window);
        return;
    }
#endif
    GetCanonicalSDLDestroyWindow()(window);
}

void QuitCanonicalSDL()
{
#ifdef JPB_D3DAPP_TESTING
    if (sdl_quit_test_hook != nullptr) {
        sdl_quit_test_hook();
        return;
    }
#endif
    GetCanonicalSDLQuit()();
}

SDLShowCursorFunction GetCanonicalSDLShowCursor()
{
    static SDLShowCursorFunction function =
        GetCanonicalSDLProcedure<SDLShowCursorFunction>("SDL_ShowCursor");
    return function;
}

SDLSetWindowGrabFunction GetCanonicalSDLSetWindowGrab()
{
    static SDLSetWindowGrabFunction function =
        GetCanonicalSDLProcedure<SDLSetWindowGrabFunction>(
            "SDL_SetWindowGrab");
    return function;
}

SDLSetWindowFullscreenFunction GetCanonicalSDLSetWindowFullscreen()
{
    static SDLSetWindowFullscreenFunction function =
        GetCanonicalSDLProcedure<SDLSetWindowFullscreenFunction>(
            "SDL_SetWindowFullscreen");
    return function;
}

SDLMallocFunction GetCanonicalSDLMalloc()
{
    static SDLMallocFunction function =
        GetCanonicalSDLProcedure<SDLMallocFunction>("SDL_malloc");
    return function;
}

SDLFreeFunction GetCanonicalSDLFree()
{
    static SDLFreeFunction function =
        GetCanonicalSDLProcedure<SDLFreeFunction>("SDL_free");
    return function;
}

SDLSetWindowBorderedFunction GetCanonicalSDLSetWindowBordered()
{
    static SDLSetWindowBorderedFunction function =
        GetCanonicalSDLProcedure<SDLSetWindowBorderedFunction>(
            "SDL_SetWindowBordered");
    return function;
}

SDLGetWindowDisplayIndexFunction GetCanonicalSDLGetWindowDisplayIndex()
{
    static SDLGetWindowDisplayIndexFunction function =
        GetCanonicalSDLProcedure<SDLGetWindowDisplayIndexFunction>(
            "SDL_GetWindowDisplayIndex");
    return function;
}

SDLGetCurrentDisplayModeFunction GetCanonicalSDLGetCurrentDisplayMode()
{
    static SDLGetCurrentDisplayModeFunction function =
        GetCanonicalSDLProcedure<SDLGetCurrentDisplayModeFunction>(
            "SDL_GetCurrentDisplayMode");
    return function;
}

SDLGetDisplayBoundsFunction GetCanonicalSDLGetDisplayBounds()
{
    static SDLGetDisplayBoundsFunction function =
        GetCanonicalSDLProcedure<SDLGetDisplayBoundsFunction>(
            "SDL_GetDisplayBounds");
    return function;
}

SDLSetWindowSizeFunction GetCanonicalSDLSetWindowSize()
{
    static SDLSetWindowSizeFunction function =
        GetCanonicalSDLProcedure<SDLSetWindowSizeFunction>(
            "SDL_SetWindowSize");
    return function;
}

SDLSetWindowPositionFunction GetCanonicalSDLSetWindowPosition()
{
    static SDLSetWindowPositionFunction function =
        GetCanonicalSDLProcedure<SDLSetWindowPositionFunction>(
            "SDL_SetWindowPosition");
    return function;
}

SDLGetWindowWMInfoFunction GetCanonicalSDLGetWindowWMInfo()
{
    static SDLGetWindowWMInfoFunction function =
        GetCanonicalSDLProcedure<SDLGetWindowWMInfoFunction>(
            "SDL_GetWindowWMInfo");
    return function;
}

int InitializeCanonicalSDL(std::uint32_t flags)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_init != nullptr) {
        return create_test_hooks->sdl_init(flags);
    }
#endif
    return GetCanonicalSDLInit()(flags);
}

const char *GetCanonicalSDLError()
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_get_error != nullptr) {
        return create_test_hooks->sdl_get_error();
    }
#endif
    return GetCanonicalSDLGetError()();
}

int InitializeCanonicalSDLImage(int flags)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->img_init != nullptr) {
        return create_test_hooks->img_init(flags);
    }
#endif
    return GetCanonicalIMGInit()(flags);
}

int InitializeCanonicalSDLTTF()
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->ttf_init != nullptr) {
        return create_test_hooks->ttf_init();
    }
#endif
    return GetCanonicalTTFInit()();
}

void *CreateCanonicalSDLWindow(
    const char *title,
    int x,
    int y,
    int width,
    int height,
    std::uint32_t flags)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_create_window != nullptr) {
        return create_test_hooks->sdl_create_window(
            title, x, y, width, height, flags);
    }
#endif
    return GetCanonicalSDLCreateWindow()(
        title, x, y, width, height, flags);
}

void *CreateCanonicalSDLRenderer(
    void *window, int index, std::uint32_t flags)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_create_renderer != nullptr) {
        return create_test_hooks->sdl_create_renderer(window, index, flags);
    }
#endif
    return GetCanonicalSDLCreateRenderer()(window, index, flags);
}

void *CreateCanonicalSDLTexture(
    void *renderer,
    std::uint32_t format,
    int access,
    int width,
    int height)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_create_texture != nullptr) {
        return create_test_hooks->sdl_create_texture(
            renderer, format, access, width, height);
    }
#endif
    return GetCanonicalSDLCreateTexture()(
        renderer, format, access, width, height);
}

int SetCanonicalSDLTextureBlendMode(void *texture, int mode)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_set_texture_blend_mode != nullptr) {
        return create_test_hooks->sdl_set_texture_blend_mode(texture, mode);
    }
#endif
    return GetCanonicalSDLSetTextureBlendMode()(texture, mode);
}

int SetCanonicalSDLRenderTarget(void *renderer, void *texture)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_set_render_target != nullptr) {
        return create_test_hooks->sdl_set_render_target(renderer, texture);
    }
#endif
    return GetCanonicalSDLSetRenderTarget()(renderer, texture);
}

int SetCanonicalSDLRenderDrawColor(
    void *renderer,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue,
    std::uint8_t alpha)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_set_render_draw_color != nullptr) {
        return create_test_hooks->sdl_set_render_draw_color(
            renderer, red, green, blue, alpha);
    }
#endif
    return GetCanonicalSDLSetRenderDrawColor()(
        renderer, red, green, blue, alpha);
}

int ClearCanonicalSDLRenderer(void *renderer)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_render_clear != nullptr) {
        return create_test_hooks->sdl_render_clear(renderer);
    }
#endif
    return GetCanonicalSDLRenderClear()(renderer);
}

int SetCanonicalSDLRenderDrawBlendMode(void *renderer, int mode)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_set_render_draw_blend_mode != nullptr) {
        return create_test_hooks->sdl_set_render_draw_blend_mode(
            renderer, mode);
    }
#endif
    return GetCanonicalSDLSetRenderDrawBlendMode()(renderer, mode);
}

int GetCanonicalSDLWindowHandle(void *window, HWND *native_window)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->sdl_get_window_handle != nullptr) {
        return create_test_hooks->sdl_get_window_handle(
            window, native_window);
    }
#endif
    CanonicalSDLSysWMinfo information = {};
    information.major = 2;
    information.minor = 26;
    information.patch = 5;
    const int result = GetCanonicalSDLGetWindowWMInfo()(window, &information);
    if (result != 0) {
        *native_window = information.window;
    }
    return result;
}

HRESULT InitializeCanonicalGameBar(CD3DApplication *application)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->initialize_game_bar != nullptr) {
        return create_test_hooks->initialize_game_bar(application);
    }
#endif
    using GameBarHandler =
        ABI::Windows::Foundation::IEventHandler<IInspectable *>;
    using Microsoft::WRL::Callback;
    using Microsoft::WRL::ComPtr;
    using Microsoft::WRL::Wrappers::HStringReference;

    HRESULT result = Windows::Foundation::GetActivationFactory(
        HStringReference(RuntimeClass_Windows_Gaming_UI_GameBar).Get(),
        &g_gameBarStatics);
    if (FAILED(result)) {
        application->debug_printf(
            const_cast<char *>("Failed to get game bar statics\n"));
        return E_FAIL;
    }

    ComPtr<GameBarHandler> visibility_handler =
        Callback<GameBarHandler>(
            [application](IInspectable *, IInspectable *) -> HRESULT {
                application->CheckGameBarVisibility(application->m_hWnd);
                return S_OK;
            });
    EventRegistrationToken visibility_token = {};
    result = g_gameBarStatics->add_VisibilityChanged(
        visibility_handler.Get(), &visibility_token);
    if (FAILED(result)) {
        application->debug_printf(
            const_cast<char *>("Failed to get game bar statics\n"));
        return E_FAIL;
    }

    ComPtr<GameBarHandler> input_handler =
        Callback<GameBarHandler>(
            [application](IInspectable *, IInspectable *) -> HRESULT {
                application->CheckGameBarInput(application->m_hWnd);
                return S_OK;
            });
    EventRegistrationToken input_token = {};
    (void)g_gameBarStatics->add_IsInputRedirectedChanged(
        input_handler.Get(), &input_token);

    boolean visible = false;
    (void)g_gameBarStatics->get_Visible(&visible);
    if (g_isVisible != visible) {
        g_isVisible = visible;
        menu_enterPauseMode();
        InvalidateRect(application->m_hWnd, nullptr, TRUE);
    }
    application->CheckGameBarInput(application->m_hWnd);
    return S_OK;
}

HRESULT InitializeCanonicalApplicationFramework(
    CD3DApplication *application, HWND window)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->initialize_framework != nullptr) {
        return create_test_hooks->initialize_framework(application, window);
    }
#endif
    return application->InitD3D12Framework(window);
}

HRESULT InitializeCanonicalApplicationTextures(
    CD3DApplication *application)
{
#ifdef JPB_D3DAPP_TESTING
    if (create_test_hooks != nullptr &&
        create_test_hooks->initialize_textures != nullptr) {
        return create_test_hooks->initialize_textures(application);
    }
#endif
    application->m_fontAtlas = std::make_unique<FontAtlas>(
        0x16762004, 0x800, 0x800);
    PHL::Texture2D *atlas_texture =
        application->m_fontAtlas->GetTexture();
    *reinterpret_cast<unsigned *>(
        reinterpret_cast<unsigned char *>(atlas_texture) + 8) = 1;
    atlasHandle = texture_GetMaterial(TT_SPRITE);
    atlasHandle->texture = atlas_texture;
    atlasHandle->ih = static_cast<std::int16_t>(atlas_texture->GetHeight());
    atlasHandle->iw = static_cast<std::int16_t>(atlas_texture->GetWidth());
    WaitForGpuTexture(application->m_pFramework);
    application->m_pVidTex = PHL::Texture2D::CreateTexture(
        1920, 1080, PHL::TextureFormat::RGBA8888);
    return S_OK;
}

TimeGetTimeFunction GetCanonicalTimeGetTime()
{
    static TimeGetTimeFunction function = nullptr;
    if (function == nullptr) {
        HMODULE module = GetModuleHandleA("winmm.dll");
        if (module == nullptr) {
            module = LoadLibraryA("winmm.dll");
        }
        if (module == nullptr) {
            RaiseFailFastException(nullptr, nullptr, 0);
        }
        FARPROC procedure = GetProcAddress(module, "timeGetTime");
        if (procedure == nullptr) {
            RaiseFailFastException(nullptr, nullptr, 0);
        }
        function = reinterpret_cast<TimeGetTimeFunction>(procedure);
    }
    return function;
}

class CanonicalShaderCompiler {
public:
    CanonicalShaderCompiler()
    {
#ifdef JPB_D3DAPP_TESTING
        if (compile_shader_test_hook != nullptr) {
            return;
        }
#endif
        DxcCreateInstance(
            CLSID_DxcUtils,
            IID_PPV_ARGS(m_utils.ReleaseAndGetAddressOf()));
        DxcCreateInstance(
            CLSID_DxcCompiler,
            IID_PPV_ARGS(m_compiler.ReleaseAndGetAddressOf()));
        m_utils->CreateDefaultIncludeHandler(
            m_include_handler.ReleaseAndGetAddressOf());
    }

    bool Compile(
        const char *resource_name,
        const wchar_t **arguments,
        UINT32 argument_count,
        Microsoft::WRL::ComPtr<IDxcBlob> &shader)
    {
#ifdef JPB_D3DAPP_TESTING
        if (compile_shader_test_hook != nullptr) {
            IDxcBlob *compiled_shader = nullptr;
            const bool result = compile_shader_test_hook(
                resource_name,
                arguments,
                argument_count,
                &compiled_shader);
            shader.Attach(compiled_shader);
            return result;
        }
#endif
        const char *shader_path =
            resource_getPath(resource_name, JPB_RESOURCE_SHADER);

        const int converted_length = MultiByteToWideChar(
            CP_UTF8, 0, shader_path, -1, nullptr, 0);
        std::unique_ptr<wchar_t[]> converted_path(
            new wchar_t[static_cast<std::size_t>(converted_length)]);
        MultiByteToWideChar(
            CP_UTF8,
            0,
            shader_path,
            -1,
            converted_path.get(),
            converted_length);

        wchar_t wide_shader_path[256];
        std::mbstowcs(
            wide_shader_path,
            shader_path,
            std::strlen(shader_path) + 1);

        Microsoft::WRL::ComPtr<IDxcBlobEncoding> source;
        m_utils->LoadFile(
            wide_shader_path, nullptr, source.ReleaseAndGetAddressOf());
        const DxcBuffer source_buffer = {
            source->GetBufferPointer(),
            source->GetBufferSize(),
            0,
        };

        Microsoft::WRL::ComPtr<IDxcResult> results;
        m_compiler->Compile(
            &source_buffer,
            arguments,
            argument_count,
            m_include_handler.Get(),
            IID_PPV_ARGS(results.ReleaseAndGetAddressOf()));

        Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
        HRESULT result = results->GetOutput(
            DXC_OUT_ERRORS,
            IID_PPV_ARGS(errors.ReleaseAndGetAddressOf()),
            nullptr);
        if (FAILED(result)) {
            if (errors != nullptr && errors->GetStringLength() != 0) {
                OutputDebugStringA(errors->GetStringPointer());
            }
            return false;
        }

        result = results->GetOutput(
            DXC_OUT_OBJECT,
            IID_PPV_ARGS(shader.ReleaseAndGetAddressOf()),
            nullptr);
        return SUCCEEDED(result);
    }

private:
    Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
    Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_include_handler;
};

D3D12_BLEND_DESC CanonicalBlendDescription(
    BOOL alpha_to_coverage,
    BOOL independent_blend,
    BOOL blend_enabled)
{
    D3D12_BLEND_DESC description = {};
    description.AlphaToCoverageEnable = alpha_to_coverage;
    description.IndependentBlendEnable = independent_blend;
    D3D12_RENDER_TARGET_BLEND_DESC &target = description.RenderTarget[0];
    target.BlendEnable = blend_enabled;
    target.LogicOpEnable = FALSE;
    target.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    target.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    target.BlendOp = D3D12_BLEND_OP_ADD;
    target.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    target.DestBlendAlpha = D3D12_BLEND_DEST_ALPHA;
    target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    target.LogicOp = D3D12_LOGIC_OP_CLEAR;
    target.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return description;
}

D3D12_RASTERIZER_DESC CanonicalRasterizerDescription()
{
    D3D12_RASTERIZER_DESC description = {};
    description.FillMode = D3D12_FILL_MODE_SOLID;
    description.CullMode = D3D12_CULL_MODE_NONE;
    description.DepthClipEnable = TRUE;
    return description;
}

D3D12_DEPTH_STENCIL_DESC CanonicalDepthStencilDescription()
{
    D3D12_DEPTH_STENCIL_DESC description = {};
    description.DepthEnable = TRUE;
    description.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    description.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    description.StencilEnable = FALSE;
    description.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    description.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    return description;
}

bool IsCanonicalSteamDeck()
{
#ifdef JPB_D3DAPP_TESTING
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

} // namespace

void jpb_D3DAppGetSDLWindowSize(
    SDL_Window *window, int *width, int *height)
{
#ifdef JPB_D3DAPP_TESTING
    if (sdl_get_window_size_test_hook != nullptr) {
        sdl_get_window_size_test_hook(window, width, height);
        return;
    }
#endif
    GetCanonicalSDLGetWindowSize()(window, width, height);
}

int jpb_D3DAppSetSDLWindowFullscreen(
    SDL_Window *window, std::uint32_t flags)
{
#ifdef JPB_D3DAPP_TESTING
    if (sdl_set_window_fullscreen_test_hook != nullptr) {
        return sdl_set_window_fullscreen_test_hook(window, flags);
    }
#endif
    return GetCanonicalSDLSetWindowFullscreen()(window, flags);
}

void *jpb_D3DAppSDLMalloc(std::size_t size)
{
#ifdef JPB_D3DAPP_TESTING
    if (sdl_malloc_test_hook != nullptr) {
        return sdl_malloc_test_hook(size);
    }
#endif
    return GetCanonicalSDLMalloc()(size);
}

void jpb_D3DAppSDLFree(void *memory)
{
#ifdef JPB_D3DAPP_TESTING
    if (sdl_free_test_hook != nullptr) {
        sdl_free_test_hook(memory);
        return;
    }
#endif
    GetCanonicalSDLFree()(memory);
}

#ifdef JPB_D3DAPP_TESTING
void jpb_d3dapp_set_create_test_hooks(
    const JPBD3DAppCreateTestHooks *hooks)
{
    create_test_hooks = hooks;
}

void jpb_d3dapp_set_create_dxgi_factory_test_hook(
    JPBCreateDXGIFactory2TestHook hook)
{
    create_dxgi_factory_test_hook = hook;
}

void jpb_d3dapp_set_framework_destroy_objects_test_hook(
    JPBFrameworkDestroyObjectsTestHook hook)
{
    framework_destroy_objects_test_hook = hook;
}

void jpb_d3dapp_set_create_device_test_hook(
    JPBD3D12CreateDeviceTestHook hook)
{
    d3d12_create_device_test_hook = hook;
}

void jpb_d3dapp_set_serialize_root_signature_test_hook(
    JPBD3D12SerializeRootSignatureTestHook hook)
{
    d3d12_serialize_root_signature_test_hook = hook;
}

void jpb_d3dapp_set_resize_resources_test_hook(
    JPBResizeResourcesTestHook hook)
{
    resize_resources_test_hook = hook;
}

void jpb_d3dapp_set_graphics_memory_commit_test_hook(
    JPBGraphicsMemoryCommitTestHook hook)
{
    graphics_memory_commit_test_hook = hook;
}

void jpb_d3dapp_set_debug_material_test_hook(
    JPBDebugMaterialTestHook hook)
{
    debug_material_test_hook = hook;
}

void jpb_d3dapp_set_message_box_test_hook(
    JPBMessageBoxATestHook hook)
{
    message_box_test_hook = hook;
}

void jpb_d3dapp_set_game_bar_query_test_hook(
    JPBGameBarQueryTestHook hook)
{
    game_bar_query_test_hook = hook;
}

void jpb_d3dapp_set_invalidate_rect_test_hook(
    JPBInvalidateRectTestHook hook)
{
    invalidate_rect_test_hook = hook;
}

void jpb_d3dapp_set_pause_menu_test_hook(JPBPauseMenuTestHook hook)
{
    pause_menu_test_hook = hook;
}

void jpb_d3dapp_set_compile_shader_test_hook(
    JPBCompileShaderTestHook hook)
{
    compile_shader_test_hook = hook;
}

void jpb_d3dapp_set_steam_deck_test_hook(JPBSteamDeckTestHook hook)
{
    steam_deck_test_hook = hook;
}

void jpb_d3dapp_set_wait_for_gpu_test_hook(JPBWaitForGpuTestHook hook)
{
    wait_for_gpu_test_hook = hook;
}

void jpb_d3dapp_set_sdl_get_window_flags_test_hook(
    JPBSDLGetWindowFlagsTestHook hook)
{
    sdl_get_window_flags_test_hook = hook;
}

void jpb_d3dapp_set_sdl_get_window_size_test_hook(
    JPBSDLGetWindowSizeTestHook hook)
{
    sdl_get_window_size_test_hook = hook;
}

void jpb_d3dapp_set_sdl_set_window_fullscreen_test_hook(
    JPBSDLSetWindowFullscreenTestHook hook)
{
    sdl_set_window_fullscreen_test_hook = hook;
}

void jpb_d3dapp_set_sdl_malloc_test_hook(JPBSDLMallocTestHook hook)
{
    sdl_malloc_test_hook = hook;
}

void jpb_d3dapp_set_sdl_free_test_hook(JPBSDLFreeTestHook hook)
{
    sdl_free_test_hook = hook;
}

void jpb_d3dapp_set_sdl_destroy_window_test_hook(
    JPBSDLDestroyWindowTestHook hook)
{
    sdl_destroy_window_test_hook = hook;
}

void jpb_d3dapp_set_sdl_quit_test_hook(JPBSDLQuitTestHook hook)
{
    sdl_quit_test_hook = hook;
}

void jpb_d3dapp_set_sdl_query_texture_test_hook(
    JPBSDLQueryTextureTestHook hook)
{
    sdl_query_texture_test_hook = hook;
}

void jpb_d3dapp_set_sdl_render_present_test_hook(
    JPBSDLRenderPresentTestHook hook)
{
    sdl_render_present_test_hook = hook;
}

void jpb_d3dapp_set_sdl_render_read_pixels_test_hook(
    JPBSDLRenderReadPixelsTestHook hook)
{
    sdl_render_read_pixels_test_hook = hook;
}

void jpb_d3dapp_sort_sprite_draws_for_test(
    SpriteDraw *draws, std::size_t count)
{
    SortCanonicalSpriteDraws(draws, draws + count);
}

void jpb_d3dapp_set_game_bar_state_for_test(
    unsigned char visible, unsigned char input_redirected)
{
    g_isVisible = visible;
    g_isInputRedirected = input_redirected;
}

unsigned char jpb_d3dapp_get_game_bar_visible_for_test()
{
    return g_isVisible;
}

unsigned char jpb_d3dapp_get_game_bar_input_redirected_for_test()
{
    return g_isInputRedirected;
}
#endif

/*
 * COMPLETE REVIEWED RECONSTRUCTION.
 * PDB module: 0020
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\d3dapp.obj
 * Primary source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 * Compiler language: c++
 * Emitted procedures: 279
 *
 * All project-owned procedures are reconstructed from the matched PDB and
 * direct shipped-executable disassembly.
 */

#include "jpb/d3dtextr.h"

#include <cstdlib>
#include <cstring>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/* 0x1000, 28 bytes, local, 0 named locals
 * `dynamic initializer for 'gQueryResult''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
int gQueryResult = QueryPerformanceFrequency(&gFrequency);

/* 0x1020, 25 bytes, local, 0 named locals
 * `dynamic initializer for 'gSecondsPerFrame''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
double gSecondsPerFrame = 1.0 / gFramesPerSecond;

/* 0x1040, 12 bytes, local, 0 named locals
 * `dynamic initializer for 'g_gameBarStatics''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

/* 0x2CD40, 8 bytes, global, 2 named locals
 * Microsoft::WRL::operator!=<IDxcBlobUtf8>
 * PDB type: bool (const Microsoft::WRL::ComP...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2CD50, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<ID3D12CommandAllocator>
 * PDB type: void** (ID3D12CommandAllocator**...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CD60, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<ID3D12CommandQueue>
 * PDB type: void** (ID3D12CommandQueue**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CD70, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<ID3D12DescriptorHeap>
 * PDB type: void** (ID3D12DescriptorHeap**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CD80, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<ID3D12Device>
 * PDB type: void** (ID3D12Device**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CD90, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<ID3D12Fence>
 * PDB type: void** (ID3D12Fence**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CDA0, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<ID3D12GraphicsCommandList>
 * PDB type: void** (ID3D12GraphicsCommandLis...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CDB0, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<ID3D12PipelineState>
 * PDB type: void** (ID3D12PipelineState**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CDC0, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<ID3D12Resource>
 * PDB type: void** (ID3D12Resource**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CDD0, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<ID3D12RootSignature>
 * PDB type: void** (ID3D12RootSignature**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CDE0, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<IDXGIFactory6>
 * PDB type: void** (IDXGIFactory6**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CDF0, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<IDXGISwapChain3>
 * PDB type: void** (IDXGISwapChain3**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CE00, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<IDxcBlob>
 * PDB type: void** (IDxcBlob**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CE10, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<IDxcBlobUtf8>
 * PDB type: void** (IDxcBlobUtf8**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CE20, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<IDxcCompiler3>
 * PDB type: void** (IDxcCompiler3**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CE30, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<IDxcResult>
 * PDB type: void** (IDxcResult**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CE40, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<IDxcUtils>
 * PDB type: void** (IDxcUtils**)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CE50, 4 bytes, global, 1 named locals
 * IID_PPV_ARGS_Helper<ABI::Windows::Gaming::UI::IGameBarStatics>
 * PDB type: void** (ABI::Windows::Gaming::UI...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\combaseapi.h
 */

/* 0x2CE60, 40 bytes, global, 2 named locals
 * IID_PPV_ARGS_Helper<Microsoft::WRL::ComPtr<IDXGIAdapter1> >
 * PDB type: void** (Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2CE90, 40 bytes, global, 2 named locals
 * IID_PPV_ARGS_Helper<Microsoft::WRL::ComPtr<IDxcBlob> >
 * PDB type: void** (Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2CEC0, 40 bytes, global, 2 named locals
 * IID_PPV_ARGS_Helper<Microsoft::WRL::ComPtr<IDxcBlobUtf8> >
 * PDB type: void** (Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2CEF0, 40 bytes, global, 2 named locals
 * IID_PPV_ARGS_Helper<Microsoft::WRL::ComPtr<IDxcCompiler3> >
 * PDB type: void** (Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2CF20, 40 bytes, global, 2 named locals
 * IID_PPV_ARGS_Helper<Microsoft::WRL::ComPtr<IDxcResult> >
 * PDB type: void** (Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2CF50, 40 bytes, global, 2 named locals
 * IID_PPV_ARGS_Helper<Microsoft::WRL::ComPtr<IDxcUtils> >
 * PDB type: void** (Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2CF80, 58 bytes, global, 4 named locals
 * std::_Allocate_manually_vector_aligned<std::_Default_allocate_traits>
 * PDB type: void* (const unsigned __int64)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x2CFC0, 270 bytes, global, 9 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Construct<1,char const *>
 * PDB type: void std::basic_string<char,std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x2D0D0, 23 bytes, global, 4 named locals
 * std::_Copy_backward_memmove<SpriteDraw *,SpriteDraw *>
 * PDB type: SpriteDraw* (SpriteDraw*, Sprite...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x2D0F0, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<unsigned char *,unsigned char *>
 * PDB type: unsigned char* (unsigned char*, ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x2D120, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<wchar_t const * *,wchar_t const * *>
 * PDB type: const wchar_t** (const wchar_t**...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x2D150, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<SpriteDraw *,SpriteDraw *>
 * PDB type: SpriteDraw* (SpriteDraw*, Sprite...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x2D180, 569 bytes, global, 21 named locals
 * std::vector<SpriteDraw,std::allocator<SpriteDraw> >::_Emplace_reallocate<SpriteDraw const &>
 * PDB type: SpriteDraw* std::vector<SpriteDr...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x2D3C0, 415 bytes, global, 23 named locals
 * std::vector<wchar_t const *,std::allocator<wchar_t const *> >::_Emplace_reallocate<wchar_t const *>
 * PDB type: const wchar_t** std::vector<wcha...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x2D560, 191 bytes, global, 8 named locals
 * std::_Tree_val<std::_Tree_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::_Erase_tree<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_Tree_val<std::_Tree_s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0x2D620, 15 bytes, global, 2 named locals
 * std::_Fill_zero_memset<int *>
 * PDB type: void (int*, const unsigned __int...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x2D630, 8 bytes, global, 0 named locals
 * std::_Immortalize_memcpy_image<std::_Future_error_category2>
 * PDB type: const std::_Future_error_categor...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x2D640, 7 bytes, global, 1 named locals
 * std::_Is_all_bits_zero<int>
 * PDB type: bool (const int&)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x2D650, 392 bytes, local, 6 named locals
 * std::_Med3_unchecked<SpriteDraw *,<lambda_5b4adf6faf65c01d426ffae0d97f784a> >
 * PDB type: void (SpriteDraw*, SpriteDraw*, ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x2D7E0, 2092 bytes, local, 16 named locals
 * std::_Partition_by_median_guess_unchecked<SpriteDraw *,<lambda_5b4adf6faf65c01d426ffae0d97f784a> >
 * PDB type: std::pair<SpriteDraw *,SpriteDra...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x2E010, 518 bytes, local, 10 named locals
 * std::_Pop_heap_hole_by_index<SpriteDraw *,SpriteDraw,<lambda_5b4adf6faf65c01d426ffae0d97f784a> >
 * PDB type: void (SpriteDraw*, __int64, __in...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x2E220, 323 bytes, global, 21 named locals
 * std::vector<unsigned char,std::allocator<unsigned char> >::_Resize_reallocate<std::_Value_init_tag>
 * PDB type: void std::vector<unsigned char,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x2E370, 1421 bytes, local, 19 named locals
 * std::_Sort_unchecked<SpriteDraw *,<lambda_5b4adf6faf65c01d426ffae0d97f784a> >
 * PDB type: void (SpriteDraw*, SpriteDraw*, ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x2E900, 31 bytes, global, 2 named locals
 * std::_Zero_range<unsigned char *>
 * PDB type: unsigned char* (unsigned char* c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x2E920, 32 bytes, global, 4 named locals
 * std::uninitialized_fill<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > > > *,std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > > > >
 * PDB type: void (std::_List_unchecked_itera...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x2E940, 792 bytes, global, 14 named locals
 * CD3DApplication::CD3DApplication
 * PDB type: void CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
CD3DApplication::CD3DApplication()
    : m_texture(nullptr),
      textureUploadHeap(nullptr),
      m_bActive(0),
      m_bReady(0),
      m_bFrameMoving(1),
      m_bSingleStep(0),
      m_pVidTex(nullptr),
      m_pFramework(nullptr),
      m_hWnd(nullptr),
      m_pWindow(nullptr),
      m_bAppUseFullScreen(1),
      m_bOldWindowedState(1),
      endPolyCallsPerFrame(0),
      isFedMovie(0),
      m_strWindowTitle(m_strAppName),
      m_dwWindowFlags(0),
      m_dwDefaultWidth(1920),
      m_dwDefaultHeight(1080),
      m_dwDefaultBitsPixel(16),
      courier(nullptr)
{
    std::memset(triArray, 0, sizeof(triArray));
    std::memset(quadArray, 0, sizeof(quadArray));
    g_pD3DApp = this;
    courier = CreateFontA(
        -14,
        0,
        0,
        0,
        500,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FF_MODERN,
        "Courier New");
}

/* 0x2EC60, 32 bytes, global, 1 named locals
 * Vertex::Vertex
 * PDB type: void Vertex::()
 * Source: W:\SWJediPowerBattles\winver\DirectXTK12-jun2023\Inc\VertexTypes.h
 */

/* 0x2EC80, 4 bytes, global, 1 named locals
 * _D3DLVERTEX::_D3DLVERTEX
 * PDB type: void _D3DLVERTEX::()
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\d3dtypes.h
 */

/* 0x2EC90, 60 bytes, global, 2 named locals
 * std::bad_alloc::bad_alloc
 * PDB type: void std::bad_alloc::(const std:...
 * Source: no line mapping
 */

/* 0x2ECD0, 60 bytes, global, 2 named locals
 * std::bad_array_new_length::bad_array_new_length
 * PDB type: void std::bad_array_new_length::...
 * Source: no line mapping
 */

/* 0x2ED10, 33 bytes, global, 1 named locals
 * std::bad_array_new_length::bad_array_new_length
 * PDB type: void std::bad_array_new_length::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vcruntime_exception.h
 */

/* 0x2ED40, 60 bytes, global, 2 named locals
 * std::bad_optional_access::bad_optional_access
 * PDB type: void std::bad_optional_access::(...
 * Source: no line mapping
 */

/* 0x2ED80, 50 bytes, global, 2 named locals
 * std::exception::exception
 * PDB type: void std::exception::(const std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vcruntime_exception.h
 */

/* 0x2EDC0, 77 bytes, global, 2 named locals
 * std::future_error::future_error
 * PDB type: void std::future_error::(const s...
 * Source: no line mapping
 */

/* 0x2EE10, 100 bytes, global, 3 named locals
 * std::future_error::future_error
 * PDB type: void std::future_error::(std::fu...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\future
 */

/* 0x2EE80, 60 bytes, global, 2 named locals
 * std::logic_error::logic_error
 * PDB type: void std::logic_error::(const st...
 * Source: no line mapping
 */

/* 0x2EEC0, 60 bytes, global, 2 named locals
 * std::out_of_range::out_of_range
 * PDB type: void std::out_of_range::(const s...
 * Source: no line mapping
 */

/* 0x2EF00, 71 bytes, global, 3 named locals
 * std::out_of_range::out_of_range
 * PDB type: void std::out_of_range::(const c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\stdexcept
 */

/* 0x2EF50, 24 bytes, global, 1 named locals
 * ATL::CComPtr<IDxcBlob>::~CComPtr<IDxcBlob>
 * PDB type: void ATL::CComPtr<IDxcBlob>::()
 * Source: no line mapping
 */

/* 0x2EF70, 24 bytes, global, 1 named locals
 * ATL::CComPtr<IDxcBlobEncoding>::~CComPtr<IDxcBlobEncoding>
 * PDB type: void ATL::CComPtr<IDxcBlobEncodi...
 * Source: no line mapping
 */

/* 0x2EF90, 24 bytes, global, 1 named locals
 * ATL::CComPtr<IDxcBlobUtf8>::~CComPtr<IDxcBlobUtf8>
 * PDB type: void ATL::CComPtr<IDxcBlobUtf8>:...
 * Source: no line mapping
 */

/* 0x2EFB0, 24 bytes, global, 1 named locals
 * ATL::CComPtr<IDxcCompiler3>::~CComPtr<IDxcCompiler3>
 * PDB type: void ATL::CComPtr<IDxcCompiler3>...
 * Source: no line mapping
 */

/* 0x2EFD0, 24 bytes, global, 1 named locals
 * ATL::CComPtr<IDxcIncludeHandler>::~CComPtr<IDxcIncludeHandler>
 * PDB type: void ATL::CComPtr<IDxcIncludeHan...
 * Source: no line mapping
 */

/* 0x2EFF0, 24 bytes, global, 1 named locals
 * ATL::CComPtr<IDxcResult>::~CComPtr<IDxcResult>
 * PDB type: void ATL::CComPtr<IDxcResult>::(...
 * Source: no line mapping
 */

/* 0x2F010, 24 bytes, global, 1 named locals
 * ATL::CComPtr<IDxcUtils>::~CComPtr<IDxcUtils>
 * PDB type: void ATL::CComPtr<IDxcUtils>::()
 * Source: no line mapping
 */

/* 0x2F030, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IEventHandler<IInspectable *> >::~ComPtr<ABI::Windows::Foundation::IEventHandler<IInspectable *> >
 * PDB type: void Microsoft::WRL::ComPtr<ABI:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2F060, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IDXGIAdapter1>::~ComPtr<IDXGIAdapter1>
 * PDB type: void Microsoft::WRL::ComPtr<IDXG...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2F090, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IDxcBlob>::~ComPtr<IDxcBlob>
 * PDB type: void Microsoft::WRL::ComPtr<IDxc...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2F0C0, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IDxcBlobEncoding>::~ComPtr<IDxcBlobEncoding>
 * PDB type: void Microsoft::WRL::ComPtr<IDxc...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2F0F0, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IDxcBlobUtf8>::~ComPtr<IDxcBlobUtf8>
 * PDB type: void Microsoft::WRL::ComPtr<IDxc...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2F120, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IDxcCompiler3>::~ComPtr<IDxcCompiler3>
 * PDB type: void Microsoft::WRL::ComPtr<IDxc...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2F150, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IDxcIncludeHandler>::~ComPtr<IDxcIncludeHandler>
 * PDB type: void Microsoft::WRL::ComPtr<IDxc...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2F180, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IDxcResult>::~ComPtr<IDxcResult>
 * PDB type: void Microsoft::WRL::ComPtr<IDxc...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2F1B0, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IDxcUtils>::~ComPtr<IDxcUtils>
 * PDB type: void Microsoft::WRL::ComPtr<IDxc...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2F1E0, 34 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IUnknown>::~ComPtr<IUnknown>
 * PDB type: void Microsoft::WRL::ComPtr<IUnk...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\client.h
 */

/* 0x2F210, 8 bytes, global, 1 named locals
 * Microsoft::WRL::Details::RuntimeClassImpl<Microsoft::WRL::RuntimeClassFlags<2>,1,0,1,ABI::Windows::Foundation::IEventHandler<IInspectable *> >::~RuntimeClassImpl<Microsoft::WRL::RuntimeClassFlags<2>,1,0,1,ABI::Windows::Foundation::IEventHandler<IInspectable *> >
 * PDB type: void Microsoft::WRL::Details::Ru...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\implements.h
 */

/* 0x2F220, 101 bytes, global, 5 named locals
 * std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::~_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >
 * PDB type: void std::_Hash<std::_Umap_trait...
 * Source: no line mapping
 */

/* 0x2F290, 91 bytes, global, 5 named locals
 * std::_Hash_vec<std::allocator<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > > > > >::~_Hash_vec<std::allocator<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > > > > >
 * PDB type: void std::_Hash_vec<std::allocat...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x2F2F0, 59 bytes, global, 2 named locals
 * std::future<void>::~future<void>
 * PDB type: void std::future<void>::()
 * Source: no line mapping
 */

/* 0x2F330, 182 bytes, global, 8 named locals
 * std::list<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::~list<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >
 * PDB type: void std::list<std::pair<std::ba...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\list
 */

/* 0x2F3F0, 42 bytes, global, 1 named locals
 * std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::~map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >
 * PDB type: void std::map<std::basic_string<...
 * Source: no line mapping
 */

/* 0x2F420, 20 bytes, global, 1 named locals
 * std::unique_ptr<DirectX::DX12::CommonStates,std::default_delete<DirectX::DX12::CommonStates> >::~unique_ptr<DirectX::DX12::CommonStates,std::default_delete<DirectX::DX12::CommonStates> >
 * PDB type: void std::unique_ptr<DirectX::DX...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\memory
 */

/* 0x2F440, 46 bytes, global, 1 named locals
 * std::unique_ptr<D3DTransparencyPass,std::default_delete<D3DTransparencyPass> >::~unique_ptr<D3DTransparencyPass,std::default_delete<D3DTransparencyPass> >
 * PDB type: void std::unique_ptr<D3DTranspar...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\memory
 */

/* 0x2F470, 46 bytes, global, 1 named locals
 * std::unique_ptr<FontAtlas,std::default_delete<FontAtlas> >::~unique_ptr<FontAtlas,std::default_delete<FontAtlas> >
 * PDB type: void std::unique_ptr<FontAtlas,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\memory
 */

/* 0x2F4A0, 20 bytes, global, 1 named locals
 * std::unique_ptr<DirectX::DX12::SpriteBatch,std::default_delete<DirectX::DX12::SpriteBatch> >::~unique_ptr<DirectX::DX12::SpriteBatch,std::default_delete<DirectX::DX12::SpriteBatch> >
 * PDB type: void std::unique_ptr<DirectX::DX...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\memory
 */

/* 0x2F4C0, 5 bytes, global, 1 named locals
 * std::unordered_map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::~unordered_map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >
 * PDB type: void std::unordered_map<std::bas...
 * Source: no line mapping
 */

/* 0x2F4D0, 91 bytes, global, 5 named locals
 * std::vector<wchar_t const *,std::allocator<wchar_t const *> >::~vector<wchar_t const *,std::allocator<wchar_t const *> >
 * PDB type: void std::vector<wchar_t const *...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x2F530, 118 bytes, global, 5 named locals
 * std::vector<SpriteDraw,std::allocator<SpriteDraw> >::~vector<SpriteDraw,std::allocator<SpriteDraw> >
 * PDB type: void std::vector<SpriteDraw,std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x2F5B0, 118 bytes, global, 5 named locals
 * std::vector<Vertex,std::allocator<Vertex> >::~vector<Vertex,std::allocator<Vertex> >
 * PDB type: void std::vector<Vertex,std::all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x2F630, 533 bytes, global, 13 named locals
 * CD3DApplication::~CD3DApplication
 * PDB type: void CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
CD3DApplication::~CD3DApplication()
{
    WaitForGpu();
    Cleanup3DEnvironment();
    DestroyWindow(m_hWnd);
    if (courier != nullptr) {
        DeleteObject(courier);
        courier = nullptr;
    }
}

/* 0x2F850, 450 bytes, global, 20 named locals
 * D3DTransparencyPass::~D3DTransparencyPass
 * PDB type: void D3DTransparencyPass::()
 * Source: no line mapping
 */

/* 0x2FA20, 34 bytes, global, 2 named locals
 * DirectX::DescriptorHeap::~DescriptorHeap
 * PDB type: void DirectX::DescriptorHeap::()
 * Source: no line mapping
 */

/* 0x2FA50, 523 bytes, global, 23 named locals
 * FontAtlas::~FontAtlas
 * PDB type: void FontAtlas::()
 * Source: no line mapping
 */

/* 0x2FC60, 19 bytes, global, 1 named locals
 * std::bad_alloc::~bad_alloc
 * PDB type: void std::bad_alloc::()
 * Source: no line mapping
 */

/* 0x2FC80, 19 bytes, global, 1 named locals
 * std::bad_array_new_length::~bad_array_new_length
 * PDB type: void std::bad_array_new_length::...
 * Source: no line mapping
 */

/* 0x2FCA0, 19 bytes, global, 1 named locals
 * std::bad_optional_access::~bad_optional_access
 * PDB type: void std::bad_optional_access::(...
 * Source: no line mapping
 */

/* 0x2FCC0, 3 bytes, global, 1 named locals
 * std::error_category::~error_category
 * PDB type: void std::error_category::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x2FCD0, 19 bytes, global, 1 named locals
 * std::exception::~exception
 * PDB type: void std::exception::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vcruntime_exception.h
 */

/* 0x2FCF0, 19 bytes, global, 1 named locals
 * std::future_error::~future_error
 * PDB type: void std::future_error::()
 * Source: no line mapping
 */

/* 0x2FD10, 19 bytes, global, 1 named locals
 * std::out_of_range::~out_of_range
 * PDB type: void std::out_of_range::()
 * Source: no line mapping
 */

/* 0x2FD30, 153 bytes, global, 2 named locals
 * Microsoft::WRL::ComPtr<IUnknown>::`vector deleting destructor'
 * PDB type: void* Microsoft::WRL::ComPtr<IUn...
 * Source: no line mapping
 */

/* 0x2FDD0, 40 bytes, local, 1 named locals
 * Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl ABI::Windows::Foundation::IEventHandler_impl<IInspectable *>::*)(IInspectable *,IInspectable *)>::DelegateInvokeHelper<ABI::Windows::Foundation::IEventHandler<IInspectable *>,<lambda_4f71b6447fa15199118d91075b9a45ed>,1,IInspectable *,IInspectable *>::`scalar deleting destructor'
 * PDB type: void* Microsoft::WRL::Details::D...
 * Source: no line mapping
 */

/* 0x2FE00, 40 bytes, local, 1 named locals
 * Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl ABI::Windows::Foundation::IEventHandler_impl<IInspectable *>::*)(IInspectable *,IInspectable *)>::DelegateInvokeHelper<ABI::Windows::Foundation::IEventHandler<IInspectable *>,<lambda_675d0a2d57fd62d4f0e34fd501361d84>,1,IInspectable *,IInspectable *>::`scalar deleting destructor'
 * PDB type: void* Microsoft::WRL::Details::D...
 * Source: no line mapping
 */

/* 0x2FE30, 40 bytes, global, 1 named locals
 * Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<2>,ABI::Windows::Foundation::IEventHandler<IInspectable *> >::`scalar deleting destructor'
 * PDB type: void* Microsoft::WRL::RuntimeCla...
 * Source: no line mapping
 */

/* 0x2FE60, 33 bytes, global, 1 named locals
 * std::_Future_error_category2::`scalar deleting destructor'
 * PDB type: void* std::_Future_error_categor...
 * Source: no line mapping
 */

/* 0x2FE90, 66 bytes, global, 1 named locals
 * std::bad_alloc::`scalar deleting destructor'
 * PDB type: void* std::bad_alloc::(unsigned)
 * Source: no line mapping
 */

/* 0x2FEE0, 66 bytes, global, 1 named locals
 * std::bad_array_new_length::`scalar deleting destructor'
 * PDB type: void* std::bad_array_new_length:...
 * Source: no line mapping
 */

/* 0x2FF30, 66 bytes, global, 1 named locals
 * std::bad_optional_access::`scalar deleting destructor'
 * PDB type: void* std::bad_optional_access::...
 * Source: no line mapping
 */

/* 0x2FF80, 66 bytes, global, 1 named locals
 * std::exception::`scalar deleting destructor'
 * PDB type: void* std::exception::(unsigned)
 * Source: no line mapping
 */

/* 0x2FFD0, 66 bytes, global, 1 named locals
 * std::future_error::`scalar deleting destructor'
 * PDB type: void* std::future_error::(unsign...
 * Source: no line mapping
 */

/* 0x30020, 66 bytes, global, 1 named locals
 * std::logic_error::`scalar deleting destructor'
 * PDB type: void* std::logic_error::(unsigne...
 * Source: no line mapping
 */

/* 0x30070, 66 bytes, global, 1 named locals
 * std::out_of_range::`scalar deleting destructor'
 * PDB type: void* std::out_of_range::(unsign...
 * Source: no line mapping
 */

/* 0x300C0, 72 bytes, global, 4 named locals
 * `vector constructor iterator'
 * PDB type: void (void*, unsigned __int64, u...
 * Source: no line mapping
 */

/* 0x30110, 45 bytes, global, 2 named locals
 * Microsoft::WRL::Details::RuntimeClassImpl<Microsoft::WRL::RuntimeClassFlags<2>,1,0,1,ABI::Windows::Foundation::IEventHandler<IInspectable *> >::AddRef
 * PDB type: unsigned long Microsoft::WRL::De...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\implements.h
 */

/* 0x30140, 119 bytes, global, 4 named locals
 * CD3DApplication::CapFrameRate
 * PDB type: void CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::CapFrameRate()
{
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    const double elapsed =
        static_cast<double>(current_time.QuadPart - lastTime.QuadPart) /
        static_cast<double>(gFrequency.QuadPart);
    if (elapsed < gSecondsPerFrame) {
        Sleep(static_cast<DWORD>(
            (gSecondsPerFrame - elapsed) * 1000.0));
        QueryPerformanceCounter(&current_time);
    }
    lastTime = current_time;
}

/* 0x301C0, 440 bytes, global, 5 named locals
 * CD3DApplication::Change3DEnvironment
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::Change3DEnvironment()
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    using DeleteDeviceObjectsCallback = HRESULT (*)(CD3DApplication *);
    void **vtable = *reinterpret_cast<void ***>(this);
    (void)reinterpret_cast<DeleteDeviceObjectsCallback>(vtable[2])(this);

    HRESULT result = DestroyCanonicalFrameworkObjects(m_pFramework);
    if (FAILED(result)) {
        DisplayFrameworkError(result, 1);
        PostMessageA(
            *reinterpret_cast<HWND *>(bytes + 0xD0),
            WM_CLOSE,
            0,
            0);
        return result;
    }

    const BOOL windowed = IsWindowed();
    BOOL &old_windowed_state = *reinterpret_cast<BOOL *>(bytes + 0xE8);
    HWND window = *reinterpret_cast<HWND *>(bytes + 0xD0);
    if (old_windowed_state != windowed) {
        DWORD &saved_style = *reinterpret_cast<DWORD *>(bytes + 0x38330);
        RECT &saved_rect = *reinterpret_cast<RECT *>(bytes + 0x38334);
        if (IsWindowed()) {
            SetWindowLongA(window, GWL_STYLE, saved_style);
            SetWindowPos(
                window,
                HWND_NOTOPMOST,
                saved_rect.left,
                saved_rect.top,
                saved_rect.right - saved_rect.left,
                saved_rect.bottom - saved_rect.top,
                SWP_SHOWWINDOW);
        } else {
            saved_style = static_cast<DWORD>(
                GetWindowLongA(window, GWL_STYLE));
            GetWindowRect(window, &saved_rect);
            SetWindowLongA(
                window,
                GWL_STYLE,
                static_cast<LONG>(0x90080000));
        }
        old_windowed_state = IsWindowed();
    }

    result = InitD3D12Framework(window);
    if (FAILED(result)) {
        DisplayFrameworkError(result, 1);
        PostMessageA(window, WM_CLOSE, 0, 0);
        return result;
    }

    if (*reinterpret_cast<BOOL *>(bytes + 0x2C) == FALSE) {
        *reinterpret_cast<BOOL *>(bytes + 0x30) = TRUE;
        *reinterpret_cast<DWORD *>(bytes + 0x34) +=
            GetCanonicalTimeGetTime()() -
            *reinterpret_cast<DWORD *>(bytes + 0x38);
        *reinterpret_cast<DWORD *>(bytes + 0x38) =
            GetCanonicalTimeGetTime()();
    }
    return S_OK;
}

/* 0x30380, 67 bytes, global, 3 named locals
 * CD3DApplication::CheckGameBarInput
 * PDB type: void CD3DApplication::(HWND__*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::CheckGameBarInput(HWND hwnd)
{
    unsigned char isInputRedirected;
    QueryCanonicalGameBarState(true, &isInputRedirected);
    if (g_isInputRedirected != isInputRedirected) {
        g_isVisible = isInputRedirected;
        InvalidateCanonicalRect(hwnd, nullptr, TRUE);
    }
}

/* 0x303D0, 72 bytes, global, 3 named locals
 * CD3DApplication::CheckGameBarVisibility
 * PDB type: void CD3DApplication::(HWND__*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

/* 0x30420, 97 bytes, global, 1 named locals
 * CD3DApplication::Cleanup3DEnvironment
 * PDB type: void CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

/* 0x30490, 266 bytes, global, 9 named locals
 * CD3DApplication::ConvertMatrixToDXMatrix
 * PDB type: DirectX::XMMATRIX CD3DApplicatio...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::CheckGameBarVisibility(HWND hwnd)
{
    unsigned char isVisible;
    QueryCanonicalGameBarState(false, &isVisible);
    if (g_isVisible != isVisible) {
        g_isVisible = isVisible;
        EnterCanonicalPauseMode();
        InvalidateCanonicalRect(hwnd, nullptr, TRUE);
    }
}
void CD3DApplication::Cleanup3DEnvironment()
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    *reinterpret_cast<std::uint64_t *>(bytes + 0x20) = 0;
    if (m_pFramework != nullptr) {
        using DeviceObjectCallback = HRESULT (*)(CD3DApplication *);
        void **vtable = *reinterpret_cast<void ***>(this);
        reinterpret_cast<DeviceObjectCallback>(vtable[2])(this);
        delete m_pFramework;
        m_pFramework = nullptr;
        reinterpret_cast<DeviceObjectCallback>(vtable[5])(this);
    }
    D3DEnum_FreeResources();
}
DirectX::XMMATRIX CD3DApplication::ConvertMatrixToDXMatrix(MATRIX *matrix)
{
    if (matrix == nullptr) {
        return DirectX::XMMatrixIdentity();
    }
    return DirectX::XMMATRIX(
        matrix->m[0][0], matrix->m[0][1], matrix->m[0][2], 0.0f,
        matrix->m[1][0], matrix->m[1][1], matrix->m[1][2], 0.0f,
        matrix->m[2][0], matrix->m[2][1], matrix->m[2][2], 0.0f,
        static_cast<float>(matrix->t[0]),
        static_cast<float>(matrix->t[1]),
        static_cast<float>(matrix->t[2]),
        1.0f);
}

/* 0x305A0, 29 bytes, global, 2 named locals
 * CD3DApplication::ConvertWindowSettingToFlags
 * PDB type: unsigned long CD3DApplication::(...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
unsigned long CD3DApplication::ConvertWindowSettingToFlags(int setting)
{
    switch (setting) {
    case 0:
        return 0x1101;
    case 1:
        return 0x10;
    case 2:
        return 0;
    }
}

/* 0x305C0, 1623 bytes, global, 18 named locals
 * CD3DApplication::Create
 * PDB type: HRESULT CD3DApplication::(HINSTA...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::Create(HINSTANCE, char *)
{
    if (InitializeCanonicalSDL(0x3230) != 0) {
        std::printf("SDL_Init Error: %s", GetCanonicalSDLError());
        return E_FAIL;
    }
    if (InitializeCanonicalSDLImage(0) != 0) {
        std::printf(
            "SDL_Image init Error: %s", GetCanonicalSDLError());
    }
    if (InitializeCanonicalSDLTTF() != 0) {
        std::printf(
            "SDL_ttf init Error %s", GetCanonicalSDLError());
        return E_FAIL;
    }

    m_pFramework = new (std::nothrow) CD3DFramework12;
    if (m_pFramework == nullptr) {
        char message[512];
        lstrcpyA(message, "Not enough memory!");
        lstrcatA(message, "\n\nThis program will now exit.");
        ShowCanonicalMessageBox(
            nullptr, message, m_strWindowTitle, MB_ICONERROR);
        return E_OUTOFMEMORY;
    }

    SetProcessDPIAware();
    m_pFramework->m_pSDLWindow = reinterpret_cast<SDL_Window *>(
        CreateCanonicalSDLWindow(
            m_strAppName,
            0x1FFF0000,
            0x1FFF0000,
            static_cast<int>(m_dwDefaultWidth),
            static_cast<int>(m_dwDefaultHeight),
            m_dwWindowFlags | 4));
    if (m_pFramework->m_pSDLWindow == nullptr) {
        std::printf(
            "SDL_CreateWindowFrom Error: %s",
            GetCanonicalSDLError());
        QuitCanonicalSDL();
        return E_FAIL;
    }

    m_pFramework->m_pSDLRenderer = reinterpret_cast<SDL_Renderer *>(
        CreateCanonicalSDLRenderer(
            m_pFramework->m_pSDLWindow, -1, 2));
    if (m_pFramework->m_pSDLRenderer == nullptr) {
        std::printf("SDL_RENDER Error: %s", GetCanonicalSDLError());
        DestroyCanonicalSDLWindow(m_pFramework->m_pSDLWindow);
        QuitCanonicalSDL();
        return E_FAIL;
    }

    m_pFramework->m_pSDLRenderTarget = reinterpret_cast<SDL_Texture *>(
        CreateCanonicalSDLTexture(
            m_pFramework->m_pSDLRenderer,
            0x16762004,
            2,
            static_cast<int>(m_dwDefaultWidth),
            static_cast<int>(m_dwDefaultHeight)));
    SetCanonicalSDLTextureBlendMode(
        m_pFramework->m_pSDLRenderTarget, 1);
    SetCanonicalSDLRenderTarget(
        m_pFramework->m_pSDLRenderer,
        m_pFramework->m_pSDLRenderTarget);
    SetCanonicalSDLRenderDrawColor(
        m_pFramework->m_pSDLRenderer, 0, 0, 0, 0);
    ClearCanonicalSDLRenderer(m_pFramework->m_pSDLRenderer);
    SetCanonicalSDLRenderDrawColor(
        m_pFramework->m_pSDLRenderer, 1, 1, 1, 1);
    SetCanonicalSDLRenderDrawBlendMode(
        m_pFramework->m_pSDLRenderer, 1);

    HWND native_window = nullptr;
    if (GetCanonicalSDLWindowHandle(
            m_pFramework->m_pSDLWindow, &native_window) != 0) {
        m_hWnd = native_window;
        m_pFramework->m_hWnd = native_window;
    }
    HRESULT result = InitializeCanonicalGameBar(this);
    if (FAILED(result)) {
        return E_FAIL;
    }

    m_dwBaseTime = GetCanonicalTimeGetTime()();
    if (m_pWindow != nullptr) {
        HMENU system_menu = GetSystemMenu(m_hWnd, FALSE);
        if (system_menu != nullptr) {
            AppendMenuA(system_menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuA(
                system_menu,
                MF_STRING,
                0x323,
                "&Full Screen\tF1");
        }
    }

    result = InitializeCanonicalApplicationFramework(this, m_hWnd);
    if (FAILED(result)) {
        DisplayFrameworkError(result, 1);
        Cleanup3DEnvironment();
        return E_FAIL;
    }

    (void)InitializeCanonicalApplicationTextures(this);
    m_bReady = 1;
    (void)OneTimeSceneInit();
    return S_OK;
}

/* 0x30C20, 169 bytes, global, 3 named locals
 * CD3DApplication::CreateFence
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::CreateFence()
{
    for (unsigned index = 0; index < 2; ++index) {
        HRESULT result = m_pFramework->m_pDevice->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&m_pFramework->m_pFence[index]));
        if (FAILED(result)) {
            return result;
        }
        m_pFramework->m_nFenceValues[index] = 0;
    }

    m_pFramework->m_hFenceEvent =
        CreateEventA(nullptr, FALSE, FALSE, nullptr);
    return m_pFramework->m_hFenceEvent == nullptr;
}

/* 0x30CD0, 2313 bytes, global, 33 named locals
 * CD3DApplication::CreateLevelPipelineStateObject
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::CreateLevelPipelineStateObject()
{
    CanonicalShaderCompiler compiler;
    const wchar_t *vertex_arguments[] = {
        L"-E", L"VSMain", L"-T", L"vs_6_0"};
    Microsoft::WRL::ComPtr<IDxcBlob> vertex_shader;
    if (!compiler.Compile(
            "LevelVertexShader.hlsl",
            vertex_arguments,
            static_cast<UINT32>(std::size(vertex_arguments)),
            vertex_shader)) {
        return S_OK;
    }

    const wchar_t *pixel_arguments[] = {
        L"-E", L"PSMain", L"-T", L"ps_6_0"};
    WaitForGpu();
    Microsoft::WRL::ComPtr<IDxcBlob> pixel_shader;
    if (!compiler.Compile(
            "LevelPixelShader.hlsl",
            pixel_arguments,
            static_cast<UINT32>(std::size(pixel_arguments)),
            pixel_shader)) {
        return S_OK;
    }

    const D3D12_INPUT_ELEMENT_DESC input_elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, 40,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 44,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC description = {};
    description.pRootSignature = m_pFramework->m_pRootSignature;
    description.VS = {
        vertex_shader->GetBufferPointer(), vertex_shader->GetBufferSize()};
    description.PS = {
        pixel_shader->GetBufferPointer(), pixel_shader->GetBufferSize()};
    description.BlendState = CanonicalBlendDescription(FALSE, FALSE, FALSE);
    description.SampleMask = UINT_MAX;
    description.RasterizerState = CanonicalRasterizerDescription();
    description.DepthStencilState = CanonicalDepthStencilDescription();
    description.InputLayout = {
        input_elements, static_cast<UINT>(std::size(input_elements))};
    description.IBStripCutValue =
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    description.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    description.NumRenderTargets = 1;
    description.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    description.SampleDesc.Count = 1;

    const HRESULT result = m_pFramework->m_pDevice->
        CreateGraphicsPipelineState(
            &description,
            IID_PPV_ARGS(&m_pFramework->m_pLevelPipelineState));
    m_pFramework->m_pLevelPipelineState->SetName(L"Level Pipeline State");
    return result;
}

/* 0x315E0, 2789 bytes, global, 47 named locals
 * CD3DApplication::CreatePipelineStateObject
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::CreatePipelineStateObject()
{
    CanonicalShaderCompiler compiler;
    const wchar_t *vertex_arguments[] = {
        L"-E", L"VSMain", L"-T", L"vs_6_0"};
    Microsoft::WRL::ComPtr<IDxcBlob> vertex_shader;
    if (!compiler.Compile(
            "VertexShader.hlsl",
            vertex_arguments,
            static_cast<UINT32>(std::size(vertex_arguments)),
            vertex_shader)) {
        return S_OK;
    }
    if (m_pFramework->vertexShaderBlob != nullptr) {
        m_pFramework->vertexShaderBlob->Release();
    }
    m_pFramework->vertexShaderBlob = vertex_shader.Detach();

    std::vector<const wchar_t *> pixel_arguments = {
        L"-E", L"PSMain", L"-T", L"ps_6_0"};
    if (IsCanonicalSteamDeck()) {
        pixel_arguments.push_back(L"-D");
        pixel_arguments.push_back(L"STEAM_DECK");
    }
    WaitForGpu();
    Microsoft::WRL::ComPtr<IDxcBlob> pixel_shader;
    if (!compiler.Compile(
            "PixelShader.hlsl",
            pixel_arguments.data(),
            static_cast<UINT32>(pixel_arguments.size()),
            pixel_shader)) {
        return S_OK;
    }
    if (m_pFramework->pixelShaderBlob != nullptr) {
        m_pFramework->pixelShaderBlob->Release();
    }
    m_pFramework->pixelShaderBlob = pixel_shader.Detach();

    const D3D12_INPUT_ELEMENT_DESC input_elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, 40,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    auto *vertex_blob = reinterpret_cast<IDxcBlob *>(
        m_pFramework->vertexShaderBlob);
    auto *pixel_blob = reinterpret_cast<IDxcBlob *>(
        m_pFramework->pixelShaderBlob);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC description = {};
    description.pRootSignature = m_pFramework->m_pRootSignature;
    description.VS = {
        vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize()};
    description.PS = {
        pixel_blob->GetBufferPointer(), pixel_blob->GetBufferSize()};
    description.BlendState = CanonicalBlendDescription(TRUE, TRUE, TRUE);
    description.SampleMask = UINT_MAX;
    description.RasterizerState = CanonicalRasterizerDescription();
    description.DepthStencilState = CanonicalDepthStencilDescription();
    description.InputLayout = {
        input_elements, static_cast<UINT>(std::size(input_elements))};
    description.IBStripCutValue =
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    description.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    description.NumRenderTargets = 1;
    description.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    description.SampleDesc.Count = 1;

    const HRESULT result = m_pFramework->m_pDevice->
        CreateGraphicsPipelineState(
            &description,
            IID_PPV_ARGS(&m_pFramework->m_pPipelineState));
    m_pFramework->m_pPipelineState->SetName(L"Pipeline State");
    return result;
}

/* 0x320D0, 228 bytes, global, 4 named locals
 * CD3DApplication::CreateSRVHeap
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::CreateSRVHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC heap_description = {};
    heap_description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_description.NumDescriptors = 0x400;
    heap_description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    m_pFramework->m_nDescriptorCount = 0;

    HRESULT result = m_pFramework->m_pDevice->CreateDescriptorHeap(
        &heap_description,
        IID_PPV_ARGS(&m_pFramework->m_pMainDescriptorHeap));
    m_pFramework->m_pMainDescriptorHeap->SetName(L"SRV Heap");
    if (FAILED(result)) {
        return result;
    }

    heap_description.NumDescriptors = 1;
    result = m_pFramework->m_pDevice->CreateDescriptorHeap(
        &heap_description,
        IID_PPV_ARGS(&m_pFramework->m_cbvHeap));
    return FAILED(result) ? result : S_OK;
}

/* 0x321C0, 2616 bytes, global, 44 named locals
 * CD3DApplication::CreateTransparentPipelineStateObject
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::CreateTransparentPipelineStateObject()
{
    CanonicalShaderCompiler compiler;
    const wchar_t *vertex_arguments[] = {
        L"-E", L"VSMain", L"-T", L"vs_6_0"};
    Microsoft::WRL::ComPtr<IDxcBlob> vertex_shader;
    if (!compiler.Compile(
            "LevelVertexShader.hlsl",
            vertex_arguments,
            static_cast<UINT32>(std::size(vertex_arguments)),
            vertex_shader)) {
        return S_OK;
    }
    if (m_pFramework->vertexShaderBlob != nullptr) {
        m_pFramework->vertexShaderBlob->Release();
    }
    m_pFramework->vertexShaderBlob = vertex_shader.Detach();

    const wchar_t *pixel_arguments[] = {
        L"-E", L"PSMain", L"-T", L"ps_6_0"};
    WaitForGpu();
    Microsoft::WRL::ComPtr<IDxcBlob> pixel_shader;
    if (!compiler.Compile(
            "LevelTransparencyPixelShader.hlsl",
            pixel_arguments,
            static_cast<UINT32>(std::size(pixel_arguments)),
            pixel_shader)) {
        return S_OK;
    }
    if (m_pFramework->pixelShaderBlob != nullptr) {
        m_pFramework->pixelShaderBlob->Release();
    }
    m_pFramework->pixelShaderBlob = pixel_shader.Detach();

    const D3D12_INPUT_ELEMENT_DESC input_elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, 40,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 44,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    auto *vertex_blob = reinterpret_cast<IDxcBlob *>(
        m_pFramework->vertexShaderBlob);
    auto *pixel_blob = reinterpret_cast<IDxcBlob *>(
        m_pFramework->pixelShaderBlob);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC description = {};
    description.pRootSignature = m_pFramework->m_pRootSignature;
    description.VS = {
        vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize()};
    description.PS = {
        pixel_blob->GetBufferPointer(), pixel_blob->GetBufferSize()};
    description.BlendState = CanonicalBlendDescription(FALSE, TRUE, TRUE);
    description.SampleMask = UINT_MAX;
    description.RasterizerState = CanonicalRasterizerDescription();
    description.DepthStencilState = CanonicalDepthStencilDescription();
    description.InputLayout = {
        input_elements, static_cast<UINT>(std::size(input_elements))};
    description.IBStripCutValue =
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    description.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    description.NumRenderTargets = 1;
    description.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    description.SampleDesc.Count = 1;

    m_pFramework->m_pDevice->CreateGraphicsPipelineState(
        &description,
        IID_PPV_ARGS(&m_pFramework->m_pTransparentPipelineState));
    m_pFramework->m_pTransparentPipelineState->SetName(
        L"Transparent Pipeline State");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC glass_description = description;
    glass_description.DepthStencilState.DepthWriteMask =
        D3D12_DEPTH_WRITE_MASK_ZERO;
    const HRESULT result = m_pFramework->m_pDevice->
        CreateGraphicsPipelineState(
            &glass_description,
            IID_PPV_ARGS(
                &m_pFramework->m_pTransparentGlassPipelineState));
    m_pFramework->m_pTransparentGlassPipelineState->SetName(
        L"Transparent Glass Pipeline State");
    return result;
}

/* 0x32C00, 3 bytes, global, 1 named locals
 * CD3DApplication::DeleteDeviceObjects
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.h
 */
HRESULT CD3DApplication::DeleteDeviceObjects()
{
    return S_OK;
}

/* 0x32C10, 233 bytes, global, 1 named locals
 * CD3DApplication::DestroyD3D12Objects
 * PDB type: void CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::DestroyD3D12Objects()
{
    WaitForGpu();

    if (m_pFramework->m_pRenderTargets[0] != nullptr) {
        m_pFramework->m_pRenderTargets[0]->Release();
        m_pFramework->m_pRenderTargets[0] = nullptr;
    }
    if (m_pFramework->m_pRenderTargets[1] != nullptr) {
        m_pFramework->m_pRenderTargets[1]->Release();
        m_pFramework->m_pRenderTargets[1] = nullptr;
    }
    if (m_pFramework->m_pRtvDescriptorHeap != nullptr) {
        m_pFramework->m_pRtvDescriptorHeap->Release();
        m_pFramework->m_pRtvDescriptorHeap = nullptr;
    }
    if (m_pFramework->m_pSwapChain != nullptr) {
        m_pFramework->m_pSwapChain->Release();
        m_pFramework->m_pSwapChain = nullptr;
    }
    if (m_pFramework->m_pDevice != nullptr) {
        m_pFramework->m_pDevice->Release();
        m_pFramework->m_pDevice = nullptr;
    }
    if (m_pFramework->m_pFactory != nullptr) {
        m_pFramework->m_pFactory->Release();
        m_pFramework->m_pFactory = nullptr;
    }

    auto *bytes = reinterpret_cast<unsigned char *>(this);
    void *window = *reinterpret_cast<void **>(bytes + 0xD8);
    if (window != nullptr) {
        DestroyCanonicalSDLWindow(window);
        *reinterpret_cast<void **>(bytes + 0xD8) = nullptr;
    }
    QuitCanonicalSDL();
}

/* 0x32D00, 608 bytes, global, 4 named locals
 * CD3DApplication::DisplayFrameworkError
 * PDB type: void CD3DApplication::(HRESULT, ...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

/* 0x32F60, 1535 bytes, global, 24 named locals
 * CD3DApplication::DrawLevel
 * PDB type: void CD3DApplication::(MATRIX*, ...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::DrawLevel(
    MATRIX *world_matrix,
    MATRIX *,
    MATRIX *,
    int)
{
    if (world_matrix == nullptr) {
        return;
    }

    auto *bytes = reinterpret_cast<unsigned char *>(this);
    auto *meshes = reinterpret_cast<std::vector<FBX_MESH *> *>(
        bytes + 0x3103A8);
    if (meshes->empty()) {
        return;
    }

    PrepareCanonicalLevelDraw(
        this,
        m_pFramework->m_pLevelPipelineState,
        false,
        true);
    int &total_indices =
        *reinterpret_cast<int *>(bytes + 0x3103A0);
    total_indices = 0;
    int base_vertex = 0;
    const int level_index = cullmesh[31] == 8 ? 8 : -1;

    for (std::size_t mesh_index = 0;
         mesh_index < meshes->size();
         ++mesh_index) {
        FBX_MESH *mesh = (*meshes)[mesh_index];
        for (std::size_t submesh_index = 0;
             submesh_index < mesh->subMeshes.size();
             ++submesh_index) {
            SubMeshSet &submesh = mesh->subMeshes[submesh_index];
            if (jpb_ShouldDrawFbxMesh(
                    level_index,
                    JPB_LEVEL_FBX_PASS_OPAQUE,
                    meshes->size(),
                    mesh_index,
                    mesh->name)) {
                m_pFramework->m_pCommandList->DrawIndexedInstanced(
                    static_cast<UINT>(submesh.subMeshIndices.size()),
                    1,
                    static_cast<UINT>(total_indices),
                    base_vertex,
                    0);
            }
            base_vertex += static_cast<int>(submesh.vertices.size());
            total_indices +=
                static_cast<int>(submesh.subMeshIndices.size());
        }
    }
    m_pFramework->m_pCommandList->SetPipelineState(
        m_pFramework->m_pPipelineState);
}

/* 0x33560, 1446 bytes, global, 23 named locals
 * CD3DApplication::DrawLevelTransparent
 * PDB type: void CD3DApplication::(MATRIX*, ...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::DrawLevelTransparent(
    MATRIX *world_matrix,
    MATRIX *,
    MATRIX *,
    int)
{
    if (world_matrix == nullptr) {
        return;
    }

    auto *bytes = reinterpret_cast<unsigned char *>(this);
    auto *meshes = reinterpret_cast<std::vector<FBX_MESH *> *>(
        bytes + 0x3103C0);
    if (meshes->empty()) {
        return;
    }

    PrepareCanonicalLevelDraw(
        this,
        m_pFramework->m_pTransparentPipelineState,
        true,
        false);
    const int total_opaque_indices =
        *reinterpret_cast<int *>(bytes + 0x3103A0);
    int &total_transparent_indices =
        *reinterpret_cast<int *>(bytes + 0x3103A4);
    total_transparent_indices = 0;
    const int level_index = cullmesh[31] == 8 ? 8 : -1;

    for (std::size_t mesh_index = 0;
         mesh_index < meshes->size();
         ++mesh_index) {
        FBX_MESH *mesh = (*meshes)[mesh_index];
        for (std::size_t submesh_index = 0;
             submesh_index < mesh->subMeshes.size();
             ++submesh_index) {
            SubMeshSet &submesh = mesh->subMeshes[submesh_index];
            const int draw_offset =
                total_opaque_indices + total_transparent_indices;
            if (jpb_ShouldDrawFbxMesh(
                    level_index,
                    JPB_LEVEL_FBX_PASS_TRANSPARENT,
                    meshes->size(),
                    mesh_index,
                    mesh->name)) {
                m_pFramework->m_pCommandList->DrawIndexedInstanced(
                    static_cast<UINT>(submesh.subMeshIndices.size()),
                    1,
                    static_cast<UINT>(draw_offset),
                    draw_offset,
                    0);
            }
            total_transparent_indices +=
                static_cast<int>(submesh.subMeshIndices.size());
        }
    }
    m_pFramework->m_pCommandList->SetPipelineState(
        m_pFramework->m_pPipelineState);
}

/* 0x33B10, 1422 bytes, global, 24 named locals
 * CD3DApplication::DrawLevelTransparentGlass
 * PDB type: void CD3DApplication::(MATRIX*, ...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::DrawLevelTransparentGlass(
    MATRIX *world_matrix,
    MATRIX *,
    MATRIX *,
    int)
{
    if (world_matrix == nullptr) {
        return;
    }

    auto *bytes = reinterpret_cast<unsigned char *>(this);
    auto *meshes = reinterpret_cast<std::vector<FBX_MESH *> *>(
        bytes + 0x3103D8);
    if (meshes->empty()) {
        return;
    }

    PrepareCanonicalLevelDraw(
        this,
        m_pFramework->m_pTransparentGlassPipelineState,
        true,
        false);
    const int preceding_indices =
        *reinterpret_cast<int *>(bytes + 0x3103A0) +
        *reinterpret_cast<int *>(bytes + 0x3103A4);
    int glass_indices = 0;
    const int level_index = cullmesh[31] == 8 ? 8 : -1;

    for (std::size_t mesh_index = 0;
         mesh_index < meshes->size();
         ++mesh_index) {
        FBX_MESH *mesh = (*meshes)[mesh_index];
        for (std::size_t submesh_index = 0;
             submesh_index < mesh->subMeshes.size();
             ++submesh_index) {
            SubMeshSet &submesh = mesh->subMeshes[submesh_index];
            const int draw_offset = preceding_indices + glass_indices;
            if (jpb_ShouldDrawFbxMesh(
                    level_index,
                    JPB_LEVEL_FBX_PASS_GLASS,
                    meshes->size(),
                    mesh_index,
                    mesh->name)) {
                m_pFramework->m_pCommandList->DrawIndexedInstanced(
                    static_cast<UINT>(submesh.subMeshIndices.size()),
                    1,
                    static_cast<UINT>(draw_offset),
                    draw_offset,
                    0);
            }
            glass_indices +=
                static_cast<int>(submesh.subMeshIndices.size());
        }
    }
    m_pFramework->m_pCommandList->SetPipelineState(
        m_pFramework->m_pPipelineState);
}

/* 0x340A0, 89 bytes, global, 2 named locals
 * CD3DApplication::DrawTexture
 * PDB type: void CD3DApplication::(const Spr...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::DisplayFrameworkError(
    HRESULT result, unsigned long type)
{
    const char *message;
    switch (static_cast<unsigned long>(result)) {
    case 0x8007000EUL:
        message = "Not enough memory!";
        break;
    case 0x81000001UL:
        message = "Could not create DirectDraw!";
        break;
    case 0x81000002UL:
        message =
            "Enumeration failed. Your system may be in an\n"
            "unstable state and need to be rebooted";
        break;
    case 0x81000003UL:
        message =
            "Could not find any compatible devices.\n\n"
            "Try enabling the reference rasterizer using\n"
            "EnableRefRast.reg.";
        break;
    case 0x81000004UL:
        message = "Could not find any compatible Direct3D devices.";
        break;
    case 0x82000000UL:
        message =
            "Generic initialization error.\n\n"
            "Enable debug output for detailed information.";
        break;
    case 0x82000001UL:
        message = "No DirectDraw";
        break;
    case 0x82000002UL:
        message = "No Direct3D";
        break;
    case 0x82000003UL:
        message =
            "This program requires a 16-bit (or higher) display mode\n"
            "to run in a window.\n\n"
            "Please switch your desktop settings accordingly.";
        break;
    case 0x82000004UL:
        message = "Could not set Cooperative Level";
        break;
    case 0x82000005UL:
        message = "Could not create the Direct3DDevice object.";
        break;
    case 0x82000006UL:
        message = "No ZBuffer";
        break;
    case 0x82000007UL:
        message =
            "Invalid Z-buffer depth. Try switching modes\n"
            "from 16- to 32-bit (or vice versa)";
        break;
    case 0x82000008UL:
        message = "No Viewport";
        break;
    case 0x82000009UL:
        message = "No primary";
        break;
    case 0x8200000AUL:
        message = "No Clipper";
        break;
    case 0x8200000BUL:
        message = "Bad display mode";
        break;
    case 0x8200000CUL:
        message = "No backbuffer";
        break;
    case 0x8200000DUL:
        message =
            "A DDraw object has a non-zero reference\n"
            "count (meaning it was not properly cleaned up).";
        break;
    case 0x8200000EUL:
        message = "No render target";
        break;
    case 0x8876017CUL:
        message =
            "There was insufficient video memory to use the\n"
            "hardware device.";
        break;
    default:
        message =
            "Generic application error.\n\n"
            "Enable debug output for detailed information.";
        break;
    }

    char text[512];
    lstrcpyA(text, message);
    if (static_cast<unsigned long>(result) == 0x82000005UL && type == 2) {
        lstrcatA(
            text,
            "\nThe 3D hardware chipset may not support\n"
            "rendering in the current display mode.");
    }

    UINT flags = MB_ICONEXCLAMATION;
    if (type == 1) {
        lstrcatA(text, "\n\nThis program will now exit.");
        flags = MB_ICONHAND;
    } else if (type == 2) {
        lstrcatA(text, "\n\nAttempting software rasterization.");
    }

    const char *caption = reinterpret_cast<const char *>(this) + 0x38308;
    ShowCanonicalMessageBox(nullptr, text, caption, flags);
}
void CD3DApplication::DrawTexture(const SpriteDraw &sprite_draw)
{
    auto *draws = reinterpret_cast<std::vector<SpriteDraw> *>(
        reinterpret_cast<unsigned char *>(this) + 0x58);
    draws->push_back(sprite_draw);
}

/* 0x34100, 175 bytes, global, 6 named locals
 * CD3DApplication::ERRTRACE
 * PDB type: void CD3DApplication::(char*, in...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::ERRTRACE(
    char *file, int line, char *format, ...)
{
    char formatted[256];
    char message[256];
    va_list arguments;
    va_start(arguments, format);
    std::vsprintf(formatted, format, arguments);
    va_end(arguments);
    std::sprintf(message, "%s(%d): %s\n", file, line, formatted);
    OutputDebugStringA(message);
}

/* 0x341B0, 1072 bytes, global, 8 named locals
 * CD3DApplication::EndRender
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::EndRender()
{
    const UINT frame_index = m_pFramework->m_nFrameIndex;
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource =
        m_pFramework->m_pRenderTargets[frame_index];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    m_pFramework->m_pCommandList->ResourceBarrier(1, &barrier);

    m_pFramework->TryCloseCommandList();
    ID3D12CommandList *command_lists[] = {m_pFramework->m_pCommandList};
    m_pFramework->m_pCommandQueue->ExecuteCommandLists(1, command_lists);

    if (resolutionUpdated == 0) {
        return S_OK;
    }
    resolutionUpdated = 0;

    void *window = m_pFramework->m_pSDLWindow;
    if (newWindowMode == 0) {
        GetCanonicalSDLShowCursor()(0);
        GetCanonicalSDLSetWindowGrab()(window, 1);
        GetCanonicalSDLSetWindowFullscreen()(window, 0x1001);
    } else if (newWindowMode == 1) {
        GetCanonicalSDLShowCursor()(1);
        GetCanonicalSDLSetWindowGrab()(window, 0);
        GetCanonicalSDLSetWindowFullscreen()(window, 1);
    } else {
        GetCanonicalSDLShowCursor()(1);
        GetCanonicalSDLSetWindowGrab()(window, 0);
        GetCanonicalSDLSetWindowFullscreen()(window, 0);
        GetCanonicalSDLSetWindowBordered()(window, 1);
    }

    const int display_index =
        GetCanonicalSDLGetWindowDisplayIndex()(window);
    GetCanonicalSDLGetCurrentDisplayMode()(
        display_index, &canonical_display_mode);
    GetCanonicalSDLGetDisplayBounds()(
        display_index, &canonical_display_bounds);
    GetCanonicalSDLSetWindowSize()(window, newWidth, newHeight);
    GetCanonicalSDLSetWindowPosition()(
        window,
        canonical_display_bounds.x +
            canonical_display_bounds.width / 2 - newWidth / 2,
        canonical_display_bounds.y +
            canonical_display_bounds.height / 2 - newHeight / 2);

    for (UINT index = 0; index < 2; ++index) {
        const std::uint64_t fence_value =
            ++m_pFramework->m_nFenceValues[index];
        m_pFramework->m_pCommandQueue->Signal(
            m_pFramework->m_pFence[index], fence_value);
        if (m_pFramework->m_pFence[index]->GetCompletedValue() <
            fence_value) {
            m_pFramework->m_pFence[index]->SetEventOnCompletion(
                fence_value, m_pFramework->m_hFenceEvent);
            WaitForSingleObject(m_pFramework->m_hFenceEvent, INFINITE);
        }
    }

    m_pFramework->m_nFrameIndex = 0;
    if (m_pFramework != nullptr) {
        const UINT current_frame = m_pFramework->m_nFrameIndex;
        const std::uint64_t fence_value =
            m_pFramework->m_nFenceValues[current_frame];
        if (SUCCEEDED(m_pFramework->m_pCommandQueue->Signal(
                m_pFramework->m_pFence[current_frame], fence_value)) &&
            SUCCEEDED(m_pFramework->m_pFence[current_frame]->
                SetEventOnCompletion(
                    fence_value, m_pFramework->m_hFenceEvent))) {
            WaitForSingleObject(m_pFramework->m_hFenceEvent, INFINITE);
            ++m_pFramework->m_nFenceValues[current_frame];
        }
    }

    CanonicalSDLSysWMinfo window_information;
    window_information.major = 2;
    window_information.minor = 26;
    window_information.patch = 5;
    if (GetCanonicalSDLGetWindowWMInfo()(
            window, &window_information) != 0) {
        auto *bytes = reinterpret_cast<unsigned char *>(this);
        *reinterpret_cast<HWND *>(bytes + 0xD0) =
            window_information.window;
        m_pFramework->m_hWnd = window_information.window;
    }

    ResizeCanonicalResources(
        m_pFramework,
        static_cast<UINT>(newWidth),
        static_cast<UINT>(newHeight));

    float field_of_view;
    const std::uint32_t field_of_view_bits = 0x3F6CCE68;
    std::memcpy(
        &field_of_view, &field_of_view_bits, sizeof(field_of_view));
    const float aspect =
        static_cast<float>(m_pFramework->m_dwRenderWidth) /
        static_cast<float>(m_pFramework->m_dwRenderHeight);
    const DirectX::XMMATRIX projection =
        DirectX::XMMatrixPerspectiveFovLH(
            field_of_view, aspect, 1.0f, 10000.0f);
    std::memcpy(
        reinterpret_cast<unsigned char *>(this) + 0x310430,
        &projection,
        sizeof(projection));
    return S_OK;
}

/* 0x345E0, 3 bytes, global, 1 named locals
 * CD3DApplication::FinalCleanup
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.h
 */
HRESULT CD3DApplication::FinalCleanup()
{
    return S_OK;
}

/* 0x345F0, 205 bytes, global, 3 named locals
 * CD3DApplication::FrameBegin
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

/* 0x346C0, 2250 bytes, global, 26 named locals
 * CD3DApplication::FrameEnd
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::FrameEnd()
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    auto *level_meshes = reinterpret_cast<std::vector<FBX_MESH *> *>(
        bytes + 0x3103A8);
    ID3D12GraphicsCommandList *command_list =
        m_pFramework->m_pCommandList;

    auto bind_level_targets = [&]() {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv =
            m_pFramework->m_pRtvDescriptorHeap
                ->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += static_cast<SIZE_T>(
            m_pFramework->m_RTVDescriptorSize *
            m_pFramework->m_nFrameIndex);
        const D3D12_CPU_DESCRIPTOR_HANDLE dsv =
            m_pFramework->m_pDsDescriptorHeap
                ->GetCPUDescriptorHandleForHeapStart();
        command_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    };

    if (!level_meshes->empty()) {
        bind_level_targets();
        DrawLevel(worldTURTLEMatrix, nullptr, nullptr, 1);
    }

    int &num_tris = *reinterpret_cast<int *>(bytes + 0x3C);
    int &num_quads = *reinterpret_cast<int *>(bytes + 0x40);
    if (num_tris > 0 || num_quads > 0) {
        constexpr UINT vertex_buffer_size = 0x2D8000;
        constexpr UINT index_buffer_size = 0x38000;
        m_pFramework->m_vertexBufferSize = vertex_buffer_size;
        m_pFramework->m_indexBufferSize = index_buffer_size;

        if (m_pFramework->m_vertexBufferSize > 0 &&
            m_pFramework->m_indexBufferSize > 0) {
            auto release_resource = [](ID3D12Resource *&resource) {
                if (resource != nullptr) {
                    resource->Release();
                    resource = nullptr;
                }
            };

            release_resource(m_pFramework->m_vertexUploadBuffer);
            release_resource(m_pFramework->m_indexUploadBuffer);
            release_resource(m_pFramework->m_3DVertexBuffer);
            release_resource(m_pFramework->m_3DIndexBuffer);

            D3D12_HEAP_PROPERTIES upload_heap = {};
            upload_heap.Type = D3D12_HEAP_TYPE_UPLOAD;
            upload_heap.CreationNodeMask = 1;
            upload_heap.VisibleNodeMask = 1;
            D3D12_HEAP_PROPERTIES default_heap = upload_heap;
            default_heap.Type = D3D12_HEAP_TYPE_DEFAULT;

            auto buffer_description = [](UINT size) {
                D3D12_RESOURCE_DESC description = {};
                description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                description.Width = size;
                description.Height = 1;
                description.DepthOrArraySize = 1;
                description.MipLevels = 1;
                description.SampleDesc.Count = 1;
                description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                return description;
            };
            const D3D12_RESOURCE_DESC vertex_description =
                buffer_description(vertex_buffer_size);
            const D3D12_RESOURCE_DESC index_description =
                buffer_description(index_buffer_size);

            (void)m_pFramework->m_pDevice->CreateCommittedResource(
                &upload_heap,
                D3D12_HEAP_FLAG_NONE,
                &vertex_description,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pFramework->m_vertexUploadBuffer));
            (void)m_pFramework->m_pDevice->CreateCommittedResource(
                &upload_heap,
                D3D12_HEAP_FLAG_NONE,
                &index_description,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_pFramework->m_indexUploadBuffer));
            (void)m_pFramework->m_pDevice->CreateCommittedResource(
                &default_heap,
                D3D12_HEAP_FLAG_NONE,
                &vertex_description,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&m_pFramework->m_3DVertexBuffer));
            (void)m_pFramework->m_pDevice->CreateCommittedResource(
                &default_heap,
                D3D12_HEAP_FLAG_NONE,
                &index_description,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&m_pFramework->m_3DIndexBuffer));

            void *mapped_vertices;
            void *mapped_indices;
            (void)m_pFramework->m_vertexUploadBuffer->Map(
                0, nullptr, &mapped_vertices);
            (void)m_pFramework->m_indexUploadBuffer->Map(
                0, nullptr, &mapped_indices);
            std::memcpy(
                mapped_vertices, bytes + 0x1703A0, 0x1A0000);
            std::memcpy(
                static_cast<unsigned char *>(mapped_vertices) + 0x1A0000,
                bytes + 0x383A0,
                0x138000);
            std::memcpy(
                mapped_indices, bytes + 0x18100, 0x20000);
            std::memcpy(
                static_cast<unsigned char *>(mapped_indices) + 0x20000,
                bytes + 0x100,
                0x18000);
            m_pFramework->m_vertexUploadBuffer->Unmap(0, nullptr);
            m_pFramework->m_indexUploadBuffer->Unmap(0, nullptr);

            command_list->CopyBufferRegion(
                m_pFramework->m_3DVertexBuffer,
                0,
                m_pFramework->m_vertexUploadBuffer,
                0,
                vertex_buffer_size);
            command_list->CopyBufferRegion(
                m_pFramework->m_3DIndexBuffer,
                0,
                m_pFramework->m_indexUploadBuffer,
                0,
                index_buffer_size);

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.Subresource =
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.pResource =
                m_pFramework->m_3DVertexBuffer;
            barrier.Transition.StateBefore =
                D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter =
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            command_list->ResourceBarrier(1, &barrier);
            barrier.Transition.pResource =
                m_pFramework->m_3DIndexBuffer;
            barrier.Transition.StateAfter =
                D3D12_RESOURCE_STATE_INDEX_BUFFER;
            command_list->ResourceBarrier(1, &barrier);

            D3D12_VERTEX_BUFFER_VIEW vertex_view = {};
            vertex_view.BufferLocation =
                m_pFramework->m_3DVertexBuffer
                    ->GetGPUVirtualAddress();
            vertex_view.SizeInBytes = vertex_buffer_size;
            vertex_view.StrideInBytes = sizeof(Vertex);
            D3D12_INDEX_BUFFER_VIEW index_view = {};
            index_view.BufferLocation =
                m_pFramework->m_3DIndexBuffer
                    ->GetGPUVirtualAddress();
            index_view.SizeInBytes = index_buffer_size;
            index_view.Format = DXGI_FORMAT_R16_UINT;

            command_list->SetPipelineState(
                m_pFramework->m_pPipelineState);
            command_list->IASetPrimitiveTopology(
                D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            command_list->IASetVertexBuffers(0, 1, &vertex_view);
            command_list->IASetIndexBuffer(&index_view);

            for (int index = 0; index < num_quads * 4; index += 4) {
                command_list->DrawIndexedInstanced(
                    4, 1, static_cast<UINT>(index), 0, 0);
            }
            for (int index = 0; index < num_tris * 3; index += 3) {
                command_list->DrawIndexedInstanced(
                    3, 1, static_cast<UINT>(index + 0x8000), 0, 0);
            }
            *reinterpret_cast<int *>(bytes + 0x38100) = 0;
        }
    }

    if (!level_meshes->empty()) {
        bind_level_targets();
        DrawLevelTransparent(worldTURTLEMatrix, nullptr, nullptr, 1);
    }

    auto *transparency_pass =
        *reinterpret_cast<D3DTransparencyPass **>(bytes + 0xF0);
    transparency_pass->Update();
    transparency_pass->Render();

    if (!level_meshes->empty()) {
        bind_level_targets();
        DrawLevelTransparentGlass(worldTURTLEMatrix, nullptr, nullptr, 1);
    }

    (void)RenderUI();
    HRESULT result = EndRender();
    if (FAILED(result)) {
        return result;
    }

    result = m_pFramework->Present();
    if (FAILED(result)) {
        result = m_pFramework->m_pDevice->GetDeviceRemovedReason();
        if (result == DXGI_ERROR_DEVICE_REMOVED) {
            RECT client_rectangle;
            GetClientRect(
                *reinterpret_cast<HWND *>(bytes + 0xD0),
                &client_rectangle);
            const UINT width = static_cast<UINT>(
                client_rectangle.right - client_rectangle.left);
            const UINT height = static_cast<UINT>(
                client_rectangle.bottom - client_rectangle.top);
            WaitForGpu();
            (void)m_pFramework->ResizeResources(width, height);
        }
    }

    CommitCanonicalGraphicsMemory(
        m_pFramework->m_graphicsMemory,
        m_pFramework->m_pCommandQueue);
    MoveToNextFrame();
    std::memset(bytes + 0x100, 0, 0x18000);
    std::memset(bytes + 0x18100, 0, 0x20000);
    *reinterpret_cast<std::uint64_t *>(bytes + 0x3C) = 0;
    return result;
}

/* 0x34F90, 259 bytes, global, 9 named locals
 * CD3DApplication::GetRegistryBinary
 * PDB type: int CD3DApplication::(char*, voi...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::FrameBegin()
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    *reinterpret_cast<int *>(bytes + 0x310970) = 0;

    HWND window = *reinterpret_cast<HWND *>(bytes + 0xD0);
    if (window != nullptr) {
        MSG message = {};
        while (PeekMessageA(
            &message, window, 0, 0, PM_REMOVE) != FALSE) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    if (*reinterpret_cast<HWND *>(bytes + 0xD0) == nullptr) {
        ExitProcess(0);
    }
    if (m_pFramework->m_pDevice == nullptr) {
        return S_OK;
    }
    const HRESULT result = StartRender();
    return FAILED(result) ? result : S_OK;
}
int CD3DApplication::GetRegistryBinary(
    char *name, void *data, unsigned size)
{
    HKEY key;
    if (RegOpenKeyExA(
            HKEY_CURRENT_USER, RegKeyName(), 0, KEY_ALL_ACCESS, &key) !=
        ERROR_SUCCESS) {
        return 0;
    }
    DWORD type;
    DWORD returned_size = size;
    const LSTATUS status = RegQueryValueExA(
        key, name, nullptr, &type, static_cast<BYTE *>(data), &returned_size);
    int result = 0;
    if (status != ERROR_SUCCESS) {
        SystemErrorReport(
            const_cast<char *>(
                "W:\\SWJediPowerBattles\\work\\d3d\\d3dapp.cpp"),
            0xE5D,
            status,
            const_cast<char *>("RegQueryValueEx"));
    } else if (type == REG_BINARY && returned_size <= size) {
        result = 1;
    } else {
        ERRTRACE(
            const_cast<char *>(
                "W:\\SWJediPowerBattles\\work\\d3d\\d3dapp.cpp"),
            0xE5F,
            const_cast<char *>("Incorrect registry datatype"));
    }
    RegCloseKey(key);
    return result;
}

/* 0x350A0, 155 bytes, global, 7 named locals
 * CD3DApplication::GetRegistryDWord
 * PDB type: unsigned long CD3DApplication::(...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
unsigned long CD3DApplication::GetRegistryDWord(
    char *name, unsigned long default_value)
{
    unsigned long value = default_value;
    HKEY key;
    if (RegOpenKeyExA(
            HKEY_CURRENT_USER, RegKeyName(), 0, KEY_ALL_ACCESS, &key) ==
        ERROR_SUCCESS) {
        DWORD type;
        DWORD size = sizeof(value);
        if (RegQueryValueExA(
                key,
                name,
                nullptr,
                &type,
                reinterpret_cast<BYTE *>(&value),
                &size) != ERROR_SUCCESS ||
            type != REG_DWORD) {
            value = default_value;
        }
        RegCloseKey(key);
    }
    return value;
}

/* 0x35140, 149 bytes, global, 8 named locals
 * CD3DApplication::GetRegistryString
 * PDB type: int CD3DApplication::(char*, cha...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
int CD3DApplication::GetRegistryString(
    char *name, char *value, unsigned)
{
    HKEY key;
    if (RegOpenKeyExA(
            HKEY_CURRENT_USER, RegKeyName(), 0, KEY_ALL_ACCESS, &key) !=
        ERROR_SUCCESS) {
        return 0;
    }
    DWORD type;
    DWORD size;
    const LSTATUS status = RegQueryValueExA(
        key,
        name,
        nullptr,
        &type,
        reinterpret_cast<BYTE *>(value),
        &size);
    const int result = status == ERROR_SUCCESS && type == REG_SZ;
    RegCloseKey(key);
    return result;
}

static void InitializeCanonicalApplicationSceneBuffer(
    CD3DApplication *application)
{
    auto *bytes = reinterpret_cast<unsigned char *>(application);
    CD3DFramework12 *framework = application->m_pFramework;
    const float aspect_ratio =
        static_cast<float>(framework->m_dwRenderWidth) /
        static_cast<float>(framework->m_dwRenderHeight);
    *reinterpret_cast<DirectX::XMMATRIX *>(bytes + 0x310430) =
        DirectX::XMMatrixPerspectiveFovLH(
            0.9250245f, aspect_ratio, 1.0f, 10000.0f);
    *reinterpret_cast<DirectX::XMMATRIX *>(bytes + 0x3103F0) =
        DirectX::XMMatrixIdentity();

    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heap_properties.CreationNodeMask = 1;
    heap_properties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC buffer_description = {};
    buffer_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_description.Width = 0x10000;
    buffer_description.Height = 1;
    buffer_description.DepthOrArraySize = 1;
    buffer_description.MipLevels = 1;
    buffer_description.SampleDesc.Count = 1;
    buffer_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    framework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &buffer_description,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&framework->m_constantBuffer));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_description = {};
    cbv_description.BufferLocation =
        framework->m_constantBuffer->GetGPUVirtualAddress();
    cbv_description.SizeInBytes = 0x100;
    framework->m_pDevice->CreateConstantBufferView(
        &cbv_description,
        framework->m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

    const D3D12_RANGE read_range = {0, 0};
    framework->m_constantBuffer->Map(
        0,
        &read_range,
        reinterpret_cast<void **>(&framework->m_pCbvDataBegin));
    std::memcpy(framework->m_pCbvDataBegin, bytes + 0x310470, 0x100);
    framework->m_vertexUploadBuffer = nullptr;
    framework->m_indexUploadBuffer = nullptr;
}

#ifdef JPB_D3DAPP_TESTING
void jpb_d3dapp_initialize_scene_buffer_for_test(
    CD3DApplication *application)
{
    InitializeCanonicalApplicationSceneBuffer(application);
}
#endif

/* 0x351E0, 3709 bytes, global, 56 named locals
 * CD3DApplication::InitD3D12Framework
 * PDB type: HRESULT CD3DApplication::(HWND__...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::InitD3D12Framework(HWND window)
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);

    HRESULT result = CreateCanonicalDXGIFactory2(
        0, IID_PPV_ARGS(&m_pFramework->m_pFactory));
    if (FAILED(result)) {
        return result;
    }

    result = InitializeDevice();
    if (FAILED(result)) {
        return result;
    }

    m_pFramework->m_graphicsMemory =
        new DirectX::DX12::GraphicsMemory(m_pFramework->m_pDevice);

    result = InitializeSwapChain(window);
    if (FAILED(result)) {
        return result;
    }
    result = InitializeRenderTargets();
    if (FAILED(result)) {
        return result;
    }
    result = InitializeCommandList();
    if (FAILED(result)) {
        return result;
    }
    result = CreateFence();
    if (FAILED(result)) {
        return result;
    }
    result = InitializeRootSignature();
    if (FAILED(result)) {
        return result;
    }
    result = InitializeDepthStencilBuffer();
    if (FAILED(result)) {
        return result;
    }
    result = CreateLevelPipelineStateObject();
    if (FAILED(result)) {
        return result;
    }
    result = CreateTransparentPipelineStateObject();
    if (FAILED(result)) {
        return result;
    }
    result = CreatePipelineStateObject();
    if (FAILED(result)) {
        return result;
    }

    InitializeViewportAndScissorRect();
    result = CreateSRVHeap();
    if (FAILED(result)) {
        return result;
    }

    auto &transparency_pass =
        *reinterpret_cast<std::unique_ptr<D3DTransparencyPass> *>(
            bytes + 0xF0);
    transparency_pass =
        std::make_unique<D3DTransparencyPass>(m_pFramework);

    InitializeCanonicalApplicationSceneBuffer(this);

    DirectX::ResourceUploadBatch resource_upload(m_pFramework->m_pDevice);
    resource_upload.Begin(D3D12_COMMAND_LIST_TYPE_DIRECT);

    auto &states =
        *reinterpret_cast<std::unique_ptr<DirectX::DX12::CommonStates> *>(
            bytes + 0x50);
    states = std::make_unique<DirectX::DX12::CommonStates>(
        m_pFramework->m_pDevice);
    const D3D12_GPU_DESCRIPTOR_HANDLE sampler = states->PointClamp();

    const DirectX::RenderTargetState render_target_state(
        m_pFramework->m_pSwapChainDesc, DXGI_FORMAT_UNKNOWN);
    const DirectX::DX12::SpriteBatchPipelineStateDescription
        pipeline_description(render_target_state, nullptr, nullptr, nullptr,
                             &sampler);
    auto &sprite_batch =
        *reinterpret_cast<std::unique_ptr<DirectX::DX12::SpriteBatch> *>(
            bytes + 0x48);
    sprite_batch = std::make_unique<DirectX::DX12::SpriteBatch>(
        m_pFramework->m_pDevice,
        resource_upload,
        pipeline_description,
        nullptr);

    std::future<void> upload_resources_finished =
        resource_upload.End(m_pFramework->m_pCommandQueue);
    upload_resources_finished.wait();

    m_pFramework->m_commandListOpen = true;
    m_pFramework->TryCloseCommandList();
    ID3D12CommandList *command_lists[] = {m_pFramework->m_pCommandList};
    m_pFramework->m_pCommandQueue->ExecuteCommandLists(1, command_lists);
    MoveToNextFrame();
    WaitForGpuTexture(m_pFramework);
    return S_OK;
}

/* 0x36060, 3 bytes, global, 1 named locals
 * CD3DApplication::InitDeviceObjects
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.h
 */
HRESULT CD3DApplication::InitDeviceObjects()
{
    return S_OK;
}

/* 0x36070, 140 bytes, global, 3 named locals
 * CD3DApplication::InitializeCommandList
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::InitializeCommandList()
{
    for (unsigned index = 0; index < 2; ++index) {
        HRESULT result = m_pFramework->m_pDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_pFramework->m_pCommandAllocators[index]));
        if (FAILED(result)) {
            return result;
        }
    }

    return m_pFramework->m_pDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_pFramework->m_pCommandAllocators[
            m_pFramework->m_nCurrentFrameIndex],
        nullptr,
        IID_PPV_ARGS(&m_pFramework->m_pCommandList));
}

/* 0x36100, 406 bytes, global, 6 named locals
 * CD3DApplication::InitializeDepthStencilBuffer
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::InitializeDepthStencilBuffer()
{
    D3D12_DESCRIPTOR_HEAP_DESC heap_description = {};
    heap_description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heap_description.NumDescriptors = 1;
    HRESULT result = m_pFramework->m_pDevice->CreateDescriptorHeap(
        &heap_description,
        IID_PPV_ARGS(&m_pFramework->m_pDsDescriptorHeap));
    if (FAILED(result)) {
        return result;
    }

    const auto *bytes = reinterpret_cast<const unsigned char *>(this);
    D3D12_RESOURCE_DESC resource_description = {};
    resource_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_description.Width =
        *reinterpret_cast<const UINT *>(bytes + 0x3831C);
    resource_description.Height =
        *reinterpret_cast<const UINT *>(bytes + 0x38320);
    resource_description.DepthOrArraySize = 1;
    resource_description.MipLevels = 0;
    resource_description.Format = DXGI_FORMAT_D32_FLOAT;
    resource_description.SampleDesc.Count = 1;
    resource_description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resource_description.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clear_value = {};
    clear_value.Format = DXGI_FORMAT_D32_FLOAT;
    clear_value.DepthStencil.Depth = 1.0f;

    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_properties.CreationNodeMask = 1;
    heap_properties.VisibleNodeMask = 1;

    result = m_pFramework->m_pDevice->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &resource_description,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clear_value,
        IID_PPV_ARGS(&m_pFramework->m_pDepthStencil));

    D3D12_DEPTH_STENCIL_VIEW_DESC view_description = {};
    view_description.Format = DXGI_FORMAT_D32_FLOAT;
    view_description.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    m_pFramework->m_pDevice->CreateDepthStencilView(
        m_pFramework->m_pDepthStencil,
        &view_description,
        m_pFramework->m_pDsDescriptorHeap->
            GetCPUDescriptorHandleForHeapStart());
    return result;
}

/* 0x362A0, 523 bytes, global, 10 named locals
 * CD3DApplication::InitializeDevice
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::InitializeDevice()
{
    using Microsoft::WRL::ComPtr;

    DXGI_ADAPTER_DESC1 desc;
    ComPtr<IDXGIAdapter1> adapter;
    auto *factory = reinterpret_cast<IDXGIFactory6 *>(
        m_pFramework->m_pFactory);
    HRESULT result = factory->EnumAdapterByGpuPreference(
        0,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        IID_PPV_ARGS(adapter.GetAddressOf()));
    if (result != DXGI_ERROR_NOT_FOUND) {
        if (FAILED(result)) {
            return result;
        }
        adapter->GetDesc1(&desc);
    }

    m_pFramework->m_isAMD = desc.VendorId == 0x1002;

    const auto *bytes = reinterpret_cast<const unsigned char *>(this);
    if (*reinterpret_cast<const BOOL *>(bytes + 0x28) == FALSE) {
        CreateCanonicalD3D12Device(
            adapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_pFramework->m_pDevice));
    } else {
        IDXGIAdapter1 *p_adapter = nullptr;
        UINT adapter_index = 0;
        result = m_pFramework->m_pFactory->EnumAdapters1(
            adapter_index, &p_adapter);
        while (result != DXGI_ERROR_NOT_FOUND) {
            DXGI_ADAPTER_DESC1 candidate_desc;
            p_adapter->GetDesc1(&candidate_desc);
            if ((candidate_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
                result = CreateCanonicalD3D12Device(
                    p_adapter,
                    D3D_FEATURE_LEVEL_11_0,
                    IID_PPV_ARGS(&m_pFramework->m_pDevice));
                if (SUCCEEDED(result)) {
                    break;
                }
            }
            ++adapter_index;
            result = m_pFramework->m_pFactory->EnumAdapters1(
                adapter_index, &p_adapter);
        }
    }

    m_pFramework->m_hWnd =
        *reinterpret_cast<HWND const *>(bytes + 0xD0);
    m_pFramework->m_dwRenderWidth =
        *reinterpret_cast<const UINT *>(bytes + 0x3831C);
    m_pFramework->m_dwRenderHeight =
        *reinterpret_cast<const UINT *>(bytes + 0x38320);
    m_pFramework->m_bIsFullscreen = FALSE;
    m_pFramework->m_nFrameIndex = 0;

    RECT screen_rect;
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &screen_rect, 0);
    m_pFramework->m_rcScreenRect = screen_rect;

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS quality_levels;
    quality_levels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    quality_levels.SampleCount = 4;
    quality_levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    quality_levels.NumQualityLevels = 0;
    return m_pFramework->m_pDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &quality_levels,
        sizeof(quality_levels));
}

/* 0x364B0, 320 bytes, global, 6 named locals
 * CD3DApplication::InitializeRenderTargets
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::InitializeRenderTargets()
{
    D3D12_DESCRIPTOR_HEAP_DESC heap_description = {};
    heap_description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_description.NumDescriptors = 2;

    HRESULT result = m_pFramework->m_pDevice->CreateDescriptorHeap(
        &heap_description,
        IID_PPV_ARGS(&m_pFramework->m_pRtvDescriptorHeap));
    m_pFramework->m_pRtvDescriptorHeap->SetName(L"RTV Descriptor Heap");
    if (FAILED(result)) {
        return result;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptor =
        m_pFramework->m_pRtvDescriptorHeap->
            GetCPUDescriptorHandleForHeapStart();
    m_pFramework->m_RTVDescriptorSize =
        m_pFramework->m_pDevice->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    for (UINT index = 0; index < 2; ++index) {
        result = m_pFramework->m_pSwapChain->GetBuffer(
            index,
            IID_PPV_ARGS(&m_pFramework->m_pRenderTargets[index]));
        if (FAILED(result)) {
            return result;
        }
        m_pFramework->m_pDevice->CreateRenderTargetView(
            m_pFramework->m_pRenderTargets[index], nullptr, descriptor);
        descriptor.ptr += m_pFramework->m_RTVDescriptorSize;
    }
    return result;
}

/* 0x365F0, 469 bytes, global, 10 named locals
 * CD3DApplication::InitializeRootSignature
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::InitializeRootSignature()
{
    D3D12_DESCRIPTOR_RANGE ranges[1];
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ranges[0].NumDescriptors = 0x400;
    ranges[0].BaseShaderRegister = 0;
    ranges[0].RegisterSpace = 0;
    ranges[0].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_DESCRIPTOR root_cbv_descriptor;
    root_cbv_descriptor.ShaderRegister = 0;
    root_cbv_descriptor.RegisterSpace = 0;

    D3D12_ROOT_DESCRIPTOR_TABLE descriptor_table;
    descriptor_table.NumDescriptorRanges = 1;
    descriptor_table.pDescriptorRanges = ranges;

    D3D12_ROOT_PARAMETER root_parameters[3];
    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    root_parameters[0].Descriptor = root_cbv_descriptor;
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    root_parameters[1].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    root_parameters[1].DescriptorTable = descriptor_table;
    root_parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    root_parameters[2].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_parameters[2].Constants.ShaderRegister = 2;
    root_parameters[2].Constants.RegisterSpace = 0;
    root_parameters[2].Constants.Num32BitValues = 1;
    root_parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_STATIC_SAMPLER_DESC sampler;
    sampler.Filter = D3D12_FILTER_ANISOTROPIC;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = 8;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = FLT_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.NumParameters = 3;
    root_signature_desc.pParameters = root_parameters;
    root_signature_desc.NumStaticSamplers = 1;
    root_signature_desc.pStaticSamplers = &sampler;
    root_signature_desc.Flags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

    ID3DBlob *signature;
    ID3DBlob *error;
    HRESULT result = SerializeCanonicalRootSignature(
        &root_signature_desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &error);
    if (FAILED(result)) {
        return result;
    }

    const SIZE_T signature_size = signature->GetBufferSize();
    void *signature_data = signature->GetBufferPointer();
    result = m_pFramework->m_pDevice->CreateRootSignature(
        0,
        signature_data,
        signature_size,
        IID_PPV_ARGS(&m_pFramework->m_pRootSignature));
    m_pFramework->m_pRootSignature->SetName(L"Root Signature");
    return result;
}

/* 0x367D0, 421 bytes, global, 5 named locals
 * CD3DApplication::InitializeSwapChain
 * PDB type: HRESULT CD3DApplication::(HWND__...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::InitializeSwapChain(HWND window)
{
    D3D12_COMMAND_QUEUE_DESC queue_description = {};
    m_pFramework->m_pDevice->CreateCommandQueue(
        &queue_description,
        IID_PPV_ARGS(&m_pFramework->m_pCommandQueue));
    m_pFramework->m_pCommandQueue->SetName(L"Command Queue");

    m_pFramework->m_pSwapChainDesc = new DXGI_SWAP_CHAIN_DESC1();
    m_pFramework->m_pSwapChainDesc->BufferCount = 2;
    const auto *bytes = reinterpret_cast<const unsigned char *>(this);
    m_pFramework->m_pSwapChainDesc->Width =
        *reinterpret_cast<const UINT *>(bytes + 0x3831C);
    m_pFramework->m_pSwapChainDesc->Height =
        *reinterpret_cast<const UINT *>(bytes + 0x38320);
    m_pFramework->m_pSwapChainDesc->Format =
        DXGI_FORMAT_R8G8B8A8_UNORM;
    m_pFramework->m_pSwapChainDesc->BufferUsage =
        DXGI_USAGE_RENDER_TARGET_OUTPUT;
    m_pFramework->m_pSwapChainDesc->SwapEffect =
        DXGI_SWAP_EFFECT_FLIP_DISCARD;
    m_pFramework->m_pSwapChainDesc->SampleDesc.Count = 1;

    IDXGISwapChain1 *swap_chain = nullptr;
    HRESULT result = m_pFramework->m_pFactory->CreateSwapChainForHwnd(
        m_pFramework->m_pCommandQueue,
        window,
        m_pFramework->m_pSwapChainDesc,
        nullptr,
        nullptr,
        &swap_chain);
    if (SUCCEEDED(result)) {
        result = swap_chain->QueryInterface(
            IID_PPV_ARGS(&m_pFramework->m_pSwapChain));
        swap_chain->Release();
    }
    m_pFramework->m_nCurrentFrameIndex =
        m_pFramework->m_pSwapChain->GetCurrentBackBufferIndex();
    return result;
}

/* 0x36980, 154 bytes, global, 1 named locals
 * CD3DApplication::InitializeViewportAndScissorRect
 * PDB type: void CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::InitializeViewportAndScissorRect()
{
    const auto *bytes = reinterpret_cast<const unsigned char *>(this);
    const UINT width = *reinterpret_cast<const UINT *>(bytes + 0x3831C);
    const UINT height = *reinterpret_cast<const UINT *>(bytes + 0x38320);

    m_pFramework->m_Viewport.Width = static_cast<float>(width);
    m_pFramework->m_Viewport.Height = static_cast<float>(height);
    m_pFramework->m_Viewport.MaxDepth = 1.0f;
    m_pFramework->m_Viewport.MinDepth = 0.0f;
    m_pFramework->m_Viewport.TopLeftX = 0.0f;
    m_pFramework->m_Viewport.TopLeftY = 0.0f;
    m_pFramework->m_ScissorRect.left = 0;
    m_pFramework->m_ScissorRect.top = 0;
    m_pFramework->m_ScissorRect.right = width;
    m_pFramework->m_ScissorRect.bottom = height;
}

/* 0x36A20, 81 bytes, local, 5 named locals
 * Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl ABI::Windows::Foundation::IEventHandler_impl<IInspectable *>::*)(IInspectable *,IInspectable *)>::DelegateInvokeHelper<ABI::Windows::Foundation::IEventHandler<IInspectable *>,<lambda_4f71b6447fa15199118d91075b9a45ed>,1,IInspectable *,IInspectable *>::Invoke
 * PDB type: HRESULT Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\event.h
 */

/* 0x36A80, 86 bytes, local, 5 named locals
 * Microsoft::WRL::Details::DelegateArgTraits<long (__cdecl ABI::Windows::Foundation::IEventHandler_impl<IInspectable *>::*)(IInspectable *,IInspectable *)>::DelegateInvokeHelper<ABI::Windows::Foundation::IEventHandler<IInspectable *>,<lambda_675d0a2d57fd62d4f0e34fd501361d84>,1,IInspectable *,IInspectable *>::Invoke
 * PDB type: HRESULT Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\event.h
 */

/* 0x36AE0, 42 bytes, global, 2 named locals
 * CD3DApplication::IsWindowed
 * PDB type: bool CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
bool CD3DApplication::IsWindowed()
{
    void *window = *reinterpret_cast<void **>(
        reinterpret_cast<unsigned char *>(this) + 0xD8);
    return window != nullptr &&
        (GetCanonicalSDLWindowFlags(window) & 0x1001) == 0;
}

/* 0x36B10, 163 bytes, global, 2 named locals
 * CD3DApplication::MessagePump
 * PDB type: void CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::MessagePump()
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    HWND window = *reinterpret_cast<HWND *>(bytes + 0xD0);
    *reinterpret_cast<int *>(bytes + 0x310970) = 0;
    if (window != nullptr) {
        MSG message = {};
        while (PeekMessageA(&message, window, 0, 0, PM_REMOVE) != FALSE) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    if (*reinterpret_cast<HWND *>(bytes + 0xD0) == nullptr) {
        ExitProcess(0);
    }
}

/* 0x36BC0, 342 bytes, global, 6 named locals
 * CD3DApplication::MoveToNextFrame
 * PDB type: void CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::MoveToNextFrame()
{
    UINT frame_index = m_pFramework->m_nCurrentFrameIndex;
    const std::uint64_t fence_value =
        m_pFramework->m_nFenceValues[frame_index];
    if (FAILED(m_pFramework->m_pCommandQueue->Signal(
            m_pFramework->m_pFence[frame_index], fence_value))) {
        return;
    }

    m_pFramework->m_nCurrentFrameIndex =
        m_pFramework->m_pSwapChain->GetCurrentBackBufferIndex();
    frame_index = m_pFramework->m_nCurrentFrameIndex;
    m_pFramework->m_pFence[frame_index]->GetCompletedValue();
    if (m_pFramework->m_pFence[frame_index]->GetCompletedValue() <
        m_pFramework->m_nFenceValues[frame_index]) {
        if (FAILED(m_pFramework->m_pFence[frame_index]->
                SetEventOnCompletion(
                    m_pFramework->m_nFenceValues[frame_index],
                    m_pFramework->m_hFenceEvent))) {
            return;
        }
        WaitForSingleObject(m_pFramework->m_hFenceEvent, INFINITE);
    }
    m_pFramework->m_nFenceValues[frame_index] = fence_value + 1;

    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    const double elapsed =
        static_cast<double>(current_time.QuadPart - lastTime.QuadPart) /
        static_cast<double>(gFrequency.QuadPart);
    if (elapsed < gSecondsPerFrame) {
        Sleep(static_cast<DWORD>(
            (gSecondsPerFrame - elapsed) * 1000.0));
        QueryPerformanceCounter(&current_time);
    }
    lastTime = current_time;
}

/* 0x36D20, 1642 bytes, global, 11 named locals
 * CD3DApplication::MsgProc
 * PDB type: __int64 CD3DApplication::(HWND__...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
LRESULT CD3DApplication::MsgProc(
    HWND window,
    UINT message,
    WPARAM w_param,
    LPARAM l_param)
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    void **vtable = *reinterpret_cast<void ***>(this);

    switch (message) {
    case WM_DESTROY:
        *reinterpret_cast<HWND *>(bytes + 0xD0) = nullptr;
        Cleanup3DEnvironment();
        PostQuitMessage(0);
        return 0;

    case WM_MOVE:
    case WM_MOVING:
        if (m_pFramework != nullptr &&
            *reinterpret_cast<int *>(bytes + 0x20) != 0 &&
            *reinterpret_cast<int *>(bytes + 0x24) != 0 &&
            IsWindowed()) {
            m_pFramework->Move(
                static_cast<short>(LOWORD(l_param)),
                static_cast<short>(HIWORD(l_param)));
        }
        break;

    case WM_SIZE:
        if (w_param == SIZE_MAXHIDE || w_param == SIZE_MINIMIZED) {
            *reinterpret_cast<int *>(bytes + 0x20) = 0;
        } else {
            *reinterpret_cast<int *>(bytes + 0x20) = 1;
            if (*reinterpret_cast<int *>(bytes + 0x24) != 0 &&
                IsWindowed()) {
                *reinterpret_cast<int *>(bytes + 0x24) = 0;
                GetWindowRect(
                    *reinterpret_cast<HWND *>(bytes + 0xD0),
                    reinterpret_cast<RECT *>(bytes + 0x38334));
                GetClientRect(
                    *reinterpret_cast<HWND *>(bytes + 0xD0),
                    &m_pFramework->m_rcScreenRect);
                ClientToScreen(
                    *reinterpret_cast<HWND *>(bytes + 0xD0),
                    reinterpret_cast<POINT *>(
                        &m_pFramework->m_rcScreenRect.left));
                ClientToScreen(
                    *reinterpret_cast<HWND *>(bytes + 0xD0),
                    reinterpret_cast<POINT *>(
                        &m_pFramework->m_rcScreenRect.right));
                *reinterpret_cast<int *>(bytes + 0x24) = 1;
                beenResized = 1;
            }
        }
        break;

    case WM_ACTIVATE:
        if (w_param == 0) {
            *reinterpret_cast<int *>(bytes + 0x20) = 0;
            *reinterpret_cast<DWORD *>(bytes + 0x38) =
                GetCanonicalTimeGetTime()();
        } else {
            *reinterpret_cast<int *>(bytes + 0x20) = 1;
            *reinterpret_cast<DWORD *>(bytes + 0x34) +=
                GetCanonicalTimeGetTime()() -
                *reinterpret_cast<DWORD *>(bytes + 0x38);
        }
        break;

    case WM_PAINT:
        if (m_pFramework != nullptr &&
            (*reinterpret_cast<int *>(bytes + 0x24) == 0 ||
             m_pFramework->m_bIsFullscreen == FALSE)) {
            (void)m_pFramework->Present();
        }
        break;

    case WM_CLOSE:
        DestroyWindow(window);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_SETCURSOR:
        if (*reinterpret_cast<int *>(bytes + 0x20) != 0 &&
            *reinterpret_cast<int *>(bytes + 0x24) != 0 &&
            !IsWindowed()) {
            SetCursor(nullptr);
            return 1;
        }
        break;

    case WM_GETMINMAXINFO: {
        auto *information = reinterpret_cast<MINMAXINFO *>(l_param);
        information->ptMinTrackSize.x = 100;
        information->ptMinTrackSize.y = 100;
        break;
    }

    case WM_NCHITTEST:
        if (m_pFramework->m_bIsFullscreen != FALSE) {
            return 1;
        }
        break;

    case WM_KEYDOWN: {
        const unsigned char key = static_cast<unsigned char>(
            getasciicodefromvirtualkey(
                static_cast<int>(w_param),
                static_cast<int>(l_param),
                bytes + 0x310670,
                1));
        if (key != 0) {
            bytes[0x310570 + key] = 1;
            bytes[0x310770 + key] = 1;
            *reinterpret_cast<int *>(bytes + 0x310970) = key;
            using KeyCallback = void (*)(CD3DApplication *, int);
            reinterpret_cast<KeyCallback>(vtable[6])(this, key);
        }
        break;
    }

    case WM_KEYUP: {
        const unsigned char key = static_cast<unsigned char>(
            getasciicodefromvirtualkey(
                static_cast<int>(w_param),
                static_cast<int>(l_param),
                bytes + 0x310670,
                0));
        if (key != 0) {
            bytes[0x310570 + key] = 0;
            bytes[0x310870 + key] = 1;
            using KeyCallback = void (*)(CD3DApplication *, int);
            reinterpret_cast<KeyCallback>(vtable[7])(this, key);
        }
        break;
    }

    case WM_CHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
        if (!IsWindowed()) {
            return 1;
        }
        break;

    case WM_COMMAND:
        if (LOWORD(w_param) == 0x326) {
            SendMessageA(window, WM_CLOSE, 0, 0);
            return 0;
        }
        break;

    case WM_SYSCOMMAND:
        if (w_param == 0x323) {
            using ToggleCallback = int (*)(CD3DApplication *);
            (void)reinterpret_cast<ToggleCallback>(vtable[15])(this);
        } else if (w_param == SC_SIZE || w_param == SC_MOVE ||
                   w_param == SC_MAXIMIZE || w_param == SC_MONITORPOWER) {
            void *sdl_window = *reinterpret_cast<void **>(bytes + 0xD8);
            if (sdl_window == nullptr ||
                (GetCanonicalSDLWindowFlags(sdl_window) & 0x1001) != 0) {
                return 1;
            }
        }
        break;

    case WM_ENTERMENULOOP:
    case WM_EXITMENULOOP: {
        using PauseCallback = void (*)(CD3DApplication *, int);
        reinterpret_cast<PauseCallback>(vtable[14])(
            this, message == WM_ENTERMENULOOP ? 1 : 0);
        break;
    }

    case WM_POWERBROADCAST:
        if (w_param == PBT_APMQUERYSUSPEND) {
            using QueryCallback = std::int64_t (*)(
                CD3DApplication *, unsigned long);
            return reinterpret_cast<QueryCallback>(vtable[9])(
                this, static_cast<unsigned long>(l_param));
        }
        if (w_param == PBT_APMRESUMESUSPEND) {
            using ResumeCallback = std::int64_t (*)(
                CD3DApplication *, unsigned long);
            return reinterpret_cast<ResumeCallback>(vtable[10])(
                this, static_cast<unsigned long>(l_param));
        }
        break;

    case WM_ENTERSIZEMOVE:
        if (*reinterpret_cast<int *>(bytes + 0x2C) != 0) {
            *reinterpret_cast<DWORD *>(bytes + 0x38) =
                GetCanonicalTimeGetTime()();
        }
        GetClientRect(
            *reinterpret_cast<HWND *>(bytes + 0xD0),
            &originalwinrect);
        break;

    case WM_EXITSIZEMOVE:
        if (*reinterpret_cast<int *>(bytes + 0x2C) != 0) {
            *reinterpret_cast<DWORD *>(bytes + 0x34) +=
                GetCanonicalTimeGetTime()() -
                *reinterpret_cast<DWORD *>(bytes + 0x38);
        }
        if (*reinterpret_cast<int *>(bytes + 0x20) != 0 &&
            *reinterpret_cast<int *>(bytes + 0x24) != 0 &&
            IsWindowed()) {
            *reinterpret_cast<int *>(bytes + 0x24) = 0;
            GetWindowRect(
                *reinterpret_cast<HWND *>(bytes + 0xD0),
                reinterpret_cast<RECT *>(bytes + 0x38334));
            GetClientRect(
                *reinterpret_cast<HWND *>(bytes + 0xD0),
                &m_pFramework->m_rcScreenRect);
            const LONG width = m_pFramework->m_rcScreenRect.right;
            const LONG height = m_pFramework->m_rcScreenRect.bottom;
            ClientToScreen(
                *reinterpret_cast<HWND *>(bytes + 0xD0),
                reinterpret_cast<POINT *>(
                    &m_pFramework->m_rcScreenRect.left));
            ClientToScreen(
                *reinterpret_cast<HWND *>(bytes + 0xD0),
                reinterpret_cast<POINT *>(
                    &m_pFramework->m_rcScreenRect.right));
            if (width != originalwinrect.right ||
                height != originalwinrect.bottom) {
                g_pD3DApp->debugtrace(
                    const_cast<char *>("Re_Alloc Environment!\n"));
                if (FAILED(Change3DEnvironment())) {
                    return 0;
                }
            }
            (void)m_pFramework->Present();
            *reinterpret_cast<int *>(bytes + 0x24) = 1;
            beenResized = 0;
            using RestoreCallback = HRESULT (*)(CD3DApplication *);
            (void)reinterpret_cast<RestoreCallback>(vtable[3])(this);
        }
        break;
    }

    return DefWindowProcA(window, message, w_param, l_param);
}

/* 0x37390, 3 bytes, global, 2 named locals
 * CD3DApplication::OnKeyDown
 * PDB type: void CD3DApplication::(int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.h
 */
void CD3DApplication::OnKeyDown(int)
{
}

/* 0x373A0, 3 bytes, global, 2 named locals
 * CD3DApplication::OnKeyUp
 * PDB type: void CD3DApplication::(int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.h
 */
void CD3DApplication::OnKeyUp(int)
{
}

/* 0x373B0, 26 bytes, global, 2 named locals
 * CD3DApplication::OnQuerySuspend
 * PDB type: __int64 CD3DApplication::(unsign...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
std::int64_t CD3DApplication::OnQuerySuspend(unsigned long)
{
    using PauseCallback = void (*)(CD3DApplication *, int);
    void **vtable = *reinterpret_cast<void ***>(this);
    reinterpret_cast<PauseCallback>(vtable[14])(this, 1);
    return 1;
}

/* 0x373D0, 22 bytes, global, 2 named locals
 * CD3DApplication::OnResumeSuspend
 * PDB type: __int64 CD3DApplication::(unsign...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

/* 0x373F0, 3 bytes, global, 1 named locals
 * CD3DApplication::OneTimeSceneInit
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.h
 */
HRESULT CD3DApplication::OneTimeSceneInit()
{
    return S_OK;
}

/* 0x37400, 3 bytes, global, 2 named locals
 * CD3DApplication::OutputText
 * PDB type: void CD3DApplication::(char*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
std::int64_t CD3DApplication::OnResumeSuspend(unsigned long)
{
    using PauseCallback = void (*)(CD3DApplication *, int);
    void **vtable = *reinterpret_cast<void ***>(this);
    reinterpret_cast<PauseCallback>(vtable[14])(this, 0);
    return 1;
}
void CD3DApplication::OutputText(char *)
{
}

/* 0x37410, 3 bytes, global, 4 named locals
 * CD3DApplication::OutputTextXY
 * PDB type: void CD3DApplication::(int, int,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::OutputTextXY(int, int, char *)
{
}

/* 0x37420, 101 bytes, global, 3 named locals
 * CD3DApplication::Pause
 * PDB type: void CD3DApplication::(int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::Pause(int pause)
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    dwAppPausedCount += pause != 0 ? 1UL : static_cast<unsigned long>(-1);
    *reinterpret_cast<int *>(bytes + 0x24) = dwAppPausedCount == 0;
    if (pause != 0 && dwAppPausedCount == 1 &&
        *reinterpret_cast<int *>(bytes + 0x2C) != 0) {
        *reinterpret_cast<unsigned long *>(bytes + 0x38) =
            GetCanonicalTimeGetTime()();
    }
    if (dwAppPausedCount == 0 &&
        *reinterpret_cast<int *>(bytes + 0x2C) != 0) {
        *reinterpret_cast<unsigned long *>(bytes + 0x34) +=
            GetCanonicalTimeGetTime()() -
            *reinterpret_cast<unsigned long *>(bytes + 0x38);
    }
}

/* 0x37490, 138 bytes, global, 4 named locals
 * Microsoft::WRL::Details::RuntimeClassImpl<Microsoft::WRL::RuntimeClassFlags<2>,1,0,1,ABI::Windows::Foundation::IEventHandler<IInspectable *> >::QueryInterface
 * PDB type: HRESULT Microsoft::WRL::Details:...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\implements.h
 */

/* 0x37520, 13 bytes, global, 2 named locals
 * Microsoft::WRL::Details::RaiseException
 * PDB type: void (HRESULT, unsigned long)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\internal.h
 */

/* 0x37530, 83 bytes, global, 3 named locals
 * CD3DApplication::RegCreate
 * PDB type: int CD3DApplication::(HKEY__**)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
int CD3DApplication::RegCreate(HKEY *key)
{
    DWORD disposition;
    return RegCreateKeyExA(
        HKEY_CURRENT_USER,
        RegKeyName(),
        0,
        nullptr,
        0,
        KEY_ALL_ACCESS,
        nullptr,
        key,
        &disposition) == ERROR_SUCCESS;
}

/* 0x37590, 122 bytes, global, 1 named locals
 * CD3DApplication::RegKeyName
 * PDB type: char* CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
char *CD3DApplication::RegKeyName()
{
    std::strncpy(mainkeyname, "software\\", sizeof(mainkeyname));
    std::strncat(
        mainkeyname,
        reinterpret_cast<char *>(this) + 0x38108,
        sizeof(mainkeyname));
    std::strncat(mainkeyname, "\\", sizeof(mainkeyname));
    std::strncat(
        mainkeyname,
        reinterpret_cast<char *>(this) + 0x38208,
        sizeof(mainkeyname));
    return mainkeyname;
}

/* 0x37610, 59 bytes, global, 2 named locals
 * CD3DApplication::RegOpen
 * PDB type: int CD3DApplication::(HKEY__**)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
int CD3DApplication::RegOpen(HKEY *key)
{
    return RegOpenKeyExA(
        HKEY_CURRENT_USER,
        RegKeyName(),
        0,
        KEY_ALL_ACCESS,
        key) == ERROR_SUCCESS;
}

/* 0x37650, 108 bytes, global, 4 named locals
 * Microsoft::WRL::Details::RuntimeClassImpl<Microsoft::WRL::RuntimeClassFlags<2>,1,0,1,ABI::Windows::Foundation::IEventHandler<IInspectable *> >::Release
 * PDB type: unsigned long Microsoft::WRL::De...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt\wrl\implements.h
 */

/* 0x376C0, 424 bytes, global, 4 named locals
 * CD3DApplication::Render3DEnvironment
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::Render3DEnvironment()
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    const float time = static_cast<float>(
        GetCanonicalTimeGetTime()() -
        *reinterpret_cast<DWORD *>(bytes + 0x34)) * 0.001f;
    GetClientRect(
        *reinterpret_cast<HWND *>(bytes + 0xD0),
        reinterpret_cast<RECT *>(bytes + 0x38344));

    using RenderCallback = HRESULT (*)(CD3DApplication *, float);
    void **vtable = *reinterpret_cast<void ***>(this);
    HRESULT result = reinterpret_cast<RenderCallback>(vtable[4])(
        this, time);
    if (FAILED(result)) {
        return result;
    }

    *reinterpret_cast<int *>(bytes + 0x310970) = 0;
    result = m_pFramework->Present();
    if (SUCCEEDED(result)) {
        return S_OK;
    }

    if (result == DXGI_ERROR_DEVICE_REMOVED ||
        result == DXGI_ERROR_DEVICE_RESET) {
        void *window = *reinterpret_cast<void **>(bytes + 0xD8);
        if (window == nullptr ||
            (GetCanonicalSDLWindowFlags(window) & 0x1001) != 0) {
            return S_OK;
        }
        return Change3DEnvironment();
    }

    if (result != DXGI_ERROR_DEVICE_HUNG) {
        return result;
    }

    WaitForGpu();
    auto release_and_clear = [](IUnknown *&object) {
        if (object != nullptr) {
            object->Release();
            object = nullptr;
        }
    };
    release_and_clear(reinterpret_cast<IUnknown *&>(
        m_pFramework->m_pRenderTargets[0]));
    release_and_clear(reinterpret_cast<IUnknown *&>(
        m_pFramework->m_pRenderTargets[1]));
    release_and_clear(reinterpret_cast<IUnknown *&>(
        m_pFramework->m_pRtvDescriptorHeap));
    release_and_clear(reinterpret_cast<IUnknown *&>(
        m_pFramework->m_pSwapChain));
    release_and_clear(reinterpret_cast<IUnknown *&>(
        m_pFramework->m_pDevice));
    release_and_clear(reinterpret_cast<IUnknown *&>(
        m_pFramework->m_pFactory));

    void *&window = *reinterpret_cast<void **>(bytes + 0xD8);
    if (window != nullptr) {
        DestroyCanonicalSDLWindow(window);
        window = nullptr;
    }
    QuitCanonicalSDL();
    (void)InitD3D12Framework(
        *reinterpret_cast<HWND *>(bytes + 0xD0));
    return S_OK;
}

/* 0x37870, 3 bytes, global, 2 named locals
 * CD3DApplication::Render
 * PDB type: HRESULT CD3DApplication::(float)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.h
 */
HRESULT CD3DApplication::Render(float)
{
    return S_OK;
}

/* 0x37880, 1362 bytes, global, 32 named locals
 * CD3DApplication::RenderMenuTexture
 * PDB type: void CD3DApplication::(bool)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::RenderMenuTexture(bool is_menu)
{
    if (!is_menu) {
        return;
    }

    WaitForGpu();
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    ID3D12Resource *&texture =
        *reinterpret_cast<ID3D12Resource **>(bytes + 0x10);
    ID3D12Resource *&texture_upload_heap =
        *reinterpret_cast<ID3D12Resource **>(bytes + 0x18);

    std::uint32_t format = 0;
    int access = 0;
    int width = 0;
    int height = 0;
    QueryCanonicalSDLTexture(
        m_pFramework->m_pSDLRenderTarget,
        &format,
        &access,
        &width,
        &height);

    static std::vector<unsigned char> data;
    data.resize(static_cast<std::size_t>(width * height * 4));
    PresentCanonicalSDLRenderer(m_pFramework->m_pSDLRenderer);
    ReadCanonicalSDLRendererPixels(
        m_pFramework->m_pSDLRenderer,
        nullptr,
        format,
        data.data(),
        width * 4);

    if (texture_upload_heap != nullptr) {
        texture_upload_heap->Release();
        texture_upload_heap = nullptr;
    }
    if (texture != nullptr) {
        texture->Release();
        texture = nullptr;
    }

    D3D12_RESOURCE_DESC texture_description = {};
    texture_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture_description.Width = static_cast<UINT64>(width);
    texture_description.Height = static_cast<UINT>(height);
    texture_description.DepthOrArraySize = 1;
    texture_description.MipLevels = 1;
    texture_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_description.SampleDesc.Count = 1;
    texture_description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texture_description.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES default_heap = {};
    default_heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    default_heap.CreationNodeMask = 1;
    default_heap.VisibleNodeMask = 1;
    (void)m_pFramework->m_pDevice->CreateCommittedResource(
        &default_heap,
        D3D12_HEAP_FLAG_NONE,
        &texture_description,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&texture));

    if (texture == nullptr) {
        return;
    }
    texture->SetName(L"SDLRenderTexture");

    const std::uint64_t required_size =
        GetRequiredIntermediateSize(texture, 0, 1);
    if (required_size == 0) {
        return;
    }

    D3D12_RESOURCE_DESC upload_description = {};
    upload_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    upload_description.Width = required_size;
    upload_description.Height = 1;
    upload_description.DepthOrArraySize = 1;
    upload_description.MipLevels = 1;
    upload_description.Format = DXGI_FORMAT_UNKNOWN;
    upload_description.SampleDesc.Count = 1;
    upload_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    upload_description.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES upload_heap = {};
    upload_heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    upload_heap.CreationNodeMask = 1;
    upload_heap.VisibleNodeMask = 1;
    const HRESULT result =
        m_pFramework->m_pDevice->CreateCommittedResource(
            &upload_heap,
            D3D12_HEAP_FLAG_NONE,
            &upload_description,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&texture_upload_heap));
    if (FAILED(result)) {
        g_pD3DApp->debugtrace(const_cast<char *>(
            "Failed to create committed resource texture upload heap. Its null"));
        return;
    }

    D3D12_SUBRESOURCE_DATA texture_data = {};
    texture_data.pData = data.data();
    texture_data.RowPitch = static_cast<LONG_PTR>(width * 4);
    texture_data.SlicePitch = static_cast<LONG_PTR>(width * 4);
    (void)UpdateSubresources(
        m_pFramework->m_pCommandList,
        texture,
        texture_upload_heap,
        0,
        0,
        1,
        &texture_data);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    m_pFramework->m_pCommandList->ResourceBarrier(1, &barrier);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_description = {};
    srv_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_description.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_description.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_description.Texture2D.MostDetailedMip = 0;
    srv_description.Texture2D.MipLevels = 1;
    srv_description.Texture2D.PlaneSlice = 0;
    srv_description.Texture2D.ResourceMinLODClamp = 0.0f;
    m_pFramework->m_pDevice->CreateShaderResourceView(
        texture,
        &srv_description,
        m_pFramework->m_pMainDescriptorHeap
            ->GetCPUDescriptorHandleForHeapStart());
}

/* 0x37DE0, 1124 bytes, global, 14 named locals
 * CD3DApplication::RenderUI
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::RenderUI()
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    auto *draws = reinterpret_cast<std::vector<SpriteDraw> *>(
        bytes + 0x58);
    if (draws->empty()) {
        return S_OK;
    }

    auto *font_atlas = *reinterpret_cast<FontAtlas **>(bytes + 0xF8);
    if (font_atlas->IsDirty()) {
        font_atlas->UpdateTexture();
    }

    auto *sprite_batch =
        *reinterpret_cast<DirectX::DX12::SpriteBatch **>(bytes + 0x48);
    auto *states =
        *reinterpret_cast<DirectX::DX12::CommonStates **>(bytes + 0x50);
    ID3D12GraphicsCommandList *command_list =
        m_pFramework->m_pCommandList;
    sprite_batch->SetViewport(m_pFramework->m_Viewport);

    ID3D12DescriptorHeap *descriptor_heaps[2] = {
        m_pFramework->m_pMainDescriptorHeap,
        states->Heap(),
    };
    command_list->SetDescriptorHeaps(2, descriptor_heaps);
    DirectX::DescriptorHeap descriptor_heap(
        m_pFramework->m_pMainDescriptorHeap);

    SortCanonicalSpriteDraws(draws->data(), draws->data() + draws->size());
    RECT current_scissor = m_pFramework->m_ScissorRect;
    TEXTURE_SAMPLE_TYPE current_sampler = TEXTURESAMPLER_LINEARCLAMP;

    auto begin_batch = [&](TEXTURE_SAMPLE_TYPE sampler) {
        D3D12_GPU_DESCRIPTOR_HANDLE sampler_handle;
        if (sampler == TEXTURESAMPLER_LINEARCLAMP) {
            sampler_handle = states->LinearClamp();
        } else if (sampler == TEXTURSAMPLER_POINTCLAMP) {
            sampler_handle = states->PointClamp();
        } else {
            return;
        }
        sprite_batch->Begin(
            command_list,
            sampler_handle,
            DirectX::DX12::SpriteSortMode_Deferred,
            DirectX::XMMatrixIdentity());
    };

    begin_batch(current_sampler);
    for (const SpriteDraw &draw : *draws) {
        if (draw.SamplerType != current_sampler) {
            sprite_batch->End();
            current_sampler = draw.SamplerType;
            begin_batch(current_sampler);
        }

        const RECT scissor = draw.ScissorRect.has_value()
            ? draw.ScissorRect.value()
            : m_pFramework->m_ScissorRect;
        if (current_scissor.left != scissor.left ||
            current_scissor.top != scissor.top ||
            current_scissor.right != scissor.right ||
            current_scissor.bottom != scissor.bottom) {
            current_scissor = scissor;
            sprite_batch->End();
            command_list->RSSetScissorRects(1, &current_scissor);
            begin_batch(current_sampler);
        }

        const DirectX::XMUINT2 texture_size(
            draw.Texture->m_dwWidth,
            draw.Texture->m_dwHeight);
        const RECT *source_rectangle = draw.SrcRect.has_value()
            ? &draw.SrcRect.value()
            : nullptr;
        sprite_batch->Draw(
            descriptor_heap.GetGpuHandle(draw.Texture->m_nIndex),
            texture_size,
            draw.DestRect,
            source_rectangle,
            draw.Color,
            draw.Rotation,
            draw.Origin,
            draw.Effects,
            draw.LayerDepth);
    }
    sprite_batch->End();
    draws->clear();
    return S_OK;
}

/* 0x38250, 3 bytes, global, 1 named locals
 * CD3DApplication::RestoreSurfaces
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.h
 */
HRESULT CD3DApplication::RestoreSurfaces()
{
    return S_OK;
}

/* 0x38260, 646 bytes, global, 7 named locals
 * CD3DApplication::Run
 * PDB type: int CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
int CD3DApplication::Run()
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    HACCEL accelerator = LoadAcceleratorsA(
        nullptr, MAKEINTRESOURCEA(0x71));
    MSG message;
    PeekMessageA(&message, nullptr, 0, 0, PM_NOREMOVE);

    while (message.message != WM_QUIT) {
        BOOL got_message;
        if (*reinterpret_cast<int *>(bytes + 0x20) != 0) {
            got_message = PeekMessageA(
                &message, nullptr, 0, 0, PM_REMOVE);
        } else {
            got_message = GetMessageA(
                &message, nullptr, 0, 0);
        }

        if (got_message != FALSE) {
            if (TranslateAcceleratorA(
                    *reinterpret_cast<HWND *>(bytes + 0xD0),
                    accelerator,
                    &message) == 0) {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
            continue;
        }

        if (*reinterpret_cast<int *>(bytes + 0x20) == 0 ||
            *reinterpret_cast<int *>(bytes + 0x24) == 0) {
            continue;
        }

        const HRESULT result = Render3DEnvironment();
        if (FAILED(result)) {
            using CleanupCallback = void (*)(CD3DApplication *);
            void **vtable = *reinterpret_cast<void ***>(this);
            reinterpret_cast<CleanupCallback>(vtable[8])(this);
        }
    }
    return static_cast<int>(message.wParam);
}

/* 0x384F0, 3 bytes, global, 4 named locals
 * CD3DApplication::SelectTexture
 * PDB type: void CD3DApplication::(unsigned,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::SelectTexture(
    unsigned, ID3D12Resource *, ID3D12DescriptorHeap *)
{
}

/* 0x38500, 18 bytes, global, 2 named locals
 * CD3DApplication::SetAppTitle
 * PDB type: void CD3DApplication::(char*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::SetAppTitle(char *title)
{
    std::strncpy(
        reinterpret_cast<char *>(this) + 0x38208, title, 0x100);
}

/* 0x38520, 7 bytes, global, 2 named locals
 * CD3DApplication::SetBitDepth
 * PDB type: void CD3DApplication::(unsigned)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::SetBitDepth(unsigned bits_per_pixel)
{
    *reinterpret_cast<unsigned *>(
        reinterpret_cast<unsigned char *>(this) + 0x38324) = bits_per_pixel;
}

/* 0x38530, 18 bytes, global, 2 named locals
 * CD3DApplication::SetCompanyTitle
 * PDB type: void CD3DApplication::(char*)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::SetCompanyTitle(char *title)
{
    std::strncpy(
        reinterpret_cast<char *>(this) + 0x38108, title, 0x100);
}

/* 0x38550, 7 bytes, global, 2 named locals
 * CD3DApplication::SetFullScreen
 * PDB type: void CD3DApplication::(int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::SetFullScreen(int enabled)
{
    *reinterpret_cast<int *>(
        reinterpret_cast<unsigned char *>(this) + 0xE0) = enabled;
}

/* 0x38560, 151 bytes, global, 6 named locals
 * CD3DApplication::SetRegistryBinary
 * PDB type: void CD3DApplication::(char*, vo...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

void CD3DApplication::SetRegistryBinary(
    char *name, void *data, unsigned size)
{
    HKEY key;
    DWORD disposition;
    if (RegCreateKeyExA(
            HKEY_CURRENT_USER, RegKeyName(), 0, nullptr, 0,
            KEY_ALL_ACCESS, nullptr, &key, &disposition) == ERROR_SUCCESS) {
        RegSetValueExA(
            key, name, 0, REG_BINARY,
            static_cast<const BYTE *>(data), size);
        RegCloseKey(key);
    }
}

/* 0x38600, 232 bytes, global, 6 named locals
 * CD3DApplication::SetRegistryDWord
 * PDB type: void CD3DApplication::(char*, un...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

void CD3DApplication::SetRegistryDWord(
    char *name, unsigned long value)
{
    HKEY key;
    DWORD disposition;
    if (RegCreateKeyExA(
            HKEY_CURRENT_USER, RegKeyName(), 0, nullptr, 0,
            KEY_ALL_ACCESS, nullptr, &key, &disposition) == ERROR_SUCCESS) {
        const LSTATUS status = RegSetValueExA(
            key, name, 0, REG_DWORD,
            reinterpret_cast<const BYTE *>(&value), sizeof(value));
        if (status != ERROR_SUCCESS) {
            SystemErrorReport(
                const_cast<char *>(
                    "W:\\SWJediPowerBattles\\work\\d3d\\d3dapp.cpp"),
                0xE32,
                status,
                const_cast<char *>("RegSetValueEx"));
        } else {
            g_pD3DApp->debugtrace(
                const_cast<char *>("SET %s to %d\n"), name, value);
        }
        RegCloseKey(key);
    }
}

/* 0x386F0, 161 bytes, global, 5 named locals
 * CD3DApplication::SetRegistryString
 * PDB type: void CD3DApplication::(char*, ch...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

void CD3DApplication::SetRegistryString(char *name, char *value)
{
    HKEY key;
    DWORD disposition;
    if (RegCreateKeyExA(
            HKEY_CURRENT_USER, RegKeyName(), 0, nullptr, 0,
            KEY_ALL_ACCESS, nullptr, &key, &disposition) == ERROR_SUCCESS) {
        RegSetValueExA(
            key,
            name,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE *>(value),
            static_cast<DWORD>(std::strlen(value) + 1));
        RegCloseKey(key);
    }
}

/* 0x387A0, 3 bytes, global, 5 named locals
 * CD3DApplication::SetRenderState
 * PDB type: void CD3DApplication::(ID3D12Dev...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::SetRenderState(
    ID3D12Device *, ID3D12PipelineState *, D3D12_BLEND, D3D12_BLEND)
{
}

/* 0x387B0, 7 bytes, global, 2 named locals
 * CD3DApplication::SetUseZBuffer
 * PDB type: void CD3DApplication::(int)
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::SetUseZBuffer(int enabled)
{
    *reinterpret_cast<int *>(
        reinterpret_cast<unsigned char *>(this) + 0xE4) = enabled;
}

/* 0x387C0, 12 bytes, global, 5 named locals
 * CD3DApplication::SetViewParams
 * PDB type: void CD3DApplication::(_D3DVECTO...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::SetViewParams(
    D3DVECTOR *from,
    D3DVECTOR *at,
    D3DVECTOR *world_up,
    float)
{
    D3DUtil_SetViewMatrix(
        *reinterpret_cast<D3DMATRIX *>(
            reinterpret_cast<unsigned char *>(this) + 0x38360),
        *from, *at, *world_up);
}

/* 0x387D0, 14 bytes, global, 3 named locals
 * CD3DApplication::SetWidthHeight
 * PDB type: void CD3DApplication::(unsigned,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::SetWidthHeight(unsigned width, unsigned height)
{
    auto *bytes = reinterpret_cast<unsigned char *>(this);
    *reinterpret_cast<unsigned *>(bytes + 0x3831C) = width;
    *reinterpret_cast<unsigned *>(bytes + 0x38320) = height;
}

/* 0x387E0, 7 bytes, global, 2 named locals
 * CD3DApplication::SetWindowFlags
 * PDB type: void CD3DApplication::(unsigned ...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::SetWindowFlags(unsigned long flags)
{
    *reinterpret_cast<unsigned long *>(
        reinterpret_cast<unsigned char *>(this) + 0x38318) = flags;
}

/* 0x387F0, 33 bytes, global, 1 named locals
 * CD3DApplication::ShutDown
 * PDB type: void CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

/* 0x38820, 953 bytes, global, 9 named locals
 * CD3DApplication::StartRender
 * PDB type: HRESULT CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

/* 0x38BE0, 33 bytes, global, 1 named locals
 * SteamInternal_Init_SteamUtils
 * PDB type: void (ISteamUtils**)
 * Source: W:\SWJediPowerBattles\work\steam\include\isteamutils.h
 * Exact body is in d3dapp_steam.cpp so its Steam SDK import remains in a
 * separate static-archive object from the Win32 key translator below.
 */

/* 0x38C10, 185 bytes, global, 7 named locals
 * CD3DApplication::SystemErrorReport
 * PDB type: void CD3DApplication::(char*, in...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

void CD3DApplication::SystemErrorReport(
    char *file,
    int line,
    unsigned long error,
    char *function)
{
    char *message = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        0,
        reinterpret_cast<char *>(&message),
        1,
        nullptr);
    for (char *cursor = message; *cursor != '\0'; ++cursor) {
        if (*cursor == '\r' || *cursor == '\n') {
            *cursor = '\0';
        }
    }
    g_pD3DApp->debugtrace(
        const_cast<char *>("%s(%d): ERROR %s [%s](%08x)\n"),
        file,
        line,
        message,
        function,
        error);
    LocalFree(message);
}

/* 0x38CD0, 6 bytes, global, 1 named locals
 * CD3DApplication::ToggleFullScreen
 * PDB type: int CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
HRESULT CD3DApplication::StartRender()
{
    WaitForGpu();
    if (m_pFramework->m_pDevice == nullptr) {
        return static_cast<HRESULT>(0x88760028);
    }

    m_pFramework->TryCloseCommandList();
    if (m_pFramework != nullptr) {
        const UINT frame_index = m_pFramework->m_nFrameIndex;
        const std::uint64_t fence_value =
            m_pFramework->m_nFenceValues[frame_index];
        if (SUCCEEDED(m_pFramework->m_pCommandQueue->Signal(
                m_pFramework->m_pFence[frame_index], fence_value)) &&
            SUCCEEDED(m_pFramework->m_pFence[frame_index]->
                SetEventOnCompletion(
                    fence_value, m_pFramework->m_hFenceEvent))) {
            WaitForSingleObject(m_pFramework->m_hFenceEvent, INFINITE);
            ++m_pFramework->m_nFenceValues[frame_index];
        }
    }

    const UINT frame_index = m_pFramework->m_nFrameIndex;
    HRESULT result =
        m_pFramework->m_pCommandAllocators[frame_index]->Reset();
    if (FAILED(result)) {
        return result;
    }
    if (!m_pFramework->IsCommandListOpen()) {
        result = m_pFramework->m_pCommandList->Reset(
            m_pFramework->m_pCommandAllocators[frame_index],
            m_pFramework->m_pPipelineState);
        m_pFramework->m_commandListOpen = true;
        if (FAILED(result)) {
            return result;
        }
    }

    WaitForGpu();
    ID3D12GraphicsCommandList *command_list =
        m_pFramework->m_pCommandList;
    command_list->SetPipelineState(m_pFramework->m_pPipelineState);
    command_list->SetGraphicsRootSignature(m_pFramework->m_pRootSignature);
    command_list->RSSetViewports(1, &m_pFramework->m_Viewport);
    command_list->RSSetScissorRects(1, &m_pFramework->m_ScissorRect);

    ID3D12DescriptorHeap *descriptor_heaps[] = {
        m_pFramework->m_cbvHeap};
    command_list->SetDescriptorHeaps(1, descriptor_heaps);
    command_list->SetGraphicsRootConstantBufferView(
        0, m_pFramework->m_constantBuffer->GetGPUVirtualAddress());

    ID3D12DescriptorHeap *main_descriptor_heaps[] = {
        m_pFramework->m_pMainDescriptorHeap};
    command_list->SetDescriptorHeaps(1, main_descriptor_heaps);
    command_list->SetGraphicsRootDescriptorTable(
        1,
        m_pFramework->m_pMainDescriptorHeap->
            GetGPUDescriptorHandleForHeapStart());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource =
        m_pFramework->m_pRenderTargets[frame_index];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    command_list->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle =
        m_pFramework->m_pRtvDescriptorHeap->
            GetCPUDescriptorHandleForHeapStart();
    rtv_handle.ptr += static_cast<SIZE_T>(frame_index) *
        m_pFramework->m_RTVDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle =
        m_pFramework->m_pDsDescriptorHeap->
            GetCPUDescriptorHandleForHeapStart();
    command_list->OMSetRenderTargets(
        1, &rtv_handle, FALSE, &dsv_handle);

    const float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    command_list->ClearRenderTargetView(
        rtv_handle, clear_color, 0, nullptr);
    dsv_handle = m_pFramework->m_pDsDescriptorHeap->
        GetCPUDescriptorHandleForHeapStart();
    command_list->ClearDepthStencilView(
        dsv_handle,
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f,
        0,
        0,
        nullptr);
    return S_OK;
}
void CD3DApplication::ShutDown()
{
    Cleanup3DEnvironment();
    const auto *bytes = reinterpret_cast<const unsigned char *>(this);
    DestroyWindow(*reinterpret_cast<HWND const *>(bytes + 0xD0));
}
int CD3DApplication::ToggleFullScreen()
{
    return 1;
}

/* 0x38CE0, 62 bytes, global, 5 named locals
 * TransitionResource
 * PDB type: void (ID3D12GraphicsCommandList*...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void TransitionResource(
    ID3D12GraphicsCommandList *command_list,
    ID3D12Resource *resource,
    D3D12_RESOURCE_STATES state_before,
    D3D12_RESOURCE_STATES state_after)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = state_before;
    barrier.Transition.StateAfter = state_after;
    command_list->ResourceBarrier(1, &barrier);
}

/* 0x38D20, 28 bytes, global, 4 named locals
 * CD3DApplication::UpdateResolution
 * PDB type: void CD3DApplication::(int, int,...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::UpdateResolution(
    int width, int height, int window_mode)
{
    resolutionUpdated = 1;
    newWidth = width;
    newHeight = height;
    newWindowMode = window_mode;
}

/* 0x38D40, 896 bytes, global, 21 named locals
 * UpdateSubresources
 * PDB type: unsigned __int64 (ID3D12Graphics...
 * Source: W:\SWJediPowerBattles\work\d3d\directx\d3dx12_resource_helpers.h
 */
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
    D3D12_SUBRESOURCE_DATA *source_data)
{
    const D3D12_RESOURCE_DESC intermediate_description =
        intermediate_resource->GetDesc();
    const D3D12_RESOURCE_DESC destination_description =
        destination_resource->GetDesc();
    unsigned char *mapped_data = nullptr;

    if (intermediate_description.Dimension !=
            D3D12_RESOURCE_DIMENSION_BUFFER ||
        layouts[0].Offset + required_size >
            intermediate_description.Width ||
        (destination_description.Dimension ==
             D3D12_RESOURCE_DIMENSION_BUFFER &&
         (first_subresource != 0 || subresource_count != 1)) ||
        FAILED(intermediate_resource->Map(
            0, nullptr, reinterpret_cast<void **>(&mapped_data)))) {
        return 0;
    }

    for (UINT index = 0; index < subresource_count; ++index) {
        unsigned char *destination_slice =
            mapped_data + layouts[index].Offset;
        const unsigned char *source_slice =
            static_cast<const unsigned char *>(source_data[index].pData);
        const SIZE_T destination_slice_pitch =
            static_cast<SIZE_T>(layouts[index].Footprint.RowPitch) *
            row_counts[index];

        for (UINT depth = 0;
             depth < layouts[index].Footprint.Depth;
             ++depth) {
            unsigned char *destination_row =
                destination_slice + destination_slice_pitch * depth;
            const unsigned char *source_row =
                source_slice + source_data[index].SlicePitch * depth;

            for (UINT row = 0; row < row_counts[index]; ++row) {
                std::memcpy(
                    destination_row,
                    source_row,
                    static_cast<SIZE_T>(row_sizes[index]));
                destination_row += layouts[index].Footprint.RowPitch;
                source_row += source_data[index].RowPitch;
            }
        }
    }
    intermediate_resource->Unmap(0, nullptr);

    if (destination_description.Dimension ==
        D3D12_RESOURCE_DIMENSION_BUFFER) {
        command_list->CopyBufferRegion(
            destination_resource,
            0,
            intermediate_resource,
            layouts[0].Offset,
            layouts[0].Footprint.Width);
    } else {
        for (UINT index = 0; index < subresource_count; ++index) {
            D3D12_TEXTURE_COPY_LOCATION destination = {};
            D3D12_TEXTURE_COPY_LOCATION source = {};

            destination.pResource = destination_resource;
            destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            destination.SubresourceIndex = first_subresource + index;
            source.pResource = intermediate_resource;
            source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            source.PlacedFootprint = layouts[index];
            command_list->CopyTextureRegion(
                &destination, 0, 0, 0, &source, nullptr);
        }
    }
    return required_size;
}

/* 0x390C0, 139 bytes, global, 2 named locals
 * CD3DApplication::WaitForGpu
 * PDB type: void CD3DApplication::()
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::WaitForGpu()
{
#ifdef JPB_D3DAPP_TESTING
    if (wait_for_gpu_test_hook != nullptr) {
        wait_for_gpu_test_hook();
        return;
    }
#endif
    if (m_pFramework == nullptr) {
        return;
    }
    const UINT frame_index = m_pFramework->m_nCurrentFrameIndex;
    const std::uint64_t fence_value =
        m_pFramework->m_nFenceValues[frame_index];
    if (FAILED(m_pFramework->m_pCommandQueue->Signal(
            m_pFramework->m_pFence[frame_index], fence_value))) {
        return;
    }
    if (FAILED(m_pFramework->m_pFence[frame_index]->SetEventOnCompletion(
            fence_value, m_pFramework->m_hFenceEvent))) {
        return;
    }
    WaitForSingleObject(m_pFramework->m_hFenceEvent, INFINITE);
    ++m_pFramework->m_nFenceValues[frame_index];
}

/* 0x39150, 195 bytes, global, 11 named locals
 * DirectX::XMMatrixPerspectiveFovLH
 * PDB type: DirectX::XMMATRIX (float, float,...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\DirectXMathMatrix.inl
 */

/* 0x39220, 305 bytes, global, 8 named locals
 * DirectX::XMScalarSinCos
 * PDB type: void (float*, float*, float)
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\DirectXMathMisc.inl
 */

/* 0x39360, 7 bytes, global, 3 named locals
 * ATL::_AtlInitializeCriticalSectionEx
 * PDB type: int (_RTL_CRITICAL_SECTION*, uns...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\atlmfc\include\atlwinverapi.h
 */

/* 0x39370, 32 bytes, global, 0 named locals
 * std::_Throw_bad_array_new_length
 * PDB type: void ()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x39390, 34 bytes, global, 1 named locals
 * std::_Throw_future_error2
 * PDB type: void (const std::future_errc)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\future
 */

/* 0x393C0, 17 bytes, global, 0 named locals
 * std::_Xlen_string
 * PDB type: void ()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x393E0, 17 bytes, global, 0 named locals
 * std::vector<unsigned char,std::allocator<unsigned char> >::_Xlength
 * PDB type: void std::vector<unsigned char,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x39400, 17 bytes, global, 0 named locals
 * std::vector<wchar_t const *,std::allocator<wchar_t const *> >::_Xlength
 * PDB type: void std::vector<wchar_t const *...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x39420, 17 bytes, global, 0 named locals
 * std::vector<SpriteDraw,std::allocator<SpriteDraw> >::_Xlength
 * PDB type: void std::vector<SpriteDraw,std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x39440, 61 bytes, global, 7 named locals
 * std::allocator<unsigned char>::deallocate
 * PDB type: void std::allocator<unsigned cha...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x39480, 66 bytes, global, 7 named locals
 * std::allocator<wchar_t const *>::deallocate
 * PDB type: void std::allocator<wchar_t cons...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x394D0, 62 bytes, global, 7 named locals
 * std::allocator<SpriteDraw>::deallocate
 * PDB type: void std::allocator<SpriteDraw>:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x39510, 109 bytes, global, 9 named locals
 * CD3DApplication::debug_line
 * PDB type: void CD3DApplication::(float, fl...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::debug_line(
    float, float, float, float, float, float, unsigned long color)
{
    D3DMATERIAL7 material;
    InitializeCanonicalDebugMaterial(
        material,
        static_cast<float>((color >> 16) & 0xFF) / 255.0f,
        static_cast<float>((color >> 8) & 0xFF) / 255.0f,
        static_cast<float>(color & 0xFF) / 255.0f,
        1.0f);
}

/* 0x39580, 223 bytes, global, 11 named locals
 * CD3DApplication::debug_point
 * PDB type: void CD3DApplication::(float, fl...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::debug_point(
    float, float, float, int, unsigned long color)
{
    const float red =
        static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    const float green =
        static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    const float blue =
        static_cast<float>(color & 0xFF) / 255.0f;
    D3DMATERIAL7 material;
    InitializeCanonicalDebugMaterial(material, red, green, blue, 1.0f);
    InitializeCanonicalDebugMaterial(material, red, green, blue, 1.0f);
    InitializeCanonicalDebugMaterial(material, red, green, blue, 1.0f);
}

/* 0x39660, 262 bytes, global, 9 named locals
 * CD3DApplication::debug_printf
 * PDB type: void CD3DApplication::(char*, <n...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

/* 0x39770, 132 bytes, global, 3 named locals
 * CD3DApplication::debugtrace
 * PDB type: void CD3DApplication::(char*, <n...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */

/* 0x39800, 11 bytes, global, 2 named locals
 * std::error_category::default_error_condition
 * PDB type: std::error_condition std::error_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x39810, 25 bytes, global, 3 named locals
 * std::error_category::equivalent
 * PDB type: bool std::error_category::(const...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x39830, 63 bytes, global, 4 named locals
 * std::error_category::equivalent
 * PDB type: bool std::error_category::(int, ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x39870, 168 bytes, local, 7 named locals
 * getasciicodefromvirtualkey
 * PDB type: char (int, int, unsigned char*, ...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::debug_printf(char *format, ...)
{
    char normalized_format[256];
    char buffer[256];
    CopyCanonicalDebugFormat(normalized_format, format);

    va_list arguments;
    va_start(arguments, format);
    std::vsprintf(buffer, normalized_format, arguments);
    va_end(arguments);
}
void CD3DApplication::debugtrace(char *format, ...)
{
    char buffer[256];
    va_list arguments;

    va_start(arguments, format);
    std::vsprintf(buffer, format, arguments);
    va_end(arguments);
    OutputDebugStringA(buffer);
}
char getasciicodefromvirtualkey(
    int w_param,
    int l_param,
    unsigned char *raw_key_map,
    int press)
{
    static const unsigned char key_remap_table0[8] = {
        0x97, 0x98, 0x96, 0x95, 0x1c, 0x1e, 0x1d, 0x1f
    };
    static const unsigned char key_remap_table1[13] = {
        0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0x21
    };
#if defined(_WIN32)
    WORD translated = 0;
    int translated_count;

    raw_key_map[(unsigned char)w_param] = press != 0 ? 0x80 : 0;
    translated_count = ToAscii(
        (UINT)w_param,
        (UINT)l_param,
        raw_key_map,
        &translated,
        0);
    if (translated_count == 1) {
        return (char)(translated & 0xff);
    }
    if (translated_count == 0) {
        if (w_param >= 0x21 && w_param <= 0x28) {
            return (char)key_remap_table0[w_param - 0x21];
        }
        if (w_param >= 0x70 && w_param <= 0x7c) {
            return (char)key_remap_table1[w_param - 0x70];
        }
    }
    return 0;
#else
    (void)w_param;
    (void)l_param;
    (void)raw_key_map;
    (void)press;
    std::abort();
#endif
}

/* 0x39920, 175 bytes, global, 4 named locals
 * std::_Future_error_category2::message
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\future
 */

/* 0x399D0, 8 bytes, global, 1 named locals
 * std::_Future_error_category2::name
 * PDB type: const char* std::_Future_error_c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\future
 */

/* 0x399E0, 318 bytes, global, 10 named locals
 * CD3DApplication::svprintf
 * PDB type: void CD3DApplication::(char*, ch...
 * Source: W:\SWJediPowerBattles\work\d3d\d3dapp.cpp
 */
void CD3DApplication::svprintf(
    char *buffer, char *format, char *arguments)
{
    if (buffer == nullptr) {
        other = *format;
        return;
    }

    char normalized_format[256];
    CopyCanonicalDebugFormat(normalized_format, format);
    std::vsprintf(buffer, normalized_format, arguments);
}

/* 0x39B20, 8 bytes, global, 1 named locals
 * std::bad_optional_access::what
 * PDB type: const char* std::bad_optional_ac...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\optional
 */

/* 0x39B30, 19 bytes, global, 1 named locals
 * std::exception::what
 * PDB type: const char* std::exception::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vcruntime_exception.h
 */

/* 0x39B50, 58 bytes, global, 1 named locals
 * std::future_error::what
 * PDB type: const char* std::future_error::(...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\future
 */

/* 0x26F6B0, 43 bytes, local, 1 named locals
 * `std::vector<SpriteDraw,std::allocator<SpriteDraw> >::_Emplace_reallocate<SpriteDraw const &>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x26F6E0, 40 bytes, local, 2 named locals
 * `std::vector<wchar_t const *,std::allocator<wchar_t const *> >::_Emplace_reallocate<wchar_t const *>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x26F710, 40 bytes, local, 2 named locals
 * `std::vector<unsigned char,std::allocator<unsigned char> >::_Resize_reallocate<std::_Value_init_tag>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x26F740, 16 bytes, local, 1 named locals
 * `CD3DApplication::CD3DApplication'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F750, 16 bytes, local, 1 named locals
 * `CD3DApplication::CD3DApplication'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F760, 16 bytes, local, 1 named locals
 * `CD3DApplication::CD3DApplication'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F770, 19 bytes, local, 1 named locals
 * `CD3DApplication::CD3DApplication'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F790, 19 bytes, local, 1 named locals
 * `CD3DApplication::CD3DApplication'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F7B0, 19 bytes, local, 1 named locals
 * `CD3DApplication::CD3DApplication'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F7D0, 19 bytes, local, 1 named locals
 * `CD3DApplication::CD3DApplication'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F7F0, 16 bytes, local, 1 named locals
 * `CD3DApplication::CD3DApplication'::`1'::dtor$12
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F800, 16 bytes, local, 1 named locals
 * `CD3DApplication::CD3DApplication'::`1'::dtor$13
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F810, 29 bytes, local, 4 named locals
 * `CD3DApplication::Create'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F830, 12 bytes, local, 4 named locals
 * `CD3DApplication::Create'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F840, 12 bytes, local, 4 named locals
 * `CD3DApplication::Create'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F850, 29 bytes, local, 4 named locals
 * `CD3DApplication::Create'::`1'::dtor$19
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F870, 12 bytes, local, 20 named locals
 * `CD3DApplication::CreateLevelPipelineStateObject'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F880, 12 bytes, local, 20 named locals
 * `CD3DApplication::CreateLevelPipelineStateObject'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F890, 12 bytes, local, 20 named locals
 * `CD3DApplication::CreateLevelPipelineStateObject'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F8A0, 12 bytes, local, 20 named locals
 * `CD3DApplication::CreateLevelPipelineStateObject'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F8B0, 12 bytes, local, 20 named locals
 * `CD3DApplication::CreateLevelPipelineStateObject'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F8C0, 12 bytes, local, 20 named locals
 * `CD3DApplication::CreateLevelPipelineStateObject'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F8D0, 12 bytes, local, 20 named locals
 * `CD3DApplication::CreateLevelPipelineStateObject'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F8E0, 12 bytes, local, 20 named locals
 * `CD3DApplication::CreateLevelPipelineStateObject'::`1'::dtor$7
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F8F0, 12 bytes, local, 20 named locals
 * `CD3DApplication::CreateLevelPipelineStateObject'::`1'::dtor$8
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F900, 12 bytes, local, 20 named locals
 * `CD3DApplication::CreateLevelPipelineStateObject'::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F910, 12 bytes, local, 17 named locals
 * `CD3DApplication::CreatePipelineStateObject'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F920, 12 bytes, local, 17 named locals
 * `CD3DApplication::CreatePipelineStateObject'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F930, 12 bytes, local, 17 named locals
 * `CD3DApplication::CreatePipelineStateObject'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F940, 12 bytes, local, 17 named locals
 * `CD3DApplication::CreatePipelineStateObject'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F950, 12 bytes, local, 17 named locals
 * `CD3DApplication::CreatePipelineStateObject'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F960, 12 bytes, local, 17 named locals
 * `CD3DApplication::CreatePipelineStateObject'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F970, 12 bytes, local, 17 named locals
 * `CD3DApplication::CreatePipelineStateObject'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F980, 12 bytes, local, 17 named locals
 * `CD3DApplication::CreatePipelineStateObject'::`1'::dtor$7
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F990, 12 bytes, local, 17 named locals
 * `CD3DApplication::CreatePipelineStateObject'::`1'::dtor$8
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F9A0, 12 bytes, local, 19 named locals
 * `CD3DApplication::CreateTransparentPipelineStateObject'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F9B0, 12 bytes, local, 19 named locals
 * `CD3DApplication::CreateTransparentPipelineStateObject'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F9C0, 12 bytes, local, 19 named locals
 * `CD3DApplication::CreateTransparentPipelineStateObject'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F9D0, 12 bytes, local, 19 named locals
 * `CD3DApplication::CreateTransparentPipelineStateObject'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F9E0, 12 bytes, local, 19 named locals
 * `CD3DApplication::CreateTransparentPipelineStateObject'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26F9F0, 12 bytes, local, 19 named locals
 * `CD3DApplication::CreateTransparentPipelineStateObject'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FA00, 12 bytes, local, 19 named locals
 * `CD3DApplication::CreateTransparentPipelineStateObject'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FA10, 12 bytes, local, 19 named locals
 * `CD3DApplication::CreateTransparentPipelineStateObject'::`1'::dtor$7
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FA20, 29 bytes, local, 7 named locals
 * `CD3DApplication::InitD3D12Framework'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FA40, 12 bytes, local, 7 named locals
 * `CD3DApplication::InitD3D12Framework'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FA50, 12 bytes, local, 7 named locals
 * `CD3DApplication::InitD3D12Framework'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FA60, 29 bytes, local, 7 named locals
 * `CD3DApplication::InitD3D12Framework'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FA80, 29 bytes, local, 7 named locals
 * `CD3DApplication::InitD3D12Framework'::`1'::dtor$8
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FAA0, 29 bytes, local, 7 named locals
 * `CD3DApplication::InitD3D12Framework'::`1'::dtor$10
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FAC0, 12 bytes, local, 6 named locals
 * `CD3DApplication::InitializeDevice'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x26FAD0, 12 bytes, local, 3 named locals
 * `CD3DApplication::RenderUI'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x279740, 97 bytes, local, 4 named locals
 * `CD3DApplication::RenderMenuTexture'::`2'::`dynamic atexit destructor for 'data''
 * PDB type: void ()
 * Source: no line mapping
 */

/* 0x2797B0, 39 bytes, local, 1 named locals
 * `dynamic atexit destructor for 'g_gameBarStatics''
 * PDB type: void ()
 * Source: no line mapping
 */
