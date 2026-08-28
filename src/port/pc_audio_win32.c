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

#include "pc_log_win32.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    JPB_PC_AUDIO_VOICE_COUNT = 64,
    JPB_PC_AUDIO_SAMPLE_CACHE_COUNT = 1024,
    JPB_PC_AUDIO_WARM_VOICES_PER_FORMAT = 4,
    JPB_PC_AUDIO_WARM_FORMAT_COUNT = 16,
    JPB_PC_AUDIO_PATH_CAPACITY = 1024
};

typedef struct JPBPCAudioSample {
    char path[JPB_PC_AUDIO_PATH_CAPACITY];
    unsigned char *bytes;
    size_t size;
    JPBPCAudioWavInfo info;
} JPBPCAudioSample;

typedef struct JPBPCAudioWarmFormat {
    uint16_t formatTag;
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t averageBytesPerSecond;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} JPBPCAudioWarmFormat;

typedef struct JPBPCAudioVoice {
    HWAVEOUT output;
    WAVEHDR header;
    JPBPCAudioWarmFormat outputFormat;
    unsigned char *fileBytes;
    VECTOR *loopPosition;
    int borrowedFileBytes;
    int prepared;
    int outputReady;
    int active;
    int looping;
    int quiet;
    int nonSpatial;
    int voiceSound;
    int mixerLeft;
    int mixerRight;
    int mixerDistance;
    int mixerVolume;
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
    JPBPCAudioSample samples[JPB_PC_AUDIO_SAMPLE_CACHE_COUNT];
    size_t sampleCount;
    JPBPCAudioWarmFormat warmFormats[JPB_PC_AUDIO_WARM_FORMAT_COUNT];
    size_t warmFormatCount;
};

static void pc_audio_seed_voice_outputs(
    JPBPCAudio *audio,
    const JPBPCAudioWavInfo *info,
    const WAVEFORMATEX *format,
    const char *path);

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

    if (level_index == 25) {
        if (bank == NULL || capacity == 0) {
            return 0;
        }
        bank[0] = '\0';
        return 1;
    }
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

static JPBPCAudioSample *pc_audio_cached_sample(
    JPBPCAudio *audio,
    const char *path)
{
    JPBPCAudioSample *sample;
    size_t index;
    size_t size = 0;

    if (audio == NULL || path == NULL || path[0] == '\0') {
        return NULL;
    }
    for (index = 0; index < audio->sampleCount; ++index) {
        sample = &audio->samples[index];
        if (_stricmp(sample->path, path) == 0) {
            return sample->bytes != NULL ? sample : NULL;
        }
    }
    if (audio->sampleCount >= JPB_PC_AUDIO_SAMPLE_CACHE_COUNT) {
        jpb_PCLog("audio sample cache full path=%s", path);
        return NULL;
    }
    sample = &audio->samples[audio->sampleCount];
    memset(sample, 0, sizeof(*sample));
    if (!pc_audio_copy_string(
            sample->path,
            sizeof(sample->path),
            path,
            strlen(path))) {
        return NULL;
    }
    sample->bytes = pc_audio_read_file(path, &size);
    sample->size = size;
    if (sample->bytes == NULL ||
        !jpb_PCAudioInspectWavMemory(
            sample->bytes, sample->size, &sample->info)) {
        jpb_PCLog(
            "audio sample preload failed path=%s bytes=%zu",
            path,
            size);
        free(sample->bytes);
        memset(sample, 0, sizeof(*sample));
        return NULL;
    }
    ++audio->sampleCount;
    return sample;
}

static void pc_audio_format_from_wav(
    const JPBPCAudioWavInfo *info,
    WAVEFORMATEX *format)
{
    memset(format, 0, sizeof(*format));
    format->wFormatTag = info->formatTag;
    format->nChannels = info->channels;
    format->nSamplesPerSec = info->sampleRate;
    format->nAvgBytesPerSec = info->averageBytesPerSecond;
    format->nBlockAlign = info->blockAlign;
    format->wBitsPerSample = info->bitsPerSample;
    format->cbSize = 0;
}

static int pc_audio_warm_format_matches(
    const JPBPCAudioWarmFormat *format,
    const JPBPCAudioWavInfo *info)
{
    return format != NULL && info != NULL &&
           format->formatTag == info->formatTag &&
           format->channels == info->channels &&
           format->sampleRate == info->sampleRate &&
           format->averageBytesPerSecond ==
               info->averageBytesPerSecond &&
           format->blockAlign == info->blockAlign &&
           format->bitsPerSample == info->bitsPerSample;
}

static void pc_audio_remember_warm_format(
    JPBPCAudio *audio,
    const JPBPCAudioWavInfo *info)
{
    JPBPCAudioWarmFormat *format;

    if (audio == NULL || info == NULL ||
        audio->warmFormatCount >= JPB_PC_AUDIO_WARM_FORMAT_COUNT) {
        return;
    }
    format = &audio->warmFormats[audio->warmFormatCount++];
    format->formatTag = info->formatTag;
    format->channels = info->channels;
    format->sampleRate = info->sampleRate;
    format->averageBytesPerSecond = info->averageBytesPerSecond;
    format->blockAlign = info->blockAlign;
    format->bitsPerSample = info->bitsPerSample;
}

static void pc_audio_store_format(
    JPBPCAudioWarmFormat *format,
    const JPBPCAudioWavInfo *info)
{
    if (format == NULL || info == NULL) {
        return;
    }
    format->formatTag = info->formatTag;
    format->channels = info->channels;
    format->sampleRate = info->sampleRate;
    format->averageBytesPerSecond = info->averageBytesPerSecond;
    format->blockAlign = info->blockAlign;
    format->bitsPerSample = info->bitsPerSample;
}

static int pc_audio_output_format_warmed(
    const JPBPCAudio *audio,
    const JPBPCAudioWavInfo *info)
{
    size_t index;

    if (audio == NULL || info == NULL) {
        return 0;
    }
    for (index = 0; index < audio->warmFormatCount; ++index) {
        if (pc_audio_warm_format_matches(
                &audio->warmFormats[index], info)) {
            return 1;
        }
    }
    return 0;
}

static void pc_audio_warm_output_sample(
    JPBPCAudio *audio,
    const JPBPCAudioSample *sample)
{
    WAVEFORMATEX format;
    WAVEHDR header;
    HWAVEOUT output = NULL;
    MMRESULT result;
    DWORD warm_bytes;
    int warmed;

    if (audio == NULL || sample == NULL ||
        !audio->outputEnabled ||
        sample->bytes == NULL ||
        sample->info.dataOffset >= sample->size ||
        sample->info.dataSize == 0) {
        return;
    }
    pc_audio_format_from_wav(&sample->info, &format);
    warmed = pc_audio_output_format_warmed(audio, &sample->info);
    if (warmed) {
        pc_audio_seed_voice_outputs(
            audio, &sample->info, &format, sample->path);
        return;
    }
    if (audio->warmFormatCount >= JPB_PC_AUDIO_WARM_FORMAT_COUNT) {
        jpb_PCLog("audio output warm format cache full");
        return;
    }
    result = waveOutOpen(
        &output,
        WAVE_MAPPER,
        &format,
        0,
        0,
        CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        jpb_PCLog(
            "audio output warm waveOutOpen failed path=%s result=%u "
            "format=%u channels=%u hz=%lu bits=%u",
            sample->path,
            (unsigned)result,
            (unsigned)format.wFormatTag,
            (unsigned)format.nChannels,
            (unsigned long)format.nSamplesPerSec,
            (unsigned)format.wBitsPerSample);
        return;
    }
    warm_bytes = sample->info.dataSize;
    if (warm_bytes > 4096U) {
        warm_bytes = 4096U;
    }
    if (sample->info.blockAlign > 1) {
        warm_bytes -=
            warm_bytes % (DWORD)sample->info.blockAlign;
    }
    if (warm_bytes == 0 ||
        (size_t)sample->info.dataOffset + (size_t)warm_bytes >
            sample->size) {
        waveOutClose(output);
        jpb_PCLog(
            "audio output warm skipped invalid data path=%s",
            sample->path);
        return;
    }
    memset(&header, 0, sizeof(header));
    header.lpData =
        (LPSTR)(sample->bytes + sample->info.dataOffset);
    header.dwBufferLength = warm_bytes;
    result = waveOutPrepareHeader(output, &header, sizeof(header));
    if (result == MMSYSERR_NOERROR) {
        result = waveOutWrite(output, &header, sizeof(header));
        (void)waveOutReset(output);
        (void)waveOutUnprepareHeader(output, &header, sizeof(header));
    }
    (void)waveOutClose(output);
    if (result != MMSYSERR_NOERROR) {
        jpb_PCLog(
            "audio output warm write failed path=%s result=%u",
            sample->path,
            (unsigned)result);
        return;
    }
    pc_audio_remember_warm_format(audio, &sample->info);
    pc_audio_seed_voice_outputs(
        audio, &sample->info, &format, sample->path);
}

static int pc_audio_bank_entry_path(
    const JPBPCAudio *audio,
    const char *authored_path,
    char *path,
    size_t path_capacity)
{
    char relative_path[JPB_PC_AUDIO_PATH_CAPACITY];
    size_t path_index;

    if (audio == NULL || authored_path == NULL ||
        path == NULL || path_capacity == 0 ||
        !pc_audio_copy_string(
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

static int pc_audio_preload_bank_samples(
    JPBPCAudio *audio,
    const char *const *paths,
    int count)
{
    int index;

    if (audio == NULL || paths == NULL || count < 0) {
        return 0;
    }
    for (index = 0; index < count; ++index) {
        char path[JPB_PC_AUDIO_PATH_CAPACITY];
        JPBPCAudioSample *sample;

        if (!pc_audio_bank_entry_path(
                audio,
                paths[index],
                path,
                sizeof(path)) ||
            (sample = pc_audio_cached_sample(audio, path)) == NULL) {
            jpb_PCLog(
                "audio bank preload failed index=%d path=%s",
                index,
                paths[index] != NULL ? paths[index] : "<null>");
            return 0;
        }
        pc_audio_warm_output_sample(audio, sample);
    }
    return 1;
}

static void pc_audio_clear_voice(
    JPBPCAudioVoice *voice,
    int keep_output)
{
    HWAVEOUT output;
    JPBPCAudioWarmFormat output_format;
    int output_ready;

    if (voice == NULL) {
        return;
    }
    output = voice->output;
    output_format = voice->outputFormat;
    output_ready = voice->outputReady;
    if (voice->output != NULL) {
        if (voice->active) {
            (void)waveOutReset(voice->output);
        }
        if (voice->prepared) {
            (void)waveOutUnprepareHeader(
                voice->output, &voice->header, sizeof(voice->header));
        }
        if (!keep_output) {
            (void)waveOutClose(voice->output);
            output = NULL;
            output_ready = 0;
            memset(&output_format, 0, sizeof(output_format));
        }
    }
    if (!voice->borrowedFileBytes) {
        free(voice->fileBytes);
    }
    memset(voice, 0, sizeof(*voice));
    if (keep_output && output != NULL && output_ready) {
        voice->output = output;
        voice->outputFormat = output_format;
        voice->outputReady = 1;
    }
}

static void pc_audio_recycle_voice(JPBPCAudioVoice *voice)
{
    pc_audio_clear_voice(voice, 1);
}

static void pc_audio_release_voice(JPBPCAudioVoice *voice)
{
    pc_audio_clear_voice(voice, 0);
}

static int pc_audio_voice_output_matches(
    const JPBPCAudioVoice *voice,
    const JPBPCAudioWavInfo *info)
{
    return voice != NULL && info != NULL &&
           voice->output != NULL &&
           voice->outputReady &&
           pc_audio_warm_format_matches(&voice->outputFormat, info);
}

static int pc_audio_prepare_voice_output(
    JPBPCAudioVoice *voice,
    const JPBPCAudioWavInfo *info,
    const WAVEFORMATEX *format,
    const char *path)
{
    MMRESULT result;

    if (voice == NULL || info == NULL || format == NULL) {
        return 0;
    }
    if (pc_audio_voice_output_matches(voice, info)) {
        return 1;
    }
    pc_audio_release_voice(voice);
    result = waveOutOpen(
        &voice->output,
        WAVE_MAPPER,
        format,
        0,
        0,
        CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        jpb_PCLog(
            "audio stream waveOutOpen failed path=%s result=%u "
            "format=%u channels=%u hz=%lu bits=%u",
            path,
            (unsigned)result,
            (unsigned)format->wFormatTag,
            (unsigned)format->nChannels,
            (unsigned long)format->nSamplesPerSec,
            (unsigned)format->wBitsPerSample);
        pc_audio_release_voice(voice);
        return 0;
    }
    pc_audio_store_format(&voice->outputFormat, info);
    voice->outputReady = 1;
    return 1;
}

static void pc_audio_seed_voice_outputs(
    JPBPCAudio *audio,
    const JPBPCAudioWavInfo *info,
    const WAVEFORMATEX *format,
    const char *path)
{
    size_t index;
    int ready_count = 0;

    if (audio == NULL || info == NULL || format == NULL) {
        return;
    }
    for (index = 0; index < JPB_PC_AUDIO_VOICE_COUNT; ++index) {
        JPBPCAudioVoice *voice = &audio->voices[index];

        if (!voice->active &&
            pc_audio_voice_output_matches(voice, info)) {
            ++ready_count;
        }
    }
    while (ready_count < JPB_PC_AUDIO_WARM_VOICES_PER_FORMAT) {
        JPBPCAudioVoice *voice = NULL;

        for (index = 0; index < JPB_PC_AUDIO_VOICE_COUNT; ++index) {
            if (!audio->voices[index].active &&
                audio->voices[index].output == NULL) {
                voice = &audio->voices[index];
                break;
            }
        }
        if (voice == NULL ||
            !pc_audio_prepare_voice_output(voice, info, format, path)) {
            return;
        }
        ++ready_count;
    }
}

static JPBPCAudioVoice *pc_audio_choose_voice(
    JPBPCAudio *audio,
    const JPBPCAudioWavInfo *info,
    size_t *voice_index)
{
    JPBPCAudioVoice *empty_voice = NULL;
    size_t empty_index = 0;
    JPBPCAudioVoice *available_voice = NULL;
    size_t available_index = 0;
    size_t index;

    if (audio == NULL || info == NULL) {
        return NULL;
    }
    for (index = 0; index < JPB_PC_AUDIO_VOICE_COUNT; ++index) {
        JPBPCAudioVoice *voice = &audio->voices[index];

        if (voice->active) {
            continue;
        }
        if (pc_audio_voice_output_matches(voice, info)) {
            if (voice_index != NULL) {
                *voice_index = index;
            }
            return voice;
        }
        if (voice->output == NULL && empty_voice == NULL) {
            empty_voice = voice;
            empty_index = index;
        }
        if (available_voice == NULL) {
            available_voice = voice;
            available_index = index;
        }
    }
    if (empty_voice != NULL) {
        if (voice_index != NULL) {
            *voice_index = empty_index;
        }
        return empty_voice;
    }
    if (available_voice != NULL && voice_index != NULL) {
        *voice_index = available_index;
    }
    return available_voice;
}

static void pc_audio_gains(
    const JPBPCAudioVoice *voice,
    float *left_gain,
    float *right_gain)
{
    float attenuation =
        (255.0f - (float)voice->mixerDistance) / 255.0f;
    float mixer_volume = (float)voice->mixerVolume;

    if (mixer_volume > 128.0f) {
        mixer_volume = 128.0f;
    }
    *left_gain =
        ((float)voice->mixerLeft / 255.0f) *
        attenuation *
        (mixer_volume / 128.0f);
    *right_gain =
        ((float)voice->mixerRight / 255.0f) *
        attenuation *
        (mixer_volume / 128.0f);
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
    pc_audio_gains(voice, &left, &right);
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
            pc_audio_recycle_voice(voice);
        } else if (voice->fadeDuration != 0 &&
                   GetTickCount64() - voice->fadeStart >=
                       voice->fadeDuration) {
            pc_audio_recycle_voice(voice);
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
    pc_audio_reap(audio);
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

static void *pc_audio_chunk_load_hook(
    const char *path, void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;

    if (audio == NULL || !audio->outputEnabled || path == NULL) {
        return NULL;
    }
    return pc_audio_cached_sample(audio, path);
}

static void pc_audio_chunk_free_hook(
    void *chunk, void *user_data)
{
    /* Samples are shared by authored path and released with JPBPCAudio. */
    (void)chunk;
    (void)user_data;
}

static uint16_t pc_audio_play_hook(
    void *chunk,
    int loops,
    VECTOR *position,
    int bank_id,
    char *sound,
    uint32_t flag,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    JPBPCAudioVoice *voice = NULL;
    JPBPCAudioSample *sample = (JPBPCAudioSample *)chunk;
    JPBPCAudioWavInfo info;
    WAVEFORMATEX format;
    const char *name = pc_audio_sound_name(sound);
    size_t index = 0;
    MMRESULT result;

    (void)flag;
    (void)bank_id;
    if (audio == NULL || !audio->outputEnabled || name == NULL ||
        sample == NULL) {
        return UINT16_MAX;
    }
    pc_audio_reap(audio);
    info = sample->info;
    pc_audio_format_from_wav(&info, &format);
    voice = pc_audio_choose_voice(audio, &info, &index);
    if (voice == NULL) {
        return UINT16_MAX;
    }
    if (!pc_audio_prepare_voice_output(
            voice, &info, &format, sample->path)) {
        return UINT16_MAX;
    }
    voice->fileBytes = sample->bytes;
    voice->borrowedFileBytes = 1;
    memset(&voice->header, 0, sizeof(voice->header));
    voice->header.lpData =
        (LPSTR)(voice->fileBytes + info.dataOffset);
    voice->header.dwBufferLength = info.dataSize;
    voice->looping = loops == -1;
    if (voice->looping) {
        voice->header.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        voice->header.dwLoops = UINT32_MAX;
    }
    result = waveOutPrepareHeader(
        voice->output, &voice->header, sizeof(voice->header));
    if (result != MMSYSERR_NOERROR) {
        jpb_PCLog(
            "audio stream prepare failed path=%s result=%u",
            sample->path,
            (unsigned)result);
        pc_audio_release_voice(voice);
        return UINT16_MAX;
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
    voice->mixerLeft = 255;
    voice->mixerRight = 255;
    voice->mixerDistance = 0;
    voice->mixerVolume = 128;
    voice->loopPosition = voice->looping ? position : NULL;
    if (!voice->looping) {
        voice->loopPosition = position;
    }
    pc_audio_set_voice_volume(voice);
    result = waveOutWrite(
        voice->output, &voice->header, sizeof(voice->header));
    if (result != MMSYSERR_NOERROR) {
        jpb_PCLog(
            "audio stream write failed path=%s result=%u",
            sample->path,
            (unsigned)result);
        pc_audio_release_voice(voice);
        return UINT16_MAX;
    }
    if (!voice->looping) {
        voice->loopPosition = NULL;
    }
    ++audio->stats.sfxStarted;
    return (uint16_t)index;
}

static void pc_audio_stop_hook(
    uint16_t handle,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    size_t index;

    if (audio == NULL) {
        return;
    }
    index = (size_t)handle;
    if (index < JPB_PC_AUDIO_VOICE_COUNT &&
        audio->voices[index].active) {
        pc_audio_recycle_voice(&audio->voices[index]);
    }
}

static void pc_audio_fade_hook(
    uint16_t handle,
    uint32_t fade_time,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    size_t index;

    if (audio == NULL) {
        return;
    }
    index = (size_t)handle;
    if (index >= JPB_PC_AUDIO_VOICE_COUNT ||
        !audio->voices[index].active) {
        return;
    }
    if (fade_time == 0) {
        pc_audio_recycle_voice(&audio->voices[index]);
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
        if (audio->outputEnabled &&
            !pc_audio_preload_bank_samples(audio, paths, count)) {
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

static int pc_audio_setup_hook(
    JPBSoundSetupOperation operation,
    int value0,
    int value1,
    int value2,
    int value3,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;

    (void)value1;
    (void)value2;
    (void)value3;
    if (audio == NULL) {
        return -1;
    }
    switch (operation) {
    case JPB_SOUND_SETUP_INIT:
        return 0;
    case JPB_SOUND_SETUP_OPEN_AUDIO:
        return 0;
    case JPB_SOUND_SETUP_ALLOCATE_CHANNELS:
        return value0 < JPB_PC_AUDIO_VOICE_COUNT
            ? value0
            : JPB_PC_AUDIO_VOICE_COUNT;
    default:
        return -1;
    }
}

static void pc_audio_channel_hook(
    JPBSoundChannelOperation operation,
    int channel,
    int value0,
    int value1,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    JPBPCAudioVoice *voice;

    if (audio == NULL || channel < 0 ||
        channel >= JPB_PC_AUDIO_VOICE_COUNT) {
        return;
    }
    voice = &audio->voices[channel];
    if (!voice->active) {
        return;
    }
    switch (operation) {
    case JPB_SOUND_CHANNEL_PANNING:
        voice->mixerLeft = value0;
        voice->mixerRight = value1;
        pc_audio_set_voice_volume(voice);
        break;
    case JPB_SOUND_CHANNEL_DISTANCE:
        voice->mixerDistance = value0;
        pc_audio_set_voice_volume(voice);
        break;
    case JPB_SOUND_CHANNEL_VOLUME:
        voice->mixerVolume = value0;
        pc_audio_set_voice_volume(voice);
        break;
    case JPB_SOUND_CHANNEL_PAUSE:
        (void)waveOutPause(voice->output);
        break;
    case JPB_SOUND_CHANNEL_RESUME:
        (void)waveOutRestart(voice->output);
        break;
    }
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
    if (audio == NULL) {
        jpb_PCLog(
            "audio stream ignored index=%d name=%s reason=no-audio",
            stream_index,
            stream_name != NULL ? stream_name : "<null>");
        return;
    }
    if (!audio->outputEnabled) {
        jpb_PCLog(
            "audio stream ignored index=%d name=%s reason=output-disabled",
            stream_index,
            stream_name != NULL ? stream_name : "<null>");
        return;
    }
    if (!jpb_PCAudioResolveStream(
            audio, stream_name, path, sizeof(path))) {
        jpb_PCLog(
            "audio stream resolve failed index=%d name=%s",
            stream_index,
            stream_name != NULL ? stream_name : "<null>");
        return;
    }
    jpb_PCLog(
        "audio stream request index=%d name=%s volume=%d loop=%d path=%s",
        stream_index,
        stream_name,
        volume,
        loop,
        path);
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
    jpb_PCLog(
        "audio stream started index=%d name=%s active=%d",
        stream_index,
        stream_name,
        audio->music.active);
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
    jpb_PCLog(
        "audio create roots sound=%s stream=%s level=%d bank=%s output=%d "
        "music=%u volume=%u",
        audio->soundRoot,
        audio->streamRoot,
        level_index,
        audio->levelBank,
        enable_output != 0,
        (unsigned)OptionStruct.Music,
        (unsigned)OptionStruct.musicVolume);
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
    jpb_SoundSetChunkHooks(
        pc_audio_chunk_load_hook,
        pc_audio_chunk_free_hook,
        audio);
    jpb_SoundSetSetupHook(pc_audio_setup_hook, audio);
    jpb_SoundSetChannelHook(pc_audio_channel_hook, audio);
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
    if (audio->outputEnabled && audio->bankPaths[0] == NULL) {
        jpb_PCAudioDestroy(audio);
        return NULL;
    }
    if (audio->playerBank[0][0] != '\0') {
        if (sound_LoadBank(audio->playerBank[0], 1) < 0) {
            jpb_PCAudioDestroy(audio);
            return NULL;
        }
    }
    if (audio->playerBank[1][0] != '\0') {
        if (sound_LoadBank(audio->playerBank[1], 2) < 0) {
            jpb_PCAudioDestroy(audio);
            return NULL;
        }
    }
    if (audio->levelBank[0] != '\0' &&
        sound_LoadBank(audio->levelBank, 3) < 0) {
        jpb_PCAudioDestroy(audio);
        return NULL;
    }
    return audio;
}

void jpb_PCAudioDestroy(JPBPCAudio *audio)
{
    size_t index;
    int bank;

    if (audio == NULL) {
        return;
    }
    for (bank = 0; bank < 4; ++bank) {
        sound_FreeBank(bank);
    }
    jpb_SoundSetBankHook(NULL, NULL);
    jpb_SoundSetChunkHooks(NULL, NULL, NULL);
    jpb_SoundSetSetupHook(NULL, NULL);
    jpb_SoundSetChannelHook(NULL, NULL);
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
    for (index = 0; index < audio->sampleCount; ++index) {
        free(audio->samples[index].bytes);
    }
    free(audio);
}
