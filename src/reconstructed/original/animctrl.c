/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\animctrl.c.
 *
 * All six motion-chain/lock wrappers are reviewed. Their predicates and queue
 * arguments are exact, and forced paths enter PDB-named
 * anim_ForceNextAnimSeq, including frame decode, tweening, and sound timing.
 *
 * Provenance:
 *   direct     - name/signature and object layouts from the exact PDB.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - component lookup, unsigned lock comparisons, queue arguments,
 *                activation calls, and boolean results checked at RVAs
 *                0x19660..0x1985D.
 *
 * PDB module: 0005
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\animctrl.obj
 * Primary source: W:\SWJediPowerBattles\Work\animctrl.c
 * Compiler language: c
 * Emitted procedures: 6
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/animctrl.h"
#include "jpb/scene.h"

static animObject *animctrl_get_animation(objectRoot *object)
{
    sceneObject *scene =
        (sceneObject *)object->pParent;

    return (animObject *)scene->pAnim;
}

static int animctrl_force_motion_state(
    animObject *animation, Motion *motion)
{
    if (anim_AddNextAnimSeq(animation, motion, 1) != 0) {
        return 0;
    }
    (void)anim_ForceNextAnimSeq(animation, 1);
    return 1;
}

/* 0x19660, 33 bytes, global, 2 named locals
 * animctrl_MotionChain
 * PDB type: int (objectRoot*, Motion*)
 * Source: W:\SWJediPowerBattles\Work\animctrl.c
 */
int animctrl_MotionChain(objectRoot *object, Motion *motion)
{
    animObject *animation = animctrl_get_animation(object);

    return anim_AddNextAnimSeq(animation, motion, 0) == 0;
}

/* 0x19690, 138 bytes, global, 6 named locals
 * animctrl_MotionComboChain
 * PDB type: int (objectRoot*, Motion*, int, ...
 * Source: W:\SWJediPowerBattles\Work\animctrl.c
 */
int animctrl_MotionComboChain(
    objectRoot *parent,
    Motion *motion,
    int IsChain,
    int force,
    int alt)
{
    animObject *pAnim = animctrl_get_animation(parent);

    motion->motionFlags |= UINT32_C(0x02000000);
    if (alt != 0) {
        motion->motionFlags |= UINT32_C(2);
    }
    if (IsChain != 0) {
        if (anim_AddNextAnimSeq(pAnim, motion, 0) == 0) {
            return -1;
        }
    } else if (pAnim->Lock < 30 || force == 1) {
        if (anim_AddNextAnimSeq(pAnim, motion, 1) == 0) {
            (void)anim_ForceNextAnimSeq(pAnim, 1);
            return 1;
        }
    }
    return 0;
}

/* 0x19720, 74 bytes, global, 3 named locals
 * animctrl_MotionEqualLock
 * PDB type: int (objectRoot*, Motion*)
 * Source: W:\SWJediPowerBattles\Work\animctrl.c
 */
int animctrl_MotionEqualLock(objectRoot *object, Motion *motion)
{
    animObject *animation = animctrl_get_animation(object);

    if (animation->Lock <= (uint16_t)motion->Lock) {
        return animctrl_force_motion_state(animation, motion);
    }
    return 0;
}

/* 0x19770, 74 bytes, global, 3 named locals
 * animctrl_MotionLock
 * PDB type: int (objectRoot*, Motion*)
 * Source: W:\SWJediPowerBattles\Work\animctrl.c
 */
int animctrl_MotionLock(objectRoot *object, Motion *motion)
{
    animObject *animation = animctrl_get_animation(object);

    if (animation->Lock < (uint16_t)motion->Lock) {
        return animctrl_force_motion_state(animation, motion);
    }
    return 0;
}

/* 0x197C0, 92 bytes, global, 4 named locals
 * animctrl_MotionLockLevel
 * PDB type: int (objectRoot*, Motion*, int)
 * Source: W:\SWJediPowerBattles\Work\animctrl.c
 */
int animctrl_MotionLockLevel(
    objectRoot *object, Motion *motion, int level)
{
    animObject *animation = animctrl_get_animation(object);

    if ((int)(uint16_t)animation->Lock < level) {
        return animctrl_force_motion_state(animation, motion);
    }
    return 0;
}

/* 0x19820, 61 bytes, global, 3 named locals
 * animctrl_MotionNoLock
 * PDB type: int (objectRoot*, Motion*)
 * Source: W:\SWJediPowerBattles\Work\animctrl.c
 */
int animctrl_MotionNoLock(objectRoot *object, Motion *motion)
{
    return animctrl_force_motion_state(
        animctrl_get_animation(object), motion);
}
