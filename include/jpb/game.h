#ifndef JPB_GAME_H
#define JPB_GAME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct playerObject;

enum {
    JPB_GAME_CHARACTER_CAPACITY = 20,
    JPB_GAME_JEDI_MODEL_CAPACITY = 23,
    JPB_GAME_LEVEL_CAPACITY = 30,
    JPB_GAME_CHECKPOINT_CAPACITY = 16
};

/* Exact matched-PC PDB type 0x1061. */
typedef struct optionstruct {
    int32_t saveFileVer;
    uint8_t CPULevel;
    uint8_t Stereo;
    uint8_t Music;
    uint8_t SFX;
    uint8_t Extra;
    uint8_t AutoSave;
    uint8_t musicVolume;
    uint8_t SFXVolume;
    uint8_t xaTrack;
    uint8_t oldModelSelect[2];
    uint8_t ControllerConfig[2];
    uint8_t AIDebug;
    uint8_t FunFactor;
    uint8_t WalkLimit[2];
    uint8_t RunLimit[2];
    uint8_t ShockFlag[2];
    uint8_t JumpCheat;
    uint8_t DebugLevel;
    uint8_t overlayMode;
    uint8_t Language;
    uint32_t ResolutionChanged;
    uint32_t ScreenWidth;
    uint32_t ScreenHeight;
    uint32_t WindowMode;
    uint8_t PadAudioEnabled;
    uint32_t EULAaccepted;
} optionstruct;

/* Exact matched-PC PDB type 0x108C. */
typedef struct CharacterData {
    int32_t Score;
    int16_t Energy;
    int16_t MaxEnergy;
    uint32_t MaxEnergyPerc;
    int16_t Force;
    int16_t MaxForce;
    int16_t MaxForcePerc;
    int16_t Items;
    int16_t PowerType;
    int32_t PowerLevel;
} CharacterData;

/* Exact matched-PC PDB type 0x108A. */
typedef struct JEDICOMBOMASK {
    uint8_t m[6];
} JEDICOMBOMASK;

/* Exact matched-PC PDB type 0x1457. */
typedef struct Upgrades {
    int8_t healthUpgrades;
    int8_t forceUpgrades;
    int8_t attackDefendUpgrades;
    int8_t lifeUpgrades;
    int16_t forcePowers;
    int8_t awardData[12];
} Upgrades;

/* Exact matched-PC PDB type 0x6CE8. This is the raw PC save payload. */
typedef struct saveGameStruct {
    int32_t saveFileVer;
    uint8_t validFlag;
    uint8_t gamenum;
    uint8_t lastlevel;
    uint16_t jediLevel[JPB_GAME_JEDI_MODEL_CAPACITY];
    uint8_t
        jediLevelPlayed[JPB_GAME_JEDI_MODEL_CAPACITY][JPB_GAME_LEVEL_CAPACITY];
    uint32_t
        jediScorePerLevel[JPB_GAME_JEDI_MODEL_CAPACITY][JPB_GAME_LEVEL_CAPACITY];
    uint8_t checkpoint[JPB_GAME_CHECKPOINT_CAPACITY];
    uint32_t completionPoints;
    uint16_t maxEnergyLevels[JPB_GAME_JEDI_MODEL_CAPACITY];
    uint16_t maxEnergyLineLength[JPB_GAME_JEDI_MODEL_CAPACITY];
    uint16_t maxForceLevels[JPB_GAME_JEDI_MODEL_CAPACITY];
    uint16_t maxForceLineLength[JPB_GAME_JEDI_MODEL_CAPACITY];
    char NumPlayers;
    int32_t mNumContinues;
    int8_t ContinuesUsed;
    CharacterData aCharacterData[JPB_GAME_CHARACTER_CAPACITY];
    JEDICOMBOMASK jediComboMask[JPB_GAME_JEDI_MODEL_CAPACITY];
    uint32_t secretBits;
    char AIDamage;
    char JediDamage;
    char HTHRate;
    char RangedRate;
    char BlockRate;
    char ComboLevel;
    char ForceLevel;
    char continueAble;
    char difficulty;
    int32_t gameCompleted;
    uint8_t players[2];
    uint8_t abGlobalBits[16];
    Upgrades jediUpgrades[9];
    uint16_t unlockedExtraCharacters;
} saveGameStruct;

typedef void (*JPBGameBarHook)(
    void *user_data,
    int x,
    int y,
    int width,
    int height,
    uint32_t color);

/* Exact matched-PC PDB type 0x9ADA. */
typedef struct gamestruct {
    int8_t Mode;
    int8_t gameMode;
    int32_t Continuing;
    int32_t StageExit;
    int32_t LevelExit;
    int32_t GameExit;
    int16_t ModelSelect[2];
    uint8_t AIselect[2];
    uint8_t CurrentLevel;
    uint8_t inMenuFlag;
    uint16_t xaFlag;
    uint16_t xaPause;
    uint16_t xaTimer;
    uint16_t xaClock;
    uint16_t xaLoop;
    uint16_t xaNum;
    uint16_t xaReloc;
    uint16_t xaVol;
    uint16_t xaPending;
    uint32_t xastartPos;
    uint32_t xaendPos;
    uint32_t xacurPos;
    int32_t AbortCount;
    uint32_t GameState;
    char QuickDraw;
    char screenShotFlag;
    char timerBars;
    char maxdraw;
    int32_t Counter;
    char message[256];
    int32_t TestTimerCount;
    char letterboxFlag;
    char letterboxFlag2;
    int16_t versusModeFlag;
    int16_t padFlag;
    uint16_t maxEnergyLevels[JPB_GAME_JEDI_MODEL_CAPACITY];
    uint16_t maxEnergyLineLength[JPB_GAME_JEDI_MODEL_CAPACITY];
    uint16_t maxForceLevels[JPB_GAME_JEDI_MODEL_CAPACITY];
    uint16_t maxForceLineLength[JPB_GAME_JEDI_MODEL_CAPACITY];
    char NumPlayers;
    int32_t mNumContinues;
    int8_t ContinuesUsed;
    CharacterData aCharacterData[JPB_GAME_CHARACTER_CAPACITY];
    JEDICOMBOMASK jediComboMask[JPB_GAME_JEDI_MODEL_CAPACITY];
    uint8_t
        jediLevelPlayed[JPB_GAME_JEDI_MODEL_CAPACITY][JPB_GAME_LEVEL_CAPACITY];
    uint32_t
        jediScorePerLevel[JPB_GAME_JEDI_MODEL_CAPACITY][JPB_GAME_LEVEL_CAPACITY];
    uint8_t checkpoint[JPB_GAME_CHECKPOINT_CAPACITY];
    char AIDamage;
    char JediDamage;
    char HTHRate;
    char RangedRate;
    char BlockRate;
    char ComboLevel;
    char ForceLevel;
    char continueAble;
    char difficulty;
    int32_t gameCompleted;
} gamestruct;

extern gamestruct GameStruct;
extern optionstruct OptionStruct;
extern optionstruct defaultOptionStruct;
extern int32_t gaButtonMap[5][6];
extern int32_t OMNIDIRECTIONAL_MOVEMENT;
extern int32_t gDeathCount;
extern int32_t gPilotDeathCount;
extern int32_t pilotsKilled;
extern int32_t tankID;
extern int32_t timeAdj;
extern uint32_t gGlobalTimer;
extern uint8_t nextLevel;
extern uint8_t camerablockactive;
extern uint8_t brightMax;
extern uint8_t oldworld;
extern uint8_t alwaysRun;
extern uint8_t dimScreen;
extern uint8_t streets;
extern uint32_t secretBits;
extern int32_t zerobss_levelReset;
extern int32_t zerobss_ResetBoss;
extern uint8_t abGlobalBits[16];
extern Upgrades jediUpgrades[9];
extern saveGameStruct SaveGameStruct;
extern int32_t numPlayers;
extern float deltaTime;
extern float currentTextAlpha;
extern float currentSpriteAlpha;
extern int32_t spriteBright;
extern int64_t textBright;
extern int32_t waitForInputReadAfterRefresh;
extern int32_t refreshHUDCounter;
extern uint16_t charStuff[10];

void game_CLR_GLOBALBIT(unsigned bit);
int game_GET_GLOBALBIT(unsigned bit);
void game_SET_GLOBALBIT(unsigned bit);

void jpb_GameSetBarHook(
    JPBGameBarHook hook, void *user_data);
void jpb_GameResetOverlayScbs(void);
void _AddBar(
    int x, int y, int width, int height, int32_t color);
void _AddLifeTile2D(
    struct playerObject *player,
    unsigned x,
    unsigned y,
    int alpha);
void UpdateBright(
    float targetTextBright,
    float targetSpriteBright);
void ApplySaveGameData(void);
void UpdateSaveGameStruct(void);
void game_DisplayOverlay(void);
void game_DrawBigNum(unsigned num, unsigned x, unsigned y);
void game_DrawItems(unsigned player);
void game_DrawScore(unsigned player);
char game_GetGameCounter(int index);
char game_GetGameMode(void);

/*
 * Inferred portable boundary around the five-byte difficulty copy performed
 * inside exact PDB procedure game_initPerLevel. Difficulty 0 is easy and 1
 * is normal; the original clamps authored level indexes above 15 to row 0.
 */
int jpb_game_ApplyLevelDifficulty(
    unsigned level, int difficulty);

int game_gGetEnergy(int player);
int game_gGetForce(int player);
int game_gGetItemCount(int player);
int game_gGetMaxEnergy(int player);
int game_gGetMaxForce(int player);
int game_gGetPowerLevel(int player);
int game_gGetPowerType(int player);
int game_gGetScaleEnergy(int player);
int game_gGetScaleForce(int player);
int game_gGetScaleMaxEnergy(int player);
int game_gGetScaleMaxForce(int player);
int game_gGetScore(int player);
uint32_t game_gClrGameFlags(uint32_t flag);
uint32_t game_gGetGameFlags(void);
uint32_t game_gToggleGameFlags(uint32_t flag);
void game_clearLetterBox(void);
void game_setLetterBox(void);
uint32_t game_getCombo(uint32_t jedi, uint32_t combo);
void game_disableCombo(uint32_t jedi, uint32_t combo);
void game_enableCombo(uint32_t jedi, uint32_t combo);
void game_initCombos(void);
void game_initEnergy(void);
void game_initPlayerStartCombos(uint32_t player);
void newGameGameInit(void);
void game_setAudioOptions(void);
void game_setControlsOptions(int player);
void game_setDefaultOptions(void);
uint32_t game_gIsGameFlags(uint32_t flag);
int game_gModEnergy(int player, int level);
int game_gModForce(int player, int level);
int game_gModItemCount(int player, int level);
char game_ModGameCounter(int amount);
int game_gModScore(int player, int level);
int game_gSetEnergy(int player, int level);
int game_gSetForce(int player, int level);
uint32_t game_gSetGameFlags(uint32_t flag);
int game_gSetItemCount(int player, int level);
void game_gSetMaxEnergy(int player, int level);
void game_gSetMaxForce(int player, int level);
int game_gSetPowerLevel(int player, int level);
int game_gSetPowerType(int player, int level);
int game_gSetScore(int player, int level);
void game_setFuncArray(void);

#if defined(__cplusplus)
#define JPB_GAME_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_GAME_STATIC_ASSERT(condition, message) \
    _Static_assert(condition, message)
#endif

JPB_GAME_STATIC_ASSERT(
    sizeof(optionstruct) == 56,
    "optionstruct must match PDB type 0x1061");
JPB_GAME_STATIC_ASSERT(
    offsetof(optionstruct, FunFactor) == 18,
    "optionstruct.FunFactor offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(optionstruct, ResolutionChanged) == 32,
    "optionstruct.ResolutionChanged offset changed");
JPB_GAME_STATIC_ASSERT(
    sizeof(CharacterData) == 28,
    "CharacterData must match PDB type 0x108C");
JPB_GAME_STATIC_ASSERT(
    offsetof(CharacterData, MaxEnergyPerc) == 8,
    "CharacterData.MaxEnergyPerc offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(CharacterData, PowerLevel) == 24,
    "CharacterData.PowerLevel offset changed");
JPB_GAME_STATIC_ASSERT(
    sizeof(JEDICOMBOMASK) == 6,
    "JEDICOMBOMASK must match PDB type 0x108A");
JPB_GAME_STATIC_ASSERT(
    sizeof(Upgrades) == 18,
    "Upgrades must match PDB type 0x1457");
JPB_GAME_STATIC_ASSERT(
    offsetof(Upgrades, forcePowers) == 4,
    "Upgrades.forcePowers layout changed");
JPB_GAME_STATIC_ASSERT(
    sizeof(saveGameStruct) == 4624,
    "saveGameStruct must match PDB type 0x6CE8");
JPB_GAME_STATIC_ASSERT(
    offsetof(saveGameStruct, jediLevelPlayed) == 54,
    "saveGameStruct.jediLevelPlayed offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(saveGameStruct, jediScorePerLevel) == 744,
    "saveGameStruct.jediScorePerLevel offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(saveGameStruct, maxEnergyLevels) == 3524,
    "saveGameStruct.maxEnergyLevels offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(saveGameStruct, aCharacterData) == 3720,
    "saveGameStruct.aCharacterData offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(saveGameStruct, jediComboMask) == 4280,
    "saveGameStruct.jediComboMask offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(saveGameStruct, secretBits) == 4420,
    "saveGameStruct.secretBits offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(saveGameStruct, jediUpgrades) == 4458,
    "saveGameStruct.jediUpgrades offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(saveGameStruct, unlockedExtraCharacters) == 4620,
    "saveGameStruct unlock mask offset changed");
JPB_GAME_STATIC_ASSERT(
    sizeof(gamestruct) == 4716,
    "gamestruct must match PDB type 0x9ADA");
JPB_GAME_STATIC_ASSERT(
    offsetof(gamestruct, GameState) == 64,
    "gamestruct.GameState offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(gamestruct, maxEnergyLineLength) == 388,
    "gamestruct.maxEnergyLineLength offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(gamestruct, aCharacterData) == 536,
    "gamestruct.aCharacterData offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(gamestruct, jediComboMask) == 1096,
    "gamestruct.jediComboMask offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(gamestruct, jediLevelPlayed) == 1234,
    "gamestruct.jediLevelPlayed offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(gamestruct, jediScorePerLevel) == 1924,
    "gamestruct.jediScorePerLevel offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(gamestruct, checkpoint) == 4684,
    "gamestruct.checkpoint offset changed");
JPB_GAME_STATIC_ASSERT(
    offsetof(gamestruct, gameCompleted) == 4712,
    "gamestruct.gameCompleted offset changed");

#undef JPB_GAME_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
