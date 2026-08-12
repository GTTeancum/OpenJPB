/*
 * REVIEWED RECONSTRUCTION of force_gActivate from
 * W:\SWJediPowerBattles\Work\force.c.
 *
 * Exact PDB names/types are retained. Control flow, ForceMap selection,
 * power gates, callback gates, and feedback calls were checked against the
 * x64 instructions at RVAs 0xA2B20..0xA2CA5. force_PlaySeq remains an exact
 * named dependency and this separate object keeps it link-isolated.
 */

#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/input.h"

#include <stdint.h>

/* Exact initialized PDB global at matched-PC RVA 0x4BA540. */
ForceMap mapData[23] = {
    {{{{-40, 0, 0, 0}, 6}, {{65, 0, 0, 0}, 0},
      {{-75, -76, -77, 0}, 3}, {{66, 0, 0, 0}, 4}}},
    {{{{67, 0, 0, 0}, 16}, {{65, 0, 0, 0}, 0},
      {{-37, 0, 0, 0}, 9}, {{66, 0, 0, 0}, 4}}},
    {{{{-37, 0, 0, 0}, 15}, {{65, 0, 0, 0}, 0},
      {{-40, -41, -42, 0}, 2}, {{-43, 0, 0, 0}, 10}}},
    {{{{-42, 0, 0, 0}, 11}, {{74, 75, 80, 0}, 8},
      {{-58, -59, -60, 0}, 14}, {{67, 0, 0, 0}, 5}}},
    {{{{-41, -78, -79, 0}, 7}, {{-70, -71, -72, 0}, 13},
      {{-70, -71, -72, 0}, 3}, {{-40, 0, 0, 0}, 12}}},
    {{{{67, 0, 0, 0}, 16}, {{65, 0, 0, 0}, 0},
      {{71, 72, 73, 0}, 3}, {{-66, 0, 0, 0}, 12}}},
    {{{{65, 0, 0, 0}, 0}, {{65, 0, 0, 0}, 0},
      {{65, 0, 0, 0}, 0}, {{65, 0, 0, 0}, 0}}},
    {{{{65, 0, 0, 0}, 0}, {{65, 0, 0, 0}, 0},
      {{65, 0, 0, 0}, 0}, {{65, 0, 0, 0}, 0}}},
    {{{{-42, 0, 0, 0}, 11}, {{65, 0, 0, 0}, 0},
      {{-58, -59, -60, 0}, 14}, {{141, 0, 0, 0}, 10}}},
};

/* 0xA2B20, 390 bytes, global, 5 named locals
 * force_gActivate
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\force.c
 */
int force_gActivate(
    int32_t *cpad, playerObject *player)
{
    int i = 0;
    int force =
        game_gGetForce(player->playernum);
    ForceMap *map =
        player->playerID < 9
            ? &mapData[player->playerID]
            : &mapData[7];

    if (player->pForceCallBack != NULL) {
        return 0;
    }
    if (player->pMotionCallBack != NULL) {
        return 1;
    }

    if (((uint32_t)cpad[0] & UINT32_C(0x80)) != 0) {
        if (game_gGetItemCount(player->playernum) < 1) {
            return 0;
        }
        i = force_PlaySeq(&map->slot[1], player);
    }
    if (force <= 4) {
        return 0;
    }
    if (((uint32_t)cpad[0] & UINT32_C(0x10)) != 0) {
        i = force_PlaySeq(&map->slot[0], player);
        feedback_startEffect(player->playernum, 13);
    }
    if (((uint32_t)cpad[0] & UINT32_C(0x20)) != 0 &&
        ((jediUpgrades[player->playerID].forcePowers &
          INT16_C(0x2000)) != 0 ||
         GameStruct.ForceLevel > 8)) {
        i = force_PlaySeq(&map->slot[2], player);
        feedback_startEffect(player->playernum, 13);
    }
    if (((uint32_t)cpad[0] & UINT32_C(0x40)) != 0 &&
        ((jediUpgrades[player->playerID].forcePowers &
          INT16_C(0x4000)) != 0 ||
         GameStruct.ForceLevel > 8)) {
        i = force_PlaySeq(&map->slot[3], player);
        feedback_startEffect(player->playernum, 13);
    }
    return i;
}
