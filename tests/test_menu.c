#include "jpb/menu.h"

#include "jpb/alltext.h"
#include "jpb/audio_stream.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/jedi.h"
#include "jpb/memory.h"
#include "jpb/camera.h"
#include "jpb/combo.h"
#include "jpb/console.h"
#include "jpb/debugtext.h"
#include "jpb/player.h"
#include "jpb/platform.h"
#include "jpb/prim.h"
#include "jpb/pwrup.h"
#include "jpb/resources.h"
#include "jpb/savegame.h"
#include "jpb/sprite.h"
#include "jpb/sound.h"
#include "jpb/text.h"
#include "jpb/textutil.h"
#include "jpb/texture.h"
#include "jpb/utf16.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#endif

#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed: %s (%s:%d)\n", \
                #condition, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

static int ignore_achievement_call(int id, void *user_data)
{
    (void)id;
    (void)user_data;
    return 0;
}

static int utf16_matches_utf8(
    const uint16_t *actual, const char *expected)
{
    unsigned short *converted = NULL;
    int matches;

    ConvertToUTF16(expected, &converted);
    matches = converted != NULL &&
        jpb_utf16_compare(
            actual, (const uint16_t *)(const void *)converted) == 0;
    free(converted);
    return matches;
}

static int utf16_prefix_matches_utf8(
    const uint16_t *actual, const char *expected, size_t count)
{
    unsigned short *converted = NULL;
    size_t index;
    int matches = 1;

    ConvertToUTF16(expected, &converted);
    if (converted == NULL) {
        return 0;
    }
    for (index = 0; index < count; ++index) {
        if (actual[index] != converted[index]) {
            matches = 0;
            break;
        }
    }
    free(converted);
    return matches;
}

typedef struct PlatformTrace {
    int order[8];
    int count;
    int assignCalls;
    int numPlayersDuringAssign;
    int activationCalls;
    uint32_t activatedDestination;
    int movieCalls;
    unsigned lastMovie;
    int lastMovieFlags;
    unsigned movies[8];
    int cleanupCalls;
    int saveCalls;
    int settingsSaveCalls;
    optionstruct savedSettings;
    int inMenuCalls;
    int lastInMenu;
    int scanCalls;
    unsigned lastScanLevel;
    int cueCalls;
    char lastCue[16];
    int transformCalls;
    unsigned controllerCount;
    int controllerCountCalls;
    int fallbackCalls;
    int openUrlCalls;
    char openedUrl[64];
    int exitCalls;
} PlatformTrace;

typedef struct MenuSoundTrace {
    int calls;
    int bank;
    VECTOR *position;
    uint32_t flag;
    char sound[32];
} MenuSoundTrace;

typedef struct MenuDrawTrace {
    int calls;
    int tint[32];
    int mode[32];
    int x[32];
    int y[32];
    float scale[32];
    float scaleAdjustment[32];
    int fontStyle[32];
    int clipEnabled[32];
    int clipLeft[32];
    int clipTop[32];
    int clipRight[32];
    int clipBottom[32];
    int depthEnabled[32];
    float depth[32];
    uint16_t text[32][64];
} MenuDrawTrace;

typedef struct MenuInputTrace {
    uint32_t pads[JPB_INPUT_PAD_COUNT];
    uint8_t keyboard[512];
} MenuInputTrace;

typedef struct MenuRumbleTrace {
    int calls;
    int controllerIndices[8];
} MenuRumbleTrace;

static void trace_menu_rumble(
    int32_t controller_index,
    uint16_t low_frequency,
    uint16_t high_frequency,
    uint32_t duration_ms,
    void *user_data)
{
    MenuRumbleTrace *trace = (MenuRumbleTrace *)user_data;

    (void)low_frequency;
    (void)high_frequency;
    (void)duration_ms;
    if (trace->calls < 8) {
        trace->controllerIndices[trace->calls] =
            controller_index;
    }
    ++trace->calls;
}

typedef struct CharacterDrawTrace {
    int calls;
    int model[16];
    uint32_t pad[16];
    int exitPhase[16];
} CharacterDrawTrace;

typedef struct P2CharacterDrawTrace {
    int calls;
    int playerOne[24];
    int playerTwo[24];
    uint32_t playerOnePad[24];
    uint32_t playerTwoPad[24];
    int isVersus[24];
} P2CharacterDrawTrace;

static int menu_cheat_action_calls;
static const char *menu_test_controller_name;

typedef struct MenuTextureTrace {
    unsigned loadCalls;
    unsigned unloadCalls;
} MenuTextureTrace;

typedef struct MenuTextureDrawTrace {
    int calls;
    _Material *materials[64];
    SCREENRECT destinations[64];
    SCREENRECT sources[64];
    SCREENRECT scissors[64];
    int hasScissor[64];
    CVECTOR colors[64];
    float layers[64];
} MenuTextureDrawTrace;

typedef struct MenuPolyDrawTrace {
    int calls;
    _Material *materials[4];
    int vertexCounts[4];
    JPBScreenPolyVertex vertices[4][4];
} MenuPolyDrawTrace;

typedef struct MenuUITextClipTrace {
    int calls;
    int clippedCalls;
} MenuUITextClipTrace;

typedef struct MenuLoadScreenPresentTrace {
    int calls;
} MenuLoadScreenPresentTrace;

typedef struct MenuAudioControlTrace {
    int calls;
    JPBAudioStreamControl lastControl;
    int lastValue;
    int playCalls;
    int lastTrack;
    int lastVolume;
    int lastLoop;
} MenuAudioControlTrace;

typedef struct MenuSoundControlTrace {
    int calls;
    JPBSoundControl lastControl;
} MenuSoundControlTrace;

static void capture_menu_sound_control(
    JPBSoundControl control, void *user_data)
{
    MenuSoundControlTrace *trace =
        (MenuSoundControlTrace *)user_data;

    ++trace->calls;
    trace->lastControl = control;
}

static int capture_menu_audio_control(
    JPBAudioStreamControl control,
    int value,
    void *user_data)
{
    MenuAudioControlTrace *trace =
        (MenuAudioControlTrace *)user_data;

    ++trace->calls;
    trace->lastControl = control;
    trace->lastValue = value;
    return 1;
}

static void capture_menu_audio_play(
    int track,
    const char *stream_name,
    int volume,
    int loop,
    void *user_data)
{
    MenuAudioControlTrace *trace =
        (MenuAudioControlTrace *)user_data;

    (void)stream_name;
    ++trace->playCalls;
    trace->lastTrack = track;
    trace->lastVolume = volume;
    trace->lastLoop = loop;
}

static void capture_menu_texture_draw(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth)
{
    MenuTextureDrawTrace *trace =
        (MenuTextureDrawTrace *)user_data;
    int index = trace->calls++;

    if (index < 64) {
        trace->materials[index] = texture;
        trace->destinations[index] = *destination;
        if (source != NULL) {
            trace->sources[index] = *source;
        }
        trace->colors[index] = color;
        trace->layers[index] = layer_depth;
    }
}

static void capture_menu_texture_draw_clipped(
    void *user_data,
    _Material *texture,
    const SCREENRECT *destination,
    const SCREENRECT *source,
    CVECTOR color,
    float layer_depth,
    const SCREENRECT *scissor)
{
    MenuTextureDrawTrace *trace =
        (MenuTextureDrawTrace *)user_data;
    int index = trace->calls;

    capture_menu_texture_draw(
        user_data, texture, destination, source, color, layer_depth);
    if (index < 64 && scissor != NULL) {
        trace->scissors[index] = *scissor;
        trace->hasScissor[index] = 1;
    }
}

static void capture_menu_ui_text_clip(
    void *user_data,
    const uint16_t *text,
    const SCREENRECT *destination,
    int font_style,
    int point_size,
    CVECTOR color,
    int depth_enabled,
    float depth)
{
    MenuUITextClipTrace *trace = (MenuUITextClipTrace *)user_data;

    (void)text;
    (void)destination;
    (void)font_style;
    (void)point_size;
    (void)color;
    (void)depth_enabled;
    (void)depth;
    ++trace->calls;
    if (jpb_TextGetClipRect(NULL, NULL, NULL, NULL) != 0) {
        ++trace->clippedCalls;
    }
}

static void capture_load_screen_present(
    void *user_data)
{
    MenuLoadScreenPresentTrace *trace =
        (MenuLoadScreenPresentTrace *)user_data;

    ++trace->calls;
}

static void capture_clear_window(void *user_data)
{
    ++*(int *)user_data;
}

static void ignore_clear_window(void *user_data)
{
    (void)user_data;
}

static void capture_menu_poly_draw(
    void *user_data,
    _Material *material,
    uint32_t material_flags,
    int vertex_count,
    const JPBScreenPolyVertex *vertices,
    int no_scale)
{
    MenuPolyDrawTrace *trace = (MenuPolyDrawTrace *)user_data;
    int index = trace->calls++;

    (void)material_flags;
    (void)no_scale;
    if (index < 4) {
        trace->materials[index] = material;
        trace->vertexCounts[index] = vertex_count;
        if (vertex_count > 0 && vertex_count <= 4) {
            memcpy(
                trace->vertices[index],
                vertices,
                (size_t)vertex_count * sizeof(vertices[0]));
        }
    }
}

static void *load_menu_texture(
    void *user_data,
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height)
{
    MenuTextureTrace *trace = (MenuTextureTrace *)user_data;

    (void)option;
    (void)material_type;
    if (filename == NULL) {
        return NULL;
    }
    ++trace->loadCalls;
    *width = 320;
    *height = 180;
    return (void *)(uintptr_t)(trace->loadCalls + 1u);
}

static void unload_menu_texture(void *user_data, void *texture)
{
    MenuTextureTrace *trace = (MenuTextureTrace *)user_data;

    if (texture != NULL) {
        ++trace->unloadCalls;
    }
}

static const char *read_menu_controller_name(
    unsigned player, void *user_data)
{
    (void)player;
    (void)user_data;
    return menu_test_controller_name;
}

static void record_menu_cheat_action(void)
{
    ++menu_cheat_action_calls;
}

static uint32_t read_menu_pad(int32_t pad_index, void *user_data)
{
    MenuInputTrace *trace = (MenuInputTrace *)user_data;

    return trace->pads[pad_index];
}

static const uint8_t *read_menu_keyboard(
    size_t *key_count, void *user_data)
{
    MenuInputTrace *trace = (MenuInputTrace *)user_data;

    *key_count = sizeof(trace->keyboard);
    return trace->keyboard;
}

static void capture_menu_text(
    void *user_data,
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    float scale_adjustment,
    int font_style,
    int depth_enabled,
    float depth,
    const uint16_t *text)
{
    MenuDrawTrace *trace = (MenuDrawTrace *)user_data;
    int index = trace->calls++;
    size_t text_length = jpb_utf16_length(text);

    (void)alpha;
    (void)mode;
    if (index < 32) {
        trace->tint[index] = tint;
        trace->mode[index] = mode;
        trace->x[index] = x;
        trace->y[index] = y;
        trace->scale[index] = scale;
        trace->scaleAdjustment[index] = scale_adjustment;
        trace->fontStyle[index] = font_style;
        trace->depthEnabled[index] = depth_enabled;
        trace->depth[index] = depth;
        trace->clipEnabled[index] = jpb_TextGetClipRect(
            &trace->clipLeft[index],
            &trace->clipTop[index],
            &trace->clipRight[index],
            &trace->clipBottom[index]);
        if (text_length > 63) {
            text_length = 63;
        }
        jpb_utf16_copy(trace->text[index], text, text_length);
        trace->text[index][text_length] = 0;
    }
}

static void capture_p1_character_select(
    int model,
    uint32_t pad,
    int exit_phase,
    void *user_data)
{
    CharacterDrawTrace *trace = (CharacterDrawTrace *)user_data;
    int index = trace->calls++;

    if (index < 16) {
        trace->model[index] = model;
        trace->pad[index] = pad;
        trace->exitPhase[index] = exit_phase;
    }
}

static void capture_p2_character_select(
    int player_one_model,
    int player_two_model,
    uint32_t player_one_pad,
    uint32_t player_two_pad,
    int is_versus,
    void *user_data)
{
    P2CharacterDrawTrace *trace = (P2CharacterDrawTrace *)user_data;
    int index = trace->calls++;

    if (index < 24) {
        trace->playerOne[index] = player_one_model;
        trace->playerTwo[index] = player_two_model;
        trace->playerOnePad[index] = player_one_pad;
        trace->playerTwoPad[index] = player_two_pad;
        trace->isVersus[index] = is_versus;
    }
}

static void trace_platform_step(void *user_data, int step)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;
    trace->order[trace->count++] = step;
}

static void trace_init_input(void *user_data)
{
    trace_platform_step(user_data, 1);
}

static void trace_load_textures(void *user_data)
{
    trace_platform_step(user_data, 2);
    menuTexLoaded = 1;
}

static void trace_init_bucket(void *user_data)
{
    trace_platform_step(user_data, 3);
}

static void trace_close_bucket(void *user_data)
{
    trace_platform_step(user_data, 4);
}

static int trace_assign_player_one(void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;
    ++trace->assignCalls;
    trace->numPlayersDuringAssign = GameStruct.NumPlayers;
    return 1;
}

static int trace_activate_item(
    uint32_t destination, void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

    ++trace->activationCalls;
    trace->activatedDestination = destination;
    return 1;
}

static uint16_t trace_menu_sound(
    void *chunk,
    int loops,
    VECTOR *position,
    int bank,
    char *sound,
    uint32_t flag,
    void *user_data)
{
    MenuSoundTrace *trace = (MenuSoundTrace *)user_data;

    (void)chunk;
    (void)loops;
    ++trace->calls;
    trace->bank = bank;
    trace->position = position;
    trace->flag = flag;
    (void)snprintf(trace->sound, sizeof(trace->sound), "%s", sound);
    return 1;
}

static void trace_movie(
    unsigned movie, int flags, void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

    if (trace->movieCalls <
        (int)(sizeof(trace->movies) / sizeof(trace->movies[0]))) {
        trace->movies[trace->movieCalls] = movie;
    }
    ++trace->movieCalls;
    trace->lastMovie = movie;
    trace->lastMovieFlags = flags;
}

static void trace_cleanup(void *user_data)
{
    ++((PlatformTrace *)user_data)->cleanupCalls;
}

static void trace_save(void *user_data)
{
    ++((PlatformTrace *)user_data)->saveCalls;
}

static void trace_save_settings(
    const optionstruct *options, void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

    ++trace->settingsSaveCalls;
    trace->savedSettings = *options;
}

static void trace_in_menu(int in_menu, void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

    ++trace->inMenuCalls;
    trace->lastInMenu = in_menu;
}

static void trace_scan_level(unsigned level, void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

    ++trace->scanCalls;
    trace->lastScanLevel = level;
}

static void trace_sound_cue(const char *name, void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

    ++trace->cueCalls;
    strncpy(trace->lastCue, name, sizeof(trace->lastCue) - 1u);
    trace->lastCue[sizeof(trace->lastCue) - 1u] = '\0';
}

static void trace_refresh_transforms(void *user_data)
{
    ++((PlatformTrace *)user_data)->transformCalls;
}

static unsigned trace_controller_count(void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

    ++trace->controllerCountCalls;
    return trace->controllerCount;
}

static void trace_single_controller_fallback(void *user_data)
{
    ++((PlatformTrace *)user_data)->fallbackCalls;
}

static void trace_request_exit(void *user_data)
{
    ++((PlatformTrace *)user_data)->exitCalls;
}

static void trace_open_url(const char *url, void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

    ++trace->openUrlCalls;
    strncpy(trace->openedUrl, url, sizeof(trace->openedUrl) - 1u);
    trace->openedUrl[sizeof(trace->openedUrl) - 1u] = '\0';
}

static void fixed_window_size(
    int *width, int *height, void *user_data)
{
    (void)user_data;
    *width = 960;
    *height = 540;
}

static void reset_menu_state(void)
{
    memset(&menuVars, 0, sizeof(menuVars));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(&cardLoadBuffer, 0, sizeof(cardLoadBuffer));
    menuTexLoaded = 0;
    menuTexLoaded2 = 0;
    loadScreenFlag = 0;
    loadTotal = 0;
    padShockable = 0;
    screenSaverCount = 0;
    screenSaverFlag = 0;
    saverAlpha = 0;
    memset(saverPads, 0, sizeof(saverPads));
    slider = 0;
    memset(padCurrentBits, 0, sizeof(padCurrentBits));
    keyboardBufferIndex = 0;
    keyboardKeyPressed = 0;
    memset(keyboardBuffer, 0, sizeof(keyboardBuffer));
    secretBits = 0;
    tempPlayersVs = 0;
    p2Connected = 0;
    padTypes = 0;
    padExist = 0;
    lastUsedInputType = 0;
    p1Disconnected = 0;
    p2Disconnected = 0;
    savedNumPlayer = 0;
    m_canShowRegisterGame = 0;
    newMenu_errorState = 0x10;
    newMenu_trainLevel = 1;
    newMenu_currentModelSelectNGPP1 = pilot_model;
    newMenu_currentModelSelectBaseP2 = qui_gon_model;
    newMenu_currentModelSelectNGPP2 = rifle_model;
    newMenu_state = 0;
    newMenu_bAbortMenu = 0;
    newMenu_select = 0;
    newMenu_playerSelectTypeP1 = 0;
    newMenu_currentModelSelectBaseP1 = obi_wan_model;
    newMenu_playerSelectTypeP2 = 0;
    credMuse = 0;
    cachedInputL = 0;
    cachedInputR = 0;
    creditBarPosition = 0.0f;
    skipCreditForFrame = 0;
    deltaTime = 0.01666f;
    frontZ = 0.0f;
    refreshHUDCounter = 0;
    comboIconOverride = 0;
    bonusOverride = 0;
    cachedAwardIndex = 0;
    memset(cachedRewardsInit, 0, sizeof(cachedRewardsInit));
    memset(cachedRewardsEnd, 0, sizeof(cachedRewardsEnd));
    memset(cachedBonusLines, 0, sizeof(cachedBonusLines));
    scoreYtot = 0;
    introPlayed = 0;
    VideoVolume = 0.0f;
    resolutionUpdated = 0;
    newWidth = 0;
    newHeight = 0;
    newWindowMode = 0;
    jpb_WHookSetClearWindowHook(ignore_clear_window, NULL);
    jpb_WHookSetGetWindowSizeHook(fixed_window_size, NULL);
}

static int test_title_and_stack(void)
{
    unsigned index;

    reset_menu_state();
    menuVars.menuModeSP = 2;
    memset(menuVars.mmSelect1, 0x7f, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 0x7f, sizeof(menuVars.mmSelect2));
    memset(menuVars.frKeyBuff, 0xff, sizeof(menuVars.frKeyBuff));
    menuTexLoaded2 = 7;
    menu_enterTitleMode();
    CHECK(menuTexLoaded2 == 0);
    CHECK(GameStruct.inMenuFlag == 1);
    CHECK(menuVars.menuMode[2] == 1);
    for (index = 0; index < 8; ++index) {
        CHECK(menuVars.mmSelect1[index] == 0);
        CHECK(menuVars.mmSelect2[index] == 0);
    }
    for (index = 0; index < 16; ++index) {
        CHECK(menuVars.frKeyBuff[index] == 0);
    }

    menu_pushMenu(0x41);
    CHECK(menuVars.menuModeSP == 3);
    CHECK(menuVars.menuMode[3] == 0x41);
    menu_pushMenu(0x32);
    CHECK(menuVars.menuModeSP == 3);
    menuVars.memdebugFlag = 0;
    menu_pushMenu(0x30);
    CHECK(menuVars.menuModeSP == 3);
    menuVars.memdebugFlag = 1;
    menu_pushMenu(0x30);
    CHECK(menuVars.menuModeSP == 4);
    CHECK(menuVars.menuMode[4] == 0x30);
    menu_popMenu();
    CHECK(menuVars.menuModeSP == 3);

    menuVars.menuModeSP = 3;
    menuVars.menuMode[2] = 0x90;
    menu_popMenu();
    CHECK(menuVars.menuModeSP == 1);
    menuVars.menuMode[0] = 0x32;
    menuVars.menuModeSP = 1;
    menu_popMenu();
    CHECK(menuVars.menuModeSP == 1);
    return 0;
}

static int test_special_message_transition(void)
{
    uint8_t ordinary_message[] = "ordinary";
    uint8_t canonical_message[] = "canonical";

    reset_menu_state();
    allText[376] = (char *)(void *)canonical_message;
    menuVars.menuModeSP = 6;
    menuVars.titleArt = 9;
    memset(menuVars.menuMode, 0xa5, sizeof(menuVars.menuMode));
    memset(menuVars.mmSelect1, 0xa5, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 0xa5, sizeof(menuVars.mmSelect2));

    menu_specialMess(ordinary_message);
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) != 0);
    CHECK(GameStruct.inMenuFlag == 1);
    CHECK(menuVars.specialString == ordinary_message);
    CHECK(menuVars.titleArt == 0);
    CHECK(menuVars.menuModeSP == 0);
    CHECK(menuVars.menuMode[7] == UINT16_C(0x41));
    CHECK(menuVars.menuMode[0] == UINT16_C(0x2b));
    CHECK(menuVars.mmSelect1[7] == 0);
    CHECK(menuVars.mmSelect2[7] == 0);
    CHECK(menuVars.mmSelect1[0] == 0);
    CHECK(menuVars.mmSelect2[0] == 0);

    menu_specialMess(canonical_message);
    CHECK(menuVars.specialString == canonical_message);
    CHECK(menuVars.menuModeSP == 2);
    CHECK(menuVars.menuMode[1] == UINT16_C(0x41));
    CHECK(menuVars.menuMode[2] == UINT16_C(0x2c));
    CHECK(menuVars.mmSelect1[1] == 0);
    CHECK(menuVars.mmSelect2[1] == 0);
    CHECK(menuVars.mmSelect1[2] == 0);
    CHECK(menuVars.mmSelect2[2] == 0);
    return 0;
}

static int test_pre_fmv_transition_has_no_rumble(void)
{
    MenuRumbleTrace trace = {0};

    reset_menu_state();
    OptionStruct.ShockFlag[0] = 1;
    jpb_InputSetRumbleProvider(trace_menu_rumble, &trace);
    menu_pushMenu(0x41);
    CHECK(trace.calls == 1);
    menu_pushMenu(0x66);
    CHECK(trace.calls == 1);
    CHECK(menuVars.menuMode[menuVars.menuModeSP] == 0x66);
    jpb_InputSetRumbleProvider(NULL, NULL);
    OptionStruct.ShockFlag[0] = 0;
    return 0;
}

static int test_main_menu_initialization(void)
{
    JPBMenuPlatformHooks hooks;
    PlatformTrace trace;

    reset_menu_state();
    memset(&hooks, 0, sizeof(hooks));
    memset(&trace, 0, sizeof(trace));
    hooks.initInput = trace_init_input;
    hooks.loadTextures = trace_load_textures;
    hooks.initBucket = trace_init_bucket;
    hooks.closeBucketLog = trace_close_bucket;
    hooks.assignBackToP1 = trace_assign_player_one;
    jpb_MenuSetPlatformHooks(&hooks, &trace);

    menuVars.titleArt = 9;
    GameStruct.GameState = UINT32_MAX;
    menuTextures[0] = (_Material *)(uintptr_t)1;
    menu_mainInitMenu(1);
    CHECK(menuVars.titleArt == 9);
    CHECK(GameStruct.GameState == UINT32_MAX);
    CHECK(trace.count == 0);

    menu_mainInitMenu(0);
    CHECK(trace.count == 4);
    CHECK(trace.order[0] == 1);
    CHECK(trace.order[1] == 2);
    CHECK(trace.order[2] == 3);
    CHECK(trace.order[3] == 4);
    CHECK(menuTextures[0] == NULL);
    CHECK(GameStruct.ModelSelect[0] == 0);
    CHECK(GameStruct.ModelSelect[1] == 1);
    CHECK(GameStruct.inMenuFlag == 1);
    CHECK(menuVars.menuMode[0] == 1);
    CHECK(menuVars.mmColorSelect == 14);
    CHECK(menuVars.mmColorNotSelect == 15);
    CHECK(menuVars.movieSelect == -1);

    menu_setNumPlayers(1);
    CHECK(trace.assignCalls == 1);
    CHECK(trace.numPlayersDuringAssign == 0);
    CHECK(GameStruct.NumPlayers == 1);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static uint64_t hash_menu_words(
    uint64_t hash, const uint32_t *words, size_t count)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        unsigned byte_index;
        for (byte_index = 0; byte_index < 4; ++byte_index) {
            hash ^= (words[index] >> (byte_index * 8)) & 0xffu;
            hash *= UINT64_C(1099511628211);
        }
    }
    return hash;
}

static int test_recovered_title_menu_data(void)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    uint64_t option_hash = UINT64_C(14695981039346656037);

    hash = hash_menu_words(hash, xmainMdef, 43);
    hash = hash_menu_words(hash, mainMdef, 65);
    hash = hash_menu_words(hash, mainMdefNoRegisterGame, 57);
    hash = hash_menu_words(hash, NOLOADmainMdef, 24);
    hash = hash_menu_words(hash, PSXmainMdef, 51);
    hash = hash_menu_words(hash, continuemainMdef, 73);
    hash = hash_menu_words(
        hash, continuemainMdefNoRegisterGame, 65);
    hash = hash_menu_words(hash, NOLOADmainMdef2, 24);
    hash = hash_menu_words(hash, PSXmainMdef2, 51);
    hash = hash_menu_words(hash, continueNOLOADmainMdef, 30);
    hash = hash_menu_words(hash, titlePlayerCountMdef, 21);
    hash = hash_menu_words(
        hash, titlePlayerCountContinueMdef, 21);
    hash = hash_menu_words(hash, titlePlayerCountVSMdef, 21);
    hash = hash_menu_words(hash, difficultyMdef, 21);
    hash = hash_menu_words(hash, newgameconfirmMdef, 29);
    hash = hash_menu_words(hash, optionsMdef, 44);
    hash = hash_menu_words(hash, rusureQuitMenuMdef, 25);
    CHECK(hash == UINT64_C(0xD4BE42C48C32B0A2));

    /* Exact allText indices used by the ordinary title stream. */
    CHECK(mainMdef[12] == 182); /* New Game */
    CHECK(mainMdef[16] == 331); /* Training */
    CHECK(mainMdef[20] == 381); /* VS. Mode */
    CHECK(mainMdef[24] == 188); /* Options */
    CHECK(mainMdef[28] == 259); /* Quit */
    CHECK(mainMdef[32] == 400); /* Register Your Game */
    CHECK(sizeof(titlePlayerCountMdef) == 84);
    CHECK(sizeof(titlePlayerCountContinueMdef) == 84);
    CHECK(sizeof(titlePlayerCountVSMdef) == 84);
    CHECK(sizeof(difficultyMdef) == 84);
    CHECK(sizeof(playerCountSelectMdef) == 80);
    CHECK(sizeof(newgameconfirmMdef) == 116);
    CHECK(sizeof(optionsMdef) == 176);
    CHECK(sizeof(rusureQuitMenuMdef) == 100);
    CHECK(newgameconfirmMdef[10] == 9);
    CHECK(newgameconfirmMdef[14] == 83);
    CHECK(newgameconfirmMdef[16] == 0); /* prompt x: pivot-centered */
    CHECK(newgameconfirmMdef[17] == 85);
    CHECK(newgameconfirmMdef[23] == 0); /* box x: pivot-centered */
    CHECK(newgameconfirmMdef[24] == 0);
    CHECK(newgameconfirmMdef[25] == 0x48);
    CHECK(newgameconfirmMdef[26] == 585); /* popup top before MM scale */
    CHECK(optionsMdef[10] == 149);
    CHECK(optionsMdef[30] == 27);
    CHECK(rusureQuitMenuMdef[10] == 148);
    CHECK(rusureQuitMenuMdef[14] == 147);
    CHECK(playerCountSelectMdef[10] == 106);
    CHECK(playerCountSelectMdef[14] == 107);
    CHECK(sizeof(startMdef) == 16);
    CHECK(startMdef[0] == 0 && startMdef[1] == 0);
    CHECK(startMdef[2] == 0x14 && startMdef[3] == 0);

    option_hash = hash_menu_words(option_hash, controlsMdef, 16);
    option_hash = hash_menu_words(option_hash, titlecontrolsMdef, 16);
    option_hash = hash_menu_words(option_hash, controls1Mdef, 42);
    option_hash = hash_menu_words(option_hash, controls2Mdef, 42);
    option_hash = hash_menu_words(option_hash, languageMdef, 25);
    option_hash = hash_menu_words(option_hash, videoMdef, 34);
    CHECK(option_hash == UINT64_C(0x3B28355B24668283));
    CHECK(sizeof(controlsMdef) == 64);
    CHECK(sizeof(titlecontrolsMdef) == 64);
    CHECK(sizeof(controls1Mdef) == 168);
    CHECK(sizeof(controls2Mdef) == 168);
    CHECK(sizeof(controlSubDraw) == 32);
    CHECK(controlSubDraw[5] == 0x47);
    CHECK(ClassicControlScheme[0] == 0);
    CHECK(ClassicControlScheme[6] == 6);
    CHECK(ModernControlSchemeForce[0] == 1);
    CHECK(ModernControlSchemeForce[2] == 3);
    CHECK(ModernControlSchemeForce[6] == 7);
    CHECK(controlTextList[0] == 249);
    CHECK(controlTextList[7] == 251);
    CHECK(controlTextListForce[5] == 468);
    CHECK(sizeof(languageMdef) == 100);
    CHECK(sizeof(videoMdef) == 136);
    CHECK(videoMdef[11] == 72);
    CHECK(videoMdef[16] == 73);
    return 0;
}

static int test_recovered_menu_texture_bank(void)
{
    MenuTextureTrace trace;
    JPBMenuPlatformHooks hooks;
    _Material *selected[10];

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    memset(&hooks, 0, sizeof(hooks));
    memset(selected, 0, sizeof(selected));
    memset(menuTextures, 0, sizeof(menuTextures));
    memset(fontSpec, 0, sizeof(fontSpec));
    memset(g_material, 0, sizeof(g_material));
    CHECK(sizeof(JPBMenuTextureEntry) == 16);
    CHECK(sizeof(FONTSPEC) == 12);
    CHECK(JPB_MENU_TEXTURE_ENTRY_COUNT == 132);
    CHECK(strcmp(menuTextureList[0].filename, "winsys0.png") == 0);
    CHECK(menuTextureList[0].textureIndex == 0);
    CHECK(menuTextureList[0].spriteIndex == 1);
    CHECK(menuTextureList[0].legacyFlags == 0x10);
    CHECK(strcmp(
              menuTextureList[66].filename,
              "NewUI/PlayerContainer.png") == 0);
    CHECK(menuTextureList[66].textureIndex == 183);
    CHECK(strcmp(
              menuTextureList[84].filename,
              "NewUI/CharacterSelectImages/Obi.png") == 0);
    CHECK(menuTextureList[84].textureIndex == 201);
    CHECK(menuTextureList[131].filename == NULL);
    CHECK(menuTextureList[131].textureIndex == 119);
    CHECK(menuTextureList[131].spriteIndex == 449);

    CHECK(jpb_ResourceSetBasePath("C:/jpb-menu-test"));
    jpb_TextureSetPlatformHooks(
        load_menu_texture, unload_menu_texture, &trace);
    menu_winLoadTextures();
    CHECK(menuTexLoaded == 1);
    CHECK(trace.loadCalls > 180);
    CHECK(menuTextures[0] != NULL);
    CHECK(menuTextures[80] != NULL);
    CHECK(menuTextures[94] != NULL);
    CHECK(menuTextures[119] != NULL);
    CHECK(menuTextures[183] != NULL);
    CHECK(menuTextures[201] != NULL);
    CHECK(menuTextures[248] != NULL);
    CHECK(whitemat != NULL);
    CHECK(whitematAdd != NULL);
    CHECK(controlTextures[0] != NULL);
    CHECK(controlTextures[8] != NULL);
    CHECK(kbmTextures[0] != NULL);
    CHECK(kbmTextures[9] != NULL);
    CHECK(kbmForceTextures[0] != NULL);
    CHECK(kbmForceTextures[3] != NULL);
    CHECK(ps4Textures[8] != NULL);
    CHECK(ps5Textures[8] != NULL);
    CHECK(switchTextures[8] != NULL);
    CHECK(switchProTextures[8] != NULL);
    CHECK(joyconTextures[8] != NULL);
    CHECK(xsxTextures[8] != NULL);
    lastUsedInputType = 0;
    CHECK(getControllerTextures(0, selected) == 1);
    CHECK(selected[0] == kbmTextures[0]);
    lastUsedInputType = 1;
    hooks.controllerName = read_menu_controller_name;
    jpb_MenuSetPlatformHooks(&hooks, NULL);
    menu_test_controller_name = "PS5 Controller";
    CHECK(getControllerTextures(0, selected) == 1);
    CHECK(selected[0] == ps5Textures[0]);
    menu_test_controller_name = "Xbox Series X Controller";
    CHECK(getControllerTextures(1, selected) == 1);
    CHECK(selected[8] == xsxTextures[8]);
    menu_test_controller_name = "Generic Controller";
    CHECK(getControllerTextures(1, selected) == 1);
    CHECK(selected[0] == controlTextures[0]);
    CHECK(fontSpec[1].clut == 0);
    CHECK(fontSpec[1].w == 320);
    CHECK(fontSpec[1].h == 180);
    CHECK(fontSpec[352].clut == 248);
    CHECK(fontSpec[352].w == 320);
    CHECK(fontSpec[0xe8].clut == 3);
    CHECK(fontSpec[0xe8].x == 54);
    CHECK(fontSpec[0xe8].y == 0);
    CHECK(fontSpec[0xe8].w == 28);
    CHECK(fontSpec[0xe8].h == 26);
    CHECK(fontSpec[0xf2].clut == 3);
    CHECK(fontSpec[0xf2].x == 178);
    CHECK(fontSpec[0xf2].y == 62);
    CHECK(fontSpec[0xf2].w == 43);
    CHECK(fontSpec[0xf2].h == 17);
    CHECK(fontSpec[0x115].clut == 3);
    CHECK(fontSpec[0x115].x == 8);
    CHECK(fontSpec[0x115].y == 85);
    CHECK(fontSpec[0x115].w == 8);
    CHECK(fontSpec[0x115].h == 11);
    CHECK(fontSpec[0x119].clut == 3);
    CHECK(fontSpec[0x119].x == 2);
    CHECK(fontSpec[0x119].y == 24);
    CHECK(fontSpec[0x119].w == 8);
    CHECK(fontSpec[0x119].h == 2);
    CHECK(fontSpec[411].clut == 80);
    CHECK(fontSpec[411].w == 80);
    CHECK(fontSpec[411].h == 45);
    CHECK(fontSpec[425].clut == 94);
    CHECK(fontSpec[449].clut == 119);
    CHECK(fontSpec[449].h == 180);

    texture_Flush((unsigned)TT_ANY);
    CHECK(trace.unloadCalls == trace.loadCalls);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    menu_test_controller_name = NULL;
    memset(menuTextures, 0, sizeof(menuTextures));
    memset(fontSpec, 0, sizeof(fontSpec));
    menuTexLoaded = 0;
    return 0;
}

static int test_character_select_arrows(void)
{
    MenuTextureDrawTrace trace;
    _Material pressed_left;
    _Material pressed_right;
    _Material unselected_left;
    _Material unselected_right;

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    memset(&pressed_left, 0, sizeof(pressed_left));
    memset(&pressed_right, 0, sizeof(pressed_right));
    memset(&unselected_left, 0, sizeof(unselected_left));
    memset(&unselected_right, 0, sizeof(unselected_right));
    menuTextures[169] = &pressed_left;
    menuTextures[170] = &pressed_right;
    menuTextures[172] = &unselected_left;
    menuTextures[173] = &unselected_right;
    scaleAdjustmentMM = 0.5f;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &trace);

    newMenu_DrawArrows(0, 200, 100, 700, 100);
    CHECK(trace.calls == 2);
    CHECK(trace.materials[0] == &unselected_left);
    CHECK(trace.materials[1] == &unselected_right);
    CHECK(trace.destinations[0].left == 150);
    CHECK(trace.destinations[0].top == 100);
    CHECK(trace.destinations[0].right == 200);
    CHECK(trace.destinations[0].bottom == 153);
    CHECK(trace.destinations[1].left == 700);
    CHECK(trace.destinations[1].right == 749);
    CHECK(trace.destinations[1].bottom == 153);
    CHECK(trace.colors[0].r == 255);
    CHECK(trace.colors[0].g == 255);
    CHECK(trace.colors[0].b == 255);
    CHECK(trace.colors[0].cd == 255);

    memset(&trace, 0, sizeof(trace));
    newMenu_DrawArrows(
        JPB_PAD_RIGHT, 200, 100, 700, 100);
    CHECK(trace.calls == 2);
    CHECK(trace.materials[0] == &pressed_left);
    CHECK(trace.materials[1] == &unselected_right);

    memset(&trace, 0, sizeof(trace));
    newMenu_DrawArrows(
        JPB_PAD_LEFT, 200, 100, 700, 100);
    CHECK(trace.calls == 2);
    CHECK(trace.materials[0] == &unselected_left);
    CHECK(trace.materials[1] == &pressed_right);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    memset(menuTextures, 0, sizeof(menuTextures));
    return 0;
}

static int test_audio_menu_sliders(void)
{
    MenuTextureDrawTrace trace;
    MenuAudioControlTrace audio_trace;
    _Material gradient;
    _Material mask;
    _Material background;
    int first_slider;

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    memset(&audio_trace, 0, sizeof(audio_trace));
    memset(&gradient, 0, sizeof(gradient));
    memset(&mask, 0, sizeof(mask));
    memset(&background, 0, sizeof(background));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    OptionStruct.ResolutionChanged = 0;
    OptionStruct.Music = 1;
    OptionStruct.musicVolume = 30;
    OptionStruct.SFXVolume = 45;
    scaleAdjustmentMM = 0.5f;
    generateAllText(0);
    menuTextures[166] = &gradient;
    menuTextures[167] = &mask;
    menuTextures[168] = &background;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &trace);
    jpb_AudioStreamSetControlHook(
        capture_menu_audio_control, &audio_trace);

    menu_slideco(0.45f, 0.25f, 850, 710, 30.0f, 75.0f);
    CHECK(trace.calls == 3);
    CHECK(trace.materials[0] == &gradient);
    CHECK(trace.materials[1] == &background);
    CHECK(trace.materials[2] == &mask);
    CHECK(trace.destinations[0].left == 425);
    CHECK(trace.destinations[0].top == 355);
    CHECK(trace.destinations[0].right == 512);
    CHECK(trace.destinations[0].bottom == 371);
    CHECK(trace.sources[0].left == 200);
    CHECK(trace.sources[0].top == 0);
    CHECK(trace.sources[0].right == 242);
    CHECK(trace.sources[0].bottom == 135);
    CHECK(trace.destinations[1].left == 420);
    CHECK(trace.destinations[1].top == 353);
    CHECK(trace.destinations[1].right == 646);
    CHECK(trace.destinations[1].bottom == 372);
    CHECK(trace.colors[0].r == 255);
    CHECK(trace.colors[0].g == 255);
    CHECK(trace.colors[0].b == 255);
    CHECK(trace.colors[0].cd == 255);
    CHECK(trace.layers[0] == 0.002f);
    CHECK(trace.layers[1] == 0.003f);
    CHECK(trace.layers[2] == 0.001f);

    memset(&trace, 0, sizeof(trace));
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x10;
    GameStruct.gameMode = 0;
    menu_mainLoop();
    CHECK(trace.calls >= 6);
    first_slider = trace.calls - 6;
    CHECK(trace.materials[first_slider] == &gradient);
    CHECK(trace.materials[first_slider + 1] == &background);
    CHECK(trace.materials[first_slider + 2] == &mask);
    CHECK(trace.materials[first_slider + 3] == &gradient);
    CHECK(trace.materials[first_slider + 4] == &background);
    CHECK(trace.materials[first_slider + 5] == &mask);
    CHECK(trace.destinations[first_slider].left == 425);
    CHECK(trace.destinations[first_slider].top == 355);
    CHECK(trace.destinations[first_slider + 3].left == 425);
    CHECK(trace.destinations[first_slider + 3].top == 385);
    CHECK(trace.sources[first_slider + 3].right == 363);
    CHECK(audio_trace.calls == 1);
    CHECK(audio_trace.lastControl == JPB_AUDIO_STREAM_SET_VOLUME);
    CHECK(audio_trace.lastValue == 30);

    jpb_AudioStreamSetControlHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    memset(menuTextures, 0, sizeof(menuTextures));
    return 0;
}

static int test_reachable_menu_entry_initialization(void)
{
    JPBMenuPlatformHooks hooks;
    PlatformTrace trace;

    reset_menu_state();
    memset(&hooks, 0, sizeof(hooks));
    memset(&trace, 0, sizeof(trace));
    hooks.scanLevel = trace_scan_level;
    hooks.saveSettingsData = trace_save_settings;
    jpb_MenuSetPlatformHooks(&hooks, &trace);

    menuVars.menuMode[0] = 0;
    menuVars.menuMode[4] = UINT16_C(0x7777);
    menuVars.pplayers[0] = 2;
    menuVars.pplayers[1] = 4;
    menuVars.mcount = 9;
    menuVars.titleArt = 0;
    menuVars.titleDispEnable = 2;
    GameStruct.ModelSelect[0] = 12;
    GameStruct.ModelSelect[1] = 13;
    GameStruct.NumPlayers = 2;
    GameStruct.gameMode = 7;
    GameStruct.letterboxFlag = 1;
    GameStruct.letterboxFlag2 = 1;
    totalframes = 20;
    menu_initNewMenu();
    CHECK(totalframes == 36);
    CHECK(GameStruct.letterboxFlag == 1);
    CHECK(GameStruct.letterboxFlag2 == 1);
    CHECK(GameStruct.ModelSelect[0] == obi_wan_model);
    CHECK(GameStruct.ModelSelect[1] == qui_gon_model);
    CHECK(GameStruct.gameMode == 0);
    CHECK(menuVars.mcount == 0);
    CHECK(menuVars.subplayers[0] == 2);
    CHECK(menuVars.subplayers[1] == 4);
    CHECK((abGlobalBits[0] & UINT8_C(0xf8)) == UINT8_C(0x08));
    CHECK((abGlobalBits[1] & UINT8_C(0x1f)) == UINT8_C(0x02));
    CHECK(menuVars.titleArt == 1);
    CHECK(menuVars.titleDispEnable == 3);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[0] == 0);
    CHECK(menuVars.menuMode[1] == 0);
    CHECK(menuVars.menuMode[4] == 0);
    menuVars.menuModeSP = 0;

    menuVars.oldpad[0] = UINT32_C(0x12345678);
    menuVars.oldpad[1] = UINT32_C(0x87654321);
    menuVars.itemSelect = 9;
    menuVars.vramx = 10;
    menuVars.vramy = 11;
    menuVars.pSelect = 9;
    menuVars.mmX = 99;
    menuVars.mmY = 98;
    menuVars.mmColorSelect = 97;
    menuVars.mmColorNotSelect = 96;
    menuVars.mmvCount = 6;

    GameStruct.NumPlayers = 2;
    menuVars.menuMode[0] = 0x0c;
    menu_initNewMenu();
    CHECK(savedNumPlayer == 2);
    CHECK(menuVars.oldpad[0] == UINT32_C(0x12345678));
    CHECK(menuVars.oldpad[1] == UINT32_C(0x87654321));
    CHECK(menuVars.itemSelect == 0);
    CHECK(menuVars.vramx == 0);
    CHECK(menuVars.vramy == 11);
    CHECK(menuVars.pSelect == 0);
    CHECK(menuVars.mmX == 99);
    CHECK(menuVars.mmY == 98);
    CHECK(menuVars.mmColorSelect == 11);
    CHECK(menuVars.mmColorNotSelect == 12);
    CHECK(menuVars.mmvCount == 0);

    menuVars.menuMode[0] = 0x0b;
    menuVars.titleArt = 0;
    menuVars.titleDispEnable = 2;
    OptionStruct.Music = 0;
    OptionStruct.Language = 4;
    menu_initNewMenu();
    CHECK(menuVars.titleArt == 1);
    CHECK(menuVars.titleDispEnable == 3);
    CHECK(trace.settingsSaveCalls == 1);
    CHECK(trace.savedSettings.Language == 4);

    GameStruct.CurrentLevel = 9;
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 4;
    menu_initNewMenu();
    CHECK(GameStruct.CurrentLevel == 0);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 0x0e);

    GameStruct.NumPlayers = 1;
    tempPlayersVs = 2;
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x0d;
    menu_initNewMenu();
    CHECK(savedNumPlayer == 1);
    CHECK(GameStruct.NumPlayers == 2);

    OptionStruct.musicVolume = 255;
    OptionStruct.SFXVolume = 254;
    menuVars.menuMode[0] = 0x10;
    menu_initNewMenu();
    CHECK(OptionStruct.musicVolume == 75);
    CHECK(OptionStruct.SFXVolume == 75);
    CHECK(menuVars.sfxVolume == 75);

    menuVars.scoreScore = 1234;
    menuVars.menuMode[0] = 0x1b;
    menu_initNewMenu();
    CHECK(menuVars.scoreScore == 0);

    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x0e;
    LevelSelect = 9;
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    GameStruct.NumPlayers = 2;
    GameStruct.ModelSelect[0] = 0;
    GameStruct.ModelSelect[1] = 4;
    menuVars.pselectMode[0].mode = 1;
    menuVars.pselectMode[1].mode = 1;
    menu_pushMenu(0x1a);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(LevelSelect == 9);
    menu_initNewMenu();
    CHECK(trace.scanCalls == 31);
    CHECK(trace.lastScanLevel == 30);
    CHECK(menuVars.menuModeSP == 3);
    CHECK(menuVars.menuMode[2] == 0);
    CHECK(menuVars.menuMode[3] == 0x1a);
    CHECK(LevelSelect == 1);
    CHECK(menuVars.bgWidth == 1920);
    CHECK(menuVars.bgHeight == 1080);
    CHECK(menuVars.titleDispEnable == 1);
    CHECK(GameStruct.gameMode == 0);
    CHECK(menuVars.dstSelector == 0);
    CHECK(menuVars.selbox.y == 0x86);
    CHECK(menuVars.selbox.w == 0x78);
    CHECK(menuVars.selbox.h == 2);
    CHECK(menuVars.selCount == 0);
    CHECK(menuVars.artload == 1);
    CHECK(menuVars.artLevel == 1);
    CHECK(menuVars.artloadPos == -256);
    CHECK(menuVars.mmvCount == 4);
    CHECK(menuVars.mmv[0].mmvSrc == frameBottomMover);
    CHECK(menuVars.mmv[1].mmvSrc == frameRightMover);
    CHECK(menuVars.mmv[2].mmvSrc == frameLeftMover);
    CHECK(menuVars.mmv[3].mmvSrc == frameTopMover);
    CHECK(menuVars.mmv[0].mmvPtr == 0);
    CHECK(menuVars.mmv[0].mmvCounter == 0);
    CHECK(menuVars.mmv[0].mmvX == 0);
    CHECK(menuVars.mmv[0].mmvY == 0);
    CHECK(menuVars.mmv[0].mmvMenu == NULL);
    CHECK(menuVars.mmv[0].state == 1);
    CHECK(GameStruct.ModelSelect[0] == 6);
    CHECK(GameStruct.ModelSelect[1] == 7);

    reset_menu_state();
    menuVars.menuModeSP = 1;
    menuVars.menuMode[0] = 0;
    menuVars.menuMode[1] = 0x90;
    GameStruct.continueAble = 0;
    GameStruct.jediScorePerLevel[0][0] = 1234;
    menu_initNewMenu();
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 3);
    CHECK(GameStruct.jediScorePerLevel[0][0] == 0);

    reset_menu_state();
    menuVars.menuModeSP = 1;
    menuVars.menuMode[0] = 0;
    menuVars.menuMode[1] = 0x90;
    GameStruct.continueAble = 1;
    menu_initNewMenu();
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 0x90);

    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_p1_character_select_presentation(void)
{
    MenuTextureDrawTrace texture_trace;
    MenuDrawTrace text_trace;
    CharacterDrawTrace owner_trace;
    unsigned index;

    reset_menu_state();
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&owner_trace, 0, sizeof(owner_trace));
    for (index = 0;
         index < sizeof(menuTextures) / sizeof(menuTextures[0]);
         ++index) {
        menuTextures[index] =
            (_Material *)(uintptr_t)(index + 1u);
    }
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    OptionStruct.ResolutionChanged = 0;
    scaleAdjustmentMM = 0.5f;
    generateAllText(0);
    lastUsedInputType = 1;
    GameStruct.NumPlayers = 1;
    GameStruct.ModelSelect[0] = obi_wan_model;
    newMenu_currentModelSelectBaseP1 = obi_wan_model;
    newMenu_state = 0x18;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    jpb_MenuSetP1CharacterSelectDrawHook(
        capture_p1_character_select, &owner_trace);

    CHECK(newMenu_P1CharacterSelect() == 0);
    CHECK(owner_trace.calls == 1);
    CHECK(owner_trace.model[0] == obi_wan_model);
    CHECK(owner_trace.exitPhase[0] == 0);
    CHECK(texture_trace.calls == 19);
    CHECK(texture_trace.materials[0] == menuTextures[175]);
    CHECK(texture_trace.materials[1] == menuTextures[172]);
    CHECK(texture_trace.materials[2] == menuTextures[173]);
    CHECK(texture_trace.materials[3] == menuTextures[183]);
    CHECK(texture_trace.materials[4] == menuTextures[201]);
    CHECK(texture_trace.materials[5] == menuTextures[184]);
    CHECK(texture_trace.materials[6] == menuTextures[192]);
    CHECK(texture_trace.materials[15] == menuTextures[188]);
    CHECK(texture_trace.materials[16] == menuTextures[185]);
    CHECK(texture_trace.materials[17] == menuTextures[189]);
    CHECK(texture_trace.materials[18] == menuTextures[178]);
    CHECK(text_trace.calls == 6);
    CHECK(utf16_matches_utf8(text_trace.text[2], allText[332]));
    CHECK(utf16_matches_utf8(text_trace.text[3], allText[481]));

    jpb_MenuSetP1CharacterSelectDrawHook(NULL, NULL);
    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    memset(menuTextures, 0, sizeof(menuTextures));
    return 0;
}

static int test_p2_character_select_presentation(void)
{
    MenuTextureDrawTrace texture_trace;
    MenuDrawTrace text_trace;
    P2CharacterDrawTrace owner_trace;
    unsigned index;

    reset_menu_state();
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&owner_trace, 0, sizeof(owner_trace));
    for (index = 0;
         index < sizeof(menuTextures) / sizeof(menuTextures[0]);
         ++index) {
        menuTextures[index] =
            (_Material *)(uintptr_t)(index + 1u);
    }
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    OptionStruct.ResolutionChanged = 0;
    scaleAdjustmentMM = 0.5f;
    generateAllText(0);
    lastUsedInputType = 1;
    GameStruct.NumPlayers = 2;
    GameStruct.ModelSelect[0] = obi_wan_model;
    GameStruct.ModelSelect[1] = qui_gon_model;
    newMenu_currentModelSelectBaseP1 = obi_wan_model;
    newMenu_currentModelSelectBaseP2 = qui_gon_model;
    newMenu_state = 0x18;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    jpb_MenuSetP2CharacterSelectDrawHook(
        capture_p2_character_select, &owner_trace);

    CHECK(newMenu_P2CharacterSelect(0) == 0);
    CHECK(owner_trace.calls == 1);
    CHECK(owner_trace.playerOne[0] == obi_wan_model);
    CHECK(owner_trace.playerTwo[0] == qui_gon_model);
    CHECK(owner_trace.isVersus[0] == 0);
    CHECK(texture_trace.calls == 22);
    CHECK(texture_trace.materials[0] == menuTextures[175]);
    CHECK(texture_trace.materials[1] == menuTextures[172]);
    CHECK(texture_trace.materials[2] == menuTextures[173]);
    CHECK(texture_trace.materials[3] == menuTextures[172]);
    CHECK(texture_trace.materials[4] == menuTextures[173]);
    CHECK(texture_trace.materials[5] == menuTextures[177]);
    CHECK(texture_trace.materials[6] == menuTextures[190]);
    CHECK(texture_trace.materials[7] == menuTextures[183]);
    CHECK(texture_trace.materials[8] == menuTextures[201]);
    CHECK(texture_trace.materials[14] == menuTextures[190]);
    CHECK(texture_trace.materials[15] == menuTextures[183]);
    CHECK(texture_trace.materials[16] == menuTextures[202]);
    CHECK(texture_trace.destinations[7].left == 120);
    CHECK(texture_trace.destinations[7].right == 349);
    CHECK(texture_trace.destinations[15].left == 610);
    CHECK(texture_trace.destinations[15].right == 839);
    CHECK(text_trace.calls == 10);
    CHECK(utf16_matches_utf8(text_trace.text[2], allText[481]));
    CHECK(utf16_matches_utf8(text_trace.text[3], allText[332]));
    CHECK(utf16_matches_utf8(text_trace.text[6], allText[482]));
    CHECK(utf16_matches_utf8(text_trace.text[7], allText[333]));
    CHECK(player2IconOverride == 0);

    jpb_MenuSetP2CharacterSelectDrawHook(NULL, NULL);
    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    memset(menuTextures, 0, sizeof(menuTextures));
    return 0;
}

static int test_character_select_reconnect_presentation(void)
{
    MenuTextureDrawTrace texture_trace;
    MenuDrawTrace text_trace;
    CharacterDrawTrace p1_trace;
    P2CharacterDrawTrace p2_trace;
    _Material reconnect_material;

    reset_menu_state();
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&p1_trace, 0, sizeof(p1_trace));
    memset(&p2_trace, 0, sizeof(p2_trace));
    memset(&reconnect_material, 0, sizeof(reconnect_material));
    generateAllText(0);
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustmentMM = 0.5f;
    menuTextures[241] = &reconnect_material;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    jpb_MenuSetP1CharacterSelectDrawHook(
        capture_p1_character_select, &p1_trace);
    jpb_MenuSetP2CharacterSelectDrawHook(
        capture_p2_character_select, &p2_trace);

    p1Disconnected = 1;
    GameStruct.gameMode = 9;
    menu_drawReconnect();
    CHECK(p1_trace.calls == 0);
    CHECK(text_trace.calls == 1);
    CHECK(text_trace.tint[0] == 15);
    CHECK(text_trace.mode[0] == 0);
    CHECK(text_trace.x[0] == 242);
    CHECK(text_trace.y[0] == 217);
    CHECK(text_trace.scale[0] == 2.5f);
    CHECK(utf16_prefix_matches_utf8(
              text_trace.text[0], allText[494], 63));
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &reconnect_material);
    CHECK(texture_trace.destinations[0].left == 217);
    CHECK(texture_trace.destinations[0].top == 200);
    CHECK(texture_trace.destinations[0].right == 742);
    CHECK(texture_trace.destinations[0].bottom == 386);
    CHECK(texture_trace.colors[0].r == 255);
    CHECK(texture_trace.colors[0].g == 255);
    CHECK(texture_trace.colors[0].b == 255);
    CHECK(texture_trace.colors[0].cd == 255);
    CHECK(texture_trace.layers[0] == 0.99f);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    GameStruct.gameMode = 6;
    CHECK(newMenu_P1CharacterSelect() == 0);
    CHECK(p1_trace.calls == 0);
    CHECK(text_trace.calls == 1);
    CHECK(texture_trace.calls == 1);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    p1Disconnected = 0;
    p2Disconnected = 1;
    CHECK(newMenu_P2CharacterSelect(0) == 0);
    CHECK(p2_trace.calls == 0);
    CHECK(text_trace.calls == 1);
    CHECK(text_trace.y[0] == 167);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.destinations[0].top == 150);
    CHECK(texture_trace.destinations[0].bottom == 336);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    p2Disconnected = 0;
    p1Disconnected = 1;
    GameStruct.gameMode = 6;
    CHECK(newMenu_Training() == 0);
    CHECK(text_trace.calls == 1);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &reconnect_material);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    menuConceptMenu();
    CHECK(text_trace.calls == 1);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &reconnect_material);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0;
    menu_mainLoop();
    CHECK(text_trace.calls == 1);
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &reconnect_material);

    p1Disconnected = 0;
    menuVars.menuModeSP = 1;
    menuVars.menuMode[0] = 0;
    menuVars.menuMode[1] = 0x0e;
    menu_drawReconnect();
    CHECK(menuVars.menuModeSP == 0);

    jpb_MenuSetP2CharacterSelectDrawHook(NULL, NULL);
    jpb_MenuSetP1CharacterSelectDrawHook(NULL, NULL);
    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    menuTextures[241] = NULL;
    return 0;
}

static int test_character_select_controller_tabs(void)
{
    MenuTextureDrawTrace texture_trace;
    _Material primary;
    _Material secondary;
    unsigned index;

    reset_menu_state();
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&primary, 0, sizeof(primary));
    memset(&secondary, 0, sizeof(secondary));
    memset(kbmTextures, 0, sizeof(kbmTextures));
    for (index = 0;
         index < sizeof(menuTextures) / sizeof(menuTextures[0]);
         ++index) {
        menuTextures[index] =
            (_Material *)(uintptr_t)(index + 1u);
    }
    primary.iw = 200;
    primary.ih = 100;
    secondary.iw = 200;
    secondary.ih = 100;
    kbmTextures[0] = &primary;
    kbmTextures[1] = &secondary;
    lastUsedInputType = 0;
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    OptionStruct.ResolutionChanged = 0;
    scaleAdjustmentMM = 0.5f;
    generateAllText(0);
    GameStruct.gameCompleted = 1;
    GameStruct.NumPlayers = 1;
    GameStruct.ModelSelect[0] = obi_wan_model;
    newMenu_currentModelSelectBaseP1 = obi_wan_model;
    newMenu_state = 0x18;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);

    CHECK(newMenu_P1CharacterSelect() == 0);
    CHECK(texture_trace.calls == 21);
    CHECK(texture_trace.materials[3] == &secondary);
    CHECK(texture_trace.materials[4] == &primary);
    CHECK(texture_trace.destinations[3].left == 280);
    CHECK(texture_trace.destinations[3].right == 330);
    CHECK(texture_trace.destinations[3].top == 50);
    CHECK(texture_trace.destinations[3].bottom == 75);
    CHECK(texture_trace.destinations[4].left == 630);
    CHECK(texture_trace.destinations[4].right == 680);

    memset(&texture_trace, 0, sizeof(texture_trace));
    GameStruct.NumPlayers = 2;
    GameStruct.ModelSelect[1] = qui_gon_model;
    newMenu_currentModelSelectBaseP2 = qui_gon_model;
    newMenu_state = 0x18;
    CHECK(newMenu_P2CharacterSelect(0) == 0);
    CHECK(texture_trace.calls == 26);
    CHECK(texture_trace.materials[7] == &secondary);
    CHECK(texture_trace.materials[8] == &primary);
    CHECK(texture_trace.destinations[7].left == 35);
    CHECK(texture_trace.destinations[7].right == 85);
    CHECK(texture_trace.destinations[8].left == 385);
    CHECK(texture_trace.destinations[8].right == 435);
    CHECK(texture_trace.materials[17] == &primary);
    CHECK(texture_trace.materials[18] == &secondary);
    CHECK(texture_trace.destinations[17].left == 875);
    CHECK(texture_trace.destinations[17].right == 925);
    CHECK(texture_trace.destinations[18].left == 525);
    CHECK(texture_trace.destinations[18].right == 575);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    memset(menuTextures, 0, sizeof(menuTextures));
    memset(kbmTextures, 0, sizeof(kbmTextures));
    return 0;
}

static int test_keyboard_prompt_glyph_owner(void)
{
    MenuTextureDrawTrace texture_trace;
    MenuDrawTrace text_trace;
    _Material escape_material;
    _Material select_material;
    SCREENRECT scissor = {-1, -1, -1, -1};

    reset_menu_state();
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&escape_material, 0, sizeof(escape_material));
    memset(&select_material, 0, sizeof(select_material));
    escape_material.iw = 128;
    escape_material.ih = 128;
    select_material.iw = 128;
    select_material.ih = 128;
    kbmTextures[9] = &escape_material;
    kbmTextures[2] = &select_material;
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustmentMM = 0.5f;
    generateAllText(0);
    jpb_WHookSetDrawTextureClippedHook(
        capture_menu_texture_draw_clipped, &texture_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);

    drawControlsIcon();

    CHECK(texture_trace.calls == 0);
    CHECK(text_trace.calls == 2);
    CHECK(utf16_matches_utf8(text_trace.text[0], allText[476]));
    CHECK(utf16_matches_utf8(text_trace.text[1], allText[475]));
    CHECK(text_trace.x[0] == 50);
    CHECK(text_trace.x[1] == 910);
    CHECK(text_trace.y[0] == 490);
    CHECK(text_trace.y[1] == 490);
    CHECK(text_trace.mode[0] == 0);
    CHECK(text_trace.mode[1] == 1);

    scaleAdjustmentMM = 1.0f;
    newDrawControllerIcon(
        9, 0.35f, 130, 119, 255, 0, scissor);
    newDrawControllerIconDepth(
        2, 0.35f, 820, 119, 255, 0, scissor, 0.25f);
    CHECK(texture_trace.calls == 2);
    CHECK(texture_trace.materials[0] == &escape_material);
    CHECK(texture_trace.materials[1] == &select_material);
    CHECK(texture_trace.destinations[0].left == 107);
    CHECK(texture_trace.destinations[0].top == 98);
    CHECK(texture_trace.destinations[0].right == 151);
    CHECK(texture_trace.destinations[0].bottom == 142);
    CHECK(texture_trace.destinations[1].left == 797);
    CHECK(texture_trace.destinations[1].top == 98);
    CHECK(texture_trace.destinations[1].right == 841);
    CHECK(texture_trace.destinations[1].bottom == 142);
    CHECK(texture_trace.layers[0] == 0.0f);
    CHECK(texture_trace.layers[1] == 0.25f);
    CHECK(texture_trace.hasScissor[0] == 1);
    CHECK(texture_trace.scissors[0].left == -1);
    CHECK(texture_trace.scissors[0].bottom == -1);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureClippedHook(NULL, NULL);
    kbmTextures[9] = NULL;
    kbmTextures[2] = NULL;
    return 0;
}

static int test_training_selection_owner(void)
{
    MenuTextureDrawTrace texture_trace;
    MenuDrawTrace text_trace;
    unsigned index;

    reset_menu_state();
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    for (index = 0;
         index < sizeof(menuTextures) / sizeof(menuTextures[0]);
         ++index) {
        menuTextures[index] =
            (_Material *)(uintptr_t)(index + 1u);
    }
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    OptionStruct.ResolutionChanged = 0;
    scaleAdjustmentMM = 0.5f;
    generateAllText(0);
    lastUsedInputType = 1;
    GameStruct.NumPlayers = 1;
    GameStruct.ModelSelect[0] = obi_wan_model;
    newMenu_state = 0x16;
    newMenu_trainLevel = 1;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);

    CHECK(newMenu_Training() == 0);
    CHECK(texture_trace.calls == 18);
    CHECK(texture_trace.materials[0] == menuTextures[175]);
    CHECK(texture_trace.materials[1] == menuTextures[172]);
    CHECK(texture_trace.materials[2] == menuTextures[173]);
    CHECK(texture_trace.materials[3] == menuTextures[177]);
    CHECK(texture_trace.materials[4] == menuTextures[190]);
    CHECK(texture_trace.materials[5] == menuTextures[183]);
    CHECK(texture_trace.materials[6] == menuTextures[201]);
    CHECK(texture_trace.materials[10] == menuTextures[189]);
    CHECK(texture_trace.materials[11] == menuTextures[178]);
    CHECK(texture_trace.materials[12] == menuTextures[190]);
    CHECK(texture_trace.materials[13] == menuTextures[183]);
    CHECK(texture_trace.materials[14] == menuTextures[227]);
    CHECK(texture_trace.materials[17] == menuTextures[185]);
    CHECK(text_trace.calls == 8);
    CHECK(utf16_matches_utf8(text_trace.text[2], allText[481]));
    CHECK(utf16_matches_utf8(text_trace.text[3], allText[332]));
    CHECK(utf16_matches_utf8(text_trace.text[6], allText[483]));
    CHECK(utf16_matches_utf8(text_trace.text[7], allText[321]));

    reset_menu_state();
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    for (index = 0;
         index < sizeof(menuTextures) / sizeof(menuTextures[0]);
         ++index) {
        menuTextures[index] =
            (_Material *)(uintptr_t)(index + 1u);
    }
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustmentMM = 0.5f;
    generateAllText(0);
    lastUsedInputType = 1;
    GameStruct.ModelSelect[0] = obi_wan_model;
    GameStruct.aCharacterData[0].Items = 7;
    GameStruct.aCharacterData[1].Items = 8;
    jediUpgrades[obi_wan_model].lifeUpgrades = 7;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);

    CHECK(newMenu_Training() == 0);
    CHECK(newMenu_state == 1);
    CHECK(newMenu_Training() == 0);
    CHECK(newMenu_state == 0x16);
    CHECK(GameStruct.NumPlayers == 1);
    CHECK(GameStruct.mNumContinues == 1004);
    CHECK(GameStruct.aCharacterData[0].Items == 0);
    CHECK(GameStruct.aCharacterData[1].Items == 0);

    menuVars.pad[0] = JPB_PAD_LEFT;
    CHECK(newMenu_Training() == 0);
    CHECK(GameStruct.ModelSelect[0] == qui_gon_model);
    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    CHECK(newMenu_Training() == 0);
    CHECK(newMenu_state == 0x17);
    CHECK(newMenu_select == UINT32_C(1));
    menuVars.pad[0] = JPB_PAD_LEFT;
    CHECK(newMenu_Training() == 0);
    CHECK(newMenu_trainLevel == 2);
    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    CHECK(newMenu_Training() == 0);
    CHECK(newMenu_state == 0x0e);
    CHECK(newMenu_select == UINT32_C(3));
    menuVars.pad[0] = 0;
    CHECK(newMenu_Training() == 1);
    CHECK(newMenu_state == 0);
    CHECK(menuVars.trainingLevel == 1);
    CHECK(LevelSelect == 17);
    CHECK(GameStruct.gameMode == 2);

    newMenu_state = 0x16;
    newMenu_bAbortMenu = 0;
    menuVars.pad[0] = JPB_PAD_JUMP;
    CHECK(newMenu_Training() == 0);
    CHECK(newMenu_state == 0x0e);
    menuVars.pad[0] = 0;
    CHECK(newMenu_Training() == -1);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    memset(menuTextures, 0, sizeof(menuTextures));
    return 0;
}

static int test_level_selection_owner(void)
{
    MenuDrawTrace text_trace;
    MenuTextureDrawTrace texture_trace;
    MenuTextureTrace load_trace;
    PlatformTrace platform_trace;
    JPBMenuPlatformHooks hooks;

    reset_menu_state();
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&load_trace, 0, sizeof(load_trace));
    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&hooks, 0, sizeof(hooks));
    hooks.soundCue = trace_sound_cue;
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    scaleAdjustment = getScaleAdjustment();
    scaleAdjustmentMM = getScaleAdjustmentMM();
    generateAllText(0);
    memset(menuTextures, 0, sizeof(menuTextures));
    memset(fontSpec, 0, sizeof(fontSpec));
    CHECK(jpb_ResourceSetBasePath("C:/jpb-menu-test"));
    jpb_TextureSetPlatformHooks(
        load_menu_texture, unload_menu_texture, &load_trace);
    menu_winLoadTextures();
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);

    CHECK(levelSelectMdef[0] == 0);
    CHECK(levelSelectMdef[9] == 9);
    CHECK(levelSelectMdef[12] == 0x3d);
    CHECK(levelSelectMdef[13] == 8);
    CHECK(levelSelectMdef[14] == 0x14);
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x0e;
    LevelSelect = 9;
    fontSpec[411].clut = 80;
    menu_pushMenu(0x1a);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 0x1a);
    menu_initNewMenu();
    CHECK(menuVars.menuModeSP == 3);
    CHECK(menuVars.menuMode[3] == 0x1a);
    CHECK(LevelSelect == 1);

    menu_drawLevelSelectScreen(0);
    CHECK(texture_trace.calls == 40);
    CHECK(texture_trace.materials[0] == menuTextures[164]);
    CHECK(texture_trace.materials[1] == menuTextures[165]);
    CHECK(texture_trace.materials[2] == menuTextures[3]);
    CHECK(texture_trace.materials[4] == menuTextures[80]);
    CHECK(texture_trace.materials[8] == menuTextures[3]);
    CHECK(texture_trace.materials[16] == menuTextures[3]);
    CHECK(texture_trace.layers[0] == 0.9f);
    CHECK(texture_trace.layers[1] == 0.4f);
    CHECK(texture_trace.layers[2] == 0.001f);
    CHECK(texture_trace.layers[3] == 0.002f);
    CHECK(texture_trace.layers[4] == 0.5f);
    CHECK(texture_trace.layers[39] == 1.0f);
    CHECK(texture_trace.destinations[0].left == 0);
    CHECK(texture_trace.destinations[0].top == 0);
    CHECK(texture_trace.destinations[0].right == 1920);
    CHECK(texture_trace.destinations[0].bottom == 1080);
    CHECK(texture_trace.destinations[1].left == 0);
    CHECK(texture_trace.destinations[1].top == 0);
    CHECK(texture_trace.destinations[1].right == 1920);
    CHECK(texture_trace.destinations[1].bottom == 1080);
    CHECK(texture_trace.destinations[4].left == 116);
    CHECK(texture_trace.destinations[4].top == 92);
    CHECK(texture_trace.destinations[4].right == 960);
    CHECK(texture_trace.destinations[4].bottom == 725);
    CHECK(texture_trace.destinations[5].left >= 1150);
    CHECK(texture_trace.destinations[6].left >= 1350);
    CHECK(texture_trace.destinations[7].left >= 1560);
    CHECK(texture_trace.destinations[16].left >= 1398);
    CHECK(texture_trace.destinations[16].right > texture_trace.destinations[16].left);
    CHECK(texture_trace.materials[39] == whitemat);
    CHECK(texture_trace.destinations[39].left == 0);
    CHECK(texture_trace.destinations[39].top == 0);
    CHECK(texture_trace.destinations[39].right == 1920);
    CHECK(texture_trace.destinations[39].bottom == 1080);
    CHECK(texture_trace.colors[39].r == 0);
    CHECK(texture_trace.colors[39].g == 0);
    CHECK(texture_trace.colors[39].b == 2);
    CHECK(texture_trace.colors[39].cd == 255);
    CHECK(texture_trace.sources[2].left == 216);
    CHECK(texture_trace.sources[2].top == 0);
    CHECK(texture_trace.sources[2].right == 328);
    CHECK(texture_trace.sources[2].bottom == 104);
    CHECK(texture_trace.sources[3].left == 104);
    CHECK(texture_trace.sources[3].top == 0);
    CHECK(texture_trace.sources[3].right == 216);
    CHECK(texture_trace.sources[3].bottom == 104);
    CHECK(texture_trace.sources[5].left == 712);
    CHECK(texture_trace.sources[5].top == 248);
    CHECK(texture_trace.sources[5].right == 884);
    CHECK(texture_trace.sources[5].bottom == 316);
    CHECK(texture_trace.sources[16].left == 32);
    CHECK(texture_trace.sources[16].top == 340);
    CHECK(texture_trace.sources[16].right == 64);
    CHECK(texture_trace.sources[16].bottom == 384);
    CHECK(text_trace.calls == 4);
    CHECK(utf16_matches_utf8(text_trace.text[0], allText[190]));
    CHECK(utf16_matches_utf8(text_trace.text[1], allText[476]));
    CHECK(utf16_matches_utf8(text_trace.text[2], allText[475]));
    CHECK(utf16_matches_utf8(text_trace.text[3], allText[306]));
    CHECK(text_trace.scale[0] == 2.5f);
    CHECK(text_trace.scale[1] == 1.75f);
    CHECK(text_trace.scale[3] == 2.5f);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    lastUsedInputType = 1;
    menu_test_controller_name = "Generic Controller";
    hooks.controllerName = read_menu_controller_name;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    menu_drawLevelSelectScreen(0);
    CHECK(texture_trace.calls == 40);
    CHECK(texture_trace.materials[0] == menuTextures[164]);
    CHECK(texture_trace.materials[1] == menuTextures[165]);
    CHECK(texture_trace.materials[37] ==
          menuTextures[fontSpec[0x113].clut]);
    CHECK(texture_trace.materials[38] ==
          menuTextures[fontSpec[0x114].clut]);
    CHECK(texture_trace.materials[39] == whitemat);
    CHECK(texture_trace.destinations[37].left == 1458);
    CHECK(texture_trace.destinations[37].top == 94);
    CHECK(texture_trace.destinations[38].left == 1758);
    CHECK(texture_trace.destinations[38].top == 274);
    CHECK(text_trace.calls == 4);
    CHECK(utf16_matches_utf8(text_trace.text[1], allText[239]));
    CHECK(utf16_matches_utf8(text_trace.text[2], allText[241]));

    secretBits = 0;
    GameStruct.NumPlayers = 1;
    menuVars.pad[0] = JPB_PAD_DOWN;
    menu_levelSelectMenu(levelSelectMdef);
    CHECK(LevelSelect == 2);
    CHECK(strcmp(platform_trace.lastCue, "xlvbrows") == 0);
    LevelSelect = 10;
    menu_levelSelectMenu(levelSelectMdef);
    CHECK(LevelSelect == 1);
    secretBits = 1;
    LevelSelect = 10;
    menu_levelSelectMenu(levelSelectMdef);
    CHECK(LevelSelect == 11);

    LevelSelect = 1;
    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    menu_levelSelectMenu(levelSelectMdef);
    CHECK(menuVars.menuModeSP == 4);
    CHECK(menuVars.menuMode[4] == 0x66);
    CHECK(savedNumPlayer == 1);
    CHECK(strcmp(platform_trace.lastCue, "xlvselct") == 0);

    menuVars.menuModeSP = 1;
    menuVars.pad[0] = JPB_PAD_JUMP;
    menu_levelSelectMenu(levelSelectMdef);
    CHECK(menuVars.menuModeSP == 0);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    texture_Flush((unsigned)TT_ANY);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_legacy_level_selection_renderer(void)
{
    static const unsigned spec_indices[] = {
        0xe9u, 0xeau, 421u, 0x113u, 0x114u
    };
    static const unsigned material_indices[] = {
        164u, 165u, 100u, 101u, 102u, 103u, 104u
    };
    FONTSPEC saved_specs[
        sizeof(spec_indices) / sizeof(spec_indices[0])];
    _Material *saved_materials[
        sizeof(material_indices) / sizeof(material_indices[0])];
    _Material materials[
        sizeof(material_indices) / sizeof(material_indices[0])];
    MenuDrawTrace text_trace;
    MenuTextureDrawTrace texture_trace;
    float saved_scale_w = gPSXDrawScaleW;
    float saved_scale_h = gPSXDrawScaleH;
    size_t index;

    reset_menu_state();
    generateAllText(0);
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(materials, 0, sizeof(materials));
    for (index = 0;
         index < sizeof(spec_indices) / sizeof(spec_indices[0]);
         ++index) {
        saved_specs[index] = fontSpec[spec_indices[index]];
    }
    for (index = 0;
         index < sizeof(material_indices) / sizeof(material_indices[0]);
         ++index) {
        saved_materials[index] = menuTextures[material_indices[index]];
        menuTextures[material_indices[index]] = &materials[index];
    }
    fontSpec[0xe9] = (FONTSPEC){0, 100, 1, 2, 10, 20};
    fontSpec[0xea] = (FONTSPEC){0, 101, 3, 4, 11, 21};
    fontSpec[421] = (FONTSPEC){0, 102, 0, 0, 0, 0};
    fontSpec[0x113] = (FONTSPEC){0, 103, 0, 0, 8, 5};
    fontSpec[0x114] = (FONTSPEC){0, 104, 0, 0, 8, 2};
    OptionStruct.ScreenWidth = 1280;
    OptionStruct.ScreenHeight = 720;
    gPSXDrawScaleW = 1.0f;
    gPSXDrawScaleH = 1.0f;
    frontZ = 0.25f;
    LevelSelect = 12;
    menuVars.artLevel = 1;
    menuVars.artload = 1;
    menuVars.artloadPos = -32;
    menuVars.dstSelector = 1;
    menuVars.mmv[0].mmvY = INT32_C(10) << 16;
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);

    menu_drawLevelSelectScreen_OLD(0);
    CHECK(texture_trace.calls >= 8);
    CHECK(texture_trace.materials[0] == &materials[0]);
    CHECK(texture_trace.materials[1] == &materials[1]);
    CHECK(texture_trace.destinations[0].left == 0);
    CHECK(texture_trace.destinations[0].top == 0);
    CHECK(texture_trace.destinations[0].right == 1280);
    CHECK(texture_trace.destinations[0].bottom == 720);
    CHECK(texture_trace.layers[0] == 0.9f);
    CHECK(texture_trace.layers[1] == 0.4f);
    CHECK(texture_trace.materials[2] == &materials[2]);
    CHECK(texture_trace.destinations[2].left == 825);
    CHECK(texture_trace.destinations[2].top == 102);
    CHECK(texture_trace.materials[3] == &materials[3]);
    CHECK(texture_trace.destinations[3].left == 895);
    CHECK(texture_trace.destinations[3].top == 102);
    CHECK(texture_trace.materials[4] == &materials[4]);
    CHECK(texture_trace.destinations[4].left == 5);
    CHECK(texture_trace.destinations[4].top == 63);
    CHECK(texture_trace.destinations[4].right == 560);
    CHECK(texture_trace.destinations[4].bottom == 483);
    CHECK(texture_trace.colors[4].r == 225u);
    CHECK(texture_trace.colors[4].g == 225u);
    CHECK(texture_trace.colors[4].b == 225u);
    CHECK(texture_trace.colors[4].cd == 255u);
    CHECK(text_trace.calls == 4);
    CHECK(utf16_matches_utf8(text_trace.text[0], allText[190]));
    CHECK(utf16_matches_utf8(text_trace.text[1], allText[476]));
    CHECK(text_trace.x[1] == 1017);
    CHECK(text_trace.y[1] == 108);
    CHECK(text_trace.scale[1] == 2.9f);
    CHECK(utf16_matches_utf8(text_trace.text[2], allText[475]));
    CHECK(text_trace.x[2] == 1017);
    CHECK(text_trace.y[2] == 150);
    CHECK(utf16_matches_utf8(text_trace.text[3], allText[317]));
    CHECK(text_trace.x[3] == 112);
    CHECK(text_trace.y[3] == 120);
    CHECK(text_trace.scale[3] == 2.5f);
    CHECK(menuVars.artloadPos == -16);
    CHECK(gPSXDrawScaleX == 1.0f);
    CHECK(gPSXDrawScaleY == 1.0f);

    memset(&text_trace, 0, sizeof(text_trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    menuVars.mmvTriggers[0] = 1;
    menuVars.pad[0] = JPB_PAD_DOWN;
    menuVars.fadeupCounter = 0;
    menu_drawLevelSelectScreen_OLD(1);
    CHECK(LevelSelect == 12);
    CHECK(texture_trace.calls == 3);
    CHECK(texture_trace.materials[0] == &materials[0]);
    CHECK(texture_trace.materials[1] == &materials[1]);
    CHECK(texture_trace.materials[2] == whitemat);
    CHECK(text_trace.calls == 1);
    CHECK(utf16_matches_utf8(text_trace.text[0], allText[190]));
    CHECK(gPSXDrawScaleX == 1.0f);
    CHECK(gPSXDrawScaleY == 1.0f);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    jpb_TextSetDrawHook(NULL, NULL);
    for (index = 0;
         index < sizeof(spec_indices) / sizeof(spec_indices[0]);
         ++index) {
        fontSpec[spec_indices[index]] = saved_specs[index];
    }
    for (index = 0;
         index < sizeof(material_indices) / sizeof(material_indices[0]);
         ++index) {
        menuTextures[material_indices[index]] = saved_materials[index];
    }
    gPSXDrawScaleW = saved_scale_w;
    gPSXDrawScaleH = saved_scale_h;
    return 0;
}

static int test_title_command_interpreter(void)
{
    MenuDrawTrace trace;
    MenuTextureDrawTrace texture_trace;
    JPBMenuPlatformHooks hooks;
    PlatformTrace platform_trace;

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&hooks, 0, sizeof(hooks));
    memset(&platform_trace, 0, sizeof(platform_trace));
    hooks.activateItem = trace_activate_item;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    g_resolutionsCount = 2;
    g_resolutions[0].width = 1280;
    g_resolutions[0].height = 720;
    g_resolutions[1].width = 1920;
    g_resolutions[1].height = 1080;
    scaleAdjustment = getScaleAdjustment();
    scaleAdjustmentMM = getScaleAdjustmentMM();
    generateAllText(0);
    jpb_TextSetDrawHook(capture_menu_text, &trace);
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);

    CHECK(sizeof(MMVDEF) == 48);
    CHECK(sizeof(MDEF_MOD) == 32);
    CHECK(mmNextCode(mainMdef, 0) == 2);
    CHECK(mmNextCode(mainMdef, 10) == 14);
    CHECK(mmNextCode(mainMdef, 38) == 41);
    CHECK(mmsizes[0] == 2);
    CHECK(mmsizes[8] == 4);
    CHECK(mmsizes[0x41] == 3);
    CHECK(mmsizes[0x4a] == 2);

    menuVars.menuModeSP = 0;
    menuVars.mmSelect1[0] = 0;
    menuVars.mmColorSelect = 14;
    menuVars.mmColorNotSelect = 15;
    menu_mainMenu(mainMdef);

    CHECK(trace.calls == 13);
    CHECK(jpb_utf16_compare(trace.text[0], L"> New Game <") == 0);
    CHECK(jpb_utf16_compare(trace.text[1], L"Training") == 0);
    CHECK(jpb_utf16_compare(trace.text[2], L"VS. Mode") == 0);
    CHECK(jpb_utf16_compare(trace.text[3], L"Options") == 0);
    CHECK(jpb_utf16_compare(trace.text[4], L"Quit") == 0);
    CHECK(jpb_utf16_compare(trace.text[5], L"Register Your Game") == 0);
    CHECK(trace.tint[0] == 14);
    CHECK(trace.tint[1] == 15);
    CHECK(trace.x[0] == 480);
    CHECK(trace.y[0] == 362);
    CHECK(trace.y[1] == 392);
    CHECK(trace.scale[0] == 2.25f);
    CHECK(trace.scaleAdjustment[0] == 0.5f);
    CHECK(trace.clipEnabled[0] == 1);
    CHECK(trace.clipLeft[0] == 290);
    CHECK(trace.clipTop[0] == 342);
    CHECK(trace.clipRight[0] == 669);
    CHECK(trace.clipBottom[0] == 407);
    CHECK(trace.clipEnabled[5] == 1);
    CHECK(menuVars.mmSelectPtr == &mainMdef[10]);
    CHECK(gDST.left == 268);
    CHECK(gDST.right == 691);
    CHECK(gDST.top == 337);
    CHECK(gDST.bottom == 412);
    CHECK(gColor.r == 255 && gColor.g == 255);
    CHECK(gColor.b == 255 && gColor.cd == 0xbe);

    memset(&trace, 0, sizeof(trace));
    lastUsedInputType = 0;
    mmDraw(exitSelectMdef);
    CHECK(trace.calls == 2);
    CHECK(utf16_matches_utf8(trace.text[0], allText[476]));
    CHECK(utf16_matches_utf8(trace.text[1], allText[475]));
    memset(&trace, 0, sizeof(trace));
    lastUsedInputType = 1;
    mmDraw(exitSelectMdef);
    CHECK(trace.calls == 2);
    CHECK(utf16_matches_utf8(trace.text[0], allText[239]));
    CHECK(utf16_matches_utf8(trace.text[1], allText[241]));
    lastUsedInputType = 0;

    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    menu_mainMenu(mainMdef);
    CHECK(platform_trace.activationCalls == 1);
    CHECK(platform_trace.activatedDestination == 144);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 0x90);
    menuVars.menuModeSP = 0;
    menuVars.pad[0] = JPB_PAD_DOWN;
    menu_mainMenu(mainMdef);
    CHECK(menuVars.mmSelect1[0] == 1);
    CHECK(menuVars.yoffset == 30);
    menuVars.yoffset = 0;
    menuVars.pad[0] = JPB_PAD_UP;
    menu_mainMenu(mainMdef);
    CHECK(menuVars.mmSelect1[0] == 0);
    CHECK(menuVars.yoffset == -30);

    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    menuVars.pad[0] = 0;
    menuVars.yoffset = 0;
    menuVars.mmSelect1[0] = 0;
    mmDraw(newgameconfirmMdef);
    CHECK(trace.calls == 5);
    CHECK(menuVars.mmSelectPtr == &newgameconfirmMdef[7]);
    CHECK(menuVars.mmSelectPtr[3] == 9);
    CHECK(jpb_utf16_compare(trace.text[0], L"> YES <") == 0);
    CHECK(jpb_utf16_compare(trace.text[1], L"NO") == 0);
    CHECK(jpb_utf16_compare(trace.text[2], L"Overwrite save game?") == 0);
    CHECK(trace.mode[0] == 2);
    CHECK(trace.mode[1] == 2);
    CHECK(trace.mode[2] == 2);
    CHECK(trace.x[0] == 480);
    CHECK(trace.y[0] == 357);
    CHECK(trace.x[1] == 480);
    CHECK(trace.y[1] == 387);
    CHECK(trace.x[2] == 480);
    CHECK(trace.y[2] == 312);
    CHECK(trace.scale[0] == 2.25f);
    CHECK(trace.scaleAdjustment[0] == 0.5f);
    {
        int found_confirm_box = 0;
        int texture_index;

        for (texture_index = 0;
             texture_index < texture_trace.calls;
             ++texture_index) {
            const SCREENRECT *destination =
                &texture_trace.destinations[texture_index];

            if (destination->left == 330 &&
                destination->right == 630 &&
                destination->top == 292 &&
                destination->bottom == 426 &&
                texture_trace.colors[texture_index].cd == 0xbe) {
                found_confirm_box = 1;
            }
        }
        CHECK(found_confirm_box == 1);
    }
    memset(&texture_trace, 0, sizeof(texture_trace));
    OptionStruct.ResolutionChanged = 3;
    mmDraw(newgameconfirmMdef);
    OptionStruct.ResolutionChanged = 0;
    {
        int found_confirm_box = 0;
        int texture_index;

        for (texture_index = 0;
             texture_index < texture_trace.calls;
             ++texture_index) {
            const SCREENRECT *destination =
                &texture_trace.destinations[texture_index];

            if (destination->left == 292 &&
                destination->right == 667 &&
                destination->top == 292 &&
                destination->bottom == 426 &&
                texture_trace.colors[texture_index].cd == 0xbe) {
                found_confirm_box = 1;
            }
        }
        CHECK(found_confirm_box == 1);
    }
    memset(&texture_trace, 0, sizeof(texture_trace));
    mmDraw(optionsMdef);
    {
        int found_options_box = 0;
        int texture_index;

        for (texture_index = 0;
             texture_index < texture_trace.calls;
             ++texture_index) {
            const SCREENRECT *destination =
                &texture_trace.destinations[texture_index];

            if (destination->left == 200 &&
                destination->right == 759 &&
                destination->top == 216 &&
                destination->bottom == 459 &&
                texture_trace.colors[texture_index].cd == 255) {
                found_options_box = 1;
            }
        }
        CHECK(found_options_box == 1);
    }

    memset(&trace, 0, sizeof(trace));
    menuVars.mmSelect1[0] = 1;
    OptionStruct.ResolutionChanged = 1;
    mmDraw(videoMdef);
    CHECK(trace.calls == 6);
    CHECK(jpb_utf16_compare(trace.text[1], L"> Resolution: 1920x1080 <") == 0);

    menuVars.menuMode[0] = 1;
    menuVars.menuModeSP = 0;
    CHECK(menu_handleMenuTriggers(0x90) == 0);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 0x90);
    CHECK(menu_handleMenuTriggers(0x53) == 0);
    CHECK(menuVars.menuModeSP == 0);
    CHECK(menu_handleMenuTriggers(9) == 0);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 3);
    menuVars.menuModeSP = 0;
    CHECK(menu_handleMenuTriggers(0x9d) == 0);
    CHECK(menuVars.menuMode[1] == 0x9c);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_main_loop_dispatch(void)
{
    MenuDrawTrace trace;
    MenuTextureDrawTrace texture_trace;
    MenuInputTrace input_trace;
    JPBMenuPlatformHooks hooks;
    PlatformTrace platform_trace;

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&input_trace, 0, sizeof(input_trace));
    memset(&hooks, 0, sizeof(hooks));
    memset(&platform_trace, 0, sizeof(platform_trace));
    hooks.openUrl = trace_open_url;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    scaleAdjustmentMM = getScaleAdjustmentMM();
    generateAllText(0);
    jpb_TextSetDrawHook(capture_menu_text, &trace);
    jpb_InputSetProvider(NULL, NULL);

    menuVars.menuModeSP = 0;
    menuVars.mmColorSelect = 14;
    menuVars.mmColorNotSelect = 15;
    menuVars.menuMode[0] = 0;
    m_canShowRegisterGame = 1;
    GameStruct.continueAble = 0;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr == &mainMdef[10]);

    ClearInput();
    jpb_InputSetProvider(read_menu_pad, &input_trace);
    GameStruct.inMenuFlag = 1;
    GameStruct.gameMode = 0;
    menu_mainLoop();
    input_trace.pads[0] = JPB_PAD_START;
    menu_mainLoop();
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 3);
    input_trace.pads[0] = 0;
    jpb_InputSetProvider(NULL, NULL);
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0;

    GameStruct.continueAble = 1;
    GameStruct.difficulty = 0;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= continuemainMdef);
    CHECK(menuVars.mmSelectPtr <
          continuemainMdef + 73);

    menuVars.menuMode[0] = 0x0b;
    slider = 0;
    padCurrentBits[0].padLevel1 = UINT32_C(0x8000);
    menu_mainLoop();
    CHECK(slider == 0);
    slider = UINT32_C(0xfe);
    padCurrentBits[0].padLevel1 = UINT32_C(0x2000);
    menu_mainLoop();
    CHECK(slider == UINT32_C(0xff));
    menu_mainLoop();
    CHECK(slider == UINT32_C(0xff));
    padCurrentBits[0].padLevel1 = UINT32_C(0x8000);
    menu_mainLoop();
    CHECK(slider == UINT32_C(0xfe));
    padCurrentBits[0].padLevel1 = 0;

    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x8f;
    menu_mainLoop();
    CHECK(platform_trace.openUrlCalls == 1);
    CHECK(strcmp(
              platform_trace.openedUrl,
              "https://ctep.aspyr.com/pb_pc") == 0);
    CHECK(menuVars.menuModeSP == 2);
    CHECK(menuVars.menuMode[1] == 0);
    CHECK(menuVars.menuMode[2] == 0);
    menuVars.menuModeSP = 0;

    menuVars.menuMode[0] = 4;
    GameStruct.NumPlayers = 2;
    abGlobalBits[0] = 0;
    abGlobalBits[1] = UINT8_C(0x1f);
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr == &playerCountSelectMdef[7]);
    CHECK((abGlobalBits[0] & UINT8_C(1u << 2)) != 0);
    CHECK((abGlobalBits[1] & UINT8_C(0x1f)) == 0);

    menuVars.menuMode[0] = 0x10;
    GameStruct.gameMode = 6;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= audioMdef_Game);
    CHECK(menuVars.mmSelectPtr < audioMdef_Game + 44);
    GameStruct.gameMode = 0;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= audioMdef);
    CHECK(menuVars.mmSelectPtr < audioMdef + 44);

    menuVars.menuMode[0] = 0x24;
    menuVars.controlPlayer = 0;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= controls1Mdef);
    CHECK(menuVars.mmSelectPtr < controls1Mdef + 42);
    menuVars.controlPlayer = 1;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= controls1Mdef);
    CHECK(menuVars.mmSelectPtr < controls1Mdef + 42);
    GameStruct.NumPlayers = 2;
    padExist = 2;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= controls2Mdef);
    CHECK(menuVars.mmSelectPtr < controls2Mdef + 42);

    menuVars.menuMode[0] = 0x23;
    menuVars.controlPlayer = 1;
    GameStruct.NumPlayers = 1;
    padExist = 0;
    menu_mainLoop();
    CHECK(menuVars.controlPlayer == 0);
    CHECK(menuVars.mmSelectPtr >= controls1Mdef);
    CHECK(menuVars.mmSelectPtr < controls1Mdef + 42);

    memset(&trace, 0, sizeof(trace));
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    menuVars.menuMode[0] = 0x2d;
    GameStruct.GameState = 0;
    menu_mainLoop();
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) != 0);
    CHECK(trace.calls == 2);
    CHECK(utf16_matches_utf8(trace.text[0], allText[375]));
    CHECK(utf16_matches_utf8(trace.text[1], allText[477]));
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == menuTextures[237]);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    menu_mainMenu(gameoverMdef);
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) != 0);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 6);
    menuVars.menuModeSP = 0;
    menuVars.pad[0] = 0;

    menuVars.menuMode[0] = 0x0c;
    menu_mainLoop();
    CHECK(newMenu_state == 1);
    menuVars.menuMode[0] = 0x13;
    menu_initCredits();
    menu_mainLoop();
    CHECK(skipCreditForFrame == 0);
    menuVars.menuMode[0] = 0x1b;
    menu_mainLoop();
    CHECK(menuVars.scoreScore == 1);

    jpb_TextSetDrawHook(NULL, NULL);
    return 0;
}

static int test_demo_movie_owner(void)
{
    JPBMenuPlatformHooks hooks;
    PlatformTrace platform_trace;
    MenuAudioControlTrace audio_trace;
    MenuDrawTrace text_trace;

    reset_menu_state();
    memset(&hooks, 0, sizeof(hooks));
    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&audio_trace, 0, sizeof(audio_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    hooks.triggerMovie = trace_movie;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    jpb_AudioStreamSetControlHook(
        capture_menu_audio_control, &audio_trace);
    jpb_AudioStreamSetPlayHook(
        capture_menu_audio_play, &audio_trace);

    OptionStruct.Music = 1;
    OptionStruct.musicVolume = 37;
    GameStruct.xaNum = 0;
    GameStruct.xaFlag = 0;
    menuVars.movieSelect = 7;
    menuVars.pad[0] = JPB_PAD_LEFT;
    menuVars.pad[1] = JPB_PAD_RIGHT;
    menu_demoMovie();

    CHECK(audio_trace.calls == 2);
    CHECK(audio_trace.lastControl == JPB_AUDIO_STREAM_STOP);
    CHECK(audio_trace.playCalls == 1);
    CHECK(audio_trace.lastTrack == 1);
    CHECK(audio_trace.lastVolume == 74);
    CHECK(audio_trace.lastLoop == 1);
    CHECK(platform_trace.movieCalls == 3);
    CHECK(platform_trace.movies[0] == 9u);
    CHECK(platform_trace.movies[1] == 8u);
    CHECK(platform_trace.movies[2] == 0u);
    CHECK(introPlayed == 1u);
    CHECK(VideoVolume == 0.45f);
    CHECK(menuVars.movieSelect == 0);
    CHECK(menuVars.titleArt == 1u);
    CHECK(menuVars.pad[0] == 0u && menuVars.pad[1] == 0u);
    CHECK(menuVars.mcount == 0u);

    memset(&audio_trace, 0, sizeof(audio_trace));
    menuVars.pad[0] = JPB_PAD_LEFT;
    menu_demoMovie();
    CHECK(platform_trace.movieCalls == 3);
    CHECK(audio_trace.calls == 1);
    CHECK(audio_trace.playCalls == 1);
    CHECK(menuVars.mcount == 0u);

    reset_menu_state();
    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    generateAllText(0);
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustmentMM = 0.5f;
    lastUsedInputType = 0;
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 1;
    OptionStruct.EULAaccepted = 0;
    menu_mainLoop();
    CHECK(menuVars.menuModeSP == 1u);
    CHECK(menuVars.menuMode[1] == 0x9fu);
    CHECK(platform_trace.movieCalls == 0);

    reset_menu_state();
    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustmentMM = 0.5f;
    lastUsedInputType = 0;
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 1;
    OptionStruct.EULAaccepted = 1;
    OptionStruct.Music = 0;
    menu_mainLoop();
    CHECK(menuVars.menuModeSP == 0u);
    CHECK(menuVars.menuMode[0] == 1u);
    CHECK(platform_trace.movieCalls == 3);
    CHECK(text_trace.calls == 5);
    CHECK(utf16_matches_utf8(text_trace.text[0], allText[473]));
    CHECK(utf16_matches_utf8(text_trace.text[1], allText[286]));
    CHECK(utf16_matches_utf8(text_trace.text[2], allText[287]));
    CHECK(utf16_matches_utf8(text_trace.text[3], allText[288]));
    CHECK(utf16_matches_utf8(text_trace.text[4], allText[289]));
    CHECK(text_trace.x[0] == 480 && text_trace.y[0] == 370);
    CHECK(text_trace.x[1] == 480 && text_trace.y[1] == 450);
    CHECK(text_trace.x[2] == 480 && text_trace.y[2] == 470);
    CHECK(text_trace.x[3] == 480 && text_trace.y[3] == 490);
    CHECK(text_trace.x[4] == 480 && text_trace.y[4] == 510);
    CHECK(text_trace.scale[0] == 2.25f);
    CHECK(text_trace.scale[1] == 1.5f);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_AudioStreamSetPlayHook(NULL, NULL);
    jpb_AudioStreamSetControlHook(NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_concept_art_presentation(void)
{
    MenuDrawTrace text_trace;
    MenuTextureDrawTrace texture_trace;
    unsigned index;

    reset_menu_state();
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    OptionStruct.ScreenWidth = 1280;
    OptionStruct.ScreenHeight = 720;
    scaleAdjustment = getScaleAdjustment();
    scaleAdjustmentMM = getScaleAdjustmentMM();
    generateAllText(0);
    for (index = 121; index <= 198; ++index) {
        menuTextures[index] = (_Material *)(uintptr_t)(index + 1u);
    }
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);

    menuVars.menuModeSP = 1;
    menuVars.menuMode[0] = 0;
    menuVars.menuMode[1] = 0x1b;
    menuConceptMenu();
    CHECK(menuVars.scoreScore == 1);
    CHECK(texture_trace.calls == 3);
    CHECK(texture_trace.materials[0] == menuTextures[197]);
    CHECK(texture_trace.materials[1] == menuTextures[198]);
    CHECK(texture_trace.materials[2] == menuTextures[121]);
    CHECK(texture_trace.layers[0] == 0.1f);
    CHECK(texture_trace.layers[2] == 1.0f);
    CHECK(texture_trace.destinations[2].left == 0);
    CHECK(texture_trace.destinations[2].top == 0);
    CHECK(texture_trace.destinations[2].right == 1280);
    CHECK(texture_trace.destinations[2].bottom == 720);
    CHECK(text_trace.calls == 2);
    CHECK(jpb_utf16_compare(text_trace.text[0], L"01 / 42") == 0);
    CHECK(utf16_matches_utf8(text_trace.text[1], allText[476]));

    memset(&texture_trace, 0, sizeof(texture_trace));
    menuVars.pad[0] = JPB_PAD_RIGHT;
    menuConceptMenu();
    CHECK(menuVars.scoreScore == JPB_CONCEPT_ART_PAGE_COUNT);
    CHECK(texture_trace.materials[0] == menuTextures[193]);
    CHECK(texture_trace.materials[2] == menuTextures[162]);
    menuVars.pad[0] = JPB_PAD_LEFT;
    menuConceptMenu();
    CHECK(menuVars.scoreScore == 1);

    menuVars.pad[0] = JPB_PAD_JUMP;
    menuConceptMenu();
    CHECK(menuVars.menuModeSP == 0);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    return 0;
}

static int test_credit_presentation(void)
{
    MenuDrawTrace trace;

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    OptionStruct.Music = 0;
    scaleAdjustment = getScaleAdjustment();
    scaleAdjustmentMM = getScaleAdjustmentMM();
    generateAllText(0);
    jpb_TextSetDrawHook(capture_menu_text, &trace);

    CHECK(creditsMdef[0] == UINT32_C(0x47));
    CHECK(creditsMdef[1] == UINT32_C(0x14));
    CHECK(credMusic[0] == 6);
    CHECK(credMusic[5] == 95);
    CHECK(strcmp((const char *)theCredits[0] + 1, "LucasArts") == 0);
    CHECK(strcmp(
        (const char *)theCredits[JPB_CREDIT_LINE_COUNT - 3],
        "George Lucas") == 0);

    menu_initCredits();
    CHECK(GameStruct.gameMode == 0);
    CHECK(menuVars.bar_y == 0);
    CHECK(menuVars.bar_speed == UINT32_C(0x10000));
    CHECK(menuVars.titleArt == 1);
    CHECK(menuVars.titleDispEnable == 1);
    CHECK(skipCreditForFrame == 1);
    CHECK(credMuse == 1);
    menu_drawCredits();
    CHECK(trace.calls == 0);
    CHECK(skipCreditForFrame == 0);
    menu_drawCredits();
    CHECK(creditBarPosition > 3.9f);
    CHECK(creditBarPosition < 4.1f);
    CHECK(trace.calls == 2);
    CHECK(trace.tint[0] == 16);
    CHECK(jpb_utf16_compare(trace.text[0], L"LucasArts") == 0);
    CHECK(utf16_matches_utf8(trace.text[1], allText[476]));
    CHECK(trace.clipEnabled[0] == 1);
    CHECK(trace.clipLeft[0] == 18);
    CHECK(trace.clipTop[0] == 22);
    CHECK(trace.clipRight[0] == 1901);
    CHECK(trace.clipBottom[0] == 1057);
    CHECK(trace.clipEnabled[1] == 1);
    CHECK(jpb_TextGetClipRect(NULL, NULL, NULL, NULL) == 0);

    memset(&trace, 0, sizeof(trace));
    creditBarPosition = 100000.0f;
    menu_drawCredits();
    CHECK(creditBarPosition == 0.0f);
    CHECK(trace.calls == 1);
    CHECK(utf16_matches_utf8(trace.text[0], allText[476]));

    menuVars.menuModeSP = 0;
    menuVars.scoreScore = 17;
    CHECK(menu_handleMenuTriggers(0x1b) == 0);
    CHECK(menuVars.menuMode[1] == 0x1b);
    CHECK(menuVars.scoreScore == 17);
    menu_initNewMenu();
    CHECK(menuVars.scoreScore == 0);
    menuVars.menuModeSP = 0;
    CHECK(menu_handleMenuTriggers(0x13) == 0);
    CHECK(menuVars.menuMode[1] == 0x13);
    CHECK(skipCreditForFrame == 0);
    menu_initNewMenu();
    CHECK(skipCreditForFrame == 1);

    jpb_TextSetDrawHook(NULL, NULL);
    return 0;
}

static int test_eula_presentation_and_acceptance(void)
{
    static const unsigned eula_lengths[] = {
        JPB_EULA_ENG_LINE_COUNT,
        JPB_EULA_GER_LINE_COUNT,
        JPB_EULA_FRE_LINE_COUNT,
        JPB_EULA_ITA_LINE_COUNT,
        JPB_EULA_SPA_LINE_COUNT,
        JPB_EULA_RUS_LINE_COUNT,
        JPB_EULA_ZHO_LINE_COUNT
    };
    static const unsigned texture_indices[] = {
        5u, 195u, 196u, 199u, 200u, 243u
    };
    _Material *saved_textures[
        sizeof(texture_indices) / sizeof(texture_indices[0])];
    _Material materials[
        sizeof(texture_indices) / sizeof(texture_indices[0])];
    char *saved_title = allText[73];
    char *saved_controller_prompt = allText[489];
    char *saved_keyboard_prompt = allText[490];
    float saved_scale_mm = scaleAdjustmentMM;
    float saved_delta_time = deltaTime;
    float saved_icon_scale = iconScaleOverride;
    unsigned char line_one[] = "LINE ONE";
    unsigned char line_two[] = "LINE TWO";
    unsigned char line_three[] = "LINE THREE";
    unsigned char *lines[] = {line_one, line_two, line_three};
    MenuTextureDrawTrace texture_trace;
    MenuDrawTrace text_trace;
    MenuUITextClipTrace ui_clip_trace;
    MenuInputTrace input_trace;
    PlatformTrace platform_trace;
    JPBMenuPlatformHooks hooks;
    uint32_t initial_scroll;
    int found_normal_up = 0;
    int found_pressed_down = 0;
    int found_thumb = 0;
    int found_line_one = 0;
    int found_controller_prompt = 0;
    size_t index;

    reset_menu_state();
    memset(&materials, 0, sizeof(materials));
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&ui_clip_trace, 0, sizeof(ui_clip_trace));
    memset(&input_trace, 0, sizeof(input_trace));
    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&hooks, 0, sizeof(hooks));
    for (index = 0;
         index < sizeof(texture_indices) / sizeof(texture_indices[0]);
         ++index) {
        saved_textures[index] = menuTextures[texture_indices[index]];
        menuTextures[texture_indices[index]] = &materials[index];
    }
    allText[73] = "EULA";
    allText[489] = "CONTROLLER ACCEPT";
    allText[490] = "KEYBOARD ACCEPT";
    OptionStruct.ScreenWidth = 1920u;
    OptionStruct.ScreenHeight = 1080u;
    scaleAdjustmentMM = 1.0f;
    deltaTime = 0.016f;
    iconScaleOverride = -1.0f;
    menuVars.bar_y = 200u << 16;
    menuVars.bar_speed = 1u;
    eulaMinScroll = 100u << 16;
    eulaMaxScroll = 300u << 16;
    eulaAcceptThreshold = 250u << 16;
    eulaCanAccept = 0u;
    memset(padMaskBits, 0xff, sizeof(padMaskBits));
    input_trace.pads[0] = JPB_PAD_DOWN;
    hooks.saveSettingsData = trace_save_settings;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    jpb_InputSetProvider(read_menu_pad, &input_trace);
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    jpb_WHookSetDrawUITextUTF16Hook(
        capture_menu_ui_text_clip, &ui_clip_trace);

    initial_scroll = menuVars.bar_y;
    menu_drawEULA(lines, 3);
    CHECK(menuVars.bar_y > initial_scroll);
    CHECK(eulaCanAccept == 0u);
    for (index = 0; index < (size_t)texture_trace.calls && index < 64u;
         ++index) {
        found_normal_up |=
            texture_trace.materials[index] == menuTextures[199];
        found_pressed_down |=
            texture_trace.materials[index] == menuTextures[196];
        found_thumb |=
            texture_trace.materials[index] == menuTextures[243];
    }
    CHECK(found_normal_up != 0);
    CHECK(found_pressed_down != 0);
    CHECK(found_thumb != 0);
    for (index = 0; index < (size_t)text_trace.calls && index < 32u;
         ++index) {
        if (jpb_utf16_compare(text_trace.text[index], L"LINE ONE") == 0) {
            found_line_one = 1;
        }
    }
    CHECK(found_line_one != 0);
    CHECK(ui_clip_trace.clippedCalls > 0);
    CHECK(jpb_TextGetClipRect(NULL, NULL, NULL, NULL) == 0);
    CHECK(textClipping == 0u);
    CHECK(iconScaleOverride == -1.0f);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    input_trace.pads[0] = 0u;
    menuVars.oldpad[0] = 0u;
    menuVars.bar_y = 260u << 16;
    lastUsedInputType = 1u;
    menu_drawEULA(lines, 3);
    CHECK(eulaCanAccept == 1u);
    for (index = 0; index < (size_t)text_trace.calls && index < 32u;
         ++index) {
        found_controller_prompt |= jpb_utf16_compare(
            text_trace.text[index], L"CONTROLLER ACCEPT") == 0;
    }
    CHECK(found_controller_prompt != 0);
    CHECK(iconScaleOverride == -1.0f);

    menuVars.menuModeSP = 0u;
    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    menu_drawEULA(lines, 3);
    CHECK(OptionStruct.EULAaccepted == 1u);
    CHECK(platform_trace.settingsSaveCalls == 1);
    CHECK(platform_trace.savedSettings.EULAaccepted == 1u);
    CHECK(menuVars.menuModeSP == 1u);
    CHECK(menuVars.menuMode[1] == 1u);

    CHECK(strcmp((char *)EULA_ENG[0], "Aspyr Media, Inc.") == 0);
    CHECK(EULA_ENG[1] == EULA_GER[1]);
    for (index = 0;
         index < sizeof(eula_lengths) / sizeof(eula_lengths[0]);
         ++index) {
        uint32_t content_height =
            34u * (eula_lengths[index] + 2u);

        OptionStruct.Language = (uint8_t)index;
        menuVars.menuModeSP = 0u;
        menuVars.menuMode[0] = 0x9fu;
        menu_initNewMenu();
        CHECK(menuVars.bar_y == (1010u << 16));
        CHECK(eulaMinScroll == (1010u << 16));
        CHECK(eulaMaxScroll ==
              (content_height - 985u + 1080u) << 16);
        CHECK(eulaAcceptThreshold ==
              (content_height - 1085u + 1080u) << 16);
    }

    memset(&texture_trace, 0, sizeof(texture_trace));
    OptionStruct.Language = 0u;
    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0x9fu;
    menuVars.pad[0] = 0u;
    input_trace.pads[0] = 0u;
    menu_mainLoop();
    CHECK(texture_trace.calls >= 3);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawUITextUTF16Hook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    jpb_InputSetProvider(NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    for (index = 0;
         index < sizeof(texture_indices) / sizeof(texture_indices[0]);
         ++index) {
        menuTextures[texture_indices[index]] = saved_textures[index];
    }
    allText[73] = saved_title;
    allText[489] = saved_controller_prompt;
    allText[490] = saved_keyboard_prompt;
    scaleAdjustmentMM = saved_scale_mm;
    deltaTime = saved_delta_time;
    iconScaleOverride = saved_icon_scale;
    return 0;
}

static int test_load_screen_presentation(void)
{
    static const unsigned texture_indices[] = {
        10u, 164u, 165u, 166u, 167u, 168u
    };
    _Material *saved_textures[
        sizeof(texture_indices) / sizeof(texture_indices[0])];
    _Material materials[
        sizeof(texture_indices) / sizeof(texture_indices[0])];
    _Material *saved_white = whitemat;
    FONTSPEC saved_preview = fontSpec[410];
    char *saved_loading = allText[158];
    char *saved_level = allText[306];
    float saved_scale_mm = scaleAdjustmentMM;
    MenuTextureDrawTrace texture_trace;
    MenuDrawTrace text_trace;
    MenuLoadScreenPresentTrace present_trace;
    MenuRumbleTrace rumble_trace;
    int clear_calls = 0;
    _Material white;
    size_t index;

    reset_menu_state();
    memset(&materials, 0, sizeof(materials));
    memset(&white, 0, sizeof(white));
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&present_trace, 0, sizeof(present_trace));
    memset(&rumble_trace, 0, sizeof(rumble_trace));
    for (index = 0;
         index < sizeof(texture_indices) / sizeof(texture_indices[0]);
         ++index) {
        saved_textures[index] = menuTextures[texture_indices[index]];
        menuTextures[texture_indices[index]] = &materials[index];
    }
    whitemat = &white;
    allText[158] = "LOADING";
    allText[306] = "FEDERATION BATTLESHIP";
    fontSpec[410].clut = 10u;
    OptionStruct.ScreenWidth = 1920u;
    OptionStruct.ScreenHeight = 1080u;
    scaleAdjustmentMM = 1.0f;
    LevelSelect = 1;
    loadTotal = 800u;
    menuVars.artloadPos = -32;
    menuVars.fadeupCounter = 0u;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    jpb_WHookSetRenderLoadHook(
        capture_load_screen_present, &present_trace);
    jpb_InputSetRumbleProvider(
        trace_menu_rumble, &rumble_trace);

    menu_redrawLoadscreen();
    CHECK(texture_trace.calls == 7);
    CHECK(texture_trace.materials[0] == menuTextures[164]);
    CHECK(texture_trace.materials[1] == menuTextures[165]);
    CHECK(texture_trace.materials[2] == menuTextures[10]);
    CHECK(texture_trace.materials[3] == menuTextures[166]);
    CHECK(texture_trace.materials[4] == menuTextures[168]);
    CHECK(texture_trace.materials[5] == menuTextures[167]);
    CHECK(texture_trace.materials[6] == whitemat);
    CHECK(texture_trace.layers[0] == 0.9f);
    CHECK(texture_trace.layers[1] == 0.4f);
    CHECK(texture_trace.layers[2] == 0.5f);
    CHECK(texture_trace.layers[3] == 0.3f);
    CHECK(texture_trace.layers[4] == 0.35f);
    CHECK(texture_trace.layers[5] == 0.2f);
    CHECK(texture_trace.layers[6] == 1.0f);
    CHECK(texture_trace.sources[3].left == 100);
    CHECK(texture_trace.sources[3].top == 0);
    CHECK(texture_trace.sources[3].right == 1067);
    CHECK(texture_trace.sources[3].bottom == 135);
    CHECK(texture_trace.colors[3].r == 239u);
    CHECK(texture_trace.colors[3].g == 239u);
    CHECK(texture_trace.colors[3].b == 239u);
    CHECK(texture_trace.colors[3].cd == 239u);
    CHECK(text_trace.calls == 2);
    CHECK(jpb_utf16_compare(text_trace.text[0], L"LOADING") == 0);
    CHECK(jpb_utf16_compare(
              text_trace.text[1], L"FEDERATION BATTLESHIP") == 0);
    CHECK(menuVars.artloadPos == -16);
    CHECK(menuVars.fadeupCounter == 5u);

    loadScreenFlag = 1u;
    menu_addTotal(100u);
    CHECK(loadTotal == 900u);
    CHECK(present_trace.calls == 1);
    CHECK(rumble_trace.calls == 2);
    CHECK(rumble_trace.controllerIndices[0] == 0);
    CHECK(rumble_trace.controllerIndices[1] == 1);
    CHECK(menuVars.artloadPos == 0);
    menu_killLoadScreen();
    CHECK(loadTotal == UINT32_C(10000900));
    CHECK(present_trace.calls == 2);
    CHECK(rumble_trace.calls == 4);
    CHECK(rumble_trace.controllerIndices[2] == 0);
    CHECK(rumble_trace.controllerIndices[3] == 1);
    CHECK(loadScreenFlag == 0u);
    CHECK(GameStruct.gameMode == 3u);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    menuVars.artloadPos = -32;
    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0x66u;
    jpb_WHookSetClearWindowHook(
        capture_clear_window, &clear_calls);
    menu_mainLoop();
    CHECK(clear_calls == 1);
    CHECK(texture_trace.calls == 7);
    CHECK(text_trace.calls == 2);
    jpb_WHookSetClearWindowHook(NULL, NULL);

    jpb_InputSetRumbleProvider(NULL, NULL);
    jpb_WHookSetRenderLoadHook(NULL, NULL);
    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    for (index = 0;
         index < sizeof(texture_indices) / sizeof(texture_indices[0]);
         ++index) {
        menuTextures[texture_indices[index]] = saved_textures[index];
    }
    whitemat = saved_white;
    fontSpec[410] = saved_preview;
    allText[158] = saved_loading;
    allText[306] = saved_level;
    scaleAdjustmentMM = saved_scale_mm;
    return 0;
}

static int test_load_bar_initialization(void)
{
    JPBMenuPlatformHooks hooks;
    PlatformTrace trace;

    reset_menu_state();
    memset(&hooks, 0, sizeof(hooks));
    memset(&trace, 0, sizeof(trace));
    hooks.triggerMovie = trace_movie;
    jpb_MenuSetPlatformHooks(&hooks, &trace);

    CHECK(strcmp(snames[0], "obi") == 0);
    CHECK(strcmp(snames[1], "qui") == 0);
    CHECK(strcmp(snames[2], "mace") == 0);
    CHECK(strcmp(snames[3], "adi") == 0);
    CHECK(strcmp(snames[4], "plo") == 0);

    LevelSelect = 0;
    loadTotal = 1234u;
    menuVars.titleDispEnable = 17u;
    menu_initLoadBar();
    CHECK(loadTotal == 0u);
    CHECK(menuVars.titleDispEnable == 0u);
    CHECK(loadScreenFlag == 0u);
    CHECK(trace.movieCalls == 0);

    LevelSelect = 1;
    menuVars.ingameMovies = 0u;
    menu_initLoadBar();
    CHECK(trace.movieCalls == 1);
    CHECK(trace.lastMovie == 1u);
    CHECK(trace.lastMovieFlags == 0);
    CHECK(menuVars.titleDispEnable == 1u);
    CHECK(loadScreenFlag == 1u);

    LevelSelect = 8;
    GameStruct.CurrentLevel = 3u;
    menu_initLoadBar();
    CHECK(trace.movieCalls == 2);
    CHECK(trace.lastMovie == 5u);
    CHECK(menuVars.titleDispEnable == 1u);
    CHECK(loadScreenFlag == 1u);

    menuVars.ingameMovies = 1u;
    GameStruct.CurrentLevel = 4u;
    menu_initLoadBar();
    CHECK(trace.movieCalls == 2);
    CHECK(menuVars.titleDispEnable == 1u);
    CHECK(loadScreenFlag == 1u);

    LevelSelect = 2;
    menuVars.titleDispEnable = 0u;
    menu_initLoadBar();
    CHECK(trace.movieCalls == 2);
    CHECK(menuVars.titleDispEnable == 1u);
    CHECK(loadScreenFlag == 1u);

    loadScreenFlag = 0u;
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_council_presentation(void)
{
    playerObject player;
    VECTOR center = {0, 32, 100, 0};
    FONTSPEC saved_specs[2] = {fontSpec[224], fontSpec[225]};
    _Material *saved_marker_textures[2] = {
        menuTextures[10], menuTextures[11]
    };
    _Material *saved_background = menuTextures[120];
    _Material marker_materials[2];
    _Material background;
    MenuTextureDrawTrace texture_trace;
    MenuPolyDrawTrace poly_trace;
    playerObject saved_players[JPB_PLAYER_CAPACITY];
    float saved_scale_mm = scaleAdjustmentMM;
    float saved_scale_x = gPSXDrawScaleX;
    float saved_scale_y = gPSXDrawScaleY;
    float saved_scale_w = gPSXDrawScaleW;
    float saved_scale_h = gPSXDrawScaleH;
    int saved_front_rgb = frontRGBoff;
    char saved_level = LevelSelect;
    unsigned index;

    reset_menu_state();
    memset(&player, 0, sizeof(player));
    memset(marker_materials, 0, sizeof(marker_materials));
    memset(&background, 0, sizeof(background));
    background.texture = &background;
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&poly_trace, 0, sizeof(poly_trace));
    memcpy(saved_players, gaPlayerData, sizeof(saved_players));
    for (index = 0; index < JPB_PLAYER_CAPACITY; ++index) {
        gaPlayerData[index].playerRoot.objectID = -1;
    }

    OptionStruct.ScreenWidth = 1920u;
    OptionStruct.ScreenHeight = 1080u;
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    gPSXDrawScaleW = 1.0f;
    gPSXDrawScaleH = 1.0f;
    frontRGBoff = 0;
    memset(&CameraMatrix, 0, sizeof(CameraMatrix));
    CameraMatrix.m[0][0] = 1.0f;
    CameraMatrix.m[1][1] = 1.0f;
    CameraMatrix.m[2][2] = 1.0f;
    fontSpec[224].x = 0;
    fontSpec[224].y = 0;
    fontSpec[224].w = 7;
    fontSpec[224].h = 9;
    fontSpec[224].clut = 10;
    fontSpec[225].x = 0;
    fontSpec[225].y = 0;
    fontSpec[225].w = 7;
    fontSpec[225].h = 9;
    fontSpec[225].clut = 11;
    menuTextures[10] = &marker_materials[0];
    menuTextures[11] = &marker_materials[1];
    menuTextures[120] = &background;
    player.playerID = 80;
    GameStruct.NumPlayers = 1;
    menuVars.pselectMode[0].mode = 0;
    menuVars.fcount = 0;
    menuVars.jediDebugCombo = 0;
    menuVars.menuModeSP = 0;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);

    menuVars.menuMode[0] = 6;
    menu_councilPos(&player, &center);
    CHECK(texture_trace.calls == 0);
    menuVars.menuMode[0] = 7;
    menu_councilPos(&player, &center);
    CHECK(texture_trace.calls == 2);
    CHECK(texture_trace.materials[0] == &marker_materials[1]);
    CHECK(texture_trace.materials[1] == &marker_materials[0]);
    CHECK(texture_trace.destinations[0].left == 894);
    CHECK(texture_trace.destinations[0].top == 551);
    CHECK(texture_trace.destinations[0].right == 899);
    CHECK(texture_trace.destinations[0].bottom == 562);
    CHECK(texture_trace.destinations[1].left == 896);
    CHECK(texture_trace.destinations[1].top == 540);
    CHECK(texture_trace.destinations[1].right == 931);
    CHECK(texture_trace.destinations[1].bottom == 549);
    CHECK(texture_trace.colors[0].r == 44u);
    CHECK(texture_trace.colors[0].g == 44u);
    CHECK(texture_trace.colors[0].b == 44u);
    CHECK(texture_trace.colors[0].cd == 127u);

    jpb_WHookSetScreenPolyHook(capture_menu_poly_draw, &poly_trace);
    LevelSelect = 1;
    menu_showCouncil();
    CHECK(poly_trace.calls == 0);
    LevelSelect = 0;
    menu_showCouncil();
    CHECK(poly_trace.calls == 1);
    CHECK(poly_trace.materials[0] == &background);
    CHECK(poly_trace.vertexCounts[0] == 4);
    CHECK(poly_trace.vertices[0][0].x == 0.0f);
    CHECK(poly_trace.vertices[0][0].y == 0.0f);
    CHECK(poly_trace.vertices[0][1].x == 857.1428f);
    CHECK(poly_trace.vertices[0][1].y == 0.0f);
    CHECK(poly_trace.vertices[0][2].x == 0.0f);
    CHECK(poly_trace.vertices[0][2].y > 492.0f);
    CHECK(poly_trace.vertices[0][2].y < 493.0f);
    CHECK(poly_trace.vertices[0][0].z == 1.0f);
    CHECK(poly_trace.vertices[0][0].tu == 0.0f);
    CHECK(poly_trace.vertices[0][1].tu == 1.0f);
    CHECK(poly_trace.vertices[0][2].tv == 1.0f);
    CHECK(poly_trace.vertices[0][0].argb == UINT32_MAX);

    memset(&poly_trace, 0, sizeof(poly_trace));
    OptionStruct.ScreenWidth = 1024u;
    OptionStruct.ScreenHeight = 768u;
    menu_showCouncil();
    CHECK(poly_trace.calls == 1);
    CHECK(poly_trace.vertices[0][0].x == 0.0f);
    CHECK(poly_trace.vertices[0][0].y > 61.0f);
    CHECK(poly_trace.vertices[0][0].y < 62.0f);
    CHECK(poly_trace.vertices[0][1].x == 857.1428f);
    CHECK(poly_trace.vertices[0][2].y > 430.0f);
    CHECK(poly_trace.vertices[0][2].y < 432.0f);

    jpb_WHookSetScreenPolyHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    memcpy(gaPlayerData, saved_players, sizeof(saved_players));
    fontSpec[224] = saved_specs[0];
    fontSpec[225] = saved_specs[1];
    menuTextures[10] = saved_marker_textures[0];
    menuTextures[11] = saved_marker_textures[1];
    menuTextures[120] = saved_background;
    scaleAdjustmentMM = saved_scale_mm;
    gPSXDrawScaleX = saved_scale_x;
    gPSXDrawScaleY = saved_scale_y;
    gPSXDrawScaleW = saved_scale_w;
    gPSXDrawScaleH = saved_scale_h;
    frontRGBoff = saved_front_rgb;
    LevelSelect = saved_level;
    return 0;
}

static int test_memory_menu_presentation(void)
{
    MemoryPool saved_banks[MEMORY_POOL_COUNT];
    MenuDrawTrace trace;
    int first_memory_line;

    reset_menu_state();
    memcpy(saved_banks, maMemoryBanks, sizeof(saved_banks));
    memset(&trace, 0, sizeof(trace));
    maMemoryBanks[0].memSize = 200u << 10;
    maMemoryBanks[0].memFree = 100u << 10;
    maMemoryBanks[0].memUsed = 100u << 10;
    maMemoryBanks[1].memSize = 300u << 10;
    maMemoryBanks[1].memFree = 50u << 10;
    maMemoryBanks[1].memUsed = 250u << 10;
    maMemoryBanks[2].memSize = 400u << 10;
    maMemoryBanks[2].memFree = 200u << 10;
    maMemoryBanks[2].memUsed = 200u << 10;
    jpb_TextSetDrawHook(capture_menu_text, &trace);

    menu_dumpMemory(160u, 20u);
    CHECK(trace.calls == 5);
    CHECK(trace.tint[0] == 11 && trace.mode[0] == 0);
    CHECK(trace.x[0] == 160 && trace.y[0] == 20);
    CHECK(trace.y[1] == 32);
    CHECK(trace.y[2] == 48);
    CHECK(trace.y[3] == 64);
    CHECK(trace.y[4] == 80);
    CHECK(utf16_matches_utf8(trace.text[0], "MEM USE\n"));
    CHECK(utf16_matches_utf8(
        trace.text[1], "Bank 00: F:100K T:200K"));
    CHECK(utf16_matches_utf8(
        trace.text[2], "Bank 01: F:050K T:300K"));
    CHECK(utf16_matches_utf8(
        trace.text[3], "Bank 02: F:200K T:400K"));
    CHECK(utf16_matches_utf8(
        trace.text[4], "All:used 550K, free 350K\n"));

    memset(&trace, 0, sizeof(trace));
    generateAllText(0);
    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0x31u;
    menu_mainLoop();
    CHECK(trace.calls >= 5);
    first_memory_line = trace.calls - 5;
    CHECK(utf16_matches_utf8(
        trace.text[first_memory_line], "MEM USE\n"));
    CHECK(utf16_matches_utf8(
        trace.text[first_memory_line + 4],
        "All:used 550K, free 350K\n"));

    jpb_TextSetDrawHook(NULL, NULL);
    memcpy(maMemoryBanks, saved_banks, sizeof(saved_banks));
    return 0;
}

static int test_recovered_main_loop_state_routes(void)
{
    reset_menu_state();
    menuVars.menuMode[0] = 0x3fu;
    GameStruct.gameMode = 2;
    GameStruct.inMenuFlag = 1;
    GameStruct.GameState = UINT32_MAX;
    menu_mainLoop();
    CHECK(GameStruct.gameMode == 6);
    CHECK(GameStruct.inMenuFlag == 0);
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) == 0u);

    reset_menu_state();
    menuVars.menuMode[0] = 0x40u;
    GameStruct.gameMode = 2;
    GameStruct.letterboxFlag = 0;
    GameStruct.letterboxFlag2 = 1;
    menu_mainLoop();
    CHECK(GameStruct.letterboxFlag == 1);
    CHECK(GameStruct.gameMode == 6);
    CHECK(GameStruct.inMenuFlag == 0);

    reset_menu_state();
    menuVars.menuMode[0] = 0x41u;
    GameStruct.gameMode = 2;
    GameStruct.GameState = UINT32_MAX;
    menu_mainLoop();
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) == 0u);
    CHECK(GameStruct.gameMode == 6);
    CHECK(GameStruct.inMenuFlag == 0);

    reset_menu_state();
    menuVars.menuMode[0] = 0x32u;
    GameStruct.gameMode = 6;
    menu_mainLoop();
    CHECK(menuVars.menuMode[0] == 0u);
    return 0;
}

static int test_extended_main_loop_state_routes(void)
{
    JPBMenuPlatformHooks hooks;
    PlatformTrace platform_trace;
    MenuDrawTrace text_trace;
    WorldData world;
    playerObject player_one;
    playerObject player_two;
    playerObject saved_debug_player = gaPlayerData[0];
    WorldData *saved_world = gpWorld;
    int saved_max_checkpoints = maxCheckPoints;

    reset_menu_state();
    memset(&hooks, 0, sizeof(hooks));
    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    hooks.saveGameData = trace_save;
    hooks.requestExit = trace_request_exit;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    generateAllText(0);
    scaleAdjustmentMM = 1.0f;
    GameStruct.xaNum = UINT16_C(0x1234);
    GameStruct.xaFlag = UINT16_C(0x5678);
    GameStruct.xastartPos = UINT32_C(0x11111111);
    GameStruct.xacurPos = UINT32_C(0x22222222);
    GameStruct.xaendPos = UINT32_C(0x33333333);

    menu_writexainfo(20u, 160u, 0u);
    CHECK(text_trace.calls == 5);
    CHECK(text_trace.x[0] == 20 && text_trace.y[0] == 20);
    CHECK(text_trace.x[1] == 20 && text_trace.y[1] == 80);
    CHECK(text_trace.x[2] == 20 && text_trace.y[2] == 160);
    CHECK(text_trace.x[3] == 20 && text_trace.y[3] == 220);
    CHECK(text_trace.x[4] == 20 && text_trace.y[4] == 280);
    CHECK(jpb_utf16_compare(
        text_trace.text[0], L"xaNum:00001234") == 0);
    CHECK(jpb_utf16_compare(
        text_trace.text[4], L"cd end pos:33333333") == 0);

    memset(&text_trace, 0, sizeof(text_trace));
    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0x12u;
    menu_mainLoop();
    CHECK(text_trace.calls >= 5);
    CHECK(jpb_utf16_compare(
        text_trace.text[0], L"xaNum:00001234") == 0);
    CHECK(menuVars.mmSelectPtr >= audioMusicMdef);
    CHECK(menuVars.mmSelectPtr < audioMusicMdef + 46);

    memset(&gaPlayerData[0], 0, sizeof(gaPlayerData[0]));
    mp1ComboCount = 0u;
    menuVars.jediDebugCombo = 0u;
    menuVars.menuMode[0] = 0x29u;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= comboDebugMenuMdef);
    CHECK(menuVars.mmSelectPtr < comboDebugMenuMdef + 44);
    CHECK(comboDebugMenuMdef[40] == 0x14u);

    OptionStruct.ScreenWidth = 960u;
    OptionStruct.ScreenHeight = 540u;
    menuVars.menuMode[0] = 0x2fu;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= debugMdef);
    CHECK(menuVars.mmSelectPtr < debugMdef + 112);
    CHECK(debugMdef[110] == 0x14u);

    menuVars.menuMode[0] = 0x0fu;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= loadMenuMdef);
    CHECK(menuVars.mmSelectPtr < loadMenuMdef + 18);
    CHECK(loadMenuMdef[14] == 0x14u);

    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0x94u;
    menu_mainLoop();
    CHECK(menuVars.menuModeSP == 2u);
    CHECK(menuVars.menuMode[1] == 0u);
    CHECK(menuVars.menuMode[2] == 0u);

    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0x93u;
    menu_mainLoop();
    CHECK(platform_trace.exitCalls == 1);

    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0x9eu;
    GameStruct.gameMode = 6;
    GameStruct.gameCompleted = 0;
    menu_mainLoop();
    CHECK(GameStruct.gameCompleted == 1);
    CHECK(platform_trace.saveCalls == 1);
    CHECK(menuVars.mmSelectPtr >= ngpUnlockMdef);
    CHECK(menuVars.mmSelectPtr < ngpUnlockMdef + 26);
    CHECK(ngpUnlockMdef[25] == 0x14u);

    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0x14u;
    GameStruct.gameMode = 6;
    p1Disconnected = 0;
    p2Disconnected = 0;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= gamepauseMenuMdef);
    CHECK(menuVars.mmSelectPtr < gamepauseMenuMdef + 48);
    CHECK(gamepauseMenuMdef[45] == 0x14u);
    CHECK(cheatCheckPoint[0] == 4u);
    CHECK(cheatCheckPoint[9] == 4u);
    CHECK(memcmp(
        cheatCheckPointKeyboard, "POPPOOPOOP", 10u) == 0);
    CHECK(cheatRadar[0] == 0x1000u);
    CHECK(cheatRadar[5] == 4u);

    menuVars.menuModeSP = 1u;
    menuVars.menuMode[0] = 0x40u;
    menuVars.menuMode[1] = 0x14u;
    p1Disconnected = 1;
    menu_mainLoop();
    CHECK(menuVars.menuModeSP == 0u);
    p1Disconnected = 0;

    OptionStruct.DebugLevel = 0u;
    GameStruct.GameState = UINT32_MAX;
    GameStruct.gameMode = 2;
    GameStruct.inMenuFlag = 1;
    menu_RadarCheat();
    CHECK(OptionStruct.DebugLevel == 3u);
    CHECK(GameStruct.gameMode == 6);
    CHECK(GameStruct.inMenuFlag == 0);
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) == 0u);
    menu_RadarCheat();
    CHECK(OptionStruct.DebugLevel == 0u);

    memset(&world, 0, sizeof(world));
    memset(&player_one, 0, sizeof(player_one));
    memset(&player_two, 0, sizeof(player_two));
    world.player0 = &player_one;
    world.player1 = &player_two;
    gpWorld = &world;
    maxCheckPoints = 1;
    GameStruct.CurrentLevel = 0u;
    GameStruct.checkpoint[0] = 0u;
    GameStruct.GameState = UINT32_MAX;
    GameStruct.gameMode = 2;
    GameStruct.inMenuFlag = 1;
    menu_JumpCheckPoint();
    CHECK(GameStruct.gameMode == 6);
    CHECK(GameStruct.inMenuFlag == 0);
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) == 0u);

    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0xa0u;
    p1Disconnected = 1;
    menu_mainLoop();
    CHECK(menuVars.menuMode[0] == 0xa0u);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    gpWorld = saved_world;
    maxCheckPoints = saved_max_checkpoints;
    gaPlayerData[0] = saved_debug_player;
    return 0;
}

static int test_title_load_presentation(void)
{
    char *saved_title = allText[184];
    char *saved_prompt = allText[302];
    MenuDrawTrace trace;
    int clear_calls = 0;

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    allText[184] = "LOAD GAME";
    allText[302] = "INSERT CONTROLLER PAK";
    jpb_TextSetDrawHook(capture_menu_text, &trace);

    menu_runTitleLoad();
    CHECK(trace.calls >= 2);
    CHECK(trace.y[0] == 32);
    CHECK(utf16_matches_utf8(trace.text[0], "LOAD GAME"));
    CHECK(trace.x[trace.calls - 1] == 20);
    CHECK(trace.y[trace.calls - 1] == 180);
    CHECK(utf16_matches_utf8(
        trace.text[trace.calls - 1], "INSERT CONTROLLER PAK"));
    CHECK(insert1Mdef[0] == 0u);
    CHECK(insert1Mdef[9] == 0x12eu);
    CHECK(insert1Mdef[12] == 0x14u);

    memset(&trace, 0, sizeof(trace));
    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0x26u;
    jpb_WHookSetClearWindowHook(
        capture_clear_window, &clear_calls);
    menu_mainLoop();
    CHECK(clear_calls == 1);
    CHECK(trace.calls >= 2);
    CHECK(utf16_matches_utf8(trace.text[0], "LOAD GAME"));
    CHECK(utf16_matches_utf8(
        trace.text[trace.calls - 1], "INSERT CONTROLLER PAK"));
    jpb_WHookSetClearWindowHook(NULL, NULL);

    jpb_TextSetDrawHook(NULL, NULL);
    allText[184] = saved_title;
    allText[302] = saved_prompt;
    return 0;
}

static int test_recovered_definition_routes(void)
{
    static const uint16_t modes[] = {
        0x15u, 0x17u, 0x25u, 0x28u, 0x2au,
        0x2bu, 0x2cu, 0x2eu, 0x30u
    };
    MenuDrawTrace trace;
    size_t index;

    reset_menu_state();
    generateAllText(0);
    jpb_TextSetDrawHook(capture_menu_text, &trace);
    CHECK(memcarddebugMdef[39] == 0u);
    CHECK(movieMenuMdef[22] == 0x14u);
    CHECK(rusureMenuMdef[24] == 0x14u);
    CHECK(aidebugMenuMdef[83] == 0x14u);
    CHECK(editMenuMdef[24] == 0x14u);
    CHECK(cameraMenuMdef[43] == 0x14u);
    CHECK(objectiveMenuMdef[41] == 0x14u);
    CHECK(specialMessMenuMdef[23] == 0x14u);
    CHECK(specialMessMenu2Mdef[23] == 0x14u);
    CHECK(gamecombosMenu[8] == 0x14u);

    for (index = 0; index < sizeof(modes) / sizeof(modes[0]); ++index) {
        memset(&trace, 0, sizeof(trace));
        menuVars.menuModeSP = 0u;
        menuVars.menuMode[0] = modes[index];
        menu_mainLoop();
        CHECK(trace.calls > 0);
        CHECK(menuVars.menuMode[0] == modes[index]);
        if (modes[index] >= 0x2au && modes[index] <= 0x2cu) {
            CHECK((GameStruct.GameState &
                   UINT32_C(0x02000000)) != 0u);
        }
    }

    jpb_TextSetDrawHook(NULL, NULL);
    return 0;
}

static int test_exact_state_leaves(void)
{
    uint32_t player_select_item[5] = {9, 0, 0, 0x9d, 20};

    reset_menu_state();
    CHECK(menu_controlCK() == 1);
    CHECK(menu_NOLOAD() == 0);
    CHECK(menu_checkSoftReset(123) == 0);
    secretBits = (1u << 8) | (1u << 7);
    CHECK(menu_ultimate() == 1);
    CHECK(menu_concept() == 1);

    menuVars.aibit = 1;
    menuVars.awards[1] = 3;
    CHECK(menu_healthCK(0) == 1);
    CHECK(menu_forceCK(0) == 1);
    GameStruct.NumPlayers = 1;
    menuVars.pplayers[0] = 0;
    CHECK(menu_changeSaberCheck(0) == 0);
    menuVars.pplayers[0] = 2;
    CHECK(menu_changeSaberCheck(0) == 1);

    GameStruct.NumPlayers = 1;
    menuVars.pad[1] = JPB_PAD_RIGHT;
    OptionStruct.Music = 1;
    CHECK(menu_playerSelectCheck(
              (int64_t)(uintptr_t)player_select_item) == 0);
    CHECK(OptionStruct.Music == 1);
    GameStruct.NumPlayers = 2;
    CHECK(menu_playerSelectCheck(
              (int64_t)(uintptr_t)player_select_item) == 1);
    CHECK(OptionStruct.Music == 0);
    menuVars.pad[1] = JPB_PAD_LEFT;
    CHECK(menu_playerSelectCheck(
              (int64_t)(uintptr_t)player_select_item) == 1);
    CHECK(OptionStruct.Music == 1);
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 1;
    menuVars.pad[1] = JPB_PAD_COMBO_SOUTH;
    CHECK(menu_playerSelectCheck(
              (int64_t)(uintptr_t)player_select_item) == 1);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 0x9c);
    menuVars.pad[1] = JPB_PAD_JUMP;
    CHECK(menu_playerSelectCheck(
              (int64_t)(uintptr_t)player_select_item) == 1);
    CHECK(menuVars.menuModeSP == 0);

    memset(&gColor, 0, sizeof(gColor));
    SetGlobalColorDefault();
    CHECK(gColor.r == 255 && gColor.g == 255);
    CHECK(gColor.b == 255 && gColor.cd == 255);
    gLeft = -12.75f;
    gRight = 640.9f;
    gTop = 4.99f;
    gBottom = 479.1f;
    SetGlobalDST();
    CHECK(gDST.left == -12);
    CHECK(gDST.right == 640);
    CHECK(gDST.top == 4);
    CHECK(gDST.bottom == 479);

    menuVars.mcount = 9;
    screenSaverFlag = 1;
    screenSaverCount = 10;
    saverAlpha = 20;
    menu_ClearScreenSaver();
    CHECK(screenSaverFlag == 0);
    CHECK(screenSaverCount == 0);
    CHECK(saverAlpha == 0);
    CHECK(menuVars.mcount == 0);

    menuVars.titleArt = 0;
    menuVars.titleDispEnable = 0;
    menu_checkTitleReload();
    CHECK(menuVars.titleArt == 1);
    CHECK(menuVars.titleDispEnable == 1);
    menu_mainExitMenu();
    CHECK(menuVars.titleDispEnable == 0);

    menuVars.loadSaveMode = 7;
    menu_resetSaveMenu();
    CHECK(menuVars.loadSaveMode == 0);
    menuVars.loadSaveMode = 7;
    menu_saveMCARDError();
    CHECK(menuVars.loadSaveMode == 0);
    menu_saveMCARDSelect();
    CHECK(menuVars.loadSaveMode == 8);
    menuVars.cardProtectFlag = 0;
    menu_scanProtection();
    CHECK(menuVars.cardProtectFlag == 1);
    menu_resetMemcardFlags();
    menu_saveGame();
    menu_setDrawSurface(123);
    return 0;
}

static int test_exact_noop_leaves(void)
{
    MENUVARS saved_menu;
    gamestruct saved_game;
    optionstruct saved_options;
    unsigned short load_list[2] = {17u, 29u};
    MDEF_MOD modifier;

    reset_menu_state();
    memset(&modifier, 0xa5, sizeof(modifier));
    memcpy(&saved_menu, &menuVars, sizeof(saved_menu));
    memcpy(&saved_game, &GameStruct, sizeof(saved_game));
    memcpy(&saved_options, &OptionStruct, sizeof(saved_options));

    initSaveMenu();
    menuLoadSelectTextures(load_list, 2u, 7u);
    menu_drawColorPoly(1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u);
    menu_drawColorPolyG4(
        1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u);
    menu_finishloadGame();
    menu_initReconnect();
    menu_initTitleLoad();
    menu_initialLoad(3u);
    menu_loadFrontEndArt(4u);
    menu_loadGame();
    menu_mkEmptySaveGame();
    menu_overLifeBars(99u);
    menu_showGameMode();
    menu_showSaves();
    mmDrawCard(1u, 2u, 3u, 4u);
    mmDrawMisc(&modifier, 13u);
    mmvInitScore(1u);
    newMenu_DrawMessageBox(-1, 2, 3, 4);
    runInitialMemcard();
    runSaveMenu();
    turnOffBackground();

    CHECK(memcmp(&menuVars, &saved_menu, sizeof(menuVars)) == 0);
    CHECK(memcmp(&GameStruct, &saved_game, sizeof(GameStruct)) == 0);
    CHECK(memcmp(&OptionStruct, &saved_options, sizeof(OptionStruct)) == 0);
    CHECK(load_list[0] == 17u && load_list[1] == 29u);
    return 0;
}

static int test_small_recovered_leaves(void)
{
    const float menu_aspect = 1.7777778f;
    PlatformTrace platform_trace;
    JPBMenuPlatformHooks hooks;
    MenuTextureDrawTrace draw_trace;
    MenuPolyDrawTrace poly_trace;
    MenuTextureTrace texture_trace;
    _Material background = {0};
    _Material *saved_background = menuTextures[5];
    uint32_t ordering_table = UINT32_C(0x12345678);
    uint32_t *saved_ordering_table = maCurrentOT;
    unsigned award_level = UINT32_MAX;
    unsigned char combo_a[] = "ABC";
    unsigned char combo_prefix[] = "ABCD";
    unsigned char combo_short[] = "AB";
    unsigned char combo_mismatch[] = "ABX";
    float x;
    float y;
    float expected_x;
    float expected_y;
    float current_aspect;

    reset_menu_state();
    GameStruct.continueAble = -7;
    CHECK(menu_gameContinue() == -7);
    GameStruct.continueAble = 1;
    CHECK(menu_gameContinue() == 1);

    GameStruct.NumPlayers = 1;
    GameStruct.ModelSelect[0] = obi_wan_model;
    jediUpgrades[obi_wan_model].awardData[4] = 2;
    CHECK(menu_CheckValidLevel(4u, &award_level) == 1);
    CHECK(award_level == 2u);

    CHECK(comboSubset(combo_a, combo_prefix) == 1u);
    CHECK(comboSubset(combo_a, combo_a) == 1u);
    CHECK(comboSubset(combo_a, combo_short) == 0u);
    CHECK(comboSubset(combo_a, combo_mismatch) == 0u);

    gPSXDrawScaleX = 2.0f;
    gPSXDrawScaleY = 3.0f;
    OptionStruct.ScreenWidth = 2560u;
    OptionStruct.ScreenHeight = 1080u;
    x = 123.0f;
    y = 456.0f;
    current_aspect = 2560.0f / 1080.0f;
    expected_x = 123.0f / current_aspect;
    expected_x *= menu_aspect;
    expected_x +=
        ((current_aspect - menu_aspect) / current_aspect) *
        0.5f * (2560.0f / 2.0f);
    fixPSPos(&x, &y);
    CHECK(x == expected_x);
    CHECK(y == 456.0f);

    OptionStruct.ScreenWidth = 1024u;
    OptionStruct.ScreenHeight = 768u;
    x = 123.0f;
    y = 456.0f;
    current_aspect = 1024.0f / 768.0f;
    expected_y = 456.0f / menu_aspect;
    expected_y *= current_aspect;
    expected_y +=
        ((menu_aspect - current_aspect) / menu_aspect) *
        0.5f * (768.0f / 3.0f);
    fixPSPos(&x, &y);
    CHECK(x == 123.0f);
    CHECK(y == expected_y);

    OptionStruct.ScreenWidth = 1920u;
    OptionStruct.ScreenHeight = 1080u;
    x = 321.0f;
    y = 654.0f;
    current_aspect = 1920.0f / 1080.0f;
    expected_y = 654.0f / menu_aspect;
    expected_y *= current_aspect;
    fixPSPos(&x, &y);
    CHECK(x == 321.0f);
    CHECK(y == expected_y);

    menuVars.pointSeek = 0;
    menuVars.bar_speed = UINT32_MAX;
    menu_calcTbarspeed(17u);
    CHECK(menuVars.bar_speed == 0u);
    menuVars.pointSeek = 249u;
    menu_calcTbarspeed(17u);
    CHECK(menuVars.bar_speed == 0u);
    menuVars.pointSeek = 250u;
    menu_calcTbarspeed(17u);
    CHECK(menuVars.bar_speed == (17u << 16));
    menuVars.pointSeek = 500u;
    menu_calcTbarspeed(17u);
    CHECK(menuVars.bar_speed == (17u << 15));
    menuVars.pointSeek = 1000u;
    menu_calcTbarspeed(0xffffu);
    CHECK(menuVars.bar_speed == ((0xffffu << 16) / 4u));

    scaleAdjustmentMM = 1.0f;
    OptionStruct.ScreenHeight = 1080u;
    menuVars.bar_y = 0;
    menuVars.bar_speed = 0;
    eulaMinScroll = 0;
    eulaMaxScroll = 0;
    eulaAcceptThreshold = 0;
    eulaCanAccept = 99u;
    menu_initEULA(20);
    CHECK(menuVars.bar_speed == UINT32_C(0x10000));
    CHECK(menuVars.bar_y == UINT32_C(0x03f20000));
    CHECK(eulaMinScroll == UINT32_C(0x03f20000));
    CHECK(eulaMaxScroll == UINT32_C(0x034b0000));
    CHECK(eulaAcceptThreshold == UINT32_C(0x02e70000));
    CHECK(eulaCanAccept == 0u);

    menuVars.pad[0] = 0;
    menuVars.pad[1] = 0;
    saverPads[0] = 0;
    saverPads[1] = 0;
    screenSaverCount = 0;
    screenSaverFlag = 0;
    saverAlpha = 0;
    menuVars.mcount = 9;
    menu_screenSaver();
    CHECK(screenSaverCount == 1u);
    CHECK(screenSaverFlag == 0u);
    CHECK(saverAlpha == 0u);
    CHECK(menuVars.mcount == 9);
    screenSaverCount = 17999u;
    menu_screenSaver();
    CHECK(screenSaverCount == 18000u);
    CHECK(screenSaverFlag == 0u);
    menu_screenSaver();
    CHECK(screenSaverCount == 18000u);
    CHECK(screenSaverFlag == 1u);
    CHECK(saverAlpha == 2u);
    saverAlpha = 198u;
    menu_screenSaver();
    CHECK(saverAlpha == 200u);
    menu_screenSaver();
    CHECK(saverAlpha == 200u);
    screenSaverCount = 5u;
    screenSaverFlag = 1u;
    saverAlpha = 10u;
    menu_screenSaver();
    CHECK(screenSaverCount == 6u);
    CHECK(saverAlpha == 12u);
    menuVars.pad[1] = JPB_PAD_RIGHT;
    menuVars.mcount = 77;
    menu_screenSaver();
    CHECK(saverPads[1] == JPB_PAD_RIGHT);
    CHECK(screenSaverCount == 0u);
    CHECK(screenSaverFlag == 0u);
    CHECK(saverAlpha == 0u);
    CHECK(menuVars.mcount == 0);

    menuVars.scoreCurrentPlayer = 1;
    menuVars.awardLevel[0] = 7;
    menuVars.awardLevel[1] = 2;
    menuVars.scoreScore = 1234;
    menu_decAwardLevel();
    CHECK(menuVars.awardLevel[0] == 7);
    CHECK(menuVars.awardLevel[1] == 1);
    CHECK(menuVars.scoreScore == 1234);
    menu_decAwardLevel();
    CHECK(menuVars.awardLevel[1] == 0);
    CHECK(menuVars.scoreScore == 0);
    menuVars.scoreScore = 4321;
    menu_decAwardLevel();
    CHECK(menuVars.awardLevel[1] == 0);
    CHECK(menuVars.scoreScore == 0);

    maCurrentOT = NULL;
    menu_nukePrimo();
    maCurrentOT = &ordering_table;
    menu_nukePrimo();
    CHECK(ordering_table == UINT32_C(0x12345678));
    maCurrentOT = saved_ordering_table;

    menuVars.memBGptr = (uint8_t *)(uintptr_t)UINT32_C(0x1234);
    menuVars.titleDispEnable = 0xffu;
    menu_copyCouncil();
    CHECK(menuVars.memBGptr ==
          (uint8_t *)(uintptr_t)UINT32_C(0x1234));
    CHECK(menuVars.titleDispEnable == 1u);

    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&hooks, 0, sizeof(hooks));
    hooks.saveGameData = trace_save;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    menu_saveGameTriggered();
    CHECK(platform_trace.saveCalls == 1);
    jpb_MenuSetPlatformHooks(NULL, NULL);

    memset(&draw_trace, 0, sizeof(draw_trace));
    menuTextures[5] = &background;
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &draw_trace);
    GameStruct.gameMode = 6;
    menu_showVRAMBackground(5u);
    GameStruct.gameMode = 7;
    menu_showVRAMBackground(5u);
    CHECK(draw_trace.calls == 0);
    GameStruct.gameMode = 0;
    menu_showVRAMBackground(5u);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    CHECK(draw_trace.calls == 1);
    CHECK(draw_trace.materials[0] == &background);
    CHECK(draw_trace.destinations[0].left == 0);
    CHECK(draw_trace.destinations[0].top == 60);
    CHECK(draw_trace.destinations[0].right == 640);
    CHECK(draw_trace.destinations[0].bottom == 420);
    CHECK(draw_trace.colors[0].r == 255);
    CHECK(draw_trace.colors[0].g == 255);
    CHECK(draw_trace.colors[0].b == 255);
    CHECK(draw_trace.colors[0].cd == 255);
    CHECK(draw_trace.layers[0] == 1.0f);
    menuTextures[5] = saved_background;

    memset(&poly_trace, 0, sizeof(poly_trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    jpb_TextureSetPlatformHooks(
        load_menu_texture, unload_menu_texture, &texture_trace);
    jpb_WHookSetScreenPolyHook(capture_menu_poly_draw, &poly_trace);
    menu_grayBars(10u, 20u, 30u, 40u, UINT32_MAX);
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    CHECK(poly_trace.calls == 2);
    CHECK(poly_trace.vertexCounts[0] == 4);
    CHECK(poly_trace.vertices[0][0].x == 10.0f);
    CHECK(poly_trace.vertices[0][0].y == 20.0f);
    CHECK(poly_trace.vertices[0][0].argb == UINT32_C(0xff303030));
    CHECK(poly_trace.vertices[0][1].x == 41.0f);
    CHECK(poly_trace.vertices[0][2].y == 61.0f);
    CHECK(poly_trace.vertexCounts[1] == 4);
    CHECK(poly_trace.vertices[1][0].x == 8.0f);
    CHECK(poly_trace.vertices[1][0].y == 19.0f);
    CHECK(poly_trace.vertices[1][0].argb == UINT32_C(0xff606060));
    CHECK(poly_trace.vertices[1][1].x == 43.0f);
    CHECK(poly_trace.vertices[1][2].y == 62.0f);

    memset(&poly_trace, 0, sizeof(poly_trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    jpb_TextureSetPlatformHooks(
        load_menu_texture, unload_menu_texture, &texture_trace);
    jpb_WHookSetScreenPolyHook(capture_menu_poly_draw, &poly_trace);
    menu_slideco_a(50u, 100u, 10u, 100u, 20u, 30u, 2u, 3u);
    jpb_WHookSetScreenPolyHook(NULL, NULL);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    CHECK(poly_trace.calls == 3);
    CHECK(poly_trace.vertices[0][0].x == 19.0f);
    CHECK(poly_trace.vertices[0][0].y == 29.0f);
    CHECK(poly_trace.vertices[0][0].argb == UINT32_C(0xfff0f0f0));
    CHECK(poly_trace.vertices[0][1].x == 122.0f);
    CHECK(poly_trace.vertices[0][2].y == 42.0f);
    CHECK(poly_trace.vertices[1][0].x == 20.0f);
    CHECK(poly_trace.vertices[1][0].y == 30.0f);
    CHECK(poly_trace.vertices[1][0].argb == UINT32_C(0xff80a080));
    CHECK(poly_trace.vertices[1][1].x == 71.0f);
    CHECK(poly_trace.vertices[2][0].x == 70.0f);
    CHECK(poly_trace.vertices[2][0].argb == UINT32_C(0xff008000));
    CHECK(poly_trace.vertices[2][1].x == 121.0f);

    loadvrmFlag = 7u;
    menuTexLoaded2 = 9u;
    menuBucketFront();
    CHECK(loadvrmFlag == 0u);
    CHECK(menuTexLoaded2 == 0u);
    loadvrmFlag = 5u;
    menuTexLoaded2 = 3u;
    menuBucketSavegame();
    CHECK(loadvrmFlag == 0u);
    CHECK(menuTexLoaded2 == 0u);
    return 0;
}

static int test_mover_bytecode(void)
{
    uint8_t absolute[] = {0x24, 0x34, 0x12, 0x78, 0x56};
    uint8_t tween[] = {
        0x25, 0x20, 0x00, 0x10, 0x00, 0x02, 0x2d
    };
    uint8_t chained[] = {0x30, 0x32, 0x34, 0x00, 0x26, 0x03};
    uint8_t trigger_set_wait[] = {0x35, 0x00, 0x36, 0x00};
    uint8_t trigger_clear[] = {0x37, 0x00};
    uint8_t conditional[] = {
        0x3a, 0x05, 0x26, 0x09, 0x3b, 0x05, 0x26, 0x07
    };
    uint8_t versus_condition[] = {0x3a, 0x3c, 0x26, 0x09};
    uint8_t exit_trigger[] = {0x38, 0x3a};
    uint8_t sound_wait[] = {0x39, 0x05, 0x26, 0x01};
    uint8_t inert[] = {0x2d};
    uint32_t simple_menu[] = {0, 3, 0x14};
    MenuSoundTrace sound_trace;
    MMVDEF *control;

    reset_menu_state();
    CHECK(moverMenus[0] == frameTopMdef);
    CHECK(moverMenus[1] == frameBotMdef);
    CHECK(moverMenus[2] == frameBotMdefls);
    CHECK(moverMenus[3] == frameLeftMdef);
    CHECK(moverMenus[4] == frameRightMdef);
    CHECK(moverMenus[5] == saveNowMdef);
    CHECK(frameTopMdef[4] == 0x1eu);
    CHECK(frameBotMdefls[6] == 0xc8u);
    CHECK(saveNowMdef[25] == 0x14u);

    menuVars.mmvCount = 1;
    control = &menuVars.mmv[0];
    control->mmvSrc = absolute;
    control->mmvMenuFlags = 2;
    menu_handleMovers();
    CHECK(menuVars.mmvCurrentMenuControl == control);
    CHECK(control->mmvIns == 0x24u);
    CHECK(control->mmvPtr == 5u);
    CHECK(control->mmvCounter == 0u);
    CHECK(control->mmvX == INT32_C(0x12340000));
    CHECK(control->mmvY == INT32_C(0x56780000));

    reset_menu_state();
    menuVars.mmvCount = 1;
    control = &menuVars.mmv[0];
    control->mmvSrc = tween;
    control->mmvMenuFlags = 2;
    control->state = 1;
    menu_handleMovers();
    CHECK(control->mmvIns == 0x2au);
    CHECK(control->mmvPtr == 6u);
    CHECK(control->mmvCounter == 1u);
    CHECK(control->mmvXvect == INT32_C(0x00100000));
    CHECK(control->mmvYvect == INT32_C(0x00080000));
    CHECK(control->mmvX == INT32_C(0x00100000));
    CHECK(control->mmvY == INT32_C(0x00080000));
    menu_handleMovers();
    CHECK(control->mmvCounter == 0u);
    CHECK(control->mmvX == INT32_C(0x00200000));
    CHECK(control->mmvY == INT32_C(0x00100000));
    menu_handleMovers();
    CHECK(control->mmvIns == 0x2du);
    CHECK(control->mmvPtr == 6u);
    CHECK(control->state == 0u);

    reset_menu_state();
    menuVars.mmvCount = 1;
    control = &menuVars.mmv[0];
    control->mmvSrc = chained;
    menu_handleMovers();
    CHECK(control->mmvMenuFlags == 3u);
    CHECK(control->mmvMenu == frameTopMdef);
    CHECK(control->mmvIns == 0x26u);
    CHECK(control->mmvPtr == 6u);
    CHECK(control->mmvCounter == 2u);
    CHECK(control->mmvY == -INT32_C(0x10000));

    reset_menu_state();
    menuVars.mmvCount = 1;
    control = &menuVars.mmv[0];
    control->mmvSrc = inert;
    control->mmvMenuFlags = 2;
    control->mmvCounter = 1;
    control->mmvIns = 0x27;
    menu_handleMovers();
    CHECK(control->mmvY == INT32_C(0x10000));
    control->mmvCounter = 1;
    control->mmvIns = 0x28;
    menu_handleMovers();
    CHECK(control->mmvX == -INT32_C(0x10000));
    control->mmvCounter = 1;
    control->mmvIns = 0x29;
    menu_handleMovers();
    CHECK(control->mmvX == 0);
    control->mmvCounter = 1;
    control->mmvIns = 0x2a;
    control->mmvXvect = 3;
    control->mmvYvect = -5;
    menu_handleMovers();
    CHECK(control->mmvX == 3);
    CHECK(control->mmvY == INT32_C(0x10000) - 5);

    reset_menu_state();
    menuVars.mmvCount = 1;
    control = &menuVars.mmv[0];
    control->mmvSrc = trigger_set_wait;
    control->mmvMenuFlags = 2;
    menu_handleMovers();
    CHECK(((uint8_t *)(void *)&menuVars)[0x231] == 1u);
    CHECK(control->mmvPtr == 2u);
    menu_handleMovers();
    CHECK(control->mmvIns == 0x36u);
    CHECK(control->mmvPtr == 4u);
    CHECK(control->mmvCounter == 1u);
    control->mmvSrc = trigger_clear;
    control->mmvPtr = 0;
    control->mmvCounter = 0;
    menu_handleMovers();
    CHECK(((uint8_t *)(void *)&menuVars)[0x231] == 0u);

    reset_menu_state();
    menuVars.mmvCount = 1;
    control = &menuVars.mmv[0];
    control->mmvSrc = conditional;
    control->mmvMenuFlags = 2;
    GameStruct.NumPlayers = 1;
    menu_handleMovers();
    CHECK(control->mmvIns == 0x3bu);
    CHECK(control->mmvPtr == 6u);
    menu_handleMovers();
    CHECK(control->mmvIns == 0x26u);
    CHECK(control->mmvPtr == 8u);
    CHECK(control->mmvCounter == 6u);

    reset_menu_state();
    menuVars.mmvCount = 1;
    control = &menuVars.mmv[0];
    control->mmvSrc = versus_condition;
    control->mmvMenuFlags = 2;
    GameStruct.NumPlayers = 2;
    menu_handleMovers();
    CHECK(control->mmvIns == 0x3au);
    CHECK(control->mmvPtr == 2u);

    reset_menu_state();
    menuVars.mmvCount = 1;
    control = &menuVars.mmv[0];
    control->mmvSrc = inert;
    control->mmvCounter = 1;
    control->mmvIns = 0x2b;
    control->mmvMenu = simple_menu;
    menu_handleMovers();
    CHECK(menuVars.mmTotal == 3u);

    reset_menu_state();
    menuVars.menuModeSP = 1;
    menuVars.menuMode[0] = 1;
    menuVars.menuMode[1] = 2;
    menuVars.mmvCount = 1;
    control = &menuVars.mmv[0];
    control->mmvSrc = exit_trigger;
    control->mmvMenuFlags = 2;
    menu_handleMovers();
    CHECK(menuVars.menuModeSP == 0u);

    reset_menu_state();
    memset(&sound_trace, 0, sizeof(sound_trace));
    sound_FreeBank(0);
    CHECK(sound_LoadBank("resident", 0) == 0);
    jpb_SoundSetPlaySfxHook(trace_menu_sound, &sound_trace);
    menuVars.mmvCount = 1;
    control = &menuVars.mmv[0];
    control->mmvSrc = sound_wait;
    control->mmvMenuFlags = 2;
    menu_handleMovers();
    CHECK(sound_trace.calls == 1);
    CHECK(strcmp(sound_trace.sound, "xlvbrows") == 0);
    CHECK(sound_trace.bank == 0);
    CHECK(sound_trace.flag == 8u);
    CHECK(control->mmvIns == 0x26u);
    CHECK(control->mmvPtr == 4u);
    jpb_SoundSetPlaySfxHook(NULL, NULL);
    sound_FreeBank(0);
    return 0;
}

static int test_unformatted_card_prompt(void)
{
    MenuDrawTrace text_trace;
    MenuSoundTrace sound_trace;

    reset_menu_state();
    generateAllText(0);
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&sound_trace, 0, sizeof(sound_trace));
    OptionStruct.ScreenWidth = 1000;
    menuVars.cardSelect = 2;
    menuVars.dialogBox1 = 0;
    menuVars.mmFlags = UINT32_MAX;
    menuVars.controlFlags = 0;
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    sound_FreeBank(0);
    CHECK(sound_LoadBank("resident", 0) == 0);
    jpb_SoundSetPlaySfxHook(trace_menu_sound, &sound_trace);

    CHECK(menu_handleUnformatted() == 0);
    CHECK(text_trace.calls == 2);
    CHECK(text_trace.tint[0] == 11);
    CHECK(text_trace.y[0] == 0x52);
    CHECK(utf16_matches_utf8(
        text_trace.text[0],
        "MEMORY CARD 3 is unformatted\nFormat MEMORY CARD 3?"));
    CHECK(text_trace.y[1] == 0xca);
    CHECK(utf16_matches_utf8(text_trace.text[1], "NO"));
    CHECK(menuVars.mmFlags == 0u);
    CHECK(menuVars.controlFlags == 3u);
    CHECK(menuVars.dialogBox1 == 0u);
    CHECK(sound_trace.calls == 0);

    memset(&text_trace, 0, sizeof(text_trace));
    menuVars.pad[0] = JPB_PAD_RIGHT;
    CHECK(menu_handleUnformatted() == 0);
    CHECK(text_trace.calls == 2);
    CHECK(utf16_matches_utf8(text_trace.text[1], "NO"));
    CHECK(menuVars.dialogBox1 == 1u);
    CHECK(sound_trace.calls == 1);
    CHECK(strcmp(sound_trace.sound, "xjedscrl") == 0);

    memset(&text_trace, 0, sizeof(text_trace));
    menuVars.pad[0] = 0;
    menuVars.pad[1] = JPB_PAD_COMBO_SOUTH;
    CHECK(menu_handleUnformatted() == 1);
    CHECK(utf16_matches_utf8(text_trace.text[1], "YES"));
    CHECK(menuVars.dialogBox1 == 1u);
    CHECK(sound_trace.calls == 2);

    memset(&text_trace, 0, sizeof(text_trace));
    menuVars.pad[1] = JPB_PAD_JUMP;
    CHECK(menu_handleUnformatted() == 1);
    CHECK(utf16_matches_utf8(text_trace.text[1], "YES"));
    CHECK(menuVars.dialogBox1 == 0u);
    CHECK(sound_trace.calls == 3);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    sound_FreeBank(0);
    jpb_TextSetDrawHook(NULL, NULL);
    return 0;
}

static int test_legacy_controller_prompt(void)
{
    const FONTSPEC saved_base_spec = fontSpec[0xa8];
    const FONTSPEC saved_type_spec = fontSpec[0x9a];
    _Material *saved_base_material = menuTextures[10];
    _Material *saved_type_material = menuTextures[11];
    char *saved_pad_one = allText[0xed];
    char *saved_pad_two = allText[0xee];
    const float saved_scale_mm = scaleAdjustmentMM;
    const float saved_scale_x = gPSXDrawScaleX;
    const float saved_scale_y = gPSXDrawScaleY;
    const float saved_scale_w = gPSXDrawScaleW;
    const float saved_scale_h = gPSXDrawScaleH;
    const float saved_front_z = frontZ;
    const uint8_t saved_rgb_offset = frontRGBoff;
    MenuDrawTrace text_trace;
    MenuTextureDrawTrace texture_trace;
    _Material base_material = {0};
    _Material type_material = {0};
    int text_width;
    int text_height;

    reset_menu_state();
    memset(&text_trace, 0, sizeof(text_trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    fontSpec[0xa8] = (FONTSPEC){0, 10, 3, 2, 8, 12};
    fontSpec[0x9a] = (FONTSPEC){0, 11, 7, 5, 6, 9};
    menuTextures[10] = &base_material;
    menuTextures[11] = &type_material;
    allText[0xed] = "PAD ONE";
    allText[0xee] = "PAD TWO";
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    gPSXDrawScaleW = 1.0f;
    gPSXDrawScaleH = 1.0f;
    frontZ = 0.0f;
    frontRGBoff = 0;
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);

    menu_drawController(100u, 80u, 0);
    CHECK(text_trace.calls == 0);
    CHECK(texture_trace.calls == 0);

    padExist = 1u;
    menu_drawController(100u, 80u, 0);
    GetStringSize(0.75f, &text_width, &text_height, "PAD ONE");
    CHECK(text_trace.calls == 1);
    CHECK(text_trace.tint[0] == 11);
    CHECK(text_trace.x[0] == 164 - text_width / 2);
    CHECK(text_trace.y[0] == 68);
    CHECK(utf16_matches_utf8(text_trace.text[0], "PAD ONE"));
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.materials[0] == &base_material);
    CHECK(texture_trace.destinations[0].left == 100);
    CHECK(texture_trace.destinations[0].top == 80);
    CHECK(texture_trace.destinations[0].right == 112);
    CHECK(texture_trace.destinations[0].bottom == 88);
    CHECK(texture_trace.sources[0].left == 8);
    CHECK(texture_trace.sources[0].top == 12);
    CHECK(texture_trace.sources[0].right == 56);
    CHECK(texture_trace.sources[0].bottom == 44);
    CHECK(texture_trace.colors[0].cd == 0xffu);

    memset(&text_trace, 0, sizeof(text_trace));
    memset(&texture_trace, 0, sizeof(texture_trace));
    padExist = 2u;
    padTypes = 2u;
    menu_drawController(200u, 120u, 1);
    GetStringSize(0.75f, &text_width, &text_height, "PAD TWO");
    CHECK(text_trace.calls == 1);
    CHECK(text_trace.x[0] == 264 - text_width / 2);
    CHECK(text_trace.y[0] == 108);
    CHECK(utf16_matches_utf8(text_trace.text[0], "PAD TWO"));
    CHECK(texture_trace.calls == 2);
    CHECK(texture_trace.materials[0] == &type_material);
    CHECK(texture_trace.destinations[0].left == 229);
    CHECK(texture_trace.destinations[0].top == 146);
    CHECK(texture_trace.destinations[0].right == 238);
    CHECK(texture_trace.destinations[0].bottom == 152);
    CHECK(texture_trace.materials[1] == &base_material);
    CHECK(texture_trace.destinations[1].left == 200);
    CHECK(texture_trace.destinations[1].top == 120);
    CHECK(texture_trace.destinations[1].right == 212);
    CHECK(texture_trace.destinations[1].bottom == 128);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    jpb_TextSetDrawHook(NULL, NULL);
    fontSpec[0xa8] = saved_base_spec;
    fontSpec[0x9a] = saved_type_spec;
    menuTextures[10] = saved_base_material;
    menuTextures[11] = saved_type_material;
    allText[0xed] = saved_pad_one;
    allText[0xee] = saved_pad_two;
    scaleAdjustmentMM = saved_scale_mm;
    gPSXDrawScaleX = saved_scale_x;
    gPSXDrawScaleY = saved_scale_y;
    gPSXDrawScaleW = saved_scale_w;
    gPSXDrawScaleH = saved_scale_h;
    frontZ = saved_front_z;
    frontRGBoff = saved_rgb_offset;
    return 0;
}

static int test_abort_pause_and_overlay(void)
{
    JPBMenuPlatformHooks hooks;
    PlatformTrace platform_trace;
    MenuRumbleTrace rumble_trace;
    MenuAudioControlTrace xa_trace;
    MenuSoundControlTrace sound_trace;

    reset_menu_state();
    LevelSelect = 1;
    GameStruct.AbortCount = 29;
    padCurrentBits[0].padLevel1 = UINT32_C(0x900);
    menu_checkAbortOrPause();
    CHECK(GameStruct.AbortCount == 30);
    CHECK(GameStruct.gameMode == 0);
    menu_checkAbortOrPause();
    CHECK(GameStruct.AbortCount == 31);
    CHECK((GameStruct.GameState & UINT32_C(2)) != 0);
    CHECK(GameStruct.gameMode == 9);
    CHECK(GameStruct.inMenuFlag == 0);

    reset_menu_state();
    LevelSelect = 1;
    GameStruct.AbortCount = 7;
    menu_checkAbortOrPause();
    CHECK(GameStruct.AbortCount == 0);

    reset_menu_state();
    memset(&rumble_trace, 0, sizeof(rumble_trace));
    memset(&xa_trace, 0, sizeof(xa_trace));
    memset(&sound_trace, 0, sizeof(sound_trace));
    LevelSelect = 2;
    menuVars.pad[0] = JPB_PAD_START;
    menuVars.menuModeSP = 0;
    memset(menuVars.mmSelect1, 0xff, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 0xff, sizeof(menuVars.mmSelect2));
    GameStruct.letterboxFlag = 3;
    OptionStruct.ShockFlag[0] = 1;
    jpb_InputSetRumbleProvider(trace_menu_rumble, &rumble_trace);
    jpb_AudioStreamSetControlHook(
        capture_menu_audio_control, &xa_trace);
    jpb_SoundSetControlHook(
        capture_menu_sound_control, &sound_trace);
    menu_checkAbortOrPause();
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) != 0);
    CHECK(GameStruct.inMenuFlag == 1);
    CHECK(GameStruct.letterboxFlag2 == 3);
    CHECK(menuVars.menuModeSP == 2u);
    CHECK(menuVars.menuMode[1] == 0x40u);
    CHECK(menuVars.menuMode[2] == 0x14u);
    CHECK(menuVars.pad[0] == 0u && menuVars.pad[1] == 0u);
    CHECK(menuVars.mmSelect1[1] == 0u);
    CHECK(menuVars.mmSelect1[2] == 0u);
    CHECK(menuVars.mmSelect2[1] == 0u);
    CHECK(menuVars.mmSelect2[2] == 0u);
    CHECK(rumble_trace.calls == 2);
    CHECK(xa_trace.calls == 1);
    CHECK(xa_trace.lastControl == JPB_AUDIO_STREAM_PAUSE);
    CHECK(sound_trace.calls == 1);
    CHECK(sound_trace.lastControl == JPB_SOUND_CONTROL_PAUSE_MUSIC);
    jpb_SoundSetControlHook(NULL, NULL);
    jpb_AudioStreamSetControlHook(NULL, NULL);
    jpb_InputSetRumbleProvider(NULL, NULL);

    reset_menu_state();
    LevelSelect = 2;
    p1Disconnected = 1;
    menu_checkAbortOrPause();
    CHECK(GameStruct.inMenuFlag == 1);
    CHECK(menuVars.menuModeSP == 3u);
    CHECK(menuVars.menuMode[1] == 0x40u);
    CHECK(menuVars.menuMode[2] == 0x14u);
    CHECK(menuVars.menuMode[3] == 0xa0u);

    reset_menu_state();
    LevelSelect = 2;
    p2Disconnected = 1;
    GameStruct.GameState = UINT32_C(0x4000);
    menu_checkAbortOrPause();
    CHECK(GameStruct.inMenuFlag == 0);
    CHECK(menuVars.menuModeSP == 0u);
    GameStruct.GameState = 0;
    LevelSelect = 0;
    menu_checkAbortOrPause();
    CHECK(GameStruct.inMenuFlag == 0);
    CHECK(menuVars.menuModeSP == 0u);

    reset_menu_state();
    memset(&hooks, 0, sizeof(hooks));
    memset(&platform_trace, 0, sizeof(platform_trace));
    hooks.saveSettingsData = trace_save_settings;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    menuVars.pad[0] = JPB_PAD_ZOOM_IN;
    OptionStruct.overlayMode = 2;
    menu_checkAbortOrPause();
    CHECK(OptionStruct.overlayMode == 0);
    CHECK(GameStruct.screenShotFlag == 0);
    CHECK(refreshHUDCounter == 12);
    CHECK(platform_trace.settingsSaveCalls == 1);
    CHECK(platform_trace.savedSettings.overlayMode == 0);
    OptionStruct.overlayMode = 0xffu;
    menu_checkAbortOrPause();
    CHECK(OptionStruct.overlayMode == 0);
    CHECK(GameStruct.screenShotFlag == 2);
    CHECK(platform_trace.settingsSaveCalls == 2);
    CHECK(platform_trace.savedSettings.overlayMode == 0);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_controller_disconnect_box(void)
{
    FONTSPEC saved_specs[9];
    _Material *saved_materials[9];
    _Material materials[9];
    MenuTextureDrawTrace trace;
    const float saved_scale_mm = scaleAdjustmentMM;
    const float saved_scale_x = gPSXDrawScaleX;
    const float saved_scale_y = gPSXDrawScaleY;
    const float saved_scale_w = gPSXDrawScaleW;
    const float saved_scale_h = gPSXDrawScaleH;
    const uint8_t saved_rgb_offset = frontRGBoff;
    unsigned index;

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    memset(materials, 0, sizeof(materials));
    for (index = 0; index < 9; ++index) {
        saved_specs[index] = fontSpec[0x9d + index];
        saved_materials[index] = menuTextures[20 + index];
        fontSpec[0x9d + index] =
            (FONTSPEC){0, (uint16_t)(20 + index), 0, 0, 8, 8};
        menuTextures[20 + index] = &materials[index];
    }
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    gPSXDrawScaleW = 1.0f;
    gPSXDrawScaleH = 1.0f;
    frontRGBoff = 0;
    jpb_WHookSetDrawTextureHook(capture_menu_texture_draw, &trace);

    GameStruct.NumPlayers = 1;
    CHECK(menu_controlDisconnect() == 0);
    GameStruct.NumPlayers = 2;
    padExist = 2;
    CHECK(menu_controlDisconnect() == 0);
    padExist = 0;
    GameStruct.versusModeFlag = 1;
    CHECK(menu_controlDisconnect() == 0);
    CHECK(trace.calls == 0);

    GameStruct.versusModeFlag = 0;
    CHECK(menu_controlDisconnect() == 1);
    CHECK(trace.calls == 9);
    CHECK(trace.materials[0] == &materials[0]);
    CHECK(trace.materials[8] == &materials[8]);
    CHECK(trace.destinations[0].left == 460);
    CHECK(trace.destinations[0].top == 468);
    CHECK(trace.destinations[0].right == 468);
    CHECK(trace.destinations[0].bottom == 476);
    CHECK(trace.destinations[1].left == 468);
    CHECK(trace.destinations[1].right == 812);
    CHECK(trace.destinations[4].left == 468);
    CHECK(trace.destinations[4].top == 476);
    CHECK(trace.destinations[4].right == 812);
    CHECK(trace.destinations[4].bottom == 510);
    CHECK(trace.destinations[8].left == 812);
    CHECK(trace.destinations[8].top == 510);
    CHECK(trace.destinations[8].right == 820);
    CHECK(trace.destinations[8].bottom == 518);
    CHECK(trace.colors[0].r == 0);
    CHECK(trace.colors[0].g == 0);
    CHECK(trace.colors[0].b == 0x4b);
    CHECK(trace.colors[0].cd == 200);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    for (index = 0; index < 9; ++index) {
        fontSpec[0x9d + index] = saved_specs[index];
        menuTextures[20 + index] = saved_materials[index];
    }
    scaleAdjustmentMM = saved_scale_mm;
    gPSXDrawScaleX = saved_scale_x;
    gPSXDrawScaleY = saved_scale_y;
    gPSXDrawScaleW = saved_scale_w;
    gPSXDrawScaleH = saved_scale_h;
    frontRGBoff = saved_rgb_offset;
    return 0;
}

static int test_combo_prerequisite_chain(void)
{
    Combo *saved_combos = gaPlayerData[0].paCombos;
    int16_t saved_max_combos = gaPlayerData[0].maxCombos;
    int16_t saved_model = GameStruct.ModelSelect[0];
    Combo combos[4];

    reset_menu_state();
    memset(combos, 0, sizeof(combos));
    gaPlayerData[0].paCombos = combos;
    gaPlayerData[0].maxCombos = 4;
    GameStruct.ModelSelect[0] = 3;
    combos[3].prev = 2;
    combos[2].prev = 1;
    combos[1].prev = -1;

    CHECK(menu_checkCombo(0u, 3) == 0u);
    game_enableCombo(3u, 2u);
    CHECK(menu_checkCombo(0u, 3) == 0u);
    game_enableCombo(3u, 1u);
    CHECK(menu_checkCombo(0u, 3) == 1u);
    game_disableCombo(3u, 2u);
    CHECK(menu_checkCombo(0u, 2) == 1u);
    CHECK(menu_checkCombo(0u, 1) == 1u);

    gaPlayerData[0].paCombos = saved_combos;
    gaPlayerData[0].maxCombos = saved_max_combos;
    GameStruct.ModelSelect[0] = saved_model;
    return 0;
}

static int test_combo_string_builders(void)
{
    unsigned char output[128];
    unsigned char destination[64] = "TAIL";
    unsigned char prefix[] = "HEAD";
    unsigned index;

    reset_menu_state();
    menuPreString(destination, prefix);
    CHECK(strcmp((char *)destination, "HEADTAIL") == 0);

    menuVars.menuModeSP = 3;
    menuVars.mmSelect1[3] = 5;
    OptionStruct.ControllerConfig[0] = 0;
    menu_buildComboString(
        output, (unsigned char *)"fnsw", 5u);
    CHECK(strcmp((char *)output, "  <F><Y><A><X>") == 0);
    menu_buildComboString(
        output, (unsigned char *)"fnsw", 4u);
    CHECK(strcmp((char *)output, "  <f><y><a><x>") == 0);

    OptionStruct.ControllerConfig[0] = 1;
    menu_buildComboString(
        output, (unsigned char *)"fnsw", 5u);
    CHECK(strcmp((char *)output, "  <F><B><Y><X>") == 0);
    comboIconOverride = 1;
    menu_buildComboString(
        output, (unsigned char *)"fnsw", 5u);
    CHECK(strcmp((char *)output, "  <f><b><y><x>") == 0);

    menu_buildComboString(
        output, (unsigned char *)"", 0u);
    CHECK(strcmp((char *)output, "  ") == 0);
    menu_buildComboString(
        output, (unsigned char *)".", 0u);
    CHECK(strcmp((char *)output, "     ") == 0);
    menu_buildComboString(
        output, (unsigned char *)"..", 0u);
    CHECK(strcmp((char *)output, "    -    ") == 0);

    for (index = 0; index < 16; ++index) {
        menuVars.frKeyBuff[index] = (uint16_t)index;
        menuVars.frKeyBuff2[index] = (uint16_t)(100u + index);
    }
    menuVars.pad[0] = UINT32_C(0x1234);
    menuVars.pad[1] = 0;
    menuPushKey();
    CHECK(menuVars.frKeyBuff[0] == 1u);
    CHECK(menuVars.frKeyBuff[14] == 15u);
    CHECK(menuVars.frKeyBuff[15] == UINT16_C(0x1034));
    CHECK(menuVars.frKeyBuff2[0] == 100u);
    CHECK(menuVars.frKeyBuff2[15] == 115u);
    return 0;
}

static int test_menu_control_owner(void)
{
    MenuInputTrace input;
    JPBMenuPlatformHooks hooks;
    unsigned char cheat_sequence[3];
    unsigned short pad_cheat[16];
    unsigned index;

    reset_menu_state();
    memset(&input, 0, sizeof(input));
    memset(&hooks, 0, sizeof(hooks));
    hooks.keyboardState = read_menu_keyboard;
    jpb_MenuSetPlatformHooks(&hooks, &input);
    jpb_InputSetProvider(read_menu_pad, &input);
    ClearInput();

    menu_readControl();
    CHECK(menuVars.pad[0] == 0);
    CHECK(menuVars.pad[1] == 0);
    CHECK(screenSaverCount == 1);

    input.pads[0] = JPB_PAD_UP | JPB_PAD_ZOOM_OUT;
    input.pads[1] = JPB_PAD_RIGHT;
    input.keyboard[4] = 1;
    input.keyboard[7] = 1;
    menu_readControl();
    CHECK(menuVars.pad[0] == (JPB_PAD_UP | JPB_PAD_ZOOM_OUT));
    CHECK(menuVars.pad[1] == JPB_PAD_RIGHT);
    CHECK(menuVars.frKeyBuff[15] == JPB_PAD_UP);
    CHECK(menuVars.frKeyBuff2[15] == JPB_PAD_RIGHT);
    CHECK(keyboardBufferIndex == 2);
    CHECK(keyboardBuffer[0] == 4);
    CHECK(keyboardBuffer[1] == 7);
    CHECK(keyboardKeyPressed == 1);

    menu_readControl();
    CHECK(menuVars.pad[0] == 0);
    CHECK(menuVars.pad[1] == 0);
    CHECK(keyboardBufferIndex == 2);
    memset(input.keyboard, 0, sizeof(input.keyboard));
    input.pads[0] = 0;
    input.pads[1] = 0;
    menu_readControl();
    CHECK(keyboardKeyPressed == 0);
    input.keyboard[9] = 1;
    menu_readControl();
    CHECK(keyboardBufferIndex == 3);
    CHECK(keyboardBuffer[2] == 9);

    cheat_sequence[0] = 4;
    cheat_sequence[1] = 7;
    cheat_sequence[2] = 9;
    menu_cheat_action_calls = 0;
    CHECK(cheatCheckKeyboard(
              cheat_sequence, 3, record_menu_cheat_action) == 1);
    CHECK(menu_cheat_action_calls == 1);
    CHECK(memcmp(
              keyboardBuffer,
              "\0\0\0\0\0\0\0\0\0\0",
              sizeof(keyboardBuffer)) == 0);
    keyboardBufferIndex = 1;
    keyboardBuffer[8] = 10;
    keyboardBuffer[9] = 11;
    keyboardBuffer[0] = 12;
    cheat_sequence[0] = 10;
    cheat_sequence[1] = 11;
    cheat_sequence[2] = 12;
    CHECK(cheatCheckKeyboard(
              cheat_sequence, 3, record_menu_cheat_action) == 1);
    CHECK(menu_cheat_action_calls == 2);
    keyboardBuffer[0] = 99;
    cheat_sequence[2] = 12;
    CHECK(cheatCheckKeyboard(
              cheat_sequence, 3, record_menu_cheat_action) == 0);
    CHECK(keyboardBuffer[0] == 99);
    CHECK(menu_cheat_action_calls == 2);

    memset(input.keyboard, 0, sizeof(input.keyboard));
    screenSaverCount = 18000;
    screenSaverFlag = 0;
    saverAlpha = 123;
    saverPads[0] = 0;
    saverPads[1] = 0;
    menu_readControl();
    CHECK(screenSaverFlag == 1);
    CHECK(screenSaverCount == 18000);
    CHECK(saverAlpha == 2);
    menu_readControl();
    CHECK(saverAlpha == 4);

    menuVars.mcount = 33;
    input.pads[0] = JPB_PAD_LEFT;
    menu_readControl();
    CHECK(screenSaverFlag == 0);
    CHECK(screenSaverCount == 0);
    CHECK(saverAlpha == 0);
    CHECK(menuVars.mcount == 0);

    menuVars.pad[0] = JPB_PAD_RIGHT | JPB_PAD_BLOCK;
    menuVars.pad[1] = JPB_PAD_UP | JPB_PAD_ZOOM_IN;
    menu_rotControls();
    CHECK(menuVars.pad[0] == (JPB_PAD_UP | JPB_PAD_BLOCK));
    CHECK(menuVars.pad[1] == (JPB_PAD_RIGHT | JPB_PAD_ZOOM_IN));

    for (index = 0; index < 16; ++index) {
        pad_cheat[index] = (unsigned short)(index + 1u);
        menuVars.frKeyBuff[index] = pad_cheat[index];
        menuVars.frKeyBuff2[index] = 0;
    }
    CHECK(cheatCheck(
              pad_cheat, 16, record_menu_cheat_action) == 1);
    CHECK(menu_cheat_action_calls == 3);
    CHECK(menuVars.frKeyBuff[0] == 1);
    CHECK(menuVars.frKeyBuff[15] == 16);
    CHECK(memcmp(
              menuVars.frKeyBuff2,
              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
              sizeof(menuVars.frKeyBuff2)) == 0);
    memset(menuVars.frKeyBuff, 0, sizeof(menuVars.frKeyBuff));
    CHECK(cheatCheck(
              pad_cheat, 16, record_menu_cheat_action) == 0);
    CHECK(menu_cheat_action_calls == 3);

    jpb_InputSetProvider(NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_trigger_dispatcher(void)
{
    JPBMenuPlatformHooks hooks;
    PlatformTrace trace;
    MenuSoundTrace sound_trace;
    RESOLUTION saved_resolution;

    reset_menu_state();
    memset(&hooks, 0, sizeof(hooks));
    memset(&trace, 0, sizeof(trace));
    memset(&sound_trace, 0, sizeof(sound_trace));
    hooks.triggerMovie = trace_movie;
    hooks.cleanupLevelData = trace_cleanup;
    hooks.saveGameData = trace_save;
    hooks.setInMenu = trace_in_menu;
    hooks.scanLevel = trace_scan_level;
    hooks.soundCue = trace_sound_cue;
    hooks.refreshLevelTransforms = trace_refresh_transforms;
    hooks.controllerCount = trace_controller_count;
    hooks.singleControllerFallback = trace_single_controller_fallback;
    hooks.requestExit = trace_request_exit;
    trace.controllerCount = 1;
    jpb_MenuSetPlatformHooks(&hooks, &trace);
    sound_FreeBank(0);
    CHECK(sound_LoadBank("resident", 0) == 0);
    jpb_SoundSetPlaySfxHook(trace_menu_sound, &sound_trace);

    menuVars.sndtest = 5;
    CHECK(menu_handleMenuTriggers(0x38) == 0);
    CHECK(sound_trace.calls == 1);
    CHECK(sound_trace.bank == 0);
    CHECK(sound_trace.position == NULL);
    CHECK(sound_trace.flag == 8u);
    CHECK(strcmp(sound_trace.sound, "xlvbrows") == 0);
    menuVars.sndtest = 11;
    CHECK(menu_handleMenuTriggers(0x38) == 0);
    CHECK(sound_trace.calls == 1);
    menuVars.movieSelect = 7;
    CHECK(menu_handleMenuTriggers(0x39) == 0);
    CHECK(trace.movieCalls == 1);
    CHECK(trace.lastMovie == 7 && trace.lastMovieFlags == 0);
    menuVars.mmvTriggerRemap = 0x37;
    menuVars.mmvTriggers[2] = 1;
    CHECK(menu_handleMenuTriggers(0x83) == 0);
    CHECK(menuVars.mmvTriggers[2] == 0);
    CHECK(trace.movieCalls == 2);

    menuVars.sbit = 7;
    CHECK(menu_handleMenuTriggers(0x33) == 0);
    CHECK(secretBits == (UINT32_C(1) << 7));
    CHECK(menu_handleMenuTriggers(0x33) == 0);
    CHECK(secretBits == 0);
    menuVars.aibit = 12;
    CHECK(menu_handleMenuTriggers(0x4a) == 0);
    CHECK((abGlobalBits[1] & UINT8_C(0x10)) != 0);
    GameStruct.ModelSelect[0] = 2;
    menuVars.jediDebugCombo = 0;
    menuVars.comboSelect = 7;
    CHECK(game_getCombo(2, 7) == 0);
    CHECK(menu_handleMenuTriggers(0x7a) == 0);
    CHECK(game_getCombo(2, 7) != 0);
    CHECK(menu_handleMenuTriggers(0x7a) == 0);
    CHECK(game_getCombo(2, 7) == 0);

    menuVars.itemSelect = 9;
    CHECK(menu_handleMenuTriggers(10) == 0);
    CHECK(menuVars.itemSelect == 0);
    CHECK(menuVars.menuMode[1] == 0x99);
    menuVars.menuModeSP = 0;
    GameStruct.gameMode = 0;
    GameStruct.inMenuFlag = 1;
    GameStruct.NumPlayers = 2;
    CHECK(menu_handleMenuTriggers(0x97) == 0);
    CHECK(GameStruct.NumPlayers == 1);
    CHECK(GameStruct.gameMode == 6);
    CHECK(GameStruct.inMenuFlag == 0);
    CHECK(menuVars.menuModeSP == 0);
    GameStruct.gameMode = 0;
    GameStruct.inMenuFlag = 1;
    GameStruct.NumPlayers = 1;
    CHECK(menu_handleMenuTriggers(0x98) == 0);
    CHECK(GameStruct.NumPlayers == 2);
    CHECK(GameStruct.gameMode == 6);
    CHECK(GameStruct.inMenuFlag == 0);
    CHECK(menuVars.menuModeSP == 0);
    trace.fallbackCalls = 0;
    CHECK(menu_handleMenuTriggers(0x3a) == 1);
    CHECK(menuVars.menuModeSP == 0);
    CHECK(menu_handleMenuTriggers(0x52) == 0);
    CHECK(menuVars.menuModeSP == 0);

    CHECK(menu_handleMenuTriggers(0x93) == 0);
    CHECK(trace.exitCalls == 1);
    menuVars.menuModeSP = 1;
    menuVars.menuMode[0] = 0;
    menuVars.menuMode[1] = 0x92;
    CHECK(menu_handleMenuTriggers(0x94) == 0);
    CHECK(menuVars.menuModeSP == 0);

    CHECK(menu_handleMenuTriggers(0x36) == 0);
    CHECK(menuVars.menuMode[1] == 0);
    CHECK(trace.cleanupCalls == 1);
    menuVars.menuModeSP = 0;
    LevelSelect = 9;
    CHECK(menu_handleMenuTriggers(0x3c) == 0);
    CHECK(trace.scanCalls == 31);
    CHECK(trace.lastScanLevel == 30);
    CHECK(LevelSelect == 9);

    GameStruct.ModelSelect[0] = 0;
    GameStruct.ModelSelect[1] = 1;
    GameStruct.NumPlayers = 1;
    LevelSelect = 11;
    secretBits = 0;
    CHECK(menu_handleMenuTriggers(0x3d) == 0);
    CHECK(strcmp(trace.lastCue, "xlocklvl") == 0);
    CHECK(menuVars.menuModeSP == 0);
    secretBits = 1;
    CHECK(menu_handleMenuTriggers(0x3d) == 0);
    CHECK(strcmp(trace.lastCue, "xlvselct") == 0);
    CHECK(savedNumPlayer == 1);
    CHECK(menuVars.menuMode[1] == 0x66);
    menuVars.menuModeSP = 0;
    LevelSelect = 0;
    CHECK(menu_handleMenuTriggers(0x3d) == 0);
    CHECK(menuVars.menuModeSP == 0);
    CHECK(menu_handleMenuTriggers(0x3e) == 0);
    CHECK(GameStruct.gameMode == 2);
    CHECK(strcmp(trace.lastCue, "xlvselct") == 0);

    GameStruct.NumPlayers = 1;
    menuVars.subplayers[0] = 2;
    CHECK(menu_handleMenuTriggers(0x18) == 0);
    CHECK(GameStruct.ModelSelect[0] == modisorder2[2]);
    CHECK(menuVars.pSelect == 1);
    CHECK(menuVars.menuMode[1] == 0x1a);
    CHECK(strcmp(trace.lastCue, "xjedsel") == 0);
    menuVars.menuModeSP = 0;
    GameStruct.NumPlayers = 2;
    menuVars.pSelect = 1;
    menuVars.subplayers[1] = 3;
    CHECK(menu_handleMenuTriggers(0x19) == 0);
    CHECK(GameStruct.ModelSelect[1] == modisorder2[3]);
    CHECK(menuVars.pSelect == 3);
    CHECK(menuVars.menuMode[1] == 0x1a);
    CHECK(strcmp(trace.lastCue, "xjedsel") == 0);

    menuVars.tempLevmod = 14;
    GameStruct.gameMode = 0;
    GameStruct.inMenuFlag = 1;
    CHECK(menu_handleMenuTriggers(0x44) == 0);
    CHECK(LevelSelect == 14);
    CHECK(GameStruct.gameMode == 4);
    CHECK(GameStruct.inMenuFlag == 0);
    CHECK(menu_handleMenuTriggers(0x50) == 0);
    CHECK(menuVars.loadSaveMode == 8);
    CHECK(menu_handleMenuTriggers(0x4c) == 0);
    CHECK(menuVars.loadSaveMode == 0);

    menuVars.menuModeSP = 1;
    menuVars.menuMode[0] = 1;
    menuVars.dialogBox1 = 0;
    CHECK(menu_handleMenuTriggers(0x46) == 0);
    CHECK(menuVars.menuModeSP == 0);
    menuVars.dialogBox1 = 1;
    GameStruct.gameMode = 0;
    GameStruct.inMenuFlag = 1;
    CHECK(menu_handleMenuTriggers(0x46) == 0);
    CHECK(GameStruct.gameMode == 9);
    CHECK(GameStruct.inMenuFlag == 0);
    GameStruct.letterboxFlag = 0;
    GameStruct.letterboxFlag2 = 1;
    OptionStruct.Music = 0;
    CHECK(menu_handleMenuTriggers(0x40) == 0);
    CHECK(GameStruct.letterboxFlag == 1);
    CHECK(GameStruct.gameMode == 6);
    CHECK(GameStruct.inMenuFlag == 0);
    GameStruct.GameState = UINT32_C(0x02000004);
    CHECK(menu_handleMenuTriggers(0x41) == 0);
    CHECK(GameStruct.GameState == 4);

    CHECK(menu_handleMenuTriggers(0x4b) == 0);
    CHECK(trace.saveCalls == 1);
    menuVars.menuModeSP = 0;
    menuVars.mmSelect1[0] = 4;
    menuVars.mmSelect2[0] = 5;
    menuVars.selectp = menuVars.mmSelect1;
    menuVars.mmSelect1[0] = 3;
    menuVars.td.jedi = 2;
    menuVars.td.newcombos[3] = 9;
    menuVars.scoreMode = 1;
    menuVars.scoreNextMode = 6;
    menuVars.scoreCurrentPlayer = 0;
    CHECK(menu_handleMenuTriggers(0x4d) == 0);
    CHECK(game_getCombo(2, 9) != 0);
    CHECK(menuVars.mmSelect1[0] == 0);
    CHECK(menuVars.mmSelect2[0] == 0);
    CHECK(menuVars.awardSet[0].awardCount == 1);
    CHECK(trace.saveCalls == 2);
    menuVars.menuModeSP = 0;
    LevelSelect = 7;
    CHECK(menu_handleMenuTriggers(0x4f) == 0);
    CHECK(menuVars.menuMode[1] == 0x66);
    menuVars.menuModeSP = 0;
    LevelSelect = 14;
    CHECK(menu_handleMenuTriggers(0x4f) == 0);
    CHECK(menuVars.menuMode[1] == 0);

    menuVars.menuModeSP = 0;
    p2Connected = 0;
    GameStruct.NumPlayers = 2;
    CHECK(menu_handleMenuTriggers(0x54) == 0);
    CHECK(trace.cleanupCalls == 2);
    CHECK(menuVars.titleArt == 1);
    CHECK(GameStruct.NumPlayers == 1);
    CHECK(GameStruct.gameMode == 9);
    CHECK(GameStruct.inMenuFlag == 1);
    CHECK(trace.inMenuCalls == 1 && trace.lastInMenu == 1);
    CHECK(menuVars.menuMode[1] == 0);

    CHECK(menu_handleMenuTriggers(0x55) == 0);
    CHECK(LevelSelect == 25);
    CHECK(GameStruct.versusModeFlag == 1);
    CHECK(GameStruct.NumPlayers == 2);
    CHECK(GameStruct.gameMode == 2);

    GameStruct.continueAble = 1;
    CHECK(menu_handleMenuTriggers(0x59) == 0);
    CHECK(menuVars.trainingLevel == 3);
    CHECK(LevelSelect == 19);
    CHECK(GameStruct.gameMode == 2);

    menuVars.menuModeSP = 0;
    GameStruct.NumPlayers = 2;
    CHECK(menu_handleMenuTriggers(0x69) == 0);
    CHECK(GameStruct.difficulty == 1);
    CHECK(GameStruct.NumPlayers == 2);
    CHECK(menuVars.menuMode[1] == 4);
    CHECK(trace.fallbackCalls == 1);
    menuVars.menuModeSP = 0;
    CHECK(menu_handleMenuTriggers(0x6a) == 0);
    CHECK(GameStruct.NumPlayers == 1);
    CHECK(menuVars.menuMode[1] == 0x37);
    menuVars.menuModeSP = 0;
    CHECK(menu_handleMenuTriggers(0x6b) == 0);
    CHECK(GameStruct.NumPlayers == 2);
    CHECK(menuVars.menuMode[1] == 0x37);
    CHECK(trace.fallbackCalls == 2);

    defaultOptionStruct.Music = 1;
    defaultOptionStruct.musicVolume = 63;
    OptionStruct.Music = 0;
    OptionStruct.musicVolume = 1;
    menuVars.menuMode[menuVars.menuModeSP & 7u] = 0x10;
    CHECK(menu_handleMenuTriggers(0x67) == 0);
    CHECK(OptionStruct.Music == 1);
    CHECK(OptionStruct.musicVolume == 63);
    defaultOptionStruct.WalkLimit[0] = 9;
    defaultOptionStruct.RunLimit[0] = 3;
    OptionStruct.WalkLimit[0] = 1;
    OptionStruct.RunLimit[0] = 1;
    menuVars.menuMode[menuVars.menuModeSP & 7u] = 0x23;
    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    menuVars.pad[1] = 0;
    CHECK(menu_handleMenuTriggers(0x67) == 0);
    CHECK(OptionStruct.WalkLimit[0] == 9);
    CHECK(OptionStruct.RunLimit[0] == 3);

    CHECK(menu_handleMenuTriggers(0x77) == 0);
    CHECK(trace.transformCalls == 1);

    GameStruct.ModelSelect[0] = 2;
    menuVars.scoreCurrentPlayer = 0;
    menuVars.scoreMode = 1;
    menuVars.scoreNextMode = 6;
    GameStruct.maxEnergyLevels[2] = 100;
    GameStruct.maxEnergyLineLength[2] = 25;
    CHECK(menu_handleMenuTriggers(0x7d) == 0);
    CHECK(GameStruct.maxEnergyLevels[2] == 120);
    CHECK(GameStruct.maxEnergyLineLength[2] == 30);
    CHECK(jediUpgrades[2].healthUpgrades == 1);
    CHECK(trace.saveCalls == 3);
    menuVars.scoreMode = 1;
    menuVars.scoreNextMode = 7;
    GameStruct.maxForceLevels[2] = 80;
    GameStruct.maxForceLineLength[2] = 20;
    CHECK(menu_handleMenuTriggers(0x7f) == 0);
    CHECK(GameStruct.maxForceLevels[2] == 100);
    CHECK(GameStruct.maxForceLineLength[2] == 25);
    CHECK(jediUpgrades[2].forceUpgrades == 1);
    CHECK(trace.saveCalls == 4);

    menuVars.menuModeSP = 0;
    menuVars.pad[0] = 0;
    CHECK(menu_handleMenuTriggers(0x8d) == 0);
    CHECK(menuVars.menuModeSP == 0);
    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    CHECK(menu_handleMenuTriggers(0x8d) == 0);
    CHECK(menuVars.controlPlayer == 0);
    CHECK(menuVars.menuMode[1] == 0x24);
    menuVars.menuModeSP = 0;
    menuVars.pad[1] = JPB_PAD_COMBO_SOUTH;
    CHECK(menu_handleMenuTriggers(0x8e) == 0);
    CHECK(menuVars.controlPlayer == 1);
    CHECK(menuVars.menuMode[1] == 0x24);

    OptionStruct.ResolutionChanged = 5;
    OptionStruct.WindowMode = 2;
    OptionStruct.ScreenWidth = 640;
    OptionStruct.ScreenHeight = 480;
    saved_resolution = g_resolutions[5];
    g_resolutions[5].width = 1920;
    g_resolutions[5].height = 1080;
    CHECK(menu_handleMenuTriggers(0x96) == 0);
    CHECK(OptionStruct.ScreenWidth == 1920);
    CHECK(OptionStruct.ScreenHeight == 1080);
    CHECK(OptionStruct.WindowMode == 2);
    CHECK(OptionStruct.ResolutionChanged == 5);
    CHECK(resolutionUpdated == 1u);
    CHECK(newWidth == 1920);
    CHECK(newHeight == 1080);
    CHECK(newWindowMode == 2);
    g_resolutions[5] = saved_resolution;

    menuVars.menuModeSP = 0;
    CHECK(menu_handleMenuTriggers(0x9a) == 0);
    CHECK(tempPlayersVs == 1);
    CHECK(GameStruct.NumPlayers == 1);
    CHECK(menuVars.menuMode[1] == 0x0d);
    CHECK(trace.fallbackCalls == 2);
    menuVars.menuModeSP = 0;
    CHECK(menu_handleMenuTriggers(0x9b) == 0);
    CHECK(tempPlayersVs == 2);
    CHECK(GameStruct.NumPlayers == 1);
    CHECK(menuVars.menuMode[1] == 0x0d);
    CHECK(trace.fallbackCalls == 3);

    GameStruct.aCharacterData[0].Energy = 20;
    GameStruct.aCharacterData[1].Energy = 30;
    CHECK(menu_handleMenuTriggers(0x87) == 0);
    CHECK(menu_handleMenuTriggers(0x88) == 0);
    CHECK(GameStruct.aCharacterData[0].Energy == 0);
    CHECK(GameStruct.aCharacterData[1].Energy == 0);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    sound_FreeBank(0);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_level_eligibility_and_score_modes(void)
{
    int upgrade_level = -1;
    int32_t target;
    unsigned model;
    unsigned level;

    reset_menu_state();
    GameStruct.ModelSelect[0] = 2;
    GameStruct.ModelSelect[1] = 3;
    GameStruct.NumPlayers = 1;
    jediUpgrades[2].awardData[4] = 2;
    CHECK(jedi_CheckValidLevel(4, &upgrade_level) == 1);
    CHECK(upgrade_level == 2);
    GameStruct.NumPlayers = 2;
    jediUpgrades[3].awardData[4] = 7;
    CHECK(jedi_CheckValidLevel(4, &upgrade_level) == 1);
    CHECK(upgrade_level == 3);
    GameStruct.ModelSelect[1] = 17;
    CHECK(jedi_CheckValidLevel(14, &upgrade_level) == 1);
    CHECK(upgrade_level == 0);
    CHECK(jedi_CheckValidLevel(15, &upgrade_level) == 0);
    GameStruct.ModelSelect[1] = 3;
    secretBits = UINT32_C(0x0f);
    CHECK(jedi_CheckValidLevel(11, &upgrade_level) == 1);
    CHECK(jedi_CheckValidLevel(12, &upgrade_level) == 1);
    CHECK(jedi_CheckValidLevel(13, &upgrade_level) == 1);
    CHECK(jedi_CheckValidLevel(14, &upgrade_level) == 0);
    GameStruct.NumPlayers = 1;
    CHECK(jedi_CheckValidLevel(14, &upgrade_level) == 1);

    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    for (model = 0; model < 5; ++model) {
        for (level = 1; level <= 10; ++level) {
            jediUpgrades[model].awardData[level] = (int8_t)model;
        }
    }
    GameStruct.ModelSelect[0] = 7;
    GameStruct.ModelSelect[1] = 8;
    GameStruct.NumPlayers = 1;
    secretBits = UINT32_C(0x27f);
    CHECK(menu_calcCompletionPoints() == 98u);
    CHECK(GameStruct.ModelSelect[0] == 7);
    CHECK(GameStruct.ModelSelect[1] == 8);
    GameStruct.NumPlayers = 2;
    secretBits = 0;
    CHECK(menu_calcCompletionPoints() == 91u);
    CHECK(GameStruct.ModelSelect[0] == 7);
    CHECK(GameStruct.ModelSelect[1] == 8);
    for (model = 0; model < 5; ++model) {
        for (level = 1; level <= 10; ++level) {
            jediUpgrades[model].awardData[level] = 3;
        }
    }
    GameStruct.NumPlayers = 1;
    secretBits = UINT32_C(0x27f);
    CHECK(menu_calcCompletionPoints() == 157u);

    menuVars.menuModeSP = 2;
    menuVars.mmSelect1[2] = 4;
    menuVars.mmSelect2[2] = 5;
    menuVars.scoreCurrentPlayer = 1;
    menuVars.scoreScore = 1000;
    target = 1450;
    menuVars.awardSet[1].pointAwarded[0] = (uint32_t)target;
    target = 1750;
    menuVars.awardSet[1].pointAwarded[1] = (uint32_t)target;
    target = 2100;
    menuVars.awardSet[1].pointAwarded[2] = (uint32_t)target;
    menu_setScoreMode(9, 0);
    CHECK(menuVars.scoreMode == 9);
    CHECK(menuVars.pointSeek == 450);
    CHECK(menuVars.scoreNextMode == 10);
    CHECK(menuVars.mmSelect1[2] == 0);
    CHECK(menuVars.mmSelect2[2] == 0);
    menu_setScoreMode(10, 0);
    CHECK(menuVars.pointSeek == 750);
    CHECK(menuVars.scoreNextMode == 11);
    menu_setScoreMode(11, 0);
    CHECK(menuVars.pointSeek == 1100);
    CHECK(menuVars.scoreNextMode == 1);
    menu_setScoreMode(13, 0);
    CHECK(menuVars.pointSeek == 450);
    CHECK(menuVars.scoreNextMode == 1);
    menu_setScoreMode(6, 0);
    CHECK(menuVars.awardSet[1].awardCount == 1);
    return 0;
}

static int test_save_game_struct_initialization(void)
{
    saveGameStruct *save;
    int16_t saved_model_one;
    int16_t saved_model_two;
    unsigned model;
    unsigned level;
    size_t index;

    reset_menu_state();
    memset(&cardLoadBuffer, 0xa5, sizeof(cardLoadBuffer));
    memset(&OptionStruct, 0x3c, sizeof(OptionStruct));
    memset(GameStruct.maxEnergyLevels, 0x11,
           sizeof(GameStruct.maxEnergyLevels));
    memset(GameStruct.maxEnergyLineLength, 0x22,
           sizeof(GameStruct.maxEnergyLineLength));
    memset(GameStruct.maxForceLevels, 0x33,
           sizeof(GameStruct.maxForceLevels));
    memset(GameStruct.maxForceLineLength, 0x44,
           sizeof(GameStruct.maxForceLineLength));
    memset(GameStruct.aCharacterData, 0x55,
           sizeof(GameStruct.aCharacterData));
    memset(GameStruct.jediComboMask, 0x66,
           sizeof(GameStruct.jediComboMask));
    memset(GameStruct.jediLevelPlayed, 0x77,
           sizeof(GameStruct.jediLevelPlayed));
    memset(GameStruct.jediScorePerLevel, 0x88,
           sizeof(GameStruct.jediScorePerLevel));
    memset(GameStruct.checkpoint, 0x99,
           sizeof(GameStruct.checkpoint));
    memset(abGlobalBits, 0xab, sizeof(abGlobalBits));
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    for (model = 0; model < 5; ++model) {
        for (level = 1; level <= 10; ++level) {
            jediUpgrades[model].awardData[level] = (int8_t)model;
        }
    }

    menuVars.cardSlotSelect = 1;
    menuVars.pplayers[0] = 7;
    menuVars.pplayers[1] = 8;
    GameStruct.CurrentLevel = 9;
    GameStruct.NumPlayers = 1;
    GameStruct.mNumContinues = 6;
    GameStruct.ContinuesUsed = 2;
    GameStruct.ModelSelect[0] = 7;
    GameStruct.ModelSelect[1] = 8;
    GameStruct.AIDamage = 1;
    GameStruct.JediDamage = 2;
    GameStruct.HTHRate = 3;
    GameStruct.RangedRate = 4;
    GameStruct.BlockRate = 5;
    GameStruct.ComboLevel = 6;
    GameStruct.ForceLevel = 7;
    secretBits = UINT32_C(0x27f);
    saved_model_one = GameStruct.ModelSelect[0];
    saved_model_two = GameStruct.ModelSelect[1];

    initsavegamestruct();
    save = &cardLoadBuffer.saveGame[1];

    CHECK(memcmp(
              &cardLoadBuffer.header.OptionStruct,
              &OptionStruct,
              sizeof(OptionStruct)) == 0);
    CHECK(cardLoadBuffer.header.saveChecksum == UINT32_C(0xa5a5a5a5));
    CHECK(cardLoadBuffer.header.optionChecksum == UINT32_C(0xa5a5a5a5));
    CHECK(cardLoadBuffer.header.msg.pos[0][0] == UINT8_C(0xa5));
    CHECK(save->saveFileVer == (int32_t)UINT32_C(0xa5a5a5a5));
    CHECK(save->validFlag == 1);
    CHECK(save->gamenum == 1);
    CHECK(save->lastlevel == 9);
    for (index = 0; index < JPB_GAME_JEDI_MODEL_CAPACITY; ++index) {
        CHECK(((uint8_t *)save->jediLevel)[index] == 0);
    }
    for (; index < sizeof(save->jediLevel); ++index) {
        CHECK(((uint8_t *)save->jediLevel)[index] == UINT8_C(0xa5));
    }
    CHECK(memcmp(save->maxEnergyLevels,
                 GameStruct.maxEnergyLevels,
                 sizeof(save->maxEnergyLevels)) == 0);
    CHECK(memcmp(save->maxEnergyLineLength,
                 GameStruct.maxEnergyLineLength,
                 sizeof(save->maxEnergyLineLength)) == 0);
    CHECK(memcmp(save->maxForceLevels,
                 GameStruct.maxForceLevels,
                 sizeof(save->maxForceLevels)) == 0);
    CHECK(memcmp(save->maxForceLineLength,
                 GameStruct.maxForceLineLength,
                 sizeof(save->maxForceLineLength)) == 0);
    CHECK(memcmp(save->aCharacterData,
                 GameStruct.aCharacterData,
                 sizeof(save->aCharacterData)) == 0);
    CHECK(memcmp(save->jediComboMask,
                 GameStruct.jediComboMask,
                 sizeof(save->jediComboMask)) == 0);
    CHECK(memcmp(save->jediLevelPlayed,
                 GameStruct.jediLevelPlayed,
                 sizeof(save->jediLevelPlayed)) == 0);
    CHECK(memcmp(save->jediScorePerLevel,
                 GameStruct.jediScorePerLevel,
                 sizeof(save->jediScorePerLevel)) == 0);
    CHECK(memcmp(save->checkpoint,
                 GameStruct.checkpoint,
                 sizeof(save->checkpoint)) == 0);
    CHECK(memcmp(save->abGlobalBits,
                 abGlobalBits,
                 sizeof(save->abGlobalBits)) == 0);
    CHECK(memcmp(save->jediUpgrades,
                 jediUpgrades,
                 sizeof(save->jediUpgrades)) == 0);
    CHECK(save->completionPoints == 98u);
    CHECK(save->NumPlayers == 1);
    CHECK(save->mNumContinues == 6);
    CHECK(save->ContinuesUsed == 2);
    CHECK(save->secretBits == UINT32_C(0x27f));
    CHECK(save->AIDamage == 1);
    CHECK(save->JediDamage == 2);
    CHECK(save->HTHRate == 3);
    CHECK(save->RangedRate == 4);
    CHECK(save->BlockRate == 5);
    CHECK(save->ComboLevel == 6);
    CHECK(save->ForceLevel == 7);
    CHECK(save->players[0] == 7);
    CHECK(save->players[1] == 8);
    CHECK(save->continueAble == (char)UINT8_C(0xa5));
    CHECK(save->difficulty == (char)UINT8_C(0xa5));
    CHECK(save->gameCompleted == (int32_t)UINT32_C(0xa5a5a5a5));
    CHECK(save->unlockedExtraCharacters == UINT16_C(0xa5a5));
    CHECK(GameStruct.ModelSelect[0] == saved_model_one);
    CHECK(GameStruct.ModelSelect[1] == saved_model_two);
    CHECK(cardLoadBuffer.saveGame[0].validFlag == UINT8_C(0xa5));
    CHECK(cardLoadBuffer.saveGame[2].validFlag == UINT8_C(0xa5));
    return 0;
}

static int test_vrm_font_metadata_load(void)
{
    static const char root[] = "jpb_menu_vrm_test";
    static const char res[] = "jpb_menu_vrm_test/res";
    static const char front[] = "jpb_menu_vrm_test/res/front";
    static const char path[] =
        "jpb_menu_vrm_test/res/front/font_fixture.vrm";
    unsigned char fixture[28] = {0};
    unsigned char destination[sizeof(fixture)];
    uint32_t record_bytes = 16;
    uint32_t tpage_count = 1;
    uint16_t page;
    FILE *stream;

    reset_menu_state();
#if defined(_WIN32)
    (void)_mkdir(root);
    (void)_mkdir(res);
    (void)_mkdir(front);
#endif
    memcpy(fixture, &record_bytes, sizeof(record_bytes));
    page = UINT16_C(0x1234);
    memcpy(fixture + 4, &page, sizeof(page));
    fixture[8] = 10;
    fixture[9] = 11;
    fixture[10] = 12;
    fixture[11] = 13;
    page = UINT16_C(0x5678);
    memcpy(fixture + 12, &page, sizeof(page));
    fixture[16] = 20;
    fixture[17] = 21;
    fixture[18] = 22;
    fixture[19] = 23;
    memcpy(fixture + 24, &tpage_count, sizeof(tpage_count));
    stream = fopen(path, "wb");
    CHECK(stream != NULL);
    CHECK(fwrite(fixture, 1, sizeof(fixture), stream) == sizeof(fixture));
    CHECK(fclose(stream) == 0);
    CHECK(jpb_ResourceSetBasePath(root) == 1);
    memset(destination, 0, sizeof(destination));
    memset(&fontSpec[37], 0, 2 * sizeof(fontSpec[0]));

    loadVRM("font_fixture.vrm", 37, 999, destination, 73);
    CHECK(fontSpec[37].xypage == UINT16_C(0x1234));
    CHECK(fontSpec[37].clut == 73);
    CHECK(fontSpec[37].y == 10);
    CHECK(fontSpec[37].x == 11);
    CHECK(fontSpec[37].h == 12);
    CHECK(fontSpec[37].w == 13);
    CHECK(fontSpec[38].xypage == UINT16_C(0x5678));
    CHECK(fontSpec[38].clut == 73);
    CHECK(fontSpec[38].y == 20);
    CHECK(fontSpec[38].x == 21);
    CHECK(fontSpec[38].h == 22);
    CHECK(fontSpec[38].w == 23);

    loadvrmFlag = 1;
    memset(destination, 0xcc, sizeof(destination));
    fontSpec[37].xypage = UINT16_C(0xbeef);
    loadVRM("missing.vrm", 37, 0, destination, 4);
    CHECK(fontSpec[37].xypage == UINT16_C(0xbeef));
    CHECK(destination[0] == UINT8_C(0xcc));

    CHECK(jpb_ResourceSetBasePath(NULL) == 0);
    CHECK(remove(path) == 0);
#if defined(_WIN32)
    CHECK(_rmdir(front) == 0);
    CHECK(_rmdir(res) == 0);
    CHECK(_rmdir(root) == 0);
#endif
    return 0;
}

static int test_score_smackdown(void)
{
    MenuSoundTrace sound_trace;

    reset_menu_state();
    memset(&sound_trace, 0, sizeof(sound_trace));
    sound_FreeBank(0);
    CHECK(sound_LoadBank("resident", 0) == 0);
    jpb_SoundSetPlaySfxHook(trace_menu_sound, &sound_trace);

    menuVars.scoreScore = 1000;
    menuVars.scoreBeeper = 0;
    menuVars.bar_y = UINT32_C(0x150000);
    menuVars.bar_speed = UINT32_C(0x100);
    CHECK(menu_scoreSmackdown(2000) == 0u);
    CHECK(menuVars.scoreScore == 1250u);
    CHECK(menuVars.scoreBeeper == 1u);
    CHECK(menuVars.bar_y == UINT32_C(0x14ff00));
    CHECK(sound_trace.calls == 1);
    CHECK(sound_trace.position == NULL);
    CHECK(sound_trace.bank == 0);
    CHECK(sound_trace.flag == 8u);
    CHECK(strcmp(sound_trace.sound, "xpointbp") == 0);

    CHECK(menu_scoreSmackdown(2000) == 0u);
    CHECK(menuVars.scoreScore == 1500u);
    CHECK(menuVars.scoreBeeper == 2u);
    CHECK(sound_trace.calls == 1);
    CHECK(menu_scoreSmackdown(2000) == 0u);
    CHECK(menuVars.scoreScore == 1750u);
    CHECK(menuVars.scoreBeeper == 0u);
    CHECK(sound_trace.calls == 1);
    CHECK(menu_scoreSmackdown(2000) == 1u);
    CHECK(menuVars.scoreScore == 2000u);
    CHECK(menuVars.scoreBeeper == 1u);
    CHECK(sound_trace.calls == 2);

    menuVars.bar_y = UINT32_C(0x140000);
    menuVars.scoreScore = 1900;
    CHECK(menu_scoreSmackdown(2000) == 1u);
    CHECK(menuVars.scoreScore == 2000u);
    CHECK(menuVars.bar_y == UINT32_C(0x140000));
    CHECK(menu_scoreSmackdown(1500) == 1u);
    CHECK(menuVars.scoreScore == 1500u);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    return 0;
}

static int test_vs_extra_player_mapping(void)
{
    static const struct {
        int model;
        int texture;
        int text;
    } expected[] = {
        {8, 0x18e, 0x154}, {15, 0x18c, 0x155},
        {17, 0x187, 0x156}, {18, 0x191, 0x157},
        {21, 0x188, 0x158}, {22, 0x191, 0x159},
        {26, 0x189, 0x15a}, {30, 0x196, 0x15b},
        {36, 0x190, 0x15c}, {37, 0x18f, 0x15d},
        {41, 0x191, 0x15e}, {48, 0x192, 0x15f},
        {49, 0x193, 0x160}, {50, 0x194, 0x161},
        {51, 0x195, 0x162}, {52, 0x195, 0x163},
        {53, 0x18a, 0x164}
    };
    char labels[sizeof(expected) / sizeof(expected[0])][8];
    char *name;
    int texture;
    size_t index;

    reset_menu_state();
    for (index = 0; index < sizeof(expected) / sizeof(expected[0]); ++index) {
        (void)snprintf(labels[index], sizeof(labels[index]), "p%u", (unsigned)index);
        allText[expected[index].text] = labels[index];
        name = NULL;
        texture = -1;
        CHECK(newMenu_GetVSExtraPlayer(
                  &name, &texture, expected[index].model) == 1);
        CHECK(name == labels[index]);
        CHECK(texture == expected[index].texture);
    }

    name = (char *)(uintptr_t)UINT64_C(0x1234);
    texture = 0x5678;
    CHECK(newMenu_GetVSExtraPlayer(&name, &texture, 9) == 0);
    CHECK(name == (char *)(uintptr_t)UINT64_C(0x1234));
    CHECK(texture == 0x5678);
    GameStruct.ModelSelect[0] = 22;
    scoreloadart(0);
    GameStruct.ModelSelect[1] = 53;
    scoreloadart(1);
    return 0;
}

static int test_player_selection_summary_screens(void)
{
    static const unsigned spec_indices[] = {
        0x198u, 0x17eu, 0x18eu, 0x161u
    };
    FONTSPEC saved_specs[
        sizeof(spec_indices) / sizeof(spec_indices[0])];
    _Material *saved_materials[4];
    _Material materials[4];
    char *saved_player_one = allText[332];
    char *saved_player_two_extra = allText[0x154];
    float saved_scale_mm = scaleAdjustmentMM;
    float saved_scale_x = gPSXDrawScaleX;
    float saved_scale_y = gPSXDrawScaleY;
    float saved_scale_w = gPSXDrawScaleW;
    float saved_scale_h = gPSXDrawScaleH;
    uint8_t saved_rgb_offset = frontRGBoff;
    MenuTextureDrawTrace texture_trace;
    MenuDrawTrace text_trace;
    size_t index;

    reset_menu_state();
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    memset(materials, 0, sizeof(materials));
    for (index = 0;
         index < sizeof(spec_indices) / sizeof(spec_indices[0]);
         ++index) {
        saved_specs[index] = fontSpec[spec_indices[index]];
        saved_materials[index] = menuTextures[index];
        memset(&fontSpec[spec_indices[index]], 0, sizeof(FONTSPEC));
        fontSpec[spec_indices[index]].clut = (uint16_t)index;
        fontSpec[spec_indices[index]].w = 16;
        fontSpec[spec_indices[index]].h = 16;
        menuTextures[index] = &materials[index];
    }
    allText[332] = "Obi-Wan";
    allText[0x154] = "Battle Droid";
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    gPSXDrawScaleW = 1.0f;
    gPSXDrawScaleH = 1.0f;
    frontRGBoff = 0;
    p1Disconnected = 1;
    GameStruct.gameMode = 6;
    GameStruct.ModelSelect[0] = 0;
    GameStruct.jediLevelPlayed[0][3] = 1;
    jediUpgrades[0].awardData[1] = 3;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);

    CHECK(newMenu_SelectPlayers(1) == 1);
    CHECK(texture_trace.calls == 4);
    CHECK(texture_trace.materials[0] == &materials[0]);
    CHECK(texture_trace.destinations[0].left == 154);
    CHECK(texture_trace.destinations[0].top == 208);
    CHECK(texture_trace.materials[1] == &materials[0]);
    CHECK(texture_trace.destinations[1].left == 116);
    CHECK(texture_trace.destinations[1].top == 208);
    CHECK(texture_trace.materials[2] == &materials[1]);
    CHECK(texture_trace.destinations[2].left == 257);
    CHECK(texture_trace.destinations[2].top == 111);
    CHECK(texture_trace.colors[2].r == 128);
    CHECK(texture_trace.colors[2].cd == 255);
    CHECK(texture_trace.materials[3] == &materials[3]);
    CHECK(texture_trace.destinations[3].left == 0);
    CHECK(texture_trace.destinations[3].top == 4);
    CHECK(text_trace.calls == 5);
    CHECK(utf16_matches_utf8(text_trace.text[0], "Obi-Wan"));
    CHECK(text_trace.x[0] == 20 && text_trace.y[0] == 70);
    CHECK(text_trace.scale[0] == 0.9f);
    CHECK(utf16_matches_utf8(text_trace.text[1], "Obi-Wan"));
    CHECK(utf16_matches_utf8(text_trace.text[2], "Level : 3"));
    CHECK(text_trace.x[2] == 100 && text_trace.y[2] == 105);
    CHECK(text_trace.scale[2] == 0.6f);
    CHECK(utf16_matches_utf8(text_trace.text[3], "Skill : 10"));
    CHECK(text_trace.x[3] == 100 && text_trace.y[3] == 125);
    CHECK(utf16_matches_utf8(text_trace.text[4], "player1"));
    CHECK(text_trace.x[4] == 20 && text_trace.y[4] == 145);
    CHECK(text_trace.scale[4] == 0.55f);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    GameStruct.ModelSelect[1] = 8;
    CHECK(newMenu_SelectPlayers(2) == 1);
    CHECK(texture_trace.calls == 6);
    CHECK(texture_trace.materials[0] == &materials[0]);
    CHECK(texture_trace.destinations[0].left == 321);
    CHECK(texture_trace.destinations[0].top == 155);
    CHECK(texture_trace.destinations[1].left == 283);
    CHECK(texture_trace.destinations[1].top == 155);
    CHECK(texture_trace.destinations[2].left == 325);
    CHECK(texture_trace.destinations[2].top == 283);
    CHECK(texture_trace.destinations[3].left == 287);
    CHECK(texture_trace.destinations[3].top == 283);
    CHECK(texture_trace.materials[4] == &materials[1]);
    CHECK(texture_trace.destinations[4].left == 7);
    CHECK(texture_trace.destinations[4].top == 32);
    CHECK(texture_trace.materials[5] == &materials[2]);
    CHECK(texture_trace.destinations[5].left == 457);
    CHECK(texture_trace.destinations[5].top == 178);
    CHECK(text_trace.calls == 8);
    CHECK(utf16_matches_utf8(text_trace.text[0], "Obi-Wan"));
    CHECK(text_trace.x[0] == 255 && text_trace.y[0] == 40);
    CHECK(utf16_matches_utf8(text_trace.text[1], "player1"));
    CHECK(text_trace.x[1] == 415 && text_trace.y[1] == 115);
    CHECK(text_trace.scale[1] == 0.55f);
    CHECK(utf16_matches_utf8(text_trace.text[2], "player2"));
    CHECK(text_trace.x[2] == 165 && text_trace.y[2] == 353);
    CHECK(utf16_matches_utf8(text_trace.text[3], "Battle Droid"));
    CHECK(text_trace.x[3] == 165 && text_trace.y[3] == 413);
    CHECK(text_trace.scale[3] == 0.9f);
    CHECK(utf16_matches_utf8(text_trace.text[4], "Level : 3"));
    CHECK(text_trace.x[4] == 300 && text_trace.y[4] == 75);
    CHECK(utf16_matches_utf8(text_trace.text[5], "Skill : 10"));
    CHECK(text_trace.x[5] == 300 && text_trace.y[5] == 95);
    CHECK(utf16_matches_utf8(text_trace.text[6], "Level : 0"));
    CHECK(text_trace.x[6] == 250 && text_trace.y[6] == 370);
    CHECK(utf16_matches_utf8(text_trace.text[7], "Skill : 0"));
    CHECK(text_trace.x[7] == 250 && text_trace.y[7] == 390);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    for (index = 0;
         index < sizeof(spec_indices) / sizeof(spec_indices[0]);
         ++index) {
        fontSpec[spec_indices[index]] = saved_specs[index];
        menuTextures[index] = saved_materials[index];
    }
    allText[332] = saved_player_one;
    allText[0x154] = saved_player_two_extra;
    scaleAdjustmentMM = saved_scale_mm;
    gPSXDrawScaleX = saved_scale_x;
    gPSXDrawScaleY = saved_scale_y;
    gPSXDrawScaleW = saved_scale_w;
    gPSXDrawScaleH = saved_scale_h;
    frontRGBoff = saved_rgb_offset;
    return 0;
}

static int test_legacy_player_selection_renderer(void)
{
    static const unsigned spec_indices[] = {
        0xe1u, 0xe2u, 0xdbu, 0xe5u, 0xdeu, 163u, 212u
    };
    static const unsigned text_indices[] = {
        224u, 225u, 304u, 332u, 333u
    };
    FONTSPEC saved_specs[
        sizeof(spec_indices) / sizeof(spec_indices[0])];
    char *saved_text[
        sizeof(text_indices) / sizeof(text_indices[0])];
    _Material *saved_materials[
        sizeof(spec_indices) / sizeof(spec_indices[0])];
    _Material materials[
        sizeof(spec_indices) / sizeof(spec_indices[0])];
    float saved_scale_mm = scaleAdjustmentMM;
    float saved_scale_x = gPSXDrawScaleX;
    float saved_scale_y = gPSXDrawScaleY;
    float saved_scale_w = gPSXDrawScaleW;
    float saved_scale_h = gPSXDrawScaleH;
    uint8_t saved_rgb_offset = frontRGBoff;
    MenuTextureDrawTrace texture_trace;
    MenuDrawTrace text_trace;
    size_t index;

    reset_menu_state();
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    memset(materials, 0, sizeof(materials));
    for (index = 0;
         index < sizeof(spec_indices) / sizeof(spec_indices[0]);
         ++index) {
        saved_specs[index] = fontSpec[spec_indices[index]];
        saved_materials[index] = menuTextures[index];
        memset(&fontSpec[spec_indices[index]], 0, sizeof(FONTSPEC));
        fontSpec[spec_indices[index]].clut = (uint16_t)index;
        fontSpec[spec_indices[index]].w = 16;
        fontSpec[spec_indices[index]].h = 16;
        menuTextures[index] = &materials[index];
    }
    for (index = 0;
         index < sizeof(text_indices) / sizeof(text_indices[0]);
         ++index) {
        saved_text[index] = allText[text_indices[index]];
    }
    allText[224] = "Level ";
    allText[225] = "Skill ";
    allText[304] = "Select Player";
    allText[332] = "Obi-Wan";
    allText[333] = "Qui-Gon";
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    gPSXDrawScaleW = 1.0f;
    gPSXDrawScaleH = 1.0f;
    frontRGBoff = 0;
    menuVars.pplayers[0] = 2;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &texture_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);

    menu_drawPlayerSelect(0, 0, 255, 128, 0);
    CHECK(menuVars.pselectMode[0].mode == 1);
    CHECK(menuVars.pselectMode[0].jedi == 163);
    CHECK(menuVars.pselectMode[0].x[0] == 0);
    CHECK(menuVars.pselectMode[0].x[1] == 291);
    CHECK(menuVars.pselectMode[0].y[0] == -48);
    CHECK(menuVars.pselectMode[0].y[1] == 0);
    CHECK(texture_trace.calls == 5);
    CHECK(texture_trace.materials[0] == &materials[0]);
    CHECK(texture_trace.destinations[0].left == 630);
    CHECK(texture_trace.destinations[0].top == 45);
    CHECK(texture_trace.materials[1] == &materials[2]);
    CHECK(texture_trace.destinations[1].left == 30);
    CHECK(texture_trace.destinations[1].top == 35);
    CHECK(texture_trace.materials[2] == &materials[3]);
    CHECK(texture_trace.destinations[2].left == 275);
    CHECK(texture_trace.destinations[2].top == 156);
    CHECK(texture_trace.materials[3] == &materials[5]);
    CHECK(texture_trace.destinations[3].left == 376);
    CHECK(texture_trace.destinations[3].top == 172);
    CHECK(texture_trace.materials[4] == &materials[4]);
    CHECK(texture_trace.destinations[4].left == 323);
    CHECK(texture_trace.destinations[4].top == 162);
    CHECK(text_trace.calls == 4);
    CHECK(utf16_matches_utf8(text_trace.text[0], "Select Player"));
    CHECK(text_trace.x[0] == 960 && text_trace.y[0] == 360);
    CHECK(text_trace.scale[0] == 2.25f);
    CHECK(utf16_matches_utf8(text_trace.text[1], "Obi-Wan"));
    CHECK(text_trace.x[1] == 312 && text_trace.y[1] == 64);
    CHECK(utf16_matches_utf8(text_trace.text[2], "Level 00"));
    CHECK(text_trace.x[2] == 432 && text_trace.y[2] == 201);
    CHECK(utf16_matches_utf8(text_trace.text[3], "Skill 000"));
    CHECK(text_trace.x[3] == 429 && text_trace.y[3] == 262);

    for (index = 0; index < 6; ++index) {
        menu_drawPlayerSelect(0, 0, 255, 128, 0);
    }
    CHECK(menuVars.pselectMode[0].mode == 2);
    CHECK(menuVars.pselectMode[0].y[0] == 0);
    for (index = 0; index < 9; ++index) {
        menu_drawPlayerSelect(0, 0, 255, 128, 0);
    }
    CHECK(menuVars.pselectMode[0].mode == 3);
    CHECK(menuVars.pselectMode[0].x[1] == 0);
    menuVars.pplayers[0] = 1;
    menu_drawPlayerSelect(0, 0, 255, 128, 0);
    CHECK(menuVars.pselectMode[0].mode == 4);
    for (index = 0; index < 9; ++index) {
        menu_drawPlayerSelect(0, 0, 255, 128, 0);
    }
    CHECK(menuVars.pselectMode[0].mode == 2);
    CHECK(menuVars.pselectMode[0].jedi == 212);
    CHECK(menuVars.pselectMode[0].x[1] == 291);
    menuVars.pselectMode[0].mode = 5;
    menuVars.pselectMode[0].y[0] = 0;
    for (index = 0; index < 6; ++index) {
        menu_drawPlayerSelect(0, 0, 255, 128, 0);
    }
    CHECK(menuVars.pselectMode[0].mode == 0);
    CHECK(menuVars.pselectMode[0].y[0] == -48);

    menuVars.pplayers[1] = 1;
    menu_drawPlayerSelect(1, 1, 255, 128, 1);
    CHECK(menuVars.pselectMode[1].mode == 1);
    CHECK(menuVars.pselectMode[1].jedi == 212);
    CHECK(menuVars.pselectMode[0].mode == 0);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    for (index = 0;
         index < sizeof(spec_indices) / sizeof(spec_indices[0]);
         ++index) {
        fontSpec[spec_indices[index]] = saved_specs[index];
        menuTextures[index] = saved_materials[index];
    }
    for (index = 0;
         index < sizeof(text_indices) / sizeof(text_indices[0]);
         ++index) {
        allText[text_indices[index]] = saved_text[index];
    }
    scaleAdjustmentMM = saved_scale_mm;
    gPSXDrawScaleX = saved_scale_x;
    gPSXDrawScaleY = saved_scale_y;
    gPSXDrawScaleW = saved_scale_w;
    gPSXDrawScaleH = saved_scale_h;
    frontRGBoff = saved_rgb_offset;
    return 0;
}

static int test_score_combo_filter(void)
{
    playerObject saved_player = gaPlayerData[0];
    int32_t saved_threshold = jpb_comboAwardHitThreshold[0];
    Combo combos[5];

    reset_menu_state();
    memset(combos, 0, sizeof(combos));
    gaPlayerData[0].playerID = 2;
    gaPlayerData[0].maxCombos = 5;
    gaPlayerData[0].paCombos = combos;
    GameStruct.ModelSelect[0] = 2;
    jpb_comboAwardHitThreshold[0] = 3;

    combos[0].prev = -1;
    combos[1].prev = -1;
    combos[1].numHits = 4;
    (void)strcpy(combos[1].String, "f");
    combos[2].prev = -1;
    combos[2].numHits = 1;
    (void)strcpy(combos[2].String, "n");
    combos[3].prev = 0;
    combos[3].numHits = 2;
    (void)strcpy(combos[3].String, "s");
    combos[4].prev = -1;
    combos[4].numHits = 1;
    (void)strcpy(combos[4].String, "w");
    game_enableCombo(2, 0);
    game_enableCombo(2, 4);
    memset(menuVars.td.newcombos, 0xff,
           sizeof(menuVars.td.newcombos));

    testcombo(0);
    CHECK(menuVars.td.jedi == 2u);
    CHECK(menuVars.td.comboListCount == 2u);
    CHECK(menuVars.td.newcombos[0] == 2u);
    CHECK(menuVars.td.newcombos[1] == 3u);
    CHECK(menuVars.td.newcombos[2] == 0u);
    CHECK(menuVars.td.newcombos[3] == 0u);
    CHECK(menuVars.td.newcombos[4] == 0u);

    gaPlayerData[0] = saved_player;
    jpb_comboAwardHitThreshold[0] = saved_threshold;
    return 0;
}

static int test_score_combo_list_generation(void)
{
    playerObject saved_player = gaPlayerData[0];
    Combo combos[4];

    reset_menu_state();
    memset(combos, 0, sizeof(combos));
    gaPlayerData[0].paCombos = combos;
    gaPlayerData[0].maxCombos = 4;
    (void)strcpy(combos[0].String, "f");
    (void)strcpy(combos[1].String, "f");
    (void)strcpy(combos[2].String, "ff");
    memset(menuVars.td.comboList, 0xa5,
           sizeof(menuVars.td.comboList));

    genComboStrings(0);
    CHECK(mp1ComboCount == 3u);
    CHECK(menuVars.td.comboList[0] == 0u);
    CHECK(menuVars.td.comboList[2] == 1u);
    CHECK(menuVars.td.comboList[4] == 2u);
    CHECK(menuVars.td.comboList[1] == 1u);
    CHECK(menuVars.td.comboList[3] == 1u);
    CHECK(menuVars.td.comboList[5] == 2u);
    CHECK(menuVars.td.comboList[6] == 0xa5u);

    gaPlayerData[0] = saved_player;
    return 0;
}

static int test_score_combo_draw(void)
{
    playerObject saved_players[2] = {
        gaPlayerData[0], gaPlayerData[1]
    };
    float saved_scale_mm = scaleAdjustmentMM;
    float saved_scale_x = gPSXDrawScaleX;
    float saved_scale_y = gPSXDrawScaleY;
    Combo combos[2];
    MenuDrawTrace trace;

    reset_menu_state();
    memset(combos, 0, sizeof(combos));
    memset(&trace, 0, sizeof(trace));
    gaPlayerData[1].paCombos = combos;
    (void)strcpy(combos[0].String, "n");
    (void)strcpy(combos[1].String, "f");
    menuVars.scoreCurrentPlayer = 1;
    menuVars.td.comboListCount = 2;
    menuVars.td.newcombos[0] = 1;
    menuVars.td.newcombos[1] = 0;
    menuVars.menuModeSP = 0;
    menuVars.mmSelect1[0] = 0;
    menuVars.mmSelect2[0] = 0;
    menuVars.mmX = 100;
    menuVars.mmY = 200;
    menuVars.textScale = 2.25f;
    menuVars.textSpacer = 60.0f;
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    OptionStruct.ControllerConfig[0] = 0;
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleX = 2.0f;
    gPSXDrawScaleY = 3.0f;
    jpb_TextSetDrawHook(capture_menu_text, &trace);

    CHECK(menu_scoreComboDraw() == 1);
    CHECK(comboTotal[0] == 0u);
    CHECK(comboTotal[1] == 2u);
    CHECK(comboType[0] == 6u);
    CHECK(comboType[1] == 0u);
    CHECK(menuVars.selectp == menuVars.mmSelect2);
    CHECK(menuVars.mmTotal == 2u);
    CHECK(menuVars.mmTextType == 0u);
    CHECK((menuVars.mmFlags & 8u) != 0);
    CHECK(trace.calls == 2);
    CHECK(utf16_matches_utf8(trace.text[0], ">   <F> <"));
    CHECK(utf16_matches_utf8(trace.text[1], "  <y>"));
    CHECK(trace.x[0] == trace.x[1]);
    CHECK(trace.y[1] - trace.y[0] == 59);
    CHECK(menuVars.mmX == 200u);
    CHECK(menuVars.mmY == 718u);

    jpb_TextSetDrawHook(NULL, NULL);
    gaPlayerData[0] = saved_players[0];
    gaPlayerData[1] = saved_players[1];
    scaleAdjustmentMM = saved_scale_mm;
    gPSXDrawScaleX = saved_scale_x;
    gPSXDrawScaleY = saved_scale_y;
    return 0;
}

static int test_score_combo_debug_draw(void)
{
    playerObject saved_player = gaPlayerData[0];
    float saved_scale_mm = scaleAdjustmentMM;
    Combo combos[3];
    MenuDrawTrace trace;

    reset_menu_state();
    memset(combos, 0, sizeof(combos));
    memset(&trace, 0, sizeof(trace));
    gaPlayerData[0].playerID = 0;
    gaPlayerData[0].playernum = 0;
    gaPlayerData[0].paCombos = combos;
    gaPlayerData[0].maxCombos = 3;
    GameStruct.ModelSelect[0] = 0;
    menuVars.jediDebugCombo = 0;
    combos[0].prev = -1;
    combos[1].prev = -1;
    combos[2].prev = 1;
    (void)strcpy(combos[0].String, "n");
    (void)strcpy(combos[1].String, "n");
    (void)strcpy(combos[2].String, "s");
    scaleAdjustmentMM = 1.0f;
    genComboStrings(0);
    jpb_TextSetDrawHook(capture_menu_text, &trace);

    game_enableCombo(0, 0);
    menu_drawCombos();
    CHECK(trace.calls == 2);
    CHECK(utf16_matches_utf8(trace.text[0], "00:01:  <Y>"));
    CHECK(utf16_matches_utf8(trace.text[1], "01:01:  <y>"));
    CHECK(trace.x[0] == 40 && trace.y[0] == 222);
    CHECK(trace.x[1] == 40 && trace.y[1] == 282);

    memset(&trace, 0, sizeof(trace));
    game_enableCombo(0, 1);
    menu_drawCombos();
    CHECK(trace.calls == 3);
    CHECK(utf16_matches_utf8(trace.text[2], "02:00:  <a>"));
    CHECK(trace.x[2] == 40 && trace.y[2] == 342);

    jpb_TextSetDrawHook(NULL, NULL);
    gaPlayerData[0] = saved_player;
    scaleAdjustmentMM = saved_scale_mm;
    return 0;
}

static int test_score_award_menu_draw(void)
{
    MPNT saved_positions[3];
    int16_t saved_cached_rewards[3];
    _Material *saved_material = menuTextures[238];
    char *saved_text[3] = {allText[359], allText[360], allText[369]};
    optionstruct saved_options = OptionStruct;
    float saved_scale_mm = scaleAdjustmentMM;
    float saved_scale_x = gPSXDrawScaleX;
    float saved_scale_y = gPSXDrawScaleY;
    uint8_t saved_score_mode = menuVars.scoreMode;
    _Material material;
    AWARDSET award_set;
    MenuTextureDrawTrace texture_trace;
    MenuDrawTrace text_trace;

    memcpy(saved_positions, menuVars.mp, sizeof(saved_positions));
    memcpy(
        saved_cached_rewards, cachedRewardsEnd,
        sizeof(saved_cached_rewards));
    memset(&material, 0, sizeof(material));
    memset(&award_set, 0, sizeof(award_set));
    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    menuTextures[238] = &material;
    allText[359] = "AWARD ONE";
    allText[360] = "AWARD ZERO";
    allText[369] = "AWARD TWO";
    award_set.awardType[0] = 0;
    award_set.awardType[1] = 1;
    award_set.awardType[2] = 2;
    cachedRewardsEnd[0] = 1;
    cachedRewardsEnd[1] = 1;
    cachedRewardsEnd[2] = 1;
    menuVars.mp[0].x = 20;
    menuVars.mp[0].y = 40;
    menuVars.mp[0].state = 1;
    menuVars.scoreMode = 3;
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);
    jpb_WHookSetDrawTextureClippedHook(
        capture_menu_texture_draw_clipped, &texture_trace);

    drawScoreMenus(0, &award_set);
    CHECK(menuVars.mp[0].x == 34);
    CHECK(menuVars.mp[0].state == 2u);
    CHECK(text_trace.calls == 3);
    CHECK(utf16_matches_utf8(text_trace.text[0], "AWARD ZERO"));
    CHECK(utf16_matches_utf8(text_trace.text[1], "AWARD ONE"));
    CHECK(utf16_matches_utf8(text_trace.text[2], "AWARD TWO"));
    CHECK(text_trace.x[0] == 150 && text_trace.y[0] == 525);
    CHECK(text_trace.x[1] == 150 && text_trace.y[1] == 410);
    CHECK(text_trace.x[2] == 150 && text_trace.y[2] == 300);
    CHECK(text_trace.depthEnabled[0] == 1);
    CHECK(text_trace.depth[0] == 0.4f);
    CHECK(text_trace.depth[1] == 0.4f - 0.001f);
    CHECK(text_trace.depth[2] == 0.4f - 2.0f * 0.001f);
    CHECK(texture_trace.calls == 3);
    CHECK(texture_trace.destinations[0].left == 126);
    CHECK(texture_trace.destinations[0].top == 495);
    CHECK(texture_trace.destinations[0].right == 973);
    CHECK(texture_trace.destinations[0].bottom == 601);
    CHECK(texture_trace.destinations[1].top == 382);
    CHECK(texture_trace.destinations[1].bottom == 488);
    CHECK(texture_trace.destinations[2].top == 270);
    CHECK(texture_trace.destinations[2].bottom == 376);
    CHECK(texture_trace.materials[0] == &material);
    CHECK(texture_trace.materials[1] == &material);
    CHECK(texture_trace.materials[2] == &material);
    CHECK(texture_trace.layers[0] == 0.5f);
    CHECK(texture_trace.layers[1] == 0.5f - 0.001f);
    CHECK(texture_trace.layers[2] == 0.5f - 2.0f * 0.001f);
    CHECK(texture_trace.colors[0].r == 225u);
    CHECK(texture_trace.colors[0].g == 225u);
    CHECK(texture_trace.colors[0].b == 225u);
    CHECK(texture_trace.colors[0].cd == 255u);
    CHECK(texture_trace.hasScissor[0] == 1);
    CHECK(texture_trace.scissors[0].left == 0);
    CHECK(texture_trace.scissors[0].top == 0);
    CHECK(texture_trace.scissors[0].right == 1920);
    CHECK(texture_trace.scissors[0].bottom == 1080);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    cachedRewardsEnd[0] = 0;
    cachedRewardsEnd[1] = 1;
    cachedRewardsEnd[2] = 0;
    menuVars.mp[0].x = 10;
    menuVars.mp[0].y = 20;
    menuVars.mp[0].state = 2;
    menuVars.scoreMode = 0;
    drawScoreMenus(0, &award_set);
    CHECK(text_trace.calls == 1);
    CHECK(utf16_matches_utf8(text_trace.text[0], "AWARD ONE"));
    CHECK(texture_trace.calls == 1);
    CHECK(texture_trace.destinations[0].left == 9);
    CHECK(texture_trace.destinations[0].top == 20);
    CHECK(texture_trace.destinations[0].right == 856);
    CHECK(texture_trace.destinations[0].bottom == 126);

    menuVars.maxAwardScore[1] =
        (UINT32_C(40) << 16) | UINT32_C(20);
    menuVars.maxAwardScore[2] = UINT32_C(1);
    memset(cachedRewardsEnd, 0, sizeof(cachedRewardsEnd));
    drawScoreMenus(4, &award_set);
    CHECK((int16_t)(menuVars.maxAwardScore[1] & 0xffffu) == 34);
    CHECK((uint16_t)(menuVars.maxAwardScore[2] & 0xffffu) == 2u);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureClippedHook(NULL, NULL);
    memcpy(menuVars.mp, saved_positions, sizeof(saved_positions));
    memcpy(
        cachedRewardsEnd, saved_cached_rewards,
        sizeof(saved_cached_rewards));
    menuTextures[238] = saved_material;
    allText[359] = saved_text[0];
    allText[360] = saved_text[1];
    allText[369] = saved_text[2];
    OptionStruct = saved_options;
    scaleAdjustmentMM = saved_scale_mm;
    gPSXDrawScaleX = saved_scale_x;
    gPSXDrawScaleY = saved_scale_y;
    menuVars.scoreMode = saved_score_mode;
    return 0;
}

static int test_score_bonus_message_draw(void)
{
    uint32_t saved_awards[2] = {
        menuVars.awards[0], menuVars.awards[1]
    };
    uint8_t saved_player = menuVars.scoreCurrentPlayer;
    uint32_t saved_stack = menuVars.menuModeSP;
    uint8_t saved_select = menuVars.mmSelect1[0];
    uint32_t saved_x = menuVars.mmX;
    uint32_t saved_y = menuVars.mmY;
    optionstruct saved_options = OptionStruct;
    int saved_input_type = lastUsedInputType;
    int saved_bonus_override = bonusOverride;
    float saved_depth = menuTextDepthOverride;
    float saved_scale_mm = scaleAdjustmentMM;
    float saved_scale_x = gPSXDrawScaleX;
    float saved_scale_y = gPSXDrawScaleY;
    char *saved_text = allText[365];
    MenuDrawTrace trace;

    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    OptionStruct.ControllerConfig[0] = 1;
    lastUsedInputType = 1;
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    menuTextDepthOverride = -1.0f;
    bonusOverride = 0;
    menuVars.scoreCurrentPlayer = 0;
    menuVars.menuModeSP = 0;
    menuVars.mmSelect1[0] = 1;
    menuVars.mmX = 10;
    menuVars.mmY = 20;
    menuVars.awards[0] = 0x1000u | 0x4000u | 0x0020u;
    allText[365] = "BONUS";
    jpb_TextSetDrawHook(capture_menu_text, &trace);

    CHECK(drawDropForceMess(0) == 3u);
    CHECK(trace.calls == 3);
    CHECK(utf16_matches_utf8(trace.text[0], "> <f>   <x> <"));
    CHECK(utf16_matches_utf8(trace.text[1], ">   <F>   <Y> <"));
    CHECK(utf16_matches_utf8(trace.text[2], "> BONUS <"));
    CHECK(trace.tint[0] == 15);
    CHECK(trace.tint[1] == 14);
    CHECK(trace.tint[2] == 15);
    CHECK(trace.x[0] == 20 && trace.y[0] == 20);
    CHECK(trace.x[1] == 30 && trace.y[1] == 80);
    CHECK(trace.x[2] == 40 && trace.y[2] == 200);
    CHECK(trace.depthEnabled[0] == 1);
    CHECK(trace.depth[0] == 0.0f);
    CHECK(menuVars.mmX == 40u);
    CHECK(menuVars.mmY == 200u);

    memset(&trace, 0, sizeof(trace));
    bonusOverride = 1;
    menuTextDepthOverride = 0.3f;
    menuVars.mmX = 10;
    menuVars.mmY = 20;
    CHECK(drawDropForceMess(0) == 3u);
    CHECK(trace.calls == 3);
    CHECK(utf16_matches_utf8(trace.text[0], "<f>   <x>"));
    CHECK(utf16_matches_utf8(trace.text[1], "<f>   <y>"));
    CHECK(trace.tint[0] == 15 && trace.tint[1] == 15);
    CHECK(trace.depth[0] == 0.3f);

    jpb_TextSetDrawHook(NULL, NULL);
    menuVars.awards[0] = saved_awards[0];
    menuVars.awards[1] = saved_awards[1];
    menuVars.scoreCurrentPlayer = saved_player;
    menuVars.menuModeSP = saved_stack;
    menuVars.mmSelect1[0] = saved_select;
    menuVars.mmX = saved_x;
    menuVars.mmY = saved_y;
    OptionStruct = saved_options;
    lastUsedInputType = saved_input_type;
    bonusOverride = saved_bonus_override;
    menuTextDepthOverride = saved_depth;
    scaleAdjustmentMM = saved_scale_mm;
    gPSXDrawScaleX = saved_scale_x;
    gPSXDrawScaleY = saved_scale_y;
    allText[365] = saved_text;
    return 0;
}

static int test_score_redline_draw(void)
{
    static const int expected[3][4] = {
        {108, 314, 1264, 808},
        {66, 450, 1222, 944},
        {25, 582, 1181, 1076}
    };
    _Material *saved_material = controlTextures[2];
    optionstruct saved_options = OptionStruct;
    float saved_scale_mm = scaleAdjustmentMM;
    _Material material;
    MenuTextureDrawTrace trace;
    unsigned index;

    memset(&material, 0, sizeof(material));
    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    scaleAdjustmentMM = 1.0f;
    controlTextures[2] = &material;
    jpb_WHookSetDrawTextureClippedHook(
        capture_menu_texture_draw_clipped, &trace);

    redlineFunc();
    CHECK(trace.calls == 3);
    for (index = 0; index < 3; ++index) {
        CHECK(trace.materials[index] == &material);
        CHECK(trace.destinations[index].left == expected[index][0]);
        CHECK(trace.destinations[index].top == expected[index][1]);
        CHECK(trace.destinations[index].right == expected[index][2]);
        CHECK(trace.destinations[index].bottom == expected[index][3]);
        CHECK(trace.colors[index].r == 0x87u);
        CHECK(trace.colors[index].g == 0x0eu);
        CHECK(trace.colors[index].b == 0x17u);
        CHECK(trace.colors[index].cd == 0xffu);
        CHECK(trace.layers[index] == 0.8f);
        CHECK(trace.hasScissor[index] == 1);
        CHECK(trace.scissors[index].left == 0);
        CHECK(trace.scissors[index].top == 0);
        CHECK(trace.scissors[index].right == 1920);
        CHECK(trace.scissors[index].bottom == 1080);
    }

    jpb_WHookSetDrawTextureClippedHook(NULL, NULL);
    controlTextures[2] = saved_material;
    OptionStruct = saved_options;
    scaleAdjustmentMM = saved_scale_mm;
    return 0;
}

static int test_score_screen_state_machine(void)
{
    playerObject saved_player = gaPlayerData[1];
    char *saved_player_one = allText[237];
    char *saved_player_two = allText[238];
    float saved_scale_mm = scaleAdjustmentMM;
    float saved_scale_w = gPSXDrawScaleW;
    float saved_scale_h = gPSXDrawScaleH;
    MenuTextureDrawTrace draw_trace;
    MenuTextureDrawTrace clipped_trace;
    MenuDrawTrace text_trace;

    reset_menu_state();
    memset(&draw_trace, 0, sizeof(draw_trace));
    memset(&clipped_trace, 0, sizeof(clipped_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleW = 1.0f;
    gPSXDrawScaleH = 1.0f;
    allText[237] = "PLAYER ONE";
    allText[238] = "PLAYER TWO";
    GameStruct.NumPlayers = 1;
    GameStruct.ModelSelect[0] = obi_wan_model;
    GameStruct.aCharacterData[0].Score = 1000;
    menuVars.scoreoLevel = 1;
    menuVars.mmv[0].state = 1;
    menuVars.awardSet[0].award[0] = 1;
    menuVars.awardSet[0].awardType[0] = 0;
    menuVars.awardSet[0].awardTotal = 1;
    menuVars.awardSet[0].pointAwarded[0] = 500;
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &draw_trace);
    jpb_WHookSetDrawTextureClippedHook(
        capture_menu_texture_draw_clipped, &clipped_trace);
    jpb_TextSetDrawHook(capture_menu_text, &text_trace);

    menu_drawScoreScreen(0);
    CHECK(menuVars.scoreMode == 9u);
    CHECK(menuVars.scoreNextMode == 10u);
    CHECK(menuVars.pointSeek == 500u);
    CHECK(menuVars.bar_y == UINT32_C(0x00b10000));
    CHECK(menuVars.bar_speed == 0u);
    CHECK(menuVars.mp[0].x == -166 && menuVars.mp[0].y == 81);
    CHECK(menuVars.mp[1].x == -166 && menuVars.mp[1].y == 282);
    CHECK(menuVars.mp[2].x == -166 && menuVars.mp[2].y == 60);
    CHECK(menuVars.mp[0].state == 0u);
    CHECK(menuVars.mp[0].maxScrolly == 64u);
    CHECK(cachedRewardsInit[0] == 0);
    CHECK(gPSXDrawScaleX == 1.0f && gPSXDrawScaleY == 1.0f);
    CHECK(draw_trace.calls >= 9);
    CHECK(draw_trace.materials[0] == menuTextures[167]);
    CHECK(draw_trace.materials[1] == menuTextures[168]);
    CHECK(draw_trace.destinations[8].right -
              draw_trace.destinations[8].left == 139);
    CHECK(draw_trace.destinations[8].bottom -
              draw_trace.destinations[8].top == 124);
    CHECK(draw_trace.layers[8] == 0.8f);
    CHECK(text_trace.calls >= 1);
    CHECK(utf16_matches_utf8(text_trace.text[0], "PLAYER ONE"));

    menu_drawScoreScreen(0);
    CHECK(menuVars.scoreMode == 9u);
    CHECK(menuVars.scoreScore == 250u);
    CHECK(menuVars.scoreBeeper == 1u);
    CHECK(cachedRewardsInit[0] == 1);
    CHECK(menuVars.bar_speed ==
          (UINT32_C(47) << 16) / UINT32_C(2));
    CHECK(menuVars.bar_y ==
          UINT32_C(0x00b10000) - menuVars.bar_speed);

    menu_drawScoreScreen(0);
    CHECK(menuVars.scoreMode == 6u);
    CHECK(menuVars.scoreScore == 500u);
    CHECK(menuVars.awardSet[0].awardCount == 1u);
    CHECK(cachedRewardsEnd[0] == 0);

    menu_drawScoreScreen(0);
    CHECK(menuVars.scoreMode == 6u);
    CHECK(menuVars.scoreScore == 750u);
    CHECK(cachedRewardsEnd[0] == 1);
    CHECK(menuVars.currentAward == 0u);
    CHECK(menuVars.mp[0].state == 1u);
    CHECK(menuVars.mp[0].x == -150);

    reset_menu_state();
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    scaleAdjustmentMM = 1.0f;
    allText[237] = "PLAYER ONE";
    allText[238] = "PLAYER TWO";
    GameStruct.NumPlayers = 2;
    GameStruct.ModelSelect[0] = obi_wan_model;
    GameStruct.ModelSelect[1] = qui_gon_model;
    gaPlayerData[1].playerID = 1;
    gaPlayerData[1].maxCombos = 0;
    gaPlayerData[1].paCombos = NULL;
    menuVars.scoreCurrentPlayer = 0;
    menuVars.scoreMode = 1;
    menuVars.awardSet[0].awardTotal = 1;
    menuVars.awardSet[0].awardCount = 1;
    menu_drawScoreScreen(0);
    CHECK(menuVars.scoreCurrentPlayer == 1u);
    CHECK(menuVars.scoreMode == 0u);
    CHECK(menuVars.td.jedi == 1u);

    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    jpb_WHookSetDrawTextureClippedHook(NULL, NULL);
    gaPlayerData[1] = saved_player;
    allText[237] = saved_player_one;
    allText[238] = saved_player_two;
    scaleAdjustmentMM = saved_scale_mm;
    gPSXDrawScaleW = saved_scale_w;
    gPSXDrawScaleH = saved_scale_h;
    return 0;
}

static int test_score_screen_main_dispatch(void)
{
    char *saved_player_one = allText[237];
    float saved_scale_mm = scaleAdjustmentMM;
    MenuTextureDrawTrace trace;

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    scaleAdjustmentMM = 1.0f;
    allText[237] = "PLAYER ONE";
    GameStruct.NumPlayers = 1;
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x27;
    menuVars.scoreoLevel = 1;
    menuVars.scoreMode = 0;
    menuVars.pad[0] = 0x20;
    menuVars.pad[1] = 0x20;
    jpb_WHookSetDrawTextureHook(capture_menu_texture_draw, &trace);

    menu_mainLoop();
    CHECK(trace.calls == 2);
    CHECK(trace.materials[0] == menuTextures[167]);
    CHECK(trace.materials[1] == menuTextures[168]);
    CHECK((menuVars.pad[0] & 0x20u) == 0);
    CHECK((menuVars.pad[1] & 0x20u) == 0);
    CHECK(menuVars.fadeupCounter == 4u);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    allText[237] = saved_player_one;
    scaleAdjustmentMM = saved_scale_mm;
    return 0;
}

static int test_score_screen_initialization(void)
{
    playerObject saved_players[2] = {
        gaPlayerData[0], gaPlayerData[1]
    };
    MemoryPool saved_pool = maMemoryBanks[2];
    WorldData *saved_world = gpWorld;
    WorldData world;
    char background_storage[32];
    unsigned player;

    reset_menu_state();
    memset(&world, 0, sizeof(world));
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    GameStruct.NumPlayers = 2;
    GameStruct.CurrentLevel = 1;
    GameStruct.ModelSelect[0] = 0;
    GameStruct.ModelSelect[1] = 53;
    for (player = 0; player < 2; ++player) {
        gaPlayerData[player].playerID = (int16_t)player;
        gaPlayerData[player].playernum = (int16_t)player;
        gaPlayerData[player].maxCombos = 0;
        gaPlayerData[player].paCombos = NULL;
        GameStruct.aCharacterData[player].Score = 24000;
    }
    world.player0 = &gaPlayerData[0];
    world.player1 = &gaPlayerData[1];
    gpWorld = &world;
    maMemoryBanks[2].pMemPool = background_storage;
    maMemoryBanks[2].memUsed = 0x1234;
    menuVars.mmv[0].mmvXvect = 77;
    menuVars.mmv[0].mmvYvect = 88;
    menuVars.mmv[0].mmvIns = 99;
    menuVars.scoreScore = 1234;
    menuVars.scoreMode = 7;
    menuVars.scoreCurrentPlayer = 1;
    menuVars.scoreBeeper = 2;

    menu_initScoreScreen();
    CHECK(menuVars.bgWidth == 1920u);
    CHECK(menuVars.bgHeight == 1080u);
    CHECK(menuVars.memBGptr == (uint8_t *)background_storage);
    CHECK(maMemoryBanks[2].memUsed == 0u);
    CHECK(menuVars.mloadShift == 0u);
    CHECK(menuVars.titleDispEnable == 1u);
    CHECK(GameStruct.continueAble == 1);
    CHECK(menuVars.mmvTriggers[0] == 1u);
    CHECK(menuVars.mmvCount == 0u);
    CHECK(menuVars.mmv[0].mmvSrc == frameBottomMover);
    CHECK(menuVars.mmv[1].mmvSrc == frameRightMover);
    CHECK(menuVars.mmv[2].mmvSrc == frameLeftMover);
    CHECK(menuVars.mmv[3].mmvSrc == frameTopMover);
    CHECK(menuVars.mmv[0].mmvPtr == 0u);
    CHECK(menuVars.mmv[0].mmvCounter == 0u);
    CHECK(menuVars.mmv[0].mmvX == 0);
    CHECK(menuVars.mmv[0].mmvY == 0);
    CHECK(menuVars.mmv[0].mmvMenu == NULL);
    CHECK(menuVars.mmv[0].state == 1u);
    CHECK(menuVars.mmv[0].mmvXvect == 77);
    CHECK(menuVars.mmv[0].mmvYvect == 88);
    CHECK(menuVars.mmv[0].mmvIns == 99u);
    CHECK(menuVars.scoreScore == 0u);
    CHECK(menuVars.scoreMode == 0u);
    CHECK(menuVars.scoreCurrentPlayer == 0u);
    CHECK(menuVars.scoreBeeper == 0u);
    CHECK(menuVars.scoreoLevel == 1u);
    for (player = 0; player < 2; ++player) {
        const AWARDSET *set = &menuVars.awardSet[player];

        CHECK(menuVars.awards[player] == UINT32_C(0x43));
        CHECK(menuVars.awardLevel[player] == 0u);
        CHECK(menuVars.awardOrder[player][0] == 0u);
        CHECK(menuVars.awardOrder[player][1] == 0u);
        CHECK(menuVars.awardOrder[player][2] == 0u);
        CHECK(set->award[0] == 0u);
        CHECK(set->award[1] == 3u);
        CHECK(set->award[2] == UINT32_C(0x7040));
        CHECK(set->awardType[0] == 0u);
        CHECK(set->awardType[1] == 1u);
        CHECK(set->awardType[2] == 2u);
        CHECK(set->awardCount == 0u);
        CHECK(set->awardTotal == 2u);
        CHECK(set->pointAwarded[0] == 0u);
        CHECK(set->pointAwarded[1] == 8000u);
        CHECK(set->pointAwarded[2] == 12000u);
        CHECK(set->maxpointaward == 24000u);
    }
    CHECK(menuVars.td.jedi == 0u);
    CHECK(menuVars.td.comboListCount == 0u);

    gaPlayerData[0] = saved_players[0];
    gaPlayerData[1] = saved_players[1];
    maMemoryBanks[2] = saved_pool;
    gpWorld = saved_world;
    return 0;
}

static int test_score_mode_entry(void)
{
    WorldData *saved_world = gpWorld;
    WorldData world;
    unsigned index;

    reset_menu_state();
    memset(&world, 0, sizeof(world));
    world.apAI = (BAP_AI **)malloc(2u * sizeof(*world.apAI));
    world.apActorNames =
        (char **)malloc(2u * sizeof(*world.apActorNames));
    world.apEnemy = (wsl_BAP_PLACEMENT **)malloc(
        2u * sizeof(*world.apEnemy));
    CHECK(world.apAI != NULL);
    CHECK(world.apActorNames != NULL);
    CHECK(world.apEnemy != NULL);
    gpWorld = &world;
    GameStruct.CurrentLevel = 4;
    GameStruct.GameState = 4;
    GameStruct.gameMode = 7;
    menuVars.pplayers[0] = 5;
    menuVars.yflag = 1;
    memset(menuVars.mmSelect1, 0xff, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 0xff, sizeof(menuVars.mmSelect2));
    memset(menuVars.frKeyBuff, 0xff, sizeof(menuVars.frKeyBuff));
    memset(menuVars.frKeyBuff2, 0xff, sizeof(menuVars.frKeyBuff2));

    menu_enterScoreMode(3);
    CHECK(world.apAI == NULL);
    CHECK(world.apActorNames == NULL);
    CHECK(world.apEnemy == NULL);
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) != 0);
    CHECK(GameStruct.inMenuFlag == 1);
    CHECK(GameStruct.gameMode == 1);
    CHECK(menuVars.pplayers[0] == 0u);
    CHECK(menuVars.menuModeSP == 2u);
    CHECK(menuVars.menuMode[1] == 0x66u);
    CHECK(menuVars.menuMode[2] == 0x27u);
    CHECK(menuVars.yflag == 0u);
    CHECK(GameStruct.continueAble == 1);
    for (index = 0; index < 8; ++index) {
        CHECK(menuVars.mmSelect1[index] == 0u);
        CHECK(menuVars.mmSelect2[index] == 0u);
    }
    for (index = 0; index < 16; ++index) {
        CHECK(menuVars.frKeyBuff[index] == 0u);
        CHECK(menuVars.frKeyBuff2[index] == 0u);
    }

    gpWorld = saved_world;
    return 0;
}

static int test_big_score_draw(void)
{
    FONTSPEC saved_specs[10];
    _Material *saved_materials[10];
    _Material materials[10];
    MenuTextureDrawTrace trace;
    float saved_scale_mm = scaleAdjustmentMM;
    float saved_scale_x = gPSXDrawScaleX;
    float saved_scale_y = gPSXDrawScaleY;
    float saved_scale_w = gPSXDrawScaleW;
    float saved_scale_h = gPSXDrawScaleH;
    float saved_front_z = frontZ;
    uint8_t saved_rgb_offset = frontRGBoff;
    static const int expected_digits[6] = {0, 0, 0, 0, 4, 2};
    static const int expected_x[6] = {100, 110, 120, 130, 140, 154};
    unsigned digit;
    unsigned index;

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    memset(materials, 0, sizeof(materials));
    for (digit = 0; digit < 10; ++digit) {
        unsigned texture = 0xe8u + digit;
        unsigned material = 220u + digit;

        saved_specs[digit] = fontSpec[texture];
        saved_materials[digit] = menuTextures[material];
        fontSpec[texture] = (FONTSPEC){
            0, (uint16_t)material, 2, (uint16_t)digit,
            20, (uint16_t)(10u + digit)
        };
        menuTextures[material] = &materials[digit];
    }
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    scaleAdjustmentMM = 1.0f;
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
    gPSXDrawScaleW = 1.0f;
    gPSXDrawScaleH = 1.0f;
    frontZ = 0.0f;
    frontRGBoff = 0;
    jpb_WHookSetDrawTextureHook(capture_menu_texture_draw, &trace);

    menu_drawBigScore(42u, 100u, 200u);
    CHECK(trace.calls == 6);
    for (index = 0; index < 6; ++index) {
        int digit_value = expected_digits[index];

        CHECK(trace.materials[index] == &materials[digit_value]);
        CHECK(trace.destinations[index].left == expected_x[index]);
        CHECK(trace.destinations[index].top == 200);
        CHECK(trace.destinations[index].right ==
              expected_x[index] + 10 + digit_value);
        CHECK(trace.destinations[index].bottom == 220);
        CHECK(trace.colors[index].r == 0x60u);
        CHECK(trace.colors[index].g == 0x60u);
        CHECK(trace.colors[index].b == 0x60u);
        CHECK(trace.colors[index].cd == 0xffu);
    }

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    for (digit = 0; digit < 10; ++digit) {
        fontSpec[0xe8u + digit] = saved_specs[digit];
        menuTextures[220u + digit] = saved_materials[digit];
    }
    scaleAdjustmentMM = saved_scale_mm;
    gPSXDrawScaleX = saved_scale_x;
    gPSXDrawScaleY = saved_scale_y;
    gPSXDrawScaleW = saved_scale_w;
    gPSXDrawScaleH = saved_scale_h;
    frontZ = saved_front_z;
    frontRGBoff = saved_rgb_offset;
    return 0;
}

static int test_score_mover_owner(void)
{
    uint8_t wait_stream[] = {0};
    uint8_t *score_trigger_bytes;

    reset_menu_state();
    menuVars.mmvCount = 0;
    menuVars.mmv[0].mmvCounter = 2;
    menuVars.mmv[0].mmvIns = 0x26;
    menuVars.mmv[0].mmvY = INT32_C(0x30000);
    menuVars.mmv[1].mmvCounter = 2;
    menuVars.mmv[1].mmvIns = 0x29;
    menuVars.mmv[1].mmvX = INT32_C(0x40000);
    menuVars.mmv[2].mmvCounter = 2;
    menuVars.mmv[2].mmvIns = 0x2a;
    menuVars.mmv[2].mmvX = 10;
    menuVars.mmv[2].mmvY = 20;
    menuVars.mmv[2].mmvXvect = 3;
    menuVars.mmv[2].mmvYvect = 4;
    menuVars.mmv[3].mmvCounter = 1;
    menuVars.mmv[3].mmvIns = 0x36;
    menuVars.mmv[3].mmvSrc = wait_stream;
    menuVars.mmv[3].mmvPtr = 1;
    score_trigger_bytes =
        (uint8_t *)(void *)&menuVars + 0x231;
    score_trigger_bytes[0] = 4;

    menu_drawScoreMovers();
    CHECK(menuVars.mmvCount == 0u);
    CHECK(menuVars.mmv[0].mmvY == INT32_C(0x20000));
    CHECK(menuVars.mmv[0].mmvCounter == 1u);
    CHECK(menuVars.mmv[1].mmvX == INT32_C(0x50000));
    CHECK(menuVars.mmv[1].mmvCounter == 1u);
    CHECK(menuVars.mmv[2].mmvX == 13);
    CHECK(menuVars.mmv[2].mmvY == 24);
    CHECK(menuVars.mmv[2].mmvCounter == 1u);
    CHECK(menuVars.mmv[3].mmvCounter == 7u);
    CHECK(menuVars.mmvCurrentMenuControl == &menuVars.mmv[3]);
    return 0;
}

static int test_character_selection_foundations(void)
{
    CVECTOR saved_colours[JPB_JEDI_COLOUR_STORAGE_COUNT];
    int saved_sprites[JPB_JEDI_COLOUR_STORAGE_COUNT];
    int saved_unlocks[JPB_EXTRA_CHARACTER_COUNT];
    ExtraCharacter *loader;
    size_t index;

    reset_menu_state();
    memcpy(saved_colours, gJediColourCurrent, sizeof(saved_colours));
    memcpy(saved_sprites, gJediColorSpriteCurrent, sizeof(saved_sprites));
    for (index = 0; index < JPB_EXTRA_CHARACTER_COUNT; ++index) {
        saved_unlocks[index] = ExtraCharacters[index].Unlocked;
    }

    secretBits = 0;
    CHECK(jedi_CheckValidPlayer(obi_wan_model) == 1);
    CHECK(jedi_CheckValidPlayer(plo_model) == 1);
    CHECK(jedi_CheckValidPlayer(maul_p_model) == 0);
    CHECK(jedi_CheckValidPlayer(battle_d_model) == 0);
    CHECK(jedi_CheckValidPlayer(maul_model) == 0);
    secretBits = UINT32_C(0x670);
    CHECK(jedi_CheckValidPlayer(maul_p_model) == 1);
    CHECK(jedi_CheckValidPlayer(amidala_model) == 1);
    CHECK(jedi_CheckValidPlayer(panaka_model) == 1);
    CHECK(jedi_CheckValidPlayer(ki_adi_model) == 1);
    CHECK(jedi_CheckValidPlayer(battle_d_model) == 1);

    loader = GetCharacterByID(loader_model);
    CHECK(loader != NULL);
    loader->Unlocked = 0;
    CHECK(jedi_CheckValidPlayerNGP(pilot_model) == 1);
    CHECK(jedi_CheckValidPlayerNGP(battle_d_model) == 0);
    CHECK(jedi_CheckValidPlayerNGP(loader_model) == 0);
    CHECK(jedi_CheckValidPlayerNGP(jar_jar_playable_model) == 1);
    loader->Unlocked = 1;
    CHECK(jedi_CheckValidPlayerNGP(loader_model) == 1);
    CHECK(jedi_CheckValidPlayerWTabs(1, pilot_model) == 1);
    CHECK(jedi_CheckValidPlayerWTabs(1, battle_d_model) == 0);
    CHECK(jedi_CheckValidPlayerWTabs(1, loader_model) == 1);
    CHECK(jedi_CheckValidPlayerWTabs(0, battle_d_model) == 1);

    memset(GameStruct.jediLevelPlayed, 0,
           sizeof(GameStruct.jediLevelPlayed));
    CHECK(jedi_CheckValidVersus(obi_wan_model) == 1);
    CHECK(jedi_CheckValidVersus(gungan_1_model) == 1);
    CHECK(jedi_CheckValidVersus(peck_model) == 0);

    CHECK(gJediColourArrayLength == JPB_JEDI_COLOUR_COUNT);
    CHECK(memcmp(gJediColourCurrent, gJediColourCanon,
                 sizeof(gJediColourCurrent)) == 0);
    CHECK((jedi_GetColour32(ki_adi_model) & UINT32_C(0x00ffffff)) ==
          UINT32_C(0x0045a6ff));
    CHECK(gJediColourCurrent[JPB_JEDI_COLOUR_COUNT].r == 0);
    CHECK(jedi_CanToggleSaber(mace_model) == 1);
    CHECK(jedi_CanToggleSaber(obi_wan_model) == 0);
    CHECK(jedi_GetColorSprite(mace_model) ==
          gJediColorSpriteCanon[mace_model]);
    jedi_ToggleSaberColor(mace_model);
    CHECK(memcmp(&gJediColourCurrent[mace_model],
                 &gJediColourLegacy[mace_model], sizeof(CVECTOR)) == 0);
    CHECK(gJediColorSpriteCurrent[mace_model] ==
          gJediColorSpriteLegacy[mace_model]);
    CHECK(jedi_GetColorSprite(mace_model) ==
          gJediColorSpriteLegacy[mace_model]);
    jedi_ToggleSaberColor(mace_model);
    CHECK(memcmp(&gJediColourCurrent[mace_model],
                 &gJediColourCanon[mace_model], sizeof(CVECTOR)) == 0);
    CHECK(gJediColorSpriteCurrent[mace_model] ==
          gJediColorSpriteCanon[mace_model]);
    CHECK(jedi_GetColorSprite(mace_model) ==
          gJediColorSpriteCanon[mace_model]);

    memset(menuVars.pselectMode, 0xa5, sizeof(menuVars.pselectMode));
    menuVars.scoreDst = UINT32_C(0x12345678);
    menuVars.holdButtFlag = 1;
    menuVars.pSelect = 3;
    newMenu_playerSelectTypeP1 = 1;
    newMenu_currentModelSelectBaseP1 = qui_gon_model;
    newMenu_currentModelSelectNGPP1 = thug_1_model;
    newMenu_playerSelectTypeP2 = 1;
    newMenu_currentModelSelectBaseP2 = mace_model;
    newMenu_currentModelSelectNGPP2 = thug_2_model;
    GameStruct.ModelSelect[0] = qui_gon_model;
    GameStruct.ModelSelect[1] = mace_model;
    menu_initPlayerSelect();
    CHECK(menuVars.scoreDst == 0);
    CHECK(menuVars.pselectMode[0].mode == 0);
    CHECK(((uint8_t *)&menuVars.pselectMode[0])[2] == 0xa5);
    CHECK(menuVars.pselectMode[1].mode == 0);
    CHECK(((uint8_t *)&menuVars.pselectMode[1])[2] == 0xa5);
    CHECK(menuVars.holdButtFlag == 0 && menuVars.pSelect == 0);
    CHECK(menuVars.pplayers[0] == 0 && menuVars.pplayers[1] == 3);
    CHECK(menuVars.subplayers[0] == 0 && menuVars.subplayers[1] == 3);
    CHECK(newMenu_playerSelectTypeP1 == 0);
    CHECK(newMenu_currentModelSelectBaseP1 == obi_wan_model);
    CHECK(newMenu_currentModelSelectNGPP1 == pilot_model);
    CHECK(newMenu_playerSelectTypeP2 == 0);
    CHECK(newMenu_currentModelSelectBaseP2 == qui_gon_model);
    CHECK(newMenu_currentModelSelectNGPP2 == rifle_model);
    CHECK(GameStruct.ModelSelect[0] == obi_wan_model);
    CHECK(GameStruct.ModelSelect[1] == qui_gon_model);

    GetCharacterByID(pilot_model)->Unlocked = 0;
    GetCharacterByID(battle_d_model)->Unlocked = 1;
    menuVars.subplayers[0] = 5;
    menuVars.pplayers[0] = 6;
    updatePlayerSelectIndex(0);
    CHECK(menuVars.pplayers[0] == 7);
    GetCharacterByID(jar_jar_playable_model)->Unlocked = 0;
    menuVars.subplayers[1] = 18;
    menuVars.pplayers[1] = 19;
    updatePlayerSelectIndex(1);
    CHECK(menuVars.pplayers[1] == 20);

    memcpy(gJediColourCurrent, saved_colours, sizeof(saved_colours));
    memcpy(gJediColorSpriteCurrent, saved_sprites, sizeof(saved_sprites));
    for (index = 0; index < JPB_EXTRA_CHARACTER_COUNT; ++index) {
        ExtraCharacters[index].Unlocked = saved_unlocks[index];
    }
    return 0;
}

static int test_p1_character_selection_state(void)
{
    CharacterDrawTrace draw_trace;
    PlatformTrace platform_trace;
    JPBMenuPlatformHooks hooks;
    MenuInputTrace input_trace;
    int index;

    reset_menu_state();
    memset(&draw_trace, 0, sizeof(draw_trace));
    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&hooks, 0, sizeof(hooks));
    hooks.soundCue = trace_sound_cue;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    jpb_MenuSetP1CharacterSelectDrawHook(
        capture_p1_character_select, &draw_trace);
    GameStruct.NumPlayers = 1;
    newMenu_currentModelSelectBaseP1 = obi_wan_model;

    menuVars.pad[0] = 0;
    CHECK(newMenu_P1CharacterSelect() == 0);
    CHECK(newMenu_state == 1);
    CHECK(newMenu_errorState == 0x10);
    CHECK(newMenu_P1CharacterSelect() == 0);
    CHECK(newMenu_state == 0x18);
    CHECK(GameStruct.ModelSelect[0] == obi_wan_model);

    menuVars.pad[0] = JPB_PAD_LEFT;
    CHECK(newMenu_P1CharacterSelect() == 0);
    CHECK(GameStruct.ModelSelect[0] == qui_gon_model);
    CHECK(strcmp(platform_trace.lastCue, "xjedscrl") == 0);
    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    CHECK(newMenu_P1CharacterSelect() == 0);
    CHECK(newMenu_state == 0x0e);
    CHECK(strcmp(platform_trace.lastCue, "xjedsel") == 0);
    menuVars.pad[0] = 0;
    CHECK(newMenu_P1CharacterSelect() == 1);
    CHECK(newMenu_state == 0);
    CHECK(newMenu_currentModelSelectBaseP1 == qui_gon_model);
    CHECK(draw_trace.exitPhase[draw_trace.calls - 1] == 1);

    newMenu_state = 0;
    newMenu_currentModelSelectBaseP1 = mace_model;
    menuVars.pad[0] = 0;
    CHECK(newMenu_P1CharacterSelect() == 0);
    CHECK(newMenu_P1CharacterSelect() == 0);
    menuVars.pad[0] = JPB_PAD_JUMP;
    CHECK(newMenu_P1CharacterSelect() == 0);
    menuVars.pad[0] = 0;
    CHECK(newMenu_P1CharacterSelect() == -1);
    CHECK(GameStruct.NumPlayers == 1);

    reset_menu_state();
    memset(&draw_trace, 0, sizeof(draw_trace));
    memset(&input_trace, 0, sizeof(input_trace));
    jpb_MenuSetP1CharacterSelectDrawHook(
        capture_p1_character_select, &draw_trace);
    jpb_InputSetProvider(read_menu_pad, &input_trace);
    GameStruct.NumPlayers = 1;
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x0e;
    menu_mainLoop();
    menu_mainLoop();
    input_trace.pads[0] = JPB_PAD_COMBO_SOUTH;
    menu_mainLoop();
    input_trace.pads[0] = 0;
    menu_mainLoop();
    CHECK(GameStruct.CurrentLevel == 9);
    CHECK(menuVars.menuModeSP == 3);
    CHECK(menuVars.menuMode[3] == 0x1a);

    reset_menu_state();
    memset(&input_trace, 0, sizeof(input_trace));
    jpb_InputSetProvider(read_menu_pad, &input_trace);
    GameStruct.NumPlayers = 1;
    newMenu_currentModelSelectBaseP1 = obi_wan_model;
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x0e;
    menu_mainLoop();
    menu_mainLoop();
    input_trace.pads[0] = JPB_PAD_LEFT;
    menu_mainLoop();
    CHECK(GameStruct.ModelSelect[0] == qui_gon_model);
    for (index = 0; index < 19; ++index) {
        menu_mainLoop();
    }
    CHECK(GameStruct.ModelSelect[0] == qui_gon_model);

    jpb_InputSetProvider(NULL, NULL);
    jpb_MenuSetP1CharacterSelectDrawHook(NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_p2_character_selection_state(void)
{
    P2CharacterDrawTrace draw_trace;
    PlatformTrace platform_trace;
    JPBMenuPlatformHooks hooks;
    MenuInputTrace input_trace;

    reset_menu_state();
    memset(&draw_trace, 0, sizeof(draw_trace));
    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&hooks, 0, sizeof(hooks));
    hooks.soundCue = trace_sound_cue;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    jpb_MenuSetP2CharacterSelectDrawHook(
        capture_p2_character_select, &draw_trace);
    GameStruct.NumPlayers = 2;
    newMenu_currentModelSelectBaseP1 = obi_wan_model;
    newMenu_currentModelSelectBaseP2 = qui_gon_model;

    CHECK(newMenu_P2CharacterSelect(0) == 0);
    CHECK(newMenu_state == 1 && newMenu_select == 0);
    CHECK(newMenu_P2CharacterSelect(0) == 0);
    CHECK(newMenu_state == 0x18);
    CHECK(GameStruct.ModelSelect[0] == obi_wan_model);
    CHECK(GameStruct.ModelSelect[1] == qui_gon_model);

    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    menuVars.pad[1] = 0;
    CHECK(newMenu_P2CharacterSelect(0) == 0);
    CHECK(newMenu_select == UINT32_C(1));
    menuVars.pad[0] = 0;
    menuVars.pad[1] = JPB_PAD_LEFT;
    CHECK(newMenu_P2CharacterSelect(0) == 0);
    CHECK(GameStruct.ModelSelect[1] == mace_model);
    menuVars.pad[1] = JPB_PAD_COMBO_SOUTH;
    CHECK(newMenu_P2CharacterSelect(0) == 0);
    CHECK(newMenu_select == UINT32_C(3));
    CHECK(newMenu_state == 0x0e);
    menuVars.pad[1] = 0;
    CHECK(newMenu_P2CharacterSelect(0) == 1);
    CHECK(newMenu_state == 0);
    CHECK(newMenu_currentModelSelectBaseP1 == obi_wan_model);
    CHECK(newMenu_currentModelSelectBaseP2 == mace_model);
    CHECK(draw_trace.isVersus[draw_trace.calls - 1] == 0);

    newMenu_state = 0;
    GameStruct.NumPlayers = 2;
    menuVars.pad[0] = 0;
    menuVars.pad[1] = 0;
    CHECK(newMenu_P2CharacterSelect(0) == 0);
    CHECK(newMenu_P2CharacterSelect(0) == 0);
    menuVars.pad[1] = JPB_PAD_JUMP;
    CHECK(newMenu_P2CharacterSelect(0) == 0);
    CHECK(newMenu_state == 0x0e && newMenu_bAbortMenu == 1);
    menuVars.pad[1] = 0;
    CHECK(newMenu_P2CharacterSelect(0) == -1);
    CHECK(GameStruct.NumPlayers == 1);

    newMenu_playerSelectTypeP1 = 1;
    newMenu_currentModelSelectBaseP1 = adi_model;
    newMenu_currentModelSelectNGPP1 = thug_1_model;
    newMenu_playerSelectTypeP2 = 1;
    newMenu_currentModelSelectBaseP2 = plo_model;
    newMenu_currentModelSelectNGPP2 = thug_2_model;
    newMenu_state = 0x0e;
    newMenu_bAbortMenu = 0;
    GameStruct.NumPlayers = 2;
    CHECK(newMenu_P2CharacterSelect(1) == 1);
    CHECK(newMenu_playerSelectTypeP1 == 0);
    CHECK(newMenu_currentModelSelectBaseP1 == obi_wan_model);
    CHECK(newMenu_currentModelSelectNGPP1 == pilot_model);
    CHECK(newMenu_playerSelectTypeP2 == 0);
    CHECK(newMenu_currentModelSelectBaseP2 == qui_gon_model);
    CHECK(newMenu_currentModelSelectNGPP2 == rifle_model);
    CHECK(draw_trace.isVersus[draw_trace.calls - 1] == 1);

    reset_menu_state();
    memset(&draw_trace, 0, sizeof(draw_trace));
    memset(&input_trace, 0, sizeof(input_trace));
    jpb_MenuSetP2CharacterSelectDrawHook(
        capture_p2_character_select, &draw_trace);
    jpb_InputSetProvider(read_menu_pad, &input_trace);
    GameStruct.NumPlayers = 2;
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x0e;
    menu_mainLoop();
    menu_mainLoop();
    input_trace.pads[0] = JPB_PAD_COMBO_SOUTH;
    menu_mainLoop();
    input_trace.pads[0] = 0;
    input_trace.pads[1] = JPB_PAD_COMBO_SOUTH;
    menu_mainLoop();
    input_trace.pads[1] = 0;
    menu_mainLoop();
    CHECK(GameStruct.CurrentLevel == 9);
    CHECK(menuVars.menuModeSP == 3);
    CHECK(menuVars.menuMode[3] == 0x1a);
    CHECK(draw_trace.calls >= 5);

    reset_menu_state();
    memset(&draw_trace, 0, sizeof(draw_trace));
    memset(&input_trace, 0, sizeof(input_trace));
    jpb_MenuSetP2CharacterSelectDrawHook(
        capture_p2_character_select, &draw_trace);
    jpb_InputSetProvider(read_menu_pad, &input_trace);
    GameStruct.NumPlayers = 1;
    menuVars.menuMode[0] = 0x0d;
    menu_mainLoop();
    menu_mainLoop();
    input_trace.pads[0] = JPB_PAD_COMBO_SOUTH;
    menu_mainLoop();
    input_trace.pads[0] = 0;
    menu_mainLoop();
    CHECK(LevelSelect == 0x19);
    CHECK(GameStruct.NumPlayers == 2);
    CHECK(GameStruct.versusModeFlag == 1);
    CHECK(GameStruct.gameMode == 2);
    CHECK(draw_trace.isVersus[draw_trace.calls - 1] == 1);

    jpb_InputSetProvider(NULL, NULL);
    jpb_MenuSetP2CharacterSelectDrawHook(NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_vs_mode_state(void)
{
    static const unsigned text_indices[] = {
        237u, 238u, 239u, 241u, 332u, 333u, 334u, 475u, 476u
    };
    static const unsigned font_indices[] = {
        0x197u, 0x198u, 367u, 382u, 383u, 384u
    };
    char *saved_text[sizeof(text_indices) / sizeof(text_indices[0])];
    FONTSPEC saved_font[sizeof(font_indices) / sizeof(font_indices[0])];
    _Material *saved_material = menuTextures[0];
    float saved_scale_mm = scaleAdjustmentMM;
    JPBMenuPlatformHooks hooks;
    PlatformTrace platform_trace;
    MenuTextureDrawTrace draw_trace;
    _Material material;
    size_t index;

    reset_menu_state();
    memset(&hooks, 0, sizeof(hooks));
    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&draw_trace, 0, sizeof(draw_trace));
    memset(&material, 0, sizeof(material));
    material.iw = 100;
    material.ih = 120;
    menuTextures[0] = &material;
    for (index = 0; index < sizeof(font_indices) / sizeof(font_indices[0]);
         ++index) {
        saved_font[index] = fontSpec[font_indices[index]];
        memset(&fontSpec[font_indices[index]], 0, sizeof(FONTSPEC));
        fontSpec[font_indices[index]].w = 50;
        fontSpec[font_indices[index]].h = 50;
    }
    for (index = 0; index < sizeof(text_indices) / sizeof(text_indices[0]);
         ++index) {
        saved_text[index] = allText[text_indices[index]];
        allText[text_indices[index]] = "VS";
    }
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    scaleAdjustmentMM = 1.0f;
    hooks.soundCue = trace_sound_cue;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    jpb_WHookSetDrawTextureHook(
        capture_menu_texture_draw, &draw_trace);
    GameStruct.NumPlayers = 2;
    GameStruct.ModelSelect[0] = obi_wan_model;
    GameStruct.ModelSelect[1] = mace_model;
    tempPlayersVs = 2;

    CHECK(newMenu_VSMode() == 0);
    CHECK(newMenu_state == 1 && newMenu_select == 0);
    CHECK(newMenu_VSMode() == 0);
    CHECK(newMenu_state == 0x18);

    menuVars.pad[0] = JPB_PAD_LEFT;
    CHECK(newMenu_VSMode() == 0);
    CHECK(GameStruct.ModelSelect[0] == qui_gon_model);
    CHECK(strcmp(platform_trace.lastCue, "xjedscrl") == 0);
    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    CHECK(newMenu_VSMode() == 0);
    CHECK(newMenu_select == UINT32_C(1));
    menuVars.pad[0] = 0;
    menuVars.pad[1] = JPB_PAD_COMBO_SOUTH;
    CHECK(newMenu_VSMode() == 0);
    CHECK(newMenu_select == UINT32_C(3));
    CHECK(newMenu_state == 0x0e);
    menuVars.pad[1] = 0;
    CHECK(newMenu_VSMode() == 1);
    CHECK(newMenu_state == 0);
    CHECK(GameStruct.NumPlayers == 2);
    CHECK(draw_trace.calls != 0);

    newMenu_state = 0;
    newMenu_bAbortMenu = 0;
    menuVars.pad[0] = 0;
    CHECK(newMenu_VSMode() == 0);
    CHECK(newMenu_VSMode() == 0);
    menuVars.pad[0] = JPB_PAD_JUMP;
    CHECK(newMenu_VSMode() == 0);
    CHECK(newMenu_state == 0x0e && newMenu_bAbortMenu == 1);
    menuVars.pad[0] = 0;
    CHECK(newMenu_VSMode() == -1);
    CHECK(GameStruct.NumPlayers == 1);

    newMenu_state = 0;
    newMenu_bAbortMenu = 0;
    newMenu_select = 0;
    GameStruct.NumPlayers = 2;
    GameStruct.ModelSelect[0] = obi_wan_model;
    GameStruct.ModelSelect[1] = mace_model;
    menuVars.pad[0] = 0;
    menuVars.pad[1] = 0;
    CHECK(newMenu_PlayerSelect() == 0);
    CHECK(newMenu_state == 1 && newMenu_select == 0);
    CHECK(newMenu_PlayerSelect() == 0);
    CHECK(newMenu_state == 0x18);
    menuVars.pad[0] = JPB_PAD_LEFT;
    CHECK(newMenu_PlayerSelect() == 0);
    CHECK(GameStruct.ModelSelect[0] == qui_gon_model);
    CHECK(strcmp(platform_trace.lastCue, "xjedscrl") == 0);
    menuVars.pad[0] = JPB_PAD_COMBO_SOUTH;
    CHECK(newMenu_PlayerSelect() == 0);
    CHECK(newMenu_select == UINT32_C(1));
    menuVars.pad[0] = 0;
    menuVars.pad[1] = JPB_PAD_COMBO_SOUTH;
    CHECK(newMenu_PlayerSelect() == 0);
    CHECK(newMenu_select == UINT32_C(3));
    CHECK(newMenu_state == 0x0e);
    menuVars.pad[1] = 0;
    CHECK(newMenu_PlayerSelect() == 1);
    CHECK(newMenu_state == 0);
    CHECK(GameStruct.NumPlayers == 2);

    newMenu_state = 0;
    newMenu_bAbortMenu = 0;
    menuVars.pad[0] = 0;
    CHECK(newMenu_PlayerSelect() == 0);
    CHECK(newMenu_PlayerSelect() == 0);
    menuVars.pad[0] = JPB_PAD_JUMP;
    CHECK(newMenu_PlayerSelect() == 0);
    CHECK(newMenu_state == 0x0e && newMenu_bAbortMenu == 1);
    menuVars.pad[0] = 0;
    CHECK(newMenu_PlayerSelect() == -1);
    CHECK(GameStruct.NumPlayers == 1);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    menuTextures[0] = saved_material;
    scaleAdjustmentMM = saved_scale_mm;
    for (index = 0; index < sizeof(font_indices) / sizeof(font_indices[0]);
         ++index) {
        fontSpec[font_indices[index]] = saved_font[index];
    }
    for (index = 0; index < sizeof(text_indices) / sizeof(text_indices[0]);
         ++index) {
        allText[text_indices[index]] = saved_text[index];
    }
    return 0;
}

static int test_mod_value_accessors(void)
{
    MDEF_MOD mod;
    uint32_t md[5] = {9, 0, 0, 0, 0};
    JPBMenuPlatformHooks hooks;
    PlatformTrace platform_trace;
    MenuAudioControlTrace audio_trace;
    uint8_t byte_value = UINT8_C(0x7a);
    uint16_t word_value = UINT16_C(0x1234);
    uint32_t dword_value = UINT32_C(0x89abcdef);

    reset_menu_state();
    memset(&mod, 0, sizeof(mod));
    memset(&hooks, 0, sizeof(hooks));
    memset(&platform_trace, 0, sizeof(platform_trace));
    memset(&audio_trace, 0, sizeof(audio_trace));
    hooks.soundCue = trace_sound_cue;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    jpb_AudioStreamSetControlHook(
        capture_menu_audio_control, &audio_trace);
    jpb_AudioStreamSetPlayHook(
        capture_menu_audio_play, &audio_trace);
    mod.src = &byte_value;
    CHECK(mmGetModVal(&mod) == UINT8_C(0x7a));
    CHECK(mmSetModVal(&mod, UINT32_C(0x1ab), 0) == 1);
    CHECK(byte_value == UINT8_C(0xab));

    mod.type = 3;
    mod.src = &word_value;
    CHECK(mmGetModVal(&mod) == UINT16_C(0x1234));
    CHECK(mmSetModVal(&mod, UINT32_C(0x12345), 0) == 1);
    CHECK(word_value == UINT16_C(0x2345));

    mod.type = 6;
    mod.src = &dword_value;
    CHECK(mmGetModVal(&mod) == UINT32_C(0x89abcdef));
    CHECK(mmSetModVal(&mod, UINT32_C(0x76543210), 0) == 1);
    CHECK(dword_value == UINT32_C(0x76543210));

    mod.type = 0;
    mod.src = &byte_value;
    menuVars.pSelect = 1;
    CHECK(mmSetModVal(&mod, 3, 2) == 0);
    CHECK(byte_value == UINT8_C(0xab));
    CHECK(mmSetModVal(&mod, 3, 3) == 1);
    CHECK(byte_value == 3);
    menuVars.pSelect = 2;
    CHECK(mmSetModVal(&mod, 4, 3) == 0);
    CHECK(byte_value == 3);
    CHECK(mmSetModVal(&mod, 4, 2) == 1);
    CHECK(byte_value == 4);

    mod.type = UINT16_C(0x807f);
    mod.src = NULL;
    CHECK(mmGetModVal(&mod) == 0);
    CHECK(mmSetModVal(&mod, 1, 0) == 1);

    CHECK(sizeof(modVars) / sizeof(modVars[0]) == 74);
    CHECK(modVars[1].src == &GameStruct.NumPlayers);
    CHECK(modVars[1].min == 1 && modVars[1].max == 2);
    CHECK(modVars[22].type == UINT16_C(0x3801));
    CHECK(modVars[22].incspeed == 3);
    CHECK(modVars[22].max == 75);
    CHECK(modVars[22].src == &OptionStruct.musicVolume);
    CHECK(modVars[31].src == &GameStruct.AIDamage);
    CHECK(modVars[61].src == &GameStruct.Counter);
    CHECK(modVars[71].src == &OptionStruct.Language);
    CHECK(modVars[73].type == 6);
    CHECK(modVars[73].src == &OptionStruct.ResolutionChanged);

    md[4] = 20;
    OptionStruct.Music = 1;
    mmIncVar(md);
    CHECK(OptionStruct.Music == 0);
    mmDecVar(md);
    CHECK(OptionStruct.Music == 1);

    md[4] = 22;
    OptionStruct.musicVolume = 72;
    mmIncVar(md);
    CHECK(OptionStruct.musicVolume == 75);
    mmIncVar(md);
    CHECK(OptionStruct.musicVolume == 75);
    OptionStruct.musicVolume = 0;
    mmDecVar(md);
    CHECK(OptionStruct.musicVolume == 0);

    md[4] = 1;
    GameStruct.NumPlayers = 1;
    mmIncVar(md);
    CHECK(GameStruct.NumPlayers == 2);
    CHECK(game_GET_GLOBALBIT(2) == 1);
    mmIncVar(md);
    CHECK(GameStruct.NumPlayers == 1);
    CHECK(game_GET_GLOBALBIT(2) == 0);

    OptionStruct.WalkLimit[0] = 8;
    OptionStruct.RunLimit[0] = 2;
    md[4] = 16;
    mmUpdateModSet(md, 8, 0);
    CHECK(OptionStruct.WalkLimit[0] == 2);

    menuVars.pplayers[0] = 1;
    menuVars.subplayers[0] = 0;
    md[4] = 2;
    mmUpdateModSet(md, 1, 0);
    CHECK(menuVars.pplayers[0] == 1);
    CHECK(menuVars.subplayers[0] == 1);
    CHECK(strcmp(platform_trace.lastCue, "xjedscrl") == 0);

    GameStruct.NumPlayers = 1;
    secretBits = 0;
    LevelSelect = 11;
    md[4] = 8;
    mmUpdateModSet(md, 11, 0);
    CHECK(LevelSelect == 1);
    CHECK(strcmp(platform_trace.lastCue, "xlvbrows") == 0);
    secretBits = UINT32_C(0x0f);
    GameStruct.NumPlayers = 2;
    mmUpdateModSet(md, 14, 1);
    CHECK(LevelSelect == 13);

    OptionStruct.Music = 1;
    OptionStruct.musicVolume = 37;
    GameStruct.xaVol = 0;
    GameStruct.gameMode = 0;
    GameStruct.CurrentLevel = 0;
    GameStruct.GameState = 0;
    md[4] = 21;
    mmUpdateModSet(md, 0, 0);
    CHECK(GameStruct.xaVol == 74);
    CHECK(strcmp(platform_trace.lastCue, "xopt_sel") == 0);
    CHECK(audio_trace.calls == 2);
    CHECK(audio_trace.lastControl == JPB_AUDIO_STREAM_STOP);
    CHECK(audio_trace.playCalls == 1);
    CHECK(audio_trace.lastTrack == 1);
    CHECK(audio_trace.lastVolume == 74);
    CHECK(audio_trace.lastLoop == 1);

    memset(&audio_trace, 0, sizeof(audio_trace));
    GameStruct.gameMode = 6;
    GameStruct.CurrentLevel = 2;
    mmUpdateModSet(md, 0, 0);
    CHECK(audio_trace.calls == 1);
    CHECK(audio_trace.lastControl == JPB_AUDIO_STREAM_SET_CHANNEL_TYPE);
    CHECK(audio_trace.lastValue == 2);
    CHECK(audio_trace.playCalls == 1);
    CHECK(audio_trace.lastTrack == aLevelXATracks[2]);

    md[4] = 71;
    OptionStruct.Language = 4;
    platform_trace.cueCalls = 0;
    mmUpdateModSet(md, 4, 0);
    CHECK(platform_trace.cueCalls == 1);
    CHECK(strcmp(platform_trace.lastCue, "xopt_sel") == 0);

    GameStruct.GameState = 0;
    menuVars.ultimate = 1;
    md[4] = 55;
    mmUpdateModSet(md, 1, 0);
    CHECK((GameStruct.GameState & UINT32_C(0x04000000)) != 0);
    menuVars.ultimate = 0;
    mmUpdateModSet(md, 0, 1);
    CHECK((GameStruct.GameState & UINT32_C(0x04000000)) == 0);

    jpb_AudioStreamSetPlayHook(NULL, NULL);
    jpb_AudioStreamSetControlHook(NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_variable_menu_render_and_input(void)
{
    uint32_t variable_menu[] = {
        0, 1,
        3, 0, 100,
        6, 2,
        9, 0, 100, 0, 20,
        0x14
    };
    MenuDrawTrace trace;

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
    OptionStruct.ScreenWidth = 960;
    OptionStruct.ScreenHeight = 540;
    scaleAdjustment = getScaleAdjustment();
    scaleAdjustmentMM = getScaleAdjustmentMM();
    allText[100] = "Music";
    allText[277] = "Off";
    allText[278] = "On";
    OptionStruct.Music = 1;
    menuVars.menuModeSP = 0;
    menuVars.mmSelect1[0] = 0;
    jpb_TextSetDrawHook(capture_menu_text, &trace);

    mmDraw(variable_menu);
    CHECK(trace.calls == 1);
    CHECK(jpb_utf16_compare(trace.text[0], L"> Music On <") == 0);
    CHECK(menuVars.mmSelectPtr == &variable_menu[7]);

    menuVars.pad[0] = JPB_PAD_RIGHT;
    menu_mainMenu(variable_menu);
    CHECK(OptionStruct.Music == 0);
    menuVars.pad[0] = 0;
    memset(&trace, 0, sizeof(trace));
    mmDraw(variable_menu);
    CHECK(trace.calls == 1);
    CHECK(jpb_utf16_compare(trace.text[0], L"> Music Off <") == 0);

    menuVars.pad[0] = JPB_PAD_LEFT;
    menu_mainMenu(variable_menu);
    CHECK(OptionStruct.Music == 1);
    jpb_TextSetDrawHook(NULL, NULL);
    return 0;
}

static int test_player_and_flow_selection(void)
{
    reset_menu_state();
    menu_setNumPlayers(2);
    CHECK(GameStruct.NumPlayers == 2);
    CHECK(game_GET_GLOBALBIT(2) == 1);
    CHECK(game_GET_GLOBALBIT(8) == 0);
    CHECK(game_GET_GLOBALBIT(12) == 0);

    menuVars.pplayers[0] = 4;
    menuVars.pplayers[1] = 7;
    menu_setPlayer(0, 81);
    CHECK(GameStruct.ModelSelect[0] == 81);
    CHECK(GameStruct.AIselect[0] == 4);
    CHECK(game_GET_GLOBALBIT(3) == 0);
    CHECK(game_GET_GLOBALBIT(4) == 1);
    menu_setPlayer(1, 83);
    CHECK(GameStruct.ModelSelect[1] == 83);
    CHECK(GameStruct.AIselect[1] == 7);
    CHECK(game_GET_GLOBALBIT(11) == 1);

    menu_setNumPlayers(1);
    CHECK(GameStruct.NumPlayers == 1);
    CHECK(game_GET_GLOBALBIT(2) == 0);
    padShockable = 1;
    menu_setShockOption(0);
    menu_setShockOption(1);
    CHECK(OptionStruct.ShockFlag[0] == 1);
    CHECK(OptionStruct.ShockFlag[1] == 0);

    menuVars.scoreScore = 1200;
    menu_setPointSeek(1750);
    CHECK(menuVars.pointSeek == 550);
    menuVars.pad[0] = 0x13;
    menuVars.pad[1] = 0x01;
    menu_startAcceptDecline(0x03, 0x20);
    CHECK(menuVars.pad[0] == 0x30);
    CHECK(menuVars.pad[1] == 0x20);

    memset(menuVars.mmvTriggers, 0xff, sizeof(menuVars.mmvTriggers));
    menu_tempClearTrigger();
    CHECK(memcmp(menuVars.mmvTriggers, "\0\0\0\0\0\0", 6) == 0);

    GameStruct.continueAble = 1;
    menu_startTraining(3);
    CHECK(menuVars.trainingLevel == 3);
    CHECK(LevelSelect == 19);
    CHECK(GameStruct.gameMode == 2);
    menu_levelSelect();
    CHECK(GameStruct.gameMode == 4 && GameStruct.inMenuFlag == 0);
    GameStruct.gameMode = 0;
    GameStruct.inMenuFlag = 1;
    menu_restartLevel();
    CHECK(GameStruct.gameMode == 4 && GameStruct.inMenuFlag == 0);
    CHECK(menu_setCanShowRegisterGame(7) == 7);
    CHECK(m_canShowRegisterGame == 7);
    return 0;
}

static int test_message_transitions(void)
{
    int saved_camera_type = camera_GetCurrentCameraType();

    reset_menu_state();
    gGlobalTimer = 0;
    gCamera.cameraTimer = 0;
    gCamera.userData = 0;
    camera_SetCurrentCameraType(4);
    menu_enterTitleMode();
    menuVars.menuModeSP = 0;
    menu_handleObjectiveMessage();
    CHECK(GameStruct.inMenuFlag == 1);
    CHECK(menuVars.menuModeSP == 2);
    CHECK(menuVars.menuMode[1] == 0x41);
    CHECK(menuVars.menuMode[2] == 0x2a);

    menuVars.menuModeSP = 0;
    GameStruct.GameState = 0;
    menu_initGameover();
    CHECK((GameStruct.GameState & UINT32_C(0x02000000)) != 0);
    CHECK(GameStruct.inMenuFlag == 1);
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 0x2d);

    memset(menuVars.mmSelect1, 1, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 1, sizeof(menuVars.mmSelect2));
    GameStruct.inMenuFlag = 1;
    clearMenuStuff();
    CHECK(GameStruct.inMenuFlag == 0);
    CHECK(menuVars.mmSelect1[0] == 0);
    CHECK(menuVars.mmSelect2[7] == 0);
    camera_SetCurrentCameraType(saved_camera_type);
    return 0;
}

static int test_jedi_progression_owners(void)
{
    CVECTOR lhs = {1, 2, 3, 4};
    CVECTOR rhs = {1, 2, 3, 4};
    int attack_bonus;
    int defend_bonus;
    int skill;
    int highest;
    int index;
    WorldData world;
    WorldData *saved_world = gpWorld;

    CHECK(pointLvls[0][0] == 8);
    CHECK(pointLvls[6][3] == 79);
    CHECK(award[2][2] == UINT16_C(0x4000));
    CHECK(award[10][2] == UINT16_C(0x0400));
    CHECK(CVECTOR_Equals(lhs, rhs) == 1);
    rhs.cd = 5;
    CHECK(CVECTOR_Equals(lhs, rhs) == 0);

    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    jediUpgrades[2].forcePowers =
        (int16_t)UINT16_C(0x07b0);
    jedi_CalcBonusLevels(2, &attack_bonus, &defend_bonus);
    CHECK(attack_bonus == 3);
    CHECK(defend_bonus == 3);
    jedi_CalcBonusLevels(5, &attack_bonus, &defend_bonus);
    CHECK(attack_bonus == 0);
    CHECK(defend_bonus == 0);

    memset(GameStruct.jediLevelPlayed, 0,
           sizeof(GameStruct.jediLevelPlayed));
    jediUpgrades[2].awardData[1] = 3;
    jediUpgrades[2].awardData[2] = 4;
    GameStruct.jediLevelPlayed[2][7] = 1;
    jedi_CalcSkillLevels(2, &skill, &highest);
    CHECK(skill == 20);
    CHECK(highest == 7);

    jediUpgrades[5].awardData[1] = 4;
    GameStruct.jediLevelPlayed[5][4] = 1;
    jedi_CalcSkillLevels(5, &skill, &highest);
    CHECK(skill == 10);
    CHECK(highest == 4);

    memset(GameStruct.jediLevelPlayed, 0,
           sizeof(GameStruct.jediLevelPlayed));
    GameStruct.jediLevelPlayed[4][9] = 1;
    GameStruct.jediLevelPlayed[5][10] = 1;
    CHECK(jedi_GetHighestLevel() == 10);

    GameStruct.ModelSelect[0] = 2;
    GameStruct.ModelSelect[1] = 3;
    GameStruct.NumPlayers = 2;
    jediUpgrades[2].lifeUpgrades = 2;
    jediUpgrades[3].lifeUpgrades = 7;
    CHECK(jedi_GetLives() == 4);
    GameStruct.NumPlayers = 1;
    CHECK(jedi_GetLives() == 2);

    jpb_ConsoleResetCommands();
    secretBits = UINT32_C(0x00400001);
    jedi_ShowSecrets();
    CHECK(strcmp(jpb_ConsoleBufferLine(0), "secret bits set") == 0);
    CHECK(strcmp(jpb_ConsoleBufferLine(1), "mini1 unlocked") == 0);
    CHECK(strcmp(jpb_ConsoleBufferLine(2), "t7 beat") == 0);

    memset(&world, 0, sizeof(world));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    memset(&gaPlayerData[0], 0, sizeof(gaPlayerData[0]));
    world.player0 = &gaPlayerData[0];
    gpWorld = &world;
    GameStruct.NumPlayers = 1;
    GameStruct.CurrentLevel = 1;
    GameStruct.ModelSelect[0] = obi_wan_model;
    gaPlayerData[0].playerID = obi_wan_model;
    gaPlayerData[0].playernum = 0;
    gaPlayerData[0].maxCombos = 1;
    CHECK(jedi_GetAwardFlags(0, 8000) == 4);
    CHECK(GameStruct.jediLevelPlayed[obi_wan_model][1] == 1);
    CHECK(GameStruct.jediScorePerLevel[obi_wan_model][1] == 8000);
    CHECK(jediUpgrades[obi_wan_model].awardData[1] == 1);
    jediUpgrades[obi_wan_model].awardData[1] = 0;
    jediUpgrades[obi_wan_model].forcePowers = 0;
    game_enableCombo(obi_wan_model, 0);
    CHECK(jedi_GetAwardFlags(0, 8000) == 0);
    CHECK(jedi_GetAwardFlags(0, 16000) == 3);
    CHECK(jediUpgrades[obi_wan_model].awardData[1] == 2);
    jediUpgrades[obi_wan_model].awardData[1] = 1;
    jediUpgrades[obi_wan_model].healthUpgrades = 5;
    jediUpgrades[obi_wan_model].forceUpgrades = 5;
    CHECK(jedi_GetAwardFlags(0, 16000) == 0);
    GameStruct.mNumContinues = 0;
    CHECK(jedi_GetAwardFlags(0, 24000) == 0x40);
    CHECK(GameStruct.mNumContinues == 5);
    CHECK(jediUpgrades[obi_wan_model].lifeUpgrades == 1);
    CHECK(jedi_GetAwardFlags(0, 24000) == 0);

    for (index = 0; index < JPB_EXTRA_CHARACTER_COUNT; ++index) {
        ExtraCharacters[index].Unlocked = 0;
    }
    GameStruct.CurrentLevel = 10;
    CHECK(jedi_GetAwardFlags(0, 5000) == 0);
    CHECK((secretBits & UINT32_C(0x40)) != 0);
    for (index = 0; index < JPB_EXTRA_CHARACTER_COUNT; ++index) {
        if (ExtraCharacters[index].ID != loader_model) {
            CHECK(ExtraCharacters[index].Unlocked == 1);
        }
    }

    GameStruct.CurrentLevel = 16;
    secretBits = 0;
    CHECK(jedi_GetAwardFlags(0, 0) == 0);
    CHECK(GameStruct.jediLevelPlayed[obi_wan_model][16] == 1);
    CHECK((secretBits & (UINT32_C(1) << 16)) != 0);

    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    jediUpgrades[0].attackDefendUpgrades = 0x22;
    jediUpgrades[0].lifeUpgrades = 0x33;
    CHECK(jedi_SetHighestLevel(123) == 1);
    CHECK(GameStruct.mNumContinues == 9);
    CHECK(GameStruct.maxEnergyLevels[8] == 200);
    CHECK(GameStruct.maxEnergyLineLength[8] == 50);
    CHECK(GameStruct.maxForceLevels[8] == 200);
    CHECK(GameStruct.maxForceLineLength[8] == 50);
    CHECK(jediUpgrades[0].healthUpgrades == 5);
    CHECK(jediUpgrades[0].forceUpgrades == 5);
    CHECK(jediUpgrades[0].attackDefendUpgrades == 0x22);
    CHECK(jediUpgrades[0].lifeUpgrades == 0x33);
    CHECK(jediUpgrades[0].forcePowers == -1);
    CHECK(game_getCombo(8, 47) != 0);
    for (index = 1; index <= 10; ++index) {
        CHECK(GameStruct.jediLevelPlayed[8][index] == 1);
        CHECK(GameStruct.jediScorePerLevel[8][index] == 99999);
        CHECK(jediUpgrades[8].awardData[index] == 3);
        CHECK(GameStruct.jediLevelPlayed[battle_d_model][index] == 1);
        CHECK(GameStruct.jediScorePerLevel[battle_d_model][index] == 99999);
        CHECK(jediUpgrades[battle_d_model].awardData[index] == 3);
    }
    jedi_ShowStats(0);
    gpWorld = saved_world;
    return 0;
}

static int test_jedi_combo_presentation(void)
{
    gamestruct saved_game = GameStruct;
    optionstruct saved_options = OptionStruct;
    WorldData *saved_world = gpWorld;
    char *saved_player_one_text = allText[471];
    char *saved_player_two_text = allText[472];
    float saved_icon_scale = iconScaleOverride;
    float saved_scale = scaleAdjustment;
    float saved_scale_mm = scaleAdjustmentMM;
    int saved_player_two_override = player2IconOverride;
    int saved_input_type = lastUsedInputType;
    int16_t saved_tally[2][2] = {
        {comboTally[0][0], comboTally[0][1]},
        {comboTally[1][0], comboTally[1][1]}
    };
    WorldData world;
    playerObject player_one;
    playerObject player_two;
    Combo player_one_combos[2];
    Combo player_two_combos[1];
    MenuDrawTrace trace;

    memset(&world, 0, sizeof(world));
    memset(&player_one, 0, sizeof(player_one));
    memset(&player_two, 0, sizeof(player_two));
    memset(player_one_combos, 0, sizeof(player_one_combos));
    memset(player_two_combos, 0, sizeof(player_two_combos));
    memset(&trace, 0, sizeof(trace));
    world.player0 = &player_one;
    world.player1 = &player_two;
    gpWorld = &world;
    player_one.playerID = 9;
    player_one.maxCombos = 2;
    player_one.paCombos = player_one_combos;
    (void)strcpy(player_one_combos[0].String, "en");
    (void)strcpy(player_one_combos[1].String, ".");
    player_two.playerID = 9;
    player_two.maxCombos = 1;
    player_two.paCombos = player_two_combos;
    (void)strcpy(player_two_combos[0].String, "sw..f");
    comboTally[0][0] = 12;
    comboTally[1][0] = 7;
    memset(&GameStruct, 0, sizeof(GameStruct));
    GameStruct.NumPlayers = 2;
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    OptionStruct.ScreenWidth = 1920;
    OptionStruct.ScreenHeight = 1080;
    OptionStruct.ControllerConfig[1] = 1;
    scaleAdjustment = 1.0f;
    scaleAdjustmentMM = 1.0f;
    lastUsedInputType = 0;
    allText[471] = "PLAYER ONE";
    allText[472] = "PLAYER TWO";
    iconScaleOverride = -1.0f;
    player2IconOverride = 0;
    jpb_TextSetDrawHook(capture_menu_text, &trace);

    CHECK(jedi_ShowCombos(0) == (int)(intptr_t)&player_two);
    CHECK(trace.calls == 10);
    CHECK(jpb_utf16_compare(trace.text[0], L"PLAYER ONE") == 0);
    CHECK(trace.tint[0] == 14 && trace.mode[0] == 2);
    CHECK(trace.x[0] == 510 && trace.y[0] == 80);
    CHECK(trace.scale[0] == 2.25f);
    CHECK(jpb_utf16_compare(trace.text[1], L" 12") == 0);
    CHECK(trace.x[1] == 250 && trace.y[1] == 180);
    CHECK(jpb_utf16_compare(trace.text[2], L"<B>") == 0);
    CHECK(trace.x[2] == 375 && trace.y[2] == 180);
    CHECK(jpb_utf16_compare(trace.text[3], L"<Y>") == 0);
    CHECK(trace.x[3] == 425 && trace.y[3] == 180);
    CHECK(jpb_utf16_compare(trace.text[4], L"PLAYER TWO") == 0);
    CHECK(trace.x[4] == 1410 && trace.y[4] == 80);
    CHECK(jpb_utf16_compare(trace.text[5], L"  7") == 0);
    CHECK(trace.x[5] == 1150 && trace.y[5] == 180);
    CHECK(jpb_utf16_compare(trace.text[6], L"<Y>") == 0);
    CHECK(trace.x[6] == 1275 && trace.y[6] == 180);
    CHECK(jpb_utf16_compare(trace.text[7], L"<X>") == 0);
    CHECK(trace.x[7] == 1325 && trace.y[7] == 180);
    CHECK(jpb_utf16_compare(trace.text[8], L"-") == 0);
    CHECK(trace.x[8] == 1418 && trace.y[8] == 195);
    CHECK(jpb_utf16_compare(trace.text[9], L"<F>") == 0);
    CHECK(trace.x[9] == 1525 && trace.y[9] == 180);
    CHECK(iconScaleOverride == -1.0f);
    CHECK(player2IconOverride == 0);

    memset(&trace, 0, sizeof(trace));
    menuVars.menuModeSP = 0u;
    menuVars.menuMode[0] = 0x11u;
    tempPlayersVs = 0;
    p1Disconnected = 0;
    p2Disconnected = 0;
    menu_mainLoop();
    CHECK(trace.calls > 10);
    CHECK(menuVars.menuMode[0] == 0x11u);

    memset(&trace, 0, sizeof(trace));
    GameStruct.NumPlayers = 1;
    CHECK(jedi_ShowCombos(0) == (int)(intptr_t)&player_one);
    CHECK(trace.calls == 4);
    CHECK(iconScaleOverride == 0.3f);

    jpb_TextSetDrawHook(NULL, NULL);
    GameStruct = saved_game;
    OptionStruct = saved_options;
    gpWorld = saved_world;
    allText[471] = saved_player_one_text;
    allText[472] = saved_player_two_text;
    iconScaleOverride = saved_icon_scale;
    scaleAdjustment = saved_scale;
    scaleAdjustmentMM = saved_scale_mm;
    player2IconOverride = saved_player_two_override;
    lastUsedInputType = saved_input_type;
    comboTally[0][0] = saved_tally[0][0];
    comboTally[0][1] = saved_tally[0][1];
    comboTally[1][0] = saved_tally[1][0];
    comboTally[1][1] = saved_tally[1][1];
    return 0;
}

int main(void)
{
    const JPBPlatformAchievementHooks achievement_hooks = {
        ignore_achievement_call,
        ignore_achievement_call
    };

#if defined(_MSC_VER)
    /* Keep Debug CRT assertions in the test log. A failed unattended gate
     * must never leave a modal Runtime Library window behind. */
    _set_error_mode(_OUT_TO_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    jpb_PlatformSetAchievementHooks(&achievement_hooks, NULL);
    if (test_title_and_stack() != 0 ||
        test_special_message_transition() != 0 ||
        test_pre_fmv_transition_has_no_rumble() != 0 ||
        test_main_menu_initialization() != 0 ||
        test_recovered_title_menu_data() != 0 ||
        test_recovered_menu_texture_bank() != 0 ||
        test_character_select_arrows() != 0 ||
        test_audio_menu_sliders() != 0 ||
        test_reachable_menu_entry_initialization() != 0 ||
        test_p1_character_select_presentation() != 0 ||
        test_p2_character_select_presentation() != 0 ||
        test_character_select_reconnect_presentation() != 0 ||
        test_character_select_controller_tabs() != 0 ||
        test_keyboard_prompt_glyph_owner() != 0 ||
        test_training_selection_owner() != 0 ||
        test_level_selection_owner() != 0 ||
        test_legacy_level_selection_renderer() != 0 ||
        test_title_command_interpreter() != 0 ||
        test_main_loop_dispatch() != 0 ||
        test_demo_movie_owner() != 0 ||
        test_concept_art_presentation() != 0 ||
        test_credit_presentation() != 0 ||
        test_eula_presentation_and_acceptance() != 0 ||
        test_load_bar_initialization() != 0 ||
        test_council_presentation() != 0 ||
        test_load_screen_presentation() != 0 ||
        test_memory_menu_presentation() != 0 ||
        test_recovered_main_loop_state_routes() != 0 ||
        test_extended_main_loop_state_routes() != 0 ||
        test_title_load_presentation() != 0 ||
        test_recovered_definition_routes() != 0 ||
        test_mod_value_accessors() != 0 ||
        test_variable_menu_render_and_input() != 0 ||
        test_exact_state_leaves() != 0 ||
        test_exact_noop_leaves() != 0 ||
        test_small_recovered_leaves() != 0 ||
        test_mover_bytecode() != 0 ||
        test_unformatted_card_prompt() != 0 ||
        test_legacy_controller_prompt() != 0 ||
        test_abort_pause_and_overlay() != 0 ||
        test_controller_disconnect_box() != 0 ||
        test_combo_prerequisite_chain() != 0 ||
        test_combo_string_builders() != 0 ||
        test_menu_control_owner() != 0 ||
        test_level_eligibility_and_score_modes() != 0 ||
        test_save_game_struct_initialization() != 0 ||
        test_vrm_font_metadata_load() != 0 ||
        test_score_smackdown() != 0 ||
        test_vs_extra_player_mapping() != 0 ||
        test_player_selection_summary_screens() != 0 ||
        test_legacy_player_selection_renderer() != 0 ||
        test_score_combo_filter() != 0 ||
        test_score_combo_list_generation() != 0 ||
        test_score_combo_draw() != 0 ||
        test_score_combo_debug_draw() != 0 ||
        test_score_award_menu_draw() != 0 ||
        test_score_bonus_message_draw() != 0 ||
        test_score_redline_draw() != 0 ||
        test_score_screen_state_machine() != 0 ||
        test_score_screen_main_dispatch() != 0 ||
        test_score_screen_initialization() != 0 ||
        test_score_mode_entry() != 0 ||
        test_big_score_draw() != 0 ||
        test_score_mover_owner() != 0 ||
        test_character_selection_foundations() != 0 ||
        test_p1_character_selection_state() != 0 ||
        test_p2_character_selection_state() != 0 ||
        test_vs_mode_state() != 0 ||
        test_trigger_dispatcher() != 0 ||
        test_player_and_flow_selection() != 0 ||
        test_message_transitions() != 0 ||
        test_jedi_progression_owners() != 0 ||
        test_jedi_combo_presentation() != 0) {
        return 1;
    }
    puts("menu tests passed");
    return 0;
}
