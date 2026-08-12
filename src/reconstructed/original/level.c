/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\level.c.
 * PDB module: 0047
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\level.obj
 * Primary source: W:\SWJediPowerBattles\work\level.c
 * Compiler language: c
 * Emitted procedures: 33
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/level.h"

#include "jpb/audio_stream.h"
#include "jpb/camera.h"
#include "jpb/debugtext.h"
#include "jpb/fx.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/intersec.h"
#include "jpb/menu.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/sound.h"
#include "jpb/world.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Exact PDB globals at matched-PC RVAs 0x4BC8F0 and 0x4BC940. */
VECTOR maSpark1[JPB_SPARK_ROOM_ARC_COUNT] = {
    {0x8780, 0x1147, 0x149c, 0},
    {0x8580, 0x1147, 0x149c, 0},
    {0x8380, 0x1147, 0x149c, 0},
    {0x8180, 0x1147, 0x149c, 0},
    {0x7f80, 0x1147, 0x149c, 0},
};

VECTOR maSpark2[JPB_SPARK_ROOM_ARC_COUNT] = {
    {0x8780, 0x1147, 0x1300, 0},
    {0x8580, 0x1147, 0x1300, 0},
    {0x8380, 0x1147, 0x1300, 0},
    {0x8180, 0x1147, 0x1300, 0},
    {0x7f80, 0x1147, 0x1300, 0},
};

/* Exact PDB global at matched-PC RVA 0x5381C8. */
FVECTOR g_levelUVScroll;

/* Exact initialized PDB global at matched-PC RVA 0x4BC9C0. */
_svector FedBounds[JPB_LEVEL_BOUND_CORNER_COUNT] = {
    {13015, 3500, -9625, 0}, /* front-left */
    {12045, 3500, -9625, 0}, /* back-left */
    {13015, 3500, -9400, 0}, /* front-right */
    {12045, 3500, -9400, 0}  /* back-right */
};

/* Exact initialized PDB global at matched-PC RVA 0x4BCA00. */
_svector CorusBounds[JPB_LEVEL_BOUND_CORNER_COUNT] = {
    {12054, 8704, -17875, 0},
    {12054, 9500, -17875, 0},
    {11000, 8704, -17875, 0},
    {11000, 9500, -17875, 0}
};

/* Exact initialized PDB global at matched-PC RVA 0x4BC9E0. */
_svector palaceExitBounds[JPB_LEVEL_BOUND_CORNER_COUNT] = {
    {13600, 4392, -7601, 0},
    {13600, 5000, -7601, 0},
    {13000, 4392, -7601, 0},
    {13000, 5000, -7601, 0}
};

/*
 * Exact initialized level.c water tables at matched-PC RVAs
 * 0x4BCB30, 0x4BCBB0, and 0x4BCC10. The byte-oriented tuning table retains
 * the source ownership visible in level_Theed: speed, wave factor, speed,
 * wave factor. Only the first seven Theed patches are submitted.
 */
static uint32_t water_colors[16][2] = {
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x64505080), UINT32_C(0xc8408078)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x4b808040), UINT32_C(0xff183010)},
    {UINT32_C(0x7f7fa4b4), UINT32_C(0x7f7f7f7f)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x40407f7f), UINT32_C(0xc818a0c8)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)},
    {UINT32_C(0x7fffffff), UINT32_C(0x3fff0000)}
};

static uint8_t water_crud[24][4] = {
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0xc8, 0x50, 0xff},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x04, 0x40, 0x08, 0x50},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x40, 0x18, 0x24, 0x24},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x80, 0x80, 0x80, 0x80},
    {0x00, 0x00, 0x00, 0x00}
};

static _svector theed_water[8] = {
    {9, 10, 22, 0x0406},
    {10, 10, 26, 0x0409},
    {9, 10, 30, 0x0411},
    {13, 10, 22, 0x0404},
    {17, 10, 22, 0x0404},
    {21, 10, 22, 0x0404},
    {25, 10, 22, 0x0404},
    {0, 0, 0, 0}
};

/* Exact level.c module-local state and initialized data. */
static int count;
static int s1;
static int s2;
static int skip;
static int spark;
static int zeroBSSCheck;
static int zapman[JPB_SPARK_ROOM_ARC_COUNT];
static int glow = 0x40;
static int delta = 1;

/* level_Mini4's distinct PDB module-local at matched-PC RVA 0x5382F0. */
static int mini4ZeroBSSCheck;

/* level_CountDown's PDB module-locals at RVAs 0x538320..0x538330. */
static unsigned countdownStart;
static int countdownSucceed;
static int countdownFailed;
static int countdownDelay;
static int countdownZeroBSSCheck;

/* level_Hangar's PDB module-locals at RVAs 0x5382F4/0x5382F8. */
static unsigned hangarStart;
static int hangarZeroBSSCheck;

/* level_Theed's PDB module-locals at RVAs 0x5382FC/0x538300. */
static unsigned theedStart;
static int theedZeroBSSCheck;

/* level_Arena's PDB module-locals at RVAs 0x538314..0x53831C. */
static int arenaDelay;
static int arenaSucceed;
static int arenaZeroBSSCheck;

static int16_t jpb_level_wrap_short(int32_t value)
{
    uint16_t bits = (uint16_t)value;
    int16_t result;

    memcpy(&result, &bits, sizeof(result));
    return result;
}

static int32_t jpb_level_average_short(
    int16_t first, int16_t second)
{
    int32_t sum = (int32_t)first + (int32_t)second;

    if (sum >= 0) {
        return sum / 2;
    }
    return -(int32_t)(((uint32_t)(-sum) + 1U) / 2U);
}

/* 0xB63A0, 132 bytes, global, 8 named locals
 * BigPinkPulsatingShaft
 * PDB type: void (int, int, int, unsigned, i...
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB6430, 522 bytes, global, 5 named locals
 * bigcheck
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB6640, 479 bytes, local, 5 named locals
 * boxcheck
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB6820, 294 bytes, global, 3 named locals
 * calcboxcoord
 * PDB type: void (playerObject*, int*, int*)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB6950, 5257 bytes, global, 71 named locals
 * core_specials
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB7DE0, 271 bytes, global, 5 named locals
 * corecheck0
 * PDB type: void (playerObject*, _svector*)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB7EF0, 279 bytes, global, 4 named locals
 * corecheck1
 * PDB type: void (playerObject*, _svector*)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB8010, 498 bytes, global, 7 named locals
 * corecheck2
 * PDB type: int (playerObject*, _svector*, _...
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB8210, 202 bytes, global, 3 named locals
 * corecheck3
 * PDB type: void (playerObject*, _svector*)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB82E0, 375 bytes, global, 7 named locals
 * corefloorglow
 * PDB type: int (unsigned*, int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB8460, 585 bytes, global, 5 named locals
 * drawbigpoly
 * PDB type: void (unsigned, int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB86B0, 746 bytes, global, 18 named locals
 * drawsomecrappywater
 * PDB type: void (_svector*, int, float, flo...
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void drawsomecrappywater(
    _svector *water,
    int count,
    float factor1,
    float factor2,
    int speed1,
    int speed2,
    uint32_t color1,
    uint32_t color2)
{
    int current_x =
        (0x80ff - gpWorld->location.vx) >> 8;
    int current_z =
        (gpWorld->location.vz + 0x7f00) >> 8;
    int patch;

    for (patch = 0; patch < count; ++patch) {
        uint16_t packed_size =
            (uint16_t)water[patch].pad;
        int width = (int)(uint8_t)packed_size;
        int height = (int)(packed_size >> 8);
        int16_t right_cell = jpb_level_wrap_short(
            (int32_t)water[patch].vx + 1);
        int16_t left_cell = jpb_level_wrap_short(
            (int32_t)water[patch].vx - width - 1);
        int16_t back_cell = jpb_level_wrap_short(
            (int32_t)water[patch].vz + height + 1);

        if (current_x <= (int)right_cell + 12 &&
            (int)left_cell - 12 <= current_x &&
            (int)water[patch].vz - 1 - 12 <= current_z &&
            current_z <= (int)back_cell + 12) {
            int16_t right_x = jpb_level_wrap_short(
                (int32_t)right_cell * -0x100 - 0x8000);
            int16_t left_x = jpb_level_wrap_short(
                (int32_t)left_cell * -0x100 - 0x8000);
            int16_t y = jpb_level_wrap_short(
                (int32_t)water[patch].vy << 8);
            int16_t front_z = jpb_level_wrap_short(
                ((int32_t)water[patch].vz - 0x80) *
                    0x100);
            int16_t back_z = jpb_level_wrap_short(
                (int32_t)back_cell * 0x100 - 0x7f00);
            _svector corners[4] = {
                {left_x, y, back_z, 0},
                {right_x, y, back_z, 0},
                {left_x, y, front_z, 0},
                {right_x, y, front_z, 0}
            };
            uint32_t clip_mask =
                (uint32_t)cliptofrustrumSV(
                    clippingfrustrum, &corners[0], 0, NULL) &
                (uint32_t)cliptofrustrumSV(
                    clippingfrustrum, &corners[1], 0, NULL) &
                (uint32_t)cliptofrustrumSV(
                    clippingfrustrum, &corners[2], 0, NULL) &
                (uint32_t)cliptofrustrumSV(
                    clippingfrustrum, &corners[3], 0, NULL);

            if (clip_mask == 0) {
                VECTOR pos = {
                    (int32_t)water[patch].vx * -0x100 +
                        0x8000,
                    (int32_t)water[patch].vy << 8,
                    ((int32_t)water[patch].vz - 0x7f) *
                        0x100,
                    0
                };
                int signed_width =
                    (int)(int8_t)(uint8_t)packed_size;

                (void)debug_printf((char *)"WATER!\n");
                fx_Water(
                    &pos,
                    signed_width,
                    height,
                    color1,
                    factor1,
                    speed1);
                pos.vx += 0x80;
                pos.vz += 0x80;
                pos.vy = (int32_t)((float)pos.vy - 8.0f);
                fx_Water(
                    &pos,
                    signed_width,
                    height,
                    color2,
                    factor2,
                    speed2);
            }
        }
    }
}

/* 0xB89A0, 339 bytes, global, 1 named locals
 * glowdeath
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB8B00, 563 bytes, global, 5 named locals
 * level_Arena
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void level_Arena(void)
{
    if (arenaZeroBSSCheck == zerobss_levelReset) {
        if (arenaSucceed != 0) {
            --arenaDelay;
            if (arenaDelay > 1) {
                wchar_t *message;
                int score0 = game_gGetScore(0);
                int score1 = game_gGetScore(1);

                if (score1 < score0) {
                    message = allText[437];
                } else if (score0 < score1) {
                    message = allText[438];
                } else {
                    message = allText[439];
                }
                (void)_DrawText(
                    (float)((OptionStruct.ScreenWidth >> 1) - 0x80),
                    (float)((OptionStruct.ScreenHeight >> 1) - 0x80),
                    0.0001f,
                    1.0f,
                    ((uint32_t)arenaDelay << 24) |
                        UINT32_C(0x00ffffff),
                    (char *)(void *)message);
                return;
            }
            GameStruct.LevelExit = 1;
            afterLife = NULL;
            return;
        }
    } else {
        arenaZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        arenaDelay = 0;
        arenaSucceed = 0;
    }

    if (GameStruct.ContinuesUsed == GameStruct.mNumContinues) {
        arenaSucceed = 1;
        arenaDelay = 0xff;
    }

    if (gaPlayerData[0].playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(
            &gaPlayerData[0].playerRoot, 0, 0x20) != 0 ||
        (gaPlayerData[0].pFlags & UINT32_C(0x00040200)) != 0) {
        (void)game_gModEnergy(1, 0xdc);
        (void)game_gModForce(1, 0xdc);
    }
    if (gaPlayerData[1].playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(
            &gaPlayerData[1].playerRoot, 0, 0x20) != 0 ||
        (gaPlayerData[1].pFlags & UINT32_C(0x00040200)) != 0) {
        (void)game_gModEnergy(0, 0xdc);
        (void)game_gModForce(0, 0xdc);
    }
}

/* 0xB8D40, 248 bytes, global, 7 named locals
 * level_CoreWater
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB8E40, 341 bytes, global, 4 named locals
 * level_Corus
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
static void level_corus_clamp_player(physicsObject *physics)
{
    const _svector *lower = &CorusBounds[0];
    const _svector *upper = &CorusBounds[3];

    if (physics->pos.vz <= (float)lower->vz &&
        physics->pos.vz >= -19535.0f &&
        physics->pos.vx >= (float)upper->vx &&
        physics->pos.vx <= (float)lower->vx &&
        physics->pos.vy >= (float)lower->vy &&
        physics->pos.vy <= (float)upper->vy) {
        physics_gSetPosition(
            &physics->physicsRoot,
            (int)physics->pos.vx,
            (int)physics->pos.vy,
            lower->vz);
    }
}

void level_Corus(void)
{
    level_corus_clamp_player(&maPhysicsData[0]);
    level_corus_clamp_player(&maPhysicsData[1]);
}

/* 0xB8FA0, 1591 bytes, global, 28 named locals
 * level_CountDown
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void level_CountDown(int time, int kill, int score)
{
    float timerX;
    float timerY;
    float labelX;
    float labelY;
    float textX1;
    float textY1;
    float textX2;
    float textY2;
    float width;
    float height;
    int remaining;

    if (countdownZeroBSSCheck != zerobss_levelReset) {
        countdownZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        countdownStart = 0;
        countdownSucceed = 0;
        countdownFailed = 0;
        countdownDelay = 0;
        gDeathCount = 0;
        gPilotDeathCount = 0;
    }
    if (OptionStruct.DebugLevel != 0) {
        return;
    }
    if (countdownStart == 0) {
        countdownStart = gGlobalTimer;
    }
    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0 ||
        nextLevel == 1) {
        return;
    }

    if ((abGlobalBits[3] & UINT8_C(2)) != 0 &&
        countdownSucceed == 0 && countdownFailed != 1) {
        countdownSucceed = 1;
        countdownDelay = 0xff;
        if (OptionStruct.Music != 0) {
            playXA(4, (unsigned)OptionStruct.musicVolume * 2U, 0);
        }
        return;
    }

    if (time > 0) {
        timerX = -89.0f;
        timerY = 152.0f;
        labelX = 33.0f;
        labelY = 121.0f;
        width = 0.0f;
        height = 0.0f;
        time -= (int)((gGlobalTimer - countdownStart) / UINT32_C(0x3c00));
        remaining = time < 0 ? 0 : time;
        setPivotPositionAndFixScale(
            &timerX, &timerY, &width, &height, 7);
        setPivotPositionAndFixScale(
            &labelX, &labelY, &width, &height, 7);
        (void)_DrawText(
            timerX,
            timerY,
            0.0001f,
            1.0f,
            UINT32_C(0x7fffffff),
            "%03d",
            remaining);
        (void)_DrawText(
            labelX,
            labelY,
            0.0001f,
            0.5f,
            UINT32_C(0x7fffffff),
            "sec");

        if (remaining < 11) {
            if (gGlobalTimer % UINT32_C(0x3c00) == 0) {
                (void)sound_Play(NULL, 3, "xtimerbp", 0);
            }
            if (remaining < 1 && countdownFailed == 0 &&
                countdownSucceed != 1) {
                countdownFailed = 1;
                countdownDelay = 0xff;
                if (OptionStruct.Music != 0) {
                    playXA(
                        3,
                        (unsigned)OptionStruct.musicVolume * 2U,
                        0);
                }
                return;
            }
        } else if (remaining == (remaining / 10) * 10 &&
                   gGlobalTimer % UINT32_C(0x3c00) == 0) {
            (void)sound_Play(NULL, 3, "xtimerbp", 0);
        }
    }

    textX1 = 48.0f;
    textY1 = 200.0f;
    textX2 = 48.0f;
    textY2 = 260.0f;
    width = 0.0f;
    height = 0.0f;
    setPivotPositionAndFixScale(
        &textX1, &textY1, &width, &height, 0);
    setPivotPositionAndFixScale(
        &textX2, &textY2, &width, &height, 0);

    if (kill > 0) {
        if (kill - gDeathCount > 0) {
            (void)_DrawText(
                textX1,
                textY1,
                0.0001f,
                1.0f,
                UINT32_C(0x7fffffff),
                "%03d",
                kill - gDeathCount);
            (void)_DrawText(
                textX2,
                textY2,
                0.0001f,
                0.5f,
                UINT32_C(0x7fffffff),
                (char *)(void *)allText[427]);
        } else if (countdownSucceed == 0 &&
                   countdownFailed != 1) {
            countdownSucceed = 1;
            countdownDelay = 0xff;
            if (OptionStruct.Music != 0) {
                playXA(
                    4,
                    (unsigned)OptionStruct.musicVolume * 2U,
                    0);
            }
            return;
        }
    }

    if (score > 0 && game_gGetScore(0) >= score &&
        countdownSucceed == 0 && countdownFailed != 1) {
        countdownSucceed = 1;
        countdownDelay = 0xff;
        if (OptionStruct.Music != 0) {
            playXA(4, (unsigned)OptionStruct.musicVolume * 2U, 0);
        }
        return;
    }

    if (countdownSucceed == 1) {
        float textX = 0.0f;
        float textY = 200.0f;

        if (countdownFailed == 1) {
            return;
        }
        --countdownDelay;
        if (countdownDelay > 1) {
            setPivotPosition(&textX, &textY, 1);
            (void)SDLTextWriteScale(
                11,
                0x80,
                2,
                (int)textX,
                (int)textY,
                3.0f,
                2,
                allText[435]);
            if ((uint8_t)LevelSelect == 14) {
                secretBits |= UINT32_C(0x100);
            }
            return;
        }
        abGlobalBits[0] |= UINT8_C(2);
    } else if (countdownFailed == 1) {
        float textX = 0.0f;
        float textY = 200.0f;

        --countdownDelay;
        if (countdownDelay > 1) {
            setPivotPositionAndFixScale(
                &textX, &textY, &width, &height, 1);
            (void)SDLTextWriteScale(
                11,
                0x80,
                2,
                (int)textX,
                (int)textY,
                3.0f,
                2,
                allText[436]);
            return;
        }
        abGlobalBits[3] |= UINT8_C(1);
        (void)game_gSetGameFlags(UINT32_C(0x20));
        (void)game_gSetGameFlags(UINT32_C(0x40));
        gCheckPoint = 0;
        reStartScore[0] = 0;
    } else {
        return;
    }

    countdownDelay = 0;
    countdownFailed = 0;
    countdownSucceed = 0;
    countdownStart = 0;
}

/* 0xB95E0, 385 bytes, global, 6 named locals
 * level_Fed
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */
static void level_fed_clamp_player(physicsObject *physics)
{
    const _svector *front_left = &FedBounds[0];
    const _svector *back_right = &FedBounds[3];

    if (physics->pos.vx <= (float)front_left->vx &&
        physics->pos.vx >= (float)back_right->vx &&
        physics->pos.vz >= (float)front_left->vz &&
        physics->pos.vz <= (float)back_right->vz &&
        physics->pos.vy >= (float)front_left->vy) {
        physics_gSetPosition(
            &physics->physicsRoot,
            (int)physics->pos.vx,
            front_left->vy,
            (int)physics->pos.vz);
    }
}

void level_Fed(void)
{
    level_fed_clamp_player(&maPhysicsData[0]);
    level_fed_clamp_player(&maPhysicsData[1]);
    if ((GameStruct.GameState & UINT32_C(0x02000000)) == 0) {
        g_levelUVScroll.vy = 0.0f;
        g_levelUVScroll.vx -= 0.041f;
        while (g_levelUVScroll.vx < 0.0f) {
            g_levelUVScroll.vx += 3.0f;
        }
    }
}

/* 0xB9770, 738 bytes, global, 12 named locals
 * level_Hangar
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void level_Hangar(void)
{
    if (hangarZeroBSSCheck != zerobss_levelReset) {
        hangarZeroBSSCheck = zerobss_levelReset;
        hangarStart = 0;
        zerobss_levelReset = 0;
        pilotsKilled = 0;
    }

    if ((GameStruct.GameState & UINT32_C(0x02000000)) == 0) {
        g_levelUVScroll.vx += -0.006f;
        g_levelUVScroll.vy += -0.01f;
        while (g_levelUVScroll.vy < 0.0f) {
            g_levelUVScroll.vy += 1.0f;
        }
        while (g_levelUVScroll.vx < 0.0f) {
            g_levelUVScroll.vx += 1.0f;
        }
    }
    if (hangarStart == 0) {
        hangarStart = gGlobalTimer;
    }
    (void)game_gSetGameFlags(UINT32_C(0x01000000));

    if ((GameStruct.GameState & UINT32_C(0x02000000)) == 0) {
        float timerX = -89.0f;
        float timerY = 267.0f;
        float labelX = 33.0f;
        float labelY = 236.0f;
        float width = 0.0f;
        float height = 0.0f;
        int current = 400 -
            (int)((gGlobalTimer - hangarStart) / UINT32_C(0x3c00));

        setPivotPositionAndFixScale(
            &timerX, &timerY, &width, &height, 7);
        setPivotPositionAndFixScale(
            &labelX, &labelY, &width, &height, 7);
        (void)_DrawText(
            timerX,
            timerY,
            0.0001f,
            1.0f,
            UINT32_C(0x7fffffff),
            "%03d",
            current);
        (void)_DrawText(
            labelX,
            labelY,
            0.0001f,
            0.5f,
            UINT32_C(0x7fffffff),
            "sec");

        if ((current < 11 || current == (current / 10) * 10) &&
            gGlobalTimer % UINT32_C(0x3c00) == 0) {
            (void)sound_Play(NULL, 3, "xtimerbp", 0);
        }
        if (pilotsKilled < 2 && GameStruct.Counter > 5) {
            abGlobalBits[6] |= UINT8_C(4);
        }
        if (current < 1 || pilotsKilled > 1) {
            if (GameStruct.Counter < 5) {
                abGlobalBits[3] |= UINT8_C(1);
                (void)game_gSetGameFlags(UINT32_C(0x20));
                (void)game_gSetGameFlags(UINT32_C(0x40));
                gCheckPoint = 0;
                reStartScore[0] = 0;
                return;
            }
            abGlobalBits[0] |= UINT8_C(2);
        }
    }
}

/* 0xB9A60, 59 bytes, global, 2 named locals
 * level_InitSpecials
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB9AA0, 1289 bytes, global, 20 named locals
 * level_Mini1
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xB9FB0, 603 bytes, global, 14 named locals
 * level_Mini2
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xBA210, 1428 bytes, global, 25 named locals
 * level_Mini3
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xBA7B0, 155 bytes, global, 3 named locals
 * level_Mini4
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void level_Mini4(void)
{
    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }

    if (mini4ZeroBSSCheck != zerobss_levelReset) {
        mini4ZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        gDeathCount = 0;
        gPilotDeathCount = 0;
    }

    if (100 - gDeathCount > 0) {
        menu_drawBigNums(
            (unsigned)(100 - gDeathCount),
            2,
            0xf0,
            0xb8,
            0x80,
            0x80,
            0x80);
    } else if ((secretBits & UINT32_C(0x100)) == 0) {
        secretBits |= UINT32_C(0x100);
    }
}

/* 0xBA850, 528 bytes, global, 8 named locals
 * level_Palace
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
static void level_palace_clamp_player(physicsObject *physics)
{
    const _svector *lower = &palaceExitBounds[0];
    const _svector *upper = &palaceExitBounds[3];

    if (physics->pos.vz >= (float)lower->vz &&
        physics->pos.vx >= (float)upper->vx &&
        physics->pos.vx <= (float)lower->vx &&
        physics->pos.vy >= (float)lower->vy &&
        physics->pos.vy <= (float)upper->vy) {
        physics_gSetPosition(
            &physics->physicsRoot,
            (int)physics->pos.vx,
            (int)physics->pos.vy,
            lower->vz);
    }
}

void level_Palace(void)
{
    level_palace_clamp_player(&maPhysicsData[0]);
    level_palace_clamp_player(&maPhysicsData[1]);
    level_Palace_KillOffscreenBoss(163);
    level_Palace_KillOffscreenBoss(164);
}

/* 0xBAA60, 137 bytes, global, 3 named locals
 * level_Palace_KillOffscreenBoss
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xBAAF0, 327 bytes, global, 10 named locals
 * level_Ruins
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xBAC40, 1106 bytes, global, 12 named locals
 * level_SparkRoom
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void level_Palace_KillOffscreenBoss(int enemy_index)
{
    wsl_BAP_PLACEMENT *placement =
        gpWorld->apEnemy[enemy_index];

    if (placement->status == 1) {
        wsl_ENEMY *enemy = (wsl_ENEMY *)getPtr(
            (int)placement->pLastEnemy,
            JPB_POINTER_ARRAY_ENEMY);

        if (enemy->location.vx >= -8500 &&
            physics_gGetRange(
                &enemy->pPlayer->playerRoot,
                &gpWorld->player0->playerRoot) >= 750) {
            (void)game_gSetEnergy(
                enemy->pPlayer->playernum, 0);
        }
    }
}
void level_SparkRoom(void)
{
    _svector start;
    _svector end;
    VECTOR pos;
    int zapped;
    int index;

    if (zeroBSSCheck != zerobss_levelReset) {
        zeroBSSCheck = zerobss_levelReset;
        memset(zapman, 0, sizeof(zapman));
        zerobss_levelReset = 0;
    }

    if (count < 1) {
        s1 = rand() % JPB_SPARK_ROOM_ARC_COUNT;
        s2 = s1;
        count = rand() % 16;
    }

    if (spark++ < 0x20) {
        uint32_t col1;
        uint32_t col2;
        uint32_t glowColor;
        int zapped0;
        int zapped1;

        --count;
        start.vx = jpb_level_wrap_short(
            -0x7ffe - maSpark1[s1].vx);
        start.vy = jpb_level_wrap_short(
            maSpark1[s1].vy - 0x12);
        start.vz = jpb_level_wrap_short(
            maSpark1[s1].vz - 0x7f00);
        start.pad = 0;
        end.vx = jpb_level_wrap_short(
            -0x7ffe - maSpark2[s2].vx);
        end.vy = jpb_level_wrap_short(
            maSpark2[s2].vy - 0x12);
        end.vz = jpb_level_wrap_short(
            maSpark2[s2].vz - 0x7e62);
        end.pad = 0;

        zapped0 = zapcheck(
            &gaPlayerData[0],
            &start,
            &end,
            -20,
            &gaPlayerData[0],
            2);
        zapped1 = zapcheck(
            &gaPlayerData[1],
            &start,
            &end,
            -20,
            &gaPlayerData[0],
            2);
        zapped = zapped0 | zapped1;

        if (zapped == 0) {
            col1 = UINT32_C(0xc0ffffff);
            col2 = UINT32_C(0xc0ff8020);
            glowColor = UINT32_C(0xc0ff4020);
        } else {
            col1 = UINT32_C(0xc00000ff);
            col2 = UINT32_C(0xc0ffffff);
            glowColor = UINT32_C(0xc0ffffff);
        }
        PlotZap(
            col1,
            col2,
            col2,
            &start,
            &end,
            0x800,
            8);
        fx_screenGlow(
            &start,
            &end,
            glow + 0x20,
            glowColor);
    }

    if (spark > 0x60) {
        spark = 0;
    }

    for (index = 0;
         index < JPB_SPARK_ROOM_ARC_COUNT;
         ++index) {
        uint32_t col1;
        uint32_t col2;
        int previous = zapman[index];
        int zapped0;
        int zapped1;

        zapman[index] -= gGlobalFrameRate * 2;
        if (zapman[index] < 0) {
            zapman[index] =
                ((rand() & 0x3f) + 0x50) * 0x800;
        }
        if (zapman[index] >= 0x1e000) {
            continue;
        }

        start.vx = jpb_level_wrap_short(
            -0x8000 - maSpark1[index].vx);
        start.vy = jpb_level_wrap_short(
            maSpark1[index].vy);
        start.vz = jpb_level_wrap_short(
            maSpark1[index].vz - 0x7f00);
        start.pad = 0;
        end.vx = jpb_level_wrap_short(
            -0x8000 - maSpark2[index].vx);
        end.vy = jpb_level_wrap_short(
            maSpark2[index].vy);
        end.vz = jpb_level_wrap_short(
            maSpark2[index].vz - 0x7f00);
        end.pad = 0;

        if (previous >= 0x1e000) {
            pos.vx = jpb_level_average_short(
                start.vx, end.vx);
            pos.vy = jpb_level_average_short(
                start.vy, end.vy);
            pos.vz = jpb_level_average_short(
                start.vz, end.vz);
            pos.pad = 0;
            (void)sound_Play(
                &pos, 3, (char *)"smspark", 0);
        }

        zapped0 = zapcheck(
            &gaPlayerData[0],
            &start,
            &end,
            -20,
            &gaPlayerData[0],
            2);
        zapped1 = zapcheck(
            &gaPlayerData[1],
            &start,
            &end,
            -20,
            &gaPlayerData[0],
            2);
        zapped = zapped0 | zapped1;
        if (zapped == 0) {
            col1 = UINT32_C(0x00ff4020);
            col2 = UINT32_C(0x00ff8020);
        } else {
            col1 = UINT32_C(0xc0ffffff);
            col2 = UINT32_C(0xc0ffffff);
        }

        fx_screenGlow(
            &start, &end, glow, col1);
        PlotZap(
            col1,
            col2,
            col2,
            &start,
            &end,
            0x800,
            8);
    }

    if (++skip > 0x32) {
        skip = 0;
    }
    glow += delta;
    if (abs(0x40 - glow) > 6) {
        delta = -delta;
    }
}

/* 0xBB0A0, 384 bytes, global, 9 named locals
 * level_Theed
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void level_Theed(void)
{
    int level;
    float factor1;
    float factor2;

    if (theedZeroBSSCheck == zerobss_levelReset) {
        if (theedStart != 0) {
            goto initialized;
        }
    } else {
        theedZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
    }
    theedStart = gGlobalTimer;

initialized:
    (void)game_gSetGameFlags(UINT32_C(0x01000000));
    level = (int)(int8_t)LevelSelect;
    factor1 = (float)water_crud[level][1] * 0.0078125f;
    factor2 = (float)water_crud[level][3] * 0.0078125f;
    (void)debug_printf(
        (char *)"water %08x, %08x\n",
        water_colors[level][0],
        water_colors[level][1]);
    drawsomecrappywater(
        theed_water,
        7,
        factor1,
        factor2,
        (int)water_crud[level][0],
        (int)water_crud[level][2],
        water_colors[level][0],
        water_colors[level][1]);

    if ((GameStruct.GameState & UINT32_C(0x02000000)) == 0 &&
        (abGlobalBits[8] & UINT8_C(0x40)) != 0 &&
        GameStruct.Counter < 7) {
        abGlobalBits[3] |= UINT8_C(1);
        abGlobalBits[8] &= UINT8_C(0xbf);
        (void)game_gSetGameFlags(UINT32_C(0x20));
        (void)game_gSetGameFlags(UINT32_C(0x40));
        gCheckPoint = 0;
        reStartScore[0] = 0;
        reStartScore[1] = 0;
        GameStruct.Counter = 0;
    }
}

/* 0xBB220, 360 bytes, global, 2 named locals
 * oldlevel_Mini3
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xBB390, 457 bytes, global, 15 named locals
 * plotmaze
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xBB560, 364 bytes, global, 12 named locals
 * plotnode
 * PDB type: void (_svector*, int, int, int, ...
 * Source: W:\SWJediPowerBattles\work\level.c
 */

/* 0xBB6D0, 115 bytes, global, 4 named locals
 * standingonit
 * PDB type: int (int, _svector*, _svector*)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
int standingonit(
    int player_index,
    _svector *upper,
    _svector *lower)
{
    playerObject *player = &gaPlayerData[player_index];
    physicsObject *physics = &maPhysicsData[player_index];
    int z_distance;

    if ((player->pFlags & UINT32_C(1)) != 0 ||
        player->playerID == 0x48 ||
        physics->pos.vx <= (float)lower->vx ||
        physics->pos.vx >= (float)upper->vx ||
        physics->pos.vy < (float)upper->vy) {
        return 0;
    }
    z_distance =
        (int)physics->pos.vz - (int)upper->vz;
    if (z_distance < 0) {
        z_distance = -z_distance;
    }
    return z_distance < 66;
}
