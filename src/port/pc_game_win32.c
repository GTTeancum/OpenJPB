/*
 * Thin Win32 host for the portable gameplay/software-render loop.
 *
 * Win32 owns only key polling, timing, a window, and pixel presentation.
 * Gameplay-facing pad state and all rendering inputs remain platform neutral.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "jpb/ai.h"
#include "jpb/anim.h"
#include "jpb/brainutl.h"
#include "jpb/bullet.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/combo.h"
#include "jpb/braindmg.h"
#include "jpb/enemy.h"
#include "jpb/fmath.h"
#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/input.h"
#include "jpb/intersec.h"
#include "jpb/jedi.h"
#include "jpb/jonny.h"
#include "jpb/level_world.h"
#include "jpb/loader.h"
#include "jpb/menu.h"
#include "jpb/pc_audio_win32.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/projection.h"
#include "jpb/game_runtime.h"
#include "jpb/resources.h"
#include "jpb/savegame.h"
#include "jpb/sprite.h"
#include "jpb/text.h"
#include "jpb/texture.h"

#include "pc_image_wic.h"
#include "pc_input_mapping.h"
#include "pc_log_win32.h"
#include "pc_xinput_win32.h"
#if defined(JPB_PC_HAS_UFBX)
#include "pc_level_fbx.h"
#endif

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    PC_FRAMEBUFFER_WIDTH = 960,
    PC_FRAMEBUFFER_HEIGHT = 540,
    PC_VISIBLE_FRAMEBUFFER_WIDTH = 1920,
    PC_VISIBLE_FRAMEBUFFER_HEIGHT = 1080,
    PC_HEADLESS_PHASE_CAPACITY = 32,
    /* 132 front-end records plus the exact 77 controller/KBM records. */
    PC_MENU_TEXTURE_CAPACITY = JPB_MENU_TEXTURE_ENTRY_COUNT + 77,
    PC_MENU_TEXTURE_PATH_CAPACITY = 1024
};

typedef struct PcMenuTexture {
    char path[PC_MENU_TEXTURE_PATH_CAPACITY];
    uint32_t *pixels;
    JPBSoftwareTexture texture;
} PcMenuTexture;

typedef struct PcGameplayLogState {
    uint32_t pressed;
    uint32_t held;
    int motion;
    int activeMotion;
    int inputType;
    int energy;
    int maxEnergy;
    int force;
    int maxForce;
    int items;
    JPBPlayerCallback forceCallback;
    int initialized;
} PcGameplayLogState;

typedef struct PcMenuTextureCache {
    PcMenuTexture textures[PC_MENU_TEXTURE_CAPACITY];
    size_t count;
} PcMenuTextureCache;

typedef struct PcHeadlessPhase {
    uint32_t bits;
    int frames;
    uint32_t playerTwoBits;
    int independentPlayers;
    JPBPCGameplayKeyboardState keyboard;
    uint8_t keyboardMask;
    JPBPCXInputGamepad gamepads[2];
    uint8_t xinputMask;
} PcHeadlessPhase;

typedef struct PcExpectedScreenDraw {
    int left;
    int top;
    int right;
    int bottom;
    unsigned red;
    unsigned green;
    unsigned blue;
    unsigned alpha;
    int textureWidth;
    int textureHeight;
    float layerDepth;
} PcExpectedScreenDraw;

typedef struct PcInput {
    int headless;
    int hiddenWindow;
    int scriptedInput;
    int headlessActive;
    int validateCombat;
    int validateJump;
    int validateMultiEnemy;
    int validateRadar;
    int validateRadar1080;
    int validateHudCore;
    int validateHudCore1080;
    int validateHudNormal;
    int validateHudNormal1080;
    int validateHudNormalProof1080;
    int validateHudP2Core;
    int validateHudP2Core1080;
    int validateHudContinue;
    int validateHudContinue1080;
    int validateHudRescue;
    int validateHudRescue1080;
    int validateHudPilotCounter;
    int validateHudPilotCounter1080;
    int validateHudCountdown;
    int validateHudCountdown1080;
    int validateHudCountdownKill;
    int validateHudCountdownKill1080;
    int validateHudCountdownSuccess;
    int validateHudCountdownSuccess1080;
    int validateHudCountdownFail;
    int validateHudCountdownFail1080;
    int validateHudHangar;
    int validateHudHangar1080;
    int validateHudArena;
    int validateHudArena1080;
    int validateHudMini4;
    int validateHudMini41080;
    int validateHudDamage;
    int validateHudDamage1080;
    int validateHudDamageCompact;
    int validateHudDamageCompact1080;
    int validateHudDamageP2;
    int validateHudDamageP21080;
    int validateHudDamageP2Compact;
    int validateHudDamageP2Compact1080;
    int validateHudKadu;
    int validateHudKadu1080;
    int validateHudOffscreen;
    int validateHudOffscreen1080;
    int validateHudLifeTile;
    int validateHudLifeTile1080;
    int validateHudProjectedLifeTile;
    int validateHudProjectedLifeTile1080;
    int validateHudDebugLabels;
    int validateHudDebugLabels1080;
    int validateHudDebugLabels3;
    int validateHudDebugLabels31080;
    int validateHudOwnerCoverage;
    int validateTeleport;
    int validateCameraFollow;
    int validateTitleAudio;
    int validatePlayerSaber;
    int validatePlayerProjectile;
    int validatePresentationHandoff;
    int validateAudioHandoff;
    int validatePersistenceHandoff;
    int validateNeutralHandoff;
    int headlessMaximumProgression;
    int controllerConfigOverride[2];
    int requireFbxLevel;
    uint32_t headlessBits;
    uint32_t headlessPhaseBits;
    uint32_t observedPlayerBits[2];
    uint32_t headlessPhasePlayerTwoBits;
    int headlessPhaseIndependentPlayers;
    JPBPCGameplayKeyboardState headlessPhaseKeyboard;
    uint8_t headlessPhaseKeyboardMask;
    JPBPCXInputGamepad headlessPhaseGamepads[2];
    uint8_t headlessPhaseXInputMask;
    PcHeadlessPhase phases[PC_HEADLESS_PHASE_CAPACITY];
    int phaseCount;
    int persistenceEnabled;
    int noValidGameSave;
    unsigned gameSaveWriteCount;
    int playerOneUsesKeyboard;
    /* A menu confirmation must be released before it can become gameplay. */
    uint8_t gameplayHandoffReleaseMask;
    /* Zero means unassigned; otherwise this is physical XInput user + 1. */
    unsigned playerTwoControllerSlot;
    uint32_t previousControllerMask;
    char saveGamePath[MAX_PATH];
    char optionsPath[MAX_PATH];
    JPBPCXInput xinput;
} PcInput;

static void pc_apply_control_scheme_overrides(const PcInput *input);

typedef struct PcPlayerSaberDiagnostics {
    int playerModel;
    int expectsSaber;
    uint32_t outerColor;
    size_t outerDrawCount;
    size_t trailDrawCount;
    size_t matchedCoreDrawCount;
    size_t matchedAttachmentDrawCount;
    size_t unmatchedAttachmentDrawCount;
    int minimumOuterWidth;
    int maximumOuterWidth;
    float minimumBladeLength;
    float maximumBladeLength;
} PcPlayerSaberDiagnostics;

typedef struct PcPlayerSaberActionDiagnostics {
    int playerModel;
    int expectsSaber;
    size_t actionFrames;
    size_t validActionFrames;
    size_t invalidActionFrames;
    size_t attachedBladeDrawCount;
    size_t trailDrawCount;
    int minimumOuterWidth;
    int maximumOuterWidth;
    float minimumBladeLength;
    float maximumBladeLength;
} PcPlayerSaberActionDiagnostics;

typedef struct PcDefaultAssets {
    char mesh[MAX_PATH];
    char cad[MAX_PATH];
    char bmd[MAX_PATH];
    char cmb[MAX_PATH];
    char enemyCad[MAX_PATH];
    char enemyBmd[MAX_PATH];
} PcDefaultAssets;

typedef struct PcPlayerAssets {
    char cad[MAX_PATH];
    char bmd[MAX_PATH];
    char cmb[MAX_PATH];
} PcPlayerAssets;

static int pc_running = 1;
static char pc_asset_root[MAX_PATH];

static int pc_base_player_expects_saber(int player_model)
{
    return player_model >= obi_wan_model &&
           player_model <= ki_adi_model &&
           player_model != amidala_model &&
           player_model != panaka_model;
}

static int pc_same_glow_segment(
    const JPBGameRuntimeGlowDraw *left,
    const JPBGameRuntimeGlowDraw *right)
{
    return left->start.vx == right->start.vx &&
           left->start.vy == right->start.vy &&
           left->start.vz == right->start.vz &&
           left->end.vx == right->end.vx &&
           left->end.vy == right->end.vy &&
           left->end.vz == right->end.vz;
}

static int16_t pc_saber_scaled_component(
    int component, int scale)
{
    int product = component * scale;

    return (int16_t)(
        (product + ((product >> 31) & 0xfff)) >> 12);
}

static int pc_player_saber_node_ids(
    int player_model,
    unsigned *base_id,
    unsigned *tip_id,
    unsigned *second_base_id,
    unsigned *second_tip_id)
{
    /* Diagnostic mirror of the exact matched-PC jedi_HandleSabre node
     * selection. Keeping this in the host avoids exposing private gameplay
     * helpers merely for a real-asset regression. */
    if (base_id == NULL || tip_id == NULL ||
        second_base_id == NULL || second_tip_id == NULL ||
        !pc_base_player_expects_saber(player_model)) {
        return 0;
    }
    *second_base_id = 0;
    *second_tip_id = 0;
    if (player_model == qui_gon_model ||
        player_model == mace_model) {
        *base_id = 0x14;
        *tip_id = 0x16;
    } else if (player_model == maul_p_model) {
        *base_id = 0x12;
        *tip_id = 0x13;
        *second_base_id = 0x17;
        *second_tip_id = 0x13;
    } else if (player_model == adi_model) {
        *base_id = 0x13;
        *tip_id = 0x15;
    } else {
        *base_id = 0x11;
        *tip_id = 0x13;
    }
    return 1;
}

static int pc_expected_player_saber_segment(
    const playerObject *player,
    int second_blade,
    JPBGameRuntimeGlowDraw *expected)
{
    unsigned base_id;
    unsigned tip_id;
    unsigned second_base_id;
    unsigned second_tip_id;
    Mnode *base;
    Mnode *tip;
    _svector direction;
    int direction_sign;
    int inner_scale;

    if (player == NULL || expected == NULL ||
        !pc_player_saber_node_ids(
            player->playerID,
            &base_id,
            &tip_id,
            &second_base_id,
            &second_tip_id)) {
        return 0;
    }
    if (second_blade) {
        if (second_base_id == 0 && second_tip_id == 0) {
            return 0;
        }
        base_id = second_base_id;
        tip_id = second_tip_id;
        direction_sign = 1;
        inner_scale = 0x20;
    } else {
        direction_sign = -1;
        inner_scale = 0;
    }
    base = coll_GetNode(player->playernum, base_id);
    tip = coll_GetNode(player->playernum, tip_id);
    if (base == NULL || tip == NULL) {
        return 0;
    }
    (void)normalize(
        base->v3RotCenter.vx - tip->v3RotCenter.vx,
        base->v3RotCenter.vy - tip->v3RotCenter.vy,
        base->v3RotCenter.vz - tip->v3RotCenter.vz,
        &direction);
    if (direction_sign > 0) {
        expected->start.vx = (int16_t)(
            base->v3RotCenter.vx +
            pc_saber_scaled_component(direction.vx, 0x70));
        expected->start.vy = (int16_t)(
            base->v3RotCenter.vy +
            pc_saber_scaled_component(direction.vy, 0x70));
        expected->start.vz = (int16_t)(
            base->v3RotCenter.vz +
            pc_saber_scaled_component(direction.vz, 0x70));
        expected->end.vx = (int16_t)(
            base->v3RotCenter.vx +
            pc_saber_scaled_component(direction.vx, inner_scale));
        expected->end.vy = (int16_t)(
            base->v3RotCenter.vy +
            pc_saber_scaled_component(direction.vy, inner_scale));
        expected->end.vz = (int16_t)(
            base->v3RotCenter.vz +
            pc_saber_scaled_component(direction.vz, inner_scale));
    } else {
        expected->start.vx = (int16_t)(
            base->v3RotCenter.vx -
            pc_saber_scaled_component(direction.vx, 0x70));
        expected->start.vy = (int16_t)(
            base->v3RotCenter.vy -
            pc_saber_scaled_component(direction.vy, 0x70));
        expected->start.vz = (int16_t)(
            base->v3RotCenter.vz -
            pc_saber_scaled_component(direction.vz, 0x70));
        expected->end.vx = (int16_t)base->v3RotCenter.vx;
        expected->end.vy = (int16_t)base->v3RotCenter.vy;
        expected->end.vz = (int16_t)base->v3RotCenter.vz;
    }
    expected->start.pad = 0;
    expected->end.pad = 0;
    return 1;
}

static int pc_player_saber_segment_is_attached(
    const playerObject *player,
    const JPBGameRuntimeGlowDraw *draw)
{
    JPBGameRuntimeGlowDraw expected = {0};

    if (draw == NULL) {
        return 0;
    }
    if (pc_expected_player_saber_segment(
            player, 0, &expected) &&
        pc_same_glow_segment(draw, &expected)) {
        return 1;
    }
    return pc_expected_player_saber_segment(
               player, 1, &expected) &&
           pc_same_glow_segment(draw, &expected);
}

static void pc_collect_player_saber_diagnostics(
    const JPBGameRuntime *runtime,
    const playerObject *player,
    PcPlayerSaberDiagnostics *diagnostics)
{
    size_t outer_index;

    memset(diagnostics, 0, sizeof(*diagnostics));
    diagnostics->playerModel = -1;
    if (runtime == NULL || player == NULL) {
        return;
    }
    diagnostics->playerModel = player->playerID;
    diagnostics->expectsSaber = pc_base_player_expects_saber(
        diagnostics->playerModel);
    if (diagnostics->playerModel < obi_wan_model ||
        diagnostics->playerModel > ki_adi_model) {
        return;
    }
    diagnostics->outerColor =
        jedi_GetColour32((uint64_t)diagnostics->playerModel) |
        UINT32_C(0x7f000000);
    for (outer_index = 0;
         outer_index < runtime->glowDrawCount;
         ++outer_index) {
        const JPBGameRuntimeGlowDraw *outer =
            &runtime->glowDraws[outer_index];
        int matched_core = 0;
        size_t core_index;

        if (outer->color != diagnostics->outerColor) {
            continue;
        }
        ++diagnostics->outerDrawCount;
        for (core_index = 0;
             core_index < runtime->glowDrawCount;
             ++core_index) {
            const JPBGameRuntimeGlowDraw *core =
                &runtime->glowDraws[core_index];

            if (core->color == UINT32_C(0xffffffff) &&
                core->radius == 2 &&
                pc_same_glow_segment(outer, core)) {
                ++diagnostics->matchedCoreDrawCount;
                matched_core = 1;
                break;
            }
        }
        if (matched_core) {
            float delta_x =
                (float)outer->end.vx - (float)outer->start.vx;
            float delta_y =
                (float)outer->end.vy - (float)outer->start.vy;
            float delta_z =
                (float)outer->end.vz - (float)outer->start.vz;
            float length = sqrtf(
                delta_x * delta_x +
                delta_y * delta_y +
                delta_z * delta_z);

            if (pc_player_saber_segment_is_attached(player, outer)) {
                ++diagnostics->matchedAttachmentDrawCount;
                if (diagnostics->matchedAttachmentDrawCount == 1 ||
                    outer->radius < diagnostics->minimumOuterWidth) {
                    diagnostics->minimumOuterWidth = outer->radius;
                }
                if (diagnostics->matchedAttachmentDrawCount == 1 ||
                    outer->radius > diagnostics->maximumOuterWidth) {
                    diagnostics->maximumOuterWidth = outer->radius;
                }
                if (diagnostics->matchedAttachmentDrawCount == 1 ||
                    length < diagnostics->minimumBladeLength) {
                    diagnostics->minimumBladeLength = length;
                }
                if (diagnostics->matchedAttachmentDrawCount == 1 ||
                    length > diagnostics->maximumBladeLength) {
                    diagnostics->maximumBladeLength = length;
                }
            } else {
                ++diagnostics->unmatchedAttachmentDrawCount;
            }
        } else {
            ++diagnostics->trailDrawCount;
        }
    }
}

static int pc_validate_player_saber(
    const PcPlayerSaberDiagnostics *diagnostics)
{
    size_t expected_draws;

    if (diagnostics == NULL ||
        diagnostics->playerModel < obi_wan_model ||
        diagnostics->playerModel > ki_adi_model) {
        return 0;
    }
    if (!diagnostics->expectsSaber) {
        return diagnostics->outerDrawCount == 0 &&
               diagnostics->matchedCoreDrawCount == 0 &&
               diagnostics->matchedAttachmentDrawCount == 0;
    }
    expected_draws = diagnostics->playerModel == maul_p_model
        ? 2u
        : 1u;
    if (diagnostics->matchedAttachmentDrawCount != expected_draws ||
        diagnostics->minimumOuterWidth < 0x0e ||
        diagnostics->maximumOuterWidth > 0x13 ||
        diagnostics->maximumBladeLength < 108.0f ||
        diagnostics->maximumBladeLength > 114.0f) {
        return 0;
    }
    if (diagnostics->playerModel == maul_p_model &&
        (diagnostics->minimumBladeLength < 78.0f ||
         diagnostics->minimumBladeLength > 82.0f)) {
        return 0;
    }
    return 1;
}

static void pc_trace_player_saber_action_frame(
    const JPBGameRuntime *runtime,
    const playerObject *player,
    PcPlayerSaberActionDiagnostics *trace)
{
    PcPlayerSaberDiagnostics sample;

    if (runtime == NULL || player == NULL || trace == NULL ||
        player->pMotion == NULL || *player->pMotion == NULL ||
        ((*player->pMotion)->Damage <= 1 &&
         ((*player->pMotion)->FunctPtr < 18 ||
          (*player->pMotion)->FunctPtr > 20))) {
        return;
    }
    pc_collect_player_saber_diagnostics(runtime, player, &sample);
    if (trace->actionFrames == 0) {
        trace->playerModel = sample.playerModel;
        trace->expectsSaber = sample.expectsSaber;
    }
    ++trace->actionFrames;
    if (pc_validate_player_saber(&sample)) {
        ++trace->validActionFrames;
    } else {
        ++trace->invalidActionFrames;
    }
    trace->attachedBladeDrawCount +=
        sample.matchedAttachmentDrawCount;
    trace->trailDrawCount += sample.trailDrawCount;
    if (sample.matchedCoreDrawCount != 0) {
        if (trace->attachedBladeDrawCount ==
                sample.matchedAttachmentDrawCount ||
            sample.minimumOuterWidth < trace->minimumOuterWidth) {
            trace->minimumOuterWidth = sample.minimumOuterWidth;
        }
        if (trace->attachedBladeDrawCount ==
                sample.matchedAttachmentDrawCount ||
            sample.maximumOuterWidth > trace->maximumOuterWidth) {
            trace->maximumOuterWidth = sample.maximumOuterWidth;
        }
        if (trace->attachedBladeDrawCount ==
                sample.matchedAttachmentDrawCount ||
            sample.minimumBladeLength < trace->minimumBladeLength) {
            trace->minimumBladeLength = sample.minimumBladeLength;
        }
        if (trace->attachedBladeDrawCount ==
                sample.matchedAttachmentDrawCount ||
            sample.maximumBladeLength > trace->maximumBladeLength) {
            trace->maximumBladeLength = sample.maximumBladeLength;
        }
    }
}

static void pc_request_exit(void *user_data)
{
    (void)user_data;
    jpb_PCLog("menu requested application exit");
    pc_running = 0;
}

static int pc_activate_menu_item(
    uint32_t destination, void *user_data)
{
    PcInput *input = (PcInput *)user_data;

    if (destination == 0x90 && input != NULL &&
        input->noValidGameSave) {
        /* menu_mainMenu has already applied the recovered default trigger
         * and pushed the overwrite-confirmation state. The installed PC
         * persistence result selects the recovered no-save destination 9. */
        menu_menuExit();
        (void)menu_handleMenuTriggers(9);
        jpb_PCLog("New Game used no-save route");
    }
    return 0;
}

static void *pc_load_menu_texture(
    void *user_data,
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height)
{
    PcMenuTextureCache *cache =
        (PcMenuTextureCache *)user_data;
    PcMenuTexture *entry;
    size_t index;
    int image_width;
    int image_height;

    (void)option;
    if (cache == NULL || filename == NULL ||
        width == NULL || height == NULL) {
        return NULL;
    }
    for (index = 0; index < cache->count; ++index) {
        entry = &cache->textures[index];
        if (strcmp(entry->path, filename) == 0) {
            *width = (int16_t)entry->texture.width;
            *height = (int16_t)entry->texture.height;
            return &entry->texture;
        }
    }
    if (cache->count >= PC_MENU_TEXTURE_CAPACITY ||
        strlen(filename) >= PC_MENU_TEXTURE_PATH_CAPACITY ||
        !jpb_PCInspectImageWIC(
            filename, &image_width, &image_height) ||
        image_width > INT16_MAX || image_height > INT16_MAX ||
        (size_t)image_width >
            SIZE_MAX / (size_t)image_height / sizeof(uint32_t)) {
        return NULL;
    }
    entry = &cache->textures[cache->count];
    entry->pixels = (uint32_t *)malloc(
        (size_t)image_width * (size_t)image_height *
        sizeof(*entry->pixels));
    if (entry->pixels == NULL ||
        !jpb_PCLoadImageWIC(
            filename,
            image_width,
            image_height,
            entry->pixels,
            image_width)) {
        free(entry->pixels);
        memset(entry, 0, sizeof(*entry));
        return NULL;
    }
    memcpy(entry->path, filename, strlen(filename) + 1);
    entry->texture.pixels = entry->pixels;
    entry->texture.width = (size_t)image_width;
    entry->texture.height = (size_t)image_height;
    entry->texture.stridePixels = (size_t)image_width;
    entry->texture.materialFlags = 0;
    entry->texture.samplerType = TEXTURESAMPLER_LINEARCLAMP;
    entry->texture.colorOverride = -1;
    entry->texture.materialType = material_type;
    ++cache->count;
    *width = (int16_t)image_width;
    *height = (int16_t)image_height;
    return &entry->texture;
}

static void pc_release_menu_textures(PcMenuTextureCache *cache)
{
    size_t index;

    if (cache == NULL) {
        return;
    }
    for (index = 0; index < cache->count; ++index) {
        free(cache->textures[index].pixels);
    }
    memset(cache, 0, sizeof(*cache));
}

static const char *pc_save_result_name(JPBSaveResult result)
{
    switch (result) {
    case JPB_SAVE_OK:
        return "ok";
    case JPB_SAVE_NOT_FOUND:
        return "not found";
    case JPB_SAVE_INVALID_DATA:
        return "invalid data";
    case JPB_SAVE_IO_ERROR:
        return "I/O error";
    case JPB_SAVE_BAD_ARGUMENT:
        return "bad argument";
    default:
        return "unknown error";
    }
}

static int pc_configure_persistence_directory(
    PcInput *input, const char *save_directory)
{
    int written;

    if (input == NULL || save_directory == NULL ||
        *save_directory == '\0') {
        return 0;
    }
    if (!CreateDirectoryA(save_directory, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        return 0;
    }
    written = snprintf(
        input->saveGamePath,
        sizeof(input->saveGamePath),
        "%s\\Game",
        save_directory);
    if (written < 0 ||
        (size_t)written >= sizeof(input->saveGamePath)) {
        return 0;
    }
    written = snprintf(
        input->optionsPath,
        sizeof(input->optionsPath),
        "%s\\Options",
        save_directory);
    if (written < 0 ||
        (size_t)written >= sizeof(input->optionsPath)) {
        return 0;
    }
    input->persistenceEnabled = 1;
    return 1;
}

static int pc_configure_persistence(PcInput *input)
{
    char module_path[MAX_PATH];
    char save_directory[MAX_PATH];
    char *slash;
    char *forward_slash;
    DWORD length;
    int written;

    if (input == NULL) {
        return 0;
    }
    length = GetModuleFileNameA(
        NULL, module_path, (DWORD)sizeof(module_path));
    if (length == 0 || length >= sizeof(module_path)) {
        return 0;
    }
    slash = strrchr(module_path, '\\');
    forward_slash = strrchr(module_path, '/');
    if (forward_slash != NULL &&
        (slash == NULL || forward_slash > slash)) {
        slash = forward_slash;
    }
    if (slash == NULL) {
        return 0;
    }
    *slash = '\0';
    written = snprintf(
        save_directory,
        sizeof(save_directory),
        "%s\\SAVEDATA0",
        module_path);
    if (written < 0 || (size_t)written >= sizeof(save_directory)) {
        return 0;
    }
    return pc_configure_persistence_directory(
        input, save_directory);
}

static int pc_get_module_directory(
    char *directory, size_t directory_capacity)
{
    char *slash;
    char *forward_slash;
    DWORD length;

    if (directory == NULL || directory_capacity == 0 ||
        directory_capacity > (size_t)UINT32_MAX) {
        return 0;
    }
    length = GetModuleFileNameA(
        NULL, directory, (DWORD)directory_capacity);
    if (length == 0 || (size_t)length >= directory_capacity) {
        return 0;
    }
    slash = strrchr(directory, '\\');
    forward_slash = strrchr(directory, '/');
    if (forward_slash != NULL &&
        (slash == NULL || forward_slash > slash)) {
        slash = forward_slash;
    }
    if (slash == NULL) {
        return 0;
    }
    *slash = '\0';
    return 1;
}

static int pc_set_asset_root_from_world_path(const char *world_path)
{
    char normalized[MAX_PATH];
    char *marker;
    size_t length;
    size_t index;

    if (world_path == NULL) {
        return 0;
    }
    length = strlen(world_path);
    if (length == 0 || length >= sizeof(normalized)) {
        return 0;
    }
    memcpy(normalized, world_path, length + 1u);
    for (index = 0; index < length; ++index) {
        if (normalized[index] == '/') {
            normalized[index] = '\\';
        }
    }
    marker = normalized;
    while ((marker = strchr(marker, '\\')) != NULL) {
        if (_strnicmp(marker, "\\res\\level\\jpx\\", 15) == 0) {
            *marker = '\0';
            if (snprintf(
                    pc_asset_root,
                    sizeof(pc_asset_root),
                    "%s",
                    normalized) < 0) {
                pc_asset_root[0] = '\0';
                return 0;
            }
            return 1;
        }
        ++marker;
    }
    return 0;
}

static int pc_get_asset_directory(
    char *directory, size_t directory_capacity)
{
    int written;

    if (pc_asset_root[0] == '\0') {
        return pc_get_module_directory(
            directory, directory_capacity);
    }
    written = snprintf(
        directory, directory_capacity, "%s", pc_asset_root);
    return written >= 0 && (size_t)written < directory_capacity;
}

static int pc_default_asset_path(
    char *path,
    size_t path_capacity,
    const char *directory,
    const char *relative_path)
{
    DWORD attributes;
    int written;

    if (path == NULL || path_capacity == 0 || directory == NULL ||
        relative_path == NULL) {
        return 0;
    }
    written = snprintf(
        path, path_capacity, "%s\\%s", directory, relative_path);
    if (written < 0 || (size_t)written >= path_capacity) {
        return 0;
    }
    attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static const char *pc_canonical_level_name(const char *level_name)
{
    size_t index;

    if (level_name == NULL || *level_name == '\0') {
        return NULL;
    }
    for (index = 0; index < JPB_LEVEL_NAME_COUNT; ++index) {
        if (_stricmp(level_name, sLevelNames[index]) == 0) {
            return sLevelNames[index];
        }
    }
    return NULL;
}

static int pc_configure_level_asset(
    char *mesh_path,
    size_t mesh_path_capacity,
    const char *level_name)
{
    char directory[MAX_PATH];
    char relative_path[MAX_PATH];
    const char *canonical_name =
        pc_canonical_level_name(level_name);
    int written;

    if (mesh_path == NULL || canonical_name == NULL ||
        !pc_get_asset_directory(directory, sizeof(directory))) {
        return 0;
    }
    written = snprintf(
        relative_path,
        sizeof(relative_path),
        "res\\level\\jpx\\%s\\%s.jpx",
        canonical_name,
        canonical_name);
    return written >= 0 &&
        (size_t)written < sizeof(relative_path) &&
        pc_default_asset_path(
            mesh_path,
            mesh_path_capacity,
            directory,
            relative_path);
}

static int pc_configure_default_assets(PcDefaultAssets *assets)
{
    char directory[MAX_PATH];

    if (assets == NULL ||
        !pc_get_asset_directory(directory, sizeof(directory))) {
        return 0;
    }
    return
        pc_configure_level_asset(
            assets->mesh,
            sizeof(assets->mesh),
            "fed") &&
        pc_default_asset_path(
            assets->cad,
            sizeof(assets->cad),
            directory,
            "res\\animation\\obi_wan.cad") &&
        pc_default_asset_path(
            assets->bmd,
            sizeof(assets->bmd),
            directory,
            "res\\MODEL\\obi_wan.bmd") &&
        pc_default_asset_path(
            assets->cmb,
            sizeof(assets->cmb),
            directory,
            "res\\combo\\obi_wan.cmb") &&
        pc_default_asset_path(
            assets->enemyCad,
            sizeof(assets->enemyCad),
            directory,
            "res\\animation\\battle_d.cad") &&
        pc_default_asset_path(
            assets->enemyBmd,
            sizeof(assets->enemyBmd),
            directory,
            "res\\MODEL\\battle_d.bmd");
}

static int pc_configure_player_assets(
    PcPlayerAssets *assets, int model_id)
{
    char directory[MAX_PATH];
    char relative_path[MAX_PATH];
    const char *model_name;
    int written;

    if (assets == NULL || model_id < 0 ||
        model_id >= JPB_MODEL_NAME_COUNT ||
        !pc_get_asset_directory(directory, sizeof(directory))) {
        return 0;
    }
    model_name = sModelNames[model_id];
    written = snprintf(
        relative_path,
        sizeof(relative_path),
        "res\\animation\\%s.cad",
        model_name);
    if (written < 0 || (size_t)written >= sizeof(relative_path) ||
        !pc_default_asset_path(
            assets->cad,
            sizeof(assets->cad),
            directory,
            relative_path)) {
        return 0;
    }
    written = snprintf(
        relative_path,
        sizeof(relative_path),
        "res\\MODEL\\%s.bmd",
        model_name);
    if (written < 0 || (size_t)written >= sizeof(relative_path) ||
        !pc_default_asset_path(
            assets->bmd,
            sizeof(assets->bmd),
            directory,
            relative_path)) {
        return 0;
    }
    written = snprintf(
        relative_path,
        sizeof(relative_path),
        "res\\combo\\%s.cmb",
        model_name);
    return written >= 0 &&
        (size_t)written < sizeof(relative_path) &&
        pc_default_asset_path(
            assets->cmb,
            sizeof(assets->cmb),
            directory,
            relative_path);
}

static JPBPCAudio *pc_create_current_game_audio(
    const char *world_path,
    const char *player_one_cad_path,
    int level_index,
    int enable_output,
    unsigned *generation_count)
{
    PcPlayerAssets player_two_assets = {0};
    const char *player_two_cad_path = NULL;
    JPBPCAudio *audio;

    if (GameStruct.NumPlayers == 2) {
        if (!pc_configure_player_assets(
                &player_two_assets,
                GameStruct.ModelSelect[1])) {
            jpb_PCLog(
                "audio initialization failed stage=resolve-p2-assets "
                "model=%d",
                (int)GameStruct.ModelSelect[1]);
            return NULL;
        }
        player_two_cad_path = player_two_assets.cad;
    }
    audio = jpb_PCAudioCreate(
        world_path,
        player_one_cad_path,
        player_two_cad_path,
        level_index,
        enable_output);
    if (audio != NULL && generation_count != NULL) {
        ++*generation_count;
    }
    return audio;
}

/*
 * Mini2's exact ai_Kadu owner always mounts both world player slots.  In a
 * one-player race it drives rider one as the CPU opponent, but that rider
 * still owns the selected character's model, animation, and combo records.
 * Keep GameStruct.NumPlayers at one so ai_Kadu retains its authored CPU path.
 */
static int pc_activate_solo_kadu_rider(
    JPBGameRuntime *runtime,
    const char *cad_path,
    const char *bmd_path,
    const char *cmb_path,
    int player_model_id)
{
    int result;

    if (runtime == NULL ||
        (int)(uint8_t)GameStruct.CurrentLevel != 12 ||
        GameStruct.NumPlayers != 1 ||
        runtime->secondPlayerState != NULL) {
        return JPB_GAME_RUNTIME_OK;
    }
    if (cad_path == NULL || bmd_path == NULL || cmb_path == NULL) {
        return JPB_GAME_RUNTIME_INVALID_ARGUMENT;
    }
    menu_setPlayer(1, (unsigned)player_model_id);
    result = jpb_GameRuntimeActivateSecondPlayer(
        runtime, cad_path, bmd_path, player_model_id);
    if (result == JPB_GAME_RUNTIME_OK) {
        result = jpb_GameRuntimeAddSecondPlayerComboData(
            runtime, cmb_path);
    }
    /* Activating the actor does not change this value, but republish the
     * exact one-player ownership after all rider setup callbacks. */
    menu_setNumPlayers(1);
    return result;
}

static int pc_front_end_requests_gameplay(
    int *level_index,
    int *game_mode)
{
    unsigned stack = menuVars.menuModeSP & 7u;
    unsigned mode = menuVars.menuMode[stack];

    /*
     * The recovered New Game streams return from difficulty to state 4,
     * while the independently recovered character selector is state 0x0e.
     * Keep this PC presentation bridge outside menu_mainLoop so the matched
     * menu owner and its documented evidence boundary remain unchanged.
     */
    if (mode == 4 && stack != 0 &&
        (menuVars.menuMode[(stack - 1u) & 7u] == 0x37 ||
         menuVars.menuMode[(stack - 1u) & 7u] == 0x99)) {
        jpb_PCLog(
            "front end bridge: state 4 predecessor=%u -> character select",
            (unsigned)menuVars.menuMode[(stack - 1u) & 7u]);
        menu_initPlayerSelect();
        menu_pushMenu(0x0e);
        return 0;
    }
    if (GameStruct.gameMode == 6 && GameStruct.inMenuFlag == 0) {
        int selected = 0;

        if (SaveGameStruct.validFlag != 0 &&
            SaveGameStruct.lastlevel >= 1 &&
            SaveGameStruct.lastlevel <= 14) {
            selected = SaveGameStruct.lastlevel;
        } else if (GameStruct.CurrentLevel >= 1 &&
                   GameStruct.CurrentLevel <= 14) {
            selected = (int)(uint8_t)GameStruct.CurrentLevel;
        } else if (LevelSelect >= 1 && LevelSelect <= 14) {
            selected = (int)(uint8_t)LevelSelect;
        }
        if (selected == 0) {
            return 0;
        }
        if (level_index != NULL) {
            *level_index = selected;
        }
        if (game_mode != NULL) {
            *game_mode = 6;
        }
        return 1;
    }
    if (mode != 0x66 || LevelSelect < 1 || LevelSelect > 14) {
        return 0;
    }
    if (level_index != NULL) {
        *level_index = (int)(uint8_t)LevelSelect;
    }
    if (game_mode != NULL) {
        *game_mode = 4;
    }
    return 1;
}

static int pc_front_end_requests_training(int *level_index)
{
    unsigned stack = menuVars.menuModeSP & 7u;
    unsigned mode = menuVars.menuMode[stack];
    int selected = (int)(uint8_t)LevelSelect;

    if (mode != 0x0c || GameStruct.gameMode != 2 ||
        selected < 16 || selected > 22) {
        return 0;
    }
    if (level_index != NULL) {
        *level_index = selected;
    }
    return 1;
}

static int pc_front_end_requests_versus(int *level_index)
{
    unsigned stack = menuVars.menuModeSP & 7u;
    unsigned mode = menuVars.menuMode[stack];
    int selected = (int)(uint8_t)LevelSelect;

    if (mode != 0x0d || selected != 25 ||
        GameStruct.NumPlayers != 2 ||
        GameStruct.gameMode != 2 ||
        GameStruct.versusModeFlag == 0) {
        return 0;
    }
    if (level_index != NULL) {
        *level_index = selected;
    }
    return 1;
}

static void pc_release_front_end_gameplay_control(
    JPBGameRuntime *runtime,
    int selected_players)
{
    uint32_t p1_before = 0;
    uint32_t p2_before = 0;
    uint32_t p1_after = 0;
    uint32_t p2_after = 0;
    int override_before = 0;
    int override_after = 0;
    int current_dolly = -1;
    int p1_ai_before = 0;
    int p1_ai_after = 0;

    if (runtime == NULL) {
        return;
    }
    if ((uint8_t)GameStruct.CurrentLevel == 1 &&
        selected_players == 1 &&
        ((runtime->player != NULL &&
          (runtime->player->pFlags & UINT32_C(2)) != 0) ||
         (runtime->world != NULL &&
          runtime->world->overRideDolly != 0))) {
        jpb_PCLog(
            "front-end gameplay control preserved authored FED intro "
            "pflags=%08x dolly=%d override=%d",
            runtime->player != NULL
                ? (unsigned)runtime->player->pFlags
                : 0u,
            runtime->world != NULL
                ? (int)runtime->world->currentDolly
                : -1,
            runtime->world != NULL
                ? (int)runtime->world->overRideDolly
                : 0);
        return;
    }
    if (runtime->player != NULL) {
        p1_before = runtime->player->pFlags;
        p1_ai_before = runtime->player->pEnemy != NULL;
        if (runtime->player->pEnemy != NULL) {
            runtime->player->pEnemy->exit_flag = 1;
            if (runtime->player->paMotions != NULL &&
                runtime->player->maxMotions > 2) {
                runtime->player->paMotions[2].Lock = 0x19;
            }
            runtime->player->pEnemy = NULL;
        }
        runtime->player->pFlags &= ~UINT32_C(0x10);
        runtime->player->pFlags &= ~UINT32_C(2);
        p1_after = runtime->player->pFlags;
        p1_ai_after = runtime->player->pEnemy != NULL;
    }
    if (selected_players == 2 && runtime->inactivePlayer != NULL) {
        p2_before = runtime->inactivePlayer->pFlags;
        runtime->inactivePlayer->pFlags &= ~UINT32_C(2);
        p2_after = runtime->inactivePlayer->pFlags;
    }
    if (runtime->world != NULL) {
        override_before = runtime->world->overRideDolly;
        current_dolly = runtime->world->currentDolly;
        runtime->world->overRideDolly = 0;
        runtime->world->currentDolly = 0;
        runtime->authoredCameraDolly = 0;
        if (current_dolly >= 0 && current_dolly < 256) {
            runtime->world->aDolly[current_dolly].flags &=
                ~UINT32_C(0x400);
        }
        runtime->world->aDolly[0].flags &= ~UINT32_C(0x400);
        override_after = runtime->world->overRideDolly;
    }
    game_clearLetterBox();
    GameStruct.screenShotFlag = 0;
    newcameraflag = 1;
    if (p1_before != p1_after || p2_before != p2_after ||
        p1_ai_before != p1_ai_after ||
        override_before != override_after) {
        jpb_PCLog(
            "front-end gameplay control released players=%d "
            "pflags=%08x/%08x->%08x/%08x p1_ai=%d->%d "
            "dolly=%d override=%d->%d",
            selected_players,
            (unsigned)p1_before,
            (unsigned)p2_before,
            (unsigned)p1_after,
            (unsigned)p2_after,
            p1_ai_before,
            p1_ai_after,
            current_dolly,
            override_before,
            override_after);
    }
}

static int pc_start_selected_gameplay(
    JPBGameRuntime *runtime,
    const char *mesh_path,
    PcPlayerAssets *player_assets,
    const char *enemy_cad_path,
    const char *enemy_bmd_path,
    int selected_level,
    int selected_game_mode,
    PcInput *input)
{
    optionstruct selected_options = OptionStruct;
    PcPlayerAssets second_player_assets = {0};
    int selected_model = GameStruct.ModelSelect[0];
    int selected_player_two = GameStruct.ModelSelect[1];
    int selected_players = GameStruct.NumPlayers;
    int selected_difficulty = GameStruct.difficulty;
    int selected_versus = GameStruct.versusModeFlag;
    uint8_t selected_player_index[2] = {
        menuVars.pplayers[0], menuVars.pplayers[1]
    };
    int result;

    jpb_PCLogSetCheckpoint(
        "gameplay handoff enter level=%d mode=%d players=%d models=%d/%d",
        selected_level,
        selected_game_mode,
        selected_players,
        selected_model,
        selected_player_two);
    jpb_PCLog(
        "gameplay handoff begin level=%d name=%s mode=%d players=%d "
        "models=%d/%d difficulty=%d versus=%d mesh=%s",
        selected_level,
        selected_level >= 0 && selected_level < JPB_LEVEL_NAME_COUNT
            ? sLevelNames[selected_level]
            : "<invalid>",
        selected_game_mode,
        selected_players,
        selected_model,
        selected_player_two,
        selected_difficulty,
        selected_versus,
        mesh_path != NULL ? mesh_path : "<null>");

    if (input != NULL) {
        /*
         * Arm this before runtime construction: player initialization can
         * sample the still-held menu confirmation.  The retail loading
         * owner clears that edge before it constructs controllable actors.
         */
        input->gameplayHandoffReleaseMask =
            selected_players == 2 ? UINT8_C(3) : UINT8_C(1);
    }

    if (!pc_configure_player_assets(
            player_assets, selected_model)) {
        jpb_PCLog(
            "gameplay handoff failed stage=resolve-p1-assets model=%d",
            selected_model);
        fprintf(
            stderr,
            "selected player assets are not installed: model=%d\n",
            selected_model);
        return JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    jpb_PCLogSetCheckpoint(
        "gameplay handoff p1 assets resolved model=%d",
        selected_model);
    LevelSelect = (char)selected_level;
    menu_levelSelect();
    jpb_PCLogSetCheckpoint(
        "gameplay handoff menu level-select complete level=%d",
        selected_level);
    jpb_GameRuntimeShutdown(runtime);
    jpb_PCLogSetCheckpoint(
        "gameplay handoff previous runtime shutdown level=%d",
        selected_level);
    result = jpb_GameRuntimeInitWithPlayerAssets(
        runtime,
        mesh_path,
        player_assets->cad,
        player_assets->bmd,
        selected_model);
    if (result != JPB_GAME_RUNTIME_OK) {
        jpb_PCLog(
            "gameplay handoff failed stage=runtime-init status=%d "
            "runtime_stage=%s model=%d cad=%s bmd=%s",
            result,
            jpb_GameRuntimeLastFailureStage(),
            selected_model,
            player_assets->cad,
            player_assets->bmd);
        return result;
    }
    jpb_PCLogSetCheckpoint(
        "gameplay handoff runtime initialized level=%d model=%d",
        selected_level,
        selected_model);
    menu_setNumPlayers((unsigned)selected_players);
    menu_setPlayer(0, (unsigned)selected_model);
    if (selected_players == 2) {
        menu_setPlayer(1, (unsigned)selected_player_two);
    }
    GameStruct.versusModeFlag = (int16_t)selected_versus;
    /* Runtime construction restores default options. Reapply an explicit
     * diagnostic scheme before the first controllable gameplay frame. */
    pc_apply_control_scheme_overrides(input);
    result = jpb_GameRuntimeAddPlayerComboData(
        runtime, player_assets->cmb);
    if (result != JPB_GAME_RUNTIME_OK) {
        jpb_PCLog(
            "gameplay handoff failed stage=p1-combos status=%d path=%s",
            result,
            player_assets->cmb);
        return result;
    }
    jpb_PCLogSetCheckpoint(
        "gameplay handoff p1 combos initialized model=%d",
        selected_model);
    if (selected_players == 2) {
        if (!pc_configure_player_assets(
                &second_player_assets,
                selected_player_two)) {
            jpb_PCLog(
                "gameplay handoff failed stage=resolve-p2-assets model=%d",
                selected_player_two);
            fprintf(
                stderr,
                "selected player-two assets are not installed: model=%d\n",
                selected_player_two);
            return JPB_GAME_RUNTIME_LOAD_FAILED;
        }
        result = jpb_GameRuntimeActivateSecondPlayer(
            runtime,
            second_player_assets.cad,
            second_player_assets.bmd,
            selected_player_two);
        if (result != JPB_GAME_RUNTIME_OK) {
            jpb_PCLog(
                "gameplay handoff failed stage=activate-p2 status=%d "
                "model=%d cad=%s bmd=%s",
                result,
                selected_player_two,
                second_player_assets.cad,
                second_player_assets.bmd);
            return result;
        }
        jpb_PCLogSetCheckpoint(
            "gameplay handoff p2 actor initialized model=%d",
            selected_player_two);
        result = jpb_GameRuntimeAddSecondPlayerComboData(
            runtime, second_player_assets.cmb);
        if (result != JPB_GAME_RUNTIME_OK) {
            jpb_PCLog(
                "gameplay handoff failed stage=p2-combos status=%d path=%s",
                result,
                second_player_assets.cmb);
            return result;
        }
        jpb_PCLogSetCheckpoint(
            "gameplay handoff p2 combos initialized model=%d",
            selected_player_two);
    } else if (selected_level == 12) {
        result = pc_activate_solo_kadu_rider(
            runtime,
            player_assets->cad,
            player_assets->bmd,
            player_assets->cmb,
            selected_model);
        if (result != JPB_GAME_RUNTIME_OK) {
            jpb_PCLog(
                "gameplay handoff failed stage=solo-kadu status=%d "
                "model=%d",
                result,
                selected_model);
            return result;
        }
    }
    if (enemy_cad_path != NULL) {
        result = jpb_GameRuntimeAddEnemyAssets(
            runtime, enemy_cad_path, enemy_bmd_path);
        if (result != JPB_GAME_RUNTIME_OK) {
            jpb_PCLog(
                "gameplay handoff failed stage=enemy-assets status=%d "
                "cad=%s bmd=%s",
                result,
                enemy_cad_path,
                enemy_bmd_path != NULL ? enemy_bmd_path : "<null>");
            return result;
        }
        jpb_PCLogSetCheckpoint(
            "gameplay handoff enemy assets initialized level=%d",
            selected_level);
    }

    jpb_PCLogSetCheckpoint(
        "gameplay handoff publishing selected state level=%d",
        selected_level);
    OptionStruct = selected_options;
    generateAllText(OptionStruct.Language);
    menuVars.pplayers[0] = selected_player_index[0];
    menuVars.pplayers[1] = selected_player_index[1];
    menu_setNumPlayers((unsigned)selected_players);
    menu_setPlayer(0, (unsigned)selected_model);
    if (selected_players == 2) {
        menu_setPlayer(1, (unsigned)selected_player_two);
    }
    GameStruct.versusModeFlag = (int16_t)selected_versus;
    (void)jpb_game_ApplyLevelDifficulty(
        0,
        selected_difficulty == 0 ? 0 : 1);
    GameStruct.CurrentLevel = (char)selected_level;
    GameStruct.gameMode = (char)selected_game_mode;
    GameStruct.inMenuFlag = 0;
    pc_release_front_end_gameplay_control(runtime, selected_players);
    jpb_PCLogSetCheckpoint(
        "gameplay handoff complete level=%d mode=%d players=%d models=%d/%d",
        selected_level,
        selected_game_mode,
        selected_players,
        selected_model,
        selected_player_two);
    jpb_PCLog(
        "gameplay handoff complete level=%d mode=%d players=%d models=%d/%d",
        selected_level,
        selected_game_mode,
        selected_players,
        selected_model,
        selected_player_two);
    return JPB_GAME_RUNTIME_OK;
}

static void pc_save_game_data(void *user_data)
{
    PcInput *input = (PcInput *)user_data;
    JPBSaveResult result;

    if (input == NULL || !input->persistenceEnabled) {
        return;
    }
    result = jpb_SaveGameWriteFile(input->saveGamePath);
    if (result != JPB_SAVE_OK) {
        fprintf(
            stderr,
            "game save failed (%s): %s\n",
            pc_save_result_name(result),
            input->saveGamePath);
    } else {
        ++input->gameSaveWriteCount;
    }
}

static int pc_validate_persistence_round_trip(PcInput *input)
{
    optionstruct expected;
    JPBSaveResult write_result;
    JPBSaveResult read_result;

    if (input == NULL || !input->persistenceEnabled) {
        return 0;
    }
    write_result = jpb_SaveOptionsWriteFile(input->optionsPath);
    if (write_result != JPB_SAVE_OK) {
        jpb_PCLog(
            "persistence validation failed stage=write-options "
            "result=%s path=%s",
            pc_save_result_name(write_result),
            input->optionsPath);
        return 0;
    }
    expected = OptionStruct;
    memset(&OptionStruct, 0xa5, sizeof(OptionStruct));
    read_result = jpb_SaveOptionsReadFile(input->optionsPath);
    if (read_result != JPB_SAVE_OK ||
        memcmp(&OptionStruct, &expected, sizeof(OptionStruct)) != 0) {
        jpb_PCLog(
            "persistence validation failed stage=read-options "
            "result=%s path=%s",
            pc_save_result_name(read_result),
            input->optionsPath);
        OptionStruct = expected;
        return 0;
    }
    jpb_PCLog(
        "persistence validation complete options=%s game_writes=%u",
        input->optionsPath,
        input->gameSaveWriteCount);
    return 1;
}

static unsigned pc_controller_count(void *user_data)
{
    PcInput *input = (PcInput *)user_data;
    unsigned count;

    if (input == NULL || input->headless) {
        return 0;
    }
    count = jpb_PCXInputConnectedCount(&input->xinput);
    padExist = (uint8_t)((count >= 1 ? 1u : 0u) |
                         (count >= 2 ? 2u : 0u));
    return count;
}

static void pc_refresh_controller_ownership(PcInput *input)
{
    uint32_t connected_mask;
    uint32_t added_mask;
    int player_two_user;
    unsigned selected_user;

    if (input == NULL || input->headless) {
        return;
    }
    connected_mask = input->xinput.connectedMask;
    added_mask = connected_mask & ~input->previousControllerMask;
    input->previousControllerMask = connected_mask;
    player_two_user = input->playerTwoControllerSlot != 0
        ? (int)input->playerTwoControllerSlot - 1
        : -1;
    if (GameStruct.NumPlayers != 2 ||
        player_two_user < 0 ||
        (connected_mask &
         (UINT32_C(1) << player_two_user)) == 0) {
        if (GameStruct.NumPlayers == 2 &&
            player_two_user >= 0 &&
            (connected_mask &
             (UINT32_C(1) << player_two_user)) == 0) {
            P2Disconnected();
            jpb_PCLog(
                "controller disconnected P2=xinput%d; "
                "waiting for controller reattach",
                player_two_user);
        }
        input->playerTwoControllerSlot = 0;
        player_two_user = -1;
    }
    if (p2Disconnected != 0 && added_mask != 0 &&
        jpb_PCChoosePlayerTwoAddedUser(
            input->playerOneUsesKeyboard,
            connected_mask,
            added_mask,
            &selected_user)) {
        p2Disconnected = 0;
        if (GameStruct.NumPlayers == 2) {
            input->playerTwoControllerSlot = selected_user + 1u;
            player_two_user = (int)selected_user;
            padExist |= 2u;
        }
        jpb_PCLog(
            "controller reattached P2=xinput%u",
            selected_user);
    }
    if (GameStruct.NumPlayers == 2 && player_two_user < 0 &&
        p2Disconnected == 0 &&
        jpb_PCChoosePlayerTwoUser(
            input->playerOneUsesKeyboard,
            connected_mask,
            &selected_user)) {
        input->playerTwoControllerSlot = selected_user + 1u;
        player_two_user = (int)selected_user;
        jpb_PCLog(
            "controller ownership assigned P2=xinput%u P1=%s/fallback",
            selected_user,
            input->playerOneUsesKeyboard ? "keyboard" : "xinput");
    }
    p2Connected = player_two_user >= 0;
    player2InputType = p2Connected ? 1 : 0;
}

static const char *pc_controller_name(
    unsigned player, void *user_data)
{
    PcInput *input = (PcInput *)user_data;
    unsigned user_index;

    (void)pc_controller_count(user_data);
    pc_refresh_controller_ownership(input);
    return input != NULL &&
        jpb_PCControllerUserForPlayer(
            player,
            GameStruct.NumPlayers,
            input->playerTwoControllerSlot != 0
                ? (int)input->playerTwoControllerSlot - 1
                : -1,
            input->xinput.connectedMask,
            &user_index)
        ? "Xbox Series X Controller"
        : NULL;
}

static void pc_set_rumble(
    int32_t controller_index,
    uint16_t low_frequency,
    uint16_t high_frequency,
    uint32_t duration_ms,
    void *user_data)
{
    PcInput *input = (PcInput *)user_data;
    unsigned user_index;

    if (input == NULL || input->headless ||
        controller_index < 0 || controller_index > 1) {
        return;
    }
    (void)jpb_PCXInputConnectedCount(&input->xinput);
    pc_refresh_controller_ownership(input);
    if (!jpb_PCControllerUserForPlayer(
            (unsigned)controller_index,
            GameStruct.NumPlayers,
            input->playerTwoControllerSlot != 0
                ? (int)input->playerTwoControllerSlot - 1
                : -1,
            input->xinput.connectedMask,
            &user_index)) {
        return;
    }
    jpb_PCXInputSetRumbleUser(
        &input->xinput,
        user_index,
        low_frequency,
        high_frequency,
        duration_ms);
}

static LONG WINAPI pc_headless_exception_filter(
    EXCEPTION_POINTERS *exception)
{
    jpb_PCLogException(exception);
    if (exception != NULL &&
        exception->ExceptionRecord != NULL) {
        uintptr_t module_base =
            (uintptr_t)GetModuleHandleA(NULL);
        uintptr_t exception_address =
            (uintptr_t)
                exception->ExceptionRecord->ExceptionAddress;
        uintptr_t return_address = 0;

#if defined(_M_X64) && defined(_MSC_VER)
        if (exception->ContextRecord != NULL) {
            __try {
                return_address = *(const uintptr_t *)(uintptr_t)
                    exception->ContextRecord->Rsp;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return_address = 0;
            }
        }
#endif

        fprintf(
            stderr,
            "unhandled exception code=0x%08lx address=%p rva=0x%zx "
            "return_rva=0x%zx\n",
            (unsigned long)
                exception->ExceptionRecord->ExceptionCode,
            exception->ExceptionRecord->ExceptionAddress,
            exception_address >= module_base
                ? (size_t)(exception_address - module_base)
                : (size_t)0,
            return_address >= module_base
                ? (size_t)(return_address - module_base)
                : (size_t)0);
        fflush(stderr);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

static void pc_configure_failure_mode(void)
{
    UINT error_mode =
        GetErrorMode() |
        SEM_FAILCRITICALERRORS |
        SEM_NOGPFAULTERRORBOX |
        SEM_NOOPENFILEERRORBOX;

    SetErrorMode(error_mode);
    SetUnhandledExceptionFilter(
        pc_headless_exception_filter);
#if defined(_MSC_VER)
    /*
     * Failures must report through the log/exit status, never through a
     * desktop CRT/WER dialog that can outlive either the game or a test.
     */
    (void)_set_abort_behavior(
        0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
}

enum PcLiveMenuKeyBits {
    PC_LIVE_MENU_KEY_J = 1u << 0,
    PC_LIVE_MENU_KEY_K = 1u << 1,
    PC_LIVE_MENU_KEY_L = 1u << 2,
    PC_LIVE_MENU_KEY_SPACE = 1u << 3,
    PC_LIVE_MENU_KEY_ENTER = 1u << 4,
    PC_LIVE_MENU_KEY_ESCAPE = 1u << 5
};

static uint32_t pc_live_menu_key_bits(void)
{
    uint32_t bits = 0;

    if ((GetAsyncKeyState('J') & 0x8000) != 0) {
        bits |= PC_LIVE_MENU_KEY_J;
    }
    if ((GetAsyncKeyState('K') & 0x8000) != 0) {
        bits |= PC_LIVE_MENU_KEY_K;
    }
    if ((GetAsyncKeyState('L') & 0x8000) != 0) {
        bits |= PC_LIVE_MENU_KEY_L;
    }
    if ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0) {
        bits |= PC_LIVE_MENU_KEY_SPACE;
    }
    if ((GetAsyncKeyState(VK_RETURN) & 0x8000) != 0) {
        bits |= PC_LIVE_MENU_KEY_ENTER;
    }
    if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {
        bits |= PC_LIVE_MENU_KEY_ESCAPE;
    }
    return bits;
}

static void pc_append_live_key(
    char *buffer,
    size_t capacity,
    const char *name)
{
    size_t used;

    if (buffer == NULL || capacity == 0 || name == NULL) {
        return;
    }
    used = strlen(buffer);
    if (used != 0 && used + 1 < capacity) {
        buffer[used++] = '+';
        buffer[used] = '\0';
    }
    if (used < capacity) {
        (void)snprintf(buffer + used, capacity - used, "%s", name);
    }
}

static void pc_log_live_menu_key_edges(
    uint32_t previous,
    uint32_t current,
    int frame,
    int title_active)
{
    uint32_t pressed = current & ~previous;
    char keys[64] = {0};

    if (pressed == 0) {
        return;
    }
    if ((pressed & PC_LIVE_MENU_KEY_J) != 0) {
        pc_append_live_key(keys, sizeof(keys), "J/select");
    }
    if ((pressed & PC_LIVE_MENU_KEY_K) != 0) {
        pc_append_live_key(keys, sizeof(keys), "K/west");
    }
    if ((pressed & PC_LIVE_MENU_KEY_L) != 0) {
        pc_append_live_key(keys, sizeof(keys), "L/north");
    }
    if ((pressed & PC_LIVE_MENU_KEY_SPACE) != 0) {
        pc_append_live_key(keys, sizeof(keys), "Space/confirm");
    }
    if ((pressed & PC_LIVE_MENU_KEY_ENTER) != 0) {
        pc_append_live_key(keys, sizeof(keys), "Enter/confirm");
    }
    if ((pressed & PC_LIVE_MENU_KEY_ESCAPE) != 0) {
        pc_append_live_key(keys, sizeof(keys), "Escape/back");
    }
    jpb_PCLog(
        "input key edge frame=%d keys=%s raw=%02x mode=%u stack=%u select=%u",
        frame,
        keys,
        (unsigned)pressed,
        title_active
            ? (unsigned)menuVars.menuMode[menuVars.menuModeSP & 7u]
            : UINT_MAX,
        title_active ? (unsigned)menuVars.menuModeSP : UINT_MAX,
        title_active
            ? (unsigned)menuVars.mmSelect1[menuVars.menuModeSP & 7u]
            : UINT_MAX);
}

static void pc_prepare_title_framebuffer(
    JPBSoftwareFramebuffer *framebuffer,
    const uint32_t *title_pixels)
{
    unsigned menu_mode;
    size_t pixel_count;

    if (framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->width <= 0 || framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width) {
        return;
    }
    pixel_count =
        (size_t)framebuffer->width * (size_t)framebuffer->height;
    menu_mode =
        (unsigned)menuVars.menuMode[menuVars.menuModeSP & 7u];
    if (title_pixels != NULL &&
        menu_mode != 0x0e && menu_mode != 0x1a) {
        memcpy(framebuffer->pixels, title_pixels,
               pixel_count * sizeof(*framebuffer->pixels));
        return;
    }
    memset(framebuffer->pixels, 0,
           pixel_count * sizeof(*framebuffer->pixels));
}

static uint32_t pc_filter_gameplay_handoff_input(
    PcInput *input,
    unsigned pad_index,
    uint32_t bits)
{
    uint8_t player_mask;
    uint32_t allowed;

    if (input == NULL || pad_index > 1) {
        return bits;
    }
    player_mask = (uint8_t)(1u << pad_index);
    if ((input->gameplayHandoffReleaseMask & player_mask) == 0) {
        return bits;
    }
    if (bits == 0) {
        input->gameplayHandoffReleaseMask &= (uint8_t)~player_mask;
        return 0;
    }

    allowed = bits & (
        JPB_PAD_UP | JPB_PAD_LEFT | JPB_PAD_DOWN |
        JPB_PAD_RIGHT | JPB_PAD_ANALOG_MOVEMENT);
    if (allowed != 0) {
        input->gameplayHandoffReleaseMask &= (uint8_t)~player_mask;
        return allowed;
    }
    return 0;
}

static uint32_t pc_read_pad(int32_t pad_index, void *user_data)
{
    PcInput *input = (PcInput *)user_data;
    uint32_t bits = 0;
    uint32_t controller_bits = 0;
    float controller_x = 0.0f;
    float controller_y = 0.0f;
    float keyboard_x = 0.0f;
    float keyboard_y = 0.0f;
    unsigned controller_user = 0;
    int read_controller;
    int controller_connected;

    if (pad_index < 0 || pad_index > 1) {
        return 0;
    }
    if (input->headless || input->scriptedInput) {
        uint32_t headless_bits = input->headlessBits;
        float headless_x = 0.0f;
        float headless_y = 0.0f;

        if (input->phaseCount != 0) {
            headless_bits = input->headlessPhaseBits;
            if (input->headlessPhaseIndependentPlayers &&
                pad_index == 1) {
                headless_bits = input->headlessPhasePlayerTwoBits;
            }
        }
        if (pad_index == 0 &&
            (input->headlessPhaseKeyboardMask & UINT8_C(1)) != 0) {
            headless_bits = jpb_PCMapKeyboard(
                &input->headlessPhaseKeyboard,
                GameStruct.inMenuFlag,
                GameStruct.gameMode,
                &headless_x,
                &headless_y);
            input->playerOneUsesKeyboard = 1;
            player1InputType = 0;
            lastUsedInputType = 0;
            g_p1X = headless_x;
            g_p1Y = headless_y;
        } else if ((input->headlessPhaseXInputMask &
             (uint8_t)(1u << (unsigned)pad_index)) != 0) {
            if (pad_index == 0 && input->playerOneUsesKeyboard) {
                player1InputType = 0;
                g_p1X = 0.0f;
                g_p1Y = 0.0f;
                return 0;
            }
            headless_bits = jpb_PCXInputMapGamepad(
                &input->headlessPhaseGamepads[pad_index],
                OptionStruct.WalkLimit[pad_index],
                OptionStruct.RunLimit[pad_index],
                OptionStruct.ControllerConfig[pad_index],
                GameStruct.inMenuFlag != 0,
                &headless_x,
                &headless_y);
            if (pad_index == 0) {
                player1InputType = 1;
                g_p1X = headless_x;
                g_p1Y = headless_y;
            } else {
                player2InputType = 1;
                g_p2X = headless_x;
                g_p2Y = headless_y;
            }
            lastUsedInputType = 1;
        } else {
            if ((headless_bits & JPB_PAD_LEFT) != 0) headless_x += 1.0f;
            if ((headless_bits & JPB_PAD_RIGHT) != 0) headless_x -= 1.0f;
            if ((headless_bits & JPB_PAD_UP) != 0) headless_y -= 1.0f;
            if ((headless_bits & JPB_PAD_DOWN) != 0) headless_y += 1.0f;
            if (pad_index == 0) {
                player1InputType = 0;
                g_p1X = headless_x;
                g_p1Y = headless_y;
            } else {
                /* The matched PC owner never assigns P2 to the keyboard. */
                player2InputType = 1;
                g_p2X = headless_x;
                g_p2Y = headless_y;
            }
        }

        if (input->headlessActive) {
            headless_bits = pc_filter_gameplay_handoff_input(
                input, (unsigned)pad_index, headless_bits);

            if (headless_bits == 0) {
                if (pad_index == 0) {
                    g_p1X = 0.0f;
                    g_p1Y = 0.0f;
                } else {
                    g_p2X = 0.0f;
                    g_p2Y = 0.0f;
                }
                return 0;
            }
            input->observedPlayerBits[pad_index] |= headless_bits;
            return headless_bits;
        }
        return 0;
    }
    /*
     * Device ownership follows the recovered Windows arrangement: keyboard
     * is P1-only. Once a two-player flow has assigned P1 to keyboard, the
     * first connected controller belongs to P2 instead of being silently
     * reserved as P1's controller.
     */
    (void)jpb_PCXInputConnectedCount(&input->xinput);
    pc_refresh_controller_ownership(input);
    read_controller = jpb_PCControllerUserForPlayer(
        (unsigned)pad_index,
        GameStruct.NumPlayers,
        input->playerTwoControllerSlot != 0
            ? (int)input->playerTwoControllerSlot - 1
            : -1,
        input->xinput.connectedMask,
        &controller_user);
    controller_connected = read_controller
        ? jpb_PCXInputReadUser(
              &input->xinput,
              controller_user,
              OptionStruct.WalkLimit[pad_index],
              OptionStruct.RunLimit[pad_index],
              OptionStruct.ControllerConfig[pad_index],
              GameStruct.inMenuFlag != 0,
              &controller_bits,
              &controller_x,
              &controller_y)
        : 0;
    if (pad_index == 0) {
        player1InputType = controller_connected ? 1 : 0;
        g_p1X = controller_x;
        g_p1Y = controller_y;
    } else {
        player2InputType = controller_connected ? 1 : 0;
        p2Connected = controller_connected;
        g_p2X = controller_x;
        g_p2Y = controller_y;
    }
    if (pad_index == 1) {
        controller_bits = pc_filter_gameplay_handoff_input(
            input, 1u, controller_bits);
        if (controller_bits == 0) {
            g_p2X = 0.0f;
            g_p2Y = 0.0f;
            return 0;
        }
        if (controller_bits != 0) {
            lastUsedInputType = 1;
        }
        return controller_bits;
    }
    {
        JPBPCGameplayKeyboardState keyboard;

        memset(&keyboard, 0, sizeof(keyboard));
        keyboard.moveUp =
            (GetAsyncKeyState('W') & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
        keyboard.moveLeft =
            (GetAsyncKeyState('A') & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
        keyboard.moveDown =
            (GetAsyncKeyState('S') & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
        keyboard.moveRight =
            (GetAsyncKeyState('D') & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
        keyboard.walkModifier =
            (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0;
        keyboard.zoomIn = (GetAsyncKeyState('T') & 0x8000) != 0;
        keyboard.comboSouth = (GetAsyncKeyState('J') & 0x8000) != 0;
        keyboard.comboWest = (GetAsyncKeyState('K') & 0x8000) != 0;
        keyboard.comboNorth = (GetAsyncKeyState('L') & 0x8000) != 0;
        keyboard.lockOn = (GetAsyncKeyState('H') & 0x8000) != 0;
        keyboard.jumpBlockChord =
            (GetAsyncKeyState('U') & 0x8000) != 0;
        keyboard.northBlockChord =
            (GetAsyncKeyState('I') & 0x8000) != 0;
        keyboard.southBlockChord =
            (GetAsyncKeyState('O') & 0x8000) != 0;
        keyboard.westBlockChord =
            (GetAsyncKeyState('Y') & 0x8000) != 0;
        keyboard.block =
            (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
        keyboard.jump =
            (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0 ||
            (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
        keyboard.start =
            (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        bits = jpb_PCMapKeyboard(
            &keyboard,
            GameStruct.inMenuFlag,
            GameStruct.gameMode,
            &keyboard_x,
            &keyboard_y);
    }
    if (bits != 0) {
        input->playerOneUsesKeyboard = 1;
        pc_refresh_controller_ownership(input);
        lastUsedInputType = 0;
        player1InputType = 0;
        g_p1X = keyboard_x;
        g_p1Y = keyboard_y;
    } else if (controller_bits != 0) {
        if (input->playerOneUsesKeyboard) {
            player1InputType = 0;
            g_p1X = 0.0f;
            g_p1Y = 0.0f;
            controller_bits = 0;
        } else if (GameStruct.NumPlayers != 2) {
            input->playerOneUsesKeyboard = 0;
            lastUsedInputType = 1;
            player1InputType = 1;
            g_p1X = controller_x;
            g_p1Y = controller_y;
        } else {
            lastUsedInputType = 1;
            player1InputType = 1;
            g_p1X = controller_x;
            g_p1Y = controller_y;
        }
    } else if (input->playerOneUsesKeyboard) {
        player1InputType = 0;
        g_p1X = 0.0f;
        g_p1Y = 0.0f;
    }
    bits = jpb_PCSelectPlayerOneInput(bits, controller_bits);
    bits = pc_filter_gameplay_handoff_input(input, 0u, bits);
    if (bits == 0) {
        g_p1X = 0.0f;
        g_p1Y = 0.0f;
        return 0;
    }
    return bits;
}

static uint32_t pc_read_brainutl_cheat_chords(void *user_data)
{
    uint32_t chords = 0;
    int b;
    int down;

    (void)user_data;
    b = (GetAsyncKeyState('B') & 0x8000) != 0;
    down = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
    if (!b || !down) {
        return 0;
    }
    /* Exact SDL scancodes: B=5, H=11, K=14, R=21, Down=81. */
    if ((GetAsyncKeyState('K') & 0x8000) != 0) {
        chords |= JPB_BRAINUTL_CHEAT_BIG_HEAD;
    }
    if ((GetAsyncKeyState('H') & 0x8000) != 0) {
        chords |= JPB_BRAINUTL_CHEAT_BIG_FEET_AND_SABER;
    }
    if ((GetAsyncKeyState('R') & 0x8000) != 0) {
        chords |= JPB_BRAINUTL_CHEAT_SMALL_MODE;
    }
    return chords;
}

typedef struct PcKeyboardMap {
    int virtualKey;
    unsigned sdlScancode;
} PcKeyboardMap;

static const uint8_t *pc_read_keyboard_state(
    size_t *key_count, void *user_data)
{
    static uint8_t state[512];
    static const PcKeyboardMap extra_keys[] = {
        {VK_RETURN, 40}, {VK_ESCAPE, 41}, {VK_BACK, 42}, {VK_TAB, 43},
        {VK_SPACE, 44}, {VK_OEM_MINUS, 45}, {VK_OEM_PLUS, 46},
        {VK_OEM_4, 47}, {VK_OEM_6, 48}, {VK_OEM_5, 49},
        {VK_OEM_1, 51}, {VK_OEM_7, 52}, {VK_OEM_3, 53},
        {VK_OEM_COMMA, 54}, {VK_OEM_PERIOD, 55}, {VK_OEM_2, 56},
        {VK_CAPITAL, 57},
        {VK_F1, 58}, {VK_F2, 59}, {VK_F3, 60}, {VK_F4, 61},
        {VK_F5, 62}, {VK_F6, 63}, {VK_F7, 64}, {VK_F8, 65},
        {VK_F9, 66}, {VK_F10, 67}, {VK_F11, 68}, {VK_F12, 69},
        {VK_SNAPSHOT, 70}, {VK_SCROLL, 71}, {VK_PAUSE, 72},
        {VK_INSERT, 73}, {VK_HOME, 74}, {VK_PRIOR, 75},
        {VK_DELETE, 76}, {VK_END, 77}, {VK_NEXT, 78},
        {VK_RIGHT, 79}, {VK_LEFT, 80}, {VK_DOWN, 81}, {VK_UP, 82},
        {VK_LCONTROL, 224}, {VK_LSHIFT, 225}, {VK_LMENU, 226},
        {VK_LWIN, 227}, {VK_RCONTROL, 228}, {VK_RSHIFT, 229},
        {VK_RMENU, 230}, {VK_RWIN, 231}
    };
    PcInput *input = (PcInput *)user_data;
    unsigned index;

    memset(state, 0, sizeof(state));
    *key_count = sizeof(state);
    if (input == NULL || input->headless) {
        return state;
    }

    /* SDL scancodes use the USB keyboard usage-page ordering. */
    for (index = 0; index < 26; ++index) {
        if ((GetAsyncKeyState('A' + (int)index) & 0x8000) != 0) {
            state[4u + index] = 1;
        }
    }
    for (index = 0; index < 9; ++index) {
        if ((GetAsyncKeyState('1' + (int)index) & 0x8000) != 0) {
            state[30u + index] = 1;
        }
    }
    if ((GetAsyncKeyState('0') & 0x8000) != 0) {
        state[39] = 1;
    }
    for (index = 0;
         index < sizeof(extra_keys) / sizeof(extra_keys[0]);
         ++index) {
        if ((GetAsyncKeyState(extra_keys[index].virtualKey) & 0x8000) != 0) {
            state[extra_keys[index].sdlScancode] = 1;
        }
    }
    return state;
}

static LRESULT CALLBACK pc_window_proc(
    HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    (void)wparam;
    (void)lparam;

    switch (message) {
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        pc_running = 0;
        PostQuitMessage(0);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    default:
        return DefWindowProcA(window, message, wparam, lparam);
    }
}

static HWND pc_create_window(
    HINSTANCE instance,
    int visible,
    int maximize,
    int framebuffer_width,
    int framebuffer_height)
{
    WNDCLASSA window_class;
    RECT rectangle = {0, 0, framebuffer_width, framebuffer_height};
    HWND window;

    memset(&window_class, 0, sizeof(window_class));
    window_class.style = CS_OWNDC;
    window_class.lpfnWndProc = pc_window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    window_class.lpszClassName = "JediPowerBattlesPC";
    if (!RegisterClassA(&window_class) &&
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return NULL;
    }
    AdjustWindowRect(&rectangle, WS_OVERLAPPEDWINDOW, FALSE);
    window = CreateWindowExA(
        0,
        window_class.lpszClassName,
        "Star Wars: Episode I: Jedi Power Battles",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rectangle.right - rectangle.left,
        rectangle.bottom - rectangle.top,
        NULL,
        NULL,
        instance,
        NULL);
    if (window != NULL && visible) {
        ShowWindow(window, maximize ? SW_MAXIMIZE : SW_SHOW);
        UpdateWindow(window);
    }
    return window;
}

static uint8_t pc_present_luma(uint32_t pixel)
{
    unsigned red = (pixel >> 16) & UINT32_C(0xff);
    unsigned green = (pixel >> 8) & UINT32_C(0xff);
    unsigned blue = pixel & UINT32_C(0xff);

    return (uint8_t)(
        (red * 54u + green * 183u + blue * 19u) >> 8);
}

static uint32_t pc_present_average_edge(
    uint32_t center,
    uint32_t north,
    uint32_t south,
    uint32_t west,
    uint32_t east)
{
    unsigned red =
        (((center >> 16) & UINT32_C(0xff)) * 2u +
         ((north >> 16) & UINT32_C(0xff)) +
         ((south >> 16) & UINT32_C(0xff)) +
         ((west >> 16) & UINT32_C(0xff)) +
         ((east >> 16) & UINT32_C(0xff))) / 6u;
    unsigned green =
        (((center >> 8) & UINT32_C(0xff)) * 2u +
         ((north >> 8) & UINT32_C(0xff)) +
         ((south >> 8) & UINT32_C(0xff)) +
         ((west >> 8) & UINT32_C(0xff)) +
         ((east >> 8) & UINT32_C(0xff))) / 6u;
    unsigned blue =
        ((center & UINT32_C(0xff)) * 2u +
         (north & UINT32_C(0xff)) +
         (south & UINT32_C(0xff)) +
         (west & UINT32_C(0xff)) +
         (east & UINT32_C(0xff))) / 6u;

    return (center & UINT32_C(0xff000000)) |
        (red << 16) | (green << 8) | blue;
}

static const uint32_t *pc_present_fxaa_pixels(
    const JPBSoftwareFramebuffer *framebuffer)
{
    static uint32_t *fxaa_pixels;
    static size_t fxaa_capacity;
    size_t required;
    int y;

    if (framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->width < 3 || framebuffer->height < 3 ||
        framebuffer->stridePixels < framebuffer->width) {
        return framebuffer != NULL ? framebuffer->pixels : NULL;
    }
    required = (size_t)framebuffer->width *
        (size_t)framebuffer->height;
    if (required > fxaa_capacity) {
        uint32_t *new_pixels =
            (uint32_t *)realloc(fxaa_pixels, required * sizeof(uint32_t));

        if (new_pixels == NULL) {
            return framebuffer->pixels;
        }
        fxaa_pixels = new_pixels;
        fxaa_capacity = required;
    }
    for (y = 0; y < framebuffer->height; ++y) {
        int x;

        for (x = 0; x < framebuffer->width; ++x) {
            uint32_t center = framebuffer->pixels[
                (size_t)y * (size_t)framebuffer->stridePixels +
                (size_t)x];

            if (x == 0 || y == 0 ||
                x + 1 == framebuffer->width ||
                y + 1 == framebuffer->height) {
                fxaa_pixels[
                    (size_t)y * (size_t)framebuffer->width +
                    (size_t)x] = center;
            } else {
                uint32_t north = framebuffer->pixels[
                    (size_t)(y - 1) *
                        (size_t)framebuffer->stridePixels +
                    (size_t)x];
                uint32_t south = framebuffer->pixels[
                    (size_t)(y + 1) *
                        (size_t)framebuffer->stridePixels +
                    (size_t)x];
                uint32_t west = framebuffer->pixels[
                    (size_t)y * (size_t)framebuffer->stridePixels +
                    (size_t)(x - 1)];
                uint32_t east = framebuffer->pixels[
                    (size_t)y * (size_t)framebuffer->stridePixels +
                    (size_t)(x + 1)];
                uint8_t min_luma = pc_present_luma(center);
                uint8_t max_luma = min_luma;
                uint8_t luma;

                luma = pc_present_luma(north);
                if (luma < min_luma) min_luma = luma;
                if (luma > max_luma) max_luma = luma;
                luma = pc_present_luma(south);
                if (luma < min_luma) min_luma = luma;
                if (luma > max_luma) max_luma = luma;
                luma = pc_present_luma(west);
                if (luma < min_luma) min_luma = luma;
                if (luma > max_luma) max_luma = luma;
                luma = pc_present_luma(east);
                if (luma < min_luma) min_luma = luma;
                if (luma > max_luma) max_luma = luma;

                fxaa_pixels[
                    (size_t)y * (size_t)framebuffer->width +
                    (size_t)x] =
                    max_luma - min_luma >= 24u
                        ? pc_present_average_edge(
                              center, north, south, west, east)
                        : center;
            }
        }
    }
    return fxaa_pixels;
}

static int pc_present(
    HWND window, const JPBSoftwareFramebuffer *framebuffer)
{
    BITMAPINFO bitmap;
    RECT client;
    HDC device = GetDC(window);
    const uint32_t *present_pixels;
    int client_width;
    int client_height;
    int destination_width;
    int destination_height;
    int destination_x;
    int destination_y;
    int copied_scan_lines;

    if (device == NULL || framebuffer == NULL ||
        framebuffer->pixels == NULL) {
        if (device != NULL) {
            ReleaseDC(window, device);
        }
        return 0;
    }
    memset(&bitmap, 0, sizeof(bitmap));
    bitmap.bmiHeader.biSize = sizeof(bitmap.bmiHeader);
    bitmap.bmiHeader.biWidth = framebuffer->width;
    bitmap.bmiHeader.biHeight = -framebuffer->height;
    bitmap.bmiHeader.biPlanes = 1;
    bitmap.bmiHeader.biBitCount = 32;
    bitmap.bmiHeader.biCompression = BI_RGB;
    GetClientRect(window, &client);
    client_width = client.right - client.left;
    client_height = client.bottom - client.top;
    destination_width = client_width;
    destination_height = client_height;
    if (client_width <= 0 || client_height <= 0) {
        ReleaseDC(window, device);
        return 0;
    }
    if ((int64_t)client_width * framebuffer->height >
        (int64_t)client_height * framebuffer->width) {
        destination_width = (int)(
            (int64_t)client_height * framebuffer->width /
            framebuffer->height);
    } else {
        destination_height = (int)(
            (int64_t)client_width * framebuffer->height /
            framebuffer->width);
    }
    destination_x = (client_width - destination_width) / 2;
    destination_y = (client_height - destination_height) / 2;
    present_pixels = pc_present_fxaa_pixels(framebuffer);
    if (present_pixels == NULL) {
        ReleaseDC(window, device);
        return 0;
    }
    SetStretchBltMode(device, HALFTONE);
    SetBrushOrgEx(device, 0, 0, NULL);
    if (destination_x != 0 || destination_y != 0 ||
        destination_width != client_width ||
        destination_height != client_height) {
        HBRUSH brush = (HBRUSH)GetStockObject(BLACK_BRUSH);

        FillRect(device, &client, brush);
    }
    copied_scan_lines = StretchDIBits(
        device,
        destination_x,
        destination_y,
        destination_width,
        destination_height,
        0,
        0,
        framebuffer->width,
        framebuffer->height,
        present_pixels,
        &bitmap,
        DIB_RGB_COLORS,
        SRCCOPY);
    ReleaseDC(window, device);
    return copied_scan_lines != 0 && copied_scan_lines != GDI_ERROR;
}

static int pc_screen_draw_is_enemy_radar(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           !draw->hasSource &&
           ((draw->color.cd == 0x9f &&
             draw->color.r == 0x00 &&
             draw->color.g == 0x00 &&
             draw->color.b == 0x00) ||
            (draw->color.cd == 0xff &&
             ((draw->color.r == 0xff &&
               draw->color.g == 0xff &&
               draw->color.b == 0xff) ||
              (draw->color.r == 0xff &&
               draw->color.g == 0x20 &&
               draw->color.b == 0x20) ||
              (draw->color.r == 0x20 &&
               draw->color.g == 0xff &&
               draw->color.b == 0x20))));
}

static int pc_screen_draw_is_enemy_radar_background_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 426 &&
           draw->destination.top == 34 &&
           draw->destination.right == 532 &&
           draw->destination.bottom == 154 &&
           draw->color.r == 0 &&
           draw->color.g == 0 &&
           draw->color.b == 0 &&
           draw->color.cd == 159;
}

static int pc_screen_draw_is_enemy_radar_player_marker_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 478 &&
           draw->destination.top == 93 &&
           draw->destination.right == 480 &&
           draw->destination.bottom == 95 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 255;
}

static int pc_screen_draw_is_enemy_radar_red_marker_a_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 492 &&
           draw->destination.top == 136 &&
           draw->destination.right == 494 &&
           draw->destination.bottom == 138 &&
           draw->color.r == 255 &&
           draw->color.g == 32 &&
           draw->color.b == 32 &&
           draw->color.cd == 255;
}

static int pc_screen_draw_is_enemy_radar_green_marker_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 476 &&
           draw->destination.top == 103 &&
           draw->destination.right == 478 &&
           draw->destination.bottom == 105 &&
           draw->color.r == 32 &&
           draw->color.g == 255 &&
           draw->color.b == 32 &&
           draw->color.cd == 255;
}

static int pc_screen_draw_is_enemy_radar_background_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 852 &&
           draw->destination.top == 69 &&
           draw->destination.right == 1065 &&
           draw->destination.bottom == 308 &&
           draw->color.r == 0 &&
           draw->color.g == 0 &&
           draw->color.b == 0 &&
           draw->color.cd == 159;
}

static int pc_screen_draw_is_enemy_radar_player_marker_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 956 &&
           draw->destination.top == 186 &&
           draw->destination.right == 960 &&
           draw->destination.bottom == 191 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 255;
}

static int pc_screen_draw_is_enemy_radar_red_marker_a_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 970 &&
           draw->destination.top == 229 &&
           draw->destination.right == 974 &&
           draw->destination.bottom == 234 &&
           draw->color.r == 255 &&
           draw->color.g == 32 &&
           draw->color.b == 32 &&
           draw->color.cd == 255;
}

static int pc_screen_draw_is_enemy_radar_green_marker_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 954 &&
           draw->destination.top == 196 &&
           draw->destination.right == 958 &&
           draw->destination.bottom == 201 &&
           draw->color.r == 32 &&
           draw->color.g == 255 &&
           draw->color.b == 32 &&
           draw->color.cd == 255;
}

static int pc_screen_draw_is_damage_tracker(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->color.cd == 0x7f &&
           draw->color.r == 0xfc &&
           draw->color.b == 0x00 &&
           draw->color.g != 0 &&
           draw->color.g <= 0xfc;
}

static int pc_screen_draw_is_damage_tracker_full_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 105 &&
           draw->destination.top == 63 &&
           draw->destination.right == 115 &&
           draw->destination.bottom == 70 &&
           draw->color.r == 0xfc &&
           draw->color.g == 0xd4 &&
           draw->color.b == 0x00 &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_damage_tracker_full_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 211 &&
           draw->destination.top == 127 &&
           draw->destination.right == 231 &&
           draw->destination.bottom == 140 &&
           draw->color.r == 0xfc &&
           draw->color.g == 0xd4 &&
           draw->color.b == 0x00 &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_damage_tracker_p2_full_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 839 &&
           draw->destination.top == 63 &&
           draw->destination.right == 854 &&
           draw->destination.bottom == 70 &&
           draw->color.r == 0xfc &&
           draw->color.g == 0xc0 &&
           draw->color.b == 0x00 &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_damage_tracker_p2_full_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 1679 &&
           draw->destination.top == 127 &&
           draw->destination.right == 1709 &&
           draw->destination.bottom == 140 &&
           draw->color.r == 0xfc &&
           draw->color.g == 0xc0 &&
           draw->color.b == 0x00 &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_damage_tracker_compact_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 25 &&
           draw->destination.top == 63 &&
           draw->destination.right == 35 &&
           draw->destination.bottom == 70 &&
           draw->color.r == 0xfc &&
           draw->color.g == 0xd4 &&
           draw->color.b == 0x00 &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_damage_tracker_compact_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 50 &&
           draw->destination.top == 127 &&
           draw->destination.right == 70 &&
           draw->destination.bottom == 140 &&
           draw->color.r == 0xfc &&
           draw->color.g == 0xd4 &&
           draw->color.b == 0x00 &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_damage_tracker_p2_compact_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 921 &&
           draw->destination.top == 63 &&
           draw->destination.right == 936 &&
           draw->destination.bottom == 70 &&
           draw->color.r == 0xfc &&
           draw->color.g == 0xc0 &&
           draw->color.b == 0x00 &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_damage_tracker_p2_compact_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 1842 &&
           draw->destination.top == 127 &&
           draw->destination.right == 1872 &&
           draw->destination.bottom == 140 &&
           draw->color.r == 0xfc &&
           draw->color.g == 0xc0 &&
           draw->color.b == 0x00 &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_lifetile_exact_960(
    const JPBGameRuntimeScreenDraw *draw,
    int left,
    int top,
    int right,
    int bottom,
    unsigned red,
    unsigned green,
    unsigned blue,
    unsigned alpha)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->isPlayerHudTile &&
           draw->destination.left == left &&
           draw->destination.top == top &&
           draw->destination.right == right &&
           draw->destination.bottom == bottom &&
           draw->color.r == red &&
           draw->color.g == green &&
           draw->color.b == blue &&
           draw->color.cd == alpha;
}

static int pc_screen_draw_is_lifetile_health_left_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 680, 195, 685, 199, 128, 128, 128, 76);
}

static int pc_screen_draw_is_lifetile_health_bar_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 685, 195, 738, 199, 16, 128, 16, 87);
}

static int pc_screen_draw_is_lifetile_health_right_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 738, 195, 743, 199, 128, 128, 128, 76);
}

static int pc_screen_draw_is_lifetile_force_left_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 680, 201, 685, 205, 128, 128, 128, 76);
}

static int pc_screen_draw_is_lifetile_force_bar_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 685, 201, 738, 205, 16, 16, 128, 87);
}

static int pc_screen_draw_is_lifetile_force_right_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 738, 201, 743, 205, 128, 128, 128, 76);
}

static int pc_screen_draw_is_lifetile_health_left_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 1361, 391, 1370, 398, 128, 128, 128, 76);
}

static int pc_screen_draw_is_lifetile_health_bar_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 1371, 391, 1476, 398, 16, 128, 16, 87);
}

static int pc_screen_draw_is_lifetile_health_right_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 1477, 391, 1486, 398, 128, 128, 128, 76);
}

static int pc_screen_draw_is_lifetile_force_left_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 1361, 403, 1370, 410, 128, 128, 128, 76);
}

static int pc_screen_draw_is_lifetile_force_bar_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 1371, 403, 1476, 410, 16, 16, 128, 87);
}

static int pc_screen_draw_is_lifetile_force_right_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_lifetile_exact_960(
        draw, 1477, 403, 1486, 410, 128, 128, 128, 76);
}

static int pc_screen_draw_matches_expected_player_hud_tile(
    const JPBGameRuntimeScreenDraw *draw,
    const PcExpectedScreenDraw *expected)
{
    return draw != NULL &&
           expected != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->isPlayerHudTile &&
           draw->destination.left == expected->left &&
           draw->destination.top == expected->top &&
           draw->destination.right == expected->right &&
           draw->destination.bottom == expected->bottom &&
           draw->color.r == expected->red &&
           draw->color.g == expected->green &&
           draw->color.b == expected->blue &&
           draw->color.cd == expected->alpha &&
           draw->textureWidth == expected->textureWidth &&
           draw->textureHeight == expected->textureHeight &&
           fabsf(draw->layerDepth - expected->layerDepth) < 0.001f;
}

static size_t pc_count_expected_player_hud_tile_draws(
    const JPBGameRuntime *runtime,
    const PcExpectedScreenDraw *expected)
{
    size_t count = 0;
    size_t draw_index;

    if (runtime == NULL || expected == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->screenDrawCount;
         ++draw_index) {
        if (pc_screen_draw_matches_expected_player_hud_tile(
                &runtime->screenDraws[draw_index],
                expected)) {
            ++count;
        }
    }
    return count;
}

static int pc_runtime_has_expected_player_hud_tiles(
    const JPBGameRuntime *runtime,
    const PcExpectedScreenDraw *expected,
    size_t expected_count)
{
    size_t expected_index;

    if (runtime == NULL || expected == NULL) {
        return 0;
    }
    for (expected_index = 0;
         expected_index < expected_count;
         ++expected_index) {
        if (pc_count_expected_player_hud_tile_draws(
                runtime,
                &expected[expected_index]) != 1) {
            return 0;
        }
    }
    return 1;
}

static const PcExpectedScreenDraw kProjectedLifeTile960[] = {
    {230, 1026, 238, 1032, 128, 128, 128, 165, 1024, 512, 257.550f},
    {237, 1026, 269, 1032, 16, 128, 16, 189, 128, 128, 257.550f},
    {267, 1026, 275, 1032, 128, 128, 128, 165, 0, 0, 257.550f},
    {-711, 623, -705, 628, 128, 128, 128, 165, 0, 0, 443.069f},
    {-705, 623, -677, 628, 16, 128, 16, 189, 0, 0, 443.069f},
    {-677, 623, -671, 628, 128, 128, 128, 165, 0, 0, 443.069f},
    {-2426, 1621, -2415, 1629, 128, 128, 128, 165, 0, 0, 159.105f},
    {-2418, 1621, -2381, 1629, 16, 128, 16, 189, 0, 0, 159.105f},
    {-2384, 1621, -2373, 1629, 128, 128, 128, 165, 0, 0, 159.105f},
    {0, 278, 4, 282, 128, 128, 128, 132, 0, 0, 1159.105f},
    {5, 278, 30, 282, 16, 128, 16, 151, 0, 0, 1159.105f},
    {30, 278, 35, 282, 128, 128, 128, 132, 0, 0, 1159.105f},
    {123252, 83212, 123585, 83461, 128, 128, 128, 165, 0, 0, 2.979f},
    {123381, 83212, 124170, 83461, 16, 128, 16, 189, 0, 0, 2.979f},
    {123966, 83212, 124299, 83461, 128, 128, 128, 165, 0, 0, 2.979f}
};

static const PcExpectedScreenDraw kProjectedLifeTile1080[] = {
    {461, 2052, 477, 2064, 128, 128, 128, 165, 1024, 512, 257.550f},
    {475, 2052, 538, 2064, 16, 128, 16, 189, 128, 128, 257.550f},
    {535, 2052, 551, 2064, 128, 128, 128, 165, 0, 0, 257.550f},
    {-1423, 1247, -1410, 1257, 128, 128, 128, 165, 0, 0, 443.069f},
    {-1410, 1247, -1355, 1257, 16, 128, 16, 189, 0, 0, 443.069f},
    {-1355, 1247, -1342, 1257, 128, 128, 128, 165, 0, 0, 443.069f},
    {-4852, 3242, -4831, 3258, 128, 128, 128, 165, 0, 0, 159.105f},
    {-4837, 3242, -4762, 3258, 16, 128, 16, 189, 0, 0, 159.105f},
    {-4768, 3242, -4747, 3258, 128, 128, 128, 165, 0, 0, 159.105f},
    {0, 557, 9, 564, 128, 128, 128, 132, 0, 0, 1159.105f},
    {10, 557, 60, 564, 16, 128, 16, 151, 0, 0, 1159.105f},
    {61, 557, 71, 564, 128, 128, 128, 132, 0, 0, 1159.105f},
    {246505, 166424, 247171, 166923, 128, 128, 128, 165, 0, 0, 2.979f},
    {246762, 166424, 248341, 166923, 16, 128, 16, 189, 0, 0, 2.979f},
    {247932, 166424, 248598, 166923, 128, 128, 128, 165, 0, 0, 2.979f}
};

#define PC_HUD_ITEM_PIXELS_960 ((size_t)4381)
#define PC_HUD_ITEM_PIXELS_1080 ((size_t)17489)
#define PC_HUD_ITEM_PIXELS_P2_960 ((size_t)8762)
#define PC_HUD_ITEM_PIXELS_P2_1080 ((size_t)34975)

static int pc_screen_draw_is_normal_item_panel_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_detonator") != NULL &&
           draw->destination.left == 22 &&
           draw->destination.top == 428 &&
           draw->destination.right == 118 &&
           draw->destination.bottom == 524 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 128 &&
           draw->source.bottom == 128 &&
           draw->textureWidth == 128 &&
           draw->textureHeight == 128 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_normal_item_panel_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_detonator") != NULL &&
           draw->destination.left == 44 &&
           draw->destination.top == 856 &&
           draw->destination.right == 236 &&
           draw->destination.bottom == 1048 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 128 &&
           draw->source.bottom == 128 &&
           draw->textureWidth == 128 &&
           draw->textureHeight == 128 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_normal_item_panel_proof_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_detonator") != NULL &&
           draw->destination.left == 44 &&
           draw->destination.top == 856 &&
           draw->destination.right == 236 &&
           draw->destination.bottom == 1048 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 128 &&
           draw->source.bottom == 128 &&
           draw->textureWidth == 128 &&
           draw->textureHeight == 128 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 200;
}

static int pc_screen_draw_is_core_score_panel_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_meter_main") != NULL &&
           draw->destination.left == 24 &&
           draw->destination.top == 18 &&
           draw->destination.right == 264 &&
           draw->destination.bottom == 120 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 1024 &&
           draw->source.bottom == 512 &&
           draw->textureWidth == 1024 &&
           draw->textureHeight == 512 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_core_item_panel_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_detonator") != NULL &&
           draw->destination.left == 22 &&
           draw->destination.top == 428 &&
           draw->destination.right == 118 &&
           draw->destination.bottom == 524 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 128 &&
           draw->source.bottom == 128 &&
           draw->textureWidth == 128 &&
           draw->textureHeight == 128 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_core_life_bar_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 105 &&
           draw->destination.top == 75 &&
           draw->destination.right == 263 &&
           draw->destination.bottom == 82 &&
           draw->color.r == 16 &&
           draw->color.g == 252 &&
           draw->color.b == 16 &&
           draw->color.cd == 1;
}

static int pc_screen_draw_is_core_force_bar_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 105 &&
           draw->destination.top == 87 &&
           draw->destination.right == 263 &&
           draw->destination.bottom == 94 &&
           draw->color.r == 0 &&
           draw->color.g == 0 &&
           draw->color.b == 255 &&
           draw->color.cd == 1;
}

static int pc_screen_draw_is_core_score_panel_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_meter_main") != NULL &&
           draw->destination.left == 48 &&
           draw->destination.top == 36 &&
           draw->destination.right == 528 &&
           draw->destination.bottom == 240 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 1024 &&
           draw->source.bottom == 512 &&
           draw->textureWidth == 1024 &&
           draw->textureHeight == 512 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_core_item_panel_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_detonator") != NULL &&
           draw->destination.left == 44 &&
           draw->destination.top == 856 &&
           draw->destination.right == 236 &&
           draw->destination.bottom == 1048 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 128 &&
           draw->source.bottom == 128 &&
           draw->textureWidth == 128 &&
           draw->textureHeight == 128 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_core_life_bar_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 211 &&
           draw->destination.top == 151 &&
           draw->destination.right == 527 &&
           draw->destination.bottom == 164 &&
           draw->color.r == 16 &&
           draw->color.g == 252 &&
           draw->color.b == 16 &&
           draw->color.cd == 1;
}

static int pc_screen_draw_is_core_force_bar_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 211 &&
           draw->destination.top == 175 &&
           draw->destination.right == 527 &&
           draw->destination.bottom == 188 &&
           draw->color.r == 0 &&
           draw->color.g == 0 &&
           draw->color.b == 255 &&
           draw->color.cd == 1;
}

static int pc_screen_draw_is_p2_core_score_panel_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_meter_main") != NULL &&
           draw->destination.left == 696 &&
           draw->destination.top == 18 &&
           draw->destination.right == 936 &&
           draw->destination.bottom == 120 &&
           draw->source.left == 1024 &&
           draw->source.top == 0 &&
           draw->source.right == 0 &&
           draw->source.bottom == 512 &&
           draw->textureWidth == 1024 &&
           draw->textureHeight == 512 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_p2_core_item_panel_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_bolt") != NULL &&
           draw->destination.left == 842 &&
           draw->destination.top == 428 &&
           draw->destination.right == 938 &&
           draw->destination.bottom == 524 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 128 &&
           draw->source.bottom == 128 &&
           draw->textureWidth == 128 &&
           draw->textureHeight == 128 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_p2_core_life_bar_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 696 &&
           draw->destination.top == 75 &&
           draw->destination.right == 854 &&
           draw->destination.bottom == 82 &&
           draw->color.r == 16 &&
           draw->color.g == 252 &&
           draw->color.b == 16 &&
           draw->color.cd == 1;
}

static int pc_screen_draw_is_p2_core_force_bar_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 696 &&
           draw->destination.top == 87 &&
           draw->destination.right == 854 &&
           draw->destination.bottom == 94 &&
           draw->color.r == 0 &&
           draw->color.g == 0 &&
           draw->color.b == 255 &&
           draw->color.cd == 1;
}

static int pc_screen_draw_is_p2_core_score_panel_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_meter_main") != NULL &&
           draw->destination.left == 1392 &&
           draw->destination.top == 36 &&
           draw->destination.right == 1872 &&
           draw->destination.bottom == 240 &&
           draw->source.left == 1024 &&
           draw->source.top == 0 &&
           draw->source.right == 0 &&
           draw->source.bottom == 512 &&
           draw->textureWidth == 1024 &&
           draw->textureHeight == 512 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_p2_core_item_panel_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_bolt") != NULL &&
           draw->destination.left == 1684 &&
           draw->destination.top == 856 &&
           draw->destination.right == 1876 &&
           draw->destination.bottom == 1048 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 128 &&
           draw->source.bottom == 128 &&
           draw->textureWidth == 128 &&
           draw->textureHeight == 128 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_p2_core_life_bar_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 1393 &&
           draw->destination.top == 151 &&
           draw->destination.right == 1709 &&
           draw->destination.bottom == 164 &&
           draw->color.r == 16 &&
           draw->color.g == 252 &&
           draw->color.b == 16 &&
           draw->color.cd == 1;
}

static int pc_screen_draw_is_p2_core_force_bar_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 1393 &&
           draw->destination.top == 175 &&
           draw->destination.right == 1709 &&
           draw->destination.bottom == 188 &&
           draw->color.r == 0 &&
           draw->color.g == 0 &&
           draw->color.b == 255 &&
           draw->color.cd == 1;
}

static int pc_screen_draw_is_continue_credit_exact_960(
    const JPBGameRuntimeScreenDraw *draw,
    int left,
    int top,
    int right,
    int bottom,
    unsigned alpha)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_credit") != NULL &&
           draw->destination.left == left &&
           draw->destination.top == top &&
           draw->destination.right == right &&
           draw->destination.bottom == bottom &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 128 &&
           draw->source.bottom == 128 &&
           draw->textureWidth == 128 &&
           draw->textureHeight == 128 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == alpha;
}

static int pc_screen_draw_is_continue_credit_exact_1080(
    const JPBGameRuntimeScreenDraw *draw,
    int left,
    int top,
    int right,
    int bottom,
    unsigned alpha)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_credit") != NULL &&
           draw->destination.left == left &&
           draw->destination.top == top &&
           draw->destination.right == right &&
           draw->destination.bottom == bottom &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 128 &&
           draw->source.bottom == 128 &&
           draw->textureWidth == 128 &&
           draw->textureHeight == 128 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == alpha;
}

static int pc_screen_draw_is_continue_credit_first_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_960(
        draw, 428, 22, 455, 49, 110);
}

static int pc_screen_draw_is_continue_credit_second_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_960(
        draw, 447, 22, 474, 49, 0);
}

static int pc_screen_draw_is_continue_credit_third_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_960(
        draw, 466, 22, 493, 49, 0);
}

static int pc_screen_draw_is_continue_credit_fourth_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_960(
        draw, 485, 22, 512, 49, 0);
}

static int pc_screen_draw_is_continue_credit_fifth_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_960(
        draw, 504, 22, 531, 49, 0);
}

static int pc_screen_draw_is_continue_credit_wrapped_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_960(
        draw, 466, 41, 493, 68, 0);
}

static int pc_screen_draw_is_continue_credit_first_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_1080(
        draw, 857, 44, 911, 98, 110);
}

static int pc_screen_draw_is_continue_credit_second_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_1080(
        draw, 895, 44, 949, 98, 0);
}

static int pc_screen_draw_is_continue_credit_third_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_1080(
        draw, 933, 44, 987, 98, 0);
}

static int pc_screen_draw_is_continue_credit_fourth_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_1080(
        draw, 971, 44, 1025, 98, 0);
}

static int pc_screen_draw_is_continue_credit_fifth_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_1080(
        draw, 1009, 44, 1063, 98, 0);
}

static int pc_screen_draw_is_continue_credit_wrapped_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return pc_screen_draw_is_continue_credit_exact_1080(
        draw, 933, 82, 987, 136, 0);
}

static int pc_screen_draw_is_rescue_counter_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_maiden") != NULL &&
           draw->destination.left == 432 &&
           draw->destination.top == 428 &&
           draw->destination.right == 528 &&
           draw->destination.bottom == 524 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 512 &&
           draw->source.bottom == 512 &&
           draw->textureWidth == 512 &&
           draw->textureHeight == 512 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_rescue_counter_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_maiden") != NULL &&
           draw->destination.left == 864 &&
           draw->destination.top == 856 &&
           draw->destination.right == 1056 &&
           draw->destination.bottom == 1048 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 512 &&
           draw->source.bottom == 512 &&
           draw->textureWidth == 512 &&
           draw->textureHeight == 512 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_pilot_counter_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_pilot") != NULL &&
           draw->destination.left == 432 &&
           draw->destination.top == 428 &&
           draw->destination.right == 528 &&
           draw->destination.bottom == 524 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 512 &&
           draw->source.bottom == 512 &&
           draw->textureWidth == 512 &&
           draw->textureHeight == 512 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_pilot_counter_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL || draw->texture == NULL ||
        !draw->hasSource) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_pilot") != NULL &&
           draw->destination.left == 864 &&
           draw->destination.top == 856 &&
           draw->destination.right == 1056 &&
           draw->destination.bottom == 1048 &&
           draw->source.left == 0 &&
           draw->source.top == 0 &&
           draw->source.right == 512 &&
           draw->source.bottom == 512 &&
           draw->textureWidth == 512 &&
           draw->textureHeight == 512 &&
           draw->color.r == 255 &&
           draw->color.g == 255 &&
           draw->color.b == 255 &&
           draw->color.cd == 2;
}

static int pc_screen_draw_is_kadu_race_bar(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->color.cd == 0x7f &&
           ((draw->color.r == 0xff &&
             draw->color.g == 0x40 &&
             draw->color.b == 0x10) ||
            (draw->color.r == 0x10 &&
             draw->color.g == 0x40 &&
             draw->color.b == 0xff));
}

static int pc_screen_draw_is_kadu_race_p1_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 305 &&
           draw->destination.top == 514 &&
           draw->destination.right == 369 &&
           draw->destination.bottom == 520 &&
           draw->color.r == 0xff &&
           draw->color.g == 0x40 &&
           draw->color.b == 0x10 &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_kadu_race_p2_frame10_960(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 555 &&
           draw->destination.top == 514 &&
           draw->destination.right == 655 &&
           draw->destination.bottom == 520 &&
           draw->color.r == 0x10 &&
           draw->color.g == 0x40 &&
           draw->color.b == 0xff &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_kadu_race_p1_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 610 &&
           draw->destination.top == 1028 &&
           draw->destination.right == 674 &&
           draw->destination.bottom == 1040 &&
           draw->color.r == 0xff &&
           draw->color.g == 0x40 &&
           draw->color.b == 0x10 &&
           draw->color.cd == 0x7f;
}

static int pc_screen_draw_is_kadu_race_p2_frame10_1080(
    const JPBGameRuntimeScreenDraw *draw)
{
    return draw != NULL &&
           draw->texture == NULL &&
           !draw->hasSource &&
           draw->destination.left == 1210 &&
           draw->destination.top == 1028 &&
           draw->destination.right == 1310 &&
           draw->destination.bottom == 1040 &&
           draw->color.r == 0x10 &&
           draw->color.g == 0x40 &&
           draw->color.b == 0xff &&
           draw->color.cd == 0x7f;
}

static int pc_screen_poly_is_offscreen_arrow(
    const JPBGameRuntimeScreenPolyDraw *draw)
{
    return draw != NULL &&
           draw->vertexCount == 4 &&
           draw->vertices[0].z == 0.0001f &&
           draw->vertices[1].z == 0.0001f &&
           draw->vertices[2].z == 0.0001f &&
           draw->vertices[3].z == 0.0001f &&
           draw->vertices[1].argb == 0 &&
           (draw->vertices[0].argb & UINT32_C(0xff000000)) ==
               UINT32_C(0xff000000) &&
           draw->vertices[0].argb == draw->vertices[3].argb &&
           draw->vertices[2].argb != 0 &&
           draw->vertices[2].argb != draw->vertices[0].argb;
}

static int pc_screen_poly_is_offscreen_arrow_frame1_960(
    const JPBGameRuntimeScreenPolyDraw *draw)
{
    return draw != NULL &&
           draw->vertexCount == 4 &&
           draw->noScale == 0 &&
           draw->vertices[0].x == 1848.0f &&
           draw->vertices[0].y == 437.0f &&
           draw->vertices[0].z == 0.0001f &&
           draw->vertices[0].argb == UINT32_C(0xff3880f8) &&
           draw->vertices[1].x == 1863.0f &&
           draw->vertices[1].y == 446.0f &&
           draw->vertices[1].z == 0.0001f &&
           draw->vertices[1].argb == 0 &&
           draw->vertices[2].x == 1896.0f &&
           draw->vertices[2].y == 449.0f &&
           draw->vertices[2].z == 0.0001f &&
           draw->vertices[2].argb == UINT32_C(0xdae2ecfd) &&
           draw->vertices[3].x == 1846.0f &&
           draw->vertices[3].y == 453.0f &&
           draw->vertices[3].z == 0.0001f &&
           draw->vertices[3].argb == UINT32_C(0xff3880f8);
}

static int pc_screen_poly_is_offscreen_arrow_frame1_1080(
    const JPBGameRuntimeScreenPolyDraw *draw)
{
    return draw != NULL &&
           draw->vertexCount == 4 &&
           draw->noScale == 0 &&
           draw->vertices[0].x == 1848.0f &&
           draw->vertices[0].y == 628.0f &&
           draw->vertices[0].z == 0.0001f &&
           draw->vertices[0].argb == UINT32_C(0xff3880f8) &&
           draw->vertices[1].x == 1863.0f &&
           draw->vertices[1].y == 637.0f &&
           draw->vertices[1].z == 0.0001f &&
           draw->vertices[1].argb == 0 &&
           draw->vertices[2].x == 1896.0f &&
           draw->vertices[2].y == 639.0f &&
           draw->vertices[2].z == 0.0001f &&
           draw->vertices[2].argb == UINT32_C(0xdae2ecfd) &&
           draw->vertices[3].x == 1846.0f &&
           draw->vertices[3].y == 644.0f &&
           draw->vertices[3].z == 0.0001f &&
           draw->vertices[3].argb == UINT32_C(0xff3880f8);
}

static int pc_screen_draw_has_recovered_hud_owner(
    const JPBGameRuntimeScreenDraw *draw)
{
    const char *filename;

    if (draw == NULL) {
        return 0;
    }
    if (draw->isPlayerHudTile ||
        pc_screen_draw_is_enemy_radar(draw) ||
        pc_screen_draw_is_damage_tracker(draw) ||
        pc_screen_draw_is_kadu_race_bar(draw)) {
        return 1;
    }
    if (draw->texture == NULL && !draw->hasSource &&
        ((draw->color.r == 0x10 &&
          draw->color.g == 0xfc &&
          draw->color.b == 0x10) ||
         (draw->color.r == 0x00 &&
          draw->color.g == 0x00 &&
          draw->color.b == 0xff))) {
        return 1;
    }
    if (draw->texture == NULL) {
        return 0;
    }
    filename = draw->texture->filename;
    return strstr(filename, "a_meter_main") != NULL ||
           strstr(filename, "a_meter_bars") != NULL ||
           strstr(filename, "a_detonator") != NULL ||
           strstr(filename, "a_bolt") != NULL ||
           strstr(filename, "a_battery") != NULL ||
           strstr(filename, "a_shield") != NULL ||
           strstr(filename, "a_credit") != NULL ||
           strstr(filename, "a_lives") != NULL ||
           strstr(filename, "a_pilot") != NULL ||
           strstr(filename, "a_maiden") != NULL;
}

static size_t pc_count_screen_draws_without_recovered_hud_owner(
    const JPBGameRuntime *runtime)
{
    size_t count = 0;
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->screenDrawCount;
         ++draw_index) {
        if (!pc_screen_draw_has_recovered_hud_owner(
                &runtime->screenDraws[draw_index])) {
            ++count;
        }
    }
    return count;
}

static int pc_sprite_display_has_recovered_hud_owner(
    const JPBGameRuntimeSpriteDisplay *draw)
{
    const char *filename;

    if (draw == NULL || draw->material == NULL) {
        return 0;
    }
    filename = draw->material->filename;
    return strstr(filename, "a_meter_main") != NULL ||
           strstr(filename, "a_detonator") != NULL ||
           strstr(filename, "a_bolt") != NULL ||
           strstr(filename, "a_battery") != NULL ||
           strstr(filename, "a_shield") != NULL ||
           strstr(filename, "a_credit") != NULL ||
           strstr(filename, "a_lives") != NULL ||
           strstr(filename, "a_pilot") != NULL ||
           strstr(filename, "a_maiden") != NULL;
}

static size_t pc_count_sprite_displays_without_recovered_hud_owner(
    const JPBGameRuntime *runtime)
{
    size_t count = 0;
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->spriteDisplayDrawCount;
         ++draw_index) {
        if (!pc_sprite_display_has_recovered_hud_owner(
                &runtime->spriteDisplayDraws[draw_index])) {
            ++count;
        }
    }
    return count;
}

static int pc_psx_texture_draw_has_recovered_hud_owner(
    const JPBGameRuntimePsxTextureDraw *draw)
{
    if (draw == NULL) {
        return 0;
    }
    return draw->texture >= 0xb5u &&
           draw->texture <= 0xbeu &&
           draw->width == 0.0f &&
           draw->height == 0.0f &&
           draw->transparency == 255u &&
           draw->red == 0x80 &&
           draw->green == 0x80 &&
           draw->blue == 0x80;
}

static size_t pc_count_psx_texture_draws_without_recovered_hud_owner(
    const JPBGameRuntime *runtime)
{
    size_t count = 0;
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->psxTextureDrawCount;
         ++draw_index) {
        if (!pc_psx_texture_draw_has_recovered_hud_owner(
                &runtime->psxTextureDraws[draw_index])) {
            ++count;
        }
    }
    return count;
}

static int pc_screen_poly_has_recovered_hud_owner(
    const JPBGameRuntimeScreenPolyDraw *draw)
{
    if (draw == NULL) {
        return 0;
    }
    return pc_screen_poly_is_offscreen_arrow(draw);
}

static size_t pc_count_screen_polys_without_recovered_hud_owner(
    const JPBGameRuntime *runtime,
    int require_hud_owner)
{
    size_t count = 0;
    size_t draw_index;
    size_t captured_count;

    if (runtime == NULL || !require_hud_owner) {
        return 0;
    }
    captured_count = runtime->screenPolyDrawCount;
    if (captured_count > JPB_GAME_RUNTIME_SCREEN_POLY_CAPACITY) {
        captured_count = JPB_GAME_RUNTIME_SCREEN_POLY_CAPACITY;
    }
    for (draw_index = 0; draw_index < captured_count; ++draw_index) {
        const JPBGameRuntimeScreenPolyDraw *draw =
            &runtime->screenPolyDraws[draw_index];
        const char *filename =
            draw->texture != NULL ? draw->texture->filename : "";

        if (strstr(filename, "a_shadow") != NULL) {
            continue;
        }
        if (!pc_screen_poly_has_recovered_hud_owner(draw)) {
            ++count;
        }
    }
    return count;
}

static size_t pc_count_enemy_radar_draws(
    const JPBGameRuntime *runtime)
{
    size_t count = 0;
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->screenDrawCount;
         ++draw_index) {
        if (pc_screen_draw_is_enemy_radar(
                &runtime->screenDraws[draw_index])) {
            ++count;
        }
    }
    return count;
}

static size_t pc_count_screen_draws_matching(
    const JPBGameRuntime *runtime,
    int (*predicate)(const JPBGameRuntimeScreenDraw *))
{
    size_t count = 0;
    size_t draw_index;

    if (runtime == NULL || predicate == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->screenDrawCount;
         ++draw_index) {
        if (predicate(&runtime->screenDraws[draw_index])) {
            ++count;
        }
    }
    return count;
}

static size_t pc_count_screen_polys_matching(
    const JPBGameRuntime *runtime,
    int (*predicate)(const JPBGameRuntimeScreenPolyDraw *))
{
    size_t count = 0;
    size_t draw_index;
    size_t captured_count;

    if (runtime == NULL || predicate == NULL) {
        return 0;
    }
    captured_count = runtime->screenPolyDrawCount;
    if (captured_count > JPB_GAME_RUNTIME_SCREEN_POLY_CAPACITY) {
        captured_count = JPB_GAME_RUNTIME_SCREEN_POLY_CAPACITY;
    }
    for (draw_index = 0; draw_index < captured_count; ++draw_index) {
        if (predicate(&runtime->screenPolyDraws[draw_index])) {
            ++count;
        }
    }
    return count;
}

static int pc_text_draw_matches(
    const JPBGameRuntimeTextDraw *draw,
    const wchar_t *text,
    int x,
    int y,
    float scale,
    int alpha)
{
    if (draw == NULL || text == NULL) {
        return 0;
    }
    return wcscmp(draw->text, text) == 0 &&
           draw->compositePixels != 0 &&
           draw->x == x &&
           draw->y == y &&
           draw->mode == 0 &&
           draw->tint == 11 &&
           draw->alpha == alpha &&
           draw->fontStyle == 2 &&
           fabsf(draw->scale - scale) < 0.001f;
}

static int pc_text_draw_matches_mode(
    const JPBGameRuntimeTextDraw *draw,
    const wchar_t *text,
    int mode,
    int x,
    int y,
    float scale,
    int alpha)
{
    if (draw == NULL || text == NULL) {
        return 0;
    }
    return wcscmp(draw->text, text) == 0 &&
           draw->compositePixels != 0 &&
           draw->x == x &&
           draw->y == y &&
           draw->mode == mode &&
           draw->tint == 11 &&
           draw->alpha == alpha &&
           draw->fontStyle == 2 &&
           fabsf(draw->scale - scale) < 0.001f;
}

static int pc_text_draw_matches_owner(
    const JPBGameRuntimeTextDraw *draw,
    const wchar_t *text,
    int mode,
    int x,
    int y,
    float scale)
{
    if (draw == NULL || text == NULL) {
        return 0;
    }
    return wcscmp(draw->text, text) == 0 &&
           draw->compositePixels != 0 &&
           draw->x == x &&
           draw->y == y &&
           draw->mode == mode &&
           draw->tint == 11 &&
           draw->alpha >= 0 &&
           draw->alpha <= 255 &&
           draw->fontStyle == 2 &&
           fabsf(draw->scale - scale) < 0.001f;
}

static int pc_text_draw_is_seven_digit_score_at(
    const JPBGameRuntimeTextDraw *draw,
    int x,
    int y)
{
    size_t index;

    if (draw == NULL ||
        wcslen(draw->text) != 7u ||
        !pc_text_draw_matches_owner(
            draw, draw->text, 0, x, y, 3.24f)) {
        return 0;
    }
    for (index = 0; index < 7u; ++index) {
        if (draw->text[index] < L'0' ||
            draw->text[index] > L'9') {
            return 0;
        }
    }
    return 1;
}

static int pc_runtime_has_text_draw(
    const JPBGameRuntime *runtime,
    const wchar_t *text,
    int x,
    int y,
    float scale,
    int alpha)
{
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->textDrawCount;
         ++draw_index) {
        if (pc_text_draw_matches(
                &runtime->textDraws[draw_index],
                text,
                x,
                y,
                scale,
                alpha)) {
            return 1;
        }
    }
    return 0;
}

static int pc_runtime_has_text_draw_mode(
    const JPBGameRuntime *runtime,
    const wchar_t *text,
    int mode,
    int x,
    int y,
    float scale,
    int alpha)
{
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->textDrawCount;
         ++draw_index) {
        if (pc_text_draw_matches_mode(
                &runtime->textDraws[draw_index],
                text,
                mode,
                x,
                y,
                scale,
                alpha)) {
            return 1;
        }
    }
    return 0;
}

static int pc_draw3d_text_matches(
    const JPBGameRuntimeDraw3dText *draw,
    const char *text,
    float x,
    float y,
    float z,
    float scale,
    uint32_t color)
{
    if (draw == NULL || text == NULL) {
        return 0;
    }
    return strcmp(draw->text, text) == 0 &&
           fabsf(draw->x - x) < 0.001f &&
           fabsf(draw->y - y) < 0.001f &&
           fabsf(draw->z - z) < 0.001f &&
           fabsf(draw->scale - scale) < 0.001f &&
           draw->color == color;
}

static int pc_runtime_has_draw3d_text(
    const JPBGameRuntime *runtime,
    const char *text,
    float x,
    float y,
    float z,
    float scale,
    uint32_t color)
{
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->draw3dTextDrawCount;
         ++draw_index) {
        if (pc_draw3d_text_matches(
                &runtime->draw3dTextDraws[draw_index],
                text,
                x,
                y,
                z,
                scale,
                color)) {
            return 1;
        }
    }
    return 0;
}

static int pc_text_draw_has_recovered_hud_owner(
    const JPBGameRuntimeTextDraw *draw)
{
    if (draw == NULL) {
        return 0;
    }
    return pc_text_draw_is_seven_digit_score_at(draw, 106, 30) ||
           pc_text_draw_is_seven_digit_score_at(draw, 212, 60) ||
           pc_text_draw_is_seven_digit_score_at(draw, 708, 30) ||
           pc_text_draw_is_seven_digit_score_at(draw, 1416, 60) ||
           pc_text_draw_is_seven_digit_score_at(draw, 24, 31) ||
           pc_text_draw_is_seven_digit_score_at(draw, 48, 62) ||
           pc_text_draw_matches_owner(draw, L"0", 0, 94, 484, 1.8f) ||
           pc_text_draw_matches_owner(draw, L"0", 0, 188, 968, 1.8f) ||
           pc_text_draw_matches_owner(draw, L"4", 0, 914, 484, 1.8f) ||
           pc_text_draw_matches_owner(draw, L"4", 0, 1828, 968, 1.8f) ||
           pc_text_draw_matches_owner(draw, L"5", 0, 504, 484, 1.8f) ||
           pc_text_draw_matches_owner(draw, L"5", 0, 1008, 968, 1.8f) ||
           pc_text_draw_matches_owner(draw, L"7", 0, 504, 484, 1.8f) ||
           pc_text_draw_matches_owner(draw, L"7", 0, 1008, 968, 1.8f) ||
           pc_text_draw_matches_owner(draw, L"035", 0, 435, 464, 3.0f) ||
           pc_text_draw_matches_owner(draw, L"sec", 0, 496, 479, 1.5f) ||
           pc_text_draw_matches_owner(draw, L"035", 0, 871, 928, 3.0f) ||
           pc_text_draw_matches_owner(draw, L"sec", 0, 993, 959, 1.5f) ||
           pc_text_draw_matches_owner(draw, L"015", 0, 24, 100, 3.0f) ||
           pc_text_draw_matches_owner(draw, L"t", 0, 24, 130, 1.5f) ||
           pc_text_draw_matches_owner(draw, L"015", 0, 48, 200, 3.0f) ||
           pc_text_draw_matches_owner(draw, L"t", 0, 48, 260, 1.5f) ||
           pc_text_draw_matches_owner(draw, L"SUCCESS", 2, 480, 100, 1.0f) ||
           pc_text_draw_matches_owner(draw, L"SUCCESS", 2, 960, 200, 1.0f) ||
           pc_text_draw_matches_owner(draw, L"FAILED", 2, 480, 100, 1.0f) ||
           pc_text_draw_matches_owner(draw, L"FAILED", 2, 960, 200, 1.0f) ||
           pc_text_draw_matches_owner(draw, L"400", 0, 435, 406, 3.0f) ||
           pc_text_draw_matches_owner(draw, L"sec", 0, 496, 422, 1.5f) ||
           pc_text_draw_matches_owner(draw, L"400", 0, 871, 813, 3.0f) ||
           pc_text_draw_matches_owner(draw, L"sec", 0, 993, 844, 1.5f) ||
           pc_text_draw_matches_owner(draw, L"O", 0, 352, 142, 3.0f) ||
           pc_text_draw_matches_owner(draw, L"O", 0, 832, 412, 3.0f) ||
           pc_text_draw_matches_owner(draw, L"1", 0, 240, 184, 2.0f) ||
           pc_text_draw_matches_owner(draw, L"0", 0, 242, 184, 2.0f) ||
           pc_text_draw_matches_owner(draw, L"0", 0, 244, 184, 2.0f);
}

static size_t pc_count_text_draws_without_recovered_hud_owner(
    const JPBGameRuntime *runtime)
{
    size_t count = 0;
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->textDrawCount;
         ++draw_index) {
        if (!pc_text_draw_has_recovered_hud_owner(
                &runtime->textDraws[draw_index])) {
            ++count;
        }
    }
    return count;
}

static int pc_draw3d_text_has_recovered_hud_owner(
    const JPBGameRuntimeDraw3dText *draw)
{
    if (draw == NULL) {
        return 0;
    }
    return pc_draw3d_text_matches(
               draw,
               "0- ai 0\nt-LAND\n   ",
               6.0f,
               102.0f,
               22.0f,
               1.0f,
               UINT32_C(0xff8090a0)) ||
           pc_draw3d_text_matches(
               draw,
               "0- ai 0\nt-LAND\n   ",
               18.0f,
               136.0f,
               50.0f,
               1.0f,
               UINT32_C(0xff8090a0)) ||
           pc_draw3d_text_matches(
               draw,
               "2-enemy000 enemy000",
               20224.0f,
               3328.0f,
               -14848.0f,
               1.0f,
               UINT32_C(0xff8090a0)) ||
           pc_draw3d_text_matches(
               draw,
               "3-enemy001 enemy001",
               22157.0f,
               3328.0f,
               -14976.0f,
               1.0f,
               UINT32_C(0xff8090a0)) ||
           pc_draw3d_text_matches(
               draw,
               "4-enemy004 enemy004",
               22823.0f,
               3328.0f,
               -15181.0f,
               1.0f,
               UINT32_C(0xff8090a0)) ||
           pc_draw3d_text_matches(
               draw,
               "5-enemy005 enemy005",
               22580.0f,
               3328.0f,
               -15386.0f,
               1.0f,
               UINT32_C(0xff8090a0)) ||
           pc_draw3d_text_matches(
               draw,
               "6-enemy008 enemy008",
               23271.0f,
               3328.0f,
               -14554.0f,
               1.0f,
               UINT32_C(0xff8090a0)) ||
           pc_draw3d_text_matches(
               draw,
               "7-enemy014 enemy014",
               22272.0f,
               3328.0f,
               -13671.0f,
               1.0f,
               UINT32_C(0xff8090a0)) ||
           pc_draw3d_text_matches(
               draw,
               "8-enemy129 enemy129",
               21492.0f,
               3328.0f,
               -14861.0f,
               1.0f,
               UINT32_C(0xff8090a0));
}

static size_t pc_count_draw3d_text_without_recovered_hud_owner(
    const JPBGameRuntime *runtime)
{
    size_t count = 0;
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->draw3dTextDrawCount;
         ++draw_index) {
        if (!pc_draw3d_text_has_recovered_hud_owner(
                &runtime->draw3dTextDraws[draw_index])) {
            ++count;
        }
    }
    return count;
}

static int pc_sprite_display_matches(
    const JPBGameRuntimeSpriteDisplay *draw,
    int type,
    const char *filename_fragment,
    int x,
    int y,
    int width,
    int height,
    int clut,
    int texture_width,
    int texture_height)
{
    const char *filename;

    if (draw == NULL || filename_fragment == NULL ||
        draw->material == NULL) {
        return 0;
    }
    filename = draw->material->filename;
    return strstr(filename, filename_fragment) != NULL &&
           draw->type == type &&
           draw->x == x &&
           draw->y == y &&
           draw->width == width &&
           draw->height == height &&
           draw->clut == clut &&
           draw->textureWidth == texture_width &&
           draw->textureHeight == texture_height;
}

static int pc_runtime_has_sprite_display(
    const JPBGameRuntime *runtime,
    int type,
    const char *filename_fragment,
    int x,
    int y,
    int width,
    int height,
    int clut,
    int texture_width,
    int texture_height)
{
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->spriteDisplayDrawCount;
         ++draw_index) {
        if (pc_sprite_display_matches(
                &runtime->spriteDisplayDraws[draw_index],
                type,
                filename_fragment,
                x,
                y,
                width,
                height,
                clut,
                texture_width,
                texture_height)) {
            return 1;
        }
    }
    return 0;
}

static int pc_psx_texture_draw_matches(
    const JPBGameRuntimePsxTextureDraw *draw,
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int red,
    int green,
    int blue)
{
    return draw != NULL &&
           draw->texture == texture &&
           fabsf(draw->x - x) < 0.001f &&
           fabsf(draw->y - y) < 0.001f &&
           fabsf(draw->width - width) < 0.001f &&
           fabsf(draw->height - height) < 0.001f &&
           draw->transparency == transparency &&
           draw->red == red &&
           draw->green == green &&
           draw->blue == blue;
}

static int pc_runtime_has_psx_texture_draw(
    const JPBGameRuntime *runtime,
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int red,
    int green,
    int blue)
{
    size_t draw_index;

    if (runtime == NULL) {
        return 0;
    }
    for (draw_index = 0;
         draw_index < runtime->psxTextureDrawCount;
         ++draw_index) {
        if (pc_psx_texture_draw_matches(
                &runtime->psxTextureDraws[draw_index],
                texture,
                x,
                y,
                width,
                height,
                transparency,
                red,
                green,
                blue)) {
            return 1;
        }
    }
    return 0;
}

static int pc_validate_normal_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           runtime->playerHudTileDrawCount == 6 &&
           runtime->playerHudTileDroppedCount == 0 &&
           runtime->playerHudTileCompositePixelCount != 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_score_panel_960) == 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_normal_item_panel_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_left_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_bar_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_right_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_left_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_bar_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_right_960) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               45,
               "a_detonator",
               22,
               428,
               96,
               96,
               8,
               128,
               128) &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_960 &&
           pc_runtime_has_text_draw(
               runtime,
               L"0000000",
               24,
               31,
               3.24f,
               1) &&
           pc_runtime_has_text_draw(
               runtime,
               L"0",
               94,
               484,
               1.8f,
               1);
}

static int pc_validate_normal_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           runtime->playerHudTileDrawCount == 6 &&
           runtime->playerHudTileDroppedCount == 0 &&
           runtime->playerHudTileCompositePixelCount != 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_score_panel_1080) == 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_normal_item_panel_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_left_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_bar_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_right_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_left_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_bar_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_right_1080) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               45,
               "a_detonator",
               44,
               856,
               192,
               192,
               8,
               128,
               128) &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_1080 &&
           pc_runtime_has_text_draw(
               runtime,
               L"0000000",
               48,
               62,
               3.24f,
               1) &&
           pc_runtime_has_text_draw(
               runtime,
               L"0",
               188,
               968,
               1.8f,
               1);
}

static int pc_validate_normal_hud_proof_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           runtime->screenDrawDroppedCount == 0 &&
           runtime->textDrawDroppedCount == 0 &&
           runtime->playerHudTileDrawCount >= 24 &&
           runtime->playerHudTileDroppedCount == 0 &&
           runtime->playerHudTileCompositePixelCount >= 4000 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_score_panel_1080) == 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_normal_item_panel_proof_1080) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               45,
               "a_detonator",
               44,
               856,
               192,
               192,
               8,
               128,
               128) &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_1080 &&
           runtime->screenDrawCompositePixelCount != 0 &&
           runtime->textDrawCompositePixelCount != 0 &&
           pc_runtime_has_text_draw(
               runtime,
               L"0000000",
               48,
               62,
               3.24f,
               130) &&
           pc_runtime_has_text_draw(
               runtime,
               L"0",
               188,
               968,
               1.8f,
               130);
}

static int pc_validate_core_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_score_panel_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_item_panel_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_life_bar_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_force_bar_960) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               40,
               "a_meter_main",
               24,
               18,
               240,
               102,
               8,
               1024,
               512) &&
           pc_runtime_has_sprite_display(
               runtime,
               45,
               "a_detonator",
               22,
               428,
               96,
               96,
               8,
               128,
               128) &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_960 &&
           pc_runtime_has_text_draw(
               runtime,
               L"0000000",
               106,
               30,
               3.24f,
               1) &&
           pc_runtime_has_text_draw(
               runtime,
               L"0",
               94,
               484,
               1.8f,
               1);
}

static int pc_validate_core_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_score_panel_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_item_panel_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_life_bar_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_force_bar_1080) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               40,
               "a_meter_main",
               48,
               36,
               480,
               204,
               8,
               1024,
               512) &&
           pc_runtime_has_sprite_display(
               runtime,
               45,
               "a_detonator",
               44,
               856,
               192,
               192,
               8,
               128,
               128) &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_1080 &&
           pc_runtime_has_text_draw(
               runtime,
               L"0000000",
               212,
               60,
               3.24f,
               1) &&
           pc_runtime_has_text_draw(
               runtime,
               L"0",
               188,
               968,
               1.8f,
               1);
}

static int pc_validate_lifetile_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           runtime->playerHudTileDrawCount == 6 &&
           runtime->playerHudTileDroppedCount == 0 &&
           runtime->playerHudTileCompositePixelCount != 0 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           pc_runtime_has_sprite_display(
               runtime,
               45,
               "a_detonator",
               22,
               428,
               96,
               96,
               8,
               128,
               128) &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_960 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_left_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_bar_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_right_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_left_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_bar_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_right_960) == 1 &&
           pc_runtime_has_text_draw(
               runtime,
               L"0000000",
               24,
               31,
               3.24f,
               1) &&
           pc_runtime_has_text_draw(
               runtime,
               L"0",
               94,
               484,
               1.8f,
               1);
}

static int pc_validate_lifetile_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           runtime->playerHudTileDrawCount == 6 &&
           runtime->playerHudTileDroppedCount == 0 &&
           runtime->playerHudTileCompositePixelCount != 0 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           pc_runtime_has_sprite_display(
               runtime,
               45,
               "a_detonator",
               44,
               856,
               192,
               192,
               8,
               128,
               128) &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_1080 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_core_item_panel_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_left_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_bar_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_health_right_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_left_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_bar_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_lifetile_force_right_1080) == 1 &&
           pc_runtime_has_text_draw(
               runtime,
               L"0000000",
               48,
               62,
               3.24f,
               1) &&
           pc_runtime_has_text_draw(
               runtime,
               L"0",
               188,
               968,
               1.8f,
               1);
}

static int pc_validate_projected_lifetile_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           runtime->playerHudTileDrawCount == 15 &&
           runtime->playerHudTileDroppedCount == 0 &&
           runtime->playerHudTileCompositePixelCount != 0 &&
           runtime->screenDrawDroppedCount == 0 &&
           pc_runtime_has_expected_player_hud_tiles(
               runtime,
               kProjectedLifeTile960,
               sizeof(kProjectedLifeTile960) /
                   sizeof(kProjectedLifeTile960[0]));
}

static int pc_validate_projected_lifetile_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           runtime->playerHudTileDrawCount == 15 &&
           runtime->playerHudTileDroppedCount == 0 &&
           runtime->playerHudTileCompositePixelCount != 0 &&
           runtime->screenDrawDroppedCount == 0 &&
           pc_runtime_has_expected_player_hud_tiles(
               runtime,
               kProjectedLifeTile1080,
               sizeof(kProjectedLifeTile1080) /
                   sizeof(kProjectedLifeTile1080[0]));
}

static int pc_validate_debug_labels_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           runtime->draw3dTextDrawCount == 1 &&
           runtime->draw3dTextDroppedCount == 0 &&
           pc_runtime_has_draw3d_text(
               runtime,
               "0- ai 0\nt-LAND\n   ",
               6.0f,
               102.0f,
               22.0f,
               1.0f,
               UINT32_C(0xff8090a0));
}

static int pc_validate_debug_labels_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           runtime->draw3dTextDrawCount == 1 &&
           runtime->draw3dTextDroppedCount == 0 &&
           pc_runtime_has_draw3d_text(
               runtime,
               "0- ai 0\nt-LAND\n   ",
               18.0f,
               136.0f,
               50.0f,
               1.0f,
               UINT32_C(0xff8090a0));
}

static int pc_validate_debug_labels3_hud(
    const JPBGameRuntime *runtime)
{
    return runtime != NULL &&
           runtime->draw3dTextDrawCount == 7 &&
           runtime->draw3dTextDroppedCount == 0 &&
           pc_runtime_has_draw3d_text(
               runtime,
               "2-enemy000 enemy000",
               20224.0f,
               3328.0f,
               -14848.0f,
               1.0f,
               UINT32_C(0xff8090a0)) &&
           pc_runtime_has_draw3d_text(
               runtime,
               "3-enemy001 enemy001",
               22157.0f,
               3328.0f,
               -14976.0f,
               1.0f,
               UINT32_C(0xff8090a0)) &&
           pc_runtime_has_draw3d_text(
               runtime,
               "4-enemy004 enemy004",
               22823.0f,
               3328.0f,
               -15181.0f,
               1.0f,
               UINT32_C(0xff8090a0)) &&
           pc_runtime_has_draw3d_text(
               runtime,
               "5-enemy005 enemy005",
               22580.0f,
               3328.0f,
               -15386.0f,
               1.0f,
               UINT32_C(0xff8090a0)) &&
           pc_runtime_has_draw3d_text(
               runtime,
               "6-enemy008 enemy008",
               23271.0f,
               3328.0f,
               -14554.0f,
               1.0f,
               UINT32_C(0xff8090a0)) &&
           pc_runtime_has_draw3d_text(
               runtime,
               "7-enemy014 enemy014",
               22272.0f,
               3328.0f,
               -13671.0f,
               1.0f,
               UINT32_C(0xff8090a0)) &&
           pc_runtime_has_draw3d_text(
               runtime,
               "8-enemy129 enemy129",
               21492.0f,
               3328.0f,
               -14861.0f,
               1.0f,
               UINT32_C(0xff8090a0));
}

static int pc_validate_enemy_radar_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           OptionStruct.ScreenWidth == 960 &&
           OptionStruct.ScreenHeight == 540 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_enemy_radar_background_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_enemy_radar_player_marker_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_enemy_radar_red_marker_a_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_enemy_radar_green_marker_960) == 1;
}

static int pc_validate_enemy_radar_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           OptionStruct.ScreenWidth == 1920 &&
           OptionStruct.ScreenHeight == 1080 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_enemy_radar_background_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_enemy_radar_player_marker_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_enemy_radar_red_marker_a_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_enemy_radar_green_marker_1080) == 1;
}

static int pc_validate_p2_core_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           GameStruct.NumPlayers == 2 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_p2_core_score_panel_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_p2_core_item_panel_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_p2_core_life_bar_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_p2_core_force_bar_960) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               40,
               "a_meter_main",
               936,
               18,
               -240,
               102,
               8,
               1024,
               512) &&
           pc_runtime_has_sprite_display(
               runtime,
               46,
               "a_bolt",
               842,
               428,
               96,
               96,
               8,
               128,
               128) &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_P2_960 &&
           pc_runtime_has_text_draw(
               runtime,
               L"0009001",
               708,
               30,
               3.24f,
               1) &&
           pc_runtime_has_text_draw(
               runtime,
               L"4",
               914,
               484,
               1.8f,
               1);
}

static int pc_validate_p2_core_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           GameStruct.NumPlayers == 2 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_p2_core_score_panel_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_p2_core_item_panel_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_p2_core_life_bar_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_p2_core_force_bar_1080) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               40,
               "a_meter_main",
               1872,
               36,
               -480,
               204,
               8,
               1024,
               512) &&
           pc_runtime_has_sprite_display(
               runtime,
               46,
               "a_bolt",
               1684,
               856,
               192,
               192,
               8,
               128,
               128) &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_P2_1080 &&
           pc_runtime_has_text_draw(
               runtime,
               L"0009001",
               1416,
               60,
               3.24f,
               1) &&
           pc_runtime_has_text_draw(
               runtime,
               L"4",
               1828,
               968,
               1.8f,
               1);
}

static int pc_validate_continue_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_960 &&
           runtime->creditHudTextureAlphaModulatedPixelCount != 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_first_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_second_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_third_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_fourth_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_fifth_960) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_wrapped_960) == 1 &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 428, 22, 27, 27, 8, 128, 128) &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 447, 22, 27, 27, 8, 128, 128) &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 466, 22, 27, 27, 8, 128, 128) &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 485, 22, 27, 27, 8, 128, 128) &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 504, 22, 27, 27, 8, 128, 128) &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 466, 41, 27, 27, 8, 128, 128);
}

static int pc_validate_continue_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_1080 &&
           runtime->creditHudTextureAlphaModulatedPixelCount != 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_first_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_second_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_third_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_fourth_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_fifth_1080) == 1 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_continue_credit_wrapped_1080) == 1 &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 857, 44, 54, 54, 8, 128, 128) &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 895, 44, 54, 54, 8, 128, 128) &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 933, 44, 54, 54, 8, 128, 128) &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 971, 44, 54, 54, 8, 128, 128) &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 1009, 44, 54, 54, 8, 128, 128) &&
           pc_runtime_has_sprite_display(
               runtime, 49, "a_credit", 933, 82, 54, 54, 8, 128, 128);
}

static int pc_validate_rescue_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_960 &&
           runtime->rescueHudTextureAlphaModulatedPixelCount != 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_rescue_counter_960) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               44,
               "a_maiden",
               432,
               428,
               96,
               96,
               8,
               512,
               512) &&
           pc_runtime_has_text_draw(
               runtime,
               L"5",
               504,
               484,
               1.8f,
               1);
}

static int pc_validate_rescue_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           runtime->itemHudTextureAlphaModulatedPixelCount ==
               PC_HUD_ITEM_PIXELS_1080 &&
           runtime->rescueHudTextureAlphaModulatedPixelCount != 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_rescue_counter_1080) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               44,
               "a_maiden",
               864,
               856,
               192,
               192,
               8,
               512,
               512) &&
           pc_runtime_has_text_draw(
               runtime,
               L"5",
               1008,
               968,
               1.8f,
               1);
}

static int pc_validate_pilot_counter_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           runtime->itemHudTextureAlphaModulatedPixelCount == 0 &&
           runtime->rescueHudTextureAlphaModulatedPixelCount != 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_pilot_counter_960) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               43,
               "a_pilot",
               432,
               428,
               96,
               96,
               8,
               512,
               512) &&
           pc_runtime_has_text_draw(
               runtime,
               L"7",
               504,
               484,
               1.8f,
               1);
}

static int pc_validate_pilot_counter_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           runtime->spriteDisplayDroppedCount == 0 &&
           runtime->itemHudTextureAlphaModulatedPixelCount == 0 &&
           runtime->rescueHudTextureAlphaModulatedPixelCount != 0 &&
           pc_count_screen_draws_matching(
               runtime,
               pc_screen_draw_is_pilot_counter_1080) == 1 &&
           pc_runtime_has_sprite_display(
               runtime,
               43,
               "a_pilot",
               864,
               856,
               192,
               192,
               8,
               512,
               512) &&
           pc_runtime_has_text_draw(
               runtime,
               L"7",
               1008,
               968,
               1.8f,
               1);
}

static int pc_validate_countdown_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           pc_runtime_has_text_draw(
               runtime,
               L"035",
               435,
               464,
               3.0f,
               127) &&
           pc_runtime_has_text_draw(
               runtime,
               L"sec",
               496,
               479,
               1.5f,
               127);
}

static int pc_validate_countdown_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           pc_runtime_has_text_draw(
               runtime,
               L"035",
               871,
               928,
               3.0f,
               127) &&
           pc_runtime_has_text_draw(
               runtime,
               L"sec",
               993,
               959,
               1.5f,
               127);
}

static int pc_validate_countdown_kill_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           pc_runtime_has_text_draw(
               runtime,
               L"015",
               24,
               100,
               3.0f,
               127) &&
           pc_runtime_has_text_draw(
               runtime,
               L"t",
               24,
               130,
               1.5f,
               127);
}

static int pc_validate_countdown_kill_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           pc_runtime_has_text_draw(
               runtime,
               L"015",
               48,
               200,
               3.0f,
               127) &&
           pc_runtime_has_text_draw(
               runtime,
               L"t",
               48,
               260,
               1.5f,
               127);
}

static int pc_validate_countdown_success_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           pc_runtime_has_text_draw_mode(
               runtime,
               L"SUCCESS",
               2,
               480,
               100,
               1.0f,
               128);
}

static int pc_validate_countdown_success_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           pc_runtime_has_text_draw_mode(
               runtime,
               L"SUCCESS",
               2,
               960,
               200,
               1.0f,
               128);
}

static int pc_validate_countdown_fail_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           pc_runtime_has_text_draw_mode(
               runtime,
               L"FAILED",
               2,
               480,
               100,
               1.0f,
               128);
}

static int pc_validate_countdown_fail_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           pc_runtime_has_text_draw_mode(
               runtime,
               L"FAILED",
               2,
               960,
               200,
               1.0f,
               128);
}

static int pc_validate_hangar_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           pc_runtime_has_text_draw(
               runtime,
               L"400",
               435,
               406,
               3.0f,
               127) &&
           pc_runtime_has_text_draw(
               runtime,
               L"sec",
               496,
               422,
               1.5f,
               127);
}

static int pc_validate_hangar_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           pc_runtime_has_text_draw(
               runtime,
               L"400",
               871,
               813,
               3.0f,
               127) &&
           pc_runtime_has_text_draw(
               runtime,
               L"sec",
               993,
               844,
               1.5f,
               127);
}

static int pc_validate_arena_hud_960(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 960 &&
           framebuffer->height == 540 &&
           pc_runtime_has_text_draw(
               runtime,
               L"O",
               352,
               142,
               3.0f,
               252);
}

static int pc_validate_arena_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           pc_runtime_has_text_draw(
               runtime,
               L"O",
               832,
               412,
               3.0f,
               252);
}

static int pc_validate_mini4_hud(
    const JPBGameRuntime *runtime)
{
    return runtime != NULL &&
           pc_runtime_has_psx_texture_draw(
               runtime,
               0xb6,
               240.0f,
               184.0f,
               0.0f,
               0.0f,
               0xff,
               0x80,
               0x80,
               0x80) &&
           pc_runtime_has_psx_texture_draw(
               runtime,
               0xb5,
               242.0f,
               184.0f,
               0.0f,
               0.0f,
               0xff,
               0x80,
               0x80,
               0x80) &&
           pc_runtime_has_psx_texture_draw(
               runtime,
               0xb5,
               244.0f,
               184.0f,
               0.0f,
               0.0f,
               0xff,
               0x80,
               0x80,
               0x80);
}

static int pc_validate_mini4_hud_1080(
    const JPBGameRuntime *runtime,
    const JPBSoftwareFramebuffer *framebuffer)
{
    return runtime != NULL &&
           framebuffer != NULL &&
           framebuffer->width == 1920 &&
           framebuffer->height == 1080 &&
           pc_validate_mini4_hud(runtime) &&
           pc_runtime_has_text_draw(
               runtime,
               L"1",
               240,
               184,
               2.0f,
               255) &&
           pc_runtime_has_text_draw(
               runtime,
               L"0",
               242,
               184,
               2.0f,
               255) &&
           pc_runtime_has_text_draw(
               runtime,
               L"0",
               244,
               184,
               2.0f,
               255);
}

static void pc_seed_hud_p2_core_validation(void)
{
    GameStruct.NumPlayers = 2;
    menu_setNumPlayers(2);
    GameStruct.ModelSelect[1] = 1;
    menu_setPlayer(1, 1u);
    GameStruct.aCharacterData[1].Score = 9001;
    GameStruct.aCharacterData[1].Items = 4;
    GameStruct.aCharacterData[1].Energy = 1000;
    GameStruct.aCharacterData[1].Force = 1000;
    GameStruct.aCharacterData[1].MaxEnergy = 1000;
    GameStruct.aCharacterData[1].MaxForce = 1000;
}

static void pc_seed_hud_continue_validation(void)
{
    GameStruct.mNumContinues = 6;
    GameStruct.ContinuesUsed = 0;
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.aCharacterData[1].Energy = 100;
}

static void pc_seed_hud_rescue_validation(void)
{
    LevelSelect = 3;
    GameStruct.CurrentLevel = 3;
    GameStruct.Counter = 5;
    GameStruct.GameState |= UINT32_C(0x01000000);
}

static void pc_seed_hud_pilot_counter_validation(void)
{
    LevelSelect = 11;
    GameStruct.CurrentLevel = 11;
    GameStruct.GameState = 0;
    gPilotDeathCount = 7;
}

static void pc_seed_hud_countdown_validation(void)
{
    LevelSelect = 17;
    GameStruct.CurrentLevel = 17;
    GameStruct.GameState = 0;
    OptionStruct.DebugLevel = 0;
    nextLevel = 0;
    gGlobalTimer = 100;
    zerobss_levelReset = 9001;
}

static void pc_seed_hud_countdown_kill_validation(void)
{
    LevelSelect = 19;
    GameStruct.CurrentLevel = 19;
    GameStruct.GameState = 0;
    OptionStruct.DebugLevel = 0;
    nextLevel = 0;
    gGlobalTimer = 100;
    zerobss_levelReset = 9011;
    allText[427] = L"targets";
}

static void pc_seed_hud_countdown_success_validation(void)
{
    LevelSelect = 14;
    GameStruct.CurrentLevel = 14;
    GameStruct.GameState = 0;
    OptionStruct.DebugLevel = 0;
    nextLevel = 0;
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    zerobss_levelReset = 9012;
    allText[435] = L"SUCCESS";
    level_CountDown(0, 0, 0);
    abGlobalBits[3] = UINT8_C(2);
    level_CountDown(0, 0, 0);
}

static void pc_seed_hud_countdown_fail_validation(void)
{
    LevelSelect = 18;
    GameStruct.CurrentLevel = 18;
    GameStruct.GameState = 0;
    OptionStruct.DebugLevel = 0;
    nextLevel = 0;
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    gGlobalTimer = 100;
    zerobss_levelReset = 9013;
    allText[436] = L"FAILED";
    level_CountDown(1, 0, 0);
    level_CountDown(1, 0, 0);
    gGlobalTimer = 100 + UINT32_C(0x3c00);
    level_CountDown(1, 0, 0);
}

static void pc_seed_hud_hangar_validation(void)
{
    LevelSelect = 9;
    GameStruct.CurrentLevel = 9;
    GameStruct.GameState = 0;
    GameStruct.Counter = 6;
    gGlobalTimer = 100;
    zerobss_levelReset = 9002;
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
}

static void pc_seed_hud_arena_validation(void)
{
    LevelSelect = 25;
    GameStruct.CurrentLevel = 25;
    GameStruct.NumPlayers = 2;
    menu_setNumPlayers(2);
    GameStruct.mNumContinues = 3;
    GameStruct.ContinuesUsed = 3;
    GameStruct.aCharacterData[0].Score = 100;
    GameStruct.aCharacterData[1].Score = 50;
    gaPlayerData[0].playerRoot.objectID = 0;
    gaPlayerData[1].playerRoot.objectID = 1;
    allText[437] = L"ONE WINS";
    allText[438] = L"TWO WINS";
    allText[439] = L"DRAW";
    zerobss_levelReset = 9003;
}

static void pc_seed_hud_mini4_validation(void)
{
    LevelSelect = 15;
    GameStruct.CurrentLevel = 15;
    GameStruct.GameState = 0;
    gDeathCount = 25;
    gPilotDeathCount = 11;
    zerobss_levelReset = 9004;
}

static void pc_seed_hud_damage_validation(void)
{
    memset(damageTracking, 0, sizeof(damageTracking));
    damageTracking[0].total = 20.0f;
}

static void pc_seed_hud_p2_damage_validation(void)
{
    pc_seed_hud_p2_core_validation();
    memset(damageTracking, 0, sizeof(damageTracking));
    damageTracking[1].total = 30.0f;
}

static void pc_seed_hud_offscreen_validation(
    JPBGameRuntime *runtime)
{
    sceneObject *scene;

    if (runtime == NULL || runtime->physics == NULL ||
        runtime->player == NULL ||
        runtime->player->playerRoot.pParent == NULL) {
        return;
    }
    scene = (sceneObject *)runtime->player->playerRoot.pParent;
    runtime->physics->pos.vx = -1000.0f;
    runtime->physics->pos.vy = 0.0f;
    runtime->physics->pos.vz = 100.0f;
    maPhysicsData[0].pos = runtime->physics->pos;
    runtime->physics->physicsRoot.objectID = 0;
    runtime->physics->physicsRoot.pParent = &scene->sceneRoot;
    runtime->player->playerRoot.objectID = 0;
    runtime->player->playerRoot.flags &=
        ~UINT32_C(0x20);
    runtime->player->pFlags &=
        ~UINT32_C(0x00040202);
    gaPlayerData[0].pFlags &=
        ~UINT32_C(0x00040202);
    gaPlayerData[1].pFlags &=
        ~UINT32_C(0x00040202);
    scene->pPlayer = &runtime->player->playerRoot;
    GameStruct.GameState &= ~UINT32_C(0x00000800);
    if (gpWorld != NULL &&
        gpWorld->currentDolly >= 0 &&
        gpWorld->currentDolly < 256) {
        gpWorld->aDolly[gpWorld->currentDolly].flags &=
            ~UINT32_C(0x400);
    }
    gGlobalTimer = UINT32_C(0x2200);
    gSCENE_READY = 1;
    playeronscreen[0] = 1;
}

static void pc_print_screen_draw_trace(
    const JPBGameRuntime *runtime)
{
    size_t draw_index;

    if (runtime == NULL) {
        return;
    }
    for (draw_index = 0;
         draw_index < runtime->screenDrawCount;
         ++draw_index) {
        const JPBGameRuntimeScreenDraw *draw =
            &runtime->screenDraws[draw_index];
        const char *filename =
            draw->texture != NULL && draw->texture->filename[0] != '\0'
                ? draw->texture->filename
                : "<none>";
        const char *hud_owner = "other";

        if (draw->isPlayerHudTile) {
            hud_owner = "player-hud-tile";
        } else if (draw->texture == NULL &&
             !draw->hasSource &&
             ((draw->color.r == 0x10 &&
               draw->color.g == 0xfc &&
               draw->color.b == 0x10) ||
              (draw->color.r == 0x00 &&
               draw->color.g == 0x00 &&
               draw->color.b == 0xff))) {
            hud_owner = "meter-bar";
        } else if (pc_screen_draw_is_damage_tracker(draw)) {
            hud_owner = "damage-tracker";
        } else if (pc_screen_draw_is_kadu_race_bar(draw)) {
            hud_owner = "kadu-race-bar";
        } else if (pc_screen_draw_is_enemy_radar(draw)) {
            hud_owner = "enemy-radar";
        } else if (strstr(filename, "a_meter_main") != NULL) {
            hud_owner = "score-panel";
        } else if (strstr(filename, "a_meter_bars") != NULL) {
            hud_owner = "meter-bar";
        } else if (strstr(filename, "a_detonator") != NULL ||
                   strstr(filename, "a_bolt") != NULL ||
                   strstr(filename, "a_battery") != NULL ||
                   strstr(filename, "a_shield") != NULL) {
            hud_owner = "item-panel";
        } else if (strstr(filename, "a_credit") != NULL ||
                   strstr(filename, "a_lives") != NULL) {
            hud_owner = "continue-credit";
        } else if (strstr(filename, "a_pilot") != NULL ||
                   strstr(filename, "a_maiden") != NULL) {
            hud_owner = "rescue-counter";
        }

        printf(
            "screen_draw[%zu]=(owner=%s,file=%s,dst=%d/%d/%d/%d,"
            "src=%s:%d/%d/%d/%d,material=%dx%d,color=%u/%u/%u/%u,"
            "layer=%.3f,hud=%d)\n",
            draw_index,
            hud_owner,
            filename,
            draw->destination.left,
            draw->destination.top,
            draw->destination.right,
            draw->destination.bottom,
            draw->hasSource ? "yes" : "no",
            draw->source.left,
            draw->source.top,
            draw->source.right,
            draw->source.bottom,
            draw->textureWidth,
            draw->textureHeight,
            (unsigned)draw->color.r,
            (unsigned)draw->color.g,
            (unsigned)draw->color.b,
            (unsigned)draw->color.cd,
            draw->layerDepth,
            draw->isPlayerHudTile);
    }
}

static void pc_print_screen_poly_trace(
    const JPBGameRuntime *runtime)
{
    size_t draw_index;
    size_t captured_count;

    if (runtime == NULL) {
        return;
    }
    captured_count = runtime->screenPolyDrawCount;
    if (captured_count > JPB_GAME_RUNTIME_SCREEN_POLY_CAPACITY) {
        captured_count = JPB_GAME_RUNTIME_SCREEN_POLY_CAPACITY;
    }
    for (draw_index = 0; draw_index < captured_count; ++draw_index) {
        const JPBGameRuntimeScreenPolyDraw *draw =
            &runtime->screenPolyDraws[draw_index];
        const char *filename =
            draw->texture != NULL && draw->texture->filename[0] != '\0'
                ? draw->texture->filename
                : "<none>";
        const char *hud_owner = "other";
        int vertex_index;

        if (pc_screen_poly_is_offscreen_arrow(draw)) {
            hud_owner = "offscreen-arrow";
        }

        printf(
            "screen_poly[%zu]=(owner=%s,file=%s,vertices=%d,"
            "no_scale=%d,dropped=%zu",
            draw_index,
            hud_owner,
            filename,
            draw->vertexCount,
            draw->noScale,
            runtime->screenPolyDroppedCount);
        for (vertex_index = 0;
             vertex_index < draw->vertexCount &&
             vertex_index < JPB_SCREEN_POLY_VERTEX_CAPACITY;
             ++vertex_index) {
            printf(
                ",v%d=%.1f/%.1f/%.4f/%08x/%.3f/%.3f",
                vertex_index,
                draw->vertices[vertex_index].x,
                draw->vertices[vertex_index].y,
                draw->vertices[vertex_index].z,
                (unsigned)draw->vertices[vertex_index].argb,
                draw->vertices[vertex_index].tu,
                draw->vertices[vertex_index].tv);
        }
        printf(")\n");
    }
}

static void pc_print_text_draw_trace(
    const JPBGameRuntime *runtime)
{
    size_t draw_index;

    if (runtime == NULL) {
        return;
    }
    for (draw_index = 0;
         draw_index < runtime->textDrawCount;
         ++draw_index) {
        const JPBGameRuntimeTextDraw *draw =
            &runtime->textDraws[draw_index];
        char text[JPB_GAME_RUNTIME_TEXT_CAPACITY * 4];
        size_t output_index = 0;
        size_t input_index;

        for (input_index = 0;
             input_index < JPB_GAME_RUNTIME_TEXT_CAPACITY &&
             draw->text[input_index] != L'\0' &&
             output_index + 2 < sizeof(text);
             ++input_index) {
            wchar_t ch = draw->text[input_index];

            if (ch == L'\n') {
                text[output_index++] = '\\';
                text[output_index++] = 'n';
            } else if (ch >= 32 && ch < 127) {
                text[output_index++] = (char)ch;
            } else {
                text[output_index++] = '?';
            }
        }
        text[output_index] = '\0';
        printf(
            "text_draw[%zu]=(text=\"%s\",x=%d,y=%d,scale=%.3f,"
            "mode=%d,tint=%d,alpha=%d,style=%d,pixels=%zu,"
            "clip=%d:%d/%d/%d/%d)\n",
            draw_index,
            text,
            draw->x,
            draw->y,
            draw->scale,
            draw->mode,
            draw->tint,
            draw->alpha,
            draw->fontStyle,
            draw->compositePixels,
            draw->clipEnabled,
            draw->clipLeft,
            draw->clipTop,
            draw->clipRight,
            draw->clipBottom);
    }
}

static void pc_print_psx_texture_draw_trace(
    const JPBGameRuntime *runtime)
{
    size_t draw_index;

    if (runtime == NULL) {
        return;
    }
    for (draw_index = 0;
         draw_index < runtime->psxTextureDrawCount;
         ++draw_index) {
        const JPBGameRuntimePsxTextureDraw *draw =
            &runtime->psxTextureDraws[draw_index];

        printf(
            "psx_texture_draw[%zu]=(texture=0x%x,x=%.1f,y=%.1f,"
            "size=%.1f/%.1f,alpha=%u,rgb=%d/%d/%d)\n",
            draw_index,
            draw->texture,
            draw->x,
            draw->y,
            draw->width,
            draw->height,
            draw->transparency,
            draw->red,
            draw->green,
            draw->blue);
    }
}

static void pc_print_draw3d_text_trace(
    const JPBGameRuntime *runtime)
{
    size_t draw_index;

    if (runtime == NULL) {
        return;
    }
    for (draw_index = 0;
         draw_index < runtime->draw3dTextDrawCount;
         ++draw_index) {
        const JPBGameRuntimeDraw3dText *draw =
            &runtime->draw3dTextDraws[draw_index];
        char text[JPB_GAME_RUNTIME_DRAW3D_TEXT_BYTES * 2];
        size_t output_index = 0;
        size_t input_index;

        for (input_index = 0;
             input_index < sizeof(draw->text) &&
             draw->text[input_index] != '\0' &&
             output_index + 2 < sizeof(text);
             ++input_index) {
            char ch = draw->text[input_index];

            if (ch == '\n') {
                text[output_index++] = '\\';
                text[output_index++] = 'n';
            } else if ((unsigned char)ch >= 32 &&
                       (unsigned char)ch < 127) {
                text[output_index++] = ch;
            } else {
                text[output_index++] = '?';
            }
        }
        text[output_index] = '\0';
        printf(
            "draw3d_text[%zu]=(text=\"%s\",x=%.1f,y=%.1f,z=%.1f,"
            "scale=%.3f,color=%08x)\n",
            draw_index,
            text,
            draw->x,
            draw->y,
            draw->z,
            draw->scale,
            (unsigned)draw->color);
    }
}

static void pc_print_sprite_display_trace(
    const JPBGameRuntime *runtime)
{
    size_t draw_index;

    if (runtime == NULL) {
        return;
    }
    for (draw_index = 0;
         draw_index < runtime->spriteDisplayDrawCount;
         ++draw_index) {
        const JPBGameRuntimeSpriteDisplay *draw =
            &runtime->spriteDisplayDraws[draw_index];
        const char *filename =
            draw->material != NULL && draw->material->filename[0] != '\0'
                ? draw->material->filename
                : "<none>";
        const char *hud_owner = "other";

        if (strstr(filename, "a_meter_main") != NULL) {
            hud_owner = "score-panel";
        } else if (strstr(filename, "a_detonator") != NULL ||
                   strstr(filename, "a_bolt") != NULL ||
                   strstr(filename, "a_battery") != NULL ||
                   strstr(filename, "a_shield") != NULL) {
            hud_owner = "item-panel";
        } else if (strstr(filename, "a_credit") != NULL ||
                   strstr(filename, "a_lives") != NULL) {
            hud_owner = "continue-credit";
        } else if (strstr(filename, "a_pilot") != NULL ||
                   strstr(filename, "a_maiden") != NULL) {
            hud_owner = "rescue-counter";
        }

        printf(
            "sprite_display[%zu]=(owner=%s,type=%d,file=%s,x=%d,y=%d,"
            "size=%d/%d,clut=%d,material=%dx%d)\n",
            draw_index,
            hud_owner,
            draw->type,
            filename,
            draw->x,
            draw->y,
            draw->width,
            draw->height,
            draw->clut,
            draw->textureWidth,
            draw->textureHeight);
    }
}

static int pc_write_ppm(
    const char *path, const JPBSoftwareFramebuffer *framebuffer)
{
    FILE *output = fopen(path, "wb");
    uint8_t *row_bytes;
    int y;

    if (output == NULL) {
        return 0;
    }
    row_bytes = (uint8_t *)malloc((size_t)framebuffer->width * 3);
    if (row_bytes == NULL) {
        fclose(output);
        return 0;
    }
    fprintf(
        output,
        "P6\n%d %d\n255\n",
        framebuffer->width,
        framebuffer->height);
    for (y = 0; y < framebuffer->height; ++y) {
        const uint32_t *source =
            framebuffer->pixels +
            (size_t)y * (size_t)framebuffer->stridePixels;
        int x;

        for (x = 0; x < framebuffer->width; ++x) {
            row_bytes[x * 3] = (uint8_t)(source[x] >> 16);
            row_bytes[x * 3 + 1] = (uint8_t)(source[x] >> 8);
            row_bytes[x * 3 + 2] = (uint8_t)source[x];
        }
        if (fwrite(
                row_bytes,
                (size_t)framebuffer->width * 3,
                1,
                output) != 1) {
            free(row_bytes);
            fclose(output);
            return 0;
        }
    }
    free(row_bytes);
    return fclose(output) == 0;
}

#if defined(JPB_PC_HAS_UFBX)
static int pc_fbx_sidecar_path(
    const char *level_path,
    char *fbx_path,
    size_t capacity)
{
    const char *slash;
    const char *backslash;
    const char *dot;
    size_t stem_length;

    if (level_path == NULL || fbx_path == NULL || capacity == 0) {
        return 0;
    }
    slash = strrchr(level_path, '/');
    backslash = strrchr(level_path, '\\');
    dot = strrchr(level_path, '.');
    if (dot == NULL ||
        (slash != NULL && dot < slash) ||
        (backslash != NULL && dot < backslash)) {
        dot = level_path + strlen(level_path);
    }
    stem_length = (size_t)(dot - level_path);
    if (stem_length + sizeof(".fbx") > capacity) {
        return 0;
    }
    memcpy(fbx_path, level_path, stem_length);
    memcpy(fbx_path + stem_length, ".fbx", sizeof(".fbx"));
    return 1;
}

static int pc_attach_fbx_level(
    const char *mesh_path,
    JPBGameRuntime *runtime,
    JPBPcFbxLevel *fbx_level,
    int *fbx_level_loaded)
{
    char fbx_path[4096];
    char fbx_error[512];
    int level_index;

    if (mesh_path == NULL || runtime == NULL || fbx_level == NULL ||
        fbx_level_loaded == NULL) {
        return 0;
    }
    if (*fbx_level_loaded) {
        jpb_PCFreeFbxLevel(fbx_level);
        memset(fbx_level, 0, sizeof(*fbx_level));
        *fbx_level_loaded = 0;
    }
    level_index = jpb_LevelIndexFromPath(mesh_path);
    if (level_index == JPB_LEVEL_INDEX_NONE ||
        !pc_fbx_sidecar_path(
            mesh_path, fbx_path, sizeof(fbx_path)) ||
        GetFileAttributesA(fbx_path) == INVALID_FILE_ATTRIBUTES) {
        return 1;
    }
    if (!jpb_PCLoadFbxLevel(
            fbx_path,
            level_index,
            fbx_level,
            fbx_error,
            sizeof(fbx_error))) {
        fprintf(stderr, "live FBX level import failed: %s\n", fbx_error);
        jpb_PCLog("live FBX level import failed path=%s error=%s",
            fbx_path, fbx_error);
        return 0;
    }
    *fbx_level_loaded = 1;
    jpb_GameRuntimeSetLevelRenderMesh(runtime, &fbx_level->mesh);
    printf(
        "level_fbx=%s batches=%zu vertices=%zu triangles=%zu\n",
        fbx_path,
        fbx_level->mesh.batchCount,
        fbx_level->mesh.vertices,
        fbx_level->mesh.triangles);
    jpb_PCLog("FBX level attached path=%s triangles=%zu",
        fbx_path, fbx_level->mesh.triangles);
    return 1;
}
#endif

static int pc_parse_headless_buttons(
    const char *text, uint32_t *bits)
{
    const char *cursor = text;
    uint32_t parsed = 0;

    if (text == NULL || bits == NULL || *text == '\0') {
        return 0;
    }
    while (*cursor != '\0') {
        const char *end = cursor;
        size_t length;
        uint32_t button = 0;

        while (*end != '\0' && *end != '+' && *end != ',') {
            ++end;
        }
        length = (size_t)(end - cursor);
        if (length == 2 && strncmp(cursor, "up", length) == 0) {
            button = JPB_PAD_UP;
        } else if (
            length == 4 && strncmp(cursor, "left", length) == 0) {
            /* Matched ReadKeyboardInput emits RIGHT for physical left. */
            button = JPB_PAD_RIGHT;
        } else if (
            length == 4 && strncmp(cursor, "down", length) == 0) {
            button = JPB_PAD_DOWN;
        } else if (
            length == 5 && strncmp(cursor, "right", length) == 0) {
            /* Matched ReadKeyboardInput emits LEFT for physical right. */
            button = JPB_PAD_LEFT;
        } else if (
            length == 6 && strncmp(cursor, "attack", length) == 0) {
            button = JPB_PAD_COMBO_NORTH;
        } else if (
            length == 12 &&
            strncmp(cursor, "attack-north", length) == 0) {
            button = JPB_PAD_COMBO_NORTH;
        } else if (
            length == 12 &&
            strncmp(cursor, "attack-south", length) == 0) {
            button = JPB_PAD_COMBO_SOUTH;
        } else if (
            length == 11 &&
            strncmp(cursor, "attack-west", length) == 0) {
            button = JPB_PAD_COMBO_WEST;
        } else if (
            length == 5 && strncmp(cursor, "block", length) == 0) {
            button = JPB_PAD_BLOCK;
        } else if (
            length == 5 && strncmp(cursor, "force", length) == 0) {
            button = JPB_PAD_LEFT_TRIGGER;
        } else if (
            length == 4 && strncmp(cursor, "lock", length) == 0) {
            button = JPB_PAD_LOCK_ON;
        } else if (
            length == 4 && strncmp(cursor, "jump", length) == 0) {
            button = JPB_PAD_JUMP;
        } else if (
            length == 6 && strncmp(cursor, "select", length) == 0) {
            button = JPB_PAD_COMBO_SOUTH;
        } else if (
            length == 4 && strncmp(cursor, "back", length) == 0) {
            button = JPB_PAD_JUMP;
        } else if (
            length != 4 || strncmp(cursor, "none", length) != 0) {
            return 0;
        }
        parsed |= button;
        if (*end == '\0') {
            break;
        }
        cursor = end + 1;
        if (*cursor == '\0') {
            return 0;
        }
    }
    *bits = parsed;
    return 1;
}

static int pc_parse_i16(const char *text, int16_t *value)
{
    char *end = NULL;
    long parsed;

    if (text == NULL || value == NULL || *text == '\0') {
        return 0;
    }
    parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0' ||
        parsed < INT16_MIN || parsed > INT16_MAX) {
        return 0;
    }
    *value = (int16_t)parsed;
    return 1;
}

static const char *pc_control_scheme_name(int scheme)
{
    if (scheme == 0) {
        return "classic";
    }
    if (scheme == 1) {
        return "modern";
    }
    return "unknown";
}

static int pc_parse_control_scheme_override(
    const char *player_text,
    const char *scheme_text,
    int *player_index,
    int *scheme)
{
    int parsed_player;
    int parsed_scheme;

    if (player_text == NULL || scheme_text == NULL ||
        player_index == NULL || scheme == NULL) {
        return 0;
    }
    if (strcmp(player_text, "p1") == 0 ||
        strcmp(player_text, "player1") == 0) {
        parsed_player = 0;
    } else if (strcmp(player_text, "p2") == 0 ||
               strcmp(player_text, "player2") == 0) {
        parsed_player = 1;
    } else {
        return 0;
    }
    if (strcmp(scheme_text, "classic") == 0) {
        parsed_scheme = 0;
    } else if (strcmp(scheme_text, "modern") == 0) {
        parsed_scheme = 1;
    } else {
        return 0;
    }
    *player_index = parsed_player;
    *scheme = parsed_scheme;
    return 1;
}

static void pc_apply_control_scheme_overrides(const PcInput *input)
{
    int player_index;

    if (input == NULL) {
        return;
    }
    for (player_index = 0; player_index < 2; ++player_index) {
        int scheme = input->controllerConfigOverride[player_index];

        if (scheme < 0) {
            continue;
        }
        OptionStruct.ControllerConfig[player_index] = (uint8_t)scheme;
        jpb_PCLog(
            "control scheme override player=%d scheme=%s",
            player_index + 1,
            pc_control_scheme_name(scheme));
    }
}

static int pc_parse_headless_xinput_buttons(
    const char *text, JPBPCXInputGamepad *gamepad)
{
    const char *cursor = text;

    if (text == NULL || gamepad == NULL || *text == '\0') {
        return 0;
    }
    memset(gamepad, 0, sizeof(*gamepad));
    while (*cursor != '\0') {
        const char *end = cursor;
        size_t length;
        uint16_t button = 0;

        while (*end != '\0' && *end != '+' && *end != ',') {
            ++end;
        }
        length = (size_t)(end - cursor);
        if (length == 2 && strncmp(cursor, "up", length) == 0) {
            button = JPB_PC_XINPUT_DPAD_UP;
        } else if (
            length == 4 && strncmp(cursor, "down", length) == 0) {
            button = JPB_PC_XINPUT_DPAD_DOWN;
        } else if (
            length == 4 && strncmp(cursor, "left", length) == 0) {
            button = JPB_PC_XINPUT_DPAD_LEFT;
        } else if (
            length == 5 && strncmp(cursor, "right", length) == 0) {
            button = JPB_PC_XINPUT_DPAD_RIGHT;
        } else if (length == 1 && *cursor == 'a') {
            button = JPB_PC_XINPUT_A;
        } else if (length == 1 && *cursor == 'b') {
            button = JPB_PC_XINPUT_B;
        } else if (length == 1 && *cursor == 'x') {
            button = JPB_PC_XINPUT_X;
        } else if (length == 1 && *cursor == 'y') {
            button = JPB_PC_XINPUT_Y;
        } else if (length == 2 && strncmp(cursor, "lb", length) == 0) {
            button = JPB_PC_XINPUT_LEFT_SHOULDER;
        } else if (length == 2 && strncmp(cursor, "rb", length) == 0) {
            button = JPB_PC_XINPUT_RIGHT_SHOULDER;
        } else if (
            length == 4 && strncmp(cursor, "back", length) == 0) {
            button = JPB_PC_XINPUT_BACK;
        } else if (
            length == 5 && strncmp(cursor, "start", length) == 0) {
            button = JPB_PC_XINPUT_START;
        } else if (length == 2 && strncmp(cursor, "lt", length) == 0) {
            gamepad->leftTrigger = UINT8_MAX;
        } else if (length == 2 && strncmp(cursor, "rt", length) == 0) {
            gamepad->rightTrigger = UINT8_MAX;
        } else if (
            length != 4 || strncmp(cursor, "none", length) != 0) {
            return 0;
        }
        gamepad->buttons |= button;
        if (*end == '\0') {
            break;
        }
        cursor = end + 1;
        if (*cursor == '\0') {
            return 0;
        }
    }
    return 1;
}

static int pc_parse_headless_keyboard(
    const char *text, JPBPCGameplayKeyboardState *keyboard)
{
    const char *cursor = text;

    if (text == NULL || keyboard == NULL || *text == '\0') {
        return 0;
    }
    memset(keyboard, 0, sizeof(*keyboard));
    while (*cursor != '\0') {
        const char *end = cursor;
        size_t length;

        while (*end != '\0' && *end != '+' && *end != ',') {
            ++end;
        }
        length = (size_t)(end - cursor);
        if (length == 1 && *cursor == 'w') {
            keyboard->moveUp = 1;
        } else if (length == 1 && *cursor == 'a') {
            keyboard->moveLeft = 1;
        } else if (length == 1 && *cursor == 's') {
            keyboard->moveDown = 1;
        } else if (length == 1 && *cursor == 'd') {
            keyboard->moveRight = 1;
        } else if (
            length == 5 && strncmp(cursor, "lctrl", length) == 0) {
            keyboard->walkModifier = 1;
        } else if (length == 1 && *cursor == 't') {
            keyboard->zoomIn = 1;
        } else if (length == 1 && *cursor == 'j') {
            keyboard->comboSouth = 1;
        } else if (length == 1 && *cursor == 'k') {
            keyboard->comboWest = 1;
        } else if (length == 1 && *cursor == 'l') {
            keyboard->comboNorth = 1;
        } else if (length == 1 && *cursor == 'h') {
            keyboard->lockOn = 1;
        } else if (length == 1 && *cursor == 'u') {
            keyboard->jumpBlockChord = 1;
        } else if (length == 1 && *cursor == 'i') {
            keyboard->northBlockChord = 1;
        } else if (length == 1 && *cursor == 'o') {
            keyboard->southBlockChord = 1;
        } else if (length == 1 && *cursor == 'y') {
            keyboard->westBlockChord = 1;
        } else if (
            length == 5 && strncmp(cursor, "shift", length) == 0) {
            keyboard->block = 1;
        } else if (
            (length == 5 && strncmp(cursor, "space", length) == 0) ||
            (length == 5 && strncmp(cursor, "enter", length) == 0)) {
            keyboard->jump = 1;
        } else if (
            length == 6 && strncmp(cursor, "escape", length) == 0) {
            keyboard->start = 1;
        } else if (
            length != 4 || strncmp(cursor, "none", length) != 0) {
            return 0;
        }
        if (*end == '\0') {
            break;
        }
        cursor = end + 1;
        if (*cursor == '\0') {
            return 0;
        }
    }
    return 1;
}

static void pc_select_headless_phase(
    PcInput *input, int active_frame)
{
    int phase;

    input->headlessPhaseBits = 0;
    input->headlessPhasePlayerTwoBits = 0;
    input->headlessPhaseIndependentPlayers = 0;
    memset(
        &input->headlessPhaseKeyboard,
        0,
        sizeof(input->headlessPhaseKeyboard));
    input->headlessPhaseKeyboardMask = 0;
    memset(
        input->headlessPhaseGamepads,
        0,
        sizeof(input->headlessPhaseGamepads));
    input->headlessPhaseXInputMask = 0;
    if (active_frame < 0) {
        return;
    }
    for (phase = 0; phase < input->phaseCount; ++phase) {
        if (active_frame < input->phases[phase].frames) {
            input->headlessPhaseBits =
                input->phases[phase].bits;
            input->headlessPhasePlayerTwoBits =
                input->phases[phase].playerTwoBits;
            input->headlessPhaseIndependentPlayers =
                input->phases[phase].independentPlayers;
            input->headlessPhaseKeyboard =
                input->phases[phase].keyboard;
            input->headlessPhaseKeyboardMask =
                input->phases[phase].keyboardMask;
            memcpy(
                input->headlessPhaseGamepads,
                input->phases[phase].gamepads,
                sizeof(input->headlessPhaseGamepads));
            input->headlessPhaseXInputMask =
                input->phases[phase].xinputMask;
            return;
        }
        active_frame -= input->phases[phase].frames;
    }
}

static int pc_player_active_motion_index(
    const playerObject *player)
{
    Motion *motion;

    if (player == NULL || player->pMotion == NULL ||
        player->paMotions == NULL) {
        return -1;
    }
    motion = *player->pMotion;
    if (motion < player->paMotions ||
        motion >= player->paMotions + player->maxMotions) {
        return -1;
    }
    return (int)(motion - player->paMotions);
}

static const char *pc_player_active_motion_name(
    const playerObject *player, int motion_index)
{
    return player != NULL && player->paMotions != NULL &&
        motion_index >= 0 && motion_index < player->maxMotions
            ? player->paMotions[motion_index].name
            : "<invalid>";
}

static int pc_player_authored_attack_motion(
    const playerObject *player, uint32_t attack_bit)
{
    const char *input_name;
    int index;

    if (player == NULL || player->paCombos == NULL ||
        player->maxCombos <= 0) {
        return -1;
    }
    switch (attack_bit) {
    case JPB_PAD_COMBO_NORTH:
        input_name = "n";
        break;
    case JPB_PAD_COMBO_SOUTH:
        input_name = "s";
        break;
    case JPB_PAD_COMBO_WEST:
        input_name = "w";
        break;
    default:
        return -1;
    }
    for (index = 0; index < player->maxCombos; ++index) {
        const Combo *combo = &player->paCombos[index];

        /*
         * combo_InitComboData has already applied the retail 59-motion Jedi
         * offset encoded by CMB flag 0x04000000.  The one-character entries
         * are therefore the exact per-character base attacks selected by
         * combo_ReadCombo/combo_CheckCombo, not a shared Obi-Wan table.
         */
        if (combo->Len == 1 && strcmp(combo->String, input_name) == 0) {
            return (int)combo->Index;
        }
    }
    return -1;
}

static int pc_player_is_authored_combo_motion(
    const playerObject *player,
    int motion_index,
    uint32_t observed_attack_bits)
{
    int combo_index;

    if (player == NULL || player->paCombos == NULL ||
        player->maxCombos <= 0 || motion_index < 0) {
        return 0;
    }
    for (combo_index = 0;
         combo_index < player->maxCombos;
         ++combo_index) {
        const Combo *combo = &player->paCombos[combo_index];
        const char *input = combo->String;
        int saw_attack = 0;
        int uses_unobserved_attack = 0;

        if ((int)combo->Index != motion_index) {
            continue;
        }
        for (; *input != '\0'; ++input) {
            uint32_t required_bit = 0;

            if (*input == 'n') {
                required_bit = JPB_PAD_COMBO_NORTH;
            } else if (*input == 's') {
                required_bit = JPB_PAD_COMBO_SOUTH;
            } else if (*input == 'w') {
                required_bit = JPB_PAD_COMBO_WEST;
            }
            if (required_bit != 0) {
                saw_attack = 1;
                if ((observed_attack_bits & required_bit) == 0) {
                    uses_unobserved_attack = 1;
                    break;
                }
            }
        }
        if (saw_attack && !uses_unobserved_attack) {
            return 1;
        }
    }
    return 0;
}

static int pc_player_running_attack_motion(uint32_t attack_bit)
{
    switch (attack_bit) {
    case JPB_PAD_COMBO_SOUTH:
        return 92;
    case JPB_PAD_COMBO_WEST:
        return 93;
    case JPB_PAD_COMBO_NORTH:
        return 94;
    default:
        return -1;
    }
}

static void pc_player_combo_tally_summary(
    const playerObject *player,
    int *total,
    int *last_record)
{
    int record;

    *total = 0;
    *last_record = -1;
    if (player == NULL || player->playernum < 0 ||
        player->playernum >= 2) {
        return;
    }
    for (record = 0; record < 32; ++record) {
        int count = comboTally[player->playernum][record];

        if (count != 0) {
            *total += count;
            *last_record = record;
        }
    }
}

static void pc_player_animation_queue_summary(
    const playerObject *player,
    const animObject *animation,
    char *text,
    size_t capacity)
{
    const Node *node;
    int queued = 0;
    size_t length = 0;

    if (text == NULL || capacity == 0) {
        return;
    }
    text[0] = '\0';
    if (player == NULL || animation == NULL) {
        (void)snprintf(text, capacity, "<unavailable>");
        return;
    }
    node = animation->animList.head;
    while (node != NULL && queued < JPB_ANIM_QUEUE_NODE_CAPACITY) {
        const animListNode *sequence = (const animListNode *)node;
        const Motion *motion = sequence->pMotion;
        int motion_index = -1;
        const char *motion_name = "<invalid>";
        int written;

        if (motion != NULL && player->paMotions != NULL &&
            motion >= player->paMotions &&
            motion < player->paMotions + player->maxMotions) {
            motion_index = (int)(motion - player->paMotions);
            motion_name = motion->name;
        }
        written = snprintf(
            text + length,
            capacity - length,
            "%s%d[%.*s]",
            queued != 0 ? ">" : "",
            motion_index,
            JPB_MOTION_NAME_BYTES,
            motion_name);
        if (written < 0 || (size_t)written >= capacity - length) {
            text[capacity - 1] = '\0';
            return;
        }
        length += (size_t)written;
        node = node->next;
        ++queued;
    }
    if (queued == 0) {
        (void)snprintf(text, capacity, "<empty>");
    } else if (node != NULL && length + 4 < capacity) {
        (void)snprintf(text + length, capacity - length, ">...");
    }
}

static const char *pc_force_callback_name(
    JPBPlayerCallback callback)
{
    if (callback == NULL) {
        return "none";
    }
    if (callback == force_ShieldCallBack) {
        return "force_ShieldCallBack";
    }
    if (callback == force_MesmerizeCallBack) {
        return "force_MesmerizeCallBack";
    }
    if (callback == force_StarCallBack) {
        return "force_StarCallBack";
    }
    if (callback == force_CloakCallBack) {
        return "force_CloakCallBack";
    }
    return "unknown";
}

static void pc_print_headless_phase_boundary(
    const PcInput *input,
    const JPBGameRuntime *runtime,
    int completed_active_frames)
{
    int phase;
    int boundary = 0;

    for (phase = 0; phase < input->phaseCount; ++phase) {
        boundary += input->phases[phase].frames;
        if (completed_active_frames == boundary) {
            int motion_index = pc_player_active_motion_index(
                runtime->player);
            const playerObject *second_player =
                runtime->secondPlayerState != NULL
                    ? runtime->inactivePlayer
                    : NULL;
            const animObject *second_animation =
                second_player != NULL &&
                runtime->inactivePlayerScene != NULL
                    ? (const animObject *)
                        runtime->inactivePlayerScene->pAnim
                    : NULL;
            int second_motion_index =
                pc_player_active_motion_index(second_player);
            int combo_tally_total;
            int combo_tally_last;
            char queue_summary[512];
            const char *motion_name =
                pc_player_active_motion_name(
                    runtime->player, motion_index);
            const char *second_motion_name =
                pc_player_active_motion_name(
                    second_player, second_motion_index);
            const VECTOR *landing_node = coll_GetNodeCenter(
                runtime->player->playernum, 3);
            const VECTOR *root_node = coll_GetNodeCenter(
                runtime->player->playernum, 0);
            const VECTOR *second_landing_node = second_player != NULL
                ? coll_GetNodeCenter(second_player->playernum, 3)
                : NULL;
            const VECTOR *second_root_node = second_player != NULL
                ? coll_GetNodeCenter(second_player->playernum, 0)
                : NULL;

            pc_player_combo_tally_summary(
                runtime->player,
                &combo_tally_total,
                &combo_tally_last);
            pc_player_animation_queue_summary(
                runtime->player,
                runtime->animation,
                queue_summary,
                sizeof(queue_summary));

            printf(
                "phase_end=%d active_frames=%d bits=%08x/%08x "
                "player=(%.1f,%.1f,%.1f,facing=%d) "
                "pad=%08x/%08x motion=%d[%.*s] combo=%u "
                "combo_tally=%d/%d pre=%s "
                "anim_frame=%d queue=%s lock=%u "
                "lock_target=(active=%d,id=%d) "
                "lock_flag=%d "
                "state=(flags=%08x,callbacks=%d/%d/%d,force_owner=%s,"
                "resources=%d/%d/%d/%d/%d,"
                "force_data=%lld/%lld/%lld/%lld/%lld/%lld,"
                "air=%.1f/%.1f/%d/%d/%.1f,nodes=%d/%d) "
                "p2=(ready=%d,pos=%.1f/%.1f/%.1f,facing=%d,"
                "pad=%08x/%08x,motion=%d[%.*s],anim_frame=%d,"
                "lock=%u,target=%d,lock_flag=%d,state=%08x,"
                "callbacks=%d/%d/%d,force_owner=%s,"
                "resources=%d/%d/%d/%d/%d,"
                "force_data=%lld/%lld/%lld/%lld/%lld/%lld,"
                "air=%.1f/%.1f/%d/%d/%.1f,"
                "nodes=%d/%d) "
                "enemy=(%.1f,%.1f,%.1f)\n",
                phase,
                completed_active_frames,
                (unsigned)input->phases[phase].bits,
                (unsigned)(input->phases[phase].independentPlayers
                    ? input->phases[phase].playerTwoBits
                    : input->phases[phase].bits),
                runtime->physics->pos.vx,
                runtime->physics->pos.vy,
                runtime->physics->pos.vz,
                runtime->physics->angle.vy,
                (unsigned)runtime->player->playerPad.cpad[0],
                (unsigned)runtime->player->playerPad.cpad[1],
                motion_index,
                JPB_MOTION_NAME_BYTES,
                motion_name,
                motion_index >= 0
                    ? (unsigned)runtime->player
                          ->paMotions[motion_index].combo
                    : 0u,
                combo_tally_total,
                combo_tally_last,
                runtime->player->PreMotion,
                runtime->animation->animFrameIndex /
                    JPB_FIXED_ONE,
                queue_summary,
                (unsigned)runtime->animation->Lock,
                runtime->player->locked != NULL,
                runtime->player->locked != NULL
                    ? (int)runtime->player->locked->playerID
                    : -1,
                (runtime->player->pFlags &
                 UINT32_C(0x00400000)) != 0,
                (unsigned)runtime->player->pFlags,
                runtime->player->pMainCallBack != NULL,
                runtime->player->pMotionCallBack != NULL,
                runtime->player->pForceCallBack != NULL,
                pc_force_callback_name(
                    runtime->player->pForceCallBack),
                game_gGetEnergy(runtime->player->playernum),
                game_gGetMaxEnergy(runtime->player->playernum),
                game_gGetForce(runtime->player->playernum),
                game_gGetMaxForce(runtime->player->playernum),
                game_gGetItemCount(runtime->player->playernum),
                (long long)runtime->player->forceData[0],
                (long long)runtime->player->forceData[1],
                (long long)runtime->player->forceData[2],
                (long long)runtime->player->forceData[3],
                (long long)runtime->player->forceData[4],
                (long long)runtime->player->forceData[5],
                runtime->physics->airmov.vy,
                runtime->physics->mov.vy,
                (int)runtime->physics->airTime,
                (int)runtime->physics->realAirTime,
                runtime->physics->airGround,
                landing_node != NULL ? landing_node->vy : INT_MIN,
                root_node != NULL ? root_node->vy : INT_MIN,
                second_player != NULL,
                runtime->inactivePlayerPhysics != NULL
                    ? runtime->inactivePlayerPhysics->pos.vx
                    : 0.0f,
                runtime->inactivePlayerPhysics != NULL
                    ? runtime->inactivePlayerPhysics->pos.vy
                    : 0.0f,
                runtime->inactivePlayerPhysics != NULL
                    ? runtime->inactivePlayerPhysics->pos.vz
                    : 0.0f,
                runtime->inactivePlayerPhysics != NULL
                    ? runtime->inactivePlayerPhysics->angle.vy
                    : 0,
                second_player != NULL
                    ? (unsigned)second_player->playerPad.cpad[0]
                    : 0u,
                second_player != NULL
                    ? (unsigned)second_player->playerPad.cpad[1]
                    : 0u,
                second_motion_index,
                JPB_MOTION_NAME_BYTES,
                second_motion_name,
                second_animation != NULL
                    ? second_animation->animFrameIndex /
                        JPB_FIXED_ONE
                    : 0,
                second_animation != NULL
                    ? (unsigned)second_animation->Lock
                    : 0u,
                second_player != NULL && second_player->locked != NULL
                    ? (int)second_player->locked->playerID
                    : -1,
                second_player != NULL &&
                    (second_player->pFlags &
                     UINT32_C(0x00400000)) != 0,
                second_player != NULL
                    ? (unsigned)second_player->pFlags
                    : 0u,
                second_player != NULL &&
                    second_player->pMainCallBack != NULL,
                second_player != NULL &&
                    second_player->pMotionCallBack != NULL,
                second_player != NULL &&
                    second_player->pForceCallBack != NULL,
                pc_force_callback_name(
                    second_player != NULL
                        ? second_player->pForceCallBack
                        : NULL),
                second_player != NULL
                    ? game_gGetEnergy(second_player->playernum)
                    : 0,
                second_player != NULL
                    ? game_gGetMaxEnergy(second_player->playernum)
                    : 0,
                second_player != NULL
                    ? game_gGetForce(second_player->playernum)
                    : 0,
                second_player != NULL
                    ? game_gGetMaxForce(second_player->playernum)
                    : 0,
                second_player != NULL
                    ? game_gGetItemCount(second_player->playernum)
                    : 0,
                second_player != NULL
                    ? (long long)second_player->forceData[0]
                    : 0ll,
                second_player != NULL
                    ? (long long)second_player->forceData[1]
                    : 0ll,
                second_player != NULL
                    ? (long long)second_player->forceData[2]
                    : 0ll,
                second_player != NULL
                    ? (long long)second_player->forceData[3]
                    : 0ll,
                second_player != NULL
                    ? (long long)second_player->forceData[4]
                    : 0ll,
                second_player != NULL
                    ? (long long)second_player->forceData[5]
                    : 0ll,
                runtime->inactivePlayerPhysics != NULL
                    ? runtime->inactivePlayerPhysics->airmov.vy
                    : 0.0f,
                runtime->inactivePlayerPhysics != NULL
                    ? runtime->inactivePlayerPhysics->mov.vy
                    : 0.0f,
                runtime->inactivePlayerPhysics != NULL
                    ? (int)runtime->inactivePlayerPhysics->airTime
                    : 0,
                runtime->inactivePlayerPhysics != NULL
                    ? (int)runtime->inactivePlayerPhysics->realAirTime
                    : 0,
                runtime->inactivePlayerPhysics != NULL
                    ? runtime->inactivePlayerPhysics->airGround
                    : 0.0f,
                second_landing_node != NULL
                    ? second_landing_node->vy
                    : INT_MIN,
                second_root_node != NULL
                    ? second_root_node->vy
                    : INT_MIN,
                runtime->enemyPhysics != NULL
                    ? runtime->enemyPhysics->pos.vx
                    : 0.0f,
                runtime->enemyPhysics != NULL
                    ? runtime->enemyPhysics->pos.vy
                    : 0.0f,
                runtime->enemyPhysics != NULL
                    ? runtime->enemyPhysics->pos.vz
                    : 0.0f);
            return;
        }
    }
}

static void pc_append_pad_name(
    char *text, size_t capacity, const char *name)
{
    size_t length;

    if (text == NULL || capacity == 0 || name == NULL) {
        return;
    }
    length = strlen(text);
    if (length >= capacity - 1) {
        return;
    }
    (void)snprintf(
        text + length,
        capacity - length,
        "%s%s",
        length != 0 ? "|" : "",
        name);
}

static void pc_describe_pad_bits(
    uint32_t bits, char *text, size_t capacity)
{
    struct PadBitName {
        uint32_t bit;
        const char *name;
    };
    static const struct PadBitName names[] = {
        {JPB_PAD_LEFT_TRIGGER, "LT"},
        {JPB_PAD_RIGHT_TRIGGER, "RT"},
        {JPB_PAD_BLOCK, "BLOCK"},
        {JPB_PAD_LOCK_ON, "LOCK"},
        {JPB_PAD_COMBO_NORTH, "NORTH"},
        {JPB_PAD_JUMP, "JUMP"},
        {JPB_PAD_COMBO_SOUTH, "SOUTH"},
        {JPB_PAD_COMBO_WEST, "WEST"},
        {JPB_PAD_ZOOM_IN, "ZOOM_IN"},
        {JPB_PAD_ZOOM_OUT, "ZOOM_OUT"},
        {JPB_PAD_ANALOG_MOVEMENT, "ANALOG"},
        {JPB_PAD_START, "START"},
        {JPB_PAD_UP, "UP"},
        {JPB_PAD_LEFT, "RAW_LEFT/PHYS_RIGHT"},
        {JPB_PAD_DOWN, "DOWN"},
        {JPB_PAD_RIGHT, "RAW_RIGHT/PHYS_LEFT"}
    };
    size_t index;

    if (text == NULL || capacity == 0) {
        return;
    }
    text[0] = '\0';
    for (index = 0; index < sizeof(names) / sizeof(names[0]); ++index) {
        if ((bits & names[index].bit) != 0) {
            pc_append_pad_name(text, capacity, names[index].name);
        }
    }
    if (text[0] == '\0') {
        (void)snprintf(text, capacity, "NONE");
    }
}

static void pc_log_player_controls(
    int frame,
    int player_index,
    playerObject *player,
    PcGameplayLogState *state)
{
    uint32_t pressed;
    uint32_t held;
    int input_type;
    int active_motion_index;
    int energy;
    int max_energy;
    int force;
    int max_force;
    int items;
    float axis_x;
    float axis_y;
    animObject *animation = NULL;
    sceneObject *scene;
    const char *control_motion_name = "<invalid>";
    const char *active_motion_name;
    char pressed_names[256];
    char held_names[256];

    if (player == NULL || state == NULL) {
        return;
    }
    scene = (sceneObject *)player->playerRoot.pParent;
    if (scene != NULL) {
        animation = (animObject *)scene->pAnim;
    }
    pressed = player->playerPad.cpad[0];
    held = player->playerPad.cpad[1];
    input_type = player_index == 0
        ? player1InputType
        : player2InputType;
    axis_x = player_index == 0 ? g_p1X : g_p2X;
    axis_y = player_index == 0 ? g_p1Y : g_p2Y;
    if (player->paMotions != NULL &&
        player->currentMotion >= 0 &&
        player->currentMotion < player->maxMotions) {
        control_motion_name =
            player->paMotions[player->currentMotion].name;
    }
    active_motion_index = pc_player_active_motion_index(player);
    active_motion_name = pc_player_active_motion_name(
        player, active_motion_index);
    energy = game_gGetEnergy(player->playernum);
    max_energy = game_gGetMaxEnergy(player->playernum);
    force = game_gGetForce(player->playernum);
    max_force = game_gGetMaxForce(player->playernum);
    items = game_gGetItemCount(player->playernum);
    if (state->initialized &&
        state->pressed == pressed &&
        state->held == held &&
        state->motion == player->currentMotion &&
        state->activeMotion == active_motion_index &&
        state->inputType == input_type &&
        state->energy == energy &&
        state->maxEnergy == max_energy &&
        state->force == force &&
        state->maxForce == max_force &&
        state->items == items &&
        state->forceCallback == player->pForceCallBack) {
        return;
    }
    pc_describe_pad_bits(
        pressed, pressed_names, sizeof(pressed_names));
    pc_describe_pad_bits(held, held_names, sizeof(held_names));
    jpb_PCLog(
        "controls frame=%d player=%d input=%s "
        "pressed=%08x[%s] held=%08x[%s] axes=(%.3f,%.3f) "
        "control_motion=%d[%.*s] active_motion=%d[%.*s] "
        "previous=%d action_lock=%d anim=(frame=%d,lock=%u) "
        "combo=(pre=%s,held=%08x,release=%08x,slack=%d/%d) "
        "callbacks=(main=%d,motion=%d,force=%d,owner=%s) "
        "resources=(energy=%d/%d,force=%d/%d,items=%d) "
        "flags=%08x facing=%d",
        frame,
        player_index + 1,
        input_type == 0 ? "keyboard" : "xinput",
        (unsigned)pressed,
        pressed_names,
        (unsigned)held,
        held_names,
        axis_x,
        axis_y,
        (int)player->currentMotion,
        JPB_MOTION_NAME_BYTES,
        control_motion_name,
        active_motion_index,
        JPB_MOTION_NAME_BYTES,
        active_motion_name,
        (int)player->previousMotion,
        (int)player->ACTION_LOCK,
        animation != NULL
            ? animation->animFrameIndex / JPB_FIXED_ONE
            : -1,
        animation != NULL ? (unsigned)animation->Lock : 0u,
        player->PreMotion,
        (unsigned)player->heldMask,
        (unsigned)player->releaseMask,
        (int)player->chainSlack,
        (int)player->chainSlackEnd,
        player->pMainCallBack != NULL,
        player->pMotionCallBack != NULL,
        player->pForceCallBack != NULL,
        pc_force_callback_name(player->pForceCallBack),
        energy,
        max_energy,
        force,
        max_force,
        items,
        (unsigned)player->pFlags,
        physics_gGetFacing(&player->playerRoot));
    state->pressed = pressed;
    state->held = held;
    state->motion = player->currentMotion;
    state->activeMotion = active_motion_index;
    state->inputType = input_type;
    state->energy = energy;
    state->maxEnergy = max_energy;
    state->force = force;
    state->maxForce = max_force;
    state->items = items;
    state->forceCallback = player->pForceCallBack;
    state->initialized = 1;
}

static void pc_log_gameplay_controls(
    int frame,
    JPBGameRuntime *runtime,
    PcGameplayLogState states[2])
{
    if (runtime == NULL || states == NULL) {
        return;
    }
    pc_log_player_controls(
        frame, 0, runtime->player, &states[0]);
    if (runtime->secondPlayerState != NULL) {
        pc_log_player_controls(
            frame, 1, runtime->inactivePlayer, &states[1]);
    }
}

static void pc_print_usage(const char *program)
{
    fprintf(
        stderr,
        "usage: %s [<world.jpx>] [--cad actor.cad] [--bmd actor.bmd] "
        "[--cmb actor.cmb] [--player-model id] "
        "[--player-two-cad actor.cad] [--player-two-bmd actor.bmd] "
        "[--player-two-cmb actor.cmb] [--player-two-model id] "
        "[--enemy-cad enemy.cad] [--enemy-bmd enemy.bmd] "
        "[--headless] [--hidden-window] [--scripted-input] "
        "[--mute | --silent-audio] "
        "[--persistence-directory path] [--quickload level] "
        "[--overlay-mode 0|1|2] "
        "[--control-scheme p1|p2 classic|modern] "
        "[--title] [--front-end-flow] [--title-main-select index] "
        "[--title-valid-save] "
        "[--title-character-select] "
        "[--title-character-select-completed] "
        "[--title-character-select-two-player] "
        "[--title-character-select-two-player-completed] "
        "[--title-level-select] "
        "[--headless-move] [--headless-left] "
        "[--headless-down] [--headless-right] "
        "[--headless-attack] [--headless-block] [--headless-lock] "
        "[--input-phase buttons frames] "
        "[--input-phase-p1 buttons frames] "
        "[--input-phase-p2 buttons frames] "
        "[--input-phase-pair p1-buttons p2-buttons frames] "
        "[--headless-keyboard-phase keys frames] "
        "[--headless-xinput-phase buttons frames] "
        "[--headless-xinput-phase-pair p1-buttons p2-buttons frames] "
        "[--headless-stick-pair p1-lx p1-ly p2-lx p2-ly frames] "
        "[--validate-combat] [--validate-jump] "
        "[--validate-multi-enemy] [--validate-radar] "
        "[--validate-radar-1080] "
        "[--validate-hud-core] [--validate-hud-core-1080] "
        "[--validate-hud-normal] [--validate-hud-normal-1080] "
        "[--validate-hud-normal-proof-1080] "
        "[--validate-hud-p2-core] [--validate-hud-p2-core-1080] "
        "[--validate-hud-continue] [--validate-hud-continue-1080] "
        "[--validate-hud-rescue] [--validate-hud-rescue-1080] "
        "[--validate-hud-pilot-counter] "
        "[--validate-hud-pilot-counter-1080] "
        "[--validate-hud-countdown] [--validate-hud-countdown-1080] "
        "[--validate-hud-countdown-kill] "
        "[--validate-hud-countdown-kill-1080] "
        "[--validate-hud-countdown-success] "
        "[--validate-hud-countdown-success-1080] "
        "[--validate-hud-countdown-fail] "
        "[--validate-hud-countdown-fail-1080] "
        "[--validate-hud-hangar] [--validate-hud-hangar-1080] "
        "[--validate-hud-arena] [--validate-hud-arena-1080] "
        "[--validate-hud-mini4] [--validate-hud-mini4-1080] "
        "[--validate-hud-damage] [--validate-hud-damage-1080] "
        "[--validate-hud-damage-compact] "
        "[--validate-hud-damage-compact-1080] "
        "[--validate-hud-damage-p2] "
        "[--validate-hud-damage-p2-1080] "
        "[--validate-hud-damage-p2-compact] "
        "[--validate-hud-damage-p2-compact-1080] "
        "[--validate-hud-kadu] [--validate-hud-kadu-1080] "
        "[--validate-hud-offscreen] [--validate-hud-offscreen-1080] "
        "[--validate-hud-lifetile] [--validate-hud-lifetile-1080] "
        "[--validate-hud-lifetile-projected] "
        "[--validate-hud-lifetile-projected-1080] "
        "[--validate-hud-debug-labels] [--validate-hud-debug-labels-1080] "
        "[--validate-hud-debug-labels3] "
        "[--validate-hud-debug-labels3-1080] "
        "[--validate-hud-owner-coverage] "
        "[--validate-teleport] [--validate-camera-follow] "
        "[--validate-title-audio] [--validate-player-saber] "
        "[--validate-player-projectile] "
        "[--validate-presentation-handoff] "
        "[--validate-audio-handoff] "
        "[--validate-persistence-handoff] "
        "[--validate-neutral-handoff] "
        "[--validate-player-two-sound name] "
        "[--headless-maximum-progression] "
        "[--require-fbx-level] "
        "[--camera-dolly N] "
        "[--camera-diagnostics] "
        "[--framebuffer-size width height] "
        "[--frames N] [--output frame.ppm]\n"
        "with no world path, the installed front end is shown and its "
        "FED/player/enemy assets are resolved relative to the executable\n"
        "--quickload level skips the front end and loads a named installed "
        "level for development\n"
        "keys: arrows/WASD move, J/K/L attacks, Shift block, "
        "Space jump, H lock-on, T zoom, J select, "
        "Escape start/pause\n",
        program);
}

static int pc_collision_storage_contains(
    const JPBGameRuntime *runtime,
    const void *pointer,
    size_t size)
{
    uintptr_t start;
    uintptr_t address;

    if (runtime == NULL ||
        runtime->collisionStorage == NULL ||
        pointer == NULL ||
        size > runtime->collisionStorageSize) {
        return 0;
    }
    start = (uintptr_t)(const void *)runtime->collisionStorage;
    address = (uintptr_t)pointer;
    return address >= start &&
        address - start <= runtime->collisionStorageSize - size;
}

static int pc_camera_index_at_world_position(
    const JPBGameRuntime *runtime,
    int32_t x,
    int32_t y,
    int32_t z)
{
    VECTOR high_point;
    _jheightstuff height_stuff;
    const int32_t *camera_poly;
    uint32_t polygon_word;

    if (leveldata == NULL) {
        return -1;
    }
    high_point.vx = x;
    high_point.vy = y + 0x100;
    high_point.vz = z;
    high_point.pad = 0;
    memset(&height_stuff, 0, sizeof(height_stuff));
    (void)intersec_FindWalkHeight(
        &high_point,
        NULL,
        (objectRoot *)(void *)&height_stuff,
        1);
    if (!pc_collision_storage_contains(
            runtime,
            height_stuff.poly,
            sizeof(*height_stuff.poly))) {
        return -1;
    }
    camera_poly = height_stuff.poly;
    polygon_word = (uint32_t)*camera_poly;
    if ((polygon_word & UINT32_C(0x3c000000)) == 0) {
        if (!pc_collision_storage_contains(
                runtime,
                leveldata - 4,
                5 * sizeof(*leveldata))) {
            return -1;
        }
        camera_poly =
            leveldata +
            (leveldata[-4] >> 11) +
            (int32_t)(((polygon_word >> 14) &
                       UINT32_C(0xff)) * 9U);
    }
    if (!pc_collision_storage_contains(
            runtime, camera_poly, 2 * sizeof(*camera_poly))) {
        return -1;
    }
    return ((const uint8_t *)(const void *)camera_poly)[7] &
        UINT8_C(0x7f);
}

static void pc_print_camera_collision_diagnostics(
    const JPBGameRuntime *runtime)
{
    VECTOR high_point;
    _jheightstuff height_stuff;
    const int32_t *camera_poly;
    uint32_t polygon_word;
    int height;

    if (runtime == NULL || runtime->physics == NULL ||
        leveldata == NULL) {
        return;
    }
    if (runtime->world != NULL) {
        const wsl_ENEMY *director = runtime->enemy;
        const Node *enemy_node;
        unsigned enemy_count = 0;

        printf(
            "camera_world_data=(start=%d,%d,%d,bounds=%d,%d,%d->%d,%d,%d,"
            "size=%d,%d,%d)\n",
            runtime->world->start.vx,
            runtime->world->start.vy,
            runtime->world->start.vz,
            runtime->world->minX,
            runtime->world->minY,
            runtime->world->minZ,
            runtime->world->maxX,
            runtime->world->maxY,
            runtime->world->maxZ,
            runtime->world->sizeX,
            runtime->world->sizeY,
            runtime->world->sizeZ);
        printf(
            "camera_director=(override=%d,current=%d,"
            "enemy=%d,mode=%d,node=%d,opcode=0x%04x,"
            "timer=%d,counters=%u/%u/%u/%u/%u)\n",
            (int)runtime->world->overRideDolly,
            (int)runtime->world->currentDolly,
            director != NULL ? director->enemyID : -1,
            director != NULL ? (int)director->currAIMode : -1,
            director != NULL ? director->aiLocation : -1,
            director != NULL && director->pAINode != NULL
                ? (unsigned)(uint16_t)director->pAINode->opcode
                : 0U,
            director != NULL ? director->aiTimer : 0,
            director != NULL ? (unsigned)director->counter[0] : 0U,
            director != NULL ? (unsigned)director->counter[1] : 0U,
            director != NULL ? (unsigned)director->counter[2] : 0U,
            director != NULL ? (unsigned)director->counter[3] : 0U,
            director != NULL ? (unsigned)director->counter[4] : 0U);
        fputs("camera_director_enemies=(", stdout);
        for (enemy_node = enemyList[mCurEnemyList].head;
             enemy_node != NULL && enemy_count < 20;
             enemy_node = enemy_node->next, ++enemy_count) {
            const wsl_ENEMY *enemy =
                (const wsl_ENEMY *)(const void *)enemy_node;

            printf(
                "%s%u:id=%d/ai=%d/mode=%d/node=%d/op=0x%04x",
                enemy_count == 0 ? "" : ",",
                enemy_count,
                enemy->enemyID,
                enemy->aiNum,
                (int)enemy->currAIMode,
                enemy->aiLocation,
                enemy->pAINode != NULL
                    ? (unsigned)(uint16_t)enemy->pAINode->opcode
                    : 0U);
        }
        puts(")");
        {
            int z_offset;

            fputs("camera_collision_grid=(step=512,rows=", stdout);
            for (z_offset = -2048;
                 z_offset <= 2048;
                 z_offset += 512) {
                int x_offset;

                if (z_offset != -2048) {
                    fputc('/', stdout);
                }
                for (x_offset = -2048;
                     x_offset <= 2048;
                     x_offset += 512) {
                    int camera_index =
                        pc_camera_index_at_world_position(
                            runtime,
                            (int32_t)runtime->physics->pos.vx + x_offset,
                            (int32_t)runtime->physics->pos.vy,
                            (int32_t)runtime->physics->pos.vz + z_offset);

                    if (x_offset != -2048) {
                        fputc(',', stdout);
                    }
                    if (camera_index < 0) {
                        fputc('-', stdout);
                    } else {
                        printf("%d", camera_index);
                    }
                }
            }
            puts(")");
        }
    }
    high_point.vx = (int32_t)runtime->physics->pos.vx;
    high_point.vy = (int32_t)runtime->physics->pos.vy + 0x100;
    high_point.vz = (int32_t)runtime->physics->pos.vz;
    high_point.pad = 0;
    memset(&height_stuff, 0, sizeof(height_stuff));
    height = intersec_FindWalkHeight(
        &high_point,
        NULL,
        (objectRoot *)(void *)&height_stuff,
        1);
    if (!pc_collision_storage_contains(
            runtime,
            height_stuff.poly,
            sizeof(*height_stuff.poly))) {
        printf(
            "camera_collision_probe=(height=%d,poly=none-or-invalid)\n",
            height);
        return;
    }

    camera_poly = height_stuff.poly;
    polygon_word = (uint32_t)*camera_poly;
    if ((polygon_word & UINT32_C(0x3c000000)) == 0) {
        if (!pc_collision_storage_contains(
                runtime,
                leveldata - 4,
                5 * sizeof(*leveldata))) {
            printf(
                "camera_collision_probe=(height=%d,poly=invalid)\n",
                height);
            return;
        }
        camera_poly =
            leveldata +
            (leveldata[-4] >> 11) +
            (int32_t)(((polygon_word >> 14) &
                       UINT32_C(0xff)) * 9U);
    }
    if (!pc_collision_storage_contains(
            runtime, camera_poly, 2 * sizeof(*camera_poly))) {
        printf(
            "camera_collision_probe=(height=%d,poly=invalid)\n",
            height);
        return;
    }
    printf(
        "camera_collision_probe=(height=%d,target_delta=%d,"
        "cube=%td,entry=%td,poly=%td,resolved=%td,"
        "words=0x%08x/0x%08x,camera=%u)\n",
        height,
        (int32_t)runtime->physics->pos.vy - height,
        height_stuff.cube - leveldata,
        height_stuff.entry != NULL
            ? height_stuff.entry - leveldata
            : (ptrdiff_t)-1,
        height_stuff.poly - leveldata,
        camera_poly - leveldata,
        (unsigned)polygon_word,
        (unsigned)(uint32_t)camera_poly[1],
        (unsigned)(((const uint8_t *)(const void *)camera_poly)[7] &
                   UINT8_C(0x7f)));
    if (pc_collision_storage_contains(
            runtime,
            height_stuff.entry,
            sizeof(*height_stuff.entry))) {
        uint32_t entry_word = (uint32_t)*height_stuff.entry;
        const int32_t *polygons =
            leveldata + (uint16_t)entry_word + 2;
        int polygon_count = 0;

        if (!pc_collision_storage_contains(
                runtime,
                leveldata + (uint16_t)entry_word,
                2 * sizeof(*leveldata))) {
            puts("camera_collision_library=(invalid)");
            return;
        }

        printf(
            "camera_collision_library=(entry_word=0x%08x,lib=%u,"
            "header=0x%08x/0x%08x,polygons=",
            (unsigned)entry_word,
            (unsigned)(uint16_t)entry_word,
            (unsigned)(uint32_t)leveldata[(uint16_t)entry_word],
            (unsigned)(uint32_t)leveldata[(uint16_t)entry_word + 1]);
        do {
            uint32_t word0;
            uint32_t word1;

            if (!pc_collision_storage_contains(
                    runtime,
                    polygons,
                    2 * sizeof(*polygons))) {
                fputs(
                    polygon_count == 0
                        ? "invalid"
                        : ",invalid",
                    stdout);
                break;
            }
            word0 = (uint32_t)polygons[0];
            word1 = (uint32_t)polygons[1];

            printf(
                "%s%td:%u%s/0x%08x/0x%08x",
                polygon_count == 0 ? "" : ",",
                polygons - leveldata,
                (unsigned)((word1 >> 24) & UINT32_C(0x7f)),
                polygons == height_stuff.poly ? "*" : "",
                (unsigned)word0,
                (unsigned)word1);
            polygons += 2;
            ++polygon_count;
            if (polygon_count >= 64) {
                break;
            }
        } while (((uint32_t)polygons[-2] &
                  UINT32_C(0xc0000000)) == 0);
        puts(")");
    }
}

static int pc_position_for_multi_enemy_validation(
    JPBGameRuntime *runtime)
{
    wsl_BAP_PLACEMENT *best_center = NULL;
    size_t best_class_count = 0;
    int center_index;

    if (runtime == NULL ||
        runtime->world == NULL ||
        runtime->physics == NULL ||
        runtime->enemyLoadedClassCount < 2) {
        return 0;
    }
    for (center_index = 0;
         center_index < runtime->world->nEnemy;
         ++center_index) {
        wsl_BAP_PLACEMENT *center =
            runtime->world->apEnemy[center_index];
        int actor_nums[JPB_PLAYER_CAPACITY];
        size_t actor_count = 0;
        int placement_index;

        if (center == NULL ||
            center->status != 0 ||
            center->aiDf.ownerType == 0 ||
            (center->aiDf.activeFlags & 1U) == 0 ||
            jpb_GameRuntimeEnemyClassModelId(
                runtime, center->actorNum) < 0) {
            continue;
        }
        for (placement_index = 0;
             placement_index < runtime->world->nEnemy;
             ++placement_index) {
            wsl_BAP_PLACEMENT *placement =
                runtime->world->apEnemy[placement_index];
            int duplicate = 0;
            size_t actor_index;

            if (placement == NULL ||
                placement->status != 0 ||
                placement->aiDf.ownerType == 0 ||
                (placement->aiDf.activeFlags & 1U) == 0 ||
                placement->aiDf.aRange < 0 ||
                jpb_GameRuntimeEnemyClassModelId(
                    runtime, placement->actorNum) < 0) {
                continue;
            }
            if (placement->aiDf.aRange != 0 &&
                (abs(center->loc.vx - placement->loc.vx) >
                     placement->aiDf.aRange ||
                 abs(center->loc.vy - placement->loc.vy) >
                     placement->aiDf.aRange ||
                 abs(center->loc.vz - placement->loc.vz) >
                     placement->aiDf.aRange)) {
                continue;
            }
            for (actor_index = 0;
                 actor_index < actor_count;
                 ++actor_index) {
                if (actor_nums[actor_index] ==
                    placement->actorNum) {
                    duplicate = 1;
                    break;
                }
            }
            if (!duplicate) {
                actor_nums[actor_count++] =
                    placement->actorNum;
                if (actor_count ==
                    runtime->enemyLoadedClassCount) {
                    break;
                }
            }
        }
        if (actor_count > best_class_count) {
            best_class_count = actor_count;
            best_center = center;
        }
    }
    if (best_center == NULL ||
        best_class_count < 2) {
        return 0;
    }
    runtime->physics->pos.vx =
        (float)best_center->loc.vx;
    runtime->physics->pos.vy =
        (float)best_center->loc.vy;
    runtime->physics->pos.vz =
        (float)best_center->loc.vz;
    runtime->physics->vpos.vx = best_center->loc.vx;
    runtime->physics->vpos.vy = best_center->loc.vy;
    runtime->physics->vpos.vz = best_center->loc.vz;
    runtime->physics->snapshotpos =
        runtime->physics->pos;
    runtime->physics->lastpos =
        runtime->physics->pos;
    /* A validation teleport cannot retain contact with its prior polygon. */
    runtime->physics->lastpolyhit = NULL;
    runtime->world->location = best_center->loc;
    return jpb_PhysicsUpdateSceneObject(
               runtime->physics) ==
           JPB_PHYSICS_PARTIAL_OK;
}

static int pc_position_for_uncovered_enemy_validation(
    JPBGameRuntime *runtime)
{
    int index;

    if (runtime == NULL ||
        runtime->world == NULL ||
        runtime->physics == NULL) {
        return 0;
    }
    for (index = 0;
         index < runtime->world->nEnemy;
         ++index) {
        wsl_BAP_PLACEMENT *placement =
            runtime->world->apEnemy[index];

        if (placement == NULL ||
            placement->status != 0 ||
            jpb_GameRuntimeEnemyClassModelId(
                runtime, placement->actorNum) < 0 ||
            jpb_GameRuntimeEnemyClassWasActive(
                runtime, placement->actorNum)) {
            continue;
        }
        /*
         * Some authored classes (FED Destroyers included) are link-triggered
         * and begin with neither activation bit set. Exercise the exact
         * _addEnemy owner against that real placement for this validation
         * pass; no synthetic placement or actor record is introduced.
         */
        if ((placement->aiDf.activeFlags &
             UINT32_C(0x10000001)) == 0) {
            if (_addEnemy(
                    placement, index, -1, 1) == 0) {
                continue;
            }
            placement->status = 1;
        }
        runtime->physics->pos.vx =
            (float)placement->loc.vx;
        runtime->physics->pos.vy =
            (float)placement->loc.vy;
        runtime->physics->pos.vz =
            (float)placement->loc.vz;
        runtime->physics->vpos.vx = placement->loc.vx;
        runtime->physics->vpos.vy = placement->loc.vy;
        runtime->physics->vpos.vz = placement->loc.vz;
        runtime->physics->snapshotpos =
            runtime->physics->pos;
        runtime->physics->lastpos =
            runtime->physics->pos;
        runtime->physics->lastpolyhit = NULL;
        runtime->world->location = placement->loc;
        return jpb_PhysicsUpdateSceneObject(
                   runtime->physics) ==
               JPB_PHYSICS_PARTIAL_OK;
    }
    return 0;
}

static int pc_position_for_combat_validation(
    JPBGameRuntime *runtime)
{
    wsl_ENEMY *enemy;
    playerObject *target_player = NULL;
    physicsObject *target_physics = NULL;
    int64_t nearest_distance = INT64_MAX;
    int32_t facing;

    if (runtime == NULL || runtime->world == NULL ||
        runtime->physics == NULL) {
        return 0;
    }
    for (enemy = (wsl_ENEMY *)enemyList[mCurEnemyList].head;
         enemy != NULL;
         enemy = (wsl_ENEMY *)enemy->node.next) {
        sceneObject *scene;
        physicsObject *physics;
        int64_t dx;
        int64_t dz;
        int64_t distance;

        if (enemy->ownerType != 2 ||
            enemy->active != 1 ||
            enemy->pPlayer == NULL ||
            enemy->pPlayer->playerRoot.pParent == NULL) {
            continue;
        }
        scene = (sceneObject *)enemy->pPlayer->playerRoot.pParent;
        physics = (physicsObject *)scene->pPhysics;
        if (physics == NULL) {
            continue;
        }
        dx = (int64_t)physics->vpos.vx -
             (int64_t)runtime->physics->vpos.vx;
        dz = (int64_t)physics->vpos.vz -
             (int64_t)runtime->physics->vpos.vz;
        distance = dx * dx + dz * dz;
        if (distance < nearest_distance) {
            nearest_distance = distance;
            target_player = enemy->pPlayer;
            target_physics = physics;
        }
    }
    if (target_player == NULL || target_physics == NULL) {
        return 0;
    }
    /*
     * This validation deliberately isolates authored combat after positioning
     * beside a live combatant. The real FED room director owns the opening
     * control lock, so release only the validation player's lock here; normal
     * gameplay continues to obey the authored intro lifecycle.
     */
    runtime->player->pFlags &= ~UINT32_C(2);
    runtime->world->overRideDolly = 0;
    if (runtime->world->currentDolly >= 0 &&
        runtime->world->currentDolly < 256) {
        runtime->world
            ->aDolly[runtime->world->currentDolly]
            .flags &= ~UINT32_C(0x400);
    }
    newcameraflag = 1;
    /* This is a combat-isolation harness, not an AI pursuit test.  Once the
     * nearest live actor is selected, stop its director-owned locomotion so
     * the authored attack and collision/damage pipeline are measured against
     * a stable target.  DamageControl still runs for the actor and may install
     * its own reaction callbacks after contact. */
    target_player->pMainCallBack = NULL;
    memset(&target_physics->constmov, 0,
           sizeof(target_physics->constmov));
    memset(&target_physics->currentmov, 0,
           sizeof(target_physics->currentmov));
    memset(&target_physics->airmov, 0,
           sizeof(target_physics->airmov));
    memset(&target_physics->mov, 0,
           sizeof(target_physics->mov));
    memset(&target_physics->accel, 0,
           sizeof(target_physics->accel));
    runtime->player->target = target_player;
    runtime->physics->pos = target_physics->pos;
    runtime->physics->pos.vz += 64.0f;
    runtime->physics->vpos.vx =
        (int32_t)runtime->physics->pos.vx;
    runtime->physics->vpos.vy =
        (int32_t)runtime->physics->pos.vy;
    runtime->physics->vpos.vz =
        (int32_t)runtime->physics->pos.vz;
    runtime->physics->snapshotpos = runtime->physics->pos;
    runtime->physics->lastpos = runtime->physics->pos;
    runtime->physics->lastpolyhit = NULL;
    facing = ratan2(
        (int32_t)(target_physics->pos.vx -
                  runtime->physics->pos.vx),
        (int32_t)(target_physics->pos.vz -
                  runtime->physics->pos.vz)) &
        0x0fff;
    runtime->physics->angle.vy = facing;
    runtime->physics->face.vy = facing;
    runtime->world->location.vx = runtime->physics->vpos.vx;
    runtime->world->location.vy = runtime->physics->vpos.vy;
    runtime->world->location.vz = runtime->physics->vpos.vz;
    return jpb_PhysicsUpdateSceneObject(
               runtime->physics) ==
           JPB_PHYSICS_PARTIAL_OK;
}

int main(int argc, char **argv)
{
    JPBGameRuntime runtime;
    JPBSoftwareFramebuffer framebuffer;
    JPBSoftwareRenderStats stats = {0};
    JPBPCAudio *audio = NULL;
    PcInput input = {0};
    JPBMenuPlatformHooks menu_hooks = {0};
    LARGE_INTEGER frequency;
    LARGE_INTEGER previous;
    uint32_t *pixels = NULL;
    uint32_t *title_pixels = NULL;
    PcMenuTextureCache *menu_texture_cache = NULL;
    PcPlayerSaberDiagnostics saber_diagnostics = {0};
    PcPlayerSaberActionDiagnostics saber_action_diagnostics = {0};
    PcPlayerSaberDiagnostics second_saber_diagnostics = {
        .playerModel = -1
    };
    PcPlayerSaberActionDiagnostics second_saber_action_diagnostics = {
        .playerModel = -1
    };
    PcDefaultAssets default_assets;
    PcPlayerAssets selected_player_assets;
    PcPlayerAssets validation_player_two_assets;
    const char *mesh_path;
    const char *cad_path = NULL;
    const char *bmd_path = NULL;
    const char *cmb_path = NULL;
    const char *enemy_cad_path = NULL;
    const char *enemy_bmd_path = NULL;
    const char *player_two_cad_path = NULL;
    const char *player_two_bmd_path = NULL;
    const char *player_two_cmb_path = NULL;
    int player_model = 0;
    int player_model_override = 0;
    int player_two_model = 1;
    const char *output_path = NULL;
    int frame_limit = 0;
    int frame_count = 0;
    int index;
    int result;
    int target_clipped = 1;
    FVECTOR target_screen = {0};
    FVECTOR target_view = {0};
    FVECTOR initial_position = {0};
    int jump_airborne_frames = 0;
    int camera_dolly_override = -1;
    int camera_diagnostics = 0;
    int mute = 0;
    int audio_output_enabled = 1;
    unsigned audio_generation_count = 0;
    int title_active = 0;
    int title_diagnostic = 0;
    int front_end_flow = 0;
    int front_end_flow_forced = 0;
    int front_end_playable_handoff = 0;
    const char *quickload_level = NULL;
    const char *persistence_directory = NULL;
    const char *validate_player_two_sound = NULL;
    char validated_player_two_sound_path[MAX_PATH] = {0};
    int player_two_audio_validated = 0;
    int using_installed_assets = 0;
    int title_character_select = 0;
    int title_character_select_completed = 0;
    int framebuffer_width = PC_FRAMEBUFFER_WIDTH;
    int framebuffer_height = PC_FRAMEBUFFER_HEIGHT;
    int framebuffer_size_explicit = 0;
    int title_level_select = 0;
    int title_main_select = -1;
    int title_valid_save = 0;
    int overlay_mode_override = -1;
    unsigned presentation_frame_count = 0;
    unsigned gameplay_handoff_count = 0;
    int exit_code;
    playerObject *resource_second_player = NULL;
#if defined(JPB_PC_HAS_UFBX)
    JPBPcFbxLevel fbx_level = {0};
    int fbx_level_loaded = 0;
#endif

    input.controllerConfigOverride[0] = -1;
    input.controllerConfigOverride[1] = -1;
    jpb_PCLogStart(argc, argv);
    pc_configure_failure_mode();
    jpb_PCLog("startup: resolving assets");
    memset(&default_assets, 0, sizeof(default_assets));
    memset(&selected_player_assets, 0, sizeof(selected_player_assets));
    memset(
        &validation_player_two_assets,
        0,
        sizeof(validation_player_two_assets));
    if (argc < 2 || argv[1][0] == '-') {
        if (!pc_configure_default_assets(&default_assets)) {
            fputs(
                "default installed assets were not found beside the "
                "executable\n",
                stderr);
            return 2;
        }
        mesh_path = default_assets.mesh;
        cad_path = default_assets.cad;
        bmd_path = default_assets.bmd;
        cmb_path = default_assets.cmb;
        enemy_cad_path = default_assets.enemyCad;
        enemy_bmd_path = default_assets.enemyBmd;
        using_installed_assets = 1;
        title_active = 1;
        front_end_flow = 1;
        jpb_PCLog("startup: installed assets found; front end enabled");
        index = 1;
    } else {
        mesh_path = argv[1];
        (void)pc_set_asset_root_from_world_path(mesh_path);
        index = 2;
    }
    for (; index < argc; ++index) {
        if (strcmp(argv[index], "--headless") == 0) {
            input.headless = 1;
        } else if (strcmp(argv[index], "--hidden-window") == 0) {
            input.hiddenWindow = 1;
        } else if (strcmp(argv[index], "--scripted-input") == 0) {
            input.scriptedInput = 1;
        } else if (strcmp(argv[index], "--quickload") == 0 &&
                   index + 1 < argc) {
            quickload_level = argv[++index];
        } else if (strcmp(argv[index], "--overlay-mode") == 0 &&
                   index + 1 < argc) {
            overlay_mode_override = atoi(argv[++index]);
            if (overlay_mode_override < 0 ||
                overlay_mode_override > 2) {
                pc_print_usage(argv[0]);
                return 2;
            }
        } else if (strcmp(argv[index], "--title") == 0) {
            title_active = 1;
            title_diagnostic = 1;
            front_end_flow = front_end_flow_forced;
        } else if (strcmp(argv[index], "--title-main-select") == 0 &&
                   index + 1 < argc) {
            title_active = 1;
            title_diagnostic = 1;
            front_end_flow = front_end_flow_forced;
            title_main_select = atoi(argv[++index]);
            if (title_main_select < 0 || title_main_select > 7) {
                pc_print_usage(argv[0]);
                return 2;
            }
        } else if (strcmp(argv[index], "--title-valid-save") == 0) {
            title_active = 1;
            title_diagnostic = 1;
            front_end_flow = front_end_flow_forced;
            title_valid_save = 1;
        } else if (strcmp(argv[index], "--front-end-flow") == 0) {
            title_active = 1;
            title_diagnostic = 1;
            front_end_flow_forced = 1;
            front_end_flow = 1;
        } else if (strcmp(
                       argv[index],
                       "--title-character-select") == 0) {
            title_active = 1;
            title_diagnostic = 1;
            front_end_flow = 0;
            title_character_select = 1;
        } else if (strcmp(
                       argv[index],
                       "--title-character-select-completed") == 0) {
            title_active = 1;
            title_diagnostic = 1;
            front_end_flow = 0;
            title_character_select = 1;
            title_character_select_completed = 1;
        } else if (strcmp(
                       argv[index],
                       "--title-character-select-two-player") == 0) {
            title_active = 1;
            title_diagnostic = 1;
            front_end_flow = 0;
            title_character_select = 2;
        } else if (strcmp(
                       argv[index],
                       "--title-character-select-two-player-completed") == 0) {
            title_active = 1;
            title_diagnostic = 1;
            front_end_flow = 0;
            title_character_select = 2;
            title_character_select_completed = 1;
        } else if (strcmp(
                       argv[index],
                       "--title-level-select") == 0) {
            title_active = 1;
            title_diagnostic = 1;
            front_end_flow = 0;
            title_level_select = 1;
        } else if (strcmp(argv[index], "--mute") == 0) {
            mute = 1;
        } else if (strcmp(argv[index], "--silent-audio") == 0) {
            audio_output_enabled = 0;
        } else if (strcmp(
                       argv[index],
                       "--persistence-directory") == 0 &&
                   index + 1 < argc) {
            persistence_directory = argv[++index];
        } else if (strcmp(argv[index], "--control-scheme") == 0 &&
                   index + 2 < argc) {
            int player_index;
            int scheme;

            if (!pc_parse_control_scheme_override(
                    argv[index + 1],
                    argv[index + 2],
                    &player_index,
                    &scheme)) {
                pc_print_usage(argv[0]);
                return 2;
            }
            input.controllerConfigOverride[player_index] = scheme;
            index += 2;
        } else if (strcmp(argv[index], "--headless-move") == 0) {
            input.headlessBits |= JPB_PAD_UP;
        } else if (strcmp(argv[index], "--headless-left") == 0) {
            input.headlessBits |= JPB_PAD_RIGHT;
        } else if (strcmp(argv[index], "--headless-down") == 0) {
            input.headlessBits |= JPB_PAD_DOWN;
        } else if (strcmp(argv[index], "--headless-right") == 0) {
            input.headlessBits |= JPB_PAD_LEFT;
        } else if (strcmp(argv[index], "--headless-attack") == 0) {
            input.headlessBits |= JPB_PAD_COMBO_NORTH;
        } else if (strcmp(argv[index], "--headless-block") == 0) {
            input.headlessBits |= JPB_PAD_BLOCK;
        } else if (strcmp(argv[index], "--headless-lock") == 0) {
            input.headlessBits |= JPB_PAD_LOCK_ON;
        } else if (strcmp(argv[index], "--validate-combat") == 0) {
            input.validateCombat = 1;
        } else if (strcmp(argv[index], "--validate-jump") == 0) {
            input.validateJump = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-multi-enemy") == 0) {
            input.validateMultiEnemy = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-radar") == 0) {
            input.validateRadar = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-radar-1080") == 0) {
            input.validateRadar1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-core") == 0) {
            input.validateHudCore = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-core-1080") == 0) {
            input.validateHudCore1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-normal") == 0) {
            input.validateHudNormal = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-normal-1080") == 0) {
            input.validateHudNormal1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-normal-proof-1080") == 0) {
            input.validateHudNormalProof1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-p2-core") == 0) {
            input.validateHudP2Core = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-p2-core-1080") == 0) {
            input.validateHudP2Core1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-continue") == 0) {
            input.validateHudContinue = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-continue-1080") == 0) {
            input.validateHudContinue1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-rescue") == 0) {
            input.validateHudRescue = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-rescue-1080") == 0) {
            input.validateHudRescue1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-pilot-counter") == 0) {
            input.validateHudPilotCounter = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-pilot-counter-1080") == 0) {
            input.validateHudPilotCounter1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-countdown") == 0) {
            input.validateHudCountdown = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-countdown-1080") == 0) {
            input.validateHudCountdown1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-countdown-kill") == 0) {
            input.validateHudCountdownKill = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-countdown-kill-1080") == 0) {
            input.validateHudCountdownKill1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-countdown-success") == 0) {
            input.validateHudCountdownSuccess = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-countdown-success-1080") == 0) {
            input.validateHudCountdownSuccess1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-countdown-fail") == 0) {
            input.validateHudCountdownFail = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-countdown-fail-1080") == 0) {
            input.validateHudCountdownFail1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-hangar") == 0) {
            input.validateHudHangar = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-hangar-1080") == 0) {
            input.validateHudHangar1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-arena") == 0) {
            input.validateHudArena = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-arena-1080") == 0) {
            input.validateHudArena1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-mini4") == 0) {
            input.validateHudMini4 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-mini4-1080") == 0) {
            input.validateHudMini41080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-damage") == 0) {
            input.validateHudDamage = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-damage-1080") == 0) {
            input.validateHudDamage1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-damage-compact") == 0) {
            input.validateHudDamageCompact = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-damage-compact-1080") == 0) {
            input.validateHudDamageCompact1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-damage-p2") == 0) {
            input.validateHudDamageP2 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-damage-p2-1080") == 0) {
            input.validateHudDamageP21080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-damage-p2-compact") == 0) {
            input.validateHudDamageP2Compact = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-damage-p2-compact-1080") == 0) {
            input.validateHudDamageP2Compact1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-kadu") == 0) {
            input.validateHudKadu = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-kadu-1080") == 0) {
            input.validateHudKadu1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-offscreen") == 0) {
            input.validateHudOffscreen = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-offscreen-1080") == 0) {
            input.validateHudOffscreen1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-lifetile") == 0) {
            input.validateHudLifeTile = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-lifetile-1080") == 0) {
            input.validateHudLifeTile1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-lifetile-projected") == 0) {
            input.validateHudProjectedLifeTile = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-lifetile-projected-1080") == 0) {
            input.validateHudProjectedLifeTile1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-debug-labels") == 0) {
            input.validateHudDebugLabels = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-debug-labels-1080") == 0) {
            input.validateHudDebugLabels1080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-debug-labels3") == 0) {
            input.validateHudDebugLabels3 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-debug-labels3-1080") == 0) {
            input.validateHudDebugLabels31080 = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-hud-owner-coverage") == 0) {
            input.validateHudOwnerCoverage = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-teleport") == 0) {
            input.validateTeleport = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-camera-follow") == 0) {
            input.validateCameraFollow = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-title-audio") == 0) {
            input.validateTitleAudio = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-player-saber") == 0) {
            input.validatePlayerSaber = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-player-projectile") == 0) {
            input.validatePlayerProjectile = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-presentation-handoff") == 0) {
            input.validatePresentationHandoff = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-audio-handoff") == 0) {
            input.validateAudioHandoff = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-persistence-handoff") == 0) {
            input.validatePersistenceHandoff = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-neutral-handoff") == 0) {
            input.validateNeutralHandoff = 1;
        } else if (strcmp(
                       argv[index],
                       "--validate-player-two-sound") == 0 &&
                   index + 1 < argc) {
            validate_player_two_sound = argv[++index];
        } else if (strcmp(
                       argv[index],
                       "--headless-maximum-progression") == 0) {
            input.headlessMaximumProgression = 1;
        } else if (strcmp(
                       argv[index],
                       "--require-fbx-level") == 0) {
            input.requireFbxLevel = 1;
        } else if (strcmp(argv[index], "--camera-dolly") == 0 &&
                   index + 1 < argc) {
            camera_dolly_override = atoi(argv[++index]);
            if (camera_dolly_override < 1 ||
                camera_dolly_override > UINT8_MAX) {
                pc_print_usage(argv[0]);
                return 2;
            }
        } else if (strcmp(
                       argv[index],
                       "--camera-diagnostics") == 0) {
            camera_diagnostics = 1;
        } else if (strcmp(
                       argv[index],
                       "--headless-stick-pair") == 0 &&
                   index + 5 < argc) {
            PcHeadlessPhase *phase;

            if (input.phaseCount >= PC_HEADLESS_PHASE_CAPACITY) {
                pc_print_usage(argv[0]);
                return 2;
            }
            phase = &input.phases[input.phaseCount];
            memset(phase, 0, sizeof(*phase));
            phase->independentPlayers = 1;
            phase->xinputMask = UINT8_C(3);
            if (!pc_parse_i16(
                    argv[++index], &phase->gamepads[0].thumbLX) ||
                !pc_parse_i16(
                    argv[++index], &phase->gamepads[0].thumbLY) ||
                !pc_parse_i16(
                    argv[++index], &phase->gamepads[1].thumbLX) ||
                !pc_parse_i16(
                    argv[++index], &phase->gamepads[1].thumbLY)) {
                pc_print_usage(argv[0]);
                return 2;
            }
            phase->frames = atoi(argv[++index]);
            if (phase->frames < 1) {
                pc_print_usage(argv[0]);
                return 2;
            }
            ++input.phaseCount;
        } else if (
            strcmp(argv[index], "--headless-keyboard-phase") == 0 &&
            index + 2 < argc) {
            PcHeadlessPhase *phase;

            if (input.phaseCount >= PC_HEADLESS_PHASE_CAPACITY) {
                pc_print_usage(argv[0]);
                return 2;
            }
            phase = &input.phases[input.phaseCount];
            memset(phase, 0, sizeof(*phase));
            phase->independentPlayers = 1;
            phase->keyboardMask = UINT8_C(1);
            if (!pc_parse_headless_keyboard(
                    argv[++index], &phase->keyboard)) {
                pc_print_usage(argv[0]);
                return 2;
            }
            phase->frames = atoi(argv[++index]);
            if (phase->frames < 1) {
                pc_print_usage(argv[0]);
                return 2;
            }
            ++input.phaseCount;
        } else if (
            (strcmp(argv[index], "--headless-xinput-phase") == 0 &&
             index + 2 < argc) ||
            (strcmp(argv[index], "--headless-xinput-phase-pair") == 0 &&
             index + 3 < argc)) {
            PcHeadlessPhase *phase;
            int paired = strcmp(
                argv[index], "--headless-xinput-phase-pair") == 0;

            if (input.phaseCount >= PC_HEADLESS_PHASE_CAPACITY) {
                pc_print_usage(argv[0]);
                return 2;
            }
            phase = &input.phases[input.phaseCount];
            memset(phase, 0, sizeof(*phase));
            phase->independentPlayers = 1;
            phase->xinputMask = paired ? UINT8_C(3) : UINT8_C(1);
            if (!pc_parse_headless_xinput_buttons(
                    argv[++index], &phase->gamepads[0]) ||
                (paired && !pc_parse_headless_xinput_buttons(
                    argv[++index], &phase->gamepads[1]))) {
                pc_print_usage(argv[0]);
                return 2;
            }
            phase->frames = atoi(argv[++index]);
            if (phase->frames < 1) {
                pc_print_usage(argv[0]);
                return 2;
            }
            ++input.phaseCount;
        } else if ((strcmp(
                        argv[index],
                        "--headless-phase-pair") == 0 ||
                    strcmp(
                        argv[index],
                        "--input-phase-pair") == 0) &&
                   index + 3 < argc) {
            PcHeadlessPhase *phase;

            if (input.phaseCount >= PC_HEADLESS_PHASE_CAPACITY) {
                pc_print_usage(argv[0]);
                return 2;
            }
            phase = &input.phases[input.phaseCount];
            memset(phase, 0, sizeof(*phase));
            phase->independentPlayers = 1;
            if (!pc_parse_headless_buttons(
                    argv[++index], &phase->bits) ||
                !pc_parse_headless_buttons(
                    argv[++index], &phase->playerTwoBits)) {
                pc_print_usage(argv[0]);
                return 2;
            }
            phase->frames = atoi(argv[++index]);
            if (phase->frames < 1) {
                pc_print_usage(argv[0]);
                return 2;
            }
            ++input.phaseCount;
        } else if ((strcmp(argv[index], "--headless-phase") == 0 ||
                    strcmp(argv[index], "--headless-phase-p1") == 0 ||
                    strcmp(argv[index], "--headless-phase-p2") == 0 ||
                    strcmp(argv[index], "--input-phase") == 0 ||
                    strcmp(argv[index], "--input-phase-p1") == 0 ||
                    strcmp(argv[index], "--input-phase-p2") == 0) &&
                   index + 2 < argc) {
            PcHeadlessPhase *phase;
            int phase_player =
                (strcmp(argv[index], "--headless-phase-p1") == 0 ||
                 strcmp(argv[index], "--input-phase-p1") == 0)
                    ? 0
                    : ((strcmp(argv[index], "--headless-phase-p2") == 0 ||
                        strcmp(argv[index], "--input-phase-p2") == 0)
                           ? 1 : -1);
            uint32_t phase_bits;

            if (input.phaseCount >=
                PC_HEADLESS_PHASE_CAPACITY) {
                pc_print_usage(argv[0]);
                return 2;
            }
            phase = &input.phases[input.phaseCount];
            memset(phase, 0, sizeof(*phase));
            phase->independentPlayers = phase_player >= 0;
            if (!pc_parse_headless_buttons(
                    argv[++index], &phase_bits)) {
                pc_print_usage(argv[0]);
                return 2;
            }
            if (phase_player < 0) {
                phase->bits = phase_bits;
            } else {
                if (phase_player == 0) {
                    phase->bits = phase_bits;
                } else {
                    phase->playerTwoBits = phase_bits;
                }
            }
            phase->frames = atoi(argv[++index]);
            if (phase->frames < 1) {
                pc_print_usage(argv[0]);
                return 2;
            }
            ++input.phaseCount;
        } else if (strcmp(argv[index], "--cad") == 0 &&
                   index + 1 < argc) {
            cad_path = argv[++index];
        } else if (strcmp(argv[index], "--bmd") == 0 &&
                   index + 1 < argc) {
            bmd_path = argv[++index];
        } else if (strcmp(argv[index], "--cmb") == 0 &&
                   index + 1 < argc) {
            cmb_path = argv[++index];
        } else if (strcmp(argv[index], "--player-model") == 0 &&
                   index + 1 < argc) {
            player_model = atoi(argv[++index]);
            player_model_override = 1;
            if (player_model < 0 ||
                player_model >= JPB_MODEL_NAME_COUNT) {
                pc_print_usage(argv[0]);
                return 2;
            }
        } else if (strcmp(argv[index], "--player-two-cad") == 0 &&
                   index + 1 < argc) {
            player_two_cad_path = argv[++index];
        } else if (strcmp(argv[index], "--player-two-bmd") == 0 &&
                   index + 1 < argc) {
            player_two_bmd_path = argv[++index];
        } else if (strcmp(argv[index], "--player-two-cmb") == 0 &&
                   index + 1 < argc) {
            player_two_cmb_path = argv[++index];
        } else if (strcmp(argv[index], "--player-two-model") == 0 &&
                   index + 1 < argc) {
            player_two_model = atoi(argv[++index]);
            if (player_two_model < 0 ||
                player_two_model >= JPB_MODEL_NAME_COUNT) {
                pc_print_usage(argv[0]);
                return 2;
            }
        } else if (strcmp(argv[index], "--enemy-cad") == 0 &&
                   index + 1 < argc) {
            enemy_cad_path = argv[++index];
        } else if (strcmp(argv[index], "--enemy-bmd") == 0 &&
                   index + 1 < argc) {
            enemy_bmd_path = argv[++index];
        } else if (strcmp(argv[index], "--frames") == 0 &&
                   index + 1 < argc) {
            frame_limit = atoi(argv[++index]);
            if (frame_limit < 1) {
                pc_print_usage(argv[0]);
                return 2;
            }
        } else if (strcmp(argv[index], "--output") == 0 &&
                   index + 1 < argc) {
            output_path = argv[++index];
        } else if (strcmp(argv[index], "--framebuffer-size") == 0 &&
                   index + 2 < argc) {
            framebuffer_size_explicit = 1;
            framebuffer_width = atoi(argv[++index]);
            framebuffer_height = atoi(argv[++index]);
            if (framebuffer_width < 320 ||
                framebuffer_width > 3840 ||
                framebuffer_height < 240 ||
                framebuffer_height > 2160) {
                pc_print_usage(argv[0]);
                return 2;
            }
        } else {
            pc_print_usage(argv[0]);
            return 2;
        }
    }
    if (quickload_level != NULL) {
        if (title_diagnostic) {
            fputs(
                "--quickload cannot be combined with a title "
                "diagnostic switch\n",
                stderr);
            return 2;
        }
        if (!using_installed_assets ||
            !pc_configure_level_asset(
                default_assets.mesh,
                sizeof(default_assets.mesh),
                quickload_level)) {
            fprintf(
                stderr,
                "unknown or unavailable installed level for --quickload: "
                "%s\n",
                quickload_level);
            return 2;
        }
        mesh_path = default_assets.mesh;
        title_active = 0;
        front_end_flow = 0;
    }
    if (player_model_override && using_installed_assets) {
        if (!pc_configure_player_assets(
                &selected_player_assets, player_model)) {
            fprintf(
                stderr,
                "installed player assets are unavailable for model %d\n",
                player_model);
            return 2;
        }
        cad_path = selected_player_assets.cad;
        bmd_path = selected_player_assets.bmd;
        cmb_path = selected_player_assets.cmb;
    }
    if (input.validateHudP2Core ||
        input.validateHudP2Core1080 ||
        input.validateHudDamageP2 ||
        input.validateHudDamageP21080 ||
        input.validateHudDamageP2Compact ||
        input.validateHudDamageP2Compact1080) {
        if (player_two_cad_path == NULL &&
            !pc_configure_player_assets(
                &validation_player_two_assets,
                1)) {
            fputs(
                "--validate-hud-p2-core could not resolve installed "
                "P2 assets\n",
                stderr);
            return 4;
        }
        if (player_two_cad_path == NULL) {
            player_two_cad_path = validation_player_two_assets.cad;
            player_two_bmd_path = validation_player_two_assets.bmd;
            player_two_cmb_path = validation_player_two_assets.cmb;
            player_two_model = 1;
        }
    }
    if (input.headless && frame_limit == 0) {
        frame_limit = 1;
    }
    if (overlay_mode_override >= 0) {
        OptionStruct.overlayMode = (uint8_t)overlay_mode_override;
        defaultOptionStruct.overlayMode = (uint8_t)overlay_mode_override;
    }
    if (input.headless && input.hiddenWindow) {
        fputs(
            "--hidden-window cannot be combined with --headless\n",
            stderr);
        return 2;
    }
    if (input.validatePresentationHandoff &&
        (input.headless || !input.scriptedInput || frame_limit == 0)) {
        fputs(
            "--validate-presentation-handoff requires a finite "
            "interactive --scripted-input run\n",
            stderr);
        return 2;
    }
    if (input.validateAudioHandoff &&
        (!input.validatePresentationHandoff || mute)) {
        fputs(
            "--validate-audio-handoff requires an unmuted "
            "--validate-presentation-handoff run\n",
            stderr);
        return 2;
    }
    if (input.validatePersistenceHandoff &&
        (!input.validatePresentationHandoff ||
         persistence_directory == NULL)) {
        fputs(
            "--validate-persistence-handoff requires "
            "--validate-presentation-handoff and an explicit "
            "--persistence-directory\n",
            stderr);
        return 2;
    }
    if (validate_player_two_sound != NULL &&
        !input.validateAudioHandoff) {
        fputs(
            "--validate-player-two-sound requires "
            "--validate-audio-handoff\n",
            stderr);
        return 2;
    }
    if (input.validateNeutralHandoff &&
        !input.validatePresentationHandoff) {
        fputs(
            "--validate-neutral-handoff requires "
            "--validate-presentation-handoff\n",
            stderr);
        return 2;
    }
    if (!input.headless && !input.hiddenWindow &&
        !framebuffer_size_explicit) {
        framebuffer_width = PC_VISIBLE_FRAMEBUFFER_WIDTH;
        framebuffer_height = PC_VISIBLE_FRAMEBUFFER_HEIGHT;
    }
    pixels = (uint32_t *)malloc(
        (size_t)framebuffer_width *
        framebuffer_height *
        sizeof(*pixels));
    if (pixels == NULL) {
        fputs("could not allocate PC framebuffer\n", stderr);
        return 3;
    }
    framebuffer.pixels = pixels;
    framebuffer.width = framebuffer_width;
    framebuffer.height = framebuffer_height;
    framebuffer.stridePixels = framebuffer_width;
    /* Hidden presentation tests omit persistence unless an explicit isolated
     * directory is supplied. They never fall back to the executable's save
     * directory. */
    if (persistence_directory != NULL &&
        !pc_configure_persistence_directory(
            &input, persistence_directory)) {
        fprintf(
            stderr,
            "could not initialize explicit persistence directory: %s\n",
            persistence_directory);
        free(pixels);
        return 3;
    }
    if (!input.headless && persistence_directory == NULL &&
        !input.hiddenWindow && !pc_configure_persistence(&input)) {
        input.noValidGameSave = 1;
        fputs(
            "warning: PC save directory could not be initialized; "
            "continuing without persistence\n",
            stderr);
    }
    if (!input.headless) {
        unsigned controller_count;

        (void)jpb_PCXInputInit(&input.xinput);
        controller_count = pc_controller_count(&input);
        input.previousControllerMask = input.xinput.connectedMask;
        if (controller_count != 0) {
            /* Prefer the connected pad and its authored glyph bank initially;
             * keyboard activity switches the prompt bank immediately. */
            lastUsedInputType = 1;
        }
        jpb_PCLog(
            "XInput initialized controllers=%u dynamic_runtime=%d",
            controller_count,
            input.xinput.module != NULL);
    }
    jpb_InputSetProvider(pc_read_pad, &input);
    jpb_InputSetRumbleProvider(pc_set_rumble, &input);
    jpb_BrainutlSetCheatChordProvider(
        pc_read_brainutl_cheat_chords, NULL);
    menu_hooks.activateItem = pc_activate_menu_item;
    menu_hooks.keyboardState = pc_read_keyboard_state;
    menu_hooks.saveGameData = pc_save_game_data;
    menu_hooks.controllerCount = pc_controller_count;
    menu_hooks.controllerName = pc_controller_name;
    menu_hooks.requestExit = pc_request_exit;
    jpb_MenuSetPlatformHooks(&menu_hooks, &input);
    result = jpb_GameRuntimeInitWithPlayerAssets(
        &runtime, mesh_path, cad_path, bmd_path, player_model);
    if (result != JPB_GAME_RUNTIME_OK) {
        fprintf(stderr, "game runtime init failed: status=%d\n", result);
        jpb_InputSetProvider(NULL, NULL);
        jpb_InputSetRumbleProvider(NULL, NULL);
        jpb_BrainutlSetCheatChordProvider(NULL, NULL);
        jpb_MenuSetPlatformHooks(NULL, NULL);
        jpb_PCXInputShutdown(&input.xinput);
        free(pixels);
        return 4;
    }
    jpb_PCLog("runtime initialized mesh=%s player_cad=%s player_bmd=%s",
        mesh_path,
        cad_path != NULL ? cad_path : "(none)",
        bmd_path != NULL ? bmd_path : "(none)");
    if (camera_dolly_override > 0) {
        runtime.world->overRideDolly =
            (int16_t)camera_dolly_override;
        newcameraflag = 1;
    }
    if (input.headlessMaximumProgression) {
        /*
         * Build the same fully unlocked control surface that a completed
         * save exposes.  Set combo availability before CMB initialization
         * so combo_InitComboData also derives its award thresholds from the
         * complete authored set rather than only the new-game subset.
         */
        memset(
            GameStruct.jediComboMask,
            0xff,
            sizeof(GameStruct.jediComboMask));
        GameStruct.ForceLevel = 9;
        GameStruct.aCharacterData[0].Items = 1;
        GameStruct.aCharacterData[1].Items = 1;
    }
    if (cmb_path != NULL) {
        result = jpb_GameRuntimeAddPlayerComboData(
            &runtime, cmb_path);
        if (result != JPB_GAME_RUNTIME_OK) {
            fprintf(
                stderr,
                "player combo init failed: status=%d\n",
                result);
            jpb_GameRuntimeShutdown(&runtime);
            free(pixels);
            return 4;
        }
    }
    if ((player_two_cad_path == NULL) !=
            (player_two_bmd_path == NULL) ||
        (player_two_cad_path == NULL) !=
            (player_two_cmb_path == NULL)) {
        fputs(
            "player-two CAD, BMD, and CMB must be provided together\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 2;
    }
    if (player_two_cad_path != NULL) {
        menu_setNumPlayers(2);
        menu_setPlayer(1, (unsigned)player_two_model);
        GameStruct.versusModeFlag =
            (int16_t)(jpb_LevelIndexFromPath(mesh_path) == 25);
        result = jpb_GameRuntimeActivateSecondPlayer(
            &runtime,
            player_two_cad_path,
            player_two_bmd_path,
            player_two_model);
        if (result == JPB_GAME_RUNTIME_OK) {
            result = jpb_GameRuntimeAddSecondPlayerComboData(
                &runtime, player_two_cmb_path);
        }
        if (result != JPB_GAME_RUNTIME_OK) {
            fprintf(
                stderr,
                "player-two runtime init failed: status=%d\n",
                result);
            jpb_GameRuntimeShutdown(&runtime);
            free(pixels);
            return 4;
        }
        /* jedi_InitPlayer/player_RefreshPlayer initialize P2 after the
         * completed-save fixture above, including its per-player item count.
         * Reapply that fixture at the same post-spawn boundary used by the
         * live character data so both physical controllers expose the full
         * unlocked action surface. */
        if (input.headlessMaximumProgression) {
            GameStruct.aCharacterData[1].Items = 1;
        }
    } else if ((int)(uint8_t)GameStruct.CurrentLevel == 12) {
        result = pc_activate_solo_kadu_rider(
            &runtime,
            cad_path,
            bmd_path,
            cmb_path,
            GameStruct.ModelSelect[0]);
        if (result != JPB_GAME_RUNTIME_OK) {
            fprintf(
                stderr,
                "Mini2 CPU rider init failed: status=%d\n",
                result);
            jpb_GameRuntimeShutdown(&runtime);
            free(pixels);
            return 4;
        }
    }
    if ((enemy_cad_path == NULL) != (enemy_bmd_path == NULL)) {
        fputs(
            "enemy CAD and BMD must be provided together\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 2;
    }
    /* --quickload skips only the front end.  The recovered 115-entry actor,
     * model, and animation ownership tables now supply the same authored
     * enemy and level-machinery classes as the presentation-owned gameplay
     * path, so suppressing them here would produce an incomplete level. */
    if (enemy_cad_path != NULL) {
        result = jpb_GameRuntimeAddEnemyAssets(
            &runtime, enemy_cad_path, enemy_bmd_path);
        if (result != JPB_GAME_RUNTIME_OK) {
            fprintf(
                stderr,
                "enemy actor init failed: status=%d\n",
                result);
            jpb_GameRuntimeShutdown(&runtime);
            free(pixels);
            return 4;
        }
        if (runtime.enemyState != NULL &&
            (runtime.enemyActorCount != 0 ||
            runtime.enemySpawnCount != 0 ||
            runtime.player == NULL ||
            runtime.inactivePlayer == NULL ||
            runtime.player->target != runtime.inactivePlayer ||
            runtime.inactivePlayer->target != runtime.player)) {
            fputs(
                "enemy asset loading activated a placement or changed "
                "the player-pair target before the authored frame owner\n",
                stderr);
            jpb_GameRuntimeShutdown(&runtime);
            free(pixels);
            return 4;
        }
    }
    if (input.validateMultiEnemy &&
        (!input.headless ||
         enemy_cad_path == NULL ||
         !pc_position_for_multi_enemy_validation(
             &runtime))) {
        fputs(
            "multi-enemy validation could not select an authored "
            "co-resident placement\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if ((input.validateHudKadu ||
         input.validateHudKadu1080) &&
        (!input.headless ||
         (int)(uint8_t)GameStruct.CurrentLevel != 12)) {
        fputs(
            "--validate-hud-kadu requires headless Mini2/Mini2 quickload\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if ((input.validateHudCore ||
         input.validateHudP2Core ||
         input.validateHudContinue ||
         input.validateHudRescue ||
         input.validateHudPilotCounter ||
         input.validateHudDamageP2 ||
         input.validateHudCountdown ||
         input.validateHudCountdownKill ||
         input.validateHudCountdownSuccess ||
         input.validateHudCountdownFail ||
         input.validateHudHangar ||
         input.validateHudArena ||
         input.validateHudMini4) &&
        (!input.headless ||
         OptionStruct.overlayMode != 2 ||
         framebuffer.width != 960 ||
         framebuffer.height != 540)) {
        fputs(
            "HUD overlay validators require headless overlay-mode 2 "
            "at 960x540\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if ((input.validateHudCore1080 ||
         input.validateHudP2Core1080 ||
         input.validateHudContinue1080 ||
         input.validateHudRescue1080 ||
         input.validateHudPilotCounter1080 ||
         input.validateHudCountdown1080 ||
         input.validateHudCountdownKill1080 ||
         input.validateHudCountdownSuccess1080 ||
         input.validateHudCountdownFail1080 ||
         input.validateHudHangar1080 ||
         input.validateHudArena1080 ||
         input.validateHudMini41080 ||
         input.validateHudDamage1080 ||
         input.validateHudDamageP21080 ||
         input.validateHudKadu1080 ||
         input.validateHudOffscreen1080 ||
         input.validateRadar1080) &&
        (!input.headless ||
         OptionStruct.overlayMode != 2 ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080)) {
        fputs(
            "HUD 1080 overlay validators require headless overlay-mode 2 "
            "at 1920x1080\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudLifeTile &&
        (!input.headless ||
         OptionStruct.overlayMode != 1 ||
         framebuffer.width != 960 ||
         framebuffer.height != 540)) {
        fputs(
            "HUD life-tile validator requires headless overlay-mode 1 "
            "at 960x540\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudLifeTile1080 &&
        (!input.headless ||
         OptionStruct.overlayMode != 1 ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080)) {
        fputs(
            "HUD life-tile 1080 validator requires headless overlay-mode 1 "
            "at 1920x1080\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudNormal &&
        (!input.headless ||
         OptionStruct.overlayMode != 1 ||
         framebuffer.width != 960 ||
         framebuffer.height != 540)) {
        fputs(
            "HUD normal gameplay validator requires headless "
            "overlay-mode 1 at 960x540\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudNormal1080 &&
        (!input.headless ||
         OptionStruct.overlayMode != 1 ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080)) {
        fputs(
            "HUD normal gameplay 1080 validator requires headless "
            "overlay-mode 1 at 1920x1080\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudProjectedLifeTile &&
        (!input.headless ||
         OptionStruct.overlayMode != 2 ||
         framebuffer.width != 960 ||
         framebuffer.height != 540)) {
        fputs(
            "HUD projected life-tile validator requires headless "
            "overlay-mode 2 at 960x540\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudProjectedLifeTile1080 &&
        (!input.headless ||
         OptionStruct.overlayMode != 2 ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080)) {
        fputs(
            "HUD projected life-tile 1080 validator requires headless "
            "overlay-mode 2 at 1920x1080\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudDamageCompact &&
        (!input.headless ||
         OptionStruct.overlayMode != 1 ||
         framebuffer.width != 960 ||
         framebuffer.height != 540)) {
        fputs(
            "HUD compact damage validator requires headless overlay-mode 1 "
            "at 960x540\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
            return 4;
    }
    if (input.validateHudDamageP2Compact &&
        (!input.headless ||
         OptionStruct.overlayMode != 1 ||
         framebuffer.width != 960 ||
         framebuffer.height != 540)) {
        fputs(
            "HUD P2 compact damage validator requires headless "
            "overlay-mode 1 at 960x540\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudDamageP2Compact1080 &&
        (!input.headless ||
         OptionStruct.overlayMode != 1 ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080)) {
        fputs(
            "HUD P2 compact damage 1080 validator requires headless "
            "overlay-mode 1 at 1920x1080\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudDamageCompact1080 &&
        (!input.headless ||
         OptionStruct.overlayMode != 1 ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080)) {
        fputs(
            "HUD compact damage 1080 validator requires headless "
            "overlay-mode 1 at 1920x1080\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudDebugLabels &&
        (!input.headless ||
         OptionStruct.overlayMode != 1 ||
         framebuffer.width != 960 ||
         framebuffer.height != 540)) {
        fputs(
            "HUD debug-label validator requires headless overlay-mode 1 "
            "at 960x540\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudDebugLabels1080 &&
        (!input.headless ||
         OptionStruct.overlayMode != 1 ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080)) {
        fputs(
            "HUD debug-label 1080 validator requires headless "
            "overlay-mode 1 at 1920x1080\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudDebugLabels3 &&
        (!input.headless ||
         OptionStruct.overlayMode != 2 ||
         framebuffer.width != 960 ||
         framebuffer.height != 540)) {
        fputs(
            "HUD DebugLevel 3 label validator requires headless "
            "overlay-mode 2 at 960x540\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateHudDebugLabels31080 &&
        (!input.headless ||
         OptionStruct.overlayMode != 2 ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080)) {
        fputs(
            "HUD DebugLevel 3 label 1080 validator requires headless "
            "overlay-mode 2 at 1920x1080\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
    if (input.validateRadar || input.validateRadar1080 ||
        input.validateHudProjectedLifeTile ||
        input.validateHudProjectedLifeTile1080 ||
        input.validateHudDebugLabels3 ||
        input.validateHudDebugLabels31080) {
        OptionStruct.DebugLevel = 3;
    }
    if (input.validateHudDebugLabels ||
        input.validateHudDebugLabels1080) {
        OptionStruct.DebugLevel = 2;
    }
    if (input.validateHudDamage ||
        input.validateHudDamage1080 ||
        input.validateHudDamageCompact ||
        input.validateHudDamageCompact1080) {
        pc_seed_hud_damage_validation();
    }
    if (input.validateHudP2Core || input.validateHudP2Core1080) {
        pc_seed_hud_p2_core_validation();
    }
    if (input.validateHudDamageP2 ||
        input.validateHudDamageP21080 ||
        input.validateHudDamageP2Compact ||
        input.validateHudDamageP2Compact1080) {
        pc_seed_hud_p2_damage_validation();
    }
    if (input.validateHudContinue ||
        input.validateHudContinue1080) {
        pc_seed_hud_continue_validation();
    }
    if (input.validateHudRescue ||
        input.validateHudRescue1080) {
        pc_seed_hud_rescue_validation();
    }
    if (input.validateHudPilotCounter ||
        input.validateHudPilotCounter1080) {
        pc_seed_hud_pilot_counter_validation();
    }
    if (input.validateHudCountdown ||
        input.validateHudCountdown1080) {
        pc_seed_hud_countdown_validation();
    }
    if (input.validateHudCountdownKill ||
        input.validateHudCountdownKill1080) {
        pc_seed_hud_countdown_kill_validation();
    }
    if (input.validateHudCountdownSuccess ||
        input.validateHudCountdownSuccess1080) {
        pc_seed_hud_countdown_success_validation();
    }
    if (input.validateHudCountdownFail ||
        input.validateHudCountdownFail1080) {
        pc_seed_hud_countdown_fail_validation();
    }
    if (input.validateHudHangar ||
        input.validateHudHangar1080) {
        pc_seed_hud_hangar_validation();
    }
    if (input.validateHudArena ||
        input.validateHudArena1080) {
        pc_seed_hud_arena_validation();
    }
    if (input.validateHudMini4 ||
        input.validateHudMini41080) {
        pc_seed_hud_mini4_validation();
    }
    if (input.validateHudOffscreen ||
        input.validateHudOffscreen1080) {
        pc_seed_hud_offscreen_validation(&runtime);
    }
    initial_position = runtime.physics->pos;
    if (input.validateTeleport) {
        VECTOR position = runtime.physics->vpos;
        VECTOR offset = {64, 0, 0, 0};

        enemy_SetTeleport(
            &position,
            &offset,
            0,
            INT_MAX);
    }

#if defined(JPB_PC_HAS_UFBX)
    if (!pc_attach_fbx_level(
            mesh_path,
            &runtime,
            &fbx_level,
            &fbx_level_loaded)) {
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
    }
#endif

    if (input.requireFbxLevel) {
#if defined(JPB_PC_HAS_UFBX)
        if (!fbx_level_loaded) {
            fputs(
                "required FBX level sidecar was not loaded\n",
                stderr);
            jpb_GameRuntimeShutdown(&runtime);
            free(pixels);
            return 4;
        }
#else
        fputs(
            "required FBX level rendering is unavailable; configure "
            "JPB_UFBX_SOURCE_DIR with exact ufbx 0.6.1\n",
            stderr);
        jpb_GameRuntimeShutdown(&runtime);
        free(pixels);
        return 4;
#endif
    }

    if (title_active) {
        const char *title_path = resource_getPath(
            "JPB_SplashV3_Sharpened.png",
            JPB_RESOURCE_FRONT);

        menu_texture_cache = (PcMenuTextureCache *)calloc(
            1, sizeof(*menu_texture_cache));
        title_pixels = (uint32_t *)malloc(
            (size_t)framebuffer.width *
            framebuffer.height *
            sizeof(*title_pixels));
        if (menu_texture_cache == NULL || title_pixels == NULL ||
            !jpb_PCLoadImageWIC(
                title_path,
                framebuffer.width,
                framebuffer.height,
                title_pixels,
                framebuffer.stridePixels)) {
            fputs(
                "title presentation could not decode the original splash\n",
                stderr);
            free(title_pixels);
            free(menu_texture_cache);
            jpb_GameRuntimeShutdown(&runtime);
            free(pixels);
            return 4;
        }
        /*
         * The matched menu owner supplies full front-resource paths. WIC is
         * deliberately confined to this Win32 adapter; the recovered menu
         * and the portable renderer see only opaque JPBSoftwareTexture
         * resources through the existing platform seam.
         */
        jpb_TextureSetPlatformHooks(
            pc_load_menu_texture, NULL, menu_texture_cache);
        menu_mainInitMenu(0);
        if (menuTexLoaded == 0) {
            fputs(
                "title presentation could not load the original menu bank\n",
                stderr);
            jpb_GameRuntimeShutdown(&runtime);
            pc_release_menu_textures(menu_texture_cache);
            free(menu_texture_cache);
            free(title_pixels);
            free(pixels);
            return 4;
        }
        jpb_PCLog(
            "front end initialized mode=%u stack=%u textures=%zu",
            (unsigned)menuVars.menuMode[menuVars.menuModeSP & 7u],
            (unsigned)menuVars.menuModeSP,
            menu_texture_cache->count);
        /*
         * The retail state-1 path owns EULA and attract-movie resources
         * that are not part of this installed PC data set. Enter its
         * ordinary title state explicitly; subsequent frames stay in the
         * PDB-named menu_mainLoop owner.
         */
        menuVars.menuMode[0] = 0;
        menuVars.mmSelect1[0] = 0;
        if (title_main_select >= 0) {
            menuVars.mmSelect1[0] = (uint8_t)title_main_select;
        }
        /* Registration targeted a retired external service. Keep the exact
         * no-registration PC title stream instead of exposing a dead route. */
        (void)menu_setCanShowRegisterGame(
            title_diagnostic ? 1 : 0);
        if (title_character_select) {
            menu_setNumPlayers(
                title_character_select == 2 ? 2u : 1u);
            GameStruct.ModelSelect[0] = 0;
            GameStruct.ModelSelect[1] = 1;
            if (title_character_select_completed) {
                GameStruct.gameCompleted = 1;
            }
            menuVars.menuMode[0] = 0x0e;
        } else if (title_level_select) {
            menu_setNumPlayers(1);
            menuVars.menuMode[0] = 0x1a;
            menu_initLevelSelectScreen();
        }
        if (player_model_override) {
            GameStruct.ModelSelect[0] = (int16_t)player_model;
            newMenu_currentModelSelectBaseP1 = player_model;
        }
    }

    /* menu_mainInitMenu clears GameStruct as its retail owner does. Load
     * persisted state after that boundary so Continue and saved selections
     * are visible to the ordinary front-end definitions. */
    if (input.persistenceEnabled) {
        JPBSaveResult options_result =
            jpb_SaveOptionsReadFile(input.optionsPath);
        JPBSaveResult game_result =
            jpb_SaveGameReadFile(input.saveGamePath);

        input.noValidGameSave = game_result != JPB_SAVE_OK;

        if (options_result == JPB_SAVE_OK) {
            generateAllText(OptionStruct.Language);
        } else if (options_result != JPB_SAVE_NOT_FOUND) {
            jpb_PCLog(
                "options load failed result=%s path=%s",
                pc_save_result_name(options_result),
                input.optionsPath);
            fprintf(
                stderr,
                "options load failed (%s): %s\n",
                pc_save_result_name(options_result),
                input.optionsPath);
        }
        if (game_result != JPB_SAVE_OK &&
            game_result != JPB_SAVE_NOT_FOUND) {
            jpb_PCLog(
                "game load failed result=%s path=%s",
                pc_save_result_name(game_result),
                input.saveGamePath);
            fprintf(
                stderr,
                "game load failed (%s): %s\n",
                pc_save_result_name(game_result),
                input.saveGamePath);
        }
    }
    if (title_valid_save) {
        GameStruct.continueAble = 1;
        GameStruct.difficulty = 0;
        input.noValidGameSave = 0;
        jpb_PCLog("title diagnostic forcing valid-save menu branch");
    }
    /* A command-line scheme is a deterministic diagnostic override. Apply it
     * after persisted options so it is authoritative for this process only;
     * ordinary runs continue to use the saved/menu-selected configuration. */
    pc_apply_control_scheme_overrides(&input);

    if (!input.headless && !mute) {
        audio = pc_create_current_game_audio(
            mesh_path,
            cad_path,
            jpb_LevelIndexFromPath(mesh_path),
            audio_output_enabled,
            &audio_generation_count);
        if (audio == NULL) {
            fputs(
                "warning: PC audio bank paths could not be initialized; "
                "continuing without sound\n",
                stderr);
        }
    }

    if (input.headless) {
        while (frame_count < frame_limit) {
            int selected_front_end_level = 0;
            int selected_front_end_game_mode = 4;

            input.headlessActive = frame_count > 0;
            pc_select_headless_phase(
                &input, frame_count - 1);
            if (title_active) {
                pc_prepare_title_framebuffer(
                    &framebuffer, title_pixels);
                result = jpb_GameRuntimeTitleFrame(
                    &runtime, &framebuffer);
            } else {
                if (front_end_playable_handoff) {
                    pc_release_front_end_gameplay_control(
                        &runtime, GameStruct.NumPlayers);
                }
                result = jpb_GameRuntimeFrame(
                    &runtime, 1.0f / 60.0f, &framebuffer, &stats);
                if (result == JPB_GAME_RUNTIME_OK &&
                    front_end_playable_handoff) {
                    pc_release_front_end_gameplay_control(
                        &runtime, GameStruct.NumPlayers);
                }
            }
            if (result != JPB_GAME_RUNTIME_OK) {
                break;
            }
            if (title_active && front_end_flow &&
                pc_front_end_requests_gameplay(
                    &selected_front_end_level,
                    &selected_front_end_game_mode)) {
                if (!pc_configure_level_asset(
                        default_assets.mesh,
                        sizeof(default_assets.mesh),
                        sLevelNames[selected_front_end_level])) {
                    result = JPB_GAME_RUNTIME_LOAD_FAILED;
                    break;
                }
                mesh_path = default_assets.mesh;
                result = pc_start_selected_gameplay(
                    &runtime,
                    mesh_path,
                    &selected_player_assets,
                    enemy_cad_path,
                    enemy_bmd_path,
                    selected_front_end_level,
                    selected_front_end_game_mode,
                    &input);
                if (result != JPB_GAME_RUNTIME_OK) {
                    break;
                }
                cad_path = selected_player_assets.cad;
                bmd_path = selected_player_assets.bmd;
                cmb_path = selected_player_assets.cmb;
#if defined(JPB_PC_HAS_UFBX)
                if (!pc_attach_fbx_level(
                        mesh_path,
                        &runtime,
                        &fbx_level,
                        &fbx_level_loaded)) {
                    result = JPB_GAME_RUNTIME_LOAD_FAILED;
                    break;
                }
#endif
                initial_position = runtime.physics->pos;
                memset(
                    input.observedPlayerBits,
                    0,
                    sizeof(input.observedPlayerBits));
                title_active = 0;
                front_end_flow = 0;
                front_end_playable_handoff = 1;
            } else if (title_active && front_end_flow) {
                int selected_level;

                if (pc_front_end_requests_training(&selected_level)) {
                    GameStruct.NumPlayers = 1;
                    if (!pc_configure_level_asset(
                            default_assets.mesh,
                            sizeof(default_assets.mesh),
                            sLevelNames[selected_level])) {
                        result = JPB_GAME_RUNTIME_LOAD_FAILED;
                        break;
                    }
                    mesh_path = default_assets.mesh;
                    result = pc_start_selected_gameplay(
                        &runtime,
                        mesh_path,
                        &selected_player_assets,
                        NULL,
                        NULL,
                        selected_level,
                        2,
                        &input);
                    if (result != JPB_GAME_RUNTIME_OK) {
                        break;
                    }
#if defined(JPB_PC_HAS_UFBX)
                    if (!pc_attach_fbx_level(
                            mesh_path,
                            &runtime,
                            &fbx_level,
                            &fbx_level_loaded)) {
                        result = JPB_GAME_RUNTIME_LOAD_FAILED;
                        break;
                    }
#endif
                    cad_path = selected_player_assets.cad;
                    bmd_path = selected_player_assets.bmd;
                    cmb_path = selected_player_assets.cmb;
                    enemy_cad_path = NULL;
                    enemy_bmd_path = NULL;
                    initial_position = runtime.physics->pos;
                    memset(
                        input.observedPlayerBits,
                        0,
                        sizeof(input.observedPlayerBits));
                    title_active = 0;
                    front_end_flow = 0;
                    front_end_playable_handoff = 1;
                } else if (pc_front_end_requests_versus(
                               &selected_level)) {
                    if (!pc_configure_level_asset(
                            default_assets.mesh,
                            sizeof(default_assets.mesh),
                            sLevelNames[selected_level])) {
                        result = JPB_GAME_RUNTIME_LOAD_FAILED;
                        break;
                    }
                    mesh_path = default_assets.mesh;
                    result = pc_start_selected_gameplay(
                        &runtime,
                        mesh_path,
                        &selected_player_assets,
                        NULL,
                        NULL,
                        selected_level,
                        2,
                        &input);
                    if (result != JPB_GAME_RUNTIME_OK) {
                        break;
                    }
#if defined(JPB_PC_HAS_UFBX)
                    if (!pc_attach_fbx_level(
                            mesh_path,
                            &runtime,
                            &fbx_level,
                            &fbx_level_loaded)) {
                        result = JPB_GAME_RUNTIME_LOAD_FAILED;
                        break;
                    }
#endif
                    cad_path = selected_player_assets.cad;
                    bmd_path = selected_player_assets.bmd;
                    cmb_path = selected_player_assets.cmb;
                    enemy_cad_path = NULL;
                    enemy_bmd_path = NULL;
                    initial_position = runtime.physics->pos;
                    memset(
                        input.observedPlayerBits,
                        0,
                        sizeof(input.observedPlayerBits));
                    title_active = 0;
                    front_end_flow = 0;
                    front_end_playable_handoff = 1;
                }
            }
            if (title_active) {
                ++frame_count;
                continue;
            }
            if (input.validatePlayerSaber) {
                pc_trace_player_saber_action_frame(
                    &runtime,
                    runtime.player,
                    &saber_action_diagnostics);
                if (runtime.secondPlayerState != NULL) {
                    pc_trace_player_saber_action_frame(
                        &runtime,
                        runtime.inactivePlayer,
                        &second_saber_action_diagnostics);
                }
            }
            if ((input.headlessPhaseBits & JPB_PAD_JUMP) != 0 &&
                runtime.physics->pos.vy > initial_position.vy &&
                runtime.physics->mov.vy > 0.0f) {
                ++jump_airborne_frames;
            }
            if (input.validateCombat && frame_count == 2 &&
                !pc_position_for_combat_validation(&runtime)) {
                result = JPB_GAME_RUNTIME_LOAD_FAILED;
                break;
            }
            ++frame_count;
            if (input.validateMultiEnemy &&
                frame_count < frame_limit &&
                runtime.enemyActivatedClassCount <
                    runtime.enemyPlacedClassCount &&
                !pc_position_for_uncovered_enemy_validation(
                    &runtime)) {
                result = JPB_GAME_RUNTIME_LOAD_FAILED;
                break;
            }
            if (input.phaseCount != 0) {
                pc_print_headless_phase_boundary(
                    &input, &runtime, frame_count - 1);
            }
        }
    } else {
        HINSTANCE instance = GetModuleHandleA(NULL);
        HWND window = pc_create_window(
            instance,
            !input.hiddenWindow,
            !input.hiddenWindow && !input.scriptedInput,
            framebuffer.width,
            framebuffer.height);
        MSG message;
        PcGameplayLogState gameplay_log_states[2];
        uint32_t previous_menu_key_bits = 0;
        uint16_t logged_menu_mode = title_active
            ? menuVars.menuMode[menuVars.menuModeSP & 7u]
            : UINT16_MAX;
        int logged_gameplay_view = 0;

        if (window == NULL) {
            result = JPB_GAME_RUNTIME_RENDER_FAILED;
        } else {
            memset(
                gameplay_log_states,
                0,
                sizeof(gameplay_log_states));
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&previous);
            jpb_PCLog(
                "interactive window created visible=%d scripted=%d "
                "maximize=%d fxaa=present framebuffer=%dx%d",
                !input.hiddenWindow,
                input.scriptedInput,
                !input.hiddenWindow && !input.scriptedInput,
                framebuffer.width,
                framebuffer.height);
            while (pc_running &&
                   (frame_limit == 0 || frame_count < frame_limit)) {
                LARGE_INTEGER current;
                float elapsed;
                int selected_front_end_level = 0;
                int selected_front_end_game_mode = 4;
                uint32_t live_menu_keys =
                    pc_live_menu_key_bits();

                if (input.scriptedInput) {
                    input.headlessActive = frame_count > 0;
                    pc_select_headless_phase(
                        &input, frame_count - 1);
                }

                while (PeekMessageA(
                           &message, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
                if (!pc_running) {
                    break;
                }
                pc_log_live_menu_key_edges(
                    previous_menu_key_bits,
                    live_menu_keys,
                    frame_count,
                    title_active);
                previous_menu_key_bits = live_menu_keys;
                QueryPerformanceCounter(&current);
                elapsed =
                    (float)((double)(current.QuadPart - previous.QuadPart) /
                            (double)frequency.QuadPart);
                previous = current;
                if (title_active) {
                    jpb_PCLogSetCheckpoint(
                        "interactive title frame=%d mode=%u stack=%u select=%u",
                        frame_count,
                        (unsigned)menuVars.menuMode[
                            menuVars.menuModeSP & 7u],
                        (unsigned)menuVars.menuModeSP,
                        (unsigned)menuVars.mmSelect1[
                            menuVars.menuModeSP & 7u]);
                    deltaTime = elapsed > 0.1f ? 0.1f : elapsed;
                    pc_prepare_title_framebuffer(
                        &framebuffer, title_pixels);
                    result = jpb_GameRuntimeTitleFrame(
                        &runtime, &framebuffer);
                } else {
                    jpb_PCLogSetCheckpoint(
                        "interactive gameplay frame=%d level=%d model=%d motion=%d",
                        frame_count,
                        (int)(uint8_t)GameStruct.CurrentLevel,
                        (int)GameStruct.ModelSelect[0],
                        runtime.player != NULL
                            ? (int)runtime.player->currentMotion
                            : -1);
                    if (front_end_playable_handoff) {
                        pc_release_front_end_gameplay_control(
                            &runtime, GameStruct.NumPlayers);
                    }
                    result = jpb_GameRuntimeFrame(
                        &runtime, elapsed, &framebuffer, &stats);
                    if (result == JPB_GAME_RUNTIME_OK &&
                        front_end_playable_handoff) {
                        pc_release_front_end_gameplay_control(
                            &runtime, GameStruct.NumPlayers);
                    }
                }
                if (result != JPB_GAME_RUNTIME_OK) {
                    jpb_PCLog(
                        "frame failed frame=%d status=%d title=%d",
                        frame_count, result, title_active);
                    break;
                }
                if (title_active &&
                    menuVars.menuMode[menuVars.menuModeSP & 7u] !=
                        logged_menu_mode) {
                    logged_menu_mode = menuVars.menuMode[
                        menuVars.menuModeSP & 7u];
                    jpb_PCLog(
                        "menu transition frame=%d mode=%u stack=%u select=%u "
                        "level=%d players=%d models=%d/%d",
                        frame_count,
                        (unsigned)logged_menu_mode,
                        (unsigned)menuVars.menuModeSP,
                        (unsigned)menuVars.mmSelect1[
                            menuVars.menuModeSP & 7u],
                        (int)(uint8_t)LevelSelect,
                        (int)GameStruct.NumPlayers,
                        (int)GameStruct.ModelSelect[0],
                        (int)GameStruct.ModelSelect[1]);
                }
                if (title_active && front_end_flow &&
                    pc_front_end_requests_gameplay(
                        &selected_front_end_level,
                        &selected_front_end_game_mode)) {
                    jpb_PCAudioDestroy(audio);
                    audio = NULL;
                    if (!pc_configure_level_asset(
                            default_assets.mesh,
                            sizeof(default_assets.mesh),
                            sLevelNames[selected_front_end_level])) {
                        jpb_PCLog(
                            "gameplay handoff failed stage=resolve-level "
                            "level=%d name=%s",
                            selected_front_end_level,
                            sLevelNames[selected_front_end_level]);
                        result = JPB_GAME_RUNTIME_LOAD_FAILED;
                        break;
                    }
                    mesh_path = default_assets.mesh;
                    result = pc_start_selected_gameplay(
                        &runtime,
                        mesh_path,
                        &selected_player_assets,
                        enemy_cad_path,
                        enemy_bmd_path,
                        selected_front_end_level,
                        selected_front_end_game_mode,
                        &input);
                    if (result != JPB_GAME_RUNTIME_OK) {
                        break;
                    }
                    cad_path = selected_player_assets.cad;
                    bmd_path = selected_player_assets.bmd;
                    cmb_path = selected_player_assets.cmb;
#if defined(JPB_PC_HAS_UFBX)
                    if (!pc_attach_fbx_level(
                            mesh_path,
                            &runtime,
                            &fbx_level,
                            &fbx_level_loaded)) {
                        result = JPB_GAME_RUNTIME_LOAD_FAILED;
                        break;
                    }
#endif
                    initial_position = runtime.physics->pos;
                    memset(
                        input.observedPlayerBits,
                        0,
                        sizeof(input.observedPlayerBits));
                    title_active = 0;
                    front_end_flow = 0;
                    front_end_playable_handoff = 1;
                    ++gameplay_handoff_count;
                    jpb_PCLog(
                        "front end completed; gameplay initialized "
                        "level=%d name=%s model=%d cad=%s bmd=%s",
                        selected_front_end_level,
                        sLevelNames[selected_front_end_level],
                        (int)GameStruct.ModelSelect[0],
                        cad_path,
                        bmd_path);
                    if (!mute) {
                        audio = pc_create_current_game_audio(
                            mesh_path,
                            cad_path,
                            jpb_LevelIndexFromPath(mesh_path),
                            audio_output_enabled,
                            &audio_generation_count);
                    }
                } else if (title_active && front_end_flow) {
                    int selected_level;

                    if (pc_front_end_requests_training(&selected_level)) {
                        jpb_PCAudioDestroy(audio);
                        audio = NULL;
                        GameStruct.NumPlayers = 1;
                        if (!pc_configure_level_asset(
                                default_assets.mesh,
                                sizeof(default_assets.mesh),
                                sLevelNames[selected_level])) {
                            result = JPB_GAME_RUNTIME_LOAD_FAILED;
                            break;
                        }
                        mesh_path = default_assets.mesh;
                        result = pc_start_selected_gameplay(
                            &runtime,
                            mesh_path,
                            &selected_player_assets,
                            NULL,
                            NULL,
                            selected_level,
                            2,
                            &input);
                        if (result != JPB_GAME_RUNTIME_OK) {
                            break;
                        }
#if defined(JPB_PC_HAS_UFBX)
                        if (!pc_attach_fbx_level(
                                mesh_path,
                                &runtime,
                                &fbx_level,
                                &fbx_level_loaded)) {
                            result = JPB_GAME_RUNTIME_LOAD_FAILED;
                            break;
                        }
#endif
                        cad_path = selected_player_assets.cad;
                        bmd_path = selected_player_assets.bmd;
                        cmb_path = selected_player_assets.cmb;
                        enemy_cad_path = NULL;
                        enemy_bmd_path = NULL;
                        initial_position = runtime.physics->pos;
                        memset(
                            input.observedPlayerBits,
                            0,
                            sizeof(input.observedPlayerBits));
                        title_active = 0;
                        front_end_flow = 0;
                        front_end_playable_handoff = 1;
                        ++gameplay_handoff_count;
                        jpb_PCLog(
                            "training gameplay initialized level=%d name=%s",
                            selected_level,
                            sLevelNames[selected_level]);
                        if (!mute) {
                            audio = pc_create_current_game_audio(
                                mesh_path,
                                cad_path,
                                selected_level,
                                audio_output_enabled,
                                &audio_generation_count);
                        }
                    } else if (pc_front_end_requests_versus(
                                   &selected_level)) {
                        jpb_PCAudioDestroy(audio);
                        audio = NULL;
                        if (!pc_configure_level_asset(
                                default_assets.mesh,
                                sizeof(default_assets.mesh),
                                sLevelNames[selected_level])) {
                            result = JPB_GAME_RUNTIME_LOAD_FAILED;
                            break;
                        }
                        mesh_path = default_assets.mesh;
                        result = pc_start_selected_gameplay(
                            &runtime,
                            mesh_path,
                            &selected_player_assets,
                            NULL,
                            NULL,
                            selected_level,
                            2,
                            &input);
                        if (result != JPB_GAME_RUNTIME_OK) {
                            break;
                        }
#if defined(JPB_PC_HAS_UFBX)
                        if (!pc_attach_fbx_level(
                                mesh_path,
                                &runtime,
                                &fbx_level,
                                &fbx_level_loaded)) {
                            result = JPB_GAME_RUNTIME_LOAD_FAILED;
                            break;
                        }
#endif
                        cad_path = selected_player_assets.cad;
                        bmd_path = selected_player_assets.bmd;
                        cmb_path = selected_player_assets.cmb;
                        enemy_cad_path = NULL;
                        enemy_bmd_path = NULL;
                        initial_position = runtime.physics->pos;
                        memset(
                            input.observedPlayerBits,
                            0,
                            sizeof(input.observedPlayerBits));
                        title_active = 0;
                        front_end_flow = 0;
                        front_end_playable_handoff = 1;
                        ++gameplay_handoff_count;
                        jpb_PCLog(
                            "versus gameplay initialized level=%d name=%s "
                            "p1=%d p2=%d",
                            selected_level,
                            sLevelNames[selected_level],
                            (int)GameStruct.ModelSelect[0],
                            (int)GameStruct.ModelSelect[1]);
                        if (!mute) {
                            audio = pc_create_current_game_audio(
                                mesh_path,
                                cad_path,
                                selected_level,
                                audio_output_enabled,
                                &audio_generation_count);
                        }
                    }
                }
                if (!title_active) {
                    if (!logged_gameplay_view && runtime.physics != NULL) {
                        jpb_PCLog(
                            "gameplay view initialized frame=%d "
                            "level=%d model=%d input=%s "
                            "player=(%.1f,%.1f,%.1f,facing=%d) "
                            "target=(%.1f,%.1f,%.1f) "
                            "camera=(focus=%d,%d,%d angle=%d,%d,%d "
                            "dolly=%d authored=%u collision=%.3f)",
                            frame_count,
                            (int)(uint8_t)GameStruct.CurrentLevel,
                            (int)GameStruct.ModelSelect[0],
                            player1InputType == 0
                                ? "keyboard"
                                : "xinput",
                            runtime.physics->pos.vx,
                            runtime.physics->pos.vy,
                            runtime.physics->pos.vz,
                            physics_gGetFacing(&runtime.player->playerRoot),
                            runtime.targetX,
                            runtime.targetY,
                            runtime.targetZ,
                            runtime.camera.focus.vx,
                            runtime.camera.focus.vy,
                            runtime.camera.focus.vz,
                            runtime.camera.angle.vx,
                            runtime.camera.angle.vy,
                            runtime.camera.angle.vz,
                            runtime.authoredCameraDolly,
                            (unsigned)runtime.authoredCameraFrameCount,
                            runtime.cameraCollisionFraction);
                        logged_gameplay_view = 1;
                    }
                    pc_log_gameplay_controls(
                        frame_count,
                        &runtime,
                        gameplay_log_states);
                    jpb_PCAudioUpdate(audio);
                }
                if (pc_present(window, &framebuffer)) {
                    ++presentation_frame_count;
                }
                ++frame_count;
                Sleep(1);
            }
            if (IsWindow(window)) {
                DestroyWindow(window);
            }
        }
    }
    if (overlay_mode_override >= 0) {
        OptionStruct.overlayMode = (uint8_t)overlay_mode_override;
        defaultOptionStruct.overlayMode = (uint8_t)overlay_mode_override;
    }

    if (result == JPB_GAME_RUNTIME_OK && output_path != NULL &&
        !pc_write_ppm(output_path, &framebuffer)) {
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validatePresentationHandoff &&
        (title_active || gameplay_handoff_count != 1 ||
         presentation_frame_count != (unsigned)frame_count)) {
        fprintf(
            stderr,
            "interactive presentation handoff validation failed "
            "(title=%d handoffs=%u presents=%u/%d hidden=%d)\n",
            title_active,
            gameplay_handoff_count,
            presentation_frame_count,
            frame_count,
            input.hiddenWindow);
        jpb_PCLog(
            "presentation handoff validation failed title=%d "
            "handoffs=%u presents=%u/%d hidden=%d",
            title_active,
            gameplay_handoff_count,
            presentation_frame_count,
            frame_count,
            input.hiddenWindow);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK && input.validateAudioHandoff &&
        (title_active || gameplay_handoff_count != 1 || audio == NULL ||
         audio_generation_count != 2)) {
        fprintf(
            stderr,
            "interactive audio handoff validation failed "
            "(title=%d handoffs=%u active=%d generations=%u output=%d)\n",
            title_active,
            gameplay_handoff_count,
            audio != NULL,
            audio_generation_count,
            audio_output_enabled);
        jpb_PCLog(
            "audio handoff validation failed title=%d handoffs=%u "
            "active=%d generations=%u output=%d",
            title_active,
            gameplay_handoff_count,
            audio != NULL,
            audio_generation_count,
            audio_output_enabled);
        result = JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        validate_player_two_sound != NULL) {
        DWORD sound_attributes;

        player_two_audio_validated =
            GameStruct.NumPlayers == 2 && audio != NULL &&
            jpb_PCAudioResolveSound(
                audio,
                2,
                validate_player_two_sound,
                validated_player_two_sound_path,
                sizeof(validated_player_two_sound_path));
        sound_attributes = player_two_audio_validated
            ? GetFileAttributesA(validated_player_two_sound_path)
            : INVALID_FILE_ATTRIBUTES;
        if (!player_two_audio_validated ||
            sound_attributes == INVALID_FILE_ATTRIBUTES ||
            (sound_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            fprintf(
                stderr,
                "player-two audio validation failed "
                "(players=%d sound=%s path=%s)\n",
                (int)GameStruct.NumPlayers,
                validate_player_two_sound,
                validated_player_two_sound_path[0] != '\0'
                    ? validated_player_two_sound_path
                    : "<unresolved>");
            jpb_PCLog(
                "player-two audio validation failed players=%d "
                "sound=%s path=%s",
                (int)GameStruct.NumPlayers,
                validate_player_two_sound,
                validated_player_two_sound_path[0] != '\0'
                    ? validated_player_two_sound_path
                    : "<unresolved>");
            result = JPB_GAME_RUNTIME_LOAD_FAILED;
        } else {
            jpb_PCLog(
                "player-two audio validation complete sound=%s path=%s",
                validate_player_two_sound,
                validated_player_two_sound_path);
        }
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validatePersistenceHandoff &&
        (!input.noValidGameSave || input.gameSaveWriteCount != 0 ||
         !pc_validate_persistence_round_trip(&input))) {
        fprintf(
            stderr,
            "interactive persistence handoff validation failed "
            "(enabled=%d no_game=%d game_writes=%u path=%s)\n",
            input.persistenceEnabled,
            input.noValidGameSave,
            input.gameSaveWriteCount,
            input.optionsPath);
        result = JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK && input.validateNeutralHandoff &&
        (runtime.controlObservedPressedBits[0] != 0 ||
         runtime.controlObservedHeldBits[0] != 0 ||
         (runtime.secondPlayerState != NULL &&
          (runtime.controlObservedPressedBits[1] != 0 ||
           runtime.controlObservedHeldBits[1] != 0)))) {
        fprintf(
            stderr,
            "menu input crossed the gameplay handoff "
            "(p1=%08x/%08x p2=%08x/%08x guard=%02x)\n",
            (unsigned)runtime.controlObservedPressedBits[0],
            (unsigned)runtime.controlObservedHeldBits[0],
            (unsigned)runtime.controlObservedPressedBits[1],
            (unsigned)runtime.controlObservedHeldBits[1],
            (unsigned)input.gameplayHandoffReleaseMask);
        jpb_PCLog(
            "neutral handoff validation failed p1=%08x/%08x "
            "p2=%08x/%08x guard=%02x",
            (unsigned)runtime.controlObservedPressedBits[0],
            (unsigned)runtime.controlObservedHeldBits[0],
            (unsigned)runtime.controlObservedPressedBits[1],
            (unsigned)runtime.controlObservedHeldBits[1],
            (unsigned)input.gameplayHandoffReleaseMask);
        result = JPB_GAME_RUNTIME_LOAD_FAILED;
    }
    if (title_active) {
        unsigned title_menu = menuVars.menuMode[
            menuVars.menuModeSP & 7u];
        uint32_t title_center = framebuffer.pixels[
            (size_t)(framebuffer.height / 2) *
                (size_t)framebuffer.stridePixels +
            (size_t)(framebuffer.width / 2)];
        unsigned title_center_brightness =
            ((title_center >> 16) & UINT32_C(0xff)) +
            ((title_center >> 8) & UINT32_C(0xff)) +
            (title_center & UINT32_C(0xff));

        if (result == JPB_GAME_RUNTIME_OK &&
            title_character_select &&
            (!input.headless ||
             runtime.screenDrawCount <
                 (title_menu == 0x1a
                      ? 2u
                      : (title_character_select == 2
                           ? (title_character_select_completed
                                  ? 26u : 22u)
                           : (title_character_select_completed
                                  ? 21u : 15u))) ||
             runtime.screenDrawDroppedCount != 0 ||
             runtime.screenDrawCompositePixelCount == 0 ||
             runtime.textDrawCount <
                 (title_menu == 0x1a
                      ? 0u
                      : (title_character_select == 2
                           ? (title_character_select_completed
                                  ? 12u : 8u)
                           : (title_character_select_completed
                                  ? 6u : 4u))) ||
             (title_menu != 0x1a &&
              title_center_brightness < 180))) {
            fprintf(
                stderr,
                "headless character-select presentation did not composite "
                "the recovered menu owner "
                "(screen=%zu/%zu/%zu text=%zu center=%08x/%u)\n",
                runtime.screenDrawCount,
                runtime.screenDrawDroppedCount,
                runtime.screenDrawCompositePixelCount,
                runtime.textDrawCount,
                (unsigned)title_center,
                title_center_brightness);
            result = JPB_GAME_RUNTIME_RENDER_FAILED;
        }
        if (result == JPB_GAME_RUNTIME_OK &&
            input.validateTitleAudio &&
            (!input.headless ||
             title_menu != 0x10 ||
             OptionStruct.Music != 0 ||
             runtime.textDrawCount < 5 ||
             runtime.textDrawDroppedCount != 0)) {
            fprintf(
                stderr,
                "headless title audio-menu validation did not enter "
                "the variable-backed menu and toggle Music "
                "(mode=%u music=%u text=%zu/%zu)\n",
                title_menu,
                (unsigned)OptionStruct.Music,
                runtime.textDrawCount,
                runtime.textDrawDroppedCount);
            result = JPB_GAME_RUNTIME_RENDER_FAILED;
        }
        printf(
            "menu_state=(active=%d,mode=%u,stack=%u,select=%u,"
            "level=%d,level_box=%d/%u/%u,countdown=%u,open=%u,"
            "preview=%u/fade=%d,selector_offset=%d,"
            "pplayers=%u/%u,subplayers=%u/%u,"
            "current_models=%d/%d/%d/%d,players=%d,models=%d/%d,"
            "framebuffer=%dx%d,scale=%.3f/%.3f,frontRGBoff=%u)\n",
            title_active,
            (unsigned)menuVars.menuMode[menuVars.menuModeSP & 7u],
            (unsigned)menuVars.menuModeSP,
            (unsigned)menuVars.mmSelect1[menuVars.menuModeSP & 7u],
            (int)(uint8_t)LevelSelect,
            (int)menuVars.levelSelectBoxTop,
            (unsigned)menuVars.levelSelectBoxWidth,
            (unsigned)menuVars.levelSelectBoxHeight,
            (unsigned)menuVars.levelSelectBoxCountdown,
            (unsigned)menuVars.levelSelectBoxOpen,
            (unsigned)menuVars.levelSelectPreviewLevel,
            (int)menuVars.levelSelectPreviewFade,
            (int)menuVars.levelSelectSelectorOffset,
            (unsigned)menuVars.pplayers[0],
            (unsigned)menuVars.pplayers[1],
            (unsigned)menuVars.subplayers[0],
            (unsigned)menuVars.subplayers[1],
            newMenu_currentModelSelectBaseP1,
            newMenu_currentModelSelectNGPP1,
            newMenu_currentModelSelectBaseP2,
            newMenu_currentModelSelectNGPP2,
            (int)GameStruct.NumPlayers,
            (int)GameStruct.ModelSelect[0],
            (int)GameStruct.ModelSelect[1],
            framebuffer.width,
            framebuffer.height,
            scaleAdjustment,
            scaleAdjustmentMM,
            (unsigned)frontRGBoff);
        printf(
            "frames=%d mode=title title_menu=(%u,music=%u) "
            "running=%d "
            "video=(%u,%u) controls=(%u,%u) language=%u "
            "character=(state=%d,select=%u,players=%d,models=%d/%d,"
            "disconnected=%d/%d,pad=%08x/%08x) "
            "text=%zu/%zu/%zu/%zu/%d/%dx%d "
            "screen=%zu/%zu/%zu menu_textures=%zu\n",
            frame_count,
            title_menu,
            (unsigned)OptionStruct.Music,
            pc_running,
            (unsigned)OptionStruct.WindowMode,
            (unsigned)OptionStruct.ResolutionChanged,
            (unsigned)OptionStruct.ControllerConfig[0],
            (unsigned)OptionStruct.ControllerConfig[1],
            (unsigned)OptionStruct.Language,
            newMenu_state,
            (unsigned)newMenu_select,
            (int)GameStruct.NumPlayers,
            (int)GameStruct.ModelSelect[0],
            (int)GameStruct.ModelSelect[1],
            (int)p1Disconnected,
            (int)p2Disconnected,
            (unsigned)menuVars.pad[0],
            (unsigned)menuVars.pad[1],
            runtime.textDrawCount,
            runtime.textTrueTypeDrawCount,
            runtime.textFallbackDrawCount,
            runtime.textDrawCompositePixelCount,
            runtime.maximumTextPointSize,
            runtime.maximumTextMeasuredWidth,
            runtime.maximumTextMeasuredHeight,
            runtime.screenDrawCount,
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            menu_texture_cache != NULL
                ? menu_texture_cache->count
                : 0);
        pc_print_screen_draw_trace(&runtime);
        pc_print_screen_poly_trace(&runtime);
        pc_print_text_draw_trace(&runtime);
        pc_print_psx_texture_draw_trace(&runtime);
        goto cleanup;
    }
    pc_collect_player_saber_diagnostics(
        &runtime, runtime.player, &saber_diagnostics);
    if (runtime.secondPlayerState != NULL) {
        pc_collect_player_saber_diagnostics(
            &runtime,
            runtime.inactivePlayer,
            &second_saber_diagnostics);
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validatePlayerSaber &&
        (!input.headless ||
         runtime.glowDrawDroppedCount != 0 ||
         !pc_validate_player_saber(&saber_diagnostics))) {
        fprintf(
            stderr,
            "headless character-owned saber validation failed "
            "(model=%d expected=%d color=%08x outer=%zu trail=%zu "
            "core=%zu attached=%zu unmatched=%zu "
            "width=%d..%d length=%.1f..%.1f dropped=%zu)\n",
            saber_diagnostics.playerModel,
            saber_diagnostics.expectsSaber,
            (unsigned)saber_diagnostics.outerColor,
            saber_diagnostics.outerDrawCount,
            saber_diagnostics.trailDrawCount,
            saber_diagnostics.matchedCoreDrawCount,
            saber_diagnostics.matchedAttachmentDrawCount,
            saber_diagnostics.unmatchedAttachmentDrawCount,
            saber_diagnostics.minimumOuterWidth,
            saber_diagnostics.maximumOuterWidth,
            saber_diagnostics.minimumBladeLength,
            saber_diagnostics.maximumBladeLength,
            runtime.glowDrawDroppedCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validatePlayerSaber && input.headless &&
        runtime.secondPlayerState != NULL &&
        (runtime.glowDrawDroppedCount != 0 ||
         !pc_validate_player_saber(&second_saber_diagnostics))) {
        fprintf(
            stderr,
            "headless second-player character-owned saber validation failed "
            "(model=%d expected=%d color=%08x outer=%zu trail=%zu "
            "core=%zu attached=%zu unmatched=%zu "
            "width=%d..%d length=%.1f..%.1f dropped=%zu)\n",
            second_saber_diagnostics.playerModel,
            second_saber_diagnostics.expectsSaber,
            (unsigned)second_saber_diagnostics.outerColor,
            second_saber_diagnostics.outerDrawCount,
            second_saber_diagnostics.trailDrawCount,
            second_saber_diagnostics.matchedCoreDrawCount,
            second_saber_diagnostics.matchedAttachmentDrawCount,
            second_saber_diagnostics.unmatchedAttachmentDrawCount,
            second_saber_diagnostics.minimumOuterWidth,
            second_saber_diagnostics.maximumOuterWidth,
            second_saber_diagnostics.minimumBladeLength,
            second_saber_diagnostics.maximumBladeLength,
            runtime.glowDrawDroppedCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validatePlayerSaber && input.headless &&
        saber_action_diagnostics.actionFrames != 0 &&
        saber_action_diagnostics.invalidActionFrames != 0) {
        fprintf(
            stderr,
            "headless action-window saber validation failed "
            "(model=%d expected=%d frames=%zu valid=%zu invalid=%zu "
            "attached=%zu trails=%zu width=%d..%d length=%.1f..%.1f)\n",
            saber_action_diagnostics.playerModel,
            saber_action_diagnostics.expectsSaber,
            saber_action_diagnostics.actionFrames,
            saber_action_diagnostics.validActionFrames,
            saber_action_diagnostics.invalidActionFrames,
            saber_action_diagnostics.attachedBladeDrawCount,
            saber_action_diagnostics.trailDrawCount,
            saber_action_diagnostics.minimumOuterWidth,
            saber_action_diagnostics.maximumOuterWidth,
            saber_action_diagnostics.minimumBladeLength,
            saber_action_diagnostics.maximumBladeLength);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validatePlayerSaber && input.headless &&
        runtime.secondPlayerState != NULL &&
        second_saber_action_diagnostics.actionFrames != 0 &&
        second_saber_action_diagnostics.invalidActionFrames != 0) {
        fprintf(
            stderr,
            "headless second-player action-window saber validation failed "
            "(model=%d expected=%d frames=%zu valid=%zu invalid=%zu "
            "attached=%zu trails=%zu width=%d..%d length=%.1f..%.1f)\n",
            second_saber_action_diagnostics.playerModel,
            second_saber_action_diagnostics.expectsSaber,
            second_saber_action_diagnostics.actionFrames,
            second_saber_action_diagnostics.validActionFrames,
            second_saber_action_diagnostics.invalidActionFrames,
            second_saber_action_diagnostics.attachedBladeDrawCount,
            second_saber_action_diagnostics.trailDrawCount,
            second_saber_action_diagnostics.minimumOuterWidth,
            second_saber_action_diagnostics.maximumOuterWidth,
            second_saber_action_diagnostics.minimumBladeLength,
            second_saber_action_diagnostics.maximumBladeLength);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validatePlayerProjectile &&
        (!input.headless ||
         runtime.playerProjectileLaunchCount[0] == 0 ||
         runtime.lastPlayerProjectileType[0] < 0 ||
         runtime.lastPlayerProjectileType[0] >=
             JPB_PROJECTILE_GLOBAL_CAPACITY ||
         (runtime.lastPlayerProjectileStart[0].vx ==
              runtime.lastPlayerProjectileTarget[0].vx &&
          runtime.lastPlayerProjectileStart[0].vy ==
              runtime.lastPlayerProjectileTarget[0].vy &&
          runtime.lastPlayerProjectileStart[0].vz ==
              runtime.lastPlayerProjectileTarget[0].vz))) {
        fprintf(
            stderr,
            "headless character-owned projectile validation failed "
            "(model=%d launches=%zu type=%d flags=%08x "
            "start=%d,%d,%d target=%d,%d,%d)\n",
            (int)runtime.player->playerID,
            runtime.playerProjectileLaunchCount[0],
            (int)runtime.lastPlayerProjectileType[0],
            (unsigned)runtime.lastPlayerProjectileFlags[0],
            runtime.lastPlayerProjectileStart[0].vx,
            runtime.lastPlayerProjectileStart[0].vy,
            runtime.lastPlayerProjectileStart[0].vz,
            runtime.lastPlayerProjectileTarget[0].vx,
            runtime.lastPlayerProjectileTarget[0].vy,
            runtime.lastPlayerProjectileTarget[0].vz);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK && input.headless &&
        quickload_level == NULL &&
        stats.triangles > 0 &&
        (stats.pixels == 0 ||
         runtime.worldRenderedPixels == 0 ||
         (runtime.collisionReady &&
          runtime.authoredCameraFrameCount == 0 &&
          !(GameStruct.versusModeFlag != 0 &&
            GameStruct.NumPlayers == 2)) ||
         /* FED is the material/effect reference fixture. Other shipped maps
          * legitimately have no transparent or loose-texture pass, so keep
          * those map-specific assertions from rejecting an otherwise filled
          * and camera-visible gameplay frame. */
         ((uint8_t)GameStruct.CurrentLevel == 0 &&
          ((stats.lines != 0 &&
            (runtime.glowDrawCompositePixelCount == 0 ||
             stats.lines > runtime.glowDrawCount)) ||
           stats.levelTransparentTriangles == 0 ||
           /* The multi-class overlap faces no visible transparent pixels;
            * the ordinary FED smoke retains that material assertion. */
           (!input.validateMultiEnemy &&
            stats.levelTransparentPixels == 0) ||
           runtime.worldLoadedTextures == 0 ||
           (!input.validateCombat &&
            !input.validateMultiEnemy &&
            bmd_path != NULL &&
            (runtime.glowDrawCount < 2 ||
             runtime.glowDrawDroppedCount != 0 ||
             runtime.glowDrawCompositePixelCount == 0)))))) {
        fprintf(
            stderr,
            "headless validation did not produce a textured, filled world "
            "through the selected camera "
            "(level=%u pixels=%zu transparent=%zu world=%zu "
            "textures=%zu glow=%zu/%zu lines=%zu camera=%u)\n",
            (unsigned)(uint8_t)GameStruct.CurrentLevel,
            stats.pixels,
            stats.levelTransparentPixels,
            runtime.worldRenderedPixels,
            runtime.worldLoadedTextures,
            runtime.glowDrawCount,
            runtime.glowDrawCompositePixelCount,
            stats.lines,
            (unsigned)runtime.authoredCameraFrameCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK && input.headless &&
        GameStruct.NumPlayers == 2 &&
        !input.validateHudArena &&
        !input.validateHudArena1080 &&
        (!jpb_GameRuntimeSecondPlayerReady(&runtime) ||
         jpb_GameRuntimeSecondPlayerRenderedTriangles(&runtime) == 0 ||
         jpb_GameRuntimeSecondPlayerRenderedPixels(&runtime) == 0)) {
        fputs(
            "headless two-player validation did not pose and render "
            "the second selected character\n",
            stderr);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.observedPlayerBits[0] != 0) {
        uint32_t observed_attack_bits =
            input.observedPlayerBits[0] &
            (JPB_PAD_COMBO_NORTH |
             JPB_PAD_COMBO_SOUTH |
             JPB_PAD_COMBO_WEST);
        uint8_t controller_config = OptionStruct.ControllerConfig[0];
        uint32_t force_modifier =
            controller_config < 5
                ? (uint32_t)gaButtonMap[controller_config][4]
                : JPB_PAD_LEFT_TRIGGER;
        int expected_attack_motion =
            pc_player_authored_attack_motion(
                runtime.player, observed_attack_bits);
        int expected_running_attack_motion =
            pc_player_running_attack_motion(
                observed_attack_bits);
        /* The authored frame is posed/rendered before player control selects
         * the next frame. Attack-to-idle after rendering is not a pose loss. */
        int authored_state_invalid =
            !runtime.authoredMotionReady ||
            !runtime.authoredFrameReady ||
            (bmd_path != NULL &&
             (!runtime.authoredPoseReady ||
               runtime.actorModel.pRootNode == NULL ||
               runtime.playerRenderedTriangles == 0));

        if (authored_state_invalid) {
            fputs(
                "headless authored-action validation did not pose the actor\n",
                stderr);
            result = JPB_GAME_RUNTIME_RENDER_FAILED;
        }
        if (result == JPB_GAME_RUNTIME_OK &&
            (input.observedPlayerBits[0] &
             (JPB_PAD_UP | JPB_PAD_LEFT |
              JPB_PAD_DOWN | JPB_PAD_RIGHT)) != 0 &&
            ((runtime.authoredLocomotionMotionFrameCount == 0 &&
              (input.observedPlayerBits[0] & JPB_PAD_JUMP) == 0) ||
             (runtime.physics->pos.vx == initial_position.vx &&
              runtime.physics->pos.vz == initial_position.vz))) {
            fputs(
                "headless authored-motion validation did not move the actor\n",
                stderr);
            result = JPB_GAME_RUNTIME_RENDER_FAILED;
        }
        if (result == JPB_GAME_RUNTIME_OK &&
            observed_attack_bits != 0 &&
            /* Classic uses LB (logical block) as its Force modifier while
             * Modern uses LT. A fixed LT exclusion misclassified valid
             * Classic Force motions as failed ordinary attacks. */
            (input.observedPlayerBits[0] & force_modifier) == 0 &&
            ((runtime.authoredRunningAttackFrameCount != 0 &&
              (expected_running_attack_motion < 0 ||
               runtime.lastAuthoredRunningAttackMotion !=
                   expected_running_attack_motion)) ||
             (runtime.authoredRunningAttackFrameCount == 0 &&
              (runtime.player->maxCombos == 0 ||
               runtime.authoredDamageMotionFrameCount == 0 ||
               ((expected_attack_motion < 0 ||
                 runtime.lastAuthoredDamageMotion !=
                     expected_attack_motion) &&
                !pc_player_is_authored_combo_motion(
                    runtime.player,
                    runtime.lastAuthoredDamageMotion,
                    observed_attack_bits)))))) {
            fprintf(
                stderr,
                "headless authored-attack validation failed: "
                "bits=%08x expected_motion=%d observed_motion=%d "
                "running_motion=%d running_frames=%u "
                "damage_frames=%u hot_frames=%u\n",
                (unsigned)observed_attack_bits,
                expected_attack_motion,
                (int)runtime.lastAuthoredDamageMotion,
                (int)runtime.lastAuthoredRunningAttackMotion,
                (unsigned)runtime.authoredRunningAttackFrameCount,
                (unsigned)runtime.authoredDamageMotionFrameCount,
                (unsigned)runtime.authoredHotFrameCount);
            result = JPB_GAME_RUNTIME_RENDER_FAILED;
        }
        if (result == JPB_GAME_RUNTIME_OK &&
            (input.validateCombat ||
             runtime.combatHitCount != 0) &&
            (!input.headless ||
             enemy_cad_path == NULL ||
             (input.observedPlayerBits[0] &
              JPB_PAD_COMBO_NORTH) == 0 ||
             runtime.combatHitCount == 0 ||
             runtime.enemyDamageProcessedCount == 0 ||
             runtime.enemyMinimumEnergy >=
                 runtime.enemyInitialEnergy ||
             (runtime.enemyReactionMotionFrameCount == 0 &&
              runtime.enemyRecoilReactionCount == 0))) {
            fputs(
                "headless authored-combat validation did not process "
                "enemy damage and reaction\n",
                stderr);
            result = JPB_GAME_RUNTIME_RENDER_FAILED;
        }
        if (result == JPB_GAME_RUNTIME_OK &&
            input.validateJump &&
            (((input.observedPlayerBits[0] & JPB_PAD_JUMP) == 0) ||
             runtime.player->pSettings.JumpVel != 0x7a ||
             runtime.player->pSettings.RunningJumpVel != 0x73 ||
             runtime.player->pSettings.dblJumpVel != 0x73 ||
             runtime.player->pSettings.JumpAngle != 0x2f9 ||
             runtime.player->pSettings.RunningJumpAngle != 0x341 ||
             runtime.player->pSettings.dblJumpAngle != 0x35a ||
             runtime.player->pSettings.bkJumpAngle != 0x555 ||
             runtime.player->pSettings.gravity != UINT16_C(0xe314) ||
             runtime.player->pSettings.dblgravity != 0 ||
             runtime.player->pSettings.minClosingDist != 0x14 ||
             jump_airborne_frames == 0 ||
             runtime.physics->pos.vy <= initial_position.vy)) {
            fputs(
                "headless authored-jump validation did not launch "
                "the actor with the recovered Jedi settings\n",
                stderr);
            result = JPB_GAME_RUNTIME_RENDER_FAILED;
        }
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.headless && enemy_cad_path != NULL &&
        quickload_level == NULL) {
        int enemy_state_invalid =
            runtime.enemy == NULL ||
            runtime.enemyPlayer == NULL ||
            runtime.enemy->pPlayer != runtime.enemyPlayer ||
            (!input.validateCombat &&
             enemyList[mCurEnemyList].head !=
                 &runtime.enemy->node) ||
            runtime.enemy->pPlace == NULL ||
            runtime.enemy->pPlace->actorNum !=
                runtime.enemy->actorNum ||
            runtime.enemy->pPlace->status != 1 ||
            runtime.enemy->pPlace->pLastEnemy ==
                UINT32_MAX ||
            getPtr(
                (int)runtime.enemy->pPlace->pLastEnemy,
                JPB_POINTER_ARRAY_ENEMY) !=
                runtime.enemy ||
            runtime.enemy->enemyID !=
                runtime.enemy->enemyNum ||
            runtime.enemyPlayer->playerID !=
                jpb_GameRuntimeEnemyClassModelId(
                    &runtime,
                    runtime.enemy->actorNum) ||
            runtime.enemyMainCallbackFrameCount == 0 ||
            runtime.enemyAuthoredOpcodeFrameCount == 0 ||
            runtime.enemyOpcodeBoundaryFrameCount != 0 ||
            runtime.enemyPlayer->paiMemory == NULL ||
            runtime.enemyAiStorageSize <
                sizeof(aiData) ||
            ai_GetAIHandle(
                runtime.enemy->actorNum,
                runtime.enemyAiLevel) !=
                runtime.enemyPlayer->paiMemory ||
            runtime.enemyPlayer->target != runtime.player ||
            !runtime.enemyAuthoredMotionReady ||
            !runtime.enemyAuthoredFrameReady ||
            !runtime.enemyAuthoredPoseReady ||
            runtime.enemyModel.pRootNode == NULL ||
            runtime.enemyRenderedTriangles == 0 ||
            /* The radar gate deliberately enables DebugLevel 3, whose HUD
             * pass may republish the shared scene keyframe pointer after the
             * tracked enemy has already posed and submitted.  Its dedicated
             * assertions cover that presentation pass; exact enemy keyframe
             * ownership remains required by every ordinary enemy run. */
            (!input.validateRadar &&
             !input.validateRadar1080 &&
             runtime.enemyScene->pKeyFrameModel !=
                 runtime.enemyAnimation->pCurrentAnimFrame);

        if (enemy_state_invalid) {
            fprintf(
                stderr,
                "headless enemy validation did not own, pose, and submit "
                "the authored enemy (visibility may be world-occluded) "
                "(main=%d authored_ai=%u ai_boundary=%u/0x%03x "
                "kungfu=%d ai=%d ai_size=%zu ai_level=%d "
                "ai_registered=%d actors=%zu/%zu/%zu "
                "owner=%d head=%d place=%d ptr=%d ids=%d/%d "
                "model=%d/%d target=%d/%d "
                "pose=%d/%d/%d root=%d keyframe=%d submit=%d)\n",
                runtime.enemyMainCallbackFrameCount != 0,
                (unsigned)
                    runtime.enemyAuthoredOpcodeFrameCount,
                (unsigned)
                    runtime.enemyOpcodeBoundaryFrameCount,
                (unsigned)runtime.enemyOpcodeBoundary,
                runtime.enemyKungfuSchedulerFrameCount != 0,
                runtime.enemyPlayer != NULL &&
                    runtime.enemyPlayer->paiMemory != NULL,
                runtime.enemyAiStorageSize,
                (int)runtime.enemyAiLevel,
                runtime.enemyPlayer != NULL &&
                    runtime.enemy != NULL &&
                    ai_GetAIHandle(
                        runtime.enemy->actorNum,
                        runtime.enemyAiLevel) ==
                        runtime.enemyPlayer->paiMemory,
                runtime.enemyActorCount,
                runtime.enemyActorPeakCount,
                runtime.enemySpawnCount,
                runtime.enemy != NULL &&
                    runtime.enemy->pPlayer ==
                        runtime.enemyPlayer,
                runtime.enemy != NULL &&
                    enemyList[mCurEnemyList].head ==
                        &runtime.enemy->node,
                runtime.enemy != NULL &&
                    runtime.enemy->pPlace != NULL &&
                    runtime.enemy->pPlace->actorNum ==
                        runtime.enemy->actorNum &&
                    runtime.enemy->pPlace->status == 1,
                runtime.enemy != NULL &&
                    runtime.enemy->pPlace != NULL &&
                    runtime.enemy->pPlace->pLastEnemy !=
                        UINT32_MAX &&
                    getPtr(
                        (int)runtime.enemy->pPlace
                            ->pLastEnemy,
                        JPB_POINTER_ARRAY_ENEMY) ==
                        runtime.enemy,
                runtime.enemy != NULL
                    ? runtime.enemy->enemyID
                    : -1,
                runtime.enemy != NULL
                    ? runtime.enemy->enemyNum
                    : -1,
                runtime.enemyPlayer != NULL
                    ? runtime.enemyPlayer->playerID
                    : -1,
                runtime.enemy != NULL
                    ? jpb_GameRuntimeEnemyClassModelId(
                          &runtime,
                          runtime.enemy->actorNum)
                    : -1,
                runtime.enemyPlayer != NULL &&
                    runtime.player->target ==
                        runtime.enemyPlayer,
                runtime.enemyPlayer != NULL &&
                    runtime.enemyPlayer->target ==
                        runtime.player,
                runtime.enemyAuthoredMotionReady,
                runtime.enemyAuthoredFrameReady,
                runtime.enemyAuthoredPoseReady,
                runtime.enemyModel.pRootNode != NULL,
                runtime.enemyScene != NULL &&
                    runtime.enemyAnimation != NULL &&
                    runtime.enemyScene->pKeyFrameModel ==
                        runtime.enemyAnimation->pCurrentAnimFrame,
                runtime.enemyRenderedTriangles != 0);
            result = JPB_GAME_RUNTIME_RENDER_FAILED;
        }
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateMultiEnemy &&
        (runtime.enemyActorCount < 2 ||
         runtime.enemyActorPeakCount < 2 ||
         runtime.enemySpawnCount < 2 ||
         runtime.enemyLoadedClassCount < 2 ||
         runtime.enemyPlacedClassCount < 2 ||
         runtime.enemyPlacedClassCount >
             runtime.enemyLoadedClassCount ||
         runtime.enemyActiveClassPeakCount < 2 ||
         runtime.enemyActivatedClassCount !=
             runtime.enemyPlacedClassCount ||
         runtime.enemyRenderedClassCount !=
             runtime.enemyPlacedClassCount)) {
        fprintf(
            stderr,
            "headless multi-enemy validation did not retain authored "
            "actors from every exact loader-table class "
            "(actors=%zu/%zu/%zu "
            "classes=%zu/%zu/%zu/%zu/%zu/%zu)\n",
            runtime.enemyActorCount,
            runtime.enemyActorPeakCount,
            runtime.enemySpawnCount,
            runtime.enemyLoadedClassCount,
            runtime.enemyPlacedClassCount,
            runtime.enemyActiveClassCount,
            runtime.enemyActiveClassPeakCount,
            runtime.enemyActivatedClassCount,
            runtime.enemyRenderedClassCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateRadar &&
        (!input.headless ||
         !pc_validate_enemy_radar_hud_960(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         OptionStruct.ScreenWidth !=
             (uint32_t)framebuffer.width ||
         OptionStruct.ScreenHeight !=
             (uint32_t)framebuffer.height)) {
        fprintf(
            stderr,
            "headless radar validation did not queue and composite "
            "the reviewed 960x540 screen draws "
            "(draws=%zu bg=%zu player=%zu red=%zu green=%zu broad=%zu "
            "dropped=%zu pixels=%zu "
            "screen=%ux%u)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_enemy_radar_background_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_enemy_radar_player_marker_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_enemy_radar_red_marker_a_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_enemy_radar_green_marker_960),
            pc_count_enemy_radar_draws(&runtime),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            (unsigned)OptionStruct.ScreenWidth,
            (unsigned)OptionStruct.ScreenHeight);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateRadar1080 &&
        (!input.headless ||
         !pc_validate_enemy_radar_hud_1080(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         OptionStruct.ScreenWidth !=
             (uint32_t)framebuffer.width ||
         OptionStruct.ScreenHeight !=
             (uint32_t)framebuffer.height)) {
        fprintf(
            stderr,
            "headless radar validation did not queue and composite "
            "the reviewed 1920x1080 screen draws "
            "(draws=%zu bg=%zu player=%zu red=%zu green=%zu broad=%zu "
            "dropped=%zu pixels=%zu "
            "screen=%ux%u)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_enemy_radar_background_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_enemy_radar_player_marker_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_enemy_radar_red_marker_a_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_enemy_radar_green_marker_1080),
            pc_count_enemy_radar_draws(&runtime),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            (unsigned)OptionStruct.ScreenWidth,
            (unsigned)OptionStruct.ScreenHeight);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudCore &&
        (!input.headless ||
         !pc_validate_core_hud_960(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless core HUD validation did not publish the "
            "reviewed 960x540 score/item/meter/text owners "
            "(draws=%zu text=%zu score=%zu item=%zu life=%zu "
            "force=%zu alpha=%zu/%zu/%zu/%zu "
            "dropped=%zu/%zu pixels=%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.textDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_score_panel_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_item_panel_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_life_bar_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_force_bar_960),
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount,
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudCore1080 &&
        (!input.headless ||
         !pc_validate_core_hud_1080(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless core HUD validation did not publish the "
            "reviewed 1920x1080 score/item/meter/text owners "
            "(draws=%zu text=%zu sprites=%zu score=%zu item=%zu life=%zu "
            "force=%zu alpha=%zu/%zu/%zu/%zu "
            "dropped=%zu/%zu/%zu pixels=%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.textDrawCount,
            runtime.spriteDisplayDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_score_panel_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_item_panel_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_life_bar_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_force_bar_1080),
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount,
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.spriteDisplayDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudNormal &&
        (!input.headless ||
         !pc_validate_normal_hud_960(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless normal gameplay HUD validation did not publish the "
            "reviewed 960x540 overlayMode 1 score/item/text owners "
            "(draws=%zu text=%zu sprites=%zu score_panel=%zu item=%zu "
            "life=%zu/%zu/%zu force=%zu/%zu/%zu player_hud=%zu/%zu/%zu "
            "alpha=%zu/%zu/%zu/%zu dropped=%zu/%zu/%zu "
            "pixels=%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.textDrawCount,
            runtime.spriteDisplayDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_score_panel_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_normal_item_panel_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_left_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_bar_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_right_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_left_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_bar_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_right_960),
            runtime.playerHudTileDrawCount,
            runtime.playerHudTileDroppedCount,
            runtime.playerHudTileCompositePixelCount,
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount,
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.spriteDisplayDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudNormal1080 &&
        (!input.headless ||
         !pc_validate_normal_hud_1080(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless normal gameplay HUD validation did not publish the "
            "reviewed 1920x1080 overlayMode 1 score/item/text owners "
            "(draws=%zu text=%zu sprites=%zu score_panel=%zu item=%zu "
            "life=%zu/%zu/%zu force=%zu/%zu/%zu player_hud=%zu/%zu/%zu "
            "alpha=%zu/%zu/%zu/%zu dropped=%zu/%zu/%zu "
            "pixels=%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.textDrawCount,
            runtime.spriteDisplayDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_score_panel_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_normal_item_panel_proof_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_left_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_bar_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_right_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_left_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_bar_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_right_1080),
            runtime.playerHudTileDrawCount,
            runtime.playerHudTileDroppedCount,
            runtime.playerHudTileCompositePixelCount,
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount,
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.spriteDisplayDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudNormalProof1080 &&
        (!input.headless ||
         !pc_validate_normal_hud_proof_1080(&runtime, &framebuffer))) {
        fprintf(
            stderr,
            "headless normal gameplay HUD proof validation did not publish "
            "the reviewed 1920x1080 overlayMode 1 full-frame HUD owners "
            "(draws=%zu text=%zu sprites=%zu score_panel=%zu item=%zu "
            "player_hud=%zu/%zu/%zu alpha=%zu/%zu/%zu/%zu "
            "dropped=%zu/%zu/%zu pixels=%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.textDrawCount,
            runtime.spriteDisplayDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_score_panel_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_normal_item_panel_proof_1080),
            runtime.playerHudTileDrawCount,
            runtime.playerHudTileDroppedCount,
            runtime.playerHudTileCompositePixelCount,
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount,
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.spriteDisplayDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudP2Core &&
        (!input.headless ||
         !pc_validate_p2_core_hud_960(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless P2 core HUD validation did not publish the "
            "reviewed 960x540 mirrored score/item/meter/text owners "
            "(players=%d draws=%zu text=%zu score=%zu item=%zu "
            "life=%zu force=%zu alpha=%zu/%zu/%zu/%zu "
            "dropped=%zu/%zu pixels=%zu/%zu)\n",
            (int)GameStruct.NumPlayers,
            runtime.screenDrawCount,
            runtime.textDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_p2_core_score_panel_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_p2_core_item_panel_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_p2_core_life_bar_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_p2_core_force_bar_960),
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount,
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudP2Core1080 &&
        (!input.headless ||
         !pc_validate_p2_core_hud_1080(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless P2 core HUD validation did not publish the "
            "reviewed 1920x1080 mirrored score/item/meter/text owners "
            "(players=%d draws=%zu text=%zu sprites=%zu score=%zu item=%zu "
            "life=%zu force=%zu alpha=%zu/%zu/%zu/%zu "
            "dropped=%zu/%zu/%zu pixels=%zu/%zu)\n",
            (int)GameStruct.NumPlayers,
            runtime.screenDrawCount,
            runtime.textDrawCount,
            runtime.spriteDisplayDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_p2_core_score_panel_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_p2_core_item_panel_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_p2_core_life_bar_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_p2_core_force_bar_1080),
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount,
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.spriteDisplayDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudContinue &&
        (!input.headless ||
         !pc_validate_continue_hud_960(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless continue HUD validation did not publish the "
            "reviewed 960x540 credit wrap owners "
            "(draws=%zu first=%zu second=%zu third=%zu fourth=%zu "
            "fifth=%zu wrapped=%zu dropped=%zu pixels=%zu "
            "screen_alpha=%zu/%zu/%zu/%zu)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_first_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_second_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_third_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_fourth_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_fifth_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_wrapped_960),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudContinue1080 &&
        (!input.headless ||
         !pc_validate_continue_hud_1080(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless continue HUD validation did not publish the "
            "reviewed 1920x1080 credit wrap owners "
            "(draws=%zu sprites=%zu first=%zu second=%zu third=%zu "
            "fourth=%zu fifth=%zu wrapped=%zu dropped=%zu/%zu pixels=%zu "
            "screen_alpha=%zu/%zu/%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.spriteDisplayDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_first_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_second_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_third_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_fourth_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_fifth_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_continue_credit_wrapped_1080),
            runtime.screenDrawDroppedCount,
            runtime.spriteDisplayDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudRescue &&
        (!input.headless ||
         !pc_validate_rescue_hud_960(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless rescue HUD validation did not publish the "
            "reviewed 960x540 maiden counter owner "
            "(draws=%zu text=%zu rescue=%zu dropped=%zu/%zu "
            "pixels=%zu/%zu screen_alpha=%zu/%zu/%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.textDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_rescue_counter_960),
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount,
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudRescue1080 &&
        (!input.headless ||
         !pc_validate_rescue_hud_1080(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless rescue HUD validation did not publish the "
            "reviewed 1920x1080 maiden counter owner "
            "(draws=%zu text=%zu sprites=%zu rescue=%zu dropped=%zu/%zu/%zu "
            "pixels=%zu/%zu screen_alpha=%zu/%zu/%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.textDrawCount,
            runtime.spriteDisplayDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_rescue_counter_1080),
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.spriteDisplayDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount,
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudPilotCounter &&
        (!input.headless ||
         !pc_validate_pilot_counter_hud_960(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless pilot-counter HUD validation did not publish the "
            "reviewed 960x540 level-11 pilot counter owner "
            "(draws=%zu text=%zu sprites=%zu pilot=%zu dropped=%zu/%zu/%zu "
            "pixels=%zu/%zu screen_alpha=%zu/%zu/%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.textDrawCount,
            runtime.spriteDisplayDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_pilot_counter_960),
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.spriteDisplayDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount,
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudPilotCounter1080 &&
        (!input.headless ||
         !pc_validate_pilot_counter_hud_1080(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless pilot-counter HUD validation did not publish the "
            "reviewed 1920x1080 level-11 pilot counter owner "
            "(draws=%zu text=%zu sprites=%zu pilot=%zu dropped=%zu/%zu/%zu "
            "pixels=%zu/%zu screen_alpha=%zu/%zu/%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.textDrawCount,
            runtime.spriteDisplayDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_pilot_counter_1080),
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.spriteDisplayDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount,
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudCountdown &&
        (!input.headless ||
         !pc_validate_countdown_hud_960(&runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless countdown HUD validation did not publish the "
            "reviewed 960x540 timer text owners "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudCountdown1080 &&
        (!input.headless ||
         !pc_validate_countdown_hud_1080(&runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless countdown HUD validation did not publish the "
            "reviewed 1920x1080 timer text owners "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudCountdownKill &&
        (!input.headless ||
         !pc_validate_countdown_kill_hud_960(&runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless countdown HUD validation did not publish the "
            "reviewed 960x540 kill-counter text owners "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudCountdownKill1080 &&
        (!input.headless ||
         !pc_validate_countdown_kill_hud_1080(&runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless countdown HUD validation did not publish the "
            "reviewed 1920x1080 kill-counter text owners "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudCountdownSuccess &&
        (!input.headless ||
         !pc_validate_countdown_success_hud_960(
             &runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless countdown HUD validation did not publish the "
            "reviewed 960x540 success text owner "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudCountdownSuccess1080 &&
        (!input.headless ||
         !pc_validate_countdown_success_hud_1080(
             &runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless countdown HUD validation did not publish the "
            "reviewed 1920x1080 success text owner "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudCountdownFail &&
        (!input.headless ||
         !pc_validate_countdown_fail_hud_960(&runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless countdown HUD validation did not publish the "
            "reviewed 960x540 fail text owner "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudCountdownFail1080 &&
        (!input.headless ||
         !pc_validate_countdown_fail_hud_1080(
             &runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless countdown HUD validation did not publish the "
            "reviewed 1920x1080 fail text owner "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudHangar &&
        (!input.headless ||
         !pc_validate_hangar_hud_960(&runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless Hangar HUD validation did not publish the "
            "reviewed 960x540 timer text owners "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudHangar1080 &&
        (!input.headless ||
         !pc_validate_hangar_hud_1080(&runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless Hangar HUD validation did not publish the "
            "reviewed 1920x1080 timer text owners "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudArena &&
        (!input.headless ||
         !pc_validate_arena_hud_960(&runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless Arena HUD validation did not publish the "
            "reviewed 960x540 winner text owner "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudArena1080 &&
        (!input.headless ||
         !pc_validate_arena_hud_1080(&runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless Arena HUD validation did not publish the "
            "reviewed 1920x1080 winner text owner "
            "(text=%zu dropped=%zu pixels=%zu level=%u)\n",
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudMini4 &&
        (!input.headless ||
         !pc_validate_mini4_hud(&runtime) ||
         runtime.psxTextureDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless Mini4 HUD validation did not publish the "
            "reviewed PSX digit owners "
            "(psx=%zu/%zu text=%zu/%zu pixels=%zu level=%u)\n",
            runtime.psxTextureDrawCount,
            runtime.psxTextureDrawDroppedCount,
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudMini41080 &&
        (!input.headless ||
         !pc_validate_mini4_hud_1080(&runtime, &framebuffer) ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless Mini4 HUD validation did not publish the "
            "reviewed 1920x1080 game_DrawBigNum digit owners "
            "(psx=%zu/%zu text=%zu/%zu pixels=%zu level=%u)\n",
            runtime.psxTextureDrawCount,
            runtime.psxTextureDrawDroppedCount,
            runtime.textDrawCount,
            runtime.textDrawDroppedCount,
            runtime.textDrawCompositePixelCount,
            (unsigned)(uint8_t)LevelSelect);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDamage &&
        (!input.headless ||
         framebuffer.width != 960 ||
         framebuffer.height != 540 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_damage_tracker_full_960) != 1 ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD damage validation did not publish the "
            "reviewed 960x540 player_DrawDamageTracker rectangle "
            "(draws=%zu damage=%zu broad=%zu dropped=%zu pixels=%zu)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker_full_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDamage1080 &&
        (!input.headless ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_damage_tracker_full_1080) != 1 ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD damage validation did not publish the "
            "reviewed 1920x1080 player_DrawDamageTracker rectangle "
            "(draws=%zu damage=%zu broad=%zu dropped=%zu pixels=%zu)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker_full_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDamageCompact &&
        (!input.headless ||
         framebuffer.width != 960 ||
         framebuffer.height != 540 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_damage_tracker_compact_960) != 1 ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD compact damage validation did not publish the "
            "reviewed 960x540 player_DrawDamageTracker rectangle "
            "(draws=%zu damage=%zu broad=%zu dropped=%zu pixels=%zu)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker_compact_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDamageCompact1080 &&
        (!input.headless ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_damage_tracker_compact_1080) != 1 ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD compact damage validation did not publish the "
            "reviewed 1920x1080 player_DrawDamageTracker rectangle "
            "(draws=%zu damage=%zu broad=%zu dropped=%zu pixels=%zu)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker_compact_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDamageP2 &&
        (!input.headless ||
         framebuffer.width != 960 ||
         framebuffer.height != 540 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_damage_tracker_p2_full_960) != 1 ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD P2 damage validation did not publish the "
            "reviewed 960x540 player_DrawDamageTracker rectangle "
            "(draws=%zu damage=%zu broad=%zu dropped=%zu pixels=%zu)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker_p2_full_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDamageP21080 &&
        (!input.headless ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_damage_tracker_p2_full_1080) != 1 ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD P2 damage validation did not publish the "
            "reviewed 1920x1080 player_DrawDamageTracker rectangle "
            "(draws=%zu damage=%zu broad=%zu dropped=%zu pixels=%zu)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker_p2_full_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDamageP2Compact &&
        (!input.headless ||
         framebuffer.width != 960 ||
         framebuffer.height != 540 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_damage_tracker_p2_compact_960) != 1 ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD P2 compact damage validation did not publish "
            "the reviewed 960x540 player_DrawDamageTracker rectangle "
            "(draws=%zu damage=%zu broad=%zu dropped=%zu pixels=%zu)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker_p2_compact_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDamageP2Compact1080 &&
        (!input.headless ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_damage_tracker_p2_compact_1080) != 1 ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD P2 compact damage validation did not publish "
            "the reviewed 1920x1080 player_DrawDamageTracker rectangle "
            "(draws=%zu damage=%zu broad=%zu dropped=%zu pixels=%zu)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker_p2_compact_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_damage_tracker),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudKadu &&
        (!input.headless ||
         framebuffer.width != 960 ||
         framebuffer.height != 540 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_kadu_race_p1_960) != 1 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_kadu_race_p2_frame10_960) != 1 ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD Kadu validation did not publish the "
            "reviewed 960x540 ai_Kadu race bars "
            "(draws=%zu p1=%zu p2=%zu broad=%zu dropped=%zu pixels=%zu "
            "level=%u)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_kadu_race_p1_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_kadu_race_p2_frame10_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_kadu_race_bar),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            (unsigned)(uint8_t)GameStruct.CurrentLevel);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudKadu1080 &&
        (!input.headless ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_kadu_race_p1_1080) != 1 ||
         pc_count_screen_draws_matching(
             &runtime,
             pc_screen_draw_is_kadu_race_p2_frame10_1080) != 1 ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD Kadu validation did not publish the "
            "reviewed 1920x1080 ai_Kadu race bars "
            "(draws=%zu p1=%zu p2=%zu broad=%zu dropped=%zu pixels=%zu "
            "level=%u)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_kadu_race_p1_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_kadu_race_p2_frame10_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_kadu_race_bar),
            runtime.screenDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            (unsigned)(uint8_t)GameStruct.CurrentLevel);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudOffscreen &&
        (!input.headless ||
         framebuffer.width != 960 ||
         framebuffer.height != 540 ||
         pc_count_screen_polys_matching(
             &runtime,
             pc_screen_poly_is_offscreen_arrow_frame1_960) != 1 ||
         runtime.screenPolyDroppedCount != 0)) {
        fprintf(
            stderr,
            "headless HUD offscreen validation did not publish the "
            "reviewed 960x540 playerOffScreenArrow screen poly "
            "(polys=%zu offscreen=%zu broad=%zu dropped=%zu pixels=%zu)\n",
            runtime.screenPolyDrawCount,
            pc_count_screen_polys_matching(
                &runtime,
                pc_screen_poly_is_offscreen_arrow_frame1_960),
            pc_count_screen_polys_matching(
                &runtime,
                pc_screen_poly_is_offscreen_arrow),
            runtime.screenPolyDroppedCount,
            runtime.screenPolyCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudOffscreen1080 &&
        (!input.headless ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080 ||
         pc_count_screen_polys_matching(
             &runtime,
             pc_screen_poly_is_offscreen_arrow_frame1_1080) != 1 ||
         runtime.screenPolyDroppedCount != 0)) {
        fprintf(
            stderr,
            "headless HUD offscreen validation did not publish the "
            "reviewed 1920x1080 playerOffScreenArrow screen poly "
            "(polys=%zu offscreen=%zu broad=%zu dropped=%zu pixels=%zu)\n",
            runtime.screenPolyDrawCount,
            pc_count_screen_polys_matching(
                &runtime,
                pc_screen_poly_is_offscreen_arrow_frame1_1080),
            pc_count_screen_polys_matching(
                &runtime,
                pc_screen_poly_is_offscreen_arrow),
            runtime.screenPolyDroppedCount,
            runtime.screenPolyCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudLifeTile &&
        (!input.headless ||
         !pc_validate_lifetile_hud_960(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD life-tile validation did not publish the "
            "reviewed 960x540 _AddLifeTile compact-overlay owners "
            "(draws=%zu tiles=%zu/%zu/%zu h=%zu/%zu/%zu f=%zu/%zu/%zu "
            "alpha=%zu/%zu/%zu/%zu dropped=%zu/%zu pixels=%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.playerHudTileDrawCount,
            runtime.playerHudTileDroppedCount,
            runtime.playerHudTileCompositePixelCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_left_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_bar_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_right_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_left_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_bar_960),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_right_960),
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount,
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudLifeTile1080 &&
        (!input.headless ||
         !pc_validate_lifetile_hud_1080(&runtime, &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0 ||
         runtime.textDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD life-tile validation did not publish the "
            "reviewed 1920x1080 _AddLifeTile compact-overlay owners "
            "(draws=%zu tiles=%zu/%zu/%zu item=%zu "
            "h=%zu/%zu/%zu f=%zu/%zu/%zu "
            "alpha=%zu/%zu/%zu/%zu dropped=%zu/%zu pixels=%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.playerHudTileDrawCount,
            runtime.playerHudTileDroppedCount,
            runtime.playerHudTileCompositePixelCount,
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_core_item_panel_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_left_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_bar_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_health_right_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_left_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_bar_1080),
            pc_count_screen_draws_matching(
                &runtime,
                pc_screen_draw_is_lifetile_force_right_1080),
            runtime.screenDrawTextureAlphaModulatedPixelCount,
            runtime.itemHudTextureAlphaModulatedPixelCount,
            runtime.creditHudTextureAlphaModulatedPixelCount,
            runtime.rescueHudTextureAlphaModulatedPixelCount,
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudProjectedLifeTile &&
        (!input.headless ||
         !pc_validate_projected_lifetile_hud_960(
             &runtime,
             &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD projected life-tile validation did not publish "
            "the reviewed 960x540 _AddLifeTile frame-2 owners "
            "(draws=%zu tiles=%zu/%zu/%zu expected=%d/%zu "
            "dropped=%zu/%zu pixels=%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.playerHudTileDrawCount,
            runtime.playerHudTileDroppedCount,
            runtime.playerHudTileCompositePixelCount,
            pc_runtime_has_expected_player_hud_tiles(
                &runtime,
                kProjectedLifeTile960,
                sizeof(kProjectedLifeTile960) /
                    sizeof(kProjectedLifeTile960[0])),
            sizeof(kProjectedLifeTile960) /
                sizeof(kProjectedLifeTile960[0]),
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudProjectedLifeTile1080 &&
        (!input.headless ||
         !pc_validate_projected_lifetile_hud_1080(
             &runtime,
             &framebuffer) ||
         runtime.screenDrawDroppedCount != 0 ||
         runtime.textDrawDroppedCount != 0 ||
         runtime.screenDrawCompositePixelCount == 0)) {
        fprintf(
            stderr,
            "headless HUD projected life-tile 1080 validation did not "
            "publish the reviewed 1920x1080 _AddLifeTile frame-2 owners "
            "(draws=%zu tiles=%zu/%zu/%zu expected=%d/%zu "
            "dropped=%zu/%zu pixels=%zu/%zu)\n",
            runtime.screenDrawCount,
            runtime.playerHudTileDrawCount,
            runtime.playerHudTileDroppedCount,
            runtime.playerHudTileCompositePixelCount,
            pc_runtime_has_expected_player_hud_tiles(
                &runtime,
                kProjectedLifeTile1080,
                sizeof(kProjectedLifeTile1080) /
                    sizeof(kProjectedLifeTile1080[0])),
            sizeof(kProjectedLifeTile1080) /
                sizeof(kProjectedLifeTile1080[0]),
            runtime.screenDrawDroppedCount,
            runtime.textDrawDroppedCount,
            runtime.screenDrawCompositePixelCount,
            runtime.textDrawCompositePixelCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDebugLabels &&
        (!input.headless ||
         !pc_validate_debug_labels_hud_960(&runtime, &framebuffer))) {
        fprintf(
            stderr,
            "headless HUD debug-label validation did not publish the "
            "reviewed DebugLevel 2 scr_debugPrintfXYZ owner "
            "(draw3d=%zu/%zu)\n",
            runtime.draw3dTextDrawCount,
            runtime.draw3dTextDroppedCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDebugLabels1080 &&
        (!input.headless ||
         !pc_validate_debug_labels_hud_1080(&runtime, &framebuffer))) {
        fprintf(
            stderr,
            "headless HUD debug-label validation did not publish the "
            "reviewed 1920x1080 DebugLevel 2 scr_debugPrintfXYZ owner "
            "(draw3d=%zu/%zu)\n",
            runtime.draw3dTextDrawCount,
            runtime.draw3dTextDroppedCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDebugLabels3 &&
        (!input.headless ||
         framebuffer.width != 960 ||
         framebuffer.height != 540 ||
         !pc_validate_debug_labels3_hud(&runtime))) {
        fprintf(
            stderr,
            "headless HUD DebugLevel 3 validation did not publish the "
            "reviewed frame-2 enemy scr_debugPrintfXYZ owners "
            "(draw3d=%zu/%zu)\n",
            runtime.draw3dTextDrawCount,
            runtime.draw3dTextDroppedCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudDebugLabels31080 &&
        (!input.headless ||
         framebuffer.width != 1920 ||
         framebuffer.height != 1080 ||
         !pc_validate_debug_labels3_hud(&runtime))) {
        fprintf(
            stderr,
            "headless HUD DebugLevel 3 1080 validation did not publish "
            "the reviewed frame-2 enemy scr_debugPrintfXYZ owners "
            "(draw3d=%zu/%zu)\n",
            runtime.draw3dTextDrawCount,
            runtime.draw3dTextDroppedCount);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateHudOwnerCoverage &&
        (!input.headless ||
         pc_count_screen_draws_without_recovered_hud_owner(&runtime) != 0 ||
         pc_count_screen_polys_without_recovered_hud_owner(
             &runtime,
             input.validateHudOffscreen ||
                 input.validateHudOffscreen1080) != 0 ||
         pc_count_text_draws_without_recovered_hud_owner(&runtime) != 0 ||
         pc_count_draw3d_text_without_recovered_hud_owner(&runtime) != 0 ||
         pc_count_sprite_displays_without_recovered_hud_owner(&runtime) !=
             0 ||
         pc_count_psx_texture_draws_without_recovered_hud_owner(&runtime) !=
             0)) {
        fprintf(
            stderr,
            "headless HUD owner coverage validation found HUD "
            "submissions outside recovered PDB owner buckets "
            "(screen_draws=%zu unknown_screen_draws=%zu "
            "screen_poly=%zu unknown_screen_poly=%zu "
            "text=%zu unknown_text=%zu "
            "draw3d_text=%zu unknown_draw3d_text=%zu "
            "sprite_display=%zu unknown_sprite_display=%zu "
            "psx_texture=%zu unknown_psx_texture=%zu)\n",
            runtime.screenDrawCount,
            pc_count_screen_draws_without_recovered_hud_owner(&runtime),
            runtime.screenPolyDrawCount,
            pc_count_screen_polys_without_recovered_hud_owner(
                &runtime,
                input.validateHudOffscreen ||
                    input.validateHudOffscreen1080),
            runtime.textDrawCount,
            pc_count_text_draws_without_recovered_hud_owner(&runtime),
            runtime.draw3dTextDrawCount,
            pc_count_draw3d_text_without_recovered_hud_owner(&runtime),
            runtime.spriteDisplayDrawCount,
            pc_count_sprite_displays_without_recovered_hud_owner(&runtime),
            runtime.psxTextureDrawCount,
            pc_count_psx_texture_draws_without_recovered_hud_owner(
                &runtime));
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateTeleport &&
        (!input.headless ||
         tflag != 0 ||
         tele != -1 ||
         runtime.physics->pos.vx !=
             initial_position.vx + 64.0f)) {
        fprintf(
            stderr,
            "headless teleport validation did not apply the deferred "
            "offset (flag=%d target=%d x=%.1f/%.1f)\n",
            tflag,
            tele,
            runtime.physics->pos.vx,
            initial_position.vx + 64.0f);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (!runtime.topView && frame_count > 0) {
        FVECTOR target = {
            runtime.targetX, runtime.targetY, runtime.targetZ
        };

        target_clipped = jpb_ProjectPcToViewport(
            &runtime.environment.matrix,
            &target,
            (float)framebuffer.width,
            (float)framebuffer.height,
            &target_screen);
        fApplyMatrixFV(
            &runtime.environment.matrix, &target, &target_view);
        target_view.vx += (float)runtime.environment.matrix.t[0];
        target_view.vy += (float)runtime.environment.matrix.t[1];
        target_view.vz += (float)runtime.environment.matrix.t[2];
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.validateCameraFollow &&
        (!input.headless ||
         (runtime.camera.viewType & UINT32_C(0x0100)) == 0 ||
         runtime.authoredCameraFrameCount == 0 ||
         target_clipped != 0 ||
         target_screen.vx < 0.0f ||
         target_screen.vx >= (float)framebuffer.width ||
         target_screen.vy < 0.0f ||
         target_screen.vy >= (float)framebuffer.height)) {
        fprintf(
            stderr,
            "headless authored-camera validation did not retain the "
            "recovered slide mode and follow the player "
            "(view=0x%08x authored_gameplay=%u total_frames=%d "
            "target=%d/%.1f,%.1f)\n",
            (unsigned)runtime.camera.viewType,
            (unsigned)runtime.authoredCameraFrameCount,
            frame_count,
            target_clipped,
            target_screen.vx,
            target_screen.vy);
        result = JPB_GAME_RUNTIME_RENDER_FAILED;
    }
    if (camera_diagnostics) {
        pc_print_camera_collision_diagnostics(&runtime);
    }
    printf(
        "menu_state=(active=%d,mode=%u,stack=%u,select=%u,"
        "level=%d,level_box=%d/%u/%u,countdown=%u,open=%u,"
        "preview=%u/fade=%d,selector_offset=%d,"
        "pplayers=%u/%u,subplayers=%u/%u,"
        "current_models=%d/%d/%d/%d,players=%d,models=%d/%d,"
        "framebuffer=%dx%d,scale=%.3f/%.3f,frontRGBoff=%u)\n",
        title_active,
        (unsigned)menuVars.menuMode[menuVars.menuModeSP & 7u],
        (unsigned)menuVars.menuModeSP,
        (unsigned)menuVars.mmSelect1[menuVars.menuModeSP & 7u],
        (int)(uint8_t)LevelSelect,
        (int)menuVars.levelSelectBoxTop,
        (unsigned)menuVars.levelSelectBoxWidth,
        (unsigned)menuVars.levelSelectBoxHeight,
        (unsigned)menuVars.levelSelectBoxCountdown,
        (unsigned)menuVars.levelSelectBoxOpen,
        (unsigned)menuVars.levelSelectPreviewLevel,
        (int)menuVars.levelSelectPreviewFade,
        (int)menuVars.levelSelectSelectorOffset,
        (unsigned)menuVars.pplayers[0],
        (unsigned)menuVars.pplayers[1],
        (unsigned)menuVars.subplayers[0],
        (unsigned)menuVars.subplayers[1],
        newMenu_currentModelSelectBaseP1,
        newMenu_currentModelSelectNGPP1,
        newMenu_currentModelSelectBaseP2,
        newMenu_currentModelSelectNGPP2,
        (int)GameStruct.NumPlayers,
        (int)GameStruct.ModelSelect[0],
        (int)GameStruct.ModelSelect[1],
        framebuffer.width,
        framebuffer.height,
        scaleAdjustment,
        scaleAdjustmentMM,
        (unsigned)frontRGBoff);
    printf(
        "game_state=(level=%u,mode=%d,players=%d,versus=%d)\n",
        (unsigned)(uint8_t)GameStruct.CurrentLevel,
        (int)GameStruct.gameMode,
        (int)GameStruct.NumPlayers,
        (int)GameStruct.versusModeFlag);
    printf(
        "spawn_view=(model=%d,input=%s,"
        "player=%.1f/%.1f/%.1f/facing:%d,"
        "target=%.1f/%.1f/%.1f,"
        "camera=%d/%d/%d/angle:%d/%d/%d,"
        "dolly=%d,authored=%u,collision=%.3f)\n",
        (int)GameStruct.ModelSelect[0],
        player1InputType == 0 ? "keyboard" : "xinput",
        runtime.physics->pos.vx,
        runtime.physics->pos.vy,
        runtime.physics->pos.vz,
        physics_gGetFacing(&runtime.player->playerRoot),
        runtime.targetX,
        runtime.targetY,
        runtime.targetZ,
        runtime.camera.focus.vx,
        runtime.camera.focus.vy,
        runtime.camera.focus.vz,
        runtime.camera.angle.vx,
        runtime.camera.angle.vy,
        runtime.camera.angle.vz,
        runtime.authoredCameraDolly,
        (unsigned)runtime.authoredCameraFrameCount,
        runtime.cameraCollisionFraction);
    printf(
        "control_context=(game_flags=%08x,orbit=%.3f,"
        "handoff_guard=%02x)\n",
        (unsigned)GameStruct.GameState,
        runtime.orbitDistance,
        (unsigned)input.gameplayHandoffReleaseMask);
    printf(
        "presentation=(frames=%u,handoffs=%u,hidden=%d,scripted=%d)\n",
        presentation_frame_count,
        gameplay_handoff_count,
        input.hiddenWindow,
        input.scriptedInput);
    printf(
        "audio_handoff=(active=%d,generations=%u,output=%d)\n",
        audio != NULL,
        audio_generation_count,
        audio_output_enabled);
    printf(
        "player_two_audio=(requested=%s,resolved=%d,path=%s)\n",
        validate_player_two_sound != NULL
            ? validate_player_two_sound
            : "none",
        player_two_audio_validated,
        validated_player_two_sound_path[0] != '\0'
            ? validated_player_two_sound_path
            : "none");
    printf(
        "persistence_handoff=(enabled=%d,no_game=%d,game_writes=%u)\n",
        input.persistenceEnabled,
        input.noValidGameSave,
        input.gameSaveWriteCount);
    printf(
        "player_selection=(models=%d/%d)\n",
        (int)GameStruct.ModelSelect[0],
        (int)GameStruct.ModelSelect[1]);
    printf(
        "player_projectiles=(p1=%zu/type:%d/flags:%08x/"
        "start:%d,%d,%d/target:%d,%d,%d;"
        "p2=%zu/type:%d/flags:%08x/"
        "start:%d,%d,%d/target:%d,%d,%d)\n",
        runtime.playerProjectileLaunchCount[0],
        (int)runtime.lastPlayerProjectileType[0],
        (unsigned)runtime.lastPlayerProjectileFlags[0],
        runtime.lastPlayerProjectileStart[0].vx,
        runtime.lastPlayerProjectileStart[0].vy,
        runtime.lastPlayerProjectileStart[0].vz,
        runtime.lastPlayerProjectileTarget[0].vx,
        runtime.lastPlayerProjectileTarget[0].vy,
        runtime.lastPlayerProjectileTarget[0].vz,
        runtime.playerProjectileLaunchCount[1],
        (int)runtime.lastPlayerProjectileType[1],
        (unsigned)runtime.lastPlayerProjectileFlags[1],
        runtime.lastPlayerProjectileStart[1].vx,
        runtime.lastPlayerProjectileStart[1].vy,
        runtime.lastPlayerProjectileStart[1].vz,
        runtime.lastPlayerProjectileTarget[1].vx,
        runtime.lastPlayerProjectileTarget[1].vy,
        runtime.lastPlayerProjectileTarget[1].vz);
    printf(
        "player_weapon=(model=%d,saber=%d,color=%08x,"
        "outer=%zu,trail=%zu,core=%zu,attached=%zu,unmatched=%zu,"
        "width=%d..%d,length=%.1f..%.1f)\n",
        saber_diagnostics.playerModel,
        saber_diagnostics.expectsSaber,
        (unsigned)saber_diagnostics.outerColor,
        saber_diagnostics.outerDrawCount,
        saber_diagnostics.trailDrawCount,
        saber_diagnostics.matchedCoreDrawCount,
        saber_diagnostics.matchedAttachmentDrawCount,
        saber_diagnostics.unmatchedAttachmentDrawCount,
        saber_diagnostics.minimumOuterWidth,
        saber_diagnostics.maximumOuterWidth,
        saber_diagnostics.minimumBladeLength,
        saber_diagnostics.maximumBladeLength);
    printf(
        "player_weapon_action=(model=%d,saber=%d,frames=%zu,valid=%zu,"
        "invalid=%zu,attached=%zu,trails=%zu,width=%d..%d,"
        "length=%.1f..%.1f)\n",
        saber_action_diagnostics.playerModel,
        saber_action_diagnostics.expectsSaber,
        saber_action_diagnostics.actionFrames,
        saber_action_diagnostics.validActionFrames,
        saber_action_diagnostics.invalidActionFrames,
        saber_action_diagnostics.attachedBladeDrawCount,
        saber_action_diagnostics.trailDrawCount,
        saber_action_diagnostics.minimumOuterWidth,
        saber_action_diagnostics.maximumOuterWidth,
        saber_action_diagnostics.minimumBladeLength,
        saber_action_diagnostics.maximumBladeLength);
    printf(
        "second_player_weapon=(model=%d,saber=%d,color=%08x,"
        "outer=%zu,trail=%zu,core=%zu,attached=%zu,unmatched=%zu,"
        "width=%d..%d,length=%.1f..%.1f)\n",
        second_saber_diagnostics.playerModel,
        second_saber_diagnostics.expectsSaber,
        (unsigned)second_saber_diagnostics.outerColor,
        second_saber_diagnostics.outerDrawCount,
        second_saber_diagnostics.trailDrawCount,
        second_saber_diagnostics.matchedCoreDrawCount,
        second_saber_diagnostics.matchedAttachmentDrawCount,
        second_saber_diagnostics.unmatchedAttachmentDrawCount,
        second_saber_diagnostics.minimumOuterWidth,
        second_saber_diagnostics.maximumOuterWidth,
        second_saber_diagnostics.minimumBladeLength,
        second_saber_diagnostics.maximumBladeLength);
    printf(
        "second_player_weapon_action=(model=%d,saber=%d,frames=%zu,"
        "valid=%zu,invalid=%zu,attached=%zu,trails=%zu,width=%d..%d,"
        "length=%.1f..%.1f)\n",
        second_saber_action_diagnostics.playerModel,
        second_saber_action_diagnostics.expectsSaber,
        second_saber_action_diagnostics.actionFrames,
        second_saber_action_diagnostics.validActionFrames,
        second_saber_action_diagnostics.invalidActionFrames,
        second_saber_action_diagnostics.attachedBladeDrawCount,
        second_saber_action_diagnostics.trailDrawCount,
        second_saber_action_diagnostics.minimumOuterWidth,
        second_saber_action_diagnostics.maximumOuterWidth,
        second_saber_action_diagnostics.minimumBladeLength,
        second_saber_action_diagnostics.maximumBladeLength);
    printf(
        "second_player=(ready=%d,triangles=%zu,pixels=%zu)\n",
        jpb_GameRuntimeSecondPlayerReady(&runtime),
        jpb_GameRuntimeSecondPlayerRenderedTriangles(&runtime),
        jpb_GameRuntimeSecondPlayerRenderedPixels(&runtime));
    printf(
        "control_sources=(p1=%s,p2=%s,last=%s)\n",
        player1InputType == 0 ? "keyboard" : "xinput",
        player2InputType == 0 ? "keyboard" : "xinput",
        lastUsedInputType == 0 ? "keyboard" : "xinput");
    printf(
        "control_schemes=(p1=%s,p2=%s)\n",
        pc_control_scheme_name(OptionStruct.ControllerConfig[0]),
        pc_control_scheme_name(OptionStruct.ControllerConfig[1]));
    resource_second_player =
        runtime.secondPlayerState != NULL
            ? runtime.inactivePlayer
            : NULL;
    printf(
        "player_resources=(p1=energy:%d/%d,force:%d/%d,items:%d,"
        "force_owner:%s;p2=energy:%d/%d,force:%d/%d,items:%d,"
        "force_owner:%s)\n",
        game_gGetEnergy(runtime.player->playernum),
        game_gGetMaxEnergy(runtime.player->playernum),
        game_gGetForce(runtime.player->playernum),
        game_gGetMaxForce(runtime.player->playernum),
        game_gGetItemCount(runtime.player->playernum),
        pc_force_callback_name(runtime.player->pForceCallBack),
        resource_second_player != NULL
            ? game_gGetEnergy(resource_second_player->playernum)
            : 0,
        resource_second_player != NULL
            ? game_gGetMaxEnergy(resource_second_player->playernum)
            : 0,
        resource_second_player != NULL
            ? game_gGetForce(resource_second_player->playernum)
            : 0,
        resource_second_player != NULL
            ? game_gGetMaxForce(resource_second_player->playernum)
            : 0,
        resource_second_player != NULL
            ? game_gGetItemCount(resource_second_player->playernum)
            : 0,
        pc_force_callback_name(
            resource_second_player != NULL
                ? resource_second_player->pForceCallBack
                : NULL));
    printf(
        "control_edges=(p1=pressed_frames:%u,held_frames:%u,"
        "release_events:%u,pressed_bits:%08x,held_bits:%08x,"
        "released_bits:%08x,lock_toggles:%u,lock:%u,motion:%d,"
        "locomotion:%u/%d,direction:%u,axis:%.5f/%.5f,"
        "camera:%d,desired:%d,facing:%d;"
        "p2=pressed_frames:%u,held_frames:%u,release_events:%u,"
        "pressed_bits:%08x,held_bits:%08x,released_bits:%08x,"
        "lock_toggles:%u,lock:%u,motion:%d,locomotion:%u/%d,"
        "direction:%u,axis:%.5f/%.5f,camera:%d,desired:%d,"
        "facing:%d)\n",
        (unsigned)runtime.controlPressedFrameCount[0],
        (unsigned)runtime.controlHeldFrameCount[0],
        (unsigned)runtime.controlReleaseEventCount[0],
        (unsigned)runtime.controlObservedPressedBits[0],
        (unsigned)runtime.controlObservedHeldBits[0],
        (unsigned)runtime.controlObservedReleasedBits[0],
        (unsigned)runtime.controlLockToggleCount[0],
        (unsigned)runtime.controlLockActive[0],
        (int)runtime.lastControlMotion[0],
        (unsigned)runtime.controlLocomotionFrameCount[0],
        (int)runtime.lastControlLocomotionMotion[0],
        (unsigned)runtime.controlDirectionalFrameCount[0],
        runtime.lastControlAxisX[0],
        runtime.lastControlAxisY[0],
        (int)runtime.lastControlCameraAngle[0],
        (int)runtime.lastControlDesiredFacing[0],
        (int)runtime.lastControlFacing[0],
        (unsigned)runtime.controlPressedFrameCount[1],
        (unsigned)runtime.controlHeldFrameCount[1],
        (unsigned)runtime.controlReleaseEventCount[1],
        (unsigned)runtime.controlObservedPressedBits[1],
        (unsigned)runtime.controlObservedHeldBits[1],
        (unsigned)runtime.controlObservedReleasedBits[1],
        (unsigned)runtime.controlLockToggleCount[1],
        (unsigned)runtime.controlLockActive[1],
        (int)runtime.lastControlMotion[1],
        (unsigned)runtime.controlLocomotionFrameCount[1],
        (int)runtime.lastControlLocomotionMotion[1],
        (unsigned)runtime.controlDirectionalFrameCount[1],
        runtime.lastControlAxisX[1],
        runtime.lastControlAxisY[1],
        (int)runtime.lastControlCameraAngle[1],
        (int)runtime.lastControlDesiredFacing[1],
        (int)runtime.lastControlFacing[1]);
    printf(
        "frames=%d mode=%s strips=%zu vertices=%zu streets_cull=%d "
        "triangles=%zu lines=%zu pixels=%zu streets_culled=%zu "
        "transparent=(triangles=%zu,pixels=%zu,glass=%zu/%zu) "
        "model_triangles=%zu model_lines=%zu model_pixels=%zu "
        "text=%zu/%zu/%zu/%zu/%d/%dx%d "
        "draw3d_text=%zu/%zu "
        "sprite_display=%zu/%zu "
        "psx_texture=%zu/%zu "
        "player_hud=%zu/%zu/%zu "
        "screen_alpha=%zu/%zu/%zu/%zu "
        "saber_glow=%zu/%zu/%zu cylinders=%zu "
        "screen_poly=%zu/%zu/%zu "
        "water_poly=%zu/%zu "
        "powerups=%zu/%zu/%zu "
        "camera=(dolly=%d,authored=%u,collision=%.4f/%u) "
        "camera_world=(%d,%d,%d) "
        "target=%s(%.1f,%.1f,%.3f) "
        "view=(%.1f,%.1f,%.1f) "
        "camera_state=(type=%d,view=0x%08x,"
        "angles=%d,%d->%d,%d,"
        "focus=%d,%d,%d->%d,%d,%d) "
        "motion=%d authored=%d decoded=%u tween_frames=%u "
        "model_nodes=%zu posed=%d "
        "locomotion_frames=%u last_locomotion_motion=%d "
        "damage_motion_frames=%u last_damage_motion=%d "
        "running_attack_frames=%u last_running_attack_motion=%d "
        "passive_motion_reports=%u "
        "hot_frames=%u hot_peak=%u "
        "hot_target=(distance=%u,radius=%d,nodes=%d/%d) "
        "hot_nodes=(12:%u/%d/%d,17:%u/%d/%d,"
        "18:%u/%d/%d,19:%u/%d/%d) "
        "anim_frame=%d root=(%d,%d,%d) "
        "physics=(%.1f,%.1f,%.1f,facing=%d) "
        "movement=(%.1f,%.1f,%.1f) "
        "current=(%.1f,%.1f,%.1f) constant=(%.1f,%.1f,%.1f) "
        "move_mode=%d flags=%08x pad=(%08x,%08x) lock=%u "
        "player_flags=%08x lock_target=(active=%d,id=%d) "
        "motion0_flags=%08x "
        "motion2=(seq=%u,lock=%u,funct=%d,flags=%08x) "
        "motion15=(flags=%08x,attack=%08x,damage=%u) "
        "motion21=(flags=%08x,attack=%08x,damage=%u) "
        "player_ai=(attached=%d,transitions=%u/%u,last=%d/%d) "
        "enemy=(actors=%zu/%zu/%zu,"
        "classes=%zu/%zu/%zu/%zu/%zu/%zu,"
        "id=%d,placement=%d,"
        "motion=%d/anim=%d,"
        "damage_motion=%d,flags=%08x,decoded=%u,scheduler=%u,"
        "authored_ai=%u,boundary=%u/0x%03x,"
        "nodes=%zu,posed=%d,triangles=%zu,pixels=%zu,"
        "physics=%.1f,%.1f,%.1f,hits=%u,pending=%u,"
        "damage_processed=%u,energy=%d/%d,"
        "reaction=(motion=%d,frames=%u,recoil=%u/%.1f))\n",
        frame_count,
        runtime.topView ? "top" : "game-camera",
        runtime.scene.strips,
        runtime.scene.vertices,
        runtime.scene.streetsCullMapReady,
        stats.triangles,
        stats.lines,
        stats.pixels,
        stats.levelCulledTriangles,
        stats.levelTransparentTriangles,
        stats.levelTransparentPixels,
        stats.levelGlassTriangles,
        stats.levelGlassPixels,
        stats.modelTriangles,
        stats.modelLines,
        stats.modelPixels,
        runtime.textDrawCount,
        runtime.textTrueTypeDrawCount,
        runtime.textFallbackDrawCount,
        runtime.textDrawCompositePixelCount,
        runtime.maximumTextPointSize,
        runtime.maximumTextMeasuredWidth,
        runtime.maximumTextMeasuredHeight,
        runtime.draw3dTextDrawCount,
        runtime.draw3dTextDroppedCount,
        runtime.spriteDisplayDrawCount,
        runtime.spriteDisplayDroppedCount,
        runtime.psxTextureDrawCount,
        runtime.psxTextureDrawDroppedCount,
        runtime.playerHudTileDrawCount,
        runtime.playerHudTileDroppedCount,
        runtime.playerHudTileCompositePixelCount,
        runtime.screenDrawTextureAlphaModulatedPixelCount,
        runtime.itemHudTextureAlphaModulatedPixelCount,
        runtime.creditHudTextureAlphaModulatedPixelCount,
        runtime.rescueHudTextureAlphaModulatedPixelCount,
        runtime.glowDrawCount,
        runtime.glowDrawDroppedCount,
        runtime.glowDrawCompositePixelCount,
        runtime.cylinderDrawCount,
        runtime.screenPolyDrawCount,
        runtime.screenPolyDroppedCount,
        runtime.screenPolyCompositePixelCount,
        runtime.waterPolyDrawCount,
        runtime.waterPolyCompositePixelCount,
        runtime.powerupCount,
        runtime.powerupDrawCount,
        runtime.powerupCollectedCount,
        (int)runtime.authoredCameraDolly,
        (unsigned)runtime.authoredCameraFrameCount,
        (double)runtime.cameraCollisionFraction,
        (unsigned)runtime.cameraCollisionFrameCount,
        -(int)runtime.environment.pos.vx,
        -(int)runtime.environment.pos.vy,
        -(int)runtime.environment.pos.vz,
        target_clipped ? "clipped" : "visible",
        target_screen.vx,
        target_screen.vy,
        target_screen.vz,
        target_view.vx,
        target_view.vy,
        target_view.vz,
        camera_GetCurrentCameraType(),
        (unsigned)runtime.camera.viewType,
        runtime.environment.angle.vx,
        runtime.environment.angle.vy,
        runtime.camera.angleDest.vx,
        runtime.camera.angleDest.vy,
        (int)runtime.camera.focus.vx,
        (int)runtime.camera.focus.vy,
        (int)runtime.camera.focus.vz,
        (int)runtime.camera.focusDest.vx,
        (int)runtime.camera.focusDest.vy,
        (int)runtime.camera.focusDest.vz,
        (int)runtime.player->currentMotion,
        runtime.authoredMotionReady,
        (unsigned)runtime.decodedFrameCount,
        (unsigned)runtime.authoredTweenFrameCount,
        runtime.bmdView.node_count,
        runtime.authoredPoseReady,
        (unsigned)runtime.authoredLocomotionMotionFrameCount,
        (int)runtime.lastAuthoredLocomotionMotion,
        (unsigned)runtime.authoredDamageMotionFrameCount,
        (int)runtime.lastAuthoredDamageMotion,
        (unsigned)runtime.authoredRunningAttackFrameCount,
        (int)runtime.lastAuthoredRunningAttackMotion,
        (unsigned)runtime.passiveMotionReportFrameCount,
        (unsigned)runtime.authoredHotFrameCount,
        (unsigned)runtime.authoredHotNodePeak,
        (unsigned)runtime.closestHotTargetNodeDistance,
        runtime.closestHotTargetCollisionRadius,
        (int)runtime.closestHotNodeId,
        (int)runtime.closestTargetNodeId,
        (unsigned)
            runtime.closestHotTargetDistanceByNode[12],
        runtime.hotTargetCollisionRadiusByNode[12],
        (int)runtime.closestTargetNodeByHotNode[12],
        (unsigned)
            runtime.closestHotTargetDistanceByNode[17],
        runtime.hotTargetCollisionRadiusByNode[17],
        (int)runtime.closestTargetNodeByHotNode[17],
        (unsigned)
            runtime.closestHotTargetDistanceByNode[18],
        runtime.hotTargetCollisionRadiusByNode[18],
        (int)runtime.closestTargetNodeByHotNode[18],
        (unsigned)
            runtime.closestHotTargetDistanceByNode[19],
        runtime.hotTargetCollisionRadiusByNode[19],
        (int)runtime.closestTargetNodeByHotNode[19],
        runtime.animation != NULL
            ? runtime.animation->animFrameIndex /
                  JPB_FIXED_ONE
            : 0,
        runtime.authoredFrameReady
            ? runtime.animation->pCurrentAnimFrame
                  ->v3RootTranslation.vx
            : 0,
        runtime.authoredFrameReady
            ? runtime.animation->pCurrentAnimFrame
                  ->v3RootTranslation.vy
            : 0,
        runtime.authoredFrameReady
            ? runtime.animation->pCurrentAnimFrame
                  ->v3RootTranslation.vz
            : 0,
        runtime.physics->pos.vx,
        runtime.physics->pos.vy,
        runtime.physics->pos.vz,
        runtime.physics->angle.vy,
        runtime.physics->mov.vx,
        runtime.physics->mov.vy,
        runtime.physics->mov.vz,
        runtime.physics->currentmov.vx,
        runtime.physics->currentmov.vy,
        runtime.physics->currentmov.vz,
        runtime.physics->constmov.vx,
        runtime.physics->constmov.vy,
        runtime.physics->constmov.vz,
        (int)runtime.physics->movemode,
        (unsigned)runtime.physics->flags,
        (unsigned)runtime.player->playerPad.cpad[0],
        (unsigned)runtime.player->playerPad.cpad[1],
        (unsigned)runtime.animation->Lock,
        (unsigned)runtime.player->pFlags,
        runtime.player->locked != NULL,
        runtime.player->locked != NULL
            ? (int)runtime.player->locked->playerID
            : -1,
        (unsigned)runtime.player->paMotions[0].motionFlags,
        (unsigned)runtime.player->paMotions[2].Seq,
        (unsigned)runtime.player->paMotions[2].Lock,
        (int)runtime.player->paMotions[2].FunctPtr,
        (unsigned)runtime.player->paMotions[2].motionFlags,
        (unsigned)runtime.player->paMotions[15].motionFlags,
        (unsigned)runtime.player->paMotions[15].attackFlags,
        (unsigned)runtime.player->paMotions[15].Damage,
        (unsigned)runtime.player->paMotions[21].motionFlags,
        (unsigned)runtime.player->paMotions[21].attackFlags,
        (unsigned)runtime.player->paMotions[21].Damage,
        runtime.player->pEnemy != NULL,
        (unsigned)runtime.playerAuthoredAiAttachCount,
        (unsigned)runtime.playerAuthoredAiReleaseCount,
        (int)runtime.lastPlayerAuthoredAiEnemyId,
        (int)runtime.lastPlayerAuthoredAiOwnerType,
        runtime.enemyActorCount,
        runtime.enemyActorPeakCount,
        runtime.enemySpawnCount,
        runtime.enemyLoadedClassCount,
        runtime.enemyPlacedClassCount,
        runtime.enemyActiveClassCount,
        runtime.enemyActiveClassPeakCount,
        runtime.enemyActivatedClassCount,
        runtime.enemyRenderedClassCount,
        runtime.enemyPlayer != NULL
            ? (int)runtime.enemyPlayer->playerID
            : -1,
        runtime.enemy != NULL ? runtime.enemy->enemyNum : -1,
        runtime.enemyPlayer != NULL
            ? (int)runtime.enemyPlayer->currentMotion
            : -1,
        (int)runtime.enemyAnimationMotion,
        (int)runtime.lastEnemyDamageMotion,
        runtime.enemyPlayer != NULL
            ? (unsigned)runtime.enemyPlayer->pFlags
            : 0u,
        (unsigned)runtime.enemyDecodedFrameCount,
        (unsigned)runtime.enemyKungfuSchedulerFrameCount,
        (unsigned)runtime.enemyAuthoredOpcodeFrameCount,
        (unsigned)runtime.enemyOpcodeBoundaryFrameCount,
        (unsigned)runtime.enemyOpcodeBoundary,
        runtime.enemyBmdView.node_count,
        runtime.enemyAuthoredPoseReady,
        runtime.enemyRenderedTriangles,
        runtime.enemyRenderedPixels,
        runtime.enemyPhysics != NULL
            ? runtime.enemyPhysics->pos.vx
            : 0.0f,
        runtime.enemyPhysics != NULL
            ? runtime.enemyPhysics->pos.vy
            : 0.0f,
        runtime.enemyPhysics != NULL
            ? runtime.enemyPhysics->pos.vz
            : 0.0f,
        (unsigned)runtime.combatHitCount,
        runtime.enemyPlayer != NULL
            ? (unsigned)runtime.enemyPlayer->hitNumber
            : 0u,
        (unsigned)runtime.enemyDamageProcessedCount,
        (int)runtime.enemyMinimumEnergy,
        (int)runtime.enemyInitialEnergy,
        (int)runtime.lastEnemyReactionMotion,
        (unsigned)runtime.enemyReactionMotionFrameCount,
        (unsigned)runtime.enemyRecoilReactionCount,
        runtime.lastEnemyRecoil);
    pc_print_screen_draw_trace(&runtime);
    pc_print_screen_poly_trace(&runtime);
    pc_print_text_draw_trace(&runtime);
    pc_print_draw3d_text_trace(&runtime);
    pc_print_sprite_display_trace(&runtime);
    pc_print_psx_texture_draw_trace(&runtime);
cleanup:
    if (jpb_sprite_list_recovery_count != 0) {
        fprintf(
            stderr,
            "sprite list selector recoveries=%u last=%d\n",
            (unsigned)jpb_sprite_list_recovery_count,
            (int)jpb_sprite_last_invalid_selector);
    }
    if (result == JPB_GAME_RUNTIME_OK &&
        input.persistenceEnabled) {
        JPBSaveResult save_result =
            jpb_SaveOptionsWriteFile(input.optionsPath);

        if (save_result != JPB_SAVE_OK) {
            fprintf(
                stderr,
                "options save failed (%s): %s\n",
                pc_save_result_name(save_result),
                input.optionsPath);
        }
    }
    jpb_PCAudioDestroy(audio);
    jpb_GameRuntimeShutdown(&runtime);
    pc_release_menu_textures(menu_texture_cache);
    free(menu_texture_cache);
#if defined(JPB_PC_HAS_UFBX)
    if (fbx_level_loaded) {
        jpb_PCFreeFbxLevel(&fbx_level);
    }
#endif
    jpb_InputSetProvider(NULL, NULL);
    jpb_InputSetRumbleProvider(NULL, NULL);
    jpb_BrainutlSetCheatChordProvider(NULL, NULL);
    jpb_MenuSetPlatformHooks(NULL, NULL);
    jpb_PCXInputShutdown(&input.xinput);
    free(title_pixels);
    free(pixels);
    exit_code = result == JPB_GAME_RUNTIME_OK ? 0 : 5;
    jpb_PCLogStop(exit_code);
    return exit_code;
}
