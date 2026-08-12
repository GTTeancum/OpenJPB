/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\Work\combo.c.
 *
 * All eight emitted procedures were checked against the raw Ghidra export.
 * PDB names, signatures, player/Combo layouts, source path, and module
 * ownership are retained. combo_ValidComboAward was additionally verified
 * instruction-by-instruction at RVAs 0x27AB0..0x27AE3.
 *
 * The dependency-light CMB loader is a portable companion rather than an
 * original PDB procedure.
 * PDB module: 0016
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\combo.obj
 * Primary source: W:\SWJediPowerBattles\Work\combo.c
 * Compiler language: c
 * Emitted procedures: 8
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/combo.h"
#include "jpb/anim.h"
#include "jpb/animctrl.h"
#include "jpb/animutil.h"
#include "jpb/brainutl.h"
#include "jpb/game.h"
#include "jpb/io.h"
#include "jpb/scene.h"
#include "jpb/world.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* 0x26B70, 1680 bytes, global, 19 named locals
 * combo_CheckCombo
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\combo.c
 */
static int combo_is_available(
    const playerObject *player, int x)
{
    if (LevelSelect == 11 ||
        LevelSelect == 13 ||
        GameStruct.versusModeFlag != 0 ||
        player->playerID > 8) {
        return 1;
    }
    return game_getCombo(
               (uint32_t)GameStruct.ModelSelect[
                   player->playernum],
               (uint32_t)x) != 0;
}

static int combo_retain_precombo_suffix(
    playerObject *player)
{
    int x;
    size_t length = strlen(player->PreMotion);

    for (x = 0; x < player->maxCombos; ++x) {
        Combo *paCombos = &player->paCombos[x];
        size_t combo_length;
        char buf[16];
        char *suffix;

        if ((paCombos->comboFlags &
             UINT32_C(0x00200000)) == 0) {
            continue;
        }
        combo_length = strlen(paCombos->String);
        if (combo_length >= length) {
            continue;
        }
        suffix =
            player->PreMotion + length - combo_length;
        if (strcmp(suffix, paCombos->String) != 0) {
            continue;
        }
        if (animutl_GetCurrentLock(
                &player->playerRoot) >= 23 &&
            animutl_GetPercentPlayed(
                &player->playerRoot) >= 6) {
            continue;
        }
        strcpy(buf, suffix);
        strcpy(player->PreMotion, buf);
        return 1;
    }
    return 0;
}

int combo_CheckCombo(
    int32_t *cpad, playerObject *player)
{
    Combo *paCombos = player->paCombos;
    Motion *paMotions = player->paMotions;
    size_t length = strlen(player->PreMotion);
    int cflag = 0;
    int x;

    (void)cpad;
    if (length != 0) {
        for (x = 0; x < player->maxCombos; ++x) {
            Combo *combo = &paCombos[x];
            int order;
            int move;
            int animResult;
            Motion *motion;

            if (combo->Len != (int16_t)length ||
                !combo_is_available(player, x)) {
                continue;
            }
            order = strcmp(
                player->PreMotion, combo->String);
            if (order < 0) {
                break;
            }
            if (order > 0) {
                continue;
            }

            if (combo->kdmin != 0 ||
                combo->kdmax != 0) {
                int time =
                    (player->ctime - player->vtime) /
                    0x200;

                if (time < combo->kdmin) {
                    if (combo->kdmax < time) {
                        strcat(player->PreMotion, "K");
                    } else {
                        strcat(player->PreMotion, "J");
                        player->mtime = player->ctime;
                    }
                    continue;
                }
                if (time > combo->kdmax) {
                    strcat(player->PreMotion, "K");
                    continue;
                }
            }

            player->vtime = player->ctime;
            if ((combo->comboFlags &
                 UINT32_C(0x00200000)) != 0 &&
                (player->pFlags &
                 UINT32_C(0x00200000)) != 0) {
                continue;
            }
            if ((combo->comboFlags &
                 UINT32_C(0xffff)) != 0) {
                if ((player->heldMask &
                     combo->comboFlags &
                     UINT32_C(0xffff)) == 0) {
                    continue;
                }
                cflag = 1;
            }
            if ((combo->comboFlags &
                 UINT32_C(0x01000000)) != 0 &&
                (player->currentMotion !=
                     (int16_t)combo->userData ||
                 animutl_GetPercentPlayed(
                     &player->playerRoot) >= 7)) {
                strcpy(player->PreMotion, "K");
                continue;
            }

            if (combo->Index == UINT8_C(0xff)) {
                strcat(player->PreMotion, ".");
                player->pFlags |= UINT32_C(0x00200000);
                return 1;
            }

            move =
                combo->Crap == 0 || player->mtime == 0
                    ? combo->Index
                    : combo->Crap;
            motion = &paMotions[move];
            if ((motion->motionFlags &
                 UINT32_C(0x00100000)) != 0) {
                motion->Recoil = 26;
                motion->RecoilAcc = 0;
            }
            animResult = animctrl_MotionComboChain(
                &player->playerRoot,
                motion,
                (int)(combo->comboFlags &
                      UINT32_C(0x00020000)),
                cflag,
                0);
            if (animResult == 0) {
                strcat(player->PreMotion, "!");
                motion->combo = (uint8_t)x;
                player->mtime = 0;
                return 1;
            }

            if (player->playernum < 2 && x < 32) {
                ++comboTally[player->playernum][x];
            }
            motion->combo = (uint8_t)x;
            if (animResult == 1) {
                strcat(player->PreMotion, ".");
                player->playerPad.bufferedbits = 0;
            } else if (animResult == -1) {
                strcat(player->PreMotion, ".");
                player->playerPad.bufferedbits = 0;
                if (combo->prev >= 0) {
                    paMotions[
                        paCombos[combo->prev].Index]
                        .combo =
                        (uint8_t)combo->numHits;
                }
            } else {
                player->mtime = 0;
                return 1;
            }

            if ((combo->comboFlags &
                 UINT32_C(0x00040000)) != 0) {
                int fFrame;
                int lFrame;

                player->pFlags |= UINT32_C(0x00020000);
                player->chainSlack = combo->slack;
                anim_GetSeqFrameRange(
                    &player->playerRoot,
                    motion,
                    &fFrame,
                    &lFrame);
                player->chainSlackEnd = (int16_t)lFrame;
            }
            player->pFlags |= UINT32_C(0x00200000);
            player->pFlags &= ~UINT32_C(0x01000000);
            if ((combo->comboFlags &
                 UINT32_C(0x00080000)) != 0) {
                player->pFlags &=
                    ~UINT32_C(0x00020000);
                player->pFlags |=
                    UINT32_C(0x02000000);
                player->chainSlack = 0;
                player->chainSlackEnd = 0;
            }
            player->comboUserData = combo->userData;
            player->mtime = 0;
            return 1;
        }
    }

    if ((player->pFlags & UINT32_C(0x00200000)) == 0) {
        (void)combo_retain_precombo_suffix(player);
    } else if (
        (player->pFlags & UINT32_C(0x01200000)) ==
        UINT32_C(0x00200000)) {
        player->pFlags |= UINT32_C(0x01000000);
    }

    length = strlen(player->PreMotion);
    if (length > 24) {
        memmove(
            player->PreMotion,
            player->PreMotion + length - 24,
            25);
    }
    return 0;
}

/* 0x27200, 209 bytes, global, 5 named locals
 * combo_CheckHeldPad
 * PDB type: void (long*, playerObject*, long...
 * Source: W:\SWJediPowerBattles\Work\combo.c
 */
void combo_CheckHeldPad(
    int32_t *cpad,
    playerObject *player,
    int32_t flag,
    int32_t held)
{
    int key;
    uint32_t mask = UINT32_C(1);

    for (key = 0;
         key < JPB_PLAYER_HELD_SLOTS;
         ++key, mask = (mask << 1) | (mask >> 31)) {
        if (((uint32_t)flag & mask) == 0) {
            continue;
        }
        if (((uint32_t)cpad[1] & mask) != 0) {
            if (player->bheld[key] == 0) {
                player->bheld[key] =
                    brainutl_ElapsedTime(0, 0);
            } else if (
                brainutl_ElapsedTime(
                    player->bheld[key],
                    (uint32_t)held << 9) != 0 &&
                (player->heldMask & mask) == 0) {
                player->heldMask |= mask;
            }
        } else {
            if (player->bheld[key] != 0) {
                if ((player->releaseMask & mask) == 0) {
                    player->releaseMask |= mask;
                }
            } else {
                player->releaseMask &= ~mask;
            }
            player->bheld[key] = 0;
        }
    }
}

/* 0x272E0, 342 bytes, global, 5 named locals
 * combo_CheckPreCombo
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\combo.c
 */
int combo_CheckPreCombo(
    int32_t *cpad, playerObject *player)
{
    size_t length = strlen(player->PreMotion);

    (void)cpad;
    if (combo_retain_precombo_suffix(player)) {
        return 1;
    }
    return length < 6;
}

/* 0x27440, 36 bytes, global, 3 named locals
 * combo_GetHeldTime
 * PDB type: int (playerObject*, long)
 * Source: W:\SWJediPowerBattles\Work\combo.c
 */
int combo_GetHeldTime(
    playerObject *player, int32_t flag)
{
    int key;

    for (key = 0;
         key < JPB_PLAYER_HELD_SLOTS;
         ++key) {
        if (((uint32_t)flag &
             (UINT32_C(1) << key)) != 0) {
            return (int)brainutl_DeltaTime(
                player->bheld[key]);
        }
    }
    return 0;
}

/* 0x27470, 568 bytes, global, 7 named locals
 * combo_InitComboData
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\combo.c
 */
void combo_InitComboData(playerObject *player)
{
    int hit_counts[8] = {0};
    const char *previous_string = NULL;
    int16_t previous_length = 0;
    int threshold = 0;
    int x;

    for (x = 0; x < player->maxCombos; ++x) {
        Combo *combo = &player->paCombos[x];
        const char *cursor;
        int num_hits = 0;

        combo->Len = (int16_t)strlen(combo->String);
        if (combo->Len == previous_length &&
            previous_string != NULL &&
            strcmp(combo->String, previous_string) < 0) {
            /*
             * The reference calls its fatal-error routine with status 1.
             * A malformed authored CMB is unrecoverable at this stage.
             */
            abort();
        }
        if ((combo->comboFlags & UINT32_C(0x04000000)) != 0 &&
            player->playerID < 9) {
            combo->Index =
                (uint8_t)(combo->Index + (uint8_t)';');
        }
        previous_length = combo->Len;
        previous_string = combo->String;

        for (cursor = combo->String; *cursor != '\0'; ++cursor) {
            if (*cursor == 'f') {
                num_hits = 6;
                break;
            }
            if (*cursor == 'n' ||
                *cursor == 's' ||
                *cursor == 'w') {
                ++num_hits;
            }
        }
        combo->numHits = (int16_t)num_hits;
        if (game_getCombo(
                (uint32_t)player->playerID,
                (uint32_t)x) != 0) {
            ++hit_counts[num_hits];
        }
    }

    switch (player->playerID) {
    case 0:
    case 5:
        threshold = hit_counts[4] >= 3 ? 6 : 4;
        break;
    case 1:
    case 4:
    case 8:
        threshold = hit_counts[3] >= 4 ? 6 : 3;
        break;
    case 2:
        threshold = hit_counts[3] >= 3 ? 6 : 3;
        break;
    case 3:
        threshold = hit_counts[3] >= 6 ? 6 : 3;
        break;
    default:
        break;
    }
    if (player->playernum >= 0 &&
        player->playernum <
            JPB_COMBO_AWARD_THRESHOLD_CAPACITY) {
        jpb_comboAwardHitThreshold[player->playernum] =
            threshold;
    }
}

enum JPBComboLoadResult jpb_ComboLoadFile(
    const char *path,
    void *storage,
    size_t storage_capacity,
    Combo **combos,
    int16_t *combo_count)
{
    JPBFileHandle file = 0;
    uint64_t file_size;
    uint64_t count;

    if (combos != NULL) {
        *combos = NULL;
    }
    if (combo_count != NULL) {
        *combo_count = 0;
    }
    if (path == NULL || storage == NULL ||
        combos == NULL || combo_count == NULL) {
        return JPB_COMBO_INVALID_ARGUMENT;
    }
    if (!file_OPEN((char *)path, &file)) {
        return JPB_COMBO_IO_ERROR;
    }
    file_size = file_GETSIZE(&file);
    if (file_size > storage_capacity ||
        file_size > INT32_MAX) {
        (void)file_CLOSE(&file);
        return JPB_COMBO_STORAGE_TOO_SMALL;
    }
    if (file_size == 0 ||
        file_size % sizeof(Combo) != 0) {
        (void)file_CLOSE(&file);
        return JPB_COMBO_INVALID_SIZE;
    }
    count = file_size / sizeof(Combo);
    if (count > INT16_MAX) {
        (void)file_CLOSE(&file);
        return JPB_COMBO_INVALID_SIZE;
    }
    if (file_READ(
            &file,
            (char *)storage,
            (int32_t)file_size,
            JPB_FILE_READ_STREAM) != file_size) {
        (void)file_CLOSE(&file);
        return JPB_COMBO_IO_ERROR;
    }
    (void)file_CLOSE(&file);
    *combos = (Combo *)storage;
    *combo_count = (int16_t)count;
    return JPB_COMBO_OK;
}

/* 0x276B0, 409 bytes, global, 6 named locals
 * combo_ReadCombo
 * PDB type: void (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\combo.c
 */
void combo_ReadCombo(
    int32_t *cpad, playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    animObject *animation =
        (animObject *)scene->pAnim;
    int frame =
        (animation->animFrameIndex >> 12) -
        player->subOffset +
        player->chainSlack;
    int LOCK = frame < 0 || frame > player->chainSlack;

    if ((player->pFlags & UINT32_C(0x02000000)) != 0) {
        cpad[0] = 0;
        cpad[1] = 0;
    }

    if (LOCK &&
        (player->pFlags & UINT32_C(0x00020000)) != 0) {
        if (cpad[0] != 0) {
            strcat(player->PreMotion, "SERR");
            player->pFlags &= ~UINT32_C(0x00020000);
            player->chainSlack = 0;
            player->chainSlackEnd = 0;
        }
        player->ctime += 0x200;
        player->playerPad.bufferedbits = 0;
        return;
    }

    if (cpad[1] == 0) {
        player->HeldMotion[0] = '\0';
    } else {
        brainutl_HeldPad(player, cpad);
    }

    if (cpad[0] == 0) {
        uint32_t time = brainutl_ElapsedTime(
            (uint32_t)player->ctime, UINT32_C(0x800));
        uint32_t reset;

        if (time != 0) {
            player->playerPad.bufferedbits = 0;
        }
        reset = brainutl_ElapsedTime(
            (uint32_t)player->ctime, UINT32_C(0x3000));
        if (reset != 0) {
            combo_ResetComboEngine(reset, player);
        }
    } else {
        uint32_t time;

        brainutl_MultiPad(player, cpad);
        time = brainutl_ElapsedTime(0, 0);
        player->dtime = (int32_t)time - player->ctime;
        player->ctime = (int32_t)time;
        if (((uint32_t)cpad[0] & UINT32_C(0xf000)) != 0) {
            player->playerPad.bufferedbits =
                (uint32_t)cpad[0];
        }
    }
}

/* 0x27850, 596 bytes, global, 2 named locals
 * combo_ResetComboEngine
 * PDB type: void (unsigned long, playerObjec...
 * Source: W:\SWJediPowerBattles\Work\combo.c
 */
void combo_ResetComboEngine(
    uint32_t time, playerObject *player)
{
    uint32_t original_flags = player->pFlags;
    uint32_t button;

    player->vtime = (int32_t)time;
    player->ctime = (int32_t)time;
    player->dtime = (int32_t)time;
    player->PreMotion[0] = '\0';
    player->mtime = 0;
    player->playerPad.bufferedbits = 0;
    player->pFlags &= UINT32_C(0xfffdffff);
    if ((original_flags & UINT32_C(0x01200000)) ==
        UINT32_C(0x00200000)) {
        player->pFlags |= UINT32_C(0x01000000);
    }
    player->chainSlack = 0;
    player->chainSlackEnd = 0;

    for (button = 0; button < JPB_PLAYER_HELD_SLOTS; ++button) {
        uint32_t mask = UINT32_C(1) << button;

        if (player->bheld[button] == 0) {
            player->heldMask &= ~mask;
        }
        player->releaseMask &= ~mask;
    }
}

/* 0x27AB0, 51 bytes, global, 2 named locals
 * combo_ValidComboAward
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\combo.c
 */
int combo_ValidComboAward(int jedi, int combo)
{
    return gaPlayerData[jedi]
               .paCombos[combo]
               .numHits <=
           jpb_comboAwardHitThreshold[jedi];
}
