#ifndef JPB_TPANIM_H
#define JPB_TPANIM_H

#include "jpb/sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int UVCoreIndex[256];

void tpanim_AnimateSCBTexture(TexAnim *animation, SCB *scb);
void tpanim_RegisterTextureUV(int texture, int uv);
void tpanim_ResetTextureUVIndex(void);
void tpanim_gAnimatePalette(PalAnim *animation);

#ifdef __cplusplus
}
#endif

#endif
