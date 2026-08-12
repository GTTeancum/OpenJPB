#ifndef JPB_GENERIC_HOOK_H
#define JPB_GENERIC_HOOK_H

#include "jpb/material.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exact PDB globals from genericHook.cpp. */
extern int cachedTextureIndexForBus;
extern int cachedTextureIndexForCoffin;
extern int foundAllLevelColorOverrides;

void ClearCachedTextureIndices(void);
int IsBusTextureForCorus2(
    int level, const char *filename, int texture_index);
int IsCoffinTextureForPalace(
    int level, const char *filename, int texture_index);
void SetTextureColorOverride(int level, _Material *material);

#ifdef __cplusplus
}
#endif

#endif
