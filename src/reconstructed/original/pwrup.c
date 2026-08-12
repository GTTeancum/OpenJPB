/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\pwrup.c.
 *
 * The exact authored power-up stream loader, checkpoint lifecycle, emitter
 * timer path, proximity traversal, and collection/effect dispatcher are
 * recovered here. The legacy immediate-mode model renderer remains isolated
 * behind a dependency-light realization seam; its original PDB call surface
 * and arguments are retained.
 *
 * Provenance:
 *   direct     - procedure/global/type names and layouts from game.pdb;
 *   assembly   - branches, mutation order, constants, initialized data, and
 *                callees checked at exact RVAs 0xE9390 through 0xEB686 in
 *                matched game.exe;
 *   inferred   - bounded serialized-data and release helpers prefixed jpb_.
 *
 * PDB module: 0066
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\pwrup.obj
 * Primary source: W:\SWJediPowerBattles\Work\pwrup.c
 */

#include "jpb/pwrup.h"

#include "jpb/achievement.h"
#include "jpb/animutil.h"
#include "jpb/bullet.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/cube.h"
#include "jpb/effects.h"
#include "jpb/filesys.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/intersec.h"
#include "jpb/jedi.h"
#include "jpb/loader.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/resources.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/vectors.h"
#include "jpb/world.h"

#include <stdlib.h>
#include <string.h>

/* Exact PDB globals at matched-PC RVAs 0x53D344 through 0x53D460. */
int32_t maxCheckPoints;
int32_t usedCheckPoints;
_svector aCheckPoints[JPB_POWERUP_CHECKPOINT_CAPACITY];
List poopList[2];
int32_t gPoopMode;
/* Exact PDB global at matched-PC RVA 0x94F320. */
powerPoop *poopArray;

/* Exact initialized PDB globals at matched-PC RVAs 0x4CBBF0..0x4CBDCC. */
char *powerUpNames[17] = {
    "HEAL", "HEAL+", "FORCE", "FORCE+", "ITEM", "CHECK POINT", "",
    "POINTS!", "POINTS!!", "AMP", "EXTENDER", "???", "ULTIMATE", "",
    "ARTIFACT", "CHALLENGE", "LIFE"
};
unsigned powColorLimit = 0x40;
char *powerUpFiles[17] = {
    "health_1", "health_2", "force_1", "force_2", "stuff", "check", "",
    "points_1", "points_2", "saber_b", "saber_g", "random", "uber", "",
    "g_art", "chal", "life"
};
int32_t powerUpScales[17] = {
    4096, 4096, 4096, 4096, 2048, 4096, 4096, 4096, 4096,
    4096, 4096, 2048, 4096, 4096, 4096, 4096, 4096
};
CVECTOR pwrIcons[17] = {
    {0x80, 0x80, 0x80, 0xad}, {0x80, 0x80, 0x80, 0xad},
    {0x80, 0x80, 0x80, 0xac}, {0x80, 0x80, 0xc0, 0xac},
    {0x80, 0x80, 0x80, 0xc3}, {0x80, 0x80, 0x80, 0xa7},
    {0x20, 0x20, 0x20, 0xa7}, {0x80, 0x80, 0x80, 0xc2},
    {0x80, 0x80, 0x80, 0xc1}, {0x80, 0x80, 0x80, 0xc6},
    {0x80, 0x80, 0x80, 0xc7}, {0x80, 0x80, 0x80, 0xc5},
    {0x80, 0x80, 0x80, 0xc4}, {0x00, 0x00, 0x00, 0x00},
    {0x80, 0x80, 0x80, 0x9b}, {0x80, 0x80, 0x80, 0xc0},
    {0x80, 0x80, 0x80, 0xb2}
};
int32_t mRandomPower[9] = {15, 3, 1, 3, 15, 1, 3, 1, 15};
int32_t cheat_currentCheckPoint;

/* Exact pwrup.c file statics at RVAs 0x4CBDCC and 0x546470/74. */
static int32_t growmod = 1;
static unsigned powerrott;
static int32_t grow;

static JPBPowerupDrawHook jpb_pwrup_draw_hook;
static void *jpb_pwrup_draw_user_data;
static size_t jpb_pwrup_loaded_count;

static uint16_t pwrup_read_u16(const uint8_t *source)
{
    uint16_t value;

    memcpy(&value, source, sizeof(value));
    return value;
}

static int16_t pwrup_i16_from_bits(uint16_t bits)
{
    int16_t value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

int jpb_PwrupLoadData(const void *data, size_t size)
{
    const uint8_t *source = (const uint8_t *)data;
    size_t record_count;
    size_t index;
    powerPoop *records;

    if ((data == NULL && size != 0) ||
        size % JPB_POWERUP_DISK_RECORD_SIZE != 0) {
        return 0;
    }
    record_count = size / JPB_POWERUP_DISK_RECORD_SIZE;
    if (record_count > JPB_POWERUP_CAPACITY) {
        return 0;
    }
    records = (powerPoop *)calloc(
        JPB_POWERUP_CAPACITY, sizeof(*records));
    if (records == NULL) {
        return 0;
    }

    free(poopArray);
    poopArray = records;
    list_InitList(&poopList[0]);
    list_InitList(&poopList[1]);
    for (index = 0; index < record_count; ++index) {
        const uint8_t *position =
            source + index * JPB_POWERUP_DISK_RECORD_SIZE + 4;

        records[index].pos.vx =
            pwrup_i16_from_bits(pwrup_read_u16(position));
        records[index].pos.vy =
            pwrup_i16_from_bits(pwrup_read_u16(position + 2));
        records[index].pos.vz =
            pwrup_i16_from_bits(pwrup_read_u16(position + 4));
        records[index].pos.pad =
            pwrup_i16_from_bits(pwrup_read_u16(position + 6));
        list_AddTail(&poopList[0], (Node *)&records[index]);
    }
    jpb_pwrup_loaded_count = record_count;
    return 1;
}

size_t jpb_PwrupLoadedCount(void)
{
    return jpb_pwrup_loaded_count;
}

void jpb_PwrupReleaseData(void)
{
    free(poopArray);
    poopArray = NULL;
    jpb_pwrup_loaded_count = 0;
    list_InitList(&poopList[0]);
    list_InitList(&poopList[1]);
}

void jpb_PwrupSetDrawHook(
    JPBPowerupDrawHook hook, void *user_data)
{
    jpb_pwrup_draw_hook = hook;
    jpb_pwrup_draw_user_data = user_data;
}

/*
 * Reference RVA 0xE9390, 811 bytes.
 *
 * The matched body submits powerUpData[type] through the removed immediate
 * renderer. Its exact, renderer-independent call surface is retained here;
 * the portable PC owner supplies a realization without coupling gameplay to
 * a host graphics API. The original mesh submission remains a tracked gap.
 */
void DrawPowerUp(
    _svector *position,
    unsigned type,
    _svector *rotation,
    VECTOR *scale,
    _svector *offset)
{
    if (jpb_pwrup_draw_hook != NULL) {
        jpb_pwrup_draw_hook(
            jpb_pwrup_draw_user_data,
            position,
            type,
            rotation,
            scale,
            offset);
    }
}

/* 0xE96C0, 650 bytes: FixDrawPowerUp remains unreconstructed. */

/* Reference RVA 0xE9950, 195 bytes. */
void cheat_nextCheckPoint(void)
{
    _svector fixed_position = {
        INT16_C(0x38d0), INT16_C(0x2700),
        (int16_t)-INT16_C(0x5d70), 0
    };
    _svector *position;

    if (gpWorld == NULL || gpWorld->player0 == NULL ||
        maxCheckPoints <= 1) {
        return;
    }
    if (LevelSelect == 9) {
        position = &fixed_position;
    } else {
        cheat_currentCheckPoint =
            (cheat_currentCheckPoint + 1) % maxCheckPoints;
        if (cheat_currentCheckPoint < 1) {
            cheat_currentCheckPoint = 1;
        } else if (cheat_currentCheckPoint > maxCheckPoints - 1) {
            cheat_currentCheckPoint = maxCheckPoints - 1;
        }
        position = &aCheckPoints[cheat_currentCheckPoint];
    }
    physics_gSetPosition(
        &gpWorld->player0->playerRoot,
        position->vx,
        position->vy,
        position->vz);
    if (GameStruct.NumPlayers == 2 && gpWorld->player1 != NULL) {
        physics_gSetPosition(
            &gpWorld->player1->playerRoot,
            position->vx,
            position->vy,
            position->vz);
    }
}

/* 0xE9A20, 1012 bytes: console_PowerCommand remains unreconstructed. */

/* Reference RVA 0xE9E20, 106 bytes. */
unsigned fixPowColor(unsigned color)
{
    unsigned red = ((color >> 16) & 0xffu) + 0x20u;
    unsigned green = ((color >> 8) & 0xffu) + 0x20u;
    unsigned blue = (color & 0xffu) + 0x20u;

    if (red < powColorLimit) red = powColorLimit;
    if (green < powColorLimit) green = powColorLimit;
    if (blue < powColorLimit) blue = powColorLimit;
    if (red > 0xffu) red = 0xffu;
    if (green > 0xffu) green = 0xffu;
    if (blue > 0xffu) blue = 0xffu;
    return UINT32_C(0xff000000) |
        (red << 16) | (green << 8) | blue;
}

/* 0xE9E90, 909 bytes: jitteryFesteringMatrixCrack remains unreconstructed. */

/* Reference RVA 0xEA220, 6 bytes. */
int kmAudioSFX_DumpBank(int bankID)
{
    (void)bankID;
    return -1;
}

/* Reference RVA 0xEA230, 166 bytes. */
int mrktng_GoToNextCheckpoint(void)
{
    int checkpoint;
    _svector *position;

    if (gpWorld == NULL || gpWorld->player0 == NULL ||
        GameStruct.CurrentLevel >= JPB_GAME_CHECKPOINT_CAPACITY) {
        return 0;
    }
    checkpoint = maxCheckPoints - 1;
    if ((int)GameStruct.checkpoint[GameStruct.CurrentLevel] + 1 <=
        maxCheckPoints - 1) {
        checkpoint =
            (int)GameStruct.checkpoint[GameStruct.CurrentLevel] + 1;
    }
    if (checkpoint == 0) {
        return 0;
    }
    position = &aCheckPoints[checkpoint];
    physics_gSetPosition(
        &gpWorld->player0->playerRoot,
        position->vx,
        position->vy,
        position->vz);
    if (GameStruct.NumPlayers == 2 && gpWorld->player1 != NULL) {
        physics_gSetPosition(
            &gpWorld->player1->playerRoot,
            position->vx,
            position->vy,
            position->vz);
    }
    return 1;
}

static int pwrup_abs(int value)
{
    return value < 0 ? -value : value;
}

static uint32_t pwrup_current_dolly_flags(void)
{
    if (gpWorld == NULL || gpWorld->currentDolly < 0 ||
        gpWorld->currentDolly >= 256) {
        return 0;
    }
    return gpWorld->aDolly[gpWorld->currentDolly].flags;
}

static void pwrup_resurrect_after_life(const _svector *position)
{
    playerObject *player = afterLife;
    physicsObject *other_physics;
    physicsObject *player_physics;
    Projectile *projectile;
    FVECTOR test_position;
    VECTOR *projectile_position;
    int player_index;
    int other_index;
    int old_score;
    int old_counter;

    if (player == NULL || GameStruct.NumPlayers != 2 ||
        position == NULL) {
        return;
    }
    player_index = player->playernum;
    other_index = player_index ^ 1;
    if ((unsigned)player_index >= 2u ||
        (unsigned)other_index >= 2u) {
        return;
    }
    other_physics = &maPhysicsData[other_index];
    if (pwrup_abs(other_physics->vpos.vx - position->vx) > 0x400 ||
        pwrup_abs(other_physics->vpos.vy - position->vy) > 0x400 ||
        pwrup_abs(other_physics->vpos.vz - position->vz) > 0x400) {
        return;
    }
    test_position.vx = (float)position->vx;
    test_position.vy = (float)position->vy;
    test_position.vz = (float)position->vz;
    if (cliptofrustrum(
            collisionfrustrum,
            &test_position,
            -0x40,
            NULL) != 0) {
        return;
    }

    old_score = game_gGetScore(player_index);
    old_counter = GameStruct.Counter;
    player_RefreshPlayer(player);
    game_gSetScore(player_index, old_score);
    GameStruct.Counter = old_counter;
    physics_gSetPosition(
        &player->playerRoot,
        position->vx,
        position->vy,
        position->vz);
    player_physics = &maPhysicsData[player_index];
    player_physics->validairground = (float)intersec_FindWalkHeightSV(
        (_svector *)(void *)position,
        NULL,
        &player->playerRoot,
        0);
    if (player->shadow == NULL) {
        player->shadow = (int32_t *)(void *)sprite_GetBaseNodeMarker(
            player_index, 0x30);
    }
    camera_SetCurrentCameraType(0);
    projectile = bullet_AllocProjectile(0x19);
    if (projectile != NULL) {
        projectile_position = &player_physics->vpos;
        bullet_ShootProjectile(
            projectile,
            player,
            projectile_position,
            projectile_position,
            NULL);
    }
    physics_ResetJedi(player_index);
    anim_ResetJedi(player_index);
    player_ResetJedi(player_index);
    afterLife = NULL;
}

static void pwrup_emit_authored_powerups(void)
{
    int index;

    if (gpWorld == NULL || gpWorld->pPowerups == NULL) {
        return;
    }
    for (index = 0; index < gpWorld->nPowerups; ++index) {
        wsl_Powerup *powerup = &gpWorld->pPowerups[index];
        int range = powerup->pos.pad;

        if (pwrup_abs(gpWorld->location.vx - powerup->pos.vx) > range ||
            pwrup_abs(gpWorld->location.vy - powerup->pos.vy) > range ||
            pwrup_abs(gpWorld->location.vz - powerup->pos.vz) > range) {
            continue;
        }
        if (powerup->timer == 0 &&
            (powerup->data & UINT16_C(0xc000)) == 0) {
            if (powerup->type == 13 && powerup->rate != 0) {
                EffectHeader *effect =
                    paEffects[(uint8_t)powerup->data];
                VECTOR position = {
                    powerup->pos.vx,
                    powerup->pos.vy,
                    powerup->pos.vz
                };

                if (effect != NULL) {
                    (void)sprite_AddSpriteEffect(
                        effect->aEffects,
                        (int)effect->num,
                        &position,
                        NULL);
                }
                powerup->timer = (int32_t)(
                    (uint32_t)powerup->rate * UINT32_C(0x200) +
                    gGlobalTimer);
            }
        } else if ((powerup->data & UINT16_C(0x4000)) == 0 &&
                   (uint32_t)powerup->timer < gGlobalTimer) {
            powerup->timer = 0;
        }
    }
}

static VECTOR *pwrup_player_position(int player_index)
{
    if ((unsigned)player_index >= 2u) {
        return NULL;
    }
    return &maPhysicsData[player_index].vpos;
}

static void pwrup_points(
    int player_index,
    int points,
    _svector *velocity,
    uint32_t color)
{
    VECTOR *position = pwrup_player_position(player_index);

    if (position != NULL) {
        (void)sprite_GetPointsSprite(
            points, position, velocity, color, 0);
    }
}

static void pwrup_sound(int player_index, char *name)
{
    VECTOR *position = pwrup_player_position(player_index);

    if (position != NULL) {
        (void)sound_Play(position, 0, name, 0);
    }
}

static void pwrup_checkpoint_special_case(
    const _svector *position)
{
    if ((pwrup_current_dolly_flags() & UINT32_C(0x400)) == 0) {
        if (GameStruct.ContinuesUsed != GameStruct.mNumContinues &&
            afterLife != NULL && GameStruct.NumPlayers == 2) {
            pwrup_resurrect_after_life(position);
        }
        return;
    }
    if (LevelSelect == 1) {
        _svector trigger = {
            (int16_t)UINT16_C(0xa97d), INT16_C(0x1f00),
            INT16_C(0x0815), 0
        };

        if (vec_DistanceSV(&trigger, (_svector *)(void *)position) < 0x100) {
            fed_wallfrigflag = 1;
        }
    } else if (LevelSelect == 5) {
        _svector trigger = {
            (int16_t)UINT16_C(0xa97d), INT16_C(0x1f00),
            INT16_C(0x0815), 0
        };

        if (vec_DistanceSV(&trigger, (_svector *)(void *)position) < 0x100) {
            tato_wallfrigflag = 1;
        }
    }
}

static void pwrup_collect(
    powerPoop *poop, int player_index, unsigned type)
{
    static _svector points_velocity = {-4, 6, -4, 0};
    static _svector comment_velocity = {0, 16, 0, 0};
    playerObject *player = &gaPlayerData[player_index];
    VECTOR pickup_position = {
        poop->pos.vx, poop->pos.vy, poop->pos.vz
    };
    uint32_t color = jedi_GetColour32(
        (uint64_t)(uint16_t)player->playerID);
    EffectHeader *effect = paEffects[21];

    poop->pos.pad = (int16_t)(
        (uint16_t)poop->pos.pad | JPB_POWERUP_COLLECTED_FLAG);
    if (type == 11) {
        type = (unsigned)mRandomPower[rand() % 9];
    }
    if (player->playerID == 17 && type != JPB_POWERUP_TYPE_CHECKPOINT) {
        Mnode *torso = coll_GetNode(
            player->playerRoot.objectID, 7);

        if (torso != NULL) {
            torso->flags &= UINT32_C(0xfbfffffb);
        }
    }
    if (effect != NULL) {
        (void)sprite_AddSpriteEffect(
            effect->aEffects,
            (int)effect->num,
            &pickup_position,
            NULL);
    }
    if (type < 17) {
        (void)sprite_GetCommentsSprite(
            powerUpNames[type],
            &pickup_position,
            &comment_velocity,
            UINT32_C(0xaff0f0f0));
    }

    switch (type) {
    case 0:
    case 1: {
        int amount = type == 0 ? 50 : 100;

        game_gModEnergy(player_index, amount);
        game_gModScore(player_index, amount);
        pwrup_points(player_index, amount, &points_velocity, color);
        pwrup_sound(player_index, "xsecret");
        if (player_index == 0) {
            int count = achievement_getcount(0x24) + 1;

            achievement_update(0x24, count);
            if (achievement_getcount(0x24) > 4) {
                achievement_complete(0x24);
            }
        }
        break;
    }

    case 2:
        game_gModForce(player_index, 50);
        game_gModScore(player_index, 50);
        pwrup_points(player_index, 50, &points_velocity, color);
        pwrup_sound(player_index, "xsecret");
        break;

    case 3:
        game_gModForce(player_index, 100);
        game_gModScore(player_index, 100);
        pwrup_points(player_index, 100, &points_velocity, color);
        pwrup_sound(player_index, "xsecret");
        break;

    case 4:
        game_gModItemCount(player_index, 1);
        game_gModScore(player_index, 100);
        pwrup_points(player_index, 100, &points_velocity, color);
        pwrup_sound(player_index, "xsecret");
        break;

    case JPB_POWERUP_TYPE_CHECKPOINT: {
        int index;

        gCheckPoint = 1;
        reStartPos[0] = poop->pos;
        reStartPos[1] = poop->pos;
        reStartPos[0].pad = 0;
        reStartPos[1].pad = 0;
        reStartScore[0] = (uint32_t)game_gGetScore(0);
        reStartScore[1] = (uint32_t)game_gGetScore(1);
        game_gModScore(0, 50);
        game_gModScore(1, 50);
        reStartCounter = GameStruct.Counter;
        pwrup_points(player_index, 50, &points_velocity, color);
        pwrup_sound(player_index, "xsecret");
        ++usedCheckPoints;
        if (GameStruct.CurrentLevel < JPB_GAME_CHECKPOINT_CAPACITY) {
            for (index = 0; index < maxCheckPoints; ++index) {
                if (aCheckPoints[index].vx == poop->pos.vx &&
                    aCheckPoints[index].vy == poop->pos.vy &&
                    aCheckPoints[index].vz == poop->pos.vz) {
                    GameStruct.checkpoint[GameStruct.CurrentLevel] =
                        (uint8_t)index;
                }
            }
        }
        pwrup_checkpoint_special_case(&poop->pos);
        break;
    }

    case 7:
        pwrup_points(player_index, 1000, &points_velocity, color);
        game_gModScore(player_index, 1000);
        pwrup_sound(player_index, "xsecret");
        break;

    case 8:
        pwrup_points(player_index, 2500, &points_velocity, color);
        game_gModScore(player_index, 2500);
        pwrup_sound(player_index, "xsecret");
        break;

    case 9:
        game_gSetPowerType(player_index, 9);
        game_gSetPowerLevel(player_index, 0xe1000);
        game_gModScore(player_index, 100);
        pwrup_points(player_index, 100, &points_velocity, color);
        pwrup_sound(player_index, "xsaberup");
        if (player_index == 0) {
            int count = achievement_getcount(5);

            if (count == 0) {
                achievement_update(5, -1);
            } else if (count == 1) {
                achievement_complete(5);
            }
        }
        break;

    case 10:
        game_gSetPowerType(player_index, 10);
        game_gSetPowerLevel(player_index, 0xe1000);
        game_gModScore(player_index, 100);
        pwrup_points(player_index, 100, &points_velocity, color);
        pwrup_sound(player_index, "xsaberup");
        if (player_index == 0) {
            int count = achievement_getcount(5);

            if (count == 0) {
                achievement_update(5, 1);
            } else if (count == -1) {
                achievement_complete(5);
            }
        }
        break;

    case 12:
        game_gModEnergy(player_index, 200);
        game_gModForce(player_index, 200);
        game_gModItemCount(player_index, 4);
        game_gSetPowerType(player_index, 9);
        game_gSetPowerLevel(player_index, 0x38400);
        pwrup_sound(player_index, "xsaberup");
        pwrup_sound(player_index, "xsecret");
        break;

    case 14:
        if (LevelSelect == 2) {
            abGlobalBits[1] |= UINT8_C(0x20);
        } else if (LevelSelect == 5) {
            abGlobalBits[1] |= UINT8_C(0x40);
        } else if (LevelSelect == 7) {
            abGlobalBits[1] |= UINT8_C(0x80);
        }
        game_gModScore(player_index, 50);
        pwrup_points(player_index, 50, &points_velocity, color);
        pwrup_sound(player_index, "xsecret");
        break;

    case 15:
        game_gModEnergy(
            player_index,
            -(game_gGetEnergy(player_index) / 2));
        game_gModForce(
            player_index,
            -(game_gGetForce(player_index) / 2));
        game_gModScore(player_index, 1500);
        pwrup_points(player_index, 1500, &points_velocity, color);
        pwrup_sound(player_index, "xposion");
        break;

    case 16:
        if (GameStruct.mNumContinues < 9) {
            ++GameStruct.mNumContinues;
        } else if (GameStruct.ContinuesUsed > 0) {
            --GameStruct.ContinuesUsed;
        }
        game_gModScore(player_index, 200);
        pwrup_points(player_index, 200, &points_velocity, color);
        pwrup_sound(player_index, "xsecret");
        break;

    default:
        break;
    }
    feedback_startEffect(player->playernum, 12);
}

/* Reference RVA 0xEA2E0, 4144 bytes. */
void pwrup_CheckPowerUps(void)
{
    _svector rotation = {0, 0, 0, 0};
    _svector offset = {0, 100, 0, 0};
    List *source;
    List *destination;
    powerPoop *poop;

    if (gpWorld == NULL) {
        return;
    }
    rotation.vy = (int16_t)powerrott;
    powerrott = (powerrott + 0x20u) & 0xfffu;
    gPoopMode = (int32_t)initialLevelPauseDelay;
    source = &poopList[mDrawingSurfaceId];
    destination = &poopList[mDrawingSurfaceId ^ 1];

    while ((poop = (powerPoop *)list_RemoveHead(source)) != NULL) {
        uint16_t raw_type = (uint16_t)poop->pos.pad;

        if ((int16_t)raw_type >= 0 &&
            pwrup_abs(gpWorld->location.vx - poop->pos.vx) <= 0x1400 &&
            pwrup_abs(gpWorld->location.vy - poop->pos.vy) <= 0x1400 &&
            pwrup_abs(gpWorld->location.vz - poop->pos.vz) <= 0x1400 &&
            !((abGlobalBits[2] & UINT8_C(2)) != 0 && raw_type == 14)) {
            grow += growmod;
            if (pwrup_abs(grow) > 0x18) {
                growmod = -growmod;
            }
            if (raw_type != 6 && raw_type < 17) {
                VECTOR scale = {
                    powerUpScales[raw_type],
                    powerUpScales[raw_type],
                    powerUpScales[raw_type]
                };

                DrawPowerUp(
                    &poop->pos,
                    raw_type,
                    &rotation,
                    &scale,
                    &offset);
            }
            if (raw_type == 6 &&
                (pwrup_current_dolly_flags() & UINT32_C(0x400)) == 0) {
                pwrup_resurrect_after_life(&poop->pos);
            } else if (gPoopMode == 0) {
                int player_index = -1;

                if (pwrup_abs(gpWorld->p0location.vx - poop->pos.vx) <= 0x80 &&
                    pwrup_abs(gpWorld->p0location.vy - poop->pos.vy) <= 0x80 &&
                    pwrup_abs(gpWorld->p0location.vz - poop->pos.vz) <= 0x80) {
                    player_index = 0;
                } else if (
                    pwrup_abs(gpWorld->p1location.vx - poop->pos.vx) <= 0x80 &&
                    pwrup_abs(gpWorld->p1location.vy - poop->pos.vy) <= 0x80 &&
                    pwrup_abs(gpWorld->p1location.vz - poop->pos.vz) <= 0x80) {
                    player_index = 1;
                }
                if (player_index >= 0) {
                    pwrup_collect(poop, player_index, raw_type);
                }
            }
        }
        list_AddTail(destination, (Node *)poop);
    }
    pwrup_emit_authored_powerups();
}

/* Reference RVA 0xEB310, 168 bytes. */
void pwrup_Init(void)
{
    unsigned list_index = (unsigned)mDrawingSurfaceId;
    powerPoop *charPoop;

    maxCheckPoints = 1;
    if (list_IsListEmpty(&poopList[list_index])) {
        list_index ^= 1u;
    }
    charPoop = (powerPoop *)poopList[list_index].head;
    while (charPoop != NULL) {
        charPoop->pos.pad &= INT16_C(0x7fff);
        if (charPoop->pos.pad == JPB_POWERUP_TYPE_CHECKPOINT &&
            maxCheckPoints < JPB_POWERUP_CHECKPOINT_CAPACITY) {
            int checkpoint_index = maxCheckPoints;

            aCheckPoints[checkpoint_index].vx = charPoop->pos.vx;
            ++maxCheckPoints;
            aCheckPoints[checkpoint_index].vy = charPoop->pos.vy;
            aCheckPoints[checkpoint_index].vz = charPoop->pos.vz;
        }
        charPoop = (powerPoop *)charPoop->node;
    }
}

/* Reference RVA 0xEB3C0, 164 bytes. */
int pwrup_JumpCheckPoint(void)
{
    int num;
    _svector *pos;
    playerObject *p0;
    playerObject *p1;

    p0 = gpWorld->player0;
    p1 = gpWorld->player1;
    if (GameStruct.CurrentLevel >= JPB_GAME_CHECKPOINT_CAPACITY) {
        return 0;
    }
    num = maxCheckPoints - 1;
    if (GameStruct.checkpoint[GameStruct.CurrentLevel] <= num) {
        num = GameStruct.checkpoint[GameStruct.CurrentLevel];
    }
    if (num == 0) {
        return 0;
    }
    pos = &aCheckPoints[num];
    physics_gSetPosition(
        &p0->playerRoot, pos->vx, pos->vy, pos->vz);
    if (GameStruct.NumPlayers == 2) {
        physics_gSetPosition(
            &p1->playerRoot, pos->vx, pos->vy, pos->vz);
    }
    return 1;
}

/* Reference RVA 0xEB470, 59 bytes. */
void pwrup_LevelEnd(void)
{
    int bonus = (maxCheckPoints - usedCheckPoints) * 150;
    int player = 0;

    if (GameStruct.NumPlayers == 2) {
        game_gModScore(0, bonus);
        player = 1;
    }
    game_gModScore(player, bonus);
}

/* Reference RVA 0xEB4B0, 74 bytes. */
void pwrup_LevelStart(void)
{
    powerPoop *poop;

    maxCheckPoints = 0;
    usedCheckPoints = 0;
    poop = (powerPoop *)poopList[mDrawingSurfaceId].head;
    while (poop != NULL) {
        poop->pos.pad &= INT16_C(0x7fff);
        if (poop->pos.pad == JPB_POWERUP_TYPE_CHECKPOINT) {
            ++maxCheckPoints;
        }
        poop = (powerPoop *)poop->node;
    }
}

/* Reference RVA 0xEB500, 390 bytes. */
void pwrup_LoadPoop(void)
{
    const char *path;
    char *buffer;
    int32_t len = 0;

    list_InitList(&poopList[0]);
    list_InitList(&poopList[1]);
    jpb_pwrup_loaded_count = 0;
    path = resource_getPathWithExtension(
        loader_GetLevelName(),
        JPB_RESOURCE_LEVEL_POWERUP,
        "pwr");
    if (path == NULL) {
        return;
    }
    buffer = file_LoadFile2PoolFunc(
        (char *)path,
        &len,
        2,
        0x1ee,
        "W:\\SWJediPowerBattles\\Work\\pwrup.c");
    if (buffer != NULL && len >= 0) {
        (void)jpb_PwrupLoadData(buffer, (size_t)len);
    }
}
