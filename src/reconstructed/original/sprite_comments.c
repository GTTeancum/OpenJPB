/*
 * REVIEWED RECONSTRUCTION of the comment-sprite path from
 * W:\SWJediPowerBattles\Work\sprite.c.
 *
 * Exact PDB names and locals are retained. Control flow and member stores
 * were checked against matched-PC RVAs 0xFA2F0..0xFA3AD and
 * 0xFAFB0..0xFB0DB. Keeping these two functions in their own object makes
 * the still-unrecovered Draw3dText dependency explicit and link-isolated.
 */

#include "jpb/debugtext.h"
#include "jpb/game.h"
#include "jpb/sprite.h"

#include <stdint.h>
#include <stdlib.h>

/* 0xFA2F0, 190 bytes, global, 2 named locals
 * sprite_CommentsCallBack
 * PDB type: void (int*)
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
void sprite_CommentsCallBack(int32_t *cb)
{
    Sprite *sptr = (Sprite *)(void *)cb;
    SCB *scb;

    Draw3dText(
        sptr->sp_Pos.vx,
        sptr->sp_Pos.vy,
        sptr->sp_Pos.vz,
        1.5f,
        (uint32_t)(uintptr_t)sptr->sp_PAnim,
        "%s",
        (char *)(void *)sptr->sp_Anim);
    sptr->sp_Pos.vx += sptr->sp_Vel.vx;
    sptr->sp_Pos.vy += sptr->sp_Vel.vy;
    sptr->sp_Pos.vz += sptr->sp_Vel.vz;

    if (gGlobalTimer <=
        (uint32_t)(uintptr_t)sptr->sp_User) {
        return;
    }
    sptr->sp_Flags |= 1;
    scb = sptr->sp_SCB;
    if (scb != NULL &&
        scb->scb_flags != 0) {
        scb->scb_flags |= 1;
    }
}

/* 0xFAFB0, 300 bytes, global, 5 named locals
 * sprite_GetCommentsSprite
 * PDB type: Sprite* (char*, VECTOR*, _svecto...
 * Source: W:\SWJediPowerBattles\Work\sprite.c
 */
Sprite *sprite_GetCommentsSprite(
    char *string,
    VECTOR *pos,
    _svector *vel,
    uint32_t color)
{
    Sprite *sptr = sprite_gAllocSprite(0);

    if (sptr == NULL) {
        return NULL;
    }
    sptr->sp_Pos.vx = (float)pos->vx;
    sptr->sp_Pos.vy = (float)pos->vy;
    sptr->sp_Pos.vz = (float)pos->vz;
    sptr->sp_Pos.vx +=
        (float)(rand() % 16 - 8);
    sptr->sp_Pos.vy += 16.0f;
    sptr->sp_Pos.vz +=
        (float)(rand() % 16 - 8);
    sptr->sp_Vel.vx = (float)vel->vx;
    sptr->sp_Vel.vy = (float)vel->vy;
    sptr->sp_Vel.vz = (float)vel->vz;
    sptr->sp_PAnim =
        (PalAnim *)(uintptr_t)color;
    sptr->sp_User = (int32_t *)(uintptr_t)(
        gGlobalTimer + UINT32_C(0x2000));
    sptr->sp_Func =
        (SpriteFunction)sprite_CommentsCallBack;
    sptr->sp_Anim =
        (TexAnim *)(void *)string;
    return sptr;
}
