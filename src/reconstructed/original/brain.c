/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\brain.c.
 *
 * Fourteen PDB procedures live in this translation unit. The fifteenth and
 * largest, exact brain_ControlPlayer, is split into sibling brain_control.c to
 * keep its private block helpers readable; together they cover all 15 emitted
 * procedures.
 *
 * Provenance:
 *   direct     - names/signatures/locals and player/physics layouts from PDB.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - stores, masks, fixed-point rounding, and callbacks checked at
 *                RVAs 0x1C840..0x1C922, 0x1CFF0..0x1D3AF,
 *                0x1D5FB..0x1D766, 0x1D7A2..0x1D7C4,
 *                0x1D8B0..0x1E44C.
 *
 * PDB module: 0008
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\brain.obj
 * Primary source: W:\SWJediPowerBattles\Work\brain.c
 * Compiler language: c
 * Emitted procedures: 15
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/brain.h"
#include "jpb/animctrl.h"
#include "jpb/animutil.h"
#include "jpb/brainutl.h"
#include "jpb/collision.h"
#include "jpb/fmath.h"
#include "jpb/game.h"
#include "jpb/jedi.h"
#include "jpb/jonny.h"
#include "jpb/model.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/world.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static JPBBrainLockDiagnostics jpb_brain_lock_diagnostics;

void jpb_BrainResetLockDiagnostics(void)
{
    memset(
        &jpb_brain_lock_diagnostics,
        0,
        sizeof(jpb_brain_lock_diagnostics));
}

void jpb_BrainGetLockDiagnostics(
    JPBBrainLockDiagnostics *diagnostics)
{
    if (diagnostics != NULL) {
        *diagnostics = jpb_brain_lock_diagnostics;
    }
}

static physicsObject *brain_player_physics(playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;

    return (physicsObject *)scene->pPhysics;
}

static int32_t brain_mul_fixed12(int32_t left, int32_t right)
{
    int32_t product =
        (int32_t)((uint32_t)left * (uint32_t)right);
    uint32_t adjusted =
        (uint32_t)product +
        ((uint32_t)(product >> 31) & 0x00000fffu);

    return (int32_t)adjusted >> JPB_FIXED_SHIFT;
}

/*
 * Exact direction conversion from the ordinary ground-control branch inside
 * brain_ControlPlayer (0x1D0D1..0x1D11F). The executable passes the analog
 * X/Y pair to atan2f and rotates the result into camera-relative 12-bit angle
 * space. This descriptive boundary is intentionally not an original symbol.
 */
int jpb_BrainDirectionAngle(
    float axis_x, float axis_y, int camera_angle)
{
    const float pi = 3.14159274101257324219f;
    const float half_turn = 2048.0f;
    float direction =
        (atan2f(axis_x, axis_y) / pi) * half_turn;

    return (int)(direction + half_turn +
                 (float)(0x800 - camera_angle));
}

/*
 * Instruction-reviewed extraction of brain_ControlPlayer's ordinary, unlocked directional
 * ground branch (0x1D141..0x1D166 and 0x1D5FB..0x1D64E).
 *
 * The special motion-2/motion-60 paths are rejected until their dependencies
 * are reconstructed. For the covered state, turn-to-face or lock-on
 * turn-to-attack, Motion[1] lock activation, the player-0/1 velocity rewrite,
 * and omnidirectional Motion[26]/Motion[8] selection preserve executable
 * order and stores.
 */
JPBBrainResult jpb_BrainGroundDirectionState(
    playerObject *player,
    float axis_x,
    float axis_y,
    int camera_angle)
{
    int facing;

    if (player == NULL ||
        player->playerRoot.pParent == NULL ||
        player->paMotions == NULL ||
        player->maxMotions <= 1) {
        return JPB_BRAIN_RESULT_INVALID_ARGUMENT;
    }
    if (player->currentMotion == 2 ||
        player->currentMotion == 0x3c) {
        return JPB_BRAIN_RESULT_UNSUPPORTED_STATE;
    }

    facing = jpb_BrainDirectionAngle(
        axis_x, axis_y, camera_angle);
    if ((player->pFlags & 0x00400000u) != 0) {
        enum {
            omnidirectional_forward_motion = 26,
            omnidirectional_reverse_motion = 8
        };
        int current_facing =
            physics_gGetFacing(&player->playerRoot);
        uint32_t wrapped_delta =
            ((uint32_t)current_facing -
             (uint32_t)facing) &
            0x0fffu;
        int delta =
            wrapped_delta >= 0x0800u
                ? (int)wrapped_delta - 0x1000
                : (int)wrapped_delta;
        int motion_index =
            (uint32_t)(delta + 0x05ff) > 0x0bfeu
                ? omnidirectional_reverse_motion
                : omnidirectional_forward_motion;
        Motion *motion;

        if (player->maxMotions <= motion_index) {
            return JPB_BRAIN_RESULT_UNSUPPORTED_STATE;
        }
        motion = &player->paMotions[motion_index];
        motion->Speed = 0x24;
        motion->frzin = 0;
        motion->frzout = 0;
        motion->twin = 2;
        motion->twout = 2;
        physics_gSetFacing(
            &player->playerRoot, facing);
        if (player->currentMotion == motion_index) {
            return JPB_BRAIN_RESULT_NO_CHANGE;
        }
        return animctrl_MotionEqualLock(
                   &player->playerRoot, motion)
                   ? JPB_BRAIN_RESULT_OK
                   : JPB_BRAIN_RESULT_NO_CHANGE;
    }
    if ((player->pFlags & 0x00000010u) != 0) {
        physics_gTurnToAttack(
            &player->playerRoot, facing, 2);
    } else {
        physics_gTurnToFace(
            &player->playerRoot, facing, 2);
    }
    if (!animctrl_MotionLock(
            &player->playerRoot, &player->paMotions[1])) {
        return JPB_BRAIN_RESULT_NO_CHANGE;
    }
    if (player->playerID <= 1) {
        player->paMotions[1].vel = 0x15;
    }
    return JPB_BRAIN_RESULT_OK;
}

/*
 * Instruction-reviewed extraction of brain_ControlPlayer's lock-on directional branch
 * (0x1D28D..0x1D3AF).
 *
 * The original selects forward Motion[26], left/right Motion[29]/Motion[30],
 * or rear Motion[12] from the wrapped current-minus-requested facing delta.
 * It then force-faces player->target, activates the selected motion at equal
 * lock, and increments the exact 16-bit runCounter. This descriptive facade
 * is not an original PDB symbol.
 */
JPBBrainResult jpb_BrainLockOnDirectionState(
    playerObject *player, int desired_facing)
{
    enum {
        rear_motion = 12,
        forward_motion = 26,
        left_motion = 29,
        right_motion = 30
    };
    int current_facing;
    int delta;
    int motion_index = forward_motion;
    Motion *motion;

    if (player == NULL ||
        player->playerRoot.pParent == NULL ||
        player->paMotions == NULL) {
        return JPB_BRAIN_RESULT_INVALID_ARGUMENT;
    }
    if ((player->pFlags & UINT32_C(0x00400000)) == 0 ||
        player->maxMotions <= right_motion) {
        return JPB_BRAIN_RESULT_UNSUPPORTED_STATE;
    }

    current_facing =
        physics_gGetFacing(&player->playerRoot);
    delta = (int32_t)(
        (uint32_t)current_facing -
        (uint32_t)desired_facing);
    if (delta > 0x800) {
        delta = (int32_t)(
            (uint32_t)delta - UINT32_C(0x1000));
    }
    if (delta < -0x800) {
        delta = (int32_t)(
            (uint32_t)delta + UINT32_C(0x1000));
    }

    if (delta >= -0x5ff && delta <= -0x181) {
        motion_index = left_motion;
        player->paMotions[left_motion].motionFlags |=
            UINT32_C(8);
    }
    if (delta > 0x180) {
        if (delta >= 0x600) {
            motion_index = rear_motion;
        } else {
            motion_index = right_motion;
            player->paMotions[right_motion].motionFlags |=
                UINT32_C(8);
        }
    } else if (delta < -0x600) {
        motion_index = rear_motion;
    } else if (motion_index == forward_motion) {
        motion = &player->paMotions[forward_motion];
        motion->Speed = 0x24;
        motion->frzin = 0;
        motion->frzout = 0;
        motion->twin = 2;
        motion->twout = 2;
    }

    (void)physics_gForceFaceTarget(
        &player->playerRoot,
        player->target != NULL
            ? &player->target->playerRoot
            : NULL);
    if (animctrl_MotionEqualLock(
            &player->playerRoot,
            &player->paMotions[motion_index])) {
        player->runCounter = 0;
    }
    player->runCounter = (int16_t)(
        (uint16_t)player->runCounter + UINT16_C(1));
    return JPB_BRAIN_RESULT_OK;
}

/*
 * Instruction-reviewed extraction of brain_ControlPlayer's successful jump-launch block
 * (0x1CFF0..0x1D08C).
 *
 * The runtime callback table slot at 0x1410EFBB0 is initialized by
 * game_InitGameSystems to exact brainutil_PlotTrajectory and is replaceable
 * by the existing extension layer. This descriptive boundary therefore
 * accepts the slot value explicitly instead of hard-wiring a PC-only global.
 */
JPBBrainResult jpb_BrainJumpLaunchState(
    playerObject *player,
    int stand,
    JPBPlayerCallback trajectory_callback)
{
    enum { jump_motion = 4 };
    physicsObject *physics;

    if (player == NULL ||
        player->playerRoot.pParent == NULL ||
        player->paMotions == NULL) {
        return JPB_BRAIN_RESULT_INVALID_ARGUMENT;
    }
    if (player->maxMotions <= jump_motion) {
        return JPB_BRAIN_RESULT_UNSUPPORTED_STATE;
    }
    physics = brain_player_physics(player);
    if (physics->mapinfo.poly != NULL &&
        (leveldata[
             (uint32_t)*physics->mapinfo.poly &
             UINT32_C(0x1ffff)] &
         INT32_C(0x4000)) != 0) {
        return JPB_BRAIN_RESULT_NO_CHANGE;
    }
    if (!animctrl_MotionLock(
            &player->playerRoot,
            &player->paMotions[jump_motion])) {
        return JPB_BRAIN_RESULT_NO_CHANGE;
    }

    brain_SetJumpTrajectory(player, stand != 0);
    player->pMotionCallBack = trajectory_callback;
    brain_SetTrajectory(
        player,
        player->airVelocity,
        player->airAngle);
    physics_gSnapShotPosition(
        &player->playerRoot, 0x3c);
    physics->reversoi = 0;
    return JPB_BRAIN_RESULT_OK;
}

/*
 * Instruction-exact alternate jump launch from brain_ControlPlayer
 * (0x1D23E..0x1D28C). Unlike the ordinary launch, this path quantizes facing
 * to the original 128-angle boundary and requests Motion[4] at lock level 30.
 */
JPBBrainResult jpb_BrainAlternateJumpLaunchState(
    playerObject *player,
    JPBPlayerCallback trajectory_callback)
{
    enum {
        jump_motion = 4,
        jump_lock_level = 30
    };
    physicsObject *physics;

    if (player == NULL ||
        player->playerRoot.pParent == NULL ||
        player->paMotions == NULL) {
        return JPB_BRAIN_RESULT_INVALID_ARGUMENT;
    }
    if (player->maxMotions <= jump_motion) {
        return JPB_BRAIN_RESULT_UNSUPPORTED_STATE;
    }
    physics = brain_player_physics(player);
    if (physics->mapinfo.poly != NULL &&
        (leveldata[
             (uint32_t)*physics->mapinfo.poly &
             UINT32_C(0x1ffff)] &
         INT32_C(0x4000)) != 0) {
        return JPB_BRAIN_RESULT_NO_CHANGE;
    }

    physics->angle.vy &= 0x0f80;
    if (!animctrl_MotionLockLevel(
            &player->playerRoot,
            &player->paMotions[jump_motion],
            jump_lock_level)) {
        return JPB_BRAIN_RESULT_NO_CHANGE;
    }

    brain_SetJumpTrajectory(player, 0);
    player->pMotionCallBack = trajectory_callback;
    brain_SetTrajectory(
        player,
        player->airVelocity,
        player->airAngle);
    physics_gSnapShotPosition(
        &player->playerRoot, 0x3c);
    return JPB_BRAIN_RESULT_OK;
}

/*
 * Instruction-exact attack-button transition tail from brain_ControlPlayer
 * (0x1D76B..0x1D87A). Motion indexes and masks are direct executable
 * evidence; this descriptive boundary remains separate until the complete
 * PDB entry can own the full input/callback prelude.
 */
JPBBrainResult jpb_BrainGroundAttackState(
    playerObject *player)
{
    enum {
        idle_motion = 0,
        run_motion = 2,
        attack_motion = 15,
        alternate_attack_motion = 16,
        chained_attack_motion = 21,
        run_stop_motion = 25
    };
    physicsObject *physics;

    if (player == NULL ||
        player->playerRoot.pParent == NULL ||
        player->paMotions == NULL) {
        return JPB_BRAIN_RESULT_INVALID_ARGUMENT;
    }
    if (player->maxMotions <= run_stop_motion) {
        return JPB_BRAIN_RESULT_UNSUPPORTED_STATE;
    }
    physics = brain_player_physics(player);

    if (player->currentMotion == run_motion) {
        if (player->runCounter > 15 &&
            LevelSelect != 13 &&
            animctrl_MotionNoLock(
                &player->playerRoot,
                &player->paMotions[run_stop_motion])) {
            player->hitDelay = 0;
            player->paMotions[run_stop_motion].FunctPtr = 4;
            player->pFlags &= UINT32_C(0xffbfffff);
            return JPB_BRAIN_RESULT_OK;
        }
        if (animctrl_MotionNoLock(
                &player->playerRoot,
                &player->paMotions[idle_motion])) {
            player->hitDelay = 0;
            physics_gClrConstantVector(
                &player->playerRoot);
            return JPB_BRAIN_RESULT_OK;
        }
        return JPB_BRAIN_RESULT_NO_CHANGE;
    }

    player->pFlags |= UINT32_C(0x20);
    player->paMotions[attack_motion].motionFlags &=
        UINT32_C(0x7bffffff);
    player->paMotions[attack_motion].disp = 0;
    if (player->currentMotion == chained_attack_motion ||
        player->currentMotion == attack_motion ||
        player->currentMotion == alternate_attack_motion) {
        return JPB_BRAIN_RESULT_NO_CHANGE;
    }
    if (!animctrl_MotionLock(
            &player->playerRoot,
            &player->paMotions[attack_motion])) {
        return JPB_BRAIN_RESULT_NO_CHANGE;
    }
    player->pFlags &= UINT32_C(0xffffffef);
    (void)animctrl_MotionChain(
        &player->playerRoot,
        &player->paMotions[chained_attack_motion]);
    return JPB_BRAIN_RESULT_OK;
}

/*
 * Instruction-reviewed extraction of brain_ControlPlayer's stationary ground branch
 * (0x1D65C..0x1D766, with the run-stop exit at 0x1D7A2).
 *
 * game_gGetEnergy remains outside the reviewed subset, so its result is an
 * explicit argument. The original selects Motion[0] normally, Motion[19]
 * below 26 energy, and Motion[20] while locked on. A player still in
 * Motion[2] may transition through Motion[25] after sixteen run ticks.
 *
 * A complete reference player always has pMotion linked to the animation
 * component's current Motion pointer. This bounded seam tolerates an absent
 * link, but performs the exact action-flag cleanup whenever it is present.
 */
JPBBrainResult jpb_BrainGroundIdleState(
    playerObject *player, int energy)
{
    enum {
        normal_idle_motion = 0,
        low_energy_idle_motion = 19,
        lock_on_idle_motion = 20,
        run_stop_motion = 25,
        low_energy_threshold = 26,
        idle_lock_level = 22
    };
    sceneObject *scene;
    Motion *idle_motion;
    int idle_motion_index = normal_idle_motion;
    int changed = 0;

    if (player == NULL ||
        player->playerRoot.pParent == NULL ||
        player->paMotions == NULL) {
        return JPB_BRAIN_RESULT_INVALID_ARGUMENT;
    }
    scene = (sceneObject *)player->playerRoot.pParent;
    if (scene->pAnim == NULL || scene->pPhysics == NULL) {
        return JPB_BRAIN_RESULT_INVALID_ARGUMENT;
    }

    if ((player->pFlags & 0x00400000u) != 0) {
        idle_motion_index = lock_on_idle_motion;
    } else if (energy < low_energy_threshold) {
        idle_motion_index = low_energy_idle_motion;
    }
    if (player->maxMotions <= idle_motion_index) {
        return JPB_BRAIN_RESULT_UNSUPPORTED_STATE;
    }
    idle_motion = &player->paMotions[idle_motion_index];

    if (player->currentMotion == 2) {
        if (player->runCounter > 15) {
            if (player->maxMotions <= run_stop_motion) {
                return JPB_BRAIN_RESULT_UNSUPPORTED_STATE;
            }
            if (animctrl_MotionNoLock(
                    &player->playerRoot,
                    &player->paMotions[run_stop_motion])) {
                player->hitDelay = 0;
                player->paMotions[run_stop_motion].FunctPtr = 4;
                player->pFlags &= 0xffbfffffu;
                return JPB_BRAIN_RESULT_OK;
            }
        }
        if (animctrl_MotionNoLock(
                &player->playerRoot, idle_motion)) {
            player->hitDelay = 0;
            physics_gClrConstantVector(
                &player->playerRoot);
            changed = 1;
        }
    }

    if (player->runCounter != 0) {
        changed = 1;
    }
    player->runCounter = 0;
    if (animctrl_MotionLockLevel(
            &player->playerRoot,
            idle_motion,
            idle_lock_level)) {
        player->hitDelay = 0;
        physics_gClrConstantVector(
            &player->playerRoot);
        player->currentMotion =
            (int16_t)idle_motion_index;
        changed = 1;
    }

    if (player->pMotion != NULL &&
        *player->pMotion != NULL &&
        ((*player->pMotion)->motionFlags &
         0x00000001u) != 0) {
        uint32_t old_flags = player->pFlags;

        player->pFlags = old_flags & 0xffffffefu;
        if ((old_flags & 0x01000000u) != 0) {
            player->pFlags =
                old_flags & 0xfcdfffefu;
        }
        changed = 1;
    }
    return changed
               ? JPB_BRAIN_RESULT_OK
               : JPB_BRAIN_RESULT_NO_CHANGE;
}

/*
 * Instruction-reviewed extraction of brain_ControlPlayer's directional-input handoff for
 * current Motion[2] and Motion[60] (0x1D141..0x1D15C and
 * 0x1D64F..0x1D65C). The executable raises the current animation lock to 15,
 * then enters the same energy/lock-on idle selection recovered above.
 *
 * The desired facing is deliberately absent: this special branch does not
 * consume the calculated direction after identifying either motion.
 */
JPBBrainResult jpb_BrainGroundSpecialDirectionState(
    playerObject *player, int energy)
{
    if (player == NULL ||
        (player->currentMotion != 2 &&
         player->currentMotion != 0x3c)) {
        return player == NULL
                   ? JPB_BRAIN_RESULT_INVALID_ARGUMENT
                   : JPB_BRAIN_RESULT_UNSUPPORTED_STATE;
    }

    animutl_SetCurrentLock(
        &player->playerRoot, 0x0f);
    return jpb_BrainGroundIdleState(player, energy);
}

/* 0x1C840, 227 bytes, global, 5 named locals
 * brain_CheckForEffects
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
void brain_CheckForEffects(playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    modelObject *model =
        (modelObject *)scene->pModel;
    Motion *motion = *player->pMotion;
    uint32_t events = model->eventMask;

    if (motion->Damage == 0 && events != 0) {
        int x = brainutl_FindLSB_LV(events);

        while (x != 0) {
            char sound[9];

            events &=
                ~(UINT32_C(1) << ((unsigned)x - 1u));
            memcpy(sound, motion->snd[1], 8);
            sound[8] = '\0';
            brainutl_PlayMotionSound(
                player->playernum,
                sound,
                motion->sndDelay[1]);
            x = brainutl_FindLSB_LV(events);
        }
    }
}

/*
 * Exact brain_ControlPlayer is implemented in brain_control.c. The separation
 * is a portable build boundary, not a separate game procedure.
 */

/* 0x1D8B0, 125 bytes, global, 4 named locals
 * brain_DoRingOffEffect
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
void brain_DoRingOffEffect(playerObject *player)
{
    enum { ring_off_effect = 63 };
    sceneObject *target_scene =
        (sceneObject *)player->locked->playerRoot.pParent;
    physicsObject *target_physics =
        (physicsObject *)target_scene->pPhysics;
    VECTOR *pos = &target_physics->vpos;
    EffectHeader *effect = paEffects[ring_off_effect];
    Ring **r;
    Ring *ring;
    CVECTOR colour;

    pos->vy += 2;
    r = (Ring **)(void *)sprite_AddSpriteEffect(
        effect->aEffects,
        (int)effect->num,
        pos,
        NULL);
    ring = *r;
    ring->rot.pad =
        (int16_t)((uint16_t)ring->rot.pad |
                  UINT16_C(0x20));
    colour = jedi_GetColour(
        (uint64_t)(int64_t)player->playerID);
    ring->pos.pad = (int32_t)colour.cd;
    if (player->lockRing != NULL) {
        ((Ring *)(void *)player->lockRing)->time = 0;
        player->lockRing = NULL;
    }
}

/* 0x1D930, 111 bytes, global, 4 named locals
 * brain_DoRingOnEffect
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
void brain_DoRingOnEffect(playerObject *player)
{
    enum { ring_on_effect = 44 };
    sceneObject *target_scene =
        (sceneObject *)player->locked->playerRoot.pParent;
    physicsObject *target_physics =
        (physicsObject *)target_scene->pPhysics;
    VECTOR *pos = &target_physics->vpos;
    EffectHeader *effect = paEffects[ring_on_effect];
    Ring **r;
    Ring *ring;
    CVECTOR colour;

    pos->vy += 2;
    r = (Ring **)(void *)sprite_AddSpriteEffect(
        effect->aEffects,
        (int)effect->num,
        pos,
        NULL);
    ring = *r;
    player->lockRing = (int32_t *)(void *)ring;
    ring->rot.pad =
        (int16_t)((uint16_t)ring->rot.pad |
                  UINT16_C(0x20));
    colour = jedi_GetColour(
        (uint64_t)(int64_t)player->playerID);
    ring->pos.pad = (int32_t)colour.cd;
}

/* 0x1D9A0, 680 bytes, global, 5 named locals
 * brain_GroundControl
 * PDB type: int (long*, playerObject*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
int brain_GroundControl(
    int32_t *cpad, playerObject *player, playerObject *target)
{
    enum {
        down_motion = 14,
        death_flag = 0x00000200,
        scene_afterlife_flag = 0x00000020,
        player_exit_flag = 0x00040000
    };
    uint32_t timer = gGlobalTimer;

    (void)target;
    if ((player->pFlags & death_flag) != 0) {
        if (player->playernum < 2 &&
            obj_gCheckObjectFlag(
                &player->playerRoot,
                0,
                scene_afterlife_flag) == 0 &&
            player->groundDelay <
                timer + UINT32_C(0x3c00)) {
            player_AfterLife(player);
            obj_gSetObjectFlag(
                &player->playerRoot,
                0,
                scene_afterlife_flag);
        }
        if (player->groundDelay < timer) {
            if (player->playernum < 2) {
                sceneObject *scene =
                    (sceneObject *)player->playerRoot.pParent;
                animObject *animation =
                    (animObject *)scene->pAnim;
                unsigned player_index =
                    (uint16_t)player->playernum & 31u;

                sound_StopSound(animation->loopHandle[0]);
                sound_StopSound(animation->loopHandle[1]);
                (void)game_gSetGameFlags(
                    UINT32_C(0x20) << player_index);
                player->pFlags |= player_exit_flag;
            } else {
                player->pEnemy->exit_flag = 1;
            }
        }
        return 1;
    }

    if (game_gGetEnergy(player->playernum) < 1) {
        player->pFlags |= death_flag;
        if (player->playernum > 1) {
            int32_t current_length =
                animutl_gGetCurrentAnimLength(
                    &player->playerRoot);

            player->groundDelay =
                timer +
                (uint32_t)(current_length + 6) *
                    UINT32_C(0x200);
        } else {
            player->groundDelay =
                timer + UINT32_C(0x12c00);
        }
        return 0;
    }

    if (player->hitDelay <= timer) {
        if (player->groundDelay <= timer &&
            (player->pFlags & death_flag) == 0) {
            Motion *down = &player->paMotions[down_motion];

            player->fStun = 0;
            player->PreMotion[0] = '\0';
            if (player->groundDelay +
                    UINT32_C(0x3c00) >=
                    timer ||
                !animctrl_MotionNoLock(
                    &player->playerRoot, down)) {
                if (cpad[0] == 0 ||
                    player->currentMotion == down_motion ||
                    !animctrl_MotionNoLock(
                        &player->playerRoot, down)) {
                    return 1;
                }
            }
            player->pFlags &= ~UINT32_C(0x00000c00);
            down->motionFlags |= UINT32_C(0x02000000);
            player->groundDelay = 0;
            player->hitDelay =
                timer + UINT32_C(0x5000);
            return 0;
        }

        /*
         * The reference calls debug_printf here when hitDelay < timer.
         * That exact PDB function is an 18-byte no-op returning zero, so
         * retaining the condition would have no observable effect.
         */
    }

    if (((player->groundDelay - timer) &
         UINT32_C(0xfffffe00)) >
        UINT32_C(0x5a00)) {
        player->groundDelay =
            timer + UINT32_C(0x5a00);
    }
    return 1;
}

/* 0x1DC50, 85 bytes, global, 2 named locals
 * brain_HangCallback
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
int brain_HangCallback(int32_t *cpad, playerObject *player)
{
    (void)cpad;
    if (coll_CheckForEventNode(player->playernum, 0) != 0) {
        player->pFlags &= ~UINT32_C(0x40000000);
        if (player->shadow != NULL) {
            sprite_gUnHideSprite(
                (Sprite *)(void *)player->shadow);
        }
        player->ACTION_LOCK = 0;
        return 1;
    }
    return -1;
}

/* 0x1DCB0, 336 bytes, global, 6 named locals
 * brain_LockOn
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
int brain_LockOn(int32_t *cpad, playerObject *player)
{
    static const uint64_t excluded_player_ids =
        UINT64_C(0x0020020044008000);
    uint16_t player_id = (uint16_t)player->playerID;
    /* The matched gameplay brain intentionally shares P1's scheme binding
     * across both players after per-device input translation. */
    uint32_t lock_button =
        (uint32_t)gaButtonMap[
            OptionStruct.ControllerConfig[0]][1];

    ++jpb_brain_lock_diagnostics.calls;
    jpb_brain_lock_diagnostics.lastPadBits = (uint32_t)cpad[0];
    jpb_brain_lock_diagnostics.lastLevel = LevelSelect;
    jpb_brain_lock_diagnostics.lastPlayerId = player_id;
    if (((uint32_t)cpad[0] & lock_button) != 0) {
        ++jpb_brain_lock_diagnostics.inputButtonFrames;
    }

    if (LevelSelect == 8 ||
        (player->pFlags & UINT32_C(0x8000)) != 0 ||
        (player_id <= 53 &&
         (excluded_player_ids &
          (UINT64_C(1) << player_id)) != 0) ||
        ((uint32_t)cpad[0] & lock_button) == 0) {
        return 0;
    }

    ++jpb_brain_lock_diagnostics.eligibleFrames;

    player->pFlags ^= UINT32_C(0x00400000);
    if ((player->pFlags & UINT32_C(0x00400000)) != 0) {
        objectRoot *temp =
            brainutl_gGetNearestTarget(
                &player->playerRoot, 2);

        ++jpb_brain_lock_diagnostics.targetSearches;

        if (temp != NULL) {
            sceneObject *target_scene =
                (sceneObject *)temp->pParent;
            physicsObject *target_physics;
            EffectHeader *effect = paEffects[44];
            Ring **r;
            Ring *ring;
            VECTOR *pos;
            CVECTOR colour;

            ++jpb_brain_lock_diagnostics.targetsFound;

            player->locked =
                (playerObject *)target_scene->pPlayer;
            target_scene = (sceneObject *)
                player->locked->playerRoot.pParent;
            target_physics = (physicsObject *)
                target_scene->pPhysics;
            pos = &target_physics->vpos;
            pos->vy += 2;
            r = (Ring **)(void *)
                sprite_AddSpriteEffect(
                    effect->aEffects,
                    (int)effect->num,
                    pos,
                    NULL);
            ring = *r;
            player->lockRing =
                (int32_t *)(void *)ring;
            ring->rot.pad =
                (int16_t)((uint16_t)ring->rot.pad |
                          UINT16_C(0x20));
            colour = jedi_GetColour(
                (uint64_t)(int64_t)player->playerID);
            ring->pos.pad = (int32_t)colour.cd;
            (void)sound_Play(
                &brain_player_physics(player)->vpos,
                0,
                "xlockon",
                0);
            return 0;
        }
        player->pFlags &= ~UINT32_C(0x00400000);
        return 0;
    }

    if (player->locked != NULL) {
        brain_DoRingOffEffect(player);
    }
    player->locked = NULL;
    return 0;
}

/* 0x1DE00, 68 bytes, global, 2 named locals
 * brain_SetFallTrajectory
 * PDB type: void (playerObject*, int)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
void brain_SetFallTrajectory(playerObject *player, int attack)
{
    int jump_velocity = player->pSettings.JumpVel;

    (void)attack;
    if (player->playerRoot.objectID < 2) {
        player->airVelocity = jump_velocity / 4;
        player->airAngle = 0x200;
    } else {
        player->airVelocity = jump_velocity / 2;
        player->airAngle = player->pSettings.dblJumpAngle;
    }
}

/* 0x1DE50, 126 bytes, global, 3 named locals
 * brain_SetJumpTrajectory
 * PDB type: void (playerObject*, int)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
void brain_SetJumpTrajectory(playerObject *player, int stand)
{
    physicsObject *physics = brain_player_physics(player);
    uint32_t old_flags = player->pFlags;

    physics->reversoi = 0;
    player->pFlags = old_flags & 0xffbfffffu;
    if (stand != 0) {
        player->airVelocity = player->pSettings.JumpVel;
        player->airAngle = 0x400;
    } else if ((old_flags & 0x00000100u) != 0) {
        player->airVelocity = player->pSettings.JumpVel;
        player->airAngle = player->pSettings.JumpAngle;
    } else {
        player->airVelocity = player->pSettings.RunningJumpVel;
        player->airAngle = player->pSettings.RunningJumpAngle;
    }
}

/* 0x1DED0, 258 bytes, global, 4 named locals
 * brain_SetTrajectory
 * PDB type: void (playerObject*, int, int)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
void brain_SetTrajectory(playerObject *player, int velocity, int angle)
{
    physicsObject *physics = brain_player_physics(player);
    int cosine_component;
    int sine_component;
    int speed;

    if (physics == NULL) {
        return;
    }
    if (angle == -1) {
        angle = physics->trajectory;
    } else {
        physics->trajectory = (int16_t)angle;
    }
    if (velocity == -1) {
        velocity = physics->airspeed;
    } else {
        physics->airspeed = (int16_t)velocity;
    }
    if (physics->mov.vy <= 0.0f) {
        physics->airTime = 0;
        physics->realAirTime = 0;
    }

    speed = velocity < 0 ? -velocity : velocity;
    cosine_component = brain_mul_fixed12(rcos(angle), speed);
    physics->airmov.vz = (float)cosine_component;
    physics->airmov.vx = 0.0f;
    sine_component = brain_mul_fixed12(rsin(angle), velocity);
    physics->airmov.vy =
        (float)(sine_component < 0
                    ? -sine_component
                    : sine_component);
    player->pFlags =
        (player->pFlags | 0x00000001u) & 0xbbffffffu;
}

/* 0x1DFE0, 136 bytes, global, 4 named locals
 * brain_SkidCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
int brain_SkidCallBack(int32_t *cpad, playerObject *player)
{
    enum { skid_effect = 13 };
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    animObject *pAnim =
        (animObject *)scene->pAnim;
    int frame = pAnim->animFrameIndex >> 12;
    int last_frame =
        pAnim->pCurrentAnimSeq->pAnimTemplate->Lframe;

    (void)cpad;
    if (frame <= last_frame) {
        if (rand() % 100 < 8) {
            EffectHeader *effect =
                paEffects[skid_effect];

            (void)sprite_AddSpriteEffectAtNode(
                effect->aEffects,
                (int)effect->num,
                player->playernum,
                6);
        }
        return 0;
    }
    return 1;
}

/* 0x1E070, 60 bytes, global, 3 named locals
 * brain_SwapVelDirCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
int brain_SwapVelDirCallBack(int32_t *cpad, playerObject *player)
{
    FVECTOR *velocity =
        physics_gGetConstantVector(&player->playerRoot);

    (void)cpad;
    if (velocity->vz != 0.0f) {
        physics_gSetConstantVector(
            &player->playerRoot,
            velocity->vz,
            velocity->vy,
            velocity->vx);
    }
    return 1;
}

/* 0x1E0B0, 154 bytes, global, 4 named locals
 * brain_TakeOff
 * PDB type: int (long*, playerObject*, playe...
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
int brain_TakeOff(
    int32_t *cpad, playerObject *player, playerObject *target)
{
    enum { jump_motion = 4 };
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    animObject *pAnim =
        (animObject *)scene->pAnim;
    int frame = pAnim->animFrameIndex >> 12;
    int last_frame =
        pAnim->pCurrentAnimSeq->pAnimTemplate->Lframe;
    Motion *motion = &player->paMotions[jump_motion];

    (void)cpad;
    (void)target;
    if (frame >= last_frame) {
        motion->motionFlags |= UINT32_C(0x04000000);
        (void)animctrl_MotionNoLock(
            &player->playerRoot, motion);
        player->pMotionCallBack = funcArray[6];
        brain_SetTrajectory(
            player,
            player->airVelocity,
            player->airAngle);
        physics_gSnapShotPosition(
            &player->playerRoot, 0x3c);
    }
    return 0;
}

/* 0x1E150, 284 bytes, global, 5 named locals
 * brain_ThrowEnder
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
int brain_ThrowEnder(int32_t *cpad, playerObject *player)
{
    enum { thrown_motion = 51 };
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    animObject *pAnim =
        (animObject *)scene->pAnim;
    int frame = pAnim->animFrameIndex >> 12;
    int last_frame =
        pAnim->pCurrentAnimSeq->pAnimTemplate->Lframe;

    (void)cpad;
    if (frame >= last_frame - 2) {
        int velocity = player->pSettings.JumpVel / 2;
        int angle = player->pSettings.bkJumpAngle;
        int delay_steps;

        (void)animctrl_MotionNoLock(
            &player->playerRoot,
            &player->paMotions[thrown_motion]);
        physics_gSnapShotPosition(
            &player->playerRoot, 0);
        brain_SetTrajectory(player, velocity, angle);
        player->pMotionCallBack = funcArray[6];
        player->fLife = 0;
        player->hitNumber = 0;
        player->hitDelay =
            gGlobalTimer + UINT32_C(0x0c00);
        delay_steps = velocity / 300;
        if (delay_steps < 0) {
            delay_steps = -delay_steps;
        }
        delay_steps += 15;
        if (delay_steps > 45) {
            delay_steps = 45;
        }
        player->groundDelay =
            gGlobalTimer +
            (uint32_t)delay_steps * UINT32_C(0x200);
        player->delayedMotion = 0;
    }
    return -1;
}

/* 0x1E270, 477 bytes, global, 2 named locals
 * brain_ValidateLockOn
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
void brain_ValidateLockOn(playerObject *player)
{
    enum {
        inactive_flags = 0x00040200,
        invalid_lock_flags = 0x44000001
    };
    Ring *ring;

    if (player->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(
            &player->playerRoot,
            0,
            UINT32_C(0x20)) != 0 ||
        (player->pFlags &
         (uint32_t)inactive_flags) != 0) {
        return;
    }

    if ((player->pFlags &
         UINT32_C(0x00400000)) == 0) {
        if (player->lockRing != NULL) {
            brain_DoRingOffEffect(player);
        }
        return;
    }

    if (player->locked == NULL ||
        player->locked->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(
            &player->locked->playerRoot,
            0,
            UINT32_C(0x20)) != 0 ||
        (player->pFlags &
         (uint32_t)invalid_lock_flags) != 0 ||
        physics_gGetRange(
            &player->playerRoot,
            &player->locked->playerRoot) > 0x400 ||
        game_gGetEnergy(
            player->locked->playernum) <= 0) {
        if (player->locked != NULL) {
            brain_DoRingOffEffect(player);
        }
        player->pFlags &= ~UINT32_C(0x00400000);
        player->locked = NULL;
        return;
    }

    ring = (Ring *)(void *)player->lockRing;
    if (ring != NULL) {
        sceneObject *target_scene =
            (sceneObject *)
                player->locked->playerRoot.pParent;
        physicsObject *target_physics =
            (physicsObject *)target_scene->pPhysics;
        int x = (int)target_physics->pos.vx;
        int y = (int)target_physics->pos.vy;
        int z = (int)target_physics->pos.vz;
        CVECTOR colour;

        ring->time =
            gGlobalTimer + UINT32_C(0x1e00);
        colour = jedi_GetColour(
            (uint64_t)(int64_t)player->playerID);
        ring->pos.pad = (int32_t)colour.cd;
        ring->pos.vx =
            (int32_t)((float)x +
                      target_physics->mov.vx);
        ring->pos.vy =
            (int32_t)((float)y +
                      target_physics->mov.vy);
        ring->pos.vz =
            (int32_t)((float)z +
                      target_physics->mov.vz);
        ring->pos.vy += 2;
        (void)physics_gForceFaceTarget(
            &player->playerRoot,
            &player->locked->playerRoot);
    }
}
