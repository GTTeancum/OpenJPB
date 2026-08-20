#ifndef JPB_PLAYER_H
#define JPB_PLAYER_H

#include "jpb/objroot.h"
#include "jpb/vectors.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_PLAYER_CAPACITY = 20,
    JPB_PLAYER_MOTION_NAME_BYTES = 32,
    JPB_PLAYER_HELD_SLOTS = 16,
    JPB_PLAYER_CALLBACK_CAPACITY = 50
};

/*
 * Descriptive IDs established by MovePlayer's executable branches and
 * diagnostic strings; the PDB does not provide an enum for these values.
 */
enum {
    JPB_PLAYER_ID_DESERT_BEAST = 0x28,
    JPB_PLAYER_ID_WORM = 0x4d
};

typedef struct Pad {
    uint32_t padnum;
    uint32_t mask0;
    uint32_t mask1;
    uint32_t oldbits0;
    uint32_t oldbits1;
    uint32_t bufferedbits;
    uint32_t cpad[2];
} Pad;

typedef struct _mvector {
    int16_t vx;
    int16_t vy;
    int16_t vz;
    int16_t speed;
} _mvector;

typedef struct playerSettings {
    int16_t JumpVel;
    int16_t RunningJumpVel;
    int16_t dblJumpVel;
    int16_t JumpAngle;
    int16_t RunningJumpAngle;
    int16_t dblJumpAngle;
    int16_t bkJumpAngle;
    uint16_t gravity;
    uint16_t dblgravity;
    int16_t minClosingDist;
} playerSettings;

/* Exact matched-PC PDB type 0x119F. */
typedef struct ProjType {
    int8_t range;
    uint8_t radius;
    uint8_t length;
    uint8_t width;
    int8_t muzzelEffect;
    int8_t bulletEffect;
    int8_t hitEffect;
    int8_t removeEffect;
    int8_t rangeEffect;
    int8_t bulletSprite;
    char fireSound[8];
    char hitSound[8];
    int8_t speed;
    uint8_t hitReact;
    uint8_t damage;
    uint8_t bulletFXRate;
    uint16_t flag;
    uint8_t clut;
    uint8_t pad;
} ProjType;
typedef struct Motion Motion;
typedef struct Combo Combo;
typedef struct aiData aiData;
typedef struct wsl_ENEMY wsl_ENEMY;
typedef struct playerObject playerObject;
typedef struct _Material _Material;
typedef int (*JPBPlayerCallback)(int32_t *, playerObject *);
typedef int (*JPBPlayerInitCallback)(playerObject *);

typedef enum JPBPlayerProcessPhase {
    JPB_PLAYER_PROCESS_BEFORE_CONTROL = 0,
    JPB_PLAYER_PROCESS_AFTER_CONTROL = 1
} JPBPlayerProcessPhase;

/*
 * Portable observation seam around the exact per-actor control call. It is
 * diagnostic only and cannot replace or suppress original gameplay work.
 */
typedef void (*JPBPlayerProcessObserver)(
    JPBPlayerProcessPhase phase,
    int index,
    playerObject *player,
    int AI_ON,
    const int32_t *cpad,
    void *user_data);

typedef struct JPBPlayerFrameProfile {
    double lastTotalSeconds;
    double lastCollisionsSeconds;
    double lastGlobalBitsSeconds;
    double lastMapTriggersSeconds;
    double lastLifeTileSeconds;
    double lastDebugSeconds;
    double lastInputSeconds;
    double lastDamageTrackerSeconds;
    double lastPauseSeconds;
    double lastControlSeconds;
    double maxTotalSeconds;
    double maxCollisionsSeconds;
    double maxGlobalBitsSeconds;
    double maxMapTriggersSeconds;
    double maxLifeTileSeconds;
    double maxDebugSeconds;
    double maxInputSeconds;
    double maxDamageTrackerSeconds;
    double maxPauseSeconds;
    double maxControlSeconds;
    uint32_t lastActivePlayers;
    uint32_t maxActivePlayers;
    int32_t maxControlPlayerIndex;
    int32_t maxControlPlayerId;
} JPBPlayerFrameProfile;

/*
 * Portable realization seam for exact PDB procedure _DrawTile. Geometry is
 * still authored and scheduled by player.c; a platform renderer consumes
 * the camera-space solid rectangle after the original owner emits it.
 */
typedef void (*JPBPlayerTileHook)(
    void *user_data,
    const FVECTOR *position,
    float width,
    float height,
    uint32_t color,
    float projection_depth);

/*
 * Exact PDB global `funcArray` at matched-PC RVA 0x10EFB80. The executable
 * installs every nonzero player callback; slot zero is intentionally NULL.
 */
extern JPBPlayerCallback
    funcArray[JPB_PLAYER_CALLBACK_CAPACITY];

/*
 * Inferred name for the callback-table slot at matched-PC RVA 0x10EFBB0.
 * game_InitGameSystems initializes it to exact brainutil_PlotTrajectory;
 * keeping the slot explicit also preserves the original extension boundary.
 */
extern JPBPlayerCallback jpb_TrajectoryCallbackSlot;
/*
 * Inferred name for the companion callback-table slot at matched-PC RVA
 * 0x10EFD00. game_InitGameSystems initializes it to the exact PDB procedure
 * brainutil_PlotMaulTrajectory.
 */
extern JPBPlayerCallback jpb_MaulTrajectoryCallbackSlot;

typedef struct CollisionData {
    int16_t radius1;
    int8_t id;
    int8_t parentid;
} CollisionData;

/*
 * Exact PDB globals used by MovePlayer's large-character node contacts.
 * PDB type 0x8F6D is an incomplete CollisionData[] declaration.
 */
extern CollisionData maDesert_BNodeSizes[];
extern CollisionData maWormNodeSizes[];
extern CollisionData maGunganNodeSizes[19];
extern CollisionData maGunchiefNodeSizes[19];

/*
 * Exact matched-PC PDB type 0x128D. Native pointers intentionally compact
 * this runtime-only structure on the later 32-bit Xbox target.
 */
struct playerObject {
    objectRoot playerRoot;
    playerObject *target;
    playerObject *locked;
    Pad playerPad;
    char PreMotion[JPB_PLAYER_MOTION_NAME_BYTES];
    char HeldMotion[JPB_PLAYER_MOTION_NAME_BYTES];
    int16_t playernum;
    int16_t playerID;
    int32_t FacingLR;
    int32_t ctime;
    uint32_t bheld[JPB_PLAYER_HELD_SLOTS];
    uint32_t heldMask;
    uint32_t releaseMask;
    int32_t dtime;
    int32_t vtime;
    int32_t mtime;
    int16_t chainSlack;
    int16_t chainSlackEnd;
    int32_t fLife;
    int32_t fStun;
    int32_t fForce;
    VECTOR hitLocation;
    _mvector hitVelocity;
    Motion *hitMotion;
    playerObject *whohitme;
    uint32_t hitMask;
    uint32_t hitDelay;
    uint8_t hitNumber;
    uint8_t numAttackers;
    ProjType *projectile;
    Motion **pMotion;
    uint32_t pFlags;
    uint32_t forceFlags;
    int64_t forceData[6];
    int32_t airVelocity;
    int32_t airAngle;
    uint32_t groundDelay;
    int16_t maxMotions;
    int16_t oldmaxCMotions;
    Motion *paMotions;
    int16_t maxCombos;
    int16_t subOffset;
    Combo *paCombos;
    int32_t fScale;
    CollisionData *paNodesSizes;
    int32_t numCollisionNodes;
    int16_t currentMotion;
    int16_t previousMotion;
    int32_t delayedMotion;
    int16_t ACTION_LOCK;
    int16_t runCounter;
    aiData *paiMemory;
    wsl_ENEMY *pEnemy;
    char hitLog[JPB_PLAYER_MOTION_NAME_BYTES];
    int32_t *shadow;
    int32_t *lockRing;
    playerSettings pSettings;
    JPBPlayerCallback pMainCallBack;
    JPBPlayerCallback pMotionCallBack;
    JPBPlayerCallback pForceCallBack;
    int32_t comboUserData;
};

extern playerObject gaPlayerData[JPB_PLAYER_CAPACITY];
int ch_blipad(uint16_t button);
int ch_pad(uint16_t button);
void ch_padadmin(void);
int ch_unblipad(uint16_t button);
/* Exact PDB global at matched-PC RVA 0x10DEBA0. */
extern ProjType *projType;
/* Exact PDB global at matched-PC RVA 0x53A5E8. */
extern playerObject *afterLife;
/* Exact PDB checkpoint/continue globals at RVAs 0x537DF8 and 0x53D320. */
extern int32_t corusPoints[2];
extern _svector reStartPos[2];
extern uint32_t reStartScore[2];
extern int32_t reStartCounter;
extern int32_t gCheckPoint;

Pad *player_GetPlayerPad(int index);
void _AddLifeTile(playerObject *player, VECTOR *center);
void _DrawTile(
    FVECTOR *position,
    float width,
    float height,
    uint32_t color,
    _Material *material);
void _DrawTile2D(
    FVECTOR *position,
    float width,
    float height,
    uint32_t color,
    int layer_depth);
void player_AfterLife(playerObject *player);
int player_DoCollisions(void);
void player_FreePlayer(playerObject *player);
void player_HandleSabre(void);
void player_RefreshPlayer(playerObject *player);
void player_ResetJedi(int index);
void player_gConnectMotionData(
    playerObject *player, char *motion_data);
playerObject *player_gCreateObject(
    struct sceneObject *scene,
    int type,
    JPBPlayerInitCallback initialize_player);
void player_gRefreshPlayers(void);
void jpb_PlayerSamplePad(
    playerObject *player, int pad_index, int input_enabled);
playerObject *player_gGetNewPlayerObject(int ID);
playerObject *player_gGetPlayerPtr(int index);
void player_gInitPlayers(int start);
void player_gProcessPlayers(void);
void jpb_PlayerGetFrameProfile(JPBPlayerFrameProfile *profile);
void jpb_PlayerSetFrameProfileEnabled(int enabled);
void jpb_PlayerSetProcessObserver(
    JPBPlayerProcessObserver observer,
    void *user_data);
void jpb_PlayerSetTileHook(
    JPBPlayerTileHook hook, void *user_data);

#if defined(__cplusplus)
#define JPB_PLAYER_STATIC_ASSERT static_assert
#else
#define JPB_PLAYER_STATIC_ASSERT _Static_assert
#endif

JPB_PLAYER_STATIC_ASSERT(
    sizeof(Pad) == 32, "Pad size must match PDB type 0x116E");
JPB_PLAYER_STATIC_ASSERT(
    sizeof(_mvector) == 8,
    "_mvector size must match PDB type 0x1164");
JPB_PLAYER_STATIC_ASSERT(
    sizeof(playerSettings) == 20,
    "playerSettings size must match PDB type 0x1141");
JPB_PLAYER_STATIC_ASSERT(
    sizeof(CollisionData) == 4,
    "CollisionData must match PDB type 0x1174");
JPB_PLAYER_STATIC_ASSERT(
    sizeof(ProjType) == 34,
    "ProjType must match PDB type 0x119F");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(ProjType, hitReact) == 27,
    "ProjType.hitReact layout changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(ProjType, flag) == 30,
    "ProjType.flag layout changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(CollisionData, parentid) == 3,
    "CollisionData.parentid layout changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, playerPad) ==
        sizeof(objectRoot) + sizeof(void *) * 2,
    "playerObject playerPad pointer prefix changed");
#if UINTPTR_MAX == UINT64_MAX
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, playerPad) == 40,
    "playerObject.playerPad x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, playernum) == 136,
    "playerObject.playernum x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, playerID) == 138,
    "playerObject.playerID x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, hitLocation) == 248,
    "playerObject.hitLocation x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, paNodesSizes) == 424,
    "playerObject.paNodesSizes x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, paMotions) == 392,
    "playerObject.paMotions x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, pMainCallBack) == 536,
    "playerObject.pMainCallBack x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, pMotionCallBack) == 544,
    "playerObject.pMotionCallBack x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, whohitme) == 280,
    "playerObject.whohitme x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, currentMotion) == 436,
    "playerObject.currentMotion x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, delayedMotion) == 440,
    "playerObject.delayedMotion x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, shadow) == 496,
    "playerObject.shadow x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, lockRing) == 504,
    "playerObject.lockRing x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    offsetof(playerObject, pSettings) == 512,
    "playerObject.pSettings x64 offset changed");
JPB_PLAYER_STATIC_ASSERT(
    sizeof(playerObject) == 568,
    "playerObject size must match PDB type 0x128D");
#endif

#undef JPB_PLAYER_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
