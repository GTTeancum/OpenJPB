#ifndef JPB_MENU_H
#define JPB_MENU_H

#include "jpb/fmath.h"
#include "jpb/game.h"
#include "jpb/whook.h"

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_ALL_TEXT_CAPACITY = 498,
    JPB_CREDIT_LINE_COUNT = 706,
    JPB_CONCEPT_ART_PAGE_COUNT = 42
};

/* Exact PDB type 0x6E44. The combo records are not yet decoded here. */
typedef struct MENUTEMPDATA {
    uint8_t comboList[96];
    uint32_t comboListCount;
    uint32_t jedi;
    uint8_t newcombos[48];
} MENUTEMPDATA;

/* Exact PDB type 0x132D. */
typedef struct SRECT {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} SRECT;

/* Exact PDB type 0x6EC4. */
typedef struct MDEF_MOD {
    uint16_t type;
    uint16_t incspeed;
    int32_t min;
    int32_t max;
    void *src;
    uint16_t text;
    uint8_t reserved26[6];
} MDEF_MOD;

/* Exact matched-PC PDB type 0x6F83. */
typedef struct RESOLUTION {
    int32_t width;
    int32_t height;
} RESOLUTION;

/* Exact PDB type 0x6E17. */
typedef struct MMVDEF {
    uint8_t *mmvSrc;
    uint16_t mmvPtr;
    uint16_t mmvCounter;
    int32_t mmvX;
    int32_t mmvY;
    int32_t mmvXvect;
    int32_t mmvYvect;
    uint16_t mmvIns;
    uint16_t reserved30;
    uint32_t *mmvMenu;
    uint32_t mmvMenuFlags;
    uint8_t state;
    uint8_t reserved45[3];
} MMVDEF;

/*
 * Exact named prefix and menu-stack region of PDB type MENUVARS (0x6DBE).
 * Later score/player-select records remain byte-preserved until their owners
 * are recovered. Code uses named fields, so native pointers may compact on a
 * future 32-bit target without leaking the matched x64 ABI into game logic.
 */
typedef struct MENUVARS {
    MENUTEMPDATA td;
    uint8_t titleLoaded;
    uint8_t mloadShift;
    uint8_t ingameMovies;
    uint8_t reserved155;
    uint16_t bgWidth;
    uint16_t bgHeight;
    uint8_t *memBGptr;
    uint8_t titleDispEnable;
    uint8_t menuID;
    uint8_t movieBG;
    uint8_t cardProtectFlag;
    uint8_t cardProtectFailedFlag;
    uint8_t memdebugFlag;
    uint8_t cardSelect;
    uint8_t cardSlotSelect;
    uint8_t cardTracker;
    uint8_t gameDebugMode;
    uint8_t gmiNum;
    uint8_t titleArt;
    uint8_t pplayers[2];
    uint8_t subplayers[2];
    uint16_t fcount;
    uint16_t mcount;
    uint8_t introPlayed;
    uint8_t loadSaveMode;
    uint8_t scoreMode;
    uint8_t scoreNextMode;
    uint8_t scoreCounter;
    uint8_t scoreCurrentPlayer;
    uint8_t scoreBeeper;
    uint8_t reserved195;
    uint32_t scoreScore;
    uint8_t bitSwap[2];
    uint16_t scoreoLevel;
    uint32_t scoreDst;
    uint8_t fadeupCounter;
    int8_t yoffset;
    uint8_t yflag;
    uint8_t reserved211;
    uint32_t pad[2];
    uint32_t oldpad[2];
    uint16_t itemSelect;
    uint16_t vramx;
    uint16_t vramy;
    uint16_t reserved234;
    int32_t dstSelector;
    SRECT selbox;
    uint8_t selCount;
    uint8_t artload;
    uint8_t artLevel;
    uint8_t reserved251;
    int16_t artloadPos;
    int8_t autoLoad;
    int8_t movieSelect;
    uint32_t gluon;
    uint8_t pSelect;
    uint8_t aibit;
    uint8_t controlFlags;
    uint8_t savePosSlot;
    uint8_t jediDebugCombo;
    uint8_t comboSelect;
    uint8_t pauseMenu;
    uint8_t dialogBox1;
    uint8_t tempLevmod;
    uint8_t reserved269;
    uint16_t menuMode[8];
    uint8_t mmSelect1[8];
    uint8_t mmSelect2[8];
    uint8_t reserved302[2];
    uint8_t *selectp;
    uint32_t menuModeSP;
    uint32_t mmTotal;
    uint32_t mmSubSet;
    uint32_t mmItalics;
    uint32_t mmBrackets;
    float mmItalicsScale;
    uint32_t mmTextType;
    uint32_t mmX;
    uint32_t mmY;
    uint8_t *specialString;
    uint16_t sfxVolume;
    uint16_t reserved362;
    uint32_t mmColorSelect;
    uint32_t mmColorNotSelect;
    uint32_t reserved372;
    uint32_t *mmSelectPtr;
    MMVDEF *mmvCurrentMenuControl;
    uint32_t mmFlags;
    uint8_t reserved396[4];
    MMVDEF mmv[6];
    uint8_t mmvCount;
    uint8_t sbit;
    uint8_t mmvTriggers[6];
    uint16_t mmvTriggerRemap;
    uint16_t frKeyBuff[16];
    uint16_t frKeyBuff2[16];
    uint16_t reserved762;
    uint32_t awards[2];
    uint32_t awardLevel[2];
    uint32_t bar_y;
    uint32_t bar_speed;
    uint32_t pointSeek;
    uint16_t sndtest;
    uint16_t textures;
    uint8_t pselectMode[24];
    uint8_t mp[36];
    uint8_t awardOrder[6];
    uint16_t reserved862;
    uint32_t maxAwardScore[3];
    uint8_t awardSet[72];
    uint8_t currentAward;
    uint8_t ultimate;
    uint8_t holdButtFlag;
    uint8_t trainingLevel;
    int32_t controlPlayer;
    int32_t vmuFlag;
    uint8_t obiLegacySaberColor;
    uint8_t quiLegacySaberColor;
    uint8_t maceLegacySaberColor;
    uint8_t adiLegacySaberColor;
    uint8_t ploLegacySaberColor;
    uint8_t kiLegacySaberColor;
    uint8_t reserved966[2];
    int32_t mmAnchorType;
    float textScale;
    float textSpacer;
    uint8_t reserved980[4];
} MENUVARS;

_Static_assert(
    offsetof(MENUVARS, menuMode) == 0x10e,
    "MENUVARS menu stack offset must match matched-PC PDB");
_Static_assert(
    offsetof(MENUVARS, menuModeSP) == 0x138,
    "MENUVARS menu stack pointer offset must match matched-PC PDB");

/* Exact PDB global, matched-PC RVA 0x10DEBC0. */
extern wchar_t *allText[JPB_ALL_TEXT_CAPACITY];
/* Exact PDB globals at matched-PC RVAs 0x539DC0 and 0x987D88. */
extern unsigned loadScreenFlag;
extern unsigned loadTotal;
extern MENUVARS menuVars;
extern unsigned menuTexLoaded;
extern unsigned menuTexLoaded2;
extern _Material *menuTextures[249];
extern _Material *controlTextures[10];
extern _Material *kbmTextures[10];
extern float iconScaleOverride;
extern _Material *ps4Textures[10];
extern _Material *ps5Textures[10];
extern _Material *switchTextures[10];
extern _Material *switchProTextures[10];
extern _Material *joyconTextures[10];
extern _Material *xsxTextures[10];
extern _Material *kbmForceTextures[4];
enum { JPB_MENU_TEXTURE_ENTRY_COUNT = 132 };
/*
 * Exact 16-byte initialized record used by menu_winLoadTextures. The PDB
 * retained menuTextureList's extent but not this record's source spelling.
 */
typedef struct JPBMenuTextureEntry {
    const char *filename;
    uint16_t textureIndex;
    uint16_t spriteIndex;
    uint32_t legacyFlags;
} JPBMenuTextureEntry;
extern const JPBMenuTextureEntry
    menuTextureList[JPB_MENU_TEXTURE_ENTRY_COUNT];
extern unsigned char padShockable;
extern unsigned char modisorder2[23];
extern CVECTOR gColor;
extern float gLeft;
extern float gRight;
extern float gTop;
extern float gBottom;
extern SCREENRECT gDST;
extern unsigned screenSaverCount;
extern unsigned screenSaverFlag;
extern unsigned saverAlpha;
extern unsigned saverPads[2];
extern float menuTextDepthOverride;
/* Exact PDB global at matched-PC RVA 0x539590. */
extern unsigned slider;
extern unsigned char keyboardBufferIndex;
extern unsigned char keyboardKeyPressed;
extern unsigned char keyboardBuffer[10];
extern int savedNumPlayer;
extern int m_canShowRegisterGame;
extern int tempPlayersVs;
extern int newMenu_errorState;
extern int newMenu_trainLevel;
extern int newMenu_currentModelSelectNGPP1;
extern int newMenu_currentModelSelectBaseP2;
extern int newMenu_currentModelSelectNGPP2;
extern int newMenu_state;
extern int newMenu_bAbortMenu;
extern uint32_t newMenu_select;
extern int newMenu_playerSelectTypeP1;
extern int newMenu_currentModelSelectBaseP1;
extern int newMenu_playerSelectTypeP2;

/* Exact initialized title-menu definition streams from matched menu.obj. */
extern uint32_t xmainMdef[43];
extern uint32_t mainMdef[65];
extern uint32_t mainMdefNoRegisterGame[57];
extern uint32_t NOLOADmainMdef[24];
extern uint32_t PSXmainMdef[51];
extern uint32_t continuemainMdef[73];
extern uint32_t continuemainMdefNoRegisterGame[65];
extern uint32_t NOLOADmainMdef2[24];
extern uint32_t PSXmainMdef2[51];
extern uint32_t continueNOLOADmainMdef[30];
extern uint32_t titlePlayerCountMdef[21];
extern uint32_t titlePlayerCountContinueMdef[21];
extern uint32_t titlePlayerCountVSMdef[21];
extern uint32_t difficultyMdef[21];
extern uint32_t playerCountSelectMdef[20];
extern uint32_t newgameconfirmMdef[29];
extern uint32_t optionsMdef[44];
extern uint32_t controlsMdef[16];
extern uint32_t titlecontrolsMdef[16];
extern uint32_t controls1Mdef[42];
extern uint32_t controls2Mdef[42];
extern uint32_t controlSubDraw[8];
extern unsigned char ClassicControlScheme[7];
extern unsigned char ModernControlScheme[7];
extern unsigned char ClassicControlSchemeForce[7];
extern unsigned char ModernControlSchemeForce[7];
extern uint16_t controlTextList[8];
extern uint16_t controlTextListForce[6];
extern uint32_t languageMdef[25];
extern uint32_t videoMdef[34];
extern uint32_t audioMdef[44];
extern uint32_t audioMdef_Game[44];
extern uint32_t audioMusicMdef[46];
extern uint32_t rusureQuitMenuMdef[25];
extern uint32_t gameoverMdef[26];
extern uint32_t startMdef[4];
extern uint32_t exitSelectMdef[23];
extern uint32_t exitSelectMdef2[19];
/* Exact two-word exit stream used after menu_drawCredits. */
extern uint32_t creditsMdef[2];
/* Exact level-selector command stream at matched-PC RVA 0x4C6CF0. */
extern uint32_t levelSelectMdef[15];
extern uint8_t frameTopMover[24];
extern uint8_t frameBottomMover[24];
extern uint8_t frameLeftMover[24];
extern uint8_t frameRightMover[24];
extern uint32_t mmsizes[75];
extern RESOLUTION g_resolutions[256];
extern int32_t g_resolutionsCount;
extern MDEF_MOD modVars[74];

/* Exact initialized PDB globals owned by the two bespoke presentation paths. */
extern unsigned char *theCredits[JPB_CREDIT_LINE_COUNT];
extern unsigned char credMusic[8];
extern unsigned char credMuse;
extern int cachedInputL;
extern int cachedInputR;
extern float creditBarPosition;
extern int skipCreditForFrame;

/*
 * Dependency-free publication seam for the still-pending menu renderer.
 * The callback is reconstruction infrastructure; menu_specialMess and mess
 * are exact PDB names.
 */
typedef void (*JPBMenuSpecialMessageHook)(
    const uint8_t *mess,
    uint16_t message_menu,
    uint16_t response_menu,
    void *user_data);
typedef void (*JPBMenuCheatAction)(void);
typedef void (*JPBMenuP1CharacterSelectDrawHook)(
    int model,
    uint32_t pad,
    int exit_phase,
    void *user_data);
typedef void (*JPBMenuP2CharacterSelectDrawHook)(
    int player_one_model,
    int player_two_model,
    uint32_t player_one_pad,
    uint32_t player_two_pad,
    int is_versus,
    void *user_data);

typedef struct JPBMenuPlatformHooks {
    void (*initInput)(void *user_data);
    void (*loadTextures)(void *user_data);
    void (*initBucket)(void *user_data);
    void (*closeBucketLog)(void *user_data);
    int (*assignBackToP1)(void *user_data);
    int (*activateItem)(
        uint32_t destination, void *user_data);
    void (*menuSound)(unsigned sound, void *user_data);
    const uint8_t *(*keyboardState)(
        size_t *key_count, void *user_data);
    void (*triggerMovie)(
        unsigned movie, int flags, void *user_data);
    void (*cleanupLevelData)(void *user_data);
    void (*saveGameData)(void *user_data);
    void (*saveSettingsData)(
        const optionstruct *options, void *user_data);
    void (*setInMenu)(int in_menu, void *user_data);
    void (*scanLevel)(unsigned level, void *user_data);
    void (*soundCue)(const char *name, void *user_data);
    void (*refreshLevelTransforms)(void *user_data);
    void (*applyResolution)(
        unsigned resolution_index,
        unsigned window_mode,
        uint32_t *width,
        uint32_t *height,
        void *user_data);
    unsigned (*controllerCount)(void *user_data);
    const char *(*controllerName)(
        unsigned player, void *user_data);
    void (*singleControllerFallback)(void *user_data);
    void (*openUrl)(const char *url, void *user_data);
    void (*requestExit)(void *user_data);
} JPBMenuPlatformHooks;

void jpb_MenuSetSpecialMessageHook(
    JPBMenuSpecialMessageHook hook,
    void *user_data);
void jpb_MenuSetP1CharacterSelectDrawHook(
    JPBMenuP1CharacterSelectDrawHook hook,
    void *user_data);
void jpb_MenuSetP2CharacterSelectDrawHook(
    JPBMenuP2CharacterSelectDrawHook hook,
    void *user_data);
void jpb_MenuSetPlatformHooks(
    const JPBMenuPlatformHooks *hooks,
    void *user_data);
void menu_addTotal(unsigned amount);
void menu_specialMess(uint8_t *mess);
void SetGlobalColorDefault(void);
void SetGlobalDST(void);
void clearMenuStuff(void);
int menu_controlCK(void);
int menu_ultimate(void);
int menu_concept(void);
int menu_healthCK(int unused);
int menu_forceCK(int unused);
int menu_changeSaberCheck(int64_t unused);
int menu_playerSelectCheck(int64_t menu_definition);
void menu_ClearScreenSaver(void);
void menuConceptMenu(void);
void menu_drawCredits(void);
void menu_initCredits(void);
void menu_initNewMenu(void);
int menu_NOLOAD(void);
unsigned menu_checkSoftReset(unsigned unused);
void menu_checkTitleReload(void);
void menu_cameraChange(unsigned view_type);
void menu_checkMiniMod(unsigned level, unsigned decreased);
void menu_continueGame(void);
void menu_endGame(void);
void menu_enterTitleMode(void);
void menu_mainExitMenu(void);
void menu_mainInitMenu(unsigned mode);
void menu_mainLoop(void);
void menu_winLoadTextures(void);
void drawControlsIcon(void);
int getControllerTextures(int player, _Material **materialHandle);
void newDrawControllerIcon(
    int icon,
    float control_icon_scale,
    int x,
    int y,
    int alpha,
    int player);
void runControlsMenu(void);
void menu_mainMenu(uint32_t *mdef);
void menu_slideco(
    float width,
    float height,
    int x,
    int y,
    float current,
    float maximum);
void newMenu_DrawArrows(
    uint32_t pad,
    int left_x,
    int left_y,
    int right_x,
    int right_y);
int newMenu_P1CharacterSelect(void);
int newMenu_P2CharacterSelect(int isVS);
int newMenu_Training(void);
void menu_menuExit(void);
void menu_menuMusic(unsigned track, unsigned loop);
unsigned menu_handleMenuTriggers(int destination);
void menu_levelSelect(void);
void menu_drawLevelSelectScreen(unsigned interactive);
void menu_drawReconnect(void);
void menu_initLevelSelectScreen(void);
void menu_levelSelectMenu(uint32_t *mdef);
void menu_restartLevel(void);
void menu_sound(unsigned sound);
void menu_pushMenu(unsigned menu_id);
void menu_popMenu(void);
void menu_readControl(void);
void menu_rotControls(void);
void menu_resetMemcardFlags(void);
void menu_resetSaveMenu(void);
void menu_saveGame(void);
void menu_saveMCARDError(void);
void menu_saveMCARDSelect(void);
void menu_scanAllLevels(void);
void menu_scanProtection(void);
void menu_setDrawSurface(unsigned surface);
void menu_setScoreMode(unsigned mode, unsigned index);
void menu_triggerMovie(unsigned movie);
void menu_handleObjectiveMessage(void);
void menu_initGameover(void);
void menu_initPlayerSelect(void);
void menu_setNumPlayers(unsigned num_players);
void menu_setPlayer(unsigned player, unsigned model);
void menu_setPointSeek(unsigned point_seek);
void menu_setShockOption(unsigned player);
void menu_startAcceptDecline(unsigned mask, unsigned replacement);
void menu_startTraining(unsigned training_level);
void menu_tempClearTrigger(void);
void updatePlayerSelectIndex(int player);
int cheatCheck(
    unsigned short *cheatList,
    unsigned cheatSize,
    JPBMenuCheatAction cheatJump);
int cheatCheckKeyboard(
    unsigned char *stateList,
    int cheatLength,
    JPBMenuCheatAction cheatJump);
void checkKeyboardBuffer(void);
void mmDecVar(uint32_t *md);
void mmIncVar(uint32_t *md);
void mmUpdateModSet(uint32_t *md, unsigned value, unsigned decreased);
unsigned mmNextCode(uint32_t *md, unsigned index);
unsigned mmGetModVal(MDEF_MOD *mod);
int mmSetModVal(MDEF_MOD *mod, unsigned value, unsigned mod_index);
int mmDrawItem(uint32_t *md, uint8_t *dstbuffer);
void mmDrawMod(uint32_t *md, uint8_t *dstbuffer);
unsigned mmDrawsub(uint32_t *md, unsigned index);
void mmDraw(uint32_t *md);
int menu_setCanShowRegisterGame(int can_show);
int menu_drawBigNum(
    unsigned num,
    unsigned x,
    unsigned y,
    unsigned r,
    unsigned g,
    unsigned b);
void menu_drawBigNums(
    unsigned num,
    unsigned len,
    unsigned x,
    unsigned y,
    unsigned r,
    unsigned g,
    unsigned b);

#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(MENUTEMPDATA) == 152, "MENUTEMPDATA PDB size changed");
_Static_assert(sizeof(MDEF_MOD) == 32, "MDEF_MOD PDB size changed");
_Static_assert(sizeof(MMVDEF) == 48, "MMVDEF PDB size changed");
_Static_assert(offsetof(MENUVARS, titleDispEnable) == 168, "MENUVARS title prefix changed");
_Static_assert(offsetof(MENUVARS, subplayers) == 182, "MENUVARS character selection changed");
_Static_assert(offsetof(MENUVARS, loadSaveMode) == 189, "MENUVARS load/save mode changed");
_Static_assert(offsetof(MENUVARS, menuMode) == 270, "MENUVARS menu stack changed");
_Static_assert(offsetof(MENUVARS, menuModeSP) == 312, "MENUVARS stack pointer changed");
_Static_assert(offsetof(MENUVARS, mmvTriggers) == 690, "MENUVARS triggers changed");
_Static_assert(offsetof(MENUVARS, mmv) == 400, "MENUVARS MMV controls changed");
_Static_assert(offsetof(MENUVARS, frKeyBuff) == 698, "MENUVARS key buffer changed");
_Static_assert(offsetof(MENUVARS, pointSeek) == 788, "MENUVARS point seek changed");
_Static_assert(sizeof(MENUVARS) == 984, "MENUVARS PDB size changed");
#endif

#ifdef __cplusplus
}
#endif

#endif
