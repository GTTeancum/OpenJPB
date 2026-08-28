#ifndef JPB_ANIM_H
#define JPB_ANIM_H

#include "jpb/fmath.h"
#include "jpb/list.h"
#include "jpb/objroot.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_ANIM_JOINT_CAPACITY = 32,
    JPB_ANIM_EVENT_BYTES = 32,
    JPB_MOTION_NAME_BYTES = 32,
    JPB_ANIMATION_CAPACITY = 20,
    JPB_ANIM_QUEUE_NODE_CAPACITY = 8,
    JPB_ANIM_SOUND_CHANNELS = 2
};

typedef enum JPBAnimPartialResult {
    JPB_ANIM_PARTIAL_OK = 0,
    JPB_ANIM_PARTIAL_EMPTY = 1,
    JPB_ANIM_PARTIAL_INVALID_ARGUMENT = -1,
    JPB_ANIM_PARTIAL_UNSUPPORTED_STATE = -2
} JPBAnimPartialResult;

typedef struct playerObject playerObject;

/*
 * Exact matched-PC PDB type 0x10AF. This record contains no pointers, so its
 * 100-byte representation is also suitable for the later 32-bit Xbox port.
 */
typedef struct Motion {
    uint32_t motionFlags;
    uint16_t Seq;
    uint16_t globalID;
    uint8_t twin;
    uint8_t twout;
    uint8_t cutin;
    uint8_t cutout;
    uint8_t disp;
    uint8_t Lock;
    int8_t frzin;
    int8_t frzout;
    int16_t vel;
    int16_t reface;
    uint32_t attackFlags;
    uint8_t Damage;
    uint8_t Delay;
    uint8_t hitReact;
    uint8_t combo;
    uint8_t snd[2][8];
    int8_t sndDelay[2];
    int8_t fx1;
    int8_t fx1Delay;
    int8_t fx2;
    int8_t fx2Delay;
    int16_t Speed;
    int16_t SpeedAcc;
    int16_t Charge;
    int16_t ChargeAcc;
    int16_t Recoil;
    int16_t RecoilAcc;
    int32_t FunctPtr;
    char name[JPB_MOTION_NAME_BYTES];
} Motion;

/*
 * Exact matched-PC PDB type 0x10B0. Each PDB array's reported size is 256
 * bytes: 32 eight-byte joint vectors in each of the two arrays.
 */
typedef struct _animFrame {
    _svector v3RootTranslation;
    _svector av3JointAngle[JPB_ANIM_JOINT_CAPACITY];
    _svector v3RootTranslationDelta;
    _svector av3JointAngleDeltas[JPB_ANIM_JOINT_CAPACITY];
    char event[JPB_ANIM_EVENT_BYTES];
} _animFrame;

typedef struct _animTemplate {
    uint32_t FframeAddr;
    int16_t Fframe;
    int16_t Lframe;
    int16_t parts;
    int16_t pad1;
    VECTOR ConstantNoTrans;
    uint32_t pad2;
} _animTemplate;

typedef struct _dpcontext {
    uint16_t *wordbuffer;
    int32_t wordsinbuffer;
    uint16_t *handyvalues;
    uint32_t *huffdata;
    uint32_t *huffdataorigin;
    uint32_t huffdword;
    int32_t huffbits;
    uint32_t n_bitmask;
    uint16_t numseq;
    uint16_t numparts;
    _animTemplate *seqdata;
} _dpcontext;

typedef struct animListNode {
    Node anm_Node;
    _animTemplate *pAnimTemplate;
    int16_t Lock;
    int16_t Speed;
    int16_t tweenLevel;
    int16_t pad;
    Motion *pMotion;
    uint32_t *pUserData;
} animListNode;

/*
 * Exact matched-PC PDB type 0x1145. Pointer-bearing members intentionally
 * retain native width so this runtime object compacts on 32-bit Xbox.
 */
typedef struct animObject {
    objectRoot animRoot;
    Motion *paMotions;
    _animFrame *pCurrentAnimFrame;
    _animFrame *pPreviousAnimFrame;
    _dpcontext depack_context;
    _dpcontext depack_context3;
    _animFrame AnimFrameBuffer[2];
    int32_t animFrameIndex;
    int32_t animFrameAcc;
    int32_t animFrameRate;
    int32_t animGlobalFrameRateModifier;
    List animFreeList;
    List animList;
    animListNode *pCurrentAnimSeq;
    uint8_t tweenLevel;
    uint8_t tweenFramesLeft;
    uint16_t dispIn;
    _animFrame tweenAnimFrame;
    _svector tweenRotFrame[JPB_ANIM_JOINT_CAPACITY];
    _svector tweenDeltaTranslation;
    _svector tweenDeltaRotFrame[JPB_ANIM_JOINT_CAPACITY];
    _svector *pv3TweenDeltaTranslation;
    _svector *pv3TweenDeltaRotFrame;
    uint16_t curBufferId;
    uint16_t Lock;
    Motion *pMotion;
    int32_t *pUserData;
    uint32_t animFlags;
    uint16_t loopHandle[JPB_ANIM_SOUND_CHANNELS];
    int32_t soundTimer[JPB_ANIM_SOUND_CHANNELS];
} animObject;

extern animObject maAnimationData[JPB_ANIMATION_CAPACITY];

typedef struct JPBAnimForceProfile {
    double lastTotalSeconds;
    double lastRecoverySeconds;
    double lastActivateSeconds;
    double lastActivateMotionSeconds;
    double lastActivateSoundSeconds;
    double lastActivateTweenSeconds;
    double lastDecodeStepSeconds;
    double maxTotalSeconds;
    double maxRecoverySeconds;
    double maxActivateSeconds;
    double maxActivateMotionSeconds;
    double maxActivateSoundSeconds;
    double maxActivateTweenSeconds;
    double maxDecodeStepSeconds;
    uint32_t maxObjectId;
    uint16_t maxMotionSeq;
} JPBAnimForceProfile;

void jpb_AnimGetForceProfile(JPBAnimForceProfile *profile);
void jpb_AnimSetForceProfileEnabled(int enabled);

int anim_AddNextAnimSeq(
    animObject *animation, Motion *motion, int replace_queue);
int anim_ForceNextAnimSeq(animObject *animation, int IsTween);
animObject *anim_CreateObject(
    void *scene_data, char *animfile, void *reuse, int ID);
int anim_CheckFreeze(objectRoot *object);
void anim_InitAnimations(int start);
void anim_GlobalInit(void);
void anim_ProcessAnimations(void);
int console_AnimCommand(
    int argument_count,
    char **string_arguments,
    int *integer_arguments,
    float *float_arguments);
/* Portable per-object extraction of anim_InitAnimations' exact slot body. */
void jpb_AnimResetObjectSlot(int index);

/*
 * Extracted motion-to-physics block from anim_ForceNextAnimSeq. This portable
 * boundary has a descriptive name and is not claimed as an original symbol.
 */
void jpb_AnimApplyMotionPhysics(playerObject *player, const Motion *motion);

/*
 * Descriptive bounded facade retained as a focused test boundary for the
 * reviewed activation state. The exact anim_ForceNextAnimSeq owner performs
 * its canonical queue transition directly.
 */
JPBAnimPartialResult jpb_AnimActivateQueuedMotionState(
    animObject *animation);
/*
 * Bounded facade for the reviewed sequence-end decision in
 * anim_GoNextAnimFrame. It loops authored motions, activates queued
 * successors, and uses anim_MotionRecovery when a non-looping queue is empty.
 */
JPBAnimPartialResult jpb_AnimAdvanceQueuedMotionAtEnd(
    animObject *animation);

/*
 * Bounded publication seam for the reviewed original local anim_GetAnimFrame
 * routine, including alternate target streams and pre-roll decoding. The
 * original PDB symbol remains local to anim.c.
 */
JPBAnimPartialResult jpb_AnimDecodeFrameState(
    animObject *animation, _animFrame **decoded_frame);

/* Reviewed publication boundary for PDB-local anim_CreateTweenFrame. */
JPBAnimPartialResult jpb_AnimCreateTweenFrameState(
    animObject *animation, _animFrame **tween_frame);

/*
 * Per-object publication boundary around the reviewed PDB-local
 * anim_GoNextAnimFrame owner. Unlike the decode-only boundary above, this
 * preserves authored tween frames, sequence-end transitions, slack, and
 * SpeedAcc while returning the pose that should be published this frame.
 */
JPBAnimPartialResult jpb_AnimStepFrameState(
    animObject *animation, _animFrame **published_frame);
void jpb_AnimHandleSoundState(animObject *animation);
int shouldPlayAnimSound(
    int16_t player_id, const animObject *animation);
int shouldReplayAnimSound(
    int16_t player_id, const animObject *animation);

#if defined(__cplusplus)
#define JPB_ANIM_STATIC_ASSERT static_assert
#else
#define JPB_ANIM_STATIC_ASSERT _Static_assert
#endif

JPB_ANIM_STATIC_ASSERT(
    sizeof(Motion) == 100, "Motion must match PDB type 0x10AF");
JPB_ANIM_STATIC_ASSERT(
    offsetof(Motion, vel) == 16, "Motion.vel offset changed");
JPB_ANIM_STATIC_ASSERT(
    offsetof(Motion, Charge) == 54, "Motion.Charge offset changed");
JPB_ANIM_STATIC_ASSERT(
    offsetof(Motion, ChargeAcc) == 56, "Motion.ChargeAcc offset changed");
JPB_ANIM_STATIC_ASSERT(
    offsetof(Motion, name) == 68, "Motion.name offset changed");
JPB_ANIM_STATIC_ASSERT(
    sizeof(_animFrame) == 560,
    "_animFrame must match PDB type 0x10B0");
JPB_ANIM_STATIC_ASSERT(
    offsetof(_animFrame, v3RootTranslationDelta) == 264,
    "_animFrame.v3RootTranslationDelta offset changed");
JPB_ANIM_STATIC_ASSERT(
    offsetof(_animFrame, event) == 528,
    "_animFrame.event offset changed");
JPB_ANIM_STATIC_ASSERT(
    sizeof(_animTemplate) == 32,
    "_animTemplate must match PDB type 0x106F");
#if UINTPTR_MAX == UINT64_MAX
JPB_ANIM_STATIC_ASSERT(
    sizeof(_dpcontext) == 64,
    "_dpcontext must match PDB type 0x106B");
JPB_ANIM_STATIC_ASSERT(
    sizeof(animListNode) == 40,
    "animListNode must match PDB type 0x1153");
JPB_ANIM_STATIC_ASSERT(
    sizeof(animObject) == 2496,
    "animObject must match PDB type 0x1145");
JPB_ANIM_STATIC_ASSERT(
    offsetof(animObject, animFrameIndex) == 1296,
    "animObject.animFrameIndex x64 offset changed");
JPB_ANIM_STATIC_ASSERT(
    offsetof(animObject, pCurrentAnimSeq) == 1344,
    "animObject.pCurrentAnimSeq x64 offset changed");
JPB_ANIM_STATIC_ASSERT(
    offsetof(animObject, pMotion) == 2464,
    "animObject.pMotion x64 offset changed");
JPB_ANIM_STATIC_ASSERT(
    offsetof(animObject, loopHandle) == 2484,
    "animObject.loopHandle x64 offset changed");
#endif

#undef JPB_ANIM_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
