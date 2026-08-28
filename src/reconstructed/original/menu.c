/*
 * REVIEWED RECONSTRUCTION.
 *
 * The exact MENUVARS title stack, title initialization, player/model
 * selection, objective/game-over transitions, and small menu state leaves
 * are recovered under their PDB names. Platform input enumeration, storage,
 * movie dispatch, controller metadata, and process/window services cross
 * explicit host boundaries. Test-only presentation observers run only after
 * the complete character-select draw owners. menu_specialMess retains its
 * exact two-entry stack mutation from direct shipped-executable disassembly.
 *
 * PDB module: 0055
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\menu.obj
 * Primary source: W:\SWJediPowerBattles\work\menu.c
 * Compiler language: c
 * Emitted procedures: 191
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/menu.h"
#include "jpb/achievement.h"
#include "jpb/alltext.h"
#include "jpb/audio_stream.h"
#include "jpb/camera.h"
#include "jpb/combo.h"
#include "jpb/debugtext.h"
#include "jpb/extracharacters.h"
#include "jpb/game.h"
#include "jpb/filesys.h"
#include "jpb/input.h"
#include "jpb/jedi.h"
#include "jpb/linkstubs.h"
#include "jpb/memory.h"
#include "jpb/physics.h"
#include "jpb/pwrup.h"
#include "jpb/prim.h"
#include "jpb/resources.h"
#include "jpb/savegame.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/text.h"
#include "jpb/texture.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

extern void VSync();
extern void __EndRender(void);
extern void __StartRender(void);

static JPBMenuP1CharacterSelectDrawHook
    jpb_menu_p1_character_select_draw_hook;
static void *jpb_menu_p1_character_select_draw_user_data;
static JPBMenuP2CharacterSelectDrawHook
    jpb_menu_p2_character_select_draw_hook;
static void *jpb_menu_p2_character_select_draw_user_data;
static JPBMenuPlatformHooks jpb_menu_platform_hooks;
static void *jpb_menu_platform_user_data;
static uint8_t jpb_menu_load_screen_active;

void menu_drawSelectBox(void);
void menu_drawSelectors(void);
void menu_fadeBG(void);
static void menu_drawSelector(float x, float y);
static void menu_nextMMV(MMVDEF *control);
static void menu_publishWinif2FontSpec(void);
static int menu_drawLevelSelectPsxTexture(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int color_index,
    float layer_depth);
static void menu_drawComboDebugList(void);
static void newMenu_VSSound(const char *cue);

/* Exact PDB globals owned by the matched menu presentation path. */
MENUVARS menuVars;
unsigned menuTexLoaded;
unsigned menuTexLoaded2;
unsigned loadvrmFlag;
_Material *menuTextures[249];
_Material *controlTextures[10];
_Material *kbmTextures[10];
_Material *ps4Textures[10];
_Material *ps5Textures[10];
_Material *switchTextures[10];
_Material *switchProTextures[10];
_Material *joyconTextures[10];
_Material *xsxTextures[10];
_Material *kbmForceTextures[4];
float iconScaleOverride = -1.0f;
unsigned char padShockable;
unsigned char modisorder2[23] = {
    0, 3, 8, 6, 7, 5, 15, 17, 18, 21, 26, 30,
    36, 37, 48, 49, 50, 51, 53, 79, 4, 1, 2
};
CVECTOR gColor;
float gLeft;
float gRight;
float gTop;
float gBottom;
SCREENRECT gDST;
unsigned screenSaverCount;
unsigned screenSaverFlag;
unsigned saverAlpha;
unsigned saverPads[2];
unsigned eulaMinScroll;
unsigned eulaMaxScroll;
unsigned eulaAcceptThreshold;
unsigned eulaCanAccept;
unsigned slider;
unsigned char keyboardBufferIndex;
unsigned char keyboardKeyPressed;
unsigned char keyboardBuffer[10];
/* Exact initialized and zero-initialized PDB globals used by the bespoke
 * Credits and Concept Art presentations. */
unsigned char credMusic[8] = {6, 9, 18, 21, 26, 95, 31, 34};
unsigned char credMuse;
int cachedInputL;
int cachedInputR;
float creditBarPosition;
int skipCreditForFrame;
int savedNumPlayer;
int m_canShowRegisterGame;
int tempPlayersVs;
int newMenu_errorState = 0x10;
int newMenu_trainLevel = 1;
int newMenu_currentModelSelectNGPP1 = pilot_model;
int newMenu_currentModelSelectBaseP2 = qui_gon_model;
int newMenu_currentModelSelectNGPP2 = rifle_model;
int newMenu_state;
int newMenu_bAbortMenu;
uint32_t newMenu_select;
int newMenu_playerSelectTypeP1;
int newMenu_currentModelSelectBaseP1 = obi_wan_model;
int newMenu_playerSelectTypeP2;
unsigned mp1ComboCount;
int cachedAwardIndex;
int16_t cachedRewardsInit[3];
int16_t cachedRewardsEnd[3];
int16_t cachedBonusLines[3];
unsigned scoreYtot;
int bonusOverride;

void jpb_MenuSetP1CharacterSelectDrawHook(
    JPBMenuP1CharacterSelectDrawHook hook,
    void *user_data)
{
    jpb_menu_p1_character_select_draw_hook = hook;
    jpb_menu_p1_character_select_draw_user_data = user_data;
}

void jpb_MenuSetP2CharacterSelectDrawHook(
    JPBMenuP2CharacterSelectDrawHook hook,
    void *user_data)
{
    jpb_menu_p2_character_select_draw_hook = hook;
    jpb_menu_p2_character_select_draw_user_data = user_data;
}

void jpb_MenuSetPlatformHooks(
    const JPBMenuPlatformHooks *hooks,
    void *user_data)
{
    if (hooks == NULL) {
        memset(&jpb_menu_platform_hooks, 0,
               sizeof(jpb_menu_platform_hooks));
        jpb_menu_platform_user_data = NULL;
        return;
    }
    jpb_menu_platform_hooks = *hooks;
    jpb_menu_platform_user_data = user_data;
}

/* 0xBEF00, 117 bytes, global, 2 named locals
 * menu_playerSelectCheck
 * PDB type: int (__int64)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_playerSelectCheck(int64_t menu_definition)
{
    uint32_t *md = (uint32_t *)(uintptr_t)menu_definition;
    uint32_t pad;

    if (GameStruct.NumPlayers != 2) {
        return 0;
    }
    pad = menuVars.pad[1];
    if ((pad & JPB_PAD_RIGHT) != 0 && md[0] == 9) {
        mmDecVar(md);
        pad = menuVars.pad[1];
    }
    if ((pad & JPB_PAD_LEFT) != 0 && md[0] == 9) {
        mmIncVar(md);
        pad = menuVars.pad[1];
    }
    if ((pad & JPB_PAD_COMBO_SOUTH) != 0) {
        (void)menu_handleMenuTriggers((int)md[3]);
        pad = menuVars.pad[1];
    }
    if ((pad & JPB_PAD_JUMP) != 0) {
        menu_menuExit();
    }
    return 1;
}

/* 0xBEF80, 21 bytes, global, 1 named locals
 * menu_healthCK
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_healthCK(int unused)
{
    (void)unused;
    return (int)(menuVars.awards[menuVars.aibit] & 1u);
}

/* 0xBEFA0, 23 bytes, global, 1 named locals
 * menu_forceCK
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_forceCK(int unused)
{
    (void)unused;
    return (int)((menuVars.awards[menuVars.aibit] >> 1) & 1u);
}

/* 0xBEFC0, 70 bytes, global, 2 named locals
 * menu_changeSaberCheck
 * PDB type: int (__int64)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_changeSaberCheck(int64_t unused)
{
    int player;

    (void)unused;
    for (player = 0; player < GameStruct.NumPlayers; ++player) {
        unsigned model = menuVars.pplayers[player];
        unsigned order = modisorder2[model];

        if ((order >= 2 && order <= 4) || order == 8) {
            return 1;
        }
    }
    return 0;
}

/* 0xBF010, 8 bytes, local, 0 named locals
 * menu_gameContinue
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\include\menudata.h
 */
int menu_gameContinue(void)
{
    return (int)GameStruct.continueAble;
}

/* 0xBF020, 608 bytes, global, 7 named locals
 * menu_scoreComboDraw
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_scoreComboDraw(void)
{
    Combo *combos;
    uint32_t saved_scaled_x;
    uint32_t next_y;
    unsigned combo;

    menuVars.selectp = menuVars.scoreCurrentPlayer == 0
        ? menuVars.mmSelect1
        : menuVars.mmSelect2;
    comboTotal[1] = menuVars.td.comboListCount;
    (void)mmDrawsub(comboTotal, 0);
    (void)mmDrawsub(comboType, 0);
    combos = gaPlayerData[menuVars.scoreCurrentPlayer].paCombos;
    if (menuVars.td.comboListCount != 0) {
        float x;
        float y;

        saved_scaled_x = (uint32_t)(int64_t)(
            (float)menuVars.mmX * gPSXDrawScaleX);
        next_y = (uint32_t)(int64_t)(
            (float)menuVars.mmY * gPSXDrawScaleY);
        menuVars.mmX = saved_scaled_x;
        menuVars.mmY = next_y;
        x = (float)saved_scaled_x;
        y = (float)next_y;
        setPivotPositionMM(&x, &y, 0);
        menuVars.mmX = (uint32_t)(int64_t)x;
        for (combo = 0;
             combo < menuVars.td.comboListCount;
             ++combo) {
            unsigned char combo_string[64];
            uint32_t definition[4];

            menuVars.mmY = (uint32_t)(
                (int32_t)menuVars.mmY - 1);
            next_y = (uint32_t)(int64_t)(
                (float)next_y +
                scaleAdjustmentMM * 60.0f - 1.0f);
            menu_buildComboString(
                combo_string,
                (unsigned char *)combos[
                    menuVars.td.newcombos[combo]].String,
                combo);
            menuVars.mmFlags |= 8u;
            menuVars.specialString = combo_string;
            definition[0] = 0x19;
            definition[1] = combo;
            definition[2] =
                (uint32_t)(uintptr_t)combo_string;
            definition[3] = 0x4d;
            (void)mmDrawsub(definition, 0);
        }
        menuVars.mmX = saved_scaled_x;
        menuVars.mmY = next_y;
    }
    return 1;
}

/* 0xBF280, 6 bytes, global, 0 named locals
 * menu_controlCK
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_controlCK(void)
{
    return 1;
}

/* 0xBF290, 13 bytes, global, 0 named locals
 * menu_ultimate
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_ultimate(void)
{
    return (int)((secretBits >> 8) & 1u);
}

/* 0xBF2A0, 13 bytes, global, 0 named locals
 * menu_concept
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_concept(void)
{
    return (int)((secretBits >> 7) & 1u);
}

/* 0xBF2B0, 11 bytes, global, 0 named locals
 * SetGlobalColorDefault
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void SetGlobalColorDefault(void)
{
    gColor.r = 255;
    gColor.g = 255;
    gColor.b = 255;
    gColor.cd = 255;
}

/* 0xBF2C0, 57 bytes, global, 0 named locals
 * SetGlobalDST
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void SetGlobalDST(void)
{
    gDST.left = (int32_t)gLeft;
    gDST.right = (int32_t)gRight;
    gDST.top = (int32_t)gTop;
    gDST.bottom = (int32_t)gBottom;
}

/* 0xBF300, 229 bytes, global, 7 named locals
 * cheatCheck
 * PDB type: int (unsigned short*, unsigned, ...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

int cheatCheck(
    unsigned short *cheatList,
    unsigned cheatSize,
    JPBMenuCheatAction cheatJump)
{
    unsigned player;
    int flag1 = 0;
    uint16_t *tail = NULL;

    for (player = 0; player < 2; ++player) {
        uint16_t *history = player == 0
            ? menuVars.frKeyBuff
            : menuVars.frKeyBuff2;
        unsigned loop1;
        int flag2 = 1;

        tail = history + (16u - cheatSize);
        for (loop1 = 0; loop1 < cheatSize; ++loop1) {
            flag2 = flag2 && tail[loop1] == cheatList[loop1];
        }
        if (cheatSize == 0 || flag2) {
            flag1 = 1;
        }
    }
    if (flag1) {
        cheatJump();
        /*
         * Retail clears four qwords from the second player's comparison
         * pointer after both searches. All shipped callers use a 16-entry
         * sequence, making this exactly the second 16-sample history.
         */
        memset((uint8_t *)(void *)tail, 0, 32);
    }
    return flag1;
}

/* 0xBF3F0, 196 bytes, global, 4 named locals
 * cheatCheckKeyboard
 * PDB type: int (unsigned char*, int, void*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

int cheatCheckKeyboard(
    unsigned char *stateList,
    int cheatLength,
    JPBMenuCheatAction cheatJump)
{
    int offsetIndex;
    int index;

    for (index = 0; index < cheatLength; ++index) {
        offsetIndex =
            (int)keyboardBufferIndex - cheatLength + index;
        while (offsetIndex < 0) {
            offsetIndex += 10;
        }
        if (offsetIndex >= 10) {
            offsetIndex %= 10;
        }
        if (keyboardBuffer[offsetIndex] != stateList[index]) {
            return 0;
        }
    }
    memset(keyboardBuffer, 0, sizeof(keyboardBuffer));
    cheatJump();
    return 1;
}

/* 0xBF4C0, 154 bytes, global, 4 named locals
 * checkKeyboardBuffer
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void checkKeyboardBuffer(void)
{
    const uint8_t *keyState = NULL;
    size_t key_count = 0;
    unsigned i;
    int keyWasPressed = 0;

    if (jpb_menu_platform_hooks.keyboardState != NULL) {
        keyState = jpb_menu_platform_hooks.keyboardState(
            &key_count,
            jpb_menu_platform_user_data);
    }
    if (keyState != NULL) {
        if (key_count > 512u) {
            key_count = 512u;
        }
        for (i = 0; i < key_count; ++i) {
            if (keyState[i] != 0) {
                keyWasPressed = 1;
                if (keyboardKeyPressed == 0) {
                    keyboardBuffer[keyboardBufferIndex] =
                        (unsigned char)i;
                    keyboardBufferIndex = (unsigned char)(
                        (keyboardBufferIndex + 1u) % 10u);
                }
            }
        }
    }
    if (keyWasPressed) {
        if (keyboardKeyPressed == 0) {
            keyboardKeyPressed = 1;
        }
    } else {
        keyboardKeyPressed = 0;
    }
}

/* 0xBF560, 58 bytes, global, 0 named locals
 * clearMenuStuff
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void clearMenuStuff(void)
{
    GameStruct.inMenuFlag = 0;
    memset(menuVars.mmSelect1, 0, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 0, sizeof(menuVars.mmSelect2));
    memset(menuVars.menuMode, 0x32, 8);
    memset(menuVars.frKeyBuff, 0, sizeof(menuVars.frKeyBuff));
}

/* 0xBF5A0, 57 bytes, global, 4 named locals
 * comboSubset
 * PDB type: unsigned (unsigned char*, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
unsigned comboSubset(unsigned char *source, unsigned char *candidate)
{
    unsigned index = 0;
    unsigned matches;

    do {
        matches = source[index] == candidate[index];
        ++index;
        if (matches == 0) {
            return 0;
        }
    } while (source[index] != 0);
    return matches;
}

/* 0xBF5E0, 386 bytes, global, 4 named locals
 * drawControlsIcon
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void drawControlsIcon(void)
{
    float exit_x = 100.0f;
    float exit_y = 100.0f;
    float select_x = 100.0f;
    float select_y = 100.0f;
    unsigned exit_text;
    unsigned select_text;

    if (p1Disconnected != 0 || p2Disconnected != 0) {
        return;
    }
    setPivotPositionMM(&exit_x, &exit_y, 6);
    setPivotPositionMM(&select_x, &select_y, 8);
    exit_text = lastUsedInputType == 0 ? 476u : 239u;
    select_text = lastUsedInputType == 0 ? 475u : 241u;
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)exit_x, (int)exit_y,
        2.25f, 0, "%s", allText[exit_text]);
    (void)SDLTextWriteScaleMM(
        15, 255, 1, (int)select_x, (int)select_y,
        2.25f, 0, "%s", allText[select_text]);
}

static void menu_drawLevelSelectPromptText(void)
{
    float exit_text_x;
    float select_text_x;
    float exit_text_y = 246.0f;
    float select_text_y = 193.0f;
    unsigned exit_text;
    unsigned select_text;

    if (OptionStruct.Language == 0 || OptionStruct.Language == 6) {
        exit_text_x = 607.5f;
        select_text_x = 607.5f;
    } else {
        exit_text_x = 555.0f;
        select_text_x = 555.0f;
    }
    setPivotPositionMM(&exit_text_x, &exit_text_y, 7);
    setPivotPositionMM(&select_text_x, &select_text_y, 7);
    exit_text = lastUsedInputType == 0 ? 476u : 239u;
    select_text = lastUsedInputType == 0 ? 475u : 241u;
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)exit_text_x, (int)exit_text_y,
        1.75f, 0, "%s", allText[exit_text]);
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)select_text_x, (int)select_text_y,
        1.75f, 0, "%s", allText[select_text]);
}

static void menu_drawLevelSelectPromptGlyphs(void)
{
    (void)menu_drawLevelSelectPsxTexture(
        0x113, 389.0f, 21.0f,
        scaleAdjustmentMM * 188.0f, 0.0f, 0x8000u, 0x0b, 0.8f);
    (void)menu_drawLevelSelectPsxTexture(
        0x114, 469.0f, 61.0f,
        scaleAdjustmentMM * 117.0f, 0.0f, 0x8000u, 0x0b, 0.8f);
}

/* 0xBF770, 350 bytes, global, 4 named locals
 * drawControlsIconTraining
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xBF8D0, 956 bytes, global, 8 named locals
 * drawDropForceMess
 * PDB type: unsigned (int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
unsigned drawDropForceMess(unsigned type)
{
    unsigned line = 0;
    unsigned y_offset = 0;
    unsigned bit_index;

    (void)type;
    for (bit_index = 0; bit_index < 10; ++bit_index) {
        uint16_t bit = bonusMessBits[bit_index];
        const char *message = NULL;

        if ((menuVars.awards[menuVars.scoreCurrentPlayer] & bit) == 0) {
            continue;
        }
        switch (bit) {
        case 0x0010:
        case 0x0080:
        case 0x0200:
            message = allText[364];
            break;
        case 0x0020:
        case 0x0100:
        case 0x0400:
            message = allText[365];
            break;
        case 0x0040:
            message = allText[366];
            break;
        case 0x1000:
            message =
                menuVars.mmSelect1[menuVars.menuModeSP] == line &&
                bonusOverride == 0
                    ? "  <F>   <X>"
                    : "<f>   <x>";
            break;
        case 0x2000:
            if (OptionStruct.ControllerConfig[0] == 1 &&
                lastUsedInputType != 0) {
                message =
                    menuVars.mmSelect1[menuVars.menuModeSP] == line &&
                    bonusOverride == 0
                        ? "  <F>   <A>"
                        : "<f>   <a>";
            } else {
                message =
                    menuVars.mmSelect1[menuVars.menuModeSP] == line &&
                    bonusOverride == 0
                        ? "  <F>   <B>"
                        : "<f>   <b>";
            }
            break;
        case 0x4000:
            if (OptionStruct.ControllerConfig[0] == 1 &&
                lastUsedInputType != 0) {
                message =
                    menuVars.mmSelect1[menuVars.menuModeSP] == line &&
                    bonusOverride == 0
                        ? "  <F>   <Y>"
                        : "<f>   <y>";
            } else {
                message =
                    menuVars.mmSelect1[menuVars.menuModeSP] == line &&
                    bonusOverride == 0
                        ? "  <F>   <A>"
                        : "<f>   <a>";
            }
            break;
        case 0x8000:
            message = allText[367];
            break;
        default:
            break;
        }

        if (message != NULL) {
            int32_t scaled_x = (int32_t)(
                (float)menuVars.mmX * gPSXDrawScaleX);
            int32_t scaled_y = (int32_t)(
                (float)menuVars.mmY * gPSXDrawScaleY);
            float x = (float)scaled_x + 10.0f;
            float y = (float)((uint32_t)scaled_y + y_offset);
            int selected =
                menuVars.mmSelect1[menuVars.menuModeSP] == line &&
                bonusOverride == 0;
            float depth = menuTextDepthOverride > 0.0f
                ? menuTextDepthOverride
                : 0.0f;

            menuVars.mmX = (uint32_t)scaled_x;
            menuVars.mmY = (uint32_t)scaled_y;
            setPivotPositionMM(&x, &y, 0);
            menuVars.mmX = (uint32_t)(int32_t)x;
            menuVars.mmY = (uint32_t)(int32_t)y;
            (void)SDLTextWriteScaleMMDepth(
                selected ? 14 : 15, 255, 0,
                (int)x, (int)y, 2.5f, 0, depth,
                bonusOverride == 0 ? "> %s <" : "%s", message);
            ++line;
        }
        y_offset += 60;
    }
    return line;
}

static MPNT *menu_scorePositionView(
    unsigned current_award,
    MPNT *sentinel_position,
    unsigned char **sentinel_backing)
{
    if (current_award < 3u) {
        *sentinel_backing = NULL;
        return &menuVars.mp[current_award];
    }
    if (current_award != 4u) {
        *sentinel_backing = NULL;
        return NULL;
    }

    /* Retail intentionally indexes MPNT[4], whose bytes begin at
     * maxAwardScore[1]. Preserve that layout-defined view without C UB. */
    *sentinel_backing =
        (unsigned char *)&menuVars.mp[0] + 4u * sizeof(MPNT);
    memcpy(sentinel_position, *sentinel_backing, sizeof(*sentinel_position));
    return sentinel_position;
}

/* 0xBFC90, 1230 bytes, global, 13 named locals
 * drawScoreMenus
 * PDB type: void (int, AWARDSET*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void drawScoreMenus(unsigned current_award, AWARDSET *award_set)
{
    static const unsigned reward_text[3] = {360u, 359u, 369u};
    static const float reward_top[3] = {495.0f, 382.5f, 270.0f};
    MPNT sentinel_position;
    unsigned char *sentinel_backing;
    MPNT *position = menu_scorePositionView(
        current_award, &sentinel_position, &sentinel_backing);
    unsigned award;

    if (position == NULL) {
        return;
    }
    if (position->state == 1u) {
        position->x = (int16_t)(position->x + 16);
        if (position->x > 33) {
            position->x = 34;
            position->state = 2;
        }
        if (sentinel_backing != NULL) {
            memcpy(sentinel_backing, position, sizeof(*position));
        }
    }

    for (award = 0; award < 3; ++award) {
        float text_x;
        float text_y;
        float left;
        float top;
        float right;
        float bottom;
        float clip_left = 0.0f;
        float clip_top = 0.0f;
        float clip_right = 0.0f;
        float clip_bottom = 0.0f;
        SCREENRECT destination;
        SCREENRECT scissor;
        CVECTOR color = {225, 225, 225, 255};
        int use_reward_position = 0;

        if (cachedRewardsEnd[award] == 0) {
            continue;
        }

        text_x = 150.0f;
        text_y = award == 0u ? 555.0f :
            (award == 1u ? 670.0f : 780.0f);
        setPivotPositionMM(&text_x, &text_y, 6);
        (void)SDLTextWriteScaleMMDepth(
            11, 255, 0, (int)text_x, (int)text_y,
            2.5f, 0, 0.4f - (float)award * 0.001f,
            "%s", allText[reward_text[award_set->awardType[award]]]);

        left = (float)position->x * gPSXDrawScaleX - 1.0f;
        top = (float)position->y * gPSXDrawScaleY;
        if (position->state == 0u) {
            use_reward_position = cachedRewardsEnd[award] != 0;
        } else if (award == 0u) {
            use_reward_position = cachedRewardsEnd[0] != 0 &&
                (cachedRewardsEnd[1] != 0 || cachedRewardsEnd[2] != 0);
        } else if (award == 1u) {
            use_reward_position = cachedRewardsEnd[0] != 0 &&
                cachedRewardsEnd[2] != 0;
        } else {
            use_reward_position = cachedRewardsEnd[2] != 0 &&
                menuVars.scoreMode == 3u;
        }
        if (use_reward_position != 0) {
            left = 126.5f;
            top = reward_top[award];
        }

        right = left + 847.0f;
        bottom = top + 106.0f;
        setPivotPositionMM(&left, &top, 0);
        setPivotPositionMM(&right, &bottom, 0);
        destination.left = (int32_t)left;
        destination.top = (int32_t)top;
        destination.right = (int32_t)right;
        destination.bottom = (int32_t)bottom;

        setPivotPositionMM(&clip_left, &clip_top, 0);
        setPivotPositionMM(&clip_right, &clip_bottom, 8);
        scissor.left = (int32_t)clip_left;
        scissor.top = (int32_t)clip_top;
        scissor.right = (int32_t)clip_right;
        scissor.bottom = (int32_t)clip_bottom;
        _DrawTextureClipped(
            menuTextures[238], destination, NULL, color,
            0.5f - (float)award * 0.001f, scissor);
    }
}

/* 0xC0160, 182 bytes, global, 5 named locals
 * fixPSPos
 * PDB type: void (float*, float*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void fixPSPos(float *x, float *y)
{
    const float aspect = 1.7777778f;
    float screen_width = (float)OptionStruct.ScreenWidth;
    float screen_height = (float)OptionStruct.ScreenHeight;
    float current_aspect = screen_width / screen_height;
    float x_offset = 0.0f;
    float y_offset = 0.0f;

    if (current_aspect > aspect) {
        screen_width /= gPSXDrawScaleX;
        x_offset = (current_aspect - aspect) / current_aspect;
        x_offset *= 0.5f;
        x_offset *= screen_width;
        *x /= current_aspect;
        *x *= aspect;
        *x += x_offset;
        *y += y_offset;
        return;
    }
    screen_height /= gPSXDrawScaleY;
    y_offset = (aspect - current_aspect) / aspect;
    *y /= aspect;
    *y *= current_aspect;
    y_offset *= 0.5f;
    *x += x_offset;
    y_offset *= screen_height;
    *y += y_offset;
}

/* 0xC0220, 302 bytes, global, 8 named locals
 * genComboStrings
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void genComboStrings(unsigned player)
{
    Combo *combos = gaPlayerData[player].paCombos;
    unsigned combo_count = 0;
    unsigned combo;

    mp1ComboCount = 0;
    for (combo = 0;
         combo < (unsigned)gaPlayerData[player].maxCombos;
         ++combo) {
        if (combos[combo].String[0] != '\0') {
            ++combo_count;
            menuVars.td.comboList[combo * 2u] = (uint8_t)combo;
            mp1ComboCount = combo_count;
        }
    }
    for (combo = 0; combo < combo_count; ++combo) {
        menuVars.td.comboList[combo * 2u + 1u] = 0;
    }
    for (combo = 0; combo < combo_count; ++combo) {
        unsigned other;

        for (other = 0; other < combo_count; ++other) {
            unsigned character = 0;

            if (combo == other) {
                continue;
            }
            for (;;) {
                if (combos[combo].String[character] !=
                    combos[other].String[character]) {
                    break;
                }
                ++character;
                if (combos[combo].String[character] == '\0') {
                    ++menuVars.td.comboList[other * 2u + 1u];
                    break;
                }
            }
        }
    }
}

/* 0xC0350, 1173 bytes, global, 6 named locals
 * getControllerTextures
 * PDB type: int (int, _Material**)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void drawControlsIconTraining(void)
{
    float exit_x = 75.0f;
    float exit_y = 100.0f;
    float select_x = 75.0f;
    float select_y = 175.0f;
    unsigned exit_text = lastUsedInputType == 0 ? 476u : 239u;
    unsigned select_text = lastUsedInputType == 0 ? 475u : 241u;

    setPivotPositionMM(&exit_x, &exit_y, 6);
    setPivotPositionMM(&select_x, &select_y, 8);
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)exit_x, (int)exit_y,
        2.25f, 0, "%s", allText[exit_text]);
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)select_x, (int)select_y,
        2.25f, 0, "%s", allText[select_text]);
}
int getControllerTextures(int player, _Material **materialHandle)
{
    const char *controller_name = NULL;
    _Material **source = controlTextures;

    if (materialHandle == NULL || player < 0 || player > 1) {
        return 0;
    }
    if (player == 0 && lastUsedInputType == 0) {
        memcpy(materialHandle, kbmTextures, sizeof(kbmTextures));
        return 1;
    }
    if (jpb_menu_platform_hooks.controllerName != NULL) {
        controller_name = jpb_menu_platform_hooks.controllerName(
            (unsigned)player, jpb_menu_platform_user_data);
    }
    if (controller_name == NULL) {
        source = kbmTextures;
    } else if (strcmp(controller_name, "PS5 Controller") == 0) {
        source = ps5Textures;
    } else if (strcmp(controller_name, "PS4 Controller") == 0) {
        source = ps4Textures;
    } else if (strcmp(
                   controller_name,
                   "Nintendo Switch Pro Controller") == 0) {
        source = switchProTextures;
    } else if (strcmp(
                   controller_name,
                   "Nintendo Switch Joy-Con (L/R)") == 0) {
        source = joyconTextures;
    } else if (strcmp(
                   controller_name,
                   "Xbox Series X Controller") == 0) {
        source = xsxTextures;
    }
    memcpy(materialHandle, source, sizeof(controlTextures));
    return 1;
}

/* 0xC07F0, 3 bytes, global, 0 named locals
 * initSaveMenu
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void initSaveMenu(void)
{
}

/* 0xC0800, 1553 bytes, global, 8 named locals
 * initsavegamestruct
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void initsavegamestruct(void)
{
    unsigned slot = menuVars.cardSlotSelect;
    saveGameStruct *save = &cardLoadBuffer.saveGame[slot];

    cardLoadBuffer.header.OptionStruct = OptionStruct;
    save->validFlag = 1;
    save->gamenum = (uint8_t)slot;
    save->lastlevel = GameStruct.CurrentLevel;

    /* The shipped function passes 23, not sizeof(jediLevel), to memset. */
    memset(save->jediLevel, 0, JPB_GAME_JEDI_MODEL_CAPACITY);
    memcpy(
        save->abGlobalBits,
        abGlobalBits,
        sizeof(save->abGlobalBits));
    memcpy(
        save->jediUpgrades,
        jediUpgrades,
        sizeof(save->jediUpgrades));
    memcpy(
        save->maxEnergyLevels,
        GameStruct.maxEnergyLevels,
        sizeof(save->maxEnergyLevels));
    memcpy(
        save->maxEnergyLineLength,
        GameStruct.maxEnergyLineLength,
        sizeof(save->maxEnergyLineLength));
    memcpy(
        save->maxForceLevels,
        GameStruct.maxForceLevels,
        sizeof(save->maxForceLevels));
    memcpy(
        save->maxForceLineLength,
        GameStruct.maxForceLineLength,
        sizeof(save->maxForceLineLength));
    save->NumPlayers = GameStruct.NumPlayers;
    save->mNumContinues = GameStruct.mNumContinues;
    save->ContinuesUsed = GameStruct.ContinuesUsed;
    memcpy(
        save->aCharacterData,
        GameStruct.aCharacterData,
        sizeof(save->aCharacterData));
    memcpy(
        save->jediComboMask,
        GameStruct.jediComboMask,
        sizeof(save->jediComboMask));
    memcpy(
        save->jediScorePerLevel,
        GameStruct.jediScorePerLevel,
        sizeof(save->jediScorePerLevel));
    save->secretBits = secretBits;
    memcpy(
        save->checkpoint,
        GameStruct.checkpoint,
        sizeof(save->checkpoint));
    save->AIDamage = GameStruct.AIDamage;
    save->JediDamage = GameStruct.JediDamage;
    save->HTHRate = GameStruct.HTHRate;
    save->RangedRate = GameStruct.RangedRate;
    save->BlockRate = GameStruct.BlockRate;
    save->ComboLevel = GameStruct.ComboLevel;
    save->ForceLevel = GameStruct.ForceLevel;
    save->players[0] = menuVars.pplayers[0];
    save->players[1] = menuVars.pplayers[1];
    memcpy(
        save->jediLevelPlayed,
        GameStruct.jediLevelPlayed,
        sizeof(save->jediLevelPlayed));
    save->completionPoints = menu_calcCompletionPoints();
}

/* 0xC0E20, 231 bytes, global, 8 named locals
 * loadVRM
 * PDB type: void (char*, unsigned, unsigned,...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void loadVRM(
    char *file,
    unsigned indoxo,
    unsigned height,
    unsigned char *dst,
    unsigned winTexIndex)
{
    uint32_t record_bytes;
    uint32_t tpage_count;

    (void)height;
    if (loadvrmFlag != 0) {
        return;
    }

    (void)file_LoadFile(
        (char *)resource_getPath(file, JPB_RESOURCE_FRONT), dst);
    memcpy(&record_bytes, dst, sizeof(record_bytes));
    memcpy(
        &tpage_count,
        dst + record_bytes + 8,
        sizeof(tpage_count));
    while (tpage_count != 0) {
        uint16_t loop1 = (uint16_t)indoxo;
        unsigned end = indoxo + (uint16_t)(record_bytes >> 3);
        unsigned record = 0;

        --tpage_count;
        while ((unsigned)loop1 < end) {
            const unsigned char *entry = dst + 4 + record * 8;
            FONTSPEC *spec = &fontSpec[loop1];

            memcpy(&spec->xypage, entry, sizeof(spec->xypage));
            spec->clut = (uint16_t)winTexIndex;
            spec->y = entry[4];
            spec->x = entry[5];
            spec->h = entry[6];
            spec->w = entry[7];
            ++loop1;
            ++record;
        }
    }
}

/* 0xC0F10, 103 bytes, global, 0 named locals
 * menuBucketFront
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menuBucketFront(void)
{
    texture_Flush(0xf90u);
    loadvrmFlag = 0;
    menuTexLoaded2 = 0;
    texture_Flush(0xf90u);
    menuTexLoaded2 = 0;
    texture_Flush(0xf90u);
    texture_Flush(0xf90u);
    menuTexLoaded2 = 0;
    texture_Flush(0xf90u);
    menuTexLoaded2 = 0;
    texture_Flush(0x100u);
}

/* 0xC0F80, 93 bytes, global, 0 named locals
 * menuBucketSavegame
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menuBucketSavegame(void)
{
    texture_Flush(0xf90u);
    loadvrmFlag = 0;
    menuTexLoaded2 = 0;
    texture_Flush(0xf90u);
    menuTexLoaded2 = 0;
    texture_Flush(0xf90u);
    menuTexLoaded2 = 0;
    texture_Flush(0xf90u);
    menuTexLoaded2 = 0;
    texture_Flush(0x100u);
}

/* 0xC0FE0, 1149 bytes, global, 17 named locals
 * menuConceptMenu
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menuConceptMenu(void)
{
    uint32_t input;
    SCREENRECT destination;
    CVECTOR color = {255, 255, 255, 255};
    char page_text[24];
    float x;
    float y;
    float right_x;
    float right_y;
    float content_width;
    float content_height;

    if (p1Disconnected != 0) {
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        menu_drawReconnect();
        return;
    }

    if (menuVars.scoreScore == 0) {
        menuVars.scoreScore = 1;
    }
    input = menuVars.pad[0] | menuVars.pad[1];
    if ((input & JPB_PAD_RIGHT) != 0) {
        --menuVars.scoreScore;
    }
    if ((input & JPB_PAD_LEFT) != 0) {
        ++menuVars.scoreScore;
    }
    if (menuVars.scoreScore == 0) {
        menuVars.scoreScore = JPB_CONCEPT_ART_PAGE_COUNT;
    } else if (menuVars.scoreScore > JPB_CONCEPT_ART_PAGE_COUNT) {
        menuVars.scoreScore = 1;
    }

    x = 78.0f;
    y = 104.0f;
    setPivotPositionMM(&x, &y, 7);
    (void)snprintf(
        page_text,
        sizeof(page_text),
        menuVars.scoreScore < 10 ? "0%u / %u" : "%u / %u",
        menuVars.scoreScore,
        (unsigned)JPB_CONCEPT_ART_PAGE_COUNT);
    (void)SDLTextWriteScaleMM(
        9, 255, 1, (int)x, (int)y,
        2.25f, 0, "%s", page_text);

    x = -140.0f;
    y = 108.0f;
    right_x = 90.0f;
    right_y = 108.0f;
    setPivotPositionMM(&x, &y, 7);
    setPivotPositionMM(&right_x, &right_y, 7);
    destination.left = (int32_t)x;
    destination.top = (int32_t)y;
    destination.right = (int32_t)(x + scaleAdjustmentMM * 50.0f);
    destination.bottom = (int32_t)(y + scaleAdjustmentMM * 54.0f);
    _DrawTexture(
        menuTextures[
            (input & JPB_PAD_RIGHT) != 0 || cachedInputL > 0
                ? 193
                : 197],
        destination, NULL, color, 0.1f);
    destination.left = (int32_t)right_x;
    destination.top = (int32_t)right_y;
    destination.right = (int32_t)(
        right_x + scaleAdjustmentMM * 50.0f);
    destination.bottom = (int32_t)(
        right_y + scaleAdjustmentMM * 54.0f);
    _DrawTexture(
        menuTextures[
            (input & JPB_PAD_LEFT) != 0 || cachedInputR > 0
                ? 194
                : 198],
        destination, NULL, color, 0.1f);
    --cachedInputL;
    --cachedInputR;
    if ((input & JPB_PAD_RIGHT) != 0) {
        cachedInputL = 3;
    }
    if ((input & JPB_PAD_LEFT) != 0) {
        cachedInputR = 3;
    }

    x = 100.0f;
    y = 100.0f;
    setPivotPositionMM(&x, &y, 6);
    (void)SDLTextWriteScaleMM(
        9, 255, 0, (int)x, (int)y,
        2.25f, 0, "%s",
        allText[lastUsedInputType == 0 ? 476u : 239u]);

    menuVars.titleDispEnable = 0;
    menuVars.titleArt = 0;
    content_width = (float)OptionStruct.ScreenWidth;
    content_height = (float)OptionStruct.ScreenHeight;
    if (1.7777778f <= content_width / content_height) {
        float fitted_width = content_height * 1.7777778f;

        destination.left = (int32_t)((content_width - fitted_width) * 0.5f);
        destination.top = 0;
        destination.right = (int32_t)(destination.left + fitted_width);
        destination.bottom = (int32_t)content_height;
    } else {
        float fitted_height = content_width / 1.7777778f;

        destination.left = 0;
        destination.top = (int32_t)((content_height - fitted_height) * 0.5f);
        destination.right = (int32_t)content_width;
        destination.bottom = (int32_t)(destination.top + fitted_height);
    }
    _DrawTexture(
        menuTextures[menuVars.scoreScore + 120u],
        destination, NULL, color, 1.0f);

    if ((input & JPB_PAD_JUMP) != 0) {
        menu_menuExit();
    }
}

/* 0xC1460, 3 bytes, global, 3 named locals
 * menuLoadSelectTextures
 * PDB type: void (unsigned short*, unsigned ...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menuLoadSelectTextures(
    unsigned short *load_list,
    unsigned short load_list_size,
    unsigned texture_type)
{
    (void)load_list;
    (void)load_list_size;
    (void)texture_type;
}

/* 0xC1470, 167 bytes, global, 3 named locals
 * menuPreString
 * PDB type: void (unsigned char*, unsigned c...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menuPreString(unsigned char *src1, unsigned char *src2)
{
    unsigned char src3[256];

    (void)strcpy((char *)src3, (const char *)src2);
    (void)strcat((char *)src3, (const char *)src1);
    (void)strcpy((char *)src1, (const char *)src3);
}

/* 0xC1520, 158 bytes, global, 1 named locals
 * menuPushKey
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menuPushKey(void)
{
    if (menuVars.pad[0] != 0) {
        memmove(
            menuVars.frKeyBuff,
            menuVars.frKeyBuff + 1,
            15u * sizeof(menuVars.frKeyBuff[0]));
        menuVars.frKeyBuff[15] =
            (uint16_t)(menuVars.pad[0] & UINT32_C(0xfdff));
    }
    if (menuVars.pad[1] != 0) {
        memmove(
            menuVars.frKeyBuff2,
            menuVars.frKeyBuff2 + 1,
            15u * sizeof(menuVars.frKeyBuff2[0]));
        menuVars.frKeyBuff2[15] =
            (uint16_t)(menuVars.pad[1] & UINT32_C(0xfdff));
    }
}

/* 0xC15C0, 5 bytes, global, 2 named locals
 * menu_CheckValidLevel
 * PDB type: int (unsigned, unsigned*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_CheckValidLevel(unsigned check, unsigned *award_level)
{
    return jedi_CheckValidLevel((int)check, (int *)award_level);
}

/* 0xC15D0, 28 bytes, global, 0 named locals
 * menu_ClearScreenSaver
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_ClearScreenSaver(void)
{
    screenSaverFlag = 0;
    screenSaverCount = 0;
    saverAlpha = 0;
    menuVars.mcount = 0;
}

/* 0xC15F0, 396 bytes, global, 3 named locals
 * menu_DisplayMessage
 * PDB type: void (char*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_DisplayMessage(char *message)
{
    menuVars.titleDispEnable = 0;
    for (;;) {
        __StartRender();
        (void)SDLTextWrite(11, 0, 20, 20, 0, message);
        menu_dumpMemory(20u, 44u);
        __EndRender();
    }
}

/* 0xC1780, 192 bytes, global, 5 named locals
 * menu_DrawArrows
 * PDB type: void (unsigned long, int, int, i...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_DrawArrows(
    unsigned pad,
    int left_x,
    int left_y,
    int right_x,
    int right_y)
{
    unsigned left_texture = 0x198u;
    unsigned right_texture = 0x8198u;

    if ((pad & JPB_PAD_LEFT) != 0) {
        right_texture = 0x8197u;
    } else if ((pad & JPB_PAD_RIGHT) != 0) {
        left_texture = 0x197u;
    }
    (void)winDrawTexture(
        right_texture, right_x, right_y, 50, 50,
        0xff, 0xff, 0xff, 0xff);
    (void)winDrawTexture(
        left_texture, left_x, left_y, 50, 50,
        0xff, 0xff, 0xff, 0xff);
}

/* 0xC1840, 885 bytes, global, 6 named locals
 * menu_DrawOnePlayer
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_DrawOnePlayer(void)
{
    int player;
    char *extra_name;
    int level;
    int skill;
    int extra_texture;

    SDL_ResetClipRect();
    if (GameStruct.gameMode != 7) {
        if (GameStruct.gameMode != 6) {
            winDrawBackground(8);
        }
        if ((uint8_t)(GameStruct.gameMode - 6u) > 1u) {
            winDrawBackground(11);
        }
    }
    player = GameStruct.ModelSelect[0];
    drawControlsIcon();
    menu_DrawArrows(menuVars.pad[0], 116, 208, 154, 208);
    if (newMenu_GetVSExtraPlayer(
            &extra_name, &extra_texture, player) != 0) {
        (void)psxDrawTexture(
            (unsigned)extra_texture, 257.0f, 111.0f,
            0.0f, 0.0f, 255u, 128, 128, 128);
        (void)SDLTextWriteScaleMM(
            11, 255, 0, 20, 70, 0.9f, 0,
            "%s", extra_name);
    } else {
        (void)psxDrawTexture(
            (unsigned)(player + 0x17e), 257.0f, 111.0f,
            0.0f, 0.0f, 255u, 128, 128, 128);
        (void)SDLTextWriteScaleMM(
            11, 255, 0, 20, 70, 0.9f, 0,
            "%s", allText[player + 332]);
    }
    if ((menuVars.jediDebugCombo & 1u) != 0) {
        (void)psxDrawTexture(
            0x16fu, 257.0f, 111.0f,
            0.0f, 0.0f, 255u, 128, 128, 128);
    }
    (void)psxDrawTexture(
        0x161u, 0.0f, 4.0f,
        0.0f, 0.0f, 255u, 128, 128, 128);
    jedi_CalcSkillLevels(player, &skill, &level);
    (void)SDLTextWriteScaleMM(
        11, 255, 0, 20, 70, 0.9f, 0,
        "%s", allText[player + 332]);
    (void)SDLTextWriteScaleMM(
        11, 255, 0, 100, 105, 0.6f, 0,
        "Level : %d", level);
    (void)SDLTextWriteScaleMM(
        11, 255, 0, 100, 125, 0.6f, 0,
        "Skill : %d", skill);
    (void)SDLTextWriteScaleMM(
        11, 255, 0, 20, 145, 0.55f, 0,
        "%s", "player1");
    PresentWindow();
}

/* 0xC1BC0, 1346 bytes, global, 9 named locals
 * menu_DrawTwoPlayer
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_DrawTwoPlayer(void)
{
    int player1 = GameStruct.ModelSelect[0];
    int player2 = GameStruct.ModelSelect[1];
    char *extra_name;
    int level;
    int skill;
    int extra_texture;
    unsigned long old_pad;

    drawControlsIcon();
    old_pad = menuVars.pad[0];
    if ((menuVars.jediDebugCombo & 1u) != 0) {
        old_pad &= 0xffff5fffu;
    }
    menu_DrawArrows(old_pad, 283, 155, 321, 155);
    old_pad = menuVars.pad[1];
    if ((menuVars.jediDebugCombo & 2u) != 0) {
        old_pad &= 0xffff5fffu;
    }
    menu_DrawArrows(old_pad, 287, 283, 325, 283);

    if (newMenu_GetVSExtraPlayer(
            &extra_name, &extra_texture, player1) != 0) {
        (void)psxDrawTexture(
            (unsigned)extra_texture, 7.0f, 32.0f,
            0.0f, 0.0f, 255u, 128, 128, 128);
        (void)SDLTextWriteScaleMM(
            11, 255, 0, 255, 40, 0.9f, 0,
            "%s", extra_name);
        (void)SDLTextWriteScaleMM(
            11, 255, 0, 415, 115, 0.55f, 0,
            "player1");
    } else if ((unsigned)player1 < 23u) {
        (void)psxDrawTexture(
            (unsigned)(player1 + 0x17e), 7.0f, 32.0f,
            0.0f, 0.0f, 255u, 128, 128, 128);
        (void)SDLTextWriteScaleMM(
            11, 255, 0, 255, 40, 0.9f, 0,
            "%s", allText[player1 + 332]);
        (void)SDLTextWriteScaleMM(
            11, 255, 0, 415, 115, 0.55f, 0,
            "player1");
    }

    if (newMenu_GetVSExtraPlayer(
            &extra_name, &extra_texture, player2) != 0) {
        (void)psxDrawTexture(
            (unsigned)extra_texture, 457.0f, 178.0f,
            0.0f, 0.0f, 255u, 128, 128, 128);
        (void)SDLTextWriteScaleMM(
            11, 255, 0, 165, 353, 0.55f, 0,
            "player2");
        (void)SDLTextWriteScaleMM(
            11, 255, 0, 165, 413, 0.9f, 0,
            "%s", extra_name);
    } else if ((unsigned)player2 < 23u) {
        (void)psxDrawTexture(
            (unsigned)(player2 + 0x17e), 457.0f, 178.0f,
            0.0f, 0.0f, 255u, 128, 128, 128);
        (void)SDLTextWriteScaleMM(
            11, 255, 0, 165, 353, 0.55f, 0,
            "player2");
        (void)SDLTextWriteScaleMM(
            11, 255, 0, 165, 413, 0.9f, 0,
            "%s", allText[player2 + 332]);
    }

    jedi_CalcSkillLevels(player1, &skill, &level);
    (void)SDLTextWriteScaleMM(
        11, 255, 0, 300, 75, 0.6f, 0,
        "Level : %d", level);
    (void)SDLTextWriteScaleMM(
        11, 255, 0, 300, 95, 0.6f, 0,
        "Skill : %d", skill);
    jedi_CalcSkillLevels(player2, &skill, &level);
    (void)SDLTextWriteScaleMM(
        11, 255, 0, 250, 370, 0.6f, 0,
        "Level : %d", level);
    (void)SDLTextWriteScaleMM(
        11, 255, 0, 250, 390, 0.6f, 0,
        "Skill : %d", skill);
    if ((menuVars.jediDebugCombo & 1u) != 0) {
        (void)psxDrawTexture(
            0x16fu, 7.0f, 32.0f,
            0.0f, 0.0f, 255u, 128, 128, 128);
    }
    if ((menuVars.jediDebugCombo & 2u) != 0) {
        (void)psxDrawTexture(
            0x16fu, 457.0f, 178.0f,
            0.0f, 0.0f, 255u, 128, 128, 128);
    }
    if ((uint8_t)(GameStruct.gameMode - 6u) > 1u) {
        winDrawBackground(6);
    }
    PresentWindow();
}

/* 0xC2110, 3 bytes, global, 2 named locals
 * menu_FormatMenu
 * PDB type: int (int, unsigned long)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_FormatMenu(int type, unsigned long controller_pad)
{
    (void)type;
    (void)controller_pad;
}

/* 0xC2120, 74 bytes, global, 0 named locals
 * menu_JumpCheckPoint
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_JumpCheckPoint(void)
{
    ClearInput();
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    GameStruct.gameMode = 6;
    GameStruct.inMenuFlag = 0;
    (void)sound_Resume();
    unpauseXA();
    (void)sound_PlayController(0, 0, "xsecret", 8u);
    (void)pwrup_JumpCheckPoint();
}

/* 0xC2170, 3 bytes, global, 0 named locals
 * menu_NOLOAD
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_NOLOAD(void)
{
    return 0;
}

/* 0xC2180, 92 bytes, global, 0 named locals
 * menu_RadarCheat
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_RadarCheat(void)
{
    ClearInput();
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    GameStruct.gameMode = 6;
    GameStruct.inMenuFlag = 0;
    (void)sound_Resume();
    unpauseXA();
    (void)sound_PlayController(0, 0, "xsecret", 8u);
    OptionStruct.DebugLevel =
        OptionStruct.DebugLevel != 0 ? 0u : 3u;
}

/* 0xC21E0, 43 bytes, global, 1 named locals
 * menu_addTotal
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_addTotal(unsigned amount)
{
    if (loadScreenFlag != 0) {
        loadTotal += amount;
        menu_redrawLoadscreen();
        __RenderLoad(1);
    }
}

/* 0xC2210, 124 bytes, global, 2 named locals
 * menu_applyvideooptions
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_applyvideooptions(void)
{
    unsigned resolution = mmGetModVal(&modVars[73]);
    unsigned window_mode;

    OptionStruct.ScreenWidth =
        (uint32_t)g_resolutions[resolution].width;
    OptionStruct.ScreenHeight =
        (uint32_t)g_resolutions[resolution].height;
    window_mode = mmGetModVal(&modVars[72]);
    OptionStruct.WindowMode = window_mode;
    OptionStruct.ResolutionChanged = mmGetModVal(&modVars[73]);
    UpdateResolution(
        (int)OptionStruct.ScreenWidth,
        (int)OptionStruct.ScreenHeight,
        (int)window_mode);
}

/* 0xC2290, 708 bytes, global, 7 named locals
 * menu_buildComboString
 * PDB type: void (unsigned char*, unsigned c...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_buildComboString(
    unsigned char *mp1Combos,
    unsigned char *src,
    unsigned comboNum)
{
    unsigned src_index = 0;
    unsigned dst_index = 2;
    int pending_hold = 0;

    mp1Combos[0] = ' ';
    mp1Combos[1] = ' ';
    while (src[src_index] != '\0') {
        unsigned char glyph = 0;
        int selected = 0;

        if (comboIconOverride == 0 && comboNum <= 7) {
            selected =
                menuVars.mmSelect1[menuVars.menuModeSP] == comboNum;
        }
        switch (src[src_index]) {
        case 'f':
            glyph = (unsigned char)(selected ? 'F' : 'f');
            break;
        case 'n':
            glyph = (unsigned char)(
                OptionStruct.ControllerConfig[0] == 0
                    ? (selected ? 'Y' : 'y')
                    : (selected ? 'B' : 'b'));
            break;
        case 's':
            glyph = (unsigned char)(
                OptionStruct.ControllerConfig[0] == 0
                    ? (selected ? 'A' : 'a')
                    : (selected ? 'Y' : 'y'));
            break;
        case 'w':
            glyph = (unsigned char)(selected ? 'X' : 'x');
            break;
        default:
            break;
        }

        if (glyph != 0) {
            pending_hold = 0;
            mp1Combos[dst_index++] = '<';
            mp1Combos[dst_index++] = glyph;
            mp1Combos[dst_index++] = '>';
        } else if (pending_hold == 0) {
            pending_hold = 1;
            mp1Combos[dst_index++] = ' ';
            mp1Combos[dst_index++] = ' ';
            mp1Combos[dst_index++] = ' ';
        } else {
            pending_hold = 0;
            mp1Combos[dst_index - 1] = '-';
            mp1Combos[dst_index++] = ' ';
            mp1Combos[dst_index++] = ' ';
            mp1Combos[dst_index++] = ' ';
            mp1Combos[dst_index++] = ' ';
        }
        ++src_index;
    }
    mp1Combos[dst_index] = '\0';
}

/* 0xC2560, 262 bytes, global, 7 named locals
 * menu_calcCompletionPoints
 * PDB type: unsigned (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
unsigned menu_calcCompletionPoints(void)
{
    const int16_t saved_model_one = GameStruct.ModelSelect[0];
    const int16_t saved_model_two = GameStruct.ModelSelect[1];
    unsigned completion_points = 0;
    unsigned award_level;
    unsigned level;
    unsigned model;

    for (level = 1; level <= 10; ++level) {
        for (model = 0; model < 5; ++model) {
            GameStruct.ModelSelect[0] = (int16_t)model;
            GameStruct.ModelSelect[1] = (int16_t)model;
            if (menu_CheckValidLevel(level, &award_level) != 0) {
                completion_points += award_level;
            }
        }
    }
    for (level = 11; level <= 14; ++level) {
        completion_points +=
            (unsigned)menu_CheckValidLevel(level, &award_level);
    }
    if (GameStruct.NumPlayers == 2) {
        GameStruct.ModelSelect[0] = 2;
        GameStruct.ModelSelect[1] = 2;
        if (menu_CheckValidLevel(10, &award_level) != 0) {
            ++completion_points;
        }
    }
    if ((secretBits & UINT32_C(0x10)) != 0) {
        ++completion_points;
    }
    GameStruct.ModelSelect[0] = saved_model_one;
    GameStruct.ModelSelect[1] = saved_model_two;
    if ((secretBits & UINT32_C(0x20)) != 0) {
        ++completion_points;
    }
    if ((secretBits & UINT32_C(0x40)) != 0) {
        ++completion_points;
    }
    if ((secretBits & UINT32_C(0x200)) != 0) {
        ++completion_points;
    }
    if (completion_points > 0x9du) {
        completion_points = 0x9du;
    }
    return completion_points;
}

/* 0xC2670, 57 bytes, global, 3 named locals
 * menu_calcTbarspeed
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_calcTbarspeed(unsigned range)
{
    unsigned percentage = menuVars.pointSeek / 250u;

    if (menuVars.pointSeek != 0 && percentage != 0) {
        menuVars.bar_speed = (range << 16) / percentage;
    } else {
        menuVars.bar_speed = 0;
    }
}

/* 0xC26B0, 52 bytes, global, 1 named locals
 * menu_cameraChange
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_cameraChange(unsigned view_type)
{
    camera_SetViewType((int)view_type);
    ClearInput();
    GameStruct.GameState &= UINT32_C(0xfdffffff);
    GameStruct.gameMode = 6;
    GameStruct.inMenuFlag = 0;
    (void)sound_Resume();
    unpauseXA();
}

/* 0xC26F0, 628 bytes, global, 2 named locals
 * menu_checkAbortOrPause
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_checkAbortOrPause(void)
{
    uint32_t pad;

    if ((padCurrentBits[0].padLevel1 & UINT32_C(0x900)) ==
        UINT32_C(0x900)) {
        ++GameStruct.AbortCount;
        if (GameStruct.AbortCount > 30) {
            GameStruct.GameState |= UINT32_C(2);
            menu_endGame();
        }
    } else {
        pad = menuVars.pad[0];
        if (GameStruct.NumPlayers == 2) {
            pad |= menuVars.pad[1];
        }
        if ((pad & JPB_PAD_START) == 0 &&
            p1Disconnected == 0 &&
            p2Disconnected == 0) {
            GameStruct.AbortCount = 0;
        } else if (game_gIsGameFlags(UINT32_C(0x804000)) == 0 &&
                   GameStruct.inMenuFlag == 0 &&
                   LevelSelect != 0) {
            menu_enterPauseMode();
        }
    }

    if (((menuVars.pad[0] | menuVars.pad[1]) &
         JPB_PAD_ZOOM_IN) != 0) {
        ++OptionStruct.overlayMode;
        refreshHUDCounter = 12;
        if (OptionStruct.overlayMode == 0) {
            GameStruct.screenShotFlag = 2;
        } else {
            GameStruct.screenShotFlag = 0;
            if (OptionStruct.overlayMode > 2) {
                OptionStruct.overlayMode = 0;
            }
        }
        if (jpb_menu_platform_hooks.saveSettingsData != NULL) {
            optionstruct options = OptionStruct;

            jpb_menu_platform_hooks.saveSettingsData(
                &options, jpb_menu_platform_user_data);
        }
    }
}

/* 0xC2970, 113 bytes, global, 4 named locals
 * menu_checkCombo
 * PDB type: unsigned (unsigned, short)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
unsigned menu_checkCombo(unsigned jedi, short combonum)
{
    Combo *combos = gaPlayerData[jedi].paCombos;

    for (;;) {
        combonum = combos[combonum].prev;
        if (combonum == -1) {
            return 1;
        }
        if (game_getCombo(
                (uint32_t)(int32_t)GameStruct.ModelSelect[jedi],
                (uint32_t)(int32_t)combonum) == 0) {
            return 0;
        }
    }
}

/* 0xC29F0, 152 bytes, global, 5 named locals
 * menu_checkMiniMod
 * PDB type: void (unsigned, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_checkMiniMod(unsigned level, unsigned decreased)
{
    int changed;

    do {
        unsigned secret_level = 11;
        unsigned secret_mask = 1;

        changed = 0;
        while (secret_level < 15) {
            if (level == secret_level &&
                (((secretBits & secret_mask) == 0) ||
                 (GameStruct.NumPlayers > 1 && level == 14))) {
                if (decreased == 0) {
                    level = level == 14 ? 1u : level + 1u;
                } else {
                    --level;
                }
                changed = 1;
            }
            secret_mask <<= 1;
            ++secret_level;
        }
    } while (changed);
    LevelSelect = (char)level;
}

/* 0xC2A90, 3 bytes, global, 1 named locals
 * menu_checkSoftReset
 * PDB type: unsigned (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
unsigned menu_checkSoftReset(unsigned unused)
{
    (void)unused;
    return 0;
}

/* 0xC2AA0, 33 bytes, global, 1 named locals
 * menu_checkTitleReload
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_checkTitleReload(void)
{
    if (menuVars.titleArt == 0) {
        menuVars.titleDispEnable |= 1;
        menuVars.titleArt = 1;
    } else {
        menuVars.titleDispEnable |= menuVars.titleArt;
    }
}

/* 0xC2AD0, 47 bytes, global, 0 named locals
 * menu_continueGame
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_continueGame(void)
{
    ClearInput();
    GameStruct.GameState &= UINT32_C(0xFDFFFFFF);
    GameStruct.gameMode = 6;
    GameStruct.inMenuFlag = 0;
    (void)sound_Resume();
    unpauseXA();
}

/* 0xC2B00, 120 bytes, global, 1 named locals
 * menu_controlDisconnect
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_controlDisconnect(void)
{
    if (GameStruct.NumPlayers != 2 ||
        (padExist & 2u) != 0 ||
        GameStruct.versusModeFlag == 1) {
        return 0;
    }

    menuBox(
        0x9du,
        200u,
        (int)(OptionStruct.ScreenWidth >> 1) - 0xb4,
        (int)(OptionStruct.ScreenHeight >> 1) - 0x0c,
        0x168,
        0x32,
        0,
        0,
        0x4b);
    return 1;
}

/* 0xC2B80, 63 bytes, global, 1 named locals
 * menu_copyCouncil
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_copyCouncil(void)
{
    SRECT rect = {0x200, 0x100, 0x200, 0x100};

    (void)DrawSync(0);
    (void)LoadImage(&rect, (unsigned *)(void *)menuVars.memBGptr);
    (void)DrawSync(0);
    menuVars.titleDispEnable = 1;
}

/* 0xC2BC0, 1022 bytes, global, 12 named locals
 * menu_councilPos
 * PDB type: void (playerObject*, VECTOR*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_councilPos(playerObject *player, VECTOR *center)
{
    _svector projected_position;
    int packed_screen;
    uint16_t screen_x;
    uint16_t screen_y;
    uint16_t player_model;
    uint16_t vertical_offset = 0;
    uint16_t loop;

    if ((menuVars.menuMode[menuVars.menuModeSP] & 0x1fffu) != 7u) {
        return;
    }

    projected_position.vx = (int16_t)center->vx;
    projected_position.vy = (int16_t)(center->vy - 32);
    projected_position.vz = (int16_t)center->vz;
    scene_gProject2Screen(&projected_position, &packed_screen);
    screen_x = (uint16_t)packed_screen;
    screen_y = (uint16_t)((uint32_t)packed_screen >> 16);
    player_model = (uint16_t)((uint16_t)player->playerID - 80u);

    {
        int16_t scaled = (int16_t)(int)(
            (float)OptionStruct.ScreenWidth * 0.001953125f *
            0.62222224f * (float)(int16_t)screen_x);

        screen_x = (uint16_t)(int)(
            (float)scaled +
            ((float)OptionStruct.ScreenWidth / 1920.0f) * 115.0f);
    }
    {
        int16_t scaled = (int16_t)(int)(
            ((float)OptionStruct.ScreenHeight / 240.0f) * 0.5f *
            (float)(int16_t)screen_y);

        screen_y = (uint16_t)(int)(
            (float)scaled -
            ((float)OptionStruct.ScreenHeight / 1080.0f) * 50.0f);
    }

    for (loop = 0;
         (int)(uint16_t)loop < (int)(int8_t)GameStruct.NumPlayers;
         ++loop) {
        uint16_t mapped_model =
            (uint16_t)modisorder2[menuVars.pselectMode[loop].mode];

        if (player_model == mapped_model) {
            uint16_t midpoint =
                (uint16_t)((menuVars.fcount >> 1) & 0x0fu);
            uint16_t brightness = (uint16_t)(15u - midpoint);
            uint16_t color = 0;
            uint16_t x_offset;
            int marker_x;
            int marker_y;

            if (midpoint < 9u) {
                brightness = midpoint;
            }
            if (((uint8_t)(1u << (loop & 31u)) &
                 menuVars.jediDebugCombo) == 0) {
                color = (uint16_t)(brightness * 8u + 44u);
            }
            x_offset = (uint16_t)(int)(
                ((float)OptionStruct.ScreenWidth / 1920.0f) * 80.0f);
            if (loop == 0) {
                x_offset = (uint16_t)(int)(
                    ((float)OptionStruct.ScreenWidth / 1920.0f) * 3.0f +
                    (float)x_offset);
            }
            marker_x = (int)(int16_t)screen_x;
            marker_y =
                (int)(int16_t)screen_y + (int)(uint16_t)vertical_offset;
            (void)psxDrawTexture(
                (unsigned)loop + 225u,
                (float)((int)(uint16_t)x_offset + marker_x) - 50.0f,
                (float)(marker_y + 15) + 46.0f,
                5.0f,
                11.0f,
                0x8100u,
                color,
                color,
                color);
            (void)psxDrawTexture(
                224u,
                (float)marker_x + 35.0f,
                (float)marker_y + 50.0f,
                35.0f,
                0.0f,
                0x8100u,
                color,
                color,
                color);
            vertical_offset = (uint16_t)(vertical_offset + 60u);
        }
    }
}

/* 0xC2FC0, 68 bytes, global, 1 named locals
 * menu_decAwardLevel
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_decAwardLevel(void)
{
    unsigned player = menuVars.scoreCurrentPlayer;

    if (menuVars.awardLevel[player] != 0) {
        --menuVars.awardLevel[player];
    }
    player = menuVars.scoreCurrentPlayer;
    if (menuVars.awardLevel[player] == 0) {
        menuVars.scoreScore = 0;
    }
}

/* 0xC3010, 284 bytes, global, 1 named locals
 * menu_demoMovie
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_demoMovie(void)
{
    if (menuVars.mcount == 0 &&
        (GameStruct.xaNum != 1 || GameStruct.xaFlag == 0) &&
        OptionStruct.Music != 0) {
        stopXA();
        playXA(1, (int)OptionStruct.musicVolume * 2, 1);
    }
    ++menuVars.mcount;
    if (introPlayed == 0) {
        stopXA();
        menuVars.mcount = 0;
        VideoVolume = 0.0f;
        menu_triggerMovie(9);
        menu_triggerMovie(8);
        VideoVolume = 0.45f;
        menu_triggerMovie(0);
        VideoVolume = 0.45f;
        introPlayed = 1;
        menuVars.movieSelect = 0;
        menuVars.titleArt = 1;
        maskPadBits(0);
        maskPadBits(1);
        menuVars.pad[0] = 0;
        menuVars.pad[1] = 0;
    }
    if (menuVars.pad[0] != 0 || menuVars.pad[1] != 0) {
        menuVars.mcount = 0;
    }
}

/* 0xC3130, 87 bytes, global, 7 named locals
 * menu_drawBigNum
 * PDB type: int (unsigned, unsigned, unsigne...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_drawBigNum(
    unsigned num,
    unsigned x,
    unsigned y,
    unsigned r,
    unsigned g,
    unsigned b)
{
    int width = psxDrawTexture(
        num + 0xb5,
        (float)x,
        (float)y,
        0.0f,
        0.0f,
        0xff,
        (int)r,
        (int)g,
        (int)b);

    return width + 1;
}

/* 0xC3190, 289 bytes, global, 10 named locals
 * menu_drawBigNums
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawBigNums(
    unsigned num,
    unsigned len,
    unsigned x,
    unsigned y,
    unsigned r,
    unsigned g,
    unsigned b)
{
    char c[16];
    char format[16];
    unsigned loop1;

    (void)snprintf(format, sizeof(format), "%%0%ud", len);
    (void)snprintf(c, sizeof(c), format, (int)num);
    for (loop1 = 0; c[loop1] != '\0'; ++loop1) {
        x += (unsigned)menu_drawBigNum(
            (unsigned)(uint8_t)c[loop1] - (unsigned)'0',
            x,
            y,
            r,
            g,
            b);
    }
}

/* 0xC32C0, 298 bytes, global, 8 named locals
 * menu_drawBigScore
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawBigScore(unsigned num, unsigned x, unsigned y)
{
    char score[16];
    unsigned index;

    (void)snprintf(score, sizeof(score), "%06d", (int)num);
    for (index = 0; score[index] != '\0'; ++index) {
        unsigned texture = (unsigned)(uint8_t)score[index] + 0xb8u;
        float x_copy = (float)x;
        float y_copy = (float)y;

        setPivotPositionMM_PSX(&x_copy, &y_copy, 0);
        (void)psxDrawTexture(
            texture, x_copy, y_copy, 0.0f, 0.0f,
            0x8000u, 0x60, 0x60, 0x60);
        x += fontSpec[texture].w;
    }
}

/* 0xC33F0, 3 bytes, global, 9 named locals
 * menu_drawColorPoly
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawColorPoly(
    unsigned x,
    unsigned y,
    unsigned width,
    unsigned height,
    unsigned char red,
    unsigned char green,
    unsigned char blue,
    unsigned type,
    unsigned flag)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)red;
    (void)green;
    (void)blue;
    (void)type;
    (void)flag;
}

/* 0xC3400, 3 bytes, global, 11 named locals
 * menu_drawColorPolyG4
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawColorPolyG4(
    unsigned x,
    unsigned y,
    unsigned width,
    unsigned height,
    unsigned char red1,
    unsigned char green1,
    unsigned char blue1,
    unsigned char red2,
    unsigned char green2,
    unsigned char blue2,
    unsigned type)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)red1;
    (void)green1;
    (void)blue1;
    (void)red2;
    (void)green2;
    (void)blue2;
    (void)type;
}

/* 0xC3410, 537 bytes, global, 11 named locals
 * menu_drawCombos
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawCombos(void)
{
    unsigned player = menuVars.jediDebugCombo;
    Combo *combos;
    unsigned combo;
    unsigned x = 40;
    unsigned y = 222;

    combo_InitComboData(&gaPlayerData[player]);
    combos = gaPlayerData[player].paCombos;
    for (combo = 0; combo < mp1ComboCount; ++combo) {
        int prerequisite = (int)(int16_t)combo;

        for (;;) {
            char combo_string[64];
            char display[32];
            const char *text;
            int tint;

            prerequisite = combos[prerequisite].prev;
            if (prerequisite != -1) {
                if (game_getCombo(
                        (uint32_t)(uint16_t)
                            GameStruct.ModelSelect[player],
                        (uint32_t)prerequisite) != 0) {
                    continue;
                }
                break;
            }
            tint = game_getCombo(
                (uint32_t)(uint16_t)GameStruct.ModelSelect[player],
                combo) != 0 ? 11 : 1;
            if (combo_ValidComboAward((int)player, (int)combo) == 0) {
                text = "NOT ELIGABLE";
            } else {
                menu_buildComboString(
                    (unsigned char *)combo_string,
                    (unsigned char *)combos[
                        menuVars.td.comboList[combo * 2u]].String,
                    combo);
                (void)sprintf(
                    display,
                    "%02d:%02d:%s",
                    (int)combo,
                    (int)menuVars.td.comboList[combo * 2u + 1u],
                    combo_string);
                text = display;
            }
            (void)textWrite(
                tint, 0, (int)x, (int)y, "%s", text);
            y = (unsigned)(int64_t)(
                (float)y + scaleAdjustmentMM * 60.0f);
            if (y > 400u) {
                y = 62;
                x = 256;
            }
            break;
        }
    }
}

/* 0xC3630, 230 bytes, global, 4 named locals
 * menu_drawController
 * PDB type: void (unsigned, unsigned, int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawController(unsigned x, unsigned y, int padnum)
{
    const uint8_t pad_bit =
        (uint8_t)(1u << ((unsigned)padnum & 31u));

    if ((padExist & pad_bit) == 0u) {
        return;
    }

    textWrite(
        11,
        2,
        (int)(x + 0x40u),
        (int)(y - 0x0cu),
        allText[0xedu + (unsigned)padnum]);
    if ((padTypes & pad_bit) != 0u) {
        psxDrawTexture2(
            0x9au,
            (float)(x + 0x1du),
            (float)(y + 0x1au),
            0.0f,
            0.0f,
            0xffu,
            11);
    }
    psxDrawTexture2(
        0xa8u,
        (float)x,
        (float)y,
        0.0f,
        0.0f,
        0xffu,
        11);
}

/* 0xC3720, 831 bytes, global, 11 named locals
 * menu_drawCredits
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawCredits(void)
{
    char line[256];
    float multiplier;
    float base;
    float line_spacing;
    float clip_left = 18.75f;
    float clip_top = 22.5f;
    float clip_right = 18.75f;
    float clip_bottom = 22.5f;
    unsigned shown = 0;
    unsigned loop1;
    float exit_x;
    float exit_y;

    if (skipCreditForFrame != 0) {
        skipCreditForFrame = 0;
        return;
    }
    setPivotPositionMM(&clip_left, &clip_top, 0);
    setPivotPositionMM(&clip_right, &clip_bottom, 8);
    jpb_TextSetClipRect(
        (int)clip_left,
        (int)clip_top,
        (int)clip_right,
        (int)clip_bottom);
    multiplier = (float)OptionStruct.ScreenHeight / 1080.0f;
    creditBarPosition +=
        (float)(int16_t)(menuVars.bar_speed >> 16) *
        120.0f * deltaTime * multiplier * 2.0f;
    base = (float)OptionStruct.ScreenHeight - creditBarPosition;
    line_spacing = scaleAdjustmentMM * 60.0f;

    for (loop1 = 0; loop1 < JPB_CREDIT_LINE_COUNT; ++loop1) {
        const unsigned char *source = theCredits[loop1];
        int heading;
        size_t source_index;
        size_t output_index = 0;

        if (source == NULL) {
            break;
        }
        heading = source[0] == 1;
        source_index = heading ? 1u : 0u;
        if (base > -20.0f &&
            base < (float)OptionStruct.ScreenHeight) {
            while (source[source_index] != 0 &&
                   output_index + 1u <
                       sizeof(line) / sizeof(line[0])) {
                line[output_index++] = (char)source[source_index++];
            }
            line[output_index] = '\0';
            (void)SDLTextWriteScaleMM(
                15 + heading,
                255,
                2,
                (int)(OptionStruct.ScreenWidth / 2u),
                (int)base,
                2.0f,
                0,
                "%s",
                line);
            ++shown;
        }
        base = (float)(int32_t)(base + line_spacing);
    }
    if (shown == 0 &&
        creditBarPosition > multiplier * 1000.0f) {
        creditBarPosition = 0.0f;
    }

    exit_x = 100.0f;
    exit_y = 100.0f;
    setPivotPositionMM(&exit_x, &exit_y, 6);
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)exit_x, (int)exit_y,
        2.25f, 0, "%s",
        allText[lastUsedInputType == 0 ? 476u : 239u]);
    jpb_TextClearClipRect();
}

/* 0xC3A60, 1678 bytes, global, 16 named locals
 * menu_drawEULA
 * PDB type: void (unsigned char**, int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void menu_drawEulaTexture(
    unsigned texture,
    float left,
    float top,
    float right,
    float bottom)
{
    SCREENRECT destination;
    CVECTOR color = {255, 255, 255, 255};

    setPivotPositionMM(&left, &top, 4);
    setPivotPositionMM(&right, &bottom, 4);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    _DrawTexture(
        menuTextures[texture], destination, NULL, color, 0.0f);
}

void menu_drawEULA(unsigned char **eula_text, int eula_text_length)
{
    uint32_t screen_height = OptionStruct.ScreenHeight;
    uint32_t scroll = menuVars.bar_y >> 16;
    uint32_t pad;
    float left = -960.0f;
    float top = -484.0f;
    float right = 960.0f;
    float bottom = 477.0f;
    float text_x = 205.0f;
    float text_y = 0.0f;
    float line_position = (float)screen_height;
    int line;

    menu_showVRAMBackground(5);
    mmDraw(eulaMdef);
    setPivotPositionMM(&left, &top, 4);
    setPivotPositionMM(&right, &bottom, 4);
    setTextClipFAllSides(&left, &top, &right, &bottom);

    if (eulaCanAccept != 0 &&
        (menuVars.pad[0] & JPB_PAD_COMBO_SOUTH) != 0) {
        OptionStruct.EULAaccepted = 1;
        if (jpb_menu_platform_hooks.saveSettingsData != NULL) {
            jpb_menu_platform_hooks.saveSettingsData(
                &OptionStruct, jpb_menu_platform_user_data);
        }
        menu_pushMenu(1);
    }

    pad = input_ReadControlPad(
        0, UINT32_MAX, &menuVars.oldpad[0]);
    if ((pad & (JPB_PAD_UP | JPB_PAD_DOWN)) == 0) {
        menu_drawEulaTexture(199, 710.0f, -484.0f, 760.0f, -430.0f);
        menu_drawEulaTexture(200, 710.0f, 422.0f, 760.0f, 476.0f);
    } else {
        if ((pad & JPB_PAD_UP) != 0) {
            menu_drawEulaTexture(
                195, 710.0f, -484.0f, 760.0f, -430.0f);
            menuVars.bar_y = (uint32_t)(int64_t)(
                (float)menuVars.bar_y -
                (float)menuVars.bar_speed * 2400.0f *
                    scaleAdjustmentMM * deltaTime);
        } else {
            menu_drawEulaTexture(
                199, 710.0f, -484.0f, 760.0f, -430.0f);
        }
        if ((pad & JPB_PAD_DOWN) != 0) {
            menu_drawEulaTexture(
                196, 710.0f, 422.0f, 760.0f, 476.0f);
            menuVars.bar_y = (uint32_t)(int64_t)(
                (float)menuVars.bar_y +
                (float)menuVars.bar_speed * 2400.0f *
                    scaleAdjustmentMM * deltaTime);
        } else {
            menu_drawEulaTexture(
                200, 710.0f, 422.0f, 760.0f, 476.0f);
        }
    }

    if (menuVars.bar_y < eulaMinScroll) {
        menuVars.bar_y = eulaMinScroll;
    } else if (menuVars.bar_y > eulaAcceptThreshold) {
        eulaCanAccept = 1;
        if (menuVars.bar_y > eulaMaxScroll) {
            menuVars.bar_y = eulaMaxScroll;
        }
    }

    top = ((float)(menuVars.bar_y - eulaMinScroll) /
           (float)(eulaMaxScroll - eulaMinScroll)) *
            (maxSlider - minSlider) + minSlider;
    menu_drawEulaTexture(243, 722.0f, top, 748.0f, top + 60.0f);

    setPivotPositionMM(&text_x, &text_y, 0);
    for (line = 0; line < eula_text_length + 2; ++line) {
        unsigned char *text;
        uint32_t draw_y;

        if (line < eula_text_length) {
            text = eula_text[line];
            if (text == NULL) {
                break;
            }
        } else if (line == eula_text_length) {
            text = (unsigned char *)"";
        } else {
            text = (unsigned char *)allText[490];
            if (lastUsedInputType != 0) {
                iconScaleOverride = 0.3f;
                text = (unsigned char *)allText[489];
            }
        }
        draw_y = (uint32_t)(int32_t)line_position - scroll;
        if (draw_y < screen_height) {
            (void)SDLTextWriteScaleMM(
                15, 255, 0, (int)text_x, (int)draw_y,
                1.25f, 0, "%s", (char *)text);
        }
        line_position += scaleAdjustmentMM * 34.0f;
        if (screen_height + scroll < (uint32_t)line_position) {
            break;
        }
    }
    clearTextClip();
    iconScaleOverride = -1.0f;
}

/* 0xC40F0, 1626 bytes, global, 15 named locals
 * menu_drawLevelSelectScreen
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void menu_drawLevelSelectTexture(
    unsigned texture_index,
    float left,
    float top,
    float right,
    float bottom,
    int left_top_pivot,
    int right_bottom_pivot,
    CVECTOR color,
    float layer)
{
    SCREENRECT destination;

    setPivotPositionMM(&left, &top, left_top_pivot);
    setPivotPositionMM(&right, &bottom, right_bottom_pivot);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    _DrawTexture(
        menuTextures[texture_index],
        destination,
        NULL,
        color,
        layer);
}

static void menu_publishWinif2FontSpec(void)
{
    static const FONTSPEC traced_specs[] = {
        {0x0e00, 3, 0, 54, 26, 28},   /* 0xe8 */
        {0x0e00, 3, 0, 26, 26, 28},   /* 0xe9 */
        {0x0e00, 3, 36, 226, 26, 28}, /* 0xea */
        {0x0e00, 3, 36, 198, 26, 28}, /* 0xeb */
        {0x0e00, 3, 36, 170, 26, 28}, /* 0xec */
        {0x0e00, 3, 36, 142, 26, 28}, /* 0xed */
        {0x0e00, 3, 28, 114, 26, 28}, /* 0xee */
        {0x0e00, 3, 28, 86, 26, 28},  /* 0xef */
        {0x0e00, 3, 28, 58, 26, 28},  /* 0xf0 */
        {0x0e00, 3, 28, 30, 26, 28},  /* 0xf1 */
        {0x0e00, 3, 62, 178, 17, 43}, /* 0xf2 */
        {0x0e00, 3, 85, 156, 11, 23}, /* 0xf3 */
        {0x0e00, 3, 85, 132, 11, 23}, /* 0xf4 */
        {0x0e00, 3, 62, 134, 17, 43}, /* 0xf5 */
        {0x0e00, 3, 85, 108, 11, 23}, /* 0xf6 */
        {0x0e00, 3, 85, 84, 11, 23},  /* 0xf7 */
        {0x0e00, 3, 62, 90, 17, 43}   /* 0xf8 */
    };
    static const struct {
        unsigned index;
        FONTSPEC spec;
    } traced_sparse[] = {
        {0x113, {0x0e00, 3, 52, 0, 8, 5}},
        {0x114, {0x0e00, 3, 8, 0, 8, 2}},
        {0x115, {0x0e00, 3, 85, 8, 11, 8}},
        {0x116, {0x0e00, 3, 85, 0, 11, 8}},
        {0x117, {0x0e00, 3, 97, 246, 11, 8}},
        {0x118, {0x0e00, 3, 16, 0, 8, 2}},
        {0x119, {0x0e00, 3, 24, 2, 2, 8}},
        {0x11a, {0x0e00, 3, 74, 2, 3, 8}},
        {0x11b, {0x0e00, 3, 97, 206, 8, 8}}
    };
    size_t index;

    for (index = 0; index < sizeof(traced_specs) / sizeof(traced_specs[0]);
         ++index) {
        fontSpec[0xe8u + index] = traced_specs[index];
    }
    for (index = 0;
         index < sizeof(traced_sparse) / sizeof(traced_sparse[0]);
         ++index) {
        fontSpec[traced_sparse[index].index] =
            traced_sparse[index].spec;
    }
}

static int menu_drawLevelSelectPsxTexture(
    unsigned texture,
    float x,
    float y,
    float width,
    float height,
    unsigned transparency,
    int color_index,
    float layer_depth)
{
    setPivotPositionMM_PSX(&x, &y, 0);
    return psxDrawTexture2Depth(
        texture,
        x,
        y,
        width,
        height,
        transparency,
        color_index,
        layer_depth);
}

void menu_drawLevelSelectScreen(unsigned interactive)
{
    CVECTOR panel_color = {225, 225, 225, 255};
    CVECTOR preview_color = {225, 225, 225, 255};
    int level = (int)(int8_t)LevelSelect;
    int tens = level / 10;
    int ones = level - tens * 10;
    int language = (int)OptionStruct.Language;
    float title_x = 0.0f;
    float title_y = 145.0f;
    float digit_x;
    float digit_y = 265.0f;
    float name_x = -800.0f;
    float name_y = 225.0f;
    unsigned text_index;

    gPSXDrawScaleX = 3.75f;
    gPSXDrawScaleY = 4.5f;
    menu_drawLevelSelectTexture(
        164, 0.0f, 0.0f, 0.0f, 0.0f, 0, 8,
        panel_color, 0.9f);
    menu_drawLevelSelectTexture(
        165, 0.0f, 0.0f, 0.0f, 0.0f, 0, 8,
        panel_color, 0.4f);

    setPivotPositionMM(&title_x, &title_y, 1);
    (void)SDLTextWriteScaleMM(
        15, 255, 2, (int)title_x, (int)title_y,
        2.5f, 0, "%s", allText[190]);

    if (interactive != 0) {
        menu_levelSelectMenu(levelSelectMdef);
        level = (int)(int8_t)LevelSelect;
        tens = level / 10;
        ones = level - tens * 10;
    }

    text_index = (unsigned)(305 + level);
    digit_x = (language == 0 || language == 6) ? 365.0f : 345.0f;
    setPivotPositionMM(&digit_x, &digit_y, 7);
    (void)psxDrawTexture(
        (unsigned)(0xe8 + tens),
        digit_x / gPSXDrawScaleX,
        digit_y / gPSXDrawScaleY,
        0.0f, 0.0f, 0x8000, 0x60, 0x60, 0x60);
    digit_x = (language == 0 || language == 6) ? 465.0f : 445.0f;
    digit_y = 265.0f;
    setPivotPositionMM(&digit_x, &digit_y, 7);
    (void)psxDrawTexture(
        (unsigned)(0xe8 + ones),
        digit_x / gPSXDrawScaleX,
        digit_y / gPSXDrawScaleY,
        0.0f, 0.0f, 0x8000, 0x60, 0x60, 0x60);

    iconScaleOverride = 0.3f;
    menu_drawLevelSelectPromptText();
    iconScaleOverride = -1.0f;
    setPivotPositionMM(&name_x, &name_y, 7);
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)name_x, (int)name_y,
        2.5f, 0, "%s", allText[text_index]);

    menu_drawLevelSelectTexture(
        fontSpec[410 + level].clut,
        116.0f, 92.0f, 960.0f, 354.5f, 0, 8,
        preview_color, 0.5f);
    menu_drawSelectBox();
    menu_drawSelectors();
    menu_drawLevelSelectPromptGlyphs();
    menu_fadeBG();
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
}

/* 0xC4750, 1697 bytes, global, 17 named locals
 * menu_drawLevelSelectScreen_OLD
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawLevelSelectScreen_OLD(unsigned interactive)
{
    SCREENRECT destination;
    CVECTOR color = {225, 225, 225, 255};
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    float scale_x;
    float scale_y;
    int mover_y = (int16_t)(
        (uint32_t)menuVars.mmv[0].mmvY >> 16);
    int level;
    int tens;
    int ones;
    int upgrade_level;

    scale_x = (float)OptionStruct.ScreenWidth * 0.001953125f;
    scale_y = (float)OptionStruct.ScreenHeight / 240.0f;
    gPSXDrawScaleX = scale_x;
    gPSXDrawScaleY = scale_y;

    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 8);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    _DrawTexture(
        menuTextures[164], destination, NULL, color, 0.9f);
    _DrawTexture(
        menuTextures[165], destination, NULL, color, 0.4f);

    left = 0.0f;
    top = 145.0f;
    setPivotPositionMM(&left, &top, 1);
    (void)SDLTextWriteScaleMM(
        15, 255, 2, (int)left, (int)top,
        2.5f, 0, "%s", allText[190]);

    if (menuVars.mmvTriggers[0] != 0) {
        menu_fadeBG();
        gPSXDrawScaleX = 1.0f;
        gPSXDrawScaleY = 1.0f;
        return;
    }
    if (interactive != 0) {
        menu_levelSelectMenu(levelSelectMdef);
    }

    level = (int)(int8_t)LevelSelect;
    tens = level / 10;
    ones = level - tens * 10;
    (void)psxDrawTexture(
        (unsigned)(0xe8 + tens),
        330.0f,
        (float)(mover_y + 24),
        0.0f, 0.0f, 0x8000u, 0x60, 0x60, 0x60);
    (void)psxDrawTexture(
        (unsigned)(0xe8 + ones),
        358.0f,
        (float)(mover_y + 24),
        0.0f, 0.0f, 0x8000u, 0x60, 0x60, 0x60);

    if (lastUsedInputType == 0) {
        (void)SDLTextWriteScaleMM(
            15, 255, 0,
            (int)(scale_x * 407.0f),
            (int)(scale_y * (float)(mover_y + 26)),
            2.9f, 0, "%s", allText[476]);
        (void)SDLTextWriteScaleMM(
            15, 255, 0,
            (int)(scale_x * 407.0f),
            (int)(scale_y * (float)(mover_y + 40)),
            2.9f, 0, "%s", allText[475]);
    } else {
        (void)SDLTextWriteScaleMM(
            15, 255, 0,
            (int)(scale_x * 407.0f),
            (int)(scale_y * (float)(mover_y + 26)),
            2.9f, 0, "%s", allText[239]);
        (void)SDLTextWriteScaleMM(
            15, 255, 0,
            (int)(scale_x * 407.0f),
            (int)(scale_y * (float)(mover_y + 40)),
            2.9f, 0, "%s", allText[241]);
    }
    (void)SDLTextWriteScaleMM(
        15, 255, 0,
        (int)(scale_x * 45.0f),
        (int)(scale_y * (float)(mover_y + 30)),
        2.5f, 0, "%s", allText[305 + level]);

    upgrade_level = tens;
    (void)jedi_CheckValidLevel(
        (unsigned)menuVars.artLevel, &upgrade_level);
    destination.left = (int32_t)(
        scale_x * (float)(menuVars.artloadPos + 34));
    destination.top = (int32_t)(scale_y * 21.0f);
    destination.right = (int32_t)(
        (float)destination.left + scale_x * 222.0f);
    destination.bottom = (int32_t)(
        (float)destination.top + scale_y * 140.0f);
    _DrawTexture(
        menuTextures[fontSpec[409 + level].clut],
        destination, NULL, color, frontZ);
    frontZ = (float)((double)frontZ + 0.001);

    if (menuVars.artload != 0) {
        if (menuVars.artloadPos < 0) {
            menuVars.artloadPos =
                (int16_t)(menuVars.artloadPos + 16);
        }
    } else if (menuVars.artloadPos > -256) {
        menuVars.artloadPos =
            (int16_t)(menuVars.artloadPos - 16);
    }

    menu_drawSelectBox();
    menu_drawSelectors();
    (void)psxDrawTexture2(
        0x113, 389.0f, 0.0f, 0.0f, 188.0f, 0x8000u, 11);
    (void)psxDrawTexture2(
        0x114, 469.0f, 61.0f, 0.0f, 117.0f, 0x8000u, 11);
    menu_fadeBG();
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
}

/* 0xC4E00, 1716 bytes, global, 19 named locals
 * menu_drawPlayerSelect
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawPlayerSelect(
    unsigned num,
    unsigned cnum,
    unsigned semi,
    unsigned shade,
    int player)
{
    PSELMOD *selection = &menuVars.pselectMode[num];
    unsigned select_index = menuVars.pplayers[num];
    int text_index;
    char *name;
    float offset_x;
    float offset_y;
    float pivot_x;
    float pivot_y;
    int level;
    int skill;

    if (cnum < 9u) {
        text_index = (int)cnum + 332;
    } else {
        text_index = GetCharacterByID((model_id)cnum)->TextIndex;
    }
    name = allText[text_index];

    switch ((uint8_t)selection->mode) {
    case 0:
        selection->x[0] = 0;
        selection->x[1] = 291;
        selection->mode = 1;
        selection->y[0] = -48;
        selection->y[1] = 0;
        selection->jedi = playerSelectPix[select_index];
        break;
    case 1:
        selection->y[0] = (int16_t)(selection->y[0] + 9);
        if (selection->y[0] > 0) {
            selection->y[0] = 0;
            selection->mode = 2;
            selection->jedi = playerSelectPix[select_index];
        }
        break;
    case 2:
        selection->x[1] = (int16_t)(selection->x[1] - 36);
        if (selection->x[1] < 0) {
            selection->mode = 3;
            selection->x[1] = 0;
        }
        break;
    case 3:
        if (selection->jedi != playerSelectPix[select_index]) {
            selection->mode = 4;
        }
        break;
    case 4:
        selection->x[1] = (int16_t)(selection->x[1] + 36);
        if (selection->x[1] >= 291) {
            selection->mode = 2;
            selection->x[1] = 291;
            selection->jedi = playerSelectPix[select_index];
        }
        break;
    case 5:
        selection->y[0] = (int16_t)(selection->y[0] - 9);
        if (selection->y[0] <= -48) {
            selection->mode = 0;
            selection->y[0] = -48;
        }
        break;
    default:
        break;
    }

    offset_x = 0.0f;
    offset_y = -180.0f;
    setPivotPositionMM(&offset_x, &offset_y, 4);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)offset_x, (int)offset_y,
        2.25f, 0, allText[304]);

    pivot_x = player == 0 ? 30.0f : 1225.0f;
    pivot_y = 35.0f;
    setPivotPositionMM(&pivot_x, &pivot_y, 0);

    offset_x = 600.0f;
    offset_y = 10.0f;
    setPositionOffPivotMM(&offset_x, &offset_y, pivot_x, pivot_y);
    (void)psxDrawTexture(
        num + 0xe1u,
        offset_x,
        offset_y,
        scaleAdjustmentMM * 5.0f,
        scaleAdjustmentMM * 0.0f,
        semi,
        (int)shade,
        (int)shade,
        (int)shade);
    (void)psxDrawTexture(
        0xdbu,
        pivot_x,
        pivot_y,
        scaleAdjustmentMM * 172.0f,
        scaleAdjustmentMM * 0.0f,
        semi,
        (int)shade,
        (int)shade,
        (int)shade);

    offset_x = 282.0f;
    offset_y = 29.0f;
    setPositionOffPivotMM(&offset_x, &offset_y, pivot_x, pivot_y);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)offset_x, (int)offset_y,
        2.25f, 0, name);

    jedi_CalcSkillLevels((int)cnum, &skill, &level);
    offset_x = 402.0f;
    offset_y = 166.5f;
    setPositionOffPivotMM(&offset_x, &offset_y, pivot_x, pivot_y);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)offset_x, (int)offset_y,
        2.25f, 0, "%s%02d", allText[224], level);
    offset_x = 399.0f;
    offset_y = 227.0f;
    setPositionOffPivotMM(&offset_x, &offset_y, pivot_x, pivot_y);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)offset_x, (int)offset_y,
        2.25f, 0, "%s%03d", allText[225], skill);

    offset_x = 245.0f;
    offset_y = 121.0f;
    setPositionOffPivotMM(&offset_x, &offset_y, pivot_x, pivot_y);
    (void)psxDrawTexture(
        0xe5u,
        offset_x,
        offset_y,
        scaleAdjustmentMM * 85.0f,
        scaleAdjustmentMM * 0.0f,
        semi,
        (int)shade,
        (int)shade,
        (int)shade);

    offset_x = (float)selection->x[1] + 55.0f;
    offset_y = 137.0f;
    setPositionOffPivotMM(&offset_x, &offset_y, pivot_x, pivot_y);
    if (selection->jedi < 212u) {
        float right = gPSXDrawScaleW * 40.0f;
        float bottom = gPSXDrawScaleH * 36.25f;
        SCREENRECT destination;
        CVECTOR color = {255, 255, 255, 255};

        setPositionOffPivotMM(
            &right, &bottom, offset_x, offset_y);
        destination.left = (int32_t)offset_x;
        destination.top = (int32_t)offset_y;
        destination.right = (int32_t)right;
        destination.bottom = (int32_t)bottom;
        _DrawTexture(
            menuTextures[fontSpec[selection->jedi].clut],
            destination,
            NULL,
            color,
            frontZ);
    } else {
        (void)psxDrawTexture(
            selection->jedi,
            offset_x,
            offset_y,
            scaleAdjustmentMM * 40.0f,
            scaleAdjustmentMM * 36.25f,
            0x8000u,
            (int)shade,
            (int)shade,
            (int)shade);
    }

    offset_x = (float)selection->x[1] + 2.0f;
    offset_y = 127.0f;
    setPositionOffPivotMM(&offset_x, &offset_y, pivot_x, pivot_y);
    (void)psxDrawTexture(
        0xdeu,
        offset_x,
        offset_y,
        scaleAdjustmentMM * 65.0f,
        scaleAdjustmentMM * 0.0f,
        semi,
        (int)shade,
        (int)shade,
        (int)shade);
}

/* 0xC54C0, 457 bytes, global, 2 named locals
 * menu_drawReconnect
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawReconnect(void)
{
    float x = -475.0f;
    float y = 335.0f;
    SCREENRECT destination;
    CVECTOR color = {255, 255, 255, 255};

    jpb_TextClearClipRect();
    if (p1Disconnected == 0 && p2Disconnected == 0) {
        menu_menuExit();
        return;
    }
    gTop = 300.0f;
    if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
        y = 435.0f;
        gTop = 400.0f;
    }
    if (OptionStruct.Language == 6) {
        x = -492.5f;
    }
    setPivotPositionMM(&x, &y, 1);
    (void)SDLTextWriteScaleMMDepth(
        15, 255, 0, (int)x, (int)y,
        2.5f, 0, 0.95f, "%s", allText[494]);

    gBottom = gTop + 372.4f;
    gLeft = -525.0f;
    gRight = 525.0f;
    setPivotPositionMM(&gLeft, &gTop, 1);
    setPivotPositionMM(&gRight, &gBottom, 1);
    SetGlobalDST();
    _DrawTexture(
        menuTextures[241], gDST, NULL, color, 0.99f);
}

/* 0xC5690, 364 bytes, global, 1 named locals
 * menu_drawScoreMovers
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawScoreMovers(void)
{
    unsigned index;

    for (index = 0; index < 4; ++index) {
        MMVDEF *control = &menuVars.mmv[index];
        int exit_menu = 0;

        menuVars.mmvCurrentMenuControl = control;
        if (control->mmvCounter == 0) {
            menu_nextMMV(control);
        }
        switch (control->mmvIns) {
        case 0x26:
            control->mmvY -= INT32_C(0x10000);
            break;
        case 0x27:
            control->mmvY += INT32_C(0x10000);
            break;
        case 0x28:
            control->mmvX -= INT32_C(0x10000);
            break;
        case 0x29:
            control->mmvX += INT32_C(0x10000);
            break;
        case 0x2a:
            control->mmvX += control->mmvXvect;
            control->mmvY += control->mmvYvect;
            break;
        case 0x36: {
            const uint8_t *trigger_bytes =
                (const uint8_t *)(const void *)&menuVars + 0x231;
            unsigned trigger =
                control->mmvSrc[(uint16_t)(control->mmvPtr - 1u)];

            control->mmvCounter =
                (uint16_t)((unsigned)trigger_bytes[trigger] * 2u);
            break;
        }
        case 0x38:
            exit_menu = (int)menu_handleMenuTriggers(
                control->mmvSrc[
                    (uint16_t)(control->mmvPtr - 1u)]);
            break;
        default:
            break;
        }
        if (control->mmvCounter != 0) {
            --control->mmvCounter;
        }
        if (exit_menu != 0) {
            menu_menuExit();
        } else if ((control->mmvMenuFlags & 2u) == 0 &&
                   control->mmvMenu != NULL) {
            menu_mainMenu(control->mmvMenu);
        }
    }
}

/* 0xC5800, 7640 bytes, global, 92 named locals
 * menu_drawScoreScreen
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void menu_scoreSetTextClip(float top)
{
    float left = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 8);
    setTextClipFAllSides(&left, &top, &right, &bottom);
}

static void menu_scoreDrawStaticAwardDetail(
    const AWARDSET *award_set,
    unsigned award_index,
    uint32_t menu_y,
    float health_y,
    float depth)
{
    unsigned type = award_set->awardType[award_index];

    menuTextDepthOverride = depth;
    menuVars.mmFlags = 1;
    if (type == 0u) {
        menuVars.mmX = 40;
        menuVars.mmY = menu_y;
        comboIconOverride = 1;
        mmDraw(comboDispDef);
        comboIconOverride = 0;
    } else if (type == 1u) {
        float x = 160.0f;
        float y = health_y;

        setPivotPositionMM(&x, &y, 0);
        menuVars.mmX = (uint32_t)(int32_t)x;
        menuVars.mmY = (uint32_t)(int32_t)y;
        mmDraw(healthForceMdef);
    } else if (type == 2u) {
        menuVars.mmX = 40;
        menuVars.mmY = menu_y;
        bonusOverride = 1;
        (void)drawDropForceMess(0);
        bonusOverride = 0;
    }
    menuTextDepthOverride = -1.0f;
}

static void menu_scoreDrawCurrentAwardDetail(
    AWARDSET *award_set,
    unsigned award_index)
{
    MPNT *position = &menuVars.mp[award_index];
    unsigned type = award_set->awardType[award_index];

    menuVars.mmX = (uint32_t)(int32_t)(position->x + 6);
    menuVars.mmY = (uint32_t)(int32_t)(
        position->y - (int16_t)position->maxScrolly +
        (int16_t)position->scrolly + 28);
    position->maxScrolly = 106;
    menuVars.mmFlags = 1;
    if (type == 0u) {
        comboIconOverride = 1;
        if (position->scrolly < position->maxScrolly) {
            mmDraw(comboDispDef);
        } else {
            menu_mainMenu(comboDispDef);
        }
        comboIconOverride = 0;
        cachedBonusLines[award_index] = (int16_t)menuVars.mmTotal;
    } else if (type == 1u) {
        int32_t scaled_x = (int32_t)(
            (float)menuVars.mmX * gPSXDrawScaleX);
        int32_t scaled_y = (int32_t)(
            (float)menuVars.mmY * gPSXDrawScaleY);
        float x = (float)scaled_x + 10.0f;
        float y = (float)scaled_y - 4.0f;

        setPivotPositionMM(&x, &y, 0);
        menuVars.mmX = (uint32_t)(int32_t)x;
        menuVars.mmY = (uint32_t)(int32_t)y;
        if (position->scrolly < position->maxScrolly) {
            mmDraw(healthForceMdef);
        } else {
            menu_mainMenu(healthForceMdef);
        }
        cachedBonusLines[award_index] = (int16_t)menuVars.mmTotal;
    } else if (type == 2u) {
        cachedBonusLines[award_index] =
            (int16_t)drawDropForceMess(award_index);
        if (position->scrolly >= 64u &&
            (menuVars.pad[menuVars.scoreCurrentPlayer] & 0x60u) != 0) {
            menu_setScoreMode(menuVars.scoreNextMode, 12);
            menuVars.mmSelect1[menuVars.menuModeSP & 7u] = 0;
        }
    }
}

static void menu_scoreDrawPanel(
    unsigned panel,
    unsigned active_panel,
    uint8_t score_mode)
{
    static const float fixed_top[3] = {283.0f, 170.5f, 58.0f};
    static const float fixed_clip[3] = {601.0f, 488.5f, 376.0f};
    MPNT sentinel_position;
    unsigned char *sentinel_backing;
    const MPNT *position = menu_scorePositionView(
        active_panel, &sentinel_position, &sentinel_backing);
    int dynamic = position != NULL && position->state == 2u;
    int fixed =
        (panel == 0u && cachedRewardsInit[0] != 0 &&
         (cachedRewardsInit[1] != 0 || cachedRewardsInit[2] != 0)) ||
        (panel == 1u && cachedRewardsInit[1] != 0 &&
         cachedRewardsInit[2] != 0) ||
        (panel == 2u && cachedRewardsEnd[2] != 0 && score_mode == 3u);
    float left;
    float top;
    float right;
    float bottom;
    float clip_left = 0.0f;
    float clip_top;
    float clip_right = 0.0f;
    float clip_bottom = 0.0f;
    SCREENRECT destination;
    SCREENRECT scissor;
    CVECTOR color = {225, 225, 225, 255};

    if (cachedRewardsInit[panel] == 0 || (!dynamic && !fixed)) {
        return;
    }

    left = gPSXDrawScaleX * 35.0f;
    if (dynamic) {
        top = ((float)((int)position->scrolly + position->y) - 84.0f) *
            gPSXDrawScaleY +
            (float)((unsigned)(uint16_t)cachedBonusLines[panel] * 60u) -
            320.0f;
        clip_top =
            ((float)((int)position->maxScrolly + position->y) - 84.0f) *
                gPSXDrawScaleY + 7.0f;
        if (fixed) {
            top = (float)((unsigned)(uint16_t)cachedBonusLines[panel] * 60u) +
                fixed_top[panel];
            clip_top = fixed_clip[panel];
        }
    } else {
        top = (float)((unsigned)(uint16_t)cachedBonusLines[panel] * 60u) +
            fixed_top[panel];
        clip_top = fixed_clip[panel];
    }
    top += cachedBonusLines[panel] == 1 ? 19.0f : 11.0f;
    right = left + 679.0f;
    bottom = top + 373.0f;
    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 0);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;

    setPivotPositionMM(&clip_left, &clip_top, 0);
    setPivotPositionMM(&clip_right, &clip_bottom, 8);
    scissor.left = (int32_t)clip_left;
    scissor.top = (int32_t)clip_top;
    scissor.right = (int32_t)clip_right;
    scissor.bottom = (int32_t)clip_bottom;
    _DrawTextureClipped(
        menuTextures[237], destination, NULL, color,
        0.3f - (float)panel * 0.05f, scissor);
}

static void menu_scoreDrawFrame(unsigned active_panel, AWARDSET *award_set)
{
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    SCREENRECT destination;
    SCREENRECT scissor;
    CVECTOR color = {225, 225, 225, 255};
    unsigned panel;
    int map_index = -1;

    clearTextClip();
    if (menuVars.scoreMode == 0u) {
        gPSXDrawScaleX = 1.0f;
        gPSXDrawScaleY = 1.0f;
        return;
    }

    for (panel = 0; panel < 3; ++panel) {
        menu_scoreDrawPanel(panel, active_panel, menuVars.scoreMode);
    }
    drawScoreMenus(active_panel, award_set);

    for (panel = 0; panel < 23; ++panel) {
        if ((int)psxScoreScreenMaps[panel].ID ==
            GameStruct.ModelSelect[menuVars.scoreCurrentPlayer]) {
            map_index = (int)panel;
            break;
        }
    }
    left = 339.0f;
    top = 54.5f;
    setPivotPositionMM_PSX(&left, &top, 0);
    (void)psxDrawTextureDepth(
        (unsigned)(map_index + 0x1aa),
        left, top, scaleAdjustmentMM * 139.0f,
        scaleAdjustmentMM * 124.0f,
        0x8200, menuVars.fadeupCounter, menuVars.fadeupCounter,
        menuVars.fadeupCounter, 0.8f);

    if ((menuVars.bar_y >> 16) != 0) {
        left = 925.0f;
        top = (float)(menuVars.bar_y >> 16) * 4.55f;
        right = 1165.0f;
        bottom = 0.0f;
        setPivotPositionMM(&left, &top, 0);
        setPivotPositionMM(&right, &bottom, 6);
        destination.left = (int32_t)left;
        destination.top = (int32_t)top;
        destination.right = (int32_t)right;
        destination.bottom = (int32_t)bottom;

        left = 0.0f;
        top = 0.0f;
        right = 0.0f;
        bottom = 0.0f;
        setPivotPositionMM(&left, &top, 0);
        setPivotPositionMM(&right, &bottom, 8);
        scissor.left = (int32_t)left;
        scissor.top = (int32_t)top;
        scissor.right = (int32_t)right;
        scissor.bottom = (int32_t)bottom;
        _DrawTextureClipped(
            controlTextures[0], destination, NULL, color, 0.6f, scissor);
    }

    redlineFunc();
    left = 960.0f;
    top = 165.0f;
    right = 1276.0f;
    bottom = 841.0f;
    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 0);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    left = 0.0f;
    top = 0.0f;
    right = 0.0f;
    bottom = 0.0f;
    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 8);
    scissor.left = (int32_t)left;
    scissor.top = (int32_t)top;
    scissor.right = (int32_t)right;
    scissor.bottom = (int32_t)bottom;
    _DrawTextureClipped(
        controlTextures[1], destination, NULL, color, 0.7f, scissor);
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
}

void menu_drawScoreScreen(unsigned interactive)
{
    unsigned level = menuVars.scoreoLevel;
    unsigned player = menuVars.scoreCurrentPlayer;
    AWARDSET *award_set = &menuVars.awardSet[player];
    unsigned player_score =
        (unsigned)GameStruct.aCharacterData[player].Score;
    unsigned active_panel = 4;
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    SCREENRECT destination;
    CVECTOR color = {225, 225, 225, 255};

    (void)interactive;
    gPSXDrawScaleX = 3.75f;
    gPSXDrawScaleY = 4.5f;
    if (level == 15u) {
        level = 6;
    } else if (level == 0u) {
        level = 1;
    } else if (level > 10u) {
        level = 10;
    }

    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 8);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    _DrawTexture(menuTextures[167], destination, NULL, color, 0.9f);
    _DrawTexture(menuTextures[168], destination, NULL, color, 0.4f);

    left = 0.0f;
    top = 145.0f;
    setPivotPositionMM(&left, &top, 1);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)left, (int)top,
        2.5f, 0, "%s", allText[237u + player]);
    if (menuVars.fadeupCounter < 96u) {
        unsigned fade = (unsigned)menuVars.fadeupCounter + 4u;

        menuVars.fadeupCounter = (uint8_t)(fade > 96u ? 96u : fade);
    }
    if (award_set->awardTotal != 0u &&
        award_set->awardTotal == award_set->awardCount) {
        (void)menu_scoreSmackdown(player_score);
    }

    switch (menuVars.scoreMode) {
    case 0: {
        unsigned index;

        menuVars.currentAward = 0;
        scoreYtot = 0;
        for (index = 0; index < 3; ++index) {
            MPNT *position = &menuVars.mp[index];

            position->x = -166;
            position->y = (int16_t)(redline[2u - index] - 20u);
            if (index != 0) {
                position->y = (int16_t)(position->y + (int)index * 4);
            }
            position->state = 0;
            position->scrolly = 0;
            position->maxScrolly = 64;
        }
        if (menuVars.mmv[0].state != 0 || menuVars.mmv[1].state != 0 ||
            menuVars.mmv[2].state != 0 || menuVars.mmv[3].state != 0) {
            menuVars.scoreMode = UINT8_MAX;
            menu_setScoreMode(9, 0);
            menuVars.fadeupCounter = 0;
            menuVars.bar_y = UINT32_C(0x00b10000);
            menuVars.bar_speed = 0;
        }
        memset(cachedRewardsInit, 0, sizeof(cachedRewardsInit));
        memset(cachedRewardsEnd, 0, sizeof(cachedRewardsEnd));
        memset(cachedBonusLines, 0, sizeof(cachedBonusLines));
        cachedAwardIndex = 0;
        break;
    }
    case 1:
        if (award_set->awardTotal != 0u) {
            if (GameStruct.NumPlayers == 1) {
                if (menuVars.td.comboListCount == 0u) {
                    menu_setScoreMode(3, 0);
                }
            } else if (menuVars.scoreCurrentPlayer == 0u) {
                menuVars.scoreCurrentPlayer = 1;
                testcombo(1);
                menuVars.scoreMode = 0;
            } else if (menuVars.scoreMode != 0u) {
                menu_setScoreMode(3, 0);
            }
        } else {
            menu_setScoreMode(13, 0);
            menuVars.scoreDst = award_set->maxpointaward - menuVars.scoreScore;
            menuVars.scoreNextMode = 1;
        }
        break;
    case 3:
        menuVars.loadSaveMode = 0;
        feedback_startEffect(0, 14);
        menu_pushMenu(0x66);
        if (LevelSelect == 23 || LevelSelect == 14) {
            menuVars.titleArt = 1;
            GameStruct.gameMode = 9;
            GameStruct.inMenuFlag = 1;
            if (jpb_menu_platform_hooks.setInMenu != NULL) {
                jpb_menu_platform_hooks.setInMenu(
                    1, jpb_menu_platform_user_data);
            }
            menuVars.titleDispEnable = 0;
            feedback_startEffect(0, 14);
            menu_pushMenu(0);
            menu_initNewMenu();
        }
        CleanupLevelData();
        maskPadBits(0);
        maskPadBits(1);
        break;
    case 6:
        cachedRewardsEnd[0] = 1;
        menu_scoreSetTextClip(610.0f);
        menuVars.bar_speed = 0;
        active_panel = 0;
        menuVars.currentAward = 0;
        if (menuVars.mp[0].state == 0u) {
            menuVars.mp[0].state = 1;
        } else if (menuVars.mp[0].state == 2u &&
                   menuVars.mp[0].scrolly < menuVars.mp[0].maxScrolly) {
            menuVars.mp[0].scrolly += 4;
        }
        break;
    case 7:
        cachedRewardsEnd[1] = 1;
        menu_scoreSetTextClip(495.0f);
        menuVars.bar_speed = 0;
        active_panel = 1;
        menuVars.currentAward = 1;
        if (menuVars.mp[1].state == 0u) {
            menuVars.mp[1].state = 1;
        } else if (menuVars.mp[1].state == 2u &&
                   menuVars.mp[1].scrolly < menuVars.mp[1].maxScrolly) {
            menuVars.mp[1].scrolly += 4;
        }
        break;
    case 8:
        cachedRewardsEnd[2] = 1;
        menu_scoreSetTextClip(385.0f);
        menuVars.bar_speed = 0;
        active_panel = 2;
        menuVars.currentAward = 2;
        if (menuVars.mp[2].state == 0u) {
            menuVars.mp[2].state = 1;
        } else if (menuVars.mp[2].state == 2u) {
            if (menuVars.mp[2].scrolly < menuVars.mp[2].maxScrolly) {
                menuVars.mp[2].scrolly += 4;
            }
            if ((menuVars.pad[player] & 0x60u) != 0) {
                menu_setScoreMode(menuVars.scoreNextMode, 12);
                menuVars.mmSelect1[menuVars.menuModeSP & 7u] = 0;
            }
        }
        break;
    case 9:
        menu_scoreSetTextClip(610.0f);
        menu_calcTbarspeed(47);
        if (award_set->award[0] == 0u) {
            menu_setScoreMode(menuVars.scoreNextMode, 6);
            scoreYtot += 47;
        } else {
            cachedRewardsInit[0] = 1;
            if (menu_scoreSmackdown(award_set->pointAwarded[0]) != 0) {
                menu_setScoreMode(6, 0);
            }
        }
        break;
    case 10:
        menu_scoreSetTextClip(495.0f);
        menu_calcTbarspeed(scoreYtot + 29u);
        if (award_set->award[1] == 0u) {
            menu_setScoreMode(menuVars.scoreNextMode, 7);
            scoreYtot += 29;
        } else {
            cachedRewardsInit[1] = 1;
            if (menu_scoreSmackdown(award_set->pointAwarded[1]) != 0) {
                menu_setScoreMode(7, 0);
            }
        }
        break;
    case 11:
        menu_scoreSetTextClip(385.0f);
        menu_calcTbarspeed(scoreYtot + 29u);
        if (award_set->award[2] == 0u) {
            menu_setScoreMode(1, 0);
        } else {
            cachedRewardsInit[2] = 1;
            if (menu_scoreSmackdown(award_set->pointAwarded[2]) != 0) {
                menu_setScoreMode(8, 0);
            }
        }
        break;
    case 12:
        if ((menuVars.pad[player] & 0x60u) != 0) {
            if (GameStruct.NumPlayers != 2 || player != 0u) {
                menu_setScoreMode(3, 0);
            } else {
                menuVars.scoreCurrentPlayer = 1;
                testcombo(1);
                menuVars.scoreMode = 0;
            }
        }
        break;
    case 13:
        if (award_set->awardTotal == 0u) {
            unsigned range = 47;

            award_set->awardTotal = 1;
            menuVars.scoreDst = (uint32_t)pointLvls[level][0] * 1000u;
            if (menuVars.scoreDst < player_score) {
                range = 76;
                menuVars.scoreDst =
                    (uint32_t)pointLvls[level][1] * 1000u;
                if (menuVars.scoreDst < player_score) {
                    range = 105;
                    menuVars.scoreDst =
                        (uint32_t)pointLvls[level][2] * 1000u;
                }
            }
            menu_calcTbarspeed(range - 5u);
            award_set->pointAwarded[0] = player_score;
        }
        if (menu_scoreSmackdown(award_set->pointAwarded[0]) != 0) {
            menu_setScoreMode(12, 0);
        }
        break;
    default:
        break;
    }

    if (menuVars.scoreMode != 0u) {
        char digits[16];
        unsigned shown_score = menuVars.scoreScore <= player_score
            ? menuVars.scoreScore
            : player_score;
        unsigned x = 301;
        size_t index;

        (void)sprintf(digits, "%06u", player_score - shown_score);
        for (index = 0; digits[index] != '\0'; ++index) {
            unsigned texture = (unsigned)(digits[index] - '0') + 0xb8u;
            float digit_x = (float)x;
            float digit_y = 183.0f;

            setPivotPositionMM_PSX(&digit_x, &digit_y, 0);
            (void)psxDrawTextureDepth(
                texture, digit_x, digit_y, 0.0f, 0.0f,
                0x8000, 0x60, 0x60, 0x60, 0.0f);
            x += fontSpec[texture].w;
        }
    }

    if ((cachedRewardsInit[1] != 0 || cachedRewardsInit[2] != 0) &&
        cachedRewardsEnd[0] != 0) {
        menu_scoreDrawStaticAwardDetail(
            award_set, 0, 140, 626.0f, 0.275f);
    }
    if (cachedRewardsInit[2] != 0 && cachedRewardsEnd[1] != 0) {
        menu_scoreDrawStaticAwardDetail(
            award_set, 1, 115, 513.0f, 0.225f);
    }
    if (cachedRewardsEnd[2] != 0 && menuVars.scoreMode == 3u) {
        menu_scoreDrawStaticAwardDetail(
            award_set, 2, 90, 401.0f, 0.175f);
    }
    if (active_panel < 3u) {
        menu_scoreDrawCurrentAwardDetail(award_set, active_panel);
    }
    menu_scoreDrawFrame(active_panel, award_set);
}

/* 0xC75E0, 1240 bytes, global, 13 named locals
 * menu_drawSelectBox
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawSelectBox(void)
{
    int valid_count = 0;

    if (menuVars.dstSelector != 0) {
        if (menuVars.selCount == 0) {
            menuVars.selCount = 0x18;
        }
    } else if (menuVars.selCount == 0) {
        /* no-op: retail falls through to animate/draw */
    }
    if (menuVars.selCount != 0) {
        --menuVars.selCount;
    }
    if (menuVars.dstSelector != 0 || menuVars.selCount != 0) {
        menuVars.selbox.y = 0x86;
        menuVars.selbox.w = 0x78;
        menuVars.selbox.h = 2;
        return;
    }

    if (menuVars.selbox.w < 0x78) {
        menuVars.selbox.w =
            (int16_t)(menuVars.selbox.w + 0x0f);
        if (menuVars.selbox.w > 0x78) {
            menuVars.selbox.w = 0x78;
        }
    } else if (menuVars.selbox.h < 0x16) {
        --menuVars.selbox.y;
        menuVars.selbox.h = (int16_t)(menuVars.selbox.h + 2);
        if (menuVars.selbox.h >= 0x16 && menuVars.artload == 0) {
            menuVars.artload = 1;
            menuVars.artloadPos = -256;
            menuVars.artLevel = (uint8_t)LevelSelect;
        }
    }

    (void)jedi_CheckValidLevel(
        (unsigned)menuVars.artLevel,
        &valid_count);
    {
        static const unsigned objective_textures[] = {
            0xf2u, 0xf5u, 0xf8u
        };
        static const float objective_x[] = {
            307.0f, 361.0f, 416.0f
        };
        size_t objective_index;

        for (objective_index = 0;
             objective_index < sizeof(objective_textures) /
                 sizeof(objective_textures[0]);
             ++objective_index) {
            int is_valid = valid_count > 0;

            if (is_valid) {
                --valid_count;
            }
            (void)menu_drawLevelSelectPsxTexture(
                objective_textures[objective_index],
                objective_x[objective_index],
                127.0f,
                scaleAdjustmentMM * 30.625f,
                scaleAdjustmentMM * 17.5f,
                is_valid ? 0x8000u : 0x8100u,
                is_valid ? 4 : 0x0b,
                0.5f);
        }
    }

    {
        float box_left =
            373.0f - (float)menuVars.selbox.w;
        float box_width =
            (99.0f + (float)menuVars.selbox.w) *
            scaleAdjustmentMM;
        float box_height =
            (float)menuVars.selbox.h *
            scaleAdjustmentMM;
        float box_top = (float)menuVars.selbox.y;

        (void)menu_drawLevelSelectPsxTexture(
            0x118, 472.0f, box_top,
            0.0f, box_height, 0x8000u, 0x0b, 0.6f);
        (void)menu_drawLevelSelectPsxTexture(
            0x118, box_left, box_top,
            0.0f, box_height, 0x8000u, 0x0b, 0.6f);
        (void)menu_drawLevelSelectPsxTexture(
            0x119, box_left, box_top,
            box_width, 0.0f, 0x8000u, 0x0b, 0.6f);
        (void)menu_drawLevelSelectPsxTexture(
            0x11a, box_left,
            box_top + box_height - scaleAdjustmentMM * 2.0f,
            box_width, 0.0f, 0x8000u, 0x0b, 0.6f);
        (void)menu_drawLevelSelectPsxTexture(
            0x11b, box_left, box_top,
            box_width, box_height, 0x8000u, 0x0b, 0.65f);
    }

    menu_drawSelector(
        373.0f, (float)(menuVars.selbox.y - 4));
    menu_drawSelector(
        373.0f,
        (float)(menuVars.selbox.y + (int)menuVars.selbox.h - 6));
    if (menuVars.artloadPos < 0) {
        menuVars.artloadPos = (int16_t)(menuVars.artloadPos + 0x10);
    }
}

/* 0xC7AC0, 269 bytes, local, 4 named locals
 * menu_drawSelector
 * PDB type: void (float, float)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void menu_drawSelector(float x, float y)
{
    (void)menu_drawLevelSelectPsxTexture(
        0x115, x, y, 0.0f, 0.0f, 0x8000u, 0x0b, 0.7f);
    (void)menu_drawLevelSelectPsxTexture(
        0x116,
        x + scaleAdjustmentMM * 8.0f,
        y,
        scaleAdjustmentMM * 85.0f,
        0.0f,
        0x8000u,
        0x0b,
        0.7f);
    (void)menu_drawLevelSelectPsxTexture(
        0x117,
        x + scaleAdjustmentMM * 93.0f,
        y,
        0.0f,
        0.0f,
        0x8000u,
        0x0b,
        0.7f);
}

/* 0xC7BD0, 528 bytes, global, 6 named locals
 * menu_drawSelectors
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawSelectors(void)
{
    int index;
    int y = menuVars.dstSelector;
    int curve = y * 12 - 0x7f8;

    for (index = 0; index < 9; ++index) {
        int row_y;

        if (y > 0xaa) {
            row_y = y - (curve * curve >> 16);
        } else {
            row_y = y + ((-curve) * (-curve) >> 16);
        }
        row_y -= 0x32;
        if (row_y >= 22 && row_y < 179) {
            menu_drawSelector(373.0f, (float)row_y);
        }
        y += 0x24;
        curve += 0x1b0;
    }

    if (menuVars.dstSelector < 0) {
        menuVars.dstSelector += 4;
        if (menuVars.dstSelector > 0) {
            menuVars.dstSelector = 0;
        }
    } else if (menuVars.dstSelector > 0) {
        menuVars.dstSelector -= 4;
        if (menuVars.dstSelector < 0) {
            menuVars.dstSelector = 0;
        }
    }
}

/* 0xC7DE0, 365 bytes, global, 4 named locals
 * menu_dumpMemory
 * PDB type: void (unsigned, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_dumpMemory(unsigned x, unsigned y)
{
    unsigned total_used;
    unsigned total_free;

    (void)SDLTextWrite(
        11, 0, (int)x, (int)y, 0, "MEM USE\n");
    (void)SDLTextWrite(
        11, 0, (int)x, (int)y + 12, 0,
        "Bank %02d: F:%03dK T:%03dK",
        0,
        maMemoryBanks[0].memFree >> 10,
        maMemoryBanks[0].memSize >> 10);
    total_used = maMemoryBanks[0].memUsed;
    total_free = maMemoryBanks[0].memFree;
    (void)SDLTextWrite(
        11, 0, (int)x, (int)y + 28, 0,
        "Bank %02d: F:%03dK T:%03dK",
        1,
        maMemoryBanks[1].memFree >> 10,
        maMemoryBanks[1].memSize >> 10);
    total_used += maMemoryBanks[1].memUsed;
    total_free += maMemoryBanks[1].memFree;
    (void)SDLTextWrite(
        11, 0, (int)x, (int)y + 44, 0,
        "Bank %02d: F:%03dK T:%03dK",
        2,
        maMemoryBanks[2].memFree >> 10,
        maMemoryBanks[2].memSize >> 10);
    total_used += maMemoryBanks[2].memUsed;
    total_free += maMemoryBanks[2].memFree;
    (void)SDLTextWrite(
        11, 0, (int)x, (int)y + 60, 0,
        "All:used %03dK, free %03dK\n",
        total_used >> 10,
        total_free >> 10);
}

/* 0xC7F50, 19 bytes, global, 0 named locals
 * menu_endGame
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_endGame(void)
{
    GameStruct.gameMode = 9;
    GameStruct.inMenuFlag = 0;
    sound_StopAll();
}

/* 0xC7F70, 373 bytes, global, 0 named locals
 * menu_enterPauseMode
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_enterPauseMode(void)
{
    if (game_gIsGameFlags(UINT32_C(0x804000)) != 0 ||
        GameStruct.inMenuFlag != 0 ||
        LevelSelect == 0) {
        return;
    }

    ClearGlyphCache();
    pauseXA();
    GameStruct.GameState |= UINT32_C(0x02000000);
    memset(menuVars.mmSelect1, 0, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 0, sizeof(menuVars.mmSelect2));
    GameStruct.letterboxFlag2 = GameStruct.letterboxFlag;
    GameStruct.inMenuFlag = 1;
    game_clearLetterBox();
    menu_pushMenu(0x40);
    menu_pushMenu(0x14);
    if (p1Disconnected != 0 || p2Disconnected != 0) {
        menu_pushMenu(0xa0);
    }
    menu_initNewMenu();
    sound_Pause();
}

/* 0xC80F0, 770 bytes, global, 2 named locals
 * menu_enterPlayerCouncilMode
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_enterPlayerCouncilMode(void)
{
    unsigned bit;
    unsigned model;

    sound_StopAll();
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    memset(menuVars.mmSelect1, 0, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 0, sizeof(menuVars.mmSelect2));
    memset(menuVars.menuMode, 0x32, sizeof(menuVars.menuMode));
    memset(menuVars.frKeyBuff, 0, sizeof(menuVars.frKeyBuff));
    memset(menuVars.frKeyBuff2, 0, sizeof(menuVars.frKeyBuff2));
    GameStruct.inMenuFlag = 1;

    model = (unsigned)((int)GameStruct.ModelSelect[0] % 80);
    for (bit = 3; bit <= 7; ++bit) {
        game_CLR_GLOBALBIT(bit);
        if (model == bit - 3) {
            game_SET_GLOBALBIT(bit);
        }
    }
    menuVars.subplayers[0] = menuVars.pplayers[0];

    model = (unsigned)((int)GameStruct.ModelSelect[1] % 80);
    for (bit = 8; bit <= 12; ++bit) {
        game_CLR_GLOBALBIT(bit);
        if (GameStruct.NumPlayers > 1 && model == bit - 8) {
            game_SET_GLOBALBIT(bit);
        }
    }
    menuVars.subplayers[1] = menuVars.pplayers[1];

    feedback_startEffect(0, 14);
    menuVars.menuModeSP = (menuVars.menuModeSP + 1u) & 7u;
    menuVars.menuMode[menuVars.menuModeSP] = 0;
    menuVars.mmSelect1[menuVars.menuModeSP] = 0;
    menuVars.mmSelect2[menuVars.menuModeSP] = 0;

    feedback_startEffect(0, 14);
    menuVars.menuModeSP = (menuVars.menuModeSP + 1u) & 7u;
    menuVars.menuMode[menuVars.menuModeSP] =
        GameStruct.continueAble != 0 ? 5 : 7;
    menuVars.mmSelect1[menuVars.menuModeSP] = 0;
    menuVars.mmSelect2[menuVars.menuModeSP] = 0;
    if (GameStruct.continueAble == 0) {
        menu_initNewMenu();
    }
}

/* 0xC8400, 479 bytes, global, 2 named locals
 * menu_enterScoreMode
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_enterScoreMode(unsigned mode)
{
    unsigned current_level;

    sound_StopAll();
    GameStruct.inMenuFlag = 0;
    memset(menuVars.mmSelect1, 0, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 0, sizeof(menuVars.mmSelect2));
    memset(menuVars.menuMode, 0x32, sizeof(menuVars.menuMode));
    memset(menuVars.frKeyBuff, 0, sizeof(menuVars.frKeyBuff));
    memset(menuVars.frKeyBuff2, 0, sizeof(menuVars.frKeyBuff2));
    CleanupLevelData();
    GameStruct.GameState |= UINT32_C(0x02000000);
    memset(menuVars.mmSelect1, 0, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 0, sizeof(menuVars.mmSelect2));
    menuVars.pplayers[0] = 0;
    GameStruct.inMenuFlag = 1;
    current_level = GameStruct.CurrentLevel;

    if (current_level >= 11u && current_level <= 13u) {
        achievement_complete((int)current_level + 6);
        GameStruct.gameMode = 0;
        menu_pushMenu(0);
    } else if (mode == 9u) {
        GameStruct.gameMode = 0;
        menu_pushMenu(0);
    } else {
        if (mode == 3u) {
            if (current_level == 6u) {
                GameStruct.gameMode = 3;
            } else {
                GameStruct.gameMode = 1;
                menu_pushMenu(0x66);
            }
        }
        if (current_level == 14u) {
            LevelSelect = (char)current_level;
            achievement_complete(20);
        }
        menu_pushMenu(0x27);
    }
    menu_initNewMenu();
}

/* 0xC85E0, 87 bytes, global, 0 named locals
 * menu_enterTitleMode
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_enterTitleMode(void)
{
    menuTexLoaded2 = 0;
    memset(menuVars.mmSelect1, 0, sizeof(menuVars.mmSelect1));
    memset(menuVars.mmSelect2, 0, sizeof(menuVars.mmSelect2));
    GameStruct.inMenuFlag = 1;
    memset(menuVars.menuMode, 1, 8);
    memset(menuVars.frKeyBuff, 0, sizeof(menuVars.frKeyBuff));
    menuVars.menuMode[menuVars.menuModeSP] = 1;
}

/* 0xC8640, 225 bytes, global, 7 named locals
 * menu_fadeBG
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_fadeBG(void)
{
    SCREENRECT destination;
    CVECTOR color;
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    menuVars.fadeupCounter =
        (uint8_t)(menuVars.fadeupCounter + 5u);
    if (menuVars.fadeupCounter > 0x7f) {
        menuVars.fadeupCounter = 0x7f;
        menuVars.yflag = 0;
    }

    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 8);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    color.r = 0;
    color.g = (uint8_t)(((unsigned)menuVars.fadeupCounter * 15u) >> 7);
    color.b = (uint8_t)(menuVars.fadeupCounter >> 1);
    color.cd = 0xff;
    _DrawTexture(whitemat, destination, NULL, color, 1.0f);
}

/* 0xC8730, 3 bytes, global, 0 named locals
 * menu_finishloadGame
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_finishloadGame(void)
{
}

/* 0xC8740, 156 bytes, global, 5 named locals
 * menu_grayBars
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_grayBars(
    unsigned x,
    unsigned y,
    unsigned width,
    unsigned height,
    unsigned type)
{
    uint32_t packed_color;

    (void)type;
    memcpy(&packed_color, &Colors[9], sizeof(packed_color));
    (void)DrawRectangle(
        (float)x,
        (float)y,
        (float)width,
        (float)height,
        (long)packed_color);
    memcpy(&packed_color, &Colors[10], sizeof(packed_color));
    (void)DrawRectangle(
        (float)(x - 2u),
        (float)(y - 1u),
        (float)(width + 4u),
        (float)(height + 2u),
        (long)packed_color);
}

/* 0xC87E0, 3013 bytes, global, 11 named locals
 * menu_handleMenuTriggers
 * PDB type: unsigned (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC93B0, 388 bytes, global, 3 named locals
 * menu_handleMovers
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_handleMovers(void)
{
    unsigned index;

    for (index = 0; index < (unsigned)menuVars.mmvCount; ++index) {
        MMVDEF *control = &menuVars.mmv[index];
        int exit_menu = 0;

        menuVars.mmvCurrentMenuControl = control;
        if (control->mmvCounter == 0) {
            menu_nextMMV(control);
        }
        switch (control->mmvIns) {
        case 0x26:
            control->mmvY -= INT32_C(0x10000);
            break;
        case 0x27:
            control->mmvY += INT32_C(0x10000);
            break;
        case 0x28:
            control->mmvX -= INT32_C(0x10000);
            break;
        case 0x29:
            control->mmvX += INT32_C(0x10000);
            break;
        case 0x2a:
            control->mmvX += control->mmvXvect;
            control->mmvY += control->mmvYvect;
            break;
        case 0x36: {
            const uint8_t *trigger_bytes =
                (const uint8_t *)(const void *)&menuVars + 0x231;
            unsigned trigger =
                control->mmvSrc[(uint16_t)(control->mmvPtr - 1u)];

            control->mmvCounter =
                (uint16_t)((unsigned)trigger_bytes[trigger] * 2u);
            break;
        }
        case 0x38:
            exit_menu = (int)menu_handleMenuTriggers(
                control->mmvSrc[
                    (uint16_t)(control->mmvPtr - 1u)]);
            break;
        default:
            break;
        }
        if (control->mmvCounter != 0) {
            --control->mmvCounter;
        }
        if (exit_menu != 0) {
            menu_menuExit();
        } else if ((control->mmvMenuFlags & 2u) == 0 &&
                   control->mmvMenu != NULL) {
            menu_mainMenu(control->mmvMenu);
        }
    }
}

/* 0xC9540, 168 bytes, global, 0 named locals
 * menu_handleObjectiveMessage
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_handleObjectiveMessage(void)
{
    GameStruct.inMenuFlag = 1;
    menu_pushMenu(0x41);
    menu_pushMenu(0x2A);
    camera_SetCameras();
}

/* 0xC95F0, 240 bytes, global, 2 named locals
 * menu_handleUnformatted
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_handleUnformatted(void)
{
    uint32_t pad;
    uint32_t directions;
    unsigned sound;
    int complete = 0;

    (void)textWrite(
        11,
        2,
        (int)(OptionStruct.ScreenWidth >> 1),
        0x52,
        allText[170],
        (unsigned)menuVars.cardSelect + 1u,
        (unsigned)menuVars.cardSelect + 1u);
    (void)textWrite(
        11,
        2,
        (int)(OptionStruct.ScreenWidth >> 1),
        0xca,
        allText[272u + (unsigned)menuVars.dialogBox1]);
    pad = menuVars.pad[0] | menuVars.pad[1];
    menuVars.mmFlags = 0;
    menuVars.controlFlags = 3;
    directions = pad & UINT32_C(0xa000);
    if (directions != 0) {
        menuVars.dialogBox1 ^= 1u;
    }
    if ((pad & JPB_PAD_JUMP) != 0) {
        menuVars.dialogBox1 = 0;
    }
    sound = directions != 0 ? 1u : 0u;
    if ((pad & (JPB_PAD_JUMP | JPB_PAD_COMBO_SOUTH)) != 0) {
        sound = 1;
        complete = 1;
    } else if (directions == 0) {
        return 0;
    }
    menu_sound(sound);
    return complete;
}

/* 0xC96E0, 156 bytes, global, 3 named locals
 * menu_initCredits
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initCredits(void)
{
    unsigned music;

    GameStruct.gameMode = 0;
    menuVars.bar_y = 0;
    menuVars.bar_speed = UINT32_C(0x10000);
    if (menuVars.titleArt == 0) {
        menuVars.titleArt = 1;
    }
    menuVars.titleDispEnable |= menuVars.titleArt;
    music = credMusic[credMuse];
    if (OptionStruct.Music != 0) {
        stopXA();
        playXA(
            (int)music,
            (int)OptionStruct.musicVolume * 2,
            1);
    }
    creditBarPosition = 0.0f;
    skipCreditForFrame = 1;
    credMuse = (unsigned char)((credMuse + 1u) & 7u);
}

/* 0xC9780, 157 bytes, global, 4 named locals
 * menu_initEULA
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initEULA(int text_length)
{
    float scale = scaleAdjustmentMM;
    uint32_t screen_height = OptionStruct.ScreenHeight;
    int line_space = (int)(scale * 34.0f);
    uint32_t content_height =
        (uint32_t)line_space * (uint32_t)(text_length + 2);
    uint32_t initial_position = (uint32_t)(int64_t)(
        (float)screen_height - scale * 70.0f);
    uint32_t maximum_position =
        content_height - (uint32_t)(int)(scale * 985.0f) +
        screen_height;
    uint32_t accept_position =
        content_height - (uint32_t)(int)(scale * 1085.0f) +
        screen_height;

    menuVars.bar_speed = UINT32_C(0x10000);
    eulaCanAccept = 0;
    menuVars.bar_y = initial_position << 16;
    eulaMinScroll = menuVars.bar_y;
    eulaMaxScroll = maximum_position << 16;
    eulaAcceptThreshold = accept_position << 16;
}

/* 0xC9820, 109 bytes, global, 0 named locals
 * menu_initGameover
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initGameover(void)
{
    GameStruct.GameState |= UINT32_C(0x02000000);
    menuVars.titleArt = 0;
    GameStruct.inMenuFlag = 1;
    menu_pushMenu(0x2D);
}

/* 0xC9890, 649 bytes, global, 6 named locals
 * menu_initLevelSelectScreen
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initLevelSelectScreen(void)
{
    uint8_t *movers[4] = {
        frameBottomMover,
        frameRightMover,
        frameLeftMover,
        frameTopMover
    };
    unsigned mover;
    unsigned player;

    menu_pushMenu(0);
    menu_pushMenu(0x1a);
    menuVars.bgWidth = (uint16_t)OptionStruct.ScreenWidth;
    menuVars.bgHeight = (uint16_t)OptionStruct.ScreenHeight;
    menuVars.mloadShift = 0;
    menuVars.titleDispEnable = 1;
    LevelSelect = 1;
    menuVars.dstSelector = 0;
    menuVars.selbox.y = 0x86;
    menuVars.selbox.w = 0x78;
    menuVars.selbox.h = 2;
    menuVars.selCount = 0;
    menuVars.artload = 1;
    menuVars.artLevel = 1;
    menuVars.artloadPos = -256;
    prim_gSetBkColor(0, 0, 0);
    GameStruct.gameMode = 0;
    menuVars.mmSelect1[menuVars.menuModeSP & 7u] = 0;
    menuVars.mmSelect2[menuVars.menuModeSP & 7u] = 0;
    for (mover = 0; mover < 4; ++mover) {
        unsigned index = menuVars.mmvCount;

        if (index < 6) {
            MMVDEF *control = &menuVars.mmv[index];

            control->mmvSrc = movers[mover];
            control->mmvPtr = 0;
            control->mmvCounter = 0;
            control->mmvX = 0;
            control->mmvY = 0;
            control->mmvMenu = NULL;
            control->state = 1;
        }
        ++menuVars.mmvCount;
    }
    for (player = 0;
         player < (unsigned)(uint8_t)GameStruct.NumPlayers;
         ++player) {
        if (menuVars.pselectMode[player].mode == 0) {
            continue;
        }
        if (GameStruct.ModelSelect[player] == 0) {
            GameStruct.ModelSelect[player] = 6;
        } else if (GameStruct.ModelSelect[player] == 1) {
            GameStruct.ModelSelect[player] = 5;
        } else if (GameStruct.ModelSelect[player] == 4) {
            GameStruct.ModelSelect[player] = 7;
        }
    }
}

/* 0xC9B20, 348 bytes, global, 4 named locals
 * menu_initLoadBar
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initLoadBar(void)
{
    char street_texture[64];
    int load_mode;
    int movie;

    loadTotal = 0;
    if (LevelSelect == 1) {
        load_mode = 1;
    } else if (LevelSelect == 8) {
        load_mode = 2;
    } else {
        goto configure_load_screen;
    }

    if (menuVars.ingameMovies == 0) {
        movie = 1;
        if (load_mode == 2) {
            movie = (int)GameStruct.CurrentLevel % 5 + 2;
        }
        menu_triggerMovie((unsigned)movie);
        checkResetAbort();
    }

    menuVars.titleDispEnable = 0;
    if (load_mode != 1) {
        sprintf(
            street_texture,
            "streets%s.tga",
            snames[(int)GameStruct.CurrentLevel % 5]);
        (void)resource_getPath(
            street_texture, JPB_RESOURCE_EFFECT);
    }

configure_load_screen:
    if (LevelSelect == 0) {
        menuVars.titleDispEnable = (uint8_t)LevelSelect;
        loadScreenFlag = 0;
        return;
    }
    menuVars.titleDispEnable = 1;
    (void)DrawSync(0);
    VSync(0);
    loadScreenFlag = 1;
}

/* 0xC9C80, 2500 bytes, global, 5 named locals
 * menu_initNewMenu
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initNewMenu(void)
{
    uint16_t mode;

    maskPadBits(0);
    maskPadBits(1);
    menuVars.pad[0] = 0;
    menuVars.pad[1] = 0;
    menuVars.itemSelect = 0;
    menuVars.vramx = 0;
    menuVars.yoffset = 0;
    menuVars.pSelect = 0;
    menuVars.mmColorSelect = 11;
    menuVars.mmColorNotSelect = 12;
    menuVars.mmvCount = 0;
    mode = menuVars.menuMode[menuVars.menuModeSP & 7u];

    switch (mode) {
    case 0: {
        unsigned bit;
        unsigned player_two_model;

        sound_StopAll();
        totalframes += 16;
        game_clearLetterBox();
        menuVars.mcount = 0;
        GameStruct.ModelSelect[0] = obi_wan_model;
        GameStruct.ModelSelect[1] = qui_gon_model;
        game_CLR_GLOBALBIT(3);
        game_SET_GLOBALBIT(3);
        for (bit = 4; bit <= 7; ++bit) {
            game_CLR_GLOBALBIT(bit);
        }
        menuVars.subplayers[0] = menuVars.pplayers[0];
        player_two_model =
            (unsigned)((int)GameStruct.ModelSelect[1] % 80);
        for (bit = 8; bit <= 12; ++bit) {
            game_CLR_GLOBALBIT(bit);
            if (GameStruct.NumPlayers > 1 &&
                player_two_model == bit - 8) {
                game_SET_GLOBALBIT(bit);
            }
        }
        menuVars.subplayers[1] = menuVars.pplayers[1];
        GameStruct.gameMode = 0;
        menuVars.menuMode[0] = 0;
        menuVars.menuMode[4] = 0;
        if (menuVars.titleArt == 0) {
            menuVars.titleArt = 1;
        }
        menuVars.titleDispEnable |= menuVars.titleArt;
        if (menuVars.autoLoad == 0) {
            menu_pushMenu(0);
        }
        if ((menuVars.autoLoad == 0 ||
             GameStruct.xaNum != 1 ||
             GameStruct.xaFlag == 0) &&
            OptionStruct.Music != 0) {
            stopXA();
            playXA(1, (int)OptionStruct.musicVolume * 2, 1);
        }
        break;
    }
    case 4:
    case 5:
        GameStruct.CurrentLevel = 0;
        menu_pushMenu(0x0e);
        break;
    case 0x0b:
        if (menuVars.titleArt == 0) {
            menuVars.titleArt = 1;
        }
        menuVars.titleDispEnable |= menuVars.titleArt;
        if (jpb_menu_platform_hooks.saveSettingsData != NULL) {
            optionstruct options = OptionStruct;

            jpb_menu_platform_hooks.saveSettingsData(
                &options, jpb_menu_platform_user_data);
        }
        if ((GameStruct.xaNum != 1 || GameStruct.xaFlag == 0) &&
            OptionStruct.Music != 0) {
            stopXA();
            playXA(1, (int)OptionStruct.musicVolume * 2, 1);
        }
        break;
    case 0x0c:
        savedNumPlayer = GameStruct.NumPlayers;
        break;
    case 0x0d:
        savedNumPlayer = GameStruct.NumPlayers;
        GameStruct.NumPlayers = (char)tempPlayersVs;
        break;
    case 0x10:
        if (OptionStruct.musicVolume > 74u) {
            OptionStruct.musicVolume = 75;
        }
        menuVars.sfxVolume = OptionStruct.SFXVolume;
        if (OptionStruct.SFXVolume > 74u) {
            OptionStruct.SFXVolume = 75;
            menuVars.sfxVolume = 75;
        }
        break;
    case 0x13:
        menu_initCredits();
        break;
    case 0x1a:
        menu_scanAllLevels();
        menu_initLevelSelectScreen();
        break;
    case 0x1b:
        menuVars.scoreScore = 0;
        break;
    case 0x27:
        menuVars.yflag = 0;
        menu_initScoreScreen();
        break;
    case 0x90:
        if (GameStruct.continueAble == 0) {
            menuVars.mmFlags = 0;
            menu_popMenu();
            (void)menu_handleMenuTriggers(9);
        }
        break;
    case 0x9f:
        switch (OptionStruct.Language) {
        case 0:
            menu_initEULA(JPB_EULA_ENG_LINE_COUNT);
            break;
        case 1:
            menu_initEULA(JPB_EULA_GER_LINE_COUNT);
            break;
        case 2:
            menu_initEULA(JPB_EULA_FRE_LINE_COUNT);
            break;
        case 3:
            menu_initEULA(JPB_EULA_ITA_LINE_COUNT);
            break;
        case 4:
            menu_initEULA(JPB_EULA_SPA_LINE_COUNT);
            break;
        case 5:
            menu_initEULA(JPB_EULA_RUS_LINE_COUNT);
            break;
        case 6:
            menu_initEULA(JPB_EULA_ZHO_LINE_COUNT);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/* 0xCA650, 49 bytes, global, 0 named locals
 * menu_initPlayerSelect
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initPlayerSelect(void)
{
    menuVars.scoreDst = 0;
    menuVars.pselectMode[0].mode = 0;
    menuVars.pselectMode[1].mode = 0;
    menuVars.holdButtFlag = 0;
    menuVars.pSelect = 0;
    menuVars.pplayers[0] = 0;
    menuVars.pplayers[1] = 3;
    menuVars.subplayers[0] = 0;
    menuVars.subplayers[1] = 3;
    newMenu_playerSelectTypeP1 = 0;
    newMenu_currentModelSelectBaseP1 = obi_wan_model;
    newMenu_currentModelSelectNGPP1 = pilot_model;
    newMenu_playerSelectTypeP2 = 0;
    newMenu_currentModelSelectBaseP2 = qui_gon_model;
    newMenu_currentModelSelectNGPP2 = rifle_model;
    GameStruct.ModelSelect[0] = obi_wan_model;
    GameStruct.ModelSelect[1] = qui_gon_model;
}

/* 0xCA690, 3 bytes, global, 0 named locals
 * menu_initReconnect
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initReconnect(void)
{
}

/* 0xCA6A0, 1299 bytes, global, 10 named locals
 * menu_initScoreScreen
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initScoreScreen(void)
{
    static uint8_t *const movers[4] = {
        frameBottomMover,
        frameRightMover,
        frameLeftMover,
        frameTopMover
    };
    unsigned current_level = GameStruct.CurrentLevel;
    unsigned award_level;
    unsigned player;

    prim_gSetBkColor(0, 0, 0);
    for (player = 0;
         player < (unsigned)(uint8_t)GameStruct.NumPlayers;
         ++player) {
        if (GameStruct.ModelSelect[player] >= 23) {
            char *name;
            int texture;

            (void)newMenu_GetVSExtraPlayer(
                &name, &texture, GameStruct.ModelSelect[player]);
        }
    }
    menuVars.bgWidth = (uint16_t)OptionStruct.ScreenWidth;
    menuVars.bgHeight = (uint16_t)OptionStruct.ScreenHeight;
    menuVars.memBGptr = maMemoryBanks[2].pMemPool;
    maMemoryBanks[2].memUsed = 0;
    menuVars.mloadShift = 0;
    menuVars.titleDispEnable = 1;
    GameStruct.continueAble = 1;
    menuVars.mmvTriggers[0] = 1;
    if (menuVars.mmvCount < 6) {
        unsigned mover;

        for (mover = 0; mover < 4; ++mover) {
            MMVDEF *control = &menuVars.mmv[mover];

            control->mmvSrc = movers[mover];
            control->mmvPtr = 0;
            control->mmvCounter = 0;
            control->mmvX = 0;
            control->mmvY = 0;
            control->mmvMenu = NULL;
            control->state = 1;
        }
    }

    menuVars.scoreScore = 0;
    menuVars.scoreMode = 0;
    menuVars.scoreCurrentPlayer = 0;
    menuVars.scoreBeeper = 0;
    if (current_level == 15) {
        award_level = 6;
    } else if (current_level == 0) {
        award_level = 1;
    } else {
        award_level = current_level;
    }
    menuVars.scoreoLevel = (uint16_t)current_level;

    for (player = 0;
         player < (unsigned)(uint8_t)GameStruct.NumPlayers;
         ++player) {
        AWARDSET *set = &menuVars.awardSet[player];
        uint32_t flags = (uint32_t)jedi_GetAwardFlags(
            (int)player, GameStruct.aCharacterData[player].Score);
        unsigned tier;

        menuVars.awards[player] = flags;
        menuVars.awardLevel[player] = 0;
        memset(menuVars.awardOrder[player], 0,
               sizeof(menuVars.awardOrder[player]));
        memset(set, 0, sizeof(*set));
        for (tier = 0; tier < 3; ++tier) {
            uint32_t extra = tier == 2
                ? (uint32_t)(award[award_level][3] | UINT16_C(0x7000))
                : 0;
            uint32_t award_value =
                (uint32_t)award[award_level][tier] | extra;

            if ((flags & award_value) != 0) {
                uint8_t points_level;

                set->award[tier] = award_value;
                ++set->awardTotal;
                if (tier == 2 &&
                    (flags & (uint32_t)(
                        award[award_level][2] |
                        award[award_level][3])) != 0) {
                    points_level = pointLvls[award_level][3];
                } else {
                    points_level = pointLvls[award_level][tier];
                }
                set->pointAwarded[tier] =
                    (uint32_t)points_level * 1000u;
                if (set->maxpointaward < set->pointAwarded[tier]) {
                    set->maxpointaward = set->pointAwarded[tier];
                }
            }
            if ((award_value & 3u) == 0) {
                set->awardType[tier] =
                    (uint8_t)(~((uint8_t)award_value >> 1) & 2u);
            } else {
                set->awardType[tier] = 1;
            }
            if (GameStruct.NumPlayers == 2) {
                set->pointAwarded[tier] >>= 1;
            }
        }
    }
    testcombo(0);
}

/* 0xCABC0, 3 bytes, global, 0 named locals
 * menu_initTitleLoad
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initTitleLoad(void)
{
}

/* 0xCABD0, 3 bytes, global, 1 named locals
 * menu_initialLoad
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initialLoad(unsigned card)
{
    (void)card;
}

/* 0xCABE0, 60 bytes, global, 0 named locals
 * menu_killLoadScreen
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCAC20, 15 bytes, global, 0 named locals
 * menu_levelSelect
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_killLoadScreen(void)
{
    if (loadScreenFlag != 0) {
        loadTotal += UINT32_C(10000000);
        menu_redrawLoadscreen();
        __RenderLoad(1);
    }
    jpb_menu_load_screen_active = 0;
    loadScreenFlag = 0;
}
void menu_levelSelect(void)
{
    GameStruct.gameMode = 4;
    GameStruct.inMenuFlag = 0;
}

/* 0xCAC30, 449 bytes, global, 6 named locals
 * menu_levelSelectMenu
 * PDB type: void (unsigned*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_levelSelectMenu(uint32_t *mdef)
{
    uint32_t pad;
    unsigned level;

    pad = menuVars.pad[0] | menuVars.pad[1];
    level = (unsigned)(uint8_t)LevelSelect;
    if ((pad & JPB_PAD_UP) != 0) {
        level = level <= 1 ? 14u : level - 1u;
        menu_checkMiniMod(level, 1);
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xlvbrows", jpb_menu_platform_user_data);
        }
    } else if ((pad & JPB_PAD_DOWN) != 0) {
        level = level >= 14 ? 1u : level + 1u;
        menu_checkMiniMod(level, 0);
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xlvbrows", jpb_menu_platform_user_data);
        }
    }
    if ((pad & JPB_PAD_COMBO_SOUTH) != 0) {
        (void)menu_handleMenuTriggers((int)mdef[12]);
    } else if ((pad & JPB_PAD_JUMP) != 0) {
        menu_menuExit();
    }
}

/* 0xCAE00, 3 bytes, global, 1 named locals
 * menu_loadFrontEndArt
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_loadFrontEndArt(unsigned nothing)
{
    (void)nothing;
}

/* 0xCAE10, 3 bytes, global, 0 named locals
 * menu_loadGame
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_loadGame(void)
{
}

/* 0xCAE20, 8 bytes, global, 0 named locals
 * menu_mainExitMenu
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_mainExitMenu(void)
{
    menuVars.titleDispEnable = 0;
}

/* 0xCAE30, 330 bytes, global, 1 named locals
 * menu_mainInitMenu
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_mainInitMenu(unsigned mode)
{
    if (mode != 0) {
        return;
    }
    text_gInitialise(0xa0);
    if (jpb_menu_platform_hooks.initInput != NULL) {
        jpb_menu_platform_hooks.initInput(
            jpb_menu_platform_user_data);
    }
    memset(&menuVars, 0, sizeof(menuVars));
    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(menuTextures, 0, sizeof(menuTextures));
    GameStruct.ModelSelect[1] = 1;
    menuVars.mmColorSelect = 14;
    menuVars.mmColorNotSelect = 15;
    menuVars.movieSelect = -1;
    if (menuTexLoaded == 0) {
        if (jpb_TextureHasLoadHook()) {
            menu_winLoadTextures();
        } else if (jpb_menu_platform_hooks.loadTextures != NULL) {
            jpb_menu_platform_hooks.loadTextures(
                jpb_menu_platform_user_data);
        }
    }
    menu_enterTitleMode();
    if (jpb_menu_platform_hooks.initBucket != NULL) {
        jpb_menu_platform_hooks.initBucket(
            jpb_menu_platform_user_data);
    }
    if (jpb_menu_platform_hooks.closeBucketLog != NULL) {
        jpb_menu_platform_hooks.closeBucketLog(
            jpb_menu_platform_user_data);
    }
}

/* 0xCAF80, 9688 bytes, global, 78 named locals
 * menu_mainLoop
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void menu_drawComboDebugList(void)
{
    playerObject *player = &gaPlayerData[menuVars.jediDebugCombo];
    Combo *combos = player->paCombos;
    unsigned combo;
    int x = 40;
    int y = 222;

    combo_InitComboData(player);
    for (combo = 0; combo < mp1ComboCount; ++combo) {
        int current = (int)combo;
        int chain_available = 0;
        int tint;
        char combo_string[64];
        char display[64];

        for (;;) {
            int previous = combos[current].prev;

            if (previous == -1) {
                chain_available = 1;
                break;
            }
            if (game_getCombo(
                    (uint32_t)(int)GameStruct.ModelSelect[
                        menuVars.jediDebugCombo],
                    (uint32_t)previous) == 0) {
                break;
            }
            current = previous;
        }
        if (!chain_available) {
            continue;
        }

        tint = game_getCombo(
            (uint32_t)(int)GameStruct.ModelSelect[
                menuVars.jediDebugCombo], combo) == 0
            ? 1
            : 11;
        if (combo_ValidComboAward(
                (int)menuVars.jediDebugCombo, (int)combo) == 0) {
            (void)strcpy(display, "NOT ELIGABLE");
        } else {
            menu_buildComboString(
                (unsigned char *)combo_string,
                (unsigned char *)combos[
                    menuVars.td.comboList[combo * 2u]].String,
                combo);
            (void)sprintf(
                display, "%02d:%02d:%s",
                combo,
                menuVars.td.comboList[combo * 2u + 1u],
                combo_string);
        }
        (void)textWrite(tint, 0, x, y, "%s", display);
        y = (int)(scaleAdjustmentMM * 60.0f + (float)y);
        if (y > 400) {
            y = 62;
            x = 256;
        }
    }
}

static uint32_t *menu_mainLoopDefinition(uint16_t mode)
{
    switch (mode) {
    case 0:
        if (GameStruct.continueAble != 0 &&
            GameStruct.difficulty >= 0) {
            return m_canShowRegisterGame == 1
                ? continuemainMdef
                : continuemainMdefNoRegisterGame;
        }
        return m_canShowRegisterGame == 1
            ? mainMdef
            : mainMdefNoRegisterGame;
    case 1:
        return startMdef;
    case 3:
        return titlePlayerCountMdef;
    case 4:
        return playerCountSelectMdef;
    case 0x0b:
        return optionsMdef;
    case 0x0f:
        return loadMenuMdef;
    case 0x10:
        return GameStruct.gameMode == 6 ||
                       GameStruct.gameMode == 7
            ? audioMdef_Game
            : audioMdef;
    case 0x14:
        return gamepauseMenuMdef;
    case 0x15:
        return cameraMenuMdef;
    case 0x17:
        return movieMenuMdef;
    case 0x25:
        return editMenuMdef;
    case 0x28:
        return aidebugMenuMdef;
    case 0x29:
        return comboDebugMenuMdef;
    case 0x2e:
        return rusureMenuMdef;
    case 0x2f:
        return debugMdef;
    case 0x30:
        return memcarddebugMdef;
    case 0x2d:
        GameStruct.GameState |= UINT32_C(0x02000000);
        return gameoverMdef;
    case 0x2a:
        mini2_keyboardOverride = GameStruct.CurrentLevel == 12;
        GameStruct.GameState |= UINT32_C(0x02000000);
        return objectiveMenuMdef;
    case 0x2b:
        GameStruct.GameState |= UINT32_C(0x02000000);
        return specialMessMenuMdef;
    case 0x2c:
        GameStruct.GameState |= UINT32_C(0x02000000);
        return specialMessMenu2Mdef;
    case 0x37:
        return difficultyMdef;
    case 0x90:
        return newgameconfirmMdef;
    case 0x91:
        return languageMdef;
    case 0x92:
        return rusureQuitMenuMdef;
    case 0x95:
        return videoMdef;
    case 0x99:
        return titlePlayerCountContinueMdef;
    case 0x9c:
        return titlePlayerCountVSMdef;
    default:
        return NULL;
    }
}

void menu_mainLoop(void)
{
    uint32_t *definition;
    uint16_t mode;

    menuVars.menuModeSP &= 7u;
    menu_readControl();
    if (GameStruct.inMenuFlag != 0 && GameStruct.gameMode == 0) {
        menu_startAcceptDecline(
            JPB_PAD_START, JPB_PAD_COMBO_SOUTH);
    }
    mode = menuVars.menuMode[menuVars.menuModeSP];
    if (mode == 0) {
        GameStruct.NumPlayers = 1;
        p2Connected = 0;
        ClearWindow();
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
    }
    if (mode == 4) {
        /* Exact state-four entry republishes the selected player count and
         * its gameplay global bits before drawing playerCountSelectMdef. */
        menu_setNumPlayers((unsigned)(uint8_t)GameStruct.NumPlayers);
    }
    if (mode == 1) {
        float x;
        float y;
        int window_width;
        int window_height;

        if (OptionStruct.EULAaccepted == 0) {
            menu_pushMenu(0x9f);
            goto finish;
        }
        menu_demoMovie();
        menu_mainMenu(startMdef);
        GetWindowSize(&window_width, &window_height);
        (void)window_width;
        (void)window_height;
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        if ((menuVars.fcount & UINT16_C(0x3f)) < 0x1f) {
            const char *prompt = allText[
                lastUsedInputType != 0 ? 460u : 473u];

            x = 0.0f;
            y = 200.0f;
            setPivotPositionMM(&x, &y, 4);
            (void)SDLTextWriteScaleMM(
                15, 255, 2, (int)x, (int)y,
                2.25f, 0, "%s", prompt);
        }
        x = 0.0f;
        y = 180.0f;
        setPivotPositionMM(&x, &y, 7);
        (void)SDLTextWriteScaleMM(
            15, 255, 2, (int)x, (int)y,
            1.5f, 0, "%s", allText[286]);
        x = 0.0f;
        y = 140.0f;
        setPivotPositionMM(&x, &y, 7);
        (void)SDLTextWriteScaleMM(
            15, 255, 2, (int)x, (int)y,
            1.5f, 0, "%s", allText[287]);
        x = 0.0f;
        y = 100.0f;
        setPivotPositionMM(&x, &y, 7);
        (void)SDLTextWriteScaleMM(
            15, 255, 2, (int)x, (int)y,
            1.5f, 0, "%s", allText[288]);
        x = 0.0f;
        y = 60.0f;
        setPivotPositionMM(&x, &y, 7);
        (void)SDLTextWriteScaleMM(
            15, 255, 2, (int)x, (int)y,
            1.5f, 0, "%s", allText[289]);
        goto finish;
    }
    if (mode == 0x0d) {
        int result;

        GameStruct.CurrentLevel = 9;
        result = newMenu_P2CharacterSelect(1);
        if (result < 0) {
            menu_pushMenu(0);
        } else if (result == 1) {
            LevelSelect = 0x19;
            GameStruct.NumPlayers = 2;
            GameStruct.versusModeFlag = 1;
            GameStruct.gameMode = 2;
        }
        goto finish;
    }
    if (mode == 0x0c) {
        if (newMenu_Training() < 0) {
            menu_pushMenu(0);
        }
        goto finish;
    }
    if (mode == 0x94) {
        menu_pushMenu(0);
        goto finish;
    }
    if (mode == 0x0e) {
        int result;

        GameStruct.CurrentLevel = 9;
        result = GameStruct.NumPlayers == 1
            ? newMenu_P1CharacterSelect()
            : newMenu_P2CharacterSelect(0);
        if (result < 0) {
            menu_pushMenu(0);
        } else if (result == 1) {
            menu_pushMenu(0x1a);
        }
        goto finish;
    }
    if (mode == 0x14) {
        if (p1Disconnected != 0 || p2Disconnected != 0) {
            menu_menuExit();
            goto finish;
        }
        (void)cheatCheck(
            cheatCheckPoint, 10u, menu_JumpCheckPoint);
        (void)cheatCheck(
            cheatRadar, 6u, menu_RadarCheat);
        (void)cheatCheckKeyboard(
            cheatCheckPointKeyboard, 10, menu_JumpCheckPoint);
        menu_startAcceptDecline(JPB_PAD_START, JPB_PAD_JUMP);
        if (GameStruct.gameMode == 2 &&
            (padExist & 2u) == 0 &&
            GameStruct.versusModeFlag != 1) {
            menuBox(
                0x9du, 200u,
                ((int)OptionStruct.ScreenWidth >> 1) - 180,
                ((int)OptionStruct.ScreenHeight >> 1) - 12,
                360, 50, 75, 0, 0);
        } else {
            menu_mainMenu(gamepauseMenuMdef);
        }
        goto finish;
    }
    if (mode == 0x1a) {
        ClearWindow();
        if (p1Disconnected == 0) {
            menu_drawLevelSelectScreen(1);
            PresentWindow();
        } else {
            if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
                winDrawBackground(5);
            }
            menu_drawReconnect();
        }
        goto finish;
    }
    if (mode == 0x13) {
        unsigned player;

        ClearWindow();
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        if (p1Disconnected != 0) {
            menu_drawReconnect();
            goto finish;
        }
        menu_drawCredits();
        for (player = 0; player < 2; ++player) {
            if ((menuVars.pad[player] & UINT32_C(0x840)) != 0) {
                menuVars.pad[player] =
                    (menuVars.pad[player] & ~UINT32_C(0x840)) |
                    JPB_PAD_JUMP;
            }
        }
        gColor.cd = 0;
        menu_mainMenu(creditsMdef);
        goto finish;
    }
    if (mode == 0x1b) {
        menuConceptMenu();
        goto finish;
    }
    if (mode == 0x1e) {
        menuVars.pad[0] &= ~UINT32_C(0x20);
        menuVars.pad[1] &= ~UINT32_C(0x20);
        menu_mainMenu(saveNowMdef);
        menu_drawScoreScreen(1);
        goto finish;
    }
    if (mode == 0x1f) {
        menu_mainMenu(saveNowSureMdef);
        menu_drawScoreScreen(1);
        goto finish;
    }
    if (mode == 0x27) {
        menuVars.pad[0] &= ~UINT32_C(0x20);
        menuVars.pad[1] &= ~UINT32_C(0x20);
        menu_drawScoreScreen(0);
        goto finish;
    }
    if (mode == 0x23) {
        ClearWindow();
        if (p1Disconnected != 0 || p2Disconnected != 0) {
            menu_menuExit();
            goto finish;
        }
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        runControlsMenu();
        menuVars.controlPlayer = 0;
        goto finish;
    }
    if (mode == 0x24) {
        ClearWindow();
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        runControlsMenu();
        goto finish;
    }
    if (mode == 0x8f) {
        if (jpb_menu_platform_hooks.openUrl != NULL) {
            jpb_menu_platform_hooks.openUrl(
                "https://ctep.aspyr.com/pb_pc",
                jpb_menu_platform_user_data);
        }
        menu_pushMenu(0);
        goto finish;
    }
    if (mode == 0x9f) {
        switch (OptionStruct.Language) {
        case 0:
            menu_drawEULA(EULA_ENG, JPB_EULA_ENG_LINE_COUNT);
            break;
        case 1:
            menu_drawEULA(EULA_GER, JPB_EULA_GER_LINE_COUNT);
            break;
        case 2:
            menu_drawEULA(EULA_FRE, JPB_EULA_FRE_LINE_COUNT);
            break;
        case 3:
            menu_drawEULA(EULA_ITA, JPB_EULA_ITA_LINE_COUNT);
            break;
        case 4:
            menu_drawEULA(EULA_SPA, JPB_EULA_SPA_LINE_COUNT);
            break;
        case 5:
            menu_drawEULA(EULA_RUS, JPB_EULA_RUS_LINE_COUNT);
            break;
        case 6:
            menu_drawEULA(EULA_ZHO, JPB_EULA_ZHO_LINE_COUNT);
            break;
        default:
            break;
        }
        goto finish;
    }
    if (mode == 0xa0) {
        menu_drawReconnect();
        goto finish;
    }
    if (mode == 0x93) {
        if (jpb_menu_platform_hooks.requestExit != NULL) {
            jpb_menu_platform_hooks.requestExit(
                jpb_menu_platform_user_data);
        }
        goto finish;
    }
    if (mode == 0x9e) {
        ClearWindow();
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        menu_mainMenu(ngpUnlockMdef);
        GameStruct.gameCompleted = 1;
        if (jpb_menu_platform_hooks.saveGameData != NULL) {
            jpb_menu_platform_hooks.saveGameData(
                jpb_menu_platform_user_data);
        }
        goto finish;
    }
    if (mode == 0x10) {
        float x_offset = 0.0f;
        float y_offset = 0.0f;
        float width = 0.45f;
        float y;

        switch (OptionStruct.ResolutionChanged) {
        case 1:
        case 3:
            x_offset = 40.0f;
            y_offset = 1.0f;
            break;
        case 2:
            x_offset = 70.0f;
            y_offset = 1.0f;
            break;
        case 4:
            x_offset = 60.0f;
            y_offset = 3.0f;
            break;
        case 5:
            x_offset = 184.0f;
            y_offset = 1.0f;
            width = 0.40f;
            break;
        case 6:
            x_offset = -115.0f;
            y_offset = 1.0f;
            width = 0.40f;
            break;
        default:
            break;
        }
        ClearWindow();
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        gColor.cd = 0;
        if (GameStruct.gameMode == 6 || GameStruct.gameMode == 7) {
            if (p1Disconnected != 0 || p2Disconnected != 0) {
                menu_menuExit();
                goto finish;
            }
            menu_mainMenu(audioMdef_Game);
            y = y_offset + 460.0f;
        } else {
            menu_mainMenu(audioMdef);
            y = y_offset + 710.0f;
        }
        menu_slideco(
            width, 0.25f,
            (int)(x_offset + 850.0f), (int)y,
            (float)OptionStruct.musicVolume, 75.0f);
        menu_slideco(
            width, 0.25f,
            (int)(x_offset + 850.0f), (int)(y + 60.0f),
            (float)OptionStruct.SFXVolume, 75.0f);
        setMusicVol(
            OptionStruct.Music != 0
                ? (int)OptionStruct.musicVolume
                : 0);
        goto finish;
    }
    if (mode == 0x11) {
        if (p1Disconnected == 0 && p2Disconnected == 0) {
            menu_mainMenu(gamecombosMenu);
            jedi_ShowCombos(tempPlayersVs);
        } else {
            menu_menuExit();
        }
        goto finish;
    }
    if (mode == 0x0b) {
        ClearWindow();
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        gColor.cd = 0;
        menu_mainMenu(optionsMdef);
        jpb_TextClearClipRect();
        if ((padCurrentBits[0].padLevel1 & UINT32_C(0x8000)) != 0 &&
            slider != 0) {
            --slider;
        }
        if ((padCurrentBits[0].padLevel1 & UINT32_C(0x2000)) != 0 &&
            slider < UINT32_C(0xff)) {
            ++slider;
        }
        goto finish;
    }
    if (mode == 0x12) {
        menu_writexainfo(20u, 160u, 0u);
        menu_mainMenu(audioMusicMdef);
        goto finish;
    }
    if (mode == 0x29) {
        menu_drawComboDebugList();
        menu_mainMenu(comboDebugMenuMdef);
        goto finish;
    }
    if (mode == 0x2f) {
        unsigned color;

        menu_mainMenu(debugMdef);
        (void)textWrite(
            11, 0,
            (int)(OptionStruct.ScreenWidth >> 1),
            (int)OptionStruct.ScreenHeight - 80,
            "secret:%x", secretBits);
        for (color = 0; color < 9u; ++color) {
            (void)_DrawText(
                16.0f, 32.0f + (float)color * 16.0f,
                0.0001f, 0.5f,
                jedi_GetColour32(color) | UINT32_C(0x7f000000),
                "%u", color);
        }
        goto finish;
    }
    if (mode == 0x31) {
        menu_mainMenu(memoryMdef);
        menu_dumpMemory(160u, 20u);
        goto finish;
    }
    if (mode == 0x26) {
        ClearWindow();
        menu_runTitleLoad();
        goto finish;
    }
    if (mode == 0x32) {
        menuVars.menuMode[menuVars.menuModeSP & 7u] = 0;
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        goto finish;
    }
    if (mode == 3 || mode == 0x37 || mode == 0x90 ||
        mode == 0x91 || mode == 0x92 || mode == 0x95 ||
        mode == 0x99 || mode == 0x9c) {
        ClearWindow();
        if (mode == 3 || mode == 0x37 || mode == 0x90 ||
            mode == 0x99 || mode == 0x9c) {
            menu_startAcceptDecline(
                JPB_PAD_START, JPB_PAD_COMBO_SOUTH);
        }
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        definition = menu_mainLoopDefinition(mode);
        if (definition != NULL) {
            menu_mainMenu(definition);
        }
        if (mode == 3 || mode == 0x37 ||
            mode == 0x99 || mode == 0x9c) {
            PresentWindow();
        }
        goto finish;
    }
    if (mode == 0x3f || mode == 0x40 || mode == 0x41) {
        if (mode == 0x40) {
            GameStruct.letterboxFlag = GameStruct.letterboxFlag2;
            if (GameStruct.letterboxFlag2 != 0) {
                game_setLetterBox();
            }
        } else if (mode == 0x41) {
            GameStruct.GameState &= ~UINT32_C(0x02000000);
        }
        menu_continueGame();
        goto finish;
    }
    if (mode == 0x66) {
        ClearWindow();
        menu_redrawLoadscreen();
        goto finish;
    }
    definition = menu_mainLoopDefinition(mode);
    if (definition == NULL) {
        goto finish;
    }
    if (p1Disconnected != 0 ||
        (p2Disconnected != 0 && mode != 0x37)) {
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        menu_drawReconnect();
        goto finish;
    }
    gColor.cd = 0;
    menu_mainMenu(definition);

finish:
    if (mode !=
        (menuVars.menuMode[menuVars.menuModeSP & 7u] & UINT16_C(0x01ff))) {
        menu_initNewMenu();
    }
}

/* 0xCD560, 976 bytes, global, 6 named locals
 * menu_mainMenu
 * PDB type: void (unsigned*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCD930, 104 bytes, global, 1 named locals
 * menu_menuExit
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_menuExit(void)
{
    menu_popMenu();
    maskPadBits(0);
    maskPadBits(1);
}

/* 0xCD9A0, 58 bytes, global, 2 named locals
 * menu_menuMusic
 * PDB type: void (unsigned, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_menuMusic(unsigned track, unsigned loop)
{
    if (OptionStruct.Music != 0) {
        stopXA();
        playXA(
            (int)track,
            (int)OptionStruct.musicVolume * 2,
            (int)loop);
    }
}

/* 0xCD9E0, 3 bytes, global, 0 named locals
 * menu_mkEmptySaveGame
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_mkEmptySaveGame(void)
{
}

/* 0xCD9F0, 644 bytes, local, 4 named locals
 * menu_nextMMV
 * PDB type: void (MMVDEF*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void menu_nextMMV(MMVDEF *control)
{
    for (;;) {
        uint16_t pointer = control->mmvPtr;
        uint8_t *command = control->mmvSrc + pointer;
        unsigned opcode = command[0];

        control->mmvIns = (uint16_t)opcode;
        switch (opcode) {
        case 0x24:
            control->mmvX = (int32_t)(
                ((uint32_t)command[1] |
                 ((uint32_t)command[2] << 8)) << 16);
            control->mmvY = (int32_t)(
                ((uint32_t)command[3] |
                 ((uint32_t)command[4] << 8)) << 16);
            control->mmvCounter = 0;
            break;
        case 0x25: {
            uint8_t duration = command[5];
            int32_t target_x = (int32_t)(
                ((uint32_t)command[1] |
                 ((uint32_t)command[2] << 8)) << 16);
            int32_t target_y = (int32_t)(
                ((uint32_t)command[3] |
                 ((uint32_t)command[4] << 8)) << 16);
            int32_t delta_x = (int32_t)(
                (uint32_t)target_x - (uint32_t)control->mmvX);
            int32_t delta_y = (int32_t)(
                (uint32_t)target_y - (uint32_t)control->mmvY);

            control->mmvCounter = duration;
            control->mmvXvect = delta_x / (int)duration;
            control->mmvYvect = delta_y / (int)duration;
            control->mmvPtr = (uint16_t)(
                pointer + (uint16_t)mmsizes[opcode]);
            control->mmvIns = 0x2a;
            break;
        }
        case 0x26:
        case 0x27:
        case 0x28:
        case 0x29:
        case 0x2b:
            control->mmvCounter = command[1];
            break;
        case 0x2c:
            control->mmvPtr = 0;
            break;
        case 0x2d:
            control->state = 0;
            break;
        case 0x2e:
            control->mmvPtr = (uint16_t)(
                pointer + (uint16_t)mmsizes[opcode]);
            continue;
        case 0x2f:
            /* Retail tests an uninitialized caller-shadow DWORD here.
             * No shipped mover stream contains this opcode; the zero path
             * advances once and returns. */
            break;
        case 0x30:
            control->mmvMenuFlags |= 1u;
            control->mmvPtr = (uint16_t)(
                pointer + (uint16_t)mmsizes[opcode]);
            continue;
        case 0x31:
            control->mmvMenuFlags &= ~1u;
            control->mmvPtr = (uint16_t)(
                pointer + (uint16_t)mmsizes[opcode]);
            continue;
        case 0x32:
            control->mmvMenuFlags |= 2u;
            control->mmvPtr = (uint16_t)(
                pointer + (uint16_t)mmsizes[opcode]);
            continue;
        case 0x33:
            control->mmvMenuFlags &= ~2u;
            control->mmvPtr = (uint16_t)(
                pointer + (uint16_t)mmsizes[opcode]);
            continue;
        case 0x34:
            control->mmvMenu = moverMenus[command[1]];
            control->mmvPtr = (uint16_t)(
                pointer + (uint16_t)mmsizes[opcode]);
            continue;
        case 0x35: {
            uint8_t *trigger_bytes =
                (uint8_t *)(void *)&menuVars + 0x231;

            trigger_bytes[command[1]] = 1;
            break;
        }
        case 0x36: {
            const uint8_t *trigger_bytes =
                (const uint8_t *)(const void *)&menuVars + 0x231;

            if (trigger_bytes[command[1]] != 0) {
                control->mmvCounter = 2;
            }
            break;
        }
        case 0x37: {
            uint8_t *trigger_bytes =
                (uint8_t *)(void *)&menuVars + 0x231;

            trigger_bytes[command[1]] = 0;
            break;
        }
        case 0x39: {
            unsigned sound = command[1] < 11u ? command[1] : 0u;

            if (sound != 0) {
                (void)sound_PlayController(
                    NULL, 0, menu_soundList[sound], 8);
            }
            control->mmvPtr = (uint16_t)(
                pointer + (uint16_t)mmsizes[opcode]);
            continue;
        }
        case 0x3a: {
            uint8_t condition = command[1];

            if (condition != 0x3c || GameStruct.NumPlayers != 2) {
                do {
                    pointer = (uint16_t)(
                        pointer +
                        (uint16_t)mmsizes[control->mmvIns]);
                    control->mmvPtr = pointer;
                    command = control->mmvSrc + pointer;
                    control->mmvIns = command[0];
                } while (command[0] != 0x3b ||
                         command[1] != condition);
            }
            break;
        }
        default:
            break;
        }
        control->mmvPtr = (uint16_t)(
            control->mmvPtr +
            (uint16_t)mmsizes[control->mmvIns]);
        return;
    }
}

/* 0xCDC80, 23 bytes, global, 1 named locals
 * menu_nukePrimo
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_nukePrimo(void)
{
    if (maCurrentOT != NULL) {
        (void)ClearOTagR(maCurrentOT, 0x400);
    }
}

/* 0xCDCA0, 3 bytes, global, 1 named locals
 * menu_overLifeBars
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_overLifeBars(unsigned y)
{
    (void)y;
}

/* 0xCDCB0, 91 bytes, global, 1 named locals
 * menu_popMenu
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_popMenu(void)
{
    unsigned next_stack_pointer;
    uint16_t previous_menu;

    feedback_startEffect(0, 14);
    next_stack_pointer = (menuVars.menuModeSP - 1u) & 7u;
    previous_menu = menuVars.menuMode[next_stack_pointer];
    if (previous_menu == 0x32) {
        return;
    }
    if (previous_menu == 0x90) {
        menuVars.menuModeSP =
            (menuVars.menuModeSP - 2u) & 7u;
    } else {
        menuVars.menuModeSP = next_stack_pointer;
    }
}

/* 0xCDD10, 107 bytes, global, 1 named locals
 * menu_pushMenu
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_pushMenu(unsigned menu_id)
{
    unsigned stack_pointer;

    /* User-directed host policy: entering the pre-FMV level transition must
     * not pulse the controller before playback begins. */
    if (menu_id != 0x66) {
        feedback_startEffect(0, 14);
    }
    if ((menu_id == 0x30 &&
         menuVars.memdebugFlag == 0) ||
        menu_id == 0x32) {
        return;
    }
    stack_pointer = (menuVars.menuModeSP + 1u) & 7u;
    menuVars.menuModeSP = stack_pointer;
    menuVars.menuMode[stack_pointer] = (uint16_t)menu_id;
    menuVars.mmSelect1[stack_pointer] = 0;
    menuVars.mmSelect2[stack_pointer] = 0;
}

/* 0xC87E0, 3013 bytes, global, 9 named locals
 * menu_handleMenuTriggers
 * PDB type: unsigned (int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 *
 * The executable jump table is represented in full here. Core game/menu
 * state remains in this exact owner; operations supplied by the retail host
 * (storage, video mode changes, controller enumeration, and level resource
 * work) cross the narrow platform-hook boundary above.
 */
unsigned menu_handleMenuTriggers(int destination)
{
    if (destination >= 0x81 && destination <= 0x86) {
        unsigned trigger = (unsigned)destination - 0x81u;

        menuVars.mmvTriggers[trigger] = 0;
        destination =
            (int)menuVars.mmvTriggerRemap + (int)trigger;
    }
    switch (destination) {
    case 0x93:
        if (jpb_menu_platform_hooks.requestExit != NULL) {
            jpb_menu_platform_hooks.requestExit(
                jpb_menu_platform_user_data);
        }
        break;
    case 0x94:
        menu_menuExit();
        break;
    case 9:
        newGameGameInit();
        menu_pushMenu(3);
        break;
    case 10:
        menuVars.itemSelect = 0;
        menu_pushMenu(0x99);
        break;
    case 0x18: {
        int16_t selected =
            (int16_t)modisorder2[menuVars.subplayers[0]];

        if (selected != GameStruct.ModelSelect[1] ||
            menuVars.pSelect == 0) {
            menuVars.pSelect |= UINT8_C(1);
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedsel", jpb_menu_platform_user_data);
            }
        }
        GameStruct.ModelSelect[0] = selected;
        if (menuVars.pSelect == 3 ||
            GameStruct.NumPlayers == 1) {
            menu_pushMenu(0x1a);
        }
        break;
    }
    case 0x19: {
        int16_t selected =
            (int16_t)modisorder2[menuVars.subplayers[1]];

        if (selected != GameStruct.ModelSelect[0] ||
            menuVars.pSelect == 0) {
            menuVars.pSelect |= UINT8_C(2);
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedsel", jpb_menu_platform_user_data);
            }
        }
        GameStruct.ModelSelect[1] = selected;
        if (menuVars.pSelect == 3 ||
            GameStruct.NumPlayers == 1) {
            menu_pushMenu(0x1a);
        }
        break;
    }
    case 0x33:
        secretBits ^= UINT32_C(1) << menuVars.sbit;
        break;
    case 0x34:
        menu_pushMenu(0x26);
        break;
    case 0x35:
        GameStruct.gameMode = 3;
        GameStruct.inMenuFlag = 0;
        break;
    case 0x36:
        menu_pushMenu(0);
        if (jpb_menu_platform_hooks.cleanupLevelData != NULL) {
            jpb_menu_platform_hooks.cleanupLevelData(
                jpb_menu_platform_user_data);
        }
        break;
    case 0x38:
        menu_sound(menuVars.sndtest);
        break;
    case 0x39:
        menu_triggerMovie((unsigned)(uint8_t)menuVars.movieSelect);
        break;
    case 0x3a:
        return 1;
    case 0x3c:
        menu_scanAllLevels();
        break;
    case 0x3d: {
        int upgrade_level;

        if (!jedi_CheckValidLevel(
                (int)(int8_t)LevelSelect, &upgrade_level)) {
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xlocklvl", jpb_menu_platform_user_data);
            }
            return 0;
        }
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xlvselct", jpb_menu_platform_user_data);
        }
        savedNumPlayer = GameStruct.NumPlayers;
        if (LevelSelect != 0) {
            menu_pushMenu(0x66);
        }
        break;
    }
    case 0x3e:
        GameStruct.gameMode = 2;
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xlvselct", jpb_menu_platform_user_data);
        }
        break;
    case 0x3f:
        menu_continueGame();
        break;
    case 0x40:
        GameStruct.letterboxFlag =
            GameStruct.letterboxFlag2;
        if (GameStruct.letterboxFlag != 0) {
            game_setLetterBox();
        }
        menu_menuMusic(GameStruct.xaNum, 1);
        menu_continueGame();
        break;
    case 0x41:
        GameStruct.GameState &= ~UINT32_C(0x02000000);
        menu_continueGame();
        break;
    case 0x43:
        menu_restartLevel();
        break;
    case 0x44:
        LevelSelect = (char)menuVars.tempLevmod;
        GameStruct.gameMode = 4;
        GameStruct.inMenuFlag = 0;
        break;
    case 0x45:
        GameStruct.gameMode = 9;
        GameStruct.inMenuFlag = 0;
        sound_StopAll();
        break;
    case 0x46:
        if (menuVars.dialogBox1 != 0) {
            menu_endGame();
            break;
        }
        /* FALLTHROUGH */
    case 0x4e:
        menu_menuExit();
        break;
    case 0x48:
        (void)game_gSetGameFlags(UINT32_C(0x00004000));
        menu_continueGame();
        break;
    case 0x49:
        (void)game_gSetGameFlags(UINT32_C(0x00800000));
        menu_continueGame();
        break;
    case 0x4a: {
        unsigned bit = menuVars.aibit;
        abGlobalBits[bit >> 3] ^=
            (uint8_t)(UINT32_C(1) << (bit & 7u));
        break;
    }
    case 0x4b:
        if (jpb_menu_platform_hooks.saveGameData != NULL) {
            jpb_menu_platform_hooks.saveGameData(
                jpb_menu_platform_user_data);
        }
        break;
    case 0x4c:
    case 0x51:
        menuVars.loadSaveMode = 0;
        break;
    case 0x4d: {
        unsigned stack = menuVars.menuModeSP & 7u;
        unsigned selection = menuVars.selectp != NULL
            ? menuVars.selectp[stack]
            : 0u;

        game_enableCombo(
            menuVars.td.jedi,
            menuVars.td.newcombos[selection]);
        menu_setScoreMode(menuVars.scoreNextMode, 2);
        if (menuVars.selectp != NULL) {
            menuVars.selectp[stack] = 0;
        }
        if (jpb_menu_platform_hooks.saveGameData != NULL) {
            jpb_menu_platform_hooks.saveGameData(
                jpb_menu_platform_user_data);
        }
        break;
    }
    case 0x4f:
        menuVars.loadSaveMode = 0;
        menu_pushMenu(
            LevelSelect == 23 || LevelSelect == 14
                ? 0u
                : 0x66u);
        break;
    case 0x50:
        menuVars.loadSaveMode = 8;
        break;
    case 0x52:
    case 0x75:
        break;
    case 0x53:
    case 0x65:
        menu_menuExit();
        break;
    case 0x54:
        if (jpb_menu_platform_hooks.cleanupLevelData != NULL) {
            jpb_menu_platform_hooks.cleanupLevelData(
                jpb_menu_platform_user_data);
        }
        menuVars.titleArt = 1;
        if (p2Connected == 0) {
            GameStruct.NumPlayers = 1;
        }
        GameStruct.gameMode = 9;
        GameStruct.inMenuFlag = 1;
        if (jpb_menu_platform_hooks.setInMenu != NULL) {
            jpb_menu_platform_hooks.setInMenu(
                1, jpb_menu_platform_user_data);
        }
        menu_pushMenu(0);
        break;
    case 0x55:
        LevelSelect = 25;
        GameStruct.versusModeFlag = 1;
        GameStruct.NumPlayers = 2;
        GameStruct.gameMode = 2;
        break;
    case 0x56:
    case 0x57:
    case 0x58:
    case 0x59:
    case 0x5a:
    case 0x5b:
    case 0x5c:
    case 0x5d:
    case 0x5e:
        menu_startTraining((unsigned)destination - 0x56u);
        break;
    case 0x5f:
        (void)game_gSetGameFlags(UINT32_C(0x00008000));
        menu_continueGame();
        break;
    case 0x60:
    case 0x61:
    case 0x62:
        menu_cameraChange((unsigned)destination - 0x60u);
        menu_continueGame();
        break;
    case 0x67:
        if (menuVars.menuMode[menuVars.menuModeSP & 7u] == 0x10) {
            game_setAudioOptions();
        } else if (
            menuVars.menuMode[menuVars.menuModeSP & 7u] == 0x23) {
            if ((menuVars.pad[0] & JPB_PAD_COMBO_SOUTH) != 0) {
                game_setControlsOptions(0);
            }
            if ((menuVars.pad[1] & JPB_PAD_COMBO_SOUTH) != 0) {
                game_setControlsOptions(1);
            }
        }
        break;
    case 0x68:
        GameStruct.difficulty = 0;
        if (GameStruct.NumPlayers == 2 &&
            jpb_menu_platform_hooks.controllerCount != NULL &&
            jpb_menu_platform_hooks.controllerCount(
                jpb_menu_platform_user_data) == 1 &&
            jpb_menu_platform_hooks.singleControllerFallback != NULL) {
            jpb_menu_platform_hooks.singleControllerFallback(
                jpb_menu_platform_user_data);
        }
        menu_pushMenu(4);
        break;
    case 0x69:
        GameStruct.difficulty = 1;
        if (GameStruct.NumPlayers == 2 &&
            jpb_menu_platform_hooks.controllerCount != NULL &&
            jpb_menu_platform_hooks.controllerCount(
                jpb_menu_platform_user_data) == 1 &&
            jpb_menu_platform_hooks.singleControllerFallback != NULL) {
            jpb_menu_platform_hooks.singleControllerFallback(
                jpb_menu_platform_user_data);
        }
        menu_pushMenu(4);
        break;
    case 0x6a:
        GameStruct.NumPlayers = 1;
        menu_pushMenu(0x37);
        break;
    case 0x6b:
        GameStruct.NumPlayers = 2;
        if (jpb_menu_platform_hooks.controllerCount != NULL &&
            jpb_menu_platform_hooks.controllerCount(
                jpb_menu_platform_user_data) == 1 &&
            jpb_menu_platform_hooks.singleControllerFallback != NULL) {
            jpb_menu_platform_hooks.singleControllerFallback(
                jpb_menu_platform_user_data);
        }
        menu_pushMenu(0x37);
        break;
    case 0x6c:
        if (OptionStruct.Music != 0) {
            playXA(
                OptionStruct.xaTrack,
                (int)OptionStruct.musicVolume * 2,
                0);
        }
        break;
    case 0x6d:
        if (OptionStruct.Music != 0) {
            playXA(
                OptionStruct.xaTrack,
                (int)OptionStruct.musicVolume * 2,
                1);
        }
        break;
    case 0x6e:
        stopXA();
        break;
    case 0x6f:
        pauseXA();
        break;
    case 0x70:
        unpauseXA();
        break;
    case 0x77:
        if (jpb_menu_platform_hooks.refreshLevelTransforms != NULL) {
            jpb_menu_platform_hooks.refreshLevelTransforms(
                jpb_menu_platform_user_data);
        }
        break;
    case 0x7a: {
        uint32_t player = menuVars.jediDebugCombo;
        uint32_t combo = menuVars.comboSelect;
        uint32_t jedi =
            (uint32_t)(uint16_t)GameStruct.ModelSelect[player];

        if (game_getCombo(jedi, combo) != 0) {
            game_disableCombo(jedi, combo);
        } else {
            game_enableCombo(jedi, combo);
        }
        break;
    }
    case 0x7d:
    case 0x7f: {
        unsigned stack = menuVars.menuModeSP & 7u;
        unsigned model = (uint16_t)GameStruct.ModelSelect[
            menuVars.scoreCurrentPlayer];
        unsigned score_kind = destination == 0x7d ? 0u : 1u;

        if (destination == 0x7d) {
            GameStruct.maxEnergyLevels[model] = (uint16_t)(
                GameStruct.maxEnergyLevels[model] + 20u);
            GameStruct.maxEnergyLineLength[model] = (uint16_t)(
                GameStruct.maxEnergyLineLength[model] + 5u);
            ++jediUpgrades[model].healthUpgrades;
        } else {
            GameStruct.maxForceLevels[model] = (uint16_t)(
                GameStruct.maxForceLevels[model] + 20u);
            GameStruct.maxForceLineLength[model] = (uint16_t)(
                GameStruct.maxForceLineLength[model] + 5u);
            ++jediUpgrades[model].forceUpgrades;
        }
        menu_setScoreMode(menuVars.scoreNextMode, score_kind);
        if (menuVars.selectp != NULL) {
            menuVars.selectp[stack] = 0;
        }
        if (jpb_menu_platform_hooks.saveGameData != NULL) {
            jpb_menu_platform_hooks.saveGameData(
                jpb_menu_platform_user_data);
        }
        break;
    }
    case 0x87:
        (void)game_gSetEnergy(0, 0);
        break;
    case 0x88:
        (void)game_gSetEnergy(1, 0);
        break;
    case 0x89:
        cheat_nextCheckPoint();
        break;
    case 0x8d:
    case 0x8e: {
        unsigned player = (unsigned)destination - 0x8du;

        if ((menuVars.pad[player] & JPB_PAD_COMBO_SOUTH) == 0) {
            return 0;
        }
        menuVars.controlPlayer = (int32_t)player;
        menu_pushMenu(0x24);
        break;
    }
    case 0x96: {
        menu_applyvideooptions();
        break;
    }
    case 0x97:
        menu_setNumPlayers(1);
        menu_continueGame();
        break;
    case 0x98:
        menu_setNumPlayers(2);
        if (jpb_menu_platform_hooks.controllerCount != NULL &&
            jpb_menu_platform_hooks.controllerCount(
                jpb_menu_platform_user_data) == 1 &&
            jpb_menu_platform_hooks.singleControllerFallback != NULL) {
            jpb_menu_platform_hooks.singleControllerFallback(
                jpb_menu_platform_user_data);
        }
        menu_continueGame();
        break;
    case 0x9a:
        tempPlayersVs = 1;
        GameStruct.NumPlayers = 1;
        menu_pushMenu(0x0d);
        break;
    case 0x9b:
        tempPlayersVs = 2;
        if (jpb_menu_platform_hooks.controllerCount != NULL &&
            jpb_menu_platform_hooks.controllerCount(
                jpb_menu_platform_user_data) == 1 &&
            jpb_menu_platform_hooks.singleControllerFallback != NULL) {
            jpb_menu_platform_hooks.singleControllerFallback(
                jpb_menu_platform_user_data);
        }
        GameStruct.NumPlayers = 1;
        menu_pushMenu(0x0d);
        break;
    case 0x9d:
        menu_pushMenu(0x9c);
        break;
    default:
        menu_pushMenu((unsigned)destination);
        break;
    }
    return 0;
}

/* 0xCDD80, 609 bytes, global, 7 named locals
 * menu_readControl
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_readControl(void)
{
    const uint8_t *keyboard_state = NULL;
    size_t keyboard_count = 0;
    unsigned key;
    int any_key_pressed = 0;

    menuVars.pad[0] = input_ReadControlPad(
        0, 0, &menuVars.oldpad[0]);
    menuVars.pad[1] = input_ReadControlPad(
        1, 0, &menuVars.oldpad[1]);

    menuPushKey();

    if (jpb_menu_platform_hooks.keyboardState != NULL) {
        keyboard_state = jpb_menu_platform_hooks.keyboardState(
            &keyboard_count,
            jpb_menu_platform_user_data);
    }
    if (keyboard_state != NULL) {
        if (keyboard_count > 512u) {
            keyboard_count = 512u;
        }
        for (key = 0; key < keyboard_count; ++key) {
            if (keyboard_state[key] != 0) {
                any_key_pressed = 1;
                if (keyboardKeyPressed == 0) {
                    keyboardBuffer[keyboardBufferIndex] =
                        (unsigned char)key;
                    keyboardBufferIndex = (unsigned char)(
                        (keyboardBufferIndex + 1u) % 10u);
                }
            }
        }
    }
    if (!any_key_pressed) {
        keyboardKeyPressed = 0;
    } else if (keyboardKeyPressed == 0) {
        keyboardKeyPressed = 1;
    }

    if (saverPads[0] == menuVars.pad[0] &&
        saverPads[1] == menuVars.pad[1]) {
        if (screenSaverCount < 18000u) {
            ++screenSaverCount;
            if (screenSaverFlag == 0) {
                saverPads[0] = menuVars.pad[0];
                saverPads[1] = menuVars.pad[1];
                return;
            }
        } else if (screenSaverFlag == 0) {
            saverAlpha = 0;
            screenSaverFlag = 1;
        } else {
            screenSaverFlag = 1;
        }
        saverPads[0] = menuVars.pad[0];
        saverPads[1] = menuVars.pad[1];
        if (saverAlpha < 200u) {
            saverAlpha += 2u;
            return;
        }
    } else {
        screenSaverFlag = 0;
        screenSaverCount = 0;
        menuVars.mcount = 0;
        saverAlpha = 0;
    }
    saverPads[0] = menuVars.pad[0];
    saverPads[1] = menuVars.pad[1];
}

/* 0xCDFF0, 1119 bytes, global, 10 named locals
 * menu_redrawLoadscreen
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_redrawLoadscreen(void)
{
    SCREENRECT destination;
    SCREENRECT source;
    CVECTOR color = {225, 225, 225, 255};
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    int preview_level;
    uint8_t fade;

    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 8);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    _DrawTexture(
        menuTextures[164], destination, NULL, color, 0.9f);
    _DrawTexture(
        menuTextures[165], destination, NULL, color, 0.4f);

    left = 0.0f;
    top = 145.0f;
    setPivotPositionMM(&left, &top, 1);
    (void)SDLTextWriteScaleMM(
        15, 255, 2, (int)left, (int)top,
        2.5f, 0, "%s", allText[158]);

    left = -800.0f;
    top = 225.0f;
    setPivotPositionMM(&left, &top, 7);
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)left, (int)top,
        2.5f, 0, "%s", allText[305 + (int)(int8_t)LevelSelect]);

    left = 116.0f;
    top = 92.0f;
    right = 960.0f;
    bottom = 354.5f;
    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 8);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    preview_level = (int)(int8_t)LevelSelect;
    if (preview_level > 15) {
        preview_level = 15;
    }
    _DrawTexture(
        menuTextures[fontSpec[409 + preview_level].clut],
        destination, NULL, color, 0.5f);

    if (menuVars.artloadPos < 0) {
        menuVars.artloadPos =
            (int16_t)(menuVars.artloadPos + 16);
    } else {
        GameStruct.gameMode = 3;
    }
    fade = (uint8_t)(menuVars.artloadPos - 1);
    color.r = fade;
    color.g = fade;
    color.b = fade;
    color.cd = fade;

    left = 518.0f;
    top = 729.0f;
    right = 999.0f;
    bottom = 796.0f;
    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 0);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    source.left = (int32_t)((float)loadTotal * 0.125f);
    source.top = 0;
    source.right = (int32_t)(
        (float)loadTotal * 0.125f + 967.0f);
    source.bottom = 135;
    _DrawTexture(
        menuTextures[166], destination, &source, color, 0.3f);

    left = 499.0f;
    top = 720.0f;
    right = 1016.0f;
    bottom = 802.0f;
    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 0);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    _DrawTexture(
        menuTextures[168], destination, NULL, color, 0.35f);
    _DrawTexture(
        menuTextures[167], destination, NULL, color, 0.2f);
    menu_fadeBG();
}

/* 0xCE450, 3 bytes, global, 0 named locals
 * menu_resetMemcardFlags
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_resetMemcardFlags(void)
{
}

/* 0xCE460, 8 bytes, global, 0 named locals
 * menu_resetSaveMenu
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_resetSaveMenu(void)
{
    menuVars.loadSaveMode = 0;
}

/* 0xCE470, 15 bytes, global, 0 named locals
 * menu_restartLevel
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_restartLevel(void)
{
    GameStruct.gameMode = 4;
    GameStruct.inMenuFlag = 0;
}

/* 0xCE480, 165 bytes, global, 1 named locals
 * menu_rotControls
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_rotControls(void)
{
    unsigned player;

    for (player = 0; player < 2; ++player) {
        uint32_t pad = menuVars.pad[player];
        uint32_t rotated = pad & ~UINT32_C(0x0000f000);

        if ((pad & JPB_PAD_RIGHT) != 0) {
            rotated |= JPB_PAD_UP;
        }
        if ((pad & JPB_PAD_LEFT) != 0) {
            rotated |= JPB_PAD_DOWN;
        }
        if ((pad & JPB_PAD_UP) != 0) {
            rotated |= JPB_PAD_RIGHT;
        }
        if ((pad & JPB_PAD_DOWN) != 0) {
            rotated |= JPB_PAD_LEFT;
        }
        menuVars.pad[player] = rotated;
    }
}

/* 0xCE530, 88 bytes, global, 0 named locals
 * menu_runTitleLoad
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_runTitleLoad(void)
{
    (void)textWrite(11, 2, 256, 32, allText[184]);
    menu_mainMenu(insert1Mdef);
    (void)textWrite(11, 0, 20, 180, allText[302]);
}

/* 0xCE590, 3 bytes, global, 0 named locals
 * menu_saveGame
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_saveGame(void)
{
}

/* 0xCE5A0, 5 bytes, global, 0 named locals
 * menu_saveGameTriggered
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_saveGameTriggered(void)
{
    if (jpb_menu_platform_hooks.saveGameData != NULL) {
        jpb_menu_platform_hooks.saveGameData(
            jpb_menu_platform_user_data);
    }
}

/* 0xCE5B0, 8 bytes, global, 0 named locals
 * menu_saveMCARDError
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_saveMCARDError(void)
{
    menuVars.loadSaveMode = 0;
}

/* 0xCE5C0, 8 bytes, global, 0 named locals
 * menu_saveMCARDSelect
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_saveMCARDSelect(void)
{
    menuVars.loadSaveMode = 8;
}

/* 0xCE5D0, 104 bytes, global, 2 named locals
 * menu_scanAllLevels
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_scanAllLevels(void)
{
    char previous_level = LevelSelect;
    unsigned level;

    for (level = 0; level < 31; ++level) {
        LevelSelect = (char)level;
        if (jpb_menu_platform_hooks.scanLevel != NULL) {
            jpb_menu_platform_hooks.scanLevel(
                level, jpb_menu_platform_user_data);
        }
    }
    LevelSelect = previous_level;
}

/* 0xCE640, 8 bytes, global, 0 named locals
 * menu_scanProtection
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_scanProtection(void)
{
    menuVars.cardProtectFlag = 1;
}

/* 0xCE650, 149 bytes, global, 2 named locals
 * menu_scoreSmackdown
 * PDB type: unsigned (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
unsigned menu_scoreSmackdown(unsigned target)
{
    if (menuVars.scoreScore < target) {
        if (menuVars.scoreBeeper == 0) {
            (void)sound_PlayController(
                NULL, 0, menu_soundList[10], 8);
        }
        ++menuVars.scoreBeeper;
        if (menuVars.scoreBeeper >= 3) {
            menuVars.scoreBeeper = 0;
        }
        menuVars.scoreScore += 250;
        if (menuVars.bar_y > UINT32_C(0x140000)) {
            menuVars.bar_y -= menuVars.bar_speed;
        }
        if (menuVars.scoreScore < target) {
            return 0;
        }
    }
    menuVars.scoreScore = target;
    return 1;
}

/* 0xCE6F0, 197 bytes, global, 3 named locals
 * menu_screenSaver
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_screenSaver(void)
{
    unsigned pad_one = menuVars.pad[0];
    unsigned pad_two = menuVars.pad[1];
    unsigned alpha;

    if (saverPads[0] != pad_one || saverPads[1] != pad_two) {
        saverPads[0] = pad_one;
        screenSaverFlag = 0;
        screenSaverCount = 0;
        menuVars.mcount = 0;
        saverAlpha = 0;
        saverPads[1] = pad_two;
        return;
    }
    if (screenSaverCount >= 18000u) {
        if (screenSaverFlag == 0) {
            alpha = 0;
            screenSaverFlag = 1;
            saverAlpha = 0;
        } else {
            alpha = saverAlpha;
            screenSaverFlag = 1;
        }
    } else {
        ++screenSaverCount;
        if (screenSaverFlag == 0) {
            saverPads[0] = pad_one;
            saverPads[1] = pad_two;
            return;
        }
        alpha = saverAlpha;
    }
    saverPads[0] = pad_one;
    saverPads[1] = pad_two;
    if (alpha < 200u) {
        saverAlpha = alpha + 2u;
    }
}

/* 0xCE7C0, 7 bytes, global, 1 named locals
 * menu_setCanShowRegisterGame
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int menu_setCanShowRegisterGame(int can_show)
{
    m_canShowRegisterGame = can_show;
    return can_show;
}

/* 0xCE7D0, 3 bytes, global, 1 named locals
 * menu_setDrawSurface
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_setDrawSurface(unsigned surface)
{
    (void)surface;
}

/* 0xCE7E0, 108 bytes, global, 1 named locals
 * menu_setNumPlayers
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_setNumPlayers(unsigned num_players)
{
    if (num_players == 1) {
        if (jpb_menu_platform_hooks.assignBackToP1 != NULL) {
            (void)jpb_menu_platform_hooks.assignBackToP1(
                jpb_menu_platform_user_data);
        }
        GameStruct.NumPlayers = (char)num_players;
        game_CLR_GLOBALBIT(2);
        return;
    }
    GameStruct.NumPlayers = (char)num_players;
    game_SET_GLOBALBIT(2);
    game_CLR_GLOBALBIT(8);
    game_CLR_GLOBALBIT(9);
    game_CLR_GLOBALBIT(10);
    game_CLR_GLOBALBIT(11);
    game_CLR_GLOBALBIT(12);
}

/* 0xCE850, 303 bytes, global, 3 named locals
 * menu_setPlayer
 * PDB type: void (unsigned, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_setPlayer(unsigned player, unsigned model)
{
    unsigned character = model % 80u;
    unsigned base_bit = player == 0 ? 3u : 8u;
    unsigned index;
    int player_active =
        player == 0 ||
        (player == 1 && GameStruct.NumPlayers > 1);

    GameStruct.ModelSelect[player] = (int16_t)model;
    for (index = 0; index < 5; ++index) {
        game_CLR_GLOBALBIT(base_bit + index);
        if (player_active && character == index) {
            game_SET_GLOBALBIT(base_bit + index);
        }
    }
    GameStruct.AIselect[player] = menuVars.pplayers[player];
}

/* 0xCE980, 13 bytes, global, 1 named locals
 * menu_setPointSeek
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_setPointSeek(unsigned point_seek)
{
    menuVars.pointSeek = point_seek - menuVars.scoreScore;
}

/* 0xCE990, 256 bytes, global, 2 named locals
 * menu_setScoreMode
 * PDB type: void (unsigned, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_setScoreMode(unsigned mode, unsigned index)
{
    unsigned stack = menuVars.menuModeSP & 7u;
    unsigned player = menuVars.scoreCurrentPlayer;
    AWARDSET *award = &menuVars.awardSet[player];
    int32_t target;

    (void)index;
    menuVars.mmSelect1[stack] = 0;
    menuVars.mmSelect2[stack] = 0;
    menuVars.scoreBeeper = 0;
    if (menuVars.scoreMode == mode) {
        return;
    }
    menuVars.scoreMode = (uint8_t)mode;
    switch (mode) {
    case 6:
    case 7:
    case 8:
        ++award->awardCount;
        break;
    case 9:
        target = (int32_t)award->pointAwarded[0];
        menuVars.pointSeek =
            (uint32_t)(target - (int32_t)menuVars.scoreScore);
        menuVars.scoreNextMode = 10;
        break;
    case 10:
        target = (int32_t)award->pointAwarded[1];
        menuVars.pointSeek =
            (uint32_t)(target - (int32_t)menuVars.scoreScore);
        menuVars.scoreNextMode = 11;
        break;
    case 11:
        target = (int32_t)award->pointAwarded[2];
        menuVars.pointSeek =
            (uint32_t)(target - (int32_t)menuVars.scoreScore);
        menuVars.scoreNextMode = 1;
        break;
    case 13:
        target = (int32_t)award->pointAwarded[0];
        menuVars.pointSeek =
            (uint32_t)(target - (int32_t)menuVars.scoreScore);
        menuVars.scoreNextMode = 1;
        break;
    default:
        break;
    }
}

/* 0xCEA90, 25 bytes, global, 1 named locals
 * menu_setShockOption
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_setShockOption(unsigned player)
{
    OptionStruct.ShockFlag[player] =
        (uint8_t)((padShockable >> player) & 1u);
}

/* 0xCEAB0, 497 bytes, global, 7 named locals
 * menu_showCouncil
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_showCouncil(void)
{
    FRONTENDVERT vertices[4] = {0};
    float x_offset = 0.0f;
    float y_offset = 0.0f;
    float x_extent = 857.1428f;
    float y_extent = 492.6108f;
    float current_aspect;
    int player;

    if (LevelSelect != 0) {
        return;
    }

    current_aspect =
        (float)OptionStruct.ScreenWidth /
        (float)OptionStruct.ScreenHeight;
    if (current_aspect > 1.7777778f) {
        x_offset =
            ((current_aspect - 1.7777778f) / current_aspect) *
            0.5f * 857.1428f;
        x_extent =
            (857.1428f / current_aspect) * 1.7777778f;
    } else {
        y_offset =
            ((1.7777778f - current_aspect) / 1.7777778f) *
            0.5f * 492.6108f;
        y_extent = current_aspect * 277.0936f;
    }

    vertices[0].x = x_offset;
    vertices[0].y = y_offset;
    vertices[0].u = 0.0f;
    vertices[0].v = 0.0f;
    vertices[0].r = 255;
    vertices[0].g = 255;
    vertices[0].b = 255;
    vertices[0].color = -1;

    vertices[1].x = x_offset + x_extent;
    vertices[1].y = y_offset;
    vertices[1].u = 1.0f;
    vertices[1].v = 0.0f;
    vertices[1].r = 255;
    vertices[1].g = 255;
    vertices[1].b = 255;

    vertices[2].x = x_offset;
    vertices[2].y = y_offset + y_extent;
    vertices[2].u = 0.0f;
    vertices[2].v = 1.0f;
    vertices[2].r = 255;
    vertices[2].g = 255;
    vertices[2].b = 255;

    vertices[3].x = x_offset + x_extent;
    vertices[3].y = y_offset + y_extent;
    vertices[3].u = 1.0f;
    vertices[3].v = 1.0f;
    vertices[3].r = 255;
    vertices[3].g = 255;
    vertices[3].b = 255;

    frontEndPoly(menuTextures[120], 4, vertices, 1.0f);
    for (player = 0; player < JPB_PLAYER_CAPACITY; ++player) {
        playerObject *candidate = &gaPlayerData[player];

        if (candidate->playerRoot.objectID != -1 &&
            (candidate->playerRoot.flags & UINT32_C(0x20)) == 0) {
            menu_councilPos(
                candidate,
                physics_gGetPosition(&candidate->playerRoot));
        }
    }
}

/* 0xCECB0, 3 bytes, global, 0 named locals
 * menu_showGameMode
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_showGameMode(void)
{
}

/* 0xCECC0, 3 bytes, global, 0 named locals
 * menu_showSaves
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_showSaves(void)
{
}

/* 0xCECD0, 18 bytes, global, 1 named locals
 * menu_showVRAMBackground
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_showVRAMBackground(unsigned background_index)
{
    if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
        winDrawBackground((int)background_index);
    }
}

/* 0xCECF0, 573 bytes, global, 13 named locals
 * menu_slideco
 * PDB type: void (float, float, int, int, fl...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_slideco(
    float width,
    float height,
    int x,
    int y,
    float current,
    float maximum)
{
    float ratio = current / maximum;
    float left = (float)x;
    float top = (float)y;
    float right = width * 967.0f + left;
    float bottom = height * 135.0f + top;
    SCREENRECT destination;
    SCREENRECT source;
    CVECTOR white = {255, 255, 255, 255};

    setPivotPositionMM(&left, &top, 0);
    setPivotPositionMM(&right, &bottom, 0);
    source.left = 200;
    source.top = 0;
    source.right = (int32_t)(ratio * 605.0f);
    source.bottom = 135;
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)(
        ratio * 967.0f * width * scaleAdjustmentMM + left);
    destination.bottom = (int32_t)bottom;
    _DrawTexture(
        menuTextures[166], destination, &source, white, 0.002f);

    left -= width * 19.0f * scaleAdjustmentMM;
    right += width * 17.0f * scaleAdjustmentMM;
    top -= height * 9.0f * scaleAdjustmentMM;
    bottom += height * 6.0f * scaleAdjustmentMM;
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    _DrawTexture(
        menuTextures[168], destination, NULL, white, 0.003f);
    _DrawTexture(
        menuTextures[167], destination, NULL, white, 0.001f);
}

/* 0xCEF30, 282 bytes, global, 9 named locals
 * menu_slideco_a
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_slideco_a(
    unsigned pos,
    unsigned width,
    unsigned height,
    unsigned max,
    unsigned x,
    unsigned y,
    unsigned c1,
    unsigned c2)
{
    unsigned filled = pos * width / max;
    uint32_t border_color;
    uint32_t filled_color;
    uint32_t empty_color;

    memcpy(&border_color, &Colors[11], sizeof(border_color));
    memcpy(&filled_color, &Colors[c1], sizeof(filled_color));
    memcpy(&empty_color, &Colors[c2], sizeof(empty_color));
    (void)DrawRectangle(
        (float)(x - 1u),
        (float)(y - 1u),
        (float)(width + 2u),
        (float)(height + 2u),
        (long)border_color);
    (void)DrawRectangle(
        (float)x,
        (float)y,
        (float)filled,
        (float)height,
        (long)filled_color);
    (void)DrawRectangle(
        (float)(x + filled),
        (float)y,
        (float)(width - filled),
        (float)height,
        (long)empty_color);
}

/* 0xCF050, 42 bytes, global, 1 named locals
 * menu_sound
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_sound(unsigned sound)
{
    unsigned selected_sound = sound < 11u ? sound : 0u;

    if (selected_sound != 0) {
        (void)sound_PlayController(
            NULL, 0, menu_soundList[selected_sound], 8);
    }
}

/* 0xCF080, 246 bytes, global, 1 named locals
 * menu_specialMess
 * PDB type: void (unsigned char*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_specialMess(uint8_t *mess)
{
    unsigned stack;

    GameStruct.GameState |= UINT32_C(0x02000000);
    menuVars.specialString = mess;
    GameStruct.inMenuFlag = 1;
    menuVars.titleArt = 0;

    feedback_startEffect(0, 14);
    stack = (menuVars.menuModeSP + 1u) & 7u;
    menuVars.menuModeSP = stack;
    menuVars.menuMode[stack] = UINT16_C(0x41);
    menuVars.mmSelect1[stack] = 0;
    menuVars.mmSelect2[stack] = 0;

    feedback_startEffect(0, 14);
    stack = (menuVars.menuModeSP + 1u) & 7u;
    menuVars.menuModeSP = stack;
    menuVars.menuMode[stack] =
        mess == (uint8_t *)(void *)allText[376]
            ? UINT16_C(0x2c)
            : UINT16_C(0x2b);
    menuVars.mmSelect1[stack] = 0;
    menuVars.mmSelect2[stack] = 0;
}

/* 0xCF180, 49 bytes, global, 2 named locals
 * menu_startAcceptDecline
 * PDB type: void (unsigned, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_startAcceptDecline(unsigned mask, unsigned replacement)
{
    if ((menuVars.pad[0] & mask) != 0) {
        menuVars.pad[0] =
            (menuVars.pad[0] & ~mask) | replacement;
    }
    if ((menuVars.pad[1] & mask) != 0) {
        menuVars.pad[1] =
            (menuVars.pad[1] & ~mask) | replacement;
    }
}

/* 0xCF1C0, 62 bytes, global, 2 named locals
 * menu_startTraining
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_startTraining(unsigned training_level)
{
    int16_t model = GameStruct.ModelSelect[0];

    menuVars.trainingLevel = (uint8_t)training_level;
    LevelSelect = (char)(training_level + 16u);
    GameStruct.gameMode = 2;
    if (GameStruct.continueAble == 0) {
        newGameGameInit();
        GameStruct.ModelSelect[0] = model;
    }
}

/* 0xCF200, 16 bytes, global, 0 named locals
 * menu_tempClearTrigger
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_tempClearTrigger(void)
{
    memset(menuVars.mmvTriggers, 0, sizeof(menuVars.mmvTriggers));
}

/* 0xCF210, 34 bytes, global, 1 named locals
 * menu_triggerMovie
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

void menu_triggerMovie(unsigned movie)
{
    if (jpb_menu_platform_hooks.triggerMovie != NULL) {
        jpb_menu_platform_hooks.triggerMovie(
            movie, 0, jpb_menu_platform_user_data);
    }
}

/* 0xCF240, 4976 bytes, global, 10 named locals
 * menu_winLoadTextures
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static _Material *menu_loadControllerTexture(
    const char *name, ResourceType resource_type)
{
    const char *path = resource_getPathWithExtension(
        name, resource_type, "png");

    return _LoadTexture((char *)path, TT_FRONT, 0);
}

static void menu_loadControllerBank(
    _Material **bank, ResourceType resource_type)
{
    size_t index;

    for (index = 0; index < 8; ++index) {
        bank[index] = menu_loadControllerTexture(
            jpb_AllTextUtf8(0, controlTextList[index]),
            resource_type);
    }
    bank[8] = menu_loadControllerTexture(
        "controller", resource_type);
    bank[9] = NULL;
}

void menu_winLoadTextures(void)
{
    static const char *const level_select_names[15] = {
        "fed", "marsh", "theed", "palace", "tato",
        "corus", "ruins", "streets", "hangar", "core",
        "mini1", "mini2", "mini3", "mini4", "council"
    };
    size_t index;

    {
        const char *path = resource_getPath(
            "white.png", JPB_RESOURCE_DEFAULT);

        whitemat = _LoadTexture(
            (char *)(void *)path, TT_SPRITE, 1);
        whitematAdd = _LoadTexture(
            (char *)(void *)path, TT_SPRITE, 2);
    }

    for (index = 0;
         index < JPB_MENU_TEXTURE_ENTRY_COUNT;
         ++index) {
        const JPBMenuTextureEntry *entry =
            &menuTextureList[index];
        const char *path = resource_getPath(
            entry->filename, JPB_RESOURCE_FRONT);
        _Material *material = _LoadTexture(
            (char *)path, TT_FRONT, 0);

        if (entry->textureIndex >=
                sizeof(menuTextures) / sizeof(menuTextures[0]) ||
            entry->spriteIndex >= JPB_FONT_SPEC_COUNT) {
            continue;
        }
        menuTextures[entry->textureIndex] = material;
        if (material != NULL) {
            FONTSPEC *spec = &fontSpec[entry->spriteIndex];

            spec->xypage = 0;
            spec->clut = entry->textureIndex;
            spec->y = 0;
            spec->x = 0;
            spec->h = (uint16_t)material->ih;
            spec->w = (uint16_t)material->iw;
        }
    }
    menu_publishWinif2FontSpec();

    /* Exact post-table level-preview bank. The matched owner reads
     * fontSpec[410 + LevelSelect].clut; LevelSelect is 1-based, so the
     * first playable preview is published at fontSpec[411]. */
    for (index = 0; index < 15; ++index) {
        char filename[64];
        const char *path;
        _Material *material;
        FONTSPEC *spec = &fontSpec[411u + index];

        (void)snprintf(
            filename,
            sizeof(filename),
            "loadscreens/src/orig/%s.png",
            level_select_names[index]);
        path = resource_getPath(filename, JPB_RESOURCE_FRONT);
        material = _LoadTexture((char *)path, TT_FRONT, 0);
        menuTextures[80u + index] = material;
        if (material != NULL) {
            spec->xypage = 0;
            spec->clut = (uint16_t)(80u + index);
            spec->y = 0;
            spec->x = 0;
            spec->h = (uint16_t)(material->ih / 4);
            spec->w = (uint16_t)(material->iw / 4);
        }
    }

    /* Exact controller-family banks loaded by the matched PC owner after
     * the 132-record front-end table. English strings are deliberately used
     * for filenames; labels below remain localized through allText. */
    menu_loadControllerBank(
        controlTextures, JPB_RESOURCE_CONTROLLER_SECONDARY);
    menu_loadControllerBank(
        kbmTextures, JPB_RESOURCE_KEYBOARD_MOUSE);
    kbmTextures[9] = menu_loadControllerTexture(
        "esc", JPB_RESOURCE_KEYBOARD_MOUSE);
    kbmForceTextures[0] = menu_loadControllerTexture(
        "U_Key", JPB_RESOURCE_KEYBOARD_MOUSE);
    kbmForceTextures[1] = menu_loadControllerTexture(
        "I_Key", JPB_RESOURCE_KEYBOARD_MOUSE);
    kbmForceTextures[2] = menu_loadControllerTexture(
        "O_Key", JPB_RESOURCE_KEYBOARD_MOUSE);
    kbmForceTextures[3] = menu_loadControllerTexture(
        "Y_Key", JPB_RESOURCE_KEYBOARD_MOUSE);
    menu_loadControllerBank(ps4Textures, JPB_RESOURCE_PS4);
    menu_loadControllerBank(ps5Textures, JPB_RESOURCE_PS5);
    menu_loadControllerBank(switchTextures, JPB_RESOURCE_SWITCH);
    menu_loadControllerBank(
        switchProTextures, JPB_RESOURCE_SWITCH_PRO);
    menu_loadControllerBank(
        joyconTextures, JPB_RESOURCE_SWITCH_SECONDARY);
    menu_loadControllerBank(
        xsxTextures, JPB_RESOURCE_XBOX_SERIES_X);

    menuTexLoaded = 1;
}

/* 0xD05B0, 375 bytes, global, 3 named locals
 * menu_writexainfo
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_writexainfo(unsigned y1, unsigned y2, unsigned front)
{
    int x = front != 0 ? 0x100 : 0x14;
    int line = (int)(scaleAdjustmentMM * 60.0f);

    (void)SDLTextWrite(
        11, 0, x, (int)y1, 0,
        "xaNum:%08x", GameStruct.xaNum);
    (void)SDLTextWrite(
        11, 0, x, (int)y1 + line, 0,
        "xaFlag:%08x", GameStruct.xaFlag);
    (void)SDLTextWrite(
        11, 0, x, (int)y2, 0,
        "cd start pos:%08x", GameStruct.xastartPos);
    (void)SDLTextWrite(
        11, 0, x, (int)y2 + line, 0,
        "cd cur pos:%08x", GameStruct.xacurPos);
    (void)SDLTextWrite(
        11, 0, x, (int)y2 + line * 2, 0,
        "cd end pos:%08x", GameStruct.xaendPos);
}

/* 0xD0730, 141 bytes, global, 3 named locals
 * mmDecVar
 * PDB type: void (unsigned*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void mmDecVar(uint32_t *md)
{
    MDEF_MOD *mod;
    unsigned mod_index;
    unsigned value;

    mod_index = md[4];
    mod = &modVars[mod_index];
    value = mmGetModVal(mod);
    if (mod->incspeed <= value) {
        value -= mod->incspeed;
    } else if ((mod->type & UINT16_C(0x0800)) == 0) {
        value += (unsigned)mod->max + 1u;
        value -= mod->incspeed;
    } else {
        return;
    }
    if (value < (unsigned)mod->min) {
        value = (unsigned)mod->max;
    }
    if (mmSetModVal(mod, value, mod_index)) {
        mmUpdateModSet(md, value, 1);
    }
}

/* 0xD07C0, 86 bytes, global, 2 named locals
 * mmDraw
 * PDB type: void (unsigned*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_mainMenu(uint32_t *mdef)
{
    uint32_t pad = 0;
    uint8_t *selectp;
    unsigned sound = 0;
    unsigned stream_index = 0;
    int carousel_clip = 0;

    while (mdef[stream_index] != UINT32_C(0x14)) {
        uint32_t command = mdef[stream_index] & UINT32_C(0x7fff);

        if (command == UINT32_C(0x44)) {
            carousel_clip = 1;
            break;
        }
        stream_index += mmsizes[command];
    }
    menuVars.mmFlags = 0;
    menuVars.controlFlags = 3;
    menuVars.yflag = 0;
    if (menuVars.yoffset < 0) {
        int value = menuVars.yoffset +
            (int)(scaleAdjustmentMM * 4.0f);
        menuVars.yoffset = (int8_t)(value < 1 ? value : 0);
    } else if (menuVars.yoffset > 0) {
        int value = menuVars.yoffset -
            (int)(scaleAdjustmentMM * 4.0f);
        menuVars.yoffset = (int8_t)(value > -1 ? value : 0);
    }
    if (carousel_clip) {
        float clip_left = -379.0f;
        float clip_top = 144.0f;
        float clip_right = 379.0f;
        float clip_bottom = 275.0f;

        setPivotPositionMM(&clip_left, &clip_top, 4);
        setPivotPositionMM(&clip_right, &clip_bottom, 4);
        jpb_TextSetClipRect(
            (int)clip_left,
            (int)clip_top,
            (int)clip_right,
            (int)clip_bottom);
    } else {
        jpb_TextClearClipRect();
    }
    mmDraw(mdef);
    jpb_TextClearClipRect();
    if ((menuVars.controlFlags & 1u) != 0) {
        pad |= menuVars.pad[0];
    }
    if ((menuVars.controlFlags & 2u) != 0) {
        pad |= menuVars.pad[1];
    }
    selectp = menuVars.controlFlags == 2
        ? menuVars.mmSelect2
        : menuVars.mmSelect1;
    if ((menuVars.mmFlags & 1u) == 0 &&
        menuVars.yoffset == 0) {
        if ((pad & JPB_PAD_DOWN) != 0 &&
            menuVars.mmTotal > 1) {
            if (menuVars.yflag != 0) {
                menuVars.yoffset = (int8_t)
                    (scaleAdjustmentMM * 60.0f);
            }
            selectp[menuVars.menuModeSP & 7u] =
                (uint8_t)(
                    (selectp[menuVars.menuModeSP & 7u] + 1u) %
                    menuVars.mmTotal);
            sound = 2;
        }
        if ((pad & JPB_PAD_UP) != 0 &&
            menuVars.mmTotal > 1) {
            unsigned stack = menuVars.menuModeSP & 7u;
            unsigned selection = selectp[stack];

            if (menuVars.yflag != 0) {
                menuVars.yoffset = (int8_t)
                    (scaleAdjustmentMM * -60.0f);
            }
            selectp[stack] = (uint8_t)(
                selection == 0
                    ? menuVars.mmTotal - 1u
                    : selection - 1u);
            sound = 2;
        }
        if (menuVars.mmSelectPtr != NULL &&
            menuVars.mmSelectPtr[0] == 9) {
            if ((pad & JPB_PAD_LEFT) != 0) {
                mmDecVar(menuVars.mmSelectPtr);
                sound = 2;
            }
            if ((pad & JPB_PAD_RIGHT) != 0) {
                mmIncVar(menuVars.mmSelectPtr);
                sound = 2;
            }
        }
    }
    if (menuVars.mmSelectPtr != NULL &&
        (pad & JPB_PAD_COMBO_SOUTH) != 0) {
        uint32_t *selected_item = menuVars.mmSelectPtr;
        uint32_t destination = selected_item[3];

        (void)menu_handleMenuTriggers((int)destination);

        if (jpb_menu_platform_hooks.activateItem != NULL) {
            (void)jpb_menu_platform_hooks.activateItem(
                destination,
                jpb_menu_platform_user_data);
        }
        sound = 1;
    } else if ((pad & JPB_PAD_JUMP) != 0) {
        menu_popMenu();
        sound = 1;
    }
    if (sound != 0) {
        menu_sound(sound);
    }
}
void mmDraw(uint32_t *md)
{
    unsigned index = 0;

    menuVars.mmAnchorType = 4;
    menuVars.selectp = menuVars.mmSelect1;
    menuVars.textScale = 2.25f;
    menuVars.textSpacer = 60.0f;
    while (md[index] != UINT32_C(0x14)) {
        index = mmDrawsub(md, index);
    }
}

/* 0xD0820, 3 bytes, global, 4 named locals
 * mmDrawCard
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void mmDrawCard(
    unsigned card_number,
    unsigned value,
    unsigned x,
    unsigned y)
{
    (void)card_number;
    (void)value;
    (void)x;
    (void)y;
}

/* 0xD0830, 1003 bytes, global, 9 named locals
 * mmDrawItem
 * PDB type: int (unsigned*, unsigned char*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int mmDrawItem(uint32_t *md, uint8_t *dstbuffer)
{
    char display[256];
    const char *source;
    unsigned text_index;
    unsigned selected;
    unsigned color;
    int is_selected;
    int is_active_selected;
    int draw_font_style;
    int y;

    selected = menuVars.selectp[menuVars.menuModeSP & 7u];
    is_selected = md[1] - menuVars.mmSubSet == selected;
    is_active_selected =
        is_selected &&
        (menuVars.mmFlags & 1u) == 0 &&
        menuVars.yoffset == 0;
    color = menuVars.mmColorNotSelect;
    if (is_active_selected) {
        color = menuVars.mmColorSelect;
        if (menuVars.mmTotal != 0) {
            menuVars.mmSelectPtr = md;
        }
    }
    if (md[2] == 2) {
        if (dstbuffer != NULL) {
            dstbuffer[0] = 0;
        }
        return 0;
    }
    if (md[0] == UINT32_C(0x19) ||
        md[0] == UINT32_C(0x1a)) {
        source = (const char *)menuVars.specialString;
    } else {
        text_index = md[2];
        if (lastUsedInputType == 0) {
            switch (text_index) {
            case 239:
                text_index = 476;
                break;
            case 240:
                text_index = 474;
                break;
            case 241:
                text_index = 475;
                break;
            case 221:
                text_index = 477;
                break;
            default:
                break;
            }
        }
        source = allText[text_index];
    }
    if (md[0] == UINT32_C(0x19) &&
        (menuVars.mmFlags & 1u) == 0 &&
        (menuVars.mmFlags & 8u) != 0 &&
        color == 11u) {
        color = 12;
    }
    if (dstbuffer != NULL) {
        size_t index = 0;
        size_t output = 0;

        if (is_active_selected) {
            dstbuffer[output++] = '>';
            dstbuffer[output++] = ' ';
        }
        while (source[index] != '\0' && output < 254) {
            dstbuffer[output++] = (uint8_t)source[index++];
        }
        if (output < 255) {
            dstbuffer[output++] = ' ';
        }
        dstbuffer[output] = 0;
        return (int)output;
    }
    if (is_active_selected) {
        (void)snprintf(
            display,
            sizeof(display),
            "> %s <",
            source);
    } else {
        (void)snprintf(
            display,
            sizeof(display),
            "%s",
            source);
    }
    y = (int32_t)menuVars.mmY;
    if (menuVars.yflag == 1) {
        y += menuVars.yoffset;
    }
    draw_font_style =
        menuVars.mmItalics != 0 &&
        menuVars.mmBrackets != 0 &&
        color == menuVars.mmColorSelect
            ? 1
            : 0;
    if (menuTextDepthOverride >= 0.0f) {
        return SDLTextWriteScaleMMDepth(
            (int)color,
            255,
            (int)menuVars.mmTextType,
            (int32_t)menuVars.mmX,
            y,
            menuVars.textScale,
            draw_font_style,
            menuTextDepthOverride,
            "%s",
            display);
    }
    return SDLTextWriteScaleMM(
        (int)color,
        255,
        (int)menuVars.mmTextType,
        (int32_t)menuVars.mmX,
        y,
        menuVars.textScale,
        draw_font_style,
        "%s",
        display);
}

/* 0xD0C20, 3 bytes, global, 2 named locals
 * mmDrawMisc
 * PDB type: void (MDEF_MOD*, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void mmDrawMisc(MDEF_MOD *modifier, unsigned value)
{
    (void)modifier;
    (void)value;
}

/* 0xD0C30, 859 bytes, global, 11 named locals
 * mmDrawMod
 * PDB type: void (unsigned*, unsigned char*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void mmDrawMod(uint32_t *md, uint8_t *dstbuffer)
{
    char prefix[256];
    char resolution[64];
    char display[512];
    const char *value_text = NULL;
    MDEF_MOD *mod;
    unsigned mod_index;
    unsigned value;
    unsigned color;
    unsigned selected;
    int active;
    size_t index;
    int y;

    mod_index = md[4];
    mod = &modVars[mod_index];
    value = mmGetModVal(mod);
    selected = menuVars.selectp[menuVars.menuModeSP & 7u];
    active = md[1] - menuVars.mmSubSet == selected &&
        (menuVars.mmFlags & 1u) == 0 &&
        menuVars.yoffset == 0;
    color = active
        ? menuVars.mmColorSelect
        : menuVars.mmColorNotSelect;
    if (active && menuVars.mmTotal != 0) {
        menuVars.mmSelectPtr = md;
    }
    for (index = 0;
         index + 1 < sizeof(prefix) / sizeof(prefix[0]) &&
         dstbuffer[index] != 0;
         ++index) {
        prefix[index] = (char)dstbuffer[index];
    }
    prefix[index] = '\0';

    if ((mod->type & UINT16_C(0x8000)) != 0) {
        unsigned text_base = mod->text & UINT16_C(0x7fff);
        unsigned text_index = text_base + value;

        if (mod->text == 2) {
            return;
        }
        value_text = allText[text_index];
        (void)snprintf(
            display,
            sizeof(display),
            "%s%s%s",
            prefix,
            value_text,
            active ? " <" : "");
    } else if ((mod->type & UINT16_C(6)) != 0) {
        mod->max = g_resolutionsCount - 1;
        value = mmGetModVal(mod);
        (void)snprintf(
            resolution,
            sizeof(resolution),
            "%dx%d",
            g_resolutions[value].width,
            g_resolutions[value].height);
        (void)snprintf(
            display,
            sizeof(display),
            active ? "> %s %s <" : "%s %s",
            allText[445],
            resolution);
    } else {
        if ((mod->type & UINT16_C(0x4000)) != 0) {
            return;
        }
        (void)snprintf(
            display,
            sizeof(display),
            "%s%u%s",
            prefix,
            value,
            active ? " <" : "");
    }
    y = (int32_t)menuVars.mmY;
    if (menuVars.yflag == 1) {
        y += menuVars.yoffset;
    }
    (void)SDLTextWriteScaleMM(
        (int)color,
        255,
        (int)menuVars.mmTextType,
        (int32_t)menuVars.mmX,
        y,
        menuVars.textScale,
        menuVars.mmItalics != 0 ? 1 : 0,
        "%s",
        display);
}

/* 0xD0F90, 3459 bytes, global, 12 named locals
 * mmDrawsub
 * PDB type: unsigned (unsigned*, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
unsigned mmDrawsub(uint32_t *md, unsigned index)
{
    uint32_t command;
    unsigned selected;

    command = md[index];
    selected = menuVars.selectp[menuVars.menuModeSP & 7u];
    switch (command) {
    case 0:
        menuVars.mmColorSelect = 14;
        menuVars.mmColorNotSelect = 15;
        menuVars.mmTotal = md[index + 1];
        menuVars.mmSubSet = 0;
        menuVars.mmBrackets = 0;
        if (menuVars.mmTotal != 0) {
            menuVars.mmSelectPtr = NULL;
        }
        break;
    case 1: {
        unsigned player;

        for (player = 0; player < 2; ++player) {
            uint32_t pad = menuVars.pad[player];
            uint32_t directions =
                ((pad & UINT32_C(0x8000)) >> 3) |
                ((pad & UINT32_C(0x2000)) << 1) |
                ((pad & UINT32_C(0x1000)) << 3) |
                ((pad & UINT32_C(0x4000)) >> 1);

            menuVars.pad[player] =
                (pad & UINT32_C(0xffff0fff)) | directions;
        }
        break;
    }
    case 2:
        menuVars.mmFlags =
            menuVars.mmvCurrentMenuControl->mmvMenuFlags;
        break;
    case 3:
    case 4:
    case 0x3e:
    case 0x3f:
    case 0x40:
    case 0x41: {
        float x = (float)md[index + 1];
        float y = (float)md[index + 2];

        if (command == 0x3e || command == 0x40) {
            x = -x;
        }
        if (command >= 0x3f && command <= 0x41) {
            y = -y;
        }
        setPivotPositionMM(&x, &y, menuVars.mmAnchorType);
        menuVars.mmX = (uint32_t)(int32_t)x;
        menuVars.mmY = (uint32_t)(int32_t)y;
        if (command == 4 || command == 0x41) {
            menuVars.mmY = (uint32_t)(
                (int32_t)menuVars.mmY -
                (int32_t)((float)selected *
                          scaleAdjustmentMM * 60.0f));
        }
        break;
    }
    case 5:
        menuVars.mmX = (uint32_t)
            menuVars.mmvCurrentMenuControl->mmvX;
        menuVars.mmY = (uint32_t)
            menuVars.mmvCurrentMenuControl->mmvY;
        break;
    case 6:
        menuVars.mmTextType = md[index + 1];
        break;
    case 7:
        menuVars.mmFlags |= 1u;
        break;
    case 8:
    case 0x0f:
    case 0x19:
    case 0x1a:
        (void)mmDrawItem(&md[index], NULL);
        menuVars.mmY = (uint32_t)(
            (int32_t)menuVars.mmY +
            (int32_t)(scaleAdjustmentMM *
                      menuVars.textSpacer));
        break;
    case 9: {
        uint8_t buffer[256];
        (void)mmDrawItem(&md[index], buffer);
        mmDrawMod(&md[index], buffer);
        menuVars.mmY = (uint32_t)(
            (int32_t)menuVars.mmY +
            (int32_t)(scaleAdjustmentMM *
                      menuVars.textSpacer));
        break;
    }
    case 10:
        menuVars.mmY = (uint32_t)(
            (int32_t)menuVars.mmY + (int32_t)md[index + 1]);
        break;
    case 12:
        menuVars.mmFlags |= md[index + 1];
        break;
    case 16:
        mmDraw(exitSelectMdef);
        break;
    case 17:
        mmDraw(exitSelectMdef2);
        break;
    case 19:
        menuVars.mmX = md[index + 1];
        break;
    case 21:
        menuVars.mmY = (uint32_t)(
            (int32_t)menuVars.mmY +
            (int32_t)((float)selected *
                      scaleAdjustmentMM * -60.0f));
        break;
    case 22:
        menuVars.yflag = (uint8_t)md[index + 1];
        break;
    case 28:
        menuVars.controlFlags = (uint8_t)md[index + 1];
        if (menuVars.controlFlags == 2) {
            menuVars.selectp = menuVars.mmSelect2;
        }
        break;
    case 29:
        if (menuVars.scoreCurrentPlayer == 0) {
            menuVars.controlFlags = 1;
        } else if (menuVars.scoreCurrentPlayer == 1) {
            menuVars.controlFlags = 2;
            menuVars.selectp = menuVars.mmSelect2;
        } else if (menuVars.controlFlags == 2) {
            menuVars.selectp = menuVars.mmSelect2;
        }
        break;
    case 32:
        menuVars.mmItalics = 1;
        menuVars.mmItalicsScale =
            (float)md[index + 1] +
            (float)md[index + 2] / 10.0f;
        break;
    case 33:
        menuVars.mmBrackets = 1;
        break;
    case 34:
        menuVars.mmColorSelect = md[index + 1];
        break;
    case 35:
        menuVars.mmColorNotSelect = md[index + 1];
        break;
    case 0x42:
        menuVars.mmAnchorType = (int32_t)md[index + 1];
        break;
    case 0x44: {
        float left = -423.0f;
        float right = 423.0f;
        float top = 135.0f;
        float bottom = 285.0f;

        setPivotPositionMM(&left, &top, 4);
        setPivotPositionMM(&right, &bottom, 4);
        SCREENRECT destination;
        CVECTOR color = {255, 255, 255, 0xbe};

        /* The preceding cyclic entries are clipped to this authored panel.
         * Prompts and subsequent nested streams are outside that area. */
        jpb_TextClearClipRect();
        gLeft = left;
        gRight = right;
        gTop = top;
        gBottom = bottom;
        SetGlobalColorDefault();
        gColor.cd = 0xbe;
        SetGlobalDST();
        destination = gDST;
        /* Exact matched command 0x44 owner: menuTextures[244] is the shipped
         * mainMenuTextBox material at PDB RVA 0x539D50. */
        _DrawTexture(
            menuTextures[244], destination, NULL, color, 0.0f);
        break;
    }
    case 0x45:
    case 0x46: {
        const int wide = command == 0x46;
        float left = wide ? -625.0f : -559.0f;
        float right = wide ? 625.0f : 559.0f;
        float top = (float)md[index + 1];
        float bottom = top + 485.0f;
        SCREENRECT destination;
        CVECTOR color = {255, 255, 255, 255};

        setPivotPositionMM(&left, &top, 1);
        setPivotPositionMM(&right, &bottom, 1);
        destination.left = (int32_t)left;
        destination.top = (int32_t)top;
        destination.right = (int32_t)right;
        destination.bottom = (int32_t)bottom;
        _DrawTexture(
            menuTextures[wide ? 238 : 237],
            destination,
            NULL,
            color,
            0.0f);
        break;
    }
    case 0x47: {
        float left = -936.0f;
        float right = 936.0f;
        float top = -516.5f;
        float bottom = 516.5f;
        SCREENRECT destination;
        CVECTOR color = {255, 255, 255, 255};

        setPivotPositionMM(&left, &top, 4);
        setPivotPositionMM(&right, &bottom, 4);
        destination.left = (int32_t)left;
        destination.top = (int32_t)top;
        destination.right = (int32_t)right;
        destination.bottom = (int32_t)bottom;
        _DrawTexture(
            menuTextures[236], destination, NULL, color, 0.0f);
        break;
    }
    case 0x48: {
        unsigned texture_index = 239;
        float left = -300.0f;
        float right = 300.0f;
        float top = (float)md[index + 1];
        float bottom = top + 267.0f;
        SCREENRECT destination;
        CVECTOR color = {255, 255, 255, 0xbe};

        if (OptionStruct.ResolutionChanged != 0 &&
            OptionStruct.ResolutionChanged != 6) {
            if ((OptionStruct.ResolutionChanged == 3) ||
                (OptionStruct.ResolutionChanged == 5)) {
                texture_index = 241;
                left = -375.0f;
                right = 375.0f;
            } else {
                texture_index = 240;
                left = -325.0f;
                right = 325.0f;
            }
        }

        setPivotPositionMM(&left, &top, 1);
        setPivotPositionMM(&right, &bottom, 1);
        destination.left = (int32_t)left;
        destination.top = (int32_t)top;
        destination.right = (int32_t)right;
        destination.bottom = (int32_t)bottom;
        _DrawTexture(
            menuTextures[texture_index], destination, NULL, color, 0.0f);
        break;
    }
    case 0x49: {
        float left = -789.0f;
        float right = 789.0f;
        float top = -494.5f;
        float bottom = 493.5f;
        SCREENRECT destination;
        CVECTOR color = {255, 255, 255, 0xbe};

        setPivotPositionMM(&left, &top, 4);
        setPivotPositionMM(&right, &bottom, 4);
        destination.left = (int32_t)left;
        destination.top = (int32_t)top;
        destination.right = (int32_t)right;
        destination.bottom = (int32_t)bottom;
        _DrawTexture(
            menuTextures[242], destination, NULL, color, 0.0f);
        break;
    }
    case 0x4a:
        if (md[index + 1] == 0) {
            menuVars.textScale = 2.25f;
            menuVars.textSpacer = 60.0f;
        } else if (md[index + 1] == 1) {
            menuVars.textScale = 1.75f;
            menuVars.textSpacer = 46.0f;
        } else if (md[index + 1] == 2) {
            menuVars.textScale = 3.0f;
            menuVars.textSpacer = 80.0f;
        }
        break;
    default:
        break;
    }
    return index + mmsizes[command & 0x7fffu];
}

/* 0xD1D20, 104 bytes, global, 2 named locals
 * mmGetModVal
 * PDB type: unsigned (MDEF_MOD*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
unsigned mmGetModVal(MDEF_MOD *mod)
{
    switch (mod->type & UINT16_C(0x7f)) {
    case 0:
    case 1:
        return mod->src != NULL
            ? (unsigned)*(const uint8_t *)mod->src
            : 0;
    case 2:
    case 3:
        return (unsigned)*(const uint16_t *)mod->src;
    case 4:
    case 5:
    case 6:
        return *(const uint32_t *)mod->src;
    default:
        return 0;
    }
}

/* 0xD1D90, 127 bytes, global, 3 named locals
 * mmIncVar
 * PDB type: void (unsigned*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void mmIncVar(uint32_t *md)
{
    MDEF_MOD *mod;
    unsigned mod_index;
    unsigned value;

    mod_index = md[4];
    mod = &modVars[mod_index];
    value = mmGetModVal(mod) + mod->incspeed;
    if (value > (unsigned)mod->max) {
        if ((mod->type & UINT16_C(0x0800)) != 0) {
            return;
        }
        value = (unsigned)mod->min;
    }
    if (mmSetModVal(mod, value, mod_index)) {
        mmUpdateModSet(md, value, 0);
    }
}

/* 0xD1E10, 33 bytes, global, 2 named locals
 * mmNextCode
 * PDB type: unsigned (unsigned*, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
unsigned mmNextCode(uint32_t *md, unsigned index)
{
    uint32_t command;

    command = md[index] & 0x7fffu;
    return index + mmsizes[command];
}

/* 0xD1E40, 148 bytes, global, 3 named locals
 * mmSetModVal
 * PDB type: int (MDEF_MOD*, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int mmSetModVal(
    MDEF_MOD *mod, unsigned value, unsigned mod_index)
{
    unsigned player_mask;

    if (mod_index == 2 || mod_index == 3) {
        player_mask = mod_index == 3 ? 2u : 1u;
        if (((unsigned)menuVars.pSelect & player_mask) != 0) {
            return 0;
        }
    }
    switch (mod->type & UINT16_C(0x7f)) {
    case 0:
    case 1:
        if (mod->src != NULL) {
            *(uint8_t *)mod->src = (uint8_t)value;
        }
        break;
    case 2:
    case 3:
        *(uint16_t *)mod->src = (uint16_t)value;
        break;
    case 4:
    case 5:
    case 6:
        *(uint32_t *)mod->src = value;
        break;
    default:
        break;
    }
    return 1;
}

/* 0xD1EE0, 797 bytes, global, 4 named locals
 * mmUpdateModSet
 * PDB type: void (unsigned*, unsigned, unsig...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void mmUpdateModSet(
    uint32_t *md, unsigned value, unsigned decreased)
{
    unsigned mod_index;

    mod_index = md[4];
    switch (mod_index) {
    case 1:
        menu_setNumPlayers((unsigned)(uint8_t)GameStruct.NumPlayers);
        break;
    case 2:
    case 3: {
        unsigned player = mod_index - 2;

        updatePlayerSelectIndex((int)player);
        menuVars.subplayers[player] = menuVars.pplayers[player];
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xjedscrl", jpb_menu_platform_user_data);
        }
        break;
    }
    case 8:
        menu_checkMiniMod(value, decreased);
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xlvbrows", jpb_menu_platform_user_data);
        }
        break;
    case 14:
    case 15:
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xopt_sel", jpb_menu_platform_user_data);
        }
        if (OptionStruct.ShockFlag[mod_index - 14u] != 0) {
            ClearInput();
        }
        break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 26:
    case 27:
        if (OptionStruct.RunLimit[0] < OptionStruct.WalkLimit[0]) {
            OptionStruct.WalkLimit[0] = OptionStruct.RunLimit[0];
        }
        if (OptionStruct.RunLimit[1] < OptionStruct.WalkLimit[1]) {
            OptionStruct.WalkLimit[1] = OptionStruct.RunLimit[1];
        }
        /* FALLTHROUGH: the retail owner also emits the option cue. */
    case 20:
    case 23:
    case 24:
    case 69:
    case 72:
    case 73:
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xopt_sel", jpb_menu_platform_user_data);
        }
        break;
    case 21:
        GameStruct.xaVol =
            (uint16_t)((unsigned)OptionStruct.musicVolume * 2u);
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xopt_sel", jpb_menu_platform_user_data);
        }
        setChannelType((int)OptionStruct.Music + 1);
        if (OptionStruct.Music == 1) {
            if (GameStruct.gameMode != 6 ||
                GameStruct.CurrentLevel > 0x19) {
                stopXA();
                playXA(1, GameStruct.xaVol, 1);
            } else {
                playXA(
                    (int)aLevelXATracks[GameStruct.CurrentLevel],
                    GameStruct.xaVol,
                    1);
                if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
                    stopXA();
                }
            }
        }
        break;
    case 22:
        GameStruct.xaVol =
            (uint16_t)((unsigned)OptionStruct.musicVolume * 2u);
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xopt_sel", jpb_menu_platform_user_data);
        }
        break;
    case 55:
        if (menuVars.ultimate == 0) {
            GameStruct.GameState &= ~UINT32_C(0x04000000);
        } else {
            GameStruct.GameState |= UINT32_C(0x04000000);
        }
        break;
    case 71:
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xopt_sel", jpb_menu_platform_user_data);
        }
        generateAllText(OptionStruct.Language);
        break;
    default:
        break;
    }
}

/* 0xD2200, 3 bytes, global, 1 named locals
 * mmvInitScore
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void mmvInitScore(unsigned player_number)
{
    (void)player_number;
}

/* 0xD2210, 3 bytes, global, 1 named locals
 * mmvRunScore
 * PDB type: int (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int mmvRunScore(unsigned player_number)
{
    (void)player_number;
}

/* 0xD2220, 456 bytes, global, 14 named locals
 * newDrawControllerIcon
 * PDB type: void (int, float, int, int, int,...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xD23F0, 449 bytes, global, 15 named locals
 * newDrawControllerIconDepth
 * PDB type: void (int, float, int, int, int,...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xD25C0, 359 bytes, global, 7 named locals
 * newMenu_DrawArrows
 * PDB type: void (unsigned long, int, int, i...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void newDrawControllerIcon(
    int icon,
    float control_icon_scale,
    int x,
    int y,
    int alpha,
    int player,
    SCREENRECT scissor_rect)
{
    _Material *textures[10] = {0};
    _Material *material;
    SCREENRECT destination;
    CVECTOR color;
    float draw_scale;

    if (iconScaleOverride > 0.0f) {
        control_icon_scale = iconScaleOverride;
    }
    if (player2IconOverride != 0) {
        player = 1;
    }
    (void)getControllerTextures(player, textures);
    if (player2IconOverride == 0 && icon == 9 &&
        lastUsedInputType != 0) {
        icon = 3;
    }
    material = textures[icon];
    draw_scale = control_icon_scale * scaleAdjustmentMM;
    color.r = (uint8_t)alpha;
    color.g = (uint8_t)alpha;
    color.b = (uint8_t)alpha;
    color.cd = (uint8_t)alpha;
    destination.left = (int32_t)(
        (float)x - (float)material->iw * 0.5f * draw_scale);
    destination.top = (int32_t)(
        (float)y -
        ((float)material->ih * 0.5f - 5.0f) * draw_scale);
    destination.right = (int32_t)(
        (float)material->iw * draw_scale +
        (float)destination.left);
    destination.bottom = (int32_t)(
        (float)material->ih * draw_scale +
        (float)destination.top);
    _DrawTextureClipped(
        material, destination, NULL, color, 0.0f, scissor_rect);
}

void newDrawControllerIconDepth(
    int icon,
    float control_icon_scale,
    int x,
    int y,
    int alpha,
    int player,
    SCREENRECT scissor_rect,
    float depth)
{
    _Material *textures[10] = {0};
    _Material *material;
    SCREENRECT destination;
    CVECTOR color;
    float draw_scale;

    if (iconScaleOverride > 0.0f) {
        control_icon_scale = iconScaleOverride;
    }
    if (player2IconOverride != 0) {
        player = 1;
    }
    (void)getControllerTextures(player, textures);
    if (player2IconOverride == 0 && icon == 9 &&
        lastUsedInputType != 0) {
        icon = 3;
    }
    material = textures[icon];
    draw_scale = control_icon_scale * scaleAdjustmentMM;
    color.r = (uint8_t)alpha;
    color.g = (uint8_t)alpha;
    color.b = (uint8_t)alpha;
    color.cd = (uint8_t)alpha;
    destination.left = (int32_t)(
        (float)x - (float)material->iw * 0.5f * draw_scale);
    destination.top = (int32_t)(
        (float)y -
        ((float)material->ih * 0.5f - 5.0f) * draw_scale);
    destination.right = (int32_t)(
        (float)material->iw * draw_scale +
        (float)destination.left);
    destination.bottom = (int32_t)(
        (float)material->ih * draw_scale +
        (float)destination.top);
    _DrawTextureClipped(
        material, destination, NULL, color, depth, scissor_rect);
}
void newMenu_DrawArrows(
    uint32_t pad,
    int left_x,
    int left_y,
    int right_x,
    int right_y)
{
    SCREENRECT destination;
    CVECTOR white = {255, 255, 255, 255};
    _Material *right_material;

    destination.left = (int)(
        (float)left_x - scaleAdjustmentMM * 99.0f);
    destination.top = left_y;
    destination.right = left_x;
    destination.bottom = (int)(
        (float)left_y + scaleAdjustmentMM * 107.0f);
    if ((pad & JPB_PAD_LEFT) == 0) {
        _Material *left_material =
            menuTextures[
                (pad & JPB_PAD_RIGHT) != 0 ? 169 : 172];

        _DrawTexture(
            left_material,
            destination,
            NULL,
            white,
            0.0f);
        right_material = menuTextures[173];
    } else {
        _DrawTexture(
            menuTextures[172],
            destination,
            NULL,
            white,
            0.0f);
        right_material = menuTextures[170];
    }

    destination.left = right_x;
    destination.top = right_y;
    destination.right = (int)(
        (float)right_x + scaleAdjustmentMM * 99.0f);
    destination.bottom = (int)(
        (float)right_y + scaleAdjustmentMM * 107.0f);
    _DrawTexture(
        right_material,
        destination,
        NULL,
        white,
        0.0f);
}

/* 0xD2730, 3 bytes, global, 4 named locals
 * newMenu_DrawMessageBox
 * PDB type: void (int, int, int, int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void newMenu_DrawMessageBox(int x, int y, int width, int height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

static CVECTOR newMenu_Color(uint32_t packed)
{
    CVECTOR color;

    color.r = (uint8_t)packed;
    color.g = (uint8_t)(packed >> 8);
    color.b = (uint8_t)(packed >> 16);
    color.cd = (uint8_t)(packed >> 24);
    return color;
}

static void newMenu_DrawMaterialRect(
    int texture_index,
    float left,
    float top,
    float right,
    float bottom,
    int pivot,
    uint32_t packed_color,
    float layer)
{
    SCREENRECT destination;

    if (texture_index < 0 ||
        texture_index >=
            (int)(sizeof(menuTextures) / sizeof(menuTextures[0]))) {
        return;
    }
    setPivotPositionMM(&left, &top, pivot);
    setPivotPositionMM(&right, &bottom, pivot);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    _DrawTexture(
        menuTextures[texture_index],
        destination,
        NULL,
        newMenu_Color(packed_color),
        layer);
}

/* Recovered controller-tab geometry shared by the one- and two-player
 * character-select renderers. The retail owner scales each source icon to
 * half height/width, so the destination half-width is one quarter of iw. */
static void newMenu_DrawControllerTab(
    _Material *material,
    float center_x,
    int pivot,
    int mirrored)
{
    float half_width;
    float left;
    float right;
    float top = -440.0f;
    float bottom;
    SCREENRECT destination;
    CVECTOR color = {255, 255, 255, 255};

    half_width = (float)material->iw * 0.25f;
    left = center_x - half_width;
    right = center_x + half_width;
    bottom = (float)material->ih * 0.5f - 440.0f;
    if (mirrored) {
        float swap = left;

        left = right;
        right = swap;
    }
    setPivotPositionMM(&left, &top, pivot);
    setPivotPositionMM(&right, &bottom, pivot);
    destination.left = (int32_t)left;
    destination.top = (int32_t)top;
    destination.right = (int32_t)right;
    destination.bottom = (int32_t)bottom;
    _DrawTexture(material, destination, NULL, color, 0.0f);
}

static void newMenu_DrawControllerTabs(
    int player,
    float first_center,
    float second_center,
    int pivot,
    int mirrored)
{
    _Material *textures[10] = {0};

    if (!getControllerTextures(player, textures)) {
        return;
    }
    if (mirrored) {
        newMenu_DrawControllerTab(
            textures[0], first_center, pivot, 1);
        newMenu_DrawControllerTab(
            textures[1], second_center, pivot, 1);
    } else {
        newMenu_DrawControllerTab(
            textures[1], first_center, pivot, 0);
        newMenu_DrawControllerTab(
            textures[0], second_center, pivot, 0);
    }
}

static const char *newMenu_Text(unsigned index)
{
    return allText[index];
}

static int newMenu_AdjacentModel(
    int model, int direction, int select_type)
{
    do {
        model += direction;
        if (model < 0) {
            model = LAST_PLAYABLE_MODEL;
        } else if (model > LAST_PLAYABLE_MODEL) {
            model = 0;
        }
    } while (!jedi_CheckValidPlayerWTabs(select_type, model));
    return model;
}

/* 0xD2740, 4470 bytes, local, 26 named locals
 * newMenu_DrawP1CharacterSelect
 * PDB type: void (int, unsigned long, int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void newMenu_DrawP1CharacterSelect(
    int model, uint32_t pad, int exit_phase)
{
    int converted_model = jedi_ConvertToTextIndex(model);
    int previous_model;
    int next_model;
    int previous_converted;
    int next_converted;
    int color_sprite;
    int skill_percent = 0;
    int highest_level = 0;
    float x;
    float y;
    float name_scale =
        OptionStruct.ResolutionChanged != 0 ? 1.5f : 1.75f;

    if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
        winDrawBackground(175);
    }
    drawControlsIcon();

    x = -250.0f;
    y = -60.0f;
    setPivotPositionMM(&x, &y, 4);
    {
        float right_x = 250.0f;
        float right_y = -60.0f;

        setPivotPositionMM(&right_x, &right_y, 4);
        if (exit_phase == 0) {
            newMenu_DrawArrows(
                pad,
                (int)x,
                (int)y,
                (int)right_x,
                (int)right_y);
        }
    }

    if (jedi_CanToggleSaber((model_id)model)) {
        x = 0.0f;
        y = 100.0f;
        setPivotPositionMM(&x, &y, 7);
        (void)SDLTextWriteScaleMM(
            15, 255, 2, (int)x, (int)y,
            2.25f, 0, "%s", newMenu_Text(491));
    }

    if (GameStruct.gameCompleted != 0) {
        newMenu_DrawControllerTabs(
            0, -350.0f, 350.0f, 4, 0);
        x = -300.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 4);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP1 == 0 ? 11 : 9,
            255, 0, (int)x, (int)y,
            1.75f, 0, "%s", newMenu_Text(484));
        x = 300.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 4);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP1 == 1 ? 11 : 9,
            255, 1, (int)x, (int)y,
            1.75f, 0, "%s", newMenu_Text(485));
    }

    newMenu_DrawMaterialRect(
        183, -230.0f, -278.0f, 229.0f, 278.0f,
        4, UINT32_C(0xffffffff), 0.0f);
    newMenu_DrawMaterialRect(
        201 + converted_model,
        -196.0f, -262.0f, 195.0f, 261.0f,
        4, UINT32_C(0xffffffff), 0.1f);
    newMenu_DrawMaterialRect(
        184, -196.0f, -262.0f, 195.0f, 261.0f,
        4, UINT32_C(0xffffffff), 0.2f);
    newMenu_DrawMaterialRect(
        192, -201.0f, -128.5f, 201.0f, 128.5f,
        4, UINT32_C(0xffffffff), 0.0f);

    previous_model = newMenu_AdjacentModel(
        model, -1, newMenu_playerSelectTypeP1);
    previous_converted = jedi_ConvertToTextIndex(previous_model);
    newMenu_DrawMaterialRect(
        183, -740.0f, -200.0f, -372.8f, 244.80002f,
        4, UINT32_C(0xc8ffffff), 0.0f);
    newMenu_DrawMaterialRect(
        201 + previous_converted,
        -713.0f, -188.0f, -400.19998f, 230.4f,
        4, UINT32_C(0x6effffff), 0.1f);
    newMenu_DrawMaterialRect(
        184, -713.0f, -188.0f, -400.19998f, 230.4f,
        4, UINT32_C(0x6effffff), 0.2f);
    newMenu_DrawMaterialRect(
        192, -717.0f, -80.0f, -395.4f, 125.600006f,
        4, UINT32_C(0x82ffffff), 0.0f);

    next_model = newMenu_AdjacentModel(
        model, 1, newMenu_playerSelectTypeP1);
    next_converted = jedi_ConvertToTextIndex(next_model);
    newMenu_DrawMaterialRect(
        183, 372.8f, -200.0f, 740.0f, 244.80002f,
        4, UINT32_C(0xc8ffffff), 0.0f);
    newMenu_DrawMaterialRect(
        201 + next_converted,
        399.4f, -188.0f, 713.0f, 231.20001f,
        4, UINT32_C(0x6effffff), 0.1f);
    newMenu_DrawMaterialRect(
        184, 399.4f, -188.0f, 713.0f, 231.20001f,
        4, UINT32_C(0x6effffff), 0.2f);
    newMenu_DrawMaterialRect(
        192, 395.4f, -80.0f, 717.0f, 125.600006f,
        4, UINT32_C(0x82ffffff), 0.0f);

    newMenu_DrawMaterialRect(
        188, -645.5f, 0.0f, 645.5f, 81.0f,
        1, UINT32_C(0xffffffff), 0.0f);
    if ((OptionStruct.ResolutionChanged & UINT32_C(0xf9)) == 0 &&
        OptionStruct.ResolutionChanged != 4) {
        newMenu_DrawMaterialRect(
            185, -227.0f, 180.0f, 227.0f, 253.0f,
            1, UINT32_C(0xffffffff), 0.0f);
    } else if (OptionStruct.ResolutionChanged == 5) {
        newMenu_DrawMaterialRect(
            187, -305.0f, 180.0f, 305.0f, 253.0f,
            1, UINT32_C(0xffffffff), 0.0f);
    } else {
        newMenu_DrawMaterialRect(
            186, -285.0f, 180.0f, 285.0f, 253.0f,
            1, UINT32_C(0xffffffff), 0.0f);
    }

    x = 0.0f;
    y = 198.0f;
    setPivotPositionMM(&x, &y, 1);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)x, (int)y,
        name_scale, 0, "%s",
        newMenu_Text(332u + (unsigned)converted_model));
    x = 0.0f;
    y = 11.0f;
    setPivotPositionMM(&x, &y, 1);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)x, (int)y,
        2.25f, 0, "%s", newMenu_Text(481));

    newMenu_DrawMaterialRect(
        189, -200.5f, 250.0f, 200.5f, 141.0f,
        7, UINT32_C(0xffffffff), 0.0f);
    color_sprite = jedi_GetColorSprite((uint64_t)(unsigned)model);
    if (color_sprite < 0) {
        color_sprite = jedi_IsMelee((model_id)model) ? 176 : 174;
    }
    newMenu_DrawMaterialRect(
        color_sprite, -176.0f, 232.0f, -122.0f, 182.0f,
        7, UINT32_C(0xffffffff), 0.0f);

    jedi_CalcSkillLevels(
        model, &skill_percent, &highest_level);
    x = -60.0f;
    y = 232.5f;
    setPivotPositionMM(&x, &y, 7);
    if (!jedi_HasProgression((model_id)model)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s", newMenu_Text(479));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s %02d",
            newMenu_Text(224), highest_level);
    }
    x = -60.0f;
    y = 192.5f;
    setPivotPositionMM(&x, &y, 7);
    if (!jedi_HasProgression((model_id)model)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s", newMenu_Text(480));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s %03d",
            newMenu_Text(225), skill_percent);
    }

    if (jpb_menu_p1_character_select_draw_hook != NULL) {
        jpb_menu_p1_character_select_draw_hook(
            model,
            pad,
            exit_phase,
            jpb_menu_p1_character_select_draw_user_data);
    }
}

/* 0xD38C0, 6195 bytes, local, 26 named locals
 * newMenu_DrawP2CharacterSelect
 * PDB type: void (int, int, unsigned long, u...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void newMenu_DrawP2CharacterSelect(
    int player_one,
    int player_two,
    uint32_t player_one_pad,
    uint32_t player_two_pad,
    int is_versus)
{
    int converted_one = jedi_ConvertToTextIndex(player_one);
    int converted_two = jedi_ConvertToTextIndex(player_two);
    int color_sprite;
    int skill_percent = 0;
    int highest_level = 0;
    float x;
    float y;
    float name_scale =
        OptionStruct.ResolutionChanged != 0 ? 1.5f : 1.75f;

    if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
        winDrawBackground(175);
    }
    drawControlsIcon();

    x = 230.0f;
    y = -60.0f;
    setPivotPositionMM(&x, &y, 3);
    {
        float right_x = 710.0f;
        float right_y = -60.0f;

        setPivotPositionMM(&right_x, &right_y, 3);
        if ((newMenu_select & UINT32_C(1)) == 0) {
            newMenu_DrawArrows(
                player_one_pad,
                (int)x,
                (int)y,
                (int)right_x,
                (int)right_y);
        }
    }

    x = 710.0f;
    y = -60.0f;
    setPivotPositionMM(&x, &y, 5);
    {
        float right_x = 230.0f;
        float right_y = -60.0f;

        setPivotPositionMM(&right_x, &right_y, 5);
        if (((newMenu_select & UINT32_C(1)) != 0 ||
             GameStruct.NumPlayers != 1) &&
            (newMenu_select & UINT32_C(2)) == 0) {
            newMenu_DrawArrows(
                player_two_pad,
                (int)x,
                (int)y,
                (int)right_x,
                (int)right_y);
        }
    }

    if (jedi_CanToggleSaber((model_id)player_one)) {
        x = 460.0f;
        y = 100.0f;
        setPivotPositionMM(&x, &y, 6);
        (void)SDLTextWriteScaleMM(
            15, 255, 2, (int)x, (int)y,
            2.25f, 0, "%s", newMenu_Text(491));
    }
    if (jedi_CanToggleSaber((model_id)player_two)) {
        player2IconOverride = 1;
        x = 480.0f;
        y = 100.0f;
        setPivotPositionMM(&x, &y, 8);
        (void)SDLTextWriteScaleMM(
            15, 255, 2, (int)x, (int)y,
            2.25f, 0, "%s", newMenu_Text(491));
        player2IconOverride = 0;
    }

    newMenu_DrawMaterialRect(
        177, -4.0f, -289.5f, 4.0f, 289.5f,
        4, UINT32_C(0xffffffff), 0.0f);

    newMenu_DrawMaterialRect(
        190, 19.5f, 0.0f, 920.5f, 81.0f,
        0, UINT32_C(0xffffffff), 1.0f);
    x = 470.0f;
    y = 11.0f;
    setPivotPositionMM(&x, &y, 0);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)x, (int)y,
        2.25f, 0, "%s", newMenu_Text(481));

    if (GameStruct.gameCompleted != 0 || is_versus != 0) {
        newMenu_DrawControllerTabs(
            0, 120.0f, 820.0f, 3, 0);
        x = 170.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 3);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP1 == 0 ? 11 : 9,
            255, 0, (int)x, (int)y,
            1.75f, 0, "%s", newMenu_Text(484));
        x = 770.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 3);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP1 == 1 ? 11 : 9,
            255, 1, (int)x, (int)y,
            1.75f, 0, "%s", newMenu_Text(485));
    }

    newMenu_DrawMaterialRect(
        183, 240.5f, -278.0f, 699.5f, 278.0f,
        3, UINT32_C(0xffffffff), 0.0f);
    newMenu_DrawMaterialRect(
        201 + converted_one,
        274.0f, -262.0f, 665.0f, 261.0f,
        3, UINT32_C(0xffffffff), 0.1f);
    newMenu_DrawMaterialRect(
        184, 274.0f, -262.0f, 665.0f, 261.0f,
        3, UINT32_C(0xffffffff), 0.2f);
    newMenu_DrawMaterialRect(
        192, 269.0f, -128.5f, 671.0f, 128.5f,
        3, UINT32_C(0xffffffff), 0.0f);
    if ((OptionStruct.ResolutionChanged & UINT32_C(0xf9)) == 0 &&
        OptionStruct.ResolutionChanged != 4) {
        newMenu_DrawMaterialRect(
            185, 243.0f, 180.0f, 697.0f, 253.0f,
            0, UINT32_C(0xffffffff), 0.0f);
    } else if (OptionStruct.ResolutionChanged == 5) {
        newMenu_DrawMaterialRect(
            187, 165.0f, 180.0f, 775.0f, 253.0f,
            0, UINT32_C(0xffffffff), 0.0f);
    } else {
        newMenu_DrawMaterialRect(
            186, 185.0f, 180.0f, 755.0f, 253.0f,
            0, UINT32_C(0xffffffff), 0.0f);
    }
    x = 470.0f;
    y = 198.0f;
    setPivotPositionMM(&x, &y, 0);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)x, (int)y,
        name_scale, 0, "%s",
        newMenu_Text(332u + (unsigned)converted_one));
    newMenu_DrawMaterialRect(
        189, 269.5f, 250.0f, 670.5f, 141.0f,
        6, UINT32_C(0xffffffff), 0.0f);
    color_sprite = jedi_GetColorSprite((uint64_t)(unsigned)player_one);
    if (color_sprite < 0) {
        color_sprite =
            jedi_IsMelee((model_id)player_one) ? 176 : 174;
    }
    newMenu_DrawMaterialRect(
        color_sprite, 294.0f, 232.0f, 348.0f, 182.0f,
        6, UINT32_C(0xffffffff), 0.0f);
    jedi_CalcSkillLevels(
        player_one, &skill_percent, &highest_level);
    x = 410.0f;
    y = 232.5f;
    setPivotPositionMM(&x, &y, 6);
    if (!jedi_HasProgression((model_id)player_one)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s", newMenu_Text(479));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s %02d",
            newMenu_Text(224), highest_level);
    }
    x = 410.0f;
    y = 192.5f;
    setPivotPositionMM(&x, &y, 6);
    if (!jedi_HasProgression((model_id)player_one)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s", newMenu_Text(480));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s %03d",
            newMenu_Text(225), skill_percent);
    }

    newMenu_DrawMaterialRect(
        190, 920.5f, 0.0f, 19.5f, 81.0f,
        2, UINT32_C(0xffffffff), 1.0f);
    x = 470.0f;
    y = 11.0f;
    setPivotPositionMM(&x, &y, 2);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)x, (int)y,
        2.25f, 0, "%s", newMenu_Text(482));

    if (GameStruct.gameCompleted != 0 || is_versus != 0) {
        newMenu_DrawControllerTabs(
            (int)(uint8_t)GameStruct.NumPlayers - 1,
            120.0f,
            820.0f,
            5,
            1);
        x = 770.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 5);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP2 == 0 ? 11 : 9,
            255, 0, (int)x, (int)y,
            1.75f, 0, "%s", newMenu_Text(484));
        x = 170.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 5);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP2 == 1 ? 11 : 9,
            255, 1, (int)x, (int)y,
            1.75f, 0, "%s", newMenu_Text(485));
    }

    newMenu_DrawMaterialRect(
        183, 699.5f, -278.0f, 240.5f, 278.0f,
        5, UINT32_C(0xffffffff), 0.0f);
    newMenu_DrawMaterialRect(
        201 + converted_two,
        665.0f, -262.0f, 274.0f, 261.0f,
        5, UINT32_C(0xffffffff), 0.1f);
    newMenu_DrawMaterialRect(
        184, 665.0f, -262.0f, 274.0f, 261.0f,
        5, UINT32_C(0xffffffff), 0.2f);
    newMenu_DrawMaterialRect(
        192, 671.0f, -128.5f, 269.0f, 128.5f,
        5, UINT32_C(0xffffffff), 0.0f);
    if ((OptionStruct.ResolutionChanged & UINT32_C(0xf9)) == 0 &&
        OptionStruct.ResolutionChanged != 4) {
        newMenu_DrawMaterialRect(
            185, 697.0f, 180.0f, 243.0f, 253.0f,
            2, UINT32_C(0xffffffff), 0.0f);
    } else if (OptionStruct.ResolutionChanged == 5) {
        newMenu_DrawMaterialRect(
            187, 775.0f, 180.0f, 165.0f, 253.0f,
            2, UINT32_C(0xffffffff), 0.0f);
    } else {
        newMenu_DrawMaterialRect(
            186, 755.0f, 180.0f, 185.0f, 253.0f,
            2, UINT32_C(0xffffffff), 0.0f);
    }
    x = 470.0f;
    y = 198.0f;
    setPivotPositionMM(&x, &y, 2);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)x, (int)y,
        name_scale, 0, "%s",
        newMenu_Text(332u + (unsigned)converted_two));
    newMenu_DrawMaterialRect(
        189, 670.5f, 250.0f, 269.5f, 141.0f,
        8, UINT32_C(0xffffffff), 0.0f);
    color_sprite = jedi_GetColorSprite((uint64_t)(unsigned)player_two);
    if (color_sprite < 0) {
        color_sprite =
            jedi_IsMelee((model_id)player_two) ? 176 : 174;
    }
    newMenu_DrawMaterialRect(
        color_sprite, 646.0f, 232.0f, 592.0f, 182.0f,
        8, UINT32_C(0xffffffff), 0.0f);
    skill_percent = 0;
    highest_level = 0;
    jedi_CalcSkillLevels(
        player_two, &skill_percent, &highest_level);
    x = 530.0f;
    y = 232.5f;
    setPivotPositionMM(&x, &y, 8);
    if (!jedi_HasProgression((model_id)player_two)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s", newMenu_Text(479));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s %02d",
            newMenu_Text(224), highest_level);
    }
    x = 530.0f;
    y = 192.5f;
    setPivotPositionMM(&x, &y, 8);
    if (!jedi_HasProgression((model_id)player_two)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s", newMenu_Text(480));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, "%s %03d",
            newMenu_Text(225), skill_percent);
    }

    if (jpb_menu_p2_character_select_draw_hook != NULL) {
        jpb_menu_p2_character_select_draw_hook(
            player_one,
            player_two,
            player_one_pad,
            player_two_pad,
            is_versus,
            jpb_menu_p2_character_select_draw_user_data);
    }
}

/* 0xD5100, 3285 bytes, local, 18 named locals
 * newMenu_DrawTraining
 * PDB type: void (int, unsigned long)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void newMenu_DrawTraining(int model, uint32_t pad)
{
    int converted_model = jedi_ConvertToTextIndex(model);
    int color_sprite;
    int skill_percent = 0;
    int highest_level = 0;
    float left_x;
    float left_y;
    float right_x;
    float right_y;

    if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
        winDrawBackground(175);
    }
    drawControlsIcon();

    left_x = 230.0f;
    left_y = -60.0f;
    right_x = 710.0f;
    right_y = -60.0f;
    setPivotPositionMM(&left_x, &left_y, 3);
    setPivotPositionMM(&right_x, &right_y, 3);
    if ((newMenu_select & UINT32_C(1)) == 0) {
        newMenu_DrawArrows(
            pad,
            (int)left_x,
            (int)left_y,
            (int)right_x,
            (int)right_y);
    }

    left_x = 710.0f;
    left_y = -60.0f;
    right_x = 230.0f;
    right_y = -60.0f;
    setPivotPositionMM(&left_x, &left_y, 5);
    setPivotPositionMM(&right_x, &right_y, 5);
    if ((newMenu_select & UINT32_C(3)) == UINT32_C(1)) {
        newMenu_DrawArrows(
            menuVars.pad[1],
            (int)left_x,
            (int)left_y,
            (int)right_x,
            (int)right_y);
    }

    if (jedi_CanToggleSaber((model_id)model)) {
        left_x = 0.0f;
        left_y = 100.0f;
        setPivotPositionMM(&left_x, &left_y, 7);
        (void)SDLTextWriteScaleMM(
            15, 255, 2, (int)left_x, (int)left_y,
            2.25f, 0, "%s", newMenu_Text(491));
    }

    newMenu_DrawMaterialRect(
        177, -4.0f, -289.5f, 4.0f, 289.5f,
        4, UINT32_C(0xffffffff), 0.0f);
    newMenu_DrawMaterialRect(
        190, 19.5f, 0.0f, 920.5f, 81.0f,
        0, UINT32_C(0xffffffff), 0.0f);
    left_x = 470.0f;
    left_y = 11.0f;
    setPivotPositionMM(&left_x, &left_y, 0);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)left_x, (int)left_y,
        2.25f, 0, "%s", newMenu_Text(481));

    newMenu_DrawMaterialRect(
        183, 240.5f, -278.0f, 699.5f, 278.0f,
        3, UINT32_C(0xffffffff), 0.0f);
    newMenu_DrawMaterialRect(
        201 + converted_model,
        274.0f, -262.0f, 665.0f, 261.0f,
        3, UINT32_C(0xffffffff), 0.1f);
    newMenu_DrawMaterialRect(
        184, 274.0f, -262.0f, 665.0f, 261.0f,
        3, UINT32_C(0xffffffff), 0.2f);
    newMenu_DrawMaterialRect(
        192, 269.0f, -128.5f, 671.0f, 128.5f,
        3, UINT32_C(0xffffffff), 0.0f);
    newMenu_DrawMaterialRect(
        185, 243.0f, 180.0f, 697.0f, 253.0f,
        0, UINT32_C(0xffffffff), 0.0f);

    left_x = 470.0f;
    left_y = 198.0f;
    setPivotPositionMM(&left_x, &left_y, 0);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)left_x, (int)left_y,
        1.75f, 0, "%s",
        newMenu_Text(332u + (unsigned)converted_model));

    newMenu_DrawMaterialRect(
        189, 269.5f, 250.0f, 670.5f, 141.0f,
        6, UINT32_C(0xffffffff), 0.0f);
    color_sprite = jedi_GetColorSprite((uint64_t)(unsigned)model);
    if (color_sprite < 0) {
        color_sprite = jedi_IsMelee((model_id)model) ? 176 : 174;
    }
    newMenu_DrawMaterialRect(
        color_sprite, 294.0f, 232.0f, 348.0f, 182.0f,
        6, UINT32_C(0xffffffff), 0.0f);
    jedi_CalcSkillLevels(model, &skill_percent, &highest_level);
    left_x = 410.0f;
    left_y = 232.5f;
    setPivotPositionMM(&left_x, &left_y, 6);
    if (!jedi_HasProgression((model_id)model)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)left_x, (int)left_y,
            1.5f, 0, "%s", newMenu_Text(479));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)left_x, (int)left_y,
            1.5f, 0, "%s %02d",
            newMenu_Text(224), highest_level);
    }
    left_x = 410.0f;
    left_y = 192.5f;
    setPivotPositionMM(&left_x, &left_y, 6);
    if (!jedi_HasProgression((model_id)model)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)left_x, (int)left_y,
            1.5f, 0, "%s", newMenu_Text(480));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)left_x, (int)left_y,
            1.5f, 0, "%s %03d",
            newMenu_Text(225), skill_percent);
    }

    newMenu_DrawMaterialRect(
        190, 920.5f, 0.0f, 19.5f, 81.0f,
        2, UINT32_C(0xffffffff), 0.0f);
    left_x = 470.0f;
    left_y = 11.0f;
    setPivotPositionMM(&left_x, &left_y, 2);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)left_x, (int)left_y,
        2.25f, 0, "%s", newMenu_Text(483));
    newMenu_DrawMaterialRect(
        183, 699.5f, -278.0f, 240.5f, 278.0f,
        5, UINT32_C(0xffffffff), 0.0f);
    newMenu_DrawMaterialRect(
        226 + newMenu_trainLevel,
        665.0f, -262.0f, 274.0f, 261.0f,
        5, UINT32_C(0xffffffff), 0.1f);
    newMenu_DrawMaterialRect(
        184, 665.0f, -262.0f, 274.0f, 261.0f,
        5, UINT32_C(0xffffffff), 0.2f);
    newMenu_DrawMaterialRect(
        192, 671.0f, -128.5f, 269.0f, 128.5f,
        5, UINT32_C(0xffffffff), 0.0f);
    newMenu_DrawMaterialRect(
        185, 697.0f, 180.0f, 243.0f, 253.0f,
        2, UINT32_C(0xffffffff), 0.0f);
    left_x = 470.0f;
    left_y = 198.0f;
    setPivotPositionMM(&left_x, &left_y, 2);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)left_x, (int)left_y,
        1.75f, 0, "%s",
        newMenu_Text(320u + (unsigned)newMenu_trainLevel));
}

/* 0xD5DE0, 2311 bytes, local, 28 named locals
 * newMenu_DrawVSMode
 * PDB type: void (int, int, unsigned long, u...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void newMenu_DrawVSImage(int texture, int player)
{
    _Material *material = menuTextures[fontSpec[texture].clut];
    float anchor_x = player == 0 ? 7.0f : 4.0f;
    float anchor_y = player == 0 ? 69.0f : 64.0f;
    float extent_x = (float)material->iw * 0.535f;
    float extent_y = (float)material->ih * 0.535f;
    SCREENRECT destination;
    CVECTOR color = {255, 255, 255, 255};

    setPivotPositionMM(&anchor_x, &anchor_y, player == 0 ? 0 : 8);
    if (player == 0) {
        setPositionOffPivotMM(
            &extent_x, &extent_y, anchor_x, anchor_y);
        destination.left = (int32_t)anchor_x;
        destination.top = (int32_t)anchor_y;
        destination.right = (int32_t)extent_x;
        destination.bottom = (int32_t)extent_y;
    } else {
        extent_x = -extent_x;
        extent_y = -extent_y;
        setPositionOffPivotMM(
            &extent_x, &extent_y, anchor_x, anchor_y);
        destination.left = (int32_t)extent_x;
        destination.top = (int32_t)extent_y;
        destination.right = (int32_t)anchor_x;
        destination.bottom = (int32_t)anchor_y;
    }
    _DrawTexture(material, destination, NULL, color, 0.0f);
}

static void newMenu_DrawVSPlayer(int model, int player)
{
    char *name;
    int texture;
    float text_x = 605.0f;
    float text_y = player == 0 ? 100.0f : 155.0f;

    if (newMenu_GetVSExtraPlayer(&name, &texture, model) == 0) {
        texture = 382 + model;
        name = allText[332u + (unsigned)model];
    }
    newMenu_DrawVSImage(texture, player);
    setPivotPositionMM(&text_x, &text_y, player == 0 ? 0 : 8);
    (void)SDLTextWriteScaleMM(
        11, 255, player, (int)text_x, (int)text_y,
        3.0f, 0, "%s", name);
}

static void newMenu_DrawVSMode(
    int player_one,
    int player_two,
    uint32_t pad_one,
    uint32_t pad_two)
{
    float left_x = -360.0f;
    float left_y = -200.0f;
    float right_x = -285.0f;
    float right_y = -200.0f;
    float player_two_left_x = 235.0f;
    float player_two_left_y = 140.0f;
    float player_two_right_x = 310.0f;
    float player_two_right_y = 140.0f;
    uint32_t second_arrow_pad;
    float text_x;
    float text_y;

    menu_showVRAMBackground(6);
    drawControlsIconTraining();

    setPivotPositionMM(&left_x, &left_y, 4);
    setPivotPositionMM(&right_x, &right_y, 4);
    setPivotPositionMM(&player_two_left_x, &player_two_left_y, 4);
    setPivotPositionMM(&player_two_right_x, &player_two_right_y, 4);
    if ((newMenu_select & UINT32_C(1)) == 0) {
        menu_DrawArrows(
            pad_one, (int)left_x, (int)left_y,
            (int)right_x, (int)right_y);
        second_arrow_pad = pad_one & ~UINT32_C(0xa000);
    } else {
        menu_DrawArrows(
            pad_one & ~UINT32_C(0xa000),
            (int)left_x, (int)left_y,
            (int)right_x, (int)right_y);
        newMenu_DrawVSImage(367, 0);
        second_arrow_pad = pad_one;
    }
    menu_DrawArrows(
        second_arrow_pad,
        (int)player_two_left_x, (int)player_two_left_y,
        (int)player_two_right_x, (int)player_two_right_y);

    if ((newMenu_select & UINT32_C(2)) != 0) {
        newMenu_DrawVSImage(367, 1);
    }
    (void)pad_two;

    newMenu_DrawVSPlayer(player_one, 0);
    newMenu_DrawVSPlayer(player_two, 1);

    text_x = 0.0f;
    text_y = 248.0f;
    setPivotPositionMM(&text_x, &text_y, 1);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)text_x, (int)text_y,
        2.0f, 0, "%s", allText[237]);
    text_x = 0.0f;
    text_y = 288.0f;
    setPivotPositionMM(&text_x, &text_y, 7);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)text_x, (int)text_y,
        2.0f, 0, "%s", allText[238]);
}

/* 0xD66F0, 438 bytes, global, 5 named locals
 * newMenu_GetVSExtraPlayer
 * PDB type: int (char**, int*, int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int newMenu_GetVSExtraPlayer(char **name, int *texture, int model)
{
    int text;

    switch (model) {
    case 8:
        *texture = 0x18e;
        text = 0x154;
        break;
    case 15:
        *texture = 0x18c;
        text = 0x155;
        break;
    case 17:
        *texture = 0x187;
        text = 0x156;
        break;
    case 18:
        *texture = 0x191;
        text = 0x157;
        break;
    case 21:
        *texture = 0x188;
        text = 0x158;
        break;
    case 22:
        *texture = 0x191;
        text = 0x159;
        break;
    case 26:
        *texture = 0x189;
        text = 0x15a;
        break;
    case 30:
        *texture = 0x196;
        text = 0x15b;
        break;
    case 36:
        *texture = 0x190;
        text = 0x15c;
        break;
    case 37:
        *texture = 0x18f;
        text = 0x15d;
        break;
    case 41:
        *texture = 0x191;
        text = 0x15e;
        break;
    case 48:
        *texture = 0x192;
        text = 0x15f;
        break;
    case 49:
        *texture = 0x193;
        text = 0x160;
        break;
    case 50:
        *texture = 0x194;
        text = 0x161;
        break;
    case 51:
        *texture = 0x195;
        text = 0x162;
        break;
    case 52:
        *texture = 0x195;
        text = 0x163;
        break;
    case 53:
        *texture = 0x18a;
        text = 0x164;
        break;
    default:
        return 0;
    }
    *name = allText[text];
    return 1;
}

/* 0xD68B0, 953 bytes, global, 0 named locals
 * newMenu_P1CharacterSelect
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

static uint32_t newMenu_CharacterSelectPad(
    unsigned player, uint32_t edge_pad)
{
    (void)player;
    return edge_pad;
}

int newMenu_P1CharacterSelect(void)
{
    int model;
    uint32_t pad_one = menuVars.pad[0];

    if (p1Disconnected != 0) {
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        menu_drawReconnect();
        return 0;
    }
    if (newMenu_state == 0) {
        newMenu_errorState = 0x10;
        newMenu_bAbortMenu = 0;
        newMenu_select = 0;
        newMenu_state = 1;
    } else if (newMenu_state == 1) {
        GameStruct.ModelSelect[0] =
            (int16_t)newMenu_currentModelSelectNGPP1;
        if (newMenu_playerSelectTypeP1 == 0) {
            GameStruct.ModelSelect[0] =
                (int16_t)newMenu_currentModelSelectBaseP1;
        }
        menu_setNumPlayers((unsigned)(uint8_t)GameStruct.NumPlayers);
        newMenu_state = 0x18;
    } else if (newMenu_state == 0x0e) {
        model = GameStruct.ModelSelect[0];
        if (newMenu_playerSelectTypeP1 == 0) {
            model = newMenu_currentModelSelectNGPP1;
            newMenu_currentModelSelectBaseP1 =
                GameStruct.ModelSelect[0];
        }
        newMenu_currentModelSelectNGPP1 = model;
        menu_setNumPlayers((unsigned)(uint8_t)GameStruct.NumPlayers);
        newMenu_DrawP1CharacterSelect(
            GameStruct.ModelSelect[0], menuVars.pad[0], 1);
        newMenu_state = 0;
        if (newMenu_bAbortMenu != 0) {
            GameStruct.NumPlayers = 1;
            return -1;
        }
        return 1;
    } else if (newMenu_state == 0x18 &&
               (newMenu_select & UINT32_C(1)) == 0) {
        pad_one = newMenu_CharacterSelectPad(0, menuVars.pad[0]);
        if (GameStruct.gameCompleted != 0) {
            if ((pad_one & JPB_PAD_BLOCK) != 0 &&
                newMenu_playerSelectTypeP1 == 1) {
                newMenu_currentModelSelectNGPP1 =
                    GameStruct.ModelSelect[0];
                newMenu_playerSelectTypeP1 = 0;
                GameStruct.ModelSelect[0] =
                    (int16_t)newMenu_currentModelSelectBaseP1;
            } else if ((pad_one & JPB_PAD_LOCK_ON) != 0 &&
                       newMenu_playerSelectTypeP1 == 0) {
                newMenu_currentModelSelectBaseP1 =
                    GameStruct.ModelSelect[0];
                newMenu_playerSelectTypeP1 = 1;
                GameStruct.ModelSelect[0] =
                    (int16_t)newMenu_currentModelSelectNGPP1;
            }
        }
        if ((pad_one & JPB_PAD_LEFT) != 0) {
            do {
                ++GameStruct.ModelSelect[0];
                if (GameStruct.ModelSelect[0] >
                    jar_jar_playable_model) {
                    GameStruct.ModelSelect[0] = 0;
                }
            } while (!jedi_CheckValidPlayerWTabs(
                newMenu_playerSelectTypeP1,
                GameStruct.ModelSelect[0]));
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedscrl", jpb_menu_platform_user_data);
            }
        } else if ((pad_one & JPB_PAD_RIGHT) != 0) {
            do {
                --GameStruct.ModelSelect[0];
                if (GameStruct.ModelSelect[0] < 0) {
                    GameStruct.ModelSelect[0] =
                        jar_jar_playable_model;
                }
            } while (!jedi_CheckValidPlayerWTabs(
                newMenu_playerSelectTypeP1,
                GameStruct.ModelSelect[0]));
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedscrl", jpb_menu_platform_user_data);
            }
        } else if ((pad_one & JPB_PAD_COMBO_SOUTH) != 0 &&
                   (newMenu_select == 0 ||
                    GameStruct.ModelSelect[1] !=
                        GameStruct.ModelSelect[0])) {
            newMenu_state = 0x0e;
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedsel", jpb_menu_platform_user_data);
            }
        }
        if ((pad_one & JPB_PAD_JUMP) != 0) {
            newMenu_bAbortMenu = 1;
            newMenu_state = 0x0e;
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedscrl", jpb_menu_platform_user_data);
            }
        }
    }
    if ((pad_one & JPB_PAD_ITEM) != 0 &&
        jedi_CanToggleSaber(
            (model_id)GameStruct.ModelSelect[0])) {
        jedi_ToggleSaberColor(
            (model_id)GameStruct.ModelSelect[0]);
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xopt_sel", jpb_menu_platform_user_data);
        }
    }
    newMenu_DrawP1CharacterSelect(
        GameStruct.ModelSelect[0], pad_one, 0);
    return 0;
}

/* 0xD6C70, 1729 bytes, global, 4 named locals
 * newMenu_P2CharacterSelect
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int newMenu_P2CharacterSelect(int isVS)
{
    int player_one = GameStruct.ModelSelect[0];
    int player_two = GameStruct.ModelSelect[1];
    uint32_t pad_two = 0;

    if (p1Disconnected != 0 || p2Disconnected != 0) {
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        menu_drawReconnect();
        return 0;
    }
    if (newMenu_state == 0) {
        newMenu_bAbortMenu = 0;
        newMenu_errorState = 0x10;
        newMenu_state = 1;
        newMenu_select = 0;
    } else if (newMenu_state == 1) {
        GameStruct.ModelSelect[0] =
            (int16_t)newMenu_currentModelSelectNGPP1;
        if (newMenu_playerSelectTypeP1 == 0) {
            GameStruct.ModelSelect[0] =
                (int16_t)newMenu_currentModelSelectBaseP1;
        }
        GameStruct.ModelSelect[1] =
            (int16_t)newMenu_currentModelSelectNGPP2;
        if (newMenu_playerSelectTypeP2 == 0) {
            GameStruct.ModelSelect[1] =
                (int16_t)newMenu_currentModelSelectBaseP2;
        }
        menu_setNumPlayers((unsigned)(uint8_t)GameStruct.NumPlayers);
        newMenu_state = 0x18;
    } else if (newMenu_state == 0x0e) {
        int current;

        if (isVS == 0) {
            current = GameStruct.ModelSelect[0];
            if (newMenu_playerSelectTypeP1 == 0) {
                current = newMenu_currentModelSelectNGPP1;
                newMenu_currentModelSelectBaseP1 =
                    GameStruct.ModelSelect[0];
            }
            newMenu_currentModelSelectNGPP1 = current;

            current = GameStruct.ModelSelect[1];
            if (newMenu_playerSelectTypeP2 == 0) {
                newMenu_currentModelSelectBaseP2 =
                    GameStruct.ModelSelect[1];
                current = newMenu_currentModelSelectNGPP2;
            }
        } else {
            newMenu_playerSelectTypeP1 = 0;
            newMenu_currentModelSelectBaseP1 = obi_wan_model;
            newMenu_currentModelSelectNGPP1 = pilot_model;
            newMenu_playerSelectTypeP2 = 0;
            newMenu_currentModelSelectBaseP2 = qui_gon_model;
            newMenu_currentModelSelectNGPP2 = rifle_model;
            current = newMenu_currentModelSelectNGPP2;
        }
        newMenu_currentModelSelectNGPP2 = current;
        menu_setNumPlayers((unsigned)(uint8_t)GameStruct.NumPlayers);
        newMenu_DrawP2CharacterSelect(
            player_one,
            player_two,
            menuVars.pad[0],
            pad_two,
            isVS);
        newMenu_state = 0;
        if (newMenu_bAbortMenu != 0) {
            GameStruct.NumPlayers = 1;
            return -1;
        }
        return 1;
    } else if (newMenu_state == 0x18) {
        if ((newMenu_select & UINT32_C(1)) == 0) {
            uint32_t pad_one =
                newMenu_CharacterSelectPad(0, menuVars.pad[0]);

            if (GameStruct.gameCompleted != 0 || isVS != 0) {
                if ((pad_one & JPB_PAD_BLOCK) != 0 &&
                    newMenu_playerSelectTypeP1 == 1) {
                    newMenu_currentModelSelectNGPP1 =
                        GameStruct.ModelSelect[0];
                    newMenu_playerSelectTypeP1 = 0;
                    GameStruct.ModelSelect[0] =
                        (int16_t)newMenu_currentModelSelectBaseP1;
                } else if ((pad_one & JPB_PAD_LOCK_ON) != 0 &&
                           newMenu_playerSelectTypeP1 == 0) {
                    newMenu_currentModelSelectBaseP1 =
                        GameStruct.ModelSelect[0];
                    newMenu_playerSelectTypeP1 = 1;
                    GameStruct.ModelSelect[0] =
                        (int16_t)newMenu_currentModelSelectNGPP1;
                }
            }
            if ((pad_one & JPB_PAD_LEFT) != 0) {
                do {
                    ++GameStruct.ModelSelect[0];
                    if (GameStruct.ModelSelect[0] >
                        jar_jar_playable_model) {
                        GameStruct.ModelSelect[0] = 0;
                    }
                } while (!jedi_CheckValidPlayerWTabs(
                    newMenu_playerSelectTypeP1,
                    GameStruct.ModelSelect[0]));
                if (jpb_menu_platform_hooks.soundCue != NULL) {
                    jpb_menu_platform_hooks.soundCue(
                        "xjedscrl", jpb_menu_platform_user_data);
                }
            } else if ((pad_one & JPB_PAD_RIGHT) != 0) {
                do {
                    --GameStruct.ModelSelect[0];
                    if (GameStruct.ModelSelect[0] < 0) {
                        GameStruct.ModelSelect[0] =
                            jar_jar_playable_model;
                    }
                } while (!jedi_CheckValidPlayerWTabs(
                    newMenu_playerSelectTypeP1,
                    GameStruct.ModelSelect[0]));
                if (jpb_menu_platform_hooks.soundCue != NULL) {
                    jpb_menu_platform_hooks.soundCue(
                        "xjedscrl", jpb_menu_platform_user_data);
                }
            } else if ((pad_one & JPB_PAD_COMBO_SOUTH) != 0 &&
                       (newMenu_select == 0 ||
                        GameStruct.ModelSelect[1] !=
                            GameStruct.ModelSelect[0])) {
                newMenu_select |= UINT32_C(1);
                if (jpb_menu_platform_hooks.soundCue != NULL) {
                    jpb_menu_platform_hooks.soundCue(
                        "xjedsel", jpb_menu_platform_user_data);
                }
            }
            if ((pad_one & JPB_PAD_ITEM) != 0 &&
                jedi_CanToggleSaber(
                    (model_id)GameStruct.ModelSelect[0])) {
                jedi_ToggleSaberColor(
                    (model_id)GameStruct.ModelSelect[0]);
                if (jpb_menu_platform_hooks.soundCue != NULL) {
                    jpb_menu_platform_hooks.soundCue(
                        "xopt_sel", jpb_menu_platform_user_data);
                }
            }
        }

        if (((newMenu_select & UINT32_C(2)) == 0 &&
             GameStruct.NumPlayers == 2) ||
            (newMenu_select & UINT32_C(1)) != 0) {
            pad_two = menuVars.pad[1];
            if (GameStruct.NumPlayers == 1) {
                pad_two = menuVars.pad[0];
            }
            pad_two = newMenu_CharacterSelectPad(
                GameStruct.NumPlayers == 1 ? 0u : 1u,
                pad_two);
            if (GameStruct.gameCompleted != 0 || isVS != 0) {
                if ((pad_two & JPB_PAD_BLOCK) != 0 &&
                    newMenu_playerSelectTypeP2 == 1) {
                    newMenu_currentModelSelectNGPP2 =
                        GameStruct.ModelSelect[1];
                    newMenu_playerSelectTypeP2 = 0;
                    GameStruct.ModelSelect[1] =
                        (int16_t)newMenu_currentModelSelectBaseP2;
                } else if ((pad_two & JPB_PAD_LOCK_ON) != 0 &&
                           newMenu_playerSelectTypeP2 == 0) {
                    newMenu_currentModelSelectBaseP2 =
                        GameStruct.ModelSelect[1];
                    newMenu_playerSelectTypeP2 = 1;
                    GameStruct.ModelSelect[1] =
                        (int16_t)newMenu_currentModelSelectNGPP2;
                }
            }
            if ((pad_two & JPB_PAD_LEFT) != 0) {
                do {
                    ++GameStruct.ModelSelect[1];
                    if (GameStruct.ModelSelect[1] >
                        jar_jar_playable_model) {
                        GameStruct.ModelSelect[1] = 0;
                    }
                } while (!jedi_CheckValidPlayerWTabs(
                    newMenu_playerSelectTypeP2,
                    GameStruct.ModelSelect[1]));
                if (jpb_menu_platform_hooks.soundCue != NULL) {
                    jpb_menu_platform_hooks.soundCue(
                        "xjedscrl", jpb_menu_platform_user_data);
                }
            } else if ((pad_two & JPB_PAD_RIGHT) != 0) {
                do {
                    --GameStruct.ModelSelect[1];
                    if (GameStruct.ModelSelect[1] < 0) {
                        GameStruct.ModelSelect[1] =
                            jar_jar_playable_model;
                    }
                } while (!jedi_CheckValidPlayerWTabs(
                    newMenu_playerSelectTypeP2,
                    GameStruct.ModelSelect[1]));
                if (jpb_menu_platform_hooks.soundCue != NULL) {
                    jpb_menu_platform_hooks.soundCue(
                        "xjedscrl", jpb_menu_platform_user_data);
                }
            } else if ((pad_two & JPB_PAD_COMBO_SOUTH) != 0 &&
                       (newMenu_select == 0 ||
                        GameStruct.ModelSelect[1] !=
                            GameStruct.ModelSelect[0])) {
                newMenu_select |= UINT32_C(2);
                if (jpb_menu_platform_hooks.soundCue != NULL) {
                    jpb_menu_platform_hooks.soundCue(
                        "xjedsel", jpb_menu_platform_user_data);
                }
            }
            if ((GameStruct.NumPlayers != 1 ||
                 (newMenu_select & UINT32_C(1)) != 0) &&
                (pad_two & JPB_PAD_ITEM) != 0 &&
                jedi_CanToggleSaber(
                    (model_id)GameStruct.ModelSelect[1])) {
                jedi_ToggleSaberColor(
                    (model_id)GameStruct.ModelSelect[1]);
                if (jpb_menu_platform_hooks.soundCue != NULL) {
                    jpb_menu_platform_hooks.soundCue(
                        "xopt_sel", jpb_menu_platform_user_data);
                }
            }
        }
        if (((pad_two | menuVars.pad[0]) & JPB_PAD_JUMP) != 0) {
            newMenu_bAbortMenu = 1;
            newMenu_state = 0x0e;
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedscrl", jpb_menu_platform_user_data);
            }
        }
        if (newMenu_select == UINT32_C(3)) {
            newMenu_state = 0x0e;
        }
    }
    newMenu_DrawP2CharacterSelect(
        player_one,
        player_two,
        menuVars.pad[0],
        pad_two,
        isVS);
    return 0;
}

/* 0xD7340, 1040 bytes, global, 4 named locals
 * newMenu_PlayerSelect
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int newMenu_PlayerSelect(void)
{
    int player_one = GameStruct.ModelSelect[0];
    int player_two = GameStruct.ModelSelect[1];
    uint32_t pad_two = 0;

    SetDispMask(1);
    if (newMenu_state == 0) {
        newMenu_bAbortMenu = 0;
        newMenu_errorState = 0x10;
        newMenu_state = 1;
        newMenu_select = 0;
    } else if (newMenu_state == 1) {
        menu_setNumPlayers((unsigned)(int8_t)GameStruct.NumPlayers);
        newMenu_state = 0x18;
    } else if (newMenu_state == 0x0e) {
        menu_setNumPlayers((unsigned)(int8_t)GameStruct.NumPlayers);
        newMenu_DrawVSMode(
            player_one, player_two, menuVars.pad[0], 0);
        newMenu_state = 0;
        if (newMenu_bAbortMenu != 0) {
            GameStruct.NumPlayers = 1;
            return -1;
        }
        return 1;
    } else if (newMenu_state == 0x18) {
        if ((newMenu_select & UINT32_C(1)) == 0) {
            if ((menuVars.pad[0] & JPB_PAD_LEFT) != 0) {
                do {
                    ++GameStruct.ModelSelect[0];
                    if (GameStruct.ModelSelect[0] > 79) {
                        GameStruct.ModelSelect[0] = 0;
                    }
                } while (!jedi_CheckValidVersus(
                    GameStruct.ModelSelect[0]));
                newMenu_VSSound("xjedscrl");
            } else if ((menuVars.pad[0] & JPB_PAD_RIGHT) != 0) {
                do {
                    --GameStruct.ModelSelect[0];
                    if (GameStruct.ModelSelect[0] < 0) {
                        GameStruct.ModelSelect[0] = 79;
                    }
                } while (!jedi_CheckValidVersus(
                    GameStruct.ModelSelect[0]));
                newMenu_VSSound("xjedscrl");
            } else if ((menuVars.pad[0] & JPB_PAD_COMBO_SOUTH) != 0 &&
                       (newMenu_select == 0 ||
                        GameStruct.ModelSelect[1] !=
                            GameStruct.ModelSelect[0])) {
                newMenu_select |= UINT32_C(1);
                newMenu_VSSound("xjedsel");
            }
        }

        if ((((newMenu_select & UINT32_C(2)) == 0) &&
             GameStruct.NumPlayers == 2) ||
            (newMenu_select & UINT32_C(1)) != 0) {
            pad_two = menuVars.pad[1];
            if (GameStruct.NumPlayers == 1) {
                pad_two = menuVars.pad[0];
            }
            if ((pad_two & JPB_PAD_LEFT) != 0) {
                do {
                    ++GameStruct.ModelSelect[1];
                    if (GameStruct.ModelSelect[1] > 79) {
                        GameStruct.ModelSelect[1] = 0;
                    }
                } while (!jedi_CheckValidVersus(
                    GameStruct.ModelSelect[1]));
                newMenu_VSSound("xjedscrl");
            } else if ((pad_two & JPB_PAD_RIGHT) != 0) {
                do {
                    --GameStruct.ModelSelect[1];
                    if (GameStruct.ModelSelect[1] < 0) {
                        GameStruct.ModelSelect[1] = 79;
                    }
                } while (!jedi_CheckValidVersus(
                    GameStruct.ModelSelect[1]));
                newMenu_VSSound("xjedscrl");
            } else if ((pad_two & JPB_PAD_COMBO_SOUTH) != 0 &&
                       (newMenu_select == 0 ||
                        GameStruct.ModelSelect[1] !=
                            GameStruct.ModelSelect[0])) {
                newMenu_select |= UINT32_C(2);
                newMenu_VSSound("xjedsel");
            }
        }
        if (((pad_two | menuVars.pad[0]) & JPB_PAD_JUMP) != 0) {
            newMenu_bAbortMenu = 1;
            newMenu_state = 0x0e;
            newMenu_VSSound("xjedscrl");
        }
        if (newMenu_select == UINT32_C(3)) {
            newMenu_state = 0x0e;
        }
    }
    newMenu_DrawVSMode(
        player_one,
        player_two,
        menuVars.pad[0],
        pad_two);
    return 0;
}

/* 0xD7750, 52 bytes, global, 1 named locals
 * newMenu_SelectPlayers
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int newMenu_SelectPlayers(int num_players)
{
    SetDispMask(1);
    if (num_players == 1) {
        menu_DrawOnePlayer();
        return 1;
    }
    menu_DrawTwoPlayer();
    return 1;
}

/* 0xD7790, 1024 bytes, global, 0 named locals
 * newMenu_Training
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int newMenu_Training(void)
{
    uint32_t pad = menuVars.pad[0];

    if (p1Disconnected != 0) {
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        menu_drawReconnect();
        return 0;
    }
    if (newMenu_state == 0) {
        newMenu_errorState = 0x10;
        newMenu_bAbortMenu = 0;
        newMenu_select = 0;
        newMenu_state = 1;
        newMenu_trainLevel = 1;
    } else if (newMenu_state == 1) {
        GameStruct.NumPlayers = 1;
        if (jpb_menu_platform_hooks.assignBackToP1 != NULL) {
            (void)jpb_menu_platform_hooks.assignBackToP1(
                jpb_menu_platform_user_data);
        }
        GameStruct.NumPlayers = 1;
        game_CLR_GLOBALBIT(2);
        menu_initPlayerSelect();
        jedi_InitLives();
        newMenu_state = 0x16;
    } else if (newMenu_state == 0x0e) {
        newMenu_DrawTraining(GameStruct.ModelSelect[0], pad);
        newMenu_state = 0;
        if (newMenu_bAbortMenu != 0) {
            return -1;
        }
        menu_startTraining((unsigned)newMenu_trainLevel - 1u);
        return 1;
    } else if (newMenu_state == 0x16) {
        if ((pad & JPB_PAD_LEFT) != 0) {
            do {
                ++GameStruct.ModelSelect[0];
                if (GameStruct.ModelSelect[0] > plasma_model) {
                    GameStruct.ModelSelect[0] = 0;
                }
            } while (!jedi_CheckValidPlayer(GameStruct.ModelSelect[0]));
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedscrl", jpb_menu_platform_user_data);
            }
        } else if ((pad & JPB_PAD_RIGHT) != 0) {
            do {
                --GameStruct.ModelSelect[0];
                if (GameStruct.ModelSelect[0] < 0) {
                    GameStruct.ModelSelect[0] = plasma_model;
                }
            } while (!jedi_CheckValidPlayer(GameStruct.ModelSelect[0]));
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedscrl", jpb_menu_platform_user_data);
            }
        } else if ((pad & JPB_PAD_JUMP) != 0) {
            newMenu_bAbortMenu = 1;
            newMenu_state = 0x0e;
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedscrl", jpb_menu_platform_user_data);
            }
        } else if ((pad & JPB_PAD_COMBO_SOUTH) != 0) {
            newMenu_state = 0x17;
            newMenu_select |= UINT32_C(1);
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedsel", jpb_menu_platform_user_data);
            }
        }
    } else if (newMenu_state == 0x17) {
        if ((pad & JPB_PAD_LEFT) != 0) {
            ++newMenu_trainLevel;
            if (newMenu_trainLevel > 7) {
                newMenu_trainLevel = 1;
            }
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xlvbrows", jpb_menu_platform_user_data);
            }
        } else if ((pad & JPB_PAD_RIGHT) != 0) {
            --newMenu_trainLevel;
            if (newMenu_trainLevel < 1) {
                newMenu_trainLevel = 7;
            }
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xlvbrows", jpb_menu_platform_user_data);
            }
        } else if ((pad & JPB_PAD_JUMP) != 0) {
            newMenu_bAbortMenu = 1;
            newMenu_state = 0x0e;
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xjedscrl", jpb_menu_platform_user_data);
            }
        } else if ((pad & JPB_PAD_COMBO_SOUTH) != 0) {
            newMenu_state = 0x0e;
            newMenu_select |= UINT32_C(2);
            if (jpb_menu_platform_hooks.soundCue != NULL) {
                jpb_menu_platform_hooks.soundCue(
                    "xlvselct", jpb_menu_platform_user_data);
            }
        }
    }

    if ((pad & JPB_PAD_ITEM) != 0 &&
        jedi_CanToggleSaber((model_id)GameStruct.ModelSelect[0])) {
        jedi_ToggleSaberColor((model_id)GameStruct.ModelSelect[0]);
        if (jpb_menu_platform_hooks.soundCue != NULL) {
            jpb_menu_platform_hooks.soundCue(
                "xopt_sel", jpb_menu_platform_user_data);
        }
    }
    newMenu_DrawTraining(GameStruct.ModelSelect[0], pad);
    return 0;
}

/* 0xD7B90, 1051 bytes, global, 4 named locals
 * newMenu_VSMode
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void newMenu_VSSound(const char *cue)
{
    if (jpb_menu_platform_hooks.soundCue != NULL) {
        jpb_menu_platform_hooks.soundCue(
            cue, jpb_menu_platform_user_data);
    }
}

static void newMenu_VSAdvanceModel(int player, int direction)
{
    do {
        GameStruct.ModelSelect[player] = (int16_t)(
            GameStruct.ModelSelect[player] + direction);
        if (GameStruct.ModelSelect[player] < 0) {
            GameStruct.ModelSelect[player] = jar_jar_playable_model;
        } else if (GameStruct.ModelSelect[player] >
                   jar_jar_playable_model) {
            GameStruct.ModelSelect[player] = 0;
        }
    } while (!jedi_CheckValidVersus(GameStruct.ModelSelect[player]));
}

int newMenu_VSMode(void)
{
    int player_one = GameStruct.ModelSelect[0];
    int player_two = GameStruct.ModelSelect[1];
    uint32_t pad_two = 0;

    SetDispMask(1);
    if (newMenu_state == 0) {
        newMenu_bAbortMenu = 0;
        newMenu_errorState = 0x10;
        newMenu_state = 1;
        newMenu_select = 0;
    } else if (newMenu_state == 1) {
        menu_setNumPlayers((unsigned)(uint8_t)GameStruct.NumPlayers);
        newMenu_state = 0x18;
    } else if (newMenu_state == 0x0e) {
        GameStruct.NumPlayers = (char)tempPlayersVs;
        menu_setNumPlayers((unsigned)(uint8_t)GameStruct.NumPlayers);
        newMenu_DrawVSMode(
            player_one, player_two, menuVars.pad[0], 0);
        newMenu_state = 0;
        if (newMenu_bAbortMenu != 0) {
            GameStruct.NumPlayers = 1;
            return -1;
        }
        return 1;
    } else if (newMenu_state == 0x18) {
        if ((newMenu_select & UINT32_C(1)) == 0) {
            if ((menuVars.pad[0] & JPB_PAD_LEFT) != 0) {
                newMenu_VSAdvanceModel(0, 1);
                newMenu_VSSound("xjedscrl");
            } else if ((menuVars.pad[0] & JPB_PAD_RIGHT) != 0) {
                newMenu_VSAdvanceModel(0, -1);
                newMenu_VSSound("xjedscrl");
            } else if ((menuVars.pad[0] & JPB_PAD_COMBO_SOUTH) != 0 &&
                       (newMenu_select == 0 ||
                        GameStruct.ModelSelect[1] !=
                            GameStruct.ModelSelect[0])) {
                newMenu_select |= UINT32_C(1);
                newMenu_VSSound("xjedsel");
            }
        }

        if (((newMenu_select & UINT32_C(2)) == 0 &&
             GameStruct.NumPlayers == 2) ||
            (newMenu_select & UINT32_C(1)) != 0) {
            pad_two = GameStruct.NumPlayers == 1
                ? menuVars.pad[0]
                : menuVars.pad[1];
            if ((pad_two & JPB_PAD_LEFT) != 0) {
                newMenu_VSAdvanceModel(1, 1);
                newMenu_VSSound("xjedscrl");
            } else if ((pad_two & JPB_PAD_RIGHT) != 0) {
                newMenu_VSAdvanceModel(1, -1);
                newMenu_VSSound("xjedscrl");
            } else if ((pad_two & JPB_PAD_COMBO_SOUTH) != 0 &&
                       (newMenu_select == 0 ||
                        GameStruct.ModelSelect[1] !=
                            GameStruct.ModelSelect[0])) {
                newMenu_select |= UINT32_C(2);
                newMenu_VSSound("xjedsel");
            }
        }
        if (((pad_two | menuVars.pad[0]) & JPB_PAD_JUMP) != 0) {
            newMenu_bAbortMenu = 1;
            newMenu_state = 0x0e;
            newMenu_VSSound("xjedscrl");
        }
        if (newMenu_select == UINT32_C(3)) {
            newMenu_state = 0x0e;
        }
    }
    newMenu_DrawVSMode(
        player_one, player_two, menuVars.pad[0], pad_two);
    return 0;
}

/* 0xD7FB0, 569 bytes, global, 7 named locals
 * redlineFunc
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void redlineFunc(void)
{
    static const float bounds[3][4] = {
        {108.0f, 314.0f, 1264.0f, 808.0f},
        {66.0f, 450.0f, 1222.0f, 944.0f},
        {25.0f, 582.0f, 1181.0f, 1076.0f}
    };
    float clip_left = 0.0f;
    float clip_top = 0.0f;
    float clip_right = 0.0f;
    float clip_bottom = 0.0f;
    SCREENRECT scissor;
    CVECTOR color = {0x87, 0x0e, 0x17, 0xff};
    unsigned index;

    setPivotPositionMM(&clip_left, &clip_top, 0);
    setPivotPositionMM(&clip_right, &clip_bottom, 8);
    scissor.left = (int32_t)clip_left;
    scissor.top = (int32_t)clip_top;
    scissor.right = (int32_t)clip_right;
    scissor.bottom = (int32_t)clip_bottom;
    for (index = 0; index < 3; ++index) {
        float left = bounds[index][0];
        float top = bounds[index][1];
        float right = bounds[index][2];
        float bottom = bounds[index][3];
        SCREENRECT destination;

        setPivotPositionMM(&left, &top, 0);
        setPivotPositionMM(&right, &bottom, 0);
        destination.left = (int32_t)left;
        destination.top = (int32_t)top;
        destination.right = (int32_t)right;
        destination.bottom = (int32_t)bottom;
        _DrawTextureClipped(
            controlTextures[2], destination, NULL, color, 0.8f, scissor);
    }
}

/* 0xD81F0, 4991 bytes, global, 38 named locals
 * runControlsMenu
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static void menu_drawControllerMaterial(
    _Material *material, float center_x, float top_y)
{
    float left;
    float right;
    float bottom;
    SCREENRECT dst;
    CVECTOR color = {225, 225, 225, 255};

    left = center_x - (float)material->iw * 0.15f;
    right = center_x + (float)material->iw * 0.15f;
    bottom = top_y + (float)material->ih * 0.3f;
    setPivotPositionMM(&left, &top_y, 4);
    setPivotPositionMM(&right, &bottom, 4);
    dst.left = (int32_t)left;
    dst.top = (int32_t)top_y;
    dst.right = (int32_t)right;
    dst.bottom = (int32_t)bottom;
    _DrawTexture(material, dst, NULL, color, 0.0f);
}

static float menu_controllerIconLeft(
    _Material *material, float center_x)
{
    return center_x - (float)material->iw * 0.15f;
}

static void menu_drawControlsText(
    unsigned text_index,
    float x,
    float y,
    int mode,
    float scale)
{
    setPivotPositionMM(&x, &y, 4);
    (void)SDLTextWriteScaleMM(
        11, 255, mode, (int)x, (int)y,
        scale, 0, "%s", allText[text_index]);
}

static void menu_drawControllerOverview(
    _Material **textures,
    const unsigned char *control_scheme,
    const unsigned char *force_scheme,
    int player,
    int keyboard,
    float primary_center,
    float force_center,
    float force_second_center,
    float force_text_offset)
{
    unsigned row = 0;
    float y = 130.0f;
    const float player_offset = player == 0 ? -450.0f : 450.0f;

    while (row < 7) {
        unsigned action = row == 1 ? 2u : row;
        _Material *primary = textures[control_scheme[action]];
        float primary_x = primary_center + player_offset;
        float primary_left = menu_controllerIconLeft(
            primary, primary_x);

        menu_drawControllerMaterial(primary, primary_x, y);
        menu_drawControlsText(
            controlTextList[action],
            primary_left + scaleAdjustmentMM * 65.0f,
            y, 0, 1.75f);

        if (action < 6) {
            _Material *force = textures[force_scheme[action]];
            float force_x = force_center + player_offset;
            float force_left;

            if (keyboard && player == 0) {
                if (action != 0) {
                    force = kbmForceTextures[action - 2u];
                }
                menu_drawControllerMaterial(force, force_x, y);
            } else if (action == 0) {
                menu_drawControllerMaterial(force, force_x, y);
            } else {
                _Material *modifier = textures[force_scheme[6]];
                float plus_x;
                float plus_y;

                menu_drawControllerMaterial(modifier, force_x, y);
                plus_x = force_x +
                    (float)force->iw * 0.15f +
                    scaleAdjustmentMM * 12.0f;
                plus_y = y + scaleAdjustmentMM * 8.0f;
                setPivotPositionMM(&plus_x, &plus_y, 4);
                (void)SDLTextWriteScaleMM(
                    11, 255, 0, (int)plus_x, (int)plus_y,
                    1.75f, 0, "+");
                menu_drawControllerMaterial(
                    force,
                    force_second_center + player_offset,
                    y);
            }
            force_left = menu_controllerIconLeft(force, force_x);
            menu_drawControlsText(
                controlTextListForce[action],
                force_left + force_text_offset,
                y, 0, 1.75f);
        }
        y += 46.0f;
        row = action + 1u;
    }
}

void runControlsMenu(void)
{
    _Material *p1Textures[10] = {0};
    _Material *p2Textures[10] = {0};
    const unsigned char *p1_scheme = ClassicControlScheme;
    const unsigned char *p1_force = ClassicControlSchemeForce;
    const unsigned char *p2_scheme = ClassicControlScheme;
    const unsigned char *p2_force = ClassicControlSchemeForce;
    float primary_center = -420.0f;
    float force_center = -55.0f;
    float p2_force_center = -55.0f;
    float force_second_center = 104.0f;
    float force_text_offset = 29.0f;
    float x;
    float y;
    int show_player_two =
        GameStruct.NumPlayers == 2 && (padExist & 2u) != 0;

    (void)getControllerTextures(0, p1Textures);
    (void)getControllerTextures(1, p2Textures);
    menuVars.selectp = menuVars.mmSelect1;
    menuVars.mmAnchorType = 4;
    menuVars.textScale = 2.25f;
    menuVars.textSpacer = 60.0f;
    mmDraw(controlSubDraw);

    x = -450.0f;
    y = -470.0f;
    menu_drawControlsText(237, x, y, 2, 2.25f);
    x = 0.0f;
    y = 100.0f;
    setPivotPositionMM(&x, &y, 1);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)x, (int)y,
        1.75f, 0, "%s", newMenu_Text(242));
    {
        SCREENRECT dst;
        CVECTOR color = {225, 225, 225, 255};
        float left = -621.0f;
        float top = -390.0f;
        float right = -279.0f;
        float bottom = -158.0f;

        setPivotPositionMM(&left, &top, 4);
        setPivotPositionMM(&right, &bottom, 4);
        dst.left = (int32_t)left;
        dst.top = (int32_t)top;
        dst.right = (int32_t)right;
        dst.bottom = (int32_t)bottom;
        _DrawTexture(p1Textures[8], dst, NULL, color, 0.0f);
    }
    menu_mainMenu(controls1Mdef);

    if (show_player_two) {
        SCREENRECT dst;
        CVECTOR color = {225, 225, 225, 255};
        float left = 279.0f;
        float top = -390.0f;
        float right = 621.0f;
        float bottom = -158.0f;

        menu_drawControlsText(238, 450.0f, -470.0f, 2, 2.25f);
        setPivotPositionMM(&left, &top, 4);
        setPivotPositionMM(&right, &bottom, 4);
        dst.left = (int32_t)left;
        dst.top = (int32_t)top;
        dst.right = (int32_t)right;
        dst.bottom = (int32_t)bottom;
        _DrawTexture(p2Textures[8], dst, NULL, color, 0.0f);
        menu_mainMenu(controls2Mdef);
    }

    switch (OptionStruct.ResolutionChanged) {
    case 4:
        primary_center = -410.0f;
        force_center = -35.0f;
        p2_force_center = -35.0f;
        force_second_center = 44.0f;
        force_text_offset = 44.0f;
        break;
    case 1:
    case 2:
        primary_center = -390.0f;
        force_center = -15.0f;
        p2_force_center = -15.0f;
        force_second_center = 64.0f;
        force_text_offset = 64.0f;
        break;
    case 3:
        primary_center = -310.0f;
        force_center = 25.0f;
        p2_force_center = 5.0f;
        force_second_center = 84.0f;
        force_text_offset = 84.0f;
        break;
    case 5:
        force_second_center = 29.0f;
        break;
    default:
        break;
    }

    if (OptionStruct.ControllerConfig[0] == 1 &&
        lastUsedInputType != 0) {
        p1_scheme = ModernControlScheme;
        p1_force = ModernControlSchemeForce;
    }
    if (OptionStruct.ControllerConfig[1] == 1) {
        p2_scheme = ModernControlScheme;
        p2_force = ModernControlSchemeForce;
    }
    menu_drawControllerOverview(
        p1Textures, p1_scheme, p1_force, 0,
        lastUsedInputType == 0,
        primary_center, force_center,
        force_second_center, force_text_offset);
    if (show_player_two) {
        menu_drawControllerOverview(
            p2Textures, p2_scheme, p2_force, 1, 0,
            primary_center, p2_force_center,
            force_second_center, force_text_offset);
    }
}

/* 0xD9570, 3 bytes, global, 0 named locals
 * runInitialMemcard
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void runInitialMemcard(void)
{
}

/* 0xD9580, 3 bytes, global, 0 named locals
 * runSaveMenu
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void runSaveMenu(void)
{
}

/* 0xD9590, 45 bytes, global, 3 named locals
 * scoreloadart
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void scoreloadart(unsigned player)
{
    char *name;
    int texture;
    int model = GameStruct.ModelSelect[player];

    if (model >= 23) {
        (void)newMenu_GetVSExtraPlayer(&name, &texture, model);
    }
}

/* 0xD95C0, 244 bytes, global, 6 named locals
 * testcombo
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void testcombo(unsigned player)
{
    playerObject *player_data = &gaPlayerData[player];
    unsigned combo;
    unsigned count = 0;

    menuVars.td.jedi = (uint32_t)(int32_t)player_data->playerID;
    for (combo = 0;
         combo < (unsigned)(int)player_data->maxCombos;
         ++combo) {
        menuVars.td.newcombos[combo] = 0;
        if (game_getCombo(menuVars.td.jedi, combo) == 0 &&
            player_data->paCombos[combo].String[0] != '\0' &&
            combo_ValidComboAward((int)player, (int)combo) != 0 &&
            menu_checkCombo(player, (uint16_t)combo) != 0) {
            menuVars.td.newcombos[count] = (uint8_t)combo;
            ++count;
        }
    }
    menuVars.td.comboListCount = count;
}

/* 0xD96C0, 3 bytes, global, 0 named locals
 * turnOffBackground
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void turnOffBackground(void)
{
}

/* 0xD96D0, 209 bytes, global, 3 named locals
 * updatePlayerSelectIndex
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void updatePlayerSelectIndex(int player)
{
    uint8_t index = menuVars.pplayers[player];
    int difference = (int)index - (int)menuVars.subplayers[player];
    int direction = difference;
    uint8_t model;

    if (abs(difference) > 1) {
        direction = difference > 0 ? -1 : 1;
    }
    model = modisorder2[index];
    if (model < last_jedi_model) {
        menuVars.pplayers[player] = index;
        return;
    }
    while (GetCharacterByID((model_id)model)->Unlocked == 0) {
        index = (uint8_t)(((int)index + direction + 23) % 23);
        model = modisorder2[index];
        if (model < last_jedi_model) {
            break;
        }
    }
    menuVars.pplayers[player] = index;
}
