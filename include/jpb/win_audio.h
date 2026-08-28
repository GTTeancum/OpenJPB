#ifndef JPB_WIN_AUDIO_H
#define JPB_WIN_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

int kmAudio_IsAudioStarted(void);
void kmAudio_ShutDown(void);
int kmAudio_StartUp(void);

#ifdef __cplusplus
}
#endif

#endif
