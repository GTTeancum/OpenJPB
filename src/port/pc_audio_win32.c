/*
 * Dependency-free Win32 audio host for the reconstructed sound scheduler.
 *
 * Provenance:
 *   direct/decompiled - sound_playSfx uses 64 SDL_mixer channels, six exact
 *     looping names, '-' quiet and '!' non-spatial prefixes, voice/non-
 *     spatial classification, bank lookup by basename, and live looped-
 *     position updates in the matched executable.
 *   direct/decompiled - sound_LoadBank maps level 15 to corus1, levels
 *     16..22 to the seven-entry training_level bank, and
 *     jar_jar_playable to gungan_2.
 *   substituted - WinMM waveOut voices replace SDL_mixer on the PC host.
 *     Gameplay code remains independent of both APIs.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>

#include "jpb/pc_audio_win32.h"

#include "jpb/audio_stream.h"
#include "jpb/camera.h"
#include "jpb/game.h"
#include "jpb/level_world.h"
#include "jpb/sound.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    JPB_PC_AUDIO_VOICE_COUNT = 64,
    JPB_PC_AUDIO_PATH_CAPACITY = 1024
};

typedef struct JPBPCAudioVoice {
    HWAVEOUT output;
    WAVEHDR header;
    unsigned char *fileBytes;
    VECTOR *loopPosition;
    int prepared;
    int active;
    int looping;
    int quiet;
    int nonSpatial;
    int voiceSound;
    ULONGLONG fadeStart;
    uint32_t fadeDuration;
} JPBPCAudioVoice;

struct JPBPCAudio {
    char soundRoot[JPB_PC_AUDIO_PATH_CAPACITY];
    char streamRoot[JPB_PC_AUDIO_PATH_CAPACITY];
    char playerBank[2][64];
    char levelBank[64];
    const char *const *bankPaths[5];
    int bankPathCount[5];
    JPBPCAudioVoice voices[JPB_PC_AUDIO_VOICE_COUNT];
    JPBPCAudioVoice music;
    char currentMusicPath[JPB_PC_AUDIO_PATH_CAPACITY];
    int musicPaused;
    int musicVolume;
    JPBPCAudioStats stats;
    int outputEnabled;
};

static const char *const jpb_looping_sounds[] = {
    "fan_big",
    "pistloop",
    "elev1lp",
    "stapstdy",
    "tanksty1",
    "taxiloop"
};

static uint16_t pc_audio_u16(const unsigned char *bytes)
{
    return (uint16_t)(
        (uint16_t)bytes[0] |
        ((uint16_t)bytes[1] << 8));
}

static uint32_t pc_audio_u32(const unsigned char *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

int jpb_PCAudioInspectWavMemory(
    const void *memory,
    size_t size,
    JPBPCAudioWavInfo *info)
{
    const unsigned char *bytes =
        (const unsigned char *)memory;
    size_t cursor = 12;
    int found_format = 0;
    int found_data = 0;

    if (bytes == NULL || info == NULL || size < 12 ||
        memcmp(bytes, "RIFF", 4) != 0 ||
        memcmp(bytes + 8, "WAVE", 4) != 0) {
        return 0;
    }
    memset(info, 0, sizeof(*info));
    while (cursor <= size - 8) {
        const unsigned char *chunk = bytes + cursor;
        uint32_t chunk_size = pc_audio_u32(chunk + 4);
        size_t data_offset = cursor + 8;
        size_t next;

        if ((size_t)chunk_size > size - data_offset) {
            return 0;
        }
        if (memcmp(chunk, "fmt ", 4) == 0) {
            if (chunk_size < 16) {
                return 0;
            }
            info->formatTag = pc_audio_u16(bytes + data_offset);
            info->channels = pc_audio_u16(bytes + data_offset + 2);
            info->sampleRate = pc_audio_u32(bytes + data_offset + 4);
            info->averageBytesPerSecond =
                pc_audio_u32(bytes + data_offset + 8);
            info->blockAlign = pc_audio_u16(bytes + data_offset + 12);
            info->bitsPerSample = pc_audio_u16(bytes + data_offset + 14);
            found_format = 1;
        } else if (memcmp(chunk, "data", 4) == 0) {
            if (data_offset > UINT32_MAX || chunk_size == 0) {
                return 0;
            }
            info->dataOffset = (uint32_t)data_offset;
            info->dataSize = chunk_size;
            found_data = 1;
        }
        next = data_offset + (size_t)chunk_size;
        if ((chunk_size & 1U) != 0) {
            if (next == size) {
                break;
            }
            ++next;
        }
        if (next <= cursor || next > size) {
            return 0;
        }
        cursor = next;
    }
    if (!found_format || !found_data ||
        (info->formatTag != WAVE_FORMAT_PCM &&
         info->formatTag != WAVE_FORMAT_IEEE_FLOAT) ||
        (info->channels != 1 && info->channels != 2) ||
        info->sampleRate == 0 ||
        info->averageBytesPerSecond == 0 ||
        info->blockAlign == 0 ||
        info->bitsPerSample == 0 ||
        info->dataSize % info->blockAlign != 0) {
        return 0;
    }
    return 1;
}

static unsigned char *pc_audio_read_file(
    const char *path,
    size_t *size_out)
{
    FILE *file;
    long length;
    unsigned char *bytes;

    if (path == NULL || size_out == NULL) {
        return NULL;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0 ||
        (length = ftell(file)) <= 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    bytes = (unsigned char *)malloc((size_t)length);
    if (bytes == NULL ||
        fread(bytes, 1, (size_t)length, file) != (size_t)length) {
        free(bytes);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *size_out = (size_t)length;
    return bytes;
}

int jpb_PCAudioInspectWavFile(
    const char *path,
    JPBPCAudioWavInfo *info)
{
    size_t size = 0;
    unsigned char *bytes = pc_audio_read_file(path, &size);
    int result;

    if (bytes == NULL) {
        return 0;
    }
    result = jpb_PCAudioInspectWavMemory(bytes, size, info);
    free(bytes);
    return result;
}

static int pc_audio_copy_string(
    char *destination,
    size_t capacity,
    const char *source,
    size_t length)
{
    if (destination == NULL || source == NULL ||
        capacity == 0 || length >= capacity) {
        return 0;
    }
    memcpy(destination, source, length);
    destination[length] = '\0';
    return 1;
}

static int pc_audio_parent_directory(
    char *path,
    size_t capacity)
{
    size_t length;

    (void)capacity;
    if (path == NULL || path[0] == '\0') {
        return 0;
    }
    length = strlen(path);
    while (length > 0 &&
           (path[length - 1] == '\\' || path[length - 1] == '/')) {
        path[--length] = '\0';
    }
    while (length > 0 &&
           path[length - 1] != '\\' && path[length - 1] != '/') {
        --length;
    }
    if (length == 0) {
        return 0;
    }
    while (length > 0 &&
           (path[length - 1] == '\\' || path[length - 1] == '/')) {
        --length;
    }
    path[length] = '\0';
    return length != 0;
}

static int pc_audio_join(
    char *destination,
    size_t capacity,
    const char *left,
    const char *right)
{
    int written;
    size_t length;
    char separator = '\\';

    if (destination == NULL || left == NULL || right == NULL ||
        capacity == 0) {
        return 0;
    }
    length = strlen(left);
    if (length != 0 &&
        (left[length - 1] == '\\' || left[length - 1] == '/')) {
        separator = '\0';
    }
    written = separator != '\0'
        ? snprintf(destination, capacity, "%s%c%s", left, separator, right)
        : snprintf(destination, capacity, "%s%s", left, right);
    return written >= 0 && (size_t)written < capacity;
}

static int pc_audio_bank_from_cad(
    const char *path,
    char *bank,
    size_t capacity)
{
    const char *name;
    const char *cursor;
    const char *extension = NULL;
    size_t length;

    if (path == NULL || bank == NULL) {
        return 0;
    }
    name = path;
    for (cursor = path; *cursor != '\0'; ++cursor) {
        if (*cursor == '\\' || *cursor == '/') {
            name = cursor + 1;
            extension = NULL;
        } else if (*cursor == '.') {
            extension = cursor;
        }
    }
    length = extension != NULL
        ? (size_t)(extension - name)
        : strlen(name);
    if (!pc_audio_copy_string(bank, capacity, name, length)) {
        return 0;
    }
    if (_stricmp(bank, "jar_jar_playable") == 0) {
        return pc_audio_copy_string(
            bank, capacity, "gungan_2", strlen("gungan_2"));
    }
    return bank[0] != '\0';
}

static int pc_audio_level_bank(
    int level_index,
    char *bank,
    size_t capacity)
{
    const char *name;

    if (level_index == 15) {
        name = "corus1";
    } else if (level_index >= 16 && level_index < 23) {
        name = "training_level";
    } else if (level_index >= 0 &&
               level_index < JPB_LEVEL_NAME_COUNT) {
        name = sLevelNames[level_index];
    } else {
        return 0;
    }
    return pc_audio_copy_string(
        bank, capacity, name, strlen(name));
}

static const char *pc_audio_sound_name(const char *sound)
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

int jpb_PCAudioResolveSound(
    const JPBPCAudio *audio,
    int bank_id,
    const char *sound,
    char *path,
    size_t path_capacity)
{
    const char *name = pc_audio_sound_name(sound);
    char filename[320];
    int index;
    int written;

    if (audio == NULL || name == NULL || path == NULL ||
        path_capacity == 0 || strchr(name, '\\') != NULL ||
        strchr(name, '/') != NULL) {
        return 0;
    }
    if (bank_id < 0 || bank_id >= 5 ||
        audio->bankPaths[bank_id] == NULL) {
        return 0;
    }
    written = snprintf(filename, sizeof(filename), "%s.wav", name);
    if (written < 0 || (size_t)written >= sizeof(filename)) {
        return 0;
    }
    for (index = 0; index < audio->bankPathCount[bank_id]; ++index) {
        const char *authored_path = audio->bankPaths[bank_id][index];
        const char *basename = ExtractFileNameFromPath(authored_path);

        if (basename != NULL && strcmp(basename, filename) == 0) {
            char relative_path[JPB_PC_AUDIO_PATH_CAPACITY];
            size_t path_index;

            if (!pc_audio_copy_string(
                    relative_path,
                    sizeof(relative_path),
                    authored_path,
                    strlen(authored_path))) {
                return 0;
            }
            for (path_index = 0;
                 relative_path[path_index] != '\0';
                 ++path_index) {
                if (relative_path[path_index] == '/') {
                    relative_path[path_index] = '\\';
                }
            }
            return pc_audio_join(
                path,
                path_capacity,
                audio->soundRoot,
                relative_path);
        }
    }
    return 0;
}

int jpb_PCAudioResolveStream(
    const JPBPCAudio *audio,
    const char *stream_name,
    char *path,
    size_t path_capacity)
{
    if (audio == NULL || stream_name == NULL ||
        stream_name[0] == '\0' || path == NULL ||
        path_capacity == 0 || strchr(stream_name, '\\') != NULL ||
        strchr(stream_name, '/') != NULL) {
        return 0;
    }
    return pc_audio_join(
        path, path_capacity, audio->streamRoot, stream_name);
}

static int pc_audio_is_looping(const char *sound)
{
    size_t index;

    for (index = 0;
         index < sizeof(jpb_looping_sounds) /
                     sizeof(jpb_looping_sounds[0]);
         ++index) {
        if (strcmp(sound, jpb_looping_sounds[index]) == 0) {
            return 1;
        }
    }
    return 0;
}

static void pc_audio_release_voice(JPBPCAudioVoice *voice)
{
    if (voice == NULL) {
        return;
    }
    if (voice->output != NULL) {
        if (voice->active) {
            (void)waveOutReset(voice->output);
        }
        if (voice->prepared) {
            (void)waveOutUnprepareHeader(
                voice->output, &voice->header, sizeof(voice->header));
        }
        (void)waveOutClose(voice->output);
    }
    free(voice->fileBytes);
    memset(voice, 0, sizeof(*voice));
}

static void pc_audio_gains(
    const VECTOR *position,
    int quiet,
    int non_spatial,
    int voice_sound,
    int looping,
    float *left_gain,
    float *right_gain)
{
    float left = 200.0f;
    float right = 200.0f;
    float mixer_volume;

    if (!non_spatial && position != NULL) {
        VECTOR listener = convert_svector_to_vector(cameraLocation);
        uint8_t distance = 0;
        int left_volume = 128;
        int right_volume = 128;
        float attenuation;

        get_sound_volume(
            listener,
            *position,
            &distance,
            &left_volume,
            &right_volume);
        attenuation = (255.0f - (float)distance) / 255.0f;
        left = (float)left_volume * attenuation;
        right = (float)right_volume * attenuation;
    }
    if (voice_sound) {
        left *= 1.2f;
        right *= 1.2f;
        if (left > 128.0f) {
            left = 128.0f;
        }
        if (right > 128.0f) {
            right = 128.0f;
        }
    } else if (quiet) {
        left *= 0.2f;
        right *= 0.2f;
    }
    /*
     * Exact sound_playSfx Mix_Volume arithmetic: ordinary channels use the
     * executable's 0.92 multiplier, while the six infinite loops use 1.0.
     * SDL_mixer clamps its channel volume to MIX_MAX_VOLUME (128).
     */
    mixer_volume = (float)(int)(
        (float)OptionStruct.SFXVolume *
        (looping ? 1.0f : 0.92f));
    if (mixer_volume > 128.0f) {
        mixer_volume = 128.0f;
    }
    *left_gain = (left / 255.0f) * (mixer_volume / 128.0f);
    *right_gain = (right / 255.0f) * (mixer_volume / 128.0f);
}

static void pc_audio_set_voice_volume(
    JPBPCAudioVoice *voice)
{
    float left;
    float right;
    DWORD volume;
    unsigned left_word;
    unsigned right_word;
    float fade = 1.0f;

    if (voice == NULL || voice->output == NULL) {
        return;
    }
    pc_audio_gains(
        voice->loopPosition,
        voice->quiet,
        voice->nonSpatial,
        voice->voiceSound,
        voice->looping,
        &left,
        &right);
    if (voice->fadeDuration != 0) {
        ULONGLONG elapsed = GetTickCount64() - voice->fadeStart;

        if (elapsed >= voice->fadeDuration) {
            fade = 0.0f;
        } else {
            fade = 1.0f -
                (float)elapsed / (float)voice->fadeDuration;
        }
    }
    left *= fade;
    right *= fade;
    left_word = (unsigned)(left * 65535.0f);
    right_word = (unsigned)(right * 65535.0f);
    if (left_word > 65535U) {
        left_word = 65535U;
    }
    if (right_word > 65535U) {
        right_word = 65535U;
    }
    volume = (DWORD)(left_word | (right_word << 16));
    (void)waveOutSetVolume(voice->output, volume);
}

static void pc_audio_reap(JPBPCAudio *audio)
{
    size_t index;

    if (audio == NULL) {
        return;
    }
    for (index = 0; index < JPB_PC_AUDIO_VOICE_COUNT; ++index) {
        JPBPCAudioVoice *voice = &audio->voices[index];

        if (!voice->active) {
            continue;
        }
        if ((voice->header.dwFlags & WHDR_DONE) != 0) {
            pc_audio_release_voice(voice);
        } else if (voice->fadeDuration != 0 &&
                   GetTickCount64() - voice->fadeStart >=
                       voice->fadeDuration) {
            pc_audio_release_voice(voice);
        } else if (voice->looping || voice->fadeDuration != 0) {
            pc_audio_set_voice_volume(voice);
        }
    }
    if (audio->music.active && !audio->music.looping &&
        (audio->music.header.dwFlags & WHDR_DONE) != 0) {
        pc_audio_release_voice(&audio->music);
        audio->currentMusicPath[0] = '\0';
        audio->musicPaused = 0;
    }
}

void jpb_PCAudioUpdate(JPBPCAudio *audio)
{
    if (audio == NULL) {
        return;
    }
    update_looped_sounds();
}

void jpb_PCAudioGetStats(
    const JPBPCAudio *audio,
    JPBPCAudioStats *stats)
{
    if (stats == NULL) {
        return;
    }
    if (audio == NULL) {
        memset(stats, 0, sizeof(*stats));
        return;
    }
    *stats = audio->stats;
}

static uint16_t pc_audio_play_hook(
    VECTOR *position,
    int bank_id,
    char *sound,
    uint32_t flag,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    JPBPCAudioVoice *voice = NULL;
    JPBPCAudioWavInfo info;
    WAVEFORMATEX format;
    char path[JPB_PC_AUDIO_PATH_CAPACITY];
    const char *name = pc_audio_sound_name(sound);
    size_t size = 0;
    size_t index;
    MMRESULT result;

    (void)flag;
    if (audio == NULL || !audio->outputEnabled || name == NULL ||
        !jpb_PCAudioResolveSound(
            audio, bank_id, sound, path, sizeof(path))) {
        return 0;
    }
    pc_audio_reap(audio);
    for (index = 0; index < JPB_PC_AUDIO_VOICE_COUNT; ++index) {
        if (!audio->voices[index].active) {
            voice = &audio->voices[index];
            break;
        }
    }
    if (voice == NULL) {
        return 0;
    }
    voice->fileBytes = pc_audio_read_file(path, &size);
    if (voice->fileBytes == NULL ||
        !jpb_PCAudioInspectWavMemory(
            voice->fileBytes, size, &info)) {
        pc_audio_release_voice(voice);
        return 0;
    }
    memset(&format, 0, sizeof(format));
    format.wFormatTag = info.formatTag;
    format.nChannels = info.channels;
    format.nSamplesPerSec = info.sampleRate;
    format.nAvgBytesPerSec = info.averageBytesPerSecond;
    format.nBlockAlign = info.blockAlign;
    format.wBitsPerSample = info.bitsPerSample;
    format.cbSize = 0;
    result = waveOutOpen(
        &voice->output,
        WAVE_MAPPER,
        &format,
        0,
        0,
        CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        pc_audio_release_voice(voice);
        return 0;
    }
    memset(&voice->header, 0, sizeof(voice->header));
    voice->header.lpData =
        (LPSTR)(voice->fileBytes + info.dataOffset);
    voice->header.dwBufferLength = info.dataSize;
    voice->looping = pc_audio_is_looping(name);
    if (voice->looping) {
        voice->header.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        voice->header.dwLoops = UINT32_MAX;
    }
    result = waveOutPrepareHeader(
        voice->output, &voice->header, sizeof(voice->header));
    if (result != MMSYSERR_NOERROR) {
        pc_audio_release_voice(voice);
        return 0;
    }
    voice->prepared = 1;
    voice->active = 1;
    voice->quiet = sound[0] == '-';
    voice->nonSpatial =
        position == NULL ||
        name[0] == 'z' ||
        name[0] == 'v' ||
        strchr(sound, '!') == sound ||
        (sound[0] == '-' && sound[1] == '!');
    voice->voiceSound = name[0] == 'v';
    voice->loopPosition = voice->looping ? position : NULL;
    if (!voice->looping) {
        voice->loopPosition = position;
    }
    pc_audio_set_voice_volume(voice);
    result = waveOutWrite(
        voice->output, &voice->header, sizeof(voice->header));
    if (result != MMSYSERR_NOERROR) {
        pc_audio_release_voice(voice);
        return 0;
    }
    if (!voice->looping) {
        voice->loopPosition = NULL;
    }
    ++audio->stats.sfxStarted;
    return (uint16_t)(index + 1);
}

static void pc_audio_stop_hook(
    uint16_t handle,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    size_t index;

    if (audio == NULL || handle == 0) {
        return;
    }
    index = (size_t)handle - 1;
    if (index < JPB_PC_AUDIO_VOICE_COUNT &&
        audio->voices[index].active) {
        pc_audio_release_voice(&audio->voices[index]);
    }
}

static void pc_audio_fade_hook(
    uint16_t handle,
    uint32_t fade_time,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    size_t index;

    if (audio == NULL || handle == 0) {
        return;
    }
    index = (size_t)handle - 1;
    if (index >= JPB_PC_AUDIO_VOICE_COUNT ||
        !audio->voices[index].active) {
        return;
    }
    if (fade_time == 0) {
        pc_audio_release_voice(&audio->voices[index]);
        return;
    }
    audio->voices[index].fadeStart = GetTickCount64();
    audio->voices[index].fadeDuration = fade_time;
}

static int pc_audio_bank_hook(
    int bank_id,
    const char *directory,
    const char *const *paths,
    int count,
    int load,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;

    (void)directory;
    if (audio == NULL || bank_id < 0 || bank_id >= 5) {
        return 0;
    }
    if (load) {
        if (paths == NULL || count < 0) {
            return 0;
        }
        audio->bankPaths[bank_id] = paths;
        audio->bankPathCount[bank_id] = count;
    } else {
        audio->bankPaths[bank_id] = NULL;
        audio->bankPathCount[bank_id] = 0;
    }
    return 1;
}

static void pc_audio_control_hook(
    JPBSoundControl control,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    size_t index;

    if (audio == NULL) {
        return;
    }
    switch (control) {
    case JPB_SOUND_CONTROL_PAUSE_MUSIC:
        if (audio->music.active && !audio->musicPaused &&
            waveOutPause(audio->music.output) == MMSYSERR_NOERROR) {
            audio->musicPaused = 1;
        }
        break;
    case JPB_SOUND_CONTROL_HALT_MUSIC:
        pc_audio_release_voice(&audio->music);
        audio->currentMusicPath[0] = '\0';
        audio->musicPaused = 0;
        break;
    case JPB_SOUND_CONTROL_MUTE_LOOPED:
        for (index = 0; index < JPB_PC_AUDIO_VOICE_COUNT; ++index) {
            JPBPCAudioVoice *voice = &audio->voices[index];

            if (voice->active && voice->looping) {
                (void)waveOutPause(voice->output);
            }
        }
        break;
    case JPB_SOUND_CONTROL_UNMUTE_LOOPED:
        for (index = 0; index < JPB_PC_AUDIO_VOICE_COUNT; ++index) {
            JPBPCAudioVoice *voice = &audio->voices[index];

            if (voice->active && voice->looping) {
                (void)waveOutRestart(voice->output);
            }
        }
        break;
    case JPB_SOUND_CONTROL_UPDATE_LOOPED:
        pc_audio_reap(audio);
        break;
    }
}

static void pc_audio_set_music_volume(JPBPCAudio *audio);

static int pc_audio_prepare_music(
    JPBPCAudio *audio,
    const char *path,
    int loop)
{
    JPBPCAudioVoice *voice = &audio->music;
    JPBPCAudioWavInfo info;
    WAVEFORMATEX format;
    size_t size = 0;
    MMRESULT result;

    voice->fileBytes = pc_audio_read_file(path, &size);
    if (voice->fileBytes == NULL ||
        !jpb_PCAudioInspectWavMemory(
            voice->fileBytes, size, &info)) {
        pc_audio_release_voice(voice);
        return 0;
    }
    memset(&format, 0, sizeof(format));
    format.wFormatTag = info.formatTag;
    format.nChannels = info.channels;
    format.nSamplesPerSec = info.sampleRate;
    format.nAvgBytesPerSec = info.averageBytesPerSecond;
    format.nBlockAlign = info.blockAlign;
    format.wBitsPerSample = info.bitsPerSample;
    result = waveOutOpen(
        &voice->output,
        WAVE_MAPPER,
        &format,
        0,
        0,
        CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        pc_audio_release_voice(voice);
        return 0;
    }
    memset(&voice->header, 0, sizeof(voice->header));
    voice->header.lpData =
        (LPSTR)(voice->fileBytes + info.dataOffset);
    voice->header.dwBufferLength = info.dataSize;
    voice->looping = loop != 0;
    if (voice->looping) {
        voice->header.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        voice->header.dwLoops = UINT32_MAX;
    }
    result = waveOutPrepareHeader(
        voice->output, &voice->header, sizeof(voice->header));
    if (result != MMSYSERR_NOERROR) {
        pc_audio_release_voice(voice);
        return 0;
    }
    voice->prepared = 1;
    voice->active = 1;
    pc_audio_set_music_volume(audio);
    result = waveOutWrite(
        voice->output, &voice->header, sizeof(voice->header));
    if (result != MMSYSERR_NOERROR) {
        pc_audio_release_voice(voice);
        return 0;
    }
    return 1;
}

static void pc_audio_set_music_volume(JPBPCAudio *audio)
{
    unsigned word;
    DWORD volume;

    if (audio == NULL || audio->music.output == NULL) {
        return;
    }
    if (audio->musicVolume < 0) {
        audio->musicVolume = 0;
    } else if (audio->musicVolume > 128) {
        audio->musicVolume = 128;
    }
    word = (unsigned)(
        (audio->musicVolume * 65535U) / 128U);
    volume = (DWORD)(word | (word << 16));
    (void)waveOutSetVolume(audio->music.output, volume);
}

static void pc_audio_stream_play_hook(
    int stream_index,
    const char *stream_name,
    int volume,
    int loop,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    char path[JPB_PC_AUDIO_PATH_CAPACITY];

    (void)stream_index;
    if (audio == NULL || !audio->outputEnabled ||
        !jpb_PCAudioResolveStream(
            audio, stream_name, path, sizeof(path))) {
        return;
    }
    audio->musicVolume = volume;
    if (audio->music.active && audio->musicPaused &&
        _stricmp(path, audio->currentMusicPath) == 0) {
        pc_audio_set_music_volume(audio);
        if (waveOutRestart(audio->music.output) == MMSYSERR_NOERROR) {
            audio->musicPaused = 0;
        }
        return;
    }
    pc_audio_release_voice(&audio->music);
    audio->currentMusicPath[0] = '\0';
    audio->musicPaused = 0;
    if (!pc_audio_prepare_music(audio, path, loop)) {
        return;
    }
    (void)pc_audio_copy_string(
        audio->currentMusicPath,
        sizeof(audio->currentMusicPath),
        path,
        strlen(path));
    ++audio->stats.musicStarted;
}

static int pc_audio_stream_control_hook(
    JPBAudioStreamControl control,
    int value,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    size_t index;

    if (audio == NULL) {
        return 0;
    }
    switch (control) {
    case JPB_AUDIO_STREAM_START_UP:
        return 1;
    case JPB_AUDIO_STREAM_SHUT_DOWN:
        for (index = 0; index < JPB_PC_AUDIO_VOICE_COUNT; ++index) {
            pc_audio_release_voice(&audio->voices[index]);
        }
        pc_audio_release_voice(&audio->music);
        audio->currentMusicPath[0] = '\0';
        audio->musicPaused = 0;
        return 1;
    case JPB_AUDIO_STREAM_PAUSE:
    case JPB_AUDIO_STREAM_STOP:
        if (audio->music.active && !audio->musicPaused &&
            waveOutPause(audio->music.output) == MMSYSERR_NOERROR) {
            audio->musicPaused = 1;
        }
        return 1;
    case JPB_AUDIO_STREAM_RESUME:
        if (audio->music.active && audio->musicPaused &&
            waveOutRestart(audio->music.output) == MMSYSERR_NOERROR) {
            audio->musicPaused = 0;
        }
        return 1;
    case JPB_AUDIO_STREAM_SET_VOLUME:
        audio->musicVolume = value;
        pc_audio_set_music_volume(audio);
        return 1;
    case JPB_AUDIO_STREAM_SET_CHANNEL_TYPE:
        return 1;
    default:
        return 0;
    }
}

JPBPCAudio *jpb_PCAudioCreate(
    const char *world_path,
    const char *player_one_cad_path,
    const char *player_two_cad_path,
    int level_index,
    int enable_output)
{
    JPBPCAudio *audio;
    char resource_root[JPB_PC_AUDIO_PATH_CAPACITY];
    int index;

    if (world_path == NULL ||
        !pc_audio_copy_string(
            resource_root,
            sizeof(resource_root),
            world_path,
            strlen(world_path))) {
        return NULL;
    }
    for (index = 0; index < 4; ++index) {
        if (!pc_audio_parent_directory(
                resource_root, sizeof(resource_root))) {
            return NULL;
        }
    }
    audio = (JPBPCAudio *)calloc(1, sizeof(*audio));
    if (audio == NULL ||
        !pc_audio_join(
            audio->soundRoot,
            sizeof(audio->soundRoot),
            resource_root,
            "sound\\sfx\\final") ||
        !pc_audio_join(
            audio->streamRoot,
            sizeof(audio->streamRoot),
            resource_root,
            "sound\\streams") ||
        !pc_audio_level_bank(
            level_index,
            audio->levelBank,
            sizeof(audio->levelBank))) {
        free(audio);
        return NULL;
    }
    if (player_one_cad_path != NULL &&
        !pc_audio_bank_from_cad(
            player_one_cad_path,
            audio->playerBank[0],
            sizeof(audio->playerBank[0]))) {
        free(audio);
        return NULL;
    }
    if (player_two_cad_path != NULL &&
        !pc_audio_bank_from_cad(
            player_two_cad_path,
            audio->playerBank[1],
            sizeof(audio->playerBank[1]))) {
        free(audio);
        return NULL;
    }
    audio->outputEnabled = enable_output != 0;
    audio->musicVolume = 128;
    jpb_SoundSetBankHook(pc_audio_bank_hook, audio);
    jpb_SoundSetControlHook(pc_audio_control_hook, audio);
    if (audio->outputEnabled) {
        jpb_SoundSetPlaySfxHook(pc_audio_play_hook, audio);
        jpb_SoundSetStopHook(pc_audio_stop_hook, audio);
        jpb_SoundSetFadeHook(pc_audio_fade_hook, audio);
        jpb_AudioStreamSetPlayHook(
            pc_audio_stream_play_hook, audio);
        jpb_AudioStreamSetControlHook(
            pc_audio_stream_control_hook, audio);
    }
    sound_Init();
    if (audio->playerBank[0][0] != '\0') {
        (void)sound_LoadBank(audio->playerBank[0], 1);
    }
    if (audio->playerBank[1][0] != '\0') {
        (void)sound_LoadBank(audio->playerBank[1], 2);
    }
    (void)sound_LoadBank(audio->levelBank, 3);
    return audio;
}

void jpb_PCAudioDestroy(JPBPCAudio *audio)
{
    size_t index;
    int bank;

    if (audio == NULL) {
        return;
    }
    for (bank = 0; bank < 5; ++bank) {
        sound_FreeBank(bank);
    }
    jpb_SoundSetBankHook(NULL, NULL);
    jpb_SoundSetControlHook(NULL, NULL);
    if (audio->outputEnabled) {
        jpb_SoundSetPlaySfxHook(NULL, NULL);
        jpb_SoundSetStopHook(NULL, NULL);
        jpb_SoundSetFadeHook(NULL, NULL);
        jpb_AudioStreamSetPlayHook(NULL, NULL);
        jpb_AudioStreamSetControlHook(NULL, NULL);
    }
    for (index = 0; index < JPB_PC_AUDIO_VOICE_COUNT; ++index) {
        pc_audio_release_voice(&audio->voices[index]);
    }
    pc_audio_release_voice(&audio->music);
    free(audio);
}
