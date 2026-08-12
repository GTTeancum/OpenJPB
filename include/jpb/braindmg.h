#ifndef JPB_BRAINDMG_H
#define JPB_BRAINDMG_H

#include "jpb/player.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DamageTracker {
    uint32_t timer;
    int16_t hits;
    int16_t current;
    float total;
} DamageTracker;

extern DamageTracker damageTracking[2];

/*
 * Exact PDB signature. The source accepted void* even though every recovered
 * caller supplies playerObject*.
 */
int braindmg_DamageControl(void *player);
int braindmg_AirHitReaction(
    playerObject *player, playerObject *target);
void braindmg_BlockEffects(playerObject *player);
int braindmg_Blocking(
    playerObject *player,
    playerObject *target,
    int mDamageTotal);
int braindmg_DamageEffects(playerObject *player);
void braindmg_DamageTracker(
    playerObject *player, int damage);
int braindmg_DeathReaction(
    playerObject *player, playerObject *target);
int braindmg_FindHitReaction(
    playerObject *player,
    playerObject *target,
    int DEATH);
int braindmg_HitReaction(
    playerObject *player,
    playerObject *target,
    int DEATH);
int braindmg_LevelDamage(playerObject *player);
int braindmg_LogHits(
    playerObject *player, playerObject *attacker);
void braindmg_ResetDamageTracker(int playernum);

#if defined(__cplusplus)
#define JPB_BRAINDMG_STATIC_ASSERT static_assert
#else
#define JPB_BRAINDMG_STATIC_ASSERT _Static_assert
#endif

JPB_BRAINDMG_STATIC_ASSERT(
    sizeof(DamageTracker) == 12,
    "DamageTracker must match PDB type 0x1351");
JPB_BRAINDMG_STATIC_ASSERT(
    offsetof(DamageTracker, total) == 8,
    "DamageTracker.total layout changed");

#undef JPB_BRAINDMG_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
