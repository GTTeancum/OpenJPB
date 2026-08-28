#ifndef JPB_SOUND_H
#define JPB_SOUND_H

#include "jpb/vectors.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _sound_Bank _sound_Bank;

/* Exact matched-PC PDB layouts owned by win32/sound.c. */
typedef struct tSFXHandle {
    void *ptrChunk;
    char *chunkName;
} tSFXHandle;

typedef struct tBankHandle {
    tSFXHandle *loadedSFX[62];
    int count;
} tBankHandle;

/*
 * Portable boundary around the platform mixer used by sound_playSfx. This
 * jpb_ callback is an integration seam, not an original PDB symbol. The PC
 * host binds it to the dependency-free WinMM adapter.
 */
typedef uint16_t (*JPBSoundPlaySfxHook)(
    void *chunk,
    int loops,
    VECTOR *position,
    int bankId,
    char *sound,
    uint32_t flag,
    void *user_data);
typedef void *(*JPBSoundChunkLoadHook)(
    const char *path, void *user_data);
typedef void (*JPBSoundChunkFreeHook)(
    void *chunk, void *user_data);
typedef void (*JPBSoundStopHook)(
    uint16_t handle, void *user_data);
typedef void (*JPBSoundFadeHook)(
    uint16_t handle, uint32_t fade_time, void *user_data);
typedef int (*JPBSoundBankHook)(
    int bank_id,
    const char *directory,
    const char *const *paths,
    int count,
    int load,
    void *user_data);

typedef enum JPBSoundSetupOperation {
    JPB_SOUND_SETUP_INIT = 0,
    JPB_SOUND_SETUP_OPEN_AUDIO,
    JPB_SOUND_SETUP_ALLOCATE_CHANNELS
} JPBSoundSetupOperation;

typedef int (*JPBSoundSetupHook)(
    JPBSoundSetupOperation operation,
    int value0,
    int value1,
    int value2,
    int value3,
    void *user_data);

typedef enum JPBSoundChannelOperation {
    JPB_SOUND_CHANNEL_PANNING = 0,
    JPB_SOUND_CHANNEL_DISTANCE,
    JPB_SOUND_CHANNEL_VOLUME,
    JPB_SOUND_CHANNEL_PAUSE,
    JPB_SOUND_CHANNEL_RESUME
} JPBSoundChannelOperation;

typedef void (*JPBSoundChannelHook)(
    JPBSoundChannelOperation operation,
    int channel,
    int value0,
    int value1,
    void *user_data);

typedef enum JPBSoundControl {
    JPB_SOUND_CONTROL_PAUSE_MUSIC = 0,
    JPB_SOUND_CONTROL_HALT_MUSIC,
    JPB_SOUND_CONTROL_MUTE_LOOPED,
    JPB_SOUND_CONTROL_UNMUTE_LOOPED,
    JPB_SOUND_CONTROL_UPDATE_LOOPED
} JPBSoundControl;

typedef void (*JPBSoundControlHook)(
    JPBSoundControl control, void *user_data);

void jpb_SoundSetPlaySfxHook(
    JPBSoundPlaySfxHook hook, void *user_data);
void jpb_SoundSetChunkHooks(
    JPBSoundChunkLoadHook load_hook,
    JPBSoundChunkFreeHook free_hook,
    void *user_data);
void jpb_SoundSetStopHook(
    JPBSoundStopHook hook, void *user_data);
void jpb_SoundSetFadeHook(
    JPBSoundFadeHook hook, void *user_data);
void jpb_SoundSetBankHook(
    JPBSoundBankHook hook, void *user_data);
void jpb_SoundSetSetupHook(
    JPBSoundSetupHook hook, void *user_data);
void jpb_SoundSetChannelHook(
    JPBSoundChannelHook hook, void *user_data);
void jpb_SoundSetControlHook(
    JPBSoundControlHook hook, void *user_data);

const char *ExtractFileNameFromPath(const char *path);
void add_looped_sound_to_update(
    int channel,
    VECTOR *position,
    int leftVolume,
    int rightVolume);
VECTOR convert_svector_to_vector(_svector sv);
void get_sound_volume(
    VECTOR listenerPos,
    VECTOR soundPos,
    uint8_t *out_ds,
    int *out_leftVol,
    int *out_rightVol);
void mute_looped_sounds(void);
void setCDXAvol(unsigned left, unsigned right);
void sound_FreeBank(int gabank);
int sound_GetIndex(int *whichbank, char *name);
int sound_GetSoundIndex(int *bankID, char *name);
char *sound_GetSoundName(int bankID, int index);
void sound_Init(void);
int32_t sound_IsPlaying(uint16_t handle);
int sound_LoadBank(char *file, int gabank);
int sound_NumInBank(int bankID);
void sound_Pause(void);
uint16_t sound_playSfx(
    VECTOR *position, int bankId, char *sound, uint32_t flag);
uint16_t sound_Play(
    VECTOR *position, int bankId, char *sound, uint32_t flag);
uint16_t sound_PlayController(
    VECTOR *position, int bankId, char *sound, uint32_t flag);
uint16_t sound_PlayFV(
    FVECTOR *position, int bankId, char *sound, uint32_t flag);
uint16_t sound_PlaySV(
    _svector *position, int bankId, char *sound, uint32_t flag);
int sound_Resume(void);
void sound_SetFrequency(uint16_t handle, uint32_t frequency);
void sound_StopSound(uint16_t handle);
void sound_SetLoopingFadeTime(
    uint16_t handle, uint32_t fade_time);
void sound_SetPosition(uint16_t handle, VECTOR *pos);
void sound_StopAll(void);
int32_t sound_UnLoadBank(_sound_Bank *bank);
void sound_UpdateAll(int deltatime);
void stop_all_looped_sounds(void);
void unmute_looped_sounds(void);
void update_looped_sounds(void);

extern int sound_Paused;
extern uint8_t loopedSoundMuted;

#ifdef __cplusplus
}
#endif

#endif
