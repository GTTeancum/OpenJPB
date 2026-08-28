#include "jpb/game.h"
#include "jpb/main.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                             \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

enum { TRACE_CAPACITY = 24 };

static int trace_entries[TRACE_CAPACITY];
static int trace_count;
static unsigned init_bucket_flag;
static int text_gamma;
static int generated_language;
static unsigned menu_mode;
static int display_mask;
static const char *opened_log;
static const char *output_text;
static int pause_calls;

int gFileNotFound;
gamestruct GameStruct;

static void trace(int id)
{
    if (trace_count < TRACE_CAPACITY) {
        trace_entries[trace_count++] = id;
    }
}

void initBucket(unsigned flag) { init_bucket_flag = flag; trace(1); }
void __InitSystem(void) { trace(2); }
int memory_InitMemorySystem(void) { trace(3); return 7; }
void openFileLog(const char *name) { opened_log = name; trace(4); }
void file_gInitialise(void) { trace(5); }
void sound_Init(void) { trace(6); }
void timer_gInitSystemTimer(void) { trace(7); }
void file_LoadResidentSprites(void) { trace(8); }
void file_LoadEffects(void) { trace(9); }
void text_gInitialise(int gamma) { text_gamma = gamma; trace(10); }
void generateAllText(int language) { generated_language = language; trace(11); }
void anim_GlobalInit(void) { trace(12); }
void pauseUnpauseBucket(void) { trace(pause_calls++ == 0 ? 13 : 15); }
void menu_mainInitMenu(unsigned mode) { menu_mode = mode; trace(14); }
void closeFileLog(void) { trace(16); }
void SetDispMask(int enabled) { display_mask = enabled; trace(17); }
void WriteToOutputFile(const char *text) { output_text = text; trace(18); }
int game_gPlayTheGame(void) { trace(19); return 3; }
void SteamAPI_Shutdown(void) { trace(20); }

int main(void)
{
    static const int expected[20] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20
    };

    gFileNotFound = 5;
    GameStruct.GameExit = 9;
    initialize_main();

    CHECK(gFileNotFound == 0);
    CHECK(GameStruct.GameExit == 0);
    CHECK(init_bucket_flag == 0);
    CHECK(text_gamma == 0xa0);
    CHECK(generated_language == 0);
    CHECK(menu_mode == 0);
    CHECK(display_mask == 0);
    CHECK(strcmp(opened_log, "main") == 0);
    CHECK(strcmp(output_text, "Starting the game") == 0);
    CHECK(trace_count == 20);
    CHECK(memcmp(trace_entries, expected, sizeof(expected)) == 0);

    puts("main initialize tests passed");
    return 0;
}
