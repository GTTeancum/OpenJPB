#ifndef JPB_VEHICLE_H
#define JPB_VEHICLE_H

#include "jpb/vectors.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct playerObject playerObject;
typedef struct Mnode Mnode;

extern int32_t stapbikeindex[2];
extern uint16_t stapsound;
extern uint16_t tanknoise;
extern uint16_t turretnoise;

void StopNearestFan(VECTOR *pos);
int ai_AAT(int32_t *cpad, playerObject *player);
int ai_Blades(int32_t *cpad, playerObject *player);
int ai_Stap(int32_t *cpad, playerObject *player);
int ai_Tank(int32_t *cpad, playerObject *player);
void centreturret(Mnode *pNodeTurret);

#ifdef __cplusplus
}
#endif

#endif
