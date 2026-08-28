/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\player.c.
 *
 * The reviewed subset establishes the exact player pool, Pad embedding,
 * scene-component construction, and allocation/lookup lifecycle needed by
 * the controller and motion owners.
 *
 * Provenance:
 *   direct     - names/signatures/locals and Pad, objectRoot, playerSettings,
 *                and playerObject layouts from the exact PDB; gaPlayerData
 *                from the linked global stream.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - strides, field offsets, initialization ranges, signed and
 *                unsigned bounds, first-free scan, world pointer stores, and
 *                fatal exhaustion checked at exact RVAs. The complete
 *                attack eligibility, versus-mode, actor relationship,
 *                range, hot-node, and successful-contact paths in
 *                player_DoCollisions are checked at 0xE7220..0xE73ED;
 *                the complete player_HandleSabre stride, component gates,
 *                signed ID tests, and two-argument Jedi call are checked at
 *                0xE7480..0xE7518; player_gProcessPlayers collision-first
 *                ownership, 20-slot stride, attacker budgets, map trigger,
 *                pad/AI routing, life/Force HUD and damage overlay ownership,
 *                pause gate, and controller call are checked at
 *                0xE8170..0xE8A7C. The final developer controller diagnostic
 *                is checked at 0xE6D90..0xE721A through its exact legacy text
 *                renderer boundary.
 *
 * PDB module: 0064
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\player.obj
 * Primary source: W:\SWJediPowerBattles\Work\player.c
 * Compiler language: c
 * Emitted procedures: 22
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/player.h"
#include "jpb/anim.h"
#include "jpb/animutil.h"
#include "jpb/brain.h"
#include "jpb/brainutl.h"
#include "jpb/braindmg.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/combo.h"
#include "jpb/debugtext.h"
#include "jpb/enemy.h"
#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/intersec.h"
#include "jpb/jedi.h"
#include "jpb/level_world.h"
#include "jpb/loader.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/text.h"
#include "jpb/wrender.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Direct global at RVA 0x53A600, PDB type 0xA618. */
playerObject gaPlayerData[JPB_PLAYER_CAPACITY];

/* Direct local at RVA 0x53D260, unsigned long[2], PDB type 0x1320. */
static uint32_t mPlayerRead[2];

/* Direct local at RVA 0x53D268, unsigned long, PDB type 0x0022. */
static uint32_t mCharliePad;

static JPBPlayerProcessObserver player_process_observer;
static void *player_process_observer_user_data;
static JPBPlayerTileHook player_tile_hook;
static void *player_tile_hook_user_data;
static float player_tile_projection_depth;
static JPBPlayerFrameProfile jpb_player_frame_profile;
static int jpb_player_frame_profile_enabled;

static double jpb_player_profile_seconds(void)
{
    if (!jpb_player_frame_profile_enabled) {
        return 0.0;
    }
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

static void jpb_player_profile_record(
    double *last_seconds,
    double *max_seconds,
    double seconds)
{
    if (!jpb_player_frame_profile_enabled) {
        return;
    }
    if (last_seconds != NULL) {
        *last_seconds += seconds;
    }
    if (max_seconds != NULL && *last_seconds > *max_seconds) {
        *max_seconds = *last_seconds;
    }
}

static void jpb_player_profile_begin_frame(void)
{
    if (!jpb_player_frame_profile_enabled) {
        return;
    }
    jpb_player_frame_profile.lastTotalSeconds = 0.0;
    jpb_player_frame_profile.lastCollisionsSeconds = 0.0;
    jpb_player_frame_profile.lastGlobalBitsSeconds = 0.0;
    jpb_player_frame_profile.lastMapTriggersSeconds = 0.0;
    jpb_player_frame_profile.lastLifeTileSeconds = 0.0;
    jpb_player_frame_profile.lastDebugSeconds = 0.0;
    jpb_player_frame_profile.lastInputSeconds = 0.0;
    jpb_player_frame_profile.lastDamageTrackerSeconds = 0.0;
    jpb_player_frame_profile.lastPauseSeconds = 0.0;
    jpb_player_frame_profile.lastControlSeconds = 0.0;
    jpb_player_frame_profile.lastActivePlayers = 0;
}

void jpb_PlayerGetFrameProfile(JPBPlayerFrameProfile *profile)
{
    if (profile != NULL) {
        *profile = jpb_player_frame_profile;
    }
}

void jpb_PlayerSetFrameProfileEnabled(int enabled)
{
    jpb_player_frame_profile_enabled = enabled != 0;
    memset(&jpb_player_frame_profile, 0, sizeof(jpb_player_frame_profile));
    jpb_player_frame_profile.maxControlPlayerIndex = -1;
    jpb_player_frame_profile.maxControlPlayerId = -1;
}

void jpb_PlayerSetProcessObserver(
    JPBPlayerProcessObserver observer,
    void *user_data)
{
    player_process_observer = observer;
    player_process_observer_user_data = user_data;
}

void jpb_PlayerSetTileHook(
    JPBPlayerTileHook hook, void *user_data)
{
    player_tile_hook = hook;
    player_tile_hook_user_data = user_data;
}

static void player_pool_failure(void)
{
    /* Reference fatal routine is called with code 1 and does not return. */
    abort();
}

/* 0xE6CE0, 3 bytes, global, 1 named locals
 * ch_blipad
 * PDB type: int (unsigned short)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
int ch_blipad(uint16_t button)
{
    (void)button;
    return 0;
}

/* 0xE6CF0, 3 bytes, global, 1 named locals
 * ch_pad
 * PDB type: int (unsigned short)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
int ch_pad(uint16_t button)
{
    (void)button;
    return 0;
}

/* 0xE6D00, 3 bytes, global, 0 named locals
 * ch_padadmin
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void ch_padadmin(void)
{
}

/* 0xE6D10, 3 bytes, global, 1 named locals
 * ch_unblipad
 * PDB type: int (unsigned short)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */

int ch_unblipad(uint16_t button)
{
    (void)button;
    return 0;
}

/* 0xE6D20, 102 bytes, global, 1 named locals
 * player_AfterLife
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void player_AfterLife(playerObject *player)
{
    int object_id = player->playerRoot.objectID;

    game_gSetPowerType(object_id, 0);
    game_gSetPowerLevel(object_id, 0);
    game_gSetItemCount(object_id, 0);
    if (player->lockRing != NULL) {
        player->lockRing[15] = 0;
        player->lockRing = NULL;
    }
    if (player->shadow != NULL) {
        sprite_gFreeSprite((Sprite *)(void *)player->shadow);
    }
    player->shadow = NULL;
}

/* 0xE6D90, 1163 bytes, global, 3 named locals
 * player_ControllerDump
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void player_ControllerDump(void)
{
    char output[256];
    size_t length;
    size_t source_index;
    size_t output_index = 0;

    if (GameStruct.screenShotFlag != 0) {
        return;
    }
    memset(output, 0, 255);
    length = strlen(gaPlayerData[0].PreMotion);
    source_index = length < 17 ? 0 : length - 16;
    while (source_index < length) {
        char character = gaPlayerData[0].PreMotion[source_index++];

        switch (character) {
        case 'e':
            character = 'E';
            break;
        case 'f':
            character = 'F';
            break;
        case 'n':
            character = 'N';
            break;
        case 's':
            character = 'S';
            break;
        case 'w':
            character = 'W';
            break;
        case 'L':
        case 'M':
        case 'R':
        case 'S':
            continue;
        case '!':
        case '+':
        case '.':
        case 'J':
        case 'K':
        default:
            break;
        }
        output[output_index++] = character;
    }
    output[output_index] = '\0';

    Text_gWrite(
        0x1000, -128, 2, 0, 20, 182, SmallFont, output);
    Text_gWrite(
        0x1000,
        -128,
        11,
        0,
        20,
        200,
        SmallFont,
        (*gaPlayerData[0].pMotion)->name);
}

static CVECTOR player_packed_color(uint32_t color)
{
    CVECTOR result;

    result.r = (uint8_t)(color >> 16);
    result.g = (uint8_t)(color >> 8);
    result.b = (uint8_t)color;
    result.cd = (uint8_t)(color >> 24);
    return result;
}

/* 0xE6B10, 323 bytes, global, 5 named locals
 * _DrawTile
 * PDB type: void (FVECTOR*, float, float, un...
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void _DrawTile(
    FVECTOR *position,
    float width,
    float height,
    uint32_t color,
    _Material *material)
{
    float projection_depth;

    (void)material;
    if (position == NULL || player_tile_hook == NULL) {
        return;
    }
    projection_depth = player_tile_projection_depth;
    if (!(projection_depth >= 1.0f)) {
        projection_depth = position->vz;
    }
    player_tile_hook(
        player_tile_hook_user_data,
        position,
        width,
        height,
        color,
        projection_depth);
}

/* 0xE6C60, 126 bytes, global, 7 named locals
 * _DrawTile2D
 * PDB type: void (FVECTOR*, float, float, un...
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void _DrawTile2D(
    FVECTOR *position,
    float width,
    float height,
    uint32_t color,
    int layer_depth)
{
    SCREENRECT destination;

    (void)layer_depth;
    if (position == NULL) {
        return;
    }
    destination.left = (int32_t)position->vx;
    destination.top = (int32_t)position->vy;
    destination.right = (int32_t)(position->vx + width);
    destination.bottom = (int32_t)(position->vy + height);
    _DrawTexture(
        NULL,
        destination,
        NULL,
        player_packed_color(color),
        0.0f);
}

/* 0xE6660, 1195 bytes, global, 20 named locals
 * _AddLifeTile
 * PDB type: void (playerObject*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void _AddLifeTile(playerObject *player, VECTOR *center)
{
    _svector t;
    FVECTOR pos;
    FVECTOR pos2;
    float screenz;
    float scaleMultiplier;
    float scaledEdgeWidth;
    float cachedX;
    uint32_t level;
    uint32_t defLevel;
    uint32_t level2;
    uint32_t defLevel2;
    uint32_t widestLevel;
    uint32_t green;
    uint32_t blue;
    uint32_t white;
    int brightscale;
    int brightness;
    int brightly;

    if (player == NULL || center == NULL ||
        OptionStruct.overlayMode == 0 ||
        GameStruct.screenShotFlag != 0 ||
        GameStruct.CurrentLevel == 0 ||
        (player->playernum < 2 &&
         OptionStruct.overlayMode != 1)) {
        return;
    }

    t.vx = (int16_t)center->vx;
    t.vy = (int16_t)center->vy;
    t.vz = (int16_t)center->vz;
    t.pad = 0;
    (void)fRotTransPers(&CameraMatrix, &t, &pos, 1);
    screenz = getscreenz(&CameraMatrix, center);
    if (screenz < 1.0f) {
        return;
    }

    pos.vy += 6.0f;
    brightness = (int)(screenz * 256.0f / 3072.0f);
    if (brightness < 64) {
        brightness = 64;
    }
    brightscale = 255 - brightness;
    if ((unsigned)brightscale >= 255U) {
        return;
    }
    brightly = brightscale * 255 >> 8;
    if (brightly == 0) {
        return;
    }

    green = color_interpolate(
        UINT32_C(0xff108010),
        UINT32_C(0x00108010),
        brightly);
    blue = color_interpolate(
        UINT32_C(0xff101080),
        UINT32_C(0x00101080),
        brightly);
    white = color_interpolate(
        UINT32_C(0xff808080),
        UINT32_C(0x00808080),
        brightly * 224 >> 8);

    scaleMultiplier = screenz / 500.0f;
    scaledEdgeWidth = scaleMultiplier * 3.0f;
    pos.vz = screenz - 70.0f;
    pos.vy -= 15.0f;
    level = (uint32_t)game_gGetScaleEnergy(player->playernum);
    defLevel =
        (uint32_t)game_gGetScaleMaxEnergy(player->playernum);
    level2 = 0;
    defLevel2 = 0;
    if (player->playernum < 2 && player->playerID < 10) {
        level2 =
            (uint32_t)game_gGetScaleForce(player->playernum);
        defLevel2 = (uint32_t)game_gGetScaleMaxForce(
            player->playernum);
    }
    widestLevel =
        defLevel2 > defLevel ? defLevel2 : defLevel;
    cachedX = pos.vx -
        ((float)widestLevel * 3.0f * 0.5f + 7.6f) *
            0.5f * scaleMultiplier;
    pos.vx = cachedX;
    if (player->playerRoot.objectID < 2) {
        pos.vz = 0.0001f;
    } else {
        pos.vz = (float)((double)pos.vz - 0.0000001);
    }

    player_tile_projection_depth = screenz;
    if (player->playerRoot.objectID > 1 ||
        (player->forceFlags & UINT32_C(0x10)) == 0) {
        _DrawTile(
            &pos,
            scaledEdgeWidth,
            scaledEdgeWidth,
            white,
            NULL);
        pos.vx += scaleMultiplier * 3.8f;
        if (level != 0) {
            _DrawTile(
                &pos,
                (float)level * scaledEdgeWidth * 0.5f,
                scaledEdgeWidth,
                green,
                NULL);
        }
        pos2.vx =
            pos.vx +
            (float)defLevel * scaledEdgeWidth * 0.5f +
            scaleMultiplier * 0.8f;
        pos2.vy = pos.vy;
        pos2.vz = pos.vz;
        _DrawTile(
            &pos2,
            scaledEdgeWidth,
            scaledEdgeWidth,
            white,
            NULL);
    }

    if (player->playernum < 2 && player->playerID < 10) {
        pos.vx = cachedX;
        pos.vy += scaleMultiplier * 6.0f;
        _DrawTile(
            &pos,
            scaledEdgeWidth,
            scaledEdgeWidth,
            white,
            NULL);
        pos.vx += scaleMultiplier * 3.8f;
        if (level2 != 0) {
            _DrawTile(
                &pos,
                (float)level2 * scaledEdgeWidth * 0.5f,
                scaledEdgeWidth,
                blue,
                NULL);
        }
        pos2.vx =
            pos.vx +
            (float)defLevel2 * scaledEdgeWidth * 0.5f +
            scaleMultiplier * 0.8f;
        pos2.vy = pos.vy;
        pos2.vz = pos.vz;
        _DrawTile(
            &pos2,
            scaledEdgeWidth,
            scaledEdgeWidth,
            white,
            NULL);
    }
    player_tile_projection_depth = 0.0f;
}
/* 0xE7220, 462 bytes, global, 3 named locals
 * player_DoCollisions
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
int player_DoCollisions(void)
{
    int attacker_index;

    for (attacker_index = 0;
         attacker_index < JPB_PLAYER_CAPACITY;
         ++attacker_index) {
        playerObject *attacker =
            &gaPlayerData[attacker_index];
        int target_index;

        if (attacker->playerRoot.objectID == -1 ||
            (attacker->pFlags & UINT32_C(0x602)) != 0 ||
            (obj_gCheckObjectFlag(
                 &attacker->playerRoot,
                 0,
                 UINT32_C(0x10)) == 0 &&
             (attacker->forceFlags & UINT32_C(0x40)) == 0)) {
            continue;
        }
        obj_gClrObjectFlag(
            &attacker->playerRoot, 0, UINT32_C(0x10));

        for (target_index = 0;
             target_index < JPB_PLAYER_CAPACITY;
             ++target_index) {
            playerObject *target =
                &gaPlayerData[target_index];
            int range;
            int hits;

            if (target_index == attacker_index ||
                (GameStruct.versusModeFlag == 0 &&
                 target_index == 1) ||
                target->playerRoot.objectID == -1 ||
                (target->pFlags & UINT32_C(0x602)) != 0) {
                continue;
            }
            if (target_index > 1 && attacker_index > 1 &&
                attacker->playerID != 0x23 &&
                attacker->pEnemy->pPlace->aiDf.daDelay != 3 &&
                target->pEnemy->pPlace->aiDf.daDelay != 3) {
                continue;
            }
            if (obj_gCheckObjectFlag(
                    &target->playerRoot,
                    0,
                    UINT32_C(0x20)) != 0 ||
                target->numCollisionNodes == 0) {
                continue;
            }
            range = physics_gGetRange(
                &attacker->playerRoot,
                &target->playerRoot);
            if (range >= 0x200 &&
                (attacker->pFlags & UINT32_C(0x2000)) == 0 &&
                (target->pFlags & UINT32_C(0x2000)) == 0) {
                continue;
            }
            hits = coll_gCheckHotNodes(attacker, target);
            if (hits != 0) {
                attacker->target = target;
                target->hitNumber = (uint8_t)(
                    target->hitNumber + (uint8_t)hits);
                target->target = attacker;
                target->whohitme = attacker;
            }
        }
    }
    return 1;
}

/* 0xE73F0, 103 bytes, global, 2 named locals
 * player_FreePlayer
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void player_FreePlayer(playerObject *player)
{
    if (player->playerRoot.objectID >= 0) {
        gaPlayerData[0].target = &gaPlayerData[1];
        gaPlayerData[1].target = &gaPlayerData[0];
        player->pFlags = UINT32_C(0x00800000);
        player->pEnemy->exit_flag = 1;
        if (player->shadow != NULL) {
            sprite_gFreeSprite(
                (Sprite *)(void *)player->shadow);
        }
        (void)obj_gClearObject(&player->playerRoot);
    }
}

/* 0xE7460, 21 bytes, global, 1 named locals
 * player_GetPlayerPad
 * PDB type: Pad* (int)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
Pad *player_GetPlayerPad(int index)
{
    return &gaPlayerData[index].playerPad;
}

/*
 * Extracted integration boundary from player_gProcessPlayers
 * (reference RVAs 0xE84DA..0xE853F).
 *
 * The original block samples both Pad channels into mPlayerRead, suppresses
 * them when GameStruct.CurrentLevel is zero, and publishes them to cpad.
 * Passing input_enabled keeps the large gamestruct dependency outside this
 * narrow player/input boundary while preserving those exact state changes.
 */
void jpb_PlayerSamplePad(
    playerObject *player, int pad_index, int input_enabled)
{
    Pad *pad = &player->playerPad;

    mPlayerRead[0] = input_ReadControlPad(
        pad_index, pad->mask0, &pad->oldbits0);
    mPlayerRead[1] = input_ReadControlPad(
        pad_index, pad->mask1, &pad->oldbits1);
    if (!input_enabled) {
        mPlayerRead[0] = 0;
        mPlayerRead[1] = 0;
    }
    pad->cpad[0] = mPlayerRead[0];
    pad->cpad[1] = mPlayerRead[1];
}

static void player_DrawDamageTracker(
    int index, playerObject *player)
{
    DamageTracker *tracker = &damageTracking[index];
    float x = 48.0f;
    float y = 36.0f;
    float width = 480.0f;
    float height = 204.0f;
    float anchor_x;
    float anchor_y = 91.0f;
    SCREENRECT destination;
    uint32_t color;
    int brightness_limit = 0;
    int brightness;

    if (!(tracker->total > 0.0f)) {
        return;
    }
    if (player->playerID < 9) {
        brightness_limit =
            ((int)jediUpgrades[player->playerID]
                 .attackDefendUpgrades >> 4) * 5;
    }
    brightness = (int)tracker->total;
    if (brightness > brightness_limit + 128) {
        brightness = brightness_limit + 128;
    }
    brightness *= 2;
    if (brightness < 0) {
        brightness = 0;
    } else if (brightness > 255) {
        brightness = 255;
    }
    color = color_interpolate(
        UINT32_C(0x7ffc0000),
        UINT32_C(0x7ffcfc00),
        brightness);

    if (OptionStruct.overlayMode == 1) {
        anchor_x = index == 0 ? 2.0f : 480.0f;
    } else if (OptionStruct.overlayMode == 2) {
        anchor_x = index == 0 ? 163.0f : 317.0f;
    } else {
        goto update_timer;
    }
    setPivotPositionAndFixScale(
        &x,
        &y,
        &width,
        &height,
        index == 0 ? 0 : 2);
    setPositionOffPivot(
        &anchor_x, &anchor_y, x, y);

    if (index == 0) {
        destination.left = (int32_t)anchor_x;
        destination.right = (int32_t)(
            anchor_x + scaleAdjustment * tracker->total);
    } else {
        destination.left = (int32_t)(
            anchor_x - scaleAdjustment * tracker->total);
        destination.right = (int32_t)anchor_x;
    }
    destination.top = (int32_t)anchor_y;
    destination.bottom = (int32_t)(
        anchor_y + scaleAdjustment * 12.0f + 1.0f);
    _DrawTexture(
        NULL,
        destination,
        NULL,
        player_packed_color(color),
        0.0f);

update_timer:
    if (gGlobalFrameRate != 0) {
        tracker->total -= 0.25f;
        if ((player->pFlags & UINT32_C(0x20)) == 0) {
            tracker->total -= 0.5f;
        }
    }
}

/* 0xE7480, 153 bytes, global, 0 named locals
 * player_HandleSabre
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void player_HandleSabre(void)
{
    int index;

    for (index = 0; index < JPB_PLAYER_CAPACITY; ++index) {
        playerObject *player = &gaPlayerData[index];
        sceneObject *scene;
        modelObject *model;
        int16_t player_id;

        if (player->playerRoot.objectID == -1 ||
            (player->playerRoot.flags & UINT32_C(0x20)) != 0) {
            continue;
        }
        scene = (sceneObject *)player->playerRoot.pParent;
        if (scene == NULL || scene->pModel == NULL) {
            continue;
        }
        model = (modelObject *)scene->pModel;
        if ((model->flags & UINT32_C(4)) != 0 ||
            obj_gCheckObjectFlag(
                &player->playerRoot,
                0,
                UINT32_C(0x20)) != 0 ||
            (player->pFlags & UINT32_C(0x80)) != 0) {
            continue;
        }
        player_id = player->playerID;
        if (player_id < 10 || player_id == 0x2b ||
            (uint16_t)(player_id - 0x50) < 5U) {
            (void)jedi_HandleSabre(NULL, player);
        }
    }
}

/* 0xE7520, 1904 bytes, global, 12 named locals
 * player_RefreshPlayer
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void player_RefreshPlayer(playerObject *player)
{
    int index = player->playerRoot.objectID;
    wsl_ENEMY *pEnemy;
    sceneObject *scene;
    modelObject *model;
    physicsObject *physics;
    VECTOR pos;
    int height;

    if (index == -1) {
        return;
    }

    pEnemy = player->pEnemy;
    scene = (sceneObject *)player->playerRoot.pParent;
    model = (modelObject *)scene->pModel;
    physics = (physicsObject *)scene->pPhysics;

    game_gSetPowerType(index, 0);
    game_gSetPowerLevel(index, 0);
    game_gSetItemCount(index, 0);

    if (index == 0) {
        if (LevelSelect < JPB_LEVEL_COUNT) {
            if (gCheckPoint == 0) {
                game_gSetScore(
                    0,
                    LevelSelect == 15
                        ? corusPoints[0]
                        : 0);
                GameStruct.Counter = 0;
                pos.vx =
                    0x8000 -
                    (int32_t)startPos[
                        (int)LevelSelect][0].vx * 0x100;
                pos.vy =
                    (int32_t)startPos[
                        (int)LevelSelect][0].vz * 0x100;
                pos.vz =
                    ((int32_t)startPos[
                         (int)LevelSelect][0].vy -
                     0x7f) *
                    0x100;
            } else {
                pos.vx = reStartPos[0].vx;
                pos.vy = reStartPos[0].vy;
                pos.vz = reStartPos[0].vz;
                game_gSetScore(0, (int)reStartScore[0]);
                GameStruct.Counter = reStartCounter;
            }
        } else {
            pos.vx =
                0x8000 - gpWorld->p0location.vx * 0x100;
            pos.vy = gpWorld->p0location.vy * 0x100;
            pos.vz =
                (gpWorld->p0location.vz - 0x7f) * 0x100;
        }
        if (GameStruct.CurrentLevel == 20) {
            pos.vx += 0x80;
            pos.vz += 0x80;
        }

        physics_gSetPosition(
            &player->playerRoot,
            pos.vx,
            pos.vy,
            pos.vz);
        if (GameStruct.CurrentLevel < 26 &&
            ((UINT32_C(0x02106c22) >>
              GameStruct.CurrentLevel) &
             1U) != 0) {
            physics_gSetFacing(
                &player->playerRoot, 0x800);
        }
        if (GameStruct.CurrentLevel == 9 ||
            (GameStruct.CurrentLevel >= 16 &&
             GameStruct.CurrentLevel < 20)) {
            physics_gSetFacing(
                &player->playerRoot, 0x400);
        }
        if (GameStruct.CurrentLevel == 3 ||
            GameStruct.CurrentLevel == 6 ||
            GameStruct.CurrentLevel == 7) {
            physics_gSetFacing(
                &player->playerRoot, 0xc00);
        }
        height = intersec_FindWalkHeight(
            &pos, NULL, &player->playerRoot, 0);
        physics->validairground = (float)height;

        if (player->playerID < 10 &&
            GameStruct.versusModeFlag == 0) {
            int energy =
                GameStruct.maxEnergyLevels[
                    player->playerID];
            int force =
                GameStruct.maxForceLevels[
                    player->playerID];

            game_gSetMaxEnergy(player->playernum, energy);
            game_gSetEnergy(player->playernum, energy);
            game_gSetMaxForce(player->playernum, force);
            game_gSetForce(player->playernum, force);
        } else {
            game_gSetMaxEnergy(player->playernum, 200);
            game_gSetEnergy(player->playernum, 200);
            game_gSetMaxForce(player->playernum, 200);
            game_gSetForce(player->playernum, 200);
        }
        obj_gClrObjectFlag(
            &player->playerRoot, 0, UINT32_C(0x20));
        player->pFlags &= ~UINT32_C(0x80);
        model->flags &= ~UINT32_C(0x10);
        if (player->shadow == NULL) {
            player->shadow =
                (int32_t *)(void *)
                    sprite_GetBaseNodeMarker(index, 0x30);
        }
    } else if (index == 1) {
        if (LevelSelect < JPB_LEVEL_COUNT) {
            if (gCheckPoint == 0) {
                game_gSetScore(
                    1,
                    LevelSelect == 15
                        ? corusPoints[1]
                        : 0);
                GameStruct.Counter = 0;
                pos.vx =
                    0x8000 -
                    (int32_t)startPos[
                        (int)LevelSelect][1].vx * 0x100;
                pos.vy =
                    (int32_t)startPos[
                        (int)LevelSelect][1].vz * 0x100;
                pos.vz =
                    ((int32_t)startPos[
                         (int)LevelSelect][1].vy -
                     0x7f) *
                    0x100;
            } else {
                pos.vx = reStartPos[1].vx;
                pos.vy = reStartPos[1].vy;
                pos.vz = reStartPos[1].vz;
                game_gSetScore(1, (int)reStartScore[1]);
                GameStruct.Counter = reStartCounter;
            }
        } else {
            pos.vx =
                0x7f00 - gpWorld->p1location.vx * 0x100;
            pos.vy = gpWorld->p1location.vy * 0x100;
            pos.vz =
                (gpWorld->p1location.vz - 0x7e) * 0x100;
        }

        physics_gSetPosition(
            &player->playerRoot,
            pos.vx,
            pos.vy,
            pos.vz);
        if (GameStruct.CurrentLevel < 15 &&
            ((UINT32_C(0x00006c22) >>
              GameStruct.CurrentLevel) &
             1U) != 0) {
            physics_gSetFacing(
                &player->playerRoot, 0x800);
        }
        if (GameStruct.CurrentLevel == 9) {
            physics_gSetFacing(
                &player->playerRoot, 0x400);
        }
        if (GameStruct.CurrentLevel == 3 ||
            GameStruct.CurrentLevel == 6 ||
            GameStruct.CurrentLevel == 7) {
            physics_gSetFacing(
                &player->playerRoot, 0xc00);
        }
        height = intersec_FindWalkHeight(
            &pos, NULL, &player->playerRoot, 0);
        physics->validairground = (float)height;

        if (player->playerID < 10 &&
            GameStruct.versusModeFlag == 0) {
            int energy =
                GameStruct.maxEnergyLevels[
                    player->playerID];
            int force =
                GameStruct.maxForceLevels[
                    player->playerID];

            game_gSetMaxEnergy(player->playernum, energy);
            game_gSetEnergy(player->playernum, energy);
            game_gSetMaxForce(player->playernum, force);
            game_gSetForce(player->playernum, force);
        } else {
            game_gSetMaxEnergy(player->playernum, 200);
            game_gSetEnergy(player->playernum, 200);
            game_gSetMaxForce(player->playernum, 200);
            game_gSetForce(player->playernum, 200);
        }

        if (GameStruct.NumPlayers == 2) {
            obj_gClrObjectFlag(
                &player->playerRoot,
                0,
                UINT32_C(0x20));
            if (player->shadow == NULL) {
                player->shadow =
                    (int32_t *)(void *)
                        sprite_GetBaseNodeMarker(
                            index, 0x30);
            }
        }
    } else {
        wsl_BAP_PLACEMENT *pPlace = pEnemy->pPlace;

        pos.vx = pPlace->loc.vx;
        pos.vy = pPlace->loc.vy;
        pos.vz = pPlace->loc.vz;
        physics_gSetPosition(
            &player->playerRoot,
            pos.vx,
            pos.vy,
            pos.vz);
        height = intersec_FindWalkHeight(
            (VECTOR *)(void *)&pPlace->loc,
            NULL,
            &player->playerRoot,
            0);
        physics->validairground = (float)height;
        game_gSetEnergy(
            player->playernum, pEnemy->hitPoints);
        game_gSetMaxEnergy(
            player->playernum, pEnemy->hitPoints);
        physics_gSetFacing(
            &player->playerRoot, pPlace->aiDf.angle);
    }

    player->pFlags &= UINT32_C(0x00802000);
    player->playernum = (int16_t)index;
    player->fLife = 0;
    player->fStun = 0;
    player->fForce = 0;
    player->hitMask = 0;
    player->hitDelay = 0;
    player->forceFlags = 0;
    player->ctime = 0;
    memset(player->bheld, 0, sizeof(player->bheld));
    player->chainSlack = 0;
    player->playerPad.oldbits0 = 0;
    player->playerPad.oldbits1 = 0;
    player->pForceCallBack = NULL;
    player->pMotionCallBack = NULL;
    player->comboUserData = 0;
    player->ACTION_LOCK = 0;
    player->airVelocity = 0;
    player->airAngle = 0;
    physics_gClrConstantVector(&player->playerRoot);
    player->subOffset = 0;

    if (scene->pAnim != NULL) {
        animObject *pAnimData =
            (animObject *)scene->pAnim;

        animutl_FlushSeqQueue(&player->playerRoot);
        (void)anim_AddNextAnimSeq(
            pAnimData, player->paMotions, 1);
        (void)anim_ForceNextAnimSeq(pAnimData, 0);
    }

    if (player->playerID == 3 &&
        player->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(
            &player->playerRoot,
            0,
            UINT32_C(0x20)) == 0 &&
        (player->pFlags & UINT32_C(0x40200)) == 0) {
        model->flags &= ~UINT32_C(0x10);
    }
    if (player->playerID == 30) {
        Mnode *node = coll_GetNode(index, 2);
        Mnode *node2 = coll_GetNode(index, 5);

        node->flags &= ~UINT32_C(0x04000000);
        node2->flags &= ~UINT32_C(0x04000000);
    }
    UpdateSceneObject(physics);
}

/* 0xE7C90, 119 bytes, global, 2 named locals
 * player_ResetJedi
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void player_ResetJedi(int index)
{
    playerObject *player = &gaPlayerData[index];
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    modelObject *model = (modelObject *)scene->pModel;
    uint32_t time;

    player->fLife = 0;
    player->fStun = 0;
    player->fForce = 0;
    player->playerPad.oldbits0 = 0;
    player->ACTION_LOCK = 0;
    player->runCounter = 0;
    player->groundDelay = 0;
    player->pFlags = 0;
    player->hitMask = 0;
    player->hitDelay = 0;
    player->hitNumber = 0;
    player->numAttackers = 0;
    player->projectile = NULL;
    model->flags &= UINT32_C(0xffffffef);
    time = brainutl_ElapsedTime(0, 0);
    combo_ResetComboEngine(time, player);
}

/* 0xE7D10, 368 bytes, global, 3 named locals
 * player_gConnectMotionData
 * PDB type: void (playerObject*, char*)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void player_gConnectMotionData(
    playerObject *player, char *motion_data)
{
    int32_t motions_offset;
    int16_t motion_count;
    int motion_index;

    if (motion_data == NULL) {
        return;
    }
    memcpy(&motions_offset,
           motion_data + 8,
           sizeof(motions_offset));
    memcpy(&motion_count,
           motion_data + 0x10,
           sizeof(motion_count));
    player->paMotions =
        (Motion *)(void *)(motion_data + motions_offset);
    player->maxMotions = motion_count;
    for (motion_index = 0;
         motion_index < player->maxMotions;
         ++motion_index) {
        Motion *motion = &player->paMotions[motion_index];

        if (strncmp(motion->name, "sabrhit", 7) == 0 ||
            strncmp(motion->name, "jedihit", 7) == 0) {
            motion->name[0] = '0';
        }
        if (strncmp(&motion->name[8], "sabrhit", 7) == 0 ||
            strncmp(&motion->name[8], "jedihit", 7) == 0) {
            motion->name[8] = '0';
        }
        if ((motion->motionFlags & UINT32_C(0x100000)) != 0) {
            motion->FunctPtr = 1;
        }
        if ((motion->motionFlags & UINT32_C(0x10)) != 0) {
            motion->FunctPtr = 2;
        }
        if ((uint8_t)motion->name[26] > UINT8_C(0x41)) {
            (void)loader_GetEnemyName(player->playerID);
        }
    }
}

/* 0xE7E80, 323 bytes, global, 8 named locals
 * player_gCreateObject
 * PDB type: playerObject* (sceneObject*, int,
 *                           int (playerObject*)*)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
playerObject *player_gCreateObject(
    sceneObject *pSceneObject,
    int type,
    JPBPlayerInitCallback fpPlayerInit)
{
    playerObject *pPlayer;
    animObject *tempanimChild;
    int ID = pSceneObject->sceneRoot.objectID;
    int index = 0;

    if ((uint32_t)ID < JPB_PLAYER_CAPACITY &&
        gaPlayerData[ID].playerRoot.objectID == -1) {
        pPlayer = &gaPlayerData[ID];
        pPlayer->playerRoot.objectID = ID;
        if (ID == 0) {
            gpWorld->player0 = pPlayer;
        } else if (ID == 1) {
            gpWorld->player1 = pPlayer;
        }
    } else {
        for (;;) {
            pPlayer = &gaPlayerData[index];
            if (pPlayer->playerRoot.objectID == -1) {
                pPlayer->playerRoot.objectID = index;
                break;
            }
            ++index;
            if (index >= JPB_PLAYER_CAPACITY) {
                player_pool_failure();
            }
        }
    }

    obj_gSetChildObject(
        pSceneObject, &pPlayer->playerRoot, 4);
    tempanimChild = (animObject *)pSceneObject->pAnim;
    if (tempanimChild != NULL) {
        pPlayer->pMotion = &tempanimChild->pMotion;
    }
    pPlayer->playernum = (int16_t)ID;
    pPlayer->playerID = (int16_t)type;
    pPlayer->fLife = 0;
    pPlayer->fStun = 0;
    pPlayer->fForce = 0;
    pPlayer->pFlags = 0;
    pPlayer->playerPad.oldbits0 = 0;
    pPlayer->playerPad.oldbits1 = 0;
    pPlayer->playerPad.mask0 = 0;
    pPlayer->ACTION_LOCK = 0;
    pPlayer->playerPad.mask1 = UINT32_MAX;
    if (fpPlayerInit != NULL) {
        (void)fpPlayerInit(pPlayer);
    }
    /* The original loader always attaches animation before this owner. */
    scene_gSetSceneModelKeyFrame(
        ID, tempanimChild->pCurrentAnimFrame);
    return pPlayer;
}

/* 0xE7FD0, 146 bytes, global, 3 named locals
 * player_gGetNewPlayerObject
 * PDB type: playerObject* (int)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
playerObject *player_gGetNewPlayerObject(int ID)
{
    playerObject *player = &gaPlayerData[0];
    int index;

    if ((uint32_t)ID < JPB_PLAYER_CAPACITY &&
        gaPlayerData[ID].playerRoot.objectID == -1) {
        player = &gaPlayerData[ID];
        player->playerRoot.objectID = ID;
        if (ID == 0) {
            gpWorld->player0 = player;
            return player;
        }
        if (ID == 1) {
            gpWorld->player1 = player;
        }
        return player;
    }
    for (index = 0; index < JPB_PLAYER_CAPACITY; ++index) {
        player = &gaPlayerData[index];
        if (player->playerRoot.objectID == -1) {
            player->playerRoot.objectID = index;
            return player;
        }
    }
    player_pool_failure();
    return NULL;
}

/* 0xE8070, 21 bytes, global, 1 named locals
 * player_gGetPlayerPtr
 * PDB type: playerObject* (int)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
playerObject *player_gGetPlayerPtr(int index)
{
    return &gaPlayerData[index];
}

/* 0xE8090, 221 bytes, global, 1 named locals
 * player_gInitPlayers
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void player_gInitPlayers(int start)
{
    int index;

    /*
     * Valid callers pass 0..19. The reference's signed comparison would let
     * a negative start walk before gaPlayerData; reject that corrupting case
     * while preserving every valid-input store and final byte value.
     */
    if ((uint32_t)start >= JPB_PLAYER_CAPACITY) {
        return;
    }
    for (index = start; index < JPB_PLAYER_CAPACITY; ++index) {
        playerObject *player = &gaPlayerData[index];

        memset(player, 0, sizeof(*player));
        player->playerRoot.objectID = -1;
        memcpy(
            player->playerRoot.objectName,
            "PLAYER",
            sizeof("PLAYER"));
        player->maxCombos = 0x30;
    }
}

/* 0xE8170, 2317 bytes, global, 39 named locals
 * player_gProcessPlayers
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void player_gProcessPlayers(void)
{
    int index;
    double frame_started;
    double stage_started;

    jpb_player_profile_begin_frame();
    frame_started = jpb_player_profile_seconds();
    stage_started = jpb_player_profile_seconds();
    (void)player_DoCollisions();
    jpb_player_profile_record(
        &jpb_player_frame_profile.lastCollisionsSeconds,
        &jpb_player_frame_profile.maxCollisionsSeconds,
        jpb_player_profile_seconds() - stage_started);
    PushMatrix();

    stage_started = jpb_player_profile_seconds();
    if (GameStruct.NumPlayers == 2 &&
        gpWorld != NULL &&
        gpWorld->player0 != NULL &&
        gpWorld->player1 != NULL &&
        obj_gCheckObjectFlag(
            &gpWorld->player0->playerRoot,
            0,
            UINT32_C(0x20)) == 0 &&
        obj_gCheckObjectFlag(
            &gpWorld->player1->playerRoot,
            0,
            UINT32_C(0x20)) == 0) {
        game_SET_GLOBALBIT(0x10U);
    } else {
        game_CLR_GLOBALBIT(0x10U);
    }
    jpb_player_profile_record(
        &jpb_player_frame_profile.lastGlobalBitsSeconds,
        &jpb_player_frame_profile.maxGlobalBitsSeconds,
        jpb_player_profile_seconds() - stage_started);

    for (index = 0; index < JPB_PLAYER_CAPACITY; ++index) {
        playerObject *player = &gaPlayerData[index];
        sceneObject *scene;
        physicsObject *physics;
        int AI_ON = 0;

        if (player->playerRoot.objectID == -1 ||
            (player->playerRoot.flags & UINT32_C(0x20)) != 0) {
            continue;
        }
        scene = (sceneObject *)player->playerRoot.pParent;
        if (scene == NULL || scene->pPhysics == NULL) {
            continue;
        }
        physics = (physicsObject *)scene->pPhysics;
        if (jpb_player_frame_profile_enabled) {
            ++jpb_player_frame_profile.lastActivePlayers;
            if (jpb_player_frame_profile.lastActivePlayers >
                jpb_player_frame_profile.maxActivePlayers) {
                jpb_player_frame_profile.maxActivePlayers =
                    jpb_player_frame_profile.lastActivePlayers;
            }
        }

        if (index < 2) {
            player->numAttackers = 2;
            if (physics->currentmapinfo.cube != NULL) {
                stage_started = jpb_player_profile_seconds();
                enemy_HandleMapTriggers(
                    physics->currentmapinfo.cube);
                jpb_player_profile_record(
                    &jpb_player_frame_profile.lastMapTriggersSeconds,
                    &jpb_player_frame_profile.maxMapTriggersSeconds,
                    jpb_player_profile_seconds() - stage_started);
            }
        } else {
            player->numAttackers = 20;
        }

        if (obj_gCheckObjectFlag(
                &player->playerRoot,
                0,
                UINT32_C(0x20)) == 0 &&
            (player->forceFlags & UINT32_C(0x200)) == 0 &&
            (player->pFlags & UINT32_C(0x80)) == 0 &&
            (physics->flags & UINT32_C(0x20000000)) == 0 &&
            game_gGetEnergy(index) != 255) {
            stage_started = jpb_player_profile_seconds();
            _AddLifeTile(
                player,
                physics_gGetPosition(
                    &player->playerRoot));
            jpb_player_profile_record(
                &jpb_player_frame_profile.lastLifeTileSeconds,
                &jpb_player_frame_profile.maxLifeTileSeconds,
                jpb_player_profile_seconds() - stage_started);
        }

        stage_started = jpb_player_profile_seconds();
        if (OptionStruct.DebugLevel == 2) {
            const char *attack_state =
                (player->pFlags & UINT32_C(1)) != 0 ? "LAND" : "";
            const char *boss_state =
                (player->pFlags & UINT32_C(8)) != 0 ? "BOSS" : "";
            const char *immortal_state =
                (player->forceFlags & UINT32_C(0x10)) != 0
                    ? "IMMORTAL"
                    : "";
            const char *air_state =
                (player->forceFlags & UINT32_C(0x2000)) != 0
                    ? "AIR"
                    : "";
            const char *enemy_name = "";
            int enemy_ai = 0;
            int status = 0;

            if (player->pEnemy != NULL) {
                enemy_name = player->pEnemy->aName;
                enemy_ai = player->pEnemy->aiNum;
                if (player->pEnemy->pPlace != NULL) {
                    status = player->pEnemy->pPlace->status;
                }
            }
            if (index == 0) {
                float x = 16.0f;
                float y = 45.0f;
                float width = 32.0f;
                float height = 48.0f;

                setPivotPositionAndFixScale(
                    &x,
                    &y,
                    &width,
                    &height,
                    0);
                setPositionOffPivot(&x, &y, 2.0f, 91.0f);
                scr_debugPrintfXYZ(
                    (int)x,
                    (int)y,
                    (int)(x + width),
                    "%d-%s ai %d\nt-%s\n%s %s %s %s",
                    player->playerRoot.objectID,
                    enemy_name,
                    enemy_ai,
                    physics->currentmapinfo.cube != NULL ? "LAND" : "",
                    attack_state,
                    boss_state,
                    immortal_state,
                    air_state);
            } else if (index == 1) {
                float x = 91.0f;
                float y = 45.0f;
                float width = 32.0f;
                float height = 48.0f;

                setPivotPositionAndFixScale(
                    &x,
                    &y,
                    &width,
                    &height,
                    2);
                setPositionOffPivot(&x, &y, 2.0f, 91.0f);
                scr_debugPrintfXYZ(
                    (int)(x - width),
                    (int)y,
                    (int)x,
                    "%d-%s ai %d\nt-%s\n%s %s %s %s",
                    player->playerRoot.objectID,
                    enemy_name,
                    enemy_ai,
                    physics->currentmapinfo.cube != NULL ? "LAND" : "",
                    attack_state,
                    boss_state,
                    immortal_state,
                    air_state);
            }
        } else if (OptionStruct.DebugLevel > 2 &&
                   player->pEnemy != NULL) {
            const char *state = "";

            if (player->pEnemy->pPlace != NULL) {
                state = player->pEnemy->pPlace->aName;
            }
            scr_debugPrintfXYZ(
                physics->vpos.vx,
                physics->vpos.vy,
                physics->vpos.vz,
                player->pEnemy->pPlace != NULL ? "%d-%s %s" : "%d-%s",
                player->playerRoot.objectID,
                player->pEnemy->aName,
                state);
        }
        jpb_player_profile_record(
            &jpb_player_frame_profile.lastDebugSeconds,
            &jpb_player_frame_profile.maxDebugSeconds,
            jpb_player_profile_seconds() - stage_started);

        if (index < 2 && player->pEnemy == NULL) {
            stage_started = jpb_player_profile_seconds();
            mPlayerRead[0] = input_ReadControlPad(
                index,
                player->playerPad.mask0,
                &player->playerPad.oldbits0);
            mPlayerRead[1] = input_ReadControlPad(
                index,
                player->playerPad.mask1,
                &player->playerPad.oldbits1);
            if (GameStruct.CurrentLevel == 0) {
                mPlayerRead[0] = 0;
                mPlayerRead[1] = 0;
            }
            player->playerPad.cpad[0] = mPlayerRead[0];
            player->playerPad.cpad[1] = mPlayerRead[1];

            if (GameStruct.CurrentLevel != 12 &&
                gpWorld != NULL) {
                if (gpWorld->player0 != NULL) {
                    gpWorld->player0->playerPad.cpad[0] =
                        mPlayerRead[0];
                }
                if (gpWorld->player1 != NULL) {
                    gpWorld->player1->playerPad.cpad[0] =
                        mPlayerRead[1];
                }
            }
            if (index == 0) {
                mCharliePad = player->playerPad.cpad[1];
            }
            jpb_player_profile_record(
                &jpb_player_frame_profile.lastInputSeconds,
                &jpb_player_frame_profile.maxInputSeconds,
                jpb_player_profile_seconds() - stage_started);
            stage_started = jpb_player_profile_seconds();
            player_DrawDamageTracker(index, player);
            jpb_player_profile_record(
                &jpb_player_frame_profile.lastDamageTrackerSeconds,
                &jpb_player_frame_profile.maxDamageTrackerSeconds,
                jpb_player_profile_seconds() - stage_started);
            stage_started = jpb_player_profile_seconds();
            if (brainutil_PauseControl(
                    (int32_t *)mPlayerRead,
                    player) != 0 ||
                (player->pFlags & UINT32_C(2)) != 0) {
                mPlayerRead[0] = 0;
                mPlayerRead[1] = 0;
                player->playerPad.cpad[0] = 0;
                player->playerPad.cpad[1] = 0;
            }
            jpb_player_profile_record(
                &jpb_player_frame_profile.lastPauseSeconds,
                &jpb_player_frame_profile.maxPauseSeconds,
                jpb_player_profile_seconds() - stage_started);
        } else if (player->pEnemy == NULL ||
                   (player->pEnemy->enemyFlags &
                    UINT32_C(0x0c000000)) == 0) {
            AI_ON = 1;
            mPlayerRead[0] = 0;
            mPlayerRead[1] = 0;
        } else if ((player->pEnemy->enemyFlags &
                    UINT32_C(0x04000000)) != 0) {
            mPlayerRead[0] =
                gaPlayerData[0].playerPad.cpad[0];
            mPlayerRead[1] =
                gaPlayerData[0].playerPad.cpad[1];
        } else {
            mPlayerRead[0] =
                gaPlayerData[1].playerPad.cpad[0];
            mPlayerRead[1] =
                gaPlayerData[1].playerPad.cpad[1];
        }

        if ((player->pFlags & UINT32_C(0x80)) == 0) {
            if (player_process_observer != NULL) {
                player_process_observer(
                    JPB_PLAYER_PROCESS_BEFORE_CONTROL,
                    index,
                    player,
                    AI_ON,
                    (const int32_t *)mPlayerRead,
                    player_process_observer_user_data);
            }
            stage_started = jpb_player_profile_seconds();
            brain_ControlPlayer(
                (int32_t *)mPlayerRead,
                player,
                AI_ON);
            {
                double previous_max =
                    jpb_player_frame_profile.maxControlSeconds;
                double control_seconds =
                    jpb_player_profile_seconds() - stage_started;

                jpb_player_profile_record(
                    &jpb_player_frame_profile.lastControlSeconds,
                    &jpb_player_frame_profile.maxControlSeconds,
                    control_seconds);
                if (jpb_player_frame_profile_enabled &&
                    jpb_player_frame_profile.maxControlSeconds >
                        previous_max) {
                    jpb_player_frame_profile.maxControlPlayerIndex = index;
                    jpb_player_frame_profile.maxControlPlayerId =
                        player->playerRoot.objectID;
                }
            }
            if (player_process_observer != NULL) {
                player_process_observer(
                    JPB_PLAYER_PROCESS_AFTER_CONTROL,
                    index,
                    player,
                    AI_ON,
                    (const int32_t *)mPlayerRead,
                    player_process_observer_user_data);
            }
        }
    }
    PopMatrix();
    jpb_player_profile_record(
        &jpb_player_frame_profile.lastTotalSeconds,
        &jpb_player_frame_profile.maxTotalSeconds,
        jpb_player_profile_seconds() - frame_started);
}

/* 0xE8A80, 254 bytes, global, 0 named locals
 * player_gRefreshPlayers
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\player.c
 */
void player_gRefreshPlayers(void)
{
    int index;

    for (index = 0;
         index < JPB_PLAYER_CAPACITY;
         ++index) {
        player_RefreshPlayer(&gaPlayerData[index]);
    }
    if (LevelSelect == 8) {
        totalframes = 0;
        physics_ResetJedi(0);
        physics_ResetJedi(1);
        streets_reached_stairs = 0;
        memset(&bestinfo, 0, sizeof(bestinfo));
        for (index = 0;
             index < JPB_PHYSICS_CAPACITY;
             ++index) {
            maPhysicsData[index].anycollidetime = 0;
        }
    }
    gSCENE_READY = 0;
}
