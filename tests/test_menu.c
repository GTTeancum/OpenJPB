#include "jpb/menu.h"

#include "jpb/alltext.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/jedi.h"
#include "jpb/resources.h"
#include "jpb/sprite.h"
#include "jpb/text.h"
#include "jpb/texture.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct PlatformTrace {
    int order[8];
    int count;
    int assignCalls;
    int numPlayersDuringAssign;
    int activationCalls;
    uint32_t activatedDestination;
    int soundCalls;
    unsigned lastSound;
    int movieCalls;
    unsigned lastMovie;
    int lastMovieFlags;
    int cleanupCalls;
    int saveCalls;
    int inMenuCalls;
    int lastInMenu;
    int scanCalls;
    unsigned lastScanLevel;
    int cueCalls;
    char lastCue[16];
    int transformCalls;
    int resolutionCalls;
    unsigned lastResolution;
    unsigned lastWindowMode;
    unsigned controllerCount;
    int controllerCountCalls;
    int fallbackCalls;
    int exitCalls;
} PlatformTrace;

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
    wchar_t text[32][64];
} MenuDrawTrace;

typedef struct MenuInputTrace {
    uint32_t pads[JPB_INPUT_PAD_COUNT];
    uint8_t keyboard[512];
} MenuInputTrace;

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
    CVECTOR colors[64];
    float layers[64];
} MenuTextureDrawTrace;

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

static int capture_menu_text(
    void *user_data,
    int tint,
    int alpha,
    int mode,
    int x,
    int y,
    float scale,
    float scale_adjustment,
    int font_style,
    const wchar_t *text)
{
    MenuDrawTrace *trace = (MenuDrawTrace *)user_data;
    int index = trace->calls++;

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
        trace->clipEnabled[index] = jpb_TextGetClipRect(
            &trace->clipLeft[index],
            &trace->clipTop[index],
            &trace->clipRight[index],
            &trace->clipBottom[index]);
        wcsncpy(trace->text[index], text, 63);
        trace->text[index][63] = L'\0';
    }
    return (int)wcslen(text);
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

static void trace_menu_sound(unsigned sound, void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

    ++trace->soundCalls;
    trace->lastSound = sound;
}

static void trace_movie(
    unsigned movie, int flags, void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

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

static void trace_apply_resolution(
    unsigned resolution_index,
    unsigned window_mode,
    uint32_t *width,
    uint32_t *height,
    void *user_data)
{
    PlatformTrace *trace = (PlatformTrace *)user_data;

    ++trace->resolutionCalls;
    trace->lastResolution = resolution_index;
    trace->lastWindowMode = window_mode;
    *width = 1280;
    *height = 720;
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

static void reset_menu_state(void)
{
    memset(&menuVars, 0, sizeof(menuVars));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    menuTexLoaded = 0;
    menuTexLoaded2 = 0;
    padShockable = 0;
    screenSaverCount = 0;
    screenSaverFlag = 0;
    saverAlpha = 0;
    memset(saverPads, 0, sizeof(saverPads));
    keyboardBufferIndex = 0;
    keyboardKeyPressed = 0;
    memset(keyboardBuffer, 0, sizeof(keyboardBuffer));
    secretBits = 0;
    tempPlayersVs = 0;
    p2Connected = 0;
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
    CHECK(text_trace.calls == 4);
    CHECK(wcscmp(text_trace.text[0], allText[332]) == 0);
    CHECK(wcscmp(text_trace.text[1], allText[481]) == 0);

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
    CHECK(text_trace.calls == 8);
    CHECK(wcscmp(text_trace.text[0], allText[481]) == 0);
    CHECK(wcscmp(text_trace.text[1], allText[332]) == 0);
    CHECK(wcscmp(text_trace.text[4], allText[482]) == 0);
    CHECK(wcscmp(text_trace.text[5], allText[333]) == 0);
    CHECK(player2IconOverride == 0);

    jpb_MenuSetP2CharacterSelectDrawHook(NULL, NULL);
    jpb_TextSetDrawHook(NULL, NULL);
    jpb_WHookSetDrawTextureHook(NULL, NULL);
    memset(menuTextures, 0, sizeof(menuTextures));
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
    CHECK(texture_trace.destinations[7].left == 60);
    CHECK(texture_trace.destinations[7].right == 110);
    CHECK(texture_trace.destinations[8].left == 360);
    CHECK(texture_trace.destinations[8].right == 410);
    CHECK(texture_trace.materials[17] == &primary);
    CHECK(texture_trace.materials[18] == &secondary);
    CHECK(texture_trace.destinations[17].left == 850);
    CHECK(texture_trace.destinations[17].right == 900);
    CHECK(texture_trace.destinations[18].left == 550);
    CHECK(texture_trace.destinations[18].right == 600);

    jpb_WHookSetDrawTextureHook(NULL, NULL);
    memset(menuTextures, 0, sizeof(menuTextures));
    memset(kbmTextures, 0, sizeof(kbmTextures));
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
    CHECK(wcscmp(text_trace.text[2], allText[481]) == 0);
    CHECK(wcscmp(text_trace.text[3], allText[332]) == 0);
    CHECK(wcscmp(text_trace.text[6], allText[483]) == 0);
    CHECK(wcscmp(text_trace.text[7], allText[321]) == 0);

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
    CHECK(LevelSelect == 1);

    menu_drawLevelSelectScreen(0);
    CHECK(texture_trace.calls == 42);
    CHECK(texture_trace.materials[0] == menuTextures[164]);
    CHECK(texture_trace.materials[1] == menuTextures[80]);
    CHECK(texture_trace.materials[2] == menuTextures[3]);
    CHECK(texture_trace.materials[5] == menuTextures[3]);
    CHECK(texture_trace.materials[13] == menuTextures[3]);
    CHECK(texture_trace.materials[36] == menuTextures[165]);
    CHECK(texture_trace.layers[0] == 0.9f);
    CHECK(texture_trace.layers[1] == 0.5f);
    CHECK(texture_trace.layers[2] == 0.5f);
    CHECK(texture_trace.layers[34] == 0.64f);
    CHECK(texture_trace.layers[35] == 0.64f);
    CHECK(texture_trace.layers[36] == 0.4f);
    CHECK(texture_trace.layers[41] == 1.0f);
    CHECK(texture_trace.destinations[0].left == 0);
    CHECK(texture_trace.destinations[0].top == 0);
    CHECK(texture_trace.destinations[0].right == 1920);
    CHECK(texture_trace.destinations[0].bottom == 1080);
    CHECK(texture_trace.destinations[1].left == 116);
    CHECK(texture_trace.destinations[1].top == 92);
    CHECK(texture_trace.destinations[1].right == 960);
    CHECK(texture_trace.destinations[1].bottom == 725);
    CHECK(texture_trace.destinations[2].left >= 1150);
    CHECK(texture_trace.destinations[3].left >= 1350);
    CHECK(texture_trace.destinations[4].left >= 1560);
    CHECK(texture_trace.destinations[13].left >= 1398);
    CHECK(texture_trace.destinations[13].right > texture_trace.destinations[13].left);
    CHECK(texture_trace.materials[34] == menuTextures[164]);
    CHECK(texture_trace.destinations[34].left == 0);
    CHECK(texture_trace.destinations[34].top == 0);
    CHECK(texture_trace.destinations[34].right == 1920);
    CHECK(texture_trace.destinations[34].bottom == 112);
    CHECK(texture_trace.sources[34].left == 0);
    CHECK(texture_trace.sources[34].top == 0);
    CHECK(texture_trace.sources[34].right == 1920);
    CHECK(texture_trace.sources[34].bottom == 112);
    CHECK(texture_trace.materials[35] == menuTextures[164]);
    CHECK(texture_trace.destinations[35].left == 0);
    CHECK(texture_trace.destinations[35].top == 804);
    CHECK(texture_trace.destinations[35].right == 1920);
    CHECK(texture_trace.destinations[35].bottom == 1080);
    CHECK(texture_trace.sources[35].left == 0);
    CHECK(texture_trace.sources[35].top == 804);
    CHECK(texture_trace.sources[35].right == 1920);
    CHECK(texture_trace.sources[35].bottom == 1080);
    CHECK(texture_trace.destinations[36].left == 0);
    CHECK(texture_trace.destinations[36].top == 0);
    CHECK(texture_trace.destinations[36].right == 1920);
    CHECK(texture_trace.destinations[36].bottom == 1080);
    CHECK(texture_trace.materials[41] == whitemat);
    CHECK(texture_trace.destinations[41].left == 0);
    CHECK(texture_trace.destinations[41].top == 0);
    CHECK(texture_trace.destinations[41].right == 1920);
    CHECK(texture_trace.destinations[41].bottom == 1080);
    CHECK(texture_trace.colors[41].r == 0);
    CHECK(texture_trace.colors[41].g == 0);
    CHECK(texture_trace.colors[41].b == 2);
    CHECK(texture_trace.colors[41].cd == 255);
    CHECK(texture_trace.sources[37].left == 216);
    CHECK(texture_trace.sources[37].top == 0);
    CHECK(texture_trace.sources[37].right == 328);
    CHECK(texture_trace.sources[37].bottom == 104);
    CHECK(texture_trace.sources[38].left == 104);
    CHECK(texture_trace.sources[38].top == 0);
    CHECK(texture_trace.sources[38].right == 216);
    CHECK(texture_trace.sources[38].bottom == 104);
    CHECK(texture_trace.sources[2].left == 712);
    CHECK(texture_trace.sources[2].top == 248);
    CHECK(texture_trace.sources[2].right == 884);
    CHECK(texture_trace.sources[2].bottom == 316);
    CHECK(texture_trace.sources[13].left == 32);
    CHECK(texture_trace.sources[13].top == 340);
    CHECK(texture_trace.sources[13].right == 64);
    CHECK(texture_trace.sources[13].bottom == 384);
    CHECK(text_trace.calls == 4);
    CHECK(wcscmp(text_trace.text[0], allText[190]) == 0);
    CHECK(wcscmp(text_trace.text[1], allText[306]) == 0);
    CHECK(text_trace.scale[0] == 2.5f);
    CHECK(text_trace.scale[1] == 2.5f);

    memset(&texture_trace, 0, sizeof(texture_trace));
    memset(&text_trace, 0, sizeof(text_trace));
    lastUsedInputType = 1;
    menu_test_controller_name = "Generic Controller";
    hooks.controllerName = read_menu_controller_name;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
    menu_drawLevelSelectScreen(0);
    CHECK(texture_trace.calls == 42);
    CHECK(texture_trace.materials[34] == menuTextures[164]);
    CHECK(texture_trace.materials[35] == menuTextures[164]);
    CHECK(texture_trace.materials[36] == menuTextures[165]);
    CHECK(texture_trace.materials[39] == kbmTextures[9]);
    CHECK(texture_trace.materials[40] == kbmTextures[2]);
    CHECK(texture_trace.materials[41] == whitemat);
    CHECK(texture_trace.destinations[39].left >= 1500);
    CHECK(texture_trace.destinations[39].top >= 804);
    CHECK(texture_trace.destinations[40].left >= 1500);
    CHECK(texture_trace.destinations[40].top >= 804);
    CHECK(text_trace.calls == 4);
    CHECK(wcscmp(text_trace.text[2], L"Exit") == 0);
    CHECK(wcscmp(text_trace.text[3], L"Select") == 0);

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
    CHECK(menuVars.menuModeSP == 2);
    CHECK(menuVars.menuMode[2] == 0x66);
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
    CHECK(wcscmp(trace.text[0], L"> New Game <") == 0);
    CHECK(wcscmp(trace.text[1], L"Training") == 0);
    CHECK(wcscmp(trace.text[2], L"VS. Mode") == 0);
    CHECK(wcscmp(trace.text[3], L"Options") == 0);
    CHECK(wcscmp(trace.text[4], L"Quit") == 0);
    CHECK(wcscmp(trace.text[5], L"Register Your Game") == 0);
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
    CHECK(wcscmp(trace.text[0], L"> YES <") == 0);
    CHECK(wcscmp(trace.text[1], L"NO") == 0);
    CHECK(wcscmp(trace.text[2], L"Overwrite save game?") == 0);
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

    reset_menu_state();
    memset(&trace, 0, sizeof(trace));
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

    GameStruct.continueAble = 1;
    GameStruct.difficulty = 0;
    menu_mainLoop();
    CHECK(menuVars.mmSelectPtr >= continuemainMdef);
    CHECK(menuVars.mmSelectPtr <
          continuemainMdef + 73);

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
    CHECK(wcscmp(text_trace.text[0], L"01 / 42") == 0);
    CHECK(wcscmp(text_trace.text[1], L"Exit") == 0);

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
    CHECK(wcscmp(trace.text[0], L"LucasArts") == 0);
    CHECK(wcscmp(trace.text[1], L"Exit") == 0);

    memset(&trace, 0, sizeof(trace));
    creditBarPosition = 100000.0f;
    menu_drawCredits();
    CHECK(creditBarPosition == 0.0f);
    CHECK(trace.calls == 1);
    CHECK(wcscmp(trace.text[0], L"Exit") == 0);

    menuVars.menuModeSP = 0;
    menuVars.scoreScore = 17;
    CHECK(menu_handleMenuTriggers(0x1b) == 0);
    CHECK(menuVars.menuMode[1] == 0x1b);
    CHECK(menuVars.scoreScore == 0);
    menuVars.menuModeSP = 0;
    CHECK(menu_handleMenuTriggers(0x13) == 0);
    CHECK(menuVars.menuMode[1] == 0x13);
    CHECK(skipCreditForFrame == 1);

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

    reset_menu_state();
    memset(&hooks, 0, sizeof(hooks));
    memset(&trace, 0, sizeof(trace));
    hooks.menuSound = trace_menu_sound;
    hooks.triggerMovie = trace_movie;
    hooks.cleanupLevelData = trace_cleanup;
    hooks.saveGameData = trace_save;
    hooks.setInMenu = trace_in_menu;
    hooks.scanLevel = trace_scan_level;
    hooks.soundCue = trace_sound_cue;
    hooks.refreshLevelTransforms = trace_refresh_transforms;
    hooks.applyResolution = trace_apply_resolution;
    hooks.controllerCount = trace_controller_count;
    hooks.singleControllerFallback = trace_single_controller_fallback;
    hooks.requestExit = trace_request_exit;
    trace.controllerCount = 1;
    jpb_MenuSetPlatformHooks(&hooks, &trace);

    menuVars.sndtest = 5;
    CHECK(menu_handleMenuTriggers(0x38) == 0);
    CHECK(trace.soundCalls == 1 && trace.lastSound == 5);
    menuVars.sndtest = 11;
    CHECK(menu_handleMenuTriggers(0x38) == 0);
    CHECK(trace.soundCalls == 1);
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
    CHECK(menuVars.pSelect == 0);
    CHECK(menuVars.menuMode[1] == 0x1a);
    CHECK(strcmp(trace.lastCue, "xjedsel") == 0);
    menuVars.menuModeSP = 0;
    GameStruct.NumPlayers = 2;
    menuVars.pSelect = 1;
    menuVars.subplayers[1] = 3;
    CHECK(menu_handleMenuTriggers(0x19) == 0);
    CHECK(GameStruct.ModelSelect[1] == modisorder2[3]);
    CHECK(menuVars.pSelect == 0);
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
    CHECK(menuVars.awardSet[15] == 1);
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
    CHECK(menu_handleMenuTriggers(0x96) == 0);
    CHECK(trace.resolutionCalls == 1);
    CHECK(trace.lastResolution == 5);
    CHECK(trace.lastWindowMode == 2);
    CHECK(OptionStruct.ScreenWidth == 1280);
    CHECK(OptionStruct.ScreenHeight == 720);

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

    jpb_MenuSetPlatformHooks(NULL, NULL);
    return 0;
}

static int test_level_eligibility_and_score_modes(void)
{
    int upgrade_level = -1;
    int32_t target;

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

    menuVars.menuModeSP = 2;
    menuVars.mmSelect1[2] = 4;
    menuVars.mmSelect2[2] = 5;
    menuVars.scoreCurrentPlayer = 1;
    menuVars.scoreScore = 1000;
    target = 1450;
    memcpy(menuVars.awardSet + 36 + 20, &target, sizeof(target));
    target = 1750;
    memcpy(menuVars.awardSet + 36 + 24, &target, sizeof(target));
    target = 2100;
    memcpy(menuVars.awardSet + 36 + 28, &target, sizeof(target));
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
    CHECK(menuVars.awardSet[36 + 15] == 1);
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
    CHECK(memcmp(menuVars.pselectMode, "\0\0\0\0", 4) == 0);
    CHECK(menuVars.pselectMode[4] == 0xa5);
    CHECK(memcmp(menuVars.pselectMode + 12, "\0\0\0\0", 4) == 0);
    CHECK(menuVars.pselectMode[16] == 0xa5);
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
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 0x1a);

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
    CHECK(menuVars.menuModeSP == 1);
    CHECK(menuVars.menuMode[1] == 0x1a);
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

static int test_mod_value_accessors(void)
{
    MDEF_MOD mod;
    uint32_t md[5] = {9, 0, 0, 0, 0};
    JPBMenuPlatformHooks hooks;
    PlatformTrace platform_trace;
    uint8_t byte_value = UINT8_C(0x7a);
    uint16_t word_value = UINT16_C(0x1234);
    uint32_t dword_value = UINT32_C(0x89abcdef);

    reset_menu_state();
    memset(&mod, 0, sizeof(mod));
    memset(&hooks, 0, sizeof(hooks));
    memset(&platform_trace, 0, sizeof(platform_trace));
    hooks.soundCue = trace_sound_cue;
    jpb_MenuSetPlatformHooks(&hooks, &platform_trace);
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
    CHECK(mmGetModVal(NULL) == 0);
    CHECK(mmSetModVal(NULL, 1, 0) == 0);

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

    OptionStruct.musicVolume = 37;
    GameStruct.xaVol = 0;
    md[4] = 21;
    mmUpdateModSet(md, 0, 0);
    CHECK(GameStruct.xaVol == 74);
    CHECK(strcmp(platform_trace.lastCue, "xopt_sel") == 0);

    GameStruct.GameState = 0;
    menuVars.ultimate = 1;
    md[4] = 55;
    mmUpdateModSet(md, 1, 0);
    CHECK((GameStruct.GameState & UINT32_C(0x04000000)) != 0);
    menuVars.ultimate = 0;
    mmUpdateModSet(md, 0, 1);
    CHECK((GameStruct.GameState & UINT32_C(0x04000000)) == 0);

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
    allText[100] = L"Music";
    allText[277] = L"Off";
    allText[278] = L"On";
    OptionStruct.Music = 1;
    menuVars.menuModeSP = 0;
    menuVars.mmSelect1[0] = 0;
    jpb_TextSetDrawHook(capture_menu_text, &trace);

    mmDraw(variable_menu);
    CHECK(trace.calls == 1);
    CHECK(wcscmp(trace.text[0], L"> Music On <") == 0);
    CHECK(menuVars.mmSelectPtr == &variable_menu[7]);

    menuVars.pad[0] = JPB_PAD_RIGHT;
    menu_mainMenu(variable_menu);
    CHECK(OptionStruct.Music == 0);
    menuVars.pad[0] = 0;
    memset(&trace, 0, sizeof(trace));
    mmDraw(variable_menu);
    CHECK(trace.calls == 1);
    CHECK(wcscmp(trace.text[0], L"> Music Off <") == 0);

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
    reset_menu_state();
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
    return 0;
}

int main(void)
{
#if defined(_MSC_VER)
    /* Keep Debug CRT assertions in the test log. A failed unattended gate
     * must never leave a modal Runtime Library window behind. */
    _set_error_mode(_OUT_TO_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    if (test_title_and_stack() != 0 ||
        test_main_menu_initialization() != 0 ||
        test_recovered_title_menu_data() != 0 ||
        test_recovered_menu_texture_bank() != 0 ||
        test_character_select_arrows() != 0 ||
        test_p1_character_select_presentation() != 0 ||
        test_p2_character_select_presentation() != 0 ||
        test_character_select_controller_tabs() != 0 ||
        test_training_selection_owner() != 0 ||
        test_level_selection_owner() != 0 ||
        test_title_command_interpreter() != 0 ||
        test_main_loop_dispatch() != 0 ||
        test_concept_art_presentation() != 0 ||
        test_credit_presentation() != 0 ||
        test_mod_value_accessors() != 0 ||
        test_variable_menu_render_and_input() != 0 ||
        test_exact_state_leaves() != 0 ||
        test_menu_control_owner() != 0 ||
        test_level_eligibility_and_score_modes() != 0 ||
        test_character_selection_foundations() != 0 ||
        test_p1_character_selection_state() != 0 ||
        test_p2_character_selection_state() != 0 ||
        test_trigger_dispatcher() != 0 ||
        test_player_and_flow_selection() != 0 ||
        test_message_transitions() != 0) {
        return 1;
    }
    puts("menu tests passed");
    return 0;
}
