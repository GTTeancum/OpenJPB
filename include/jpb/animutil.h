#ifndef JPB_ANIMUTIL_H
#define JPB_ANIMUTIL_H

#include "jpb/anim.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void anim_GetSeqFrameRange(
    objectRoot *object,
    Motion *motion,
    int *first_frame,
    int *last_frame);
_dpcontext *anim_GetTargetContext(animObject *animation);
int anim_GetTargetPartNum(animObject *animation);
_animTemplate *anim_GetTargetSeqPtr(
    animObject *animation, Motion *motion);
void anim_ResetJedi(int index);
void anim_gDumpSeq(int sequence, _animTemplate *template_data);
void animutl_FlushSeqQueue(objectRoot *object);
int animutl_GetCurrentLock(objectRoot *object);
int animutl_GetLockLevel(objectRoot *object);
int animutl_GetPercentPlayed(objectRoot *object);
int animutl_GetTweeningFramesLeft(objectRoot *object);
int animutl_GetWindow(objectRoot *object);
void animutl_SetCurrentLock(objectRoot *object, int lock);
void animutl_gRestartAnim(objectRoot *object);
int32_t animutl_gGetCurrentAnimLength(objectRoot *object);
int32_t animutl_gGetCurrentFrameIndex(objectRoot *object);
void animutl_gPauseAnim(objectRoot *object);
void animutl_gScaleAnimFrameRate(objectRoot *object, int scale);
void animutl_gSetAnimFrameRate(objectRoot *object, int rate);
void animutl_gSetCurrentFrameIndex(objectRoot *object, int frame);
void animutl_gUnPauseAnim(objectRoot *object);

#ifdef __cplusplus
}
#endif

#endif
