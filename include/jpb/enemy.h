#ifndef JPB_ENEMY_H
#define JPB_ENEMY_H

#include "jpb/fmath.h"
#include "jpb/list.h"
#include "jpb/world.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern List enemyList[2];
extern List enemyFreeList;
extern int32_t mCurEnemyList;
extern int32_t gShowAI;
extern int32_t nEnemy;
extern wsl_ENEMY *pLatestDebugEnemy;
extern wsl_ENEMY *pDebugEnemy;
extern wsl_ENEMY aEnemyListNodes[20];

typedef struct JPBEnemyVehicleDiagnostics {
    uint32_t opcode607Count;
    uint32_t stapCandidateCount;
    uint32_t stapAttachAttemptCount;
    uint32_t stapAttachSuccessCount;
    uint32_t rangeEvaluationCount;
    uint32_t rangeSuccessCount;
    int32_t lastRangeEnemyID;
    int32_t lastRangeTarget;
    int32_t lastRangeCompare;
    int32_t lastRangeDistance0;
    int32_t lastRangeDistance1;
    float lastRangeThreshold;
    int32_t lastRangeResult;
    int32_t lastOpcode607SourceID;
    int32_t lastOpcode607Extension;
    int32_t lastOpcode607LinkedEnemyID;
    int32_t lastOpcode607PointerIndex;
    int32_t lastOpcode607PlayerID;
    int32_t lastOpcode607CallbackIndex;
    int32_t lastOpcode607Stage;
    FVECTOR lastStapPositionBeforeAttach;
    FVECTOR lastStapPositionAfterAttach;
} JPBEnemyVehicleDiagnostics;

void jpb_EnemyGetVehicleDiagnostics(
    JPBEnemyVehicleDiagnostics *diagnostics);
extern uint8_t abGlobalBits[16];
extern int32_t _aiFlagsTimer[4];
extern int32_t _aiFlagsSave[4];
extern int32_t _aiFlags[4];
enum { JPB_ENEMY_MODEL_ACCOUNT_CAPACITY = 80 };
/* Exact 80x3 loader accounting table at matched-PC RVA 0x538BA0. */
extern int32_t maModelID[JPB_ENEMY_MODEL_ACCOUNT_CAPACITY][3];
extern int32_t moveTaxi;
extern int32_t tele;
extern int32_t trange;
extern int32_t tflag;
extern VECTOR toff;
extern VECTOR savedPlayerPos;
extern VECTOR tpos;

typedef enum JPBEnemyOpcodeParseResult {
    JPB_ENEMY_OPCODE_PARSE_COMPLETE = 0,
    JPB_ENEMY_OPCODE_PARSE_UNSUPPORTED = 1,
    JPB_ENEMY_OPCODE_PARSE_INVALID_DATA = 2,
    JPB_ENEMY_OPCODE_PARSE_LIMIT_REACHED = 3
} JPBEnemyOpcodeParseResult;

int _addEnemy(
    wsl_BAP_PLACEMENT *pPlace,
    int id,
    int newAI,
    int forceon);
void _checkForNewEnemies(void);
void _deleteEnemy(wsl_ENEMY *enemy, int remove);
int _countChildNodes(wsl_ENEMY *enemy, BAP_AINODE *node);
void _debugEnemy(wsl_ENEMY *enemy);
void _debugEnemyFlags(void);
wsl_ENEMY *_initEnemy(wsl_BAP_PLACEMENT *placement);
void aisub_arithmeticFVariables(
    float *variable, int operation, float value);
void aisub_arithmeticSIVariables(
    int *variable, int operation, int value);
int aisub_checkFlag(int flag, int value);
void aisub_clearglobalflags(void);
int aisub_compareFVariables(
    float first, int comparison, float second);
int aisub_compareLogicSense(int comparison);
int aisub_compareSIVariables(
    int first, int comparison, int second);
int aisub_findNearestWaypnt(wsl_ENEMY *enemy);
void aisub_flagsManager(void);
int aisub_handleMoveFunction(
    wsl_ENEMY *bpEnemy,
    UDATA target,
    int anim,
    int nDelta);
int aisub_handleRangeFunction(
    wsl_ENEMY *bpEnemy, UDATA *vars);
int aisub_handleScanFunction(
    wsl_ENEMY *bpEnemy,
    int target,
    int delta,
    int absolute);
void aisub_setFlag(int flag, int value);
void aisub_setNextWaypoint(wsl_ENEMY *enemy, int waypoint);
void aisub_showflags(void);
void aisub_timedFlag(int flag, int value, int duration);
BAP_AINODE *bapEnemyDoModeJump(wsl_ENEMY *enemy);
BAP_AINODE *bapEnemyGetNextOpcode(wsl_ENEMY *enemy, int use_child);
BAP_AINODE *bapEnemySetContinue(wsl_ENEMY *enemy);
BAP_AINODE *bapEnemyStartCycleLoop(wsl_ENEMY *enemy);
void bapenemy_changeAIMode(wsl_ENEMY *enemy, int mode);
void bapenemy_postFrame(wsl_ENEMY *enemy);
int bapenemy_preFrame(wsl_ENEMY *enemy);
void bapenemy_returnAIMode(wsl_ENEMY *enemy);
int console_EnemyCommand(
    int argument_count,
    char **arguments,
    int *integer_arguments,
    float *float_arguments);
BAP_AINODE *enemy_GetNodePointer(wsl_ENEMY *enemy, int node_index);
void enemy_ActivateEnemy(int enemy_index);
int enemy_CalcPoints(int *enemy_count);
void enemy_CheckTeleport(void);
void enemy_HandleMapTriggers(int32_t *cube);
void enemy_InitEnemies(void);
void enemy_KillKill(VECTOR *position, int range);
void enemy_ResetEnemies(void);
void enemy_SetTeleport(
    VECTOR *position,
    VECTOR *offset,
    int range,
    int target);
void enemy_SetTeleportReturn(VECTOR *position);
int enemy_getPointerIndex(int enemyID);
void enemy_ParseOpcodes(wsl_ENEMY *enemy);
/*
 * Diagnostic form of enemy_ParseOpcodes used by portable validation. It
 * reports malformed/unknown input and bounds traversal; the original void
 * call surface preserves the matched valid-data behavior.
 */
JPBEnemyOpcodeParseResult jpb_enemy_ParseOpcodes(
    wsl_ENEMY *enemy,
    uint16_t *unsupported_opcode);
void enemy_HandleEnemies(void);
void enemy_Radar(void);
/*
 * Diagnostic facade for the exact active-enemy owner. It retains the
 * parser-boundary result needed by portable validation while
 * enemy_HandleEnemies preserves the original void call surface.
 */
JPBEnemyOpcodeParseResult jpb_enemy_ProcessActiveFrame(
    uint16_t *unsupported_opcode);
JPBEnemyOpcodeParseResult jpb_enemy_LastFrameResult(
    uint16_t *unsupported_opcode);

#ifdef __cplusplus
}
#endif

#endif
