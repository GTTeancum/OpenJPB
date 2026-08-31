/*
 * REVIEWED RECONSTRUCTION.
 *
 * Canonical gameplay-visible bodies are reconstructed from the matching PDB
 * and shipped executable. Graphics ownership crosses explicit host-renderer
 * publication boundaries, while authored arguments and state changes at
 * those boundaries remain exact.
 *
 * Provenance:
 *   direct      - platform_completeAchievement and
 *                 platform_getCompleteAchievement names/signatures/RVAs,
 *                 plus debug_drawsphere's name/signature/RVA and authored
 *                 arguments, from the exact matching PDB.
 *   decompiled  - debug_drawsphere's optimized release body was checked
 *                 against Ghidra and raw instructions; it emits no primitive.
 *   boundary    - jpb_PlatformSetAchievementHooks optionally intercepts the
 *                 platform achievement backend; with no hook installed the
 *                 canonical Steam owner remains the direct callee.
 *                 The optional debug sphere publication hook observes the
 *                 otherwise inert retail call. The screen-polygon hook
 *                 retains the exact _StartPoly,
 *                 _SetVert, and _EndPoly payload while replacing only the
 *                 retail renderer-builder singleton.
 *
 * PDB module: 0095
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\wHook.obj
 * Primary source: W:\SWJediPowerBattles\Work\wHook.cpp
 * Compiler language: c++
 * Emitted procedures: 499
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/platform.h"
#include "jpb/alltext.h"
#include "jpb/camera.h"
#include "jpb/console.h"
#include "jpb/debugtext.h"
#include "jpb/d3dapp.h"
#include "jpb/d3derr.h"
#include "jpb/d3dframe.h"
#include "jpb/d3dtextr.h"
#include "jpb/d3dutil.h"
#include "jpb/el_chavo.h"
#include "jpb/font_atlas.h"
#include "jpb/generic_hook.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/input.h"
#include "jpb/loader.h"
#include "jpb/menu.h"
#include "jpb/main.h"
#include "jpb/resources.h"
#include "jpb/savegame.h"
#include "jpb/steam_achievements.h"
#include "jpb/steam_game_manager.h"
#include "jpb/steam_interfaces.h"
#include "jpb/steam_rich_presence.h"
#include "jpb/text.h"
#include "jpb/theoraplay.h"
#include "jpb/texture.h"
#include "jpb/whook.h"
#include "jpb/world.h"
#include "jpb/wrender.h"
#include "jpb/zerobss.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static JPBPlatformAchievementHooks jpb_platform_achievement_hooks;
static void *jpb_platform_achievement_user_data;
static JPBDrawTextureHook jpb_draw_texture_hook;
static void *jpb_draw_texture_user_data;
static JPBClearWindowHook jpb_clear_window_hook;
static void *jpb_clear_window_user_data;
static JPBRenderLoadHook jpb_render_load_hook;
static void *jpb_render_load_user_data;
static JPBGetWindowSizeHook jpb_get_window_size_hook;
static void *jpb_get_window_size_user_data;
static JPBDrawTextureClippedHook jpb_draw_texture_clipped_hook;
static void *jpb_draw_texture_clipped_user_data;
static JPBDrawUITextUTF16Hook jpb_draw_ui_text_utf16_hook;
static void *jpb_draw_ui_text_utf16_user_data;
static JPBDrawUITextUTF163DHook jpb_draw_ui_text_utf16_3d_hook;
static void *jpb_draw_ui_text_utf16_3d_user_data;
static bool resolutionComparison(
    const RESOLUTION &left, const RESOLUTION &right);

static RECT jpb_whook_video_destination(
    std::uint32_t screen_width, std::uint32_t screen_height)
{
    constexpr float video_aspect = 1.7777778f;
    const float width = static_cast<float>(screen_width);
    float height = static_cast<float>(screen_height);
    float left;
    float top;
    float right;

    if (video_aspect <= width / height) {
        right = height * video_aspect;
        left = (width - right) * 0.5f;
        top = 0.0f;
    } else {
        left = 0.0f;
        top = (height - width / video_aspect) * 0.5f;
        right = width;
        height = width / video_aspect;
    }

    RECT destination;
    destination.left = static_cast<LONG>(left);
    destination.top = static_cast<LONG>(top);
    destination.right = static_cast<LONG>(left + right);
    destination.bottom = static_cast<LONG>(top + height);
    return destination;
}

static float jpb_whook_text_3d_line_advance(
    float y, float maximum_height)
{
    return y + maximum_height * 1.5f;
}

static float jpb_whook_text_3d_quad(
    const GlyphEntry &entry,
    float atlas_width,
    float atlas_height,
    float scale,
    float maximum_height,
    float x,
    float y,
    float z,
    std::uint32_t color,
    JPBScreenPolyVertex vertices[4])
{
    const float glyph_width =
        static_cast<float>(entry.Right - entry.Left) * scale;
    const float glyph_height =
        static_cast<float>(entry.Bottom - entry.Top) * scale;
    const float glyph_top =
        y + (maximum_height - glyph_height) * 0.5f;
    const float glyph_right = x + glyph_width;
    const float glyph_bottom = glyph_top + glyph_height;
    const float left_u = static_cast<float>(entry.Left) / atlas_width;
    const float right_u = static_cast<float>(entry.Right) / atlas_width;
    const float top_v = static_cast<float>(entry.Top) / atlas_height;
    const float bottom_v = static_cast<float>(entry.Bottom) / atlas_height;

    vertices[0] = {x, glyph_top, z, color, left_u, top_v};
    vertices[1] = {glyph_right, glyph_top, z, color, right_u, top_v};
    vertices[2] = {x, glyph_bottom, z, color, left_u, bottom_v};
    vertices[3] = {glyph_right, glyph_bottom, z, color, right_u, bottom_v};
    return x + scale + scale + glyph_width;
}

typedef struct JPBPortableTextClipState {
    int enabled;
    int left;
    int top;
    int right;
    int bottom;
} JPBPortableTextClipState;

static JPBPortableTextClipState jpb_whook_begin_legacy_text_clip(void)
{
    JPBPortableTextClipState previous = {};

    previous.enabled = jpb_TextGetClipRect(
        &previous.left,
        &previous.top,
        &previous.right,
        &previous.bottom);
    if (textClipping != 0) {
        jpb_TextSetClipRect(
            (int)textclipRect[0],
            (int)textclipRect[1],
            (int)textclipRect[2],
            (int)textclipRect[3]);
    }
    return previous;
}

static void jpb_whook_end_legacy_text_clip(
    const JPBPortableTextClipState &previous)
{
    if (textClipping == 0) {
        return;
    }
    if (previous.enabled != 0) {
        jpb_TextSetClipRect(
            previous.left,
            previous.top,
            previous.right,
            previous.bottom);
    } else {
        jpb_TextClearClipRect();
    }
}
static JPBDebugSphereHook jpb_debug_sphere_hook;
static void *jpb_debug_sphere_user_data;
static JPBScreenPolyHook jpb_screen_poly_hook;
static void *jpb_screen_poly_user_data;
static JPBInitFBXLevelDataHook jpb_init_fbx_level_data_hook;
static void *jpb_init_fbx_level_data_user_data;
static JPBLevelTransformation jpb_level_transformation;
static std::map<std::string, _Material *> jpb_try_texture_cache;
static std::unordered_map<std::string, _Material *> jpb_texture_cache;
/* Exact PDB locals at matched-PC RVAs 0x582E5C and 0x92D8F0..0x92D8F8. */
static int near_z;
static float total_z;
static int alphapolys;
static Texture *oldtexture;
static AudioQueue *volatile audio_queue;
static AudioQueue *volatile audio_queue_tail;
static unsigned int baseticks;
static LARGE_INTEGER lastTime;

typedef struct JPBWHookSDLImports {
    void *(*create_texture_from_surface)(void *renderer, void *surface);
    int (*render_copy)(
        void *renderer,
        void *texture,
        const void *source,
        const void *destination);
    void (*destroy_texture)(void *texture);
    void (*free_surface)(void *surface);
    int (*render_set_clip_rect)(void *renderer, const void *rectangle);
    int (*set_texture_color_mod)(
        void *texture,
        std::uint8_t red,
        std::uint8_t green,
        std::uint8_t blue);
    int (*set_texture_alpha_mod)(void *texture, std::uint8_t alpha);
    int (*set_texture_blend_mode)(void *texture, int blend_mode);
    int (*render_copy_ex)(
        void *renderer,
        void *texture,
        const void *source,
        const void *destination,
        double angle,
        const void *center,
        int flip);
    char *(*get_base_path)(void);
    void (*delay)(std::uint32_t milliseconds);
    int (*open_audio)(
        const SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
    void (*pause_audio)(int pause_on);
    std::uint32_t (*get_ticks)(void);
    int (*lock_surface)(SDL_Surface *surface);
    void (*unlock_surface)(SDL_Surface *surface);
    int (*poll_event)(SDL_Event *event);
    void (*lock_audio)(void);
    void (*unlock_audio)(void);
    void (*close_audio)(void);
    void *(*malloc_memory)(std::size_t size);
    void (*free_memory)(void *memory);
} JPBWHookSDLImports;

#if defined(JPB_WHOOK_TESTING)
static JPBWHookSDLImports jpb_whook_sdl_test_imports;
static bool jpb_whook_has_sdl_test_imports;
static JPBWHookWinMainTestHooks jpb_whook_win_main_test_hooks;
static bool jpb_whook_has_win_main_test_hooks;
#endif

template <typename Function>
static Function jpb_whook_resolve_sdl(const char *name)
{
    static HMODULE module = []() {
        HMODULE loaded = GetModuleHandleA("SDL2.dll");
        if (loaded == nullptr) {
            loaded = LoadLibraryA("SDL2.dll");
        }
        if (loaded == nullptr) {
            RaiseFailFastException(nullptr, nullptr, 0);
        }
        return loaded;
    }();
    FARPROC procedure = GetProcAddress(module, name);

    if (procedure == nullptr) {
        RaiseFailFastException(nullptr, nullptr, 0);
    }
    return reinterpret_cast<Function>(procedure);
}

static const JPBWHookSDLImports &jpb_whook_sdl_imports(void)
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_sdl_test_imports) {
        return jpb_whook_sdl_test_imports;
    }
#endif
    static const JPBWHookSDLImports imports = {
        jpb_whook_resolve_sdl<
            decltype(JPBWHookSDLImports::create_texture_from_surface)>(
            "SDL_CreateTextureFromSurface"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::render_copy)>(
            "SDL_RenderCopy"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::destroy_texture)>(
            "SDL_DestroyTexture"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::free_surface)>(
            "SDL_FreeSurface"),
        jpb_whook_resolve_sdl<
            decltype(JPBWHookSDLImports::render_set_clip_rect)>(
            "SDL_RenderSetClipRect"),
        jpb_whook_resolve_sdl<
            decltype(JPBWHookSDLImports::set_texture_color_mod)>(
            "SDL_SetTextureColorMod"),
        jpb_whook_resolve_sdl<
            decltype(JPBWHookSDLImports::set_texture_alpha_mod)>(
            "SDL_SetTextureAlphaMod"),
        jpb_whook_resolve_sdl<
            decltype(JPBWHookSDLImports::set_texture_blend_mode)>(
            "SDL_SetTextureBlendMode"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::render_copy_ex)>(
            "SDL_RenderCopyEx"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::get_base_path)>(
            "SDL_GetBasePath"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::delay)>(
            "SDL_Delay"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::open_audio)>(
            "SDL_OpenAudio"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::pause_audio)>(
            "SDL_PauseAudio"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::get_ticks)>(
            "SDL_GetTicks"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::lock_surface)>(
            "SDL_LockSurface"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::unlock_surface)>(
            "SDL_UnlockSurface"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::poll_event)>(
            "SDL_PollEvent"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::lock_audio)>(
            "SDL_LockAudio"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::unlock_audio)>(
            "SDL_UnlockAudio"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::close_audio)>(
            "SDL_CloseAudio"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::malloc_memory)>(
            "SDL_malloc"),
        jpb_whook_resolve_sdl<decltype(JPBWHookSDLImports::free_memory)>(
            "SDL_free"),
    };
    return imports;
}

/* Exact trailing key-state fields of matched-PDB CD3DApplication. */
typedef struct JPBWHookKeyState {
    unsigned char m_bKeyMap[256];
    unsigned char m_bRawKeyMap[256];
    unsigned char m_bKeyMapPressed[256];
    unsigned char m_bKeyMapReleased[256];
    int m_nLastKey;
} JPBWHookKeyState;

static_assert(
    offsetof(JPBWHookKeyState, m_bRawKeyMap) == 0x100,
    "CD3DApplication raw key-map offset changed");
static_assert(
    offsetof(JPBWHookKeyState, m_bKeyMapPressed) == 0x200,
    "CD3DApplication pressed key-map offset changed");
static_assert(
    offsetof(JPBWHookKeyState, m_bKeyMapReleased) == 0x300,
    "CD3DApplication released key-map offset changed");
static_assert(
    offsetof(JPBWHookKeyState, m_nLastKey) == 0x400,
    "CD3DApplication last-key offset changed");
static_assert(
    sizeof(JPBWHookKeyState) == 0x404,
    "CD3DApplication key-state slice size changed");

static JPBWHookKeyState jpb_whook_key_state;

char getasciicodefromvirtualkey(
    int w_param,
    int l_param,
    unsigned char *raw_key_map,
    int press);

/* Exact PDB global at matched-PC RVA 0x537DEC. */
int refreshFontAtlasFlag;
/* Exact PDB global at matched-PC RVA 0x539D78. */
_Material *whitemat;
_Material *whitematAdd;

static struct {
    _Material *material;
    uint32_t materialFlags;
    int requestedVertexCount;
    JPBScreenPolyVertex vertices[JPB_SCREEN_POLY_VERTEX_CAPACITY];
} jpb_screen_poly_builder;

void jpb_PlatformSetAchievementHooks(
    const JPBPlatformAchievementHooks *hooks,
    void *user_data)
{
    if (hooks == nullptr) {
        std::memset(
            &jpb_platform_achievement_hooks,
            0,
            sizeof(jpb_platform_achievement_hooks));
        jpb_platform_achievement_user_data = nullptr;
        return;
    }
    jpb_platform_achievement_hooks = *hooks;
    jpb_platform_achievement_user_data = user_data;
}

extern "C" int jpb_PlatformInitializeSteamServices(void)
{
    if (g_SteamAchievements != nullptr &&
        g_SteamGameManager != nullptr &&
        g_SteamRicherPresence != nullptr) {
        return 1;
    }
    if (!SteamAPI_Init()) {
        return 0;
    }
    g_SteamAchievements = new CSteamAchievements(g_Achievements, 43);
    g_SteamGameManager = new CSteamGameManager();
    g_SteamRicherPresence = new CSteamRichPresence();
    return 1;
}

extern "C" void jpb_PlatformShutdownSteamServices(void)
{
    delete g_SteamRicherPresence;
    g_SteamRicherPresence = nullptr;
    delete g_SteamGameManager;
    g_SteamGameManager = nullptr;
    delete g_SteamAchievements;
    g_SteamAchievements = nullptr;
    SteamAPI_Shutdown();
}

void jpb_WHookSetDrawTextureHook(
    JPBDrawTextureHook hook, void *user_data)
{
    jpb_draw_texture_hook = hook;
    jpb_draw_texture_user_data = user_data;
}

void jpb_WHookSetClearWindowHook(
    JPBClearWindowHook hook, void *user_data)
{
    jpb_clear_window_hook = hook;
    jpb_clear_window_user_data = user_data;
}

void jpb_WHookSetRenderLoadHook(
    JPBRenderLoadHook hook, void *user_data)
{
    jpb_render_load_hook = hook;
    jpb_render_load_user_data = user_data;
}

void jpb_WHookSetGetWindowSizeHook(
    JPBGetWindowSizeHook hook, void *user_data)
{
    jpb_get_window_size_hook = hook;
    jpb_get_window_size_user_data = user_data;
}

void jpb_WHookSetDrawTextureClippedHook(
    JPBDrawTextureClippedHook hook, void *user_data)
{
    jpb_draw_texture_clipped_hook = hook;
    jpb_draw_texture_clipped_user_data = user_data;
}

void jpb_WHookSetDrawUITextUTF16Hook(
    JPBDrawUITextUTF16Hook hook, void *user_data)
{
    jpb_draw_ui_text_utf16_hook = hook;
    jpb_draw_ui_text_utf16_user_data = user_data;
}

void jpb_WHookSetDrawUITextUTF163DHook(
    JPBDrawUITextUTF163DHook hook, void *user_data)
{
    jpb_draw_ui_text_utf16_3d_hook = hook;
    jpb_draw_ui_text_utf16_3d_user_data = user_data;
}

void jpb_WHookSetDebugSphereHook(
    JPBDebugSphereHook hook, void *user_data)
{
    jpb_debug_sphere_hook = hook;
    jpb_debug_sphere_user_data = user_data;
}

void jpb_WHookSetScreenPolyHook(
    JPBScreenPolyHook hook, void *user_data)
{
    jpb_screen_poly_hook = hook;
    jpb_screen_poly_user_data = user_data;
}

void jpb_WHookSetInitFBXLevelDataHook(
    JPBInitFBXLevelDataHook hook, void *user_data)
{
    jpb_init_fbx_level_data_hook = hook;
    jpb_init_fbx_level_data_user_data = user_data;
}

const JPBLevelTransformation *jpb_WHookLevelTransformation(void)
{
    std::memcpy(
        jpb_level_transformation.world,
        &chavo.m_fbxWorldMatrix,
        sizeof(jpb_level_transformation.world));
    DirectX::XMStoreFloat4(
        reinterpret_cast<DirectX::XMFLOAT4 *>(
            jpb_level_transformation.scale),
        chavo.m_sceneConstantBufferData.levelScale);
    return &jpb_level_transformation;
}

/* 0x37C0, 32 bytes, local, 0 named locals
 * `dynamic initializer for 'chavo''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x37E0, 21 bytes, local, 0 named locals
 * `dynamic initializer for 'framelast''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
static std::chrono::system_clock::time_point framelast =
    std::chrono::system_clock::now();

/* 0x3800, 21 bytes, local, 0 named locals
 * `dynamic initializer for 'framenow''
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x10ADB0, 332 bytes, global, 15 named locals
 * std::vector<unsigned int,std::allocator<unsigned int> >::_Assign_counted_range<unsigned int *>
 * PDB type: void std::vector<unsigned int,st...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10AF00, 334 bytes, global, 14 named locals
 * std::vector<std::_Tgt_state_t<char const *>::_Grp_t,std::allocator<std::_Tgt_state_t<char const *>::_Grp_t> >::_Assign_counted_range<std::_Tgt_state_t<char const *>::_Grp_t *>
 * PDB type: void std::vector<std::_Tgt_state...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10B050, 143 bytes, global, 6 named locals
 * std::_Cmp_chrange<char const *,char const *,std::_Cmp_collate<std::regex_traits<char> > >
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10B0E0, 47 bytes, global, 6 named locals
 * std::_Cmp_chrange<char const *,char const *,std::_Cmp_cs<std::regex_traits<char> > >
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10B110, 147 bytes, global, 6 named locals
 * std::_Cmp_chrange<char const *,char const *,std::_Cmp_icase<std::regex_traits<char> > >
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10B1B0, 294 bytes, global, 13 named locals
 * std::_Compare<char const *,char const *,std::regex_traits<char> >
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10B2E0, 303 bytes, global, 10 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::_Construct<1,char16_t const *>
 * PDB type: void std::basic_string<char16_t,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10B410, 250 bytes, global, 3 named locals
 * std::filesystem::_Convert_Source_to_wide<char [256],std::filesystem::_Normal_conversion>
 * PDB type: std::basic_string<wchar_t,std::c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x10B510, 240 bytes, global, 3 named locals
 * std::filesystem::_Convert_stringoid_to_wide<std::filesystem::_Normal_conversion>
 * PDB type: std::basic_string<wchar_t,std::c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x10B600, 227 bytes, global, 4 named locals
 * std::filesystem::_Convert_wide_to_narrow_replace_chars<std::char_traits<char>,std::allocator<char> >
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x10B6F0, 23 bytes, global, 4 named locals
 * std::_Copy_backward_memmove<RESOLUTION *,RESOLUTION *>
 * PDB type: RESOLUTION* (RESOLUTION*, RESOLU...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B710, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<char *,char *>
 * PDB type: char* (char*, char*, char*)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B740, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<unsigned short *,unsigned short *>
 * PDB type: unsigned short* (unsigned short*...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B770, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<CD3DApplication::FBX_MESH * *,CD3DApplication::FBX_MESH * *>
 * PDB type: CD3DApplication::FBX_MESH** (CD3...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B7A0, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<std::_Tgt_state_t<char const *>::_Grp_t *,std::_Tgt_state_t<char const *>::_Grp_t *>
 * PDB type: std::_Tgt_state_t<char const *>:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B7D0, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<std::_Loop_vals_t *,std::_Loop_vals_t *>
 * PDB type: std::_Loop_vals_t* (std::_Loop_v...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B800, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<std::sub_match<char const *> *,std::sub_match<char const *> *>
 * PDB type: std::sub_match<char const *>* (s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B830, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<char const *,char *>
 * PDB type: char* (const char*, const char*,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B860, 48 bytes, global, 4 named locals
 * std::_Copy_memmove<unsigned int const *,unsigned int *>
 * PDB type: unsigned* (const unsigned*, cons...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B890, 50 bytes, global, 5 named locals
 * std::_Copy_memmove_n<unsigned int *,unsigned int *>
 * PDB type: unsigned* (unsigned*, const unsi...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B8D0, 49 bytes, global, 5 named locals
 * std::_Copy_memmove_n<std::_Tgt_state_t<char const *>::_Grp_t *,std::_Tgt_state_t<char const *>::_Grp_t *>
 * PDB type: std::_Tgt_state_t<char const *>:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10B910, 1074 bytes, global, 44 named locals
 * std::_Copy_vbool<std::_Vb_iterator<std::_Wrap_alloc<std::allocator<unsigned int> > >,std::_Vb_iterator<std::_Wrap_alloc<std::allocator<unsigned int> > > >
 * PDB type: std::_Vb_iterator<std::_Wrap_all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10BD50, 49 bytes, global, 3 named locals
 * std::_Destroy_range<std::allocator<CD3DApplication::SubMeshSet> >
 * PDB type: void (CD3DApplication::SubMeshSe...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x10BD90, 415 bytes, global, 23 named locals
 * std::vector<CD3DApplication::FBX_MESH *,std::allocator<CD3DApplication::FBX_MESH *> >::_Emplace_reallocate<CD3DApplication::FBX_MESH * const &>
 * PDB type: CD3DApplication::FBX_MESH** std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10BF30, 592 bytes, global, 19 named locals
 * std::vector<CD3DApplication::SubMeshSet,std::allocator<CD3DApplication::SubMeshSet> >::_Emplace_reallocate<CD3DApplication::SubMeshSet const &>
 * PDB type: CD3DApplication::SubMeshSet* std...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10C180, 540 bytes, global, 21 named locals
 * std::vector<Vertex,std::allocator<Vertex> >::_Emplace_reallocate<Vertex const &>
 * PDB type: Vertex* std::vector<Vertex,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10C3A0, 411 bytes, global, 24 named locals
 * std::vector<unsigned short,std::allocator<unsigned short> >::_Emplace_reallocate<unsigned short>
 * PDB type: unsigned short* std::vector<unsi...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10C540, 85 bytes, global, 4 named locals
 * std::_Tree_val<std::_Tree_simple_types<std::pair<unsigned short const ,unsigned int> > >::_Erase_tree<std::allocator<std::_Tree_node<std::pair<unsigned short const ,unsigned int>,void *> > >
 * PDB type: void std::_Tree_val<std::_Tree_s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0x10C5A0, 15 bytes, global, 2 named locals
 * std::_Fill_zero_memset<unsigned int *>
 * PDB type: void (unsigned*, const unsigned ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10C5B0, 204 bytes, global, 10 named locals
 * std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Find_last<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >
 * PDB type: std::_Hash_find_last_result<std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x10C680, 195 bytes, global, 6 named locals
 * std::_Tree<std::_Tmap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Find_lower_bound<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >
 * PDB type: std::_Tree_find_result<std::_Tre...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0x10C750, 1223 bytes, global, 37 named locals
 * std::_Format_default<char const *,std::allocator<std::sub_match<char const *> >,char const *,std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
 * PDB type: std::back_insert_iterator<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10CC20, 550 bytes, global, 14 named locals
 * std::_Format_sed<char const *,std::allocator<std::sub_match<char const *> >,char const *,std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >
 * PDB type: std::back_insert_iterator<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10CE50, 108 bytes, global, 6 named locals
 * std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *>::_Freenode<std::allocator<std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_List_node<std::pair<s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\list
 */

/* 0x10CEC0, 8 bytes, global, 0 named locals
 * std::_Immortalize_memcpy_image<std::_Generic_error_category>
 * PDB type: const std::_Generic_error_catego...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x10CED0, 8 bytes, global, 0 named locals
 * std::_Immortalize_memcpy_image<std::_System_error_category>
 * PDB type: const std::_System_error_categor...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x10CEE0, 7 bytes, global, 1 named locals
 * std::_Is_all_bits_zero<unsigned int>
 * PDB type: bool (const unsigned&)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10CEF0, 137 bytes, global, 6 named locals
 * std::_Lookup_coll<char const *,char>
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10CF80, 1495 bytes, global, 49 named locals
 * std::_Lookup_equiv<char,std::regex_traits<char> >
 * PDB type: bool (unsigned char, const std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10D560, 55 bytes, global, 3 named locals
 * std::_Lookup_range<char>
 * PDB type: bool (unsigned, const std::_Buf<...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10D5A0, 549 bytes, global, 10 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Match<std::allocator<std::sub_match<char const *> > >
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10D7D0, 125 bytes, global, 7 named locals
 * std::_Med3_unchecked<RESOLUTION *,bool (__cdecl*)(RESOLUTION const &,RESOLUTION const &)>
 * PDB type: void (RESOLUTION*, RESOLUTION*, ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x10D850, 572 bytes, global, 17 named locals
 * std::_Partition_by_median_guess_unchecked<RESOLUTION *,bool (__cdecl*)(RESOLUTION const &,RESOLUTION const &)>
 * PDB type: std::pair<RESOLUTION *,RESOLUTIO...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x10DA90, 255 bytes, global, 10 named locals
 * std::_Pop_heap_hole_by_index<RESOLUTION *,RESOLUTION,bool (__cdecl*)(RESOLUTION const &,RESOLUTION const &)>
 * PDB type: void (RESOLUTION*, __int64, __in...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x10DB90, 365 bytes, global, 18 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Reallocate_grow_by<<lambda_319d5e083f45f90dcdce5dce53cbb275>,char>
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10DD00, 331 bytes, global, 18 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Reallocate_grow_by<<lambda_9013ee9e23efe4882b67eff5b0ecf103> >
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10DE50, 449 bytes, global, 20 named locals
 * std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >::_Reallocate_grow_by<<lambda_a3050a43f3157934f354774ab3dd2e02>,unsigned __int64,wchar_t>
 * PDB type: std::basic_string<wchar_t,std::c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10E020, 395 bytes, global, 20 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_Reallocate_grow_by<<lambda_e1befb086ad3257e3f042a63030725f7>,unsigned __int64,char>
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10E1B0, 515 bytes, global, 21 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::_Reallocate_grow_by<<lambda_e749a49405295b58ff21f3ec583d0a05>,unsigned __int64,char16_t const *,unsigned __int64>
 * PDB type: std::basic_string<char16_t,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x10E3C0, 733 bytes, global, 22 named locals
 * std::_Regex_replace1<std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,char const *,std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >
 * PDB type: std::back_insert_iterator<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10E6A0, 516 bytes, global, 12 named locals
 * std::_Regex_search2<char const *,std::allocator<std::sub_match<char const *> >,char,std::regex_traits<char>,char const *>
 * PDB type: bool (const char*, const char*, ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10E8B0, 2188 bytes, global, 29 named locals
 * std::basic_regex<char,std::regex_traits<char> >::_Reset<char const *>
 * PDB type: void std::basic_regex<char,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x10F140, 402 bytes, global, 21 named locals
 * std::vector<unsigned int,std::allocator<unsigned int> >::_Resize_reallocate<unsigned int>
 * PDB type: void std::vector<unsigned int,st...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10F2E0, 353 bytes, global, 20 named locals
 * std::vector<std::_Tgt_state_t<char const *>::_Grp_t,std::allocator<std::_Tgt_state_t<char const *>::_Grp_t> >::_Resize_reallocate<std::_Value_init_tag>
 * PDB type: void std::vector<std::_Tgt_state...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10F450, 353 bytes, global, 20 named locals
 * std::vector<std::_Loop_vals_t,std::allocator<std::_Loop_vals_t> >::_Resize_reallocate<std::_Value_init_tag>
 * PDB type: void std::vector<std::_Loop_vals...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10F5C0, 471 bytes, global, 18 named locals
 * std::vector<std::sub_match<char const *>,std::allocator<std::sub_match<char const *> > >::_Resize_reallocate<std::_Value_init_tag>
 * PDB type: void std::vector<std::sub_match<...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x10F7A0, 738 bytes, global, 19 named locals
 * std::_Sort_unchecked<RESOLUTION *,bool (__cdecl*)(RESOLUTION const &,RESOLUTION const &)>
 * PDB type: void (RESOLUTION*, RESOLUTION*, ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\algorithm
 */

/* 0x10FA90, 33 bytes, global, 1 named locals
 * std::filesystem::_Stringoid_from_Source<char [256]>
 * PDB type: std::basic_string_view<char,std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x10FAC0, 108 bytes, global, 2 named locals
 * std::_To_absolute_time<__int64,std::ratio<1,1000> >
 * PDB type: std::chrono::time_point<std::chr...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\thread
 */

/* 0x10FB30, 737 bytes, global, 20 named locals
 * std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Try_emplace<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const &>
 * PDB type: std::pair<std::_List_node<std::p...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x10FE20, 144 bytes, global, 11 named locals
 * std::_Uninitialized_move<CD3DApplication::SubMeshSet *,std::allocator<CD3DApplication::SubMeshSet> >
 * PDB type: CD3DApplication::SubMeshSet* (CD...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x10FEB0, 5 bytes, global, 3 named locals
 * __std_find_trivial<char const ,unsigned char>
 * PDB type: const char* (const char*, const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10FEC0, 132 bytes, global, 4 named locals
 * std::fill<std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > > > *,std::_List_unchecked_iterator<std::_List_val<std::_List_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > > > >
 * PDB type: void (std::_List_unchecked_itera...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xutility
 */

/* 0x10FF50, 322 bytes, global, 10 named locals
 * std::_Regex_traits<char>::lookup_classname<char const *>
 * PDB type: short std::_Regex_traits<char>::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1100A0, 65 bytes, global, 6 named locals
 * std::regex_replace<std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::_String_const_iterator<std::_String_val<std::_Simple_types<char> > >,std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >
 * PDB type: std::back_insert_iterator<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1100F0, 279 bytes, global, 10 named locals
 * std::regex_replace<std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110210, 353 bytes, global, 10 named locals
 * std::this_thread::sleep_for<__int64,std::ratio<1,1000> >
 * PDB type: void (const std::chrono::duratio...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\thread
 */

/* 0x110380, 313 bytes, global, 9 named locals
 * std::this_thread::sleep_until<std::chrono::steady_clock,std::chrono::duration<__int64,std::ratio<1,1000000000> > >
 * PDB type: void (const std::chrono::time_po...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\thread
 */

/* 0x1104C0, 576 bytes, global, 24 named locals
 * std::_Regex_traits<char>::transform_primary<char *>
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110700, 576 bytes, global, 24 named locals
 * std::_Regex_traits<char>::transform_primary<char const *>
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110940, 587 bytes, global, 23 named locals
 * std::_Regex_traits<char>::transform_primary<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110B90, 270 bytes, global, 10 named locals
 * std::use_facet<std::collate<char> >
 * PDB type: const std::collate<char>& (const...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0x110CA0, 175 bytes, global, 6 named locals
 * std::_Bt_state_t<char const *>::_Bt_state_t<char const *>
 * PDB type: void std::_Bt_state_t<char const...
 * Source: no line mapping
 */

/* 0x110D50, 145 bytes, global, 3 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Builder<char const *,char,std::regex_traits<char> >
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110DF0, 312 bytes, global, 13 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Matcher<char const *,char,std::regex_traits<char>,char const *>
 * PDB type: void std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x110F30, 347 bytes, global, 7 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Parser<char const *,char,std::regex_traits<char> >
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x111090, 403 bytes, global, 12 named locals
 * std::basic_regex<char,std::regex_traits<char> >::basic_regex<char,std::regex_traits<char> >
 * PDB type: void std::basic_regex<char,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x111230, 299 bytes, global, 10 named locals
 * std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >
 * PDB type: void std::basic_string<wchar_t,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x111360, 379 bytes, global, 15 named locals
 * CD3DApplication::SubMeshSet::SubMeshSet
 * PDB type: void CD3DApplication::SubMeshSet...
 * Source: no line mapping
 */

/* 0x1114E0, 4 bytes, global, 1 named locals
 * _D3DTLVERTEX::_D3DTLVERTEX
 * PDB type: void _D3DTLVERTEX::()
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um\d3dtypes.h
 */

/* 0x1114F0, 4 bytes, global, 1 named locals
 * _linked_poly::_linked_poly
 * PDB type: void _linked_poly::()
 * Source: no line mapping
 */

/* 0x111500, 576 bytes, global, 11 named locals
 * el_chavo::el_chavo
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
static std::chrono::system_clock::time_point framenow =
    std::chrono::system_clock::now();
el_chavo::el_chavo()
{
    currentpolyistrans = 0;
    UpdateValidResolutions();
    SetCompanyTitle(const_cast<char *>("LucasArts"));
    SetAppTitle(const_cast<char *>(
        "STAR WARS: Episode I: Jedi Power Battles"));
    SetUseZBuffer(1);
    SetWindowFlags(ConvertWindowSettingToFlags(0));
    SetWidthHeight(
        static_cast<unsigned>(g_resolutions[0].width),
        static_cast<unsigned>(g_resolutions[0].height));
    SetBitDepth(16);
    svprintf(nullptr, const_cast<char *>("d"), nullptr);
    D3DUtil_InitMaterial(m, 1.0f, 1.0f, 1.0f, 1.0f);
}

el_chavo chavo;

/* Exact matched-PC PDB global at RVA 0x4CDE70. */
Achievement_t g_Achievements[43] = {
    {1, "ACH_GRAND_MASTER"},
    {2, "ACH_BATTER_UP"},
    {3, "ACH_AAAAAAH"},
    {4, "ACH_TRANSLATE_THIS"},
    {5, "ACH_BLADE_AMPLIFIER"},
    {6, "ACH_JEDI_MASTER"},
    {7, "ACH_TOUGH_NEGOTIATOR"},
    {8, "ACH_YOU_HEAR_THAT"},
    {9, "ACH_SCAVENGER_HUNT"},
    {10, "ACH_ROYAL_ARCHITECTURE"},
    {11, "ACH_TUSKEN_DEATH_CRY"},
    {12, "ACH_PARDON_ME_COMING_THROUGH"},
    {13, "ACH_NICE_WALK_IN_THE_WOODS"},
    {14, "ACH_CRUISIN"},
    {15, "ACH_CLIFF_HANGER"},
    {16, "ACH_MAULD"},
    {17, "ACH_BATTLEMECH"},
    {18, "ACH_YEEHAW"},
    {19, "ACH_JAR_JAR_JAIL"},
    {20, "ACH_UNLIMITED_POWER"},
    {21, "ACH_AGGRESSIVE_NEGOTIATIONS"},
    {22, "ACH_JEDI_ARE_NOT_TO_BE"},
    {23, "ACH_COMMUNICATIONS_DISRUPTION"},
    {24, "ACH_INVASION"},
    {25, "ACH_THATS_SO_WIZARD"},
    {26, "ACH_A_SURPRISE_TO_BE_SURE"},
    {27, "ACH_GUNGAN_NO_LIKIN_OUTSIDERS"},
    {28, "ACH_THE_SPEEDIEST_WAY"},
    {29, "ACH_SEEKING_THE_HIGHGROUND"},
    {30, "ACH_ALWAYS_TWO_THERE_ARE"},
    {31, "ACH_TANK_WARS"},
    {32, "ACH_1000_YEARS_IN_THE_PIT"},
    {33, "ACH_GREAT_ROLLIN_DEATH_BALLS"},
    {34, "ACH_HELPING_HAND_MAIDENS"},
    {35, "ACH_IM_A_PILOT"},
    {36, "ACH_PORTABLE_BACTA"},
    {37, "ACH_FORCE_GRENADE"},
    {38, "ACH_DARTH_MAUL"},
    {39, "ACH_FOOD_DELIVERY"},
    {40, "ACH_MASTER_MACE_WINDU"},
    {41, "ACH_MASTER_PLO_KOON"},
    {42, "ACH_CAPTAIN_QUARSH_PANAKA"},
    {43, "ACH_AUGIES_GREAT_MUNICIPAL_BAND"},
};

/* 0x111740, 134 bytes, global, 2 named locals
 * std::filesystem::filesystem_error::filesystem_error
 * PDB type: void std::filesystem::filesystem...
 * Source: no line mapping
 */

/* 0x1117D0, 302 bytes, global, 7 named locals
 * std::filesystem::filesystem_error::filesystem_error
 * PDB type: void std::filesystem::filesystem...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x111900, 231 bytes, global, 7 named locals
 * std::system_error::system_error
 * PDB type: void std::system_error::(std::er...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x1119F0, 20 bytes, global, 2 named locals
 * std::_Alloc_construct_ptr<std::allocator<std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >::~_Alloc_construct_ptr<std::allocator<std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_Alloc_construct_ptr<s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x111A10, 20 bytes, global, 2 named locals
 * std::_Alloc_construct_ptr<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >::~_Alloc_construct_ptr<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_Alloc_construct_ptr<s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x111A30, 93 bytes, global, 5 named locals
 * std::_Bt_state_t<char const *>::~_Bt_state_t<char const *>
 * PDB type: void std::_Bt_state_t<char const...
 * Source: no line mapping
 */

/* 0x111A90, 133 bytes, global, 6 named locals
 * std::_List_node_emplace_op2<std::allocator<std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >::~_List_node_emplace_op2<std::allocator<std::_List_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_List_node_emplace_op2...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\list
 */

/* 0x111B20, 124 bytes, global, 5 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::~_Matcher<char const *,char,std::regex_traits<char>,char const *>
 * PDB type: void std::_Matcher<char const *,...
 * Source: no line mapping
 */

/* 0x111BA0, 93 bytes, global, 5 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::~_Parser<char const *,char,std::regex_traits<char> >
 * PDB type: void std::_Parser<char const *,c...
 * Source: no line mapping
 */

/* 0x111C00, 172 bytes, global, 9 named locals
 * std::_Tgt_state_t<char const *>::~_Tgt_state_t<char const *>
 * PDB type: void std::_Tgt_state_t<char cons...
 * Source: no line mapping
 */

/* 0x111CB0, 95 bytes, global, 4 named locals
 * std::_Tidy_guard<std::_Builder<char const *,char,std::regex_traits<char> > >::~_Tidy_guard<std::_Builder<char const *,char,std::regex_traits<char> > >
 * PDB type: void std::_Tidy_guard<std::_Buil...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x111D10, 20 bytes, global, 2 named locals
 * std::_Tree_temp_node_alloc<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >::~_Tree_temp_node_alloc<std::allocator<std::_Tree_node<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *>,void *> > >
 * PDB type: void std::_Tree_temp_node_alloc<...
 * Source: no line mapping
 */

/* 0x111D30, 91 bytes, global, 5 named locals
 * std::_Vb_val<std::allocator<bool> >::~_Vb_val<std::allocator<bool> >
 * PDB type: void std::_Vb_val<std::allocator...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x111D90, 5 bytes, global, 1 named locals
 * std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >::~basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >
 * PDB type: void std::basic_string<wchar_t,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x111DA0, 44 bytes, global, 1 named locals
 * std::collate<char>::~collate<char>
 * PDB type: void std::collate<char>::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\locale
 */

/* 0x111DD0, 92 bytes, global, 3 named locals
 * std::map<unsigned short,unsigned int,std::less<unsigned short>,std::allocator<std::pair<unsigned short const ,unsigned int> > >::~map<unsigned short,unsigned int,std::less<unsigned short>,std::allocator<std::pair<unsigned short const ,unsigned int> > >
 * PDB type: void std::map<unsigned short,uns...
 * Source: no line mapping
 */

/* 0x111E30, 9 bytes, global, 1 named locals
 * std::match_results<char const *,std::allocator<std::sub_match<char const *> > >::~match_results<char const *,std::allocator<std::sub_match<char const *> > >
 * PDB type: void std::match_results<char con...
 * Source: no line mapping
 */

/* 0x111E40, 47 bytes, global, 1 named locals
 * std::regex_traits<char>::~regex_traits<char>
 * PDB type: void std::regex_traits<char>::()
 * Source: no line mapping
 */

/* 0x111E70, 20 bytes, global, 1 named locals
 * std::unique_ptr<std::_Node_assert,std::default_delete<std::_Node_assert> >::~unique_ptr<std::_Node_assert,std::default_delete<std::_Node_assert> >
 * PDB type: void std::unique_ptr<std::_Node_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\memory
 */

/* 0x111E90, 87 bytes, global, 6 named locals
 * std::vector<char,std::allocator<char> >::~vector<char,std::allocator<char> >
 * PDB type: void std::vector<char,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x111EF0, 93 bytes, global, 6 named locals
 * std::vector<unsigned short,std::allocator<unsigned short> >::~vector<unsigned short,std::allocator<unsigned short> >
 * PDB type: void std::vector<unsigned short,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x111F50, 173 bytes, global, 7 named locals
 * std::vector<CD3DApplication::SubMeshSet,std::allocator<CD3DApplication::SubMeshSet> >::~vector<CD3DApplication::SubMeshSet,std::allocator<CD3DApplication::SubMeshSet> >
 * PDB type: void std::vector<CD3DApplication...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x112000, 91 bytes, global, 5 named locals
 * std::vector<std::_Loop_vals_t,std::allocator<std::_Loop_vals_t> >::~vector<std::_Loop_vals_t,std::allocator<std::_Loop_vals_t> >
 * PDB type: void std::vector<std::_Loop_vals...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x112060, 122 bytes, global, 5 named locals
 * std::vector<std::sub_match<char const *>,std::allocator<std::sub_match<char const *> > >::~vector<std::sub_match<char const *>,std::allocator<std::sub_match<char const *> > >
 * PDB type: void std::vector<std::sub_match<...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1120E0, 91 bytes, global, 5 named locals
 * std::vector<bool,std::allocator<bool> >::~vector<bool,std::allocator<bool> >
 * PDB type: void std::vector<bool,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x112140, 104 bytes, global, 6 named locals
 * CD3DApplication::SubMeshSet::~SubMeshSet
 * PDB type: void CD3DApplication::SubMeshSet...
 * Source: no line mapping
 */

/* 0x1121B0, 11 bytes, global, 1 named locals
 * std::_Node_base::~_Node_base
 * PDB type: void std::_Node_base::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1121C0, 8 bytes, global, 1 named locals
 * std::_System_error_message::~_System_error_message
 * PDB type: void std::_System_error_message:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x1121D0, 15 bytes, global, 1 named locals
 * el_chavo::~el_chavo
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
el_chavo::~el_chavo() = default;

/* 0x1121E0, 138 bytes, global, 5 named locals
 * std::filesystem::filesystem_error::~filesystem_error
 * PDB type: void std::filesystem::filesystem...
 * Source: no line mapping
 */

/* 0x112270, 5 bytes, global, 1 named locals
 * std::filesystem::path::~path
 * PDB type: void std::filesystem::path::()
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x112280, 19 bytes, global, 1 named locals
 * std::runtime_error::~runtime_error
 * PDB type: void std::runtime_error::()
 * Source: no line mapping
 */

/* 0x1122A0, 131 bytes, global, 6 named locals
 * std::_Tgt_state_t<char const *>::operator=
 * PDB type: std::_Tgt_state_t<char const *>&...
 * Source: no line mapping
 */

/* 0x112330, 19 bytes, global, 3 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::operator[]
 * PDB type: char& std::basic_string<char,std...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x112350, 233 bytes, global, 4 named locals
 * std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::operator[]
 * PDB type: _Material*& std::map<std::basic_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\map
 */

/* 0x112440, 111 bytes, global, 2 named locals
 * std::_Vb_iterator<std::_Wrap_alloc<std::allocator<unsigned int> > >::operator+
 * PDB type: std::_Vb_iterator<std::_Wrap_all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1124B0, 263 bytes, global, 5 named locals
 * std::_Node_class<char,std::regex_traits<char> >::`scalar deleting destructor'
 * PDB type: void* std::_Node_class<char,std:...
 * Source: no line mapping
 */

/* 0x1125C0, 66 bytes, global, 1 named locals
 * std::_Node_str<char>::`scalar deleting destructor'
 * PDB type: void* std::_Node_str<char>::(uns...
 * Source: no line mapping
 */

/* 0x112610, 76 bytes, global, 1 named locals
 * std::collate<char>::`scalar deleting destructor'
 * PDB type: void* std::collate<char>::(unsig...
 * Source: no line mapping
 */

/* 0x112660, 33 bytes, global, 1 named locals
 * std::_Generic_error_category::`scalar deleting destructor'
 * PDB type: void* std::_Generic_error_catego...
 * Source: no line mapping
 */

/* 0x112690, 129 bytes, global, 3 named locals
 * std::_Node_assert::`scalar deleting destructor'
 * PDB type: void* std::_Node_assert::(unsign...
 * Source: no line mapping
 */

/* 0x112720, 43 bytes, global, 1 named locals
 * std::_Node_back::`scalar deleting destructor'
 * PDB type: void* std::_Node_back::(unsigned...
 * Source: no line mapping
 */

/* 0x112750, 43 bytes, global, 1 named locals
 * std::_Node_base::`scalar deleting destructor'
 * PDB type: void* std::_Node_base::(unsigned...
 * Source: no line mapping
 */

/* 0x112780, 43 bytes, global, 1 named locals
 * std::_Node_capture::`scalar deleting destructor'
 * PDB type: void* std::_Node_capture::(unsig...
 * Source: no line mapping
 */

/* 0x1127B0, 43 bytes, global, 1 named locals
 * std::_Node_end_group::`scalar deleting destructor'
 * PDB type: void* std::_Node_end_group::(uns...
 * Source: no line mapping
 */

/* 0x1127E0, 43 bytes, global, 1 named locals
 * std::_Node_end_rep::`scalar deleting destructor'
 * PDB type: void* std::_Node_end_rep::(unsig...
 * Source: no line mapping
 */

/* 0x112810, 43 bytes, global, 1 named locals
 * std::_Node_endif::`scalar deleting destructor'
 * PDB type: void* std::_Node_endif::(unsigne...
 * Source: no line mapping
 */

/* 0x112840, 188 bytes, global, 6 named locals
 * std::_Node_if::`scalar deleting destructor'
 * PDB type: void* std::_Node_if::(unsigned)
 * Source: no line mapping
 */

/* 0x112900, 43 bytes, global, 1 named locals
 * std::_Node_rep::`scalar deleting destructor'
 * PDB type: void* std::_Node_rep::(unsigned)
 * Source: no line mapping
 */

/* 0x112930, 43 bytes, global, 1 named locals
 * std::_Root_node::`scalar deleting destructor'
 * PDB type: void* std::_Root_node::(unsigned...
 * Source: no line mapping
 */

/* 0x112960, 33 bytes, global, 1 named locals
 * std::_System_error_category::`scalar deleting destructor'
 * PDB type: void* std::_System_error_categor...
 * Source: no line mapping
 */

/* 0x112990, 172 bytes, global, 5 named locals
 * std::filesystem::filesystem_error::`scalar deleting destructor'
 * PDB type: void* std::filesystem::filesyste...
 * Source: no line mapping
 */

/* 0x112A40, 1227 bytes, global, 16 named locals
 * el_chavo::ApplyCulling
 * PDB type: int el_chavo::(&, int, int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x112F10, 169 bytes, global, 5 named locals
 * el_chavo::ApplyLevelTransformation
 * PDB type: void el_chavo::(MATRIX*, float, ...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::ApplyLevelTransformation(
    MATRIX *world_matrix,
    float level_scale_x,
    float level_scale_y,
    float level_scale_z)
{
    float *world = reinterpret_cast<float *>(&m_fbxWorldMatrix);

    world[0] = world_matrix->m[0][0];
    world[1] = world_matrix->m[1][0];
    world[2] = world_matrix->m[2][0];
    world[4] = world_matrix->m[0][1];
    world[5] = world_matrix->m[1][1];
    world[6] = world_matrix->m[2][1];
    world[8] = world_matrix->m[0][2];
    world[9] = world_matrix->m[1][2];
    world[10] = world_matrix->m[2][2];
    world[12] = static_cast<float>(world_matrix->t[0]);
    world[13] = static_cast<float>(world_matrix->t[1]);
    world[14] = static_cast<float>(world_matrix->t[2]);
    m_sceneConstantBufferData.levelScale = DirectX::XMVectorSet(
        level_scale_x,
        level_scale_y,
        level_scale_z,
        1.0f);
}

/* 0x112FC0, 142 bytes, global, 10 named locals
 * el_chavo::ApplyProjection
 * PDB type: void el_chavo::(FVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::ApplyProjection(FVECTOR *vertices)
{
    DirectX::XMVECTOR position = DirectX::XMLoadFloat3(
        reinterpret_cast<DirectX::XMFLOAT3 *>(vertices));

    position = DirectX::XMVectorSetW(position, 1.0f);
    position = DirectX::XMVector3TransformCoord(
        position, m_fbxProjectionMatrix);
    DirectX::XMStoreFloat3(
        reinterpret_cast<DirectX::XMFLOAT3 *>(vertices), position);
}

/* 0x113050, 614 bytes, global, 11 named locals
 * el_chavo::ApplyProjectionPolyArray
 * PDB type: void el_chavo::(&, int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1132C0, 307 bytes, global, 5 named locals
 * el_chavo::CleanupFBXData
 * PDB type: void el_chavo::(std::vector<CD3D...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::CleanupFBXData(std::vector<FBX_MESH *> &meshes)
{
    for (FBX_MESH *mesh : meshes) {
        for (SubMeshSet &submesh : mesh->subMeshes) {
            submesh.vertices.clear();
            submesh.subMeshIndices.clear();
        }
        delete mesh;
    }
    meshes.clear();
}

/* 0x113400, 21 bytes, global, 1 named locals
 * el_chavo::DeleteDeviceObjects
 * PDB type: HRESULT el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
HRESULT el_chavo::DeleteDeviceObjects()
{
    DeleteAllTextures();
    CleanupInput();
    return S_OK;
}

/* 0x113420, 3 bytes, global, 6 named locals
 * el_chavo::DrawLine2d
 * PDB type: void el_chavo::(int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::DrawLine2d(
    int x1, int y1, int x2, int y2, unsigned long color)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)color;
}

/* 0x113430, 108 bytes, global, 10 named locals
 * el_chavo::DrawLine
 * PDB type: void el_chavo::(int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::DrawLine(
    int x1,
    int y1,
    int z1,
    int x2,
    int y2,
    int z2,
    unsigned long color)
{
    _svector points[2];
    int screen_xy[2];

    (void)color;
    points[0] = {
        static_cast<int16_t>(x1),
        static_cast<int16_t>(y1),
        static_cast<int16_t>(z1),
        0};
    points[1] = {
        static_cast<int16_t>(x2),
        static_cast<int16_t>(y2),
        static_cast<int16_t>(z2),
        0};
    (void)TransformPoints(points, screen_xy, 2);
}

/* 0x1134A0, 212 bytes, global, 10 named locals
 * el_chavo::DrawSphere
 * PDB type: void el_chavo::(int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::DrawSphere(
    int x, int y, int z, int radius, unsigned long color)
{
    _svector point = {
        static_cast<int16_t>(x),
        static_cast<int16_t>(y),
        static_cast<int16_t>(z),
        0};
    int screen_xy;
    VECTOR camera_location;
    _svector camera_direction;

    (void)radius;
    (void)color;
    if (m_pFramework->m_pDevice != nullptr && m_pFramework != nullptr) {
        PushMatrix();
        camera_gGetLocation(&camera_location);
        (void)TransformPoints(&point, &screen_xy, 1);
        (void)normalize(
            camera_location.vx - x,
            camera_location.vy - y,
            camera_location.vz - z,
            &camera_direction);
        PopMatrix();
        for (int angle = 0; angle < 17; ++angle) {
            (void)rsin(angle << 8);
            (void)rcos(angle << 8);
        }
    }
}

/* 0x113580, 4125 bytes, global, 78 named locals
 * el_chavo::DrawUITextUTF16
 * PDB type: void el_chavo::(unsigned short*,...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
static int jpb_whook_text_controller_icon(char16_t character)
{
    switch (character) {
    case u'\u2021':
        return 3;
    case u'\u20ac':
        return 2;
    case u'\u0192':
        return 4;
    case u'\u2020':
        return 5;
    case u'\u0160':
        return 7;
    case u'\u017d':
        return 9;
    default:
        return -1;
    }
}

static int jpb_whook_text_tag_icon(char16_t character, int *alpha)
{
    *alpha = 255;
    switch (character) {
    case u'A':
        return 3;
    case u'B':
        return 2;
    case u'F':
        return 7;
    case u'X':
        return 4;
    case u'Y':
        return 5;
    case u'a':
        *alpha = 128;
        return 3;
    case u'b':
        *alpha = 128;
        return 2;
    case u'f':
        *alpha = 128;
        return 7;
    case u'x':
        *alpha = 128;
        return 4;
    case u'y':
        *alpha = 128;
        return 5;
    default:
        return -1;
    }
}

static void jpb_whook_draw_controller_icon(
    int icon,
    int x,
    int y,
    int alpha,
    SCREENRECT scissor,
    bool depth_enabled,
    float depth)
{
    if (depth_enabled) {
        newDrawControllerIconDepth(
            icon, 0.35f, x, y, alpha, 0, scissor, depth);
    } else {
        newDrawControllerIcon(
            icon, 0.35f, x, y, alpha, 0, scissor);
    }
}

static SpriteDraw jpb_whook_text_2d_sprite(
    Texture *texture,
    const Glyph &glyph,
    const GlyphEntry &entry,
    int maximum_height,
    int x,
    int y,
    CVECTOR color,
    bool clipping,
    const RECT &scissor,
    bool depth_enabled,
    float depth)
{
    SpriteDraw draw = {};
    draw.Texture = texture;
    draw.SrcRect = RECT{
        static_cast<LONG>(entry.Left),
        static_cast<LONG>(entry.Top),
        static_cast<LONG>(entry.Right),
        static_cast<LONG>(entry.Bottom)};
    draw.DestRect.left = x;
    draw.DestRect.top = maximum_height - glyph.Metrics.maxy + y;
    draw.DestRect.right =
        static_cast<LONG>(entry.Right - entry.Left) + x;
    draw.DestRect.bottom =
        static_cast<LONG>(entry.Bottom - entry.Top) + draw.DestRect.top;
    draw.Color.f[0] = static_cast<float>(color.r) / 255.0f;
    draw.Color.f[1] = static_cast<float>(color.g) / 255.0f;
    draw.Color.f[2] = static_cast<float>(color.b) / 255.0f;
    draw.Color.f[3] = static_cast<float>(color.cd) / 255.0f;
    draw.Effects = DirectX::DX12::SpriteEffects_None;
    draw.Origin = DirectX::XMFLOAT2(0.0f, 0.0f);
    draw.Rotation = 0.0f;
    draw.LayerDepth = depth_enabled ? depth : 0.0f;
    if (clipping) {
        draw.ScissorRect = scissor;
    }
    draw.SamplerType = TEXTURESAMPLER_LINEARCLAMP;
    return draw;
}

static void jpb_whook_draw_ui_text_utf16(
    el_chavo *application,
    unsigned short *utf_text,
    SCREENRECT destination,
    int font_style,
    int size,
    CVECTOR color,
    bool depth_enabled,
    float depth)
{
    std::u16string text(reinterpret_cast<char16_t *>(utf_text));
    char *font_file = nullptr;
    int maximum_height = 0;

    for (char16_t character : text) {
        font_file = getFontFile(font_style);
        std::optional<std::pair<Glyph, GlyphEntry>> glyph =
            application->m_fontAtlas->AddGlyph(
                font_file,
                static_cast<unsigned short>(character),
                size);
        if (!glyph.has_value()) {
            font_file = getDefaultFontFile(font_style);
            glyph = application->m_fontAtlas->AddGlyph(
                font_file,
                static_cast<unsigned short>(character),
                size);
        }
        if (glyph.has_value()) {
            maximum_height = (std::max)(
                maximum_height, glyph->first.Metrics.maxy);
        }
    }

    int cursor_x = destination.left;
    int cursor_y = destination.top;
    for (int index = 0;
         index < static_cast<int>(text.size());
         ++index) {
        SCREENRECT scissor = {-1, -1, -1, -1};
        if (textClipping != 0) {
            scissor.left = static_cast<int>(textclipRect[0]);
            scissor.top = static_cast<int>(textclipRect[1]);
            scissor.right = static_cast<int>(textclipRect[2]);
            scissor.bottom = static_cast<int>(textclipRect[3]);
        }

        if (text[index] == u'\n') {
            cursor_y = static_cast<int>(
                static_cast<float>(maximum_height) * 1.5f +
                static_cast<float>(cursor_y));
            cursor_x = destination.left;
            continue;
        }

        const int direct_icon =
            jpb_whook_text_controller_icon(text[index]);
        if (direct_icon >= 0) {
            if (index < 3) {
                text.insert(0, u" ");
                --index;
            } else {
                jpb_whook_draw_controller_icon(
                    direct_icon,
                    cursor_x,
                    static_cast<int>(
                        static_cast<float>(maximum_height) * 0.5f +
                        static_cast<float>(cursor_y)),
                    255,
                    scissor,
                    depth_enabled,
                    depth);
                text[index] = u' ';
            }
            continue;
        }

        if (index + 2 < static_cast<int>(text.size()) &&
            text[index] == u'<' && text[index + 2] == u'>') {
            int alpha;
            const int tag_icon =
                jpb_whook_text_tag_icon(text[index + 1], &alpha);
            if (tag_icon >= 0) {
                jpb_whook_draw_controller_icon(
                    tag_icon,
                    cursor_x,
                    static_cast<int>(
                        static_cast<float>(maximum_height) * 0.5f +
                        static_cast<float>(cursor_y)),
                    alpha,
                    scissor,
                    depth_enabled,
                    depth);
            }
            text[index] = u' ';
            text[index + 1] = u' ';
            text[index + 2] = u' ';
            continue;
        }

        std::optional<std::pair<Glyph, GlyphEntry>> glyph =
            application->m_fontAtlas->AddGlyph(
                font_file,
                static_cast<unsigned short>(text[index]),
                size);
        if (!glyph.has_value()) {
            continue;
        }

        const Glyph &glyph_data = glyph->first;
        const GlyphEntry &entry = glyph->second;
        RECT sprite_scissor = {};
        if (textClipping != 0) {
            sprite_scissor = {
                static_cast<LONG>(textclipRect[0]),
                static_cast<LONG>(textclipRect[1]),
                static_cast<LONG>(textclipRect[2]),
                static_cast<LONG>(textclipRect[3])};
        }
        const SpriteDraw draw = jpb_whook_text_2d_sprite(
            static_cast<Texture *>(application->m_fontAtlas->GetTexture()),
            glyph_data,
            entry,
            maximum_height,
            cursor_x,
            cursor_y,
            color,
            textClipping != 0,
            sprite_scissor,
            depth_enabled,
            depth);
        application->DrawTexture(draw);
        cursor_x +=
            glyph_data.Metrics.maxx - glyph_data.Metrics.minx;
    }
}

void el_chavo::DrawUITextUTF16(
    unsigned short *utf_text,
    SCREENRECT destination,
    int font_style,
    int size,
    CVECTOR color)
{
    jpb_whook_draw_ui_text_utf16(
        this,
        utf_text,
        destination,
        font_style,
        size,
        color,
        false,
        0.0f);
}

/* 0x1145A0, 4261 bytes, global, 79 named locals
 * el_chavo::DrawUITextUTF16Depth
 * PDB type: void el_chavo::(unsigned short*,...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::DrawUITextUTF16Depth(
    unsigned short *utf_text,
    SCREENRECT destination,
    int font_style,
    int size,
    CVECTOR color,
    float depth)
{
    jpb_whook_draw_ui_text_utf16(
        this,
        utf_text,
        destination,
        font_style,
        size,
        color,
        true,
        depth);
}

/* 0x115650, 2103 bytes, global, 55 named locals
 * el_chavo::DrawUITextUTF16_3D
 * PDB type: void el_chavo::(unsigned short*,...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::DrawUITextUTF16_3D(
    unsigned short *utf_text,
    float x,
    float y,
    float z,
    int font_style,
    int size,
    unsigned long color)
{
    std::u16string text(reinterpret_cast<char16_t *>(utf_text));
    const float scale = static_cast<float>(size) / 50.0f;
    const float atlas_width =
        static_cast<float>(m_fontAtlas->GetTexture()->GetWidth());
    const float atlas_height =
        static_cast<float>(m_fontAtlas->GetTexture()->GetHeight());
    char *font_file = nullptr;
    float maximum_height = 0.0f;

    for (char16_t character : text) {
        font_file = getFontFile(font_style);
        std::optional<std::pair<Glyph, GlyphEntry>> glyph =
            m_fontAtlas->AddGlyph(
                font_file,
                static_cast<unsigned short>(character),
                50);

        if (!glyph.has_value()) {
            font_file = getDefaultFontFile(font_style);
            glyph = m_fontAtlas->AddGlyph(
                font_file,
                static_cast<unsigned short>(character),
                50);
        }
        if (glyph.has_value()) {
            maximum_height = (std::max)(
                maximum_height,
                static_cast<float>(glyph->first.Metrics.maxy));
        }
    }
    maximum_height *= scale;

    float cursor_x = x;
    float cursor_y = y;
    for (char16_t character : text) {
        if (character == u'\n') {
            cursor_y = jpb_whook_text_3d_line_advance(
                cursor_y, maximum_height);
            cursor_x = x;
            continue;
        }

        std::optional<std::pair<Glyph, GlyphEntry>> glyph =
            m_fontAtlas->AddGlyph(
                font_file,
                static_cast<unsigned short>(character),
                50);
        if (!glyph.has_value()) {
            continue;
        }

        const GlyphEntry &entry = glyph->second;
        JPBScreenPolyVertex vertices[4];
        const float next_x = jpb_whook_text_3d_quad(
            entry,
            atlas_width,
            atlas_height,
            scale,
            maximum_height,
            cursor_x,
            cursor_y,
            z,
            static_cast<std::uint32_t>(color),
            vertices);

        _StartPoly(4, atlasHandle);
        for (int vertex = 0; vertex < 4; ++vertex) {
            _SetVert(
                vertex,
                vertices[vertex].x,
                vertices[vertex].y,
                vertices[vertex].z,
                vertices[vertex].argb,
                vertices[vertex].tu,
                vertices[vertex].tv);
        }
        _NoScaleEndPoly();
        cursor_x = next_x;
    }
}

/* 0x115E90, 5208 bytes, global, 50 named locals
 * el_chavo::EndPoly
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1172F0, 28 bytes, global, 1 named locals
 * el_chavo::FinalCleanup
 * PDB type: HRESULT el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
HRESULT el_chavo::FinalCleanup()
{
    FreeFont();
    ::operator delete[](g_resolutions);
    return S_OK;
}

/* 0x117310, 560 bytes, global, 1 named locals
 * GetAchNameFromIndex
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" char *GetAchNameFromIndex(int index)
{
    static const char *const achievement_names[] = {
        "JPB_Trophy_001", "JPB_Trophy_002", "JPB_Trophy_003",
        "JPB_Trophy_004", "JPB_Trophy_005", "JPB_Trophy_006",
        "JPB_Trophy_007", "JPB_Trophy_008", "JPB_Trophy_009",
        "JPB_Trophy_010", "JPB_Trophy_011", "JPB_Trophy_012",
        "JPB_Trophy_013", "JPB_Trophy_014", "JPB_Trophy_015",
        "JPB_Trophy_016", "JPB_Trophy_017", "JPB_Trophy_018",
        "JPB_Trophy_019", "JPB_Trophy_020", "JPB_Trophy_021",
        "JPB_Trophy_022", "JPB_Trophy_023", "JPB_Trophy_024",
        "JPB_Trophy_025", "JPB_Trophy_026", "JPB_Trophy_027",
        "JPB_Trophy_028", "JPB_Trophy_029", "JPB_Trophy_030",
        "JPB_Trophy_031", "JPB_Trophy_032", "JPB_Trophy_033",
        "JPB_Trophy_034", "JPB_Trophy_035", "JPB_Trophy_036",
        "JPB_Trophy_037", "JPB_Trophy_038", "JPB_Trophy_039",
        "JPB_Trophy_040", "JPB_Trophy_041", "JPB_Trophy_042",
        "JPB_Trophy_043"
    };

    if (index < 1 || index > 43) {
        return const_cast<char *>("");
    }
    return const_cast<char *>(achievement_names[index - 1]);
}

/* 0x117540, 99 bytes, global, 2 named locals
 * el_chavo::InitDeviceObjects
 * PDB type: HRESULT el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
HRESULT el_chavo::InitDeviceObjects()
{
    defaulttexture = CreateEmptyTexture(
        const_cast<char *>(resource_getPath(
            "o_default.tga", JPB_RESOURCE_DEFAULT)),
        0x100,
        0x100,
        0,
        0);
    currenttexture = defaulttexture;
    currenttexture->Invalidate();
    defaulttexture = nullptr;
    (void)InitInput();
    return S_OK;
}

/* 0x1175B0, 6700 bytes, global, 124 named locals
 * el_chavo::InitFBXLevelData
 * PDB type: void el_chavo::(ufbx_scene*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x118FE0, 3 bytes, global, 3 named locals
 * el_chavo::InitFBXTextureData
 * PDB type: void el_chavo::(char*, int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::InitFBXTextureData(char *filename, int texture_index)
{
    (void)filename;
    (void)texture_index;
}

/* 0x118FF0, 65 bytes, global, 1 named locals
 * el_chavo::InitTransPolys
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::InitTransPolys()
{
    std::memset(otag, 0, sizeof(otag));
    currentpoly = trans_polygon;
    currentpolyistrans = 0;
    numtranspolys = 0;
    oldtexture = nullptr;
}

/* 0x119040, 42 bytes, global, 2 named locals
 * IsNullTerminated
 * PDB type: bool (const char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" bool IsNullTerminated(const char *text)
{
    size_t length;

    if (text == nullptr) {
        return false;
    }
    length = std::strlen(text);
    return length != 0 && text[length - 1] == '\0';
}

/* 0x119070, 289 bytes, global, 9 named locals
 * el_chavo::LoadTexture
 * PDB type: Texture* el_chavo::(char*, unsig...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
Texture *el_chavo::LoadTexture(
    char *filename, unsigned long option, int tpf, int type)
{
    char *modified_filename = ModifyFilename(filename);

    return CreateTextureFromFile(
        modified_filename,
        0,
        0,
        option & 0x02000000UL,
        tpf,
        type,
        m_pFramework);
}

/* 0x1191A0, 220 bytes, global, 5 named locals
 * ModifyFilename
 * PDB type: char* (const char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" char *ModifyFilename(const char *filename)
{
    size_t length = std::strlen(filename);
    char *modified = new char[length + 4];
    char *extension;

    (void)strncpy_s(modified, length + 1, filename, length);
    if (std::strstr(modified, "sgi") != nullptr) {
        extension = std::strrchr(modified, '.');
        if (extension != nullptr) {
            std::memcpy(extension + 1, "tim", 4);
        }
    }
    if (std::strstr(modified, "pvr") != nullptr) {
        extension = std::strrchr(modified, '.');
        if (extension != nullptr) {
            std::memcpy(extension + 1, "tga", 4);
        }
    }
    if (modified[length + 3] != '\0') {
        modified[length + 3] = '\0';
    }
    return modified;
}

/* 0x119280, 5529 bytes, global, 65 named locals
 * el_chavo::NoScaleEndPoly
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x11A820, 56 bytes, global, 2 named locals
 * el_chavo::OnKeyUp
 * PDB type: void el_chavo::(int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::OnKeyUp(int key)
{
    if (key == VK_ESCAPE && GetAsyncKeyState(VK_SHIFT) < 0) {
        PostMessageA(m_hWnd, WM_CLOSE, 0, 0);
    }
}

/* 0x11A860, 3 bytes, global, 1 named locals
 * el_chavo::OneTimeSceneInit
 * PDB type: HRESULT el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
HRESULT el_chavo::OneTimeSceneInit()
{
    return S_OK;
}

/* 0x11A870, 38 bytes, global, 1 named locals
 * el_chavo::PlotTransPolys
 * PDB type: void el_chavo::()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::PlotTransPolys()
{
    ID3D12Device *device = m_pFramework->m_pDevice;

    if (device != nullptr && m_pFramework != nullptr) {
        using LegacyPlotFunction = void (*)(ID3D12Device *, unsigned);
        void **vtable = *reinterpret_cast<void ***>(device);

        alphapolys = 0;
        reinterpret_cast<LegacyPlotFunction>(vtable[15])(device, 0);
    }
}

/* 0x11A8A0, 99 bytes, global, 6 named locals
 * SetFilenameExtension
 * PDB type: void (char*, char*, char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void SetFilenameExtension(
    char *source, char *destination, char *extension)
{
    char *last_dot = nullptr;
    char *extension_source = extension[0] == '.'
        ? extension + 1
        : extension;

    while (*source != '\0') {
        if (*source == '.') {
            last_dot = destination;
        }
        *destination++ = *source++;
    }
    char *extension_destination = last_dot != nullptr
        ? last_dot + 1
        : source;
    do {
        *extension_destination++ = *extension_source;
    } while (*extension_source++ != '\0');
}

/* 0x11A910, 399 bytes, global, 8 named locals
 * el_chavo::SetVert
 * PDB type: void el_chavo::(int, float, floa...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::SetVert(
    int vertex,
    float x,
    float y,
    float z,
    unsigned long argb,
    float tu,
    float tv)
{
    if (near_z == 0) {
        unsigned clip_code = 0;
        DXGI_SWAP_CHAIN_DESC1 *swap_chain = m_pFramework->m_pSwapChainDesc;
        D3DTLVERTEX *destination;

        if (static_cast<float>(swap_chain->Width) < x) {
            clip_code |= 4;
        }
        if (x < 0.0f) {
            clip_code |= 8;
        }
        if (y < 0.0f) {
            clip_code |= 2;
        }
        if (static_cast<float>(swap_chain->Height) < y) {
            clip_code |= 1;
        }
        clip[vertex] = static_cast<int>(clip_code);

        destination = currentpolyistrans != 0
            ? &currentpoly->vert[vertex]
            : &polyarray[vertex];
        destination->sx = x;
        destination->sy = y;
        destination->sz = z;
        destination->color = argb;
        destination->specular = 0;
        destination->rhw = 1.0f / z;
        destination->tu = tu;
        destination->tv = tv;
        if (currentpolyistrans != 0) {
            total_z += z;
        }
    }
}

/* 0x11AAA0, 187 bytes, global, 3 named locals
 * el_chavo::StartPoly
 * PDB type: void el_chavo::(int, _Material*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::StartPoly(int vertex_count, _Material *material)
{
    near_z = 0;
    if (material != nullptr && material->texture != nullptr) {
        currenttexture = static_cast<Texture *>(material->texture);
        currenttexture->m_dwFlags = material->flags;
        nv = vertex_count;
        if (currenttexture->m_type == 0) {
            currentpolyistrans = 0;
        } else if ((currenttexture->m_type == 1 ||
                    currenttexture->m_type == 2) &&
                   numtranspolys < 4096) {
            currentpolyistrans = 1;
            total_z = 0.0f;
            currentpoly->texture = currenttexture;
            currentpoly->nvert = nv;
            oldtexture = currenttexture;
            return;
        }
        oldtexture = currenttexture;
    }
}

/* 0x11AB60, 388 bytes, global, 10 named locals
 * UpdateValidResolutions
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void UpdateValidResolutions(void)
{
    DEVMODEA mode;
    DEVMODEA current_mode;
    RESOLUTION resolution_buffer[256];
    HDC device_context = GetDC(nullptr);
    int mode_index = 0;

    g_resolutionsCount = 0;
    mode.dmSize = sizeof(mode);
    current_mode.dmSize = sizeof(current_mode);
    EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &current_mode);

    while (EnumDisplaySettingsA(nullptr, mode_index, &mode) != FALSE) {
        if (mode.dmPelsWidth >= current_mode.dmPelsWidth &&
            mode.dmPelsHeight >= current_mode.dmPelsHeight) {
            int resolution_index = 0;

            while (resolution_index < g_resolutionsCount &&
                   (resolution_buffer[resolution_index].width !=
                        static_cast<int32_t>(mode.dmPelsWidth) ||
                    resolution_buffer[resolution_index].height !=
                        static_cast<int32_t>(mode.dmPelsHeight))) {
                ++resolution_index;
            }
            if (resolution_index == g_resolutionsCount) {
                resolution_buffer[g_resolutionsCount].width =
                    static_cast<int32_t>(mode.dmPelsWidth);
                resolution_buffer[g_resolutionsCount].height =
                    static_cast<int32_t>(mode.dmPelsHeight);
                ++g_resolutionsCount;
            }
        }
        ++mode_index;
    }

    std::sort(
        resolution_buffer,
        resolution_buffer + g_resolutionsCount,
        resolutionComparison);
    std::memcpy(g_resolutions, resolution_buffer, sizeof(g_resolutions));
    ReleaseDC(nullptr, device_context);
}

/* 0x11ACF0, 138 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_backreference
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11AD80, 10 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_bol
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11AD90, 287 bytes, global, 4 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_char
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11AEB0, 177 bytes, global, 5 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_char_to_array
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11AF70, 131 bytes, global, 4 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_char_to_bitmap
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B000, 131 bytes, global, 5 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_char_to_class
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B090, 146 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_class
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B130, 27 bytes, global, 5 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_coll
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B150, 10 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_dot
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B160, 230 bytes, global, 6 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_elts
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B250, 10 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_eol
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B260, 394 bytes, global, 16 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_equiv
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B3F0, 216 bytes, global, 6 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_named_class
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B4D0, 404 bytes, global, 10 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_range
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B670, 809 bytes, global, 15 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_rep
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11B9A0, 130 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_str_node
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11BA30, 10 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Add_wbound
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11BA40, 1587 bytes, global, 15 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Alternative
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C080, 384 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_AtomEscape
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C200, 24 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Beg_expr
 * PDB type: bool std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C220, 53 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Beg_expr
 * PDB type: bool std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C260, 209 bytes, global, 5 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Begin_assert_group
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C340, 141 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Begin_capture_group
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C3D0, 10 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Begin_group
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C3E0, 242 bytes, global, 5 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Begin_if
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C4E0, 168 bytes, global, 3 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Better_match
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C590, 138 bytes, global, 6 named locals
 * std::vector<Vertex,std::allocator<Vertex> >::_Buy_nonzero
 * PDB type: void std::vector<Vertex,std::all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x11C620, 135 bytes, global, 6 named locals
 * std::vector<unsigned int,std::allocator<unsigned int> >::_Buy_raw
 * PDB type: void std::vector<unsigned int,st...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x11C6B0, 134 bytes, global, 6 named locals
 * std::vector<std::_Tgt_state_t<char const *>::_Grp_t,std::allocator<std::_Tgt_state_t<char const *>::_Grp_t> >::_Buy_raw
 * PDB type: void std::vector<std::_Tgt_state...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x11C740, 224 bytes, global, 4 named locals
 * std::_Calculate_loop_simplicity
 * PDB type: void (std::_Node_base*, std::_No...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C820, 234 bytes, global, 8 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Char_to_elts
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11C910, 270 bytes, global, 4 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_CharacterClass
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11CA20, 168 bytes, global, 4 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_CharacterClassEscape
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11CAD0, 1088 bytes, global, 9 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_CharacterEscape
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11CF10, 522 bytes, global, 3 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_ClassAtom
 * PDB type: std::_Prs_ret std::_Parser<char ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D120, 132 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_ClassEscape
 * PDB type: std::_Prs_ret std::_Parser<char ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D1B0, 348 bytes, global, 4 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_ClassRanges
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D310, 145 bytes, global, 5 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Compile
 * PDB type: std::_Root_node* std::_Parser<ch...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D3B0, 240 bytes, global, 6 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_DecimalDigits
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D4A0, 604 bytes, global, 11 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Disjunction
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D700, 230 bytes, global, 5 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_assert_group
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D7F0, 123 bytes, global, 4 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_capture_group
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11D870, 386 bytes, global, 12 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_class
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11DA00, 279 bytes, global, 7 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_digits
 * PDB type: int std::_Parser<char const *,ch...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11DB20, 414 bytes, global, 8 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_ex_class
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11DCC0, 78 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_ffn
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11DD10, 35 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_ffnx
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11DD40, 1069 bytes, global, 33 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_if
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E170, 70 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Do_noncapture_group
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E1C0, 1063 bytes, global, 34 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep0
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E5F0, 759 bytes, global, 25 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E8F0, 172 bytes, global, 6 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Else_if
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E9A0, 36 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_End_assert_group
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11E9D0, 177 bytes, global, 3 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_End_group
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EA90, 28 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_End_pattern
 * PDB type: std::_Root_node* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EAB0, 12 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Error
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EAC0, 106 bytes, global, 4 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Expect
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EB30, 861 bytes, global, 40 named locals
 * std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Forced_rehash
 * PDB type: void std::_Hash<std::_Umap_trait...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xhash
 */

/* 0x11EE90, 4 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Get_bmax
 * PDB type: unsigned std::_Builder<char cons...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EEA0, 7 bytes, global, 1 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Get_ncap
 * PDB type: unsigned std::_Matcher<char cons...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EEB0, 4 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Get_tmax
 * PDB type: unsigned std::_Builder<char cons...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11EEC0, 516 bytes, global, 6 named locals
 * std::collate<char>::_Getcat
 * PDB type: unsigned __int64 std::collate<ch...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\locale
 */

/* 0x11F0D0, 5 bytes, global, 1 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Getmark
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F0E0, 239 bytes, global, 6 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_HexDigits
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F1D0, 392 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_IdentityEscape
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F360, 25 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Insert_node
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F380, 603 bytes, global, 13 named locals
 * std::_Tree_val<std::_Tree_simple_types<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::_Insert_node
 * PDB type: std::_Tree_node<std::pair<std::b...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xtree
 */

/* 0x11F5E0, 553 bytes, global, 9 named locals
 * std::vector<bool,std::allocator<bool> >::_Insert_x
 * PDB type: unsigned __int64 std::vector<boo...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x11F810, 288 bytes, global, 1 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_IsIdentityEscape
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F930, 55 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Is_esc
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F970, 115 bytes, global, 1 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Is_wbound
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11F9F0, 53 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Link_node
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11FA30, 17 bytes, global, 1 named locals
 * std::_Make_ec
 * PDB type: std::error_code (__std_win_error...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x11FA50, 9 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Mark_final
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x11FA60, 1576 bytes, global, 23 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Match_pat
 * PDB type: bool std::_Matcher<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120090, 9 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Negate
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1200A0, 134 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_New_node
 * PDB type: std::_Node_base* std::_Builder<c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120130, 80 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Next
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120180, 212 bytes, global, 5 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_OctalDigits
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120260, 743 bytes, global, 28 named locals
 * std::filesystem::filesystem_error::_Pretty_message
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x120550, 411 bytes, global, 7 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Quantifier
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1206F0, 191 bytes, global, 8 named locals
 * std::match_results<char const *,std::allocator<std::sub_match<char const *> > >::_Resize
 * PDB type: void std::match_results<char con...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1207B0, 8 bytes, global, 2 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Setlong
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x1207C0, 736 bytes, global, 16 named locals
 * std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Skip
 * PDB type: const char* std::_Matcher<char c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120AA0, 85 bytes, global, 3 named locals
 * std::filesystem::_Throw_fs_error
 * PDB type: void (const char*, const std::er...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x120B00, 102 bytes, global, 3 named locals
 * std::filesystem::_Throw_fs_error
 * PDB type: void (const char*, __std_win_err...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x120B70, 57 bytes, global, 1 named locals
 * std::_Throw_system_error
 * PDB type: void (const std::errc)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x120BB0, 57 bytes, global, 1 named locals
 * std::_Throw_system_error_from_std_win_error
 * PDB type: void (const __std_win_error)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x120BF0, 98 bytes, global, 3 named locals
 * std::_Builder<char const *,char,std::regex_traits<char> >::_Tidy
 * PDB type: void std::_Builder<char const *,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120C60, 125 bytes, global, 3 named locals
 * std::basic_regex<char,std::regex_traits<char> >::_Tidy
 * PDB type: void std::basic_regex<char,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x120CE0, 118 bytes, global, 5 named locals
 * std::vector<Vertex,std::allocator<Vertex> >::_Tidy
 * PDB type: void std::vector<Vertex,std::all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x120D60, 97 bytes, global, 5 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::_Tidy_deallocate
 * PDB type: void std::basic_string<char16_t,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x120DD0, 97 bytes, global, 5 named locals
 * std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >::_Tidy_deallocate
 * PDB type: void std::basic_string<wchar_t,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x120E40, 516 bytes, global, 2 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Trans
 * PDB type: void std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x121050, 108 bytes, global, 3 named locals
 * std::vector<bool,std::allocator<bool> >::_Trim
 * PDB type: void std::vector<bool,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1210C0, 348 bytes, global, 6 named locals
 * std::_Parser<char const *,char,std::regex_traits<char> >::_Wrapped_disjunction
 * PDB type: bool std::_Parser<char const *,c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x121220, 17 bytes, global, 0 named locals
 * std::vector<bool,std::allocator<bool> >::_Xlen
 * PDB type: void std::vector<bool,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x121240, 17 bytes, global, 0 named locals
 * std::vector<char,std::allocator<char> >::_Xlength
 * PDB type: void std::vector<char,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x121260, 17 bytes, global, 0 named locals
 * std::vector<unsigned short,std::allocator<unsigned short> >::_Xlength
 * PDB type: void std::vector<unsigned short,...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x121280, 17 bytes, global, 0 named locals
 * std::vector<CD3DApplication::FBX_MESH *,std::allocator<CD3DApplication::FBX_MESH *> >::_Xlength
 * PDB type: void std::vector<CD3DApplication...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1212A0, 17 bytes, global, 0 named locals
 * std::vector<CD3DApplication::SubMeshSet,std::allocator<CD3DApplication::SubMeshSet> >::_Xlength
 * PDB type: void std::vector<CD3DApplication...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1212C0, 17 bytes, global, 0 named locals
 * std::vector<std::_Tgt_state_t<char const *>::_Grp_t,std::allocator<std::_Tgt_state_t<char const *>::_Grp_t> >::_Xlength
 * PDB type: void std::vector<std::_Tgt_state...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1212E0, 17 bytes, global, 0 named locals
 * std::vector<std::_Loop_vals_t,std::allocator<std::_Loop_vals_t> >::_Xlength
 * PDB type: void std::vector<std::_Loop_vals...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x121300, 17 bytes, global, 0 named locals
 * std::vector<std::sub_match<char const *>,std::allocator<std::sub_match<char const *> > >::_Xlength
 * PDB type: void std::vector<std::sub_match<...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x121320, 17 bytes, global, 0 named locals
 * std::_String_val<std::_Simple_types<char16_t> >::_Xran
 * PDB type: void std::_String_val<std::_Simp...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x121340, 62 bytes, global, 7 named locals
 * std::allocator<unsigned short>::deallocate
 * PDB type: void std::allocator<unsigned sho...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x121380, 66 bytes, global, 7 named locals
 * std::allocator<CD3DApplication::FBX_MESH *>::deallocate
 * PDB type: void std::allocator<CD3DApplicat...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x1213D0, 62 bytes, global, 7 named locals
 * std::allocator<CD3DApplication::SubMeshSet>::deallocate
 * PDB type: void std::allocator<CD3DApplicat...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x121410, 65 bytes, global, 7 named locals
 * std::allocator<std::_Tgt_state_t<char const *>::_Grp_t>::deallocate
 * PDB type: void std::allocator<std::_Tgt_st...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x121460, 65 bytes, global, 7 named locals
 * std::allocator<std::_Loop_vals_t>::deallocate
 * PDB type: void std::allocator<std::_Loop_v...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x1214B0, 66 bytes, global, 7 named locals
 * std::allocator<std::sub_match<char const *> >::deallocate
 * PDB type: void std::allocator<std::sub_mat...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xmemory
 */

/* 0x121500, 114 bytes, global, 3 named locals
 * std::_System_error_category::default_error_condition
 * PDB type: std::error_condition std::_Syste...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121580, 73 bytes, global, 6 named locals
 * std::collate<char>::do_compare
 * PDB type: int std::collate<char>::(const c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\locale
 */

/* 0x1215D0, 53 bytes, global, 5 named locals
 * std::collate<char>::do_hash
 * PDB type: long std::collate<char>::(const ...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\locale
 */

/* 0x121610, 415 bytes, global, 12 named locals
 * std::collate<char>::do_transform
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\locale
 */

/* 0x1217B0, 500 bytes, global, 6 named locals
 * std::vector<bool,std::allocator<bool> >::erase
 * PDB type: std::_Vb_iterator<std::_Wrap_all...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x1219B0, 295 bytes, global, 11 named locals
 * std::basic_string<char16_t,std::char_traits<char16_t>,std::allocator<char16_t> >::insert
 * PDB type: std::basic_string<char16_t,std::...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x121AE0, 17 bytes, global, 1 named locals
 * std::make_error_code
 * PDB type: std::error_code (std::errc)
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121B00, 78 bytes, global, 3 named locals
 * std::_Generic_error_category::message
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121B50, 151 bytes, global, 4 named locals
 * std::_System_error_category::message
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121BF0, 8 bytes, global, 1 named locals
 * std::_Generic_error_category::name
 * PDB type: const char* std::_Generic_error_...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121C00, 8 bytes, global, 1 named locals
 * std::_System_error_category::name
 * PDB type: const char* std::_System_error_c...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\system_error
 */

/* 0x121C10, 366 bytes, global, 7 named locals
 * std::locale::name
 * PDB type: std::basic_string<char,std::char...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xlocale
 */

/* 0x121D80, 216 bytes, global, 5 named locals
 * std::chrono::steady_clock::now
 * PDB type: std::chrono::time_point<std::chr...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\__msvc_chrono.hpp
 */

/* 0x121E60, 189 bytes, global, 6 named locals
 * el_chavo::renderLoadProgress
 * PDB type: void el_chavo::(int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::renderLoadProgress(int endframe)
{
    (void)endframe;
    (void)StartRender();
    (void)RenderUI();
    if (SUCCEEDED(EndRender())) {
        HRESULT result = m_pFramework->Present();

        if (FAILED(result) &&
            m_pFramework->m_pDevice->GetDeviceRemovedReason() ==
                DXGI_ERROR_DEVICE_REMOVED) {
            RECT client_rect;

            (void)GetClientRect(m_hWnd, &client_rect);
            WaitForGpu();
            (void)m_pFramework->ResizeResources(
                static_cast<UINT>(client_rect.right - client_rect.left),
                static_cast<UINT>(client_rect.bottom - client_rect.top));
        }
        MoveToNextFrame();
    }
}

/* 0x121F20, 852 bytes, global, 19 named locals
 * el_chavo::renderVideoFrame
 * PDB type: void el_chavo::(SDL_Surface*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void el_chavo::renderVideoFrame(SDL_Surface *surface)
{
    (void)StartRender();
    m_pVidTex->UpdateTexture(
        surface->pixels,
        static_cast<std::uint64_t>(surface->w * surface->h * 4));

    SpriteDraw frame = {};
    frame.Texture = static_cast<Texture *>(m_pVidTex);
    frame.DestRect = jpb_whook_video_destination(
        OptionStruct.ScreenWidth, OptionStruct.ScreenHeight);
    frame.Color = DirectX::XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f};

    m_spriteBatch->SetViewport(m_pFramework->m_Viewport);
    ID3D12DescriptorHeap *descriptor_heaps[2] = {
        m_pFramework->m_pMainDescriptorHeap,
        m_states->Heap(),
    };
    m_pFramework->m_pCommandList->SetDescriptorHeaps(
        2, descriptor_heaps);
    DirectX::DescriptorHeap descriptor_heap(
        m_pFramework->m_pMainDescriptorHeap);
    m_spriteBatch->Begin(
        m_pFramework->m_pCommandList,
        m_states->LinearClamp(),
        DirectX::DX12::SpriteSortMode_Deferred,
        DirectX::XMMatrixIdentity());

    const DirectX::XMUINT2 texture_size(
        frame.Texture->m_dwWidth,
        frame.Texture->m_dwHeight);
    const RECT *source = frame.SrcRect.has_value()
        ? &frame.SrcRect.value()
        : nullptr;
    m_spriteBatch->Draw(
        descriptor_heap.GetGpuHandle(frame.Texture->m_nIndex),
        texture_size,
        frame.DestRect,
        source,
        frame.Color,
        frame.Rotation,
        frame.Origin,
        frame.Effects,
        frame.LayerDepth);
    m_spriteBatch->End();

    if (SUCCEEDED(EndRender())) {
        const HRESULT result = m_pFramework->Present();

        if (FAILED(result) &&
            m_pFramework->m_pDevice->GetDeviceRemovedReason() ==
                DXGI_ERROR_DEVICE_REMOVED) {
            RECT client_rect;

            (void)GetClientRect(m_hWnd, &client_rect);
            WaitForGpu();
            (void)m_pFramework->ResizeResources(
                static_cast<UINT>(client_rect.right - client_rect.left),
                static_cast<UINT>(client_rect.bottom - client_rect.top));
        }
        MoveToNextFrame();
    }
}

/* 0x122280, 147 bytes, global, 7 named locals
 * std::basic_string<char,std::char_traits<char>,std::allocator<char> >::resize
 * PDB type: void std::basic_string<char,std:...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x122320, 137 bytes, global, 8 named locals
 * std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >::resize
 * PDB type: void std::basic_string<wchar_t,s...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\xstring
 */

/* 0x1223B0, 642 bytes, global, 16 named locals
 * std::vector<bool,std::allocator<bool> >::resize
 * PDB type: void std::vector<bool,std::alloc...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x122640, 16 bytes, global, 2 named locals
 * resolutionComparison
 * PDB type: bool (const RESOLUTION&, const R...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
static bool resolutionComparison(
    const RESOLUTION &left, const RESOLUTION &right)
{
    if (left.height != right.height) {
        return left.height > right.height;
    }
    return left.width > right.width;
}

/* 0x122650, 165 bytes, global, 8 named locals
 * std::_Regex_traits<char>::translate
 * PDB type: char std::_Regex_traits<char>::(...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\regex
 */

/* 0x122700, 15 bytes, global, 2 named locals
 * std::filesystem::filesystem_error::what
 * PDB type: const char* std::filesystem::fil...
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\filesystem
 */

/* 0x122710, 330 bytes, global, 2 named locals
 * CleanupLevelData
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void CleanupLevelData(void)
{
    if (gpWorld != nullptr) {
        std::free(gpWorld->apAI);
        gpWorld->apAI = nullptr;
        std::free(gpWorld->apActorNames);
        gpWorld->apActorNames = nullptr;
        std::free(gpWorld->apEnemy);
        gpWorld->apEnemy = nullptr;
    }
    jpb_texture_cache.clear();
    FreeFont();
}

/* 0x122860, 59 bytes, global, 0 named locals
 * ClearWindow
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x1228A0, 26 bytes, global, 0 named locals
 * CtrlKeyDown
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int CtrlKeyDown(void)
{
#if defined(_WIN32)
    return ((unsigned short)GetAsyncKeyState(VK_CONTROL)) >> 15;
#else
    std::abort();
#endif
}

/* 0x1228C0, 35 bytes, global, 5 named locals
 * DrawTile
 * PDB type: int (int, int, int, int, unsigne...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" int DrawTile(
    int x, int y, int width, int height, unsigned color)
{
    return DrawRectangle(
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height),
        static_cast<long>(color));
}

/* 0x1228F0, 3 bytes, global, 5 named locals
 * DrawUIRect
 * PDB type: void (int, int, int, int, long)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void DrawUIRect(
    int x, int y, int width, int height, long color)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
}

/* 0x122900, 25 bytes, global, 2 named locals
 * GetWindowSize
 * PDB type: void (int*, int*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void GetWindowSize(int *width, int *height)
{
    if (jpb_get_window_size_hook == nullptr) {
        std::abort();
    }
    jpb_get_window_size_hook(
        width, height, jpb_get_window_size_user_data);
}

/* 0x122920, 15 bytes, global, 1 named locals
 * KeyHeld
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int KeyHeld(int key)
{
    return jpb_whook_key_state.m_bKeyMap[key];
}

/* 0x122930, 15 bytes, global, 1 named locals
 * KeyPressed
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void ClearWindow(void)
{
    if (jpb_clear_window_hook == nullptr) {
        std::abort();
    }
    jpb_clear_window_hook(jpb_clear_window_user_data);
}
int KeyPressed(int key)
{
    return jpb_whook_key_state.m_bKeyMapPressed[key];
}

/* 0x122940, 15 bytes, global, 1 named locals
 * KeyReleased
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int KeyReleased(int key)
{
    return jpb_whook_key_state.m_bKeyMapReleased[key];
}

/* 0x122950, 7 bytes, global, 0 named locals
 * LastKey
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int LastKey(void)
{
    return jpb_whook_key_state.m_nLastKey;
}

extern "C" void jpb_WHookHandleKeyEvent(
    int virtual_key, int scan_code, int pressed)
{
    unsigned char key = (unsigned char)getasciicodefromvirtualkey(
        virtual_key,
        scan_code,
        jpb_whook_key_state.m_bRawKeyMap,
        pressed);

    if (key == 0) {
        return;
    }
    if (pressed != 0) {
        jpb_whook_key_state.m_bKeyMap[key] = 1;
        jpb_whook_key_state.m_bKeyMapPressed[key] = 1;
        jpb_whook_key_state.m_nLastKey = key;
    } else {
        jpb_whook_key_state.m_bKeyMap[key] = 0;
        jpb_whook_key_state.m_bKeyMapReleased[key] = 1;
    }
}

extern "C" void jpb_WHookEndInputFrame(void)
{
    std::memset(
        jpb_whook_key_state.m_bKeyMapPressed,
        0,
        sizeof(jpb_whook_key_state.m_bKeyMapPressed));
    std::memset(
        jpb_whook_key_state.m_bKeyMapReleased,
        0,
        sizeof(jpb_whook_key_state.m_bKeyMapReleased));
    jpb_whook_key_state.m_nLastKey = 0;
}

extern "C" void jpb_WHookClearKeyState(void)
{
    std::memset(&jpb_whook_key_state, 0, sizeof(jpb_whook_key_state));
}

/* 0x122960, 1273 bytes, global, 22 named locals
 * LoadGameData
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

extern "C" void LoadGameData(void)
{
    char save_dir_path[256];
    char save_path[256];
    saveGameStruct save_data;
    void *serialized_data;
    FILE *file;

    std::memset(&save_data, 0, sizeof(save_data));
    std::memcpy(&SaveGameStruct, &save_data, sizeof(SaveGameStruct));
    serialized_data = std::malloc(sizeof(SaveGameStruct));

    std::memset(save_dir_path, 0, sizeof(save_dir_path));
    std::snprintf(
        save_dir_path,
        sizeof(save_dir_path),
        "%s%s",
        jpb_whook_sdl_imports().get_base_path(),
        "SAVEDATA0");
    if (!std::filesystem::exists(save_dir_path)) {
        (void)std::filesystem::create_directory(save_dir_path);
    }

    std::memset(save_path, 0, sizeof(save_path));
    std::snprintf(
        save_path,
        sizeof(save_path),
        "%s%s%s%s",
        jpb_whook_sdl_imports().get_base_path(),
        "SAVEDATA0",
        "\\",
        "Game");
    file = std::fopen(save_path, "rb");
    if (file != nullptr) {
        (void)std::fread(
            serialized_data, 1, sizeof(SaveGameStruct), file);
        deserializeGameStruct(serialized_data);
        ApplySaveGameData();
        (void)std::fclose(file);
    }
    std::free(serialized_data);
}

/* 0x122E60, 2152 bytes, global, 31 named locals
 * LoadOptionsData
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

extern "C" void LoadOptionsData(void)
{
    char save_dir_path[256];
    char save_path[256];
    optionstruct settings_data;
    void *serialized_data = std::malloc(sizeof(optionstruct));
    FILE *file;

    if (serialized_data == nullptr) {
        return;
    }

    std::memset(save_dir_path, 0, sizeof(save_dir_path));
    std::snprintf(
        save_dir_path,
        sizeof(save_dir_path),
        "%s%s",
        jpb_whook_sdl_imports().get_base_path(),
        "SAVEDATA0");
    if (!std::filesystem::exists(save_dir_path)) {
        (void)std::filesystem::create_directory(save_dir_path);
    }

    std::memset(save_path, 0, sizeof(save_path));
    std::snprintf(
        save_path,
        sizeof(save_path),
        "%s%s%s%s",
        jpb_whook_sdl_imports().get_base_path(),
        "SAVEDATA0",
        "\\",
        "Options");
    file = std::fopen(save_path, "rb");
    if (file == nullptr) {
        const unsigned char language = platform_getSystemLanguage();
        int resolution_index;

        OptionStruct.Language = language;
        generateAllText(language);
        resolution_index = getDefaultResolutionIndex();
        OptionStruct.ScreenWidth = static_cast<std::uint32_t>(
            g_resolutions[resolution_index].width);
        OptionStruct.ScreenHeight = static_cast<std::uint32_t>(
            g_resolutions[resolution_index].height);
        settings_data = defaultOptionStruct;
        settings_data.Language = OptionStruct.Language;
        settings_data.ScreenWidth = OptionStruct.ScreenWidth;
        settings_data.ScreenHeight = OptionStruct.ScreenHeight;
        OptionStruct = settings_data;
        SaveSettingsData(settings_data);
        InitGameResolution();
    } else {
        (void)std::fread(serialized_data, 1, sizeof(optionstruct), file);
        std::memcpy(&settings_data, serialized_data, sizeof(settings_data));
        LoadSettingsData(settings_data);
        (void)std::fclose(file);
    }
    std::free(serialized_data);
}

/* 0x1236D0, 11 bytes, global, 0 named locals
 * MarkFontAtlasForRefresh
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void MarkFontAtlasForRefresh(void)
{
    refreshFontAtlasFlag = 1;
}

/* 0x1236E0, 150 bytes, global, 4 named locals
 * OutputTextXY
 * PDB type: void (int, int, char*, <no type>...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void OutputTextXY(int x, int y, char *format, ...)
{
    char buffer[256];
    va_list arguments;

    va_start(arguments, format);
    std::vsprintf(buffer, format, arguments);
    va_end(arguments);
    g_pD3DApp->OutputTextXY(x, y, buffer);
}

/* 0x123780, 4600 bytes, global, 107 named locals
 * PlayVideo
 * PDB type: void (char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
static void audio_callback(
    void *userdata, unsigned char *stream, int length);
extern "C" void __StartRender(void);

static std::string jpb_whook_localized_video_path(const char *path)
{
    static const char *const languages[] = {
        nullptr,
        "German",
        "French",
        "Italian",
        "Spanish",
        "Russian",
        "Simplified Chinese",
    };
    std::string localized(path);

    if (localized.find("English") != std::string::npos &&
        OptionStruct.Language >= 1 && OptionStruct.Language <= 6) {
        localized = std::regex_replace(
            localized,
            std::regex("English"),
            languages[OptionStruct.Language]);
    }
    return localized;
}

extern "C" void PlayVideo(char *path)
{
    constexpr std::uint32_t kSDLQuit = 0x100;
    constexpr std::uint32_t kSDLKeyDown = 0x300;
    constexpr std::uint32_t kSDLControllerButtonDown = 0x603;
    constexpr std::uint16_t kAudioS16LSB = 0x8010;
    constexpr double kSecondsPerFrame = 0.03333333507180214;
    const JPBWHookSDLImports &sdl = jpb_whook_sdl_imports();
    std::string video_path;
    THEORAPLAY_Decoder *decoder;
    const THEORAPLAY_AudioPacket *audio = nullptr;
    const THEORAPLAY_VideoFrame *video = nullptr;
    long long frame_ms = -1;

    vibration_stop(0);
    vibration_stop(1);
    video_path = jpb_whook_localized_video_path(path);
    g_pD3DApp->isFedMovie =
        video_path.find("TEXTCRAWL_7200.ogg") != std::string::npos;

    decoder = THEORAPLAY_startDecodeFile(
        video_path.c_str(), 240, THEORAPLAY_VIDFMT_RGBA);
    for (;;) {
        if (audio == nullptr) {
            audio = THEORAPLAY_getAudio(decoder);
        }
        if (video == nullptr) {
            video = THEORAPLAY_getVideo(decoder);
        }
        if (audio != nullptr && video != nullptr) {
            break;
        }
        sdl.delay(10);
    }

    if (video->fps != 0.0) {
        frame_ms = static_cast<long long>(1000.0 / video->fps);
    }

    SDL_AudioSpec spec = {};
    spec.freq = audio->freq;
    spec.format = kAudioS16LSB;
    spec.channels = static_cast<std::uint8_t>(audio->channels);
    spec.samples = 0x800;
    spec.callback = audio_callback;
    const int open_audio_result = sdl.open_audio(&spec, nullptr);
    bool quit = open_audio_result != 0;
    auto *pixels = static_cast<unsigned char *>(
        std::malloc(
            static_cast<std::size_t>(video->width) * video->height * 4));

    const auto queue_audio = [&sdl](
        const THEORAPLAY_AudioPacket *packet) {
        auto *item = static_cast<AudioQueue *>(
            sdl.malloc_memory(sizeof(AudioQueue)));

        if (item == nullptr) {
            THEORAPLAY_freeAudio(packet);
            return;
        }
        item->audio = packet;
        item->offset = 0;
        item->next = nullptr;
        sdl.lock_audio();
        if (audio_queue_tail != nullptr) {
            audio_queue_tail->next = item;
        } else {
            audio_queue = item;
        }
        audio_queue_tail = item;
        sdl.unlock_audio();
    };

    sdl.delay(2000);
    do {
        queue_audio(audio);
        audio = THEORAPLAY_getAudio(decoder);
    } while (audio != nullptr);
    baseticks = sdl.get_ticks();
    if (open_audio_result == 0) {
        sdl.pause_audio(0);
    }

    SDL_Surface *surface = new SDL_Surface{};
    if (open_audio_result == 0) {
        int width = surface->w;

        for (;;) {
            if (THEORAPLAY_isDecoding(decoder) == 0) {
                for (;;) {
                    sdl.lock_audio();
                    const bool queue_empty = audio_queue == nullptr;
                    sdl.unlock_audio();
                    if (queue_empty) {
                        break;
                    }
                    sdl.delay(100);
                }
                break;
            }

            const std::uint32_t now = sdl.get_ticks() - baseticks;
            if (video == nullptr) {
                video = THEORAPLAY_getVideo(decoder);
            }
            if (video != nullptr && video->playms <= now) {
                if (static_cast<std::uint32_t>(frame_ms) != 0 &&
                    static_cast<std::uint32_t>(frame_ms) <=
                        now - video->playms) {
                    const THEORAPLAY_VideoFrame *last_video =
                        THEORAPLAY_getVideo(decoder);

                    if (last_video != nullptr) {
                        THEORAPLAY_freeVideo(video);
                        video = last_video;
                        if (last_video->fps <= 0.10000000149011612) {
                            quit = true;
                        }
                    }
                }

                (void)sdl.lock_surface(surface);
                std::memcpy(
                    pixels,
                    video->pixels,
                    static_cast<std::size_t>(video->width) *
                        video->height * 4);
                surface->pixels = pixels;
                surface->w = static_cast<int>(video->width);
                surface->h = static_cast<int>(video->height);
                sdl.unlock_surface(surface);
                THEORAPLAY_freeVideo(video);
                video = nullptr;
                width = surface->w;
            }

            audio = THEORAPLAY_getAudio(decoder);
            while (audio != nullptr) {
                queue_audio(audio);
                audio = THEORAPLAY_getAudio(decoder);
            }

            if (video_path.find("Aspyr_Logo") == std::string::npos) {
                SDL_Event event = {};

                while (sdl.poll_event(&event) != 0) {
                    if (event.type == kSDLQuit ||
                        (event.type == kSDLKeyDown &&
                         event.key.keysym.sym == 0x1b) ||
                        event.type == kSDLControllerButtonDown) {
                        quit = true;
                    }
                }
            }

            LARGE_INTEGER current_time;
            LARGE_INTEGER frequency;
            QueryPerformanceCounter(&current_time);
            QueryPerformanceFrequency(&frequency);
            const double elapsed =
                static_cast<double>(
                    current_time.QuadPart - lastTime.QuadPart) /
                static_cast<double>(frequency.QuadPart);
            if (elapsed < kSecondsPerFrame) {
                Sleep(static_cast<DWORD>(
                    (kSecondsPerFrame - elapsed) * 1000.0));
                QueryPerformanceCounter(&current_time);
            }
            lastTime = current_time;
            if (width > 0) {
                chavo.renderVideoFrame(surface);
            }
            if (quit) {
                break;
            }
        }

        std::printf(
            THEORAPLAY_decodingError(decoder) != 0
                ? "There was an error decoding this file!\n"
                : "done with this file!\n");
    } else {
        std::printf("Initialization failed!\n");
    }

    if (video != nullptr) {
        THEORAPLAY_freeVideo(video);
    }
    if (decoder != nullptr) {
        THEORAPLAY_stopDecode(decoder);
    }
    sdl.lock_audio();
    while (audio_queue != nullptr) {
        AudioQueue *next = audio_queue->next;

        sdl.free_memory(audio_queue);
        audio_queue = next;
    }
    audio_queue_tail = nullptr;
    sdl.unlock_audio();
    sdl.close_audio();
    __StartRender();
    std::free(pixels);
}

#if defined(JPB_WHOOK_TESTING)
extern "C" const char *jpb_WHookLocalizedVideoPathForTest(
    const char *path)
{
    static std::string localized;

    localized = jpb_whook_localized_video_path(path);
    return localized.c_str();
}
#endif

/* 0x124980, 3 bytes, global, 0 named locals
 * PresentWindow
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x124990, 22 bytes, global, 0 named locals
 * RefreshFontAtlas
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void RefreshFontAtlas(void)
{
    refreshFontAtlasFlag = 0;
    g_pD3DApp->m_fontAtlas->ClearAtlas();
}

/* 0x1249B0, 161 bytes, global, 7 named locals
 * RenderUIText
 * PDB type: void (int, int, int, int, SDL_Su...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void RenderUIText(
    int x, int y, int width, int height, SDL_Surface *surface)
{
    const JPBWHookSDLImports &sdl = jpb_whook_sdl_imports();
    void *renderer = chavo.m_pFramework->m_pSDLRenderer;
    void *texture = sdl.create_texture_from_surface(renderer, surface);
    const SDL_Rect destination = {x, y, width, height};

    sdl.render_copy(renderer, texture, nullptr, &destination);
    sdl.destroy_texture(texture);
    sdl.free_surface(surface);
}

/* 0x124A60, 223 bytes, global, 9 named locals
 * RenderUITexture
 * PDB type: void (_Material*, SDL_Rect, SDL_...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void RenderUITexture(
    _Material *material,
    SDL_Rect destination,
    SDL_Rect source,
    std::uint8_t alpha,
    int red,
    int green,
    int blue,
    int flip)
{
    const JPBWHookSDLImports &sdl = jpb_whook_sdl_imports();
    void *renderer = chavo.m_pFramework->m_pSDLRenderer;

    sdl.render_set_clip_rect(renderer, nullptr);
    void *texture = *reinterpret_cast<void **>(
        static_cast<unsigned char *>(material->texture) + 0x188);
    if (texture != nullptr) {
        sdl.set_texture_color_mod(
            texture,
            static_cast<std::uint8_t>(red),
            static_cast<std::uint8_t>(green),
            static_cast<std::uint8_t>(blue));
        sdl.set_texture_alpha_mod(texture, alpha);
        sdl.set_texture_blend_mode(texture, 1);
        sdl.render_copy_ex(
            renderer,
            texture,
            source.w == 0 ? nullptr : &source,
            &destination,
            0.0,
            nullptr,
            flip);
    }
}

#if defined(JPB_WHOOK_TESTING)
extern "C" void jpb_WHookSetSDLTestHooks(
    const JPBWHookSDLTestHooks *hooks)
{
    if (hooks == nullptr) {
        std::memset(
            &jpb_whook_sdl_test_imports,
            0,
            sizeof(jpb_whook_sdl_test_imports));
        jpb_whook_has_sdl_test_imports = false;
        return;
    }
    jpb_whook_sdl_test_imports.create_texture_from_surface =
        hooks->create_texture_from_surface;
    jpb_whook_sdl_test_imports.render_copy = hooks->render_copy;
    jpb_whook_sdl_test_imports.destroy_texture = hooks->destroy_texture;
    jpb_whook_sdl_test_imports.free_surface = hooks->free_surface;
    jpb_whook_sdl_test_imports.render_set_clip_rect =
        hooks->render_set_clip_rect;
    jpb_whook_sdl_test_imports.set_texture_color_mod =
        hooks->set_texture_color_mod;
    jpb_whook_sdl_test_imports.set_texture_alpha_mod =
        hooks->set_texture_alpha_mod;
    jpb_whook_sdl_test_imports.set_texture_blend_mode =
        hooks->set_texture_blend_mode;
    jpb_whook_sdl_test_imports.render_copy_ex = hooks->render_copy_ex;
    jpb_whook_sdl_test_imports.get_base_path = hooks->get_base_path;
    jpb_whook_has_sdl_test_imports = true;
}

extern "C" void jpb_WHookSetWinMainTestHooks(
    const JPBWHookWinMainTestHooks *hooks)
{
    if (hooks == nullptr) {
        std::memset(
            &jpb_whook_win_main_test_hooks,
            0,
            sizeof(jpb_whook_win_main_test_hooks));
        jpb_whook_has_win_main_test_hooks = false;
        return;
    }
    jpb_whook_win_main_test_hooks = *hooks;
    jpb_whook_has_win_main_test_hooks = true;
}

extern "C" void jpb_WHookVideoDestinationForTest(
    std::uint32_t screen_width,
    std::uint32_t screen_height,
    SCREENRECT *destination)
{
    const RECT result =
        jpb_whook_video_destination(screen_width, screen_height);
    destination->left = result.left;
    destination->top = result.top;
    destination->right = result.right;
    destination->bottom = result.bottom;
}

extern "C" void jpb_WHookText3DQuadForTest(
    std::uint32_t left,
    std::uint32_t top,
    std::uint32_t right,
    std::uint32_t bottom,
    float atlas_width,
    float atlas_height,
    float scale,
    float maximum_height,
    float x,
    float y,
    float z,
    std::uint32_t color,
    JPBScreenPolyVertex vertices[4],
    float *next_x)
{
    const GlyphEntry entry = {left, top, right, bottom};
    *next_x = jpb_whook_text_3d_quad(
        entry,
        atlas_width,
        atlas_height,
        scale,
        maximum_height,
        x,
        y,
        z,
        color,
        vertices);
}

extern "C" float jpb_WHookText3DLineAdvanceForTest(
    float y, float maximum_height)
{
    return jpb_whook_text_3d_line_advance(y, maximum_height);
}

extern "C" void jpb_WHookText2DDrawForTest(
    int minimum_x,
    int maximum_x,
    int maximum_y,
    std::uint32_t left,
    std::uint32_t top,
    std::uint32_t right,
    std::uint32_t bottom,
    int maximum_height,
    int x,
    int y,
    CVECTOR color,
    int clipping,
    SCREENRECT scissor,
    int depth_enabled,
    float depth,
    JPBWHookText2DDrawTest *result,
    int *next_x)
{
    Glyph glyph = {};
    glyph.Metrics.minx = minimum_x;
    glyph.Metrics.maxx = maximum_x;
    glyph.Metrics.maxy = maximum_y;
    const GlyphEntry entry = {left, top, right, bottom};
    const RECT native_scissor = {
        scissor.left, scissor.top, scissor.right, scissor.bottom};
    const SpriteDraw draw = jpb_whook_text_2d_sprite(
        nullptr,
        glyph,
        entry,
        maximum_height,
        x,
        y,
        color,
        clipping != 0,
        native_scissor,
        depth_enabled != 0,
        depth);

    const RECT &source = draw.SrcRect.value();
    result->source = {
        source.left, source.top, source.right, source.bottom};
    result->destination = {
        draw.DestRect.left,
        draw.DestRect.top,
        draw.DestRect.right,
        draw.DestRect.bottom};
    result->color[0] = draw.Color.f[0];
    result->color[1] = draw.Color.f[1];
    result->color[2] = draw.Color.f[2];
    result->color[3] = draw.Color.f[3];
    result->depth = draw.LayerDepth;
    result->has_scissor = draw.ScissorRect.has_value() ? 1 : 0;
    if (draw.ScissorRect.has_value()) {
        const RECT &draw_scissor = draw.ScissorRect.value();
        result->scissor = {
            draw_scissor.left,
            draw_scissor.top,
            draw_scissor.right,
            draw_scissor.bottom};
    } else {
        result->scissor = {};
    }
    result->sampler_type = static_cast<int>(draw.SamplerType);
    *next_x = x + maximum_x - minimum_x;
}

extern "C" int jpb_WHookTextControllerIconForTest(
    std::uint16_t character)
{
    return jpb_whook_text_controller_icon(
        static_cast<char16_t>(character));
}

extern "C" int jpb_WHookTextTagIconForTest(
    std::uint16_t character, int *alpha)
{
    return jpb_whook_text_tag_icon(
        static_cast<char16_t>(character), alpha);
}
#endif

/* 0x124B40, 21 bytes, global, 0 named locals
 * SDL_ResetClipRect
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void SDL_ResetClipRect(void)
{
    jpb_TextClearClipRect();
}

/* 0x124B60, 79 bytes, global, 5 named locals
 * SDL_SetClip
 * PDB type: void (int, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void SDL_SetClip(int x, int y, int width, int height)
{
    jpb_TextSetClipRect(x, y, x + width, y + height);
}

/* 0x124BB0, 1228 bytes, global, 23 named locals
 * SaveGameData
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

extern "C" void SaveGameData(void)
{
    char save_dir_path[256];
    char save_path[256];
    void *serialized_data;
    FILE *file;

    UpdateSaveGameStruct();
    serialized_data = serializeGameStruct();

    std::memset(save_dir_path, 0, sizeof(save_dir_path));
    std::snprintf(
        save_dir_path,
        sizeof(save_dir_path),
        "%s%s",
        jpb_whook_sdl_imports().get_base_path(),
        "SAVEDATA0");
    if (!std::filesystem::exists(save_dir_path)) {
        (void)std::filesystem::create_directory(save_dir_path);
    }

    std::memset(save_path, 0, sizeof(save_path));
    std::snprintf(
        save_path,
        sizeof(save_path),
        "%s%s%s%s",
        jpb_whook_sdl_imports().get_base_path(),
        "SAVEDATA0",
        "\\",
        "Game");
    file = std::fopen(save_path, "wb");
    if (file != nullptr) {
        (void)std::fwrite(
            serialized_data, 1, sizeof(SaveGameStruct), file);
        (void)std::fclose(file);
    }
    std::free(serialized_data);
}

/* 0x125080, 1241 bytes, global, 24 named locals
 * SaveSettingsData
 * PDB type: void (optionstruct)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

extern "C" void SaveSettingsData(optionstruct settings_data)
{
    char save_dir_path[256];
    char save_path[256];
    void *serialized_data = serializeOptionsStruct(&settings_data);
    FILE *file;

    std::memset(save_dir_path, 0, sizeof(save_dir_path));
    std::snprintf(
        save_dir_path,
        sizeof(save_dir_path),
        "%s%s",
        jpb_whook_sdl_imports().get_base_path(),
        "SAVEDATA0");
    if (!std::filesystem::exists(save_dir_path)) {
        (void)std::filesystem::create_directory(save_dir_path);
    }

    std::memset(save_path, 0, sizeof(save_path));
    std::snprintf(
        save_path,
        sizeof(save_path),
        "%s%s%s%s",
        jpb_whook_sdl_imports().get_base_path(),
        "SAVEDATA0",
        "\\",
        "Options");
    file = std::fopen(save_path, "wb");
    if (file != nullptr) {
        (void)std::fwrite(serialized_data, 1, sizeof(optionstruct), file);
        (void)std::fclose(file);
    }
    std::free(serialized_data);
}

/* 0x125560, 20 bytes, global, 1 named locals
 * SetInMenu
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void SetInMenu(int in_menu)
{
    g_pD3DApp->m_pFramework->inMenu = in_menu == 1;
}

/* 0x125580, 26 bytes, global, 0 named locals
 * ShiftKeyDown
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int ShiftKeyDown(void)
{
#if defined(_WIN32)
    return ((unsigned short)GetAsyncKeyState(VK_SHIFT)) >> 15;
#else
    std::abort();
#endif
}

/* 0x1255A0, 20 bytes, global, 3 named locals
 * UpdateResolution
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void PresentWindow(void)
{
}
extern "C" void UpdateResolution(int width, int height, int window_mode)
{
    resolutionUpdated = 1;
    newWidth = width;
    newHeight = height;
    newWindowMode = window_mode;
}

/* 0x1255C0, 497 bytes, global, 11 named locals
 * WaitVBlank
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void WaitVBlank(void)
{
    constexpr double target_frame_milliseconds = 16.6666603088379;

    framenow = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> elapsed =
        framenow - framelast;
    if (elapsed.count() < target_frame_milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<long long>(
                target_frame_milliseconds - elapsed.count())));
    }
    framelast = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> slept =
        framelast - framenow;
    deltaTime = static_cast<float>(
        (slept.count() + elapsed.count()) / 2.0);
}

/* 0x1257C0, 590 bytes, global, 6 named locals
 * WinMain
 * PDB type: int (HINSTANCE__*, HINSTANCE__*,...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
#if defined(_WIN32)
namespace {

HINSTANCE hInst;

constexpr char kSteamInitializationError[] =
    "Fatal Error - Steam must be running to play this game "
    "(SteamAPI_Init() failed).\n";
constexpr char kWHookSourcePath[] =
    "W:\\SWJediPowerBattles\\Work\\wHook.cpp";
constexpr char kCreateApplicationCall[] = "app.Create";

void WinMainUpdateMenus()
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.update_menus != nullptr) {
        jpb_whook_win_main_test_hooks.update_menus();
        return;
    }
#endif
    UpdateMenus();
}

void WinMainLoadOptionsData()
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.load_options_data != nullptr) {
        jpb_whook_win_main_test_hooks.load_options_data();
        return;
    }
#endif
    LoadOptionsData();
}

void WinMainSetWidthHeight(unsigned width, unsigned height)
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.set_width_height != nullptr) {
        jpb_whook_win_main_test_hooks.set_width_height(width, height);
        return;
    }
#endif
    chavo.SetWidthHeight(width, height);
}

bool WinMainSteamAPIInit()
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.steam_api_init != nullptr) {
        return jpb_whook_win_main_test_hooks.steam_api_init() != 0;
    }
#endif
    return SteamAPI_Init();
}

HRESULT WinMainCreateApplication(HINSTANCE instance, char *command_line)
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.create_application != nullptr) {
        return static_cast<HRESULT>(
            jpb_whook_win_main_test_hooks.create_application(
                instance, command_line));
    }
#endif
    return chavo.Create(instance, command_line);
}

bool WinMainRestartAppIfNecessary(std::uint32_t app_id)
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks
                .steam_api_restart_app_if_necessary != nullptr) {
        return jpb_whook_win_main_test_hooks
                   .steam_api_restart_app_if_necessary(app_id) != 0;
    }
#endif
    return SteamAPI_RestartAppIfNecessary(app_id);
}

CSteamAchievements *WinMainCreateAchievements()
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.create_achievements != nullptr) {
        return static_cast<CSteamAchievements *>(
            jpb_whook_win_main_test_hooks.create_achievements(
                g_Achievements, 43));
    }
#endif
    return new CSteamAchievements(g_Achievements, 43);
}

CSteamGameManager *WinMainCreateGameManager()
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.create_game_manager != nullptr) {
        return static_cast<CSteamGameManager *>(
            jpb_whook_win_main_test_hooks.create_game_manager());
    }
#endif
    return new CSteamGameManager();
}

CSteamRichPresence *WinMainCreateRichPresence()
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.create_rich_presence != nullptr) {
        return static_cast<CSteamRichPresence *>(
            jpb_whook_win_main_test_hooks.create_rich_presence());
    }
#endif
    return new CSteamRichPresence();
}

bool WinMainIsSteamRunningOnSteamDeck()
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks
                .is_steam_running_on_steam_deck != nullptr) {
        return jpb_whook_win_main_test_hooks
                   .is_steam_running_on_steam_deck() != 0;
    }
#endif
    ISteamUtils *utils = SteamUtils();
    void **vtable = *reinterpret_cast<void ***>(utils);
    using IsSteamRunningOnSteamDeckFunction = bool (*)(ISteamUtils *);
    return reinterpret_cast<IsSteamRunningOnSteamDeckFunction>(vtable[34])(
        utils);
}

void WinMainInitializeMain()
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.initialize_main != nullptr) {
        jpb_whook_win_main_test_hooks.initialize_main();
        return;
    }
    std::abort();
#else
    initialize_main();
#endif
}

void WinMainSteamAPIShutdown()
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.steam_api_shutdown != nullptr) {
        jpb_whook_win_main_test_hooks.steam_api_shutdown();
        return;
    }
#endif
    SteamAPI_Shutdown();
}

void WinMainDestroyAchievements(CSteamAchievements *achievements)
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.destroy_achievements != nullptr) {
        jpb_whook_win_main_test_hooks.destroy_achievements(achievements);
        return;
    }
#endif
    delete achievements;
}

void WinMainReportCreateError(HRESULT result)
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.report_create_error != nullptr) {
        jpb_whook_win_main_test_hooks.report_create_error(
            static_cast<std::uint32_t>(result),
            const_cast<char *>(kCreateApplicationCall),
            0x2CC,
            const_cast<char *>(kWHookSourcePath));
        return;
    }
#endif
    d3derr(
        static_cast<unsigned>(result),
        const_cast<char *>(kCreateApplicationCall),
        0x2CC,
        const_cast<char *>(kWHookSourcePath));
}

void WinMainPrintFatalError()
{
#if defined(JPB_WHOOK_TESTING)
    if (jpb_whook_has_win_main_test_hooks &&
        jpb_whook_win_main_test_hooks.print_fatal_error != nullptr) {
        jpb_whook_win_main_test_hooks.print_fatal_error(
            kSteamInitializationError);
        return;
    }
#endif
    std::printf(kSteamInitializationError);
}

} // namespace

extern "C" int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE,
    char *command_line,
    int)
{
    WinMainUpdateMenus();
    hInst = instance;
    WinMainLoadOptionsData();
    if (OptionStruct.ScreenWidth != 0 &&
        OptionStruct.ScreenHeight != 0) {
        WinMainSetWidthHeight(
            OptionStruct.ScreenWidth, OptionStruct.ScreenHeight);
    }

    const bool steam_initialized = WinMainSteamAPIInit();
    const HRESULT result =
        WinMainCreateApplication(instance, command_line);
    if (FAILED(result)) {
        WinMainReportCreateError(result);
        return 0;
    }

    if (steam_initialized) {
        if (WinMainRestartAppIfNecessary(UINT32_C(0x25496e))) {
            WinMainPrintFatalError();
            return 0;
        }

        g_SteamAchievements = WinMainCreateAchievements();
        g_SteamGameManager = WinMainCreateGameManager();
        g_SteamRicherPresence = WinMainCreateRichPresence();
        const bool is_steam_deck = WinMainIsSteamRunningOnSteamDeck();
        if (is_steam_deck) {
            chavo.m_pFramework->m_isAMD = 1;
        }
        chavo.m_pFramework->m_isSteamDeck = is_steam_deck ? 1 : 0;
        g_isSteamDeck = is_steam_deck ? 1 : 0;
        WinMainInitializeMain();
        return 0;
    }

    WinMainPrintFatalError();
    WinMainSteamAPIShutdown();
    if (g_SteamAchievements != nullptr) {
        WinMainDestroyAchievements(g_SteamAchievements);
    }
    return 0;
}
#endif

/* 0x125A10, 3 bytes, global, 1 named locals
 * WriteToOutputFile
 * PDB type: void (const char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void WriteToOutputFile(const char *)
{
}

/* 0x125A20, 221 bytes, global, 4 named locals
 * _ApplyLevelTransformation
 * PDB type: void (MATRIX*, float, float, flo...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _ApplyLevelTransformation(
    MATRIX *matrix, float x_scale, float y_scale, float z_scale)
{
    chavo.ApplyLevelTransformation(matrix, x_scale, y_scale, z_scale);
}

/* 0x125B00, 108 bytes, global, 9 named locals
 * _ApplyProjection
 * PDB type: void (FVECTOR*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _ApplyProjection(FVECTOR *vertices)
{
    chavo.ApplyProjection(vertices);
}

/* 0x125B70, 81 bytes, global, 1 named locals
 * _ClearTextureCache
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _ClearTextureCache(void)
{
    jpb_try_texture_cache.clear();
}

/* 0x125BD0, 309 bytes, global, 6 named locals
 * _DrawTexture
 * PDB type: void (_Material*, SCREENRECT, co...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _DrawTexture(
    _Material *texture,
    SCREENRECT destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth)
{
    if (jpb_draw_texture_hook != nullptr) {
        jpb_draw_texture_hook(
            jpb_draw_texture_user_data,
            texture,
            &destination,
            source,
            color,
            layer_depth);
    }
}

/* 0x125D10, 339 bytes, global, 7 named locals
 * _DrawTextureClipped
 * PDB type: void (_Material*, SCREENRECT, co...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _DrawTextureClipped(
    _Material *texture,
    SCREENRECT destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth,
    SCREENRECT scissor)
{
    if (jpb_draw_texture_clipped_hook != nullptr) {
        jpb_draw_texture_clipped_hook(
            jpb_draw_texture_clipped_user_data,
            texture,
            &destination,
            source,
            color,
            layer_depth,
            &scissor);
    }
}

/* 0x125E70, 53 bytes, global, 5 named locals
 * _DrawUITextUTF16
 * PDB type: void (unsigned short*, SCREENREC...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _DrawUITextUTF16(
    uint16_t *text,
    SCREENRECT destination,
    int font_style,
    int point_size,
    CVECTOR color)
{
    if (jpb_draw_ui_text_utf16_hook != nullptr) {
        JPBPortableTextClipState previous =
            jpb_whook_begin_legacy_text_clip();

        jpb_draw_ui_text_utf16_hook(
            jpb_draw_ui_text_utf16_user_data,
            text,
            &destination,
            font_style,
            point_size,
            color,
            0,
            0.0f);
        jpb_whook_end_legacy_text_clip(previous);
        return;
    }
#if defined(JPB_WHOOK_TESTING)
    return;
#else
    chavo.DrawUITextUTF16(
        text, destination, font_style, point_size, color);
#endif
}

/* 0x125EB0, 71 bytes, global, 6 named locals
 * _DrawUITextUTF16Depth
 * PDB type: void (unsigned short*, SCREENREC...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _DrawUITextUTF16Depth(
    uint16_t *text,
    SCREENRECT destination,
    int font_style,
    int point_size,
    CVECTOR color,
    float depth)
{
    if (jpb_draw_ui_text_utf16_hook != nullptr) {
        JPBPortableTextClipState previous =
            jpb_whook_begin_legacy_text_clip();

        jpb_draw_ui_text_utf16_hook(
            jpb_draw_ui_text_utf16_user_data,
            text,
            &destination,
            font_style,
            point_size,
            color,
            1,
            depth);
        jpb_whook_end_legacy_text_clip(previous);
        return;
    }
#if defined(JPB_WHOOK_TESTING)
    return;
#else
    chavo.DrawUITextUTF16Depth(
        text, destination, font_style, point_size, color, depth);
#endif
}

/* 0x125F00, 63 bytes, global, 7 named locals
 * _DrawUITextUTF16_3D
 * PDB type: void (unsigned short*, float, fl...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _DrawUITextUTF16_3D(
    uint16_t *text,
    float x,
    float y,
    float z,
    int font_style,
    int point_size,
    uint32_t color)
{
    if (jpb_draw_ui_text_utf16_3d_hook != nullptr) {
        jpb_draw_ui_text_utf16_3d_hook(
            jpb_draw_ui_text_utf16_3d_user_data,
            text,
            x,
            y,
            z,
            font_style,
            point_size,
            color);
        return;
    }
#if defined(JPB_WHOOK_TESTING)
    return;
#else
    chavo.DrawUITextUTF16_3D(
        text, x, y, z, font_style, point_size, color);
#endif
}

static void jpb_whook_publish_screen_poly(int no_scale)
{
    if (jpb_screen_poly_hook != nullptr &&
        jpb_screen_poly_builder.requestedVertexCount > 0 &&
        jpb_screen_poly_builder.requestedVertexCount <=
            JPB_SCREEN_POLY_VERTEX_CAPACITY) {
        jpb_screen_poly_hook(
            jpb_screen_poly_user_data,
            jpb_screen_poly_builder.material,
            jpb_screen_poly_builder.materialFlags,
            jpb_screen_poly_builder.requestedVertexCount,
            jpb_screen_poly_builder.vertices,
            no_scale);
    }
    jpb_screen_poly_builder.material = nullptr;
    jpb_screen_poly_builder.requestedVertexCount = 0;
}

/* 0x125F40, 12 bytes, global, 0 named locals
 * _EndPoly
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _EndPoly(void)
{
    jpb_whook_publish_screen_poly(0);
}

/* Reference RVA 0x125F50; the platform destructor is isolated by texture.c. */
void _FreeTexture(_Material *texture)
{
    if (texture == nullptr || texture->texture == nullptr) {
        return;
    }
    jpb_texture_cache.erase(texture->filename);
    jpb_TextureUnloadPlatformResource(texture->texture);
    texture->texture = nullptr;
}

/* 0x1262D0, 15 bytes, global, 1 named locals
 * _InitFBXLevelData
 * PDB type: void (ufbx_scene*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

static std::string el_chavo_LoadTexturePath(const char *filename)
{
    std::string path(filename);
    size_t dot;

    if (std::strstr(path.c_str(), "sgi") != nullptr) {
        dot = path.rfind('.');
        if (dot != std::string::npos) {
            path.replace(dot + 1, std::string::npos, "tim");
        }
    }
    if (std::strstr(path.c_str(), "pvr") != nullptr) {
        dot = path.rfind('.');
        if (dot != std::string::npos) {
            path.replace(dot + 1, std::string::npos, "tga");
        }
    }
    return path;
}

/* Reference RVA 0x1262E0, excluding the original D3D12 resource internals. */
_Material *_LoadTexture(
    char *filename, TT_TEXTYPE texturetype, unsigned long option)
{
    const char *resolvedFilename = filename;
    const char *baseName;
    _Material *material;
    void *texture;
    int materialtype = 0;
    int16_t width = 0;
    int16_t height = 0;
    std::string platformFilename;
    std::unordered_map<std::string, _Material *>::const_iterator cached;

    if (resolvedFilename == nullptr) {
        resolvedFilename = resource_getPath(
            "white.png", JPB_RESOURCE_DEFAULT);
    }
    if (resolvedFilename != nullptr) {
        cached = jpb_texture_cache.find(resolvedFilename);
        if (cached != jpb_texture_cache.end()) {
            return cached->second;
        }
    }

    material = texture_GetMaterial(texturetype);
    if (material == nullptr) {
        return nullptr;
    }
    if (resolvedFilename == nullptr) {
        material->texture = jpb_TextureLoadPlatformResource(
            nullptr,
            static_cast<unsigned>(option & UINT32_C(0xff)),
            0,
            &width,
            &height);
        return material;
    }

    baseName = resolvedFilename;
    for (const char *cursor = resolvedFilename;
         *cursor != '\0'; ++cursor) {
        if (*cursor == '\\' || *cursor == ':') {
            baseName = cursor + 1;
        }
    }
    /* Exact retail prefix classification: lowercase a_/p_ only. */
    if (baseName[0] == 'a' && baseName[1] == '_') {
        materialtype = 2;
    } else if (baseName[0] == 'p' && baseName[1] == '_') {
        materialtype = 1;
    }
    {
        char *destination = material->filename;
        const char *source = resolvedFilename;

        do {
            *destination++ = *source;
        } while (*source++ != '\0');
    }

    platformFilename = el_chavo_LoadTexturePath(resolvedFilename);
    texture = jpb_TextureLoadPlatformResource(
        platformFilename.c_str(),
        static_cast<unsigned>(
            option & (UINT32_C(0x02000000) | UINT32_C(0xff))),
        materialtype,
        &width,
        &height);
    if (texture == nullptr) {
        std::fprintf(
            stderr,
            "texture_fallback=(requested=%s,platform=%s,level=%d)\n",
            resolvedFilename,
            platformFilename.c_str(),
            static_cast<int>(static_cast<std::int8_t>(LevelSelect)));
        texture = jpb_TextureLoadPlatformResource(
            "../../../res/default\\o_default.tga",
            static_cast<unsigned>(option & UINT32_C(0xff)),
            0,
            &width,
            &height);
    }
    jpb_texture_cache.emplace(resolvedFilename, material);
    material->samplerType =
        jpb_TextureIsPartOfAtlas(material->filename)
            ? TEXTURSAMPLER_POINTCLAMP
            : TEXTURESAMPLER_LINEARCLAMP;
    if (texture == nullptr) {
        texture_FreeMaterial(material);
        return nullptr;
    }
    material->texture = texture;
    material->iw = width;
    material->ih = height;
    SetTextureColorOverride((int)(int8_t)LevelSelect, material);
    return material;
}

/* 0x1266D0, 12 bytes, global, 0 named locals
 * _NoScaleEndPoly
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void _InitFBXLevelData(ufbx_scene *scene)
{
    if (jpb_init_fbx_level_data_hook == nullptr) {
        std::abort();
    }
    jpb_init_fbx_level_data_hook(
        jpb_init_fbx_level_data_user_data, scene);
}
void _NoScaleEndPoly(void)
{
    jpb_whook_publish_screen_poly(1);
}

/* 0x1266E0, 70 bytes, global, 7 named locals
 * _SetVert
 * PDB type: void (int, float, float, float, ...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _SetVert(
    int vertex,
    float x,
    float y,
    float z,
    unsigned long argb,
    float tu,
    float tv)
{
    JPBScreenPolyVertex *destination;

    if (vertex < 0 ||
        vertex >= jpb_screen_poly_builder.requestedVertexCount ||
        vertex >= JPB_SCREEN_POLY_VERTEX_CAPACITY) {
        return;
    }
    destination = &jpb_screen_poly_builder.vertices[vertex];
    destination->x = x;
    destination->y = y;
    destination->z = z;
    destination->argb = static_cast<uint32_t>(argb);
    destination->tu = tu;
    destination->tv = tv;
}

/* 0x126730, 17 bytes, global, 2 named locals
 * _StartPoly
 * PDB type: void (int, _Material*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
void _StartPoly(int vertex_count, _Material *material)
{
    if (material == nullptr || material->texture == nullptr) {
        return;
    }
    jpb_screen_poly_builder.material = material;
    jpb_screen_poly_builder.materialFlags = material->flags;
    jpb_screen_poly_builder.requestedVertexCount = vertex_count;
}

/* 0x126750, 3 bytes, global, 0 named locals
 * _StoreDescriptorHeapOffsetsEnd
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void _StoreDescriptorHeapOffsetsEnd(void)
{
}

/* 0x126760, 3 bytes, global, 0 named locals
 * _StoreDescriptorHeapOffsetsStart
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void _StoreDescriptorHeapOffsetsStart(void)
{
}

/* Reference RVA 0x126770. */
_Material *_TryLoadTexture(
    const char *baseFileName,
    TT_TEXTYPE texturetype,
    unsigned long option)
{
    std::string levelName;
    std::string convertedFileName;
    const char *fullFilePath;
    _Material *materialHandle;
    std::map<std::string, _Material *>::const_iterator cached;

    if (baseFileName == nullptr) {
        return nullptr;
    }
    cached = jpb_try_texture_cache.find(baseFileName);
    if (cached != jpb_try_texture_cache.end()) {
        if (cached->second != nullptr) {
            return cached->second;
        }
        std::printf("Handle is null: %s", baseFileName);
    }
    levelName = loader_GetLevelName();
    if (levelName == "arena") {
        levelName = "fed";
    }
    convertedFileName = levelName + "/" + baseFileName;
    fullFilePath = resource_getPath(
        convertedFileName.c_str(), JPB_RESOURCE_LEVEL_JPX);
    materialHandle = _LoadTexture(
        const_cast<char *>(fullFilePath), texturetype, option);
    jpb_try_texture_cache[baseFileName] = materialHandle;
    return materialHandle;
}

/* 0x126DA0, 71 bytes, global, 0 named locals
 * __EndRender
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void __EndRender(void)
{
    g_pD3DApp->FrameEnd();
    std::memset(g_pD3DApp->m_bKeyMapPressed, 0, 0x100);
    std::memset(g_pD3DApp->m_bKeyMapReleased, 0, 0x100);
    g_pD3DApp->m_nLastKey = 0;
}

/* 0x126DF0, 23 bytes, global, 0 named locals
 * __HandleWindow
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" int __HandleWindow(void)
{
    g_pD3DApp->MessagePump();
    return 0;
}

/* 0x126E10, 3 bytes, global, 0 named locals
 * __InitSystem
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void __InitSystem(void)
{
}

/* 0x126E20, 136 bytes, global, 2 named locals
 * __PCTrace
 * PDB type: void (char*, <no type>)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void __PCTrace(char *format, ...)
{
    char buffer[1024];
    va_list arguments;

    va_start(arguments, format);
    std::vsprintf(buffer, format, arguments);
    va_end(arguments);
    OutputDebugStringA(buffer);
}

/* 0x126EB0, 236 bytes, global, 5 named locals
 * __RenderLoad
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void __RenderLoad(int endframe)
{
    (void)endframe;
    vibration_stop(0);
    vibration_stop(1);
    if (jpb_render_load_hook == nullptr) {
        std::abort();
    }
    jpb_render_load_hook(jpb_render_load_user_data);
}

/* 0x126FA0, 71 bytes, global, 0 named locals
 * __StartRender
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void __StartRender(void)
{
    constexpr size_t kTransPolygonOffset = 0x310A70;
    constexpr size_t kOrderingTableOffset = 0x3A8A70;
    constexpr size_t kCurrentPolyOffset = 0x3AAA70;
    constexpr size_t kCurrentPolyIsTransOffset = 0x3AAA78;
    auto *bytes = reinterpret_cast<unsigned char *>(g_pD3DApp);

    g_pD3DApp->FrameBegin();
    std::memset(bytes + kOrderingTableOffset, 0, 0x2000);
    *reinterpret_cast<void **>(bytes + kCurrentPolyOffset) =
        bytes + kTransPolygonOffset;
    *reinterpret_cast<int *>(bytes + kCurrentPolyIsTransOffset) = 0;
    oldtexture = nullptr;
}

/* 0x126FF0, 699 bytes, local, 11 named locals
 * audio_callback
 * PDB type: void (void*, unsigned char*, int...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
static void audio_callback(void *userdata, unsigned char *stream, int length)
{
    AudioQueue *queue = audio_queue;

    (void)userdata;
    while (queue != nullptr) {
        AudioQueue *current;
        AudioQueue *next;
        const THEORAPLAY_AudioPacket *packet;
        float *samples;
        int channels;
        int remaining_samples;
        int sample_count;
        float volume;

        if (length < 1) {
            if (queue != nullptr) {
                return;
            }
            break;
        }

        current = queue;
        next = current->next;
        packet = current->audio;
        channels = packet->channels;
        samples = packet->samples + current->offset * channels;
        remaining_samples = (packet->frames - current->offset) * channels;
        sample_count = length / 2;
        if (static_cast<std::uint64_t>(
                static_cast<std::int64_t>(remaining_samples)) <=
            (static_cast<std::uint64_t>(
                 static_cast<std::int64_t>(length)) >> 1)) {
            sample_count = remaining_samples;
        }

        volume = static_cast<float>(OptionStruct.musicVolume) * 0.0078125f;
        for (int index = 0; index < sample_count; ++index) {
            const float scaled_sample = volume * samples[index];
            std::int16_t output;

            if (scaled_sample < -1.0f) {
                output = INT16_MIN;
            } else if (scaled_sample > 1.0f) {
                output = INT16_MAX;
            } else {
                output = static_cast<std::int16_t>(static_cast<int>(
                    scaled_sample * 32767.0f * VideoVolume));
            }
            std::memcpy(stream, &output, sizeof(output));
            stream += sizeof(output);
        }

        length -= sample_count * 2;
        current->offset += sample_count / channels;
        if (current->offset >= packet->frames) {
            THEORAPLAY_freeAudio(packet);
            jpb_D3DAppSDLFree(current);
            audio_queue = next;
            queue = next;
        } else {
            queue = audio_queue;
        }
    }

    audio_queue_tail = nullptr;
    if (length > 0) {
        std::memset(stream, 0, static_cast<std::size_t>(length));
    }
}

#if defined(JPB_WHOOK_TESTING)
extern "C" void jpb_WHookSetAudioQueueForTest(
    AudioQueue *head, AudioQueue *tail)
{
    audio_queue = head;
    audio_queue_tail = tail;
}

extern "C" AudioQueue *jpb_WHookAudioQueueForTest(void)
{
    return audio_queue;
}

extern "C" AudioQueue *jpb_WHookAudioQueueTailForTest(void)
{
    return audio_queue_tail;
}

extern "C" void jpb_WHookAudioCallbackForTest(
    void *userdata, unsigned char *stream, int length)
{
    audio_callback(userdata, stream, length);
}
#endif

/* 0x1272B0, 5 bytes, global, 0 named locals
 * clearzerobss
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void clearzerobss(void)
{
    ZeroBSS_ClearAll();
}

/* 0x1272C0, 652 bytes, global, 13 named locals
 * cliptoscreen
 * PDB type: int (short*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127550, 23 bytes, global, 4 named locals
 * console_TextureListCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" int console_TextureListCommand(
    int integer_argument_count,
    char **string_arguments,
    int *integer_arguments,
    float *float_arguments)
{
    (void)integer_argument_count;
    (void)string_arguments;
    (void)integer_arguments;
    (void)float_arguments;
    console_Printf("This is a dreamcast command!\n");
    return 0;
}

/* 0x127570, 141 bytes, global, 2 named locals
 * dbgprintf
 * PDB type: void (char*, <no type>)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void dbgprintf(char *format, ...)
{
    char buffer[512];
    va_list arguments;

    va_start(arguments, format);
    _vsnprintf(buffer, sizeof(buffer), format, arguments);
    va_end(arguments);
    g_pD3DApp->OutputText(buffer);
}

/* 0x127600, 764 bytes, global, 27 named locals
 * debug_box
 * PDB type: void (_svector*, _svector*, unsi...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void debug_box(
    _svector *top_left, _svector *bottom_right, uint32_t color)
{
    debug_drawline(
        top_left->vx, top_left->vy, top_left->vz,
        bottom_right->vx, top_left->vy, top_left->vz,
        color);
    debug_drawline(
        bottom_right->vx, top_left->vy, top_left->vz,
        bottom_right->vx, top_left->vy, bottom_right->vz,
        color);
    debug_drawline(
        bottom_right->vx, top_left->vy, bottom_right->vz,
        top_left->vx, top_left->vy, bottom_right->vz,
        color);
    debug_drawline(
        top_left->vx, top_left->vy, bottom_right->vz,
        top_left->vx, top_left->vy, top_left->vz,
        color);
    debug_drawline(
        top_left->vx, bottom_right->vy, top_left->vz,
        bottom_right->vx, bottom_right->vy, top_left->vz,
        color);
    debug_drawline(
        bottom_right->vx, bottom_right->vy, top_left->vz,
        bottom_right->vx, bottom_right->vy, bottom_right->vz,
        color);
    debug_drawline(
        bottom_right->vx, bottom_right->vy, bottom_right->vz,
        top_left->vx, bottom_right->vy, bottom_right->vz,
        color);
    debug_drawline(
        top_left->vx, bottom_right->vy, bottom_right->vz,
        top_left->vx, bottom_right->vy, top_left->vz,
        color);
    debug_drawline(
        top_left->vx, top_left->vy, top_left->vz,
        top_left->vx, bottom_right->vy, top_left->vz,
        color);
    debug_drawline(
        bottom_right->vx, top_left->vy, top_left->vz,
        bottom_right->vx, bottom_right->vy, top_left->vz,
        color);
    debug_drawline(
        bottom_right->vx, top_left->vy, bottom_right->vz,
        bottom_right->vx, bottom_right->vy, bottom_right->vz,
        color);
    debug_drawline(
        top_left->vx, top_left->vy, bottom_right->vz,
        top_left->vx, bottom_right->vy, bottom_right->vz,
        color);
}

/* 0x127900, 100 bytes, global, 9 named locals
 * debug_drawline
 * PDB type: void (int, int, int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void debug_drawline(
    int x1,
    int y1,
    int z1,
    int x2,
    int y2,
    int z2,
    uint32_t color)
{
    chavo.DrawLine(x1, y1, z1, x2, y2, z2, color);
}

/* 0x127970, 156 bytes, global, 10 named locals
 * debug_drawpoint
 * PDB type: void (int, int, int, int, int, u...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void debug_drawpoint(
    int x, int y, int z, int width, int height, uint32_t color)
{
    _svector point = {
        static_cast<int16_t>(x),
        static_cast<int16_t>(y),
        static_cast<int16_t>(z),
        0};
    int screen_xy;

    if (TransformPoints(&point, &screen_xy, 1) == 0) {
        int center_offset = height / 2;
        int screen_x = static_cast<int16_t>(screen_xy) - center_offset;
        int screen_y = static_cast<int16_t>(screen_xy >> 16) - center_offset;

        (void)DrawRectangle(
            static_cast<float>(screen_x),
            static_cast<float>(screen_y),
            static_cast<float>(width),
            static_cast<float>(height),
            static_cast<long>(color));
    }
}

/* 0x127A10, 35 bytes, global, 5 named locals
 * debug_drawpoint2d
 * PDB type: void (int, int, int, int, unsign...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void debug_drawpoint2d(
    int x, int y, int width, int height, uint32_t color)
{
    (void)DrawRectangle(
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height),
        static_cast<long>(color));
}

/* 0x127A40, 200 bytes, global, 9 named locals
 * debug_drawsphere
 * PDB type: void (int, int, int, int, unsign...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
static int jpb_whook_fixed_product(int left, int right)
{
    uint32_t bits = (uint32_t)left * (uint32_t)right;

    if ((bits & UINT32_C(0x80000000)) != 0) {
        bits = ~(~bits >> 12);
    } else {
        bits >>= 12;
    }
    int32_t result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

int cliptoscreen(short *pos)
{
    enum {
        LEFT = 1,
        RIGHT = 2,
        TOP = 4,
        BOTTOM = 8,
        MIN_X = 0x18,
        MAX_X = 0x768,
        MIN_Y = 8,
        MAX_Y = 0x430
    };
    int original_x = pos[0];
    int original_y = pos[1];
    int x = original_x;
    int y = original_y;
    int clipcode = 0;
    int alpha = 0xff;
    int center_delta_x;
    int center_delta_y;
    int y_over_x = 0;
    int x_over_y = 0;
    int vertical_distance;
    int horizontal_distance;

    if (x < MIN_X) {
        clipcode |= LEFT;
    }
    if (x > MAX_X) {
        clipcode |= RIGHT;
    }
    if (y < MIN_Y) {
        clipcode |= TOP;
    }
    if (y > MAX_Y) {
        clipcode |= BOTTOM;
    }
    if (clipcode == 0) {
        return alpha;
    }

    center_delta_y = (OptionStruct.ScreenHeight >> 1) - y;
    center_delta_x = (OptionStruct.ScreenWidth >> 1) - x;
    if (center_delta_x != 0) {
        y_over_x = (center_delta_y * 0x1000) / center_delta_x;
    }

    vertical_distance =
        (clipcode & TOP) != 0 ? MIN_Y - y : y - MAX_Y;
    horizontal_distance =
        (clipcode & LEFT) != 0 ? MIN_X - x : x - MAX_X;
    if (vertical_distance < (horizontal_distance >> 1)) {
        vertical_distance = horizontal_distance >> 1;
    }
    alpha = vertical_distance < 0x80
        ? 0xff - vertical_distance * 2
        : 0;

    if (center_delta_y != 0) {
        x_over_y = (center_delta_x * 0x1000) / center_delta_y;
    }
    if ((clipcode & (LEFT | RIGHT)) != 0 && center_delta_y == 0) {
        x = center_delta_x > 0 ? MIN_X : MAX_X;
        pos[0] = (short)x;
        pos[1] = (short)y;
        return alpha;
    }
    if ((clipcode & (TOP | BOTTOM)) != 0 && center_delta_x == 0) {
        y = center_delta_y > 0 ? MIN_Y : MAX_Y;
        pos[0] = (short)x;
        pos[1] = (short)y;
        return alpha;
    }

    switch (clipcode) {
    case LEFT:
        y += jpb_whook_fixed_product(MIN_X - x, y_over_x);
        x = MIN_X;
        break;
    case RIGHT:
        y += jpb_whook_fixed_product(MAX_X - x, y_over_x);
        x = MAX_X;
        break;
    case TOP:
        y = MIN_Y;
        x = original_x +
            jpb_whook_fixed_product(MIN_Y - original_y, x_over_y);
        break;
    case LEFT | TOP:
        y += jpb_whook_fixed_product(MIN_X - x, y_over_x);
        x = MIN_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MIN_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MIN_Y - original_y, x_over_y);
        }
        break;
    case RIGHT | TOP:
        y += jpb_whook_fixed_product(MAX_X - x, y_over_x);
        x = MAX_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MIN_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MIN_Y - original_y, x_over_y);
        }
        break;
    case BOTTOM:
        y = MAX_Y;
        x = original_x +
            jpb_whook_fixed_product(MAX_Y - original_y, x_over_y);
        break;
    case LEFT | BOTTOM:
        y += jpb_whook_fixed_product(MIN_X - x, y_over_x);
        x = MIN_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MAX_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MAX_Y - original_y, x_over_y);
        }
        break;
    case RIGHT | BOTTOM:
        y += jpb_whook_fixed_product(MAX_X - x, y_over_x);
        x = MAX_X;
        if ((uint32_t)(y - MIN_Y) > (uint32_t)(MAX_Y - MIN_Y)) {
            y = MAX_Y;
            x = original_x +
                jpb_whook_fixed_product(
                    MAX_Y - original_y, x_over_y);
        }
        break;
    default:
        break;
    }
    pos[0] = (short)x;
    pos[1] = (short)y;
    return alpha;
}
void debug_drawsphere(
    int x, int y, int z, int radius, uint32_t color)
{
    /*
     * The matched optimized body retains legacy camera/transform arithmetic
     * but emits no render primitive. Publish the exact authored arguments
     * through a portable renderer seam and otherwise preserve that inert
     * release behavior.
     */
    if (jpb_debug_sphere_hook != nullptr) {
        jpb_debug_sphere_hook(
            jpb_debug_sphere_user_data,
            x,
            y,
            z,
            radius,
            color);
    }
}

/* 0x127B10, 3 bytes, global, 5 named locals
 * debug_line2d
 * PDB type: void (int, int, int, int, unsign...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void debug_line2d(
    int x1, int y1, int x2, int y2, unsigned color)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)color;
}

/* 0x127B20, 18 bytes, global, 1 named locals
 * debug_printf
 * PDB type: int (char*, <no type>)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */

/* 0x127B40, 161 bytes, global, 2 named locals
 * debug_printf1
 * PDB type: int (char*, <no type>)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" int debug_printf1(char *format, ...)
{
    char buffer[256];
    va_list arguments;

    std::memset(buffer, 0, sizeof(buffer));
    va_start(arguments, format);
    std::vsprintf(buffer, format, arguments);
    va_end(arguments);
    OutputDebugStringA(buffer);
    return 0;
}

/* 0x127BF0, 125 bytes, global, 10 named locals
 * debug_vectoroffset
 * PDB type: void (int, int, int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void debug_vectoroffset(
    int x1,
    int y1,
    int z1,
    int x2,
    int y2,
    int z2,
    int shift,
    uint32_t color)
{
    int shift_count = shift & 31;

    chavo.DrawLine(
        x1,
        y1,
        z1,
        x1 + (x2 >> shift_count),
        y1 + (y2 >> shift_count),
        z1 + (z2 >> shift_count),
        color);
}

/* 0x127C70, 21 bytes, global, 1 named locals
 * deserializeGameStruct
 * PDB type: void (unsigned char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void deserializeGameStruct(const void *data)
{
    std::memcpy(&SaveGameStruct, data, sizeof(SaveGameStruct));
}

/* 0x127C90, 33 bytes, global, 2 named locals
 * deserializeOptionStruct
 * PDB type: void (unsigned char*, optionstru...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void deserializeOptionStruct(
    const void *data, optionstruct *options)
{
    std::memcpy(options, data, sizeof(*options));
}

/* 0x127CC0, 166 bytes, global, 6 named locals
 * frontEndPoly
 * PDB type: void (_Material*, int, FRONTENDV...
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void frontEndPoly(
    _Material *material,
    int vertex_count,
    FRONTENDVERT *vertices,
    float depth)
{
    int color = vertices[0].color;
    int index;

    _StartPoly(vertex_count, material);
    for (index = 0; index < vertex_count; ++index) {
        _SetVert(
            index,
            vertices[index].x,
            vertices[index].y,
            depth,
            (uint32_t)color,
            vertices[index].u,
            vertices[index].v);
    }
    _EndPoly();
}

/* 0x127D70, 165 bytes, global, 5 named locals
 * getDefaultResolutionIndex
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" int getDefaultResolutionIndex(void)
{
    if (g_pD3DApp != nullptr && g_pD3DApp->m_pWindow != nullptr) {
        int width;
        int height;

        jpb_D3DAppGetSDLWindowSize(
            g_pD3DApp->m_pWindow, &width, &height);
        (void)jpb_D3DAppSetSDLWindowFullscreen(
            g_pD3DApp->m_pWindow, 0x1001);
        jpb_D3DAppGetSDLWindowSize(
            g_pD3DApp->m_pWindow, &width, &height);
        for (int index = 0; index < g_resolutionsCount; ++index) {
            if (g_resolutions[index].width == width &&
                g_resolutions[index].height == height) {
                return index;
            }
        }
    }
    return 0;
}

/* 0x127E20, 3 bytes, global, 0 named locals
 * initXAstuff
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void initXAstuff(void)
{
}

/* 0x127E30, 1400 bytes, global, 2 named locals
 * platform_completeAchievement
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int platform_completeAchievement(int id)
{
    if (jpb_platform_achievement_hooks.complete != nullptr) {
        return jpb_platform_achievement_hooks.complete(
            id, jpb_platform_achievement_user_data);
    }
    return g_SteamAchievements->SetAchievement(
        GetAchNameFromIndex(id)) ? 1 : 0;
}

/* 0x1283B0, 14 bytes, global, 1 named locals
 * platform_getCompleteAchievement
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int platform_getCompleteAchievement(int id)
{
    if (jpb_platform_achievement_hooks.get_complete != nullptr) {
        return jpb_platform_achievement_hooks.get_complete(
            id, jpb_platform_achievement_user_data);
    }
    return g_SteamAchievements->GetAchievmentStatus(id);
}

/* 0x1283C0, 280 bytes, global, 3 named locals
 * platform_getSystemLanguage
 * PDB type: unsigned char ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" unsigned char platform_getSystemLanguage(void)
{
    char language[96];
    LCID locale = GetUserDefaultLCID();

    if (GetLocaleInfoA(
            locale, LOCALE_SISO639LANGNAME, language, 0x55) == 0) {
        return 0;
    }
    if (std::strcmp(language, "en") == 0) {
        return 0;
    }
    if (std::strcmp(language, "de") == 0) {
        return 1;
    }
    if (std::strcmp(language, "fr") == 0) {
        return 2;
    }
    if (std::strcmp(language, "it") == 0) {
        return 3;
    }
    if (std::strcmp(language, "es") == 0) {
        return 4;
    }
    if (std::strcmp(language, "ru") == 0) {
        return 5;
    }
    return std::strcmp(language, "zh") == 0 ? 6 : 0;
}

/* 0x1284E0, 84 bytes, global, 3 named locals
 * platform_openURL
 * PDB type: int (const char*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" int platform_openURL(const char *url)
{
    char command[256];

    std::snprintf(command, sizeof(command), "start %s", url);
    return std::system(command);
}

/* 0x128540, 42 bytes, global, 0 named locals
 * platform_update
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void platform_update(void)
{
    g_SteamRicherPresence->SetRichPresence(
        GameStruct.inMenuFlag, GameStruct.CurrentLevel);
    SteamAPI_RunCallbacks();
}

/* 0x128570, 3 bytes, global, 2 named locals
 * seecull
 * PDB type: int (FVECTOR4*, FVECTOR4*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
int seecull(FVECTOR4 *point, FVECTOR4 *planes)
{
    (void)point;
    (void)planes;
    return 0;
}

/* 0x128580, 60 bytes, global, 1 named locals
 * serializeGameStruct
 * PDB type: unsigned char* ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void *serializeGameStruct(void)
{
    void *data = std::malloc(sizeof(SaveGameStruct));

    if (data != nullptr) {
        std::memcpy(data, &SaveGameStruct, sizeof(SaveGameStruct));
    }
    return data;
}

/* 0x1285C0, 68 bytes, global, 2 named locals
 * serializeOptionsStruct
 * PDB type: unsigned char* (optionstruct*)
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void *serializeOptionsStruct(const optionstruct *options)
{
    void *data = std::malloc(sizeof(*options));

    if (data != nullptr) {
        std::memcpy(data, options, sizeof(*options));
    }
    return data;
}

/* 0x128610, 3 bytes, global, 0 named locals
 * texture_GarbageCollect
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void texture_GarbageCollect(void)
{
}

/* 0x128620, 12 bytes, global, 0 named locals
 * whook_RestoreTextures
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wHook.cpp
 */
extern "C" void whook_RestoreTextures(void)
{
    (void)RestoreAllTextures(g_pD3DApp->m_pFramework);
}

/* 0x2719C0, 38 bytes, local, 1 named locals
 * `std::filesystem::_Convert_Source_to_wide<char [256],std::filesystem::_Normal_conversion>'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2719F0, 38 bytes, local, 1 named locals
 * `std::filesystem::_Convert_stringoid_to_wide<std::filesystem::_Normal_conversion>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271A20, 38 bytes, local, 1 named locals
 * `std::filesystem::_Convert_wide_to_narrow_replace_chars<std::char_traits<char>,std::allocator<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271A50, 40 bytes, local, 2 named locals
 * `std::vector<CD3DApplication::FBX_MESH *,std::allocator<CD3DApplication::FBX_MESH *> >::_Emplace_reallocate<CD3DApplication::FBX_MESH * const &>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271A80, 70 bytes, local, 2 named locals
 * `std::vector<CD3DApplication::SubMeshSet,std::allocator<CD3DApplication::SubMeshSet> >::_Emplace_reallocate<CD3DApplication::SubMeshSet const &>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271AD0, 43 bytes, local, 1 named locals
 * `std::vector<Vertex,std::allocator<Vertex> >::_Emplace_reallocate<Vertex const &>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271B00, 40 bytes, local, 2 named locals
 * `std::vector<unsigned short,std::allocator<unsigned short> >::_Emplace_reallocate<unsigned short>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271B30, 12 bytes, local, 2 named locals
 * `std::_Lookup_equiv<char,std::regex_traits<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271B40, 12 bytes, local, 2 named locals
 * `std::_Lookup_equiv<char,std::regex_traits<char> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271B50, 41 bytes, local, 2 named locals
 * `std::_Lookup_equiv<char,std::regex_traits<char> >'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271B80, 12 bytes, local, 2 named locals
 * `std::_Lookup_equiv<char,std::regex_traits<char> >'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271B90, 12 bytes, local, 1 named locals
 * `std::_Regex_replace1<std::back_insert_iterator<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,char const *,std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BA0, 12 bytes, local, 1 named locals
 * `std::_Regex_search2<char const *,std::allocator<std::sub_match<char const *> >,char,std::regex_traits<char>,char const *>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BB0, 12 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::_Reset<char const *>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BC0, 16 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::_Reset<char const *>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BD0, 12 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::_Reset<char const *>'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BE0, 12 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::_Reset<char const *>'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271BF0, 40 bytes, local, 1 named locals
 * `std::vector<unsigned int,std::allocator<unsigned int> >::_Resize_reallocate<unsigned int>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271C20, 40 bytes, local, 2 named locals
 * `std::vector<std::_Tgt_state_t<char const *>::_Grp_t,std::allocator<std::_Tgt_state_t<char const *>::_Grp_t> >::_Resize_reallocate<std::_Value_init_tag>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271C50, 40 bytes, local, 2 named locals
 * `std::vector<std::_Loop_vals_t,std::allocator<std::_Loop_vals_t> >::_Resize_reallocate<std::_Value_init_tag>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271C80, 40 bytes, local, 2 named locals
 * `std::vector<std::sub_match<char const *>,std::allocator<std::sub_match<char const *> > >::_Resize_reallocate<std::_Value_init_tag>'::`1'::catch$0
 * PDB type: unknown
 * Source: C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.39.33519\include\vector
 */

/* 0x271CB0, 12 bytes, local, 1 named locals
 * `std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Try_emplace<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const &>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271CC0, 12 bytes, local, 1 named locals
 * `std::_Hash<std::_Umap_traits<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::_Uhash_compare<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::hash<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::equal_to<std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> >,0> >::_Try_emplace<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const &>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271CD0, 38 bytes, local, 1 named locals
 * `std::regex_replace<std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D00, 12 bytes, local, 1 named locals
 * `std::regex_replace<std::regex_traits<char>,char,std::char_traits<char>,std::allocator<char> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D10, 38 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<char *>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D40, 12 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<char *>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D50, 38 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<char const *>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D80, 12 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<char const *>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271D90, 38 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271DC0, 12 bytes, local, 1 named locals
 * `std::_Regex_traits<char>::transform_primary<std::_String_iterator<std::_String_val<std::_Simple_types<char> > > >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271DD0, 12 bytes, local, 0 named locals
 * `std::use_facet<std::collate<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271DE0, 12 bytes, local, 0 named locals
 * `std::use_facet<std::collate<char> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271DF0, 12 bytes, local, 1 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Matcher<char const *,char,std::regex_traits<char>,char const *>'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E00, 16 bytes, local, 1 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Matcher<char const *,char,std::regex_traits<char>,char const *>'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E10, 19 bytes, local, 1 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Matcher<char const *,char,std::regex_traits<char>,char const *>'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E30, 16 bytes, local, 0 named locals
 * `std::_Parser<char const *,char,std::regex_traits<char> >::_Parser<char const *,char,std::regex_traits<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E40, 12 bytes, local, 0 named locals
 * `std::_Parser<char const *,char,std::regex_traits<char> >::_Parser<char const *,char,std::regex_traits<char> >'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E50, 16 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::basic_regex<char,std::regex_traits<char> >'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E60, 16 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::basic_regex<char,std::regex_traits<char> >'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E70, 12 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::basic_regex<char,std::regex_traits<char> >'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E80, 12 bytes, local, 2 named locals
 * `std::basic_regex<char,std::regex_traits<char> >::basic_regex<char,std::regex_traits<char> >'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271E90, 16 bytes, local, 1 named locals
 * `CD3DApplication::SubMeshSet::SubMeshSet'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271EA0, 12 bytes, local, 1 named locals
 * `el_chavo::el_chavo'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271EB0, 12 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271EC0, 16 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271ED0, 16 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271EE0, 12 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271EF0, 16 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F00, 16 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F10, 12 bytes, local, 1 named locals
 * `std::filesystem::filesystem_error::filesystem_error'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F20, 12 bytes, local, 0 named locals
 * `std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::operator[]'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F30, 12 bytes, local, 0 named locals
 * `std::map<std::basic_string<char,std::char_traits<char>,std::allocator<char> >,_Material *,std::less<std::basic_string<char,std::char_traits<char>,std::allocator<char> > >,std::allocator<std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,_Material *> > >::operator[]'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F40, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F50, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F60, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F70, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16Depth'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F80, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16Depth'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271F90, 12 bytes, local, 3 named locals
 * `el_chavo::DrawUITextUTF16Depth'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FA0, 12 bytes, local, 2 named locals
 * `el_chavo::DrawUITextUTF16_3D'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FB0, 12 bytes, local, 2 named locals
 * `el_chavo::DrawUITextUTF16_3D'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FC0, 12 bytes, local, 0 named locals
 * `el_chavo::EndPoly'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FD0, 12 bytes, local, 0 named locals
 * `el_chavo::EndPoly'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FE0, 12 bytes, local, 0 named locals
 * `el_chavo::EndPoly'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x271FF0, 12 bytes, local, 0 named locals
 * `el_chavo::EndPoly'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272000, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272010, 47 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272040, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272050, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272060, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272070, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272080, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$7
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272090, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$8
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2720A0, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2720B0, 47 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$11
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2720E0, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$12
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2720F0, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$13
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272100, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$14
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272110, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$15
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272120, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$16
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272130, 12 bytes, local, 15 named locals
 * `el_chavo::InitFBXLevelData'::`1'::dtor$17
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272140, 12 bytes, local, 0 named locals
 * `el_chavo::NoScaleEndPoly'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272150, 12 bytes, local, 0 named locals
 * `el_chavo::NoScaleEndPoly'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272160, 12 bytes, local, 0 named locals
 * `el_chavo::NoScaleEndPoly'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272170, 12 bytes, local, 0 named locals
 * `el_chavo::NoScaleEndPoly'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272180, 12 bytes, local, 3 named locals
 * `std::_Builder<char const *,char,std::regex_traits<char> >::_Add_equiv'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272190, 12 bytes, local, 0 named locals
 * `std::_Builder<char const *,char,std::regex_traits<char> >::_Begin_assert_group'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721A0, 12 bytes, local, 0 named locals
 * `std::_Parser<char const *,char,std::regex_traits<char> >::_Compile'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721B0, 12 bytes, local, 0 named locals
 * `std::_Parser<char const *,char,std::regex_traits<char> >::_Do_assert_group'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721C0, 12 bytes, local, 2 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_if'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721D0, 12 bytes, local, 2 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_if'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721E0, 12 bytes, local, 2 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_if'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2721F0, 12 bytes, local, 2 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_if'::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272200, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep0'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272210, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep0'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272220, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep0'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272230, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep0'::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272240, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272250, 12 bytes, local, 3 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Do_rep'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272260, 32 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272280, 41 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2722B0, 12 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$3
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2722C0, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$4
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2722D0, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$5
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2722E0, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$6
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2722F0, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$7
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272300, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$8
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272310, 16 bytes, local, 0 named locals
 * `std::collate<char>::_Getcat'::`1'::dtor$9
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272320, 12 bytes, local, 1 named locals
 * `std::_Matcher<char const *,char,std::regex_traits<char>,char const *>::_Match_pat'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272330, 38 bytes, local, 2 named locals
 * `std::filesystem::filesystem_error::_Pretty_message'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272360, 12 bytes, local, 2 named locals
 * `std::filesystem::filesystem_error::_Pretty_message'::`1'::dtor$1
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272370, 12 bytes, local, 2 named locals
 * `std::filesystem::filesystem_error::_Pretty_message'::`1'::dtor$2
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272380, 12 bytes, local, 0 named locals
 * `std::filesystem::_Throw_fs_error'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x272390, 12 bytes, local, 0 named locals
 * `std::filesystem::_Throw_fs_error'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2723A0, 38 bytes, local, 0 named locals
 * `std::collate<char>::do_transform'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2723D0, 12 bytes, local, 2 named locals
 * `std::_System_error_category::message'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x2723E0, 12 bytes, local, 4 named locals
 * `el_chavo::renderVideoFrame'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */

/* 0x279F30, 26 bytes, local, 0 named locals
 * `dynamic atexit destructor for 'chavo''
 * PDB type: void ()
 * Source: no line mapping
 */
