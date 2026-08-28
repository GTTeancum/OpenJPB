/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\enemy.c.
 *
 * The reviewed subset contains placement activation/spawn and cleanup,
 * enemy initialization/reset, defend/pre/post-frame state transfer, the small
 * BAP AI-node traversal and mode-stack procedures, enemy_KillKill, map-trigger
 * activation, pointer adjustment, nearest-waypoint selection, point
 * accounting, console-state commands, and deferred teleport application. The
 * PDB-named
 * enemy_HandleEnemies call surface now owns the reviewed normal enemy frame,
 * including its exact debug-radar branch; the deliberately prefixed facade
 * retains parser-boundary diagnostics for portable validation. The PDB-named
 * enemy_ParseOpcodes call surface covers every matched dispatch branch while
 * its jpb_ companion reports malformed or unknown authored data safely.
 * Exact procedure control flow and stores were checked against the matched
 * x64 instructions.
 *
 * PDB module: 0030
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\enemy.obj
 * Primary source: W:\SWJediPowerBattles\Work\enemy.c
 * Compiler language: c
 * Emitted procedures: 48
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/enemy.h"
#include "jpb/achievement.h"
#include "jpb/ai.h"
#include "jpb/animctrl.h"
#include "jpb/animutil.h"
#include "jpb/audio_stream.h"
#include "jpb/brainutl.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/combo.h"
#include "jpb/effects.h"
#include "jpb/extracharacters.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/jonny.h"
#include "jpb/level.h"
#include "jpb/loader.h"
#include "jpb/menu.h"
#include "jpb/model.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/shaolin.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/vectors.h"
#include "jpb/vehicle.h"
#include "jpb/whook.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Exact enemy.c globals from the matched PDB. The 20-record pool occupies
 * RVAs 0x51C2C0..0x51D57F in the x64 reference.
 */
wsl_ENEMY aEnemyListNodes[20];
List enemyList[2];
List enemyFreeList;
int32_t mCurEnemyList;
int32_t gShowAI;
int32_t nEnemy;
wsl_ENEMY *pLatestDebugEnemy;
wsl_ENEMY *pDebugEnemy;
/* Exact enemy.c module-local global at matched-PC RVA 0x51D5D4. */
static int32_t nearestDebugRange;
/* Exact enemy.c module-local globals at matched-PC RVAs 0x51D5D8/DC. */
static int32_t debugCounter;
static int32_t processTimer;

/* Portable validation record; not part of the matched game's global ABI. */
static JPBEnemyOpcodeParseResult jpb_enemy_last_frame_result =
    JPB_ENEMY_OPCODE_PARSE_COMPLETE;
static uint16_t jpb_enemy_last_unsupported_opcode;
static JPBEnemyFrameProfile jpb_enemy_frame_profile;
static int jpb_enemy_frame_profile_enabled;

/* Exact AI flag storage at matched-PC RVAs 0x10DBEF0..0x10DBF7F. */
uint8_t abGlobalBits[16];
int32_t _aiFlagsTimer[4];
int32_t _aiFlagsSave[4];
int32_t _aiFlags[4];

static wsl_ENEMY *aisub_get_extended_enemy(
    wsl_ENEMY *bpEnemy,
    int extension_index)
{
    int enemyID =
        bpEnemy->pPlace->aiDf
            .enemyExt[extension_index];

    if (enemyID >= gpWorld->nEnemy) {
        return NULL;
    }
    return (wsl_ENEMY *)getPtr(
        enemy_getPointerIndex(enemyID),
        JPB_POINTER_ARRAY_ENEMY);
}

static int jpb_enemy_axis_distance_within(
    const VECTOR *first,
    const VECTOR *second,
    int range)
{
    int dx = first->vx - second->vx;
    int dy = first->vy - second->vy;
    int dz = first->vz - second->vz;

    return
        (dx < 0 ? -dx : dx) <= range &&
        (dy < 0 ? -dy : dy) <= range &&
        (dz < 0 ? -dz : dz) <= range;
}

static int jpb_enemy_ascii_equal_ignore_case(
    const char *first, const char *second)
{
    unsigned char a;
    unsigned char b;

    do {
        a = (unsigned char)*first++;
        b = (unsigned char)*second++;
        if (a >= 'A' && a <= 'Z') {
            a = (unsigned char)(a + ('a' - 'A'));
        }
        if (b >= 'A' && b <= 'Z') {
            b = (unsigned char)(b + ('a' - 'A'));
        }
        if (a != b) {
            return 0;
        }
    } while (a != 0);
    return 1;
}

static double jpb_enemy_profile_seconds(void)
{
    if (!jpb_enemy_frame_profile_enabled) {
        return 0.0;
    }
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

static void jpb_enemy_profile_record(
    double *last_seconds,
    double *max_seconds,
    double seconds)
{
    if (last_seconds != NULL) {
        *last_seconds = seconds;
    }
    if (max_seconds != NULL && seconds > *max_seconds) {
        *max_seconds = seconds;
    }
}

static void jpb_enemy_profile_begin_frame(void)
{
    jpb_enemy_frame_profile.lastTotalSeconds = 0.0;
    jpb_enemy_frame_profile.lastRadarSeconds = 0.0;
    jpb_enemy_frame_profile.lastPrepareSeconds = 0.0;
    jpb_enemy_frame_profile.lastCheckNewSeconds = 0.0;
    jpb_enemy_frame_profile.lastReferenceSeconds = 0.0;
    jpb_enemy_frame_profile.lastKungfuStartSeconds = 0.0;
    jpb_enemy_frame_profile.lastLoopSeconds = 0.0;
    jpb_enemy_frame_profile.lastPreFrameSeconds = 0.0;
    jpb_enemy_frame_profile.lastParseSeconds = 0.0;
    jpb_enemy_frame_profile.lastPostFrameSeconds = 0.0;
    jpb_enemy_frame_profile.lastRangeSeconds = 0.0;
    jpb_enemy_frame_profile.lastKungfuDoSeconds = 0.0;
    jpb_enemy_frame_profile.lastProcessedEnemies = 0;
    jpb_enemy_frame_profile.lastParseInstructions = 0;
}

void jpb_EnemyGetFrameProfile(JPBEnemyFrameProfile *profile)
{
    if (profile != NULL) {
        *profile = jpb_enemy_frame_profile;
    }
}

void jpb_EnemySetFrameProfileEnabled(int enabled)
{
    jpb_enemy_frame_profile_enabled = enabled != 0;
    memset(&jpb_enemy_frame_profile, 0, sizeof(jpb_enemy_frame_profile));
}

static int jpb_enemy_profile_node_index(
    const wsl_ENEMY *enemy,
    const BAP_AINODE *node)
{
    if (enemy == NULL || enemy->pAI == NULL ||
        enemy->pAI->aiNodes == NULL || node == NULL ||
        node < enemy->pAI->aiNodes ||
        node >= enemy->pAI->aiNodes + enemy->pAI->numNodes) {
        return -1;
    }
    return (int)(node - enemy->pAI->aiNodes);
}

static void jpb_enemy_profile_record_single_parse(
    const wsl_ENEMY *enemy,
    double seconds)
{
    if (seconds > jpb_enemy_frame_profile.maxSingleParseSeconds) {
        jpb_enemy_frame_profile.maxSingleParseSeconds = seconds;
        jpb_enemy_frame_profile.maxParseEnemyId =
            enemy != NULL ? enemy->enemyID : -1;
        jpb_enemy_frame_profile.maxParseAi =
            enemy != NULL ? enemy->aiNum : -1;
    }
}

static void jpb_enemy_profile_record_opcode(
    const wsl_ENEMY *enemy,
    const BAP_AINODE *node,
    uint16_t opcode,
    double seconds)
{
    if (seconds > jpb_enemy_frame_profile.maxSingleOpcodeSeconds) {
        jpb_enemy_frame_profile.maxSingleOpcodeSeconds = seconds;
        jpb_enemy_frame_profile.maxOpcodeEnemyId =
            enemy != NULL ? enemy->enemyID : -1;
        jpb_enemy_frame_profile.maxOpcodeAi =
            enemy != NULL ? enemy->aiNum : -1;
        jpb_enemy_frame_profile.maxOpcodeNode =
            jpb_enemy_profile_node_index(enemy, node);
        jpb_enemy_frame_profile.maxOpcode = opcode;
    }
}

static void getintank(
    wsl_ENEMY *enemy, int driver_index);
static void getonstap(
    wsl_ENEMY *enemy, int driver_index);
static JPBEnemyVehicleDiagnostics jpb_enemy_vehicle_diagnostics;
static JPBEnemyCameraOpcodeDiagnostics
    jpb_enemy_camera_opcode_diagnostics;

void jpb_EnemyGetVehicleDiagnostics(
    JPBEnemyVehicleDiagnostics *diagnostics)
{
    if (diagnostics != NULL) {
        *diagnostics = jpb_enemy_vehicle_diagnostics;
    }
}

void jpb_EnemyGetCameraOpcodeDiagnostics(
    JPBEnemyCameraOpcodeDiagnostics *diagnostics)
{
    if (diagnostics != NULL) {
        *diagnostics = jpb_enemy_camera_opcode_diagnostics;
    }
}

/* 0x460F0, 211 bytes, global, 5 named locals
 * _addEnemy
 * PDB type: int (wsl_BAP_PLACEMENT*, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int _addEnemy(
    wsl_BAP_PLACEMENT *pPlace,
    int id,
    int newAI,
    int forceon)
{
    wsl_ENEMY *bpEnemy;

    /*
     * forceon is a named fourth parameter in PDB type 0x69B5. The matched
     * optimized procedure never reads it, so preserve the interface without
     * inventing behavior.
     */
    (void)forceon;

    if ((pPlace->aiDf.activeFlags & UINT32_C(0x10)) != 0 &&
        (abGlobalBits[2] & UINT8_C(2)) != 0) {
        return 0;
    }

    bpEnemy = _initEnemy(pPlace);
    if (bpEnemy != NULL) {
        if (newAI == -1) {
            newAI = pPlace->aiNum;
        }
        bpEnemy->enemyID = id;
        bpEnemy->pAI = gpWorld->apAI[newAI];

        if (loader_CreateEnemy(bpEnemy) != 0) {
            list_AddTail(
                &enemyList[mCurEnemyList],
                &bpEnemy->node);
            pPlace->aiDf.activeFlags &=
                ~UINT32_C(0x10000000);
            return 1;
        }
        list_AddTail(&enemyFreeList, &bpEnemy->node);
    }

    pPlace->pLastEnemy = UINT32_MAX;
    return 0;
}

/* 0x461D0, 328 bytes, global, 6 named locals
 * _checkForNewEnemies
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void _checkForNewEnemies(void)
{
    int i;
    VECTOR *pos = (VECTOR *)(void *)&gpWorld->location;

    for (i = 0; i < gpWorld->nEnemy; ++i) {
        wsl_BAP_PLACEMENT *pPlace =
            gpWorld->apEnemy[i];

        if ((pPlace->aiDf.activeFlags &
             UINT32_C(0x10000001)) != 0 &&
            pPlace->status == 0) {
            int range = pPlace->aiDf.aRange;
            int dx = pos->vx - pPlace->loc.vx;
            int dy = pos->vy - pPlace->loc.vy;
            int dz = pos->vz - pPlace->loc.vz;
            int in_range =
                (dx < 0 ? -dx : dx) <= range &&
                (dz < 0 ? -dz : dz) <= range &&
                (dy < 0 ? -dy : dy) <= range;

            /*
             * Level 15 deliberately wakes placements 27 and 29 once
             * placement 24 is active, irrespective of their authored range.
             * The indexes and status tests are direct executable evidence.
             */
            if (GameStruct.CurrentLevel == 15 &&
                gpWorld->apEnemy[24]->status == 1) {
                if (i == 27) {
                    if (gpWorld->apEnemy[27]->status == 0) {
                        in_range = 1;
                    }
                } else if (i == 29 &&
                           gpWorld->apEnemy[29]->status == 0) {
                    in_range = 1;
                }
            }

            if ((range == 0 || in_range) &&
                _addEnemy(pPlace, i, -1, 1) != 0) {
                pPlace->status = 1;
            }
        }
    }
}

/* 0x46320, 90 bytes, global, 5 named locals
 * _countChildNodes
 * PDB type: int (wsl_ENEMY*, BAP_AINODE*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int _countChildNodes(
    wsl_ENEMY *enemy, BAP_AINODE *node)
{
    int count = 0;

    node = enemy_GetNodePointer(enemy, node->iChild);
    while (node != NULL) {
        ++count;
        node = enemy_GetNodePointer(
            enemy, node->iSibling);
    }
    return count;
}

/* 0x46380, 3 bytes, global, 1 named locals
 * _debugEnemy
 * PDB type: void (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void _debugEnemy(wsl_ENEMY *enemy)
{
    (void)enemy;
}

/* 0x46390, 3 bytes, global, 0 named locals
 * _debugEnemyFlags
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void _debugEnemyFlags(void)
{
}

/* 0x463A0, 305 bytes, global, 5 named locals
 * _deleteEnemy
 * PDB type: void (wsl_ENEMY*, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void _deleteEnemy(wsl_ENEMY *enemy, int remove)
{
    wsl_BAP_PLACEMENT *placement = enemy->pPlace;
    sceneObject *scene =
        (sceneObject *)
            enemy->pPlayer->playerRoot.pParent;
    animObject *animation =
        (animObject *)scene->pAnim;
    int link_index;

    if ((abGlobalBits[3] & 1U) != 0) {
        game_gSetGameFlags(UINT32_C(0x20));
        game_gSetGameFlags(UINT32_C(0x40));
        gCheckPoint = 0;
        reStartScore[0] = 0;
    }
    sound_StopSound(animation->loopHandle[0]);
    sound_StopSound(animation->loopHandle[1]);

    if (remove == 1 &&
        (placement->aiDf.activeFlags &
         UINT32_C(0x40)) == 0) {
        placement->status = 2;
        if (placement->aiDf.ownerType != 4 &&
            placement->aiDf.ownerType != 5) {
            player_FreePlayer(enemy->pPlayer);
        }
        enemy->exit_flag = 0;
        placement->pLastEnemy = UINT32_MAX;
        for (link_index = 0;
             link_index < placement->nLink;
             ++link_index) {
            wsl_BAP_PLACEMENT *linked =
                gpWorld->apEnemy[
                    placement->links[link_index]];

            linked->aiDf.activeFlags |=
                UINT32_C(0x10000000);
        }
    } else {
        placement->status = 0;
        player_FreePlayer(enemy->pPlayer);
        placement->pLastEnemy = UINT32_MAX;
    }
}

/* 0x464E0, 321 bytes, global, 2 named locals
 * _initEnemy
 * PDB type: wsl_ENEMY* (wsl_BAP_PLACEMENT*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
wsl_ENEMY *_initEnemy(wsl_BAP_PLACEMENT *placement)
{
    wsl_ENEMY *enemy =
        (wsl_ENEMY *)list_RemoveHead(&enemyFreeList);

    if (enemy == NULL) {
        return NULL;
    }

    memset(enemy, 0, sizeof(*enemy));
    enemy->pPlace = placement;
    enemy->aiNum = placement->aiNum;
    enemy->ownerType = placement->aiDf.ownerType;
    enemy->active = 1;
    enemy->enemyFlags = placement->aiDf.activeFlags;
    strcpy(enemy->aName, placement->aName);
    enemy->currAIMode = (int16_t)placement->aiDf.startMode;
    enemy->prevAIMode[0] = (int16_t)placement->aiDf.startMode;
    enemy->prevAIMode[1] = (int16_t)placement->aiDf.startMode;
    enemy->prevAIMode[2] = (int16_t)placement->aiDf.startMode;
    enemy->stackID = 0;
    enemy->aiLocation = 0;
    enemy->movementMode = (int16_t)placement->aiDf.movementMode;
    enemy->hitPoints = placement->aiDf.hitPoints;
    enemy->movementSpeed = (int16_t)placement->aiDf.movementSpeed;
    enemy->range = placement->aiDf.range;
    enemy->location = placement->loc;
    enemy->aiLevel = placement->aiDf.skillLevel / 5;
    placement->pLastEnemy =
        (uint32_t)addPtr(enemy, JPB_POINTER_ARRAY_ENEMY);
    enemy->exit_flag = 0;
    return enemy;
}

/* 0x46630, 36 bytes, global, 3 named locals
 * aisub_arithmeticFVariables
 * PDB type: void (float*, int, float)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void aisub_arithmeticFVariables(
    float *variable, int operation, float value)
{
    switch (operation) {
    case 0:
        *variable = value;
        break;
    case 1:
        *variable += value;
        break;
    case 2:
        *variable -= value;
        break;
    default:
        break;
    }
}

/* 0x46660, 192 bytes, global, 3 named locals
 * aisub_arithmeticSIVariables
 * PDB type: void (int*, int, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void aisub_arithmeticSIVariables(
    int *variable, int operation, int value)
{
    switch (operation) {
    case 0:
        *variable = value;
        break;
    case 1:
        *variable += value;
        break;
    case 2:
        *variable -= value;
        break;
    case 3:
        if (value != 0 && value != -1) {
            *variable = rand() % value + 1;
        } else {
            *variable = 0;
        }
        break;
    case 4:
        if (value != 0 && value != -1) {
            *variable += rand() % value + 1;
        }
        break;
    default:
        break;
    }
}

/* 0x46720, 20 bytes, global, 2 named locals
 * aisub_checkFlag
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int aisub_checkFlag(int flag, int value)
{
    return _aiFlags[flag] == value;
}

/* 0x46740, 1352 bytes, global, 1 named locals
 * aisub_clearglobalflags
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void aisub_clearglobalflags(void)
{
    /*
     * The optimized reference clears bits 0..127 individually. Its observable
     * result is the same all-zero 16-byte bit array, followed by clearing each
     * of the three four-int AI flag arrays.
     */
    memset(abGlobalBits, 0, sizeof(abGlobalBits));
    memset(_aiFlagsTimer, 0, sizeof(_aiFlagsTimer));
    memset(_aiFlagsSave, 0, sizeof(_aiFlagsSave));
    memset(_aiFlags, 0, sizeof(_aiFlags));
}

/* 0x46C90, 112 bytes, global, 3 named locals
 * aisub_compareFVariables
 * PDB type: int (float, int, float)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int aisub_compareFVariables(
    float first, int comparison, float second)
{
    switch (comparison) {
    case 0:
        return first == second;
    case 1:
        return first >= second;
    case 2:
        return first <= second;
    case 3:
        return first != second;
    case 4:
        return first > second;
    case 5:
        return first < second;
    default:
        return 0;
    }
}

/* 0x46D00, 60 bytes, global, 1 named locals
 * aisub_compareLogicSense
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int aisub_compareLogicSense(int comparison)
{
    switch (comparison) {
    case 1:
    case 3:
    case 4:
        return 1;
    default:
        return 0;
    }
}

/* 0x46D40, 112 bytes, global, 3 named locals
 * aisub_compareSIVariables
 * PDB type: int (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int aisub_compareSIVariables(
    int first, int comparison, int second)
{
    switch (comparison) {
    case 0:
        return first == second;
    case 1:
        return first >= second;
    case 2:
        return first <= second;
    case 3:
        return first != second;
    case 4:
        return first > second;
    case 5:
        return first < second;
    default:
        return 0;
    }
}

/* 0x46DB0, 207 bytes, global, 8 named locals
 * aisub_findNearestWaypnt
 * PDB type: int (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int aisub_findNearestWaypnt(wsl_ENEMY *enemy)
{
    VECTOR *position;
    int nearest = -1;
    int nearest_range = 0x2000;
    int index;

    position = physics_gGetPosition(
        &enemy->pPlayer->playerRoot);
    for (index = 0;
         index < enemy->pPlace->nWaypnt;
         ++index) {
        VECTOR waypoint = {
            enemy->pPlace->wayPoints[index].loc.vx,
            enemy->pPlace->wayPoints[index].loc.vy,
            enemy->pPlace->wayPoints[index].loc.vz,
            0
        };
        int range =
            (int)vec_Distance2DLV(
                position, &waypoint);

        if (range < nearest_range) {
            nearest = index;
            nearest_range = range;
        }
    }
    return nearest;
}

/* 0x46E80, 137 bytes, global, 1 named locals
 * aisub_flagsManager
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void aisub_flagsManager(void)
{
    int flag;

    for (flag = 0; flag < 4; ++flag) {
        if (_aiFlagsTimer[flag] > 0 &&
            (uint32_t)_aiFlagsTimer[flag] <
                gGlobalTimer) {
            _aiFlags[flag] = _aiFlagsSave[flag];
            _aiFlagsTimer[flag] = 0;
        }
    }
}

/* 0x46F10, 904 bytes, global, 17 named locals
 * aisub_handleMoveFunction
 * PDB type: int (wsl_ENEMY*, UDATA, int, int...
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int aisub_handleMoveFunction(
    wsl_ENEMY *bpEnemy,
    UDATA target,
    int anim,
    int nDelta)
{
    playerObject *player = bpEnemy->pPlayer;
    int move = anim == 0 ? 1 :
               anim == 1 ? 2 : anim;
    int rVal = 0;
    int target_index = target.sw[0];

    switch (target_index) {
    case 0:
        bpEnemy->lastWayPoint = 0;
        /* fall through */
    case 1:
        return ai_WalkWayPoints(
            player, move, 1, nDelta);

    case 2: {
        VECTOR *pos =
            physics_gGetPosition(
                &player->playerRoot);
        wsl_BAP_PLACEMENT *pPlace =
            bpEnemy->pPlace;
        int dist = 0x2000;
        int waypoint_index = -1;
        int i;

        for (i = 0;
             i < pPlace->nWaypnt;
             ++i) {
            VECTOR waypoint = {
                pPlace->wayPoints[i].loc.vx,
                pPlace->wayPoints[i].loc.vy,
                pPlace->wayPoints[i].loc.vz,
                pPlace->wayPoints[i].flags
            };
            int r = (int)vec_Distance2DLV(
                pos, &waypoint);

            if (r < dist) {
                dist = r;
                waypoint_index = i;
            }
        }
        if (waypoint_index >= 0) {
            bpEnemy->lastWayPoint =
                waypoint_index;
            rVal = ai_WalkWayPoints(
                player, move, 1, nDelta);
        }
        return rVal;
    }

    case 3: {
        int dist = ai_FindFarPlayer(
            player, &player->target, 0);

        if (dist >= nDelta) {
            ai_WalktoPlayer(
                player, move, dist);
            return 0;
        }
        break;
    }

    case 4: {
        int dist = ai_FindNearestPlayer(
            player, &player->target);

        if (dist >= nDelta) {
            ai_WalktoPlayer(
                player, move, dist);
            return 0;
        }
        break;
    }

    case 5:
    case 6:
    case 7:
    case 8:
        return 0;

    default:
        if (target_index >= 9 &&
            target_index <= 18) {
            wsl_ENEMY *enemy =
                aisub_get_extended_enemy(
                    bpEnemy,
                    target_index - 9);

            if (enemy == NULL ||
                enemy->pPlayer == NULL) {
                return 0;
            }
            player->target = enemy->pPlayer;
        } else if (target_index == 19 ||
                   target_index == 20) {
            objectRoot *temp =
                brainutl_gGetNearestTarget(
                    &player->playerRoot,
                    target_index == 19 ? 3 : 2);

            if (temp == NULL) {
                return 0;
            }
            player->target =
                (playerObject *)
                    ((sceneObject *)
                         temp->pParent)
                        ->pPlayer;
        } else {
            return 0;
        }

        {
            int dist = physics_gGetRange(
                &player->playerRoot,
                &player->target->playerRoot);

            if (dist >= nDelta) {
                ai_WalktoPlayer(
                    player, move, dist);
                return 0;
            }
        }
        break;
    }

    if ((player->target->pFlags &
         UINT32_C(0x80)) == 0 &&
        move == 0) {
        physics_gTurnToFace(
            &player->playerRoot,
            physics_gFaceTarget(
                &player->playerRoot,
                &player->target->playerRoot),
            4);
    }
    if (player->currentMotion != 0) {
        (void)animctrl_MotionLockLevel(
            &player->playerRoot,
            &player->paMotions[0],
            0x19);
    }
    return 1;
}

/* 0x472A0, 960 bytes, global, 14 named locals
 * aisub_handleRangeFunction
 * PDB type: int (wsl_ENEMY*, UDATA*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int aisub_handleRangeFunction(
    wsl_ENEMY *bpEnemy, UDATA *vars)
{
    playerObject *player = bpEnemy->pPlayer;
    int rangeTarget = vars[0].si;
    int compare = vars[1].si;
    int dist0 = 0x8000;
    int dist1 = 0x8000;
    int branchFlag = 0;
    int posCheck;
    float compareThreshold = 0.0f;

    switch (rangeTarget) {
    case 0: {
        VECTOR *pos =
            physics_gGetPosition(
                &player->playerRoot);
        VECTOR waypoint = {
            bpEnemy->pPlace->wayPoints[0]
                .loc.vx,
            bpEnemy->pPlace->wayPoints[0]
                .loc.vy,
            bpEnemy->pPlace->wayPoints[0]
                .loc.vz,
            bpEnemy->pPlace->wayPoints[0]
                .flags
        };

        dist0 = (int)vec_Distance2DLV(
            pos, &waypoint);
        break;
    }

    case 1:
        dist0 = ai_FindNearestPlayer(
            player, &player->target);
        break;

    case 2:
    case 3:
        dist0 = physics_FindNearestEnemy(
            &player->playerRoot, rangeTarget);
        break;

    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10: {
        wsl_ENEMY *enemy =
            aisub_get_extended_enemy(
                bpEnemy, rangeTarget - 4);

        if (enemy != NULL &&
            enemy->pPlayer != NULL) {
            dist0 = physics_gGetRange(
                &enemy->pPlayer->playerRoot,
                &player->playerRoot);
        }
        break;
    }

    case 11:
        dist0 = physics_gGetRange(
            &player->playerRoot,
            &gpWorld->player0->playerRoot);
        dist1 = physics_gGetRange(
            &player->playerRoot,
            &gpWorld->player1->playerRoot);
        break;

    case 12:
        dist0 = ai_FindNearestPlayer(
            player, &player->target);
        branchFlag =
            brainutil_ReverseCheck(player);
        break;

    case 13:
    case 14:
    case 15: {
        sceneObject *player_scene;
        sceneObject *target_scene;
        physicsObject *player_physics;
        physicsObject *target_physics;
        int delta;

        (void)ai_FindNearestPlayer(
            player, &player->target);
        player_scene =
            (sceneObject *)
                player->playerRoot.pParent;
        target_scene =
            (sceneObject *)
                player->target
                    ->playerRoot.pParent;
        player_physics =
            (physicsObject *)
                player_scene->pPhysics;
        target_physics =
            (physicsObject *)
                target_scene->pPhysics;
        delta =
            (int)(target_physics->pos.vy -
                  player_physics->pos.vy);
        if (rangeTarget == 14) {
            delta = -delta;
        } else if (rangeTarget == 15 &&
                   delta < 0) {
            delta = -delta;
        }
        dist0 = delta;
        break;
    }

    default:
        return 0;
    }

    if (compare < 6) {
        /*
         * Exact RVAs 0x47560..0x4757F multiply the authored float by the
         * 256.0 constant at 0x28BA00, truncate to int, then convert back to
         * float before aisub_compareFVariables. World/physics distances use
         * the corresponding fixed-unit coordinate space.
         */
        compareThreshold =
            (float)(int)(vars[2].f * 256.0f);
        int first = aisub_compareFVariables(
            (float)dist0,
            compare,
            compareThreshold);

        posCheck = first;
        if (rangeTarget == 11 &&
            obj_gCheckObjectFlag(
                &gpWorld->player1->playerRoot,
                0,
                UINT32_C(0x20)) == 0) {
            int second =
                aisub_compareFVariables(
                    (float)dist1,
                    compare,
                    compareThreshold);

            posCheck = second;
            if (obj_gCheckObjectFlag(
                    &gpWorld->player0->playerRoot,
                    0,
                    UINT32_C(0x20)) == 0) {
                posCheck =
                    first & second;
            }
        }
    } else {
        float range =
            (float)bpEnemy->pPlace->aiDf
                .rangeExt[compare - 6];

        compareThreshold = range;
        posCheck =
            (float)dist0 <= range;
        if (rangeTarget == 11 &&
            obj_gCheckObjectFlag(
                &gpWorld->player1->playerRoot,
                0,
                UINT32_C(0x20)) == 0) {
            int second =
                (float)dist1 <= range;

            posCheck = second;
            if (obj_gCheckObjectFlag(
                    &gpWorld->player0->playerRoot,
                    0,
                    UINT32_C(0x20)) == 0) {
                posCheck =
                    ((float)dist0 <= range) &&
                    second;
            }
        }
    }

    if (rangeTarget == 12) {
        posCheck = branchFlag & posCheck;
    }
    ++jpb_enemy_vehicle_diagnostics.rangeEvaluationCount;
    if (posCheck != 0) {
        ++jpb_enemy_vehicle_diagnostics.rangeSuccessCount;
    }
    jpb_enemy_vehicle_diagnostics.lastRangeEnemyID =
        bpEnemy->enemyID;
    jpb_enemy_vehicle_diagnostics.lastRangeTarget = rangeTarget;
    jpb_enemy_vehicle_diagnostics.lastRangeCompare = compare;
    jpb_enemy_vehicle_diagnostics.lastRangeDistance0 = dist0;
    jpb_enemy_vehicle_diagnostics.lastRangeDistance1 = dist1;
    jpb_enemy_vehicle_diagnostics.lastRangeThreshold =
        compareThreshold;
    jpb_enemy_vehicle_diagnostics.lastRangeResult = posCheck;
    return posCheck;
}

/* 0x47660, 496 bytes, global, 10 named locals
 * aisub_handleScanFunction
 * PDB type: int (wsl_ENEMY*, int, int, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int aisub_handleScanFunction(
    wsl_ENEMY *bpEnemy,
    int target,
    int delta,
    int absolute)
{
    playerObject *player = bpEnemy->pPlayer;

    switch (target) {
    case 0: {
        sceneObject *scene =
            (sceneObject *)
                player->playerRoot.pParent;
        physicsObject *p =
            (physicsObject *)scene->pPhysics;

        if (absolute != 0) {
            p->face.vy = absolute;
            return 1;
        }
        if (delta != 0) {
            p->face.vy += delta;
        }
        return 1;
    }

    case 1:
        return 1;

    case 3:
        (void)ai_FindFarPlayer(
            player, &player->target, 0);
        (void)physics_gForceFaceTarget(
            &player->playerRoot,
            &player->target->playerRoot);
        return 1;

    case 4:
        (void)ai_FindNearestPlayer(
            player, &player->target);
        (void)physics_gForceFaceTarget(
            &player->playerRoot,
            &player->target->playerRoot);
        return 1;

    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18: {
        wsl_ENEMY *enemy =
            aisub_get_extended_enemy(
                bpEnemy, target - 9);

        if (enemy == NULL ||
            enemy->pPlayer == NULL) {
            return 0;
        }
        player->target = enemy->pPlayer;
        (void)physics_gForceFaceTarget(
            &player->playerRoot,
            &player->target->playerRoot);
        return 1;
    }

    case 19:
    case 20: {
        objectRoot *temp =
            brainutl_gGetNearestTarget(
                &player->playerRoot,
                target == 19 ? 3 : 2);

        if (temp == NULL) {
            return 1;
        }
        player->target =
            (playerObject *)
                ((sceneObject *)
                     temp->pParent)
                    ->pPlayer;
        (void)physics_gForceFaceTarget(
            &player->playerRoot,
            &player->target->playerRoot);
        return 1;
    }

    default:
        return 0;
    }
}

/* 0x47850, 34 bytes, global, 2 named locals
 * aisub_setFlag
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void aisub_setFlag(int flag, int value)
{
    if ((unsigned)flag < 4U) {
        _aiFlags[flag] = value;
        _aiFlagsTimer[flag] = 0;
    }
}

/* 0x47880, 31 bytes, global, 2 named locals
 * aisub_setNextWaypoint
 * PDB type: void (wsl_ENEMY*, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void aisub_setNextWaypoint(
    wsl_ENEMY *enemy, int waypoint)
{
    int count = enemy->pPlace->nWaypnt;

    if (count > 1) {
        enemy->lastWayPoint =
            waypoint < count - 1 ? waypoint : 0;
    }
}

/* 0x478A0, 3 bytes, global, 0 named locals
 * aisub_showflags
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void aisub_showflags(void)
{
}

/* 0x478B0, 71 bytes, global, 3 named locals
 * aisub_timedFlag
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void aisub_timedFlag(
    int flag, int value, int duration)
{
    if (_aiFlagsTimer[flag] == 0) {
        _aiFlagsSave[flag] = _aiFlags[flag];
        _aiFlags[flag] = value;
        _aiFlagsTimer[flag] =
            (int32_t)(gGlobalTimer + (uint32_t)duration);
    }
}

/* 0x47900, 188 bytes, global, 6 named locals
 * bapEnemyDoModeJump
 * PDB type: BAP_AINODE* (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
BAP_AINODE *bapEnemyDoModeJump(wsl_ENEMY *enemy)
{
    BAP_AI *ai = enemy->pAI;
    BAP_AINODE *node;
    int mode;

    enemy->pAINode = NULL;
    if (ai->numNodes < 1) {
        return NULL;
    }

    node = enemy_GetNodePointer(enemy, 0);
    if (node == NULL) {
        return NULL;
    }
    node = enemy_GetNodePointer(enemy, node->iChild);
    /*
     * The original data guarantees a valid root child. Keep the traversal
     * result guard at the point where the reference checks subsequent
     * sibling lookups, while avoiding an invalid host dereference for a
     * malformed reconstructed fixture.
     */
    if (node == NULL) {
        return NULL;
    }
    mode = node->opcode == 1;
    while (mode <= enemy->currAIMode) {
        node = enemy_GetNodePointer(enemy, node->iSibling);
        if (node == NULL) {
            return enemy->pAINode;
        }
        if (node->opcode == 1) {
            ++mode;
        }
    }
    enemy->pAINode =
        enemy_GetNodePointer(enemy, node->iChild);
    return enemy->pAINode;
}

/* 0x479C0, 68 bytes, global, 3 named locals
 * bapEnemyGetNextOpcode
 * PDB type: BAP_AINODE* (wsl_ENEMY*, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
BAP_AINODE *bapEnemyGetNextOpcode(
    wsl_ENEMY *enemy, int use_child)
{
    BAP_AINODE *node = enemy->pAINode;

    if (node != NULL) {
        int node_index =
            use_child ? node->iChild : node->iSibling;

        enemy->pAINode =
            enemy_GetNodePointer(enemy, node_index);
    }
    return enemy->pAINode;
}

/* 0x47A10, 55 bytes, global, 3 named locals
 * bapEnemySetContinue
 * PDB type: BAP_AINODE* (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
BAP_AINODE *bapEnemySetContinue(wsl_ENEMY *enemy)
{
    enemy->pAINode = enemy_GetNodePointer(
        enemy, enemy->pAINode->iParent);
    return enemy->pAINode;
}

/* 0x47A50, 71 bytes, global, 3 named locals
 * bapEnemyStartCycleLoop
 * PDB type: BAP_AINODE* (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
BAP_AINODE *bapEnemyStartCycleLoop(wsl_ENEMY *enemy)
{
    BAP_AINODE *root;

    enemy->pAINode = NULL;
    root = enemy_GetNodePointer(enemy, 0);
    if (root == NULL) {
        return NULL;
    }
    enemy->pAINode =
        enemy_GetNodePointer(enemy, root->iChild);
    return enemy->pAINode;
}

/* 0x47AA0, 63 bytes, global, 2 named locals
 * bapenemy_changeAIMode
 * PDB type: void (wsl_ENEMY*, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void bapenemy_changeAIMode(wsl_ENEMY *enemy, int mode)
{
    if (enemy->currAIMode != mode) {
        ++enemy->stackID;
        enemy->prevAIMode[2] = enemy->prevAIMode[1];
        enemy->prevAIMode[1] = enemy->prevAIMode[0];
        enemy->prevAIMode[0] = enemy->currAIMode;
        enemy->currAIMode = (int16_t)mode;
    }
}

/* 0x47AE0, 83 bytes, global, 1 named locals
 * bapenemy_postFrame
 * PDB type: void (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void bapenemy_postFrame(wsl_ENEMY *enemy)
{
    VECTOR *position;

    if (enemy == NULL || enemy->pPlayer == NULL) {
        return;
    }
    game_gSetEnergy(
        enemy->pPlayer->playernum, enemy->hitPoints);
    position = physics_gGetPosition(
        &enemy->pPlayer->playerRoot);
    if (position == NULL) {
        return;
    }
    enemy->location.vx = position->vx;
    enemy->location.vy = position->vy;
    enemy->location.vz = position->vz;
}

/* 0x47B40, 206 bytes, global, 2 named locals
 * bapenemy_preFrame
 * PDB type: int (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int bapenemy_preFrame(wsl_ENEMY *enemy)
{
    playerObject *player = enemy->pPlayer;

    if (enemy->exit_flag != 1 &&
        player->playerID != 99) {
        int energy =
            game_gGetEnergy(
                player->playerRoot.objectID);
        uint16_t player_id;

        enemy->hitPoints = energy;
        if (energy < 1) {
            enemy->active = 7;
            return 0;
        }
        enemy->aiLevel =
            (int32_t)(
                (uint32_t)enemy->aiLevel +
                gGlobalTimer);
        player_id = (uint16_t)player->playerID;
        if (player_id < 44 &&
            ((UINT64_C(0x0000080400000200) >>
              (player_id & 63U)) &
             1U) != 0) {
            if ((player->pFlags &
                 UINT32_C(0x0c01)) == 0) {
                (void)ai_DefendCheck(player);
                return 0;
            }
        } else if (player->currentMotion != 3 &&
                   (player->pFlags &
                    UINT32_C(0x0c01)) == 0) {
            if (player_id != 10 &&
                ai_DefendCheck(player) != 0) {
                return 1;
            }
            return 0;
        }
    }
    return 1;
}

/* 0x47C10, 64 bytes, global, 1 named locals
 * bapenemy_returnAIMode
 * PDB type: void (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void bapenemy_returnAIMode(wsl_ENEMY *enemy)
{
    enemy->currAIMode = enemy->prevAIMode[0];
    enemy->prevAIMode[0] = enemy->prevAIMode[1];
    enemy->prevAIMode[1] = enemy->prevAIMode[2];
    --enemy->stackID;
    enemy->prevAIMode[2] =
        (int16_t)enemy->pPlace->aiDf.startMode;
}

/* 0x47C50, 796 bytes, global, 16 named locals
 * console_EnemyCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int console_EnemyCommand(
    int argument_count,
    char **arguments,
    int *integer_arguments,
    float *float_arguments)
{
    (void)float_arguments;

    if (argument_count == 1) {
        /* list/getplayerpos/flags only emit console_Printf diagnostics. */
        if (jpb_enemy_ascii_equal_ignore_case(
                arguments[0], "list") ||
            jpb_enemy_ascii_equal_ignore_case(
                arguments[0], "getplayerpos") ||
            jpb_enemy_ascii_equal_ignore_case(
                arguments[0], "flags")) {
            return 0;
        }
    } else if (argument_count == 3) {
        if (jpb_enemy_ascii_equal_ignore_case(
                arguments[0], "flags") &&
            integer_arguments[2] >= 0 &&
            integer_arguments[2] < 128) {
            unsigned flag =
                (unsigned)integer_arguments[2];
            uint8_t mask =
                (uint8_t)(1U << (flag & 7U));

            if (jpb_enemy_ascii_equal_ignore_case(
                    arguments[1], "set")) {
                abGlobalBits[flag >> 3] |= mask;
                return 0;
            }
            if (jpb_enemy_ascii_equal_ignore_case(
                    arguments[1], "clr")) {
                abGlobalBits[flag >> 3] &=
                    (uint8_t)~mask;
                return 0;
            }
        }
    } else if (
        (argument_count == 5 ||
         argument_count == 6) &&
        jpb_enemy_ascii_equal_ignore_case(
            arguments[0], "active")) {
        int enemy_index = integer_arguments[1];
        wsl_BAP_PLACEMENT *placement;

        if (enemy_index > gpWorld->nEnemy) {
            enemy_index = gpWorld->nEnemy;
        }
        placement = gpWorld->apEnemy[enemy_index];
        placement->loc.vx =
            0x8000 - integer_arguments[2] * 0x100;
        placement->loc.vy =
            integer_arguments[3] * 0x100;
        placement->loc.vz =
            (integer_arguments[4] - 0x7f) * 0x100;
        placement->status = 0;
        if ((placement->aiDf.activeFlags &
             UINT32_C(0x10000000)) == 0) {
            placement->aiDf.activeFlags |=
                UINT32_C(0x10000000);
        }
        return 0;
    }
    /* The original tail prints the command synopsis and returns zero. */
    return 0;
}

/* 0x47F70, 42 bytes, global, 2 named locals
 * enemy_ActivateEnemy
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void enemy_ActivateEnemy(int enemy_index)
{
    if (gpWorld->apEnemy != NULL) {
        wsl_BAP_PLACEMENT *placement =
            gpWorld->apEnemy[enemy_index];

        if ((placement->aiDf.activeFlags &
             UINT32_C(0x10000001)) == 0) {
            placement->aiDf.activeFlags |=
                UINT32_C(0x10000000);
        }
    }
}

/* 0x47FA0, 280 bytes, global, 5 named locals
 * enemy_CalcPoints
 * PDB type: int (int*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int enemy_CalcPoints(int *enemy_count)
{
    int points = 0;
    int index;

    for (index = 0; index < gpWorld->nEnemy; ++index) {
        wsl_BAP_PLACEMENT *placement =
            gpWorld->apEnemy[index];
        int actor = placement->actorNum;
        int model = maModelID[actor][0];

        if (model < 80 && gaPoints[model] > 0) {
            ++maModelID[actor][2];
            points += gaPoints[model];
        }
    }
    if (enemy_count != NULL) {
        *enemy_count = gpWorld->nEnemy;
    }
    /*
     * The matched tail reports the per-model counts through console_Printf.
     * That presentation owner is still bounded; score/count state and the
     * exact return value remain game-owned here.
     */
    return points;
}

/* 0x480C0, 1008 bytes, global, 7 named locals
 * enemy_CheckTeleport
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void enemy_CheckTeleport(void)
{
    wsl_ENEMY *enemy;
    int camera_type = 0;
    int player0_active;
    int player1_active;

    if (tflag == 0 || tele == -1) {
        return;
    }
    if (LevelSelect == 8) {
        if (toff.vx == 0x0f080) {
            toff.vx = 0x0ee8c;
        }
        camera_type = 5;
    } else if (LevelSelect == 9) {
        toff.vx -= 0x0fa;
    }

    for (enemy = (wsl_ENEMY *)
             enemyList[mCurEnemyList].head;
         enemy != NULL;
         enemy = (wsl_ENEMY *)enemy->node.next) {
        sceneObject *scene;
        physicsObject *physics;

        if (enemy->enemyID == tele ||
            enemy->pPlayer == NULL ||
            enemy->pPlayer->playerRoot.pParent == NULL) {
            continue;
        }
        scene = (sceneObject *)
            enemy->pPlayer->playerRoot.pParent;
        physics = (physicsObject *)scene->pPhysics;
        if (physics == NULL ||
            !jpb_enemy_axis_distance_within(
                &tpos, &physics->vpos, trange)) {
            continue;
        }

        physics->pos.vx += (float)toff.vx;
        physics->pos.vy += (float)toff.vy;
        physics->pos.vz += (float)toff.vz;
        enemy->location.vx += toff.vx;
        enemy->location.vy += toff.vy;
        enemy->location.vz += toff.vz;
        scene_gSetSceneModelMatrixFV(
            0, &physics->angle, &physics->pos);
    }

    tflag = 0;
    tele = -1;
    player0_active =
        obj_gCheckObjectFlag(
            &gpWorld->player0->playerRoot,
            0,
            UINT32_C(0x20)) == 0;
    player1_active =
        obj_gCheckObjectFlag(
            &gpWorld->player1->playerRoot,
            0,
            UINT32_C(0x20)) == 0;

    if (player1_active) {
        if (player0_active) {
            maPhysicsData[0].pos.vx +=
                (float)toff.vx;
            maPhysicsData[0].pos.vy +=
                (float)toff.vy;
            maPhysicsData[0].pos.vz +=
                (float)toff.vz;
        }
        maPhysicsData[1].pos.vx +=
            (float)toff.vx;
        maPhysicsData[1].pos.vy +=
            (float)toff.vy;
        maPhysicsData[1].pos.vz +=
            (float)toff.vz;
        if (camera_type == 0 && !player0_active) {
            camera_type = 2;
        }
        (void)camera_SetCameraPos(camera_type);
        camera_SnapCamera(&gCamera);
        if (player0_active) {
            scene_gSetSceneModelMatrixFV(
                0,
                &maPhysicsData[0].angle,
                &maPhysicsData[0].pos);
        }
        scene_gSetSceneModelMatrixFV(
            1,
            &maPhysicsData[1].angle,
            &maPhysicsData[1].pos);
    } else {
        maPhysicsData[0].pos.vx +=
            (float)toff.vx;
        maPhysicsData[0].pos.vy +=
            (float)toff.vy;
        maPhysicsData[0].pos.vz +=
            (float)toff.vz;
        if (camera_type == 0) {
            camera_type = 1;
        }
        (void)camera_SetCameraPos(camera_type);
        camera_SnapCamera(&gCamera);
        scene_gSetSceneModelMatrixFV(
            0,
            &maPhysicsData[0].angle,
            &maPhysicsData[0].pos);
    }
    gCamera.viewType &= ~UINT32_C(0x1000);
}

/* 0x484B0, 35 bytes, global, 2 named locals
 * enemy_GetNodePointer
 * PDB type: BAP_AINODE* (wsl_ENEMY*, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
BAP_AINODE *enemy_GetNodePointer(
    wsl_ENEMY *enemy, int node_index)
{
    if (node_index >= 0 &&
        node_index < enemy->pAI->numNodes) {
        enemy->aiLocation = node_index;
        return &enemy->pAI->aiNodes[node_index];
    }
    return NULL;
}

/* 0x484E0, 1929 bytes, global, 25 named locals
 * enemy_HandleEnemies
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
static VECTOR *jpb_enemy_frame_reference_position(void)
{
    BAP_CAMERADOLLY *dolly =
        &gpWorld->aDolly[gpWorld->currentDolly];

    if ((dolly->flags & UINT32_C(0x400)) != 0) {
        playerObject *player = gpWorld->player0;

        if (player->playerRoot.objectID != -1 &&
            obj_gCheckObjectFlag(
                &player->playerRoot,
                0,
                UINT32_C(0x20)) == 0 &&
            (player->pFlags &
             UINT32_C(0x40200)) == 0) {
            return physics_gGetPosition(
                &player->playerRoot);
        }
        player = gpWorld->player1;
        if (player->playerRoot.objectID != -1 &&
            obj_gCheckObjectFlag(
                &player->playerRoot,
                0,
                UINT32_C(0x20)) == 0 &&
            (player->pFlags &
             UINT32_C(0x40200)) == 0) {
            return physics_gGetPosition(
                &player->playerRoot);
        }
    }
    return (VECTOR *)(void *)&gpWorld->location;
}

static void jpb_enemy_update_tank_countdown(int32_t *countdown)
{
    if (*countdown != 0) {
        uint32_t remaining =
            (uint32_t)*countdown -
            (uint32_t)gGlobalFrameRate;

        *countdown = 0;
        if ((remaining & UINT32_C(0x80000000)) == 0) {
            *countdown = (int32_t)remaining;
        }
    }
}

static void jpb_enemy_prepare_active_frame(void)
{
    uint32_t elapsed;

    timeAdj = 1;
    elapsed = brainutl_ElapsedTime(
        processTimer, UINT32_C(0x1000));
    if (elapsed != 0) {
        processTimer = (int32_t)elapsed;
    }
    pDebugEnemy = pLatestDebugEnemy;
    nearestDebugRange = 0x10000;
    debugCounter = 0;
    aisub_flagsManager();
    jpb_enemy_update_tank_countdown(&timesincetank[0]);
    jpb_enemy_update_tank_countdown(&timesincetank[1]);

    if (GameStruct.CurrentLevel == 3 ||
        GameStruct.CurrentLevel == 9) {
        if ((abGlobalBits[0] & UINT8_C(1)) != 0) {
            achievement_complete(
                GameStruct.CurrentLevel == 3 ? 34 : 35);
            (void)game_ModGameCounter(1);
            abGlobalBits[0] &= UINT8_C(0xfe);
            (void)game_gModScore(0, 250);
            (void)game_gModScore(1, 250);
        }
        tankID = -1;
    }
    if ((abGlobalBits[0] & UINT8_C(2)) != 0) {
        abGlobalBits[0] &= UINT8_C(0xfd);
        nextLevel = 1;
    }
    if ((abGlobalBits[5] & UINT8_C(0x80)) != 0) {
        VECTOR *position = coll_GetNodeCenter(0, 8);
        _svector velocity = {0, 6, 0, 0};

        if (position != NULL) {
            (void)sprite_GetPointsSprite(
                1,
                position,
                &velocity,
                UINT32_C(0x00808080),
                0);
        }
        abGlobalBits[5] &= UINT8_C(0x7f);
    }
    if ((abGlobalBits[6] & UINT8_C(1)) != 0) {
        VECTOR *position = coll_GetNodeCenter(1, 8);
        _svector velocity = {0, 6, 0, 0};

        if (position != NULL) {
            (void)sprite_GetPointsSprite(
                1,
                position,
                &velocity,
                UINT32_C(0x00808080),
                0);
        }
        abGlobalBits[6] &= UINT8_C(0xfe);
    }
    if (GameStruct.CurrentLevel == 13) {
        if (GameStruct.NumPlayers == 1) {
            abGlobalBits[6] |= UINT8_C(2);
        } else {
            abGlobalBits[6] &= UINT8_C(0xfd);
        }
    }
}

static void jpb_enemy_apply_level_frame_overrides(
    wsl_ENEMY *enemy)
{
    if (GameStruct.CurrentLevel == 6 &&
        enemy->enemyID == 0x75) {
        enemy->pPlace->aiDf.daRange = 10000;
    } else if (GameStruct.CurrentLevel == 7 &&
               enemy->enemyID == 0x3a) {
        sceneObject *scene =
            (sceneObject *)
                enemy->pPlayer->playerRoot.pParent;
        physicsObject *physics =
            scene == NULL
                ? NULL
                : (physicsObject *)scene->pPhysics;

        if (physics != NULL &&
            physics->pos.vz > -14800.0f) {
            physics_gSetPosition(
                &physics->physicsRoot,
                (int)physics->pos.vx,
                (int)physics->pos.vy,
                -14800);
        }
    }
}

JPBEnemyOpcodeParseResult jpb_enemy_ProcessActiveFrame(
    uint16_t *unsupported_opcode)
{
    int source_list = mCurEnemyList;
    int destination_list = source_list ^ 1;
    double frame_started;
    double stage_started;
    double loop_started;
    VECTOR *reference_position;
    JPBEnemyOpcodeParseResult frame_result =
        JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    wsl_ENEMY *enemy;
    int processed = 0;

    if (gpWorld == NULL ||
        gpWorld->player0 == NULL ||
        gpWorld->player1 == NULL) {
        return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
    }
    if (unsupported_opcode != NULL) {
        *unsupported_opcode = 0;
    }
    jpb_enemy_profile_begin_frame();
    frame_started = jpb_enemy_profile_seconds();

    if (OptionStruct.DebugLevel == 3) {
        stage_started = jpb_enemy_profile_seconds();
        enemy_Radar();
        jpb_enemy_profile_record(
            &jpb_enemy_frame_profile.lastRadarSeconds,
            &jpb_enemy_frame_profile.maxRadarSeconds,
            jpb_enemy_profile_seconds() - stage_started);
    }

    stage_started = jpb_enemy_profile_seconds();
    jpb_enemy_prepare_active_frame();
    jpb_enemy_profile_record(
        &jpb_enemy_frame_profile.lastPrepareSeconds,
        &jpb_enemy_frame_profile.maxPrepareSeconds,
        jpb_enemy_profile_seconds() - stage_started);
    stage_started = jpb_enemy_profile_seconds();
    _checkForNewEnemies();
    jpb_enemy_profile_record(
        &jpb_enemy_frame_profile.lastCheckNewSeconds,
        &jpb_enemy_frame_profile.maxCheckNewSeconds,
        jpb_enemy_profile_seconds() - stage_started);
    stage_started = jpb_enemy_profile_seconds();
    reference_position =
        jpb_enemy_frame_reference_position();
    jpb_enemy_profile_record(
        &jpb_enemy_frame_profile.lastReferenceSeconds,
        &jpb_enemy_frame_profile.maxReferenceSeconds,
        jpb_enemy_profile_seconds() - stage_started);
    stage_started = jpb_enemy_profile_seconds();
    shaolin_StartKungfu();
    jpb_enemy_profile_record(
        &jpb_enemy_frame_profile.lastKungfuStartSeconds,
        &jpb_enemy_frame_profile.maxKungfuStartSeconds,
        jpb_enemy_profile_seconds() - stage_started);
    list_InitList(&enemyList[destination_list]);
    gShowAI = OptionStruct.AIDebug > 2;

    loop_started = jpb_enemy_profile_seconds();
    while ((enemy =
                (wsl_ENEMY *)list_RemoveHead(
                    &enemyList[source_list])) != NULL) {
        JPBEnemyOpcodeParseResult parse_result =
            JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        int preframe_result;
        if (OptionStruct.AIDebug < 3 ||
            enemy->pPlayer->playerRoot.objectID !=
                (int)OptionStruct.AIDebug - 3) {
            gShowAI = 0;
        } else {
            gShowAI = 1;
        }
        ++processed;
        stage_started = jpb_enemy_profile_seconds();
        preframe_result = bapenemy_preFrame(enemy);
        jpb_enemy_profile_record(
            &jpb_enemy_frame_profile.lastPreFrameSeconds,
            &jpb_enemy_frame_profile.maxPreFrameSeconds,
            jpb_enemy_frame_profile.lastPreFrameSeconds +
                jpb_enemy_profile_seconds() - stage_started);
        if (preframe_result == 0 &&
            OptionStruct.AIDebug != 1 &&
            enemy->active == 1) {
            uint16_t opcode = 0;
            double parse_seconds;

            stage_started = jpb_enemy_profile_seconds();
            parse_result =
                jpb_enemy_ParseOpcodes(
                    enemy, &opcode);
            parse_seconds =
                jpb_enemy_profile_seconds() - stage_started;
            jpb_enemy_profile_record(
                &jpb_enemy_frame_profile.lastParseSeconds,
                &jpb_enemy_frame_profile.maxParseSeconds,
                jpb_enemy_frame_profile.lastParseSeconds +
                    parse_seconds);
            jpb_enemy_profile_record_single_parse(
                enemy, parse_seconds);
            if (frame_result ==
                    JPB_ENEMY_OPCODE_PARSE_COMPLETE &&
                parse_result !=
                    JPB_ENEMY_OPCODE_PARSE_COMPLETE) {
                frame_result = parse_result;
                if (unsupported_opcode != NULL) {
                    *unsupported_opcode = opcode;
                }
            }
        }
        stage_started = jpb_enemy_profile_seconds();
        bapenemy_postFrame(enemy);
        jpb_enemy_profile_record(
            &jpb_enemy_frame_profile.lastPostFrameSeconds,
            &jpb_enemy_frame_profile.maxPostFrameSeconds,
            jpb_enemy_frame_profile.lastPostFrameSeconds +
                jpb_enemy_profile_seconds() - stage_started);
        stage_started = jpb_enemy_profile_seconds();
        jpb_enemy_apply_level_frame_overrides(enemy);

        if (enemy->exit_flag == 0) {
            int range =
                enemy->pPlace->aiDf.daRange;
            int dx =
                reference_position->vx -
                enemy->location.vx;
            int dy =
                reference_position->vy -
                enemy->location.vy;
            int dz =
                reference_position->vz -
                enemy->location.vz;

            if (range == 0 ||
                (abs(dx) <= range &&
                 abs(dy) <= range &&
                 abs(dz) <= range)) {
                list_AddTail(
                    &enemyList[destination_list],
                    &enemy->node);
                jpb_enemy_profile_record(
                    &jpb_enemy_frame_profile.lastRangeSeconds,
                    &jpb_enemy_frame_profile.maxRangeSeconds,
                    jpb_enemy_frame_profile.lastRangeSeconds +
                        jpb_enemy_profile_seconds() - stage_started);
                continue;
            }
            _deleteEnemy(enemy, 0);
        } else {
            _deleteEnemy(enemy, 1);
        }
        list_AddTail(
            &enemyFreeList, &enemy->node);
        jpb_enemy_profile_record(
            &jpb_enemy_frame_profile.lastRangeSeconds,
            &jpb_enemy_frame_profile.maxRangeSeconds,
            jpb_enemy_frame_profile.lastRangeSeconds +
                jpb_enemy_profile_seconds() - stage_started);
    }
    jpb_enemy_profile_record(
        &jpb_enemy_frame_profile.lastLoopSeconds,
        &jpb_enemy_frame_profile.maxLoopSeconds,
        jpb_enemy_profile_seconds() - loop_started);

    nEnemy = processed;
    jpb_enemy_frame_profile.lastProcessedEnemies = (uint32_t)processed;
    if ((uint32_t)processed >
        jpb_enemy_frame_profile.maxProcessedEnemies) {
        jpb_enemy_frame_profile.maxProcessedEnemies =
            (uint32_t)processed;
    }
    stage_started = jpb_enemy_profile_seconds();
    shaolin_DoKungfu();
    jpb_enemy_profile_record(
        &jpb_enemy_frame_profile.lastKungfuDoSeconds,
        &jpb_enemy_frame_profile.maxKungfuDoSeconds,
        jpb_enemy_profile_seconds() - stage_started);
    mCurEnemyList = destination_list;
    jpb_enemy_profile_record(
        &jpb_enemy_frame_profile.lastTotalSeconds,
        &jpb_enemy_frame_profile.maxTotalSeconds,
        jpb_enemy_profile_seconds() - frame_started);
    return frame_result;
}

void enemy_HandleEnemies(void)
{
    jpb_enemy_last_unsupported_opcode = 0;
    jpb_enemy_last_frame_result =
        jpb_enemy_ProcessActiveFrame(
            &jpb_enemy_last_unsupported_opcode);
}

JPBEnemyOpcodeParseResult jpb_enemy_LastFrameResult(
    uint16_t *unsupported_opcode)
{
    if (unsupported_opcode != NULL) {
        *unsupported_opcode =
            jpb_enemy_last_unsupported_opcode;
    }
    return jpb_enemy_last_frame_result;
}

/* 0x48C70, 114 bytes, global, 5 named locals
 * enemy_HandleMapTriggers
 * PDB type: void (long*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void enemy_HandleMapTriggers(int32_t *cube)
{
    int triganim;

    if (cube == NULL) {
        return;
    }

    if (((uint32_t)cube[0] &
         UINT32_C(0x3c000000)) == 0) {
        int cube_index =
            ((cube[0] >> 14) & 0xff) * 9 +
            (leveldata[-4] >> 11);

        cube = &leveldata[cube_index];
    }

    triganim = (cube[1] >> 8) & 0xff;
    if (triganim != 0xff &&
        gpWorld->apEnemy != NULL) {
        wsl_BAP_PLACEMENT *pPlace =
            gpWorld->apEnemy[
                gpWorld->animMapEnemies[triganim]];

        if ((pPlace->aiDf.activeFlags &
             UINT32_C(0x10000001)) == 0) {
            pPlace->aiDf.activeFlags |=
                UINT32_C(0x10000000);
        }
    }
}

/* 0x48CF0, 195 bytes, global, 3 named locals
 * enemy_InitEnemies
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void enemy_InitEnemies(void)
{
    int i;

    aisub_clearglobalflags();
    memset(
        &jpb_enemy_vehicle_diagnostics,
        0,
        sizeof(jpb_enemy_vehicle_diagnostics));
    list_InitList(&enemyList[0]);
    list_InitList(&enemyList[1]);
    list_InitList(&enemyFreeList);
    for (i = 0; i < 20; ++i) {
        list_AddTail(&enemyFreeList, &aEnemyListNodes[i].node);
    }

    for (i = 0; i < gpWorld->nEnemy; ++i) {
        wsl_BAP_PLACEMENT *pPlace = gpWorld->apEnemy[i];

        pPlace->aiDf.activeFlags &= ~0x10000000U;
        pPlace->status = 0;
        pPlace->pLastEnemy = UINT32_MAX;
    }
}

/* 0x48DC0, 146 bytes, global, 4 named locals
 * enemy_KillKill
 * PDB type: void (VECTOR*, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void enemy_KillKill(VECTOR *pos, int trange)
{
    wsl_ENEMY *pEnemy =
        (wsl_ENEMY *)enemyList[mCurEnemyList].head;

    while (pEnemy != NULL) {
        sceneObject *scene =
            (sceneObject *)pEnemy->pPlayer->playerRoot.pParent;
        physicsObject *p =
            (physicsObject *)scene->pPhysics;
        int delta_x = pos->vx - p->vpos.vx;
        int delta_y = pos->vy - p->vpos.vy;
        int delta_z = pos->vz - p->vpos.vz;

        if (p->physicsRoot.objectID >= 2 &&
            pEnemy->ownerType != 3 &&
            pEnemy->ownerType != 0 &&
            (delta_x < 0 ? -delta_x : delta_x) <= trange &&
            (delta_z < 0 ? -delta_z : delta_z) <= trange &&
            (delta_y < 0 ? -delta_y : delta_y) <= trange) {
            pEnemy->exit_flag = 1;
        }
        pEnemy = (wsl_ENEMY *)pEnemy->node.next;
    }
}

static int jpb_enemy_ai_variable_count(const BAP_AI *ai)
{
    int stored_nodes;
    int variable_bytes;

    if (ai == NULL ||
        ai->numNodes < 0 ||
        ai->numAvailable < 0 ||
        ai->numAvailable > ai->numNodes) {
        return -1;
    }
    stored_nodes = ai->numNodes - ai->numAvailable;
    variable_bytes =
        ai->bSize -
        (int)offsetof(BAP_AI, aiNodes) -
        stored_nodes * (int)sizeof(BAP_AINODE);
    if (variable_bytes < 0 ||
        variable_bytes % (int)sizeof(UDATA) != 0) {
        return -1;
    }
    return variable_bytes / (int)sizeof(UDATA);
}

static UDATA *jpb_enemy_resolve_opcode_variables(
    wsl_ENEMY *enemy,
    BAP_AINODE *node,
    int required_count)
{
    uint16_t encoded_opcode =
        (uint16_t)node->opcode;

    if ((encoded_opcode & UINT16_C(0x4000)) != 0) {
        return required_count <= 1 ? &node->vx : NULL;
    } else {
        UDATA *variables =
            (UDATA *)getPtr(
                (int)enemy->pAI->pVars,
                JPB_POINTER_ARRAY_AI);
        int variable_count =
            jpb_enemy_ai_variable_count(enemy->pAI);

        if (variables == NULL ||
            variable_count < 0 ||
            node->vx.ui > (uint32_t)variable_count ||
            required_count >
                variable_count - (int)node->vx.ui) {
            return NULL;
        }
        return &variables[node->vx.ui];
    }
}

static JPBEnemyOpcodeParseResult
jpb_enemy_handle_player_placement(
    wsl_BAP_PLACEMENT *placement,
    int enemy_id,
    int new_ai,
    uint16_t opcode,
    uint16_t *unsupported_opcode)
{
    playerObject *player =
        placement->aiDf.ownerType == 4
            ? gpWorld->player0
            : gpWorld->player1;
    sceneObject *scene;
    physicsObject *physics;
    int object_id;

    (void)opcode;
    (void)unsupported_opcode;
    if (player == NULL ||
        player->playerRoot.objectID == -1) {
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }
    if (player->playerRoot.pParent == NULL) {
        return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
    }
    scene =
        (sceneObject *)
            player->playerRoot.pParent;
    if (scene->pScene == NULL ||
        obj_gCheckObjectFlag(
            &player->playerRoot,
            0,
            UINT32_C(0x20)) != 0) {
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }
    physics = (physicsObject *)scene->pPhysics;
    object_id = player->playerRoot.objectID;
    if (physics == NULL ||
        scene->pModel == NULL ||
        scene->pAnim == NULL ||
        player->paMotions == NULL ||
        player->maxMotions <= 2 ||
        object_id < 0 ||
        object_id >= JPB_PLAYER_CAPACITY ||
        player != &gaPlayerData[object_id] ||
        physics != &maPhysicsData[object_id] ||
        scene->pAnim !=
            &maAnimationData[object_id].animRoot) {
        return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
    }

    if (player->pEnemy == NULL) {
        wsl_ENEMY *new_enemy;

        physics_ResetJedi(object_id);
        anim_ResetJedi(object_id);
        player_ResetJedi(object_id);
        if ((game_gGetEnergy(object_id) == 0 ||
             (player->pFlags & UINT32_C(0x200)) != 0 ||
             afterLife == player) &&
            GameStruct.NumPlayers == 2) {
            int force = game_gGetForce(object_id);
            int x = (int)physics->pos.vx;
            int y = (int)physics->pos.vy;
            int z = (int)physics->pos.vz;

            if (afterLife == NULL) {
                player_AfterLife(player);
                afterLife = player;
            }
            player_RefreshPlayer(player);
            physics_gSetPosition(
                &player->playerRoot, x, y, z);
            game_gSetForce(object_id, force);
            if (afterLife->shadow == NULL) {
                afterLife->shadow =
                    (int32_t *)(void *)
                        sprite_GetBaseNodeMarker(
                            afterLife->playerRoot.objectID,
                            0x30);
            }
            afterLife = NULL;
            camera_SetCurrentCameraType(0);
        }
        new_enemy = _initEnemy(placement);
        if (new_enemy == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }
        animutl_SetCurrentLock(
            &player->playerRoot, 0);
        player->pEnemy = new_enemy;
        new_enemy->enemyID = enemy_id;
        new_enemy->pAI =
            gpWorld->apAI[new_ai];
        new_enemy->aiNum = new_ai;
        new_enemy->pPlayer = player;
        new_enemy->location.vx =
            (int32_t)physics->pos.vx;
        new_enemy->location.vy =
            (int32_t)physics->pos.vy;
        new_enemy->location.vz =
            (int32_t)physics->pos.vz;
        list_AddTail(
            &enemyList[mCurEnemyList],
            &new_enemy->node);
        player->paMotions[2].Lock =
            player->paMotions[1].Lock;
        player->pFlags |= UINT32_C(0x10);
    } else {
        player->pEnemy->exit_flag = 1;
        player->paMotions[2].Lock = 0x19;
        player->pFlags &=
            ~UINT32_C(0x10);
        player->pEnemy = NULL;
    }
    return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
}

static JPBEnemyOpcodeParseResult
jpb_enemy_execute_authored_opcode(
    wsl_ENEMY *enemy,
    BAP_AINODE *node,
    int *branch_flag,
    uint16_t *unsupported_opcode,
    int diagnostic)
{
    uint16_t encoded_opcode =
        (uint16_t)node->opcode;
    uint16_t opcode =
        (encoded_opcode & UINT16_C(0x4000)) != 0
            ? encoded_opcode & UINT16_C(0x0fff)
            : encoded_opcode;
    UDATA *variables;

    *branch_flag = 0;
    switch (opcode) {
    case 0x100:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 3);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        *branch_flag = aisub_handleScanFunction(
            enemy,
            variables[0].si,
            variables[1].si,
            variables[2].si);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x101:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 3);
        if (variables == NULL ||
            variables[0].si < 0 ||
            variables[0].si >= 5) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        *branch_flag = aisub_compareSIVariables(
            (int)enemy->counter[variables[0].si],
            variables[1].si,
            variables[2].si);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x103:
    case 0x10b:
    case 0x205:
    case 0x206:
    case 0x603:
    case 0x608:
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x105: {
        BAP_AINODE *entry;
        int child_count;
        int selector;
        int match_value = 999999;
        int selected_entry = 999999;
        int index;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL ||
            enemy->currAIMode < 0 ||
            enemy->currAIMode >=
                (int)sizeof(enemy->switchData)) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }

        child_count = _countChildNodes(enemy, node);
        selector = variables[0].si;
        if (selector >= 0 && selector < 5) {
            match_value =
                (int)enemy->counter[selector];
        } else if (selector == 5) {
            selected_entry = 0;
            if (child_count > 1) {
                do {
                    selected_entry =
                        rand() % child_count;
                } while (selected_entry ==
                         enemy->switchData[
                             enemy->currAIMode]);
            }
            enemy->switchData[enemy->currAIMode] =
                (uint8_t)selected_entry;
        } else if (selector == 6) {
            selected_entry =
                enemy->switchData[enemy->currAIMode];
            if (selected_entry >= child_count) {
                selected_entry = 0;
            }
            enemy->switchData[enemy->currAIMode] =
                (uint8_t)(selected_entry + 1);
        } else {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }

        entry = enemy_GetNodePointer(
            enemy, node->iChild);
        for (index = 0;
             entry != NULL && index < child_count;
             ++index) {
            uint16_t entry_opcode =
                (uint16_t)entry->opcode;

            if ((entry_opcode &
                 UINT16_C(0x0fff)) ==
                    UINT16_C(0x0180) &&
                (match_value == entry->vx.si ||
                 selected_entry == index)) {
                enemy->pAINode = entry;
                *branch_flag = 1;
                break;
            }
            entry = enemy_GetNodePointer(
                enemy, entry->iSibling);
        }
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }

    case 0x106:
        *branch_flag = -1;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x107:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 3);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        *branch_flag =
            aisub_handleRangeFunction(enemy, variables);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x108:
        /* The original reads the first short in BAP_AINODE (iParent), then
         * normal traversal advances to that parent's sibling.  This is the
         * script "continue" operation, not a jump to this node's child. */
        (void)bapEnemySetContinue(enemy);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x109:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 2);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        *branch_flag = aisub_compareFVariables(
            (float)gGlobalTimer,
            variables[0].si,
            (float)enemy->aiTimer +
                variables[1].f);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x10a:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 2);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        if (LevelSelect == 7 &&
            enemy->enemyID == 0x3a) {
            enemy->range = 100;
        }
        *branch_flag = aisub_compareSIVariables(
            enemy->range,
            variables[0].si,
            variables[1].si);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x180:
        *branch_flag = 1;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x201:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 3);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        *branch_flag = aisub_handleMoveFunction(
            enemy,
            variables[0],
            variables[1].si,
            (int)(variables[2].f * 256.0f));
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x203:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL ||
            variables[0].si < 0 ||
            variables[0].si >= 5) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        enemy->counter[variables[0].si] +=
            (uint32_t)timeAdj;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x204:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL ||
            variables[0].si < 0 ||
            variables[0].si >= 5) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        if (enemy->counter[variables[0].si] != 0) {
            enemy->counter[variables[0].si] -=
                (uint32_t)timeAdj;
        }
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x209:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 2);
        if (variables == NULL ||
            variables[0].si < 0 ||
            variables[0].si >= 4) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        aisub_setFlag(
            variables[0].si,
            variables[1].si);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x20a:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 2);
        if (variables == NULL ||
            variables[0].si < 0 ||
            variables[0].si >= 4) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        *branch_flag =
            _aiFlags[variables[0].si] ==
            variables[1].si;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x20c: {
        uint32_t toggle_mask = 0;
        BAP_CAMERADOLLY *dolly;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL ||
            gpWorld == NULL ||
            gpWorld->currentDolly < 0 ||
            gpWorld->currentDolly >=
                (int)(sizeof(gpWorld->aDolly) /
                      sizeof(gpWorld->aDolly[0]))) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        if (variables[0].si >= 0 &&
            variables[0].si <= 5) {
            toggle_mask =
                UINT32_C(8) << variables[0].si;
        } else if (variables[0].si == 6) {
            toggle_mask = UINT32_C(0x1e0);
        }
        dolly = &gpWorld->aDolly[gpWorld->currentDolly];
        ++jpb_enemy_camera_opcode_diagnostics.sequence;
        jpb_enemy_camera_opcode_diagnostics.enemyID = enemy->enemyID;
        jpb_enemy_camera_opcode_diagnostics.aiNum = enemy->aiNum;
        jpb_enemy_camera_opcode_diagnostics.nodeIndex =
            jpb_enemy_profile_node_index(enemy, node);
        jpb_enemy_camera_opcode_diagnostics.encodedOpcode =
            encoded_opcode;
        jpb_enemy_camera_opcode_diagnostics.value = variables[0].si;
        jpb_enemy_camera_opcode_diagnostics.dolly =
            gpWorld->currentDolly;
        jpb_enemy_camera_opcode_diagnostics.flagsBefore = dolly->flags;
        dolly->flags ^= toggle_mask;
        jpb_enemy_camera_opcode_diagnostics.flagsAfter = dolly->flags;
        jpb_enemy_camera_opcode_diagnostics.totalFrames = totalframes;
        jpb_enemy_camera_opcode_diagnostics.globalTimer = gGlobalTimer;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }

    case 0x20d:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 3);
        if (variables == NULL ||
            variables[0].si < 0 ||
            variables[0].si >= 4) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        aisub_timedFlag(
            variables[0].si,
            variables[1].si,
            variables[2].si);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x20e: {
        int motion_index;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL ||
            enemy->pPlayer == NULL ||
            enemy->pPlayer->paMotions == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        motion_index = variables[0].si;
        if (motion_index == 84 &&
            enemy->pPlayer->currentMotion <= 1 &&
            IsExtraCharacter(
                (model_id)enemy->pPlayer->playerID) !=
                0) {
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }
        if (motion_index >= 0) {
            if (motion_index > 7) {
                motion_index +=
                    enemy->pPlayer->subOffset;
            }
            if (motion_index < 0 ||
                motion_index >=
                    enemy->pPlayer->maxMotions) {
                return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            (void)animctrl_MotionNoLock(
                &enemy->pPlayer->playerRoot,
                &enemy->pPlayer
                     ->paMotions[motion_index]);
        } else {
            motion_index = -motion_index;
            if (motion_index > 7) {
                motion_index +=
                    enemy->pPlayer->subOffset;
            }
            if (motion_index < 0 ||
                motion_index >=
                    enemy->pPlayer->maxMotions) {
                return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            (void)animctrl_MotionChain(
                &enemy->pPlayer->playerRoot,
                &enemy->pPlayer
                     ->paMotions[motion_index]);
        }
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }

    case 0x20f:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL ||
            enemy->pPlayer == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        *branch_flag =
            enemy->pPlayer->currentMotion ==
            variables[0].si;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x301:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 4);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        *branch_flag =
            ai_SeqAttack(enemy, variables);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x302:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 4);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        *branch_flag =
            ai_HthAttack(enemy, variables);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x307:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 4);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        ai_RangedAttack(enemy, variables);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x303:
        nearestDebugRange = 0;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x400:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        bapenemy_changeAIMode(
            enemy, variables[0].si);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x401:
        bapenemy_returnAIMode(enemy);
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x405:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 2);
        if (variables == NULL ||
            variables[0].si < 0 ||
            variables[0].si >= 5) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        enemy->counter[variables[0].si] =
            (uint32_t)variables[1].si;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x408: {
        int16_t movement_speed;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL ||
            enemy->pPlayer == NULL ||
            enemy->pPlayer->paMotions == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        movement_speed =
            (int16_t)(int)(variables[0].f * 16.0f);
        enemy->movementSpeed = movement_speed;
        enemy->pPlayer->paMotions[1].vel =
            movement_speed;
        enemy->pPlayer->paMotions[2].vel =
            movement_speed;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }

    case 0x409: {
        sceneObject *scene;
        physicsObject *physics;
        int movement_mode;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL ||
            enemy->pPlayer == NULL ||
            enemy->pPlayer->playerRoot.pParent == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        scene =
            (sceneObject *)
                enemy->pPlayer->playerRoot.pParent;
        physics = (physicsObject *)scene->pPhysics;
        if (physics == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }

        movement_mode = variables[0].sw[0];
        if (movement_mode < 0 || movement_mode > 6) {
            movement_mode = 0;
        }
        enemy->movementMode =
            (int16_t)movement_mode;
        switch (movement_mode) {
        case 2:
            physics->movemode = MOVE_HOVER;
            physics->flags &= ~UINT32_C(0x2000);
            break;
        case 3:
            physics->movemode = MOVE_HOVER3D;
            physics->flags |= UINT32_C(0x2000);
            break;
        case 4:
            physics->movemode = MOVE_FLY;
            physics->flags |= UINT32_C(0x2000);
            break;
        default:
            physics->movemode = MOVE_NORMAL;
            physics->flags &= ~UINT32_C(0x2000);
            break;
        }
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }

    case 0x40f:
        enemy->aiTimer = (int32_t)gGlobalTimer;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x40c:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 2);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        if (LevelSelect == 15 &&
            enemy->enemyID == 0x49) {
            moveTaxi = 1;
        }
        if (variables[1].si == 0) {
            enemy->exit_flag = 1;
        } else {
            aisub_arithmeticSIVariables(
                &enemy->range,
                variables[0].si,
                variables[1].si);
        }
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x410:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        enemy->lastWayPoint =
            variables[0].si > 0
                ? variables[0].si
                : 0;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x411:
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x412:
    {
        int value;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 3);
        if (variables == NULL ||
            variables[0].si < 0 ||
            variables[0].si >= 5) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        value =
            (int)enemy->counter[variables[0].si];
        aisub_arithmeticSIVariables(
            &value,
            variables[1].si,
            variables[2].si);
        enemy->counter[variables[0].si] =
            (uint32_t)value;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }

    case 0x602: {
        unsigned flag;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 1);
        if (variables == NULL ||
            variables[0].si < 0) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        flag = (unsigned)variables[0].si;
        if (flag >= sizeof(abGlobalBits) * 8U) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        *branch_flag =
            (abGlobalBits[flag >> 3] &
             (uint8_t)(1U << (flag & 7U))) != 0;
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }

    case 0x604:
        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 2);
        if (variables == NULL ||
            gpWorld == NULL ||
            gpWorld->player0 == NULL ||
            gpWorld->player1 == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        if (GameStruct.CurrentLevel == 5 &&
            enemy->aiNum == 0x10 &&
            GameStruct.checkpoint[5] >= 6) {
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }
        if (variables[0].si < 0) {
            gpWorld->overRideDolly = 0;
            gpWorld->player0->pFlags &=
                ~UINT32_C(2);
            gpWorld->player1->pFlags &=
                ~UINT32_C(2);
            game_clearLetterBox();
            GameStruct.screenShotFlag = 0;
        } else {
            gpWorld->overRideDolly =
                variables[0].sw[0];
            gpWorld->player0->pFlags |=
                UINT32_C(2);
            gpWorld->player1->pFlags |=
                UINT32_C(2);
            if (variables[1].si != 0) {
                game_setLetterBox();
                GameStruct.screenShotFlag = 1;
            }
        }
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

    case 0x606: {
        int command;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 3);
        if (variables == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        command = variables[0].si;
        if (command != 0) {
            *branch_flag = 1;
        }
        switch (command) {
        case 0: {
            wsl_BAP_PLACEMENT *destination;
            sceneObject *scene;
            physicsObject *physics;

            if (gpWorld == NULL ||
                gpWorld->apEnemy == NULL ||
                variables[1].si < 0 ||
                variables[1].si >=
                    gpWorld->nEnemy ||
                enemy->pPlayer == NULL ||
                enemy->pPlayer->playerRoot.pParent ==
                    NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            destination =
                gpWorld->apEnemy[variables[1].si];
            if (destination == NULL) {
                return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
            }
            scene =
                (sceneObject *)
                    enemy->pPlayer->playerRoot.pParent;
            physics = (physicsObject *)scene->pPhysics;
            if (physics == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }

            {
                VECTOR offset;

                offset.vx =
                    (int32_t)(
                        (float)destination->loc.vx -
                        physics->pos.vx);
                offset.vy =
                    (int32_t)(
                        (float)destination->loc.vy -
                        physics->pos.vy);
                offset.vz =
                    (int32_t)(
                        (float)destination->loc.vz -
                        physics->pos.vz);
                offset.pad = 0;
                enemy_SetTeleport(
                    &physics->vpos,
                    &offset,
                    (int32_t)(
                        variables[2].f * 256.0f),
                    enemy->enemyID);
            }
            *branch_flag = 1;
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }

        case 1: {
            wsl_Powerup *powerup;
            int rate;

            if (gpWorld == NULL ||
                gpWorld->pPowerups == NULL ||
                variables[1].si < 0 ||
                variables[1].si >=
                    gpWorld->nPowerups) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            powerup =
                &gpWorld->pPowerups[variables[1].si];
            rate = (int)(variables[2].f * 30.0f);
            if (rate == 0) {
                powerup->data |= UINT16_C(0x8000);
            } else {
                powerup->rate = (uint8_t)rate;
                powerup->data &=
                    (uint16_t)~UINT16_C(0x8000);
            }
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }

        case 2:
            if (enemy->pPlayer == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            if (variables[1].si != 0) {
                enemy->pPlayer->pFlags |=
                    UINT32_C(0x10);
            } else {
                enemy->pPlayer->pFlags &=
                    ~UINT32_C(0x10);
            }
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

        case 3: {
            EffectHeader *effect;
            sceneObject *scene;
            physicsObject *physics;
            int effect_id = variables[1].si;

            if (effect_id < 0 ||
                effect_id >= JPB_EFFECT_COUNT ||
                paEffects[effect_id] == NULL ||
                enemy->pPlayer == NULL ||
                enemy->pPlayer->playerRoot.pParent ==
                    NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            scene =
                (sceneObject *)
                    enemy->pPlayer->playerRoot.pParent;
            physics = (physicsObject *)scene->pPhysics;
            if (physics == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            effect = paEffects[effect_id];
            (void)sprite_AddSpriteEffect(
                effect->aEffects,
                (int)effect->num,
                &physics->vpos,
                NULL);
            if (effect_id == 49) {
                (void)sound_Play(
                    &physics->vpos,
                    3,
                    (char *)"teleprt2",
                    0);
            } else if (
                effect_id == 10 ||
                effect_id == 11 ||
                effect_id == 17 ||
                effect_id == 18) {
                (void)sound_Play(
                    &physics->vpos,
                    0,
                    (char *)"explomed",
                    0);
            }
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }

        case 4:
            camera_SetShake(6);
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

        case 6:
        case 7:
            if (enemy->pPlayer == NULL ||
                enemy->pPlayer->paMotions == NULL ||
                enemy->pPlayer->maxMotions <=
                    (command == 6 ? 0 : 6)) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            enemy->pPlayer
                ->paMotions[command == 6 ? 0 : 6]
                .globalID = variables[1].uw[0];
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

        case 8:
            if (gpWorld == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            gpWorld->overRideDolly =
                variables[1].si < 0
                    ? 0
                    : variables[1].sw[0];
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

        case 9: {
            sceneObject *scene;
            physicsObject *physics;

            if (enemy->pPlayer == NULL ||
                enemy->pPlayer->playerRoot.pParent ==
                    NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            scene =
                (sceneObject *)
                    enemy->pPlayer->playerRoot.pParent;
            physics = (physicsObject *)scene->pPhysics;
            if (physics == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            enemy_KillKill(
                &physics->vpos,
                (int)(variables[2].f * 256.0f));
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }

        case 10:
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

        case 11:
            level_SparkRoom();
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

        case 12: {
            sceneObject *scene;
            physicsObject *physics;

            if (enemy->pPlayer == NULL ||
                enemy->pPlayer->playerRoot.pParent ==
                    NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            scene =
                (sceneObject *)
                    enemy->pPlayer->playerRoot.pParent;
            physics = (physicsObject *)scene->pPhysics;
            if (physics == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            if (variables[1].si != 0) {
                physics->flags ^=
                    UINT32_C(0x400000);
            }
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }

        case 14:
        case 15:
        case 19:
        case 20: {
            sceneObject *scene;
            physicsObject *physics;
            int range =
                (int)(variables[2].f * 256.0f);

            if (gpWorld == NULL ||
                enemy->pPlayer == NULL ||
                enemy->pPlayer->playerRoot.pParent ==
                    NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            scene =
                (sceneObject *)
                    enemy->pPlayer->playerRoot.pParent;
            physics = (physicsObject *)scene->pPhysics;
            if (physics == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            uberPos.vx = (int32_t)physics->pos.vx;
            uberPos.vy = (int32_t)physics->pos.vy;
            uberPos.vz = (int32_t)physics->pos.vz;
            if (command == 14 || command == 19) {
                uberXRange = range;
            } else {
                uberZRange = range;
            }
            if (command == 14 || command == 15) {
                gpWorld->overRideDolly =
                    range == 0
                        ? 0
                        : gpWorld->currentDolly;
            } else if (range == 0) {
                gpWorld->overRideDolly = -1;
                uberLock = 0;
            } else {
                gpWorld->overRideDolly =
                    gpWorld->currentDolly;
                uberLock = 1;
            }
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }

        case 16: {
            int track = variables[1].si;
            int loop = variables[2].si;

            if (gpWorld == NULL ||
                gpWorld->player0 == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            if ((((unsigned)track - 44U) &
                 UINT32_C(0xfffffff7)) == 0) {
                track +=
                    gpWorld->player0->currentMotion %
                    5;
            } else if (
                (((unsigned)track - 60U) &
                 UINT32_C(0xffffffef)) == 0) {
                if (GameStruct.gameMode == 2) {
                    track += 8;
                }
                track +=
                    gpWorld->player0->currentMotion %
                    5;
            }
            if ((loop == 0 || loop == 1) &&
                OptionStruct.Music == 1) {
                playXA(
                    track,
                    (int)OptionStruct.musicVolume *
                        2,
                    loop);
            }
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }

        case 17: {
            sceneObject *scene;
            physicsObject *physics;
            VECTOR blast;
            int force = variables[1].si;

            if (enemy->pPlayer == NULL ||
                enemy->pPlayer->playerRoot.pParent ==
                    NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            scene =
                (sceneObject *)
                    enemy->pPlayer->playerRoot.pParent;
            physics = (physicsObject *)scene->pPhysics;
            if (physics == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }

            (void)BigBlowMe(
                &physics->vpos, force);
            blast.vx =
                (int32_t)physics->pos.vx + 0x100;
            blast.vy =
                (int32_t)physics->pos.vy + 0x80;
            blast.vz =
                (int32_t)physics->pos.vz;
            blast.pad = 0;
            (void)BigBlowMe(&blast, force);
            blast.vx -= 0x200;
            (void)BigBlowMe(&blast, force);
            blast.vx += 0x100;
            blast.vz += 0x100;
            (void)BigBlowMe(&blast, force);
            blast.vz -= 0x200;
            (void)BigBlowMe(&blast, force);
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }

        case 18: {
            playerObject *target;
            sceneObject *source_scene;
            sceneObject *target_scene;
            physicsObject *source_physics;
            physicsObject *target_physics;
            int object_id;

            if (gpWorld == NULL ||
                enemy->pPlayer == NULL ||
                enemy->pPlayer->playerRoot.pParent ==
                    NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            target =
                variables[1].si == 0
                    ? gpWorld->player0
                    : gpWorld->player1;
            if (target == NULL ||
                target->playerRoot.pParent == NULL ||
                target->playernum < 0 ||
                target->playernum >=
                    JPB_GAME_CHARACTER_CAPACITY) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }

            source_scene =
                (sceneObject *)
                    enemy->pPlayer->playerRoot.pParent;
            target_scene =
                (sceneObject *)
                    target->playerRoot.pParent;
            source_physics =
                (physicsObject *)source_scene->pPhysics;
            target_physics =
                (physicsObject *)target_scene->pPhysics;
            object_id =
                target->playerRoot.objectID;
            if (source_physics == NULL ||
                target_physics == NULL ||
                target_scene->pModel == NULL ||
                target_scene->pAnim == NULL ||
                target->paMotions == NULL ||
                object_id < 0 ||
                object_id >=
                    JPB_PLAYER_CAPACITY ||
                target !=
                    &gaPlayerData[object_id] ||
                target_physics !=
                    &maPhysicsData[object_id] ||
                target_scene->pAnim !=
                    &maAnimationData[
                        object_id].animRoot) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }

            if (game_gGetEnergy(
                    target->playernum) == 0 ||
                (target->pFlags &
                 UINT32_C(0x200)) != 0 ||
                afterLife == target) {
                int force =
                    game_gGetForce(object_id);
                int score =
                    game_gGetScore(object_id);
                int counter =
                    GameStruct.Counter;

                if (afterLife == NULL &&
                    GameStruct.NumPlayers == 2) {
                    player_AfterLife(target);
                    afterLife = target;
                }
                player_RefreshPlayer(target);
                game_gSetScore(
                    afterLife->playerRoot.objectID,
                    score);
                GameStruct.Counter = counter;
                game_gSetForce(object_id, force);
                if (target->shadow == NULL) {
                    target->shadow =
                        (int32_t *)(void *)
                            sprite_GetBaseNodeMarker(
                                object_id, 0x30);
                }
                afterLife = NULL;
                if (GameStruct.NumPlayers == 2) {
                    camera_SetCurrentCameraType(0);
                }
            }

            target_physics->pos =
                source_physics->pos;
            target_physics->angle =
                source_physics->angle;
            physics_ResetJedi(object_id);
            anim_ResetJedi(object_id);
            player_ResetJedi(object_id);
            target->pFlags |= UINT32_C(2);
            GameStruct.GameState &=
                ~UINT32_C(0x1000);
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }

        case 21:
            if (enemy->pPlayer == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            physics_gSetFacing(
                &enemy->pPlayer->playerRoot, 0);
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

        case 22:
            if (variables[1].si < 0 ||
                variables[1].si >=
                    JPB_ALL_TEXT_CAPACITY ||
                allText[variables[1].si] == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            menu_specialMess(
                (uint8_t *)(void *)
                    allText[variables[1].si]);
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

        case 23:
            if (gpWorld == NULL ||
                gpWorld->player0 == NULL ||
                gpWorld->player1 == NULL) {
                return
                    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
            }
            if (variables[1].si == 0) {
                gpWorld->player0->pFlags &=
                    ~UINT32_C(2);
            } else {
                gpWorld->player1->pFlags &=
                    ~UINT32_C(2);
            }
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;

        default:
            if (diagnostic &&
                unsupported_opcode != NULL) {
                *unsupported_opcode = opcode;
            }
            return diagnostic
                ? JPB_ENEMY_OPCODE_PARSE_UNSUPPORTED
                : JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }
    }

    case 0x607: {
        int extension;
        int enemy_id;
        int pointer_index;
        wsl_ENEMY *vehicle_enemy;
        playerObject *vehicle;
        sceneObject *vehicle_scene;
        physicsObject *vehicle_physics;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 2);
        ++jpb_enemy_vehicle_diagnostics.opcode607Count;
        jpb_enemy_vehicle_diagnostics.lastOpcode607SourceID =
            enemy->enemyID;
        jpb_enemy_vehicle_diagnostics.lastOpcode607Stage = 1;
        if (variables == NULL ||
            enemy->pPlace == NULL ||
            gpWorld == NULL ||
            gpWorld->apEnemy == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        extension = variables[1].si;
        jpb_enemy_vehicle_diagnostics.lastOpcode607Extension =
            extension;
        jpb_enemy_vehicle_diagnostics.lastOpcode607Stage = 2;
        if (extension < 0 || extension >= 12) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        enemy_id =
            enemy->pPlace->aiDf.enemyExt[extension];
        jpb_enemy_vehicle_diagnostics.lastOpcode607LinkedEnemyID =
            enemy_id;
        jpb_enemy_vehicle_diagnostics.lastOpcode607Stage = 3;
        if (enemy_id < 0 ||
            enemy_id >= gpWorld->nEnemy ||
            gpWorld->apEnemy[enemy_id] == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        /*
         * Exact opcode 0x607 RVAs 0x4A922..0x4A946 read the linked
         * placement's pLastEnemy field and pass it directly to getPtr type
         * 6. enemy_getPointerIndex is used by other linked-enemy paths, but
         * its level-specific adjustment does not belong to vehicle entry.
         */
        pointer_index =
            (int)gpWorld->apEnemy[enemy_id]->pLastEnemy;
        jpb_enemy_vehicle_diagnostics.lastOpcode607PointerIndex =
            pointer_index;
        jpb_enemy_vehicle_diagnostics.lastOpcode607Stage = 4;
        vehicle_enemy =
            pointer_index < 0
                ? NULL
                : (wsl_ENEMY *)getPtr(
                      pointer_index,
                      JPB_POINTER_ARRAY_ENEMY);
        if (vehicle_enemy == NULL ||
            vehicle_enemy->pPlayer == NULL ||
            vehicle_enemy->pPlayer
                    ->playerRoot.pParent == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }
        vehicle = vehicle_enemy->pPlayer;
        jpb_enemy_vehicle_diagnostics.lastOpcode607PlayerID =
            vehicle->playerID;
        jpb_enemy_vehicle_diagnostics.lastOpcode607CallbackIndex = -1;
        for (pointer_index = 0;
             pointer_index < JPB_PLAYER_CALLBACK_CAPACITY;
             ++pointer_index) {
            if (vehicle->pMainCallBack == funcArray[pointer_index]) {
                jpb_enemy_vehicle_diagnostics
                    .lastOpcode607CallbackIndex = pointer_index;
                break;
            }
        }
        jpb_enemy_vehicle_diagnostics.lastOpcode607Stage = 5;
        vehicle_scene =
            (sceneObject *)
                vehicle->playerRoot.pParent;
        vehicle_physics =
            (physicsObject *)
                vehicle_scene->pPhysics;
        if (vehicle_physics == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        jpb_enemy_vehicle_diagnostics.lastOpcode607Stage = 6;

        if (vehicle->playerID == 0x23) {
            int driver_index;

            tankID =
                vehicle->playerRoot.objectID;
            for (driver_index = 0;
                 driver_index < 2;
                 ++driver_index) {
                playerObject *driver =
                    driver_index == 0
                        ? gpWorld->player0
                        : gpWorld->player1;
                sceneObject *driver_scene;
                physicsObject *driver_physics;

                if (driver == NULL ||
                    driver->playerRoot.pParent ==
                        NULL) {
                    continue;
                }
                driver_scene =
                    (sceneObject *)
                        driver->playerRoot.pParent;
                driver_physics =
                    (physicsObject *)
                        driver_scene->pPhysics;
                if (driver_physics != NULL &&
                    jpb_enemy_axis_distance_within(
                        &driver_physics->vpos,
                        &vehicle_physics->vpos,
                        0x200) &&
                    (driver->pFlags &
                     UINT32_C(1)) != 0 &&
                    timesincetank[driver_index] ==
                        0 &&
                    (driver->pFlags &
                     UINT32_C(0x80)) == 0 &&
                    driver->pForceCallBack ==
                        NULL) {
                    getintank(
                        vehicle_enemy,
                        driver_index);
                }
            }
        } else if (
            vehicle->playerID == 0x17 &&
            vehicle->pMainCallBack !=
                funcArray[31]) {
            int driver_index;

            ++jpb_enemy_vehicle_diagnostics.stapCandidateCount;
            jpb_enemy_vehicle_diagnostics.lastOpcode607Stage = 7;

            for (driver_index = 0;
                 driver_index < 2;
                 ++driver_index) {
                playerObject *driver =
                    driver_index == 0
                        ? gpWorld->player0
                        : gpWorld->player1;
                sceneObject *driver_scene;
                physicsObject *driver_physics;

                if (driver == NULL ||
                    driver->playerRoot.objectID ==
                        -1 ||
                    driver->playerRoot.pParent ==
                        NULL ||
                    (driver->pFlags &
                     UINT32_C(0x40200)) != 0) {
                    continue;
                }
                driver_scene =
                    (sceneObject *)
                        driver->playerRoot.pParent;
                driver_physics =
                    (physicsObject *)
                        driver_scene->pPhysics;
                if (driver_scene->pScene != NULL &&
                    driver_physics != NULL &&
                    obj_gCheckObjectFlag(
                        &driver->playerRoot,
                        0,
                        UINT32_C(0x20)) == 0 &&
                    jpb_enemy_axis_distance_within(
                        &driver_physics->vpos,
                        &vehicle_physics->vpos,
                        0x100)) {
                    getonstap(
                        vehicle_enemy,
                        driver_index);
                    break;
                }
            }
        }
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }

    case 0x60f: {
        wsl_BAP_PLACEMENT *placement;
        int extension;
        int enemy_id;
        int new_ai;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 2);
        if (variables == NULL ||
            enemy->pPlace == NULL ||
            gpWorld == NULL ||
            gpWorld->apEnemy == NULL ||
            gpWorld->apAI == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        extension = variables[0].si;
        new_ai = variables[1].si;
        if (extension < 0 || extension >= 12 ||
            new_ai < 0 || new_ai >= gpWorld->nAI) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        enemy_id =
            enemy->pPlace->aiDf.enemyExt[extension];
        if (enemy_id < 0 ||
            enemy_id >= gpWorld->nEnemy) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        placement = gpWorld->apEnemy[enemy_id];
        if (placement == NULL) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }

        /*
         * Matched enemy_ParseOpcodes RVAs 0x4A660..0x4A670 read offset
         * 0x34 in wsl_BAP_PLACEMENT, the PDB-backed
         * wsl_BAPAI_DEFAULTS.ownerType field.  Offset 0xC8 is the mutable
         * placement status and is cleared by enemy_InitEnemies; using it
         * here turns authored player proxies into ordinary level actors.
         */
        if (placement->aiDf.ownerType == 4 ||
            placement->aiDf.ownerType == 5) {
            return
                jpb_enemy_handle_player_placement(
                    placement,
                    enemy_id,
                    new_ai,
                    opcode,
                    unsupported_opcode);
        }
        if (placement != enemy->pPlace) {
            if (_addEnemy(
                    placement,
                    enemy_id,
                    new_ai,
                    1) != 0) {
                placement->status = 1;
            }
        } else {
            enemy->pAI = gpWorld->apAI[new_ai];
            enemy->aiNum = new_ai;
            enemy->aiLocation = 0;
            enemy->pAINode = NULL;
            memset(enemy->counter, 0,
                   sizeof(enemy->counter));
            enemy->aiTimer = 0;
            enemy->currAIMode = 0;
            memset(enemy->prevAIMode, 0,
                   sizeof(enemy->prevAIMode));
            enemy->stackID = 0;
            memset(enemy->switchData, 0, 4);
        }
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }

    case 0x610: {
        unsigned flag;
        uint8_t mask;

        variables = jpb_enemy_resolve_opcode_variables(
            enemy, node, 2);
        if (variables == NULL ||
            variables[0].si < 0) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        flag = (unsigned)variables[0].si;
        if (flag >= sizeof(abGlobalBits) * 8U) {
            return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
        }
        mask = (uint8_t)(1U << (flag & 7U));
        if (variables[1].si == 0) {
            abGlobalBits[flag >> 3] &=
                (uint8_t)~mask;
        } else if (LevelSelect != 8 ||
                   flag != 99U) {
            abGlobalBits[flag >> 3] |= mask;
        }
        return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
    }

    default:
        if (diagnostic && unsupported_opcode != NULL) {
            *unsupported_opcode = opcode;
        }
        if (!diagnostic) {
            nearestDebugRange = 0;
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }
        return JPB_ENEMY_OPCODE_PARSE_UNSUPPORTED;
    }
}

static JPBEnemyOpcodeParseResult
jpb_enemy_parse_opcodes_internal(
    wsl_ENEMY *enemy,
    uint16_t *unsupported_opcode,
    int diagnostic)
{
    BAP_AINODE *node;
    int instruction_limit;
    int instructions = 0;

    if (unsupported_opcode != NULL) {
        *unsupported_opcode = 0;
    }
    if (enemy == NULL ||
        enemy->pAI == NULL ||
        enemy->pAI->numNodes < 0) {
        return JPB_ENEMY_OPCODE_PARSE_INVALID_DATA;
    }
    if (enemy->pPlayer != NULL) {
        enemy->pPlayer->pFlags &=
            ~UINT32_C(0x80000000);
    }

    /*
     * enemy_ParseOpcodes enters each authored cycle at the root child.  A
     * 0x106 mode-jump opcode is what redirects traversal through
     * bapEnemyDoModeJump later in the cycle.  Starting at the mode target
     * skips the root-side conditions and lets scene-director scripts run
     * ahead of their authored gates.
     */
    node = bapEnemyStartCycleLoop(enemy);
    instruction_limit =
        enemy->pAI->numNodes > 0
            ? enemy->pAI->numNodes * 2
            : 1;
    while (node != NULL) {
        JPBEnemyOpcodeParseResult result;
        double opcode_started;
        double opcode_seconds;
        uint16_t encoded_opcode;
        uint16_t opcode;
        int branch_flag;

        ++instructions;
        ++jpb_enemy_frame_profile.lastParseInstructions;
        if (jpb_enemy_frame_profile.lastParseInstructions >
            jpb_enemy_frame_profile.maxParseInstructions) {
            jpb_enemy_frame_profile.maxParseInstructions =
                jpb_enemy_frame_profile.lastParseInstructions;
        }
        if (diagnostic &&
            instructions > instruction_limit) {
            return
                JPB_ENEMY_OPCODE_PARSE_LIMIT_REACHED;
        }
        if ((uint16_t)node->opcode ==
            UINT16_C(0x0200)) {
            return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
        }

        encoded_opcode = (uint16_t)node->opcode;
        opcode =
            (encoded_opcode & UINT16_C(0x4000)) != 0
                ? encoded_opcode & UINT16_C(0x0fff)
                : encoded_opcode;
        opcode_started = jpb_enemy_profile_seconds();
        result = jpb_enemy_execute_authored_opcode(
            enemy,
            node,
            &branch_flag,
            unsupported_opcode,
            diagnostic);
        opcode_seconds =
            jpb_enemy_profile_seconds() - opcode_started;
        jpb_enemy_profile_record_opcode(
            enemy, node, opcode, opcode_seconds);
        if (result !=
            JPB_ENEMY_OPCODE_PARSE_COMPLETE) {
            return result;
        }

        if (branch_flag < 0) {
            node = bapEnemyDoModeJump(enemy);
        } else {
            node = bapEnemyGetNextOpcode(
                enemy, branch_flag != 0);
        }
    }
    return JPB_ENEMY_OPCODE_PARSE_COMPLETE;
}

/*
 * Portable diagnostic companion to exact enemy_ParseOpcodes. Unlike the
 * matched void owner, it bounds malformed cycles and reports unknown opcode
 * data so real-asset gates cannot hang or silently lose coverage.
 */
JPBEnemyOpcodeParseResult jpb_enemy_ParseOpcodes(
    wsl_ENEMY *enemy,
    uint16_t *unsupported_opcode)
{
    return jpb_enemy_parse_opcodes_internal(
        enemy, unsupported_opcode, 1);
}

/* 0x48E60, 7884 bytes, global, 81 named locals
 * enemy_ParseOpcodes
 * PDB type: void (wsl_ENEMY*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void enemy_ParseOpcodes(wsl_ENEMY *enemy)
{
    (void)jpb_enemy_parse_opcodes_internal(
        enemy, NULL, 0);
}

/* 0x4AD30, 947 bytes, global, 23 named locals
 * enemy_Radar
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */

/* 0x4B0F0, 1623 bytes, global, 6 named locals
 * enemy_ResetEnemies
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void enemy_Radar(void)
{
    CVECTOR color;
    float centerX;
    VECTOR rot;
    float scaleX;
    float scaleY;
    float w;
    float h;
    SCREENRECT rectBackground;
    float centerY;
    wsl_ENEMY *bpEnemy;
    float x;
    SCREENRECT rectPlayer;
    CVECTOR playerColor;
    float playerY;
    float y;
    float playerX;
    float playerW;
    VECTOR off;
    float playerH;
    VECTOR *pos;
    VECTOR *tpos;
    SCREENRECT rectEnemy;
    CVECTOR colorV;
    const float layerDepth = 0.0001f;

    if (LevelSelect == 0 || gpWorld == NULL) {
        return;
    }

    scaleX = (float)OptionStruct.ScreenWidth / 640.0f;
    scaleY = (float)OptionStruct.ScreenHeight / 480.0f;
    x = scaleX * 284.0f;
    y = scaleY * 31.0f;
    w = scaleX * 71.0f;
    h = scaleY * 106.0f;
    rectBackground.left = (int)x;
    rectBackground.top = (int)y;
    rectBackground.right = (int)(x + w);
    rectBackground.bottom = (int)(y + h);
    color.r = 0;
    color.g = 0;
    color.b = 0;
    color.cd = UINT8_C(0x9f);
    _DrawTexture(
        transHandle,
        rectBackground,
        NULL,
        color,
        layerDepth);

    playerW = w / 50.0f;
    playerH = h / 50.0f;
    centerX = w * 0.5f + x - playerW * 0.5f;
    centerY = h * 0.5f + y - playerH * 0.5f;
    playerX = centerX;
    playerY = centerY;
    rectPlayer.left = (int)playerX;
    rectPlayer.top = (int)playerY;
    rectPlayer.right = (int)(playerX + playerW);
    rectPlayer.bottom = (int)(playerY + playerH);
    playerColor.r = UINT8_C(0xff);
    playerColor.g = UINT8_C(0xff);
    playerColor.b = UINT8_C(0xff);
    playerColor.cd = UINT8_C(0xff);
    _DrawTexture(
        transHandle,
        rectPlayer,
        NULL,
        playerColor,
        layerDepth);

    for (bpEnemy = (wsl_ENEMY *)
             enemyList[mCurEnemyList].head;
         bpEnemy != NULL;
         bpEnemy = (wsl_ENEMY *)bpEnemy->node.next) {
        sceneObject *scene;
        physicsObject *enemyObj;
        int owner_type;

        if (bpEnemy->active != 1 ||
            bpEnemy->pPlace == NULL ||
            bpEnemy->pPlayer == NULL) {
            continue;
        }
        owner_type = bpEnemy->pPlace->aiDf.ownerType;
        if (owner_type != 2 && owner_type != 3) {
            continue;
        }
        scene = (sceneObject *)(void *)
            bpEnemy->pPlayer->playerRoot.pParent;
        enemyObj = scene != NULL
            ? (physicsObject *)(void *)scene->pPhysics
            : NULL;
        if (enemyObj == NULL) {
            continue;
        }

        pos = (VECTOR *)(void *)&gpWorld->location;
        tpos = &enemyObj->vpos;
        if (vec_QuickRangeCheck(pos, tpos, 0x0c00) == 0) {
            continue;
        }
        off.vx = tpos->vx - pos->vx;
        off.vy = tpos->vy - pos->vy;
        off.vz = tpos->vz - pos->vz;
        off.pad = 0;
        (void)vec_RotVectorY(
            0x800 - gCamera.angle.vy,
            &off,
            &rot);
        rot.vx /= 64;
        rot.vz /= 64;

        if (owner_type == 3) {
            colorV.r = UINT8_C(0x20);
            colorV.g = UINT8_C(0xff);
        } else {
            colorV.r = UINT8_C(0xff);
            colorV.g = UINT8_C(0x20);
        }
        colorV.b = UINT8_C(0x20);
        colorV.cd = UINT8_C(0xff);
        rectEnemy.left = (int)(centerX - (float)rot.vx);
        rectEnemy.top = (int)(centerY - (float)rot.vz);
        rectEnemy.right =
            (int)(centerX - (float)rot.vx + playerW);
        rectEnemy.bottom =
            (int)(centerY - (float)rot.vz + playerH);
        _DrawTexture(
            transHandle,
            rectEnemy,
            NULL,
            colorV,
            layerDepth);
    }
}
void enemy_ResetEnemies(void)
{
    wsl_ENEMY *enemy;
    int index;

    while ((enemy =
                (wsl_ENEMY *)list_RemoveHead(
                    &enemyList[mCurEnemyList])) != NULL) {
        _deleteEnemy(enemy, 0);
        list_AddTail(&enemyFreeList, &enemy->node);
    }

    for (index = 0; index < gpWorld->nEnemy; ++index) {
        wsl_BAP_PLACEMENT *placement =
            gpWorld->apEnemy[index];

        placement->aiDf.activeFlags &=
            ~UINT32_C(0x10000000);
        placement->status = 0;
        placement->pLastEnemy = UINT32_MAX;
    }

    abGlobalBits[0] &= UINT8_C(0xfe);
    for (index = 18; index < 128; ++index) {
        abGlobalBits[index >> 3] &=
            (uint8_t)~(UINT8_C(1) << (index & 7));
    }
}

/* 0x4B750, 112 bytes, global, 4 named locals
 * enemy_SetTeleport
 * PDB type: void (VECTOR*, VECTOR*, int, int...
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void enemy_SetTeleport(
    VECTOR *position,
    VECTOR *offset,
    int range,
    int target)
{
    if (GameStruct.CurrentLevel == 9) {
        savedPlayerPos.vx = position->vx;
        savedPlayerPos.vy = position->vy;
        savedPlayerPos.vz = position->vz;
    }
    tpos.vx = position->vx;
    tpos.vy = position->vy;
    tpos.vz = position->vz;
    toff.vx = offset->vx;
    toff.vy = offset->vy;
    toff.vz = offset->vz;
    trange = range;
    tflag = 1;
    tele = target;
}

/* 0x4B7C0, 189 bytes, global, 9 named locals
 * enemy_SetTeleportReturn
 * PDB type: void (VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
void enemy_SetTeleportReturn(VECTOR *position)
{
    sceneObject *scene0 =
        (sceneObject *)gpWorld->player0->playerRoot.pParent;
    sceneObject *scene1 =
        (sceneObject *)gpWorld->player1->playerRoot.pParent;
    physicsObject *physics0 =
        (physicsObject *)scene0->pPhysics;
    physicsObject *physics1 =
        (physicsObject *)scene1->pPhysics;
    int x_offset = 0;
    int z_offset = -200;
    int y = savedPlayerPos.vy;
    int x;

    (void)position;
    if (GameStruct.NumPlayers == 2) {
        x_offset = 100;
        z_offset = -100;
    }
    z_offset += savedPlayerPos.vz;
    tpos.vx = savedPlayerPos.vx;
    tpos.vy = savedPlayerPos.vy;
    tpos.vz = savedPlayerPos.vz;
    x = savedPlayerPos.vx + 500;
    physics0->pos.vx = (float)x;
    physics0->pos.vy = (float)y;
    physics0->pos.vz =
        (float)(savedPlayerPos.vz + x_offset);
    physics1->pos.vx = (float)x;
    physics1->pos.vy = (float)y;
    physics1->pos.vz = (float)z_offset;
    gCamera.viewType &= ~INT32_C(0x1000);
}

/* 0x4B880, 118 bytes, global, 3 named locals
 * enemy_getPointerIndex
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
int enemy_getPointerIndex(int enemyID)
{
    int offset = 1;
    uint8_t level = GameStruct.CurrentLevel;

    if ((uint8_t)(level - 12U) <= 1U ||
        level == 1 ||
        level == 4 ||
        level == 5) {
        offset = 0;
    } else if (level == 9) {
        if ((uint32_t)enemyID <= 58U &&
            (UINT64_C(0x0512600000000000) &
             (UINT64_C(1) << (uint32_t)enemyID)) != 0) {
            offset = 0;
        }
    } else if (level == 3 ||
               level == 6 ||
               level == 15 ||
               level == 2) {
        offset = 0;
    }

    return (int)gpWorld->apEnemy[enemyID]->pLastEnemy -
           offset;
}

/* 0x4B900, 289 bytes, local, 4 named locals
 * getintank
 * PDB type: void (wsl_ENEMY*, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
static void getintank(
    wsl_ENEMY *enemy, int driver_index)
{
    playerObject *driver;
    playerObject *vehicle;
    sceneObject *driver_scene;
    sceneObject *vehicle_scene;
    modelObject *driver_model;
    physicsObject *driver_physics;
    physicsObject *vehicle_physics;
    uint32_t driver_flag;

    if (enemy == NULL ||
        enemy->pPlayer == NULL ||
        (unsigned)driver_index >= 2U) {
        return;
    }
    driver = &gaPlayerData[driver_index];
    vehicle = enemy->pPlayer;
    if (driver->playerRoot.pParent == NULL ||
        vehicle->playerRoot.pParent == NULL ||
        vehicle->playernum < 0 ||
        vehicle->playernum >=
            JPB_GAME_CHARACTER_CAPACITY) {
        return;
    }
    driver_scene =
        (sceneObject *)driver->playerRoot.pParent;
    vehicle_scene =
        (sceneObject *)vehicle->playerRoot.pParent;
    driver_model =
        (modelObject *)driver_scene->pModel;
    driver_physics =
        (physicsObject *)driver_scene->pPhysics;
    vehicle_physics =
        (physicsObject *)vehicle_scene->pPhysics;
    if (driver_model == NULL ||
        driver_physics == NULL ||
        vehicle_physics == NULL) {
        return;
    }

    driver->pFlags |= UINT32_C(0x80);
    driver_physics->flags =
        (driver_physics->flags &
         UINT32_C(0xffffffe0)) |
        (uint32_t)vehicle->playerRoot.objectID |
        UINT32_C(0xa0);
    driver_model->flags |= UINT32_C(0x10);
    driver_flag =
        UINT32_C(1) <<
        (unsigned)(driver_index + 26);
    /* Exact getintank RVA 0x4B96C stores the rider bit at enemy + 0x28. */
    enemy->enemyFlags |= driver_flag;
    (void)game_gSetEnergy(
        vehicle->playernum, 0xfe);
    vehicle->pMainCallBack = funcArray[30];
    driver_physics->flags |=
        UINT32_C(0x100);
    jumpheld[driver_index] = 1;
    if (tankdrivers[0] != NULL) {
        tankdrivers[1] = driver;
    } else {
        tankdrivers[0] = driver;
    }
}

/* 0x4BA30, 296 bytes, local, 4 named locals
 * getonstap
 * PDB type: void (wsl_ENEMY*, int)
 * Source: W:\SWJediPowerBattles\Work\enemy.c
 */
static void getonstap(
    wsl_ENEMY *enemy, int driver_index)
{
    playerObject *driver;
    playerObject *vehicle;
    sceneObject *driver_scene;
    sceneObject *vehicle_scene;
    physicsObject *driver_physics;
    physicsObject *vehicle_physics;
    uint32_t driver_flag;

    ++jpb_enemy_vehicle_diagnostics.stapAttachAttemptCount;

    if (enemy == NULL ||
        enemy->pPlayer == NULL ||
        (unsigned)driver_index >= 2U) {
        return;
    }
    driver = &gaPlayerData[driver_index];
    vehicle = enemy->pPlayer;
    if (driver->playerRoot.pParent == NULL ||
        vehicle->playerRoot.pParent == NULL ||
        driver->paMotions == NULL ||
        driver->maxMotions <= 78 ||
        vehicle->playernum < 0 ||
        vehicle->playernum >=
            JPB_GAME_CHARACTER_CAPACITY) {
        return;
    }
    driver_scene =
        (sceneObject *)driver->playerRoot.pParent;
    vehicle_scene =
        (sceneObject *)vehicle->playerRoot.pParent;
    driver_physics =
        (physicsObject *)driver_scene->pPhysics;
    vehicle_physics =
        (physicsObject *)vehicle_scene->pPhysics;
    if (driver_physics == NULL ||
        vehicle_physics == NULL ||
        (driver_physics->flags &
         UINT32_C(0x20)) != 0) {
        return;
    }

    jpb_enemy_vehicle_diagnostics.lastStapPositionBeforeAttach =
        vehicle_physics->pos;

    stapbikeindex[driver_index] =
        vehicle->playerRoot.objectID + 1;
    (void)animctrl_MotionNoLock(
        &driver_physics->physicsRoot,
        &driver->paMotions[78]);
    driver_physics->flags =
        (driver_physics->flags &
         UINT32_C(0xffffffe0)) |
        (uint32_t)vehicle->playerRoot.objectID |
        UINT32_C(0x4000a0);
    /*
     * Exact getonstap RVAs 0x4BAA0..0x4BAC1 address motion 78 through
     * driver->paMotions, then OR its motionFlags.  These are animation
     * ownership bits, not vehicle physics flags.
     */
    driver->paMotions[78].motionFlags |=
        UINT32_C(0xc0000000);
    driver_physics->solidgrabbed = NULL;
    driver_flag =
        UINT32_C(1) <<
        (unsigned)(driver_index + 26);
    /* Exact getonstap RVA 0x4BACD stores the rider bit at enemy + 0x28. */
    enemy->enemyFlags |= driver_flag;
    vehicle_physics->flags |=
        UINT32_C(0x400000);
    (void)game_gSetEnergy(
        vehicle->playernum, 0xff);
    vehicle_physics->airTime = 0;
    vehicle->pMainCallBack = funcArray[31];
    stapsound = 0;
    jpb_enemy_vehicle_diagnostics.lastStapPositionAfterAttach =
        vehicle_physics->pos;
    ++jpb_enemy_vehicle_diagnostics.stapAttachSuccessCount;
}
