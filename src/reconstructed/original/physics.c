/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\physics.c.
 *
 * The reviewed subset establishes the scene-root-to-physics boundary, actor
 * contact state, exact landing dispatch, and the original inline splash
 * effect/audio sequence.
 *
 * Provenance:
 *   direct     - names/signatures/locals, physicsObject/sceneObject layouts,
 *                and file-local splasheffects[30] from the exact PDB.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - member offsets, conversion order, null behavior, and scene
 *                readiness/facing-lock branch, WorldBlocking landing call,
 *                inline splash sequence, CalcMovement's processed/recursive
 *                standee entry, CharBlocking, and the complete MovePlayer
 *                scheduler including character contacts, fall, ledge,
 *                special-player, platform, and map-contact paths, the exact
 *                FindBestMachineGunTarget actor/range/facing filter, plus the
 *                complete UpdateSceneObject publication entry checked at
 *                exact RVAs, and the complete ProcessPhysicsObjects frame
 *                scheduler and terminal street-ending sequence, including
 *                the complete Level 10-aware physics_ResetJedi entry.
 *                Exact cached/fallback range lookup is checked at
 *                0xE1830..0xE18A0. The ground query, target-position
 *                transform, physics component creator, range-cache
 *                initializer, map callback, polygon accessor, and nearest
 *                daDelay target selector are checked at their complete
 *                0xE0A20..0xE1735 instruction ranges.
 *
 * PDB module: 0060
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\physics.obj
 * Primary source: W:\SWJediPowerBattles\Work\physics.c
 * Compiler language: c
 * Emitted procedures: 62
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/physics.h"
#include "jpb/alloc.h"
#include "jpb/anim.h"
#include "jpb/animctrl.h"
#include "jpb/animutil.h"
#include "jpb/bmd.h"
#include "jpb/brain.h"
#include "jpb/brainutl.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/cube.h"
#include "jpb/effects.h"
#include "jpb/extracharacters.h"
#include "jpb/flex.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/intersec.h"
#include "jpb/jonny.h"
#include "jpb/model.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/vehicle.h"
#include "jpb/vectors.h"
#include "jpb/world.h"
#include "jpb/wrender.h"

#include <limits.h>
#include <math.h>
#include <string.h>

/*
 * Portable fallback for the still-bounded loader_CreateModel relocation seam.
 * Exact loaded models continue through addPtr/getPtr; immutable JPBBmdView
 * owners can resolve their original file-relative geomData offsets first.
 */
static JPBPhysicsGeometryStreamResolver
    physics_geometry_stream_resolver;
static void *physics_geometry_stream_resolver_user_data;

void jpb_PhysicsSetGeometryStreamResolver(
    JPBPhysicsGeometryStreamResolver resolver,
    void *user_data)
{
    physics_geometry_stream_resolver = resolver;
    physics_geometry_stream_resolver_user_data = user_data;
}

void *jpb_PhysicsResolveGeometryStream(
    const geomData *geometry,
    int pointer_type)
{
    void *resolved = NULL;
    int index;

    if (geometry == NULL ||
        pointer_type < JPB_POINTER_ARRAY_VERTEX ||
        pointer_type > JPB_POINTER_ARRAY_INDEX) {
        return NULL;
    }
    if (physics_geometry_stream_resolver != NULL) {
        resolved = physics_geometry_stream_resolver(
            geometry,
            pointer_type,
            physics_geometry_stream_resolver_user_data);
        if (resolved != NULL) {
            return resolved;
        }
    }
    switch (pointer_type) {
    case JPB_POINTER_ARRAY_VERTEX:
        index = geometry->pVertex;
        break;
    case JPB_POINTER_ARRAY_NORMAL:
        index = geometry->pNormal;
        break;
    case JPB_POINTER_ARRAY_UV:
        index = geometry->pUV;
        break;
    case JPB_POINTER_ARRAY_COLOR:
        index = geometry->pColor;
        break;
    case JPB_POINTER_ARRAY_INDEX:
        index = geometry->pIndex;
        break;
    default:
        return NULL;
    }
    return getPtr(index, pointer_type);
}

/* Direct globals at RVAs 0x951AA0, 0x951BA0, 0x951B90, and 0x9543E0. */
_collidevars cvars;
physicsObject maPhysicsData[JPB_PHYSICS_CAPACITY];
int32_t numsolids;
float maRange[JPB_PHYSICS_CAPACITY][JPB_PHYSICS_CAPACITY];

/*
 * Exact globals at RVAs 0x4CC0D0 and 0x4CC0E0. Their PDB type 0x8F6D is an
 * incomplete CollisionData[]; the branch bounds and executable bytes prove
 * the four and six live records respectively.
 */
CollisionData maDesert_BNodeSizes[] = {
    {0x100, 0x00, -1},
    {0x100, 0x0e, -1},
    {0x080, 0x14, -1},
    {0x080, 0x15, -1},
};
CollisionData maWormNodeSizes[] = {
    {0x100, 0x00, -1},
    {0x0e6, 0x0e, -1},
    {0x100, 0x0f, -1},
    {0x0cc, 0x17, -1},
    {0x066, 0x19, -1},
    {0x066, 0x1a, -1},
};

static void CalcWorldRelativePos(_solid *s, physicsObject *p);
static void CharBlocking(
    playerObject *player,
    physicsObject *p0,
    physicsObject *p1,
    FVECTOR *pos0,
    FVECTOR *testpos0,
    FVECTOR *dir0,
    float dist0,
    float *range);
static int generalCollide(
    _solid *s,
    FVECTOR *mov,
    FVECTOR *from,
    float vel,
    float radius);
static int planecheck(int radius, FVECTOR4 *v);
static int polycollidecheck(void);
static int sphereAndPoly(void);
static void checkdriving(int player_index);
static int WorldBlocking(
    playerObject *player,
    physicsObject *p0,
    FVECTOR *startpos,
    FVECTOR *endpos,
    FVECTOR *direction,
    float distance);
static _svector *BuildNodeVertexList(_solid *s);
static void BuildSolids(void);

/* Exact PDB-named physics.c module static at RVA 0x53A590. */
static VECTOR p;

/* Exact PDB-named physics.c module statics at RVAs 0x53A520/0x53A530. */
static FVECTOR edge_start;
static FVECTOR edge_end;
static FVECTOR ledgeoff;
static FVECTOR tmpnorm;
static FVECTOR tmpmove;
static int solidhack;
static FVECTOR4 box[5];
static int streetsending;

/* Direct PDB global at RVA 0x53A4E0. */
_collide_info bestinfo;
_movement_packet mvp;

/* Exact PDB-named physics.c module static at RVA 0x954320. */
static _solid *whichsolid;

/* Exact PDB-named physics.c char[30] at RVA 0x4CB8B0. */
static char splasheffects[30] = {
    61, 61, 61, 61, 61, 61, 61, 61, 61, 61,
    61, 61, 61, 61, 61, 61, 61, 61, 61, 61,
    61, 61, 61, 61, 61, 0, 0, 0, 0, 0
};

_solid *jpb_PhysicsGetWhichSolid(void)
{
    return whichsolid;
}

static int32_t physics_trunc_float_to_i32(float value)
{
    if (!isfinite(value) ||
        value >= 2147483648.0f ||
        value < -2147483648.0f) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int32_t physics_abs_truncated_float(float value)
{
    int32_t truncated = physics_trunc_float_to_i32(value);

    if (truncated < 0 && truncated != INT32_MIN) {
        truncated = -truncated;
    }
    return truncated;
}

static int16_t physics_i16_from_bits(uint16_t value)
{
    if (value <= INT16_MAX) {
        return (int16_t)value;
    }
    return (int16_t)((int32_t)value - 65536);
}

static uint32_t physics_load_u32(const void *source)
{
    uint32_t value;

    memcpy(&value, source, sizeof(value));
    return value;
}

static int16_t physics_add_low_i16(int16_t a, uint32_t b)
{
    uint16_t value =
        (uint16_t)((uint32_t)(uint16_t)a + b);

    return physics_i16_from_bits(value);
}

static int16_t physics_negated_add_low_i16(int16_t a, uint32_t b)
{
    uint16_t value =
        (uint16_t)(0U - ((uint32_t)(uint16_t)a + b));

    return physics_i16_from_bits(value);
}

static uint32_t physics_arithmetic_shift_right_8(uint32_t value)
{
    return
        (value >> 8) |
        ((value & UINT32_C(0x80000000)) != 0
             ? UINT32_C(0xff000000)
             : 0);
}

static int physics_map_contact_events_enabled(void)
{
    int level = (int)(int8_t)LevelSelect;

    return level != 1 && level != 4 && level != 9;
}

static int physics_pack_map_event_cube(const VECTOR *position)
{
    uint32_t x =
        physics_arithmetic_shift_right_8(
            UINT32_C(0x80ff) - (uint32_t)position->vx);
    uint32_t y =
        ((uint32_t)position->vy & UINT32_C(0xffffff00)) << 8;
    uint32_t z =
        ((uint32_t)position->vz + UINT32_C(0x7f00)) &
        UINT32_C(0xffffff00);

    return (int32_t)(x | y | z);
}

static VECTOR physics_map_event_position(const FVECTOR *position)
{
    VECTOR result = {
        physics_trunc_float_to_i32(position->vx),
        physics_trunc_float_to_i32(position->vy),
        physics_trunc_float_to_i32(position->vz),
        0
    };

    return result;
}

static void physics_launch_map_contact(
    int32_t *entry, int hitforce, const FVECTOR *position)
{
    VECTOR worldpos;
    int n;

    if (entry == NULL || !physics_map_contact_events_enabled()) {
        return;
    }

    worldpos = physics_map_event_position(position);
    n = HitsHit(
        leveldata,
        entry,
        hitforce,
        physics_pack_map_event_cube(&worldpos),
        (int32_t *)(void *)gaScratch);
    if (n != 0) {
        LaunchMapAnimEffects(
            n, &worldpos, (int32_t *)(void *)gaScratch);
    }
}

static void physics_launch_splash(physicsObject *physics)
{
    VECTOR position = {
        physics_trunc_float_to_i32(physics->pos.vx),
        physics_trunc_float_to_i32(physics->pos.vy),
        physics_trunc_float_to_i32(physics->pos.vz),
        0
    };
    int level = (int)(int8_t)LevelSelect;
    int effect = (int)(int8_t)splasheffects[level];
    EffectHeader *header;

    physics->airTime = 0xf000;
    position.vy =
        intersec_FindWalkHeight(&position, NULL, NULL, 0);
    header = paEffects[effect];
    sprite_AddSpriteEffect(
        header->aEffects,
        (int)header->num,
        &position,
        NULL);
    (void)sound_PlayFV(
        &physics->pos, 3, "splash", 0);
}

void jpb_PhysicsLaunchSplash(physicsObject *physics)
{
    if (physics != NULL) {
        physics_launch_splash(physics);
    }
}

static void physics_launch_fixed_effect(
    unsigned effect_index, const FVECTOR *position)
{
    VECTOR worldpos = physics_map_event_position(position);
    EffectHeader *effect = paEffects[effect_index];

    sprite_AddSpriteEffect(
        effect->aEffects, (int)effect->num, &worldpos, NULL);
}

static uint32_t physics_map_flags(const int32_t *poly)
{
    return (uint32_t)leveldata[(uint32_t)*poly & UINT32_C(0x1ffff)];
}

static int32_t physics_sign_extend_10(uint32_t value)
{
    value &= UINT32_C(0x3ff);
    if ((value & UINT32_C(0x200)) != 0) {
        return (int32_t)value - 0x400;
    }
    return (int32_t)value;
}

static void physics_decode_map_normal(
    int32_t *mapbase, uint32_t normal_index, FVECTOR *normal)
{
    uint32_t packed = (uint32_t)mapbase[normal_index + 1U];

    normal->vx =
        (float)(physics_sign_extend_10(packed) * 8) *
        (1.0f / 4096.0f);
    normal->vy =
        (float)(physics_sign_extend_10(packed >> 10) * 8) *
        (1.0f / 4096.0f);
    normal->vz =
        (float)(physics_sign_extend_10(packed >> 20) * 8) *
        (1.0f / 4096.0f);
}

/*
 * Descriptive extraction of CalcMovement's per-axis slope merge at RVAs
 * 0xDB6E7..0xDB7D6. A zero surface component preserves existing motion;
 * opposing nonzero motion is replaced by the original two-unit nudge.
 */
static float physics_merge_slope_component(
    float surface_component, float old_component)
{
    if (surface_component == 0.0f) {
        return old_component;
    }
    if (old_component != 0.0f &&
        ((old_component > 0.0f && surface_component < 0.0f) ||
         (old_component < 0.0f && surface_component > 0.0f))) {
        return surface_component > 0.0f ? 2.0f : -2.0f;
    }
    return surface_component;
}

static physicsObject *physics_from_root(objectRoot *object)
{
    sceneObject *scene = (sceneObject *)object->pParent;

    return (physicsObject *)scene->pPhysics;
}

/* 0xDA750, 117 bytes, global, 6 named locals
 * BigBlowMe
 * PDB type: int (VECTOR*, int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int BigBlowMe(VECTOR *pos, int force)
{
    int n = (int)(int8_t)LevelSelect;
    int cubeshite;

    if (n == 9 || n == 4) {
        return n;
    }
    cubeshite =
        (int)(
            (((uint32_t)pos->vy &
              UINT32_C(0xffffff00)) <<
             8) |
            (((uint32_t)pos->vz +
              UINT32_C(0x00007f00)) &
             UINT32_C(0xffffff00)) |
            ((uint32_t)(0x80ff - pos->vx) >>
             8));
    n = BlockBuster(
        leveldata, force, cubeshite);
    LaunchMapAnimEffects(
        n,
        pos,
        (int32_t *)(void *)gaScratch);
    return n;
}

/* 0xDA7D0, 165 bytes, global, 7 named locals
 * BlowUp
 * PDB type: int (int*, VECTOR*, int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int BlowUp(int *entry, VECTOR *pos, int force)
{
    int n;
    int cubeshite;
    int level;

    if (entry == NULL) {
        return 0;
    }
    level = (int)(int8_t)LevelSelect;
    if ((unsigned)level <= 9U &&
        (UINT32_C(0x212) & (UINT32_C(1) << level)) != 0) {
        return 0;
    }

    cubeshite =
        (int)(
            (((uint32_t)pos->vy & UINT32_C(0xffffff00)) << 8) |
            (((uint32_t)pos->vz + UINT32_C(0x00007f00)) &
             UINT32_C(0xffffff00)) |
            (uint32_t)((int32_t)(
                UINT32_C(0x000080ff) - (uint32_t)pos->vx) >> 8));
    n = HitsHit(
        leveldata,
        entry,
        force,
        cubeshite,
        (int32_t *)(void *)gaScratch);
    if (n != 0) {
        LaunchMapAnimEffects(
            n, pos, (int32_t *)(void *)gaScratch);
    }
    return n;
}

/* 0xDA880, 450 bytes, local, 9 named locals
 * BuildNodeVertexList
 * PDB type: _svector* (_solid*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static _svector *BuildNodeVertexList(_solid *s)
{
    int ident = s->object->sceneRoot.objectID;
    modelObject *m = (modelObject *)s->object->pModel;
    Mnode *h = coll_GetNode(ident, (unsigned)s->node);
    geomData *g;
    physicsObject *p;
    _svector *dst;
    const uint8_t *rot;
    sceneObject *owner;
    _svector tmp = {0, 0, 0, 0};
    uint32_t vertex_count;

    if (h == NULL) {
        s->coords = NULL;
        s->normals = NULL;
        return NULL;
    }

    g = h->pGeomData;
    p = (physicsObject *)s->object->pPhysics;
    dst = s->coords;
    vertex_count = (uint32_t)g->numVerts * 3U;
    if (dst == NULL) {
        uint32_t vector_count =
            vertex_count + (uint32_t)g->numFaces;

        dst = (_svector *)memalloc(
            vector_count * (uint32_t)sizeof(*dst));
        if (dst == NULL) {
            return NULL;
        }
        s->coords = dst;
        s->normals = dst + vertex_count;
    }

    /*
     * The PDB names `rot` as VECTOR*, although coll_GetNodeRotation returns
     * _svector*. The executable consequently consumes three overlapping
     * 32-bit words beginning at Mnode.v3CurrentRotation. Preserve that
     * shipped behavior explicitly, without an aliasing or alignment fault.
     */
    rot = (const uint8_t *)(const void *)
        coll_GetNodeRotation(ident, 0);
    owner = (sceneObject *)(void *)
        ((sceneObject *)p->physicsRoot.pParent)->pScene;
    tmp.vx = physics_add_low_i16(
        owner->v3WorldAngle.vx, physics_load_u32(rot));
    tmp.vy = physics_add_low_i16(
        owner->v3WorldAngle.vy,
        physics_load_u32(rot + sizeof(uint32_t)));
    tmp.vz = physics_negated_add_low_i16(
        owner->v3WorldAngle.vz,
        physics_load_u32(rot + 2U * sizeof(uint32_t)));

    fRotMatrixZYX(&tmp, &s->rotmatrix);
    s->rotmatrix.t[0] = 0;
    s->rotmatrix.t[1] = 0;
    s->rotmatrix.t[2] = 0;
    SetTransformMatrix(&s->rotmatrix);
    SetGTETransLV(NULL);
    ApplyMatrixMany10Bit(
        (int *)jpb_PhysicsResolveGeometryStream(
            g, JPB_POINTER_ARRAY_NORMAL),
        s->normals,
        g->numFaces,
        0x13);

    fScaleMatrix(&s->rotmatrix, &m->v3Scale);
    SetTransformMatrix(&s->rotmatrix);
    SetGTETransLV(&h->v3RotCenter);
    ApplyMatrixMany10Bit(
        (int *)jpb_PhysicsResolveGeometryStream(
            g, JPB_POINTER_ARRAY_VERTEX),
        s->coords,
        (int)vertex_count,
        0x16);

    s->scale.vx = 0x01000000 / m->v3Scale.vx;
    s->scale.vy = s->scale.vx;
    s->scale.vz = s->scale.vx;
    s->modelnode = h;
    s->geometry = g;
    s->model = m;
    s->physics = p;
    return dst;
}

/* 0xDAA50, 517 bytes, local, 7 named locals
 * BuildSolids
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static void BuildSolids(void)
{
    int i;

    numsolids = 0;
    if (gSCENE_READY == 0) {
        return;
    }

    for (i = 2; i < JPB_PHYSICS_CAPACITY; ++i) {
        physicsObject *p1 = &maPhysicsData[i];
        sceneObject *scene;
        playerObject *player;
        wsl_ENEMY *enemy;
        modelObject *model;
        uint32_t enemy_flags;
        uint32_t flags = 0;
        int j;
        _solid *s;

        if (p1->physicsRoot.objectID == -1 ||
            obj_gCheckObjectFlag(
                &p1->physicsRoot, 0, UINT32_C(0x20)) != 0) {
            continue;
        }
        scene = (sceneObject *)p1->physicsRoot.pParent;
        player = (playerObject *)scene->pPlayer;
        enemy = player->pEnemy;
        if (enemy == NULL) {
            continue;
        }
        enemy_flags = enemy->enemyFlags;
        if ((enemy_flags & UINT32_C(0xc000)) == 0) {
            continue;
        }
        model = (modelObject *)scene->pModel;
        if ((model->flags & UINT32_C(2)) == 0) {
            continue;
        }

        for (j = 0; j < JPB_PHYSICS_CAPACITY; ++j) {
            physicsObject *other = &maPhysicsData[j];

            if (other == p1 ||
                other->physicsRoot.objectID == -1 ||
                obj_gCheckObjectFlag(
                    &other->physicsRoot, 0, UINT32_C(0x20)) != 0) {
                continue;
            }
            if (vec_QuickRangeCheckFV(
                    &other->pos,
                    &p1->pos,
                    (float)((int)p1->height + 0x200)) != 0) {
                flags |= UINT32_C(1) << (unsigned)j;
            }
        }

        if (flags == 0) {
            if (p1->solid != NULL) {
                if (p1->solid->coords != NULL) {
                    memfree(p1->solid->coords);
                    p1->solid->coords = NULL;
                    p1->solid->normals = NULL;
                }
                memfree(p1->solid);
                p1->solid = NULL;
            }
            continue;
        }

        s = p1->solid;
        if (s == NULL) {
            s = (_solid *)memalloc((unsigned)sizeof(*s));
            p1->solid = s;
            if (s == NULL) {
                return;
            }
            s->coords = NULL;
            s->normals = NULL;
        }
        p1->flags |= UINT32_C(0x400);
        s->physics = p1;
        s->node = 0;
        s->object = &maSceneData[i];
        s->flags = flags;
        if ((enemy_flags & UINT32_C(0x8000)) != 0) {
            s->flags |= UINT32_C(2);
        }
        if ((enemy_flags & UINT32_C(0x4000)) != 0) {
            s->flags |= UINT32_C(1);
        }
        if (BuildNodeVertexList(s) == NULL) {
            return;
        }
        ++numsolids;
    }
}

void jpb_PhysicsBuildSolids(void)
{
    BuildSolids();
}

int jpb_PhysicsGetStreetsEndingCountdown(void)
{
    return streetsending;
}

void jpb_PhysicsSetStreetsEndingCountdown(int countdown)
{
    streetsending = countdown;
}

/* 0xE0AF0, 1574 bytes, global, 4 named locals
 * physics_ResetJedi
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 *
 * The matched compiler fully unrolled the 20-slot userdata loop. Disassembly
 * proves that level ten preserves slot state for two-player vehicle ID 0x4c,
 * and also for the otherwise-unused player-count values outside one and two.
 */
void physics_ResetJedi(int index)
{
    physicsObject *physics = &maPhysicsData[index];
    playerObject *player = &gaPlayerData[index];
    int height;
    int object_index;

    physics->flags &= UINT32_C(0xffe4bfc0);
    physics->movemode = MOVE_NORMAL;
    height = intersec_FindWalkHeightFV(
        &physics->pos, NULL, &physics->physicsRoot, 0);
    player->pFlags &= UINT32_C(0xfffffffe);
    physics->solidgrabbed = NULL;
    physics->noncollideframes = 0;
    physics->collidetime = 0;
    physics->anycollidetime = 0;
    physics->reversoi = 0;
    physics->clipcode = 0;
    physics->hangcheck = 0;
    physics->standee = NULL;
    physics->airTime = 0;
    physics->pos.vy = (float)height;
    memset(&physics->airmov, 0, sizeof(physics->airmov));
    UpdateSceneObject(physics);

    for (object_index = 0;
         object_index < JPB_PHYSICS_CAPACITY;
         ++object_index) {
        if (LevelSelect != 10 ||
            GameStruct.NumPlayers == 1 ||
            (GameStruct.NumPlayers == 2 &&
             gaPlayerData[object_index].playerID != 0x4c)) {
            maPhysicsData[object_index].userdata[0] = 0;
        }
    }
}

static int physics_streets_player_is_finishing_stap(
    int player_index, int collision_object_id)
{
    playerObject *candidate = &gaPlayerData[player_index];

    return candidate->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(
            &candidate->playerRoot, 0, UINT32_C(0x20)) == 0 &&
        (candidate->pFlags & UINT32_C(0x00040200)) == 0 &&
        collision_object_id == stapbikeindex[player_index] - 1;
}

static int physics_try_start_streets_ending(
    physicsObject *physics,
    playerObject *collision_player,
    int collision_type)
{
    uint32_t collision_timeout;
    int eligible;
    EffectHeader *effect;

    if (LevelSelect != 8 ||
        totalframes <= 0x20) {
        return 0;
    }
    collision_timeout =
        jpb_StreetsEndingShortCollisionTimeout != 0
            ? UINT32_C(0x0000f000)
            : UINT32_C(0x0001e000);
    if ((jpb_CubeRuntimeFlags & UINT32_C(0x00000008)) != 0 ||
        (bestinfo.flags & 8) != 0 ||
        collision_type != 1 ||
        (bestinfo.facenormal.vx <= 0.9f &&
         physics->anycollidetime <= collision_timeout)) {
        return 0;
    }

    eligible = physics_streets_player_is_finishing_stap(
        0, collision_player->playerRoot.objectID);
    if (GameStruct.NumPlayers == 2 &&
        physics_streets_player_is_finishing_stap(
            1, collision_player->playerRoot.objectID)) {
        eligible = 1;
    }
    if (!eligible || streetsending != 0) {
        return 0;
    }

    effect = paEffects[18];
    sprite_AddSpriteEffect(
        effect->aEffects,
        (int)effect->num,
        &physics->vpos,
        NULL);
    jpb_CubeRuntimeFlags |= UINT32_C(0x00000008);
    streetsending = 0x00013800;
    physics->collidetime = 0;
    physics->anycollidetime = 0;
    obj_gSetObjectFlag(
        &physics->physicsRoot, 0, UINT32_C(0x20));
    physics_ResetJedi(0);
    physics_ResetJedi(1);
    player_ResetJedi(0);
    player_ResetJedi(1);
    (void)sound_Play(
        &physics->vpos, 0, (char *)"explomed", 0);
    stapbikeindex[0] = 0;
    stapbikeindex[1] = 0;
    if (stapsound != 0) {
        sound_StopSound(stapsound);
        stapsound = 0;
    }
    return 1;
}

int jpb_PhysicsTryStartStreetsEnding(
    physicsObject *physics, int collision_type)
{
    sceneObject *scene;

    if (physics == NULL ||
        physics->physicsRoot.pParent == NULL) {
        return 0;
    }
    scene =
        (sceneObject *)physics->physicsRoot.pParent;
    if (scene->pPlayer == NULL) {
        return 0;
    }
    return physics_try_start_streets_ending(
        physics,
        (playerObject *)scene->pPlayer,
        collision_type);
}

/* 0xDAC60, 4212 bytes, local, 23 named locals
 * CalcMovement
 * PDB type: void (physicsObject*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */

/*
 * Reviewed extraction of every MOVE_MODE case from CalcMovement.
 *
 * Covered reference ranges:
 *   0xDAC60..0xDAD28  processed guard and recursive standee scheduling
 *   0xDAD28..0xDAE35  charge decay and currentmov/mov staging
 *   0xDAE60..0xDB0BD  map-triggered MOVE_BLOWN setup
 *   0xDB0EE..0xDB273  complete MOVE_BLOWN impulse, gravity, and exit phases
 *   0xDAEB9..0xDAF73  map-triggered MOVE_COREDEATH setup
 *   0xDB273..0xDB3F7  complete MOVE_COREDEATH return motion and teardown
 *   0xDB3F7..0xDB7FD  complete ordinary MOVE_NORMAL case, including reverse
 *                     air steering, facing rotation, and slope forcing
 *   0xDB902..0xDBAFC  complete MOVE_HOVER height control, yaw-relative
 *                     motion, turn contribution, and model scaling
 *   0xDBB01..0xDBBCC  complete shared MOVE_HOVER3D/MOVE_FLY target steering
 *   0xDB809..0xDBC84  shared frame scaling, landing-flag publication,
 *                     moving-platform delta, and directional conveyors
 *
 * Processed standee chains, including cycles, are scheduled recursively using
 * the original entry guard. MOVE_BLOWN returns through the runtime trajectory
 * callback-table slot used by the original extension layer. Map surfaces that
 * All anonymous cross-module state is exposed through documented portable
 * seams rather than decompiler placeholder names.
 */
int jpb_PhysicsCalcMovement(physicsObject *physics)
{
    enum {
        JPB_CALC_PLAYER_FLAG_AIRBORNE = 0x00000001,
        JPB_CALC_PLAYER_FLAG_LANDING = 0x00000008,
        JPB_CALC_PLAYER_FLAG_MOVEMENT_LOCK = 0x00000200,
        JPB_CALC_PLAYER_FLAG_SLOPE_FORCE = 0x10000000,
        JPB_CALC_PLAYER_SLOPE_BLOCK_MASK = 0x44000001,
        JPB_CALC_MAP_FLAG_SLOPE = 0x00014000,
        JPB_CALC_MAP_FLAG_MODE_TRANSITION = 0x00040000,
        JPB_CALC_MAP_FLAG_CONVEYOR_POS_Z = 0x00200000,
        JPB_CALC_MAP_FLAG_CONVEYOR_NEG_Z = 0x00400000,
        JPB_CALC_MAP_FLAG_CONVEYOR_POS_X = 0x00800000,
        JPB_CALC_MAP_FLAG_CONVEYOR_NEG_X = 0x01000000,
        JPB_CALC_MAP_FLAG_CONVEYOR_MASK = 0x01e00000
    };
    sceneObject *scene;
    playerObject *player;
    modelObject *model;
    _solid *s;
    unsigned currentgmi1;
    FVECTOR polynormal;
    FVECTOR oldmov;
    FVECTOR old;
    MATRIX rotation;
    FVECTOR rotated;
    float scale;
    int player_valid;

    if (physics == NULL || physics->physicsRoot.pParent == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    scene = (sceneObject *)physics->physicsRoot.pParent;
    player = (playerObject *)scene->pPlayer;
    model = (modelObject *)scene->pModel;
    if (player == NULL || model == NULL || scene->pAnim == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    if ((physics->movemode != MOVE_NORMAL &&
         physics->movemode != MOVE_HOVER &&
         physics->movemode != MOVE_HOVER3D &&
         physics->movemode != MOVE_FLY &&
         physics->movemode != MOVE_BLOWN &&
         physics->movemode != MOVE_COREDEATH) ||
        (physics->currentmapinfo.poly != NULL &&
         leveldata == NULL)) {
        return JPB_PHYSICS_PARTIAL_UNSUPPORTED_STATE;
    }
    if (physics->movemode == MOVE_COREDEATH &&
        player->playernum >= 2 &&
        player->pEnemy == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }

    currentgmi1 =
        physics->currentmapinfo.poly != NULL
            ? physics_map_flags(physics->currentmapinfo.poly)
            : 0;
    player_valid =
        player->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(
            &player->playerRoot, 0, 0x00000020u) == 0 &&
        (player->pFlags & 0x00040200u) == 0;
    if (physics->movemode == MOVE_NORMAL &&
        (currentgmi1 &
         (unsigned)JPB_CALC_MAP_FLAG_MODE_TRANSITION) != 0 &&
        physics->pos.vy - physics->airGround < 512.0f &&
        LevelSelect == 10 &&
        player_valid &&
        player->playernum >= 2 &&
        player->pEnemy == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    if (physics->movemode == MOVE_NORMAL &&
        (currentgmi1 &
         (unsigned)JPB_CALC_MAP_FLAG_MODE_TRANSITION) != 0 &&
        physics->pos.vy - physics->airGround < 512.0f &&
        LevelSelect == 10 &&
        player_valid &&
        (player->pFlags &
         (unsigned)JPB_CALC_PLAYER_FLAG_AIRBORNE) == 0 &&
        player->paMotions == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    if (physics->movemode == MOVE_NORMAL &&
        (currentgmi1 &
         (unsigned)JPB_CALC_MAP_FLAG_MODE_TRANSITION) != 0 &&
        physics->pos.vy - physics->airGround < 512.0f &&
        LevelSelect != 10 &&
        player->paMotions == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    if ((physics->flags & 0x00004000u) != 0) {
        return JPB_PHYSICS_PARTIAL_ALREADY_PROCESSED;
    }
    physics->flags |= 0x00004000u;

    s = NULL;
    if (player->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(
            &player->playerRoot, 0, 0x00000020u) == 0 &&
        (player->pFlags & 0x00040200u) == 0 &&
        physics->standee != NULL &&
        (player->pFlags & 0x00000001u) == 0) {
        s = physics->standee->solid;
        if (s != NULL) {
            int result;

            if (s->physics == NULL) {
                physics->flags &= ~UINT32_C(0x00004000);
                return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
            }
            if ((s->physics->flags &
                 UINT32_C(0x00004000)) == 0) {
                result =
                    jpb_PhysicsCalcMovement(s->physics);
                if (result != JPB_PHYSICS_PARTIAL_OK &&
                    result !=
                        JPB_PHYSICS_PARTIAL_ALREADY_PROCESSED) {
                    physics->flags &= ~UINT32_C(0x00004000);
                    return result;
                }
            }
        }
    }

    if (anim_CheckFreeze(&physics->physicsRoot) == 0) {
        if (physics->accel.vz > 0.0f) {
            float old_charge = physics->constmov.vz;

            if (old_charge >= 0.0f) {
                physics->constmov.vz =
                    old_charge - physics->accel.vz;
                if (physics->constmov.vz < 0.0f) {
                    physics->accel.vz = 0.0f;
                    physics->constmov.vz = 0.0f;
                }
            } else {
                physics->constmov.vz =
                    old_charge + physics->accel.vz;
                if (physics->constmov.vz > 0.0f) {
                    physics->accel.vz = 0.0f;
                    physics->constmov.vz = 0.0f;
                }
            }
        }
        player_valid =
            ((player->playerRoot.objectID != -1 &&
              obj_gCheckObjectFlag(
                  &player->playerRoot, 0, 0x00000020u) == 0 &&
              (player->pFlags & 0x00040200u) == 0) ||
             (uint16_t)(player->playerID - 0x35) < 4u);
        if (player_valid) {
            physics->currentmov = physics->constmov;
        } else {
            memset(&physics->currentmov, 0, sizeof(physics->currentmov));
        }
    } else {
        memset(&physics->currentmov, 0, sizeof(physics->currentmov));
    }

    if (physics->currentmov.vx != 0.0f ||
        physics->currentmov.vy != 0.0f ||
        physics->currentmov.vz != 0.0f) {
        physics->flags |= 0x00000800u;
    }
    physics->mov = physics->currentmov;

    if (s != NULL &&
        (physics->flags & 0x00001800u) == 0) {
        physics->angle.vy =
            (int32_t)(
                (float)s->physics->angle.vy +
                physics->localfacing.vy);
    }

    if (physics->movemode == MOVE_NORMAL &&
        (currentgmi1 &
         (unsigned)JPB_CALC_MAP_FLAG_MODE_TRANSITION) != 0 &&
        physics->pos.vy - physics->airGround < 512.0f &&
        LevelSelect == 10 &&
        player_valid) {
        physics->movemode = MOVE_COREDEATH;
        physics->uservector.vx =
            (int32_t)physics->pos.vx;
        physics->uservector.vz =
            (int32_t)physics->pos.vz;
        physics->userdata[0] =
            (player->pFlags &
             (unsigned)
                 JPB_CALC_PLAYER_FLAG_AIRBORNE) != 0
                ? (int32_t)physics->airmov.vy
                : 0;
        physics->mov.vx = 0.0f;
        physics->mov.vy = 0.0f;
        physics->airmov.vy = 0.0f;
        if ((player->pFlags &
             (unsigned)
                 JPB_CALC_PLAYER_FLAG_AIRBORNE) == 0) {
            (void)animctrl_MotionLock(
                &player->playerRoot,
                &player->paMotions[4]);
            player->pFlags |=
                (uint32_t)
                    JPB_CALC_PLAYER_FLAG_AIRBORNE;
        }
    }

    if (physics->movemode == MOVE_NORMAL &&
        (currentgmi1 &
         (unsigned)JPB_CALC_MAP_FLAG_MODE_TRANSITION) != 0 &&
        physics->pos.vy - physics->airGround < 512.0f &&
        LevelSelect != 10) {
        uint32_t normal_index =
            (uint32_t)*physics->currentmapinfo.poly &
            UINT32_C(0x1ffff);
        uint32_t packed =
            (uint32_t)leveldata[normal_index + 1U];
        int32_t normal_x =
            physics_sign_extend_10(packed) * 8;
        int32_t normal_y =
            physics_sign_extend_10(packed >> 10) * 8;
        int32_t normal_z =
            physics_sign_extend_10(packed >> 20) * 8;

        physics->userdata[0] = 0;
        physics->movemode = MOVE_BLOWN;
        if (normal_x > -8 && normal_x < 8) {
            normal_x = 0;
        }
        if (normal_y > -8 && normal_y < 8) {
            normal_y = 0;
        }
        if (normal_z > -8 && normal_z < 8) {
            normal_z = 0;
        }
        physics->uservector.vx =
            (int32_t)((float)normal_x * 5.0f);
        physics->uservector.vy =
            (int32_t)((float)normal_y * 5.0f);
        physics->uservector.vz =
            (int32_t)((float)normal_z * 5.0f);
        (void)animctrl_MotionNoLock(
            &player->playerRoot,
            &player->paMotions[4]);
        player->pMotionCallBack = NULL;
        memset(&physics->airmov, 0, sizeof(physics->airmov));
        memset(&physics->mov, 0, sizeof(physics->mov));
    }

    for (;;) {
        if (physics->movemode == MOVE_BLOWN) {
            if (physics->userdata[0] > 0x0000a000) {
                physics->movemode = MOVE_NORMAL;
                player->pFlags |=
                    (uint32_t)JPB_CALC_PLAYER_FLAG_AIRBORNE;
                physics->airTime = 0;
                physics->realAirTime = 0;
                player->pMotionCallBack =
                    jpb_TrajectoryCallbackSlot;
                continue;
            }

            if (physics->userdata[0] < 0x00002800) {
                float impulse_scale =
                    fGlobalFrameRate *
                    (1.0f / 4096.0f);

                physics->airmov.vx +=
                    (float)physics->uservector.vx *
                    impulse_scale;
                physics->airmov.vy +=
                    (float)physics->uservector.vy *
                    impulse_scale;
                physics->airmov.vz +=
                    (float)physics->uservector.vz *
                    impulse_scale;
                physics->mov = physics->airmov;
                player->pFlags |=
                    (uint32_t)JPB_CALC_PLAYER_FLAG_AIRBORNE;
            } else {
                physics->airmov.vx +=
                    globalgravity.vx * fGlobalFrameRate;
                physics->airmov.vy +=
                    globalgravity.vy * fGlobalFrameRate;
                physics->airmov.vz +=
                    globalgravity.vz * fGlobalFrameRate;
                if (physics->airmov.vy < -196.0f) {
                    physics->airmov.vy = -196.0f;
                }
                physics->mov = physics->airmov;
            }
            physics->userdata[0] =
                (int32_t)(
                    (uint32_t)physics->userdata[0] +
                    (uint32_t)flexmul(
                        gGlobalFrameRate, 0x00000200));
            break;
        }

        if (physics->movemode == MOVE_COREDEATH) {
            FVECTOR diff;
            int len;

            diff.vx =
                (float)physics->uservector.vx -
                physics->pos.vx;
            diff.vy = 0.0f;
            diff.vz =
                (float)physics->uservector.vz -
                physics->pos.vz;
            len = (int)VectorNormalize(&diff);
            if (len == 0) {
                physics->mov.vx = 0.0f;
                physics->mov.vz = 0.0f;
            } else {
                if (len > 12) {
                    len = 12;
                }
                physics->mov.vx =
                    (float)len * diff.vx;
                physics->mov.vz =
                    (float)len * diff.vz;
            }

            physics->userdata[0] =
                (int32_t)(
                    fGlobalFrameRate * 6.0f +
                    (float)physics->userdata[0]);
            physics->mov.vy =
                physics->userdata[0] > 96
                    ? 96.0f
                    : (float)physics->userdata[0];
            if (physics->userdata[0] > 200) {
                animObject *animation =
                    (animObject *)scene->pAnim;

                sound_StopSound(
                    animation->loopHandle[0]);
                sound_StopSound(
                    animation->loopHandle[1]);
                if ((player->pFlags &
                     (unsigned)
                         JPB_CALC_PLAYER_FLAG_MOVEMENT_LOCK) ==
                    0) {
                    player->pFlags |=
                        (uint32_t)
                            JPB_CALC_PLAYER_FLAG_MOVEMENT_LOCK;
                }
                (void)game_gModEnergy(
                    (int)player->playernum, -255);
                if (player->playernum < 2) {
                    (void)game_gSetGameFlags(
                        UINT32_C(0x00000020) <<
                        ((uint16_t)player->playernum &
                         31));
                    player_AfterLife(player);
                    afterLife = player;
                    obj_gSetObjectFlag(
                        &player->playerRoot,
                        0,
                        UINT32_C(0x00000020));
                } else {
                    player->pEnemy->exit_flag = 1;
                }
                player->whohitme = NULL;
                physics->userdata[0] = 0;
                physics->movemode = MOVE_NORMAL;
            }
            break;
        }

        if (physics->movemode == MOVE_HOVER) {
            float diff =
                physics->pos.vy - physics->airGround;
            float descent_limit =
                (jpb_CubeRuntimeFlags &
                 UINT32_C(0x00000008)) != 0
                    ? -64.0f
                    : -32.0f;

            if ((float)physics->userdata[0] + 90.0f >
                diff) {
                physics->airmov.vy += 4.0f;
                if (physics->airmov.vy > 32.0f) {
                    physics->airmov.vy = 32.0f;
                }
            } else if (
                diff >
                (float)physics->userdata[0] + 120.0f) {
                physics->airmov.vy +=
                    descent_limit * 0.125f;
                if (physics->airmov.vy <
                    descent_limit) {
                    physics->airmov.vy =
                        descent_limit;
                }
            } else if (physics->airmov.vy > 16.0f) {
                physics->airmov.vy -= 16.0f;
            } else if (physics->airmov.vy < -16.0f) {
                physics->airmov.vy += 16.0f;
            } else {
                physics->airmov.vy = 0.0f;
            }

            physics->mov.vx += physics->airmov.vx;
            physics->mov.vy += physics->airmov.vy;
            physics->mov.vz += physics->airmov.vz;

            vec_IdentMatrix(&rotation);
            fRotMatrixY(
                physics->angle.vy * 4 - 0x00002400,
                &rotation);
            if (physics->userdata[1] != 0) {
                int turn_contribution = 0;
                int turn_phase = physics->userdata[1];

                if (turn_phase <= 160000) {
                    if (turn_phase > 80000) {
                        turn_phase =
                            160000 - turn_phase;
                    }
                    turn_contribution =
                        turn_phase / 1024;
                }
                physics->mov.vz +=
                    (float)turn_contribution;
            }
            fApplyMatrixFV(
                &rotation,
                &physics->mov,
                &physics->mov);
            scale =
                (float)model->v3Scale.vx *
                (1.0f / 4096.0f);
            physics->mov.vx *= scale;
            physics->mov.vy *= scale;
            physics->mov.vz *= scale;
            break;
        }

        if (physics->movemode == MOVE_HOVER3D ||
            physics->movemode == MOVE_FLY) {
            wsl_ENEMY *enemy = player->pEnemy;

            if (enemy != NULL) {
                FVECTOR offset;

                offset.vx =
                    (float)enemy->destination.vx -
                    physics->pos.vx;
                offset.vy =
                    (float)enemy->destination.vy -
                    physics->pos.vy;
                offset.vz =
                    (float)enemy->destination.vz -
                    physics->pos.vz;
                (void)VectorNormalize(&offset);
                scale =
                    (float)model->v3Scale.vx *
                    physics->currentmov.vz *
                    (1.0f / 4096.0f);
                physics->mov.vx = offset.vx * scale;
                physics->mov.vy = offset.vy * scale;
                physics->mov.vz = offset.vz * scale;
            }
            break;
        }

        if ((player->pFlags &
             (unsigned)JPB_CALC_PLAYER_FLAG_AIRBORNE) != 0) {
            physics->mov.vy += physics->airmov.vy;
            if (physics->reversoi == 0) {
                physics->mov.vx += physics->airmov.vx;
                physics->mov.vz += physics->airmov.vz;
            } else {
                physics->mov.vx -= physics->airmov.vx;
                physics->mov.vz -= physics->airmov.vz;
                if ((uint32_t)physics->reversoi +
                        UINT32_C(0x0f00) <
                    (uint32_t)totalframes) {
                    physics->reversoi = 0;
                }
            }
            physics->airmov.vx +=
                globalgravity.vx * fGlobalFrameRate;
            physics->airmov.vy +=
                globalgravity.vy * fGlobalFrameRate;
            physics->airmov.vz +=
                globalgravity.vz * fGlobalFrameRate;
            if (physics->airmov.vy < -196.0f) {
                physics->airmov.vy = -196.0f;
            }
        }

        vec_IdentMatrix(&rotation);
        fRotMatrixY(physics->angle.vy, &rotation);
        fApplyMatrixFV(
            &rotation, &physics->mov, &rotated);
        scale =
            (float)model->v3Scale.vx *
            (1.0f / 4096.0f);
        physics->mov.vx = rotated.vx * scale;
        physics->mov.vy = rotated.vy * scale;
        physics->mov.vz = rotated.vz * scale;

        if ((player->pFlags &
             (unsigned)
                 JPB_CALC_PLAYER_FLAG_MOVEMENT_LOCK) != 0 &&
            (uint16_t)(player->playerID - 0x35) >= 4u) {
            physics->mov.vx = 0.0f;
            physics->mov.vz = 0.0f;
        }
        player->pFlags &=
            ~(uint32_t)JPB_CALC_PLAYER_FLAG_SLOPE_FORCE;

        if ((currentgmi1 &
             (unsigned)JPB_CALC_MAP_FLAG_SLOPE) != 0 &&
            (player->pFlags &
             (unsigned)
                 JPB_CALC_PLAYER_SLOPE_BLOCK_MASK) == 0 &&
            (physics->flags &
             UINT32_C(0x00100000)) == 0) {
            uint32_t normal_index =
                (uint32_t)*physics->currentmapinfo.poly &
                UINT32_C(0x1ffff);

            oldmov = physics->mov;
            physics_decode_map_normal(
                leveldata, normal_index, &polynormal);
            polynormal.vy = 0.0f;
            (void)VectorNormalize(&polynormal);
            physics->mov.vx =
                physics_merge_slope_component(
                    polynormal.vx * 26.0f, oldmov.vx);
            physics->mov.vy =
                physics_merge_slope_component(
                    polynormal.vy * 26.0f, oldmov.vy);
            physics->mov.vz =
                physics_merge_slope_component(
                    polynormal.vz * 26.0f, oldmov.vz);
            player->pFlags |=
                (uint32_t)
                    JPB_CALC_PLAYER_FLAG_SLOPE_FORCE;
            player->runCounter = 0;
        }
        break;
    }

    physics->mov.vx *= fGlobalFrameRate;
    physics->mov.vy *= fGlobalFrameRate;
    physics->mov.vz *= fGlobalFrameRate;
    player->pFlags &=
        ~(uint32_t)JPB_CALC_PLAYER_FLAG_LANDING;
    if (s != NULL) {
        old = physics->newlocalpos;
        CalcWorldRelativePos(s, physics);
        physics->mov.vx += physics->newlocalpos.vx - old.vx;
        physics->mov.vy += physics->newlocalpos.vy - old.vy;
        physics->mov.vz += physics->newlocalpos.vz - old.vz;
    } else if (
        physics->currentmapinfo.poly != NULL &&
        (currentgmi1 &
         (unsigned)JPB_CALC_MAP_FLAG_CONVEYOR_MASK) != 0 &&
        (player->pFlags &
         (unsigned)JPB_CALC_PLAYER_SLOPE_BLOCK_MASK) == 0) {
        float movespeed =
            (LevelSelect == 1 ? 21.0f : 12.0f) *
            fGlobalFrameRate;

        if ((currentgmi1 &
             (unsigned)JPB_CALC_MAP_FLAG_CONVEYOR_POS_Z) != 0) {
            physics->mov.vz += movespeed;
        }
        if ((currentgmi1 &
             (unsigned)JPB_CALC_MAP_FLAG_CONVEYOR_NEG_Z) != 0) {
            physics->mov.vz -= movespeed;
        }
        if ((currentgmi1 &
             (unsigned)JPB_CALC_MAP_FLAG_CONVEYOR_POS_X) != 0) {
            physics->mov.vx += movespeed;
        }
        if ((currentgmi1 &
             (unsigned)JPB_CALC_MAP_FLAG_CONVEYOR_NEG_X) != 0) {
            physics->mov.vx -= movespeed;
        }
    }
    return JPB_PHYSICS_PARTIAL_OK;
}

int jpb_PhysicsCalcMovementNormal(physicsObject *physics)
{
    if (physics != NULL &&
        physics->movemode != MOVE_NORMAL) {
        return JPB_PHYSICS_PARTIAL_UNSUPPORTED_STATE;
    }
    return jpb_PhysicsCalcMovement(physics);
}

/*
 * Exact no-contact position commit at MovePlayer RVA 0xDE29E plus its final
 * player-ground flag publication at RVAs 0xDE5D3..0xDE62A.
 */
int jpb_PhysicsMoveNoContact(physicsObject *physics)
{
    sceneObject *scene;
    playerObject *player;

    if (physics == NULL || physics->physicsRoot.pParent == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    scene = (sceneObject *)physics->physicsRoot.pParent;
    player = (playerObject *)scene->pPlayer;
    if (player == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    if (physics->movemode != MOVE_NORMAL ||
        physics->standee != NULL ||
        physics->solid != NULL) {
        return JPB_PHYSICS_PARTIAL_UNSUPPORTED_STATE;
    }

    physics->pos.vx += physics->mov.vx;
    physics->pos.vy += physics->mov.vy;
    physics->pos.vz += physics->mov.vz;
    if ((player->pFlags & 0x00000008u) == 0) {
        physics->flags &= 0xfffeffffu;
    } else {
        physics->hangcheck = 0;
        physics->flags |= 0x00010000u;
    }
    physics->flags &= 0xffffe7ffu;
    return JPB_PHYSICS_PARTIAL_OK;
}

/*
 * Exact per-object guard reset from ProcessPhysicsObjects RVA
 * 0xDE746..0xDE74C.
 */
void jpb_PhysicsBeginObjectFrame(physicsObject *physics)
{
    if (physics != NULL) {
        physics->flags &= 0xffd7bfffu;
    }
}

/* 0xDBCE0, 635 bytes, global, 6 named locals
 * CalcNewBox
 * PDB type: int (int, FVECTOR4*, FVECTOR4*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */

/* 0xDBF60, 182 bytes, global, 5 named locals
 * CalcRelativePosFromWorld
 * PDB type: void (_solid*, FVECTOR*, FVECTOR...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int CalcNewBox(int h1, FVECTOR4 *frustplane, FVECTOR4 *box)
{
    float s;
    float pv;
    FVECTOR horizontal;
    int index;

    p.vx = gpWorld->start.vx;
    p.vy = h1;
    p.vz = gpWorld->start.vz;
    for (index = 0; index < 4; ++index) {
        horizontal.vx = frustplane[index].vx;
        horizontal.vy = 0.0f;
        horizontal.vz = frustplane[index].vz;
        s = VectorNormalize(&horizontal);
        box[index].vx = horizontal.vx;
        box[index].vy = horizontal.vy;
        box[index].vz = horizontal.vz;
        if (s > 0.0625f) {
            pv =
                (float)p.vx * frustplane[index].vx +
                (float)p.vy * frustplane[index].vy +
                (float)p.vz * frustplane[index].vz -
                frustplane[index].vw;
            box[index].vw =
                (float)p.vx * box[index].vx +
                (float)p.vz * box[index].vz -
                pv / s;
        } else {
            box[index].vw = 65536.0f;
        }
    }
    return 16;
}
void CalcRelativePosFromWorld(
    _solid *s, FVECTOR *world, FVECTOR *relative)
{
    FVECTOR knob;
    MATRIX m;

    InvertMatrix(&s->rotmatrix, &m);
    fScaleMatrix(&m, &s->scale);
    m.t[0] = 0;
    m.t[1] = 0;
    m.t[2] = 0;
    knob.vx = world->vx - (float)s->modelnode->v3RotCenter.vx;
    knob.vy = world->vy - (float)s->modelnode->v3RotCenter.vy;
    knob.vz = world->vz - (float)s->modelnode->v3RotCenter.vz;
    fApplyMatrixFV(&m, &knob, relative);
}

/* 0xDC020, 287 bytes, global, 5 named locals
 * CalcSolidRelativePos
 * PDB type: void (_solid*, physicsObject*, F...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void CalcSolidRelativePos(
    _solid *s, physicsObject *physics, FVECTOR *pos)
{
    FVECTOR knob;
    MATRIX m;

    if ((physics->flags & 0x00011800u) != 0) {
        InvertMatrix(&s->rotmatrix, &m);
        fScaleMatrix(&m, &s->scale);
        m.t[0] = 0;
        m.t[1] = 0;
        m.t[2] = 0;
        knob.vx = pos->vx - (float)s->modelnode->v3RotCenter.vx;
        knob.vy = pos->vy - (float)s->modelnode->v3RotCenter.vy;
        knob.vz = pos->vz - (float)s->modelnode->v3RotCenter.vz;
        fApplyMatrixFV(&m, &knob, &physics->localpos);
        physics->localfacing.vx =
            (float)(physics->angle.vx - s->physics->angle.vx);
        physics->localfacing.vy =
            (float)(physics->angle.vy - s->physics->angle.vy);
        physics->localfacing.vz =
            (float)(physics->angle.vz - s->physics->angle.vz);
        physics->flags &= 0xffffe7ffu;
    }
}

/* 0xDC140, 174 bytes, global, 4 named locals
 * CalcWorldPosFromRelative
 * PDB type: void (_solid*, FVECTOR*, FVECTOR...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void CalcWorldPosFromRelative(
    _solid *s, FVECTOR *relative, FVECTOR *world)
{
    MATRIX knob = s->rotmatrix;

    fScaleMatrix(&knob, &s->scale);
    fApplyMatrixFV(&knob, relative, world);
    world->vx += (float)s->modelnode->v3RotCenter.vx;
    world->vy += (float)s->modelnode->v3RotCenter.vy;
    world->vz += (float)s->modelnode->v3RotCenter.vz;
}

/* 0xDC1F0, 216 bytes, local, 3 named locals
 * CalcWorldRelativePos
 * PDB type: void (_solid*, physicsObject*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static void CalcWorldRelativePos(_solid *s, physicsObject *p)
{
    MATRIX knob = s->rotmatrix;

    fScaleMatrix(&knob, &s->scale);
    fApplyMatrixFV(&knob, &p->localpos, &p->newlocalpos);
    p->newlocalpos.vx += (float)s->modelnode->v3RotCenter.vx;
    p->newlocalpos.vy += (float)s->modelnode->v3RotCenter.vy;
    p->newlocalpos.vz += (float)s->modelnode->v3RotCenter.vz;
}

/* 0xDC2D0, 791 bytes, local, 23 named locals
 * CharBlocking
 * PDB type: void (playerObject*, physicsObje...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static void CharBlocking(
    playerObject *player,
    physicsObject *p0,
    physicsObject *p1,
    FVECTOR *pos0,
    FVECTOR *testpos0,
    FVECTOR *dir0,
    float dist0,
    float *range)
{
    const float max_push = 48.0f;
    sceneObject *scene1 =
        (sceneObject *)p1->physicsRoot.pParent;
    playerObject *player1 =
        (playerObject *)scene1->pPlayer;
    FVECTOR angle;
    float totalradius;
    FVECTOR testpos1;
    FVECTOR newangle = {0.0f, 0.0f, 0.0f};
    float distween;
    float top1;
    float bottom0;
    float top0;
    float flatdist;
    float extra;
    float totalmass;
    float vel0;
    FVECTOR push0;
    FVECTOR push1;
    float vel1;

    (void)pos0;
    (void)dir0;
    (void)dist0;
    testpos1.vx = p1->pos.vx + p1->mov.vx;
    testpos1.vy = p1->pos.vy + p1->mov.vy;
    testpos1.vz = p1->pos.vz + p1->mov.vz;
    angle.vx = testpos0->vx - testpos1.vx;
    angle.vy = testpos0->vy - testpos1.vy;
    angle.vz = testpos0->vz - testpos1.vz;
    totalradius = (float)(
        (int)p0->radius + (int)p1->radius);
    distween = VectorNormalize2(&angle, &newangle);

    top1 = testpos1.vy + (float)(int)p1->height;
    bottom0 = testpos0->vy;
    top0 = bottom0 + (float)(int)p0->height;
    if (!(testpos1.vy > top0) &&
        !(bottom0 > top1)) {
        flatdist = VectorNormalize3(
            angle.vx, 0.0f, angle.vz, &newangle);
        if (flatdist < totalradius) {
            extra = totalradius - flatdist;
            if (extra > 0.0f &&
                p1->radius != 0 &&
                (p1->flags & UINT32_C(0x20)) == 0) {
                totalmass = (float)(
                    (int)p0->mass + (int)p1->mass);
                if (totalmass < 1.0f) {
                    totalmass = 1.0f;
                }
                if (p0->mass == INT16_MAX ||
                    (player->pFlags &
                     UINT32_C(0x40000000)) != 0) {
                    vel0 = 0.0f;
                    vel1 = extra;
                } else if (
                    p1->mass == INT16_MAX ||
                    (player1->pFlags &
                     UINT32_C(0x40000000)) != 0) {
                    vel0 = extra;
                    vel1 = 0.0f;
                } else {
                    vel0 =
                        (float)(int)p1->mass *
                        extra / totalmass;
                    vel1 =
                        (float)(int)p0->mass *
                        extra / totalmass;
                }
                if (vel0 < 0.0f) {
                    vel0 = 0.0f;
                }
                if (vel1 < 0.0f) {
                    vel1 = 0.0f;
                }
                if (vel0 > max_push) {
                    vel0 = max_push;
                }
                if (vel1 > max_push) {
                    vel1 = max_push;
                }
                push0.vx = newangle.vx * vel0;
                push0.vy = newangle.vy * vel0;
                push0.vz = newangle.vz * vel0;
                push1.vx = newangle.vx * vel1;
                push1.vy = newangle.vy * vel1;
                push1.vz = newangle.vz * vel1;

                p1->mov.vx -= push1.vx;
                p1->mov.vz -= push1.vz;
                p0->mov.vx += push0.vx;
                p0->mov.vz += push0.vz;
                if ((p1->flags &
                     UINT32_C(0x2000)) != 0) {
                    p1->mov.vy -= push1.vy;
                }
                if ((p0->flags &
                     UINT32_C(0x2000)) != 0) {
                    p0->mov.vy -= push0.vy;
                }
                p0->flags |= UINT32_C(0x80800);
                p1->flags |= UINT32_C(0x80800);
            }
        }
    }
    *range = distween;
}

int jpb_PhysicsCharBlockingState(
    playerObject *player,
    physicsObject *p0,
    physicsObject *p1,
    FVECTOR *testpos0,
    float *range)
{
    sceneObject *scene1;

    if (player == NULL ||
        p0 == NULL ||
        p1 == NULL ||
        testpos0 == NULL ||
        range == NULL ||
        p1->physicsRoot.pParent == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    scene1 =
        (sceneObject *)p1->physicsRoot.pParent;
    if (scene1->pPlayer == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    CharBlocking(
        player,
        p0,
        p1,
        NULL,
        testpos0,
        NULL,
        0.0f,
        range);
    return JPB_PHYSICS_PARTIAL_OK;
}

/*
 * Bounded extraction of MovePlayer's character-contact sweep at RVAs
 * 0xDDC64..0xDDF3D, including the exact desert-beast/worm node-contact branch
 * at RVAs 0xDDD43..0xDDEE5. This descriptive facade is not an original PDB
 * symbol.
 */
int jpb_PhysicsMoveCharacterContacts(physicsObject *p0)
{
    sceneObject *scene0;
    playerObject *player;
    FVECTOR testpos0;
    FVECTOR dir0 = {0.0f, 0.0f, 0.0f};
    float dist0;
    int object_id;
    int index;

    if (p0 == NULL ||
        p0->physicsRoot.pParent == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    object_id = p0->physicsRoot.objectID;
    if ((uint32_t)object_id >= JPB_PHYSICS_CAPACITY) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    scene0 =
        (sceneObject *)p0->physicsRoot.pParent;
    player = (playerObject *)scene0->pPlayer;
    if (player == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    if ((player->pFlags &
         UINT32_C(0x44000000)) != 0) {
        return JPB_PHYSICS_PARTIAL_OK;
    }

    /* Validate authentic component graphs before publishing any pair state. */
    for (index = object_id + 1;
         index < JPB_PHYSICS_CAPACITY;
         ++index) {
        physicsObject *p1 = &maPhysicsData[index];
        sceneObject *scene1;
        playerObject *player1;

        if (p1->physicsRoot.objectID == -1) {
            continue;
        }
        if (p1->physicsRoot.pParent == NULL) {
            return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
        }
        scene1 =
            (sceneObject *)p1->physicsRoot.pParent;
        player1 = (playerObject *)scene1->pPlayer;
        if (scene1->pScene == NULL ||
            player1 == NULL) {
            return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
        }
    }

    testpos0.vx = p0->pos.vx + p0->mov.vx;
    testpos0.vy = p0->pos.vy + p0->mov.vy;
    testpos0.vz = p0->pos.vz + p0->mov.vz;
    dist0 = VectorNormalize2(&p0->mov, &dir0);
    for (index = object_id + 1;
         index < JPB_PHYSICS_CAPACITY;
         ++index) {
        physicsObject *p1 = &maPhysicsData[index];
        sceneObject *scene1;
        playerObject *player1;

        maRange[object_id][index] = -1.0f;
        if (p1->physicsRoot.objectID == -1) {
            continue;
        }
        scene1 =
            (sceneObject *)p1->physicsRoot.pParent;
        player1 = (playerObject *)scene1->pPlayer;
        if (obj_gCheckObjectFlag(
                &p1->physicsRoot,
                0,
                UINT32_C(0x20)) == 0 &&
            (player1->pFlags &
             UINT32_C(0x0a00)) == 0) {
            if ((p1->flags &
                 UINT32_C(0x00800000)) != 0 &&
                object_id <= 1) {
                CollisionData *c;
                int nodes;

                if (player1->playerID ==
                    JPB_PLAYER_ID_DESERT_BEAST) {
                    c = maDesert_BNodeSizes;
                    nodes = 4;
                } else if (player1->playerID ==
                           JPB_PLAYER_ID_WORM) {
                    c = maWormNodeSizes;
                    nodes = 6;
                } else {
                    continue;
                }

                /*
                 * The reference emits a diagnostic string here. Logging is
                 * intentionally left at the platform boundary; it does not
                 * affect collision state.
                 */
                while (nodes != 0) {
                    float radius = (float)c->radius1;
                    float totalradius =
                        (float)p0->radius + radius;
                    VECTOR *pos =
                        coll_GetNodeCenter(index, c->id);

                    if (coll_ChkNodeFlags(index, c->id, 1) == 0 &&
                        pos != NULL) {
                        FVECTOR diff;

                        diff.vx = p0->pos.vx - (float)pos->vx;
                        diff.vy = p0->pos.vy - (float)pos->vy;
                        diff.vz = p0->pos.vz - (float)pos->vz;
                        if (radius >
                            (float)physics_abs_truncated_float(
                                diff.vy)) {
                            float len;

                            diff.vy = 0.0f;
                            len = VectorNormalize(&diff);
                            if (totalradius > len) {
                                float amount = totalradius - len;

                                p0->flags |= UINT32_C(0x80800);
                                p0->mov.vx += diff.vx * amount;
                                p0->mov.vy += diff.vy * amount;
                                p0->mov.vz += diff.vz * amount;
                            }
                        }
                    }
                    ++c;
                    --nodes;
                }
            } else if (((p1->flags | p0->flags) &
                        UINT32_C(0x0480)) == 0) {
                CharBlocking(
                    player,
                    p0,
                    p1,
                    &p0->pos,
                    &testpos0,
                    &dir0,
                    dist0,
                    &maRange[object_id][index]);
            }
        }
    }
    return JPB_PHYSICS_PARTIAL_OK;
}

/* 0xDC5F0, 4566 bytes, local, 56 named locals
 * CheckCubeBlocking
 * PDB type: int (playerObject*, FVECTOR*, FV...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static int CheckCubeBlocking(
    playerObject *player,
    FVECTOR *world,
    FVECTOR *dir,
    FVECTOR *dirNormal,
    float dist,
    float *ground)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *p0 = (physicsObject *)scene->pPhysics;
    FVECTOR org = *world;
    FVECTOR move = *dir;
    FVECTOR initial_from = {
        org.vx - move.vx,
        org.vy - move.vy,
        org.vz - move.vz
    };
    FVECTOR from = initial_from;
    FVECTOR to;
    FVECTOR endpos;
    FVECTOR slidenorm;
    FVECTOR ledgepos;
    int32_t *cubehit = NULL;
    int32_t *entryhit = NULL;
    int32_t *polyhit = NULL;
    float vel = VectorNormalize(&move);
    float yoffset;
    int bigblock = 0;
    int numcollides = 0;
    int nonmoves = 0;

    (void)dist;
    p0->hangcheck = 0;
    bestinfo.type = 0;
    bestinfo.washack = 0;
    solidhack = p0->physicsRoot.objectID < 2;

    yoffset =
        (float)p0->height *
        (((player->pFlags & UINT32_C(1)) != 0) ? 0.25f : 0.5f);
    org.vy += yoffset;
    from.vy += yoffset;
    {
        int tground = intersec_FindWalkHeightFV(
            &org, NULL, &player->playerRoot, 0);

        if (p0->movemode == MOVE_NORMAL && tground == -32768) {
            dir->vx = 0.0f;
            dir->vz = 0.0f;
            *ground = p0->airGround;
            return 1;
        }
    }
    org.vy += yoffset;

    if (p0->physicsRoot.objectID < 2 || vel != 0.0f) {
        do {
            float tmpdist;
            float tslide;
            float slide1;
            float len;
            int collidetype;

            bestinfo.type = 0;
            bestinfo.flags = 0;
            whichsolid = NULL;
            to.vx = from.vx + move.vx * vel;
            to.vy = from.vy + move.vy * vel;
            to.vz = from.vz + move.vz * vel;
            bestinfo.dist = vel;
            p0->lastpolyhit = NULL;
            if (newclosestPoly(
                    &from,
                    &to,
                    &move,
                    vel,
                    (float)p0->radius,
                    &move,
                    p0->physicsRoot.objectID,
                    &cubehit,
                    &entryhit,
                    &polyhit) == 0) {
                break;
            }

            collidetype = bestinfo.type & 3;
            if (collidetype == 2) {
                p0->airTime = 0;
            }
            p0->lastpolyhit = polyhit;
            if (bestinfo.dist <= 4.0f &&
                (bestinfo.flags & 4) == 0) {
                ++nonmoves;
            }
            if (entryhit != NULL &&
                p0->physicsRoot.objectID < 2) {
                physics_launch_map_contact(
                    entryhit, 4, &bestinfo.kisspoint);
            }
            if ((p0->flags & UINT32_C(0x40000)) != 0) {
                physics_launch_fixed_effect(24, &bestinfo.kisspoint);
            }
            ++numcollides;

            if ((bestinfo.flags & 4) == 0) {
                FVECTOR tmp = move;

                tmpdist = bestinfo.dist - 1.0f;
                if (tmpdist < 0.0f) {
                    tmpdist = 0.0f;
                }
                endpos.vx = from.vx + tmp.vx * tmpdist;
                endpos.vy = from.vy + tmp.vy * tmpdist;
                endpos.vz = from.vz + tmp.vz * tmpdist;
            } else {
                tmpdist = bestinfo.dist + 4.0f;
                if (tmpdist < 1.0f) {
                    tmpdist = 1.0f;
                } else if (tmpdist > 32.0f) {
                    tmpdist = 32.0f;
                }
                endpos.vx = from.vx + bestinfo.n.vx * tmpdist;
                endpos.vy = from.vy + bestinfo.n.vy * tmpdist;
                endpos.vz = from.vz + bestinfo.n.vz * tmpdist;
            }

            if (collidetype == 1) {
                slidenorm = bestinfo.facenormal;
            } else {
                slidenorm.vx =
                    endpos.vx - bestinfo.kisspoint.vx;
                slidenorm.vy =
                    endpos.vy - bestinfo.kisspoint.vy;
                slidenorm.vz =
                    endpos.vz - bestinfo.kisspoint.vz;
            }
            from = endpos;
            org = endpos;

            if ((player->pFlags & UINT32_C(1)) == 0 &&
                (p0->flags & UINT32_C(0x2000)) == 0 &&
                p0->movemode != MOVE_HOVER) {
                tmpnorm.vx = dirNormal->vx;
                tmpnorm.vy = 0.0f;
                tmpnorm.vz = dirNormal->vz;
                VectorNormalize(&slidenorm);
                VectorNormalize(&tmpnorm);
                tslide =
                    tmpnorm.vx * slidenorm.vx +
                    tmpnorm.vy * slidenorm.vy +
                    tmpnorm.vz * slidenorm.vz -
                    0.002f;
                slide1 =
                    move.vx * slidenorm.vx +
                    move.vy * slidenorm.vy +
                    move.vz * slidenorm.vz;
                move.vx = tmpnorm.vx - tslide * slidenorm.vx;
                move.vy = tmpnorm.vy - tslide * slidenorm.vy;
                move.vz = tmpnorm.vz - tslide * slidenorm.vz;
            } else {
                VectorNormalize(&slidenorm);
                slide1 =
                    move.vx * slidenorm.vx +
                    move.vy * slidenorm.vy +
                    move.vz * slidenorm.vz;
                tslide = slide1 - 0.008f;
                move.vx -= tslide * slidenorm.vx;
                move.vy -= tslide * slidenorm.vy;
                move.vz -= tslide * slidenorm.vz;
            }
            len = VectorNormalize(&move);

            if ((player->hitNumber == 0 && whichsolid != NULL) ||
                (entryhit != NULL &&
                 polyhit != NULL &&
                 p0->hangcheck == 0 &&
                 collidetype == 2 &&
                 p0->mov.vy < 0.0f &&
                 (p0->flags & UINT32_C(0x100080)) == 0 &&
                 bestinfo.facenormal.vy > 0.732421875f &&
                 (physics_map_flags(polyhit) &
                  UINT32_C(0x01e88000)) == 0 &&
                 extracharacter_CanLedgeClimb(
                     (model_id)player->playerID))) {
                float ledgelength;

                tmpmove.vx = edge_start.vx - edge_end.vx;
                tmpmove.vy = 0.0f;
                tmpmove.vz = edge_start.vz - edge_end.vz;
                ledgelength = VectorNormalize(&tmpmove);
                if (ledgelength > 16.0f &&
                    fabsf(edge_start.vy - edge_end.vy) /
                            ledgelength <
                        0.125f) {
                    ledgeoff.vx =
                        tmpmove.vz * bestinfo.facenormal.vy;
                    ledgeoff.vy =
                        tmpmove.vx * bestinfo.facenormal.vz -
                        tmpmove.vz * bestinfo.facenormal.vx;
                    ledgeoff.vz =
                        -tmpmove.vx * bestinfo.facenormal.vy;
                    p0->flags |= UINT32_C(0x100000);
                    p0->ledgeangle =
                        (ratan2(
                             (int32_t)(ledgeoff.vx * 4096.0f),
                             (int32_t)(ledgeoff.vz * 4096.0f)) -
                         0x800) &
                        0x0fff;
                    p0->ledgepoint = bestinfo.kisspoint;
                    memset(
                        &p0->currentmapinfo,
                        0,
                        sizeof(p0->currentmapinfo));
                    if ((bestinfo.flags & 0x60000) == 0 ||
                        whichsolid == NULL ||
                        (whichsolid->flags & 1) == 0) {
                        p0->solidgrabbed = NULL;
                        p0->hangcheck = 1;
                    } else {
                        MATRIX m;
                        FVECTOR knob;

                        p0->solidgrabbed = whichsolid->physics;
                        InvertMatrix(&whichsolid->rotmatrix, &m);
                        fScaleMatrix(&m, &whichsolid->scale);
                        knob.vx =
                            p0->ledgepoint.vx -
                            (float)whichsolid->modelnode
                                ->v3RotCenter.vx;
                        knob.vy =
                            p0->ledgepoint.vy -
                            (float)whichsolid->modelnode
                                ->v3RotCenter.vy;
                        knob.vz =
                            p0->ledgepoint.vz -
                            (float)whichsolid->modelnode
                                ->v3RotCenter.vz;
                        fApplyMatrixFV(&m, &knob, &p0->ledgepoint);
                        p0->hangcheck = 1;
                    }
                }
            }

            (void)physics_try_start_streets_ending(
                p0, player, collidetype);

            if (fabs((double)(slide1 + 1.0f)) <=
                0.0010000000474974513) {
                bigblock = 1;
                break;
            }
            vel = (vel - bestinfo.dist) * len;
            if (vel <= 0.0f) {
                break;
            }
            if ((bestinfo.flags & 4) == 0 && nonmoves == 2) {
                if ((player->pFlags & UINT32_C(1)) == 0) {
                    break;
                }
                if (p0->mov.vy < 0.0f && p0->airstick < 0x7f) {
                    p0->airstick =
                        (uint8_t)(p0->airstick + 2);
                }
            } else if (
                (bestinfo.flags & 4) != 0 &&
                p0->mov.vy < 0.0f &&
                p0->airstick < 0x7f) {
                p0->airstick = (uint8_t)(p0->airstick + 2);
            }
        } while (numcollides < 4);
    }

    if (p0->airstick != 0) {
        --p0->airstick;
    }
    if ((player->pFlags & UINT32_C(1)) == 0) {
        p0->flags &= ~UINT32_C(0x100000);
        player->pFlags &= ~UINT32_C(0x04000000);
    } else if (
        (p0->flags & UINT32_C(0x100000)) != 0 &&
        p0->physicsRoot.objectID <= 1) {
        FVECTOR *ledger = NULL;

        if (p0->solidgrabbed == NULL) {
            ledger = &p0->ledgepoint;
        } else if (p0->solidgrabbed->solid != NULL) {
            CalcWorldPosFromRelative(
                p0->solidgrabbed->solid,
                &p0->ledgepoint,
                &ledgepos);
            ledger = &ledgepos;
        }
        if (ledger != NULL) {
            float ledgediff = ledger->vy - org.vy;
            int angle =
                (p0->face.vy - p0->ledgeangle) *
                0x100000 >> 20;

            if (ledgediff > 8.0f &&
                fabs((double)ledgediff) <
                    (double)fGlobalFrameRate * 128.0 &&
                (angle < 0 ? -angle : angle) < 0x21d) {
                p0->face.vy = p0->ledgeangle;
                player->pFlags |= UINT32_C(0x04000000);
                p0->flags &= ~UINT32_C(0x100000);
                p0->hangcheck = 0;
                if (player->shadow != NULL) {
                    sprite_gHideSprite(
                        (Sprite *)(void *)player->shadow);
                }
                p0->airGround = ledger->vy;
                if (p0->movemode == MOVE_NORMAL) {
                    p0->mov.vy = 0.0f;
                }
                return 1;
            }
        }
    }

    org.vy -= yoffset;
    if (numcollides == 0) {
        ++p0->noncollideframes;
        if (p0->noncollideframes > 1) {
            p0->flags &= ~UINT32_C(0x100000);
            player->pFlags &= ~UINT32_C(0x04000000);
        }
        p0->collidetime = 0;
        p0->anycollidetime = 0;
    } else {
        dir->vx = org.vx - initial_from.vx;
        dir->vy = org.vy - initial_from.vy;
        dir->vz = org.vz - initial_from.vz;
        p0->noncollideframes = 0;
        if (bigblock == 0) {
            p0->collidetime = 0;
        } else {
            p0->collidetime += (uint32_t)gGlobalFrameRate;
        }
        if (LevelSelect != 8 || totalframes > 0x40) {
            p0->anycollidetime += (uint32_t)gGlobalFrameRate;
        }
        if (p0->physicsRoot.objectID < 2 &&
            (player->pFlags & UINT32_C(1)) != 0 &&
            p0->anycollidetime > UINT32_C(0x5a000) &&
            p0->falltimer > 0x5a000 &&
            p0->mov.vy < 0.0f) {
            player->pFlags &= UINT32_C(0xbbfff3f6);
            hurtplayer(player, -0xff);
        }
    }

    *ground = (float)intersec_FindWalkHeightFV(
        &org, NULL, &player->playerRoot, 0);
    if (p0->mapinfo.entry != NULL &&
        p0->physicsRoot.objectID < 2) {
        physics_launch_map_contact(p0->mapinfo.entry, 8, &org);
    }
    if (p0->movemode != MOVE_NORMAL ||
        (*ground > -32768.0f &&
         (((player->pFlags & UINT32_C(1)) != 0) ||
          p0->pos.vy - *ground <= (float)p0->maxledge))) {
        *world = org;
        if (*ground > -32760.0f) {
            float threshold =
                (player->pFlags & UINT32_C(0x10000000)) != 0
                    ? 0.25f
                    : 0.125f;

            if ((((player->pFlags & UINT32_C(1)) == 0) &&
                 (p0->flags & UINT32_C(0x2000)) == 0 &&
                 fabs((double)(org.vy - *ground)) <
                     (double)fGlobalFrameRate * 256.0 *
                         (double)threshold) ||
                (p0->mapinfo.poly != NULL &&
                 (physics_map_flags(p0->mapinfo.poly) &
                  UINT32_C(0x14000)) != 0)) {
                world->vy = *ground;
            }
        }
    } else {
        dir->vx = 0.0f;
        dir->vz = 0.0f;
        *ground = p0->airGround;
        *world = initial_from;
        numcollides = 1;
    }
    return numcollides;
}

int jpb_PhysicsCheckCubeBlocking(
    playerObject *player,
    FVECTOR *world,
    FVECTOR *dir,
    FVECTOR *dirNormal,
    float dist,
    float *ground)
{
    if (player == NULL ||
        player->playerRoot.pParent == NULL ||
        world == NULL ||
        dir == NULL ||
        dirNormal == NULL ||
        ground == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    return CheckCubeBlocking(
        player, world, dir, dirNormal, dist, ground);
}

/* 0xDD7D0, 85 bytes, global, 2 named locals
 * DebugPlayer
 * PDB type: void (physicsObject*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */

/* 0xDD830, 428 bytes, global, 20 named locals
 * FindBestMachineGunTarget
 * PDB type: playerObject* (VECTOR*, VECTOR*,...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static int physics_machinegun_abs(int value)
{
    return value < 0
        ? (int)(UINT32_C(0) - (uint32_t)value)
        : value;
}

static int physics_machinegun_signed_angle(int value)
{
    uint32_t wrapped = (uint32_t)value & UINT32_C(0x0fff);

    return (wrapped & UINT32_C(0x0800)) != 0
        ? (int)wrapped - 0x1000
        : (int)wrapped;
}

playerObject *FindBestMachineGunTarget(
    VECTOR *pos,
    VECTOR *angle,
    playerObject *tank,
    int range,
    int maxangle,
    int maxheightdiff,
    int jedi)
{
    playerObject *targ = NULL;
    int bestdist = INT_MAX;
    physicsObject *ptank = (physicsObject *)(
        (sceneObject *)tank->playerRoot.pParent)->pPhysics;
    int first = jedi != 0 ? 0 : 2;
    int last = jedi != 0 ? 2 : JPB_PLAYER_CAPACITY;
    int i;

    for (i = first; i < last; ++i) {
        playerObject *pl = &gaPlayerData[i];

        if (pl != tank && pl->playerRoot.objectID != -1 &&
            obj_gCheckObjectFlag(&pl->playerRoot, 0, 0x20) == 0 &&
            (jedi != 0 ||
             (pl->pEnemy != NULL && pl->pEnemy->pPlace != NULL &&
              pl->pEnemy->pPlace->aiDf.daDelay == 2))) {
            int dist = physics_gGetRange(
                &pl->playerRoot, &tank->playerRoot);

            if (dist < range * 0x100 && game_gGetEnergy(i) != 0) {
                physicsObject *p2 = (physicsObject *)(
                    (sceneObject *)pl->playerRoot.pParent)->pPhysics;
                int yd = (int)(p2->pos.vy - (float)pos->vy);

                if (physics_machinegun_abs(yd) < maxheightdiff) {
                    int xd = (int)(p2->pos.vx - (float)pos->vx);
                    int zd = (int)(p2->pos.vz - (float)pos->vz);
                    int a = ratan2(-xd, zd);
                    int angle_delta = physics_machinegun_signed_angle(
                        0x1000 - a - ptank->angle.vy);

                    if (physics_machinegun_abs(angle_delta) < maxangle) {
                        dist = physics_gGetRange(
                            &tank->playerRoot, &pl->playerRoot);
                    }
                    if (physics_machinegun_abs(angle_delta) < maxangle &&
                        dist > 0x10 && dist < bestdist) {
                        targ = pl;
                        bestdist = dist;
                    }
                }
            }
        }
    }
    (void)angle;
    return targ;
}

/* 0xDD9E0, 450 bytes, global, 10 named locals
 * LaunchMapAnimEffects
 * PDB type: void (int, VECTOR*, int*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */

/* 0xDDBB0, 2713 bytes, global, 30 named locals
 * MovePlayer
 * PDB type: void (physicsObject*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static int physics_move_signed_angle_delta(
    int new_angle, int old_angle)
{
    uint32_t wrapped =
        ((uint32_t)new_angle - (uint32_t)old_angle) &
        UINT32_C(0x0fff);

    return
        (wrapped & UINT32_C(0x0800)) != 0
            ? (int)wrapped - 0x1000
            : (int)wrapped;
}

static int physics_move_arithmetic_shift_right(
    int value, unsigned shift)
{
    if (value >= 0) {
        return value >> shift;
    }
    return -(int)(((~(uint32_t)value) >> shift) + 1U);
}

/*
 * Instruction-reviewed MovePlayer tail at RVAs 0xDE4B2..0xDE612. This
 * inferred decomposition keeps map-contact dispatch, moving-platform
 * coordinates, and landing-state publication in their original order.
 */
static void physics_move_postprocess(
    physicsObject *p0, playerObject *player)
{
    if (p0->physicsRoot.objectID < 2 &&
        p0->mapinfo.entry != NULL &&
        (player->pFlags & UINT32_C(0x00000008)) != 0) {
        FVECTOR position = {
            p0->pos.vx,
            p0->validairground,
            p0->pos.vz
        };

        physics_launch_map_contact(
            p0->mapinfo.entry, 12, &position);
    }

    if (p0->standee != NULL) {
        if (p0->standee->solid == NULL) {
            p0->standee = NULL;
        } else {
            CalcSolidRelativePos(
                p0->standee->solid, p0, &p0->pos);
            CalcWorldRelativePos(p0->standee->solid, p0);
        }
    }

    if ((player->pFlags & UINT32_C(0x00000008)) == 0) {
        p0->flags &= ~UINT32_C(0x00010000);
    } else {
        p0->hangcheck = 0;
        p0->flags |= UINT32_C(0x00010000);
    }
}

/*
 * Exact ordinary-mode fall transition at RVAs 0xDDF8C..0xDE29E. Returns one
 * only when motion 4 was accepted and the airborne state was published.
 */
static int physics_move_start_fall(
    physicsObject *p0, playerObject *player)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *p =
        (physicsObject *)scene->pPhysics;
    int attempt_fall;

    if ((player->pFlags & UINT32_C(0x00000009)) != 0) {
        attempt_fall =
            p->pos.vy - p->airGround +
                    (float)(p->height / 4) >
                32.0f;
    } else if (
        p->mapinfo.poly != NULL &&
        (physics_map_flags(p->mapinfo.poly) &
         UINT32_C(0x00020000)) != 0) {
        attempt_fall = 1;
    } else {
        attempt_fall =
            p->pos.vy - p->airGround > 32.0f;
    }
    if (!attempt_fall ||
        animctrl_MotionNoLock(
            &player->playerRoot,
            &player->paMotions[4]) == 0) {
        return 0;
    }

    p0->userdata[0] = (int32_t)gGlobalTimer;
    p0->standee = NULL;
    player->pFlags &= ~UINT32_C(0x00000008);
    player->paMotions[4].motionFlags |= UINT32_C(0x04000000);

    if ((player->pFlags & UINT32_C(0x88000000)) == 0) {
        int move_angle = ratan2(
            physics_trunc_float_to_i32(p0->mov.vx),
            physics_trunc_float_to_i32(p0->mov.vz));
        int delta = physics_move_signed_angle_delta(
            move_angle, p0->angle.vy);
        int sub;

        brain_SetFallTrajectory(player, 0);
        sub = delta < 0 ? -delta : delta;
        if (sub < 0x401) {
            int reverse_sub = sub - 0x400;

            if (reverse_sub < 0) {
                reverse_sub = -reverse_sub;
            }
            if (reverse_sub < 0x40) {
                p0->angle.vy +=
                    physics_move_arithmetic_shift_right(delta, 6);
            }
        } else {
            p0->reversoi = (int32_t)gGlobalTimer;
        }
    } else {
        int use_full_jump =
            (player->pFlags & UINT32_C(0x08000000)) == 0 ||
            player->target == NULL;

        if (!use_full_jump) {
            sceneObject *target_scene =
                (sceneObject *)player->target->playerRoot.pParent;
            physicsObject *target_physics =
                target_scene != NULL
                    ? (physicsObject *)target_scene->pPhysics
                    : NULL;

            use_full_jump =
                target_physics == NULL ||
                target_physics->pos.vy - p->pos.vy >= 256.0f;
        }
        player->airVelocity =
            use_full_jump
                ? (int)player->pSettings.JumpVel
                : (int)(
                      (float)player->pSettings.JumpVel * 0.5f);
        player->airAngle =
            (int)player->pSettings.dblJumpAngle;
    }

    if ((player->playerID == 9 || player->playerID == 0x2b) &&
        LevelSelect == 10) {
        player->pMotionCallBack =
            jpb_MaulTrajectoryCallbackSlot;
    } else {
        player->pMotionCallBack =
            jpb_TrajectoryCallbackSlot;
    }
    brain_SetTrajectory(
        player, player->airVelocity, player->airAngle);

    if (player->playerRoot.objectID != -1 &&
        obj_gCheckObjectFlag(
            &player->playerRoot, 0, UINT32_C(0x20)) == 0 &&
        (player->pFlags & UINT32_C(0x00002000)) == 0) {
        modelObject *model =
            (modelObject *)scene->pModel;
        VECTOR *pelvis;

        scene_gGetSceneModelMatrixFV(
            p->physicsRoot.objectID,
            NULL,
            &p->pos,
            &p->snapshotpos);
        p->pos.vy += 100.0f;
        if (model != NULL &&
            (model->flags & UINT32_C(0x20)) != 0) {
            pelvis =
                coll_GetNodeCenter(player->playerID, 0);
            if (pelvis != NULL) {
                p->pos.vy = (float)pelvis->vy;
            }
        }
        scene_gSetSceneModelMatrixFV(
            p->physicsRoot.objectID,
            &p->angle,
            &p->pos);
    }
    return 1;
}

void MovePlayer(physicsObject *p0)
{
    sceneObject *scene;
    playerObject *player;
    FVECTOR testpos0;
    FVECTOR dir0 = {0.0f, 0.0f, 0.0f};
    float dist0;

    /*
     * The compact entry predicate at RVA 0xDDBE1 selects exactly MOVE_FLY
     * and MOVE_COREDEATH.
     */
    if (p0->movemode == MOVE_FLY ||
        p0->movemode == MOVE_COREDEATH) {
        p0->pos.vx += p0->mov.vx;
        p0->pos.vy += p0->mov.vy;
        p0->pos.vz += p0->mov.vz;
        p0->flags &= ~UINT32_C(0x00001800);
        return;
    }

    scene = (sceneObject *)p0->physicsRoot.pParent;
    player = (playerObject *)scene->pPlayer;
    if ((player->pFlags & UINT32_C(0x44000000)) == 0) {
        testpos0.vx = p0->pos.vx + p0->mov.vx;
        testpos0.vy = p0->pos.vy + p0->mov.vy;
        testpos0.vz = p0->pos.vz + p0->mov.vz;
        dist0 = VectorNormalize2(&p0->mov, &dir0);

        /*
         * Exact RVAs 0xDDC64..0xDDF3D. The public facade shares this
         * instruction-reviewed sweep and adds validation only.
         */
        (void)jpb_PhysicsMoveCharacterContacts(p0);
        if ((p0->flags & UINT32_C(0x00000080)) == 0 &&
            WorldBlocking(
                player,
                p0,
                &p0->pos,
                &testpos0,
                &dir0,
                dist0) != 0 &&
            (player->pFlags & UINT32_C(0x04000000)) != 0) {
            p0->flags &= ~UINT32_C(0x00001800);
            return;
        }

        if (p0->movemode == MOVE_NORMAL &&
            (p0->flags & UINT32_C(0x00000080)) == 0 &&
            (player->pFlags & UINT32_C(0x44000009)) == 0 &&
            player->playerID != 0x23 &&
            physics_move_start_fall(p0, player)) {
            physics_move_postprocess(p0, player);
            p0->flags &= ~UINT32_C(0x00001800);
            return;
        }

        p0->pos.vx += p0->mov.vx;
        p0->pos.vy += p0->mov.vy;
        p0->pos.vz += p0->mov.vz;
    } else if (
        (player->pFlags & UINT32_C(0x04000000)) == 0) {
        int newground;
        int diff;

        if ((uint16_t)(player->playerID - 0x3e) > 1U) {
            player->pFlags &= UINT32_C(0xbbffffff);
        }
        p0->pos.vx += p0->mov.vx;
        p0->pos.vy += p0->mov.vy;
        p0->pos.vz += p0->mov.vz;
        player->groundDelay = 0;
        if (p0->standee == NULL &&
            (p0->flags & UINT32_C(0x00000040)) == 0) {
            newground = intersec_FindWalkHeightFV(
                &p0->pos, NULL, &player->playerRoot, 0);
            diff = physics_abs_truncated_float(
                p0->pos.vy - (float)newground);
            if ((float)diff <
                fGlobalFrameRate * 256.0f * 0.25f) {
                p0->pos.vy = (float)newground;
                p0->validairground = (float)newground;
            }
        } else {
            p0->validairground = p0->pos.vy;
        }
        player->pFlags &= ~UINT32_C(0x00000008);
    } else {
        unsigned shift;

        p0->flags |= UINT32_C(0x00001800);
        if (p0->solidgrabbed == NULL ||
            p0->solidgrabbed->solid == NULL) {
            p0->pos = p0->ledgepoint;
        } else {
            CalcWorldPosFromRelative(
                p0->solidgrabbed->solid,
                &p0->ledgepoint,
                &p0->pos);
            p0->standee = p0->solidgrabbed;
            CalcSolidRelativePos(
                p0->standee->solid, p0, &p0->pos);
            CalcWorldRelativePos(p0->standee->solid, p0);
        }
        shift = p0->standee != NULL ? 9U : 10U;
        p0->pos.vx +=
            (float)physics_move_arithmetic_shift_right(
                rsin(p0->angle.vy), shift);
        p0->pos.vz +=
            (float)physics_move_arithmetic_shift_right(
                rcos(p0->angle.vy), shift);
        player->pFlags &= ~UINT32_C(0x04000000);
        if ((player->pFlags & UINT32_C(0x40000000)) == 0) {
            player->pFlags |= UINT32_C(0x40000000);
        }
        player->pFlags &= ~UINT32_C(0x00000008);
    }

    physics_move_postprocess(p0, player);
    p0->flags &= ~UINT32_C(0x00001800);
}

int jpb_PhysicsMovePlayer(physicsObject *physics)
{
    sceneObject *scene;

    if (physics == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    if (physics->movemode == MOVE_FLY ||
        physics->movemode == MOVE_COREDEATH) {
        MovePlayer(physics);
        return JPB_PHYSICS_PARTIAL_OK;
    }
    if (physics->physicsRoot.pParent == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    scene =
        (sceneObject *)physics->physicsRoot.pParent;
    if (scene->pPlayer == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    MovePlayer(physics);
    return JPB_PHYSICS_PARTIAL_OK;
}

/* 0xDE650, 953 bytes, global, 5 named locals
 * ProcessPhysicsObjects
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static int physics_object_is_scheduled(physicsObject *physics)
{
    return
        physics->physicsRoot.objectID != -1 &&
        obj_gCheckObjectFlag(
            &physics->physicsRoot,
            0,
            UINT32_C(0x00000020)) == 0;
}

void ProcessPhysicsObjects(void)
{
    int distances[5];
    int height = 0;
    int index;

    if (gSCENE_READY == 0) {
        return;
    }

    /*
     * The reference prints this mask and the five distances every frame.
     * Keep the calculations, which are useful to later culling stages, but
     * leave the diagnostic-only stdout traffic out of the dependency-light
     * runtime until it is routed through the portable debug backend.
     */
    (void)cliptofrustrum(
        clippingfrustrum,
        &maPhysicsData[0].pos,
        0,
        distances);

    PushMatrix();
    BuildSolids();

    switch (camera_GetCurrentCameraType()) {
    case 0:
        height = physics_trunc_float_to_i32(
            (maPhysicsData[0].validairground +
             maPhysicsData[1].validairground) *
            0.5f);
        break;
    case 1:
        height = physics_trunc_float_to_i32(
            maPhysicsData[0].validairground);
        break;
    case 2:
        height = physics_trunc_float_to_i32(
            maPhysicsData[1].validairground);
        break;
    default:
        break;
    }
    (void)CalcNewBox(height, collisionfrustrum, box);

    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        physicsObject *physics = &maPhysicsData[index];

        if (physics_object_is_scheduled(physics)) {
            physics->flags &= UINT32_C(0xffd7bfff);
        }
    }

    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        physicsObject *physics = &maPhysicsData[index];

        if (physics_object_is_scheduled(physics) &&
            gSCENE_READY != 0 &&
            OptionStruct.AIDebug != 0) {
            sceneObject *scene =
                (sceneObject *)physics->physicsRoot.pParent;
            playerObject *player =
                (playerObject *)scene->pPlayer;
            int id = player->playerRoot.objectID;

            if (coll_GetNode(id, 8) != NULL) {
                (void)coll_GetNodeCenter(id, 8);
            }
        }
    }

    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        physicsObject *physics = &maPhysicsData[index];

        if (physics_object_is_scheduled(physics)) {
            /*
             * CalcMovement is a PDB-local routine exposed under this
             * descriptive facade while its portable extraction is reviewed.
             * Valid scheduler records satisfy the facade's component checks.
             */
            (void)jpb_PhysicsCalcMovement(physics);
        }
    }

    if ((gSCENE_READY != 0 && initialLevelPauseDelay < 2) ||
        game_gIsGameFlags(UINT32_C(0x02000000)) == 0) {
        for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
            physicsObject *physics = &maPhysicsData[index];

            if (physics_object_is_scheduled(physics)) {
                MovePlayer(physics);
            }
        }
    }

    checkdriving(0);
    checkdriving(1);

    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        physicsObject *physics = &maPhysicsData[index];

        if (physics_object_is_scheduled(physics)) {
            UpdateSceneObject(physics);
        }
    }

    gpWorld->p0location.vx = maPhysicsData[0].vpos.vx;
    gpWorld->p0location.vy = maPhysicsData[0].vpos.vy;
    gpWorld->p0location.vz = maPhysicsData[0].vpos.vz;
    gpWorld->p1location.vx = maPhysicsData[1].vpos.vx;
    gpWorld->p1location.vy = maPhysicsData[1].vpos.vy;
    gpWorld->p1location.vz = maPhysicsData[1].vpos.vz;

    PopMatrix();

    if (streetsending != 0) {
        streetsending -= gGlobalFrameRate;
        if (streetsending <= 0) {
            (void)game_gModEnergy(0, -255);
            (void)game_gModEnergy(1, -255);
            (void)game_gSetGameFlags(UINT32_C(0x00000020));
            (void)game_gSetGameFlags(UINT32_C(0x00000040));
            obj_gSetObjectFlag(
                &gaPlayerData[0].playerRoot,
                0,
                UINT32_C(0x00000020));
            obj_gSetObjectFlag(
                &gaPlayerData[1].playerRoot,
                0,
                UINT32_C(0x00000020));
            maPhysicsData[0].flags &= UINT32_C(0xffffff40);
            maPhysicsData[1].flags &= UINT32_C(0xffffff40);
            /*
             * The reference emits one eight-byte zero store at RVA target
             * 0x954440: maRange[1][4] and maRange[1][5].
             */
            maRange[1][4] = 0.0f;
            maRange[1][5] = 0.0f;
            streetsending = 0;
        }
    }
}

/* 0xDEA10, 148 bytes, global, 1 named locals
 * UpdatePublicVars
 * PDB type: void (physicsObject*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void LaunchMapAnimEffects(int n, VECTOR *worldpos, int32_t *elist)
{
    int soundplayed = 0;
    int fanstop = 0;
    int level = (int)(int8_t)LevelSelect;
    int i;

    if (level == 9 || level == 4 || n <= 0) {
        return;
    }

    for (i = 0; i < n; ++i) {
        uint32_t eventword = (uint32_t)elist[i];
        unsigned f = eventword >> 24;
        uint16_t event = eventarray[level][f];
        int e = event & 0xff;
        int s = (event >> 8) & 0xf;
        VECTOR pos = {
            0x8080 - (int)((eventword & UINT32_C(0xff)) << 8),
            (int)((eventword >> 8) & UINT32_C(0xff00)) + 0x80,
            (int)(eventword & UINT32_C(0xff00)) + 0x8180,
            0
        };

        if (f != 0xf && e != 0) {
            EffectHeader *effect = paEffects[e];

            sprite_AddSpriteEffect(
                effect->aEffects,
                (int)effect->num,
                worldpos,
                NULL);
            if (!soundplayed) {
                sound_Play(&pos, 3, maphitsounds[s], 0);
                soundplayed = 1;
            }
        }

        if (level == 1) {
            if (f == 0xe) {
                cube_HideMesh(0);
                cube_ShowMesh(1);
            }
        } else if (level == 5) {
            if (f == 2) {
                cube_HideMesh(2);
                cube_ShowMesh(1);
            }
        } else if (level == 8) {
            cube_HideMesh((int)f * 2);
            cube_ShowMesh((int)f * 2 - 1);
        } else if (level == 10 &&
                   (f == 3 || f == 0xe)) {
            fanstop = 1;
        }
    }

    if (fanstop) {
        StopNearestFan(worldpos);
    }
}
void UpdatePublicVars(physicsObject *physics)
{
    physics->svangle.vx = (int16_t)physics->angle.vx;
    physics->svangle.vy = (int16_t)physics->angle.vy;
    physics->svangle.vz = (int16_t)physics->angle.vz;
    physics->svpos.vx = (int16_t)(int32_t)physics->pos.vx;
    physics->svpos.vy = (int16_t)(int32_t)physics->pos.vy;
    physics->svpos.vz = (int16_t)(int32_t)physics->pos.vz;
    physics->svmov.vx = (int16_t)(int32_t)physics->mov.vx;
    physics->svmov.vy = (int16_t)(int32_t)physics->mov.vy;
    physics->svmov.vz = (int16_t)(int32_t)physics->mov.vz;
    physics->vpos.vx = (int32_t)physics->pos.vx;
    physics->vpos.vy = (int32_t)physics->pos.vy;
    physics->vpos.vz = (int32_t)physics->pos.vz;
}

/* 0xDEAB0, 245 bytes, global, 4 named locals
 * UpdateSceneObject
 * PDB type: void (physicsObject*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void UpdateSceneObject(physicsObject *p0)
{
    enum {
        JPB_UPDATE_PLAYER_FACE_TARGET = 0x00400000,
        JPB_UPDATE_PLAYER_FACE_MASK = 0x00000501,
        JPB_UPDATE_PLAYER_USE_FACE = 0x00080000,
        JPB_UPDATE_PLAYER_ANGLE_MASK = 0x00080501,
        JPB_UPDATE_PHYSICS_FACING_LOCK = 0x00400000
    };
    sceneObject *scene =
        (sceneObject *)p0->physicsRoot.pParent;
    playerObject *player =
        (playerObject *)scene->pPlayer;
    objectRoot *target;
    VECTOR *targetpos;
    VECTOR *playerpos;
    VECTOR *angle;

    if ((player->pFlags &
         (uint32_t)JPB_UPDATE_PLAYER_FACE_TARGET) != 0 &&
        (player->pFlags &
         (uint32_t)JPB_UPDATE_PLAYER_FACE_MASK) == 0 &&
        (p0->flags &
         (uint32_t)JPB_UPDATE_PHYSICS_FACING_LOCK) == 0) {
        int facing = 0;

        target = (objectRoot *)player->target;
        playerpos =
            physics_gGetPosition(&player->playerRoot);
        targetpos = physics_gGetPosition(target);
        if (playerpos != NULL && targetpos != NULL) {
            facing =
                ratan2(
                    targetpos->vx - playerpos->vx,
                    targetpos->vz - playerpos->vz) &
                0x0fff;
        }
        ((physicsObject *)scene->pPhysics)->angle.vy =
            facing;
    }

    angle =
        (player->pFlags &
         (uint32_t)JPB_UPDATE_PLAYER_ANGLE_MASK) ==
                (uint32_t)JPB_UPDATE_PLAYER_USE_FACE
            ? &p0->face
            : &p0->angle;
    scene_gSetSceneModelMatrixFV(
        p0->physicsRoot.objectID, angle, &p0->pos);
    if (p0->pos.vy - p0->airGround < 512.0f) {
        p0->validairground = p0->airGround;
    }
}

int jpb_PhysicsUpdateSceneObject(physicsObject *physics)
{
    sceneObject *scene;
    playerObject *player;
    int id;

    if (physics == NULL ||
        physics->physicsRoot.pParent == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    id = physics->physicsRoot.objectID;
    if (id < 0 || id >= JPB_PHYSICS_CAPACITY ||
        physics != &maPhysicsData[id]) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    scene =
        (sceneObject *)physics->physicsRoot.pParent;
    if (scene != &maSceneData[id] ||
        scene->pPhysics != &physics->physicsRoot ||
        scene->pPlayer == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    player = (playerObject *)scene->pPlayer;
    if ((player->pFlags & UINT32_C(0x00400000)) != 0 &&
        (player->pFlags & UINT32_C(0x00000501)) == 0 &&
        (physics->flags & UINT32_C(0x00400000)) == 0) {
        objectRoot *target =
            (objectRoot *)player->target;

        if (player->playerRoot.pParent == NULL ||
            ((sceneObject *)player->playerRoot.pParent)
                    ->pPhysics == NULL ||
            (target != NULL &&
             (target->pParent == NULL ||
              ((sceneObject *)target->pParent)
                      ->pPhysics == NULL))) {
            return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
        }
    }

    UpdateSceneObject(physics);
    return JPB_PHYSICS_PARTIAL_OK;
}

/* 0xDEBB0, 1252 bytes, local, 16 named locals
 * WorldBlocking
 * PDB type: int (playerObject*, physicsObjec...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static int WorldBlocking(
    playerObject *player,
    physicsObject *p0,
    FVECTOR *startpos,
    FVECTOR *endpos,
    FVECTOR *direction,
    float distance)
{
    enum {
        JPB_PLAYER_FLAG_AIRBORNE = 0x00000001,
        JPB_PLAYER_FLAG_LANDING = 0x00000008,
        JPB_PLAYER_FLAG_WORLD_BLOCKED = 0x00100000,
        JPB_PHYSICS_FLAG_WORLD_BLOCKED = 0x00000800,
        JPB_PHYSICS_FLAG_DISABLE_GROUND_SNAP = 0x00002000,
        JPB_MAP_FLAG_LIQUID = 0x00020000,
        JPB_MAP_FLAG_AIR_GROUND = 0x00014000,
        JPB_MAP_FLAG_KEEP_NEW_MAPINFO = 0x02000000
    };
    _jheightstuff oldmapinfo;
    int worldBlocked;
    float ground;

    if (p0->physicsRoot.objectID > 1 &&
        player->pEnemy != NULL &&
        (player->pEnemy->enemyFlags & UINT32_C(0xc400)) != 0) {
        return 0;
    }
    if (p0->physicsRoot.objectID > 1 &&
        p0->mov.vx == 0.0f &&
        p0->mov.vy == 0.0f &&
        p0->mov.vz == 0.0f) {
        return 0;
    }

    oldmapinfo = p0->mapinfo;
    worldBlocked = CheckCubeBlocking(
        player,
        endpos,
        &p0->mov,
        direction,
        distance,
        &ground);
    if (worldBlocked == 0) {
        player->pFlags &= ~(uint32_t)JPB_PLAYER_FLAG_WORLD_BLOCKED;
        p0->lastpos = *endpos;
    } else {
        p0->flags |= (uint32_t)JPB_PHYSICS_FLAG_WORLD_BLOCKED;
        player->pFlags |= (uint32_t)JPB_PLAYER_FLAG_WORLD_BLOCKED;
    }

    if ((player->pFlags & (uint32_t)JPB_PLAYER_FLAG_AIRBORNE) != 0) {
        if (p0->mov.vy < 0.0f) {
            sceneObject *scene =
                (sceneObject *)player->playerRoot.pParent;
            modelObject *model = (modelObject *)scene->pModel;
            VECTOR *pos = NULL;
            int use_body_center =
                player->currentMotion == 4 ||
                player->currentMotion == 0x16 ||
                (player->pMotion[0]->motionFlags & UINT32_C(0x40)) != 0;

            if (use_body_center &&
                (model->flags & UINT32_C(0x20)) != 0) {
                /* Matched RVA 0xDEE49 reads the signed short at
                 * playerObject+0x88 (playernum).  playerID follows at +0x8A
                 * and is only used by the landing-motion exception below.
                 * Using the character ID here accidentally worked for
                 * Obi-Wan/P1 and Qui-Gon/P2, but looked in an unrelated
                 * collision row for every other roster/slot pairing. */
                pos = coll_GetNodeCenter(player->playernum, 3);
                if (pos == NULL) {
                    pos = coll_GetNodeCenter(player->playernum, 6);
                }
            }
            if (pos == NULL) {
                pos = coll_GetNodeCenter(player->playernum, 0);
            }

            if (pos != NULL &&
                (float)pos->vy + p0->mov.vy <= ground) {
                int landing_motion =
                    player->playerID == 0x4b ? -1 : 3;

                if (player->currentMotion != landing_motion) {
                    if (p0->movemode == MOVE_BLOWN) {
                        p0->movemode = MOVE_NORMAL;
                        p0->airTime = 0;
                        p0->realAirTime = 0;
                    } else if (
                        /* Matched RVA 0xDEEA3 reads physicsObject+0x180.
                         * On the recovered 64-bit _jheightstuff layout that
                         * is currentmapinfo.poly (+0x170 cube, +0x178 entry,
                         * +0x180 poly), not currentmapinfo.cube.  Reading the
                         * cube header as a polygon made arbitrary header bits
                         * masquerade as the liquid flag and suppressed normal
                         * landing on Coruscant and Marsh. */
                        p0->currentmapinfo.poly == NULL ||
                        (physics_map_flags(p0->currentmapinfo.poly) &
                         (uint32_t)JPB_MAP_FLAG_LIQUID) == 0) {
                        player->pFlags |=
                            (uint32_t)JPB_PLAYER_FLAG_LANDING;
                        p0->flags |=
                            (uint32_t)JPB_PHYSICS_FLAG_WORLD_BLOCKED;
                        p0->airmov.vx = 0.0f;
                        p0->airmov.vy = 0.0f;
                        p0->airmov.vz = 0.0f;
                        if ((p0->flags &
                             (uint32_t)
                                 JPB_PHYSICS_FLAG_DISABLE_GROUND_SNAP) ==
                            0) {
                            player->pFlags &=
                                ~(uint32_t)JPB_PLAYER_FLAG_AIRBORNE;
                            p0->pos.vy = ground;
                        }
                        p0->mov.vy = 0.0f;
                        brainutl_Land(player);
                    } else if (p0->airTime < 0xf000) {
                        physics_launch_splash(p0);
                    }
                }
            }
        }

        if (p0->mapinfo.poly != NULL &&
            (((player->pFlags &
               (uint32_t)JPB_PLAYER_FLAG_AIRBORNE) != 0) ||
             (((physics_map_flags(p0->mapinfo.poly) &
                (uint32_t)JPB_MAP_FLAG_KEEP_NEW_MAPINFO) == 0) &&
              ((player->pFlags &
                (uint32_t)JPB_PLAYER_FLAG_LANDING) == 0)))) {
            p0->mapinfo = oldmapinfo;
        }
        p0->airGround = ground;
        return worldBlocked;
    }

    p0->airGround = ground;
    if (p0->mapinfo.poly != NULL &&
        p0->currentmapinfo.poly != NULL &&
        (physics_map_flags(p0->mapinfo.poly) &
         (uint32_t)JPB_MAP_FLAG_AIR_GROUND) != 0 &&
        p0->mapinfo.poly == p0->currentmapinfo.poly) {
        p0->pos.vy = ground;
        return worldBlocked;
    }
    if (!(ground > -32760.0f)) {
        return worldBlocked;
    }
    if (p0->mapinfo.poly != NULL &&
        (physics_map_flags(p0->mapinfo.poly) &
         (uint32_t)JPB_MAP_FLAG_LIQUID) != 0) {
        return worldBlocked;
    }
    if (p0->currentmapinfo.poly != NULL &&
        p0->mapinfo.poly != NULL &&
        (physics_map_flags(p0->mapinfo.poly) &
         (uint32_t)JPB_MAP_FLAG_AIR_GROUND) != 0) {
        int threshold = flexmul(gGlobalFrameRate, 0x100);

        if (physics_abs_truncated_float(startpos->vy - ground) <
            threshold) {
            startpos->vy = ground;
            return worldBlocked;
        }
    }
    if (physics_abs_truncated_float(ground - startpos->vy) <
            p0->height / 2 - p0->radius &&
        (p0->flags &
         (uint32_t)JPB_PHYSICS_FLAG_DISABLE_GROUND_SNAP) == 0) {
        startpos->vy = ground;
    }
    return worldBlocked;
}

int jpb_PhysicsWorldBlocking(
    playerObject *player,
    physicsObject *physics,
    FVECTOR *startpos,
    FVECTOR *endpos,
    FVECTOR *direction,
    float distance)
{
    if (player == NULL ||
        physics == NULL ||
        startpos == NULL ||
        endpos == NULL ||
        direction == NULL) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    return WorldBlocking(
        player, physics, startpos, endpos, direction, distance);
}

/* 0xDF0A0, 342 bytes, global, 6 named locals
 * buildfrustrum
 * PDB type: void (MATRIX*, FVECTOR4*, VECTOR...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void buildfrustrum(
    MATRIX *m,
    FVECTOR4 *collide,
    VECTOR *campos,
    float percent,
    float xoff,
    float yoff)
{
    float z = (-xoff * percent) / 100.0f;

    buildplane(m, campos, &collide[0], 460.0f, 0.0f, z);
    buildplane(m, campos, &collide[1], -460.0f, 0.0f, z);
    z = (-yoff * percent) / 100.0f;
    buildplane(m, campos, &collide[2], 0.0f, -460.0f, z);
    buildplane(m, campos, &collide[3], 0.0f, 460.0f, z);
    buildplane(m, campos, &collide[4], 0.0f, 0.0f, -4096.0f);
}

/* 0xDF200, 354 bytes, global, 8 named locals
 * buildplane
 * PDB type: void (MATRIX*, VECTOR*, FVECTOR4...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */

/* 0xDF370, 327 bytes, local, 1 named locals
 * checkdriving
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static void checkdriving(int player_index)
{
    playerObject *player =
        &gaPlayerData[player_index];
    physicsObject *p0;

    if (player->playerRoot.objectID == -1 ||
        obj_gCheckObjectFlag(
            &player->playerRoot, 0, 0x00000020u) != 0 ||
        (player->pFlags & UINT32_C(0x00040200)) != 0) {
        return;
    }

    p0 = &maPhysicsData[player_index];
    if ((p0->flags & UINT32_C(0x00000020)) != 0) {
        int id = (int)(p0->flags & UINT32_C(0x0000001f));
        physicsObject *driven = &maPhysicsData[id];

        p0->pos = driven->pos;
        p0->angle = driven->angle;
        p0->mov = driven->mov;
        /* Exact checkdriving RVAs 0xDF429..0xDF461 copy mapinfo. */
        p0->mapinfo = driven->mapinfo;
    }
}

int jpb_PhysicsSyncDriverState(int player_index)
{
    physicsObject *physics;

    if (player_index < 0 || player_index >= 2) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }
    physics = &maPhysicsData[player_index];
    if ((physics->flags & UINT32_C(0x00000020)) != 0 &&
        (physics->flags & UINT32_C(0x0000001f)) >=
            JPB_PHYSICS_CAPACITY) {
        return JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT;
    }

    checkdriving(player_index);
    return JPB_PHYSICS_PARTIAL_OK;
}

/* 0xDF4C0, 140 bytes, global, 7 named locals
 * console_HideMeshCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */

/* 0xDF550, 140 bytes, global, 7 named locals
 * console_ShowMeshCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */

/* 0xDF5E0, 132 bytes, global, 1 named locals
 * dumpmatrix
 * PDB type: void (MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */

/* 0xDF670, 1168 bytes, local, 15 named locals
 * generalCollide
 * PDB type: int (_solid*, FVECTOR*, FVECTOR*...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static int generalCollide(
    _solid *s,
    FVECTOR *mov,
    FVECTOR *from,
    float vel,
    float radius)
{
    int ret = 0;
    int npolys;
    _svector *normals;
    int16_t *index;
    _svector *verts;
    int polygon;

    (void)from;
    (void)radius;
    verts = s->coords;
    normals = s->normals;
    index = (int16_t *)jpb_PhysicsResolveGeometryStream(
        s->geometry, JPB_POINTER_ARRAY_INDEX);
    npolys = s->geometry->numFaces;
    mvp.info.flags = 6;
    mvp.info.washack = 0;

    for (polygon = 0; polygon < npolys; ++polygon) {
        int16_t p3 = index[3];

        mvp.points[0].vx = (float)verts[index[0]].vx;
        mvp.points[0].vy = (float)verts[index[0]].vy;
        mvp.points[0].vz = (float)verts[index[0]].vz;
        mvp.points[1].vx = (float)verts[index[1]].vx;
        mvp.points[1].vy = (float)verts[index[1]].vy;
        mvp.points[1].vz = (float)verts[index[1]].vz;
        mvp.points[2].vx = (float)verts[index[2]].vx;
        mvp.points[2].vy = (float)verts[index[2]].vy;
        mvp.points[2].vz = (float)verts[index[2]].vz;

        if (p3 == INT16_MAX) {
            mvp.numsides = 3;
        } else {
            mvp.points[3].vx = (float)verts[p3].vx;
            mvp.points[3].vy = (float)verts[p3].vy;
            mvp.points[3].vz = (float)verts[p3].vz;
            mvp.numsides = 4;
        }

        mvp.facenormal.vx =
            (float)normals[polygon].vx *
            (1.0f / 4096.0f);
        mvp.facenormal.vy =
            (float)normals[polygon].vy *
            (1.0f / 4096.0f);
        mvp.facenormal.vz =
            (float)normals[polygon].vz *
            (1.0f / 4096.0f);
        mvp.info.washack = 0;

        if (solidhack == 0 ||
            (vel != 0.0f &&
             (float)normals[polygon].vy * mov->vy +
                     (float)normals[polygon].vx * mov->vx +
                     (float)normals[polygon].vz * mov->vz <=
                 0.0f)) {
            mvp.movement = *mov;
            mvp.distance = vel;
        } else {
            mvp.distance = 0.0f;
            mvp.movement.vx = -mvp.facenormal.vx;
            mvp.movement.vy = -mvp.facenormal.vy;
            mvp.movement.vz = -mvp.facenormal.vz;
            mvp.info.washack = 1;
        }

        if (sphereAndPoly() != 0) {
            int replace = 0;

            if ((mvp.info.type & 4) == 0) {
                if ((bestinfo.type & 4) == 0 &&
                    mvp.info.dist < bestinfo.dist) {
                    replace = 1;
                }
            } else if ((bestinfo.type & 4) == 0 ||
                       mvp.info.dist < bestinfo.dist) {
                replace = 1;
            }

            if (replace != 0) {
                bestinfo = mvp.info;
                ret = 1;
                if (mvp.info.type == 2) {
                    int newedge = mvp.info.edge;
                    int e = newedge + 1;

                    if (mvp.numsides <= e) {
                        e = 0;
                    }
                    edge_end = mvp.points[newedge];
                    edge_start = mvp.points[e];
                }
            }
        }

        index += 4;
    }

    return ret;
}

/*
 * Portable test/integration facade for the original module-local traversal.
 */
int jpb_PhysicsGeneralCollide(
    _solid *solid,
    FVECTOR *movement,
    FVECTOR *from,
    float velocity,
    float radius)
{
    return generalCollide(
        solid, movement, from, velocity, radius);
}

/* 0xDFB00, 3314 bytes, global, 42 named locals
 * newclosestPoly
 * PDB type: int (FVECTOR*, FVECTOR*, FVECTOR...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
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
    int32_t **ppPoly)
{
    int32_t *whichcube = NULL;
    int32_t *whichentry = NULL;
    int32_t *whichpoly = NULL;
    FVECTOR *scratch = (FVECTOR *)(void *)gaScratch;
    uint32_t flag = UINT32_C(1) << ((uint32_t)playerid & 31U);
    int match = 0;
    int min_x;
    int max_x;
    int min_z;
    int max_z;
    int x_start;
    int x_end;
    int z_start;
    int z_end;
    FVECTOR cubeorg;
    int z;

    if (move->vx <= 0.0f) {
        mvp.vmin.vx = to->vx;
        mvp.vmax.vx = from->vx;
    } else {
        mvp.vmin.vx = from->vx;
        mvp.vmax.vx = to->vx;
    }
    if (move->vy <= 0.0f) {
        mvp.vmin.vy = to->vy;
        mvp.vmax.vy = from->vy;
    } else {
        mvp.vmin.vy = from->vy;
        mvp.vmax.vy = to->vy;
    }
    if (move->vz <= 0.0f) {
        mvp.vmin.vz = to->vz;
        mvp.vmax.vz = from->vz;
    } else {
        mvp.vmin.vz = from->vz;
        mvp.vmax.vz = to->vz;
    }

    mvp.vmin.vx -= radius;
    mvp.vmin.vy -= radius;
    mvp.vmin.vz -= radius;
    mvp.vmax.vx += radius;
    mvp.vmax.vy += radius;
    mvp.vmax.vz += radius;
    mvp.startpos = *from;
    mvp.to = *to;
    mvp.movement = *movenormal;
    mvp.radius = radius;
    mvp.distance = distance;
    mvp.info.type = 0;
    mvp.info.flags = 0;
    mvp.info.washack = 0;
    bestinfo.type = 0;
    bestinfo.dist = distance + 1.0f;

    min_x = physics_trunc_float_to_i32(mvp.vmin.vx);
    max_x = physics_trunc_float_to_i32(mvp.vmax.vx);
    min_z = physics_trunc_float_to_i32(mvp.vmin.vz);
    max_z = physics_trunc_float_to_i32(mvp.vmax.vz);
    x_start = (int32_t)(
        UINT32_C(0x80ff) - (uint32_t)min_x) >> 8;
    x_end = (int32_t)(
        UINT32_C(0x80ff) - (uint32_t)max_x) >> 8;
    z_start = (int32_t)(
        (uint32_t)min_z + UINT32_C(0x7f00)) >> 8;
    z_end = (int32_t)(
        (uint32_t)max_z + UINT32_C(0x7f00)) >> 8;
    cubeorg.vx = (float)(int32_t)((uint32_t)min_x & UINT32_C(0xffffff00));
    cubeorg.vy = (float)(int32_t)(
        (uint32_t)physics_trunc_float_to_i32(mvp.vmin.vy) &
        UINT32_C(0xffffff00));
    cubeorg.vz = (float)(int32_t)((uint32_t)min_z & UINT32_C(0xffffff00));

    if (maPhysicsData[playerid].airstick != 0) {
        /* Reference diagnostic: "AIRSTICK MY FRIEND: %d!\n". */
    }

    if (playerid < 2 && camera_GetCurrentCameraType() != 6) {
        if (gpWorld->currentDolly < 0x80 &&
            maPhysicsData[playerid].airstick < 2) {
            static const float radius_adjustment[4] = {
                24.0f, 24.0f, 64.0f, -128.0f
            };
            int plane;

            for (plane = 0; plane < 4; ++plane) {
                if (box[plane].vw < 65536.0f) {
                    match |= planecheck(
                        physics_trunc_float_to_i32(
                            radius + radius_adjustment[plane]),
                        &box[plane]);
                }
            }
        }

        if (uberXRange != 0 &&
            uberZRange != 0 &&
            uberLock == 0 &&
            maPhysicsData[playerid].airstick < 2) {
            FVECTOR4 *uberplane = (FVECTOR4 *)(void *)gaScratch;
            int plane_radius = physics_trunc_float_to_i32(radius);

            uberplane[0].vx = 0.0f;
            uberplane[0].vy = 0.0f;
            uberplane[0].vz = 1.0f;
            uberplane[0].vw = (float)(uberPos.vz - uberZRange);
            match |= planecheck(plane_radius, &uberplane[0]);

            uberplane[1].vx = 0.0f;
            uberplane[1].vy = 0.0f;
            uberplane[1].vz = -1.0f;
            uberplane[1].vw = (float)-(uberPos.vz + uberZRange);
            match |= planecheck(plane_radius, &uberplane[1]);

            uberplane[2].vx = 1.0f;
            uberplane[2].vy = 0.0f;
            uberplane[2].vz = 0.0f;
            uberplane[2].vw = (float)(uberPos.vx - uberXRange);
            match |= planecheck(plane_radius, &uberplane[2]);

            uberplane[3].vx = -1.0f;
            uberplane[3].vy = 0.0f;
            uberplane[3].vz = 0.0f;
            uberplane[3].vw = (float)-(uberPos.vx + uberXRange);
            match |= planecheck(plane_radius, &uberplane[3]);
        }
    }

    mvp.startpos = *from;
    mvp.to = *to;
    mvp.movement = *movenormal;
    mvp.radius = radius;
    mvp.distance = distance;
    mvp.info.washack = 0;
    mvp.info.flags = 0;

    if (numsolids != 0) {
        int slot;

        for (slot = 0; slot < JPB_PHYSICS_CAPACITY; ++slot) {
            physicsObject *physics = &maPhysicsData[slot];
            _solid *s;

            if (physics->physicsRoot.objectID == -1 ||
                obj_gCheckObjectFlag(
                    &physics->physicsRoot, 0, UINT32_C(0x20)) != 0) {
                continue;
            }
            s = physics->solid;
            if (s == NULL ||
                s->modelnode == NULL ||
                (s->flags & flag) == 0) {
                continue;
            }
            if (generalCollide(
                    s, move, from, distance, radius) != 0) {
                bestinfo.flags |= 6;
                match = 1;
                whichsolid = s;
            }
        }
    }

    mvp.movement = *movenormal;
    mvp.to = *to;
    mvp.distance = distance;
    mvp.info.washack = 0;
    mvp.info.flags = 0;

    for (z = z_start; z <= z_end; ++z) {
        int x;

        cubeorg.vx =
            (float)(int32_t)((uint32_t)min_x & UINT32_C(0xffffff00));
        for (x = x_start; x >= x_end; --x) {
            if ((uint32_t)x < UINT32_C(0x100) &&
                z >= 0 &&
                z < mapyend) {
                int32_t cell = leveldata[x + z * 0x100];

                if (cell < 0) {
                    int32_t *cube =
                        leveldata + ((uint32_t)cell & UINT32_C(0x1ffff));

                    for (;;) {
                        uint32_t cube_header = (uint32_t)cube[0];
                        int lastcube =
                            (cube_header & UINT32_C(0x40000000)) != 0;
                        int cubebase =
                            (int)(cube_header & UINT32_C(0x7f)) * 0x100;
                        int cubetop =
                            cubebase +
                            (int)((cube_header >> 2) & UINT32_C(0xfe0));
                        int32_t *nextcube =
                            cube +
                            ((cube_header >> 26) & UINT32_C(0x0f)) +
                            1;

                        if (mvp.vmax.vy < (float)cubebase) {
                            break;
                        }
                        if (mvp.vmin.vy <= (float)cubetop) {
                            cubeorg.vy = (float)cubebase;
                            if (nextcube == cube + 1) {
                                int32_t *fat =
                                    leveldata +
                                    (leveldata[-4] >> 11) +
                                    (int)((cube_header >> 14) &
                                          UINT32_C(0xff)) *
                                        9;
                                int32_t normal_word = fat[0];

                                if (normal_word >= 0) {
                                    uint32_t normal_index =
                                        (uint32_t)normal_word &
                                        UINT32_C(0x1ffff);

                                    physics_decode_map_normal(
                                        leveldata,
                                        normal_index,
                                        &mvp.facenormal);
                                    mvp.points[0].vx =
                                        (float)(0x8100 -
                                            (int)(uint16_t)fat[5]);
                                    mvp.points[0].vy =
                                        (float)(
                                            (int16_t)(
                                                (uint32_t)fat[5] >> 16) -
                                            0x7f00);
                                    mvp.points[0].vz = (float)fat[3];
                                    mvp.points[1].vx =
                                        (float)(0x8100 -
                                            (int)(uint16_t)fat[6]);
                                    mvp.points[1].vy =
                                        (float)(
                                            (int16_t)(
                                                (uint32_t)fat[6] >> 16) -
                                            0x7f00);
                                    mvp.points[1].vz =
                                        (float)(int16_t)(
                                            (uint32_t)fat[3] >> 16);
                                    mvp.points[2].vx =
                                        (float)(0x8100 -
                                            (int)(uint16_t)fat[7]);
                                    mvp.points[2].vy =
                                        (float)(
                                            (int16_t)(
                                                (uint32_t)fat[7] >> 16) -
                                            0x7f00);
                                    mvp.points[2].vz = (float)fat[4];
                                    mvp.points[3].vx =
                                        (float)(0x8100 -
                                            (int)(uint16_t)fat[8]);
                                    mvp.points[3].vy =
                                        (float)(
                                            (int16_t)(
                                                (uint32_t)fat[8] >> 16) -
                                            0x7f00);
                                    mvp.points[3].vz =
                                        (float)(int16_t)(
                                            (uint32_t)fat[4] >> 16);
                                    mvp.numsides = 4;

                                    if (polycollidecheck() != 0) {
                                        bestinfo.flags &= (int16_t)~6;
                                        match = 1;
                                        whichcube = cube;
                                        whichentry = NULL;
                                        whichpoly = fat;
                                    }
                                }
                            } else {
                                int32_t *entry = cube + 2;

                                while (entry < nextcube) {
                                    int numv;
                                    int32_t *libpart =
                                        jon_getlibpartfloat(
                                            scratch,
                                            entry,
                                            &cubeorg,
                                            leveldata,
                                            &numv);
                                    int32_t *poly = libpart + 2;

                                    (void)numv;
                                    for (;;) {
                                        uint32_t poly0 =
                                            (uint32_t)poly[0];
                                        uint32_t poly1 =
                                            (uint32_t)poly[1];
                                        int endpoly =
                                            (poly0 &
                                             UINT32_C(0xc0000000)) != 0;

                                        if ((poly1 &
                                             UINT32_C(0xc0000000)) == 0) {
                                            uint32_t normal_index =
                                                poly0 &
                                                UINT32_C(0x1ffff);

                                            if (((uint32_t)
                                                     leveldata[normal_index] &
                                                 UINT32_C(0x20000)) == 0) {
                                                int istri =
                                                    (int)((poly1 >> 20) &
                                                          UINT32_C(1));
                                                unsigned p0 =
                                                    poly1 & UINT32_C(0x1f);
                                                unsigned p1 =
                                                    (poly1 >> 5) &
                                                    UINT32_C(0x1f);
                                                unsigned p2 =
                                                    (poly1 >> 10) &
                                                    UINT32_C(0x1f);

                                                mvp.info.type = 0;
                                                mvp.info.flags = 0;
                                                mvp.numsides = 4 - istri;
                                                physics_decode_map_normal(
                                                    leveldata,
                                                    normal_index,
                                                    &mvp.facenormal);
                                                mvp.points[0] = scratch[p0];
                                                mvp.points[1] = scratch[p1];
                                                mvp.points[2] = scratch[p2];
                                                if (istri == 0) {
                                                    unsigned p3 =
                                                        (poly1 >> 15) &
                                                        UINT32_C(0x1f);

                                                    mvp.points[3] =
                                                        mvp.points[2];
                                                    mvp.points[2] =
                                                        scratch[p3];
                                                }

                                                if (polycollidecheck() != 0) {
                                                    bestinfo.flags &=
                                                        (int16_t)~6;
                                                    match = 1;
                                                    whichcube = cube;
                                                    whichentry = entry;
                                                    whichpoly = poly;
                                                }
                                            }
                                        }
                                        poly += 2;
                                        if (endpoly) {
                                            break;
                                        }
                                    }

                                    entry +=
                                        ((uint32_t)entry[0] >> 30) + 1;
                                }
                            }
                        }

                        if (lastcube) {
                            break;
                        }
                        cube = nextcube;
                    }
                }
            }
            cubeorg.vx += 256.0f;
        }
        cubeorg.vz += 256.0f;
    }

    if (match != 0) {
        if (ppCube != NULL) {
            *ppCube = whichcube;
        }
        if (ppEntry != NULL) {
            *ppEntry = whichentry;
        }
        if (ppPoly != NULL) {
            *ppPoly = whichpoly;
        }
    }
    return match;
}

/* 0xE0800, 146 bytes, global, 6 named locals
 * physics_FindNearestEnemy
 * PDB type: int (objectRoot*, int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int physics_FindNearestEnemy(
    objectRoot *object0, int type)
{
    sceneObject *scene0 =
        (sceneObject *)object0->pParent;
    physicsObject *p0 =
        (physicsObject *)scene0->pPhysics;
    int id = p0->physicsRoot.objectID;
    int dist = 0x1fffe;
    int index;

    for (index = 0;
         index < JPB_PHYSICS_CAPACITY;
         ++index) {
        physicsObject *p = &maPhysicsData[index];

        if (p->physicsRoot.objectID != id &&
            p->physicsRoot.objectID != -1) {
            sceneObject *scene =
                (sceneObject *)
                    p->physicsRoot.pParent;
            playerObject *player;

            /*
             * The physics pool also contains level machinery.  The retail
             * caller asks specifically for an enemy owner type, so only
             * actor scenes with a complete enemy placement participate.
             * Treating every occupied physics slot as an enemy dereferences
             * the NULL pPlayer/pEnemy chain used by doors and other authored
             * machinery in Palace, Ruins, and Mini4.
             */
            if (scene == NULL || scene->pPlayer == NULL) {
                continue;
            }
            player = (playerObject *)scene->pPlayer;
            if (player->pEnemy == NULL ||
                player->pEnemy->pPlace == NULL) {
                continue;
            }

            if (player->pEnemy->pPlace
                    ->aiDf.ownerType == type) {
                int d = physics_gGetRange(
                    &p0->physicsRoot,
                    &p->physicsRoot);

                if (d < dist) {
                    dist = d;
                }
            }
        }
    }
    return dist;
}

/* 0xE08A0, 243 bytes, global, 7 named locals
 * physics_FindWithinRange
 * PDB type: physicsObject* (VECTOR*, long*, ...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
physicsObject *physics_FindWithinRange(
    VECTOR *position, uint32_t *mask, int range)
{
    uint32_t checked;
    uint32_t bit = 1;
    int index;

    if (position == NULL || mask == NULL) {
        return NULL;
    }
    checked = *mask;
    for (index = 0;
         index < JPB_PHYSICS_CAPACITY;
         ++index, bit <<= 1) {
        playerObject *player =
            &gaPlayerData[index];

        if ((checked & bit) != 0 ||
            player->playerRoot.objectID == -1 ||
            obj_gCheckObjectFlag(
                &player->playerRoot,
                0,
                UINT32_C(0x20)) != 0 ||
            player->playerID == 0x48) {
            continue;
        }
        if (index < 2 &&
            (player->pFlags &
             UINT32_C(0x40200)) != 0) {
            continue;
        }
        if (vec_QuickRangeCheck(
                position,
                &maPhysicsData[index].vpos,
                range) != 0) {
            *mask |= bit;
            return &maPhysicsData[index];
        }
    }
    return NULL;
}

/* 0xE09A0, 116 bytes, global, 4 named locals
 * physics_ForceFaceLock
 * PDB type: long (objectRoot*, objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int32_t physics_ForceFaceLock(
    objectRoot *player, objectRoot *locked)
{
    VECTOR *player_position = NULL;
    VECTOR *locked_position = NULL;
    physicsObject *physics;
    uint32_t facing = 0;

    if (player != NULL) {
        sceneObject *scene =
            (sceneObject *)player->pParent;

        physics = (physicsObject *)scene->pPhysics;
        player_position = &physics->vpos;
    }
    if (locked != NULL) {
        sceneObject *scene =
            (sceneObject *)locked->pParent;

        physics = (physicsObject *)scene->pPhysics;
        locked_position = &physics->vpos;
    }
    if (player_position != NULL &&
        locked_position != NULL) {
        facing = (uint32_t)ratan2(
                     locked_position->vx -
                         player_position->vx,
                     locked_position->vz -
                         player_position->vz) &
                 UINT32_C(0x00000fff);
    }
    physics = (physicsObject *)(
        (sceneObject *)player->pParent)->pPhysics;
    physics->face.vy = (int32_t)facing;
    return 0;
}

/* 0xE0A20, 15 bytes, global, 1 named locals
 * physics_GetPoly
 * PDB type: long* (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int32_t *physics_GetPoly(objectRoot *object)
{
    return physics_from_root(object)->lastpolyhit;
}

/* 0xE0A30, 169 bytes, global, 0 named locals
 * physics_InitPhysics
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_InitPhysics(void)
{
    int row;
    int column;

    for (row = 0; row < JPB_PHYSICS_CAPACITY; ++row) {
        for (column = 0;
             column < JPB_PHYSICS_CAPACITY;
             ++column) {
            maRange[row][column] = -1.0f;
        }
    }
}

/* 0xE0AE0, 3 bytes, global, 2 named locals
 * physics_MapAnimCallBack
 * PDB type: int (long*, objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int physics_MapAnimCallBack(
    int32_t *arguments, objectRoot *object)
{
    (void)arguments;
    (void)object;
    return 0;
}

/* 0xE0AF0, 1574 bytes, global, 4 named locals
 * physics_ResetJedi
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */

/* 0xE1120, 486 bytes, global, 9 named locals
 * physics_gCalcTargetPos
 * PDB type: void (int, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gCalcTargetPos(int player_num, VECTOR *offset)
{
    MATRIX matrix = {
        {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}
        },
        {0, 0, 0}
    };
    physicsObject *physics = &maPhysicsData[player_num];
    physicsObject *target = &maPhysicsData[player_num ^ 1];
    _svector angle = {
        (int16_t)physics->angle.vx,
        (int16_t)physics->angle.vy,
        (int16_t)physics->angle.vz,
        0
    };
    VECTOR relative;
    playerObject *player;

    (void)fRotMatrix(&angle, &matrix);
    PushMatrix();
    relative.vx = physics_trunc_float_to_i32(
        (float)offset->vx - target->mov.vx);
    relative.vy = physics_trunc_float_to_i32(
        (float)offset->vy - target->mov.vy);
    relative.vz = physics_trunc_float_to_i32(
        (float)offset->vz - target->mov.vz);
    relative.pad = 0;
    (void)fApplyMatrixLV(&matrix, &relative, offset);
    PopMatrix();

    offset->vx = physics_trunc_float_to_i32(
        (float)offset->vx + physics->pos.vx);
    offset->vy = physics_trunc_float_to_i32(
        (float)offset->vy + physics->pos.vy);
    offset->vz = physics_trunc_float_to_i32(
        (float)offset->vz + physics->pos.vz);
    target->pos.vx = (float)offset->vx;
    target->pos.vy = (float)offset->vy;
    target->pos.vz = (float)offset->vz;

    player = player_gGetPlayerPtr(0);
    physics_from_root(&player->playerRoot)->constmov =
        (FVECTOR){0.0f, 0.0f, 0.0f};
    player = player_gGetPlayerPtr(1);
    physics_from_root(&player->playerRoot)->constmov =
        (FVECTOR){0.0f, 0.0f, 0.0f};

    scene_gSetSceneModelMatrixFV(
        0, &maPhysicsData[0].angle, &maPhysicsData[0].pos);
    scene_gSetSceneModelMatrixFV(
        1, &maPhysicsData[1].angle, &maPhysicsData[1].pos);
}

/* 0xE1310, 135 bytes, global, 3 named locals
 * physics_gCheckGround
 * PDB type: int (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int physics_gCheckGround(playerObject *player)
{
    physicsObject *physics =
        physics_from_root(&player->playerRoot);
    int radius_offset = 0;
    int height;

    if ((player->pFlags & UINT32_C(9)) != 0) {
        radius_offset = physics->radius / 4;
    } else if (
        physics->lastpolyhit != NULL &&
        jpb_LevelDataContains(
            physics->lastpolyhit,
            sizeof(*physics->lastpolyhit)) &&
        (leveldata[
             (uint32_t)*physics->lastpolyhit &
             UINT32_C(0x1ffff)] &
         INT32_C(0x20000)) != 0) {
        return 1;
    }
    height = physics_trunc_float_to_i32(
        physics->pos.vy - physics->airGround +
        (float)radius_offset);
    return height > 32;
}

/* 0xE13A0, 23 bytes, global, 2 named locals
 * physics_gClrConstantVector
 * PDB type: void (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void buildplane(
    MATRIX *m,
    VECTOR *campos,
    FVECTOR4 *plane,
    float x,
    float y,
    float z)
{
    float l;
    FVECTOR tmp;
    int32_t component;
    uint32_t negated;

    component = physics_trunc_float_to_i32(
        y * m->m[0][1] + x * m->m[0][0] + z * m->m[0][2]);
    negated = UINT32_C(0) - (uint32_t)component;
    tmp.vx = (float)(int32_t)negated;
    tmp.vz = (float)physics_trunc_float_to_i32(
        x * m->m[1][0] + y * m->m[1][1] + z * m->m[1][2]);
    tmp.vy = (float)physics_trunc_float_to_i32(
        x * m->m[2][0] + y * m->m[2][1] + z * m->m[2][2]);
    l = (float)(1.0 / sqrt((double)(
        tmp.vy * tmp.vy + tmp.vx * tmp.vx + tmp.vz * tmp.vz)));
    plane->vy = l * tmp.vy;
    plane->vx = l * tmp.vx;
    plane->vz = l * tmp.vz;
    plane->vw =
        (float)campos->vy * plane->vy +
        (float)campos->vx * plane->vx +
        (float)campos->vz * plane->vz;
}
void physics_gClrConstantVector(objectRoot *object)
{
    physicsObject *physics = physics_from_root(object);

    physics->constmov.vy = 0.0f;
    physics->constmov.vz = 0.0f;
    physics->constmov.vx = 0.0f;
}

/* 0xE13C0, 263 bytes, global, 5 named locals
 * physics_gCreateObject
 * PDB type: physicsObject* (sceneObject*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
physicsObject *physics_gCreateObject(sceneObject *scene)
{
    physicsObject *physics =
        physics_gGetNewObject(scene->sceneRoot.objectID);

    if (physics != NULL) {
        (void)obj_gSetChildObject(
            scene, &physics->physicsRoot, 2);
        scene_gSetSceneModelMatrixFV(
            physics->physicsRoot.objectID,
            &physics->angle,
            &physics->pos);
        physics->maxledge = 0x10000;
    }
    return physics;
}

/* 0xE14D0, 96 bytes, global, 4 named locals
 * physics_gFaceTarget
 * PDB type: long (objectRoot*, objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int32_t physics_gFaceTarget(
    objectRoot *player, objectRoot *target)
{
    VECTOR *playerpos = NULL;
    VECTOR *targetpos = NULL;

    if (player != NULL) {
        playerpos = &physics_from_root(player)->vpos;
    }
    if (target != NULL) {
        targetpos = &physics_from_root(target)->vpos;
    }
    if (playerpos != NULL && targetpos != NULL) {
        int32_t dx = (int32_t)(
            (uint32_t)targetpos->vx -
            (uint32_t)playerpos->vx);
        int32_t dz = (int32_t)(
            (uint32_t)targetpos->vz -
            (uint32_t)playerpos->vz);

        return ratan2(dx, dz) & 0x0fff;
    }
    return 0;
}

/* 0xE1530, 122 bytes, global, 3 named locals
 * physics_gForceFaceTarget
 * PDB type: long (objectRoot*, objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int32_t physics_gForceFaceTarget(
    objectRoot *player, objectRoot *target)
{
    physicsObject *p = physics_from_root(player);

    if ((p->flags & UINT32_C(0x400000)) == 0) {
        VECTOR *targetpos = NULL;
        int32_t aoa = 0;

        if (target != NULL) {
            targetpos = &physics_from_root(target)->vpos;
        }
        if (targetpos != NULL) {
            int32_t dx = (int32_t)(
                (uint32_t)targetpos->vx -
                (uint32_t)p->vpos.vx);
            int32_t dz = (int32_t)(
                (uint32_t)targetpos->vz -
                (uint32_t)p->vpos.vz);

            aoa = ratan2(dx, dz) & 0x0fff;
        }
        p->angle.vy = aoa;
    }
    return 0;
}

/* 0xE15B0, 14 bytes, global, 1 named locals
 * physics_gGetConstantVector
 * PDB type: FVECTOR* (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
FVECTOR *physics_gGetConstantVector(objectRoot *object)
{
    return &physics_from_root(object)->constmov;
}

/* 0xE15C0, 132 bytes, global, 6 named locals
 * physics_gGetFaceTargetDelta
 * PDB type: int (objectRoot*, objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int32_t physics_gGetFaceTargetDelta(
    objectRoot *player, objectRoot *target)
{
    VECTOR *playerpos = NULL;
    VECTOR *targetpos = NULL;
    int32_t aoa = 0;
    physicsObject *p;

    if (player != NULL) {
        playerpos = &physics_from_root(player)->vpos;
    }
    if (target != NULL) {
        targetpos = &physics_from_root(target)->vpos;
    }
    if (playerpos != NULL && targetpos != NULL) {
        int32_t dx = (int32_t)(
            (uint32_t)targetpos->vx -
            (uint32_t)playerpos->vx);
        int32_t dz = (int32_t)(
            (uint32_t)targetpos->vz -
            (uint32_t)playerpos->vz);

        aoa = ratan2(dx, dz) & 0x0fff;
    }
    p = physics_from_root(player);
    return (int32_t)(
        (uint32_t)(p != NULL ? p->angle.vy : 0) -
        (uint32_t)aoa);
}

/* 0xE1650, 17 bytes, global, 2 named locals
 * physics_gGetFacing
 * PDB type: int (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int physics_gGetFacing(objectRoot *object)
{
    physicsObject *physics = physics_from_root(object);

    return physics != NULL ? physics->angle.vy : 0;
}

/* 0xE1670, 197 bytes, global, 6 named locals
 * physics_gGetNearestTarget
 * PDB type: objectRoot* (objectRoot*, int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
objectRoot *physics_gGetNearestTarget(
    objectRoot *object0, int type)
{
    objectRoot *target = NULL;
    int nearest_range = 0x2000;
    int index;

    for (index = 2;
         index < JPB_PHYSICS_CAPACITY;
         ++index) {
        objectRoot *candidate =
            &maPhysicsData[index].physicsRoot;
        sceneObject *scene;
        playerObject *player;
        int range;

        if (object0->objectID == index ||
            candidate->objectID == -1 ||
            obj_gCheckObjectFlag(
                candidate, 0, UINT32_C(0x20)) != 0) {
            continue;
        }
        scene = (sceneObject *)candidate->pParent;
        player = (playerObject *)scene->pPlayer;
        if (player->pEnemy->pPlace->aiDf.daDelay != type) {
            continue;
        }
        range = physics_gGetRange(object0, candidate);
        if (range > 0 && range < nearest_range) {
            target = candidate;
            nearest_range = range;
        }
    }
    return target;
}

/* 0xE1740, 204 bytes, global, 4 named locals
 * physics_gGetNewObject
 * PDB type: physicsObject* (int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
physicsObject *physics_gGetNewObject(int ID)
{
    physicsObject *physics = NULL;
    int index;

    if (ID < 0) {
        for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
            physics = &maPhysicsData[index];
            if (physics->physicsRoot.objectID == -1) {
                physics->physicsRoot.objectID = index;
                break;
            }
        }
        if (index == JPB_PHYSICS_CAPACITY) {
            return NULL;
        }
    } else if (ID < JPB_PHYSICS_CAPACITY) {
        physics = &maPhysicsData[ID];
        if (physics->physicsRoot.objectID == -1) {
            physics->physicsRoot.objectID = ID;
        }
    } else {
        return NULL;
    }

    if (physics->solid != NULL) {
        if (physics->solid->coords != NULL) {
            memfree(physics->solid->coords);
        }
        memfree(physics->solid);
        physics->solid = NULL;
    }
    physics->flags = 0;
    physics->lastpolyhit = NULL;
    memset(physics->userdata, 0, sizeof(physics->userdata));
    memset(&physics->uservector, 0, sizeof(physics->uservector));
    return physics;
}

/* 0xE1810, 22 bytes, global, 1 named locals
 * physics_gGetPosition
 * PDB type: VECTOR* (objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
VECTOR *physics_gGetPosition(objectRoot *object)
{
    if (object != NULL) {
        return &physics_from_root(object)->vpos;
    }
    return NULL;
}

/* 0xE1830, 113 bytes, global, 6 named locals
 * physics_gGetRange
 * PDB type: int (objectRoot*, objectRoot*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
int physics_gGetRange(
    objectRoot *object0, objectRoot *object1)
{
    int a = object0->objectID;
    int b = object1->objectID;

    if (a >= 0 && b >= 0) {
        int low = a <= b ? a : b;
        int high = a <= b ? b : a;
        int cached = (int)maRange[low][high];

        if (cached >= 0) {
            return cached;
        }
        {
            sceneObject *scene0 =
                (sceneObject *)object0->pParent;
            sceneObject *scene1 =
                (sceneObject *)object1->pParent;
            physicsObject *p0 =
                (physicsObject *)scene0->pPhysics;
            physicsObject *p1 =
                (physicsObject *)scene1->pPhysics;

            if (p0 != NULL && p1 != NULL) {
                return (int)vec_DistanceLV(
                    &p0->vpos, &p1->vpos);
            }
        }
    }
    return -1;
}

/* 0xE18B0, 230 bytes, global, 1 named locals
 * physics_gInitObjects
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gInitObjects(int start)
{
    int index;

    /*
     * Valid callers pass 0..19. The reference's signed comparison would let
     * a negative start walk before maPhysicsData; reject that corrupting case
     * while preserving the stores for every valid pool element.
     */
    if ((uint32_t)start < JPB_PHYSICS_CAPACITY) {
        for (index = start; index < JPB_PHYSICS_CAPACITY; ++index) {
            physicsObject *physics = &maPhysicsData[index];

            physics->physicsRoot.pParent = NULL;
            physics->physicsRoot.flags = 0;
            memset(
                &physics->matrix,
                0,
                offsetof(physicsObject, turnspeed) -
                    offsetof(physicsObject, matrix));
            memset(
                &physics->noncollideframes,
                0,
                sizeof(*physics) -
                    offsetof(physicsObject, noncollideframes));
            physics->turnspeed = 0x500;
            physics->radius = 0x36;
            physics->mass = 0x800;
            physics->height = 0xdc;
            physics->physicsRoot.objectID = -1;
            memcpy(
                physics->physicsRoot.objectName,
                "PHYSICS",
                sizeof("PHYSICS"));
        }
    }
    numsolids = 0;
}

/* 0xE19A0, 99 bytes, global, 2 named locals
 * physics_gModFacing
 * PDB type: void (objectRoot*, int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gModFacing(objectRoot *object, int amount)
{
    physicsObject *physics =
        physics_from_root(object);

    if (gSCENE_READY != 0 &&
        (physics->flags & 0x00400000u) != 0) {
        return;
    }
    physics->angle.vy =
        (physics->angle.vy + amount) & 0x0fff;
    physics->flags |= 0x00001000u;
    scene_gSetSceneModelMatrixFV(
        object->objectID,
        &physics->angle,
        &physics->pos);
}

/* 0xE1A10, 164 bytes, global, 7 named locals
 * physics_gSetCharge
 * PDB type: void (playerObject*, int, int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gSetCharge(playerObject *player, int charge, int charge_acc)
{
    physicsObject *physics =
        physics_from_root(&player->playerRoot);

    /*
     * The reference computes this target bearing and discards the returned
     * angle. Preserve the call while the surrounding target-facing behavior
     * is recovered, rather than assigning a meaning not present in assembly.
     */
    if (player->target != NULL) {
        physicsObject *target =
            physics_from_root(&player->target->playerRoot);

        if (physics != NULL && target != NULL) {
            (void)ratan2(
                target->vpos.vx - physics->vpos.vx,
                target->vpos.vz - physics->vpos.vz);
        }
    }

    physics->constmov.vz = (float)charge;
    physics->constmov.vx = 0.0f;
    physics->accel.vz =
        charge_acc < 0 ? 0.0f : (float)charge_acc;
}

/* 0xE1AC0, 32 bytes, global, 5 named locals
 * physics_gSetConstantVector
 * PDB type: void (objectRoot*, float, float,...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gSetConstantVector(
    objectRoot *object, float vx, float vy, float vz)
{
    physicsObject *physics = physics_from_root(object);

    physics->constmov.vx = vx;
    physics->constmov.vy = vy;
    physics->constmov.vz = vz;
}

/* 0xE1AE0, 40 bytes, global, 2 named locals
 * physics_gSetFacing
 * PDB type: void (objectRoot*, int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gSetFacing(objectRoot *object, int facing)
{
    physicsObject *physics = physics_from_root(object);

    if (gSCENE_READY == 0 || (physics->flags & 0x00400000u) == 0) {
        physics->angle.vy = facing;
    }
}

/* 0xE1B10, 76 bytes, global, 5 named locals
 * physics_gSetPosition
 * PDB type: void (objectRoot*, int, int, int...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gSetPosition(
    objectRoot *object, int x, int y, int z)
{
    physicsObject *physics = physics_from_root(object);

    physics->pos.vx = (float)x;
    physics->lastpos.vx = (float)x;
    physics->pos.vy = (float)y;
    physics->lastpos.vy = (float)y;
    physics->pos.vz = (float)z;
    physics->lastpos.vz = (float)z;
    scene_gSetSceneModelMatrixFV(
        physics->physicsRoot.objectID,
        &physics->angle,
        &physics->pos);
}

/* 0xE1B60, 215 bytes, global, 7 named locals
 * physics_gSetRecoil
 * PDB type: void (playerObject*, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gSetRecoil(
    playerObject *player, int recoil, int acc, int REV)
{
    sceneObject *scene =
        (sceneObject *)player->playerRoot.pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;

    if ((physics->flags & UINT32_C(0x00400000)) != 0) {
        return;
    }
    if (REV == 0) {
        playerObject *target = player->target;
        FVECTOR *position = &physics->pos;
        FVECTOR *target_position = NULL;

        if (target != NULL) {
            sceneObject *target_scene =
                (sceneObject *)target->playerRoot.pParent;
            physicsObject *target_physics =
                (physicsObject *)target_scene->pPhysics;

            target_position = &target_physics->pos;
        }
        if (position != NULL && target_position != NULL) {
            physics->angle.vy =
                ratan2(
                    (int)(target_position->vx - position->vx),
                    (int)(target_position->vz - position->vz)) &
                0x0fff;
        } else {
            physics->angle.vy = 0;
        }
    } else {
        recoil = -recoil;
    }
    physics->constmov.vz += (float)recoil;
    physics->accel.vz = acc < 0 ? 0.0f : (float)acc;
}

/* 0xE1C40, 197 bytes, global, 6 named locals
 * physics_gSnapShotPosition
 * PDB type: void (objectRoot*, int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gSnapShotPosition(
    objectRoot *object, int yoffset)
{
    if (object->objectID != -1 &&
        obj_gCheckObjectFlag(object, 0, UINT32_C(0x20)) == 0) {
        sceneObject *scene =
            (sceneObject *)object->pParent;
        playerObject *player =
            (playerObject *)scene->pPlayer;
        physicsObject *p =
            (physicsObject *)scene->pPhysics;
        modelObject *model =
            (modelObject *)scene->pModel;

        if ((player->pFlags & UINT32_C(0x2000)) == 0) {
            scene_gGetSceneModelMatrixFV(
                p->physicsRoot.objectID,
                NULL,
                &p->pos,
                &p->snapshotpos);
            p->snapshotpos.vy += (float)yoffset;
            if ((model->flags & UINT32_C(0x20)) != 0) {
                VECTOR *pelvis =
                    coll_GetNodeCenter(
                        player->playerID, 0);

                p->snapshotpos.vy =
                    (float)pelvis->vy;
            }
            scene_gSetSceneModelMatrixFV(
                p->physicsRoot.objectID,
                &p->angle,
                &p->pos);
        }
    }
}

/* 0xE1D10, 43 bytes, global, 3 named locals
 * physics_gSwapVel
 * PDB type: void (playerObject*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gSwapVel(playerObject *player)
{
    physicsObject *physics =
        physics_from_root(&player->playerRoot);
    float old_x = physics->constmov.vx;

    physics->constmov.vx = physics->constmov.vz;
    physics->constmov.vz = (float)(int32_t)old_x;
}

/* 0xE1D40, 157 bytes, global, 5 named locals
 * physics_gTurnToAttack
 * PDB type: void (objectRoot*, int, int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gTurnToAttack(
    objectRoot *object, int facing, int scalar)
{
    sceneObject *scene =
        (sceneObject *)object->pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    uint32_t wrapped_delta;
    int delta;

    if (gSCENE_READY != 0 &&
        (((physics->flags & 0x00400060u) != 0) ||
         physics->movemode == MOVE_HOVER)) {
        return;
    }

    wrapped_delta =
        ((uint32_t)physics->angle.vy -
         (uint32_t)facing) &
        0x0fffu;
    delta = wrapped_delta >= 0x0800u
                ? (int)wrapped_delta - 0x1000
                : (int)wrapped_delta;
    if (physics->vmov.vx != 0 &&
        anim_CheckFreeze(&physics->physicsRoot) == 0 &&
        delta != 0) {
        physics->angle.vy -=
            flexmul(delta / scalar, gGlobalFrameRate);
        physics->flags |= 0x00001000u;
    }
}

/* 0xE1DE0, 304 bytes, global, 8 named locals
 * physics_gTurnToFace
 * PDB type: void (objectRoot*, int, int)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
void physics_gTurnToFace(
    objectRoot *object, int facing, int scalar)
{
    sceneObject *scene =
        (sceneObject *)object->pParent;
    physicsObject *physics =
        (physicsObject *)scene->pPhysics;
    playerObject *player =
        (playerObject *)scene->pPlayer;
    uint32_t wrapped_delta;
    int delta;
    int turn = 0;
    int turn_speed;
    int64_t scaled;

    if (gSCENE_READY != 0 &&
        (((physics->flags & 0x00400060u) != 0) ||
         physics->movemode == MOVE_HOVER)) {
        return;
    }
    if (player == NULL ||
        (player->pFlags & 0x44000000u) != 0 ||
        physics == NULL) {
        return;
    }

    wrapped_delta =
        ((uint32_t)physics->angle.vy -
         (uint32_t)facing) &
        0x0fffu;
    delta = wrapped_delta >= 0x0800u
                ? (int)wrapped_delta - 0x1000
                : (int)wrapped_delta;
    if (delta == 0) {
        return;
    }

    if (physics->movemode == MOVE_HOVER3D ||
        physics->movemode == MOVE_FLY) {
        int player_id = player->playerID;

        turn_speed =
            player_id == 0x43 ||
                    player_id == 0x44 ||
                    player_id == 0x45
                ? 0x0c
                : 0x60;
        if (delta > turn_speed) {
            turn = -turn_speed;
        } else if (delta < -turn_speed) {
            turn = turn_speed;
        } else {
            physics->angle.vy = facing;
        }
    } else if ((physics->flags & 0x00000100u) == 0) {
        int magnitude = delta < 0 ? -delta : delta;

        if ((unsigned)(magnitude - 0x40) < 0x781u &&
            scalar != 0) {
            turn = -(delta / scalar);
        } else {
            physics->angle.vy = facing;
        }
    } else if (delta > 8) {
        turn = -8;
    } else if (delta < -8) {
        turn = 8;
    } else {
        physics->angle.vy = facing;
    }

    physics->flags |= 0x00001000u;
    scaled = (int64_t)turn * (int64_t)gGlobalFrameRate;
    if (scaled < 0) {
        physics->angle.vy +=
            (int)(-1 -
                  ((-scaled - 1) / JPB_FIXED_ONE));
    } else {
        physics->angle.vy +=
            (int)(scaled / JPB_FIXED_ONE);
    }
}

/* 0xE1F10, 410 bytes, local, 5 named locals
 * planecheck
 * PDB type: int (int, FVECTOR4*)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static int planecheck(int radius, FVECTOR4 *v)
{
    int match = 0;
    float t;
    float hitDist;

    mvp.radius = (float)radius;
    hitDist = -(
        mvp.movement.vy * v->vy +
        mvp.movement.vx * v->vx +
        mvp.movement.vz * v->vz);

    if (hitDist > 0.0f) {
        t =
            (mvp.startpos.vy * v->vy +
             mvp.startpos.vx * v->vx +
             mvp.startpos.vz * v->vz) -
            v->vw -
            (float)radius;

        if (t < 0.0f ||
            hitDist * mvp.distance < mvp.distance) {
            mvp.info.washack = 0;
            mvp.info.type = 1;
            mvp.info.dist = t / hitDist;
            if (mvp.info.dist < 0.0f) {
                mvp.info.dist = 0.0f;
            }
            mvp.info.facenormal.vx = v->vx;
            mvp.info.facenormal.vy = v->vy;
            mvp.info.facenormal.vz = v->vz;
            mvp.info.n.vx = -v->vx;
            mvp.info.n.vy = -v->vy;
            mvp.info.n.vz = -v->vz;

            if (mvp.info.dist < bestinfo.dist) {
                bestinfo = mvp.info;
                bestinfo.type |= 8;
                bestinfo.washack = 0;
                match = 1;
            }
        }
    }

    return match;
}

/*
 * Portable test/integration facade for the original module-local helper.
 * The jpb_ prefix deliberately distinguishes it from a recovered PDB symbol.
 */
int jpb_PhysicsPlaneCheck(int radius, FVECTOR4 *plane)
{
    return planecheck(radius, plane);
}

/* 0xE20B0, 512 bytes, local, 6 named locals
 * polycollidecheck
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static int polycollidecheck(void)
{
    int match;
    int newdist;
    int olddist;
    int newedge;
    int e;

    match = sphereAndPoly();
    if (match == 0) {
        return 0;
    }

    if ((mvp.info.type & 4) != 0) {
        if ((bestinfo.type & 4) != 0 &&
            bestinfo.dist <= mvp.info.dist) {
            return 0;
        }
        bestinfo = mvp.info;
        return 1;
    }

    if ((bestinfo.type & 4) != 0) {
        return 0;
    }

    if (mvp.info.type == 2 && bestinfo.type == 2) {
        newdist = (int)mvp.info.dist - (int)bestinfo.dist;
        olddist = newdist < 0 ? -newdist : newdist;
        if (olddist < 2) {
            match =
                bestinfo.facenormal.vy <
                mvp.info.facenormal.vy;
            if (match != 0) {
                bestinfo = mvp.info;
            }
            if (match == 0) {
                return 0;
            }
            goto publish_edge;
        }
    }

    if ((int)bestinfo.dist <= (int)mvp.info.dist) {
        return 0;
    }

    bestinfo = mvp.info;
    match = 1;
    if (mvp.info.type != 2) {
        return match;
    }

publish_edge:
    newedge = bestinfo.edge;
    e = newedge + 1;
    if (mvp.numsides <= e) {
        e = 0;
    }
    edge_end = mvp.points[newedge];
    edge_start = mvp.points[e];
    return match;
}

/*
 * Portable test/integration facade for the original module-local selector.
 */
int jpb_PhysicsPolyCollideCheck(void)
{
    return polycollidecheck();
}

/* 0xE22B0, 4699 bytes, local, 31 named locals
 * sphereAndPoly
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\Work\physics.c
 */
static int sphereAndPoly(void)
{
    float pointlen;
    int i;
    int edgeintersect = 0;
    FVECTOR *goodpoint = NULL;
    int dotMask = 0;
    int edgeMask = 0;
    float minradsquared;
    float bestEdge = 0.0f;
    FVECTOR pl;
    FVECTOR En;
    FVECTOR pointonedge = {0.0f, 0.0f, 0.0f};
    FVECTOR off;
    float t;
    FVECTOR K;
    float hitDist;
    int dot;
    FVECTOR L;
    FVECTOR edgePlane;
    float tmpf;
    float distToEdge;
    float temp;
    FVECTOR Delta;
    float adjustSqr;
    float bob;
    FVECTOR *testPoint;
    FVECTOR NR;
    unsigned edgeBit;

    cvars.moveToPlaneDot =
        mvp.movement.vy * mvp.facenormal.vy +
        mvp.movement.vx * mvp.facenormal.vx +
        mvp.movement.vz * mvp.facenormal.vz;
    if (cvars.moveToPlaneDot >= 0.0f) {
        return 0;
    }

    cvars.tmp.vx = mvp.startpos.vx - mvp.points[0].vx;
    cvars.tmp.vy = mvp.startpos.vy - mvp.points[0].vy;
    cvars.tmp.vz = mvp.startpos.vz - mvp.points[0].vz;
    cvars.distToPlane =
        cvars.tmp.vy * mvp.facenormal.vy +
        cvars.tmp.vx * mvp.facenormal.vx +
        cvars.tmp.vz * mvp.facenormal.vz;
    if (mvp.radius + mvp.distance <= cvars.distToPlane ||
        cvars.distToPlane <= 0.0f) {
        return 0;
    }

    cvars.bestDist = mvp.distance;
    cvars.rsquared = mvp.radius * mvp.radius;
    cvars.sideMask = 0;
    cvars.vmin = mvp.points[0];
    cvars.vmax = mvp.points[0];

    for (i = 0; i < mvp.numsides; ++i) {
        int next = i + 1;

        if (mvp.numsides <= next) {
            next = 0;
        }
        cvars.P0[i] = &mvp.points[i];
        cvars.P1[i] = &mvp.points[next];
        cvars.edge[i].vx =
            cvars.P0[i]->vx - cvars.P1[i]->vx;
        cvars.edge[i].vy =
            cvars.P0[i]->vy - cvars.P1[i]->vy;
        cvars.edge[i].vz =
            cvars.P0[i]->vz - cvars.P1[i]->vz;
        cvars.edgenormal[i].vx =
            cvars.edge[i].vy * mvp.facenormal.vz -
            cvars.edge[i].vz * mvp.facenormal.vy;
        cvars.edgenormal[i].vy =
            cvars.edge[i].vz * mvp.facenormal.vx -
            cvars.edge[i].vx * mvp.facenormal.vz;
        cvars.edgenormal[i].vz =
            cvars.edge[i].vx * mvp.facenormal.vy -
            cvars.edge[i].vy * mvp.facenormal.vx;

        if (i != 0) {
            if (cvars.vmax.vx < mvp.points[i].vx) {
                cvars.vmax.vx = mvp.points[i].vx;
            }
            if (cvars.vmax.vy < mvp.points[i].vy) {
                cvars.vmax.vy = mvp.points[i].vy;
            }
            if (cvars.vmax.vz < mvp.points[i].vz) {
                cvars.vmax.vz = mvp.points[i].vz;
            }
            if (mvp.points[i].vx < cvars.vmin.vx) {
                cvars.vmin.vx = mvp.points[i].vx;
            }
            if (mvp.points[i].vy < cvars.vmin.vy) {
                cvars.vmin.vy = mvp.points[i].vy;
            }
            if (mvp.points[i].vz < cvars.vmin.vz) {
                cvars.vmin.vz = mvp.points[i].vz;
            }
        }
    }

    if (cvars.vmax.vx < mvp.vmin.vx ||
        cvars.vmax.vy < mvp.vmin.vy ||
        cvars.vmax.vz < mvp.vmin.vz ||
        mvp.vmax.vx < cvars.vmin.vx ||
        mvp.vmax.vy < cvars.vmin.vy ||
        mvp.vmax.vz < cvars.vmin.vz) {
        return 0;
    }

    if (cvars.distToPlane <= mvp.radius) {
        float closestPointDist;

        pl.vx =
            mvp.startpos.vx -
            cvars.distToPlane * mvp.facenormal.vx;
        pl.vy =
            mvp.startpos.vy -
            cvars.distToPlane * mvp.facenormal.vy;
        pl.vz =
            mvp.startpos.vz -
            cvars.distToPlane * mvp.facenormal.vz;
        minradsquared =
            cvars.rsquared -
            cvars.distToPlane * cvars.distToPlane;
        closestPointDist = minradsquared;

        for (i = 0; i < mvp.numsides; ++i) {
            float edgeDistance;

            En = cvars.edgenormal[i];
            VectorNormalize(&En);
            L.vx = pl.vx - cvars.P0[i]->vx;
            L.vy = pl.vy - cvars.P0[i]->vy;
            L.vz = pl.vz - cvars.P0[i]->vz;
            edgeDistance =
                En.vy * L.vy +
                L.vx * En.vx +
                L.vz * En.vz;
            if (mvp.radius <= edgeDistance) {
                dotMask = 0;
                goto moving_contact;
            }

            if (edgeDistance < 0.0f) {
                dotMask |= 1 << i;
                continue;
            }

            pointlen =
                cvars.edge[i].vy * cvars.edge[i].vy +
                cvars.edge[i].vx * cvars.edge[i].vx +
                cvars.edge[i].vz * cvars.edge[i].vz;
            if (edgeDistance * edgeDistance <= minradsquared) {
                edgeMask |= 1 << i;
            }

            pointonedge.vx = pl.vx - En.vx * edgeDistance;
            pointonedge.vy = pl.vy - En.vy * edgeDistance;
            pointonedge.vz = pl.vz - En.vz * edgeDistance;

            Delta.vx = cvars.P0[i]->vx - pointonedge.vx;
            Delta.vy = cvars.P0[i]->vy - pointonedge.vy;
            Delta.vz = cvars.P0[i]->vz - pointonedge.vz;
            if (pointlen <
                Delta.vx * Delta.vx +
                Delta.vy * Delta.vy +
                Delta.vz * Delta.vz) {
                Delta.vx = pl.vx - cvars.P1[i]->vx;
                Delta.vy = pl.vy - cvars.P1[i]->vy;
                Delta.vz = pl.vz - cvars.P1[i]->vz;
                temp =
                    Delta.vy * Delta.vy +
                    Delta.vx * Delta.vx +
                    Delta.vz * Delta.vz;
                if (temp < closestPointDist) {
                    goodpoint = cvars.P1[i];
                    closestPointDist = temp;
                }
            } else {
                Delta.vx = cvars.P1[i]->vx - pointonedge.vx;
                Delta.vy = cvars.P1[i]->vy - pointonedge.vy;
                Delta.vz = cvars.P1[i]->vz - pointonedge.vz;
                if (pointlen <
                    Delta.vx * Delta.vx +
                    Delta.vy * Delta.vy +
                    Delta.vz * Delta.vz) {
                    Delta.vx = pl.vx - cvars.P0[i]->vx;
                    Delta.vy = pl.vy - cvars.P0[i]->vy;
                    Delta.vz = pl.vz - cvars.P0[i]->vz;
                    temp =
                        Delta.vx * Delta.vx +
                        Delta.vy * Delta.vy +
                        Delta.vz * Delta.vz;
                    if (temp < closestPointDist) {
                        goodpoint = cvars.P0[i];
                        closestPointDist = temp;
                    }
                } else {
                    bestEdge = -edgeDistance;
                    edgeintersect = 1;
                    cvars.edge_start = i;
                }
            }
        }

        if (edgeMask != 0) {
            if (goodpoint == NULL) {
                if (edgeintersect != 0) {
                    hitDist =
                        (float)(
                            (double)cvars.distToPlane -
                            sqrt(
                                (double)(
                                    cvars.rsquared -
                                    bestEdge * bestEdge)));
                    mvp.info.dist = hitDist;
                    if (hitDist < 0.0f) {
                        mvp.info.type = 6;
                        mvp.info.kisspoint = pointonedge;
                        mvp.info.edge = cvars.edge_start;
                        mvp.info.n = mvp.facenormal;
                        mvp.info.facenormal = mvp.facenormal;
                        return 1;
                    }
                }
            } else {
                Delta.vx = pl.vx - goodpoint->vx;
                Delta.vy = pl.vy - goodpoint->vy;
                Delta.vz = pl.vz - goodpoint->vz;
                if (Delta.vy * Delta.vy +
                        Delta.vx * Delta.vx +
                        Delta.vz * Delta.vz <=
                    minradsquared) {
                    hitDist =
                        (float)(
                            sqrt((double)minradsquared) -
                            sqrt((double)closestPointDist));
                    mvp.info.dist = hitDist;
                    if (hitDist < 0.0f) {
                        mvp.info.kisspoint = *goodpoint;
                        mvp.info.type = 7;
                        mvp.info.facenormal = mvp.facenormal;
                        mvp.info.n = Delta;
                        VectorNormalize(&mvp.info.n);
                        return 1;
                    }
                }
            }
            goto moving_contact;
        }

        if (dotMask == (1 << mvp.numsides) - 1) {
            mvp.info.type = 5;
            mvp.info.dist =
                mvp.radius - cvars.distToPlane;
            mvp.info.kisspoint = pl;
            mvp.info.n = mvp.facenormal;
            mvp.info.facenormal = mvp.facenormal;
            return 1;
        }
    }

moving_contact:
    cvars.distToPlane -= mvp.radius;
    cvars.moveToPlaneDot = -cvars.moveToPlaneDot;
    if (cvars.distToPlane < mvp.distance &&
        cvars.distToPlane <
            cvars.moveToPlaneDot * mvp.distance) {
        FVECTOR center;

        t = cvars.distToPlane / cvars.moveToPlaneDot;
        center.vx = t * mvp.movement.vx + mvp.startpos.vx;
        center.vy = t * mvp.movement.vy + mvp.startpos.vy;
        center.vz = t * mvp.movement.vz + mvp.startpos.vz;

        if (cvars.distToPlane >= 0.0f) {
            cvars.sideMask = 0;
            for (i = 0; i < mvp.numsides; ++i) {
                L.vx = center.vx - cvars.P0[i]->vx;
                L.vy = center.vy - cvars.P0[i]->vy;
                L.vz = center.vz - cvars.P0[i]->vz;
                dot = (int)(
                    L.vy * cvars.edgenormal[i].vy +
                    L.vx * cvars.edgenormal[i].vx +
                    L.vz * cvars.edgenormal[i].vz);
                if (dot >= 0) {
                    cvars.sideMask |= 1 << i;
                }
            }

            if (cvars.sideMask == 0) {
                if (t >= 0.0f) {
                    mvp.info.type = 1;
                    mvp.info.kisspoint.vx =
                        center.vx -
                        mvp.radius * mvp.facenormal.vx;
                    mvp.info.kisspoint.vy =
                        center.vy -
                        mvp.radius * mvp.facenormal.vy;
                    mvp.info.kisspoint.vz =
                        center.vz -
                        mvp.radius * mvp.facenormal.vz;
                    cvars.bestDist = t;
                    mvp.info.dist = t;
                    mvp.info.facenormal = mvp.facenormal;
                }
                goto finish;
            }
        } else {
            cvars.sideMask = 0x0f;
        }

        edgeBit = 1;
        for (i = 0; i < mvp.numsides; ++i) {
            if ((cvars.sideMask & (int)edgeBit) != 0) {
                edgePlane.vx =
                    cvars.edge[i].vy * mvp.movement.vz -
                    cvars.edge[i].vz * mvp.movement.vy;
                edgePlane.vy =
                    cvars.edge[i].vz * mvp.movement.vx -
                    cvars.edge[i].vx * mvp.movement.vz;
                edgePlane.vz =
                    cvars.edge[i].vx * mvp.movement.vy -
                    cvars.edge[i].vy * mvp.movement.vx;
                VectorNormalize(&edgePlane);

                K.vx = cvars.P0[i]->vx - mvp.startpos.vx;
                K.vy = cvars.P0[i]->vy - mvp.startpos.vy;
                K.vz = cvars.P0[i]->vz - mvp.startpos.vz;
                distToEdge =
                    K.vy * edgePlane.vy +
                    K.vx * edgePlane.vx +
                    K.vz * edgePlane.vz;
                if (distToEdge < mvp.radius) {
                    adjustSqr =
                        cvars.rsquared -
                        distToEdge * distToEdge;
                    if (adjustSqr >= 0.0f) {
                        NR.vx =
                            cvars.edge[i].vz * edgePlane.vy -
                            cvars.edge[i].vy * edgePlane.vz;
                        NR.vy =
                            cvars.edge[i].vx * edgePlane.vz -
                            cvars.edge[i].vz * edgePlane.vx;
                        NR.vz =
                            cvars.edge[i].vy * edgePlane.vx -
                            cvars.edge[i].vx * edgePlane.vy;
                        VectorNormalize(&NR);

                        K.vx =
                            mvp.startpos.vx -
                            cvars.P0[i]->vx;
                        K.vy =
                            mvp.startpos.vy -
                            cvars.P0[i]->vy;
                        K.vz =
                            mvp.startpos.vz -
                            cvars.P0[i]->vz;
                        bob =
                            NR.vy * mvp.movement.vy +
                            NR.vx * mvp.movement.vx +
                            NR.vz * mvp.movement.vz;
                        temp = -(
                            K.vy * NR.vy +
                            K.vx * NR.vx +
                            K.vz * NR.vz);
                        hitDist =
                            (float)(
                                ((double)temp -
                                 sqrt((double)adjustSqr)) /
                                (double)bob);

                        if (hitDist < cvars.bestDist) {
                            off.vx =
                                hitDist * mvp.movement.vx +
                                mvp.startpos.vx -
                                cvars.P0[i]->vx;
                            off.vy =
                                hitDist * mvp.movement.vy +
                                mvp.startpos.vy -
                                cvars.P0[i]->vy;
                            off.vz =
                                hitDist * mvp.movement.vz +
                                mvp.startpos.vz -
                                cvars.P0[i]->vz;
                            pointlen =
                                cvars.edge[i].vy *
                                    cvars.edge[i].vy +
                                cvars.edge[i].vx *
                                    cvars.edge[i].vx +
                                cvars.edge[i].vz *
                                    cvars.edge[i].vz;
                            tmpf = -(
                                off.vy * cvars.edge[i].vy +
                                off.vx * cvars.edge[i].vx +
                                off.vz * cvars.edge[i].vz);

                            if (tmpf < 0.0f) {
                                testPoint = cvars.P0[i];
                            } else if (tmpf <= pointlen) {
                                if (hitDist >= 0.0f) {
                                    tmpf /= pointlen;
                                    mvp.info.kisspoint.vx =
                                        cvars.P0[i]->vx -
                                        cvars.edge[i].vx * tmpf;
                                    mvp.info.kisspoint.vy =
                                        cvars.P0[i]->vy -
                                        cvars.edge[i].vy * tmpf;
                                    mvp.info.kisspoint.vz =
                                        cvars.P0[i]->vz -
                                        cvars.edge[i].vz * tmpf;
                                    mvp.info.type = 2;
                                    mvp.info.facenormal =
                                        mvp.facenormal;
                                    cvars.bestDist = hitDist;
                                    mvp.info.dist = hitDist;
                                    mvp.info.edge = i;
                                }
                                goto next_edge;
                            } else {
                                testPoint = cvars.P1[i];
                            }

                            Delta.vx =
                                testPoint->vx -
                                mvp.startpos.vx;
                            Delta.vy =
                                testPoint->vy -
                                mvp.startpos.vy;
                            Delta.vz =
                                testPoint->vz -
                                mvp.startpos.vz;
                            temp =
                                Delta.vy * mvp.movement.vy +
                                Delta.vx * mvp.movement.vx +
                                Delta.vz * mvp.movement.vz;
                            K.vx =
                                testPoint->vx -
                                (temp * mvp.movement.vx +
                                 mvp.startpos.vx);
                            K.vy =
                                testPoint->vy -
                                (temp * mvp.movement.vy +
                                 mvp.startpos.vy);
                            K.vz =
                                testPoint->vz -
                                (temp * mvp.movement.vz +
                                 mvp.startpos.vz);
                            adjustSqr =
                                cvars.rsquared -
                                (K.vy * K.vy +
                                 K.vx * K.vx +
                                 K.vz * K.vz);
                            if (adjustSqr > 0.0f) {
                                hitDist =
                                    (float)(
                                        (double)temp -
                                        sqrt((double)adjustSqr));
                                if (hitDist < cvars.bestDist &&
                                    hitDist >= 0.0f) {
                                    mvp.info.type = 3;
                                    mvp.info.kisspoint =
                                        *testPoint;
                                    mvp.info.facenormal =
                                        mvp.facenormal;
                                    cvars.bestDist = hitDist;
                                    mvp.info.dist = hitDist;
                                }
                            }
                        }
                    }
                }
            }

next_edge:
            edgeBit =
                (edgeBit << 1) |
                (edgeBit >> 31);
        }
    }

finish:
    return cvars.bestDist < mvp.distance;
}

/*
 * Portable test/integration facade for the original module-local kernel.
 */
int jpb_PhysicsSphereAndPoly(void)
{
    return sphereAndPoly();
}
