/*
 * COMPLETE REVIEWED RECONSTRUCTION.
 *
 * All 15 emitted procedures were checked against matched PDB symbols and
 * types plus direct disassembly/decompilation of the shipped executable at
 * RVAs 0x946B0..0x986BB. Editor state, packed effect/projectile mutations,
 * key-read order, save/load behavior, audition dispatch, and retail bounds
 * (including intentional wraparound and missing returns) are preserved.
 * PDB source: W:\SWJediPowerBattles\work\ferret.c
 */
#include "jpb/ferret.h"

#include "jpb/bullet.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/console.h"
#include "jpb/debugtext.h"
#include "jpb/effects.h"
#include "jpb/filesys.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/io.h"
#include "jpb/resources.h"
#include "jpb/whook.h"
#include "jpb/wrender.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int SAVEMODE;
EffectHeader effEdit[128];
EffectData sfx_Buffer;
EffectData ed[16];
ProjType *pj;
int spin;
char *spinName[6] = {"w", "e", "a", "s", "e", "l"};
char *sParticles[15] = {
    "rate", "count", "velocity min", "velocity max", "spread",
    "tail on/off", "start rgb", "end   rgb", "trans mode",
    "color speed", "call", "v1", "v2", "v3", "death speed"};

static uint32_t gOldBits;
static uint32_t gOldBits2;
static int saveNum;
static int32_t savedLength;
static SCB *scb[16];
static SCB *big;

static int ferret_UsedEditCount(void)
{
    int used = 0;
    int index;

    for (index = 0; index < 16; ++index) {
        if (ed[index].bank != 0) {
            ++used;
        }
    }
    return used;
}

/* PDB 0x946B0; shipped body size 996 bytes. */
int ferret_AuditionEffect(int32_t *cpad, VECTOR *loc)
{
    static int showme;
    char fileName[32];
    EffectHeader buffer;
    int index;

    if (KeyPressed(0x1d)) {
        ++showme;
    }
    if (KeyPressed(0x1c)) {
        --showme;
    }
    if (showme < 0) {
        showme = gMaxEffect - 1;
    }
    if (showme > gMaxEffect - 1) {
        showme = 0;
    }

    saveNum = showme;
    debug_printf("\t\tshowing effect %d press sqr to edit\n");
    debug_printf("\t\tsize %d\n", paEffects[showme]->num);
    if (paEffects[showme]->num > 15) {
        debug_printf("\t\terror!!!!!\n");
        return 1;
    }

    if (KeyPressed(0x13)) {
        memset(ed, 0, sizeof(ed));
        memcpy(ed, paEffects[showme]->aEffects,
            paEffects[showme]->num * sizeof(EffectData));
        SAVEMODE = 1;
        ferret_SaveLoadEffect(
            (uint32_t *)cpad, ferret_UsedEditCount());
        return 1;
    }

    if (KeyPressed(1)) {
        SAVEMODE = 0;
        for (index = 0; index < gMaxEffect; ++index) {
            EffectHeader *effect = paEffects[index];
            const char *fullFilePath;

            sprintf(fileName, "f%d.eff", index);
            fullFilePath = resource_getPath(
                fileName, JPB_RESOURCE_EFFECT);
            memset(&buffer.aEffects, 0, sizeof(buffer.aEffects));
            buffer.num = effect->num;
            memcpy(buffer.aEffects, effect->aEffects,
                effect->num * sizeof(EffectData));
            if (file_WriteFile((char *)fullFilePath, (char *)&buffer,
                    (int32_t)(sizeof(buffer.num) +
                        effect->num * sizeof(EffectData))) != 1) {
                break;
            }
        }
    }

    if (KeyPressed(10)) {
        memset(ed, 0, sizeof(ed));
        memcpy(ed, paEffects[showme]->aEffects,
            paEffects[showme]->num * sizeof(EffectData));
        return 2;
    }
    if (KeyPressed(0x20)) {
        sprite_AddSpriteEffect(
            paEffects[showme]->aEffects,
            (int)paEffects[showme]->num,
            loc,
            NULL);
    }
    return 1;
}

/* PDB 0x94AA0; shipped body size 577 bytes. */
void ferret_Control(void)
{
    static int mode = 1;
    VECTOR show;
    uint32_t cpad[2];
    int index;

    if (effEdit[0].num == 0 && (int)(gMaxEffect - 1U) > 0) {
        for (index = 0; index < gMaxEffect - 1; ++index) {
            int len = (int)paEffects[index]->num;
            if (len < 1) {
                len = 1;
            } else if (len > 15) {
                len = 15;
            }
            memcpy(&effEdit[index], paEffects[index],
                sizeof(uint32_t) + (size_t)len * sizeof(EffectData));
            paEffects[index] = &effEdit[index];
        }
    }

    show.vx = gpWorld->p0location.vx;
    show.vy = gpWorld->p0location.vy + 0x100;
    show.vz = gpWorld->p0location.vz;
    show.pad = 0;
    cpad[0] = input_ReadControlPad(0, 0, &gOldBits);
    cpad[1] = input_ReadControlPad(0, UINT32_MAX, &gOldBits2);
    game_gSetGameFlags(0x800);

    switch ((int8_t)LastKey()) {
    case -15:
        mode = 1;
        break;
    case -14:
        mode = 2;
        break;
    case -13:
        mode = 3;
        break;
    case -12:
        mode = 4;
        break;
    case 0x1b:
        mode = 5;
        break;
    }

    if (mode == 1) {
        mode = ferret_AuditionEffect((int32_t *)cpad, &show);
    } else if (mode == 2) {
        ferret_SpriteFerret((int32_t *)cpad, &show);
    } else if (mode == 3) {
        ferret_ParticleEditor((int32_t *)cpad, &show);
    } else if (mode == 4) {
        ferret_EditProjectile(cpad);
    } else if (mode == 5) {
        game_gClrGameFlags(0x800);
        game_gClrGameFlags(0x800000);
        camera_SetCurrentCameraType(1);
        if (big != NULL) {
            sprite_gFreeSCB(big);
            big = NULL;
        }
        for (index = 0; index < 16; ++index) {
            if (scb[index] != NULL) {
                sprite_gFreeSCB(scb[index]);
            }
            scb[index] = NULL;
        }
        console_ReleaseKeyboard();
    }
}

/* PDB 0x94CF0; the ignored pointer parameter is shipped behavior. */
int32_t ferret_CountSprites(EffectData *eff, int len)
{
    int used = 0;
    int index;

    (void)eff;
    for (index = 0; index < len; ++index) {
        if (ed[index].bank != 0) {
            ++used;
        }
    }
    return used;
}

/* PDB 0x94D20; shipped body size 2,472 bytes. */
void ferret_EditProjectile(uint32_t *cpad)
{
    static VECTOR offset = {0x40, 0x200, 0x200, 0};
    static int n;
    static int row;
    static int edit;
    static int count;
    VECTOR *center = coll_GetNodeCenter(0, 0);
    int delta;
    int index;

    pj = &((ProjType *)maProjTypes)[n];
    debug_printf("\n\n\n\n\n\n\t\t[%2d] TWITCHING MONGOOSE(c)\n");
    for (index = 0; index < edit; ++index) {
        debug_printf(row == 2 ? "    -" : "    \t");
    }
    debug_printf(row == 2 ? "    %s\n" : "  %s\n", spinName[spin++]);
    if (spin < 0) {
        spin = 5;
    } else if (spin > 5) {
        spin = 0;
    }

    if (KeyPressed(9)) {
        if (ShiftKeyDown() && KeyPressed(9)) {
            --n;
        } else if (KeyPressed(9)) {
            ++n;
        }
        if (n < 0) {
            n = 0;
        } else if (n > 0x20) {
            n = 0x20;
        }
    }

    if (KeyPressed(0x13)) {
        SAVEMODE = 1;
    } else if (SAVEMODE == 0) {
        if (cpad[1] & 0x2000) offset.vx += 0x10;
        if (cpad[1] & 0x8000) offset.vx -= 0x10;
        if (cpad[1] & 8) offset.vy += 0x10;
        if (cpad[1] & 4) offset.vy -= 0x10;
        if (cpad[1] & 0x1000) offset.vz += 0x10;
        if (cpad[1] & 0x4000) offset.vz -= 0x10;
        debug_printf("test target projectile %d,%d,%d\n");

        if (KeyPressed(0x1c)) --edit;
        if (KeyPressed(0x1d)) ++edit;
        delta = KeyPressed(0x1e) != 0;
        if (KeyPressed(0x1f)) delta = -1;

        if (row == 0) {
            debug_printf(" rng\t rad\t len\t wid\t muz\t bul\t hit\t rem\n");
            debug_printf("%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\n",
                pj->range, pj->radius, pj->length, pj->width,
                pj->muzzelEffect, pj->bulletEffect, pj->hitEffect, pj->removeEffect);
            if (edit < 0) edit = 7;
            else if (edit > 7) edit = 0;
        }
        if (row == 1) {
            debug_printf(" die\t spr\t spd\t rct\t dmg\t fxr\tclut\n");
            debug_printf("%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\n",
                pj->rangeEffect, pj->bulletSprite, pj->speed,
                pj->hitReact, pj->damage, pj->bulletFXRate, pj->clut);
            if (edit < 0) edit = 6;
            else if (edit > 6) edit = 0;
        }
        if (row == 2) {
            static const char *off = "OFF";
            static const char *on = " ON";
            static const uint16_t masks[9] = {
                1, 2, 4, 8, 0x80, 0x10, 0x40, 0x100, 0x4000};
            debug_printf(" area grav nrft __2d bnce barl expl trak nblk\n");
            debug_printf("  %s  %s  %s  %s  %s  %s  %s  %s  %s\n",
                (pj->flag & masks[0]) ? on : off,
                (pj->flag & masks[1]) ? on : off,
                (pj->flag & masks[2]) ? on : off,
                (pj->flag & masks[3]) ? on : off,
                (pj->flag & masks[4]) ? on : off,
                (pj->flag & masks[5]) ? on : off,
                (pj->flag & masks[6]) ? on : off,
                (pj->flag & masks[7]) ? on : off,
                (pj->flag & masks[8]) ? on : off);
            if (edit < 0) edit = 8;
            else if (edit > 8) edit = 0;
        }

        if (KeyPressed(0x98)) ++row;
        if (KeyPressed(0x97)) --row;
        if (row < 0) row = 2;
        else if (row > 2) row = 0;

        switch (edit + row * 8) {
        case 0: pj->range = (int8_t)(pj->range + delta); break;
        case 1: pj->radius = (uint8_t)(pj->radius + delta); break;
        case 2: pj->length = (uint8_t)(pj->length + delta); break;
        case 3: pj->width = (uint8_t)(pj->width + delta); break;
        case 4:
            pj->muzzelEffect = (int8_t)(pj->muzzelEffect + delta);
            if (pj->muzzelEffect < -1)
                pj->muzzelEffect = -1;
            else if (pj->muzzelEffect > gMaxEffect - 1)
                pj->muzzelEffect = (int8_t)(gMaxEffect - 1);
            break;
        case 5:
            pj->bulletEffect = (int8_t)(pj->bulletEffect + delta);
            if (pj->bulletEffect < -1)
                pj->bulletEffect = -1;
            else if (pj->bulletEffect > gMaxEffect - 1)
                pj->bulletEffect = (int8_t)(gMaxEffect - 1);
            break;
        case 6:
            pj->hitEffect = (int8_t)(pj->hitEffect + delta);
            if (pj->hitEffect < -1)
                pj->hitEffect = -1;
            else if (pj->hitEffect > gMaxEffect - 1)
                pj->hitEffect = (int8_t)(gMaxEffect - 1);
            break;
        case 7:
            pj->removeEffect = (int8_t)(pj->removeEffect + delta);
            if (pj->removeEffect < -1)
                pj->removeEffect = -1;
            else if (pj->removeEffect > gMaxEffect - 1)
                pj->removeEffect = (int8_t)(gMaxEffect - 1);
            break;
        case 8:
            pj->rangeEffect = (int8_t)(pj->rangeEffect + delta);
            if (pj->rangeEffect < -1)
                pj->rangeEffect = -1;
            else if (pj->rangeEffect > gMaxEffect - 1)
                pj->rangeEffect = (int8_t)(gMaxEffect - 1);
            break;
        case 9:
            pj->bulletSprite = (int8_t)(pj->bulletSprite + delta);
            if (pj->bulletSprite > gMaxEffect - 1)
                pj->bulletSprite = (int8_t)(gMaxEffect - 1);
            break;
        case 10: pj->speed = (int8_t)(pj->speed + delta); break;
        case 11: pj->hitReact = (uint8_t)(pj->hitReact + delta); break;
        case 12: pj->damage = (uint8_t)(pj->damage + delta); break;
        case 13:
            pj->bulletFXRate = (uint8_t)(pj->bulletFXRate + delta);
            if (pj->bulletFXRate > gMaxEffect - 1)
                pj->bulletFXRate = (uint8_t)(gMaxEffect - 1);
            break;
        case 14:
            pj->clut = (uint8_t)(pj->clut + delta);
            if (pj->clut > 0x40) pj->clut = 0x40;
            break;
        case 16: case 17: case 18: case 19: case 20:
        case 21: case 22: case 23: case 24:
            if (delta != 0) {
                static const uint16_t masks[9] = {
                    1, 2, 4, 8, 0x80, 0x10, 0x40, 0x100, 0x4000};
                pj->flag ^= masks[edit];
            }
            break;
        }

        if (KeyPressed(0x20)) {
            Projectile *projectile;
            VECTOR target;

            count = 0;
            projectile = bullet_AllocProjectile(n);
            if (projectile != NULL) {
                target.vx = center->vx + offset.vx;
                target.vy = center->vy + offset.vy;
                target.vz = center->vz + offset.vz;
                target.pad = 0;
                if (n == 0x13) {
                    uint32_t value = (uint32_t)(rand() % 100 + 0x40);
                    uint32_t component = value >> 3;
                    projectile->pj_Flags |= 0x2000;
                    projectile->launchID = 0;
                    projectile->color =
                        (int32_t)(((component << 8) | value) << 8 | component);
                }
                bullet_ShootProjectile(
                    projectile, &gaPlayerData[0], center, &target, NULL);
            }
        }
        return;
    }
    ferret_SaveLoadProjectiles(cpad);
}

/* PDB 0x956D0; shipped body size 2,324 bytes. */
void ferret_EditRing(uint32_t *cpad, RingData *ring)
{
    static int row;
    static int edit;
    int delta;
    int toggle_delta = 0;
    int index;
    int value;
    uint16_t *flags = &ring->rot.pad;

    (void)cpad;
    if (KeyPressed(0x1c)) --edit;
    if (KeyPressed(0x1d)) ++edit;
    delta = KeyPressed(0x1e) != 0;
    if (KeyPressed(0x1f)) delta = -1;
    if (ShiftKeyDown()) {
        toggle_delta = KeyPressed(0x1e) != 0;
        if (KeyPressed(0x1f)) toggle_delta = -1;
    }

    if (row == 0) {
        debug_printf("ring effectors\n");
        debug_printf("sprt\t brt\t vel\t acc\t lim\tclut\tlife\n");
        debug_printf("%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\n",
            ring->type, ring->b.init, ring->b.vel, ring->b.acc,
            ring->b.limit, ring->clut, ring->time);
        if (edit < 0) edit = 6;
        else if (edit > 6) edit = 0;
    }
    if (row == 1) {
        debug_printf("ring edge 1\n");
        debug_printf(" rad\t vel\t acc\t lim\t hgt\t vel\t acc\t lim\n");
        debug_printf("%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\n",
            ring->r1.init, ring->r1.vel, ring->r1.acc, ring->r1.limit,
            ring->h1.init, ring->h1.vel, ring->h1.acc, ring->h1.limit);
        if (edit < 0) edit = 7;
        else if (edit > 7) edit = 0;
    }
    if (row == 2) {
        debug_printf("ring edge 2\n");
        debug_printf(" rad\t vel\t acc\t lim\t hgt\t vel\t acc\t lim\n");
        debug_printf("%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\n",
            ring->r2.init, ring->r2.vel, ring->r2.acc, ring->r2.limit,
            ring->h2.init, ring->h2.vel, ring->h2.acc, ring->h2.limit);
        if (edit < 0) edit = 7;
        else if (edit > 7) edit = 0;
    }
    if (row == 3) {
        debug_printf("ring spin\n");
        debug_printf("  rx\t  ry\t  rz\trvel\tratio\tmode\n");
        debug_printf("%4d\t%4d\t%4d\t%4d\t%4d\t%4d\n",
            ring->rot.vx, ring->rot.vy, ring->rot.vz,
            ring->rvel, ring->ratio, ring->tmode);
        if (edit < 0) edit = 5;
        else if (edit > 5) edit = 0;
    }
    if (row == 4) {
        static const char *off = "OFF";
        static const char *on = " ON";
        debug_printf("ring ping\n");
        debug_printf("    b   r1   r2   SP   NT\n");
        debug_printf("  %s  %s  %s  %s  %s\n",
            (*flags & 1) ? on : off,
            (*flags & 2) ? on : off,
            (*flags & 8) ? on : off,
            (*flags & 0x40) ? on : off,
            (*flags & 0x80) ? on : off);
        if (edit < 0) edit = 5;
        else if (edit > 5) edit = 0;
    }

    if (KeyPressed(0x98)) ++row;
    if (KeyPressed(0x97)) --row;
    if (row < 0) row = 4;
    else if (row > 4) row = 0;
    index = edit + row * 8;
    for (value = 0; value < edit; ++value) debug_printf("    \t");
    debug_printf("  %s\n", spinName[spin++]);
    if (spin < 0) spin = 5;
    else if (spin > 5) spin = 0;

#define FERRET_CLAMP(v, low, high) \
    ((v) < (low) ? (low) : ((v) > (high) ? (high) : (v)))
    switch (index) {
    case 0:
        value = (int)ring->type + delta;
        if (value < 0) value = 0x32;
        if (value > 0x32) value = 0;
        ring->type = (uint8_t)value;
        break;
    case 1:
        value = ring->b.init + delta;
        if (value < 0) value = 0xff;
        if (value > 0xff) value = 0;
        ring->b.init = (int16_t)value;
        break;
    case 2:
        value = ring->b.vel + delta;
        ring->b.vel = (int8_t)(value <= -0x80 ? -0x80 : (value > 0x80 ? -0x7f : value));
        break;
    case 3:
        value = ring->b.acc + delta;
        ring->b.acc = (int8_t)(value <= -0x80 ? -0x80 : (value > 0x80 ? -0x7f : value));
        break;
    case 4:
        value = ring->b.limit + delta;
        if (value < 0) value = 0xff;
        if (value > 0xff) value = 0;
        ring->b.limit = (int16_t)value;
        break;
    case 5:
        value = ring->clut + delta;
        if (value < 0) value = 0xff;
        if (value > 0xff) value = 0;
        ring->clut = (uint8_t)value;
        break;
    case 6:
        ring->time = (uint32_t)FERRET_CLAMP((int)ring->time + delta * 0x200, 0, 0x38400);
        break;
    case 8: ring->r1.init = (int16_t)FERRET_CLAMP(ring->r1.init + delta * 0x10, 0, 0xc00); break;
    case 9: ring->r1.vel = (int8_t)FERRET_CLAMP(ring->r1.vel + delta, -0xff, 0xff); break;
    case 10: ring->r1.acc = (int8_t)FERRET_CLAMP(ring->r1.acc + delta, -0xff, 0xff); break;
    case 11: ring->r1.limit = (int16_t)FERRET_CLAMP(ring->r1.limit + delta, -0xc00, 0xc00); break;
    case 12: ring->h1.init = (int16_t)FERRET_CLAMP(ring->h1.init + delta * 0x10, -0x600, 0x600); break;
    case 13: ring->h1.vel = (int8_t)FERRET_CLAMP(ring->h1.vel + delta, -0xff, 0xff); break;
    case 14: ring->h1.acc = (int8_t)FERRET_CLAMP(ring->h1.acc + delta, -0xff, 0xff); break;
    case 15: ring->h1.limit = (int16_t)FERRET_CLAMP(ring->h1.limit + delta, -0x600, 0x600); break;
    case 16: ring->r2.init = (int16_t)FERRET_CLAMP(ring->r2.init + delta * 0x10, 0, 0xc00); break;
    case 17: ring->r2.vel = (int8_t)FERRET_CLAMP(ring->r2.vel + delta, -0xff, 0xff); break;
    case 18: ring->r2.acc = (int8_t)FERRET_CLAMP(ring->r2.acc + delta, -0xff, 0xff); break;
    case 19: ring->r2.limit = (int16_t)FERRET_CLAMP(ring->r2.limit + delta, -0xc00, 0xc00); break;
    case 20: ring->h2.init = (int16_t)FERRET_CLAMP(ring->h2.init + delta * 0x10, -0x600, 0x600); break;
    case 21: ring->h2.vel = (int8_t)FERRET_CLAMP(ring->h2.vel + delta, -0xff, 0xff); break;
    case 22: ring->h2.acc = (int8_t)FERRET_CLAMP(ring->h2.acc + delta, -0xff, 0xff); break;
    case 23: ring->h2.limit = (int16_t)FERRET_CLAMP(ring->h2.limit + delta, -0x600, 0x600); break;
    case 24: ring->rot.vx = (int16_t)FERRET_CLAMP(ring->rot.vx + delta * 8, 0, 0x1000); break;
    case 25: ring->rot.vy = (int16_t)FERRET_CLAMP(ring->rot.vy + delta * 8, 0, 0x1000); break;
    case 26: ring->rot.vz = (int16_t)FERRET_CLAMP(ring->rot.vz + delta * 8, 0, 0x1000); break;
    case 27: ring->rvel = (int8_t)FERRET_CLAMP(ring->rvel + delta * 4, -0x7f, 0x80); break;
    case 28: ring->ratio = (uint8_t)FERRET_CLAMP((int)ring->ratio + delta * 4, 0, 0xff); break;
    case 29:
        value = (int)ring->tmode + delta;
        if (value > 4) value = 4;
        ring->tmode = (uint8_t)value;
        break;
    case 32: if (toggle_delta) *flags ^= 1; break;
    case 33: if (toggle_delta) *flags ^= 2; break;
    case 34: if (toggle_delta) *flags ^= 8; break;
    case 35: if (toggle_delta) *flags ^= 0x40; break;
    case 36: if (toggle_delta) *flags ^= 0x80; break;
    }
#undef FERRET_CLAMP
}

/* PDB 0x95FF0; shipped body size 1,429 bytes. */
void ferret_EmmbeddedBank(int32_t *cpad, VECTOR *loc, int n)
{
    static int row;
    static int edit;
    static const char *off = "OFF";
    static const char *on = " ON";
    EffectData *effect = &ed[n];
    int delta;
    int index;

    (void)cpad;
    (void)loc;
    if (KeyPressed(0x1c)) {
        --edit;
    }
    if (KeyPressed(0x1d)) {
        ++edit;
    }
    delta = KeyPressed(0x1e) != 0;
    if (KeyPressed(0x1f)) {
        delta = -1;
    }
    if (KeyPressed(0x98)) {
        ++row;
    }
    if (KeyPressed(0x97)) {
        --row;
    }
    if (row < 0) {
        row = 2;
    } else if (row > 2) {
        row = 0;
    }

    debug_printf("embedded\n");
    if (row == 0) {
        debug_printf(" typ\t dly\t rpt\toffx\toffy\toffz\n");
        debug_printf("%4d\t%4d\t%4d\t%4d\t%4d\t%4d\n",
            effect->type, effect->delay, effect->acc.vx,
            effect->pos.vx, effect->pos.vy, effect->pos.vz);
        switch (edit) {
        case 0:
            effect->type = (uint8_t)(effect->type + delta);
            break;
        case 1:
            effect->delay = (int16_t)(effect->delay + delta);
            break;
        case 2:
            effect->acc.vx = (int16_t)(effect->acc.vx + delta);
            break;
        case 3:
            effect->pos.vx = (int16_t)(effect->pos.vx + delta * 8);
            break;
        case 4:
            effect->pos.vy = (int16_t)(effect->pos.vy + delta * 8);
            break;
        case 5:
            effect->pos.vz = (int16_t)(effect->pos.vz + delta * 8);
            break;
        }
        if (edit < 0) {
            edit = 0;
        } else if (edit > 5) {
            edit = 5;
        }
        if (effect->type > gMaxEffect) {
            effect->type = (uint8_t)gMaxEffect;
        }
        if (effect->pos.vx < -0x200) {
            effect->pos.vx = -0x200;
        } else if (effect->pos.vx > 0x200) {
            effect->pos.vx = 0x200;
        }
        if (effect->pos.vy < -0x200) {
            effect->pos.vy = -0x200;
        } else if (effect->pos.vy > 0x200) {
            effect->pos.vy = 0x200;
        }
        if (effect->pos.vz < -0x200) {
            effect->pos.vz = -0x200;
        } else if (effect->pos.vz > 0x200) {
            effect->pos.vz = 0x200;
        }
    }

    if (row == 1) {
        debug_printf("RNDP\tFLSH\tSHAK\n");
        debug_printf(" %s\t %s\t %s\n",
            (effect->flags & 1) != 0 ? on : off,
            (effect->flags & 2) != 0 ? on : off,
            (effect->flags & 4) != 0 ? on : off);
        if (delta != 0) {
            if (edit == 0) {
                effect->flags ^= 1;
            } else if (edit == 1) {
                effect->flags ^= 2;
            } else if (edit == 2) {
                effect->flags ^= 4;
            }
        }
        if (edit < 0) {
            edit = 0;
        } else if (edit > 2) {
            edit = 2;
        }
    }

    if (row == 2) {
        MATRIX matrix = {
            {{1.0f, 0.0f, 0.0f},
             {0.0f, 1.0f, 0.0f},
             {0.0f, 0.0f, 1.0f}},
            {0, 0, 0}};
        _svector end = {0, 0x200, 0, 0};

        debug_printf("rotx\troty\trotz\tsecd\n");
        debug_printf("%4d\t%4d\t%4d\t%4d\n",
            effect->vel.vx, effect->vel.vy,
            effect->vel.vz, effect->acc.vy);
        switch (edit) {
        case 0:
            effect->vel.vx = (int16_t)(effect->vel.vx + delta * 0x40);
            break;
        case 1:
            effect->vel.vy = (int16_t)(effect->vel.vy + delta * 0x40);
            break;
        case 2:
            effect->vel.vz = (int16_t)(effect->vel.vz + delta * 0x40);
            break;
        case 3:
            effect->acc.vy = (int16_t)(effect->acc.vy + delta);
            break;
        }
        if (edit < 0) {
            edit = 0;
        } else if (edit > 3) {
            edit = 3;
        }
        if (effect->vel.vx < 0) {
            effect->vel.vx = 0;
        } else if (effect->vel.vx > 0x1000) {
            effect->vel.vx = 0x1000;
        }
        if (effect->vel.vy < 0) {
            effect->vel.vy = 0;
        } else if (effect->vel.vy > 0x1000) {
            effect->vel.vy = 0x1000;
        }
        if (effect->vel.vz < 0) {
            effect->vel.vz = 0;
        } else if (effect->vel.vz > 0x1000) {
            effect->vel.vz = 0x1000;
        }
        if (delta != 0) {
            PushMatrix();
            fRotMatrix(&effect->vel, &matrix);
            fApplyMatrixSV(&matrix, &end, &end);
            PopMatrix();
        }
    }

    for (index = 0; index < edit; ++index) {
        debug_printf("    \t");
    }
    debug_printf("  %s\n", spinName[spin++]);
    if (spin < 0) {
        spin = 5;
    } else if (spin > 5) {
        spin = 0;
    }
}

/* PDB 0x96590; shipped body size 563 bytes. */
void ferret_ParticleBank(int32_t *cpad, int n)
{
    static int edit;
    EffectData *effect = &ed[n];
    int16_t delta;
    int index;

    (void)cpad;
    if (KeyPressed(0x1c)) {
        --edit;
    }
    if (KeyPressed(0x1d)) {
        ++edit;
    }
    delta = (int16_t)(KeyPressed(0x1e) != 0);
    if (KeyPressed(0x1f)) {
        delta = -1;
    }

    debug_printf("particle\n");
    debug_printf(" typ\t loc\t idx\t dly\n");
    debug_printf("%4d\t%4d\t%4d\t%4d\n",
        effect->type, effect->acc.vx, effect->acc.vy, effect->delay);
    if (edit == 0) {
        effect->type = (uint8_t)(effect->type + delta);
    } else if (edit == 1) {
        effect->acc.vx = (int16_t)(effect->acc.vx + delta);
    }
    if (edit == 2) {
        effect->acc.vy = (int16_t)(effect->acc.vy + delta);
    }
    if (edit == 3) {
        effect->delay = (int16_t)(effect->delay + delta);
    }
    if (edit < 0) {
        edit = 0;
    } else if (edit > 3) {
        edit = 3;
    }
    if (effect->bank > 4) {
        effect->bank = 4;
    }
    if (effect->type > 0x40) {
        effect->type = 0x40;
    }
    effect->acc.vx = 0;
    if (effect->acc.vy < 0) {
        effect->acc.vy = 0;
    } else if (effect->acc.vy > 0xf) {
        effect->acc.vy = 0xf;
    }

    for (index = 0; index < edit; ++index) {
        debug_printf("    \t");
    }
    debug_printf("  %s\n", spinName[spin++]);
    if (spin < 0) {
        spin = 5;
    } else if (spin > 5) {
        spin = 0;
    }
}

/* PDB 0x967D0; shipped body size 1,444 bytes. */
void ferret_ParticleEditor(int32_t *cpad, VECTOR *loc)
{
    static int eNum = 1;
    static int32_t edit;
    static _svector rot;
    static const int speed[15] = {
        0x80, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 0x20, 3};
    static char *aParticle[4] = {
        "MODE_ACC   v1=ax,     v2=ay,     v3=az",
        "MODE_RED   v1=scale,  v2=framerate",
        "MODE_ROT   v1=start,  v2=inc,    v3=scale",
        "MODE_GRAV  v1=time"};
    Emiter *emit;
    int delta = 0;
    uint32_t value = 0;
    int8_t red;
    uint8_t green;
    uint8_t blue;
    int frames;

    if (KeyPressed(0x1c)) {
        --edit;
    }
    if (KeyPressed(0x1d)) {
        ++edit;
    }
    if (KeyPressed(9)) {
        if (ShiftKeyDown() && KeyPressed(9)) {
            --eNum;
        } else if (KeyPressed(9)) {
            ++eNum;
        }
    }
    if (edit < 0) {
        edit = 0xe;
    } else if (edit > 0xe) {
        edit = 0;
    }
    if (eNum < 0) {
        eNum = 0x1f;
    } else if (eNum > 0x1f) {
        eNum = 0;
    }

    emit = &((Emiter *)aEmiter)[eNum];
    debug_printf("%s\ncall %d\tv1 %d,\tv2 %d,  v3 %d\n",
        aParticle[(int8_t)emit->mode], (int8_t)emit->mode,
        (int8_t)emit->v1, (int8_t)emit->v2, (int8_t)emit->v3);
    frames = 0x1000 / emit->colorSpeed;
    debug_printf("rgb %d-%d-%d to %d-%d-%d in %d frames\n",
        emit->sr, emit->sg, emit->sb,
        emit->er, emit->eg, emit->eb, frames);
    if (((uint32_t *)cpad)[1] & 1) {
        debug_printf("down to save\n");
        debug_printf("  up to load\n");
    }

    if (edit == 6) {
        red = emit->sr;
        green = (uint8_t)emit->sg;
        blue = (uint8_t)emit->sb;
    } else if (edit == 7) {
        red = emit->er;
        green = (uint8_t)emit->eg;
        blue = (uint8_t)emit->eb;
    } else {
        uint32_t pad = ((uint32_t *)cpad)[1];

        if ((int8_t)pad < 0) {
            rot.vx = (int16_t)(rot.vx + 0x20);
        }
        if (pad & 0x20) {
            rot.vx = (int16_t)(rot.vx - 0x20);
        }
        if (pad & 0x10) {
            rot.vz = (int16_t)(rot.vz + 0x20);
        }
        if (pad & 0x40) {
            rot.vz = (int16_t)(rot.vz - 0x20);
        }
        if (pad & 0x2000) {
            delta = speed[edit];
        }
        if (pad & 0x8000) {
            delta = -speed[edit];
        }

        switch (edit) {
        case 0:
            emit->rate = (int16_t)(emit->rate + delta);
            value = (uint32_t)(int32_t)emit->rate;
            break;
        case 1:
            emit->count = (int16_t)(emit->count + delta);
            value = (uint32_t)(int32_t)emit->count;
            break;
        case 2:
            emit->vmin = (int8_t)(emit->vmin + delta);
            value = (uint8_t)emit->vmin;
            break;
        case 3:
            emit->vmax = (int8_t)(emit->vmax + delta);
            value = (uint8_t)emit->vmax;
            break;
        case 4:
            emit->spread = (int8_t)(emit->spread + delta);
            value = (uint8_t)emit->spread;
            break;
        case 5:
            if (delta != 0) {
                emit->flags ^= 1;
            }
            value = (uint16_t)emit->flags;
            break;
        case 8:
            debug_printf("\n\t\t\ttrans disabled\n");
            break;
        case 9:
            emit->colorSpeed = (int16_t)(emit->colorSpeed + delta);
            value = (uint32_t)(int32_t)emit->colorSpeed;
            break;
        case 10:
            emit->mode = (uint8_t)((int8_t)emit->mode + delta);
            value = (uint32_t)(int32_t)(int8_t)emit->mode;
            break;
        case 11:
            emit->v1 = (uint8_t)((int8_t)emit->v1 + delta);
            value = (uint32_t)(int32_t)(int8_t)emit->v1;
            break;
        case 12:
            emit->v2 = (uint8_t)((int8_t)emit->v2 + delta);
            value = (uint32_t)(int32_t)(int8_t)emit->v2;
            break;
        case 13:
            emit->v3 = (uint8_t)((int8_t)emit->v3 + delta);
            value = (uint32_t)(int32_t)(int8_t)emit->v3;
            break;
        case 14:
            emit->deathSpeed = (int16_t)(emit->deathSpeed + delta);
            value = (uint32_t)(int32_t)emit->deathSpeed;
            break;
        }
        goto clamp_values;
    }

    if (KeyPressed(0x72)) {
        red = (int8_t)(red +
            (ShiftKeyDown() ? -(int8_t)speed[edit] : (int8_t)speed[edit]));
    }
    if (KeyPressed(0x67)) {
        red = (int8_t)(red +
            (ShiftKeyDown() ? -(int8_t)speed[edit] : (int8_t)speed[edit]));
    }
    if (KeyPressed(0x62)) {
        red = (int8_t)(red +
            (ShiftKeyDown() ? -(int8_t)speed[edit] : (int8_t)speed[edit]));
    }
    debug_printf("\n\tparticle rgb %d,%d,%d\n", red, green, blue);
    if (edit == 6) {
        emit->sr = red;
        emit->sg = (int8_t)green;
        emit->sb = (int8_t)blue;
    } else if (edit == 7) {
        emit->er = red;
        emit->eg = (int8_t)green;
        emit->eb = (int8_t)blue;
    }

clamp_values:
    if (emit->rate < 0x20) {
        emit->rate = 0x20;
    } else if (emit->rate > 0x2000) {
        emit->rate = 0x2000;
    }
    if (emit->count < 1) {
        emit->count = 1;
    } else if (emit->count > 0x40) {
        emit->count = 0x40;
    }
    if ((uint8_t)emit->spread > 8) {
        emit->spread = 8;
    }
    if ((int8_t)emit->mode < 0) {
        emit->mode = 0;
    } else if ((int8_t)emit->mode > 4) {
        emit->mode = 4;
    }
    if (emit->colorSpeed < 0x20) {
        emit->colorSpeed = 0x20;
    } else if (emit->colorSpeed > 0x1000) {
        emit->colorSpeed = 0x1000;
    }
    if (emit->deathSpeed < 0x20) {
        emit->deathSpeed = 0x20;
    } else if (emit->deathSpeed > 0x1000) {
        emit->deathSpeed = 0x1000;
    }
    if (rot.vx < 0) {
        rot.vx = 0x1000;
    } else if (rot.vx > 0x1000) {
        rot.vx = 0;
    }
    if (rot.vz < 0) {
        rot.vz = 0x1000;
    } else if (rot.vz > 0x1000) {
        rot.vz = 0;
    }

    debug_printf("\n\tparticle effect %s[%d] %d\n",
        sParticles[edit], eNum, value,
        emit->er, emit->eg, emit->eb, frames);
    debug_printf("\n\tparticle rotation %d,%d\n", rot.vx, rot.vz);
    if (((uint32_t *)cpad)[0] & 0x800) {
        emit->pos = *loc;
        emit->rot = rot;
        sprite_FireEmiter(emit, NULL);
    }
}

/* PDB 0x96D80; the original non-void procedure has no return statement. */
int ferret_SaveAllEffects(void)
{
    char fileName[32];
    EffectHeader buffer;
    int index;

    SAVEMODE = 0;
    for (index = 0; index < gMaxEffect; ++index) {
        EffectHeader *effect = paEffects[index];
        const char *fullFilePath;

        sprintf(fileName, "f%d.eff", index);
        fullFilePath = resource_getPath(fileName, JPB_RESOURCE_EFFECT);
        memset(buffer.aEffects, 0, sizeof(buffer.aEffects));
        buffer.num = effect->num;
        memcpy(buffer.aEffects, effect->aEffects,
            effect->num * sizeof(EffectData));
        if (file_WriteFile((char *)fullFilePath, (char *)&buffer,
                (int32_t)(sizeof(buffer.num) +
                    effect->num * sizeof(EffectData))) != 1) {
            break;
        }
    }
}

/* PDB 0x96ED0; the original non-void procedure has no return statement. */
int ferret_SaveLoadEffect(uint32_t *cpad, int32_t length)
{
    char fileName[32];
    struct {
        int32_t num;
        EffectData effects[16];
    } saveBuffer;
    uint8_t loadBuffer[884];
    EffectData *writeEffect = saveBuffer.effects;
    int index;

    (void)cpad;
    debug_printf("down to save\n");
    debug_printf("up to load\n");
    debug_printf("L1R1 to change number %d\n", saveNum);
    if (KeyPressed(0x1d)) {
        ++saveNum;
    }
    if (KeyPressed(0x1c)) {
        --saveNum;
    }
    if (saveNum < 0) {
        saveNum = 0xff;
    } else if (saveNum > 0xff) {
        saveNum = 0;
    }

    if (KeyPressed(0x1f)) {
        SAVEMODE = 0;
        sprintf(fileName, "f%d.eff", saveNum);
        (void)resource_getPath(fileName, JPB_RESOURCE_EFFECT);
        for (index = 0; index < 16; ++index) {
            if (ed[index].bank != 0) {
                *writeEffect++ = ed[index];
            }
        }
        saveBuffer.num = length;
        if (file_WriteFile(fileName, (char *)&saveBuffer,
                length * (int32_t)sizeof(EffectData) +
                    (int32_t)sizeof(saveBuffer.num)) != 1) {
            return;
        }
    }

    if (KeyPressed(0x1e)) {
        const char *fullFilePath;
        int32_t count;

        memset(loadBuffer, 0, sizeof(loadBuffer));
        SAVEMODE = 0;
        sprintf(fileName, "f%d.eff", saveNum);
        fullFilePath = resource_getPath(fileName, JPB_RESOURCE_EFFECT);
        file_ReadPC((char *)fullFilePath, (char *)loadBuffer);
        memcpy(&count, loadBuffer, sizeof(count));
        memset(ed, 0, sizeof(ed));
        memcpy(ed, loadBuffer + sizeof(count),
            (size_t)count * sizeof(EffectData));
    }
}

/* PDB 0x97400; both parameters are ignored in the shipped body. */
int ferret_SaveLoadParticles(Emiter *emiter, uint32_t cpad)
{
    (void)emiter;
    (void)cpad;
    debug_printf("down to save\n");
    debug_printf("  up to load\n");
}

/* PDB 0x97420; the original non-void procedure has no return statement. */
int ferret_SaveLoadProjectiles(uint32_t *cpad)
{
    const char *fullFilePath;

    (void)cpad;
    debug_printf("down to save\n");
    debug_printf("  up to load\n");
    if (KeyPressed(0x1f)) {
        SAVEMODE = 0;
        fullFilePath = resource_getPath(
            "project.eff", JPB_RESOURCE_EFFECT);
        if (file_WriteFile((char *)fullFilePath,
                (char *)maProjTypes, JPB_PROJECT_TYPE_BYTES) != 1) {
            return;
        }
    }
    if (KeyPressed(0x1e)) {
        SAVEMODE = 0;
        fullFilePath = resource_getPath(
            "project.eff", JPB_RESOURCE_EFFECT);
        file_LoadFile((char *)fullFilePath, maProjTypes);
    }
}

/* PDB 0x974D0; shipped body returns len. */
int32_t ferret_ShowEffect(EffectData *eff, VECTOR *loc, int len)
{
    Sprite *sprites[16] = {0};
    int index;

    for (index = 0; index < len; ++index) {
        switch (eff[index].bank) {
        case 1:
        case 3:
            sprites[index] = *sprite_AddSpriteEffect(
                &eff[index], 1, loc, NULL);
            break;
        case 2:
            sprite_FireEmiter(
                &((Emiter *)aEmiter)[eff[index].type],
                sprites[eff[index].delay]);
            break;
        case 4:
            sprite_FireRing((RingData *)&eff[index], loc);
            break;
        }
    }
    return len;
}

/* PDB 0x97620; shipped body size 2,436 bytes. */
void ferret_SpriteBank(int32_t *cpad, VECTOR *loc, int n)
{
    static int edit = 3;
    static int row;
    EffectData *effect = &ed[n];
    int delta;
    int index;
    int value;
    int mode;

    (void)cpad;
    (void)loc;
    if (KeyPressed(0x1c)) --edit;
    if (KeyPressed(0x1d)) ++edit;
    delta = KeyPressed(0x1e) != 0;
    if (KeyPressed(0x1f)) delta = -1;
    if (ShiftKeyDown()) delta *= 8;
    if (KeyPressed(0x98)) ++row;
    if (KeyPressed(0x97)) --row;
    if (row < 0) row = 3;
    else if (row > 3) row = 0;
    index = edit + row * 8;

    if (row == 0) {
        debug_printf("effect\n");
        debug_printf(" typ\t  rx\t rvx\toffx\toffy\toffz\n");
        debug_printf("%4d\t%4d\t%4d\t%4d\t%4d\t%4d\n",
            effect->type, effect->rx, effect->rvx,
            effect->pos.vx, effect->pos.vy, effect->pos.vz);
        if (edit < 0) edit = 5;
        else if (edit > 5) edit = 0;
    } else if (row == 1) {
        debug_printf(" BRT\tBRTR\tBRTA\tBRTL\t SCL\tSCLR\tSCLA\tSCLL\n");
        debug_printf("%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\n\n",
            effect->bright.init, effect->bright.vel,
            effect->bright.acc, effect->bright.limit,
            effect->scale.init, effect->scale.vel,
            effect->scale.acc, effect->scale.limit);
        if (edit < 0) edit = 7;
        else if (edit > 7) edit = 0;
    } else if (row == 2) {
        debug_printf("  VX\t  VY\t  VZ\t  AX\t  AY\t  AZ\t  CT\t  DLY\n");
        debug_printf("%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\t%4d\n\n",
            effect->vel.vx, effect->vel.vy, effect->vel.vz,
            effect->acc.vx, effect->acc.vy, effect->acc.vz,
            effect->vel.pad, effect->delay);
    } else if (row == 3) {
        static const char *off = "OFF";
        static const char *on = " ON";
        debug_printf("FLPA\tFLPB\tFLPC\tFLIK\tZZ90\tSHRD\tMODE\n");
        debug_printf(" %s\t %s\t %s\t %s\t %s\t %s\t%2d\n\n",
            (effect->flags & 1) ? on : off,
            (effect->flags & 2) ? on : off,
            (effect->flags & 4) ? on : off,
            (effect->flags & 8) ? on : off,
            (effect->flags & 0x10) ? on : off,
            (effect->flags & 0x20) ? on : off,
            (effect->flags >> 6) & 7);
        if (edit < 0) edit = 6;
        else if (edit > 6) edit = 0;
    }

    for (value = 0; value < edit; ++value) debug_printf("    \t");
    debug_printf("  %s\n", spinName[spin++]);
    if (spin < 0) spin = 5;
    else if (spin > 5) spin = 0;

#define FERRET_CLAMP(v, low, high) \
    ((v) < (low) ? (low) : ((v) > (high) ? (high) : (v)))
    switch (index) {
    case 0:
        effect->type = (uint8_t)FERRET_CLAMP((int)effect->type + delta, 0, 0x32);
        break;
    case 1:
        effect->rx = (int16_t)FERRET_CLAMP(effect->rx + delta * 0x20, -0x800, 0x800);
        break;
    case 2:
        effect->rvx = (int16_t)FERRET_CLAMP(effect->rvx + delta * 8, -0x800, 0x800);
        break;
    case 3: effect->pos.vx = (int16_t)FERRET_CLAMP(effect->pos.vx + delta * 8, -0x100, 0x100); break;
    case 4: effect->pos.vy = (int16_t)FERRET_CLAMP(effect->pos.vy + delta * 8, -0x100, 0x100); break;
    case 5: effect->pos.vz = (int16_t)FERRET_CLAMP(effect->pos.vz + delta * 8, -0x100, 0x100); break;
    case 8: effect->bright.init = (int16_t)FERRET_CLAMP(effect->bright.init + delta, 0, 0xff); break;
    case 9: effect->bright.vel = (int16_t)FERRET_CLAMP(effect->bright.vel + delta, -0xff, 0xff); break;
    case 10: effect->bright.acc = (int16_t)FERRET_CLAMP(effect->bright.acc + delta, -0xff, 0xff); break;
    case 11: effect->bright.limit = (int16_t)FERRET_CLAMP(effect->bright.limit + delta, -0xff, 0xff); break;
    case 12: effect->scale.init = (int16_t)FERRET_CLAMP(effect->scale.init + delta * 0x20, 0, 0x4000); break;
    case 13: effect->scale.vel = (int16_t)FERRET_CLAMP(effect->scale.vel + delta * 8, -0x4000, 0x4000); break;
    case 14: effect->scale.acc = (int16_t)FERRET_CLAMP(effect->scale.acc + delta, -0x4000, 0x4000); break;
    case 15: effect->scale.limit = (int16_t)FERRET_CLAMP(effect->scale.limit + delta * 0x20, -0x4000, 0x4000); break;
    case 16: effect->vel.vx = (int16_t)FERRET_CLAMP(effect->vel.vx + delta, -0x200, 0x200); break;
    case 17:
        value = effect->vel.vy;
        if (value < 0) value = -value;
        if (value > 0x100) delta <<= 7;
        debug_printf(value > 0x100
            ? "\t\t\t\t\tgravity on\n"
            : "\t\t\t\t\tover 256 to get gravity on\n");
        effect->vel.vy = (int16_t)FERRET_CLAMP((int)effect->vel.vy + delta, -0x8000, 0x8000);
        break;
    case 18: effect->vel.vz = (int16_t)FERRET_CLAMP(effect->vel.vz + delta, -0x200, 0x200); break;
    case 19: effect->acc.vx = (int16_t)FERRET_CLAMP(effect->acc.vx + delta, -0x200, 0x200); break;
    case 20: effect->acc.vy = (int16_t)FERRET_CLAMP(effect->acc.vy + delta, -0x200, 0x200); break;
    case 21: effect->acc.vz = (int16_t)FERRET_CLAMP(effect->acc.vz + delta, -0x200, 0x200); break;
    case 22: effect->vel.pad = (uint16_t)FERRET_CLAMP((int)effect->vel.pad + delta, 0, 0xbf); break;
    case 23:
        effect->delay = (int16_t)FERRET_CLAMP(effect->delay + delta, -0xff, 0xff);
        debug_printf("\n\t\t\t\t-premotion +postmotion\n");
        break;
    case 24: if (delta) effect->flags ^= 1; break;
    case 25: if (delta) effect->flags ^= 2; break;
    case 26: if (delta) effect->flags ^= 4; break;
    case 27: if (delta) effect->flags ^= 8; break;
    case 28: if (delta) effect->flags ^= 0x10; break;
    case 29: if (delta) effect->flags ^= 0x20; break;
    case 30:
        if (delta) {
            mode = (int)((effect->flags >> 6) & 7) + delta;
            mode = FERRET_CLAMP(mode, 0, 3);
            effect->flags = ((uint32_t)mode << 6) | (effect->flags & 0xfffffe3fU);
        }
        break;
    }
#undef FERRET_CLAMP
}

/* PDB 0x97FB0; shipped body size 1,803 bytes. */
void ferret_SpriteFerret(int32_t *cpad, VECTOR *loc)
{
    static int n;
    static uint8_t change[16];
    Sprite *shots[16];
    int index;
    int x;

    if (SAVEMODE != 0) {
        savedLength = ferret_CountSprites(ed, 16);
        ferret_SaveLoadEffect((uint32_t *)cpad, savedLength);
        return;
    }

    if (!KeyPressed(9)) {
        if (CtrlKeyDown()) {
            debug_printf("BUFFER: <v> paste\t<c> copy\t<x> clear <s> save");
            if (KeyPressed(0x13)) {
                SAVEMODE = 1;
            } else if (KeyPressed(0x16)) {
                ed[n] = sfx_Buffer;
                debug_printf("paste\n");
            } else if (KeyPressed(3)) {
                sfx_Buffer = ed[n];
                debug_printf("copy\n");
            } else if (KeyPressed(0x18)) {
                memset(&ed[n], 0, sizeof(ed[n]));
            } else {
                debug_printf("\n");
            }
            return;
        }

        debug_printf("\n\n\n\n\n\n\t\t[%2d]", n);
        if (ed[n].bank == 0 || ed[n].bank == 1) {
            debug_printf("sprites    ELECTRIC FERRET(c)\n");
            if (change[n]) ferret_SpriteBank(cpad, loc, n);
        } else if (ed[n].bank == 2) {
            debug_printf("particles  ELECTRIC FERRET(c)\n");
            if (change[n]) ferret_ParticleBank(cpad, n);
        } else if (ed[n].bank == 3) {
            debug_printf("embedded   ELECTRIC FERRET(c)\n");
            if (change[n]) ferret_EmmbeddedBank(cpad, loc, n);
        } else if (ed[n].bank == 4) {
            debug_printf("rings      FLATULENT MOUSE(c)\n");
            if (change[n]) ferret_EditRing((uint32_t *)cpad, (RingData *)&ed[n]);
        }

        if (KeyPressed(10)) {
            change[n] ^= 1;
            return;
        }
        if (!change[n]) {
            debug_printf("press <enter> to edit\n");
            debug_printf("press <ctrl-x> to clear\n");
            debug_printf("\n");
            debug_printf(" bnk\n");
            debug_printf("%4d\n", ed[n].bank);
            if (KeyPressed(0x1e)) ++ed[n].bank;
            if (KeyPressed(0x1f) && ed[n].bank != 0) --ed[n].bank;
            if (ed[n].bank > 4) ed[n].bank = 4;
        }
    } else {
        if (ShiftKeyDown() && KeyPressed(9)) {
            --n;
        } else if (KeyPressed(9)) {
            ++n;
        }
        if (n < 0) n = 0;
        else if (n > 15) n = 15;
        change[n] = 0;
    }

    x = 8;
    for (index = 0; index < 16; ++index, x += 0x1f) {
        if (ed[index].bank == 1) {
            scb[index] = sprite_DisplaySprite(
                scb[index], ed[index].type, x, 0x14,
                0x1f, 0x1f, ed[index].vel.pad);
            if (index == n) {
                big = sprite_DisplaySprite(
                    big, ed[index].type, 0x10, 0xf0,
                    0x60, 0x60, ed[index].vel.pad);
                scr_debugPrintfXY(x, 0x34, "cur\n");
            } else {
                scr_debugPrintfXY(x, 0x34, " %d\n", index);
            }
        } else {
            if (index == n) {
                if (big != NULL) {
                    sprite_gFreeSCB(big);
                    big = NULL;
                }
                scr_debugPrintfXY(x, 0x34, "cur\n");
            } else if (ed[index].bank == 2) {
                scr_debugPrintfXY(x, 0x34, " p\n");
            } else if (ed[index].bank == 3) {
                scr_debugPrintfXY(x, 0x34, " e\n");
            } else if (ed[index].bank == 4) {
                scr_debugPrintfXY(x, 0x34, " r\n");
            }
            if (scb[index] != NULL) sprite_gFreeSCB(scb[index]);
            scb[index] = NULL;
        }
    }

    if (KeyPressed(0x20)) {
        memset(shots, 0, sizeof(shots));
        for (index = 0; index < 16; ++index) {
            if (ed[index].bank == 1 || ed[index].bank == 3) {
                shots[index] = *sprite_AddSpriteEffect(
                    &ed[index], 1, loc, NULL);
            } else if (ed[index].bank == 2) {
                sprite_FireEmiter(
                    &((Emiter *)aEmiter)[ed[index].type],
                    shots[ed[index].delay]);
            } else if (ed[index].bank == 4) {
                sprite_FireRing((RingData *)&ed[index], loc);
            }
        }
    }
}
