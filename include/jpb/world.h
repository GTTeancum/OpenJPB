#ifndef JPB_WORLD_H
#define JPB_WORLD_H

#include "jpb/fmath.h"
#include "jpb/list.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rdVECTOR {
    int32_t vx;
    int32_t vy;
    int32_t vz;
} rdVECTOR;

typedef struct BAP_CAMERADOLLY {
    uint32_t flags;
    int16_t pitch;
    int16_t slacky;
    int16_t yaw;
    int16_t offy;
    int16_t slackx;
    int16_t offx;
    rdVECTOR offset;
    int16_t slackz;
    int16_t offz;
} BAP_CAMERADOLLY;

typedef struct wsl_fatPoly wsl_fatPoly;
typedef struct wsl_thinPoly wsl_thinPoly;
typedef struct wsl_mapSlot wsl_mapSlot;
typedef struct wsl_entryTags wsl_entryTags;
typedef struct wsl_mapEntry wsl_mapEntry;
typedef struct wsl_libTags wsl_libTags;
typedef struct wsl_libPart wsl_libPart;
typedef struct wsl_BAP_TEXTURE wsl_BAP_TEXTURE;
typedef struct playerObject playerObject;
typedef struct wsl_BT_ANIMDEF wsl_BT_ANIMDEF;
typedef struct wsl_BAP_WAYPOINT wsl_BAP_WAYPOINT;
typedef struct wsl_BAP_PLACEMENT wsl_BAP_PLACEMENT;
typedef struct wsl_BAPAI_DEFAULTS wsl_BAPAI_DEFAULTS;
typedef struct BAP_AI BAP_AI;
typedef struct BAP_AINODE BAP_AINODE;
typedef struct objectRoot objectRoot;
typedef struct kfNode kfNode;
typedef struct wsl_ENEMY wsl_ENEMY;
typedef struct wsl_Powerup wsl_Powerup;

/* Direct PDB types 0x10C0 and 0x10CC. */
typedef union UDATA {
    int32_t p;
    int32_t si;
    uint32_t ui;
    float f;
    int16_t sw[2];
    uint16_t uw[2];
    char b[4];
} UDATA;

struct BAP_AINODE {
    int16_t iParent;
    int16_t iChild;
    int16_t iSibling;
    int16_t opcode;
    UDATA vx;
};

/*
 * Direct PDB type 0x10DC. aiNodes is the one-element trailing inline array
 * used by the original variable-size record.
 */
struct BAP_AI {
    int32_t numNodes;
    int32_t numAvailable;
    int32_t bSize;
    uint32_t pVars;
    int32_t pad[4];
    BAP_AINODE aiNodes[1];
};

/* Direct PDB type 0x10CA. */
struct kfNode {
    Node node;
    uint32_t timer;
    objectRoot *id;
    int16_t chi;
    int16_t loc;
    int32_t flags;
};

/* Direct PDB type 0x1098. */
struct wsl_BAP_WAYPOINT {
    rdVECTOR loc;
    int32_t flags;
};

/* Direct PDB type 0x11DE. */
struct wsl_Powerup {
    _svector pos;
    uint8_t type;
    uint8_t rate;
    uint16_t data;
    int32_t timer;
};

/*
 * Direct PDB type 0x10ED. Pointer-bearing runtime links retain native width:
 * the matched x64 record is 240 bytes and intentionally compacts for Xbox.
 */
struct wsl_ENEMY {
    Node node;
    int32_t aiNum;
    int32_t actorNum;
    char aName[12];
    wsl_BAP_PLACEMENT *pPlace;
    uint32_t enemyFlags;
    int32_t enemyID;
    int32_t ownerType;
    int32_t active;
    int32_t exit_flag;
    BAP_AI *pAI;
    BAP_AINODE *pAINode;
    playerObject *pPlayer;
    int32_t enemyNum;
    int32_t hitPoints;
    int32_t range;
    int32_t aRange;
    int32_t aiLocation;
    int32_t lastWayPoint;
    uint32_t counter[5];
    int32_t aiTimer;
    int16_t currAIMode;
    int16_t prevAIMode[3];
    int32_t stackID;
    uint8_t switchData[16];
    kfNode *kungfu;
    int16_t movementMode;
    int16_t movementSpeed;
    _svector destination;
    uint32_t radius;
    rdVECTOR location;
    int32_t hitFlag;
    int32_t sinceLast;
    int32_t seqMode;
    int32_t currSeq;
    int32_t currSeqIndex;
    int32_t currHTHDelay;
    int32_t currRangedDelay;
    int32_t aiLevel;
};

/* Direct PDB types 0x108F, 0x10C2, and 0x1138. */
typedef struct wsl_BT_ANIMENTRY {
    int32_t frame;
    int32_t num;
    int32_t flags;
    rdVECTOR xyz;
    rdVECTOR pyr;
    rdVECTOR scale;
} wsl_BT_ANIMENTRY;

typedef struct wsl_BT_ANIMNODE {
    int32_t num;
    int32_t used;
    int32_t flags;
    int32_t level;
    int32_t numEntries;
    int32_t numChildren;
    int32_t iParent;
    int32_t iChild;
    int32_t iSibling;
    int32_t iNext;
    int32_t nodeSpeed;
    int32_t pad;
    wsl_BT_ANIMENTRY aEntry[8];
} wsl_BT_ANIMNODE;

struct wsl_BT_ANIMDEF {
    int32_t type;
    int32_t num;
    int32_t bSize;
    int32_t flags;
    int32_t fps;
    int32_t numFrames;
    int32_t numNodes;
    int32_t totalNodes;
    int32_t pad[8];
    wsl_BT_ANIMNODE aNodes[1];
};

/* Exact matched-PC PDB type 0x10DF. */
struct wsl_BAPAI_DEFAULTS {
    uint32_t activeFlags;
    int32_t startMode;
    int32_t movementMode;
    int32_t hitPoints;
    int32_t movementSpeed;
    int32_t mass;
    int32_t ftSpeed;
    int32_t fov;
    int32_t range;
    int32_t angle;
    int32_t aRange;
    int32_t daRange;
    int32_t daDelay;
    int32_t ownerType;
    int32_t latency;
    int32_t skillLevel;
    int32_t performanceLevel;
    int32_t rangeExt[4];
    int32_t pad[2];
    uint16_t soundExt[4];
    uint16_t emitterExt[4];
    uint16_t enemyExt[12];
    uint16_t waypntExt[4];
    uint16_t weaponExt[4];
    uint16_t mapanimExt[4];
    uint16_t lightExt[4];
};

/* Direct PDB type 0x115F. Pointer-like archive fields remain 32-bit IDs. */
struct wsl_BAP_PLACEMENT {
    wsl_BAPAI_DEFAULTS aiDf;
    int32_t aiNum;
    int32_t actorNum;
    rdVECTOR loc;
    char aName[12];
    int32_t enemyID;
    int32_t status;
    uint32_t genDelay;
    uint32_t pLastEnemy;
    int32_t nLink;
    uint16_t links[8];
    int32_t nWaypnt;
    wsl_BAP_WAYPOINT wayPoints[1];
};

/* Direct PDB types 0x11B4 and 0x11C1. */
typedef struct wsl_libPoly {
    uint16_t tagIndex;
    int16_t textureID;
    int32_t n;
} wsl_libPoly;

struct wsl_libPart {
    int32_t *index;
    int16_t *shared;
    uint32_t animstuff;
    int16_t center;
    uint8_t numverts;
    uint8_t numpolys;
    wsl_libPoly polys[20];
};

/*
 * Direct PDB type 0x1251. Pointer fields intentionally remain native-width:
 * x64 offsets match the evidence build, while the 32-bit Xbox layout compacts
 * without embedding host pointers or serialization assumptions.
 */
typedef struct WorldData {
    rdVECTOR start;
    rdVECTOR location;
    rdVECTOR p0location;
    rdVECTOR p1location;
    wsl_fatPoly *pFat;
    wsl_thinPoly *pThin;
    wsl_mapSlot *pNewMap;
    wsl_entryTags *pEntryTags;
    wsl_mapEntry *pEntry;
    wsl_libTags *pLibTags;
    int32_t numTags;
    wsl_libPart **pLib;
    int32_t numLibs;
    CVECTOR bkColor;
    int32_t numPolys;
    wsl_BAP_TEXTURE *pTexture;
    int32_t numTexture;
    int16_t minX;
    int16_t minZ;
    int16_t minY;
    int16_t maxX;
    int16_t maxZ;
    int16_t maxY;
    int16_t sizeX;
    int16_t sizeZ;
    int16_t sizeY;
    BAP_CAMERADOLLY *pObiDolly;
    BAP_CAMERADOLLY aDolly[256];
    BAP_CAMERADOLLY aBkDolly[256];
    int16_t currentDolly;
    int16_t overRideDolly;
    playerObject *player0;
    playerObject *player1;
    wsl_BT_ANIMDEF *animDef[255];
    int32_t nADef;
    int32_t *animMapEnemies;
    int32_t nAnimMap;
    int16_t *pPalette;
    char *pColor;
    int32_t nEnemy;
    wsl_BAP_PLACEMENT **apEnemy;
    int32_t nAI;
    BAP_AI **apAI;
    int32_t nActor;
    char **apActorNames;
    int32_t nPowerups;
    wsl_Powerup *pPowerups;
    void *celbase;
    int32_t gotbackdrop;
} WorldData;

extern WorldData *gpWorld;
extern char LevelSelect;
extern int32_t totalframes;
/* Exact PDB global at matched-PC RVA 0x508584. */
extern int32_t streets_reached_stairs;
/* Exact PDB global at matched-PC RVA 0x4BA9F0. */
extern int32_t gHidePikobisModel;

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_WORLD_STATIC_ASSERT(condition, message) \
    static_assert(condition, message)
#else
#define JPB_WORLD_STATIC_ASSERT(condition, message) \
    _Static_assert(condition, message)
#endif

JPB_WORLD_STATIC_ASSERT(sizeof(rdVECTOR) == 12, "rdVECTOR size changed");
JPB_WORLD_STATIC_ASSERT(
    sizeof(wsl_BAP_WAYPOINT) == 16,
    "wsl_BAP_WAYPOINT size changed");
JPB_WORLD_STATIC_ASSERT(
    sizeof(wsl_Powerup) == 16,
    "wsl_Powerup size must match PDB type 0x11DE");
JPB_WORLD_STATIC_ASSERT(
    offsetof(wsl_Powerup, data) == 10,
    "wsl_Powerup.data offset changed");
JPB_WORLD_STATIC_ASSERT(
    sizeof(BAP_CAMERADOLLY) == 32, "BAP_CAMERADOLLY size changed");
JPB_WORLD_STATIC_ASSERT(sizeof(UDATA) == 4, "UDATA size changed");
JPB_WORLD_STATIC_ASSERT(
    sizeof(BAP_AINODE) == 12, "BAP_AINODE size changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(BAP_AINODE, vx) == 8,
    "BAP_AINODE.vx offset changed");
JPB_WORLD_STATIC_ASSERT(sizeof(BAP_AI) == 44, "BAP_AI size changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(BAP_AI, aiNodes) == 32,
    "BAP_AI.aiNodes offset changed");
#if UINTPTR_MAX == UINT64_MAX
JPB_WORLD_STATIC_ASSERT(sizeof(kfNode) == 32, "x64 kfNode size changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(kfNode, timer) == 8, "x64 kfNode.timer offset changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(kfNode, id) == 16, "x64 kfNode.id offset changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(kfNode, chi) == 24, "x64 kfNode.chi offset changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(kfNode, flags) == 28, "x64 kfNode.flags offset changed");
#elif UINTPTR_MAX == UINT32_MAX
JPB_WORLD_STATIC_ASSERT(sizeof(kfNode) == 20, "32-bit kfNode size changed");
#endif
JPB_WORLD_STATIC_ASSERT(
    sizeof(wsl_BT_ANIMENTRY) == 48, "wsl_BT_ANIMENTRY size changed");
JPB_WORLD_STATIC_ASSERT(
    sizeof(wsl_BT_ANIMNODE) == 432, "wsl_BT_ANIMNODE size changed");
JPB_WORLD_STATIC_ASSERT(
    sizeof(wsl_BT_ANIMDEF) == 496, "wsl_BT_ANIMDEF size changed");
JPB_WORLD_STATIC_ASSERT(
    sizeof(wsl_BAPAI_DEFAULTS) == 164,
    "wsl_BAPAI_DEFAULTS size changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(wsl_BAPAI_DEFAULTS, ownerType) == 52,
    "wsl_BAPAI_DEFAULTS.ownerType offset changed");
JPB_WORLD_STATIC_ASSERT(
    sizeof(wsl_BAP_PLACEMENT) == 252,
    "wsl_BAP_PLACEMENT size changed");
JPB_WORLD_STATIC_ASSERT(sizeof(wsl_libPoly) == 8, "wsl_libPoly size changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(wsl_libPart, animstuff) == sizeof(void *) * 2,
    "wsl_libPart pointer prefix changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(wsl_libPart, polys) == sizeof(void *) * 2 + 8,
    "wsl_libPart metadata changed");

#if UINTPTR_MAX == UINT64_MAX
JPB_WORLD_STATIC_ASSERT(
    sizeof(wsl_ENEMY) == 240, "wsl_ENEMY x64 size changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(wsl_ENEMY, enemyFlags) == 40,
    "wsl_ENEMY.enemyFlags x64 offset changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(wsl_ENEMY, pPlayer) == 80,
    "wsl_ENEMY.pPlayer x64 offset changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(wsl_ENEMY, exit_flag) == 56,
    "wsl_ENEMY.exit_flag x64 offset changed");
JPB_WORLD_STATIC_ASSERT(
    sizeof(wsl_libPart) == 184, "wsl_libPart x64 size changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(WorldData, pFat) == 48, "WorldData.pFat x64 offset changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(WorldData, pObiDolly) == 160,
    "WorldData.pObiDolly x64 offset changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(WorldData, animMapEnemies) == 18624,
    "WorldData.animMapEnemies x64 offset changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(WorldData, pPalette) == 18640,
    "WorldData.pPalette x64 offset changed");
JPB_WORLD_STATIC_ASSERT(
    offsetof(WorldData, pPowerups) == 18712,
    "WorldData.pPowerups x64 offset changed");
JPB_WORLD_STATIC_ASSERT(
    sizeof(WorldData) == 18736, "WorldData x64 size changed");
#elif UINTPTR_MAX == UINT32_MAX
JPB_WORLD_STATIC_ASSERT(
    sizeof(wsl_libPart) == 176, "wsl_libPart x86 size changed");
#endif

#undef JPB_WORLD_STATIC_ASSERT

#endif
