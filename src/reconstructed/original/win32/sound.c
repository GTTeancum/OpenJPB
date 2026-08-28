/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\win32\sound.c.
 *
 * Gameplay-visible wrappers, bank ownership, and the 100-entry positional-
 * loop table retain their exact PDB names, layouts, and control flow. The
 * SDL_mixer calls are the sole portable boundary; the PC host binds those
 * calls to its WinMM adapter.
 *
 * Provenance:
 *   direct/decompiled - PDB module 0099, all 31 procedures and globals,
 *     exact PDB layouts, and direct retail disassembly at every procedure.
 *   assembly - exact tail calls, bare returns, bank cascade, loop-table layout,
 *     volume arithmetic, and pause/halt behavior at RVAs 0x12A6D0..0x12BB2B.
 *   platform boundary - SDL_mixer setup, bank I/O, and channel calls only.
 */

#include "jpb/sound.h"

#include "jpb/camera.h"
#include "jpb/game.h"
#include "jpb/resources.h"
#include "jpb/sound_bank_data.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    SOUND_LOADED_BANK_COUNT = 4,
    SOUND_LOADED_BANK_WRITE_LIMIT = 4,
    SOUND_LOOPED_SOUND_COUNT = 100
};

/* Exact 24-byte x64 PDB record at matched address 0x140932D00. */
typedef struct LoopedSound {
    uint8_t isValid;
    uint8_t padding[3];
    int channel;
    VECTOR *position;
    int leftVolume;
    int rightVolume;
} LoopedSound;

_Static_assert(
    sizeof(LoopedSound) == 24,
    "loopedSounds entry must match the PDB/matched x64 layout");
_Static_assert(sizeof(tSFXHandle) == 16, "tSFXHandle PDB layout");
_Static_assert(sizeof(tBankHandle) == 504, "tBankHandle PDB layout");
_Static_assert(sizeof(tAudioSFX_Bank) == 80, "tAudioSFX_Bank PDB layout");
_Static_assert(
    offsetof(tAudioSFX_Bank, ptrSFXNames) == 64,
    "tAudioSFX_Bank pointer offset");

int sound_Paused;
uint8_t loopedSoundMuted;

/* PDB declares four slots. Retail's > 4 gate still admits latent slot 4. */
static tBankHandle *loadedBanks[SOUND_LOADED_BANK_COUNT];
static LoopedSound loopedSounds[SOUND_LOOPED_SOUND_COUNT];

static JPBSoundPlaySfxHook jpb_sound_play_sfx_hook;
static void *jpb_sound_play_sfx_user_data;
static JPBSoundChunkLoadHook jpb_sound_chunk_load_hook;
static JPBSoundChunkFreeHook jpb_sound_chunk_free_hook;
static void *jpb_sound_chunk_user_data;
static JPBSoundStopHook jpb_sound_stop_hook;
static void *jpb_sound_stop_user_data;
static JPBSoundFadeHook jpb_sound_fade_hook;
static void *jpb_sound_fade_user_data;
static JPBSoundBankHook jpb_sound_bank_hook;
static void *jpb_sound_bank_user_data;
static JPBSoundSetupHook jpb_sound_setup_hook;
static void *jpb_sound_setup_user_data;
static JPBSoundChannelHook jpb_sound_channel_hook;
static void *jpb_sound_channel_user_data;
static JPBSoundControlHook jpb_sound_control_hook;
static void *jpb_sound_control_user_data;

void jpb_SoundSetPlaySfxHook(
    JPBSoundPlaySfxHook hook, void *user_data)
{
    jpb_sound_play_sfx_hook = hook;
    jpb_sound_play_sfx_user_data = user_data;
}

void jpb_SoundSetChunkHooks(
    JPBSoundChunkLoadHook load_hook,
    JPBSoundChunkFreeHook free_hook,
    void *user_data)
{
    jpb_sound_chunk_load_hook = load_hook;
    jpb_sound_chunk_free_hook = free_hook;
    jpb_sound_chunk_user_data = user_data;
}

void jpb_SoundSetStopHook(
    JPBSoundStopHook hook, void *user_data)
{
    jpb_sound_stop_hook = hook;
    jpb_sound_stop_user_data = user_data;
}

void jpb_SoundSetFadeHook(
    JPBSoundFadeHook hook, void *user_data)
{
    jpb_sound_fade_hook = hook;
    jpb_sound_fade_user_data = user_data;
}

void jpb_SoundSetBankHook(
    JPBSoundBankHook hook, void *user_data)
{
    jpb_sound_bank_hook = hook;
    jpb_sound_bank_user_data = user_data;
}

void jpb_SoundSetSetupHook(
    JPBSoundSetupHook hook, void *user_data)
{
    jpb_sound_setup_hook = hook;
    jpb_sound_setup_user_data = user_data;
}

void jpb_SoundSetChannelHook(
    JPBSoundChannelHook hook, void *user_data)
{
    jpb_sound_channel_hook = hook;
    jpb_sound_channel_user_data = user_data;
}

void jpb_SoundSetControlHook(
    JPBSoundControlHook hook, void *user_data)
{
    jpb_sound_control_hook = hook;
    jpb_sound_control_user_data = user_data;
}

static void sound_platform_control(JPBSoundControl control)
{
    if (jpb_sound_control_hook != NULL) {
        jpb_sound_control_hook(
            control, jpb_sound_control_user_data);
    }
}

static int sound_platform_setup(
    JPBSoundSetupOperation operation,
    int value0,
    int value1,
    int value2,
    int value3)
{
    if (jpb_sound_setup_hook == NULL) {
        return operation == JPB_SOUND_SETUP_OPEN_AUDIO ? -1 : 0;
    }
    return jpb_sound_setup_hook(
        operation,
        value0,
        value1,
        value2,
        value3,
        jpb_sound_setup_user_data);
}

static void sound_platform_channel(
    JPBSoundChannelOperation operation,
    int channel,
    int value0,
    int value1)
{
    if (jpb_sound_channel_hook != NULL) {
        jpb_sound_channel_hook(
            operation,
            channel,
            value0,
            value1,
            jpb_sound_channel_user_data);
    }
}

/* 0x12A6D0, 49 bytes. */
const char *ExtractFileNameFromPath(const char *path)
{
    const char *ptr = path;
    const char *filename = path;

    while (*ptr != '\0') {
        if (*ptr == '/' || *ptr == '\\') {
            filename = ptr + 1;
        }
        ++ptr;
    }
    return filename;
}

/* 0x12A710, 101 bytes. */
void add_looped_sound_to_update(
    int channel,
    VECTOR *position,
    int leftVolume,
    int rightVolume)
{
    int idx = -1;
    int i;

    for (i = 0; i < SOUND_LOOPED_SOUND_COUNT; ++i) {
        if (!loopedSounds[i].isValid) {
            idx = i;
            break;
        }
    }
    loopedSounds[idx].channel = channel;
    loopedSounds[idx].isValid = 1;
    loopedSounds[idx].position = position;
    loopedSounds[idx].leftVolume = leftVolume;
    loopedSounds[idx].rightVolume = rightVolume;
}

/* 0x12A780, 37 bytes. */
VECTOR convert_svector_to_vector(_svector sv)
{
    VECTOR result = {
        (int32_t)sv.vx,
        (int32_t)sv.vy,
        (int32_t)sv.vz,
        0
    };

    return result;
}

/* 0x12A7B0, 432 bytes. */
void get_sound_volume(
    VECTOR listenerPos,
    VECTOR soundPos,
    uint8_t *out_ds,
    int *out_leftVol,
    int *out_rightVol)
{
    int difference_x = soundPos.vx - listenerPos.vx;
    int difference_z = soundPos.vz - listenerPos.vz;
    int16_t short_x = (int16_t)difference_x;
    int16_t short_z = (int16_t)difference_z;
    double distance = sqrt(
        (double)((int)short_x * (int)short_x +
                 (int)short_z * (int)short_z));
    int length = (int)distance - 1024;
    int fixed_distance;
    int mixer_distance;

    if (length < 0) {
        length = 0;
    } else if (length > 1536) {
        length = 1536;
    }
    fixed_distance = (int)(
        4096.0f - (float)(length << 12) / 2304.0f);
    if (fixed_distance < 0) {
        return;
    }
    if (fixed_distance > 16383) {
        fixed_distance = 16383;
    }
    mixer_distance = 256 - (int)(
        ((double)fixed_distance / 16383.0) * 1024.0);
    if (mixer_distance > 255) {
        mixer_distance = 255;
    }
    *out_ds = (uint8_t)mixer_distance;

    if (OptionStruct.Stereo == 0) {
        *out_leftVol = 255;
        *out_rightVol = 255;
    } else {
        int dot =
            (int)cameraFacing.vx * difference_x +
            (int)cameraFacing.vz * difference_z;
        int cross =
            (int)cameraFacing.vz * difference_x -
            (int)cameraFacing.vx * difference_z;
        double angle = atan2(-(double)dot, (double)cross);
        double pan = cos(angle);

        *out_leftVol = (int)((pan + 1.0) * 0.5 * 255.0);
        *out_rightVol = (int)((1.0 - pan) * 0.5 * 255.0);
    }
}

/* 0x12A960, 12 bytes. */
void mute_looped_sounds(void)
{
    loopedSoundMuted = 1;
    update_looped_sounds();
}

/* 0x12A970, exact three-byte bare return. */
void setCDXAvol(unsigned left, unsigned right)
{
    (void)left;
    (void)right;
}

/* 0x12A980, 153 bytes. */
void sound_FreeBank(int gabank)
{
    tBankHandle *bank = loadedBanks[gabank];
    size_t i;

    if (bank == NULL) {
        return;
    }
    if (jpb_sound_bank_hook != NULL) {
        (void)jpb_sound_bank_hook(
            gabank,
            NULL,
            NULL,
            0,
            0,
            jpb_sound_bank_user_data);
    }
    for (i = 0; i < (size_t)bank->count; ++i) {
        tSFXHandle *sfx = bank->loadedSFX[i];

        if (sfx != NULL) {
            if (sfx->ptrChunk != NULL &&
                jpb_sound_chunk_free_hook != NULL) {
                jpb_sound_chunk_free_hook(
                    sfx->ptrChunk,
                    jpb_sound_chunk_user_data);
            }
            free(sfx);
            bank->loadedSFX[i] = NULL;
        }
    }
    free(bank);
    loadedBanks[gabank] = NULL;
}

/* 0x12AA20, exact six-byte constant return. */
int sound_GetIndex(int *whichbank, char *name)
{
    (void)whichbank;
    (void)name;
    return -1;
}

/* 0x12AA30, exact three-byte constant return. */
int sound_GetSoundIndex(int *bankID, char *name)
{
    (void)bankID;
    (void)name;
    return 0;
}

/* 0x12AA40, 39 bytes. */
char *sound_GetSoundName(int bankID, int index)
{
    tBankHandle *bank = loadedBanks[bankID];
    tSFXHandle *sound;

    if (bank == NULL) {
        return NULL;
    }
    sound = bank->loadedSFX[index];
    if (sound == NULL) {
        return NULL;
    }
    return sound->chunkName;
}

/* 0x12AA70, 801 bytes. */
void sound_Init(void)
{
    int result = sound_platform_setup(
        JPB_SOUND_SETUP_INIT, 0, 0, 0, 0);
    int i;

    printf("[SOUND] Mix Init %d\n", result);
    result = sound_platform_setup(
        JPB_SOUND_SETUP_OPEN_AUDIO,
        44100,
        0x8120,
        2,
        8192);
    if (result >= 0) {
        result = sound_platform_setup(
            JPB_SOUND_SETUP_ALLOCATE_CHANNELS,
            64,
            0,
            0,
            0);
        printf("[SOUND] Mix Allocate Channles %d\n", result);
        (void)sound_LoadBank("resident/", 0);
    }
    for (i = 0; i < SOUND_LOOPED_SOUND_COUNT; ++i) {
        loopedSounds[i].isValid = 0;
    }
}

/* 0x12ADA0, exact three-byte constant return. */
int32_t sound_IsPlaying(uint16_t handle)
{
    (void)handle;
    return 0;
}

/* 0x12ADB0, 420 bytes. */
int sound_LoadBank(char *file, int gabank)
{
    size_t fileLength = strlen(file);
    char *bankName = _strnicmp(
        "jar_jar_playable", file, fileLength) == 0
        ? "gungan_2"
        : file;
    int bankIndex;
    const tAudioSFX_Bank *sourceBank;
    tBankHandle *bank;
    size_t i;

    if (gabank > SOUND_LOADED_BANK_WRITE_LIMIT || bankName == NULL) {
        return -1;
    }
    for (bankIndex = 0;
         bankIndex < AUDIO_SFX_BANK_COUNT;
         ++bankIndex) {
        if (_strnicmp(
                bankName,
                audioSFX_aSFXBanks[bankIndex].bankName,
                strlen(bankName)) == 0) {
            break;
        }
    }
    if (bankIndex == AUDIO_SFX_BANK_COUNT) {
        return -1;
    }
    sourceBank = &audioSFX_aSFXBanks[bankIndex];
    bank = loadedBanks[gabank];
    if (bank == NULL) {
        bank = (tBankHandle *)malloc(sizeof(*bank));
        loadedBanks[gabank] = bank;
        if (bank == NULL) {
            return -1;
        }
    }
    if (jpb_sound_bank_hook != NULL) {
        (void)jpb_sound_bank_hook(
            gabank,
            sourceBank->bankName,
            sourceBank->ptrSFXNames,
            sourceBank->numSFXs,
            1,
            jpb_sound_bank_user_data);
    }
    for (i = 0; i < (size_t)sourceBank->numSFXs; ++i) {
        const char *fullFilePath = resource_getPath(
            sourceBank->ptrSFXNames[i],
            JPB_RESOURCE_SOUND_SFX_FINAL);
        tSFXHandle *sfx;

        (void)fullFilePath;
        if (bank == NULL) {
            continue;
        }
        sfx = (tSFXHandle *)malloc(sizeof(*sfx));
        bank->loadedSFX[i] = sfx;
        if (sfx == NULL) {
            return -1;
        }
        sfx->ptrChunk = jpb_sound_chunk_load_hook != NULL
            ? jpb_sound_chunk_load_hook(
                fullFilePath,
                jpb_sound_chunk_user_data)
            : NULL;
        sfx->chunkName = (char *)sourceBank->ptrSFXNames[i];
    }
    if (bank != NULL) {
        bank->count = sourceBank->numSFXs;
    }
    return 0;
}

/* 0x12AF60, 73 bytes. */
int sound_NumInBank(int bankID)
{
    tBankHandle *bank = loadedBanks[bankID];
    int count = 0;
    int i;

    if (bank != NULL) {
        for (i = 0; i < bank->count; ++i) {
            if (bank->loadedSFX[i] != NULL) {
                ++count;
            }
        }
    }
    return count;
}

/* 0x12AFB0, tail call to Mix_PauseMusic. */
void sound_Pause(void)
{
    sound_platform_control(JPB_SOUND_CONTROL_PAUSE_MUSIC);
}

/* 0x12AFC0, exact 131-byte bank-cascade wrapper. */
uint16_t sound_Play(
    VECTOR *position, int bankId, char *sound, uint32_t flag)
{
    uint16_t result = sound_playSfx(
        position, bankId, sound, flag);

    if (result != 0) {
        return result;
    }
    if (bankId != 3) {
        result = sound_playSfx(position, 3, sound, flag);
        if (result != 0) {
            return result;
        }
        if (bankId == 0) {
            return 0;
        }
        (void)sound_playSfx(position, 0, sound, flag);
    }
    return sound_playSfx(position, 0, sound, flag);
}

/* 0x12B050, exact tail call to sound_Play. */
uint16_t sound_PlayController(
    VECTOR *position, int bankId, char *sound, uint32_t flag)
{
    return sound_Play(position, bankId, sound, flag);
}

/* 0x12B060, 171 bytes. */
uint16_t sound_PlayFV(
    FVECTOR *position, int bankId, char *sound, uint32_t flag)
{
    VECTOR integer_position;

    integer_position.vx = (int32_t)position->vx;
    integer_position.vy = (int32_t)position->vy;
    integer_position.vz = (int32_t)position->vz;

    return sound_Play(&integer_position, bankId, sound, flag);
}

/* 0x12B110, calls sound_Play and then clears EAX. */
uint16_t sound_PlaySV(
    _svector *position, int bankId, char *sound, uint32_t flag)
{
    (void)sound_Play(
        (VECTOR *)position, bankId, sound, flag);
    return 0;
}

/* 0x12B120, exact three-byte constant return. */
int sound_Resume(void)
{
    return 0;
}

/* 0x12B130, exact three-byte bare return. */
void sound_SetFrequency(uint16_t handle, uint32_t frequency)
{
    (void)handle;
    (void)frequency;
}

/* 0x12B140, tail call to Mix_FadeOutChannel. */
void sound_SetLoopingFadeTime(
    uint16_t handle, uint32_t fade_time)
{
    if (jpb_sound_fade_hook != NULL) {
        jpb_sound_fade_hook(
            handle, fade_time, jpb_sound_fade_user_data);
    }
}

/* 0x12B150, exact three-byte bare return. */
void sound_SetPosition(uint16_t handle, VECTOR *pos)
{
    (void)handle;
    (void)pos;
}

/* 0x12B160, tail call to Mix_HaltMusic. */
void sound_StopAll(void)
{
    sound_platform_control(JPB_SOUND_CONTROL_HALT_MUSIC);
}

/* 0x12B170, 588 bytes; clears every matching loop-table entry. */
void sound_StopSound(uint16_t handle)
{
    size_t index;

    if (jpb_sound_stop_hook != NULL) {
        jpb_sound_stop_hook(
            handle, jpb_sound_stop_user_data);
    }
    for (index = 0; index < SOUND_LOOPED_SOUND_COUNT; ++index) {
        if (loopedSounds[index].isValid != 0 &&
            loopedSounds[index].channel == (int)handle) {
            loopedSounds[index].isValid = 0;
        }
    }
}

/* 0x12B3C0, exact three-byte constant return. */
int32_t sound_UnLoadBank(_sound_Bank *bank)
{
    (void)bank;
    return 0;
}

/* 0x12B3D0, exact three-byte bare return. */
void sound_UpdateAll(int deltatime)
{
    (void)deltatime;
}

/* 0x12B3E0, 1040-byte mixer/file owner behind the platform seam. */
uint16_t sound_playSfx(
    VECTOR *position, int bankId, char *sound, uint32_t flag)
{
    static const char *const loopingSounds[6] = {
        "fan_big",
        "pistloop",
        "elev1lp",
        "stapstdy",
        "tanksty1",
        "taxiloop"
    };
    char soundWithExtension[256];
    char tempPath[256];
    char *originalSound = sound;
    char *name;
    int quieten = 0;
    int is2D;
    uint8_t ds = 0;
    int leftVolume = 128;
    int rightVolume = 128;
    tBankHandle *bank;
    size_t i;
    size_t j;
    int loop = 0;
    int result;

    sprintf(soundWithExtension, "%s.wav", sound);
    if (sound == NULL || *sound == '\0' ||
        (sound_Paused != 0 && (flag & 8U) == 0)) {
        return 0;
    }
    if (*sound == '-') {
        ++sound;
        quieten = 1;
    }
    name = *sound == '!' ? sound + 1 : sound;

    if (position != NULL && name[0] != 'z' && name[0] != 'v') {
        is2D = *sound == '!';
        if (*sound != '!') {
            VECTOR listener = convert_svector_to_vector(cameraLocation);

            get_sound_volume(
                listener,
                *position,
                &ds,
                &leftVolume,
                &rightVolume);
        } else {
            leftVolume = 200;
            rightVolume = 200;
            ds = 0;
        }
    } else {
        is2D = 1;
        leftVolume = 200;
        rightVolume = 200;
        ds = 0;
    }
    if (name[0] == 'v') {
        leftVolume = (leftVolume * 0x1333) >> 12;
        if (leftVolume > 128) {
            leftVolume = 128;
        }
        rightVolume = (rightVolume * 0x1333) >> 12;
        if (rightVolume > 128) {
            rightVolume = 128;
        }
    } else if (quieten != 0) {
        leftVolume = (leftVolume * 0x333) >> 12;
        rightVolume = (rightVolume * 0x333) >> 12;
    }
    if (name[0] != 'x') {
        (void)rand();
    }

    bank = loadedBanks[bankId];
    if (bank == NULL) {
        return 0;
    }
    for (j = 0; j < (size_t)bank->count; ++j) {
        tSFXHandle *loaded = bank->loadedSFX[j];

        if (loaded != NULL && loaded->chunkName != NULL) {
            const char *filename;

            strncpy(tempPath, loaded->chunkName, sizeof(tempPath));
            tempPath[sizeof(tempPath) - 1] = '\0';
            filename = ExtractFileNameFromPath(tempPath);
            if (strcmp(filename, soundWithExtension) == 0) {
                break;
            }
        }
    }
    if (j == (size_t)bank->count) {
        return 0;
    }
    for (i = 0; i < 6; ++i) {
        if (strcmp(name, loopingSounds[i]) == 0) {
            loop = -1;
            break;
        }
    }
    result = jpb_sound_play_sfx_hook != NULL
        ? (int)jpb_sound_play_sfx_hook(
            bank->loadedSFX[j]->ptrChunk,
            loop,
            position,
            bankId,
            originalSound,
            flag,
            jpb_sound_play_sfx_user_data)
        : -1;
    if ((uint16_t)result == UINT16_MAX) {
        return UINT16_MAX;
    }
    sound_platform_channel(
        JPB_SOUND_CHANNEL_PANNING,
        result,
        (uint8_t)leftVolume,
        (uint8_t)rightVolume);
    if (!is2D) {
        sound_platform_channel(
            JPB_SOUND_CHANNEL_DISTANCE,
            result,
            ds,
            0);
        if (loop == -1) {
            add_looped_sound_to_update(
                result,
                position,
                leftVolume,
                rightVolume);
        }
    }
    sound_platform_channel(
        JPB_SOUND_CHANNEL_VOLUME,
        result,
        (int)((float)OptionStruct.SFXVolume *
              (is2D != 0 ? 0.92f : 1.0f)),
        0);
    return (uint16_t)result;
}

/* 0x12B7F0, 60 bytes. */
void stop_all_looped_sounds(void)
{
    size_t index;

    for (index = 0; index < SOUND_LOOPED_SOUND_COUNT; ++index) {
        if (loopedSounds[index].isValid != 0) {
            if (jpb_sound_stop_hook != NULL) {
                jpb_sound_stop_hook(
                    (uint16_t)loopedSounds[index].channel,
                    jpb_sound_stop_user_data);
            }
            loopedSounds[index].isValid = 0;
        }
    }
}

/* 0x12B830, 12 bytes. */
void unmute_looped_sounds(void)
{
    loopedSoundMuted = 0;
    update_looped_sounds();
}

/* 0x12B840, 747 bytes. */
void update_looped_sounds(void)
{
    VECTOR listener = convert_svector_to_vector(cameraLocation);
    size_t index;

    for (index = 0; index < SOUND_LOOPED_SOUND_COUNT; ++index) {
        LoopedSound *entry = &loopedSounds[index];

        if (entry->isValid != 0) {
            uint8_t distance;
            int leftVolume;
            int rightVolume;

            get_sound_volume(
                listener,
                *entry->position,
                &distance,
                &leftVolume,
                &rightVolume);
            sound_platform_channel(
                JPB_SOUND_CHANNEL_PANNING,
                entry->channel,
                (uint8_t)leftVolume,
                (uint8_t)rightVolume);
            sound_platform_channel(
                JPB_SOUND_CHANNEL_DISTANCE,
                entry->channel,
                distance,
                0);
            sound_platform_channel(
                loopedSoundMuted != 0
                    ? JPB_SOUND_CHANNEL_PAUSE
                    : JPB_SOUND_CHANNEL_RESUME,
                entry->channel,
                0,
                0);
        }
    }
}
