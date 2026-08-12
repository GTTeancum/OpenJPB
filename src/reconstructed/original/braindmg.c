/*
 * PARTIALLY REVIEWED RECONSTRUCTION.
 * PDB module: 0009
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\braindmg.obj
 * Primary source: W:\SWJediPowerBattles\Work\braindmg.c
 * Compiler language: c
 * Emitted procedures: 12
 *
 * Focused assembly review:
 *   - braindmg_Blocking 0x1E5A4..0x1E5F4 distinguishes the player's locked
 *     target at playerObject+0x18 from the hit-source parameter.
 *   - braindmg_DeathReaction 0x1FBC8..0x1FC1B forces effect 18 for player ID
 *     35 before consulting a motion-owned fx1 value for other actors.
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/braindmg.h"

#include "jpb/anim.h"
#include "jpb/animctrl.h"
#include "jpb/animutil.h"
#include "jpb/ai.h"
#include "jpb/achievement.h"
#include "jpb/brain.h"
#include "jpb/brainutl.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/combo.h"
#include "jpb/enemy.h"
#include "jpb/extracharacters.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/jonny.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/sound.h"
#include "jpb/world.h"

#include <stdlib.h>

/* Exact PDB global at matched-PC RVA 0x4F1430. */
DamageTracker damageTracking[2];
/* Exact file-local PDB globals at matched-PC RVAs 0x4F13B4/0x4F13B8. */
static int mCurrentSFXP;
static int mProjectileAttack;
/* Exact PDB globals at matched-PC RVAs 0x4F13B0/0x4F13C0. */
static uint8_t mDamageTotal;
static Motion mProjMotion;
/* Exact file-local PDB global at matched-PC RVA 0x4F1520. */
static Motion *mpMotion;
/* Exact file-local PDB globals at matched-PC RVAs 0x4F1528/0x4F152C. */
static int knob;
static int zeroBSSCheck;

static void braindmg_detach_node(
    playerObject *player,
    Mnode *node,
    int vertical_velocity)
{
    node->flags |= UINT32_C(0x04000000);
    node->v3Translation2.vx =
        (int16_t)node->v3RotCenter.vx;
    node->v3Translation2.vy =
        (int16_t)node->v3RotCenter.vy;
    node->v3Translation2.vz =
        (int16_t)node->v3RotCenter.vz;
    node->v3Velocity2.vx = player->hitVelocity.vx;
    node->v3Velocity2.vy = (int16_t)vertical_velocity;
    node->v3Velocity2.vz = player->hitVelocity.vz;
    node->v3Velocity2.vx =
        (int16_t)(rand() % 4 - 8);
    node->v3Velocity2.vz =
        (int16_t)(rand() % 4 - 8);
    node->time = 0;
}

static int jpb_braindmg_fixed_div4096(int value)
{
    return value / 4096;
}

static VECTOR *jpb_braindmg_score_position(
    const playerObject *player)
{
    VECTOR *position =
        coll_GetNodeCenter(player->playernum, 8);

    if (position == NULL) {
        position =
            coll_GetNodeCenter(player->playernum, 0);
    }
    return position;
}

static void jpb_braindmg_award_points(
    playerObject *player,
    playerObject *attacker,
    int points,
    const _svector velocity[2],
    int small)
{
    VECTOR *position =
        jpb_braindmg_score_position(player);
    uint32_t color =
        jedi_GetColour32((uint64_t)attacker->playerID);

    if ((player->hitMask & UINT32_C(3)) ==
        UINT32_C(3)) {
        int other_player = attacker->playernum ^ 1;
        uint32_t color2 = jedi_GetColour32(
            (uint64_t)gaPlayerData[other_player].playerID);

        points /= 2;
        (void)game_gModScore(
            attacker->playernum, points);
        (void)game_gModScore(other_player, points);
        (void)sprite_GetPointsSprite(
            points,
            position,
            (_svector *)&velocity[0],
            color,
            small);
        (void)sprite_GetPointsSprite(
            points,
            position,
            (_svector *)&velocity[1],
            color2,
            small);
    } else {
        _svector centered_velocity = velocity[0];

        centered_velocity.vx = 0;
        centered_velocity.vz = 0;
        (void)game_gModScore(
            attacker->playernum, points);
        (void)sprite_GetPointsSprite(
            points,
            position,
            &centered_velocity,
            color,
            small);
    }
}

/* 0x1E450, 239 bytes, global, 4 named locals
 * braindmg_AirHitReaction
 * PDB type: int (playerObject*, playerObject...
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
int braindmg_AirHitReaction(
    playerObject *player, playerObject *target)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    int move;
    Motion *motion;

    (void)target;
    if ((physics->flags & UINT32_C(0x20)) != 0) {
        return 0;
    }
    move = mDrawingSurfaceId + 49;
    if (move == player->currentMotion) {
        animutl_gRestartAnim(&player->playerRoot);
    }
    motion = &player->paMotions[move];
    if (animctrl_MotionNoLock(
            &player->playerRoot, motion) == 0) {
        return 0;
    }

    brain_SetTrajectory(
        player, motion->Recoil, motion->RecoilAcc);
    physics_gSnapShotPosition(
        &player->playerRoot, 0);
    physics->airTime = 0;
    physics->realAirTime = 0;
    player->pMotionCallBack =
        jpb_TrajectoryCallbackSlot;
    player->fStun = 0;
    player->hitNumber = 0;
    player->hitDelay =
        gGlobalTimer + UINT32_C(0x0c00);
    player->hitMask = 0;
    player->groundDelay =
        gGlobalTimer + UINT32_C(0x1e00);
    player->delayedMotion = 0;
    return 1;
}

/* 0x1E540, 71 bytes, global, 1 named locals
 * braindmg_BlockEffects
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
void braindmg_BlockEffects(playerObject *player)
{
    EffectHeader *effect = paEffects[4 + rand() % 2];

    (void)sprite_AddSpriteEffectAtNode(
        effect->aEffects,
        (int)effect->num,
        player->playernum,
        11);
}

/* 0x1E590, 779 bytes, global, 9 named locals
 * braindmg_Blocking
 * PDB type: int (playerObject*, playerObject...
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
int braindmg_Blocking(
    playerObject *player,
    playerObject *target,
    int mDamageTotal)
{
    int move;
    int forceit = 0;
    int blocked;

    if ((player->playerID == 43 ||
         player->playerID == 9) &&
        ((player->target->playerID == 2 &&
          player->target->currentMotion == 43 &&
          player->fStun > 90) ||
         ((player->target->pFlags &
           UINT32_C(0x40)) != 0))) {
        player->fStun = 0;
        forceit = 1;
    } else if (
        player->fStun >= 181 &&
        player->playerID <= 43 &&
        ((UINT64_C(0x80408080200) >>
          ((uint16_t)player->playerID & 63)) &
         UINT64_C(1)) != 0) {
        player->fStun /= 16;
        forceit = 1;
    } else {
        if ((player->pFlags & UINT32_C(0x0c11)) != 0 ||
            (target->pFlags & UINT32_C(0x0800)) != 0 ||
            (((target->pFlags & UINT32_C(0x2000)) != 0) &&
             mProjectileAttack == 0) ||
            (player->pFlags & UINT32_C(0x8000)) != 0 ||
            (mProjectileAttack != 0 &&
             (projType->flag & UINT16_C(0x4000)) != 0) ||
            (player->pFlags & UINT32_C(0x20)) == 0) {
            return 0;
        }
        if (player->playernum < 2) {
            int blockmod = 0;

            if (player->playerID < 6) {
                blockmod =
                    (jediUpgrades[player->playerID]
                         .attackDefendUpgrades >>
                     4) *
                    5;
            }
            if ((float)(blockmod + 128) <
                damageTracking[player->playernum].total) {
                return 0;
            }
        } else if (brainutil_ReverseCheck(player) != 0) {
            return 0;
        }
    }

    move = rand() % 3 + 16;
    if (forceit == 0) {
        blocked = animctrl_MotionEqualLock(
            &player->playerRoot,
            &player->paMotions[move]);
    } else {
        blocked = animctrl_MotionNoLock(
            &player->playerRoot,
            &player->paMotions[move]);
    }

    if ((GameStruct.GameState & UINT32_C(0x04000000)) ==
            0 ||
        (secretBits & UINT32_C(0x100)) == 0 ||
        player->playerID < 6 ||
        (player->pFlags & UINT32_C(0x2000)) != 0 ||
        player->playerID == 9 ||
        player->playerID == 43) {
        if (GameStruct.CurrentLevel == 10 &&
            player->playerID == 43 &&
            IsExtraCharacter(target->playerID) != 0) {
            int chance =
                target->playerID == 30
                ? rand() % 2
                : rand() % 3;

            if (chance == 0) {
                return 0;
            }
        }
        if (blocked != 0) {
            uint8_t hit_number = player->hitNumber;

            (void)physics_gGetPosition(
                &player->playerRoot);
            (void)animctrl_MotionChain(
                &player->playerRoot,
                &player->paMotions[21]);
            player->fStun +=
                mDamageTotal * 8 * (int)hit_number;
            player->hitDelay =
                gGlobalTimer +
                ((uint32_t)(mpMotion->Delay + 2) >> 1) *
                    UINT32_C(0x200);
            player->hitNumber = 0;
            player->fForce +=
                (mDamageTotal / 2) * (int)hit_number;
            feedback_startEffect(player->playernum, 7);
            return 1;
        }
    }
    return 0;
}

/* 0x1E8A0, 4255 bytes, global, 37 named locals
 * braindmg_DamageControl
 * PDB type: int (void*)
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
int braindmg_DamageControl(void *player_data)
{
    static const _svector hit_velocity[2] = {
        {-4, -6, -4, 0},
        {4, -6, 4, 0}
    };
    static const int combo_points[8] = {
        0, 0, 10, 25, 50, 100, 150, 200
    };
    static const _svector death_velocity[2] = {
        {-4, 6, -4, 0},
        {4, 6, 4, 0}
    };
    playerObject *player =
        (playerObject *)player_data;
    playerObject *attacker;
    sceneObject *scene;
    physicsObject *physics;
    int energy =
        game_gGetEnergy(player->playernum);
    int damage;
    int delta;
    int mod;
    int feedback_effect = 2;
    uint8_t damagemod = 0;

    if ((gpWorld->aDolly[gpWorld->currentDolly].flags &
         UINT32_C(0x400)) != 0) {
        return 0x400;
    }

    projType = player->projectile;
    attacker =
        player->whohitme != NULL
        ? player->whohitme
        : player;
    if (projType == NULL) {
        mpMotion = player->hitMotion;
        mProjectileAttack = 0;
        projType = NULL;
        if (mpMotion == NULL) {
            mProjMotion.Damage = 1;
            mProjMotion.Delay = 0x10;
            mProjMotion.hitReact = 0x21;
            mProjMotion.fx1 = 0;
            mpMotion = &mProjMotion;
        }
    } else {
        int friendly_fire =
            GameStruct.NumPlayers == 2 &&
            GameStruct.versusModeFlag == 0 &&
            ((player == gpWorld->player0 &&
              attacker == gpWorld->player1) ||
             (player == gpWorld->player1 &&
              attacker == gpWorld->player0));

        if ((player->pFlags & UINT32_C(0x800)) != 0 ||
            projType->damage == 0 ||
            friendly_fire) {
            player->hitNumber = 0;
            player->projectile = NULL;
            return 1;
        }
        mProjMotion.hitReact = projType->hitReact;
        mProjMotion.Damage = projType->damage;
        mProjMotion.Delay = 0x10;
        mProjMotion.fx1 = projType->hitEffect;
        mProjectileAttack = 1;
        player->projectile = NULL;
        mpMotion = &mProjMotion;
    }

    player->fLife = 0;
    if (energy == 0 &&
        (player->pFlags & UINT32_C(0x44000201)) == 0) {
        player->hitNumber = 1;
    }
    if (player->hitNumber != 0 &&
        (mpMotion->Damage != 0 ||
         (attacker->pFlags & UINT32_C(0x40)) != 0)) {
        if ((player->pFlags & UINT32_C(0x44000600)) ==
                0 &&
            energy != 255) {
            (void)coll_GetNodeCenter(
                player->playernum, 0);
            mDamageTotal = 0;
            if (attacker->playerID < 6 &&
                mProjectileAttack == 0) {
                damagemod = (uint8_t)(
                    (jediUpgrades[attacker->playerID]
                         .attackDefendUpgrades &
                     0x0f) *
                    2);
            }
            if (gGlobalTimer < player->hitDelay) {
                player->hitNumber = 0;
                return 1;
            }

            player->PreMotion[0] = '\0';
            if ((player->pFlags &
                 UINT32_C(0x02000000)) != 0) {
                player->pFlags &=
                    UINT32_C(0xfcdfffff);
            }
            mDamageTotal = mpMotion->Damage;
            if ((attacker->pFlags &
                 UINT32_C(0x40)) != 0) {
                if (mProjectileAttack == 0) {
                    if (attacker->playerID == 2 ||
                        attacker->playerID == 3) {
                        mDamageTotal = 0;
                        if (attacker->playerID == 2) {
                            (void)game_gModEnergy(
                                attacker->playernum, 2);
                        }
                    } else if (attacker->playerID < 6) {
                        mDamageTotal = 2;
                    }
                } else if (
                    (projType->flag &
                     UINT16_C(0x40)) == 0 &&
                    player->playerRoot.objectID > 1) {
                    mDamageTotal = (uint8_t)(
                        mDamageTotal +
                        (mDamageTotal >> 1));
                }

                if (mProjectileAttack == 0 ||
                    (projType->flag &
                     UINT16_C(0x40)) != 0) {
                    if (player->playerRoot.objectID > 1 &&
                        attacker->playerRoot.objectID > 1 &&
                        attacker->playerID != 35 &&
                        player->playerID != 35 &&
                        GameStruct.CurrentLevel != 13) {
                        mDamageTotal = 0;
                    }
                }
            }

            mCurrentSFXP = mpMotion->fx1;
            braindmg_DamageTracker(
                player,
                mProjectileAttack != 0
                    ? mDamageTotal >> 1
                    : mDamageTotal);
            if (braindmg_Blocking(
                    player,
                    attacker,
                    mDamageTotal) != 0) {
                braindmg_BlockEffects(player);
                return 0;
            }

            if ((player->pFlags & UINT32_C(0x20)) == 0) {
                braindmg_ResetDamageTracker(
                    player->playernum);
            }
            if (player->playerID == 35) {
                mDamageTotal >>= 1;
            }

            if (player->playernum < 2) {
                damage =
                    (int)(int8_t)GameStruct.AIDamage *
                    0x200 *
                    (int)player->hitNumber *
                    (int)mDamageTotal;
                damage =
                    jpb_braindmg_fixed_div4096(damage);
                if (GameStruct.NumPlayers == 1) {
                    damage =
                        jpb_braindmg_fixed_div4096(
                            damage * 0x0c00);
                }
                player->fLife -= damage;
            } else {
                int power_type =
                    game_gGetPowerType(
                        attacker->playernum);

                if (power_type == 9 ||
                    power_type == 10) {
                    if (mProjectileAttack == 0 &&
                        gGlobalTimer <
                            (uint32_t)game_gGetPowerLevel(
                                attacker->playernum)) {
                        mDamageTotal <<= 1;
                    }
                    damage =
                        (int)player->hitNumber *
                        (int)mDamageTotal;
                } else {
                    if (mProjectileAttack == 0) {
                        mDamageTotal = mpMotion->Damage;
                        if (attacker->playerID == 3 &&
                            (attacker->pFlags &
                             UINT32_C(0x52)) ==
                                UINT32_C(0x52)) {
                            mDamageTotal = 5;
                        }
                    }
                    damage =
                        (int)(int8_t)GameStruct.JediDamage *
                        0x200 *
                        (int)player->hitNumber *
                        (int)mDamageTotal;
                    damage =
                        jpb_braindmg_fixed_div4096(damage);
                    if ((GameStruct.GameState &
                         UINT32_C(0x04000000)) != 0 &&
                        (secretBits &
                         UINT32_C(0x100)) != 0 &&
                        (player->pFlags &
                         UINT32_C(0x2000)) == 0 &&
                        player->playerID != 9 &&
                        player->playerID != 43 &&
                        damage < 75) {
                        damage = 75;
                    }
                }
                if (GameStruct.NumPlayers == 1) {
                    damage =
                        jpb_braindmg_fixed_div4096(
                            damage * 0x1400);
                }
                player->fLife -= damage;
                player->fLife -= damagemod;

                if (attacker->playernum < 2 &&
                    mProjectileAttack == 0) {
                    int points;
                    Motion *attacker_motion =
                        *attacker->pMotion;
                    int hit_count =
                        attacker->paCombos[
                            attacker_motion->combo]
                            .numHits;

                    points = combo_points[hit_count];
                    if (player->playerID < 85 &&
                        points != 0) {
                        jpb_braindmg_award_points(
                            player,
                            attacker,
                            points,
                            hit_velocity,
                            1);
                    }
                }
            }

            player->fStun +=
                (int)player->hitNumber *
                (int)mDamageTotal *
                16;
            player->hitDelay =
                gGlobalTimer +
                (uint32_t)mpMotion->Delay *
                    UINT32_C(0x200);
            if ((attacker->pFlags & UINT32_C(0x40)) != 0 &&
                attacker->playerID == 3) {
                player->hitDelay =
                    gGlobalTimer + UINT32_C(0x1400);
            }
            if (player->fStun > 0x200) {
                player->fStun = 0x200;
            }

            delta = player->fLife;
            if ((player->forceFlags & UINT32_C(1)) != 0) {
                player->hitDelay = 0;
                delta *= -2;
                player->fLife = delta;
            }
            if ((player->forceFlags &
                 UINT32_C(0x10)) != 0) {
                player->fLife = 0;
                delta = 0;
                player->hitDelay = 0;
            }
            (void)game_gModEnergy(
                player->playernum, delta);

            if (player->playerID == 9) {
                _svector velocity = {0, 6, 0, 0};
                VECTOR *position = coll_GetNodeCenter(
                    player->playernum, 8);
                uint32_t color = jedi_GetColour32(
                    (uint64_t)attacker->playerID);
                int points =
                    (int)player->hitNumber *
                    (int)mDamageTotal *
                    20;

                (void)game_gModScore(
                    attacker->playernum, points);
                (void)sprite_GetPointsSprite(
                    points,
                    position,
                    &velocity,
                    color,
                    1);
            } else if (
                (playertankindex == 0 ||
                 attacker->playerRoot.objectID > 1 ||
                 player->playerRoot.objectID !=
                     playertankindex - 1) &&
                player->playerID != 45) {
                (void)game_gModScore(
                    attacker->playernum,
                    (int)player->hitNumber *
                        (int)mDamageTotal);
            }

            player->target->fStun = 0;
            if ((player->pFlags & UINT32_C(0x800)) == 0 &&
                (attacker->pFlags & UINT32_C(0x800)) == 0) {
                (void)braindmg_LogHits(
                    player, attacker);
                energy =
                    game_gGetEnergy(player->playernum);
                if (energy < 1) {
                    if (attacker->playernum == 0 &&
                        mProjectileAttack != 0 &&
                        (projType->flag &
                         UINT16_C(0x40)) != 0) {
                        achievement_complete(37);
                    }
                    if ((player->pFlags &
                         UINT32_C(0x0c01)) == 0 &&
                        ((*player->pMotion)->motionFlags &
                         UINT32_C(0x40000000)) == 0) {
                        int attacker_number =
                            attacker->playernum;

                        if (attacker_number == 0) {
                            if (mProjectileAttack != 0 &&
                                (projType->flag &
                                 UINT16_C(0x0440)) ==
                                    UINT16_C(0x0400)) {
                                int count =
                                    achievement_getcount(2) +
                                    1;

                                achievement_update(2, count);
                                if (achievement_getcount(2) >
                                    3) {
                                    achievement_complete(2);
                                }
                            }
                            if (player->playerID == 12) {
                                achievement_complete(4);
                            }
                            if (player->playerID == 26) {
                                achievement_complete(33);
                            }
                            if (player->playerID == 35 &&
                                (attacker->pFlags &
                                 UINT32_C(0x80)) != 0) {
                                achievement_complete(31);
                            }
                        }
                        (void)braindmg_DeathReaction(
                            player, attacker);
                        if (attacker_number < 2 &&
                            player->playerID < 85) {
                            int points =
                                gaPoints[player->playerID];

                            if (points != 0) {
                                jpb_braindmg_award_points(
                                    player,
                                    attacker,
                                    points,
                                    death_velocity,
                                    0);
                            }
                        }
                        goto damage_effects;
                    }
                }

                if ((player->pFlags & UINT32_C(0x400)) ==
                        0 &&
                    (player->forceFlags &
                     UINT32_C(0x30)) == 0) {
                    Motion *current_motion =
                        *player->pMotion;

                    if ((player->pFlags &
                         UINT32_C(1)) == 0 &&
                        (current_motion->motionFlags &
                         UINT32_C(0x40000000)) == 0) {
                        (void)braindmg_HitReaction(
                            player, attacker, 0);
                    } else if (
                        player->playerID != 9 &&
                        player->playerID != 43 &&
                        ((((physicsObject *)
                            ((sceneObject *)
                                 player->playerRoot.pParent)
                                ->pPhysics)
                              ->flags &
                          UINT32_C(0x20)) == 0)) {
                        Motion *motion;
                        int move =
                            mDrawingSurfaceId + 49;

                        if (move ==
                            player->currentMotion) {
                            animutl_gRestartAnim(
                                &player->playerRoot);
                        }
                        motion =
                            &player->paMotions[move];
                        if (animctrl_MotionNoLock(
                                &player->playerRoot,
                                motion) != 0) {
                            physicsObject *air_physics =
                                (physicsObject *)
                                    ((sceneObject *)
                                         player->playerRoot
                                             .pParent)
                                        ->pPhysics;

                            brain_SetTrajectory(
                                player,
                                motion->Recoil,
                                motion->RecoilAcc);
                            physics_gSnapShotPosition(
                                &player->playerRoot, 0);
                            air_physics->airTime = 0;
                            air_physics->realAirTime = 0;
                            player->pMotionCallBack =
                                jpb_TrajectoryCallbackSlot;
                            player->fStun = 0;
                            player->hitNumber = 0;
                            player->hitDelay =
                                gGlobalTimer +
                                UINT32_C(0x0c00);
                            player->hitMask = 0;
                            player->groundDelay =
                                gGlobalTimer +
                                UINT32_C(0x1e00);
                            player->delayedMotion = 0;
                        }
                    }
                }
            }

damage_effects:
            (void)braindmg_DamageEffects(player);
            if (player->playernum < 2) {
                if (projType == NULL ||
                    projType->hitEffect != 17) {
                    if (mDamageTotal < 6) {
                        feedback_effect = 1;
                    } else {
                        if (mDamageTotal > 10) {
                            feedback_effect = 3;
                            if (mDamageTotal > 15) {
                                feedback_effect = 12;
                            }
                        }
                    }
                    feedback_startEffect(
                        player->playernum,
                        feedback_effect);
                } else {
                    feedback_startEffect(
                        player->playernum, 10);
                }
            } else {
                if (attacker->projectile == NULL) {
                    feedback_startEffect(
                        attacker->playernum, 5);
                }
            }
            player->hitNumber = 0;
            return 1;
        }
    }

    delta = player->fStun / 128;
    mod = 1;
    if (delta > 0) {
        mod = delta > 32 ? 32 : delta;
    }
    player->fStun -= mod;
    if (player->fStun < 0) {
        player->fStun = 0;
    }
    player->hitNumber = 0;

    scene =
        (sceneObject *)player->playerRoot.pParent;
    physics =
        (physicsObject *)scene->pPhysics;
    if ((player->pFlags & UINT32_C(0x44000601)) != 0 ||
        game_gGetEnergy(player->playernum) == 255) {
        goto damage_control_done;
    }

    if (physics->mapinfo.poly != NULL &&
        (leveldata[
             (uint32_t)*physics->mapinfo.poly &
             UINT32_C(0x1ffff)] &
         INT32_C(0x100000)) != 0) {
        if (LevelSelect == 1) {
            if (zeroBSSCheck == zerobss_levelReset) {
                if (knob != 0) {
                    knob -= gGlobalFrameRate;
                    if (knob < 0) {
                        knob = 0;
                    }
                    goto apply_level_damage;
                }
            } else {
                zeroBSSCheck = zerobss_levelReset;
                zerobss_levelReset = 0;
                knob = 0;
            }
            (void)sound_Play(
                &physics->vpos, 3, "lgspark", 0);
            {
                EffectHeader *effect = paEffects[25];

                (void)sprite_AddSpriteEffectAtNode(
                    effect->aEffects,
                    (int)effect->num,
                    player->playernum,
                    6);
            }
            knob = 0xa000;
        }
        if (LevelSelect == 5) {
            (void)game_gModEnergy(
                player->playernum, -25);
            if ((player->playerID < 10 ||
                 IsExtraCharacter(
                     player->playerID) != 0) &&
                player->playernum == 0) {
                achievement_complete(32);
            } else if (
                player->playerID > 9 &&
                player->playerID != 17) {
                achievement_complete(39);
            }
        }
        goto apply_level_damage;
    }
    if (physics->lastpolyhit == NULL ||
        !jpb_LevelDataContains(
            physics->lastpolyhit, sizeof(*physics->lastpolyhit)) ||
        (leveldata[
             (uint32_t)*physics->lastpolyhit &
             UINT32_C(0x1ffff)] &
         INT32_C(0x100000)) == 0) {
        goto damage_control_done;
    }

apply_level_damage:
    delta = mDrawingSurfaceId * -2;
    player->fLife = delta;
    if ((player->forceFlags & UINT32_C(1)) != 0) {
        player->hitDelay = 0;
        delta = mDrawingSurfaceId * 4;
        player->fLife = delta;
    }
    if ((player->forceFlags & UINT32_C(0x10)) != 0) {
        player->fLife = 0;
        delta = 0;
        player->hitDelay = 0;
    }
    (void)game_gModEnergy(
        player->playernum, delta);
    if (rand() % 100 < 10) {
        EffectHeader *effect = paEffects[45];

        (void)sprite_AddSpriteEffect(
            effect->aEffects,
            (int)effect->num,
            &physics->vpos,
            NULL);
    }
    if (game_gGetEnergy(player->playernum) < 1) {
        (void)braindmg_DeathReaction(
            player, attacker);
    }

damage_control_done:
    return gGlobalTimer < player->hitDelay;
}

/* 0x1F940, 102 bytes, global, 2 named locals
 * braindmg_DamageEffects
 * PDB type: int (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
int braindmg_DamageEffects(playerObject *player)
{
    EffectHeader *effect;
    VECTOR *location;

    if (mCurrentSFXP < 0) {
        return 0;
    }
    if (mCurrentSFXP >= gMaxEffect) {
        mCurrentSFXP = 2;
    }
    if (mCurrentSFXP == 81) {
        sceneObject *scene =
            (sceneObject *)player->playerRoot.pParent;
        physicsObject *physics =
            (physicsObject *)scene->pPhysics;

        effect = paEffects[81];
        location = &physics->vpos;
    } else {
        effect = paEffects[mCurrentSFXP];
        location = &player->hitLocation;
    }
    (void)sprite_AddSpriteEffect(
        effect->aEffects,
        (int)effect->num,
        location,
        NULL);
    return 0;
}

/* 0x1F9B0, 111 bytes, global, 3 named locals
 * braindmg_DamageTracker
 * PDB type: void (playerObject*, int)
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
void braindmg_DamageTracker(
    playerObject *player, int damage)
{
    int playernum = player->playerRoot.objectID;

    if (playernum < 2) {
        int scaled_damage;
        DamageTracker *tracker =
            &damageTracking[playernum];

        ++tracker->hits;
        tracker->timer = gGlobalTimer;
        tracker->current = (int16_t)damage;
        scaled_damage =
            player->playerID == 9 ||
                    player->playerID == 43
                ? damage * 4
                : damage * 3;
        tracker->total += (float)scaled_damage;
        if (tracker->total > 192.0f) {
            tracker->total = 192.0f;
        }
    }
}

/* 0x1FA20, 1577 bytes, global, 12 named locals
 * braindmg_DeathReaction
 * PDB type: int (playerObject*, playerObject...
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
int braindmg_DeathReaction(
    playerObject *player, playerObject *target)
{
    Motion *motion = mpMotion;
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    int move;
    int fx1;

    (void)target;
    if (player->playerID == 10 ||
        player->playerID == 11 ||
        player->playerID == 58) {
        (void)game_gSetGameFlags(UINT32_C(0x20));
        (void)game_gSetGameFlags(UINT32_C(0x40));
        abGlobalBits[3] |= UINT8_C(1);
    }
    if (player->playerID == 59 &&
        LevelSelect == 9) {
        sceneObject *target_scene =
            (sceneObject *)
                player->target->playerRoot.pParent;
        physicsObject *target_physics =
            (physicsObject *)target_scene->pPhysics;

        ++pilotsKilled;
        enemy_SetTeleportReturn(&target_physics->vpos);
        abGlobalBits[0] &= UINT8_C(0xfe);
    }
    if (player->pEnemy != NULL &&
        player->pEnemy->pPlace != NULL &&
        player->pEnemy->pPlace->status == 2) {
        ++gDeathCount;
        if (player->pEnemy->aiNum == 30) {
            ++gPilotDeathCount;
        }
    }

    animutl_FlushSeqQueue(&player->playerRoot);
    if (motion->hitReact != 0 &&
        (player->pFlags & UINT32_C(0x8000)) == 0) {
        Motion *reaction =
            &player->paMotions[motion->hitReact];

        if ((reaction->motionFlags &
             UINT32_C(0x40000000)) != 0) {
            (void)animctrl_MotionNoLock(
                &player->playerRoot, reaction);
            brain_SetTrajectory(
                player,
                reaction->Recoil,
                reaction->RecoilAcc);
            physics_gSnapShotPosition(
                &player->playerRoot, 0);
            physics->airTime = 0;
            physics->realAirTime = 0;
            player->paMotions[51].FunctPtr = 6;
            (void)animctrl_MotionChain(
                &player->playerRoot,
                &player->paMotions[51]);
            return 1;
        }
    }

    if (player->paiMemory == NULL) {
        move = 23 + player->subOffset;
    } else {
        move = ai_Death(player, 0);
        if (move > 7) {
            move += player->subOffset;
        }
    }
    motion = &player->paMotions[move];
    if (animctrl_MotionNoLock(
            &player->playerRoot, motion) == 0) {
        return 1;
    }

    fx1 = player->playerID == 35
        ? 18
        : motion->fx1;
    if (player->playerID == 35 ||
        (unsigned int)(fx1 - 10) < 2u ||
        (unsigned int)(fx1 - 17) < 2u) {
        EffectHeader *effect = paEffects[fx1];

        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            0);
        (void)sound_Play(
            &physics->vpos,
            0,
            "explomed",
            0);
        if (player->pEnemy != NULL) {
            player->pEnemy->exit_flag = 1;
        }
    }

    if (rand() % 100 < 30 &&
        player->playerID == 17) {
        Mnode *node = coll_GetNode(
            player->playerRoot.objectID, 7);
        EffectHeader *effect = paEffects[2];

        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            0);
        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            7);
        braindmg_detach_node(player, node, 32);
    } else if (
        rand() % 100 < 30 &&
        (player->playerID == 15 ||
         player->playerID == 17) &&
        player->playernum > 1) {
        Mnode *node = coll_GetNode(
            player->playerRoot.objectID, 8);
        EffectHeader *effect = paEffects[2];

        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            16);
        braindmg_detach_node(player, node, 32);
    } else if (
        rand() % 100 < 30 &&
        (player->playerID == 15 ||
         player->playerID == 17 ||
         (player->playerID == 26 &&
          player->playernum > 1))) {
        Mnode *node = coll_GetNode(
            player->playerRoot.objectID, 13);
        EffectHeader *effect = paEffects[2];

        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            13);
        braindmg_detach_node(player, node, 24);
    } else if (
        player->playerID == 30 &&
        player->playernum < 2) {
        Mnode *node = coll_GetNode(
            player->playerRoot.objectID, 2);

        node->flags |= UINT32_C(0x04000000);
        node = coll_GetNode(
            player->playerRoot.objectID, 5);
        node->flags |= UINT32_C(0x04000000);
    }

    player->groundDelay =
        gGlobalTimer + UINT32_C(0x2800);
    motion->motionFlags |= UINT32_C(0x04000000);
    player->pFlags |= UINT32_C(0x400);
    return 1;
}

/* 0x20050, 161 bytes, global, 4 named locals
 * braindmg_FindHitReaction
 * PDB type: int (playerObject*, playerObject...
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
int braindmg_FindHitReaction(
    playerObject *player,
    playerObject *target,
    int DEATH)
{
    int current;
    Motion *motion = mpMotion;

    (void)target;
    (void)DEATH;
    if (brainutil_ReverseCheck(player) &&
        player->playerID != 9 &&
        player->playerID != 43) {
        current = player->currentMotion;
        if (current == 13 ||
            player->paMotions[current].vel == 0x800) {
            return -42;
        }
        return current == 37 ? -38 : -37;
    }
    if (motion->hitReact != 0) {
        return -(int)motion->hitReact;
    }
    return -33;
}

/* 0x20100, 563 bytes, global, 6 named locals
 * braindmg_HitReaction
 * PDB type: int (playerObject*, playerObject...
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
int braindmg_HitReaction(
    playerObject *player,
    playerObject *target,
    int DEATH)
{
    int reaction;
    int reverse = 0;
    Motion *motion;

    if ((player->pFlags & UINT32_C(0x8000)) != 0) {
        motion = &player->paMotions[6];
        if (animctrl_MotionLock(
                &player->playerRoot, motion) != 0) {
            player->currentMotion = 6;
            if (player->playerID != 40 &&
                player->playerID != 35) {
                physics_gSetRecoil(
                    player,
                    mpMotion->Recoil,
                    mpMotion->RecoilAcc,
                    0);
            }
        }
        return 1;
    }

    reaction =
        braindmg_FindHitReaction(player, target, DEATH);
    if (reaction >= 0) {
        return 1;
    }
    reaction = reaction < 0 ? -reaction : reaction;

    if (mProjectileAttack == 0) {
        reverse = brainutil_ReverseCheck(player) != 0;
    } else if (mpMotion->hitReact == 1) {
        EffectHeader *effect = paEffects[20];

        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            0);
        return 1;
    }

    motion = &player->paMotions[reaction];
    if (animctrl_MotionNoLock(
            &player->playerRoot, motion) != 0) {
        player->currentMotion = (int16_t)reaction;
        if (player->playerID != 40 &&
            player->playerID != 35) {
            physics_gSetRecoil(
                player,
                mpMotion->Recoil,
                mpMotion->RecoilAcc,
                reverse);
        }
    }
    if ((motion->motionFlags &
         UINT32_C(0x40000000)) != 0) {
        sceneObject *scene =
            (sceneObject *)player->playerRoot.pParent;
        physicsObject *physics =
            (physicsObject *)scene->pPhysics;

        brain_SetTrajectory(
            player, motion->Recoil, motion->RecoilAcc);
        physics_gSnapShotPosition(
            &player->playerRoot, 0);
        physics->airTime = 0;
        physics->realAirTime = 0;
        player->paMotions[51].FunctPtr = 6;
        (void)animctrl_MotionChain(
            &player->playerRoot,
            &player->paMotions[51]);
    }
    return 1;
}

/* 0x20340, 462 bytes, global, 7 named locals
 * braindmg_LevelDamage
 * PDB type: int (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
int braindmg_LevelDamage(playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    int32_t *poly;

    if ((player->pFlags &
         UINT32_C(0x44000601)) != 0 ||
        game_gGetEnergy(player->playernum) == 255) {
        return 0;
    }

    poly = physics->mapinfo.poly;
    if (poly != NULL &&
        (leveldata[(uint32_t)*poly &
                   UINT32_C(0x1ffff)] &
         INT32_C(0x100000)) != 0) {
        if (LevelSelect == 1) {
            if (zeroBSSCheck != zerobss_levelReset) {
                zeroBSSCheck = zerobss_levelReset;
                zerobss_levelReset = 0;
                knob = 0;
            } else if (knob != 0) {
                knob -= gGlobalFrameRate;
                if (knob < 0) {
                    knob = 0;
                }
                return 1;
            }

            (void)sound_Play(
                &physics->vpos,
                3,
                "lgspark",
                0);
            {
                EffectHeader *effect = paEffects[25];

                (void)sprite_AddSpriteEffectAtNode(
                    effect->aEffects,
                    (int)effect->num,
                    player->playernum,
                    6);
            }
            knob = 0xa000;
        }
        if (LevelSelect == 5) {
            (void)game_gModEnergy(
                player->playernum, -25);
            if ((player->playerID < 10 ||
                 IsExtraCharacter(
                     player->playerID) != 0) &&
                player->playernum == 0) {
                achievement_complete(32);
                return 1;
            }
            if (player->playerID > 9 &&
                player->playerID != 17) {
                achievement_complete(39);
                return 1;
            }
        }
        return 1;
    }

    poly = physics->lastpolyhit;
    if (poly == NULL ||
        !jpb_LevelDataContains(poly, sizeof(*poly))) {
        return 0;
    }
    return (leveldata[(uint32_t)*poly &
                      UINT32_C(0x1ffff)] &
            INT32_C(0x100000)) != 0;
}

/* 0x20510, 38 bytes, global, 2 named locals
 * braindmg_LogHits
 * PDB type: int (playerObject*, playerObject...
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
int braindmg_LogHits(
    playerObject *player, playerObject *attacker)
{
    uint32_t mask =
        UINT32_C(1) <<
        ((uint32_t)attacker->playernum & 31);

    player->hitMask |= mask;
    return (int)player->hitMask;
}

/* 0x20540, 31 bytes, global, 1 named locals
 * braindmg_ResetDamageTracker
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\braindmg.c
 */
void braindmg_ResetDamageTracker(int playernum)
{
    /*
     * The reference places damageTracking[2] before 216 bytes of zero BSS
     * padding, so calls for enemy object IDs only touch inert storage. A
     * portable linker may place another global there instead. Preserve the
     * reference's observable behavior without retaining that out-of-bounds
     * store.
     */
    if ((unsigned)playernum >=
        sizeof(damageTracking) / sizeof(damageTracking[0])) {
        return;
    }
    damageTracking[playernum].timer = gGlobalTimer;
    damageTracking[playernum].hits = 0;
    damageTracking[playernum].current = 0;
    damageTracking[playernum].total = 0.0f;
}
