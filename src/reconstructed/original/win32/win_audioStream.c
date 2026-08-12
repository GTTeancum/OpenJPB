/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\win32\win_audioStream.c.
 *
 * playXA's exact stream-name table and all nine PDB interfaces are recovered
 * here. SDL_mixer ownership remains behind dependency-free callbacks so the
 * game-side callers do not acquire a platform audio dependency.
 *
 * Provenance:
 *   direct     - procedure, parameter/local, and table names from the PDB.
 *   decompiled - stream lookup at matched RVA 0x12BC00 and all 103 table
 *                entries at matched RVA 0x4D45D0.
 *   boundary   - mixer loading/playback/control is delegated to jpb_
 *                callbacks; the PC host binds them to WinMM.
 *
 * PDB module: 0101
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\win_audioStream.obj
 * Primary source: W:\SWJediPowerBattles\work\win32\win_audioStream.c
 * Compiler language: c
 * Emitted procedures: 9
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/audio_stream.h"

#include <stddef.h>

static JPBAudioStreamPlayHook jpb_audio_stream_play_hook;
static void *jpb_audio_stream_play_user_data;
static JPBAudioStreamControlHook jpb_audio_stream_control_hook;
static void *jpb_audio_stream_control_user_data;

/* Exact module-local PDB global, matched-PC RVA 0x4D45D0. */
static const char *ptrStreamWAVNames[103] = {
    NULL,
    "00_SplashScreen.wav",
    "00_PointScreen.wav",
    "00_LevelOverDie.wav",
    "00_LevelOverWin.wav",
    "01_StarFighterDroidBoss.wav",
    "01_FedAmbient1.wav",
    "01_FedFight1.wav",
    "01_FedFight2.wav",
    "02_MarshAmbient1.wav",
    "02_MarshAmbient2.wav",
    "02_MarshFight1.wav",
    "02_MarshFight2.wav",
    "02_WormBoss.wav",
    "03_TheedFight1.wav",
    "03_TheedFight2.wav",
    "03_TheedTankDrive.wav",
    "03_TurretDroidBoss.wav",
    "04_PalaceAmbient1.wav",
    "04_PalaceFight1.wav",
    "04_PalaceFight2.wav",
    "05_TatAmbient1.wav",
    "05_TatFight1.wav",
    "05_TatFight2.wav",
    "05_TatMaulFight.wav",
    "05_TatTuskenFight.wav",
    "06_CorAmbient1.wav",
    "06_CorAmbient2.wav",
    "06_CorFight1.wav",
    "06_CorFight2.wav",
    NULL,
    "07_RuinsAmbient1.wav",
    "07_RuinsFight1.wav",
    "08_StreetFight1.wav",
    "09_HangarAmbient1.wav",
    "09_HangarFight1.wav",
    "10_CoreFight1.wav",
    "10_CoreMaulFight1.wav",
    "10_CoreMaulFight2.wav",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "AnakinHyperdrive_OB.wav",
    "AnakinHyperdrive_QG.wav",
    "AnakinHyperdrive_WN.wav",
    "AnakinHyperdrive_AG.wav",
    "AnakinHyperdrive_PK.wav",
    "AnakinHyperdrive_WN.wav",
    "AnakinHyperdrive_QA.wav",
    "AnakinHyperdrive_WN.wav",
    "MaulRidesIn_OB.wav",
    "MaulRidesIn_QG.wav",
    "MaulRidesIn_WN.wav",
    "MaulRidesIn_AG.wav",
    "MaulRidesIn_PK.wav",
    "MaulRidesIn_WN.wav",
    "MaulRidesIn_QA.wav",
    "MaulRidesIn_WN.wav",
    "RescueQueen_OB.wav",
    "RescueQueen_QG.wav",
    "RescueQueen_WN.wav",
    "RescueQueen_AG.wav",
    "RescueQueen_PK.wav",
    "RescueQueen_PK.wav",
    "RescueQueen_QA.wav",
    "RescueQueen_WN.wav",
    "RescueQueen_OB2.wav",
    "RescueQueen_QG2.wav",
    "RescueQueen_WN2.wav",
    "RescueQueen_AG2.wav",
    "RescueQueen_PK2.wav",
    "RescueQueen_PK2.wav",
    "RescueQueen_QA2.wav",
    "RescueQueen_WN2.wav",
    "SecretPassage_OB.wav",
    "SecretPassage_QG.wav",
    "SecretPassage_WN.wav",
    "SecretPassage_AG.wav",
    "SecretPassage_PL.wav",
    "SecretPassage_PL.wav",
    "SecretPassage_QA.wav",
    "SecretPassage_WN.wav",
    "SecretPassage2_OB.wav",
    "SecretPassage2_QG.wav",
    "SecretPassage2_WN.wav",
    "SecretPassage2_AG.wav",
    "SecretPassage2_PL.wav",
    "SecretPassage2_PL.wav",
    "SecretPassage2_QA.wav",
    "SecretPassage2_WN.wav",
    "KaaduRace2.wav",
    "02_MarshStampede.wav",
    "QueenArrested.wav",
    "06_CorThugBoss.wav",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

void jpb_AudioStreamSetPlayHook(
    JPBAudioStreamPlayHook hook,
    void *user_data)
{
    jpb_audio_stream_play_hook = hook;
    jpb_audio_stream_play_user_data = user_data;
}

void jpb_AudioStreamSetControlHook(
    JPBAudioStreamControlHook hook,
    void *user_data)
{
    jpb_audio_stream_control_hook = hook;
    jpb_audio_stream_control_user_data = user_data;
}

static int audio_stream_control(
    JPBAudioStreamControl control,
    int value)
{
    if (jpb_audio_stream_control_hook == NULL) {
        return control == JPB_AUDIO_STREAM_START_UP;
    }
    return jpb_audio_stream_control_hook(
        control, value, jpb_audio_stream_control_user_data);
}

/* 0x12BB60, 23 bytes, global, 0 named locals
 * kmAudioStream_ShutDown
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\win_audioStream.c
 */
void kmAudioStream_ShutDown(void)
{
    (void)audio_stream_control(
        JPB_AUDIO_STREAM_SHUT_DOWN, 0);
}

/* 0x12BB80, 67 bytes, global, 0 named locals
 * kmAudioStream_StartUp
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\win32\win_audioStream.c
 */
int kmAudioStream_StartUp(void)
{
    return audio_stream_control(
        JPB_AUDIO_STREAM_START_UP, 0);
}

/* 0x12BBD0, 36 bytes, global, 0 named locals
 * pauseXA
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\win_audioStream.c
 */

/* 0x12BC00, 270 bytes, global, 5 named locals
 * playXA
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\work\win32\win_audioStream.c
 */
void pauseXA(void)
{
    (void)audio_stream_control(JPB_AUDIO_STREAM_PAUSE, 0);
}
void playXA(int strIndex, int volume, int bLoop)
{
    const char *soundPath;

    if (strIndex < 0 ||
        strIndex >=
            (int)(sizeof(ptrStreamWAVNames) /
                  sizeof(ptrStreamWAVNames[0]))) {
        return;
    }
    soundPath = ptrStreamWAVNames[strIndex];
    if (soundPath == NULL ||
        jpb_audio_stream_play_hook == NULL) {
        return;
    }
    jpb_audio_stream_play_hook(
        strIndex,
        soundPath,
        volume,
        bLoop,
        jpb_audio_stream_play_user_data);
}

/* 0x12BD10, 95 bytes, global, 2 named locals
 * setChannelType
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\win32\win_audioStream.c
 */
void setChannelType(int numChannels)
{
    (void)audio_stream_control(
        JPB_AUDIO_STREAM_SET_CHANNEL_TYPE, numChannels);
}

/* 0x12BD70, 5 bytes, global, 1 named locals
 * setMusicVol
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\win32\win_audioStream.c
 */
void setMusicVol(int volume)
{
    (void)audio_stream_control(
        JPB_AUDIO_STREAM_SET_VOLUME, volume);
}

/* 0x12BD80, 36 bytes, global, 0 named locals
 * stopXA
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\win_audioStream.c
 */
void stopXA(void)
{
    (void)audio_stream_control(JPB_AUDIO_STREAM_STOP, 0);
}

/* 0x12BDB0, 36 bytes, global, 0 named locals
 * unpauseXA
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\win_audioStream.c
 */
void unpauseXA(void)
{
    (void)audio_stream_control(JPB_AUDIO_STREAM_RESUME, 0);
}

/* 0x12BDE0, 3 bytes, global, 0 named locals
 * updateXA
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\win32\win_audioStream.c
 */
void updateXA(void)
{
}
