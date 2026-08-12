#ifndef JPB_AI_H
#define JPB_AI_H

#include "jpb/fmath.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct playerObject playerObject;
typedef union UDATA UDATA;
typedef struct wsl_ENEMY wsl_ENEMY;

/* Exact matched-PC PDB type 0x117B. */
typedef struct aiData {
    int8_t ranged[2];
    int8_t hth[2];
    int8_t block[2];
    int8_t noblock[2];
    int8_t death[2];
    int8_t reload[2];
    int8_t violence[2];
    int8_t combos[2];
    int32_t flags;
} aiData;

enum {
    JPB_AI_MODEL_CAPACITY = 20,
    JPB_AI_LEVEL_CAPACITY = 4,
    JPB_AI_REFERENCE_CAPACITY = 0x1000
};

enum JPBAiLoadResult {
    JPB_AI_OK = 0,
    JPB_AI_INVALID_ARGUMENT = -1,
    JPB_AI_IO_ERROR = -2,
    JPB_AI_STORAGE_TOO_SMALL = -3,
    JPB_AI_INVALID_DATA = -4
};

int ai_CheckBounds(playerObject *player);
int ai_DefendCheck(playerObject *player);
int ai_Death(playerObject *player, int DEATH);
int ai_FindFarPlayer(
    playerObject *player,
    playerObject **target,
    int minimum_range_difference);
int ai_FindNearestPlayer(
    playerObject *player,
    playerObject **target);
int ai_FireWeapon(
    int32_t *cpad, playerObject *player);
int ai_HthAttack(
    wsl_ENEMY *pEnemy, UDATA *vars);
void ai_RangedAttack(
    wsl_ENEMY *pEnemy, UDATA *vars);
int ai_SeqAttack(
    wsl_ENEMY *pEnemy, UDATA *vars);
int ai_Throw(
    int32_t *cpad, playerObject *player);
void ai_SetTarget(
    playerObject *player, int target);
int ai_WalkToPoint(
    playerObject *player,
    int move,
    VECTOR *waypoint,
    int nDelta);
int ai_WalkWayPoints(
    playerObject *player,
    int move,
    int dir,
    int nDelta);
void ai_WalktoPlayer(
    playerObject *player,
    int move,
    int dist);
aiData *ai_GetAIHandle(int modelID, int level);
extern _svector mShotOffset[9];
extern _svector mShotMiss[9];
int jpb_AiRegisterData(
    int modelID, int level, aiData *data);
uint8_t ai_GetAiDataValue(
    aiData *data, const int8_t value[2]);
uint8_t ai_GetAiDataValueN(
    aiData *data, const int8_t value[2], int index);
uint8_t ai_GetAiSeqValue(
    aiData *data, int sequence, int index);
enum JPBAiLoadResult jpb_AiLoadDataFile(
    const char *path,
    void *storage,
    size_t storage_capacity,
    aiData **data,
    size_t *data_size);
/*
 * Exact PDB procedure ai_Main is a no-op with a void source signature.
 * funcArray is an int-returning callback table, so this descriptive adapter
 * supplies the deterministic non-completing result expected by the portable
 * controller while retaining ai_Main as its own exact symbol.
 */
void ai_Main(int32_t *cpad, playerObject *player);
int jpb_ai_MainCallback(
    int32_t *cpad, playerObject *player);

#if defined(__cplusplus)
#define JPB_AI_STATIC_ASSERT static_assert
#else
#define JPB_AI_STATIC_ASSERT _Static_assert
#endif

JPB_AI_STATIC_ASSERT(
    sizeof(aiData) == 20,
    "aiData must match PDB type 0x117B");

#undef JPB_AI_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
