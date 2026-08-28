/*
 * REVIEWED RECONSTRUCTION.
 *
 * Provenance for brainutl_AddSabreEdge, brainutl_ConformGeomNodes,
 * brainutl_FindLSB, brainutl_Land, brainutil_PlotMaulTrajectory, and
 * brainutil_PlotTrajectory:
 *   direct     - name, signature, player/chain/p local names, source module,
 *                and dependent symbol names from the exact matching PDB.
 *   decompiled - branch structure from the matching game.exe.
 *   assembly   - PauseControl's pad flags and zero return checked at
 *                0x20560..0x20635; fall-duration threshold,
 *                energy/achievement/death effects,
 *                motion selection, flags, ground delay, and animation calls
 *                checked across RVA 0x21410..0x2158D; exact Maul and ordinary
 *                trajectory callbacks checked across 0x20640..0x20B9D;
 *                saber-edge sampling checked at 0x20C30..0x20C7E; geometry
 *                scaling and controller/keyboard cheat ownership checked at
 *                0x20C80..0x21287; the 16-bit and 32-bit LSB leaves checked
 *                at 0x212D0 and 0x212F0; motion sound checked at 0x21700; and
 *                nearest target selection checked at 0x21740..0x218AA.
 *
 * PDB module: 0010
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\brainutl.obj
 * Primary source: W:\SWJediPowerBattles\Work\brainutl.c
 * Compiler language: c
 * Emitted procedures: 16
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/brainutl.h"

#include "jpb/achievement.h"
#include "jpb/animctrl.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/world.h"
#include "jpb/whook.h"

#include <string.h>

/*
 * Exact linked globals from brainutl.obj. The scale table values were checked
 * against initialized image data at matched-PC RVA 0x4A6C70.
 */
NodeScale BigHandsFeetNodeScale[8] = {
    {2, {5500, 5500, 5500, 0}},
    {5, {5500, 5500, 5500, 0}},
    {3, {6200, 6200, 6200, 0}},
    {6, {6200, 6200, 6200, 0}},
    {10, {5500, 5500, 5500, 0}},
    {14, {5500, 5500, 5500, 0}},
    {11, {6200, 6200, 6200, 0}},
    {15, {6200, 6200, 6200, 0}}
};
int32_t cheat_bigHeadPressed[2];
int32_t cheat_bigHeadKeyPressed;
int32_t cheat_smallModeKeyPressed;
int32_t cheat_bigHead[2];
int32_t cheat_smallModePressed[2];
int32_t cheat_smallMode[2];
int32_t cheat_bigFeetAndSaberPressed[2];
int32_t cheat_bigFeetAndSaberKeyPressed;
int32_t cheat_bigFeetAndSaber[2];

static JPBBrainutlCheatChordProvider brainutl_cheat_chord_provider;
static void *brainutl_cheat_chord_user_data;

void jpb_BrainutlSetCheatChordProvider(
    JPBBrainutlCheatChordProvider provider,
    void *user_data)
{
    brainutl_cheat_chord_provider = provider;
    brainutl_cheat_chord_user_data = user_data;
}

/* 0x20560, 214 bytes, global, 2 named locals
 * brainutil_PauseControl
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
int brainutil_PauseControl(
    int32_t *cpad, playerObject *player)
{
    uint32_t pressed;
    uint32_t held;
    unsigned player_number;

    if (cpad == NULL || player == NULL) {
        return 0;
    }
    pressed = (uint32_t)cpad[0];
    held = (uint32_t)cpad[1];
    player_number = (uint16_t)player->playernum;

    if ((held & UINT32_C(0x100)) != 0) {
        (void)game_gSetGameFlags(
            UINT32_C(0x200000) << (player_number & 31U));
    }
    if ((held & UINT32_C(0x800)) != 0 ||
        (pressed & UINT32_C(0x800)) != 0) {
        (void)game_gSetGameFlags(
            UINT32_C(0x1000) << (player_number & 31U));
    }
    if ((held & UINT32_C(0x100)) != 0) {
        (void)game_gSetGameFlags(
            UINT32_C(0x200000) << (player_number & 31U));
    }
    if ((held & UINT32_C(0x800)) != 0 ||
        (pressed & UINT32_C(0x800)) != 0 ||
        (KeyPressed(0x20) != 0 && player_number == 0)) {
        (void)game_gSetGameFlags(
            UINT32_C(0x1000) << (player_number & 31U));
    }
    return 0;
}

/* 0x20640, 286 bytes, global, 3 named locals
 * brainutil_PlotMaulTrajectory
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
int brainutil_PlotMaulTrajectory(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene;
    physicsObject *physics;

    (void)cpad;
    if (player == NULL ||
        player->playerRoot.pParent == NULL) {
        return -1;
    }
    scene =
        (sceneObject *)player->playerRoot.pParent;
    physics =
        (physicsObject *)scene->pPhysics;
    if (physics == NULL) {
        return -1;
    }

    if ((player->currentMotion == 4 ||
         player->currentMotion == 22) &&
        (player->pFlags & UINT32_C(8)) == 0 &&
        physics->airTime > 0x7800 &&
        physics->airGround <= 0.0f) {
        physics->airTime = 0;
        physics->realAirTime = 0;
        if ((player->pFlags &
             UINT32_C(0x08000000)) != 0 &&
            player->target != NULL) {
            (void)physics_gForceFaceTarget(
                &player->playerRoot,
                &player->target->playerRoot);
        }
        brain_SetTrajectory(
            player,
            player->pSettings.dblJumpVel,
            player->pSettings.dblJumpAngle);
        if (player->paMotions != NULL &&
            player->maxMotions > 22) {
            (void)animctrl_MotionNoLock(
                &player->playerRoot,
                &player->paMotions[22]);
            player
                ->paMotions[22]
                .motionFlags |=
                UINT32_C(0x04000000);
        }
        player->pMotionCallBack = funcArray[48];
    }

    if (physics->airmov.vy <= 0.0f &&
        physics->airmov.vy != 0.0f) {
        physics->airTime += gGlobalFrameRate;
        physics->realAirTime +=
            gGlobalFrameRate;
    }
    if (physics->airTime > 0xc800) {
        physics->airTime = 0xc800;
    }
    return -1;
}

/* 0x20760, 1086 bytes, global, 7 named locals
 * brainutil_PlotTrajectory
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
int brainutil_PlotTrajectory(
    int32_t *cpad, playerObject *player)
{
    static const uint64_t special_air_ids =
        UINT64_C(0x0008000044000000);
    sceneObject *scene;
    physicsObject *physics;
    animObject *animation;
    Motion *current_motion;
    uint16_t player_id;
    int recovery_motion;
    int special_air_motion = 0;

    if (player == NULL ||
        player->playerRoot.pParent == NULL) {
        return 1;
    }
    scene =
        (sceneObject *)player->playerRoot.pParent;
    physics =
        (physicsObject *)scene->pPhysics;
    animation =
        (animObject *)scene->pAnim;
    current_motion =
        player->pMotion != NULL
            ? *player->pMotion
            : NULL;
    if (physics == NULL) {
        return 1;
    }

    recovery_motion =
        (player->pFlags &
         UINT32_C(0x8000)) != 0
            ? 4
            : 49;
    if (player->currentMotion == 3) {
        return -1;
    }

    player_id = (uint16_t)player->playerID;
    if (current_motion == NULL ||
        ((current_motion->motionFlags &
          UINT32_C(0x40000000)) == 0 &&
         (player_id > 51 ||
          ((special_air_ids >>
            (player_id & 63)) &
           UINT64_C(1)) == 0 ||
          (uint16_t)(
              player->currentMotion - 49) >
              1))) {
        physics_gClrConstantVector(
            &player->playerRoot);
        return 1;
    }

    if (player->playernum <= 1 &&
        (player->currentMotion == 4 ||
         player->currentMotion == 22) &&
        (player->pFlags & UINT32_C(8)) == 0 &&
        cpad != NULL) {
        uint32_t direction =
            (uint32_t)cpad[1];
        int desired_facing = -1;
        int base;

        if ((direction & UINT32_C(0x2000)) != 0) {
            base =
                (direction & UINT32_C(0x1000)) != 0
                    ? -0x200
                    : -0x400;
            if ((direction &
                 UINT32_C(0x4000)) != 0) {
                base -= 0x200;
            }
            desired_facing =
                base - mCameraAngleDest + 0x800;
        } else if ((direction &
                    UINT32_C(0x8000)) != 0) {
            base =
                (direction & UINT32_C(0x1000)) != 0
                    ? 0x200
                    : 0x400;
            if ((direction &
                 UINT32_C(0x4000)) != 0) {
                base += 0x200;
            }
            desired_facing =
                base - mCameraAngleDest + 0x800;
        } else if ((direction &
                    UINT32_C(0x1000)) != 0) {
            desired_facing =
                0x800 - mCameraAngleDest;
        } else if ((direction &
                    UINT32_C(0x4000)) != 0) {
            desired_facing =
                -mCameraAngleDest;
        }
        if (desired_facing != -1) {
            physics_gTurnToFace(
                &player->playerRoot,
                desired_facing,
                2);
        }

        if (((uint32_t)cpad[0] &
             UINT32_C(0x20)) != 0 &&
            (player->currentMotion == 4 ||
             OptionStruct.JumpCheat == 0)) {
            if (gGlobalTimer <
                (uint32_t)physics->falltimer +
                    UINT32_C(0x1000)) {
                brain_SetTrajectory(
                    player,
                    player->pSettings.JumpVel,
                    player->pSettings.JumpAngle);
                physics->airTime = 0;
                physics->realAirTime = 0;
                physics->falltimer = 0;
            } else if (
                physics->airTime < 0x11000 ||
                OptionStruct.JumpCheat == 0) {
                if ((direction &
                     UINT32_C(0xf000)) == 0) {
                    brain_SetTrajectory(
                        player,
                        player
                            ->pSettings
                            .dblJumpVel,
                        0x3ff);
                } else {
                    brain_SetTrajectory(
                        player,
                        player->pSettings.JumpVel,
                        player
                            ->pSettings
                            .dblJumpAngle);
                }
                if (player->paMotions != NULL &&
                    player->maxMotions > 22) {
                    (void)animctrl_MotionNoLock(
                        &player->playerRoot,
                        &player->paMotions[22]);
                }
            }
            player->pMotionCallBack = funcArray[6];
            if (player->paMotions != NULL &&
                player->maxMotions > 22) {
                player
                    ->paMotions[22]
                    .motionFlags |=
                    UINT32_C(0x04000000);
            }
        }

        if ((direction & UINT32_C(0xf000)) != 0 &&
            player->airAngle == 0x400) {
            physics->airmov.vz = 12.0f;
        }
    }

    if (physics->airmov.vy <= 0.0f &&
        physics->airmov.vy != 0.0f &&
        player->playerID != 75 &&
        player->playerID != 78) {
        physics->airTime += gGlobalFrameRate;
        physics->realAirTime +=
            gGlobalFrameRate;
    }
    if ((OptionStruct.JumpCheat != 0 ||
         player->playernum > 1) &&
        player->currentMotion != 49 &&
        physics->airTime > 0x167ff &&
        player->paMotions != NULL &&
        player->maxMotions >
            recovery_motion) {
        (void)animctrl_MotionNoLock(
            &player->playerRoot,
            &player
                 ->paMotions[recovery_motion]);
        player->pMotionCallBack = funcArray[6];
    }

    if (player_id <= 51 &&
        ((special_air_ids >>
          (player_id & 63)) &
         UINT64_C(1)) != 0) {
        special_air_motion =
            (uint16_t)(
                player->currentMotion - 49) <
            2;
    }
    if (player->currentMotion ==
            recovery_motion ||
        special_air_motion) {
        int distance =
            (int)(
                physics->airGround -
                physics->pos.vy);

        if (distance < 0) {
            distance = -distance;
        }
        if (distance > 0x7ff &&
            physics->realAirTime > 0x167ff) {
            if (animation != NULL) {
                sound_StopSound(
                    animation->loopHandle[0]);
                sound_StopSound(
                    animation->loopHandle[1]);
            }
            player->pFlags |=
                UINT32_C(0x200);
            (void)game_gModEnergy(
                player->playernum, -255);
            if (player->playernum < 2) {
                (void)game_gSetGameFlags(
                    UINT32_C(0x20) <<
                    ((uint32_t)player
                         ->playernum &
                     31));
                player_AfterLife(player);
                obj_gSetObjectFlag(
                    &player->playerRoot,
                    0,
                    UINT32_C(0x20));
                if (player->playernum == 0) {
                    achievement_complete(3);
                }
            } else if (
                player->pEnemy != NULL) {
                if (player->pEnemy->pPlace !=
                        NULL &&
                    player
                            ->pEnemy
                            ->pPlace
                            ->aiDf
                            .ownerType == 2) {
                    ++gDeathCount;
                }
                player->pEnemy->exit_flag = 1;
            }
            return 1;
        }
    }
    return -1;
}

/* 0x20BA0, 109 bytes, global, 4 named locals
 * brainutil_ReverseCheck
 * PDB type: int (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
int brainutil_ReverseCheck(playerObject *player)
{
    int shouldbe =
        physics_gFaceTarget(
            &player->playerRoot,
            &player->target->playerRoot);
    int face =
        physics_gGetFacing(&player->playerRoot);
    int delta = face - shouldbe;

    if (delta < 0) {
        delta = -delta;
    }
    delta &= 0xfff;
    return delta >= 0x601 && delta <= 0x9ff;
}

/* 0x20C10, 17 bytes, global, 4 named locals
 * brainutil_limitRange
 * PDB type: void (int*, int, int)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
void brainutil_limitRange(
    int *input, int min, int max)
{
    if (*input < min) {
        *input = min;
    }
    if (*input > max) {
        *input = max;
    }
}

/* 0x20C30, 69 bytes, global, 3 named locals
 * brainutl_AddSabreEdge
 * PDB type: int (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
int brainutl_AddSabreEdge(int playerID, int w0, int w1)
{
    (void)coll_GetNodeCenter(playerID, w0);
    (void)coll_GetNodeCenter(playerID, w1);
    (void)coll_GetNodeVelocity(playerID, w0);
    /* The emitted function leaves the final pointer's low word in EAX. */
    return (int)(uint32_t)(uintptr_t)
        coll_GetNodeVelocity(playerID, w1);
}

/* 0x20C80, 1543 bytes, global, 12 named locals
 * brainutl_ConformGeomNodes
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
void brainutl_ConformGeomNodes(playerObject *player)
{
    static const VECTOR small = {0x500, 0x500, 0x500, 0};
    static const VECTOR normal = {0x78a, 0x78a, 0x78a, 0};
    sceneObject *scene;
    modelObject *model;
    Mnode *head;
    uint16_t padd;
    int player_number;
    size_t index;

    if (player == NULL) {
        return;
    }
    player_number = (uint16_t)player->playernum;
    if ((unsigned)player_number >= 2U) {
        return;
    }
    padd = (uint16_t)player->playerPad.cpad[1];

    head = coll_GetNode(player_number, 8);
    if (head != NULL) {
        int32_t head_scale =
            cheat_bigHead[player_number] == 1 ? 20000 : 4000;

        head->v3Scale.vx = head_scale;
        head->v3Scale.vy = head_scale;
        head->v3Scale.vz = head_scale;
        head->v3Scale.pad = 0;
        coll_SetNodeFlags(
            player_number,
            8,
            JPB_COLLISION_FLAG_SCALE_OVERRIDE);
    }

    scene = (sceneObject *)player->playerRoot.pParent;
    model = scene != NULL ? (modelObject *)scene->pModel : NULL;
    if (model != NULL) {
        if (cheat_smallMode[player_number] == 1) {
            model->v3Scale = small;
            player->fScale = 0x4fd;
            player->pSettings.minClosingDist = (int16_t)(
                ((int32_t)player->pSettings.minClosingDist * 0x4fd) /
                JPB_FIXED_ONE);
        } else {
            model->v3Scale = normal;
            player->fScale = 0x78a;
            player->pSettings.minClosingDist = 0x14;
        }
    }

    for (index = 0;
         index < sizeof(BigHandsFeetNodeScale) /
                     sizeof(BigHandsFeetNodeScale[0]);
         ++index) {
        const NodeScale *node_scale = &BigHandsFeetNodeScale[index];
        Mnode *node = coll_GetNode(
            player_number, (unsigned)node_scale->NodeID);

        if (node == NULL) {
            continue;
        }
        if (cheat_bigFeetAndSaber[player_number] == 1) {
            node->v3Scale = node_scale->Scale;
            coll_SetNodeFlags(
                player_number,
                node_scale->NodeID,
                JPB_COLLISION_FLAG_SCALE_OVERRIDE);
        } else {
            coll_ClrNodeFlags(
                player_number,
                node_scale->NodeID,
                JPB_COLLISION_FLAG_SCALE_OVERRIDE);
        }
    }

    if (WInput_IsKBM() == 0 || player_number > 0) {
        if (padd == UINT16_C(0x4028)) {
            if (cheat_bigFeetAndSaberPressed[player_number] == 0) {
                cheat_bigFeetAndSaber[player_number] ^= 1;
            }
            cheat_bigFeetAndSaberPressed[player_number] = 1;
            cheat_bigHeadPressed[player_number] = 0;
            cheat_smallModePressed[player_number] = 0;
        } else {
            cheat_bigFeetAndSaberPressed[player_number] = 0;
        }

        if (padd == UINT16_C(0x4048)) {
            if (cheat_bigHeadPressed[player_number] == 0) {
                cheat_bigHead[player_number] ^= 1;
            }
            cheat_bigHeadPressed[player_number] = 1;
            cheat_smallModePressed[player_number] = 0;
        } else {
            cheat_bigHeadPressed[player_number] = 0;
            if (padd == UINT16_C(0x400a)) {
                if (cheat_smallModePressed[player_number] == 0) {
                    cheat_smallModePressed[player_number] = 1;
                    cheat_smallMode[player_number] ^= 1;
                }
            } else {
                cheat_smallModePressed[player_number] = 0;
            }
        }
    }
    /* The retail P1 path polls these raw keys after either input branch. */
    if (player_number == 0) {
        uint32_t key_state =
            brainutl_cheat_chord_provider != NULL
                ? brainutl_cheat_chord_provider(
                      brainutl_cheat_chord_user_data)
                : 0;

        if ((key_state & JPB_BRAINUTL_CHEAT_BIG_HEAD) != 0) {
            if (cheat_bigHeadKeyPressed == 0) {
                cheat_bigHead[player_number] ^= 1;
            }
            cheat_bigHeadKeyPressed = 1;
        } else {
            cheat_bigHeadKeyPressed = 0;
        }
        if ((key_state &
             JPB_BRAINUTL_CHEAT_BIG_FEET_AND_SABER) != 0) {
            if (cheat_bigFeetAndSaberKeyPressed == 0) {
                cheat_bigFeetAndSaber[player_number] ^= 1;
            }
            cheat_bigFeetAndSaberKeyPressed = 1;
        } else {
            cheat_bigFeetAndSaberKeyPressed = 0;
        }
        if ((key_state & JPB_BRAINUTL_CHEAT_SMALL_MODE) != 0) {
            if (cheat_smallModeKeyPressed == 0) {
                cheat_smallMode[player_number] ^= 1;
            }
            cheat_smallModeKeyPressed = 1;
        } else {
            cheat_smallModeKeyPressed = 0;
        }
    }
}

/* 0x21290, 18 bytes, global, 1 named locals
 * brainutl_DeltaTime
 * PDB type: unsigned long (unsigned long)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
uint32_t brainutl_DeltaTime(uint32_t time)
{
    return time == 0
        ? gGlobalTimer
        : gGlobalTimer - time;
}

/* 0x212B0, 27 bytes, global, 4 named locals
 * brainutl_ElapsedTime
 * PDB type: unsigned long (unsigned long, un...
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
uint32_t brainutl_ElapsedTime(
    uint32_t time, uint32_t duration)
{
    if (time != 0 &&
        duration >= (uint32_t)totalframes - time) {
        return 0;
    }
    return (uint32_t)totalframes;
}

/* 0x212D0, 30 bytes, global, 3 named locals
 * brainutl_FindLSB
 * PDB type: int (unsigned long)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
int brainutl_FindLSB(uint32_t flag)
{
    return brainutl_FindLSB_LV((uint16_t)flag);
}

/* 0x212F0, 29 bytes, global, 2 named locals
 * brainutl_FindLSB_LV
 * PDB type: int (unsigned long)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
int brainutl_FindLSB_LV(uint32_t flag)
{
    int b;

    if (flag == 0) {
        return 0;
    }
    b = 1;
    while ((flag & UINT32_C(1)) == 0) {
        flag >>= 1;
        ++b;
    }
    return b;
}

/* 0x21310, 249 bytes, global, 6 named locals
 * brainutl_HeldPad
 * PDB type: void (playerObject*, long*)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
static const char *const jpb_pad_motion_name[
    JPB_PLAYER_HELD_SLOTS] = {
    "", "", "", "", "n", "e", "s", "w",
    "", "", "", "", "", "", "", ""
};

void brainutl_HeldPad(
    playerObject *player, int32_t *cpad)
{
    char chbuf[JPB_PLAYER_MOTION_NAME_BYTES] = {0};
    uint32_t flag = (uint32_t)cpad[1] & UINT32_C(0xffff);

    while (flag != 0) {
        int b = brainutl_FindLSB_LV(flag);

        flag &= ~(UINT32_C(1) << (b - 1));
        strcat(chbuf, jpb_pad_motion_name[b - 1]);
        if (flag != 0) {
            strcat(chbuf, "+");
        }
    }
    strncpy(
        player->HeldMotion,
        chbuf,
        JPB_PLAYER_MOTION_NAME_BYTES);
}

/* 0x21410, 382 bytes, global, 3 named locals
 * brainutl_Land
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
void brainutl_Land(playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *p =
        (physicsObject *)scene->pPhysics;
    int chain = 0;
    int motion;

    if (player->playerID != 9 &&
        player->playerID != 43 &&
        p->airTime >= 0x16800) {
        (void)game_gModEnergy(player->playernum, -255);
        if (player->playernum == 0) {
            achievement_complete(3);
        }
        if (player->pEnemy != NULL &&
            player->pEnemy->pPlace != NULL &&
            player->pEnemy->pPlace->aiDf.ownerType == 2) {
            ++gDeathCount;
        }
    }
    p->airTime = 0;

    if (player->playerID == 9 ||
        player->playerID == 43 ||
        player->currentMotion == 4 ||
        player->currentMotion == 22 ||
        (((player->pMotion[0]->motionFlags &
           UINT32_C(0x40)) != 0) &&
         game_gGetEnergy(player->playernum) > 0)) {
        motion =
            (player->pFlags & UINT32_C(0x100)) != 0
                ? 2
                : 5;
    } else if (player->currentMotion == 76) {
        motion = 77;
    } else {
        player->groundDelay =
            gGlobalTimer + UINT32_C(0x1e00);
        motion = 53;
        player->pFlags |= UINT32_C(0x400);
        chain = 59;
    }

    player->pFlags &= ~UINT32_C(1);
    if ((player->pFlags & UINT32_C(0x8000)) != 0) {
        motion = 5;
        chain = 0;
    }

    (void)animctrl_MotionNoLock(
        &player->playerRoot,
        &player->paMotions[motion]);
    if (chain != 0) {
        (void)animctrl_MotionChain(
            &player->playerRoot,
            &player->paMotions[chain]);
    }
}

/* 0x21590, 362 bytes, global, 6 named locals
 * brainutl_MultiPad
 * PDB type: void (playerObject*, long*)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
void brainutl_MultiPad(
    playerObject *player, int32_t *cpad)
{
    char cpbuf[JPB_PLAYER_MOTION_NAME_BYTES] = {0};
    size_t length = strlen(player->PreMotion);
    uint32_t flag;

    if (length >= JPB_PLAYER_MOTION_NAME_BYTES - 1) {
        return;
    }

    flag = (uint32_t)cpad[0] & UINT32_C(0xffff);
    /* Combo text uses the same shared P1 Force binding as the retail brain. */
    if (((uint32_t)gaButtonMap[
             OptionStruct.ControllerConfig[0]][4] &
         (uint32_t)cpad[0]) != 0) {
        strcat(cpbuf, "f");
    }

    while (flag != 0) {
        int b = brainutl_FindLSB_LV(flag);
        const char *bpad;

        flag &= ~(UINT32_C(1) << (b - 1));
        bpad = jpb_pad_motion_name[b - 1];
        strcat(cpbuf, bpad);
        if (flag != 0 && bpad[0] != '\0') {
            strcat(cpbuf, "+");
        }
    }

    strncat(
        player->PreMotion,
        cpbuf,
        JPB_PLAYER_MOTION_NAME_BYTES - length);
}

/* 0x21700, 61 bytes, global, 3 named locals
 * brainutl_PlayMotionSound
 * PDB type: void (int, char*, int)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
void brainutl_PlayMotionSound(
    int playernum, char *sound, int delay)
{
    int bank;

    (void)delay;
    if (sound == NULL ||
        sound[0] == '\0' ||
        sound[0] == '0') {
        return;
    }
    bank = playernum + 1;
    if (bank > 3) {
        bank = 3;
    }
    (void)sound_Play(
        &maPhysicsData[playernum].vpos,
        bank,
        sound,
        0);
}

/* 0x21740, 363 bytes, global, 8 named locals
 * brainutl_gGetNearestTarget
 * PDB type: objectRoot* (objectRoot*, int)
 * Source: W:\SWJediPowerBattles\Work\brainutl.c
 */
objectRoot *brainutl_gGetNearestTarget(
    objectRoot *object0, int type)
{
    objectRoot *t = NULL;
    int range = 0x2000;
    int start =
        GameStruct.versusModeFlag == 1 ? 0 : 2;
    sceneObject *source_scene =
        (sceneObject *)object0->pParent;
    physicsObject *source_physics =
        (physicsObject *)source_scene->pPhysics;
    int i;

    for (i = start; i < JPB_PHYSICS_CAPACITY; ++i) {
        physicsObject *candidate = &maPhysicsData[i];
        objectRoot *candidate_root =
            &candidate->physicsRoot;
        sceneObject *candidate_scene;
        playerObject *player;
        modelObject *model;
        int r;
        int vertical_delta;

        if (object0->objectID == i ||
            candidate_root->objectID == -1 ||
            obj_gCheckObjectFlag(
                candidate_root, 0, UINT32_C(0x20)) != 0) {
            continue;
        }

        candidate_scene =
            (sceneObject *)candidate_root->pParent;
        player =
            (playerObject *)candidate_scene->pPlayer;
        model = (modelObject *)((sceneObject *)
            player->playerRoot.pParent)->pModel;
        if ((model->flags & UINT32_C(4)) != 0 &&
            (player->pFlags & UINT32_C(0x2000)) == 0) {
            continue;
        }
        if (GameStruct.CurrentLevel == UINT8_C(20) &&
            player->playerID == 0x48) {
            continue;
        }
        if (game_gGetEnergy(player->playernum) <= 0) {
            continue;
        }
        if (player->pEnemy != NULL &&
            player->pEnemy->pPlace->aiDf.ownerType != type) {
            continue;
        }

        r = physics_gGetRange(object0, candidate_root);
        if (r <= 0 || r >= range) {
            continue;
        }
        vertical_delta =
            (int)(source_physics->pos.vy -
                  candidate->pos.vy);
        if (vertical_delta < 0) {
            vertical_delta = -vertical_delta;
        }
        if (vertical_delta <= 0x100) {
            t = candidate_root;
            range = r;
        }
    }
    return t;
}
