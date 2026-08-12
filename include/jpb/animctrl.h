#ifndef JPB_ANIMCTRL_H
#define JPB_ANIMCTRL_H

#include "jpb/anim.h"
#include "jpb/objroot.h"

#ifdef __cplusplus
extern "C" {
#endif

int animctrl_MotionChain(objectRoot *object, Motion *motion);
int animctrl_MotionComboChain(
    objectRoot *parent,
    Motion *motion,
    int IsChain,
    int force,
    int alt);
int animctrl_MotionEqualLock(objectRoot *object, Motion *motion);
int animctrl_MotionLock(objectRoot *object, Motion *motion);
int animctrl_MotionLockLevel(
    objectRoot *object, Motion *motion, int level);
int animctrl_MotionNoLock(objectRoot *object, Motion *motion);

#ifdef __cplusplus
}
#endif

#endif
