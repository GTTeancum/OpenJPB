/* Dependency-isolated exact body from matched-PDB main module 0052. */

#include "jpb/alltext.h"
#include "jpb/anim.h"
#include "jpb/bucket.h"
#include "jpb/filesys.h"
#include "jpb/game.h"
#include "jpb/io.h"
#include "jpb/memory.h"
#include "jpb/menu.h"
#include "jpb/sound.h"
#include "jpb/text.h"
#include "jpb/timer.h"

void __InitSystem(void);
void closeFileLog(void);
void openFileLog();
void SetDispMask(int enabled);
void WriteToOutputFile(const char *text);
void SteamAPI_Shutdown(void);

/* 0xBE1A0, 155 bytes, global, 0 named locals
 * initialize_main
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\main.c
 */
void initialize_main(void)
{
    gFileNotFound = 0;
    initBucket(0);
    __InitSystem();
    (void)memory_InitMemorySystem();
    openFileLog("main");
    file_gInitialise();
    sound_Init();
    timer_gInitSystemTimer();
    file_LoadResidentSprites();
    file_LoadEffects();
    text_gInitialise(0xa0);
    generateAllText(0);
    anim_GlobalInit();
    pauseUnpauseBucket();
    menu_mainInitMenu(0);
    pauseUnpauseBucket();
    closeFileLog();
    GameStruct.GameExit = 0;
    SetDispMask(0);
    WriteToOutputFile("Starting the game");
    (void)game_gPlayTheGame();
    SteamAPI_Shutdown();
}
