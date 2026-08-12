#ifndef JPB_AUDIO_STREAM_H
#define JPB_AUDIO_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Dependency-free platform seam for the matched PC streaming-music
 * procedure. The callback is reconstruction infrastructure, not a PDB
 * symbol; playXA and its parameter names are exact.
 */
typedef void (*JPBAudioStreamPlayHook)(
    int strIndex,
    const char *streamName,
    int volume,
    int bLoop,
    void *user_data);

typedef enum JPBAudioStreamControl {
    JPB_AUDIO_STREAM_START_UP = 0,
    JPB_AUDIO_STREAM_SHUT_DOWN,
    JPB_AUDIO_STREAM_PAUSE,
    JPB_AUDIO_STREAM_RESUME,
    JPB_AUDIO_STREAM_STOP,
    JPB_AUDIO_STREAM_SET_VOLUME,
    JPB_AUDIO_STREAM_SET_CHANNEL_TYPE
} JPBAudioStreamControl;

typedef int (*JPBAudioStreamControlHook)(
    JPBAudioStreamControl control,
    int value,
    void *user_data);

void jpb_AudioStreamSetPlayHook(
    JPBAudioStreamPlayHook hook,
    void *user_data);
void jpb_AudioStreamSetControlHook(
    JPBAudioStreamControlHook hook,
    void *user_data);
void kmAudioStream_ShutDown(void);
int kmAudioStream_StartUp(void);
void pauseXA(void);
void playXA(int strIndex, int volume, int bLoop);
void setChannelType(int numChannels);
void setMusicVol(int volume);
void stopXA(void);
void unpauseXA(void);
void updateXA(void);

#ifdef __cplusplus
}
#endif

#endif
