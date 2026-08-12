/*
 * PARTIAL REVIEWED RECONSTRUCTION.
 *
 * The exact MENUVARS title stack, title initialization, player/model
 * selection, objective/game-over transitions, and small menu state leaves
 * are now recovered under their PDB names. Platform input enumeration,
 * texture loading, and bucket setup remain explicit dependency-light hooks.
 * menu_specialMess preserves its exact transition and menu-choice IDs while
 * its still-pending renderer remains behind a callback.
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
#include "jpb/alltext.h"
#include "jpb/audio_stream.h"
#include "jpb/camera.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/jedi.h"
#include "jpb/pwrup.h"
#include "jpb/resources.h"
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

static JPBMenuSpecialMessageHook
    jpb_menu_special_message_hook;
static void *jpb_menu_special_message_user_data;
static JPBMenuP1CharacterSelectDrawHook
    jpb_menu_p1_character_select_draw_hook;
static void *jpb_menu_p1_character_select_draw_user_data;
static JPBMenuP2CharacterSelectDrawHook
    jpb_menu_p2_character_select_draw_hook;
static void *jpb_menu_p2_character_select_draw_user_data;
static JPBMenuPlatformHooks jpb_menu_platform_hooks;
static void *jpb_menu_platform_user_data;

void menu_drawSelectBox(void);
void menu_drawSelectors(void);
void menu_fadeBG(void);
static void menu_drawSelector(float x, float y);
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

/* Exact PDB globals owned by the matched menu presentation path. */
MENUVARS menuVars;
unsigned menuTexLoaded;
unsigned menuTexLoaded2;
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

void jpb_MenuSetSpecialMessageHook(
    JPBMenuSpecialMessageHook hook,
    void *user_data)
{
    jpb_menu_special_message_hook = hook;
    jpb_menu_special_message_user_data =
        user_data;
}

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

/* 0xBF020, 608 bytes, global, 7 named locals
 * menu_scoreComboDraw
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

/* 0xBF5E0, 386 bytes, global, 4 named locals
 * drawControlsIcon
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
static const wchar_t *menu_skipPromptGlyph(const wchar_t *text)
{
    if (text == NULL) {
        return L"";
    }
    while (*text != L'\0' && (*text > 0x7f || *text == L' ')) {
        ++text;
    }
    return text;
}

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
    if (lastUsedInputType == 0) {
        _Material *textures[10] = {0};
        SCREENRECT destination;
        CVECTOR white = {255, 255, 255, 255};
        int icon_size = (int)(64.0f * scaleAdjustmentMM);
        int gap = (int)(20.0f * scaleAdjustmentMM);
        int text_x;

        (void)getControllerTextures(0, textures);
        if (textures[9] != NULL && icon_size > 0) {
            destination.left = (int32_t)exit_x;
            destination.top = (int32_t)exit_y;
            destination.right = destination.left + icon_size;
            destination.bottom = destination.top + icon_size;
            _DrawTexture(textures[9], destination, NULL, white, 0.0f);
        }
        text_x = (int32_t)exit_x + icon_size + gap;
        (void)SDLTextWriteScaleMM(
            15, 255, 0, text_x, (int)exit_y,
            2.25f, 0, L"%ls",
            menu_skipPromptGlyph(allText[exit_text]));

        if (textures[2] != NULL && icon_size > 0) {
            destination.left = (int32_t)(
                select_x - 210.0f * scaleAdjustmentMM);
            destination.top = (int32_t)select_y;
            destination.right = destination.left + icon_size;
            destination.bottom = destination.top + icon_size;
            _DrawTexture(textures[2], destination, NULL, white, 0.0f);
        }
        (void)SDLTextWriteScaleMM(
            15, 255, 1, (int)select_x, (int)select_y,
            2.25f, 0, L"%ls",
            menu_skipPromptGlyph(allText[select_text]));
        return;
    }
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)exit_x, (int)exit_y,
        2.25f, 0, L"%ls", allText[exit_text]);
    (void)SDLTextWriteScaleMM(
        15, 255, 1, (int)select_x, (int)select_y,
        2.25f, 0, L"%ls", allText[select_text]);
}

static void menu_drawLevelSelectControlsIcon(void)
{
    float exit_text_x;
    float select_text_x;
    float exit_text_y = 246.0f;
    float select_text_y = 193.0f;
    float exit_glyph_x = 389.0f;
    float exit_glyph_y = 21.0f;
    float select_glyph_x = 469.0f;
    float select_glyph_y = 61.0f;
    unsigned exit_text;
    unsigned select_text;
    _Material *textures[10] = {0};
    int use_texture_prompts = 0;

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
    (void)getControllerTextures(0, textures);
    use_texture_prompts = textures[9] != NULL && textures[2] != NULL;
    if (!use_texture_prompts &&
        kbmTextures[9] != NULL && kbmTextures[2] != NULL) {
        memcpy(textures, kbmTextures, sizeof(kbmTextures));
        exit_text = 476u;
        select_text = 475u;
        use_texture_prompts = 1;
    }
    if (use_texture_prompts) {
        SCREENRECT destination;
        CVECTOR white = {255, 255, 255, 255};
        int icon_size = (int)(32.0f * scaleAdjustmentMM);
        int gap = (int)(10.0f * scaleAdjustmentMM);

        if (textures[9] != NULL && icon_size > 0) {
            destination.left = (int32_t)exit_text_x;
            destination.top = (int32_t)exit_text_y;
            destination.right = destination.left + icon_size;
            destination.bottom = destination.top + icon_size;
            _DrawTexture(textures[9], destination, NULL, white, 0.0f);
        }
        (void)SDLTextWriteScaleMM(
            15, 255, 0,
            (int)exit_text_x + icon_size + gap,
            (int)exit_text_y,
            1.75f, 0, L"%ls",
            menu_skipPromptGlyph(allText[exit_text]));
        if (textures[2] != NULL && icon_size > 0) {
            destination.left = (int32_t)select_text_x;
            destination.top = (int32_t)select_text_y;
            destination.right = destination.left + icon_size;
            destination.bottom = destination.top + icon_size;
            _DrawTexture(textures[2], destination, NULL, white, 0.0f);
        }
        (void)SDLTextWriteScaleMM(
            15, 255, 0,
            (int)select_text_x + icon_size + gap,
            (int)select_text_y,
            1.75f, 0, L"%ls",
            menu_skipPromptGlyph(allText[select_text]));
    } else {
        (void)SDLTextWriteScaleMM(
            15, 255, 0, (int)exit_text_x, (int)exit_text_y,
            1.75f, 0, L"%ls", allText[exit_text]);
        (void)SDLTextWriteScaleMM(
            15, 255, 0, (int)select_text_x, (int)select_text_y,
            1.75f, 0, L"%ls", allText[select_text]);
        (void)menu_drawLevelSelectPsxTexture(
            0x113, exit_glyph_x, exit_glyph_y,
            scaleAdjustmentMM * 188.0f, 0.0f, 0x8000u, 0x0b, 0.8f);
        (void)menu_drawLevelSelectPsxTexture(
            0x114, select_glyph_x, select_glyph_y,
            scaleAdjustmentMM * 117.0f, 0.0f, 0x8000u, 0x0b, 0.8f);
    }
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

/* 0xBFC90, 1230 bytes, global, 13 named locals
 * drawScoreMenus
 * PDB type: void (int, AWARDSET*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC0160, 182 bytes, global, 5 named locals
 * fixPSPos
 * PDB type: void (float*, float*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC0220, 302 bytes, global, 8 named locals
 * genComboStrings
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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
        2.25f, 0, L"%ls", allText[exit_text]);
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)select_x, (int)select_y,
        2.25f, 0, L"%ls", allText[select_text]);
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

/* 0xC0800, 1553 bytes, global, 8 named locals
 * initsavegamestruct
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC0E20, 231 bytes, global, 8 named locals
 * loadVRM
 * PDB type: void (char*, unsigned, unsigned,...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC0F10, 103 bytes, global, 0 named locals
 * menuBucketFront
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC0F80, 93 bytes, global, 0 named locals
 * menuBucketSavegame
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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
    wchar_t page_text[24];
    float x;
    float y;
    float right_x;
    float right_y;
    float content_width;
    float content_height;

    if (p1Disconnected != 0) {
        const wchar_t *message =
            allText[494] != NULL ? allText[494] : L"Controller disconnected";

        x = 0.0f;
        y = 0.0f;
        setPivotPositionMM(&x, &y, 4);
        (void)SDLTextWriteScaleMM(
            15, 255, 2, (int)x, (int)y,
            1.75f, 0, L"%ls", message);
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
    (void)swprintf(
        page_text,
        sizeof(page_text) / sizeof(page_text[0]),
        menuVars.scoreScore < 10 ? L"0%u / %u" : L"%u / %u",
        menuVars.scoreScore,
        (unsigned)JPB_CONCEPT_ART_PAGE_COUNT);
    (void)SDLTextWriteScaleMM(
        9, 255, 1, (int)x, (int)y,
        2.25f, 0, L"%ls", page_text);

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
        2.25f, 0, L"%ls",
        allText[222] != NULL ? allText[222] : L"Exit");

    menuVars.titleDispEnable = 0;
    menuVars.titleArt = 0;
    content_width = (float)OptionStruct.ScreenWidth;
    content_height = (float)OptionStruct.ScreenHeight;
    if (content_height > 0.0f &&
        1.7777778f <= content_width / content_height) {
        float fitted_width = content_height * 1.7777778f;

        destination.left = (int32_t)((content_width - fitted_width) * 0.5f);
        destination.top = 0;
        destination.right = (int32_t)(destination.left + fitted_width);
        destination.bottom = (int32_t)content_height;
    } else if (content_width > 0.0f) {
        float fitted_height = content_width / 1.7777778f;

        destination.left = 0;
        destination.top = (int32_t)((content_height - fitted_height) * 0.5f);
        destination.right = (int32_t)content_width;
        destination.bottom = (int32_t)(destination.top + fitted_height);
    } else {
        memset(&destination, 0, sizeof(destination));
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

/* 0xC1470, 167 bytes, global, 3 named locals
 * menuPreString
 * PDB type: void (unsigned char*, unsigned c...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC1520, 158 bytes, global, 1 named locals
 * menuPushKey
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC15C0, 5 bytes, global, 2 named locals
 * menu_CheckValidLevel
 * PDB type: int (unsigned, unsigned*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

/* 0xC1780, 192 bytes, global, 5 named locals
 * menu_DrawArrows
 * PDB type: void (unsigned long, int, int, i...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC1840, 885 bytes, global, 6 named locals
 * menu_DrawOnePlayer
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC1BC0, 1346 bytes, global, 9 named locals
 * menu_DrawTwoPlayer
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC2110, 3 bytes, global, 2 named locals
 * menu_FormatMenu
 * PDB type: int (int, unsigned long)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC2120, 74 bytes, global, 0 named locals
 * menu_JumpCheckPoint
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

/* 0xC21E0, 43 bytes, global, 1 named locals
 * menu_addTotal
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_addTotal(unsigned amount)
{
    if (loadScreenFlag != 0) {
        loadTotal += amount;
        /*
         * The matched tail redraws and presents the loading screen. Those
         * two platform-facing procedures remain presentation boundaries;
         * the exact game-owned progress state is retained here.
         */
    }
}

/* 0xC2210, 124 bytes, global, 2 named locals
 * menu_applyvideooptions
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC2290, 708 bytes, global, 7 named locals
 * menu_buildComboString
 * PDB type: void (unsigned char*, unsigned c...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC2560, 262 bytes, global, 7 named locals
 * menu_calcCompletionPoints
 * PDB type: unsigned (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC2670, 57 bytes, global, 3 named locals
 * menu_calcTbarspeed
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

/* 0xC2970, 113 bytes, global, 4 named locals
 * menu_checkCombo
 * PDB type: unsigned (unsigned, short)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

/* 0xC2B80, 63 bytes, global, 1 named locals
 * menu_copyCouncil
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC2BC0, 1022 bytes, global, 12 named locals
 * menu_councilPos
 * PDB type: void (playerObject*, VECTOR*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC2FC0, 68 bytes, global, 1 named locals
 * menu_decAwardLevel
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC3010, 284 bytes, global, 1 named locals
 * menu_demoMovie
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

/* 0xC33F0, 3 bytes, global, 9 named locals
 * menu_drawColorPoly
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC3400, 3 bytes, global, 11 named locals
 * menu_drawColorPolyG4
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC3410, 537 bytes, global, 11 named locals
 * menu_drawCombos
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC3630, 230 bytes, global, 4 named locals
 * menu_drawController
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC3720, 831 bytes, global, 11 named locals
 * menu_drawCredits
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawCredits(void)
{
    wchar_t line[256];
    float multiplier;
    float base;
    float line_spacing;
    unsigned shown = 0;
    unsigned loop1;
    float exit_x;
    float exit_y;

    if (skipCreditForFrame != 0) {
        skipCreditForFrame = 0;
        return;
    }
    multiplier = OptionStruct.ScreenHeight != 0
        ? (float)OptionStruct.ScreenHeight / 1080.0f
        : 1.0f;
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
                line[output_index++] =
                    (wchar_t)source[source_index++];
            }
            line[output_index] = L'\0';
            (void)SDLTextWriteScaleMM(
                15 + heading,
                255,
                2,
                (int)(OptionStruct.ScreenWidth / 2u),
                (int)base,
                2.0f,
                0,
                L"%ls",
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
        2.25f, 0, L"%ls",
        allText[222] != NULL ? allText[222] : L"Exit");
}

/* 0xC3A60, 1678 bytes, global, 16 named locals
 * menu_drawEULA
 * PDB type: void (unsigned char**, int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

    if (texture_index >= 249 || menuTextures[texture_index] == NULL) {
        return;
    }
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

static void menu_drawLevelSelectBackgroundSlice(
    float left,
    float top,
    float right,
    float bottom,
    CVECTOR color,
    float layer)
{
    SCREENRECT destination;
    SCREENRECT source;
    float dst_left = left;
    float dst_top = top;
    float dst_right = right;
    float dst_bottom = bottom;

    if (menuTextures[164] == NULL || left >= right || top >= bottom) {
        return;
    }
    setPivotPositionMM(&dst_left, &dst_top, 0);
    setPivotPositionMM(&dst_right, &dst_bottom, 0);
    destination.left = (int32_t)dst_left;
    destination.top = (int32_t)dst_top;
    destination.right = (int32_t)dst_right;
    destination.bottom = (int32_t)dst_bottom;
    source.left = (int32_t)left;
    source.top = (int32_t)top;
    source.right = (int32_t)right;
    source.bottom = (int32_t)bottom;
    _DrawTexture(menuTextures[164], destination, &source, color, layer);
}

static void menu_drawLevelSelectMaskBands(CVECTOR color)
{
    menu_drawLevelSelectBackgroundSlice(
        0.0f, 0.0f, 1920.0f, 112.0f, color, 0.64f);
    menu_drawLevelSelectBackgroundSlice(
        0.0f, 804.0f, 1920.0f, 1080.0f, color, 0.64f);
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
    gPSXDrawScaleX = 3.75f;
    gPSXDrawScaleY = 4.5f;
    setPivotPositionMM_PSX(&x, &y, 0);
    return jpb_PsxDrawTextureColorIndexLayer(
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

    menu_drawLevelSelectTexture(
        164, 0.0f, 0.0f, 0.0f, 0.0f, 0, 8,
        panel_color, 0.9f);

    if (interactive != 0) {
        menu_levelSelectMenu(levelSelectMdef);
        level = (int)(int8_t)LevelSelect;
        tens = level / 10;
        ones = level - tens * 10;
    }

    text_index = (unsigned)(305 + level);
    if (text_index >= JPB_ALL_TEXT_CAPACITY ||
        allText[text_index] == NULL) {
        text_index = 306;
    }

    if (level >= 1 && level <= 14 &&
        (unsigned)(410 + level) < JPB_FONT_SPEC_COUNT) {
        unsigned material_index = fontSpec[410 + level].clut;

        if (material_index < 249) {
            menu_drawLevelSelectTexture(
                material_index,
                116.0f, 92.0f, 960.0f, 354.5f, 0, 8,
                preview_color, 0.5f);
        }
    }

    menu_drawSelectBox();
    menu_drawSelectors();
    menu_drawLevelSelectMaskBands(panel_color);
    menu_drawLevelSelectTexture(
        165, 0.0f, 0.0f, 0.0f, 0.0f, 0, 8,
        panel_color, 0.4f);

    setPivotPositionMM(&title_x, &title_y, 1);
    (void)SDLTextWriteScaleMM(
        15, 255, 2, (int)title_x, (int)title_y,
        2.5f, 0, L"%ls", allText[190]);

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

    setPivotPositionMM(&name_x, &name_y, 7);
    (void)SDLTextWriteScaleMM(
        15, 255, 0, (int)name_x, (int)name_y,
        2.5f, 0, L"%ls", allText[text_index]);

    menu_drawLevelSelectControlsIcon();
    menu_fadeBG();
    gPSXDrawScaleX = 1.0f;
    gPSXDrawScaleY = 1.0f;
}

/* 0xC4750, 1697 bytes, global, 17 named locals
 * menu_drawLevelSelectScreen_OLD
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC4E00, 1716 bytes, global, 19 named locals
 * menu_drawPlayerSelect
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC54C0, 457 bytes, global, 2 named locals
 * menu_drawReconnect
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC5690, 364 bytes, global, 1 named locals
 * menu_drawScoreMovers
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC5800, 7640 bytes, global, 92 named locals
 * menu_drawScoreScreen
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC75E0, 1240 bytes, global, 13 named locals
 * menu_drawSelectBox
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_drawSelectBox(void)
{
    int valid_count = 0;

    if (menuVars.levelSelectSelectorOffset != 0) {
        if (menuVars.levelSelectBoxCountdown == 0) {
            menuVars.levelSelectBoxCountdown = 0x18;
        }
    } else if (menuVars.levelSelectBoxCountdown == 0) {
        /* no-op: retail falls through to animate/draw */
    }
    if (menuVars.levelSelectBoxCountdown != 0) {
        --menuVars.levelSelectBoxCountdown;
    }
    if (menuVars.levelSelectSelectorOffset != 0 ||
        menuVars.levelSelectBoxCountdown != 0) {
        menuVars.levelSelectBoxTop = 0x86;
        menuVars.levelSelectBoxWidth = 0x78;
        menuVars.levelSelectBoxHeight = 2;
        return;
    }

    if (menuVars.levelSelectBoxWidth < 0x78) {
        menuVars.levelSelectBoxWidth =
            (uint16_t)(menuVars.levelSelectBoxWidth + 0x0f);
        if (menuVars.levelSelectBoxWidth > 0x78) {
            menuVars.levelSelectBoxWidth = 0x78;
        }
    } else if (menuVars.levelSelectBoxHeight < 0x16) {
        --menuVars.levelSelectBoxTop;
        menuVars.levelSelectBoxHeight =
            (uint16_t)(menuVars.levelSelectBoxHeight + 2);
        if (menuVars.levelSelectBoxHeight >= 0x16 &&
            menuVars.levelSelectBoxOpen == 0) {
            menuVars.levelSelectBoxOpen = 1;
            menuVars.levelSelectPreviewFade = -256;
            menuVars.levelSelectPreviewLevel = (uint8_t)LevelSelect;
        }
    }

    (void)jedi_CheckValidLevel(
        (unsigned)menuVars.levelSelectPreviewLevel,
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
            373.0f - (float)menuVars.levelSelectBoxWidth;
        float box_width =
            (99.0f + (float)menuVars.levelSelectBoxWidth) *
            scaleAdjustmentMM;
        float box_height =
            (float)menuVars.levelSelectBoxHeight *
            scaleAdjustmentMM;
        float box_top = (float)menuVars.levelSelectBoxTop;

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
        373.0f, (float)(menuVars.levelSelectBoxTop - 4));
    menu_drawSelector(
        373.0f,
        (float)(menuVars.levelSelectBoxTop +
                (int)menuVars.levelSelectBoxHeight - 6));
    if (menuVars.levelSelectPreviewFade < 0) {
        menuVars.levelSelectPreviewFade =
            (int16_t)(menuVars.levelSelectPreviewFade + 0x10);
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
    int y = menuVars.levelSelectSelectorOffset;
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

    if (menuVars.levelSelectSelectorOffset < 0) {
        menuVars.levelSelectSelectorOffset += 4;
        if (menuVars.levelSelectSelectorOffset > 0) {
            menuVars.levelSelectSelectorOffset = 0;
        }
    } else if (menuVars.levelSelectSelectorOffset > 0) {
        menuVars.levelSelectSelectorOffset -= 4;
        if (menuVars.levelSelectSelectorOffset < 0) {
            menuVars.levelSelectSelectorOffset = 0;
        }
    }
}

/* 0xC7DE0, 365 bytes, global, 4 named locals
 * menu_dumpMemory
 * PDB type: void (unsigned, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

/* 0xC80F0, 770 bytes, global, 2 named locals
 * menu_enterPlayerCouncilMode
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC8400, 479 bytes, global, 2 named locals
 * menu_enterScoreMode
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

/* 0xC8740, 156 bytes, global, 5 named locals
 * menu_grayBars
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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
    LevelSelect = 1;
    menuVars.levelSelectSelectorOffset = 0;
    menuVars.levelSelectBoxTop = 0x86;
    menuVars.levelSelectBoxWidth = 0x78;
    menuVars.levelSelectBoxHeight = 2;
    menuVars.levelSelectBoxCountdown = 0;
    menuVars.levelSelectBoxOpen = 1;
    menuVars.levelSelectPreviewLevel = 1;
    menuVars.levelSelectPreviewFade = -256;
    menuVars.pSelect = 0;
    menuVars.mmSelect1[menuVars.menuModeSP & 7u] = 0;
    menuVars.mmSelect2[menuVars.menuModeSP & 7u] = 0;
}

/* 0xC9B20, 348 bytes, global, 4 named locals
 * menu_initLoadBar
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xC9C80, 2500 bytes, global, 5 named locals
 * menu_initNewMenu
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCA650, 49 bytes, global, 0 named locals
 * menu_initPlayerSelect
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_initPlayerSelect(void)
{
    menuVars.scoreDst = 0;
    memset(menuVars.pselectMode, 0, sizeof(uint32_t));
    memset(menuVars.pselectMode + 12, 0, sizeof(uint32_t));
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

/* 0xCA6A0, 1299 bytes, global, 10 named locals
 * menu_initScoreScreen
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCABC0, 3 bytes, global, 0 named locals
 * menu_initTitleLoad
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCABD0, 3 bytes, global, 1 named locals
 * menu_initialLoad
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

    if (mdef == NULL) {
        return;
    }
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

/* 0xCAE10, 3 bytes, global, 0 named locals
 * menu_loadGame
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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
    case 0x10:
        return GameStruct.gameMode == 6 ||
                       GameStruct.gameMode == 7
            ? audioMdef_Game
            : audioMdef;
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
    mode = menuVars.menuMode[menuVars.menuModeSP];
    if (mode == 4) {
        /* Exact state-four entry republishes the selected player count and
         * its gameplay global bits before drawing playerCountSelectMdef. */
        menu_setNumPlayers((unsigned)(uint8_t)GameStruct.NumPlayers);
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
        return;
    }
    if (mode == 0x0c) {
        if (newMenu_Training() < 0) {
            menu_menuExit();
        }
        return;
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
        return;
    }
    if (mode == 0x1a) {
        menu_drawLevelSelectScreen(1);
        return;
    }
    if (mode == 0x13) {
        unsigned player;

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
        return;
    }
    if (mode == 0x1b) {
        menuConceptMenu();
        return;
    }
    if (mode == 0x23) {
        if (p1Disconnected != 0 || p2Disconnected != 0) {
            menu_menuExit();
            return;
        }
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        runControlsMenu();
        menuVars.controlPlayer = 0;
        return;
    }
    if (mode == 0x24) {
        if (GameStruct.gameMode != 6 && GameStruct.gameMode != 7) {
            winDrawBackground(5);
        }
        runControlsMenu();
        return;
    }
    definition = menu_mainLoopDefinition(mode);
    if (definition == NULL) {
        return;
    }
    gColor.cd = 0;
    menu_mainMenu(definition);
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

/* 0xCD9F0, 644 bytes, local, 4 named locals
 * menu_nextMMV
 * PDB type: void (MMVDEF*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCDC80, 23 bytes, global, 1 named locals
 * menu_nukePrimo
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCDCA0, 3 bytes, global, 1 named locals
 * menu_overLifeBars
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

    feedback_startEffect(0, 14);
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
    if (menu_id == 0x13) {
        menu_initCredits();
    } else if (menu_id == 0x1b) {
        menuVars.scoreScore = 0;
    } else if (menu_id == 0x1a) {
        menu_initLevelSelectScreen();
    }
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
        unsigned resolution_index = mmGetModVal(&modVars[73]);
        unsigned window_mode = mmGetModVal(&modVars[72]);
        uint32_t width = OptionStruct.ScreenWidth;
        uint32_t height = OptionStruct.ScreenHeight;

        if (jpb_menu_platform_hooks.applyResolution != NULL) {
            jpb_menu_platform_hooks.applyResolution(
                resolution_index,
                window_mode,
                &width,
                &height,
                jpb_menu_platform_user_data);
        }
        OptionStruct.ScreenWidth = width;
        OptionStruct.ScreenHeight = height;
        OptionStruct.WindowMode = window_mode;
        OptionStruct.ResolutionChanged = resolution_index;
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

    /*
     * The matched menu keeps the previous sixteen non-Zoom-Out pad samples
     * for cheat recognition. memmove preserves the original overlapping
     * scalar shift.
     */
    if (menuVars.pad[0] != 0) {
        memmove(
            menuVars.frKeyBuff,
            menuVars.frKeyBuff + 1,
            15u * sizeof(menuVars.frKeyBuff[0]));
        menuVars.frKeyBuff[15] =
            (uint16_t)(menuVars.pad[0] & ~JPB_PAD_ZOOM_OUT);
    }
    if (menuVars.pad[1] != 0) {
        memmove(
            menuVars.frKeyBuff2,
            menuVars.frKeyBuff2 + 1,
            15u * sizeof(menuVars.frKeyBuff2[0]));
        menuVars.frKeyBuff2[15] =
            (uint16_t)(menuVars.pad[1] & ~JPB_PAD_ZOOM_OUT);
    }

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

/* 0xCE6F0, 197 bytes, global, 3 named locals
 * menu_screenSaver
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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
    uint8_t *award = &menuVars.awardSet[player * 36u];
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
        ++award[15];
        break;
    case 9:
        memcpy(&target, award + 20, sizeof(target));
        menuVars.pointSeek =
            (uint32_t)(target - (int32_t)menuVars.scoreScore);
        menuVars.scoreNextMode = 10;
        break;
    case 10:
        memcpy(&target, award + 24, sizeof(target));
        menuVars.pointSeek =
            (uint32_t)(target - (int32_t)menuVars.scoreScore);
        menuVars.scoreNextMode = 11;
        break;
    case 11:
        memcpy(&target, award + 28, sizeof(target));
        menuVars.pointSeek =
            (uint32_t)(target - (int32_t)menuVars.scoreScore);
        menuVars.scoreNextMode = 1;
        break;
    case 13:
        memcpy(&target, award + 20, sizeof(target));
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

/* 0xCECB0, 3 bytes, global, 0 named locals
 * menu_showGameMode
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCECC0, 3 bytes, global, 0 named locals
 * menu_showSaves
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCECD0, 18 bytes, global, 1 named locals
 * menu_showVRAMBackground
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCECF0, 573 bytes, global, 13 named locals
 * menu_slideco
 * PDB type: void (float, float, int, int, fl...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCEF30, 282 bytes, global, 9 named locals
 * menu_slideco_a
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xCF050, 42 bytes, global, 1 named locals
 * menu_sound
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_sound(unsigned sound)
{
    unsigned selected_sound = sound < 11u ? sound : 0u;

    if (selected_sound != 0 &&
        jpb_menu_platform_hooks.menuSound != NULL) {
        jpb_menu_platform_hooks.menuSound(
            selected_sound, jpb_menu_platform_user_data);
    }
}

/* 0xCF080, 246 bytes, global, 1 named locals
 * menu_specialMess
 * PDB type: void (unsigned char*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void menu_specialMess(uint8_t *mess)
{
    uint16_t response_menu =
        mess == (uint8_t *)(void *)allText[376]
            ? UINT16_C(0x2c)
            : UINT16_C(0x2b);

    GameStruct.GameState |=
        UINT32_C(0x02000000);
    GameStruct.inMenuFlag = 1;
    feedback_startEffect(0, 14);
    feedback_startEffect(0, 14);
    if (jpb_menu_special_message_hook != NULL) {
        jpb_menu_special_message_hook(
            mess,
            UINT16_C(0x41),
            response_menu,
            jpb_menu_special_message_user_data);
    }
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

    if (md == NULL || md[4] >= 74) {
        return;
    }
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
    unsigned stream_count = 0;
    int carousel_clip = 0;

    if (mdef == NULL) {
        return;
    }
    while (mdef[stream_index] != UINT32_C(0x14) &&
           stream_count < 4096) {
        uint32_t command = mdef[stream_index] & UINT32_C(0x7fff);
        unsigned width;

        if (command == UINT32_C(0x44)) {
            carousel_clip = 1;
            break;
        }
        if (command >= sizeof(mmsizes) / sizeof(mmsizes[0])) {
            break;
        }
        width = mmsizes[command];
        if (width == 0) {
            break;
        }
        stream_index += width;
        ++stream_count;
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
    selectp = menuVars.selectp != NULL
        ? menuVars.selectp
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
    if (sound != 0 &&
        jpb_menu_platform_hooks.menuSound != NULL) {
        jpb_menu_platform_hooks.menuSound(
            sound, jpb_menu_platform_user_data);
    }
}
void mmDraw(uint32_t *md)
{
    unsigned index = 0;
    unsigned instruction_count = 0;

    if (md == NULL) {
        return;
    }
    menuVars.mmAnchorType = 4;
    menuVars.selectp = menuVars.mmSelect1;
    menuVars.textScale = 2.25f;
    menuVars.textSpacer = 60.0f;
    while (md[index] != UINT32_C(0x14) &&
           instruction_count < 4096) {
        unsigned next = mmDrawsub(md, index);

        if (next <= index) {
            break;
        }
        index = next;
        ++instruction_count;
    }
}

/* 0xD0820, 3 bytes, global, 4 named locals
 * mmDrawCard
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xD0830, 1003 bytes, global, 9 named locals
 * mmDrawItem
 * PDB type: int (unsigned*, unsigned char*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int mmDrawItem(uint32_t *md, uint8_t *dstbuffer)
{
    wchar_t display[256];
    const wchar_t *source;
    unsigned selected;
    unsigned color;
    int is_selected;
    int is_active_selected;
    int draw_font_style;
    int y;

    if (md == NULL) {
        return 0;
    }
    selected = menuVars.selectp != NULL
        ? menuVars.selectp[menuVars.menuModeSP & 7u]
        : 0;
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
        source = (const wchar_t *)menuVars.specialString;
    } else if (md[2] < JPB_ALL_TEXT_CAPACITY) {
        source = allText[md[2]];
    } else {
        source = NULL;
    }
    if (source == NULL) {
        source = L"";
    }
    if (dstbuffer != NULL) {
        size_t index = 0;
        size_t output = 0;

        if (is_active_selected) {
            dstbuffer[output++] = '>';
            dstbuffer[output++] = ' ';
        }
        while (source[index] != L'\0' && output < 254) {
            wchar_t value = source[index++];
            dstbuffer[output++] =
                (uint8_t)(value <= 0x7f ? value : '?');
        }
        if (output < 255) {
            dstbuffer[output++] = ' ';
        }
        dstbuffer[output] = 0;
        return (int)output;
    }
    if (is_active_selected) {
        (void)swprintf(
            display,
            sizeof(display) / sizeof(display[0]),
            L"> %ls <",
            source);
    } else {
        (void)swprintf(
            display,
            sizeof(display) / sizeof(display[0]),
            L"%ls",
            source);
    }
    y = (int32_t)menuVars.mmY;
    if (menuVars.yflag == 1) {
        y += menuVars.yoffset;
    }
    draw_font_style = menuVars.mmItalics != 0 ? 1 : 0;
    if ((md[2] == 239u || md[2] == 241u) &&
        menuVars.mmTextType <= 1u) {
        _Material *textures[10] = {0};
        _Material *glyph;
        const wchar_t *label = source;
        SCREENRECT destination;
        CVECTOR white = {255, 255, 255, 255};
        int icon_size = (int)(64.0f * scaleAdjustmentMM);
        int gap = (int)(20.0f * scaleAdjustmentMM);
        int text_x = (int32_t)menuVars.mmX;

        while (*label != L'\0' &&
               (*label > 0x7f || *label == L' ')) {
            ++label;
        }
        (void)getControllerTextures(0, textures);
        glyph = md[2] == 241u
            ? textures[lastUsedInputType == 0 ? 2 : 3]
            : textures[lastUsedInputType == 0 ? 9 : 2];
        if (glyph != NULL && icon_size > 0) {
            if (menuVars.mmTextType == 0) {
                destination.left = text_x;
                destination.right = text_x + icon_size;
                text_x += icon_size + gap;
            } else {
                destination.left = text_x -
                    (int)(210.0f * scaleAdjustmentMM);
                destination.right = destination.left + icon_size;
            }
            destination.top = y;
            destination.bottom = y + icon_size;
            _DrawTexture(glyph, destination, NULL, white, 0.0f);
        }
        return SDLTextWriteScaleMM(
            (int)color,
            255,
            (int)menuVars.mmTextType,
            text_x,
            y,
            menuVars.textScale,
            draw_font_style,
            L"%ls",
            label);
    }
    return SDLTextWriteScaleMM(
        (int)color,
        255,
        (int)menuVars.mmTextType,
        (int32_t)menuVars.mmX,
        y,
        menuVars.textScale,
        draw_font_style,
        L"%ls",
        display);
}

/* 0xD0C20, 3 bytes, global, 2 named locals
 * mmDrawMisc
 * PDB type: void (MDEF_MOD*, unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xD0C30, 859 bytes, global, 11 named locals
 * mmDrawMod
 * PDB type: void (unsigned*, unsigned char*)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
void mmDrawMod(uint32_t *md, uint8_t *dstbuffer)
{
    wchar_t prefix[256];
    wchar_t display[512];
    const wchar_t *value_text = NULL;
    MDEF_MOD *mod;
    unsigned mod_index;
    unsigned value;
    unsigned color;
    unsigned selected;
    int active;
    size_t index;
    int y;

    if (md == NULL || dstbuffer == NULL || md[4] >= 74) {
        return;
    }
    mod_index = md[4];
    mod = &modVars[mod_index];
    value = mmGetModVal(mod);
    selected = menuVars.selectp != NULL
        ? menuVars.selectp[menuVars.menuModeSP & 7u]
        : 0;
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
        prefix[index] = (wchar_t)dstbuffer[index];
    }
    prefix[index] = L'\0';

    if ((mod->type & UINT16_C(0x8000)) != 0) {
        unsigned text_base = mod->text & UINT16_C(0x7fff);
        unsigned text_index = text_base + value;

        if (mod->text == 2) {
            return;
        }
        if (text_index < JPB_ALL_TEXT_CAPACITY) {
            value_text = allText[text_index];
        }
        if (value_text == NULL) {
            value_text = L"";
        }
        (void)swprintf(
            display,
            sizeof(display) / sizeof(display[0]),
            L"%ls%ls%ls",
            prefix,
            value_text,
            active ? L" <" : L"");
    } else {
        if ((mod->type & UINT16_C(0x4000)) != 0) {
            return;
        }
        (void)swprintf(
            display,
            sizeof(display) / sizeof(display[0]),
            L"%ls%u%ls",
            prefix,
            value,
            active ? L" <" : L"");
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
        L"%ls",
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

    if (md == NULL) {
        return index;
    }
    command = md[index];
    selected = menuVars.selectp != NULL
        ? menuVars.selectp[menuVars.menuModeSP & 7u]
        : 0;
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
        menuVars.mmFlags = menuVars.mmvCurrentMenuControl != NULL
            ? menuVars.mmvCurrentMenuControl->mmvMenuFlags
            : 0;
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
        if (menuVars.mmvCurrentMenuControl != NULL) {
            menuVars.mmX = (uint32_t)
                menuVars.mmvCurrentMenuControl->mmvX;
            menuVars.mmY = (uint32_t)
                menuVars.mmvCurrentMenuControl->mmvY;
        }
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
    if ((command & 0x7fffu) >=
        sizeof(mmsizes) / sizeof(mmsizes[0])) {
        return index;
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
    if (mod == NULL) {
        return 0;
    }
    switch (mod->type & UINT16_C(0x7f)) {
    case 0:
    case 1:
        return mod->src != NULL
            ? (unsigned)*(const uint8_t *)mod->src
            : 0;
    case 2:
    case 3:
        return mod->src != NULL
            ? (unsigned)*(const uint16_t *)mod->src
            : 0;
    case 4:
    case 5:
    case 6:
        return mod->src != NULL
            ? *(const uint32_t *)mod->src
            : 0;
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

    if (md == NULL || md[4] >= 74) {
        return;
    }
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

    if (md == NULL) {
        return index;
    }
    command = md[index] & 0x7fffu;
    if (command >= sizeof(mmsizes) / sizeof(mmsizes[0])) {
        return index;
    }
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

    if (mod == NULL) {
        return 0;
    }
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
        if (mod->src != NULL) {
            *(uint16_t *)mod->src = (uint16_t)value;
        }
        break;
    case 4:
    case 5:
    case 6:
        if (mod->src != NULL) {
            *(uint32_t *)mod->src = value;
        }
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

    if (md == NULL || md[4] >= 74) {
        return;
    }
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

/* 0xD2210, 3 bytes, global, 1 named locals
 * mmvRunScore
 * PDB type: int (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

    if (material == NULL) {
        return;
    }
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

static const wchar_t *newMenu_Text(unsigned index)
{
    if (index >= JPB_ALL_TEXT_CAPACITY || allText[index] == NULL) {
        return L"";
    }
    return allText[index];
}

static int newMenu_AdjacentModel(
    int model, int direction, int select_type)
{
    do {
        model += direction;
        if (model < 0) {
            model = jar_jar_playable_model;
        } else if (model > jar_jar_playable_model) {
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
            2.25f, 0, L"%ls", newMenu_Text(491));
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
            1.75f, 0, L"%ls", newMenu_Text(484));
        x = 300.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 4);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP1 == 1 ? 11 : 9,
            255, 1, (int)x, (int)y,
            1.75f, 0, L"%ls", newMenu_Text(485));
    }

    newMenu_DrawMaterialRect(
        183, -230.0f, -278.0f, 229.0f, 278.0f,
        4, UINT32_C(0xffffffff), 0.0f);
    if (converted_model >= 0) {
        newMenu_DrawMaterialRect(
            201 + converted_model,
            -196.0f, -262.0f, 195.0f, 261.0f,
            4, UINT32_C(0xffffffff), 0.1f);
    }
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
    if (previous_converted >= 0) {
        newMenu_DrawMaterialRect(
            201 + previous_converted,
            -713.0f, -188.0f, -400.19998f, 230.4f,
            4, UINT32_C(0x6effffff), 0.1f);
    }
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
    if (next_converted >= 0) {
        newMenu_DrawMaterialRect(
            201 + next_converted,
            399.4f, -188.0f, 713.0f, 231.20001f,
            4, UINT32_C(0x6effffff), 0.1f);
    }
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
        name_scale, 0, L"%ls",
        converted_model >= 0
            ? newMenu_Text(332u + (unsigned)converted_model)
            : L"");
    x = 0.0f;
    y = 11.0f;
    setPivotPositionMM(&x, &y, 1);
    (void)SDLTextWriteScaleMM(
        11, 255, 2, (int)x, (int)y,
        2.25f, 0, L"%ls", newMenu_Text(481));

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
            1.5f, 0, L"%ls", newMenu_Text(479));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, L"%ls %02d",
            newMenu_Text(224), highest_level);
    }
    x = -60.0f;
    y = 192.5f;
    setPivotPositionMM(&x, &y, 7);
    if (!jedi_HasProgression((model_id)model)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, L"%ls", newMenu_Text(480));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, L"%ls %03d",
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
            2.25f, 0, L"%ls", newMenu_Text(491));
    }
    if (jedi_CanToggleSaber((model_id)player_two)) {
        player2IconOverride = 1;
        x = 480.0f;
        y = 100.0f;
        setPivotPositionMM(&x, &y, 8);
        (void)SDLTextWriteScaleMM(
            15, 255, 2, (int)x, (int)y,
            2.25f, 0, L"%ls", newMenu_Text(491));
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
        2.25f, 0, L"%ls", newMenu_Text(481));

    if (GameStruct.gameCompleted != 0 || is_versus != 0) {
        newMenu_DrawControllerTabs(
            0, 170.0f, 770.0f, 3, 0);
        x = 170.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 3);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP1 == 0 ? 11 : 9,
            255, 0, (int)x, (int)y,
            1.75f, 0, L"%ls", newMenu_Text(484));
        x = 770.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 3);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP1 == 1 ? 11 : 9,
            255, 1, (int)x, (int)y,
            1.75f, 0, L"%ls", newMenu_Text(485));
    }

    newMenu_DrawMaterialRect(
        183, 240.5f, -278.0f, 699.5f, 278.0f,
        3, UINT32_C(0xffffffff), 0.0f);
    if (converted_one >= 0) {
        newMenu_DrawMaterialRect(
            201 + converted_one,
            274.0f, -262.0f, 665.0f, 261.0f,
            3, UINT32_C(0xffffffff), 0.1f);
    }
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
        name_scale, 0, L"%ls",
        converted_one >= 0
            ? newMenu_Text(332u + (unsigned)converted_one)
            : L"");
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
            1.5f, 0, L"%ls", newMenu_Text(479));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, L"%ls %02d",
            newMenu_Text(224), highest_level);
    }
    x = 410.0f;
    y = 192.5f;
    setPivotPositionMM(&x, &y, 6);
    if (!jedi_HasProgression((model_id)player_one)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, L"%ls", newMenu_Text(480));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, L"%ls %03d",
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
        2.25f, 0, L"%ls", newMenu_Text(482));

    if (GameStruct.gameCompleted != 0 || is_versus != 0) {
        newMenu_DrawControllerTabs(
            (int)(uint8_t)GameStruct.NumPlayers - 1,
            170.0f,
            770.0f,
            5,
            1);
        x = 770.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 5);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP2 == 0 ? 11 : 9,
            255, 0, (int)x, (int)y,
            1.75f, 0, L"%ls", newMenu_Text(484));
        x = 170.0f;
        y = -427.0f;
        setPivotPositionMM(&x, &y, 5);
        (void)SDLTextWriteScaleMM(
            newMenu_playerSelectTypeP2 == 1 ? 11 : 9,
            255, 1, (int)x, (int)y,
            1.75f, 0, L"%ls", newMenu_Text(485));
    }

    newMenu_DrawMaterialRect(
        183, 699.5f, -278.0f, 240.5f, 278.0f,
        5, UINT32_C(0xffffffff), 0.0f);
    if (converted_two >= 0) {
        newMenu_DrawMaterialRect(
            201 + converted_two,
            665.0f, -262.0f, 274.0f, 261.0f,
            5, UINT32_C(0xffffffff), 0.1f);
    }
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
        name_scale, 0, L"%ls",
        converted_two >= 0
            ? newMenu_Text(332u + (unsigned)converted_two)
            : L"");
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
            1.5f, 0, L"%ls", newMenu_Text(479));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, L"%ls %02d",
            newMenu_Text(224), highest_level);
    }
    x = 530.0f;
    y = 192.5f;
    setPivotPositionMM(&x, &y, 8);
    if (!jedi_HasProgression((model_id)player_two)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, L"%ls", newMenu_Text(480));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)x, (int)y,
            1.5f, 0, L"%ls %03d",
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
            2.25f, 0, L"%ls", newMenu_Text(491));
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
        2.25f, 0, L"%ls", newMenu_Text(481));

    newMenu_DrawMaterialRect(
        183, 240.5f, -278.0f, 699.5f, 278.0f,
        3, UINT32_C(0xffffffff), 0.0f);
    if (converted_model >= 0) {
        newMenu_DrawMaterialRect(
            201 + converted_model,
            274.0f, -262.0f, 665.0f, 261.0f,
            3, UINT32_C(0xffffffff), 0.1f);
    }
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
        1.75f, 0, L"%ls",
        converted_model >= 0
            ? newMenu_Text(332u + (unsigned)converted_model)
            : L"");

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
            1.5f, 0, L"%ls", newMenu_Text(479));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)left_x, (int)left_y,
            1.5f, 0, L"%ls %02d",
            newMenu_Text(224), highest_level);
    }
    left_x = 410.0f;
    left_y = 192.5f;
    setPivotPositionMM(&left_x, &left_y, 6);
    if (!jedi_HasProgression((model_id)model)) {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)left_x, (int)left_y,
            1.5f, 0, L"%ls", newMenu_Text(480));
    } else {
        (void)SDLTextWriteScaleMM(
            11, 255, 0, (int)left_x, (int)left_y,
            1.5f, 0, L"%ls %03d",
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
        2.25f, 0, L"%ls", newMenu_Text(483));
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
        1.75f, 0, L"%ls",
        newMenu_Text(320u + (unsigned)newMenu_trainLevel));
}

/* 0xD5DE0, 2311 bytes, local, 28 named locals
 * newMenu_DrawVSMode
 * PDB type: void (int, int, unsigned long, u...
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xD66F0, 438 bytes, global, 5 named locals
 * newMenu_GetVSExtraPlayer
 * PDB type: int (char**, int*, int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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
        /* The exact reconnect presentation is a separate pending owner. */
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
        /* The exact reconnect presentation is a separate pending owner. */
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

/* 0xD7750, 52 bytes, global, 1 named locals
 * newMenu_SelectPlayers
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xD7790, 1024 bytes, global, 0 named locals
 * newMenu_Training
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\menu.c
 */
int newMenu_Training(void)
{
    uint32_t pad = menuVars.pad[0];

    if (p1Disconnected != 0) {
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
                if (GameStruct.ModelSelect[0] > battle_d_model) {
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
                    GameStruct.ModelSelect[0] = battle_d_model;
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

/* 0xD7FB0, 569 bytes, global, 7 named locals
 * redlineFunc
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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

    if (material == NULL) {
        return;
    }
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
    return material != NULL
        ? center_x - (float)material->iw * 0.15f
        : center_x;
}

static void menu_drawControlsText(
    unsigned text_index,
    float x,
    float y,
    int mode,
    float scale)
{
    if (text_index >= JPB_ALL_TEXT_CAPACITY ||
        allText[text_index] == NULL) {
        return;
    }
    setPivotPositionMM(&x, &y, 4);
    (void)SDLTextWriteScaleMM(
        11, 255, mode, (int)x, (int)y,
        scale, 0, L"%ls", allText[text_index]);
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
                    (force != NULL ? (float)force->iw * 0.15f : 0.0f) +
                    scaleAdjustmentMM * 12.0f;
                plus_y = y + scaleAdjustmentMM * 8.0f;
                setPivotPositionMM(&plus_x, &plus_y, 4);
                (void)SDLTextWriteScaleMM(
                    11, 255, 0, (int)plus_x, (int)plus_y,
                    1.75f, 0, L"+");
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
        1.75f, 0, L"%ls", newMenu_Text(242));
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

/* 0xD9580, 3 bytes, global, 0 named locals
 * runSaveMenu
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xD9590, 45 bytes, global, 3 named locals
 * scoreloadart
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xD95C0, 244 bytes, global, 6 named locals
 * testcombo
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

/* 0xD96C0, 3 bytes, global, 0 named locals
 * turnOffBackground
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\menu.c
 */

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
