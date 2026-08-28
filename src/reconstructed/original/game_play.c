/*
 * COMPLETE REVIEWED RECONSTRUCTION of game_gPlayTheGame from
 * W:\SWJediPowerBattles\Work\game.c.
 *
 * PDB module: 0039, RVA 0xA98F0, 1328 bytes.
 */

#include "jpb/audio_stream.h"
#include "jpb/braindmg.h"
#include "jpb/camera.h"
#include "jpb/cube.h"
#include "jpb/enemy.h"
#include "jpb/game.h"
#include "jpb/generic_hook.h"
#include "jpb/input.h"
#include "jpb/jonny.h"
#include "jpb/linkstubs.h"
#include "jpb/menu.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/pwrup.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/whook.h"
#include "jpb/world.h"

#include <stdint.h>

typedef union JPBSDLEvent {
    struct {
        uint32_t type;
        uint32_t timestamp;
        int32_t which;
    } device;
    uint8_t bytes[56];
} JPBSDLEvent;

enum {
    JPB_SDL_QUIT = 0x100,
    JPB_SDL_JOYDEVICEADDED = 0x605,
    JPB_SDL_CONTROLLERDEVICEADDED = 0x653
};

extern void AddControllerDevice(int device_index);
extern void AddJoyDevice(int device_index);
extern void CleanupLevelData(void);
extern void LoadGameData(void);
extern void LoadOptionsData(void);
extern int SDL_PollEvent(JPBSDLEvent *event);
extern void UpdateJoyDevices(void);
extern void WriteToOutputFile(const char *text);
extern void clearzerobss(void);
extern void platform_update(void);

static void game_reset_stage(void)
{
    stop_all_looped_sounds();
    waitForInputReadAfterRefresh = 2;
    stop_all_looped_sounds();
    restore_events(leveldata);
    physics_InitPhysics();
    player_gRefreshPlayers();
    enemy_ResetEnemies();
    camera_RestoreCameras();
    pwrup_Init();
    braindmg_ResetDamageTracker(0);
    braindmg_ResetDamageTracker(1);
    afterLife = NULL;
    clearzerobss();
    zerobss_levelReset = 1;
    zerobss_ResetBoss = 1;
    cube_InitVisibility();
    ClearCachedTextureIndices();
    sound_StopAll();
    stopXA();
    if (GameStruct.CurrentLevel < 26 && OptionStruct.Music == 1) {
        playXA(
            (int)aLevelXATracks[GameStruct.CurrentLevel],
            (int)OptionStruct.musicVolume * 2,
            1);
    }
    GameStruct.GameState &= ~UINT32_C(0xe0);
    gHidePikobisModel = 1;
    GameStruct.StageExit = 0;
    GameStruct.LevelExit = 0;
}

static void game_run_active_mode(void)
{
    if (GameStruct.inMenuFlag == 0 &&
        nextLevel != 0 && GameStruct.gameMode == 6) {
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

    ClearWindow();
    game_OneGameLoop();
    if ((GameStruct.GameState & UINT32_C(0x00100000)) != 0) {
        player_gRefreshPlayers();
        GameStruct.GameState &= ~UINT32_C(0x00100000);
    }
    if (GameStruct.CurrentLevel == 0) {
        GameStruct.GameState &= ~UINT32_C(0xe0);
        GameStruct.StageExit = 0;
        GameStruct.LevelExit = 0;
    } else if ((int8_t)GameStruct.GameState >= 0) {
        game_ProcessStatus();
    }

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        mute_looped_sounds();
    } else {
        if (GameStruct.LevelExit != 0) {
            GameStruct.GameState |= UINT32_C(2);
            stop_all_looped_sounds();
        }
        if (GameStruct.StageExit != 0) {
            game_reset_stage();
        }
        if (GameStruct.LevelExit != 0) {
            GameStruct.LevelExit = 0;
            GameStruct.gameMode = 9;
            stop_all_looped_sounds();
            if (GameStruct.mNumContinues < 5) {
                GameStruct.mNumContinues = 5;
            } else if (GameStruct.mNumContinues > 9) {
                GameStruct.mNumContinues = 9;
            }
            GameStruct.ContinuesUsed = 0;
            afterLife = NULL;
        }
        unmute_looped_sounds();
    }
    update_looped_sounds();
    GameStruct.Continuing = 0;
    PresentWindow();
}

int game_gPlayTheGame(void)
{
    JPBSDLEvent event;
    int running = 1;

    OptionStruct = defaultOptionStruct;
    WriteToOutputFile("Default Options Set");
    GameStruct.screenShotFlag = 0;
    GameStruct.timerBars = 0;
    GameStruct.difficulty = 1;
    GameStruct.maxdraw = 0x20;
    LoadOptionsData();
    setMusicVol(OptionStruct.musicVolume);
    game_initVar(0);
    LoadGameData();
    WriteToOutputFile("Game Data Loaded");
    WriteToOutputFile("Entering Game Loop");

    for (;;) {
        while (platform_isSuspended() != 0) {
        }
        scaleAdjustment = getScaleAdjustment();
        scaleAdjustmentMM = getScaleAdjustmentMM();
        UpdateJoyDevices();
        while (SDL_PollEvent(&event) != 0) {
            if (event.device.type == JPB_SDL_QUIT) {
                running = 0;
            } else if (event.device.type == JPB_SDL_JOYDEVICEADDED) {
                AddJoyDevice(event.device.which);
            } else if (event.device.type == JPB_SDL_CONTROLLERDEVICEADDED) {
                AddControllerDevice(event.device.which);
            }
        }
        psxUpdatePadbits();
        platform_update();
        while (platform_isSuspended() != 0) {
        }
        if (waitForInputReadAfterRefresh != 0) {
            ForceClearPlayerCPad();
            --waitForInputReadAfterRefresh;
        }

        switch (GameStruct.gameMode) {
        case 0:
        case 1:
            game_OneGameLoop();
            break;
        case 2:
        case 3:
            stop_all_looped_sounds();
            CleanupLevelData();
            stopXA();
            game_initVar(3);
            GameStruct.gameMode = 6;
            break;
        case 4:
        case 5:
            camera_SetCameras();
            menu_enterScoreMode(3);
            GameStruct.CurrentLevel = (uint8_t)LevelSelect;
            break;
        case 6:
            game_run_active_mode();
            break;
        case 8:
            LevelSelect = 0;
            GameStruct.CurrentLevel = 0;
            CleanupLevelData();
            stopXA();
            game_initVar(3);
            GameStruct.inMenuFlag = 1;
            GameStruct.gameMode = 6;
            break;
        case 9:
            stop_all_looped_sounds();
            menu_enterScoreMode(9);
            break;
        default:
            break;
        }
        if (!running) {
            return 0;
        }
    }
}
