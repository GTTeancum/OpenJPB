/* State-transition smoothing layered over the recovered 12-bit CAD poses.
 * This is a requested enhancement, not a reconstructed stock procedure. */
#include "jpb/animation_blend.h"
#include <math.h>
static float blend_seconds = 0.2f;
void jpb_AnimationSetBlendSeconds(float seconds)
{
    if (!isfinite(seconds) || seconds < 0.0f) seconds = 0.0f;
    if (seconds > 0.25f) seconds = 0.25f;
    blend_seconds = seconds;
}
float jpb_AnimationBlendSeconds(void) { return blend_seconds; }
float jpb_AnimationBlendMotionSeconds(const Motion *motion)
{
    /* Preserve the previously tested attack startup while easing locomotion
     * and recovery. This is a presentation policy, not a retail motion flag. */
    if (motion != NULL &&
        (motion->Damage != 0 || motion->attackFlags != 0) &&
        blend_seconds > 0.1f) {
        return 0.1f;
    }
    return blend_seconds;
}
static int16_t blend_angle(int16_t from, int16_t to, float fraction)
{
    int delta = (((int)to - (int)from + 2048) & 4095) - 2048;
    return (int16_t)(((int)from + (int)lroundf((float)delta * fraction)) & 4095);
}
const _animFrame *jpb_AnimationBlendFrame(JPBAnimationBlend *state,
    const _animFrame *frame, const void *sequence, int motion,
    float elapsed_seconds, float duration_seconds)
{
    float fraction;
    int joint;
    if (state == NULL || frame == NULL) return frame;
    if (!(duration_seconds > 0.0f) || !isfinite(duration_seconds)) {
        state->initialized = 0;
        return frame;
    }
    if (!state->initialized) {
        state->output = *frame;
        state->sequence = sequence;
        state->motion = motion;
        state->elapsed = duration_seconds;
        state->initialized = 1;
    } else if (state->sequence != sequence || state->motion != motion) {
        state->from = state->output;
        state->sequence = sequence;
        state->motion = motion;
        state->elapsed = 0.0f;
        ++state->transitions;
    } else if (elapsed_seconds > 0.0f && isfinite(elapsed_seconds)) {
        state->elapsed += elapsed_seconds;
    }
    state->output = *frame;
    if (state->elapsed >= duration_seconds) return &state->output;
    fraction = state->elapsed / duration_seconds;
    fraction = fraction * fraction * (3.0f - 2.0f * fraction);
    for (joint = 0; joint < JPB_ANIM_JOINT_CAPACITY; ++joint) {
        _svector *out = &state->output.av3JointAngle[joint];
        const _svector *from = &state->from.av3JointAngle[joint];
        out->vx = blend_angle(from->vx, out->vx, fraction);
        out->vy = blend_angle(from->vy, out->vy, fraction);
        out->vz = blend_angle(from->vz, out->vz, fraction);
    }
    return &state->output;
}
