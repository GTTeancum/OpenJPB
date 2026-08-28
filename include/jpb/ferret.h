#ifndef JPB_FERRET_H
#define JPB_FERRET_H

#include "jpb/fmath.h"
#include "jpb/player.h"
#include "jpb/sprite.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int SAVEMODE;
extern EffectData ed[16];
extern EffectHeader effEdit[128];
extern ProjType *pj;
extern EffectData sfx_Buffer;
extern int spin;
extern char *spinName[6];
extern char *sParticles[15];

int ferret_AuditionEffect(int32_t *cpad, VECTOR *loc);
void ferret_Control(void);
int32_t ferret_CountSprites(EffectData *eff, int len);
void ferret_EditProjectile(uint32_t *cpad);
void ferret_EditRing(uint32_t *cpad, RingData *r);
void ferret_EmmbeddedBank(int32_t *cpad, VECTOR *loc, int n);
void ferret_ParticleBank(int32_t *cpad, int n);
void ferret_ParticleEditor(int32_t *cpad, VECTOR *loc);
int ferret_SaveAllEffects(void);
int ferret_SaveLoadEffect(uint32_t *cpad, int32_t length);
int ferret_SaveLoadParticles(Emiter *emiter, uint32_t cpad);
int ferret_SaveLoadProjectiles(uint32_t *cpad);
int32_t ferret_ShowEffect(EffectData *eff, VECTOR *loc, int len);
void ferret_SpriteBank(int32_t *cpad, VECTOR *loc, int n);
void ferret_SpriteFerret(int32_t *cpad, VECTOR *loc);

#ifdef __cplusplus
}
#endif

#endif
