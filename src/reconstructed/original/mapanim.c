/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\Work\mapanim.c.
 * PDB module: 0053
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\mapanim.obj
 * Primary source: W:\SWJediPowerBattles\Work\mapanim.c
 * Compiler language: c
 * Emitted procedures: 7
 */

#include "jpb/mapanim.h"

#include "jpb/enemy.h"
#include "jpb/fmath.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

extern int gGlobalFrameRate;
extern WorldData *gpWorld;

int animsPaused;
static int32_t animNodeStackCount;
static int32_t stackOverFlow;
static int32_t firstPass;
static int32_t animNodeStack[32];
static wsl_BT_ANIMDEF **pMapAnim;

static int32_t mapanim_from_bits(uint32_t value)
{
    int32_t result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static int32_t mapanim_add(int32_t left, int32_t right)
{
    return mapanim_from_bits((uint32_t)left + (uint32_t)right);
}

static int32_t mapanim_subtract(int32_t left, int32_t right)
{
    return mapanim_from_bits((uint32_t)left - (uint32_t)right);
}

static int32_t mapanim_multiply(int32_t left, int32_t right)
{
    return mapanim_from_bits((uint32_t)left * (uint32_t)right);
}

static int32_t mapanim_fixed12_truncate(int32_t value)
{
    uint32_t adjusted = (uint32_t)value +
        (value < 0 ? UINT32_C(0xfff) : UINT32_C(0));
    uint32_t result = adjusted >> 12;

    if ((int32_t)adjusted < 0) {
        result |= UINT32_C(0xfff00000);
    }
    return mapanim_from_bits(result);
}

static int32_t mapanim_frame_step(int32_t delta_time, int32_t fps)
{
    return mapanim_fixed12_truncate(
        mapanim_multiply(delta_time, fps));
}

/* 0xBE240, 3 bytes, global, 1 named locals
 * bapmanim_Activate
 * PDB type: void (long)
 * Source: W:\SWJediPowerBattles\Work\mapanim.c
 */
void bapmanim_Activate(int32_t animID)
{
    (void)animID;
}

/* 0xBE250, 3 bytes, global, 1 named locals
 * bapmanim_DeActivate
 * PDB type: void (long)
 * Source: W:\SWJediPowerBattles\Work\mapanim.c
 */
void bapmanim_DeActivate(int32_t animID)
{
    (void)animID;
}

/* 0xBE540, 447 bytes, local, 8 named locals
 * btanplay_GetEntryValues
 * PDB type: void (wsl_BT_ANIMENTRY*, wsl_BT_ANIMDEF*,
 *                 wsl_BT_ANIMNODE*, long)
 * Source: W:\SWJediPowerBattles\Work\mapanim.c
 */
static void btanplay_GetEntryValues(
    wsl_BT_ANIMENTRY *entry,
    wsl_BT_ANIMDEF *definition,
    wsl_BT_ANIMNODE *node,
    int32_t frame_number)
{
    int32_t index = 0;
    int32_t last_index;
    wsl_BT_ANIMENTRY *base;

    if (entry == NULL) {
        return;
    }
    memset(entry, 0, sizeof(*entry));
    if (definition == NULL || node == NULL || definition->numFrames <= 0 ||
        node->numEntries <= 0) {
        return;
    }

    if (node->numEntries == 1) {
        *entry = node->aEntry[0];
        entry->flags |= 0x8000;
        return;
    }

    if (frame_number < 0) {
        frame_number = 0;
    }
    while (frame_number > definition->numFrames) {
        frame_number -= definition->numFrames;
    }

    last_index = node->numEntries - 1;
    while (index < last_index &&
           frame_number >= node->aEntry[index + 1].frame) {
        ++index;
    }

    base = &node->aEntry[index];
    *entry = *base;
    if (index == last_index) {
        if (entry->frame == frame_number) {
            entry->flags |= 0x8000;
        }
        return;
    }

    entry->frame = frame_number;
    if (base->frame != frame_number &&
        base->frame < base[1].frame) {
        int32_t range = (base[1].frame - base->frame) >> 12;
        int32_t offset = (frame_number - base->frame) >> 12;

        entry->xyz.vx =
            ((base[1].xyz.vx - base->xyz.vx) / range) * offset +
            base->xyz.vx;
        entry->xyz.vy =
            ((base[1].xyz.vy - base->xyz.vy) / range) * offset +
            base->xyz.vy;
        entry->xyz.vz =
            ((base[1].xyz.vz - base->xyz.vz) / range) * offset +
            base->xyz.vz;
        entry->pyr.vx =
            ((base[1].pyr.vx - base->pyr.vx) / range) * offset +
            base->pyr.vx;
        entry->pyr.vy =
            ((base[1].pyr.vy - base->pyr.vy) / range) * offset +
            base->pyr.vy;
        entry->pyr.vz =
            ((base[1].pyr.vz - base->pyr.vz) / range) * offset +
            base->pyr.vz;
        return;
    }
    entry->flags |= 0x8000;
}

/* 0xBE260, 735 bytes, local, 10 named locals
 * btanplay_BuildMatrices
 * PDB type: void (wsl_BT_ANIMMAP*)
 * Source: W:\SWJediPowerBattles\Work\mapanim.c
 */
static void btanplay_BuildMatrices(wsl_BT_ANIMMAP *animation)
{
    MATRIX root = {
        {{4096.0f, 0.0f, 0.0f},
         {0.0f, 4096.0f, 0.0f},
         {0.0f, 0.0f, 4096.0f}},
        {0, 0, 0}
    };
    wsl_BT_ANIMDEF *definition =
        gpWorld->animDef[animation->defNum];

    if (definition == NULL) {
        return;
    }

    pMapAnim = &definition;
    animNodeStackCount = 0;
    animNodeStack[0] = definition->aNodes[0].num;
    stackOverFlow = 0;
    firstPass = 0;

    while (animNodeStackCount >= 0) {
        int32_t node_index = animNodeStack[animNodeStackCount--];
        wsl_BT_ANIMNODE *animation_node =
            &definition->aNodes[node_index];
        wsl_BT_PARTNODE *part_node = NULL;
        wsl_BT_PARTNODE *parent_node = NULL;
        wsl_BT_ANIMENTRY entry;
        MATRIX rotation;
        _svector angles;
        int32_t x;
        int32_t y;
        int32_t z;

        if (animation_node->iSibling != -1 && firstPass != 0) {
            if (animNodeStackCount < 31) {
                animNodeStack[++animNodeStackCount] =
                    animation_node->iSibling;
            } else {
                stackOverFlow = 1;
            }
        }
        if (animation_node->iChild != -1 &&
            (animation_node->flags & 1) == 0) {
            if (animNodeStackCount < 31) {
                animNodeStack[++animNodeStackCount] =
                    animation_node->iChild;
            } else {
                stackOverFlow = 1;
            }
        }
        firstPass = 1;

        if (animation_node->num >= 0 &&
            animation_node->num < animation->numNodes) {
            part_node = &animation->aNodes[animation_node->num];
            if (animation_node->iParent >= 0 &&
                animation_node->iParent < animation->numNodes) {
                parent_node =
                    &animation->aNodes[animation_node->iParent];
            }
        }
        if (part_node == NULL) {
            continue;
        }

        part_node->currFrame = animation->currFrame;
        if (animation->orient == 0) {
            btanplay_GetEntryValues(
                &entry, definition, animation_node,
                animation->currFrame);
        } else {
            wsl_BT_ANIMENTRY temporary;
            btanplay_GetEntryValues(
                &temporary, definition, animation_node,
                animation->currFrame);
            entry = temporary;
        }

        if ((entry.flags & 0x40) != 0) {
            enemy_ActivateEnemy(animation->enemyNum);
        }

        angles.vx = (int16_t)entry.pyr.vx;
        angles.vy = (int16_t)entry.pyr.vy;
        angles.vz = (int16_t)entry.pyr.vz;
        angles.pad = 0;
        fRotMatrixZYX(&angles, &rotation);
        if (parent_node == NULL) {
            fMulMatrix0(
                &root, &rotation, &part_node->mat.matrix);
            x = mapanim_add(entry.xyz.vx, part_node->pivot.vx);
            y = mapanim_add(entry.xyz.vy, part_node->pivot.vy);
            z = mapanim_add(entry.xyz.vz, part_node->pivot.vz);
        } else {
            fMulMatrix0(
                &parent_node->mat.matrix,
                &rotation,
                &part_node->mat.matrix);
            x = mapanim_add(
                mapanim_subtract(
                    part_node->pivot.vx, parent_node->pivot.vx),
                entry.xyz.vx);
            y = mapanim_add(
                mapanim_subtract(
                    part_node->pivot.vy, parent_node->pivot.vy),
                entry.xyz.vy);
            z = mapanim_add(
                mapanim_subtract(
                    part_node->pivot.vz, parent_node->pivot.vz),
                entry.xyz.vz);
        }
        part_node->mat.matrix.t[0] = x;
        part_node->mat.matrix.t[1] = y;
        part_node->mat.matrix.t[2] = mapanim_add(z, 0x100);
    }
}

/* 0xBE700, 3 bytes, global, 0 named locals
 * manim_HandleMapAnims
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\mapanim.c
 */
void manim_HandleMapAnims(void)
{
}

/* 0xBE710, 6 bytes, global, 1 named locals
 * manim_InitAnim
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\mapanim.c
 */
int manim_InitAnim(int animNum)
{
    (void)animNum;
    return 1;
}

/* 0xBE720, 856 bytes, global, 4 named locals
 * manim_UpdateMapAnim
 * PDB type: void (wsl_BT_ANIMMAP*)
 * Source: W:\SWJediPowerBattles\Work\mapanim.c
 */
void manim_UpdateMapAnim(wsl_BT_ANIMMAP *animation)
{
    int32_t delta_time = gGlobalFrameRate;
    int32_t frame;
    int32_t step;

    if (animsPaused != 0 || delta_time == 0) {
        return;
    }

    frame = animation->currFrame;
    animation->kDTime = delta_time;
    step = mapanim_frame_step(delta_time, animation->fps);
    switch (animation->type) {
    case 0:
        frame = mapanim_add(frame, step);
        break;
    case 1:
        if (animation->state != 0 && animation->state != 1) {
            return;
        }
        animation->timer1 = mapanim_add(animation->timer1, delta_time);
        frame = mapanim_add(frame, step);
        if (animation->timer1 >= animation->delayTime) {
            animation->state = 2;
        }
        break;
    case 2:
        switch (animation->state) {
        case 0:
            animation->on = 0;
            frame = 0;
            break;
        case 1:
            frame = mapanim_add(frame, step);
            if (frame < animation->numFrames) {
                break;
            }
            animation->state = 2;
            animation->timer1 = 0;
            {
                int32_t excess = mapanim_multiply(
                    mapanim_subtract(frame, animation->numFrames),
                    animation->fps);
                int32_t correction = mapanim_multiply(
                    mapanim_fixed12_truncate(excess), -0x1000);
                delta_time = mapanim_add(
                    delta_time, correction / 0x3e8000);
            }
            /* fall through */
        case 2:
            animation->timer1 =
                mapanim_add(animation->timer1, delta_time);
            frame = animation->numFrames;
            if (animation->timer1 < animation->delayTime) {
                break;
            }
            delta_time =
                mapanim_subtract(animation->timer1, animation->delayTime);
            animation->state = 3;
            /* fall through */
        case 3:
            frame = mapanim_subtract(
                frame,
                mapanim_frame_step(delta_time, animation->fps));
            if (frame <= 0) {
                animation->state = 0;
                animation->on = 0;
            }
            break;
        default:
            break;
        }
        break;
    case 3:
        if (animation->state == 1) {
            frame = mapanim_add(frame, step);
            if (frame >= animation->numFrames) {
                animation->state = 2;
                frame = animation->numFrames;
            }
        } else if (animation->state == 2) {
            animation->on = 0;
            return;
        } else {
            return;
        }
        break;
    case 4:
        if (animation->state != 1) {
            return;
        }
        if (mapanim_add(frame, step) >= animation->numFrames) {
            animation->state = 2;
        }
        /* fall through */
    case 5:
        if (animation->state == 0) {
            frame = 0;
            break;
        }
        if (animation->state != 1) {
            return;
        }
        frame = mapanim_add(frame, step);
        if (frame >= animation->numFrames) {
            animation->state = 2;
            animation->on = 0;
            frame = animation->numFrames;
        }
        break;
    case 6:
        switch (animation->state) {
        case 0:
            animation->on = 0;
            frame = 0;
            break;
        case 1:
            frame = mapanim_add(frame, step);
            if (frame >= animation->numFrames) {
                animation->state = 2;
                animation->timer1 = 0;
                frame = animation->numFrames;
            }
            break;
        case 2:
            animation->timer1 =
                mapanim_add(animation->timer1, delta_time);
            frame = animation->numFrames;
            if (animation->timer1 >= animation->delayTime) {
                animation->state = 3;
            }
            break;
        case 3:
            frame = animation->numFrames;
            break;
        case 4:
            frame = mapanim_subtract(frame, step);
            if (frame <= 0) {
                animation->state = 5;
                animation->timer1 = 0;
                frame = 0;
            }
            break;
        case 5:
            animation->timer1 =
                mapanim_add(animation->timer1, delta_time);
            frame = 0;
            if (animation->timer1 >= animation->delayTime) {
                animation->state = 0;
                animation->on = 0;
            }
            break;
        }
        break;
    case 7:
        frame = 0;
        break;
    }

    if (frame < 0) {
        frame = 0;
    }
    while (frame > animation->numFrames) {
        frame -= animation->numFrames;
    }
    animation->currFrame = frame;
    btanplay_BuildMatrices(animation);
}
