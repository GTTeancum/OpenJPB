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

typedef struct Projectile Projectile;

/* Exact matched-PC PDB type TexAnim. */
typedef struct TexAnim {
    void *anm_UVIndex;
    int16_t anm_width;
    int16_t anm_height;
    int16_t anm_fframe;
    int16_t anm_lframe;
    int32_t anm_cframe;
    int32_t anm_frate;
    int32_t anm_flags;
} TexAnim;

/* Exact matched-PC PDB type ColorCycle. */
typedef struct ColorCycle {
    int16_t cc_fcolor;
    int16_t cc_lcolor;
    int16_t cc_dcolor;
    int16_t cc_pad0;
    int32_t cc_current;
    int32_t cc_rate;
    uint32_t cc_flags;
} ColorCycle;

/* Exact matched-PC PDB type PalAnim. */
typedef struct PalAnim {
    Node *pal_Next;
    _Material *pal_Data;
    ColorCycle pal_cycle[4];
    uint32_t pal_flags;
} PalAnim;

/* Exact matched-PC PDB type Emiter (spelling retained). */
typedef struct Emiter {
    int16_t rate;
    int16_t count;
    int16_t flags;
    int8_t vmin;
    int8_t vmax;
    int8_t sr;
    int8_t sg;
    int8_t sb;
    int8_t spread;
    int8_t er;
    int8_t eg;
    int8_t eb;
    int8_t pad;
    _svector rot;
    VECTOR pos;
    int16_t colorSpeed;
    int16_t deathSpeed;
    uint8_t v1;
    uint8_t v2;
    uint8_t v3;
    uint8_t mode;
} Emiter;

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

/* Exact matched-PC PDB type Particle. */
typedef struct Particle {
    _svector vel;
    CVECTOR color;
    _svector pos;
    int16_t vx;
    int16_t vy;
    int16_t vz;
} Particle;

/* Exact matched-PC PDB type PCB. */
typedef struct PCB {
    Node *pcb_next_PCB;
    int32_t pcb_flags;
    VECTOR *pcb_Pos;
    int32_t pcb_fRate;
    int32_t pcb_fLaunch;
    int32_t pcb_Interp;
    uint8_t pcb_r;
    uint8_t pcb_g;
    uint8_t pcb_b;
    uint8_t pcb_trans;
    uint8_t pcb_sr;
    uint8_t pcb_sg;
    uint8_t pcb_sb;
    uint8_t pcb_pad0;
    int32_t pcb_Bits;
    Sprite *pcb_Sptr;
    int16_t pcb_v1;
    int16_t pcb_v2;
    int16_t pcb_v3;
    int16_t pcb_mode;
    int16_t pcb_Scale;
    int16_t pcb_grnd;
    Particle pcb_Particle[8];
    _svector pcb_Data[8];
} PCB;

extern List mSCBDraw[2];
extern int mCurSCBList;
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
void setPivotPositionAndFixScaleMM(
    float *x,
    float *y,
    float *width,
    float *height,
    int pivot);
void setPositionOffPivot(
    float *x, float *y, float pivot_x, float pivot_y);
void setPositionOffPivotMM(
    float *x, float *y, float pivot_x, float pivot_y);

void _RenderSprite(MATRIX *matrix, SCB *scb);
int sprite_AddCallBack(int32_t *cb);
Sprite *sprite_AddProjectile(
    Projectile *proj, VECTOR *pos, int32_t *callback, int type);
Sprite *sprite_AddSpriteAtLoc(Sprite *sptr, int type, VECTOR *pos);
Sprite *sprite_AddSpriteAtNode(int playernum, int nodeID, int type);
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
SCB *sprite_DisplayTpage(int tpage, int x, int y);
void sprite_FireEmiter(Emiter *emit, Sprite *shot);
Ring *sprite_FireSphere(RingData *r, VECTOR *pos0);
void sprite_Get3DShear(Sprite *sptr, VECTOR *center, int dist);
Sprite *sprite_GetSpotMarker(int playernum, int dist, VECTOR *pos);
void sprite_GetSuckSpritePos(
    VECTOR *pos, _svector *dir, VECTOR *center);
Sprite *sprite_GetTargetMarker(int playernum, int dist, int type);
int sprite_Glow(int32_t *cb);
void sprite_InitSCBPool(void);
int sprite_LightMotion(int32_t *cb);
int sprite_Lock(int32_t *cb);
int sprite_LockNode(int32_t *cb);
int sprite_OrbitNode(int32_t *cb);
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
void sprite_SCBDraw(void *matrix);
void sprite_Set2DSCBPos(
    SCB *scb, int x, int y, int z, int w, int h);
Sprite *sprite_SetColorCycle(
    Sprite *sptr, int type, int ps, int pe, int pr);
void sprite_SetSpriteEffect(
    Sprite *sptr, int effect, int w, int h);
int sprite_SetTrajectory(Sprite *sptr, int velocity, int angle);
int sprite_Sparks(int32_t *cb);
int sprite_Spot(int32_t *cb);
int sprite_SpriteMotion(int32_t *cb);
int sprite_TrackNode(int32_t *cb);
int sprite_ViewPoint(int32_t *cb);
PCB *sprite_gAllocPCB(void);
void sprite_gMoveSCBPosition(
    SCB *scb, float dx, float dy, float dz);
void sprite_gSetBillBrd(Sprite *sptr);
void sprite_gSetLineColor(Sprite *sptr, CVECTOR *color);
void sprite_gSetLinePosition(
    Sprite *sptr, VECTOR *p0, VECTOR *p1);
void sprite_gSetRGB(SCB *scb, char r, char g, char b);
void sprite_gSetSCBPosition(
    SCB *scb, int x, int y, int z, int w, int h);
void sprite_gSetSpriteCenter(Sprite *sptr, int x, int y, int z);
void sprite_gSetSpriteRGB(
    Sprite *sptr, char r, char g, char b);
void sprite_gUnHideSCB(SCB *scb);

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
    sizeof(TexAnim) == 32,
    "TexAnim must match the matched-PC PDB");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(ColorCycle) == 20,
    "ColorCycle must match the matched-PC PDB");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(Emiter) == 48,
    "Emiter must match the matched-PC PDB");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(Particle) == 26,
    "Particle must match the matched-PC PDB");
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
    sizeof(PalAnim) == 104,
    "PalAnim must match the matched-PC PDB");
JPB_SPRITE_STATIC_ASSERT(
    sizeof(PCB) == 344,
    "PCB must match the matched-PC PDB");
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
