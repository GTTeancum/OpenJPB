#ifndef JPB_STUBS_H
#define JPB_STUBS_H

#ifdef __cplusplus
extern "C" {
#endif

void LoadBackdrop(char *name, unsigned char brightness);
void PlotBackdrop(int index);
void SetBackdropBrightness(unsigned char brightness);
void _DisplayIcon(int icon, int x, int y);
void cd_gInitMusic(void);
void cd_gPause(void);
void cd_gPlay(void);
void cd_gPlayTrack(void);
void cd_gStop(void);
void psx_DrawBlur(void);
void sound_Debug(void);

#ifdef __cplusplus
}
#endif

#endif
