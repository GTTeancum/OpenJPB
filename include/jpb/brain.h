#ifndef JPB_BRAIN_H
#define JPB_BRAIN_H

#include "jpb/player.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum JPBBrainPartialResult {
    JPB_BRAIN_PARTIAL_OK = 0,
    JPB_BRAIN_PARTIAL_NO_CHANGE = 1,
    JPB_BRAIN_PARTIAL_INVALID_ARGUMENT = -1,
    JPB_BRAIN_PARTIAL_UNSUPPORTED_STATE = -2
} JPBBrainPartialResult;

void brain_CheckForEffects(playerObject *player);
void brain_ControlPlayer(
    int32_t *cpad, playerObject *player, int AI_ON);
void brain_DoRingOffEffect(playerObject *player);
void brain_DoRingOnEffect(playerObject *player);
int brain_HangCallback(int32_t *cpad, playerObject *player);
int brain_LockOn(int32_t *cpad, playerObject *player);
void brain_SetFallTrajectory(playerObject *player, int attack);
void brain_SetJumpTrajectory(playerObject *player, int stand);
void brain_SetTrajectory(playerObject *player, int velocity, int angle);
int brain_GroundControl(
    int32_t *cpad, playerObject *player, playerObject *target);
int brain_SkidCallBack(int32_t *cpad, playerObject *player);
int brain_SwapVelDirCallBack(int32_t *cpad, playerObject *player);
int brain_TakeOff(
    int32_t *cpad, playerObject *player, playerObject *target);
int brain_ThrowEnder(int32_t *cpad, playerObject *player);
void brain_ValidateLockOn(playerObject *player);

int jpb_BrainDirectionAngle(
    float axis_x, float axis_y, int camera_angle);
JPBBrainPartialResult jpb_BrainGroundDirectionState(
    playerObject *player,
    float axis_x,
    float axis_y,
    int camera_angle);
JPBBrainPartialResult jpb_BrainLockOnDirectionState(
    playerObject *player, int desired_facing);
JPBBrainPartialResult jpb_BrainJumpLaunchState(
    playerObject *player,
    int stand,
    JPBPlayerCallback trajectory_callback);
JPBBrainPartialResult jpb_BrainAlternateJumpLaunchState(
    playerObject *player,
    JPBPlayerCallback trajectory_callback);
JPBBrainPartialResult jpb_BrainGroundAttackState(
    playerObject *player);
JPBBrainPartialResult jpb_BrainGroundIdleState(
    playerObject *player, int energy);
JPBBrainPartialResult jpb_BrainGroundSpecialDirectionState(
    playerObject *player, int energy);

#ifdef __cplusplus
}
#endif

#endif
