#include "jpb/game.h"
#include "jpb/sprite.h"

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
    TRACE_DEFAULT_OPTIONS,
    TRACE_LOAD_OPTIONS,
    TRACE_MUSIC_VOLUME,
    TRACE_INIT_ZERO,
    TRACE_LOAD_GAME,
    TRACE_GAME_DATA,
    TRACE_ENTER_LOOP,
    TRACE_SUSPENDED_ONE,
    TRACE_SCALE,
    TRACE_SCALE_MM,
    TRACE_UPDATE_JOY,
    TRACE_POLL_QUIT,
    TRACE_POLL_EMPTY,
    TRACE_PAD_BITS,
    TRACE_PLATFORM_UPDATE,
    TRACE_SUSPENDED_TWO,
    TRACE_CLEANUP,
    TRACE_STOP_XA,
    TRACE_INIT_THREE
};

int8_t aLevelXATracks[26];
int32_t waitForInputReadAfterRefresh;

static enum TraceEvent trace_events[64];
static int trace_count;
static int poll_count;

static void trace(enum TraceEvent event)
{
    trace_events[trace_count++] = event;
}

void WriteToOutputFile(const char *text)
{
    if (strcmp(text, "Default Options Set") == 0) {
        trace(TRACE_DEFAULT_OPTIONS);
    } else if (strcmp(text, "Game Data Loaded") == 0) {
        trace(TRACE_GAME_DATA);
    } else if (strcmp(text, "Entering Game Loop") == 0) {
        trace(TRACE_ENTER_LOOP);
    }
}
void LoadOptionsData(void)
{
    trace(TRACE_LOAD_OPTIONS);
    OptionStruct.musicVolume = 42;
}
void setMusicVol(int volume)
{
    if (volume != 42) {
        trace_count = -100;
        return;
    }
    trace(TRACE_MUSIC_VOLUME);
}
void game_initVar(unsigned mode)
{
    if (mode == 0) {
        trace(TRACE_INIT_ZERO);
        GameStruct.gameMode = 7;
    } else if (mode == 3) {
        trace(TRACE_INIT_THREE);
    }
}
void LoadGameData(void) { trace(TRACE_LOAD_GAME); }
int platform_isSuspended(void)
{
    trace(trace_count < 9 ? TRACE_SUSPENDED_ONE : TRACE_SUSPENDED_TWO);
    return 0;
}
float getScaleAdjustment(void) { trace(TRACE_SCALE); return 1.0f; }
float getScaleAdjustmentMM(void) { trace(TRACE_SCALE_MM); return 2.0f; }
void UpdateJoyDevices(void) { trace(TRACE_UPDATE_JOY); }
int SDL_PollEvent(void *event)
{
    if (poll_count++ == 0) {
        *(uint32_t *)event = UINT32_C(0x100);
        trace(TRACE_POLL_QUIT);
        return 1;
    }
    trace(TRACE_POLL_EMPTY);
    return 0;
}
void psxUpdatePadbits(void) { trace(TRACE_PAD_BITS); }
void platform_update(void) { trace(TRACE_PLATFORM_UPDATE); }

void CleanupLevelData(void) { trace(TRACE_CLEANUP); }
void stopXA(void) { trace(TRACE_STOP_XA); }

void playXA(int track, int volume, int loop) {(void)track;(void)volume;(void)loop;}
void player_gRefreshPlayers(void) {}
void braindmg_ResetDamageTracker(int player) {(void)player;}
void camera_RestoreCameras(void) {}
void camera_SetCameras(void) {}
void cube_InitVisibility(void) {}
void enemy_ResetEnemies(void) {}
void game_OneGameLoop(void) {}
void ForceClearPlayerCPad(void) {}
void game_ProcessStatus(void) {}
void ClearCachedTextureIndices(void) {}
void restore_events(int32_t *map) {(void)map;}
int platform_completeLevel(char level) {(void)level; return 0;}
void clearzerobss(void) {}
void physics_InitPhysics(void) {}
void pwrup_Init(void) {}
void mute_looped_sounds(void) {}
void sound_StopAll(void) {}
void stop_all_looped_sounds(void) {}
void unmute_looped_sounds(void) {}
void update_looped_sounds(void) {}
void AddControllerDevice(int index) {(void)index;}
void AddJoyDevice(int index) {(void)index;}
void ClearWindow(void) {}
void PresentWindow(void) {}
void menu_enterScoreMode(int mode) {(void)mode;}

static int test_play_quit_path(void)
{
    static const enum TraceEvent expected[] = {
        TRACE_DEFAULT_OPTIONS, TRACE_LOAD_OPTIONS, TRACE_MUSIC_VOLUME,
        TRACE_INIT_ZERO, TRACE_LOAD_GAME, TRACE_GAME_DATA, TRACE_ENTER_LOOP,
        TRACE_SUSPENDED_ONE, TRACE_SCALE, TRACE_SCALE_MM, TRACE_UPDATE_JOY,
        TRACE_POLL_QUIT, TRACE_POLL_EMPTY, TRACE_PAD_BITS,
        TRACE_PLATFORM_UPDATE, TRACE_SUSPENDED_TWO
    };

    memset(&GameStruct, 0xa5, sizeof(GameStruct));
    memset(&OptionStruct, 0xa5, sizeof(OptionStruct));
    trace_count = 0;
    poll_count = 0;
    CHECK(game_gPlayTheGame() == 0);
    CHECK(trace_count == (int)(sizeof(expected) / sizeof(expected[0])));
    CHECK(memcmp(trace_events, expected, sizeof(expected)) == 0);
    CHECK(GameStruct.screenShotFlag == 0);
    CHECK(GameStruct.timerBars == 0);
    CHECK(GameStruct.difficulty == 1);
    CHECK((uint8_t)GameStruct.maxdraw == UINT8_C(0x20));
    CHECK(OptionStruct.Music == defaultOptionStruct.Music);
    CHECK(OptionStruct.musicVolume == 42);
    CHECK(scaleAdjustment == 1.0f);
    CHECK(scaleAdjustmentMM == 2.0f);
    return 0;
}

static int test_load_level_mode_order(void)
{
    static const enum TraceEvent expected[] = {
        TRACE_CLEANUP, TRACE_STOP_XA, TRACE_INIT_THREE
    };

    trace_count = 0;
    GameStruct.gameMode = 0;
    game_loadLevelMode();
    CHECK(trace_count == 3);
    CHECK(memcmp(trace_events, expected, sizeof(expected)) == 0);
    CHECK(GameStruct.gameMode == 6);
    return 0;
}

int main(void)
{
    CHECK(test_play_quit_path() == 0);
    CHECK(test_load_level_mode_order() == 0);
    return 0;
}
