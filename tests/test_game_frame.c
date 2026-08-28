#include "jpb/game.h"
#include "jpb/fmath.h"
#include "jpb/world.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n",                 \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

enum TraceEvent {
    TRACE_HANDLE,
    TRACE_PRE,
    TRACE_START,
    TRACE_DEBUG,
    TRACE_WAIT,
    TRACE_CAMERA,
    TRACE_MIDDLE,
    TRACE_MIX_QUERY,
    TRACE_PLAY_XA,
    TRACE_MENU,
    TRACE_DISP_MASK,
    TRACE_ENEMY,
    TRACE_END,
    TRACE_SOUND,
    TRACE_UPDATE_XA,
    TRACE_POST,
    TRACE_MEMCARD
};

int32_t numPlayers;
int tempPlayersVs;
int debug_slomo;
int debug_singlestep;
int8_t aLevelXATracks[26];

static enum TraceEvent trace_events[64];
static int trace_count;
static int mix_playing;
static int play_track;
static int play_volume;
static int play_loop;
static MATRIX render_matrix;

static void trace(enum TraceEvent event)
{
    trace_events[trace_count++] = event;
}

void __HandleWindow(void) { trace(TRACE_HANDLE); }
void scene_preRender(MATRIX **matrix)
{
    trace(TRACE_PRE);
    *matrix = &render_matrix;
}
void __StartRender(void) { trace(TRACE_START); }
void debugReset(void) { trace(TRACE_DEBUG); }
void WaitVBlank(void) { trace(TRACE_WAIT); }
void camera_SetCameras(void) { trace(TRACE_CAMERA); }
void scene_middleRender(MATRIX *matrix)
{
    if (matrix != &render_matrix) {
        trace_count = -100;
        return;
    }
    trace(TRACE_MIDDLE);
}
int Mix_PlayingMusic(void)
{
    trace(TRACE_MIX_QUERY);
    return mix_playing;
}
void playXA(int track, int volume, int loop)
{
    trace(TRACE_PLAY_XA);
    play_track = track;
    play_volume = volume;
    play_loop = loop;
}
void menu_mainLoop(void) { trace(TRACE_MENU); }
void SetDispMask(int enabled)
{
    if (enabled != 1) {
        trace_count = -100;
        return;
    }
    trace(TRACE_DISP_MASK);
}
void enemy_CheckTeleport(void) { trace(TRACE_ENEMY); }
void __EndRender(void) { trace(TRACE_END); }
void sound_UpdateAll(int delta)
{
    if (delta != 0x88) {
        trace_count = -100;
        return;
    }
    trace(TRACE_SOUND);
}
void updateXA(void) { trace(TRACE_UPDATE_XA); }
void scene_postRender(void) { trace(TRACE_POST); }
void kmMemcard_Update(void) { trace(TRACE_MEMCARD); }

static int check_trace(const enum TraceEvent *expected, int count)
{
    CHECK(trace_count == count);
    CHECK(memcmp(trace_events, expected, (size_t)count * sizeof(*expected)) == 0);
    return 0;
}

int main(void)
{
    static const enum TraceEvent first_frame[] = {
        TRACE_HANDLE, TRACE_PRE, TRACE_START, TRACE_DEBUG, TRACE_WAIT,
        TRACE_CAMERA, TRACE_MIDDLE, TRACE_MIX_QUERY, TRACE_PLAY_XA,
        TRACE_MENU, TRACE_ENEMY, TRACE_END, TRACE_SOUND, TRACE_UPDATE_XA,
        TRACE_POST, TRACE_MEMCARD
    };
    static const enum TraceEvent level_eight_frame[] = {
        TRACE_HANDLE, TRACE_PRE, TRACE_START, TRACE_DEBUG, TRACE_WAIT,
        TRACE_MIDDLE, TRACE_CAMERA, TRACE_MENU, TRACE_DISP_MASK,
        TRACE_ENEMY, TRACE_END, TRACE_SOUND, TRACE_UPDATE_XA,
        TRACE_POST, TRACE_MEMCARD
    };
    static const enum TraceEvent menu_frame[] = {
        TRACE_HANDLE, TRACE_START, TRACE_DEBUG, TRACE_WAIT, TRACE_CAMERA,
        TRACE_MENU, TRACE_ENEMY, TRACE_END, TRACE_SOUND, TRACE_UPDATE_XA,
        TRACE_MEMCARD
    };

    memset(&GameStruct, 0, sizeof(GameStruct));
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    memset(aLevelXATracks, 0, sizeof(aLevelXATracks));
    GameStruct.gameMode = 6;
    GameStruct.CurrentLevel = 3;
    GameStruct.NumPlayers = (char)-1;
    OptionStruct.Music = 1;
    OptionStruct.musicVolume = 27;
    aLevelXATracks[3] = 14;
    secretBits = UINT32_C(0x80000000);
    mix_playing = 0;
    trace_count = 0;
    game_OneGameLoop();
    CHECK(check_trace(first_frame, (int)(sizeof(first_frame) / sizeof(first_frame[0]))) == 0);
    CHECK(numPlayers == -1);
    CHECK(secretBits == UINT32_C(0x800006ff));
    CHECK(play_track == 14);
    CHECK(play_volume == 54);
    CHECK(play_loop == 1);

    GameStruct.CurrentLevel = 8;
    GameStruct.versusModeFlag = 1;
    tempPlayersVs = 2;
    OptionStruct.Music = 0;
    trace_count = 0;
    game_OneGameLoop();
    CHECK(check_trace(
              level_eight_frame,
              (int)(sizeof(level_eight_frame) / sizeof(level_eight_frame[0]))) == 0);
    CHECK(numPlayers == 2);

    GameStruct.gameMode = 0;
    GameStruct.inMenuFlag = 0;
    totalframes = 1;
    trace_count = 0;
    game_OneGameLoop();
    CHECK(check_trace(menu_frame, (int)(sizeof(menu_frame) / sizeof(menu_frame[0]))) == 0);
    return 0;
}
