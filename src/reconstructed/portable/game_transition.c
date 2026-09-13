/*
 * Shared host boundary for the exact next-level prelude in the gameMode 6
 * branch of game_gPlayTheGame. Keeping this in its own translation unit lets
 * portable frame owners consume the retail transition without pulling the
 * complete outer game loop into focused tests.
 */

#include "jpb/audio_stream.h"
#include "jpb/game.h"
#include "jpb/linkstubs.h"
#include "jpb/player.h"
#include "jpb/sound.h"
#include "jpb/world.h"

void jpb_GameRunActiveModePrelude(void)
{
    /* Retail only consumes/clears nextLevel outside menus. A pause between
     * the authored signal and the following active frame must preserve it. */
    if (GameStruct.inMenuFlag != 0) {
        return;
    }
    if (nextLevel != 0 && GameStruct.gameMode == 6) {
        (void)platform_completeLevel(LevelSelect);
        stop_all_looped_sounds();
        GameStruct.gameMode = 5;
        if (LevelSelect == 6) {
            corusPoints[0] = GameStruct.aCharacterData[0].Score;
            corusPoints[1] = GameStruct.aCharacterData[1].Score;
            LevelSelect = 15;
        } else {
            LevelSelect = LevelSelect == 15 ? 7 : (char)(LevelSelect + 1);
            if (OptionStruct.Music != 0) {
                playXA(4, (int)OptionStruct.musicVolume * 2, 0);
            }
        }
    }
    nextLevel = 0;
}
