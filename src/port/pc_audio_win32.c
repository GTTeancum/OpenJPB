/*
 * Runtime-loaded SDL_mixer host for the reconstructed sound scheduler.
 *
 * Provenance:
 *   direct/decompiled - sound_playSfx uses 64 SDL_mixer channels, six exact
 *     looping names, '-' quiet and '!' non-spatial prefixes, voice/non-
 *     spatial classification, bank lookup by basename, and live looped-
 *     position updates in the matched executable.
 *   direct/decompiled - sound_LoadBank maps level 15 to corus1, levels
 *     16..22 to the seven-entry training_level bank, and
 *     jar_jar_playable to gungan_2.
 *   direct/decompiled - the shipped executable imports SDL2_mixer directly:
 *     one 44.1 kHz float-stereo device, 64 channels, Mix_LoadWAV_RW for SFX,
 *     and Mix_LoadMUS for streaming music. Gameplay remains API-independent
 *     through the reconstruction hooks.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "jpb/pc_audio_win32.h"

#include "jpb/audio_stream.h"
#include "jpb/camera.h"
#include "jpb/game.h"
#include "jpb/level_world.h"
#include "jpb/sound.h"

#include "pc_log_win32.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    JPB_WAVE_FORMAT_PCM = 1,
    JPB_WAVE_FORMAT_IEEE_FLOAT = 3,
    JPB_PC_AUDIO_VOICE_COUNT = 64,
    JPB_PC_AUDIO_SAMPLE_CACHE_COUNT = 1024,
    JPB_PC_AUDIO_PATH_CAPACITY = 1024
};

typedef struct SDL_RWops SDL_RWops;
typedef struct Mix_Chunk Mix_Chunk;
typedef struct Mix_Music Mix_Music;

typedef struct JPBSdlMixerApi {
    HMODULE sdlModule;
    HMODULE mixerModule;
    SDL_RWops *(__cdecl *SDL_RWFromFile)(const char *, const char *);
    const char *(__cdecl *SDL_GetError)(void);
    int (__cdecl *Mix_Init)(int);
    void (__cdecl *Mix_Quit)(void);
    int (__cdecl *Mix_OpenAudio)(int, uint16_t, int, int);
    void (__cdecl *Mix_CloseAudio)(void);
    int (__cdecl *Mix_AllocateChannels)(int);
    Mix_Chunk *(__cdecl *Mix_LoadWAV_RW)(SDL_RWops *, int);
    void (__cdecl *Mix_FreeChunk)(Mix_Chunk *);
    int (__cdecl *Mix_PlayChannel)(int, Mix_Chunk *, int);
    int (__cdecl *Mix_SetPanning)(int, uint8_t, uint8_t);
    int (__cdecl *Mix_SetDistance)(int, uint8_t);
    int (__cdecl *Mix_Volume)(int, int);
    int (__cdecl *Mix_FadeOutChannel)(int, int);
    int (__cdecl *Mix_HaltChannel)(int);
    void (__cdecl *Mix_Pause)(int);
    void (__cdecl *Mix_Resume)(int);
    int (__cdecl *Mix_Playing)(int);
    Mix_Music *(__cdecl *Mix_LoadMUS)(const char *);
    void (__cdecl *Mix_FreeMusic)(Mix_Music *);
    int (__cdecl *Mix_PlayMusic)(Mix_Music *, int);
    int (__cdecl *Mix_HaltMusic)(void);
    void (__cdecl *Mix_PauseMusic)(void);
    void (__cdecl *Mix_ResumeMusic)(void);
    int (__cdecl *Mix_PlayingMusic)(void);
    int (__cdecl *Mix_PausedMusic)(void);
    int (__cdecl *Mix_VolumeMusic)(int);
} JPBSdlMixerApi;

typedef struct JPBPCAudioSample {
    char path[JPB_PC_AUDIO_PATH_CAPACITY];
    unsigned char *bytes;
    size_t size;
    JPBPCAudioWavInfo info;
    Mix_Chunk *chunk;
} JPBPCAudioSample;

struct JPBPCAudio {
    char soundRoot[JPB_PC_AUDIO_PATH_CAPACITY];
    char streamRoot[JPB_PC_AUDIO_PATH_CAPACITY];
    char playerBank[2][64];
    char levelBank[64];
    const char *const *bankPaths[5];
    int bankPathCount[5];
    JPBSdlMixerApi mixer;
    Mix_Music *music;
    char currentMusicPath[JPB_PC_AUDIO_PATH_CAPACITY];
    int musicPaused;
    int musicVolume;
    JPBPCAudioStats stats;
    int outputEnabled;
    int mixerOpened;
    JPBPCAudioSample samples[JPB_PC_AUDIO_SAMPLE_CACHE_COUNT];
    size_t sampleCount;
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
        (info->formatTag != JPB_WAVE_FORMAT_PCM &&
         info->formatTag != JPB_WAVE_FORMAT_IEEE_FLOAT) ||
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

static int pc_audio_load_proc(
    HMODULE module,
    const char *name,
    void *destination,
    size_t destination_size)
{
    FARPROC procedure;

    if (module == NULL || name == NULL || destination == NULL ||
        destination_size != sizeof(procedure)) {
        return 0;
    }
    procedure = GetProcAddress(module, name);
    if (procedure == NULL) {
        return 0;
    }
    memcpy(destination, &procedure, sizeof(procedure));
    return 1;
}

static void pc_audio_unload_mixer(JPBPCAudio *audio)
{
    if (audio == NULL) {
        return;
    }
    if (audio->mixer.mixerModule != NULL) {
        FreeLibrary(audio->mixer.mixerModule);
    }
    if (audio->mixer.sdlModule != NULL) {
        FreeLibrary(audio->mixer.sdlModule);
    }
    memset(&audio->mixer, 0, sizeof(audio->mixer));
}

static int pc_audio_load_mixer(
    JPBPCAudio *audio,
    const char *resource_root)
{
    char game_root[JPB_PC_AUDIO_PATH_CAPACITY];
    char sdl_path[JPB_PC_AUDIO_PATH_CAPACITY];
    char mixer_path[JPB_PC_AUDIO_PATH_CAPACITY];
    DWORD flags = LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                  LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;

#define JPB_LOAD_SDL_PROC(field) \
    pc_audio_load_proc( \
        audio->mixer.sdlModule, #field, \
        &audio->mixer.field, sizeof(audio->mixer.field))
#define JPB_LOAD_MIX_PROC(field) \
    pc_audio_load_proc( \
        audio->mixer.mixerModule, #field, \
        &audio->mixer.field, sizeof(audio->mixer.field))

    if (audio == NULL || resource_root == NULL ||
        !pc_audio_copy_string(
            game_root, sizeof(game_root), resource_root,
            strlen(resource_root)) ||
        !pc_audio_parent_directory(game_root, sizeof(game_root)) ||
        !pc_audio_join(
            sdl_path, sizeof(sdl_path), game_root, "SDL2.dll") ||
        !pc_audio_join(
            mixer_path, sizeof(mixer_path), game_root,
            "SDL2_mixer.dll")) {
        return 0;
    }
    audio->mixer.sdlModule = LoadLibraryExA(sdl_path, NULL, flags);
    audio->mixer.mixerModule = LoadLibraryExA(mixer_path, NULL, flags);
    if (audio->mixer.sdlModule == NULL ||
        audio->mixer.mixerModule == NULL ||
        !JPB_LOAD_SDL_PROC(SDL_RWFromFile) ||
        !JPB_LOAD_SDL_PROC(SDL_GetError) ||
        !JPB_LOAD_MIX_PROC(Mix_Init) ||
        !JPB_LOAD_MIX_PROC(Mix_Quit) ||
        !JPB_LOAD_MIX_PROC(Mix_OpenAudio) ||
        !JPB_LOAD_MIX_PROC(Mix_CloseAudio) ||
        !JPB_LOAD_MIX_PROC(Mix_AllocateChannels) ||
        !JPB_LOAD_MIX_PROC(Mix_LoadWAV_RW) ||
        !JPB_LOAD_MIX_PROC(Mix_FreeChunk) ||
        !JPB_LOAD_MIX_PROC(Mix_PlayChannel) ||
        !JPB_LOAD_MIX_PROC(Mix_SetPanning) ||
        !JPB_LOAD_MIX_PROC(Mix_SetDistance) ||
        !JPB_LOAD_MIX_PROC(Mix_Volume) ||
        !JPB_LOAD_MIX_PROC(Mix_FadeOutChannel) ||
        !JPB_LOAD_MIX_PROC(Mix_HaltChannel) ||
        !JPB_LOAD_MIX_PROC(Mix_Pause) ||
        !JPB_LOAD_MIX_PROC(Mix_Resume) ||
        !JPB_LOAD_MIX_PROC(Mix_Playing) ||
        !JPB_LOAD_MIX_PROC(Mix_LoadMUS) ||
        !JPB_LOAD_MIX_PROC(Mix_FreeMusic) ||
        !JPB_LOAD_MIX_PROC(Mix_PlayMusic) ||
        !JPB_LOAD_MIX_PROC(Mix_HaltMusic) ||
        !JPB_LOAD_MIX_PROC(Mix_PauseMusic) ||
        !JPB_LOAD_MIX_PROC(Mix_ResumeMusic) ||
        !JPB_LOAD_MIX_PROC(Mix_PlayingMusic) ||
        !JPB_LOAD_MIX_PROC(Mix_PausedMusic) ||
        !JPB_LOAD_MIX_PROC(Mix_VolumeMusic)) {
        jpb_PCLog(
            "audio SDL_mixer load failed root=%s win32=%lu",
            game_root,
            (unsigned long)GetLastError());
        pc_audio_unload_mixer(audio);
        return 0;
    }
    jpb_PCLog("audio SDL_mixer loaded path=%s", mixer_path);
    return 1;

#undef JPB_LOAD_SDL_PROC
#undef JPB_LOAD_MIX_PROC
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
            return sample->bytes != NULL &&
                   (!audio->outputEnabled || sample->chunk != NULL)
                ? sample : NULL;
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
    if (audio->outputEnabled) {
        SDL_RWops *stream =
            audio->mixer.SDL_RWFromFile(path, "rb");

        sample->chunk = stream != NULL
            ? audio->mixer.Mix_LoadWAV_RW(stream, 1)
            : NULL;
        if (sample->chunk == NULL) {
            jpb_PCLog(
                "audio Mix_LoadWAV_RW failed path=%s error=%s",
                path,
                audio->mixer.SDL_GetError());
            free(sample->bytes);
            memset(sample, 0, sizeof(*sample));
            return NULL;
        }
    }
    ++audio->sampleCount;
    return sample;
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
        (void)sample;
    }
    return 1;
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
    JPBPCAudioSample *sample = (JPBPCAudioSample *)chunk;
    const char *name = pc_audio_sound_name(sound);
    int channel;

    (void)position;
    (void)flag;
    (void)bank_id;
    if (audio == NULL || !audio->outputEnabled || name == NULL ||
        sample == NULL || sample->chunk == NULL) {
        return UINT16_MAX;
    }
    channel = audio->mixer.Mix_PlayChannel(
        -1, sample->chunk, loops);
    if (channel < 0 || channel >= JPB_PC_AUDIO_VOICE_COUNT) {
        jpb_PCLog(
            "audio Mix_PlayChannel failed path=%s error=%s",
            sample->path,
            audio->mixer.SDL_GetError());
        return UINT16_MAX;
    }
    ++audio->stats.sfxStarted;
    return (uint16_t)channel;
}

static void pc_audio_stop_hook(
    uint16_t handle,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;

    if (audio == NULL || handle >= JPB_PC_AUDIO_VOICE_COUNT) {
        return;
    }
    (void)audio->mixer.Mix_HaltChannel((int)handle);
}

static void pc_audio_fade_hook(
    uint16_t handle,
    uint32_t fade_time,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;
    int milliseconds;

    if (audio == NULL || handle >= JPB_PC_AUDIO_VOICE_COUNT) {
        return;
    }
    if (fade_time == 0) {
        (void)audio->mixer.Mix_HaltChannel((int)handle);
        return;
    }
    milliseconds = fade_time > (uint32_t)INT_MAX
        ? INT_MAX : (int)fade_time;
    (void)audio->mixer.Mix_FadeOutChannel(
        (int)handle, milliseconds);
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

    if (audio == NULL) {
        return -1;
    }
    if (!audio->outputEnabled) {
        return operation == JPB_SOUND_SETUP_ALLOCATE_CHANNELS
            ? value0 : 0;
    }
    switch (operation) {
    case JPB_SOUND_SETUP_INIT:
        return audio->mixer.Mix_Init(value0);
    case JPB_SOUND_SETUP_OPEN_AUDIO:
        if (audio->mixer.Mix_OpenAudio(
                value0, (uint16_t)value1, value2, value3) < 0) {
            jpb_PCLog(
                "audio Mix_OpenAudio failed hz=%d format=%04x "
                "channels=%d buffer=%d error=%s",
                value0,
                (unsigned)(uint16_t)value1,
                value2,
                value3,
                audio->mixer.SDL_GetError());
            return -1;
        }
        audio->mixerOpened = 1;
        return 0;
    case JPB_SOUND_SETUP_ALLOCATE_CHANNELS:
        return audio->mixer.Mix_AllocateChannels(value0);
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

    if (audio == NULL || channel < 0 ||
        channel >= JPB_PC_AUDIO_VOICE_COUNT ||
        !audio->outputEnabled) {
        return;
    }
    switch (operation) {
    case JPB_SOUND_CHANNEL_PANNING:
        (void)audio->mixer.Mix_SetPanning(
            channel, (uint8_t)value0, (uint8_t)value1);
        break;
    case JPB_SOUND_CHANNEL_DISTANCE:
        (void)audio->mixer.Mix_SetDistance(
            channel, (uint8_t)value0);
        break;
    case JPB_SOUND_CHANNEL_VOLUME:
        (void)audio->mixer.Mix_Volume(channel, value0);
        break;
    case JPB_SOUND_CHANNEL_PAUSE:
        audio->mixer.Mix_Pause(channel);
        break;
    case JPB_SOUND_CHANNEL_RESUME:
        audio->mixer.Mix_Resume(channel);
        break;
    }
}

static void pc_audio_control_hook(
    JPBSoundControl control,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;

    if (audio == NULL || !audio->outputEnabled) {
        return;
    }
    switch (control) {
    case JPB_SOUND_CONTROL_PAUSE_MUSIC:
        if (audio->mixer.Mix_PlayingMusic() != 0 &&
            audio->mixer.Mix_PausedMusic() == 0) {
            audio->mixer.Mix_PauseMusic();
            audio->musicPaused = 1;
        }
        break;
    case JPB_SOUND_CONTROL_HALT_MUSIC:
        (void)audio->mixer.Mix_HaltMusic();
        audio->currentMusicPath[0] = '\0';
        audio->musicPaused = 0;
        break;
    case JPB_SOUND_CONTROL_MUTE_LOOPED:
    case JPB_SOUND_CONTROL_UNMUTE_LOOPED:
    case JPB_SOUND_CONTROL_UPDATE_LOOPED:
        break;
    }
}

static void pc_audio_set_music_volume(JPBPCAudio *audio);

static int pc_audio_prepare_music(
    JPBPCAudio *audio,
    const char *path,
    int loop)
{
    audio->music = audio->mixer.Mix_LoadMUS(path);
    if (audio->music == NULL) {
        jpb_PCLog(
            "audio Mix_LoadMUS failed path=%s error=%s",
            path,
            audio->mixer.SDL_GetError());
        return 0;
    }
    pc_audio_set_music_volume(audio);
    if (audio->mixer.Mix_PlayMusic(
            audio->music, loop != 0 ? -1 : 0) < 0) {
        jpb_PCLog(
            "audio Mix_PlayMusic failed path=%s error=%s",
            path,
            audio->mixer.SDL_GetError());
        audio->mixer.Mix_FreeMusic(audio->music);
        audio->music = NULL;
        return 0;
    }
    return 1;
}

static void pc_audio_set_music_volume(JPBPCAudio *audio)
{
    if (audio == NULL || !audio->outputEnabled) {
        return;
    }
    if (audio->musicVolume < 0) {
        audio->musicVolume = 0;
    } else if (audio->musicVolume > 128) {
        audio->musicVolume = 128;
    }
    (void)audio->mixer.Mix_VolumeMusic(audio->musicVolume);
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
    if (audio->music != NULL &&
        audio->mixer.Mix_PlayingMusic() != 0 &&
        audio->mixer.Mix_PausedMusic() != 0 &&
        _stricmp(path, audio->currentMusicPath) == 0) {
        pc_audio_set_music_volume(audio);
        audio->mixer.Mix_ResumeMusic();
        audio->musicPaused = 0;
        return;
    }
    if (audio->music != NULL) {
        audio->mixer.Mix_FreeMusic(audio->music);
        audio->music = NULL;
    }
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
        audio->mixer.Mix_PlayingMusic() != 0);
}

static int pc_audio_stream_control_hook(
    JPBAudioStreamControl control,
    int value,
    void *user_data)
{
    JPBPCAudio *audio = (JPBPCAudio *)user_data;

    if (audio == NULL) {
        return 0;
    }
    if (!audio->outputEnabled) {
        return 1;
    }
    switch (control) {
    case JPB_AUDIO_STREAM_START_UP:
        return 1;
    case JPB_AUDIO_STREAM_SHUT_DOWN:
        (void)audio->mixer.Mix_HaltChannel(-1);
        (void)audio->mixer.Mix_HaltMusic();
        audio->currentMusicPath[0] = '\0';
        audio->musicPaused = 0;
        return 1;
    case JPB_AUDIO_STREAM_PAUSE:
        if (audio->mixer.Mix_PlayingMusic() != 0 &&
            audio->mixer.Mix_PausedMusic() == 0) {
            audio->mixer.Mix_PauseMusic();
            audio->musicPaused = 1;
        }
        return 1;
    case JPB_AUDIO_STREAM_STOP:
        if (audio->mixer.Mix_PlayingMusic() != 0 &&
            audio->mixer.Mix_PausedMusic() == 0) {
            (void)audio->mixer.Mix_HaltMusic();
        }
        audio->musicPaused = 0;
        return 1;
    case JPB_AUDIO_STREAM_RESUME:
        if (audio->mixer.Mix_PlayingMusic() != 0 &&
            audio->mixer.Mix_PausedMusic() != 0) {
            audio->mixer.Mix_ResumeMusic();
            audio->musicPaused = 0;
        }
        return 1;
    case JPB_AUDIO_STREAM_SET_VOLUME:
        audio->musicVolume = value;
        pc_audio_set_music_volume(audio);
        return 1;
    case JPB_AUDIO_STREAM_SET_CHANNEL_TYPE:
        audio->mixer.Mix_CloseAudio();
        audio->mixerOpened = 0;
        if (audio->mixer.Mix_OpenAudio(
                44100, UINT16_C(0x8120), value, 8192) < 0) {
            jpb_PCLog(
                "audio stream channel reset failed channels=%d error=%s",
                value,
                audio->mixer.SDL_GetError());
            return 0;
        }
        audio->mixerOpened = 1;
        (void)audio->mixer.Mix_AllocateChannels(32);
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
    if (audio->outputEnabled &&
        !pc_audio_load_mixer(audio, resource_root)) {
        free(audio);
        return NULL;
    }
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
        (void)audio->mixer.Mix_HaltChannel(-1);
        (void)audio->mixer.Mix_HaltMusic();
    }
    if (audio->music != NULL) {
        audio->mixer.Mix_FreeMusic(audio->music);
        audio->music = NULL;
    }
    for (index = 0; index < audio->sampleCount; ++index) {
        if (audio->samples[index].chunk != NULL) {
            audio->mixer.Mix_FreeChunk(
                audio->samples[index].chunk);
        }
        free(audio->samples[index].bytes);
    }
    if (audio->mixerOpened) {
        audio->mixer.Mix_CloseAudio();
    }
    if (audio->outputEnabled && audio->mixer.Mix_Quit != NULL) {
        audio->mixer.Mix_Quit();
    }
    pc_audio_unload_mixer(audio);
    free(audio);
}
