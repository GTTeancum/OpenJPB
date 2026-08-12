#ifndef JPB_FORCE_H
#define JPB_FORCE_H

#include "jpb/player.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact matched-PC PDB types 0x6D34 and 0x6D44. */
typedef struct ForceSlot {
    int16_t map[4];
    int32_t idx;
} ForceSlot;

typedef struct ForceMap {
    ForceSlot slot[4];
} ForceMap;

extern ForceMap mapData[23];

/*
 * Exact PDB global at matched-PC RVA 0x10DB3B0. force_PlaySeq retains the
 * animation owner used by force-toss callback paths.
 */
struct animObject;
extern struct animObject *storeAnim;

uint32_t color_interpolate(
    uint32_t color,
    uint32_t base_color,
    int amount);
uint32_t color_interpolate4k(
    uint32_t color,
    uint32_t base_color,
    int amount);
int force_AbsorbReflectCallBack(
    int32_t *cpad, playerObject *player);
int force_AttackCallBack(
    int32_t *cpad, playerObject *player);
int force_CloakCallBack(
    int32_t *cpad, playerObject *player);
int force_FlameCallBack(
    int32_t *cpad, playerObject *player);
int force_HealingCallBack(
    int32_t *cpad, playerObject *player);
int force_MesmerizeCallBack(
    int32_t *cpad, playerObject *player);
int force_PushCallBack(
    int32_t *cpad, playerObject *player);
int force_Ranged3CallBack(
    int32_t *cpad, playerObject *player);
int force_ReflectCallBack(
    int32_t *cpad, playerObject *player);
int force_RingCallBack(
    int32_t *cpad, playerObject *player);
int force_SabreTossCallBack(
    int32_t *cpad, playerObject *player);
int force_SabreSpinCallBack(
    int32_t *cpad, playerObject *player);
int force_SabreYoYoBack(
    int32_t *cpad, playerObject *player);
int force_ShieldCallBack(
    int32_t *cpad, playerObject *player);
int force_StarCallBack(
    int32_t *cpad, playerObject *player);
int force_TossCallBack(
    int32_t *cpad, playerObject *player);
int force_TossGrenadeCallBack(
    int32_t *cpad, playerObject *player);
int force_ZapCallBack(
    int32_t *cpad, playerObject *player);
int force_AttackSpinCallBack(
    int32_t *cpad, playerObject *player);
int force_PlaySeq(
    ForceSlot *slot, playerObject *player);
int force_gActivate(
    int32_t *cpad, playerObject *player);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_FORCE_STATIC_ASSERT static_assert
#else
#define JPB_FORCE_STATIC_ASSERT _Static_assert
#endif

JPB_FORCE_STATIC_ASSERT(
    sizeof(ForceSlot) == 12,
    "ForceSlot must match PDB type 0x6D34");
JPB_FORCE_STATIC_ASSERT(
    sizeof(ForceMap) == 48,
    "ForceMap must match PDB type 0x6D44");

#undef JPB_FORCE_STATIC_ASSERT

#endif
