/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\anim.c.
 *
 * The reviewed surface establishes the exact motion/frame records, fixed
 * animation queue, motion-to-physics handoff, raw/delta/pre-roll decode,
 * tween and frame transitions, sound scheduling, global animation pass,
 * resource-table bootstrap, and animation console mutations. All 17 emitted
 * PDB procedures now have reviewed bodies; descriptive `jpb_` helpers expose
 * bounded state/decode stages for tests and portable callers.
 *
 * Provenance:
 *   direct     - names and Motion/_animFrame layouts from the exact PDB.
 *   decompiled - handoff control flow checked against the raw Ghidra export.
 *   assembly   - queue stores/branches checked at RVA 0x17750..0x17886;
 *                activation state checked at RVA 0x17DCA..0x17F58; Motion
 *                member offsets and calls checked at RVAs
 *                0x17F13..0x17F37; frame buffer/depack selection, delta
 *                accumulation widths, events, and endpoint loop checked at
 *                RVAs 0x17FF0..0x184EE; tween fractions, delta stores, and
 *                countdown checked at RVAs 0x179E0..0x17D43; transition
 *                branches checked at RVAs 0x18580..0x188BD; animation sound
 *                timers, bank selection, loop handles, and replay vetoes
 *                checked at RVAs 0x188D0..0x1940F and 0x19580..0x1965F;
 *                pre-roll, global scheduling, bootstrap, and commands at
 *                RVAs 0x184F0, 0x18D40..0x19578.
 *
 * PDB module: 0004
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\anim.obj
 * Primary source: W:\SWJediPowerBattles\Work\anim.c
 * Compiler language: c
 * Emitted procedures: 17
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/anim.h"
#include "jpb/animctrl.h"
#include "jpb/brainutl.h"
#include "jpb/camera.h"
#include "jpb/combo.h"
#include "jpb/game.h"
#include "jpb/huffman.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/resources.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/unpack.h"
#include "jpb/world.h"

#include <stdio.h>
#include <string.h>

/* Direct globals at RVAs 0x4DC680 and 0x4E8980. */
animObject maAnimationData[JPB_ANIMATION_CAPACITY];
static animListNode
    aAnimListNodes[JPB_ANIMATION_CAPACITY][JPB_ANIM_QUEUE_NODE_CAPACITY];
static JPBHuffmanTableSet animationHuffmanTables;

/*
 * Exact PDB global name at RVA 0x538390. Its PDB type is unresolved; this
 * reviewed use covers the 33 _svector entries required by animation decode.
 */
static _svector gaScratch[JPB_ANIM_JOINT_CAPACITY + 1];

static int anim_CreateTweenFrame(animObject *animation);
static int anim_GoNextAnimFrame(animObject *animation);
static _dpcontext *anim_GetDecodeContext(animObject *animation);
static void anim_HandleSound(
    animObject *animation, int channel);
static void anim_SoundStart(
    playerObject *player,
    animObject *animation,
    int channel);

/* 0x17750, 310 bytes, global, 5 named locals
 * anim_AddNextAnimSeq
 * PDB type: int (animObject*, Motion*, int)
 * Source: W:\SWJediPowerBattles\Work\anim.c
 */
int anim_AddNextAnimSeq(
    animObject *animation, Motion *motion, int replace_queue)
{
    sceneObject *scene =
        (sceneObject *)animation->animRoot.pParent;
    playerObject *player =
        (playerObject *)scene->pPlayer;
    _animTemplate *templates;
    animListNode *node;
    uint32_t motion_flags;

    motion_flags = motion->motionFlags;
    if ((int)motion->Seq < player->maxMotions) {
        if ((motion_flags & 0x00000020u) == 0) {
            templates = animation->depack_context.seqdata;
        } else {
            sceneObject *target_scene =
                (sceneObject *)player->target->playerRoot.pParent;
            animObject *target_animation =
                (animObject *)target_scene->pAnim;

            templates = target_animation->depack_context3.seqdata;
        }
    } else {
        if ((motion_flags & 0x00000020u) == 0) {
            return -1;
        }
        {
            sceneObject *target_scene =
                (sceneObject *)player->target->playerRoot.pParent;
            animObject *target_animation =
                (animObject *)target_scene->pAnim;

            templates = target_animation->depack_context3.seqdata;
        }
    }

    if ((int32_t)motion_flags < 0 &&
        animation->pCurrentAnimSeq != NULL &&
        &templates[motion->Seq] ==
            animation->pCurrentAnimSeq->pAnimTemplate) {
        return -1;
    }

    node =
        (animListNode *)list_RemoveHead(&animation->animFreeList);
    if (node == NULL) {
        return -1;
    }
    node->pAnimTemplate = &templates[motion->Seq];
    if (replace_queue != 0) {
        list_MoveList(
            &animation->animFreeList, &animation->animList);
    }
    node->tweenLevel = motion->twin;
    node->Speed =
        motion->Speed == -1 ? JPB_FIXED_ONE : motion->Speed;
    node->pMotion = motion;
    if ((motion->motionFlags & 0x02000000u) == 0) {
        node->Lock = motion->Lock;
    } else {
        node->Lock = 0x1e;
        motion->motionFlags &= ~0x02000000u;
    }
    list_AddTail(&animation->animList, &node->anm_Node);
    return 0;
}

/* 0x17910, 208 bytes, global, 8 named locals
 * anim_CreateObject
 * PDB type: animObject* (void*, char*, void*...
 * Source: W:\SWJediPowerBattles\Work\anim.c
 */

animObject *anim_CreateObject(
    void *scene_data, char *animfile, void *reuse, int ID)
{
    sceneObject *scene = (sceneObject *)scene_data;
    animObject *animation = NULL;

    (void)ID;
    if (animfile != NULL) {
        int scene_id = scene->sceneRoot.objectID;
        int index;

        if ((uint32_t)scene_id < JPB_ANIMATION_CAPACITY &&
            maAnimationData[scene_id].animRoot.objectID == -1) {
            animation = &maAnimationData[scene_id];
            animation->animRoot.objectID = scene_id;
        } else {
            for (index = 0;
                 index < JPB_ANIMATION_CAPACITY;
                 ++index) {
                if (maAnimationData[index]
                        .animRoot.objectID == -1) {
                    animation = &maAnimationData[index];
                    animation->animRoot.objectID = index;
                    break;
                }
            }
            /*
             * The reference terminates the process when the pool is full.
             * Return NULL at this library boundary so a portable host can
             * report exhaustion without corrupting or killing its process.
             */
            if (animation == NULL) {
                return NULL;
            }
        }
        animation->paMotions =
            (Motion *)(animfile +
                       *(int32_t *)(animfile + 8));
        unpack_initcontext(
            &animation->depack_context, animfile);
        unpack_initcontext(
            &animation->depack_context3, animfile);
        animation->animFrameIndex = 0;
    } else {
        objectRoot *reuse_object =
            (objectRoot *)reuse;
        sceneObject *reuse_scene;

        if (reuse_object == NULL) {
            return NULL;
        }
        reuse_scene =
            (sceneObject *)reuse_object->pParent;
        animation =
            (animObject *)reuse_scene->pAnim;
    }
    obj_gSetChildObject(
        scene, &animation->animRoot, 3);
    return animation;
}

void jpb_AnimApplyMotionPhysics(playerObject *player, const Motion *motion)
{
    physics_gSetCharge(
        player,
        (int)motion->Charge + (int)motion->vel,
        (int)motion->ChargeAcc);
    if ((motion->motionFlags & 0x00000008u) != 0) {
        physics_gSwapVel(player);
    }
}

static int32_t anim_arithmetic_shift12(int32_t value)
{
    if (value >= 0) {
        return value / JPB_FIXED_ONE;
    }
    return -1 -
           (int32_t)((-(int64_t)value - 1) / JPB_FIXED_ONE);
}

/* 0x17890, 114 bytes, local, 2 named locals
 * anim_CheckSlack
 * PDB type: int (animObject*, animListNode*)
 * Source: W:\SWJediPowerBattles\Work\anim.c
 */
static int anim_CheckSlack(
    animObject *animation, animListNode *sequence)
{
    int last_frame = sequence->pAnimTemplate->Lframe;
    int slack = animation->pMotion->disp;

    if (last_frame - 1 < slack) {
        slack = last_frame - 1;
    }
    if (slack > 0 &&
        last_frame - slack <
            anim_arithmetic_shift12(
                animation->animFrameIndex) &&
        animation->Lock > 22) {
        if (animation->Lock != 27) {
            animation->Lock = 22;
        }
        if (animation->animList.head == NULL) {
            return 1;
        }
    }
    return 0;
}

JPBAnimPartialResult jpb_AnimActivateQueuedMotionState(
    animObject *animation)
{
    const float animation_rate_scale = 1.1f;
    animListNode *next;
    Motion *motion;
    sceneObject *scene;
    playerObject *player;
    int32_t rate;
    int32_t product;

    if (animation == NULL ||
        animation->animRoot.pParent == NULL) {
        return JPB_ANIM_PARTIAL_INVALID_ARGUMENT;
    }
    scene =
        (sceneObject *)animation->animRoot.pParent;
    player = (playerObject *)scene->pPlayer;
    next = (animListNode *)animation->animList.head;
    if (player == NULL ||
        (next != NULL &&
         (next->pAnimTemplate == NULL ||
          next->pMotion == NULL))) {
        return JPB_ANIM_PARTIAL_INVALID_ARGUMENT;
    }
    if (next == NULL) {
        return JPB_ANIM_PARTIAL_EMPTY;
    }

    if (animation->pCurrentAnimSeq != NULL &&
        (uint16_t)(animation->pCurrentAnimSeq->Speed + 1) > 1) {
        physics_gModFacing(
            &animation->animRoot,
            animation->pCurrentAnimSeq->Speed);
    }
    if (animation->pCurrentAnimSeq != NULL) {
        list_AddTail(
            &animation->animFreeList,
            &animation->pCurrentAnimSeq->anm_Node);
    }

    next = (animListNode *)
        list_RemoveHead(&animation->animList);
    animation->pCurrentAnimSeq = next;
    if (next == NULL) {
        return JPB_ANIM_PARTIAL_EMPTY;
    }

    animation->animFrameIndex =
        (int32_t)((uint32_t)(int32_t)
                      next->pAnimTemplate->Fframe
                  << JPB_FIXED_SHIFT);
    if (animation->pMotion != NULL &&
        (next->pMotion->motionFlags & 0x00000001u) != 0) {
        animation->tweenLevel = animation->pMotion->twout;
    } else {
        animation->tweenLevel = (uint8_t)next->tweenLevel;
    }
    animation->dispIn = 0;

    rate = (int32_t)((float)next->Speed *
                     animation_rate_scale);
    motion = next->pMotion;
    if (motion->Damage != 0) {
        rate =
            (int32_t)((float)rate * animation_rate_scale);
    }
    animation->animFrameRate = rate;
    product =
        (int32_t)((uint32_t)rate *
                  (uint32_t)gGlobalFrameRate);
    animation->animFrameAcc =
        anim_arithmetic_shift12(product);
    animation->Lock = (uint16_t)next->Lock;
    animation->pMotion = motion;
    animation->tweenFramesLeft = 0;

    jpb_AnimApplyMotionPhysics(player, motion);
    player->previousMotion = player->currentMotion;
    player->currentMotion = (int16_t)motion->Seq;
    if (motion->FunctPtr >= 0 &&
        motion->FunctPtr <
            JPB_PLAYER_CALLBACK_CAPACITY) {
        player->pMotionCallBack =
            funcArray[motion->FunctPtr];
    } else {
        player->pMotionCallBack = NULL;
    }
    if (shouldPlayAnimSound(player->playerID, animation)) {
        anim_SoundStart(player, animation, 0);
        anim_SoundStart(player, animation, 1);
    }

    /*
     * Exact anim_ForceNextAnimSeq enters anim_CreateTweenFrame when a
     * nonzero tween is requested and Motion bit 28 does not suppress it.
     * Synthetic state-only callers deliberately omit a Huffman context;
     * those callers retain the bounded state transition and begin decoding
     * only after a real CAD stream is attached.
     */
    if (animation->tweenLevel != 0 &&
        (motion->motionFlags & UINT32_C(0x10000000)) == 0 &&
        anim_GetDecodeContext(animation) != NULL &&
        anim_GetDecodeContext(animation)->huffdataorigin != NULL &&
        next->pAnimTemplate->parts >= 0 &&
        next->pAnimTemplate->parts <=
            JPB_ANIM_JOINT_CAPACITY &&
        animation->pCurrentAnimFrame != NULL) {
        (void)anim_CreateTweenFrame(animation);
    }
    return JPB_ANIM_PARTIAL_OK;
}

/*
 * Exact PDB-local anim_MotionRecovery, matched-PC RVA 0x18B50.
 * This selects the normal idle, low-energy idle, or lock-on idle motion,
 * replaces any stale queue, resets combo state, and clears residual motion.
 */
static int anim_MotionRecovery(animObject *animation)
{
    sceneObject *scene =
        (sceneObject *)animation->animRoot.pParent;
    playerObject *player =
        (playerObject *)scene->pPlayer;
    int move = 0;
    Motion *motion;

    if ((player->pFlags & UINT32_C(0x00008000)) == 0) {
        if ((player->pFlags &
             UINT32_C(0x00400000)) == 0) {
            int maximum_energy =
                game_gGetMaxEnergy(player->playernum);
            int energy =
                game_gGetEnergy(player->playernum);

            if (energy < maximum_energy / 4) {
                move = 19;
            }
        } else {
            move = 20;
        }
    }
    player->hitDelay = 0;
    if ((player->pFlags & UINT32_C(0x00000401)) != 0) {
        return 1;
    }

    motion = &player->paMotions[move];
    if ((int)motion->Seq >= player->oldmaxCMotions &&
        (motion->motionFlags & UINT32_C(0x20)) == 0) {
        return 1;
    }
    if (anim_AddNextAnimSeq(animation, motion, 1) != 0) {
        return 1;
    }

    combo_ResetComboEngine(
        brainutl_ElapsedTime(0, 0),
        player);
    if ((player->pFlags & UINT32_C(0x01000000)) != 0) {
        player->pFlags &= UINT32_C(0xfcdfffff);
    }
    player->currentMotion = (int16_t)move;
    physics_gClrConstantVector(&animation->animRoot);
    return 0;
}

/* 0x17D50, 672 bytes, global, 4 named locals
 * anim_ForceNextAnimSeq
 * PDB type: int (animObject*, int)
 * Source: W:\\SWJediPowerBattles\\Work\\anim.c
 *
 * IsTween is present in the PDB signature but is not read by the optimized
 * matched body. The tween decision comes from the selected queue node and
 * Motion bit 28.
 */
int anim_ForceNextAnimSeq(animObject *animation, int IsTween)
{
    JPBAnimPartialResult result;
    int tween_requested;

    (void)IsTween;
    if (animation == NULL) {
        return 1;
    }
    if (animation->pCurrentAnimSeq != NULL &&
        animation->pCurrentAnimSeq->pMotion != NULL &&
        (animation->pCurrentAnimSeq->pMotion->motionFlags &
         UINT32_C(0x20000000)) != 0) {
        animation->animFlags |= UINT32_C(0x20000000);
    }
    animation->animFlags |= UINT32_C(0x20);

    if (animation->animList.head == NULL &&
        anim_MotionRecovery(animation) != 0) {
        return 1;
    }
    result = jpb_AnimActivateQueuedMotionState(animation);
    if (result == JPB_ANIM_PARTIAL_EMPTY) {
        return 0;
    }
    if (result != JPB_ANIM_PARTIAL_OK ||
        animation->pMotion == NULL) {
        return 1;
    }
    tween_requested =
        animation->tweenLevel != 0 &&
        (animation->pMotion->motionFlags &
         UINT32_C(0x10000000)) == 0;
    if (tween_requested &&
        (animation->animFlags & UINT32_C(0x40)) != 0 &&
        animation->pCurrentAnimFrame ==
            &animation->tweenAnimFrame) {
        return 0;
    }
    return anim_GoNextAnimFrame(animation);
}

JPBAnimPartialResult jpb_AnimAdvanceQueuedMotionAtEnd(
    animObject *animation)
{
    _animTemplate *sequence;
    int32_t current_frame;
    int32_t transition_frame;

    if (animation == NULL ||
        animation->pCurrentAnimSeq == NULL ||
        animation->pCurrentAnimSeq->pAnimTemplate == NULL ||
        animation->pMotion == NULL) {
        return JPB_ANIM_PARTIAL_INVALID_ARGUMENT;
    }
    sequence =
        animation->pCurrentAnimSeq->pAnimTemplate;
    current_frame =
        anim_arithmetic_shift12(
            animation->animFrameIndex);
    transition_frame =
        (int32_t)sequence->Lframe -
        (int32_t)animation->pMotion->cutout;
    if (current_frame < transition_frame) {
        return JPB_ANIM_PARTIAL_OK;
    }

    /*
     * Exact non-tween sequence-end branches from anim_GoNextAnimFrame,
     * matched-PC RVAs 0x1860D..0x1875D. Negative motionFlags loop back to
     * Fframe; bit 26 freezes on Lframe; other motions consume their queued
     * successor through the already-reviewed activation helper.
     */
    if ((int32_t)animation->pMotion->motionFlags < 0) {
        sceneObject *scene =
            (sceneObject *)animation->animRoot.pParent;
        playerObject *player = scene != NULL
            ? (playerObject *)scene->pPlayer
            : NULL;

        animation->animFrameIndex =
            (int32_t)sequence->Fframe *
            JPB_FIXED_ONE;
        if (player != NULL &&
            shouldReplayAnimSound(
                player->playerID, animation)) {
            anim_SoundStart(player, animation, 0);
            anim_SoundStart(player, animation, 1);
        }
        return JPB_ANIM_PARTIAL_OK;
    }
    if ((animation->pMotion->motionFlags &
         UINT32_C(0x04000000)) != 0) {
        animation->animFrameIndex =
            (int32_t)sequence->Lframe *
            JPB_FIXED_ONE;
        animation->animFrameAcc = 0;
        animation->animFlags |= UINT32_C(8);
        return JPB_ANIM_PARTIAL_OK;
    }
    if (animation->animList.head == NULL) {
        if (anim_MotionRecovery(animation) != 0) {
            return JPB_ANIM_PARTIAL_EMPTY;
        }
    }
    return jpb_AnimActivateQueuedMotionState(
        animation);
}

/* 0x18A40, 264 bytes, global, 2 named locals
 * anim_InitAnimations
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\anim.c
 */
/*
 * Dependency-light per-object reuse seam extracted from the exact
 * anim_InitAnimations loop body. This jpb_ name is not a PDB symbol.
 */
void jpb_AnimResetObjectSlot(int index)
{
    animObject *animation;
    int node_index;

    if ((uint32_t)index >= JPB_ANIMATION_CAPACITY) {
        return;
    }
    animation = &maAnimationData[index];
    memset(animation, 0, sizeof(*animation));
    animation->animRoot.objectID = -1;
    (void)snprintf(
        animation->animRoot.objectName,
        sizeof(animation->animRoot.objectName),
        "ANIM%d",
        index);
    animation->pCurrentAnimFrame =
        animation->AnimFrameBuffer;
    animation->pPreviousAnimFrame =
        animation->AnimFrameBuffer;
    animation->animFrameRate = JPB_FIXED_ONE;
    list_InitList(&animation->animList);
    list_InitList(&animation->animFreeList);
    for (node_index = 0;
         node_index < JPB_ANIM_QUEUE_NODE_CAPACITY;
         ++node_index) {
        list_AddTail(
            &animation->animFreeList,
            &aAnimListNodes[index][node_index].anm_Node);
    }
}

void anim_InitAnimations(int start)
{
    int index;

    /*
     * Valid original callers pass 0..19. Reject a negative start instead of
     * reproducing the reference's out-of-bounds pool walk.
     */
    if ((uint32_t)start >= JPB_ANIMATION_CAPACITY) {
        return;
    }
    for (index = start; index < JPB_ANIMATION_CAPACITY; ++index) {
        jpb_AnimResetObjectSlot(index);
    }
}

/* 0x17FF0, 1279 bytes, local, 9 named locals
 * anim_GetAnimFrame
 * PDB type: _animFrame* (animObject*)
 * Source: W:\SWJediPowerBattles\Work\anim.c
 */
static int16_t anim_wrap_signed_bits(int32_t value, unsigned bits)
{
    uint32_t mask = ((uint32_t)1 << bits) - 1u;
    uint32_t wrapped = (uint32_t)value & mask;

    if ((wrapped & ((uint32_t)1 << (bits - 1))) != 0) {
        wrapped |= ~mask;
    }
    return (int16_t)(int32_t)wrapped;
}

static int16_t anim_add_angle12(int16_t left, int16_t right)
{
    return anim_wrap_signed_bits(
        (int32_t)left + (int32_t)right, 12);
}

static int16_t anim_add_root_y13(int16_t left, int16_t right)
{
    return anim_wrap_signed_bits(
        (int32_t)left + (int32_t)right, 13);
}

static void anim_apply_first_delta(
    _animFrame *frame, const _svector *delta, int vector_count)
{
    _svector *pose = &frame->v3RootTranslation;
    _svector *stored_delta = &frame->v3RootTranslationDelta;
    int index;

    for (index = 0; index < vector_count; ++index) {
        pose[index].pad =
            (int16_t)((uint16_t)pose[index].pad ^
                      (uint16_t)delta[index].pad);
    }
    memcpy(
        stored_delta,
        delta,
        (size_t)vector_count * sizeof(*stored_delta));
    for (index = 0; index < vector_count; ++index) {
        pose[index].vx =
            anim_add_angle12(
                pose[index].vx, delta[index].vx);
        pose[index].vy =
            index == 0
                ? anim_add_root_y13(
                      pose[index].vy, delta[index].vy)
                : anim_add_angle12(
                      pose[index].vy, delta[index].vy);
        pose[index].vz =
            anim_add_angle12(
                pose[index].vz, delta[index].vz);
    }
}

static void anim_apply_later_delta(
    _animFrame *frame, const _svector *delta, int vector_count)
{
    _svector *pose = &frame->v3RootTranslation;
    _svector *stored_delta = &frame->v3RootTranslationDelta;
    int index;

    for (index = 0; index < vector_count; ++index) {
        pose[index].pad =
            (int16_t)((uint16_t)pose[index].pad ^
                      (uint16_t)delta[index].pad);
        stored_delta[index].vx =
            anim_add_angle12(
                stored_delta[index].vx, delta[index].vx);
        stored_delta[index].vy =
            anim_add_angle12(
                stored_delta[index].vy, delta[index].vy);
        stored_delta[index].vz =
            anim_add_angle12(
                stored_delta[index].vz, delta[index].vz);
        pose[index].vx =
            anim_add_angle12(
                pose[index].vx, stored_delta[index].vx);
        pose[index].vy =
            index == 0
                ? anim_add_root_y13(
                      pose[index].vy,
                      stored_delta[index].vy)
                : anim_add_angle12(
                      pose[index].vy,
                      stored_delta[index].vy);
        pose[index].vz =
            anim_add_angle12(
                pose[index].vz, stored_delta[index].vz);
    }
}

/* 0x18DB0, 964 bytes, local, 9 named locals
 * anim_SkipToStartFrame
 * PDB type: void (animObject*, _dpcontext*)
 * Source: W:\\SWJediPowerBattles\\Work\\anim.c
 *
 * Some authored streams carry pose deltas before their published start
 * frame. The reference consumes that pre-roll into the current frame buffer
 * without exposing its event bytes to gameplay.
 */
static void anim_SkipToStartFrame(
    animObject *animation, _dpcontext *context)
{
    _animTemplate *sequence =
        animation->pCurrentAnimSeq->pAnimTemplate;
    _animFrame *frame =
        &animation->AnimFrameBuffer[animation->curBufferId];
    int vector_count = (int)sequence->parts + 1;
    int pre_roll_count = sequence->Lframe;
    int current_frame =
        anim_arithmetic_shift12(animation->animFrameIndex);
    int index;

    if (sequence->pad1 <= 0 || current_frame > 0) {
        return;
    }
    if (sequence->pad1 < pre_roll_count) {
        pre_roll_count = sequence->pad1;
    }
    if (pre_roll_count <= 0 || current_frame != 0) {
        return;
    }

    for (index = 0; index < pre_roll_count; ++index) {
        if (index == 0) {
            unpack_grabsvectors_raw(
                context,
                vector_count,
                (int16_t *)&frame->v3RootTranslation);
        } else {
            unpack_grabsvectors_s(
                context,
                vector_count,
                (int16_t *)gaScratch);
            if (index == 1) {
                anim_apply_first_delta(
                    frame, gaScratch, vector_count);
            } else {
                anim_apply_later_delta(
                    frame, gaScratch, vector_count);
            }
        }
        animation->animFrameIndex += JPB_FIXED_ONE;
    }
}

static _dpcontext *anim_GetDecodeContext(animObject *animation)
{
    sceneObject *scene;
    playerObject *player;
    sceneObject *target_scene;
    animObject *target_animation;

    if ((animation->pMotion->motionFlags & UINT32_C(0x20)) == 0) {
        return &animation->depack_context;
    }
    scene = (sceneObject *)animation->animRoot.pParent;
    player = scene != NULL
        ? (playerObject *)scene->pPlayer
        : NULL;
    target_scene = player != NULL && player->target != NULL
        ? (sceneObject *)player->target->playerRoot.pParent
        : NULL;
    target_animation = target_scene != NULL
        ? (animObject *)target_scene->pAnim
        : NULL;
    return target_animation != NULL
        ? &target_animation->depack_context3
        : NULL;
}

static _animFrame *anim_GetAnimFrame(animObject *anim)
{
    _animFrame *pNewFrame;
    _animTemplate *sequence =
        anim->pCurrentAnimSeq->pAnimTemplate;
    _dpcontext *context = anim_GetDecodeContext(anim);
    int frame = anim_arithmetic_shift12(anim->animFrameIndex);
    int parts = sequence->parts;
    int vector_count = parts + 1;
    int count;
    int done;
    int index;

    if (frame == sequence->Fframe) {
        uint16_t previous_buffer = anim->curBufferId;

        anim->curBufferId ^= 1u;
        anim->pPreviousAnimFrame =
            &anim->AnimFrameBuffer[previous_buffer];
        unpack_seekcontext(
            context, (int)sequence->FframeAddr);
        /*
         * Matched instructions 0x140018090..0x1400180A4 read the byte at
         * Motion + 0x0A (`cutin`).  `disp` lives at +0x0C and belongs to
         * anim_CheckSlack; using it here skipped the opening 24 frames of
         * Ki-Adi's 19-frame south attack and made its authored combo prefix
         * impossible to retain.
         */
        count = (int)anim->dispIn + (int)anim->pMotion->cutin;
        if (count == 0) {
            count = 1;
        }
    } else {
        count = anim_arithmetic_shift12(anim->animFrameAcc);
    }

    done = (int)sequence->Lframe - frame;
    if (frame + count <= sequence->Lframe) {
        done = count;
    }
    anim->animFrameAcc -= done * JPB_FIXED_ONE;
    pNewFrame = &anim->AnimFrameBuffer[anim->curBufferId];

    if (done < 1) {
        if (parts > 0) {
            memset(pNewFrame->event, 0, (size_t)parts);
        }
        return pNewFrame;
    }

    anim_SkipToStartFrame(anim, context);
    frame = anim_arithmetic_shift12(anim->animFrameIndex);
    for (index = 0; index < done; ++index) {
        int joint;

        if (frame == 0) {
            unpack_grabsvectors_raw(
                context,
                vector_count,
                (int16_t *)&pNewFrame->v3RootTranslation);
        } else {
            unpack_grabsvectors_s(
                context,
                vector_count,
                (int16_t *)gaScratch);
            if (frame == 1) {
                anim_apply_first_delta(
                    pNewFrame, gaScratch, vector_count);
            } else {
                anim_apply_later_delta(
                    pNewFrame, gaScratch, vector_count);
            }
        }
        anim->animFrameIndex += JPB_FIXED_ONE;
        ++frame;

        for (joint = 0; joint < parts; ++joint) {
            uint8_t event =
                (uint8_t)pNewFrame->av3JointAngle[joint].pad;

            if (index == 0) {
                pNewFrame->event[joint] = (char)event;
            } else {
                pNewFrame->event[joint] =
                    (char)(
                        (uint8_t)pNewFrame->event[joint] |
                        event);
            }
        }
    }
    return pNewFrame;
}

static int16_t anim_tween_delta(
    int16_t destination, int16_t source, int32_t fraction)
{
    int16_t wrapped_difference =
        (int16_t)((uint16_t)destination - (uint16_t)source);
    int32_t product =
        (int32_t)wrapped_difference * fraction;

    if (product < 0) {
        product += JPB_FIXED_ONE - 1;
    }
    return (int16_t)anim_arithmetic_shift12(product);
}

static int16_t anim_tween_angle_delta12(
    int16_t destination, int16_t source, int32_t fraction)
{
    int16_t wrapped_difference = anim_wrap_signed_bits(
        (int32_t)destination - (int32_t)source, 12);
    int32_t product =
        (int32_t)wrapped_difference * fraction;

    if (product < 0) {
        product += JPB_FIXED_ONE - 1;
    }
    return (int16_t)anim_arithmetic_shift12(product);
}

static int16_t anim_add_tween_delta(
    int16_t value, int16_t delta)
{
    return (int16_t)((uint16_t)value + (uint16_t)delta);
}

static void anim_advance_tween_pose(animObject *animation)
{
    int part;

    animation->tweenAnimFrame.v3RootTranslation.vx =
        anim_add_tween_delta(
            animation->tweenAnimFrame.v3RootTranslation.vx,
            animation->tweenDeltaTranslation.vx);
    animation->tweenAnimFrame.v3RootTranslation.vy =
        anim_add_tween_delta(
            animation->tweenAnimFrame.v3RootTranslation.vy,
            animation->tweenDeltaTranslation.vy);
    animation->tweenAnimFrame.v3RootTranslation.vz =
        anim_add_tween_delta(
            animation->tweenAnimFrame.v3RootTranslation.vz,
            animation->tweenDeltaTranslation.vz);
    for (part = 0;
         part < animation->pCurrentAnimSeq->pAnimTemplate->parts;
         ++part) {
        _svector *pose =
            &animation->tweenAnimFrame.av3JointAngle[part];
        const _svector *delta =
            &animation->tweenDeltaRotFrame[part];

        pose->vx = anim_add_tween_delta(pose->vx, delta->vx);
        pose->vy = anim_add_tween_delta(pose->vy, delta->vy);
        pose->vz = anim_add_tween_delta(pose->vz, delta->vz);
    }
}

/* 0x179E0, 868 bytes, local, 3 named locals
 * anim_CreateTweenFrame
 * PDB type: int (animObject*)
 * Source: W:\SWJediPowerBattles\Work\anim.c
 */
static int anim_CreateTweenFrame(animObject *animation)
{
    static const int32_t tween_fraction[17] = {
        0, 4096, 2048, 1365, 1024, 819, 682, 585, 512,
        455, 409, 372, 341, 315, 292, 273, 256
    };
    _animFrame *destination;
    _animFrame *source;
    int32_t fraction;
    int part;

    if (animation->tweenLevel > 16) {
        animation->tweenLevel = 16;
    }
    fraction = tween_fraction[animation->tweenLevel];
    destination = anim_GetAnimFrame(animation);
    source = animation->pPreviousAnimFrame;
    if (destination == NULL || source == NULL) {
        return 1;
    }

    animation->animFlags |= UINT32_C(0x40);
    animation->tweenDeltaTranslation.vx = anim_tween_delta(
        destination->v3RootTranslation.vx,
        source->v3RootTranslation.vx,
        fraction);
    animation->tweenDeltaTranslation.vy = anim_tween_delta(
        destination->v3RootTranslation.vy,
        source->v3RootTranslation.vy,
        fraction);
    animation->tweenDeltaTranslation.vz = anim_tween_delta(
        destination->v3RootTranslation.vz,
        source->v3RootTranslation.vz,
        fraction);
    animation->tweenAnimFrame.v3RootTranslation.vx =
        anim_add_tween_delta(
            source->v3RootTranslation.vx,
            animation->tweenDeltaTranslation.vx);
    animation->tweenAnimFrame.v3RootTranslation.vy =
        anim_add_tween_delta(
            source->v3RootTranslation.vy,
            animation->tweenDeltaTranslation.vy);
    animation->tweenAnimFrame.v3RootTranslation.vz =
        anim_add_tween_delta(
            source->v3RootTranslation.vz,
            animation->tweenDeltaTranslation.vz);
    animation->tweenAnimFrame.v3RootTranslation.pad =
        destination->v3RootTranslation.pad;

    for (part = 0;
         part < animation->pCurrentAnimSeq->pAnimTemplate->parts;
         ++part) {
        const _svector *to = &destination->av3JointAngle[part];
        const _svector *from = &source->av3JointAngle[part];
        _svector *delta = &animation->tweenDeltaRotFrame[part];
        _svector *pose = &animation->tweenAnimFrame.av3JointAngle[part];

        /*
         * Exact anim_CreateTweenFrame joint loop at matched-PC RVAs
         * 0x17BB0..0x17BF9. The renderer consumes 12-bit rotations, so the
         * retail code sign-extends each destination-source difference from
         * 12 bits before scaling. Keeping the root-translation path above
         * at 16 bits is intentional and matches its separate instruction
         * sequence at 0x17A5A..0x17AFB.
         */
        delta->vx = anim_tween_angle_delta12(
            to->vx, from->vx, fraction);
        delta->vy = anim_tween_angle_delta12(
            to->vy, from->vy, fraction);
        delta->vz = anim_tween_angle_delta12(
            to->vz, from->vz, fraction);
        pose->vx = anim_add_tween_delta(from->vx, delta->vx);
        pose->vy = anim_add_tween_delta(from->vy, delta->vy);
        pose->vz = anim_add_tween_delta(from->vz, delta->vz);
        pose->pad = from->pad;
    }

    animation->pv3TweenDeltaTranslation =
        &animation->tweenDeltaTranslation;
    animation->tweenFramesLeft =
        (uint8_t)(animation->tweenLevel - 1);
    animation->pCurrentAnimFrame =
        &animation->tweenAnimFrame;
    animation->pv3TweenDeltaRotFrame =
        animation->tweenDeltaRotFrame;
    return 0;
}

JPBAnimPartialResult jpb_AnimCreateTweenFrameState(
    animObject *animation, _animFrame **tween_frame)
{
    _animTemplate *sequence;

    if (tween_frame != NULL) {
        *tween_frame = NULL;
    }
    if (animation == NULL ||
        tween_frame == NULL ||
        animation->pCurrentAnimSeq == NULL ||
        animation->pCurrentAnimSeq->pAnimTemplate == NULL ||
        animation->pMotion == NULL ||
        animation->pCurrentAnimFrame == NULL ||
        animation->tweenLevel == 0) {
        return JPB_ANIM_PARTIAL_INVALID_ARGUMENT;
    }
    sequence = animation->pCurrentAnimSeq->pAnimTemplate;
    if (sequence->parts < 0 ||
        sequence->parts > JPB_ANIM_JOINT_CAPACITY ||
        anim_GetDecodeContext(animation) == NULL ||
        anim_GetDecodeContext(animation)->huffdataorigin == NULL) {
        return JPB_ANIM_PARTIAL_UNSUPPORTED_STATE;
    }
    if (anim_CreateTweenFrame(animation) != 0) {
        return JPB_ANIM_PARTIAL_UNSUPPORTED_STATE;
    }
    *tween_frame = animation->pCurrentAnimFrame;
    return JPB_ANIM_PARTIAL_OK;
}

JPBAnimPartialResult jpb_AnimDecodeFrameState(
    animObject *animation, _animFrame **decoded_frame)
{
    _animTemplate *sequence;

    if (decoded_frame != NULL) {
        *decoded_frame = NULL;
    }
    if (animation == NULL ||
        decoded_frame == NULL ||
        animation->pCurrentAnimSeq == NULL ||
        animation->pCurrentAnimSeq->pAnimTemplate == NULL ||
        animation->pMotion == NULL) {
        return JPB_ANIM_PARTIAL_INVALID_ARGUMENT;
    }
    sequence = animation->pCurrentAnimSeq->pAnimTemplate;
    if (sequence->parts < 0 ||
        sequence->parts > JPB_ANIM_JOINT_CAPACITY ||
        anim_GetDecodeContext(animation) == NULL) {
        return JPB_ANIM_PARTIAL_UNSUPPORTED_STATE;
    }
    if (anim_GetDecodeContext(animation)->huffdataorigin == NULL) {
        return JPB_ANIM_PARTIAL_INVALID_ARGUMENT;
    }
    *decoded_frame = anim_GetAnimFrame(animation);
    return JPB_ANIM_PARTIAL_OK;
}

/* 0x18580, 837 bytes, local, 8 named locals
 * anim_GoNextAnimFrame
 * PDB type: int (animObject*)
 * Source: W:\SWJediPowerBattles\Work\anim.c
 *
 * Sound scheduling remains in anim_HandleSound and anim_SoundStart; this
 * reviewed owner covers the pose-producing state used by the PC frame.
 */
static int anim_GoNextAnimFrame(animObject *animation)
{
    JPBAnimPartialResult result;
    _animFrame *published_frame = NULL;
    int32_t increment;

    if (animation == NULL ||
        animation->pCurrentAnimSeq == NULL ||
        animation->pCurrentAnimSeq->pAnimTemplate == NULL ||
        animation->pMotion == NULL) {
        return JPB_ANIM_PARTIAL_INVALID_ARGUMENT;
    }

    animation->animFlags &= ~UINT32_C(8);
    if (animation->tweenFramesLeft != 0) {
        if (anim_CheckSlack(
                animation,
                animation->pCurrentAnimSeq) == 0) {
            --animation->tweenFramesLeft;
            if (animation->tweenFramesLeft == 0) {
                animation->animFlags &= ~UINT32_C(0x40);
            }
            anim_advance_tween_pose(animation);
            animation->pCurrentAnimFrame =
                &animation->tweenAnimFrame;
            return JPB_ANIM_PARTIAL_OK;
        }
        if (animation->animList.head == NULL &&
            anim_MotionRecovery(animation) != 0) {
            return JPB_ANIM_PARTIAL_EMPTY;
        }
        result = jpb_AnimActivateQueuedMotionState(animation);
        if (result != JPB_ANIM_PARTIAL_OK) {
            return result;
        }
        return animation->pCurrentAnimFrame != NULL
            ? JPB_ANIM_PARTIAL_OK
            : JPB_ANIM_PARTIAL_EMPTY;
    }

    result = jpb_AnimAdvanceQueuedMotionAtEnd(animation);
    if (result != JPB_ANIM_PARTIAL_OK &&
        result != JPB_ANIM_PARTIAL_EMPTY) {
        return result;
    }
    if (animation->tweenFramesLeft != 0) {
        return animation->pCurrentAnimFrame != NULL
            ? JPB_ANIM_PARTIAL_OK
            : JPB_ANIM_PARTIAL_EMPTY;
    }

    result = jpb_AnimDecodeFrameState(
        animation, &published_frame);
    if (result != JPB_ANIM_PARTIAL_OK ||
        published_frame == NULL) {
        return result;
    }
    animation->pCurrentAnimFrame = published_frame;
    increment = anim_arithmetic_shift12(
        (int32_t)((uint32_t)gGlobalFrameRate *
                  (uint32_t)animation->animFrameRate));
    animation->animFrameAcc += increment;

    if (anim_CheckSlack(
            animation,
            animation->pCurrentAnimSeq) != 0) {
        if (animation->animList.head == NULL &&
            anim_MotionRecovery(animation) != 0) {
            return JPB_ANIM_PARTIAL_EMPTY;
        }
        result = jpb_AnimActivateQueuedMotionState(animation);
        if (result != JPB_ANIM_PARTIAL_OK) {
            return result;
        }
    }

    if (animation->animFrameRate > JPB_FIXED_ONE &&
        animation->pMotion->SpeedAcc != 0) {
        animation->animFrameRate -=
            animation->pMotion->SpeedAcc;
        if (animation->animFrameRate < JPB_FIXED_ONE) {
            animation->animFrameRate = JPB_FIXED_ONE;
        }
    }
    return animation->pCurrentAnimFrame != NULL
        ? JPB_ANIM_PARTIAL_OK
        : JPB_ANIM_PARTIAL_EMPTY;
}

JPBAnimPartialResult jpb_AnimStepFrameState(
    animObject *animation, _animFrame **published_frame)
{
    JPBAnimPartialResult result;

    if (published_frame != NULL) {
        *published_frame = NULL;
    }
    if (published_frame == NULL) {
        return JPB_ANIM_PARTIAL_INVALID_ARGUMENT;
    }
    result = (JPBAnimPartialResult)
        anim_GoNextAnimFrame(animation);
    if (result == JPB_ANIM_PARTIAL_OK) {
        *published_frame = animation->pCurrentAnimFrame;
        anim_HandleSound(animation, 0);
        anim_HandleSound(animation, 1);
    }
    return result;
}

/* 0x188D0, 366 bytes, local, 6 named locals
 * anim_HandleSound
 * PDB type: void (animObject*, int)
 * Source: W:\SWJediPowerBattles\Work\anim.c
 */
static void anim_HandleSound(
    animObject *animation, int channel)
{
    Motion *motion;
    sceneObject *scene;
    playerObject *player;
    char sound[9];
    int bank;

    if (animation == NULL ||
        (unsigned)channel >= JPB_ANIM_SOUND_CHANNELS ||
        animation->soundTimer[channel] == 0 ||
        LevelSelect == 0 ||
        (float)animation->soundTimer[channel] >
            (float)gGlobalTimer) {
        return;
    }
    motion = animation->pMotion;
    scene = (sceneObject *)animation->animRoot.pParent;
    player = scene != NULL
        ? (playerObject *)scene->pPlayer
        : NULL;
    if (motion == NULL || player == NULL) {
        return;
    }
    memcpy(sound, motion->snd[channel], 8);
    sound[8] = '\0';
    if (sound[0] == '#') {
        if (animation->loopHandle[channel] != 0) {
            sound_StopSound(animation->loopHandle[channel]);
        }
    } else if (sound[0] != '0' &&
               strcmp(sound, "vbdgetdm") != 0) {
        uint16_t handle;

        bank = player->playernum + 1;
        if (bank > 3) {
            bank = 3;
        }
        handle = sound_Play(
            physics_gGetPosition(&player->playerRoot),
            bank,
            sound,
            0);
        if (handle == 0) {
            (void)sound_Play(
                physics_gGetPosition(&player->playerRoot),
                0,
                sound,
                0);
        }
    }
    animation->soundTimer[channel] = 0;
}

/* 0x19180, 642 bytes, local, 9 named locals
 * anim_SoundStart
 * PDB type: void (playerObject*, animObject*, int)
 * Source: W:\SWJediPowerBattles\Work\anim.c
 */
static void anim_SoundStart(
    playerObject *player,
    animObject *animation,
    int channel)
{
    Motion *motion;
    int8_t delay;
    uint16_t handle;
    char sound[9];
    int bank;

    if (LevelSelect == 0 ||
        player == NULL ||
        animation == NULL ||
        animation->pMotion == NULL ||
        (unsigned)channel >= JPB_ANIM_SOUND_CHANNELS) {
        return;
    }
    motion = animation->pMotion;
    delay = motion->sndDelay[channel];
    handle = animation->loopHandle[channel];
    memcpy(sound, motion->snd[channel], 8);
    sound[8] = '\0';

    if (sound[0] == '#') {
        if (handle != 0) {
            sound_StopSound(handle);
        }
        handle = 0;
    } else if (sound[0] != '0') {
        if (delay < 1) {
            if (handle != 0) {
                sound_StopSound(handle);
            }
            if (LevelSelect == 2 &&
                player->playerID == 45 &&
                player->target != NULL &&
                (player->target->pFlags &
                 UINT32_C(0x400)) == 0) {
                animation->soundTimer[channel] =
                    animation->animFrameAcc == 800
                        ? (int32_t)(gGlobalTimer + 25256u)
                        : 0;
                return;
            }
            bank = player->playernum + 1;
            if (bank > 3) {
                bank = 3;
            }
            handle = sound_Play(
                physics_gGetPosition(&player->playerRoot),
                bank,
                sound,
                0);
            if (handle == 0) {
                handle = sound_Play(
                    physics_gGetPosition(&player->playerRoot),
                    0,
                    sound,
                    0);
            }
            if (delay < 0 && handle != 0) {
                sound_SetLoopingFadeTime(
                    handle,
                    gGlobalTimer +
                        (uint32_t)(-(int32_t)delay) *
                            JPB_FIXED_ONE);
                animation->soundTimer[channel] = 0;
            }
        } else {
            animation->soundTimer[channel] =
                (int32_t)(gGlobalTimer +
                    (uint32_t)(uint8_t)delay * 512u);
        }
    }
    animation->loopHandle[channel] = handle;
}

/* 0x19580, 75 bytes, global, 2 named locals
 * shouldPlayAnimSound
 * PDB type: int (short, const animObject*)
 * Source: W:\SWJediPowerBattles\Work\anim.c
 */
int shouldPlayAnimSound(
    int16_t player_id, const animObject *animation)
{
    if (player_id == 26 &&
        LevelSelect == 3 &&
        strncmp(
            (const char *)animation->pMotion->snd[0],
            "dstroll1",
            8) == 0 &&
        playertankindex != 0) {
        return 0;
    }
    return 1;
}

/* 0x195D0, 144 bytes, global, 3 named locals
 * shouldReplayAnimSound
 * PDB type: int (short, const animObject*)
 * Source: W:\SWJediPowerBattles\Work\anim.c
 */
int shouldReplayAnimSound(
    int16_t player_id, const animObject *animation)
{
    const char *sound =
        (const char *)animation->pMotion->snd[0];

    if (player_id == 26 &&
        LevelSelect == 3 &&
        strncmp(sound, "dstroll1", 8) == 0 &&
        playertankindex != 0) {
        return 0;
    }
    return strncmp(sound, "taxiaway", 8) != 0;
}

void jpb_AnimHandleSoundState(animObject *animation)
{
    anim_HandleSound(animation, 0);
    anim_HandleSound(animation, 1);
}

/* 0x184F0, 143 bytes, global, 3 named locals
 * anim_GlobalInit
 * PDB type: void ()
 * Source: W:\\SWJediPowerBattles\\Work\\anim.c
 */
void anim_GlobalInit(void)
{
    char table_path[JPB_RESOURCE_PATH_CAPACITY];
    char value_path[JPB_RESOURCE_PATH_CAPACITY];
    char option_path[JPB_RESOURCE_PATH_CAPACITY];
    const char *resolved_path;

    resolved_path = resource_getPath(
        "huffman.tab", JPB_RESOURCE_ANIMATION);
    if (resolved_path == NULL) {
        return;
    }
    (void)snprintf(
        table_path, sizeof(table_path), "%s", resolved_path);
    resolved_path = resource_getPath(
        "huffman.val", JPB_RESOURCE_ANIMATION);
    if (resolved_path == NULL) {
        return;
    }
    (void)snprintf(
        value_path, sizeof(value_path), "%s", resolved_path);
    resolved_path = resource_getPath(
        "huffman.opt", JPB_RESOURCE_ANIMATION);
    if (resolved_path == NULL) {
        return;
    }
    (void)snprintf(
        option_path, sizeof(option_path), "%s", resolved_path);

    if (jpb_HuffmanLoadFiles(
            table_path,
            value_path,
            option_path,
            &animationHuffmanTables) == JPB_HUFFMAN_OK) {
        jpb_HuffmanUseTables(&animationHuffmanTables);
    }
}

/* 0x18D40, 98 bytes, global, 1 named locals
 * anim_ProcessAnimations
 * PDB type: void ()
 * Source: W:\\SWJediPowerBattles\\Work\\anim.c
 */
void anim_ProcessAnimations(void)
{
    int index;

    for (index = 0; index < JPB_ANIMATION_CAPACITY; ++index) {
        animObject *animation = &maAnimationData[index];

        if (animation->animRoot.objectID != -1 &&
            (animation->animRoot.flags & UINT32_C(0x20)) == 0) {
            (void)anim_GoNextAnimFrame(animation);
            scene_gSetSceneModelKeyFrame(
                index, animation->pCurrentAnimFrame);
            anim_HandleSound(animation, 0);
            anim_HandleSound(animation, 1);
        }
    }
}

static int anim_ascii_equal_ignore_case(
    const char *left, const char *right)
{
    unsigned char a;
    unsigned char b;

    if (left == NULL || right == NULL) {
        return 0;
    }
    do {
        a = (unsigned char)*left++;
        b = (unsigned char)*right++;
        if (a >= 'A' && a <= 'Z') {
            a = (unsigned char)(a + ('a' - 'A'));
        }
        if (b >= 'A' && b <= 'Z') {
            b = (unsigned char)(b + ('a' - 'A'));
        }
        if (a != b) {
            return 0;
        }
    } while (a != '\0');
    return 1;
}

/* 0x19410, 361 bytes, global, 8 named locals
 * console_AnimCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\\SWJediPowerBattles\\Work\\anim.c
 */
int console_AnimCommand(
    int argument_count,
    char **string_arguments,
    int *integer_arguments,
    float *float_arguments)
{
    playerObject *player;
    int count;
    int index;

    (void)float_arguments;
    if (argument_count <= 1 || string_arguments == NULL ||
        integer_arguments == NULL || gpWorld == NULL ||
        gpWorld->player0 == NULL) {
        return 0;
    }
    player = gpWorld->player0;
    if (anim_ascii_equal_ignore_case(
            string_arguments[0], "play")) {
        count = argument_count - 1;
        if (count > JPB_ANIM_QUEUE_NODE_CAPACITY) {
            return 0;
        }
        for (index = 1; index <= count; ++index) {
            int motion_index = integer_arguments[index];

            if (motion_index < 0 ||
                motion_index >= player->maxMotions) {
                continue;
            }
            if (index == 1) {
                (void)animctrl_MotionLock(
                    &player->playerRoot,
                    &player->paMotions[motion_index]);
            } else {
                (void)animctrl_MotionChain(
                    &player->playerRoot,
                    &player->paMotions[motion_index]);
            }
        }
        return 0;
    }
    if (anim_ascii_equal_ignore_case(
            string_arguments[0], "fx") &&
        argument_count > 2 && player->paMotions != NULL &&
        player->maxMotions >= 0) {
        int motion_index = integer_arguments[1];
        int effect = integer_arguments[2];

        if (motion_index < 0) {
            motion_index = 0;
        }
        if (motion_index > player->maxMotions) {
            motion_index = player->maxMotions;
        }
        if (effect < 0) {
            effect = 0;
        }
        if (effect > 0x54) {
            effect = 0x54;
        }
        player->paMotions[motion_index].fx1 = (uint8_t)effect;
    }
    return 0;
}


