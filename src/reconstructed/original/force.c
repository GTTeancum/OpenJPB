/*
 * REVIEWED RECONSTRUCTION.
 * PDB module: 0037
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\force.obj
 * Primary source: W:\SWJediPowerBattles\Work\force.c
 * Compiler language: c
 * Emitted procedures: 23
 *
 * force_PlaySeq and all callback bodies below are checked against the raw
 * Ghidra export and exact x64 instructions. The completed module retains the
 * authored projectile, targeting, Force-state, animation, and effect paths.
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/force.h"

#include "jpb/anim.h"
#include "jpb/animctrl.h"
#include "jpb/animutil.h"
#include "jpb/bullet.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/effects.h"
#include "jpb/flex.h"
#include "jpb/fx.h"
#include "jpb/fmath.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/intersec.h"
#include "jpb/jedi.h"
#include "jpb/linkstubs.h"
#include "jpb/model.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/world.h"
#include "jpb/wrender.h"

#include <stddef.h>
#include <stdlib.h>

/* Exact PDB global at matched-PC RVA 0x10DB3B0. */
animObject *storeAnim;

/* Exact force.c PDB globals at matched-PC RVAs 0x537D18 and 0x537D20. */
static int zeroBSSCheck;
static Sprite *sptr[8];

/* Exact force.c PDB globals at matched-PC RVAs 0x537D60 and 0x537D70. */
static VECTOR pos;
static _svector delta;

/* Exact initialized force.c PDB globals at matched-PC RVAs 0x4BA990..99C. */
static int scale = 1;
static int scaleR = 0x200;
static int scaleA = -0x80;
static int g = 0xff;

/* Exact zero-initialized force.c PDB global at matched-PC RVA 0x537D78. */
static _svector force_reflect_rot;

static Motion *jpb_force_resolve_motion(
    playerObject *player, int16_t encoded_motion)
{
    int motion_index = encoded_motion;

    if (motion_index < 0) {
        motion_index = 59 - motion_index;
    }
    if (player == NULL ||
        player->paMotions == NULL ||
        motion_index < 0 ||
        motion_index >= player->maxMotions) {
        return NULL;
    }
    return &player->paMotions[motion_index];
}

static void jpb_force_clear_sequence_state(
    playerObject *player)
{
    player->forceData[0] = 0;
    player->forceData[1] = 0;
    player->forceData[2] = 0;
    player->forceData[3] = 0;
}

static int jpb_force_arithmetic_shift4(int value)
{
    if (value >= 0) {
        return value / 16;
    }
    return -1 -
           (int)((-(int64_t)value - 1) / 16);
}

static void jpb_force_set_star_effect_timing(
    Sprite **sprites)
{
    int index;

    if (sprites == NULL) {
        return;
    }
    for (index = 0; index < 7; ++index) {
        if (sprites[index] != NULL &&
            sprites[index]->sp_SCB != NULL) {
            sprites[index]
                ->sp_SCB
                ->scb_cvertex.pad =
                (int16_t)(index == 0 ? 6 : 10);
        }
    }
}

/* Exact dereference chain emitted by both callback bodies. */
static Motion *jpb_force_current_motion(
    playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    animObject *animation = (animObject *)scene->pAnim;

    return animation->pCurrentAnimSeq->pMotion;
}

/* 0x9FE60, 197 bytes, global, 12 named locals
 * color_interpolate
 * PDB type: unsigned long (unsigned long, un...
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
uint32_t color_interpolate(
    uint32_t a,
    uint32_t b,
    int scalar)
{
    int32_t a1 = (int32_t)(a >> 24);
    int32_t r1 =
        (int32_t)((a >> 16) & UINT32_C(0xff));
    int32_t g1 =
        (int32_t)((a >> 8) & UINT32_C(0xff));
    int32_t b1 =
        (int32_t)(a & UINT32_C(0xff));
    int32_t a2 = (int32_t)(b >> 24);
    int32_t r2 =
        (int32_t)((b >> 16) & UINT32_C(0xff));
    int32_t g2 =
        (int32_t)((b >> 8) & UINT32_C(0xff));
    int32_t b2 =
        (int32_t)(b & UINT32_C(0xff));
    int s2 = 256 - scalar;

    a1 = (a2 * s2 + a1 * scalar) / 256;
    r1 = (r2 * s2 + r1 * scalar) / 256;
    g1 = (g2 * s2 + g1 * scalar) / 256;
    b1 = (b2 * s2 + b1 * scalar) / 256;
    return
        ((uint32_t)a1 & UINT32_C(0xff)) << 24 |
        ((uint32_t)r1 & UINT32_C(0xff)) << 16 |
        ((uint32_t)g1 & UINT32_C(0xff)) << 8 |
        ((uint32_t)b1 & UINT32_C(0xff));
}

/* 0x9FF30, 9 bytes, global, 3 named locals
 * color_interpolate4k
 * PDB type: unsigned long (unsigned long, un...
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
uint32_t color_interpolate4k(
    uint32_t a,
    uint32_t b,
    int scalar)
{
    return color_interpolate(
        a,
        b,
        jpb_force_arithmetic_shift4(scalar));
}

/* 0x9FF40, 366 bytes, global, 6 named locals
 * force_AbsorbReflectCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_AbsorbReflectCallBack(
    int32_t *cpad, playerObject *player)
{
    Motion *motion;
    int force;
    VECTOR *pos0;

    motion = jpb_force_current_motion(player);
    force = game_gGetForce(player->playernum);

    if (force > 3 &&
        (cpad[1] & INT32_C(0x20)) != 0 &&
        game_gGetEnergy(player->playernum) > 0) {
        pos0 =
            coll_GetNodeCenter(
                player->playerRoot.objectID, 3);
        ++player->forceData[0];
        if (player->forceData[0] > 16) {
            (void)game_gModForce(
                player->playernum,
                player->fLife * 2 - 1);
            player->forceData[0] = 0;
        }
        ++player->forceData[2];
        if (player->forceData[2] > 50) {
            EffectHeader *effect = paEffects[69];

            player->forceData[2] = 0;
            if (effect != NULL && pos0 != NULL) {
                (void)sprite_AddSpriteEffect(
                    effect->aEffects,
                    (int)effect->num,
                    pos0,
                    NULL);
            }
        }
        player->fScale = INT32_C(0x36d8);
        if (player->currentMotion == 41 &&
            (int32_t)motion->motionFlags >= 0) {
            motion->motionFlags |=
                UINT32_C(0x80000000);
        }
        player->forceFlags |=
            UINT32_C(0x51);
        return 0;
    }

    player->forceFlags &=
        UINT32_C(0xffffffae);
    player->fScale = INT32_C(0x6db);
    motion->motionFlags &= UINT32_C(0x7fffffff);
    return 1;
}

/* 0xA00B0, 943 bytes, global, 6 named locals
 * force_AttackCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_AttackCallBack(
    int32_t *cpad, playerObject *player)
{
    static const unsigned node_ids[8] = {
        0, 7, 2, 5, 3, 6, 9, 13
    };
    int index;
    size_t node_index;

    (void)cpad;
    if (player == NULL ||
        player->playerRoot.pParent == NULL) {
        return 0;
    }

    index =
        animutl_gGetCurrentFrameIndex(
            &player->playerRoot);
    if (index == 4) {
        (void)game_gModForce(
            player->playernum, -25);
    } else if ((unsigned)(index - 5) < 7u) {
        player->pFlags |= UINT32_C(0x2000);
        for (node_index = 0;
             node_index <
                 sizeof(node_ids) /
                     sizeof(node_ids[0]);
             ++node_index) {
            Mnode *node =
                coll_GetNode(
                    player->playernum,
                    node_ids[node_index]);
            _svector start;
            _svector end;

            if (node == NULL) {
                continue;
            }
            end.vx =
                (int16_t)node->v3RotCenter.vx;
            end.vy =
                (int16_t)node->v3RotCenter.vy;
            end.vz =
                (int16_t)node->v3RotCenter.vz;
            end.pad = 0;
            start.vx =
                (int16_t)(
                    end.vx -
                    node->v3Velocity.vx);
            start.vy =
                (int16_t)(
                    end.vy -
                    node->v3Velocity.vy);
            start.vz =
                (int16_t)(
                    end.vz -
                    node->v3Velocity.vz);
            start.pad = 0;
            fx_screenGlow(
                &start,
                &end,
                30,
                UINT32_C(0xc0202020));
        }
    }
    player->pFlags &= UINT32_C(0xffffdfff);
    return 0;
}

/* 0xA0460, 232 bytes, global, 4 named locals
 * force_AttackSpinCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_AttackSpinCallBack(
    int32_t *cpad, playerObject *player)
{
    Motion *motion;
    int force;

    motion = jpb_force_current_motion(player);
    force = game_gGetForce(player->playernum);

    if (force > 3 &&
        (cpad[1] & INT32_C(0x10)) != 0) {
        ++player->forceData[0];
        if (player->forceData[0] > 2) {
            (void)game_gModForce(
                player->playernum,
                (cpad[1] & INT32_C(0xf000)) == 0
                    ? -2
                    : -4);
            player->forceData[0] = 0;
        }
        if ((int32_t)motion->motionFlags >= 0) {
            motion->motionFlags |=
                UINT32_C(0x80000000);
        }
        player->forceFlags |=
            UINT32_C(0x22);
        return 0;
    }

    motion->motionFlags &= UINT32_C(0x7fffffff);
    player->forceFlags &=
        UINT32_C(0xffffffdd);
    return 1;
}

/* 0xA0550, 359 bytes, global, 5 named locals
 * force_CloakCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_CloakCallBack(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene;
    modelObject *model;
    objectRoot *physics;
    VECTOR *position;
    int cancel_requested = 0;
    int alternate_color;

    (void)cpad;
    if (player == NULL ||
        player->playerRoot.pParent == NULL) {
        return 1;
    }
    scene =
        (sceneObject *)player->playerRoot.pParent;
    model = (modelObject *)scene->pModel;
    physics = scene->pPhysics;
    position =
        coll_GetNodeCenter(
            player->playerRoot.objectID, 0);
    (void)game_gGetForce(player->playernum);

    if (player->forceData[0] == 0) {
        player->forceData[0] = 1;
        player->forceData[1] = 600;
        if (paEffects[80] != NULL &&
            position != NULL) {
            (void)sprite_AddSpriteEffect(
                paEffects[80]->aEffects,
                (int)paEffects[80]->num,
                position,
                NULL);
        }
        (void)game_gModForce(
            player->playernum, -20);
    }

    --player->forceData[1];
    if (gpWorld != NULL &&
        gpWorld->currentDolly >= 0 &&
        gpWorld->currentDolly < 256) {
        cancel_requested =
            (gpWorld
                 ->aDolly[gpWorld->currentDolly]
                 .flags &
             UINT32_C(0x400)) != 0;
    }
    if (player->forceData[1] < 0 ||
        cancel_requested) {
        player->forceFlags &=
            UINT32_C(0xffffff7f);
        if (model != NULL) {
            model->flags &=
                UINT32_C(0xffffffef);
        }
        if (paEffects[80] != NULL &&
            position != NULL) {
            (void)sprite_AddSpriteEffect(
                paEffects[80]->aEffects,
                (int)paEffects[80]->num,
                position,
                NULL);
        }
        return 1;
    }

    player->forceFlags |= UINT32_C(0x80);
    if (model != NULL) {
        model->flags |= UINT32_C(0x10);
    }
    alternate_color = player->playerID != 3;
    fx_GlowingMan(
        physics,
        48,
        54,
        alternate_color
            ? UINT32_C(0xc0200820)
            : UINT32_C(0xc0200808),
        alternate_color
            ? UINT32_C(0xc0301030)
            : UINT32_C(0xc0301010));
    return 0;
}

/* 0xA06C0, 536 bytes, global, 9 named locals
 * force_FlameCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_FlameCallBack(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    modelObject *model =
        (modelObject *)scene->pModel;
    int frame = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);
    int alternate_motion = player->currentMotion != 0x16;
    int projectile_type = alternate_motion ? 0x0a : 0x13;
    int muzzle_node;
    int aim_node;
    Projectile *proj;
    VECTOR *pos0;
    VECTOR *pos1;
    VECTOR temp;
    uint32_t original_flags;

    (void)cpad;
    (void)frame;
    if (model->eventMask == 0) {
        return 0;
    }

    if (mDrawingSurfaceId == 0) {
        muzzle_node = alternate_motion ? 0x0e : 0x0b;
        aim_node = alternate_motion ? 0x12 : 0x0a;
    } else {
        muzzle_node = alternate_motion ? 0x0a : 0x0f;
        aim_node = alternate_motion ? 0x11 : 0x0e;
    }

    proj = bullet_AllocProjectile(projectile_type);
    if (proj == NULL) {
        return 0;
    }
    pos0 = coll_GetNodeCenter(player->playernum, muzzle_node);
    pos1 = coll_GetNodeCenter(player->playernum, aim_node);
    temp = *pos1;
    temp.vy += rand() % 16 / 2;

    original_flags = (uint32_t)proj->pj_Flags;
    proj->pj_Flags = (int32_t)(original_flags | UINT32_C(0x0810));
    if (player->playernum > 1) {
        proj->pj_Flags =
            (int32_t)(original_flags | UINT32_C(0x1810));
    }
    if (player->currentMotion == 0x16) {
        proj->color = (int32_t)UINT32_C(0x7f004000);
        proj->pj_Flags |= (int32_t)UINT32_C(0x2000);
        proj->launchID = (char)(
            mDrawingSurfaceId == 0 ? 0x0b : 0x0f);
    }
    bullet_ShootProjectile(proj, player, pos0, &temp, NULL);
    return 0;
}

/* 0xA08E0, 100 bytes, global, 4 named locals
 * force_HealingCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_HealingCallBack(
    int32_t *cpad, playerObject *player)
{
    VECTOR *pos0;

    (void)cpad;
    if (player == NULL) {
        return 1;
    }
    pos0 =
        coll_GetNodeCenter(
            player->playerRoot.objectID, 0);
    (void)game_gModForce(
        player->playernum, -10);
    (void)game_gModEnergy(
        player->playernum, 25);
    if (paEffects[12] != NULL && pos0 != NULL) {
        (void)sprite_AddSpriteEffect(
            paEffects[12]->aEffects,
            (int)paEffects[12]->num,
            pos0,
            NULL);
    }
    return 1;
}

/* 0xA0950, 520 bytes, global, 8 named locals
 * force_MesmerizeCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_MesmerizeCallBack(
    int32_t *cpad, playerObject *player)
{
    VECTOR position;
    uint32_t checked = UINT32_C(3);
    physicsObject *target_physics;
    int effect_index;
    int cancel_requested = 0;

    (void)cpad;
    if (player == NULL) {
        return 1;
    }
    (void)game_gGetForce(player->playernum);
    position.vx = (int32_t)player->forceData[0];
    position.vy = (int32_t)player->forceData[1];
    position.vz = (int32_t)player->forceData[2];
    position.pad = 0;
    effect_index =
        player->playerID == 8 ? 43 : 38;

    if (gpWorld != NULL &&
        gpWorld->currentDolly >= 0 &&
        gpWorld->currentDolly < 256) {
        cancel_requested =
            (gpWorld
                 ->aDolly[gpWorld->currentDolly]
                 .flags &
             UINT32_C(0x400)) != 0;
    }
    if (cancel_requested) {
        return 1;
    }

    if (player->forceData[4] == 0 &&
        paEffects[effect_index] != NULL) {
        (void)sprite_AddSpriteEffect(
            paEffects[effect_index]->aEffects,
            (int)paEffects[effect_index]->num,
            &position,
            NULL);
    }

    target_physics =
        physics_FindWithinRange(
            &position, &checked, 307);
    while (target_physics != NULL) {
        sceneObject *target_scene =
            (sceneObject *)
                target_physics
                    ->physicsRoot
                    .pParent;
        playerObject *target = NULL;
        modelObject *target_model = NULL;

        if (target_scene != NULL) {
            target =
                (playerObject *)
                    target_scene->pPlayer;
            target_model =
                (modelObject *)
                    target_scene->pModel;
        }
        if (target != NULL &&
            (target->pEnemy == NULL ||
             (target->pEnemy->pPlace != NULL &&
              target
                      ->pEnemy
                      ->pPlace
                      ->aiDf
                      .ownerType == 2)) &&
            (target->pFlags &
             UINT32_C(0x44002c01)) == 0 &&
            target->playerID != 43) {
            target->pFlags &=
                UINT32_C(0xffffffdf);
            if (target->paMotions != NULL) {
                if ((target->pFlags &
                     UINT32_C(0x8000)) == 0) {
                    if (target->maxMotions > 61) {
                        (void)animctrl_MotionLock(
                            &target->playerRoot,
                            &target->paMotions[61]);
                    }
                } else if (target->maxMotions > 6) {
                    (void)animctrl_MotionNoLock(
                        &target->playerRoot,
                        &target->paMotions[6]);
                }
            }
            if (player->forceData[4] ==
                    target
                        ->playerRoot
                        .objectID &&
                target_model != NULL &&
                paEffects[effect_index] != NULL) {
                VECTOR *target_position =
                    coll_GetNodeCenter(
                        target
                            ->playerRoot
                            .objectID,
                        (int)(
                            target_model
                                ->idMask &
                            UINT32_C(8)));

                if (target_position != NULL) {
                    (void)sprite_AddSpriteEffect(
                        paEffects[effect_index]
                            ->aEffects,
                        (int)paEffects[effect_index]
                            ->num,
                        target_position,
                        NULL);
                }
            }
        }
        target_physics =
            physics_FindWithinRange(
                &position, &checked, 307);
    }

    ++player->forceData[4];
    if (player->forceData[4] > 20) {
        player->forceData[4] = 0;
    }
    ++player->forceData[3];
    return player->forceData[3] > 300;
}

/* 0xA0B60, 1270 bytes, global, 9 named locals
 * force_PlaySeq
 * PDB type: int (ForceSlot*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_PlaySeq(
    ForceSlot *slot, playerObject *player)
{
    short *seq;
    int i = 1;
    Motion *table;
    int move;
    int force;
    physicsObject *p;
    sceneObject *scene;

    if (slot == NULL ||
        player == NULL ||
        slot->map[0] == 0 ||
        player->playerRoot.pParent == NULL) {
        return 0;
    }
    scene =
        (sceneObject *)player->playerRoot.pParent;
    p = (physicsObject *)scene->pPhysics;
    if (scene->pAnim == NULL || p == NULL) {
        return 0;
    }

    seq = &slot->map[1];
    move = slot->map[0];
    force = game_gGetForce(player->playernum);
    table =
        jpb_force_resolve_motion(
            player, (int16_t)move);
    if (table == NULL) {
        return 0;
    }

    switch (slot->idx) {
    case 0:
        table->FunctPtr = 23;
        storeAnim = (animObject *)scene->pAnim;
        break;
    case 3:
        player->forceData[0] = 0;
        player->forceData[1] = 1;
        player->forceData[2] = 22;
        player->forceData[3] = 0;
        table->FunctPtr = 16;
        break;
    case 4:
        table->FunctPtr = 14;
        break;
    case 5:
        table->FunctPtr = 15;
        break;
    case 6:
        table->FunctPtr = 8;
        break;
    case 8:
        jpb_force_clear_sequence_state(player);
        player->pForceCallBack =
            force_ShieldCallBack;
        break;
    case 9:
        table->FunctPtr = 12;
        break;
    case 10:
        player->forceData[0] = 0;
        table->FunctPtr = 19;
        break;
    case 11:
        if (force < 36) {
            return 0;
        }
        (void)game_gModForce(
            player->playernum, -32);
        player->forceData[0] =
            (int64_t)p->pos.vx;
        player->forceData[1] =
            (int64_t)(p->pos.vy + 128.0f);
        player->forceData[2] =
            (int64_t)p->pos.vz;
        player->forceData[3] = 0;
        player->pForceCallBack =
            force_MesmerizeCallBack;
        break;
    case 12:
        jpb_force_clear_sequence_state(player);
        table->FunctPtr = 24;
        break;
    case 13:
        jpb_force_clear_sequence_state(player);
        player->pForceCallBack =
            force_StarCallBack;
        break;
    case 14:
        jpb_force_clear_sequence_state(player);
        player->pForceCallBack =
            force_CloakCallBack;
        break;
    case 15:
        table->FunctPtr = 18;
        break;
    case 16:
        table->FunctPtr = 17;
        break;
    default:
        break;
    }

    if (animctrl_MotionComboChain(
            &player->playerRoot,
            table,
            0,
            0,
            0) == 0) {
        return 0;
    }

    while (*seq != 0) {
        move = *seq++;
        table =
            jpb_force_resolve_motion(
                player, (int16_t)move);
        if (table == NULL) {
            break;
        }

        if (i == 1) {
            if (slot->idx == 2) {
                player->forceData[0] = 0;
                player->forceData[1] = 0;
                player->forceData[2] = 50;
                if ((int32_t)table->motionFlags >= 0) {
                    table->motionFlags |=
                        UINT32_C(0x80000000);
                }
                table->FunctPtr = 7;
            }
            if (slot->idx == 3) {
                if ((int32_t)table->motionFlags >= 0) {
                    table->motionFlags |=
                        UINT32_C(0x80000000);
                }
                table->FunctPtr = 16;
            }
            if (slot->idx == 7) {
                EffectHeader *effect = paEffects[74];

                player->forceData[0] = 0;
                player->forceData[1] = 0;
                if ((int32_t)table->motionFlags >= 0) {
                    table->motionFlags |=
                        UINT32_C(0x80000000);
                }
                table->FunctPtr = 9;
                (void)game_gModForce(
                    player->playernum, -20);
                if (effect != NULL) {
                    (void)sprite_AddSpriteEffect(
                        effect->aEffects,
                        (int)effect->num,
                        &p->vpos,
                        NULL);
                }
            }
        }
        if (animctrl_MotionComboChain(
                &player->playerRoot,
                table,
                1,
                0,
            0) != 0) {
            ++i;
        }
    }
    return 1;
}

/* 0xA1060, 332 bytes, global, 9 named locals
 * force_PushCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_PushCallBack(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    int force = game_gGetForce(player->playernum);
    int index = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);
    int type = player->currentMotion == 0 ? 5 : 0x10;

    (void)cpad;
    (void)force;
    if (index > 7) {
        Projectile *proj = bullet_AllocProjectile(type);

        if (proj != NULL) {
            VECTOR *pos0 = coll_GetNodeCenter(
                player->playernum, 8);
            VECTOR *pos1 = coll_GetNodeCenter(
                player->playernum, 0x0f);
            VECTOR temp;
            playerObject *target;

            temp.vx = pos1->vx;
            temp.vy = pos0->vy;
            temp.vz = pos1->vz;
            temp.pad = 0;
            (void)game_gModForce(player->playernum, -20);
            proj->pj_Flags |= (int32_t)UINT32_C(0x10);
            target = FindBestMachineGunTarget(
                pos0,
                &physics->angle,
                player,
                0x0c,
                0x100,
                0x0a00,
                GameStruct.versusModeFlag);
            if (target != NULL) {
                player->target = target;
            }
            bullet_ShootProjectile(
                proj, player, pos0, &temp, NULL);
        }
        return 1;
    }
    return 0;
}

/* 0xA11B0, 461 bytes, global, 12 named locals
 * force_Ranged3CallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
static void force_fire_ranged3_projectile(
    playerObject *player,
    int aim_node,
    uint32_t *mask)
{
    Projectile *proj = bullet_AllocProjectile(0x1c);

    if (proj != NULL) {
        VECTOR *pos0 = coll_GetNodeCenter(
            player->playernum, 8);
        VECTOR *pos1 = coll_GetNodeCenter(
            player->playernum, aim_node);
        VECTOR temp;
        physicsObject *target_physics;

        temp.vx = pos1->vx;
        temp.vy = pos0->vy;
        temp.vz = pos1->vz;
        temp.pad = 0;
        (void)game_gModForce(player->playernum, -20);
        proj->pj_Flags |= (int32_t)UINT32_C(0x2010);
        proj->color = (int32_t)UINT32_C(0x40400000);
        proj->launchID = (char)aim_node;
        target_physics = physics_FindWithinRange(
            pos0, mask, 0x0c00);
        if (target_physics != NULL) {
            sceneObject *target_scene =
                (sceneObject *)target_physics
                    ->physicsRoot.pParent;

            player->target =
                (playerObject *)target_scene->pPlayer;
        }
        bullet_ShootProjectile(
            proj, player, pos0, &temp, NULL);
    }
}

int force_Ranged3CallBack(
    int32_t *cpad, playerObject *player)
{
    int index = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);
    uint32_t mask = UINT32_C(3);

    (void)cpad;
    if (index > 1) {
        force_fire_ranged3_projectile(
            player, 0x0f, &mask);
        force_fire_ranged3_projectile(
            player, 0x0b, &mask);
        return 1;
    }
    return 0;
}

/* 0xA1380, 1006 bytes, global, 18 named locals
 * force_ReflectCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_ReflectCallBack(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    animObject *animation =
        (animObject *)scene->pAnim;
    Motion *motion = animation->pCurrentAnimSeq->pMotion;
    int force = game_gGetForce(player->playernum);

    if (force > 3 &&
        ((uint32_t)cpad[1] & UINT32_C(0x20)) != 0) {
        VECTOR *location = &physics->vpos;
        uint32_t color;
        uint32_t inner_color;
        float scale_factor;
        int x;
        int circle_index = 0;

        if (player->forceData[3] == 0) {
            int bank = player->playernum + 1;

            if (bank > 3) {
                bank = 3;
            }
            (void)sound_Play(
                location, bank, (char *)"trflsphr", 0);
            player->forceData[3] = 1;
        }
        (void)game_gModForce(
            player->playernum,
            -(mDrawingSurfaceId & 1));
        if (g == 0x5f || g == 0x7f) {
            EffectHeader *effect = paEffects[68];

            if (effect != NULL) {
                (void)sprite_AddSpriteEffect(
                    effect->aEffects,
                    (int)effect->num,
                    location,
                    NULL);
            }
        }

        color = jedi_GetColour32(
            (uint64_t)(uint16_t)player->playerID);
        color = (color & UINT32_C(0x00ffffff)) |
            UINT32_C(0x60000000);
        inner_color =
            ((uint32_t)(g / 2) << 24) |
            UINT32_C(0x00101010);
        scale_factor = (float)scale * (1.0f / 4096.0f);
        for (x = 0; x < 0x100; x += 0x10) {
            uint32_t outer_color = color_interpolate(
                color, inner_color, g / 2 + x);
            float radius1 = (float)(int32_t)(
                (float)x * scale_factor);
            float radius2 = (float)(int32_t)(
                (float)(x + 0x10) * scale_factor);
            float h1 = (float)(int32_t)(
                (float)aCircle[circle_index] *
                scale_factor);
            float h2 = (float)(int32_t)(
                (float)aCircle[circle_index + 1] *
                scale_factor);

            drawCylinderG(
                location,
                &force_reflect_rot,
                radius1,
                radius2,
                h1,
                h2,
                color,
                outer_color);
            ++circle_index;
        }

        player->fScale = 0x36d8;
        physics->mass = 0x2000;
        motion->Delay = 5;
        force_reflect_rot.vy = (int16_t)(
            force_reflect_rot.vy +
            flexmul12(0x20, gGlobalFrameRate));
        scale += flexmul12(scaleR, gGlobalFrameRate);
        scaleR += flexmul12(scaleA, gGlobalFrameRate);
        scaleA /= 2;
        if (scaleR < 0) {
            scaleR = 0;
        }
        g -= flexmul12(0x10, gGlobalFrameRate);
        if (g < 0) {
            g = 0xff;
            scale = 1;
            scaleR = 0x200;
            scaleA = -0x80;
        }
        player->forceFlags |= UINT32_C(0x52);
        return 0;
    }

    player->forceFlags &= UINT32_C(0xffffffad);
    motion->motionFlags &= UINT32_C(0x7fffffff);
    player->fScale = 0x6db;
    physics->mass = 0x800;
    motion->Delay = 1;
    jpb_force_clear_sequence_state(player);
    g = 0xff;
    scale = 1;
    scaleR = 0x200;
    scaleA = -0x80;
    return 1;
}

/* 0xA1770, 211 bytes, global, 6 named locals
 * force_RingCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_RingCallBack(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    int force = game_gGetForce(player->playernum);
    int index = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);

    (void)cpad;
    if (force > 4 || player->playerID == 9 ||
        player->playerID == 0x2b) {
        if (index < 2) {
            return 0;
        }
        {
            int type = player->playerID == 1
                ? 0x0b
                : 0x0e;
            Projectile *proj = bullet_AllocProjectile(type);

            if (proj != NULL) {
                VECTOR *pos = &physics->vpos;

                (void)game_gModForce(
                    player->playernum, -15);
                bullet_ShootProjectile(
                    proj, player, pos, pos, NULL);
                (void)BigBlowMe(pos, 2);
            }
        }
    }
    return 1;
}

/* 0xA1850, 417 bytes, global, 5 named locals
 * force_SabreSpinCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_SabreSpinCallBack(
    int32_t *cpad, playerObject *player)
{
    Mnode *sabre;
    Mnode *target;
    int index;

    (void)cpad;
    if (player == NULL ||
        player->playerRoot.pParent == NULL) {
        return 0;
    }
    (void)game_gGetForce(player->playernum);
    index =
        animutl_gGetCurrentFrameIndex(
            &player->playerRoot);
    sabre =
        coll_GetNode(
            player->playerRoot.objectID, 12);
    target =
        coll_GetNode(
            player->playerRoot.objectID, 20);
    if (sabre == NULL || target == NULL) {
        return 0;
    }

    player->forceFlags |= UINT32_C(0x10);
    if (index > 63) {
        sabre->flags &=
            UINT32_C(0xfbffffff);
        sabre->v3Velocity2.vx = 0;
        sabre->v3Velocity2.vy = 0;
        sabre->v3Velocity2.vz = 0;
        sabre->v3Translation2.vx = 0;
        sabre->v3Translation2.vy = 0;
        sabre->v3Translation2.vz = 0;
        player->forceFlags &=
            UINT32_C(0xffffffed);
        player->pFlags &=
            UINT32_C(0xffffdfff);
        (void)game_gModForce(
            player->playernum, -15);
        return 1;
    }

    if (index > 9) {
        sabre->v3Velocity2.vx = 0;
        sabre->v3Velocity2.vy = 0;
        sabre->v3Velocity2.vz = 0;
        sabre->v3Translation2.vx =
            (int16_t)target->v3RotCenter.vx;
        sabre->v3Translation2.vy =
            (int16_t)target->v3RotCenter.vy;
        sabre->v3Translation2.vz =
            (int16_t)target->v3RotCenter.vz;
        sabre->flags |=
            UINT32_C(0x04000000);
        coll_SetNodeFlags(
            player->playernum, 20, 1);
        coll_SetNodeFlags(
            player->playernum, 21, 1);
        coll_SetNodeFlags(
            player->playernum, 22, 1);
        sabre->time = 0;
        player->forceFlags |=
            UINT32_C(0x12);
        player->pFlags |=
            UINT32_C(0x2000);
    }
    return 0;
}

/* 0xA1A00, 690 bytes, global, 8 named locals
 * force_SabreTossCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_SabreTossCallBack(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    Mnode *sabre;
    int index;

    (void)cpad;
    (void)game_gGetForce(player->playernum);
    index = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);
    sabre = coll_GetNode(
        player->playerRoot.objectID, 12);
    player->forceFlags |= UINT32_C(0x10);
    if (index < 42) {
        if (index > 9) {
            playerObject *target;
            VECTOR *node;
            _mvector home;
            int dist;

            sabre->v3Velocity2.vx = 0;
            sabre->v3Velocity2.vy = 0;
            sabre->v3Velocity2.vz = 0;
            ++player->forceData[0];
            if (player->forceData[0] == 1) {
                delta = sabre->v3CurrentRotation;
                pos = sabre->v3RotCenter;
            }
            target = FindBestMachineGunTarget(
                &sabre->v3RotCenter,
                &physics->angle,
                player,
                4,
                0x100,
                0x0a00,
                GameStruct.versusModeFlag);
            if (target == NULL) {
                target = player->target;
            }
            node = coll_GetNodeCenter(
                target->playernum, 0);
            dist = normalize(
                node->vx - pos.vx,
                node->vy - pos.vy,
                node->vz - pos.vz,
                (_svector *)(void *)&home);
            home.speed = (int16_t)(dist / 4);
            if (home.speed < 0x10) {
                home.speed = 0x10;
            } else if (home.speed > 0x40) {
                home.speed = 0x40;
            }
            (void)MoveObject(&home, &pos, SMALL_HIT);
            sabre->v3Translation2.vx =
                (int16_t)pos.vx;
            sabre->v3Translation2.vy =
                (int16_t)pos.vy;
            sabre->v3Translation2.vz =
                (int16_t)pos.vz;
            sabre->v3RotCenter = pos;
            sabre->flags |= UINT32_C(0x04000000);
            sabre->flags |= UINT32_C(0x20);
            delta.vz = (int16_t)(delta.vz + 0x100);
            delta.vx = (int16_t)(delta.vx + 0x0c0);
            coll_SetNodeRotationAbs(
                player->playernum, 12, &delta);
            sabre->time = 0;
            player->forceFlags |= UINT32_C(0x10);
            player->pFlags |= UINT32_C(0x2000);
        }
        return 0;
    }

    sabre->flags &= UINT32_C(0xfbffffff);
    sabre->v3Velocity2.vx = 0;
    sabre->v3Velocity2.vy = 0;
    sabre->v3Velocity2.vz = 0;
    sabre->v3Translation2.vx = 0;
    sabre->v3Translation2.vy = 0;
    sabre->v3Translation2.vz = 0;
    player->forceFlags &= UINT32_C(0xffffffef);
    sabre->flags &= UINT32_C(0xffffffdf);
    player->pFlags &= UINT32_C(0xffffdfff);
    (void)game_gModForce(player->playernum, -20);
    player->forceData[0] = 0;
    return 1;
}

/* 0xA1CC0, 367 bytes, global, 5 named locals
 * force_SabreYoYoBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_SabreYoYoBack(
    int32_t *cpad, playerObject *player)
{
    Mnode *sabre;
    Mnode *target;
    int index;

    (void)cpad;
    if (player == NULL ||
        player->playerRoot.pParent == NULL) {
        return 0;
    }
    (void)game_gGetForce(player->playernum);
    index =
        animutl_gGetCurrentFrameIndex(
            &player->playerRoot);
    sabre =
        coll_GetNode(
            player->playerRoot.objectID, 12);
    target =
        coll_GetNode(
            player->playerRoot.objectID, 20);
    if (sabre == NULL || target == NULL) {
        return 0;
    }

    player->forceFlags |= UINT32_C(0x10);
    if (index >= 23) {
        sabre->flags &=
            UINT32_C(0xfbffffff);
        sabre->v3Velocity2.vx = 0;
        sabre->v3Velocity2.vy = 0;
        sabre->v3Velocity2.vz = 0;
        sabre->v3Translation2.vx = 0;
        sabre->v3Translation2.vy = 0;
        sabre->v3Translation2.vz = 0;
        player->forceFlags &=
            UINT32_C(0xffffffef);
        player->pFlags &=
            UINT32_C(0xffffdfff);
        (void)game_gModForce(
            player->playernum, -15);
        return 1;
    }

    if (index > 6) {
        sabre->v3Velocity2.vx = 0;
        sabre->v3Velocity2.vy = 0;
        sabre->v3Velocity2.vz = 0;
        sabre->v3Translation2.vx =
            (int16_t)target->v3RotCenter.vx;
        sabre->v3Translation2.vy =
            (int16_t)target->v3RotCenter.vy;
        sabre->v3Translation2.vz =
            (int16_t)target->v3RotCenter.vz;
        sabre->flags |=
            UINT32_C(0x04000000);
        coll_SetNodeFlags(
            player->playernum, 20, 1);
        coll_SetNodeFlags(
            player->playernum, 21, 1);
        coll_SetNodeFlags(
            player->playernum, 22, 1);
        sabre->time = 0;
        player->pFlags |=
            UINT32_C(0x2000);
    }
    return 0;
}

/* 0xA1E30, 712 bytes, global, 9 named locals
 * force_ShieldCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_ShieldCallBack(
    int32_t *cpad, playerObject *player)
{
    physicsObject *phy;
    int x;
    VECTOR *pos;
    Sprite **s;
    _svector temp;
    sceneObject *scene;
    int sprite_count = 0;
    int64_t remaining;

    (void)cpad;
    if (player == NULL ||
        player->playerRoot.pParent == NULL) {
        return 1;
    }
    scene =
        (sceneObject *)player->playerRoot.pParent;
    phy = (physicsObject *)scene->pPhysics;
    pos =
        coll_GetNodeCenter(
            player->playerRoot.objectID, 0);
    if (phy == NULL || pos == NULL) {
        return 1;
    }

    if (zeroBSSCheck != zerobss_levelReset) {
        zeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        for (x = 0; x < 8; ++x) {
            sprite_gFreeSprite(sptr[x]);
            sptr[x] = NULL;
        }
    }

    if (player->forceData[0] == 0) {
        player->forceData[0] = 1;
        player->forceData[1] = 600;
        if (paEffects[50] != NULL) {
            (void)sprite_AddSpriteEffect(
                paEffects[50]->aEffects,
                (int)paEffects[50]->num,
                &phy->vpos,
                NULL);
        }
        if (paEffects[51] != NULL) {
            sprite_count = (int)paEffects[51]->num;
            if (sprite_count > 8) {
                sprite_count = 8;
            }
            s =
                sprite_AddSpriteEffect(
                    paEffects[51]->aEffects,
                    (int)paEffects[51]->num,
                    pos,
                    NULL);
            if (s != NULL) {
                for (x = 0; x < sprite_count; ++x) {
                    sptr[x] = s[x];
                }
            }
        }
        (void)game_gModItemCount(
            player->playernum, -1);
    }

    remaining = player->forceData[1];
    player->forceData[1] = remaining - 1;
    if (remaining < 1) {
        player->fScale = INT32_C(0x6db);
        player->forceFlags &=
            UINT32_C(0xffffffad);
        if (paEffects[52] != NULL) {
            (void)sprite_AddSpriteEffect(
                paEffects[52]->aEffects,
                (int)paEffects[52]->num,
                &phy->vpos,
                NULL);
        }
        sprite_count =
            paEffects[51] == NULL
                ? 0
                : (int)paEffects[51]->num;
        if (sprite_count > 8) {
            sprite_count = 8;
        }
        for (x = 0; x < sprite_count; ++x) {
            sprite_gFreeSprite(sptr[x]);
            sptr[x] = NULL;
        }
        return 1;
    }

    player->forceFlags |= UINT32_C(0x52);
    player->fScale = INT32_C(0x2922);
    temp.vx =
        (int16_t)(int32_t)(
            (float)pos->vx + phy->mov.vx);
    temp.vy =
        (int16_t)(int32_t)(
            (float)pos->vy + phy->mov.vy);
    temp.vz =
        (int16_t)(int32_t)(
            (float)pos->vz + phy->mov.vz);
    temp.pad = 0;
    sprite_count =
        paEffects[51] == NULL
            ? 0
            : (int)paEffects[51]->num;
    if (sprite_count > 8) {
        sprite_count = 8;
    }
    for (x = 0; x < sprite_count; ++x) {
        if (sptr[x] != NULL) {
            sprite_gSetSpritePosition(
                sptr[x],
                (int)temp.vx,
                (int)temp.vy,
                (int)temp.vz);
            sptr[x]->sp_Time = 0;
        }
    }
    return 0;
}

/* 0xA2100, 591 bytes, global, 5 named locals
 * force_StarCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_StarCallBack(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene;
    physicsObject *physics;
    Sprite **sprites;
    int bank;

    (void)cpad;
    if (player == NULL ||
        player->playerRoot.pParent == NULL) {
        return 1;
    }
    scene =
        (sceneObject *)player->playerRoot.pParent;
    physics =
        (physicsObject *)scene->pPhysics;
    if (physics == NULL) {
        return 1;
    }

    (void)coll_GetNodeCenter(
        player->playerRoot.objectID, 0);
    if (player->forceData[0] == 0) {
        bank = player->playernum + 1;
        if (bank > 3) {
            bank = 3;
        }
        (void)sound_Play(
            &physics->vpos,
            bank,
            "stmfrcht",
            0);
        player->forceData[0] = 1;
        player->forceData[1] = 600;
        sprites = NULL;
        if (paEffects[80] != NULL) {
            sprites =
                sprite_AddSpriteEffect(
                    paEffects[80]->aEffects,
                    (int)paEffects[80]->num,
                    &physics->vpos,
                    NULL);
        }
        jpb_force_set_star_effect_timing(
            sprites);
        (void)game_gModItemCount(
            player->playernum, -1);
    }

    --player->forceData[1];
    if (player->forceData[1] < 0) {
        player->forceFlags &=
            UINT32_C(0xffffff9f);
        sprites = NULL;
        if (paEffects[80] != NULL) {
            sprites =
                sprite_AddSpriteEffect(
                    paEffects[80]->aEffects,
                    (int)paEffects[80]->num,
                    &physics->vpos,
                    NULL);
        }
        jpb_force_set_star_effect_timing(
            sprites);
        return 1;
    }

    player->forceFlags |= UINT32_C(0x60);
    fx_GlowingMan(
        &physics->physicsRoot,
        48,
        54,
        UINT32_C(0x00302010),
        UINT32_C(0xc0482814));
    return 0;
}

/* 0xA2350, 524 bytes, global, 11 named locals
 * force_TossCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_TossCallBack(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    int index;
    int throwIndex = 4;

    (void)cpad;
    (void)game_gGetForce(player->playernum);
    index = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);
    if (game_gGetEnergy(player->playernum) < 1) {
        int bank = player->playernum + 1;

        if (bank > 3) {
            bank = 3;
        }
        (void)sound_Play(
            &physics->vpos, bank, (char *)"none", 0);
        return 1;
    }

    if (player->playerID == 0x4f ||
        player->playerID == 0x35) {
        throwIndex = 0x19;
    } else if (player->playerID == 0x1e) {
        throwIndex = 0;
    }
    if (index > throwIndex) {
        int bombtype = 6;
        Projectile *proj;

        if (player->playerID == 1 ||
            player->playerID > 7) {
            bombtype = 0x18;
        }
        proj = bullet_AllocProjectile(bombtype);
        if (proj != NULL) {
            VECTOR *pos0 = coll_GetNodeCenter(
                player->playernum, 0x0f);
            _svector v = {0, 0x200, 0x200, 0};
            _svector dest;
            MATRIX m;
            VECTOR pos1;

            (void)fRotMatrix(
                &maPhysicsData[
                    player->playerRoot.objectID]
                     .svangle,
                &m);
            PushMatrix();
            (void)fApplyMatrixSV(&m, &v, &dest);
            PopMatrix();
            pos1.vx = pos0->vx + dest.vx;
            pos1.vy = pos0->vy + dest.vy;
            pos1.vz = pos0->vz + dest.vz;
            pos1.pad = 0;
            (void)game_gModItemCount(
                player->playernum, -1);
            bullet_ShootProjectile(
                proj, player, pos0, &pos1, NULL);
            if (player->playerID == 0x1e) {
                (void)anim_ForceNextAnimSeq(storeAnim, 0);
            }
        }
        return 1;
    }
    return 0;
}

/* 0xA2560, 381 bytes, global, 10 named locals
 * force_TossGrenadeCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_TossGrenadeCallBack(
    int32_t *cpad, playerObject *player)
{
    int index = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);

    (void)cpad;
    if (index >= 5) {
        int bombtype;
        Projectile *proj;

        if (player->playerID == 0x38 ||
            player->playerID == 0x22) {
            bombtype = 0x17;
        } else {
            bombtype = player->playerID == 1
                ? 0x18
                : 6;
        }
        proj = bullet_AllocProjectile(bombtype);
        if (proj != NULL) {
            VECTOR *pos0 = coll_GetNodeCenter(
                player->playernum, 0x0f);
            _svector v = {0, 0x200, 0x200, 0};
            _svector dest;
            MATRIX m;
            VECTOR pos1;

            (void)fRotMatrix(
                &maPhysicsData[
                    player->playerRoot.objectID]
                     .svangle,
                &m);
            PushMatrix();
            (void)fApplyMatrixSV(&m, &v, &dest);
            PopMatrix();
            pos1.vx = pos0->vx + dest.vx;
            pos1.vy = pos0->vy + dest.vy;
            pos1.vz = pos0->vz + dest.vz;
            pos1.pad = 0;
            bullet_ShootProjectile(
                proj, player, pos0, &pos1, NULL);
        }
        return 1;
    }
    return 0;
}

/* 0xA26E0, 1078 bytes, global, 26 named locals
 * force_ZapCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_ZapCallBack(
    int32_t *cpad, playerObject *player)
{
    int force = game_gGetForce(player->playernum);
    int index = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);
    VECTOR *pos0;
    VECTOR *pos1;
    FVECTOR direction;
    FVECTOR start;
    FVECTOR hitpoint;
    float ray_length;
    int *cube = NULL;
    int *entry = NULL;
    int *poly = NULL;
    int len = 0;
    int hit;
    _svector svstart;
    _svector svhitpoint;
    VECTOR eff;
    uint32_t glow_color;
    uint32_t color1;
    uint32_t color2;
    uint32_t color3;
    int glow_width;
    int starti;
    int x;

    (void)cpad;
    if (force < 5) {
        return 1;
    }
    if (index == 0x13) {
        (void)game_gModForce(player->playernum, -20);
    } else if ((uint32_t)(index - 0x13) > UINT32_C(0x1a)) {
        return 0;
    }

    pos0 = coll_GetNodeCenter(player->playernum, 0x0d);
    pos1 = coll_GetNodeCenter(player->playernum, 0x0f);
    direction.vx = (float)(pos1->vx - pos0->vx);
    direction.vy = (float)(pos1->vy - pos0->vy);
    direction.vz = (float)(pos1->vz - pos0->vz);
    start.vx = (float)pos1->vx;
    start.vy = (float)pos1->vy;
    start.vz = (float)pos1->vz;
    (void)VectorNormalize(&direction);
    ray_length = LineAndPlane(
        &clippingfrustrum[4],
        &start,
        &direction,
        768.0f,
        0x100);
    hit = RaycastCheck(
        &start,
        &direction,
        ray_length,
        &cube,
        &entry,
        &poly,
        &len,
        &hitpoint);
    SetCameraMatrix();

    svstart.vx = (int16_t)(int32_t)start.vx;
    svstart.vy = (int16_t)(int32_t)start.vy;
    svstart.vz = (int16_t)(int32_t)start.vz;
    svstart.pad = 0;
    svhitpoint.vx = (int16_t)(int32_t)hitpoint.vx;
    svhitpoint.vy = (int16_t)(int32_t)hitpoint.vy;
    svhitpoint.vz = (int16_t)(int32_t)hitpoint.vz;
    svhitpoint.pad = 0;
    if (player->playerID == 4) {
        glow_color = UINT32_C(0xc0804020);
        glow_width = 0x20;
        color1 = UINT32_C(0x007f7f7f);
        color2 = UINT32_C(0x005f4020);
        color3 = UINT32_C(0x00603018);
    } else {
        glow_color = UINT32_C(0xc0ff4000);
        glow_width = 0x18;
        color1 = UINT32_C(0x00ff4020);
        color2 = UINT32_C(0x00ff8040);
        color3 = UINT32_C(0x00ff8000);
    }
    fx_screenGlow(
        &svstart, &svhitpoint, glow_width, glow_color);
    SetCameraMatrix();
    PlotZap(
        color1,
        color2,
        color3,
        &svstart,
        &svhitpoint,
        0x800,
        0x40);

    eff.vx = (int32_t)hitpoint.vx;
    eff.vy = (int32_t)hitpoint.vy;
    eff.vz = (int32_t)hitpoint.vz;
    eff.pad = 0;
    if (hit != 0 && paEffects[3] != NULL) {
        (void)sprite_AddSpriteEffect(
            paEffects[3]->aEffects,
            (int)paEffects[3]->num,
            &eff,
            NULL);
    }

    starti = 2;
    if (player->playernum > 1 ||
        GameStruct.versusModeFlag != 0) {
        starti = 0;
    }
    for (x = starti; x < JPB_PHYSICS_CAPACITY; ++x) {
        physicsObject *physics = &maPhysicsData[x];
        VECTOR *body;
        _svector strt;
        _svector fvp;
        _svector hitp;
        int distance;

        if (physics->physicsRoot.objectID == -1 ||
            obj_gCheckObjectFlag(
                &physics->physicsRoot, 0, 0x20) != 0 ||
            physics->physicsRoot.objectID == player->playernum) {
            continue;
        }
        body = coll_GetNodeCenter(
            physics->physicsRoot.objectID, 0);
        if (body == NULL) {
            continue;
        }
        strt.vx = (int16_t)(int32_t)start.vx;
        strt.vy = (int16_t)(int32_t)start.vy;
        strt.vz = (int16_t)(int32_t)start.vz;
        strt.pad = 0;
        fvp.vx = (int16_t)(int32_t)hitpoint.vx;
        fvp.vy = (int16_t)(int32_t)hitpoint.vy;
        fvp.vz = (int16_t)(int32_t)hitpoint.vz;
        fvp.pad = 0;
        hitp.vx = (int16_t)body->vx;
        hitp.vy = (int16_t)body->vy;
        hitp.vz = (int16_t)body->vz;
        hitp.pad = 0;
        distance = vecpointlinesquared(
            &strt, &fvp, &hitp, NULL);
        if (distance >= 0 &&
            distance <=
                (int)physics->height *
                    (int)physics->height * 2) {
            sceneObject *target_scene =
                (sceneObject *)physics
                    ->physicsRoot.pParent;
            playerObject *target =
                (playerObject *)target_scene->pPlayer;

            if (paEffects[9] != NULL) {
                (void)sprite_AddSpriteEffect(
                    paEffects[9]->aEffects,
                    (int)paEffects[9]->num,
                    body,
                    NULL);
            }
            target->whohitme = player;
            target->hitNumber = 1;
            target->projectile =
                &((ProjType *)(void *)maProjTypes)[5];
        }
    }
    return 0;
}

/* 0xA2B20, 390 bytes, global, 5 named locals
 * force_gActivate
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
