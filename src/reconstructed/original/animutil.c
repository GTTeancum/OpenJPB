/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\animutil.c.
 *
 * All 22 emitted procedures preserve the exact freeze-window decision,
 * target-context and sequence access, lock/frame/window queries, queue flush,
 * and pause/rate/frame mutation used by player control.
 *
 * Provenance:
 *   direct     - name/signature/locals and dependent layouts from the PDB.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - comparisons, member widths, fixed-point rounding, and
 *                signed frame bounds checked at RVAs 0x19860..0x19CDF.
 *
 * PDB module: 0006
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\animutil.obj
 * Primary source: W:\SWJediPowerBattles\Work\animutil.c
 * Compiler language: c
 * Emitted procedures: 22
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/animutil.h"
#include "jpb/animctrl.h"
#include "jpb/brainutl.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/sound.h"

#include <string.h>

static animObject *animutil_from_object(objectRoot *object)
{
    sceneObject *scene =
        (sceneObject *)object->pParent;

    return (animObject *)scene->pAnim;
}

static int32_t animutil_arithmetic_shift12(int32_t value)
{
    if (value >= 0) {
        return value / JPB_FIXED_ONE;
    }
    return -1 -
           (int32_t)((-(int64_t)value - 1) / JPB_FIXED_ONE);
}

/* 0x19860, 100 bytes, global, 5 named locals
 * anim_CheckFreeze
 * PDB type: int (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
int anim_CheckFreeze(objectRoot *object)
{
    animObject *animation = animutil_from_object(object);
    Motion *motion = animation->pMotion;
    int frame = (animation->animFrameIndex >> JPB_FIXED_SHIFT) - 1;

    if (animation->tweenFramesLeft == 0 &&
        (frame < 0 ||
         (int32_t)motion->motionFlags < 0 ||
         (animation->pCurrentAnimSeq->pAnimTemplate->Fframe +
                  motion->frzin <
              frame &&
          (motion->frzout < 1 ||
           frame <=
               animation->pCurrentAnimSeq->pAnimTemplate->Lframe -
                   motion->frzout)))) {
        return 0;
    }
    return 1;
}

/* 0x198D0, 37 bytes, global, 5 named locals
 * anim_GetSeqFrameRange
 * PDB type: void (objectRoot*, Motion*, int*...
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void anim_GetSeqFrameRange(
    objectRoot *object,
    Motion *motion,
    int *first_frame,
    int *last_frame)
{
    animObject *animation =
        animutil_from_object(object);
    _animTemplate *sequence =
        &animation->depack_context.seqdata[motion->Seq];

    *first_frame = sequence->Fframe;
    *last_frame = sequence->Lframe;
}

/* 0x19900, 23 bytes, global, 1 named locals
 * anim_GetTargetContext
 * PDB type: _dpcontext* (animObject*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
_dpcontext *anim_GetTargetContext(animObject *animation)
{
    sceneObject *scene =
        (sceneObject *)animation->animRoot.pParent;
    playerObject *player =
        (playerObject *)scene->pPlayer;
    sceneObject *target_scene =
        (sceneObject *)player->target->playerRoot.pParent;
    animObject *target_animation =
        (animObject *)target_scene->pAnim;

    return &target_animation->depack_context;
}

/* 0x19920, 23 bytes, global, 1 named locals
 * anim_GetTargetPartNum
 * PDB type: int (animObject*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
int anim_GetTargetPartNum(animObject *animation)
{
    return anim_GetTargetContext(animation)->numparts;
}

/* 0x19940, 31 bytes, global, 2 named locals
 * anim_GetTargetSeqPtr
 * PDB type: _animTemplate* (animObject*, Mot...
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
_animTemplate *anim_GetTargetSeqPtr(
    animObject *animation, Motion *motion)
{
    return &anim_GetTargetContext(animation)
                ->seqdata[motion->Seq];
}

/* 0x19960, 133 bytes, global, 4 named locals
 * anim_ResetJedi
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void anim_ResetJedi(int index)
{
    animObject *animation;

    if ((unsigned)index >=
        JPB_ANIMATION_CAPACITY) {
        return;
    }
    animation = &maAnimationData[index];
    animation->animFlags = 0;
    list_MoveList(
        &animation->animFreeList,
        &animation->animList);
    if (animation->loopHandle[0] != 0) {
        sound_StopSound(
            animation->loopHandle[0]);
    }
    if (animation->loopHandle[1] != 0) {
        sound_StopSound(
            animation->loopHandle[1]);
    }
    animation->soundTimer[0] = 0;
    animation->soundTimer[1] = 0;
    animation->animFrameIndex = 0;
    (void)animctrl_MotionNoLock(
        &animation->animRoot,
        animation->paMotions);
}

/* 0x199F0, 3 bytes, global, 2 named locals
 * anim_gDumpSeq
 * PDB type: void (int, _animTemplate*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void anim_gDumpSeq(int sequence, _animTemplate *template_data)
{
    (void)sequence;
    (void)template_data;
}

/* 0x19A00, 26 bytes, global, 2 named locals
 * animutl_FlushSeqQueue
 * PDB type: void (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void animutl_FlushSeqQueue(objectRoot *object)
{
    animObject *animation =
        animutil_from_object(object);

    list_MoveList(
        &animation->animFreeList, &animation->animList);
}

/* 0x19A20, 15 bytes, global, 1 named locals
 * animutl_GetCurrentLock
 * PDB type: int (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
int animutl_GetCurrentLock(objectRoot *object)
{
    return animutil_from_object(object)->Lock;
}

/* 0x19A30, 19 bytes, global, 1 named locals
 * animutl_GetLockLevel
 * PDB type: int (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
int animutl_GetLockLevel(objectRoot *object)
{
    return animutil_from_object(object)
        ->pCurrentAnimSeq->Lock;
}

/* 0x19A50, 42 bytes, global, 2 named locals
 * animutl_GetPercentPlayed
 * PDB type: int (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
int animutl_GetPercentPlayed(objectRoot *object)
{
    animObject *animation =
        animutil_from_object(object);
    int played =
        (animation->animFrameIndex >> JPB_FIXED_SHIFT) -
        animation->pCurrentAnimSeq->pAnimTemplate->Fframe;

    return played < 0 ? 0 : played;
}

/* 0x19A80, 15 bytes, global, 1 named locals
 * animutl_GetTweeningFramesLeft
 * PDB type: int (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
int animutl_GetTweeningFramesLeft(objectRoot *object)
{
    return animutil_from_object(object)->tweenFramesLeft;
}

/* 0x19A90, 121 bytes, global, 7 named locals
 * animutl_GetWindow
 * PDB type: int (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
int animutl_GetWindow(objectRoot *object)
{
    animObject *animation =
        animutil_from_object(object);
    _animTemplate *sequence =
        animation->pCurrentAnimSeq->pAnimTemplate;
    Motion *motion =
        animation->pCurrentAnimSeq->pMotion;
    int first = sequence->Fframe;
    int length = sequence->Lframe - first;
    int cutin = motion->cutin;
    int display_end = length - motion->disp;
    int frame;

    if (motion->Damage == 0 || cutin == UINT8_MAX) {
        return animation->Lock > 0x16 ? length : 6;
    }
    frame =
        (animation->animFrameIndex >> JPB_FIXED_SHIFT) -
        first;
    if (frame < cutin) {
        return cutin - frame;
    }
    if (display_end < frame) {
        return (length - frame) + 6;
    }
    return frame - display_end;
}

/* 0x19B10, 26 bytes, global, 3 named locals
 * animutl_SetCurrentLock
 * PDB type: void (objectRoot*, int)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void animutl_SetCurrentLock(objectRoot *object, int lock)
{
    animObject *animation =
        animutil_from_object(object);

    if (lock < (int)(uint16_t)animation->Lock) {
        animation->Lock = (uint16_t)lock;
    }
}

/* 0x19B30, 29 bytes, global, 1 named locals
 * animutl_gGetCurrentAnimLength
 * PDB type: long (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
int32_t animutl_gGetCurrentAnimLength(objectRoot *object)
{
    _animTemplate *sequence =
        animutil_from_object(object)
            ->pCurrentAnimSeq->pAnimTemplate;

    return (int32_t)sequence->Lframe -
           (int32_t)sequence->Fframe;
}

/* 0x19B50, 36 bytes, global, 2 named locals
 * animutl_gGetCurrentFrameIndex
 * PDB type: long (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
int32_t animutl_gGetCurrentFrameIndex(objectRoot *object)
{
    animObject *animation =
        animutil_from_object(object);

    return (int32_t)(
        (animation->animFrameIndex >> JPB_FIXED_SHIFT) -
        animation->pCurrentAnimSeq->pAnimTemplate->Fframe);
}

/* 0x19B80, 27 bytes, global, 2 named locals
 * animutl_gPauseAnim
 * PDB type: void (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void animutl_gPauseAnim(objectRoot *object)
{
    animutil_from_object(object)->animFlags |=
        0x00000004u;
}

/* 0x19BA0, 179 bytes, global, 6 named locals
 * animutl_gRestartAnim
 * PDB type: void (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void animutl_gRestartAnim(objectRoot *object)
{
    animObject *animation =
        animutil_from_object(object);
    animListNode *current =
        animation->pCurrentAnimSeq;
    Motion *motion = current->pMotion;
    sceneObject *scene =
        (sceneObject *)animation->animRoot.pParent;
    playerObject *player =
        (playerObject *)scene->pPlayer;
    char sound[9];

    animation->animFrameIndex =
        (int32_t)current->pAnimTemplate->Fframe
        << JPB_FIXED_SHIFT;
    memcpy(sound, motion->snd[0], 8);
    sound[8] = '\0';
    brainutl_PlayMotionSound(
        player->playernum,
        sound,
        motion->sndDelay[0]);
}

/* 0x19C60, 35 bytes, global, 3 named locals
 * animutl_gScaleAnimFrameRate
 * PDB type: void (objectRoot*, int)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void animutl_gScaleAnimFrameRate(
    objectRoot *object, int scale)
{
    animObject *animation =
        animutil_from_object(object);
    int32_t product =
        (int32_t)((uint32_t)scale *
                  (uint32_t)animation->animFrameRate);
    uint32_t adjusted =
        (uint32_t)product +
        (product < 0 ? JPB_FIXED_ONE - 1u : 0u);

    animation->animFrameRate =
        animutil_arithmetic_shift12((int32_t)adjusted);
}

/* 0x19C90, 14 bytes, global, 2 named locals
 * animutl_gSetAnimFrameRate
 * PDB type: void (objectRoot*, int)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void animutl_gSetAnimFrameRate(
    objectRoot *object, int rate)
{
    animutil_from_object(object)->animFrameRate = rate;
}

/* 0x19CA0, 35 bytes, global, 3 named locals
 * animutl_gSetCurrentFrameIndex
 * PDB type: void (objectRoot*, int)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void animutl_gSetCurrentFrameIndex(
    objectRoot *object, int frame)
{
    animObject *animation =
        animutil_from_object(object);
    int32_t absolute_frame =
        animation->pCurrentAnimSeq->pAnimTemplate->Fframe +
        frame;

    animation->animFrameIndex =
        (int32_t)((uint32_t)absolute_frame *
                  (uint32_t)JPB_FIXED_ONE);
}

/* 0x19CD0, 15 bytes, global, 2 named locals
 * animutl_gUnPauseAnim
 * PDB type: void (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\animutil.c
 */
void animutl_gUnPauseAnim(objectRoot *object)
{
    animutil_from_object(object)->animFlags &=
        ~0x00000004u;
}
