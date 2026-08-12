/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\jedi.c.
 *
 * Provenance for jedi_GetColour:
 *   direct     - name, signature, CVECTOR return type, and linked colour
 *                globals from the exact PDB.
 *   decompiled - bounds and fallback checked against the raw Ghidra export.
 *   assembly   - unsigned player-ID comparison, indexed four-byte load, and
 *                white fallback checked at RVA 0xB2270..0xB2286.
 *
 * Exact jedi_InitPlayer and jedi_Main preserve their PDB names. Their combo
 * and collision-table selections, model/player stores, callback publication,
 * Maul upgrade, and Force-state cleanup are checked against initialized image
 * data and RVAs 0xB3220..0xB35D7.
 * Exact jedi_FireWeapon restores the ranged-player projectile type, authored
 * muzzle/aim-node switch, target acquisition, powered-shot override, and
 * paired-shot path. Exact tusken_stab restores its frame-gated hot node.
 *
 * jedi_HandleSabre retains its exact PDB name and typed parameters. Its node
 * selection, fixed-point blade endpoints, colors, glow/core calls, world
 * sweep, feedback, power-state decisions, and node timers are checked against
 * the raw decompilation and machine-code call sites at 0xB23F0..0xB312D.
 * jedi_DrawBlur and the powered cylinders keep those original call roles but
 * currently use a documented dependency-free glow realization rather than
 * claiming immediate-mode textured parity.
 *
 * PDB module: 0045
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\jedi.obj
 * Primary source: W:\SWJediPowerBattles\Work\jedi.c
 * Compiler language: c
 * Emitted procedures: 30
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/jedi.h"
#include "jpb/anim.h"
#include "jpb/animutil.h"
#include "jpb/bullet.h"
#include "jpb/collision.h"
#include "jpb/combo.h"
#include "jpb/fx.h"
#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/intersec.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/vectors.h"

#include <stdlib.h>

/*
 * Exact initialized saber tables at matched-PC RVAs 0x4BAED0..0x4BAFD0.
 * Each linked table has ten storage entries although the published logical
 * length is nine. CVECTOR byte order is the executable's r/g/b/cd order.
 */
int gJediColorSpriteLegacy[JPB_JEDI_COLOUR_STORAGE_COUNT] = {
    0xb2, 0xb3, 0xb2, 0xb5, 0xb6,
    0xb5, -1, -1, 0xb4, 0
};
int gJediColorSpriteCanon[JPB_JEDI_COLOUR_STORAGE_COUNT] = {
    0xb2, 0xb3, 0xb4, 0xb2, 0xb2,
    0xb5, -1, -1, 0xb2, 0
};
int gJediColorSpriteCurrent[JPB_JEDI_COLOUR_STORAGE_COUNT] = {
    0xb2, 0xb3, 0xb4, 0xb2, 0xb2,
    0xb5, -1, -1, 0xb2, 0
};
CVECTOR gJediColourLegacy[JPB_JEDI_COLOUR_STORAGE_COUNT] = {
    {0xf8, 0x80, 0x38, 0x1b},
    {0x3c, 0xc0, 0x01, 0x00},
    {0xf8, 0x80, 0x38, 0x1b},
    {0x10, 0x20, 0xc0, 0x01},
    {0x01, 0xc0, 0xf8, 0x06},
    {0x10, 0x10, 0xc0, 0x01},
    {0x01, 0xf0, 0x58, 0x00},
    {0xf8, 0x80, 0x38, 0x1b},
    {0xdf, 0x60, 0xa0, 0x1a},
    {0x00, 0x00, 0x00, 0x00},
};
CVECTOR gJediColourCanon[JPB_JEDI_COLOUR_STORAGE_COUNT] = {
    {0xf8, 0x80, 0x38, 0x1b},
    {0x3c, 0xc0, 0x01, 0x00},
    {0xfe, 0x6e, 0xa7, 0x00},
    {0xf8, 0x80, 0x38, 0x1b},
    {0xf8, 0x80, 0x38, 0x1b},
    {0x10, 0x10, 0xc0, 0x01},
    {0x01, 0xf0, 0x58, 0x00},
    {0xf8, 0x80, 0x38, 0x1b},
    {0xf8, 0x80, 0x38, 0x1b},
    {0x00, 0x00, 0x00, 0x00},
};
CVECTOR gJediColourCurrent[JPB_JEDI_COLOUR_STORAGE_COUNT] = {
    {0xf8, 0x80, 0x38, 0x1b},
    {0x3c, 0xc0, 0x01, 0x00},
    {0xfe, 0x6e, 0xa7, 0x00},
    {0xf8, 0x80, 0x38, 0x1b},
    {0xf8, 0x80, 0x38, 0x1b},
    {0x10, 0x10, 0xc0, 0x01},
    {0x01, 0xf0, 0x58, 0x00},
    {0xf8, 0x80, 0x38, 0x1b},
    {0xf8, 0x80, 0x38, 0x1b},
    {0x00, 0x00, 0x00, 0x00},
};
int *gJediColorSprite = gJediColorSpriteCurrent;
CVECTOR *gJediColour = gJediColourCurrent;
uint64_t gJediColourArrayLength = JPB_JEDI_COLOUR_COUNT;

/* Exact initialized two-short records at matched-PC RVA 0x4BAFE0. */
int16_t versusPlayers[22][2] = {
    {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0},
    {6, 0}, {7, 0}, {8, 0}, {15, 0}, {17, 0}, {18, 0},
    {21, 0}, {26, 0}, {30, 0}, {36, 0}, {37, 0},
    {48, 0}, {49, 0}, {50, 0}, {51, 0}, {53, 0}
};

/* Exact initialized global combo tables, 48 records each. */
Combo combos1[48] = {
    {.Index = 45, .comboFlags = UINT32_C(0x210000),
     .String = "n", .kdmax = 20, .slack = 10, .prev = -1},
    {.Index = 46, .comboFlags = UINT32_C(0x210000),
     .String = "s", .prev = -1},
    {.Index = 44, .comboFlags = UINT32_C(0x210000),
     .String = "w", .prev = -1},
    {.Index = 70, .comboFlags = UINT32_C(0x0a0000),
     .String = "n.f"},
    {.Index = 58, .comboFlags = UINT32_C(0x020000),
     .String = "s.s", .kdmax = 18, .slack = 14, .prev = 1},
    {.Index = 57, .comboFlags = UINT32_C(0x020000),
     .String = "w.w", .kdmax = 12, .slack = 14, .prev = 2},
    {.Index = 45, .comboFlags = UINT32_C(0x020000),
     .String = "s.s.n", .prev = 4},
    {.Index = 62, .comboFlags = UINT32_C(0x020000),
     .String = "s.s.w", .kdmax = 20, .slack = 8, .prev = 4},
    {.Index = 50, .comboFlags = UINT32_C(0x020000),
     .String = "w.w.n", .kdmax = 14, .slack = 8, .prev = 5},
    {.Index = 62, .comboFlags = UINT32_C(0x020000),
     .String = "w.w.w", .kdmax = 16, .slack = 8, .prev = 5},
    {.Index = 51, .comboFlags = UINT32_C(0x020000),
     .String = "w.w.n.", .prev = 8},
    {.Index = 63, .comboFlags = UINT32_C(0x020000),
     .String = "s.s.n.w", .kdmax = 24, .slack = 14, .prev = 6},
    {.Index = 61, .comboFlags = UINT32_C(0x020000),
     .String = "s.s.w.w", .slack = 6, .prev = 7},
    {.Index = 54, .comboFlags = UINT32_C(0x020000),
     .String = "w.w.w.n", .kdmax = 24, .slack = 9, .prev = 9},
    {.Index = 48, .comboFlags = UINT32_C(0x0a0000),
     .String = "w.w.n..w", .prev = 10},
    {.Index = 57, .comboFlags = UINT32_C(0x020000),
     .String = "s.s.n.w.w", .prev = 11},
    {.Index = 56, .comboFlags = UINT32_C(0x0a0000),
     .String = "s.s.w.w.n", .prev = 12},
    {.Index = 67, .comboFlags = UINT32_C(0x0a0000),
     .String = "w.w.w.n.f", .prev = 13},
    {.Index = 53, .comboFlags = UINT32_C(0x0a0000),
     .String = "s.s.n.w.w.n", .prev = 15},
    {.Len = -1}
};

Combo combos2[48] = {
    {.Index = 45, .comboFlags = UINT32_C(0x210000),
     .String = "n", .kdmax = 20, .slack = 10, .prev = -1},
    {.Index = 46, .comboFlags = UINT32_C(0x210000),
     .String = "s", .prev = -1},
    {.Index = 44, .comboFlags = UINT32_C(0x210000),
     .String = "w", .prev = -1},
    {.Index = 70, .comboFlags = UINT32_C(0x0a0000),
     .String = "n.f"},
    {.Index = 58, .comboFlags = UINT32_C(0x020000),
     .String = "s.s", .kdmax = 18, .slack = 14, .prev = 1},
    {.Index = 57, .comboFlags = UINT32_C(0x020000),
     .String = "w.w", .kdmax = 12, .slack = 14, .prev = 2},
    {.Index = 45, .comboFlags = UINT32_C(0x020000),
     .String = "s.s.n", .prev = 4},
    {.Index = 62, .comboFlags = UINT32_C(0x020000),
     .String = "s.s.w", .kdmax = 20, .slack = 8, .prev = 4},
    {.Index = 50, .comboFlags = UINT32_C(0x020000),
     .String = "w.w.n", .kdmax = 14, .slack = 8, .prev = 5},
    {.Index = 62, .comboFlags = UINT32_C(0x020000),
     .String = "w.w.w", .kdmax = 16, .slack = 8, .prev = 5},
    {.Index = 51, .comboFlags = UINT32_C(0x020000),
     .String = "w.w.n.", .prev = 8},
    {.Index = 63, .comboFlags = UINT32_C(0x020000),
     .String = "s.s.n.w", .kdmax = 24, .slack = 14, .prev = 6},
    {.Index = 61, .comboFlags = UINT32_C(0x020000),
     .String = "s.s.w.w", .slack = 6, .prev = 7},
    {.Index = 54, .comboFlags = UINT32_C(0x020000),
     .String = "w.w.w.n", .kdmax = 24, .slack = 9, .prev = 9},
    {.Index = 48, .comboFlags = UINT32_C(0x0a0000),
     .String = "w.w.n..w", .prev = 10},
    {.Index = 57, .comboFlags = UINT32_C(0x020000),
     .String = "s.s.n.w.w", .prev = 11},
    {.Index = 56, .comboFlags = UINT32_C(0x0a0000),
     .String = "s.s.w.w.n", .prev = 12},
    {.Index = 67, .comboFlags = UINT32_C(0x0a0000),
     .String = "w.w.w.n.f", .prev = 13},
    {.Index = 53, .comboFlags = UINT32_C(0x0a0000),
     .String = "s.s.n.w.w.n", .prev = 15},
    {.Len = -1}
};

/* Exact module-local collision tables in linked address order. */
static CollisionData maDestroyeNodeSizes[8] = {
    {0x20, 0x00, -1}, {0x40, 0x03, -1},
    {0x40, 0x06, -1}, {0x40, 0x09, -1},
    {0x40, 0x0b, -1}, {0x40, 0x0c, -1},
    {0x40, 0x0e, -1}, {0x40, 0x12, -1}
};

static CollisionData maHumanNodeSizes[16] = {
    {0x20, 0x00, -1}, {0x00, 0x01, -1},
    {0x20, 0x02, -1}, {0x40, 0x03, -1},
    {0x00, 0x04, -1}, {0x20, 0x05, -1},
    {0x40, 0x06, -1}, {0x00, 0x07, -1},
    {0x20, 0x08, -1}, {0x00, 0x09, -1},
    {0x30, 0x0a, -1}, {0x60, 0x0b, -1},
    {0x10, 0x0c, -1}, {0x00, 0x0d, -1},
    {0x30, 0x0e, -1}, {0x60, 0x0f, -1}
};

static CollisionData maBigHumanNodeSizes[16] = {
    {0x20, 0x00, -1}, {0x00, 0x01, -1},
    {0x20, 0x02, -1}, {0x40, 0x03, -1},
    {0x00, 0x04, -1}, {0x20, 0x05, -1},
    {0x40, 0x06, -1}, {0x00, 0x07, -1},
    {0x20, 0x08, -1}, {0x00, 0x09, -1},
    {0x30, 0x0a, -1}, {0x80, 0x0b, -1},
    {0x10, 0x0c, -1}, {0x00, 0x0d, -1},
    {0x30, 0x0e, -1}, {0x80, 0x0f, -1}
};

static CollisionData maNodeSizes[19] = {
    {0x40, 0x00, -1}, {0x00, 0x01, -1},
    {0x20, 0x02, -1}, {0x20, 0x03, -1},
    {0x00, 0x04, -1}, {0x20, 0x05, -1},
    {0x20, 0x06, -1}, {0x00, 0x07, -1},
    {0x40, 0x08, -1}, {0x00, 0x09, -1},
    {0x30, 0x0a, -1}, {0x10, 0x0b, -1},
    {0x20, 0x0c, -1}, {0x00, 0x0d, -1},
    {0x30, 0x0e, -1}, {0x10, 0x0f, -1},
    {0x20, 0x11, 0x0c}, {0x80, 0x12, 0x0c},
    {0x80, 0x13, 0x0c}
};

static CollisionData maQuiMaceNodeSizes[19] = {
    {0x40, 0x00, -1}, {0x00, 0x01, -1},
    {0x20, 0x02, -1}, {0x20, 0x03, -1},
    {0x00, 0x04, -1}, {0x20, 0x05, -1},
    {0x20, 0x06, -1}, {0x00, 0x07, -1},
    {0x40, 0x08, -1}, {0x00, 0x09, -1},
    {0x30, 0x0a, -1}, {0x10, 0x0b, -1},
    {0x60, 0x0c, -1}, {0x00, 0x0d, -1},
    {0x30, 0x0e, -1}, {0x10, 0x0f, -1},
    {0x20, 0x14, 0x0c}, {0x80, 0x15, 0x0c},
    {0x80, 0x16, 0x0c}
};

static CollisionData maAdiNodeSizes[19] = {
    {0x40, 0x00, -1}, {0x00, 0x01, -1},
    {0x20, 0x02, -1}, {0x20, 0x03, -1},
    {0x00, 0x04, -1}, {0x20, 0x05, -1},
    {0x20, 0x06, -1}, {0x00, 0x07, -1},
    {0x40, 0x08, -1}, {0x00, 0x09, -1},
    {0x30, 0x0a, -1}, {0x10, 0x0b, -1},
    {0x20, 0x0c, -1}, {0x00, 0x0d, -1},
    {0x30, 0x0e, -1}, {0x10, 0x0f, -1},
    {0x20, 0x13, 0x0c}, {0x80, 0x14, 0x0c},
    {0x80, 0x15, 0x0c}
};

static CollisionData maMaul_DNodeSizes[22] = {
    {0x20, 0x00, -1}, {0x00, 0x01, -1},
    {0x10, 0x02, -1}, {0x20, 0x03, -1},
    {0x00, 0x04, -1}, {0x10, 0x05, -1},
    {0x20, 0x06, -1}, {0x00, 0x07, -1},
    {0x20, 0x08, -1}, {0x00, 0x09, -1},
    {0x18, 0x0a, -1}, {0x18, 0x0b, -1},
    {0x10, 0x0c, -1}, {0x00, 0x0d, -1},
    {0x18, 0x0e, -1}, {0x20, 0x0f, -1},
    {0x10, 0x11, 0x0c}, {0x20, 0x12, 0x0c},
    {0x20, 0x13, 0x0c}, {0x10, 0x13, 0x0c},
    {0x20, 0x16, 0x0c}, {0x20, 0x17, 0x0c}
};

static CollisionData maLoaderNodeSizes[9] = {
    {0x100, 0x00, -1}, {0x100, 0x01, -1},
    {0x080, 0x02, -1}, {0x100, 0x03, -1},
    {0x100, 0x04, -1}, {0x080, 0x05, -1},
    {0x100, 0x06, -1}, {0x100, 0x07, -1},
    {0x040, 0x0a, -1}
};

static VECTOR jediScale = {0x78a, 0x78a, 0x78a, 0};
static playerObject *mPlayer;

static void jedi_apply_player_settings(playerObject *player)
{
    player->pSettings.JumpVel = 0x7a;
    player->pSettings.RunningJumpVel = 0x73;
    player->pSettings.dblJumpVel = 0x73;
    player->pSettings.JumpAngle = 0x2f9;
    player->pSettings.RunningJumpAngle = 0x341;
    player->pSettings.dblJumpAngle = 0x35a;
    player->pSettings.bkJumpAngle = 0x555;
    player->pSettings.gravity = UINT16_C(0xe314);
    player->pSettings.dblgravity = 0;
    player->pSettings.minClosingDist = 0x14;
}

int jpb_jedi_ApplyPlayerSettings(
    playerObject *player)
{
    if (player == NULL) {
        return 0;
    }

    /*
     * Exact little-endian stores at RVAs 0xB3401..0xB342D:
     *   0073007a, 02f90073, 035a0341, e3140555, then 0014.
     */
    jedi_apply_player_settings(player);
    return 1;
}

/* 0xB10B0, 57 bytes, global, 2 named locals
 * CVECTOR_Equals
 * PDB type: int (CVECTOR, CVECTOR)
 * Source: W:\SWJediPowerBattles\Work\include\libgeom.h
 */

/* 0xB10F0, 99 bytes, global, 4 named locals
 * jedi_CalcBonusLevels
 * PDB type: void (int, int*, int*)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */

/* 0xB1160, 457 bytes, global, 7 named locals
 * jedi_CalcSkillLevels
 * PDB type: void (int, int*, int*)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
void jedi_CalcSkillLevels(
    int jedi_id, int *skill_percent, int *highest_level)
{
    int high = 0;
    int upgrade_total = 0;
    int level;

    if (skill_percent == NULL || highest_level == NULL) {
        return;
    }
    if (jedi_id < 0 ||
        jedi_id >= JPB_GAME_JEDI_MODEL_CAPACITY) {
        *skill_percent = 0;
        *highest_level = 0;
        return;
    }
    for (level = 1; level <= 10; ++level) {
        int upgrade;

        if (GameStruct.jediLevelPlayed[jedi_id][level] != 0) {
            high = level;
        }
        upgrade = 0;
        if (jedi_id < 9) {
            upgrade = jediUpgrades[jedi_id].awardData[level];
            if (upgrade > 3) {
                upgrade = 3;
            }
        }
        upgrade_total += upgrade;
    }
    *highest_level = high;
    *skill_percent = (upgrade_total * 100) / 30;
}

/* 0xB1330, 29 bytes, global, 1 named locals
 * jedi_CanToggleSaber
 * PDB type: int (model_id)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int jedi_CanToggleSaber(model_id cnum)
{
    return cnum == mace_model ||
           cnum == adi_model ||
           cnum == plo_model ||
           cnum == ki_adi_model;
}

/* 0xB1350, 225 bytes, global, 3 named locals
 * jedi_CheckValidLevel
 * PDB type: int (int, int*)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */

int jedi_CheckValidLevel(int level, int *upgrade_level)
{
    int upgrade;

    if (GameStruct.ModelSelect[0] == 17 ||
        GameStruct.ModelSelect[1] == 17) {
        *upgrade_level = 0;
        return level < 15;
    }
    if (level < 11) {
        upgrade = jediUpgrades[
            (uint16_t)GameStruct.ModelSelect[0]].awardData[level];
        if (GameStruct.NumPlayers == 2) {
            int player_two_upgrade = jediUpgrades[
                (uint16_t)GameStruct.ModelSelect[1]].awardData[level];

            if (upgrade <= player_two_upgrade) {
                upgrade = player_two_upgrade;
            }
        }
        if (upgrade > 3) {
            upgrade = 3;
        }
        *upgrade_level = upgrade;
        return 1;
    }
    if (level == 11) {
        return (secretBits & UINT32_C(0x1)) != 0;
    }
    if (level == 12) {
        return (secretBits & UINT32_C(0x2)) != 0;
    }
    if (level == 13) {
        return (secretBits & UINT32_C(0x4)) != 0;
    }
    if (level == 14) {
        return (secretBits & UINT32_C(0x8)) != 0 &&
               GameStruct.NumPlayers == 1;
    }
    return 0;
}

/* 0xB1440, 88 bytes, global, 1 named locals
 * jedi_CheckValidPlayer
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int jedi_CheckValidPlayer(int jediID)
{
    if (jediID <= plo_model) {
        return 1;
    }
    switch (jediID) {
    case maul_p_model:
        return (secretBits & UINT32_C(0x10)) != 0;
    case amidala_model:
        return (secretBits & UINT32_C(0x20)) != 0;
    case panaka_model:
        return (secretBits & UINT32_C(0x40)) != 0;
    case ki_adi_model:
        return (secretBits & UINT32_C(0x200)) != 0;
    case battle_d_model:
        return (secretBits & UINT32_C(0x400)) != 0;
    default:
        return 0;
    }
}

/* 0xB14A0, 73 bytes, global, 2 named locals
 * jedi_CheckValidPlayerNGP
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int jedi_CheckValidPlayerNGP(int jediID)
{
    static const uint64_t valid_player_mask =
        UINT64_C(0x002f003004248000);
    uint32_t id = (uint32_t)jediID;

    if ((id <= 0x35u &&
         ((valid_player_mask >> (id & 63u)) & UINT64_C(1)) != 0) ||
        id == (uint32_t)jar_jar_playable_model) {
        return 1;
    }
    if (id == (uint32_t)loader_model) {
        ExtraCharacter *extra = GetCharacterByID(loader_model);

        return extra != NULL && extra->Unlocked == 1;
    }
    return 0;
}

/* 0xB14F0, 248 bytes, global, 3 named locals
 * jedi_CheckValidPlayerWTabs
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int jedi_CheckValidPlayerWTabs(int selectType, int jediID)
{
    if (selectType == 1) {
        switch (jediID) {
        case pilot_model:
        case rifle_model:
        case flame_model:
        case destroye_model:
        case tusken_s_model:
        case tusken_r_model:
        case thug_1_model:
        case thug_2_model:
        case thug_3_model:
        case thug_4_model:
        case gungan_1_model:
        case jar_jar_playable_model:
            return 1;
        case loader_model: {
            ExtraCharacter *extra = GetCharacterByID(loader_model);

            return extra != NULL && extra->Unlocked == 1;
        }
        default:
            return 0;
        }
    }
    return jedi_CheckValidPlayer(jediID);
}

/* 0xB15F0, 297 bytes, global, 4 named locals
 * jedi_CheckValidVersus
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int jedi_CheckValidVersus(int jediID)
{
    int high = 0;
    int player;
    int level;
    size_t index;

    for (player = 0; player < 5; ++player) {
        for (level = 1; level <= 10; ++level) {
            if (GameStruct.jediLevelPlayed[player][level] != 0 &&
                high < level) {
                high = level;
            }
        }
    }
    for (index = 0;
         index < sizeof(versusPlayers) / sizeof(versusPlayers[0]);
         ++index) {
        if (versusPlayers[index][0] == jediID &&
            high + 1 > versusPlayers[index][1]) {
            return 1;
        }
    }
    return 0;
}

/* 0xB1720, 356 bytes, global, 1 named locals
 * jedi_ConvertToTextIndex
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int jedi_ConvertToTextIndex(int jedi_id)
{
    switch (jedi_id) {
    case 0: case 1: case 2: case 3: case 4:
    case 5: case 6: case 7: case 8:
        return jedi_id;
    case 0x0f: return 9;
    case 0x11: return 10;
    case 0x12: return 11;
    case 0x15: return 12;
    case 0x1a: return 14;
    case 0x1e: return 15;
    case 0x24: return 16;
    case 0x25: return 17;
    case 0x30: return 19;
    case 0x31: return 20;
    case 0x32: return 21;
    case 0x33: return 22;
    case 0x35: return 24;
    case 0x4f: return 25;
    default: return -1;
    }
}

/* 0xB1890, 614 bytes, global, 8 named locals
 * jedi_DrawBlur
 * PDB type: void (VECTOR*, _svector*, _svect...
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
void jedi_DrawBlur(
    VECTOR *p1,
    _svector *v1,
    _svector *p2,
    _svector *v2,
    uint32_t color)
{
    _svector previous_start;
    _svector previous_end;

    if (p1 == NULL || v1 == NULL ||
        p2 == NULL || v2 == NULL) {
        return;
    }

    /*
     * The matched routine emits the four-point saber motion quad. The
     * portable immediate renderer consumes the same two time samples as a
     * broad glow segment until its textured quad backend is recovered.
     */
    previous_start.vx = (int16_t)(p1->vx - v1->vx);
    previous_start.vy = (int16_t)(p1->vy - v1->vy);
    previous_start.vz = (int16_t)(p1->vz - v1->vz);
    previous_start.pad = 0;
    previous_end.vx = (int16_t)(p2->vx - v2->vx);
    previous_end.vy = (int16_t)(p2->vy - v2->vy);
    previous_end.vz = (int16_t)(p2->vz - v2->vz);
    previous_end.pad = 0;
    fx_screenGlow(
        &previous_start, &previous_end, 8, color);
}

/* 0xB1B00, 736 bytes, global, 15 named locals
 * jedi_FireWeapon
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */

/* 0xB1DE0, 1125 bytes, global, 15 named locals
 * jedi_GetAwardFlags
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */

/* 0xB2250, 23 bytes, global, 1 named locals
 * jedi_GetColorSprite
 * PDB type: int (unsigned __int64)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int jedi_GetColorSprite(uint64_t player_id)
{
    if (player_id >= JPB_JEDI_COLOUR_COUNT) {
        return -1;
    }
    return gJediColorSprite[player_id];
}

/* 0xB2270, 23 bytes, global, 1 named locals
 * jedi_GetColour
 * PDB type: CVECTOR (unsigned __int64)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
static void jedi_fire_projectile_from_nodes(
    playerObject *player,
    physicsObject *physics,
    int projectile_type,
    int muzzle_node,
    int aim_node)
{
    Projectile *proj =
        bullet_AllocProjectile(projectile_type);
    VECTOR *pos0;
    VECTOR *pos1;
    playerObject *target;
    VECTOR target_position;

    if (proj == NULL) {
        return;
    }
    pos0 = coll_GetNodeCenter(
        player->playernum, muzzle_node);
    target = FindBestMachineGunTarget(
        &physics->vpos,
        &physics->angle,
        player,
        0x12,
        0x100,
        0x0a00,
        GameStruct.versusModeFlag);
    if (target == NULL) {
        pos1 = coll_GetNodeCenter(
            player->playernum, aim_node);
        proj->pj_Flags |= (int32_t)UINT32_C(0x10);
    } else {
        player->target = target;
        pos1 = coll_GetNodeCenter(target->playernum, 0);
    }
    target_position.vx = pos1->vx;
    target_position.vy = pos1->vy;
    target_position.vz = pos1->vz;
    target_position.pad = 0;
    bullet_ShootProjectile(
        proj, player, pos0, &target_position, NULL);
}

int jedi_FireWeapon(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    modelObject *model =
        (modelObject *)scene->pModel;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    uint32_t events = model->eventMask;
    int powered_model =
        (model->flags & UINT32_C(4)) != 0;
    int projectile_type;
    int muzzle_node;
    int aim_node;
    int second_muzzle_node = 0;
    int second_aim_node = 0;

    if (powered_model) {
        events = 1;
    }
    if (player->playerID == 0x15 ||
        player->playerID == 0x16) {
        (void)force_FlameCallBack(cpad, player);
        return 0;
    }
    if (events == 0) {
        return powered_model;
    }

    projectile_type = (int)(*player->pMotion)->fx2;
    if (gGlobalTimer <
        (uint32_t)game_gGetPowerLevel(player->playernum)) {
        projectile_type =
            game_gGetPowerType(player->playernum) == 10
                ? 4
                : 0x11;
    }

    /* These are character/model IDs at playerObject+0x8A, not motions. */
    if (player->playerID == 0x1a ||
        player->playerID == 0x1c) {
        muzzle_node = 0x0c;
        aim_node = 0x1a;
        second_muzzle_node = 0x0b;
        second_aim_node = 0x1b;
    } else if (player->playerID == 0x11) {
        muzzle_node = 0x0b;
        aim_node = 0x0c;
    } else if (player->playerID == 0x12) {
        muzzle_node = 0x0c;
        aim_node = 0x13;
    } else {
        muzzle_node = 0x0c;
        aim_node =
            player->playerID == 0x30
                ? 0x12
                : 0x11;
    }

    jedi_fire_projectile_from_nodes(
        player,
        physics,
        projectile_type,
        muzzle_node,
        aim_node);
    if (second_muzzle_node != 0) {
        jedi_fire_projectile_from_nodes(
            player,
            physics,
            projectile_type,
            second_muzzle_node,
            second_aim_node);
    }
    return powered_model;
}
CVECTOR jedi_GetColour(uint64_t playerID)
{
    static const CVECTOR white = {
        UINT8_C(0xff),
        UINT8_C(0xff),
        UINT8_C(0xff),
        UINT8_C(0)
    };

    if (playerID < JPB_JEDI_COLOUR_COUNT) {
        return gJediColour[playerID];
    }
    return white;
}

/* 0xB2290, 23 bytes, global, 1 named locals
 * jedi_GetColour32
 * PDB type: unsigned (unsigned __int64)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
uint32_t jedi_GetColour32(uint64_t playerID)
{
    if (playerID < JPB_JEDI_COLOUR_COUNT) {
        const CVECTOR colour = gJediColour[playerID];

        return (uint32_t)colour.r |
               (uint32_t)colour.g << 8 |
               (uint32_t)colour.b << 16 |
               (uint32_t)colour.cd << 24;
    }
    return UINT32_C(0x00ffffff);
}

/* 0xB22B0, 224 bytes, global, 2 named locals
 * jedi_GetHighestLevel
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */

/* 0xB2390, 90 bytes, global, 2 named locals
 * jedi_GetLives
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */

/* 0xB23F0, 3440 bytes, global, 44 named locals
 * jedi_HandleSabre
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
static uint8_t jedi_sabre_counter;
static int jedi_sabre_spin;
static int jedi_sabre_spin1 = 0x20;
static int jedi_sabre_spin2 = 0x40;
static int jedi_sabre_dir = 1;
static int jedi_sabre_dir2 = 1;
static int jedi_sabre_height = 2;

static int16_t jedi_sabre_scaled_component(
    int component, int scale)
{
    int product = component * scale;

    return (int16_t)(
        (product + ((product >> 31) & 0xfff)) >> 12);
}

static uint32_t jedi_sabre_color(int16_t player_id)
{
    uint64_t color_index = (uint16_t)player_id;

    if (player_id == 9 || player_id == 0x2b) {
        color_index = 5;
    } else if ((uint16_t)(player_id - 0x50) < 9U) {
        color_index = (uint16_t)(player_id - 0x50);
    }
    return jedi_GetColour32(color_index) & UINT32_C(0x00ffffff);
}

static void jedi_sabre_node_ids(
    int16_t player_id,
    unsigned *base_id,
    unsigned *tip_id,
    unsigned *second_base_id,
    unsigned *second_tip_id)
{
    *second_base_id = 0;
    *second_tip_id = 0;
    if (player_id == 1 || player_id == 2 ||
        player_id == 0x51 || player_id == 0x52) {
        *base_id = 0x14;
        *tip_id = 0x16;
    } else if (player_id == 5 || player_id == 0x2b) {
        *base_id = 0x12;
        *tip_id = 0x13;
        *second_base_id = 0x17;
        *second_tip_id = 0x13;
    } else if (player_id == 3 || player_id == 0x53) {
        *base_id = 0x13;
        *tip_id = 0x15;
    } else {
        *base_id = 0x11;
        *tip_id = 0x13;
    }
}

static int jedi_sabre_endpoints(
    Mnode *base,
    Mnode *tip,
    int scale,
    int direction_sign,
    int inner_scale,
    _svector *outer,
    _svector *inner)
{
    _svector direction;

    if (base == NULL || tip == NULL) {
        return 0;
    }
    (void)normalize(
        base->v3RotCenter.vx - tip->v3RotCenter.vx,
        base->v3RotCenter.vy - tip->v3RotCenter.vy,
        base->v3RotCenter.vz - tip->v3RotCenter.vz,
        &direction);
    if (direction_sign > 0) {
        outer->vx = (int16_t)(
            base->v3RotCenter.vx +
            jedi_sabre_scaled_component(direction.vx, scale));
        outer->vy = (int16_t)(
            base->v3RotCenter.vy +
            jedi_sabre_scaled_component(direction.vy, scale));
        outer->vz = (int16_t)(
            base->v3RotCenter.vz +
            jedi_sabre_scaled_component(direction.vz, scale));
        inner->vx = (int16_t)(
            base->v3RotCenter.vx +
            jedi_sabre_scaled_component(direction.vx, inner_scale));
        inner->vy = (int16_t)(
            base->v3RotCenter.vy +
            jedi_sabre_scaled_component(direction.vy, inner_scale));
        inner->vz = (int16_t)(
            base->v3RotCenter.vz +
            jedi_sabre_scaled_component(direction.vz, inner_scale));
    } else {
        outer->vx = (int16_t)(
            base->v3RotCenter.vx -
            jedi_sabre_scaled_component(direction.vx, scale));
        outer->vy = (int16_t)(
            base->v3RotCenter.vy -
            jedi_sabre_scaled_component(direction.vy, scale));
        outer->vz = (int16_t)(
            base->v3RotCenter.vz -
            jedi_sabre_scaled_component(direction.vz, scale));
        inner->vx = (int16_t)base->v3RotCenter.vx;
        inner->vy = (int16_t)base->v3RotCenter.vy;
        inner->vz = (int16_t)base->v3RotCenter.vz;
    }
    outer->pad = 0;
    inner->pad = 0;
    return 1;
}

static void jedi_draw_sabre_blade(
    playerObject *player,
    Mnode *base,
    Mnode *tip,
    int second_blade,
    uint32_t color)
{
    _svector outer;
    _svector inner;
    uint32_t blade_color = color | UINT32_C(0x7f000000);

    if (!jedi_sabre_endpoints(
            base,
            tip,
            0x70,
            second_blade ? 1 : -1,
            second_blade ? 0x20 : 0,
            &outer,
            &inner)) {
        return;
    }
    fx_screenGlow(
        &outer,
        &inner,
        rand() % 6 + 0xe,
        blade_color);
    fx_screenGlow(
        &outer, &inner, 2, UINT32_C(0xffffffff));
    if (player->subOffset != 0 && player->pMotion != NULL &&
        *player->pMotion != NULL &&
        (*player->pMotion)->Damage > 1) {
        jedi_DrawBlur(
            &base->v3RotCenter,
            &base->v3Velocity,
            &outer,
            &tip->v3Velocity,
            blade_color);
    }
}

static void jedi_check_sabre_world_contact(
    playerObject *player, Mnode *tip)
{
    _mvector swing;
    VECTOR last;

    if (tip == NULL || player->pMotion == NULL ||
        *player->pMotion == NULL ||
        (*player->pMotion)->Damage <= 1) {
        return;
    }
    swing.speed = (int16_t)normalize(
        tip->v3Velocity.vx,
        tip->v3Velocity.vy,
        tip->v3Velocity.vz,
        (_svector *)(void *)&swing);
    last.vx = tip->v3RotCenter.vx - tip->v3Velocity.vx;
    last.vy = tip->v3RotCenter.vy - tip->v3Velocity.vy;
    last.vz = tip->v3RotCenter.vz - tip->v3Velocity.vz;
    last.pad = 0;
    if (MoveObject(&swing, &last, SMALL_HIT) >= 0) {
        feedback_startEffect(player->playernum, 4);
    }
}

static void jedi_draw_power_sabre(
    Mnode *base,
    Mnode *tip,
    uint32_t color)
{
    _svector start;
    _svector end;
    _svector normal;
    _svector rotation;
    float radius;

    if (base == NULL || tip == NULL) {
        return;
    }
    start.vx = (int16_t)base->v3RotCenter.vx;
    start.vy = (int16_t)base->v3RotCenter.vy;
    start.vz = (int16_t)base->v3RotCenter.vz;
    start.pad = 0;
    end.vx = (int16_t)tip->v3RotCenter.vx;
    end.vy = (int16_t)tip->v3RotCenter.vy;
    end.vz = (int16_t)tip->v3RotCenter.vz;
    end.pad = 0;
    fx_screenGlow(
        &start, &end, 0x10,
        color | UINT32_C(0xff000000));
    (void)normalize(
        base->v3RotCenter.vx - tip->v3RotCenter.vx,
        base->v3RotCenter.vy - tip->v3RotCenter.vy,
        base->v3RotCenter.vz - tip->v3RotCenter.vz,
        &normal);
    (void)vec_RotFromNormalS(&rotation, &normal);
    rotation.vx = (int16_t)(rotation.vx - 0x400);
    jedi_sabre_height += jedi_sabre_dir2 / 2;
    if (abs(jedi_sabre_height) > 0x10) {
        jedi_sabre_dir2 = -jedi_sabre_dir2;
    }
    if (rand() % 500 < 10) {
        jedi_sabre_dir = -jedi_sabre_dir;
    }
    jedi_sabre_spin += (jedi_sabre_dir * 8) / 2;
    jedi_sabre_spin1 += (jedi_sabre_dir * 8) / 2;
    jedi_sabre_spin2 += (jedi_sabre_dir * 8) / 2;
    if (jedi_sabre_spin > 0x60) jedi_sabre_spin = 0;
    if (jedi_sabre_spin1 > 0x60) jedi_sabre_spin1 = 0;
    if (jedi_sabre_spin2 > 0x60) jedi_sabre_spin2 = 0;
    if (jedi_sabre_spin < 0) jedi_sabre_spin = 0x60;
    if (jedi_sabre_spin1 < 0) jedi_sabre_spin1 = 0x60;
    if (jedi_sabre_spin2 < 0) jedi_sabre_spin2 = 0x60;

    radius = (float)(jedi_sabre_height + 0x18);
    drawCylinder(
        &base->v3RotCenter,
        &rotation,
        radius,
        radius,
        (float)jedi_sabre_spin,
        (float)(jedi_sabre_spin + 8),
        color | UINT32_C(0x7f000000),
        UINT32_C(0x800),
        0,
        0,
        2);
    drawCylinder(
        &base->v3RotCenter,
        &rotation,
        radius,
        radius,
        (float)jedi_sabre_spin1,
        (float)(jedi_sabre_spin1 + 8),
        color | UINT32_C(0x7f000000),
        UINT32_C(0x800),
        0,
        0,
        2);
    drawCylinder(
        &base->v3RotCenter,
        &rotation,
        radius,
        radius,
        (float)jedi_sabre_spin2,
        (float)(jedi_sabre_spin2 + 8),
        color | UINT32_C(0x7f000000),
        UINT32_C(0x800),
        0,
        0,
        2);
}

static void jedi_draw_long_sabre(
    playerObject *player,
    Mnode *base,
    Mnode *tip,
    uint32_t color)
{
    _svector outer;
    _svector inner;
    uint32_t blade_color = color | UINT32_C(0x7f000000);

    if (!jedi_sabre_endpoints(
            base, tip, 0xc4, -1, 0, &outer, &inner)) {
        return;
    }
    fx_screenGlow(
        &outer, &inner, (rand() & 7) + 0x18, blade_color);
    fx_screenGlow(
        &outer, &inner, 6, UINT32_C(0xffffffff));
    if (player->subOffset != 0 && player->pMotion != NULL &&
        *player->pMotion != NULL &&
        (*player->pMotion)->Damage > 1) {
        jedi_DrawBlur(
            &base->v3RotCenter,
            &base->v3Velocity,
            &outer,
            &tip->v3Velocity,
            blade_color);
    }
}

int jedi_HandleSabre(
    int32_t *cpad, playerObject *player)
{
    unsigned base_id;
    unsigned tip_id;
    unsigned second_base_id;
    unsigned second_tip_id;
    Mnode *base;
    Mnode *tip;
    uint32_t color;
    int power_type;
    int power_level;
    int long_saber = 0;
    int power_saber = 0;

    (void)cpad;
    if (player == NULL) {
        return 0;
    }
    if (player->playerID == 6 || player->playerID == 7) {
        return 1;
    }
    color = jedi_sabre_color(player->playerID);
    ++jedi_sabre_counter;
    if ((jedi_sabre_counter & UINT8_C(0x1f)) == 0 &&
        rand() % 100 < 0x19 &&
        (GameStruct.GameState & UINT32_C(0x02000000)) == 0) {
        (void)sound_Play(
            physics_gGetPosition(&player->playerRoot),
            0,
            "lshum",
            0);
    }
    if (player->playernum < 2) {
        power_type = game_gGetPowerType(player->playernum);
        power_level = game_gGetPowerLevel(player->playernum);
        long_saber =
            power_type == 10 &&
            gGlobalTimer < (uint32_t)power_level;
        power_saber =
            power_type == 9 &&
            gGlobalTimer < (uint32_t)power_level;
    }

    jedi_sabre_node_ids(
        player->playerID,
        &base_id,
        &tip_id,
        &second_base_id,
        &second_tip_id);
    base = coll_GetNode(player->playernum, base_id);
    tip = coll_GetNode(player->playernum, tip_id);
    if (base == NULL || tip == NULL) {
        return 1;
    }

    if (!long_saber) {
        jedi_draw_sabre_blade(player, base, tip, 0, color);
        if (second_base_id != 0 || second_tip_id != 0) {
            Mnode *second_base = coll_GetNode(
                player->playernum, second_base_id);
            Mnode *second_tip = coll_GetNode(
                player->playernum, second_tip_id);

            jedi_draw_sabre_blade(
                player,
                second_base,
                second_tip,
                1,
                color);
        }
    }
    jedi_check_sabre_world_contact(player, tip);
    if (long_saber) {
        jedi_draw_long_sabre(player, base, tip, color);
        tip->time = 0x16c;
    } else {
        tip->time = 0xd0;
    }
    if (power_saber) {
        jedi_draw_power_sabre(base, tip, color);
        if (second_base_id != 0 || second_tip_id != 0) {
            jedi_draw_power_sabre(
                coll_GetNode(player->playernum, second_base_id),
                coll_GetNode(player->playernum, second_tip_id),
                color);
        }
    }
    return 1;
}

/* 0xB3160, 80 bytes, global, 2 named locals
 * jedi_HasProgression
 * PDB type: int (model_id)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int jedi_HasProgression(model_id cnum)
{
    switch (cnum) {
    case amidala_model:
    case panaka_model:
    case battle_d_model:
    case pilot_model:
    case rifle_model:
    case flame_model:
    case destroye_model:
    case tusken_s_model:
    case tusken_r_model:
    case thug_1_model:
    case thug_2_model:
    case thug_3_model:
    case thug_4_model:
    case gungan_1_model:
    case jar_jar_playable_model:
        return 0;
    case loader_model: {
        ExtraCharacter *character = GetCharacterByID(cnum);

        if (character != NULL && character->Unlocked == 1) {
            return 0;
        }
        return 1;
    }
    default:
        return 1;
    }
}

/* 0xB31B0, 105 bytes, global, 2 named locals
 * jedi_InitLives
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
void jedi_InitLives(void)
{
    int life_upgrades =
        jediUpgrades[(uint16_t)GameStruct.ModelSelect[0]].lifeUpgrades;

    if (GameStruct.NumPlayers == 2) {
        int player_two_lives =
            jediUpgrades[(uint16_t)GameStruct.ModelSelect[1]].lifeUpgrades;

        if (life_upgrades <= player_two_lives) {
            life_upgrades = player_two_lives;
        }
    }
    if (life_upgrades > 4) {
        life_upgrades = 4;
    }
    GameStruct.mNumContinues = life_upgrades + 1000;
    GameStruct.aCharacterData[0].Items = 0;
    GameStruct.aCharacterData[1].Items = 0;
}

/* 0xB3220, 559 bytes, global, 2 named locals
 * jedi_InitPlayer
 * PDB type: int (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int jedi_InitPlayer(playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    modelObject *model =
        (modelObject *)scene->pModel;
    CollisionData *node_sizes;
    int node_count;

    mPlayer = player;
    model->v3Scale = jediScale;
    player->fScale = jediScale.vx;
    player->paCombos =
        player->playernum == 0 ? combos1 : combos2;

    switch (player->playerID) {
    case 1:
    case 2:
        node_sizes = maQuiMaceNodeSizes;
        node_count = 19;
        break;
    case 3:
        node_sizes = maAdiNodeSizes;
        node_count = 19;
        break;
    case 5:
        node_sizes = maMaul_DNodeSizes;
        node_count = 22;
        break;
    case 6:
    case 7:
        node_sizes = maHumanNodeSizes;
        node_count = 16;
        break;
    case 10:
    case 0x35:
    case 0x36:
    case 0x4f:
        node_sizes = maGunganNodeSizes;
        node_count = 19;
        break;
    case 0x1a:
    case 0x1c:
        node_sizes = maDestroyeNodeSizes;
        node_count = 8;
        break;
    case 0x1e:
        model->flags |= UINT32_C(0x00000008);
        model->v3Scale.vx = 0x78a;
        model->v3Scale.vy = 0x78a;
        model->v3Scale.vz = 0x78a;
        node_sizes = maLoaderNodeSizes;
        node_count = 9;
        break;
    case 0x33:
        model->v3Scale.vx = 0x9cd;
        model->v3Scale.vy = 0x9cd;
        model->v3Scale.vz = 0x9cd;
        node_sizes = maBigHumanNodeSizes;
        node_count = 16;
        break;
    default:
        node_sizes = maNodeSizes;
        node_count = 19;
        break;
    }
    player->paNodesSizes = node_sizes;
    player->numCollisionNodes = node_count;
    if (player->playerID == 5) {
        jediUpgrades[5].forcePowers = (int16_t)UINT16_C(0xf800);
    }
    player->pMainCallBack = jedi_Main;
    jedi_apply_player_settings(player);

    /* The executable leaves EAX unspecified despite the PDB int type. */
    return 0;
}

/* 0xB3450, 121 bytes, global, 1 named locals
 * jedi_IsMelee
 * PDB type: int (model_id)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int jedi_IsMelee(model_id cnum)
{
    switch (cnum) {
    case pilot_model:
    case loader_model:
    case tusken_s_model:
    case thug_2_model:
    case thug_4_model:
    case gungan_1_model:
    case jar_jar_playable_model:
        return 1;
    default:
        return 0;
    }
}

/* 0xB34D0, 263 bytes, global, 3 named locals
 * jedi_Main
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
static void jedi_clear_force_effect(
    playerObject *player, Mnode *node)
{
    node->v3Velocity2.vx = 0;
    node->v3Velocity2.vy = 0;
    node->v3Velocity2.vz = 0;
    node->v3Translation2.vx = 0;
    node->v3Translation2.vy = 0;
    node->v3Translation2.vz = 0;
    player->forceFlags &= UINT32_C(0xffffffed);
    player->pFlags &= UINT32_C(0xffffdfff);
    node->flags &= UINT32_C(0xfbffffdf);
    player->forceData[0] = 0;
}

int jedi_Main(int32_t *cpad, playerObject *player)
{
    Mnode *node = coll_GetNode(player->playernum, 12);
    int clear_effect = 0;

    (void)cpad;
    if (player->playerID == 2 &&
        player->currentMotion != 0x60 &&
        player->currentMotion != 0x66 &&
        player->currentMotion != 0x7f &&
        (node->flags & UINT32_C(0x04000000)) != 0) {
        clear_effect = 1;
    }
    if (player->playerID == 8 &&
        player->currentMotion != 0x8d &&
        (node->flags & UINT32_C(0x04000000)) != 0) {
        clear_effect = 1;
    }
    if (clear_effect) {
        jedi_clear_force_effect(player, node);
    }
    return 0;
}

/* 0xB35E0, 506 bytes, global, 3 named locals
 * jedi_SetHighestLevel
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */

/* 0xB37E0, 2806 bytes, global, 10 named locals
 * jedi_ShowCombos
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */

/* 0xB42E0, 441 bytes, global, 1 named locals
 * jedi_ShowSecrets
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */

/* 0xB44A0, 24 bytes, global, 3 named locals
 * jedi_ShowStats
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */

/* 0xB44C0, 145 bytes, global, 1 named locals
 * jedi_ToggleSaberColor
 * PDB type: void (model_id)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
void jedi_ToggleSaberColor(model_id cnum)
{
    uint64_t index = (uint32_t)cnum;
    CVECTOR current;
    CVECTOR legacy;

    if (index >= gJediColourArrayLength) {
        return;
    }
    current = gJediColourCurrent[index];
    legacy = gJediColourLegacy[index];
    if (current.r == legacy.r &&
        current.g == legacy.g &&
        current.b == legacy.b &&
        current.cd == legacy.cd) {
        gJediColourCurrent[index] = gJediColourCanon[index];
        gJediColorSpriteCurrent[index] = gJediColorSpriteCanon[index];
        return;
    }
    gJediColorSpriteCurrent[index] = gJediColorSpriteLegacy[index];
    gJediColourCurrent[index] = legacy;
}

/* 0xB4560, 92 bytes, global, 3 named locals
 * tusken_stab
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\jedi.c
 */
int tusken_stab(int32_t *cpad, playerObject *player)
{
    int frame = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);

    (void)cpad;
    if (frame > 11) {
        coll_ClrNodeFlags(
            player->playernum,
            12,
            UINT32_C(0x40000000));
        return 1;
    }
    if (frame > 9) {
        coll_SetNodeFlags(
            player->playernum,
            12,
            UINT32_C(0x40000000));
    }
    return 0;
}
