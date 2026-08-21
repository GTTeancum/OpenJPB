/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\game.c.
 *
 * Provenance:
 *   direct     - names/signatures/locals and CharacterData, JEDICOMBOMASK,
 *                and gamestruct layouts from the exact matched PDB.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - clamps, signed field reads, player strides, level-eight
 *                exception path, percentage arithmetic, stores, and return
 *                values checked at the exact RVAs below; global-bit byte
 *                addressing is checked at 0xA7610, 0xA8760, and 0xA9070.
 *
 * PDB module: 0039
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\game.obj
 * Primary source: W:\SWJediPowerBattles\Work\game.c
 * Compiler language: c
 * Emitted procedures: 84
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/game.h"
#include "jpb/ai.h"
#include "jpb/audio_stream.h"
#include "jpb/boss.h"
#include "jpb/brain.h"
#include "jpb/braindmg.h"
#include "jpb/brainutl.h"
#include "jpb/camera.h"
#include "jpb/combo.h"
#include "jpb/cube.h"
#include "jpb/debugtext.h"
#include "jpb/extracharacters.h"
#include "jpb/force.h"
#include "jpb/generic_hook.h"
#include "jpb/jedi.h"
#include "jpb/jonny.h"
#include "jpb/menu.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/pwrup.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/text.h"
#include "jpb/vehicle.h"
#include "jpb/world.h"

#include <string.h>

/* Exact matched-PC globals at RVAs 0x537E00..0x537F0F and 0x539EC8. */
int32_t numPlayers;
float currentTextAlpha;
float currentSpriteAlpha;
int32_t spriteBright;
int64_t textBright;
int32_t waitForInputReadAfterRefresh;
int32_t refreshHUDCounter;

static int32_t glow;
static SCB *itemScb[2];
static SCB *rescueScb;
static int32_t deadline;
static SCB *creditScb[16];
static SCB *scoreScb[2];
static int32_t b[16];

/* Exact initialized PDB global charStuff at RVA 0x4BABD0. */
uint16_t charStuff[10] = {
    45, 46, 45, 48, 47, 45, 45, 45, 48, 46
};

/* Exact initialized PDB global at matched-PC RVA 0x4BD9B0. */
int8_t aLevelXATracks[26] = {
    1, 7, 9, 14, 18, 21, 26, 31, 33, 35, 38, 18, 92,
    10, 13, 27, 35, 33, 7, 13, 35, 33, 7, 13, 13, 38
};

static void game_hide_overlay_scb(SCB *scb)
{
    if (scb != NULL && (scb->scb_flags & 0x40) == 0) {
        scb->scb_flags |= 0x40;
    }
}

/* Exact PDB global at matched-PC RVA 0x10EFB80. */
JPBPlayerCallback
    funcArray[JPB_PLAYER_CALLBACK_CAPACITY];

static JPBGameBarHook jpb_game_bar_hook;
static void *jpb_game_bar_user_data;

void jpb_GameSetBarHook(
    JPBGameBarHook hook, void *user_data)
{
    jpb_game_bar_hook = hook;
    jpb_game_bar_user_data = user_data;
}

void jpb_GameResetOverlayScbs(void)
{
    memset(itemScb, 0, sizeof(itemScb));
    rescueScb = NULL;
    memset(creditScb, 0, sizeof(creditScb));
    memset(scoreScb, 0, sizeof(scoreScb));
}

/*
 * Exact initialized bytes from PDB globals aNormalDifficulty
 * (matched-PC RVA 0x4BAA80) and aEasyDifficulty (RVA 0x4BAB10).
 * game_initPerLevel addresses these as sixteen five-byte rows and copies the
 * selected row to gamestruct fields AIDamage through BlockRate.
 */
const int8_t aNormalDifficulty[16][5] = {
    {8, 8, 8, 8, 8},
    {6, 9, 11, 10, 10},
    {8, 8, 7, 8, 7},
    {8, 8, 8, 8, 8},
    {7, 10, 8, 9, 10},
    {8, 8, 5, 6, 8},
    {7, 9, 9, 11, 7},
    {7, 8, 9, 11, 6},
    {8, 8, 8, 8, 8},
    {8, 8, 8, 8, 8},
    {8, 8, 8, 8, 8},
    {8, 8, 8, 8, 8},
    {8, 8, 8, 8, 8},
    {8, 8, 8, 8, 8},
    {8, 8, 8, 8, 8},
    {7, 9, 9, 11, 7}
};

const int8_t aEasyDifficulty[16][5] = {
    {6, 10, 10, 8, 8},
    {4, 11, 13, 10, 10},
    {6, 10, 9, 8, 7},
    {6, 10, 10, 8, 8},
    {6, 10, 10, 8, 8},
    {6, 10, 7, 6, 8},
    {5, 11, 11, 11, 7},
    {5, 10, 11, 11, 6},
    {6, 10, 10, 8, 8},
    {6, 10, 10, 8, 8},
    {6, 10, 10, 8, 8},
    {6, 10, 10, 8, 8},
    {6, 10, 10, 8, 8},
    {6, 10, 10, 8, 8},
    {6, 10, 10, 8, 8},
    {5, 11, 11, 11, 7}
};

/*
 * Exact initialized PDB globals at matched-PC RVAs 0x4BAA78 and
 * 0x4BABE8..0x4BAC3F. Each list terminates with -1 and supplies the default
 * combo bits for one of the first nine Jedi/model slots.
 */
int8_t maceInitCombos[] = {0, 1, 2, 4, 12, -1};
int8_t obiwanInitCombos[] = {0, 1, 2, 4, 5, 6, 9, 10, 11, -1};
int8_t quigonInitCombos[] = {0, 1, 2, 3, 4, 6, 8, 11, -1};
int8_t adiInitCombos[] = {0, 1, 2, 3, 4, 5, 9, 10, 11, 12, -1};
int8_t ploInitCombos[] = {0, 1, 2, 4, 6, -1};
int8_t maulInitCombos[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, -1
};
int8_t amidalaInitCombos[] = {0, 1, 2, 3, 4, 5, 6, -1};
int8_t kiadiInitCombos[] = {0, 1, 2, 3, 4, 5, 8, -1};

/* Exact PDB pointer table initialJediCombos at RVA 0x4BAC40. */
int8_t *initialJediCombos[9] = {
    obiwanInitCombos,
    quigonInitCombos,
    maceInitCombos,
    adiInitCombos,
    ploInitCombos,
    maulInitCombos,
    amidalaInitCombos,
    amidalaInitCombos,
    kiadiInitCombos
};

int jpb_game_ApplyLevelDifficulty(
    unsigned level, int difficulty)
{
    const int8_t (*table)[5];
    const int8_t *row;

    if (difficulty == 0) {
        table = aEasyDifficulty;
    } else if (difficulty == 1) {
        table = aNormalDifficulty;
    } else {
        return 0;
    }
    if (level > 15u) {
        level = 0;
    }
    row = table[level];
    GameStruct.AIDamage = (char)row[0];
    GameStruct.JediDamage = (char)row[1];
    GameStruct.HTHRate = (char)row[2];
    GameStruct.RangedRate = (char)row[3];
    GameStruct.BlockRate = (char)row[4];
    GameStruct.difficulty = (char)difficulty;
    return 1;
}

/* 0xA5620, 2533 bytes, global, 2 named locals
 * ApplySaveGameData
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

void ApplySaveGameData(void)
{
    size_t index;

    memcpy(GameStruct.maxEnergyLevels, SaveGameStruct.maxEnergyLevels,
           sizeof(GameStruct.maxEnergyLevels));
    memcpy(GameStruct.maxEnergyLineLength,
           SaveGameStruct.maxEnergyLineLength,
           sizeof(GameStruct.maxEnergyLineLength));
    memcpy(GameStruct.maxForceLevels, SaveGameStruct.maxForceLevels,
           sizeof(GameStruct.maxForceLevels));
    memcpy(GameStruct.maxForceLineLength,
           SaveGameStruct.maxForceLineLength,
           sizeof(GameStruct.maxForceLineLength));
    memcpy(GameStruct.jediLevelPlayed, SaveGameStruct.jediLevelPlayed,
           sizeof(GameStruct.jediLevelPlayed));
    memcpy(GameStruct.jediScorePerLevel, SaveGameStruct.jediScorePerLevel,
           sizeof(GameStruct.jediScorePerLevel));
    memcpy(GameStruct.checkpoint, SaveGameStruct.checkpoint,
           sizeof(GameStruct.checkpoint));
    GameStruct.mNumContinues = SaveGameStruct.mNumContinues;
    GameStruct.ContinuesUsed = SaveGameStruct.ContinuesUsed;
    memcpy(GameStruct.aCharacterData, SaveGameStruct.aCharacterData,
           sizeof(GameStruct.aCharacterData));
    memcpy(GameStruct.jediComboMask, SaveGameStruct.jediComboMask,
           sizeof(GameStruct.jediComboMask));
    GameStruct.NumPlayers = 1;
    GameStruct.AIDamage = SaveGameStruct.AIDamage;
    GameStruct.JediDamage = SaveGameStruct.JediDamage;
    GameStruct.HTHRate = SaveGameStruct.HTHRate;
    GameStruct.RangedRate = SaveGameStruct.RangedRate;
    GameStruct.BlockRate = SaveGameStruct.BlockRate;
    GameStruct.ComboLevel = SaveGameStruct.ComboLevel;
    GameStruct.ForceLevel = SaveGameStruct.ForceLevel;
    GameStruct.continueAble = SaveGameStruct.continueAble;
    GameStruct.difficulty = SaveGameStruct.difficulty;
    GameStruct.gameCompleted = SaveGameStruct.gameCompleted;
    memcpy(abGlobalBits, SaveGameStruct.abGlobalBits,
           sizeof(abGlobalBits));
    memcpy(jediUpgrades, SaveGameStruct.jediUpgrades,
           sizeof(jediUpgrades));
    for (index = 0; index < ExtraCharactersSize; ++index) {
        ExtraCharacters[index].Unlocked =
            (SaveGameStruct.unlockedExtraCharacters &
             (UINT16_C(1) << index)) != 0;
    }
}

/* 0xA6010, 74 bytes, global, 0 named locals
 * ForceClearPlayerCPad
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA6060, 42 bytes, global, 1 named locals
 * InitGameResolution
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA6090, 289 bytes, global, 4 named locals
 * LoadPlayerPos
 * PDB type: void (char*)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA61C0, 299 bytes, global, 3 named locals
 * LoadSettingsData
 * PDB type: void (optionstruct)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA62F0, 141 bytes, global, 3 named locals
 * SavePlayerPos
 * PDB type: void (char*)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA6380, 242 bytes, global, 4 named locals
 * UpdateBright
 * PDB type: void (float, float)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

void UpdateBright(
    float targetTextBright,
    float targetSpriteBright)
{
    float next;

    if (currentTextAlpha == targetTextBright &&
        currentSpriteAlpha == targetSpriteBright) {
        return;
    }
    if (targetTextBright <= currentTextAlpha) {
        next = currentTextAlpha;
        if (targetTextBright < currentTextAlpha) {
            next = targetTextBright;
            if (currentTextAlpha - targetTextBright >= 16.25f) {
                next = currentTextAlpha - 16.25f;
            }
        }
    } else {
        next = targetTextBright;
        if (targetTextBright - currentTextAlpha >= 1.7333333f) {
            next = currentTextAlpha + 1.7333333f;
        }
    }
    currentTextAlpha = next;

    if (targetSpriteBright <= currentSpriteAlpha) {
        next = currentSpriteAlpha;
        if (targetSpriteBright < currentSpriteAlpha) {
            next = targetSpriteBright;
            if (currentSpriteAlpha - targetSpriteBright >= 25.0f) {
                next = currentSpriteAlpha - 25.0f;
            }
        }
    } else {
        next = targetSpriteBright;
        if (targetSpriteBright - currentSpriteAlpha >= 2.6666667f) {
            next = currentSpriteAlpha + 2.6666667f;
        }
    }
    currentSpriteAlpha = next;
    textBright = (int32_t)currentTextAlpha;
    spriteBright = (int32_t)currentSpriteAlpha;
}

/* 0xA6480, 2516 bytes, global, 3 named locals
 * UpdateSaveGameStruct
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

void UpdateSaveGameStruct(void)
{
    size_t index;
    uint16_t unlocked = 0;

    memcpy(SaveGameStruct.maxEnergyLevels, GameStruct.maxEnergyLevels,
           sizeof(SaveGameStruct.maxEnergyLevels));
    memcpy(SaveGameStruct.maxEnergyLineLength,
           GameStruct.maxEnergyLineLength,
           sizeof(SaveGameStruct.maxEnergyLineLength));
    memcpy(SaveGameStruct.maxForceLevels, GameStruct.maxForceLevels,
           sizeof(SaveGameStruct.maxForceLevels));
    memcpy(SaveGameStruct.maxForceLineLength,
           GameStruct.maxForceLineLength,
           sizeof(SaveGameStruct.maxForceLineLength));
    memcpy(SaveGameStruct.jediLevelPlayed, GameStruct.jediLevelPlayed,
           sizeof(SaveGameStruct.jediLevelPlayed));
    memcpy(SaveGameStruct.jediScorePerLevel, GameStruct.jediScorePerLevel,
           sizeof(SaveGameStruct.jediScorePerLevel));
    memcpy(SaveGameStruct.checkpoint, GameStruct.checkpoint,
           sizeof(SaveGameStruct.checkpoint));
    SaveGameStruct.NumPlayers = GameStruct.NumPlayers;
    SaveGameStruct.mNumContinues = GameStruct.mNumContinues;
    SaveGameStruct.ContinuesUsed = GameStruct.ContinuesUsed;
    memcpy(SaveGameStruct.aCharacterData, GameStruct.aCharacterData,
           sizeof(SaveGameStruct.aCharacterData));
    memcpy(SaveGameStruct.jediComboMask, GameStruct.jediComboMask,
           sizeof(SaveGameStruct.jediComboMask));
    SaveGameStruct.AIDamage = GameStruct.AIDamage;
    SaveGameStruct.JediDamage = GameStruct.JediDamage;
    SaveGameStruct.HTHRate = GameStruct.HTHRate;
    SaveGameStruct.RangedRate = GameStruct.RangedRate;
    SaveGameStruct.BlockRate = GameStruct.BlockRate;
    SaveGameStruct.ComboLevel = GameStruct.ComboLevel;
    SaveGameStruct.ForceLevel = GameStruct.ForceLevel;
    SaveGameStruct.continueAble = GameStruct.continueAble;
    SaveGameStruct.difficulty = GameStruct.difficulty;
    SaveGameStruct.gameCompleted = GameStruct.gameCompleted;
    memcpy(SaveGameStruct.abGlobalBits, abGlobalBits,
           sizeof(SaveGameStruct.abGlobalBits));
    memcpy(SaveGameStruct.jediUpgrades, jediUpgrades,
           sizeof(SaveGameStruct.jediUpgrades));
    for (index = 0; index < ExtraCharactersSize; ++index) {
        if (ExtraCharacters[index].Unlocked != 0) {
            unlocked |= (uint16_t)(UINT16_C(1) << index);
        }
    }
    SaveGameStruct.unlockedExtraCharacters = unlocked;
}

/* 0xA6E60, 112 bytes, global, 6 named locals
 * _AddBar
 * PDB type: void (int, int, int, int, long)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void _AddBar(
    int x, int y, int width, int height, int32_t color)
{
    /*
     * The matched body forwards these values to _DrawTile2D after forcing
     * alpha 0x7f. A platform renderer can consume the same authored solid
     * rectangle without adding a graphics dependency to gameplay code.
     */
    if (jpb_game_bar_hook != NULL) {
        jpb_game_bar_hook(
            jpb_game_bar_user_data,
            x,
            y,
            width,
            height,
            (uint32_t)color | UINT32_C(0x7f000000));
    }
}

/* 0xA6ED0, 561 bytes, global, 13 named locals
 * _AddLifeTile2D
 * PDB type: void (playerObject*, unsigned, u...
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void _AddLifeTile2D(
    playerObject *player,
    unsigned x,
    unsigned y,
    int alpha)
{
    float level;
    float level2 = 0.0f;
    float defLevel;
    float defLevel2;
    float x1 = (float)x;
    float x2 = (float)x;
    FVECTOR pos = {0.0f, 0.0f, 0.0f};
    int playernum;

    if (player->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(
            &player->playerRoot, 0, UINT32_C(0x20)) != 0 ||
        (player->pFlags & UINT32_C(0x40200)) != 0 ||
        LevelSelect == 8 ||
        GameStruct.CurrentLevel == 0) {
        return;
    }

    playernum = player->playernum;
    level = (float)game_gGetEnergy(playernum);
    defLevel = (float)game_gGetMaxEnergy(playernum);
    defLevel2 = (float)game_gGetScaleMaxForce(playernum);
    if (defLevel != 0.0f) {
        level = level / defLevel * 158.0f;
    } else {
        level = 0.0f;
    }
    if (defLevel2 != 0.0f) {
        level2 =
            (float)game_gGetScaleForce(playernum) /
            defLevel2 * 158.0f;
    }

    if (player->playerRoot.objectID != 0) {
        x1 = (float)x - level * 2.0f * scaleAdjustment;
        x2 = (float)x - level2 * 2.0f * scaleAdjustment;
    }

    pos.vx = x1;
    pos.vy = (float)y;
    _DrawTile2D(
        &pos,
        level * 2.0f * scaleAdjustment,
        scaleAdjustment * 12.0f + 1.0f,
        ((uint32_t)alpha << 24) | UINT32_C(0x0010fc10),
        0);

    pos.vx = x2;
    pos.vy = (float)y + scaleAdjustment * 24.0f;
    _DrawTile2D(
        &pos,
        level2 * 2.0f * scaleAdjustment,
        scaleAdjustment * 12.0f + 1.0f,
        ((uint32_t)alpha << 24) | UINT32_C(0x000000ff),
        0);
}

/* 0xA7110, 3 bytes, global, 0 named locals
 * checkResetAbort
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA7120, 71 bytes, global, 5 named locals
 * console_DecCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA7170, 71 bytes, global, 5 named locals
 * console_IncCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA71C0, 384 bytes, global, 7 named locals
 * console_LoadPlayerPosCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA7340, 201 bytes, global, 6 named locals
 * console_SavePlayerPosCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA7410, 257 bytes, global, 6 named locals
 * console_SetCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA7520, 106 bytes, global, 6 named locals
 * console_ToggleCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA7590, 3 bytes, global, 0 named locals
 * continueGameGameInit
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA75A0, 109 bytes, global, 2 named locals
 * findvar
 * PDB type: int (char*)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA7610, 28 bytes, global, 1 named locals
 * game_CLR_GLOBALBIT
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_CLR_GLOBALBIT(unsigned bit)
{
    abGlobalBits[bit >> 3] &=
        (uint8_t)~(uint8_t)(1U << (bit & 7U));
}

/* 0xA7630, 2848 bytes, global, 27 named locals
 * game_DisplayOverlay
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

void game_DisplayOverlay(void)
{
    uint32_t game_state = GameStruct.GameState;
    int suppress_overlay =
        GameStruct.screenShotFlag == 2 ||
        OptionStruct.AIDebug >= 2 ||
        (game_state & UINT32_C(0x02000000)) != 0 ||
        refreshHUDCounter != 0;
    int show_credits;
    int index;

    if (suppress_overlay) {
        for (index = 0; index < 16; ++index) {
            game_hide_overlay_scb(creditScb[index]);
        }
        game_hide_overlay_scb(rescueScb);
        for (index = 0; index < 2; ++index) {
            game_hide_overlay_scb(itemScb[index]);
            game_hide_overlay_scb(scoreScb[index]);
        }
        if (refreshHUDCounter < 1) {
            UpdateBright(0.0f, 0.0f);
        } else {
            --refreshHUDCounter;
            currentTextAlpha = 0.0f;
            currentSpriteAlpha = 0.0f;
            textBright = 0;
            spriteBright = 0;
        }
        for (index = 0; index < 16; ++index) {
            b[index] -= 10;
            if (b[index] < 0) {
                b[index] = 0;
            }
        }
        show_credits = 0;
    } else {
        BAP_CAMERADOLLY *dolly = NULL;

        UpdateBright(130.0f, 200.0f);
        show_credits = 1;
        if (gpWorld != NULL &&
            gpWorld->currentDolly >= 0 &&
            gpWorld->currentDolly < 256) {
            dolly = &gpWorld->aDolly[gpWorld->currentDolly];
        }
        if ((game_state & UINT32_C(0x00000800)) == 0 &&
            gGlobalTimer > UINT32_C(0x2000) &&
            (dolly == NULL ||
             (dolly->flags & UINT32_C(0x400)) == 0) &&
            (gaPlayerData[0].pFlags & 2) == 0 &&
            (gaPlayerData[1].pFlags & 2) == 0) {
            playerOffScreenArrow(
                &maPhysicsData[0],
                jedi_GetColour32(
                    (uint64_t)(uint16_t)
                        gaPlayerData[0].playerID) &
                    UINT32_C(0x00ffffff));
            if (GameStruct.NumPlayers == 2) {
                playerOffScreenArrow(
                    &maPhysicsData[1],
                    jedi_GetColour32(
                        (uint64_t)(uint16_t)
                            gaPlayerData[1].playerID) &
                        UINT32_C(0x00ffffff));
            }
        }
    }

    if (LevelSelect != 0 && LevelSelect != 12) {
        float continue_space = scaleAdjustment * 38.0f;
        float y = scaleAdjustment * 44.0f;
        float icon_offset =
            (scaleAdjustment * 54.0f - continue_space) * 0.5f;
        float screen_center =
            (float)OptionStruct.ScreenWidth * 0.5f;
        float remaining =
            (float)(GameStruct.mNumContinues -
                    GameStruct.ContinuesUsed);
        float columns = 2.5f;
        float x;
        int count = (int)remaining;
        int icon_size = (int)(scaleAdjustment * 54.0f);

        ++glow;
        if (glow > 32) {
            glow = 0;
        }
        if (remaining <= 5.0f) {
            columns = remaining * 0.5f;
        }
        x = screen_center - continue_space * columns - icon_offset;

        for (index = 0; index < count && index < 16; ++index) {
            int clut = 8;

            if (index == 5) {
                y += continue_space;
                x = screen_center -
                    (remaining - 5.0f) * 0.5f *
                        continue_space -
                    icon_offset;
            }
            if (index == count - 1 &&
                (GameStruct.aCharacterData[0].Energy < 25 ||
                 GameStruct.aCharacterData[1].Energy < 25)) {
                clut = 2;
            }
            if (creditScb[index] != NULL) {
                creditScb[index]->scb_flags &= ~0x40;
            }
            creditScb[index] = sprite_DisplaySprite(
                creditScb[index],
                49,
                (int)x,
                (int)y,
                icon_size,
                icon_size,
                clut);
            if (creditScb[index] != NULL) {
                creditScb[index]->scb_vertex0.pad =
                    (int16_t)b[index];
                if (clut == 2) {
                    creditScb[index]->scb_vertex0.pad =
                        (int16_t)(b[index] + glow);
                }
            }
            if (show_credits && OptionStruct.overlayMode != 0) {
                if (index == 0) {
                    if (b[index] < 200) {
                        b[index] += 10;
                    }
                } else if (b[index - 1] > 199 &&
                           b[index] < 201) {
                    b[index] += 10;
                }
            } else {
                b[index] -= 25;
                if (b[index] < 0) {
                    b[index] = 0;
                }
            }
            x += continue_space;
        }
        if (count < 0) {
            count = 0;
        }
        for (index = count; index < 16; ++index) {
            game_hide_overlay_scb(creditScb[index]);
        }
    }

    if (OptionStruct.overlayMode == 0) {
        return;
    }
    game_DrawScore(0);
    game_DrawItems(0);
    if (GameStruct.NumPlayers == 2) {
        game_DrawScore(1);
        game_DrawItems(1);
    }

    if ((game_state & UINT32_C(0x01000000)) != 0) {
        float sprite_width = 192.0f;
        float sprite_height = 192.0f;
        float sprite_x = 0.0f;
        float sprite_y = 32.0f;
        float text_x = 144.0f;
        float text_y = 112.0f;
        int sprite_type =
            GameStruct.CurrentLevel == 3 ? 44 : 43;

        setPivotPositionAndFixScale(
            &sprite_x,
            &sprite_y,
            &sprite_width,
            &sprite_height,
            7);
        setPositionOffPivot(
            &text_x, &text_y, sprite_x, sprite_y);
        if (rescueScb != NULL) {
            rescueScb->scb_flags &= ~0x40;
        }
        rescueScb = sprite_DisplaySprite(
            rescueScb,
            sprite_type,
            (int)sprite_x,
            (int)sprite_y,
            (int)sprite_width,
            (int)sprite_height,
            8);
        if (rescueScb != NULL) {
            rescueScb->scb_vertex0.pad =
                (int16_t)spriteBright;
        }
        (void)_DrawText(
            text_x,
            text_y,
            0.0001f,
            0.6f,
            ((uint32_t)textBright << 24) |
                UINT32_C(0x00ffffff),
            "%d",
            GameStruct.Counter);
        return;
    }

    if (LevelSelect == 11) {
        float sprite_width = 192.0f;
        float sprite_height = 192.0f;
        float sprite_x = 0.0f;
        float sprite_y = 32.0f;
        float text_x;
        float text_y;

        setPivotPositionAndFixScale(
            &sprite_x,
            &sprite_y,
            &sprite_width,
            &sprite_height,
            7);
        if (rescueScb != NULL) {
            rescueScb->scb_flags &= ~0x40;
        }
        rescueScb = sprite_DisplaySprite(
            rescueScb,
            43,
            (int)sprite_x,
            (int)sprite_y,
            (int)sprite_width,
            (int)sprite_height,
            8);
        if (rescueScb != NULL) {
            rescueScb->scb_vertex0.pad =
                (int16_t)spriteBright;
        }
        if (gPilotDeathCount < 10) {
            text_x = 144.0f;
            text_y = 112.0f;
            setPositionOffPivot(
                &text_x, &text_y, sprite_x, sprite_y);
            (void)SDLTextWriteScale(
                11,
                (int)textBright,
                0,
                (int)text_x,
                (int)text_y,
                1.8f,
                2,
                L"%d",
                gPilotDeathCount);
        } else {
            text_x = 153.0f;
            text_y = 115.0f;
            setPositionOffPivot(
                &text_x, &text_y, sprite_x, sprite_y);
            (void)SDLTextWriteScale(
                11,
                (int)textBright,
                2,
                (int)text_x,
                (int)text_y,
                1.5f,
                2,
                L"%d",
                gPilotDeathCount);
        }
    }
}

/* 0xA8150, 257 bytes, global, 3 named locals
 * game_DrawBigNum
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

void game_DrawBigNum(unsigned num, unsigned x, unsigned y)
{
    (void)psxDrawTexture(
        (num / 10) % 10 + 0xb5,
        (float)x,
        (float)y,
        16.0f,
        14.0f,
        200,
        0x80,
        0x80,
        0x80);
    (void)psxDrawTexture(
        num % 10 + 0xb5,
        (float)(x + 18),
        (float)y,
        16.0f,
        14.0f,
        200,
        0x80,
        0x80,
        0x80);
}

/* 0xA8260, 434 bytes, global, 9 named locals
 * game_DrawItems
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

void game_DrawItems(unsigned player)
{
    float sprite_width = 192.0f;
    float sprite_height = 192.0f;
    float sprite_x = 44.0f;
    float sprite_y = 32.0f;
    float text_x = 144.0f;
    float text_y = 112.0f;
    int level = (uint8_t)LevelSelect;
    int model;

    if (player >= 2 || level == 11) {
        return;
    }
    setPivotPositionAndFixScale(
        &sprite_x,
        &sprite_y,
        &sprite_width,
        &sprite_height,
        player == 0 ? 6 : 8);
    setPositionOffPivot(
        &text_x, &text_y, sprite_x, sprite_y);
    if (itemScb[player] != NULL) {
        itemScb[player]->scb_flags &= ~0x40;
    }
    if (!((level >= 1 && level <= 11) ||
          (level >= 14 && level <= 15))) {
        return;
    }

    (void)_DrawText(
        text_x,
        text_y,
        0.0001f,
        0.6f,
        ((uint32_t)textBright << 24) |
            UINT32_C(0x00ffffff),
        "%d",
        GameStruct.aCharacterData[player].Items);
    model = GameStruct.ModelSelect[player];
    if (model >= 9) {
        model = 9;
    }
    itemScb[player] = sprite_DisplaySprite(
        itemScb[player],
        charStuff[model],
        (int)sprite_x,
        (int)sprite_y,
        (int)sprite_width,
        (int)sprite_height,
        8);
    if (itemScb[player] != NULL) {
        itemScb[player]->scb_vertex0.pad =
            (int16_t)spriteBright;
    }
}

/* 0xA8420, 822 bytes, global, 13 named locals
 * game_DrawScore
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

void game_DrawScore(unsigned player)
{
    unsigned level = (uint8_t)LevelSelect;

    if (player >= 2 || level >= 26 ||
        ((UINT32_C(0x027fcffe) >> (level & 31)) & 1) == 0) {
        return;
    }
    if (OptionStruct.overlayMode == 2) {
        float sprite_width = 480.0f;
        float sprite_height = 204.0f;
        float sprite_x = 48.0f;
        float sprite_y = 36.0f;
        float text_x = player == 0 ? 164.0f : 24.0f;
        float text_y = 24.0f;
        float health_x = player == 0 ? 163.0f : 317.0f;
        float health_y = 115.0f;

        setPivotPositionAndFixScale(
            &sprite_x,
            &sprite_y,
            &sprite_width,
            &sprite_height,
            player == 0 ? 0 : 2);
        setPositionOffPivot(
            &text_x, &text_y, sprite_x, sprite_y);
        setPositionOffPivot(
            &health_x, &health_y, sprite_x, sprite_y);
        if (player != 0) {
            sprite_x += sprite_width;
            sprite_width = -sprite_width;
        }
        if (scoreScb[player] != NULL) {
            scoreScb[player]->scb_flags &= ~0x40;
        }
        (void)_DrawText(
            text_x,
            text_y,
            0.0001f,
            1.08f,
            ((uint32_t)textBright << 24) |
                UINT32_C(0x00ffffff),
            "%07d",
            GameStruct.aCharacterData[player].Score);
        _AddLifeTile2D(
            &gaPlayerData[player],
            (unsigned)health_x,
            (unsigned)health_y,
            (int)textBright);
        scoreScb[player] = sprite_DisplaySprite(
            scoreScb[player],
            40,
            (int)sprite_x,
            (int)sprite_y,
            (int)sprite_width,
            (int)sprite_height,
            8);
        if (scoreScb[player] != NULL) {
            scoreScb[player]->scb_vertex0.pad =
                (int16_t)spriteBright;
        }
        return;
    }
    if (OptionStruct.overlayMode == 1) {
        float text_x = 48.0f;
        float text_y = 62.0f;

        setPivotPosition(
            &text_x, &text_y, player == 0 ? 0 : 2);
        (void)SDLTextWriteScale(
            11,
            (int)textBright & 0xff,
            player == 0 ? 0 : 1,
            (int)text_x,
            (int)text_y,
            3.24f,
            2,
            L"%07d",
            GameStruct.aCharacterData[player].Score);
    }
}

/* 0xA8760, 35 bytes, global, 1 named locals
 * game_GET_GLOBALBIT
 * PDB type: int (unsigned)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_GET_GLOBALBIT(unsigned bit)
{
    return (abGlobalBits[bit >> 3] &
            (uint8_t)(1U << (bit & 7U))) != 0;
}

/* 0xA8790, 8 bytes, global, 1 named locals
 * game_GetGameCounter
 * PDB type: char (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
char game_GetGameCounter(int index)
{
    (void)index;
    return (char)GameStruct.Counter;
}

/* 0xA87A0, 8 bytes, global, 0 named locals
 * game_GetGameMode
 * PDB type: char ()
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
char game_GetGameMode(void)
{
    return GameStruct.Mode;
}

/* 0xA87B0, 883 bytes, global, 0 named locals
 * game_InitGameSystems
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA8B30, 15 bytes, global, 2 named locals
 * game_ModGameCounter
 * PDB type: char (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
char game_ModGameCounter(int amount)
{
    uint32_t counter =
        (uint32_t)GameStruct.Counter +
        (uint32_t)amount;

    GameStruct.Counter = (int32_t)counter;
    return (char)counter;
}

/* 0xA8B40, 351 bytes, global, 4 named locals
 * game_OneGameLoop
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA8CA0, 769 bytes, global, 3 named locals
 * game_ProcessStatus
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_ProcessStatus(void)
{
    playerObject *player0 = gpWorld->player0;
    playerObject *player1 = gpWorld->player1;
    uint32_t state;
    int previous_continues;

    brainutl_ConformGeomNodes(player0);
    brainutl_ConformGeomNodes(player1);
    state = GameStruct.GameState;
    if ((state & UINT32_C(0x60)) == UINT32_C(0x60)) {
        GameStruct.GameState = state & ~UINT32_C(0x60);
        afterLife = NULL;
        GameStruct.StageExit = 1;
    } else if ((state & UINT32_C(0x40)) != 0) {
        GameStruct.GameState = state & ~UINT32_C(0x40);
        if (GameStruct.NumPlayers != 1 &&
            (player0->pFlags & UINT32_C(0x200)) == 0) {
            camera_SetCurrentCameraType(1);
            GameStruct.StageExit = 0;
            afterLife = player1;
        } else {
            GameStruct.StageExit = 1;
        }
    } else if ((state & UINT32_C(0x20)) != 0) {
        GameStruct.GameState = state & ~UINT32_C(0x20);
        if (GameStruct.NumPlayers != 1 &&
            (player1->pFlags & UINT32_C(0x200)) == 0) {
            camera_SetCurrentCameraType(2);
            GameStruct.StageExit = 0;
            afterLife = player0;
        } else {
            GameStruct.StageExit = 1;
        }
    } else {
        if ((state & UINT32_C(0x00010000)) == 0) {
            return;
        }
        /* platform_completeLevel is an exact no-op linkstub in this build. */
        GameStruct.GameState &= ~UINT32_C(0x80);
        GameStruct.StageExit = 1;
        return;
    }

    GameStruct.GameState &= ~UINT32_C(0x80);
    newcameraflag = 1;
    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }

    previous_continues = (int)GameStruct.ContinuesUsed;
    GameStruct.ContinuesUsed =
        (int8_t)((uint8_t)GameStruct.ContinuesUsed + UINT8_C(1));
    if (previous_continues < GameStruct.mNumContinues) {
        GameStruct.Continuing = 1;
        if ((abGlobalBits[3] & UINT8_C(1)) == 0) {
            return;
        }
        abGlobalBits[3] &= UINT8_C(0xfe);
        if (GameStruct.CurrentLevel <= 22 &&
            ((UINT32_C(0x007f2800) >> GameStruct.CurrentLevel) & 1U) != 0) {
            return;
        }
        if (OptionStruct.Music != 0) {
            stopXA();
            playXA(3, (int)OptionStruct.musicVolume * 2, 0);
        }
        menu_specialMess((uint8_t *)(void *)allText[376]);
        gCheckPoint = 0;
        afterLife = NULL;
        return;
    }

    if (((player0->playerRoot.objectID != -1 &&
          obj_gCheckObjectFlag(&player0->playerRoot, 0, 0x20) == 0 &&
          (player0->pFlags & UINT32_C(0x40200)) == 0) ||
         (player1->playerRoot.objectID != -1 &&
          obj_gCheckObjectFlag(&player1->playerRoot, 0, 0x20) == 0 &&
          (player1->pFlags & UINT32_C(0x40200)) == 0)) &&
        (GameStruct.CurrentLevel <= 10 ||
         GameStruct.CurrentLevel == 15) &&
        (abGlobalBits[3] & UINT8_C(1)) == 0) {
        GameStruct.ContinuesUsed = (int8_t)GameStruct.mNumContinues;
        GameStruct.Continuing = 1;
        GameStruct.StageExit = 0;
        afterLife = NULL;
        return;
    }

    if (OptionStruct.Music != 0) {
        stopXA();
        playXA(3, (int)OptionStruct.musicVolume * 2, 0);
    }
    GameStruct.LevelExit = 1;
    afterLife = NULL;
    sound_StopAll();
    menu_initGameover();
}

/* 0xA8FB0, 185 bytes, global, 1 named locals
 * game_ResetGameSystems
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_ResetGameSystems(void)
{
    stop_all_looped_sounds();
    restore_events(leveldata);
    physics_InitPhysics();
    player_gRefreshPlayers();
    enemy_ResetEnemies();
    camera_RestoreCameras();
    pwrup_Init();
    braindmg_ResetDamageTracker(0);
    braindmg_ResetDamageTracker(1);
    afterLife = NULL;
    /* clearzerobss tail-calls the matched no-op _clearzerobss. */
    zerobss_levelReset = 1;
    zerobss_ResetBoss = 1;
    cube_InitVisibility();
    ClearCachedTextureIndices();
    sound_StopAll();
    stopXA();
    if (GameStruct.CurrentLevel < 26 && OptionStruct.Music == 1) {
        playXA(
            (int)aLevelXATracks[GameStruct.CurrentLevel],
            (int)OptionStruct.musicVolume * 2,
            1);
    }
    gHidePikobisModel = 1;
}

/* 0xA9070, 28 bytes, global, 1 named locals
 * game_SET_GLOBALBIT
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_SET_GLOBALBIT(unsigned bit)
{
    abGlobalBits[bit >> 3] |=
        (uint8_t)(1U << (bit & 7U));
}

/* 0xA9090, 1036 bytes, global, 2 named locals
 * game_checkCompleteAchievements
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA94A0, 162 bytes, global, 0 named locals
 * game_checkNextLevel
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA9550, 3 bytes, global, 0 named locals
 * game_clearLetterBox
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_clearLetterBox(void)
{
}

/* 0xA9560, 39 bytes, global, 2 named locals
 * game_disableCombo
 * PDB type: void (unsigned, unsigned)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_disableCombo(uint32_t jedi, uint32_t combo)
{
    GameStruct.jediComboMask[jedi].m[combo >> 3] &=
        (uint8_t)~(uint8_t)(1U << (combo & 7U));
}

/* 0xA9590, 39 bytes, global, 2 named locals
 * game_enableCombo
 * PDB type: void (unsigned, unsigned)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_enableCombo(uint32_t jedi, uint32_t combo)
{
    GameStruct.jediComboMask[jedi].m[combo >> 3] |=
        (uint8_t)(1U << (combo & 7U));
}

/* 0xA95C0, 17 bytes, global, 1 named locals
 * game_gClrGameFlags
 * PDB type: unsigned long (unsigned long)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
uint32_t game_gClrGameFlags(uint32_t flag)
{
    GameStruct.GameState &= ~flag;
    return GameStruct.GameState;
}

/* 0xA95E0, 19 bytes, global, 1 named locals
 * game_gGetEnergy
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetEnergy(int player)
{
    return GameStruct.aCharacterData[player].Energy;
}

/* 0xA9600, 19 bytes, global, 1 named locals
 * game_gGetForce
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetForce(int player)
{
    return GameStruct.aCharacterData[player].Force;
}

/* 0xA9620, 7 bytes, global, 0 named locals
 * game_gGetGameFlags
 * PDB type: unsigned long ()
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
uint32_t game_gGetGameFlags(void)
{
    return GameStruct.GameState;
}

/* 0xA9630, 19 bytes, global, 1 named locals
 * game_gGetItemCount
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetItemCount(int player)
{
    return GameStruct.aCharacterData[player].Items;
}

/* 0xA9650, 19 bytes, global, 1 named locals
 * game_gGetMaxEnergy
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetMaxEnergy(int player)
{
    return GameStruct.aCharacterData[player].MaxEnergy;
}

/* 0xA9670, 19 bytes, global, 1 named locals
 * game_gGetMaxForce
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetMaxForce(int player)
{
    return GameStruct.aCharacterData[player].MaxForce;
}

/* 0xA9690, 18 bytes, global, 1 named locals
 * game_gGetPowerLevel
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetPowerLevel(int player)
{
    return GameStruct.aCharacterData[player].PowerLevel;
}

/* 0xA96B0, 19 bytes, global, 1 named locals
 * game_gGetPowerType
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetPowerType(int player)
{
    return GameStruct.aCharacterData[player].PowerType;
}

/* 0xA96D0, 34 bytes, global, 1 named locals
 * game_gGetScaleEnergy
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetScaleEnergy(int player)
{
    CharacterData *character =
        &GameStruct.aCharacterData[player];
    uint32_t product = (uint32_t)(
        (int32_t)character->Energy *
        (int32_t)character->MaxEnergyPerc);

    return (int)(product >> 16);
}

/* 0xA9700, 37 bytes, global, 1 named locals
 * game_gGetScaleForce
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetScaleForce(int player)
{
    CharacterData *character =
        &GameStruct.aCharacterData[player];

    return ((int32_t)character->Force *
            (int32_t)character->MaxForcePerc) >> 16;
}

/* 0xA9730, 34 bytes, global, 1 named locals
 * game_gGetScaleMaxEnergy
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetScaleMaxEnergy(int player)
{
    CharacterData *character =
        &GameStruct.aCharacterData[player];
    uint32_t product = (uint32_t)(
        (int32_t)character->MaxEnergy *
        (int32_t)character->MaxEnergyPerc);

    return (int)(product >> 16);
}

/* 0xA9760, 37 bytes, global, 1 named locals
 * game_gGetScaleMaxForce
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetScaleMaxForce(int player)
{
    CharacterData *character =
        &GameStruct.aCharacterData[player];

    return ((int32_t)character->MaxForce *
            (int32_t)character->MaxForcePerc) >> 16;
}

/* 0xA9790, 18 bytes, global, 1 named locals
 * game_gGetScore
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gGetScore(int player)
{
    return GameStruct.aCharacterData[player].Score;
}

/* 0xA97B0, 9 bytes, global, 1 named locals
 * game_gIsGameFlags
 * PDB type: unsigned long (unsigned long)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
uint32_t game_gIsGameFlags(uint32_t flag)
{
    return GameStruct.GameState & flag;
}

/* 0xA97C0, 99 bytes, global, 2 named locals
 * game_gModEnergy
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gModEnergy(int player, int level)
{
    int energy = GameStruct.aCharacterData[player].Energy;
    int modified = energy + level;

    if (modified < 0) {
        modified = 0;
    } else if (modified > GameStruct.aCharacterData[player].MaxEnergy) {
        modified = GameStruct.aCharacterData[player].MaxEnergy;
    }

    if (LevelSelect == 8) {
        if (player < 2 ||
            gaPlayerData[player].playerID == 0x17 ||
            gaPlayerData[player].playerID == 0x48) {
            return energy;
        }
        modified = 0;
    }
    return game_gSetEnergy(player, modified);
}

/* 0xA9830, 69 bytes, global, 2 named locals
 * game_gModForce
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gModForce(int player, int level)
{
    CharacterData *character =
        &GameStruct.aCharacterData[player];
    int force = (int)character->Force + level;

    if (force < 0) {
        character->Force = 0;
        return 0;
    }
    if (force > character->MaxForce) {
        force = character->MaxForce;
    }
    character->Force = (int16_t)force;
    return (int)character->Force;
}

/* 0xA9880, 53 bytes, global, 2 named locals
 * game_gModItemCount
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gModItemCount(int player, int level)
{
    int item_count =
        (int)GameStruct.aCharacterData[player].Items +
        level;

    if (item_count < 0) {
        item_count = 0;
    } else if (item_count > 4) {
        item_count = 4;
    }
    GameStruct.aCharacterData[player].Items =
        (int16_t)item_count;
    return item_count;
}

/* 0xA98C0, 48 bytes, global, 2 named locals
 * game_gModScore
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gModScore(int player, int level)
{
    int score =
        GameStruct.aCharacterData[player].Score + level;

    if (score < 0) {
        score = 0;
    } else if (score > 999999) {
        score = 999999;
    }
    GameStruct.aCharacterData[player].Score = score;
    return score;
}

/* 0xA98F0, 1328 bytes, global, 4 named locals
 * game_gPlayTheGame
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xA9E20, 187 bytes, global, 3 named locals
 * game_gSetEnergy
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gSetEnergy(int player, int level)
{
    CharacterData *character = &GameStruct.aCharacterData[player];
    uint32_t LifeLineLength;

    if (level < 0) {
        level = 0;
    } else if (player == 0) {
        if (level > 0x640) {
            level = 0x640;
        }
    } else if (level > 0xff) {
        level = 0xff;
    }

    character->Energy = (int16_t)level;
    if (level <= character->MaxEnergy) {
        return level;
    }

    character->MaxEnergy = (int16_t)level;
    if (player < 2) {
        if (GameStruct.ModelSelect[player] < 9) {
            LifeLineLength = GameStruct.maxEnergyLineLength[
                GameStruct.ModelSelect[player]];
        } else {
            LifeLineLength = 25;
        }
    } else if (level >= 50) {
        LifeLineLength = 25;
    } else {
        LifeLineLength = 12;
    }

    if (level == 0) {
        character->MaxEnergyPerc = 0;
    } else {
        character->MaxEnergyPerc =
            (LifeLineLength << 16) / (uint32_t)level;
    }
    return level;
}

/* 0xA9EE0, 40 bytes, global, 2 named locals
 * game_gSetForce
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gSetForce(int player, int level)
{
    if (level < 0) {
        level = 0;
    } else if (level > 0xff) {
        level = 0xff;
    }
    GameStruct.aCharacterData[player].Force = (int16_t)level;
    return level;
}

/* 0xA9F10, 15 bytes, global, 1 named locals
 * game_gSetGameFlags
 * PDB type: unsigned long (unsigned long)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
uint32_t game_gSetGameFlags(uint32_t flag)
{
    GameStruct.GameState |= flag;
    return GameStruct.GameState;
}

/* 0xA9F20, 40 bytes, global, 2 named locals
 * game_gSetItemCount
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gSetItemCount(int player, int level)
{
    if (level < 0) {
        level = 0;
    } else if (level > 4) {
        level = 4;
    }
    GameStruct.aCharacterData[player].Items = (int16_t)level;
    return level;
}

/* 0xA9F50, 116 bytes, global, 3 named locals
 * game_gSetMaxEnergy
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_gSetMaxEnergy(int player, int level)
{
    CharacterData *character = &GameStruct.aCharacterData[player];
    uint32_t line_length;

    character->MaxEnergy = (int16_t)level;
    if (player < 2) {
        if (GameStruct.ModelSelect[player] < 9) {
            line_length = GameStruct.maxEnergyLineLength[
                GameStruct.ModelSelect[player]];
        } else {
            line_length = 25;
        }
    } else if (level > 49) {
        line_length = 25;
    } else {
        line_length = 12;
    }
    character->MaxEnergyPerc =
        level != 0
            ? (line_length << 16) / (uint32_t)level
            : 0;
}

/* 0xA9FD0, 93 bytes, global, 3 named locals
 * game_gSetMaxForce
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_gSetMaxForce(int player, int level)
{
    CharacterData *character = &GameStruct.aCharacterData[player];
    uint32_t line_length;

    character->MaxForce = (int16_t)level;
    if (player < 2 &&
        GameStruct.ModelSelect[player] < 9) {
        line_length = GameStruct.maxForceLineLength[
            GameStruct.ModelSelect[player]];
    } else {
        line_length = 25;
    }
    character->MaxForcePerc =
        level != 0
            ? (int16_t)(
                  (line_length << 16) / (uint32_t)level)
            : 0;
}

/* 0xAA030, 44 bytes, global, 2 named locals
 * game_gSetPowerLevel
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gSetPowerLevel(int player, int level)
{
    int32_t power_level;

    if (level < 0) {
        level = 0;
    } else if (level > 0x70800) {
        level = 0x70800;
    }
    power_level = (int32_t)(gGlobalTimer + (uint32_t)level);
    GameStruct.aCharacterData[player].PowerLevel = power_level;
    return power_level;
}

/* 0xAA060, 22 bytes, global, 2 named locals
 * game_gSetPowerType
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gSetPowerType(int player, int level)
{
    int16_t power_type = (int16_t)level;

    GameStruct.aCharacterData[player].PowerType = power_type;
    return power_type;
}

/* 0xAA080, 54 bytes, global, 2 named locals
 * game_gSetScore
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
int game_gSetScore(int player, int level)
{
    if (level < 0) {
        level = 0;
    } else if (level > 999999) {
        level = 999999;
    }
    GameStruct.aCharacterData[player].Score = level;
    return level;
}

/* 0xAA0C0, 15 bytes, global, 1 named locals
 * game_gToggleGameFlags
 * PDB type: unsigned long (unsigned long)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
uint32_t game_gToggleGameFlags(uint32_t flag)
{
    GameStruct.GameState ^= flag;
    return GameStruct.GameState;
}

/* 0xAA0D0, 47 bytes, global, 2 named locals
 * game_getCombo
 * PDB type: unsigned (unsigned, unsigned)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
uint32_t game_getCombo(uint32_t jedi, uint32_t combo)
{
    return (uint32_t)GameStruct.jediComboMask[jedi]
               .m[combo >> 3] &
           (UINT32_C(1) << (combo & 7));
}

/* 0xAA100, 728 bytes, global, 2 named locals
 * game_initCombos
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_initCombos(void)
{
    uint32_t jedi;

    for (jedi = 0; jedi < 9; ++jedi) {
        int8_t *combo = initialJediCombos[jedi];

        memset(
            &GameStruct.jediComboMask[jedi],
            0,
            sizeof(GameStruct.jediComboMask[jedi]));
        while (*combo != -1) {
            game_enableCombo(jedi, (uint8_t)*combo);
            ++combo;
        }
    }
}

/* 0xAA3E0, 267 bytes, global, 0 named locals
 * game_initEnergy
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_initEnergy(void)
{
    int model;

    for (model = 0;
         model < JPB_GAME_JEDI_MODEL_CAPACITY;
         ++model) {
        GameStruct.maxEnergyLevels[model] = 100;
        GameStruct.maxEnergyLineLength[model] = 25;
        GameStruct.maxForceLevels[model] = 100;
        GameStruct.maxForceLineLength[model] = 25;
    }
}

/* 0xAA4F0, 780 bytes, global, 6 named locals
 * game_initPerLevel
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xAA800, 200 bytes, global, 5 named locals
 * game_initPlayerStartCombos
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_initPlayerStartCombos(uint32_t player)
{
    playerObject *player_object = &gaPlayerData[player];
    uint32_t jedi = player;
    uint32_t combo;

    if (player_object->playerID <
        JPB_GAME_JEDI_MODEL_CAPACITY) {
        jedi = (uint32_t)player_object->playerID;
    }
    for (combo = 0;
         combo < (uint32_t)player_object->maxCombos;
         ++combo) {
        Combo *combo_data =
            &player_object->paCombos[combo];

        if (combo_data->String[0] != '\0' &&
            (combo_data->comboFlags &
             UINT32_C(0x00200000)) != 0) {
            GameStruct.jediComboMask[jedi]
                .m[combo >> 3] |=
                (uint8_t)(UINT32_C(1) <<
                          (combo & 7));
        }
    }
}

/* 0xAA8D0, 1151 bytes, global, 1 named locals
 * game_initVar
 * PDB type: void (unsigned)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xAAD50, 36 bytes, global, 0 named locals
 * game_loadLevelMode
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xAAD80, 453 bytes, global, 1 named locals
 * game_runStage
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_runStage(void)
{
    if ((GameStruct.GameState & UINT32_C(0x00100000)) != 0) {
        player_gRefreshPlayers();
        GameStruct.GameState &= ~UINT32_C(0x00100000);
    }
    if (GameStruct.CurrentLevel == 0) {
        GameStruct.GameState &= ~UINT32_C(0xe0);
        GameStruct.StageExit = 0;
        GameStruct.LevelExit = 0;
    } else if ((int32_t)GameStruct.GameState >= 0) {
        game_ProcessStatus();
    }

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        mute_looped_sounds();
        update_looped_sounds();
        GameStruct.Continuing = 0;
        return;
    }
    if (GameStruct.LevelExit != 0) {
        GameStruct.GameState |= UINT32_C(2);
        stop_all_looped_sounds();
    }
    if (GameStruct.StageExit != 0) {
        stop_all_looped_sounds();
        waitForInputReadAfterRefresh = 2;
        game_ResetGameSystems();
        GameStruct.GameState &= ~UINT32_C(0xe0);
        GameStruct.StageExit = 0;
    }
    if (GameStruct.LevelExit != 0) {
        GameStruct.StageExit = 0;
        GameStruct.gameMode = 9;
        stop_all_looped_sounds();
        if (GameStruct.mNumContinues < 5) {
            GameStruct.mNumContinues = 5;
        } else if (GameStruct.mNumContinues > 9) {
            GameStruct.mNumContinues = 9;
        }
        GameStruct.ContinuesUsed = 0;
        afterLife = NULL;
    }
    unmute_looped_sounds();
    update_looped_sounds();
    GameStruct.Continuing = 0;
}

/* 0xAAF50, 66 bytes, global, 0 named locals
 * game_setAudioOptions
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_setAudioOptions(void)
{
    OptionStruct.Stereo = defaultOptionStruct.Stereo;
    OptionStruct.Music = defaultOptionStruct.Music;
    OptionStruct.musicVolume = defaultOptionStruct.musicVolume;
    OptionStruct.SFXVolume = defaultOptionStruct.SFXVolume;
    OptionStruct.PadAudioEnabled =
        defaultOptionStruct.PadAudioEnabled;
}

/* 0xAAFA0, 76 bytes, global, 1 named locals
 * game_setControlsOptions
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_setControlsOptions(int player)
{
    if (player < 2) {
        OptionStruct.ControllerConfig[player] =
            defaultOptionStruct.ControllerConfig[player];
        OptionStruct.WalkLimit[player] =
            defaultOptionStruct.WalkLimit[player];
        OptionStruct.RunLimit[player] =
            defaultOptionStruct.RunLimit[player];
        OptionStruct.ShockFlag[player] =
            defaultOptionStruct.ShockFlag[player];
    }
}

/* 0xAAFF0, 59 bytes, global, 0 named locals
 * game_setDefaultOptions
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_setDefaultOptions(void)
{
    OptionStruct = defaultOptionStruct;
}

/* 0xAB030, 698 bytes, global, 0 named locals
 * game_setFuncArray
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_setFuncArray(void)
{
    /*
     * Exact callback indexes come from the stores at RVA 0xAB030. Keep
     * slot zero intentionally null, matching the retail initialization.
     */
    memset(funcArray, 0, sizeof(funcArray));
    funcArray[1] = ai_FireWeapon;
    funcArray[2] = ai_Throw;
    funcArray[3] = brain_HangCallback;
    funcArray[4] = brain_SkidCallBack;
    funcArray[5] = brain_ThrowEnder;
    funcArray[6] = brainutil_PlotTrajectory;
    funcArray[7] = force_AbsorbReflectCallBack;
    funcArray[8] = force_AttackCallBack;
    funcArray[9] = force_AttackSpinCallBack;
    funcArray[10] = force_CloakCallBack;
    funcArray[11] = force_FlameCallBack;
    funcArray[12] = force_HealingCallBack;
    funcArray[13] = force_MesmerizeCallBack;
    funcArray[14] = force_PushCallBack;
    funcArray[15] = force_Ranged3CallBack;
    funcArray[16] = force_ReflectCallBack;
    funcArray[17] = force_RingCallBack;
    funcArray[18] = force_SabreSpinCallBack;
    funcArray[19] = force_SabreTossCallBack;
    funcArray[20] = force_SabreYoYoBack;
    funcArray[21] = force_ShieldCallBack;
    funcArray[22] = force_StarCallBack;
    funcArray[23] = force_TossCallBack;
    funcArray[24] = force_ZapCallBack;
    funcArray[25] = force_TossGrenadeCallBack;
    funcArray[26] = jedi_FireWeapon;
    funcArray[27] = maul_PushCallBack;
    funcArray[28] = maul_RingCallBack;
    funcArray[29] = maul_ZapCallBack;
    funcArray[30] = ai_Tank;
    funcArray[31] = ai_Stap;
    funcArray[32] = jedi_Main;
    funcArray[33] = jpb_ai_MainCallback;
    funcArray[34] = ai_Kadu;
    funcArray[35] = ai_Thug;
    funcArray[36] = ai_Blades;
    funcArray[37] = ai_Maul;
    funcArray[38] = ai_JarJar;
    funcArray[39] = ai_Worm;
    funcArray[40] = ai_StarFighter;
    funcArray[41] = ai_Krakis;
    funcArray[42] = ai_LoaderDroid;
    funcArray[43] = ai_Mtt;
    funcArray[44] = ai_Destroyer;
    funcArray[45] = ai_AAT;
    funcArray[46] = ai_TurretDroid;
    funcArray[47] = ai_Deadly;
    funcArray[48] =
        brainutil_PlotMaulTrajectory;
    funcArray[49] = tusken_stab;

    jpb_TrajectoryCallbackSlot = funcArray[6];
    jpb_MaulTrajectoryCallbackSlot = funcArray[48];
}

/* 0xAB2F0, 3 bytes, global, 0 named locals
 * game_setLetterBox
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void game_setLetterBox(void)
{
}

/* 0xAB300, 108 bytes, global, 3 named locals
 * getvar
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */

/* 0xAB370, 142 bytes, global, 0 named locals
 * newGameGameInit
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
void newGameGameInit(void)
{
    memset(
        GameStruct.jediLevelPlayed,
        0,
        sizeof(GameStruct.jediLevelPlayed));
    memset(
        GameStruct.jediScorePerLevel,
        0,
        sizeof(GameStruct.jediScorePerLevel));
    memset(
        GameStruct.aCharacterData,
        0,
        sizeof(GameStruct.aCharacterData));
    game_initCombos();
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    GameStruct.Continuing = 0;
    secretBits = 0;
    game_initEnergy();
    memset(jediUpgrades, 0, sizeof(jediUpgrades));
    GameStruct.gameCompleted = 0;
    GameStruct.ModelSelect[0] = 0;
    GameStruct.ModelSelect[1] = 1;
}

/* 0xAB400, 220 bytes, global, 6 named locals
 * setvar
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\game.c
 */
