#include "jpb/d3dapp.h"
#include "jpb/d3dframe.h"
#include "jpb/el_chavo.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/menu.h"
#include "jpb/savegame.h"
#include "jpb/steam_achievements.h"
#include "jpb/steam_game_manager.h"
#include "jpb/steam_rich_presence.h"
#include "jpb/text.h"
#include "jpb/whook.h"

#include <cstdlib>
#include <cstdio>
#include <crtdbg.h>
#include <cstring>
#include <filesystem>
#include <vector>

extern "C" int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE previous_instance,
    char *command_line,
    int show_command);

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::fprintf(                                                    \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                             \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int sdl_window_size_calls;
static int sdl_fullscreen_calls;
static int sdl_window_mismatch;
static void *sdl_expected_window;
static std::uint32_t sdl_fullscreen_flags;
static int plot_trans_polys_calls;
static void *plot_trans_polys_device;
static unsigned plot_trans_polys_argument;
static int sdl_free_calls;
static int whook_sdl_calls[16];
static int whook_sdl_call_count;
static void *whook_sdl_renderer;
static void *whook_sdl_surface;
static void *whook_sdl_texture;
static const void *whook_sdl_source;
static SDL_Rect whook_sdl_source_rect;
static int whook_sdl_source_present;
static SDL_Rect whook_sdl_destination;
static std::uint8_t whook_sdl_red;
static std::uint8_t whook_sdl_green;
static std::uint8_t whook_sdl_blue;
static std::uint8_t whook_sdl_alpha;
static int whook_sdl_blend_mode;
static double whook_sdl_angle;
static const void *whook_sdl_center;
static int whook_sdl_flip;
static int whook_sdl_base_path_calls;
static char whook_sdl_base_path[] = "jpb_whook_persistence_test\\";

enum {
    JPB_TEST_WINMAIN_UPDATE_MENUS = 1,
    JPB_TEST_WINMAIN_LOAD_OPTIONS,
    JPB_TEST_WINMAIN_SET_SIZE,
    JPB_TEST_WINMAIN_STEAM_INIT,
    JPB_TEST_WINMAIN_CREATE,
    JPB_TEST_WINMAIN_RESTART,
    JPB_TEST_WINMAIN_CREATE_ACHIEVEMENTS,
    JPB_TEST_WINMAIN_CREATE_GAME_MANAGER,
    JPB_TEST_WINMAIN_CREATE_RICH_PRESENCE,
    JPB_TEST_WINMAIN_IS_STEAM_DECK,
    JPB_TEST_WINMAIN_INITIALIZE_MAIN,
    JPB_TEST_WINMAIN_SHUTDOWN,
    JPB_TEST_WINMAIN_DESTROY_ACHIEVEMENTS,
    JPB_TEST_WINMAIN_REPORT_ERROR,
    JPB_TEST_WINMAIN_PRINT_FATAL
};

static int win_main_calls[32];
static int win_main_call_count;
static int win_main_steam_init_result;
static std::int32_t win_main_create_result;
static int win_main_restart_result;
static int win_main_steam_deck_result;
static std::uint32_t win_main_width;
static std::uint32_t win_main_height;
static std::uint32_t win_main_app_id;
static void *win_main_instance;
static char *win_main_command_line;
static void *win_main_achievement_data;
static int win_main_achievement_count;
static void *win_main_destroyed_achievements;
static std::uint32_t win_main_error_result;
static std::uint32_t win_main_error_line;
static char win_main_error_message[64];
static char win_main_error_file[128];
static char win_main_fatal_message[128];

static void record_win_main_call(int call)
{
    win_main_calls[win_main_call_count++] = call;
}

static void test_win_main_update_menus(void)
{
    record_win_main_call(JPB_TEST_WINMAIN_UPDATE_MENUS);
}

static void test_win_main_load_options(void)
{
    record_win_main_call(JPB_TEST_WINMAIN_LOAD_OPTIONS);
}

static void test_win_main_set_size(
    std::uint32_t width, std::uint32_t height)
{
    record_win_main_call(JPB_TEST_WINMAIN_SET_SIZE);
    win_main_width = width;
    win_main_height = height;
}

static int test_win_main_steam_init(void)
{
    record_win_main_call(JPB_TEST_WINMAIN_STEAM_INIT);
    return win_main_steam_init_result;
}

static std::int32_t test_win_main_create(
    void *instance, char *command_line)
{
    record_win_main_call(JPB_TEST_WINMAIN_CREATE);
    win_main_instance = instance;
    win_main_command_line = command_line;
    return win_main_create_result;
}

static int test_win_main_restart(std::uint32_t app_id)
{
    record_win_main_call(JPB_TEST_WINMAIN_RESTART);
    win_main_app_id = app_id;
    return win_main_restart_result;
}

static void *test_win_main_create_achievements(
    void *achievements, int count)
{
    record_win_main_call(JPB_TEST_WINMAIN_CREATE_ACHIEVEMENTS);
    win_main_achievement_data = achievements;
    win_main_achievement_count = count;
    return reinterpret_cast<void *>(static_cast<std::uintptr_t>(0x1111));
}

static void *test_win_main_create_game_manager(void)
{
    record_win_main_call(JPB_TEST_WINMAIN_CREATE_GAME_MANAGER);
    return reinterpret_cast<void *>(static_cast<std::uintptr_t>(0x2222));
}

static void *test_win_main_create_rich_presence(void)
{
    record_win_main_call(JPB_TEST_WINMAIN_CREATE_RICH_PRESENCE);
    return reinterpret_cast<void *>(static_cast<std::uintptr_t>(0x3333));
}

static int test_win_main_is_steam_deck(void)
{
    record_win_main_call(JPB_TEST_WINMAIN_IS_STEAM_DECK);
    return win_main_steam_deck_result;
}

static void test_win_main_initialize(void)
{
    record_win_main_call(JPB_TEST_WINMAIN_INITIALIZE_MAIN);
}

static void test_win_main_shutdown(void)
{
    record_win_main_call(JPB_TEST_WINMAIN_SHUTDOWN);
}

static void test_win_main_destroy_achievements(void *achievements)
{
    record_win_main_call(JPB_TEST_WINMAIN_DESTROY_ACHIEVEMENTS);
    win_main_destroyed_achievements = achievements;
}

static void test_win_main_report_error(
    std::uint32_t result,
    char *message,
    std::uint32_t line,
    char *file)
{
    record_win_main_call(JPB_TEST_WINMAIN_REPORT_ERROR);
    win_main_error_result = result;
    win_main_error_line = line;
    std::snprintf(
        win_main_error_message, sizeof(win_main_error_message), "%s", message);
    std::snprintf(
        win_main_error_file, sizeof(win_main_error_file), "%s", file);
}

static void test_win_main_print_fatal(const char *message)
{
    record_win_main_call(JPB_TEST_WINMAIN_PRINT_FATAL);
    std::snprintf(
        win_main_fatal_message, sizeof(win_main_fatal_message), "%s", message);
}

static const JPBWHookWinMainTestHooks win_main_hooks = {
    test_win_main_update_menus,
    test_win_main_load_options,
    test_win_main_set_size,
    test_win_main_steam_init,
    test_win_main_create,
    test_win_main_restart,
    test_win_main_create_achievements,
    test_win_main_create_game_manager,
    test_win_main_create_rich_presence,
    test_win_main_is_steam_deck,
    test_win_main_initialize,
    test_win_main_shutdown,
    test_win_main_destroy_achievements,
    test_win_main_report_error,
    test_win_main_print_fatal,
};

enum {
    JPB_TEST_SDL_CREATE_TEXTURE = 1,
    JPB_TEST_SDL_RENDER_COPY = 2,
    JPB_TEST_SDL_DESTROY_TEXTURE = 3,
    JPB_TEST_SDL_FREE_SURFACE = 4,
    JPB_TEST_SDL_CLEAR_CLIP = 5,
    JPB_TEST_SDL_COLOR = 6,
    JPB_TEST_SDL_ALPHA = 7,
    JPB_TEST_SDL_BLEND = 8,
    JPB_TEST_SDL_RENDER_COPY_EX = 9
};

static void record_whook_sdl_call(int call)
{
    whook_sdl_calls[whook_sdl_call_count++] = call;
}

static void *test_whook_create_texture(void *renderer, void *surface)
{
    record_whook_sdl_call(JPB_TEST_SDL_CREATE_TEXTURE);
    whook_sdl_renderer = renderer;
    whook_sdl_surface = surface;
    return whook_sdl_texture;
}

static int test_whook_render_copy(
    void *renderer,
    void *texture,
    const void *source,
    const void *destination)
{
    record_whook_sdl_call(JPB_TEST_SDL_RENDER_COPY);
    whook_sdl_renderer = renderer;
    whook_sdl_texture = texture;
    whook_sdl_source = source;
    whook_sdl_source_present = source != nullptr;
    if (source != nullptr) {
        std::memcpy(
            &whook_sdl_source_rect, source, sizeof(whook_sdl_source_rect));
    }
    std::memcpy(
        &whook_sdl_destination, destination, sizeof(whook_sdl_destination));
    return 37;
}

static void test_whook_destroy_texture(void *texture)
{
    record_whook_sdl_call(JPB_TEST_SDL_DESTROY_TEXTURE);
    whook_sdl_texture = texture;
}

static void test_whook_free_surface(void *surface)
{
    record_whook_sdl_call(JPB_TEST_SDL_FREE_SURFACE);
    whook_sdl_surface = surface;
}

static int test_whook_clear_clip(void *renderer, const void *rectangle)
{
    record_whook_sdl_call(JPB_TEST_SDL_CLEAR_CLIP);
    whook_sdl_renderer = renderer;
    whook_sdl_source = rectangle;
    whook_sdl_source_present = rectangle != nullptr;
    return 41;
}

static int test_whook_color(
    void *texture,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue)
{
    record_whook_sdl_call(JPB_TEST_SDL_COLOR);
    whook_sdl_texture = texture;
    whook_sdl_red = red;
    whook_sdl_green = green;
    whook_sdl_blue = blue;
    return 43;
}

static int test_whook_alpha(void *texture, std::uint8_t alpha)
{
    record_whook_sdl_call(JPB_TEST_SDL_ALPHA);
    whook_sdl_texture = texture;
    whook_sdl_alpha = alpha;
    return 47;
}

static int test_whook_blend(void *texture, int blend_mode)
{
    record_whook_sdl_call(JPB_TEST_SDL_BLEND);
    whook_sdl_texture = texture;
    whook_sdl_blend_mode = blend_mode;
    return 53;
}

static int test_whook_render_copy_ex(
    void *renderer,
    void *texture,
    const void *source,
    const void *destination,
    double angle,
    const void *center,
    int flip)
{
    record_whook_sdl_call(JPB_TEST_SDL_RENDER_COPY_EX);
    whook_sdl_renderer = renderer;
    whook_sdl_texture = texture;
    whook_sdl_source = source;
    whook_sdl_source_present = source != nullptr;
    if (source != nullptr) {
        std::memcpy(
            &whook_sdl_source_rect, source, sizeof(whook_sdl_source_rect));
    }
    std::memcpy(
        &whook_sdl_destination, destination, sizeof(whook_sdl_destination));
    whook_sdl_angle = angle;
    whook_sdl_center = center;
    whook_sdl_flip = flip;
    return 59;
}

static char *test_whook_get_base_path(void)
{
    ++whook_sdl_base_path_calls;
    return whook_sdl_base_path;
}

static const JPBWHookSDLTestHooks whook_sdl_hooks = {
    test_whook_create_texture,
    test_whook_render_copy,
    test_whook_destroy_texture,
    test_whook_free_surface,
    test_whook_clear_clip,
    test_whook_color,
    test_whook_alpha,
    test_whook_blend,
    test_whook_render_copy_ex,
    test_whook_get_base_path,
};

static void test_sdl_free(void *memory)
{
    ++sdl_free_calls;
    std::free(memory);
}

static void test_plot_trans_polys(void *device, unsigned argument)
{
    ++plot_trans_polys_calls;
    plot_trans_polys_device = device;
    plot_trans_polys_argument = argument;
}

static void test_sdl_get_window_size(
    void *window, int *width, int *height)
{
    if (window != sdl_expected_window) {
        sdl_window_mismatch = 1;
    }
    ++sdl_window_size_calls;
    if (sdl_window_size_calls == 1) {
        *width = 1280;
        *height = 720;
    } else {
        *width = 1920;
        *height = 1080;
    }
}

static int test_sdl_set_window_fullscreen(
    void *window, std::uint32_t flags)
{
    if (window != sdl_expected_window) {
        sdl_window_mismatch = 1;
    }
    ++sdl_fullscreen_calls;
    sdl_fullscreen_flags = flags;
    return 0;
}

static int test_key_edges(void)
{
    jpb_WHookClearKeyState();

    jpb_WHookHandleKeyEvent(0x26, 0, 1);
    CHECK(KeyHeld(0x1e) == 1);
    CHECK(KeyPressed(0x1e) == 1);
    CHECK(KeyReleased(0x1e) == 0);
    CHECK(LastKey() == 0x1e);

    jpb_WHookEndInputFrame();
    CHECK(KeyHeld(0x1e) == 1);
    CHECK(KeyPressed(0x1e) == 0);
    CHECK(KeyReleased(0x1e) == 0);
    CHECK(LastKey() == 0);

    jpb_WHookHandleKeyEvent(0x26, 0, 0);
    CHECK(KeyHeld(0x1e) == 0);
    CHECK(KeyPressed(0x1e) == 0);
    CHECK(KeyReleased(0x1e) == 1);

    jpb_WHookEndInputFrame();
    CHECK(KeyReleased(0x1e) == 0);

    jpb_WHookHandleKeyEvent(0x20, 0, 1);
    CHECK(KeyHeld(0x20) == 1);
    CHECK(KeyPressed(0x20) == 1);
    CHECK(LastKey() == 0x20);
    jpb_WHookEndInputFrame();
    jpb_WHookHandleKeyEvent(0x20, 0, 0);
    CHECK(KeyHeld(0x20) == 0);
    CHECK(KeyReleased(0x20) == 1);

    jpb_WHookClearKeyState();
    CHECK(KeyReleased(0x20) == 0);
    return 0;
}

static int test_clip_rect(void)
{
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    SDL_SetClip(10, 20, 30, 40);
    CHECK(jpb_TextGetClipRect(&left, &top, &right, &bottom) == 1);
    CHECK(left == 10);
    CHECK(top == 20);
    CHECK(right == 40);
    CHECK(bottom == 60);

    SDL_ResetClipRect();
    CHECK(jpb_TextGetClipRect(&left, &top, &right, &bottom) == 0);
    return 0;
}

static int test_canonical_serializers(void)
{
    saveGameStruct saved_game = {};
    optionstruct saved_options = {};
    saveGameStruct restored_game = {};
    optionstruct restored_options = {};
    void *game_bytes;
    void *option_bytes;
    size_t index;

    for (index = 0; index < sizeof(saved_game); ++index) {
        reinterpret_cast<unsigned char *>(&saved_game)[index] =
            static_cast<unsigned char>(index * 29u + 7u);
    }
    for (index = 0; index < sizeof(saved_options); ++index) {
        reinterpret_cast<unsigned char *>(&saved_options)[index] =
            static_cast<unsigned char>(index * 13u + 3u);
    }

    SaveGameStruct = saved_game;
    game_bytes = serializeGameStruct();
    CHECK(game_bytes != nullptr);
    CHECK(std::memcmp(game_bytes, &saved_game, sizeof(saved_game)) == 0);
    std::memset(&SaveGameStruct, 0, sizeof(SaveGameStruct));
    deserializeGameStruct(game_bytes);
    restored_game = SaveGameStruct;
    CHECK(std::memcmp(&restored_game, &saved_game, sizeof(saved_game)) == 0);

    option_bytes = serializeOptionsStruct(&saved_options);
    CHECK(option_bytes != nullptr);
    CHECK(option_bytes != &saved_options);
    CHECK(std::memcmp(
        option_bytes, &saved_options, sizeof(saved_options)) == 0);
    deserializeOptionStruct(option_bytes, &restored_options);
    CHECK(std::memcmp(
        &restored_options, &saved_options, sizeof(saved_options)) == 0);

    std::free(option_bytes);
    std::free(game_bytes);
    return 0;
}

static int test_set_in_menu(void)
{
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    CD3DApplication *previous = g_pD3DApp;

    application->m_pFramework = framework;
    g_pD3DApp = application;
    SetInMenu(1);
    CHECK(framework->inMenu);
    SetInMenu(2);
    CHECK(!framework->inMenu);
    SetInMenu(0);
    CHECK(!framework->inMenu);
    g_pD3DApp = previous;
    return 0;
}

static int test_default_resolution_index(void)
{
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    CD3DApplication *previous_application = g_pD3DApp;
    RESOLUTION saved_resolutions[3];
    int saved_count = g_resolutionsCount;

    std::memcpy(saved_resolutions, g_resolutions, sizeof(saved_resolutions));
    sdl_expected_window = reinterpret_cast<void *>(
        static_cast<std::uintptr_t>(0x12345678));
    application->m_pWindow = reinterpret_cast<SDL_Window *>(
        sdl_expected_window);
    g_pD3DApp = application;
    g_resolutionsCount = 3;
    g_resolutions[0] = {1280, 720};
    g_resolutions[1] = {1600, 900};
    g_resolutions[2] = {1920, 1080};
    sdl_window_size_calls = 0;
    sdl_fullscreen_calls = 0;
    sdl_window_mismatch = 0;
    sdl_fullscreen_flags = 0;
    jpb_d3dapp_set_sdl_get_window_size_test_hook(
        test_sdl_get_window_size);
    jpb_d3dapp_set_sdl_set_window_fullscreen_test_hook(
        test_sdl_set_window_fullscreen);

    CHECK(getDefaultResolutionIndex() == 2);
    CHECK(sdl_window_size_calls == 2);
    CHECK(sdl_fullscreen_calls == 1);
    CHECK(sdl_window_mismatch == 0);
    CHECK(sdl_fullscreen_flags == 0x1001);

    application->m_pWindow = nullptr;
    CHECK(getDefaultResolutionIndex() == 0);
    CHECK(sdl_window_size_calls == 2);
    CHECK(sdl_fullscreen_calls == 1);

    jpb_d3dapp_set_sdl_get_window_size_test_hook(nullptr);
    jpb_d3dapp_set_sdl_set_window_fullscreen_test_hook(nullptr);
    std::memcpy(g_resolutions, saved_resolutions, sizeof(saved_resolutions));
    g_resolutionsCount = saved_count;
    g_pD3DApp = previous_application;
    return 0;
}

static int test_filename_helpers(void)
{
    char dotted_source[32] = "folder/ship.pvr";
    char plain_source[32] = "plain";
    char destination[32];
    char *modified;

    CHECK(!IsNullTerminated(nullptr));
    CHECK(!IsNullTerminated(""));
    CHECK(!IsNullTerminated("abc"));

    modified = ModifyFilename("models/ship.sgi");
    CHECK(std::strcmp(modified, "models/ship.tim") == 0);
    delete[] modified;
    modified = ModifyFilename("textures/panel.pvr");
    CHECK(std::strcmp(modified, "textures/panel.tga") == 0);
    delete[] modified;
    modified = ModifyFilename("textures/panel.png");
    CHECK(std::strcmp(modified, "textures/panel.png") == 0);
    delete[] modified;

    std::memset(destination, '?', sizeof(destination));
    SetFilenameExtension(dotted_source, destination, ".tga");
    CHECK(std::strcmp(destination, "folder/ship.tga") == 0);

    std::memset(destination, '?', sizeof(destination));
    SetFilenameExtension(plain_source, destination, ".tim");
    CHECK(std::memcmp(destination, "plain", 5) == 0);
    CHECK(destination[5] == '?');
    CHECK(std::strcmp(plain_source, "plaintim") == 0);
    return 0;
}

static int test_valid_resolutions(void)
{
    DEVMODEA current_mode = {};

    current_mode.dmSize = sizeof(current_mode);
    CHECK(EnumDisplaySettingsA(
        nullptr, ENUM_CURRENT_SETTINGS, &current_mode) != FALSE);
    UpdateValidResolutions();
    CHECK(g_resolutionsCount > 0);
    CHECK(g_resolutionsCount <= 256);

    for (int index = 0; index < g_resolutionsCount; ++index) {
        CHECK(g_resolutions[index].width >=
              static_cast<int32_t>(current_mode.dmPelsWidth));
        CHECK(g_resolutions[index].height >=
              static_cast<int32_t>(current_mode.dmPelsHeight));
        for (int previous = 0; previous < index; ++previous) {
            CHECK(g_resolutions[index].width !=
                      g_resolutions[previous].width ||
                  g_resolutions[index].height !=
                      g_resolutions[previous].height);
        }
        if (index > 0) {
            CHECK(g_resolutions[index - 1].height >
                      g_resolutions[index].height ||
                  (g_resolutions[index - 1].height ==
                       g_resolutions[index].height &&
                   g_resolutions[index - 1].width >=
                       g_resolutions[index].width));
        }
    }
    return 0;
}

static int test_debug_formatting_wrappers(void)
{
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    CD3DApplication *previous = g_pD3DApp;

    g_pD3DApp = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    OutputTextXY(12, 34, const_cast<char *>("xy:%d:%s"), 56, "ok");
    dbgprintf(const_cast<char *>("text:%08x"), 0x1234);
    __PCTrace(const_cast<char *>("trace:%d\n"), 7);
    CHECK(debug_printf1(
        const_cast<char *>("debug:%s\n"), "ok") == 0);
    g_pD3DApp = previous;
    return 0;
}

static int test_achievement_names(void)
{
    char expected[16];

    CHECK(std::strcmp(GetAchNameFromIndex(-1), "") == 0);
    CHECK(std::strcmp(GetAchNameFromIndex(0), "") == 0);
    for (int index = 1; index <= 43; ++index) {
        std::snprintf(
            expected, sizeof(expected), "JPB_Trophy_%03d", index);
        CHECK(std::strcmp(GetAchNameFromIndex(index), expected) == 0);
    }
    CHECK(std::strcmp(GetAchNameFromIndex(44), "") == 0);
    return 0;
}

static int test_fbx_cleanup_owner(void)
{
    std::vector<CD3DApplication::FBX_MESH *> meshes;
    auto *first = new CD3DApplication::FBX_MESH{};
    auto *second = new CD3DApplication::FBX_MESH{};

    first->subMeshes.emplace_back();
    first->subMeshes[0].vertices.resize(2);
    first->subMeshes[0].subMeshIndices = {0, 1, 0};
    second->subMeshes.emplace_back();
    second->subMeshes[0].vertices.resize(1);
    second->subMeshes[0].subMeshIndices = {0};
    meshes.push_back(first);
    meshes.push_back(second);

    chavo.CleanupFBXData(meshes);
    CHECK(meshes.empty());
    return 0;
}

static int test_transparent_polygon_dispatch(void)
{
    void *device_vtable[16] = {};
    struct FakeDevice {
        void **vtable;
    } device = {device_vtable};
    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    CD3DFramework12 *previous_framework = chavo.m_pFramework;

    device_vtable[15] = reinterpret_cast<void *>(test_plot_trans_polys);
    framework->m_pDevice = reinterpret_cast<ID3D12Device *>(&device);
    chavo.m_pFramework = framework;
    plot_trans_polys_calls = 0;
    plot_trans_polys_device = nullptr;
    plot_trans_polys_argument = 1;

    chavo.PlotTransPolys();
    CHECK(plot_trans_polys_calls == 1);
    CHECK(plot_trans_polys_device == &device);
    CHECK(plot_trans_polys_argument == 0);

    chavo.m_pFramework = previous_framework;
    return 0;
}

static AudioQueue *make_audio_queue(
    int channels, int frames, const float *source)
{
    auto *samples = static_cast<float *>(
        std::malloc(
            static_cast<std::size_t>(channels * frames) * sizeof(float)));
    auto *packet = static_cast<THEORAPLAY_AudioPacket *>(
        std::calloc(1, sizeof(THEORAPLAY_AudioPacket)));
    auto *queue = static_cast<AudioQueue *>(
        std::calloc(1, sizeof(AudioQueue)));

    if (samples == nullptr || packet == nullptr || queue == nullptr) {
        std::free(samples);
        std::free(packet);
        std::free(queue);
        return nullptr;
    }
    std::memcpy(
        samples,
        source,
        static_cast<std::size_t>(channels * frames) * sizeof(float));
    packet->channels = channels;
    packet->frames = frames;
    packet->samples = samples;
    queue->audio = packet;
    return queue;
}

static int test_audio_callback(void)
{
    const float clipped_samples[] = {0.5f, -0.5f, 2.0f, -2.0f};
    const float partial_samples[] = {0.25f, 0.5f, 0.75f, 1.0f};
    unsigned char output[12];
    std::int16_t converted[4];
    AudioQueue *queue;
    const optionstruct previous_options = OptionStruct;
    const float previous_video_volume = VideoVolume;

    jpb_d3dapp_set_sdl_free_test_hook(test_sdl_free);
    sdl_free_calls = 0;
    OptionStruct.musicVolume = 128;
    VideoVolume = 1.0f;

    queue = make_audio_queue(2, 2, clipped_samples);
    CHECK(queue != nullptr);
    std::memset(output, 0xcc, sizeof(output));
    jpb_WHookSetAudioQueueForTest(queue, queue);
    jpb_WHookAudioCallbackForTest(nullptr, output, sizeof(output));
    std::memcpy(converted, output, sizeof(converted));
    CHECK(converted[0] == 16383);
    CHECK(converted[1] == -16383);
    CHECK(converted[2] == INT16_MAX);
    CHECK(converted[3] == INT16_MIN);
    CHECK(output[8] == 0);
    CHECK(output[9] == 0);
    CHECK(output[10] == 0);
    CHECK(output[11] == 0);
    CHECK(jpb_WHookAudioQueueForTest() == nullptr);
    CHECK(jpb_WHookAudioQueueTailForTest() == nullptr);
    CHECK(sdl_free_calls == 1);

    queue = make_audio_queue(2, 2, partial_samples);
    CHECK(queue != nullptr);
    jpb_WHookSetAudioQueueForTest(queue, queue);
    jpb_WHookAudioCallbackForTest(nullptr, output, 2);
    std::memcpy(converted, output, sizeof(converted[0]));
    CHECK(converted[0] == 8191);
    CHECK(queue->offset == 0);
    CHECK(jpb_WHookAudioQueueForTest() == queue);
    CHECK(jpb_WHookAudioQueueTailForTest() == queue);

    std::memset(output, 0xcc, sizeof(output));
    jpb_WHookAudioCallbackForTest(nullptr, output, 8);
    std::memcpy(converted, output, sizeof(converted));
    CHECK(converted[0] == 8191);
    CHECK(converted[1] == 16383);
    CHECK(converted[2] == 24575);
    CHECK(converted[3] == 32767);
    CHECK(jpb_WHookAudioQueueForTest() == nullptr);
    CHECK(jpb_WHookAudioQueueTailForTest() == nullptr);
    CHECK(sdl_free_calls == 2);

    OptionStruct = previous_options;
    VideoVolume = previous_video_volume;
    jpb_d3dapp_set_sdl_free_test_hook(nullptr);
    return 0;
}

static int test_localized_video_paths(void)
{
    static const char *const expected[] = {
        "movies/English/photo_English.ogg",
        "movies/German/photo_German.ogg",
        "movies/French/photo_French.ogg",
        "movies/Italian/photo_Italian.ogg",
        "movies/Spanish/photo_Spanish.ogg",
        "movies/Russian/photo_Russian.ogg",
        "movies/Simplified Chinese/photo_Simplified Chinese.ogg",
    };
    const optionstruct previous_options = OptionStruct;

    for (std::uint8_t language = 0; language <= 6; ++language) {
        OptionStruct.Language = language;
        CHECK(std::strcmp(
                  jpb_WHookLocalizedVideoPathForTest(
                      "movies/English/photo_English.ogg"),
                  expected[language]) == 0);
    }
    OptionStruct.Language = 7;
    CHECK(std::strcmp(
              jpb_WHookLocalizedVideoPathForTest(
                  "movies/English/photo_English.ogg"),
              expected[0]) == 0);
    OptionStruct.Language = 4;
    CHECK(std::strcmp(
              jpb_WHookLocalizedVideoPathForTest(
                  "movies/Aspyr_Logo_1080_Flipped.ogg"),
              "movies/Aspyr_Logo_1080_Flipped.ogg") == 0);

    OptionStruct = previous_options;
    return 0;
}

static int test_legacy_sdl_ui_owners(void)
{
    std::vector<unsigned char> framework_storage(
        sizeof(CD3DFramework12), 0);
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage.data());
    std::vector<unsigned char> texture_storage(416, 0);
    CD3DFramework12 *previous_framework = chavo.m_pFramework;
    void *const renderer = reinterpret_cast<void *>(UINT64_C(0x12340000));
    void *const texture = reinterpret_cast<void *>(UINT64_C(0x56780000));
    SDL_Surface surface = {};
    _Material material = {};
    const SDL_Rect destination = {11, 22, 33, 44};
    SDL_Rect source = {55, 66, 0, 88};

    framework->m_pSDLRenderer = reinterpret_cast<SDL_Renderer *>(renderer);
    chavo.m_pFramework = framework;
    jpb_WHookSetSDLTestHooks(&whook_sdl_hooks);

    whook_sdl_call_count = 0;
    whook_sdl_texture = texture;
    RenderUIText(101, 102, 103, 104, &surface);
    CHECK(whook_sdl_call_count == 4);
    CHECK(whook_sdl_calls[0] == JPB_TEST_SDL_CREATE_TEXTURE);
    CHECK(whook_sdl_calls[1] == JPB_TEST_SDL_RENDER_COPY);
    CHECK(whook_sdl_calls[2] == JPB_TEST_SDL_DESTROY_TEXTURE);
    CHECK(whook_sdl_calls[3] == JPB_TEST_SDL_FREE_SURFACE);
    CHECK(whook_sdl_renderer == renderer);
    CHECK(whook_sdl_surface == &surface);
    CHECK(whook_sdl_texture == texture);
    CHECK(whook_sdl_source == nullptr);
    CHECK(whook_sdl_source_present == 0);
    CHECK(whook_sdl_destination.x == 101);
    CHECK(whook_sdl_destination.y == 102);
    CHECK(whook_sdl_destination.w == 103);
    CHECK(whook_sdl_destination.h == 104);

    *reinterpret_cast<void **>(texture_storage.data() + 0x188) = texture;
    material.texture = texture_storage.data();
    whook_sdl_call_count = 0;
    RenderUITexture(
        &material,
        destination,
        source,
        UINT8_C(0x9a),
        0x123,
        -1,
        0x102,
        2);
    CHECK(whook_sdl_call_count == 5);
    CHECK(whook_sdl_calls[0] == JPB_TEST_SDL_CLEAR_CLIP);
    CHECK(whook_sdl_calls[1] == JPB_TEST_SDL_COLOR);
    CHECK(whook_sdl_calls[2] == JPB_TEST_SDL_ALPHA);
    CHECK(whook_sdl_calls[3] == JPB_TEST_SDL_BLEND);
    CHECK(whook_sdl_calls[4] == JPB_TEST_SDL_RENDER_COPY_EX);
    CHECK(whook_sdl_renderer == renderer);
    CHECK(whook_sdl_texture == texture);
    CHECK(whook_sdl_source == nullptr);
    CHECK(whook_sdl_source_present == 0);
    CHECK(whook_sdl_red == UINT8_C(0x23));
    CHECK(whook_sdl_green == UINT8_C(0xff));
    CHECK(whook_sdl_blue == UINT8_C(0x02));
    CHECK(whook_sdl_alpha == UINT8_C(0x9a));
    CHECK(whook_sdl_blend_mode == 1);
    CHECK(whook_sdl_destination.x == destination.x);
    CHECK(whook_sdl_destination.y == destination.y);
    CHECK(whook_sdl_destination.w == destination.w);
    CHECK(whook_sdl_destination.h == destination.h);
    CHECK(whook_sdl_angle == 0.0);
    CHECK(whook_sdl_center == nullptr);
    CHECK(whook_sdl_flip == 2);

    source.w = 77;
    whook_sdl_call_count = 0;
    RenderUITexture(&material, destination, source, 1, 2, 3, 4, 0);
    CHECK(whook_sdl_source_present == 1);
    CHECK(whook_sdl_source_rect.x == source.x);
    CHECK(whook_sdl_source_rect.w == source.w);

    *reinterpret_cast<void **>(texture_storage.data() + 0x188) = nullptr;
    whook_sdl_call_count = 0;
    RenderUITexture(&material, destination, source, 1, 2, 3, 4, 0);
    CHECK(whook_sdl_call_count == 1);
    CHECK(whook_sdl_calls[0] == JPB_TEST_SDL_CLEAR_CLIP);

    jpb_WHookSetSDLTestHooks(nullptr);
    chavo.m_pFramework = previous_framework;
    return 0;
}

static int read_exact_file(
    const std::filesystem::path &path, void *data, std::size_t size)
{
    FILE *file = std::fopen(path.string().c_str(), "rb");
    std::size_t read_count;

    if (file == nullptr) {
        return 0;
    }
    read_count = std::fread(data, 1, size, file);
    if (std::fclose(file) != 0) {
        return 0;
    }
    return read_count == size;
}

static int test_canonical_persistence_owners(void)
{
    const std::filesystem::path base_path =
        "jpb_whook_persistence_test";
    const std::filesystem::path save_directory =
        base_path / "SAVEDATA0";
    const std::filesystem::path game_path = save_directory / "Game";
    const std::filesystem::path options_path = save_directory / "Options";
    const gamestruct previous_game = GameStruct;
    const saveGameStruct previous_save = SaveGameStruct;
    const optionstruct previous_options = OptionStruct;
    std::uint8_t previous_global_bits[sizeof(abGlobalBits)];
    RESOLUTION previous_resolutions[2];
    const int previous_resolution_count = g_resolutionsCount;
    CD3DApplication *const previous_application = g_pD3DApp;
    std::vector<unsigned char> application_storage(
        sizeof(CD3DApplication), 0);
    auto *application = reinterpret_cast<CD3DApplication *>(
        application_storage.data());
    optionstruct settings = defaultOptionStruct;
    optionstruct stored_settings = {};
    saveGameStruct stored_game = {};

    std::memcpy(
        previous_resolutions,
        g_resolutions,
        sizeof(previous_resolutions));
    std::memcpy(
        previous_global_bits,
        abGlobalBits,
        sizeof(previous_global_bits));
    (void)std::filesystem::remove_all(base_path);
    CHECK(std::filesystem::create_directory(base_path));
    jpb_WHookSetSDLTestHooks(&whook_sdl_hooks);

    settings.Music = 0;
    settings.Stereo = 0;
    settings.musicVolume = 17;
    settings.SFXVolume = 23;
    settings.Language = 4;
    settings.ResolutionChanged = 1;
    settings.ScreenWidth = 1280;
    settings.ScreenHeight = 720;
    settings.WindowMode = 2;
    whook_sdl_base_path_calls = 0;
    SaveSettingsData(settings);
    CHECK(whook_sdl_base_path_calls == 2);
    CHECK(std::filesystem::file_size(options_path) == sizeof(optionstruct));
    CHECK(read_exact_file(
        options_path, &stored_settings, sizeof(stored_settings)));
    CHECK(std::memcmp(
        &stored_settings, &settings, sizeof(stored_settings)) == 0);

    g_resolutionsCount = 1;
    g_resolutions[0] = {1920, 1080};
    OptionStruct = defaultOptionStruct;
    whook_sdl_base_path_calls = 0;
    LoadOptionsData();
    CHECK(whook_sdl_base_path_calls == 2);
    CHECK(OptionStruct.Music == settings.Music);
    CHECK(OptionStruct.Stereo == settings.Stereo);
    CHECK(OptionStruct.musicVolume == settings.musicVolume);
    CHECK(OptionStruct.Language == settings.Language);
    CHECK(OptionStruct.ScreenWidth == settings.ScreenWidth);
    CHECK(OptionStruct.ScreenHeight == settings.ScreenHeight);
    CHECK(OptionStruct.WindowMode == settings.WindowMode);

    CHECK(std::filesystem::remove(options_path));
    application->m_pWindow = reinterpret_cast<SDL_Window *>(
        static_cast<std::uintptr_t>(0x12345678));
    sdl_expected_window = application->m_pWindow;
    g_pD3DApp = application;
    g_resolutionsCount = 2;
    g_resolutions[0] = {1280, 720};
    g_resolutions[1] = {1920, 1080};
    sdl_window_size_calls = 0;
    sdl_fullscreen_calls = 0;
    sdl_window_mismatch = 0;
    jpb_d3dapp_set_sdl_get_window_size_test_hook(
        test_sdl_get_window_size);
    jpb_d3dapp_set_sdl_set_window_fullscreen_test_hook(
        test_sdl_set_window_fullscreen);
    whook_sdl_base_path_calls = 0;
    LoadOptionsData();
    CHECK(whook_sdl_base_path_calls == 4);
    CHECK(sdl_window_size_calls == 2);
    CHECK(sdl_fullscreen_calls == 1);
    CHECK(sdl_window_mismatch == 0);
    CHECK(read_exact_file(
        options_path, &stored_settings, sizeof(stored_settings)));
    CHECK(stored_settings.ScreenWidth == 1920);
    CHECK(stored_settings.ScreenHeight == 1080);
    CHECK(OptionStruct.ScreenWidth == 1280);
    CHECK(OptionStruct.ScreenHeight == 720);
    CHECK(OptionStruct.ResolutionChanged == 0);

    GameStruct.difficulty = 3;
    GameStruct.ComboLevel = 7;
    abGlobalBits[0] = UINT8_C(0xa5);
    whook_sdl_base_path_calls = 0;
    SaveGameData();
    CHECK(whook_sdl_base_path_calls == 2);
    CHECK(std::filesystem::file_size(game_path) == sizeof(saveGameStruct));
    CHECK(read_exact_file(game_path, &stored_game, sizeof(stored_game)));
    CHECK(std::memcmp(
        &stored_game, &SaveGameStruct, sizeof(stored_game)) == 0);

    std::memset(&SaveGameStruct, 0xcc, sizeof(SaveGameStruct));
    whook_sdl_base_path_calls = 0;
    LoadGameData();
    CHECK(whook_sdl_base_path_calls == 2);
    CHECK(std::memcmp(
        &SaveGameStruct, &stored_game, sizeof(stored_game)) == 0);
    CHECK(GameStruct.difficulty == stored_game.difficulty);
    CHECK(GameStruct.ComboLevel == stored_game.ComboLevel);

    CHECK(std::filesystem::remove(game_path));
    std::memset(&SaveGameStruct, 0xcc, sizeof(SaveGameStruct));
    LoadGameData();
    for (std::size_t index = 0; index < sizeof(SaveGameStruct); ++index) {
        CHECK(reinterpret_cast<unsigned char *>(
                  &SaveGameStruct)[index] == 0);
    }

    jpb_d3dapp_set_sdl_get_window_size_test_hook(nullptr);
    jpb_d3dapp_set_sdl_set_window_fullscreen_test_hook(nullptr);
    jpb_WHookSetSDLTestHooks(nullptr);
    g_pD3DApp = previous_application;
    g_resolutionsCount = previous_resolution_count;
    std::memcpy(
        g_resolutions,
        previous_resolutions,
        sizeof(previous_resolutions));
    GameStruct = previous_game;
    SaveGameStruct = previous_save;
    OptionStruct = previous_options;
    std::memcpy(
        abGlobalBits,
        previous_global_bits,
        sizeof(previous_global_bits));
    (void)std::filesystem::remove_all(base_path);
    return 0;
}

static void reset_win_main_trace(void)
{
    std::memset(win_main_calls, 0, sizeof(win_main_calls));
    win_main_call_count = 0;
    win_main_width = 0;
    win_main_height = 0;
    win_main_app_id = 0;
    win_main_instance = nullptr;
    win_main_command_line = nullptr;
    win_main_achievement_data = nullptr;
    win_main_achievement_count = 0;
    win_main_destroyed_achievements = nullptr;
    win_main_error_result = 0;
    win_main_error_line = 0;
    win_main_error_message[0] = '\0';
    win_main_error_file[0] = '\0';
    win_main_fatal_message[0] = '\0';
}

static int check_win_main_calls(
    const int *expected, std::size_t expected_count)
{
    CHECK(win_main_call_count == static_cast<int>(expected_count));
    for (std::size_t index = 0; index < expected_count; ++index) {
        CHECK(win_main_calls[index] == expected[index]);
    }
    return 0;
}

static int test_canonical_video_destination(void)
{
    SCREENRECT destination;

    jpb_WHookVideoDestinationForTest(1920, 1080, &destination);
    CHECK(destination.left == 0);
    CHECK(destination.top == 0);
    CHECK(destination.right == 1920);
    CHECK(destination.bottom == 1080);

    jpb_WHookVideoDestinationForTest(2560, 1080, &destination);
    CHECK(destination.left == 320);
    CHECK(destination.top == 0);
    CHECK(destination.right == 2240);
    CHECK(destination.bottom == 1080);

    jpb_WHookVideoDestinationForTest(1920, 1200, &destination);
    CHECK(destination.left == 0);
    CHECK(destination.top == 60);
    CHECK(destination.right == 1920);
    CHECK(destination.bottom == 1140);

    jpb_WHookVideoDestinationForTest(1024, 768, &destination);
    CHECK(destination.left == 0);
    CHECK(destination.top == 96);
    CHECK(destination.right == 1024);
    CHECK(destination.bottom == 672);
    return 0;
}

static int test_canonical_text_3d_layout(void)
{
    JPBScreenPolyVertex vertices[4] = {};
    float next_x = 0.0f;

    jpb_WHookText3DQuadForTest(
        8,
        4,
        18,
        24,
        64.0f,
        32.0f,
        2.0f,
        50.0f,
        100.0f,
        200.0f,
        3.0f,
        UINT32_C(0x80402010),
        vertices,
        &next_x);
    CHECK(vertices[0].x == 100.0f);
    CHECK(vertices[0].y == 205.0f);
    CHECK(vertices[0].z == 3.0f);
    CHECK(vertices[0].argb == UINT32_C(0x80402010));
    CHECK(vertices[0].tu == 0.125f);
    CHECK(vertices[0].tv == 0.125f);
    CHECK(vertices[1].x == 120.0f);
    CHECK(vertices[1].y == 205.0f);
    CHECK(vertices[1].tu == 0.28125f);
    CHECK(vertices[1].tv == 0.125f);
    CHECK(vertices[2].x == 100.0f);
    CHECK(vertices[2].y == 245.0f);
    CHECK(vertices[2].tu == 0.125f);
    CHECK(vertices[2].tv == 0.75f);
    CHECK(vertices[3].x == 120.0f);
    CHECK(vertices[3].y == 245.0f);
    CHECK(vertices[3].tu == 0.28125f);
    CHECK(vertices[3].tv == 0.75f);
    CHECK(next_x == 124.0f);
    CHECK(jpb_WHookText3DLineAdvanceForTest(200.0f, 50.0f) == 275.0f);
    return 0;
}

static int test_canonical_text_2d_layout(void)
{
    const CVECTOR color = {0, 64, 128, 255};
    const SCREENRECT scissor = {3, 4, 603, 404};
    JPBWHookText2DDrawTest draw = {};
    int next_x = 0;
    int alpha = 0;

    jpb_WHookText2DDrawForTest(
        -2,
        7,
        12,
        8,
        4,
        18,
        24,
        20,
        100,
        200,
        color,
        1,
        scissor,
        1,
        0.25f,
        &draw,
        &next_x);
    CHECK(draw.source.left == 8);
    CHECK(draw.source.top == 4);
    CHECK(draw.source.right == 18);
    CHECK(draw.source.bottom == 24);
    CHECK(draw.destination.left == 100);
    CHECK(draw.destination.top == 208);
    CHECK(draw.destination.right == 110);
    CHECK(draw.destination.bottom == 228);
    CHECK(draw.color[0] == 0.0f);
    CHECK(draw.color[1] == 64.0f / 255.0f);
    CHECK(draw.color[2] == 128.0f / 255.0f);
    CHECK(draw.color[3] == 1.0f);
    CHECK(draw.depth == 0.25f);
    CHECK(draw.has_scissor == 1);
    CHECK(draw.scissor.left == 3);
    CHECK(draw.scissor.top == 4);
    CHECK(draw.scissor.right == 603);
    CHECK(draw.scissor.bottom == 404);
    CHECK(draw.sampler_type == TEXTURESAMPLER_LINEARCLAMP);
    CHECK(next_x == 109);

    jpb_WHookText2DDrawForTest(
        0, 1, 1, 0, 0, 1, 1, 1, 0, 0,
        color, 0, scissor, 0, 0.75f, &draw, &next_x);
    CHECK(draw.depth == 0.0f);
    CHECK(draw.has_scissor == 0);

    CHECK(jpb_WHookTextControllerIconForTest(0x2021) == 3);
    CHECK(jpb_WHookTextControllerIconForTest(0x20ac) == 2);
    CHECK(jpb_WHookTextControllerIconForTest(0x0192) == 4);
    CHECK(jpb_WHookTextControllerIconForTest(0x2020) == 5);
    CHECK(jpb_WHookTextControllerIconForTest(0x0160) == 7);
    CHECK(jpb_WHookTextControllerIconForTest(0x017d) == 9);
    CHECK(jpb_WHookTextControllerIconForTest('Q') == -1);
    CHECK(jpb_WHookTextTagIconForTest('A', &alpha) == 3);
    CHECK(alpha == 255);
    CHECK(jpb_WHookTextTagIconForTest('f', &alpha) == 7);
    CHECK(alpha == 128);
    CHECK(jpb_WHookTextTagIconForTest('Q', &alpha) == -1);
    CHECK(alpha == 255);
    return 0;
}

static int test_canonical_win_main(void)
{
    static const char *const achievement_ids[43] = {
        "ACH_GRAND_MASTER",
        "ACH_BATTER_UP",
        "ACH_AAAAAAH",
        "ACH_TRANSLATE_THIS",
        "ACH_BLADE_AMPLIFIER",
        "ACH_JEDI_MASTER",
        "ACH_TOUGH_NEGOTIATOR",
        "ACH_YOU_HEAR_THAT",
        "ACH_SCAVENGER_HUNT",
        "ACH_ROYAL_ARCHITECTURE",
        "ACH_TUSKEN_DEATH_CRY",
        "ACH_PARDON_ME_COMING_THROUGH",
        "ACH_NICE_WALK_IN_THE_WOODS",
        "ACH_CRUISIN",
        "ACH_CLIFF_HANGER",
        "ACH_MAULD",
        "ACH_BATTLEMECH",
        "ACH_YEEHAW",
        "ACH_JAR_JAR_JAIL",
        "ACH_UNLIMITED_POWER",
        "ACH_AGGRESSIVE_NEGOTIATIONS",
        "ACH_JEDI_ARE_NOT_TO_BE",
        "ACH_COMMUNICATIONS_DISRUPTION",
        "ACH_INVASION",
        "ACH_THATS_SO_WIZARD",
        "ACH_A_SURPRISE_TO_BE_SURE",
        "ACH_GUNGAN_NO_LIKIN_OUTSIDERS",
        "ACH_THE_SPEEDIEST_WAY",
        "ACH_SEEKING_THE_HIGHGROUND",
        "ACH_ALWAYS_TWO_THERE_ARE",
        "ACH_TANK_WARS",
        "ACH_1000_YEARS_IN_THE_PIT",
        "ACH_GREAT_ROLLIN_DEATH_BALLS",
        "ACH_HELPING_HAND_MAIDENS",
        "ACH_IM_A_PILOT",
        "ACH_PORTABLE_BACTA",
        "ACH_FORCE_GRENADE",
        "ACH_DARTH_MAUL",
        "ACH_FOOD_DELIVERY",
        "ACH_MASTER_MACE_WINDU",
        "ACH_MASTER_PLO_KOON",
        "ACH_CAPTAIN_QUARSH_PANAKA",
        "ACH_AUGIES_GREAT_MUNICIPAL_BAND",
    };
    const optionstruct previous_options = OptionStruct;
    CD3DFramework12 *const previous_framework = chavo.m_pFramework;
    CSteamAchievements *const previous_achievements = g_SteamAchievements;
    CSteamGameManager *const previous_game_manager = g_SteamGameManager;
    CSteamRichPresence *const previous_rich_presence =
        g_SteamRicherPresence;
    const int previous_is_steam_deck = g_isSteamDeck;
    alignas(CD3DFramework12)
        unsigned char framework_storage[sizeof(CD3DFramework12)] = {};
    auto *framework = reinterpret_cast<CD3DFramework12 *>(
        framework_storage);
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(
        static_cast<std::uintptr_t>(0x12340000));
    char command_line[] = "-canonical-test";

    for (int index = 0; index < 43; ++index) {
        CHECK(g_Achievements[index].m_eAchievementID == index + 1);
        CHECK(std::strcmp(
                  g_Achievements[index].m_pchAchievementID,
                  achievement_ids[index]) == 0);
        CHECK(g_Achievements[index].m_rgchName[0] == '\0');
        CHECK(g_Achievements[index].m_rgchDescription[0] == '\0');
        CHECK(!g_Achievements[index].m_bAchieved);
        CHECK(g_Achievements[index].m_iIconImage == 0);
    }

    jpb_WHookSetWinMainTestHooks(&win_main_hooks);
    chavo.m_pFramework = framework;

    reset_win_main_trace();
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    win_main_steam_init_result = 1;
    win_main_create_result = static_cast<std::int32_t>(UINT32_C(0x80004005));
    CHECK(WinMain(instance, nullptr, command_line, 7) == 0);
    {
        const int expected[] = {
            JPB_TEST_WINMAIN_UPDATE_MENUS,
            JPB_TEST_WINMAIN_LOAD_OPTIONS,
            JPB_TEST_WINMAIN_SET_SIZE,
            JPB_TEST_WINMAIN_STEAM_INIT,
            JPB_TEST_WINMAIN_CREATE,
            JPB_TEST_WINMAIN_REPORT_ERROR,
        };
        CHECK(check_win_main_calls(
                  expected, sizeof(expected) / sizeof(expected[0])) == 0);
    }
    CHECK(win_main_width == 1920);
    CHECK(win_main_height == 1080);
    CHECK(win_main_instance == instance);
    CHECK(win_main_command_line == command_line);
    CHECK(win_main_error_result == UINT32_C(0x80004005));
    CHECK(win_main_error_line == UINT32_C(0x2cc));
    CHECK(std::strcmp(win_main_error_message, "app.Create") == 0);
    CHECK(std::strcmp(
              win_main_error_file,
              "W:\\SWJediPowerBattles\\Work\\wHook.cpp") == 0);

    reset_win_main_trace();
    OptionStruct.ScreenWidth = 0;
    OptionStruct.ScreenHeight = 1080;
    win_main_steam_init_result = 0;
    win_main_create_result = 0;
    g_SteamAchievements = reinterpret_cast<CSteamAchievements *>(
        static_cast<std::uintptr_t>(0x4444));
    CHECK(WinMain(instance, nullptr, command_line, 7) == 0);
    {
        const int expected[] = {
            JPB_TEST_WINMAIN_UPDATE_MENUS,
            JPB_TEST_WINMAIN_LOAD_OPTIONS,
            JPB_TEST_WINMAIN_STEAM_INIT,
            JPB_TEST_WINMAIN_CREATE,
            JPB_TEST_WINMAIN_PRINT_FATAL,
            JPB_TEST_WINMAIN_SHUTDOWN,
            JPB_TEST_WINMAIN_DESTROY_ACHIEVEMENTS,
        };
        CHECK(check_win_main_calls(
                  expected, sizeof(expected) / sizeof(expected[0])) == 0);
    }
    CHECK(win_main_destroyed_achievements == g_SteamAchievements);
    CHECK(std::strcmp(
              win_main_fatal_message,
              "Fatal Error - Steam must be running to play this game "
              "(SteamAPI_Init() failed).\n") == 0);

    reset_win_main_trace();
    g_SteamAchievements = nullptr;
    win_main_steam_init_result = 1;
    win_main_restart_result = 1;
    CHECK(WinMain(instance, nullptr, command_line, 7) == 0);
    {
        const int expected[] = {
            JPB_TEST_WINMAIN_UPDATE_MENUS,
            JPB_TEST_WINMAIN_LOAD_OPTIONS,
            JPB_TEST_WINMAIN_STEAM_INIT,
            JPB_TEST_WINMAIN_CREATE,
            JPB_TEST_WINMAIN_RESTART,
            JPB_TEST_WINMAIN_PRINT_FATAL,
        };
        CHECK(check_win_main_calls(
                  expected, sizeof(expected) / sizeof(expected[0])) == 0);
    }
    CHECK(win_main_app_id == UINT32_C(0x25496e));

    reset_win_main_trace();
    win_main_restart_result = 0;
    win_main_steam_deck_result = 0;
    framework->m_isAMD = 37;
    framework->m_isSteamDeck = 41;
    g_isSteamDeck = 43;
    CHECK(WinMain(instance, nullptr, command_line, 7) == 0);
    {
        const int expected[] = {
            JPB_TEST_WINMAIN_UPDATE_MENUS,
            JPB_TEST_WINMAIN_LOAD_OPTIONS,
            JPB_TEST_WINMAIN_STEAM_INIT,
            JPB_TEST_WINMAIN_CREATE,
            JPB_TEST_WINMAIN_RESTART,
            JPB_TEST_WINMAIN_CREATE_ACHIEVEMENTS,
            JPB_TEST_WINMAIN_CREATE_GAME_MANAGER,
            JPB_TEST_WINMAIN_CREATE_RICH_PRESENCE,
            JPB_TEST_WINMAIN_IS_STEAM_DECK,
            JPB_TEST_WINMAIN_INITIALIZE_MAIN,
        };
        CHECK(check_win_main_calls(
                  expected, sizeof(expected) / sizeof(expected[0])) == 0);
    }
    CHECK(win_main_achievement_data == g_Achievements);
    CHECK(win_main_achievement_count == 43);
    CHECK(g_SteamAchievements == reinterpret_cast<CSteamAchievements *>(
              static_cast<std::uintptr_t>(0x1111)));
    CHECK(g_SteamGameManager == reinterpret_cast<CSteamGameManager *>(
              static_cast<std::uintptr_t>(0x2222)));
    CHECK(g_SteamRicherPresence == reinterpret_cast<CSteamRichPresence *>(
              static_cast<std::uintptr_t>(0x3333)));
    CHECK(framework->m_isAMD == 37);
    CHECK(framework->m_isSteamDeck == 0);
    CHECK(g_isSteamDeck == 0);

    reset_win_main_trace();
    win_main_steam_deck_result = 1;
    framework->m_isAMD = 0;
    framework->m_isSteamDeck = 0;
    g_isSteamDeck = 0;
    CHECK(WinMain(instance, nullptr, command_line, 7) == 0);
    CHECK(framework->m_isAMD == 1);
    CHECK(framework->m_isSteamDeck == 1);
    CHECK(g_isSteamDeck == 1);

    jpb_WHookSetWinMainTestHooks(nullptr);
    OptionStruct = previous_options;
    chavo.m_pFramework = previous_framework;
    g_SteamAchievements = previous_achievements;
    g_SteamGameManager = previous_game_manager;
    g_SteamRicherPresence = previous_rich_presence;
    g_isSteamDeck = previous_is_steam_deck;
    return 0;
}

int main(void)
{
#if defined(_DEBUG)
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    CHECK(test_key_edges() == 0);
    CHECK(test_clip_rect() == 0);
    CHECK(test_canonical_serializers() == 0);
    CHECK(test_set_in_menu() == 0);
    CHECK(test_default_resolution_index() == 0);
    CHECK(test_filename_helpers() == 0);
    CHECK(test_valid_resolutions() == 0);
    CHECK(test_debug_formatting_wrappers() == 0);
    CHECK(test_achievement_names() == 0);
    CHECK(test_fbx_cleanup_owner() == 0);
    CHECK(test_transparent_polygon_dispatch() == 0);
    CHECK(test_audio_callback() == 0);
    CHECK(test_localized_video_paths() == 0);
    CHECK(test_legacy_sdl_ui_owners() == 0);
    CHECK(test_canonical_persistence_owners() == 0);
    CHECK(test_canonical_video_destination() == 0);
    CHECK(test_canonical_text_3d_layout() == 0);
    CHECK(test_canonical_text_2d_layout() == 0);
    CHECK(test_canonical_win_main() == 0);
    std::puts("wHook input tests passed");
    return 0;
}
