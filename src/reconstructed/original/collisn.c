/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\collisn.c.
 *
 * This reviewed subset owns the collision-node registry, the exact
 * player/node contact solvers, including projectile reflection, and the
 * collision-node state accessors.
 *
 * Provenance:
 *   direct     - function names/signatures/locals from the exact PDB; Mnode
 *                type 0x119A and registry globals from linked symbols.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - complete bodies, offsets, mask widths, low-word stores,
 *                reset ranges, flag tests, and fatal registration paths
 *                checked at exact RVAs.
 *
 * PDB module: 0014
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\collisn.obj
 * Primary source: W:\SWJediPowerBattles\Work\collisn.c
 * Compiler language: c
 * Emitted procedures: 27
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/collision.h"
#include "jpb/braindmg.h"
#include "jpb/bullet.h"
#include "jpb/debugtext.h"
#include "jpb/effects.h"
#include "jpb/extracharacters.h"
#include "jpb/fmath.h"
#include "jpb/game.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/world.h"

#include <stdlib.h>
#include <string.h>

/*
 * Direct local symbols:
 *   maNodes    RVA 0x4F1AF0, Mnode*[20][32] (PDB type 0x133C)
 *   mNodeIndex RVA 0x4F2EF0, int[20]         (PDB type 0x133E)
 *
 * A flat backing array makes the reference's linear address calculation
 * defined even when coll_gRegisterNode examines an out-of-row static ID.
 */
static Mnode *maNodes[
    JPB_COLLISION_PLAYER_CAPACITY * JPB_COLLISION_NODE_CAPACITY];
static int32_t mNodeIndex[JPB_COLLISION_PLAYER_CAPACITY];

/*
 * Exact PDB global `mReflects`, type 0x1349 (`_svector[5]`), at matched-PC
 * RVA 0x4AFCB8. The component values were checked against initialized image
 * data as well as the three indexed loads in coll_CheckProjectileCollision.
 */
_svector mReflects[5] = {
    {64, 32, -64, 0},
    {-64, 32, 64, 0},
    {-64, 32, -64, 0},
    {64, 32, 64, 0},
    {64, -8, -96, 0}
};

static int32_t collision_wrap_subtract(int32_t left, int32_t right)
{
    return (int32_t)((uint32_t)left - (uint32_t)right);
}

static int32_t collision_wrap_add(int32_t left, int32_t right)
{
    uint32_t raw = (uint32_t)left + (uint32_t)right;
    int32_t result;

    memcpy(&result, &raw, sizeof(result));
    return result;
}

static int32_t collision_emitted_abs(int32_t value)
{
    if (value < 0) {
        uint32_t raw = UINT32_C(0) - (uint32_t)value;

        memcpy(&value, &raw, sizeof(value));
    }
    return value;
}

static int32_t collision_scale_radius(
    int16_t radius, int32_t scale)
{
    int32_t product = (int32_t)(
        (uint32_t)(int32_t)radius * (uint32_t)scale);

    /*
     * The reference's add-0xfff-for-negative arithmetic shift is exactly
     * C99 signed division truncated toward zero.
     */
    return product / 4096;
}

static int16_t collision_low_i16(int32_t value)
{
    uint16_t low = (uint16_t)(uint32_t)value;

    if (low <= INT16_MAX) {
        return (int16_t)low;
    }
    return (int16_t)((int32_t)low - 65536);
}

static int16_t collision_add_i16(int16_t left, int16_t right)
{
    return collision_low_i16(
        (int32_t)(uint16_t)left + (int32_t)(uint16_t)right);
}

static size_t collision_slot_index(int player, unsigned node_id)
{
    return (size_t)player * JPB_COLLISION_NODE_CAPACITY +
           (size_t)(node_id & NODE_INDEX_MASK);
}

static Mnode **collision_slot(int player, unsigned node_id)
{
    return &maNodes[collision_slot_index(player, node_id)];
}

static Mnode *collision_node(int player, unsigned node_id)
{
    return *collision_slot(player, node_id);
}

static Mnode *collision_signed_node(int player, int8_t node_id)
{
    ptrdiff_t index =
        (ptrdiff_t)player * JPB_COLLISION_NODE_CAPACITY +
        (ptrdiff_t)node_id;

    return maNodes[index];
}

static void collision_registration_failure(void)
{
    exit(EXIT_FAILURE);
}

/* 0x25630, 197 bytes, global, 8 named locals
 * coll_4DCollision
 * PDB type: int (VECTOR*, _svector*, VECTOR*...
 * Source: W:\SWJediPowerBattles\Work\collisn.c
 */
int coll_4DCollision(
    VECTOR *p0_B,
    _svector *v0,
    VECTOR *p1,
    _svector *v1,
    int dist)
{
    VECTOR p1_J;
    VECTOR p0_A;
    VECTOR p1_I;

    p0_A.vx = collision_wrap_subtract(p0_B->vx, v0->vx);
    p0_A.vy = collision_wrap_subtract(p0_B->vy, v0->vy);
    p0_A.vz = collision_wrap_subtract(p0_B->vz, v0->vz);
    p1_I.vx = collision_wrap_subtract(p1->vx, v0->vx);
    p1_I.vy = collision_wrap_subtract(p1->vy, v0->vy);
    p1_I.vz = collision_wrap_subtract(p1->vz, v0->vz);
    p1_J.vx = collision_wrap_subtract(p1->vx, v1->vx);
    p1_J.vy = collision_wrap_subtract(p1->vy, v1->vy);
    p1_J.vz = collision_wrap_subtract(p1->vz, v1->vz);
    return vec_PointNearSegment(dist, &p0_A, &p1_J, &p1_I) != 0;
}

/* 0x25700, 37 bytes, global, 4 named locals */
int coll_CheckForEventNode(int player, int node_id)
{
    return (int)(collision_node(player, (unsigned)node_id)->flags &
                 JPB_COLLISION_FLAG_EVENT);
}

/* 0x25730, 37 bytes, global, 4 named locals */
int coll_CheckForHotNode(int player, int node_id)
{
    return (int)(collision_node(player, (unsigned)node_id)->flags &
                 JPB_COLLISION_FLAG_HOT);
}

/* 0x25760, 37 bytes, global, 4 named locals */
int coll_CheckForSabreNode(int player, int node_id)
{
    return (int)(collision_node(player, (unsigned)node_id)->flags &
                 JPB_COLLISION_FLAG_SABRE);
}

/* 0x25790, 952 bytes, global, 13 named locals
 * coll_CheckNodeCollision
 * PDB type: int (playerObject*, CollisionData*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\collisn.c
 */
int coll_CheckNodeCollision(
    playerObject *attacker,
    CollisionData *attacker_node_data,
    playerObject *target)
{
    static char *const sabre_hit_sounds[4] = {
        "sabrhit1", "sabrhit2", "sabrhit3", "sabrhit4"
    };
    Mnode *attacker_node = collision_node(
        attacker->playernum,
        (uint32_t)(int32_t)attacker_node_data->id);
    int target_node_index;

    for (target_node_index = 0;
         target_node_index < target->numCollisionNodes;
         ++target_node_index) {
        CollisionData *target_node_data =
            &target->paNodesSizes[target_node_index];
        Mnode *target_node = collision_node(
            target->playernum,
            (uint32_t)(int32_t)target_node_data->id);

        if (target_node != NULL &&
            target_node_data->radius1 > 0) {
            int radius =
                (collision_scale_radius(
                     target_node_data->radius1,
                     target->fScale) +
                 collision_scale_radius(
                     attacker_node_data->radius1,
                     attacker->fScale)) /
                2;
            int collided = 0;

            if (attacker->playernum < 2 &&
                IsExtraCharacter(
                    (model_id)attacker->playerID) != 0) {
                collided =
                    vec_DistanceLV(
                        &attacker_node->v3RotCenter,
                        &target_node->v3RotCenter) <
                    (uint32_t)collision_wrap_add(radius, radius);
            } else {
                VECTOR attacker_previous;
                VECTOR target_previous;
                VECTOR relative_target;

                attacker_previous.vx = collision_wrap_subtract(
                    attacker_node->v3RotCenter.vx,
                    attacker_node->v3Velocity.vx);
                attacker_previous.vy = collision_wrap_subtract(
                    attacker_node->v3RotCenter.vy,
                    attacker_node->v3Velocity.vy);
                attacker_previous.vz = collision_wrap_subtract(
                    attacker_node->v3RotCenter.vz,
                    attacker_node->v3Velocity.vz);
                target_previous.vx = collision_wrap_subtract(
                    target_node->v3RotCenter.vx,
                    target_node->v3Velocity.vx);
                target_previous.vy = collision_wrap_subtract(
                    target_node->v3RotCenter.vy,
                    target_node->v3Velocity.vy);
                target_previous.vz = collision_wrap_subtract(
                    target_node->v3RotCenter.vz,
                    target_node->v3Velocity.vz);
                relative_target.vx = collision_wrap_subtract(
                    target_node->v3RotCenter.vx,
                    attacker_node->v3Velocity.vx);
                relative_target.vy = collision_wrap_subtract(
                    target_node->v3RotCenter.vy,
                    attacker_node->v3Velocity.vy);
                relative_target.vz = collision_wrap_subtract(
                    target_node->v3RotCenter.vz,
                    attacker_node->v3Velocity.vz);
                collided = vec_PointNearSegment(
                    radius,
                    &attacker_previous,
                    &target_previous,
                    &relative_target);
            }
            if (collided != 0) {
                _svector normalized_velocity;
                int velocity;

                if (attacker->playernum < 2) {
                    sceneObject *target_scene =
                        (sceneObject *)target->playerRoot.pParent;
                    physicsObject *target_physics =
                        (physicsObject *)target_scene->pPhysics;

                    if (IsExtraCharacter(
                            (model_id)attacker->playerID) == 0 &&
                        (uint16_t)(
                            (uint16_t)attacker->playerID -
                            UINT16_C(6)) > UINT16_C(1)) {
                        (void)sound_Play(
                            &target_physics->vpos,
                            0,
                            sabre_hit_sounds[
                                (unsigned)rand() & 3u],
                            0);
                    } else {
                        (void)sound_Play(
                            &target_physics->vpos,
                            0,
                            "jedihit",
                            0);
                    }
                }

                velocity = normalize(
                    attacker_node->v3Velocity.vx,
                    attacker_node->v3Velocity.vy,
                    attacker_node->v3Velocity.vz,
                    &normalized_velocity);
                target->hitVelocity.vx =
                    normalized_velocity.vx;
                target->hitVelocity.vy =
                    normalized_velocity.vy;
                target->hitVelocity.vz =
                    normalized_velocity.vz;
                target->hitVelocity.speed =
                    collision_low_i16(velocity);
                target->hitLocation.vx =
                    collision_wrap_add(
                        attacker_node->v3RotCenter.vx,
                        target_node->v3RotCenter.vx) /
                    2;
                target->hitLocation.vy =
                    collision_wrap_add(
                        attacker_node->v3RotCenter.vy,
                        target_node->v3RotCenter.vy) /
                    2;
                target->hitLocation.vz =
                    collision_wrap_add(
                        attacker_node->v3RotCenter.vz,
                        target_node->v3RotCenter.vz) /
                    2;
                if ((uint8_t)(
                        (uint8_t)attacker_node_data->id -
                        UINT8_C(12)) < UINT8_C(9) &&
                    (target->pFlags & 0x00000400u) == 0 &&
                    (uint8_t)(
                        (uint8_t)target_node_data->id -
                        UINT8_C(12)) < UINT8_C(9) &&
                    (attacker->pFlags & 0x00004000u) == 0) {
                    attacker->pFlags |= 0x00004000u;
                }
                target_node = collision_signed_node(
                    target->playernum, target_node_data->id);
                if ((target_node->flags & 0x00020000u) == 0) {
                    target_node->flags |= 0x00020000u;
                }
                target->hitMotion = *attacker->pMotion;
                return 1;
            }
        }
    }
    return 0;
}

/* 0x25B50, 1361 bytes, global, 22 named locals
 * coll_CheckProjectileCollision
 * PDB type: int (Projectile*)
 * Source: W:\SWJediPowerBattles\Work\collisn.c
 */
int coll_CheckProjectileCollision(Projectile *proj)
{
    ProjType *type =
        &((ProjType *)(void *)maProjTypes)[proj->pj_Type];
    playerObject *target =
        (playerObject *)(void *)proj->pj_Target;
    playerObject *owner =
        (playerObject *)(void *)proj->pj_Owner;
    int projSize = type->radius;
    int numNodes;
    int targetnode;

    if ((proj->pj_Flags & UINT32_C(0x20)) != 0) {
        projSize *= 4;
    }
    if ((proj->pj_Flags & UINT32_C(0x400)) != 0) {
        projSize *= 2;
    }
    if (GameStruct.CurrentLevel == UINT8_C(8)) {
        projSize = 0x100;
    }
    if (target == NULL) {
        return 0;
    }

    if (owner != NULL) {
        sceneObject *owner_scene =
            (sceneObject *)(void *)owner->playerRoot.pParent;
        playerObject *owner_player =
            (playerObject *)(void *)owner_scene->pPlayer;

        if (owner_player->playernum < 2 &&
            ((uint16_t)(owner_player->playerID - 0x12) &
             UINT16_C(0xffdf)) == 0 &&
            target->playerID == 0x2f) {
            projSize *= 4;
        }
    }

    numNodes = target->numCollisionNodes;
    if (numNodes == 0) {
        return 0;
    }
    if (GameStruct.CurrentLevel == UINT8_C(8)) {
        numNodes = 1;
    }
    if ((target->pFlags & UINT32_C(0x400)) != 0) {
        return 0;
    }

    for (targetnode = 0; targetnode < numNodes; ++targetnode) {
        CollisionData *collision =
            &target->paNodesSizes[targetnode];
        Mnode *nodeptr;
        int miss;

        if (collision->parentid != -1) {
            continue;
        }
        nodeptr = collision_node(
            target->playernum,
            (unsigned)(int)collision->id);
        if (nodeptr == NULL) {
            continue;
        }

        miss = collision_emitted_abs(collision_wrap_subtract(
                   proj->pj_Start.vx,
                   nodeptr->v3RotCenter.vx)) > projSize;
        if (collision_emitted_abs(collision_wrap_subtract(
                proj->pj_Start.vy,
                nodeptr->v3RotCenter.vy)) > projSize) {
            miss = 1;
        }
        if (collision_emitted_abs(collision_wrap_subtract(
                proj->pj_Start.vz,
                nodeptr->v3RotCenter.vz)) > projSize ||
            miss != 0) {
            continue;
        }

        target->target = owner;
        if ((type->flag & UINT16_C(4)) == 0 &&
            damageTracking[target->playerRoot.objectID].total < 1.0f &&
            (target->playerID < 6 ||
             target->playerID == 8 ||
             (target->playernum < 2 &&
              extracharacter_CanReflect(
                  (model_id)target->playerID) != 0))) {
            Mnode *wptr = collision_node(target->playernum, 12);

            if ((wptr->flags & UINT32_C(1)) != 0 &&
                (target->playerID < 6 ||
                 target->playerID == 8 ||
                 (target->playernum < 2 &&
                  extracharacter_CanReflect(
                      (model_id)target->playerID) != 0)) &&
                (target->pFlags & UINT32_C(0x20)) == 0) {
                target->pFlags |= UINT32_C(0x20);
            }

            if ((target->pFlags & UINT32_C(0x20)) != 0 ||
                (target->forceFlags & UINT32_C(2)) != 0) {
                physicsObject *p;
                VECTOR dest;

                if ((wptr->flags & UINT32_C(2)) == 0 &&
                    (target->forceFlags & UINT32_C(2)) == 0 &&
                    target->currentMotion != 0x0f) {
                    sceneObject *owner_scene =
                        (sceneObject *)(void *)
                            target->target->playerRoot.pParent;
                    VECTOR *targetpoint = &nodeptr->v3RotCenter;

                    p = (physicsObject *)(void *)
                        owner_scene->pPhysics;
                    dest.vx = collision_wrap_add(
                        targetpoint->vx,
                        mReflects[rand() % 5].vx);
                    dest.vy = collision_wrap_add(
                        targetpoint->vy,
                        mReflects[rand() % 5].vy);
                    dest.vz = collision_wrap_add(
                        targetpoint->vz,
                        mReflects[rand() % 5].vz);
                    bullet_ShootProjectile(
                        proj,
                        target,
                        targetpoint,
                        &dest,
                        (_svector *)(void *)&p->mov);
                    proj->pj_Dir.speed = collision_low_i16(
                        proj->pj_Dir.speed * 2);
                    proj->pj_Range =
                        (int16_t)(proj->pj_Range / 2);
                } else {
                    sceneObject *target_scene =
                        (sceneObject *)(void *)
                            target->playerRoot.pParent;
                    VECTOR *targetpoint = coll_GetNodeCenter(
                        target->target->playernum, 0);
                    int dist;
                    float travel;

                    p = (physicsObject *)(void *)
                        target_scene->pPhysics;
                    dist = (int)vec_DistanceLV(
                        targetpoint, &proj->pj_Start);
                    travel = (float)(
                        dist / (proj->pj_Dir.speed * 2));
                    dest.vx = collision_wrap_add(
                        (int32_t)(p->mov.vx * travel),
                        targetpoint->vx);
                    dest.vy = collision_wrap_add(
                        (int32_t)(p->mov.vy * travel),
                        targetpoint->vy);
                    dest.vz = collision_wrap_add(
                        (int32_t)(p->mov.vz * travel),
                        targetpoint->vz);
                    bullet_ShootProjectile(
                        proj,
                        target,
                        &proj->pj_Start,
                        &dest,
                        (_svector *)(void *)&p->mov);
                    (void)debug_printf("SHOOTS IT BACK!\n");
                    proj->pj_Dir.speed = collision_low_i16(
                        proj->pj_Dir.speed * 2);
                }

                p = (physicsObject *)(void *)
                    ((sceneObject *)(void *)
                         target->playerRoot.pParent)->pPhysics;
                (void)sound_Play(
                    &p->vpos,
                    0,
                    IsExtraCharacter(
                        (model_id)target->playerID) != 0
                        ? "ricoext"
                        : "rico1",
                    0);
                return -1;
            }
        }

        target->hitLocation.vx = collision_wrap_add(
            proj->pj_Start.vx, nodeptr->v3RotCenter.vx) / 2;
        target->hitLocation.vy = collision_wrap_add(
            proj->pj_Start.vy, nodeptr->v3RotCenter.vy) / 2;
        target->hitLocation.vz = collision_wrap_add(
            proj->pj_Start.vz, nodeptr->v3RotCenter.vz) / 2;
        if ((nodeptr->flags & UINT32_C(0x20000)) == 0) {
            nodeptr->flags |= UINT32_C(0x20000);
        }
        target->hitMotion =
            (Motion *)(void *)proj->pj_User;
        return 1;
    }
    return 0;
}

/* 0x260B0, 37 bytes, global, 5 named locals */
int coll_ChkNodeFlags(int player, int node_id, uint32_t flags)
{
    return (int)(collision_node(player, (unsigned)node_id)->flags & flags);
}

/* 0x260E0, 38 bytes, global, 5 named locals */
void coll_ClrNodeFlags(int player, int node_id, uint32_t flags)
{
    collision_node(player, (unsigned)node_id)->flags &= ~flags;
}

/* 0x26110, 28 bytes, global, 3 named locals */
Mnode *coll_GetNode(int player, unsigned node_id)
{
    return collision_node(player, node_id);
}

/* 0x26130, 62 bytes, global, 4 named locals */
VECTOR *coll_GetNodeCenter(int player, int node_id)
{
    Mnode *node = collision_node(player, (unsigned)node_id);

    if (node == NULL && node_id != 0) {
        node = collision_node(player, 0);
    }
    return node != NULL ? &node->v3RotCenter : NULL;
}

/* 0x26170, 32 bytes, global, 4 named locals */
_svector *coll_GetNodeRotation(int player, int node_id)
{
    return &collision_node(
        player, (unsigned)node_id)->v3CurrentRotation;
}

/* 0x26190, 32 bytes, global, 4 named locals */
_svector *coll_GetNodeRotationAbs(int player, int node_id)
{
    return &collision_node(player, (unsigned)node_id)->v3RotationAbs;
}

/* 0x261B0, 57 bytes, global, 5 named locals */
_svector *coll_GetNodeRotationDelta(
    int player, int node_id, _svector *rotation)
{
    Mnode *node = collision_node(player, (unsigned)node_id);

    rotation->vx = node->v3RotationDelta.vx;
    rotation->vy = node->v3RotationDelta.vy;
    rotation->vz = node->v3RotationDelta.vz;
    return rotation;
}

/* 0x261F0, 32 bytes, global, 4 named locals */
_svector *coll_GetNodeTranslation(int player, int node_id)
{
    return &collision_node(player, (unsigned)node_id)->v3Translation;
}

/* 0x26210, 32 bytes, global, 4 named locals */
_svector *coll_GetNodeVelocity(int player, int node_id)
{
    return &collision_node(player, (unsigned)node_id)->v3Velocity;
}

/* 0x26230, 76 bytes, global, 5 named locals */
void coll_IncNodeRotationAbs(
    int player, int node_id, _svector *rotation)
{
    Mnode *node = collision_node(player, (unsigned)node_id);

    node->v3RotationAbs.vx =
        collision_add_i16(node->v3RotationAbs.vx, rotation->vx);
    node->v3RotationAbs.vy =
        collision_add_i16(node->v3RotationAbs.vy, rotation->vy);
    node->v3RotationAbs.vz =
        collision_add_i16(node->v3RotationAbs.vz, rotation->vz);
    if ((node->flags &
         JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) == 0) {
        node->flags |= JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY;
    }
}

/* 0x26280, 76 bytes, global, 5 named locals */
void coll_IncNodeRotationDelta(
    int player, int node_id, _svector *rotation)
{
    Mnode *node = collision_node(player, (unsigned)node_id);

    node->v3RotationDelta.vx =
        collision_add_i16(node->v3RotationDelta.vx, rotation->vx);
    node->v3RotationDelta.vy =
        collision_add_i16(node->v3RotationDelta.vy, rotation->vy);
    node->v3RotationDelta.vz =
        collision_add_i16(node->v3RotationDelta.vz, rotation->vz);
    if ((node->flags &
         JPB_COLLISION_FLAG_ROTATION_DELTA_DIRTY) == 0) {
        node->flags |= JPB_COLLISION_FLAG_ROTATION_DELTA_DIRTY;
    }
}

/* 0x262D0, 58 bytes, global, 0 named locals */
void coll_ResetCollisionSystem(void)
{
    memset(mNodeIndex, 0, sizeof(mNodeIndex));
    memset(maNodes, 0, sizeof(maNodes));
}

/* 0x26310, 71 bytes, global, 1 named locals */
void coll_ResetPlayerCollision(int player)
{
    if ((uint32_t)player < JPB_COLLISION_PLAYER_CAPACITY) {
        mNodeIndex[player] = 0;
        memset(
            &maNodes[(size_t)player * JPB_COLLISION_NODE_CAPACITY],
            0,
            JPB_COLLISION_NODE_CAPACITY * sizeof(maNodes[0]));
    }
}

/* 0x26360, 48 bytes, global, 5 named locals */
void coll_SetNodeFlags(int player, int node_id, uint32_t flags)
{
    Mnode *node = collision_node(player, (unsigned)node_id);

    if ((node->flags & flags) == 0) {
        node->flags |= flags;
    }
}

/* 0x26390, 76 bytes, global, 5 named locals */
void coll_SetNodeRotationAbs(
    int player, int node_id, _svector *rotation)
{
    Mnode *node = collision_node(player, (unsigned)node_id);

    node->v3RotationAbs.vx = rotation->vx;
    node->v3RotationAbs.vy = rotation->vy;
    node->v3RotationAbs.vz = rotation->vz;
    if ((node->flags &
         JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) == 0) {
        node->flags |= JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY;
    }
}

/* 0x263E0, 76 bytes, global, 5 named locals */
void coll_SetNodeRotationDelta(
    int player, int node_id, _svector *rotation)
{
    Mnode *node = collision_node(player, (unsigned)node_id);

    node->v3RotationDelta.vx = rotation->vx;
    node->v3RotationDelta.vy = rotation->vy;
    node->v3RotationDelta.vz = rotation->vz;
    if ((node->flags &
         JPB_COLLISION_FLAG_ROTATION_DELTA_DIRTY) == 0) {
        node->flags |= JPB_COLLISION_FLAG_ROTATION_DELTA_DIRTY;
    }
}

/* 0x26430, 54 bytes, global, 5 named locals */
void coll_SetNodeTranslation(
    int player, int node_id, VECTOR *translation)
{
    Mnode *node = collision_node(player, (unsigned)node_id);

    node->v3Translation.vx = collision_low_i16(translation->vx);
    node->v3Translation.vy = collision_low_i16(translation->vy);
    node->v3Translation.vz = collision_low_i16(translation->vz);
}

/* 0x26470, 33 bytes, global, 5 named locals */
void coll_SetNodeZBufferOffset(int player, int node_id, int offset)
{
    collision_node(player, (unsigned)node_id)->ZBufferOffset =
        collision_low_i16(offset);
}

/* 0x264A0, 267 bytes, global, 6 named locals
 * coll_gCheckHotNodes
 * PDB type: int (playerObject*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\collisn.c
 */
int coll_gCheckHotNodes(
    playerObject *attacker, playerObject *target)
{
    int node_index;

    attacker->pFlags &= ~0x00010000u;
    if ((uint32_t)totalframes < target->hitDelay) {
        return 0;
    }
    for (node_index = 0;
         node_index < attacker->numCollisionNodes;
         ++node_index) {
        CollisionData *node_data =
            &attacker->paNodesSizes[node_index];

        if (node_data->radius1 > 0) {
            Mnode *node = collision_signed_node(
                attacker->playernum, node_data->id);

            if (node != NULL) {
                Mnode *hot_node =
                    node_data->parentid == -1
                    ? node
                    : collision_signed_node(
                          attacker->playernum,
                          node_data->parentid);

                if ((hot_node->flags &
                     JPB_COLLISION_FLAG_HOT) != 0 ||
                    (attacker->pFlags & 0x00000040u) != 0) {
                    attacker->pFlags |= 0x00010000u;
                    if (coll_CheckNodeCollision(
                            attacker,
                            node_data,
                            target) != 0) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* 0x265B0, 143 bytes, global, 2 named locals */
void coll_gRegisterNode(int player, Mnode *node)
{
    uint32_t id;
    unsigned index;
    Mnode **slot;

    if (mNodeIndex[player] >= JPB_COLLISION_NODE_CAPACITY) {
        collision_registration_failure();
    }
    id = (uint32_t)node->id;
    index = id & NODE_INDEX_MASK;
    if (index < JPB_COLLISION_NODE_CAPACITY) {
        slot = collision_slot(player, id);
        if ((id & (NODE_DYNAMIC | NODE_VIRTUAL)) != 0) {
            *slot = node;
            return;
        }
    } else {
        if (id == UINT32_MAX) {
            return;
        }
        if ((id & (NODE_DYNAMIC | NODE_VIRTUAL)) != 0) {
            collision_registration_failure();
        }
        slot = collision_slot(player, id);
    }
    if (*slot != NULL) {
        return;
    }
    collision_registration_failure();
}

/* 0x26640, 38 bytes, global, 4 named locals */
void old_coll_ZeroNodeTranslation(int player, int node_id)
{
    Mnode *node = collision_node(player, (unsigned)node_id);

    node->v3Translation.vz = 0;
    node->v3Translation.vx = 0;
}
