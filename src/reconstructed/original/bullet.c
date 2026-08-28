/*
 * REVIEWED RECONSTRUCTION.
 *
 * The exact Projectile layout, allocation/free leaves, hard-coded sound-name
 * initialization, and projectile firing setup are checked against the
 * matched PDB and executable. All eight procedures are now reviewed,
 * including bullet_CallBack's ballistic/homing motion, sprite ownership,
 * lifetime/effect/audio handling, map and character collision rules,
 * reflected/piercing behavior, authored level exceptions, and explosion tail.
 * PDB module: 0012
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\bullet.obj
 * Primary source: W:\SWJediPowerBattles\Work\bullet.c
 * Compiler language: c
 * Emitted procedures: 8
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/bullet.h"

#include "jpb/alloc.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/effects.h"
#include "jpb/flex.h"
#include "jpb/fx.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/intersec.h"
#include "jpb/jonny.h"
#include "jpb/model.h"
#include "jpb/player.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/vectors.h"
#include "jpb/world.h"

#include <string.h>

static JPBBulletLaunchObserver bullet_launch_observer;
static void *bullet_launch_observer_user_data;

void jpb_BulletSetLaunchObserver(
    JPBBulletLaunchObserver observer, void *user_data)
{
    bullet_launch_observer = observer;
    bullet_launch_observer_user_data = user_data;
}

static void bullet_observe_launch(
    const Projectile *projectile,
    const playerObject *player,
    const VECTOR *start,
    const VECTOR *target)
{
    if (bullet_launch_observer != NULL) {
        bullet_launch_observer(
            bullet_launch_observer_user_data,
            projectile,
            player,
            start,
            target);
    }
}

static ProjType *bullet_projectile_types(void)
{
    return (ProjType *)(void *)maProjTypes;
}

static void bullet_spawn_effect(
    int effect_index, VECTOR *position, _svector *velocity)
{
    EffectHeader *effect;

    if (effect_index < 0) {
        return;
    }
    effect = paEffects[effect_index];
    (void)sprite_AddSpriteEffect(
        effect->aEffects,
        (int)effect->num,
        position,
        velocity);
}

static uint32_t bullet_owner_mask(const playerObject *owner)
{
    if (owner == NULL) {
        return UINT32_C(3);
    }
    return UINT32_C(1) <<
        ((unsigned)owner->playerRoot.objectID & 31u);
}

/* 0x22780, 30 bytes, global, 2 named locals
 * TerminateSFXString
 * PDB type: void (char*, const char*)
 * Source: W:\SWJediPowerBattles\Work\bullet.c
 */
void TerminateSFXString(char *dest, const char *source)
{
    strncpy(dest, source, 8);
    dest[8] = '\0';
}

/* 0x227A0, 61 bytes, global, 2 named locals
 * bullet_AllocProjectile
 * PDB type: Projectile* (int)
 * Source: W:\SWJediPowerBattles\Work\bullet.c
 */
Projectile *bullet_AllocProjectile(int type)
{
    Projectile *proj =
        (Projectile *)memalloc((unsigned)sizeof(*proj));

    if (proj != NULL) {
        memset(proj, 0, sizeof(*proj));
        proj->pj_Type = (int16_t)type;
    }
    return proj;
}

/* 0x227E0, 2178 bytes, global, 28 named locals
 * bullet_CallBack
 * PDB type: int (Projectile*)
 * Source: W:\SWJediPowerBattles\Work\bullet.c
 */
int bullet_CallBack(Projectile *callback)
{
    Sprite *cb = (Sprite *)(void *)callback;
    playerObject *target;
    int trigger = 0;
    int kill = 0;
    physicsObject *object;
    playerObject *owner;
    uint32_t mask = UINT32_C(3);
    VECTOR mid;
    Projectile *proj;
    _svector normal = {0, 0, 0, 0};
    int result;
    VECTOR dir;
    VECTOR pos;
    ProjType *type;
    _svector rot;
    int spd;
    VECTOR *pelvis;
    _svector home;
    _mvector bob;
    VECTOR *node;
    int speed;
    int n;
    _svector temp;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return 0;
    }
    proj = (Projectile *)(void *)cb->sp_User;
    if (proj == NULL) {
        return 1;
    }
    owner = (playerObject *)(void *)proj->pj_Owner;
    type = &bullet_projectile_types()[proj->pj_Type];
    if (owner != NULL &&
        (owner->playerRoot.objectID > 1 ||
         GameStruct.versusModeFlag != 0)) {
        mask = bullet_owner_mask(owner);
    }
    pos = proj->pj_Start;

    if ((type->flag & UINT16_C(2)) != 0) {
        spd = (int)proj->pj_Dir.speed;
        home.vx = (int16_t)flexmul((int)proj->pj_Dir.vx, spd);
        home.vy = (int16_t)(
            flexmul((int)proj->pj_Dir.vy, spd) -
            ((type->flag & UINT16_C(0x200)) != 0 ? 4 : 16));
        home.vz = (int16_t)flexmul((int)proj->pj_Dir.vz, spd);
        home.pad = 0;
        proj->pj_Dir.speed = (int16_t)normalize_svector(
            &home, (_svector *)(void *)&proj->pj_Dir);
        (void)vec_RotFromNormalS(
            &home, (_svector *)(void *)&proj->pj_Dir);
        if ((type->flag & UINT16_C(8)) == 0) {
            (void)sprite_Get3DProjectile(
                proj, &proj->pj_Start, &home);
        }
    } else if ((type->flag & UINT16_C(0x100)) != 0 &&
               proj->pj_Target != NULL) {
        target = (playerObject *)(void *)proj->pj_Target;
        pelvis = coll_GetNodeCenter(target->playernum, 0);
        if (pelvis != NULL) {
            home.vx = (int16_t)(pelvis->vx - proj->pj_Start.vx);
            home.vy = (int16_t)(pelvis->vy - proj->pj_Start.vy);
            home.vz = (int16_t)(pelvis->vz - proj->pj_Start.vz);
            home.pad = 0;
            (void)normalize(
                (int)home.vx,
                (int)home.vy,
                (int)home.vz,
                &home);
            proj->pj_Dir.vx = (int16_t)(
                proj->pj_Dir.vx + flexmul((int)home.vx, 0x200));
            proj->pj_Dir.vy = (int16_t)(
                proj->pj_Dir.vy + flexmul((int)home.vy, 0x200));
            proj->pj_Dir.vz = (int16_t)(
                proj->pj_Dir.vz + flexmul((int)home.vz, 0x200));
            (void)normalize(
                (int)proj->pj_Dir.vx,
                (int)proj->pj_Dir.vy,
                (int)proj->pj_Dir.vz,
                (_svector *)(void *)&proj->pj_Dir);
            (void)vec_RotFromNormalS(
                &rot, (_svector *)(void *)&proj->pj_Dir);
            if ((type->flag & UINT16_C(8)) == 0) {
                sprite_Move3DSprite(cb, &proj->pj_Start);
            }
        }
    }

    bob = proj->pj_Dir;
    bob.speed = (int16_t)((int)bob.speed / 8);
    result = MoveObjectNormal(&bob, &pos, SMALL_HIT, &normal);
    dir.vx = pos.vx - proj->pj_Start.vx;
    dir.vy = pos.vy - proj->pj_Start.vy;
    dir.vz = pos.vz - proj->pj_Start.vz;
    dir.pad = 0;
    mid.vx = proj->pj_Start.vx + pos.vx;
    mid.vy = proj->pj_Start.vy + pos.vy;
    mid.vz = proj->pj_Start.vz + pos.vz;
    mid.pad = 0;

    if ((proj->pj_Flags & UINT32_C(0x2000)) != 0 &&
        owner != NULL) {
        node = coll_GetNodeCenter(
            owner->playernum, (int)(int8_t)proj->launchID);
        if (node != NULL) {
            fx_PlasmaZap(
                &jpb_PlasmaZapVars[
                    ((int)owner->playernum +
                     (int)(int8_t)proj->launchID) & 7],
                node,
                &proj->pj_Start,
                (uint32_t)proj->color,
                (uint32_t)proj->color,
                0x800);
        }
    }
    proj->pj_Start = pos;

    if ((type->flag & UINT16_C(8)) == 0) {
        if (proj->pj_Parent != NULL) {
            sprite_Move3DSprite(proj->pj_Parent, &dir);
        }
        if (proj->pj_Child != NULL) {
            sprite_Move3DSprite(proj->pj_Child, &dir);
        }
    } else {
        if (proj->pj_Parent != NULL) {
            sprite_gMoveSpritePosition(
                proj->pj_Parent,
                (float)dir.vx,
                (float)dir.vy,
                (float)dir.vz);
            (void)sprite_MainCallBack(
                (int32_t *)(void *)proj->pj_Parent);
            if ((proj->pj_Parent->sp_Flags & UINT32_C(1)) != 0) {
                proj->pj_Range = 0;
            }
        }
        if (proj->pj_Child != NULL) {
            sprite_gSetSpritePosition(
                proj->pj_Child,
                mid.vx / 2,
                mid.vy / 2,
                mid.vz / 2);
            (void)sprite_MainCallBack(
                (int32_t *)(void *)proj->pj_Child);
            if ((proj->pj_Child->sp_Flags & UINT32_C(1)) != 0) {
                proj->pj_Range = 0;
            }
        }
    }

    if (gGlobalFrameRate == 0) {
        return -1;
    }
    --proj->pj_Range;
    if (proj->pj_Range < 1) {
        bullet_spawn_effect(
            (int)type->rangeEffect, &proj->pj_Start, NULL);
        goto terminate_projectile;
    }
    if (result >= 0) {
        speed = (int)proj->pj_Dir.speed;
        bullet_spawn_effect(
            (int)type->hitEffect, &proj->pj_Start, NULL);
        if ((type->flag & UINT16_C(0x80)) != 0 && speed > 0) {
            int dot =
                flexmul((int)proj->pj_Dir.vz, (int)normal.vz) +
                flexmul((int)proj->pj_Dir.vy, (int)normal.vy) +
                flexmul((int)proj->pj_Dir.vx, (int)normal.vx);

            vec_ScaleVector(&normal, dot * 2);
            proj->pj_Dir.vx = (int16_t)(
                proj->pj_Dir.vx - normal.vx);
            proj->pj_Dir.vy = (int16_t)(
                proj->pj_Dir.vy - normal.vy);
            proj->pj_Dir.vz = (int16_t)(
                proj->pj_Dir.vz - normal.vz);
            (void)normalize_svector(
                (_svector *)(void *)&proj->pj_Dir,
                (_svector *)(void *)&proj->pj_Dir);
            proj->pj_Dir.speed =
                (int16_t)flexmul(speed, 0x0dac);
            if ((type->flag & UINT16_C(0x40)) != 0 && speed > 0x20) {
                (void)sound_Play(
                    &proj->pj_Start, 0, "tink", 0);
            }
            return 0;
        }

terminate_projectile:
        if ((type->flag & UINT16_C(0x40)) == 0) {
            goto remove_projectile;
        }
        TerminateSFXString(terminatedSound, type->hitSound);
        (void)sound_Play(
            &proj->pj_Start, 0, terminatedSound, 0);
        proj->pj_Flags |= UINT32_C(0x20);
        if (owner != NULL && owner->playerRoot.objectID > 1) {
            mask = bullet_owner_mask(owner);
        }
        trigger = 1;
    }

    if ((proj->pj_Flags & UINT32_C(0x1000)) != 0 &&
        owner != NULL) {
        mask = bullet_owner_mask(owner) | UINT32_C(0xfffc);
    }
    if (owner != NULL && tankID >= 0 &&
        owner->playerRoot.objectID < 2) {
        mask |= UINT32_C(1) << ((unsigned)tankID & 31u);
    }

    object = physics_FindWithinRange(
        &proj->pj_Start, &mask, 0x200);
    kill = trigger;
    while (object != NULL) {
        sceneObject *scene = (sceneObject *)(void *)
            object->physicsRoot.pParent;
        int collision_result;

        target = (playerObject *)(void *)scene->pPlayer;
        proj->pj_Target = (int32_t *)(void *)target;

        if (owner != NULL && owner->playernum < 2) {
            if (LevelSelect == 4 &&
                (proj->pj_Flags & UINT32_C(0x6000)) == 0 &&
                (target->playerID == 0x62 ||
                 (gpWorld != NULL &&
                  (gpWorld->aDolly[gpWorld->currentDolly].flags &
                   UINT32_C(0x400)) != 0))) {
                proj->pj_Flags |= UINT32_C(0x6000);
            }
            if (LevelSelect == 15 &&
                target->pEnemy != NULL &&
                (target->pEnemy->enemyID == 26 ||
                 target->pEnemy->enemyID == 28) &&
                (proj->pj_Flags & UINT32_C(0x6000)) == 0) {
                sceneObject *target_scene = (sceneObject *)(void *)
                    target->playerRoot.pParent;
                modelObject *model =
                    (modelObject *)(void *)target_scene->pModel;

                if ((model->flags & UINT32_C(4)) != 0) {
                    proj->pj_Flags |= UINT32_C(0x6000);
                }
            }
        }

        if (target->playerID == 0x61 &&
            (uint32_t)(proj->pj_Start.vx + 0x2132) <
                UINT32_C(0x234) &&
            (uint32_t)(proj->pj_Start.vy - 0x0d01) <
                UINT32_C(0x175) &&
            (uint32_t)(proj->pj_Start.vz + 0x6361) <
                UINT32_C(0x3e0)) {
            kill = 1;
        }

        collision_result = 0;
        if ((target->pFlags & UINT32_C(0x80)) == 0 ||
            target->playerID == 0x23 ||
            (owner != NULL && owner->playerID == 0x23)) {
            collision_result = coll_CheckProjectileCollision(proj);
        }
        if (collision_result != 0) {
            if ((proj->pj_Flags & UINT32_C(0x6000)) != 0) {
                goto remove_projectile;
            }
            target->hitNumber = 1;
            target->projectile = type;
            target->whohitme = owner;
            if (collision_result == 1) {
                if ((proj->pj_Flags & UINT32_C(0x800)) == 0) {
                    kill = 1;
                }
            } else if (collision_result == -1) {
                proj->pj_Flags |= UINT32_C(0x400);
                type->flag |= UINT16_C(0x400);
                proj->pj_Owner = (int32_t *)(void *)target;
                break;
            }
            if ((type->flag & UINT16_C(1)) == 0) {
                break;
            }
            if ((type->flag & UINT16_C(0x40)) != 0 &&
                trigger == 0) {
                bullet_spawn_effect(
                    (int)type->rangeEffect,
                    &proj->pj_Start,
                    NULL);
                proj->pj_Flags |= UINT32_C(0x20);
                if (owner != NULL) {
                    mask = bullet_owner_mask(owner);
                }
                trigger = 1;
            }
        }
        object = physics_FindWithinRange(
            &proj->pj_Start, &mask, 0x200);
    }

    if (kill != 0) {
        if (proj->pj_Type == 6) {
            n = BigBlowMe(&proj->pj_Start, 2);
            if (n != 0) {
                LaunchMapAnimEffects(
                    n,
                    &proj->pj_Start,
                    (int32_t *)(void *)gaScratch);
            }
            camera_SetShake(6);
        }
        goto remove_projectile;
    }

    if (type->bulletFXRate != 0 &&
        proj->pj_Range % (int)type->bulletFXRate == 0) {
        temp.vx = proj->pj_Dir.vx;
        temp.vy = proj->pj_Dir.vy;
        temp.vz = proj->pj_Dir.vz;
        temp.pad = 0;
        vec_ScaleVector(&temp, (int)proj->pj_Dir.speed);
        bullet_spawn_effect(
            (int)type->bulletEffect,
            &proj->pj_Start,
            &temp);
    }
    return 0;

remove_projectile:
    sprite_RemoveProjectile(proj);
    return 1;
}

/* 0x23070, 3 bytes, global, 1 named locals
 * bullet_Dummy
 * PDB type: int (long*)
 * Source: W:\SWJediPowerBattles\Work\bullet.c
 */
int bullet_Dummy(int32_t *unused)
{
    (void)unused;
    return 0;
}

/* 0x23080, 124 bytes, global, 7 named locals
 * bullet_Explosion
 * PDB type: void (VECTOR*, unsigned long*, int, int)
 * Source: W:\SWJediPowerBattles\Work\bullet.c
 */
void bullet_Explosion(
    VECTOR *pos, uint32_t *mask, int radius, int bomb)
{
    ProjType *type = &bullet_projectile_types()[bomb];
    physicsObject *object =
        physics_FindWithinRange(pos, mask, radius);

    while (object != NULL) {
        sceneObject *scene =
            (sceneObject *)(void *)object->physicsRoot.pParent;
        playerObject *target =
            (playerObject *)(void *)scene->pPlayer;

        target->hitNumber = (uint8_t)(target->hitNumber + 1u);
        target->projectile = type;
        object = physics_FindWithinRange(pos, mask, radius);
    }
}

/* 0x23100, 5 bytes, global, 1 named locals
 * bullet_FreeProjectile
 * PDB type: void (Projectile*)
 * Source: W:\SWJediPowerBattles\Work\bullet.c
 */
void bullet_FreeProjectile(Projectile *proj)
{
    memfree(proj);
}

/* 0x23110, 608 bytes, global, 0 named locals
 * bullet_InitProjectilePool
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\bullet.c
 */
void bullet_InitProjectilePool(void)
{
    ProjType *types = bullet_projectile_types();

    strncpy(types[6].hitSound, "explomed", 8);
    strncpy(types[7].hitSound, "explosm", 8);
    strncpy(types[9].hitSound, "explosm", 8);
    strncpy(types[8].hitSound, "explosm", 8);
    strncpy(types[23].hitSound, "gungball", 8);
    strncpy(types[24].hitSound, "pulsgrnd", 8);
    strncpy(types[16].hitSound, "pushjedi", 8);
    strncpy(types[5].hitSound, "pushjedi", 8);
    strncpy(types[17].hitSound, "explomed", 8);

    strncpy(types[0].fireSound, "barnlasr", 8);
    strncpy(types[2].fireSound, "tankfire", 8);
    strncpy(types[3].fireSound, "jawalasr", 8);
    strncpy(types[4].fireSound, "tanklasr", 8);
    strncpy(types[12].fireSound, "tuskrifl", 8);
    strncpy(types[18].fireSound, "amidlasr", 8);
    strncpy(types[1].fireSound, "jawalasr", 8);
    strncpy(types[17].fireSound, "riflfire", 8);
    strncpy(types[26].fireSound, "stapfire", 8);
    strncpy(types[27].fireSound, "dstfire1", 8);
    strncpy(types[21].fireSound, "thuglasr", 8);
    strncpy(types[29].fireSound, "probfire", 8);
    strncpy(types[30].fireSound, "amidlasr", 8);
    strncpy(types[19].fireSound, "plasma", 8);
    strncpy(types[10].fireSound, "", 8);
}

/* 0x23370, 909 bytes, global, 9 named locals
 * bullet_ShootProjectile
 * PDB type: void (Projectile*, playerObject*...
 * Source: W:\SWJediPowerBattles\Work\bullet.c
 */
void bullet_ShootProjectile(
    Projectile *proj,
    playerObject *player,
    VECTOR *pos0,
    VECTOR *pos1,
    _svector *vel)
{
    ProjType *types = bullet_projectile_types();
    ProjType *type;
    VECTOR *cpos;
    _svector rot;
    Sprite *sptr;
    Sprite **s;
    int sound_bank;

    if ((GameStruct.GameState & UINT32_C(0x02000000)) != 0) {
        return;
    }
    type = &types[proj->pj_Type];
    proj->pj_Start.vx = pos0->vx;
    proj->pj_Start.vy = pos0->vy;
    proj->pj_Start.vz = pos0->vz;
    cpos = (proj->pj_Flags & UINT32_C(0x10)) != 0
        ? pos1
        : pos0;
    normalize(
        (int)(int16_t)(pos1->vx - pos0->vx),
        (int)(int16_t)(pos1->vy - pos0->vy),
        (int)(int16_t)(pos1->vz - pos0->vz),
        (_svector *)(void *)&proj->pj_Dir);
    proj->pj_Dir.speed = (int16_t)((int16_t)type->speed << 3);
    if (proj->pj_Type == 0x11 && player->playernum < 2) {
        proj->pj_Dir.speed =
            (int16_t)((int16_t)types[18].speed << 3);
    }
    if (vel != NULL && vel->pad == 0x5432) {
        proj->pj_Dir.speed = (int16_t)(proj->pj_Dir.speed * 2);
    }
    proj->pj_Range = (int16_t)((int16_t)type->range * 2);
    proj->pj_Owner = (int32_t *)(void *)player;
    proj->pj_Target = (int32_t *)(void *)player->target;
    proj->pj_User = (int32_t *)(void *)*player->pMotion;
    vec_RotFromNormalS(&rot, (_svector *)(void *)&proj->pj_Dir);

    if ((type->flag & UINT16_C(8)) == 0) {
        sptr = sprite_Get3DProjectile(proj, cpos, &rot);
        if (sptr != NULL) {
            if (type->muzzelEffect >= 0) {
                EffectHeader *effect = paEffects[type->muzzelEffect];

                sprite_AddSpriteEffect(
                    effect->aEffects, (int)effect->num, cpos, vel);
            }
            sprite_SetProjectile(
                sptr,
                proj,
                (int32_t *)(void *)bullet_CallBack);
            proj->pj_Parent->sp_SCB->scb_cvertex.pad = type->clut;
            proj->pj_Child->sp_SCB->scb_cvertex.pad = type->clut;
            proj->pj_Parent->sp_SCB->scb_vertex0.pad = 0xd0;
            proj->pj_Child->sp_SCB->scb_vertex0.pad = 0xd0;
            proj->pj_Child->sp_Func =
                (SpriteFunction)(void *)bullet_Dummy;
            TerminateSFXString(terminatedSound, type->fireSound);
            sound_bank = player->playernum + 1;
            if (sound_bank > 3) {
                sound_bank = 3;
            }
            (void)sound_Play(pos0, sound_bank, terminatedSound, 0);
            if (player->playernum < 2) {
                feedback_startEffect(
                    player->playernum,
                    type->hitEffect == 0x11 ? 9 : 8);
            }
            bullet_observe_launch(proj, player, pos0, pos1);
            return;
        }
    } else {
        EffectHeader *effect = paEffects[type->bulletSprite];

        s = sprite_AddSpriteEffect(
            effect->aEffects, (int)effect->num, cpos, NULL);
        if (s == NULL) {
            return;
        }
        sptr = *s;
        if (sptr != NULL) {
            if (proj->pj_Type == 6 || proj->pj_Type == 9) {
                s = sprite_AddSpriteEffect(
                    effect->aEffects, (int)effect->num, cpos, NULL);
                proj->pj_Child = *s;
                proj->pj_Child->sp_Time = 0x20;
                proj->pj_Child->sp_Func =
                    (SpriteFunction)(void *)bullet_Dummy;
                memset(&proj->pj_Child->sp_Vel, 0, 3 * sizeof(float));
                memset(&proj->pj_Child->sp_Acc, 0, 3 * sizeof(float));
            }
            if (type->muzzelEffect >= 0) {
                EffectHeader *muzzle = paEffects[type->muzzelEffect];

                sprite_AddSpriteEffect(
                    muzzle->aEffects, (int)muzzle->num, cpos, vel);
            }
            memset(&sptr->sp_Vel, 0, 3 * sizeof(float));
            memset(&sptr->sp_Acc, 0, 3 * sizeof(float));
            sprite_SetProjectile(
                sptr,
                proj,
                (int32_t *)(void *)bullet_CallBack);
            TerminateSFXString(terminatedSound, type->fireSound);
            sound_bank = player->playernum + 1;
            if (sound_bank > 3) {
                sound_bank = 3;
            }
            (void)sound_Play(pos0, sound_bank, terminatedSound, 0);
            bullet_observe_launch(proj, player, pos0, pos1);
            return;
        }
    }
    bullet_FreeProjectile(proj);
}
