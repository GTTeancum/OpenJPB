/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\win32\sound.c.
 *
 * Gameplay-visible wrappers, bank ownership, the 100-entry positional-loop
 * table, and retail stubs retain their exact PDB names. SDL_mixer calls enter
 * through game-owned callbacks; the PC host binds a dependency-free WinMM
 * adapter without leaking a desktop API into this original module.
 *
 * Provenance:
 *   direct/decompiled - PDB module 0099, its 31 procedures, globals,
 *     signatures, parameter names, and the 43-entry bank table.
 *   assembly - exact tail calls/stubs, fallback order, loop-table layout,
 *     volume arithmetic, and pause/halt behavior at RVAs 0x12A6D0..0x12BB2B.
 *   substituted - allocation, WAV loading, and mixer calls behind jpb_ hooks.
 */

#include "jpb/sound.h"

#include "jpb/camera.h"
#include "jpb/game.h"
#include "jpb/sound_bank_data.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

enum {
    SOUND_LOADED_BANK_COUNT = 5,
    SOUND_LOOPED_SOUND_COUNT = 100
};

/* Exact 24-byte x64 record at matched address 0x140932D00. */
typedef struct SoundLoopedSound {
    uint8_t active;
    uint8_t padding[3];
    int channel;
    VECTOR *position;
    int leftVolume;
    int rightVolume;
} SoundLoopedSound;

_Static_assert(
    sizeof(SoundLoopedSound) == 24,
    "loopedSounds entry must match the PDB/matched x64 layout");

int sound_Paused;
uint8_t loopedSoundMuted;

static const JPBSoundBankData
    *loadedBanks[SOUND_LOADED_BANK_COUNT];
static SoundLoopedSound loopedSounds[SOUND_LOOPED_SOUND_COUNT];

static JPBSoundPlaySfxHook jpb_sound_play_sfx_hook;
static void *jpb_sound_play_sfx_user_data;
static JPBSoundStopHook jpb_sound_stop_hook;
static void *jpb_sound_stop_user_data;
static JPBSoundFadeHook jpb_sound_fade_hook;
static void *jpb_sound_fade_user_data;
static JPBSoundBankHook jpb_sound_bank_hook;
static void *jpb_sound_bank_user_data;
static JPBSoundControlHook jpb_sound_control_hook;
static void *jpb_sound_control_user_data;

void jpb_SoundSetPlaySfxHook(
    JPBSoundPlaySfxHook hook, void *user_data)
{
    jpb_sound_play_sfx_hook = hook;
    jpb_sound_play_sfx_user_data = user_data;
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

static int sound_ascii_lower(int value)
{
    if (value >= 'A' && value <= 'Z') {
        return value + ('a' - 'A');
    }
    return value;
}

static int sound_prefix_equal_case_insensitive(
    const char *left, const char *right, size_t count)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        unsigned char left_character = (unsigned char)left[index];
        unsigned char right_character = (unsigned char)right[index];

        if (sound_ascii_lower(left_character) !=
            sound_ascii_lower(right_character)) {
            return 0;
        }
        if (left_character == '\0' || right_character == '\0') {
            return left_character == right_character;
        }
    }
    return 1;
}

static int32_t sound_trunc_float_to_i32(float value)
{
    if (!(value >= -2147483648.0f && value < 2147483648.0f)) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static const char *sound_unprefixed_name(const char *sound)
{
    if (sound == NULL) {
        return NULL;
    }
    if (*sound == '-') {
        ++sound;
    }
    if (*sound == '!') {
        ++sound;
    }
    return *sound != '\0' ? sound : NULL;
}

static int sound_name_is_looped(const char *sound)
{
    static const char *const loopingSounds[] = {
        "fan_big",
        "pistloop",
        "elev1lp",
        "stapstdy",
        "tanksty1",
        "taxiloop"
    };
    size_t index;

    for (index = 0;
         index < sizeof(loopingSounds) / sizeof(loopingSounds[0]);
         ++index) {
        if (strcmp(sound, loopingSounds[index]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x12A6D0, 49 bytes. */
const char *ExtractFileNameFromPath(const char *path)
{
    const char *filename = path;

    if (path == NULL) {
        return NULL;
    }
    while (*path != '\0') {
        if (*path == '/' || *path == '\\') {
            filename = path + 1;
        }
        ++path;
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
    size_t index;

    for (index = 0; index < SOUND_LOOPED_SOUND_COUNT; ++index) {
        SoundLoopedSound *entry = &loopedSounds[index];

        if (entry->active == 0) {
            entry->channel = channel;
            entry->active = 1;
            entry->position = position;
            entry->leftVolume = leftVolume;
            entry->rightVolume = rightVolume;
            return;
        }
    }
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

    if (out_ds == NULL || out_leftVol == NULL ||
        out_rightVol == NULL) {
        return;
    }
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
        double pan = sin(atan2(-(double)dot, (double)cross));

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

/* 0x12A970, 3-byte retail stub. */
void setCDXAvol(unsigned left, unsigned right)
{
    (void)left;
    (void)right;
}

/* 0x12A980, 153 bytes. */
void sound_FreeBank(int gabank)
{
    const JPBSoundBankData *bank;

    if (gabank < 0 || gabank >= SOUND_LOADED_BANK_COUNT) {
        return;
    }
    bank = loadedBanks[gabank];
    if (bank == NULL) {
        return;
    }
    if (jpb_sound_bank_hook != NULL) {
        (void)jpb_sound_bank_hook(
            gabank,
            bank->directory,
            bank->paths,
            bank->count,
            0,
            jpb_sound_bank_user_data);
    }
    loadedBanks[gabank] = NULL;
}

/* 0x12AA20, exact six-byte retail stub. */
int sound_GetIndex(int *whichbank, char *name)
{
    (void)whichbank;
    (void)name;
    return -1;
}

/* 0x12AA30, exact three-byte retail stub. */
int sound_GetSoundIndex(int *bankID, char *name)
{
    (void)bankID;
    (void)name;
    return 0;
}

/* 0x12AA40, 39 bytes. */
char *sound_GetSoundName(int bankID, int index)
{
    const JPBSoundBankData *bank;

    if (bankID < 0 || bankID >= SOUND_LOADED_BANK_COUNT ||
        index < 0) {
        return NULL;
    }
    bank = loadedBanks[bankID];
    if (bank == NULL || index >= bank->count) {
        return NULL;
    }
    return (char *)bank->paths[index];
}

/* 0x12AA70, 801 bytes. */
void sound_Init(void)
{
    int bank;

    for (bank = 0; bank < SOUND_LOADED_BANK_COUNT; ++bank) {
        sound_FreeBank(bank);
    }
    memset(loopedSounds, 0, sizeof(loopedSounds));
    loopedSoundMuted = 0;
    (void)sound_LoadBank("resident/", 0);
}

/* 0x12ADA0, exact three-byte retail stub. */
int32_t sound_IsPlaying(uint16_t handle)
{
    (void)handle;
    return 0;
}

/* 0x12ADB0, 420 bytes. */
int sound_LoadBank(char *file, int gabank)
{
    const char *requested;
    const JPBSoundBankData *bank = NULL;
    size_t length;
    size_t index;

    if (file == NULL || gabank < 0 ||
        gabank >= SOUND_LOADED_BANK_COUNT) {
        return -1;
    }
    length = strlen(file);
    requested = sound_prefix_equal_case_insensitive(
        "jar_jar_playable", file, length)
        ? "gungan_2"
        : file;
    length = strlen(requested);
    for (index = 0; index < JPB_SOUND_BANK_TABLE_COUNT; ++index) {
        if (sound_prefix_equal_case_insensitive(
                requested,
                jpb_soundBankTable[index].directory,
                length)) {
            bank = &jpb_soundBankTable[index];
            break;
        }
    }
    if (bank == NULL) {
        return -1;
    }
    if (jpb_sound_bank_hook != NULL &&
        !jpb_sound_bank_hook(
            gabank,
            bank->directory,
            bank->paths,
            bank->count,
            1,
            jpb_sound_bank_user_data)) {
        return -1;
    }
    loadedBanks[gabank] = bank;
    return 0;
}

/* 0x12AF60, 73 bytes. */
int sound_NumInBank(int bankID)
{
    if (bankID < 0 || bankID >= SOUND_LOADED_BANK_COUNT ||
        loadedBanks[bankID] == NULL) {
        return 0;
    }
    return loadedBanks[bankID]->count;
}

/* 0x12AFB0, tail call to Mix_PauseMusic. */
void sound_Pause(void)
{
    sound_platform_control(JPB_SOUND_CONTROL_PAUSE_MUSIC);
}

/* 0x12AFC0, exact 131-byte bank-fallback wrapper. */
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
    VECTOR integer_position = {
        sound_trunc_float_to_i32(position->vx),
        sound_trunc_float_to_i32(position->vy),
        sound_trunc_float_to_i32(position->vz),
        0
    };

    return sound_Play(
        &integer_position, bankId, sound, flag);
}

/* 0x12B110, calls sound_Play and then clears EAX. */
uint16_t sound_PlaySV(
    _svector *position, int bankId, char *sound, uint32_t flag)
{
    (void)sound_Play(
        (VECTOR *)position, bankId, sound, flag);
    return 0;
}

/* 0x12B120, exact three-byte retail stub. */
int sound_Resume(void)
{
    return 0;
}

/* 0x12B130, exact three-byte retail stub. */
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

/* 0x12B150, exact three-byte retail stub. */
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
        if (loopedSounds[index].active != 0 &&
            loopedSounds[index].channel == (int)handle) {
            loopedSounds[index].active = 0;
        }
    }
}

/* 0x12B3C0, exact three-byte retail stub. */
int32_t sound_UnLoadBank(_sound_Bank *bank)
{
    (void)bank;
    return 0;
}

/* 0x12B3D0, exact three-byte retail stub. */
void sound_UpdateAll(int deltatime)
{
    (void)deltatime;
}

/* 0x12B3E0, 1040-byte mixer/file owner behind the platform seam. */
uint16_t sound_playSfx(
    VECTOR *position, int bankId, char *sound, uint32_t flag)
{
    const char *name = sound_unprefixed_name(sound);
    const char *prefix_cursor = sound;
    uint16_t handle;

    if (name == NULL ||
        (sound_Paused != 0 && (flag & 8U) == 0) ||
        jpb_sound_play_sfx_hook == NULL) {
        return 0;
    }
    handle = jpb_sound_play_sfx_hook(
        position,
        bankId,
        sound,
        flag,
        jpb_sound_play_sfx_user_data);
    if (handle != 0 && handle != UINT16_MAX && position != NULL) {
        VECTOR listener;
        uint8_t distance;
        int left = 128;
        int right = 128;

        if (*prefix_cursor == '-') {
            ++prefix_cursor;
        }
        if (*prefix_cursor != '!' &&
            name[0] != 'z' && name[0] != 'v' &&
            sound_name_is_looped(name)) {
            listener = convert_svector_to_vector(cameraLocation);
            get_sound_volume(
                listener, *position, &distance, &left, &right);
            add_looped_sound_to_update(
                (int)handle, position, left, right);
        }
    }
    return handle;
}

/* 0x12B7F0, 60 bytes. */
void stop_all_looped_sounds(void)
{
    size_t index;

    for (index = 0; index < SOUND_LOOPED_SOUND_COUNT; ++index) {
        if (loopedSounds[index].active != 0) {
            sound_StopSound(
                (uint16_t)loopedSounds[index].channel);
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
        SoundLoopedSound *entry = &loopedSounds[index];

        if (entry->active != 0 && entry->position != NULL) {
            uint8_t distance;

            get_sound_volume(
                listener,
                *entry->position,
                &distance,
                &entry->leftVolume,
                &entry->rightVolume);
        }
    }
    sound_platform_control(JPB_SOUND_CONTROL_UPDATE_LOOPED);
    sound_platform_control(
        loopedSoundMuted != 0
            ? JPB_SOUND_CONTROL_MUTE_LOOPED
            : JPB_SOUND_CONTROL_UNMUTE_LOOPED);
}
