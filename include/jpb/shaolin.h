#ifndef JPB_SHAOLIN_H
#define JPB_SHAOLIN_H

#include "jpb/list.h"
#include "jpb/world.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_KUNGFU_CAPACITY = 20
};

/* Exact PDB globals from shaolin.c. */
extern uint8_t attackChoice[JPB_KUNGFU_CAPACITY];
extern kfNode akfNodes[JPB_KUNGFU_CAPACITY];
extern List kfList;
extern int16_t a[2];

void shaolin_AddKungfu(kfNode *kungfu);
void shaolin_Attack(kfNode *kungfu);
int shaolin_CheckMove(int move, int slot);
void shaolin_DoKungfu(void);
kfNode *shaolin_GetKungfu(int index);
void shaolin_InitKungfu(void);
void shaolin_StartKungfu(void);

#ifdef __cplusplus
}
#endif

#endif
