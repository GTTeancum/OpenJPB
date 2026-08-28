/*
 * COMPLETE REVIEWED RECONSTRUCTION of the frame owner emitted from
 * W:\SWJediPowerBattles\Work\game.c.
 *
 * PDB module: 0039, RVA 0xA8B40, 351 bytes.
 */

#include "jpb/audio_stream.h"
#include "jpb/camera.h"
#include "jpb/enemy.h"
#include "jpb/game.h"
#include "jpb/linkstubs.h"
#include "jpb/main.h"
#include "jpb/menu.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/win_memcard.h"
#include "jpb/world.h"

#include <stdint.h>

extern void __EndRender(void);
extern void __HandleWindow(void);
extern void __StartRender(void);
extern void WaitVBlank(void);
extern void debugReset(void);
extern int Mix_PlayingMusic(void);

static int32_t deadline;

void game_OneGameLoop(void)
{
    MATRIX *render_matrix;

    numPlayers = (int)(int8_t)GameStruct.NumPlayers;
    if (GameStruct.versusModeFlag == 1) {
        numPlayers = tempPlayersVs;
    }

    __HandleWindow();
    secretBits |= UINT32_C(0x6ff);
    if (GameStruct.gameMode == 6) {
        scene_preRender(&render_matrix);
    }
    __StartRender();
    debugReset();
    WaitVBlank();

    while (debug_slomo != 0 && debug_singlestep == 0) {
    }
    if (debug_singlestep != 0) {
        --debug_singlestep;
    }

    if (GameStruct.gameMode == 6) {
        if (GameStruct.CurrentLevel == 8) {
            scene_middleRender(render_matrix);
            camera_SetCameras();
        } else {
            camera_SetCameras();
            scene_middleRender(render_matrix);
        }
        if (GameStruct.xaPending == 0 &&
            GameStruct.xaFlag == 0 &&
            OptionStruct.Music != 0 &&
            Mix_PlayingMusic() == 0 &&
            GameStruct.CurrentLevel < 26) {
            playXA(
                (int)aLevelXATracks[GameStruct.CurrentLevel],
                (int)OptionStruct.musicVolume * 2,
                1);
        }
    } else if (totalframes != 0 && GameStruct.inMenuFlag == 0) {
        camera_SetCameras();
    }

    ++deadline;
    menu_mainLoop();
    if (deadline == 2) {
        SetDispMask(1);
    }
    enemy_CheckTeleport();
    __EndRender();
    sound_UpdateAll(0x88);
    updateXA();
    if (GameStruct.gameMode == 6) {
        scene_postRender();
    }
    kmMemcard_Update();
}
