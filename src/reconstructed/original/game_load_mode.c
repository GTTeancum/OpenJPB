/* Exact PDB owner from game.c, module 0039, RVA 0xAAD50, 36 bytes. */

#include "jpb/audio_stream.h"
#include "jpb/game.h"

extern void CleanupLevelData(void);

void game_loadLevelMode(void)
{
    CleanupLevelData();
    stopXA();
    game_initVar(3);
    GameStruct.gameMode = 6;
}
