#ifndef JPB_OVERLAY_H
#define JPB_OVERLAY_H

#ifdef __cplusplus
extern "C" {
#endif

int ovrlay_CheckForce(int player, int amount);
void ovrlay_DoAlphaScreen(void);
void ovrlay_DoWork(void);
void ovrlay_InitOverLay(int player, int mode);
void ovrlay_SetForceBar(int player, int value, int maximum);
void ovrlay_SetForceGem(int player, int value, int maximum);
void ovrlay_SetLifeBar(int player, int value, int maximum);
void ovrlay_SetStunBar(int player, int value);
void ovrlay_gSetBarColor(int player, int color);
void ovrlay_gToggleOverlay(int enabled);

#ifdef __cplusplus
}
#endif

#endif
