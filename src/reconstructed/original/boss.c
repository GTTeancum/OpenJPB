/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\boss.c.
 * PDB module: 0007
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\boss.obj
 * Primary source: W:\SWJediPowerBattles\Work\boss.c
 * Compiler language: c
 * Emitted procedures: 18
 *
 * All 18 emitted procedures are reviewed against the matched executable.
 * Function-local statics retain their exact PDB names; repeated names remain
 * scoped to their original owners.
 */

#include "jpb/boss.h"

#include "jpb/ai.h"
#include "jpb/alloc.h"
#include "jpb/anim.h"
#include "jpb/animctrl.h"
#include "jpb/animutil.h"
#include "jpb/brain.h"
#include "jpb/brainutl.h"
#include "jpb/bullet.h"
#include "jpb/collision.h"
#include "jpb/debugtext.h"
#include "jpb/effects.h"
#include "jpb/game.h"
#include "jpb/flex.h"
#include "jpb/fx.h"
#include "jpb/intersec.h"
#include "jpb/linkstubs.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/vectors.h"
#include "jpb/whook.h"
#include "jpb/wrender.h"
#include "jpb/world.h"

#include <stdlib.h>
#include <string.h>

/*
 * Exact file-local PDB globals at matched-PC RVAs
 * 0x4F1294 and 0x4F129C..0x4F12B0. The two PDB-named `count` records and
 * repeated `zeroBSSCheck` records are function-local statics.
 */
static int MODE;
static int box;
static int arms[2];
static int toss;

static void boss_mount_KaduRider(
    playerObject *kadu,
    playerObject *rider,
    int rider_index,
    int playerOffsetY)
{
    sceneObject *rider_scene =
        (sceneObject *)rider->playerRoot.pParent;
    physicsObject *rider_physics =
        (physicsObject *)rider_scene->pPhysics;
    FVECTOR *playerPos = &rider_physics->pos;
    VECTOR *pos;
    Mnode *lLowerArm;
    Mnode *rLowerArm;
    _svector lrot = {0, 0x076c, 0x0bd6, 0};
    _svector rrot = {0, -0x076c, -0x0bd6, 0};

    (void)animctrl_MotionNoLock(
        &maPhysicsData[rider_index].physicsRoot,
        &rider->paMotions[78]);
    rider->paMotions[78].motionFlags |=
        UINT32_C(0xc0000000);
    if ((rider_physics->flags & UINT32_C(0x80)) == 0) {
        rider_physics->flags |= UINT32_C(0x80);
    }

    pos = coll_GetNodeCenter(
        kadu->playerRoot.objectID, 0);
    playerPos->vx = (float)pos->vx;
    playerPos->vy = (float)pos->vy;
    playerPos->vz = (float)pos->vz;
    physics_gSetPosition(
        &rider->playerRoot,
        pos->vx,
        (int)((float)pos->vy + 20.0f),
        (int)((float)pos->vz + (float)playerOffsetY));
    physics_gSetFacing(
        &rider->playerRoot,
        physics_gGetFacing(&kadu->playerRoot));

    lLowerArm = coll_GetNode(rider_index, 10);
    rLowerArm = coll_GetNode(rider_index, 14);
    lLowerArm->v3RotationAbs = lrot;
    rLowerArm->v3RotationAbs = rrot;
    lLowerArm->flags |= UINT32_C(0x20000000);
    rLowerArm->flags |= UINT32_C(0x20000000);
}

/* 0x19CE0, 25 bytes, global, 2 named locals
 * ai_Deadly
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */

/* 0x19D00, 6 bytes, global, 2 named locals
 * ai_Destroyer
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int ai_Deadly(int32_t *cpad, playerObject *player)
{
    (void)cpad;
    if ((player->forceFlags & UINT32_C(0x40)) == 0) {
        player->forceFlags |= UINT32_C(0x40);
    }
    return 1;
}
int ai_Destroyer(
    int32_t *cpad, playerObject *player)
{
    (void)cpad;
    (void)player;
    return -1;
}

/* 0x19D10, 448 bytes, global, 5 named locals
 * ai_JarJar
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int ai_JarJar(int32_t *cpad, playerObject *player)
{
    sceneObject *scene = (sceneObject *)player->playerRoot.pParent;
    physicsObject *p = (physicsObject *)scene->pPhysics;

    (void)cpad;
    if (GameStruct.CurrentLevel == UINT8_C(13)) {
        if (player->fStun != 0 && player->target != NULL) {
            int target_id;

            abGlobalBits[6] &= UINT8_C(0xf3);
            if (player->target->playernum > 1) {
                abGlobalBits[6] |= UINT8_C(8);
                return -1;
            }
            target_id = player->target->playerRoot.objectID;
            abGlobalBits[(target_id + 50) >> 3] |= (uint8_t)(
                UINT8_C(1) << ((target_id + 2) & 7));
        }
        return -1;
    }

    gJarJarPos.vx = (int16_t)(int32_t)p->pos.vx;
    gJarJarPos.vy = (int16_t)(int32_t)p->pos.vy;
    gJarJarPos.vz = (int16_t)(int32_t)p->pos.vz;
    if ((abGlobalBits[4] & UINT8_C(1)) != 0) {
        camera_SetCurrentCameraType(6);
    }
    if ((abGlobalBits[4] & UINT8_C(2)) == 0) {
        if (gpWorld->player0->playerRoot.objectID != -1 &&
            obj_gCheckObjectFlag(
                &gpWorld->player0->playerRoot, 0, 0x20) == 0 &&
            (gpWorld->player0->pFlags & UINT32_C(0x00040200)) == 0) {
            return -1;
        }
        if (gpWorld->player1->playerRoot.objectID != -1 &&
            obj_gCheckObjectFlag(
                &gpWorld->player1->playerRoot, 0, 0x20) == 0 &&
            (gpWorld->player1->pFlags & UINT32_C(0x00040200)) == 0) {
            return -1;
        }
    }
    if (GameStruct.NumPlayers == 2 &&
        obj_gCheckObjectFlag(
            &gpWorld->player1->playerRoot, 0, 0x20) == 0) {
        int player_zero_dead = obj_gCheckObjectFlag(
            &gpWorld->player0->playerRoot, 0, 0x20);

        camera_SetCurrentCameraType(player_zero_dead != 0 ? 2 : 0);
        return 1;
    }
    camera_SetCurrentCameraType(1);
    return 1;
}

/* 0x19ED0, 1977 bytes, global, 39 named locals
 * ai_Kadu
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int ai_Kadu(int32_t *dummy, playerObject *player)
{
    static uint32_t keyh[2];
    static int speedh[2];
    static int slowh[2];
    static uint32_t lasth[2];
    static int zeroBSSCheck;
    Motion *motion = *player->pMotion;
    wsl_ENEMY *pEnemy = player->pEnemy;
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *kaaduPhysicsObj =
        (physicsObject *)scene->pPhysics;
    animObject *animation =
        (animObject *)scene->pAnim;
    playerObject *rider;
    physicsObject *player0Physics;
    physicsObject *player1Physics;
    uint32_t events;
    uint32_t key;
    uint32_t last;
    int playerOffsetY;
    int maxSpeed;
    int speed;
    int slow;
    int mod = 0;
    int id;

    (void)dummy;
    if (zeroBSSCheck != zerobss_levelReset ||
        zerobss_ResetBoss != 0) {
        zerobss_ResetBoss = 0;
        zeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        keyh[0] = 1;
        speedh[0] = 1;
        slowh[0] = 1;
        lasth[0] = 1;
    }

    pEnemy->pPlace->aiDf.daRange = 0;
    if (GameStruct.CurrentLevel != UINT8_C(12)) {
        return 1;
    }

    if ((gpWorld->player0->pFlags & UINT32_C(0x80)) == 0) {
        gpWorld->player0->pFlags |= UINT32_C(0x80);
    }
    if ((gpWorld->player1->pFlags & UINT32_C(0x80)) == 0) {
        gpWorld->player1->pFlags |= UINT32_C(0x80);
    }

    kaaduPhysicsObj->angle.vx = 300;
    playerOffsetY = motion->Seq == 0 ? -0x50 : -0x70;
    if (kaaduPhysicsObj->userdata[0] > 1 &&
        (motion->Seq == 1 || motion->Seq == 2)) {
        if ((kaaduPhysicsObj->flags & UINT32_C(0x00400000)) == 0) {
            kaaduPhysicsObj->flags |= UINT32_C(0x00400000);
        }
        kaaduPhysicsObj->angle.vy = 0;
        playerOffsetY = 0x70;
        kaaduPhysicsObj->angle.vx = -300;
    }

    if (player->playernum == 2) {
        boss_mount_KaduRider(
            player, gpWorld->player0, 0, playerOffsetY);
    }
    if (player->playernum == 3) {
        boss_mount_KaduRider(
            player, gpWorld->player1, 1, playerOffsetY);
    }

    player0Physics = (physicsObject *)(
        (sceneObject *)gpWorld->player0->playerRoot.pParent)->pPhysics;
    player1Physics = (physicsObject *)(
        (sceneObject *)gpWorld->player1->playerRoot.pParent)->pPhysics;
    gJarJarPos.vx = (int16_t)(int32_t)player1Physics->pos.vx;
    if (player1Physics->pos.vz <= player0Physics->pos.vz) {
        gJarJarPos.vy =
            (int16_t)(int32_t)maPhysicsData[3].pos.vy;
        gJarJarPos.vz =
            (int16_t)(int32_t)player1Physics->pos.vz;
    } else {
        gJarJarPos.vy =
            (int16_t)(int32_t)maPhysicsData[2].pos.vy;
        gJarJarPos.vz =
            (int16_t)(int32_t)player0Physics->pos.vz;
    }
    camera_SetCurrentCameraType(6);

    id = player->playernum != 2;
    rider = id == 0 ? gpWorld->player0 : gpWorld->player1;
    last = lasth[id];
    slow = slowh[id];
    speed = speedh[id];
    key = keyh[id];

    if (motion->Seq == 1 || motion->Seq == 2) {
        kaaduPhysicsObj->constmov.vz = (float)speed;
        animation->animFrameRate = speed << 7;
    } else {
        ++kaaduPhysicsObj->userdata[0];
    }

    events = rider->playerPad.cpad[0];
    if (key == 0) {
        key = last;
    }
    if (OptionStruct.ControllerConfig[id] == 0) {
        events &= UINT32_C(0xa0);
        if (((events & UINT32_C(0x80)) != 0 &&
             (last & UINT32_C(0x20)) != 0) ||
            ((events & UINT32_C(0x20)) != 0 &&
             (last & UINT32_C(0x80)) != 0)) {
            mod = 4;
        }
    } else {
        events &= UINT32_C(0x90);
        if (((events & UINT32_C(0x80)) != 0 &&
             (last & UINT32_C(0x10)) != 0) ||
            ((events & UINT32_C(0x10)) != 0 &&
             (last & UINT32_C(0x80)) != 0)) {
            mod = 4;
        }
    }

    maxSpeed = 0x50;
    if (GameStruct.NumPlayers == 1 && id == 1) {
        mod = rand() % 3;
        maxSpeed = 0x46;
    }
    if (mod == 0) {
        if (slow > 12) {
            speed -= 8;
            slow = 0;
        }
    } else {
        speed += mod;
    }
    if (speed < 0x10) {
        speed = 0x10;
    } else if (speed > maxSpeed) {
        speed = maxSpeed;
    }

    if (key != 0) {
        lasth[id] = key;
    }
    slowh[id] = slow + 1;
    speedh[id] = speed;
    keyh[id] = events;

    if (game_gIsGameFlags(UINT32_C(0x02000000)) == 0) {
        int speedScaled = speed * 4;
        float x;
        float y;
        float width;
        float height;

        if (id == 0) {
            x = -200.0f;
            y = 40.0f;
            width = 300.0f;
            height = 12.0f;
            setPivotPositionAndFixScale(
                &x, &y, &width, &height, 7);
            _AddBar(
                (int)x,
                (int)y,
                speedScaled,
                (int)height,
                INT32_C(0x00ff4010));
        } else if (id == 1) {
            x = 200.0f;
            y = 40.0f;
            width = 300.0f;
            height = 12.0f;
            setPivotPositionAndFixScale(
                &x, &y, &width, &height, 7);
            x += width - (float)speedScaled;
            _AddBar(
                (int)x,
                (int)y,
                speedScaled,
                (int)height,
                INT32_C(0x001040ff));
        }
    }
    return -1;
}

/* 0x1A690, 51 bytes, global, 2 named locals
 * ai_Krakis
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */

/* 0x1A6D0, 1143 bytes, global, 9 named locals
 * ai_LoaderDroid
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int ai_Krakis(int32_t *cpad, playerObject *player)
{
    ai_ShowFlags(player);
    (void)ai_Sphere(cpad, player);
    return -1;
}
int ai_LoaderDroid(
    int32_t *cpad, playerObject *player)
{
    /* Exact function-local PDB static at matched-PC RVA 0x4F12A8. */
    static int count[2];
    /* Exact function-local PDB static at matched-PC RVA 0x4F12B4. */
    static int zeroBSSCheck;
    EffectHeader *effect;
    sceneObject *scene;
    animObject *animation;

    (void)cpad;
    (void)coll_GetNode(player->playernum, 0);
    if (zeroBSSCheck != zerobss_levelReset ||
        zerobss_ResetBoss != 0) {
        zerobss_ResetBoss = 0;
        zeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        arms[0] = 0;
        arms[1] = 0;
        count[0] = 0;
        count[1] = 0;
        box = 0;
        toss = 0;
    }

    ai_ShowFlags(player);
    effect = paEffects[2];
    if (arms[0] == 1) {
        ++count[0];
        if (count[0] > 12) {
            count[0] = 0;
            (void)sprite_AddSpriteEffectAtNode(
                effect->aEffects,
                (int)effect->num,
                player->playernum,
                12);
        }
    } else {
        coll_SetNodeFlags(
            player->playernum, 12, 4);
    }
    if (arms[1] == 1) {
        ++count[1];
        if (count[1] > 12) {
            count[1] = 0;
            (void)sprite_AddSpriteEffectAtNode(
                effect->aEffects,
                (int)effect->num,
                player->playernum,
                11);
        }
    } else {
        coll_SetNodeFlags(
            player->playernum, 11, 4);
    }

    scene = (sceneObject *)player->playerRoot.pParent;
    animation = (animObject *)scene->pAnim;
    if (player->currentMotion == 0x49) {
        VECTOR *pos = coll_GetNodeCenter(
            player->playerRoot.objectID, 4);
        int frame = animation->animFrameIndex >> 12;

        ai_SetTarget(player, 1);
        if (frame > 10) {
            if (frame < 31) {
                physics_gSetPosition(
                    &player->target->playerRoot,
                    pos->vx,
                    pos->vy,
                    pos->vz);
                player->target->pFlags |=
                    UINT32_C(0x00000001);
                player->target->pFlags |=
                    UINT32_C(0x00000800);
                (void)physics_gForceFaceTarget(
                    &player->target->playerRoot,
                    &player->playerRoot);
                if (player->target->currentMotion !=
                    0x33) {
                    (void)animctrl_MotionLock(
                        &player->target->playerRoot,
                        &player->target->paMotions[51]);
                    toss = 1;
                }
            } else if (toss != 0) {
                (void)animctrl_MotionLock(
                    &player->target->playerRoot,
                    &player->target->paMotions[51]);
                (void)brain_ThrowEnder(
                    NULL, player->target);
                toss = 0;
            }
        }
    }

    if (player->currentMotion == 0x4b &&
        arms[0] == 0) {
        Mnode *node = coll_GetNode(
            player->playerRoot.objectID, 2);

        arms[0] = 1;
        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            2);
        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            3);
        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            4);
        node->flags |= UINT32_C(0x04000000);
        node->v3Translation2.vx =
            (int16_t)node->v3RotCenter.vx;
        node->v3Translation2.vy =
            (int16_t)node->v3RotCenter.vy;
        node->v3Translation2.vz =
            (int16_t)node->v3RotCenter.vz;
        node->v3Velocity2.vx =
            player->hitVelocity.vx;
        node->v3Velocity2.vy =
            player->hitVelocity.vy;
        node->v3Velocity2.vz =
            player->hitVelocity.vz;
        node->v3Velocity2.vy = 0x40;
        node->v3Velocity2.vx =
            (int16_t)(rand() % 4 - 8);
        node->v3Velocity2.vz =
            (int16_t)(rand() % 4 - 8);
        node->time = 0;
    }

    if (player->currentMotion == 0x4a &&
        arms[1] == 0) {
        Mnode *node = coll_GetNode(
            player->playerRoot.objectID, 5);

        arms[1] = 1;
        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            5);
        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            6);
        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            7);
        node->flags |= UINT32_C(0x04000000);
        node->v3Translation2.vx =
            (int16_t)node->v3RotCenter.vx;
        node->v3Translation2.vy =
            (int16_t)node->v3RotCenter.vy;
        node->v3Translation2.vz =
            (int16_t)node->v3RotCenter.vz;
        node->v3Velocity2.vx =
            player->hitVelocity.vx;
        node->v3Velocity2.vy =
            player->hitVelocity.vy;
        node->v3Velocity2.vz =
            player->hitVelocity.vz;
        node->v3Velocity2.vy = 0x20;
        node->v3Velocity2.vx =
            (int16_t)(rand() % 4 - 8);
        node->v3Velocity2.vz =
            (int16_t)(rand() % 4 - 8);
        node->time = 0;
    }
    return -1;
}

/* 0x1AB50, 527 bytes, global, 6 named locals
 * ai_Maul
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int ai_Maul(int32_t *cpad, playerObject *player)
{
    static int engaged;
    static int timer;
    static int zeroBSSCheck;
    wsl_ENEMY *pEnemy = player->pEnemy;
    int next_engaged;

    (void)cpad;
    (void)coll_GetNode(player->playernum, 1);
    if (zeroBSSCheck != zerobss_levelReset ||
        zerobss_ResetBoss != 0) {
        zerobss_ResetBoss = 0;
        timer = 0;
        zeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        engaged = 0;
    }

    if (GameStruct.CurrentLevel == UINT8_C(5)) {
        uint32_t start = (uint32_t)timer;

        if (timer == 0) {
            timer = (int32_t)gGlobalTimer;
            start = gGlobalTimer;
        } else if (start + UINT32_C(0x000e1000) < gGlobalTimer) {
            pEnemy->range = 10;
        }
        isTatoMaul = 1;
        (void)debug_printf(
            "\n\n\n%d\n", (gGlobalTimer - start) / UINT32_C(0x3c00));
    }
    if (pEnemy->aRange == -1) {
        pEnemy->aRange = 0;
        engaged = 0;
    }

    (void)debug_printf(
        "\n\n\n\nMaul is thinking about killing you\nstun %d\n",
        player->fStun);
    ai_ShowFlags(player);
    if (player->playerID == 9) {
        player->paMotions[81].FunctPtr = 27;
        player->paMotions[86].FunctPtr = 29;
        player->paMotions[82].FunctPtr = 28;
    } else {
        player->paMotions[85].FunctPtr = 27;
        player->paMotions[86].FunctPtr = 29;
    }

    if ((*player->pMotion)->Damage == 0 &&
        (uint16_t)(player->currentMotion - 16) > UINT16_C(2) &&
        player->fStun == 0 &&
        (uint16_t)(player->currentMotion - 33) > UINT16_C(10)) {
        next_engaged = engaged - 1;
    } else {
        next_engaged = engaged + 1;
    }
    if (next_engaged < 0) {
        next_engaged = 0;
    } else if (next_engaged > 0x200) {
        next_engaged = 0x200;
    }
    pEnemy->aRange = next_engaged;
    engaged = next_engaged;

    if ((abGlobalBits[5] & UINT8_C(4)) == 0 ||
        (player->pFlags & UINT32_C(1)) != 0) {
        (void)debug_printf("Maul is NOT LOCKED ON\n");
        player->pFlags &= UINT32_C(0xffbfffff);
        player->locked = NULL;
    } else {
        (void)debug_printf("Maul is LOCKED ON\n");
        player->pFlags |= UINT32_C(0x00400000);
        player->locked = player->target;
    }
    return -1;
}

/* 0x1AD60, 237 bytes, global, 3 named locals
 * ai_Mtt
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */

/* 0x1AE50, 191 bytes, global, 1 named locals
 * ai_ShowFlags
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int ai_Mtt(int32_t *cpad, playerObject *player)
{
    sceneObject *scene = (sceneObject *)player->playerRoot.pParent;
    physicsObject *p = (physicsObject *)scene->pPhysics;
    playerObject *target;
    sceneObject *target_scene;
    physicsObject *target_physics;

    (void)cpad;
    target = gpWorld->player0;
    if (target->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(&target->playerRoot, 0, 0x20) == 0 &&
        (target->pFlags & UINT32_C(0x00040200)) == 0) {
        target_scene = (sceneObject *)target->playerRoot.pParent;
        target_physics = (physicsObject *)target_scene->pPhysics;
        if (vec_QuickRangeCheckFV(
                &p->pos, &target_physics->pos, 512.0f) != 0) {
            (void)game_gModEnergy(0, -255);
        }
    }

    target = gpWorld->player1;
    if (target->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(&target->playerRoot, 0, 0x20) == 0 &&
        (target->pFlags & UINT32_C(0x00040200)) == 0) {
        target_scene = (sceneObject *)target->playerRoot.pParent;
        target_physics = (physicsObject *)target_scene->pPhysics;
        if (vec_QuickRangeCheckFV(
                &p->pos, &target_physics->pos, 512.0f) != 0) {
            (void)game_gModEnergy(1, -255);
        }
    }
    return -1;
}
void ai_ShowFlags(playerObject *player)
{
    if ((player->forceFlags & UINT32_C(0x001)) != 0) {
        (void)debug_printf("FORCE_ABSORB        \n");
    }
    if ((player->forceFlags & UINT32_C(0x002)) != 0) {
        (void)debug_printf("FORCE_REFLECT       \n");
    }
    if ((player->forceFlags & UINT32_C(0x004)) != 0) {
        (void)debug_printf("FORCE_REVERSE       \n");
    }
    if ((player->forceFlags & UINT32_C(0x008)) != 0) {
        (void)debug_printf("FORCE_GATHER        \n");
    }
    if ((player->forceFlags & UINT32_C(0x020)) != 0) {
        (void)debug_printf("FORCE_NOREACT       \n");
    }
    if ((player->forceFlags & UINT32_C(0x040)) != 0) {
        (void)debug_printf("FORCE_DEADLY        \n");
    }
    if ((player->forceFlags & UINT32_C(0x080)) != 0) {
        (void)debug_printf("FORCE_INVISIBLE     \n");
    }
    if ((player->forceFlags & UINT32_C(0x100)) != 0) {
        (void)debug_printf("FORCE_SHIELD        \n");
    }
}

/* 0x1AF10, 249 bytes, global, 5 named locals
 * ai_Sphere
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */

/* 0x1B010, 1148 bytes, global, 15 named locals
 * ai_StarFighter
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int ai_Sphere(int32_t *cpad, playerObject *player)
{
    int i;

    (void)cpad;
    if (player->hitDelay > gGlobalTimer) {
        return (int)gGlobalTimer;
    }

    for (i = 0; i < player->numCollisionNodes; ++i) {
        CollisionData *collision = &player->paNodesSizes[i];
        int n = (int)collision->id;
        VECTOR *pos = coll_GetNodeCenter(player->playernum, n);
        uint32_t color = coll_ChkNodeFlags(
            player->playernum, n, JPB_COLLISION_FLAG_HOT)
            ? UINT32_C(0x00ff0000)
            : UINT32_C(0x00ffffff);
        int32_t scaled_radius = (int32_t)(
            (uint32_t)(int32_t)collision->radius1 *
            (uint32_t)player->fScale);

        debug_drawsphere(
            pos->vx,
            pos->vy,
            pos->vz,
            scaled_radius / 0x1000,
            color);
    }
    return -1;
}
int ai_StarFighter(
    int32_t *cpad, playerObject *player)
{
    /* Exact function-local PDB statics at RVAs 0x4F1290 and 0x4F1298. */
    static int count;
    static int zeroBSSCheck;
    sceneObject *scene;
    physicsObject *p;
    Mnode *pelvis;
    int dist;
    int face;
    int sound_bank;
    int fire;

    (void)cpad;
    scene = (sceneObject *)player->playerRoot.pParent;
    p = (physicsObject *)scene->pPhysics;
    pelvis = coll_GetNode(player->playernum, 0);
    if (zeroBSSCheck != zerobss_levelReset ||
        zerobss_ResetBoss != 0) {
        zerobss_ResetBoss = 0;
        zeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        count = 0;
        MODE = 0;
    }

    ai_ShowFlags(player);
    MODE = -1;
    player->paMotions[6].vel = 0x38;
    player->paMotions[10].vel = 0x28;
    player->paMotions[12].vel = 0x28;
    player->paMotions[12].motionFlags =
        player->paMotions[1].motionFlags;
    player->paMotions[12].Lock =
        player->paMotions[1].Lock;
    player->pFlags &= UINT32_C(0xfff7ffff);
    if ((abGlobalBits[4] & UINT8_C(1)) != 0) {
        MODE = 0;
    }
    if ((abGlobalBits[4] & UINT8_C(2)) != 0) {
        MODE = 1;
    }
    if ((abGlobalBits[4] & UINT8_C(4)) != 0) {
        MODE = 2;
    }
    fire = abGlobalBits[5] & UINT8_C(1);
    if (player->currentMotion == 6) {
        abGlobalBits[5] &= UINT8_C(0xfe);
        return 0;
    }
    if (MODE != 0 || player->currentMotion != 2) {
        return -1;
    }

    dist = ai_FindFarPlayer(
        player, &player->target, 0);
    player->locked = player->target;
    player->pFlags |= UINT32_C(0x00080000);
    face = dist < 0x600
        ? physics_ForceFaceLock(
            &player->playerRoot,
            (objectRoot *)player->locked)
        : -0x400;
    pelvis->flags |= UINT32_C(0x00800000);
    sound_bank = player->playernum + 1;
    if (sound_bank > 3) {
        sound_bank = 3;
    }
    if (p->mov.vz >= 0.0f) {
        if (pelvis->v3RotationAbs.vz == 0x100) {
            (void)sound_Play(
                physics_gGetPosition(&player->playerRoot),
                sound_bank,
                "dfpsby1",
                0);
        }
        face += 0x40;
        if (pelvis->v3RotationAbs.vz >= -0xff) {
            pelvis->v3RotationAbs.vz -= 8;
        }
    } else {
        if (pelvis->v3RotationAbs.vz == -0x100) {
            (void)sound_Play(
                physics_gGetPosition(&player->playerRoot),
                sound_bank,
                "dfpsby1",
                0);
        }
        face -= 0x40;
        if (pelvis->v3RotationAbs.vz <= 0xff) {
            pelvis->v3RotationAbs.vz += 8;
        }
    }
    {
        int32_t delta = (int32_t)(
            (uint32_t)(p->face.vy - face) << 20);

        p->face.vy -= delta >> 22;
    }

    if (fire != 0 && --count < 0 && dist < 0xa00) {
        count = rand() % 6 + 3;
        boss_StarFighterBlaster(player, 0);
    }
    return -1;
}

/* 0x1B490, 587 bytes, global, 12 named locals
 * ai_Thug
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int ai_Thug(int32_t *cpad, playerObject *player)
{
    static int delay;
    static int rad;
    static int shield;
    static int zeroBSSCheck;
    EffectHeader *effect = paEffects[61];

    (void)cpad;
    (void)coll_GetNode(player->playernum, 1);
    if (zeroBSSCheck != zerobss_levelReset ||
        zerobss_ResetBoss != 0) {
        zerobss_ResetBoss = 0;
        zeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        delay = 0;
        rad = 0;
        shield = 0;
    }

    player->paMotions[65].FunctPtr = 25;
    ai_ShowFlags(player);
    if ((player->forceFlags & UINT32_C(0x100)) != 0) {
        sceneObject *scene = (sceneObject *)player->playerRoot.pParent;
        physicsObject *phy = (physicsObject *)scene->pPhysics;
        VECTOR *pos = coll_GetNodeCenter(
            player->playerRoot.objectID, 0);
        Sprite **sprites;
        uint32_t x;

        if (player->forceData[0] == 0) {
            Sprite **created;

            sprites = (Sprite **)memalloc(
                effect->num * (unsigned)sizeof(*sprites));
            player->forceData[0] = (int64_t)(intptr_t)sprites;
            created = sprite_AddSpriteEffect(
                effect->aEffects, (int)effect->num, pos, NULL);
            memcpy(
                sprites,
                created,
                effect->num * sizeof(*sprites));
        }
        sprites = (Sprite **)(intptr_t)player->forceData[0];
        player->forceFlags |= UINT32_C(0x10);
        for (x = 0; x < effect->num; ++x) {
            if (sprites[x] != NULL) {
                int px = (int)(pos->vx + phy->mov.vx);
                int py = (int)(pos->vy + phy->mov.vy);
                int pz = (int)(pos->vz + phy->mov.vz);

                sprite_gSetSpritePosition(
                    sprites[x],
                    (int16_t)px,
                    (int16_t)py + 0x20,
                    (int16_t)pz);
                sprites[x]->sp_Time = 0;
            }
        }
        return -1;
    }

    if (player->forceData[0] != 0) {
        Sprite **sprites =
            (Sprite **)(intptr_t)player->forceData[0];
        uint32_t x;

        (void)coll_GetNodeCenter(player->playerRoot.objectID, 0);
        player->forceFlags &= UINT32_C(0xffffffef);
        for (x = 0; x < effect->num; ++x) {
            if (sprites[x] != NULL) {
                sprite_gFreeSprite(sprites[x]);
                sprites[x] = NULL;
            }
        }
        memfree(sprites);
        player->forceData[0] = 0;
    }
    return -1;
}

/* 0x1B6E0, 2927 bytes, global, 53 named locals
 * ai_TurretDroid
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int ai_TurretDroid(int32_t *cpad, playerObject *player)
{
    static int arms[2];
    static int count[2];
    static int delay;
    static int shield;
    static Sprite *sptr[16];
    static int zeroBSSCheck;
    static _svector delta;
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physobj =
        (physicsObject *)scene->pPhysics;
    modelObject *model =
        (modelObject *)scene->pModel;
    int i;

    (void)cpad;
    (void)coll_GetNode(player->playernum, 1);
    if (zeroBSSCheck != zerobss_levelReset ||
        zerobss_ResetBoss != 0) {
        zerobss_ResetBoss = 0;
        memset(sptr, 0, sizeof(sptr));
        zeroBSSCheck = zerobss_levelReset;
        zerobss_levelReset = 0;
        arms[0] = 0;
        arms[1] = 0;
        count[0] = 0;
        count[1] = 0;
        delay = 0;
        shield = 0;
        for (i = 0; i < 16; ++i) {
            if (sptr[i] != NULL) {
                sprite_gFreeSprite(sptr[i]);
                sptr[i] = NULL;
            }
        }
    }

    if (game_GetGameCounter(0) < 7) {
        abGlobalBits[3] |= UINT8_C(1);
    }
    ai_ShowFlags(player);
    if ((abGlobalBits[5] & UINT8_C(8)) != 0) {
        player->forceFlags ^= UINT32_C(0x20);
    }

    delta.vx = (int16_t)(delta.vx + 0x10);
    if (arms[0] == 1) {
        if (++count[0] > 12) {
            EffectHeader *effect = paEffects[2];

            count[0] = 0;
            (void)sprite_AddSpriteEffectAtNode(
                effect->aEffects,
                (int)effect->num,
                player->playernum,
                12);
        }
    } else {
        coll_SetNodeRotationAbs(
            player->playernum, 12, &delta);
    }
    if (arms[1] == 1) {
        if (++count[1] > 12) {
            EffectHeader *effect = paEffects[2];

            count[1] = 0;
            (void)sprite_AddSpriteEffectAtNode(
                effect->aEffects,
                (int)effect->num,
                player->playernum,
                11);
        }
    } else {
        coll_SetNodeRotationAbs(
            player->playernum, 11, &delta);
    }

    if (player->currentMotion == 15 ||
        player->currentMotion == 14) {
        int projectileType =
            player->currentMotion == 15 ? 7 : 8;
        uint32_t events = model->eventMask;

        if (player->currentMotion == 14 &&
            player->locked == NULL) {
            return -1;
        }
        while (events != 0) {
            int event = brainutl_FindLSB_LV(events);
            Projectile *proj;

            if (event == 0) {
                break;
            }
            events &= ~(UINT32_C(1) << (event - 1));
            proj = bullet_AllocProjectile(projectileType);
            if (proj != NULL) {
                VECTOR *pos0 = coll_GetNodeCenter(
                    player->playernum, event - 1);
                _svector randomDirection = {
                    0, 0x400, 0, 0
                };
                _svector rotation = {
                    (int16_t)physobj->angle.vx,
                    (int16_t)physobj->angle.vy,
                    (int16_t)physobj->angle.vz,
                    0
                };
                _svector direction;
                MATRIX matrix;
                VECTOR pos1;

                fRotMatrix(&rotation, &matrix);
                PushMatrix();
                if (player->currentMotion == 15) {
                    randomDirection.vy = 0x100;
                }
                randomDirection.vx =
                    (int16_t)((rand() % 512 - 0x100) * 2);
                randomDirection.vz =
                    (int16_t)((rand() % 512 - 0x100) * 2);
                fApplyMatrixSV(
                    &matrix, &randomDirection, &direction);
                PopMatrix();
                pos1.vx = pos0->vx + direction.vx;
                pos1.vy = pos0->vy + direction.vy;
                pos1.vz = pos0->vz + direction.vz;
                pos1.pad = 0;
                bullet_ShootProjectile(
                    proj, player, pos0, &pos1, NULL);
            }
        }
    }

    if ((abGlobalBits[5] & UINT8_C(1)) != 0) {
        VECTOR *nodeCenter = coll_GetNodeCenter(
            player->playernum, 9);
        _svector *nodeRotation = coll_GetNodeRotation(
            player->playernum, 9);
        _svector start = {
            (int16_t)nodeCenter->vx,
            (int16_t)nodeCenter->vy,
            (int16_t)nodeCenter->vz,
            0
        };
        _svector orientation = {
            nodeRotation->vx,
            (int16_t)(nodeRotation->vy + physobj->angle.vy),
            nodeRotation->vz,
            0
        };
        _svector forward = {0, 0, 0x200, 0};
        _svector transformed;
        _svector direction;
        _svector hitpoint;
        MATRIX matrix;
        VECTOR effectPosition;
        int *cube = NULL;
        int *entry = NULL;
        int *poly = NULL;
        int dist;
        EffectHeader *effect;

        fRotMatrix(&orientation, &matrix);
        PushMatrix();
        fApplyMatrixSV(&matrix, &forward, &transformed);
        PopMatrix();
        (void)normalize(
            transformed.vx,
            transformed.vy - 0x40,
            transformed.vz,
            &direction);
        (void)RaycastCheckSV(
            &start,
            &direction,
            0x500,
            &cube,
            &entry,
            &poly,
            &dist,
            &hitpoint);
        SetCameraMatrix();
        zpush = -38;
        start.vx = (int16_t)(
            start.vx + direction.vx * 0x40 / 0x1000);
        start.vy = (int16_t)(
            start.vy + direction.vy * 0x40 / 0x1000);
        start.vz = (int16_t)(
            start.vz + direction.vz * 0x40 / 0x1000);
        fx_screenGlow(
            &start, &hitpoint, 12, UINT32_C(0xc07f7f7f));
        zpush = -38;
        fx_screenGlow(
            &start, &hitpoint, 0x40, UINT32_C(0xc0102070));
        effectPosition.vx = hitpoint.vx;
        effectPosition.vy = hitpoint.vy;
        effectPosition.vz = hitpoint.vz;
        effectPosition.pad = 0;
        effect = paEffects[73];
        (void)sprite_AddSpriteEffect(
            effect->aEffects,
            (int)effect->num,
            &effectPosition,
            NULL);
        (void)zapcheck(
            &gaPlayerData[0],
            &start,
            &hitpoint,
            -15,
            player,
            8);
        (void)zapcheck(
            &gaPlayerData[1],
            &start,
            &hitpoint,
            -15,
            player,
            8);
    }

    if (player->currentMotion == 22 ||
        player->currentMotion == 23) {
        uint32_t events = model->eventMask;

        (void)debug_printf("strafe!!\n");
        while (events != 0) {
            int event = brainutl_FindLSB_LV(events);
            int nodeID;
            Projectile *proj;

            if (event == 0) {
                break;
            }
            nodeID = event - 1;
            events &= ~(UINT32_C(1) << nodeID);
            if ((nodeID == 12 && arms[0] == 1) ||
                (nodeID == 11 && arms[1] == 1)) {
                continue;
            }
            proj = bullet_AllocProjectile(0);
            if (proj != NULL) {
                VECTOR *pos0 = coll_GetNodeCenter(
                    player->playernum, nodeID);
                _svector *nodeRotation = coll_GetNodeRotation(
                    player->playernum, 9);
                _svector orientation = {
                    nodeRotation->vx,
                    (int16_t)(nodeRotation->vy + physobj->angle.vy),
                    nodeRotation->vz,
                    0
                };
                _svector localDirection = {0, -8, 0x40, 0};
                _svector direction;
                MATRIX matrix;
                VECTOR pos1;

                fRotMatrix(&orientation, &matrix);
                PushMatrix();
                fApplyMatrixSV(
                    &matrix, &localDirection, &direction);
                PopMatrix();
                pos1.vx = pos0->vx + direction.vx;
                pos1.vy = pos0->vy + direction.vy;
                pos1.vz = pos0->vz + direction.vz;
                pos1.pad = 0;
                proj->pj_Flags |= 0x10;
                bullet_ShootProjectile(
                    proj, player, pos0, &pos1, NULL);
            }
        }
    }

    if ((abGlobalBits[5] & UINT8_C(4)) != 0) {
        Mnode *head = coll_GetNode(player->playernum, 9);
        _svector *pelvis = coll_GetNodeRotation(
            player->playernum, 0);
        int facing = physics_gFaceTarget(
            &player->playerRoot,
            &player->locked->playerRoot);

        head->flags |= UINT32_C(0x00800000);
        head->v3RotationAbs.vy = (int16_t)(
            facing - pelvis->vy - physobj->angle.vy);
    }

    if ((abGlobalBits[5] & UINT8_C(2)) != 0) {
        EffectHeader *shieldEffect = paEffects[27];
        EffectHeader *impactEffect = paEffects[28];
        VECTOR *pos = coll_GetNodeCenter(
            player->playerRoot.objectID, 0);
        int x = (int)((float)pos->vx + physobj->mov.vx);
        int y = (int)((float)pos->vy + physobj->mov.vy);
        int z = (int)((float)pos->vz + physobj->mov.vz);
        uint32_t spriteIndex;

        if (shield == 0) {
            Sprite **created;

            (void)sprite_AddSpriteEffect(
                impactEffect->aEffects,
                (int)impactEffect->num,
                pos,
                NULL);
            created = sprite_AddSpriteEffect(
                shieldEffect->aEffects,
                (int)shieldEffect->num,
                pos,
                NULL);
            memcpy(
                sptr,
                created,
                shieldEffect->num * sizeof(*sptr));
            for (spriteIndex = 0;
                 spriteIndex < shieldEffect->num;
                 ++spriteIndex) {
                if (sptr[spriteIndex] != NULL) {
                    ++shield;
                }
            }
        }
        for (spriteIndex = 0;
             spriteIndex < shieldEffect->num;
             ++spriteIndex) {
            if (sptr[spriteIndex] != NULL) {
                sprite_gSetSpritePosition(
                    sptr[spriteIndex],
                    (int16_t)x,
                    (int16_t)y + 0x20,
                    (int16_t)z);
                sptr[spriteIndex]->sp_cScale.init = 0x2800;
                sptr[spriteIndex]->sp_Time = 0;
            }
        }
    } else if (shield != 0) {
        EffectHeader *shieldEffect = paEffects[27];
        EffectHeader *impactEffect = paEffects[28];
        VECTOR *pos = coll_GetNodeCenter(
            player->playerRoot.objectID, 0);
        uint32_t spriteIndex;

        shield = 0;
        (void)sprite_AddSpriteEffect(
            impactEffect->aEffects,
            (int)impactEffect->num,
            pos,
            NULL);
        for (spriteIndex = 0;
             spriteIndex < shieldEffect->num;
             ++spriteIndex) {
            if (sptr[spriteIndex] != NULL) {
                sprite_gFreeSprite(sptr[spriteIndex]);
                sptr[spriteIndex] = NULL;
            }
        }
    }

    if (player->currentMotion == 17 && arms[0] == 0) {
        EffectHeader *effect = paEffects[2];
        Mnode *node = coll_GetNode(
            player->playerRoot.objectID, 12);

        arms[0] = 1;
        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            12);
        node->flags |= UINT32_C(0x04000000);
        node->v3Translation2.vx =
            (int16_t)node->v3RotCenter.vx;
        node->v3Translation2.vy =
            (int16_t)node->v3RotCenter.vy;
        node->v3Translation2.vz =
            (int16_t)node->v3RotCenter.vz;
        node->v3Velocity2.vx = player->hitVelocity.vx;
        node->v3Velocity2.vy = player->hitVelocity.vy;
        node->v3Velocity2.vz = player->hitVelocity.vz;
        node->v3Velocity2.vy = 0x40;
        node->v3Velocity2.vx = (int16_t)(rand() % 4 - 8);
        node->v3Velocity2.vz = (int16_t)(rand() % 4 - 8);
        node->time = 0;
    }

    if (player->currentMotion == 18 && arms[1] == 0) {
        EffectHeader *effect = paEffects[2];
        Mnode *node = coll_GetNode(
            player->playerRoot.objectID, 11);

        arms[1] = 1;
        (void)sprite_AddSpriteEffectAtNode(
            effect->aEffects,
            (int)effect->num,
            player->playernum,
            11);
        node->flags |= UINT32_C(0x04000000);
        node->v3Translation2.vx =
            (int16_t)node->v3RotCenter.vx;
        node->v3Translation2.vy =
            (int16_t)node->v3RotCenter.vy;
        node->v3Translation2.vz =
            (int16_t)node->v3RotCenter.vz;
        node->v3Velocity2.vx = player->hitVelocity.vx;
        node->v3Velocity2.vy = player->hitVelocity.vy;
        node->v3Velocity2.vz = player->hitVelocity.vz;
        node->v3Velocity2.vy = 0x20;
        node->v3Velocity2.vx = (int16_t)(rand() % 4 - 8);
        node->v3Velocity2.vz = (int16_t)(rand() % 4 - 8);
        node->time = 0;
    }
    return -1;
}

/* 0x1C250, 51 bytes, global, 2 named locals
 * ai_Worm
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */

/* 0x1C290, 414 bytes, global, 8 named locals
 * boss_StarFighterBlaster
 * PDB type: void (playerObject*, int)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int ai_Worm(int32_t *cpad, playerObject *player)
{
    ai_ShowFlags(player);
    (void)ai_Sphere(cpad, player);
    return -1;
}
void boss_StarFighterBlaster(playerObject *player, int LOCK_ON)
{
    Motion *motion = player->pMotion != NULL
        ? *player->pMotion
        : NULL;
    VECTOR *pos0;
    VECTOR *pos1;
    _svector *vel;
    VECTOR temp;
    Projectile *proj;
    int sound_bank;

    (void)LOCK_ON;
    if (motion == NULL || player->target == NULL) {
        return;
    }

    pos0 = coll_GetNodeCenter(player->playernum, 13);
    pos1 = coll_GetNodeCenter(player->target->playernum, 0);
    vel = coll_GetNodeVelocity(player->playernum, 13);
    if (pos0 != NULL && pos1 != NULL) {
        temp.vx = pos1->vx;
        temp.vy = pos1->vy - rand() % 256;
        temp.vz = pos0->vz;
        temp.pad = 0;
        proj = bullet_AllocProjectile((int)motion->fx2);
        if (proj != NULL) {
            bullet_ShootProjectile(
                proj, player, pos0, &temp, vel);
            sound_bank = player->playernum + 1;
            if (sound_bank > 3) {
                sound_bank = 3;
            }
            (void)sound_Play(
                physics_gGetPosition(&player->playerRoot),
                sound_bank,
                "dfrblstr",
                0);
        }
    }

    pos0 = coll_GetNodeCenter(player->playernum, 8);
    pos1 = coll_GetNodeCenter(player->target->playernum, 0);
    vel = coll_GetNodeVelocity(player->playernum, 13);
    if (pos0 != NULL && pos1 != NULL) {
        temp.vx = pos1->vx;
        temp.vy = pos1->vy - rand() % 256;
        temp.vz = pos0->vz;
        temp.pad = 0;
        proj = bullet_AllocProjectile((int)motion->fx2);
        if (proj != NULL) {
            bullet_ShootProjectile(
                proj, player, pos0, &temp, vel);
        }
    }
}

/* 0x1C430, 93 bytes, global, 5 named locals
 * maul_PushCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int maul_PushCallBack(int32_t *cpad, playerObject *player)
{
    int index = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);

    (void)cpad;
    if (index > 1) {
        Projectile *proj = bullet_AllocProjectile(14);

        if (proj != NULL) {
            sceneObject *scene =
                (sceneObject *)player->playerRoot.pParent;
            physicsObject *physics =
                (physicsObject *)scene->pPhysics;
            VECTOR *pos = &physics->vpos;

            bullet_ShootProjectile(
                proj, player, pos, pos, NULL);
        }
        return 1;
    }
    return 0;
}

/* 0x1C490, 93 bytes, global, 5 named locals
 * maul_RingCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int maul_RingCallBack(int32_t *cpad, playerObject *player)
{
    int index = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);

    (void)cpad;
    if (index > 1) {
        Projectile *proj = bullet_AllocProjectile(14);

        if (proj != NULL) {
            sceneObject *scene =
                (sceneObject *)player->playerRoot.pParent;
            physicsObject *physics =
                (physicsObject *)scene->pPhysics;
            VECTOR *pos = &physics->vpos;

            bullet_ShootProjectile(
                proj, player, pos, pos, NULL);
        }
        return 1;
    }
    return 0;
}

/* 0x1C4F0, 833 bytes, global, 21 named locals
 * maul_ZapCallBack
 * PDB type: int (long*, playerObject*)
 * Source: W:\SWJediPowerBattles\Work\boss.c
 */
int maul_ZapCallBack(int32_t *cpad, playerObject *player)
{
    int index = animutl_gGetCurrentFrameIndex(
        &player->playerRoot);

    (void)cpad;
    if ((uint32_t)(index - 19) < UINT32_C(27)) {
        VECTOR *pos1 = coll_GetNodeCenter(player->playernum, 13);
        VECTOR *pos0 = coll_GetNodeCenter(player->playernum, 15);
        FVECTOR direction;
        FVECTOR start;
        FVECTOR hitpoint;
        int *cube = NULL;
        int *entry = NULL;
        int *poly = NULL;
        int dist = 0;
        int hit;

        direction.vx = (float)(pos0->vx - pos1->vx);
        direction.vy = (float)(pos0->vy - pos1->vy);
        direction.vz = (float)(pos0->vz - pos1->vz);
        start.vx = (float)pos0->vx;
        start.vy = (float)pos0->vy;
        start.vz = (float)pos0->vz;
        if (VectorNormalize(&direction) > 0.1f) {
            _svector s;
            _svector h;
            VECTOR eff;
            int x;

            hit = RaycastCheck(
                &start,
                &direction,
                768.0f,
                &cube,
                &entry,
                &poly,
                &dist,
                &hitpoint);
            s.vx = (int16_t)(int32_t)start.vx;
            s.vy = (int16_t)(int32_t)start.vy;
            s.vz = (int16_t)(int32_t)start.vz;
            h.vx = (int16_t)(int32_t)hitpoint.vx;
            h.vy = (int16_t)(int32_t)hitpoint.vy;
            h.vz = (int16_t)(int32_t)hitpoint.vz;
            fx_screenGlow(&s, &h, 0x18, UINT32_C(0xc0ff4020));
            PlotZap(
                UINT32_C(0x00ff4020),
                UINT32_C(0x00ff8040),
                UINT32_C(0x00ff8040),
                &s,
                &h,
                0x800,
                0x40);
            eff.vx = (int32_t)hitpoint.vx;
            eff.vy = (int32_t)hitpoint.vy;
            eff.vz = (int32_t)hitpoint.vz;
            eff.pad = 0;
            if (hit != 0) {
                EffectHeader *effect = paEffects[67];

                (void)sprite_AddSpriteEffect(
                    effect->aEffects,
                    (int)effect->num,
                    &eff,
                    NULL);
            }

            for (x = 0; x < 2; ++x) {
                physicsObject *p = &maPhysicsData[x];
                VECTOR *body;
                int line_distance;

                if (p->physicsRoot.objectID == -1 ||
                    obj_gCheckObjectFlag(
                        &p->physicsRoot, 0, 0x20) != 0 ||
                    p->physicsRoot.objectID == player->playernum) {
                    continue;
                }
                body = coll_GetNodeCenter(
                    p->physicsRoot.objectID, 0);
                if (body == NULL) {
                    continue;
                }
                line_distance = vec_DistPoint2Line(
                    body, pos0, &eff);
                if (line_distance < p->height && line_distance > 0) {
                    sceneObject *scene =
                        (sceneObject *)p->physicsRoot.pParent;
                    playerObject *target =
                        (playerObject *)scene->pPlayer;
                    EffectHeader *effect = paEffects[73];

                    (void)sprite_AddSpriteEffect(
                        effect->aEffects,
                        (int)effect->num,
                        &p->vpos,
                        NULL);
                    ++target->hitNumber;
                    target->projectile =
                        &((ProjType *)(void *)maProjTypes)[6];
                }
            }
        }
    }
    return 0;
}
