/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\shaolin.c.
 *
 * All seven emitted procedures and the complete 2,404-byte module are
 * represented below. Layouts and stores were checked against PDB types
 * 0x10CA/0x7922/0x7924 and matched x64 RVAs 0xF7220..0xF7BB3.
 *
 * PDB module: 0078
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\shaolin.obj
 * Primary source: W:\SWJediPowerBattles\Work\shaolin.c
 * Compiler language: c
 * Emitted procedures: 7
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/shaolin.h"
#include "jpb/ai.h"
#include "jpb/animctrl.h"
#include "jpb/brain.h"
#include "jpb/game.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/vectors.h"

#include <string.h>

uint8_t attackChoice[JPB_KUNGFU_CAPACITY];
kfNode akfNodes[JPB_KUNGFU_CAPACITY];
List kfList;
/*
 * The exact PDB name is simply `a`; the two shorts count attackers assigned
 * to each player during a scheduler pass.
 */
int16_t a[2];

/* Exact file-local PDB globals at matched-PC RVAs 0x547C40/0x547C80. */
static VECTOR point[4];
static int32_t first;

static playerObject *shaolin_node_player(kfNode *kungfu)
{
    sceneObject *scene;

    if (kungfu == NULL ||
        kungfu->id == NULL ||
        kungfu->id->pParent == NULL) {
        return NULL;
    }
    scene = (sceneObject *)kungfu->id->pParent;
    return (playerObject *)scene->pPlayer;
}

static physicsObject *shaolin_node_physics(
    kfNode *kungfu)
{
    sceneObject *scene;

    if (kungfu == NULL ||
        kungfu->id == NULL ||
        kungfu->id->pParent == NULL) {
        return NULL;
    }
    scene = (sceneObject *)kungfu->id->pParent;
    return (physicsObject *)scene->pPhysics;
}

static int32_t shaolin_smooth_component(
    int32_t current, int32_t target)
{
    int32_t delta = (int16_t)(
        (uint16_t)current - (uint16_t)target);
    int32_t step =
        delta >= 0
            ? delta / 16
            : -((-delta + 15) / 16);

    return current - step;
}

/* 0xF7220, 15 bytes, global, 1 named locals
 * shaolin_AddKungfu
 * PDB type: void (kfNode*)
 * Source: W:\SWJediPowerBattles\Work\shaolin.c
 */
void shaolin_AddKungfu(kfNode *kungfu)
{
    list_AddTail(&kfList, &kungfu->node);
}

/* 0xF7230, 1511 bytes, global, 12 named locals
 * shaolin_Attack
 * PDB type: void (kfNode*)
 * Source: W:\SWJediPowerBattles\Work\shaolin.c
 */
void shaolin_Attack(kfNode *n)
{
    playerObject *player =
        shaolin_node_player(n);
    physicsObject *phy =
        shaolin_node_physics(n);
    wsl_ENEMY *pEnemy;
    aiData *pAiData;
    playerObject *other;
    int16_t *attacker_count;
    int closingRange;
    int dist;
    int move;

    if (player == NULL ||
        phy == NULL ||
        player->pEnemy == NULL ||
        player->paiMemory == NULL ||
        player->target == NULL ||
        gpWorld == NULL) {
        return;
    }
    pEnemy = player->pEnemy;
    pAiData = player->paiMemory;
    if (player->target == gpWorld->player0) {
        ++a[0];
        attacker_count = &a[0];
        other = gpWorld->player1;
    } else if (player->target == gpWorld->player1) {
        ++a[1];
        attacker_count = &a[1];
        other = gpWorld->player0;
    } else {
        return;
    }

    dist = physics_gGetRange(
        &player->target->playerRoot,
        &player->playerRoot);
    move =
        (player->target->pFlags &
         UINT32_C(0x100)) != 0 ||
        (double)dist > 307.2
            ? 2
            : 1;
    if ((player->pFlags & UINT32_C(0x8000)) == 0 &&
        (player->pFlags & UINT32_C(0x400000)) != 0 &&
        move == 1) {
        move = 26;
    }
    closingRange =
        player->target->playerRoot.objectID == tankID
            ? 0x1cc
            : 0x100;

    if (*attacker_count <= 2 ||
        (player->pFlags & UINT32_C(0x8000)) != 0) {
        if ((double)dist >
            (double)closingRange * 0.75) {
            ai_WalktoPlayer(
                player,
                move,
                (int)((double)closingRange * 0.85));
            return;
        }

        if (pEnemy->seqMode > 0) {
            int attack = ai_GetAiSeqValue(
                pAiData,
                pEnemy->currSeq,
                pEnemy->currSeqIndex);

            /* Exact debug_printf("\tseq mode\n") is diagnostic only. */
            if (attack == 0) {
                pEnemy->currSeq = 0;
                pEnemy->currSeqIndex = 0;
                pEnemy->seqMode = 0;
                ++n->chi;
                return;
            }

            move = player->subOffset + attack;
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
            if (move == 4) {
                player->paMotions[4].FunctPtr = 6;
            }
            if (animctrl_MotionLock(
                    &player->playerRoot,
                    &player->paMotions[move]) == 0) {
                return;
            }
            if (player->playerID == 9) {
                ++pEnemy->counter[3];
            }
            player->pFlags &= ~UINT32_C(0x20);
            ++pEnemy->currSeqIndex;
            if (move == 4) {
                brain_SetJumpTrajectory(player, 1);
                brain_SetTrajectory(
                    player,
                    player->airVelocity,
                    player->airAngle);
                physics_gSnapShotPosition(
                    &player->playerRoot, 0);
            }
            return;
        }

        --pEnemy->currHTHDelay;
        if (player->target->currentMotion <= 1 ||
            (player->target->currentMotion >= 0x14 &&
             player->target->currentMotion <= 0x15) ||
            player->target->currentMotion == 0x13) {
            pEnemy->currHTHDelay -= 2;
        }

        {
            sceneObject *scene =
                (sceneObject *)
                    player->playerRoot.pParent;
            modelObject *model =
                (modelObject *)scene->pModel;

            if ((model->flags & UINT32_C(4)) != 0 &&
                (player->pFlags &
                 UINT32_C(0x2000)) == 0) {
                return;
            }
        }

        if (player->fStun > 0xc8 &&
            player->playerID != 9 &&
            player->playerID != 0x2b) {
            player->pFlags |= UINT32_C(0x20);
            return;
        }

        if (pEnemy->currHTHDelay >= 0) {
            int idle = 0;

            if ((player->pFlags &
                 UINT32_C(0x8000)) == 0) {
                if ((player->pFlags &
                     UINT32_C(0x400000)) != 0) {
                    idle = 0x14;
                } else if (
                    game_gGetEnergy(
                        player->playernum) <
                    game_gGetMaxEnergy(
                        player->playernum) / 4) {
                    idle = 0x13;
                }
            }
            (void)animctrl_MotionLockLevel(
                &player->playerRoot,
                &player->paMotions[idle],
                0x16);
            return;
        }

        {
            uint8_t delay = ai_GetAiDataValue(
                pAiData, pAiData->violence);
            uint8_t attack = ai_GetAiDataValue(
                pAiData, pAiData->hth);
            int32_t scaled_delay;

            pEnemy->currHTHDelay = (int32_t)delay * 2;
            scaled_delay =
                (int32_t)(int8_t)GameStruct.HTHRate *
                0x200 *
                pEnemy->currHTHDelay;
            pEnemy->currHTHDelay =
                scaled_delay / 0x1000;

            if (shaolin_CheckMove(
                    attack,
                    player->playerRoot.objectID) != 0) {
                attack = ai_GetAiDataValue(
                    pAiData, pAiData->hth);
            }

            if (attack == 0) {
                pEnemy->currSeqIndex = 0;
                pEnemy->currSeq =
                    ai_GetAiDataValue(
                        pAiData,
                        pAiData->combos);
                attack = ai_GetAiSeqValue(
                    pAiData,
                    pEnemy->currSeq,
                    pEnemy->currSeqIndex);
                ++pEnemy->currSeqIndex;
                pEnemy->seqMode = 1;
            }

            move = player->subOffset + attack;
            if (move > player->maxMotions) {
                return;
            }
            (void)physics_gForceFaceTarget(
                &player->playerRoot,
                &player->target->playerRoot);
            if ((player->paMotions[move].motionFlags &
                 UINT32_C(0x100000)) != 0) {
                player->paMotions[move].FunctPtr = 1;
            }
            if (animctrl_MotionLockLevel(
                    &player->playerRoot,
                    &player->paMotions[move],
                    0x19) != 0) {
                ++n->chi;
                player->pFlags &=
                    ~UINT32_C(0x20);
            }
        }
        return;
    }

    if (*attacker_count == 4) {
        if (dist > 0x200) {
            ai_WalktoPlayer(
                player, move, closingRange);
            return;
        }
        if (dist >= 0x100) {
            return;
        }
        (void)animctrl_MotionLockLevel(
            &player->playerRoot,
            &player->paMotions[0x15],
            0x16);
        (void)physics_gForceFaceTarget(
            &player->playerRoot,
            &player->target->playerRoot);
        player->pFlags |= UINT32_C(0x20);
        return;
    }

    if (*attacker_count == 3) {
        if ((double)dist >= 384.0) {
            ai_WalktoPlayer(
                player, move, closingRange);
            return;
        }
        player->pFlags |= UINT32_C(0x400000);
        player->locked = player->target;
        if (vec_DistanceLV(
                (VECTOR *)(void *)&phy->pos,
                &point[n->loc]) > UINT32_C(0x55) &&
            ai_WalkToPoint(
                player,
                0x1a,
                &point[n->loc],
                0x20) == 0) {
            return;
        }
    }

    if (other->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(
            &other->playerRoot, 0, 0x20) == 0 &&
        (other->pFlags & UINT32_C(0x40200)) == 0) {
        player->target = other;
        ai_WalktoPlayer(
            player, 2, closingRange);
        return;
    }

    (void)animctrl_MotionLockLevel(
        &player->playerRoot,
        &player->paMotions[0],
        0x16);
    (void)physics_gForceFaceTarget(
        &player->playerRoot,
        &player->target->playerRoot);
}

/* 0xF7820, 51 bytes, global, 2 named locals
 * shaolin_CheckMove
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\shaolin.c
 */
int shaolin_CheckMove(int move, int slot)
{
    int index;

    for (index = 0;
         index < JPB_KUNGFU_CAPACITY;
         ++index) {
        if (move == attackChoice[index]) {
            return 1;
        }
    }
    attackChoice[slot] = (uint8_t)move;
    return 0;
}

/* 0xF7860, 721 bytes, global, 12 named locals
 * shaolin_DoKungfu
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\shaolin.c
 */
void shaolin_DoKungfu(void)
{
    int i;
    Node *node;

    for (i = 0; i < 4; ++i) {
        physicsObject *p =
            &maPhysicsData[i >> 1];
        VECTOR dest = {
            (i & 1) != 0 ? -0x100 : 0x100,
            0,
            -0x55,
            0
        };
        VECTOR dest2;

        vec_RotVectorY(
            p->angle.vy, &dest, &dest2);
        dest2.vx =
            (int)((float)dest2.vx + p->pos.vx);
        dest2.vy =
            (int)((float)dest2.vy + p->pos.vy);
        dest2.vz =
            (int)((float)dest2.vz + p->pos.vz);
        if (first == 0) {
            point[i] = dest2;
        }
        point[i].vx = shaolin_smooth_component(
            point[i].vx, dest2.vx);
        point[i].vy = shaolin_smooth_component(
            point[i].vy, dest2.vy);
        point[i].vz = shaolin_smooth_component(
            point[i].vz, dest2.vz);
    }
    first = 1;

    for (node = kfList.head;
         node != NULL;
         node = node->next) {
        kfNode *n = (kfNode *)node;
        playerObject *player =
            shaolin_node_player(n);
        physicsObject *p =
            shaolin_node_physics(n);
        int closest = 0;
        uint32_t close;

        if (player == NULL || p == NULL) {
            n->flags = 1;
            continue;
        }
        close = vec_DistanceLV(
            &p->vpos, &point[0]);

        n->flags = 0;
        player->pFlags &=
            ~UINT32_C(0x400000);
        player->locked = NULL;
        for (i = 1; i < 4; ++i) {
            uint32_t dist =
                vec_DistanceLV(
                    &p->vpos, &point[i]);

            if (dist < close) {
                close = dist;
                closest = i;
            }
        }
        n->loc = (int16_t)closest;
    }

    for (;;) {
        kfNode *atkr = NULL;
        int chi = 0x100;

        for (node = kfList.head;
             node != NULL;
             node = node->next) {
            kfNode *n = (kfNode *)node;

            if (n->flags != 1 &&
                n->chi < chi) {
                atkr = n;
                chi = n->chi;
            }
        }
        if (atkr == NULL) {
            return;
        }

        {
            playerObject *player =
                shaolin_node_player(atkr);

            if (player == NULL ||
                player->target == NULL ||
                player->paMotions == NULL) {
                atkr->flags = 1;
                continue;
            }
            if ((player->target->pFlags &
                 UINT32_C(0x80)) != 0) {
                (void)animctrl_MotionLockLevel(
                    &player->playerRoot,
                    &player->paMotions[0],
                    0x19);
            } else {
                shaolin_Attack(atkr);
            }
            atkr->flags = 1;
        }
    }
}

/* 0xF7B40, 28 bytes, global, 2 named locals
 * shaolin_GetKungfu
 * PDB type: kfNode* (int)
 * Source: W:\SWJediPowerBattles\Work\shaolin.c
 */
kfNode *shaolin_GetKungfu(int index)
{
    kfNode *kungfu = &akfNodes[index];

    memset(kungfu, 0, sizeof(*kungfu));
    return kungfu;
}

/* 0xF7B60, 58 bytes, global, 0 named locals
 * shaolin_InitKungfu
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\shaolin.c
 */
void shaolin_InitKungfu(void)
{
    memset(akfNodes, 0, sizeof(akfNodes));
    memset(attackChoice, 0, sizeof(attackChoice));
    list_InitList(&kfList);
}

/* 0xF7BA0, 20 bytes, global, 0 named locals
 * shaolin_StartKungfu
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\shaolin.c
 */
void shaolin_StartKungfu(void)
{
    a[0] = 0;
    a[1] = 0;
    list_InitList(&kfList);
}
