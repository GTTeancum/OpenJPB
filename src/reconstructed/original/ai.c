#include "jpb/ai.h"
#include "jpb/animctrl.h"
#include "jpb/brain.h"
#include "jpb/brainutl.h"
#include "jpb/bullet.h"
#include "jpb/collision.h"
#include "jpb/combo.h"
#include "jpb/enemy.h"
#include "jpb/game.h"
#include "jpb/force.h"
#include "jpb/filesys.h"
#include "jpb/globalarrays.h"
#include "jpb/loader.h"
#include "jpb/memory.h"
#include "jpb/model.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/resources.h"
#include "jpb/scene.h"
#include "jpb/shaolin.h"
#include "jpb/vectors.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * REVIEWED RECONSTRUCTION.
 * PDB module: 0001
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\ai.obj
 * Primary source: W:\SWJediPowerBattles\Work\ai.c
 * Compiler language: c
 * Emitted procedures: 23
 *
 * Exact AI-data access and registration, player target selection, HTH,
 * ranged and sequence attack owners, point/player walking, ai_Death, and
 * the no-op ai_Main procedures are integrated. The dependency-free
 * jpb_AiLoadDataFile boundary validates caller-owned WAI storage used by the
 * portable runtime. Unrepresented procedures remain explicit in this
 * module-shaped source rather than being replaced with invented behavior.
 *
 * All gameplay procedures emitted by this module have been audited against
 * the matched PDB/export and are represented below. UCRT helper records are
 * intentionally supplied by the host toolchain.
 */

/*
 * Exact file-local PDB array maAiData at matched-PC RVA 0x4DC3F0. The
 * 0x280-byte extent to the next linked global proves 20 models by 4 levels.
 */
static aiData *maAiData[
    JPB_AI_MODEL_CAPACITY][JPB_AI_LEVEL_CAPACITY];

/* Exact initialized globals at matched-PC RVAs 0x4AFC10 and 0x4AFC60. */
_svector mShotOffset[9] = {
    {0, 0, 0, 0},
    {-4, 3, 4, 0},
    {0, 3, 0, 0},
    {8, 6, -8, 0},
    {-4, 6, -4, 0},
    {-4, 6, -4, 0},
    {0, 0, 0, 0},
    {4, -8, 4, 0},
    {0, 0, 0, 0}
};

_svector mShotMiss[9] = {
    {0x100, 0x100, 0, 0},
    {0, 0x100, 0, 0},
    {0, 0x100, 0x100, 0},
    {0x100, 0x100, 0, 0},
    {0, 0x100, 0, 0},
    {0, 0x100, 0x100, 0},
    {0x100, 0x100, 0, 0},
    {0x100, 0x100, 0, 0},
    {0x100, 0x100, 0x100, 0}
};

/* 0x159F0, 8 bytes, global, 0 named locals
 * __local_stdio_printf_options
 * PDB type: unsigned __int64* ()
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt\corecrt_stdio_config.h
 */

/* 0x15A00, 204 bytes, global, 6 named locals
 * ai_CheckBounds
 * PDB type: int (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_CheckBounds(playerObject *player)
{
    VECTOR *position =
        physics_gGetPosition(&player->playerRoot);
    wsl_ENEMY *enemy = player->pEnemy;
    wsl_BAP_PLACEMENT *placement = enemy->pPlace;
    uint32_t range = UINT32_C(0x8000);

    if (placement->nWaypnt == 1) {
        VECTOR waypoint = {
            placement->wayPoints[0].loc.vx,
            placement->wayPoints[0].loc.vy,
            placement->wayPoints[0].loc.vz,
            placement->wayPoints[0].flags
        };

        range = vec_Distance2DLV(position, &waypoint);
    } else {
        int index = 1;

        if (placement->nWaypnt > 1) {
            do {
                wsl_BAP_WAYPOINT *selected =
                    &placement->wayPoints[
                        enemy->lastWayPoint];
                VECTOR waypoint = {
                    selected->loc.vx,
                    selected->loc.vy,
                    selected->loc.vz,
                    selected->flags
                };
                uint32_t distance =
                    vec_Distance2DLV(position, &waypoint);

                if (distance <= range) {
                    range = vec_Distance2DLV(
                        position, &waypoint);
                }
                ++index;
            } while (index < placement->nWaypnt);
        }
    }
    return (int32_t)range > 0x400;
}

/* 0x15AD0, 97 bytes, global, 5 named locals
 * ai_Death
 * PDB type: int (playerObject*, int)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_Death(playerObject *player, int DEATH)
{
    uint8_t *death =
        (uint8_t *)(void *)player->paiMemory->death;
    uint8_t move = death[0];
    uint8_t count = death[1];
    uint8_t *moves =
        (uint8_t *)(void *)player->paiMemory + move;

    (void)DEATH;
    if (count == 1) {
        return move;
    }
    if (count != 0) {
        return moves[rand() % (int)count];
    }
    return moves[0];
}

/* 0x15B40, 417 bytes, global, 4 named locals
 * ai_DefendCheck
 * PDB type: int (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_DefendCheck(playerObject *player)
{
    wsl_ENEMY *enemy = player->pEnemy;
    int range;
    uint32_t flags;
    uint8_t chance;
    int divisor;
    int roll;

    (void)physics_gGetPosition(&player->playerRoot);
    range = ai_FindNearestPlayer(
        player, &player->target);
    flags = player->pFlags;
    if (range > 307 ||
        (flags & UINT32_C(0x8000)) != 0 ||
        (flags & UINT32_C(0x2000)) != 0 ||
        enemy->ownerType != 2) {
        player->pFlags =
            flags & ~UINT32_C(0x20);
        return 0;
    }

    chance = ai_GetAiDataValue(
        player->paiMemory,
        player->paiMemory->block);
    if (chance != 0 &&
        (*player->pMotion)->Damage == 0 &&
        player->target != NULL) {
        Motion *target_motion =
            *player->target->pMotion;

        player->pFlags &= ~UINT32_C(0x20);
        if (target_motion->Damage != 0) {
            unsigned threshold =
                (unsigned)chance + 25U;
            Combo *combo =
                &player->target->paCombos[
                    target_motion->combo];

            if ((int)LevelSelect <
                combo->userData) {
                threshold = chance;
            }
            divisor =
                ((15 -
                  (int)(int8_t)GameStruct.BlockRate) *
                 0xc800) /
                0x1000;
            roll =
                divisor == 0
                    ? 0
                    : rand() % divisor;
            if (roll < (int)threshold) {
                player->pFlags |= UINT32_C(0x20);
                enemy->counter[2] = 3;
                return 1;
            }
        }
    }

    player->pFlags &= ~UINT32_C(0x20);
    return 0;
}

/* 0x15CF0, 270 bytes, global, 5 named locals
 * ai_FindFarPlayer
 * PDB type: int (playerObject*, playerObject...
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_FindFarPlayer(
    playerObject *player,
    playerObject **target,
    int minimum_range_difference)
{
    int range0 = 0;
    int range1 = 0;

    if (gpWorld->player0->playerRoot.objectID != -1 &&
        !obj_gCheckObjectFlag(
            &gpWorld->player0->playerRoot, 0, 0x20) &&
        (gpWorld->player0->pFlags & 0x00040200U) == 0) {
        range0 = physics_gGetRange(
            &player->playerRoot,
            &gpWorld->player0->playerRoot);
    }
    if (gpWorld->player1->playerRoot.objectID != -1 &&
        !obj_gCheckObjectFlag(
            &gpWorld->player1->playerRoot, 0, 0x20) &&
        (gpWorld->player1->pFlags & 0x00040200U) == 0) {
        range1 = physics_gGetRange(
            &player->playerRoot,
            &gpWorld->player1->playerRoot);
    }

    if (range0 != 0 || range1 != 0) {
        int difference = range0 - range1;

        if (difference < 0) {
            difference = -difference;
        }
        if (difference >= minimum_range_difference) {
            if (range0 < range1) {
                *target = gpWorld->player1;
            } else {
                *target = gpWorld->player0;
            }
            return range0 < range1 ? range1 : range0;
        }
    }
    return 0;
}

/* 0x15E00, 245 bytes, global, 4 named locals
 * ai_FindNearestPlayer
 * PDB type: int (playerObject*, playerObject...
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_FindNearestPlayer(
    playerObject *player, playerObject **target)
{
    int range0 = physics_gGetRange(
        &player->playerRoot,
        &gpWorld->player0->playerRoot);
    int range1;

    if (obj_gCheckObjectFlag(
            &gpWorld->player1->playerRoot, 0, 0x20)) {
        *target = gpWorld->player0;
        return range0;
    }

    range1 = physics_gGetRange(
        &player->playerRoot,
        &gpWorld->player1->playerRoot);
    if (!obj_gCheckObjectFlag(
            &gpWorld->player0->playerRoot, 0, 0x20)) {
        if (range1 < range0) {
            *target = gpWorld->player1;
        } else {
            *target = gpWorld->player0;
            range1 = range0;
        }
    } else {
        *target = gpWorld->player1;
    }
    return range1;
}

/* 0x15F00, 870 bytes, global, 14 named locals
 * ai_FireWeapon
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_FireWeapon(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    modelObject *model =
        (modelObject *)scene->pModel;
    uint32_t events = model->eventMask;
    Motion *motion = *player->pMotion;
    int ptype;
    int result;

    if (player->playerID == 0x15 ||
        player->playerID == 0x16) {
        (void)force_FlameCallBack(cpad, player);
        return 0;
    }
    if ((model->flags & UINT32_C(4)) == 0) {
        result = 0;
        if (events == 0) {
            return 0;
        }
    } else {
        events = UINT32_C(1);
        result = 1;
    }

    ptype = (int)(int8_t)motion->fx2;
    for (;;) {
        int x = brainutl_FindLSB_LV(events);
        Projectile *proj;

        if (x == 0) {
            break;
        }
        --x;
        events &= ~(UINT32_C(1) << ((unsigned)x & 31U));
        proj = bullet_AllocProjectile(ptype);
        if (proj != NULL) {
            VECTOR *pos0 = coll_GetNodeCenter(
                player->playernum, x);
            _svector *vel = coll_GetNodeVelocity(
                player->playernum, x);
            VECTOR *pos1 = coll_GetNodeCenter(
                player->target->playernum, 8);
            VECTOR tpos1;
            Mnode *torso = coll_GetNode(
                player->target->playerRoot.objectID, 7);
            _svector *shot_table;

            if (player->target->playerID == 0x11 &&
                torso != NULL &&
                (torso->flags &
                 UINT32_C(0x04000000)) != 0) {
                pos1 = coll_GetNodeCenter(
                    player->target->playernum, 0);
            }
            if ((model->flags & UINT32_C(4)) == 0 ||
                (player->pFlags & UINT32_C(0x2000)) != 0) {
                shot_table = mShotOffset;
            } else {
                shot_table = mShotMiss;
            }
            tpos1.vx = pos1->vx +
                shot_table[rand() % 9].vx;
            tpos1.vy = pos1->vy +
                shot_table[rand() % 9].vy;
            tpos1.vz = pos1->vz +
                shot_table[rand() % 9].vz;
            tpos1.pad = 0;
            if (ptype == 0x13) {
                uint32_t shade =
                    (uint32_t)(rand() % 0x40) +
                    UINT32_C(0x40);
                uint32_t edge = shade / 8U;

                proj->pj_Flags |=
                    (int32_t)UINT32_C(0x2000);
                proj->launchID = (char)x;
                proj->color = (int32_t)(
                    ((edge << 8) | shade) << 8 |
                    edge);
            }
            bullet_ShootProjectile(
                proj, player, pos0, &tpos1, vel);
        }
    }
    return result;
}

/* 0x16270, 22 bytes, global, 2 named locals
 * ai_GetAIHandle
 * PDB type: aiData* (int, int)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */

/* 0x16290, 76 bytes, global, 4 named locals
 * ai_GetAiDataValue
 * PDB type: unsigned char (aiData*, unsigned...
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */

/* 0x162E0, 32 bytes, global, 4 named locals
 * ai_GetAiDataValueN
 * PDB type: unsigned char (aiData*, unsigned...
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */

/* 0x16300, 33 bytes, global, 3 named locals
 * ai_GetAiSeqValue
 * PDB type: unsigned char (aiData*, int, int...
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */

/* 0x16330, 386 bytes, global, 8 named locals
 * ai_HthAttack
 * PDB type: int (wsl_ENEMY*, UDATA*)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_HthAttack(
    wsl_ENEMY *pEnemy, UDATA *vars)
{
    playerObject *player = pEnemy->pPlayer;
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    modelObject *model =
        (modelObject *)scene->pModel;
    int move = vars[1].si;

    ai_SetTarget(player, vars[0].si);
    if (move == 0) {
        kfNode *kungfu = pEnemy->kungfu;

        if (kungfu == NULL) {
            kungfu = shaolin_GetKungfu(
                player->playerRoot.objectID);
            pEnemy->kungfu = kungfu;
            if (kungfu == NULL) {
                return 0;
            }
        }
        if (kungfu->timer < gGlobalTimer ||
            (player->target->pFlags &
             UINT32_C(0x100)) != 0) {
            int aoa = physics_gGetFaceTargetDelta(
                &player->target->playerRoot,
                &player->playerRoot);
            int dist = physics_gGetRange(
                &player->target->playerRoot,
                &player->playerRoot);

            kungfu->timer =
                gGlobalTimer + UINT32_C(0x7800);
            if (aoa < 0) {
                aoa = -aoa;
            }
            kungfu->chi =
                (int16_t)(
                    dist / 16 +
                    (aoa > 0x400 ? 4 : 0));
            kungfu->id =
                &player->playerRoot;
        }
        shaolin_AddKungfu(kungfu);
        return 1;
    }

    move += player->subOffset;
    if (move <= player->maxMotions) {
        pEnemy->kungfu = NULL;
        (void)physics_gForceFaceTarget(
            &player->playerRoot,
            &player->target->playerRoot);
        if ((model->flags & UINT32_C(4)) == 0 ||
            (player->pFlags &
             UINT32_C(0x2000)) != 0) {
            if ((player->paMotions[move].motionFlags &
                 UINT32_C(0x100000)) != 0) {
                player->paMotions[move].FunctPtr = 1;
            }
            if (animctrl_MotionLockLevel(
                    &player->playerRoot,
                    &player->paMotions[move],
                    0x19) != 0) {
                player->pFlags &=
                    ~UINT32_C(0x20);
                return 1;
            }
        }
    }
    return 0;
}

/* 0x164C0, 697 bytes, global, 5 named locals
 * ai_LoadAI
 * PDB type: int (int, char*)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */

int ai_LoadAI(int modelID, char *name)
{
    char filename[JPB_RESOURCE_PATH_CAPACITY];
    int level;

    for (level = 1; level <= JPB_AI_LEVEL_CAPACITY; ++level) {
        const char *path;
        uint64_t file_size;
        unsigned allocation_size;

        memset(filename, 0, sizeof(filename));
        sprintf(filename, "%s.0%d", name, level);
        path = resource_getPath(filename, JPB_RESOURCE_AI);
        file_size = file_getFileSize((char *)(void *)path);
        if (file_size == 0) {
            path = resource_getPath("dummy.wai", JPB_RESOURCE_AI);
            file_size = file_getFileSize((char *)(void *)path);
            if (file_size == 0) {
                exit(1);
            }
        }
        allocation_size =
            ((unsigned)file_size + 3U) & ~UINT32_C(3);
        maAiData[modelID][level - 1] =
            (aiData *)memory_gCallocAnyMemory(1, allocation_size);
        (void)file_LoadFile(
            (char *)(void *)path,
            maAiData[modelID][level - 1]);
    }
    return 0;
}
aiData *ai_GetAIHandle(int modelID, int level)
{
    return maAiData[modelID][level];
}

int jpb_AiRegisterData(
    int modelID, int level, aiData *data)
{
    if (modelID < 0 ||
        modelID >= JPB_AI_MODEL_CAPACITY ||
        level < 0 ||
        level >= JPB_AI_LEVEL_CAPACITY) {
        return 0;
    }
    maAiData[modelID][level] = data;
    return 1;
}

/* 0x16780, 3 bytes, global, 2 named locals
 * ai_Main
 * PDB type: void (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
void ai_Main(
    int32_t *cpad, playerObject *player)
{
    (void)cpad;
    (void)player;
}

int jpb_ai_MainCallback(
    int32_t *cpad, playerObject *player)
{
    ai_Main(cpad, player);
    return 0;
}

/* 0x16790, 386 bytes, global, 6 named locals
 * ai_RangedAttack
 * PDB type: void (wsl_ENEMY*, UDATA*)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
void ai_RangedAttack(
    wsl_ENEMY *pEnemy, UDATA *vars)
{
    playerObject *player = pEnemy->pPlayer;
    aiData *pAiData = player->paiMemory;
    int move = vars[3].si;

    ai_SetTarget(player, vars[2].si);
    if ((player->target->pFlags &
         UINT32_C(0x80)) != 0) {
        (void)animctrl_MotionLockLevel(
            &player->playerRoot,
            &player->paMotions[0],
            0x19);
        return;
    }

    {
        int aoa = physics_gGetFaceTargetDelta(
            &player->playerRoot,
            &player->target->playerRoot);

        if (player->target->currentMotion <= 1 ||
            (player->target->currentMotion >= 0x13 &&
             player->target->currentMotion <= 0x15)) {
            pEnemy->currRangedDelay -= 2;
        }
        pEnemy->currRangedDelay -= timeAdj;
        if (aoa < 0) {
            aoa = -aoa;
        }

        if (aoa < 0x100 &&
            pEnemy->currRangedDelay < 0) {
            if (move == 0) {
                move = ai_GetAiDataValue(
                    pAiData, pAiData->ranged);
                if (move == 0) {
                    pEnemy->currRangedDelay = 0;
                    return;
                }
            }
        } else if (move == 0) {
            physics_gTurnToFace(
                &player->playerRoot,
                physics_gFaceTarget(
                    &player->playerRoot,
                    &player->target->playerRoot),
                4);
            (void)animctrl_MotionLockLevel(
                &player->playerRoot,
                &player->paMotions[0],
                0x19);
            return;
        }
    }

    {
        uint8_t delay = ai_GetAiDataValue(
            pAiData, pAiData->reload);
        int32_t scaled_delay;

        pEnemy->currRangedDelay =
            (int32_t)delay * 2;
        scaled_delay =
            (int32_t)(int8_t)GameStruct.RangedRate *
            0x200 *
            pEnemy->currRangedDelay;
        pEnemy->currRangedDelay =
            scaled_delay / 0x1000;
    }
    if (move > 7) {
        move += player->subOffset;
    }
    if ((player->paMotions[move].motionFlags &
         UINT32_C(0x100000)) != 0) {
        player->paMotions[move].FunctPtr = 1;
    }
    if (animctrl_MotionLock(
            &player->playerRoot,
            &player->paMotions[move]) != 0) {
        player->pFlags &= ~UINT32_C(0x20);
    }
}

uint8_t ai_GetAiDataValue(
    aiData *data, const int8_t value[2])
{
    uint8_t offset = (uint8_t)value[0];
    uint8_t count = (uint8_t)value[1];
    const uint8_t *bytes =
        (const uint8_t *)(const void *)data;

    if (count == 1) {
        return offset;
    }
    if (count == 0) {
        return bytes[offset];
    }
    return bytes[offset + rand() % (int)count];
}

/* 0x16920, 581 bytes, global, 6 named locals
 * ai_SeqAttack
 * PDB type: int (wsl_ENEMY*, UDATA*)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_SeqAttack(
    wsl_ENEMY *pEnemy, UDATA *vars)
{
    playerObject *player = pEnemy->pPlayer;
    aiData *pAiData = player->paiMemory;
    const uint8_t *bytes =
        (const uint8_t *)(const void *)pAiData;
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    modelObject *model =
        (modelObject *)scene->pModel;
    int number = vars[2].si;
    uint8_t move;

    if ((model->flags & UINT32_C(4)) != 0 &&
        (player->pFlags &
         UINT32_C(0x2000)) == 0) {
        return 0;
    }

    ai_SetTarget(player, vars[0].si);
    pEnemy->currSeqIndex = 0;
    if ((uint8_t)pAiData->combos[1] == 1 ||
        (int)(uint8_t)pAiData->combos[1] <
            number) {
        pEnemy->currSeq =
            (uint8_t)pAiData->combos[0];
    } else {
        pEnemy->currSeq =
            bytes[
                (uint8_t)pAiData->combos[0] +
                number];
    }
    pEnemy->seqMode = 1;

    if (bytes[pEnemy->currSeq + 1] == 0) {
        return 0;
    }
    move = bytes[
        bytes[pEnemy->currSeq]];
    if (move == 0) {
        return 0;
    }

    physics_gTurnToFace(
        &player->playerRoot,
        physics_gFaceTarget(
            &player->playerRoot,
            &player->target->playerRoot),
        4);
    if ((player->paMotions[move].motionFlags &
         UINT32_C(0x100000)) != 0) {
        player->paMotions[move].FunctPtr = 1;
    }
    if (animctrl_MotionLock(
            &player->playerRoot,
            &player->paMotions[move]) == 0) {
        return 0;
    }

    player->pFlags &= ~UINT32_C(0x20);
    ++pEnemy->currSeqIndex;
    while (pEnemy->seqMode > 0) {
        int selected_move;

        move = ai_GetAiSeqValue(
            pAiData,
            pEnemy->currSeq,
            pEnemy->currSeqIndex);
        if (move == 0) {
            break;
        }
        selected_move = move;
        if (move > 7) {
            selected_move += player->subOffset;
        }
        physics_gTurnToFace(
            &player->playerRoot,
            physics_gFaceTarget(
                &player->playerRoot,
                &player->target->playerRoot),
            4);
        if ((player->paMotions[selected_move].motionFlags &
             UINT32_C(0x100000)) != 0) {
            player->paMotions[selected_move].FunctPtr = 1;
        }
        if (selected_move == 4) {
            brain_SetJumpTrajectory(player, 0);
            player->paMotions[4].FunctPtr = 6;
            brain_SetTrajectory(
                player,
                player->airVelocity,
                player->airAngle);
            physics_gSnapShotPosition(
                &player->playerRoot, 0x3c);
        }
        if (animctrl_MotionChain(
                &player->playerRoot,
                &player->paMotions[selected_move]) == 0) {
            return 0;
        }
        player->pFlags &= ~UINT32_C(0x20);
        ++pEnemy->currSeqIndex;
        if (pEnemy->seqMode < 1) {
            return 1;
        }
    }

    pEnemy->currSeq = 0;
    pEnemy->currSeqIndex = 0;
    pEnemy->seqMode = 0;
    return 1;
}

uint8_t ai_GetAiDataValueN(
    aiData *data,
    const int8_t value[2],
    int index)
{
    uint8_t count = (uint8_t)value[1];

    if (count != 1 && index <= (int)count) {
        return ((const uint8_t *)(const void *)data)[
            (uint8_t)value[0] + index];
    }
    return (uint8_t)value[0];
}

/* 0x16B70, 252 bytes, global, 5 named locals
 * ai_SetTarget
 * PDB type: void (playerObject*, int)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
uint8_t ai_GetAiSeqValue(
    aiData *data, int sequence, int index)
{
    const uint8_t *bytes =
        (const uint8_t *)(const void *)data;
    uint8_t count = bytes[sequence + 1];

    if (index >= (int)count) {
        return 0;
    }
    return bytes[bytes[sequence] + index];
}

void ai_SetTarget(
    playerObject *player, int target)
{
    if (target == 2 || target == 3) {
        objectRoot *root =
            brainutl_gGetNearestTarget(
                &player->playerRoot, target);

        if (root != NULL) {
            sceneObject *scene =
                (sceneObject *)root->pParent;

            player->target =
                (playerObject *)scene->pPlayer;
        }
        return;
    }
    if (target >= 4 && target <= 10) {
        wsl_ENEMY *pEnemy = player->pEnemy;
        uint16_t enemy_id =
            *(const uint16_t *)(const void *)(
                (const uint8_t *)(const void *)
                    pEnemy->pPlace +
                0x64 + target * 2);

        if ((int)enemy_id < gpWorld->nEnemy) {
            wsl_ENEMY *target_enemy =
                (wsl_ENEMY *)getPtr(
                    enemy_getPointerIndex(
                        (int)enemy_id),
                    JPB_POINTER_ARRAY_ENEMY);

            if (target_enemy != NULL &&
                target_enemy->pPlayer != NULL) {
                player->target =
                    target_enemy->pPlayer;
            }
        }
        return;
    }
    (void)ai_FindNearestPlayer(
        player, &player->target);
}

static int jpb_ai_validate_data(
    const uint8_t *bytes, size_t size)
{
    size_t pair;

    if (size < sizeof(aiData)) {
        return 0;
    }
    for (pair = 0; pair < 8; ++pair) {
        uint8_t offset = bytes[pair * 2];
        uint8_t count = bytes[pair * 2 + 1];
        size_t required =
            count == 0 ? 1u : (size_t)count;

        if (count != 1 &&
            ((size_t)offset >= size ||
             required > size - (size_t)offset)) {
            return 0;
        }
    }
    return 1;
}

enum JPBAiLoadResult jpb_AiLoadDataFile(
    const char *path,
    void *storage,
    size_t storage_capacity,
    aiData **data,
    size_t *data_size)
{
    FILE *file;
    long file_size;

    if (path == NULL || storage == NULL ||
        data == NULL || data_size == NULL) {
        return JPB_AI_INVALID_ARGUMENT;
    }
    *data = NULL;
    *data_size = 0;
    file = fopen(path, "rb");
    if (file == NULL) {
        return JPB_AI_IO_ERROR;
    }
    if (fseek(file, 0, SEEK_END) != 0 ||
        (file_size = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return JPB_AI_IO_ERROR;
    }
    if ((size_t)file_size > storage_capacity) {
        fclose(file);
        return JPB_AI_STORAGE_TOO_SMALL;
    }
    if (file_size != 0 &&
        fread(storage, (size_t)file_size, 1, file) != 1) {
        fclose(file);
        return JPB_AI_IO_ERROR;
    }
    if (fclose(file) != 0) {
        return JPB_AI_IO_ERROR;
    }
    if (!jpb_ai_validate_data(
            (const uint8_t *)storage,
            (size_t)file_size)) {
        return JPB_AI_INVALID_DATA;
    }
    *data = (aiData *)storage;
    *data_size = (size_t)file_size;
    return JPB_AI_OK;
}

/* 0x16C70, 382 bytes, global, 4 named locals
 * ai_Throw
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_Throw(
    int32_t *cpad, playerObject *player)
{
    playerObject *target = player->target;
    int ptype = (int)(int8_t)(*player->pMotion)->fx2;

    (void)cpad;
    if (target != NULL &&
        (player->pFlags &
         UINT32_C(0x44000601)) == 0 &&
        physics_gGetRange(
            &player->playerRoot,
            &target->playerRoot) <= 128) {
        int aoa = physics_gGetFaceTargetDelta(
            &player->playerRoot,
            &target->playerRoot);

        if (aoa < 0) {
            aoa = -aoa;
        }
        if (aoa < 0x200) {
            Motion *throw_motion =
                &player->paMotions[ptype];

            player->pFlags &= UINT32_C(0xffbfffff);
            player->locked = NULL;
            target->pFlags &= UINT32_C(0xffbfffff);
            target->locked = NULL;
            if (animctrl_MotionLock(
                    &player->playerRoot,
                    throw_motion) != 0) {
                Motion *target_motion =
                    &player->paMotions[ptype + 1];

                target->target = player;
                (void)physics_gForceFaceTarget(
                    &player->playerRoot,
                    &target->playerRoot);
                (void)physics_gForceFaceTarget(
                    &target->playerRoot,
                    &player->playerRoot);
                target->pFlags |= UINT32_C(0x801);
                target_motion->motionFlags |=
                    UINT32_C(0x40000020);
                target_motion->FunctPtr = 5;
                target_motion->Lock = 0x1e;
                (void)animctrl_MotionNoLock(
                    &target->playerRoot,
                    target_motion);
                return 1;
            }
        }
    }
    return 0;
}

/* 0x16DF0, 48 bytes, global, 1 named locals
 * ai_ValidateData
 * PDB type: char (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
char ai_ValidateData(playerObject *player)
{
    int16_t player_id = player->playerID;

    (void)loader_GetModelName(player_id);
    if (player_id == 0x48 || player_id > 0x4f) {
        player_id = (int16_t)(player_id & (int16_t)0xff00);
    }
    return (char)player_id;
}

/* 0x16E20, 436 bytes, global, 10 named locals
 * ai_WalkToPoint
 * PDB type: int (playerObject*, int, VECTOR*...
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_WalkToPoint(
    playerObject *player,
    int move,
    VECTOR *waypoint,
    int nDelta)
{
    VECTOR *pos =
        physics_gGetPosition(&player->playerRoot);
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    wsl_ENEMY *pEnemy = player->pEnemy;
    int dist;

    player->pFlags &= ~UINT32_C(0x20);
    if ((physics->flags & UINT32_C(0x2000)) == 0) {
        dist = (int)vec_Distance2DLV(pos, waypoint);
    } else {
        dist = (int)vec_DistanceLV(pos, waypoint);
    }

    if (dist > nDelta) {
        VECTOR normal = {
            waypoint->vx - pos->vx,
            waypoint->vy - pos->vy,
            waypoint->vz - pos->vz,
            0
        };
        VECTOR dir;
        _svector rot = {0, 0, 0, 0};
        uint32_t flags = player->pFlags;

        pEnemy->destination.vx =
            (int16_t)waypoint->vx;
        pEnemy->destination.vy =
            (int16_t)waypoint->vy;
        pEnemy->destination.vz =
            (int16_t)waypoint->vz;
        if ((int32_t)flags >= 0) {
            flags |= UINT32_C(0x80000000);
        }
        player->pFlags =
            flags & ~UINT32_C(0x08000000);

        /*
         * The reference computes dir but then derives rot from normal.
         * Preserve that apparently redundant call and operand choice.
         */
        vec_VectorNormalLV(&normal, &dir);
        vec_RotFromNormal(&rot, &normal);
        pEnemy->destination.vx =
            (int16_t)waypoint->vx;
        pEnemy->destination.vy =
            (int16_t)waypoint->vy;
        pEnemy->destination.vz =
            (int16_t)waypoint->vz;
        pEnemy->radius = (uint32_t)nDelta;

        if (player->currentMotion == move) {
            physics_gSetFacing(
                &player->playerRoot, rot.vy);
        }
        (void)animctrl_MotionEqualLock(
            &player->playerRoot,
            &player->paMotions[move]);
        return 0;
    }

    {
        int motion =
            player->currentMotion == 2 &&
            (player->pFlags & UINT32_C(0x8000)) == 0
                ? 25
                : 0;

        if (animctrl_MotionNoLock(
                &player->playerRoot,
                &player->paMotions[motion]) != 0) {
            player->pFlags &=
                ~UINT32_C(0x400000);
            player->hitMask = 0;
        }
    }
    return 1;
}

/* 0x16FE0, 835 bytes, global, 12 named locals
 * ai_WalkWayPoints
 * PDB type: int (playerObject*, int, int, in...
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
int ai_WalkWayPoints(
    playerObject *player,
    int move,
    int dir,
    int nDelta)
{
    VECTOR *pos =
        physics_gGetPosition(&player->playerRoot);
    wsl_ENEMY *pEnemy = player->pEnemy;
    wsl_BAP_PLACEMENT *pPlace =
        pEnemy->pPlace;

    if ((player->pFlags & UINT32_C(1)) != 0) {
        return 0;
    }

    player->pFlags &= ~UINT32_C(0x20);
    if (LevelSelect == 15 &&
        pEnemy->enemyID == 0x5f) {
        pPlace->wayPoints[
            pEnemy->lastWayPoint].loc.vz = 6000;
    }

    {
        wsl_BAP_WAYPOINT *selected =
            &pPlace->wayPoints[
                pEnemy->lastWayPoint];
        VECTOR waypoint = {
            selected->loc.vx,
            selected->loc.vy,
            selected->loc.vz,
            selected->flags
        };
        sceneObject *scene =
            (sceneObject *)
                player->playerRoot.pParent;
        physicsObject *physics =
            (physicsObject *)scene->pPhysics;
        int dist =
            (physics->flags &
             UINT32_C(0x2000)) == 0
                ? (int)vec_Distance2DLV(
                      pos, &waypoint)
                : (int)vec_DistanceLV(
                      pos, &waypoint);
        int motion =
            (player->pFlags &
             UINT32_C(0x400000)) != 0
                ? 26
                : move;

        if (dist <= nDelta) {
            int next =
                pEnemy->lastWayPoint + dir;

            pEnemy->lastWayPoint = next;
            if (pPlace->nWaypnt == 1) {
                int idle =
                    player->currentMotion == 2 &&
                    (player->pFlags &
                     UINT32_C(0x8000)) == 0
                        ? 25
                        : 0;

                pEnemy->lastWayPoint = 0;
                if (animctrl_MotionNoLock(
                        &player->playerRoot,
                        &player->paMotions[idle]) != 0) {
                    player->pFlags &=
                        ~UINT32_C(0x400000);
                    player->hitMask = 0;
                }
            } else if (next >= pPlace->nWaypnt) {
                pEnemy->lastWayPoint = 1;
            } else if (next < 1) {
                pEnemy->lastWayPoint =
                    pPlace->nWaypnt - 1;
            }
            return 1;
        }

        {
            VECTOR normal = {
                waypoint.vx - pos->vx,
                waypoint.vy - pos->vy,
                waypoint.vz - pos->vz,
                0
            };
            VECTOR normalized;
            _svector rot = {0, 0, 0, 0};
            uint32_t flags = player->pFlags;

            vec_VectorNormalLV(
                &normal, &normalized);
            vec_RotFromNormal(&rot, &normal);
            pEnemy->destination.vx =
                (int16_t)waypoint.vx;
            pEnemy->destination.vy =
                (int16_t)waypoint.vy;
            pEnemy->destination.vz =
                (int16_t)waypoint.vz;
            pEnemy->radius = (uint32_t)nDelta;

            if ((int32_t)flags >= 0) {
                flags |= UINT32_C(0x80000000);
            }
            player->pFlags =
                flags & ~UINT32_C(0x08000000);
            if (player->currentMotion == motion) {
                physics_gTurnToFace(
                    &player->playerRoot,
                    rot.vy,
                    4);
            } else {
                (void)animctrl_MotionLockLevel(
                    &player->playerRoot,
                    &player->paMotions[motion],
                    0x16);
            }
            if ((player->pFlags &
                 UINT32_C(0x400000)) != 0) {
                physics_gSetFacing(
                    &player->playerRoot, rot.vy);
            }
            if (motion == 4) {
                brain_SetJumpTrajectory(player, 0);
                player->paMotions[4].FunctPtr = 6;
                brain_SetTrajectory(
                    player,
                    player->airVelocity,
                    player->airAngle);
                physics_gSnapShotPosition(
                    &player->playerRoot, 0x3c);
            }

            if (LevelSelect == 5 &&
                pEnemy->enemyID == 0x21 &&
                player->playerID == 0x26 &&
                player->paMotions[motion].vel ==
                    0x5d &&
                pEnemy->location.vx > -0x9ac &&
                (uint32_t)(
                    pEnemy->location.vy - 0xe74) <
                    UINT32_C(0x12d) &&
                pEnemy->destination.vx ==
                    (int16_t)-0x480) {
                physics_gSnapShotPosition(
                    &player->playerRoot, 1);
                player->pFlags &=
                    ~UINT32_C(0x400000);
            }
        }
    }
    return 0;
}

/* 0x17330, 212 bytes, global, 5 named locals
 * ai_WalktoPlayer
 * PDB type: void (playerObject*, int, int)
 * Source: W:\SWJediPowerBattles\Work\ai.c
 */
void ai_WalktoPlayer(
    playerObject *player,
    int move,
    int dist)
{
    wsl_ENEMY *pEnemy = player->pEnemy;
    sceneObject *target_scene =
        (sceneObject *)player->target
            ->playerRoot.pParent;
    physicsObject *target_physics =
        (physicsObject *)target_scene->pPhysics;
    uint32_t flags = player->pFlags;

    /*
     * dist is part of exact PDB type 0x12C2, but the optimized procedure
     * never reads it.
     */
    (void)dist;
    pEnemy->destination.vx =
        (int16_t)(int)target_physics->pos.vx;
    pEnemy->destination.vy =
        (int16_t)(int)target_physics->pos.vy;
    pEnemy->destination.vz =
        (int16_t)(int)target_physics->pos.vz;
    pEnemy->radius = UINT32_C(0x100);
    if ((flags & UINT32_C(0x08000000)) == 0) {
        flags |= UINT32_C(0x08000000);
    }
    player->pFlags =
        flags & ~UINT32_C(0x80000000);

    if ((flags & UINT32_C(1)) == 0) {
        int facing = physics_gFaceTarget(
            &player->playerRoot,
            &player->target->playerRoot);
        int selected_move =
            (flags & UINT32_C(0x400000)) != 0
                ? 26
                : move;

        physics_gTurnToFace(
            &player->playerRoot, facing, 4);
        if (animctrl_MotionEqualLock(
                &player->playerRoot,
                &player->paMotions[selected_move]) != 0) {
            player->pFlags &=
                ~UINT32_C(0x20);
        }
    }
}

/* 0x17410, 93 bytes, global, 3 named locals
 * sprintf
 * PDB type: int (char* const, const char* co...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt\stdio.h
 */
