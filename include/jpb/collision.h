#ifndef JPB_COLLISION_H
#define JPB_COLLISION_H

#include "jpb/vectors.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum modelNodeId {
    NODE_INDEX_MASK = 0x0fff,
    NODE_DYNAMIC = 0x1000,
    NODE_VIRTUAL = 0x2000,
    NODE_VD_INDEX_MASK = 0x3fff,
    NODE_INVALID = 0x4000,
    NODE_STATIC = 0x8000
} modelNodeId;

typedef struct geomData geomData;
struct CollisionData;
struct Projectile;
struct playerObject;

/*
 * Exact PDB type 0x119A. Pointer members intentionally remain native:
 * the matched x64 record is 152 bytes and the Xbox runtime record is 136.
 */
typedef struct Mnode {
    modelNodeId id;
    _svector v3CurrentRotation;
    _svector v3RotationDelta;
    _svector v3RotationAbs;
    _svector v3Translation;
    _svector v3Translation2;
    VECTOR v3RotCenter;
    _svector v3Velocity2;
    int32_t time;
    _svector v3Velocity;
    VECTOR v3Scale;
    VECTOR v3PostTranslation;
    int16_t RendPacketID;
    int16_t ZBufferOffset;
    int16_t zPushMod;
    int16_t numChildNodes;
    struct Mnode *aChildNode;
    struct Mnode *pParent;
    geomData *pGeomData;
    uint32_t flags;
} Mnode;

enum {
    JPB_COLLISION_PLAYER_CAPACITY = 20,
    JPB_COLLISION_NODE_CAPACITY = 32,
    JPB_COLLISION_FLAG_HOT = 0x00000001,
    JPB_COLLISION_FLAG_EVENT = 0x00000002,
    JPB_COLLISION_FLAG_SABRE = 0x00000010,
    /* Node-local v3Scale overrides the model scale while this flag is set. */
    JPB_COLLISION_FLAG_SCALE_OVERRIDE = 0x00400000,
    JPB_COLLISION_FLAG_ROTATION_ABS_DIRTY = 0x00800000,
    JPB_COLLISION_FLAG_ROTATION_DELTA_DIRTY = 0x01000000
};

int coll_4DCollision(
    VECTOR *p0_B,
    _svector *v0,
    VECTOR *p1,
    _svector *v1,
    int dist);
int coll_CheckForEventNode(int player, int node_id);
int coll_CheckForHotNode(int player, int node_id);
int coll_CheckForSabreNode(int player, int node_id);
int coll_CheckNodeCollision(
    struct playerObject *attacker,
    struct CollisionData *attacker_node_data,
    struct playerObject *target);
int coll_CheckProjectileCollision(struct Projectile *proj);
int coll_ChkNodeFlags(int player, int node_id, uint32_t flags);
void coll_ClrNodeFlags(int player, int node_id, uint32_t flags);
Mnode *coll_GetNode(int player, unsigned node_id);
VECTOR *coll_GetNodeCenter(int player, int node_id);
_svector *coll_GetNodeRotation(int player, int node_id);
_svector *coll_GetNodeRotationAbs(int player, int node_id);
_svector *coll_GetNodeRotationDelta(
    int player, int node_id, _svector *rotation);
_svector *coll_GetNodeTranslation(int player, int node_id);
_svector *coll_GetNodeVelocity(int player, int node_id);
void coll_IncNodeRotationAbs(
    int player, int node_id, _svector *rotation);
void coll_IncNodeRotationDelta(
    int player, int node_id, _svector *rotation);
void coll_ResetCollisionSystem(void);
void coll_ResetPlayerCollision(int player);
void coll_SetNodeFlags(int player, int node_id, uint32_t flags);
void coll_SetNodeRotationAbs(
    int player, int node_id, _svector *rotation);
void coll_SetNodeRotationDelta(
    int player, int node_id, _svector *rotation);
void coll_SetNodeTranslation(
    int player, int node_id, VECTOR *translation);
void coll_SetNodeZBufferOffset(int player, int node_id, int offset);
void coll_gRegisterNode(int player, Mnode *node);
int coll_gCheckHotNodes(
    struct playerObject *attacker,
    struct playerObject *target);
void old_coll_ZeroNodeTranslation(int player, int node_id);

/* Exact PDB global `mReflects` at matched-PC RVA 0x4AFCB8. */
extern _svector mReflects[5];

#if defined(__cplusplus)
#define JPB_COLLISION_STATIC_ASSERT static_assert
#else
#define JPB_COLLISION_STATIC_ASSERT _Static_assert
#endif

JPB_COLLISION_STATIC_ASSERT(
    offsetof(Mnode, v3CurrentRotation) == 4,
    "Mnode.v3CurrentRotation layout changed");
JPB_COLLISION_STATIC_ASSERT(
    offsetof(Mnode, v3RotCenter) == 44,
    "Mnode.v3RotCenter layout changed");
JPB_COLLISION_STATIC_ASSERT(
    offsetof(Mnode, v3Velocity) == 72,
    "Mnode.v3Velocity layout changed");
JPB_COLLISION_STATIC_ASSERT(
    offsetof(Mnode, aChildNode) == 120,
    "Mnode.aChildNode layout changed");
#if UINTPTR_MAX == UINT64_MAX
JPB_COLLISION_STATIC_ASSERT(
    offsetof(Mnode, flags) == 144,
    "x64 Mnode.flags layout changed");
JPB_COLLISION_STATIC_ASSERT(
    sizeof(Mnode) == 152,
    "x64 Mnode size must match PDB type 0x119A");
#elif UINTPTR_MAX == UINT32_MAX
JPB_COLLISION_STATIC_ASSERT(
    offsetof(Mnode, flags) == 132,
    "32-bit Mnode.flags layout changed");
JPB_COLLISION_STATIC_ASSERT(
    sizeof(Mnode) == 136,
    "32-bit Mnode runtime size changed");
#else
#error Unsupported pointer width
#endif

#undef JPB_COLLISION_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
