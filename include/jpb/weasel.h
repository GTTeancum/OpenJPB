#ifndef JPB_WEASEL_H
#define JPB_WEASEL_H

#include "jpb/anim.h"
#include "jpb/player.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact PDB globals at matched-PC RVAs 0x5825C8, 0x5825D8, and 0x944740. */
extern int32_t EDITNODE[2];
extern Motion *paMotions;
extern uint32_t lFrame;
extern uint32_t fFrame;

void weasel_DumpFlags(uint32_t flags);
void weasel_EditAnim(int32_t *cpad, playerObject *player);
void weasel_EditBasic(uint32_t *cpad, playerObject *player, int sequence);
void weasel_EditCombat(uint32_t *cpad, playerObject *player, int sequence);
int weasel_EditCombo(uint32_t *cpad, playerObject *player);
void weasel_EditEffects(uint32_t *cpad, playerObject *player, int sequence);
void weasel_EditFlags(uint32_t *cpad, playerObject *player, int sequence);
void weasel_EditFrames(uint32_t *cpad, playerObject *player, int sequence);
void weasel_EditMechanics(uint32_t *cpad, playerObject *player, int sequence);
void weasel_EditNode(int32_t *cpad, playerObject *player);
void weasel_LevelBars(int current, int maximum, int color);

#ifdef __cplusplus
}
#endif

#endif
