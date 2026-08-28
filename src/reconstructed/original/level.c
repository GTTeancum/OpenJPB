/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\level.c.
 * PDB module: 0047
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\level.obj
 * Primary source: W:\SWJediPowerBattles\work\level.c
 * Compiler language: c
 * Emitted procedures: 33
 *
 * All 33 project procedures are represented under their PDB names and RVAs.
 * Their level-special dispatch reaches the exact scene scheduler directly;
 * no reconstruction-only level callback remains.
 */

#include "jpb/level.h"

#include "jpb/anim.h"
#include "jpb/animctrl.h"
#include "jpb/audio_stream.h"
#include "jpb/camera.h"
#include "jpb/debugtext.h"
#include "jpb/effects.h"
#include "jpb/fx.h"
#include "jpb/flex.h"
#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/intersec.h"
#include "jpb/jonny.h"
#include "jpb/linkstubs.h"
#include "jpb/menu.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/resources.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/text.h"
#include "jpb/texture.h"
#include "jpb/whook.h"
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

/* Exact initialized level_Ruins water table at matched-PC RVA 0x4BCC50. */
static _svector ruins_water[25] = {
    {126, 11, 75, 0x0a06},
    {140, 11, 68, 0x0606},
    {134, 11, 74, 0x0608},
    {136, 11, 80, 0x060a},
    {140, 11, 74, 0x0606},
    {150, 11, 72, 0x060a},
    {152, 11, 77, 0x0606},
    {160, 11, 72, 0x060a},
    {166, 11, 73, 0x0a06},
    {172, 11, 78, 0x0206},
    {171, 11, 88, 0x0a06},
    {179, 11, 88, 0x0a02},
    {177, 11, 88, 0x0a06},
    {180, 11, 98, 0x0608},
    {180, 11, 104, 0x0a08},
    {186, 11, 107, 0x0806},
    {190, 11, 100, 0x0904},
    {190, 11, 109, 0x0404},
    {215, 10, 76, 0x0706},
    {225, 10, 69, 0x0a0a},
    {215, 10, 70, 0x0607},
    {212, 10, 60, 0x0a05},
    {215, 10, 54, 0x0608},
    {225, 10, 53, 0x0a0a},
    {235, 10, 53, 0x0a0a}
};

/* Exact initialized Core level tables at RVAs 0x4BC990..0x4BCD3F. */
static _svector mazeorg[6] = {
    {51, 10, 56, 0}, {51, 10, 60, 0},
    {54, 10, 56, 0}, {54, 10, 60, 0},
    {57, 10, 56, 0}, {57, 10, 60, 0}
};
static const uint8_t core_xz[5][2] = {
    {0x48, 0x57}, {0x3a, 0x65}, {0x4b, 0x68},
    {0x52, 0x61}, {0x63, 0x68}
};
static const uint8_t core_spak[7] = {
    0x58, 0x56, 0x50, 0x4f, 0x4a, 0x48, 0x46
};
static const uint32_t hardmaze[6] = {
    UINT32_C(0x00444413), UINT32_C(0x00024a13),
    UINT32_C(0x00220a13), UINT32_C(0x00024853),
    UINT32_C(0x00400457), UINT32_C(0x00420252)
};
static const uint32_t easymaze[6] = {
    UINT32_C(0x00442802), UINT32_C(0x002a2a13),
    UINT32_C(0x00444457), UINT32_C(0x0020420e),
    UINT32_C(0x00080661), UINT32_C(0x00286c39)
};
static const uint32_t core_colors[6] = {
    UINT32_C(0x7f00007f), UINT32_C(0x7f7f0000),
    UINT32_C(0x7f007f00), UINT32_C(0x7f007f7f),
    UINT32_C(0x7f7f7f00), UINT32_C(0x7f7f007f)
};
static const int8_t core_triggers[6][4] = {
    {3, 3, 2, 0}, {0, 3, 1, 1}, {2, 3, 2, 0},
    {2, 2, 0, 3}, {2, 3, 2, 0}, {1, 2, 3, 3}
};
static const int32_t core_xvals[6] = {11, 12, 14, 15, 17, 20};
static const int32_t core_events[6][3] = {
    {3000, 400, 8192}, {4600, 300, 4000},
    {5450, 250, 9000}, {6400, 200, 10000},
    {7500, 175, 12000}, {8400, 150, 6000}
};
static _svector core_water[5] = {
    {65, 8, 53, 0x0a0a}, {75, 8, 53, 0x0a0a},
    {85, 8, 53, 0x0a0a}, {95, 8, 53, 0x0a0a},
    {105, 8, 53, 0x0a0a}
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

/* Core level-owner state at matched-PC RVAs 0x538228..0x538238. */
static int glowtime[2];
static uint32_t glowcolor[2];
static _Material *trans;
static int brightness1;
static int wideness1;
static int brightness2;
static int wideness2;
static int core_hack_ninehundred;
static int coreZeroBSSCheck;
static int bigwallZeroBSSCheck;
static uint32_t bigwallcolor[3];
static int currentmaze;
static int mazeTransition;
static int mazeVelocity;
static int mazeZeroBSSCheck;
static int planktimer;
static int plankZeroBSSCheck;
static uint8_t plotted[6];
static int timers[6];
static int flatZeroBSSCheck;
static _Material *knob;
static uint32_t coreFloorColor[6];
static int floorZeroBSSCheck;
static unsigned coreWaterStart;
static int coreWaterZeroBSSCheck;
/* Exact PDB module-local at matched-PC RVA 0x538338. */
static _Material *conveyortexture;

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

/* level_Mini1's PDB module-locals at RVAs 0x538340..0x538350. */
static unsigned mini1Start;
static int mini1Succeed;
static int mini1Failed;
static int mini1Delay;
static int mini1ZeroBSSCheck;

/* level_Mini2's PDB module-locals at RVAs 0x5382CC/0x5382D0. */
static unsigned mini2Start;
static int mini2ZeroBSSCheck;

/* level_Mini3's PDB module-locals at RVAs 0x5382D4..0x5382EC. */
static unsigned mini3Start;
static int mini3Succeed;
static int mini3Failed;
static int mini3Delay;
static int mini3Score1;
static int mini3Score2;
static int mini3ZeroBSSCheck;

/* level_Ruins' PDB module-locals at RVAs 0x538304/0x538308. */
static unsigned ruinsStart;
static int ruinsZeroBSSCheck;

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

static int32_t jpb_level_float_bits(float value)
{
    int32_t bits;

    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/* 0xB63A0, 132 bytes, global, 8 named locals
 * BigPinkPulsatingShaft
 * PDB type: void (int, int, int, unsigned, i...
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void BigPinkPulsatingShaft(
    int x,
    int z,
    int width,
    uint32_t color,
    int section,
    int zpush_value)
{
    _svector top = {
        (int16_t)x, INT16_C(-0x1800), (int16_t)z, 0};
    _svector bottom = {
        (int16_t)x, 0, (int16_t)z, 0};

    (void)section;
    (void)zpush_value;
    do {
        bottom.vy = (int16_t)(top.vy + 0x800);
        fx_screenSection(&top, &bottom, width, color);
        top.vy = (int16_t)(top.vy + 0x800);
    } while (top.vy <= 0x1800);
}

/* 0xB6430, 522 bytes, global, 5 named locals
 * bigcheck
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
int bigcheck(int index)
{
    playerObject *player = &gaPlayerData[index];
    physicsObject *physics = &maPhysicsData[index];
    _svector *points = (_svector *)(void *)&gaScratch[0x80];

    if (player->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) == 0 &&
        (player->pFlags & UINT32_C(0x00040200)) == 0 &&
        player->currentMotion != 0x2c &&
        player->currentMotion != 0x27 &&
        physics->pos.vx < (float)points[0].vx &&
        (float)points[1].vx < physics->pos.vx &&
        (float)(points[0].vy - 0x40) <= physics->pos.vy &&
        physics->pos.vy < (float)(points[0].vy + 0x300) &&
        abs((int)(physics->pos.vz - (float)points[0].vz)) < 0x40) {
        (void)game_gModEnergy(index, -20);
        physics_gSetFacing(&player->playerRoot, 0x1000);
        if ((player->pFlags & 1U) == 0) {
            (void)animctrl_MotionNoLock(
                &player->playerRoot, &player->paMotions[0x27]);
        } else {
            physics->airmov.vx = -physics->airmov.vx;
            physics->airmov.vz = -physics->airmov.vz;
            (void)animctrl_MotionNoLock(
                &player->playerRoot, &player->paMotions[0x2c]);
        }
        physics_gSetPosition(
            &player->playerRoot,
            (int)physics->pos.vx,
            (int)physics->pos.vy,
            points[0].vz - 0x46);
        return 1;
    }
    if ((float)points[0].vz < physics->pos.vz) {
        physics_gSetPosition(
            &player->playerRoot,
            (int)physics->pos.vx,
            (int)physics->pos.vy,
            points[0].vz - 0x46);
    }
    return 0;
}

/* 0xB6640, 479 bytes, local, 5 named locals
 * boxcheck
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
static int boxcheck(int effect)
{
    static const int8_t core_boxes[5][6] = {
        {0x30, 0x71, 0x11, 0x00, 0x6e, 0x50},
        {0x6f, 0x77, 0x29, 0x11, 0x5c, 0x35},
        {0x2d, 0x3f, 0x0d, 0x09, 0x3f, 0x35},
        {0x04, 0x1b, 0x10, 0x00, 0x40, 0x35},
        {0x4e, 0x61, 0x10, 0x0a, 0x6c, 0x65}
    };
    int players = 0;
    int i;

    for (i = 0; i < (int)(int8_t)GameStruct.NumPlayers; ++i) {
        playerObject *player = &gaPlayerData[i];
        physicsObject *physics = &maPhysicsData[i];

        if (player->playerRoot.objectID != -1 &&
            obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) == 0 &&
            (player->pFlags & UINT32_C(0x00040200)) == 0) {
            int x = (0x80ff - (int)physics->pos.vx) >> 8;
            int y = (int)physics->pos.vy >> 8;
            int z = ((int)physics->pos.vz + 0x7f00) >> 8;

            ++players;
            if (x < core_boxes[effect][0] ||
                core_boxes[effect][1] < x ||
                y < core_boxes[effect][3] ||
                core_boxes[effect][2] < y ||
                z < core_boxes[effect][5] ||
                core_boxes[effect][4] < z) {
                return 0;
            }
        }
    }
    if (players == 0 && effect == 0) {
        int x = (0x80ff - gpWorld->location.vx) >> 8;
        int z = (gpWorld->location.vz + 0x7f00) >> 8;

        if (core_boxes[0][0] <= x && x <= core_boxes[0][1] &&
            core_boxes[0][5] <= z && z <= core_boxes[0][4]) {
            return 1;
        }
    }
    return players != 0;
}

/* 0xB6820, 294 bytes, global, 3 named locals
 * calcboxcoord
 * PDB type: void (playerObject*, int*, int*)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void calcboxcoord(playerObject *player, int *x, int *y)
{
    physicsObject *physics;
    int cell;

    if (player->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) != 0 ||
        (player->pFlags & UINT32_C(0x00040200)) != 0) {
        *x = 3;
        *y = 3;
        return;
    }
    physics = (physicsObject *)(void *)(
        (sceneObject *)player->playerRoot.pParent)->pPhysics;
    cell = ((int)physics->pos.vz + 0x7f00) >> 8;
    if (cell < 0x38) {
        *x = 0;
    } else if ((unsigned)(cell - 0x39) < 3U) {
        *x = 1;
    } else if (cell > 0x3c) {
        *x = 2;
    }
    cell = (0x80ff - (int)physics->pos.vx) >> 8;
    if (cell <= 0x32) {
        *y = 3;
    } else if ((unsigned)(cell - 0x34) < 2U) {
        *y = 2;
    } else if ((unsigned)(cell - 0x37) < 2U) {
        *y = 1;
    } else if (cell >= 0x3a) {
        *y = 0;
    }
}

/* 0xB6950, 5257 bytes, global, 71 named locals
 * core_specials
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */
static uint32_t jpb_core_update_wall_color(int index, int enabled)
{
    int delta_value = flexmul12(
        enabled != 0 ? 8 : -8, gGlobalFrameRate);
    int value = (int)bigwallcolor[index] + delta_value;

    if (value < 0) {
        value = 0;
    } else if (value > 0xff) {
        value = 0xff;
    }
    bigwallcolor[index] = (uint32_t)value;
    return (uint32_t)value;
}

static void jpb_core_draw_walls(void)
{
    static const int16_t z_positions[3] = {
        INT16_C(-0x418c), INT16_C(-0x3c8c), INT16_C(-0x378c)
    };
    _svector *points = (_svector *)(void *)&gaScratch[0x80];
    int index;

    if (bigwallZeroBSSCheck != zerobss_levelReset) {
        memset(bigwallcolor, 0, sizeof(bigwallcolor));
    }
    for (index = 0; index < 3; ++index) {
        int enabled = (abGlobalBits[12] >> index) & 1;
        uint32_t alpha;
        uint32_t color;

        (void)debug_printf(
            (char *)(enabled != 0
                ? "SPECIAL BIT %d is ON!\n"
                : "SPECIAL BIT %d is OFF!\n"),
            index);
        alpha = jpb_core_update_wall_color(index, enabled);
        if (alpha == 0) {
            continue;
        }
        points[0] = (_svector){0x7bf0, 0x0a00, z_positions[index], 0};
        points[1] = (_svector){0x7710, 0x0a00, z_positions[index], 0};
        color = ((alpha & UINT32_C(0xfffffffe)) |
                 UINT32_C(0x0000fe00)) << 15;
        if (bigcheck(0) != 0 || bigcheck(1) != 0) {
            color = UINT32_C(0xff7f7f7f);
        }
        drawbigpoly(color, -0x100);
    }
}

static void jpb_core_draw_shafts(void)
{
    int index;

    (void)debug_printf((char *)"pulsating shafts\n");
    for (index = 0; index < 5; ++index) {
        int base_x = (int)core_xz[index][0] * -0x100;
        int world_z = ((int)core_xz[index][1] - 0x7f) * 0x100;
        int brightness = (rsin(brightness1) + 0x1000) >> 7;
        int width = (rsin(wideness1) >> 8) + 0x300;
        uint32_t color = color_interpolate(
            UINT32_C(0x90ff40a0), 0, brightness + 0xbe);
        _svector position = {
            (int16_t)(base_x - 0x8000),
            0,
            (int16_t)world_z,
            0
        };

        BigPinkPulsatingShaft(
            base_x - 0x7f80,
            world_z,
            width,
            color,
            index,
            0);
        brightness = (rsin(brightness2) + 0x1000) >> 7;
        width = (rsin(wideness2) >> 8) + 0x280;
        color = color_interpolate(
            UINT32_C(0x90ffffff), 0, brightness + 0xbe);
        BigPinkPulsatingShaft(
            base_x + 0x8080,
            world_z,
            width,
            color,
            index,
            0);
        corecheck0(&gaPlayerData[0], &position);
        corecheck0(&gaPlayerData[1], &position);
    }
}

static void jpb_core_draw_columns(void)
{
    int index;

    (void)debug_printf((char *)"wobbling columns\n");
    for (index = 0; index < 7; ++index) {
        int world_z = ((int)core_spak[index] - 0x7f) * 0x100;
        uint32_t random_value = (uint32_t)rand();
        _svector outer0 = {0x0e00, 0x2400, (int16_t)(world_z + 0x80), 0};
        _svector outer1 = {0x0b00, 0x2400, (int16_t)(world_z + 0x80), 0};
        _svector beam_start;
        _svector beam_end;
        FVECTOR line_start;
        FVECTOR line = {0.0f, 1.0f, 0.0f};
        float length;
        int wobble;

        fx_screenGlow(&outer0, &outer1, 0x10, UINT32_C(0xc07f7f7f));
        fx_screenGlow(&outer0, &outer1, 0x60, UINT32_C(0xc04f0028));
        outer0.vx -= 0xc0;
        outer0.vy -= 0xc0;
        outer1.vx -= 0xc0;
        outer1.vy -= 0xc0;
        if (index != 2) {
            fx_screenGlow(
                &outer0,
                &outer1,
                0x100,
                (random_value & 7U) != 0
                    ? UINT32_C(0x7f2f0018)
                    : UINT32_C(0x7f5e0030));
        }

        wobble = rsin(
            (((~index & 1) + 2) * globaltimer +
             ((int)core_spak[index] - 0x4e) * 0x80) * 8);
        line_start.vx = (float)(0x0c80 + ((wobble * 0x14) >> 8));
        line_start.vy = (float)0x2354;
        line_start.vz = (float)(world_z + 0x80);
        length = LineAndPlane(
            &clippingfrustrum[4],
            &line_start,
            &line,
            630.0f,
            0x100);
        beam_start.vx = (int16_t)(int)line_start.vx;
        beam_start.vy = 0x2434;
        beam_start.vz = (int16_t)(world_z + 0x80);
        beam_start.pad = 0;
        beam_end = beam_start;
        beam_end.vy = (int16_t)(beam_start.vy + (int16_t)(int)length);
        SetCameraMatrix();
        PlotZap(
            UINT32_C(0xbf7f003f),
            UINT32_C(0x7f003f7f),
            UINT32_C(0xbfbfbfbf),
            &beam_start,
            &beam_end,
            0x1000,
            0x30);
        corecheck1(&gaPlayerData[0], &beam_start);
        corecheck1(&gaPlayerData[1], &beam_start);
    }
    glowdeath();
}

static void jpb_core_draw_maze(void);
static void jpb_core_draw_flat_polys(void);
static void jpb_core_draw_floor_glow(void);
static void jpb_core_clamp_maul(void);

void core_specials(void)
{
    int level;
    float factor1;
    float factor2;

    if (coreZeroBSSCheck != zerobss_levelReset) {
        core_hack_ninehundred = 0;
    }
    brightness1 += 0x3c;
    wideness2 += 0xe0;
    wideness1 += 0x30;
    brightness2 += 0x94;
    if (core_hack_ninehundred == 0) {
        core_hack_ninehundred = 1;
        gpWorld->apEnemy[68]->actorNum = 7;
        gpWorld->apEnemy[68]->aiDf.ownerType = 3;
        gpWorld->apEnemy[69]->actorNum = 7;
        gpWorld->apEnemy[69]->aiDf.ownerType = 3;
        gpWorld->apEnemy[71]->actorNum = 7;
        gpWorld->apEnemy[72]->actorNum = 7;
        gpWorld->apEnemy[73]->actorNum = 7;
    }

    if (coreWaterZeroBSSCheck != zerobss_levelReset) {
        coreWaterStart = 0;
    }
    level = (int)(int8_t)LevelSelect;
    factor1 = (float)water_crud[level][1] * 0.0078125f;
    factor2 = (float)water_crud[level][3] * 0.0078125f;
    (void)debug_printf(
        (char *)"water %08x, %08x\n",
        water_colors[level][0],
        water_colors[level][1]);
    drawsomecrappywater(
        core_water,
        5,
        factor1,
        factor2,
        (int)water_crud[level][0],
        (int)water_crud[level][2],
        water_colors[level][0],
        water_colors[level][1]);

    jpb_core_draw_walls();
    if (boxcheck(0) != 0) {
        jpb_core_draw_shafts();
    }
    if (boxcheck(1) != 0) {
        jpb_core_draw_columns();
    }
    if (boxcheck(2) != 0) {
        jpb_core_draw_maze();
    }
    if (boxcheck(3) != 0) {
        jpb_core_draw_flat_polys();
    }
    if (boxcheck(4) != 0) {
        jpb_core_draw_floor_glow();
    }
    jpb_core_clamp_maul();
}

static int jpb_core_player_active(playerObject *player)
{
    return player->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) == 0 &&
        (player->pFlags & UINT32_C(0x00040200)) == 0;
}

static void jpb_core_maze_player_state(
    playerObject *player,
    int *players,
    int *forward,
    int *back)
{
    int x = -1;
    int y = -1;

    if (!jpb_core_player_active(player)) {
        *forward = 1;
        *back = 1;
        return;
    }
    calcboxcoord(player, &x, &y);
    ++*players;
    *forward =
        x == core_triggers[currentmaze][2] &&
        y == core_triggers[currentmaze][3];
    *back =
        x == core_triggers[currentmaze][0] &&
        y == core_triggers[currentmaze][1];
}

static void jpb_core_draw_maze(void)
{
    const uint32_t *maze;
    int next;
    int transition;
    int players = 0;
    int forward0;
    int forward1;
    int back0;
    int back1;

    if (mazeZeroBSSCheck != zerobss_levelReset) {
        mazeZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        currentmaze = 0;
        mazeTransition = 0;
        mazeVelocity = 0;
    }
    (void)debug_printf((char *)"Crappy maze\n");
    maze = GameStruct.difficulty != 0 ? hardmaze : easymaze;
    next = (currentmaze + 1) % 6;
    transition = 0x1000 - mazeTransition;
    if (transition < 0) {
        transition = 0;
    }
    plotmaze(
        maze[currentmaze],
        maze[next],
        core_colors[currentmaze],
        core_colors[next],
        transition);

    mazeTransition += flexmul12(mazeVelocity, gGlobalFrameRate);
    if (mazeTransition > 0x1000 || mazeTransition < 0) {
        ++currentmaze;
        if (currentmaze > 5) {
            currentmaze = GameStruct.difficulty != 0 ? 5 : 0;
        }
        mazeTransition = 0;
        mazeVelocity = 0;
    }
    if (GameStruct.difficulty == 0) {
        if (plankZeroBSSCheck != zerobss_levelReset) {
            plankZeroBSSCheck = zerobss_levelReset;
            zerobss_levelReset = 0;
            planktimer = 0;
        }
        planktimer += gGlobalFrameRate;
        if (planktimer > 0x87000) {
            if (mazeVelocity == 0) {
                (void)sound_Play(
                    NULL, 3, (char *)"lasrgate", 0);
            }
            planktimer = 0;
            mazeVelocity = 0x200;
        }
        return;
    }

    jpb_core_maze_player_state(
        &gaPlayerData[0], &players, &forward0, &back0);
    jpb_core_maze_player_state(
        &gaPlayerData[1], &players, &forward1, &back1);
    if (players != 0) {
        if (forward0 != 0 && forward1 != 0) {
            if (mazeVelocity == 0) {
                (void)sound_Play(
                    NULL, 3, (char *)"lasrgate", 0);
            }
            mazeVelocity = 0x100;
        } else if (back0 != 0 && back1 != 0) {
            if (mazeVelocity == 0) {
                (void)sound_Play(
                    NULL, 3, (char *)"lasrgate", 0);
            }
            --currentmaze;
            mazeTransition = 0x1000;
            mazeVelocity = -0x100;
        }
    }
}

static void jpb_core_submit_flat_poly(_svector *points)
{
    FVECTOR transformed[4];
    int vertex;

    points[2] = points[0];
    points[3] = points[1];
    points[2].vz = (int16_t)(points[0].vz - 0x500);
    points[3].vz = (int16_t)(points[1].vz - 0x500);
    (void)TransformPointsFV(points, transformed, 4);
    if (knob == NULL) {
        knob = _LoadTexture(NULL, TT_SPRITE, 2);
    }
    _StartPoly(4, knob);
    for (vertex = 0; vertex < 4; ++vertex) {
        _SetVert(
            vertex,
            transformed[vertex].vx,
            transformed[vertex].vy,
            transformed[vertex].vz,
            UINT32_C(0x90804020),
            0.0f,
            0.0f);
    }
    _NoScaleEndPoly();
}

static void jpb_core_draw_flat_polys(void)
{
    _svector *points = (_svector *)(void *)&gaScratch[0x80];
    int index;

    if (flatZeroBSSCheck != zerobss_levelReset) {
        flatZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        memset(plotted, 0, sizeof(plotted));
        memset(timers, 0, sizeof(timers));
    }
    (void)debug_printf((char *)"Flat polys\n");
    for (index = 0; index < 6; ++index) {
        int elapsed;
        int intensity;

        points[0] = (_svector){
            (int16_t)(core_xvals[index] * -0x100 - 0x8000),
            0x0a74,
            INT16_C(-0x4256),
            0
        };
        points[1] = (_svector){
            (int16_t)(core_xvals[index] * -0x100 - 0x7f00),
            0x0a74,
            INT16_C(-0x4256),
            0
        };
        elapsed = timers[index] - core_events[index][2];
        if (elapsed < core_events[index][0] + 0x2000) {
            if (elapsed < core_events[index][0] + 0x1000) {
                intensity = elapsed > 0x1000 ? 0x1000 : elapsed;
            } else {
                intensity =
                    core_events[index][0] - elapsed + 0x2000;
            }
            if (intensity < 0) {
                intensity = 0;
            } else if (intensity > 0x0fff) {
                intensity = 0x0fff;
            }
            fx_screenGlow(
                &points[0],
                &points[1],
                8,
                color_interpolate4k(
                    UINT32_C(0xc0bfbfbf), 0, intensity));
            fx_screenGlow(
                &points[0],
                &points[1],
                0x30,
                color_interpolate4k(
                    UINT32_C(0xc07f0000), 0, intensity));

            if (elapsed > 0x1000 &&
                elapsed < core_events[index][0] + 0x1000) {
                jpb_core_submit_flat_poly(points);
                corecheck3(&gaPlayerData[0], &points[0]);
                corecheck3(&gaPlayerData[1], &points[0]);
                if (plotted[index] == 0) {
                    (void)sound_PlaySV(
                        &points[0],
                        3,
                        (char *)"beam_on",
                        0);
                }
                plotted[index] = 1;
            } else {
                plotted[index] = 0;
            }
        } else if (
            core_events[index][1] + 0x2000 +
                core_events[index][0] < elapsed) {
            timers[index] = 0;
        }
        if ((GameStruct.GameState & UINT32_C(0x02000000)) == 0) {
            timers[index] += flexmul12(0x100, gGlobalFrameRate);
        }
    }
    glowdeath();
}

static void jpb_core_draw_floor_glow(void)
{
    int index;

    if (floorZeroBSSCheck != zerobss_levelReset) {
        floorZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        memset(coreFloorColor, 0, sizeof(coreFloorColor));
    }
    (void)debug_printf((char *)"Floor glow\n");
    for (index = 0; index < 6; ++index) {
        (void)corefloorglow(
            &coreFloorColor[index],
            index * 0x80 - 0x17cc);
    }
}

static void jpb_core_clamp_maul(void)
{
    int index;

    if ((gpWorld->aDolly[gpWorld->currentDolly].flags &
         UINT32_C(0x400)) != 0) {
        return;
    }
    for (index = 2; index < JPB_PLAYER_CAPACITY; ++index) {
        playerObject *player = &gaPlayerData[index];

        if (player->playerID == 0x2b) {
            physicsObject *physics = (physicsObject *)(void *)(
                (sceneObject *)player->playerRoot.pParent)->pPhysics;

            if (physics->pos.vx > 30411.0f &&
                physics->pos.vx < 31725.0f &&
                physics->pos.vz > -13770.0f &&
                physics->pos.vz < -12209.0f &&
                physics->pos.vy < 1855.0f) {
                physics_gSetPosition(
                    &player->playerRoot,
                    0x7989,
                    0x0a00,
                    -0x3558);
            }
            break;
        }
    }
}

/* 0xB7DE0, 271 bytes, global, 5 named locals
 * corecheck0
 * PDB type: void (playerObject*, _svector*)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void corecheck0(playerObject *player, _svector *position)
{
    physicsObject *physics;
    _svector bob;

    if (player->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) != 0 ||
        (player->pFlags & UINT32_C(0x00040200)) != 0) {
        return;
    }
    physics = &maPhysicsData[player->playernum];
    if (physics->movemode == 5) {
        return;
    }
    bob.vx = (int16_t)(int)(
        physics->pos.vx - (float)(position->vx + 0x80));
    bob.vy = 0;
    bob.vz = (int16_t)(int)(
        physics->pos.vz - (float)(position->vz + 0x80));
    if (veclength(&bob) < 0x118) {
        physics->movemode = 5;
        physics->uservector.vx = position->vx + 0x80;
        physics->uservector.vz = position->vz + 0xe0;
        physics->mov.vx = 0.0f;
        physics->mov.vy = 0.0f;
        physics->userdata[0] = (int)physics->airmov.vy;
        physics->airmov.vy = 0.0f;
    }
}

/* 0xB7EF0, 279 bytes, global, 4 named locals
 * corecheck1
 * PDB type: void (playerObject*, _svector*)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void corecheck1(playerObject *player, _svector *position)
{
    physicsObject *physics;
    int id;

    if (player->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) != 0 ||
        (player->pFlags & UINT32_C(0x00040200)) != 0) {
        return;
    }
    physics = (physicsObject *)(void *)(
        (sceneObject *)player->playerRoot.pParent)->pPhysics;
    if (abs((int)(physics->pos.vx - (float)position->vx)) >= 0x40 ||
        abs((int)(physics->pos.vz - (float)position->vz)) >= 0x40) {
        return;
    }
    id = player->playerRoot.objectID;
    if (glowtime[id] == 0) {
        EffectHeader *effect = paEffects[13];

        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            8);
        (void)sound_Play(
            &physics->vpos,
            id + 1,
            (char *)"vbjhit",
            0);
    }
    glowtime[id] = 0x1000;
    glowcolor[id] = UINT32_C(0xff281810);
}

/* 0xB8010, 498 bytes, global, 7 named locals
 * corecheck2
 * PDB type: int (playerObject*, _svector*, _...
 * Source: W:\SWJediPowerBattles\work\level.c
 */
int corecheck2(
    playerObject *player,
    _svector *position1,
    _svector *position2)
{
    physicsObject *physics;
    int left = position1->vx;
    int right = position2->vx;
    int bottom = position1->vz;
    int top = position1->vz;
    int facing;

    if (player->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) != 0 ||
        (player->pFlags & UINT32_C(0x00040200)) != 0) {
        return 0;
    }
    physics = (physicsObject *)(void *)(
        (sceneObject *)player->playerRoot.pParent)->pPhysics;
    if (position1->vx == position2->vx) {
        top = position2->vz;
        right = position1->vx;
        if (position2->vz <= position1->vz) {
            bottom = position2->vz;
            top = position1->vz;
        }
    } else if (position1->vx >= position2->vx) {
        left = position2->vx;
        right = position1->vx;
    }
    if (!(physics->pos.vx > (float)(left - 0x30) &&
          physics->pos.vx < (float)(right + 0x30) &&
          physics->pos.vz > (float)(bottom - 0x30) &&
          physics->pos.vz < (float)(top + 0x30)) ||
        player->currentMotion == 0x2c ||
        player->currentMotion == 0x27) {
        return 0;
    }
    (void)game_gModEnergy(player->playernum, -20);
    if (position1->vx == position2->vx) {
        facing = physics->pos.vx <= (float)position1->vx
            ? 0x1400
            : 0x0c00;
    } else {
        facing = (float)position1->vz < physics->pos.vz
            ? 0x0800
            : 0x1000;
    }
    physics_gSetFacing(&player->playerRoot, facing);
    if ((player->pFlags & 1U) == 0) {
        (void)animctrl_MotionNoLock(
            &player->playerRoot, &player->paMotions[0x27]);
    } else {
        physics->airmov.vx = -physics->airmov.vx;
        physics->airmov.vz = -physics->airmov.vz;
        (void)animctrl_MotionNoLock(
            &player->playerRoot, &player->paMotions[0x2c]);
    }
    return 1;
}

/* 0xB8210, 202 bytes, global, 3 named locals
 * corecheck3
 * PDB type: void (playerObject*, _svector*)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void corecheck3(playerObject *player, _svector *points)
{
    physicsObject *physics;
    int id;

    if (player->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) != 0 ||
        (player->pFlags & UINT32_C(0x00040200)) != 0) {
        return;
    }
    physics = (physicsObject *)(void *)(
        (sceneObject *)player->playerRoot.pParent)->pPhysics;
    if (!(physics->pos.vx > (float)points[0].vx &&
          physics->pos.vx < (float)points[1].vx &&
          physics->pos.vy < (float)points[0].vy)) {
        return;
    }
    id = player->playerRoot.objectID;
    if (glowtime[id] == 0) {
        (void)sound_Play(
            &physics->vpos,
            id + 1,
            (char *)"vbjhit",
            0);
    }
    glowtime[id] = 0x1000;
    glowcolor[id] = UINT32_C(0xff703218);
}

/* 0xB82E0, 375 bytes, global, 7 named locals
 * corefloorglow
 * PDB type: int (unsigned*, int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
int corefloorglow(uint32_t *color, int position)
{
    _svector p0 = {0x2c9c, 0x0ce0, (int16_t)position, 0};
    _svector p1 = {0x2747, 0x0ce0, (int16_t)position, 0};
    int add1 = flexmul12(-4, gGlobalFrameRate);
    int add2 = flexmul12(-4, gGlobalFrameRate);
    int i;

    for (i = 0; i < JPB_PLAYER_CAPACITY; ++i) {
        playerObject *player = &gaPlayerData[i];
        physicsObject *physics = &maPhysicsData[i];

        if ((player->pFlags & 1U) == 0 && player->playerID != 0x48 &&
            jpb_level_float_bits(physics->pos.vx) > p1.vx &&
            jpb_level_float_bits(physics->pos.vx) < p0.vx &&
            jpb_level_float_bits(physics->pos.vy) >= p0.vy &&
            abs(jpb_level_float_bits(physics->pos.vz) - p0.vz) < 0x42) {
            int add = flexmul12(0x10, gGlobalFrameRate);

            if (i < 2) {
                add1 = add;
            } else {
                add2 = add;
            }
        }
    }
    {
        int value = (int)*color + add2 + add1;

        if (value < 0) {
            value = 0;
        } else if (value > 0x7f) {
            value = 0x7f;
        }
        *color = (uint32_t)value;
    }
    zpush = 0x20;
    fx_screenGlow(
        &p0,
        &p1,
        0x20,
        UINT32_C(0xc0000000) |
            (*color << 16) | (*color << 8) | *color);
    zpush = 0x20;
    fx_screenGlow(
        &p0, &p1, 0x40, UINT32_C(0xc06f5f4f));

}

/* 0xB8460, 585 bytes, global, 5 named locals
 * drawbigpoly
 * PDB type: void (unsigned, int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void drawbigpoly(uint32_t color, int sort_offset)
{
    _svector *points = (_svector *)(void *)&gaScratch[0x80];
    FVECTOR transformed[4];
    uint32_t glow_color;
    int i;

    points[2].vx = points[0].vx;
    points[2].vy = (int16_t)(points[0].vy + 0x300);
    points[2].vz = points[0].vz;
    points[3].vx = points[1].vx;
    points[3].vy = (int16_t)(points[1].vy + 0x300);
    points[3].vz = points[1].vz;
    points[4].vx = (int16_t)((points[0].vx + points[1].vx) >> 1);
    points[4].vy = (int16_t)(
        ((points[0].vy + points[1].vy) >> 1) + sort_offset);
    points[4].vz = (int16_t)((points[0].vz + points[1].vz) >> 1);

    (void)TransformPointsFV(points, transformed, 4);
    if (trans == NULL) {
        trans = _LoadTexture(NULL, TT_SPRITE, 2);
    }
    trans->flags = 1;
    _StartPoly(4, trans);
    for (i = 0; i < 4; ++i) {
        _SetVert(
            i,
            transformed[i].vx,
            transformed[i].vy,
            transformed[i].vz,
            color,
            0.0f,
            0.0f);
    }
    _NoScaleEndPoly();

    glow_color =
        ((color & UINT32_C(0x007f7f7f)) |
         UINT32_C(0xe0000000)) * 2U;
    fx_screenGlow(&points[0], &points[2], 0x0c, glow_color);
    fx_screenGlow(&points[1], &points[3], 0x0c, glow_color);
    fx_screenGlow(&points[2], &points[3], 0x0c, glow_color);
}

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
void glowdeath(void)
{
    int player;

    for (player = 0; player < 2; ++player) {
        if (glowtime[player] != 0) {
            uint32_t color = (rand() & 7) == 0
                ? UINT32_C(0xc07f7f7f)
                : glowcolor[player];
            int decay;

            color = color_interpolate4k(
                color, 0, glowtime[player]);
            fx_GlowingMan(
                &gaPlayerData[player].playerRoot,
                0x2a,
                0x2a,
                color,
                color);
            decay = flexmul12(0x80, gGlobalFrameRate);
            if (glowtime[player] - decay < 0) {
                glowtime[player] = 0;
            } else {
                glowtime[player] -=
                    flexmul12(0x80, gGlobalFrameRate);
                if (glowtime[player] > 0x0c00) {
                    (void)game_gModEnergy(player, -1);
                }
            }
        }
    }
}

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
                char *message;
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
                    message);
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
void level_CoreWater(void)
{
    int level;
    float factor1;
    float factor2;

    if (coreWaterZeroBSSCheck != zerobss_levelReset) {
        coreWaterStart = 0;
    }
    level = (int)(int8_t)LevelSelect;
    factor1 = (float)water_crud[level][1] * 0.0078125f;
    factor2 = (float)water_crud[level][3] * 0.0078125f;
    (void)debug_printf(
        (char *)"water %08x, %08x\n",
        water_colors[level][0],
        water_colors[level][1]);
    drawsomecrappywater(
        core_water,
        5,
        factor1,
        factor2,
        (int)water_crud[level][0],
        (int)water_crud[level][2],
        water_colors[level][0],
        water_colors[level][1]);
}

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
void level_InitSpecials(int level)
{
    char *fullFilePath;

    if (level == 1) {
        fullFilePath = (char *)resource_getPath(
            "fed\\belt.tga", JPB_RESOURCE_LEVEL_JPX);
        conveyortexture = _LoadTexture(
            texture_Name(fullFilePath), TT_LEVEL, 0);
    }
}

/* 0xB9AA0, 1289 bytes, global, 20 named locals
 * level_Mini1
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void level_Mini1(int time, int kill)
{
    float timerX;
    float timerY;
    float labelX;
    float labelY;
    float textX;
    float textY;
    float width;
    float height;
    int remaining;

    if (mini1ZeroBSSCheck != zerobss_levelReset) {
        mini1ZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        mini1Start = 0;
        mini1Succeed = 0;
        mini1Failed = 0;
        mini1Delay = 0;
        gDeathCount = 0;
        gPilotDeathCount = 0;
    }
    if (OptionStruct.DebugLevel != 0) {
        return;
    }
    if (mini1Start == 0) {
        mini1Start = gGlobalTimer;
    }
    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0 ||
        nextLevel == 1) {
        return;
    }

    if ((abGlobalBits[3] & UINT8_C(2)) != 0 &&
        mini1Succeed == 0 && mini1Failed != 1) {
        mini1Succeed = 1;
        mini1Delay = 0xff;
        if (OptionStruct.Music != 0) {
            playXA(4, (unsigned)OptionStruct.musicVolume * 2U, 0);
        }
        return;
    }

    if (time > 0) {
        width = 0.0f;
        height = 0.0f;
        timerX = -89.0f;
        timerY = 267.0f;
        labelX = 33.0f;
        labelY = 236.0f;
        time -= (int)((gGlobalTimer - mini1Start) / UINT32_C(0x3c00));
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
            if (remaining < 1 && mini1Failed == 0 &&
                mini1Succeed != 1) {
                mini1Failed = 1;
                mini1Delay = 0xff;
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

    if (kill > 0 && kill - gDeathCount <= 0 &&
        mini1Succeed == 0 && mini1Failed != 1) {
        mini1Succeed = 1;
        mini1Delay = 0xff;
        if (OptionStruct.Music != 0) {
            playXA(4, (unsigned)OptionStruct.musicVolume * 2U, 0);
        }
        return;
    }

    if (mini1Succeed == 1) {
        textX = 0.0f;
        textY = 200.0f;
        --mini1Delay;
        if (mini1Delay > 1) {
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
            if ((uint8_t)GameStruct.CurrentLevel == 14 &&
                (secretBits & UINT32_C(0x100)) == 0) {
                secretBits |= UINT32_C(0x100);
            }
            return;
        }
        abGlobalBits[0] |= UINT8_C(2);
    } else if (mini1Failed == 1) {
        width = 0.0f;
        height = 0.0f;
        textX = 0.0f;
        textY = 200.0f;
        setPivotPositionAndFixScale(
            &textX, &textY, &width, &height, 1);
        --mini1Delay;
        if (mini1Delay > 1) {
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
        reStartScore[1] = 0;
    } else {
        return;
    }

    mini1Delay = 0;
    mini1Failed = 0;
    mini1Succeed = 0;
    mini1Start = 0;
}

/* 0xB9FB0, 603 bytes, global, 14 named locals
 * level_Mini2
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void level_Mini2(void)
{
    float timerX = -89.0f;
    float timerY = 137.0f;
    float labelX = 33.0f;
    float labelY = 106.0f;
    float p1LabelX = -350.0f;
    float p1LabelY = 106.0f;
    float p2LabelX = 215.0f;
    float p2LabelY = 106.0f;
    float width = 0.0f;
    float height = 0.0f;
    unsigned current;

    if (mini2ZeroBSSCheck != zerobss_levelReset) {
        mini2ZeroBSSCheck = zerobss_levelReset;
        mini2Start = 0;
        zerobss_levelReset = 0;
    }
    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }
    if (mini2Start == 0) {
        mini2Start = gGlobalTimer;
    }
    current = (gGlobalTimer - mini2Start) / UINT32_C(0x3c00);

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

    setPivotPosition(&p1LabelX, &p1LabelY, 7);
    (void)_DrawText(
        p1LabelX,
        p1LabelY,
        0.0001f,
        0.5f,
        UINT32_C(0x7fffffff),
        (char *)(void *)allText[237]);
    setPivotPosition(&p2LabelX, &p2LabelY, 7);
    (void)_DrawText(
        p2LabelX,
        p2LabelY,
        0.0001f,
        0.5f,
        UINT32_C(0x7fffffff),
        (char *)(void *)allText[238]);

    if (current > 200U) {
        abGlobalBits[3] |= UINT8_C(1);
        (void)game_gSetGameFlags(UINT32_C(0x20));
        (void)game_gSetGameFlags(UINT32_C(0x40));
        gCheckPoint = 0;
        reStartScore[0] = 0;
        reStartScore[1] = 0;
    }
}

/* 0xBA210, 1428 bytes, global, 25 named locals
 * level_Mini3
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void level_Mini3(void)
{
    float timerX;
    float timerY;
    float labelX;
    float labelY;
    float score1X;
    float score1Y;
    float score2X;
    float score2Y;
    float textX;
    float textY;
    float width;
    float height;
    int current;

    if (mini3ZeroBSSCheck != zerobss_levelReset) {
        mini3ZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        mini3Start = 0;
        mini3Succeed = 0;
        mini3Failed = 0;
        mini3Delay = 0;
        mini3Score1 = 0;
        mini3Score2 = 0;
    }

    gpWorld->player0->paMotions[1].Charge = 12;
    gpWorld->player1->paMotions[1].Charge = 12;
    if (mini3Start == 0) {
        mini3Start = gGlobalTimer;
    }
    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }

    if (mini3Succeed == 1) {
        textX = 0.0f;
        textY = 200.0f;
        setPivotPosition(&textX, &textY, 1);
        --mini3Delay;
        if (mini3Delay > 1) {
            (void)SDLTextWriteScale(
                11,
                0x80,
                2,
                (int)textX,
                (int)textY,
                3.0f,
                2,
                allText[435]);
            return;
        }
        abGlobalBits[0] |= UINT8_C(2);
        return;
    }
    if (mini3Failed == 1) {
        width = 0.0f;
        height = 0.0f;
        textX = 0.0f;
        textY = 200.0f;
        setPivotPositionAndFixScale(
            &textX, &textY, &width, &height, 1);
        --mini3Delay;
        if (mini3Delay > 1) {
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
        reStartScore[1] = 0;
        return;
    }

    width = 0.0f;
    height = 0.0f;
    timerX = -89.0f;
    timerY = 152.0f;
    labelX = 33.0f;
    labelY = 121.0f;
    current = 90 -
        (int)((gGlobalTimer - mini3Start) / UINT32_C(0x3c00));
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

    if (current < 11) {
        if (gGlobalTimer % UINT32_C(0x3c00) == 0) {
            (void)sound_Play(NULL, 3, "xtimerbp", 0);
        }
        if (current < 1 && mini3Succeed == 0) {
            mini3Failed = 1;
            mini3Delay = 0xff;
            if (OptionStruct.Music != 0) {
                playXA(
                    3,
                    (unsigned)OptionStruct.musicVolume * 2U,
                    0);
            }
            return;
        }
    } else if (current == (current / 10) * 10 &&
               gGlobalTimer % UINT32_C(0x3c00) == 0) {
        (void)sound_Play(NULL, 3, "xtimerbp", 0);
    }

    if ((abGlobalBits[5] & UINT8_C(0x80)) != 0) {
        (void)sound_Play(NULL, 0, "xsecret", 0);
        ++mini3Score1;
    }
    if ((abGlobalBits[6] & UINT8_C(1)) != 0) {
        (void)sound_Play(NULL, 0, "xsecret", 0);
        ++mini3Score2;
    }

    score1X = -300.0f;
    score1Y = 56.0f;
    score2X = 300.0f;
    score2Y = 56.0f;
    setPivotPositionAndFixScale(
        &score1X, &score1Y, &width, &height, 1);
    setPivotPositionAndFixScale(
        &score2X, &score2Y, &width, &height, 1);
    (void)SDLTextWriteScale(
        11,
        0x80,
        1,
        (int)score1X,
        (int)score1Y,
        3.0f,
        2,
        "%d",
        mini3Score1);
    (void)SDLTextWriteScale(
        11,
        0x80,
        0,
        (int)score2X,
        (int)score2Y,
        3.0f,
        2,
        "%d",
        mini3Score2);

    if (mini3Score1 < 3 && mini3Score2 < 3) {
        return;
    }
    if ((GameStruct.NumPlayers == 1 && mini3Score1 >= 3) ||
        GameStruct.NumPlayers == 2) {
        if (mini3Succeed == 0) {
            mini3Succeed = 1;
            mini3Delay = 0xff;
            if (OptionStruct.Music != 0) {
                playXA(
                    4,
                    (unsigned)OptionStruct.musicVolume * 2U,
                    0);
            }
        }
        return;
    }
    if (mini3Failed == 0) {
        mini3Failed = 1;
        mini3Delay = 0x1e;
    }
}

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
void level_Ruins(void)
{
    int currentX;
    int currentZ;
    int level;
    float factor1;
    float factor2;

    if (ruinsZeroBSSCheck != zerobss_levelReset) {
        ruinsZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        ruinsStart = 0;
    }

    currentX = (0x80ff - gpWorld->location.vx) >> 8;
    currentZ = (gpWorld->location.vz + 0x7f00) >> 8;
    if ((uint32_t)(currentX - 0xdd) > UINT32_C(0x1c) ||
        (uint32_t)(currentZ - 0x3c) > UINT32_C(6)) {
        level = (int)(int8_t)LevelSelect;
        factor1 = (float)water_crud[level][1] * 0.0078125f;
        factor2 = (float)water_crud[level][3] * 0.0078125f;
        (void)debug_printf(
            (char *)"water %08x, %08x\n",
            water_colors[level][0],
            water_colors[level][1]);
        drawsomecrappywater(
            ruins_water,
            25,
            factor1,
            factor2,
            (int)water_crud[level][0] * 2,
            (int)water_crud[level][2] * 2,
            water_colors[level][0],
            water_colors[level][1]);
    }
}

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
void oldlevel_Mini3(void)
{
    int score1 = 0;
    int score2 = 0;

    gpWorld->player0->paMotions[1].Charge = 12;
    gpWorld->player1->paMotions[1].Charge = 12;
    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }

    menu_drawBigNums(
        90,
        2,
        0x100,
        OptionStruct.ScreenHeight - 22U,
        0x80,
        0x80,
        0x80);
    if (gGlobalTimer % UINT32_C(0x3c00) == 0) {
        (void)sound_Play(NULL, 3, "xtimerbp", 0);
    }
    if ((abGlobalBits[5] & UINT8_C(0x80)) != 0) {
        (void)sound_Play(NULL, 0, "xsecret", 0);
        score1 = 1;
    }
    if ((abGlobalBits[6] & UINT8_C(1)) != 0) {
        (void)sound_Play(NULL, 0, "xsecret", 0);
        score2 = 1;
    }
    menu_drawBigNums(score1, 2, 0xa0, 0x20, 0x80, 0x80, 0x80);
    menu_drawBigNums(score2, 2, 0x1c0, 0x20, 0x80, 0x80, 0x80);
}

/* 0xBB390, 457 bytes, global, 15 named locals
 * plotmaze
 * PDB type: void (unsigned, unsigned, unsign...
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void plotmaze(
    unsigned maze1,
    unsigned maze2,
    unsigned color1,
    unsigned color2,
    int transition)
{
    int loop;

    for (loop = 0; loop < 6; ++loop) {
        int length = (2 - (loop & 1)) * 0x100;

        plotnode(
            &mazeorg[loop],
            (int)(maze1 & 1U),
            (int)(maze2 & 1U),
            1,
            0,
            color1,
            color2,
            transition,
            0x100);
        plotnode(
            &mazeorg[loop],
            (int)(maze1 & 2U),
            (int)(maze2 & 2U),
            0,
            1,
            color1,
            color2,
            transition,
            length);
        plotnode(
            &mazeorg[loop],
            (int)(maze1 & 4U),
            (int)(maze2 & 4U),
            -1,
            0,
            color1,
            color2,
            transition,
            0x100);
        plotnode(
            &mazeorg[loop],
            (int)(maze1 & 8U),
            (int)(maze2 & 8U),
            0,
            -1,
            color1,
            color2,
            transition,
            length);
        maze1 >>= 4;
        maze2 >>= 4;
    }
}

/* 0xBB560, 364 bytes, global, 12 named locals
 * plotnode
 * PDB type: void (_svector*, int, int, int, ...
 * Source: W:\SWJediPowerBattles\work\level.c
 */
void plotnode(
    _svector *node,
    int direction1,
    int direction2,
    int x_offset,
    int y_offset,
    uint32_t color1,
    uint32_t color2,
    int transition,
    int length)
{
    _svector *points = (_svector *)(void *)&gaScratch[0x80];
    uint32_t color;
    int hit1;
    int hit2;

    if (direction1 == 0 && direction2 == 0) {
        return;
    }
    color = color_interpolate(
        direction1 != 0 ? color1 : 0,
        direction2 != 0 ? color2 : 0,
        transition);
    if (color == 0) {
        return;
    }
    points[0].vx = (int16_t)(
        INT16_C(-0x8000) - ((uint16_t)node->vx << 8));
    points[0].vy = (int16_t)((uint16_t)node->vy << 8);
    points[0].vz = (int16_t)(node->vz * 0x100 - 0x7f00);
    points[1].vx = (int16_t)(
        points[0].vx + x_offset * 0x100 + 0x80);
    points[1].vy = points[0].vy;
    points[1].vz = (int16_t)(
        points[0].vz + y_offset * 0x100 + 0x80);
    points[2].vx = (int16_t)(points[1].vx + length * x_offset);
    points[2].vz = (int16_t)(points[1].vz + length * y_offset);

    hit1 = corecheck2(&gaPlayerData[0], &points[0], &points[1]);
    hit2 = corecheck2(&gaPlayerData[1], &points[0], &points[1]);
    if (hit1 != 0 || hit2 != 0) {
        color = UINT32_C(0xff7f7f7f);
    }
    drawbigpoly(color, 0x106);
}

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
