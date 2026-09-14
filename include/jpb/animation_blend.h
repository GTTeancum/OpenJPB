#ifndef JPB_ANIMATION_BLEND_H
#define JPB_ANIMATION_BLEND_H
#include "jpb/anim.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct JPBAnimationBlend {
    _animFrame from, output;
    const void *sequence;
    int motion, initialized;
    float elapsed;
    unsigned transitions;
} JPBAnimationBlend;
/* Crossfade joint angles only. Root motion and event bytes stay authored. */
const _animFrame *jpb_AnimationBlendFrame(JPBAnimationBlend *state,
    const _animFrame *frame, const void *sequence, int motion,
    float elapsed_seconds, float duration_seconds);
void jpb_AnimationSetBlendSeconds(float seconds);
float jpb_AnimationBlendSeconds(void);
float jpb_AnimationBlendMotionSeconds(const Motion *motion);
#ifdef __cplusplus
}
#endif
#endif
