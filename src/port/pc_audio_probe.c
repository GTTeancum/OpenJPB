/* Diagnostic host for exercising real SDL_mixer output without a game window. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "jpb/audio_stream.h"
#include "jpb/camera.h"
#include "jpb/game.h"
#include "jpb/level_world.h"
#include "jpb/pc_audio_win32.h"
#include "jpb/resources.h"
#include "jpb/sound.h"

#include "pc_log_win32.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile LONG pcm_nonzero_buffers;
static void __cdecl verify_pcm(void *unused, unsigned char *bytes, int length)
{
    int offset;
    (void)unused;
    for (offset = 0; offset + (int)sizeof(float) <= length;
         offset += (int)sizeof(float)) {
        float sample;
        memcpy(&sample, bytes + offset, sizeof(sample));
        if (sample > 0.00001f || sample < -0.00001f) {
            InterlockedIncrement(&pcm_nonzero_buffers);
            break;
        }
    }
}

static void print_usage(const char *program)
{
    fprintf(
        stderr,
        "usage: %s <world.jpx> <player.cad> "
        "(--sfx bank name | --stream index) [--milliseconds N] "
        "[--movie-gate | --pickup-gain] [--verify-pcm] [--music-gain] [--initial-music-off]\n",
        program);
}

int main(int argc, char **argv)
{
    JPBPCAudio *audio;
    JPBPCAudioStats stats = {0};
    uint16_t sound_handle = 0;
    int milliseconds = 1000;
    int movie_gate = 0;
    int pickup_gain = 0;
    int check_pcm = 0;
    int music_gain = 0;
    int initial_music_off = 0;
    void (__cdecl *set_postmix)(void (__cdecl *)(void *, unsigned char *, int), void *) = NULL;
    int mode;
    int value;
    int index;
    ULONGLONG start;
    char base_path[1024];
    int parent;

    if (argc < 5) {
        print_usage(argv[0]);
        return 2;
    }
    if (strcmp(argv[3], "--sfx") == 0) {
        if (argc < 6) {
            print_usage(argv[0]);
            return 2;
        }
        mode = 0;
        value = atoi(argv[4]);
        index = 6;
    } else if (strcmp(argv[3], "--stream") == 0) {
        mode = 1;
        value = atoi(argv[4]);
        index = 5;
    } else {
        print_usage(argv[0]);
        return 2;
    }
    if (index + 1 < argc &&
        strcmp(argv[index], "--milliseconds") == 0) {
        milliseconds = atoi(argv[index + 1]);
        index += 2;
    }
    if (index < argc && strcmp(argv[index], "--movie-gate") == 0) {
        movie_gate = 1;
        ++index;
    }
    if (index < argc && strcmp(argv[index], "--pickup-gain") == 0) {
        pickup_gain = 1;
        ++index;
    }
    if (index < argc && strcmp(argv[index], "--verify-pcm") == 0) {
        check_pcm = 1;
        ++index;
    }
    if (index < argc && strcmp(argv[index], "--music-gain") == 0) {
        music_gain = 1;
        ++index;
    }
    if (index < argc && strcmp(argv[index], "--initial-music-off") == 0) {
        initial_music_off = 1;
        ++index;
    }
    if (index != argc || milliseconds < 1 || (check_pcm && mode != 0) ||
        (music_gain && mode != 1) ||
        ((movie_gate || pickup_gain) && mode != 0) ||
        (pickup_gain && (movie_gate || value != 0 ||
         (strcmp(argv[5], "xsecret") != 0 &&
          strcmp(argv[5], "xsaberup") != 0)))) {
        print_usage(argv[0]);
        return 2;
    }
    jpb_PCLogStart(argc, argv);
    if (strlen(argv[1]) >= sizeof(base_path)) {
        return 2;
    }
    strcpy(base_path, argv[1]);
    for (parent = 0; parent < 5; ++parent) {
        char *slash = strrchr(base_path, '\\');
        char *forward = strrchr(base_path, '/');

        if (forward != NULL && (slash == NULL || forward > slash)) {
            slash = forward;
        }
        if (slash == NULL) {
            return 2;
        }
        *slash = '\0';
    }
    if (!jpb_ResourceSetBasePath(base_path)) {
        return 2;
    }
    game_setDefaultOptions();
    if (initial_music_off) OptionStruct.Music = 0;
    audio = jpb_PCAudioCreate(
        argv[1],
        argv[2],
        NULL,
        jpb_LevelIndexFromPath(argv[1]),
        1);
    if (audio == NULL) {
        fputs("could not initialize PC audio paths\n", stderr);
        return 3;
    }
    if (mode == 0) {
        VECTOR pickup_position = {0, 0, 2560, 0};

        if (check_pcm) {
            HMODULE mixer = GetModuleHandleA("SDL2_mixer.dll");
            int (__cdecl *query_spec)(int *, uint16_t *, int *) =
                (int (__cdecl *)(int *, uint16_t *, int *))GetProcAddress(mixer, "Mix_QuerySpec");
            int frequency, channels;
            uint16_t format;
            set_postmix = (void (__cdecl *)(void (__cdecl *)(void *, unsigned char *, int), void *))
                GetProcAddress(mixer, "Mix_SetPostMix");
            if (query_spec == NULL || set_postmix == NULL ||
                !query_spec(&frequency, &format, &channels) || format != 0x8120) {
                fputs("PCM verification requires the native float mixer\n", stderr);
                jpb_PCAudioDestroy(audio);
                return 4;
            }
            set_postmix(verify_pcm, NULL);
        }

        if (pickup_gain) {
            memset(&cameraLocation, 0, sizeof(cameraLocation));
            cameraFacing = (_svector){0, 0, 4096, 0};
        }
        if (movie_gate) {
            jpb_PCAudioSetMoviePlayback(audio, 1);
            sound_handle = sound_playSfx(NULL, value, argv[5], 0);
            jpb_PCAudioGetStats(audio, &stats);
            if (sound_handle != UINT16_MAX || stats.movieSfxSuppressed != 1 ||
                stats.sfxStarted != 0) {
                fputs("SFX was not suppressed during the movie gate\n", stderr);
                jpb_PCAudioDestroy(audio);
                return 4;
            }
            jpb_PCAudioSetMoviePlayback(audio, 0);
        }
        sound_handle = pickup_gain
            ? sound_Play(&pickup_position, value, argv[5], 0)
            : sound_playSfx(NULL, value, argv[5], 0);
        if (sound_handle == UINT16_MAX) {
            fputs("could not open or play SFX WAV\n", stderr);
            jpb_PCAudioDestroy(audio);
            return 4;
        }
        if (pickup_gain) {
            HMODULE mixer = GetModuleHandleA("SDL2_mixer.dll");
            int (__cdecl *volume)(int, int) =
                (int (__cdecl *)(int, int))GetProcAddress(mixer, "Mix_Volume");
            void *(__cdecl *get_chunk)(int) =
                (void *(__cdecl *)(int))GetProcAddress(mixer, "Mix_GetChunk");
            int (__cdecl *chunk_volume)(void *, int) =
                (int (__cdecl *)(void *, int))GetProcAddress(mixer, "Mix_VolumeChunk");
            int actual_volume, actual_chunk_volume;
            void *chunk;

            if (volume == NULL || get_chunk == NULL || chunk_volume == NULL ||
                (chunk = get_chunk(sound_handle)) == NULL) {
                fputs("could not query live pickup mixer gain\n", stderr);
                jpb_PCAudioDestroy(audio);
                return 4;
            }
            actual_volume = volume(sound_handle, -1);
            actual_chunk_volume = chunk_volume(chunk, -1);
            if (actual_volume != 30 || actual_chunk_volume != 128) {
                fprintf(stderr, "unexpected pickup gain channel=%d chunk=%d\n",
                        actual_volume, actual_chunk_volume);
                jpb_PCAudioDestroy(audio);
                return 4;
            }
            printf("pickup_gain=(sound=%s,channel=%d,chunk=%d,"
                   "expected_pan=127/127,expected_distance=171)\n",
                   argv[5], actual_volume, actual_chunk_volume);
        }
        if (movie_gate) {
            jpb_PCAudioGetStats(audio, &stats);
            if (stats.movieGateBegins != 1 || stats.movieGateEnds != 1 ||
                stats.sfxStarted != 1 || stats.movieSfxSuppressed != 1) {
                fputs("SFX did not resume after the movie gate\n", stderr);
                jpb_PCAudioDestroy(audio);
                return 4;
            }
            puts("movie_sfx_gate=(suppressed=1,resumed=1,output=SDL_mixer)");
        }
        printf("SFX started on handle %u\n", (unsigned)sound_handle);
    } else {
        playXA(value, 128, 0);
        jpb_PCAudioGetStats(audio, &stats);
        if (stats.musicStarted == 0) {
            fputs("could not open or play stream WAV\n", stderr);
            jpb_PCAudioDestroy(audio);
            return 4;
        }
        pauseXA();
        unpauseXA();
        jpb_PCAudioGetStats(audio, &stats);
        if (stats.musicPauseRequests != 1 ||
            stats.musicResumeRequests != 1) {
            fputs("stream pause/resume lifecycle failed\n", stderr);
            jpb_PCAudioDestroy(audio);
            return 4;
        }
        printf("stream request %d submitted\n", value);
        if (music_gain) {
            HMODULE mixer = GetModuleHandleA("SDL2_mixer.dll");
            int (__cdecl *music_volume)(int) =
                (int (__cdecl *)(int))GetProcAddress(mixer, "Mix_VolumeMusic");
            const int gains[] = {0, 27, 30, 75};
            size_t gain_index;
            if (music_volume == NULL) return 4;
            if (music_volume(-1) != (OptionStruct.Music ? OptionStruct.musicVolume : 0)) {
                fprintf(stderr, "initial music gain does not match options\n");
                jpb_PCAudioDestroy(audio);
                return 4;
            }
            for (gain_index = 0; gain_index < sizeof(gains) / sizeof(gains[0]); ++gain_index) {
                int gain = gains[gain_index];
                setMusicVol(gain);
                playXA(value, 60, 0);
                if (music_volume(-1) != gain) {
                    fprintf(stderr, "music start replaced gain %d with %d\n", gain, music_volume(-1));
                    jpb_PCAudioDestroy(audio);
                    return 4;
                }
                pauseXA();
                playXA(value, 128, 0);
                if (music_volume(-1) != gain) return 4;
                jpb_PCAudioSetMoviePlayback(audio, 1);
                playXA(value, 60, 0);
                setMusicVol(gain);
                jpb_PCAudioSetMoviePlayback(audio, 0);
                if (music_volume(-1) != gain) return 4;
                printf("music_gain=(selected=%d,start=%d,resume=%d,deferred=%d)\n",
                       gain, gain, gain, gain);
            }
        }
    }
    start = GetTickCount64();
    while (GetTickCount64() - start < (ULONGLONG)milliseconds) {
        jpb_PCAudioUpdate(audio);
        Sleep(10);
    }
    if (check_pcm) {
        set_postmix(NULL, NULL);
        printf("mixed_pcm=(nonzero_buffers=%ld)\n", pcm_nonzero_buffers);
        if (pcm_nonzero_buffers == 0) {
            jpb_PCAudioDestroy(audio);
            return 4;
        }
    }
    if (mode == 0 && sound_handle != UINT16_MAX) {
        sound_StopSound(sound_handle);
    } else {
        stopXA();
        jpb_PCAudioGetStats(audio, &stats);
        if (stats.musicStopRequests != 1) {
            fputs("stream stop lifecycle failed\n", stderr);
            jpb_PCAudioDestroy(audio);
            return 4;
        }
    }
    jpb_PCAudioDestroy(audio);
    jpb_PCLogStop(0);
    return 0;
}
