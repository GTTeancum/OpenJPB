/*
 * COMPLETE REVIEWED RECONSTRUCTION of the level initialization owners
 * emitted from W:\SWJediPowerBattles\Work\game.c.
 *
 * PDB module: 0039, RVAs 0xAA4F0 and 0xAA8D0.
 */

#include "jpb/alloc.h"
#include "jpb/anim.h"
#include "jpb/audio_stream.h"
#include "jpb/bullet.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/cube.h"
#include "jpb/enemy.h"
#include "jpb/game.h"
#include "jpb/generic_hook.h"
#include "jpb/globalarrays.h"
#include "jpb/input.h"
#include "jpb/linkstubs.h"
#include "jpb/loader.h"
#include "jpb/memory.h"
#include "jpb/menu.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/pwrup.h"
#include "jpb/scene.h"
#include "jpb/slots.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/texture.h"
#include "jpb/timer.h"
#include "jpb/vehicle.h"
#include "jpb/vram.h"
#include "jpb/whook.h"
#include "jpb/world.h"
#include "jpb/wrender.h"

#include <stdint.h>
#include <string.h>

extern void _StoreDescriptorHeapOffsetsEnd(void);
extern void _StoreDescriptorHeapOffsetsStart(void);
extern void loader_LoadJedi(void);
extern void menu_loadFrontEndArt(int group);
extern void turnOffBackground(void);
extern void VSync();

void game_initPerLevel(void)
{
    playertankindex = 0;
    fed_wallfrigflag = 0;
    tato_wallfrigflag = 0;
    cube_InitVisibility();
    ClearCachedTextureIndices();

    if (GameStruct.Continuing == 0) {
        GameStruct.CurrentLevel = (uint8_t)LevelSelect;
        if (LevelSelect == 6) {
            corusPoints[0] = 0;
            corusPoints[1] = 0;
        }
        if (GameStruct.mNumContinues < 0) {
            GameStruct.mNumContinues = 0;
        } else if (GameStruct.mNumContinues > 9) {
            GameStruct.mNumContinues = 9;
        }

        jpb_game_ApplyLevelDifficulty((unsigned)(uint8_t)LevelSelect);

        if (LevelSelect != 25) {
            GameStruct.versusModeFlag = 0;
        }
        texture_Flush(UINT32_C(0xf8c));
        menuTexLoaded2 = 0;
        jpb_GameResetOverlayScbs();
        loader_LevelLoad();
        (void)platform_enterLevel(LevelSelect);
        physics_InitPhysics();
        pwrup_Init();
        game_initPlayerStartCombos(0);
        if (GameStruct.NumPlayers == 2) {
            game_initPlayerStartCombos(1);
        }
        closeFileLog();
        if (GameStruct.gameMode == 8) {
            menu_enterPlayerCouncilMode();
        } else {
            menu_handleObjectiveMessage();
        }
    }

    if (GameStruct.CurrentLevel < 26 && OptionStruct.Music == 1) {
        stopXA();
        playXA(
            (int)aLevelXATracks[GameStruct.CurrentLevel],
            (int)OptionStruct.musicVolume * 2,
            1);
    }
    __InitDisplay(
        (int)OptionStruct.ScreenWidth,
        (int)OptionStruct.ScreenHeight,
        0x200);

    gCheckPoint = 0;
    afterLife = NULL;
    reStartScore[0] = 0;
    reStartScore[1] = 0;
    GameStruct.LevelExit = 0;
    GameStruct.Continuing = 0;
    GameStruct.StageExit = 0;
    GameStruct.Counter = 0;
    memset(GameStruct.message, 0, 4);
    GameStruct.aCharacterData[0].Force = 0;
    GameStruct.aCharacterData[1].Force = 0;
    GameStruct.aCharacterData[0].Score = 0;
    GameStruct.aCharacterData[1].Score = 0;
    initialLevelPauseDelay = 0;

    if (LevelSelect == 14) {
        uberPos.vx = 0x3b00;
        uberPos.vy = 0;
        uberPos.vz = -0x5000;
        uberXRange = 0x500;
        uberZRange = 0x5cc;
    } else {
        uberXRange = 0;
        uberZRange = 0;
    }
    uberLock = 0;

    if (tanknoise != 0) {
        sound_StopSound(tanknoise);
        tanknoise = 0;
    }
    if (turretnoise != 0) {
        sound_StopSound(turretnoise);
        turretnoise = 0;
    }
    if (stapsound != 0) {
        sound_StopSound(stapsound);
        stapsound = 0;
    }

    GameStruct.Continuing = 0;
    GameStruct.StageExit = 0;
    GameStruct.AbortCount = 0;
    timer_gSetRoundTimer(-1);
    player_gRefreshPlayers();
    slot_levelstart();
    GameStruct.GameState &= ~UINT32_C(0x01000000);
    nextLevel = 0;
    gHidePikobisModel = 1;
}

void game_initVar(unsigned mode)
{
    if (mode == 0) {
        GameStruct.AIDamage = 8;
        GameStruct.JediDamage = 8;
        GameStruct.HTHRate = 8;
        GameStruct.RangedRate = 8;
        GameStruct.BlockRate = 8;
        GameStruct.ComboLevel = 0;
        GameStruct.ForceLevel = 8;
        GameStruct.gameMode = 0;
        kmAudioStream_StartUp();
        menu_setNumPlayers(((padExist & UINT8_C(2)) != 0) + 1);
        initXAstuff();
        GameStruct.mNumContinues = 1000;
        OptionStruct.DebugLevel = 0;
        game_initEnergy();
        __InitDisplay(0x200, 0xe0, 0x3c0);
        VSync(1);
        return;
    }
    if (mode != 3) {
        return;
    }

    texture_Flush(UINT32_C(0xf80));
    memset(gpWorld, 0, sizeof(*gpWorld));
    scene_gInitRoot();
    scene_gInitScenes(0);
    model_InitModels();
    player_gInitPlayers(0);
    physics_gInitObjects(0);
    physics_InitPhysics();
    anim_InitAnimations(0);
    enemy_InitEnemies();
    coll_ResetCollisionSystem();
    bullet_InitProjectilePool();
    vram_gResetVram();
    initXAstuff();
    memory_FlushMemoryPool(1);
    memory_FlushMemoryPool(2);
    memory_FlushMemoryPool(3);
    meminit();
    physics_gInitObjects(0);
    game_setFuncArray();
    initArrays();
    clearzerobss();
    GameStruct.GameState &= UINT32_C(0xfffff67d);
    zerobss_levelReset = 1;
    zerobss_ResetBoss = 1;
    *(uint8_t *)&menuVars.fcount = 0;
    GameStruct.inMenuFlag = 1;
    GameStruct.Mode = 2;
    GameStruct.TestTimerCount = 5999;
    GameStruct.GameExit = 0;
    sprite_gInitSprites();
    if (GameStruct.Continuing == 0) {
        openFileLog();
    }
    resetTexTrack();
    menu_initLoadBar();
    menu_loadFrontEndArt(8);
    _StoreDescriptorHeapOffsetsStart();
    loader_LoadJedi();
    game_initPerLevel();
    _StoreDescriptorHeapOffsetsEnd();
    turnOffBackground();
}
