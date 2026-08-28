#ifndef JPB_PHYSICS_H
#define JPB_PHYSICS_H

#include "jpb/scene.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { JPB_PHYSICS_CAPACITY = 20 };

typedef enum JPBPhysicsResult {
    JPB_PHYSICS_RESULT_OK = 0,
    JPB_PHYSICS_RESULT_INVALID_ARGUMENT = -1,
    JPB_PHYSICS_RESULT_UNSUPPORTED_STATE = -2,
    JPB_PHYSICS_RESULT_ALREADY_PROCESSED = -3
} JPBPhysicsResult;

typedef enum MOVE_MODE {
    MOVE_NORMAL = 0,
    MOVE_HOVER = 1,
    MOVE_HOVER3D = 2,
    MOVE_FLY = 3,
    MOVE_BLOWN = 4,
    MOVE_COREDEATH = 5
} MOVE_MODE;

typedef struct Mnode Mnode;
typedef struct geomData geomData;
typedef struct modelObject modelObject;
typedef struct physicsObject physicsObject;
typedef struct playerObject playerObject;

typedef struct _jheightstuff {
    int32_t *cube;
    int32_t *entry;
    int32_t *poly;
} _jheightstuff;

typedef struct _solid {
    uint32_t flags;
    sceneObject *object;
    Mnode *modelnode;
    geomData *geometry;
    modelObject *model;
    physicsObject *physics;
    int32_t node;
    _svector *coords;
    _svector *normals;
    MATRIX rotmatrix;
    VECTOR scale;
} _solid;

typedef struct _collide_info {
    int16_t type;
    int16_t flags;
    float dist;
    FVECTOR kisspoint;
    int32_t edge;
    FVECTOR n;
    FVECTOR facenormal;
    int32_t washack;
} _collide_info;

typedef struct _movement_packet {
    FVECTOR points[4];
    FVECTOR facenormal;
    FVECTOR movement;
    FVECTOR startpos;
    FVECTOR to;
    FVECTOR vmin;
    FVECTOR vmax;
    float radius;
    float distance;
    int32_t numsides;
    _collide_info info;
} _movement_packet;

typedef struct _collidevars {
    FVECTOR *P0[5];
    FVECTOR *P1[5];
    FVECTOR edge[4];
    FVECTOR edgenormal[4];
    FVECTOR vmin;
    FVECTOR vmax;
    FVECTOR tmp;
    float rsquared;
    float bestDist;
    float distToPlane;
    float moveToPlaneDot;
    int32_t sideMask;
    int32_t edge_start;
} _collidevars;

/*
 * Exact matched-PC PDB type 0x125F. Native pointers preserve the PDB layout
 * on x64 and intentionally compact this runtime-only record on 32-bit Xbox.
 */
struct physicsObject {
    objectRoot physicsRoot;
    MATRIX matrix;
    VECTOR angle;
    VECTOR face;
    FVECTOR pos;
    FVECTOR snapshotpos;
    FVECTOR lastpos;
    FVECTOR localpos;
    FVECTOR newlocalpos;
    FVECTOR localfacing;
    _svector svangle;
    _svector svpos;
    _svector svmov;
    VECTOR vpos;
    VECTOR vmov;
    struct physicsObject *solidgrabbed;
    FVECTOR constmov;
    FVECTOR currentmov;
    FVECTOR airmov;
    FVECTOR mov;
    FVECTOR accel;
    MOVE_MODE movemode;
    int16_t trajectory;
    int16_t airspeed;
    int16_t turnspeed;
    int16_t radius;
    int16_t mass;
    int16_t height;
    int16_t noncollideframes;
    uint32_t collidetime;
    uint32_t anycollidetime;
    int32_t airTime;
    int32_t realAirTime;
    float airGround;
    float validairground;
    int32_t reversoi;
    int32_t airVx;
    int32_t airVz;
    int32_t airVelocity;
    int32_t airAngle;
    _jheightstuff currentmapinfo;
    _jheightstuff mapinfo;
    int32_t *lastpolyhit;
    uint32_t flags;
    uint8_t clipcode;
    int8_t hangcheck;
    uint8_t airstick;
    int32_t maxledge;
    FVECTOR ledgepoint;
    int32_t ledgeangle;
    struct physicsObject *standee;
    _solid *solid;
    int32_t falltimer;
    int32_t userdata[3];
    VECTOR uservector;
};

extern physicsObject maPhysicsData[JPB_PHYSICS_CAPACITY];
extern int32_t numsolids;
extern FVECTOR4 collisionfrustrum[6];
extern FVECTOR4 clippingfrustrum[6];
extern uint8_t initialLevelPauseDelay;
extern float fGlobalFrameRate;
extern FVECTOR globalgravity;
extern _collidevars cvars;
extern _collide_info bestinfo;
extern _movement_packet mvp;
extern uint16_t eventarray[30][15];
extern char *maphitsounds[16];
/* Exact PDB global type 0x8F1A: float[20][20]. */
extern float maRange[JPB_PHYSICS_CAPACITY][JPB_PHYSICS_CAPACITY];

void UpdatePublicVars(physicsObject *physics);
void UpdateSceneObject(physicsObject *physics);
int32_t *physics_GetPoly(objectRoot *object);
void physics_InitPhysics(void);
int physics_MapAnimCallBack(
    int32_t *arguments, objectRoot *object);
/*
 * Exact PDB procedure at matched-PC RVA 0xE0AF0. The caller supplies one of
 * the two initialized Jedi slots, as in the original game flow.
 */
void physics_ResetJedi(int index);
int CalcNewBox(int h1, FVECTOR4 *frustplane, FVECTOR4 *box);
void CalcRelativePosFromWorld(
    _solid *s, FVECTOR *world, FVECTOR *relative);
void CalcSolidRelativePos(
    _solid *s, physicsObject *physics, FVECTOR *pos);
void CalcWorldPosFromRelative(
    _solid *s, FVECTOR *relative, FVECTOR *world);
void buildfrustrum(
    MATRIX *m,
    FVECTOR4 *collide,
    VECTOR *campos,
    float percent,
    float xoff,
    float yoff);
void buildplane(
    MATRIX *m,
    VECTOR *campos,
    FVECTOR4 *plane,
    float x,
    float y,
    float z);
int jpb_PhysicsPlaneCheck(int radius, FVECTOR4 *plane);
int jpb_PhysicsGeneralCollide(
    _solid *solid,
    FVECTOR *movement,
    FVECTOR *from,
    float velocity,
    float radius);
int jpb_PhysicsPolyCollideCheck(void);
int jpb_PhysicsSphereAndPoly(void);
int newclosestPoly(
    FVECTOR *from,
    FVECTOR *to,
    FVECTOR *move,
    float distance,
    float radius,
    FVECTOR *movenormal,
    int playerid,
    int32_t **ppCube,
    int32_t **ppEntry,
    int32_t **ppPoly);
/*
 * Inferred inspection facade for the exact physics.c module-local
 * `whichsolid`. The matched program has a second, unrelated module-local
 * variable with the same PDB name in intersec.c.
 */
_solid *jpb_PhysicsGetWhichSolid(void);
int jpb_PhysicsCheckCubeBlocking(
    playerObject *player,
    FVECTOR *world,
    FVECTOR *dir,
    FVECTOR *dirNormal,
    float dist,
    float *ground);
/*
 * Descriptive test/integration facade for CheckCubeBlocking's exact
 * file-local STREETS terminal trigger.
 */
int jpb_PhysicsTryStartStreetsEnding(
    physicsObject *physics, int collision_type);
int BigBlowMe(VECTOR *pos, int force);
int BlowUp(int *entry, VECTOR *pos, int force);
void LaunchMapAnimEffects(int n, VECTOR *worldpos, int32_t *elist);
void MovePlayer(physicsObject *physics);
void ProcessPhysicsObjects(void);
/*
 * Inferred inspection facade for the inline, instruction-reviewed
 * WorldBlocking splash sequence.
 */
void jpb_PhysicsLaunchSplash(physicsObject *physics);
int jpb_PhysicsWorldBlocking(
    playerObject *player,
    physicsObject *physics,
    FVECTOR *startpos,
    FVECTOR *endpos,
    FVECTOR *direction,
    float distance);
/*
 * Descriptive integration facade for exact file-local PDB procedure
 * CharBlocking.
 */
int jpb_PhysicsCharBlockingState(
    playerObject *player,
    physicsObject *p0,
    physicsObject *p1,
    FVECTOR *testpos0,
    float *range);
int jpb_PhysicsMoveCharacterContacts(physicsObject *p0);
void jpb_PhysicsBeginObjectFrame(physicsObject *physics);
/*
 * Descriptive integration facade for exact file-local PDB procedure
 * CalcMovement. The reviewed implementation currently covers MOVE_NORMAL,
 * MOVE_HOVER, MOVE_HOVER3D, MOVE_FLY, MOVE_BLOWN, and MOVE_COREDEATH.
 */
int jpb_PhysicsCalcMovement(physicsObject *physics);
/* Compatibility facade requiring MOVE_NORMAL on entry. */
int jpb_PhysicsCalcMovementNormal(physicsObject *physics);
/*
 * Validating integration facade for the exact PDB procedure MovePlayer.
 * MOVE_FLY and MOVE_COREDEATH do not require a component graph; every other
 * mode requires the original scene/player relationship.
 */
int jpb_PhysicsMovePlayer(physicsObject *physics);
int jpb_PhysicsMoveNoContact(physicsObject *physics);
/*
 * Validating integration facade for the exact PDB procedure
 * UpdateSceneObject. The original scheduler only passes records from
 * maPhysicsData with a complete scene/player component graph.
 */
int jpb_PhysicsUpdateSceneObject(physicsObject *physics);
/*
 * Descriptive validation facade for exact file-local PDB procedure
 * checkdriving, which mirrors a driven vehicle's physics state back to
 * player slots zero and one.
 */
int jpb_PhysicsSyncDriverState(int player_index);
void DebugPlayer(physicsObject *physics);
int console_HideMeshCommand(
    int argument_count,
    char **string_arguments,
    int *integer_arguments,
    float *float_arguments);
int console_ShowMeshCommand(
    int argument_count,
    char **string_arguments,
    int *integer_arguments,
    float *float_arguments);
void dumpmatrix(MATRIX *matrix);
/*
 * Descriptive integration facade for exact file-local PDB procedures
 * BuildSolids and BuildNodeVertexList. Solid records retain their original
 * single-allocation ownership: normals aliases the tail of coords.
 */
void jpb_PhysicsBuildSolids(void);
/*
 * Descriptive integration/test seam for the exact physics.c module-local
 * `streetsending` countdown, now owned by the recovered level-eight trigger.
 */
int jpb_PhysicsGetStreetsEndingCountdown(void);
void jpb_PhysicsSetStreetsEndingCountdown(int countdown);
void physics_gCalcTargetPos(int player_num, VECTOR *offset);
int physics_gCheckGround(playerObject *player);
void physics_gClrConstantVector(objectRoot *object);
physicsObject *physics_gCreateObject(sceneObject *scene);
int32_t physics_gFaceTarget(
    objectRoot *player, objectRoot *target);
int32_t physics_gForceFaceTarget(
    objectRoot *player, objectRoot *target);
FVECTOR *physics_gGetConstantVector(objectRoot *object);
int32_t physics_gGetFaceTargetDelta(
    objectRoot *player, objectRoot *target);
int physics_gGetFacing(objectRoot *object);
objectRoot *physics_gGetNearestTarget(
    objectRoot *object0, int type);
int physics_gGetRange(
    objectRoot *object0, objectRoot *object1);
physicsObject *physics_FindWithinRange(
    VECTOR *position, uint32_t *mask, int range);
int physics_FindNearestEnemy(
    objectRoot *object0, int type);
playerObject *FindBestMachineGunTarget(
    VECTOR *pos,
    VECTOR *angle,
    playerObject *tank,
    int range,
    int maxangle,
    int maxheightdiff,
    int jedi);
int32_t physics_ForceFaceLock(
    objectRoot *player, objectRoot *locked);
physicsObject *physics_gGetNewObject(int ID);
VECTOR *physics_gGetPosition(objectRoot *object);
void physics_gInitObjects(int start);
void physics_gModFacing(objectRoot *object, int amount);
void physics_gSetCharge(playerObject *player, int charge, int charge_acc);
void physics_gSetConstantVector(
    objectRoot *object, float vx, float vy, float vz);
void physics_gSetFacing(objectRoot *object, int facing);
void physics_gSetPosition(
    objectRoot *object, int x, int y, int z);
void physics_gSetRecoil(
    playerObject *player, int recoil, int acc, int REV);
void physics_gSnapShotPosition(
    objectRoot *object, int yoffset);
void physics_gSwapVel(playerObject *player);
void physics_gTurnToAttack(
    objectRoot *object, int facing, int scalar);
void physics_gTurnToFace(
    objectRoot *object, int facing, int scalar);

#if defined(__cplusplus)
#define JPB_PHYSICS_STATIC_ASSERT static_assert
#else
#define JPB_PHYSICS_STATIC_ASSERT _Static_assert
#endif

JPB_PHYSICS_STATIC_ASSERT(
    sizeof(MOVE_MODE) == 4, "MOVE_MODE must match PDB type 0x104C");
JPB_PHYSICS_STATIC_ASSERT(
    sizeof(_collide_info) == 52,
    "_collide_info must match PDB type 0x6982");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(_collide_info, kisspoint) == 8,
    "_collide_info.kisspoint layout changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(_collide_info, facenormal) == 36,
    "_collide_info.facenormal layout changed");
JPB_PHYSICS_STATIC_ASSERT(
    sizeof(_movement_packet) == 184,
    "_movement_packet must match PDB type 0x8F49");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(_movement_packet, radius) == 120,
    "_movement_packet.radius layout changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(_movement_packet, info) == 132,
    "_movement_packet.info layout changed");
#if UINTPTR_MAX == UINT64_MAX
JPB_PHYSICS_STATIC_ASSERT(
    sizeof(_collidevars) == 240,
    "_collidevars must match PDB type 0x8F73");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(_collidevars, edge) == 80,
    "_collidevars.edge x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(_collidevars, vmin) == 176,
    "_collidevars.vmin x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(_collidevars, edge_start) == 232,
    "_collidevars.edge_start x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    sizeof(_jheightstuff) == 24,
    "_jheightstuff must match PDB type 0x1222");
JPB_PHYSICS_STATIC_ASSERT(
    sizeof(_solid) == 136,
    "_solid must match PDB type 0x1242");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(_solid, coords) == 56,
    "_solid.coords x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(_solid, rotmatrix) == 72,
    "_solid.rotmatrix x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(_solid, scale) == 120,
    "_solid.scale x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    sizeof(physicsObject) == 504,
    "physicsObject must match PDB type 0x125F");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(physicsObject, angle) == 72,
    "physicsObject.angle x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(physicsObject, pos) == 104,
    "physicsObject.pos x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(physicsObject, constmov) == 240,
    "physicsObject.constmov x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(physicsObject, mov) == 276,
    "physicsObject.mov x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(physicsObject, airGround) == 336,
    "physicsObject.airGround x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(physicsObject, currentmapinfo) == 368,
    "physicsObject.currentmapinfo x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(physicsObject, currentmapinfo) +
            offsetof(_jheightstuff, poly) == 384,
    "physicsObject.currentmapinfo.poly x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(physicsObject, flags) == 424,
    "physicsObject.flags x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(physicsObject, userdata) == 476,
    "physicsObject.userdata x64 offset changed");
JPB_PHYSICS_STATIC_ASSERT(
    offsetof(physicsObject, uservector) == 488,
    "physicsObject.uservector x64 offset changed");
#elif UINTPTR_MAX != UINT32_MAX
#error Unsupported pointer width
#endif

#undef JPB_PHYSICS_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
