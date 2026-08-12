/* Diagnostic host for exercising real WinMM output without a game window. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "jpb/audio_stream.h"
#include "jpb/game.h"
#include "jpb/level_world.h"
#include "jpb/pc_audio_win32.h"
#include "jpb/sound.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program)
{
    fprintf(
        stderr,
        "usage: %s <world.jpx> <player.cad> "
        "(--sfx bank name | --stream index) [--milliseconds N]\n",
        program);
}

int main(int argc, char **argv)
{
    JPBPCAudio *audio;
    uint16_t sound_handle = 0;
    int milliseconds = 1000;
    int mode;
    int value;
    int index;
    ULONGLONG start;

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
    if (index != argc || milliseconds < 1) {
        print_usage(argv[0]);
        return 2;
    }
    game_setDefaultOptions();
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
        sound_handle = sound_Play(NULL, value, argv[5], 0);
        if (sound_handle == 0) {
            fputs("could not open or play SFX WAV\n", stderr);
            jpb_PCAudioDestroy(audio);
            return 4;
        }
        printf("SFX started on handle %u\n", (unsigned)sound_handle);
    } else {
        JPBPCAudioStats stats;

        playXA(value, 128, 0);
        jpb_PCAudioGetStats(audio, &stats);
        if (stats.musicStarted == 0) {
            fputs("could not open or play stream WAV\n", stderr);
            jpb_PCAudioDestroy(audio);
            return 4;
        }
        printf("stream request %d submitted\n", value);
    }
    start = GetTickCount64();
    while (GetTickCount64() - start < (ULONGLONG)milliseconds) {
        jpb_PCAudioUpdate(audio);
        Sleep(10);
    }
    if (sound_handle != 0) {
        sound_StopSound(sound_handle);
    } else {
        stopXA();
    }
    jpb_PCAudioDestroy(audio);
    return 0;
}
