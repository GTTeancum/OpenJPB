/*
 * REVIEWED RECONSTRUCTION.
 * PDB module: 0091
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\vehicle.obj
 * Primary source: W:\SWJediPowerBattles\Work\vehicle.c
 * Compiler language: c
 * Emitted procedures: 6
 *
 * All six PDB-emitted procedures are reviewed. The STAP and tank bodies keep
 * their PDB-named persistent state and route targeting through the original
 * PDB-named FindBestMachineGunTarget dependency.
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/vehicle.h"

#include "jpb/anim.h"
#include "jpb/animctrl.h"
#include "jpb/bullet.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/cube.h"
#include "jpb/debugtext.h"
#include "jpb/fmath.h"
#include "jpb/flex.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/model.h"
#include "jpb/objroot.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/world.h"

#include <limits.h>
#include <stdlib.h>

static int32_t stapZeroBSSCheck;
static _svector stapAngle[2];
static uint32_t stapShot[2];
static int32_t stapShotZeroBSSCheck;

static int32_t tankSpeed;
static int32_t machinegunfacing;
static playerObject *tankTarget[2];
static int32_t tankElapsed;
static int32_t tankFireSequence;
static int32_t tankFireElapsed;
static int32_t tankWeirdFireElapsed;
static int32_t tankZeroBSSCheck;
static int32_t tankWhich;

static int32_t vehicle_signed_angle(int32_t value)
{
    uint32_t wrapped = (uint32_t)value & UINT32_C(0x0fff);

    if ((wrapped & UINT32_C(0x0800)) != 0) {
        wrapped |= UINT32_C(0xfffff000);
    }
    return (int32_t)wrapped;
}

static int32_t vehicle_emitted_abs(int32_t value)
{
    return value < 0
        ? (int32_t)(UINT32_C(0) - (uint32_t)value)
        : value;
}

static int32_t vehicle_clamp(int32_t value, int32_t low, int32_t high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static void vehicle_mark_rotation(Mnode *node)
{
    if (node != NULL) {
        node->flags |= JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY;
    }
}

static void vehicle_tank_recentre_gun(Mnode *node)
{
    int x;
    int y;

    if (node == NULL) {
        return;
    }
    x = vehicle_signed_angle(node->v3RotationAbs.vx);
    y = vehicle_signed_angle(node->v3RotationAbs.vy);
    node->v3RotationAbs.vx = (int16_t)(
        (uint16_t)node->v3RotationAbs.vx +
        (uint16_t)(x < -8 ? 8 : x > 8 ? -8 : -x));
    node->v3RotationAbs.vy = (int16_t)(
        (uint16_t)node->v3RotationAbs.vy +
        (uint16_t)(y < -4 ? 4 : y > 4 ? -4 : -y));
}

static void vehicle_tank_aim_gun(
    Mnode *node,
    VECTOR *base,
    VECTOR *muzzle,
    playerObject *target)
{
    VECTOR gun;
    VECTOR guy;
    physicsObject *targetPhysics;
    int anglediff;

    if (node == NULL || base == NULL || muzzle == NULL || target == NULL ||
        target->playerRoot.pParent == NULL) {
        vehicle_tank_recentre_gun(node);
        return;
    }
    targetPhysics = (physicsObject *)(
        (sceneObject *)target->playerRoot.pParent)->pPhysics;
    gun.vx = muzzle->vx - base->vx;
    gun.vy = muzzle->vy - base->vy;
    gun.vz = muzzle->vz - base->vz;
    guy.vx = (int)targetPhysics->pos.vx - base->vx;
    guy.vy = (int)targetPhysics->pos.vy + 100 - base->vy;
    guy.vz = (int)targetPhysics->pos.vz - base->vz;
    normalize_lvector(&gun, &gun);
    normalize_lvector(&guy, &guy);
    node->v3RotationAbs.vx = (int16_t)(
        (uint16_t)node->v3RotationAbs.vx +
        (uint16_t)(guy.vy < gun.vy ? 0x10 : -0x10));
    anglediff = vehicle_signed_angle(
        ratan2(gun.vz, gun.vx) - ratan2(guy.vz, guy.vx));
    node->v3RotationAbs.vy = (int16_t)(
        (uint16_t)node->v3RotationAbs.vy +
        (uint16_t)vehicle_clamp(anglediff, -8, 8));
}

static void vehicle_tank_fire(
    int projectileType,
    playerObject *owner,
    VECTOR *from,
    VECTOR *to,
    _svector *velocity)
{
    Projectile *proj = bullet_AllocProjectile(projectileType);

    if (proj != NULL && from != NULL && to != NULL) {
        proj->pj_Flags |= UINT32_C(0x10);
        bullet_ShootProjectile(proj, owner, from, to, velocity);
    }
}

/* 0x1047B0, 243 bytes, global, 4 named locals
 * StopNearestFan
 * PDB type: void (VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\vehicle.c
 */
void StopNearestFan(VECTOR *pos)
{
    int32_t closest = INT32_MAX;
    physicsObject *winner = NULL;
    int i;

    for (i = 2; i < JPB_PLAYER_CAPACITY; ++i) {
        playerObject *player = &gaPlayerData[i];
        physicsObject *physics = &maPhysicsData[i];

        if (player->playerRoot.objectID != -1 &&
            obj_gCheckObjectFlag(&player->playerRoot, 0, 0x20) == 0 &&
            player->playerID == 0x4c &&
            physics->userdata[0] == 0) {
            int32_t dist = normalize_l(
                (int)(physics->pos.vx - (float)pos->vx),
                (int)(physics->pos.vy - (float)pos->vy),
                (int)(physics->pos.vz - (float)pos->vz),
                NULL);

            if (dist < closest) {
                winner = physics;
                closest = dist;
            }
        }
    }

    if (winner != NULL) {
        winner->userdata[0] = 1;
    }
}

/* 0x1048B0, 479 bytes, global, 16 named locals
 * ai_AAT
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\vehicle.c
 */
int ai_AAT(int32_t *cpad, playerObject *player)
{
    Mnode *pNodeTurret = coll_GetNode(player->playernum, 9);
    sceneObject *scene = (sceneObject *)player->playerRoot.pParent;
    physicsObject *p = (physicsObject *)scene->pPhysics;

    (void)cpad;
    p->flags &= ~UINT32_C(0x00400000);
    if (playertankindex != 0) {
        VECTOR *tankpos = coll_GetNodeCenter(player->playernum, 9);
        physicsObject *tank_physics =
            &maPhysicsData[playertankindex - 1];
        int xd = (int)((float)tankpos->vx - tank_physics->pos.vx);
        int zd = (int)((float)tankpos->vz - tank_physics->pos.vz);
        int angle = vehicle_signed_angle((int32_t)(
            0u - ((uint32_t)ratan2(xd, -zd) +
                  (uint32_t)p->angle.vy)));
        int turretangle = vehicle_signed_angle(
            (int32_t)pNodeTurret->v3RotationAbs.vy - angle);
        int range = physics_gGetRange(
            &gaPlayerData[playertankindex - 1].playerRoot,
            &player->playerRoot);

        if (turretangle < -0x10) {
            pNodeTurret->v3RotationAbs.vy = (int16_t)(
                (uint16_t)pNodeTurret->v3RotationAbs.vy +
                (uint16_t)flexmul12(0xc, gGlobalFrameRate));
        } else if (turretangle > 0x10) {
            pNodeTurret->v3RotationAbs.vy = (int16_t)(
                (uint16_t)pNodeTurret->v3RotationAbs.vy -
                (uint16_t)flexmul12(0xc, gGlobalFrameRate));
        }

        if (vehicle_emitted_abs(turretangle) < 0x20 &&
            p->userdata[0] < 1 && range < 0x800) {
            int id = player->playerRoot.objectID;
            VECTOR *pos0 = coll_GetNodeCenter(id, 12);
            VECTOR *pos1 = coll_GetNodeCenter(id, 15);
            Projectile *proj = bullet_AllocProjectile(2);

            if (proj != NULL) {
                proj->pj_Flags |= UINT32_C(0x10);
                bullet_ShootProjectile(
                    proj, player, pos1, pos0, &p->svmov);
                p->userdata[0] = 0x3c000;
            }
        }
    }

    p->userdata[0] = p->userdata[0] > 0
        ? (int32_t)((uint32_t)p->userdata[0] -
                    (uint32_t)gGlobalFrameRate)
        : 0;
    if ((pNodeTurret->flags &
         JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) == 0) {
        pNodeTurret->flags |=
            JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY;
    }
    return -1;
}

/* 0x104A90, 153 bytes, global, 4 named locals
 * ai_Blades
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\vehicle.c
 */
int ai_Blades(int32_t *cpad, playerObject *player)
{
    Mnode *pelvis = coll_GetNode(player->playerRoot.objectID, 0);

    (void)cpad;
    if (pelvis != NULL) {
        sceneObject *scene =
            (sceneObject *)player->playerRoot.pParent;
        physicsObject *physics =
            (physicsObject *)scene->pPhysics;

        if ((pelvis->flags &
             JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY) == 0) {
            pelvis->flags |=
                JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY;
        }
        pelvis->v3RotationAbs.vy = 0;
        physics->angle.vy = (int32_t)(
            (uint32_t)physics->angle.vy +
            (uint32_t)flexmul12(
                0xc0 - physics->userdata[0],
                gGlobalFrameRate));
        if (physics->userdata[0] != 0) {
            physics->userdata[0] = (int32_t)(
                (uint32_t)physics->userdata[0] +
                (uint32_t)flexmul12(2, gGlobalFrameRate));
            if (physics->userdata[0] > 0xc0) {
                physics->userdata[0] = 0xc0;
            }
        }
    }
    return 0;
}

/* 0x104B30, 2034 bytes, global, 25 named locals
 * ai_Stap
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\vehicle.c
 */
int ai_Stap(int32_t *cpad, playerObject *player)
{
    physicsObject *p = (physicsObject *)(
        (sceneObject *)player->playerRoot.pParent)->pPhysics;
    uint32_t pad = (uint32_t)cpad[1];
    int sitter = player->playernum + 1;
    int index = 0;
    playerObject *controller;
    float xMultiplier;
    float yMultiplier;
    int diff = 0;
    Mnode *stap;
    Mnode *jedi;
    int baseSpeed;
    int playerSpeedAdjustment;

    if (stapZeroBSSCheck != zerobss_levelReset) {
        stapZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        stapAngle[0] = (_svector){0};
        stapAngle[1] = (_svector){0};
    }
    if ((jpb_CubeRuntimeFlags & UINT32_C(8)) != 0) {
        _svector cube;

        cube.vx = (int16_t)((0x80ff - (int)p->pos.vx) >> 8);
        cube.vy = (int16_t)((int)p->pos.vy >> 8);
        cube.vz = (int16_t)(((int)p->pos.vz + 0x7f00) >> 8);
        cube.pad = 0;
        debug_printf("CUBX: %d\n", cube.vz);
        if (stapsound != 0) {
            sound_SetLoopingFadeTime(
                stapsound, gGlobalTimer + UINT32_C(0x2800));
            stapsound = 0;
        }
        debug_printf("STREETS ENDING!\n");
        pad = 0;
    }

    if (stapbikeindex[0] != sitter) {
        if (stapbikeindex[1] == sitter) {
            index = 1;
        } else {
            debug_printf("Who's stapping the stap?\n");
        }
    }
    controller = &gaPlayerData[index];
    if ((GameStruct.GameState & UINT32_C(0x02000000)) == 0 &&
        (jpb_CubeRuntimeFlags & UINT32_C(8)) == 0 && stapsound == 0) {
        stapsound = sound_Play(&p->vpos, 3, "stapstdy", 0);
    }
    camera_SetCurrentCameraType(5);
    p->flags |= UINT32_C(0x00400000);

    if (stapShotZeroBSSCheck != zerobss_levelReset) {
        stapShotZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        stapShot[0] = 0;
        stapShot[1] = 0;
    }
    if ((pad & UINT32_C(0x10)) != 0 &&
        stapShot[index] + UINT32_C(0x1000) < gGlobalTimer) {
        Projectile *proj1 = bullet_AllocProjectile(0);
        Projectile *proj2 = bullet_AllocProjectile(0);

        if (proj1 != NULL && proj2 != NULL) {
            VECTOR *pos0 = coll_GetNodeCenter(player->playernum, 5);
            VECTOR *pos1 = coll_GetNodeCenter(player->playernum, 8);
            VECTOR *pos2 = coll_GetNodeCenter(player->playernum, 6);
            VECTOR *pos3 = coll_GetNodeCenter(player->playernum, 9);
            playerObject *target = FindBestMachineGunTarget(
                pos0, &p->angle, player, 0x500, 0xc8, 0x100, 0);
            _svector crapness = {0, 0, 0, (int16_t)0x5432};
            VECTOR *targetPosition = target != NULL
                ? physics_gGetPosition(&target->playerRoot)
                : pos1;

            bullet_ShootProjectile(
                proj1, controller, pos0, targetPosition, &crapness);
            bullet_ShootProjectile(
                proj2, controller, pos2,
                target != NULL ? targetPosition : pos3, &crapness);
            stapShot[index] = gGlobalTimer;
        } else {
            if (proj1 != NULL) {
                bullet_FreeProjectile(proj1);
            }
            if (proj2 != NULL) {
                bullet_FreeProjectile(proj2);
            }
        }
    }

    xMultiplier = index == 0
        ? (g_p1X > -1.0f ? g_p1X : -1.0f)
        : (g_p2X > -1.0f ? g_p2X : -1.0f);
    yMultiplier = index == 0
        ? (g_p1Y > -1.0f ? g_p1Y : -1.0f)
        : (g_p2Y > -1.0f ? g_p2Y : -1.0f);
    xMultiplier = xMultiplier < 1.0f ? xMultiplier : 1.0f;
    yMultiplier = yMultiplier < 1.0f ? yMultiplier : 1.0f;
    xMultiplier += xMultiplier < 0.0f ? 0.35f : -0.35f;

    if ((pad & UINT32_C(0x8000)) != 0) {
        float limit = 3072.0f - xMultiplier / 0.65f * 128.0f;

        p->angle.vy = (int)((float)p->angle.vy + 32.0f);
        if ((float)p->angle.vy > limit) {
            p->angle.vy = (int)limit;
        }
        stapAngle[index].vz = (int16_t)vehicle_clamp(
            stapAngle[index].vz - 8, -0x40, 0x40);
    } else if ((pad & UINT32_C(0x2000)) != 0) {
        float limit = 3072.0f - xMultiplier / 0.65f * 128.0f;

        p->angle.vy = (int)((float)p->angle.vy - 32.0f);
        if ((float)p->angle.vy < limit) {
            p->angle.vy = (int)limit;
        }
        stapAngle[index].vz = (int16_t)vehicle_clamp(
            stapAngle[index].vz + 8, -0x40, 0x40);
    } else {
        int a = vehicle_signed_angle(p->angle.vy + 0x400);

        p->angle.vy += a < -0x20
            ? 0x10
            : a > 0x20 ? -0x10 : 0xc00 - p->angle.vy;
        stapAngle[index].vz = (int16_t)vehicle_clamp(
            stapAngle[index].vz, -0x10, 0x10);
        if (stapAngle[index].vz > 0) {
            stapAngle[index].vz = (int16_t)vehicle_clamp(
                stapAngle[index].vz - 0x10, 0, 0x10);
        } else if (stapAngle[index].vz < 0) {
            stapAngle[index].vz = (int16_t)vehicle_clamp(
                stapAngle[index].vz + 0x10, -0x10, 0);
        }
    }

    if ((pad & UINT32_C(0x1000)) != 0) {
        int limit = (int)(150.0f - yMultiplier * 250.0f);
        stapAngle[index].vx = (int16_t)vehicle_clamp(
            stapAngle[index].vx + 0x20, INT16_MIN, limit);
    } else if ((pad & UINT32_C(0x4000)) != 0) {
        int limit = (int)(150.0f - yMultiplier * 250.0f);
        stapAngle[index].vx = (int16_t)vehicle_clamp(
            stapAngle[index].vx - 0x14, limit, INT16_MAX);
    } else if (stapAngle[index].vx > 0) {
        stapAngle[index].vx = (int16_t)vehicle_clamp(
            stapAngle[index].vx - 0x10, 0, INT16_MAX);
    } else if (stapAngle[index].vx < 0) {
        stapAngle[index].vx = (int16_t)vehicle_clamp(
            stapAngle[index].vx + 0x10, INT16_MIN, 0);
    }

    if (GameStruct.NumPlayers == 2) {
        int other = index == 0 ? 1 : 0;

        if (stapbikeindex[other] != 0) {
            diff = (int)(
                maPhysicsData[stapbikeindex[other] - 1].pos.vx - p->pos.vx);
        }
    }
    if ((pad & UINT32_C(0x40)) != 0) {
        p->userdata[1] += flexmul12(4, gGlobalFrameRate);
    }
    if ((pad & UINT32_C(0x80)) != 0) {
        p->userdata[1] -= flexmul12(8, gGlobalFrameRate);
    }
    p->userdata[1] -= flexmul12(diff >> 7, gGlobalFrameRate);
    p->userdata[1] = vehicle_clamp(p->userdata[1], -0x20, 0x40);
    if (p->userdata[2] < 0x1000) {
        p->userdata[2] += flexmul12(0x80, gGlobalFrameRate);
        if (p->userdata[2] > 0x1000) {
            p->userdata[2] = 0x1000;
        }
    }
    p->userdata[0] = 0xa0 - ((int)stapAngle[index].vx >> 1);

    stap = coll_GetNode(player->playernum, 0);
    jedi = coll_GetNode(index, 0);
    if (stap != NULL) {
        stap->v3RotationAbs = stapAngle[index];
        vehicle_mark_rotation(stap);
    }
    if (jedi != NULL) {
        jedi->v3RotationAbs.vx = (int16_t)(stapAngle[index].vx - 0x100);
        jedi->v3RotationAbs.vy = stapAngle[index].vy;
        jedi->v3RotationAbs.vz = stapAngle[index].vz;
        vehicle_mark_rotation(jedi);
    }
    baseSpeed = GameStruct.difficulty != 0 ? 0x68 : 0x51;
    playerSpeedAdjustment =
        GameStruct.difficulty != 0 ? 0x20 : 0x18;
    p->constmov.vz = (float)flexmul(
        p->userdata[1] - (GameStruct.NumPlayers - 1) *
        playerSpeedAdjustment + baseSpeed,
        p->userdata[2]);
    return -1;
}

/* 0x105330, 3887 bytes, global, 67 named locals
 * ai_Tank
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\vehicle.c
 */
int ai_Tank(int32_t *cpad, playerObject *player)
{
    wsl_ENEMY *pEnemy = player->pEnemy;
    physicsObject *p = (physicsObject *)(
        (sceneObject *)player->playerRoot.pParent)->pPhysics;
    uint32_t activeFlags = pEnemy != NULL && pEnemy->pPlace != NULL
        ? pEnemy->pPlace->aiDf.activeFlags & UINT32_C(0x0c000000)
        : 0;
    playerObject *primary;
    playerObject *secondary;
    uint32_t contpad0;
    uint32_t contpad1;
    Mnode *pNodeLeft = coll_GetNode(player->playernum, 7);
    Mnode *pNodeRight = coll_GetNode(player->playernum, 8);
    Mnode *pNodeTurret = coll_GetNode(player->playernum, 9);
    int numplayers;
    int angle = physics_gGetFacing(&player->playerRoot);
    int i;

    (void)cpad;
    if (tankZeroBSSCheck != zerobss_levelReset) {
        tankZeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        tankSpeed = 0;
        machinegunfacing = 0;
        tankTarget[0] = NULL;
        tankTarget[1] = NULL;
        tankElapsed = 0;
        tankFireSequence = 0;
        tankFireElapsed = 0;
        tankWeirdFireElapsed = 0;
    }
    p->trajectory = 0x7ffe;
    if (tanknoise == 0) {
        (void)sound_Play(&p->vpos, 3, "tankstrt", 0);
        tanknoise = sound_Play(&p->vpos, 3, "tanksty1", 0);
    } else {
        sound_SetFrequency(
            tanknoise,
            (uint32_t)(vehicle_emitted_abs(tankSpeed) * 4 + 0x1000));
    }
    playertankindex = player->playernum + 1;
    tankElapsed += gGlobalFrameRate;
    if (activeFlags == 0) {
        playertankindex = 0;
        return 1;
    }

    primary = tankdrivers[0];
    secondary = activeFlags == UINT32_C(0x0c000000)
        ? tankdrivers[1]
        : tankdrivers[0];
    numplayers = activeFlags == UINT32_C(0x0c000000) ? 2 : 1;
    for (i = 0; i < numplayers; ++i) {
        playerObject *driver = tankdrivers[i];

        if (driver == NULL) {
            continue;
        }
        driver->pFlags &= ~UINT32_C(1);
        if ((driver->playerPad.cpad[1] & UINT32_C(0x20)) == 0) {
            if (jumpheld[i] == 1) {
                jumpheld[i] = 0;
            }
            continue;
        }
        if (jumpheld[i] == 0) {
            sceneObject *driverScene =
                (sceneObject *)driver->playerRoot.pParent;
            physicsObject *driverPhysics =
                (physicsObject *)driverScene->pPhysics;
            modelObject *driverModel =
                (modelObject *)driverScene->pModel;
            VECTOR *hatch = coll_GetNodeCenter(player->playernum, 5);
            int x = rsin(driverPhysics->angle.vy) >> 5;
            int z = rcos(driverPhysics->angle.vy) >> 5;

            if (pEnemy != NULL && pEnemy->pPlace != NULL) {
                pEnemy->pPlace->aiDf.activeFlags &=
                    ~(UINT32_C(1) << (driver->playernum + 0x1a));
            }
            timesincetank[driver->playernum] = 0x1e000;
            driver->pFlags &= ~UINT32_C(0x80);
            if (driverModel != NULL) {
                driverModel->flags &= ~UINT32_C(0x10);
            }
            driverPhysics->flags &= UINT32_C(0x40);
            driver->playerPad.cpad[0] = 0;
            if (hatch != NULL) {
                physics_gSetPosition(
                    &driverPhysics->physicsRoot,
                    hatch->vx + driver->playernum * 100 + x,
                    hatch->vy + 0x80,
                    hatch->vz + driver->playernum * 100 + z);
            }
            tankdrivers[i] = NULL;
            jumpheld[i] = 1;
            player->pFlags &= ~UINT32_C(0x40);
            if (player->paMotions != NULL) {
                ((uint8_t *)player->paMotions)[0x7c] = 0;
            }
        }
    }
    if (tankdrivers[1] == NULL) {
        secondary = tankdrivers[0];
        if (tankdrivers[0] == NULL) {
            tankElapsed = 0;
            tankFireSequence = 0;
            tankFireElapsed = 0;
            p->constmov.vz = 0.0f;
            playertankindex = 0;
            tankWeirdFireElapsed = 0;
            if (tanknoise != 0) {
                sound_StopSound(tanknoise);
                (void)sound_Play(&p->vpos, 3, "tankstop", 0);
                tanknoise = 0;
            }
            return 1;
        }
    } else if (tankdrivers[0] == NULL) {
        primary = tankdrivers[1];
        secondary = tankdrivers[1];
        tankdrivers[0] = tankdrivers[1];
        tankdrivers[1] = NULL;
    }

    contpad0 = primary->playerPad.cpad[1];
    contpad1 = secondary->playerPad.cpad[1];
    if ((contpad1 & UINT32_C(0x10)) != 0 && p->userdata[0] == 0) {
        VECTOR *pos0 = coll_GetNodeCenter(player->playernum, 12);
        VECTOR *pos1 = coll_GetNodeCenter(player->playernum, 18);

        p->userdata[0] = 0x800;
        camera_SetShake(4);
        if (pos0 != NULL && pos1 != NULL) {
            VECTOR aim = {
                pos0->vx + flexmul(pos1->vx - pos0->vx, 0x898),
                pos0->vy + flexmul(pos1->vy - pos0->vy, 0x898),
                pos0->vz + flexmul(pos1->vz - pos0->vz, 0x898),
                0
            };
            vehicle_tank_fire(2, secondary, &aim, pos1, &p->svmov);
            if (primary != secondary) {
                vehicle_tank_fire(2, primary, &aim, pos1, &p->svmov);
            }
        }
    }

    if ((contpad1 & UINT32_C(0x80)) != 0) {
        tankFireElapsed -= gGlobalFrameRate;
        if (tankFireElapsed < 1) {
            int side;

            tankFireElapsed = 0xc000;
            for (side = 0; side < 2; ++side) {
                int baseNode = side == 0 ? 8 : 7;
                int muzzleNode = side == 0 ? 19 : 20;
                VECTOR *base = coll_GetNodeCenter(
                    player->playernum, baseNode);
                VECTOR *muzzle = coll_GetNodeCenter(
                    player->playernum, muzzleNode);

                if (base != NULL && muzzle != NULL) {
                    VECTOR tpos = {
                        muzzle->vx * 2 - base->vx + (rand() & 7) - 4,
                        muzzle->vy * 2 - base->vy + (rand() & 7) - 4,
                        muzzle->vz * 2 - base->vz + (rand() & 7) - 4,
                        0
                    };
                    vehicle_tank_fire(
                        3, player, muzzle, &tpos, &p->svmov);
                }
            }
        }
    }

    if (tankElapsed > 0x1fff) {
        int side = tankWhich;
        int muzzleNode = side == 0 ? 19 : 20;
        VECTOR *muzzle = coll_GetNodeCenter(player->playernum, muzzleNode);

        if (muzzle != NULL) {
            tankTarget[side] = FindBestMachineGunTarget(
                muzzle, &p->angle, player, 12, 0x200, 0x100, 0);
        }
        tankWhich = 1 - tankWhich;
        tankElapsed = 0;
    }
    vehicle_tank_aim_gun(
        pNodeRight,
        coll_GetNodeCenter(player->playernum, 8),
        coll_GetNodeCenter(player->playernum, 19),
        tankTarget[0]);
    vehicle_tank_aim_gun(
        pNodeLeft,
        coll_GetNodeCenter(player->playernum, 7),
        coll_GetNodeCenter(player->playernum, 20),
        tankTarget[1]);
    if (pNodeLeft != NULL) {
        pNodeLeft->v3RotationAbs.vx &= 0x0fff;
        pNodeLeft->v3RotationAbs.vy &= 0x0fff;
    }
    if (pNodeRight != NULL) {
        pNodeRight->v3RotationAbs.vx &= 0x0fff;
        pNodeRight->v3RotationAbs.vy &= 0x0fff;
    }

    if ((contpad1 & UINT32_C(0x40)) != 0) {
        tankWeirdFireElapsed -= gGlobalFrameRate;
        if (tankWeirdFireElapsed < 1) {
            int side;

            tankWeirdFireElapsed = 0x4000;
            for (side = 0; side < 2; ++side) {
                int node = (side == 0 ? 21 : 24) + rand() % 3;
                VECTOR *pos0 = coll_GetNodeCenter(player->playernum, node);

                if (pos0 != NULL) {
                    VECTOR knob = {
                        pos0->vx + rsin(angle),
                        pos0->vy,
                        pos0->vz + rcos(angle),
                        0
                    };
                    vehicle_tank_fire(
                        4, secondary, pos0, &knob, &p->svmov);
                }
            }
        }
    }

    if ((contpad0 & UINT32_C(0xf000)) == 0) {
        int deceleration = flexmul12(4, gGlobalFrameRate);

        if (tankSpeed - deceleration < 1) {
            tankSpeed = 0;
            if (player->paMotions != NULL) {
                (void)animctrl_MotionLockLevel(
                    &player->playerRoot, player->paMotions, 0x16);
            }
        } else {
            tankSpeed -= deceleration;
            if (player->paMotions != NULL) {
                (void)animctrl_MotionLock(
                    &player->playerRoot, &player->paMotions[1]);
            }
        }
    } else {
        int turn = 0;
        int throttle = 0;
        int limit = (contpad0 & UINT32_C(2)) != 0 ? 0x40 : 0x20;

        if ((contpad0 & UINT32_C(0x1000)) != 0) {
            turn = 1;
            throttle = 3;
        } else if ((contpad0 & UINT32_C(0x4000)) != 0) {
            turn = -1;
            throttle = 3;
        }
        if ((contpad0 & UINT32_C(0x2000)) != 0) {
            angle -= flexmul12(0x18, gGlobalFrameRate);
            throttle = 2;
        } else if ((contpad0 & UINT32_C(0x8000)) != 0) {
            angle += flexmul12(0x18, gGlobalFrameRate);
            throttle = 2;
        }
        if (throttle != 0) {
            tankSpeed += flexmul12(throttle, gGlobalFrameRate) * turn;
            tankSpeed = vehicle_clamp(tankSpeed, -limit, limit);
        }
        if (player->paMotions != NULL) {
            (void)animctrl_MotionLock(
                &player->playerRoot, &player->paMotions[1]);
        }
        machinegunfacing = vehicle_signed_angle(pNodeLeft != NULL
            ? (int)pNodeLeft->v3RotationAbs.vy - angle
            : -angle);
        p->angle.vy = angle;
    }

    if (tankSpeed < 9) {
        player->pFlags &= ~UINT32_C(0x40);
        if (player->paMotions != NULL) {
            ((uint8_t *)player->paMotions)[0x7c] = 0;
        }
    } else {
        player->pFlags |= UINT32_C(0x40);
        if (player->paMotions != NULL) {
            uint8_t *motionBytes = (uint8_t *)player->paMotions;
            motionBytes[0x7c] = (uint8_t)(tankSpeed / 4);
            motionBytes[0x92] = 1;
            motionBytes[0x7d] = 8;
            motionBytes[0x7e] = 0x28;
        }
    }

    if ((contpad1 & UINT32_C(0x0c)) == 0 ||
        (contpad1 & UINT32_C(0x0c)) == UINT32_C(0x0c)) {
        if (turretnoise != 0) {
            sound_StopSound(turretnoise);
            turretnoise = 0;
        }
    } else {
        int turretStep = (contpad1 & UINT32_C(8)) != 0 ? -0x20 : 0x20;

        if (turretnoise == 0) {
            turretnoise = sound_Play(&p->vpos, 3, "td_rotat", 0);
        }
        if (pNodeTurret != NULL) {
            pNodeTurret->v3RotationAbs.vy = (int16_t)(
                ((uint16_t)pNodeTurret->v3RotationAbs.vy +
                 (uint16_t)flexmul12(turretStep, gGlobalFrameRate)) &
                UINT16_C(0x0fff));
        }
    }

    p->constmov.vz = (float)(tankSpeed * 2);
    if (p->userdata[0] != 0) {
        p->userdata[0] -= flexmul12(0x40, gGlobalFrameRate);
        if (p->userdata[0] < 0) {
            p->userdata[0] = 0;
        }
    }
    if (pNodeLeft != NULL) {
        pNodeLeft->flags |= UINT32_C(0x20000000);
    }
    if (pNodeRight != NULL) {
        pNodeRight->flags |= UINT32_C(0x20000000);
    }
    if (pNodeTurret != NULL) {
        pNodeTurret->flags |= UINT32_C(0x20000000);
    }
    if ((activeFlags & UINT32_C(0x04000000)) != 0 &&
        tankdrivers[0] != NULL) {
        tankdrivers[0]->pFlags |= UINT32_C(0x80);
    }
    if ((activeFlags & UINT32_C(0x08000000)) != 0 &&
        tankdrivers[1] != NULL) {
        tankdrivers[1]->pFlags |= UINT32_C(0x80);
    }
    return -1;
}

/* 0x106260, 64 bytes, global, 2 named locals
 * centreturret
 * PDB type: void (Mnode*)
 * Source: W:\SWJediPowerBattles\Work\vehicle.c
 */
void centreturret(Mnode *pNodeTurret)
{
    int32_t reaim = vehicle_signed_angle(
        0 - (int32_t)pNodeTurret->v3RotationAbs.vy);

    if (reaim < -8) {
        reaim = -8;
    } else if (reaim > 8) {
        reaim = 8;
    }
    pNodeTurret->v3RotationAbs.vy = (int16_t)(
        (uint16_t)pNodeTurret->v3RotationAbs.vy +
        (uint16_t)flexmul12(reaim, gGlobalFrameRate));
}
