#ifndef JPB_MAPANIM_H
#define JPB_MAPANIM_H

#include "jpb/world.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int animsPaused;

void bapmanim_Activate(int32_t animID);
void bapmanim_DeActivate(int32_t animID);
void manim_HandleMapAnims(void);
int manim_InitAnim(int animNum);
void manim_UpdateMapAnim(wsl_BT_ANIMMAP *animation);

#ifdef __cplusplus
}
#endif

#endif
