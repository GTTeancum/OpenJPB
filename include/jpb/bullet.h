#ifndef JPB_BULLET_H
#define JPB_BULLET_H

#include "jpb/player.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Sprite Sprite;
/* Exact matched-PC PDB type 0x11C9. */
typedef struct Projectile {
    Sprite *pj_Parent;
    Sprite *pj_Child;
    VECTOR pj_Start;
    _mvector pj_Dir;
    int16_t pj_Range;
    int16_t pj_Type;
    int32_t *pj_Owner;
    int32_t *pj_Target;
    int32_t *pj_User;
    int32_t pj_Flags;
    int32_t color;
    char launchID;
    char pad[3];
} Projectile;

/*
 * Diagnostic observation seam after the exact projectile owner has created
 * and attached a live shot. It cannot replace or suppress retail behavior.
 */
typedef void (*JPBBulletLaunchObserver)(
    void *user_data,
    const Projectile *projectile,
    const playerObject *player,
    const VECTOR *start,
    const VECTOR *target);

enum {
    JPB_PROJECTILE_GLOBAL_CAPACITY = 32
};

/* Exact PDB global at matched-PC RVA 0x10DB3C0. */
extern Projectile maProjectile[JPB_PROJECTILE_GLOBAL_CAPACITY];
/* Exact PDB global at matched-PC RVA 0x10DE700. */
extern char terminatedSound[9];

void TerminateSFXString(char *dest, const char *source);
void jpb_BulletSetLaunchObserver(
    JPBBulletLaunchObserver observer, void *user_data);
Projectile *bullet_AllocProjectile(int type);
int bullet_CallBack(Projectile *proj);
int bullet_Dummy(int32_t *unused);
void bullet_Explosion(
    VECTOR *pos, uint32_t *mask, int radius, int bomb);
void bullet_FreeProjectile(Projectile *proj);
void bullet_InitProjectilePool(void);
void bullet_ShootProjectile(
    Projectile *proj,
    playerObject *player,
    VECTOR *pos0,
    VECTOR *pos1,
    _svector *vel);

#if defined(__cplusplus)
#define JPB_BULLET_STATIC_ASSERT(condition, message) \
    static_assert(condition, message)
#else
#define JPB_BULLET_STATIC_ASSERT(condition, message) \
    _Static_assert(condition, message)
#endif

JPB_BULLET_STATIC_ASSERT(
    offsetof(Projectile, pj_Start) == sizeof(void *) * 2,
    "Projectile pointer prefix changed");
JPB_BULLET_STATIC_ASSERT(
    offsetof(Projectile, pj_Type) ==
        (UINTPTR_MAX == UINT64_MAX ? 42 : 34),
    "Projectile.pj_Type native-pointer offset changed");
#if UINTPTR_MAX == UINT64_MAX
JPB_BULLET_STATIC_ASSERT(
    sizeof(Projectile) == 88,
    "Projectile must match PDB type 0x11C9");
JPB_BULLET_STATIC_ASSERT(
    offsetof(Projectile, pj_Owner) == 48,
    "Projectile.pj_Owner x64 offset changed");
JPB_BULLET_STATIC_ASSERT(
    offsetof(Projectile, launchID) == 80,
    "Projectile.launchID x64 offset changed");
#elif UINTPTR_MAX == UINT32_MAX
JPB_BULLET_STATIC_ASSERT(
    sizeof(Projectile) == 60,
    "Projectile must use the native Xbox pointer-width layout");
JPB_BULLET_STATIC_ASSERT(
    offsetof(Projectile, pj_Owner) == 36,
    "Projectile.pj_Owner x86 offset changed");
JPB_BULLET_STATIC_ASSERT(
    offsetof(Projectile, launchID) == 56,
    "Projectile.launchID x86 offset changed");
#else
#error Unsupported pointer width
#endif

#undef JPB_BULLET_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
