#ifndef JPB_BOSS_H
#define JPB_BOSS_H

#include <stdint.h>
#include "jpb/fmath.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct playerObject playerObject;

/* Exact PDB global at matched-PC RVA 0x4F1288. */
extern _svector gJarJarPos;
/* Exact PDB global at matched-PC RVA 0x4F1280. */
extern int32_t isTatoMaul;

/*
 * Exact PDB procedure. The matched Windows PDB spells the first parameter
 * long*; int32_t preserves that 32-bit contract on every target.
 */
int ai_Destroyer(int32_t *cpad, playerObject *player);
int ai_Deadly(int32_t *cpad, playerObject *player);
int ai_JarJar(int32_t *cpad, playerObject *player);
int ai_Kadu(int32_t *cpad, playerObject *player);
int ai_Krakis(int32_t *cpad, playerObject *player);
int ai_LoaderDroid(int32_t *cpad, playerObject *player);
int ai_Maul(int32_t *cpad, playerObject *player);
int ai_Mtt(int32_t *cpad, playerObject *player);
int ai_Sphere(int32_t *cpad, playerObject *player);
int ai_StarFighter(int32_t *cpad, playerObject *player);
int ai_Thug(int32_t *cpad, playerObject *player);
int ai_TurretDroid(int32_t *cpad, playerObject *player);
int ai_Worm(int32_t *cpad, playerObject *player);
void boss_StarFighterBlaster(playerObject *player, int LOCK_ON);
void ai_ShowFlags(playerObject *player);
int maul_PushCallBack(int32_t *cpad, playerObject *player);
int maul_RingCallBack(int32_t *cpad, playerObject *player);
int maul_ZapCallBack(int32_t *cpad, playerObject *player);

#ifdef __cplusplus
}
#endif

#endif
