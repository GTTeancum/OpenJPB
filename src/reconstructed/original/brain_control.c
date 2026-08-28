/*
 * REVIEWED RECONSTRUCTION of brain_ControlPlayer from
 * W:\SWJediPowerBattles\Work\brain.c.
 *
 * The matched PDB supplies the exact procedure name, signature, and all
 * source-local names. Control flow is recovered from Ghidra and checked
 * against the x64 instructions at RVAs 0x1C930..0x1D8A2. Small static
 * helpers below only name source blocks from that one PDB procedure; none
 * are claimed as original symbols.
 *
 * The Windows build queried SDL_GetKeyboardState directly for a hidden
 * five-key cheat. jpb_InputPowerBattleChordPressed is the platform input
 * boundary: the PC adapter may expose that chord, while nxdk can leave
 * the optional provider unset without importing SDL into gameplay code.
 */

#include "jpb/brain.h"
#include "jpb/animctrl.h"
#include "jpb/animutil.h"
#include "jpb/braindmg.h"
#include "jpb/brainutl.h"
#include "jpb/camera.h"
#include "jpb/combo.h"
#include "jpb/enemy.h"
#include "jpb/extracharacters.h"
#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/jonny.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sprite.h"
#include "jpb/world.h"

#include <stdint.h>

/* Exact brain.c module-local symbol at matched-PC RVA 0x4F13A0. */
static uint32_t wait;

static physicsObject *brain_control_physics(
    playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;

    return (physicsObject *)scene->pPhysics;
}

static int brain_control_jump_blocked(
    physicsObject *physics)
{
    if (physics->mapinfo.poly == NULL) {
        return 0;
    }
    return (leveldata[
                (uint32_t)*physics->mapinfo.poly &
                UINT32_C(0x1ffff)] &
            INT32_C(0x4000)) != 0;
}

static int brain_control_start_jump(
    int32_t *cpad,
    playerObject *player,
    physicsObject *physics)
{
    if (brain_control_jump_blocked(physics)) {
        return 1;
    }
    if (!animctrl_MotionLock(
            &player->playerRoot,
            &player->paMotions[4])) {
        return 0;
    }

    brain_SetJumpTrajectory(
        player,
        ((uint32_t)cpad[1] & UINT32_C(0xf000)) == 0);
    player->pMotionCallBack = funcArray[6];
    brain_SetTrajectory(
        player,
        player->airVelocity,
        player->airAngle);
    physics_gSnapShotPosition(&player->playerRoot, 0x3c);
    physics->reversoi = 0;
    return 1;
}

static void brain_control_start_alternate_jump(
    playerObject *player,
    physicsObject *physics)
{
    if (brain_control_jump_blocked(physics)) {
        return;
    }

    physics->angle.vy &= 0x0f80;
    if (!animctrl_MotionLockLevel(
            &player->playerRoot,
            &player->paMotions[4],
            0x1e)) {
        return;
    }

    brain_SetJumpTrajectory(player, 0);
    player->pMotionCallBack = funcArray[6];
    brain_SetTrajectory(
        player,
        player->airVelocity,
        player->airAngle);
    physics_gSnapShotPosition(&player->playerRoot, 0x3c);
}

static void brain_control_idle(
    playerObject *player)
{
    int motion_index = 0;
    Motion *motion;

    if ((player->pFlags & UINT32_C(0x00400000)) != 0) {
        motion_index = 20;
    } else if (game_gGetEnergy(player->playernum) < 0x1a) {
        motion_index = 19;
    }
    motion = &player->paMotions[motion_index];

    if (player->currentMotion == 2) {
        if (player->runCounter > 0x0f &&
            animctrl_MotionNoLock(
                &player->playerRoot,
                &player->paMotions[25])) {
            player->hitDelay = 0;
            player->paMotions[25].FunctPtr = 4;
            player->pFlags &= UINT32_C(0xffbfffff);
            return;
        }
        if (animctrl_MotionNoLock(
                &player->playerRoot, motion)) {
            player->hitDelay = 0;
            physics_gClrConstantVector(
                &player->playerRoot);
        }
    }

    player->runCounter = 0;
    if (animctrl_MotionLockLevel(
            &player->playerRoot, motion, 0x16)) {
        player->hitDelay = 0;
        physics_gClrConstantVector(
            &player->playerRoot);
        player->currentMotion = (int16_t)motion_index;
    }

    if (((*player->pMotion)->motionFlags &
         UINT32_C(1)) != 0) {
        uint32_t flags = player->pFlags;

        player->pFlags = flags & UINT32_C(0xffffffef);
        if ((flags & UINT32_C(0x01000000)) != 0) {
            player->pFlags =
                flags & UINT32_C(0xfcdfffef);
        }
    }
}

static void brain_control_block(
    playerObject *player)
{
    physicsObject *physics =
        brain_control_physics(player);

    if (player->currentMotion == 2) {
        if (player->runCounter > 0x0f &&
            LevelSelect != 13 &&
            animctrl_MotionNoLock(
                &player->playerRoot,
                &player->paMotions[25])) {
            player->hitDelay = 0;
            player->paMotions[25].FunctPtr = 4;
            player->pFlags &= UINT32_C(0xffbfffff);
            return;
        }
        if (animctrl_MotionNoLock(
                &player->playerRoot,
                &player->paMotions[0])) {
            player->hitDelay = 0;
            physics_gClrConstantVector(
                &player->playerRoot);
        }
        return;
    }

    player->pFlags =
        (player->pFlags & UINT32_C(0xffffffdf)) |
        UINT32_C(0x20);
    player->paMotions[15].motionFlags &=
        UINT32_C(0xfbffffff);
    player->paMotions[15].motionFlags &=
        UINT32_C(0x7fffffff);
    player->paMotions[15].disp = 0;
    if (player->currentMotion == 21 ||
        player->currentMotion == 15 ||
        player->currentMotion == 16) {
        return;
    }
    if (!animctrl_MotionLock(
            &player->playerRoot,
            &player->paMotions[15])) {
        return;
    }

    player->pFlags &= UINT32_C(0xffffffef);
    (void)animctrl_MotionChain(
        &player->playerRoot,
        &player->paMotions[21]);
}

static int brain_control_get_desired_facing(
    playerObject *player)
{
    float axis_x;
    float axis_y;

    if (player->playernum == 0) {
        axis_x = g_p1X;
        axis_y = g_p1Y;
    } else {
        axis_x = g_p2X;
        axis_y = g_p2Y;
    }
    return jpb_BrainDirectionAngle(
        axis_x, axis_y, mCameraAngleDest);
}

static int brain_control_use_directional_path(
    int32_t *cpad,
    playerObject *player)
{
    uint32_t held = (uint32_t)cpad[1];
    uint32_t directions = held & UINT32_C(0xf000);
    uint32_t mapped =
        held & (uint32_t)gaButtonMap[player->playernum][0];

    if (player->playernum == 0 &&
        player1InputType == 0) {
        return directions != 0 && mapped == 0;
    }
    if (player->playernum != 0 &&
        player2InputType == 0 &&
        player->playernum != 1) {
        return 0;
    }
    return directions != 0 && mapped != 0;
}

static playerObject *brain_control_nearest_player(
    playerObject *player)
{
    objectRoot *target =
        brainutl_gGetNearestTarget(
            &player->playerRoot, 2);
    sceneObject *scene;

    if (target == NULL) {
        return NULL;
    }
    scene = (sceneObject *)target->pParent;
    return (playerObject *)scene->pPlayer;
}

static int brain_control_try_special_motion(
    int32_t *cpad,
    playerObject *player)
{
    playerObject *nearby;
    int angle;
    int delta;
    int range;
    int motion_index;

    if (player->runCounter <= 0x14 ||
        !((player->playerID < 9) ||
          (player->playerID == 0x11))) {
        return 0;
    }
    if (((uint32_t)cpad[0] & UINT32_C(0xd0)) != 0) {
        nearby = brain_control_nearest_player(player);
        if (nearby != NULL &&
            nearby->playerRoot.objectID != -1 &&
            obj_gCheckObjectFlag(
                &nearby->playerRoot,
                0,
                UINT32_C(0x20)) == 0) {
            angle = physics_gGetFaceTargetDelta(
                &player->playerRoot,
                &nearby->playerRoot);
            delta = angle < 0 ? -angle : angle;
            range = physics_gGetRange(
                &player->playerRoot,
                &nearby->playerRoot);
            if (delta < 0x800 && range <= 0x200) {
                (void)physics_gForceFaceTarget(
                    &player->playerRoot,
                    &nearby->playerRoot);
            }
        }
    }

    if (((uint32_t)cpad[0] & UINT32_C(0x40)) != 0) {
        motion_index = 92;
    } else if (((uint32_t)cpad[0] &
                UINT32_C(0x80)) != 0) {
        motion_index = 93;
    } else if (((uint32_t)cpad[0] &
                UINT32_C(0x10)) != 0) {
        motion_index = 94;
    } else {
        return 0;
    }

    (void)animctrl_MotionLock(
        &player->playerRoot,
        &player->paMotions[motion_index]);
    player->paMotions[motion_index].disp =
        motion_index == 94 ? 0x0e : 0x0f;
    player->pFlags &= UINT32_C(0xfffffeff);
    return 1;
}

static void brain_control_locked_direction(
    playerObject *player,
    int facing,
    int desired_facing)
{
    int delta = facing - desired_facing;
    int move = 26;
    Motion *motion;

    if (delta > 0x800) {
        delta -= 0x1000;
    }
    if (delta < -0x800) {
        delta += 0x1000;
    }
    if ((uint32_t)(delta + 0x5ff) <
        UINT32_C(0x47f)) {
        move = 29;
        player->paMotions[29].motionFlags |=
            UINT32_C(8);
    }
    if (delta > 0x180) {
        if (delta >= 0x600) {
            move = 12;
        } else {
            move = 30;
            player->paMotions[30].motionFlags |=
                UINT32_C(8);
        }
    } else if (delta < -0x600) {
        move = 12;
    } else if (move == 26) {
        motion = &player->paMotions[26];
        motion->Speed = 0x24;
        motion->frzin = 0;
        motion->frzout = 0;
        motion->twin = 2;
        motion->twout = 2;
    }

    (void)physics_gForceFaceTarget(
        &player->playerRoot,
        &player->locked->playerRoot);
    if (animctrl_MotionEqualLock(
            &player->playerRoot,
            &player->paMotions[move])) {
        player->runCounter = 0;
    }
    player->runCounter =
        (int16_t)((uint16_t)player->runCounter + 1u);
}

static void brain_control_special_direction(
    playerObject *player,
    int facing,
    int desired_facing)
{
    int delta;
    int move;
    Motion *motion;

    if (player->currentMotion == 2 ||
        player->currentMotion == 60) {
        animutl_SetCurrentLock(
            &player->playerRoot, 0x0f);
        brain_control_idle(player);
        return;
    }

    if ((player->pFlags & UINT32_C(0x00400000)) == 0) {
        if ((player->pFlags & UINT32_C(0x10)) == 0) {
            physics_gTurnToFace(
                &player->playerRoot,
                desired_facing,
                2);
        } else {
            physics_gTurnToAttack(
                &player->playerRoot,
                desired_facing,
                2);
        }
        if (animctrl_MotionLock(
                &player->playerRoot,
                &player->paMotions[1]) &&
            player->playerID <= 1) {
            player->paMotions[1].vel = 0x15;
        }
        return;
    }

    delta = facing - desired_facing;
    if (delta > 0x800) {
        delta -= 0x1000;
    }
    if (delta < -0x800) {
        delta += 0x1000;
    }
    move =
        delta >= -0x5ff && delta <= 0x5ff
            ? 26
            : 8;
    motion = &player->paMotions[move];
    motion->Speed = 0x24;
    motion->frzin = 0;
    motion->frzout = 0;
    motion->twin = 2;
    motion->twout = 2;
    physics_gSetFacing(
        &player->playerRoot, desired_facing);
    if (player->currentMotion != move) {
        (void)animctrl_MotionEqualLock(
            &player->playerRoot, motion);
    }
}

static void brain_control_direction(
    int32_t *cpad,
    playerObject *player,
    physicsObject *physics)
{
    int facing = physics_gGetFacing(
        &player->playerRoot);
    int desired_facing =
        brain_control_get_desired_facing(player);
    int move = 1;

    OMNIDIRECTIONAL_MOVEMENT = 1;
    if (!brain_control_use_directional_path(
            cpad, player)) {
        brain_control_special_direction(
            player, facing, desired_facing);
        return;
    }

    if ((player->pFlags & UINT32_C(0x00400000)) != 0) {
        brain_control_locked_direction(
            player, facing, desired_facing);
        return;
    }

    /* Matched-PC brain_ControlPlayer reads the single P1 scheme byte here,
     * even for P2. ReadJoystickInput has already translated each physical
     * pad with its per-player scheme; preserve the executable's shared
     * gameplay action lookup rather than normalizing this to playernum. */
    if ((((uint32_t)cpad[1] &
          (uint32_t)gaButtonMap[
              OptionStruct.ControllerConfig[0]][4]) == 0) ||
        OptionStruct.JumpCheat == 0) {
        move = 2;
        physics->movemode = MOVE_NORMAL;
    }
    player->pFlags |= UINT32_C(0x100);
    physics_gTurnToFace(
        &player->playerRoot,
        desired_facing,
        2);

    if (brain_control_try_special_motion(
            cpad, player)) {
        return;
    }
    if (((uint32_t)cpad[0] &
         UINT32_C(0x20)) != 0) {
        brain_control_start_alternate_jump(
            player, physics);
        return;
    }
    if (animctrl_MotionEqualLock(
            &player->playerRoot,
            &player->paMotions[move])) {
        player->runCounter = 0;
    }
    player->runCounter =
        (int16_t)((uint16_t)player->runCounter + 1u);
}

static int brain_control_motion_allows_input(
    playerObject *player)
{
    static const uint32_t excluded_low_motions =
        UINT32_C(0x60001800);
    uint16_t current = (uint16_t)player->currentMotion;
    Motion *motion = &player->paMotions[
        (int16_t)current];

    if ((motion->motionFlags &
         UINT32_C(0x01000000)) != 0) {
        return 0;
    }
    if (current <= 30 &&
        ((excluded_low_motions >>
          ((unsigned)current & 31u)) & 1u) != 0) {
        return 0;
    }
    return (uint16_t)(current - UINT16_C(92)) > 2u;
}

static void brain_control_combo_target(
    playerObject *player)
{
    playerObject *nearby =
        brain_control_nearest_player(player);

    if (nearby != NULL &&
        nearby->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(
            &nearby->playerRoot,
            0,
            UINT32_C(0x20)) == 0) {
        int range =
            player->playerID == 6 ||
            player->playerID == 7
                ? 0x500
                : 0x200;

        (void)physics_gGetFaceTargetDelta(
            &player->playerRoot,
            &nearby->playerRoot);
        if (physics_gGetRange(
                &player->playerRoot,
                &nearby->playerRoot) <= range ||
            (nearby->pFlags & UINT32_C(0x2000)) != 0) {
            (void)physics_gForceFaceTarget(
                &player->playerRoot,
                &nearby->playerRoot);
        }
    }
    player->pFlags |= UINT32_C(0x10);
}

static int brain_control_power_battle_mode(
    int32_t *cpad,
    playerObject *player,
    physicsObject *physics)
{
    int powerBattleModePressed =
        jpb_InputPowerBattleChordPressed() ||
        (uint32_t)cpad[1] == UINT32_C(0xf2);
    char *message = "JEDI POWER BATTLE ON!";
    _svector velup = {0, 0x10, 0, 0};

    if (!powerBattleModePressed ||
        GameStruct.NumPlayers != 2 ||
        LevelSelect == 0x19 ||
        wait >= gGlobalTimer) {
        return 0;
    }

    wait = gGlobalTimer + UINT32_C(0x1e00);
    GameStruct.versusModeFlag ^= 1;
    if (GameStruct.versusModeFlag == 0) {
        message = "JEDI POWER BATTLE OFF!";
    }
    (void)sprite_GetCommentsSprite(
        message,
        &physics->vpos,
        &velup,
        UINT32_C(0x7fffffff));
    return 1;
}

/*
 * 0x1C930, 3954 bytes, global, 31 named locals
 * brain_ControlPlayer
 * PDB type: void (long*, playerObject*, int)
 * Source: W:\SWJediPowerBattles\Work\brain.c
 */
void brain_ControlPlayer(
    int32_t *cpad, playerObject *player, int AI_ON)
{
    playerObject *target = player->target;
    physicsObject *physics =
        brain_control_physics(player);
    int rtn = 0;
    uint32_t flags;
    uint32_t block_button;

    if ((player->pFlags & UINT32_C(0x40000)) != 0) {
        return;
    }
    if (player->playernum < 2) {
        brain_ValidateLockOn(player);
    }
    if ((((uint32_t)cpad[1] & UINT32_C(0x0c)) ==
         UINT32_C(0x0c)) &&
        OptionStruct.JumpCheat != 0) {
        enemy_KillKill(&physics->vpos, 0x600);
    }

    /* Matched executable store at playerObject + 0x1bc.  The adjacent
     * currentMotion field is at +0x1b4 and must remain intact so motion
     * callbacks can distinguish active airborne motions 4 and 22. */
    player->ACTION_LOCK = 0;
    if (brain_control_power_battle_mode(
            cpad, player, physics)) {
        return;
    }

    brain_CheckForEffects(player);
    rtn = braindmg_DamageControl(player);
    if (rtn != 0) {
        player->pFlags &= UINT32_C(0xbbffffff);
        player->ACTION_LOCK = 1;
        if (player->shadow != NULL) {
            sprite_gUnHideSprite(
                (Sprite *)(void *)player->shadow);
        }
    }

    if ((player->pFlags & UINT32_C(0x04000000)) != 0) {
        player->pMotionCallBack = NULL;
        player->pFlags &= UINT32_C(0xfffffffe);
        player->paMotions[63].FunctPtr = 3;
        if (animctrl_MotionNoLock(
                &player->playerRoot,
                &player->paMotions[62])) {
            (void)animctrl_MotionChain(
                &player->playerRoot,
                &player->paMotions[63]);
            player->ACTION_LOCK = 1;
            return;
        }
    }

    if (player->pMotionCallBack != NULL) {
        rtn = player->pMotionCallBack(cpad, player);
        if (rtn == 1) {
            player->pMotionCallBack = NULL;
        }
    }
    if (player->pForceCallBack == NULL ||
        (rtn = player->pForceCallBack(
            cpad, player)) != 1) {
        if (rtn == -1 &&
            (player->pFlags & UINT32_C(0x200)) == 0) {
            return;
        }
    } else {
        rtn = 0;
        player->pForceCallBack = NULL;
    }

    if ((player->pFlags & UINT32_C(0x400)) != 0 &&
        brain_GroundControl(
            cpad, player, target) != 0) {
        return;
    }
    if (player->pMainCallBack != NULL) {
        if ((player->pFlags &
             UINT32_C(0x44000601)) != 0) {
            return;
        }
        rtn = player->pMainCallBack(cpad, player);
        if (rtn == 1) {
            player->pMainCallBack = NULL;
            return;
        }
        if (rtn == -1) {
            return;
        }
    }

    if (AI_ON != 0 ||
        GameStruct.CurrentLevel == 8 ||
        (player->pFlags &
         UINT32_C(0x44000800)) != 0) {
        return;
    }

    combo_CheckHeldPad(
        cpad,
        player,
        INT32_C(0xffff0fff),
        AI_ON + 3);
    combo_CheckHeldPad(
        cpad,
        player,
        INT32_C(0x0000f000),
        AI_ON + 0x0c);
    if (((uint32_t)cpad[1] &
         (uint32_t)gaButtonMap[
             OptionStruct.ControllerConfig[0]][4]) != 0 &&
        (player->playerID < 9 ||
         (player->playernum < 2 &&
          extracharacter_CanForcePower(
              (model_id)player->playerID))) &&
        force_gActivate(cpad, player) != 0) {
        return;
    }
    combo_ReadCombo(cpad, player);
    (void)brain_LockOn(cpad, player);

    flags = player->pFlags;
    if ((flags & UINT32_C(0x0d00)) == 0) {
        if (combo_CheckCombo(cpad, player) == 1) {
            brain_control_combo_target(player);
        }
        flags = player->pFlags;
        if ((flags & UINT32_C(0x00200000)) != 0) {
            return;
        }
    }
    player->pFlags = flags & UINT32_C(0xffffffdf);

    /* Block shares the same executable-backed P1 scheme lookup as Force. */
    block_button =
        (uint32_t)gaButtonMap[
            OptionStruct.ControllerConfig[0]][3];
    if (((uint32_t)cpad[1] & block_button) != 0 ||
        (((uint32_t)cpad[0] & block_button) != 0 &&
         (flags & UINT32_C(0x8000)) == 0)) {
        brain_control_block(player);
        return;
    }

    if (player->currentMotion == 21) {
        animutl_SetCurrentLock(
            &player->playerRoot, 0);
    }
    player->pFlags &= UINT32_C(0xfffffeff);
    if (brain_control_motion_allows_input(player)) {
        uint32_t held = (uint32_t)cpad[1];
        uint32_t pressed = (uint32_t)cpad[0];
        uint32_t directions =
            held & UINT32_C(0x0000f000);
        uint32_t player_map =
            (uint32_t)gaButtonMap[
                player->playernum][0];

        if (directions == 0 &&
            (pressed & UINT32_C(0x20)) == 0 &&
            (player_map & held &
             UINT32_C(0x0000f000)) == 0) {
            brain_control_idle(player);
            return;
        }
        if ((pressed & UINT32_C(0x20)) != 0 &&
            brain_control_start_jump(
                cpad, player, physics)) {
            return;
        }
        if ((held &
             (player_map |
              UINT32_C(0x0000f020))) != 0) {
            brain_control_direction(
                cpad, player, physics);
            return;
        }
    }
    brain_control_idle(player);
}
