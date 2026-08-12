/*
 * Linked PDB global at RVA 0x4AFDA0. The WorldData object itself is supplied
 * by the later game/bootstrap layer; chunk loaders operate through this
 * original pointer.
 */

#include "jpb/world.h"
#include "jpb/bullet.h"
#include "jpb/camera.h"
#include "jpb/combo.h"
#include "jpb/cube.h"
#include "jpb/effects.h"
#include "jpb/fx.h"
#include "jpb/jpx.h"
#include "jpb/jonny.h"
#include "jpb/menu.h"
#include "jpb/input.h"
#include "jpb/game.h"
#include "jpb/player.h"

Camera gCamera;
VECTOR streetcampos;
/* Exact PDB boss global at matched-PC RVA 0x4F1280. */
int32_t isTatoMaul;
/* Exact PDB boss/camera focus global at matched-PC RVA 0x4F1288. */
_svector gJarJarPos;
/* Exact PDB global at matched-PC RVA 0x4F1A98. */
int newcameraflag;
VECTOR uberPos;
int uberXRange;
int uberZRange;
/* Exact PDB global at matched-PC RVA 0x4F1AA4. */
int mCameraAngleDest;
/* Exact PDB global at matched-PC RVA 0x4F1AC0. */
int uberLock;
int gGlobalFrameRate = 2048;
float fGlobalFrameRate = 0.5f;
float framerate = 0.5f;
/* Exact initialized PDB frame-delta global at matched-PC RVA 0x4BECB8. */
float deltaTime = 0.01666f;
/* Exact PDB global at matched-PC RVA 0x547EB0. */
float scaleAdjustment;
/* Exact PDB global at matched-PC RVA 0x547EB4. */
float scaleAdjustmentMM;
FVECTOR globalgravity = {0.0f, -11.0f, 0.0f};
int screenshake;
int screenshakeamplitude;
/* Exact PDB camera publication globals at RVAs 0x10DE668..0x10DE68F. */
int cameraYaw;
_svector cameraFacing;
_svector cameraLocation;
VECTOR cameraposition;
int mDrawingSurfaceId;
/* Exact PDB global and type at matched-PC RVA 0x547B50. */
sceneRoot gSceneRoot;
/* Exact PDB global at matched-PC RVA 0x515C08. */
MATRIX CameraMatrix;
/* Exact PDB loading-screen globals at RVAs 0x539DC0 and 0x987D88. */
unsigned loadScreenFlag;
unsigned loadTotal;
/* Exact PDB globals at matched-PC RVAs 0x10DBF80 and 0x515C00. */
MATRIX twattedcameramatrix;
MATRIX *worldTURTLEMatrix;
int32_t gSCENE_READY;
/* Exact PDB global at matched-PC RVA 0x547B38. */
int32_t gSTROBE_MODE;
/* Exact PDB global at matched-PC RVA 0x547B30. */
int32_t gCurrentSceneObject;
/* Exact PDB global at matched-PC RVA 0x547B40. */
int32_t playeronscreen[2];
/* Exact PDB globals at matched-PC RVAs 0x10D7E30 and 0x10D7E34. */
int32_t globaltimer;
int32_t screenworldpos;
/* Exact initialized PDB global at matched-PC RVA 0x4CC09C. */
CVECTOR mStrobe;
/*
 * Exact PDB globals at matched-PC RVAs 0x946000, 0x946060, and 0x547B3C.
 * The neighboring global addresses prove both frustum arrays contain six
 * 16-byte FVECTOR4 records.
 */
FVECTOR4 collisionfrustrum[6];
FVECTOR4 clippingfrustrum[6];
uint8_t initialLevelPauseDelay;
/* Inferred name for the anonymous matched-PC byte at RVA 0x10DB3A4. */
uint8_t jpb_StreetsEndingShortCollisionTimeout;
VECTOR v3Translate;
MATRIX gGTEMATRIX;
uint32_t padMaskBits[JPB_INPUT_PAD_COUNT];
PADLOAD padCurrentBits[JPB_INPUT_PAD_COUNT];
uint8_t padExist;
/* Exact PDB frontend connection flag at matched-PC RVA 0x92DA8C. */
int32_t p1Disconnected;
/* Exact PDB frontend connection flag at matched-PC RVA 0x92DAC8. */
int32_t p2Disconnected;
/* Exact PDB global at matched-PC RVA 0x5380F0. */
uint32_t secretBits;
int32_t nShockers[2];
/* Exact PDB globals used by brain_ControlPlayer's analog direction path. */
SDL_InputType player1InputType;
SDL_InputType player2InputType;
/* Exact PDB global at matched-PC RVA 0x4D49AC. Zero selects KBM art. */
int32_t lastUsedInputType;
float g_p1X;
float g_p1Y;
float g_p2X;
float g_p2Y;
WorldData *gpWorld;
char *jonnylevel;
int32_t *leveldata;
void *leveltexture;
int32_t *texturebase;
int32_t *colorbase;
int32_t *vertbase;
int32_t mapyend;
char LevelSelect;
/* Exact PDB global at matched-PC RVA 0x4BA9F0. */
int32_t gHidePikobisModel;
/* Exact PDB globals at matched-PC RVAs 0x537DF0 and 0x537DF4. */
int32_t zerobss_levelReset;
int32_t zerobss_ResetBoss;
/* Exact PDB global at matched-PC RVA 0x537D84. */
int32_t zpush;
/* Exact PDB globals at matched-PC RVAs 0x537DF8 and 0x53D320..0x53D33C. */
int32_t corusPoints[2];
_svector reStartPos[2];
uint32_t reStartScore[2];
int32_t reStartCounter;
int32_t gCheckPoint;
/* Direct PDB globals at RVAs 0x10DA140 and 0x10DA100. */
gamestruct GameStruct;
optionstruct OptionStruct;
/* Exact PDB global at matched-PC RVA 0x10D8EE0. */
saveGameStruct SaveGameStruct;
/*
 * Exact initialized PDB global at matched-PC RVA 0x4BAB98. The named
 * option procedures copy either the whole record or their owned fields.
 */
optionstruct defaultOptionStruct = {
    .saveFileVer = 0,
    .CPULevel = 0,
    .Stereo = 1,
    .Music = 1,
    .SFX = 1,
    .Extra = 0,
    .AutoSave = 1,
    .musicVolume = 30,
    .SFXVolume = 30,
    .xaTrack = 0,
    .oldModelSelect = {0, 1},
    .ControllerConfig = {1, 1},
    .AIDebug = 0,
    .FunFactor = 4,
    .WalkLimit = {2, 2},
    .RunLimit = {8, 8},
    .ShockFlag = {1, 1},
    .JumpCheat = 1,
    .DebugLevel = 0,
    .overlayMode = 1,
    .Language = 0,
    .ResolutionChanged = 0,
    .ScreenWidth = 1920,
    .ScreenHeight = 1080,
    .WindowMode = 0,
    .PadAudioEnabled = 1,
    .EULAaccepted = 0
};
/* Exact PDB global at matched-PC RVA 0x10DEBC0, type wchar_t*[498]. */
wchar_t *allText[JPB_ALL_TEXT_CAPACITY];
/* Exact BSS global at matched-PC RVA 0x538100. */
Upgrades jediUpgrades[9];
/*
 * Exact initialized PDB global at matched-PC RVA 0x4BAA00.
 * Type 0x1063 is int[5][6].
 */
int32_t gaButtonMap[5][6] = {
    {1024, 8, 32, 4, 4, 64},
    {1024, 8, 64, 4, 1, 64},
    {1024, 2, 4, 32, 8, 64},
    {1024, 2, 8, 32, 4, 64},
    {1024, 2, 4, 8, 32, 64},
};
/* Exact PDB global at matched-PC RVA 0x4F2F70. */
int16_t comboTally[2][32];
/*
 * Inferred name for the anonymous matched-PC words at RVA 0x4F2F60.
 * The following named comboTally global proves the four-entry extent.
 */
int32_t jpb_comboAwardHitThreshold[
    JPB_COMBO_AWARD_THRESHOLD_CAPACITY];
int16_t gaPoints[88] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    10, 0, 0, 50, -250, 100, 100, 350, 350, 350,
    350, 50, 250, 0, 300, 350, 350, 0, 800, 0,
    150, 50, 1000, 500, 150, 200, 100, 150, 1200,
    1400, 1400, 2500, -10, -10, -10, 1000, 100,
    150, 100, 1600, 150, 100, 100, 150, 200, 100,
    0, 250, 250, 200, 150, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 200, 150, 0, 0, 1200, 100, 0, 0,
    0, 0, 0, 0, 0, 0, 0
};
/* Exact PDB global at matched-PC RVA 0x538BA0. */
int32_t maModelID[80][3];
/* Exact PDB global at matched-PC RVA 0x4A65A4. */
int32_t OMNIDIRECTIONAL_MOVEMENT;
int32_t gDeathCount;
int32_t gPilotDeathCount;
int32_t pilotsKilled;
/* Exact PDB global at matched-PC RVA 0x4B8AFC. */
int32_t tankID = -1;
/* Exact initialized PDB global at matched-PC RVA 0x4B8AF8. */
int32_t timeAdj = 1;
uint32_t gGlobalTimer;
/* Exact PDB global at matched-PC RVA 0x5395A7. */
uint8_t nextLevel;
/* Exact byte globals referenced by the initialized menu modifier table. */
uint8_t camerablockactive;
uint8_t brightMax;
uint8_t oldworld;
uint8_t alwaysRun;
uint8_t dimScreen;
uint8_t streets;
/* Exact PDB global at matched-PC RVA 0x51D5D0. */
int32_t moveTaxi;
/* Exact PDB globals at matched-PC RVAs 0x10DBF10..0x10DBF2B. */
int32_t tele;
int32_t trange;
VECTOR toff;
/* Exact PDB global at matched-PC RVA 0x51D5BC. */
int32_t tflag;
/* Exact PDB global at matched-PC RVA 0x10DEBA0. */
ProjType *projType;
Projectile maProjectile[JPB_PROJECTILE_GLOBAL_CAPACITY];
char terminatedSound[9];
/* Exact PDB globals at matched-PC RVAs 0x10DBEF0 and 0x10DBF00/0x10DBF30. */
uint8_t abGlobalBits[16];
VECTOR savedPlayerPos;
VECTOR tpos;
/*
 * Direct PDB globals. tankdrivers has an unsized PDB array type; hurtplayer's
 * machine code proves the two driver slots paired with timesincetank[2].
 */
int32_t timesincetank[2];
/* Exact PDB global at matched-PC RVA 0x51D5C8. */
int32_t jumpheld[2];
int32_t playertankindex;
playerObject *tankdrivers[2];
int32_t totalframes;
/* Exact PDB global at matched-PC RVA 0x508584. */
int32_t streets_reached_stairs;
/*
 * Exact PDB globals. stapbikeindex has an unsized PDB array declaration;
 * CheckCubeBlocking proves the two player slots at RVAs 0x582420/0x582424.
 */
int32_t stapbikeindex[2];
uint16_t stapsound;
uint16_t tanknoise;
uint16_t turretnoise;
/* Exact PDB global at matched-PC RVA 0x53A5E8. */
playerObject *afterLife;
/* Inferred name for the callback-table slot at matched-PC RVA 0x10EFBB0. */
JPBPlayerCallback jpb_TrajectoryCallbackSlot;
/* Inferred companion slot at RVA 0x10EFD00. */
JPBPlayerCallback jpb_MaulTrajectoryCallbackSlot;
/* Inferred name for the anonymous flag word at matched-PC RVA 0x10DBEFC. */
uint32_t jpb_CubeRuntimeFlags;
_Alignas(16) uint8_t gaScratch[2048];
/*
 * The matched pointers span the anonymous 8192-byte event buffer at
 * RVA 0x10DC200. The jpb_ storage name is inferred; the three public pointer
 * names and their initial values are exact PDB/executable evidence.
 */
static int32_t jpb_eventlist_storage[2048];
int32_t *eventlist_start = jpb_eventlist_storage;
int32_t *eventlist_next = jpb_eventlist_storage;
int32_t *eventlist_end = jpb_eventlist_storage + 2048;
int32_t *WorldmeshData;
size_t gJpxWorldmeshSize;
_Alignas(2) uint8_t maProjTypes[JPB_PROJECT_TYPE_BYTES];
uint8_t aEmiter[JPB_EMITTER_BYTES];
EffectHeader *paEffects[JPB_EFFECT_COUNT];
_plasma_zapvars jpb_PlasmaZapVars[JPB_PLASMA_ZAP_CHANNELS];
int gMaxEffect;
_Material *effects1Handle[JPB_RESIDENT_SPRITE_COUNT];
void *addHandle;
void *transHandle;

/* Exact initialized global at matched-PC RVA 0x4B0BD0. */
int32_t cullmesh[JPB_CULL_MESH_COUNT] = {1};
/* Exact PDB globals at RVAs 0x10DE200, 0x508588, and 0x508589. */
JPBCullState cull;
char tato_wallfrigflag;
char fed_wallfrigflag;

#define JPB_MAP_EVENT_ROW \
    {0, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, \
     0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0}
#define JPB_MAP_EVENT_ROW_LEVEL_4 \
    {0, 0x0e1e, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, \
     0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0x0e0a, 0}

/*
 * Exact 30-by-15 unsigned-short event table at RVA 0x4CB490. Row zero is
 * empty, level four has its one distinct effect ID, and all other rows share
 * the executable's common mapping.
 */
uint16_t eventarray[30][15] = {
    {0},
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW_LEVEL_4,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW,
    JPB_MAP_EVENT_ROW
};

#undef JPB_MAP_EVENT_ROW_LEVEL_4
#undef JPB_MAP_EVENT_ROW

/* Exact 16-entry PDB pointer table at matched-PC RVA 0x4CB820. */
char *maphitsounds[16] = {
    "explosm",
    "break1",
    "break2",
    "walbreak",
    "lgspark",
    "explomed",
    "glasbrk1",
    "trap",
    "walbreak",
    "brk_pole",
    "trap",
    "beam_on",
    "lasrgate",
    "forcefld1",
    "explosm",
    "sabrhit7"
};
