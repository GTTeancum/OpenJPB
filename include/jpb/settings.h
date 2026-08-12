#ifndef JPB_SETTINGS_H
#define JPB_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct playerObject playerObject;

void ai_InitModelData(playerObject *pPlayer);
int ai_InitPlayer(playerObject *pPlayer);

/*
 * Descriptive reviewed extraction of ai_InitPlayer's shared/default
 * character setup: model/physics dimensions, collision profile, fixed
 * movement settings, scale, and generic AI callback ownership.
 */
int jpb_ai_ApplyDefaultPlayerSettings(
    playerObject *player);
/*
 * Null-safe host facade for the complete reviewed ai_InitPlayer owner.
 */
int jpb_ai_ApplyPlayerSettings(
    playerObject *player);

#ifdef __cplusplus
}
#endif

#endif
