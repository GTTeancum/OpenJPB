#ifndef JPB_SPRITE_H
#define JPB_SPRITE_H

#include "jpb/effects.h"
#include "jpb/fmath.h"
#include "jpb/list.h"
#include "jpb/material.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TexAnim TexAnim;
typedef struct PalAnim PalAnim;
typedef struct Projectile Projectile;

/* Exact matched-PC PDB type 0x1210. */
typedef struct CControl {
    int16_t init;
    int8_t vel;
    int8_t acc;
    int16_t limit;
} CControl;

/* Exact matched-PC PDB type 0x1247. */
typedef struct SControl {
    int16_t init;
    int16_t vel;
    int16_t acc;
    int16_t limit;
} SControl;

/* Exact matched-PC PDB type 0x1227. */
typedef struct EffectData {
    uint8_t bank;
    uint8_t type;
    int16_t delay;
    _svector vel;
    _svector acc;
    SControl bright;
    SControl scale;
    int16_t rx;
    int16_t rvx;
    _svector pos;
    uint32_t flags;
} EffectData;

/* Exact matched-PC PDB type 0x1276. */
struct EffectHeader {
    uint32_t num;
    EffectData aEffects[16];
};

/* Exact matched-PC PDB type 0x121B. */
typedef struct RingData {
    uint8_t bank;
    uint8_t type;
    CControl b;
    CControl r1;
    CControl r2;
    CControl h1;
    CControl h2;
    _svector rot;
    uint8_t clut;
    int8_t rvel;
    uint8_t ratio;
    uint8_t pad0;
    uint32_t time;
    uint8_t tmode;
    uint8_t pad1[3];
} RingData;

/* Exact matched-PC PDB type 0x120B. */
typedef struct Ring {
    Node *sp_Next;
    int32_t sp_Type;
    _svector rot;
    VECTOR pos;
    int16_t b1;
    int16_t b1v;
    int16_t rad1;
    int16_t rad2;
    int16_t rad1v;
    int16_t rad2v;
    int16_t h1;
    int16_t h2;
    int16_t h1v;
    int16_t h2v;
    int32_t ratio;
    uint32_t time;
    RingData *pRingData;
} Ring;

/* Exact matched-PC PDB type 0x10DA. */
typedef struct SCB {
    Node *scb_next_SCB;
    int32_t scb_flags;
    _Material *scb_Texture;
    _sfvector scb_vertex0;
    _sfvector scb_vertex1;
    _sfvector scb_vertex2;
    _sfvector scb_vertex3;
    _sfvector scb_cvertex;
} SCB;

typedef int (*SpriteFunction)(int32_t *);

/*
 * Portable renderer boundary for exact PDB procedure drawCylinderG. The
 * gameplay owner retains the authored cylinder parameters; each renderer may
 * consume the resulting 16-segment gradient primitive without entering the
 * original wHook/D3D implementation.
 */
typedef void (*JPBSpriteCylinderHook)(
    void *user_data,
    const VECTOR *loc,
    const _svector *rot,
    float radius1,
    float radius2,
    float h1,
    float h2,
    uint32_t color1,
    uint32_t color2);

typedef void (*JPBSpriteDisplayHook)(
    void *user_data,
    int type,
    int x,
    int y,
    int w,
    int h,
    int clut,
    const _Material *material);

/* Exact matched-PC PDB type 0x126E. */
typedef struct Sprite {
    Node *sp_Next;
    int32_t sp_Type;
    SCB *sp_SCB;
    _sfvector sp_Pos;
    _sfvector sp_Vel;
    _sfvector sp_Acc;
    _svector sp_Rot;
    _svector sp_RVel;
    TexAnim *sp_Anim;
    PalAnim *sp_PAnim;
    SpriteFunction sp_Func;
    int16_t sp_Time;
    int16_t sp_Delay;
    int32_t *sp_User;
    int32_t sp_Flags;
    SControl sp_cScale;
    SControl sp_cBright;
    int16_t sp_Num;
} Sprite;

extern List mSCBDraw[2];
extern int mCurSCBList;
/* Portable diagnostics for corrupted double-buffer selector recovery. */
extern uint32_t jpb_sprite_list_recovery_count;
extern int32_t jpb_sprite_last_invalid_selector;
extern uint32_t cluts[32];
extern int numSprite;
extern int numSCB;
extern float framerate;
extern int mDrawingSurfaceId;
extern float scaleAdjustment;
extern float scaleAdjustmentMM;
extern int16_t aCircle[17];

void jpb_SpriteSetCylinderHook(
    JPBSpriteCylinderHook hook, void *user_data);
void jpb_SpriteSetDisplayHook(
    JPBSpriteDisplayHook hook, void *user_data);
void drawCylinder(
    VECTOR *loc,
    _svector *rot,
    float radius1,
    float radius2,
    float height1,
    float height2,
    uint32_t color1,
    uint32_t ratio,
    int id,
    int clut,
    int tmode);
void drawCylinderG(
    VECTOR *loc,
    _svector *rot,
    float radius1,
    float radius2,
    float h1,
    float h2,
    uint32_t color1,
    uint32_t color2);
float getScaleAdjustment(void);
float getScaleAdjustmentMM(void);
void setPivotPosition(float *x, float *y, int pivot);
void setPivotPositionMM(float *x, float *y, int pivot);
void setPivotPositionMM_PSX(float *x, float *y, int pivot);
void setPivotPositionAndFixScale(
    float *x,
    float *y,
    float *width,
    float *height,
    int pivot);
void setPositionOffPivot(
    float *x, float *y, float pivot_x, float pivot_y);

void _RenderSprite(MATRIX *matrix, SCB *scb);
int sprite_AddCallBack(int32_t *cb);
Sprite *sprite_AddProjectile(
    Projectile *proj, VECTOR *pos, int32_t *callback, int type);
Sprite *sprite_AddSpriteAtLoc(Sprite *sptr, int type, VECTOR *pos);
void sprite_Flash(int32_t *cb);
Sprite **sprite_AddSpriteEffect(
    EffectData *data, int num, VECTOR *loc, _svector *vel);
Sprite **sprite_AddSpriteEffectAtNode(
    EffectData *data, int num, int playernum, int nodeID);
Ring *sprite_AllocRing(void);
Ring *sprite_FireRing(RingData *r, VECTOR *pos0);
Sprite *sprite_GetCommentsSprite(
    char *text,
    VECTOR *pos,
    _svector *velocity,
    uint32_t colour);
Sprite *sprite_GetBaseNodeMarker(int player, int distance);
Sprite *sprite_GetPointsSprite(
    int points,
    VECTOR *pos,
    _svector *velocity,
    uint32_t colour,
    int small);
void sprite_CommentsCallBack(int32_t *cb);
SCB *sprite_DisplaySprite(
    SCB *scb,
    int type,
    int x,
    int y,
    int w,
    int h,
    int clut);
int sprite_Center(int32_t *cb);
void sprite_PointsCallBack(int32_t *cb);
void sprite_SmallPointsCallBack(int32_t *cb);
int sprite_MainCallBack(int32_t *cb);
SCB *sprite_gAllocSCB(void);
Sprite *sprite_gAllocSprite(int flag);
void sprite_gHideSCB(SCB *scb);
void sprite_gHideSprite(Sprite *sptr);
void sprite_gUnHideSprite(Sprite *sptr);
void sprite_gFreeSCB(SCB *scb);
void sprite_gFreeSprite(Sprite *sptr);
void sprite_gInitSprites(void);
void sprite_gMoveSpritePosition(
    Sprite *sptr, float dx, float dy, float dz);
void sprite_gSetSpritePosition(
    Sprite *sptr, int x, int y, int z);
void sprite_ClearProjectilePool(Projectile *pool, int num);
Sprite *sprite_Get3DProjectile(
    Projectile *proj, VECTOR *cpos, _svector *rot);
void sprite_InitProjectilePool(Projectile *pool, int num);
void sprite_Move3DSprite(Sprite *sptr, VECTOR *pos);
void sprite_RemoveProjectile(Projectile *proj);
int sprite_RingCallBack(Ring *ring);
void sprite_Rotate3DSprite(Sprite *sptr, _svector *rot);
Projectile *sprite_SetProjectile(
    Sprite *sptr, Projectile *proj, int32_t *callback);
void sprite_SpriteRotScale(
    Sprite *sptr, int x_rotation, int y_rotation, int z_rotation);
void sprite_SpriteWork(MATRIX *matrix);
void sprite_SwapData(SCB *scb);

#if defined(__cplusplus)
#define JPB_SPRITE_STATIC_ASSERT(condition, message) \
    static_assert(condition, message)
#else
#define JPB_SPRITE_STATIC_ASSERT(condition, message) \
    _Static_assert(condition, message)
#endif

JPB_SPRITE_STATIC_ASSERT(
    sizeof(CControl) == 6,
    "CControl must match PDB type 0x1210");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(SControl) == 8,
    "SControl must match PDB type 0x1247");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(EffectData) == 52,
    "EffectData must match PDB type 0x1227");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(EffectData, flags) == 48,
    "EffectData.flags layout changed");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(EffectHeader) == 836,
    "EffectHeader must match PDB type 0x1276");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(RingData) == 52,
    "RingData must match PDB type 0x121B");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(RingData, rot) == 32,
    "RingData.rot layout changed");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(RingData, time) == 44,
    "RingData.time layout changed");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(SCB, scb_flags) == sizeof(void *),
    "SCB.scb_flags native-pointer offset changed");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(Sprite, sp_SCB) ==
        (UINTPTR_MAX == UINT64_MAX ? 16 : 8),
    "Sprite.sp_SCB native-pointer offset changed");
#if UINTPTR_MAX == UINT64_MAX
JPB_SPRITE_STATIC_ASSERT(
    sizeof(SCB) == 104,
    "SCB must match PDB type 0x10DA");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(SCB, scb_Texture) == 16,
    "SCB.scb_Texture x64 offset changed");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(SCB, scb_cvertex) == 88,
    "SCB.scb_cvertex x64 offset changed");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(Sprite) == 152,
    "Sprite must match PDB type 0x126E");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(Ring) == 72,
    "Ring must match PDB type 0x120B");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(Ring, pos) == 20,
    "Ring.pos x64 offset changed");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(Ring, time) == 60,
    "Ring.time x64 offset changed");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(Ring, pRingData) == 64,
    "Ring.pRingData x64 offset changed");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(Sprite, sp_Pos) == 24,
    "Sprite.sp_Pos x64 offset changed");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(Sprite, sp_Func) == 104,
    "Sprite.sp_Func x64 offset changed");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(Sprite, sp_Num) == 148,
    "Sprite.sp_Num x64 offset changed");
#elif UINTPTR_MAX == UINT32_MAX
JPB_SPRITE_STATIC_ASSERT(
    sizeof(SCB) == 92,
    "SCB must use the native Xbox pointer-width layout");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(SCB, scb_Texture) == 8,
    "SCB.scb_Texture x86 offset changed");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(Sprite) == 120,
    "Sprite must use the native Xbox pointer-width layout");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(Sprite, sp_Func) == 84,
    "Sprite.sp_Func x86 offset changed");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(Sprite, sp_Num) == 116,
    "Sprite.sp_Num x86 offset changed");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(Ring) == 64,
    "Ring must use the native Xbox pointer-width layout");
JPB_SPRITE_STATIC_ASSERT(
    offsetof(Ring, pRingData) == 60,
    "Ring.pRingData x86 offset changed");
#else
#error Unsupported pointer width
#endif

#undef JPB_SPRITE_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
